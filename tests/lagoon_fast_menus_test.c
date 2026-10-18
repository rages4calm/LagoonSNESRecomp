#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/lagoon_fast_menus.h"
static uint8_t rom[0x15000], ram[0x20000], copy[0x20000];
static void seedrom(void) {
  memset(rom,0,sizeof(rom));
  memcpy(rom+0x4E0,"\x8D\x3C\x02\xAD\x3C\x02\xD0\xFB\x6B",9);
  memcpy(rom+0x3D3,"\xA9\x03\x22\xE0\x84\x00",6);
  memcpy(rom+0x46C,"\xA9\x03\x22\xE0\x84\x00",6);
  memcpy(rom+0x136B6,"\x22\xC4\x83\x00",4);memcpy(rom+0x136E3,"\x22\x58\x84\x00",4);
  memcpy(rom+0x137E0,"\x22\xC4\x83\x00",4);memcpy(rom+0x13814,"\x22\x58\x84\x00",4);
  memcpy(rom+0x13C41,"\x22\xC4\x83\x00",4);memcpy(rom+0x13C62,"\x22\x58\x84\x00",4);
  memcpy(rom+0x1465B,"\x22\xC4\x83\x00",4);memcpy(rom+0x14672,"\x22\x58\x84\x00",4);
  memcpy(rom+0x8703,"\xA9\x1B\x22\xB9\x80\x00",6);memcpy(rom+0x8760,"\xA9\x1C\x22\xB9\x80\x00",6);
  memcpy(rom+0x13737,"\x22\xB3\x80\x00",4);memcpy(rom+0x1387A,"\x22\xB3\x80\x00",4);memcpy(rom+0x13C98,"\x22\xB3\x80\x00",4);
}
static void putret(unsigned s,unsigned a) {ram[s]=a;ram[s+1]=a>>8;ram[s+2]=a>>16;}
int main(void) {
  seedrom();CpuWramReadContext c={.instruction_pc=0x0084E0,.stack=0x1E0,.m=1,.interpreter=1};
  putret(c.stack+1,0x0083D8);putret(c.stack+4,0x02B6B9);
  LagoonFastMenus_Reset(rom,sizeof(rom));assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==1);
  LagoonFastMenus_Activate();assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==1);
  unsigned roots[]={0x02B6B9,0x02B7E3,0x02BC44,0x02B6E6,0x02B817,0x02BC65};
  for(unsigned i=0;i<6;++i) {putret(c.stack+1,i<3?0x0083D8:0x008471);putret(c.stack+4,roots[i]);assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==1);}
  unsigned closes[]={0x02B73A,0x02B87D,0x02BC9B};
  for(unsigned i=0;i<3;i++)for(unsigned j=0;j<2;j++){
    putret(c.stack+1,j?0x008471:0x0083D8);putret(c.stack+4,j?0x02C675:0x02C65E);
    putret(c.stack+7,0x0080E3);ram[c.stack+10]=1;putret(c.stack+11,j?0x018765:0x018708);
    putret(c.stack+14,0x0080E3);ram[c.stack+17]=2;putret(c.stack+18,closes[i]);
    assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==1);
    ram[c.stack+18]^=1;assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==3);ram[c.stack+18]^=1;
    ram[c.stack+10]=0;assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==3);ram[c.stack+10]=1;
  }
  putret(c.stack+1,0x0083D8);putret(c.stack+4,0x02B6E6);assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==3);
  putret(c.stack+4,0x02B6B9);memcpy(copy,ram,sizeof(ram));
  unsigned ticks=0;for(unsigned level=0;level<15;++level){unsigned timer=LagoonFastMenus_Filter(0x23C,3,&c,ram);while(timer){--timer;++ticks;}}
  assert(ticks==15 && !memcmp(copy,ram,sizeof(ram))); /* stock15*3=45, core15*1=15 */
  assert(LagoonFastMenus_Filter(0x23D,3,&c,ram)==3);assert(LagoonFastMenus_Filter(0x23C,1,&c,ram)==1);
  assert(LagoonFastMenus_Filter(0x23C,3,NULL,ram)==3);assert(LagoonFastMenus_Filter(0x23C,3,&c,NULL)==3);
  CpuWramReadContext bad=c;
  bad.interpreter=0;assert(LagoonFastMenus_Filter(0x23C,3,&bad,ram)==3);
  bad=c;bad.instruction_pc++;assert(LagoonFastMenus_Filter(0x23C,3,&bad,ram)==3);
  bad=c;bad.m=0;assert(LagoonFastMenus_Filter(0x23C,3,&bad,ram)==3);
  bad=c;bad.x=1;assert(LagoonFastMenus_Filter(0x23C,3,&bad,ram)==3);
  bad=c;bad.e=1;assert(LagoonFastMenus_Filter(0x23C,3,&bad,ram)==3);
  bad=c;bad.pb=0x80;assert(LagoonFastMenus_Filter(0x23C,3,&bad,ram)==3);
  bad=c;bad.stack=0xFFFD;assert(LagoonFastMenus_Filter(0x23C,3,&bad,ram)==3);
  LagoonFastMenus_Reset(rom,sizeof(rom));assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==1);
  unsigned sites[]={0x4E0,0x3D3,0x46C,0x136B6,0x136E3,0x137E0,0x13814,0x13C41,0x13C62,0x1465B,0x14672,0x8703,0x8760,0x13737,0x1387A,0x13C98};
  for(unsigned i=0;i<sizeof(sites)/sizeof(*sites);++i){seedrom();rom[sites[i]]^=1;LagoonFastMenus_Reset(rom,sizeof(rom));LagoonFastMenus_Activate();assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==3);}
  seedrom();LagoonFastMenus_Reset(rom,0x13817);LagoonFastMenus_Activate();assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==3);
  LagoonFastMenus_Reset(NULL,0);LagoonFastMenus_Activate();assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==3);
  puts("PASS core default, six menu contexts, counter45->15, immutable RAM, wrong context/pairs, ROM mutations, short/null ROM, reset");
  return 0;
}
