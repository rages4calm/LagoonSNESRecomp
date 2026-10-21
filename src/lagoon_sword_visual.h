#pragma once
#include <stddef.h>
#include <stdint.h>
struct Ppu;
/* Call after ordinary title extra objects are prepared, before PPU scanout.
 * The real weapon OAM/staging source must match completely. Appends only host
 * pixels with the original object's priority/math/order. No guest mutations. */
int LagoonSwordVisual_Prepare(struct Ppu *ppu,const uint8_t *rom,size_t size,
    const uint8_t *ram,int enabled);
