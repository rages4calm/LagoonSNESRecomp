/* Host-only focused regression. All profiles live below the supplied scratch root. */
#include "lagoon_achievements.h"
#include "cJSON.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h>

#define CHECK(x) do { if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static SDL_Semaphore *entered,*release_commit;
static SDL_AtomicInt block_commit;
void LagoonAchievements_TestBeforeCommit(void) {
  if(SDL_CompareAndSwapAtomicInt(&block_commit,1,0)) {
    SDL_SignalSemaphore(entered);
    SDL_WaitSemaphore(release_commit);
  }
}
static uint8_t ram[0x20000],before[sizeof ram];
static uint64_t generation=1;
static void w(unsigned p,unsigned value){ram[p]=(uint8_t)value;ram[p+1]=(uint8_t)(value>>8);}
static void flag(unsigned base,unsigned index){ram[base+index/8]|=(uint8_t)(0x80u>>(index&7));}
static void observe(void){memcpy(before,ram,sizeof ram);LagoonAchievements_Observe(ram,sizeof ram,generation,true);CHECK(!memcmp(before,ram,sizeof ram));}
static void baseline(unsigned level,unsigned room){memset(ram,0,sizeof ram);ram[0x500]=0xc0;w(0x520,18);ram[0x52c]=(uint8_t)level;w(0x48e,room);LagoonAchievements_OnLoaded(++generation);observe();}
static void idle(void){CHECK(LagoonAchievements_WaitIdle(5000));}
static size_t notices(void){size_t i,n=0;while(LagoonAchievements_PopNotice(&i)){CHECK(i<LagoonAchievements_Count());n++;}return n;}
static LagoonAchievementRow row(const char*id){LagoonAchievementRow r={0};for(size_t i=0;i<LagoonAchievements_Count();i++){CHECK(LagoonAchievements_GetRow(i,&r));if(!strcmp(r.id,id))return r;}CHECK(0);return r;}
static void path(char*out,size_t cap,const char*root,const char*part){CHECK(snprintf(out,cap,"%s/%s",root,part)<(int)cap);}
static char *read_all(const char*p){FILE*f=fopen(p,"rb");CHECK(f);CHECK(!fseek(f,0,SEEK_END));long n=ftell(f);CHECK(n>=0);rewind(f);char*s=malloc((size_t)n+1);CHECK(s);CHECK(fread(s,1,(size_t)n,f)==(size_t)n);s[n]=0;fclose(f);return s;}
static void write_all(const char*p,const char*s){FILE*f=fopen(p,"wb");CHECK(f);CHECK(fwrite(s,1,strlen(s),f)==strlen(s));CHECK(!fclose(f));}
static void assert_unchanged(const char*p,const char*s){char*after=read_all(p);CHECK(!strcmp(after,s));free(after);}
static void reject_json(const char*root,const char*profile,const char*text){write_all(profile,text);CHECK(!LagoonAchievements_Init(root,false));CHECK(strstr(LagoonAchievements_Status(),"unreadable"));assert_unchanged(profile,text);LagoonAchievements_Shutdown();}
int main(int argc,char**argv){
  CHECK(argc>=3);
  if(!strcmp(argv[1],"locked")){CHECK(!LagoonAchievements_Init(argv[2],false));CHECK(strstr(LagoonAchievements_Status(),"Another game copy"));LagoonAchievements_Shutdown();return 0;}
  CHECK(!strcmp(argv[1],"run"));const char*root=argv[2];
  entered=SDL_CreateSemaphore(0);release_commit=SDL_CreateSemaphore(0);CHECK(entered&&release_commit);
  CHECK(!LagoonAchievements_Init("relative-root",false));LagoonAchievements_Shutdown();
  CHECK(LagoonAchievements_Init(root,false));CHECK(LagoonAchievements_Count()==26);CHECK(!LagoonAchievements_Unlocked());
  char profile[4096];strcpy(profile,LagoonAchievements_ProfilePath());
  CHECK(strstr(profile,"/Achievements/lagoon/native-qol/unseeded/progress.json"));
  char*original=read_all(profile);CHECK(strstr(original,"\"schemaVersion\":1"));
  CHECK(_spawnl(_P_WAIT,argv[0],argv[0],"locked",root,NULL)==0);assert_unchanged(profile,original);free(original);
  baseline(1,2);idle();CHECK(!notices());CHECK(!LagoonAchievements_Unlocked());

  /* Block actual commit, then prove the frame/UI calls never acquire its I/O lock. */
  SDL_SetAtomicInt(&block_commit,1);ram[0x52c]=2;observe();CHECK(SDL_WaitSemaphoreTimeout(entered,5000));
  for(unsigned i=0;i<100;i++){observe();CHECK(!row("level.2").unlocked_at);CHECK(!strcmp(LagoonAchievements_Status(),"Tracking locally"));CHECK(!notices());}
  CHECK(!LagoonAchievements_WaitIdle(1));SDL_SignalSemaphore(release_commit);idle();
  CHECK(row("level.2").unlocked_at);CHECK(notices()==1);uint64_t earned=row("level.2").unlocked_at;
  observe();idle();CHECK(!notices());
  LagoonAchievements_Shutdown();CHECK(LagoonAchievements_Init(root,false));CHECK(row("level.2").unlocked_at==earned);CHECK(!notices());

  /* Loading a high-level save is silent; only the next committed level earns milestones. */
  baseline(20,2);idle();CHECK(!notices());CHECK(!row("level.20").unlocked_at);
  ram[0x52c]=21;observe();idle();CHECK(notices()==3);CHECK(row("level.20").unlocked_at);CHECK(!row("level.30").unlocked_at);
  baseline(1,0x40);flag(0x4e0,0);observe();idle();CHECK(!row("boss.samson").unlocked_at);CHECK(!notices());
  baseline(1,0x40);ram[0x540]=0xc0;ram[0x558]=3;w(0x560,8);observe();flag(0x4e0,0);observe();idle();CHECK(row("boss.samson").unlocked_at);CHECK(notices()==1);
  baseline(1,0x41);ram[0x540]=0xc0;ram[0x558]=1;w(0x560,8);observe();flag(0x4e0,1);observe();idle();CHECK(!row("boss.natela").unlocked_at);CHECK(!notices());
  baseline(1,0x41);ram[0x540]=0xc0;ram[0x558]=3;w(0x560,8);observe();flag(0x4e0,1);observe();idle();CHECK(row("boss.natela").unlocked_at);CHECK(notices()==1);
  const char*remaining_bosses[]={"boss.eardon","boss.duma","boss.thimale","boss.ella","boss.battler"};
  for(unsigned i=2;i<7;i++){baseline(1,0x40+i);ram[0x540]=0xc0;ram[0x558]=3;w(0x560,8);observe();flag(0x4e0,i);observe();idle();CHECK(row(remaining_bosses[i-2]).unlocked_at);CHECK(notices()==1);}

  baseline(1,2);for(unsigned i=0;i<3;i++){flag(0x4d0,0x36+i);observe();}idle();CHECK(notices()==4);CHECK(row("tablet.all").unlocked_at);
  memset(ram+0x4d0,0,16);flag(0x4e0,0x11);flag(0x4e0,0x10);observe();idle();CHECK(notices()==2);CHECK(row("quest.tablet-spell").unlocked_at);CHECK(row("quest.giles-home").unlocked_at);
  flag(0x4c0,0);observe();idle();CHECK(notices()==1);CHECK(row("treasure.1").unlocked_at);
  baseline(1,2);flag(0x4c0,0);observe();idle();CHECK(!notices());CHECK(row("treasure.10").progress==1);
  baseline(1,2);for(unsigned i=0;i<10;i++)flag(0x4c0,i);observe();idle();CHECK(notices()==1);CHECK(row("treasure.10").unlocked_at);
  w(0x528,10000);observe();idle();CHECK(notices()==3);CHECK(row("gold.10000").unlocked_at);

  LagoonAchievements_OnLoaded(++generation);LagoonAchievements_SecretVisit(generation);LagoonAchievements_CompletedEnding(generation);idle();CHECK(!notices());
  observe();LagoonAchievements_SecretVisit(generation-1);idle();CHECK(!notices());
  LagoonAchievements_SecretVisit(generation);idle();CHECK(notices()==1);CHECK(row("secret.wrong-game").secret);CHECK(row("secret.wrong-game").unlocked_at);
  LagoonAchievements_SecretVisit(generation);idle();CHECK(!notices());
  LagoonAchievements_CompletedEnding(generation-1);idle();CHECK(!notices());CHECK(!row("story.finished").unlocked_at);
  LagoonAchievements_Observe(ram,sizeof ram,generation,false);LagoonAchievements_CompletedEnding(generation);idle();CHECK(notices()==1);CHECK(row("story.finished").unlocked_at);
  LagoonAchievements_CompletedEnding(generation);idle();CHECK(!notices());

  char*valid=read_all(profile);LagoonAchievements_Shutdown();
  /* Unknown IDs round-trip without becoming rule triggers. */
  cJSON*doc=cJSON_Parse(valid);CHECK(doc);cJSON*item=cJSON_CreateObject();cJSON_AddStringToObject(item,"id","future.example");cJSON_AddNumberToObject(item,"progress",7);cJSON_AddNumberToObject(item,"unlockedAt",0);cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(doc,"achievements"),item);
  char*modified=cJSON_PrintUnformatted(doc);CHECK(modified);write_all(profile,modified);cJSON_free(modified);cJSON_Delete(doc);
  CHECK(LagoonAchievements_Init(root,false));CHECK(!notices());baseline(21,2);ram[0x52c]=30;observe();idle();CHECK(notices()==1);CHECK(row("level.30").unlocked_at);char*unknown=read_all(profile);CHECK(strstr(unknown,"future.example"));free(unknown);

  /* A failed atomic commit cannot expose an unlock or modify the previous profile. */
  char*before_failure=read_all(profile);char temp[4096];CHECK(snprintf(temp,sizeof temp,"%s.tmp",profile)<(int)sizeof temp);CHECK(CreateDirectoryA(temp,NULL));
  for(unsigned i=0;i<25;i++)flag(0x4c0,i);observe();idle();CHECK(!row("treasure.25").unlocked_at);CHECK(!notices());CHECK(strstr(LagoonAchievements_Status(),"could not be saved"));assert_unchanged(profile,before_failure);free(before_failure);LagoonAchievements_Shutdown();
  /* Invalid documents are preserved byte-for-byte, never overwritten by a reset. */
  reject_json(root,profile,"{broken");
  reject_json(root,profile,"{\"schemaVersion\":1,\"gameId\":\"other\",\"editionId\":\"native-qol\",\"seedId\":null,\"catalogVersion\":1,\"achievements\":[]}");
  reject_json(root,profile,"{\"schemaVersion\":1,\"gameId\":\"lagoon\",\"editionId\":\"native-qol\",\"seedId\":null,\"catalogVersion\":99,\"achievements\":[]}");
  reject_json(root,profile,"{\"schemaVersion\":1,\"gameId\":\"lagoon\",\"gameId\":\"lagoon\",\"editionId\":\"native-qol\",\"seedId\":null,\"catalogVersion\":1,\"achievements\":[]}");
  reject_json(root,profile,"{\"schemaVersion\":1,\"gameId\":\"lagoon\",\"editionId\":\"native-qol\",\"seedId\":null,\"catalogVersion\":1,\"achievements\":[{\"id\":\"level.2\",\"progress\":3,\"unlockedAt\":1}]}");
  reject_json(root,profile,"{\"schemaVersion\":1,\"gameId\":\"lagoon\",\"editionId\":\"native-qol\",\"seedId\":null,\"catalogVersion\":1,\"achievements\":[{\"id\":\"level.2\",\"progress\":0,\"unlockedAt\":0},{\"id\":\"level.2\",\"progress\":0,\"unlockedAt\":0}]}");
  write_all(profile,valid);free(valid);
  char diagnostic[4096];path(diagnostic,sizeof diagnostic,root,"never-create-diagnostic");CHECK(LagoonAchievements_Init(diagnostic,true));baseline(1,2);ram[0x52c]=30;observe();LagoonAchievements_SecretVisit(generation);idle();CHECK(!notices());CHECK(!LagoonAchievements_Unlocked());CHECK(GetFileAttributesA(diagnostic)==INVALID_FILE_ATTRIBUTES);LagoonAchievements_Shutdown();
  SDL_DestroySemaphore(entered);SDL_DestroySemaphore(release_commit);
  puts("PASS: host observer read-only; silent baselines/restore; qualified outcomes; exactly-once; worker isolation; schema, lock, atomic failure and diagnostic preservation.");return 0;
}
