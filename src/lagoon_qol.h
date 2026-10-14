#ifndef LAGOON_QOL_H
#define LAGOON_QOL_H

/* Owner-requested quality-of-life features (see lagoon_qol.c for what each one
 * does and how it was proven). The ROM hook is wired through the desktop host's
 * game descriptor and runs on the loaded image before the machine exists, which
 * is the only point where a data patch is visible to both the interpreter and
 * ahead-of-time code. */

#include <stddef.h>
#include <stdint.h>

void LagoonQol_OnRomLoaded(const uint8_t *rom, size_t size);

#endif /* LAGOON_QOL_H */
