#include <stdint.h>
#include <stddef.h>
#pragma once
void LagoonWide_SetConfigPath(const char *path);
void LagoonWide_AfterConfig(void);
void LagoonWide_PrepareFrame(int drawable_w, int drawable_h, int *width, int *height);
void LagoonWide_BeginFrame(unsigned number);
void LagoonWide_Reset(void);

void LagoonWide_OnVramWrite(unsigned byte_address);

void LagoonWide_EndFrame(const uint8_t *field, unsigned number);

int LagoonWide_DrawFrame(uint8_t *dst, size_t pitch, const uint8_t *field,
    int width, int height, double alpha);
