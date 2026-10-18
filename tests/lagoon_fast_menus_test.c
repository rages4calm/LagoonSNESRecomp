#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/lagoon_fast_menus.h"
static uint8_t rom[0x15000], ram[0x20000], copy[0x20000];
static void seedrom(void) {
  memset(rom,0,sizeof(rom));
  memcpy(rom+0x91B,"\xAD\x3C\x02\x3A\x8D\x3C\x02",7);
  memcpy(rom+0x3C4,"\xAD\x3D\x02\x29\x80\xC9\x80\xD0\x01\x6B\xAD\x75\x02\x10\x14\xA9\x03\x22\xE0\x84\x00\xAD\x3D\x02\x29\x0F\x3A\x8D\x3D\x02\x8D\x00\x21\xD0\xEC\xA9\x80\x8D\x3D\x02\x8D\x00\x21\x6B",44);
  memcpy(rom+0x458,"\xAD\x3D\x02\xC9\x0F\xD0\x01\x6B\x22\x12\x81\x00\xA9\x00\x8D\x3D\x02\x8D\x00\x21\xA9\x03\x22\xE0\x84\x00\xAD\x3D\x02\x1A\x8D\x3D\x02\x8D\x00\x21\xC9\x0F\xD0\xEC\x6B",41);

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
  /* All six opening callers use the same five-field fade. Test every
   * brightness/counter and every owner-frame byte; no guest writes occur. */
  for(unsigned owner=0;owner<6;owner++) {
    putret(c.stack+1,owner<3?0x0083D8:0x008471);putret(c.stack+4,roots[owner]);
    CpuWramReadContext read=c;read.instruction_pc=0x0084E3;
    for(unsigned level=0;level<256;level++)for(unsigned counter=0;counter<4;counter++) {
      unsigned value=counter==3?255:counter;ram[0x23D]=level;memcpy(copy,ram,sizeof(ram));
      unsigned legal=owner<3?(level>=1&&level<=15):level<=14;
      unsigned expected=legal&&level%3&&value<=1?0:value;
      assert(LagoonFastMenus_FilterRead(0x23C,value,&read,ram)==expected);
      assert(!memcmp(copy,ram,sizeof(ram)));
    }
    ram[0x23D]=owner<3?14:1;
    for(unsigned byte=1;byte<=6;byte++) {
      ram[c.stack+byte]^=64;memcpy(copy,ram,sizeof(ram));
      assert(LagoonFastMenus_FilterRead(0x23C,1,&read,ram)==1);
      assert(!memcmp(copy,ram,sizeof(ram)));ram[c.stack+byte]^=64;
    }
    CpuWramReadContext bad=read;bad.stack=0xff;assert(LagoonFastMenus_FilterRead(0x23C,1,&bad,ram)==1);
    bad=read;bad.stack=0x1fa;assert(LagoonFastMenus_FilterRead(0x23C,1,&bad,ram)==1);
  }
  unsigned closes[]={0x02B73A,0x02B87D,0x02BC9B};
  for(unsigned i=0;i<3;i++)for(unsigned j=0;j<2;j++){
    putret(c.stack+1,j?0x008471:0x0083D8);putret(c.stack+4,j?0x02C675:0x02C65E);
    putret(c.stack+7,0x0080E3);ram[c.stack+10]=1;putret(c.stack+11,j?0x018765:0x018708);
    putret(c.stack+14,0x0080E3);ram[c.stack+17]=2;putret(c.stack+18,closes[i]);
    ram[0x23D]=j?0:15;
    assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==1);
    ram[c.stack+18]^=1;assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==3);ram[c.stack+18]^=1;
    ram[c.stack+10]=0;assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==3);ram[c.stack+10]=1;
  }

  /* Exact close frames: five real waits while all fifteen brightness levels
   * still run. Bad levels and every frame byte refuse without writes. */
  for(unsigned owner=0;owner<3;owner++)for(unsigned direction=0;direction<2;direction++){
    putret(c.stack+1,direction?0x008471:0x0083D8);putret(c.stack+4,direction?0x02C675:0x02C65E);
    putret(c.stack+7,0x0080E3);ram[c.stack+10]=1;putret(c.stack+11,direction?0x018765:0x018708);
    putret(c.stack+14,0x0080E3);ram[c.stack+17]=2;putret(c.stack+18,closes[owner]);
    unsigned waits=0;
    for(unsigned level=0;level<256;level++){
      ram[0x23D]=level;memcpy(copy,ram,sizeof(ram));CpuWramReadContext before=c;
      unsigned expected=(direction?level<=14:level>=1&&level<=15)?1:3;
      assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==expected);
      assert(!memcmp(copy,ram,sizeof(ram))&&!memcmp(&before,&c,sizeof(c)));
      CpuWramReadContext read=c;read.instruction_pc=0x0084E3;
      for(unsigned counter=0;counter<4;counter++){
        unsigned value=counter==3?255:counter;
        unsigned result=expected==1&&level%3&&value<=1?0:value;
        assert(LagoonFastMenus_FilterRead(0x23C,value,&read,ram)==result);
      }
      assert(!memcmp(copy,ram,sizeof(ram))&&!memcmp(&before,&c,sizeof(c)));
      if(expected!=3)waits+=level%3?0:1;
    }
    assert(waits==5);
    ram[0x23D]=direction?0:15;
    for(unsigned frame=1;frame<=20;frame++){
      ram[c.stack+frame]^=0x40;memcpy(copy,ram,sizeof(ram));
      assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==3);assert(!memcmp(copy,ram,sizeof(ram)));
      ram[c.stack+frame]^=0x40;
    }
    CpuWramReadContext wrong=c;wrong.stack=0xff;assert(LagoonFastMenus_Filter(0x23C,3,&wrong,ram)==3);
    wrong=c;wrong.stack=0x1eb;assert(LagoonFastMenus_Filter(0x23C,3,&wrong,ram)==3);
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
  unsigned sites[]={0x91B,0x4E0,0x3D3,0x46C,0x136B6,0x136E3,0x137E0,0x13814,0x13C41,0x13C62,0x1465B,0x14672,0x8703,0x8760,0x13737,0x1387A,0x13C98};
  for(unsigned i=0;i<sizeof(sites)/sizeof(*sites);++i){seedrom();rom[sites[i]]^=1;LagoonFastMenus_Reset(rom,sizeof(rom));LagoonFastMenus_Activate();assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==3);}
  for(unsigned site=0;site<2;site++)for(unsigned byte=0;byte<(site?41:44);byte++){
    seedrom();rom[(site?0x458:0x3C4)+byte]^=1;LagoonFastMenus_Reset(rom,sizeof(rom));
    assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==3);
  }
  /* Read refusal is observational: unknown instructions, nonnative modes,
   * wrong frames and unexpected counters never clear the real timer. */
  seedrom();LagoonFastMenus_Reset(rom,sizeof(rom));
  putret(c.stack+1,0x0083D8);putret(c.stack+4,0x02C65E);putret(c.stack+7,0x0080E3);ram[c.stack+10]=1;putret(c.stack+11,0x018708);putret(c.stack+14,0x0080E3);ram[c.stack+17]=2;putret(c.stack+18,closes[0]);ram[0x23D]=14;
  CpuWramReadContext read=c;read.instruction_pc=0x0084E3;
  assert(LagoonFastMenus_FilterRead(0x23C,1,&read,ram)==0);
  assert(LagoonFastMenus_FilterRead(0x23C,0,&read,ram)==0);
  assert(LagoonFastMenus_FilterRead(0x23C,255,&read,ram)==255);
  for(unsigned badcase=0;badcase<28;badcase++){
    CpuWramReadContext badread=read;unsigned addr=0x23C;
    if(badcase<20)ram[read.stack+badcase+1]^=64;
    else switch(badcase){case 20:badread.instruction_pc=0x00891B;break;case 21:badread.interpreter=0;break;case 22:badread.m=0;break;case 23:badread.x=1;break;case 24:badread.e=1;break;case 25:badread.pb=1;break;case 26:addr++;break;case 27:badread.stack=0x1eb;break;}
    memcpy(copy,ram,sizeof(ram));assert(LagoonFastMenus_FilterRead(addr,1,&badread,ram)==1);assert(!memcmp(copy,ram,sizeof(ram)));
    if(badcase<20)ram[read.stack+badcase+1]^=64;
  }
  for(unsigned byte=0;byte<7;byte++){seedrom();rom[0x91B+byte]^=1;LagoonFastMenus_Reset(rom,sizeof(rom));assert(LagoonFastMenus_FilterRead(0x23C,1,&read,ram)==1);}
  putret(c.stack+1,0x0083D8);putret(c.stack+4,0x02B6B9);
  seedrom();LagoonFastMenus_Reset(rom,0x13817);LagoonFastMenus_Activate();assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==3);
  LagoonFastMenus_Reset(NULL,0);LagoonFastMenus_Activate();assert(LagoonFastMenus_Filter(0x23C,3,&c,ram)==3);
  puts("PASS core default, six menu contexts, counter45->15, immutable RAM, wrong context/pairs, ROM mutations, short/null ROM, reset");
  return 0;
}
