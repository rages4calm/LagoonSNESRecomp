/* Presentation-only sword readability for the38px sword-reach option.
 * Original01C4EE stages four sword tiles25..28 from11:8000 throughF286;
 * 01C156 places their native2x2 OAM block throughE2D4. Never infer a weapon
 * from white framebuffer pixels, redraw Nasir, or modify a collision box. */
#include "lagoon_sword_visual.h"
#include "snes/ppu.h"
#include <string.h>

enum { OBJECT_CAP=512, SIDE=48, BLADE=16 };
static PpuExtraObject objects[OBJECT_CAP+1];
static uint16_t pixels[SIDE*SIDE];
static unsigned word(const uint8_t*r,unsigned p){return r[p]|r[p+1]<<8;}
static uint64_t source_hash(const uint8_t*r,size_t size,unsigned a,unsigned n){
  size_t p=(a>>16)*32768+(a&32767);uint64_t h=UINT64_C(14695981039346656037);
  if(!r||p>size||n>size-p)return 0;
  while(n--)h=(h^r[p++])*UINT64_C(1099511628211);return h;
}
static int source(const uint8_t*r,size_t size){
  return source_hash(r,size,0x01c156,37)==UINT64_C(0xb70a22a20810552f)&&
    source_hash(r,size,0x01c4ee,44)==UINT64_C(0x0dd2d8c1a353338c)&&
    source_hash(r,size,0x01c579,38)==UINT64_C(0xcb44fc378d8945b9)&&
    source_hash(r,size,0x01e364,48)==UINT64_C(0x9149159843c210af)&&
    source_hash(r,size,0x01f286,104)==UINT64_C(0xe5b98d29849c3ff3);
}
static int absolute(int v){return v<0?-v:v;}
static void dot(int x,int y,uint16_t c){if(x>=0&&y>=0&&x<SIDE&&y<SIDE)pixels[y*SIDE+x]=c;}
/* Retain the original blade and add a parallel edge plus12px beyond its tip.
 * The actual blade silhouette selects the stroke orientation, so windup and
 * finishing poses retain their authored angle rather than using facing alone. */
static int extend(const uint16_t blade[256],int hero_x,int hero_y,int*origin_x,int*origin_y){
  int ax=0,ay=0,tx=0,ty=0,best=100000,far=0,opaque=0;
  for(int y=0;y<BLADE;y++)for(int x=0;x<BLADE;x++)if(blade[y*BLADE+x]&0x8000){
    int dx=x-hero_x,dy=y-hero_y,d=dx*dx+dy*dy;opaque++;
    if(d<best){best=d;ax=x;ay=y;}
  }
  if(opaque<3||opaque>128)return 0;
  for(int y=0;y<BLADE;y++)for(int x=0;x<BLADE;x++)if(blade[y*BLADE+x]&0x8000){
    int dx=x-ax,dy=y-ay,d=dx*dx+dy*dy;
    if(d>far){far=d;tx=x;ty=y;}
  }
  if(far<9)return 0;
  int dx=tx-ax,dy=ty-ay,major=absolute(dx)>absolute(dy)?absolute(dx):absolute(dy);
  int ex=absolute(dx)>=absolute(dy)?0:1,ey=ex?0:1;
  memset(pixels,0,sizeof pixels);
  /* A48px canvas surrounds the16px source by16px on every side. */
  for(int y=0;y<BLADE;y++)for(int x=0;x<BLADE;x++)if(blade[y*BLADE+x]&0x8000)
    dot(x+16+ex,y+16+ey,blade[y*BLADE+x]);
  uint16_t color=blade[ty*BLADE+tx];
  for(int n=1;n<=12;n++){
    int x=tx+(dx*n+(dx<0?-major/2:major/2))/major;
    int y=ty+(dy*n+(dy<0?-major/2:major/2))/major;
    if(absolute(x-hero_x)>38||absolute(y-hero_y)>38)continue;
    dot(x+16,y+16,color);dot(x+16+ex,y+16+ey,color);
  }
  *origin_x=-16;*origin_y=-16;return 1;
}
static void remove_previous(Ppu*p){
  if(p&&p->extraObjects==objects&&p->extraObjectCount&&p->extraObjectCount<=OBJECT_CAP+1&&
    objects[p->extraObjectCount-1].pixels==pixels)
    PpuSetExtraObjects(p,objects,p->extraObjectCount-1);
}
int LagoonSwordVisual_Prepare(Ppu*p,const uint8_t*rom,size_t size,const uint8_t*r,int enabled){
  if(!p)return 0;remove_previous(p);
  if(!enabled||!rom||!r||!source(rom,size)||p->inidisp!=15||p->obsel!=2||
    PPU_objPriority(p)||PPU_mode(p)!=1||r[0x100e]||r[0x41b]||r[0x417]>1||
    (r[0x500]&0xc1)!=0xc0||r[0x501]!=0x10||r[0x518]||r[0x515]||
    r[0x51d]>=12||r[0x50a]!=r[0x51d]/3||r[0x51c]!=0x24+r[0x51d]||
    p->extraObjectCount>OBJECT_CAP||(p->extraObjectCount&&!p->extraObjects))return 0;
  const uint16_t*v=PpuRenderVram(p);uint16_t blade[256]={0};
  unsigned pose=r[0x51d],frame=r[0x51c],slots[4],tags[4],opaque_tiles=0;
  int hero_x=(int16_t)word(r,0x50c),hero_y=(int16_t)word(r,0x50e);
  int ox=hero_x+(int16_t)word(rom,0xe2d4+frame*4);
  int oy=hero_y+(int16_t)word(rom,0xe2d6+frame*4);
  /* Screen coordinates must still belong to this actual camera. */
  if((uint16_t)(word(r,0x502)-word(r,0x480))!=(uint16_t)hero_x||
    (uint16_t)(word(r,0x504)-word(r,0x482))!=(uint16_t)hero_y)return 0;
  for(unsigned q=0;q<4;q++){
    unsigned data=word(rom,0xf286+(pose+1)*8+q*2);
    unsigned at=0x4000+(25+q)*16,from=0x88000+(data&511)*32;
    if(from>size||size-from<32)return 0;
    for(unsigned n=0;n<16;n++)if(v[at+n]!=word(rom,from+n*2))return 0;
    unsigned expected=(data&0xfe00)|(25+q),found=128;
    for(unsigned n=0;n<128;n++){
      unsigned hi=(p->highOam[n/4]>>((n&3)*2))&3;
      if(hi&2)continue;
      int x=(p->oam[n*2]&255)|((hi&1)<<8),y=p->oam[n*2]>>8;
      if(x==((ox+(int)(q&1)*8)&511)&&y==((oy+(int)(q>>1)*8)&255)&&
        p->oam[n*2+1]==expected){if(found!=128)return 0;found=n;}
    }
    if(found==128)return 0;slots[q]=found;tags[q]=expected;
    for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++){
      unsigned sx=expected&0x4000?7-x:x,sy=expected&0x8000?7-y:y;
      uint32_t planes=v[at+sy]|(uint32_t)v[at+sy+8]<<16;
      unsigned bit=7-sx,c=((planes>>bit)&1)|((planes>>(bit+7))&2)|
        ((planes>>(bit+14))&4)|((planes>>(bit+21))&8);
      if(c){opaque_tiles|=1u<<q;blade[((q>>1)*8+y)*16+(q&1)*8+x]=(uint16_t)(0x8000|p->cgram[128+((expected>>9)&7)*16+c]);}
    }
  }
  if(!opaque_tiles)return 0;
  unsigned primary=0;while(!(opaque_tiles&(1u<<primary)))primary++;
  for(unsigned q=0;q<4;q++)if(slots[q]!=slots[0]+q||
    ((opaque_tiles&(1u<<q))&&((tags[q]^tags[primary])&0x3e00)))return 0;
  int ax,ay;if(!extend(blade,hero_x-ox,hero_y-oy,&ax,&ay))return 0;
  if(p->extraObjectCount)memmove(objects,p->extraObjects,p->extraObjectCount*sizeof(*objects));
  size_t count=p->extraObjectCount;
  objects[count]=(PpuExtraObject){ox+ax,oy+ay,SIDE,SIDE,SIDE,
    (uint8_t)((tags[primary]>>12)&3),(uint8_t)!!(tags[primary]&0x800),(uint8_t)slots[primary],1,pixels};
  PpuSetExtraObjects(p,objects,count+1);return 1;
}
