#pragma once
#include "cpu_state.h"
#include "common_cpu_infra.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Original bank02 direction/velocity commands and bank00 dispatch prefix,
 * stopping before the real original0080E5 RTL. No guest clocks are fabricated. */
int LagoonNativeActorCommands_Source(const uint8_t *rom,size_t size);
int LagoonNativeActorCommands_Apply(const uint8_t *rom,size_t size,uint8_t *ram,
    CpuState *cpu,unsigned selector);
int LagoonNativeActorCommands_Try(const uint8_t *rom,size_t size,CpuState *cpu,
    uint64_t generation,uint64_t frame_generation,int normal_world);
void LagoonNativeActorCommands_BeforeFrame(void);

/* Preserve the established guarded zero-clock prefix when B9 is compiled.
 * The original80E5RTL owns the unchanged physical3-byte caller frame. */
bool LagoonNativeActorCommands_Continue(const uint8_t *rom,size_t size,CpuState *cpu,
    uint16_t entry_s,uint8_t hrv,uint32_t host_return,RecompReturn *out,
    uint64_t generation,uint64_t frame_generation,int normal_world);
bool LagoonNativeActorCommands_Run(CpuState *cpu,uint16_t entry_s,uint8_t hrv,
    uint32_t host_return,RecompReturn *out);
