/* Complete original effect/allocator closure, distinct from whole BB05 coverage. */
#include "cpu_state.h"
#include "cpu_trace.h"
#include "common_cpu_infra.h"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef assert
#define assert(v) do {if(!(v)){fprintf(stderr,"line%d: %s\n",__LINE__,#v);exit(2);}}while(0)
static uint8 rom[0x100000],ram[0x20000],before[0x20000],expected[0x20000],io[65536],expected_io[65536];
static unsigned pending,boundary,inherited_pending,fallback_count;
static uint16 inherited_s;static uint8 inherited_hrv;
static uint64_t deadline,bus_extra;
uint8_t g_memsel;int g_recomp_stack_top;uint16_t g_cpu_entry_s[16];const char*g_last_recomp_func;
void RecompStackPush(const char*n){(void)n;g_recomp_stack_top++;}
void RecompStackPop(void){assert(g_recomp_stack_top>0);g_recomp_stack_top--;}
void RecompStackPopYield(void){assert(g_recomp_stack_top>0);g_recomp_stack_top--;}
void WatchdogCheck(void){}void cpu_dbg_funcname(const char*n){(void)n;}
int cpu_take_tailcall_return_context(uint16*s,uint8*h){if(!inherited_pending)return 0;inherited_pending=0;*s=inherited_s;*h=inherited_hrv;return 1;}
void cpu_tailcall_inherit_return_context(uint16 s,uint8 h){inherited_s=s;inherited_hrv=h;inherited_pending=1;}
int interp_bridge_lle_master_deadline_reached(const CpuState*c){return deadline&&c->master_cycles>=deadline;}
int interp_bridge_lle_unwind_pending(void){return pending;}
RecompReturn interp_bridge_lle_yield_unwind(CpuState*c,uint32 pc){(void)c;boundary=pc;pending=1;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;}
void cpu_instruction_timing_commit(CpuState*c){(void)c;}
int cpu_dispatch_has_entry(CpuState*c,uint32 pc){(void)c;(void)pc;assert(0);return 0;}
int interp_bridge_has_direct_paired_bounce(void){assert(0);return 0;}
int cpu_resolve_ancestor_skip(uint16 s){(void)s;assert(0);return -1;}
int interp_bridge_return_targets_owner(uint16 s,uint16 p){(void)s;(void)p;assert(0);return 0;}
RecompReturn interp_tier_dispatch_rewritten_return(CpuState*c,uint32 p,uint32 s){(void)c;(void)p;(void)s;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn interp_tier_dispatch_popped_return(CpuState*c,uint32 p,uint32 s,uint16 e){(void)c;(void)p;(void)s;(void)e;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_dispatch_pc_from(CpuState*c,uint32 p,uint16 e,uint32 s){(void)c;(void)p;(void)e;(void)s;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_unresolved_abandon_balanced(CpuState*c,uint32 p,uint16 e,uint8 h){(void)c;(void)p;(void)e;(void)h;assert(0);return RECOMP_RETURN_NORMAL;}
uint32_t cpu_region_speed(uint32_t a){(void)a;return 8;}
uint8 cpu_read8(CpuState*c,uint8 b,uint16 a){
 int32_t o=cpu_wram_offset(b,a);if(o>=0)return c->open_bus=ram[o];
 if(b<64&&a>=0x2100&&a<0x4400)return c->open_bus=io[a];
 if(a>=32768)return c->open_bus=rom[(b&31)*32768+(a&32767)];return c->open_bus=0;
}
void cpu_write8(CpuState*c,uint8 b,uint16 a,uint8 v){
 c->open_bus=v;int32_t o=cpu_wram_offset(b,a);if(o>=0){ram[o]=v;return;}
 if(b<64&&a>=0x2100&&a<0x4400)io[a]=v;
}
uint16 cpu_read16(CpuState*c,uint8 b,uint16 a){unsigned l=cpu_read8(c,b,a);return l|cpu_read8(c,b,a+1)<<8;}
void cpu_write16(CpuState*c,uint8 b,uint16 a,uint16 v){cpu_write8(c,b,a,v);cpu_write8(c,b,a+1,v>>8);}
#include "projectile_effect_services_under_test.h"
RecompReturn interp_tier_dispatch_tail(CpuState*c,uint32 pc,uint32 site,uint16 s,uint8 h){
 assert(site==0x01be8a&&s==c->S&&h==2);fallback_count++;boundary=pc;pending=1;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;
}
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 p,uint32 s,uint8 n,uint32*out){(void)c;(void)p;(void)s;(void)n;(void)out;assert(0);return RECOMP_RETURN_NORMAL;}
static uint8 rd(void*ctx,uint32 a){
 (void)ctx;bus_extra+=2;int32_t o=cpu_wram_offset(a>>16,a);if(o>=0)return ram[o];
 if((a>>16)<64&&(a&65535)>=0x2100&&(a&65535)<0x4400)return io[a&65535];
 if((a&65535)>=32768)return rom[((a>>16)&31)*32768+(a&32767)];return 0;
}
static void wr(void*ctx,uint32 a,uint8 v){
 (void)ctx;bus_extra+=2;int32_t o=cpu_wram_offset(a>>16,a);if(o>=0)ram[o]=v;
 else if((a>>16)<64&&(a&65535)>=0x2100&&(a&65535)<0x4400)io[a&65535]=v;
}
static void put(unsigned a,unsigned v){ram[a]=v;ram[a+1]=v>>8;}
static unsigned cases,complete,interrupted,entry_deadlines,inherited_cases,dispatch_cases,allocator_cases;
static unsigned born[4],intent_alias,homing_cases,sine_cases,gravity_cases,pool_full,higher_born;
static void compare(unsigned pc,unsigned kind,unsigned clock,unsigned hi,unsigned flags,
 unsigned stack,unsigned phase,unsigned pool,unsigned subtype,unsigned direction,unsigned inherited){
 for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*31+clock*17+flags);
 memset(io,0,sizeof io);CpuState initial={0};initial.A=hi|0x81;
 initial.X=((clock+subtype)%16)*24;initial.Y=clock*29;initial.S=stack;initial.DB=initial.PB=1;initial.ram=ram;initial.host_return_valid=2;
 initial.P=CPU_P_M|CPU_P_I|((flags&1)?CPU_P_C:0)|((flags&2)?CPU_P_V:0)|((flags&4)?CPU_P_N:0)|((flags&8)?CPU_P_Z:0)|((flags&16)?CPU_P_D:0);
 cpu_p_to_mirrors(&initial);ram[stack+1]=0x34;ram[stack+2]=0x12;
 /* Occupied, one/two free, and mixed/empty pool profiles. A real BED1 retires
  * its own slot before allocation, so even initially-full pool has one slot. */
 for(unsigned n=0;n<16;n++){
  unsigned a=0xd40+n*24;memset(ram+a,0,24);ram[a]=(pool==4||((pool==3)&&(n&1)))?0:11;
  put(a+1,500+n);put(a+3,600+n);put(a+5,2);put(a+7,-2);put(a+22,0xffff);
 }
 if(pool==1||pool==2)ram[0xd40+((initial.X/24+1)%16)*24]=0;
 if(pool==2)ram[0xd40+((initial.X/24+2)%16)*24]=0;
 unsigned a=0xd40+initial.X;ram[a]=kind;ram[a+16]=direction;ram[a+18]=clock;
 static const unsigned targets[]={0xffff,0,0x40,0x800,1,2};put(a+22,targets[subtype%6]);
 ram[0x500]=ram[0x540]=ram[0xd00]=0xc0;put(0x502,520);put(0x504,580);put(0x542,480);put(0x544,620);put(0xd02,510);put(0xd04,590);
 if(flags&8)ram[0x540]=0;
 ram[0x53e + initial.X]=255;ram[0x52f]=1;ram[0x1012]=3;ram[0x1013]=direction&7;ram[0x1014]=0;
 put(0x1015,500);put(0x1017,600);put(0x1019,0xffff);
 initial.master_cycles=phase==1?1:0;initial.coprocessor_master_cycles=initial.master_cycles;
 deadline=phase==1?1:phase==2?48:phase==3?112:phase==4?400:0;
 pending=boundary=inherited_pending=0;inherited_pending=inherited;inherited_s=stack;inherited_hrv=2;
 memcpy(before,ram,sizeof ram);CpuState actual=initial;unsigned old_fallback=fallback_count;
 RecompReturn result=effect_body(pc,&actual);assert(!g_recomp_stack_top);cpu_mirrors_to_p(&actual);
 assert(result==RECOMP_RETURN_NORMAL||(pending&&boundary));
 memcpy(expected,ram,sizeof ram);memcpy(expected_io,io,sizeof io);
 if(!pending){
  if(pc==0x019a70){
   allocator_cases++;
   if(pool==0&&!(flags&16)){assert(!memcmp(ram+0xd40,before+0xd40,384));pool_full++;}
  }
  /* Canonical16-slot/three-shot statements require the game's binary mode.
   * Decimal controls below still compare every original byte and clock; they
   * must retain original BCD index arithmetic rather than these semantics. */
  if(!(flags&16)&&(pc==0x01bed1||(pc==0x01be79&&kind==2))&&clock==10){
   unsigned shots=0;for(unsigned n=0;n<16;n++)shots+=ram[0xd40+n*24]==3;
   assert(shots>=1&&shots<=3&&ram[0x52f]==3);born[shots]++;
   assert(ram[0x53e + initial.X]==(255&0xcf));intent_alias++;
   for(unsigned n=initial.X/24+1;n<16;n++)
    if(!before[0xd40+n*24]&&ram[0xd40+n*24]==3)higher_born++;
   /* Fixtures include genuine higher-slot allocations; their SAME-pass BB05
    * execution is independently exercised by the whole-loop source oracle. */
  }
  if(pc==0x01bf20)homing_cases++;
  if(pc==0x01bf83||pc==0x01bf9c)sine_cases++;
  if(pc==0x01bff5)gravity_cases++;
 }
 memcpy(ram,before,sizeof ram);memset(io,0,sizeof io);
 Interp816*i=interp816_init(NULL,rd,wr);assert(i);i->a=initial.A;i->x=initial.X;i->y=initial.Y;i->sp=initial.S;i->dp=0;i->db=initial.DB;i->k=1;i->pc=pc;i->e=0;interp816_setFlags(i,initial.P);
 unsigned stop=boundary?boundary:0x011235,ops=0;uint64_t cycles=0,last=initial.master_cycles;bus_extra=0;
 while(cycles<actual.cycles){assert(++ops<10000);last=initial.master_cycles+cycles*6+bus_extra;cycles+=interp816_runOpcode(i);}
 unsigned first=0;while(first<sizeof ram&&ram[first]==expected[first])first++;
 if(first<sizeof ram||memcmp(io,expected_io,sizeof io)||i->a!=actual.A||i->x!=actual.X||i->y!=actual.Y||i->sp!=actual.S||i->dp!=actual.D||i->db!=actual.DB||i->k!=actual.PB||interp816_getFlags(i)!=actual.P||(((unsigned)i->k<<16)|i->pc)!=stop){
  fprintf(stderr,"STATE pc%06x kind%u clock%u hi%04x flags%u S%04x phase%u pool%u subtype%u RAM%05x A%04x/%04x X%04x/%04x Y%04x/%04x S%04x/%04x P%02x/%02x PC%06x/%06x\n",pc,kind,clock,hi,flags,stack,phase,pool,subtype,first,i->a,actual.A,i->x,actual.X,i->y,actual.Y,i->sp,actual.S,interp816_getFlags(i),actual.P,((unsigned)i->k<<16)|i->pc,stop);assert(0);
 }
 if(actual.cycles!=cycles||actual.master_cycles!=initial.master_cycles+cycles*6+bus_extra||actual.coprocessor_master_cycles!=last){fprintf(stderr,"CLOCK pc%06x kind%u clock%u phase%u CPU%llu/%llu master%llu/%llu copro%llu/%llu\n",pc,kind,clock,phase,(unsigned long long)actual.cycles,(unsigned long long)cycles,(unsigned long long)actual.master_cycles,(unsigned long long)(initial.master_cycles+cycles*6+bus_extra),(unsigned long long)actual.coprocessor_master_cycles,(unsigned long long)last);assert(0);}
 cases++;if(pending)interrupted++;else complete++;if(phase==1)entry_deadlines++;if(inherited)inherited_cases++;if(pc==0x01be79)dispatch_cases++;
 assert(fallback_count==old_fallback||pc==0x01be79);interp816_free(i);
}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 const unsigned stacks[]={0x180,0x1fd0},highs[]={0,0x100,0xff00};
 for(unsigned kind=0;kind<34;kind++)for(unsigned clock=0;clock<256;clock++)
  compare(effect_targets[kind],kind,clock,0,clock&15,stacks[clock&1],0,clock%5,clock%6,clock&7,clock&1);
 for(unsigned kind=0;kind<256;kind++)for(unsigned phase=0;phase<5;phase++)for(unsigned st=0;st<2;st++)
  compare(0x01be79,kind,kind*7,0,kind&15,stacks[st],phase,kind%5,kind%6,kind&7,st);
 const unsigned special[]={0x01bed1,0x01bf1f,0x01bf20,0x01bf83,0x01bf9c,0x01bff5,0x019a70};
 for(unsigned n=0;n<7;n++)for(unsigned hi=0;hi<3;hi++)for(unsigned flags=0;flags<32;flags++)for(unsigned phase=0;phase<5;phase++)
  compare(special[n],n?8:2,n?flags*17:10,highs[hi],flags,stacks[flags&1],phase,flags%5,flags%6,flags&7,flags&1);
 for(unsigned pool=0;pool<5;pool++)for(unsigned direction=0;direction<8;direction++)for(unsigned sub=0;sub<6;sub++){
  compare(0x01be79,2,10,0,0,stacks[sub&1],0,pool,sub,direction,sub&1);
  compare(0x019a70,2,9,0,0,stacks[sub&1],0,pool,sub,direction,sub&1);
 }
 printf("PASS effects%u complete%u interrupted-or-boundary%u entry-deadline%u inherited-frame%u full-dispatch%u unknown-pointer-floor%u actualallocator%u; fork1/2/3=%u/%u/%u intent-alias%u fullpool-no-spawn%u higher-slot-births%u homing%u sine%u gravity%u; all34slots/all256byte-selectors+aliases, hiddenA/flags/physicalRTS/deadlines, fullRAM+IO/registers/CPU-master-copro exact; constant8master unitbus\n",cases,complete,interrupted,entry_deadlines,inherited_cases,dispatch_cases,fallback_count,allocator_cases,born[1],born[2],born[3],intent_alias,pool_full,higher_born,homing_cases,sine_cases,gravity_cases);
 assert(complete&&interrupted&&entry_deadlines&&inherited_cases&&dispatch_cases&&fallback_count&&allocator_cases&&born[1]&&born[2]&&born[3]&&intent_alias&&pool_full&&higher_born&&homing_cases&&sine_cases&&gravity_cases);return 0;
}
