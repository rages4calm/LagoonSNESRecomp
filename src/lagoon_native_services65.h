#pragma once
#include "cpu_state.h"
#include <stdint.h>
#include <stddef.h>
int LagoonServices65_Source(const uint8_t*,size_t);
int LagoonServices65_Process(const uint8_t*,size_t,CpuState*,const CpuState*,uint64_t,uint64_t,uint32_t,uint32_t*);
void LagoonServices65_BeforeFrame(void);
