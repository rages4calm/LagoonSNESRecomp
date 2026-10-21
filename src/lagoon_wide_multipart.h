#pragma once
#include "lagoon_wide_entities.h"

/* Original completed type3 carrier only. This decoder never advances actors,
 * predicts animation, extends culling, or grants graphics ownership. Caller
 * must bind every returned cell to the complete current native OAM packet,
 * exact room/camera/palette/generation and owned room/common0 resources. A
 * refused optional layer refuses the entire carrier. Output is transactional.
 * Actual source-proven simple handlers only; kinds0/1 and scripted11..13 floor.
 */
int LagoonWideMultipart_PacketCells(const uint8_t *rom,size_t rom_size,
    const uint8_t *packet,unsigned queue,LagoonWideCell *cells,size_t capacity);
