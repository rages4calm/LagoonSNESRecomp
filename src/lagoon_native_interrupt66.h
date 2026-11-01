#pragma once
#include "cpu_state.h"
#include <stdint.h>
#include <stddef.h>
int LagoonInterrupt66_Source(const uint8_t*,size_t);
int LagoonInterrupt66_Process(const uint8_t*,size_t,CpuState*,const CpuState*,uint64_t,uint64_t,uint32_t,uint32_t,uint16_t,uint32_t*);
void LagoonInterrupt66_BeforeFrame(void);
