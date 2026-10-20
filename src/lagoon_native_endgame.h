#pragma once
#include "snes/interp_bridge.h"
typedef struct LagoonNativeEndgameStats {
  unsigned long long calls[3],armed[3],proposals[3],takes[3],refusals;
  unsigned long long waits[13],queue_ancestors;
} LagoonNativeEndgameStats;
void LagoonNativeEndgameBeforeFrame(void);
unsigned LagoonNativeEndgameDescriptors(InterpAotContinuation *out,unsigned cap);
int LagoonNativeEndgameSource(const InterpAotContinuation *entry);
int LagoonNativeEndgameHook(unsigned event,const CpuState *cpu,uint32_t source,
    uint32_t target,const InterpAotContinuation *entry,uint16_t owner_entry_s,
    InterpAotAncestor *proposal);
const LagoonNativeEndgameStats *LagoonNativeEndgameGetStats(void);
