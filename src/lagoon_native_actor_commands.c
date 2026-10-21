/* Ordinary enemy direction/velocity dispatch on the host. The complete source
 * prefix is RAM/ROM-only; original AI, collision, jumps, NMI, audio and actual
 * terminal RTL remain in their original owners. Unknown selectors/contexts run
 * unchanged through the existing interpreter, including the global80E6 floor. */
#include "lagoon_native_actor_commands.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef struct SourceSpan{unsigned pc,size;uint64_t hash;}SourceSpan;
static const SourceSpan spans[]={
 {0x0080b9,0x30,UINT64_C(0x37f9de38ea6c1480)},
 {0x018184,0x37,UINT64_C(0x23b0dcfba0bcb3d3)},
 {0x028000,0x42,UINT64_C(0x07ed9f99340b9416)},
 {0x0280a0,0x89,UINT64_C(0x789757cb6332dfac)},
 {0x028129,0x103,UINT64_C(0xf9986ec0b2b32cab)},
 /* Actual main-loop JSR8184 return is80BD, before the collision/A4D3 owners. */
 {0x0180af,0x18,UINT64_C(0x027bbc2ea8acdea8)}
};
static unsigned word(const uint8_t*r,unsigned a){return r[a]|r[a+1]<<8;}
static void put(uint8_t*r,unsigned a,unsigned v){r[a]=(uint8_t)v;r[a+1]=(uint8_t)(v>>8);}
static uint64_t hash(const uint8_t*p,unsigned n){uint64_t h=UINT64_C(14695981039346656037);while(n--)h=(h^*p++)*UINT64_C(1099511628211);return h;}
int LagoonNativeActorCommands_Source(const uint8_t*rom,size_t size){
 if(!rom||size!=0x100000)return 0;
 for(unsigned i=0;i<sizeof spans/sizeof*spans;i++){
  unsigned p=((spans[i].pc>>16)&127)*32768+(spans[i].pc&32767);
  if(hash(rom+p,spans[i].size)!=spans[i].hash)return 0;
 }return 1;
}
static int mode(const CpuState*c){return c&&c->ram&&c->PB==0&&c->DB==1&&!c->D&&!c->emulation&&c->m_flag&&!c->x_flag&&!c->_flag_D&&(c->P&0x38)==0x20&&c->S>=0x104&&c->S<=0x1ffa&&c->X>=0x40&&c->X<=0x800&&!(c->X&63);}
int LagoonNativeActorCommands_Apply(const uint8_t*rom,size_t size,uint8_t*r,CpuState*c,unsigned selector){
 if(!r||!mode(c)||c->ram!=r||(selector!=0x11&&selector!=0x13)||c->A!=selector||!LagoonNativeActorCommands_Source(rom,size))return 0;
 unsigned a=0x500+c->X,result,carry=0,overflow=c->_flag_V,y=c->Y;
 unsigned target=selector==0x11?0x80a0:0x80ea;
 /* Original dispatcher clears hiddenA with AND00FF. ASL16(selector) setsC0.
  * Only the selected original command's LSR/ADC can subsequently alterC/V. */
 if(selector==0x11){
  unsigned intent;
  if(r[a+1]&0x20){carry=r[a+10]&1;intent=r[a+0x3f]&(carry?0x0c:3);}
  else intent=(r[a+1]&3)?0:r[a+0x3f]&15;
  if(!intent)result=255;
  else{y=intent;result=rom[0x100da+intent];if(result&128)result=255;else r[a+10]=(uint8_t)result;}
  r[a+11]=r[a+25]=(uint8_t)result;
 }else{
  result=r[a+11];
  if(result&128)memset(r+a+6,0,4);
  else{
   unsigned tier=(r[a+26]<<4)&255,left=(result<<2)&255,sum=tier+left;
   r[0x1033]=(uint8_t)tier;y=sum&255;carry=sum>255;
   overflow=!!((~(left^tier)&(left^y))&128);
   for(unsigned q=0;q<4;q++)r[a+6+q]=rom[0x10129+y+q];
   result=rom[0x10129+y+3];
  }
 }
 r[0x13]=(uint8_t)selector;r[0x12]=2;put(r,0x10,target);
 /* PHB, PHY/PLY, nestedJSL80E6 and handlerRTL leave exactly these four
  * dead bytes. CallerJSL and underlyingJSR bytes aboveS remain untouched. */
 r[c->S]=1;r[c->S-1]=0;r[c->S-2]=0x80;r[c->S-3]=0xe3;
 c->A=(uint16_t)result;c->Y=(uint16_t)y;c->_flag_C=(uint8_t)carry;c->_flag_V=(uint8_t)overflow;
 c->_flag_N=c->_flag_Z=0;c->P=(uint8_t)((c->P&~0xc3)|(carry?1:0)|(overflow?0x40:0));
 c->open_bus=1; /* Final original PLB reads its savedDB=1. */
 return 1;
}
int LagoonNativeActorCommands_Try(const uint8_t*rom,size_t size,CpuState*c,uint64_t generation,uint64_t frame_generation,int normal_world){
 if(!mode(c)||generation!=frame_generation||!normal_world)return 0;
 uint8_t*r=c->ram;unsigned selector=c->A;
 if(selector!=0x11&&selector!=0x13)return 0;
 if(word(r,c->S+1)!=(selector==0x11?0x81af:0x81b9)||r[c->S+3]!=1||word(r,c->S+4)!=0x80bd||
   r[0x518+c->X]!=2||(r[0x500+c->X]&0xc0)!=0xc0||(r[0x500]&0xc0)!=0xc0||
   !r[0x520]||r[0x521]||r[0x406]||r[0x100e]||r[0x23d]!=15||r[0x275]!=0x81||word(r,0x48e)>0x4a)return 0;
 return LagoonNativeActorCommands_Apply(rom,size,r,c,selector);
}
#ifndef LAGOON_NATIVE_ACTOR_COMMANDS_TEST
#include "common_rtl.h"
#include "snes/interp_bridge.h"
#include "snes/ppu.h"
static uint64_t frame_generation;
static unsigned long long seen,applied;static int enabled=-1,report_enabled;
static void report(void){if(report_enabled)fprintf(stderr,"[actor-commands] seen=%llu applied=%llu enabled=%d scope=ordinary-enemy2-selector11-13\n",seen,applied,enabled);}
static void hook(CpuState*c,uint32_t pc){
 if(!enabled||c!=&g_cpu||(pc&0x7fffff)!=0x0080b9||!g_ppu)return;
 seen++;
 if(LagoonNativeActorCommands_Try(g_rom,0x100000,c,RtlStateGeneration(),frame_generation,!PPU_forcedBlank(g_ppu))){applied++;interp_bridge_pre_opcode_redirect(0x0080e5);}
}
void LagoonNativeActorCommands_BeforeFrame(void){
 if(enabled<0){const char*v=getenv("LAGOON_ACTOR_COMMANDS");enabled=!v||!strcmp(v,"auto")||!strcmp(v,"on");v=getenv("LAGOON_ACTOR_COMMANDS_REPORT");report_enabled=v&&!strcmp(v,"1");atexit(report);}
 frame_generation=RtlStateGeneration();
 if(enabled)interp_bridge_add_pre_opcode_hook(0x0080b9,hook);
}
#else
void LagoonNativeActorCommands_BeforeFrame(void){}
#endif
