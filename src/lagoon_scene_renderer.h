#ifndef LAGOON_SCENE_RENDERER_H
#define LAGOON_SCENE_RENDERER_H
#include <stddef.h>
#include <stdint.h>
struct Ppu;
typedef struct LagoonSceneRendererStats {
  uint64_t resets, packets, refused_packets, renders, independent_pixels;
  uint64_t verified_presents, replaced_presents, mismatches, fallbacks;
} LagoonSceneRendererStats;
/* Stable Atland and exact Gold Cave03/04/05/06. Observe lines1..224, then EndFrame.
 * All presentation data is copied; TryPresent never reads the mutable guest. */
void LagoonSceneRenderer_Reset(void);
void LagoonSceneRenderer_ObserveRaster(const struct Ppu *ppu,unsigned line,
    const uint8_t *ram,const uint8_t *rom,size_t rom_size,
    uint64_t generation,int authority);
void LagoonSceneRenderer_EndFrame(unsigned frame,uint64_t generation,int authority);
/* Full-word, whole-field reference equality is required before output changes.
 * Default auto replaces exact candidates. Explicit verify never replaces. */
int LagoonSceneRenderer_TryPresent(uint8_t *dst,size_t pitch,
    const uint8_t *reference,size_t reference_pitch,unsigned width,unsigned height,
    uint64_t generation);
void LagoonSceneRenderer_GetStats(LagoonSceneRendererStats *out);
#endif
