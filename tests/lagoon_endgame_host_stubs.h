/* Standalone continuation-policy tests do not run host frame registration.
 * Keep these boundaries inert; fixture and music execution have separate
 * original-opcode/unit/device tests. Never stub timing, RAM, IO or matcher code. */
#pragma once
#include "snes/interp_bridge.h"
CpuState g_cpu;
void LagoonNativeMusic_BeforeFrame(void){}
bool interp_bridge_add_pre_opcode_hook(uint32_t pc,InterpPreOpcodeHook hook){
  (void)pc;(void)hook;return true;
}
