/* A small credited visitor, entirely outside the guest's actor/script system.
 * The guest RAM/ROM, OAM, collision and story remain read-only. The completed
 * raster witness is independent of widescreen being enabled. */
#include "lagoon_easter_egg.h"
#include "lagoon_dialogue_font.h"
#include "snes/ppu.h"
#include "snes_overlay_draw.h"
#include "../assets/easter_egg/crono_dance.generated.h"
#include "../assets/easter_egg/crono_portrait.generated.h"
#include <string.h>

/* Open ground east of the house: source collision alone also permits its
 * roof, so the live-scene visual proof fixes this path between wall and bank. */
enum { ROOM=0, X=1232, Y=1008, SIDE=1536, PW=512, PH=448 };
typedef struct Raster {
  uint64_t generation;
  unsigned lines, valid;
  int x,y;
  uint16_t camera_x,camera_y;
  uint8_t foreground[20*35];
} Raster;
static struct {
  const uint8_t *rom;
  size_t size;
  uint8_t descriptor[56];
  uint64_t generation,discovery_generation,shown_generation;
  unsigned qualified,have_generation,valid,near,opened,page,primed,pending,visible;
  uint32_t prior_world_input,prior_dialogue_input;
  uint32_t typed_at;
  unsigned revealed,typing_started;
  Raster build,complete;
} s;
static uint32_t panel[PW*PH];

static unsigned word(const uint8_t *r,unsigned a){return r[a]|r[a+1]<<8;}
static unsigned pointer(const uint8_t *r,unsigned a){return word(r,a)|r[a+2]<<16;}
static unsigned distance(unsigned a,unsigned b){return a>b?a-b:b-a;}
static uint64_t source_hash(const uint8_t *rom,size_t size,unsigned pc,unsigned bytes){
  size_t at=((pc>>16)&127)*32768+(pc&32767);
  uint64_t h=UINT64_C(14695981039346656037);
  if(!rom||!(pc&0x8000)||at>size||bytes>size-at)return 0;
  while(bytes--)h=(h^rom[at++])*UINT64_C(1099511628211);
  return h;
}
static void select_generation(uint64_t generation){
  if(!s.have_generation||s.generation!=generation){
    s.generation=generation;s.have_generation=1;
    s.valid=s.near=s.opened=s.page=s.primed=s.pending=s.visible=0;
    s.prior_world_input=s.prior_dialogue_input=0;
    memset(&s.build,0,sizeof s.build);memset(&s.complete,0,sizeof s.complete);
  }
}
void LagoonEasterEgg_Init(const uint8_t *rom,size_t size){
  memset(&s,0,sizeof s);s.rom=rom;s.size=size;
  LagoonDialogueFont_Init(rom,size);
  if(size!=0x100000||
     source_hash(rom,size,0x05b2df,56)!=UINT64_C(0xab072f6fd13163b3)||
     source_hash(rom,size,0x018385,83)!=UINT64_C(0xaa6815d0b23960aa)||
     source_hash(rom,size,0x038000,1024)!=UINT64_C(0x15815a7cc1a35507))return;
  memcpy(s.descriptor,rom+5*32768+(0xb2df&32767),56);s.qualified=1;
}
static int ordinary_world(const uint8_t *r){
  return s.qualified&&r&&word(r,0x48e)==ROOM&&!r[0x100e]&&
    !memcmp(r+0x50,s.descriptor,56)&&
    (r[0x500]&0xc1)==0xc0&&!(r[0x501]&0xc3)&&!r[0x41b]&&r[0x417]<=1&&
    word(r,0x520)&&word(r,0x502)<SIDE&&word(r,0x504)<SIDE&&
    pointer(r,0x88)==0x7f0000&&pointer(r,0x8b)==0x7f4000;
}
static int read_source(const uint8_t *r,unsigned a,unsigned *v){
  unsigned bank=(a>>16)&127,adr=a&65535;
  if(bank==126||bank==127){*v=r[(bank-126)*65536+adr];return 1;}
  size_t at=bank*32768+(adr&32767);
  if(bank<32&&adr>=32768&&at<s.size){*v=s.rom[at];return 1;}
  return 0;
}
static int walkable(const uint8_t *r,int x,int y){
  if(x<0||y<0||x>=SIDE||y>=SIDE)return 0;
  unsigned meta,attr,map=pointer(r,0x8b),defs=pointer(r,0x78);
  unsigned index=((unsigned)y&0xfff0)*8+((unsigned)x>>4);
  if(!read_source(r,map+index,&meta)||
     !read_source(r,defs+meta*4+((x&8)>>3)+((y&8)>>2),&attr))return 0;
  return !(attr&128)&&((attr&15)==0||(attr&15)==3||(attr&15)==5);
}
static int clear_site(const uint8_t *r){
  /* Source collision probes cover the feet and each approach. Nothing here
   * authorizes movement; a changed/blocked site simply hides the visitor. */
  static const int8_t points[][2]={{-10,-9},{0,-9},{10,-9},{-10,0},{0,0},
    {10,0},{-10,9},{0,9},{10,9},{-24,0},{24,0},{0,-24},{0,24}};
  for(unsigned i=0;i<sizeof points/sizeof *points;i++)
    if(!walkable(r,X+points[i][0],Y+points[i][1]))return 0;
  /* Do not cover or steal interaction from an original live NPC or doorway. */
  for(unsigned a=0x540;a<=0xd00;a+=64)
    if((r[a]&0xc0)==0xc0&&distance(word(r,a+2),X)<64&&distance(word(r,a+4),Y)<64)return 0;
  return 1;
}
int LagoonEasterEgg_CollisionBody(const uint8_t *r,uint64_t generation,int *x,int *y){
  if(!s.have_generation||generation!=s.generation||!LagoonDialogueFont_Ready()||
     !ordinary_world(r)||!clear_site(r))return 0;
  if(x)*x=X;if(y)*y=Y;return 1;
}
static int facing_site(const uint8_t *r){
  int dx=X-(int)word(r,0x502),dy=Y-(int)word(r,0x504);
  if(word(r,0x514)||r[0x50a]>3)return 0;
  if(dx>=-10&&dx<=10&&dy>=-10&&dy<=10)return 1;
  switch(r[0x50a]){
    case 0:return dx>0&&dx<=36&&dy>=-18&&dy<=18;
    case 1:return dy>0&&dy<=36&&dx>=-18&&dx<=18;
    case 2:return dx<0&&dx>=-36&&dy>=-18&&dy<=18;
    case 3:return dy<0&&dy>=-36&&dx>=-18&&dx<=18;
  }
  return 0;
}
void LagoonEasterEgg_Observe(const uint8_t *r,size_t size,uint64_t generation,int ready){
  select_generation(generation);
  s.valid=ready&&r&&size>=0x20000&&ordinary_world(r)&&clear_site(r);
  s.near=s.valid&&facing_site(r);
  if(!s.valid){s.opened=s.page=s.primed=s.visible=0;}
}
void LagoonEasterEgg_BeginFrame(uint64_t generation){
  select_generation(generation);memset(&s.build,0,sizeof s.build);s.build.generation=generation;
}
static int raster_supported(const Ppu *p){
  return p&&p->inidisp==15&&!p->mosaic&&p->bgmode==9&&p->obsel==2&&!p->setini&&
    p->bgXsc[0]==0x71&&p->bgXsc[1]==0x79&&p->bgXsc[2]==0x3c&&p->bgTileAdr==0x600&&
    !p->cgadsub&&!(p->cgwsel&1)&&!p->windowsel&&!p->screenWindowed[0]&&!p->screenWindowed[1]&&
    p->screenEnabled[0]==31&&p->hScroll[0]==p->hScroll[1]&&p->vScroll[0]==p->vScroll[1]&&
    !p->hScroll[2]&&!p->vScroll[2]&&!(p->oamaddh&0x80);
}
/* Native depth order: when Nasir stands in front, retain opaque pixels from
 * his actual live OBJ pack instead of painting the visitor over his head.
 * Word4000..423F is the original36-tile player pack, separate from town actors. */
static void player_foreground(const Ppu *p,const uint8_t *r,Raster *field){
  if(word(r,0x504)<=Y)return;
  int left=X-field->x-10,top=Y-field->y-35;
  for(unsigned n=0;n<128;n++){
    unsigned high=(p->highOam[n/4]>>((n&3)*2))&3,tag=p->oam[n*2+1];
    if((tag&0x100)||(tag&255)>=36)continue;
    int size=(high&2)?16:8;
    int ox=(p->oam[n*2]&255)|((high&1)<<8),oy=p->oam[n*2]>>8;
    if(ox>=256)ox-=512;if(oy>=224)oy-=256;
    if(ox>=left+20||ox+size<=left||oy>=top+35||oy+size<=top)continue;
    for(int yy=0;yy<size;yy++)for(int xx=0;xx<size;xx++){
      int dx=ox+xx-left,dy=oy+yy-top;if(dx<0||dx>=20||dy<0||dy>=35)continue;
      unsigned sx=(tag&0x4000)?size-1-xx:xx,sy=(tag&0x8000)?size-1-yy:yy;
      unsigned tile=((((tag&255)>>4)+(sy>>3))<<4)|(((tag&15)+(sx>>3))&15);
      if(tile>=36)continue;
      unsigned at=0x4000+tile*16+(sy&7),bit=7-(sx&7);
      uint32_t planes=p->vram[at]|(uint32_t)p->vram[at+8]<<16;
      unsigned color=((planes>>bit)&1)|((planes>>(bit+7))&2)|
        ((planes>>(bit+14))&4)|((planes>>(bit+21))&8);
      if(color)field->foreground[dy*20+dx]=1;
    }
  }
}
void LagoonEasterEgg_ObserveRaster(const Ppu *p,unsigned line,const uint8_t *r,
    uint64_t generation,int hdma_active){
  if(line<1||line>224)return;
  if(generation!=s.generation||generation!=s.build.generation||line!=s.build.lines+1||
     hdma_active||!ordinary_world(r)||!raster_supported(p)){
    s.build.valid=0;s.build.lines=225;return;
  }
  int x=(int)word(r,0x480),y=(int)word(r,0x482);
  x+=(((int)(p->hScroll[0]&1023)-x+512)&1023)-512;
  y+=(((int)(p->vScroll[0]&1023)-y+512)&1023)-512+1;
  if(line==1){
    s.build.valid=clear_site(r);LagoonDialogueFont_Capture(p);s.build.x=x;s.build.y=y;
    player_foreground(p,r,&s.build);
    s.build.camera_x=(uint16_t)word(r,0x480);s.build.camera_y=(uint16_t)word(r,0x482);
  }else if(x!=s.build.x||y!=s.build.y||word(r,0x480)!=s.build.camera_x||word(r,0x482)!=s.build.camera_y){
    s.build.valid=0;
  }
  s.build.lines++;
}
void LagoonEasterEgg_EndFrame(uint64_t generation){
  /* A fresh completed field needs its own successful presentation. */
  s.visible=0;
  if(s.build.lines==224&&s.build.valid&&generation==s.generation&&generation==s.build.generation)s.complete=s.build;
  else memset(&s.complete,0,sizeof s.complete);
}
int LagoonEasterEgg_Draw(uint8_t *dst,size_t pitch,unsigned width,unsigned height,
    uint32_t ticks,uint64_t generation){
  s.visible=0;
  if(!dst||width<256||width>800||(width&1)||height!=224||pitch<width*4||pitch%4||pitch>64000||
     !LagoonDialogueFont_Ready()||!s.valid||!s.complete.valid||generation!=s.generation||generation!=s.complete.generation)return 0;
  int left=(int)(width-256)/2+X-s.complete.x-10,top=Y-s.complete.y-35;
  if(left>= (int)width||left+20<=0||top>=224||top+35<=0)return 0;
  static const uint8_t steps[]={0,2,1,2};unsigned frame=steps[(ticks/180)%4],painted=0;
  for(int y=0;y<35;y++)for(int x=0;x<20;x++){
    unsigned index=cameo_frames[frame][y*20+x];int dx=left+x,dy=top+y;
    if(index&&!s.complete.foreground[y*20+x]&&dx>=0&&dx<(int)width&&dy>=0&&dy<224){
      ((uint32_t*)(dst+(size_t)dy*pitch))[dx]=cameo_palette[index];painted++;
    }
  }
  if(painted){s.visible=1;s.shown_generation=generation;}
  return painted!=0;
}
int LagoonEasterEgg_TryInteract(uint32_t human){
  uint32_t edge=human&~s.prior_world_input;s.prior_world_input=human;
  if(s.opened||!LagoonDialogueFont_Ready()||!s.valid||!s.near||!s.visible||s.shown_generation!=s.generation||!s.complete.valid||
     s.complete.generation!=s.generation||!(edge&SNES_PAD_Y))return 0;
  s.opened=1;s.page=0;s.primed=0;s.revealed=s.typing_started=0;
  s.prior_dialogue_input=human;return 1;
}
int LagoonEasterEgg_IsOpen(void){return s.opened!=0;}
int LagoonEasterEgg_PollOpen(uint32_t human){
  if(s.opened&&!s.primed){s.prior_dialogue_input=human;s.primed=1;}
  return s.opened!=0;
}
void LagoonEasterEgg_Close(void){s.opened=s.page=s.primed=0;}
static const char *const dialogue[]={
  "...\nI finally get a\nspeaking part...\nand Carl puts me in\nthe wrong RPG.",
  "I was looking for\nLucca and the\nMillennial Fair.\nWait... wrong game!\n\nPortal maintenance:\nrages4calm\nNow please let me\nfinish my dance."
};
void LagoonEasterEgg_Tick(uint32_t human,uint32_t ticks){
  if(!s.opened)return;
  if(!s.typing_started){s.typing_started=1;s.typed_at=ticks;}
  unsigned length=(unsigned)strlen(dialogue[s.page]);
  /* Original NPC dialogue reveals one character every three NTSC fields. */
  unsigned count=(ticks-s.typed_at)/50;
  if(count>s.revealed)s.revealed=count>length?length:count;
  if(!s.primed){s.prior_dialogue_input=human;s.primed=1;return;}
  uint32_t edge=human&~s.prior_dialogue_input;s.prior_dialogue_input=human;
  if(edge&SNES_PAD_B){LagoonEasterEgg_Close();return;}
  if(edge&(SNES_PAD_A|SNES_PAD_Y)){
    if(s.revealed<length){s.revealed=length;return;}
    if(s.page==0){s.page=1;s.revealed=0;s.typed_at=ticks;return;}
    s.pending=1;s.discovery_generation=s.generation;LagoonEasterEgg_Close();
  }
}
int LagoonEasterEgg_TakeDiscovery(uint64_t *generation){
  if(!s.pending||s.discovery_generation!=s.generation)return 0;
  if(generation)*generation=s.discovery_generation;s.pending=0;return 1;
}
int LagoonEasterEgg_Image(const uint32_t **pixels,int *width,int *height){
  if(!s.opened)return 0;
  /* Transparent full-field canvas preserves the actual frozen wide scenery.
   * These are the game's original resident font and border tiles, read once
   * at a qualified ordinary Atland raster; no guest script or tilemap writes. */
  memset(panel,0,sizeof panel);
  LagoonDialogueFont_DrawBox(panel,PW,PH,144,0,368,64,2);
  LagoonDialogueFont_DrawBox(panel,PW,PH,144,64,368,384,2);
  LagoonDialogueFont_DrawBox(panel,PW,PH,0,80,128,128,2);
  const char *name="Crono";
  for(unsigned i=0;name[i];i++)LagoonDialogueFont_DrawChar(panel,PW,PH,160+i*16,16,name[i],2);
  for(unsigned y=0;y<48;y++)for(unsigned x=0;x<48;x++){
    uint32_t color=cameo_portrait_palette[cameo_portrait[y*48+x]];
    for(unsigned sy=0;sy<2;sy++)for(unsigned sx=0;sx<2;sx++)
      panel[(96+y*2+sy)*PW+16+x*2+sx]=color;
  }
  int x=160,y=88;
  const char *text=dialogue[s.page];
  for(unsigned i=0;text[i]&&i<s.revealed;i++){
    if(text[i]=='\n'){x=160;y+=32;continue;}
    LagoonDialogueFont_DrawChar(panel,PW,PH,x,y,text[i],2);x+=16;
  }
  if(s.revealed>=strlen(text)){
    /* Same quiet white continuation cue as the native text window. */
    snes_ovl_fill_rect(panel,PW,PH,480,426,12,2,0xffffffff);
    snes_ovl_fill_rect(panel,PW,PH,482,428,8,2,0xffffffff);
    snes_ovl_fill_rect(panel,PW,PH,484,430,4,2,0xffffffff);
  }
  if(pixels)*pixels=panel;if(width)*width=PW;if(height)*height=PH;return 1;
}
