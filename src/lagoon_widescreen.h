#include <stdint.h>
#include <stddef.h>
#pragma once
void LagoonWide_SetConfigPath(const char *path);
void LagoonWide_AfterConfig(void);
/* Main-thread host preference only; no guest state/timing is changed.
 * Valid extents are100,125,150 percent. Invalid values leave it unchanged. */
unsigned LagoonWide_GetZoom(void);
int LagoonWide_SetZoom(unsigned percent);

void LagoonWide_PrepareFrame(int drawable_w, int drawable_h, int *width, int *height);
void LagoonWide_PresentationGeometry(int sw,int sh,int *width,int *height);
void LagoonWide_PresentationCompose(uint8_t *dst,size_t pitch,int width,int height,
    const uint8_t *source,size_t source_pitch,int sw,int sh);
int LagoonWide_ViewExtraX(int original_extra);
int LagoonWide_ViewExtraY(void);
void LagoonWide_ViewBounds(const uint8_t *ram,int original_extra,int *left,int *top,int *right,int *bottom);
void LagoonWide_BeginFrame(unsigned number);
void LagoonWide_Reset(void);

void LagoonWide_OnVramWrite(unsigned byte_address);
void LagoonWide_OnBackgroundDma(unsigned bank,unsigned address,unsigned destination,unsigned length);
void LagoonWide_OnParallaxQueueDma(unsigned length);

void LagoonWide_EndFrame(const uint8_t *field, unsigned number);

int LagoonWide_DrawFrame(uint8_t *dst, size_t pitch, const uint8_t *field,
    int width, int height, double alpha);

struct Ppu;
void LagoonWide_ObserveRaster(const struct Ppu *ppu, unsigned line, void *context);
