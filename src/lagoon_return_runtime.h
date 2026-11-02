#ifndef LAGOON_RETURN_RUNTIME_H
#define LAGOON_RETURN_RUNTIME_H
#include "lagoon_return.h"
struct CpuWramReadContext;

/* Register after original source authentication, before live QoL patches.
 * One real main-CPU timeline; no campaign cheat driver is enabled. */
void LagoonReturnRuntime_OnRom(const uint8_t *rom,size_t size);
void LagoonReturnRuntime_BeforeFrame(uint64_t tick,int host_blocked);
void LagoonReturnRuntime_AfterFrame(void);
uint8_t LagoonReturnRuntime_FilterRead(uint32_t address,uint8_t value,
    const struct CpuWramReadContext *context,const uint8_t *ram);
void LagoonReturnRuntime_GetStatus(LagoonReturnStatus *out);
int LagoonReturnRuntime_Request(LagoonReturnStatus *out);
#endif
