#include "lagoon_wide_entities.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char rom[0x100000];
static void byte(unsigned address, unsigned v) {
  rom[(((address >> 16) & 0x7f) << 15) | (address & 0x7fff)] = (unsigned char)v;
}
static void setup(const unsigned char *stream, size_t length) {
  memset(rom, 0, sizeof(rom));
  byte(0x999d, 0); byte(0x999e, 0xa0); byte(0x999f, 0);
  byte(0xa000, 0); byte(0xa001, 0xb0);
  for (size_t i = 0; i < length; ++i) byte(0xb000 + (unsigned)i, stream[i]);
}
int main(void) {
  LagoonWideCell c[16];
  const unsigned char plain[] = {0x22,1, 1,0x26,2,0x66,3,0xa6,4,0xe6};
  setup(plain, sizeof(plain));
  assert(LagoonWideEntities_Decode(rom,sizeof(rom),0,0,-8,9,c,16)==4);
  assert(c[0].x==-8 && c[0].y==9 && c[0].size==16 && c[0].tile==0x2601);
  assert(c[3].x==8 && c[3].y==25 && c[3].tile==0xe604);
  assert(LagoonWideEntities_Decode(rom,sizeof(rom),0,0,0,0,c,3)==-1);
  const unsigned char escape[] = {0x31,0, 0xff,0xff, 0xff,0x42,0x6a,0xfc,0x03, 7,0x22};
  setup(escape,sizeof(escape));
  assert(LagoonWideEntities_Decode(rom,sizeof(rom),0,0,20,30,c,16)==2);
  assert(c[0].x==24 && c[0].y==33 && c[0].tile==0x6a42 && c[0].size==8);
  assert(c[1].x==36 && c[1].y==30); /* escape does not move following cells */
  const unsigned char grids[] = {0x11,0x81, 1,0x20, 0x21, 2,0x40,3,0x60};
  setup(grids,sizeof(grids));
  assert(LagoonWideEntities_Decode(rom,sizeof(rom),0,0,3,4,c,16)==3);
  assert(c[0].size==16 && c[1].size==8 && c[2].x==11 && c[2].size==8);
  assert(LagoonWideEntities_Decode(rom,16,0,0,0,0,c,16)==-1);
  assert(LagoonWideEntities_Decode(rom,sizeof(rom),0x49,0,0,0,c,16)==-1);
  const unsigned char empty[]={0,0}; setup(empty,sizeof(empty));
  assert(LagoonWideEntities_Decode(rom,sizeof(rom),0,0,0,0,c,16)==-1);
  puts("wide entity decoder: grids, sizes, attributes, skips, signed escapes and refusal PASS");
  return 0;
}
