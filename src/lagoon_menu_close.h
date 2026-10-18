#pragma once
#include "cpu_state.h"
#include <stdint.h>
#include <stddef.h>
int LagoonMenuClose_BuildMap(const uint8_t *rom,size_t size,uint8_t *ram,CpuState *cpu);
int LagoonMenuClose_QueueTile(uint8_t *ram,CpuState *cpu,unsigned *landing);
int LagoonMenuClose_PrepareTile(const uint8_t *rom,size_t size,uint8_t *ram,CpuState *cpu);
int LagoonMenuClose_AdvanceTile(uint8_t *ram,CpuState *cpu,unsigned *landing);
void LagoonMenuClose_BeforeFrame(void);
