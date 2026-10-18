/* Exercise the actual framework token matcher with the original death frame. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
typedef struct CpuState {uint16_t S,D;uint8_t m_flag,x_flag,emulation,DB,PB;uint8_t ram[131072];} CpuState;
static uint64_t generation=1;
static uint64_t RtlStateGeneration(void){return generation;}
#include "continuation_policy_under_test.h"
static int source_ok=1;
static int source_guard(const InterpAotContinuation *d){(void)d;return source_ok;}
static const InterpAotContinuation descriptor={0x01930f,0x008985,0x019313,0x019294,{0x00898d,0x00899a},0,1,0,0,1,3,0,2};
static CpuState cpu,before;
static void setup(void){
  memset(&cpu,0,sizeof(cpu));cpu.S=0x1ff0;cpu.DB=1;cpu.m_flag=1;
  /* Real JSL930F return9312, then inherited JSR9291 return9293. */
  const uint8_t stack[]={0x12,0x93,1,0x93,0x92};memcpy(cpu.ram+cpu.S+1,stack,5);
  source_ok=1;generation=1;assert(interp_bridge_set_aot_continuations(&descriptor,1,source_guard));
}
static uint64_t arm(int ready){return continuation_arm(&cpu,0x008985,0x01930f,3,ready);}
static int take(uint32_t site){InterpAotContinuation out;uint16_t owner=0;int ok=continuation_take(&cpu,site,0x019313,0x1ff0,&out,&owner);if(ok){assert(owner==cpu.S);assert(!memcmp(&out,&descriptor,sizeof(out)));}return ok;}
static void rtl(void){cpu.S+=3;cpu.PB=1;}
int main(void){
  setup();assert(continuation_candidate(0x00898d,0x019313));
  assert(!continuation_candidate(0x00898e,0x019313));
  assert(!continuation_candidate(0x00898d,0x019314));
  setup();assert(!take(0x00898d));
  setup();uint64_t id=arm(0);assert(id);continuation_finish_wait(id,1);rtl();before=cpu;
  assert(take(0x00899a));assert(!memcmp(&cpu,&before,sizeof(cpu)));assert(!take(0x00899a));
  setup();id=arm(0);assert(id);continuation_finish_wait(id,0);rtl();assert(!take(0x00898d));
  setup();assert(arm(1));rtl();assert(!take(0x00898e));assert(take(0x00898d));
  for(unsigned offset=1;offset<=5;offset++){
    setup();assert(arm(1));rtl();cpu.ram[0x1ff0+offset]^=1;before=cpu;
    assert(!take(0x00898d));assert(!memcmp(&cpu,&before,sizeof(cpu)));
    setup();cpu.ram[0x1ff0+offset]^=1;assert(!arm(1));
  }
  for(unsigned bad=0;bad<9;bad++){
    setup();assert(arm(1));rtl();
    switch(bad){case 0:generation++;break;case 1:cpu.m_flag=0;break;case 2:cpu.x_flag=1;break;
      case 3:cpu.D=1;break;case 4:cpu.DB=2;break;case 5:cpu.emulation=1;break;
      case 6:cpu.S++;break;case 7:source_ok=0;break;case 8:continuation_discard_tokens();break;}
    before=cpu;assert(!take(0x00898d));assert(!memcmp(&cpu,&before,sizeof(cpu)));
  }
  setup();assert(!continuation_arm(&cpu,0x008985,0x019310,3,1));
  setup();cpu.S=0x1ffb;assert(!arm(1));
  puts("PASS original five-byte death frame, real-wait/single-use, readonly stack/mode/source/generation refusals");
  return 0;
}
