/* Four ACTUAL generated continuation bodies versus original opcodes up to
 * their exact original callee/tail boundary. Child entry is captured before
 * executing any refill instruction: no fabricated HP/MP, waits or results.
 * Full refill execution and interleaved resume ownership are separate proofs. */
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
static uint16_t inherited_s;
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
int cpu_take_tailcall_return_context(uint16 *s,uint8 *h) {*s=inherited_s;*h=2;return 1;}
void cpu_tailcall_inherit_return_context(uint16 s,uint8 h){assert(h==2);inherited_s=s;}
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
 (void)c;(void)site;assert(pc==0x01b80b||pc==0x01b816);assert(s==inherited_s&&h==2);boundary=pc;return RECOMP_RETURN_NORMAL;
}
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 pc,uint32 site,uint8 n,uint32*out) {(void)c;(void)pc;(void)site;(void)n;(void)out;assert(0);return RECOMP_RETURN_NORMAL;}
/* These are explicit STOP-AT-REAL-CALLEE-ENTRY sentinels, not fake returns.
 * The caller propagates one NLR level; no original child result is invented. */
RecompReturn Lagoon_Revival_HealPlayer_M1X0(CpuState*c){(void)c;boundary=0x01d487;return (RecompReturn)1;}
RecompReturn bank_01_D4B1_M1X0(CpuState*c){(void)c;boundary=0x01d4b1;return (RecompReturn)1;}
#include "boss_continuations_under_test.h"
static uint8 rd(void *ctx,uint32 a) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);if(off>=0)return ram[off];
 assert((a>>16)==1&&(a&65535)>=32768);return rom[0x8000+(a&32767)];
}
static void wr(void *ctx,uint32 a,uint8 v) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);assert(off>=0);ram[off]=v;
}
int main(int argc,char **argv) {
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 unsigned count=0;const unsigned starts[]={0xb806,0xb80f,0xb812,0xb815};
 const unsigned stops[]={0xb80b,0xd487,0xd4b1,0xb816};
 const unsigned saved[]={0,1,255,256,65535},sp[]={0x180,0x1d0,0x1fd0};
 for(unsigned body=0;body<4;body++)for(unsigned stack=0;stack<3;stack++)
 for(unsigned reg=0;reg<3;reg++)for(unsigned queue=0;queue<8;queue++)
 for(unsigned flags=0;flags<16;flags++)for(unsigned x=0;x<5;x++) {
  for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*73+count*17);
  CpuState initial={0};initial.A=(uint16)(count*1237);initial.X=(reg+1)*0x100;
  initial.Y=count*29;initial.S=sp[stack];initial.DB=initial.PB=1;initial.D=0;
  initial.P=0x24|((flags&1)?1:0)|((flags&2)?0x40:0)|((flags&4)?0x80:0)|((flags&8)?8:0);
  initial.ram=ram;initial.host_return_valid=2;cpu_p_to_mirrors(&initial);
  inherited_s=initial.S+2;ram[initial.S+1]=saved[x];ram[initial.S+2]=saved[x]>>8;
  ram[initial.S+3]=0xcc;ram[initial.S+4]=0x80;ram[0x23a]=(uint8)(queue*37);
  memcpy(before,ram,sizeof ram);CpuState actual=initial;boundary=0;
  RecompReturn result=body==0?Lagoon_Boss_AfterRender_M1X0(&actual):
    body==1?Lagoon_Boss_RefillHP_M1X0(&actual):body==2?Lagoon_Boss_RefillMP_M1X0(&actual):
    Lagoon_Boss_AfterRefill_M1X0(&actual);
  assert(result==RECOMP_RETURN_NORMAL);assert(!g_recomp_stack_top);cpu_mirrors_to_p(&actual);
  memcpy(expected,ram,sizeof ram);memcpy(ram,before,sizeof ram);
  Interp816*i=interp816_init(NULL,rd,wr);assert(i);i->a=initial.A;i->x=initial.X;
  i->y=initial.Y;i->sp=initial.S;i->dp=initial.D;i->db=i->k=1;i->pc=starts[body];
  i->e=0;interp816_setFlags(i,initial.P);unsigned ops=0;
  uint64_t cycles=0,last_master=0;bus_extra=0;
  while(i->pc!=stops[body]){assert(++ops<=2);last_master=cycles*6+bus_extra;cycles+=interp816_runOpcode(i);}
  assert(!memcmp(ram,expected,sizeof ram));
  assert(i->a==actual.A&&i->x==actual.X&&i->y==actual.Y&&i->sp==actual.S&&i->dp==actual.D&&
    i->db==actual.DB&&i->k==actual.PB&&interp816_getFlags(i)==actual.P);
  assert(actual.cycles==cycles);assert(actual.master_cycles==cycles*6+bus_extra);
  assert(actual.coprocessor_master_cycles==last_master);assert(i->pc==(boundary&65535));
  assert(i->mf&&!i->xf);interp816_free(i);count++;
 }
 printf("PASS %u actual generated boss continuation boundary cases; full128KiB RAM/CPU/flags/saved+deadstack/CPU+master+coprocessor clocks; real child entry only, no refill results simulated\n",count);
 return 0;
}
