/* Generated original inline-helper prefix versus actual65816 opcodes. Stops BEFORE
 * original8106 indirectJMP; no invented callee/target or synthetic helper.
 * Original0181A7 JSR and01D409 JSL produce the physical2/3-byte frames. */
#include "cpu_state.h"
#include "cpu_trace.h"
#include "common_cpu_infra.h"
#include "snes/interp816.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(v) do{if(!(v)){fprintf(stderr,"line%d case%u %s\n",__LINE__,cases,#v);exit(2);}}while(0)
static uint8 rom[0x100000],ram[0x20000],before[0x20000],expected[0x20000],io[65536],expected_io[65536];
static unsigned cases,completed,children,interrupted,pending,boundary;
static uint64 deadline,bus_extra;
static unsigned inherit;static uint16 inherited_s;static uint8 inherited_h;
uint8_t g_memsel;int g_recomp_stack_top;uint16_t g_cpu_entry_s[16];const char*g_last_recomp_func;
void RecompStackPush(const char*n){(void)n;CHECK(++g_recomp_stack_top<16);}
void RecompStackPop(void){CHECK(g_recomp_stack_top>0);--g_recomp_stack_top;}
void RecompStackPopYield(void){RecompStackPop();}
void WatchdogCheck(void){}void cpu_dbg_funcname(const char*n){(void)n;}
int cpu_take_tailcall_return_context(uint16*s,uint8*h){if(!inherit)return 0;inherit=0;*s=inherited_s;*h=inherited_h;return 1;}
void cpu_tailcall_inherit_return_context(uint16 s,uint8 h){inherit=1;inherited_s=s;inherited_h=h;}
int interp_bridge_lle_master_deadline_reached(const CpuState*c){return deadline&&c->master_cycles>=deadline;}
int interp_bridge_lle_unwind_pending(void){return pending;}
RecompReturn interp_bridge_lle_yield_unwind(CpuState*c,uint32 pc){(void)c;pending=1;boundary=pc;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;}
void cpu_instruction_timing_commit(CpuState*c){(void)c;}
int cpu_dispatch_has_entry(CpuState*c,uint32 pc){(void)c;(void)pc;return 0;}
int interp_bridge_has_direct_paired_bounce(void){return 0;}
int interp_bridge_in_lle_scheduler(void){return 1;}
int cpu_resolve_ancestor_skip(uint16 s){(void)s;CHECK(0);return -1;}
int interp_bridge_return_targets_owner(uint16 s,uint16 p){(void)s;(void)p;CHECK(0);return 0;}
RecompReturn interp_tier_dispatch_rewritten_return(CpuState*c,uint32 p,uint32 s){(void)c;(void)p;(void)s;CHECK(0);return RECOMP_RETURN_NORMAL;}
RecompReturn interp_tier_dispatch_popped_return(CpuState*c,uint32 p,uint32 s,uint16 e){(void)c;(void)p;(void)s;(void)e;CHECK(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_dispatch_pc_from(CpuState*c,uint32 p,uint16 e,uint32 s){(void)c;(void)p;(void)e;(void)s;CHECK(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_unresolved_abandon_balanced(CpuState*c,uint32 p,uint16 e,uint8 h){(void)c;(void)p;(void)e;(void)h;CHECK(0);return RECOMP_RETURN_NORMAL;}
uint32_t cpu_region_speed(uint32_t a){(void)a;return 8;}
static uint8 peek(uint32 a){int32_t o=cpu_wram_offset(a>>16,a);if(o>=0)return ram[o];
 if((a>>16)<64&&(a&65535)>=0x2100&&(a&65535)<0x4400)return io[a&65535];
 if((a&65535)>=32768)return rom[((a>>16)&31)*32768+(a&32767)];return 0;}
static void write(uint32 a,uint8 v){int32_t o=cpu_wram_offset(a>>16,a);if(o>=0){ram[o]=v;return;}
 if((a>>16)<64&&(a&65535)>=0x2100&&(a&65535)<0x4400)io[a&65535]=v;}
uint8 cpu_read8(CpuState*c,uint8 b,uint16 a){return c->open_bus=peek(((uint32)b<<16)|a);}
void cpu_write8(CpuState*c,uint8 b,uint16 a,uint8 v){c->open_bus=v;write(((uint32)b<<16)|a,v);}
uint16 cpu_read16(CpuState*c,uint8 b,uint16 a){unsigned l=cpu_read8(c,b,a);return l|cpu_read8(c,b,a+1)<<8;}
void cpu_write16(CpuState*c,uint8 b,uint16 a,uint16 v){cpu_write8(c,b,a,v);cpu_write8(c,b,a+1,v>>8);}
RecompReturn interp_tier_dispatch_tail(CpuState*c,uint32 pc,uint32 site,uint16 s,uint8 h){
 (void)site;CHECK(pc==0x008106&&h==3&&c->S==(uint16)(s+3));
 boundary=pc;pending=1;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;}
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 pc,uint32 site,uint8 n,uint32*out){(void)c;(void)pc;(void)site;(void)n;(void)out;CHECK(0);return RECOMP_RETURN_NORMAL;}
/* This tool isolates the original prefix; the separately qualified title
 * dispatch guard deliberately declines so all original instructions execute. */
bool LagoonInlineTable_Run(CpuState*c,uint16_t e,uint8_t h,uint32_t r,RecompReturn*out){(void)c;(void)e;(void)h;(void)r;(void)out;return false;}
#include "inline_helper_under_test.h"
static uint8 rd(void*ctx,uint32 a){(void)ctx;bus_extra+=2;return peek(a);}
static void wr(void*ctx,uint32 a,uint8 v){(void)ctx;bus_extra+=2;write(a,v);}
static Interp816* make_interp(CpuState*c,uint16 pc){Interp816*i=interp816_init(NULL,rd,wr);CHECK(i);cpu_mirrors_to_p(c);i->a=c->A;i->x=c->X;i->y=c->Y;i->sp=c->S;i->dp=c->D;i->db=c->DB;i->k=c->PB;i->pc=pc;i->e=c->emulation;interp816_setFlags(i,c->P);return i;}
static void compare(unsigned selector,unsigned high,unsigned dp,unsigned flags,unsigned phase){
 for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*31+cases*17);memset(io,0,sizeof io);
 unsigned stack=(cases&1)?0x1fd0:0x180;
 CpuState initial={0};initial.A=(uint16)((high<<8)|selector);initial.X=(cases&1)?0x0840:0;initial.Y=(cases&2)?0xffff:0x1234;initial.S=stack;initial.DB=(cases&4)?1:0;initial.PB=1;initial.D=(uint16)dp;initial.P=0x24|((flags&1)?1:0)|((flags&2)?0xc0:0)|((flags&4)?8:0);initial.ram=ram;cpu_p_to_mirrors(&initial);
 /* Execute only the two real caller opcodes to construct their physical
  * frames. Registers are explicit standalone helper inputs, not a claim that
  * every arbitrary selector/DP appears through the normal item-use prefix. */
 Interp816* ancestor=make_interp(&initial,0x81a7);bus_extra=0;
 CHECK(interp816_runOpcode(ancestor)==6&&ancestor->pc==0xd3e9&&ancestor->sp==stack-2);
 CHECK(ram[stack]==0x81&&ram[stack-1]==0xa9);
 ancestor->pc=0xd409;CHECK(interp816_runOpcode(ancestor)==8&&ancestor->k==0&&ancestor->pc==0x80e9&&ancestor->sp==stack-5);
 CHECK(ram[stack-2]==1&&ram[stack-3]==0xd4&&ram[stack-4]==0x0c);
 initial.S=ancestor->sp;initial.A=ancestor->a;initial.X=ancestor->x;initial.Y=ancestor->y;initial.PB=ancestor->k;initial.P=interp816_getFlags(ancestor);initial.host_return_valid=3;cpu_p_to_mirrors(&initial);interp816_free(ancestor);
 deadline=phase==0?0:phase==1?1:phase==2?40:phase==3?100:phase==4?200:phase==5?400:800;
 pending=boundary=inherit=0;memcpy(before,ram,sizeof ram);CpuState actual=initial;
 RecompReturn result=INLINE_HELPER(&actual);CHECK(!g_recomp_stack_top);cpu_mirrors_to_p(&actual);CHECK(pending&&boundary&&result==(RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE);
 memcpy(expected,ram,sizeof ram);memcpy(expected_io,io,sizeof io);
 if(boundary==0x008106){children++;CHECK(actual.S==stack-2&&actual.host_return_valid==3);CHECK(ram[stack]==0x81&&ram[stack-1]==0xa9);}
 else interrupted++;
 memcpy(ram,before,sizeof ram);memset(io,0,sizeof io);Interp816*i=make_interp(&initial,0x80e9);unsigned ops=0;uint64 cycles=0,last=0;bus_extra=0;
 while(cycles<actual.cycles){CHECK(++ops<100);last=cycles*6+bus_extra;cycles+=interp816_runOpcode(i);}
 unsigned first=0;while(first<sizeof ram&&ram[first]==expected[first])first++;
 if(first<sizeof ram||memcmp(io,expected_io,sizeof io)||i->a!=actual.A||i->x!=actual.X||i->y!=actual.Y||i->sp!=actual.S||i->dp!=actual.D||i->db!=actual.DB||i->k!=actual.PB||interp816_getFlags(i)!=actual.P||(((unsigned)i->k<<16)|i->pc)!=boundary){fprintf(stderr,"STATE sel%02x high%02x DP%04x flags%u phase%u RAM%05x A%04x/%04x X%04x/%04x Y%04x/%04x S%04x/%04x D%04x/%04x DB%02x/%02x PB%02x/%02x P%02x/%02x PC%06x/%06x IO%d\n",selector,high,dp,flags,phase,first,i->a,actual.A,i->x,actual.X,i->y,actual.Y,i->sp,actual.S,i->dp,actual.D,i->db,actual.DB,i->k,actual.PB,interp816_getFlags(i),actual.P,((unsigned)i->k<<16)|i->pc,boundary,memcmp(io,expected_io,sizeof io));CHECK(0);}
 CHECK(cycles==actual.cycles&&cycles*6+bus_extra==actual.master_cycles&&last==actual.coprocessor_master_cycles);
 interp816_free(i);cases++;
}
int main(int argc,char**argv){CHECK(argc==2);FILE*f=fopen(argv[1],"rb");CHECK(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 const unsigned highs[]={0,1,0xff},dps[]={0,0x100,0xff};
 for(unsigned selector=0;selector<256;selector++)for(unsigned h=0;h<3;h++)for(unsigned d=0;d<3;d++)for(unsigned flags=0;flags<8;flags++)for(unsigned phase=0;phase<7;phase++)compare(selector,highs[h],dps[d],flags,phase);
 CHECK(children&&interrupted);
 printf("PASS inline helper %u actual-generated/original-opcode cases: %u original8106JMP-boundary/%u deadlines; realJSR+JSLphysicalframes, consumed3/inherited2, full128KiBRAM+IO+deadstack+registers+flags+hiddenA/DP+CPU/master/copro clocks; no synthetic target/handler/bridge or physicalhardware timing claim\n",cases,children,interrupted);return 0;
}
