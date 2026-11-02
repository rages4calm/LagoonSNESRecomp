#ifndef LAGOON_RETURN_H
#define LAGOON_RETURN_H
#include <stddef.h>
#include <stdint.h>

enum { LAGOON_RETURN_RAM_BYTES=0x2000 };
typedef struct LagoonReturnView {
 const uint8_t *ram;
 size_t size;
 uint64_t generation,tick; /* Monotonic host simulation tick, not wall time. */
 uint8_t scene_qualified,host_blocked,escort_active;
} LagoonReturnView;
typedef struct LagoonReturnCpu {
 uint32_t pc24;
 uint16_t stack,y,d;
 uint8_t pb,db,m,x,e,p,decimal;
} LagoonReturnCpu;
typedef struct LagoonReturnRead {
 uint32_t pc24;
 uint16_t stack;
 uint8_t pb,m,x,e;
} LagoonReturnRead;
typedef enum LagoonReturnReason {
 LAGOON_RETURN_READY=0,LAGOON_RETURN_SOURCE,LAGOON_RETURN_WORLD,
 LAGOON_RETURN_AREA,LAGOON_RETURN_BOSS,LAGOON_RETURN_UNCLEARED,
 LAGOON_RETURN_NO_ENTRY,LAGOON_RETURN_PATH,LAGOON_RETURN_MOVING,
 LAGOON_RETURN_ACTION,LAGOON_RETURN_ESCORT,LAGOON_RETURN_ACTIVITY,
 LAGOON_RETURN_HOST,LAGOON_RETURN_ALREADY_THERE,LAGOON_RETURN_PENDING
} LagoonReturnReason;
typedef struct LagoonReturnStatus {
 LagoonReturnReason reason;
 const char *message;
 const char *dungeon;
 uint8_t entrance_room;
 uint16_t entrance_x,entrance_y;
} LagoonReturnStatus;
typedef struct LagoonReturnPlan {
 uint64_t generation,request_tick;
 uint32_t source_payload,target_payload;
 uint16_t source_cell_x,source_cell_y,source_y;
 uint8_t source_room,target_room;
} LagoonReturnPlan;

/* Original ROM before QoL patching; all state below is disposable host history.
 * These functions NEVER write RAM, CPU registers, PC, stack or guest flags. */
void LagoonReturn_Init(const uint8_t *rom,size_t size);
void LagoonReturn_Reset(uint64_t generation);
void LagoonReturn_Observe(const LagoonReturnView *view);
/* Called only at an actual original01C8B5 before any destination modification.
 * This authenticates main JSR stack80A2, current DP door table/Y and raw CA05
 * cells, then waits for the actual matching settled destination observation. */
int LagoonReturn_WitnessDoor(const LagoonReturnView *view,const LagoonReturnCpu *cpu);
/* Natural main-entry history is preferred. Observe may recover the exact
 * canonical original entrance for an already-cleared loaded save; equivalent
 * duplicate rows are allowed, different destinations or changed tables refuse. */
void LagoonReturn_Status(const LagoonReturnView *view,LagoonReturnStatus *out);
int LagoonReturn_Request(const LagoonReturnView *view,LagoonReturnStatus *out);
void LagoonReturn_Cancel(void);
/* Runtime adapter: Begin only at real main-loop01C7F6; FilterRead participates
 * in original attribute dispatch and door comparisons. It changes the returned
 * data view only. All original normalization/latch/call instructions still run. */
int LagoonReturn_Begin(const LagoonReturnView *view,const LagoonReturnCpu *cpu);
int LagoonReturn_InFlight(void);
/* Observe original main-loop resume0180A3. Cancels an unactivated request;
 * returns-1 for a bad resume contract or an activated, unconsumed virtual door. */
int LagoonReturn_End(const LagoonReturnView *view,const LagoonReturnCpu *cpu);
uint8_t LagoonReturn_FilterRead(const LagoonReturnView *view,const LagoonReturnRead *read,
                               uint32_t ram_offset,uint8_t value);
/* Consume at original01C8B5 only after the complete authorized read sequence.
 * On success the caller may substitute DP40..42=target_payload and Y=0, then
 * execute the SAME original opcode/loader. Never move PC/stack or store048E.
 * A successful consume must NOT also be recorded as an ordinary WitnessDoor.
 * Returns-1 if an activated virtual-door contract is lost: the host MUST stop
 * before this opcode rather than let a partly authorized virtual warp continue.
 * Zero means no virtual door was activated; one means a complete authorized plan. */
int LagoonReturn_Consume(const LagoonReturnView *view,const LagoonReturnCpu *cpu,
                        LagoonReturnPlan *out);
#endif
