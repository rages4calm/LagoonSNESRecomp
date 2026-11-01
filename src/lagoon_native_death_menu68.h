#pragma once
#include "cpu_state.h"
#include <stdint.h>
#include <stddef.h>
int LagoonDeathMenu68_Source(const uint8_t*,size_t);
int LagoonDeathMenu68_Process(const uint8_t*,size_t,CpuState*,const CpuState*,uint64_t,uint64_t,uint32_t,uint32_t*);
void LagoonDeathMenu68_BeforeFrame(void);
bool LagoonDeathMenu68_DispatchEntry(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*);
