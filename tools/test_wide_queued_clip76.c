/* External ROM/Atland graphics; arranged single-actor queues. The original
 * 01:80F1/C026 draw pass runs to its actual return and creates native OAM.
 * This tests producer/clip authority, not a campaign or actor AI. */
#define LAGOON_WIDE_MULTIPART_PRODUCER_NO_MAIN
#include "test_wide_multipart_producer.c"
CpuState g_cpu;
int LagoonWide_ViewExtraY(void){return 112;}
int LagoonWide_ViewExtraX(int extra){(void)extra;return 256;}
static uint8_t initial_ram[sizeof g_ram],saved_ram[sizeof g_ram],actor_record[64];
static uint16_t initial_vram[32768],initial_pal[256];
static LagoonWideCell decoded[64];
static unsigned positive_cases,negative_cases,original_draw_steps;

static void sync_observer(const Interp816 *c){
  memset(&g_cpu,0,sizeof g_cpu);g_cpu.ram=g_ram;g_cpu.PB=c->k;g_cpu.DB=c->db;
  g_cpu.D=c->dp;g_cpu.S=c->sp;g_cpu.m_flag=c->mf;g_cpu.x_flag=c->xf;
  g_cpu.emulation=c->e;g_cpu._flag_D=c->d;g_cpu.P=c->mf?32:0;
  g_cpu.master_cycles=1000+original_draw_steps;
}
static void original_draw(void){
  Interp816 *c=interp816_init(NULL,oracle_read,oracle_write);assert(c);
  c->pc=0x80f1;c->k=c->db=1;c->dp=0;c->sp=0x1d0;c->x=c->a=c->y=0;
  c->mf=1;c->xf=0;c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;
  sync_observer(c);LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f1);assert(s_draw_producer.begun);
  unsigned steps=0;
  do{assert(++steps<100000);interp816_runOpcode(c);}while(c->k!=1||c->pc!=0x80f4);
  original_draw_steps+=steps;sync_observer(c);LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f4);
  assert(s_draw_producer.ready);interp816_free(c);
  memcpy(p.oam,g_ram+0x2000,512);memcpy(p.highOam,g_ram+0x2200,32);
  capture();assert(s_frame.producer_valid);
}
static int clip_setup(unsigned type,int y){
  memcpy(g_ram,initial_ram,sizeof g_ram);memset(&p,0,sizeof p);ppu_reset(&p);
  memcpy(p.vram,initial_vram,sizeof initial_vram);memcpy(p.cgram,initial_pal,sizeof initial_pal);
  p.inidisp=15;p.obsel=2;p.bgmode=9;p.bgXsc[0]=0x71;p.bgXsc[1]=0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
  p.hScroll[0]=p.hScroll[1]=ram_word(g_ram,0x480)&1023;
  p.vScroll[0]=p.vScroll[1]=ram_word(g_ram,0x482)&1023;
  memcpy(actor_record,initial_ram+(type==5?0x540:0x940),64);
  assert(actor_record[24]==type);memset(g_ram+0x540,0,0x800);memcpy(g_ram+0x540,actor_record,64);
  g_ram[0x540]=0xc8;put(0x542,ram_word(g_ram,0x480)+100);put(0x544,ram_word(g_ram,0x482)+y);
  put(0x54c,100);put(0x54e,y);put(0x48c,1);put(0xec0,0x40);put(0xec2,y);put(0xec4,0xff00|type);
  empty_oam();g_ram[0x2a2]=255;g_config.widescreen=1;
  LagoonWideEntities_Reset();LagoonWideEntities_Init();atlas();PpuSetExtraSpace(&p,43);
  int n=decode_queued_entity(g_ram,0x40,g_ram[0x55c],decoded,0);assert(n>0&&n<=64);
  original_draw();return n;
}
static void visible(unsigned type,int y,int expected_native){
  int n=clip_setup(type,y);unsigned eligible=0;
  for(int i=0;i<n;i++)eligible+=decoded[i].y>=-(int)decoded[i].size&&decoded[i].y<240;
  assert((eligible!=0)==expected_native);assert(g_ram[0x236]==eligible);
  memcpy(saved_ram,g_ram,sizeof saved_ram);LagoonWideEntities_BeginFrame(1);
  assert(s_diag_producer&&p.extraObjectCount&&!memcmp(saved_ram,g_ram,sizeof saved_ram));
  /* Advancing later CPU camera/actor bytes cannot change the completed pose. */
  int x=p.extraObjects[0].x,dy=p.extraObjects[0].y;
  put(0x480,ram_word(g_ram,0x480)+5);put(0x54c,95);++g_ram[0x408];
  LagoonWideEntities_BeginFrame(1);assert(s_diag_producer&&p.extraObjectCount);
  assert(p.extraObjects[0].x==x&&p.extraObjects[0].y==dy);positive_cases++;
}
static void refuse(void){LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);negative_cases++;}
#ifndef LAGOON_WIDE_QUEUED_CLIP_NO_MAIN
int main(int argc,char **argv){
  assert(argc==3);baseline_root=argv[2];FILE *f=fopen(argv[1],"rb");
  assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);
  load_fixture("wram.bin",initial_ram,sizeof initial_ram);load_fixture("vram.bin",initial_vram,sizeof initial_vram);
  load_fixture("cgram.bin",initial_pal,sizeof initial_pal);assert(!ram_word(initial_ram,0x48e));
  for(unsigned t=0;t<2;t++){
    unsigned type=t?5:1;int n=clip_setup(type,0),minimum=32767,maximum=-32768;
    for(int i=0;i<n;i++){if(decoded[i].y<minimum)minimum=decoded[i].y;if(decoded[i].y+(int)decoded[i].size>maximum)maximum=decoded[i].y+decoded[i].size;}
    int above=-1-maximum,below=240-minimum;
    /* Entire source group is omitted, then gains/loses a native sibling.
     * It remains continuously visible in the expanded vertical canvas. */
    for(int step=-4;step<=4;step++){visible(type,above+step,step>=1);visible(type,below+step,step<0);}
    clip_setup(type,above);s_frame.producer_valid=0;refuse();
    clip_setup(type,above);++generation;refuse();--generation;
    clip_setup(type,above);s_atlas.ready=0;refuse();
    clip_setup(type,above);s_atlas.source^=1;refuse();
    clip_setup(type,above);g_ram[0x53]^=1;refuse();
    clip_setup(type,above);p.oam[0]^=1;refuse();
    clip_setup(type,above);memset(s_atlas.owned,0,sizeof s_atlas.owned);refuse();
    /* An eligible cell still needs literal native OAM; an all-clipped producer
     * is not permission to admit missing on-screen cells. */
    clip_setup(type,above+4);assert(g_ram[0x236]);
    ((uint8_t*)p.oam)[2]^=1;s_frame.oam[2]^=1;refuse();
  }
  printf("PASS queued vertical clipping: %u actual original-draw producer boundary/future-CPU cases, %u missing-producer/generation/atlas/descriptor/OAM/eligible-cell refusals; %u original opcodes, no guest writes by presenter\n",positive_cases,negative_cases,original_draw_steps);
}
#endif
