/*24 actual full credits bodies tested at original source instruction labels.
 * Explicit post-service CPU/stack fixtures exercise every emitted label; they
 * do not create runtime reentry authority or fabricate a child return. All
 * unsupported native children STOP at their real original entry. */
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
static unsigned credits_entry;
static uint8 io[65536],expected_io[65536];
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
RecompReturn cpu_unresolved_abandon_balanced(CpuState*c,uint32 site,uint16 s,uint8 h){(void)site;(void)s;(void)h;boundary=((unsigned)c->PB<<16)|ram[0x26]|ram[0x27]<<8;return RECOMP_RETURN_NORMAL;}
uint32_t cpu_region_speed(uint32_t a) {(void)a;return 8;}
uint8 cpu_read8(CpuState *c,uint8 bank,uint16 addr) {
 int32_t off=cpu_wram_offset(bank,addr);
 if(off>=0)return c->open_bus=ram[off];
 if(bank<0x40&&addr>=0x2100&&addr<0x4400)return c->open_bus=io[addr];
 if(addr>=0x8000)return c->open_bus=rom[(bank&31)*0x8000+(addr&0x7fff)];return c->open_bus=0;
}
void cpu_write8(CpuState *c,uint8 bank,uint16 addr,uint8 value) {
 int32_t off=cpu_wram_offset(bank,addr);c->open_bus=value;
 if(off>=0){ram[off]=value;return;}
 if(!(bank<0x40&&addr>=0x2100&&addr<0x4400))return;io[addr]=value;
 if(addr==0x4203){unsigned product=io[0x4202]*value;io[0x4216]=product;io[0x4217]=product>>8;}
}
uint16 cpu_read16(CpuState *c,uint8 b,uint16 a) {unsigned lo=cpu_read8(c,b,a);return lo|((unsigned)cpu_read8(c,b,a+1)<<8);}
void cpu_write16(CpuState *c,uint8 b,uint16 a,uint16 v) {cpu_write8(c,b,a,v);cpu_write8(c,b,a+1,v>>8);}
RecompReturn interp_tier_dispatch_tail(CpuState *c,uint32 pc,uint32 site,uint16 s,uint8 h) {
 (void)c;(void)site;assert((pc>>16)==2);assert(s==inherited_s&&h==2);boundary=pc;return RECOMP_RETURN_NORMAL;
}
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 pc,uint32 site,uint8 n,uint32*out) {(void)c;(void)site;(void)n;(void)out;pending=1;boundary=pc;return (RecompReturn)1;}
#include "credits_under_test.h"
static uint8 rd(void *ctx,uint32 a) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);if(off>=0)return ram[off];
 if((a>>16)<0x40&&(a&65535)>=0x2100&&(a&65535)<0x4400)return io[a&65535];
 if((a&65535)>=32768)return rom[((a>>16)&31)*0x8000+(a&32767)];return 0;
}
static void wr(void *ctx,uint32 a,uint8 v) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);
 if(off>=0){ram[off]=v;return;}
 if(!((a>>16)<0x40&&(a&65535)>=0x2100&&(a&65535)<0x4400))return;io[a&65535]=v;
 if((a&65535)==0x4203){unsigned product=io[0x4202]*v;io[0x4216]=product;io[0x4217]=product>>8;}
}
int main(int argc,char **argv) {
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 unsigned count=0,complete=0,children=0,yields=0;
 const unsigned high[]={0,0x100,0xFF00},stacks[]={0x180,0x1FD0};
 for(unsigned body=0;body<sizeof(credit_cases)/sizeof(*credit_cases);body++)
 for(unsigned stack=0;stack<2;stack++)for(unsigned hi=0;hi<3;hi++)
 for(unsigned flags=0;flags<16;flags++)for(unsigned state=0;state<(credit_cases[body].pc==0xEAC7?8u:2u);state++) {
  for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*73+count*17);
  CpuState initial={0};initial.A=high[hi]|(uint8_t)(count*73);initial.X=state?8:2;
  if(credit_cases[body].pc==0xEDAC)initial.X=state?7:1; /* secondDEX: real odd midpoint */
  initial.Y=state?28:0;initial.S=stacks[stack]-credit_cases[body].saved;initial.DB=initial.PB=2;
  initial.P=(credit_cases[body].m?CPU_P_M:0)|(credit_cases[body].x?CPU_P_X:0)|
    ((flags&1)?CPU_P_C:0)|((flags&2)?CPU_P_V:0)|((flags&4)?CPU_P_N:0)|((flags&8)?CPU_P_D:0);
  initial.ram=ram;initial.host_return_valid=2;cpu_p_to_mirrors(&initial);
  inherited_s=stacks[stack];inherited_pending=1;credits_entry=credit_cases[body].pc;
  ram[inherited_s+1]=0x34;ram[inherited_s+2]=0x12;
  ram[0x1099]=credit_cases[body].pc==0xEAC7?state:(state?7:0);ram[0x109A]=state?255:7;
  ram[0x40]=0x13;ram[0x41]=0xEA;ram[0x42]=2;
  initial.master_cycles=credit_cases[body].expired?1:0;initial.coprocessor_master_cycles=initial.master_cycles;
  memcpy(before,ram,sizeof ram);CpuState actual=initial;boundary=0;pending=0;deadline=credit_cases[body].expired?1:0;
  memset(io,0,sizeof io);
  RecompReturn result=credit_body(credit_cases[body].body,&actual);
  assert(result==RECOMP_RETURN_NORMAL || (result==1&&pending&&boundary));assert(!g_recomp_stack_top);cpu_mirrors_to_p(&actual);
  unsigned stop=boundary?boundary:0x021235;
  memcpy(expected,ram,sizeof ram);memcpy(ram,before,sizeof ram);memcpy(expected_io,io,sizeof io);memset(io,0,sizeof io);
  Interp816*i=interp816_init(NULL,rd,wr);assert(i);i->a=initial.A;i->x=initial.X;
  i->y=initial.Y;i->sp=initial.S;i->dp=0;i->db=i->k=2;i->pc=credits_entry;
  i->e=0;interp816_setFlags(i,initial.P);unsigned ops=0;uint64_t cycles=0,last_master=initial.master_cycles;bus_extra=0;
  unsigned transfer=0;
  while((((unsigned)i->k<<16)|i->pc)!=stop || (boundary&&!credit_cases[body].expired&&!transfer)) {
   if(++ops>800000){fprintf(stderr,"cap body%06x entry%04x stop%06x pc%02x%04x S%04x/%04x\n",credit_cases[body].body,credits_entry,stop,i->k,i->pc,i->sp,actual.S);assert(0);}
   unsigned opcode=rom[(i->k&31)*0x8000+(i->pc&32767)];
   transfer=opcode==0x20||opcode==0x22||opcode==0x4C||opcode==0x5C||opcode==0x6C||opcode==0xDC||opcode==0x60||opcode==0x6B||opcode==0x7C||opcode==0xFC;
   last_master=cycles*6+bus_extra;cycles+=interp816_runOpcode(i);
  }
  if(memcmp(ram,expected,sizeof ram)||memcmp(io,expected_io,sizeof io)||
    i->a!=actual.A||i->x!=actual.X||i->y!=actual.Y||i->sp!=actual.S||i->dp!=actual.D||
    i->db!=actual.DB||i->k!=actual.PB||interp816_getFlags(i)!=actual.P) {
    fprintf(stderr,"state stop%06x body%06x entry%04x state%u flags%u A%04x/%04x X%04x/%04x Y%04x/%04x S%04x/%04x P%02x/%02x PB%02x/%02x\n",stop,credit_cases[body].body,credits_entry,state,flags,i->a,actual.A,i->x,actual.X,i->y,actual.Y,i->sp,actual.S,interp816_getFlags(i),actual.P,i->k,actual.PB);assert(0);
  }
  if(actual.cycles!=cycles||actual.master_cycles!=cycles*6+bus_extra+initial.master_cycles||actual.coprocessor_master_cycles!=last_master) {
    fprintf(stderr,"clock body%06x entry%04x cpu%llu/%llu master%llu/%llu copro%llu/%llu\n",credit_cases[body].body,credits_entry,(unsigned long long)actual.cycles,(unsigned long long)cycles,(unsigned long long)actual.master_cycles,(unsigned long long)(cycles*6+bus_extra+initial.master_cycles),(unsigned long long)actual.coprocessor_master_cycles,(unsigned long long)last_master);assert(0);
  }
  if(credit_cases[body].expired){assert(stop==credit_cases[body].body&&!ops);yields++;}
  else if(!boundary)complete++;else children++;
  interp816_free(i);count++;
 }
 assert(count&&complete&&children&&yields);
 printf("PASS %u actual generatedcredits/original-opcode cases;24fullbodies at%zu sourceinstruction/rootdeadline fixtures;full128KiBRAM+IO/registers/flags/hiddenA/realframes+deadstack/CPU-master-coprocessor clocks; complete=%u actualchild-boundary=%u rootdeadline-yields=%u; explicitpostservicefixtures are not liveauthority; constantunitbus\n",count,sizeof(credit_cases)/sizeof(*credit_cases),complete,children,yields);
 return 0;
}
