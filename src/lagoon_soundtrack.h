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
