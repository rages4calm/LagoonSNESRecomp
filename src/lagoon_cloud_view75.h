#ifndef LAGOON_CLOUD_VIEW75_H
#define LAGOON_CLOUD_VIEW75_H
/* Read-only original Phantom Hill decorative cloud. No guest actor/RNG writes.
 * 02:C701..C796 owns0390/style0391/world0392/0394 and selector29 frames0C/0D.
 * Its original spawn is cameraX-176, cameraY+random8-32; retirement is X>=256.
 * Only the group anchor is remapped; cell art, size, order and offsets remain. */
#include "lagoon_wide_entities.h"
#include <string.h>
typedef struct LagoonCloudView75 {
  LagoonWideCell cells[64];
  uint8_t native_slots[128];
  unsigned count,matched,valid;
  int anchor_x,anchor_y;
} LagoonCloudView75;
static unsigned LagoonCloud75_Word(const uint8_t *r,unsigned a){return r[a]|r[a+1]<<8;}
static uint64_t LagoonCloud75_Hash(const uint8_t *r,unsigned a,unsigned n){
  uint64_t h=UINT64_C(14695981039346656037);
  for(unsigned i=0;i<n;i++)h=(h^r[a+i])*UINT64_C(1099511628211);
  return h;
}
static int LagoonCloud75_Source(const uint8_t *rom,size_t size){
  return rom&&size==0x100000&&
    LagoonCloud75_Hash(rom,0x14701,150)==UINT64_C(0xc435d61148c307bd)&&
    LagoonCloud75_Hash(rom,0xd5b6,728)==UINT64_C(0xabf064da81ac7ab9)&&
    LagoonCloud75_Hash(rom,0x1a18,3)==UINT64_C(0x8fd3fe1a85cb1a7c);
}
/* Caller binds the complete immutable raster packet to exact room resources,
 * state generation and OAM/VRAM/palette equality for the field as usual. */
static int LagoonCloudView75_Capture(LagoonCloudView75 *out,const uint8_t *ram,
    const uint8_t *rom,size_t size,const uint8_t *oam,const uint8_t *high){
  if(!out)return 0;
  memset(out,0,sizeof *out);
  if(!ram||!oam||!high||LagoonCloud75_Word(ram,0x48e)!=0x28||
      ram[0x391]>1||!LagoonCloud75_Source(rom,size))return 0;
  int x=(int16_t)(LagoonCloud75_Word(ram,0x392)-LagoonCloud75_Word(ram,0x480));
  int y=(int16_t)(LagoonCloud75_Word(ram,0x394)-LagoonCloud75_Word(ram,0x482));
  /* A retired flag can precede presentation of its final completed OAM.
   * C72F retires only when relativeX has high byte01. All other inactive
   * records refuse, even if old cloud-looking sprites happen to remain. */
  if(!ram[0x390]&&(x<256||x>=512))return 0;
  /* At the first raster the guest can already have advanced cloudX by its
   * original six pixels while OAM still holds the completed producer. Admit
   * either exact literal phase only; never infer motion from an arbitrary tile. */
  for(unsigned phase=0;phase<2;phase++){
    int ax=x-(phase?6:0);
    if(ax < -256 || ax>261 || y < -256 || y>479)continue;
    int n=LagoonWideEntities_Decode(rom,size,0x29,0x0c+ram[0x391],ax,y,out->cells,64);
    if(n<=0||n>64)continue;
    unsigned slot=8,matched=0;int exact=1;
    memset(out->native_slots,0,sizeof out->native_slots);
    for(int i=0;i<n;i++){
      const LagoonWideCell *c=&out->cells[i];
      /* Original00:8C7F accepts the top16 wrap and y0..239. */
      if(c->y < -16 || c->y>=240)continue;
      if(slot>=128){exact=0;break;}
      unsigned hi=(high[slot/4]>>((slot%4)*2))&3;
      unsigned ox=oam[slot*4]|(hi&1)<<8;
      if(ox!=((unsigned)c->x&511)||oam[slot*4+1]!=((unsigned)c->y&255)||
          LagoonCloud75_Word(oam,slot*4+2)!=c->tile||((hi&2)?16:8)!=c->size){exact=0;break;}
      out->native_slots[slot++]=1;matched++;
    }
    if(exact&&matched){out->count=(unsigned)n;out->matched=matched;out->anchor_x=ax;
      out->anchor_y=y;out->valid=1;return 1;}
  }
  memset(out,0,sizeof *out);return 0;
}
static int LagoonCloud75_FloorDiv(int a,int b){return a>=0?a/b:-((-a+b-1)/b);}
/* Bounds are relative to the native camera; width/height are the expanded
 * presentation world rectangle. Native256x224 bounds produce identity. */
static void LagoonCloudView75_RemapAnchor(const LagoonCloudView75 *c,int left,int top,
    unsigned width,unsigned height,int *x,int *y){
  *x=left-176+LagoonCloud75_FloorDiv((c->anchor_x+176)*((int)width+176),432);
  *y=top-32+LagoonCloud75_FloorDiv((c->anchor_y+32)*((int)height+32),256);
}
#endif
