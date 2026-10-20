#pragma once
#include "snes/interp_bridge.h"
typedef struct LagoonNativeCameraStats {
  unsigned long long calls,armed,proposals,takes,refusals;
} LagoonNativeCameraStats;
void LagoonNativeCameraBeforeFrame(void);
void LagoonNativeCameraObserve(unsigned event,const CpuState *cpu,uint32_t source,
    uint32_t target,const InterpAotContinuation *entry,uint16_t owner_entry_s);
const LagoonNativeCameraStats *LagoonNativeCameraGetStats(void);
