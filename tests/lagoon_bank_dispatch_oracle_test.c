/* Original bank dispatch and retirement family against65816 opcodes.
 * Real JSL frames and unknown-target interpreter handoffs are retained.
 * MMIO math comes from current production source; ports are deterministic. */
#include "cpu_state.h"
#include "cpu_trace.h"
#include "common_cpu_infra.h"
#include "snes/interp816.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(v) do{if(!(v)){fprintf(stderr,"line%d case%u %s\n",__LINE__,cases,#v);exit(2);}}while(0)
static uint8 rom[0x100000],ram[0x20000],before[0x20000],expected[0x20000],io[65536],expected_io[65536];
static unsigned cases,completed,children,interrupted,mmio_cases,cleared_cases,pending,boundary;
static unsigned root_pc,unknown_case,resumed,nonlocal_cases,target_handoff,spawned;
static uint64 deadline,bus_extra;
static unsigned inherit;static uint16 inherited_s;static uint8 inherited_h;
uint8_t g_memsel;int g_recomp_stack_top;uint16_t g_cpu_entry_s[16];const char*g_last_recomp_func;
typedef struct {uint8 multiplyA;uint16 multiplyResult;} TestSnes;
static TestSnes hardware,expected_hardware;
#include "bank_dispatch_math_under_test.h"
typedef struct{uint16 adr;uint8 write,value;} Event;
static Event events[16],expected_events[16];static unsigned event_count,expected_event_count;
static void event(unsigned a,unsigned write,unsigned value){CHECK(event_count<16);events[event_count++]=(Event){(uint16)a,(uint8)write,(uint8)value};}
void RecompStackPush(const char*n){if(!strcmp(n,"bank_01_8C54_M1X0"))spawned++;CHECK(++g_recomp_stack_top<16);}
void RecompStackPop(void){CHECK(g_recomp_stack_top>0);--g_recomp_stack_top;}
void RecompStackPopYield(void){RecompStackPop();}
void WatchdogCheck(void){}void cpu_dbg_funcname(const char*n){(void)n;}
int cpu_take_tailcall_return_context(uint16*s,uint8*h){if(!inherit)return 0;inherit=0;if(s)*s=inherited_s;if(h)*h=inherited_h;return 1;}
void cpu_tailcall_inherit_return_context(uint16 s,uint8 h){inherit=1;inherited_s=s;inherited_h=h;}
int interp_bridge_lle_master_deadline_reached(const CpuState*c){return deadline&&c->master_cycles>=deadline;}
int interp_bridge_lle_unwind_pending(void){return pending;}
RecompReturn interp_bridge_lle_yield_unwind(CpuState*c,uint32 pc){(void)c;inherit=0;pending=1;boundary=pc;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;}
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
#include "bank_dispatch_bus_under_test.h"
static uint8 peek(uint32 a){int32_t o=cpu_wram_offset(a>>16,a);if(o>=0)return ram[o];
 if((a>>16)<64&&(a&65535)>=0x2100&&(a&65535)<0x4400){unsigned adr=a&65535;
  if(adr==0x4216||adr==0x4217){uint8 value=original_math_read(&hardware,adr);event(adr,0,value);return value;}return io[adr];}
 if((a&65535)>=32768)return rom[((a>>16)&31)*32768+(a&32767)];return 0;}
static void write(uint32 a,uint8 v){int32_t o=cpu_wram_offset(a>>16,a);if(o>=0){ram[o]=v;return;}
 if((a>>16)<64&&(a&65535)>=0x2100&&(a&65535)<0x4400){unsigned adr=a&65535;io[adr]=v;
  if(adr==0x4202||adr==0x4203){original_math_write(&hardware,adr,v);event(adr,1,v);}}}
uint8 cpu_read8(CpuState*c,uint8 b,uint16 a){return c->open_bus=peek(((uint32)b<<16)|a);}
void cpu_write8(CpuState*c,uint8 b,uint16 a,uint8 v){c->open_bus=v;write(((uint32)b<<16)|a,v);}
uint16 cpu_read16(CpuState*c,uint8 b,uint16 a){unsigned l=cpu_read8(c,b,a);return l|cpu_read8(c,b,a+1)<<8;}
void cpu_write16(CpuState*c,uint8 b,uint16 a,uint16 v){cpu_write8(c,b,a,v);cpu_write8(c,b,a+1,v>>8);}
static RecompReturn source_boundary(CpuState*c,unsigned pc){
 /* Existing owning interpreter installs the yielded PC/PB on resumption.
  * No target instructions or return outcomes are synthesized here. */
 target_handoff=1;c->PB=(uint8)(pc>>16);return interp_bridge_lle_yield_unwind(c,pc);
}
RecompReturn interp_tier_dispatch_tail(CpuState*c,uint32 pc,uint32 site,uint16 saved,uint8 h){
 (void)site;(void)saved;CHECK(h==3||h==0);return source_boundary(c,pc);
}
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 pc,uint32 site,uint8 n,uint32*out){(void)c;(void)pc;(void)site;(void)n;(void)out;CHECK(0);return RECOMP_RETURN_NORMAL;}
/* This oracle proves the generic original-source branch. The separately tested
 * established actor-command guard is deliberately not admitted here. */
bool LagoonNativeActorCommands_Run(CpuState*c,uint16_t e,uint8_t h,uint32_t r,RecompReturn*out){(void)c;(void)e;(void)h;(void)r;(void)out;return false;}
#include "bank_dispatch_under_test.h"

static uint8 rd(void*ctx,uint32 a){(void)ctx;bus_extra+=cpu_region_speed(a)-6;return peek(a);}
static void wr(void*ctx,uint32 a,uint8 v){(void)ctx;bus_extra+=cpu_region_speed(a)-6;write(a,v);}
static void put(unsigned a,unsigned v){ram[a]=v;ram[a+1]=v>>8;}
static Interp816*make_interp(CpuState*c,unsigned pc){Interp816*i=interp816_init(NULL,rd,wr);CHECK(i);cpu_mirrors_to_p(c);i->a=c->A;i->x=c->X;i->y=c->Y;i->sp=c->S;i->dp=c->D;i->db=c->DB;i->k=pc>>16;i->pc=pc;i->e=c->emulation;interp816_setFlags(i,c->P);return i;}
static void copy_cpu(CpuState*c,Interp816*i){c->A=i->a;c->X=i->x;c->Y=i->y;c->S=i->sp;c->D=i->dp;c->DB=i->db;c->PB=i->k;c->P=interp816_getFlags(i);cpu_p_to_mirrors(c);}
static void original_frame(CpuState*c,unsigned site){Interp816*i=make_interp(c,site);bus_extra=0;CHECK(interp816_runOpcode(i)==8);copy_cpu(c,i);interp816_free(i);c->host_return_valid=3;}
static unsigned original_resume(CpuState*c,unsigned pc,unsigned stop,unsigned stop_s){
 Interp816*i=make_interp(c,pc);uint64 cycles=0,last=c->coprocessor_master_cycles,base=c->master_cycles;bus_extra=0;unsigned ops=0;
 while((((unsigned)i->k<<16)|i->pc)!=stop||i->sp!=stop_s){CHECK(++ops<100);last=base+cycles*6+bus_extra;cycles+=interp816_runOpcode(i);}
 copy_cpu(c,i);c->cycles+=cycles;c->master_cycles=base+cycles*6+bus_extra;c->coprocessor_master_cycles=last;interp816_free(i);return stop;
}
static void compare(unsigned kind,unsigned index,unsigned high,unsigned slot,unsigned path,unsigned flags,unsigned phase,unsigned fixture){
 for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*31+cases*17);memset(io,0,sizeof io);hardware=(TestSnes){0x5a,0xa55a};
 unsigned stack=(cases&1)?0x1fd0:0x180;CpuState initial={0};initial.A=(uint16)(high|index);initial.X=slot*64;initial.Y=(uint16)(9*(slot?slot-1:0));initial.S=stack;initial.DB=1;initial.PB=1;initial.P=0x24|((flags&1)?1:0)|((flags&2)?0xc0:0)|((flags&4)?8:0);initial.ram=ram;cpu_p_to_mirrors(&initial);
 /* Nine-byte authored record inputs live in private WRAM, never outcomes. */
 ram[0x7b]=0;ram[0x7c]=0x30;ram[0x7d]=0x7e;
 for(unsigned n=0;n<33;n++){unsigned a=0x3000+n*9;ram[a]=2;ram[a+1]=13;put(a+2,224+n);put(a+4,256+n);ram[a+6]=1;ram[a+7]=(uint8)n;ram[a+8]=0;}
 unsigned stop;
 if(kind==2){
  unsigned a=0x500+slot*64;ram[a]=path==0?0xc0:0x80;ram[a+24]=path==1?1:2;
  put(0x502,0x4000);put(0x504,0x5000);put(a+0x30,path==2?0x4000:0x4200);put(a+0x32,path==3?0x5000:0x5200);put(0x98,slot*64);put(0x9a,path==4?slot*64:0x07c0);
  original_frame(&initial,0x0180cd);root_pc=0x0281de;stop=0x0180d1;
 }else{
  original_frame(&initial,kind==0?0x02824a:0x018084);root_pc=kind==0?0x0080b3:0x0080b9;stop=kind==0?0x02824e:0x018088;
 }
 if(fixture){
  CHECK(kind==0&&high==0x4000&&index==0);put(0,0x1000);
  /* Unknown guest pointer01:1000 is WRAM. Execute real REP/SEP/RTL or
   * PLA*4/RTL instructions to test return widths/nonlocal stack ownership;
   * these are CPU fixtures, not claims of authored Lagoon handler coverage. */
  if(fixture==5){memset(ram+0x1000,0x68,4);ram[0x1004]=0x6b;}
  else{ram[0x1000]=0xc2;ram[0x1001]=0x30;ram[0x1002]=0xe2;ram[0x1003]=(uint8)((fixture-1)&3?(((fixture-1)&2)?0x20:0)|(((fixture-1)&1)?0x10:0):0);ram[0x1004]=0x6b;}
 }
 deadline=phase==0?0:phase==1?1:phase==2?100:phase==3?500:phase==4?1500:phase==5?5000:20000;
 pending=boundary=inherit=event_count=target_handoff=0;memcpy(before,ram,sizeof ram);CpuState actual=initial;
 RecompReturn result=kind==0?BANK_B3(&actual):kind==1?BANK_B9(&actual):BANK_RETIRE(&actual);CHECK(!g_recomp_stack_top);
 if(fixture){
  CHECK(!phase&&pending&&boundary==0x011000);CHECK(actual.S==initial.S-4);pending=boundary=inherit=target_handoff=0;
  if(fixture==5){original_resume(&actual,0x011000,stop,stack);nonlocal_cases++;result=RECOMP_RETURN_NORMAL;}
  else{
   original_resume(&actual,0x011000,0x0080e4,initial.S-1);unsigned mode=((actual.m_flag&1)<<1)|(actual.x_flag&1);CHECK(mode==fixture-1);actual.host_return_valid=3;
   result=bank_cleanup[mode](&actual);CHECK(!g_recomp_stack_top&&!pending&&result==RECOMP_RETURN_NORMAL);resumed++;
  }
 }
 cpu_mirrors_to_p(&actual);
 if(!pending){CHECK(result==RECOMP_RETURN_NORMAL&&actual.S==stack);actual.PB=(uint8)(stop>>16);completed++;}
 else if(target_handoff)children++;else interrupted++;
 memcpy(expected,ram,sizeof ram);memcpy(expected_io,io,sizeof io);expected_hardware=hardware;expected_event_count=event_count;memcpy(expected_events,events,sizeof events);
 memcpy(ram,before,sizeof ram);memset(io,0,sizeof io);hardware=(TestSnes){0x5a,0xa55a};event_count=0;
 Interp816*i=make_interp(&initial,root_pc);uint64 cycles=0,last=0;bus_extra=0;unsigned ops=0;
 while(cycles<actual.cycles){CHECK(++ops<20000);last=cycles*6+bus_extra;cycles+=interp816_runOpcode(i);}
 unsigned end=pending?boundary:stop,first=0;while(first<sizeof ram&&ram[first]==expected[first])first++;
 if(first<sizeof ram||memcmp(io,expected_io,sizeof io)||hardware.multiplyA!=expected_hardware.multiplyA||hardware.multiplyResult!=expected_hardware.multiplyResult||event_count!=expected_event_count||memcmp(events,expected_events,event_count*sizeof(*events))||i->a!=actual.A||i->x!=actual.X||i->y!=actual.Y||i->sp!=actual.S||i->dp!=actual.D||i->db!=actual.DB||i->k!=actual.PB||interp816_getFlags(i)!=actual.P||(((unsigned)i->k<<16)|i->pc)!=end||cycles!=actual.cycles||cycles*6+bus_extra!=actual.master_cycles||last!=actual.coprocessor_master_cycles){
  fprintf(stderr,"STATE case%u kind%u idx%02x high%04x slot%u path%u flags%u phase%u fixture%u RAM%05x A%04x/%04x X%04x/%04x Y%04x/%04x S%04x/%04x D%04x/%04x DB%02x/%02x PB%02x/%02x P%02x/%02x PC%06x/%06x CPU%llu/%llu MASTER%llu/%llu COPRO%llu/%llu events%u/%u\n",cases,kind,index,high,slot,path,flags,phase,fixture,first,i->a,actual.A,i->x,actual.X,i->y,actual.Y,i->sp,actual.S,i->dp,actual.D,i->db,actual.DB,i->k,actual.PB,interp816_getFlags(i),actual.P,((unsigned)i->k<<16)|i->pc,end,(unsigned long long)cycles,(unsigned long long)actual.cycles,(unsigned long long)(cycles*6+bus_extra),(unsigned long long)actual.master_cycles,(unsigned long long)last,(unsigned long long)actual.coprocessor_master_cycles,event_count,expected_event_count);CHECK(0);
 }
 if(event_count)mmio_cases++;interp816_free(i);cases++;
}
int main(int argc,char**argv){CHECK(argc==2);FILE*f=fopen(argv[1],"rb");CHECK(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 const unsigned highs[]={0,0x100,0x4000,0xff00};
 for(unsigned kind=0;kind<2;kind++)for(unsigned index=0;index<256;index++)for(unsigned h=0;h<4;h++)for(unsigned phase=0;phase<7;phase++)compare(kind,index,highs[h],1,0,index&7,phase,0);
 for(unsigned slot=1;slot<33;slot++)for(unsigned path=0;path<6;path++)for(unsigned phase=0;phase<7;phase++)compare(2,0,0,slot,path,slot&3,phase,0);
 for(unsigned flags=0;flags<8;flags++)for(unsigned fixture=1;fixture<=5;fixture++)compare(0,0,0x4000,1,0,flags,0,fixture);
 CHECK(completed&&children&&interrupted&&resumed==32&&nonlocal_cases==8&&mmio_cases&&spawned);
 printf("PASS bank dispatch %u source cases/%u completed outerRTL/%u target handoffs/%u instruction deadlines;32 actual all-width source-cleanup fixtures/8 actual nonlocal source returns; %u generated18C54 child entries and81DE/824A/MMIO/real80E0 frames, fullRAM/IO/stack/register/flags/CPU-master-copro parity\n",cases,completed,children,interrupted,spawned);return 0;
}
