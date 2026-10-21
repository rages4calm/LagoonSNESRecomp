/* Actual profiled desktop lock/wait source with a deterministic SDL consumer. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct {uint32_t sampleWrite,sampleRead;} Dsp;
typedef struct {Dsp *dsp;} Apu;
typedef struct {Apu *apu;} Snes;
static Dsp dsp;static Apu apu={&dsp};static Snes snes={&apu},*g_snes=&snes;
static bool g_audio_producer_active,g_audio_consumer_stalled,speculative,consume,callback_only;
static uint32_t g_audio_stalled_read;
static unsigned g_apu_lock_depth,locked,delays,callback_notifications;
static uint64_t milliseconds;
static void *g_audio_mutex;
static bool g_profile=true;
static unsigned g_profile_owner_thread=1,g_profile_audio_queue_peak;
static struct {double mutex_seconds,highwater_seconds,sleep_seconds;uint64_t mutex_calls,highwater_waits,highwater_sleeps;}g_profile_audio;
static unsigned ProfileCallingThread(void){return 1;}
static double MonotonicSeconds(void){return(double)milliseconds/1000.0;}
static void SDL_LockMutex(void *m){(void)m;locked++;}
static void SDL_UnlockMutex(void *m){(void)m;assert(locked);locked--;}
static bool RtlSpeculativeFrame(void){return speculative;}
static uint32_t dsp_available(Dsp *d){return d->sampleWrite-d->sampleRead;}
static void SDL_Delay(unsigned ms){
  assert(ms==1&&!locked);milliseconds+=ms;delays++;
  if(consume)dsp.sampleRead+=32;
  if(callback_only)callback_notifications++;
}
#include "audio_producer_under_test.h"
static void checkpoint(void){RtlApuLock();RtlApuUnlock();assert(!locked&&!g_apu_lock_depth);}
static void reset(void){
  dsp.sampleRead=0;dsp.sampleWrite=7000;g_audio_producer_active=true;
  g_audio_consumer_stalled=false;g_audio_stalled_read=0;g_apu_lock_depth=locked=delays=callback_notifications=0;
  milliseconds=0;speculative=consume=callback_only=false;g_profile=true;
  memset(&g_profile_audio,0,sizeof g_profile_audio);g_profile_audio_queue_peak=0;
}
int main(void){
  unsigned cases=0;reset();consume=true;checkpoint();
  assert(delays==91&&dsp.sampleRead==2912&&dsp.sampleWrite==7000&&!g_audio_consumer_stalled);
  assert(g_profile_audio.highwater_waits==1&&g_profile_audio.highwater_sleeps==91&&g_profile_audio_queue_peak==7000);cases++;
  reset();callback_only=true;checkpoint();assert(delays==250&&g_audio_consumer_stalled&&!g_audio_producer_active);
  assert(callback_notifications==250&&dsp.sampleWrite==7000&&dsp.sampleRead==0);
  for(unsigned i=0;i<100;i++){callback_notifications++;g_audio_producer_active=true;checkpoint();}
  assert(delays==250&&callback_notifications==350&&g_audio_consumer_stalled&&g_profile_audio.highwater_waits==1);cases++;
  consume=true;dsp.sampleRead=32;g_audio_producer_active=true;checkpoint();
  assert(delays==340&&dsp.sampleRead==2912&&dsp.sampleWrite==7000&&!g_audio_consumer_stalled);cases++;
  reset();checkpoint();unsigned before=delays;
  dsp.sampleWrite=HOST_AUDIO_HIGH_WATER;g_audio_producer_active=true;checkpoint();
  assert(delays==before&&!g_audio_consumer_stalled);
  dsp.sampleWrite=7000;consume=true;checkpoint();assert(delays==before+91&&dsp.sampleWrite==7000);cases++;
  reset();consume=true;RtlApuLock();RtlApuLock();RtlApuUnlock();
  assert(locked==1&&g_apu_lock_depth==1&&!delays);RtlApuUnlock();
  assert(!locked&&!g_apu_lock_depth&&delays==91);cases++;
  reset();g_audio_producer_active=false;checkpoint();assert(!delays&&dsp.sampleWrite==7000&&dsp.sampleRead==0);cases++;
  reset();speculative=true;checkpoint();assert(!delays&&dsp.sampleWrite==7000&&dsp.sampleRead==0);cases++;
  reset();g_profile=false;consume=true;checkpoint();assert(delays==91&&!g_profile_audio.highwater_waits&&!g_profile_audio.highwater_sleeps);cases++;
  reset();milliseconds=(uint64_t)UINT32_MAX-100;checkpoint();assert(delays>=250&&delays<=251&&g_audio_consumer_stalled);cases++;
  reset();dsp.sampleRead=UINT32_MAX-100;dsp.sampleWrite=dsp.sampleRead+7000;
  uint32_t write=dsp.sampleWrite;consume=true;checkpoint();assert(delays==91&&dsp.sampleRead==2811&&dsp.sampleWrite==write);cases++;
  reset();for(unsigned i=0;i<2;i++){dsp.sampleWrite=HOST_AUDIO_HIGH_WATER-i;checkpoint();assert(!delays&&!g_audio_consumer_stalled);}cases++;
  printf("PASS actual desktop producer latch:%u scenarios;100 callback-only checkpoints remain latched; genuine read or low occupancy rearms; nested/inactive/spec/profile-off/wrap paths; sleeps unlocked, no producer PCM/cursor writes\n",cases);
  return 0;
}
