/* Production WaitUntil with deterministic SDL deadline/clock doubles. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint32_t Uint32;
typedef uint64_t Uint64;
typedef struct {double at; uint64_t cycles;} ProfileStamp;
enum {kProfileWait=8};
static uint64_t clock_ns, precise_args[16], precise_steps[16], query_step;
static unsigned precise_count, coarse_count, coarse_args[32], starts, ends, clock_calls;
static bool g_profile;
static void *g_frame_timings;
static double profiled_seconds;
static double MonotonicSeconds(void) {clock_calls++;double v=(double)clock_ns/1e9;clock_ns+=query_step;return v;}
static ProfileStamp ProfileStart(void) {starts++;return (ProfileStamp){(double)clock_ns/1e9,1};}
static void ProfileEnd(unsigned stage,ProfileStamp start) {assert(stage==kProfileWait);ends++;if(g_profile)profiled_seconds=(double)clock_ns/1e9-start.at;}
#if SNESRECOMP_SDL3
static void SDL_DelayPrecise(Uint64 ns) {assert(precise_count<16);precise_args[precise_count]=ns;uint64_t step=precise_steps[precise_count++];clock_ns+=step?step:ns;}
#else
static void SDL_Delay(Uint32 ms) {assert(coarse_count<32);coarse_args[coarse_count++]=ms;clock_ns+=ms?(uint64_t)ms*1000000:250000;}
#endif
#include "wait_until_under_test.h"
static void reset(void) {clock_ns=0;query_step=1;precise_count=coarse_count=starts=ends=clock_calls=0;g_frame_timing_count=0;g_profile=true;g_frame_timings=NULL;g_profile_wait=(ProfileWait){0};profiled_seconds=0;memset(precise_args,0,sizeof precise_args);memset(precise_steps,0,sizeof precise_steps);memset(coarse_args,0,sizeof coarse_args);}
static void balanced(void) {assert(starts==1&&ends==1);}
int main(void) {
 unsigned cases=0;
 reset();WaitUntil(0);balanced();assert(!precise_count&&!coarse_count);cases++;
 reset();clock_ns=20000000;WaitUntil(.010);balanced();assert(!precise_count&&!coarse_count);cases++;
#if SNESRECOMP_SDL3
 reset();WaitUntil(.016);balanced();assert(precise_count==1&&!coarse_count&&precise_args[0]>=15999998&&precise_args[0]<=16000000&&clock_ns>=16000000);cases++;
 reset();precise_steps[0]=5000000;WaitUntil(.016);balanced();assert(precise_count==2&&precise_args[0]>=15999998&&precise_args[1]>=10999997&&precise_args[1]<=11000000&&clock_ns>=16000000);cases++;
 reset();precise_steps[0]=21000000;WaitUntil(.016);balanced();assert(precise_count==1&&clock_ns>=21000000);cases++;
 reset();WaitUntil(.0004);balanced();assert(precise_count==1&&precise_args[0]>=399998&&precise_args[0]<=400000);cases++;
 reset();query_step=1;WaitUntil(.0000000005);balanced();assert(precise_count==1&&precise_args[0]==0&&clock_ns>=1);cases++;
 reset();clock_ns=1000000000000;WaitUntil(1000.016);balanced();assert(precise_count<=2&&precise_args[0]>=15999900&&precise_args[0]<=16000100);cases++;
 reset();g_profile=false;WaitUntil(.016);balanced();assert(precise_count==1&&profiled_seconds==0);cases++;
 reset();WaitUntil(.016);balanced();assert(profiled_seconds>=.016&&profiled_seconds<.016001);cases++;
#else
 reset();WaitUntil(.016);balanced();assert(!precise_count&&coarse_count>1&&coarse_args[0]==15&&coarse_args[1]==0&&clock_ns>=16000000);cases++;
 reset();WaitUntil(.0015);balanced();assert(coarse_args[0]==0&&clock_ns>=1500000);cases++;
 reset();WaitUntil(.0016);balanced();assert(coarse_args[0]==1&&coarse_args[1]==0&&clock_ns>=1600000);cases++;
 reset();g_profile=false;WaitUntil(.002);balanced();assert(profiled_seconds==0&&!precise_count);cases++;
 reset();WaitUntil(.002);balanced();assert(profiled_seconds>=.002&&!precise_count);cases++;
#endif
 reset();query_step=0;g_frame_timings=(void*)1;WaitUntil(.016);balanced();assert(g_profile_wait.calls==1&&g_profile_wait.requested_seconds==.016&&g_profile_wait.overshoot_seconds>=0&&g_profile_wait.overshoot_seconds<=.00025);cases++;
 reset();query_step=0;g_frame_timings=(void*)1;WaitUntil(.008);WaitUntil(.016);assert(starts==2&&ends==2&&g_profile_wait.calls==2&&g_profile_wait.requested_seconds==.016);cases++;
 reset();g_frame_timings=(void*)1;clock_ns=20000000;WaitUntil(.010);balanced();assert(!g_profile_wait.calls&&!g_profile_wait.requested_seconds&&!g_profile_wait.overshoot_seconds);cases++;
 reset();g_frame_timings=(void*)1;g_profile=false;WaitUntil(.016);balanced();assert(!g_profile_wait.calls&&!g_profile_wait.requested_seconds&&!g_profile_wait.overshoot_seconds);cases++;
 reset();WaitUntil(.016);balanced();assert(!g_profile_wait.calls&&!g_profile_wait.requested_seconds&&!g_profile_wait.overshoot_seconds);cases++;
#if SNESRECOMP_SDL3
 reset();query_step=0;g_frame_timings=(void*)1;precise_steps[0]=21000000;WaitUntil(.016);balanced();assert(g_profile_wait.calls==1&&g_profile_wait.requested_seconds==.016&&g_profile_wait.overshoot_seconds>.004999999&&g_profile_wait.overshoot_seconds<.005000001);cases++;
 reset();query_step=0;g_frame_timings=(void*)1;precise_steps[0]=5000000;WaitUntil(.016);balanced();assert(precise_count==2&&g_profile_wait.calls==1&&g_profile_wait.requested_seconds==.016&&g_profile_wait.overshoot_seconds==0);cases++;
#endif
 reset();g_frame_timings=(void*)1;g_frame_timing_count=kFrameTimingCapacity;WaitUntil(.016);balanced();assert(!g_profile_wait.calls&&!g_profile_wait.requested_seconds&&!g_profile_wait.overshoot_seconds);cases++;
 reset();WaitUntil(.016);unsigned plain_clocks=clock_calls,plain_precise=precise_count,plain_coarse=coarse_count;uint64_t plain_end=clock_ns;
 reset();g_frame_timings=(void*)1;WaitUntil(.016);balanced();assert(clock_calls==plain_clocks&&precise_count==plain_precise&&coarse_count==plain_coarse&&clock_ns==plain_end);cases++;
 printf("PASS actual WaitUntil SDL%d: %u deadline/profile scenarios; no guest/device state in harness\n",SNESRECOMP_SDL3?3:2,cases);
 return 0;
}
