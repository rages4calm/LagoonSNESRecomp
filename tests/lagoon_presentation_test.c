#include "support/lagoon_epx_reference.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
  enum {W=5,H=36,SP=7,DP=13};
  uint32_t src[SP*H],copy[SP*H],dst[DP*H*2];
  for(unsigned i=0;i<SP*H;++i)src[i]=0xff000000u;
  /* Upper-left EPX corner chooses the common north/west neighbor. */
  src[1*SP+2]=src[2*SP+1]=0xffffffffu;
  src[2*SP+2]=0xff0000ffu;
  memcpy(copy,src,sizeof(src));
  for(unsigned i=0;i<DP*H*2;++i)dst[i]=0x12345678u;
  assert(LagoonEpxReference2x(src,SP*4,W,H,dst,DP*4));
  assert(dst[4*DP+4]==0xffffffffu);
  assert(dst[4*DP+5]==0xff0000ffu);
  assert(dst[5*DP+4]==0xff0000ffu);
  assert(!memcmp(src,copy,sizeof(src)));
  for(unsigned y=0;y<H*2;++y)
    for(unsigned x=W*2;x<DP;++x)assert(dst[y*DP+x]==0x12345678u);
  for(unsigned y=H-32;y<H;++y)for(unsigned x=0;x<W;++x)
    for(unsigned dy=0;dy<2;++dy)for(unsigned dx=0;dx<2;++dx)
      assert(dst[(y*2+dy)*DP+x*2+dx]==src[y*SP+x]);
  assert(!LagoonEpxReference2x(src,SP*4,W,H,dst,4));
  assert(!LagoonEpxReference2x(src,SP*4,4097,H,dst,DP*4));
  assert(!LagoonEpxReference2x(NULL,SP*4,W,H,dst,DP*4));
  /* Every output value must be an existing input palette entry. */
  unsigned seed=7;
  for(unsigned i=0;i<SP*H;++i) {seed=seed*1664525u+1013904223u;src[i]=0xff000000u|((seed>>28)*0x111111u);}
  assert(LagoonEpxReference2x(src,SP*4,W,H,dst,DP*4));
  for(unsigned y=0;y<H*2;++y)for(unsigned x=0;x<W*2;++x) {
    unsigned found=0;
    for(unsigned sy=0;sy<H;++sy)for(unsigned sx=0;sx<W;++sx)
      found |= dst[y*DP+x]==src[sy*SP+sx];
    assert(found);
  }
  puts("PASS EPX corners, HUD identity, palette preservation, immutable input, padded rows, invalid dimensions");
  return 0;
}
