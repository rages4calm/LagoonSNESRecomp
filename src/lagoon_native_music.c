/* Three original port writers. Preserve the existing interpreter's observable
 * opcode-start CPU/master point at STA $2140..2; no shared clock is rewound.
 * Full instruction totals, physical RTL and all unknown contexts stay original. */
#include "lagoon_native_music.h"
#include "lagoon_native_endgame.h"
#include "common_rtl.h"
#include "cpu_trace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint64_t source_hash(const uint8_t *rom,unsigned pc,unsigned n) {
  unsigned at=((pc>>16)&127)*32768+(pc&32767);
  uint64_t h=UINT64_C(1469598103934665603);
  for(unsigned i=0;i<n;i++)h=(h^rom[at+i])*UINT64_C(1099511628211);
  return h;
}
static unsigned word(const uint8_t *r){return r[0]|r[1]<<8;}
static bool music_timeline_valid(const CpuState*c){return c&&c==&g_cpu&&rtl_apu_absolute_periodic_enabled()&&c->master_cycles>=g_apu_last_sync_master;}
static void a8(CpuState*c,unsigned a){c->A=(c->A&0xff00)|(a&255);
 c->_flag_Z=!(a&255);c->_flag_N=!!(a&128);
 c->P=(c->P&~0x82)|(c->_flag_Z?2:0)|(c->_flag_N?128:0);}
static const unsigned targets[]={0x0cfe00,0x0cfe2f,0x0cfe5e};
static const uint16_t calls[3][4]={{0xf862,0xf882,0xf8ab,0xf8dd},
 {0xf872,0xf88b,0xf8c4,0xf8e6},{0xf89b,0xf8b4,0xf8cd,0xf8ef}};
static const uint64_t hashes[]={UINT64_C(0x22CA34149D19DC99),
 UINT64_C(0x110EB90AA3981273),UINT64_C(0xF6A2E0D71171A5F5)};
bool LagoonNativeMusic_Try(const uint8_t*rom,size_t size,CpuState*c,
    const CpuState*current,uint64_t generation,uint64_t frame,bool frame_valid,
    unsigned w,uint16_t entry_s,uint8_t hrv,uint32_t host_return,RecompReturn*out){
 if(!rom||size!=0x100000||!c||c!=current||!c->ram||!out||w>=3||!frame_valid||generation!=frame||
    c->PB!=0x0c||(c->DB!=1&&c->DB!=2)||c->D||!c->m_flag||c->x_flag||c->emulation||c->_flag_D||
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
#include "lagoon_native_music_upload.h"

/* F8FE's finite port0 toggle writer has a distinct physical caller family. */
bool LagoonNativeMusic_ToggleTry(const uint8_t*rom,size_t size,CpuState*c,
 const CpuState*current,uint64_t gen,uint64_t frame,bool frame_valid,uint16_t entry_s,uint8_t hrv,uint32_t host,RecompReturn*out){
 if(!rom||size!=0x100000||!c||c!=current||!c->ram||!out||!frame_valid||gen!=frame||c->PB!=12||(c->DB!=1&&c->DB!=2)||c->D||!c->m_flag||c->x_flag||c->emulation||c->_flag_D||(c->P&(CPU_P_M|CPU_P_X|CPU_P_D))!=CPU_P_M||c->S!=entry_s||hrv!=3||c->host_return_valid!=3||c->S<0x100||c->S>0x1ffc||c->ram[c->S+3]!=12||word(c->ram+c->S+1)!=((host-1)&65535))return false;
 unsigned known=host==0xcf964||host==0xcf984||host==0xcf9ad||host==0xcf9df;
 if(!known||source_hash(rom,0xcf8fe,254)!=UINT64_C(0xd7a217c3ed76d407)||source_hash(rom,0xcfe0e,33)!=UINT64_C(0xe6e99bbd9323d9ef)||(0x430>c->S&&0x430<=c->S+3u)||(0x43a>c->S&&0x43a<=c->S+3u))return false;
 LagoonNativeEndgameConsumeMusic(6);
#define TB(at,n) do{if(!upload_begin(rom,c,at,n,out))return true;}while(0)
#define TC(cyc,n) upload_charge(c,cyc,n)
#define TS(at,cyc,n,stmt) do{TB(at,n);stmt;TC(cyc,n);}while(0)
 TS(0xcfe0e,3,2,c->P|=CPU_P_M;cpu_p_to_mirrors(c));TS(0xcfe10,3,1,upload_push(c,c->A));TS(0xcfe11,2,2,a8(c,0));TS(0xcfe13,3,1,c->A=(c->A>>8)|(c->A<<8);a8(c,c->A));TS(0xcfe14,4,1,a8(c,upload_pull(c)));
 TS(0xcfe15,c->_flag_Z?3:2,2,(void)0);
 if(!c->_flag_Z){TS(0xcfe17,2,2,a8(c,c->A&63));TS(0xcfe19,4,3,a8(c,c->A|cpu_read8_timed(c,c->DB,0x430)));TS(0xcfe1c,3,1,upload_push(c,c->A));TS(0xcfe1d,4,3,a8(c,cpu_read8_timed(c,c->DB,0x430)));TS(0xcfe20,2,2,a8(c,c->A^64));TS(0xcfe22,2,2,a8(c,c->A&64));TS(0xcfe24,4,3,cpu_write8_timed(c,c->DB,0x430,c->A));TS(0xcfe27,4,1,a8(c,upload_pull(c)));}
 TS(0xcfe28,4,3,cpu_write8(c,c->DB,0x2140,c->A));TS(0xcfe2b,4,3,cpu_write8_timed(c,c->DB,0x43a,c->A));TB(0xcfe2e,1);TC(6,1);*out=upload_return(c,3,host,0xcfe2e);return true;
#undef TB
#undef TC
#undef TS
}
#ifndef LAGOON_NATIVE_MUSIC_TEST
/* Cold generation0 is valid; registration is a separate authority bit. */
static uint64_t frame_generation;
static bool frame_registered;
void LagoonNativeMusic_BeforeFrame(void){frame_generation=RtlStateGeneration();frame_registered=true;}
static void refusal_trace(CpuState*c,uint16_t entry_s,uint8_t h,uint32_t host,unsigned w){
 static int initialized,enabled;static unsigned mask;
 if(!initialized){const char*v=getenv("LAGOON_NATIVE_MUSIC_TRACE");enabled=v&&!strcmp(v,"1");initialized=1;}
 if(!enabled||w>=7||(mask&(1u<<w))||!c)return;mask|=1u<<w;
 unsigned physical=0,pointer=0;if(c->ram){if(c->S<=0x1ffcu)physical=word(c->ram+c->S+1)|(c->ram[c->S+3]<<16);pointer=c->ram[0x1c]|c->ram[0x1d]<<8|c->ram[0x1e]<<16;}
 fprintf(stderr,"[music_guard] service=%u current=%u registered=%u mapped=%u generation=%llu frame_generation=%llu PB=%02X DB=%02X D=%04X M=%u X=%u E=%u P=%02X decimal=%u S=%04X entryS=%04X hrv=%u actual_hrv=%u host=%06X physical=%06X A=%04X pointer=%06X f800=%016llX f8fe=%016llX upload=%016llX\n",w,c==&g_cpu,frame_registered,rtl_apu_absolute_periodic_enabled(),(unsigned long long)RtlStateGeneration(),(unsigned long long)frame_generation,c->PB,c->DB,c->D,c->m_flag,c->x_flag,c->emulation,c->P,c->_flag_D,c->S,entry_s,h,c->host_return_valid,host,physical,c->A,pointer,g_rom?(unsigned long long)source_hash(g_rom,0xcf800,254):0,g_rom?(unsigned long long)source_hash(g_rom,0xcf8fe,254):0,g_rom?(unsigned long long)source_hash(g_rom,0xcfe9b,357):0);
}

/* Unmapped boot owns private bridge pending/progress APU accounting. Never
 * substitute raw title bus accesses there, even at a registered generation0.
 * Pure Try APIs above qualify source semantics only; live admission is mapped. */
static bool run(CpuState*c,uint16_t s,uint8_t h,uint32_t host,RecompReturn*out,unsigned w){
 if(!c||!out)return false;
 if(music_timeline_valid(c)&&LagoonNativeMusic_Try(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),frame_generation,frame_registered,w,s,h,host,out))return true;
 /* Do not fall through the known different AOT port-observation policy.
  * Existing bridge owns this real entry and inherited frame in unknown cases. */
 refusal_trace(c,s,h,host,w);LagoonNativeEndgameRefuseMusic(w);
 *out=interp_tier_dispatch_tail(c,targets[w],targets[w],s,h);return true;
}
bool LagoonNativeMusic_Port0(CpuState*c,uint16_t s,uint8_t h,uint32_t p,RecompReturn*out){return run(c,s,h,p,out,0);}
bool LagoonNativeMusic_Port1(CpuState*c,uint16_t s,uint8_t h,uint32_t p,RecompReturn*out){return run(c,s,h,p,out,1);}
bool LagoonNativeMusic_Port2(CpuState*c,uint16_t s,uint8_t h,uint32_t p,RecompReturn*out){return run(c,s,h,p,out,2);}
#endif

#ifndef LAGOON_NATIVE_MUSIC_TEST
static bool upload_run(CpuState*c,uint16_t s,uint8_t h,uint32_t host,RecompReturn*out,unsigned service){
 static const unsigned targets[]={0x0cfe9b,0x0cfeec,0x0cff69};
 if(!c||!out)return false;
 if(music_timeline_valid(c)&&LagoonNativeMusic_UploadTry(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),frame_generation,frame_registered,service,s,h,host,out))return true;
 refusal_trace(c,s,h,host,service+3);LagoonNativeEndgameRefuseMusic(service+3);*out=interp_tier_dispatch_tail(c,targets[service],targets[service],s,h);return true;
}
bool LagoonNativeMusic_Bank(CpuState*c,uint16_t s,uint8_t h,uint32_t p,RecompReturn*out){return upload_run(c,s,h,p,out,0);}
bool LagoonNativeMusic_Samples(CpuState*c,uint16_t s,uint8_t h,uint32_t p,RecompReturn*out){return upload_run(c,s,h,p,out,1);}
bool LagoonNativeMusic_Transfer(CpuState*c,uint16_t s,uint8_t h,uint32_t p,RecompReturn*out){return upload_run(c,s,h,p,out,2);}
#endif

#ifndef LAGOON_NATIVE_MUSIC_TEST
bool LagoonNativeMusic_Toggle(CpuState*c,uint16_t s,uint8_t h,uint32_t p,RecompReturn*out){if(!c||!out)return false;if(music_timeline_valid(c)&&LagoonNativeMusic_ToggleTry(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),frame_generation,frame_registered,s,h,p,out))return true;refusal_trace(c,s,h,p,6);LagoonNativeEndgameRefuseMusic(6);*out=interp_tier_dispatch_tail(c,0x0cfe0e,0x0cfe0e,s,h);return true;}
#endif
