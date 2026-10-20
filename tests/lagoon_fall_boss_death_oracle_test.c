/* Actual generated source versus original 65816 instructions. B936 is the
 * ORIGINAL interpreter service with its real JSR/JSL/PLA/JML/RTS frames.
 * Unknown service inputs and existing reward/attack work STOP at real entries. */
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
static uint8 rom[0x100000],ram[0x20000],before[0x20000],expected[0x20000];
static unsigned boundary,service_calls,service_selectors[16];
static uint16 inherited_s;
static unsigned inherited_pending;
static int pending;
static uint64_t deadline,bus_extra;
uint8_t g_memsel;
int g_recomp_stack_top;
uint16_t g_cpu_entry_s[16];
const char *g_last_recomp_func;
void RecompStackPush(const char*n){(void)n;g_recomp_stack_top++;}
void RecompStackPop(void){assert(g_recomp_stack_top>0);g_recomp_stack_top--;}
void RecompStackPopYield(void){assert(g_recomp_stack_top>0);g_recomp_stack_top--;}
void WatchdogCheck(void){}
void cpu_dbg_funcname(const char*n){(void)n;}
int cpu_take_tailcall_return_context(uint16*s,uint8*h){if(!inherited_pending)return 0;inherited_pending=0;*s=inherited_s;*h=2;return 1;}
void cpu_tailcall_inherit_return_context(uint16 s,uint8 h){assert(h==2);inherited_s=s;inherited_pending=1;}
int interp_bridge_lle_master_deadline_reached(const CpuState*c){return deadline&&c->master_cycles>=deadline;}
int interp_bridge_lle_unwind_pending(void){return pending;}
RecompReturn interp_bridge_lle_yield_unwind(CpuState*c,uint32 pc){(void)c;boundary=pc;pending=1;return RECOMP_RETURN_NORMAL;}
void cpu_instruction_timing_commit(CpuState*c){(void)c;}
int cpu_dispatch_has_entry(CpuState*c,uint32 pc){(void)c;(void)pc;assert(0);return 0;}
int interp_bridge_has_direct_paired_bounce(void){assert(0);return 0;}
int cpu_resolve_ancestor_skip(uint16 s){(void)s;assert(0);return -1;}
int interp_bridge_return_targets_owner(uint16 s,uint16 post){(void)s;(void)post;assert(0);return 0;}
RecompReturn interp_tier_dispatch_rewritten_return(CpuState*c,uint32 pc,uint32 site){(void)c;(void)pc;(void)site;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn interp_tier_dispatch_popped_return(CpuState*c,uint32 pc,uint32 site,uint16 s){(void)c;(void)pc;(void)site;(void)s;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_dispatch_pc_from(CpuState*c,uint32 pc,uint16 s,uint32 site){(void)c;(void)pc;(void)site;(void)s;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_unresolved_abandon_balanced(CpuState*c,uint32 site,uint16 s,uint8 h){(void)c;(void)site;(void)s;(void)h;assert(0);return RECOMP_RETURN_NORMAL;}
uint32_t cpu_region_speed(uint32_t a){(void)a;return 8;}
uint8 cpu_read8(CpuState*c,uint8 b,uint16 a){int32_t off=cpu_wram_offset(b,a);if(off>=0)return c->open_bus=ram[off];assert(b<32&&a>=0x8000);return c->open_bus=rom[b*0x8000+(a&0x7fff)];}
void cpu_write8(CpuState*c,uint8 b,uint16 a,uint8 v){int32_t off=cpu_wram_offset(b,a);assert(off>=0);ram[off]=v;c->open_bus=v;}
uint16 cpu_read16(CpuState*c,uint8 b,uint16 a){unsigned lo=cpu_read8(c,b,a);return lo|((unsigned)cpu_read8(c,b,a+1)<<8);}
void cpu_write16(CpuState*c,uint8 b,uint16 a,uint16 v){cpu_write8(c,b,a,v);cpu_write8(c,b,a+1,v>>8);}
RecompReturn interp_tier_dispatch_tail(CpuState*c,uint32 pc,uint32 site,uint16 s,uint8 h){(void)c;(void)site;(void)s;assert(h==2);boundary=pc;return RECOMP_RETURN_NORMAL;}
static uint8 rd(void*ctx,uint32 a){(void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);if(off>=0)return ram[off];assert((a>>16)<32&&(a&65535)>=32768);return rom[(a>>16)*0x8000+(a&32767)];}
static void wr(void*ctx,uint32 a,uint8 v){(void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);assert(off>=0);ram[off]=v;}
static Interp816* original(const CpuState*c,unsigned pc){
 Interp816*i=interp816_init(NULL,rd,wr);assert(i);i->a=c->A;i->x=c->X;i->y=c->Y;
 i->sp=c->S;i->dp=c->D;i->db=c->DB;i->k=pc>>16;i->pc=pc;i->e=c->emulation;
 interp816_setFlags(i,c->P);return i;
}
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 pc,uint32 site,uint8 n,uint32*out){
 if(pc!=0x01b936){pending=1;boundary=pc;return (RecompReturn)1;}
 assert(site==0x01b5bf&&n==2&&ram[c->S+1]==0xc1&&ram[c->S+2]==0xb5);
 /* The original inline service's TAY retains hidden A-high. Only its actual
  * sixteen source-table inputs are executed to completion in this unit bus. */
 unsigned selector=ram[0x51b+c->X];
 if((c->A&0xff00)||selector>=16){pending=1;boundary=pc;return (RecompReturn)1;}
 cpu_mirrors_to_p(c);Interp816*i=original(c,pc);uint64_t cycles=0,last=0;unsigned ops=0;bus_extra=0;
 while((((unsigned)i->k<<16)|i->pc)!=0x01b5c2){assert(++ops<100);last=cycles*6+bus_extra;cycles+=interp816_runOpcode(i);}
 c->A=i->a;c->X=i->x;c->Y=i->y;c->S=i->sp;c->D=i->dp;c->DB=i->db;c->PB=i->k;
 c->P=interp816_getFlags(i);cpu_p_to_mirrors(c);
 c->coprocessor_master_cycles=c->master_cycles+last;c->master_cycles+=cycles*6+bus_extra;c->cycles+=cycles;
 if(out)*out=0x01b5c2;interp816_free(i);service_calls++;service_selectors[selector]++;
 return RECOMP_RETURN_NORMAL;
}
#include "fall_boss_death_under_test.h"
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 unsigned count=0,complete=0,children=0,deadlines=0;
 const unsigned times[]={0,1,4,5,6,7,8,9,15,16,17,254,255};
 const unsigned stages[]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,254,255};
 const unsigned highs[]={0,0x100,0xff00},stacks[]={0x180,0x1fd0};
 for(unsigned body=0;body<4;body++)for(unsigned stack=0;stack<2;stack++)
 for(unsigned hi=0;hi<3;hi++)for(unsigned flags=0;flags<16;flags++)
 for(unsigned scenario=0;scenario<234;scenario++)for(unsigned early=0;early<2;early++){
  for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*73+count*17);
  CpuState initial={0};initial.A=highs[hi]|(uint8)(count*73);initial.X=(scenario%8+1)*0x40;
  initial.Y=count*29;initial.S=stacks[stack];initial.DB=initial.PB=1;
  initial.P=0x24|((flags&1)?1:0)|((flags&2)?0x40:0)|((flags&4)?0x80:0)|((flags&8)?8:0);
  initial.ram=ram;initial.host_return_valid=2;cpu_p_to_mirrors(&initial);
  unsigned timer=times[scenario%13],phase=stages[scenario/13];
  ram[initial.S+1]=0x34;ram[initial.S+2]=0x12;
  ram[0x500]=0xc0;ram[0x501]=phase&3;ram[0x510]=timer;ram[0x511]=phase;
  ram[0x500+initial.X]=0xc8;ram[0x501+initial.X]=1;
  ram[0x518+initial.X]=3;ram[0x51b+initial.X]=phase;
  for(unsigned k=0;k<3;k++){
   ram[0x538+initial.X+k]=(uint8)(timer+k);
   ram[0x53b+initial.X+k]=(uint8)((phase+k)%18);
  }
  /* Cross all sixteen real cleanup kinds at progress0A, both sides of the
   * final progress10 tail and raw timer/progress byte wrapping. */
  if((scenario&3)==0)ram[0x53b+initial.X]=10;
  if((scenario&7)==1)for(unsigned k=0;k<3;k++)ram[0x53b+initial.X+k]=16;
  if((scenario&7)==2)for(unsigned k=0;k<3;k++)ram[0x53b+initial.X+k]=255;
  memcpy(before,ram,sizeof ram);CpuState actual=initial;boundary=0;pending=0;
  inherited_s=initial.S;inherited_pending=1;deadline=early?1:0;
  if(early){actual.master_cycles=1;initial.master_cycles=1;}
  RecompReturn result=death_body(body,&actual);
  assert(result==RECOMP_RETURN_NORMAL||(result==1&&pending&&boundary));assert(!g_recomp_stack_top);cpu_mirrors_to_p(&actual);
  unsigned stop=boundary?boundary:0x011235;
  memcpy(expected,ram,sizeof ram);memcpy(ram,before,sizeof ram);
  Interp816*i=original(&initial,death_starts[body]);unsigned ops=0;uint64_t cycles=0,last=0;bus_extra=0;
  while((((unsigned)i->k<<16)|i->pc)!=stop){assert(++ops<256);last=cycles*6+bus_extra;cycles+=interp816_runOpcode(i);}
  if(memcmp(ram,expected,sizeof ram)||i->a!=actual.A||i->x!=actual.X||i->y!=actual.Y||i->sp!=actual.S||
     i->dp!=actual.D||i->db!=actual.DB||i->k!=actual.PB||interp816_getFlags(i)!=actual.P){
   fprintf(stderr,"state body%06x scenario%u hi%04x flags%u early%u stop%06x A%04x/%04x X%04x/%04x Y%04x/%04x S%04x/%04x P%02x/%02x\n",death_starts[body],scenario,highs[hi],flags,early,stop,i->a,actual.A,i->x,actual.X,i->y,actual.Y,i->sp,actual.S,interp816_getFlags(i),actual.P);assert(0);
  }
  if(actual.cycles!=cycles||actual.master_cycles!=initial.master_cycles+cycles*6+bus_extra||
     actual.coprocessor_master_cycles!=(ops?initial.master_cycles+last:0)){
   fprintf(stderr,"timing body%06x scenario%u cpu%llu/%llu master%llu/%llu copro%llu/%llu\n",death_starts[body],scenario,(unsigned long long)actual.cycles,(unsigned long long)cycles,(unsigned long long)actual.master_cycles,(unsigned long long)(initial.master_cycles+cycles*6+bus_extra),(unsigned long long)actual.coprocessor_master_cycles,(unsigned long long)(ops?initial.master_cycles+last:0));assert(0);
  }
  if(early)deadlines++;else if(boundary)children++;else complete++;
  interp816_free(i);count++;
 }
 assert(complete&&children&&deadlines&&service_calls);
 for(unsigned kind=0;kind<16;kind++)assert(service_selectors[kind]);
 printf("PASS %u actual generated fall/boss-death and selector/original-opcode cases; complete=%u realchild-boundary=%u deadline-before-entry=%u actualB936services=%u all16selectors; full128KiB RAM/registers/flags/Ahigh/saved+deadstack/CPU+master+coprocessor clocks\n",count,complete,children,deadlines,service_calls);
 return 0;
}
