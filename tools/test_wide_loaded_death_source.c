/* Original common0 selector/decompressor/queue/NMI/RTI and actual512-byte DMA
 * prove successful-load resource recovery. Hardware DMA alone is modeled. */
#define main original_parallax_main
#define writeb previous_writeb
#include "test_wide_parallax_sources.c"
#undef writeb
#undef main
static uint16_t resident[32768];
static unsigned source_refusals,resident_refusals,partial_refusals;
static void hardware(void *ctx,uint32_t address,uint8_t value) {
  (void)ctx;unsigned bank=address>>16,a=address&65535;
  if(bank==126||bank==127){ram[(bank-126)*65536+a]=value;return;}
  if((bank&127)<64&&a<8192){ram[a]=value;return;}
  assert((bank&127)<64&&a<32768);io[a]=value;
  if(a==0x4202||a==0x4203) {
    unsigned result=io[0x4202]*io[0x4203];io[0x4216]=result;io[0x4217]=result>>8;
  }
  if(a==0x4206) {
    unsigned result=value?word(io,0x4204)/value:65535;io[0x4214]=result;io[0x4215]=result>>8;
  }
  if(a==0x420b) {
    assert(value==2&&io[0x4310]==1&&io[0x4311]==0x18);
    unsigned from=word(io,0x4312),source_bank=io[0x4314];
    unsigned to=word(io,0x2116),length=word(io,0x4315);
    assert(source_bank==127&&from==0x7f80&&to==0x4300&&length==512);
    for(unsigned n=0;n<length;++n)vram[to*2+n]=readb(NULL,(source_bank<<16)|((from+n)&65535));
    ++jobs;
  }
}
static void refused(LagoonWideDeathAtlas *a,uint64_t generation) {
  LagoonWideDeathAtlas before=*a;
  assert(!LagoonWideDeath_RebindResident(a,resident,rom,sizeof rom,generation));
  assert(!memcmp(a,&before,sizeof before));
}
int main(int argc,char **argv) {
  assert(argc==2);FILE *f=fopen(argv[1],"rb");
  assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  Interp816 *c=interp816_init(NULL,readb,hardware);assert(c);
  initcpu(c,0,0x94a1,0x1d0,3);c->db=1;unsigned instructions=0;
  while(c->k!=0||c->pc!=0x7ff0) {
    assert(++instructions<1000000);
    if(c->k==0&&c->pc==0x9143){source=ram[0xe0]|ram[0xe1]<<8|ram[0xe2]<<16;maximum=source;}
    if(c->k==0&&c->pc==0x89ac&&(ram[0x23a]&32))c->nmiWanted=true;
    interp816_runOpcode(c);
  }
  assert(c->sp==0x1d3&&jobs==1&&source==0x11be00&&maximum-source+1==168);
  assert(fnv(vram+0x8600,512)==UINT64_C(0x84462e34625605fe));
  memcpy(resident,vram,sizeof resident);LagoonWideDeathAtlas empty={0},a=empty;
  assert(LagoonWideDeath_RebindResident(&a,resident,rom,sizeof rom,43));
  assert(a.ready&&a.completed==512&&!a.left&&a.next==0x8800&&a.generation==43);
  assert(!memcmp(a.bytes,vram+0x8600,512));
  assert(LagoonWideDeath_Match(&a,resident,rom,sizeof rom,43));
  assert(!LagoonWideDeath_Match(&a,resident,rom,sizeof rom,44));
  for(unsigned b=0;b<512;++b) {
    ((uint8_t*)resident)[0x8600+b]^=1;refused(&empty,43);
    ((uint8_t*)resident)[0x8600+b]^=1;++resident_refusals;
    LagoonWideDeathAtlas written=a;LagoonWideDeath_Write(&written,0x8600+b,a.bytes[b]);
    assert(!LagoonWideDeath_Match(&written,resident,rom,sizeof rom,43));++resident_refusals;
  }
  const unsigned spans[][2]={{0x11be00,168},{0x94e6,7},{0x94a1,69},{0x18893,6},
    {0x9143,445},{0x908a,81},{0x884b,56},{0x89a7,13},{0x87b4,120}};
  for(unsigned s=0;s<sizeof spans/sizeof*spans;++s)for(unsigned b=0;b<spans[s][1];++b) {
    rom[off(spans[s][0])+b]^=1;refused(&empty,43);
    rom[off(spans[s][0])+b]^=1;++source_refusals;
  }
  for(unsigned n=0;n<=512;++n) {
    LagoonWideDeathAtlas partial={0};partial.completed=n;partial.left=512-n;
    refused(&partial,43);++partial_refusals;
  }
  refused(&a,43);refused(&empty,0);
  assert(!LagoonWideDeath_RebindResident(NULL,resident,rom,sizeof rom,43));
  assert(!LagoonWideDeath_RebindResident(&empty,NULL,rom,sizeof rom,43));
  assert(!LagoonWideDeath_RebindResident(&empty,resident,NULL,sizeof rom,43));
  assert(!LagoonWideDeath_RebindResident(&empty,resident,rom,sizeof rom-1,43));
  interp816_free(c);
  printf("loaded death common0: original94A1 selector0/decompress/queue/NMI/RTI and one512-byte DMA; complete resident positive; %u resident/write/%u source/%u partial readonly refusals; generation/zero/null/size guards PASS\n",
      resident_refusals,source_refusals,partial_refusals);
  return 0;
}
