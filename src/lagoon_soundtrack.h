#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
struct SaveLoadInfo;

/* Constructor registers passive diagnostics and opt-in partial replacement.
 * Host reset stops stale PCM. MSU entry points own their recursive APU lock. */
void LagoonSoundtrack_Reset(void);
bool LagoonSoundtrack_Preflight(const void *data, size_t size);
void LagoonSoundtrack_SaveExtra(struct SaveLoadInfo *sli);
void LagoonSoundtrack_LoadExtra(struct SaveLoadInfo *sli, uint32_t version);
void LagoonSoundtrack_OnStateLoaded(uint32_t version);

/* Live replacement-music trim, separate from the host master volume/SFX.
 * Percent0..100 multiplies the album-balanced gain; defaults100. This host
 * preference is not saved in guest states, and remains through state loads.
 * Call at the title/CPU frame boundary, like the other soundtrack hooks. */
void LagoonSoundtrack_SetVolume(unsigned percent);
unsigned LagoonSoundtrack_GetVolume(void);
