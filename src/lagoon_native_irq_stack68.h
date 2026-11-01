#pragma once
#include "cpu_state.h"
#include <stddef.h>
int LagoonIrqStack68_Source(const uint8_t*,size_t);
int LagoonIrqStack68_Process(const uint8_t*,size_t,CpuState*,const CpuState*,uint64_t,uint64_t,uint32_t,uint32_t,uint16_t,uint32_t*);
void LagoonIrqStack68_BeforeFrame(void);
