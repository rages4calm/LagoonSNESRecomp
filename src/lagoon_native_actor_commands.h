#pragma once
#include "cpu_state.h"
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
