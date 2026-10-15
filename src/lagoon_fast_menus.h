#ifndef LAGOON_FAST_MENUS_H
#define LAGOON_FAST_MENUS_H
#include <stddef.h>
#include <stdint.h>
#include "cpu_state.h"
void LagoonFastMenus_Reset(const uint8_t *rom, size_t size);
void LagoonFastMenus_Activate(void);
uint8_t LagoonFastMenus_Filter(uint32_t addr, uint8_t value,
    const CpuWramReadContext *context, const uint8_t *ram);
#endif
