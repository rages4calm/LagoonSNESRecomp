/* Actual remaining boss motion bodies versus original 65816 instructions.
 * Original RNG source runs with a bounded deterministic divider unit bus.
 * First landing/unknown targets stop at genuine entries; no fake results. */
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
static unsigned boundary,service_calls;
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
 io[a]=v;if(a==0x4206){unsigned n=io[0x4204]|((unsigned)io[0x4205]<<8);unsigned q=v?n/v:65535;
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
 assert((a>>16)<0x40&&(a&65535)>=0x4200&&(a&65535)<0x4300);a&=65535;io[a]=v;
 /* Constant-cycle fixture: quotient becomes available immediately. Original
  * RNG performs its full9848 source before reading4214; this is arithmetic
  * bus modeling, not qualification of real divider latency or IRQ handling. */
 if(a==0x4206){unsigned n=io[0x4204]|((unsigned)io[0x4205]<<8);unsigned q=v?n/v:65535;
  io[0x4214]=q;io[0x4215]=q>>8;}
}
static Interp816* original(const CpuState*c,unsigned pc){
 Interp816*i=interp816_init(NULL,rd,wr);assert(i);i->a=c->A;i->x=c->X;i->y=c->Y;
 i->sp=c->S;i->dp=c->D;i->db=c->DB;i->k=pc>>16;i->pc=pc;i->e=c->emulation;
 interp816_setFlags(i,c->P);return i;
}
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 pc,uint32 site,uint8 n,uint32*out){
 if(pc!=0x009800){pending=1;boundary=pc;return (RecompReturn)1;}
 assert((site==0x0294ba||site==0x0294c9)&&n==3);
 unsigned ret=site+3;assert(ram[c->S+1]==(ret&255)&&ram[c->S+2]==((ret>>8)&255)&&ram[c->S+3]==2);
 cpu_mirrors_to_p(c);Interp816*i=original(c,pc);uint64_t cycles=0,last=0;unsigned ops=0;bus_extra=0;
 while((((unsigned)i->k<<16)|i->pc)!=site+4){assert(++ops<512);last=cycles*6+bus_extra;cycles+=interp816_runOpcode(i);}
 c->A=i->a;c->X=i->x;c->Y=i->y;c->S=i->sp;c->D=i->dp;c->DB=i->db;c->PB=i->k;
 c->P=interp816_getFlags(i);cpu_p_to_mirrors(c);
 c->coprocessor_master_cycles=c->master_cycles+last;c->master_cycles+=cycles*6+bus_extra;c->cycles+=cycles;
 if(out)*out=site+4;interp816_free(i);service_calls++;return RECOMP_RETURN_NORMAL;
}
#if MOTION_NATIVE_RNG
static void motion_trace_entry(unsigned pc){if(pc==0x009800)service_calls++;}
#define cpu_trace_func_entry(cpu,pc,name) motion_trace_entry(pc)
#endif
#include "boss_motion_under_test.h"
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 unsigned count=0,complete=0,children=0,deadlines=0,unknown=0;
 const unsigned edge[]={0,1,6,7,8,15,16,0x26,0x27,0x28,0x29,0x3f,0x40,0x44,0x45,0x7f,0x80,0xfe,0xff};
 const unsigned coord[]={0,1,0x2f,0x30,0x31,0x73,0x74,0x75,0x7f,0x80,0x81,0x8b,0x8c,0x8d,0xcf,0xd0,0xd1,0xdf,0xe0,0xe1,0xff,0x100,0x7fff,0x8000,0xffff};
 const unsigned highs[]={0,0x100,0xff00},stacks[]={0x180,0x1fd0};
 for(unsigned body=0;body<16;body++)for(unsigned stack=0;stack<2;stack++)
 for(unsigned hi=0;hi<3;hi++)for(unsigned flags=0;flags<16;flags++)
 for(unsigned scenario=0;scenario<152;scenario++)for(unsigned early=0;early<2;early++){
  for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*73+count*17);
  memset(io,0,sizeof io);CpuState initial={0};initial.A=highs[hi]|(uint8)(count*73);initial.X=(scenario%8+1)*0x40;
  initial.Y=count*29;initial.S=stacks[stack];initial.DB=initial.PB=2;
  initial.P=0x24|((flags&1)?1:0)|((flags&2)?0x40:0)|((flags&4)?0x80:0)|((flags&8)?8:0);
  initial.ram=ram;initial.host_return_valid=2;cpu_p_to_mirrors(&initial);
  unsigned value=edge[scenario%19],phase=scenario/19;
  ram[initial.S+1]=0x34;ram[initial.S+2]=0x12;
  ram[0x541]=(phase&1)?1:0;ram[0x542]=value;
  ram[0x390]=(scenario&3)?value:0;ram[0x391]=(scenario&4)?value:0;
  ram[0x392]=(scenario&8)?value:0;ram[0x393]=value;ram[0x398]=value;ram[0x399]=(scenario&3)?value:0;
  ram[0x3af]=(phase==6)?6:phase;ram[0x3b5]=value;
  ram[0x500+initial.X]=0xc0|(phase&6);ram[0x501+initial.X]=phase;
  ram[0x51b+initial.X]=scenario%18==17?255:scenario%18;
  ram[0x50a+initial.X]=value;ram[0x51f+initial.X]=0xff;
  ram[0x520+initial.X]=value;ram[0x521+initial.X]=(phase&1)?255:0;
  ram[0x52c+initial.X]=(phase&1)?value:0;ram[0x534+initial.X]=value;
  ram[0x53e + initial.X]=value;ram[0x53f+initial.X]=phase<<2;
  unsigned x=coord[scenario%25],y=coord[(scenario*7)%25],vx=coord[(scenario*11)%25],vy=coord[(scenario*13)%25];
  ram[0x502+initial.X]=x;ram[0x503+initial.X]=x>>8;ram[0x504+initial.X]=y;ram[0x505+initial.X]=y>>8;
  ram[0x506+initial.X]=vx;ram[0x507+initial.X]=vx>>8;ram[0x508+initial.X]=vy;ram[0x509+initial.X]=vy>>8;
  unsigned px=coord[(scenario*17)%25];ram[0x502]=px;ram[0x503]=px>>8;
  memcpy(before,ram,sizeof ram);CpuState actual=initial;boundary=0;pending=0;
  inherited_s=initial.S;inherited_pending=1;deadline=early?1:0;
  if(early){actual.master_cycles=1;initial.master_cycles=1;}
  RecompReturn result=motion_body(body,&actual);
  assert(result==RECOMP_RETURN_NORMAL||(result==1&&pending&&boundary));assert(!g_recomp_stack_top);cpu_mirrors_to_p(&actual);
  unsigned stop=boundary?boundary:0x021235;
  memcpy(expected,ram,sizeof ram);memcpy(expected_io,io,sizeof io);
  memcpy(ram,before,sizeof ram);memset(io,0,sizeof io);
  Interp816*i=original(&initial,motion_starts[body]);unsigned ops=0;uint64_t cycles=0,last=0;bus_extra=0;
  while((((unsigned)i->k<<16)|i->pc)!=stop){assert(++ops<1200);last=cycles*6+bus_extra;cycles+=interp816_runOpcode(i);}
  if(memcmp(ram,expected,sizeof ram)||memcmp(io,expected_io,sizeof io)||i->a!=actual.A||i->x!=actual.X||i->y!=actual.Y||i->sp!=actual.S||
     i->dp!=actual.D||i->db!=actual.DB||i->k!=actual.PB||interp816_getFlags(i)!=actual.P){
   fprintf(stderr,"state body%06x scenario%u hi%04x flags%u early%u stop%06x A%04x/%04x X%04x/%04x Y%04x/%04x S%04x/%04x P%02x/%02x\n",motion_starts[body],scenario,highs[hi],flags,early,stop,i->a,actual.A,i->x,actual.X,i->y,actual.Y,i->sp,actual.S,interp816_getFlags(i),actual.P);assert(0);
  }
  if(actual.cycles!=cycles||actual.master_cycles!=initial.master_cycles+cycles*6+bus_extra||
     actual.coprocessor_master_cycles!=(ops?initial.master_cycles+last:0)){
   fprintf(stderr,"timing body%06x scenario%u cpu%llu/%llu master%llu/%llu copro%llu/%llu\n",motion_starts[body],scenario,(unsigned long long)actual.cycles,(unsigned long long)cycles,(unsigned long long)actual.master_cycles,(unsigned long long)(initial.master_cycles+cycles*6+bus_extra),(unsigned long long)actual.coprocessor_master_cycles,(unsigned long long)(ops?initial.master_cycles+last:0));assert(0);
  }
  if(early)deadlines++;else if(boundary){children++;if(boundary!=0x02906a)unknown++;}else complete++;
  interp816_free(i);count++;
 }
 assert(complete&&children&&deadlines&&unknown&&service_calls);
 printf("PASS %u actual generated15 boss-motion targets+selector/original-opcode cases; complete=%u realchild-boundary=%u unknown-pointer=%u deadline-before-entry=%u actual009800RNGservices=%u; full128KiB RAM/registers/flags/Ahigh/saved+deadstack/CPU+master+coprocessor clocks; deterministic divider unitbus\n",count,complete,children,unknown,deadlines,service_calls);
 return 0;
}
