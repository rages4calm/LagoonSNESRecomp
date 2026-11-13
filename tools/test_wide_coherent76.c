/* Completed producer/OAM proof is independent of later mutable CPU RAM.
 * Actor layout is synthetic over an external real Atland source/atlas fixture. */
#define LAGOON_WIDE_ACTOR_POLICY_NO_MAIN
#include "test_wide_additional_producer.c"
static int zoom=112;
int LagoonWide_ViewExtraY(void){return zoom;}
int LagoonWide_ViewExtraX(int extra){(void)extra;return 256;}
static CpuState producer_cpu;
static unsigned positives,refused;
static uint8_t actor[64],baseline[0x20000];
static void read_input(const char *prefix,const char *suffix,void *dst,size_t n){char path[4096];snprintf(path,sizeof path,"%s.%s",prefix,suffix);FILE*f=fopen(path,"rb");assert(f&&fread(dst,1,n,f)==n);fclose(f);}
static void seal(void){memset(&producer_cpu,0,sizeof producer_cpu);producer_cpu.ram=g_ram;producer_cpu.PB=producer_cpu.DB=1;producer_cpu.m_flag=1;producer_cpu.P=0x20;producer_cpu.S=0x1d0;producer_cpu.master_cycles=100;LagoonWideEntities_ObserveDraw(&producer_cpu,0x0180f1);assert(s_draw_producer.begun);g_ram[0x1cf]=0xf3;g_ram[0x1d0]=0x80;producer_cpu.master_cycles+=42;LagoonWideEntities_ObserveDraw(&producer_cpu,0x0180f4);assert(s_draw_producer.ready);capture();assert(s_frame.producer_valid);}
static int minx(void){assert(p.extraObjectCount);int x=32767;for(unsigned i=0;i<p.extraObjectCount;i++)if(p.extraObjects[i].x<x)x=p.extraObjects[i].x;return x;}
static void drift(void){put(0x480,ram_word(g_ram,0x480)+5);put(0x502,ram_word(g_ram,0x502)+5);put(0x542,ram_word(g_ram,0x542)+3);put(0x54c,ram_word(g_ram,0x542)-ram_word(g_ram,0x480));++g_ram[0x550];++g_ram[0x408];}
static void refusal(void){LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);++refused;}
int main(int argc,char**argv){assert(argc==3);FILE*f=fopen(argv[1],"rb");assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);read_input(argv[2],"wram.bin",g_ram,sizeof g_ram);read_input(argv[2],"vram.bin",p.vram,sizeof p.vram);read_input(argv[2],"cgram.bin",p.cgram,sizeof p.cgram);assert(!ram_word(g_ram,0x48e));memcpy(actor,g_ram+0x940,64);assert(actor[24]==1&&actor[27]==44);memset(g_ram+0x540,0,0x800);memcpy(g_ram+0x540,actor,64);actor_type=1;p.obsel=2;p.inidisp=15;p.bgmode=9;p.bgXsc[0]=0x71;p.bgXsc[1]=0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;g_config.widescreen=1;LagoonWideEntities_Reset();LagoonWideEntities_Init();atlas();PpuSetExtraSpace(&p,43);oam_for_record(-140,80,0);seal();memcpy(baseline,g_ram,sizeof baseline);LagoonWideEntities_BeginFrame(1);int initial=minx();++positives;
 drift();LagoonWideEntities_BeginFrame(1);assert(s_diag_producer&&minx()==initial);++positives;
 /* The next actual producer advances, rather than retaining the old pose. */
 memcpy(g_ram,baseline,sizeof baseline);oam_for_record(-137,80,0);++g_ram[0x408];seal();memcpy(baseline,g_ram,sizeof baseline);drift();LagoonWideEntities_BeginFrame(1);assert(s_diag_producer&&minx()==initial+3);++positives;
 g_ram[0x53]^=1;refusal();g_ram[0x53]^=1;
 p.oam[0]^=1;refusal();p.oam[0]^=1;
 ++generation;refusal();--generation;
 s_atlas.source^=1;refusal();s_atlas.source^=1;
 own_rom[0xc1c6]^=1;refusal();own_rom[0xc1c6]^=1;
 /* A capture without the original producer proof cannot borrow itself. */
 s_frame.producer_valid=0;refusal();
 printf("PASS %u completed-packet/future-RAM/new-field pose cases, %u resource/OAM/generation/atlas/source/unwitnessed-producer refusals\n",positives,refused);return 0;}
