/* Timing-owned hot windows of the original resource decoder. Observe the real
 * 9143 entry; execute the original dictionary stack data operations in order.
 * All decoder returns remain at their original source boundaries.
 * This supplements, rather than broadens, the qualified menu fast path. */
#include "lagoon_native_decode63.h"
#include "snes/interp_bridge.h"
#include <stdlib.h>
#include <string.h>
static struct {const CpuState*cpu;const uint8_t*ram;uint64_t generation;
 uint16_t s;uint8_t db,ret[3];int armed;} decode_witness;
static struct {uint64_t armed,clear,chain,output,instructions,deadlines,refused;} decode_stats;
int LagoonNativeDecode63_Source(const uint8_t*r,size_t n){
 if(!r||n!=0x100000)return 0;
 uint64_t h=UINT64_C(14695981039346656037);
 for(unsigned k=0x1143;k<0x1304;++k)h=(h^r[k])*UINT64_C(1099511628211);
 return h==UINT64_C(0x1ab145d7fd2e3ea6);
}
static int decode_mode(const CpuState*c){return c&&c->ram&&!c->PB&&c->DB<=2&&!c->D&&
 !c->emulation&&c->m_flag&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x3c)==0x24;}
static void decode_nz(CpuState*c,unsigned v,unsigned bits){
 unsigned mask=bits==8?255:65535;c->_flag_Z=!(v&mask);c->_flag_N=!!(v&(1u<<(bits-1)));cpu_mirrors_to_p(c);
}
static void decode_a(CpuState*c,unsigned v){c->A=(c->A&0xff00)|(v&255);decode_nz(c,v,8);}
static unsigned decode_long(CpuState*c,unsigned base){unsigned p=(base+c->X)&0xffffff;return cpu_read8_timed(c,p>>16,p);}
static void decode_store(CpuState*c,unsigned base){unsigned p=(base+c->X)&0xffffff;cpu_write8_timed(c,p>>16,p,c->A);}
/* pc, source length, base cycles. No opcode decoding or synthetic return. */
static const unsigned decode_ops[][3]={
 {0x9162,4,5},{0x9166,1,2},{0x9167,2,0},
 {0x925d,2,3},{0x925f,2,3},{0x9261,1,2},{0x9262,4,5},{0x9266,2,0},{0x9268,3,4},
 {0x926b,1,2},{0x926c,2,4},{0x926e,2,0},{0x9270,2,3},{0x9272,2,3},{0x9274,2,4},
 {0x9276,4,5},{0x927a,2,3},{0x927c,2,4},{0x927e,4,5},{0x9282,2,3},{0x9284,2,0},
 {0x9286,1,2},{0x9287,2,4},{0x9289,2,0},{0x928b,2,0},{0x928d,2,3},
 {0x928f,2,4},{0x9291,4,5},{0x9295,2,3},{0x9297,2,4},{0x9299,4,5},{0x929d,2,3},{0x929f,1,2},
 {0x92a0,1,5},{0x92a1,1,2},{0x92a2,4,5},{0x92a6,1,2},{0x92a7,2,3},{0x92a9,4,5},
 {0x92ad,1,4},{0x92ae,2,4},{0x92b0,4,5},{0x92b4,2,3},{0x92b6,2,3},
 {0x92b8,2,3},{0x92ba,2,3},{0x92bc,2,3},{0x92be,2,4},{0x92c0,4,5},{0x92c4,1,2},{0x92c5,2,4},
 {0x92c7,1,5},{0x92c8,4,5},{0x92cc,2,3},{0x92ce,1,2},{0x92cf,4,5},{0x92d3,2,3},
 {0x92d5,2,0},{0x92d7,1,2},{0x92d8,2,3},{0x92da,2,3},{0x92dc,1,4},{0x92dd,3,4},
 {0x92e0,1,2},{0x92e1,1,4},{0x92e2,3,4}
};
int LagoonNativeDecode63_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,
 uint64_t generation,uint64_t frame,uint32_t pc,uint32_t*landing){
 if(!landing||c!=primary||generation!=frame||!decode_mode(c)||!LagoonNativeDecode63_Source(r,n)){
  decode_witness.armed=0;++decode_stats.refused;return 0;
 }
 if(pc==0x9143){
  decode_witness.armed=0;if(c->S<0x120||c->S>0x1fc)return 0;
  decode_witness.cpu=c;decode_witness.ram=c->ram;decode_witness.generation=generation;
  decode_witness.s=c->S;decode_witness.db=c->DB;memcpy(decode_witness.ret,c->ram+c->S+1,3);
  decode_witness.armed=1;++decode_stats.armed;return 0;
 }
 if(pc==0x9303){decode_witness.armed=0;return 0;}
 if(!decode_witness.armed||decode_witness.cpu!=c||decode_witness.ram!=c->ram||
  decode_witness.generation!=generation||decode_witness.db!=c->DB||
  c->S>decode_witness.s||c->S+8<decode_witness.s||
  memcmp(decode_witness.ret,c->ram+decode_witness.s+1,3)){
  decode_witness.armed=0;++decode_stats.refused;return 0;
 }
 unsigned executed=0;
 for(;;){
  if(c->S>decode_witness.s||c->S+8<decode_witness.s||memcmp(decode_witness.ret,c->ram+decode_witness.s+1,3)){
   decode_witness.armed=0;++decode_stats.refused;if(!executed)return 0;*landing=pc;return 1;
  }
  unsigned k;for(k=0;k<sizeof decode_ops/sizeof *decode_ops;++k)if(decode_ops[k][0]==pc)break;
  if(k==sizeof decode_ops/sizeof *decode_ops){if(!executed)return 0;*landing=pc;return 1;}
  if(interp_bridge_source_opcode_deadline_reached(c)){++decode_stats.deadlines;*landing=pc;return 1;}
  unsigned bytes=decode_ops[k][1],cycles=decode_ops[k][2],next=pc+bytes,wrote=0;
  if(!cycles){unsigned take=pc==0x9167?!c->_flag_N:pc==0x9266?!c->_flag_Z:
    pc==0x926e||pc==0x9289?c->_flag_N:c->_flag_Z;
   cycles=take?3:2;if(take)next=pc+bytes+(int8_t)r[(pc&32767)+1];}
  uint64_t prior=c->master_cycles;c->open_bus=r[(pc&32767)+bytes-1];
  c->coprocessor_master_cycles=c->master_cycles;c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2;
  switch(pc){
  case 0x9162:decode_store(c,0x7f2110);wrote=1;++decode_stats.clear;break;
  case 0x9166:c->X=(uint16_t)(c->X-1);decode_nz(c,c->X,16);break;
  case 0x925d:case 0x92bc:decode_a(c,cpu_read8_timed(c,0,0xeb));break;
  case 0x925f:case 0x929d:cpu_write8_timed(c,0,0xed,c->A);wrote=1;break;
  case 0x9261:case 0x926b:case 0x9286:c->X=c->A;decode_nz(c,c->X,16);break;
  case 0x9262:case 0x9276:case 0x9291:decode_a(c,decode_long(c,0x7f2110));break;
  case 0x9268:next=0x92bc;break;
  case 0x926c:case 0x9287:{unsigned v=cpu_read16_timed(c,0,pc==0x926c?0xe9:0xeb);c->_flag_C=c->X>=v;decode_nz(c,(uint16_t)(c->X-v),16);break;}
  case 0x9270:decode_a(c,cpu_read8_timed(c,0,0xe9));break;
  case 0x9272:case 0x92ba:cpu_write8_timed(c,0,0xeb,c->A);wrote=1;break;
  case 0x9274:case 0x928f:c->X=cpu_read16_timed(c,0,0xed);decode_nz(c,c->X,16);break;
  case 0x927a:case 0x9282:case 0x9295:cpu_write8_timed(c,0,0xe9,c->A);wrote=1;break;
  case 0x927c:case 0x9297:c->X=cpu_read16_timed(c,0,0xe9);decode_nz(c,c->X,16);break;
  case 0x927e:decode_a(c,decode_long(c,0x7f2220));break;
  case 0x928d:next=0x927c;break;
  case 0x9299:decode_a(c,decode_long(c,0x7f2440));break;
  case 0x929f:decode_a(c,c->X);break;
  case 0x92a0:case 0x92c7:c->X=cpu_read16_timed(c,0,c->S+1);c->S+=2;decode_nz(c,c->X,16);break;
  case 0x92a1:case 0x92a6:c->X=(uint16_t)(c->X+1);decode_nz(c,c->X,16);break;
  case 0x92a2:case 0x92a9:decode_store(c,0x7f2800);wrote=1;break;
  case 0x92a7:case 0x92d8:decode_a(c,cpu_read8_timed(c,0,0xed));break;
  case 0x92ad:case 0x92dc:case 0x92e1:cpu_write8_timed(c,0,c->S,c->X>>8);--c->S;cpu_write8_timed(c,0,c->S,c->X);--c->S;wrote=1;break;
  case 0x92ae:c->X=cpu_read16_timed(c,0,0xe9);decode_nz(c,c->X,16);break;
  case 0x92b0:decode_a(c,decode_long(c,0x7f2330));break;
  case 0x92b4:case 0x92da:cpu_write8_timed(c,0,0xeb,c->A);wrote=1;break;
  case 0x92b6:case 0x92dd:next=0x925d;break;
  case 0x92b8:decode_a(c,cpu_read8_timed(c,0,0xed));break;
  case 0x92be:c->X=cpu_read16_timed(c,0,0xf9);decode_nz(c,c->X,16);break;
  case 0x92c0:decode_store(c,0x7f7f7c);wrote=1;++decode_stats.output;break;
  case 0x92c4:c->X=(uint16_t)(c->X+1);decode_nz(c,c->X,16);break;
  case 0x92c5:cpu_write16_timed(c,0,0xf9,c->X);wrote=1;break;
  case 0x92c8:case 0x92cf:decode_a(c,decode_long(c,0x7f2800));break;
  case 0x92cc:cpu_write8_timed(c,0,0xed,c->A);wrote=1;break;
  case 0x92ce:case 0x92d7:case 0x92e0:c->X=(uint16_t)(c->X-1);decode_nz(c,c->X,16);break;
  case 0x92d3:cpu_write8_timed(c,0,0xe9,c->A);wrote=1;break;
  case 0x92e2:next=0x920d;break;
  default:break;
  }
  if(pc>=0x925d&&pc<0x92bc)++decode_stats.chain;
  ++decode_stats.instructions;++executed;
  if(!interp_bridge_commit_source_opcode(c,prior,wrote))abort();pc=next;
 }
}
#ifndef LAGOON_NATIVE_DECODE63_TEST
#include "common_rtl.h"
#include <stdio.h>
static uint64_t decode_frame;
static int decode_enabled=-1,decode_report;
void LagoonNativeDecode63_ObserveEntry(CpuState*c,uint16_t s,uint8_t h,uint32_t host){
 if(decode_enabled!=1||c!=&g_cpu||!c->ram||c->S!=s||h!=3||c->host_return_valid!=3||s>0x1fc)return;
 unsigned physical=c->ram[s+1]|c->ram[s+2]<<8|c->ram[s+3]<<16;
 if(((physical&0xff0000)|((physical+1)&65535))!=host)return;
 uint32_t unused;LagoonNativeDecode63_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),decode_frame,0x9143,&unused);
}
static void decode_exit(void){if(decode_report)fprintf(stderr,"[native-decode63] armed=%llu clear=%llu chain=%llu output=%llu instructions=%llu deadlines=%llu refused=%llu\n",
 (unsigned long long)decode_stats.armed,(unsigned long long)decode_stats.clear,(unsigned long long)decode_stats.chain,
 (unsigned long long)decode_stats.output,(unsigned long long)decode_stats.instructions,(unsigned long long)decode_stats.deadlines,(unsigned long long)decode_stats.refused);}
static void decode_hook(CpuState*c,uint32_t pc){
 if(!decode_enabled||c!=&g_cpu)return;
 if(!interp_bridge_source_opcode_available(c)){decode_witness.armed=0;return;}
 for(unsigned k=0x1143;k<0x1304;++k)if(!cpu_aot_rom_mapping_matches(0x8000u+k,k,1)){decode_witness.armed=0;return;}
 uint32_t landing;if(LagoonNativeDecode63_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),decode_frame,pc,&landing))interp_bridge_pre_opcode_redirect(landing);
}
void LagoonNativeDecode63_BeforeFrame(void){
 if(decode_enabled<0){const char*v=getenv("LAGOON_NATIVE_DECODE63");decode_enabled=!v||!strcmp(v,"on");v=getenv("LAGOON_NATIVE_WAIT_REPORT");decode_report=v&&!strcmp(v,"1");if(decode_report)atexit(decode_exit);}
 decode_frame=RtlStateGeneration();if(decode_witness.armed&&decode_witness.generation!=decode_frame)decode_witness.armed=0;
 extern void (*g_lagoon_decode_source_observer)(CpuState*,uint16_t,uint8_t,uint32_t);
 g_lagoon_decode_source_observer=decode_enabled?LagoonNativeDecode63_ObserveEntry:NULL;
 if(decode_enabled){interp_bridge_add_pre_opcode_hook(0x9143,decode_hook);interp_bridge_add_pre_opcode_hook(0x9303,decode_hook);
  for(unsigned k=0;k<sizeof decode_ops/sizeof *decode_ops;++k)interp_bridge_add_pre_opcode_hook(decode_ops[k][0],decode_hook);}
}
#endif
