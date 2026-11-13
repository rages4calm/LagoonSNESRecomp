/* Real-ROM metadata/OAM binding plus cosmetic bounds; no guest execution. */
#include "../src/lagoon_cloud_view75.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static uint8_t rom[0x100000],ram[0x20000],oam[512],high[32];
static void put(unsigned a,unsigned v){ram[a]=(uint8_t)v;ram[a+1]=(uint8_t)(v>>8);}
static unsigned build(unsigned style,int x,int y){
  LagoonWideCell cells[64];int n=LagoonWideEntities_Decode(rom,sizeof rom,0x29,0xc+style,x,y,cells,64);
  assert(n>0);memset(oam,0,sizeof oam);memset(high,0,sizeof high);unsigned slot=8;
  for(int i=0;i<n;i++){
    const LagoonWideCell*c=&cells[i];if(c->y < -16||c->y>=240)continue;
    oam[slot*4]=(uint8_t)c->x;oam[slot*4+1]=(uint8_t)c->y;
    oam[slot*4+2]=(uint8_t)c->tile;oam[slot*4+3]=(uint8_t)(c->tile>>8);
    unsigned bits=(((unsigned)c->x>>8)&1)|(c->size==16?2:0);
    high[slot/4]|=(uint8_t)(bits<<((slot%4)*2));slot++;
  }
  return slot-8;
}
int main(int argc,char**argv){
  assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);assert(fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  assert(LagoonCloud75_Source(rom,sizeof rom));put(0x48e,0x28);ram[0x390]=255;put(0x480,880);put(0x482,1824);
  unsigned cases=0,mutations=0;
  for(unsigned style=0;style<2;style++)for(int x=-176;x<=250;x+=71)for(int y=-32;y<=220;y+=63)for(unsigned phase=0;phase<2;phase++){
    ram[0x391]=(uint8_t)style;put(0x392,880+x+(phase?6:0));put(0x394,1824+y);
    unsigned n=build(style,x,y);LagoonCloudView75 c;
    int got=LagoonCloudView75_Capture(&c,ram,rom,sizeof rom,oam,high);
    if(!n){assert(!got);continue;}
    assert(got&&c.matched==n&&c.anchor_x==x&&c.anchor_y==y);
    int xx,yy;LagoonCloudView75_RemapAnchor(&c,0,0,256,224,&xx,&yy);assert(xx==x&&yy==y);
    oam[8*4+2]^=1;assert(!LagoonCloudView75_Capture(&c,ram,rom,sizeof rom,oam,high));oam[8*4+2]^=1;
    cases++;
  }
  {
    LagoonCloudView75 c;ram[0x391]=1;put(0x392,880+50);put(0x394,1824+20);build(1,50,20);
    assert(LagoonCloudView75_Capture(&c,ram,rom,sizeof rom,oam,high));
    ram[0x390]=0;assert(!LagoonCloudView75_Capture(&c,ram,rom,sizeof rom,oam,high));ram[0x390]=255;
    put(0x48e,0x29);assert(!LagoonCloudView75_Capture(&c,ram,rom,sizeof rom,oam,high));put(0x48e,0x28);
    ram[0x391]=0;assert(!LagoonCloudView75_Capture(&c,ram,rom,sizeof rom,oam,high));ram[0x391]=1;
    high[2]^=1;assert(!LagoonCloudView75_Capture(&c,ram,rom,sizeof rom,oam,high));high[2]^=1;
    put(0x392,880+70);assert(!LagoonCloudView75_Capture(&c,ram,rom,sizeof rom,oam,high));
    put(0x392,880+256);build(1,250,20);ram[0x390]=0;
    assert(LagoonCloudView75_Capture(&c,ram,rom,sizeof rom,oam,high)&&c.anchor_x==250);ram[0x390]=255;
  }
  for(unsigned i=0;i<3;i++){rom[0x1a18+i]^=1;assert(!LagoonCloud75_Source(rom,sizeof rom));rom[0x1a18+i]^=1;mutations++;}
  for(unsigned i=0;i<150;i++){rom[0x14701+i]^=1;assert(!LagoonCloud75_Source(rom,sizeof rom));rom[0x14701+i]^=1;mutations++;}
  for(unsigned i=0;i<728;i++){rom[0xd5b6+i]^=1;assert(!LagoonCloud75_Source(rom,sizeof rom));rom[0xd5b6+i]^=1;mutations++;}
  LagoonCloudView75 c={0};int x,y;c.anchor_x=-176;c.anchor_y=-32;
  LagoonCloudView75_RemapAnchor(&c,-128,-112,512,336,&x,&y);assert(x==-304&&y==-144);
  c.anchor_x=256;c.anchor_y=224;LagoonCloudView75_RemapAnchor(&c,-128,-112,512,336,&x,&y);assert(x==384&&y==224);
  printf("PASS cloud75 %u exact OAM/phase/identity cases, %u source mutations, expanded travel endpoints\n",cases,mutations);
}
