#pragma once
#include "cpu_state.h"
#include <stdint.h>
#include <stddef.h>
int LagoonSprites67_Source(const uint8_t*,size_t);
int LagoonSprites67_Process(const uint8_t*,size_t,CpuState*,const CpuState*,uint64_t,uint64_t,uint32_t,uint32_t*);
void LagoonSprites67_BeforeFrame(void);
