/* Original8869 BG/actor/common/icon loaders and real NMI/RTI. Hardware DMA is
 * modeled; all actor art comes from the external ROM, never embedded bytes. */
#define main original_parallax_main
#define writeb old_writeb
#include "test_wide_parallax_sources.c"
#undef writeb
#undef main
static uint8_t captured[0x20000],actor_output[8324];
static uint16_t actual[32768],expected[32768];
static unsigned prefix_refusals,source_refusals,partial_refusals,metadata_refusals;
static unsigned actor_bytes,bg_bytes;
static void read_prefix(const char *prefix,const char *suffix,void *out,size_t n) {
  char name[4096];snprintf(name,sizeof name,"%s.%s",prefix,suffix);
  FILE *f=fopen(name,"rb");assert(f&&fread(out,1,n,f)==n);fclose(f);
}
static void hardware(void *ctx,uint32_t address,uint8_t value) {
  (void)ctx;unsigned b=address>>16,a=address&65535;
  if(b==126||b==127){ram[(b-126)*65536+a]=value;return;}
  if((b&127)<64&&a<8192){ram[a]=value;return;}
  assert((b&127)<64&&a<32768);io[a]=value;
  if(a==0x420b) {
    assert(value==2&&io[0x4310]==1&&io[0x4311]==0x18);
    unsigned from=word(io,0x4312),bank=io[0x4314],to=word(io,0x2116),length=word(io,0x4315);
    assert(bank==127&&length&&length<=4096);
    for(unsigned n=0;n<length;++n)vram[((to*2+n)&65535)]=readb(NULL,(bank<<16)|((from+n)&65535));
    if(to>=0x4400&&to<0x5900)actor_bytes+=length;
    else if(to<0x1be0)bg_bytes+=length;
    else assert((to>=0x4240&&to<0x4300)||(to>=0x4300&&to<0x4400)||to>=0x5800);
    ++jobs;
  }
}
static void original_load(Interp816 *c,unsigned marker) {
  memcpy(ram,captured,sizeof ram);memset(io,0,sizeof io);memset(vram,marker,sizeof vram);
  memset(ram+0x17f80+8324,marker,0x2a00-8324);
  initcpu(c,1,0x8869,0x1d0,2);unsigned steps=0;actor_bytes=bg_bytes=0;
  while(c->k!=1||c->pc!=0x7ff0) {
    assert(++steps<3000000);
    if(c->k==1&&c->pc==0x888a) {
      assert(word(ram,0xf9)==8324);memcpy(actor_output,ram+0x17f80,8324);
      assert(fnv(actor_output,8324)==UINT64_C(0xc169d6bda862bdc1));
    }
    if(c->k==0&&c->pc==0x89ac&&(ram[0x23a]&32))c->nmiWanted=true;
    interp816_runOpcode(c);
  }
  assert(c->sp==0x1d2&&actor_bytes==0x2a00&&bg_bytes==0x37c0);
  assert(!memcmp(vram+0x8800,actor_output,8324));
}
static void refused(LagoonWideAtlas *a) {
  LagoonWideAtlas before=*a;
  assert(!LagoonWideAtlas_RebindResident(a,rom,sizeof rom,captured,actual,45));
  assert(!memcmp(a,&before,sizeof before));
}
int main(int argc,char **argv) {
  assert(argc==3);FILE *f=fopen(argv[1],"rb");
  assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  read_prefix(argv[2],"wram.bin",captured,sizeof captured);
  read_prefix(argv[2],"vram.bin",actual,sizeof actual);
  /* Enter the loader with no unrelated preexisting pending PPU job. The
   * captured post-load fixture supplies this precondition without edits. */
  assert(word(captured,0x48e)==0x14&&!captured[0x100e]&&!captured[0x23a]);
  Interp816 *c=interp816_init(NULL,readb,hardware);assert(c);
  original_load(c,0xa5);memcpy(expected,vram,sizeof expected);
  original_load(c,0x5a);
  assert(!memcmp(vram+0x8800,(uint8_t*)expected+0x8800,8324));
  assert(memcmp(vram+0xa884,(uint8_t*)expected+0xa884,0x2a00-8324));
  assert(!memcmp(vram+0x8800,(uint8_t*)actual+0x8800,8324));
  LagoonWideAtlas a={0},empty={0};
  assert(LagoonWideAtlas_RebindResident(&a,rom,sizeof rom,captured,actual,45));
  assert(a.ready&&a.completed==8324&&!a.transfer_left&&a.generation==45);
  for(unsigned b=0;b<0x2a00;++b)assert(a.owned[b]==(b<8320));
  LagoonWideCell first={-180,30,0x2040,8},last={-180,30,0x2143,8};
  assert(LagoonWideAtlas_CellOwned(&a,0x4000,0x5000,&first));
  assert(LagoonWideAtlas_CellOwned(&a,0x4000,0x5000,&last));
  LagoonWideCell mixed={-180,30,0x2143,16},tail={-180,30,0x2144,8};
  assert(!LagoonWideAtlas_CellOwned(&a,0x4000,0x5000,&mixed));
  assert(!LagoonWideAtlas_CellOwned(&a,0x4000,0x5000,&tail));
  for(unsigned b=0;b<8324;++b) {
    ((uint8_t*)actual)[0x8800+b]^=1;refused(&empty);
    ((uint8_t*)actual)[0x8800+b]^=1;++prefix_refusals;
  }
  for(unsigned b=8324;b<0x2a00;++b) {
    ((uint8_t*)actual)[0x8800+b]^=1;a=empty;
    assert(LagoonWideAtlas_RebindResident(&a,rom,sizeof rom,captured,actual,45));
    assert(!LagoonWideAtlas_CellOwned(&a,0x4000,0x5000,&tail));
    ((uint8_t*)actual)[0x8800+b]^=1;
  }
  const unsigned spans[][2]={{0x05b73f,56},{0x18869,100},{0x9143,445},{0x188cd,20},
    {0x908a,81},{0x884b,56},{0x89a7,13},{0x87b4,120},{0x94a1,69},{0x94e6,28},
    {0x19008,52},{0x190bb,56},{0x19114,17},{0x13c716,4143},{0x1c92f5,6836}};
  for(unsigned s=0;s<sizeof spans/sizeof*spans;++s)for(unsigned b=0;b<spans[s][1];++b) {
    rom[off(spans[s][0])+b]^=1;refused(&empty);rom[off(spans[s][0])+b]^=1;++source_refusals;
  }
  for(unsigned b=0;b<56;++b){captured[0x50+b]^=1;refused(&empty);captured[0x50+b]^=1;++metadata_refusals;}
  for(unsigned n=1;n<=0x2a00;++n){a=empty;a.completed=n;refused(&a);++partial_refusals;}
  a=empty;a.transfer_left=1;refused(&a);++partial_refusals;
  a=empty;assert(LagoonWideAtlas_Begin(&a,45,0x14,0x1c92f5,127,0x7f80,0x4400,4096));
  LagoonWideAtlas_Write(&a,0x8800);refused(&a);++partial_refusals;
  a=empty;assert(LagoonWideAtlas_RebindResident(&a,rom,sizeof rom,captured,actual,45));
  LagoonWideAtlas_Write(&a,0x8800);assert(!LagoonWideAtlas_CellOwned(&a,0x4000,0x5000,&first));
  assert(!LagoonWideAtlas_RebindResident(&empty,rom,sizeof rom,captured,actual,0));
  captured[0x100e]=1;refused(&empty);captured[0x100e]=0;
  captured[0x48e]=0x15;refused(&empty);captured[0x48e]=0x14;
  interp816_free(c);
  printf("loaded room14: original8869 BG/actor/common/icon+NMI/RTI DMA twice, complete8324-byte actor identity, ONLY8320 complete tile bytes owned; unknown buffer tails differ and all2428 resident tail mutants remain unowned; %u prefix/%u source/%u partial/%u descriptor refusals, mixedtile/write/generation/loading/room15 guards PASS\n",
      prefix_refusals,source_refusals,partial_refusals,metadata_refusals);return 0;
}
