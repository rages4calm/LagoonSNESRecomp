#pragma once
#include <stdint.h>
#include <stddef.h>
/* A complete outgoing presentation, used during a loader or its one-field
 * descriptor handoff. room_key is room+1 only after the caller independently
 * qualifies its exact authored descriptor; zero means unknown. frame is the
 * completed simulation-field serial, so repeated presentations cannot exhaust
 * the handoff or revive a stale scene. No guest state or partial incoming
 * pixels are cached. */
void LagoonTransition_Reset(void);
int LagoonTransition_Apply(uint8_t *dst,size_t pitch,unsigned width,unsigned height,
    uint64_t generation,int coherent,int loading,unsigned brightness,unsigned room_key,
    unsigned frame);
