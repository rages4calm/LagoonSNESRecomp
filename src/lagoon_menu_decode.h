#ifndef LAGOON_MENU_DECODE_H
#define LAGOON_MENU_DECODE_H
#include "cpu_state.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Transactional host implementation of the original dictionary expander.
 * Input/output storage is private until every bound/termination check passes. */
int LagoonMenuDecode_Expand(const uint8_t *rom,size_t size,uint8_t *ram,CpuState *cpu,
    unsigned *consumed,unsigned *produced);
bool LagoonMenuDecode_TryNative(CpuState *cpu,uint16_t entry_s,uint8_t hrv,
    uint32_t host_return_pc24,RecompReturn *out);
#endif
