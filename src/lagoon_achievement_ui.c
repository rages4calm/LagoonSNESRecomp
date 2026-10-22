/* Frozen browser and nonblocking toast in the existing Lagoon host-panel
 * visual language. Definitions/progress are copied from the core's published
 * read-only view; no event, storage, guest-memory or emulation calls belong here. */
#include "lagoon_achievement_ui.h"
#include "lagoon_achievements.h"
#include "snes_overlay_draw.h"
#include <stdio.h>
#include <string.h>

enum { WIDTH=640,HEIGHT=360,ROWS=5,TOAST_WIDTH=144,TOAST_HEIGHT=32,TOAST_MS=5500 };
static const uint32_t BG=0xff101a28,FG=0xffe2ecf1,DIM=0xff91a3b6,
  ACCENT=0xff65ddc3,GOLD=0xffffcf77,SELECTED=0xff203548,LINE=0xff314357;
static uint32_t panel[WIDTH*HEIGHT],toast_panel[TOAST_WIDTH*TOAST_HEIGHT];
static uint32_t prior_input,repeat_at,toast_started,toast_paused_at;
static size_t selected,toast_index;
static unsigned opened,primed,repeat_armed,toast_active,toast_paused;

static void limited_text(uint32_t *image,int width,int height,int x,int y,
                         const char *source,unsigned columns,uint32_t colour) {
  char text[96];unsigned length=0;
  if(!source)return;
  if(columns>=sizeof text)columns=sizeof text-1;
  while(source[length]&&length<columns){unsigned char c=(unsigned char)source[length];text[length]=(c>=32&&c<=126)?(char)c:'?';length++;}
  if(source[length]&&columns>=3){text[columns-3]='.';text[columns-2]='.';text[columns-1]='.';length=columns;}
  text[length]=0;snes_ovl_draw_text(image,width,height,x,y,text,colour,1);
}
static void text(int x,int y,const char *s,unsigned columns,uint32_t c) {
  limited_text(panel,WIDTH,HEIGHT,x,y,s,columns,c);
}
static void rect(int x,int y,int width,int height,uint32_t c) {
  snes_ovl_fill_rect(panel,WIDTH,HEIGHT,x,y,width,height,c);
}
/* A small, authored trophy in the same pixel geometry as the existing font. */
static void trophy(uint32_t *image,int width,int height,int x,int y,uint32_t c) {
  snes_ovl_fill_rect(image,width,height,x+3,y,12,3,c);
  snes_ovl_fill_rect(image,width,height,x+4,y+3,10,5,c);
  snes_ovl_fill_rect(image,width,height,x+6,y+8,6,3,c);
  snes_ovl_fill_rect(image,width,height,x+8,y+11,2,4,c);
  snes_ovl_fill_rect(image,width,height,x+4,y+15,10,2,c);
  snes_ovl_stroke_rect(image,width,height,x,y+2,4,7,c);
  snes_ovl_stroke_rect(image,width,height,x+14,y+2,4,7,c);
}
static const char *row_title(const LagoonAchievementRow *row) {
  return row->secret&&!row->unlocked_at?"A familiar stranger":row->title;
}
static const char *row_description(const LagoonAchievementRow *row) {
  return row->secret&&!row->unlocked_at?"Find a traveler who has taken a wrong turn.":row->description;
}
static void description(const char *source,int x,int y,unsigned columns,unsigned lines) {
  if(!source)return;
  for(unsigned line=0;line<lines&&*source;line++){
    while(*source==' ')source++;
    size_t remaining=strlen(source),length=remaining<columns?remaining:columns;
    if(remaining>columns){size_t space=length;while(space&&source[space]!=' ')space--;if(space)length=space;}
    char part[96];if(length>=sizeof part)length=sizeof part-1;
    memcpy(part,source,length);part[length]=0;
    source+=length;while(*source==' ')source++;
    if(line+1==lines&&*source&&length>=3){part[length-3]='.';part[length-2]='.';part[length-1]='.';}
    text(x,y+(int)line*13,part,columns,DIM);
  }
}
void LagoonAchievementUI_Reset(void) {
  opened=primed=repeat_armed=toast_active=toast_paused=0;selected=toast_index=0;
  prior_input=repeat_at=toast_started=toast_paused_at=0;
}
void LagoonAchievementUI_Open(void) {
  opened=1;selected=0;primed=repeat_armed=0;prior_input=repeat_at=0;
}
void LagoonAchievementUI_Close(void) {opened=primed=repeat_armed=0;}
int LagoonAchievementUI_IsOpen(void) {return opened!=0;}
int LagoonAchievementUI_PollOpen(uint32_t human) {
  if(opened&&!primed){prior_input=human;primed=1;}
  return opened!=0;
}
void LagoonAchievementUI_Command(unsigned command) {
  if(!opened)return;
  if(command==LAGOON_ACHIEVEMENT_UI_CLOSE){LagoonAchievementUI_Close();return;}
  size_t count=LagoonAchievements_Count();if(!count){selected=0;return;}
  if(selected>=count)selected=count-1;
  if(command==LAGOON_ACHIEVEMENT_UI_UP)selected=selected?selected-1:count-1;
  else if(command==LAGOON_ACHIEVEMENT_UI_DOWN)selected=(selected+1)%count;
  else if(command==LAGOON_ACHIEVEMENT_UI_FIRST)selected=0;
  else if(command==LAGOON_ACHIEVEMENT_UI_LAST)selected=count-1;
  else if(command==LAGOON_ACHIEVEMENT_UI_PREVIOUS||command==LAGOON_ACHIEVEMENT_UI_NEXT){
    size_t pages=(count+ROWS-1)/ROWS,page=selected/ROWS,offset=selected%ROWS;
    page=command==LAGOON_ACHIEVEMENT_UI_PREVIOUS?(page?page-1:pages-1):(page+1)%pages;
    selected=page*ROWS+offset;if(selected>=count)selected=count-1;
  }
}
void LagoonAchievementUI_Tick(uint32_t human,uint32_t ticks) {
  if(toast_active&&!toast_paused&&(uint32_t)(ticks-toast_started)>=TOAST_MS)toast_active=0;
  if(!opened)return;
  if(!primed){prior_input=human;primed=1;return;}
  uint32_t edge=human&~prior_input,nav=human&(SNES_PAD_UP|SNES_PAD_DOWN|SNES_PAD_LEFT|SNES_PAD_RIGHT);
  if(edge&(SNES_PAD_B|SNES_PAD_SELECT)){LagoonAchievementUI_Close();return;}
  if(edge&nav){repeat_at=ticks+SNES_OVL_REPEAT_DELAY;repeat_armed=1;}
  else if(nav&&repeat_armed&&(int32_t)(ticks-repeat_at)>=0){edge|=nav;repeat_at=ticks+SNES_OVL_REPEAT_RATE;}
  if(!nav)repeat_armed=0;
  if(edge&SNES_PAD_UP)LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_UP);
  else if(edge&SNES_PAD_DOWN)LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_DOWN);
  if(edge&SNES_PAD_LEFT)LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_PREVIOUS);
  else if(edge&SNES_PAD_RIGHT)LagoonAchievementUI_Command(LAGOON_ACHIEVEMENT_UI_NEXT);
  prior_input=human;
}
int LagoonAchievementUI_Image(const uint32_t **pixels,int *width,int *height) {
  if(!opened)return 0;
  size_t count=LagoonAchievements_Count();if(count&&selected>=count)selected=count-1;
  for(unsigned i=0;i<WIDTH*HEIGHT;i++)panel[i]=BG;
  rect(0,0,WIDTH,4,ACCENT);text(28,25,"LAGOON  /  ACHIEVEMENTS",70,ACCENT);
  char label[96];snprintf(label,sizeof label,"%zu OF %zu EARNED",LagoonAchievements_Unlocked(),count);text(28,50,label,70,FG);
  text(312,50,"LOCAL PROGRESS - GAME PAUSED",38,DIM);
  size_t first=(selected/ROWS)*ROWS;
  for(size_t i=first;i<count&&i<first+ROWS;i++){
    LagoonAchievementRow row;if(!LagoonAchievements_GetRow(i,&row))continue;
    int y=82+(int)(i-first)*38;
    if(i==selected)rect(16,y-7,608,34,SELECTED);
    if(row.unlocked_at)trophy(panel,WIDTH,HEIGHT,29,y+1,GOLD);
    else{snes_ovl_stroke_rect(panel,WIDTH,HEIGHT,33,y+3,10,10,LINE);rect(36,y+6,4,4,DIM);}
    text(64,y,row_title(&row),53,row.unlocked_at?GOLD:FG);
    if(row.unlocked_at)snprintf(label,sizeof label,"EARNED");
    else if(row.target>1)snprintf(label,sizeof label,"%u / %u",row.progress,row.target);
    else snprintf(label,sizeof label,"LOCKED");
    text(64,y+14,label,56,row.unlocked_at?GOLD:DIM);
    if(row.target>1&&!row.unlocked_at){
      uint32_t progress=row.progress>row.target?row.target:row.progress;
      rect(336,y+16,248,4,LINE);
      rect(336,y+16,(int)((uint64_t)248*progress/row.target),4,ACCENT);
    }
  }
  if(!count){text(28,113,"ACHIEVEMENTS ARE NOT AVAILABLE IN THIS SESSION.",70,FG);text(28,137,"YOUR GAME AND SAVES ARE UNCHANGED.",70,DIM);}
  size_t pages=count?(count+ROWS-1)/ROWS:0;
  snprintf(label,sizeof label,"PAGE %zu / %zu",count?selected/ROWS+1:0,pages);text(28,272,label,35,ACCENT);
  LagoonAchievementRow row;
  if(count&&LagoonAchievements_GetRow(selected,&row))description(row_description(&row),28,294,72,2);
  text(28,325,LagoonAchievements_Status(),72,DIM);
  text(28,346,"UP/DOWN: SELECT   LEFT/RIGHT: PAGE   B / ESC: CLOSE",72,FG);
  if(pixels)*pixels=panel;if(width)*width=WIDTH;if(height)*height=HEIGHT;return 1;
}
void LagoonAchievementUI_StartToast(size_t index,uint32_t ticks) {
  LagoonAchievementRow row;if(!LagoonAchievements_GetRow(index,&row)||!row.unlocked_at)return;
  toast_index=index;toast_started=ticks;toast_active=1;toast_paused=0;
}
void LagoonAchievementUI_PauseToast(int paused,uint32_t ticks) {
  if(!toast_active)return;
  if(paused&&!toast_paused){toast_paused_at=ticks;toast_paused=1;}
  else if(!paused&&toast_paused){toast_started+=ticks-toast_paused_at;toast_paused=0;}
}
int LagoonAchievementUI_ToastActive(void) {return toast_active!=0;}
int LagoonAchievementUI_ToastImage(const uint32_t **pixels,int *width,int *height) {
  if(!toast_active||toast_paused||opened)return 0;
  LagoonAchievementRow row;if(!LagoonAchievements_GetRow(toast_index,&row)||!row.unlocked_at)return 0;
  for(unsigned i=0;i<TOAST_WIDTH*TOAST_HEIGHT;i++)toast_panel[i]=BG;
  snes_ovl_stroke_rect(toast_panel,TOAST_WIDTH,TOAST_HEIGHT,0,0,TOAST_WIDTH,TOAST_HEIGHT,GOLD);
  trophy(toast_panel,TOAST_WIDTH,TOAST_HEIGHT,5,12,GOLD);
  limited_text(toast_panel,TOAST_WIDTH,TOAST_HEIGHT,28,4,"UNLOCKED",14,ACCENT);
  const char *source=row.title;
  for(unsigned line=0;source&&*source&&line<2;line++){
    while(*source==' ')source++;
    size_t remaining=strlen(source),length=remaining<14?remaining:14;
    if(remaining>14){size_t space=length;while(space&&source[space]!=' ')space--;if(space)length=space;}
    char title[15];memcpy(title,source,length);title[length]=0;source+=length;while(*source==' ')source++;
    if(line==1&&*source&&length>=3){title[length-3]='.';title[length-2]='.';title[length-1]='.';}
    limited_text(toast_panel,TOAST_WIDTH,TOAST_HEIGHT,28,14+(int)line*9,title,14,FG);
  }
  if(pixels)*pixels=toast_panel;if(width)*width=TOAST_WIDTH;if(height)*height=TOAST_HEIGHT;return 1;
}
