/* Actual shared-service opcodes; real child/wait boundaries, no fabricated returns. */
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
static unsigned service_entry;
static unsigned probe_clock_failures;
static unsigned poll_reads,poll_model;
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
RecompReturn cpu_unresolved_abandon_balanced(CpuState*c,uint32 site,uint16 s,uint8 h){(void)site;(void)s;(void)h;boundary=((unsigned)c->PB<<16)|ram[0x26]|ram[0x27]<<8;return RECOMP_RETURN_NORMAL;}
uint32_t cpu_region_speed(uint32_t a) {(void)a;return 8;}
uint8 cpu_read8(CpuState *c,uint8 bank,uint16 addr) {
 int32_t off=cpu_wram_offset(bank,addr);
 if(off>=0){if(off==0x23A&&poll_model&&(ram[off]&0x20)&&++poll_reads==4){ram[off]&=~0x20;poll_reads=0;}return c->open_bus=ram[off];}
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
RecompReturn interp_tier_dispatch_tail(CpuState *c,uint32 pc,uint32 site,uint16 s,uint8 h) {
 (void)c;(void)site;assert((pc>>16)==2);assert(s==inherited_s&&h==2);boundary=pc;return RECOMP_RETURN_NORMAL;
}
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 pc,uint32 site,uint8 n,uint32*out) {(void)c;(void)site;(void)n;(void)out;pending=1;boundary=pc;return (RecompReturn)1;}
#include "shared_services_under_test.h"
static uint8 rd(void *ctx,uint32 a) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);if(off>=0){if(off==0x23A&&poll_model&&(ram[off]&0x20)&&++poll_reads==4){ram[off]&=~0x20;poll_reads=0;}return ram[off];}
 if((a>>16)<0x40&&(a&65535)>=0x2100&&(a&65535)<0x4400)return io[a&65535];
 if((a&65535)>=32768)return rom[((a>>16)&31)*0x8000+(a&32767)];return 0;
}
static void wr(void *ctx,uint32 a,uint8 v) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);
 if(off>=0){ram[off]=v;return;}
 if(!((a>>16)<0x40&&(a&65535)>=0x2100&&(a&65535)<0x4400))return;io[a&65535]=v;
 if((a&65535)==0x4203){unsigned product=io[0x4202]*v;io[0x4216]=product;io[0x4217]=product>>8;}
}

static void word(unsigned addr,unsigned value){ram[addr]=(uint8)value;ram[addr+1]=(uint8)(value>>8);}
static unsigned frame(unsigned pc){return pc==0x8211||pc==0x8C24||pc==0x8C93||pc==0x8DFE||pc==0x8291||pc==0x8B68||pc==0x8D0B||(pc>>16)==1?2:3;}
static void compare(unsigned pc,unsigned state,unsigned hi,unsigned flags,unsigned stack,unsigned early,unsigned*counts){
 for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*73+state*17+hi*19+flags);
 memset(io,0,sizeof io);
 CpuState initial={0};initial.A=hi|(uint8)(state*37);initial.X=state*19;initial.Y=state*29;
 initial.S=stack;initial.DB=1;initial.PB=pc>>16;initial.ram=ram;initial.host_return_valid=frame(pc);
 initial.P=CPU_P_M|((flags&1)?CPU_P_C:0)|((flags&2)?CPU_P_V:0)|((flags&4)?CPU_P_N:0)|((flags&8)?CPU_P_D:0);
 cpu_p_to_mirrors(&initial);ram[stack+1]=0x34;ram[stack+2]=0x12;if(frame(pc)==3)ram[stack+3]=pc>>16;
 ram[0x236]=(uint8)state;ram[0x29e]=state;
 word(0x28,0x7FFF+state);word(0x2A,0xFFF0+state);ram[0x2C]=state*31;ram[0x2D]=state*73;ram[0x2E]=state&1;
 // Real writable source fixture for the queue builder; dimensions1..3 and all
 // skip, ordinary, page/column-wrap and queue-full source branches.
 ram[8]=0;ram[9]=0x30;ram[10]=0x7E;ram[0x3000]=state%3+1;ram[0x3001]=(state/3)%3+1;
 for(unsigned n=0;n<32;n++)word(0x3002+n*2,(state&8)?0xFFFF:state*99+n);
 word(0x215,0x3F0+state);word(0x238,(state&16)?0x77C:0);ram[0x299]=state&3;
 word(0x27B,0xFF00+state);word(0x27D,state*23);
 //8C96's existing8B68 child takes its real cached branch; pointer table and
 //metasprite dimensions are fixture RAM, not patched original ROM outcomes.
 ram[0x2A1]=ram[0x2A2]=0;ram[0x20]=0;ram[0x21]=0x31;ram[0x22]=0x7E;
 word(0x3100+2*(initial.A&255),0x3200);ram[0x3200]=state%3+1;ram[0x3201]=(state/3)%3+1;
 for(unsigned n=2;n<30;n++)ram[0x3200+n]=n+state;
 word(0x27F,0xFFF0+state);word(0x281,0x7FF0+state);
 // D82 selects original seven-byte ROM resource metadata. DCE enters the
 // actual waiting loop; bounded deadline observes it without fake completion.
 if(pc==0x8D82)initial.A=hi|(state%5);
 if(pc==0x8DCE){word(0x28A,state*9);word(8,0xFFF0+state);}
 if(pc==0x8C96){initial.A=hi|(state%3);word(0x3100+((2*initial.A)&65535),0x3200);}
 if(pc==0x8B68&&state&1)ram[0x2A1]=state%4+1;
 if((pc>>16)!=0){word(0x3100,0x3200);word(0x3102,0x3200);word(0x3104,0x3200);word(0x3250,0x3200);word(0x3252,0x3200);ram[0x51D]=state;ram[0x51E]=state;ram[0x536]=state&1?0x80:0;ram[0x537]=state&2?0x80:0;word(0x40,0xFFF0+state);word(0x42,0x7FF0+state);initial.Y=state*4;}
 if(pc==0x8C24){initial.Y=(state%3)==2?0xFFFE:state%3;ram[12]=0;ram[13]=0x32;ram[14]=0x7E;unsigned q=0x3200+initial.Y;
  ram[q]=(state&1)?0xFF:0x37;ram[q+1]=(state&2)?0xFF:0x33;ram[q+2]=0xB2;ram[q+3]=state&4?0x80:0x7F;ram[q+4]=state&8?0x80:0x7F;}
 inherited_s=initial.S;inherited_hrv=initial.host_return_valid;inherited_pending=SERVICE_OAM_PROBE?1:0;
 initial.master_cycles=early==1?1:0;initial.coprocessor_master_cycles=initial.master_cycles;
 poll_model=(pc==0x8DCE||pc==0x8D82)&&(state&1);poll_reads=0;
 deadline=early==4?850:early==3?700:early==2?100:early==1?1:((pc==0x8DCE||pc==0x8D82)&&!poll_model?1000:0);pending=0;boundary=0;service_entry=pc;
 memcpy(before,ram,sizeof ram);CpuState actual=initial;
 RecompReturn result=service_body(pc,&actual);assert(!g_recomp_stack_top);cpu_mirrors_to_p(&actual);
 assert(result==RECOMP_RETURN_NORMAL||(pending&&boundary));
 memcpy(expected,ram,sizeof ram);memcpy(expected_io,io,sizeof io);memcpy(ram,before,sizeof ram);memset(io,0,sizeof io);poll_reads=0;
 Interp816*i=interp816_init(NULL,rd,wr);assert(i);i->a=initial.A;i->x=initial.X;i->y=initial.Y;i->sp=initial.S;
 i->dp=initial.D;i->db=initial.DB;i->k=pc>>16;i->pc=pc;i->e=0;interp816_setFlags(i,initial.P);
 unsigned stop=boundary?boundary:((pc&0xFF0000)|0x1235),ops=0;uint64_t cycles=0,last=initial.master_cycles;bus_extra=0;
 // A deadline boundary may be the same poll PC repeatedly: stop by completed
 // instruction cycles, not first textual PC equality.
 while(cycles<actual.cycles){
  if(++ops>2000000){fprintf(stderr,"cap %04x state%u pc%04x stop%04x cycles%llu/%llu\n",pc,state,i->pc,stop,(unsigned long long)cycles,(unsigned long long)actual.cycles);assert(0);}
  last=initial.master_cycles+cycles*6+bus_extra;cycles+=interp816_runOpcode(i);
 }
 unsigned first=0;while(first<sizeof ram&&ram[first]==expected[first])first++;
 if(first<sizeof ram||memcmp(io,expected_io,sizeof io)||i->a!=actual.A||i->x!=actual.X||i->y!=actual.Y||i->sp!=actual.S||i->dp!=actual.D||i->db!=actual.DB||i->k!=actual.PB||interp816_getFlags(i)!=actual.P||(((unsigned)i->k<<16)|i->pc)!=stop){
  fprintf(stderr,"STATE %04x state%u hi%04x flags%u S%04x early%u RAM%05x %02x/%02x A%04x/%04x X%04x/%04x Y%04x/%04x S%04x/%04x P%02x/%02x PC%06x/%06x\n",pc,state,hi,flags,stack,early,first,first<sizeof ram?ram[first]:0,first<sizeof ram?expected[first]:0,i->a,actual.A,i->x,actual.X,i->y,actual.Y,i->sp,actual.S,interp816_getFlags(i),actual.P,((unsigned)i->k<<16)|i->pc,stop);assert(0);
 }
 if(actual.cycles!=cycles||actual.master_cycles!=initial.master_cycles+cycles*6+bus_extra||actual.coprocessor_master_cycles!=last){
  if(!(SERVICE_OAM_PROBE||SERVICE_LEGACY_CALLER)||!probe_clock_failures)fprintf(stderr,"CLOCK %04x state%u hi%04x flags%u S%04x early%u CPU%llu/%llu master%llu/%llu copro%llu/%llu; full state agrees\n",pc,state,hi,flags,stack,early,(unsigned long long)actual.cycles,(unsigned long long)cycles,(unsigned long long)actual.master_cycles,(unsigned long long)(initial.master_cycles+cycles*6+bus_extra),(unsigned long long)actual.coprocessor_master_cycles,(unsigned long long)last);if(!(SERVICE_OAM_PROBE||SERVICE_LEGACY_CALLER))assert(0);probe_clock_failures++;
 }
 counts[0]++;if(early>1&&boundary>=0x8291&&boundary<=0x829A){assert(actual.S==initial.S-2);counts[4]++;}if(early==1)counts[3]++;else if(boundary)counts[2]++;else counts[1]++;
 interp816_free(i);
}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 unsigned counts[5]={0};const unsigned highs[]={0,0x100,0xFF00},stacks[]={0x180,0x1FD0};
 for(unsigned n=0;n<sizeof(service_roots)/sizeof(*service_roots);n++)for(unsigned st=0;st<2;st++)
 for(unsigned hi=0;hi<3;hi++)for(unsigned fl=0;fl<16;fl++)for(unsigned v=0;v<(service_roots[n]==0x8211?256u:32u);v++)for(unsigned early=0;early<(SERVICE_OAM_PROBE?1u:(service_roots[n]==0x8C93||service_roots[n]==0x8C24)?5u:2u);early++)
  if(!(service_roots[n]==0x8C93&&early==1))compare(service_roots[n],v,highs[hi],fl,stacks[st],early,counts);
 if((SERVICE_OAM_PROBE||SERVICE_LEGACY_CALLER)&&probe_clock_failures){printf("STATE_PASS_CLOCK_FAIL %u actual source entries; fullRAM/registers/flags/Ahigh/realstack agree; %u coarse clock divergences\n",counts[0],probe_clock_failures);return 3;}
 printf("PASS %u shared-service original-opcode comparisons; complete=%u genuine-boundary-or-poll-deadline=%u entry-deadline=%u saved-child-deadline=%u; full128KiBRAM+IO/registers/flags/Ahigh/realnested+deadstack/CPU-master-copro clocks; constant8master unitbus\n",counts[0],counts[1],counts[2],counts[3],counts[4]);return 0;
}
