/* Compile the real SDK matcher, not a second implementation of its policy. */
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
typedef struct CpuState {uint16_t S,D;uint8_t m_flag,x_flag,emulation,DB,PB;uint8_t ram[131072];} CpuState;
static uint64_t generation=1;
static uint64_t RtlStateGeneration(void){return generation;}
#include "continuation_policy_under_test.h"
static CpuState cpu,before;
static InterpAotContinuation expected;
static int source_ok;
static int guard(const InterpAotContinuation *d){return source_ok && !memcmp(d,&expected,sizeof(*d));}
static void setup(unsigned saved){
  memset(&cpu,0,sizeof(cpu));cpu.S=0x1fe0;cpu.DB=1;cpu.m_flag=1;generation=1;source_ok=1;
  expected=(InterpAotContinuation){0x019100,0x0090db,0x019104,0x019050,
    {0x0090f3,0x00910d,0x009129,0x009139},0,1,0,0,1,3,(uint8_t)saved,2};
  cpu.ram[cpu.S+1]=3;cpu.ram[cpu.S+2]=0x91;cpu.ram[cpu.S+3]=1;
  for(unsigned j=0;j<saved;++j)cpu.ram[cpu.S+4+j]=(uint8_t)(0xa0+j);
  cpu.ram[cpu.S+4+saved]=0x4f;cpu.ram[cpu.S+5+saved]=0x90;
  assert(interp_bridge_set_aot_continuations(&expected,1,guard));
}
static uint64_t arm(void){return continuation_arm(&cpu,0x0090db,0x019100,3,0);}
static void rtl(void){cpu.S+=3;cpu.PB=1;}
static int take(unsigned site){InterpAotContinuation out;uint16_t owner;
  int result=continuation_take(&cpu,site,0x019104,0x1fe0,&out,&owner);
  if(result){assert(!memcmp(&out,&expected,sizeof(out)));assert(owner==cpu.S+expected.saved_stack_bytes);}
  return result;
}
int main(void){
  unsigned positives=0,mutants=0;
  setup(0);InterpAotAncestor unknown;assert(!continuation_ancestor(&cpu,&expected,0,&unknown));
  const unsigned saved_sizes[]={0,2,3,4};
  for(unsigned s=0;s<4;++s)for(unsigned slot=0;slot<4;++slot){
    for(unsigned cycle=0;cycle<6;++cycle){
      setup(saved_sizes[s]);unsigned site=expected.return_sites[slot];
      assert(continuation_candidate(site,0x019104));assert(!continuation_candidate(site+1,0x019104));
      uint64_t id=arm();assert(id);rtl();assert(!take(site));continuation_finish_wait(id,1);
      before=cpu;assert(take(site));assert(!memcmp(&cpu,&before,sizeof(cpu)));assert(!take(site));++positives;
    }
    for(unsigned byte=1;byte<=5+saved_sizes[s];++byte){
      setup(saved_sizes[s]);uint64_t id=arm();assert(id);continuation_finish_wait(id,1);rtl();
      cpu.ram[0x1fe0+byte]^=1;before=cpu;assert(!take(expected.return_sites[slot]));
      assert(!memcmp(&cpu,&before,sizeof(cpu)));++mutants;
    }
    setup(saved_sizes[s]);uint64_t id=arm();continuation_finish_wait(id,1);rtl();++generation;
    assert(!take(expected.return_sites[slot]));
    setup(saved_sizes[s]);id=arm();continuation_finish_wait(id,1);rtl();source_ok=0;
    assert(!take(expected.return_sites[slot]));
  }
  setup(0);expected.return_sites[2]=expected.return_sites[3]=0;
  assert(interp_bridge_set_aot_continuations(&expected,1,guard));
  assert(!continuation_candidate(0,0x019104));assert(!continuation_candidate(0x009139,0x019104));
  InterpAotContinuation bad=expected;bad.return_sites[2]=0x009139;
  assert(!interp_bridge_set_aot_continuations(&bad,1,guard));
  assert(interp_bridge_set_aot_continuations(NULL,0,NULL));assert(!arm());
  printf("four-site actual SDK matcher: %u positive six-cycle takes, %u full-stack mutants; read-only, one-shot, source/generation and zero-padding refusals PASS\n",positives,mutants);
}
