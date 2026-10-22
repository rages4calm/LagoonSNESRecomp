#include "lagoon_fast_menus.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t rom[0x100000],ram[0x20000],saved[0x20000];
static void put(unsigned p,unsigned v,unsigned n){while(n--){ram[p++]=v;v>>=8;}}
static CpuWramReadContext frame(unsigned dir,unsigned level) {
  memset(ram,0,sizeof ram);CpuWramReadContext c={.stack=0x1D0,.m=1,.interpreter=1,.instruction_pc=0x84E0};
  put(c.stack+1,dir?0x008471:0x0083D8,3);put(c.stack+4,dir?0x02C675:0x02C65E,3);
  put(c.stack+7,0x0080E3,3);ram[c.stack+10]=1;put(c.stack+11,dir?0x018765:0x018708,3);
  put(c.stack+14,0x018792,3);put(c.stack+17,0xC8E4,2);put(c.stack+19,0x80A2,2);
  ram[0x23D]=level;ram[0x100E]=255;ram[0x500]=0xC0;ram[0x520]=10;return c;
}
int main(int argc,char**argv) {
  assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);LagoonFastMenus_Reset(rom,sizeof rom);
  unsigned checks=0;
  for(unsigned dir=0;dir<2;dir++)for(unsigned level=0;level<256;level++) {
    CpuWramReadContext c=frame(dir,level);unsigned legal=dir?level<=14:level>=1&&level<=15;
    memcpy(saved,ram,sizeof ram);assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==(legal?1:3));
    c.interpreter=0;c.instruction_pc=~0u;assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==(legal?1:3));
    c.interpreter=1;c.instruction_pc=0x84E3;
    for(unsigned counter=0;counter<4;counter++)assert(LagoonFastMenus_FilterRead(0x23C,counter,&c,ram)==(legal&&level%3&&counter<=1?0:counter));
    assert(!memcmp(saved,ram,sizeof ram));checks+=6;
  }
  for(unsigned dir=0;dir<2;dir++)for(unsigned b=1;b<=20;b++) {
    CpuWramReadContext c=frame(dir,dir?1:14);ram[c.stack+b]^=64;memcpy(saved,ram,sizeof ram);
    assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==3);c.instruction_pc=0x84E3;
    assert(LagoonFastMenus_FilterRead(0x23C,1,&c,ram)==1);assert(!memcmp(saved,ram,sizeof ram));checks+=2;
  }
  printf("PASS%u native/interpreted physical door fade eligibility/counter/refusal checks; RAM unchanged\n",checks);
}
