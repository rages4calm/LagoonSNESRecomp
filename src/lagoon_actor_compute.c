/* Compute the original per-enemy RAM leaves on the host. This is deliberately
 * scoped to normal cave/Elf Field gameplay (rooms03..06 and08..0A),
 * not an emulator overclock.
 * No AI, collision, tick, DMA, NMI or audio work is skipped or fabricated. */
#include "lagoon_actor_compute.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static unsigned word(const uint8_t*r,unsigned q){return r[q]|r[q+1]<<8;}
static void put(uint8_t*r,unsigned q,unsigned v){r[q]=v;r[q+1]=v>>8;}
static uint64_t hash(const uint8_t*r,unsigned n){uint64_t h=UINT64_C(14695981039346656037);while(n--)h=(h^*r++)*UINT64_C(1099511628211);return h;}
static void flag(CpuState*c,unsigned bit,unsigned value){c->P=(c->P&~bit)|(value?bit:0);}
static int valid(const CpuState*c){return c&&c->m_flag&&!c->x_flag&&!c->emulation&&!c->D&&!c->_flag_D&&!(c->P&8)&&c->PB==1&&c->DB==1&&c->S>=0x104&&c->S<=0x1fb&&c->X<=0x800&&!(c->X&63);}
int LagoonActorCompute_Apply(unsigned kind,uint8_t*r,CpuState*c){
 if(kind>1||!r||!valid(c))return 0;
 unsigned x=c->X,a=word(r,0x502+x),b=word(r,0x504+x),result;
 if(!kind){
  if(a<16)a=16;else if(a>=word(r,0x492))a=word(r,0x492);
  put(r,0x502+x,a);
  unsigned carry=b>=16;if(b<16)b=16;else{carry=b>=word(r,0x494);if(carry)b=word(r,0x494);}
  put(r,0x504+x,b);result=b&255;c->_flag_C=carry;flag(c,1,carry);
 }else{
  unsigned cx=word(r,0x480),cy=word(r,0x482),dx=(a-cx)&65535,dy=(b-cy)&65535;
  put(r,0x50c+x,dx);put(r,0x50e + x,dy);
  /* Original PHA16 order and all subsequent PLA/PHA pairs leave these exact
   * four dead stack bytes, despite ending with the original S. */
  put(r,c->S-1,dx);put(r,c->S-3,dy);
  unsigned bits=word(r,0xa4);
  if((dy-0x120)&0x8000)bits|=4;
  if((dy-0xb0)&0x8000)bits|=0x400;
  if(!((dy-0xffe0)&0x8000))bits|=8;
  if(!((dy-0x50)&0x8000))bits|=0x800;
  if((dx-0x160)&0x8000)bits|=1;
  if((dx-0xc0)&0x8000)bits|=0x100;
  if(!((dx-0xffa0)&0x8000))bits|=2;
  unsigned final=!((dx-0x40)&0x8000);if(final)bits|=0x200;
  put(r,0xa4,bits);result=(final?0x200:dx)&255;
  c->_flag_C=dx>=0x40;flag(c,1,c->_flag_C);
  c->_flag_V=!!((b^cy)&(b^dy)&0x8000);flag(c,0x40,c->_flag_V);
 }
 c->A=result;c->_flag_N=0;c->_flag_Z=!result;flag(c,0x80,0);flag(c,2,!result);return 1;
}
bool LagoonActorCompute_Run(unsigned kind,const uint8_t*rom,size_t size,uint8_t*r,CpuState*c,uint16_t entry_s,uint8_t hrv,uint32_t host,RecompReturn*out,int normal_world){
 if(kind>1||!rom||size!=0x100000||!r||!out||!valid(c)||!normal_world||c->S!=entry_s||hrv!=2||c->host_return_valid!=2||c->X<0x40)return false;
 unsigned room=word(r,0x48e);
 if(!((room>=3&&room<=6)||(room>=8&&room<=10)))return false;
 unsigned s=c->S,ret=kind?0xa4e7:0xa4d8;
 if(host!=(0x010000|((ret+1)&65535))||word(r,s+1)!=ret||word(r,s+3)!=0x80c6||r[0x406]||r[0x100e]||r[0x23d]!=15||r[0x275]!=0x81||!r[0x520]||r[0x521]||(r[0x500+c->X]&0xc0)!=0xc0||r[0x518+c->X]!=2)return false;
 if(hash(rom+0xa4d3,0x18)!=UINT64_C(0x09abeb222846112b)||hash(rom+(kind?0xa5aa:0xa56a),kind?0x7a:0x40)!=(kind?UINT64_C(0x55f5e93f5e4ec5f7):UINT64_C(0x8d84ee94fd1f84c5)))return false;
 if(!LagoonActorCompute_Apply(kind,r,c))return false;
 /* Preserve the selected precise leaf's actual RTS: six CPU cycles, opcode
  * fetch plus two slow-WRAM reads =42 master clocks. Only pure computation
  * moves to the host; no elapsed-clock subtraction or guest-frame synthesis. */
 c->coprocessor_master_cycles=c->master_cycles;c->cycles+=6;c->master_cycles+=42;
 cpu_read8(c,0,++c->S);cpu_read8(c,0,++c->S);*out=RECOMP_RETURN_NORMAL;return true;
}
#ifndef LAGOON_ACTOR_COMPUTE_TEST
#include "common_rtl.h"
#include "snes/ppu.h"
static uint64_t calls[2];static int mode=-1,report;
static void summary(void){if(report)fprintf(stderr,"[actor-compute] clamp=%llu project=%llu scope=rooms03-06-and08-0A-enemy2 enabled=%d\n",(unsigned long long)calls[0],(unsigned long long)calls[1],mode);}
static bool run(unsigned kind,CpuState*c,uint16_t s,uint8_t h,uint32_t pc,RecompReturn*out){
 if(mode<0){const char*v=getenv("LAGOON_ACTOR_COMPUTE");mode=!v||!strcmp(v,"auto")||!strcmp(v,"on");const char*t=getenv("LAGOON_ACTOR_COMPUTE_REPORT");report=t&&!strcmp(t,"1");atexit(summary);}
 if(!mode||c!=&g_cpu||!g_ppu)return false;
 bool ok=LagoonActorCompute_Run(kind,g_rom,0x100000,g_ram,c,s,h,pc,out,!PPU_forcedBlank(g_ppu));if(ok)calls[kind]++;return ok;
}
bool LagoonActorCompute_Clamp(CpuState*c,uint16_t s,uint8_t h,uint32_t pc,RecompReturn*out){return run(0,c,s,h,pc,out);}
bool LagoonActorCompute_Project(CpuState*c,uint16_t s,uint8_t h,uint32_t pc,RecompReturn*out){return run(1,c,s,h,pc,out);}
#endif
