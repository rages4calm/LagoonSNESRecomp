/* An ending witness, never a replacement for the original credits script.
 * $02:DE39 JSLs E99D; E99D saves/restores both16-bit indices, then JSRs
 * EDCB. The final-Red handler also enters through its real bank2 dispatcher.
 * Only the returned, balanced $02:E9BD terminal BRA completes either path. */
#include "lagoon_achievement_ending.h"
#include "cpu_state.h"
#include <string.h>
static struct {
  const uint8_t *rom,*ram;
  const CpuState *cpu;
  size_t size;
  uint64_t generation;
  uint16_t entry_s;
  uint8_t frame_kind,frame[9];
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
static int alternate_source_ok(void){
  /* Selector10, its exact bank2 target, the real wrapper and outer ancestry. */
  return hash(0x01b90b,14)==UINT64_C(0xd3ba42206fe3632e)&&
    hash(0x0080b9,0x30)==UINT64_C(0x37f9de38ea6c1480)&&
    hash(0x028020,2)==UINT64_C(0x0a25b307b6933173)&&
    hash(0x01b816,4)==UINT64_C(0x6b018774562bf2df);
}
void LagoonAchievementEnding_Reset(uint64_t generation){
  ending.generation=generation;ending.have_generation=1;
  ending.armed=ending.emitted=0;ending.cpu=NULL;ending.ram=NULL;ending.entry_s=0;
  ending.frame_kind=0;memset(ending.frame,0,sizeof ending.frame);
}
void LagoonAchievementEnding_Init(const uint8_t *rom,size_t size){
  memset(&ending,0,sizeof ending);ending.rom=rom;ending.size=size;
}
static unsigned frame_kind(const CpuState *c){
  /* S+1..3 must remain in bank-zero WRAM's linear low8KiB mirror.
   * The entry also needs four available bytes for its immediate PHY/PHX.
   * DB is deliberately unconstrained: the real credits helpers change it. */
  if(!(c&&c->ram&&c->PB==2&&!c->D&&!c->emulation&&
    c->m_flag==1&&!c->x_flag&&!c->_flag_D&&
    (c->P&(CPU_P_M|CPU_P_X|CPU_P_D))==CPU_P_M&&
    c->S>=4&&c->S<=0x1ffc))return 0;
  static const uint8_t direct[]={0x3c,0xde,2};
  static const uint8_t alternate[]={0xe3,0x80,0,1,0x16,0xb9,1,0x18,0xb8};
  if(!memcmp(c->ram+c->S+1,direct,sizeof direct))return 1;
  /* Both physical JSLs, saved DB and actual B816 JSR ancestry stay intact.
   * Scratch dispatcher0010..13 is intentionally not persistent state. */
  if(c->S<=0x1ff6&&!memcmp(c->ram+c->S+1,alternate,sizeof alternate)&&
     alternate_source_ok())return 2;
  return 0;
}
int LagoonAchievementEnding_Observe(const CpuState *c,uint32_t pc,uint64_t generation){
  if(!ending.have_generation||generation!=ending.generation)LagoonAchievementEnding_Reset(generation);
  if(pc!=0x02e99d&&pc!=0x02e9bd)return 0;
  if(ending.emitted)return 0;
  unsigned kind=source_ok()?frame_kind(c):0;
  if(!kind){ending.armed=0;return 0;}
  if(pc==0x02e99d){
    ending.cpu=c;ending.ram=c->ram;ending.entry_s=c->S;ending.frame_kind=(uint8_t)kind;
    memcpy(ending.frame,c->ram+c->S+1,kind==1?3:9);ending.armed=1;return 0;
  }
  /* E99D's STZ/INC/LDA/CMP are M8 byte operations. $109A is the separate
   * credits palette-animation counter (INC at02:ED91), not a high counter byte.
   * Requiring it to be zero rejects real completed campaigns. */
  int valid=ending.armed&&ending.cpu==c&&ending.ram==c->ram&&ending.entry_s==c->S&&
    ending.frame_kind==kind&&!memcmp(ending.frame,c->ram+c->S+1,kind==1?3:9)&&
    c->ram[0x1099]==7;
  ending.armed=0;
  if(valid){ending.emitted=1;return 1;}
  return 0;
}
