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
static int inherited;static uint16 inherited_s;static uint8 inherited_h;
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
int cpu_take_tailcall_return_context(uint16 *s,uint8 *h) {if(!inherited)return 0;*s=inherited_s;*h=inherited_h;inherited=0;return 1;}
void cpu_tailcall_inherit_return_context(uint16 s,uint8 h){inherited=1;inherited_s=s;inherited_h=h;}
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
 assert(bank<32&&addr>=0x8000);return c->open_bus=rom[bank*0x8000+(addr&0x7fff)];
}
void cpu_write8(CpuState *c,uint8 bank,uint16 addr,uint8 value) {
 int32_t off=cpu_wram_offset(bank,addr);assert(off>=0);ram[off]=value;c->open_bus=value;
}
uint16 cpu_read16(CpuState *c,uint8 b,uint16 a) {unsigned lo=cpu_read8(c,b,a);return lo|((unsigned)cpu_read8(c,b,a+1)<<8);}
void cpu_write16(CpuState *c,uint8 b,uint16 a,uint16 v) {cpu_write8(c,b,a,v);cpu_write8(c,b,a+1,v>>8);}
RecompReturn interp_tier_dispatch_tail(CpuState *c,uint32 pc,uint32 site,uint16 s,uint8 h) {
 (void)c;(void)site;assert((pc>>16)==1);assert(s==c->S&&h==2);boundary=pc;return RECOMP_RETURN_NORMAL;
}
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 pc,uint32 site,uint8 n,uint32*out) {(void)c;(void)pc;(void)site;(void)n;(void)out;assert(0);return RECOMP_RETURN_NORMAL;}
#include "player_death_under_test.h"
static uint8 rd(void *ctx,uint32 a) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);if(off>=0)return ram[off];
 assert((a>>16)<32&&(a&65535)>=32768);return rom[(a>>16)*0x8000+(a&32767)];
}
static void wr(void *ctx,uint32 a,uint8 v) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);assert(off>=0);ram[off]=v;
}
int main(int argc,char **argv) {
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 unsigned count=0,complete=0,unknown=0,normal=0,other=0,blocked=0;
 const unsigned timers[]={0,6,7,255},stages[]={0,1,3,4,255},high[]={0,0x100,0xff00};
 const unsigned states[]={0,1,2,3,0x80,0x81},sp[]={0x180,0x1fd0};
 for(unsigned selector=0;selector<2;selector++)for(unsigned stack=0;stack<2;stack++)
 for(unsigned t=0;t<4;t++)for(unsigned stage=0;stage<5;stage++)for(unsigned state=0;state<6;state++)
 for(unsigned flags=0;flags<8;flags++)for(unsigned hi=0;hi<3;hi++) {
  for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*73+count*17);
  CpuState initial={0};initial.A=high[hi]|(uint8_t)(count*73);initial.X=count*37;
  initial.Y=count*29;initial.S=sp[stack];initial.DB=initial.PB=1;initial.D=0;
  initial.P=0x24|((flags&1)?1:0)|((flags&2)?0x40:0)|((flags&4)?8:0);
  initial.ram=ram;initial.host_return_valid=2;cpu_p_to_mirrors(&initial);
  ram[initial.S+1]=0x34;ram[initial.S+2]=0x12;
  ram[0x500]=(uint8_t)((count&1)?0xc1:0xc0);ram[0x501]=states[state];
  ram[0x510]=timers[t];ram[0x511]=stages[stage];
  memcpy(before,ram,sizeof ram);CpuState actual=initial;boundary=0;inherited=0;
  RecompReturn result=selector?Lagoon_ActorLleBoundary_a7d6_M1X0(&actual):Lagoon_Player_DeathAnimation_M1X0(&actual);
  assert(result==RECOMP_RETURN_NORMAL&&!g_recomp_stack_top&&!inherited);cpu_mirrors_to_p(&actual);
  memcpy(expected,ram,sizeof ram);memcpy(ram,before,sizeof ram);
  Interp816*i=interp816_init(NULL,rd,wr);assert(i);i->a=initial.A;i->x=initial.X;i->y=initial.Y;
  i->sp=initial.S;i->dp=initial.D;i->db=i->k=1;i->pc=selector?0xa7d6:0xaac1;
  i->e=0;interp816_setFlags(i,initial.P);unsigned ops=0;uint64_t cycles=0,last_master=0;bus_extra=0;
  while(i->pc!=(boundary?boundary&65535:0x1235)) {
   assert(++ops<=64);last_master=cycles*6+bus_extra;cycles+=interp816_runOpcode(i);
  }
  assert(!memcmp(ram,expected,sizeof ram));
  assert(i->a==actual.A&&i->x==actual.X&&i->y==actual.Y&&i->sp==actual.S&&i->dp==actual.D&&
    i->db==actual.DB&&i->k==actual.PB&&interp816_getFlags(i)==actual.P);
  assert(actual.cycles==cycles&&actual.master_cycles==cycles*6+bus_extra&&actual.coprocessor_master_cycles==last_master);
  if(!boundary)complete++;else if(boundary==0x1a7d5)blocked++;else if(boundary==0x1a7fa)normal++;
  else if(boundary==0x1aaf6)other++;else unknown++;
  interp816_free(i);count++;
 }
 assert(complete&&unknown&&normal&&other&&blocked);
 printf("PASS %u actual generated Nasir animation/selector original-opcode cases; all4states/actualinheritedframe/stack/flags/timers/stages/A-high/complete128KiBRAM/CPU+master+coprocessor clocks; complete=%u ordinaryLLE=%u otherdeathLLE=%u blocked=%u unknownpointerLLE=%u\n",count,complete,normal,other,blocked,unknown);
 return 0;
}
