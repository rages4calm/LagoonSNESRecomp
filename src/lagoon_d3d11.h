#pragma once
struct RendererFuncs;
#ifdef __cplusplus
extern "C" {
#endif
int LagoonD3D11_Create(const char *id, struct RendererFuncs *funcs);
void LagoonD3D11_TitleUi(int active);
void LagoonD3D11_Capture(const char *path);
#ifdef __cplusplus
}
#endif
