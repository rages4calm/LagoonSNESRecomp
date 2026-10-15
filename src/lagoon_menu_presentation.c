/* Reflow original, live-rendered frame pieces; contains no copied game pixels.
 * Exact24px chain periods constrain symmetric expansion to48px increments.
 * Native8px outside black margins may be clipped; ornaments remain intact. */
#include "lagoon_menu_presentation.h"
#include "snes/ppu.h"
#include <string.h>
static uint32_t hash_word(uint32_t hash, uint16_t word) {
  hash=(hash^(word&255u))*16777619u;
  return (hash^(word>>8))*16777619u;
}
LagoonMenuPanel LagoonMenuPresentation_Identify(const Ppu *p, const uint8_t *ram) {
  if(!p || !ram || (p->inidisp&0x80) || !(p->inidisp&15) || p->bgmode!=9 ||
      p->bgTileAdr!=0x0600 || p->bgXsc[0]!=0x71 || p->bgXsc[1]!=0x79 ||
      p->bgXsc[2]!=0x3C || p->setini || p->screenEnabled[0]!=31 ||
      p->screenEnabled[1]!=31 || ram[0x275]!=0x81 ||
      ram[0xE0]!=0xC3 || ram[0xE1]!=0xE9 || ram[0xE2]!=0x10) return LAGOON_MENU_NONE;
  for(unsigned i=0;i<3;++i) if(p->hScroll[i] || p->vScroll[i]) return LAGOON_MENU_NONE;
  /* Immutable border/font character resource, including every referenced
   * character in the audited frame/label templates. Hash only, no asset data. */
  uint32_t chars=2166136261u;
  for(unsigned i=0;i<0x500;++i)chars=hash_word(chars,p->vram[i]);
  if(chars!=0x2F4FB15Cu)return LAGOON_MENU_NONE;
  /* Exact border + top label map. Dynamic inventory cells and counters are
   * excluded; unknown label, frame, layout or graphics cannot borrow authority. */
  uint32_t frame=2166136261u;
  for(unsigned y=0;y<28;++y)for(unsigned x=0;x<32;++x)
    if(y<4 || y>=25 || x<4 || x>=28)frame=hash_word(frame,p->vram[0x7000+y*32+x]);
  if(frame==0xC8B4967Au)return LAGOON_MENU_ITEMS;
  if(frame==0xB406703Fu)return LAGOON_MENU_MAGIC;
  return LAGOON_MENU_NONE;
}
static int equal_period(const uint32_t *native, unsigned y0, unsigned y1) {
  for(unsigned y=y0;y<y1;++y)for(unsigned x=40;x<64;++x)
    if(native[y*256+x]!=native[y*256+x+24] ||
       native[y*256+168+(x-40)]!=native[y*256+192+(x-40)])return 0;
  return 1;
}
static int layout(uint8_t *dst, size_t pitch, const uint32_t *native,
    unsigned width, LagoonMenuPanel panel) {
  if(!equal_period(native,0,32) || !equal_period(native,200,224) ||
      (native[0]&0xFFFFFFu))return 0;
  if(panel==LAGOON_MENU_MAGIC)for(unsigned y=130;y<=131;++y)
    for(unsigned x=24;x<232;++x)if(native[y*256+x]!=native[y*256+40])return 0;
  unsigned virtual_width=256+48*((width-240)/48);
  int left=((int)width-(int)virtual_width)/2;
  unsigned extra=virtual_width-256,center=(width-256)/2;
  uint32_t black=native[0];
  for(unsigned y=0;y<224;++y) {
    uint32_t *row=(uint32_t *)(dst+y*pitch);
    for(unsigned x=0;x<width;++x) {
      int vx=(int)x-left;
      uint32_t color=black;
      if(vx>=0 && vx<(int)virtual_width) {
        int sx=-1;
        if(vx<40)sx=vx;
        else if(vx>=(int)virtual_width-40)sx=vx-(int)extra;
        else if(y<32 || y>=200) {
          if(x>=center+88 && x<center+168)sx=(int)x-(int)center;
          else if(x<center+88)sx=40+(vx-40)%24;
          else sx=168+(int)(x-center-168)%24;
        } else if(x>=center+40 && x<center+216)sx=(int)x-(int)center;
        if(sx>=0 && sx<256)color=native[y*256+(unsigned)sx];
        if(panel==LAGOON_MENU_MAGIC && (y==130 || y==131) &&
            vx>=24 && vx<(int)virtual_width-24)color=native[y*256+40];
      }
      row[x]=color;
    }
  }
  return 1;
}
int LagoonMenuPresentation_Draw(uint8_t *dst, size_t pitch, const uint8_t *field,
    size_t source_pitch, unsigned width, unsigned height, const Ppu *p,
    const uint8_t *ram, int widescreen) {
  if(!widescreen || !dst || !field || width<=256 || width>800 || (width&1) ||
      height!=224 || pitch<width*4u || source_pitch<width*4u || pitch%4 || source_pitch%4)
    return 0;
  LagoonMenuPanel panel=LagoonMenuPresentation_Identify(p,ram);
  if(panel==LAGOON_MENU_NONE)return 0;
  uint32_t native[256*224];
  unsigned center=(width-256)/2;
  for(unsigned y=0;y<224;++y)memcpy(native+y*256,field+y*source_pitch+center*4,256*4);
  return layout(dst,pitch,native,width,panel);
}
