#ifndef LAGOON_EASTER_COLLISION_H
#define LAGOON_EASTER_COLLISION_H
#include <stddef.h>
#include <stdint.h>
struct CpuWramReadContext;
/* The cameo owns its source/room/site qualification and world foot position. */
typedef int (*LagoonEasterCollisionBody)(const uint8_t *ram,uint64_t generation,
    int *x,int *y);
void LagoonEasterCollision_Init(const uint8_t *rom,size_t size,
    LagoonEasterCollisionBody body);
void LagoonEasterCollision_BeforeFrame(void);
/* Original movement still executes. Only its scoped velocity operand view can
 * become zero; no host code writes guest position, velocity, camera or actors. */
uint8_t LagoonEasterCollision_FilterRead(uint32_t address,uint8_t value,
    const struct CpuWramReadContext *context,const uint8_t *ram);
unsigned long long LagoonEasterCollision_BlockedSteps(void);
unsigned long long LagoonEasterCollision_FilteredReads(void);
#endif
