/* Complete finite projectile animation leaves; original opcodes and actual inherited RTS frames. */
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
static unsigned fallback_count;



static uint8 inherited_hrv;
static uint8 io[65536],expected_io[65536];
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
int cpu_take_tailcall_return_context(uint16 *s,uint8 *h) {if(!inherited_pending)return 0;inherited_pending=0;*s=inherited_s;*h=inherited_hrv;return 1;}
void cpu_tailcall_inherit_return_context(uint16 s,uint8 h){inherited_hrv=h;inherited_s=s;inherited_pending=1;}
int interp_bridge_lle_master_deadline_reached(const CpuState *c) {(void)c;return deadline && c->master_cycles>=deadline;}
int interp_bridge_lle_unwind_pending(void) {return pending;}
RecompReturn interp_bridge_lle_yield_unwind(CpuState *c,uint32 pc) {(void)c;boundary=pc;pending=1;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;}
void cpu_instruction_timing_commit(CpuState *c) {(void)c;}
int cpu_dispatch_has_entry(CpuState *c,uint32 pc) {(void)c;(void)pc;assert(0);return 0;}
int interp_bridge_has_direct_paired_bounce(void) {assert(0);return 0;}
int cpu_resolve_ancestor_skip(uint16 s) {(void)s;assert(0);return -1;}
int interp_bridge_return_targets_owner(uint16 s,uint16 post) {(void)s;(void)post;assert(0);return 0;}
RecompReturn interp_tier_dispatch_rewritten_return(CpuState *c,uint32 pc,uint32 site) {(void)c;(void)pc;(void)site;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn interp_tier_dispatch_popped_return(CpuState *c,uint32 pc,uint32 site,uint16 s) {(void)c;(void)pc;(void)site;(void)s;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_dispatch_pc_from(CpuState *c,uint32 pc,uint16 s,uint32 site) {(void)c;(void)pc;(void)site;(void)s;assert(0);return RECOMP_RETURN_NORMAL;}
RecompReturn cpu_unresolved_abandon_balanced(CpuState*c,uint32 site,uint16 s,uint8 h){(void)site;(void)s;(void)h;assert(0);return RECOMP_RETURN_NORMAL;}
uint32_t cpu_region_speed(uint32_t a) {(void)a;return 8;}
uint8 cpu_read8(CpuState *c,uint8 bank,uint16 addr) {
 int32_t off=cpu_wram_offset(bank,addr);
 if(off>=0){return c->open_bus=ram[off];}
 if(bank<0x40&&addr>=0x2100&&addr<0x4400)return c->open_bus=io[addr];
 if(addr>=0x8000)return c->open_bus=rom[(bank&31)*0x8000+(addr&0x7fff)];return c->open_bus=0;
}
void cpu_write8(CpuState *c,uint8 bank,uint16 addr,uint8 value) {
 int32_t off=cpu_wram_offset(bank,addr);c->open_bus=value;
 if(off>=0){ram[off]=value;return;}
 if(!(bank<0x40&&addr>=0x2100&&addr<0x4400))return;io[addr]=value;
 if(addr==0x4203){unsigned product=io[0x4202]*value;io[0x4216]=product;io[0x4217]=product>>8;}
}
uint16 cpu_read16(CpuState *c,uint8 b,uint16 a) {unsigned lo=cpu_read8(c,b,a);return lo|((unsigned)cpu_read8(c,b,a+1)<<8);}
void cpu_write16(CpuState *c,uint8 b,uint16 a,uint16 v) {cpu_write8(c,b,a,v);cpu_write8(c,b,a+1,v>>8);}
#include "animation_handlers_services_under_test.h"
RecompReturn interp_tier_dispatch_tail(CpuState *c,uint32 pc,uint32 site,uint16 s,uint8 h) {
 assert(s==c->S&&h==2);
 if(pc==0x01BCB2){cpu_tailcall_inherit_return_context(s,h);return animation_handlers_body(pc,c);}
 assert(site==0x01BB5C);fallback_count++;boundary=pc;pending=1;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;
}
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 pc,uint32 site,uint8 n,uint32*out) {(void)c;(void)pc;(void)site;(void)n;(void)out;assert(0);return RECOMP_RETURN_NORMAL;}
static uint8 rd(void *ctx,uint32 a) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);if(off>=0){return ram[off];}
 if((a>>16)<0x40&&(a&65535)>=0x2100&&(a&65535)<0x4400)return io[a&65535];
 if((a&65535)>=32768)return rom[((a>>16)&31)*0x8000+(a&32767)];return 0;
}
static void wr(void *ctx,uint32 a,uint8 v) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);
 if(off>=0){ram[off]=v;return;}
 if(!((a>>16)<0x40&&(a&65535)>=0x2100&&(a&65535)<0x4400))return;io[a&65535]=v;
 if((a&65535)==0x4203){unsigned product=io[0x4202]*v;io[0x4216]=product;io[0x4217]=product>>8;}
}




static void compare(unsigned slot,unsigned frame,unsigned hi,unsigned flags,unsigned stack,unsigned phase,unsigned inherited,unsigned dispatch,unsigned*counts){
 unsigned pc=dispatch?0x01BB4B:animation_table_targets[slot];unsigned prior_fallback=fallback_count;
 for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*31+frame*17+flags);
 memset(io,0,sizeof io);CpuState initial={0};initial.A=hi|0x81;
 initial.X=(frame%16)*0x18;initial.Y=frame*29;initial.S=stack;initial.DB=1;initial.PB=1;initial.ram=ram;initial.host_return_valid=2;
 initial.P=CPU_P_M|CPU_P_I|((flags&1)?CPU_P_C:0)|((flags&2)?CPU_P_V:0)|((flags&4)?CPU_P_N:0)|((flags&8)?CPU_P_Z:0);
 cpu_p_to_mirrors(&initial);ram[stack+1]=0x34;ram[stack+2]=0x12;
 ram[0xd40+initial.X]=slot;ram[0xd50+initial.X]=(frame>>4)&3;ram[0xd52+initial.X]=frame;
 initial.master_cycles=phase==1?1:0;initial.coprocessor_master_cycles=initial.master_cycles;
 deadline=phase==1?1:phase==2?48:phase==3?112:0;pending=0;boundary=0;inherited_pending=inherited;inherited_s=stack;inherited_hrv=2;
 memcpy(before,ram,sizeof ram);CpuState actual=initial;
 RecompReturn result=animation_handlers_body(pc,&actual);assert(!g_recomp_stack_top);cpu_mirrors_to_p(&actual);assert(result==RECOMP_RETURN_NORMAL||(pending&&boundary));
 memcpy(expected,ram,sizeof ram);memcpy(expected_io,io,sizeof io);memcpy(ram,before,sizeof ram);memset(io,0,sizeof io);
 Interp816*i=interp816_init(NULL,rd,wr);assert(i);i->a=initial.A;i->x=initial.X;i->y=initial.Y;i->sp=initial.S;i->dp=0;i->db=initial.DB;i->k=1;i->pc=pc;i->e=0;interp816_setFlags(i,initial.P);
 unsigned stop=boundary?boundary:0x011235,ops=0;uint64_t cycles=0,last=initial.master_cycles;bus_extra=0;
 while(cycles<actual.cycles){assert(++ops<128);last=initial.master_cycles+cycles*6+bus_extra;cycles+=interp816_runOpcode(i);}
 unsigned first=0;while(first<sizeof ram&&ram[first]==expected[first])first++;
 if(first<sizeof ram||memcmp(io,expected_io,sizeof io)||i->a!=actual.A||i->x!=actual.X||i->y!=actual.Y||i->sp!=actual.S||i->dp!=actual.D||i->db!=actual.DB||i->k!=actual.PB||interp816_getFlags(i)!=actual.P||(((unsigned)i->k<<16)|i->pc)!=stop){
  fprintf(stderr,"STATE %06x slot%u frame%u hi%04x flags%u S%04x phase%u RAM%05x A%04x/%04x X%04x/%04x Y%04x/%04x S%04x/%04x P%02x/%02x PC%06x/%06x\n",pc,slot,frame,hi,flags,stack,phase,first,i->a,actual.A,i->x,actual.X,i->y,actual.Y,i->sp,actual.S,interp816_getFlags(i),actual.P,((unsigned)i->k<<16)|i->pc,stop);assert(0);
 }
 if(actual.cycles!=cycles||actual.master_cycles!=initial.master_cycles+cycles*6+bus_extra||actual.coprocessor_master_cycles!=last){fprintf(stderr,"CLOCK %06x slot%u frame%u phase%u CPU%llu/%llu master%llu/%llu copro%llu/%llu\n",pc,slot,frame,phase,(unsigned long long)actual.cycles,(unsigned long long)cycles,(unsigned long long)actual.master_cycles,(unsigned long long)(initial.master_cycles+cycles*6+bus_extra),(unsigned long long)actual.coprocessor_master_cycles,(unsigned long long)last);assert(0);}
 counts[0]++;if(fallback_count!=prior_fallback)counts[5]++;if(dispatch)counts[6]++;if(phase==1)counts[3]++;else if(boundary)counts[2]++;else counts[1]++;if(inherited)counts[4]++;interp816_free(i);
}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 unsigned counts[7]={0};const unsigned highs[]={0,0x100,0xff00},stacks[]={0x180,0x1fd0};
 for(unsigned slot=0;slot<34;slot++){
  for(unsigned frame=0;frame<256;frame++)compare(slot,frame,0,frame&15,stacks[frame&1],0,frame&1,0,counts);
  for(unsigned hi=0;hi<3;hi++)for(unsigned fl=0;fl<16;fl++)for(unsigned st=0;st<2;st++)for(unsigned phase=0;phase<4;phase++)compare(slot,fl*17,highs[hi],fl,stacks[st],phase,st,0,counts);
 }
 for(unsigned slot=0;slot<256;slot++)for(unsigned fl=0;fl<16;fl++)for(unsigned st=0;st<2;st++)for(unsigned phase=0;phase<4;phase++)compare(slot,fl*17,0,fl,stacks[st],phase,st,1,counts);
 for(unsigned slot=0;slot<34;slot++)for(unsigned hi=1;hi<3;hi++)for(unsigned phase=0;phase<4;phase++)compare(slot,slot*7,highs[hi],slot&15,stacks[slot&1],phase,slot&1,1,counts);
 printf("PASS %u original animation cases; complete=%u interrupted-or-fallback=%u entry-deadline=%u inherited-frame=%u unknown-pointer-floor-boundary=%u full-dispatcher=%u; all34slots/21handlers/sharedBCB2, originalBB4B/256byteselectors+128..161aliases, all256frames, binaryflags/hiddenA/physicalRTSframes, fullRAM+IO/registers/CPU-master-copro clocks; constant8master unitbus\n",counts[0],counts[1],counts[2],counts[3],counts[4],counts[5],counts[6]);assert(counts[1]&&counts[2]&&counts[3]&&counts[4]&&counts[5]&&counts[6]);return 0;
}
