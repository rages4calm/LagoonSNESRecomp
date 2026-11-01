#pragma once
#include "cpu_state.h"
#include <stddef.h>
#include <stdint.h>
int LagoonNativeDecode63_Source(const uint8_t*,size_t);
int LagoonNativeDecode63_Process(const uint8_t*,size_t,CpuState*,const CpuState*,uint64_t,uint64_t,uint32_t,uint32_t*);
void LagoonNativeDecode63_BeforeFrame(void);
void LagoonNativeDecode63_ObserveEntry(CpuState*,uint16_t,uint8_t,uint32_t);
