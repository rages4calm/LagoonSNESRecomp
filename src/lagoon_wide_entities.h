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
/* Host-only outgoing-scene object packet; never restores guest records. */
void LagoonWideEntities_RetainScene(void);
void LagoonWideEntities_BeginRetained(int enabled);
int LagoonWideEntities_RetainedSceneValid(void);
int LagoonWideEntities_RetainedNativeAnchors(void);
/* Opt-in report only: first refusal, source address, captured/current value.
 * Kinds: 1 packet/generation, 2 actor RAM, 3 atlas word, 4 CGRAM word,
 * 5 OAM byte, 6 object layout/priority, 7 current native pose/source. */
void LagoonWideEntities_RetainedDiagnostic(unsigned *kind,unsigned *address,
    unsigned *captured,unsigned *current);

typedef struct LagoonWideRetainedMap {
  uint8_t maps[2][0x4000];
  uint16_t definitions[2][1024];
  uint8_t fill;
} LagoonWideRetainedMap;
unsigned LagoonWideRetainedMap_Tile(const LagoonWideRetainedMap *map,int layer,int x,int y);

/* Pure presentation state. These helpers never execute a guest bus access. */
typedef struct LagoonWideAnimation { uint8_t timer, phase; } LagoonWideAnimation;
int LagoonWideEntities_CommonFrame(const uint8_t *rom, size_t rom_size,
    const uint8_t *record, LagoonWideAnimation *animation, int step, uint8_t *frame);
/* Additional exact room families use the same original ordinary selectors.
 * Special/death animation dispatches never gain far presentation authority. */
int LagoonWideEntities_AnimationSource(const uint8_t *rom,size_t rom_size,unsigned type);
int LagoonWideEntities_FrameForRoom(const uint8_t *rom,size_t rom_size,unsigned room,
    const uint8_t *record,LagoonWideAnimation *animation,int step,uint8_t *frame);
int LagoonWideEntities_FrameForContext(const uint8_t *rom,size_t rom_size,const uint8_t *ram,
    const uint8_t *record,LagoonWideAnimation *animation,int step,uint8_t *frame);
int LagoonWideEntities_AllowedKind(unsigned room,unsigned type,unsigned kind);
/* Exact current guest frame, independent of old native witnesses or AI.
 * Caller additionally owns exact scene/layout/generation/atlas and culling. */
int LagoonWideEntities_CurrentSource(const uint8_t *rom,size_t size,unsigned selector,unsigned frame);
int LagoonWideEntities_CurrentFrame(const uint8_t *rom,size_t size,const uint8_t *captured,
    const uint8_t *current,unsigned entity,uint8_t *frame);
int LagoonWideEntities_RecordCells(const uint8_t *rom,size_t rom_size,const uint8_t *ram,
    unsigned entity,unsigned frame,int projected,LagoonWideCell *cells,size_t capacity);
/* Actual queued enemy projectile kinds11..33, read-only original alive band.
 * Requires the exact full current24-byte record and queue; caller additionally
 * owns room/generation/layout/palette/atlas/native OAM and stable draw order. */
int LagoonWideEntities_ProjectileCells(const uint8_t *rom,size_t size,
    const uint8_t *captured,const uint8_t *current,unsigned queue,
    LagoonWideCell *cells,size_t capacity);

/* Completed native OAM packet variant. The CPU may already be constructing
 * its next frame. Caller MUST bind every returned cell to the complete still-
 * current native OAM, original source packet, room/camera/layout and owned
 * atlas/palette/generation. No virtual record or extrapolated pose authority. */
int LagoonWideEntities_ProjectilePacketCells(const uint8_t *rom,size_t size,
    const uint8_t *packet,unsigned queue,LagoonWideCell *cells,size_t capacity);

/* Completed original equipped-spell OBJ pack, separate from the room atlas.
 * Index3 is a BG spell and never grants OBJ ownership. */
enum { kLagoonWideMagicBytes=0x7e0 };
typedef struct LagoonWideMagicAtlas {
  uint8_t bytes[kLagoonWideMagicBytes];
  uint64_t generation;
  unsigned room,form,next,left,completed,length,ready;
} LagoonWideMagicAtlas;
void LagoonWideMagic_Reset(LagoonWideMagicAtlas *atlas);
int LagoonWideMagic_Source(const uint8_t *rom,size_t size,unsigned spell);
int LagoonWideMagic_Begin(LagoonWideMagicAtlas *atlas,uint64_t generation,
    unsigned room,const uint8_t *rom,size_t size,unsigned spell,unsigned bank,
    unsigned address,unsigned destination,unsigned length);
void LagoonWideMagic_Write(LagoonWideMagicAtlas *atlas,unsigned address,uint8_t value);
int LagoonWideMagic_Match(const LagoonWideMagicAtlas *atlas,const uint16_t *vram,
    const uint8_t *rom,size_t size,uint64_t generation,unsigned room,unsigned spell);
int LagoonWideMagic_CellOwned(const LagoonWideMagicAtlas *atlas,unsigned base1,
    unsigned base2,const LagoonWideCell *cell);
/* Same original projectile carrier and source/OAM guards, kinds1..8 only.
 * Caller additionally requires the separately completed matching magic pack. */
int LagoonWideEntities_MagicPacketCells(const uint8_t *rom,size_t size,
    const uint8_t *packet,unsigned queue,LagoonWideCell *cells,size_t capacity);

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

/* Original common0 death glyphs, separate from the room OBJ atlas and live
 * Nasir staging pack. Only a completed original512-byte upload grants use. */
enum { kLagoonWideDeathBytes=0x200 };
typedef struct LagoonWideDeathAtlas {
  uint8_t bytes[kLagoonWideDeathBytes];
  uint64_t generation;
  unsigned next,left,completed,ready;
} LagoonWideDeathAtlas;
int LagoonWideDeath_Source(const uint8_t *rom,size_t size);
void LagoonWideDeath_Reset(LagoonWideDeathAtlas *atlas);
int LagoonWideDeath_Begin(LagoonWideDeathAtlas *atlas,uint64_t generation,
    const uint8_t *rom,size_t size,unsigned bank,unsigned address,unsigned destination,unsigned length);
void LagoonWideDeath_Write(LagoonWideDeathAtlas *atlas,unsigned address,uint8_t value);
int LagoonWideDeath_Match(const LagoonWideDeathAtlas *atlas,const uint16_t *vram,
    const uint8_t *rom,size_t size,uint64_t generation);
int LagoonWideDeath_CellOwned(const LagoonWideDeathAtlas *atlas,unsigned base1,unsigned base2,
    const LagoonWideCell *cell);

/* Match initialized native interior tiles against the retained map. */
int LagoonWideEntities_WorldCharacters(const uint16_t *vram,
    const uint16_t *witness,const uint8_t used[1024]);
/* Original static32x64 BG2 pages and quarter-speed raster contracts.
 * Unsuffixed Map/Scroll retain the original room07 compatibility bounds. */
int LagoonWideEntities_ParallaxMap(const uint16_t *vram,const uint8_t *rom,size_t size);
int LagoonWideEntities_ParallaxMapForRoom(const uint16_t *vram,const uint8_t *rom,size_t size,unsigned room);
int LagoonWideEntities_ParallaxScroll(unsigned x0,unsigned y0,unsigned x1,unsigned y1);
int LagoonWideEntities_ParallaxRoomScroll(unsigned room,unsigned x0,unsigned y0,unsigned x1,unsigned y1);
int LagoonWideEntities_ParallaxRaster(const unsigned camera[4],const unsigned raster[4]);
/* Exact room-owned outgoing pages and four quarter-speed anchors, bound to
 * timeline and compressed source. Capture is the room07 compatibility wrapper. */
typedef struct LagoonWideParallaxPacket {
  uint16_t pages[0x800];
  unsigned camera[4];
  uint64_t generation;
  unsigned room;
  int valid;
} LagoonWideParallaxPacket;
int LagoonWideParallaxPacket_CaptureRoom(LagoonWideParallaxPacket *packet,
    const uint16_t *vram,const uint8_t *rom,size_t size,uint64_t generation,
    const unsigned camera[4],unsigned room);
int LagoonWideParallaxPacket_Capture(LagoonWideParallaxPacket *packet,
    const uint16_t *vram,const uint8_t *rom,size_t size,uint64_t generation,
    const unsigned camera[4]);
int LagoonWideParallaxPacket_Match(const LagoonWideParallaxPacket *packet,
    const uint16_t *vram,const uint8_t *rom,size_t size,uint64_t generation,
    const unsigned camera[4],const unsigned raster[4]);
/* Exact original NMI top-edge stream:34 [BG1 word address,tile] pairs.
 * Preparing a new job never lends authority to an incomplete earlier job. */
typedef struct LagoonWideParallaxQueue {
  uint16_t address[34],tile[34];
  uint64_t generation;
  unsigned next;
  int valid;
} LagoonWideParallaxQueue;
int LagoonWideParallaxQueue_Begin(LagoonWideParallaxQueue *queue,
    const LagoonWideRetainedMap *map,const uint8_t *source,size_t size,
    const unsigned camera[4],uint64_t generation);
int LagoonWideParallaxQueue_Write(LagoonWideParallaxQueue *queue,
    unsigned address,unsigned value,uint64_t generation);
int LagoonWideParallaxQueue_Pending(const LagoonWideParallaxQueue *queue);
int LagoonWideEntities_WorldTilemap(const uint16_t *vram,unsigned base,
    int tile_x,int tile_y,const uint16_t expected[720]);
/* Exact streamed world graphics upload from $01:8875/$01:88CD. */
enum { kLagoonWideBackgroundBytes=0x37C0 };
typedef struct LagoonWideBackground {
  uint8_t bytes[kLagoonWideBackgroundBytes];
  uint64_t generation;
  unsigned room,source,completed,next,left,ready;
} LagoonWideBackground;
void LagoonWideBackground_Reset(LagoonWideBackground *background);
int LagoonWideBackground_Begin(LagoonWideBackground *background,uint64_t generation,
    unsigned room,unsigned source,unsigned bank,unsigned address,unsigned destination,unsigned length);
void LagoonWideBackground_Write(LagoonWideBackground *background,unsigned address,uint8_t value);

/* Audited room01 two-frame / room02 four-frame / cave three-frame animation,
 * uploaded by $00:8F19. Room02's original12-entry table repeats four frames.
 * Only completed, byte-verified transfers in this room/timeline are witnesses. */
enum { kLagoonWideTorchBytes=0xC0, kLagoonWideTorchMaxBytes=0x200, kLagoonWideTorchAddress=0x7600 };
typedef struct LagoonWideTorches {
  uint8_t bytes[8][kLagoonWideTorchMaxBytes];
  uint8_t pending[kLagoonWideTorchMaxBytes];
  uint64_t generation;
  unsigned room, source, next, left, frame, seen;
} LagoonWideTorches;
unsigned LagoonWideTorches_Bytes(unsigned room,unsigned source);
void LagoonWideTorches_Reset(LagoonWideTorches *torches);
int LagoonWideTorches_Begin(LagoonWideTorches *torches,uint64_t generation,
    unsigned room,unsigned source,unsigned bank,unsigned address,unsigned destination,unsigned length);
void LagoonWideTorches_Write(LagoonWideTorches *torches,unsigned address,uint8_t value,
    const uint8_t *rom,size_t size);
/* Returns the actual completed render phase (0..7), or -1 on refusal. */
int LagoonWideTorches_Match(const LagoonWideTorches *torches,const uint16_t *vram,
    uint64_t generation,unsigned room,unsigned source);

int LagoonWideEntities_MenuOverlay(const uint16_t *vram, unsigned map_base, unsigned char_base);
void LagoonWideEntities_BlankMargins(uint16_t *depth, unsigned native_offset, unsigned left, unsigned right);

/* Finite audited rooms requiring masking beyond their actual map extent. */
int LagoonWideEntities_BoundedRoom(unsigned room,unsigned width,unsigned height);
/* Only resource combinations with actual retained-map/color qualification. */
int LagoonWideEntities_AuditedRoom(unsigned room, unsigned bg_source, unsigned obj_source,
    unsigned definitions0, unsigned definitions1, unsigned map0, unsigned map1);

void LagoonWideEntities_BlankField(uint8_t *pixels, size_t pitch, unsigned width, unsigned height);

typedef struct LagoonWideSceneGate { uint64_t generation; unsigned key, fields; } LagoonWideSceneGate;
int LagoonWideEntities_SceneSettled(LagoonWideSceneGate *gate, int ready, unsigned key, uint64_t generation);

/* Mask only added pixels; native columns and row padding stay untouched. */
void LagoonWideEntities_MaskRoomRow(uint8_t *pixels, unsigned width,
    int world_x, int world_y, unsigned room_width, unsigned room_height);
