/* Three original port writers. Preserve the existing interpreter's observable
 * opcode-start CPU/master point at STA $2140..2; no shared clock is rewound.
 * Full instruction totals, physical RTL and all unknown contexts stay original. */
#include "lagoon_native_music.h"
#include "lagoon_native_endgame.h"
#include "common_rtl.h"
#include "cpu_trace.h"
static uint64_t source_hash(const uint8_t *rom,unsigned pc,unsigned n) {
  unsigned at=((pc>>16)&127)*32768+(pc&32767);
  uint64_t h=UINT64_C(1469598103934665603);
  for(unsigned i=0;i<n;i++)h=(h^rom[at+i])*UINT64_C(1099511628211);
  return h;
}
static unsigned word(const uint8_t *r){return r[0]|r[1]<<8;}
static void a8(CpuState*c,unsigned a){c->A=(c->A&0xff00)|(a&255);
 c->_flag_Z=!(a&255);c->_flag_N=!!(a&128);
 c->P=(c->P&~0x82)|(c->_flag_Z?2:0)|(c->_flag_N?128:0);}
static const unsigned targets[]={0x0cfe00,0x0cfe2f,0x0cfe5e};
static const uint16_t calls[3][4]={{0xf862,0xf882,0xf8ab,0xf8dd},
 {0xf872,0xf88b,0xf8c4,0xf8e6},{0xf89b,0xf8b4,0xf8cd,0xf8ef}};
static const uint64_t hashes[]={UINT64_C(0x22CA34149D19DC99),
 UINT64_C(0x110EB90AA3981273),UINT64_C(0xF6A2E0D71171A5F5)};
bool LagoonNativeMusic_Try(const uint8_t*rom,size_t size,CpuState*c,
    const CpuState*current,uint64_t generation,uint64_t frame,
    unsigned w,uint16_t entry_s,uint8_t hrv,uint32_t host_return,RecompReturn*out){
 if(!rom||size!=0x100000||!c||c!=current||!c->ram||!out||w>=3||!frame||generation!=frame||
    c->PB!=0x0c||c->DB!=1||c->D||!c->m_flag||c->x_flag||c->emulation||c->_flag_D||
    (c->P&(CPU_P_M|CPU_P_X|CPU_P_D))!=CPU_P_M||c->S!=entry_s||hrv!=3||
    c->host_return_valid!=3||c->S<0x100||c->S>0x1ffc||c->ram[c->S+3]!=0x0c||
    (0x43au+w>c->S&&0x43au+w<=c->S+3u))return false;
 unsigned known=0;
 for(unsigned i=0;i<4;i++)known|=host_return==(0x0c0000u|((unsigned)calls[w][i]+4));
 if(!known||word(c->ram+c->S+1)!=((host_return-1)&65535)||
    source_hash(rom,0x0cf800,254)!=UINT64_C(0x3648F5416CCF0CAF)||
    source_hash(rom,targets[w],14)!=hashes[w])return false;
 LagoonNativeEndgameConsumeMusic(w);
 unsigned pc=targets[w];
#define BEGIN(off) do {cpu_instruction_timing_commit(c);  if(interp_bridge_lle_master_deadline_reached(c)){*out=interp_bridge_lle_yield_unwind(c,pc+(off));return true;}  cpu_trace_block(c,pc+(off));c->coprocessor_master_cycles=c->master_cycles;}while(0)
#define CHARGE(cyc,bytes) do{c->cycles+=(cyc);c->master_cycles+=(cyc)*6+(bytes)*2;}while(0)
 BEGIN(0);CHARGE(3,2);c->open_bus=0x20; /* SEP20: alreadyM1; other flags unchanged. */
 BEGIN(2);CHARGE(3,1);cpu_write8_timed(c,0,c->S,(uint8_t)c->A);--c->S;
 BEGIN(3);CHARGE(2,2);c->open_bus=0;a8(c,0);
 BEGIN(5);CHARGE(3,1);c->open_bus=0xeb;c->A=(c->A>>8)|(c->A<<8);a8(c,c->A);
 BEGIN(6);CHARGE(4,1);a8(c,cpu_read8_timed(c,0,++c->S));
 BEGIN(7);
 /* The original interpreter publishes the pre-op CPU/master state during
  * this bus write and adds all30 master clocks after runOpcode returns.
  * Keep that exact observable policy locally. No fake cycles or clock rewind. */
 cpu_write8(c,c->DB,(uint16_t)(0x2140+w),(uint8_t)c->A);CHARGE(4,3);
 BEGIN(10);CHARGE(4,3);cpu_write8_timed(c,c->DB,(uint16_t)(0x43a+w),(uint8_t)c->A);
 BEGIN(13);CHARGE(6,1);
 ++c->S;unsigned lo=cpu_read8_timed(c,0,c->S);
 ++c->S;unsigned hi=cpu_read8_timed(c,0,c->S);
 ++c->S;unsigned bank=cpu_read8_timed(c,0,c->S);
 /* Exact physical frame was pinned and no instruction writes aboveentryS.
  * RTL consumes that actual frame; the paired C caller owns the same return. */
 unsigned actual_return=(bank<<16)|(((lo|(hi<<8))+1u)&0xffffu);
 c->PB=(uint8_t)bank;
 if(actual_return!=host_return){cpu_instruction_timing_commit(c);*out=interp_tier_dispatch_rewritten_return(c,actual_return,pc+13);return true;}
 cpu_instruction_timing_commit(c);*out=RECOMP_RETURN_NORMAL;return true;
#undef BEGIN
#undef CHARGE
}
#ifndef LAGOON_NATIVE_MUSIC_TEST
static uint64_t frame_generation;
void LagoonNativeMusic_BeforeFrame(void){frame_generation=RtlStateGeneration();}
static bool run(CpuState*c,uint16_t s,uint8_t h,uint32_t host,RecompReturn*out,unsigned w){
 if(!c||!out)return false;
 if(LagoonNativeMusic_Try(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),frame_generation,w,s,h,host,out))return true;
 /* Do not fall through the known different AOT port-observation policy.
  * Existing bridge owns this real entry and inherited frame in unknown cases. */
 LagoonNativeEndgameRefuseMusic(w);
 *out=interp_tier_dispatch_tail(c,targets[w],targets[w],s,h);return true;
}
bool LagoonNativeMusic_Port0(CpuState*c,uint16_t s,uint8_t h,uint32_t p,RecompReturn*out){return run(c,s,h,p,out,0);}
bool LagoonNativeMusic_Port1(CpuState*c,uint16_t s,uint8_t h,uint32_t p,RecompReturn*out){return run(c,s,h,p,out,1);}
bool LagoonNativeMusic_Port2(CpuState*c,uint16_t s,uint8_t h,uint32_t p,RecompReturn*out){return run(c,s,h,p,out,2);}
#endif
