/* Original cold-reset clear window. No call/return or guessed owner frame.
 * Witness the actual forced-blank/NMI-disable prefix, then execute only the
 * twelve source-owned RAM clear opcodes. All other contexts stay interpreted. */
#include "lagoon_native_boot62.h"
#include "snes/interp_bridge.h"
#include <string.h>
#include <stdlib.h>
static struct {const CpuState *cpu;const uint8_t *ram;uint64_t generation;
 uint32_t next;unsigned phase;} witness;
static struct {uint64_t armed,instructions,complete,deadlines,refused,observed;} stats;
static int pace_allowed=1;
int LagoonNativeBoot62_Source(const uint8_t *rom,size_t size){
 if(!rom||size!=0x100000)return 0;
 uint64_t h=UINT64_C(14695981039346656037);
 for(unsigned n=0x13;n<0x59;++n)h=(h^rom[n])*UINT64_C(1099511628211);
 return h==UINT64_C(0xdd1e01c4ed439698);
}
static int mode(const CpuState*c){
 return c&&c->ram&&c->PB==0&&c->DB==0&&c->D==0&&c->S==0x1ff&&
  !c->emulation&&c->m_flag&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&
  (c->P&0x3c)==0x24&&!c->host_return_valid;
}
static void nz(CpuState*c,unsigned v,unsigned sign){
 c->_flag_Z=!v;c->_flag_N=!!(v&sign);cpu_mirrors_to_p(c);
}
int LagoonNativeBoot62_Process(const uint8_t*rom,size_t size,CpuState*c,
 const CpuState*current,uint64_t generation,uint64_t frame,uint32_t pc,uint32_t*landing){
 if(!landing||c!=current||generation!=frame||!mode(c)||!LagoonNativeBoot62_Source(rom,size)){
  witness.phase=0;++stats.refused;return 0;
 }
 if(pc==0x8033){
  witness.phase=0;
  if(c->X!=0x1ff||c->A||c->master_cycles>65536)return 0;
  witness.cpu=c;witness.ram=c->ram;witness.generation=generation;
  witness.next=0x8035;witness.phase=1;return 0;
 }
 if(!witness.phase||witness.cpu!=c||witness.ram!=c->ram||witness.generation!=generation||witness.next!=pc){
  witness.phase=0;++stats.refused;return 0;
 }
 if(witness.phase==1){
  if(pc==0x8035&&c->A==0x80)witness.next=0x8038;
  else if(pc==0x8038&&c->A==0x80)witness.next=0x803a;
  else if(pc==0x803a&&!c->A)witness.next=0x803d;
  else if(pc==0x803d&&!c->A&&c->X==0x1ff){witness.phase=2;++stats.armed;}
  else {witness.phase=0;++stats.refused;}
  if(witness.phase!=2)return 0;
 }
 if(!pace_allowed){
  /* Witness original execution until the shared absolute APU cadence is ready.
   * No CPU/RAM/clock changes on this observation-only path. */
  unsigned next;
  switch(pc){
  case 0x803d:next=0x8040;break;case 0x8040:next=0x8042;break;
  case 0x8042:next=0x8045;break;case 0x8045:next=0x8046;break;
  case 0x8046:next=0x8049;break;case 0x8049:next=c->_flag_Z?0x804b:0x8042;break;
  case 0x804b:next=0x804f;break;case 0x804f:next=0x8050;break;
  case 0x8050:next=c->_flag_Z?0x8052:0x804b;break;case 0x8052:next=0x8056;break;
  case 0x8056:next=0x8057;break;case 0x8057:next=c->_flag_Z?0x8059:0x8052;break;
  default:witness.phase=0;return 0;
  }
  witness.next=next;++stats.observed;if(next==0x8059)witness.phase=0;return 0;
 }
 for(;;){
  if(interp_bridge_source_opcode_deadline_reached(c)){
   witness.next=pc;++stats.deadlines;*landing=pc;return 1;
  }
  unsigned bytes,cycles,next;
  switch(pc){
  case 0x803d:bytes=3;cycles=3;next=0x8040;break;
  case 0x8040:bytes=2;cycles=2;next=0x8042;break;
  case 0x8042:bytes=3;cycles=5;next=0x8045;break;
  case 0x8045:bytes=1;cycles=2;next=0x8046;break;
  case 0x8046:bytes=3;cycles=3;next=0x8049;break;
  case 0x8049:bytes=2;cycles=c->_flag_Z?2:3;next=c->_flag_Z?0x804b:0x8042;break;
  case 0x804b:bytes=4;cycles=5;next=0x804f;break;
  case 0x804f:bytes=1;cycles=2;next=0x8050;break;
  case 0x8050:bytes=2;cycles=c->_flag_Z?2:3;next=c->_flag_Z?0x8052:0x804b;break;
  case 0x8052:bytes=4;cycles=5;next=0x8056;break;
  case 0x8056:bytes=1;cycles=2;next=0x8057;break;
  case 0x8057:bytes=2;cycles=c->_flag_Z?2:3;next=c->_flag_Z?0x8059:0x8052;break;
  default:witness.phase=0;return 0;
  }
  const uint64_t prior_master=c->master_cycles;
  c->open_bus=rom[(pc&32767)+bytes-1];
  c->coprocessor_master_cycles=c->master_cycles;
  c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2;
  switch(pc){
  case 0x803d:c->X=0;nz(c,0,32768);break;
  case 0x8040:c->A&=0xff00;nz(c,0,128);break;
  case 0x8042:cpu_write8_timed(c,0,c->X,0);break;
  case 0x804b:cpu_write8_timed(c,0x7e,c->X,c->A&255);break;
  case 0x8052:cpu_write8_timed(c,0x7f,c->X,c->A&255);break;
  case 0x8045:case 0x804f:case 0x8056:c->X=(c->X+1)&65535;nz(c,c->X,32768);break;
  case 0x8046:c->_flag_C=c->X>=0x2000;nz(c,(c->X-0x2000)&65535,32768);break;
  default:break;
  }
  ++stats.instructions;
  if(!interp_bridge_commit_source_opcode(c,prior_master,pc==0x8042||pc==0x804b||pc==0x8052))abort();
  pc=next;
  if(pc==0x8059){witness.phase=0;++stats.complete;*landing=pc;return 1;}
 }
}
#ifndef LAGOON_NATIVE_BOOT62_TEST
#include "common_rtl.h"
#include <stdio.h>
#include <stdlib.h>
static uint64_t frame_generation;
static int enabled=-1,report_enabled;
static void report(void){if(report_enabled)fprintf(stderr,
 "[native-boot62] armed=%llu instructions=%llu complete=%llu deadlines=%llu refused=%llu observed=%llu enabled=%d\n",
 (unsigned long long)stats.armed,(unsigned long long)stats.instructions,
 (unsigned long long)stats.complete,(unsigned long long)stats.deadlines,
 (unsigned long long)stats.refused,(unsigned long long)stats.observed,enabled);}
static void hook(CpuState*c,uint32_t pc){
 if(!enabled||c!=&g_cpu)return;
 for(unsigned n=0x13;n<0x59;++n)if(!cpu_aot_rom_mapping_matches(0x8000u+n,n,1)){witness.phase=0;++stats.refused;return;}
 pace_allowed=interp_bridge_source_opcode_available(c);
 uint32_t landing;
 if(LagoonNativeBoot62_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),frame_generation,pc,&landing))
  interp_bridge_pre_opcode_redirect(landing);
}
void LagoonNativeBoot62_BeforeFrame(void){
 if(enabled<0){const char*v=getenv("LAGOON_NATIVE_BOOT62");enabled=!v||!strcmp(v,"auto")||!strcmp(v,"on");
  v=getenv("LAGOON_NATIVE_WAIT_REPORT");report_enabled=v&&!strcmp(v,"1");if(report_enabled)atexit(report);}
 frame_generation=RtlStateGeneration();
 if(witness.phase&&witness.generation!=frame_generation)witness.phase=0;
 if(enabled){const uint32_t pcs[]={0x8033,0x8035,0x8038,0x803a,0x803d,0x8040,0x8042,0x8045,0x8046,0x8049,0x804b,0x804f,0x8050,0x8052,0x8056,0x8057};
  for(unsigned n=0;n<sizeof pcs/sizeof *pcs;++n)interp_bridge_add_pre_opcode_hook(pcs[n],hook);}
}
#endif
