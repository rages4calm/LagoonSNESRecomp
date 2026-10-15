#pragma once
void LagoonWide_SetConfigPath(const char *path);
void LagoonWide_AfterConfig(void);
void LagoonWide_PrepareFrame(int drawable_w, int drawable_h, int *width, int *height);
void LagoonWide_BeginFrame(unsigned number);
void LagoonWide_Reset(void);
