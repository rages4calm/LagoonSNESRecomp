/* Diagnostic input only: original NPC interaction, then one script alias.
 * No PC/stack/flags/request/outcome writes. Unknown inputs stay original. */
#pragma once
#include "cpu_state.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
static unsigned LagoonScriptFixture_Parse(const char *v) {
  return v && !strcmp(v,"30") ? 30 : v && !strcmp(v,"39") ? 39 : 0;
}
static uint64_t LagoonScriptFixture_Hash(const uint8_t *rom,unsigned pc,unsigned n) {
  uint64_t h=UINT64_C(1469598103934665603);
  unsigned at=((pc>>16)&127)*32768+(pc&32767);
  for(unsigned i=0;i<n;i++)h=(h^rom[at+i])*UINT64_C(1099511628211);
  return h;
}
static int LagoonScriptFixture_Apply(const uint8_t *rom,size_t size,
    CpuState *c,const CpuState *current,uint64_t generation,uint64_t frame,
    unsigned selector,unsigned *used) {
  if(!rom||size!=0x100000||!c||c!=current||!c->ram||!used||*used||
     (selector!=30&&selector!=39)||generation!=frame||
     c->PB!=1||c->DB!=1||c->D||!c->m_flag||c->x_flag||c->emulation||c->_flag_D||
     (c->P&(CPU_P_M|CPU_P_X|CPU_P_D))!=CPU_P_M||
     c->S<0x100||c->S>0x1ffd||c->X||c->Y<0x40||c->Y>0x800||(c->Y&63))return 0;
  uint8_t *r=c->ram;
  if(r[c->S+1]!=0x9c||r[c->S+2]!=0x80||r[0x9c]||r[0x9d]!=0||
     (r[0x500+c->Y]&0xc8)!=0xc8||r[0x518+c->Y]!=1||r[0x51b+c->Y]!=0||
     (r[0x1030]|r[0x1031]<<8)!=c->Y||r[0xa1]!=rom[0x2c347]||
     r[0xa2]!=rom[0x2c348]||c->A!=rom[0x2c348])return 0;
  const unsigned pc[]={0x01809a,0x019b34,0x01cc7b,0x0080e9,0x05c347,0x05c6af,0x05cab1};
  const unsigned len[]={3,0x88,0x25,32,2,2,2};
  const uint64_t hashes[]={UINT64_C(0xBDDE924FD713E3A4),UINT64_C(0x84E6C601C79062BB),
    UINT64_C(0xD66CA320EB8EA947),UINT64_C(0xD98520CA8A4BEFBC),UINT64_C(0x9A510C00C5341B35),
    UINT64_C(0x9AC0E000C592D828),UINT64_C(0x9AD8E000C5A76A09)};
  for(unsigned i=0;i<7;i++)if(LagoonScriptFixture_Hash(rom,pc[i],len[i])!=hashes[i])return 0;
  unsigned p=selector==30?0xc6af:0xcab1;
  r[0xa1]=(uint8_t)p;r[0xa2]=(uint8_t)(p>>8);*used=1;return 1;
}
