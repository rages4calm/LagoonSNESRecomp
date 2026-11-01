#pragma once
#include "cpu_state.h"
#include <stddef.h>
#include <stdint.h>
int LagoonPlayerIntent68_Source(const uint8_t*,size_t);
int LagoonPlayerIntent68_Process(const uint8_t*,size_t,CpuState*,const CpuState*,uint64_t,uint64_t,uint32_t,uint32_t*);
void LagoonPlayerIntent68_BeforeFrame(void);
bool LagoonPlayerIntent68_DispatchEntry(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*);
