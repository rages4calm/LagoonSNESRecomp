#pragma once
#include "cpu_state.h"
#include <stddef.h>
#include <stdint.h>
int LagoonNmiPrep68_Source(const uint8_t*,size_t);
int LagoonNmiPrep68_Process(const uint8_t*,size_t,CpuState*,const CpuState*,uint64_t,uint64_t,uint32_t,uint32_t,uint16_t,uint32_t*);
void LagoonNmiPrep68_BeforeFrame(void);
