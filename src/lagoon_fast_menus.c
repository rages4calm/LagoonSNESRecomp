/* Core submenu fade timing. The interpreter cursor and original nested
 * frames distinguish Items/Magic/Equipment opening and closing from shared
 * room-loader fades. DMA queues remain guest-owned. */
#include "lagoon_fast_menus.h"
#include <string.h>
#ifndef LAGOON_FAST_MENUS_TEST
#include "mod_runtime.h"
#endif
static int enabled, valid_rom;
static int match(const uint8_t *rom, size_t size, size_t offset,
    const uint8_t *bytes, size_t count) {
  return rom && offset <= size && count <= size-offset && !memcmp(rom+offset,bytes,count);
}
void LagoonFastMenus_Reset(const uint8_t *rom, size_t size) {
  enabled = 0;
  valid_rom =
    match(rom,size,0x4E0,(const uint8_t *)"\x8D\x3C\x02\xAD\x3C\x02\xD0\xFB\x6B",9) &&
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
uint8_t LagoonFastMenus_Filter(uint32_t addr, uint8_t value,
    const CpuWramReadContext *c, const uint8_t *ram) {
  if (!enabled || addr!=0x23C || value!=3 || !c || !ram ||
      !c->interpreter || c->instruction_pc!=0x0084E0 || c->pb ||
      !c->m || c->x || c->e || c->stack>0x1FF9) return value;
  uint32_t inner=return24(ram,c->stack+1),outer=return24(ram,c->stack+4);
  if ((inner==0x0083D8 && (outer==0x02B6B9 || outer==0x02B7E3 || outer==0x02BC44)) ||
      (inner==0x008471 && (outer==0x02B6E6 || outer==0x02B817 || outer==0x02BC65))) return 1;
  /* Real dispatcher PHB frames distinguish submenu close from ordinary room
   * loading through the very same world-loader fade routines. */
  if(c->stack<=0x1FEA && ram[c->stack+10]==1 && ram[c->stack+17]==2 &&
      return24(ram,c->stack+7)==0x0080E3 && return24(ram,c->stack+14)==0x0080E3) {
    uint32_t menu=return24(ram,c->stack+18);
    if(menu==0x02B73A||menu==0x02B87D||menu==0x02BC9B) {
      if(inner==0x0083D8 && outer==0x02C65E && return24(ram,c->stack+11)==0x018708)return 1;
      if(inner==0x008471 && outer==0x02C675 && return24(ram,c->stack+11)==0x018765)return 1;
    }
  }
  return value;
}
#ifndef LAGOON_FAST_MENUS_TEST
SNES_MOD_CONSTRUCTOR(lagoon_fast_menus_register) {
  snes_mod_register_activation_plugin("lagoon.qol.fast_menus", LagoonFastMenus_Activate);
}
#endif
