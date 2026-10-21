#pragma once
#include "lagoon_wide_entities.h"

/* Original completed type3 carrier only. This decoder never advances actors,
 * predicts animation, extends culling, or grants graphics ownership. Caller
 * must bind every returned cell to the complete current native OAM packet,
 * exact room/camera/palette/generation and owned room/common0 resources. A
 * refused optional layer refuses the entire carrier. Output is transactional.
 * Source-proven authored handlers; composite kinds0/1 require primary slot540.
 * The original FF no-draw sentinel skips only that component. Unknown script
 * spawns11..13 and all unowned components still refuse the whole carrier.
 */
int LagoonWideMultipart_PacketCells(const uint8_t *rom,size_t rom_size,
    const uint8_t *packet,unsigned queue,LagoonWideCell *cells,size_t capacity);

/* Room40's queued authored prop only: C026 draws before D8DA advances the
 * finite shake table. Recover its draw-time offset at phases2..42; the exact
 * FF first phase offers only the resting-zero native-anchor candidate, which
 * must still match every original OAM cell. No current/far/resource authority. */
int LagoonWideMultipart_PropDrawOffset(const uint8_t *rom,size_t rom_size,
    const uint8_t *packet,unsigned queue,int *draw_offset);

/* Inactive phase42/current90=92=0 only. Source-derived terminal +1 candidate;
 * current whole-group native OAM must fail first. A nonempty complete native
 * group and all resource/camera/generation guards are still mandatory. */
int LagoonWideMultipart_PropTailDrawOffset(const uint8_t *rom,size_t rom_size,
    const uint8_t *packet,unsigned queue,int *draw_offset);
