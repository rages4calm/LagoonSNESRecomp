/* lagoon_qol.c — owner-requested quality-of-life features, opt-in.
 *
 * Every feature here is OFF by default, and the faithful game is what a player
 * gets if they change nothing. These are trusted static plugins: the mod runtime
 * resolves the enabled set before frame 1 and calls what this file registered
 * under the matching plugin id (docs/MOD_PACKAGES.md), and packages themselves
 * carry data only, so the behaviour has to live in the executable.
 *
 * Two mechanisms, chosen deliberately:
 *
 *   ROM data, patched once in LagoonQol_OnRomLoaded. Data reads go through the
 *   memory system at run time, so a patched table is honoured by the interpreter
 *   and by ahead-of-time code alike. An *instruction* patch would not be: a
 *   promoted routine carries the bytes the generator compiled, so a later AOT
 *   promotion would silently undo it. That is why nothing here writes instructions.
 *
 *   Game RAM, written from a frame callback, for features the engine only
 *   expresses as state (camera position, invulnerability timer). Not used yet.
 *
 * The const on the ROM pointer is API politeness rather than a promise: the host
 * reads the file into a heap buffer and hands that same buffer to SnesInit after
 * this hook returns, so a patch here is what the guest will read.
 *
 * A patch that cannot find the exact bytes it expects is refused and reported,
 * never applied on top of unknown data: a mod that silently rewrites something
 * else is worse than a mod that does nothing, because it looks like it worked.
 */

#include "lagoon_qol.h"

#include "mod_runtime.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define QOL_PACKAGE_ID "lagoon.quality"

/* Walking speed.
 *
 * The engine does not carry a walk speed. $02:80EA computes an index from a
 * per-entity direction byte ($050B,x) and a speed-tier selector ($051A,x),
 * `direction * 4 + tier * 16`, and copies a four-byte (vx_lo, vx_hi, vy_lo,
 * vy_hi) record out of the table at $02:8129 into the entity's velocity at
 * $0506/$0508; $02:811C zeroes it. $01:A4EB then adds velocity to the position
 * at $0502/$0504, which is why a position advances only on frames the game
 * chooses to apply it.
 *
 * The table holds six tiers of +-1, 2, 3, 4, 5 and 6 pixels for right, down,
 * left, up. Nasir walks on tier 2, which is +-3; the owner's measurement of
 * "3 px/frame" is that value, and the dumps agree with it: 9 pixels per 7-frame
 * interval in which the direction was held for 6 frames. Tier 4 already holds
 * +-5, so "walking speed 5 px/frame" is this row, not a new number: the four
 * signed magnitudes 3, 3, -3, -3 become 5, 5, -5, -5.
 *
 * Address in the file: bank $02 at LoROM offset 2 << 15, so bus $02:8149 is
 * file offset 0x10149.
 */
#define QOL_WALK_TIER2_OFFSET 0x10149u

static const uint8_t kWalkTier2Stock[16] = {
    0x03, 0x00, 0x00, 0x00, /* right: vx +3, vy 0  */
    0x00, 0x00, 0x03, 0x00, /* down:  vx 0, vy +3  */
    0xFD, 0xFF, 0x00, 0x00, /* left:  vx -3, vy 0  */
    0x00, 0x00, 0xFD, 0xFF, /* up:    vx 0, vy -3  */
};

static const uint8_t kWalkTier2Faster[16] = {
    0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00,
    0xFB, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFB, 0xFF,
};

static int g_walk_speed_applied;

/* Declared by the package manifest, so the runtime requires it to exist. The
 * ROM patch above is the one that changes the game; this records that the
 * feature resolved, and is the place a RAM-writing feature would set its flag. */
static void qol_walk_speed_activated(void) { g_walk_speed_applied = 1; }

void LagoonQol_OnRomLoaded(const uint8_t *rom, size_t size) {
  g_walk_speed_applied = 0;
  if (!rom) {
    return;
  }
  if (size <= QOL_WALK_TIER2_OFFSET + sizeof(kWalkTier2Stock)) {
    fprintf(stderr, "lagoon.qol: walk_speed refused, rom is %u bytes\n",
            (unsigned)size);
    return;
  }
  if (!snes_mod_runtime_feature_enabled_c(QOL_PACKAGE_ID, "walk_speed")) {
    return; /* the faithful default: nothing is touched */
  }
  uint8_t *row = (uint8_t *)(uintptr_t)(rom + QOL_WALK_TIER2_OFFSET);
  if (memcmp(row, kWalkTier2Stock, sizeof(kWalkTier2Stock)) != 0) {
    fprintf(stderr,
            "lagoon.qol: walk_speed refused, $02:8149 does not hold the proven "
            "stock row\n");
    return;
  }
  memcpy(row, kWalkTier2Faster, sizeof(kWalkTier2Faster));
  g_walk_speed_applied = 1;
  fprintf(stderr, "lagoon.qol: walk_speed applied, tier 2 is now +-5 px (was +-3)\n");
}

SNES_MOD_CONSTRUCTOR(lagoon_qol_register) {
  snes_mod_register_activation_plugin("lagoon.qol.walk_speed",
                                      qol_walk_speed_activated);
}
