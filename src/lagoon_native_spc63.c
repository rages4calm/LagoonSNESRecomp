/* Original byte-wide IPL acknowledgment polls during unmapped boot upload.
 * Never an idle replay: each CMP reads the actual asynchronous APU port. */
#include "lagoon_native_spc63.h"
#include "snes/interp_bridge.h"
#include <string.h>
#include <stdlib.h>
static struct {const CpuState*cpu;const uint8_t*ram;uint64_t generation;
 uint16_t s;uint32_t pointer;uint8_t db,ret[2];int armed;} spc_witness;
static struct {uint64_t armed,reads,branches,deadlines,refused;} spc_stats;
int LagoonNativeSpc63_Source(const uint8_t*r,size_t n){
 if(!r||n!=0x100000)return 0;uint64_t h=UINT64_C(14695981039346656037);
 for(unsigned k=0x67f69;k<0x67fcc;++k)h=(h^r[k])*UINT64_C(1099511628211);
 if(h!=UINT64_C(0xbfffc88dd0db851e))return 0;
 h=UINT64_C(14695981039346656037);
 for(unsigned k=0x67e9b;k<0x68000;++k)h=(h^r[k])*UINT64_C(1099511628211);
 if(h!=UINT64_C(0x92b9c182f5a7c275))return 0;
 h=UINT64_C(14695981039346656037);
 for(unsigned k=0x68000;k<0x68d46;++k)h=(h^r[k])*UINT64_C(1099511628211);
 return h==UINT64_C(0xf92530e36c298aab);
}
static int spc_mode(const CpuState*c){return c&&c->ram&&c->PB==12&&c->DB<=2&&!c->D&&
 !c->emulation&&c->m_flag&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x3c)==0x24;}
static unsigned spc_pointer(const CpuState*c){return c->ram[0x1c]|c->ram[0x1d]<<8|c->ram[0x1e]<<16;}
static int spc_pack(const uint8_t*r,unsigned p){
 static const struct {unsigned p,n;uint64_t hash;} packs[]={
 {0x0c8000,24764,UINT64_C(0xb36f260378f7d934)},{0x0d8d46,28556,UINT64_C(0x804ad309af479599)},
 {0x0fa12e,6461,UINT64_C(0x1366f356cc6088e1)},{0x0fba6b,5100,UINT64_C(0xc0c86d80094f14a8)},
 {0x0fce57,1490,UINT64_C(0x4aa0346eddcb4b2e)},{0x0e8000,7445,UINT64_C(0x3a857526da7d58a6)},
 {0x0e9d15,9110,UINT64_C(0x141213c21c2cc7f5)},{0x0ec0ab,8028,UINT64_C(0xdf132f503ac6f21e)},
 {0x0f8000,8494,UINT64_C(0x999adb07d17671c8)},{0x0fd429,7900,UINT64_C(0x5e90eb241276b539)},
 {0x0d8000,3398,UINT64_C(0x5b367902e5ef1975)}};
 for(unsigned k=0;k<sizeof packs/sizeof *packs;++k)if(p==packs[k].p){
  uint64_t h=UINT64_C(1469598103934665603);unsigned at=(p>>16)*32768+(p&32767);
  for(unsigned j=0;j<packs[k].n;++j)h=(h^r[at+j])*UINT64_C(1099511628211);
  return h==packs[k].hash;
 }
 return 0;
}
int LagoonNativeSpc63_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,
 uint64_t generation,uint64_t frame,uint32_t pc,uint32_t*landing){
 if(!landing||c!=primary||generation!=frame||!spc_mode(c)||!LagoonNativeSpc63_Source(r,n)){
  spc_witness.armed=0;++spc_stats.refused;return 0;
 }
 if(pc==0x0cff69){
  spc_witness.armed=0;
  if(c->S<0x100||c->S>0x1ff8)return 0;
  unsigned ret=c->ram[c->S+1]|c->ram[c->S+2]<<8,p=spc_pointer(c);
  int context=ret==0xff5b?!c->DB&&p==0x0d8000:
    ret==0xfee2?c->DB&&(p==0x0c8000||p==0x0d8d46):
    ret==0xff33?c->DB&&p!=0x0c8000&&p!=0x0d8d46&&p!=0x0d8000:0;
  if(!context||!spc_pack(r,p))return 0;
  spc_witness.cpu=c;spc_witness.ram=c->ram;spc_witness.generation=generation;spc_witness.s=c->S;
  spc_witness.pointer=p;spc_witness.db=c->DB;memcpy(spc_witness.ret,c->ram+c->S+1,2);spc_witness.armed=1;++spc_stats.armed;return 0;
 }
 if(pc==0x0cffca){spc_witness.armed=0;return 0;}
 if(!spc_witness.armed||spc_witness.cpu!=c||spc_witness.ram!=c->ram||
   spc_witness.generation!=generation||c->DB!=spc_witness.db||c->S+1!=spc_witness.s||
   memcmp(spc_witness.ret,c->ram+spc_witness.s+1,2)||
   spc_pointer(c)!=spc_witness.pointer){
  spc_witness.armed=0;++spc_stats.refused;return 0;
 }
 unsigned base=pc==0x0cff8a||pc==0x0cff8d?0x0cff8a:
   pc==0x0cff9a||pc==0x0cff9d?0x0cff9a:pc==0x0cffc3||pc==0x0cffc6?0x0cffc3:0;
 if(!base)return 0;
 for(;;){
  if(interp_bridge_source_opcode_deadline_reached(c)){++spc_stats.deadlines;*landing=pc;return 1;}
  uint64_t prior=c->master_cycles;unsigned cycles,bytes,next;
  if(pc==base){
   /* Original opcode-start bus observation precedes instruction clock charge. */
   c->open_bus=0x21;uint8_t v;
   if(!interp_bridge_source_read8(c,((uint32_t)c->DB<<16)|0x2140,&v))abort();
   c->_flag_C=(c->A&255)>=v;unsigned d=((c->A&255)-v)&255;
   c->_flag_Z=!d;c->_flag_N=!!(d&128);cpu_mirrors_to_p(c);
   cycles=4;bytes=3;next=pc+3;++spc_stats.reads;
  }else{
   c->open_bus=0xfb;cycles=c->_flag_Z?2:3;bytes=2;next=c->_flag_Z?base+5:base;++spc_stats.branches;
  }
  c->coprocessor_master_cycles=c->master_cycles;c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2;
  if(bytes==3)c->master_cycles+=cpu_region_speed(((uint32_t)c->DB<<16)|0x2140)-6;
  if(!interp_bridge_commit_source_opcode(c,prior,false))abort();pc=next;
  if(pc==base+5){*landing=pc;return 1;}
 }
}
#ifndef LAGOON_NATIVE_SPC63_TEST
#include "common_rtl.h"
#include <stdio.h>
static uint64_t spc_frame;static int spc_enabled=-1,spc_report;
void LagoonNativeSpc63_ObserveEntry(CpuState*c,uint16_t s,uint8_t h,uint32_t host){
 if(spc_enabled!=1||c!=&g_cpu||!c->ram||c->S!=s||s>0x1ff8||h!=2||c->host_return_valid!=2||
  (host!=0x0cfee3&&host!=0x0cff34&&host!=0x0cff5c))return;
 unsigned physical=c->ram[s+1]|c->ram[s+2]<<8;if(((physical+1)&65535)!=(host&65535))return;
 uint32_t unused;LagoonNativeSpc63_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),spc_frame,0xcff69,&unused);
}
static void spc_exit(void){if(spc_report)fprintf(stderr,"[native-spc63] armed=%llu reads=%llu branches=%llu deadlines=%llu refused=%llu\n",
 (unsigned long long)spc_stats.armed,(unsigned long long)spc_stats.reads,(unsigned long long)spc_stats.branches,(unsigned long long)spc_stats.deadlines,(unsigned long long)spc_stats.refused);}
static void spc_hook(CpuState*c,uint32_t pc){
 if(!spc_enabled||c!=&g_cpu)return;
 /* An unyieldable nested observation is original-only. Retain the witnessed
  * real frame; resume only after all callback/source/frame guards pass again. */
 if(!interp_bridge_source_opcode_available(c))return;
 for(unsigned k=0x67f69;k<0x67fcc;++k)if(!cpu_aot_rom_mapping_matches(0x0c8000u+(k&32767),k,1)){spc_witness.armed=0;return;}
 uint32_t landing;
 if(LagoonNativeSpc63_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),spc_frame,pc,&landing))interp_bridge_pre_opcode_redirect(landing);
}
void LagoonNativeSpc63_BeforeFrame(void){
 if(spc_enabled<0){const char*v=getenv("LAGOON_NATIVE_SPC63");spc_enabled=!v||!strcmp(v,"on");v=getenv("LAGOON_NATIVE_WAIT_REPORT");spc_report=v&&!strcmp(v,"1");if(spc_report)atexit(spc_exit);}
 spc_frame=RtlStateGeneration();if(spc_witness.armed&&spc_witness.generation!=spc_frame)spc_witness.armed=0;
 if(spc_witness.armed&&!spc_pack(g_rom,spc_witness.pointer))spc_witness.armed=0;
 if(spc_enabled){const unsigned pcs[]={0x0cff69,0x0cff8a,0x0cff8d,0x0cff9a,0x0cff9d,0x0cffc3,0x0cffc6,0x0cffca};
  for(unsigned k=0;k<sizeof pcs/sizeof *pcs;++k)interp_bridge_add_pre_opcode_hook(pcs[k],spc_hook);}
}
#endif
