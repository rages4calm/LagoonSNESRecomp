#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct LagoonWideCell {
  int x, y;
  uint16_t tile;
  uint8_t size;
} LagoonWideCell;

/* ROM-free tests supply synthetic LoROM bytes. No guest code is executed. */
int LagoonWideEntities_Decode(const uint8_t *rom, size_t rom_size,
    unsigned selector, unsigned frame, int x, int y,
    LagoonWideCell *cells, size_t capacity);
void LagoonWideEntities_Init(void);
int LagoonWideEntities_Pixels(const uint16_t *vram, const uint16_t *cgram,
    unsigned base1, unsigned base2, const LagoonWideCell *cell,
    uint16_t *pixels, int clip_native);
void LagoonWideEntities_Reset(void);
void LagoonWideEntities_BeginFrame(int enabled);
