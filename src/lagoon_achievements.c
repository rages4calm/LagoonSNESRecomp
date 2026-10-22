/* Host-only CompanionAchievements adapter. Gameplay only queues immutable facts;
 * the serialized storage worker never owns guest memory or guest clocks. */
#include "lagoon_achievements.h"
#include "lagoon_achievements_catalog.generated.h"
#include "cJSON.h"
#include <SDL3/SDL.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <unistd.h>
#endif

enum { PATH_CAP=4096, UNKNOWN_CAP=64, EVENT_CAP=32 };
enum Status { OFF, READY, DIAGNOSTIC, IO_ERROR, CORRUPT, LOCKED, WORKER_ERROR };
typedef struct LagoonUnknownAchievement {char id[81];uint32_t progress;uint64_t when;} LagoonUnknownAchievement;
typedef struct Event {const char *name;uint32_t amount;uint64_t when;} Event;
typedef struct Snapshot {
  uint8_t story[16],items[16],chests[16],level;
  uint16_t gold,room;
} Snapshot;
static struct {
  ca_engine engine;ca_state published[CA_MAX_ACHIEVEMENTS];
  SDL_Mutex *mutex;SDL_Condition *condition;SDL_Thread *thread;
  Event events[EVENT_CAP];unsigned pending;
  size_t notices[CA_MAX_ACHIEVEMENTS],notice_read,notice_count;
  size_t newly_unlocked[CA_MAX_ACHIEVEMENTS],new_count;
  LagoonUnknownAchievement unknown[UNKNOWN_CAP];size_t unknown_count;
  char profile[PATH_CAP];enum Status status;
  bool stop,busy,tracking,baseline,observed_ready;
  uint64_t generation;Snapshot previous;bool boss_armed[7];
#ifdef _WIN32
  HANDLE lock;
#else
  int lock;
#endif
} a;
static uint16_t word(const uint8_t*p){return (uint16_t)(p[0]|p[1]<<8);}
static bool bit(const uint8_t*p,unsigned n){return !!(p[n/8]&(0x80u>>(n&7)));}
static unsigned chest_count(const Snapshot*s){unsigned n=0;for(unsigned i=0;i<16;i++)for(unsigned b=0;b<8;b++)n+=!!(s->chests[i]&(1u<<b));return n;}

#ifdef _WIN32
static bool wide(const char*p,wchar_t*out){return MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,p,-1,out,PATH_CAP)!=0;}
static FILE *open_file(const char*p,const wchar_t*mode){wchar_t w[PATH_CAP];return wide(p,w)?_wfopen(w,mode):NULL;}
static bool directory_one(const char*p){wchar_t w[PATH_CAP];if(!wide(p,w))return false;DWORD attr=GetFileAttributesW(w);if(attr!=INVALID_FILE_ATTRIBUTES)return (attr&FILE_ATTRIBUTE_DIRECTORY)&&!(attr&FILE_ATTRIBUTE_REPARSE_POINT);return CreateDirectoryW(w,NULL)!=0;}
static bool regular_target(const char*p){wchar_t w[PATH_CAP];if(!wide(p,w))return false;DWORD attr=GetFileAttributesW(w);return attr==INVALID_FILE_ATTRIBUTES?GetLastError()==ERROR_FILE_NOT_FOUND:!(attr&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT));}
#else
static FILE *open_file(const char*p,const char*mode){return fopen(p,mode);}
static bool directory_one(const char*p){struct stat s;if(lstat(p,&s)==0)return S_ISDIR(s.st_mode)&&!S_ISLNK(s.st_mode);return errno==ENOENT&&mkdir(p,0700)==0;}
static bool regular_target(const char*p){struct stat s;if(lstat(p,&s)==0)return S_ISREG(s.st_mode)&&!S_ISLNK(s.st_mode);return errno==ENOENT;}
#endif
static bool directories(const char*path){
  char p[PATH_CAP];size_t n=strlen(path);if(!n||n>=PATH_CAP-64)return false;
#ifdef _WIN32
  if(!((n>=3&&path[1]==':'&&(path[2]=='/'||path[2]=='\\'))||(n>=5&&path[0]=='\\'&&path[1]=='\\')))return false;
#else
  if(path[0]!='/')return false;
#endif
  memcpy(p,path,n+1);unsigned unc=path[0]=='\\'&&path[1]=='\\'?2:0;
  for(size_t i=1;i<n;i++)if(p[i]=='/'||p[i]=='\\'){
    if(i==2&&p[1]==':')continue;if(p[i-1]=='/'||p[i-1]=='\\')continue;
    if(unc){--unc;continue;}char c=p[i];p[i]=0;bool ok=directory_one(p);p[i]=c;if(!ok)return false;
  }
  return directory_one(p);
}
static bool acquire_lock(void){
  char path[PATH_CAP];if(snprintf(path,sizeof path,"%s.lock",a.profile)>=(int)sizeof path||!regular_target(path))return false;
#ifdef _WIN32
  wchar_t w[PATH_CAP];if(!wide(path,w))return false;
  a.lock=CreateFileW(w,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);return a.lock!=INVALID_HANDLE_VALUE;
#else
  a.lock=open(path,O_RDWR|O_CREAT,0600);if(a.lock<0)return false;if(flock(a.lock,LOCK_EX|LOCK_NB)==0)return true;close(a.lock);a.lock=-1;return false;
#endif
}
static void release_lock(void){
#ifdef _WIN32
  if(a.lock!=INVALID_HANDLE_VALUE)CloseHandle(a.lock);a.lock=INVALID_HANDLE_VALUE;
#else
  if(a.lock>=0)close(a.lock);a.lock=-1;
#endif
}
static bool integer(const cJSON*x,double maximum){return cJSON_IsNumber(x)&&isfinite(x->valuedouble)&&x->valuedouble>=0&&x->valuedouble<=maximum&&floor(x->valuedouble)==x->valuedouble;}
static bool object_keys(const cJSON*x,const char*const*keys,size_t count){
  if(!cJSON_IsObject(x))return false;unsigned seen=0;size_t actual=0;
  for(cJSON*c=x->child;c;c=c->next){size_t k=0;for(;k<count;k++)if(c->string&&!strcmp(c->string,keys[k]))break;if(k==count||(seen&(1u<<k)))return false;seen|=1u<<k;actual++;}
  return actual==count;
}
static bool logical_id(const char*p){if(!p||!*p||strlen(p)>80)return false;for(;*p;p++)if(!((*p>='a'&&*p<='z')||(*p>='0'&&*p<='9')||*p=='-'||*p=='_'||*p=='.'))return false;return true;}
static size_t definition_index(const char*id){size_t i=0;for(;i<a.engine.count;i++)if(!strcmp(id,lagoon_definitions[i].id))break;return i;}
static bool restore(FILE*f){
  char text[32769];size_t n=fread(text,1,sizeof text-1,f);if(ferror(f)||!feof(f)||n==sizeof text-1)return false;text[n]=0;
  if(memchr(text,0,n)||strstr(text,"\\u0000"))return false;
  const char*end=NULL;cJSON*r=cJSON_ParseWithLengthOpts(text,n+1,&end,1);if(!r)return false;
  const char*keys[]={"schemaVersion","gameId","editionId","seedId","catalogVersion","achievements"};
  const char*itemkeys[]={"id","progress","unlockedAt"};
  cJSON*schema=cJSON_GetObjectItemCaseSensitive(r,"schemaVersion"),*game=cJSON_GetObjectItemCaseSensitive(r,"gameId"),*edition=cJSON_GetObjectItemCaseSensitive(r,"editionId"),*seed=cJSON_GetObjectItemCaseSensitive(r,"seedId"),*version=cJSON_GetObjectItemCaseSensitive(r,"catalogVersion"),*items=cJSON_GetObjectItemCaseSensitive(r,"achievements");
  bool ok=object_keys(r,keys,6)&&integer(schema,1)&&schema->valuedouble==1&&cJSON_IsString(game)&&!strcmp(game->valuestring,LAGOON_GAME_ID)&&cJSON_IsString(edition)&&!strcmp(edition->valuestring,LAGOON_EDITION_ID)&&cJSON_IsNull(seed)&&integer(version,LAGOON_CATALOG_VERSION)&&version->valuedouble>=1&&cJSON_IsArray(items);
  char seen[CA_MAX_ACHIEVEMENTS+UNKNOWN_CAP][81];size_t seen_count=0;
  for(cJSON*i=items?items->child:NULL;ok&&i;i=i->next){
    cJSON*id=cJSON_GetObjectItemCaseSensitive(i,"id"),*p=cJSON_GetObjectItemCaseSensitive(i,"progress"),*t=cJSON_GetObjectItemCaseSensitive(i,"unlockedAt");
    ok=object_keys(i,itemkeys,3)&&cJSON_IsString(id)&&logical_id(id->valuestring)&&integer(p,UINT32_MAX)&&integer(t,253402300799.0)&&seen_count<CA_MAX_ACHIEVEMENTS+UNKNOWN_CAP;
    if(!ok)break;for(size_t k=0;k<seen_count;k++)if(!strcmp(seen[k],id->valuestring)){ok=false;break;}if(!ok)break;
    strcpy(seen[seen_count++],id->valuestring);size_t index=definition_index(id->valuestring);
    if(index<a.engine.count)ok=ca_restore(&a.engine,id->valuestring,(uint32_t)p->valuedouble,(uint64_t)t->valuedouble);
    else if(a.unknown_count==UNKNOWN_CAP)ok=false;
    else{LagoonUnknownAchievement*u=a.unknown+a.unknown_count++;strcpy(u->id,id->valuestring);u->progress=(uint32_t)p->valuedouble;u->when=(uint64_t)t->valuedouble;}
  }
  cJSON_Delete(r);return ok;
}
static bool store(void*context,const ca_state*states,size_t count){
#ifdef LAGOON_ACHIEVEMENTS_TEST
  extern void LagoonAchievements_TestBeforeCommit(void);
  LagoonAchievements_TestBeforeCommit();
#endif
  (void)context;cJSON*r=cJSON_CreateObject(),*items=cJSON_CreateArray();if(!r||!items){cJSON_Delete(r);cJSON_Delete(items);return false;}
  cJSON_AddNumberToObject(r,"schemaVersion",1);cJSON_AddStringToObject(r,"gameId",LAGOON_GAME_ID);cJSON_AddStringToObject(r,"editionId",LAGOON_EDITION_ID);cJSON_AddNullToObject(r,"seedId");cJSON_AddNumberToObject(r,"catalogVersion",LAGOON_CATALOG_VERSION);if(!cJSON_AddItemToObject(r,"achievements",items)){cJSON_Delete(items);cJSON_Delete(r);return false;}
  for(size_t i=0;i<count+a.unknown_count;i++){
    cJSON*x=cJSON_CreateObject();if(!x){cJSON_Delete(r);return false;}if(!cJSON_AddItemToArray(items,x)){cJSON_Delete(x);cJSON_Delete(r);return false;}
    cJSON_AddStringToObject(x,"id",i<count?lagoon_definitions[i].id:a.unknown[i-count].id);
    cJSON_AddNumberToObject(x,"progress",i<count?states[i].progress:a.unknown[i-count].progress);
    cJSON_AddNumberToObject(x,"unlockedAt",(double)(i<count?states[i].unlocked_at:a.unknown[i-count].when));
  }
  const char*rootkeys[]={"schemaVersion","gameId","editionId","seedId","catalogVersion","achievements"};const char*itemkeys[]={"id","progress","unlockedAt"};
  bool complete=object_keys(r,rootkeys,6)&&(size_t)cJSON_GetArraySize(items)==count+a.unknown_count;for(cJSON*i=items->child;complete&&i;i=i->next)complete=object_keys(i,itemkeys,3);
  char*data=complete?cJSON_PrintUnformatted(r):NULL;cJSON_Delete(r);if(!data)return false;
  char temp[PATH_CAP];bool ok=snprintf(temp,sizeof temp,"%s.tmp",a.profile)<(int)sizeof temp&&regular_target(a.profile)&&regular_target(temp);FILE*f=NULL;
#ifdef _WIN32
  if(ok)f=open_file(temp,L"wb");
#else
  if(ok)f=open_file(temp,"wb");
#endif
  if(f){size_t length=strlen(data);ok=fwrite(data,1,length,f)==length&&fflush(f)==0;
#ifdef _WIN32
    if(ok)ok=_commit(_fileno(f))==0;
#else
    if(ok)ok=fsync(fileno(f))==0;
#endif
    if(fclose(f))ok=false;
    if(ok){
#ifdef _WIN32
      wchar_t from[PATH_CAP],to[PATH_CAP];ok=wide(temp,from)&&wide(a.profile,to)&&MoveFileExW(from,to,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
#else
      ok=rename(temp,a.profile)==0;
#endif
    }
  }else ok=false;
  cJSON_free(data);return ok;
}
static void notify(void*context,const ca_definition*d,const ca_state*s){(void)context;(void)s;if(a.new_count<CA_MAX_ACHIEVEMENTS)a.newly_unlocked[a.new_count++]=(size_t)(d-lagoon_definitions);}
static int worker(void*unused){
  (void)unused;for(;;){
    SDL_LockMutex(a.mutex);while(!a.stop&&!a.pending)SDL_WaitCondition(a.condition,a.mutex);
    if(a.stop&&!a.pending){SDL_UnlockMutex(a.mutex);break;}
    unsigned slot=0;while(slot<EVENT_CAP&&!a.events[slot].name)slot++;
    Event event=a.events[slot];a.events[slot].name=NULL;a.pending--;a.busy=true;bool tracking=a.tracking;SDL_UnlockMutex(a.mutex);
    a.new_count=0;bool ok=!tracking||ca_event(&a.engine,event.name,event.amount,event.when);
    SDL_LockMutex(a.mutex);
    if(ok){memcpy(a.published,a.engine.states,sizeof a.published);for(size_t i=0;i<a.new_count&&a.notice_count<CA_MAX_ACHIEVEMENTS;i++)a.notices[(a.notice_read+a.notice_count++)%CA_MAX_ACHIEVEMENTS]=a.newly_unlocked[i];}
    else{a.tracking=false;a.status=IO_ERROR;memset(a.events,0,sizeof a.events);a.pending=0;}
    a.busy=false;SDL_BroadcastCondition(a.condition);SDL_UnlockMutex(a.mutex);
  }return 0;
}
static void queue_event(const char*name,uint32_t amount){
  time_t now=time(NULL);if(now<=0||!a.mutex)return;SDL_LockMutex(a.mutex);
  if(a.tracking){unsigned slot=0;for(;slot<EVENT_CAP;slot++)if(a.events[slot].name&&!strcmp(a.events[slot].name,name))break;
    if(slot<EVENT_CAP){if(amount>a.events[slot].amount)a.events[slot].amount=amount;}
    else{for(slot=0;slot<EVENT_CAP&&a.events[slot].name;slot++){/* Find a free bounded queue slot. */}if(slot<EVENT_CAP){a.events[slot]=(Event){name,amount,(uint64_t)now};a.pending++;}else{a.tracking=false;a.status=WORKER_ERROR;}}
    SDL_SignalCondition(a.condition);
  }SDL_UnlockMutex(a.mutex);
}
bool LagoonAchievements_Init(const char*root,bool diagnostic){
  LagoonAchievements_Shutdown();memset(&a,0,sizeof a);
#ifdef _WIN32
  a.lock=INVALID_HANDLE_VALUE;
#else
  a.lock=-1;
#endif
  a.mutex=SDL_CreateMutex();a.condition=SDL_CreateCondition();
  if(!a.mutex||!a.condition){a.status=WORKER_ERROR;return false;}
  if(!ca_init(&a.engine,lagoon_definitions,LAGOON_ACHIEVEMENT_COUNT,store,notify,NULL)){a.status=WORKER_ERROR;return false;}
  if(diagnostic){a.status=DIAGNOSTIC;return true;}
  char directory[PATH_CAP];if(!root||snprintf(directory,sizeof directory,"%s/Achievements/%s/%s/unseeded",root,LAGOON_GAME_ID,LAGOON_EDITION_ID)>=(int)sizeof directory||!directories(directory)||snprintf(a.profile,sizeof a.profile,"%s/progress.json",directory)>=(int)sizeof a.profile||!regular_target(a.profile)){a.status=IO_ERROR;return false;}
  if(!acquire_lock()){a.status=LOCKED;return false;}
  errno=0;
#ifdef _WIN32
  FILE*f=open_file(a.profile,L"rb");
#else
  FILE*f=open_file(a.profile,"rb");
#endif
  bool ok=f?restore(f):errno==ENOENT;if(f)fclose(f);
  if(!ok){a.status=CORRUPT;release_lock();return false;}
  if(!f&&!store(NULL,a.engine.states,a.engine.count)){a.status=IO_ERROR;release_lock();return false;}
  memcpy(a.published,a.engine.states,sizeof a.published);a.tracking=true;a.status=READY;
  a.thread=SDL_CreateThread(worker,"Lagoon achievements",NULL);if(!a.thread){a.tracking=false;a.status=WORKER_ERROR;release_lock();return false;}return true;
}
void LagoonAchievements_Shutdown(void){
  if(a.thread){SDL_LockMutex(a.mutex);a.stop=true;SDL_SignalCondition(a.condition);SDL_UnlockMutex(a.mutex);SDL_WaitThread(a.thread,NULL);a.thread=NULL;}
  /* Static initial zero is not an owned file handle. */
  if(a.profile[0])release_lock();if(a.condition)SDL_DestroyCondition(a.condition);if(a.mutex)SDL_DestroyMutex(a.mutex);a.condition=NULL;a.mutex=NULL;a.tracking=false;a.status=OFF;
}
void LagoonAchievements_OnLoaded(uint64_t generation){a.baseline=false;a.observed_ready=false;a.generation=generation;memset(a.boss_armed,0,sizeof a.boss_armed);}
static void arm_boss(const uint8_t*r,unsigned room,const Snapshot*s){
  if(room<0x40||room>0x46||bit(s->story,room-0x40))return;
  for(unsigned slot=0x540;slot<0xd40;slot+=64)if((r[slot]&0xc0)==0xc0&&r[slot+0x18]==3&&word(r+slot+0x20)){a.boss_armed[room-0x40]=true;break;}
}
void LagoonAchievements_Observe(const uint8_t*r,size_t size,uint64_t generation,bool ready){
  if(!r||size<0x1010||!ready||(r[0x500]&0xc0)!=0xc0||!word(r+0x520)||r[0x100e]||word(r+0x48e)>=75){a.observed_ready=false;return;}
  bool tracking=false;if(a.mutex){SDL_LockMutex(a.mutex);tracking=a.tracking;SDL_UnlockMutex(a.mutex);}if(!tracking)return;
  Snapshot s;memcpy(s.chests,r+0x4c0,16);memcpy(s.items,r+0x4d0,16);memcpy(s.story,r+0x4e0,16);s.level=r[0x52c];s.gold=word(r+0x528);s.room=word(r+0x48e);
  if(!s.level||s.level>50){a.observed_ready=false;return;}
  bool reset=!a.baseline||generation!=a.generation||s.level<a.previous.level;
  if(!reset)for(unsigned i=0;i<16;i++)if((a.previous.chests[i]&~s.chests[i])||(i<1&&(a.previous.story[i]&0xfe&~s.story[i])))reset=true;
  if(reset){a.baseline=true;a.generation=generation;a.previous=s;memset(a.boss_armed,0,sizeof a.boss_armed);arm_boss(r,s.room,&s);a.observed_ready=true;return;}
  arm_boss(r,s.room,&s);
  static const char*boss_events[]={"boss.0","boss.1","boss.2","boss.3","boss.4","boss.5","boss.6"};
  for(unsigned i=0;i<7;i++)if(!bit(a.previous.story,i)&&bit(s.story,i)&&a.boss_armed[i]){queue_event(boss_events[i],1);a.boss_armed[i]=false;}
  if(s.level>a.previous.level)queue_event("player.level",s.level);
  if(s.gold>a.previous.gold)queue_event("player.gold",s.gold);
  if(chest_count(&s)>chest_count(&a.previous))queue_event("treasure.opened",chest_count(&s));
  static const char*tablet_events[]={"tablet.0","tablet.1","tablet.2"};
  for(unsigned i=0;i<3;i++)if(!bit(a.previous.items,0x36+i)&&bit(s.items,0x36+i))queue_event(tablet_events[i],1);
  if(bit(s.items,0x36)&&bit(s.items,0x37)&&bit(s.items,0x38)&&!(bit(a.previous.items,0x36)&&bit(a.previous.items,0x37)&&bit(a.previous.items,0x38)))queue_event("tablet.all",1);
  if(!bit(a.previous.story,0x11)&&bit(s.story,0x11))queue_event("quest.tablet-spell",1);
  if(!bit(a.previous.story,0x10)&&bit(s.story,0x10))queue_event("quest.giles-home",1);
  a.previous=s;a.observed_ready=true;
}
void LagoonAchievements_SecretVisit(uint64_t generation){if(a.baseline&&a.observed_ready&&a.generation==generation)queue_event("secret.wrong-game",1);}
void LagoonAchievements_CompletedEnding(uint64_t generation){if(a.baseline&&a.generation==generation)queue_event("story.finished",1);}
size_t LagoonAchievements_Count(void){return LAGOON_ACHIEVEMENT_COUNT;}
bool LagoonAchievements_GetRow(size_t i,LagoonAchievementRow*out){
  if(!out||i>=LAGOON_ACHIEVEMENT_COUNT)return false;ca_state s={0};if(a.mutex){SDL_LockMutex(a.mutex);s=a.published[i];SDL_UnlockMutex(a.mutex);}
  const ca_definition*d=lagoon_definitions+i;*out=(LagoonAchievementRow){d->id,d->title,d->description,s.progress,d->target,s.unlocked_at,!strncmp(d->id,"secret.",7)};return true;
}
size_t LagoonAchievements_Unlocked(void){size_t n=0;if(a.mutex){SDL_LockMutex(a.mutex);for(size_t i=0;i<LAGOON_ACHIEVEMENT_COUNT;i++)n+=a.published[i].unlocked_at!=0;SDL_UnlockMutex(a.mutex);}return n;}
const char*LagoonAchievements_Status(void){enum Status s=OFF;if(a.mutex){SDL_LockMutex(a.mutex);s=a.status;SDL_UnlockMutex(a.mutex);}static const char*text[]={"Achievements unavailable","Tracking locally","Tracking disabled for this diagnostic session","Progress could not be saved; gameplay continues","Progress unreadable or unsupported; file preserved","Another game copy owns achievement progress","Achievement worker unavailable; gameplay continues"};return text[s<=WORKER_ERROR?s:OFF];}
const char*LagoonAchievements_ProfilePath(void){return a.profile;}
bool LagoonAchievements_PopNotice(size_t*index){if(!index||!a.mutex)return false;SDL_LockMutex(a.mutex);bool found=a.notice_count!=0;if(found){*index=a.notices[a.notice_read];a.notice_read=(a.notice_read+1)%CA_MAX_ACHIEVEMENTS;a.notice_count--;}SDL_UnlockMutex(a.mutex);return found;}
bool LagoonAchievements_WaitIdle(unsigned timeout){if(!a.mutex)return true;uint64_t start=SDL_GetTicks();SDL_LockMutex(a.mutex);while(a.pending||a.busy){uint64_t elapsed=SDL_GetTicks()-start;if(elapsed>=timeout){SDL_UnlockMutex(a.mutex);return false;}SDL_WaitConditionTimeout(a.condition,a.mutex,(Sint32)(timeout-elapsed));}SDL_UnlockMutex(a.mutex);return true;}
