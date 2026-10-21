#pragma once
#include "cpu_state.h"
#include <stddef.h>
/* Original RAM-only leaf semantics, before their genuine RTS. */
int LagoonActorCompute_Apply(unsigned,uint8_t*,CpuState*);
bool LagoonActorCompute_Run(unsigned,const uint8_t*,size_t,uint8_t*,CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*,int);
bool LagoonActorCompute_Clamp(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*);
bool LagoonActorCompute_Project(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*);
/* Original terrain/quadrant lookup, restricted to the enemy bounds owner. */
int LagoonActorCompute_SampleApply(const uint8_t*,size_t,uint8_t*,CpuState*);
bool LagoonActorCompute_SampleRun(const uint8_t*,size_t,uint8_t*,CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*,int);
bool LagoonActorCompute_Sample(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*);
int LagoonActorCompute_IntegrateApply(uint8_t*,CpuState*);
bool LagoonActorCompute_Integrate(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*);
int LagoonActorCompute_OffscreenApply(uint8_t*,CpuState*);
bool LagoonActorCompute_OffscreenRun(const uint8_t*,size_t,uint8_t*,CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*,int);
bool LagoonActorCompute_Offscreen(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*);

int LagoonActorCompute_CellApply(uint8_t*,CpuState*);
bool LagoonActorCompute_CellRun(const uint8_t*,size_t,uint8_t*,CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*,int);
bool LagoonActorCompute_Cell(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*);
