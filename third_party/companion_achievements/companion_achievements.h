/* SPDX-License-Identifier: MIT */
#ifndef COMPANION_ACHIEVEMENTS_H
#define COMPANION_ACHIEVEMENTS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define CA_VERSION "0.1.0"
#define CA_MAX_ACHIEVEMENTS 64
typedef enum { CA_COUNTER, CA_MILESTONE } ca_rule;
typedef struct {
    const char *id, *title, *description, *event;
    uint32_t target;
    ca_rule rule;
} ca_definition;
typedef struct { uint32_t progress; uint64_t unlocked_at; } ca_state;
typedef bool (*ca_commit)(void *context, const ca_state *states, size_t count);
typedef void (*ca_notify)(void *context, const ca_definition *definition, const ca_state *state);
typedef struct {
    const ca_definition *definitions;
    size_t count;
    ca_state states[CA_MAX_ACHIEVEMENTS];
    ca_commit commit;
    ca_notify notify;
    void *context;
} ca_engine;
/* Storage and notifications belong to the host. Commit happens before notices.
 * Events represent completed actions, not requests to perform those actions. */
bool ca_init(ca_engine *engine, const ca_definition *definitions, size_t count,
             ca_commit commit, ca_notify notify, void *context);
bool ca_restore(ca_engine *engine, const char *id, uint32_t progress, uint64_t unlocked_at);
bool ca_event(ca_engine *engine, const char *event, uint32_t amount, uint64_t timestamp);
size_t ca_unlocked(const ca_engine *engine);
#endif
