/* Actual extracted generated code versus the original opcode interpreter.
 * Only host scheduling / boundary services are stubbed, never arithmetic. */
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
static uint64_t bus_extra;
uint8_t g_memsel;
int g_recomp_stack_top;
uint16_t g_cpu_entry_s[16];
const char *g_last_recomp_func;
void RecompStackPush(const char *name) {(void)name;g_recomp_stack_top++;}
void RecompStackPop(void) {assert(g_recomp_stack_top>0);g_recomp_stack_top--;}
void RecompStackPopYield(void) {assert(0);}
void WatchdogCheck(void) {}
void cpu_dbg_funcname(const char *name) {(void)name;}
int cpu_take_tailcall_return_context(uint16 *s,uint8 *h) {(void)s;(void)h;return 0;}
int interp_bridge_lle_master_deadline_reached(const CpuState *c) {(void)c;return 0;}
RecompReturn interp_bridge_lle_yield_unwind(CpuState *c,uint32 pc) {(void)c;(void)pc;assert(0);return RECOMP_RETURN_NORMAL;}
void cpu_instruction_timing_commit(CpuState *c) {(void)c;}
int cpu_dispatch_has_entry(CpuState *c,uint32 pc) {(void)c;(void)pc;assert(0);return 0;}
int interp_bridge_has_direct_paired_bounce(void) {assert(0);return 0;}
int cpu_resolve_ancestor_skip(uint16 s) {(void)s;assert(0);return -1;}
int interp_bridge_return_targets_owner(uint16 s,uint16 post) {(void)s;(void)post;assert(0);return 0;}
RecompReturn interp_tier_dispatch_rewritten_return(CpuState *c,uint32 pc,uint32 site) {(void)c;(void)pc;(void)site;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn interp_tier_dispatch_popped_return(CpuState *c,uint32 pc,uint32 site,uint16 s) {(void)c;(void)pc;(void)site;(void)s;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_dispatch_pc_from(CpuState *c,uint32 pc,uint16 s,uint32 site) {(void)c;(void)pc;(void)site;(void)s;assert(0);return RECOMP_RETURN_NORMAL;}
uint32_t cpu_region_speed(uint32_t a) {(void)a;return 8;}
uint8 cpu_read8(CpuState *c,uint8 bank,uint16 addr) {
 int32_t off=cpu_wram_offset(bank,addr);assert(off>=0);return c->open_bus=ram[off];
}
void cpu_write8(CpuState *c,uint8 bank,uint16 addr,uint8 value) {
 int32_t off=cpu_wram_offset(bank,addr);assert(off>=0);ram[off]=value;c->open_bus=value;
}
uint16 cpu_read16(CpuState *c,uint8 b,uint16 a) {unsigned lo=cpu_read8(c,b,a);return lo|((unsigned)cpu_read8(c,b,a+1)<<8);}
void cpu_write16(CpuState *c,uint8 b,uint16 a,uint16 v) {cpu_write8(c,b,a,v);cpu_write8(c,b,a+1,v>>8);}
RecompReturn interp_tier_dispatch_tail(CpuState *c,uint32 pc,uint32 site,uint16 s,uint8 h) {
 (void)c;(void)site;assert(pc==0x01b43e||pc==0x01b7d4);assert(s==c->S&&h==2);boundary=pc;return RECOMP_RETURN_NORMAL;
}
#include "death_animation_under_test.h"
static uint8 rd(void *ctx,uint32 a) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);if(off>=0)return ram[off];
 assert((a>>16)==1&&(a&65535)>=32768);return rom[0x8000+(a&32767)];
}
static void wr(void *ctx,uint32 a,uint8 v) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);assert(off>=0);ram[off]=v;
}
int main(int argc,char **argv) {
 assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 unsigned count=0,exits[3]={0};
 const unsigned kinds[]={0,0x24,0x25,0xff};
 const unsigned timers[]={0,1,2,254,255};
 const unsigned frames[]={0,6,7,8,254,255};
 for(unsigned slot=0;slot<3;slot++)for(unsigned k=0;k<4;k++)for(unsigned t=0;t<5;t++)for(unsigned q=0;q<6;q++)for(unsigned flags=0;flags<32;flags++) {
  for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*73+count*17);
  CpuState initial={0};initial.A=(uint16)(count*1237);initial.X=slot?slot*0x100:0x40;initial.Y=count*29;initial.S=0x1d0;initial.DB=initial.PB=1;initial.D=0;initial.P=0x24|((flags&1)?1:0)|((flags&2)?2:0)|((flags&4)?0x40:0)|((flags&8)?0x80:0)|((flags&16)?8:0);initial.ram=ram;initial.host_return_valid=2;cpu_p_to_mirrors(&initial);
  ram[initial.S+1]=0x34;ram[initial.S+2]=0x12;
  ram[0x51b+initial.X]=kinds[k];ram[0x510+initial.X]=timers[t];ram[0x511+initial.X]=frames[q];
  memcpy(before,ram,sizeof ram);CpuState actual=initial;boundary=0;
  assert(Lagoon_ActorLleBoundary_b413_M1X0(&actual)==RECOMP_RETURN_NORMAL);assert(!g_recomp_stack_top);cpu_mirrors_to_p(&actual);
  memcpy(expected,ram,sizeof ram);memcpy(ram,before,sizeof ram);
  Interp816 *i=interp816_init(NULL,rd,wr);assert(i);i->a=initial.A;i->x=initial.X;i->y=initial.Y;i->sp=initial.S;i->dp=initial.D;i->db=i->k=1;i->pc=0xb413;i->e=0;interp816_setFlags(i,initial.P);
  unsigned ops=0;uint64_t cycles=0,last_master=0;bus_extra=0;
  while(i->pc!=0xb43e&&i->pc!=0xb7d4&&i->pc!=0x1235) {assert(++ops<=18);last_master=cycles*6+bus_extra;cycles+=interp816_runOpcode(i);}
  assert(!memcmp(ram,expected,sizeof ram));
  assert(i->a==actual.A&&i->x==actual.X&&i->y==actual.Y&&i->sp==actual.S&&i->dp==actual.D&&i->db==actual.DB&&i->k==actual.PB&&interp816_getFlags(i)==actual.P);
  assert(actual.cycles==cycles);assert(actual.master_cycles==cycles*6+bus_extra);assert(actual.coprocessor_master_cycles==last_master);
  if(boundary){assert(i->pc==(boundary&65535));exits[boundary==0x01b43e?0:1]++;}else{assert(i->pc==0x1235);exits[2]++;}
  interp816_free(i);count++;
 }
 assert(exits[0]&&exits[1]&&exits[2]);printf("PASS %u exact generated/original-opcode full128KiB RAM, CPU, flags, deadstack, CPU/master clocks; special=%u rewards=%u RTS=%u\n",count,exits[0],exits[1],exits[2]);return 0;
}
