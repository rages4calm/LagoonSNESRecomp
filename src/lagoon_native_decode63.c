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
static int decode64_enabled=1;
static struct {uint64_t armed,clear,chain,output,instructions,deadlines,refused,input64,raw64;} decode_stats;
int LagoonNativeDecode63_Source(const uint8_t*r,size_t n){
 if(!r||n!=0x100000)return 0;
 uint64_t h=UINT64_C(14695981039346656037);
 for(unsigned k=0x1143;k<0x1304;++k)h=(h^r[k])*UINT64_C(1099511628211);
 return h==UINT64_C(0x1ab145d7fd2e3ea6);
}
static int decode_mode(const CpuState*c){return c&&c->ram&&!c->PB&&c->DB<=2&&!c->D&&
 !c->emulation&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x1c)==4&&!!(c->P&0x20)==!!c->m_flag;}
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
/* Additional original header, input/table and raw-copy windows. Fourth
 * column pins the actual accumulator width at each source instruction. */
static const unsigned decode64_ops[][4]={
 {0x915d,3,3,1},{0x9160,2,2,1},
 {0x9169,2,6,1},{0x916b,2,3,1},{0x916d,1,2,1},{0x916e,2,6,1},{0x9170,2,3,1},{0x9172,1,2,1},
 {0x9173,2,3,1},{0x9175,2,7,0},{0x9177,2,4,0},{0x9179,1,2,0},{0x917a,1,2,0},{0x917b,2,3,0},
 {0x917d,1,3,1},{0x917e,2,2,1},{0x9180,1,3,1},{0x9181,1,4,1},{0x9182,2,4,1},{0x9184,1,2,1},{0x9185,2,4,1},
 {0x9187,2,4,1},{0x9189,2,4,1},{0x918b,2,0,1},{0x918d,3,4,1},{0x9190,3,3,1},
 {0x9193,2,6,1},{0x9195,4,5,1},{0x9199,1,2,1},{0x919a,1,2,1},{0x919b,2,5,1},{0x919d,2,0,1},
 {0x919f,2,3,1},{0x91a1,2,3,1},{0x91a3,3,3,1},{0x91a6,2,6,1},{0x91a8,4,5,1},{0x91ac,1,2,1},{0x91ad,1,2,1},{0x91ae,2,5,1},{0x91b0,2,0,1},
 {0x91b2,2,3,1},{0x91b4,2,3,1},{0x91b6,3,3,1},{0x91b9,2,6,1},{0x91bb,4,5,1},{0x91bf,1,2,1},{0x91c0,1,2,1},{0x91c1,2,5,1},{0x91c3,2,0,1},
 {0x91c5,2,4,1},{0x91c7,1,2,1},{0x91c8,2,4,1},{0x91ca,3,3,1},{0x91cd,2,4,1},{0x91cf,2,3,1},{0x91d1,2,3,1},{0x91d3,2,3,1},{0x91d5,2,3,1},
 {0x91d7,2,4,1},{0x91d9,4,5,1},{0x91dd,2,3,1},{0x91df,1,2,1},{0x91e0,4,5,1},{0x91e4,2,3,1},{0x91e6,2,4,1},{0x91e8,4,5,1},{0x91ec,1,2,1},{0x91ed,2,4,1},{0x91ef,4,5,1},
 {0x91f3,2,4,1},{0x91f5,1,2,1},{0x91f6,2,4,1},{0x91f8,2,4,1},{0x91fa,1,2,1},{0x91fb,2,4,1},{0x91fd,2,0,1},
 {0x91ff,2,4,1},{0x9201,1,2,1},{0x9202,2,4,1},{0x9204,3,3,1},{0x9207,2,4,1},{0x9209,2,3,1},{0x920b,2,3,1},
 {0x920d,2,4,1},{0x920f,1,2,1},{0x9210,2,4,1},{0x9212,2,0,1},{0x9214,2,3,1},{0x9216,2,0,1},{0x9218,3,4,1},{0x921b,3,4,1},
 {0x921e,2,6,1},{0x9220,1,2,1},{0x9221,2,3,1},{0x9223,2,3,1},{0x9225,2,4,1},{0x9227,4,5,1},{0x922b,2,3,1},{0x922d,2,0,1},
 {0x922f,1,2,1},{0x9230,2,4,1},{0x9232,4,5,1},{0x9236,1,2,1},{0x9237,2,4,1},{0x9239,2,3,1},{0x923b,2,4,1},{0x923d,4,5,1},{0x9241,2,3,1},
 {0x9243,1,5,1},{0x9244,1,2,1},{0x9245,2,2,1},{0x9247,4,5,1},{0x924b,1,2,1},{0x924c,4,5,1},{0x9250,1,2,1},{0x9251,4,5,1},{0x9255,1,2,1},{0x9256,4,5,1},{0x925a,1,4,1},{0x925b,2,3,1},
 {0x92e5,2,4,1},{0x92e7,1,2,1},{0x92e8,2,4,1},{0x92ea,2,6,1},{0x92ec,1,2,1},{0x92ed,2,4,1},{0x92ef,4,5,1},{0x92f3,1,2,1},{0x92f4,2,4,1},{0x92f6,2,4,1},{0x92f8,1,2,1},{0x92f9,2,4,1},{0x92fb,2,0,1},{0x92fd,3,4,1},
 {0x9300,1,5,1},{0x9301,1,5,1},{0x9302,1,5,1}
};
static int decode64_is_input(unsigned pc){return pc==0x9169||pc==0x916e||pc==0x9175||pc==0x9193||pc==0x91a6||pc==0x91b9||pc==0x921e||pc==0x92ea;}
static int decode64_input_valid(const CpuState*c){unsigned p=(c->ram[0xe0]|c->ram[0xe1]<<8|c->ram[0xe2]<<16)+c->Y;return p<0x200000&&(p&65535)>=32768&&(c->m_flag||(p&65535)!=65535);}
static void decode64_input(CpuState*c){unsigned p=cpu_read8_timed(c,0,0xe0)|cpu_read8_timed(c,0,0xe1)<<8|cpu_read8_timed(c,0,0xe2)<<16;p=(p+c->Y)&0xffffff;unsigned v=cpu_read8_timed(c,p>>16,p);if(c->m_flag)decode_a(c,v);else{p=(p+1)&0xffffff;c->A=v|cpu_read8_timed(c,p>>16,p)<<8;decode_nz(c,c->A,16);}}
int LagoonNativeDecode63_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,
 uint64_t generation,uint64_t frame,uint32_t pc,uint32_t*landing){
 if(!landing||c!=primary||generation!=frame||!decode_mode(c)||!LagoonNativeDecode63_Source(r,n)){
  decode_witness.armed=0;++decode_stats.refused;return 0;
 }
 if(pc==0x9143){
  decode_witness.armed=0;if(!c->m_flag||c->S<0x120||c->S>0x1fc)return 0;
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
  unsigned k;const unsigned*op=0;int extra=0;
  for(k=0;k<sizeof decode_ops/sizeof *decode_ops;++k)if(decode_ops[k][0]==pc){if(c->m_flag)op=decode_ops[k];break;}
  if(!op&&decode64_enabled)for(k=0;k<sizeof decode64_ops/sizeof *decode64_ops;++k)if(decode64_ops[k][0]==pc&&c->m_flag==decode64_ops[k][3]){op=decode64_ops[k];extra=1;break;}
  if(!op||(decode64_is_input(pc)&&!decode64_input_valid(c))){if(!executed)return 0;*landing=pc;return 1;}
  if(interp_bridge_source_opcode_deadline_reached(c)){++decode_stats.deadlines;*landing=pc;return 1;}
  unsigned bytes=op[1],cycles=op[2],next=pc+bytes,wrote=0;
  if(!cycles){unsigned take=pc==0x9167||pc==0x91fd||pc==0x92fb?!c->_flag_N:
    pc==0x926e||pc==0x9289?c->_flag_N:
    pc==0x9284||pc==0x928b||pc==0x92d5?c->_flag_Z:!c->_flag_Z;
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
  case 0x915d:case 0x9190:case 0x91a3:case 0x91b6:case 0x91ca:case 0x9204:c->X=pc==0x915d?0x101:1;decode_nz(c,c->X,16);break;
  case 0x9160:case 0x917e:case 0x9245:decode_a(c,0);break;
  case 0x9169:case 0x916e:case 0x9175:case 0x9193:case 0x91a6:case 0x91b9:case 0x921e:case 0x92ea:decode64_input(c);if(pc==0x92ea)++decode_stats.raw64;break;
  case 0x916b:case 0x9170:case 0x91a1:case 0x91b4:case 0x91dd:case 0x91e4:case 0x9221:case 0x9223:case 0x922b:case 0x9241:cpu_write8_timed(c,0,r[(pc&32767)+1],c->A);wrote=1;break;
  case 0x9177:cpu_write16_timed(c,0,0xe7,c->A);wrote=1;break;
  case 0x916d:case 0x9172:case 0x9179:case 0x917a:case 0x919a:case 0x91ad:case 0x91c0:case 0x9220:case 0x92ec:c->Y=(uint16_t)(c->Y+1);decode_nz(c,c->Y,16);break;
  case 0x9173:c->P&=~0x20;cpu_p_to_mirrors(c);break;
  case 0x917b:c->P|=0x20;cpu_p_to_mirrors(c);break;
  case 0x917d:cpu_write8_timed(c,0,c->S,c->A);--c->S;wrote=1;break;
  case 0x9180:c->A=(uint16_t)((c->A>>8)|(c->A<<8));decode_nz(c,c->A&255,8);break;
  case 0x9181:decode_a(c,cpu_read8_timed(c,0,++c->S));break;
  case 0x9182:case 0x9187:case 0x91c5:case 0x91d7:case 0x91e6:case 0x91ed:case 0x91f3:case 0x91f8:case 0x91ff:case 0x920d:case 0x9225:case 0x9230:case 0x923b:case 0x92e5:case 0x92ed:case 0x92f6:c->X=cpu_read16_timed(c,0,r[(pc&32767)+1]);decode_nz(c,c->X,16);break;
  case 0x9184:case 0x9199:case 0x91ac:case 0x91bf:case 0x91f5:case 0x9201:case 0x9236:case 0x9244:case 0x924b:case 0x9250:case 0x9255:case 0x92f3:c->X=(uint16_t)(c->X+1);decode_nz(c,c->X,16);break;
  case 0x9185:case 0x9189:case 0x91c8:case 0x91cd:case 0x91f6:case 0x91fb:case 0x9202:case 0x9207:case 0x9210:case 0x9237:case 0x92e8:case 0x92f4:case 0x92f9:cpu_write16_timed(c,0,r[(pc&32767)+1],c->X);wrote=1;break;
  case 0x918d:next=0x92e5;break;
  case 0x9195:case 0x91a8:case 0x91bb:case 0x91e8:case 0x91ef:case 0x9232:case 0x9247:case 0x924c:case 0x9251:case 0x9256:case 0x92ef:{unsigned at=pc&32767,base=r[at+1]|r[at+2]<<8|r[at+3]<<16;decode_store(c,base);wrote=1;break;}
  case 0x919b:case 0x91ae:case 0x91c1:{unsigned at=r[(pc&32767)+1],v=(cpu_read8_timed(c,0,at)-1)&255;cpu_write8_timed(c,0,at,v);wrote=1;decode_nz(c,v,8);break;}
  case 0x919f:case 0x91b2:case 0x9214:decode_a(c,cpu_read8_timed(c,0,r[(pc&32767)+1]));break;
  case 0x91c7:case 0x91fa:case 0x920f:case 0x92e7:case 0x92f8:c->X=(uint16_t)(c->X-1);decode_nz(c,c->X,16);break;
  case 0x91cf:case 0x91d1:case 0x91d3:case 0x91d5:case 0x9209:case 0x920b:cpu_write8_timed(c,0,r[(pc&32767)+1],0);wrote=1;break;
  case 0x91d9:case 0x91e0:case 0x9227:case 0x923d:{unsigned at=pc&32767,base=r[at+1]|r[at+2]<<8|r[at+3]<<16;decode_a(c,decode_long(c,base));break;}
  case 0x91df:c->X=c->A;decode_nz(c,c->X,16);break;
  case 0x91ec:case 0x922f:decode_a(c,c->X);break;
  case 0x9218:next=0x9300;break;
  case 0x921b:next=0x915d;break;
  case 0x9239:next=0x920d;break;
  case 0x9243:case 0x9300:case 0x9302:c->X=cpu_read16_timed(c,0,c->S+1);c->S+=2;decode_nz(c,c->X,16);break;
  case 0x925a:cpu_write8_timed(c,0,c->S,c->X>>8);--c->S;cpu_write8_timed(c,0,c->S,c->X);--c->S;wrote=1;break;
  case 0x925b:next=0x9297;break;
  case 0x92fd:next=0x9214;break;
  case 0x9301:c->Y=cpu_read16_timed(c,0,c->S+1);c->S+=2;decode_nz(c,c->Y,16);break;
  default:break;
  }
  if(pc>=0x925d&&pc<0x92bc)++decode_stats.chain;
  if(extra)++decode_stats.input64;
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
static void decode_exit(void){if(decode_report)fprintf(stderr,"[native-decode63] armed=%llu clear=%llu chain=%llu output=%llu instructions=%llu deadlines=%llu refused=%llu input64=%llu raw64=%llu\n",
 (unsigned long long)decode_stats.armed,(unsigned long long)decode_stats.clear,(unsigned long long)decode_stats.chain,
 (unsigned long long)decode_stats.output,(unsigned long long)decode_stats.instructions,(unsigned long long)decode_stats.deadlines,(unsigned long long)decode_stats.refused,(unsigned long long)decode_stats.input64,(unsigned long long)decode_stats.raw64);}
static void decode_hook(CpuState*c,uint32_t pc){
 if(!decode_enabled||c!=&g_cpu)return;
 if(!interp_bridge_source_opcode_available(c)){decode_witness.armed=0;return;}
 for(unsigned k=0x1143;k<0x1304;++k)if(!cpu_aot_rom_mapping_matches(0x8000u+k,k,1)){decode_witness.armed=0;return;}
 uint32_t landing;if(LagoonNativeDecode63_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),decode_frame,pc,&landing))interp_bridge_pre_opcode_redirect(landing);
}
void LagoonNativeDecode63_BeforeFrame(void){
 if(decode_enabled<0){const char*timing=getenv("LAGOON_TIMING64");decode64_enabled=!timing||strcmp(timing,"0");const char*v=getenv("LAGOON_NATIVE_DECODE63");decode_enabled=!v||!strcmp(v,"on");v=getenv("LAGOON_NATIVE_WAIT_REPORT");decode_report=v&&!strcmp(v,"1");if(decode_report)atexit(decode_exit);}
 decode_frame=RtlStateGeneration();if(decode_witness.armed&&decode_witness.generation!=decode_frame)decode_witness.armed=0;
 extern void (*g_lagoon_decode_source_observer)(CpuState*,uint16_t,uint8_t,uint32_t);
 g_lagoon_decode_source_observer=decode_enabled?LagoonNativeDecode63_ObserveEntry:NULL;
 if(decode_enabled){interp_bridge_add_pre_opcode_hook(0x9143,decode_hook);interp_bridge_add_pre_opcode_hook(0x9303,decode_hook);
  for(unsigned k=0;k<sizeof decode_ops/sizeof *decode_ops;++k)interp_bridge_add_pre_opcode_hook(decode_ops[k][0],decode_hook);
  for(unsigned k=0;k<sizeof decode64_ops/sizeof *decode64_ops;++k)interp_bridge_add_pre_opcode_hook(decode64_ops[k][0],decode_hook);}
}
#endif
