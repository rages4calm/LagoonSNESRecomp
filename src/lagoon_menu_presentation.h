#ifndef LAGOON_MENU_PRESENTATION_H
#define LAGOON_MENU_PRESENTATION_H
#include <stddef.h>
#include <stdint.h>
typedef struct Ppu Ppu;
typedef enum LagoonMenuPanel {
  LAGOON_MENU_NONE = 0, LAGOON_MENU_ITEMS = 1, LAGOON_MENU_MAGIC = 2,
  LAGOON_MENU_EQUIPMENT = 3
} LagoonMenuPanel;
/* Reads live guest resources without bus accesses, latches or guest writes. */
LagoonMenuPanel LagoonMenuPresentation_Identify(const Ppu *ppu, const uint8_t *ram);
/* Source field has actual width, native256px center and explicit byte pitch.
 * Returns1 only after handling a verified Items/Magic/Equipment panel in optional wide
 * mode. Unknown scenes return0 without touching destination. Padded rows and
 * source/destination aliasing are supported. No fonts or artwork are stretched. */
int LagoonMenuPresentation_Draw(uint8_t *dst, size_t pitch, const uint8_t *field,
    size_t source_pitch, unsigned width, unsigned height, const Ppu *ppu,
    const uint8_t *ram, int widescreen);
#endif
