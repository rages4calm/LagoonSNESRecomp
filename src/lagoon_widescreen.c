/* Presentation-only expansion. The $01:8311 map resolver is reproduced as
 * read-only host lookups, never by moving the guest camera or replaying DMA.
 * No ROM data, graphics, or generated guest code belongs in this source. */
#include "lagoon_widescreen.h"
#include "common_rtl.h"
#include "config.h"
#include "snes/ppu.h"
#include "snes/ws_shadow.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double s_aspect = 16.0 / 9.0; /* 0 means drawable aspect */
static int s_was_active;
static unsigned s_room_key;
static uint64_t s_state_generation;
static const char *s_config_path = "config.ini";

void LagoonWide_SetConfigPath(const char *path) { if (path) s_config_path = path; }

static unsigned word(unsigned a) { return g_ram[a] | (g_ram[a + 1] << 8); }
static unsigned pointer(unsigned a) { return word(a) | (g_ram[a + 2] << 16); }

/* This is deliberately not snes_read: presentation must not tick the guest
 * bus or acknowledge registers. The map resolver only permits WRAM/LoROM. */
static unsigned byte_at(unsigned a) {
  unsigned bank = (a >> 16) & 255, off = a & 65535;
  if (bank == 0x7e || bank == 0x7f)
    return g_ram[((bank - 0x7e) << 16) | off];
  if ((bank & 0x7f) < 0x20 && off >= 0x8000)
    return g_rom[((bank & 0x1f) << 15) | (off & 0x7fff)];
  return 0;
}

static int valid_pointer(unsigned a) {
  unsigned bank = (a >> 16) & 255;
  return bank == 0x7e || bank == 0x7f ||
      ((bank & 0x7f) < 0x20 && (a & 65535) >= 0x8000);
}

static unsigned map_tile(int layer, int x, int y) {
  unsigned meta;
  if (x < 0 || y < 0 || x >= 2048 || y >= 2048)
    meta = g_ram[0x52]; /* same out-of-map fill as $01:833E */
  else
    meta = byte_at(pointer(layer ? 0x8b : 0x88) + (y >> 4) * 128 + (x >> 4));
  unsigned a = pointer(layer ? 0x75 : 0x69) + meta * 8 +
      ((x & 8) >> 2) + ((y & 8) >> 1);
  unsigned tile = byte_at(a) | (byte_at(a + 1) << 8);
  return layer ? tile & 0xdfff : tile;
}

static void read_aspect(const char *value) {
  if (!strcmp(value, "window")) s_aspect = 0;
  else if (!strcmp(value, "16:9")) s_aspect = 16.0 / 9;
  else if (!strcmp(value, "16:10")) s_aspect = 1.6;
  else if (!strcmp(value, "21:9")) s_aspect = 21.0 / 9;
  else if (!strcmp(value, "32:9")) s_aspect = 32.0 / 9;
}

static void read_ini(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) return;
  char line[512], section[64] = "";
  while (fgets(line, sizeof(line), f)) {
    char name[64], value[64];
    if (sscanf(line, " [%63[^]]]", name) == 1) {
      snprintf(section, sizeof(section), "%s", name);
    } else if (!strcmp(section, "Lagoon") &&
               sscanf(line, " %63[^= ] = %63s", name, value) == 2 &&
               !strcmp(name, "WideAspect")) read_aspect(value);
  }
  fclose(f);
}

void LagoonWide_AfterConfig(void) {
  s_aspect = 16.0 / 9;
  read_ini(s_config_path);
  read_ini("config.local.ini");
  const char *aspect = getenv("LAGOON_WIDE_ASPECT");
  if (aspect) read_aspect(aspect);
}

void LagoonWide_PrepareFrame(int drawable_w, int drawable_h, int *width, int *height) {
  *height = 224;
  *width = 256;
  if (g_config.widescreen) {
    double aspect = s_aspect;
    if (!aspect) aspect = drawable_w > 0 && drawable_h > 0 ?
        (double)drawable_w / drawable_h : 16.0 / 9;
    SnesDisplayFrame frame = SnesDisplayAspect_ComputeAdaptiveFrame(
        256, 224, kPpuBufWidth, aspect,
        SnesDisplayAspect_Clamp(g_config.display_aspect));
    *width = frame.width;
  }
}

void LagoonWide_Reset(void) {
  WsShadowReset();
  s_was_active = 0;
  s_room_key = 0;
  s_state_generation = RtlStateGeneration();
}

void LagoonWide_BeginFrame(unsigned number) {
  (void)number;
  /* External/debugger loads can advance the timeline without the desktop
   * menu's on_reset hook. Never let pre-load shadow cells cross that jump,
   * including while this scene has no supported widescreen layout. */
  if (s_state_generation != RtlStateGeneration()) LagoonWide_Reset();
  if (!g_ppu) return;
  /* Lagoon parks unused OAM at x=320, leaving old tile/y bytes intact. The
   * generic widened decoder would resurrect them as ghost menu/HUD sprites.
   * Until a title-specific off-screen object producer exists, only native
   * OAM and sprites straddling the native edge are authorized. */
  static const uint8_t no_margin_objects[16] = {0};
  PpuWsSetOamLeftHints(g_ppu, no_margin_objects);
  PpuWsSetOamRightHints(g_ppu, no_margin_objects);
  /* Clamp title/dialogue/menu layers; allow ordinary retained-map rooms only.
   * Parallax rooms ($50 != 0) need a different BG2 resolver. */
  PpuSetWidescreenLayerMask(g_ppu, 0);
  PpuSetWidescreenLayerClamp(g_ppu, 15);
  int active = g_config.widescreen && g_ppu->extraLeftRight &&
      (g_ram[0x500] & 0xc0) == 0xc0 && g_ram[0x50] == 0 &&
      !(g_ppu->mosaic & 3) &&
      (g_ppu->bgmode & 7) == 1 && !PPU_bigTiles(g_ppu, 0) &&
      !PPU_bigTiles(g_ppu, 1) && PPU_bgTilemapAdr(g_ppu, 0) == 0x7000 &&
      PPU_bgTilemapAdr(g_ppu, 1) == 0x7800 &&
      valid_pointer(pointer(0x88)) && valid_pointer(pointer(0x8b)) &&
      valid_pointer(pointer(0x69)) && valid_pointer(pointer(0x75));
  unsigned key = pointer(0x69) ^ pointer(0x75) ^ (g_ram[0x48e] << 24);
  if (!active || !s_was_active || key != s_room_key) WsShadowReset();
  s_was_active = active;
  s_room_key = key;
  if (!active) return;
  PpuSetWidescreenLayerMask(g_ppu, 3);
  PpuSetWidescreenShadowNativeExactMask(g_ppu, 3);
  PpuSetWidescreenLayerClamp(g_ppu, 12); /* BG3 HUD remains native centered */
  for (int layer = 0; layer < 2; ++layer) {
    /* Preserve the actual PPU's phase, borrowing only lost high bits from
     * WRAM. The wrapped difference handles a one-frame boundary disagreement. */
    int camera_x = word(0x480 + layer * 4), camera_y = word(0x482 + layer * 4);
    int sx = g_ppu->hScroll[layer] & 0x3ff, sy = g_ppu->vScroll[layer] & 0x3ff;
    int wx = camera_x + ((sx - camera_x + 512) & 1023) - 512;
    int wy = camera_y + ((sy - camera_y + 512) & 1023) - 512;
    if (wx < 0) wx = sx;
    if (wy < 0) wy = sy;
    WsShadowSetWorld(layer, wx, wy);
    WsShadowSetScroll(layer, sx, sy);
    WsShadowSetBlankTile(layer, map_tile(layer, -16, -16));
  }
  WsShadowFrame(g_ppu);
  for (int layer = 0; layer < 2; ++layer) {
    int wx = WsShadowWorldX(layer), wy = WsShadowWorldY(layer);
    int x0 = (wx - g_ppu->extraLeftRight - 8) / 8;
    int x1 = (wx + 256 + g_ppu->extraLeftRight + 8) / 8;
    for (int y = wy / 8; y <= (wy + 232) / 8 && y < 512; ++y)
      for (int x = x0 < 0 ? 0 : x0; x <= x1 && x < 4096; ++x)
        WsShadowPrefillTile(layer, x, y, map_tile(layer, x * 8, y * 8));
  }
}
