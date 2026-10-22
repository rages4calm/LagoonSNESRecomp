#pragma once
#include <stdbool.h>
/* Independent host audio. Never writes the SPC/DSP or guest sample ring. */
void LagoonAchievementSound_Configure(bool enabled);
/* Host volume changes also affect a chime that is already playing. */
void LagoonAchievementSound_SetVolume(unsigned master_volume);
void LagoonAchievementSound_Play(unsigned master_volume);
void LagoonAchievementSound_Shutdown(void);
