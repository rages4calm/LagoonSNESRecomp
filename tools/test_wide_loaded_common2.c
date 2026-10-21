/* Private actual room09 snapshot, original NPC predicates and original source
 * decompression prove the full common2 overlap. No embedded game assets. */
#define main original_parallax_main
#include "test_wide_parallax_sources.c"
#undef main
static uint8_t captured[0x20000];
static uint16_t expected[32768],actual[32768];
static unsigned decoded,source_refusals,resident_refusals,metadata_refusals;
static void read_prefix(const char *prefix,const char *suffix,void *out,size_t n) {
  char name[4096];snprintf(name,sizeof name,"%s.%s",prefix,suffix);
  FILE *f=fopen(name,"rb");assert(f&&fread(out,1,n,f)==n);fclose(f);
}
static void execute(Interp816 *c,unsigned bank,unsigned pc,unsigned depth) {
  initcpu(c,bank,pc,0x1d0,depth);if(depth==3)ram[0x1d3]=1;
  unsigned count=0;
  while(c->k!=1||c->pc!=0x7ff0) {
    assert(++count<3000000);interp816_runOpcode(c);
  }
}
static void decode(Interp816 *c,unsigned address) {
  source=maximum=address;ram[0xe0]=address;ram[0xe1]=address>>8;ram[0xe2]=address>>16;
  execute(c,0,0x9143,3);++decoded;
}
static void refused(LagoonWideAtlas *a) {
  LagoonWideAtlas before=*a;
  assert(!LagoonWideAtlas_RebindResident(a,rom,sizeof rom,captured,actual,42));
  assert(!memcmp(a,&before,sizeof before));
}
int main(int argc,char **argv) {
  assert(argc==3);FILE *f=fopen(argv[1],"rb");
  assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  read_prefix(argv[2],"wram.bin",captured,sizeof captured);
  read_prefix(argv[2],"vram.bin",actual,sizeof actual);
  assert(word(captured,0x48e)==9&&!captured[0x100e]);
  Interp816 *c=interp816_init(NULL,readb,writeb);assert(c);
  memcpy(ram,captured,sizeof ram);execute(c,1,0x89ab,2);
  assert((c->a&255)==255); /* Actual original NPC0 predicate. */
  memcpy(ram,captured,sizeof ram);execute(c,1,0x89b3,2);
  assert(!(c->a&255)); /* No later NPC1/3 common3 replacement. */
  memset(ram,0,sizeof ram);memset(io,0,sizeof io);
  memcpy(ram+0x50,rom+off(0x05b2df+9*56),56);
  decode(c,ram[0x57]|ram[0x58]<<8|ram[0x59]<<16);assert(word(ram,0xf9)==11748);
  decode(c,ram[0x5d]|ram[0x5e]<<8|ram[0x5f]<<16);
  memcpy((uint8_t*)expected+0x8800,ram+0x17f80,kLagoonWideAtlasBytes);
  assert(fnv((uint8_t*)expected+0x8800,kLagoonWideAtlasBytes)==UINT64_C(0xf4067c2896de3c27));
  LagoonWideAtlas empty={0},a=empty;
  assert(LagoonWideAtlas_RebindResident(&a,rom,sizeof rom,captured,expected,42));
  unsigned table=off(0x0094e6)+2*7;
  unsigned ptr=rom[table]|rom[table+1]<<8|rom[table+2]<<16;
  unsigned destination=word(rom,table+3),length=word(rom,table+5);
  assert(ptr==0x11c2f3&&destination==0x5800&&length==0x6e0);
  decode(c,ptr);assert(maximum-source+1==1478&&word(ram,0xf9)==1764);
  memcpy((uint8_t*)expected+destination*2,ram+0x17f80,length);
  assert(!memcmp((uint8_t*)expected+0x8800,(uint8_t*)actual+0x8800,kLagoonWideAtlasBytes));
  assert(!memcmp((uint8_t*)expected+destination*2,(uint8_t*)actual+destination*2,length));
  a=empty;assert(LagoonWideAtlas_RebindResident(&a,rom,sizeof rom,captured,actual,42));
  assert(a.ready&&a.completed==kLagoonWideAtlasBytes&&a.generation==42);
  for(unsigned b=0x8800;b<0xb6e0;++b) {
    ((uint8_t*)actual)[b]^=1;refused(&empty);((uint8_t*)actual)[b]^=1;++resident_refusals;
  }
  const unsigned spans[][2]={{0x018869,100},{0x0189ab,74},{0x0094a1,69},
    {0x0094f4,7},{0x11c2f3,1478}};
  for(unsigned s=0;s<sizeof spans/sizeof*spans;++s)for(unsigned b=0;b<spans[s][1];++b) {
    rom[off(spans[s][0])+b]^=1;refused(&empty);rom[off(spans[s][0])+b]^=1;++source_refusals;
  }
  for(unsigned b=0;b<56;++b) {
    captured[0x50+b]^=1;refused(&empty);captured[0x50+b]^=1;++metadata_refusals;
  }
  /* Every active record is scanned by the original predicate; a later common3
   * qualifier must refuse the common2 form regardless of its entity slot. */
  for(unsigned off=0x40;off<0x840;off+=64)for(unsigned kind=1;kind<=3;kind+=2) {
    uint8_t saved[64];memcpy(saved,captured+0x500+off,64);
    captured[0x500+off]=0xc0;captured[0x518+off]=1;captured[0x51b+off]=kind;
    refused(&empty);memcpy(captured+0x500+off,saved,64);++metadata_refusals;
  }
  uint8_t saved[0x800];memcpy(saved,captured+0x540,sizeof saved);
  for(unsigned off=0x40;off<0x840;off+=64)
    if(captured[0x518+off]==1&&captured[0x51b+off]==0)captured[0x500+off]=0;
  refused(&empty);memcpy(captured+0x540,saved,sizeof saved);++metadata_refusals;
  a=empty;a.completed=1;refused(&a);a=empty;a.transfer_left=1;refused(&a);
  a=empty;a.ready=1;refused(&a);metadata_refusals+=3;
  LagoonWideCell tile={-180,30,0x2780,8};a=empty;
  assert(LagoonWideAtlas_RebindResident(&a,rom,sizeof rom,captured,actual,42));
  assert(LagoonWideAtlas_CellOwned(&a,0x4000,0x5000,&tile));
  LagoonWideAtlas_Write(&a,0xb000);
  assert(!LagoonWideAtlas_CellOwned(&a,0x4000,0x5000,&tile));
  interp816_free(c);
  printf("loaded common2: original NPC0/NPC1-or3 predicates, %u original009143 decodes, complete actual09 atlas+common2 upload exact; %u resident/%u source/%u metadata/partial readonly refusals; later-write tile revocation PASS\n",
      decoded,resident_refusals,source_refusals,metadata_refusals);
  return 0;
}
