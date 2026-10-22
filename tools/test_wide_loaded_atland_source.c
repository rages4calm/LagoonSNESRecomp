/* Exact original8869 loader, real NMI/RTI and dictionary decompressor. Only
 * hardware DMA is modeled. No ROM, art or captured resource bytes are stored. */
#define main old_parallax_test_main
#include "test_wide_parallax_sources.c"
#undef main
static uint8_t fixture[0x20000],actor_writes[0x2a00],actor_output[10048];
static uint16_t resident[32768],first[32768];
static unsigned track_actor,actor_dma,bg_dma,byte_refusals,source_refusals,metadata_refusals,tail_checks;
static void read_fixture(const char *prefix,const char *suffix,void *out,size_t n) {
  char path[4096];snprintf(path,sizeof(path),"%s.%s",prefix,suffix);
  FILE *f=fopen(path,"rb");assert(f&&fread(out,1,n,f)==n);fclose(f);
}
static void hardware(void *ctx,uint32_t address,uint8_t value) {
  (void)ctx;unsigned b=address>>16,a=address&65535;
  if(b==126||b==127) {
    unsigned at=(b-126)*65536+a;ram[at]=value;
    if(track_actor&&at>=0x17f80&&at<0x1a980)actor_writes[at-0x17f80]=1;
    return;
  }
  if((b&127)<64&&a<8192){ram[a]=value;return;}
  assert((b&127)<64&&a<32768);io[a]=value;
  if(a==0x420b) {
    assert(value==2&&io[0x4310]==1&&io[0x4311]==0x18);
    unsigned from=word(io,0x4312),bank=io[0x4314],to=word(io,0x2116),n=word(io,0x4315);
    assert(bank==127&&n&&n<=4096);
    for(unsigned i=0;i<n;++i)vram[(to*2+i)&65535]=readb(NULL,(bank<<16)|((from+i)&65535));
    if(to>=0x4400&&to<0x5900)actor_dma+=n;
    else if(to<0x1be0)bg_dma+=n;
    else assert((to>=0x4240&&to<0x4400)||to>=0x5800);
  }
}
static void original_load(Interp816 *c,unsigned marker) {
  memcpy(ram,fixture,sizeof(ram));memset(io,0,sizeof(io));memset(vram,marker,sizeof(vram));
  memset(ram+0x17f80+10048,marker,0x2a00-10048);memset(actor_writes,0,sizeof(actor_writes));
  initcpu(c,1,0x8869,0x1d0,2);unsigned steps=0;actor_dma=bg_dma=track_actor=0;
  while(c->k!=1||c->pc!=0x7ff0) {
    assert(++steps<3000000);
    if(c->k==0&&c->pc==0x9143) {
      unsigned src=ram[0xe0]|ram[0xe1]<<8|ram[0xe2]<<16;
      track_actor=src==0x198000;
    }
    if(c->k==1&&c->pc==0x888a) {
      track_actor=0;assert(word(ram,0xf9)==10052);
      memcpy(actor_output,ram+0x17f80,sizeof(actor_output));
      assert(fnv(actor_output,sizeof(actor_output))==UINT64_C(0x1b2702f8c56329ce));
    }
    if(c->k==0&&c->pc==0x89ac&&(ram[0x23a]&32))c->nmiWanted=true;
    interp816_runOpcode(c);
  }
  assert(c->sp==0x1d2&&actor_dma==0x2a00&&bg_dma==0x37c0);
  for(unsigned i=0;i<0x2a00;++i)assert(actor_writes[i]==(i<10048));
  assert(!memcmp(vram+0x8800,actor_output,10048));
}
static void refuse(void) {
  LagoonWideAtlas a={0},before=a;
  assert(!LagoonWideAtlas_RebindResident(&a,rom,sizeof(rom),fixture,resident,71));
  assert(!memcmp(&a,&before,sizeof(a)));
}
int main(int argc,char **argv) {
  assert(argc==3);FILE *f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);
  read_fixture(argv[2],"wram.bin",fixture,sizeof(fixture));read_fixture(argv[2],"vram.bin",resident,sizeof(resident));
  assert(!word(fixture,0x48e)&&!fixture[0x100e]&&!fixture[0x23a]);
  Interp816 *c=interp816_init(NULL,readb,hardware);assert(c);
  original_load(c,0xa5);memcpy(first,vram,sizeof(first));original_load(c,0x5a);
  assert(!memcmp(vram+0x8800,(uint8_t*)first+0x8800,10048));
  assert(memcmp(vram+0xaf40,(uint8_t*)first+0xaf40,704));
  assert(!memcmp(vram+0x8800,(uint8_t*)resident+0x8800,10048));
  LagoonWideAtlas a={0};assert(LagoonWideAtlas_RebindResident(&a,rom,sizeof(rom),fixture,resident,71));
  assert(a.ready&&a.completed==10048&&a.room==0&&a.source==0x198000&&a.generation==71);
  for(unsigned i=0;i<0x2a00;++i)assert(a.owned[i]==(i<10048));
  LagoonWideCell door={-180,40,0x3440,16},tail={-180,40,0x117a,8};
  assert(LagoonWideAtlas_CellOwned(&a,0x4000,0x5000,&door));
  assert(!LagoonWideAtlas_CellOwned(&a,0x4000,0x5000,&tail));
  for(unsigned i=0;i<10048;++i){((uint8_t*)resident)[0x8800+i]^=1;refuse();((uint8_t*)resident)[0x8800+i]^=1;++byte_refusals;}
  /* The older-buffer tail can vary freely but never becomes an owned tile. */
  for(unsigned i=10048;i<0x2a00;++i) {
    ((uint8_t*)resident)[0x8800+i]^=255;memset(&a,0,sizeof(a));
    assert(LagoonWideAtlas_RebindResident(&a,rom,sizeof(rom),fixture,resident,71));
    assert(!a.owned[i]&&!LagoonWideAtlas_CellOwned(&a,0x4000,0x5000,&tail));
    ((uint8_t*)resident)[0x8800+i]^=255;++tail_checks;
  }
  const unsigned spans[][2]={{0x05b2df,56},{0x018869,100},{0x009143,445},{0x0188cd,20},
    {0x00908a,81},{0x00884b,56},{0x0089a7,13},{0x0087b4,120},{0x0094a1,69},
    {0x0094e6,28},{0x019008,52},{0x0190bb,56},{0x019114,17},{0x138000,6778},{0x198000,8044}};
  for(unsigned s=0;s<sizeof(spans)/sizeof(*spans);++s)for(unsigned i=0;i<spans[s][1];++i) {
    unsigned at=off(spans[s][0])+i;rom[at]^=1;refuse();rom[at]^=1;++source_refusals;
  }
  for(unsigned i=0;i<56;++i){fixture[0x50+i]^=1;refuse();fixture[0x50+i]^=1;++metadata_refusals;}
  const unsigned fields[]={0x48e,0x48f,0x100e};
  for(unsigned i=0;i<3;++i){fixture[fields[i]]^=1;refuse();fixture[fields[i]]^=1;++metadata_refusals;}
  for(unsigned bad=0;bad<5;++bad) {
    memset(&a,0,sizeof(a));if(bad==0)a.ready=1;if(bad==1)a.completed=1;if(bad==2)a.transfer_left=1;
    LagoonWideAtlas before=a;assert(!LagoonWideAtlas_RebindResident(&a,rom,bad==3?sizeof(rom)-1:sizeof(rom),fixture,resident,bad==4?0:71));
    assert(!memcmp(&a,&before,sizeof(a)));++metadata_refusals;
  }
  printf("PASS Atland original8869/NMI/RTI:10048 source-written bytes,704 poisoned/unowned tail bytes; %u resident-byte/%u source-byte/%u metadata-partial-generation readonly refusals; %u tail variations remain unowned\n",byte_refusals,source_refusals,metadata_refusals,tail_checks);
  interp816_free(c);return 0;
}
