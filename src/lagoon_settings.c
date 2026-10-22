/* A frozen host-settings panel. No guest RAM, quest, save-state or emulation
 * timing is changed here. Music trim persists by editing one owned INI key;
 * all other bytes remain exact, with a concurrent-edit check before replace. */
#include "lagoon_settings.h"
#include "lagoon_soundtrack.h"
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
static uint32_t panel[WIDTH*HEIGHT],prior_input,repeat_at;
static unsigned opened,selected,primed,repeat_armed,achievements_requested;
static char config_file[1024];
static int save_failed;
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
static int locate(const char *text,size_t bytes,size_t *begin,size_t *end,size_t *insert,unsigned *value,int *has_section,const char **newline){
 unsigned section=0,found=0,crlf=0,lf=0;*begin=*end=*insert=bytes;*has_section=0;*value=100;
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
    if(equal&&same_name(a,equal,"SoundtrackVolume")){
     if(found++)return 0;*begin=at;*end=next;
     const char *v=equal+1;while(v<b&&isspace((unsigned char)*v))v++;
     char *tail;if(v>=b)return 0;unsigned long number=strtoul(v,&tail,10);if(tail==v||tail>b||number>100)return 0;
     while(tail<b&&isspace((unsigned char)*tail))tail++;
     if(tail<b&&*tail!='#'&&*tail!=';')return 0;*value=(unsigned)number;
    }
   }
  }
  at=next;
 }
 return 1;
}
static int persist_music(unsigned volume){
 size_t bytes=0;char *before=file_read(config_file,&bytes);if(!before)return 0;
 size_t begin,end,insert;unsigned old;int section;const char *nl;
 if(!locate(before,bytes,&begin,&end,&insert,&old,&section,&nl)){free(before);return 0;}
 char line[96];snprintf(line,sizeof line,"SoundtrackVolume = %u%s",volume,nl);
 const char *prefix="";char heading[32];size_t replace_begin=begin,replace_end=end;
 if(begin==bytes){
  replace_begin=replace_end=section?insert:bytes;
  snprintf(heading,sizeof heading,"%s%s%s",replace_begin&&before[replace_begin-1]!='\n'?nl:"",section?"":"[Lagoon]",section?"":nl);prefix=heading;
 }
 size_t plen=strlen(prefix),llen=strlen(line),total=replace_begin+plen+llen+bytes-replace_end;
 char *after=malloc(total);if(!after){free(before);return 0;}
 memcpy(after,before,replace_begin);memcpy(after+replace_begin,prefix,plen);memcpy(after+replace_begin+plen,line,llen);memcpy(after+replace_begin+plen+llen,before+replace_end,bytes-replace_end);
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
 if(f)file_remove(temporary);free(after);free(before);return wrote;
}
void LagoonSettings_Configure(const char *path){
 opened=selected=primed=repeat_armed=achievements_requested=0;prior_input=repeat_at=0;save_failed=0;config_file[0]=0;
 if(path&&strlen(path)<sizeof config_file)memcpy(config_file,path,strlen(path)+1);
 size_t bytes=0;char *text=file_read(config_file,&bytes);unsigned value=100;size_t a,b,c;int section;const char *nl;
 if(text&&!locate(text,bytes,&a,&b,&c,&value,&section,&nl))value=100;free(text);LagoonSoundtrack_SetVolume(value);
}
void LagoonSettings_Open(void){opened=1;selected=0;primed=repeat_armed=achievements_requested=0;repeat_at=prior_input=0;}
void LagoonSettings_Close(void){opened=0;primed=repeat_armed=0;}
int LagoonSettings_IsOpen(void){return opened!=0;}
int LagoonSettings_PollOpen(uint32_t human){if(opened&&!primed){prior_input=human;primed=1;}return opened!=0;}
int LagoonSettings_TakeAchievementsRequest(void){unsigned requested=achievements_requested;achievements_requested=0;return requested!=0;}
void LagoonSettings_Command(unsigned command){
 if(!opened)return;
 if(command==LAGOON_SETTINGS_CLOSE){LagoonSettings_Close();return;}
 if(command==LAGOON_SETTINGS_ACTIVATE){if(selected==2){achievements_requested=1;LagoonSettings_Close();}return;}
 if(command==LAGOON_SETTINGS_UP){selected=selected?selected-1:2;return;}
 if(command==LAGOON_SETTINGS_DOWN){selected=(selected+1)%3;return;}
 if(selected==2)return;
 if(command!=LAGOON_SETTINGS_LEFT&&command!=LAGOON_SETTINGS_RIGHT)return;
 int value=selected?(int)LagoonSoundtrack_GetVolume():snesrecomp_desktop_get_volume();
 int next=value+(command==LAGOON_SETTINGS_LEFT?-5:5);if(next<0)next=0;if(next>100)next=100;if(next==value)return;
 if(selected){LagoonSoundtrack_SetVolume((unsigned)next);save_failed=!persist_music((unsigned)next);}
 else snesrecomp_desktop_set_volume(next);
}
void LagoonSettings_Tick(uint32_t human,uint32_t ticks){
 if(!opened)return;if(!primed){prior_input=human;primed=1;return;}
 uint32_t edge=human&~prior_input,nav=human&(SNES_PAD_UP|SNES_PAD_DOWN|SNES_PAD_LEFT|SNES_PAD_RIGHT);
 if(edge&SNES_PAD_B){LagoonSettings_Close();return;}
 if(edge&(SNES_PAD_A|SNES_PAD_Y)){LagoonSettings_Command(LAGOON_SETTINGS_ACTIVATE);if(!opened)return;}
 if(edge&nav){repeat_at=ticks+SNES_OVL_REPEAT_DELAY;repeat_armed=1;}
 else if(nav&&repeat_armed&&(int32_t)(ticks-repeat_at)>=0){edge|=nav;repeat_at=ticks+SNES_OVL_REPEAT_RATE;}
 if(!nav)repeat_armed=0;
 if(edge&SNES_PAD_UP)LagoonSettings_Command(LAGOON_SETTINGS_UP);else if(edge&SNES_PAD_DOWN)LagoonSettings_Command(LAGOON_SETTINGS_DOWN);
 if(edge&SNES_PAD_LEFT)LagoonSettings_Command(LAGOON_SETTINGS_LEFT);else if(edge&SNES_PAD_RIGHT)LagoonSettings_Command(LAGOON_SETTINGS_RIGHT);
 prior_input=human;
}
static void rect(int x,int y,int w,int h,uint32_t c){snes_ovl_fill_rect(panel,WIDTH,HEIGHT,x,y,w,h,c);}
static void text(int x,int y,const char *s,uint32_t c){snes_ovl_draw_text(panel,WIDTH,HEIGHT,x,y,s,c,1);}
int LagoonSettings_Image(const uint32_t **pixels,int *width,int *height){
 if(!opened)return 0;const uint32_t bg=0xff101a28,fg=0xffe2ecf1,dim=0xff91a3b6,accent=0xff65ddc3;
 for(unsigned n=0;n<WIDTH*HEIGHT;n++)panel[n]=bg;
 rect(0,0,WIDTH,4,accent);text(28,25,"LAGOON  /  QUICK SETTINGS",accent);
 text(28,52,"GAME PAUSED - CHANGES APPLY IMMEDIATELY",dim);
 for(unsigned row=0;row<3;row++){
  int y=91+(int)row*64;char label[48];
  if(row==selected){rect(16,y-10,608,54,0xff203548);text(28,y,">",accent);}
  text(48,y,row==2?"ACHIEVEMENTS":row?"X68000 MUSIC":"MASTER VOLUME",fg);
  if(row<2){int v=row?(int)LagoonSoundtrack_GetVolume():snesrecomp_desktop_get_volume();snprintf(label,sizeof label,"%d%%",v);text(552,y,label,fg);rect(48,y+23,548,6,0xff314357);rect(48,y+23,548*v/100,6,accent);}
  else{text(552,y,"OPEN",accent);text(48,y+23,"VIEW YOUR JOURNEY AND LOCAL UNLOCKS",dim);}
 }
 text(28,282,"UP/DOWN: SELECT     LEFT/RIGHT: ADJUST",fg);
 text(28,307,selected==2?"A / ENTER: OPEN     B / ESC: CLOSE":"B / ESC: CLOSE     R2 / CTRL+COMMA: TOGGLE",dim);
 text(28,333,save_failed?"MUSIC LEVEL IS SESSION ONLY - CONFIG COULD NOT BE SAVED":"PREFERENCES AND ACHIEVEMENTS STAY OUTSIDE GAME SAVES.",save_failed?0xffffbf69:dim);
 if(pixels)*pixels=panel;if(width)*width=WIDTH;if(height)*height=HEIGHT;return 1;
}
