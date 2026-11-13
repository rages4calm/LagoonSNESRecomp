/* A frozen host-settings panel. No guest RAM, quest, save-state or emulation
 * timing is changed here. Music trim persists by editing one owned INI key;
 * all other bytes remain exact, with a concurrent-edit check before replace. */
#include "lagoon_settings.h"
#include "lagoon_soundtrack.h"
#include "lagoon_ui.h"
#include "lagoon_journal.h"
#include "lagoon_item_guide.h"
#include "snes_overlay_draw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
extern int snesrecomp_desktop_get_volume(void);
extern void snesrecomp_desktop_set_volume(int volume);
enum {WIDTH=640,HEIGHT=360};
enum {TAB_SOUND,TAB_JOURNAL,TAB_ITEMS,TAB_DISPLAY,TAB_TRAVEL,TAB_COUNT,CONTEXT_BYTES=8192};
static uint32_t panel[WIDTH*HEIGHT],prior_input,repeat_at;
static unsigned opened,selected,primed,repeat_armed,achievements_requested;
static unsigned active_tab;
static size_t item_selected,context_size;
static uint8_t context[CONTEXT_BYTES];
static char config_file[1024];
static int save_failed;
static unsigned sword_range_enabled;
static int range_save_failed;
static char travel_status[256];
static unsigned travel_available,travel_confirming,return_requested;
static unsigned quit_confirming,quit_selected,quit_requested;
static unsigned display_selected,view_zoom=100,zoom_requested;
static int zoom_save_failed;
#ifdef _WIN32
static wchar_t *wide(const char *text){
 int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text,-1,NULL,0);if(!count)return NULL;
 wchar_t *out=malloc((size_t)count*sizeof *out);if(out&&!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text,-1,out,count)){free(out);return NULL;}return out;
}
static FILE *file_open(const char *path,const char *mode){
 wchar_t *p=wide(path),*m=wide(mode);FILE *f=p&&m?_wfopen(p,m):NULL;free(p);free(m);return f;
}
static int file_replace(const char *from,const char *to){
 wchar_t *a=wide(from),*b=wide(to);int ok=a&&b&&MoveFileExW(a,b,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);free(a);free(b);return ok;
}
static void file_remove(const char *path){wchar_t *p=wide(path);if(p){_wremove(p);free(p);}}
#else
#define file_open fopen
static int file_replace(const char *from,const char *to){return rename(from,to)==0;}
static void file_remove(const char *path){remove(path);}
#endif
static char *file_read(const char *path,size_t *size){
 if(!path||!*path)return NULL;
 FILE *f=file_open(path,"rb");if(!f)return NULL;
 if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);
 if(n<0||n>262144||fseek(f,0,SEEK_SET)){fclose(f);return NULL;}
 char *data=malloc((size_t)n+1);if(!data){fclose(f);return NULL;}
 if(fread(data,1,(size_t)n,f)!=(size_t)n){free(data);fclose(f);return NULL;}fclose(f);data[n]=0;*size=(size_t)n;return data;
}
static int same_name(const char *start,const char *end,const char *name){
 while(start<end&&isspace((unsigned char)*start))start++;
 while(end>start&&isspace((unsigned char)end[-1]))end--;
 size_t n=strlen(name);if((size_t)(end-start)!=n)return 0;
 for(size_t i=0;i<n;i++)if(tolower((unsigned char)start[i])!=tolower((unsigned char)name[i]))return 0;return 1;
}
/* Locate a single owned key. Duplicates, NULs and mixed newlines cannot be
 * safely normalized during play, so persistence refuses those documents. */
static int locate(const char *text,size_t bytes,const char *key,unsigned default_value,unsigned limit,size_t *begin,size_t *end,size_t *insert,unsigned *value,int *has_section,const char **newline){
 unsigned section=0,found=0,crlf=0,lf=0;*begin=*end=*insert=bytes;*has_section=0;*value=default_value;
 if(memchr(text,0,bytes))return 0;
 for(size_t i=0;i<bytes;i++)if(text[i]=='\n'){if(i&&text[i-1]=='\r')crlf++;else lf++;}else if(text[i]=='\r'&&(i+1==bytes||text[i+1]!='\n'))return 0;
 if(crlf&&lf)return 0;*newline=crlf?"\r\n":"\n";
 size_t at=bytes>=3&&(unsigned char)text[0]==0xef&&(unsigned char)text[1]==0xbb&&(unsigned char)text[2]==0xbf?3:0;
 while(at<bytes){
  size_t stop=at;while(stop<bytes&&text[stop]!='\n')stop++;size_t next=stop<bytes?stop+1:stop;
  const char *a=text+at,*b=text+stop;if(b>a&&b[-1]=='\r')b--;
  while(a<b&&isspace((unsigned char)*a))a++;
  if(a<b&&*a=='['){
   if(section)*insert=at;
   section=b>a+1&&b[-1]==']'&&same_name(a+1,b-1,"Lagoon");
   if(section){*has_section=1;*insert=next;}
  }else if(section){
   *insert=next;
   if(a<b&&*a!='#'&&*a!=';'){
    const char *equal=memchr(a,'=',(size_t)(b-a));
    if(equal&&same_name(a,equal,key)){
     if(found++)return 0;*begin=at;*end=next;
     const char *v=equal+1;while(v<b&&isspace((unsigned char)*v))v++;
     char *tail;if(v>=b)return 0;unsigned long number=strtoul(v,&tail,10);if(tail==v||tail>b||number>limit)return 0;
     while(tail<b&&isspace((unsigned char)*tail))tail++;
     if(tail<b&&*tail!='#'&&*tail!=';')return 0;*value=(unsigned)number;
    }
   }
  }
  at=next;
 }
 return 1;
}
static int commit_setting_bytes(const char *before,size_t bytes,const char *after,size_t total){
 char temporary[1100];
#ifdef _WIN32
 unsigned long pid=GetCurrentProcessId();
#else
 unsigned long pid=(unsigned long)getpid();
#endif
 int valid=snprintf(temporary,sizeof temporary,"%s.lagoon-settings-%lu.tmp",config_file,pid)<(int)sizeof temporary;
 FILE *f=valid?file_open(temporary,"wbx"):NULL;int wrote=f&&fwrite(after,1,total,f)==total;
 if(f&&fclose(f))wrote=0;
 if(wrote){size_t now_bytes=0;char *now=file_read(config_file,&now_bytes);wrote=now&&now_bytes==bytes&&!memcmp(now,before,bytes);free(now);}
 if(wrote)wrote=file_replace(temporary,config_file);
 if(f)file_remove(temporary);return wrote;
}
static int persist_setting(const char *key,unsigned value,unsigned default_value,unsigned limit){
 size_t bytes=0;char *before=file_read(config_file,&bytes);if(!before)return 0;
 size_t begin,end,insert;unsigned old;int section;const char *nl;
 if(!locate(before,bytes,key,default_value,limit,&begin,&end,&insert,&old,&section,&nl)){free(before);return 0;}
 char line[96];snprintf(line,sizeof line,"%s = %u%s",key,value,nl);
 const char *prefix="";char heading[32];size_t replace_begin=begin,replace_end=end;
 if(begin==bytes){
  replace_begin=replace_end=section?insert:bytes;
  snprintf(heading,sizeof heading,"%s%s%s",replace_begin&&before[replace_begin-1]!='\n'?nl:"",section?"":"[Lagoon]",section?"":nl);prefix=heading;
 }
 size_t plen=strlen(prefix),llen=strlen(line),total=replace_begin+plen+llen+bytes-replace_end;
 char *after=malloc(total);if(!after){free(before);return 0;}
 memcpy(after,before,replace_begin);memcpy(after+replace_begin,prefix,plen);memcpy(after+replace_begin+plen,line,llen);memcpy(after+replace_begin+plen+llen,before+replace_end,bytes-replace_end);
 int wrote=commit_setting_bytes(before,bytes,after,total);free(after);free(before);return wrote;
}
/* Zoom has fractional values. Locate only its exact existing token so inline
 * comments, indentation and all unrelated bytes survive the edit unchanged. */
static int locate_zoom(const char *text,size_t bytes,size_t *begin,size_t *end,size_t *insert,int *has_section,const char **newline){
 unsigned section=0,found=0,crlf=0,lf=0;*begin=*end=*insert=bytes;*has_section=0;
 if(memchr(text,0,bytes))return 0;
 for(size_t i=0;i<bytes;i++)if(text[i]=='\n'){if(i&&text[i-1]=='\r')crlf++;else lf++;}else if(text[i]=='\r'&&(i+1==bytes||text[i+1]!='\n'))return 0;
 if(crlf&&lf)return 0;*newline=crlf?"\r\n":"\n";
 size_t at=bytes>=3&&(unsigned char)text[0]==0xef&&(unsigned char)text[1]==0xbb&&(unsigned char)text[2]==0xbf?3:0;
 while(at<bytes){
  size_t stop=at;while(stop<bytes&&text[stop]!='\n')stop++;size_t next=stop<bytes?stop+1:stop;
  const char *a=text+at,*b=text+stop;if(b>a&&b[-1]=='\r')b--;
  while(a<b&&isspace((unsigned char)*a))a++;
  if(a<b&&*a=='['){
   if(section)*insert=at;
   const char *close=memchr(a,']',(size_t)(b-a));if(!close)return 0;
   const char *tail=close+1;while(tail<b&&isspace((unsigned char)*tail))tail++;
   if(tail<b&&*tail!='#'&&*tail!=';')return 0;
   section=same_name(a+1,close,"Lagoon");if(section){*has_section=1;*insert=next;}
  }else if(section){
   *insert=next;
   if(a<b&&*a!='#'&&*a!=';'){
    const char *equal=memchr(a,'=',(size_t)(b-a));
    if(equal&&same_name(a,equal,"ViewExtent")){
     if(found++)return 0;
     const char *value=equal+1;while(value<b&&isspace((unsigned char)*value))value++;
     const char *tail=value;while(tail<b&&!isspace((unsigned char)*tail)&&*tail!='#'&&*tail!=';')tail++;
     size_t n=(size_t)(tail-value);
     if(!((n==1&&!memcmp(value,"1",1))||(n==4&&!memcmp(value,"1.25",4))||(n==3&&!memcmp(value,"1.5",3))))return 0;
     const char *rest=tail;while(rest<b&&isspace((unsigned char)*rest))rest++;
     if(rest<b&&*rest!='#'&&*rest!=';')return 0;
     *begin=(size_t)(value-text);*end=(size_t)(tail-text);
    }
   }
  }
  at=next;
 }
 return 1;
}
int LagoonSettings_SaveZoom(unsigned percent){
 const char *value=percent==100?"1":percent==125?"1.25":percent==150?"1.5":NULL;
 if(!value){zoom_save_failed=1;return 0;}
 size_t bytes=0;char *before=file_read(config_file,&bytes);if(!before){zoom_save_failed=1;return 0;}
 size_t begin,end,insert;int section;const char *nl;
 if(!locate_zoom(before,bytes,&begin,&end,&insert,&section,&nl)){free(before);zoom_save_failed=1;return 0;}
 const char *edit=value;char addition[128];size_t edit_bytes=strlen(value);
 if(begin==bytes){
  begin=end=section?insert:bytes;
  int n=snprintf(addition,sizeof addition,"%s%s%sViewExtent = %s%s",begin&&before[begin-1]!='\n'?nl:"",section?"":"[Lagoon]",section?"":nl,value,nl);
  if(n<0||(size_t)n>=sizeof addition){free(before);zoom_save_failed=1;return 0;}
  edit=addition;edit_bytes=(size_t)n;
 }else if(end-begin==edit_bytes&&!memcmp(before+begin,value,edit_bytes)){free(before);zoom_save_failed=0;return 1;}
 size_t total=begin+edit_bytes+bytes-end;char *after=malloc(total);
 if(!after){free(before);zoom_save_failed=1;return 0;}
 memcpy(after,before,begin);memcpy(after+begin,edit,edit_bytes);memcpy(after+begin+edit_bytes,before+end,bytes-end);
 int wrote=commit_setting_bytes(before,bytes,after,total);free(after);free(before);zoom_save_failed=!wrote;return wrote;
}

void LagoonSettings_Configure(const char *path){
 opened=selected=primed=repeat_armed=achievements_requested=0;prior_input=repeat_at=0;save_failed=0;config_file[0]=0;
 sword_range_enabled=0;range_save_failed=zoom_save_failed=0;
 travel_status[0]=0;travel_available=travel_confirming=return_requested=0;
 quit_confirming=quit_selected=quit_requested=display_selected=zoom_requested=0;view_zoom=100;
 active_tab=TAB_SOUND;item_selected=context_size=0;memset(context,0,sizeof context);
 if(path&&strlen(path)<sizeof config_file)memcpy(config_file,path,strlen(path)+1);
 size_t bytes=0;char *text=file_read(config_file,&bytes);unsigned value=100;size_t a,b,c;int section;const char *nl;
 if(text&&!locate(text,bytes,"SoundtrackVolume",100,100,&a,&b,&c,&value,&section,&nl))value=100;
 LagoonSoundtrack_SetVolume(value);value=0;
 if(text&&!locate(text,bytes,"ShowSwordRange",0,1,&a,&b,&c,&value,&section,&nl))value=0;
 sword_range_enabled=value;free(text);
}
void LagoonSettings_Observe(const uint8_t *ram,size_t size){
 if(!ram||size<LAGOON_JOURNAL_RAM_BYTES||size<LAGOON_ITEM_GUIDE_RAM_BYTES){context_size=0;memset(context,0,sizeof context);return;}
 context_size=size<sizeof context?size:sizeof context;memcpy(context,ram,context_size);
 if(context_size<sizeof context)memset(context+context_size,0,sizeof context-context_size);
}
void LagoonSettings_SetTravelStatus(const char *message,int available){
 char next[sizeof travel_status];size_t i=0;
 if(!message||!*message)message=available?"Ready to return to the dungeon entrance.":"Return is unavailable here.";
 for(;i<sizeof next-1&&message[i];i++){unsigned char c=(unsigned char)message[i];next[i]=c<32||c==127?' ':message[i];}next[i]=0;
 if(!available||travel_available!=(available!=0)||strcmp(next,travel_status))travel_confirming=0;
 memcpy(travel_status,next,i+1);travel_available=available!=0;
}
void LagoonSettings_Open(void){opened=1;selected=0;active_tab=TAB_SOUND;item_selected=0;primed=repeat_armed=achievements_requested=travel_confirming=return_requested=0;quit_confirming=quit_selected=quit_requested=display_selected=zoom_requested=0;repeat_at=prior_input=0;}
void LagoonSettings_Close(void){opened=0;primed=repeat_armed=travel_confirming=quit_confirming=0;}
int LagoonSettings_IsOpen(void){return opened!=0;}
int LagoonSettings_SwordRangeEnabled(void){return sword_range_enabled!=0;}
int LagoonSettings_PollOpen(uint32_t human){if(opened&&!primed){prior_input=human;primed=1;}return opened!=0;}
int LagoonSettings_TakeAchievementsRequest(void){unsigned requested=achievements_requested;achievements_requested=0;return requested!=0;}
int LagoonSettings_TakeReturnRequest(void){unsigned requested=return_requested;return_requested=0;return requested!=0;}
int LagoonSettings_TakeQuitRequest(void){unsigned requested=quit_requested;quit_requested=0;return requested!=0;}
void LagoonSettings_SetZoom(unsigned percent){
 if((percent==100||percent==125||percent==150)&&!zoom_requested)view_zoom=percent;
}
int LagoonSettings_TakeZoomRequest(unsigned *percent){
 if(!zoom_requested||!percent)return 0;*percent=view_zoom;zoom_requested=0;return 1;
}

void LagoonSettings_Command(unsigned command){
 if(!opened)return;
 if(command==LAGOON_SETTINGS_CLOSE){if(quit_confirming){quit_confirming=quit_selected=repeat_armed=0;return;}if(travel_confirming){travel_confirming=repeat_armed=0;return;}LagoonSettings_Close();return;}
 if(command==LAGOON_SETTINGS_NEXT_TAB||command==LAGOON_SETTINGS_PREVIOUS_TAB){
  active_tab=(active_tab+(command==LAGOON_SETTINGS_NEXT_TAB?1:TAB_COUNT-1))%TAB_COUNT;
  travel_confirming=quit_confirming=quit_selected=repeat_armed=0;return;
 }
 if(quit_confirming){
  if(command==LAGOON_SETTINGS_LEFT||command==LAGOON_SETTINGS_RIGHT||command==LAGOON_SETTINGS_UP||command==LAGOON_SETTINGS_DOWN){quit_selected=!quit_selected;return;}
  if(command==LAGOON_SETTINGS_ACTIVATE){
   if(quit_selected){quit_requested=1;LagoonSettings_Close();}
   else quit_confirming=quit_selected=repeat_armed=0;
  }
  return;
 }
 if(active_tab==TAB_TRAVEL){
  if(command==LAGOON_SETTINGS_ACTIVATE&&travel_available){
   if(travel_confirming){return_requested=1;LagoonSettings_Close();}
   else travel_confirming=1;
  }
  return;
 }
 if(active_tab==TAB_JOURNAL)return;
 if(active_tab==TAB_DISPLAY){
  if(command==LAGOON_SETTINGS_UP||command==LAGOON_SETTINGS_DOWN){display_selected=!display_selected;return;}
  if(command==LAGOON_SETTINGS_ACTIVATE||command==LAGOON_SETTINGS_LEFT||command==LAGOON_SETTINGS_RIGHT){
   if(display_selected){
    unsigned index=view_zoom==125?1:view_zoom==150?2:0;
    index=(index+(command==LAGOON_SETTINGS_LEFT?2:1))%3;
    view_zoom=index==1?125:index==2?150:100;zoom_requested=1;
   }else{
    sword_range_enabled=!sword_range_enabled;
    range_save_failed=!persist_setting("ShowSwordRange",sword_range_enabled,0,1);
   }
  }
  return;
 }
 if(active_tab==TAB_ITEMS){
  size_t count=LagoonItemGuide_Count();if(!count){item_selected=0;return;}
  if(item_selected>=count)item_selected=count-1;
  if(command==LAGOON_SETTINGS_UP)item_selected=item_selected?item_selected-1:count-1;
  else if(command==LAGOON_SETTINGS_DOWN)item_selected=(item_selected+1)%count;
  return;
 }
 if(command==LAGOON_SETTINGS_ACTIVATE){if(selected==2){achievements_requested=1;LagoonSettings_Close();}else if(selected==3){quit_confirming=1;quit_selected=repeat_armed=0;}return;}
 if(command==LAGOON_SETTINGS_UP){selected=selected?selected-1:3;return;}
 if(command==LAGOON_SETTINGS_DOWN){selected=(selected+1)%4;return;}
 if(selected>=2)return;
 if(command!=LAGOON_SETTINGS_LEFT&&command!=LAGOON_SETTINGS_RIGHT)return;
 int value=selected?(int)LagoonSoundtrack_GetVolume():snesrecomp_desktop_get_volume();
 int next=value+(command==LAGOON_SETTINGS_LEFT?-5:5);if(next<0)next=0;if(next>100)next=100;if(next==value)return;
 if(selected){LagoonSoundtrack_SetVolume((unsigned)next);save_failed=!persist_setting("SoundtrackVolume",(unsigned)next,100,100);}
 else snesrecomp_desktop_set_volume(next);
}
void LagoonSettings_Tick(uint32_t human,uint32_t ticks){
 if(!opened)return;if(!primed){prior_input=human;primed=1;return;}
 uint32_t edge=human&~prior_input,nav=human&(SNES_PAD_UP|SNES_PAD_DOWN|SNES_PAD_LEFT|SNES_PAD_RIGHT);
 if(edge&SNES_PAD_B){LagoonSettings_Command(LAGOON_SETTINGS_CLOSE);prior_input=human;return;}
 if(edge&SNES_PAD_L)LagoonSettings_Command(LAGOON_SETTINGS_PREVIOUS_TAB);
 else if(edge&SNES_PAD_R)LagoonSettings_Command(LAGOON_SETTINGS_NEXT_TAB);
 unsigned display_activated=0,quit_activated=0;
 if(edge&(SNES_PAD_A|SNES_PAD_Y)){display_activated=active_tab==TAB_DISPLAY;quit_activated=quit_confirming||(active_tab==TAB_SOUND&&selected==3);LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);if(!opened)return;}
 if(edge&nav){repeat_at=ticks+SNES_OVL_REPEAT_DELAY;repeat_armed=1;}
 else if(nav&&repeat_armed&&(int32_t)(ticks-repeat_at)>=0){edge|=nav;repeat_at=ticks+SNES_OVL_REPEAT_RATE;}
 if(!nav)repeat_armed=0;
 if(display_activated)edge&=~(SNES_PAD_LEFT|SNES_PAD_RIGHT);
 if(quit_activated)edge&=~(SNES_PAD_UP|SNES_PAD_DOWN|SNES_PAD_LEFT|SNES_PAD_RIGHT);
 if(edge&SNES_PAD_UP)LagoonSettings_Command(LAGOON_SETTINGS_UP);else if(edge&SNES_PAD_DOWN)LagoonSettings_Command(LAGOON_SETTINGS_DOWN);
 if(edge&SNES_PAD_LEFT)LagoonSettings_Command(LAGOON_SETTINGS_LEFT);else if(edge&SNES_PAD_RIGHT)LagoonSettings_Command(LAGOON_SETTINGS_RIGHT);
 prior_input=human;
}
static void rect(int x,int y,int w,int h,uint32_t c){snes_ovl_fill_rect(panel,WIDTH,HEIGHT,x,y,w,h,c);}
static void text(int x,int y,const char *s,uint32_t c){LagoonUI_TextFit(panel,WIDTH,HEIGHT,x,y,s,c,1,WIDTH-x-26);}
static void wrapped(int x,int y,const char *source,uint32_t colour,int width,unsigned lines){
 if(!source)return;
 for(unsigned row=0;row<lines&&*source;row++){
  while(*source==' ')source++;if(!*source)break;
  char part[192];unsigned n=0,space=0;int pixels=0;
  while(source[n]&&n<sizeof part-1){char glyph[2]={source[n],0};int step=LagoonUI_TextWidth(glyph,1);if(n&&pixels+step>width)break;pixels+=step;if(source[n]==' ')space=n;part[n]=source[n];n++;}
  if(source[n]&&space)n=space;part[n]=0;source+=n;while(*source==' ')source++;
  if(row+1==lines&&*source&&n>3){part[n-3]='.';part[n-2]='.';part[n-1]='.';}
  LagoonUI_TextFit(panel,WIDTH,HEIGHT,x,y+(int)row*20,part,colour,1,width);
 }
}
static void tabs(void){
 static const char *names[]={"Sound","Journal","Items","Display","Travel"};static const int x[]={200,280,360,440,520};
 for(unsigned i=0;i<TAB_COUNT;i++){
  LagoonUI_Text(panel,WIDTH,HEIGHT,x[i],30,names[i],i==active_tab?LAGOON_UI_GOLD:LAGOON_UI_DIM,1);
  if(i==active_tab)rect(x[i],53,LagoonUI_TextWidth(names[i],1),2,LAGOON_UI_GOLD);
 }
}
static void journal_image(void){
 LagoonJournalEntry entry;
 if(!context_size||!LagoonJournal_Get(context,context_size,&entry)){
  wrapped(28,106,"Load or begin an adventure to view your journal.",LAGOON_UI_DIM,584,3);return;
 }
 text(28,88,entry.chapter,LAGOON_UI_GOLD);text(28,114,entry.title,LAGOON_UI_FG);
 wrapped(28,142,entry.detail,LAGOON_UI_FG,584,3);
 for(unsigned i=0;i<entry.milestone_count&&i<4;i++){
  int y=210+(int)i*18;uint32_t c=entry.completed_mask&(1u<<i)?LAGOON_UI_GOLD:LAGOON_UI_DIM;
  rect(30,y+3,9,9,c);rect(31,y+4,7,7,LAGOON_UI_BG);
  if(entry.completed_mask&(1u<<i)){rect(32,y+6,2,3,c);rect(34,y+7,2,3,c);rect(36,y+4,2,5,c);}
  LagoonUI_SmallFit(panel,WIDTH,HEIGHT,48,y+1,entry.milestone[i],c,560);
 }
}
static void items_image(void){
 size_t count=LagoonItemGuide_Count();if(!count){wrapped(28,106,"The item guide is not available in this session.",LAGOON_UI_DIM,584,3);return;}
 if(item_selected>=count)item_selected=count-1;size_t first=item_selected/5*5;
 for(size_t i=first;i<count&&i<first+5;i++){
  LagoonItemGuideEntry e;if(!LagoonItemGuide_GetAt(i,&e))continue;int y=94+(int)(i-first)*34;
  if(i==item_selected){rect(20,y-4,244,32,LAGOON_UI_SELECT);LagoonUI_Cursor(panel,WIDTH,HEIGHT,28,y+5,LAGOON_UI_GOLD);}
  LagoonUI_TextFit(panel,WIDTH,HEIGHT,46,y-1,e.name,LAGOON_UI_FG,1,204);
  LagoonUI_TextSmall(panel,WIDTH,HEIGHT,46,y+18,e.kind,LAGOON_UI_DIM);
 }
 rect(276,88,1,194,LAGOON_UI_LINE);LagoonItemGuideEntry e;
 if(LagoonItemGuide_GetAt(item_selected,&e)){
  LagoonUI_TextFit(panel,WIDTH,HEIGHT,294,92,e.name,LAGOON_UI_GOLD,1,310);
  LagoonUI_TextSmall(panel,WIDTH,HEIGHT,294,116,e.kind,LAGOON_UI_DIM);
  if(context_size)LagoonUI_TextSmall(panel,WIDTH,HEIGHT,294,135,LagoonItemGuide_Owned(context,context_size,e.id)?"Carried":"Not carried",LAGOON_UI_DIM);
  wrapped(294,156,e.description,LAGOON_UI_FG,310,4);
  char label[80];if(e.atland_price)snprintf(label,sizeof label,"Atland: %u G",e.atland_price);else snprintf(label,sizeof label,"Not sold in Atland");
  LagoonUI_TextSmall(panel,WIDTH,HEIGHT,294,254,label,LAGOON_UI_DIM);
  if(e.voloh_price)snprintf(label,sizeof label,"Voloh: %u G",e.voloh_price);else snprintf(label,sizeof label,"Not sold in Voloh");
  LagoonUI_TextSmall(panel,WIDTH,HEIGHT,294,271,label,LAGOON_UI_DIM);
 }
 char page[64];snprintf(page,sizeof page,"Items %zu-%zu of %zu",first+1,first+5<count?first+5:count,count);
 LagoonUI_TextSmall(panel,WIDTH,HEIGHT,28,274,page,LAGOON_UI_DIM);
}
static void display_image(void){
 int y=display_selected?157:94;
 rect(20,y-7,600,52,LAGOON_UI_SELECT);LagoonUI_Cursor(panel,WIDTH,HEIGHT,29,y+5,LAGOON_UI_GOLD);
 text(50,94,"Sword range cue",LAGOON_UI_FG);
 const char *state=sword_range_enabled?"On":"Off";
 text(602-LagoonUI_TextWidth(state,1),94,state,LAGOON_UI_GOLD);
 LagoonUI_TextSmall(panel,WIDTH,HEIGHT,50,120,"A visual reach guide; combat stays unchanged.",LAGOON_UI_DIM);
 text(50,157,"World zoom out",LAGOON_UI_FG);
 state=view_zoom==125?"1.25x":view_zoom==150?"1.5x":"1x";
 text(602-LagoonUI_TextWidth(state,1),157,state,LAGOON_UI_GOLD);
 LagoonUI_TextSmall(panel,WIDTH,HEIGHT,50,183,"Changes live when you return to the game.",LAGOON_UI_DIM);
 wrapped(50,222,"Zoom shortcut: R3 / Ctrl+Z. Widescreen must be enabled to reveal the expanded view.",LAGOON_UI_DIM,548,3);
}
static void quit_image(void){
 LagoonUI_Frame(panel,WIDTH,HEIGHT,28,90,584,188);
 wrapped(50,106,"Quit to desktop?",LAGOON_UI_FG,540,2);
 wrapped(50,149,"Only progress you've saved in-game will be kept.",LAGOON_UI_DIM,540,2);
 for(unsigned i=0;i<2;i++){
  int x=92+(int)i*252;
  if(quit_selected==i){rect(x,223,204,40,LAGOON_UI_SELECT);LagoonUI_Cursor(panel,WIDTH,HEIGHT,x+12,236,LAGOON_UI_GOLD);}
  text(x+34,228,i?"Quit":"Cancel",quit_selected==i?LAGOON_UI_GOLD:LAGOON_UI_FG);
 }
}
static void travel_image(void){
 if(travel_confirming){
  LagoonUI_Frame(panel,WIDTH,HEIGHT,28,90,584,188);
  wrapped(50,106,"Return to the dungeon entrance?",LAGOON_UI_FG,540,2);
  wrapped(50,158,"You'll arrive inside the entrance.",LAGOON_UI_DIM,540,2);
  LagoonUI_Cursor(panel,WIDTH,HEIGHT,50,235,LAGOON_UI_GOLD);
  text(72,227,"A / Enter: return",LAGOON_UI_GOLD);
  return;
 }
 rect(20,87,600,48,LAGOON_UI_SELECT);
 if(travel_available)LagoonUI_Cursor(panel,WIDTH,HEIGHT,29,101,LAGOON_UI_GOLD);
 text(50,94,"Return to dungeon entrance",travel_available?LAGOON_UI_FG:LAGOON_UI_DIM);
 wrapped(50,151,*travel_status?travel_status:"Return is unavailable here.",LAGOON_UI_DIM,548,4);
}
int LagoonSettings_Image(const uint32_t **pixels,int *width,int *height){
 if(!opened)return 0;const uint32_t bg=LAGOON_UI_BG,fg=LAGOON_UI_FG,dim=LAGOON_UI_DIM,accent=LAGOON_UI_GOLD;
 for(unsigned n=0;n<WIDTH*HEIGHT;n++)panel[n]=bg;
 LagoonUI_Frame(panel,WIDTH,HEIGHT,0,0,WIDTH,HEIGHT);
 LagoonUI_Text(panel,WIDTH,HEIGHT,28,14,"Options",fg,2);
 tabs();LagoonUI_TextSmall(panel,WIDTH,HEIGHT,28,60,active_tab==TAB_SOUND?"Changes apply immediately. The game is paused.":active_tab==TAB_JOURNAL?"Your current journey":active_tab==TAB_ITEMS?"Item reference and shop prices":active_tab==TAB_DISPLAY?"Live display options and optional visual guides":"Return to a known dungeon entrance",dim);
 rect(28,76,584,1,LAGOON_UI_LINE);
 if(quit_confirming)quit_image();else if(active_tab==TAB_JOURNAL)journal_image();else if(active_tab==TAB_ITEMS)items_image();else if(active_tab==TAB_DISPLAY)display_image();else if(active_tab==TAB_TRAVEL)travel_image();else for(unsigned row=0;row<4;row++){
  int y=92+(int)row*48;char label[48];
  if(row==selected){rect(20,y-5,600,45,LAGOON_UI_SELECT);LagoonUI_Cursor(panel,WIDTH,HEIGHT,29,y+5,accent);}
  text(50,y,row==3?"Quit to desktop":row==2?"Achievements":row?"X68000 music":"Master volume",fg);
  if(row<2){int v=row?(int)LagoonSoundtrack_GetVolume():snesrecomp_desktop_get_volume();snprintf(label,sizeof label,"%d%%",v);text(602-LagoonUI_TextWidth(label,1),y,label,fg);rect(50,y+22,550,4,LAGOON_UI_LINE);rect(50,y+22,550*v/100,4,accent);
   LagoonUI_TextSmall(panel,WIDTH,HEIGHT,50,y+30,row?"Adjust the optional soundtrack's music level.":"Adjust the game's overall sound level.",dim);}
  else{text(565,y,row==3?"Quit":"Open",accent);LagoonUI_TextSmall(panel,WIDTH,HEIGHT,50,y+24,row==3?"Close the game after a confirmation.":"Browse your journey and earned achievements.",dim);}
 }
 rect(28,292,584,1,LAGOON_UI_LINE);
 LagoonUI_TextSmall(panel,WIDTH,HEIGHT,28,305,quit_confirming?"Left/Right: choose    A / Enter: select":active_tab==TAB_SOUND?"Up/Down: select    Left/Right: adjust":active_tab==TAB_ITEMS?"Up/Down: browse items":active_tab==TAB_DISPLAY?"Up/Down: select    Left/Right or A / Enter: change":active_tab==TAB_TRAVEL?travel_confirming?"A / Enter: confirm return":travel_available?"A / Enter: choose return":"Return is not available at this moment.":"Progress follows your current adventure.",fg);
 LagoonUI_TextSmall(panel,WIDTH,HEIGHT,28,322,quit_confirming?"B / Esc: cancel":travel_confirming?"L / R or PageUp / PageDown: tabs    B / Esc: cancel":"L / R or PageUp / PageDown: tabs    B / Esc: close",dim);
 if(active_tab==TAB_SOUND&&selected>=2&&!quit_confirming)LagoonUI_TextSmall(panel,WIDTH,HEIGHT,28,341,selected==3?"A / Enter: choose quit":"A / Enter: open achievements",accent);
 if(active_tab==TAB_SOUND&&save_failed&&!quit_confirming)LagoonUI_SmallFit(panel,WIDTH,HEIGHT,28,341,"Music volume couldn't be saved. Try again from the launcher.",accent,582);
 if(active_tab==TAB_DISPLAY&&range_save_failed)LagoonUI_SmallFit(panel,WIDTH,HEIGHT,28,341,"Setting couldn't be saved. The range cue applies for this session.",accent,582);
 if(active_tab==TAB_DISPLAY&&zoom_save_failed)LagoonUI_SmallFit(panel,WIDTH,HEIGHT,28,341,"Zoom applies now but couldn't be saved. Try again in Companion.",accent,582);
 if(pixels)*pixels=panel;if(width)*width=WIDTH;if(height)*height=HEIGHT;return 1;
}
