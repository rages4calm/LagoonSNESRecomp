#ifndef LAGOON_EASTER_EGG_H
#define LAGOON_EASTER_EGG_H
#include <stddef.h>
#include <stdint.h>
struct Ppu;

/* Host-rendered cameo. Guest pointers stay const; collision is a scoped
 * read view consumed by the original movement code. */
void LagoonEasterEgg_Init(const uint8_t *rom,size_t size);
void LagoonEasterEgg_Observe(const uint8_t *ram,size_t size,uint64_t generation,int gameplay_ready);
void LagoonEasterEgg_BeginFrame(uint64_t generation);
int LagoonEasterEgg_CollisionBody(const uint8_t *ram,uint64_t generation,int *x,int *y);
void LagoonEasterEgg_ObserveRaster(const struct Ppu *ppu,unsigned line,
    const uint8_t *ram,uint64_t generation,int hdma_active);
void LagoonEasterEgg_EndFrame(uint64_t generation);
/* Draw only a complete matching ordinary-world raster; pitch is bytes. */
int LagoonEasterEgg_Draw(uint8_t *dst,size_t pitch,unsigned width,unsigned height,
    uint32_t ticks,uint64_t generation);

/* Call before forwarding human input to the guest. Returns1 only when this
 * fresh Y edge actually opens the nearby, facing cameo conversation, after
 * its current completed field was drawn visibly in the matching generation. */
int LagoonEasterEgg_TryInteract(uint32_t human);
int LagoonEasterEgg_IsOpen(void);
int LagoonEasterEgg_PollOpen(uint32_t human);
void LagoonEasterEgg_Tick(uint32_t human,uint32_t ticks);
void LagoonEasterEgg_Close(void);
int LagoonEasterEgg_Image(const uint32_t **pixels,int *width,int *height);
/* Exactly once after the final page is deliberately completed, never on open,
 * seeing the sprite, cancelling, changing rooms or loading a state. */
int LagoonEasterEgg_TakeDiscovery(uint64_t *generation);
#endif
