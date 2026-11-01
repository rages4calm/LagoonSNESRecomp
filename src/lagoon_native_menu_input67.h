#pragma once
#include "cpu_state.h"
#include <stdint.h>
#include <stddef.h>
int LagoonMenuInput67_Source(const uint8_t*,size_t);
int LagoonMenuInput67_Process(const uint8_t*,size_t,CpuState*,const CpuState*,uint64_t,uint64_t,uint32_t,uint32_t*);
void LagoonMenuInput67_BeforeFrame(void);
