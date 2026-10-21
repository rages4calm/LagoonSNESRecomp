#ifndef LAGOON_SETTINGS_H
#define LAGOON_SETTINGS_H
#include <stdint.h>
enum {LAGOON_SETTINGS_UP=1,LAGOON_SETTINGS_DOWN,LAGOON_SETTINGS_LEFT,LAGOON_SETTINGS_RIGHT,LAGOON_SETTINGS_CLOSE};
/* Host preferences only. Call from the host/main thread at a frame boundary;
 * guest execution remains frozen by the title-overlay host while open. */
void LagoonSettings_Configure(const char *config_path);
void LagoonSettings_Open(void);
void LagoonSettings_Close(void);
int LagoonSettings_IsOpen(void);
int LagoonSettings_PollOpen(uint32_t human);
void LagoonSettings_Command(unsigned command);
void LagoonSettings_Tick(uint32_t human,uint32_t ticks);
int LagoonSettings_Image(const uint32_t **pixels,int *width,int *height);
#endif
