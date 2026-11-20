#ifndef LAGOON_GOLD_CLOUD77_H
#define LAGOON_GOLD_CLOUD77_H
/* Original02:C67C Gold Cave cloud append, independently completed AFTER the
 * ordinary C026 draw. This is not an actor or the Phantom Hill C701 cloud. */
#include "lagoon_wide_entities.h"
#include <string.h>
enum {kGoldCloud77Cells=96};
typedef struct LagoonGoldCloud77 {
  LagoonWideCell cells[kGoldCloud77Cells];
  int slots[kGoldCloud77Cells];
  uint8_t native_slots[128];
  unsigned count,first,end;
} LagoonGoldCloud77;
static unsigned LagoonGold77_Word(const uint8_t*r,unsigned a){return r[a]|r[a+1]<<8;}
static uint64_t LagoonGold77_Hash(const uint8_t*r,unsigned a,unsigned n){
  uint64_t h=UINT64_C(14695981039346656037);
  for(unsigned i=0;i<n;i++)h=(h^r[a+i])*UINT64_C(1099511628211);return h;
}
static int LagoonGold77_Source(const uint8_t*rom,size_t size){
  return rom&&size==0x100000&&
    LagoonGold77_Hash(rom,0x1467c,133)==UINT64_C(0x5410bb7127851a66)&&
    LagoonWideEntities_CurrentSource(rom,size,8,0)&&LagoonWideEntities_CurrentSource(rom,size,8,1);
}
/* Caller owns a positive completed original append, full OAM/generation and
 * current source graphics. Native vertical eligibility is the original8C7F;
 * anchor X wraps9bits and anchor Y8bits before cell offsets, never per cell. */
static int LagoonGold77_Decode(LagoonGoldCloud77*out,const uint8_t*ram,
    const uint8_t*rom,size_t size,unsigned first){
  if(!out)return 0;memset(out,0,sizeof *out);
  if(!ram||first<8||first>128||LagoonGold77_Word(ram,0x48e)!=7||!LagoonGold77_Source(rom,size))return 0;
  unsigned slot=first;out->first=first;
  for(unsigned group=0;group<7;group++){
    unsigned a=0x146de + group*5;
    int x=(LagoonGold77_Word(rom,a)+LagoonGold77_Word(ram,0x40d)-LagoonGold77_Word(ram,0x204))&511;
    int y=(LagoonGold77_Word(rom,a+2)-LagoonGold77_Word(ram,0x206))&255;
    int n=LagoonWideEntities_Decode(rom,size,8,rom[a+4],x,y,out->cells+out->count,kGoldCloud77Cells-out->count);
    if(n<=0||out->count+(unsigned)n>kGoldCloud77Cells)return 0;
    for(int i=0;i<n;i++){
      unsigned at=out->count+(unsigned)i;const LagoonWideCell*c=&out->cells[at];
      if(c->size!=16||(c->tile>>12&3)!=2)return 0;
      /* Negative encoding preserves conceptual order after the preceding
       * native slot, including a partial high-OAM byte. */
      out->slots[at]=-(int)slot-1;
      if(c->y>=-16&&c->y<240){
        if(slot>=128)return 0;out->slots[at]=(int)slot;out->native_slots[slot++]=1;
      }
    }
    out->count+=(unsigned)n;
  }
  out->end=slot;return out->count&&slot>first&&slot==ram[0x236];
}
static void LagoonGold77_WriteCell(uint8_t*oam,const LagoonWideCell*c,unsigned slot){
  unsigned shift=(slot&3)*2;
  oam[slot*4]=(uint8_t)c->x;oam[slot*4+1]=(uint8_t)c->y;
  oam[slot*4+2]=(uint8_t)c->tile;oam[slot*4+3]=(uint8_t)(c->tile>>8);
  oam[512+slot/4]=(uint8_t)((oam[512+slot/4]&~(3u<<shift))|
      ((((unsigned)c->x>>8&1)|(c->size==16?2:0))<<shift));
}
static int LagoonGold77_Match(const LagoonGoldCloud77*c,const uint8_t*oam){
  if(!c||!oam||!c->count||c->end>128)return 0;
  for(unsigned i=0;i<c->count;i++)if(c->slots[i]>=0){
    const LagoonWideCell*p=&c->cells[i];unsigned slot=(unsigned)c->slots[i],shift=(slot&3)*2;
    unsigned hi=(oam[512+slot/4]>>shift)&3;
    if((oam[slot*4]|(hi&1)<<8)!=((unsigned)p->x&511)||oam[slot*4+1]!=((unsigned)p->y&255)||
        LagoonGold77_Word(oam,slot*4+2)!=p->tile||((hi&2)?16:8)!=p->size)return 0;
  }
  return 1;
}
/* This proves both unchanged base/tail and every exact source-produced cell.
 * Updating2bits per slot preserves a shared partial high-OAM byte. */
static int LagoonGold77_Append(const LagoonGoldCloud77*c,const uint8_t*before,const uint8_t*after){
  if(!c||!before||!after)return 0;uint8_t expected[544];memcpy(expected,before,sizeof expected);
  for(unsigned i=0;i<c->count;i++)if(c->slots[i]>=0)LagoonGold77_WriteCell(expected,&c->cells[i],(unsigned)c->slots[i]);
  return !memcmp(expected,after,sizeof expected);
}
#endif
