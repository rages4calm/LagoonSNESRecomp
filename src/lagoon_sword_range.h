#pragma once
#include <stddef.h>
#include <stdint.h>

enum { LAGOON_SWORD_RANGE_SIDE=48 };
typedef struct LagoonSwordRangeImage {
  int x,y;
  unsigned width,height;
  uint16_t pixels[LAGOON_SWORD_RANGE_SIDE*LAGOON_SWORD_RANGE_SIDE];
} LagoonSwordRangeImage;
/* Pure geometry/image builder. Caller must qualify the actual active weapon
 * pose before drawing. Reads the current original-or-QoL hit rectangles; never
 * changes ROM/RAM or grants a hit. Coordinates are the verified hero screen
 * anchor; the PPU performs ordinary viewport/foreground/window clipping. */
int LagoonSwordRange_Build(const uint8_t *rom,size_t size,unsigned facing,
    int hero_x,int hero_y,LagoonSwordRangeImage *out);
