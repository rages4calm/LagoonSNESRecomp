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
 *   expresses as per-frame state (the camera). The frame tick fires at the frame
 *   boundary and skips speculative frames, so a write lands between frames and is
 *   what the next frame renders from.
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
#include "common_rtl.h" /* g_ram: WRAM, $7E:xxxx -> g_ram[addr] */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> /* getenv: the development hook reads its target from here */
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

/* Both feature flags live here because LagoonQol_OnRomLoaded, which runs first,
 * reads them for every feature; the camera one is set there from the runtime
 * query rather than from its activation callback. */
static int g_camera_center;

/* Declared by the package manifest, so the runtime requires it to exist. The
 * ROM patch above is the one that changes the game; this records that the
 * feature resolved, and is the place a RAM-writing feature would set its flag. */
static void qol_walk_speed_activated(void) { g_walk_speed_applied = 1; }

void LagoonQol_OnRomLoaded(const uint8_t *rom, size_t size) {
  g_walk_speed_applied = 0;
  g_camera_center = 0;
  /* Nothing camera-related is decided here. Its activation callback sets the flag
   * and registers the frame callback, and that runs after this hook (ROM is read
   * at host_main.c:3496, plugins are activated at 3522), so clearing here cannot
   * undo it. Reading the feature state at ROM load was a workaround for a callback
   * that never ran; the reason it never ran was the wiped callback list, not this
   * hook. */
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

/* Immediate camera following.
 *
 * The engine keeps the view in two 10-bit values: $0200 is BG1's horizontal
 * scroll and $0202 the vertical, each a low byte plus two high bits in the next
 * byte, and $00:871C pushes them to $210D/$210E every frame. $0204/$0206 feed
 * BG2 through $210F/$2110 and, in the town at least, hold the same numbers, so
 * the layer copies the view rather than parallaxing it.
 *
 * The view does not track Nasir. Each frame the engine derives his screen
 * position at $050C/$050E as `world - scroll`, tests it against a soft window
 * ($01:A5C2 onward compares against 288, 176, -32 and 80) and only moves the
 * scroll once he has reached the edge of that window -- measured rather than
 * assumed: in the town the scroll stayed at 528 while he walked from screen
 * y=112 down to y=157, so he drifts toward the bottom of the screen instead of
 * the view following him.
 *
 * Centring is therefore a per-frame write of `position - half field`, 128 and
 * 112 of a 256x224 field, clamped to the room. The room bounds are the ones the
 * engine itself clamps entities to at $01:A56A: at least 16, and no further than
 * the far bound ($0492/$0494) minus one screen. Writing the view rather than the
 * border constants is deliberate: the constants are instruction immediates, and
 * an instruction patch would be silently undone by a later ahead-of-time
 * promotion of the routine that holds them. BG3 is left alone because it holds
 * zero here and is evidently not part of the scrolled field.
 */
#define QOL_WRAM_ENTITY_X 0x0502u /* entity 0 is Nasir: proven from movement dumps */
#define QOL_WRAM_ENTITY_Y 0x0504u
#define QOL_WRAM_ROOM_RIGHT 0x0492u /* the bound $01:A574 clamps entity X to */
#define QOL_WRAM_ROOM_BOTTOM 0x0494u
#define QOL_WRAM_VIEW_X 0x0480u /* the camera the engine keeps, not the derived copy */
#define QOL_WRAM_VIEW_Y 0x0482u
#define QOL_WRAM_VIEW_X_FRAC 0x0094u /* $01:815A: LDA $0480; ADC $94; STA $0200 */
#define QOL_WRAM_VIEW_Y_FRAC 0x0096u /* $01:8169 does the same for $0482 and $96 */
#define QOL_FIELD_HALF_W 128u
#define QOL_FIELD_HALF_H 112u
#define QOL_FIELD_W 256u
#define QOL_FIELD_H 224u
#define QOL_ROOM_EDGE_MARGIN 16u /* the low bound the engine clamps entities to */
#define QOL_SCROLL_MASK 0x03FFu  /* the PPU takes 10 bits; $0201/$0203 are masked */

static uint16_t qol_wram_read16(uint32_t addr) {
  return (uint16_t)(g_ram[addr] | ((uint16_t)g_ram[addr + 1] << 8));
}

static void qol_wram_write16(uint32_t addr, uint16_t value) {
  g_ram[addr] = (uint8_t)(value & 0xFF);
  g_ram[addr + 1] = (uint8_t)(value >> 8);
}

/* Clamp to the room, and hold the near edge if the room is smaller than a
 * screen: pinning the view beats scrolling into unrawn map. */
static uint16_t qol_clamp_view(long wanted, long low, long high) {
  if (high < low) {
    return (uint16_t)low;
  }
  if (wanted < low) {
    wanted = low;
  } else if (wanted > high) {
    wanted = high;
  }
  return (uint16_t)(wanted & (long)QOL_SCROLL_MASK);
}

static void qol_frame(void) {
  if (!g_camera_center) {
    return; /* the faithful game, untouched */
  }
  uint16_t x = qol_wram_read16(QOL_WRAM_ENTITY_X);
  uint16_t y = qol_wram_read16(QOL_WRAM_ENTITY_Y);
  uint16_t right = qol_wram_read16(QOL_WRAM_ROOM_RIGHT);
  uint16_t bottom = qol_wram_read16(QOL_WRAM_ROOM_BOTTOM);
  uint16_t view_x = qol_clamp_view((long)x - QOL_FIELD_HALF_W, QOL_ROOM_EDGE_MARGIN,
                                   (long)right - QOL_FIELD_W);
  uint16_t view_y = qol_clamp_view((long)y - QOL_FIELD_HALF_H, QOL_ROOM_EDGE_MARGIN,
                                   (long)bottom - QOL_FIELD_H);
  /* Write the camera, not the value the PPU is fed from. $01:8158 recomputes
   * $0200 = $0480 + $94 every frame, so a write to $0200 at the frame boundary is
   * overwritten before the next frame renders it -- tried and measured, the screen
   * position stayed exactly where the stock game put it. The engine only rewrites
   * $0480/$0482 when a border scroll actually fires, which is why writing here
   * holds. The fractions are cleared so the view is centred rather than centred
   * plus a sub-pixel remainder the smooth scroller left behind. */
  qol_wram_write16(QOL_WRAM_VIEW_X, view_x);
  qol_wram_write16(QOL_WRAM_VIEW_Y, view_y);
  qol_wram_write16(QOL_WRAM_VIEW_X_FRAC, 0);
  qol_wram_write16(QOL_WRAM_VIEW_Y_FRAC, 0);
  qol_wram_write16(QOL_WRAM_VIEW_X + 4, view_x); /* BG2 mirrors the view */
  qol_wram_write16(QOL_WRAM_VIEW_Y + 4, view_y);
}

/* The frame callback is registered here, not from the constructor, and that is
 * the whole reason this feature took three attempts: mod_runtime_activate_plugins
 * (mod_runtime.cpp:2959) clears the frame and APU callback lists and then invokes
 * reset and activation callbacks, at every session start. A static constructor
 * runs before main, so a callback registered there is wiped before the first
 * frame -- registration returns success, nothing ever calls it, and no diagnostic
 * complains. This function is also the only place the feature's state is known
 * for certain: the runtime calls it exactly when the player has enabled it. */
static void qol_camera_activated(void) {
  g_camera_center = 1;
  snes_mod_register_frame_callback(qol_frame);
  /* One line, once per session: a plugin that changes the game should say so
   * where a player or a receipt will see it. */
  fprintf(stderr, "lagoon.qol: camera_center active, view centred on Nasir\n");
}

/* Development hook: put Nasir on a chosen tile.
 *
 * This exists because route work is a positioning problem. Standing one tile off
 * from a door or an NPC changes the outcome completely -- a warp fires or it does
 * not, a dialogue opens or the button does nothing -- and walking him there with
 * scripted presses costs about 45 s a try and lands on whatever the collision
 * grid allows, not on the square being asked about. One write of $0502/$0504 at a
 * chosen frame makes the question "what happens on tile (352,60)?" a single run.
 *
 * It is a harness hook, not a feature: off by default, and the position comes
 * from the environment (LAGOON_DEBUG_AT="x,y[,frame]") rather than a player-facing
 * setting, so nothing about the shipped game depends on it. A player who has
 * never set that variable gets the faithful game even with the feature switched
 * on, which is the safe direction for a mistake.
 *
 * Velocity at $0506/$0508 is zeroed with the position, otherwise the momentum
 * from the walk that got him there carries on applying and the measured position
 * is a frame or two away from the one asked for.
 */
#define QOL_WRAM_ENTITY_VX 0x0506u /* the velocity $02:80EA fills from the tier table */
#define QOL_WRAM_ENTITY_VY 0x0508u
#define QOL_DEBUG_ENV "LAGOON_DEBUG_AT"
#define QOL_DEBUG_MAX 8 /* a route is a handful of moves, not a stream */

typedef struct {
  uint16_t x, y;
  long frame; /* run frame to fire on, counted from plugin activation */
  int fired;
} qol_debug_move_t;

static qol_debug_move_t g_debug_moves[QOL_DEBUG_MAX];
static int g_debug_move_count;
static long g_debug_tick; /* frames since activation, so the hook is self-timed */

static void qol_debug_frame(void) {
  g_debug_tick++;
  for (int i = 0; i < g_debug_move_count; i++) {
    qol_debug_move_t *m = &g_debug_moves[i];
    if (m->fired || g_debug_tick < m->frame) {
      continue;
    }
    m->fired = 1;
    qol_wram_write16(QOL_WRAM_ENTITY_X, m->x);
    qol_wram_write16(QOL_WRAM_ENTITY_Y, m->y);
    qol_wram_write16(QOL_WRAM_ENTITY_VX, 0);
    qol_wram_write16(QOL_WRAM_ENTITY_VY, 0);
    fprintf(stderr, "lagoon.qol: debug_position frame %ld moved Nasir to (%u,%u)\n",
            g_debug_tick, (unsigned)m->x, (unsigned)m->y);
  }
}

/* Parse "x,y[,frame]" entries separated by ';'. More than one is needed because
 * a route is not one jump: getting to the fight means landing in the town band
 * first, crossing the gate, and only then standing next to the enemy. The frame
 * a move fires on defaults to 1 so a single-position spec still behaves. */
static void qol_debug_position_activated(void) {
  const char *spec = getenv(QOL_DEBUG_ENV);
  if (!spec || !*spec) {
    fprintf(stderr, "lagoon.qol: debug_position enabled but %s is unset, nothing moved\n",
            QOL_DEBUG_ENV);
    return;
  }
  g_debug_move_count = 0;
  g_debug_tick = 0;
  const char *p = spec;
  while (p && *p && g_debug_move_count < QOL_DEBUG_MAX) {
    long x = -1, y = -1, frame = 1;
    int used = 0;
    while (*p == ';') {
      p++; /* the separator is not part of the next entry, and %ld will not skip it */
    }
    if (sscanf(p, " %ld,%ld,%ld%n", &x, &y, &frame, &used) < 2) {
      used = 0;
      x = y = -1;
      frame = 1;
      if (sscanf(p, " %ld,%ld%n", &x, &y, &used) < 2) {
        fprintf(stderr,
                "lagoon.qol: debug_position refused, %s=\"%s\" is not "
                "\"x,y[,frame][;...]\"\n",
                QOL_DEBUG_ENV, spec);
        return;
      }
    }
    if (x < 0 || x > 65535 || y < 0 || y > 65535 || frame < 1) {
      fprintf(stderr, "lagoon.qol: debug_position refused, bad entry in \"%s\"\n", spec);
      return;
    }
    g_debug_moves[g_debug_move_count].x = (uint16_t)x;
    g_debug_moves[g_debug_move_count].y = (uint16_t)y;
    g_debug_moves[g_debug_move_count].frame = frame;
    g_debug_moves[g_debug_move_count].fired = 0;
    g_debug_move_count++;
    p = strchr(p, ';');
  }
  snes_mod_register_frame_callback(qol_debug_frame);
  fprintf(stderr, "lagoon.qol: debug_position armed with %d move(s) from %s\n",
          g_debug_move_count, QOL_DEBUG_ENV);
}

SNES_MOD_CONSTRUCTOR(lagoon_qol_register) {
  /* Only the plugin identities are declared at load time, since the activation
   * plugin table is not cleared. Anything per-frame belongs in the activation
   * callback above. */
  snes_mod_register_activation_plugin("lagoon.qol.walk_speed",
                                      qol_walk_speed_activated);
  snes_mod_register_activation_plugin("lagoon.qol.camera_center",
                                      qol_camera_activated);
  snes_mod_register_activation_plugin("lagoon.qol.debug_position",
                                      qol_debug_position_activated);
}
