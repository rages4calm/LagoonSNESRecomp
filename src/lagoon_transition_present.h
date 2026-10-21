#pragma once
#include <stdint.h>
#include <stddef.h>
/* A complete outgoing presentation, used only while a loader owns the game.
 * No guest state, inferred map geometry, or partial incoming pixels are cached. */
void LagoonTransition_Reset(void);
int LagoonTransition_Apply(uint8_t *dst,size_t pitch,unsigned width,unsigned height,
    uint64_t generation,int coherent,int loading,unsigned brightness);
