#include "lagoon_vram_span.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

static unsigned prefix[2][131073];
static unsigned original(unsigned first, unsigned bytes) {
  unsigned touches=0,graphics=0;
  for(unsigned n=0;n<bytes;++n) {
    unsigned a=(first+n)&65535;
    if((a>=0x6000&&a<0x7260)||(a>=0xe000&&a<0xe800)||a>=0xf000)touches=1;
    if(a>=0x6000&&a<0x7260)graphics=1;
  }
  return touches|(graphics<<1);
}
static unsigned prefix_oracle(unsigned first,unsigned bytes) {
  if(bytes>=65536)return 3;
  return !!(prefix[0][first+bytes]-prefix[0][first]) |
    (!!(prefix[1][first+bytes]-prefix[1][first])<<1);
}
int main(void) {
  for(unsigned i=0;i<131072;++i) {
    unsigned a=i&65535;
    prefix[0][i+1]=prefix[0][i]+((a>=0x6000&&a<0x7260)||(a>=0xe000&&a<0xe800)||a>=0xf000);
    prefix[1][i+1]=prefix[1][i]+(a>=0x6000&&a<0x7260);
  }
  unsigned lengths[]={0,1,2,3,7,8,31,32,63,64,127,128,255,256,511,512,1023,1024,
    2047,2048,4095,4096,8191,8192,16383,16384,32767,32768,65535,65536};
  unsigned boundaries[]={0,0x6000,0x7260,0xe000,0xe800,0xf000,0x10000};
  uint64_t cases=0;
  for(unsigned first=0;first<65536;++first) {
    for(unsigned j=0;j<sizeof(lengths)/sizeof(*lengths);++j) {
      unsigned bytes=lengths[j];
      assert(LagoonVramSpan_Spell3Mask(first,bytes)==prefix_oracle(first,bytes));++cases;
    }
    for(unsigned j=0;j<sizeof(boundaries)/sizeof(*boundaries);++j)for(int delta=-1;delta<=1;++delta) {
      unsigned bytes=((boundaries[j]-first)&65535);
      int n=(int)bytes+delta;
      if(n<0||n>65536)continue;
      assert(LagoonVramSpan_Spell3Mask(first,(unsigned)n)==prefix_oracle(first,(unsigned)n));++cases;
    }
  }
  uint32_t rng=0x43555253;
  for(unsigned i=0;i<8192;++i) {
    rng=rng*1664525u+1013904223u;unsigned first=rng&65535;
    rng=rng*1664525u+1013904223u;unsigned bytes=rng&65535;
    assert(LagoonVramSpan_Spell3Mask(first,bytes)==original(first,bytes));++cases;
  }
  assert(LagoonVramSpan_Spell3Mask(65535,65537)==3);
  /* Include actual DMA word-address normalization and zero-length expansion. */
  for(unsigned destination=0;destination<65536;++destination) {
    unsigned first=(destination&32767)*2;
    assert(LagoonVramSpan_Spell3Mask(first,65536)==3);++cases;
  }
  volatile unsigned sum=0;
  double begin=(double)clock()/CLOCKS_PER_SEC;
  for(unsigned i=0;i<8192;++i)sum+=original((i*37)&65535,4096);
  double reference=(double)clock()/CLOCKS_PER_SEC-begin;
  begin=(double)clock()/CLOCKS_PER_SEC;
  for(unsigned i=0;i<819200;++i)sum+=LagoonVramSpan_Spell3Mask((i*37)&65535,4096);
  double fast=(double)clock()/CLOCKS_PER_SEC-begin;
  printf("{\"passed\":true,\"cases\":%llu,\"reference_calls\":8192,\"fast_calls\":819200,\"reference_seconds\":%.9f,\"fast_seconds\":%.9f,\"check\":%u}\n",
    (unsigned long long)cases,reference,fast,sum);
  return 0;
}
