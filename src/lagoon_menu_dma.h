#pragma once
#include "cpu_state.h"
#include <stddef.h>
/* Physical device authority is supplied afresh at the original request poll. */
typedef struct LagoonMenuDmaAuthority {
 unsigned forced_blank,hdma,idle;
} LagoonMenuDmaAuthority;
typedef void (*LagoonMenuDmaWrite)(void*,uint16_t,uint8_t);
/* False is entirely read-only, including the callback. True performs one real
 * original linear transfer. */
int LagoonMenuDma_Service(const uint8_t*,size_t,uint8_t*,const CpuState*,
 uint32_t,LagoonMenuDmaAuthority,LagoonMenuDmaWrite,void*);
bool LagoonMenuDma_Run(const uint8_t*,size_t,uint8_t*,CpuState*,uint16_t,uint8_t,
 uint32_t,RecompReturn*,LagoonMenuDmaAuthority,LagoonMenuDmaWrite,void*);
bool LagoonMenuDma_TryNative(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*);

void LagoonMenuDma_BeforeFrame(void);
