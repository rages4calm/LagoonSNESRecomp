/* Exercise the actual framework token matcher with the original death frame. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "cpu_state.h"
static uint8_t ram[131072],before_ram[131072];
static uint64_t generation=1;
static uint64_t RtlStateGeneration(void){return generation;}
#include "continuation_policy_under_test.h"
static int source_ok=1;
static int source_guard(const InterpAotContinuation *d){(void)d;return source_ok;}
static const InterpAotContinuation descriptor={0x01930f,0x008985,0x019313,0x019294,{0x00898d,0x00899a},0,1,0,0,1,3,0,2};
static CpuState cpu,before;
static void setup(void){
  memset(&cpu,0,sizeof(cpu));memset(ram,0,sizeof ram);cpu.ram=ram;cpu.S=0x1ff0;cpu.DB=1;cpu.m_flag=1;cpu.P=CPU_P_M;
  /* Real JSL930F return9312, then inherited JSR9291 return9293. */
  const uint8_t stack[]={0x12,0x93,1,0x93,0x92};memcpy(cpu.ram+cpu.S+1,stack,5);
  source_ok=1;generation=1;assert(interp_bridge_set_aot_continuations(&descriptor,1,source_guard));
}
static uint64_t arm(int ready){return continuation_arm(&cpu,0x008985,0x01930f,3,ready);}
static int take(uint32_t site){InterpAotContinuation out;uint16_t owner=0;int ok=continuation_take(&cpu,site,0x019313,0x1ff0,&out,&owner);if(ok){assert(owner==cpu.S);assert(!memcmp(&out,&descriptor,sizeof(out)));}return ok;}
static void rtl(void){cpu.S+=3;cpu.PB=1;}
static unsigned hook_events[4],bad_proposal;
static int ancestor_hook(unsigned event,const CpuState *state,uint32_t source,
  uint32_t target,const InterpAotContinuation *d,uint16_t owner,InterpAotAncestor *out) {
  (void)source;(void)target;(void)d;(void)owner;hook_events[event]++;
  if(event!=INTERP_CONT_RETURN)return 0;
  out->pc24=0x019294;out->owner_return_pc24=0x019500;
  out->entry_s=(uint16_t)(state->S+2);out->owner_frame=2;
  switch(bad_proposal){case 1:out->pc24++;break;case 2:out->owner_frame=3;break;
    case 3:out->owner_return_pc24=0x029500;break;case 4:out->entry_s=state->S-1;break;
    case 5:out->entry_s=state->S+17;break;case 6:out->entry_s=0x1ffe;break;
    case 7:out->owner_return_pc24++;break;}
  return 1;
}
int main(void){
  setup();assert(continuation_candidate(0x00898d,0x019313));
  assert(!continuation_candidate(0x00898e,0x019313));
  assert(!continuation_candidate(0x00898d,0x019314));
  setup();InterpAotAncestor ancestor;
  assert(!continuation_ancestor(&cpu,&descriptor,0,&ancestor));
  setup();assert(!take(0x00898d));
  setup();uint64_t id=arm(0);assert(id);continuation_finish_wait(id,1);rtl();before=cpu;memcpy(before_ram,ram,sizeof ram);
  assert(take(0x00899a));assert(!memcmp(&cpu,&before,sizeof(cpu))&&!memcmp(ram,before_ram,sizeof ram));assert(!take(0x00899a));
  setup();id=arm(0);assert(id);continuation_finish_wait(id,0);rtl();assert(!take(0x00898d));
  setup();assert(arm(1));rtl();assert(!take(0x00898e));assert(take(0x00898d));
  for(unsigned offset=1;offset<=5;offset++){
    setup();assert(arm(1));rtl();cpu.ram[0x1ff0+offset]^=1;before=cpu;memcpy(before_ram,ram,sizeof ram);
    assert(!take(0x00898d));assert(!memcmp(&cpu,&before,sizeof(cpu))&&!memcmp(ram,before_ram,sizeof ram));
    setup();cpu.ram[0x1ff0+offset]^=1;assert(!arm(1));
  }
  for(unsigned bad=0;bad<9;bad++){
    setup();assert(arm(1));rtl();
    switch(bad){case 0:generation++;break;case 1:cpu.m_flag=0;break;case 2:cpu.x_flag=1;break;
      case 3:cpu.D=1;break;case 4:cpu.DB=2;break;case 5:cpu.emulation=1;break;
      case 6:cpu.S++;break;case 7:source_ok=0;break;case 8:continuation_discard_tokens();break;}
    before=cpu;memcpy(before_ram,ram,sizeof ram);assert(!take(0x00898d));assert(!memcmp(&cpu,&before,sizeof(cpu))&&!memcmp(ram,before_ram,sizeof ram));
  }
  setup();assert(!continuation_arm(&cpu,0x008985,0x019310,3,1));
  setup();cpu.S=0x1ffb;assert(!arm(1));
  /* Actual matcher with seven-byte render/main boss frames, both original
   * return sites and saved X bytes. Readiness comes only from real suspension
   * or quiescent interpreter ownership, never a synthetic guest frame. */
  for(unsigned owner_kind=0;owner_kind<2;owner_kind++)for(unsigned render=0;render<2;render++){
    InterpAotContinuation boss={render?0x01B802:0x01B80B,render?0x01C643:0x0081B5,
      render?0x01B806:0x01B80F,owner_kind?0x01C6AF:0x0180CD,
      {render?0x01C694:0x0081BD,render?0x01C694:0x0081BD},0,1,0,0,1,3,2,2};
    setup();assert(interp_bridge_set_aot_continuations(&boss,1,source_guard));
    uint16_t wr=(uint16_t)(boss.continuation_pc24-1),own=(uint16_t)(boss.owner_return_pc24-1);
    uint8_t stack[]={wr&255,wr>>8,1,0x34,0x12,own&255,own>>8};
    memcpy(cpu.ram+cpu.S+1,stack,sizeof(stack));cpu.PB=(uint8_t)(boss.wait_pc24>>16);
    id=continuation_arm(&cpu,boss.wait_pc24,boss.call_pc24,3,0);assert(id);
    continuation_finish_wait(id,1);rtl();InterpAotContinuation out;uint16_t outer;
    before=cpu;memcpy(before_ram,ram,sizeof ram);assert(continuation_take(&cpu,boss.return_sites[0],boss.continuation_pc24,0x1ff0,&out,&outer));
    assert(outer==cpu.S+2&&(!memcmp(&cpu,&before,sizeof(cpu))&&!memcmp(ram,before_ram,sizeof ram)));
    assert(!continuation_take(&cpu,boss.return_sites[0],boss.continuation_pc24,0x1ff0,&out,&outer));
  }
  /* Death flash: REAL PHA flag + PHX index + JSR owner make eight bytes.
   * Each of six waits owns a fresh serial; no completed wait can be reused. */
  for(unsigned owner_kind=0;owner_kind<2;owner_kind++)for(unsigned render=0;render<2;render++){
    InterpAotContinuation flash={render?0x019364:0x01936D,render?0x01C643:0x0081B5,
      render?0x019368:0x019371,owner_kind?0x01B449:0x019321,
      {render?0x01C694:0x0081BD,render?0x01C694:0x0081BD},0,1,0,0,1,3,3,2};
    for(unsigned step=0;step<6;step++){
      setup();assert(interp_bridge_set_aot_continuations(&flash,1,source_guard));
      uint16_t wr=(uint16_t)(flash.continuation_pc24-1),own=(uint16_t)(flash.owner_return_pc24-1);
      uint8_t stack[]={wr&255,wr>>8,1,0x40,0x01,0xa5,own&255,own>>8};
      memcpy(cpu.ram+cpu.S+1,stack,sizeof stack);cpu.PB=flash.wait_pc24>>16;
      id=continuation_arm(&cpu,flash.wait_pc24,flash.call_pc24,3,0);assert(id);
      continuation_finish_wait(id,1);rtl();InterpAotContinuation out;uint16_t outer;
      before=cpu;memcpy(before_ram,ram,sizeof ram);assert(continuation_take(&cpu,flash.return_sites[0],flash.continuation_pc24,0x1ff0,&out,&outer));
      assert(outer==cpu.S+3&&(!memcmp(&cpu,&before,sizeof cpu)&&!memcmp(ram,before_ram,sizeof ram)));
      assert(!continuation_take(&cpu,flash.return_sites[0],flash.continuation_pc24,0x1ff0,&out,&outer));
      for(unsigned bad=1;bad<=8;bad++){
        /* All saved/owner/wait bytes are protected by the ACTUAL matcher. */
        cpu.S=0x1ff0;cpu.PB=flash.wait_pc24>>16;memcpy(cpu.ram+cpu.S+1,stack,sizeof stack);
        assert(continuation_arm(&cpu,flash.wait_pc24,flash.call_pc24,3,1));rtl();
        cpu.ram[0x1ff0+bad]^=1;before=cpu;memcpy(before_ram,ram,sizeof ram);
        assert(!continuation_take(&cpu,flash.return_sites[0],flash.continuation_pc24,0x1ff0,&out,&outer));
        assert((!memcmp(&cpu,&before,sizeof cpu)&&!memcmp(ram,before_ram,sizeof ram)));
      }
    }
  }
  setup();interp_bridge_set_aot_continuation_hook(ancestor_hook);
  assert(arm(1));assert(hook_events[INTERP_CONT_CALL]);rtl();assert(take(0x00898d));
  assert(hook_events[INTERP_CONT_TAKE]);
  cpu.ram[cpu.S+3]=0xff;cpu.ram[cpu.S+4]=0x94;
  for(bad_proposal=0;bad_proposal<8;bad_proposal++){
    before=cpu;memcpy(before_ram,ram,sizeof ram);assert(continuation_ancestor(&cpu,&descriptor,cpu.S,&ancestor)==!bad_proposal);
    assert(!memcmp(&cpu,&before,sizeof(cpu))&&!memcmp(ram,before_ram,sizeof ram));
  }
  bad_proposal=0;cpu.emulation=1;assert(!continuation_ancestor(&cpu,&descriptor,cpu.S,&ancestor));
  cpu.emulation=0;cpu.PB=0;assert(!continuation_ancestor(&cpu,&descriptor,cpu.S,&ancestor));
  continuation_discard_tokens();assert(hook_events[INTERP_CONT_DISCARD]);
  interp_bridge_set_aot_continuation_hook(NULL);
  assert(!continuation_ancestor(&cpu,&descriptor,cpu.S,&ancestor));
  puts("PASS real five/seven/eight-byte death/boss frames and six original death-flash cycles, both owners/waits, one-shot readonly policy and bounded ancestor proposals");
  return 0;
}
