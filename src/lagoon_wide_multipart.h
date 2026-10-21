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
 * finite shake table. Recover its source draw-time offset from an unambiguous
 * post-update phase. This grants no current/far pose or OAM/resource authority. */
int LagoonWideMultipart_PropDrawOffset(const uint8_t *rom,size_t rom_size,
    const uint8_t *packet,unsigned queue,int *draw_offset);
