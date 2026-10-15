#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
struct SaveLoadInfo;
bool LagoonState_Preflight(const void *data, size_t size);
const char *LagoonState_Identity(void);
void LagoonState_SaveExtra(struct SaveLoadInfo *sli);
void LagoonState_LoadExtra(struct SaveLoadInfo *sli, uint32_t version);
void LagoonState_OnLoaded(uint32_t version);
