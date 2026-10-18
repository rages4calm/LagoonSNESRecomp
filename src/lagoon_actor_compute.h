#pragma once
#include "cpu_state.h"
#include <stddef.h>
/* Original RAM-only leaf semantics, before their genuine RTS. */
int LagoonActorCompute_Apply(unsigned,uint8_t*,CpuState*);
bool LagoonActorCompute_Run(unsigned,const uint8_t*,size_t,uint8_t*,CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*,int);
bool LagoonActorCompute_Clamp(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*);
bool LagoonActorCompute_Project(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*);
