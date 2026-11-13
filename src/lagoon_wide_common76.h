#ifndef LAGOON_WIDE_COMMON76_H
#define LAGOON_WIDE_COMMON76_H
#include "lagoon_wide_entities.h"
/* Geometry only: callers must independently prove the complete original
 * common2 upload, current kind0 NPC source/frame and current producer context.
 * Its1760 uploaded bytes are all produced by the original1764-byte decode;
 * no room-atlas tail or common3-only output is authority for this range. */
static inline int LagoonWideCommon2_CellsOwned(unsigned base1,unsigned base2,
    const LagoonWideCell *cells,int count) {
  if(!cells||count<=0)return 0;
  for(int i=0;i<count;++i) {
    const LagoonWideCell *c=&cells[i];
    if(c->size!=8&&c->size!=16)return 0;
    unsigned base=c->tile&0x100?base2:base1;
    for(unsigned y=0;y<c->size;y+=8)for(unsigned x=0;x<c->size;x+=8) {
      unsigned tile=((((c->tile&255)>>4)+y/8)<<4)|(((c->tile&15)+x/8)&15);
      unsigned address=(base+tile*16)&0x7fff;
      if(address<0x5800||address+16>0x5b70)return 0;
    }
  }
  return 1;
}
#endif
