#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "cpu_state.h"
#include "snes/snes.h"
#include "snes/apu.h"
#include "snes/spc.h"
#include "snes/dsp.h"
#include "snes/dsp_shadow.h"

CpuState g_cpu;
Snes *g_snes;
int snes_frame_counter=50;
uint64_t g_apu_timer0_total_ticks;
static uint64_t generation=7;
static bool speculative,rollback,g_apu_frame_time_valid=true;
static unsigned locks;
void RtlApuLock(void){++locks;}
void RtlApuUnlock(void){assert(locks);--locks;}
bool RtlSpeculativeFrame(void){return speculative;}
bool RtlIsRollbackLoad(void){return rollback;}
uint64_t RtlStateGeneration(void){return generation;}
#include "modal_audio_under_test.h"
DspShadow *dsp_shadow_create(void){return NULL;}
void dsp_shadow_free(DspShadow*s){(void)s;}
void dsp_shadow_process(DspShadow*s,Dsp*d,int l,int r,int*ol,int*orr){(void)s;(void)d;*ol=l;*orr=r;}

int main(void){
  static Snes machine;
  static unsigned char wram[131072],before_wram[131072];
  g_snes=&machine;machine.apu=apu_init();apu_reset(machine.apu);
  Apu*a=machine.apu;a->romReadable=false;a->spc->pc=0x200;
  const uint8_t loop[]={0xE4,0xF4,0xC4,0xF5,0x2F,0xFA};
  memcpy(a->ram+0x200,loop,sizeof(loop));
  dsp_write(a->dsp,0x0C,0x7F);dsp_write(a->dsp,0x1C,0x7F);
  dsp_write(a->dsp,0x00,0x7F);dsp_write(a->dsp,0x01,0x7F);
  dsp_write(a->dsp,0x05,0);dsp_write(a->dsp,0x07,0x7F);
  dsp_write(a->dsp,0x3D,1);dsp_write(a->dsp,0x6C,0x3F);dsp_write(a->dsp,0x4C,1);
  assert(apu_schedulePortWrite(a,3,0,1000));
  assert(apu_runToGuestCycle(a,1000,100));
  g_cpu.master_cycles=1234567;
  memset(wram,0x59,sizeof(wram));memcpy(before_wram,wram,sizeof(wram));
  CpuState cpu_before=g_cpu;Snes machine_before=machine;
  uint64_t clock0=a->portClock,target0=a->portTargetAnchor,last0=a->portLastTarget;
  uint64_t guest0=a->portGuestAnchor,lastguest0=a->portLastGuest;
  assert(!RtlAudioModalPump(2136));assert(RtlAudioModalBegin());
  unsigned nonzero=0;int16_t samples[512*2];
  for(unsigned block=0;block<1200;++block){
    assert(RtlAudioModalPump(2136));assert(dsp_available(a->dsp)==2136);
    assert(dsp_getSamples(a->dsp,samples,512)==512);
    for(unsigned i=0;i<1024;i++)nonzero+=samples[i]!=0;
    assert(a->portTargetAnchor-target0==a->portClock-clock0);
    assert(a->portLastTarget-last0==a->portClock-clock0);
    assert(a->portGuestAnchor==guest0&&a->portLastGuest==lastguest0);
    assert(!memcmp(&g_cpu,&cpu_before,sizeof(g_cpu)));
    assert(!memcmp(wram,before_wram,sizeof(wram)));
    assert(!memcmp(&machine,&machine_before,sizeof(machine)));
    assert(snes_frame_counter==50&&generation==7&&locks==0);
  }
  assert(nonzero>100000);uint64_t advanced=a->portClock-clock0;
  RtlAudioModalEnd();assert(!RtlAudioModalPump(2136));
  /* Resume with the next real guest timestamp: exactly its new512 samples,
   * no modal-duration catch-up, no delayed CPU port command. */
  uint64_t before=a->portClock;
  assert(apu_schedulePortWrite(a,0,0x73,1000+512*32));
  assert(apu_runToGuestCycle(a,1000+512*32,512*32+2));
  assert(a->portClock-before==512*32);
  assert(a->inPorts[0]==0x73&&apu_portQueueDepth(a)==0);
  assert(RtlAudioModalBegin());RtlAudioModalEnd();
  speculative=true;assert(!RtlAudioModalBegin());speculative=false;
  rollback=true;assert(!RtlAudioModalBegin());rollback=false;
  g_apu_frame_time_valid=false;assert(!RtlAudioModalBegin());g_apu_frame_time_valid=true;
  assert(apu_schedulePortWrite(a,2,0x12,1000+1024*32));
  assert(!RtlAudioModalBegin());assert(apu_runUntilPortQueueEmpty(a,32770));
  assert(RtlAudioModalBegin());++generation;assert(!RtlAudioModalPump(2136));RtlAudioModalEnd();
  assert(RtlAudioModalBegin());++g_cpu.master_cycles;assert(!RtlAudioModalPump(2136));RtlAudioModalEnd();
  assert(RtlAudioModalBegin());++snes_frame_counter;assert(!RtlAudioModalPump(2136));RtlAudioModalEnd();
  assert(RtlAudioModalBegin());speculative=true;assert(!RtlAudioModalPump(2136));speculative=false;RtlAudioModalEnd();
  assert(RtlAudioModalBegin());assert(!RtlAudioModalPump(4097));RtlAudioModalEnd();
  assert(RtlAudioModalBegin());assert(!RtlAudioModalPump(0));RtlAudioModalEnd();
  assert(RtlAudioModalBegin());a->portTargetAnchor=UINT64_MAX-1;
  before=a->portClock;assert(!RtlAudioModalPump(2136));assert(a->portClock==before);
  RtlAudioModalEnd();assert(locks==0);
  printf("PASS real SPC/DSP:1200 bounded blocks,614400 native stereo frames, %u nonzero values; modal SPC advanced=%" PRIu64 "; exact rebased512-sample resume; CPU/WRAM/PPU/frame/guest timestamps unchanged; source guards/refusals pass\n",nonzero,advanced);
  apu_free(a);return 0;
}
