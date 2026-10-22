/* Title-owned, read-only achievement presentation. No guest or progress writes. */
#pragma once
#include <stddef.h>
#include <stdint.h>

enum {
  LAGOON_ACHIEVEMENT_UI_UP=1, LAGOON_ACHIEVEMENT_UI_DOWN,
  LAGOON_ACHIEVEMENT_UI_PREVIOUS, LAGOON_ACHIEVEMENT_UI_NEXT,
  LAGOON_ACHIEVEMENT_UI_CLOSE, LAGOON_ACHIEVEMENT_UI_FIRST,
  LAGOON_ACHIEVEMENT_UI_LAST
};
void LagoonAchievementUI_Reset(void);
void LagoonAchievementUI_Open(void);
void LagoonAchievementUI_Close(void);
int LagoonAchievementUI_IsOpen(void);
/* Prime the opener's held buttons before running Tick; the host also owns its
 * release-before-gameplay gate when this frozen panel is dismissed. */
int LagoonAchievementUI_PollOpen(uint32_t human);
void LagoonAchievementUI_Command(unsigned command);
/* Call with monotonic host milliseconds, including when the viewer is closed.
 * This clock never modifies guest time or achievement progress. */
void LagoonAchievementUI_Tick(uint32_t human,uint32_t ticks);
int LagoonAchievementUI_Image(const uint32_t **pixels,int *width,int *height);
/* Root consumes the committed core notice queue and starts one toast at a
 * time. Sound is a separate host concern; this module never consumes notices. */
void LagoonAchievementUI_StartToast(size_t index,uint32_t ticks);
/* Hide and retain the remaining readable duration while another host panel is
 * open. Call before Tick with the same clock when visibility changes. */
void LagoonAchievementUI_PauseToast(int paused,uint32_t ticks);
int LagoonAchievementUI_ToastActive(void);
int LagoonAchievementUI_ToastImage(const uint32_t **pixels,int *width,int *height);
