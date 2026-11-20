/* Actual original C026 -> C67C composite source oracle over an external Giles
 * room fixture. Arranged clocks/scrolls prove source math, not a campaign. */
#define LAGOON_WIDE_QUEUED_CLIP_NO_MAIN
#include "test_wide_queued_clip76.c"
static unsigned cloud_cases,cloud_refusals,cloud_steps,cloud_mutations;
static uint8_t post_ram[sizeof g_ram];
static __typeof__(s_draw_producer) pre_append;
static CpuState post_cpu;
static void setup_gold(unsigned wind,unsigned bx,unsigned by){
  memcpy(g_ram,initial_ram,sizeof g_ram);memset(&p,0,sizeof p);ppu_reset(&p);
  memcpy(p.vram,initial_vram,sizeof p.vram);memcpy(p.cgram,initial_pal,sizeof p.cgram);
  p.inidisp=15;p.obsel=2;p.bgmode=9;p.bgXsc[0]=0x71;p.bgXsc[1]=0x7a;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
  p.hScroll[0]=ram_word(g_ram,0x480)&1023;p.vScroll[0]=ram_word(g_ram,0x482)&1023;
  p.hScroll[1]=bx;p.vScroll[1]=by;put(0x204,bx);put(0x206,by);put(0x40d,wind);
  empty_oam();g_ram[0x2a2]=255;g_config.widescreen=1;
  LagoonWideEntities_Reset();LagoonWideEntities_Init();atlas();PpuSetExtraSpace(&p,213);
}
static void composite(void){
  Interp816*c=interp816_init(NULL,oracle_read,oracle_write);assert(c);
  c->pc=0x80f1;c->k=c->db=1;c->dp=0;c->sp=0x1d0;c->x=c->a=c->y=0;
  c->mf=1;c->xf=c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;
  sync_observer(c);LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f1);
  unsigned steps=0;do{assert(++steps<100000);interp816_runOpcode(c);}while(c->k!=1||c->pc!=0x80f4);
  original_draw_steps+=steps;sync_observer(c);LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f4);
  assert(s_draw_producer.ready&&s_draw_producer.gold_pending);pre_append=s_draw_producer;
  steps=0;do{assert(++steps<100000);interp816_runOpcode(c);}while(c->k!=1||c->pc!=0x80fa);
  cloud_steps+=steps;original_draw_steps+=steps;sync_observer(c);post_cpu=g_cpu;memcpy(post_ram,g_ram,sizeof g_ram);
  LagoonWideEntities_ObserveDraw(&g_cpu,0x0180fa);assert(!memcmp(post_ram,g_ram,sizeof g_ram));
  assert(s_draw_producer.ready&&s_draw_producer.gold_valid&&!s_draw_producer.gold_pending);
  LagoonGoldCloud77 cloud;assert(LagoonGold77_Decode(&cloud,g_ram,g_rom,kRomSize,pre_append.ram[0x236]));
  assert(LagoonGold77_Append(&cloud,pre_append.oam,g_ram+0x2000)&&LagoonGold77_Match(&cloud,g_ram+0x2000));
  assert(cloud.count==68&&cloud.end==g_ram[0x236]);
  memcpy(p.oam,g_ram+0x2000,512);memcpy(p.highOam,g_ram+0x2200,32);capture();
  assert(s_frame.producer_valid&&s_frame.gold_valid);
  uint64_t emitted=s_gold_cells;LagoonWideEntities_BeginFrame(1);
  assert(s_diag_producer&&s_gold_cells>emitted&&p.extraObjectCount);
  assert(!memcmp(post_ram,g_ram,sizeof g_ram));++cloud_cases;interp816_free(c);
}
static void reject_append(void){
  s_draw_producer=pre_append;g_cpu=post_cpu;
  LagoonWideEntities_ObserveDraw(&g_cpu,0x0180fa);
  assert(!s_draw_producer.gold_valid&&!s_draw_producer.ready);++cloud_refusals;
}
int main(int argc,char**argv){
  assert(argc==3);baseline_root=argv[2];FILE*f=fopen(argv[1],"rb");assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);
  load_fixture("wram.bin",initial_ram,sizeof initial_ram);load_fixture("vram.bin",initial_vram,sizeof initial_vram);load_fixture("cgram.bin",initial_pal,sizeof initial_pal);
  assert(ram_word(initial_ram,0x48e)==7);
  const unsigned winds[]={0,1,254,255,256,510,511,512,1023,65535};
  const unsigned ys[]={0,15,58,239,255};
  for(unsigned w=0;w<sizeof winds/sizeof *winds;w++)for(unsigned y=0;y<sizeof ys/sizeof *ys;y++){
    setup_gold(winds[w],51,ys[y]);composite();
  }
  const unsigned xs[]={0,255,511,512,1023};
  for(unsigned x=0;x<sizeof xs/sizeof *xs;x++)for(unsigned w=510;w<=512;w++){
    setup_gold(w,xs[x],58);composite();
  }
  setup_gold(965,51,58);composite();
  assert(pre_append.ram[0x236]%4); /* Real cloud append shares a high-OAM byte. */
  /* Every actual appended/prefix/tail OAM byte is independently guarded. */
  for(unsigned a=0;a<544;a++){memcpy(g_ram,post_ram,sizeof g_ram);g_ram[0x2000+a]^=1;reject_append();}
  memcpy(g_ram,post_ram,sizeof g_ram);
  const unsigned fields[]={0x204,0x205,0x206,0x207,0x236,0x408,0x40d,0x40e,0x480,0x482,0x48e,0x4e1,0x500,0x8000,0xec0,0x1ce,0x1cf,0x1d0,0x1d1};
  for(unsigned n=0;n<sizeof fields/sizeof *fields;n++){memcpy(g_ram,post_ram,sizeof g_ram);g_ram[fields[n]]^=1;reject_append();}
  memcpy(g_ram,post_ram,sizeof g_ram);
  for(unsigned i=0;i<133;i++){own_rom[0x1467c+i]^=1;reject_append();own_rom[0x1467c+i]^=1;++cloud_mutations;}
  const unsigned callers[][2]={{0x80f4,6},{0x10016,2},{0xb9,48}};
  for(unsigned n=0;n<3;n++)for(unsigned i=0;i<callers[n][1];i++){own_rom[callers[n][0]+i]^=1;reject_append();own_rom[callers[n][0]+i]^=1;++cloud_mutations;}
  const unsigned shared_sources[]={0x22a8,0x199d+8*3,0x0b71};
  for(unsigned n=0;n<sizeof shared_sources/sizeof *shared_sources;n++){
    unsigned a=shared_sources[n];own_rom[a]^=1;reject_append();own_rom[a]^=1;++cloud_mutations;
  }
  for(unsigned mode=0;mode<10;mode++){
    s_draw_producer=pre_append;g_cpu=post_cpu;
    switch(mode){case 0:g_cpu.PB=2;break;case 1:g_cpu.DB=2;break;case 2:g_cpu.S--;break;
      case 3:g_cpu.m_flag=0;break;case 4:g_cpu.x_flag=1;break;case 5:g_cpu.emulation=1;break;
      case 6:g_cpu.D=1;break;case 7:g_cpu._flag_D=1;break;case 8:g_cpu.P|=8;break;
      case 9:g_cpu.master_cycles=pre_append.master-1;break;}
    LagoonWideEntities_ObserveDraw(&g_cpu,0x0180fa);assert(!s_draw_producer.ready);++cloud_refusals;
  }
  s_draw_producer=pre_append;g_cpu=post_cpu;CpuState impostor=g_cpu;
  LagoonWideEntities_ObserveDraw(&impostor,0x0180fa);assert(!s_draw_producer.ready);++cloud_refusals;
  s_draw_producer=pre_append;g_cpu=post_cpu;++generation;LagoonWideEntities_ObserveDraw(&g_cpu,0x0180fa);assert(!s_draw_producer.ready);--generation;++cloud_refusals;
  s_draw_producer=pre_append;s_draw_producer.gold_pending=0;s_draw_producer.gold_valid=0;g_cpu=post_cpu;LagoonWideEntities_ObserveDraw(&g_cpu,0x0180fa);assert(!s_draw_producer.gold_valid);++cloud_refusals;
  setup_gold(965,51,58);composite();uint64_t previous=s_gold_cells;
  p.oam[0]^=1;LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount&&s_gold_cells==previous);p.oam[0]^=1;++cloud_refusals;
  ++generation;LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount&&s_gold_cells==previous);--generation;++cloud_refusals;
  memset(s_atlas.owned,0,sizeof s_atlas.owned);LagoonWideEntities_BeginFrame(1);assert(s_gold_cells==previous);++cloud_refusals;
  printf("PASS Goldcloud77: %u original composite producers over wind/Y wrap cases; %u original cloud opcodes; %u fullOAM/clock/scroll/stack/generation/bypass refusals and%u source mutations; no presenter writes.\n",cloud_cases,cloud_steps,cloud_refusals,cloud_mutations);
}
