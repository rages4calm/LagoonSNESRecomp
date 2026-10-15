#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct LagoonWideCell {
  int x, y;
  uint16_t tile;
  uint8_t size;
} LagoonWideCell;

/* ROM-free tests supply synthetic LoROM bytes. No guest code is executed. */
int LagoonWideEntities_Decode(const uint8_t *rom, size_t rom_size,
    unsigned selector, unsigned frame, int x, int y,
    LagoonWideCell *cells, size_t capacity);
void LagoonWideEntities_Init(void);
int LagoonWideEntities_Pixels(const uint16_t *vram, const uint16_t *cgram,
    unsigned base1, unsigned base2, const LagoonWideCell *cell,
    uint16_t *pixels, int clip_native);
void LagoonWideEntities_Reset(void);
void LagoonWideEntities_BeginFrame(int enabled);

/* Pure presentation state. These helpers never execute a guest bus access. */
typedef struct LagoonWideAnimation { uint8_t timer, phase; } LagoonWideAnimation;
int LagoonWideEntities_CommonFrame(const uint8_t *rom, size_t rom_size,
    const uint8_t *record, LagoonWideAnimation *animation, int step, uint8_t *frame);
enum { kLagoonWideAtlasBytes = 0x2a00 };
typedef struct LagoonWideAtlas {
  uint8_t owned[kLagoonWideAtlasBytes];
  uint64_t generation;
  unsigned source, room, completed, transfer_next, transfer_left, ready;
} LagoonWideAtlas;
void LagoonWideAtlas_Reset(LagoonWideAtlas *atlas);
int LagoonWideAtlas_Begin(LagoonWideAtlas *atlas, uint64_t generation,
    unsigned room, unsigned source, unsigned bank, unsigned address,
    unsigned destination, unsigned length);
void LagoonWideAtlas_Write(LagoonWideAtlas *atlas, unsigned byte_address);
int LagoonWideAtlas_CellOwned(const LagoonWideAtlas *atlas, unsigned base1,
    unsigned base2, const LagoonWideCell *cell);

int LagoonWideEntities_MenuOverlay(const uint16_t *vram, unsigned map_base, unsigned char_base);
void LagoonWideEntities_BlankMargins(uint16_t *depth, unsigned native_offset, unsigned left, unsigned right);

/* Only resource combinations with actual retained-map/color qualification. */
int LagoonWideEntities_AuditedRoom(unsigned room, unsigned bg_source, unsigned obj_source,
    unsigned definitions0, unsigned definitions1, unsigned map0, unsigned map1);

void LagoonWideEntities_BlankField(uint8_t *pixels, size_t pitch, unsigned width, unsigned height);

typedef struct LagoonWideSceneGate { uint64_t generation; unsigned key, fields; } LagoonWideSceneGate;
int LagoonWideEntities_SceneSettled(LagoonWideSceneGate *gate, int ready, unsigned key, uint64_t generation);

/* Mask only added pixels; native columns and row padding stay untouched. */
void LagoonWideEntities_MaskRoomRow(uint8_t *pixels, unsigned width,
    int world_x, int world_y, unsigned room_width, unsigned room_height);
