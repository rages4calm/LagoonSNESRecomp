/* Read-only ordinary NPC and nondeath enemy sprite presentation. Capture at the real OAM
 * DMA boundary; never widen guest culling, run guest drawing again, or alter
 * the native viewport. Unsupported streams and ownership fail closed. */
#include "lagoon_wide_entities.h"
#ifndef LAGOON_WIDE_ENTITIES_DECODER_ONLY
#include "common_rtl.h"
#include "config.h"
#include "snes/dma.h"
#include "snes/ppu.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { kEntities = 33, kCells = 64, kObjects = 256, kRomSize = 0x100000 };
#ifndef LAGOON_WIDE_ENTITIES_DECODER_ONLY
typedef struct WideEntitySnapshot {
  uint8_t ram[0x1100], oam[544];
  uint64_t generation;
  unsigned serial;
  int valid;
} WideEntitySnapshot;
static WideEntitySnapshot s_frame;
static unsigned s_serial, s_last_serial;
static PpuExtraObject s_objects[kObjects];
static uint16_t s_pixels[kObjects][256];
static size_t s_object_count;
static uint64_t s_captures, s_validated, s_rejected, s_emitted;
static void report_status(const char *status, unsigned entity, unsigned cell) {
  const char *path = getenv("LAGOON_WIDE_ENTITY_REPORT");
  if (!path) return;
  unsigned left = 0, right = 0;
  for (size_t i = 0; i < s_object_count; ++i) {
    left += s_objects[i].x < 0;
    right += s_objects[i].x + s_objects[i].width > 256;
  }
  FILE *f = fopen(path, "w");
  if (!f) return;
  fprintf(f, "status=%s serial=%u captures=%llu validated=%llu rejected=%llu emitted=%llu live_objects=%zu left=%u right=%u entity=%u cell=%u\n",
      status, s_frame.serial, (unsigned long long)s_captures, (unsigned long long)s_validated,
      (unsigned long long)s_rejected, (unsigned long long)s_emitted, s_object_count, left, right, entity, cell);
  fclose(f);
}
#endif

static int rom_byte(const uint8_t *rom, size_t size, unsigned a, unsigned *v) {
  if (a > 0xffffff || (a & 0xffff) < 0x8000) return 0;
  size_t off = (((a >> 16) & 0x7f) << 15) | (a & 0x7fff);
  if (off >= size) return 0;
  *v = rom[off];
  return 1;
}
static int rom_word(const uint8_t *rom, size_t size, unsigned a, unsigned *v) {
  unsigned lo, hi;
  if (!rom_byte(rom, size, a, &lo) || !rom_byte(rom, size, a + 1, &hi)) return 0;
  *v = lo | (hi << 8); return 1;
}

int LagoonWideEntities_Decode(const uint8_t *rom, size_t size,
    unsigned selector, unsigned frame, int x, int y,
    LagoonWideCell *cells, size_t capacity) {
  unsigned lo, hi, bank, table, ptr, dims, flags;
  if (!rom || !cells || selector > 0x48 || frame > 255 ||
      !rom_byte(rom, size, 0x999d + selector * 3, &lo) ||
      !rom_byte(rom, size, 0x999e + selector * 3, &hi) ||
      !rom_byte(rom, size, 0x999f + selector * 3, &bank)) return -1;
  table = lo | (hi << 8) | (bank << 16);
  if (!rom_word(rom, size, table + frame * 2, &ptr)) return -1;
  ptr |= table & 0xff0000;
  if (!rom_byte(rom, size, ptr++, &dims) || !rom_byte(rom, size, ptr++, &flags)) return -1;
  int count = 0;
  for (int grid = 0; grid < ((flags & 0x80) ? 2 : 1); ++grid) {
    if (grid && !rom_byte(rom, size, ptr++, &dims)) return -1;
    unsigned step = ((flags ^ grid) & 1) ? 16 : 8;
    if (!(dims >> 4) || !(dims & 15)) return -1;
    for (unsigned row = 0; row < (dims & 15); ++row)
      for (unsigned col = 0; col < (dims >> 4); ++col) {
        unsigned tile, attr, dx = 0, dy = 0;
        if (!rom_byte(rom, size, ptr++, &tile) || !rom_byte(rom, size, ptr++, &attr)) return -1;
        if (tile == 255) {
          if (attr == 255) continue;
          tile = attr;
          if (!rom_byte(rom, size, ptr++, &attr) ||
              !rom_byte(rom, size, ptr++, &dx) || !rom_byte(rom, size, ptr++, &dy)) return -1;
        }
        if ((size_t)count >= capacity) return -1;
        cells[count++] = (LagoonWideCell){ x + (int)(col * step) + (int8_t)dx,
            y + (int)(row * step) + (int8_t)dy, (uint16_t)(tile | (attr << 8)), (uint8_t)step };
      }
  }
  return count;
}

int LagoonWideEntities_Pixels(const uint16_t *vram, const uint16_t *cgram,
    unsigned base1, unsigned base2, const LagoonWideCell *c,
    uint16_t *pixels, int clip_native) {
  if (!vram || !cgram || !c || !pixels || c->size > 16 || !c->size) return 0;
  unsigned base = c->tile & 0x100 ? base2 : base1;
  unsigned palette = 0x80 + ((c->tile >> 9) & 7) * 16;
  memset(pixels, 0, 256 * sizeof(*pixels));
  int opaque = 0;
  for (unsigned y = 0; y < c->size; ++y)
    for (unsigned x = 0; x < c->size; ++x) {
      int screen_x = c->x + (int)x;
      if (clip_native && screen_x >= 0 && screen_x < 256) continue;
      unsigned sx = c->tile & 0x4000 ? c->size - 1 - x : x;
      unsigned sy = c->tile & 0x8000 ? c->size - 1 - y : y;
      unsigned tile = ((((c->tile & 255) >> 4) + (sy >> 3)) << 4) |
          (((c->tile & 15) + (sx >> 3)) & 15);
      unsigned address = (base + tile * 16 + (sy & 7)) & 0x7fff;
      /* The player owns its live 36-tile staging pack at word4000..423F.
       * Ordinary room cells must resolve wholly outside that pack. */
      if (address >= 0x4000 && address < 0x4240) return 0;
      uint32_t planes = vram[address] | ((uint32_t)vram[(address + 8) & 0x7fff] << 16);
      unsigned bit = 7 - (sx & 7);
      unsigned color = ((planes >> bit) & 1) | ((planes >> (bit + 7)) & 2) |
          ((planes >> (bit + 14)) & 4) | ((planes >> (bit + 21)) & 8);
      if (color) { pixels[y * c->size + x] = 0x8000 | cgram[palette + color]; opaque = 1; }
    }
  return opaque;
}

#ifndef LAGOON_WIDE_ENTITIES_DECODER_ONLY
static unsigned ram_word(const uint8_t *ram, unsigned a) { return ram[a] | (ram[a + 1] << 8); }
static int signed_word(unsigned v) { return (int16_t)v; }

static int decode_entity(const uint8_t *ram, unsigned entity, LagoonWideCell *cells) {
  unsigned a = 0x500 + entity, state = ram[a + 0x1b], frame = ram[a + 0x1c], ox, oy;
  unsigned type = ram[a + 0x18];
  if ((type != 1 && type != 2) || state > 0x3f || (state < 2 && type == 1) ||
      (type == 2 && ((ram[a + 1] & 1) || state >= 0x25))) return -1;
  unsigned offsets = type == 1 ? 0x1fb7f : 0x1f7e9;
  if (!rom_word(g_rom, kRomSize, offsets + state * 4, &ox) ||
      !rom_word(g_rom, kRomSize, offsets + state * 4 + 2, &oy)) return -1;
  int x = signed_word(ram_word(ram, a + 12)) + signed_word(ox);
  int y = signed_word(ram_word(ram, a + 14)) + signed_word(oy);
  unsigned selector = ram[0x48e] + 1;
  int count = LagoonWideEntities_Decode(g_rom, kRomSize, selector, frame, x, y, cells, kCells);
  /* C402 -> D0B: flash/damage remaps palette bits to7, preserving all other
   * OBJ attributes. Native OAM matching independently verifies this choice. */
  if (type == 2 && ((ram[a + 1] & 0x80) || (ram[a + 0x1f] & 2)))
    for (int i = 0; i < count; ++i) cells[i].tile = (cells[i].tile & ~0xe00) | 0xe00;
  return count;
}


static int find_cell(const uint8_t *oam, const LagoonWideCell *cell, const uint8_t *used) {
  for (int slot = 0; slot < 128; ++slot) {
    unsigned a = slot * 4, high = (oam[512 + slot / 4] >> ((slot & 3) * 2)) & 3;
    if (!used[slot] && (oam[a] | ((high & 1) << 8)) == (unsigned)(cell->x & 511) &&
        oam[a + 1] == (cell->y & 255) && ram_word(oam, a + 2) == cell->tile &&
        ((high & 2) ? 16 : 8) == cell->size) return slot;
  }
  return -1;
}

static void observe_dma(const Dma *dma, unsigned channel, const DmaChannel *t) {
  (void)dma;
  if (!g_config.widescreen || !g_rom || !g_ppu || channel != 0 || t->fromB ||
      t->mode != 0 || t->fixed || t->decrement || t->bAdr != 4 ||
      t->aBank != 0x7e || t->aAdr != 0x2000 || t->size != 0x220) return;
  memcpy(s_frame.ram, g_ram, sizeof(s_frame.ram));
  memcpy(s_frame.oam, g_ram + 0x2000, sizeof(s_frame.oam));
  s_frame.generation = RtlStateGeneration();
  s_frame.serial = ++s_serial;
  s_frame.valid = 1;
  s_captures++;
}

void LagoonWideEntities_Init(void) { dma_set_start_observer(observe_dma); }
void LagoonWideEntities_Reset(void) {
  s_frame.valid = 0; s_last_serial = 0; s_object_count = 0;
  if (g_ppu) PpuSetExtraObjects(g_ppu, NULL, 0);
}

/* Decode the same OBJ 4bpp source used by the actual per-line renderer. */
static int draw_cell(const LagoonWideCell *c, unsigned anchor, uint32_t order) {
  if (s_object_count >= kObjects || c->size > 16 || c->y <= -(int)c->size || c->y >= 224 ||
      c->x >= 256 + g_ppu->extraRightCur || c->x + c->size <= -g_ppu->extraLeftCur ||
      (c->x >= 0 && c->x + c->size <= 256)) return 0;
  uint16_t *pixels = s_pixels[s_object_count];
  int opaque = LagoonWideEntities_Pixels(PpuRenderVram(g_ppu), g_ppu->cgram,
      PPU_objTileAdr1(g_ppu), PPU_objTileAdr2(g_ppu), c, pixels, 1);
  if (!opaque) return 0;
  s_objects[s_object_count++] = (PpuExtraObject){ c->x, c->y, c->size, c->size, c->size,
      (c->tile >> 12) & 3, !!(c->tile & 0x800), (uint8_t)anchor, order, pixels };
  return 1;
}

void LagoonWideEntities_BeginFrame(int enabled) {
  s_object_count = 0;
  PpuSetExtraObjects(g_ppu, NULL, 0);
  if (!enabled || !s_frame.valid) { report_status("inactive-or-no-capture", 0, 0); return; }
  if (s_frame.generation != RtlStateGeneration() || g_ppu->obsel != 2 || PPU_objPriority(g_ppu)) {
    report_status("unsupported-timeline-or-obj-layout", 0, 0); return;
  }
  if (memcmp(s_frame.oam, g_ppu->oam, 512) || memcmp(s_frame.oam + 512, g_ppu->highOam, 32)) {
    report_status("oam-fingerprint-mismatch", 0, 0); return;
  }
  const uint8_t *ram = s_frame.ram;
  if (ram[0x48e] != g_ram[0x48e] || ram[0x48c] > 64) return;
  uint8_t used[128] = {0}, queued[kEntities] = {0};
  int cell_slot[kEntities][kCells];
  memset(cell_slot, -1, sizeof(cell_slot));
  LagoonWideCell cells[kCells];
  /* Validate visible native NPC/enemy cells before showing any extra cells. */
  for (unsigned q = 0; q < ram[0x48c]; ++q) {
    unsigned offset = ram_word(ram, 0xec0 + q * 6), type = ram[0xec4 + q * 6];
    if (offset > 0x800 || (offset & 63)) continue;
    queued[offset / 64] = 1;
    if (type != 1 && type != 2) continue;
    int count = decode_entity(ram, offset, cells);
    if (count < 0) continue; /* unsupported death/common/player-pack form */
    for (int i = 0; i < count; ++i) {
      if (cells[i].y < -(int)cells[i].size || cells[i].y >= 240) continue;
      int slot = find_cell(s_frame.oam, &cells[i], used);
      if (slot < 0) {
        if (s_rejected < 8 && getenv("LAGOON_WIDE_ENTITY_REPORT"))
          fprintf(stderr, "wide-entity: native mismatch entity=%03X cell=%d xy=%d,%d tile=%04X size=%u\n",
              offset, i, cells[i].x, cells[i].y, cells[i].tile, cells[i].size);
        s_rejected++; report_status("native-cell-mismatch", offset, i); return;
      }
      used[slot] = 1;
      cell_slot[offset / 64][i] = slot;
    }
  }
  if (s_last_serial != s_frame.serial) { s_validated++; s_last_serial = s_frame.serial; }
  for (unsigned e = 1; e < kEntities; ++e) {
    unsigned offset = e * 64, a = 0x500 + offset;
    if ((ram[a] & 0xc0) != 0xc0 || (ram[a + 0x18] != 1 && ram[a + 0x18] != 2) || !queued[e]) continue;
    int count = decode_entity(ram, offset, cells);
    if (count < 0) continue;
    /* Reuse each independently matched original OAM slot. The host copy is
     * immediately behind that same native cell, preserving its ordering
     * against player, projectiles and ambient pieces without renumbering. */
    for (int i = 0; i < count; ++i)
      if (cell_slot[e][i] >= 0) s_emitted += draw_cell(&cells[i], cell_slot[e][i], 1);
  }
  PpuSetExtraObjects(g_ppu, s_objects, s_object_count);
  report_status("ready", 0, 0);
}

#endif /* runtime */
