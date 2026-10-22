/* Extra-view eligibility for ORIGINAL ordinary-enemy AI and animation only.
 * No host AI, global actor-flag writes, collision or original OAM queue edits. */
#pragma once
#include <stddef.h>
#include <stdint.h>
struct CpuState;
struct CpuWramReadContext;
/* Call after original ROM identity verification and optional camera patch. */
void LagoonWideAI_Init(const uint8_t *rom,size_t size,const struct CpuState *actual_cpu);
/* Call before each real/speculative simulation frame, using the actual native
 * horizontal extension per side. A load/reset invalidates old read witnesses. */
void LagoonWideAI_BeginFrame(uint64_t generation,int extra,int world_ready);
/* Register pre-opcode hooks at02:973A and01:AB31. The bridge synchronizes the
 * actual CPU here; its public CPU container can lag between interpreter ops. */
void LagoonWideAI_ObserveOpcode(struct CpuState *cpu,uint32_t pc,uint64_t generation);
/* Compose through LagoonQol's existing world read filter. Does not own hooks. */
uint8_t LagoonWideAI_FilterRead(uint32_t address,uint8_t value,
    const struct CpuWramReadContext *context,const uint8_t *ram,uint64_t generation);
unsigned long long LagoonWideAI_AIReads(void);
unsigned long long LagoonWideAI_AnimationReads(void);
