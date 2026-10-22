#pragma once
#include "cpu_state.h"
#include <stddef.h>
#include <stdint.h>
/* Only original inactive-slot checks/index advancement. Actual actor children,
 * draw sorting, audio, waits and terminal return keep their original owners. */
int LagoonNativeIterator52_Source(const uint8_t *rom, size_t size);
int LagoonNativeIterator52_Process(const uint8_t *rom, size_t size, CpuState *cpu,
    const CpuState *current, uint64_t generation, uint64_t frame_generation,
    uint32_t pc, uint32_t *landing);
void LagoonNativeIterator52_BeforeFrame(void);
