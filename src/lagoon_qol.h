#ifndef LAGOON_QOL_H
#define LAGOON_QOL_H

/* Owner-requested quality-of-life features (see lagoon_qol.c for what each one
 * does and how it was proven). The ROM hook is wired through the desktop host's
 * game descriptor and runs on the loaded image before the machine exists, which
 * is the point where a data patch is visible to both execution tiers. Camera
 * instruction operands require the explicit interpreter-only generation guard. */

#include <stddef.h>
#include <stdint.h>

/* Clear host-only hold counters after a guest timeline discontinuity. The
 * implementation also observes RtlStateGeneration before CPU writes/frame ticks. */
void LagoonQol_ResetTransient(void);

void LagoonQol_OnRomLoaded(const uint8_t *rom, size_t size);

#endif /* LAGOON_QOL_H */
