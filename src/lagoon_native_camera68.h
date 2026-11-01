#pragma once
#include "cpu_state.h"
#include <stddef.h>
#include <stdint.h>
int LagoonCamera68_Source(const uint8_t*,size_t);
int LagoonCamera68_Process(const uint8_t*,size_t,CpuState*,const CpuState*,uint64_t,uint64_t,uint32_t,uint32_t*);
void LagoonCamera68_BeforeFrame(void);
