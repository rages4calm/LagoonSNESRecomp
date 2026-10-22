#pragma once
#include <stdbool.h>
/* Independent host audio. Never writes the SPC/DSP or guest sample ring. */
void LagoonAchievementSound_Configure(bool enabled);
/* Host volume changes also affect an effect that is already playing. */
void LagoonAchievementSound_SetVolume(unsigned master_volume);
void LagoonAchievementSound_Play(unsigned master_volume);
/* Load the optional owner-supplied WAV once at startup; fail silently if absent
 * or malformed. Returns false and leaves the laugh disabled on refusal. */
bool LagoonAchievementSound_LoadLaugh(const char *absolute_path);
unsigned LagoonAchievementSound_LaughDurationMs(void);
/* Play the loaded effect for a completed secret encounter. */
void LagoonAchievementSound_PlayLaugh(unsigned master_volume);
void LagoonAchievementSound_Shutdown(void);
