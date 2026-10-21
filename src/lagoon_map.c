/* Explored map from original room/door descriptors and observed collision
 * attributes. Source:018B84/01C87B/018385. Static exits are landmarks, never
 * authority to unlock a door or declare an unexplored destination visited. */
#include "lagoon_map.h"
#include "lagoon_scene_renderer.h"
#include "lagoon_quest_guidance.h"
#include "common_rtl.h"
#include "snes_overlay_draw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#define MW 640
#define MH 520
#define DOORS 64
#define CHESTS 128
#define BG 0xff101a28u
#define FG 0xffe2ecf1u
#define DIM 0xff91a3b6u
#define TEAL 0xff65ddc3u
#define GOLD 0xfff2c46au
typedef struct MapDoor {uint16_t x,y;uint8_t room;} MapDoor;
typedef struct MapChest {uint16_t x,y;uint8_t room,flag,opened;} MapChest;
typedef struct MapRoom {uint16_t w,h;unsigned count;MapDoor door[DOORS];char name[32];} MapRoom;
static MapRoom rooms[LAGOON_MAP_ROOMS];
static MapChest chests[CHESTS];static unsigned chest_count;
static uint8_t cells[LAGOON_MAP_ROOMS][LAGOON_MAP_CELLS];
static uint32_t panel[MW*MH];
static const uint8_t *source;static size_t source_size;
static unsigned current=75,selected=75,px,py,available,open,pending,dirty;
static uint32_t prior_human,prior_nav;static uint64_t last_generation;
static char save_path[1024];static int save_loaded,save_invalid;
static int selftest_frame=-2,selftest_opened;static unsigned selftest_pumps;
static unsigned rd16(const uint8_t *p){return p[0]|p[1]<<8;}
static unsigned rd24(const uint8_t *p){return rd16(p)|p[2]<<16;}
static int rom_at(unsigned a,unsigned n,size_t *out){
 size_t o=((a>>16)&127)*32768+(a&32767);
 if(!(a&0x8000)||o>source_size||n>source_size-o)return 0;
 *out=o;return 1;
}
static int visited(unsigned r){if(r>=75)return 0;for(unsigned i=0;i<16384;i++)if(cells[r][i])return 1;return 0;}
static int chest_seen(const MapChest *c){return c->room<75&&cells[c->room][(c->y/16)*128+c->x/16]!=0;}
static uint32_t checksum(const uint8_t *p,size_t n){uint32_t h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;}
/* Explicit bytes, not a compiler-dependent struct. Corrupt/foreign files are
 * preserved and disable persistence; they never become exploration. */
static void load_save(void){
 if(save_loaded)return;save_loaded=1;
 const char *root=RtlSaveRoot();
 if(!root||snprintf(save_path,sizeof(save_path),"%s/lagoon-exploration.map",root)>=(int)sizeof(save_path)){save_invalid=1;return;}
 FILE *f=fopen(save_path,"rb");if(!f)return;
 uint8_t header[16],*data=malloc(sizeof(cells));
 int good=data&&fread(header,1,16,f)==16&&!memcmp(header,"LGMAP001",8)&&
    rd24(header+8)==sizeof(cells)&&header[11]==0&&fread(data,1,sizeof(cells),f)==sizeof(cells)&&fgetc(f)==EOF;
 if(good){uint32_t sum=header[12]|header[13]<<8|header[14]<<16|(uint32_t)header[15]<<24;
   good=sum==checksum(data,sizeof(cells));
   for(size_t i=0;good&&i<sizeof(cells);i++)if(data[i]&0x7c)good=0;
 }
 fclose(f);
 if(good)memcpy(cells,data,sizeof(cells));else {save_invalid=1;fprintf(stderr,"[lagoon_map] invalid exploration preserved: %s\n",save_path);}
 free(data);
}
void LagoonMap_Save(void){
 if(!dirty||!save_loaded||save_invalid||!save_path[0])return;
 char tmp[1040];if(snprintf(tmp,sizeof(tmp),"%s.tmp",save_path)>=(int)sizeof(tmp))return;
 uint32_t sum=checksum((uint8_t*)cells,sizeof(cells));
 uint8_t h[16]={'L','G','M','A','P','0','0','1',sizeof(cells)&255,(sizeof(cells)>>8)&255,(sizeof(cells)>>16)&255,0,sum&255,(sum>>8)&255,(sum>>16)&255,sum>>24};
 FILE *f=fopen(tmp,"wb");if(!f)return;
 int good=fwrite(h,1,16,f)==16&&fwrite(cells,1,sizeof(cells),f)==sizeof(cells);
 if(fclose(f))good=0;if(!good)return;
#ifdef _WIN32
 if(!MoveFileExA(tmp,save_path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return;
#else
 if(rename(tmp,save_path))return;
#endif
 dirty=0;
}
void LagoonMap_Init(const uint8_t *rom,size_t size){
 source=rom;source_size=size;available=open=pending=0;current=selected=75;
 memset(rooms,0,sizeof(rooms));memset(cells,0,sizeof(cells));
 memset(chests,0,sizeof(chests));chest_count=0;
 save_loaded=save_invalid=dirty=0;save_path[0]=0;prior_human=prior_nav=0;
 if(!rom||size!=0x100000)return;
 for(unsigned r=0;r<75;r++){
  size_t at;if(!rom_at(0x05b2df+r*0x38,56,&at))return;
  rooms[r].w=rd16(rom+at+3)*8;rooms[r].h=rd16(rom+at+5)*8;
  if(!rooms[r].w||rooms[r].w>2048||!rooms[r].h||rooms[r].h>2048)return;
  /* Original0287F0 resolves bank04 room names through048000. Decode only
   * the original alphabet; room3F intentionally has an empty source label. */
  if(!rom_at(0x048000+r*2,2,&at))return;unsigned name=0x040000|rd16(rom+at);
  unsigned k;for(k=0;k<sizeof(rooms[r].name)-1;k++){
   if(!rom_at(name+k,1,&at))return;unsigned c=rom[at];if(c==255)break;
   if(c==254)c=' ';else if(c>=0x5d&&c<=0x76)c-=28;
   else if(c<'A'||c>'Z')return;
   rooms[r].name[k]=(char)c;
  }if(k==sizeof(rooms[r].name)-1)return;
  if(!rom_at(0x058059+r*2,2,&at))return;unsigned p=0x050000|rd16(rom+at);
  unsigned n;for(n=0;n<DOORS;n++,p+=11){
   if(!rom_at(p,11,&at))return;const uint8_t *d=rom+at;
   if(rd16(d)==65535)break;
   if(d[8]>=75)return;
   /* Eight original exit rows lie outside their room descriptor bounds.
    * Keep their guest meaning untouched; do not invent clipped map positions. */
   if(rd16(d)*32>=rooms[r].w||rd16(d+2)*32>=rooms[r].h)continue;
   rooms[r].door[rooms[r].count++]=(MapDoor){rd16(d)*32+16,rd16(d+2)*32+16,d[8]};
  }if(n==DOORS)return;
 }
 /* Original C969 uses BG interaction cells, not actor types. Source table
  * 05903C selects six-byte rows [x32,y32,flag,reward,lo,hi], terminated by FF.
  * Store no reward/item roster in the map. The actual ROM has33 entries. */
 for(unsigned r=0;r<75;r++){
  size_t at;if(!rom_at(0x05903c+r*2,2,&at))return;unsigned p=0x050000|rd16(rom+at);
  unsigned n;for(n=0;n<CHESTS;n++,p+=6){
   if(!rom_at(p,6,&at))return;const uint8_t *d=rom+at;if(d[0]==255)break;
   if(d[2]>=128||d[3]<1||d[3]>3)return;
   if(d[0]*32>=rooms[r].w||d[1]*32>=rooms[r].h)continue;
   if(chest_count==CHESTS)return;
   chests[chest_count++]=(MapChest){d[0]*32+16,d[1]*32+16,r,d[2],0};
  }if(n==CHESTS)return;
 }
 available=1;
}
static int collision(unsigned base,unsigned meta,unsigned q,unsigned *v){
 size_t at;if(!rom_at(base+meta*4+q,1,&at))return 0;*v=source[at];return 1;
}
void LagoonMap_Observe(const uint8_t *r,uint64_t generation){
 if(!available||!r)return;load_save();
 if(generation!=last_generation){last_generation=generation;current=75;open=pending=0;}
 unsigned room=rd16(r+0x48e),x=rd16(r+0x502),y=rd16(r+0x504);
 if(room>=75||r[0x100e]||(r[0x500]&0xc0)!=0xc0||!rd16(r+0x520)||
    !LagoonSceneRenderer_ExactRoom(r)||rd16(r+0x53)*8!=rooms[room].w||
    rd16(r+0x55)*8!=rooms[room].h||x>=rooms[room].w||y>=rooms[room].h){current=75;return;}
 current=room;px=x;py=y;
 /* CDF9/CE21 use the original persistent04C0 bitset, MSB first. Flags alone
  * never discover a chest; the drawing path still requires explored fog. */
 for(unsigned i=0;i<chest_count;i++)chests[i].opened=!!(r[0x4c0+chests[i].flag/8]&(0x80u>>(chests[i].flag&7)));
 /* Nearby observed geometry only. Four source attribute bytes describe the
  * 8px quadrants. Solid80 is shown; other tags retain their unknown semantics. */
 unsigned maxx=rooms[room].w/16,maxy=rooms[room].h/16,defs=rd24(r+0x78);
 for(int dy=-5;dy<=5;dy++)for(int dx=-6;dx<=6;dx++){
  int cx=(int)(x/16)+dx,cy=(int)(y/16)+dy;if(cx<0||cy<0||cx>=(int)maxx||cy>=(int)maxy)continue;
  unsigned i=cy*128+cx,meta=r[0x14000+i],solid=0,good=1;
  for(unsigned q=0;q<4;q++){unsigned v;if(!collision(defs,meta,q,&v)){good=0;break;}solid+=(v&0x80)!=0;}
  if(!good)continue;
  unsigned value=(cells[room][i]&0x80)|(solid==4?2:solid?3:1);
  if(!dx&&!dy)value|=0x80;
  if(value!=cells[room][i]){cells[room][i]=value;dirty=1;}
 }
}
static void browse(int dir){if(selected>=75)return;unsigned at=selected;for(unsigned i=0;i<75;i++){at=(at+75+dir)%75;if(visited(at)){selected=at;return;}}}
void LagoonMap_Command(unsigned cmd){
 if(cmd==LAGOON_MAP_TOGGLE){if(open){open=0;return;}if(current<75){pending=1;}return;}
 if(!open)return;
 if(cmd==LAGOON_MAP_CLOSE)open=0;
 else if(cmd==LAGOON_MAP_PREVIOUS)browse(-1);
 else if(cmd==LAGOON_MAP_NEXT)browse(1);
 else if(cmd==LAGOON_MAP_HOME)selected=current;
}
int LagoonMap_IsOpen(void){return open!=0;}
int LagoonMap_PollOpen(uint32_t human){
 prior_human=human; /* Map opener is the host L2 trigger or M key. */
 if(pending&&current<75){pending=0;open=1;selected=current;prior_nav=human;selftest_pumps=0;fprintf(stderr,"[lagoon_map] open room%02X position%u,%u\n",current,px,py);}
 return open;
}
void LagoonMap_Tick(uint32_t human,uint32_t ticks){
 (void)ticks;uint32_t edge=human&~prior_nav;prior_nav=human;
 if(edge&SNES_PAD_B)open=0;
 if(edge&(SNES_PAD_LEFT|SNES_PAD_L))browse(-1);
 if(edge&(SNES_PAD_RIGHT|SNES_PAD_R))browse(1);
 if(edge&SNES_PAD_Y)selected=current;
 if(selftest_opened&&++selftest_pumps==12){open=0;selftest_opened=0;}
 if(!open)LagoonMap_Save();
}
void LagoonMap_AfterFrame(unsigned frame){
 if(selftest_frame==-2){const char *v=getenv("LAGOON_MAP_SELFTEST_FRAME");selftest_frame=v?atoi(v):-1;}
 if(selftest_frame>=0&&frame>=(unsigned)selftest_frame&&current<75){selftest_frame=-1;selftest_opened=1;LagoonMap_Command(LAGOON_MAP_TOGGLE);}
}
static void rect(int x,int y,int w,int h,uint32_t c){snes_ovl_fill_rect(panel,MW,MH,x,y,w,h,c);}
static void text(int x,int y,const char *s,uint32_t c){snes_ovl_draw_text(panel,MW,MH,x,y,s,c,1);}
static const char *area(unsigned r){return r<75&&rooms[r].name[0]?rooms[r].name:"EXPLORED AREA";}
int LagoonMap_Image(const uint32_t **image,int *width,int *height){
 if(!open||selected>=75)return 0;
 for(unsigned i=0;i<MW*MH;i++)panel[i]=BG;
 rect(0,0,MW,4,TEAL);text(24,22,"LAGOON  /  FIELD NOTES",TEAL);
 char label[80];snprintf(label,sizeof(label),"%s   -   AREA %02X",area(selected),selected);text(24,46,label,FG);
 text(24,65,selected==current?"CURRENT LOCATION":"PREVIOUSLY EXPLORED",DIM);
 const int bx=24,by=92,bw=416,bh=296;rect(bx-1,by-1,bw+2,bh+2,0xff314357u);rect(bx,by,bw,bh,0xff0b121cu);
 unsigned cw=rooms[selected].w/16,ch=rooms[selected].h/16;
 double scale=(double)bw/cw;if((double)bh/ch<scale)scale=(double)bh/ch;
 int ox=bx+(bw-(int)(cw*scale))/2,oy=by+(bh-(int)(ch*scale))/2;
 unsigned seen=0;
 for(unsigned y=0;y<ch;y++)for(unsigned x=0;x<cw;x++){
  unsigned v=cells[selected][y*128+x];if(!v)continue;seen++;
  int x0=ox+(int)(x*scale),y0=oy+(int)(y*scale),x1=ox+(int)((x+1)*scale),y1=oy+(int)((y+1)*scale);
  uint32_t col=(v&3)==2?0xff5b6b7cu:(v&3)==3?0xff3d6270u:0xff28444fu;
  if(v&0x80)col=0xff4fa793u;rect(x0,y0,x1-x0,y1-y0,col);
 }
 unsigned known=0;
 for(unsigned i=0;i<rooms[selected].count;i++){
  MapDoor *d=rooms[selected].door+i;
  if(!cells[selected][(d->y/16)*128+d->x/16])continue;known++;
  int x=ox+(int)(d->x*scale/16),y=oy+(int)(d->y*scale/16);rect(x-3,y-3,7,7,GOLD);rect(x-1,y-1,3,3,BG);
 }
 unsigned treasure_seen=0;
 for(unsigned i=0;i<chest_count;i++){
  MapChest *c=chests+i;if(c->room!=selected||!chest_seen(c))continue;
  treasure_seen++;int x=ox+(int)(c->x*scale/16),y=oy+(int)(c->y*scale/16);
  rect(x-3,y-2,7,5,c->opened?DIM:GOLD);rect(x-2,y-1,5,3,c->opened?BG:GOLD);rect(x,y,1,1,BG);
 }
 LagoonQuestGuidance objective;int has_objective=LagoonQuestGuidance_Get(g_ram,&objective);
 if(has_objective&&objective.marker&&objective.room==selected){
  int x=ox+(int)(objective.x*scale/16),y=oy+(int)(objective.y*scale/16);
  snes_ovl_fill_disc(panel,MW,MH,x,y,5,0xffda8becu);rect(x-1,y-3,3,7,BG);
 }
 if(selected==current){int x=ox+(int)(px*scale/16),y=oy+(int)(py*scale/16);
  snes_ovl_fill_disc(panel,MW,MH,x,y,5,0xff101a28u);snes_ovl_fill_disc(panel,MW,MH,x,y,3,0xffffffffu);}
 text(464,98,"LEGEND",TEAL);rect(464,123,7,7,0xffffffff);text(480,122,"NASIR",FG);
 rect(464,147,7,7,GOLD);text(480,146,"EXIT",FG);rect(464,171,7,7,0xff4fa793);text(480,170,"TRAIL",FG);
 rect(464,195,7,7,0xff5b6b7c);text(480,194,"SOLID",FG);
 rect(464,218,7,5,GOLD);text(476,216,"CHEST",FG);rect(524,218,7,5,DIM);rect(525,219,5,3,BG);text(536,216,"OPENED",FG);
 snprintf(label,sizeof(label),"%u EXITS SEEN",known);text(464,232,label,DIM);
 snprintf(label,sizeof(label),"%u%% EXPLORED",cw*ch?seen*100/(cw*ch):0);text(464,250,label,DIM);
 snprintf(label,sizeof(label),"%u TREASURES SEEN",treasure_seen);text(464,268,label,DIM);
 text(464,280,"EXIT LINKS",TEAL);
 unsigned listed=0;uint8_t dest_seen[75]={0};
 for(unsigned i=0;i<rooms[selected].count&&listed<4;i++){
  MapDoor *d=rooms[selected].door+i;
  if(!cells[selected][(d->y/16)*128+d->x/16]||dest_seen[d->room])continue;
  dest_seen[d->room]=1;
  if(visited(d->room))snprintf(label,sizeof(label),"%02X %.14s",d->room,area(d->room));
  else snprintf(label,sizeof(label),"?? UNEXPLORED");
  text(464,300+listed*14,label,FG);listed++;
 }
 if(!listed)text(464,300,"NONE DISCOVERED",DIM);
 text(464,365,"EXITS MAY BE",DIM);text(464,379,"STORY LOCKED",DIM);
 if(has_objective){
  text(24,405,objective.title,0xffda8becu);
  const char *p=objective.detail;char line[73];unsigned row=0;
  while(*p&&row<3){unsigned n=0;while(p[n]&&n<72)n++;
   if(p[n]){unsigned b=n;while(b&&p[b]!=' ')b--;if(b)n=b;}
   memcpy(line,p,n);line[n]=0;text(24,423+row*14,line,FG);p+=n;while(*p==' ')p++;row++;
  }
 }else text(24,405,"NO ACTIVE TABLET OBJECTIVE",DIM);
 text(24,480,"LEFT/RIGHT: AREAS   Y: NASIR   M / L2 / B / ESC: CLOSE",FG);
 text(24,500,"L2: MAP   R2 / CTRL+COMMA: SETTINGS   -   AUTO-SAVED EXPLORATION",DIM);
 *image=panel;*width=MW;*height=MH;return 1;
}
