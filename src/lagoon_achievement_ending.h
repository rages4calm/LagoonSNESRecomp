#ifndef LAGOON_ACHIEVEMENT_ENDING_H
#define LAGOON_ACHIEVEMENT_ENDING_H
#include <stddef.h>
#include <stdint.h>
struct CpuState;
/* Read-only observation of the original ending entry and terminal opcode.
 * The caller registers pre-opcode observations; this module owns no hooks. */
void LagoonAchievementEnding_Init(const uint8_t *rom,size_t size);
void LagoonAchievementEnding_Reset(uint64_t generation);
int LagoonAchievementEnding_Observe(const struct CpuState *cpu,uint32_t pc,uint64_t generation);
#endif
