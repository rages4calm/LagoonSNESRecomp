/* Complete original RNG + seed routine, including real divider-bus accesses.
 * Generated bodies are compared to original opcodes, not substitute formulas. */
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
static unsigned boundary;
static uint8 io[0x10000],expected_io[0x10000];
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
uint8 cpu_read8(CpuState*c,uint8 b,uint16 a){int32_t off=cpu_wram_offset(b,a);if(off>=0)return c->open_bus=ram[off];if(b<0x40&&a>=0x4200&&a<0x4300)return c->open_bus=io[a];assert(b<32&&a>=0x8000);return c->open_bus=rom[b*0x8000+(a&0x7fff)];}
static void io_write(unsigned a,uint8 v) {
 io[a]=v;
 if(a==0x4206){unsigned n=io[0x4204]|((unsigned)io[0x4205]<<8);unsigned q=v?n/v:65535;
  io[0x4214]=q;io[0x4215]=q>>8;}
}
void cpu_write8(CpuState*c,uint8 b,uint16 a,uint8 v){int32_t off=cpu_wram_offset(b,a);
 if(off>=0)ram[off]=v;else{assert(b<0x40&&a>=0x4200&&a<0x4300);io_write(a,v);}c->open_bus=v;}
uint16 cpu_read16(CpuState*c,uint8 b,uint16 a){unsigned lo=cpu_read8(c,b,a);return lo|((unsigned)cpu_read8(c,b,a+1)<<8);}
void cpu_write16(CpuState*c,uint8 b,uint16 a,uint16 v){cpu_write8(c,b,a,v);cpu_write8(c,b,a+1,v>>8);}
RecompReturn interp_tier_dispatch_tail(CpuState*c,uint32 pc,uint32 site,uint16 s,uint8 h){(void)c;(void)site;(void)s;assert(h==2);boundary=pc;return RECOMP_RETURN_NORMAL;}
static uint8 rd(void*ctx,uint32 a){
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);if(off>=0)return ram[off];
 if((a>>16)<0x40&&(a&65535)>=0x4200&&(a&65535)<0x4300)return io[a&65535];
 assert((a>>16)<32&&(a&65535)>=32768);return rom[(a>>16)*0x8000+(a&32767)];
}
static void wr(void*ctx,uint32 a,uint8 v){
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);if(off>=0){ram[off]=v;return;}
 assert((a>>16)<0x40&&(a&65535)>=0x4200&&(a&65535)<0x4300);a&=65535;io_write(a,v);
}
static Interp816* original(const CpuState*c,unsigned pc){
 Interp816*i=interp816_init(NULL,rd,wr);assert(i);i->a=c->A;i->x=c->X;i->y=c->Y;
 i->sp=c->S;i->dp=c->D;i->db=c->DB;i->k=pc>>16;i->pc=pc;i->e=c->emulation;
 interp816_setFlags(i,c->P);return i;
}
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 pc,uint32 site,uint8 n,uint32*out){
 (void)c;(void)pc;(void)site;(void)n;(void)out;assert(0);return RECOMP_RETURN_NORMAL;
}
#include "rng_under_test.h"
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 unsigned count=0,complete=0,deadlines=0,spins=0;const unsigned highs[]={0,0x100,0xff00},stacks[]={0x180,0x1fd0};
 for(unsigned body=0;body<2;body++)for(unsigned stack=0;stack<2;stack++)
 for(unsigned hi=0;hi<3;hi++)for(unsigned flags=0;flags<16;flags++)
 for(unsigned value=0;value<256;value++)for(unsigned seed=0;seed<(body?1:16);seed++)
 for(unsigned early=0;early<2;early++){
  for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*73+count*17);
  memset(io,0,sizeof io);CpuState initial={0};initial.A=highs[hi]|value;
  initial.X=(uint16)(count*97);initial.Y=(uint16)(count*29);initial.D=(count&4)?0x100:0;
  initial.S=stacks[stack];initial.DB=(count>>2)%3;initial.PB=0;
  initial.P=0x24|((flags&1)?1:0)|((flags&2)?0x40:0)|((flags&4)?0x80:0)|((flags&8)?8:0);
  initial.ram=ram;initial.host_return_valid=body?2:3;cpu_p_to_mirrors(&initial);
  ram[initial.S+1]=0x34;ram[initial.S+2]=0x12;ram[initial.S+3]=0;
  ram[0x3b5]=body?value:(seed*17);ram[0x3b4]=(uint8)(value^0x5a);
  memcpy(before,ram,sizeof ram);CpuState actual=initial;boundary=0;pending=0;
  inherited_pending=0;deadline=early?1:0;if(early){actual.master_cycles=1;initial.master_cycles=1;}
  RecompReturn result=rng_body(body,&actual);assert(result==RECOMP_RETURN_NORMAL);assert(!g_recomp_stack_top);cpu_mirrors_to_p(&actual);
  unsigned stop=boundary?boundary:0x001235;
  memcpy(expected,ram,sizeof ram);memcpy(expected_io,io,sizeof io);
  memcpy(ram,before,sizeof ram);memset(io,0,sizeof io);
  Interp816*i=original(&initial,body?0x009848:0x009800);unsigned ops=0;uint64_t cycles=0,last=0;bus_extra=0;
  while((((unsigned)i->k<<16)|i->pc)!=stop){assert(++ops<1600);last=cycles*6+bus_extra;cycles+=interp816_runOpcode(i);}
  if(memcmp(ram,expected,sizeof ram)||memcmp(io,expected_io,sizeof io)||i->a!=actual.A||i->x!=actual.X||i->y!=actual.Y||i->sp!=actual.S||
     i->dp!=actual.D||i->db!=actual.DB||i->k!=actual.PB||interp816_getFlags(i)!=actual.P){
   for(unsigned z=0;z<sizeof ram;z++)if(ram[z]!=expected[z]){fprintf(stderr,"firstRAM%05x original%02x native%02x\n",z,ram[z],expected[z]);break;}
   for(unsigned z=0;z<sizeof io;z++)if(io[z]!=expected_io[z]){fprintf(stderr,"firstIO%04x original%02x native%02x\n",z,io[z],expected_io[z]);break;}
   fprintf(stderr,"state body%u value%u seed%u flags%u hi%u early%u A%04x/%04x S%04x/%04x P%02x/%02x D%04x/%04x DB%02x/%02x PB%02x/%02x X%04x/%04x Y%04x/%04x\n",body,value,seed,flags,hi,early,i->a,actual.A,i->sp,actual.S,interp816_getFlags(i),actual.P,i->dp,actual.D,i->db,actual.DB,i->k,actual.PB,i->x,actual.X,i->y,actual.Y);assert(0);
  }
  if(actual.cycles!=cycles||actual.master_cycles!=initial.master_cycles+cycles*6+bus_extra||actual.coprocessor_master_cycles!=(ops?initial.master_cycles+last:0)){
   fprintf(stderr,"timing body%u value%u flags%u early%u cpu%llu/%llu master%llu/%llu copro%llu/%llu\n",body,value,flags,early,(unsigned long long)actual.cycles,(unsigned long long)cycles,(unsigned long long)actual.master_cycles,(unsigned long long)(initial.master_cycles+cycles*6+bus_extra),(unsigned long long)actual.coprocessor_master_cycles,(unsigned long long)(ops?initial.master_cycles+last:0));assert(0);
  }
  if(early)deadlines++;else complete++;interp816_free(i);count++;
 }
 /* Incoming Z=1 really spins at9800/9803. Compare bounded native deadline
  * unwind to the original opcodes at the same completed instruction clock. */
 for(unsigned hi=0;hi<3;hi++)for(unsigned stack=0;stack<2;stack++)for(unsigned flags=0;flags<16;flags++){
  for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*31+count);
  memset(io,0,sizeof io);CpuState initial={0};initial.A=highs[hi]|(flags*17);
  initial.X=0x1234;initial.Y=0xfedc;initial.S=stacks[stack];initial.DB=1;
  initial.P=0x26|((flags&1)?1:0)|((flags&2)?0x40:0)|((flags&4)?0x80:0)|((flags&8)?8:0);
  initial.ram=ram;initial.host_return_valid=3;cpu_p_to_mirrors(&initial);
  ram[initial.S+1]=0x34;ram[initial.S+2]=0x12;ram[initial.S+3]=0;
  memcpy(before,ram,sizeof ram);CpuState actual=initial;pending=0;boundary=0;inherited_pending=0;deadline=1000;
  assert(rng_body(0,&actual)==RECOMP_RETURN_NORMAL);assert(pending&&(boundary==0x009800||boundary==0x009803));
  assert(!g_recomp_stack_top);cpu_mirrors_to_p(&actual);memcpy(expected,ram,sizeof ram);memcpy(expected_io,io,sizeof io);
  memcpy(ram,before,sizeof ram);memset(io,0,sizeof io);Interp816*i=original(&initial,0x009800);
  unsigned ops=0;uint64_t cycles=0,last=0;bus_extra=0;
  while(cycles<actual.cycles){assert(++ops<1000);last=cycles*6+bus_extra;cycles+=interp816_runOpcode(i);}
  assert(cycles==actual.cycles&&(((unsigned)i->k<<16)|i->pc)==boundary);
  assert(!memcmp(ram,expected,sizeof ram)&&!memcmp(io,expected_io,sizeof io));
  assert(i->a==actual.A&&i->x==actual.X&&i->y==actual.Y&&i->sp==actual.S&&i->dp==actual.D&&i->db==actual.DB&&i->k==actual.PB&&interp816_getFlags(i)==actual.P);
  assert(actual.master_cycles==cycles*6+bus_extra&&actual.coprocessor_master_cycles==last);
  interp816_free(i);spins++;count++;
 }
 assert(complete&&deadlines&&spins);
 printf("PASS %u complete native RNG/seed original-opcode comparisons; complete=%u deadline-before-entry=%u zero-flag-loop-deadline=%u; all256divisors/all256seed-stepinputs/16RNGseeds/16flagcombinations/3Ahigh/2stacks; full128KiBRAM/allmodeledIO/registers/flags/saved+deadstack/CPU+master+copro clocks; constant8cycle/immediateunitdivider, runtimehardwaretimingseparate\n",count,complete,deadlines,spins);
 return 0;
}
