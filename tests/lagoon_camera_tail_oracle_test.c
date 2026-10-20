/* Actual original centered-camera opcodes, actual SDK/title owner matcher,
 * then actual generated actor/boss dispatcher versus original opcodes. */
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
static uint8 rom[0x100000], ram[0x20000], before[0x20000], expected[0x20000];
static unsigned boundary;
static uint16_t inherited_s;
static unsigned inherited_pending;
static int pending;
static uint64_t generation=1;
const uint8_t *g_rom;
uint64_t RtlStateGeneration(void){return generation;}
static uint64_t deadline;
static uint64_t bus_extra;
uint8_t g_memsel;
int g_recomp_stack_top;
uint16_t g_cpu_entry_s[16];
const char *g_last_recomp_func;
void RecompStackPush(const char *name) {(void)name;g_recomp_stack_top++;}
void RecompStackPop(void) {assert(g_recomp_stack_top>0);g_recomp_stack_top--;}
void RecompStackPopYield(void) {assert(g_recomp_stack_top>0);g_recomp_stack_top--;}
void WatchdogCheck(void) {}
void cpu_dbg_funcname(const char *name) {(void)name;}
int cpu_take_tailcall_return_context(uint16 *s,uint8 *h) {if(!inherited_pending)return 0;inherited_pending=0;*s=inherited_s;*h=2;return 1;}
void cpu_tailcall_inherit_return_context(uint16 s,uint8 h){assert(h==2);inherited_s=s;inherited_pending=1;}
int interp_bridge_lle_master_deadline_reached(const CpuState *c) {(void)c;return deadline && c->master_cycles>=deadline;}
int interp_bridge_lle_unwind_pending(void) {return pending;}
RecompReturn interp_bridge_lle_yield_unwind(CpuState *c,uint32 pc) {(void)c;boundary=pc;pending=1;return RECOMP_RETURN_NORMAL;}
void cpu_instruction_timing_commit(CpuState *c) {(void)c;}
int cpu_dispatch_has_entry(CpuState *c,uint32 pc) {(void)c;(void)pc;assert(0);return 0;}
int interp_bridge_has_direct_paired_bounce(void) {assert(0);return 0;}
int cpu_resolve_ancestor_skip(uint16 s) {(void)s;assert(0);return -1;}
int interp_bridge_return_targets_owner(uint16 s,uint16 post) {(void)s;(void)post;assert(0);return 0;}
RecompReturn interp_tier_dispatch_rewritten_return(CpuState *c,uint32 pc,uint32 site) {(void)c;(void)pc;(void)site;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn interp_tier_dispatch_popped_return(CpuState *c,uint32 pc,uint32 site,uint16 s) {(void)c;(void)pc;(void)site;(void)s;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_dispatch_pc_from(CpuState *c,uint32 pc,uint16 s,uint32 site) {(void)c;(void)pc;(void)site;(void)s;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_unresolved_abandon_balanced(CpuState*c,uint32 site,uint16 s,uint8 h){(void)c;(void)site;(void)s;(void)h;assert(0);return RECOMP_RETURN_NORMAL;}
uint32_t cpu_region_speed(uint32_t a) {(void)a;return 8;}
uint8 cpu_read8(CpuState *c,uint8 bank,uint16 addr) {
 int32_t off=cpu_wram_offset(bank,addr);
 if(off>=0)return c->open_bus=ram[off];
 assert(bank<32&&addr>=0x8000);return c->open_bus=rom[bank*0x8000+(addr&0x7fff)];
}
void cpu_write8(CpuState *c,uint8 bank,uint16 addr,uint8 value) {
 int32_t off=cpu_wram_offset(bank,addr);assert(off>=0);ram[off]=value;c->open_bus=value;
}
uint16 cpu_read16(CpuState *c,uint8 b,uint16 a) {unsigned lo=cpu_read8(c,b,a);return lo|((unsigned)cpu_read8(c,b,a+1)<<8);}
void cpu_write16(CpuState *c,uint8 b,uint16 a,uint16 v) {cpu_write8(c,b,a,v);cpu_write8(c,b,a+1,v>>8);}
RecompReturn interp_tier_dispatch_tail(CpuState *c,uint32 pc,uint32 site,uint16 s,uint8 h) {
 (void)c;(void)site;assert((pc>>16)==1);assert(h==2);(void)s;boundary=pc;return RECOMP_RETURN_NORMAL;
}
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 pc,uint32 site,uint8 n,uint32*out) {(void)c;(void)site;(void)n;(void)out;pending=1;boundary=pc;return (RecompReturn)1;}
#include "../src/lagoon_native_camera.c"
#include "camera_policy_under_test.h"
#include "camera_actor_under_test.h"
static int observer(unsigned e,const CpuState*c,uint32_t source,uint32_t target,
    const InterpAotContinuation*d,uint16_t owner,InterpAotAncestor*out) {
 (void)out;LagoonNativeCameraObserve(e,c,source,target,d,owner);return 0;
}
static uint8 rd(void*ctx,uint32 a) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);if(off>=0)return ram[off];
 assert((a>>16)<32&&(a&65535)>=32768);return rom[(a>>16)*0x8000+(a&32767)];
}
static void wr(void*ctx,uint32 a,uint8 v) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);assert(off>=0);ram[off]=v;
}
static void sync_cpu(CpuState*c,Interp816*i,uint64_t cycles) {
 c->A=i->a;c->X=i->x;c->Y=i->y;c->S=i->sp;c->D=i->dp;
 c->DB=i->db;c->PB=i->k;c->emulation=i->e;c->P=interp816_getFlags(i);cpu_p_to_mirrors(c);
 c->cycles=cycles;c->master_cycles=cycles*6+bus_extra;c->coprocessor_master_cycles=c->master_cycles;
}
static Interp816 *make_interp(const CpuState*c) {
 Interp816*i=interp816_init(NULL,rd,wr);assert(i);i->a=c->A;i->x=c->X;i->y=c->Y;i->sp=c->S;
 i->dp=c->D;i->db=c->DB;i->k=c->PB;i->pc=0x80CA;i->e=0;interp816_setFlags(i,c->P);return i;
}
static void word(unsigned address,unsigned value){ram[address]=value;ram[address+1]=value>>8;}
int main(int argc,char**argv) {
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);g_rom=rom;
 const uint8_t center[]={0x5A,0x9B,0xD0,0x16,0xDA,0x64,0x41,
  0x20,0xFB,0x83,0x20,0x26,0x84,0x20,0x52,0x84,0x20,0x7D,0x84,
  0xFA,0xAD,0x19,0x05,0x8D,0x19,0x05,0x7A,0x4C,0xB5,0xA7,0xEA,0xEA,0xEA,0xEA,0xEA};
 memset(rom+0x809D,0xEA,3);rom[0x80CB]=0xD8;rom[0x80CC]=0x83;memcpy(rom+0x83D8,center,sizeof(center));
 rom[0x8408]=rom[0x8432]=128;rom[0x845F]=rom[0x8489]=112;rom[0x87F6]=128;
 assert(camera_source());interp_bridge_set_aot_continuation_hook(observer);LagoonNativeCameraBeforeFrame();
 const unsigned high[]={0,0x100,0xFF00},stacks[]={0x180,0x1FD0};
 const unsigned kinds[]={0,2,3,6,10,14,15},timers[]={0,1,2,3,4,5,6,7,8,10,11,12,15,16,31,63,127,254,255};
 unsigned count=0,complete=0,children=0,yields=0;
 for(unsigned kind=0;kind<7;kind++)for(unsigned stack=0;stack<2;stack++)
 for(unsigned hi=0;hi<3;hi++)for(unsigned flags=0;flags<16;flags++)for(unsigned scenario=0;scenario<152;scenario++) {
  for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8_t)(n*73+count*17);
  CpuState initial={0};initial.ram=ram;initial.PB=initial.DB=1;initial.S=stacks[stack];initial.A=high[hi]|(count*73&255);
  initial.X=kind?(scenario%8+1)*0x40:0;initial.Y=count*29;
  initial.P=0x24|((flags&1)?1:0)|((flags&2)?0x40:0)|((flags&4)?0x80:0)|((flags&8)?2:0);cpu_p_to_mirrors(&initial);
  unsigned timer=timers[scenario%19],phase=scenario/19;
  ram[0x50]=0;ram[0x51]=scenario&1;word(0x480,2);word(0x482,2);word(0x488,1024);word(0x48A,1024);ram[0x496]=0;
  word(0x502,130+(int)(scenario%3)-1);word(0x504,114+(int)(scenario%3)-1);ram[0x500]=0x40;ram[0x501]=0;
  ram[0x550]=timer;ram[0x551]=phase;ram[0x56C]=phase&3;ram[0x541]=scenario&1?4:0;
  ram[0x54A]=scenario&3;ram[0x54B]=scenario&2?0x80:0;ram[0x50E]=scenario&4?0x80:0;
  if(kind){ram[0x518+initial.X]=3;ram[0x51B+initial.X]=kinds[kind];ram[0x501+initial.X]=scenario&1?4:0;
   ram[0x51F+initial.X]=scenario&2?0x10:0;ram[0x510+initial.X]=timer;ram[0x511+initial.X]=phase;ram[0x51D+initial.X]=phase;
  }else{ram[0x518]=0;}
  memcpy(before,ram,sizeof ram);CpuState actual=initial;continuation_discard_tokens();inherited_pending=0;boundary=0;pending=0;deadline=0;
  Interp816*i=make_interp(&initial);unsigned ops=0;uint64_t cycles=0;bus_extra=0;
  while((((unsigned)i->k<<16)|i->pc)!=0x01A7B5) {
   unsigned pc=((unsigned)i->k<<16)|i->pc;assert(++ops<=1024);cycles+=interp816_runOpcode(i);
   if(pc==0x0180CA){sync_cpu(&actual,i,cycles);assert(!continuation_arm(&actual,0x0183D8,pc,2,1));assert(camera.phase==1);}
  }
  sync_cpu(&actual,i,cycles);interp816_free(i);InterpAotAncestor owner={0};
  assert(continuation_tail(&actual,0x0183F3,0x01A7B5,&owner));assert(owner.owner_return_pc24==0x0180CD&&owner.entry_s==actual.S&&owner.owner_frame==2);
  continuation_event(INTERP_CONT_TAKE,&actual,0x0183F3,0x01A7B5,NULL,owner.entry_s);
  actual.host_return_valid=2;deadline=(flags&1)?actual.master_cycles:0;
  RecompReturn result=camera_body(&actual);assert(result==0||(pending&&boundary));assert(!g_recomp_stack_top);cpu_mirrors_to_p(&actual);
  unsigned stop=boundary?boundary:0x0180CD;memcpy(expected,ram,sizeof ram);memcpy(ram,before,sizeof ram);
  i=make_interp(&initial);cycles=0;bus_extra=0;ops=0;uint64_t last_master=0;unsigned native_ops=0;
  while((((unsigned)i->k<<16)|i->pc)!=0x01A7B5){assert(++ops<=1024);cycles+=interp816_runOpcode(i);}
  last_master=cycles*6+bus_extra;
  while((((unsigned)i->k<<16)|i->pc)!=stop){assert(++native_ops<=512);last_master=cycles*6+bus_extra;cycles+=interp816_runOpcode(i);}
  if(memcmp(ram,expected,sizeof ram)||i->a!=actual.A||i->x!=actual.X||i->y!=actual.Y||i->sp!=actual.S||i->dp!=actual.D||i->db!=actual.DB||i->k!=actual.PB||interp816_getFlags(i)!=actual.P) {
   fprintf(stderr,"state kind%u scenario%u flags%u high%u stop%06x S%04x/%04x A%04x/%04x P%02x/%02x\n",kind,scenario,flags,hi,stop,i->sp,actual.S,i->a,actual.A,interp816_getFlags(i),actual.P);assert(0);
  }
  if(actual.cycles!=cycles||actual.master_cycles!=cycles*6+bus_extra||actual.coprocessor_master_cycles!=last_master) {
   fprintf(stderr,"clock kind%u scenario%u flags%u high%u stop%06x cpu%llu/%llu master%llu/%llu copro%llu/%llu\n",kind,scenario,flags,hi,stop,(unsigned long long)actual.cycles,(unsigned long long)cycles,(unsigned long long)actual.master_cycles,(unsigned long long)(cycles*6+bus_extra),(unsigned long long)actual.coprocessor_master_cycles,(unsigned long long)last_master);assert(0);
  }
  if(deadline){assert(stop==0x01A7B5&&!native_ops);yields++;}else if(boundary)children++;else complete++;
  interp816_free(i);count++;
 }
 assert(count==102144&&children&&complete&&yields);
 printf("PASS %u original-camera/actual-SDK-owner/actual-native-dispatch opcode cases; full128KiB RAM/registers/flags/realCC80frame+deadstack/CPU-master-coprocessor clocks; complete=%u originalchild-boundary=%u deadline-entry-yields=%u; actualcamera-handoffs=%llu\n",count,complete,children,yields,stats.takes);
 return 0;
}
