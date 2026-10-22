/* CompiledB9 preserves the established zero-clock prefix and real80E5RTL.
 * Reuse only the old fixture builders; this does not rerun the old full suite. */
#define main unused_actor_source_main
#include "lagoon_native_actor_commands_test.c"
#undef main
#include "cpu_trace.h"
uint8_t g_memsel;int g_recomp_stack_top;uint16_t g_cpu_entry_s[16];const char*g_last_recomp_func;
static unsigned inherited,pending;static uint16_t saved_s;static uint8_t saved_h;static uint64_t deadline;
void RecompStackPush(const char*n){(void)n;assert(++g_recomp_stack_top<16);}
void RecompStackPop(void){assert(g_recomp_stack_top);--g_recomp_stack_top;}
void RecompStackPopYield(void){RecompStackPop();}
void WatchdogCheck(void){}void cpu_dbg_funcname(const char*n){(void)n;}
void cpu_tailcall_inherit_return_context(uint16_t s,uint8_t h){inherited=1;saved_s=s;saved_h=h;}
int cpu_take_tailcall_return_context(uint16_t*s,uint8_t*h){if(!inherited)return 0;inherited=0;*s=saved_s;*h=saved_h;return 1;}
int interp_bridge_lle_master_deadline_reached(const CpuState*c){return deadline&&c->master_cycles>=deadline;}
RecompReturn interp_bridge_lle_yield_unwind(CpuState*c,uint32_t pc){(void)c;assert(pc==0x80e5);inherited=0;pending=1;return RECOMP_RETURN_LLE_UNWIND_BASE;}
void cpu_instruction_timing_commit(CpuState*c){(void)c;}
int cpu_dispatch_has_entry(CpuState*c,uint32_t pc){(void)c;(void)pc;return 0;}
int cpu_resolve_ancestor_skip(uint16_t s){(void)s;assert(0);return -1;}
int interp_bridge_return_targets_owner(uint16_t s,uint16_t p){(void)s;(void)p;assert(0);return 0;}
int interp_bridge_has_direct_paired_bounce(void){return 1;}
RecompReturn interp_tier_dispatch_rewritten_return(CpuState*c,uint32_t p,uint32_t s){(void)c;(void)p;(void)s;assert(0);return 0;}
RecompReturn interp_tier_dispatch_popped_return(CpuState*c,uint32_t p,uint32_t s,uint16_t e){(void)c;(void)p;(void)s;(void)e;assert(0);return 0;}
RecompReturn cpu_dispatch_pc_from(CpuState*c,uint32_t p,uint16_t e,uint32_t s){(void)c;(void)p;(void)e;(void)s;assert(0);return 0;}
RecompReturn cpu_unresolved_abandon_balanced(CpuState*c,uint32_t p,uint16_t e,uint8_t h){(void)c;(void)p;(void)e;(void)h;assert(0);return 0;}
uint32_t cpu_region_speed(uint32_t a){(void)a;return 8;}
uint8_t cpu_read8(CpuState*c,uint8_t b,uint16_t a){return c->open_bus=read8(0,((uint32_t)b<<16)|a);}
uint16_t cpu_read16(CpuState*c,uint8_t b,uint16_t a){unsigned v=cpu_read8(c,b,a);return v|cpu_read8(c,b,a+1)<<8;}
#include "actor_return_under_test.h"
static void refusal(CpuState*c,uint16_t es,unsigned h,uint32_t host,RecompReturn*out,uint64_t g,uint64_t f,int normal){
 CpuState old=*c;memcpy(before,ram,sizeof ram);RecompReturn prior=out?*out:0;
 assert(!LagoonNativeActorCommands_Continue(rom,sizeof rom,c,es,h,host,out,g,f,normal));
 assert(!memcmp(&old,c,sizeof old)&&!memcmp(before,ram,sizeof ram)&&(!out||*out==prior));refusals++;
}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 for(unsigned sel=0x11;sel<=0x13;sel+=2)for(unsigned n=0;n<32;n++)for(unsigned stop=0;stop<2;stop++){
  CpuState c=prepare(sel,n,n*7,n*3,n*11),initial=c;c.host_return_valid=3;initial.host_return_valid=3;
  memcpy(before,ram,sizeof ram);CpuState expected_cpu=c;assert(LagoonNativeActorCommands_Try(rom,sizeof rom,&expected_cpu,9,9,1));
  Interp816*i=interp816_init(NULL,read8,write8);assert(i);i->a=expected_cpu.A;i->x=expected_cpu.X;i->y=expected_cpu.Y;i->sp=expected_cpu.S;i->dp=0;i->db=1;i->k=0;i->pc=0x80e5;i->e=0;interp816_setFlags(i,expected_cpu.P);
  if(!stop)assert(interp816_runOpcode(i)==6);
  memcpy(expected,ram,sizeof ram);memcpy(ram,before,sizeof ram);deadline=stop?initial.master_cycles:0;pending=0;RecompReturn out=(RecompReturn)99;
  assert(LagoonNativeActorCommands_Continue(rom,sizeof rom,&c,initial.S,3,sel==0x11?0x0181b0:0x0181ba,&out,9,9,1));
  assert(!memcmp(expected,ram,sizeof ram)&&c.A==i->a&&c.X==i->x&&c.Y==i->y&&c.S==i->sp&&c.DB==i->db&&c.P==interp816_getFlags(i));
  /* NORMAL leavesPB restoration to the real generated JSL caller. */
  assert(c.PB==0&&c.cycles==initial.cycles+(stop?0:6)&&c.master_cycles==initial.master_cycles+(stop?0:44));
  assert(c.coprocessor_master_cycles==(stop?initial.coprocessor_master_cycles:initial.master_cycles));
  assert(out==(stop?RECOMP_RETURN_LLE_UNWIND_BASE:RECOMP_RETURN_NORMAL)&&pending==stop&&!inherited&&!g_recomp_stack_top);interp816_free(i);cases++;
 }
 deadline=0;
 for(unsigned n=0;n<39;n++){
  CpuState c=prepare(0x11,0,0,1,2);c.host_return_valid=3;uint16_t es=c.S;unsigned h=3;uint32_t host=0x0181b0;uint64_t gen=9,frame=9;int normal=1;RecompReturn out=(RecompReturn)99,*op=&out;
  switch(n){case 0:c.PB=1;break;case 1:c.DB=2;break;case 2:c.D=1;break;case 3:c.emulation=1;break;case 4:c.m_flag=0;break;case 5:c.x_flag=1;break;case 6:c._flag_D=1;break;case 7:c.P|=8;break;case 8:c.A=0x111;break;case 9:c.X=0;break;case 10:c.X=0x840;break;case 11:c.X=0x41;break;case 12:c.S=0x100;es=c.S;break;case 13:c.S=0x1ffe;es=c.S;break;case 14:ram[c.S+1]^=1;break;case 15:ram[c.S+3]=2;break;case 16:ram[c.S+4]^=1;break;case 17:ram[0x518+c.X]=1;break;case 18:ram[0x500+c.X]=0;break;case 19:ram[0x500]=0;break;case 20:ram[0x520]=0;break;case 21:ram[0x521]=1;break;case 22:ram[0x406]=1;break;case 23:ram[0x100e]=1;break;case 24:ram[0x23d]=14;break;case 25:ram[0x275]=0;break;case 26:put(0x48e,75);break;case 27:c.A=0x12;break;case 28:gen=8;break;case 29:normal=0;break;case 30:es++;break;case 31:h=0;break;case 32:h=2;break;case 33:c.host_return_valid=0;break;case 34:host++;break;case 35:op=NULL;break;case 36:frame++;break;case 37:rom[0xb9]^=1;break;case 38:c.ram=NULL;break;}
  refusal(&c,es,h,host,op,gen,frame,normal);if(n==37)rom[0xb9]^=1;
 }
 printf("PASS compiled actor guard %u positives/deadlines with actual80E5RTL, %u read-only refusals; original physical3-byte caller and zero-clock prefix retained\n",cases,refusals);return 0;
}
