#pragma once
#include "snes/interp_bridge.h"
typedef struct LagoonNativeRevivalStats {
  unsigned long long wait_takes, ancestor_returns, refusals;
} LagoonNativeRevivalStats;
unsigned LagoonNativeRevivalDescriptors(InterpAotContinuation *out,unsigned cap);
int LagoonNativeRevivalSource(const InterpAotContinuation *entry);
int LagoonNativeRevivalHook(unsigned event,const CpuState *cpu,uint32_t source,
    uint32_t target,const InterpAotContinuation *entry,uint16_t owner_entry_s,
    InterpAotAncestor *proposal);
const LagoonNativeRevivalStats *LagoonNativeRevivalGetStats(void);
