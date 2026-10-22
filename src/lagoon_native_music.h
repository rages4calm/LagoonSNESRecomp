#pragma once
#include "snes/interp_bridge.h"
#include <stddef.h>
bool LagoonNativeMusic_Try(const uint8_t *rom,size_t size,CpuState *cpu,
    const CpuState *current,uint64_t generation,uint64_t frame_generation,bool frame_registered,
    unsigned writer,uint16_t entry_s,uint8_t hrv,uint32_t host_return,RecompReturn *out);
bool LagoonNativeMusic_Port0(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*);
bool LagoonNativeMusic_Port1(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*);
bool LagoonNativeMusic_Port2(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*);
void LagoonNativeMusic_BeforeFrame(void);

bool LagoonNativeMusic_UploadTry(const uint8_t*,size_t,CpuState*,const CpuState*,uint64_t,uint64_t,bool,unsigned,uint16_t,uint8_t,uint32_t,RecompReturn*);
bool LagoonNativeMusic_Bank(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*);
bool LagoonNativeMusic_Samples(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*);
bool LagoonNativeMusic_Transfer(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*);

bool LagoonNativeMusic_ToggleTry(const uint8_t*,size_t,CpuState*,const CpuState*,uint64_t,uint64_t,bool,uint16_t,uint8_t,uint32_t,RecompReturn*);
bool LagoonNativeMusic_Toggle(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*);
