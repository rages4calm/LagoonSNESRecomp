#ifndef LAGOON_ITEM_GUIDE_H
#define LAGOON_ITEM_GUIDE_H
#include <stddef.h>
#include <stdint.h>
struct Ppu;
enum { LAGOON_ITEM_GUIDE_RAM_BYTES=0x4e0,
       LAGOON_ITEM_GUIDE_INVENTORY_RAM_BYTES=0x1063 };

typedef struct LagoonItemGuideEntry {
  uint8_t id;
  const char *name;
  const char *kind;
  const char *description;
  /* Original clean-USA shop prices; zero means not offered in that shop.
   * These are catalogue facts, never a live transaction/selection authority. */
  uint16_t atland_price, voloh_price;
} LagoonItemGuideEntry;

/* Call with original source before the live QoL patcher changes ROM bytes. */
void LagoonItemGuide_Init(const uint8_t *rom,size_t size);
size_t LagoonItemGuide_Count(void);
int LagoonItemGuide_GetAt(size_t index,LagoonItemGuideEntry *out);
int LagoonItemGuide_Get(unsigned id,LagoonItemGuideEntry *out);
int LagoonItemGuide_Owned(const uint8_t *ram,size_t size,unsigned id);
/* Genuine original Items panel only: exact existing PPU/frame/font authority,
 * bounded original row/column and a real owned item. Stale shop0469 is NOT used. */
int LagoonItemGuide_FromInventory(const struct Ppu *ppu,const uint8_t *ram,
                                 size_t size,LagoonItemGuideEntry *out);
#endif
