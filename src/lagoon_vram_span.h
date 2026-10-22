#pragma once

/* Byte-addressed circular VRAM transfer against one nonempty half-open range.
 * A zero DMA length has already been expanded to 65536 by the caller. */
static inline int LagoonVramSpan_Overlaps(unsigned first, unsigned bytes,
                                        unsigned low, unsigned high) {
  if (!bytes) return 0;
  if (bytes >= 65536) return 1;
  unsigned end = first + bytes;
  return (first < high && end > low) ||
         (end > 65536 && end - 65536 > low);
}

static inline unsigned LagoonVramSpan_Spell3Mask(unsigned first, unsigned bytes) {
  int graphics = LagoonVramSpan_Overlaps(first, bytes, 0x6000, 0x7260);
  int maps = LagoonVramSpan_Overlaps(first, bytes, 0xe000, 0xe800) ||
             LagoonVramSpan_Overlaps(first, bytes, 0xf000, 0x10000);
  return (graphics || maps) | (graphics << 1);
}
