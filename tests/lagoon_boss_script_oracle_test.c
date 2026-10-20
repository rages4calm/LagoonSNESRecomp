/* Three ACTUAL generated death-flash bodies versus original opcodes up to
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
static unsigned inherited_pending;
static int pending;
static unsigned boss_test_entry, poll_busy, poll_reads;
static uint64_t deadline;
static uint64_t bus_extra;
uint8_t g_memsel;
int g_recomp_stack_top;
uint16_t g_cpu_entry_s[16];
const char *g_last_recomp_func;
void RecompStackPush(const char *name) {(void)name;g_recomp_stack_top++;}
void RecompStackPop(void) {assert(g_recomp_stack_top>0);g_recomp_stack_top--;}
void RecompStackPopYield(void) {assert(g_recomp_stack_top>0);g_recomp_stack_top--;}
void WatchdogCheck(void) {}
void cpu_dbg_funcname(const char *name) {(void)name;}
int cpu_take_tailcall_return_context(uint16 *s,uint8 *h) {if(!inherited_pending)return 0;inherited_pending=0;*s=inherited_s;*h=2;return 1;}
void cpu_tailcall_inherit_return_context(uint16 s,uint8 h){assert(h==2);inherited_s=s;inherited_pending=1;}
int interp_bridge_lle_master_deadline_reached(const CpuState *c) {(void)c;return deadline && c->master_cycles>=deadline;}
int interp_bridge_lle_unwind_pending(void) {return pending;}
RecompReturn interp_bridge_lle_yield_unwind(CpuState *c,uint32 pc) {(void)c;boundary=pc;pending=1;return RECOMP_RETURN_NORMAL;}
void cpu_instruction_timing_commit(CpuState *c) {(void)c;}
int cpu_dispatch_has_entry(CpuState *c,uint32 pc) {(void)c;(void)pc;assert(0);return 0;}
int interp_bridge_has_direct_paired_bounce(void) {assert(0);return 0;}
int cpu_resolve_ancestor_skip(uint16 s) {(void)s;assert(0);return -1;}
int interp_bridge_return_targets_owner(uint16 s,uint16 post) {(void)s;(void)post;assert(0);return 0;}
RecompReturn interp_tier_dispatch_rewritten_return(CpuState *c,uint32 pc,uint32 site) {(void)c;(void)pc;(void)site;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn interp_tier_dispatch_popped_return(CpuState *c,uint32 pc,uint32 site,uint16 s) {(void)c;(void)pc;(void)site;(void)s;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_dispatch_pc_from(CpuState *c,uint32 pc,uint16 s,uint32 site) {(void)c;(void)pc;(void)site;(void)s;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_unresolved_abandon_balanced(CpuState*c,uint32 site,uint16 s,uint8 h){(void)c;(void)site;(void)s;(void)h;assert(0);return RECOMP_RETURN_NORMAL;}
uint32_t cpu_region_speed(uint32_t a) {(void)a;return 8;}
uint8 cpu_read8(CpuState *c,uint8 bank,uint16 addr) {
 int32_t off=cpu_wram_offset(bank,addr);
 if(off>=0)return c->open_bus=ram[off];
 if(addr==0x2141&&bank<0x40)return c->open_bus=(poll_reads++<poll_busy?0x80:0);
 assert(bank<32&&addr>=0x8000);return c->open_bus=rom[bank*0x8000+(addr&0x7fff)];
}
void cpu_write8(CpuState *c,uint8 bank,uint16 addr,uint8 value) {
 int32_t off=cpu_wram_offset(bank,addr);assert(off>=0);ram[off]=value;c->open_bus=value;
}
uint16 cpu_read16(CpuState *c,uint8 b,uint16 a) {unsigned lo=cpu_read8(c,b,a);return lo|((unsigned)cpu_read8(c,b,a+1)<<8);}
void cpu_write16(CpuState *c,uint8 b,uint16 a,uint16 v) {cpu_write8(c,b,a,v);cpu_write8(c,b,a+1,v>>8);}
RecompReturn interp_tier_dispatch_tail(CpuState *c,uint32 pc,uint32 site,uint16 s,uint8 h) {
 (void)c;(void)site;assert((pc>>16)==1);assert(s==inherited_s&&h==2);boundary=pc;return RECOMP_RETURN_NORMAL;
}
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 pc,uint32 site,uint8 n,uint32*out) {(void)c;(void)site;(void)n;(void)out;pending=1;boundary=pc;return (RecompReturn)1;}
#include "boss_script_under_test.h"
static uint8 rd(void *ctx,uint32 a) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);if(off>=0)return ram[off];
 if((a&65535)==0x2141&&(a>>16)<0x40)return poll_reads++<poll_busy?0x80:0;
 assert((a>>16)<32&&(a&65535)>=32768);return rom[(a>>16)*0x8000+(a&32767)];
}
static void wr(void *ctx,uint32 a,uint8 v) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);assert(off>=0);ram[off]=v;
}
int main(int argc,char **argv) {
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 unsigned count=0,complete=0,children=0,yields=0,poll_cases=0;
 const unsigned high[]={0,0x100,0xff00},stacks[]={0x180,0x1fd0};
 for(unsigned body=0;body<BOSS_TEST_COUNT;body++)for(unsigned stack=0;stack<2;stack++)
 for(unsigned hi=0;hi<3;hi++)for(unsigned flags=0;flags<16;flags++)for(unsigned state=0;state<9;state++) {
  for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*73+count*17);
  CpuState initial={0};initial.A=high[hi]|(uint8_t)(count*73);initial.X=(state+1)*0x40;
  initial.Y=count*29;initial.S=stacks[stack];initial.DB=initial.PB=1;
  initial.P=0x24|((flags&1)?1:0)|((flags&2)?0x40:0)|((flags&4)?0x80:0)|((flags&8)?8:0);
  initial.ram=ram;initial.host_return_valid=2;cpu_p_to_mirrors(&initial);
  inherited_s=initial.S;inherited_pending=1;boss_test_entry=boss_starts[body];
  if(boss_test_entry==0xb837||boss_test_entry==0xb909)inherited_s+=2;
  ram[inherited_s+1]=0x34;ram[inherited_s+2]=0x12;
  ram[0x48e]=0x40+state;ram[0xec4]=state;
  ram[0x51b+initial.X]=state;ram[0x4c2]=(uint8_t)(count*39);
  memcpy(before,ram,sizeof ram);CpuState actual=initial;boundary=0;pending=0;deadline=0;
  poll_busy=state*3;poll_reads=0;
  if(boss_test_entry==0x937c&&(flags&1))deadline=96;
  RecompReturn result=boss_body(body,&actual);
  assert(result==RECOMP_RETURN_NORMAL || (result==1&&pending&&boundary));assert(!g_recomp_stack_top);cpu_mirrors_to_p(&actual);
  unsigned actual_reads=poll_reads,stop=boundary?boundary:0x011235;
  memcpy(expected,ram,sizeof ram);memcpy(ram,before,sizeof ram);
  Interp816*i=interp816_init(NULL,rd,wr);assert(i);i->a=initial.A;i->x=initial.X;
  i->y=initial.Y;i->sp=initial.S;i->dp=0;i->db=i->k=1;i->pc=boss_test_entry;
  i->e=0;interp816_setFlags(i,initial.P);unsigned ops=0;uint64_t cycles=0,last_master=0;bus_extra=0;poll_reads=0;
  while((((unsigned)i->k<<16)|i->pc)!=stop || (boss_test_entry==0x937c&&poll_reads<actual_reads)) {
   assert(++ops<=256);last_master=cycles*6+bus_extra;cycles+=interp816_runOpcode(i);
  }
  assert(!memcmp(ram,expected,sizeof ram));
  if(!(i->a==actual.A&&i->x==actual.X&&i->y==actual.Y&&i->sp==actual.S&&i->dp==actual.D&&
    i->db==actual.DB&&i->k==actual.PB&&interp816_getFlags(i)==actual.P)) {
    fprintf(stderr,"body%04x state%u flags%u A%04x/%04x X%04x/%04x Y%04x/%04x S%04x/%04x P%02x/%02x PB%02x/%02x\n",boss_test_entry,state,flags,i->a,actual.A,i->x,actual.X,i->y,actual.Y,i->sp,actual.S,interp816_getFlags(i),actual.P,i->k,actual.PB);assert(0);
  }
  if(actual.cycles!=cycles||actual.master_cycles!=cycles*6+bus_extra||actual.coprocessor_master_cycles!=last_master) {
    fprintf(stderr,"timing%04x cpu%llu/%llu master%llu/%llu copro%llu/%llu\n",boss_test_entry,(unsigned long long)actual.cycles,(unsigned long long)cycles,(unsigned long long)actual.master_cycles,(unsigned long long)(cycles*6+bus_extra),(unsigned long long)actual.coprocessor_master_cycles,(unsigned long long)last_master);assert(0);
  }
  if(boss_test_entry==0x937c){assert(poll_reads==actual_reads);poll_cases++;if(boundary==0x01937c)yields++;}
  if(!boundary)complete++;else children++;
  interp816_free(i);count++;
 }
 assert(complete&&children&&yields&&poll_cases);
 printf("PASS %u actual generated boss-script/original-opcode cases; full128KiB RAM/CPU+master+coprocessor clocks/stack/flags/A-high/9 dialogue states/flag-stage branches; complete=%u originalchild-boundary=%u SPCpoll=%u deadline-yields=%u\n",count,complete,children,poll_cases,yields);
 return 0;
}
