#include "cpu_state.h"
#include "snes/interp_bridge.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test only the extracted live progress block and absolute selector. Device
 * state/PCM is exercised by the separate real-SPC/DSP batch/device fixtures. */
typedef struct TestApu { int portTimeValid; } TestApu;
typedef struct TestSnes { int *cart;TestApu *apu; } TestSnes;
static TestSnes *g_snes;
static int timeline,extended,shared,snes_frame_counter;
static uint64_t g_interp_bridge_write_epoch,s_interp_dynamic_progress_epoch;
static unsigned flushes;
static uint64_t pending_master;
static bool rtl_apu_frame_timeline_active(void) {return timeline!=0;}
static bool rtl_apu_extended_frame_timing(void) {return extended!=0;}
static int cart_has_sa1(const int *c) {return c&&*c;}
static int cosim_apu_shared_clock(void) {return shared;}
static void bridge_apu_flush(CpuState *cpu) {(void)cpu;flushes++;pending_master=0;}
#define SNES_COSIM 1
#include "apu_progress_under_test.h"

int main(int argc,char **argv) {
  assert(argc==2);int no_apu=atoi(argv[1]);unsigned tests=0;
  TestApu apu;TestSnes snes;int sa1;CpuState cpu={0};
  cpu.master_cycles=123456;cpu.A=0xa55a;cpu.X=0x40;cpu.S=0x1ffd;
  for(unsigned bits=0;bits<256;bits++)for(unsigned auto_q=0;auto_q<2;auto_q++)
  for(unsigned writes=0;writes<2;writes++)for(unsigned dynamic=0;dynamic<2;dynamic++) {
    timeline=!!(bits&1);extended=!!(bits&2);sa1=!!(bits&4);apu.portTimeValid=!!(bits&8);
    shared=!!(bits&16);snes_frame_counter=!(bits&128);
    snes.cart=&sa1;snes.apu=bits&32?NULL:&apu;g_snes=bits&64?NULL:&snes;
    int absolute=!no_apu&&snes_frame_counter&&timeline&&!shared&&g_snes&&snes.apu&&
      (sa1||(extended&&apu.portTimeValid));
    assert(rtl_apu_absolute_periodic_enabled()==absolute);
    uint64_t prev_write=100,prev_dynamic=200;
    g_interp_bridge_write_epoch=100+writes;s_interp_dynamic_progress_epoch=200+dynamic;
    unsigned steps=17;flushes=0;pending_master=99;CpuState before=cpu;
    progress_under_test(&cpu,(int)auto_q,&prev_write,&prev_dynamic,&steps);
    int progressed=auto_q&&(writes||dynamic);
    assert(prev_write==(progressed?100+writes:100));
    assert(prev_dynamic==(progressed?200+dynamic:200));
    assert(steps==(progressed?0:17));
    assert(flushes==(unsigned)(progressed&&!absolute));
    assert(pending_master==(flushes?0:99));
    assert(!memcmp(&before,&cpu,sizeof cpu));tests++;
    /* A repeated checkpoint has no progress and must not introduce a flush. */
    unsigned previous_flushes=flushes;progress_under_test(&cpu,(int)auto_q,&prev_write,&prev_dynamic,&steps);
    assert(flushes==previous_flushes);tests++;
  }
  printf("PASS %u extracted progress-policy cases: noAPU%d; boot/legacy extra flush retained, shared absolute no extra flush, epochs/steps/CPU preserved\n",tests,no_apu);
  return 0;
}
