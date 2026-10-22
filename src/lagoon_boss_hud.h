#ifndef LAGOON_BOSS_HUD_H
#define LAGOON_BOSS_HUD_H
#include <stddef.h>
#include <stdint.h>
struct Ppu;
void LagoonBossHud_Init(const uint8_t *rom,size_t size);
/* Freeze this completed field's actual phase health, graphics and underlay.
 * No guest reads through the CPU bus, writes, callbacks or simulated opcodes. */
void LagoonBossHud_ObserveField(const struct Ppu *ppu,const uint8_t *ram,
    uint64_t generation,unsigned frame,int hdma);
/* All1024 original source pixels must match before changing only128x8 pixels
 * of the centered ENEMY bar. Works at native width as well as widescreen. */
int LagoonBossHud_Draw(uint8_t *dst,size_t pitch,const uint8_t *field,
    size_t source_pitch,unsigned width,unsigned height,uint64_t generation);
#endif
