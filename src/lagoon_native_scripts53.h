#pragma once
#include "cpu_state.h"
#include <stddef.h>
#include <stdint.h>
int LagoonNativeScripts53_Source(const uint8_t *rom, size_t size);
int LagoonNativeScripts53_Process(const uint8_t *rom, size_t size,
    CpuState *cpu, const CpuState *current, uint64_t generation,
    uint64_t frame_generation, uint32_t pc, uint32_t *landing);
void LagoonNativeScripts53_BeforeFrame(void);
