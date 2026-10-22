/* Offline host achievements. Never part of serialized guest state. */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct LagoonAchievementRow {
  const char *id, *title, *description;
  uint32_t progress, target;
  uint64_t unlocked_at;
  bool secret;
} LagoonAchievementRow;

/* Startup/quit only. Actual selected save-root must already be resolved.
 * Diagnostic sessions display the catalog but neither write nor award.
 * The core owns one background storage worker; no disk I/O in Observe/Get. */
bool LagoonAchievements_Init(const char *absolute_save_root, bool diagnostic_session);
void LagoonAchievements_Shutdown(void);
/* Call after a real completed host field. Source ROM must already be verified
 * by the game; gameplay_ready excludes boot/menus/loaders and invalid players.
 * New generation/explicit load silently seeds the next valid observation. */
void LagoonAchievements_Observe(const uint8_t *ram, size_t size,
                                uint64_t generation, bool gameplay_ready);
void LagoonAchievements_OnLoaded(uint64_t generation);
/* Only after an explicit, nearby user interaction with the host easter egg. */
void LagoonAchievements_SecretVisit(uint64_t generation);
/* Only after the host's source/caller-guarded, same-generation credits route
 * reaches its terminal ending loop. Credits need not be gameplay_ready. */
void LagoonAchievements_CompletedEnding(uint64_t generation);
size_t LagoonAchievements_Count(void);
bool LagoonAchievements_GetRow(size_t index, LagoonAchievementRow *out);
size_t LagoonAchievements_Unlocked(void);
const char *LagoonAchievements_Status(void);
const char *LagoonAchievements_ProfilePath(void);
bool LagoonAchievements_PopNotice(size_t *index);
/* Test/shutdown synchronization; never call from the gameplay frame path. */
bool LagoonAchievements_WaitIdle(unsigned timeout_ms);
