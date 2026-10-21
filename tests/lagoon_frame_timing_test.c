/* Actual buffered presentation diagnostic bookkeeping with deterministic inputs. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
enum {kProfileCount=9};
typedef struct ProfileAudio {
 double highwater_seconds,sleep_seconds,mutex_seconds;
 uint64_t highwater_waits,highwater_sleeps,mutex_calls;
} ProfileAudio;
static bool g_profile=true;
static ProfileAudio g_profile_audio,g_profile_audio_previous;
static unsigned g_profile_audio_queue_peak;
static struct {double total;uint64_t thread_cycles;}g_timings[kProfileCount];
static double now=3,periods=1.459094;
static unsigned clock_calls;
static double MonotonicSeconds(void){clock_calls++;return now;}
static double RtlLastFramePeriods(void){return periods;}
static unsigned ProfileProcessor(void){return 7;}
#include "frame_timing_under_test.h"
int main(void){
 unsigned cases=0;
 g_profile_wait=(ProfileWait){.02,.004,2};RecordFrameTiming(1);assert(!clock_calls&&g_profile_wait.calls==2);cases++;
 FrameTiming frames[2]={0};g_frame_timings=frames;g_profile=false;RecordFrameTiming(1);assert(!clock_calls&&g_profile_wait.calls==2);cases++;
 g_profile=true;g_profile_audio=(ProfileAudio){.01,.008,.001,1,8,9};g_profile_audio_queue_peak=3200;
 g_timings[0].total=.003;g_timings[0].thread_cycles=70;RecordFrameTiming(123);
 assert(clock_calls==1&&g_frame_timing_count==1&&frames[0].frame==123&&frames[0].at==3&&frames[0].periods==periods&&frames[0].processor==7);
 assert(frames[0].wait.requested_seconds==.02&&frames[0].wait.overshoot_seconds==.004&&frames[0].wait.calls==2);
 assert(!g_profile_wait.calls&&!g_profile_wait.requested_seconds&&!g_profile_wait.overshoot_seconds);
 assert(frames[0].audio.highwater_seconds==.01&&frames[0].audio_queue_peak==3200&&!g_profile_audio_queue_peak&&frames[0].stages[0]==.003&&frames[0].thread_cycles[0]==70);cases++;
 g_profile_wait=(ProfileWait){.016,.0001,1};now=4;periods=1;g_timings[0].total=.005;g_timings[0].thread_cycles=100;RecordFrameTiming(124);
 assert(clock_calls==2&&g_frame_timing_count==2&&frames[1].wait.calls==1&&frames[1].wait.requested_seconds==.016&&frames[1].wait.overshoot_seconds==.0001);
 assert(frames[1].audio.highwater_seconds==0&&frames[1].audio_queue_peak==0&&frames[1].stages[0]>.001999999&&frames[1].stages[0]<.002000001&&frames[1].thread_cycles[0]==30&&!g_profile_wait.calls);cases++;
 g_frame_timing_count=kFrameTimingCapacity;g_profile_wait=(ProfileWait){.01,0,1};RecordFrameTiming(125);assert(clock_calls==2&&g_profile_wait.calls==1);cases++;
 printf("PASS actual RecordFrameTiming: %u off/window/delta/wait-reset/capacity cases; no file/device calls\n",cases);return 0;
}
