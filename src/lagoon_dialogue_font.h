#ifndef LAGOON_DIALOGUE_FONT_H
#define LAGOON_DIALOGUE_FONT_H
#include <stddef.h>
#include <stdint.h>
struct Ppu;
/* Decode the owner's original resident font; no game artwork is bundled. */
void LagoonDialogueFont_Init(const uint8_t *rom,size_t size);
int LagoonDialogueFont_Capture(const struct Ppu *ppu);
int LagoonDialogueFont_Ready(void);
/* Destination pixel coordinates. Each native glyph is 8x16 before scale. */
void LagoonDialogueFont_DrawChar(uint32_t *dst,int stride,int height,int x,int y,
    unsigned char c,int scale);
/* Original eight-pixel border tiles, black interior. Destination dimensions. */
void LagoonDialogueFont_DrawBox(uint32_t *dst,int stride,int height,int x,int y,
    int width,int box_height,int scale);
#endif
