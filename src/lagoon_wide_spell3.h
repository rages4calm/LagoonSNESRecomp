#pragma once
#include <stddef.h>
#include <stdint.h>
typedef struct Ppu Ppu;

/* Finite original fullscreen BG spell. This owns no world or OBJ artwork.
 * Authority comes from completed, ordered original uploads, never a spell flag. */
void LagoonWideSpell3_Reset(void);
void LagoonWideSpell3_Dma(const uint8_t *rom,size_t size,const uint8_t *ram,
    uint64_t generation,unsigned channel,unsigned bank,unsigned source,
    unsigned destination,unsigned length,int original_mode);
void LagoonWideSpell3_Write(unsigned byte_address,uint8_t value);
/* Writes only outside the centered original256px image. Unknown presentation,
 * state generation, source, palette, layout or interrupted upload returns0. */
int LagoonWideSpell3_DrawMargins(uint8_t *dst,size_t pitch,unsigned width,
    unsigned height,const Ppu *ppu,const uint8_t *rom,size_t size,
    const uint8_t *ram,uint64_t generation);
