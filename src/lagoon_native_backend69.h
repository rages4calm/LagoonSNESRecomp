#pragma once
#include <stddef.h>
#include <stdint.h>
/* Called first by OnRom, before SDK policy queries or CPU execution. Release
 * default=1 installs only absent backend/strict=1 and boot62=off policies;
 * explicit0/0 remains a
 * development reference. Repeated calls require the same process-latched
 * strict policy. No generated program/source authority is granted here. */
void LagoonNativeBackend69_ConfigureExecution(void);
void LagoonNativeBackend69_OnRom(const uint8_t*,size_t);
void LagoonNativeBackend69_BeforeFrame(void);
