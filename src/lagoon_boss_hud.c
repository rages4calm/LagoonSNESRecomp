/* Correct two original first-phase proxy health displays without changing any
 * guest health, UI queue, CPU timing, collision or boss scripts. The original
 * bar glyphs and the independently decoded current scene underlay stay live. */
#include "lagoon_boss_hud.h"
#include "snes/ppu.h"
#include <string.h>

enum { BAR_X=64,BAR_Y=207,BAR_W=128,BAR_H=8 };
static const uint8_t *image;static size_t image_size;
static struct {uint32_t old_pixels[1024],new_pixels[1024];
  uint64_t generation;unsigned frame,valid,health,maximum,phase;} packet;
static unsigned word(const uint8_t*r,unsigned a){return r[a]|r[a+1]<<8;}
static uint64_t hash(const uint8_t*p,unsigned n){uint64_t h=UINT64_C(14695981039346656037);
  while(n--)h=(h^*p++)*UINT64_C(1099511628211);return h;}
static int source(unsigned pc,unsigned n,uint64_t h){unsigned a=(pc>>16)*32768+(pc&32767);
  return image&&a<=image_size&&n<=image_size-a&&hash(image+a,n)==h;}
static int source_ok(void){
  return source(0x02d470,155,UINT64_C(0xc448c1de7b4de14e))&&
    source(0x01c59f,78,UINT64_C(0xd9affd04425cc529))&&
    source(0x018ce8,89,UINT64_C(0xf1d46df2cf57a9cb));
}
void LagoonBossHud_Init(const uint8_t *rom,size_t size){
  image=size==0x100000?rom:NULL;image_size=size;memset(&packet,0,sizeof packet);
}
static int phase_health(const uint8_t*r,unsigned *health,unsigned *maximum,unsigned *phase){
  unsigned room=word(r,0x48e);if(room!=0x46&&room!=0x47)return 0;
  unsigned d=5*32768+((0xb2df+room*56)&32767);
  if(!image||memcmp(image+d,r+0x50,56)||!r[0x98]||word(r,0x9a)!=0x40||r[0x100e])return 0;
  if(room==0x46){
    /* Actual authored05AD8E record is type2/kind37 in slot100. C59F keeps
     * selecting the untouched256HP proxy in40 while this1280HP caster loses
     * health. Original8CE8 loads its real maximum from59424+1. */
    if((r[0x4e0]&1)||(r[0x540]&0xc0)!=0xc0||r[0x558]!=3||r[0x55b]!=10||
        word(r,0x560)!=256||r[0x618]!=2||r[0x61b]!=37||
        (r[0x600]&0xc0)!=0xc0||word(r,0x620)>1280||
        !source(0x059424,15,UINT64_C(0x33b1695845bb84b3))||
        !source(0x0591f5,2,UINT64_C(0x07b7a407b4829045))||
        !source(0x05ad73,38,UINT64_C(0x3b5d53ba35de5c60))||
        !source(0x05c25a,3,UINT64_C(0x833006195db19e68)))return 0;
    *health=word(r,0x620);*maximum=1280;*phase=1;return 1;
  }
  /* Actual Zerah is type2/kind38 in80; the original HUD still selects the
   * retired zero-HP NPC2 in40. Its real source maximum is59433+1=500.
   * Thor11/Bird12 replace40 later and use their changing primary HP unchanged. */
  if((r[0x540]&0xc0)||r[0x558]!=1||r[0x55b]!=2||word(r,0x560)||
      r[0x598]!=2||r[0x59b]!=38||(r[0x580]&0xc0)!=0xc0||word(r,0x5a0)>500||
      !source(0x059433,15,UINT64_C(0x004cf7edc8580c9d))||
      !source(0x0591f7,2,UINT64_C(0x08002207b4c11c02))||
      !source(0x05ad99,29,UINT64_C(0x49452bcac3699ba2))||
      !source(0x05c292,3,UINT64_C(0xf69be01b520a0ada)))return 0;
  *health=word(r,0x5a0);*maximum=500;*phase=2;return 1;
}
static unsigned pixel(const uint16_t*v,unsigned tile,unsigned x,unsigned y,unsigned base,unsigned bpp){
  if(tile&0x4000)x=7-x;if(tile&0x8000)y=7-y;
  unsigned a=(base+(tile&1023)*(bpp==2?8:16)+y)&32767;
  unsigned c=((v[a]>>(7-x))&1)|(((v[a]>>(15-x))&1)<<1);
  if(bpp==4)c|=(((v[(a+8)&32767]>>(7-x))&1)<<2)|(((v[(a+8)&32767]>>(15-x))&1)<<3);
  return c;
}
static uint32_t rgb(unsigned z){unsigned r=z&31,g=z>>5&31,b=z>>10&31;
  return ((r<<3)|(r>>2))<<16|((g<<3)|(g>>2))<<8|(b<<3)|(b>>2);}
static void underlay(const Ppu*p,const uint16_t*v,uint16_t *colors){
  uint8_t depth[1024]={0};unsigned ranks[1024];
  for(unsigned i=0;i<1024;++i){colors[i]=p->cgram[0];ranks[i]=128;}
  for(unsigned l=0;l<2;++l)for(unsigned y=0;y<8;++y)for(unsigned x=0;x<128;++x){
    unsigned qx=p->hScroll[l]+BAR_X+x,qy=p->vScroll[l]+BAR_Y+y+1;
    unsigned a=PPU_bgTilemapAdr(p,l)+((qy>>3)&31)*32+((qx>>3)&31)+((qx&256)?0x400:0);
    unsigned t=v[a&32767],c=pixel(v,t,qx&7,qy&7,PPU_bgTileAdr(p,l),4),z=((t&0x2000)?12:8)-l,i=y*128+x;
    if(c&&z>depth[i]){depth[i]=z;colors[i]=p->cgram[((t>>10)&7)*16+c];}
  }
  const uint8_t*o=(const uint8_t*)p->oam;
  for(unsigned s=0;s<128;++s){unsigned hi=(p->highOam[s/4]>>((s&3)*2))&3,n=hi&2?16:8;
    int ox=o[s*4]|((hi&1)<<8);if(ox>=384)ox-=512;
    if(ox>=BAR_X+128||ox+(int)n<=BAR_X)continue;
    unsigned t=o[s*4+2]|o[s*4+3]<<8;
    for(unsigned y=0;y<8;++y){unsigned sy=(BAR_Y+y-o[s*4+1])&255;if(sy>=n)continue;if(t&0x8000)sy=n-1-sy;
      for(unsigned x=0;x<128;++x){int sx=BAR_X+(int)x-ox;if(sx<0||sx>=(int)n)continue;if(t&0x4000)sx=n-1-sx;
        unsigned i=y*128+x,ch=((((t&255)>>4)+(sy>>3))<<4)|(((t&15)+((unsigned)sx>>3))&15);
        unsigned c=pixel(v,ch,sx&7,sy&7,(t&256)?0x5000:0x4000,4);
        if(c&&s<ranks[i]){ranks[i]=s;unsigned z=2+4*((t>>12)&3);
          if(z>depth[i]){colors[i]=p->cgram[128+((t>>9)&7)*16+c];depth[i]=z;}}
      }
    }
  }
}
void LagoonBossHud_ObserveField(const Ppu*p,const uint8_t*r,uint64_t generation,unsigned frame,int hdma){
  packet.valid=0;
  unsigned health,maximum,phase;
  if(!p||!r||!generation||hdma||!source_ok()||!phase_health(r,&health,&maximum,&phase)||
      p->inidisp!=15||p->bgmode!=9||p->obsel!=2||p->bgTileAdr!=0x600||
      p->bgXsc[0]!=0x71||p->bgXsc[1]!=0x79||p->bgXsc[2]!=0x3c||
      p->hScroll[2]||p->vScroll[2]||p->hScroll[0]!=p->hScroll[1]||p->vScroll[0]!=p->vScroll[1]||
      p->mosaic||p->setini||p->cgadsub||p->windowsel||(p->cgwsel&1)||(p->oamaddh&0x80)||
      p->screenWindowed[0]||p->screenWindowed[1]||
      p->screenEnabled[0]!=31||p->screenEnabled[1]!=31||
      !(p->renderFlags&kPpuRenderFlags_NoSpriteLimits))return;
  const uint16_t*v=PpuRenderVram(p);
  /* Actual literal original font2bpp resource for all nine bar fractions. */
  if(hash((const uint8_t*)(v+0x6e80),144)!=UINT64_C(0xb48da6ecc204e1e1))return;
  uint16_t old_tiles[16];int tail=0;
  for(unsigned j=0;j<16;++j){
    unsigned t=v[0x3f48+j];if(t<0x2dd0||t>0x2dd8||(tail&&t!=0x2dd0))return;
    if(t!=0x2dd8)tail=1;old_tiles[j]=t;
  }
  uint16_t below[1024];underlay(p,v,below);
  unsigned scaled=(health*256+maximum-1)/maximum;if(scaled>256)return;
  for(unsigned y=0;y<8;++y)for(unsigned x=0;x<128;++x){unsigned i=y*128+x,j=x/8;
    unsigned old=pixel(v,old_tiles[j],x&7,y,0x6000,2),t=0x2dd0;
    if(j<scaled/16)t+=8;else if(j==scaled/16)t+=(scaled&15)/2;
    unsigned now=pixel(v,t,x&7,y,0x6000,2);
    packet.old_pixels[i]=rgb(old?p->cgram[12+old]:below[i]);
    packet.new_pixels[i]=rgb(now?p->cgram[12+now]:below[i]);
  }
  packet.health=health;packet.maximum=maximum;packet.phase=phase;
  packet.generation=generation;packet.frame=frame;packet.valid=1;
}
int LagoonBossHud_Draw(uint8_t*dst,size_t pitch,const uint8_t*field,size_t source_pitch,
    unsigned width,unsigned height,uint64_t generation){
  if(!packet.valid||generation!=packet.generation||!dst||!field||height!=224||
      width<256||width>800||(width&1)||pitch<width*4||source_pitch<width*4)return 0;
  unsigned left=(width-256)/2+BAR_X;
  for(unsigned y=0;y<8;++y)if(memcmp(field+(BAR_Y+y)*source_pitch+left*4,packet.old_pixels+y*128,128*4))return 0;
  for(unsigned y=0;y<8;++y)memcpy(dst+(BAR_Y+y)*pitch+left*4,packet.new_pixels+y*128,128*4);
  return 1;
}
