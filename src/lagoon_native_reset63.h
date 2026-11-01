#pragma once
#include "cpu_state.h"
#include <stddef.h>
#include <stdint.h>
int LagoonNativeReset63_Source(const uint8_t*,size_t);
int LagoonNativeReset63_Process(const uint8_t*,size_t,CpuState*,const CpuState*,uint64_t,uint64_t,uint32_t,uint32_t*);
void LagoonNativeReset63_BeforeFrame(void);
