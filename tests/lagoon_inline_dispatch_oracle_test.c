/* Guarded source-owned inline tables versus original helper instructions.
 * Each physical ancestor JSR and helper JSL executes as an original opcode. */
#include "cpu_state.h"
#include "cpu_trace.h"
#include "common_cpu_infra.h"
#include "snes/interp816.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#define CHECK(v) do{if(!(v)){fprintf(stderr,"line%d case%u %s\n",__LINE__,cases,#v);exit(2);}}while(0)
static uint8 rom[0x100000],ram[0x20000],before[0x20000],expected[0x20000],io[65536],expected_io[65536];
static unsigned cases,children,interrupted,pending,boundary,refused;
static int test_available=1,test_direct=1; static uint8 test_args;
static unsigned paired_mode,paired_calls;
static unsigned completed_selector[40],mp_changes,projectiles_born,poll_stops;
#ifdef NATIVE_MAGIC_CLOSURE
static RecompReturn native_magic(CpuState*,uint32);
#endif
#ifdef NATIVE_SCRIPT_CLOSURE
static RecompReturn native_script(CpuState*,uint32);
#endif
const uint8_t *g_rom;
CpuState g_cpu; /* Standalone context intentionally has no current hostCPU/frame lease. */
void LagoonNativeEndgameConsumeMusic(unsigned w){CHECK(w<7);}
void LagoonNativeEndgameRefuseMusic(unsigned w){CHECK(w<7);}
uint64_t RtlStateGeneration(void){return 1;}
/* Standalone source oracle has no live mapped APU or hostCPU authority. */
bool rtl_apu_absolute_periodic_enabled(void){return false;}
uint64_t g_apu_last_sync_master;
void RtlApuLock(void){CHECK(0);}
void RtlApuUnlock(void){CHECK(0);}
void rtl_sync_apu_to_cpu_locked(void){CHECK(0);}
void LagoonNativeEndgameConsumeInline(unsigned family){CHECK(family<6);}
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
int cpu_dispatch_has_entry(CpuState*c,uint32 pc){(void)c;(void)pc;return test_available;}
uint8 cpu_dispatch_inline_arg_bytes(uint32 p){(void)p;return test_args;}
RecompReturn cpu_dispatch_pc_paired(CpuState*c,uint32 p,uint8 h){CHECK(h==2&&c->PB==(p>>16));paired_calls++;
#ifdef NATIVE_MAGIC_CLOSURE
if(paired_mode==3){c->host_return_valid=2;return native_magic(c,p);}
#endif
#ifdef NATIVE_SCRIPT_CLOSURE
if(paired_mode==3){c->host_return_valid=2;return native_script(c,p);}
#endif
if(paired_mode==1){c->S+=2;return RECOMP_RETURN_NORMAL;}if(paired_mode==2)return RECOMP_RETURN_SKIP_1;if(paired_mode==4)return RECOMP_RETURN_NORMAL;if(paired_mode==5){c->S+=2;c->PB=2;return RECOMP_RETURN_NORMAL;}pending=1;boundary=p;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;}
int interp_bridge_has_direct_paired_bounce(void){return test_direct;}
int interp_bridge_in_lle_scheduler(void){return 1;}
int cpu_resolve_ancestor_skip(uint16 s){(void)s;CHECK(0);return -1;}
int interp_bridge_return_targets_owner(uint16 s,uint16 p){(void)s;(void)p;CHECK(0);return 0;}
RecompReturn interp_tier_dispatch_rewritten_return(CpuState*c,uint32 p,uint32 s){CHECK(s==0x8106&&c->PB==(p>>16)); boundary=p;pending=1;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;}
RecompReturn interp_tier_dispatch_popped_return(CpuState*c,uint32 p,uint32 s,uint16 e){(void)c;(void)p;(void)s;(void)e;CHECK(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_dispatch_pc_from(CpuState*c,uint32 p,uint16 e,uint32 s){(void)c;(void)p;(void)e;(void)s;CHECK(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_unresolved_abandon_balanced(CpuState*c,uint32 p,uint16 e,uint8 h){(void)c;(void)p;(void)e;(void)h;CHECK(0);return RECOMP_RETURN_NORMAL;}
/* Standalone original-source mode has no QoL runtime or normal-world actor
 * ownership. These guarded host optimizations refuse and use original bodies. */
bool LagoonNativeActorCommands_Run(CpuState*c,uint16_t s,uint8_t h,uint32_t pc,RecompReturn*out){(void)c;(void)s;(void)h;(void)pc;(void)out;return false;}
bool LagoonMenuDma_TryNative(CpuState*c,uint16_t s,uint8_t h,uint32_t pc,RecompReturn*out){(void)c;(void)s;(void)h;(void)pc;(void)out;return false;}
bool LagoonActorCompute_Cell(CpuState*c,uint16_t s,uint8_t h,uint32_t pc,RecompReturn*out){(void)c;(void)s;(void)h;(void)pc;(void)out;return false;}
RecompReturn interp_tier_dispatch_balanced(CpuState*c,uint32 p,uint32 site,uint16 s,uint8 h,bool jump){(void)c;(void)p;(void)site;(void)s;(void)h;(void)jump;CHECK(0);return RECOMP_RETURN_NORMAL;}
#ifdef NATIVE_SCRIPT_CLOSURE
#include "script_bus_under_test.h"
#else
uint32_t cpu_region_speed(uint32_t a){(void)a;return 8;}
#endif
static uint8 peek(uint32 a){int32_t o=cpu_wram_offset(a>>16,a);if(o>=0)return ram[o];
 if((a>>16)<64&&(a&65535)>=0x2100&&(a&65535)<0x4400)return io[a&65535];
 if((a&65535)>=32768)return rom[((a>>16)&31)*32768+(a&32767)];return 0;}
static void write(uint32 a,uint8 v){int32_t o=cpu_wram_offset(a>>16,a);if(o>=0){ram[o]=v;return;}
 if((a>>16)<64&&(a&65535)>=0x2100&&(a&65535)<0x4400)io[a&65535]=v;}
uint8 cpu_read8(CpuState*c,uint8 b,uint16 a){return c->open_bus=peek(((uint32)b<<16)|a);}
void cpu_write8(CpuState*c,uint8 b,uint16 a,uint8 v){c->open_bus=v;write(((uint32)b<<16)|a,v);}
uint16 cpu_read16(CpuState*c,uint8 b,uint16 a){unsigned l=cpu_read8(c,b,a);return l|cpu_read8(c,b,a+1)<<8;}
void cpu_write16(CpuState*c,uint8 b,uint16 a,uint16 v){cpu_write8(c,b,a,v);cpu_write8(c,b,a+1,v>>8);}
static unsigned tail_stack_override;
static RecompReturn original_child(CpuState*,uint32,uint8,uint32*);
RecompReturn interp_tier_dispatch_tail(CpuState*c,uint32 pc,uint32 site,uint16 s,uint8 h){
#ifdef NATIVE_SCRIPT_CLOSURE
 (void)site;tail_stack_override=s+1;inherit=0;uint32 out=pc;return original_child(c,pc,h,&out);
#endif
 (void)site;CHECK(pc==0x008106&&h==3&&c->S==(uint16)(s+3));
 boundary=pc;pending=1;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;}
static RecompReturn original_child(CpuState*,uint32,uint8,uint32*);
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 pc,uint32 site,uint8 n,uint32*out){(void)site;return original_child(c,pc,n,out);}

#ifdef NATIVE_SCRIPT_CLOSURE
static uint64 trace_master[100001];static uint32 trace_pc[100001];
static void script_trace(CpuState*c,uint32 pc){if(c->cycles<100001){trace_master[c->cycles]=c->master_cycles;trace_pc[c->cycles]=pc;}}
#define cpu_trace_block script_trace
#endif
static jmp_buf contract_jmp;static unsigned contract_expected,contract_failures;
static _Noreturn void test_contract_abort(void){CHECK(contract_expected);contract_failures++;longjmp(contract_jmp,1);}
#define abort test_contract_abort
#include "../src/lagoon_native_inline.c"
#undef abort
static uint8 rd(void*ctx,uint32 a){(void)ctx;bus_extra+=cpu_region_speed(a)-6;return peek(a);}
static void wr(void*ctx,uint32 a,uint8 v){(void)ctx;bus_extra+=cpu_region_speed(a)-6;write(a,v);}
static Interp816* make_interp(CpuState*c,uint16 pc){Interp816*i=interp816_init(NULL,rd,wr);CHECK(i);cpu_mirrors_to_p(c);i->a=c->A;i->x=c->X;i->y=c->Y;i->sp=c->S;i->dp=c->D;i->db=c->DB;i->k=c->PB;i->pc=pc;i->e=c->emulation;interp816_setFlags(i,c->P);return i;}
static unsigned original_children; static uint64 original_ops;
static RecompReturn original_child(CpuState*c,uint32 pc,uint8 n,uint32*out){
 CHECK(n==2||n==3);unsigned stack=tail_stack_override?tail_stack_override-1:c->S;tail_stack_override=0;unsigned stop=((n==3?ram[stack+3]:pc>>16)<<16)|((ram[stack+1]|ram[stack+2]<<8)+1);
 Interp816*i=make_interp(c,pc);i->k=pc>>16;uint64 cycles=0,base=c->master_cycles,last=c->coprocessor_master_cycles;bus_extra=0;unsigned ops=0;
 while((((unsigned)i->k<<16)|i->pc)!=stop||i->sp!=(uint16)(stack+n)){
  /* Original8985 waits for NMI to clear023A. A standalone CPU fixture has
   * no NMI: retain the first completed poll and stop at its actual backedge. */
  if(pc==0x008985&&i->k==0&&i->pc==0x8993&&ops>=8){poll_stops++;pending=1;boundary=0x008993;break;}
  if(deadline&&base+cycles*6+bus_extra>=deadline){pending=1;boundary=((unsigned)i->k<<16)|i->pc;break;}
  if(++ops>200000){fprintf(stderr,"childcap start%06x current%02x%04x S%04x stop%06x\n",pc,i->k,i->pc,i->sp,stop);CHECK(0);}
  last=base+cycles*6+bus_extra;cycles+=interp816_runOpcode(i);
 }
 c->A=i->a;c->X=i->x;c->Y=i->y;c->S=i->sp;c->D=i->dp;c->DB=i->db;c->PB=i->k;c->P=interp816_getFlags(i);cpu_p_to_mirrors(c);c->cycles+=cycles;c->master_cycles=base+cycles*6+bus_extra;c->coprocessor_master_cycles=last;
 if(out)*out=((unsigned)i->k<<16)|i->pc;interp816_free(i);original_children++;original_ops+=ops;return pending?(RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE:RECOMP_RETURN_NORMAL;
}
#ifdef NATIVE_MAGIC_CLOSURE
#include "inline_magic_closure_under_test.h"
#endif
#ifdef NATIVE_SCRIPT_CLOSURE
#include "inline_script_closure_under_test.h"
#endif
static void compare(unsigned family,unsigned selector,unsigned high,unsigned flags,unsigned phase,unsigned variant){
 const InlineFamily*f=families+family;unsigned owner=f->owner[variant%4]?f->owner[variant%4]:f->owner[0];
 for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*31+cases*17);memset(io,0,sizeof io);
 #ifdef NATIVE_MAGIC_CLOSURE
 ram[0x238]=variant&1;ram[0x239]=0;memset(ram+0xd40,0,384);ram[0x522]=(cases%3)==0?0:(cases%3)==1?1:255;ram[0x537]=selector;
 ram[0x52f]=variant==0?0:variant==1?1:variant==2?4:8;ram[0x520]=255;ram[0x521]=0;
 for(unsigned k=1;k<34;k++){unsigned q=0x500+k*64;ram[q]=variant==3?0xc8:0;ram[q+0x18]=2;ram[q+0xa]=0;}
#endif
 #ifdef NATIVE_SCRIPT_CLOSURE
 memset(ram+0x500,0,0x880);ram[0xa1]=0;ram[0xa2]=0x18;ram[0xa3]=0x7e;
 ram[0x1030]=0x40;ram[0x1031]=0;ram[0x48e]=(unsigned[]){0,14,9,74}[variant];ram[0x48f]=0;ram[0x497]=0x80;ram[0x238]=0;ram[0x239]=0;
 for(unsigned k=0;k<16;k++)ram[0x1800+k]=(uint8)(variant?variant:0);
 ram[0x1801]=0;ram[0x1802]=0x18;ram[0x1803]=0x7e;ram[0x1808]=variant%3;
 /* Command15 consumes a real seven-byte authored room descriptor. Arbitrary
  * random bytes would be invalid room/resource inputs, not a native oracle. */
 if(selector==15)memcpy(ram+0x1801,rom+0xd173,7);
 if(selector==22){ram[0x1801]=5;ram[0x1802]=7;ram[0x540]=0xc0;ram[0x558]=5;ram[0x55b]=7;}
 if(selector==33){ram[0x1801]=7;ram[0x540]=0xc0;ram[0x558]=5;ram[0x55b]=7;}
 if(selector==21&&variant==3)for(unsigned k=0;k<32;k++)ram[0x540+k*64]=0xc0;
 if((selector==22||selector==33)&&variant==3)memset(ram+0x540,0,0x800);
#endif
 unsigned stack=(cases&1)?0x1fd0:0x180;
 CpuState initial={0};initial.A=(uint16)((high<<8)|selector);initial.X=(cases&1)?0x0840:0;initial.Y=(cases&2)?0xffff:0x1234;
#ifdef NATIVE_SCRIPT_CLOSURE
 initial.Y=1;initial.X=0x740;
#endif
initial.S=stack;initial.DB=initial.PB=1;initial.P=0x24|((flags&1)?1:0)|((flags&2)?0xc0:0)|((flags&4)?8:0);initial.ram=ram;cpu_p_to_mirrors(&initial);
 Interp816*ancestor=make_interp(&initial,owner-3);bus_extra=0;
 CHECK(interp816_runOpcode(ancestor)==6&&ancestor->sp==stack-2);
 ancestor->pc=f->call;CHECK(interp816_runOpcode(ancestor)==8&&ancestor->k==0&&ancestor->pc==0x80e9&&ancestor->sp==stack-5);
 initial.S=ancestor->sp;initial.A=ancestor->a;initial.X=ancestor->x;initial.Y=ancestor->y;initial.PB=ancestor->k;initial.P=interp816_getFlags(ancestor);initial.host_return_valid=3;cpu_p_to_mirrors(&initial);interp816_free(ancestor);
 deadline=phase==0?0:phase==1?1:phase==2?40:phase==3?100:phase==4?200:phase==5?400:phase==6?800:phase==7?1200:phase==8?2500:phase==9?8000:20000;
 #ifdef NATIVE_SCRIPT_CLOSURE
 if(!deadline)deadline=100000;
#endif
 pending=boundary=inherit=0;memcpy(before,ram,sizeof ram);CpuState actual=initial;RecompReturn result=RECOMP_RETURN_NORMAL;
 #ifdef NATIVE_SCRIPT_CLOSURE
 memset(trace_master,0xff,sizeof trace_master);
#endif
 int took=LagoonInlineTable_Run(&actual,actual.S,3,0x10000|((f->call+4)&65535),&result);
 int allowed=!high&&selector<f->count&&!(flags&4);
 if(family==5&&!(f->selectors&(UINT64_C(1)<<selector)))allowed=0;
 CHECK(took==allowed);
 if(!took){CHECK(!memcmp(before,ram,sizeof ram)&&!memcmp(&initial,&actual,sizeof actual));refused++;cases++;return;}
 cpu_mirrors_to_p(&actual);if(!pending||!boundary||result==RECOMP_RETURN_NORMAL)fprintf(stderr,"RETURN fam%u sel%u phase%u variant%u result%d pending%u pc%06x S%04x initial%04x PB%u A%04x P%02x cycles%llu\n",family,selector,phase,variant,result,pending,boundary,actual.S,initial.S,actual.PB,actual.A,actual.P,(unsigned long long)actual.cycles);CHECK(pending&&boundary&&result!=RECOMP_RETURN_NORMAL);
 memcpy(expected,ram,sizeof ram);memcpy(expected_io,io,sizeof io);
 #if defined(NATIVE_MAGIC_CLOSURE)||defined(NATIVE_SCRIPT_CLOSURE)
 if(boundary==(0x10000|owner)){children++;completed_selector[selector]++;CHECK(actual.S==stack);}else interrupted++;
 if(ram[0x522]!=before[0x522])mp_changes++;
 for(unsigned k=0;k<16;k++)if(!before[0xd40+k*24]&&ram[0xd40+k*24])projectiles_born++;
#else
 if(actual.PB==1){children++;CHECK(actual.S==stack-2);CHECK(ram[stack]==((owner-1)>>8)&&ram[stack-1]==((owner-1)&255));}
 else interrupted++;
#endif
 memcpy(ram,before,sizeof ram);memset(io,0,sizeof io);Interp816*i=make_interp(&initial,0x80e9);unsigned ops=0;uint64 cycles=0,last=0;bus_extra=0;
 while(cycles<actual.cycles){
#ifdef NATIVE_SCRIPT_CLOSURE
 if(cycles<100001&&trace_master[cycles]!=UINT64_MAX&&cycles*6+bus_extra!=trace_master[cycles]){fprintf(stderr,"FIRSTCLOCK sel%u pc%06x refpc%02x%04x cpu%llu actual%llu original%llu\n",selector,trace_pc[cycles],i->k,i->pc,(unsigned long long)cycles,(unsigned long long)trace_master[cycles],(unsigned long long)(cycles*6+bus_extra));CHECK(0);}
#endif
 CHECK(++ops<200000);last=cycles*6+bus_extra;cycles+=interp816_runOpcode(i);}
 unsigned first=0;while(first<sizeof ram&&ram[first]==expected[first])first++;
 if(first<sizeof ram||memcmp(io,expected_io,sizeof io)||i->a!=actual.A||i->x!=actual.X||i->y!=actual.Y||i->sp!=actual.S||i->dp!=actual.D||i->db!=actual.DB||i->k!=actual.PB||interp816_getFlags(i)!=actual.P||(((unsigned)i->k<<16)|i->pc)!=boundary){fprintf(stderr,"STATE family%u sel%02x phase%u RAM%05x A%04x/%04x Y%04x/%04x S%04x/%04x P%02x/%02x PC%06x/%06x\n",family,selector,phase,first,i->a,actual.A,i->y,actual.Y,i->sp,actual.S,interp816_getFlags(i),actual.P,((unsigned)i->k<<16)|i->pc,boundary);CHECK(0);}
 if(cycles!=actual.cycles||cycles*6+bus_extra!=actual.master_cycles||last!=actual.coprocessor_master_cycles){fprintf(stderr,"CLOCK family%u phase%u CPU%llu/%llu master%llu/%llu copro%llu/%llu\n",family,phase,(unsigned long long)cycles,(unsigned long long)actual.cycles,(unsigned long long)(cycles*6+bus_extra),(unsigned long long)actual.master_cycles,(unsigned long long)last,(unsigned long long)actual.coprocessor_master_cycles);CHECK(0);}
 interp816_free(i);cases++;
}
static void negatives(void){
 memset(ram,0,sizeof ram);unsigned s=0x180;ram[s+1]=0x0c;ram[s+2]=0xd4;ram[s+3]=1;ram[s+4]=0xa9;ram[s+5]=0x81;
 CpuState c={0};c.S=s;c.P=CPU_P_M;c.m_flag=1;c.DB=1;c.host_return_valid=3;c.ram=ram;
 for(unsigned n=0;n<21;n++){
  CpuState q=c;unsigned at=0;uint8 old=0;uint16 es=s;uint8 h=3;uint32 host=0x1d40d;RecompReturn out=RECOMP_RETURN_NORMAL;
  switch(n){case 0:q.PB=1;break;case 1:q.DB=0;break;case 2:q.D=1;break;case 3:q.m_flag=0;break;case 4:q.x_flag=1;break;case 5:q.emulation=1;break;case 6:q._flag_D=1;break;case 7:q.P|=8;break;case 8:h=2;break;case 9:q.host_return_valid=2;break;case 10:es++;break;case 11:host++;break;case 12:at=s+3;break;case 13:at=s+4;break;case 14:at=0xe9;break;case 15:at=0xd40d;break;case 16:at=0x81a7;break;case 17:at=s+1;break;case 18:test_available=0;break;case 19:test_args=1;break;case 20:test_direct=0;break;}
  if(at){uint8*data=n>=14&&n<=16?rom:ram;old=data[at];data[at]^=1;}
  CpuState b=q;memcpy(before,ram,sizeof ram);CHECK(!LagoonInlineTable_Run(&q,es,h,host,&out));CHECK(!memcmp(&q,&b,sizeof q)&&!memcmp(before,ram,sizeof ram));
  if(at){uint8*data=n>=14&&n<=16?rom:ram;data[at]=old;}test_available=test_direct=1;test_args=0;refused++;
 }
}
static void script_negatives(void){
 for(unsigned selector=30;selector<=39;selector+=9)for(unsigned n=0;n<5;n++){
  memset(ram,0,sizeof ram);unsigned s=0x180;
  ram[s+1]=0xde;ram[s+2]=0xcc;ram[s+3]=1;ram[s+4]=0xd7;ram[s+5]=0xcc;
  CpuState c={0};c.A=selector;c.S=s;c.P=CPU_P_M;c.m_flag=1;c.DB=1;c.host_return_valid=3;c.ram=ram;
  unsigned at=n==0?0xccdf:n==1?0xcd2e:n==2?0xccd5:n==3?s+4:s+5;
  uint8*data=n<3?rom:ram;uint8 old=data[at];data[at]^=1;
  CpuState b=c;memcpy(before,ram,sizeof ram);RecompReturn out=RECOMP_RETURN_NORMAL;
  CHECK(!LagoonInlineTable_Run(&c,s,3,0x1ccdf,&out));
  CHECK(!memcmp(&c,&b,sizeof c)&&!memcmp(before,ram,sizeof ram));
  data[at]=old;refused++;
 }
}
static void return_policy(void){
 for(unsigned mode=1;mode<=2;mode++){
  memset(ram,0,sizeof ram);unsigned s=0x180;ram[s+1]=0x0c;ram[s+2]=0xd4;ram[s+3]=1;ram[s+4]=0xa9;ram[s+5]=0x81;
  CpuState c={0};c.S=s;c.P=CPU_P_M;c.m_flag=1;c.DB=1;c.host_return_valid=3;c.ram=ram;RecompReturn out=RECOMP_RETURN_NORMAL;
  paired_mode=mode;paired_calls=pending=boundary=0;deadline=0;
  CHECK(LagoonInlineTable_Run(&c,s,3,0x1d40d,&out)&&paired_calls==1);
  if(mode==1)CHECK(c.S==s+5&&pending&&boundary==0x181aa&&out==(RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE);
  else CHECK(c.S==s+3&&!pending&&out==RECOMP_RETURN_SKIP_1);
 }
 for(unsigned mode=4;mode<=5;mode++){
  memset(ram,0,sizeof ram);unsigned s=0x180;ram[s+1]=0x0c;ram[s+2]=0xd4;ram[s+3]=1;ram[s+4]=0xa9;ram[s+5]=0x81;
  CpuState c={0};c.S=s;c.P=CPU_P_M;c.m_flag=1;c.DB=1;c.host_return_valid=3;c.ram=ram;RecompReturn out=RECOMP_RETURN_NORMAL;
  paired_mode=mode;paired_calls=pending=boundary=0;deadline=0;contract_expected=1;
  if(!setjmp(contract_jmp)){LagoonInlineTable_Run(&c,s,3,0x1d40d,&out);CHECK(0);}
  contract_expected=0;CHECK(paired_calls==1&&!pending&&!boundary);
 }
 CHECK(contract_failures==2);paired_mode=0;
}
int main(int argc,char**argv){CHECK(argc==2);FILE*f=fopen(argv[1],"rb");CHECK(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);g_rom=rom;
 #ifdef NATIVE_SCRIPT_CLOSURE
 paired_mode=3;
 for(unsigned family=5;family<6;family++)
#elif defined(NATIVE_MAGIC_CLOSURE)
 paired_mode=3;
 for(unsigned family=1;family<2;family++)
#else
 for(unsigned family=0;family<6;family++)
#endif
 for(unsigned selector=0;selector<=families[family].count;selector++)for(unsigned high=0;high<2;high++)for(unsigned flags=0;flags<8;flags++)for(unsigned phase=0;phase<(paired_mode==3?11:7);phase++)for(unsigned variant=0;variant<4;variant++)compare(family,selector,high,flags,phase,variant);
 #if !defined(NATIVE_MAGIC_CLOSURE)&&!defined(NATIVE_SCRIPT_CLOSURE)
 negatives();script_negatives();return_policy();
#endif
 CHECK(children&&interrupted&&refused);
#ifdef NATIVE_SCRIPT_CLOSURE
 printf("PASS native script closure %u actualhelper+generatedhandler cases: %u completeRTS/%u realdeadline/%u readonlyrefusals;40 available CCDBhandlers, originalCCD5JSR+CCDBJSL, fullRAM/IO/deadstack/registers/CPU-master-copro clocks, actualproductionregion-speed bus\n",cases,children,interrupted,refused);
 printf("Completions per40selectors");for(unsigned n=0;n<40;n++)printf(" %u",completed_selector[n]);puts("");
#elif defined(NATIVE_MAGIC_CLOSURE)
 printf("PASS native magic inline closure %u original-helper-plus-generated-handler cases: %u completeRTS/%u deadline-or-originalwait/%u refusals; all16 table selectors, fouractualparentJSRs, MP/pool/targetsearch/hiddenA/flags, actualsourceLLEchildren, fullRAM/IO/stack/registers/CPU-master-copro clocks; constant8-master bus\n",cases,children,interrupted,refused);
 printf("Actual MPdebitcases%u sourcebornprojectiles%u originalNMIpollstops%u completions_by_selector",mp_changes,projectiles_born,poll_stops);for(unsigned n=0;n<16;n++)printf(" %u",completed_selector[n]);puts("");CHECK(mp_changes&&projectiles_born&&poll_stops);
#else
 printf("PASS inline dispatch %u actual-source cases: %u completedDC/%u deadline/%u read-only refusals; six authoredtables, all129 selectors+unknown,9physicalparentJSRs, CCDB40available, consumed3/inherited2, allRAM/IO/deadstack/registers/flags/CPU-master-copro clocks; constant8-master bus; selected-handler/live paired API gates separate\n",cases,children,interrupted,refused);
#endif
 return 0;
}
