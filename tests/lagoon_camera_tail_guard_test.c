/* Actual title token AND extracted actual SDK post-JMP owner matcher.
 * Synthetic CPU/stack/source mutants never write the protected game inputs. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/lagoon_native_camera.c"
static uint8_t rom[0x100000],ram[0x20000],snapshot[0x20000];
const uint8_t *g_rom;
static uint64_t generation=1;
uint64_t RtlStateGeneration(void){return generation;}
#include "camera_policy_under_test.h"
static CpuState cpu;
static unsigned checks;
static void observe(unsigned e,const CpuState*c,uint32_t source,uint32_t target,
    const InterpAotContinuation*d,uint16_t owner,InterpAotAncestor*out) {
  (void)out;LagoonNativeCameraObserve(e,c,source,target,d,owner);
}
static int adapter(unsigned e,const CpuState*c,uint32_t source,uint32_t target,
    const InterpAotContinuation*d,uint16_t owner,InterpAotAncestor*out) {
  observe(e,c,source,target,d,owner,out);return 0;
}
static void setup(void) {
  memset(&cpu,0,sizeof(cpu));memset(ram,0,sizeof(ram));generation=1;g_rom=rom;
  cpu.ram=ram;cpu.S=0x1fe0;cpu.PB=cpu.DB=1;cpu.m_flag=1;cpu.P=CPU_P_M;
  ram[cpu.S+1]=0xCC;ram[cpu.S+2]=0x80;
  continuation_discard_tokens();
  interp_bridge_set_aot_continuation_hook(adapter);
  LagoonNativeCameraBeforeFrame();
}
static void arm(void) {
  memcpy(snapshot,ram,sizeof(ram));CpuState before=cpu;
  /* Existing production real-call notification. No fake wait token is made. */
  assert(!continuation_arm(&cpu,0x0183D8,0x0180CA,2,1));
  assert(!memcmp(snapshot,ram,sizeof(ram))&&!memcmp(&before,&cpu,sizeof(cpu)));checks++;
}
static int take_at(uint32_t source,uint32_t target,InterpAotAncestor *out) {
  memcpy(snapshot,ram,sizeof(ram));CpuState before=cpu;
  int result=continuation_tail(&cpu,source,target,out);
  assert(!memcmp(snapshot,ram,sizeof(ram))&&!memcmp(&before,&cpu,sizeof(cpu)));checks++;
  return result;
}
static int take(void) {
  InterpAotAncestor out={0};int result=take_at(0x0183F3,0x01A7B5,&out);
  if(result)assert(out.pc24==0x01A7B5&&out.owner_return_pc24==0x0180CD&&
    out.entry_s==cpu.S&&out.owner_frame==2);
  return result;
}
static void mutate(unsigned index) {
  switch(index) {
    case 0:cpu.PB=0x81;break;case 1:cpu.DB=0;break;
    case 2:cpu.D=1;break;case 3:cpu.m_flag=0;break;
    case 4:cpu.x_flag=1;break;case 5:cpu.emulation=1;break;
    case 6:cpu.S++;break;case 7:cpu.S=0x1ffe;break;
    case 8:generation++;break;case 9:cpu.ram=NULL;break;
    case 10:cpu._flag_D=1;break;case 11:cpu.P|=CPU_P_D;break;
    case 12:cpu.P&=~CPU_P_M;break;case 13:cpu.P|=CPU_P_X;break;
  }
}
static unsigned proposal_mutant;
static int forged(const CpuState*c,uint32_t source,uint32_t target,InterpAotAncestor*out) {
  (void)source;*out=(InterpAotAncestor){target,0x0180CD,c->S,2};
  switch(proposal_mutant) {
    case 0:out->pc24++;break;case 1:out->owner_frame=3;break;
    case 2:out->entry_s++;break;case 3:out->owner_return_pc24=0x0280CD;break;
    case 4:out->owner_return_pc24=0x0180CE;break;
    case 5:out->owner_return_pc24=0x10080CD;break;
  }
  return 1;
}
static int valid_long_proposal(const CpuState*c,uint32_t source,uint32_t target,InterpAotAncestor*out) {
  (void)source;*out=(InterpAotAncestor){target,0x0180CD,c->S,2};return 1;
}
int main(int argc,char **argv) {
  assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);
  const uint8_t center[]={0x5A,0x9B,0xD0,0x16,0xDA,0x64,0x41,
    0x20,0xFB,0x83,0x20,0x26,0x84,0x20,0x52,0x84,0x20,0x7D,0x84,
    0xFA,0xAD,0x19,0x05,0x8D,0x19,0x05,0x7A,0x4C,0xB5,0xA7,0xEA,0xEA,0xEA,0xEA,0xEA};
  setup();assert(!camera_source());arm();assert(!take()); /* Stock remains interpreted. */
  memset(rom+0x809D,0xEA,3);rom[0x80CB]=0xD8;rom[0x80CC]=0x83;
  memcpy(rom+0x83D8,center,sizeof(center));
  rom[0x8408]=rom[0x8432]=128;rom[0x845F]=rom[0x8489]=112;rom[0x87F6]=128;
  setup();assert(camera_source());assert(!take()); /* Forged owner bytes alone grant nothing. */
  arm();assert(camera.phase==1);assert(take());assert(!take());
  setup();arm();assert(take());
  continuation_event(INTERP_CONT_TAKE,&cpu,0x0183F3,0x01A7B5,NULL,cpu.S);
  assert(stats.takes==1&&!camera.phase);assert(!take());
  /* Complete patched main/helpers/selector byte coverage at arm AND take. */
  const unsigned spans[][2]={{0x807A,0x53},{0x8218,0x4EB},{0xA7B5,0x21}};
  for(unsigned s=0;s<3;s++)for(unsigned b=0;b<spans[s][1];b++) {
    unsigned at=spans[s][0]+b;
    setup();rom[at]^=1;arm();assert(!take());rom[at]^=1;
    setup();arm();rom[at]^=1;assert(!take());rom[at]^=1;
  }
  for(unsigned b=0;b<2;b++) {
    setup();ram[cpu.S+1+b]^=1;arm();assert(!take());
    setup();arm();ram[cpu.S+1+b]^=1;assert(!take());
  }
  for(unsigned m=0;m<14;m++) {
    setup();mutate(m);arm();assert(m==8 ? take() : !take()); /* New real call owns its current generation. */
    setup();arm();mutate(m);assert(!take());
  }
  setup();arm();continuation_discard_tokens();assert(!take());
  setup();arm();continuation_arm(&cpu,0x0183D9,0x0180CA,2,1);assert(!take());
  setup();continuation_arm(&cpu,0x0183D8,0x0180CB,2,1);assert(!take());
  setup();arm();InterpAotAncestor out={0};assert(!take_at(0x0183F2,0x01A7B5,&out));assert(take());
  setup();arm();assert(!take_at(0x0183F3,0x01A7B6,&out));assert(!take());
  setup();arm();assert(!take_at(0x0183F3,0x01A7B5,NULL));assert(take()); /* Null output is rejected before title proposal. */
  setup();arm();CpuState other=cpu;assert(!continuation_tail(&other,0x0183F3,0x01A7B5,&out));checks++;assert(!take());
  /* Actual SDK owner checks also reject proposals from a malicious title hook. */
  for(proposal_mutant=0;proposal_mutant<6;proposal_mutant++) {
    setup();assert(interp_bridge_set_aot_tail_hook(0x0183F3,forged));assert(!take());
  }
  setup();arm();assert(interp_bridge_set_aot_tail_hook(0,NULL));assert(!take());
  assert(!interp_bridge_set_aot_tail_hook(0x1000000,forged));checks++;
  /* Both explicitly different jump channels coexist; all other opcodes/sites
   * remain off. This checks actual SDK routing, not a duplicate title policy. */
  setup();arm();assert(interp_bridge_set_aot_indirect_long_tail_hook(0x008106,valid_long_proposal));
  const unsigned sites[]={0x0183F3,0x008106,0x008105,0x0183F2};
  for(unsigned op=0;op<256;op++)for(unsigned site=0;site<4;site++) {
    assert(continuation_tail_site(op,sites[site])==
      ((op==0x4C&&site==0)||(op==0xDC&&site==1)));checks++;
  }
  assert(take());checks++;
  setup();arm();assert(continuation_indirect_long_tail(&cpu,0x008106,0x01A7B5,&out));
  assert(out.owner_frame==2&&out.entry_s==cpu.S&&out.owner_return_pc24==0x0180CD);checks++;
  assert(take()); /* DC matching did not replace or consume camera's token. */
  for(proposal_mutant=0;proposal_mutant<6;proposal_mutant++) {
    setup();assert(interp_bridge_set_aot_indirect_long_tail_hook(0x008106,forged));
    assert(!continuation_indirect_long_tail(&cpu,0x008106,0x01A7B5,&out));checks++;
  }
  setup();arm();assert(interp_bridge_set_aot_indirect_long_tail_hook(0,NULL));
  assert(!continuation_tail_site(0xDC,0x008106));assert(take());checks++;
  assert(!interp_bridge_set_aot_indirect_long_tail_hook(0x1000000,forged));checks++;
  printf("PASS %u actual title-camera/SDK-owner guard checks; full patched-source mutants, real-CALL witness, exact owner bytes/SP/CPU/RAM/mode/generation, one-shot/refused/unset hooks and independent4C/DC opcode channels; guest state read-only\n",checks);
  return 0;
}
