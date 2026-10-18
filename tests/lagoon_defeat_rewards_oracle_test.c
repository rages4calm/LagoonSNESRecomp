/* Six actual generated reward arithmetic bodies versus original ROM opcodes.
 * No mirrored arithmetic or stubbed child results. Scheduling is separate. */
#include "cpu_state.h"
#include "cpu_trace.h"
#include "common_cpu_infra.h"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef assert
#define assert(v) do { if (!(v)) { fprintf(stderr,"line %d: %s\n",__LINE__,#v);exit(2); } } while(0)
static uint8 rom[0x100000], ram[0x20000], before[0x20000], expected[0x20000];
static unsigned boundary;
static uint64_t bus_extra;
uint8_t g_memsel;
int g_recomp_stack_top;
uint16_t g_cpu_entry_s[16];
const char *g_last_recomp_func;
void RecompStackPush(const char *name) {(void)name;g_recomp_stack_top++;}
void RecompStackPop(void) {assert(g_recomp_stack_top>0);g_recomp_stack_top--;}
void RecompStackPopYield(void) {assert(0);}
void WatchdogCheck(void) {}
void cpu_dbg_funcname(const char *name) {(void)name;}
int cpu_take_tailcall_return_context(uint16 *s,uint8 *h) {(void)s;(void)h;return 0;}
int interp_bridge_lle_master_deadline_reached(const CpuState *c) {(void)c;return 0;}
int interp_bridge_lle_unwind_pending(void) {return 0;}
RecompReturn interp_bridge_lle_yield_unwind(CpuState *c,uint32 pc) {(void)c;(void)pc;assert(0);return RECOMP_RETURN_NORMAL;}
void cpu_instruction_timing_commit(CpuState *c) {(void)c;}
int cpu_dispatch_has_entry(CpuState *c,uint32 pc) {(void)c;(void)pc;assert(0);return 0;}
int interp_bridge_has_direct_paired_bounce(void) {assert(0);return 0;}
int cpu_resolve_ancestor_skip(uint16 s) {(void)s;assert(0);return -1;}
int interp_bridge_return_targets_owner(uint16 s,uint16 post) {(void)s;(void)post;assert(0);return 0;}
RecompReturn interp_tier_dispatch_rewritten_return(CpuState *c,uint32 pc,uint32 site) {(void)c;(void)pc;(void)site;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn interp_tier_dispatch_popped_return(CpuState *c,uint32 pc,uint32 site,uint16 s) {(void)c;(void)pc;(void)site;(void)s;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_dispatch_pc_from(CpuState *c,uint32 pc,uint16 s,uint32 site) {(void)c;(void)pc;(void)site;(void)s;assert(0);return RECOMP_RETURN_NORMAL;}
uint32_t cpu_region_speed(uint32_t a) {(void)a;return 8;}
uint8 cpu_read8(CpuState *c,uint8 bank,uint16 addr) {
 int32_t off=cpu_wram_offset(bank,addr);
 if(off>=0)return c->open_bus=ram[off];
 assert(bank==1&&addr>=0x8000);return c->open_bus=rom[0x8000+(addr&0x7fff)];
}
void cpu_write8(CpuState *c,uint8 bank,uint16 addr,uint8 value) {
 int32_t off=cpu_wram_offset(bank,addr);assert(off>=0);ram[off]=value;c->open_bus=value;
}
uint16 cpu_read16(CpuState *c,uint8 b,uint16 a) {unsigned lo=cpu_read8(c,b,a);return lo|((unsigned)cpu_read8(c,b,a+1)<<8);}
void cpu_write16(CpuState *c,uint8 b,uint16 a,uint16 v) {cpu_write8(c,b,a,v);cpu_write8(c,b,a+1,v>>8);}
RecompReturn interp_tier_dispatch_tail(CpuState *c,uint32 pc,uint32 site,uint16 s,uint8 h) {
 (void)c;(void)site;assert(pc==0x01b7fe||pc==0x01b819);assert(s==c->S&&h==2);boundary=pc;return RECOMP_RETURN_NORMAL;
}
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 pc,uint32 site,uint8 n,uint32*out) {(void)c;(void)pc;(void)site;(void)n;(void)out;assert(0);return RECOMP_RETURN_NORMAL;}
#include "defeat_rewards_under_test.h"
static uint8 rd(void *ctx,uint32 a) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);if(off>=0)return ram[off];
 assert((a>>16)==1&&(a&65535)>=32768);return rom[0x8000+(a&32767)];
}
static void wr(void *ctx,uint32 a,uint8 v) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);assert(off>=0);ram[off]=v;
}
int main(int argc,char **argv) {
 assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 unsigned count=0,exits[2]={0},levelups=0;
 const unsigned levels[]={0,1,20,34,35};
 const unsigned totals[]={0,1,65534,65535};
 const unsigned rewards[]={0,1,255,256,65535};
 const unsigned types[]={0,3,4};
 for(unsigned slot=0;slot<3;slot++)for(unsigned l=0;l<5;l++)for(unsigned v=0;v<4;v++)for(unsigned r=0;r<5;r++)for(unsigned ty=0;ty<3;ty++)for(unsigned flags=0;flags<16;flags++) {
  for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*73+count*17);
  CpuState initial={0};initial.A=(uint16)(count*1237);initial.X=(slot+1)*0x100;initial.Y=count*29;initial.S=0x1d0;initial.DB=initial.PB=1;initial.D=0;initial.P=0x24|((flags&1)?1:0)|((flags&2)?0x40:0)|((flags&4)?0x80:0)|((flags&8)?8:0);initial.ram=ram;initial.host_return_valid=2;cpu_p_to_mirrors(&initial);
  ram[initial.S+1]=0x34;ram[initial.S+2]=0x12;
  ram[0x518+initial.X]=types[ty];ram[0x51b+initial.X]=(count&1)?10:36;
  ram[0x528+initial.X]=rewards[r];ram[0x529+initial.X]=rewards[r]>>8;
  ram[0x52a+initial.X]=rewards[r];ram[0x52b+initial.X]=rewards[r]>>8;
  ram[0x528]=ram[0x52a]=totals[v];ram[0x529]=ram[0x52b]=totals[v]>>8;
  ram[0x52c]=levels[l];ram[0x530]=ram[0x531]=ram[0x532]=0;ram[0x533]=count%3;
  memcpy(before,ram,sizeof ram);CpuState actual=initial;boundary=0;
  assert(Lagoon_Combat_GrantDefeatRewards_M1X0(&actual)==RECOMP_RETURN_NORMAL);assert(!g_recomp_stack_top);cpu_mirrors_to_p(&actual);
  memcpy(expected,ram,sizeof ram);memcpy(ram,before,sizeof ram);
  Interp816 *i=interp816_init(NULL,rd,wr);assert(i);i->a=initial.A;i->x=initial.X;i->y=initial.Y;i->sp=initial.S;i->dp=initial.D;i->db=i->k=1;i->pc=0xb7d4;i->e=0;interp816_setFlags(i,initial.P);
  unsigned ops=0;uint64_t cycles=0,last_master=0;bus_extra=0;
  while(i->pc!=0xb7fe&&i->pc!=0xb819) {assert(++ops<=8192);last_master=cycles*6+bus_extra;cycles+=interp816_runOpcode(i);}
  assert(!memcmp(ram,expected,sizeof ram));
  assert(i->a==actual.A&&i->x==actual.X&&i->y==actual.Y&&i->sp==actual.S&&i->dp==actual.D&&i->db==actual.DB&&i->k==actual.PB&&interp816_getFlags(i)==actual.P);
  assert(actual.cycles==cycles);assert(actual.master_cycles==cycles*6+bus_extra);assert(actual.coprocessor_master_cycles==last_master);
  assert(i->pc==(boundary&65535));assert(i->sp==initial.S);assert(i->mf&&!i->xf);exits[boundary==0x01b7fe?0:1]++;
  levelups+=ram[0x52c]!=levels[l];interp816_free(i);count++;
 }
 assert(exits[0]&&exits[1]&&levelups);printf("PASS %u exact generated prefix and five child bodies/original-opcode full128KiB RAM, CPU, flags, paired/deadstack, CPU/master clocks; bossboundary=%u ordinaryreturn=%u levelupcases=%u\n",count,exits[0],exits[1],levelups);return 0;
}
