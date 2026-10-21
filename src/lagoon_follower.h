#ifndef LAGOON_FOLLOWER_H
#define LAGOON_FOLLOWER_H
#include <stddef.h>
#include <stdint.h>

/* Host history is disposable authority. It is never serialized into guest
 * saves and must be reset on every state/room/execution-generation change. */
enum { LAGOON_FOLLOWER_DELAY = 6, LAGOON_FOLLOWER_HISTORY = 256 };
typedef struct LagoonFollowerStep {
  int16_t dx, dy;
  uint16_t target_x, target_y, height;
  uint8_t direction, moving, connector;
} LagoonFollowerStep;
void LagoonFollower_Reset(const uint8_t *rom, size_t size);
void LagoonFollower_Discard(void);
/* Call once at the source-qualified ordinary Nasir actor update, before its
 * current movement. Observes the completed previous step, never intended input.
 * Both routines below are read-only with respect to all guest bytes. */
int LagoonFollower_Observe(const uint8_t *ram, uint64_t generation);
int LagoonFollower_Plan(const uint8_t *ram, unsigned actor_offset,
                        uint64_t generation, LagoonFollowerStep *out);
int LagoonFollower_Role(const uint8_t *ram, unsigned actor_offset);
/* Runtime integration: Reset after ROM QoL preparation, BeforeFrame before
 * guest execution, Discard on reset/load/rollback. No SDK change is required. */
void LagoonFollower_BeforeFrame(void);
int LagoonFollower_OwnsVelocity(unsigned ram_offset);
struct CpuWramReadContext;
uint8_t LagoonFollower_FilterRead(uint32_t ram_offset,uint8_t value,
    const struct CpuWramReadContext *context,const uint8_t *ram);
#endif
