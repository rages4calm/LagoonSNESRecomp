/* Explored map from original room/door descriptors and observed collision
 * attributes. Source:018B84/01C87B/018385. Static exits are landmarks, never
 * authority to unlock a door or declare an unexplored destination visited. */
#include "lagoon_map.h"
#include "lagoon_scene_renderer.h"
#include "lagoon_quest_guidance.h"
#include "common_rtl.h"
#include "snes_overlay_draw.h"
#include "lagoon_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
#define MW 960
#define MH 540
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
static uint32_t prior_human,prior_nav,nav_repeat;static uint64_t last_generation;
static unsigned zoom,exit_page;static double center_x,center_y;
static void fit_view(void){zoom=exit_page=0;if(selected<75){center_x=rooms[selected].w/2.0;center_y=rooms[selected].h/2.0;}}
static double map_scale(void){double a=668.0/rooms[selected].w,b=372.0/rooms[selected].h;return (a<b?a:b)*(1u<<zoom);}
static void clamp_view(void){if(selected>=75)return;double scale=map_scale(),hw=334.0/scale,hh=186.0/scale;
 double w=rooms[selected].w,h=rooms[selected].h;
 if(hw*2>=w)center_x=w/2;else {if(center_x<hw)center_x=hw;if(center_x>w-hw)center_x=w-hw;}
 if(hh*2>=h)center_y=h/2;else {if(center_y<hh)center_y=hh;if(center_y>h-hh)center_y=h-hh;}}
static char save_path[1024],scenery_path[1024];static int save_loaded,save_invalid;
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
 if(root&&snprintf(scenery_path,sizeof scenery_path,"%s/lagoon-scenery.maps",root)>=(int)sizeof(scenery_path))scenery_path[0]=0;
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
 /* Terrain persistence is independent of exploration changes. Its archive
  * performs no I/O during live fields, only here at modal close/normal quit. */
 if(scenery_path[0])LagoonSceneRenderer_ArchiveSave(scenery_path);
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
 LagoonSceneRenderer_ArchiveOnRomInit(rom,size);
 source=rom;source_size=size;available=open=pending=0;current=selected=75;zoom=0;center_x=center_y=0;nav_repeat=0;
 memset(rooms,0,sizeof(rooms));memset(cells,0,sizeof(cells));
 memset(chests,0,sizeof(chests));chest_count=0;
 save_loaded=save_invalid=dirty=0;save_path[0]=scenery_path[0]=0;prior_human=prior_nav=0;
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
static void browse(int dir){if(selected>=75)return;unsigned at=selected;for(unsigned i=0;i<75;i++){at=(at+75+dir)%75;if(visited(at)){selected=at;fit_view();return;}}}
void LagoonMap_Command(unsigned cmd){
 if(cmd==LAGOON_MAP_TOGGLE){if(open){open=0;LagoonMap_Save();return;}if(current<75)pending=1;return;}
 if(!open)return;
 if(cmd==LAGOON_MAP_CLOSE){open=0;LagoonMap_Save();return;}
 if(cmd==LAGOON_MAP_PREVIOUS){browse(-1);return;}
 if(cmd==LAGOON_MAP_NEXT){browse(1);return;}
 if(cmd==LAGOON_MAP_NEXT_EXITS){exit_page++;return;}
 if(cmd==LAGOON_MAP_HOME){if(current<75){selected=current;center_x=px;center_y=py;}}
 else if(cmd==LAGOON_MAP_FIT)fit_view();
 else if(cmd==LAGOON_MAP_ZOOM_IN){if(zoom<3)zoom++;}
 else if(cmd==LAGOON_MAP_ZOOM_OUT){if(zoom)zoom--;}
 else {double step=24.0/map_scale();
  if(cmd==LAGOON_MAP_PAN_LEFT)center_x-=step;
  if(cmd==LAGOON_MAP_PAN_RIGHT)center_x+=step;
  if(cmd==LAGOON_MAP_PAN_UP)center_y-=step;
  if(cmd==LAGOON_MAP_PAN_DOWN)center_y+=step;
 }
 clamp_view();
}
int LagoonMap_IsOpen(void){return open!=0;}
int LagoonMap_PollOpen(uint32_t human){
 prior_human=human;
 if(pending&&current<75){pending=0;open=1;selected=current;prior_nav=human;nav_repeat=0;fit_view();selftest_pumps=0;
  if(scenery_path[0])LagoonSceneRenderer_ArchiveLoad(scenery_path);
  /* Optional integration diagnostic uses the real next-area command. It
   * changes only the frozen browser, never exploration or guest progress. */
  if(selftest_opened){const char *v=getenv("LAGOON_MAP_SELFTEST_BROWSE");unsigned steps=v?(unsigned)atoi(v):0;
   if(steps<75)while(steps--)LagoonMap_Command(LAGOON_MAP_NEXT);}
  fprintf(stderr,"[lagoon_map] open room%02X position%u,%u\n",current,px,py);}
 return open;
}
void LagoonMap_Tick(uint32_t human,uint32_t ticks){
 uint32_t edge=human&~prior_nav,nav=human&(SNES_PAD_LEFT|SNES_PAD_RIGHT|SNES_PAD_UP|SNES_PAD_DOWN);
 if(edge&nav)nav_repeat=ticks+250;
 else if(nav&&nav_repeat&&(int32_t)(ticks-nav_repeat)>=0){edge|=nav;nav_repeat=ticks+45;}
 if(!nav)nav_repeat=0;
 if(edge&SNES_PAD_B)LagoonMap_Command(LAGOON_MAP_CLOSE);
 if(edge&SNES_PAD_L)browse(-1);if(edge&SNES_PAD_R)browse(1);
 if(edge&SNES_PAD_LEFT)LagoonMap_Command(LAGOON_MAP_PAN_LEFT);
 if(edge&SNES_PAD_RIGHT)LagoonMap_Command(LAGOON_MAP_PAN_RIGHT);
 if(edge&SNES_PAD_UP)LagoonMap_Command(LAGOON_MAP_PAN_UP);
 if(edge&SNES_PAD_DOWN)LagoonMap_Command(LAGOON_MAP_PAN_DOWN);
 if(edge&SNES_PAD_A)LagoonMap_Command(LAGOON_MAP_ZOOM_IN);
 if(edge&SNES_PAD_X)LagoonMap_Command(LAGOON_MAP_ZOOM_OUT);
 if(edge&SNES_PAD_Y)LagoonMap_Command(LAGOON_MAP_HOME);
 if(edge&SNES_PAD_START)LagoonMap_Command(LAGOON_MAP_FIT);
 if(edge&SNES_PAD_SELECT)LagoonMap_Command(LAGOON_MAP_NEXT_EXITS);
 prior_nav=human;
 if(selftest_opened&&++selftest_pumps==12){open=0;selftest_opened=0;}
 if(!open)LagoonMap_Save();
}
void LagoonMap_AfterFrame(unsigned frame){
 if(selftest_frame==-2){const char *v=getenv("LAGOON_MAP_SELFTEST_FRAME");selftest_frame=v?atoi(v):-1;}
 if(selftest_frame>=0&&frame>=(unsigned)selftest_frame&&current<75){selftest_frame=-1;selftest_opened=1;LagoonMap_Command(LAGOON_MAP_TOGGLE);}
}
static void rect(int x,int y,int w,int h,uint32_t c){LagoonUI_Fill(panel,MW,MH,x,y,w,h,c);}
static void text(int x,int y,const char *s,uint32_t c){LagoonUI_Text(panel,MW,MH,x,y,s,c,1);}
static const char *area(unsigned r){return r<75&&rooms[r].name[0]?rooms[r].name:"EXPLORED AREA";}
static void area_title(unsigned r,char *out,size_t cap){
 const char *p=area(r);unsigned n=0,start=1;while(*p&&n+1<cap){char c=*p++;if(!start&&c>='A'&&c<='Z')c+=32;out[n++]=c;start=c==' ';}out[n]=0;
}
static int marker(double x,double y,double scale,int *sx,int *sy){
 *sx=354+(int)((x-center_x)*scale);*sy=270+(int)((y-center_y)*scale);
 return *sx>=26&&*sx<682&&*sy>=90&&*sy<450;
}
static void door_icon(int x,int y,uint32_t c){rect(x-4,y-5,9,11,0xff080808);rect(x-3,y-4,7,9,c);rect(x-1,y-2,3,7,0xff080808);}
int LagoonMap_Image(const uint32_t **image,int *width,int *height){
 if(!open||selected>=75)return 0;
 const uint32_t ink=0xfff5f1df,dim=0xffbdb8a7,gold=0xffedc977,dark=0xff080808;
 for(unsigned i=0;i<MW*MH;i++)panel[i]=dark;
 LagoonUI_Frame(panel,MW,MH,12,12,936,52);
 char label[100],name[40];area_title(selected,name,sizeof name);LagoonUI_TextFit(panel,MW,MH,30,18,name,ink,2,650);
 text(714,28,"Area map",gold);
 LagoonUI_Frame(panel,MW,MH,12,76,684,388);LagoonUI_Frame(panel,MW,MH,708,76,240,388);
 const uint32_t *terrain=NULL;unsigned tw=0,th=0,ww=0,wh=0;
 int scenery=LagoonSceneRenderer_MapImage(selected,last_generation,&terrain,&tw,&th,&ww,&wh);
 int last_seen=0;
 if(!scenery)last_seen=scenery=LagoonSceneRenderer_VisitedMapImage(selected,&terrain,&tw,&th,&ww,&wh);
 if(scenery&&(ww!=rooms[selected].w||wh!=rooms[selected].h))scenery=0;
 if(selftest_opened&&!selftest_pumps)fprintf(stderr,"[lagoon_map] view current=%02X selected=%02X scenery=%d last_seen=%d size=%ux%u archived=%u\n",current,selected,scenery,last_seen,tw,th,LagoonSceneRenderer_ArchivedRoomCount());
 clamp_view();double scale=map_scale();
 unsigned cw=rooms[selected].w/16,ch=rooms[selected].h/16,seen=0;
 for(unsigned y=0;y<ch;y++)for(unsigned x=0;x<cw;x++)seen+=cells[selected][y*128+x]!=0;
 if(scenery){
  for(int y=84;y<456;y++){
   double wy=center_y+(y-270)/scale;if(wy<0||wy>=wh)continue;unsigned ty=(unsigned)(wy*th/wh);
   for(int x=20;x<688;x++){
    double wx=center_x+(x-354)/scale;if(wx<0||wx>=ww)continue;unsigned tx=(unsigned)(wx*tw/ww);
    panel[y*MW+x]=terrain[ty*tw+tx];
   }
  }
 }else{
  for(unsigned y=0;y<ch;y++)for(unsigned x=0;x<cw;x++){
   unsigned v=cells[selected][y*128+x];if(!v)continue;
   int x0=354+(int)((x*16-center_x)*scale),y0=270+(int)((y*16-center_y)*scale);
   int x1=354+(int)(((x+1)*16-center_x)*scale),y1=270+(int)(((y+1)*16-center_y)*scale);
   if(x0<20)x0=20;if(y0<84)y0=84;if(x1>688)x1=688;if(y1>456)y1=456;
   uint32_t col=(v&3)==2?0xff777466:(v&3)==3?0xff4e6656:0xff293d2e;
   if(x1>x0&&y1>y0)rect(x0,y0,x1-x0,y1-y0,col);
  }
 }
 unsigned exit_count=0;uint8_t exit_number[75]={0};
 for(unsigned i=0;i<rooms[selected].count;i++){MapDoor*d=&rooms[selected].door[i];
  if(cells[selected][(d->y/16)*128+d->x/16]&&!exit_number[d->room])exit_number[d->room]=(uint8_t)++exit_count;
 }
 int sx,sy;
 for(unsigned i=0;i<rooms[selected].count;i++){
  MapDoor *d=&rooms[selected].door[i];int found=cells[selected][(d->y/16)*128+d->x/16]!=0;
  if((scenery||found)&&marker(d->x,d->y,scale,&sx,&sy)){
   door_icon(sx,sy,gold);
   if(found&&sx+27<688&&sy+15<456){snprintf(label,sizeof label,"%u",exit_number[d->room]);rect(sx+6,sy-5,LagoonUI_TextWidth(label,1)+5,20,dark);text(sx+9,sy-5,label,gold);}
  }
 }
 unsigned treasure_seen=0;
 for(unsigned i=0;i<chest_count;i++){
  MapChest *c=&chests[i];if(c->room!=selected||!chest_seen(c))continue;treasure_seen++;
  if(marker(c->x,c->y,scale,&sx,&sy)){rect(sx-5,sy-4,11,9,dark);rect(sx-4,sy-3,9,7,c->opened?dim:gold);rect(sx-4,sy-1,9,1,dark);}
 }
 LagoonQuestGuidance objective;int has_objective=LagoonQuestGuidance_Get(g_ram,&objective);
 if(has_objective&&objective.marker&&objective.room==selected&&marker(objective.x,objective.y,scale,&sx,&sy)){
  snes_ovl_fill_disc(panel,MW,MH,sx,sy,7,dark);snes_ovl_fill_disc(panel,MW,MH,sx,sy,5,0xffd9a7e8);
 }
 if(selected==current&&marker(px,py,scale,&sx,&sy)){
  snes_ovl_fill_disc(panel,MW,MH,sx,sy,7,dark);snes_ovl_fill_disc(panel,MW,MH,sx,sy,4,ink);
  rect(sx-1,sy-8,3,3,ink);
 }
 text(728,91,scenery?"Scenery map":"Explored layout",gold);
 if(!scenery)text(728,115,"Scenery unavailable",dim);
 else if(last_seen)text(728,115,"Last mapped visit",dim);
 int ly=scenery&&!last_seen?126:150;
 rect(730,ly+6,7,7,ink);text(750,ly,"Nasir",ink);
 door_icon(733,ly+37,gold);text(750,ly+28,"Door / exit",ink);
 rect(729,ly+63,9,7,gold);text(750,ly+56,"Treasure found",ink);
 snprintf(label,sizeof label,"%u%% explored",cw*ch?seen*100/(cw*ch):0);text(728,ly+91,label,dim);
 snprintf(label,sizeof label,"%u treasures seen",treasure_seen);text(728,ly+113,label,dim);
 text(728,ly+148,"Discovered exits",gold);
 unsigned pages=(exit_count+2)/3;if(pages)exit_page%=pages;else exit_page=0;
 unsigned first=exit_page*3,listed=0;
 for(unsigned n=first+1;n<=exit_count&&listed<3;n++)for(unsigned destination=0;destination<75;destination++){
  if(exit_number[destination]!=n)continue;
  if(visited(destination))area_title(destination,name,sizeof name);else snprintf(name,sizeof name,"Unexplored area");
  snprintf(label,sizeof label,"%u. %s",n,name);
  LagoonUI_TextFit(panel,MW,MH,728,ly+172+listed*22,label,ink,1,202);listed++;
 }
 if(!listed)text(728,ly+172,"None discovered",dim);
 if(pages>1){snprintf(label,sizeof label,"Exits %u-%u of %u",first+1,first+listed,exit_count);text(728,390,label,dim);text(728,410,"Select / Tab: more",gold);}
 if(has_objective){LagoonUI_TextFit(panel,MW,MH,728,430,objective.title,0xffd9a7e8,1,200);}
 else if(scenery)text(728,430,"Exits may be locked",dim);
 LagoonUI_Frame(panel,MW,MH,12,476,936,52);
 text(28,483,"D-pad / arrows: pan    A / +: zoom in    X / -: zoom out    Y / Home: Nasir",ink);
 text(28,505,"L/R or [ ]: areas   Start/0: fit   Select/Tab: exits   M/L2/B/Esc: close",dim);
 if(image)*image=panel;if(width)*width=MW;if(height)*height=MH;return 1;
}
