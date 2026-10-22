/* Actual generated BB05 outer loop, actual original-opcode children and oracle.
 * This deliberately does not replace children with guessed return values. */
#include "cpu_state.h"
#include "cpu_trace.h"
#include "common_cpu_infra.h"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef assert
#define assert(v) do{if(!(v)){fprintf(stderr,"line%d %s\n",__LINE__,#v);exit(2);}}while(0)
static uint8 rom[0x100000],ram[0x20000],before[0x20000],expected[0x20000],io[65536],expected_io[65536];
static unsigned pending,boundary,inherited_pending;static uint16 inherited_s;static uint8 inherited_h;
static uint64_t deadline,bus_extra,source_ops;static unsigned child_calls,unknown_tails,case_kind,case_slot,case_pattern,case_phase,case_unknown,returned_modes;
uint8_t g_memsel;int g_recomp_stack_top;uint16_t g_cpu_entry_s[16];const char*g_last_recomp_func;
void RecompStackPush(const char*n){(void)n;g_recomp_stack_top++;}
void RecompStackPop(void){assert(g_recomp_stack_top>0);g_recomp_stack_top--;}
void RecompStackPopYield(void){assert(g_recomp_stack_top>0);g_recomp_stack_top--;}
void WatchdogCheck(void){}void cpu_dbg_funcname(const char*n){(void)n;}
int cpu_take_tailcall_return_context(uint16*s,uint8*h){if(!inherited_pending)return 0;inherited_pending=0;*s=inherited_s;*h=inherited_h;return 1;}
void cpu_tailcall_inherit_return_context(uint16 s,uint8 h){inherited_s=s;inherited_h=h;inherited_pending=1;}
int interp_bridge_lle_master_deadline_reached(const CpuState*c){return deadline&&c->master_cycles>=deadline;}
int interp_bridge_lle_unwind_pending(void){return pending;}
RecompReturn interp_bridge_lle_yield_unwind(CpuState*c,uint32 pc){(void)c;pending=1;boundary=pc;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;}
void cpu_instruction_timing_commit(CpuState*c){(void)c;}
int cpu_dispatch_has_entry(CpuState*c,uint32 pc){(void)c;(void)pc;return 0;}
int interp_bridge_has_direct_paired_bounce(void){return 0;}
int interp_bridge_in_lle_scheduler(void){return 1;}
int cpu_resolve_ancestor_skip(uint16 s){(void)s;assert(0);return -1;}
int interp_bridge_return_targets_owner(uint16 s,uint16 p){(void)s;(void)p;assert(0);return 0;}
RecompReturn interp_tier_dispatch_rewritten_return(CpuState*c,uint32 p,uint32 s){(void)c;(void)p;(void)s;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn interp_tier_dispatch_popped_return(CpuState*c,uint32 p,uint32 s,uint16 e){(void)c;(void)p;(void)s;(void)e;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_dispatch_pc_from(CpuState*c,uint32 p,uint16 e,uint32 s){(void)c;(void)p;(void)e;(void)s;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_unresolved_abandon_balanced(CpuState*c,uint32 p,uint16 e,uint8 h){(void)c;(void)p;(void)e;(void)h;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn interp_tier_dispatch_balanced(CpuState*c,uint32 p,uint32 site,uint16 s,uint8 h,bool jump){(void)c;(void)p;(void)site;(void)s;(void)h;(void)jump;assert(0);return RECOMP_RETURN_NORMAL;}
uint32_t cpu_region_speed(uint32_t a){(void)a;return 8;}
static uint8 peek(uint32 a){int32_t o=cpu_wram_offset(a>>16,a);if(o>=0)return ram[o];if((a>>16)<64&&(a&65535)>=0x2100&&(a&65535)<0x4400)return io[a&65535];if((a&65535)>=32768)return rom[((a>>16)&31)*32768+(a&32767)];return 0;}
static void write(uint32 a,uint8 v){int32_t o=cpu_wram_offset(a>>16,a);if(o>=0){ram[o]=v;return;}if((a>>16)<64&&(a&65535)>=0x2100&&(a&65535)<0x4400){io[a&65535]=v;if((a&65535)==0x4203){unsigned p=io[0x4202]*v;io[0x4216]=p;io[0x4217]=p>>8;}}}
uint8 cpu_read8(CpuState*c,uint8 b,uint16 a){return c->open_bus=peek(((uint32)b<<16)|a);}
void cpu_write8(CpuState*c,uint8 b,uint16 a,uint8 v){c->open_bus=v;write(((uint32)b<<16)|a,v);}
uint16 cpu_read16(CpuState*c,uint8 b,uint16 a){unsigned l=cpu_read8(c,b,a);return l|cpu_read8(c,b,a+1)<<8;}
void cpu_write16(CpuState*c,uint8 b,uint16 a,uint16 v){cpu_write8(c,b,a,v);cpu_write8(c,b,a+1,v>>8);}
static uint8 rd(void*ctx,uint32 a){(void)ctx;bus_extra+=2;return peek(a);}
static void wr(void*ctx,uint32 a,uint8 v){(void)ctx;bus_extra+=2;write(a,v);}
static void put(unsigned a,unsigned v){ram[a]=v;ram[a+1]=v>>8;}
static Interp816* make_interp(CpuState*c,unsigned pc){Interp816*i=interp816_init(NULL,rd,wr);assert(i);cpu_mirrors_to_p(c);i->a=c->A;i->x=c->X;i->y=c->Y;i->sp=c->S;i->dp=c->D;i->db=c->DB;i->k=pc>>16;i->pc=pc;i->e=c->emulation;interp816_setFlags(i,c->P);return i;}
static RecompReturn source_child(CpuState*c,unsigned pc,uint32*out){
 child_calls++;unsigned stack=c->S;unsigned stop=(pc&0xff0000)|((ram[stack+1]|ram[stack+2]<<8)+1);
 Interp816*i=make_interp(c,pc);uint64_t cycles=0,base=c->master_cycles,last=c->coprocessor_master_cycles;bus_extra=0;unsigned ops=0;uint32 trace[64]={0};
 while((((unsigned)i->k<<16)|i->pc)!=stop||i->sp!=((stack+2)&65535)){
  if(deadline&&base+cycles*6+bus_extra>=deadline){pending=1;boundary=((unsigned)i->k<<16)|i->pc;break;}
  if(++ops>30000){fprintf(stderr,"child cap start%06x actual%02x%04x S%04x stop%06x S%04x\n",pc,i->k,i->pc,i->sp,stop,stack+2);assert(0);}
  trace[ops&63]=((unsigned)i->k<<16)|i->pc;
  if(!peek(((unsigned)i->k<<16)|i->pc)){fprintf(stderr,"BRK child%06x pc%02x%04x S%04x startS%04x stop%06x k%u slot%u pat%u phase%u unk%u P%02x A%04x X%04x Y%04x\n",pc,i->k,i->pc,i->sp,stack,stop,case_kind,case_slot,case_pattern,case_phase,case_unknown,interp816_getFlags(i),i->a,i->x,i->y);for(unsigned z=ops>63?ops-63:1;z<=ops;z++)fprintf(stderr," %06x",trace[z&63]);fputc('\n',stderr);assert(0);}last=base+cycles*6+bus_extra;
  unsigned at=((unsigned)i->k<<16)|i->pc;cycles+=interp816_runOpcode(i);
  if((at==0x01bb5c||at==0x01be8a)&&!case_unknown){unsigned table=at==0x01bb5c?0xbb5f:0xbe8d,known=0;for(unsigned t=0;t<34;t++)known|=i->pc==(rom[table+t*2]|rom[table+t*2+1]<<8);if(!known){pending=1;boundary=((unsigned)i->k<<16)|i->pc;break;}}
 }
 source_ops+=ops;c->A=i->a;c->X=i->x;c->Y=i->y;c->S=i->sp;c->D=i->dp;c->DB=i->db;c->PB=i->k;c->P=interp816_getFlags(i);cpu_p_to_mirrors(c);c->cycles+=cycles;c->master_cycles=base+cycles*6+bus_extra;c->coprocessor_master_cycles=last;
 if(case_unknown&&!pending&&(pc==0x01be79||pc==0x0103a9))returned_modes|=1u<<((i->mf?2:0)|(i->xf?1:0));
 if(out)*out=((unsigned)i->k<<16)|i->pc;interp816_free(i);return pending?(RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE:RECOMP_RETURN_NORMAL;
}
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 pc,uint32 site,uint8 n,uint32*out){(void)site;assert(n==2);return source_child(c,pc,out);}
RecompReturn interp_tier_dispatch_tail(CpuState*c,uint32 pc,uint32 site,uint16 s,uint8 h){
 assert(s==c->S&&h==2);
 if(site==0x01bb49||site==0x01bb3a){assert(pc==0x01bb4c&&c->x_flag);unknown_tails++;pending=1;boundary=pc;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;}
 assert(site==0x01bb5c||site==0x01be8a);
 if(case_unknown)return source_child(c,pc,NULL);
 pending=1;boundary=pc;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;
}
/* This standalone source oracle has no normal-world runtime. The title's
 * enemy-only sampler guard refuses this projectile owner (and room0); exercise
 * its original generated source fallback. Its host math has a separate oracle. */
bool LagoonActorCompute_Sample(CpuState*c,uint16_t s,uint8_t h,uint32_t pc,RecompReturn*out){
 (void)c;(void)s;(void)h;(void)pc;(void)out;return false;
}
bool LagoonActorCompute_Cell(CpuState*c,uint16_t s,uint8_t h,uint32_t pc,RecompReturn*out){
 (void)c;(void)s;(void)h;(void)pc;(void)out;return false;
}
bool LagoonNativeActorCommands_Run(CpuState*c,uint16_t s,uint8_t h,uint32_t pc,RecompReturn*out){
 (void)c;(void)s;(void)h;(void)pc;(void)out;return false;
}
#ifdef WHOLE_NATIVE_CLOSURE
#include "projectile_iteration_closure_under_test.h"
#else
#include "projectile_iteration_under_test.h"
#endif
static unsigned cases,completed,interrupted,same_pass,retired,unknown_cases;
static void compare(unsigned kind,unsigned slot,unsigned pattern,unsigned phase,unsigned flags,unsigned hi,unsigned stack,unsigned unknown){
 case_kind=kind;case_slot=slot;case_pattern=pattern;case_phase=phase;case_unknown=unknown;
 memset(ram,0,sizeof ram);memset(io,0,sizeof io);put(0x88,0x4000);ram[0x8a]=0x7e;put(0x8b,0x4000);ram[0x8d]=0x7e;put(0x6c,0x2000);ram[0x6e]=0x7e;put(0x78,0x2000);ram[0x7a]=0x7e;
 put(0x480,400);put(0x482,500);put(0x502,520);put(0x504,580);put(0x520,54);ram[0x52f]=1;ram[0x1012]=3;ram[0x1013]=slot&7;put(0x1015,500);put(0x1017,600);put(0x1019,0xffff);
 for(unsigned n=0;n<16;n++){unsigned a=0xd40+n*24;ram[a]=pattern==2?kind:pattern==3?(n%33+1):0;put(a+1,500+n);put(a+3,600+n);put(a+5,(n&1)?2:-2);put(a+7,(n&2)?2:-2);put(a+22,0xffff);ram[a+16]=n&7;ram[a+18]=(slot*17+n)&255;}
 unsigned a=0xd40+slot*24;ram[a]=kind;if(pattern==4){ram[a]=2;ram[a+18]=9;}if(pattern==5){put(a+1,4096);put(a+3,4096);}if(pattern==0)memset(ram+0xd40,0,384);
 if(unknown){ram[a]=39;ram[0x60]=0x60;ram[0x3a9]=unknown==4?0x60:unknown==1?0xe2:0xc2;ram[0x3aa]=unknown==1?0x30:0x20;ram[0x3ab]=0x60;if(unknown==3){ram[0x3ab]=0xe2;ram[0x3ac]=0x10;ram[0x3ad]=0x60;}}
 CpuState initial={0};initial.A=hi|0x81;initial.X=ITERATION_PC==0x01bb25?slot*24:0x7777;initial.Y=0x123;initial.S=stack;initial.DB=initial.PB=1;initial.P=0x24|((flags&1)?1:0)|((flags&2)?0xc0:0)|((flags&4)?8:0);initial.host_return_valid=2;initial.ram=ram;cpu_p_to_mirrors(&initial);put(stack+1,0x1234);
 deadline=phase==0?0:phase==1?1:phase==2?100:phase==3?1000:phase==4?4000:20000;pending=boundary=inherited_pending=0;memcpy(before,ram,sizeof ram);CpuState actual=initial;
 RecompReturn result=ITERATION_ROOT(&actual);assert(!g_recomp_stack_top);cpu_mirrors_to_p(&actual);assert(result==RECOMP_RETURN_NORMAL||(pending&&boundary));memcpy(expected,ram,sizeof ram);memcpy(expected_io,io,sizeof io);
 if(!pending){completed++;if(pattern==4){unsigned higher=0;for(unsigned n=slot+1;n<16;n++)if(ram[0xd40+n*24]&&ram[0xd52+n*24])higher++;same_pass+=higher;}if(pattern==5)retired+=!ram[a];}else{interrupted++;if(boundary==0x01bb4c)unknown_cases++;}
 memcpy(ram,before,sizeof ram);memset(io,0,sizeof io);Interp816*i=make_interp(&initial,ITERATION_PC);unsigned stop=boundary?boundary:0x011235,ops=0;uint64_t cycles=0,last=0;bus_extra=0;
 while(cycles<actual.cycles){if(++ops>100000){fprintf(stderr,"oraclecap k%u pattern%u phase%u pc%02x%04x\n",kind,pattern,phase,i->k,i->pc);assert(0);}last=cycles*6+bus_extra;cycles+=interp816_runOpcode(i);}
 unsigned first=0;while(first<sizeof ram&&ram[first]==expected[first])first++;
 if(first<sizeof ram||memcmp(io,expected_io,sizeof io)||i->a!=actual.A||i->x!=actual.X||i->y!=actual.Y||i->sp!=actual.S||i->dp!=actual.D||i->db!=actual.DB||i->k!=actual.PB||interp816_getFlags(i)!=actual.P||(((unsigned)i->k<<16)|i->pc)!=stop){fprintf(stderr,"STATE k%u slot%u pat%u ph%u fl%u hi%04x unk%u RAM%05x A%04x/%04x X%04x/%04x Y%04x/%04x S%04x/%04x P%02x/%02x PC%06x/%06x cycles%llu\n",kind,slot,pattern,phase,flags,hi,unknown,first,i->a,actual.A,i->x,actual.X,i->y,actual.Y,i->sp,actual.S,interp816_getFlags(i),actual.P,((unsigned)i->k<<16)|i->pc,stop,(unsigned long long)cycles);assert(0);}
 if(cycles!=actual.cycles||cycles*6+bus_extra!=actual.master_cycles||last!=actual.coprocessor_master_cycles){fprintf(stderr,"CLOCK k%u slot%u pat%u ph%u unk%u CPU%llu/%llu master%llu/%llu copro%llu/%llu\n",kind,slot,pattern,phase,unknown,(unsigned long long)cycles,(unsigned long long)actual.cycles,(unsigned long long)(cycles*6+bus_extra),(unsigned long long)actual.master_cycles,(unsigned long long)last,(unsigned long long)actual.coprocessor_master_cycles);assert(0);}
 interp816_free(i);cases++;
}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 for(unsigned k=1;k<34;k++)for(unsigned slot=0;slot<16;slot++)for(unsigned phase=0;phase<6;phase++)compare(k,slot,1,phase,slot&3,(slot&1)?0xff00:0,slot&1?0x1fd0:0x180,0);
 for(unsigned p=0;p<6;p++)for(unsigned slot=0;slot<16;slot++)for(unsigned phase=0;phase<6;phase++)compare(3,slot,p,phase,0,0,0x180,0);
 for(unsigned u=1;u<5;u++)for(unsigned phase=0;phase<6;phase++)compare(39,0,1,phase,0,0,0x180,u);
 for(unsigned k=1;k<34;k++)for(unsigned fl=0;fl<8;fl++)compare(k,0,1,3,fl,0x100,0x1fd0,0);
 printf("PASS projectile iteration %u cases completed%u interrupted%u actual-original-child-calls%u source-opcodes%llu same-pass-higher-born%u retired%u unknown-width-exact-tails%u all-four-returned-modes-mask%X\n",cases,completed,interrupted,child_calls,(unsigned long long)source_ops,same_pass,retired,unknown_cases,returned_modes);assert(completed&&interrupted&&(same_pass||ITERATION_PC==0x01bb25)&&retired&&unknown_cases&&unknown_tails&&returned_modes==15);return 0;}
