#ifndef LAGOON_EPX_REFERENCE_H
#define LAGOON_EPX_REFERENCE_H
#include <stddef.h>
#include <stdint.h>

/* Test-only CPU oracle for the GPU EPX decision. Destination is exactly 2w by 2h.
 * Both pitches are bytes. Returns zero without writes for invalid arguments.
 * Input/output must not overlap. Bottom 32 rows retain nearest-neighbor HUD. */
int LagoonEpxReference2x(const uint32_t *source, size_t source_pitch,
    unsigned width, unsigned height, uint32_t *destination, size_t destination_pitch);
#endif
