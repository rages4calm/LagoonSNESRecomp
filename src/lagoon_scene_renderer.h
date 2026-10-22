#ifndef LAGOON_SCENE_RENDERER_H
#define LAGOON_SCENE_RENDERER_H
#include <stddef.h>
#include <stdint.h>
struct Ppu;
enum { LAGOON_SCENE_WORLD = 1, LAGOON_SCENE_INCOMING_MOSAIC = 2 };
typedef struct LagoonSceneRendererStats {
  uint64_t resets, packets, refused_packets, renders, independent_pixels;
  uint64_t verified_presents, replaced_presents, mismatches, fallbacks;
} LagoonSceneRendererStats;
/* Exact authored scene resources00..4A. Observe lines1..224, then EndFrame.
 * All presentation data is copied; TryPresent never reads the mutable guest. */
/* Pure exact descriptor matcher for the additional rooms09..4A. This is
 * background resource scope, not actor, fade, map-upload or raster authority. */
int LagoonSceneRenderer_AdditionalRoom(const uint8_t *ram);
/* Exact source-owned layered scenes, including the original room07 contract. */
int LagoonSceneRenderer_ParallaxRoom(const uint8_t *ram);
int LagoonSceneRenderer_ExactRoom(const uint8_t *ram);
void LagoonSceneRenderer_Reset(void);
void LagoonSceneRenderer_ObserveRaster(const struct Ppu *ppu,unsigned line,
    const uint8_t *ram,const uint8_t *rom,size_t rom_size,
    uint64_t generation,int authority);
void LagoonSceneRenderer_EndFrame(unsigned frame,uint64_t generation,int authority);
/* Ordinary world: full-word, whole-field equality before output changes.
 * Proven incoming mosaic: full native256x224 word equality, then margins only;
 * the original native center is never replaced by this presentation class.
 * Default auto replaces exact candidates. Explicit verify never replaces. */
int LagoonSceneRenderer_TryPresent(uint8_t *dst,size_t pitch,
    const uint8_t *reference,size_t reference_pitch,unsigned width,unsigned height,
    uint64_t generation);
void LagoonSceneRenderer_GetStats(LagoonSceneRendererStats *out);
#endif
