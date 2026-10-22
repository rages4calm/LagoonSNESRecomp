/* An ending witness, never a replacement for the original credits script.
 * $02:DE39 JSLs E99D; E99D saves/restores both16-bit indices, then JSRs
 * EDCB. Only the returned, balanced $02:E9BD terminal BRA completes it. */
#include "lagoon_achievement_ending.h"
#include "cpu_state.h"
#include <string.h>
static struct {
  const uint8_t *rom,*ram;
  const CpuState *cpu;
  size_t size;
  uint64_t generation;
  uint16_t entry_s;
  unsigned have_generation,armed,emitted;
} ending;
static uint64_t hash(unsigned pc,unsigned length){
  size_t at=((pc>>16)&127)*32768+(pc&32767);
  uint64_t h=UINT64_C(14695981039346656037);
  if(!ending.rom||ending.size!=0x100000||at>ending.size||length>ending.size-at)return 0;
  while(length--)h=(h^ending.rom[at++])*UINT64_C(1099511628211);
  return h;
}
static int source_ok(void){
  return hash(0x02e99d,0x22)==UINT64_C(0x258794c19ee333ec)&&
    hash(0x02edcb,0x1d7)==UINT64_C(0xa2260183f9fce054)&&
    hash(0x02de39,6)==UINT64_C(0x2996a96516b80f2a);
}
void LagoonAchievementEnding_Reset(uint64_t generation){
  ending.generation=generation;ending.have_generation=1;
  ending.armed=ending.emitted=0;ending.cpu=NULL;ending.ram=NULL;ending.entry_s=0;
}
void LagoonAchievementEnding_Init(const uint8_t *rom,size_t size){
  memset(&ending,0,sizeof ending);ending.rom=rom;ending.size=size;
}
static int frame_ok(const CpuState *c){
  /* S+1..3 must remain in bank-zero WRAM's linear low8KiB mirror.
   * The entry also needs four available bytes for its immediate PHY/PHX.
   * DB is deliberately unconstrained: the real credits helpers change it. */
  return c&&c->ram&&c->PB==2&&!c->D&&!c->emulation&&
    c->m_flag==1&&!c->x_flag&&!c->_flag_D&&
    (c->P&(CPU_P_M|CPU_P_X|CPU_P_D))==CPU_P_M&&
    c->S>=4&&c->S<=0x1ffc&&
    c->ram[c->S+1]==0x3c&&c->ram[c->S+2]==0xde&&c->ram[c->S+3]==2;
}
int LagoonAchievementEnding_Observe(const CpuState *c,uint32_t pc,uint64_t generation){
  if(!ending.have_generation||generation!=ending.generation)LagoonAchievementEnding_Reset(generation);
  if(pc!=0x02e99d&&pc!=0x02e9bd)return 0;
  if(ending.emitted)return 0;
  if(!source_ok()||!frame_ok(c)){ending.armed=0;return 0;}
  if(pc==0x02e99d){
    ending.cpu=c;ending.ram=c->ram;ending.entry_s=c->S;ending.armed=1;return 0;
  }
  /* E99D's STZ/INC/LDA/CMP are M8 byte operations. $109A is the separate
   * credits palette-animation counter (INC at02:ED91), not a high counter byte.
   * Requiring it to be zero rejects real completed campaigns. */
  int valid=ending.armed&&ending.cpu==c&&ending.ram==c->ram&&ending.entry_s==c->S&&
    c->ram[0x1099]==7;
  ending.armed=0;
  if(valid){ending.emitted=1;return 1;}
  return 0;
}
