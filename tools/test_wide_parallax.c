/* Own clean ROM required; no copied asset bytes are embedded in this test. */
#include "lagoon_wide_entities.h"
#include "lagoon_menu_decode.h"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t rom[0x100000],ram[0x20000],fast[0x20000];
static uint16_t vram[32768];
static uint8_t read_byte(void *m,uint32_t a) {
  (void)m;unsigned bank=a>>16,off=a&65535;
  if(bank==126 || bank==127)return ram[(bank-126)*65536+off];
  if((bank&127)<64 && off<8192)return ram[off];
  assert((bank&127)<32 && off>=32768);
  return rom[(bank&31)*32768+(off&32767)];
}
static void write_byte(void *m,uint32_t a,uint8_t v) {
  (void)m;unsigned bank=a>>16,off=a&65535;
  if(bank==126 || bank==127){ram[(bank-126)*65536+off]=v;return;}
  assert((bank&127)<64 && off<8192);ram[off]=v;
}
int main(int argc,char **argv) {
  assert(argc==2);FILE *f=fopen(argv[1],"rb");
  assert(f && fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);
  ram[0xE0]=0x7D;ram[0xE1]=0xFB;ram[0xE2]=0x11;
  ram[0x1D1]=0xFF;ram[0x1D2]=0x0F;ram[0x1D3]=0;
  memcpy(fast,ram,sizeof(ram));
  CpuState c={0};c.S=0x1D0;c.m_flag=1;c.P=0x24;
  unsigned consumed=0,produced=0;
  assert(LagoonMenuDecode_Expand(rom,sizeof(rom),fast,&c,&consumed,&produced));
  assert(consumed==761 && produced==2052);
  Interp816 *i=interp816_init(NULL,read_byte,write_byte);assert(i);
  i->pc=0x9143;i->sp=0x1D0;i->mf=1;i->xf=0;i->e=0;i->i=1;
  unsigned steps=0;
  while(i->k || i->pc!=0x1000){assert(++steps<1000000);interp816_runOpcode(i);}
  assert(!memcmp(ram,fast,sizeof(ram)));interp816_free(i);
  for(unsigned page=0;page<2;++page)for(unsigned n=0;n<1024;++n)
    vram[0x7800+page*0x400+n]=ram[0x17F80+n*2]|ram[0x17F81+n*2]<<8;
  assert(LagoonWideEntities_ParallaxMap(vram,rom,sizeof(rom)));
  assert(!LagoonWideEntities_ParallaxMap(vram,rom,0x8FB7D+760));
  assert(!LagoonWideEntities_ParallaxMap(NULL,rom,sizeof(rom)));
  assert(!LagoonWideEntities_ParallaxMap(vram,NULL,sizeof(rom)));
  for(unsigned page=0;page<2;++page) {
    for(unsigned n=0;n<1024;++n) {
      vram[0x7800+page*0x400+n]^=1;
      assert(!LagoonWideEntities_ParallaxMap(vram,rom,sizeof(rom)));
      vram[0x7800+page*0x400+n]^=1;
    }
  }
  for(unsigned n=0;n<761;++n) {
    rom[0x8FB7D+n]^=1;assert(!LagoonWideEntities_ParallaxMap(vram,rom,sizeof(rom)));
    rom[0x8FB7D+n]^=1;
  }
  for(unsigned n=0;n<1024;++n) {
    assert(LagoonWideEntities_ParallaxScroll(n,1023-n,n>>2,(1023-n)>>2));
    assert(!LagoonWideEntities_ParallaxScroll(n,1023-n,(n>>2)^1,(1023-n)>>2));
    assert(!LagoonWideEntities_ParallaxScroll(n,1023-n,n>>2,((1023-n)>>2)^1));
  }
  assert(!LagoonWideEntities_ParallaxScroll(1024,0,256,0));
  assert(!LagoonWideEntities_ParallaxScroll(0,1024,0,256));
  puts("room07 parallax: original decoder/full128KiB, both map pages, every source/map-word mutation, quarter raster anchors PASS");
  return 0;
}
