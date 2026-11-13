/* External Denegul fixture: exact common2 resident/source identity and original
 * draw producer. This is bounded ownership policy proof, not a campaign. */
#define LAGOON_WIDE_QUEUED_CLIP_NO_MAIN
#include "test_wide_queued_clip76.c"
static unsigned refusals;
static void read_common(const char *prefix,const char *suffix,void *out,size_t n) {
  char path[4096];snprintf(path,sizeof path,"%s.%s",prefix,suffix);
  FILE *f=fopen(path,"rb");assert(f&&fread(out,1,n,f)==n);fclose(f);
}
static void common_setup(void) {
  memcpy(g_ram,initial_ram,sizeof g_ram);memset(&p,0,sizeof p);ppu_reset(&p);
  memcpy(p.vram,initial_vram,sizeof initial_vram);memcpy(p.cgram,initial_pal,sizeof initial_pal);
  p.inidisp=15;p.obsel=2;p.bgmode=9;p.bgXsc[0]=0x71;p.bgXsc[1]=g_ram[0x50]?0x7a:0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
  memcpy(actor_record,g_ram+0x980,64);memset(g_ram+0x540,0,0x800);memcpy(g_ram+0x980,actor_record,64);
  g_ram[0x980]=0xc8;put(0x48c,1);put(0xec0,0x480);put(0xec2,35);put(0xec4,0xff01);
  empty_oam();g_ram[0x2a2]=255;g_config.widescreen=1;
  LagoonWideEntities_Reset();LagoonWideEntities_Init();atlas();PpuSetExtraSpace(&p,43);
  original_draw();
  LagoonWideEntities_BeginFrame(1);
  assert(s_diag_producer);
  /* BeginFrame intentionally ends the packet lease. Re-enter that same
   * production-selected lease solely for isolated ownership mutation vectors. */
  s_render_packet=s_frame.producer_ram;s_render_generation=s_frame.generation;
}
static int owns(void) {
  const uint8_t *r=s_frame.producer_ram;
  int n=decode_queued_entity(r,0x480,147,decoded,0);assert(n==11);
  return record_cells_owned(r,0x480,147,decoded,n);
}
static void no_ownership(void){assert(!owns());++refusals;}
int main(int argc,char **argv) {
  assert(argc==3);FILE *f=fopen(argv[1],"rb");assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);
  read_common(argv[2],"wram.bin",initial_ram,sizeof initial_ram);
  read_common(argv[2],"vram.bin",initial_vram,sizeof initial_vram);
  read_common(argv[2],"cgram.bin",initial_pal,sizeof initial_pal);
  common_setup();assert(owns()&&s_has_common_npc);
  memcpy(saved_ram,g_ram,sizeof saved_ram);assert(owns());assert(!memcmp(saved_ram,g_ram,sizeof saved_ram));
  LagoonWideEntities_RetainScene();assert(!s_retained_objects.valid);++refusals;
  s_has_common_npc=0;LagoonWideCell seam={-8,30,0x2bb4,8};
  assert(draw_cell(&seam,0,1,0)&&s_has_common_npc);
  LagoonWideEntities_RetainScene();assert(!s_retained_objects.valid);++refusals;
  for(unsigned b=0;b<0x6e0;++b){common_setup();((uint8_t*)p.vram)[0xb000+b]^=1;no_ownership();}
  const unsigned spans[][2]={{0x018869,100},{0x0189ab,74},{0x0094a1,69},{0x0094f4,7},{0x11c2f3,1478}};
  for(unsigned s=0;s<sizeof spans/sizeof *spans;++s)for(unsigned b=0;b<spans[s][1];++b){
    common_setup();unsigned a=((spans[s][0]>>16)&127)*32768+(spans[s][0]&32767)+b;
    own_rom[a]^=1;no_ownership();own_rom[a]^=1;
  }
  common_setup();s_frame.producer_valid=0;no_ownership();
  common_setup();++generation;no_ownership();--generation;
  common_setup();g_ram[0x53]^=1;no_ownership();
  common_setup();assert(owns());observe_vram_byte(0xb200,((uint8_t*)p.vram)[0xb200]);assert(!s_has_common_npc&&!s_frame.producer_valid);++refusals;
  common_setup();assert(!record_cells_owned(s_frame.producer_ram,0x480,148,decoded,11));++refusals;
  for(unsigned type=2;type<=5;++type){common_setup();s_frame.producer_ram[0x998]=(uint8_t)type;assert(!record_cells_owned(s_frame.producer_ram,0x480,147,decoded,11));++refusals;}
  common_setup();s_frame.producer_ram[0x99b]=2;assert(!record_cells_owned(s_frame.producer_ram,0x480,147,decoded,11));++refusals;
  common_setup();s_frame.producer_ram[0x540]=0xc0;s_frame.producer_ram[0x558]=1;s_frame.producer_ram[0x55b]=3;no_ownership();
  LagoonWideCell tail={0,0,0x2bb7,8};assert(!LagoonWideCommon2_CellsOwned(0x4000,0x5000,&tail,1));++refusals;
  common_setup();assert(owns()); /* Fresh complete producer revalidates after refusals. */
  printf("PASS common2:11 actual wizard cells, complete1760 resident bytes and1728 original source bytes mutated; %u source/resident/type/frame/tail/context/retained refusals; fresh revalidation and readonly ownership\n",refusals);
  return 0;
}
