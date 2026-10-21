/* Original queued-transfer opcodes with real frames and strict clocks. */
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



static uint8 inherited_hrv;
static uint8 io[65536],expected_io[65536];
static uint8 vram[65536],expected_vram[65536];
typedef uint32_t Write;
static Write writes[512],expected_writes[512];
static unsigned nwrites,expected_nwrites,dma_count;
static uint64_t dma_master;
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
/* Deliberately bounded device model, shared by both buses. The original
 * 420B write triggers real source bytes, not a substituted transfer result.
 * Only the mode1/increment DMA3 path authored by these four services is legal. */
static uint64_t device_write(uint16 addr,uint8 value) {
 assert(nwrites<sizeof(writes)/sizeof(*writes));writes[nwrites++]=((uint32_t)addr<<8)|value;
 io[addr]=value;if(addr!=0x420B)return 0;
 assert(value==8&&io[0x4330]==1&&io[0x4331]==0x18);
 unsigned source=io[0x4332]|io[0x4333]<<8,bank=io[0x4334];
 unsigned dest=io[0x2116]|io[0x2117]<<8,count=io[0x4335]|io[0x4336]<<8;
 assert(count&&count<=0x800);assert(io[0x2115]==0x80);
 for(unsigned n=0;n<count;n++){
  int32_t off=cpu_wram_offset(bank,(source+n)&65535);assert(off>=0);
  vram[((dest*2)+n)&65535]=ram[off];
 }
 source=(source+count)&65535;dest=(dest+(count+1)/2)&65535;
 io[0x4332]=source;io[0x4333]=source>>8;io[0x4335]=io[0x4336]=0;
 io[0x2116]=dest;io[0x2117]=dest>>8;io[0x420B]=0;dma_count++;
 return count*8+16;
}
uint8 cpu_read8(CpuState *c,uint8 bank,uint16 addr) {
 int32_t off=cpu_wram_offset(bank,addr);
 if(off>=0){return c->open_bus=ram[off];}
 if(bank<0x40&&addr>=0x2100&&addr<0x4400)return c->open_bus=io[addr];
 if(addr>=0x8000)return c->open_bus=rom[(bank&31)*0x8000+(addr&0x7fff)];return c->open_bus=0;
}
void cpu_write8(CpuState *c,uint8 bank,uint16 addr,uint8 value) {
 int32_t off=cpu_wram_offset(bank,addr);c->open_bus=value;
 if(off>=0){ram[off]=value;return;}
 if(!(bank<0x40&&addr>=0x2100&&addr<0x4400))return;
 uint64_t stall=device_write(addr,value);dma_master+=stall;c->master_cycles+=stall;
}
uint16 cpu_read16(CpuState *c,uint8 b,uint16 a) {unsigned lo=cpu_read8(c,b,a);return lo|((unsigned)cpu_read8(c,b,a+1)<<8);}
void cpu_write16(CpuState *c,uint8 b,uint16 a,uint16 v) {cpu_write8(c,b,a,v);cpu_write8(c,b,a+1,v>>8);}
RecompReturn interp_tier_dispatch_tail(CpuState *c,uint32 pc,uint32 site,uint16 s,uint8 h) {
 (void)c;(void)site;assert((pc>>16)==2);assert(s==inherited_s&&h==2);boundary=pc;return RECOMP_RETURN_NORMAL;
}
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 pc,uint32 site,uint8 n,uint32*out) {(void)c;(void)site;(void)n;(void)out;pending=1;boundary=pc;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;}
#include "transfer_services_under_test.h"
static uint8 rd(void *ctx,uint32 a) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);if(off>=0){return ram[off];}
 if((a>>16)<0x40&&(a&65535)>=0x2100&&(a&65535)<0x4400)return io[a&65535];
 if((a&65535)>=32768)return rom[((a>>16)&31)*0x8000+(a&32767)];return 0;
}
static void wr(void *ctx,uint32 a,uint8 v) {
 (void)ctx;bus_extra+=2;int32_t off=cpu_wram_offset(a>>16,a);
 if(off>=0){ram[off]=v;return;}
 if(!((a>>16)<0x40&&(a&65535)>=0x2100&&(a&65535)<0x4400))return;
 bus_extra+=device_write(a&65535,v);
}


static void compare(unsigned pc,uint32_t number,unsigned state,unsigned hi,unsigned flags,unsigned stack,unsigned phase,unsigned*counts){
 for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*31+state*17+flags);
 memset(io,0,sizeof io);CpuState initial={0};initial.A=hi|(uint8)(state*37);
 const unsigned xs[]={0,0x122,0xFFC0,0xFFFF},ys[]={0,1,0x7FFF,0xFFFF};
 initial.X=xs[state&3];initial.Y=ys[(state>>2)&3];initial.S=stack;initial.DB=(state&2)?2:0;initial.PB=0;initial.ram=ram;
 initial.host_return_valid=pc==0x00871C?3:2;
 initial.P=CPU_P_M|CPU_P_I|((flags&1)?CPU_P_C:0)|((flags&2)?CPU_P_V:0)|((flags&4)?CPU_P_N:0)|((flags&8)?CPU_P_D:0);
 cpu_p_to_mirrors(&initial);ram[stack+1]=0x34;ram[stack+2]=0x12;if(initial.host_return_valid==3)ram[stack+3]=pc>>16;
 ram[0x23A]=number&15;ram[0x23B]=(number&0xF0);ram[0x23F]=state&7;
 ram[0x410]=0x00;ram[0x411]=0x60;ram[0x412]=0x7E;
 ram[0x413]=(state&1)?0x80:0;ram[0x414]=(state&1)?0:8;
 ram[0x415]=state*31;ram[0x416]=state*7;
 if(pc==0x008F8C){initial.X=(state&1)?0xFFF0:0x4000;initial.Y=(state&2)?0xFFF0:0x6000;}
 io[0x2115]=0x80;for(unsigned n=0;n<sizeof(vram);n++)vram[n]=n*7+state;
 nwrites=0;dma_master=0;
 initial.master_cycles=phase==1?1:0;initial.coprocessor_master_cycles=initial.master_cycles;
 deadline=phase==1?1:phase==2?60:phase==3?700:0;pending=0;boundary=0;inherited_pending=0;
 memcpy(before,ram,sizeof ram);CpuState actual=initial;
 RecompReturn result=transfer_body(pc,&actual);assert(!g_recomp_stack_top);cpu_mirrors_to_p(&actual);assert(result==RECOMP_RETURN_NORMAL||(pending&&boundary));
 memcpy(expected,ram,sizeof ram);memcpy(expected_io,io,sizeof io);memcpy(expected_vram,vram,sizeof vram);
 expected_nwrites=nwrites;memcpy(expected_writes,writes,nwrites*sizeof(*writes));
 memcpy(ram,before,sizeof ram);memset(io,0,sizeof io);io[0x2115]=0x80;
 for(unsigned n=0;n<sizeof(vram);n++)vram[n]=n*7+state;nwrites=0;
 Interp816*i=interp816_init(NULL,rd,wr);assert(i);i->a=initial.A;i->x=initial.X;i->y=initial.Y;i->sp=initial.S;i->dp=0;i->db=initial.DB;i->k=initial.PB;i->pc=pc;i->e=0;interp816_setFlags(i,initial.P);
 unsigned stop=boundary?boundary:((pc&0xFF0000)|0x1235),ops=0;uint64_t cycles=0,last=initial.master_cycles;bus_extra=0;
 while(cycles<actual.cycles){
  if(++ops>1000000){fprintf(stderr,"cap %06x value%08x phase%u pc%02x%04x stop%06x\n",pc,number,phase,i->k,i->pc,stop);assert(0);}
  last=initial.master_cycles+cycles*6+bus_extra;cycles+=interp816_runOpcode(i);
 }
 unsigned first=0;while(first<sizeof ram&&ram[first]==expected[first])first++;
 if(first<sizeof ram||memcmp(io,expected_io,sizeof io)||memcmp(vram,expected_vram,sizeof vram)||nwrites!=expected_nwrites||memcmp(writes,expected_writes,nwrites*sizeof(*writes))||i->a!=actual.A||i->x!=actual.X||i->y!=actual.Y||i->sp!=actual.S||i->dp!=actual.D||i->db!=actual.DB||i->k!=actual.PB||interp816_getFlags(i)!=actual.P||(((unsigned)i->k<<16)|i->pc)!=stop){
  fprintf(stderr,"STATE %06x value%08x hi%04x flags%u S%04x phase%u RAM%05x A%04x/%04x X%04x/%04x Y%04x/%04x S%04x/%04x P%02x/%02x PC%06x/%06x\n",pc,number,hi,flags,stack,phase,first,i->a,actual.A,i->x,actual.X,i->y,actual.Y,i->sp,actual.S,interp816_getFlags(i),actual.P,((unsigned)i->k<<16)|i->pc,stop);assert(0);
 }
 if(actual.cycles!=cycles||actual.master_cycles!=initial.master_cycles+cycles*6+bus_extra||actual.coprocessor_master_cycles!=last){
  fprintf(stderr,"CLOCK %06x value%08x flags%u phase%u CPU%llu/%llu master%llu/%llu copro%llu/%llu\n",pc,number,flags,phase,(unsigned long long)actual.cycles,(unsigned long long)cycles,(unsigned long long)actual.master_cycles,(unsigned long long)(initial.master_cycles+cycles*6+bus_extra),(unsigned long long)actual.coprocessor_master_cycles,(unsigned long long)last);assert(0);
 }
 counts[0]++;if(phase==1)counts[3]++;else if(boundary){counts[2]++;if(actual.S<initial.S)counts[4]++;}else counts[1]++;
 interp816_free(i);
}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 unsigned counts[5]={0};const unsigned highs[]={0,0x100,0xFF00},stacks[]={0x180,0x1FD0};
 for(unsigned body=0;body<sizeof(transfer_roots)/sizeof(*transfer_roots);body++)for(unsigned st=0;st<2;st++)for(unsigned hi=0;hi<3;hi++)for(unsigned fl=0;fl<16;fl++)for(unsigned n=0;n<(transfer_roots[body]==0x8EE7?64u:4u);n++)for(unsigned phase=0;phase<4;phase++)for(unsigned state=0;state<8;state++)
  compare(transfer_roots[body],(transfer_roots[body]==0x8EE7?((n&3)<<2)|((n>>2)<<4):n),state,highs[hi],fl,stacks[st],phase,counts);
 assert(dma_count);printf("PASS %u queued-transfer original-opcode cases; complete=%u internal-deadline=%u entry-deadline=%u saved-frame-boundary=%u; DMAwrites=%u; %ufullbodies/16flags/3Ahigh/2stacks/DB00+02/all8BGmodes; full128KiBRAM+IO+64KiBVRAM+orderedMMIO/registers/realnested+deadstack/CPU-master-copro clocks; constant8master unitbus+modeledDMAstall\n",counts[0],counts[1],counts[2],counts[3],counts[4],dma_count,(unsigned)(sizeof(transfer_roots)/sizeof(*transfer_roots)));return 0;
}
