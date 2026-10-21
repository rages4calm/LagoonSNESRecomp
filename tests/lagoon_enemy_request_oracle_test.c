/* Actual enemy request source, original children and complete native closure.
 * IO is deterministic byte storage, not a simulated SPC or physical device. */
#include "cpu_state.h"
#include "cpu_trace.h"
#include "common_cpu_infra.h"
#include "snes/interp816.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(v) do{if(!(v)){fprintf(stderr,"line%d %s\n",__LINE__,#v);exit(2);}}while(0)
static uint8 rom[0x100000],ram[0x20000],before[0x20000],expected[0x20000],io[65536],expected_io[65536];
typedef struct PortEvent{uint16 address;uint8 write,value;}PortEvent;
static PortEvent events[4096],expected_events[4096];static unsigned event_count,expected_event_count;
static unsigned pending,boundary,inherited_pending;static uint16 inherited_s;static uint8 inherited_h;
static uint64_t deadline,bus_extra,source_ops;static unsigned child_calls,cases,completed,interrupted,spawned,apu_cases;
static unsigned case_kind,case_slot,case_direction,case_pool,case_phase,case_flags,case_high,case_caller,unknown_targets;
uint8_t g_memsel;int g_recomp_stack_top;uint16_t g_cpu_entry_s[32];const char*g_last_recomp_func;
void RecompStackPush(const char*n){(void)n;CHECK(++g_recomp_stack_top<32);}
void RecompStackPop(void){CHECK(g_recomp_stack_top>0);--g_recomp_stack_top;}
void RecompStackPopYield(void){RecompStackPop();}
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
int cpu_resolve_ancestor_skip(uint16 s){(void)s;CHECK(0);return -1;}
int interp_bridge_return_targets_owner(uint16 s,uint16 p){(void)s;(void)p;CHECK(0);return 0;}
RecompReturn interp_tier_dispatch_rewritten_return(CpuState*c,uint32 p,uint32 s){(void)c;(void)p;(void)s;CHECK(0);return RECOMP_RETURN_NORMAL;}
RecompReturn interp_tier_dispatch_popped_return(CpuState*c,uint32 p,uint32 s,uint16 e){(void)c;(void)p;(void)s;(void)e;CHECK(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_dispatch_pc_from(CpuState*c,uint32 p,uint16 e,uint32 s){(void)c;(void)p;(void)e;(void)s;CHECK(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_unresolved_abandon_balanced(CpuState*c,uint32 p,uint16 e,uint8 h){(void)c;(void)p;(void)e;(void)h;CHECK(0);return RECOMP_RETURN_NORMAL;}
uint32_t cpu_region_speed(uint32_t a){(void)a;return 8;}
static int is_io(uint32 a){return (a>>16)<64&&(a&65535)>=0x2100&&(a&65535)<0x4400;}
static void event(uint32 a,uint8 w,uint8 value){CHECK(event_count<4096);events[event_count++]=(PortEvent){(uint16)a,w,value};}
static uint8 peek(uint32 a){int32_t o=cpu_wram_offset(a>>16,a);if(o>=0)return ram[o];if(is_io(a))return io[a&65535];if((a&65535)>=32768)return rom[((a>>16)&31)*32768+(a&32767)];return 0;}
static uint8 read(uint32 a){uint8 v=peek(a);if(is_io(a))event(a,0,v);return v;}
static void write(uint32 a,uint8 v){int32_t o=cpu_wram_offset(a>>16,a);if(o>=0){ram[o]=v;return;}if(is_io(a)){event(a,1,v);io[a&65535]=v;}}
uint8 cpu_read8(CpuState*c,uint8 b,uint16 a){return c->open_bus=read(((uint32)b<<16)|a);}
void cpu_write8(CpuState*c,uint8 b,uint16 a,uint8 v){c->open_bus=v;write(((uint32)b<<16)|a,v);}
uint16 cpu_read16(CpuState*c,uint8 b,uint16 a){unsigned l=cpu_read8(c,b,a);return l|cpu_read8(c,b,a+1)<<8;}
void cpu_write16(CpuState*c,uint8 b,uint16 a,uint16 v){cpu_write8(c,b,a,v);cpu_write8(c,b,a+1,v>>8);}
static uint8 rd(void*ctx,uint32 a){(void)ctx;bus_extra+=2;return read(a);}
static void wr(void*ctx,uint32 a,uint8 v){(void)ctx;bus_extra+=2;write(a,v);}
static void put(unsigned a,unsigned v){ram[a]=v;ram[a+1]=v>>8;}
static Interp816*make_interp(CpuState*c,unsigned pc){Interp816*i=interp816_init(NULL,rd,wr);CHECK(i);cpu_mirrors_to_p(c);i->a=c->A;i->x=c->X;i->y=c->Y;i->sp=c->S;i->dp=c->D;i->db=c->DB;i->k=pc>>16;i->pc=pc;i->e=c->emulation;interp816_setFlags(i,c->P);return i;}
static RecompReturn source_child(CpuState*c,unsigned pc,uint32*out){
 ++child_calls;unsigned stack=c->S,n=c->host_return_valid;CHECK(n==2||n==3);
 unsigned bank=n==3?ram[(stack+3)&65535]:(pc>>16);unsigned stop=(bank<<16)|((ram[(stack+1)&65535]|ram[(stack+2)&65535]<<8)+1);
 Interp816*i=make_interp(c,pc);uint64_t cycles=0,base=c->master_cycles,last=c->coprocessor_master_cycles;bus_extra=0;unsigned ops=0;
 while((((unsigned)i->k<<16)|i->pc)!=stop||i->sp!=((stack+n)&65535)){
  if(deadline&&base+cycles*6+bus_extra>=deadline){pending=1;boundary=((unsigned)i->k<<16)|i->pc;break;}
  CHECK(++ops<40000);CHECK(peek(((unsigned)i->k<<16)|i->pc));last=base+cycles*6+bus_extra;cycles+=interp816_runOpcode(i);
 }
 source_ops+=ops;c->A=i->a;c->X=i->x;c->Y=i->y;c->S=i->sp;c->D=i->dp;c->DB=i->db;c->PB=i->k;c->P=interp816_getFlags(i);cpu_p_to_mirrors(c);c->cycles+=cycles;c->master_cycles=base+cycles*6+bus_extra;c->coprocessor_master_cycles=last;
 if(out)*out=((unsigned)i->k<<16)|i->pc;interp816_free(i);return pending?(RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE:RECOMP_RETURN_NORMAL;
}
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 pc,uint32 site,uint8 n,uint32*out){(void)site;CHECK(n==2||n==3);CHECK(c->host_return_valid==n);return source_child(c,pc,out);}
RecompReturn interp_tier_dispatch_tail(CpuState*c,uint32 pc,uint32 site,uint16 s,uint8 h){
 CHECK(h==2&&s==c->S);
 if(site==0x0195af){CHECK(pc!=0x01977f);++unknown_targets;pending=1;boundary=pc;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;}
 return source_child(c,pc,NULL);
}
#ifdef WHOLE_NATIVE_CLOSURE
#include "enemy_request_closure.h"
#else
#include "enemy_request_children.h"
#endif
static void compare(unsigned kind,unsigned slot,unsigned direction,unsigned pool,unsigned phase,unsigned flags,unsigned high,unsigned attack,unsigned caller){
 case_kind=kind;case_slot=slot;case_direction=direction;case_pool=pool;case_phase=phase;case_flags=flags;case_high=high;case_caller=caller;
 memset(ram,0,sizeof ram);memset(io,0,sizeof io);unsigned a=0x500+slot*64;
 ram[a]=0xc8;ram[a+0x18]=caller>=4?caller-4:2;ram[a+0x1b]=kind;ram[a+0x3e]=0xc1|(attack?0x10:0);ram[a+0xa]=direction;
 ram[0x51]=caller==2?0:1;ram[a+1]=caller==3?1:0;
 put(a+2,0xfffc);put(a+4,4);put(0x1019,0xfffe);ram[0x431]=0x40;ram[0x432]=0;
 for(unsigned n=0;n<16;++n){unsigned p=0xd40+n*24;ram[p]=pool==0?0:pool==1?0x21:pool==2?(n!=15?0x21:0):(n!=7&&n!=11?0x21:0);memset(ram+p+1,0x5a,23);}
 CpuState initial={0};initial.A=high|0x81;initial.X=slot*64;initial.Y=0x12fe;initial.S=(cases&1)?0x1fd0:0x180;initial.DB=initial.PB=1;initial.P=0x24|((flags&1)?1:0)|((flags&2)?0xc0:0)|((flags&4)?8:0);initial.host_return_valid=2;initial.ram=ram;cpu_p_to_mirrors(&initial);put(initial.S+1,0x81a6);
 deadline=phase==0?0:phase==1?1:phase==2?100:phase==3?1000:phase==4?4000:20000;pending=boundary=inherited_pending=event_count=0;memcpy(before,ram,sizeof ram);CpuState actual=initial;
 RecompReturn result=caller?bank_01_9592_M1X0(&actual):Lagoon_Enemy_ProjectileRequest_M1X0(&actual);CHECK(!g_recomp_stack_top);cpu_mirrors_to_p(&actual);CHECK(result==RECOMP_RETURN_NORMAL||(pending&&boundary));
 memcpy(expected,ram,sizeof ram);memcpy(expected_io,io,sizeof io);expected_event_count=event_count;memcpy(expected_events,events,event_count*sizeof(*events));
 if(pending)++interrupted;else{++completed;CHECK(actual.S==initial.S+2);}if(event_count)++apu_cases;
 for(unsigned n=0;n<16;++n)spawned+=ram[0xd40+n*24]&&ram[0xd40+n*24]!=before[0xd40+n*24];
 memcpy(ram,before,sizeof ram);memset(io,0,sizeof io);event_count=0;Interp816*i=make_interp(&initial,caller?0x019592:0x01977f);unsigned stop=boundary?boundary:0x0181a7,ops=0;uint64_t cycles=0,last=0;bus_extra=0;
 while(cycles<actual.cycles){CHECK(++ops<100000);last=cycles*6+bus_extra;CHECK(peek(((unsigned)i->k<<16)|i->pc));cycles+=interp816_runOpcode(i);}
 unsigned first=0;while(first<sizeof ram&&ram[first]==expected[first])++first;
 if(first<sizeof ram||memcmp(io,expected_io,sizeof io)||event_count!=expected_event_count||memcmp(events,expected_events,event_count*sizeof(*events))||i->a!=actual.A||i->x!=actual.X||i->y!=actual.Y||i->sp!=actual.S||i->dp!=actual.D||i->db!=actual.DB||i->k!=actual.PB||interp816_getFlags(i)!=actual.P||(((unsigned)i->k<<16)|i->pc)!=stop||cycles!=actual.cycles||cycles*6+bus_extra!=actual.master_cycles||last!=actual.coprocessor_master_cycles){
  fprintf(stderr,"STATE kind%02X slot%u dir%u pool%u ph%u flags%u high%04X RAM%05X A%04X/%04X X%04X/%04X Y%04X/%04X S%04X/%04X PB%02X/%02X P%02X/%02X PC%06X/%06X CPU%llu/%llu MASTER%llu/%llu COPRO%llu/%llu IOevents%u/%u\n",kind,slot,direction,pool,phase,flags,high,first,i->a,actual.A,i->x,actual.X,i->y,actual.Y,i->sp,actual.S,i->k,actual.PB,interp816_getFlags(i),actual.P,((unsigned)i->k<<16)|i->pc,stop,(unsigned long long)cycles,(unsigned long long)actual.cycles,(unsigned long long)(cycles*6+bus_extra),(unsigned long long)actual.master_cycles,(unsigned long long)last,(unsigned long long)actual.coprocessor_master_cycles,event_count,expected_event_count);CHECK(0);
 }
 interp816_free(i);++cases;
}
int main(int argc,char**argv){CHECK(argc==2);FILE*f=fopen(argv[1],"rb");CHECK(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 unsigned kinds[]={7,11,14,15,21,26,27,36,37,38};
 for(unsigned k=0;k<10;++k)for(unsigned slot=0;slot<3;++slot)for(unsigned dir=0;dir<8;++dir)for(unsigned pool=0;pool<4;++pool)for(unsigned phase=0;phase<6;++phase)compare(kinds[k],slot==2?32:slot,dir,pool,phase,dir&3,0,1,0);
 for(unsigned kind=0;kind<256;++kind){compare(kind,1,3,0,0,0,0,1,0);compare(kind,32,7,1,0,7,0xff00,0,0);}
 for(unsigned slot=0;slot<33;++slot)for(unsigned phase=0;phase<6;++phase)compare(14,slot,6,2,phase,slot&7,0,1,0);
 for(unsigned flags=0;flags<8;++flags)for(unsigned high=0;high<3;++high)for(unsigned phase=1;phase<4;++phase)compare(14,1,0,3,phase,flags,high==2?0xff00:high<<8,1,0);
 for(unsigned k=0;k<10;++k)for(unsigned slot=0;slot<3;++slot)for(unsigned pool=0;pool<4;++pool)for(unsigned phase=0;phase<6;++phase)compare(kinds[k],slot==2?32:slot,k&7,pool,phase,k&7,0,1,1);
 for(unsigned caller=1;caller<4;++caller)for(unsigned phase=0;phase<6;++phase)compare(14,1,7,0,phase,7,0,1,caller);
 for(unsigned type=0;type<256;++type)for(unsigned phase=0;phase<6;++phase)compare(14,1,5,0,phase,0,0,1,type+4);
 CHECK(completed&&interrupted&&spawned&&apu_cases&&unknown_targets);
 printf("PASS enemy request %u cases completed%u deadline-boundaries%u source-child-calls%u source-opcodes%llu allocated-records%u ordered-APU-IO-cases%u; full128KiBRAM+IO+physical2/3byteframes+registers+flags+hiddenA+CPU/master/copro clocks, deterministic8master bus; no liveSPC claim\n",cases,completed,interrupted,child_calls,(unsigned long long)source_ops,spawned,apu_cases);return 0;
}
