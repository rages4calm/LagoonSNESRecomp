/* Core submenu fade timing. The interpreter cursor and original nested
 * frames distinguish Items/Magic/Equipment opening and closing from shared
 * room-loader fades. DMA queues remain guest-owned. */
#include "lagoon_fast_menus.h"
#include <string.h>
#ifndef LAGOON_FAST_MENUS_TEST
#include "mod_runtime.h"
#endif
static int enabled, valid_rom, valid_door_rom;
static int match(const uint8_t *rom, size_t size, size_t offset,
    const uint8_t *bytes, size_t count) {
  return rom && offset <= size && count <= size-offset && !memcmp(rom+offset,bytes,count);
}
void LagoonFastMenus_Reset(const uint8_t *rom, size_t size) {
  enabled = 0;
  /* Door timing is independent of the submenu contracts below. Only the real
   * main-loop door ->8772 ->8703 ->1B/1C fade call chain may use it. */
  valid_door_rom =
    match(rom,size,0x878A,(const uint8_t *)"\xA9\xFF\x8D\x0E\x10\x22\x03\x87\x01\x60",10) &&
    match(rom,size,0xC8E2,(const uint8_t *)"\x20\x72\x87",3) &&
    match(rom,size,0x80A0,(const uint8_t *)"\x20\xF6\xC7",3) &&
    match(rom,size,0x1465B,(const uint8_t *)"\x22\xC4\x83\x00\x6B",5) &&
    match(rom,size,0x14672,(const uint8_t *)"\x22\x58\x84\x00\x6B",5);
  valid_rom =
    match(rom,size,0x4E0,(const uint8_t *)"\x8D\x3C\x02\xAD\x3C\x02\xD0\xFB\x6B",9) &&
    match(rom,size,0x91B,(const uint8_t *)"\xAD\x3C\x02\x3A\x8D\x3C\x02",7) &&
    match(rom,size,0x3C4,(const uint8_t *)"\xAD\x3D\x02\x29\x80\xC9\x80\xD0\x01\x6B\xAD\x75\x02\x10\x14\xA9\x03\x22\xE0\x84\x00\xAD\x3D\x02\x29\x0F\x3A\x8D\x3D\x02\x8D\x00\x21\xD0\xEC\xA9\x80\x8D\x3D\x02\x8D\x00\x21\x6B",44) &&
    match(rom,size,0x458,(const uint8_t *)"\xAD\x3D\x02\xC9\x0F\xD0\x01\x6B\x22\x12\x81\x00\xA9\x00\x8D\x3D\x02\x8D\x00\x21\xA9\x03\x22\xE0\x84\x00\xAD\x3D\x02\x1A\x8D\x3D\x02\x8D\x00\x21\xC9\x0F\xD0\xEC\x6B",41) &&
    match(rom,size,0x3D3,(const uint8_t *)"\xA9\x03\x22\xE0\x84\x00",6) &&
    match(rom,size,0x46C,(const uint8_t *)"\xA9\x03\x22\xE0\x84\x00",6) &&
    match(rom,size,0x136B6,(const uint8_t *)"\x22\xC4\x83\x00",4) &&
    match(rom,size,0x136E3,(const uint8_t *)"\x22\x58\x84\x00",4) &&
    match(rom,size,0x137E0,(const uint8_t *)"\x22\xC4\x83\x00",4) &&
    match(rom,size,0x13814,(const uint8_t *)"\x22\x58\x84\x00",4) &&
    match(rom,size,0x13C41,(const uint8_t *)"\x22\xC4\x83\x00",4) &&
    match(rom,size,0x13C62,(const uint8_t *)"\x22\x58\x84\x00",4) &&
    match(rom,size,0x1465B,(const uint8_t *)"\x22\xC4\x83\x00",4) &&
    match(rom,size,0x14672,(const uint8_t *)"\x22\x58\x84\x00",4) &&
    match(rom,size,0x8703,(const uint8_t *)"\xA9\x1B\x22\xB9\x80\x00",6) &&
    match(rom,size,0x8760,(const uint8_t *)"\xA9\x1C\x22\xB9\x80\x00",6) &&
    match(rom,size,0x13737,(const uint8_t *)"\x22\xB3\x80\x00",4) &&
    match(rom,size,0x1387A,(const uint8_t *)"\x22\xB3\x80\x00",4) &&
    match(rom,size,0x13C98,(const uint8_t *)"\x22\xB3\x80\x00",4);
  enabled = valid_rom;
}
/* Legacy package aliases may still invoke this; core speed is already active. */
void LagoonFastMenus_Activate(void) { enabled = valid_rom; }
static uint32_t return24(const uint8_t *ram, unsigned address) {
  return ram[address] | ((uint32_t)ram[address+1]<<8) | ((uint32_t)ram[address+2]<<16);
}
static unsigned word(const uint8_t *r,unsigned a) {return r[a]|r[a+1]<<8;}
static int owned_door(const CpuWramReadContext *c,const uint8_t *ram) {
  if(!valid_door_rom || c->stack<0x100 || c->stack>0x1EA ||
      ram[0x100E]!=255 || word(ram,0x48E)>=75 ||
      (ram[0x500]&0xC0)!=0xC0 || !ram[0x520] || ram[0x521])return 0;
  unsigned s=c->stack,b=ram[0x23D];
  if(return24(ram,s+7)!=0x0080E3 || ram[s+10]!=1 ||
      return24(ram,s+14)!=0x018792 || word(ram,s+17)!=0xC8E4 ||
      word(ram,s+19)!=0x80A2)return 0;
  if(return24(ram,s+1)==0x0083D8 && return24(ram,s+4)==0x02C65E &&
      return24(ram,s+11)==0x018708 && b>=1 && b<=15)return 1;
  return return24(ram,s+1)==0x008471 && return24(ram,s+4)==0x02C675 &&
      return24(ram,s+11)==0x018765 && b<=14;
}
/* The opening has direct JSL fade callers rather than the world-loader's
 * dispatcher frames. Pin each direction and its original legal brightness
 * range; never authorize shared room/cutscene fade callers. */
static int owned_open(const CpuWramReadContext *c,const uint8_t *ram) {
  if(c->stack<0x100||c->stack>0x1F9)return 0;
  uint32_t inner=return24(ram,c->stack+1),outer=return24(ram,c->stack+4);
  unsigned b=ram[0x23D];
  if(inner==0x0083D8&&b>=1&&b<=15)
    return outer==0x02B6B9||outer==0x02B7E3||outer==0x02BC44;
  if(inner==0x008471&&b<=14)
    return outer==0x02B6E6||outer==0x02B817||outer==0x02BC65;
  return 0;
}
static int owned_close(const CpuWramReadContext *c,const uint8_t *ram) {
  if(c->stack<0x100||c->stack>0x1EA)return 0;
  uint32_t inner=return24(ram,c->stack+1),outer=return24(ram,c->stack+4);
  /* Real dispatcher PHB frames distinguish submenu close from ordinary room
   * loading through the very same world-loader fade routines. */
  if(c->stack>=0x100 && c->stack<=0x1EA && ram[c->stack+10]==1 && ram[c->stack+17]==2 &&
      return24(ram,c->stack+7)==0x0080E3 && return24(ram,c->stack+14)==0x0080E3) {
    uint32_t menu=return24(ram,c->stack+18);
    if(menu==0x02B73A||menu==0x02B87D||menu==0x02BC9B) {
      unsigned brightness=ram[0x23D];
      if(inner==0x0083D8 && outer==0x02C65E && return24(ram,c->stack+11)==0x018708 && brightness>=1 && brightness<=15)return 1;
      if(inner==0x008471 && outer==0x02C675 && return24(ram,c->stack+11)==0x018765 && brightness<=14)return 1;
    }
  }
  return 0;
}
uint8_t LagoonFastMenus_Filter(uint32_t addr,uint8_t value,const CpuWramReadContext*c,const uint8_t*ram){
  if(!enabled||addr!=0x23C||value!=3||!c||!ram||c->pb||!c->m||c->x||c->e||c->stack>0x1FF9)return value;
  /* Generated84E0 has no instruction cursor. Its exact physical ordinary-door
   * frame separately owns this sole timer store; menu ownership stays as before. */
  if(c->interpreter?c->instruction_pc!=0x0084E0:!owned_door(c,ram))return value;
  uint32_t inner=return24(ram,c->stack+1),outer=return24(ram,c->stack+4);
  if((inner==0x0083D8&&(outer==0x02B6B9||outer==0x02B7E3||outer==0x02BC44))||
     (inner==0x008471&&(outer==0x02B6E6||outer==0x02B817||outer==0x02BC65))||owned_close(c,ram)||owned_door(c,ram))return 1;
  return value;
}
uint8_t LagoonFastMenus_FilterRead(uint32_t addr,uint8_t value,const CpuWramReadContext*c,const uint8_t*ram){
  /* Keep a real counter1: the original NMI decrements unconditionally. Only
   * the exact owned opening/closing fade read may see zero early. If NMI already made1->0,
   * zero remains zero. Values above1 are never concealed or rewritten. */
  if(!enabled||addr!=0x23C||value>1||!c||!ram||!c->interpreter||c->instruction_pc!=0x0084E3||c->pb||!c->m||c->x||c->e||(!owned_close(c,ram)&&!owned_open(c,ram)&&!owned_door(c,ram)))return value;
  return ram[0x23D]%3?0:value;
}
#ifndef LAGOON_FAST_MENUS_TEST
SNES_MOD_CONSTRUCTOR(lagoon_fast_menus_register) {
  snes_mod_register_activation_plugin("lagoon.qol.fast_menus", LagoonFastMenus_Activate);
}
#endif
