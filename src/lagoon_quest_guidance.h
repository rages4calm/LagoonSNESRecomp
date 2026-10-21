#ifndef LAGOON_QUEST_GUIDANCE_H
#define LAGOON_QUEST_GUIDANCE_H
#include <stddef.h>
#include <stdint.h>

typedef struct LagoonQuestGuidance {
  const char *title;
  const char *detail;
  uint16_t x, y;
  uint8_t room, marker;
} LagoonQuestGuidance;

/* Read-only presentation hints. Original scripts/items still own progression. */
void LagoonQuestGuidance_Init(const uint8_t *rom, size_t size);
int LagoonQuestGuidance_Get(const uint8_t *ram, LagoonQuestGuidance *out);
#endif
