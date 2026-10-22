#pragma once
#include "snes/interp_bridge.h"

bool LagoonInlineTable_Run(CpuState *cpu, uint16_t entry_s, uint8_t hrv,
                          uint32_t host_return, RecompReturn *out);
