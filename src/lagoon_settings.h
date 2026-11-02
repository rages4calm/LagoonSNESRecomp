#ifndef LAGOON_SETTINGS_H
#define LAGOON_SETTINGS_H
#include <stddef.h>
#include <stdint.h>
enum {LAGOON_SETTINGS_UP=1,LAGOON_SETTINGS_DOWN,LAGOON_SETTINGS_LEFT,LAGOON_SETTINGS_RIGHT,LAGOON_SETTINGS_CLOSE,LAGOON_SETTINGS_ACTIVATE,LAGOON_SETTINGS_NEXT_TAB,LAGOON_SETTINGS_PREVIOUS_TAB};
#define LAGOON_SETTINGS_PREV_TAB LAGOON_SETTINGS_PREVIOUS_TAB
/* Host preferences only. Call from the host/main thread at a frame boundary;
 * guest execution remains frozen by the title-overlay host while open. */
void LagoonSettings_Configure(const char *config_path);
/* Copy a bounded read-only adventure view after a completed game frame. */
void LagoonSettings_Observe(const uint8_t *ram,size_t size);
/* The host supplies current travel eligibility. Text is copied; this UI never
 * decides whether a return is safe or changes the adventure itself. */
void LagoonSettings_SetTravelStatus(const char *message,int available);
void LagoonSettings_Open(void);
void LagoonSettings_Close(void);
int LagoonSettings_IsOpen(void);
/* Host-only optional attack-range drawing; never affects combat/hit detection. */
int LagoonSettings_SwordRangeEnabled(void);
int LagoonSettings_PollOpen(uint32_t human);
/* A host action only. Consume once at the overlay boundary, then open the
 * achievement browser and retain the host's release-before-gameplay input gate. */
int LagoonSettings_TakeAchievementsRequest(void);
/* Two explicit confirmations queue one request. The host must revalidate it
 * against the live game state before returning to the entrance. */
int LagoonSettings_TakeReturnRequest(void);
void LagoonSettings_Command(unsigned command);
void LagoonSettings_Tick(uint32_t human,uint32_t ticks);
int LagoonSettings_Image(const uint32_t **pixels,int *width,int *height);
#endif
