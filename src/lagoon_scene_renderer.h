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
int LagoonSceneRenderer_DrawView(uint8_t *dst,size_t pitch,unsigned width,unsigned height,
    const uint8_t *source,size_t source_pitch,unsigned sw,unsigned sh,uint64_t generation);
/* Fresh packet only: BG3 lettering and eight source-owned equipment cells;
 * never copy world pixels from the original HUD rows. */
int LagoonSceneRenderer_DrawHud(uint8_t *dst,size_t pitch,unsigned width,unsigned height,uint64_t generation);
/* Full finite terrain from the latest immutable qualified ordinary room source. Opaque
 * ARGB, no actors/HUD, at original resolution or an integer2x reduction so
 * neither side exceeds1024. The returned image is owned here, valid until the
 * next call/reset. All outputs clear on refusal; room and generation must match
 * the current completed field's verified identity. A presentation-only refusal
 * may reuse the last source only after fresh exact terrain/resource equality;
 * another room/generation, changed art, fades or loading cannot reuse it.
 * Parallax BG2 retains its actual qualified quarter-scroll anchor and32x64 page
 * layout over full BG1 terrain. No guest state is read or advanced here. */
int LagoonSceneRenderer_MapImage(unsigned room,uint64_t generation,
    const uint32_t **pixels,unsigned *width,unsigned *height,
    unsigned *world_width,unsigned *world_height);
/* UI-only visited terrain archive. Authenticate the original clean ROM before
 * title patches, once per ROM/session; ordinary renderer/state resets retain
 * this archive. It never grants current-world packet/render authority. */
int LagoonSceneRenderer_ArchiveOnRomInit(const uint8_t *rom,size_t size);
/* Call only after a committed live host frame, never speculative/runahead or
 * rollback replay. The exact latest completed packet/source/identity must match
 * this room and generation. Returns1accepted(including unchanged),0refused. */
int LagoonSceneRenderer_ArchiveCommit(unsigned room,uint64_t generation);
/* Lazy load at map opening; save only on modal close or normal quit. Load merges
 * untouched rooms without replacing newer qualified terrain from this session.
 * Returns1 loaded/saved,0 missing/unchanged,-1 refused/error. Corrupt or foreign
 * files are preserved and disable writes for this session. UTF8 path, one file. */
int LagoonSceneRenderer_ArchiveLoad(const char *path);
int LagoonSceneRenderer_ArchiveSave(const char *path);
unsigned LagoonSceneRenderer_ArchivedRoomCount(void);
int LagoonSceneRenderer_ArchivePersistenceBlocked(void);
/* Same dimensions/ARGB contract as MapImage, but explicitly last-seen terrain.
 * No generation/current-room claim. Never calls guest code or disk I/O. */
int LagoonSceneRenderer_VisitedMapImage(unsigned room,
    const uint32_t **pixels,unsigned *width,unsigned *height,
    unsigned *world_width,unsigned *world_height);
/* Read-only original Game Over backdrop identity; no execution authority. */
int LagoonSceneRenderer_GameOver(const uint8_t *ram,const uint8_t *rom,size_t size);

int LagoonSceneRenderer_ViewPlacement(unsigned width,unsigned height,uint64_t generation,int *left,int *top);
#endif
