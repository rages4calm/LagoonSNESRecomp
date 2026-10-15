/* Read-only ordinary NPC and nondeath enemy sprite presentation. Capture at the real OAM
 * DMA boundary; never widen guest culling, run guest drawing again, or alter
 * the native viewport. Unsupported streams and ownership fail closed. */
#include "lagoon_wide_entities.h"
#ifndef LAGOON_WIDE_ENTITIES_DECODER_ONLY
#include "common_rtl.h"
#include "config.h"
#include "lagoon_widescreen.h"
#include "snes/dma.h"
#include "snes/ppu.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { kEntities = 33, kCells = 64, kObjects = 512, kRomSize = 0x100000 };
#ifndef LAGOON_WIDE_ENTITIES_DECODER_ONLY
typedef struct WideEntitySnapshot {
  uint8_t ram[0x1100], oam[544];
  uint64_t generation;
  unsigned serial;
  int valid;
} WideEntitySnapshot;
static WideEntitySnapshot s_frame;
static unsigned s_serial, s_last_serial;
static LagoonWideAtlas s_atlas;
typedef struct WidePoseHistory {
  LagoonWideAnimation animation;
  uint8_t type, kind, clock, valid;
  uint16_t x, y;
  unsigned serial;
} WidePoseHistory;
static WidePoseHistory s_pose[kEntities];
static uint8_t s_kind_witness[2][64];
static unsigned s_pose_room_key, s_last_far_entity, s_last_far_frame;
static uint64_t s_pose_generation;
static int s_last_far_x;
static uint64_t s_far_emitted, s_frame_witness, s_frame_bad, s_far_gated;
static int s_has_far;

static PpuExtraObject s_objects[kObjects];
static uint16_t s_pixels[kObjects][256];
static size_t s_object_count;
static uint64_t s_captures, s_validated, s_rejected, s_emitted;
static void report_status(const char *status, unsigned entity, unsigned cell) {
  const char *path = getenv("LAGOON_WIDE_ENTITY_REPORT");
  if (!path) return;
  unsigned left = 0, right = 0;
  for (size_t i = 0; i < s_object_count; ++i) {
    left += s_objects[i].x < 0;
    right += s_objects[i].x + s_objects[i].width > 256;
  }
  FILE *f = fopen(path, "w");
  if (!f) return;
  fprintf(f, "status=%s serial=%u captures=%llu validated=%llu rejected=%llu emitted=%llu live_objects=%zu left=%u right=%u entity=%u cell=%u atlas_ready=%u atlas_bytes=%u frame_witness=%llu frame_bad=%llu far_cells=%llu far_gated=%llu last_far=%03X far_x=%d far_frame=%u\n",
      status, s_frame.serial, (unsigned long long)s_captures, (unsigned long long)s_validated,
      (unsigned long long)s_rejected, (unsigned long long)s_emitted, s_object_count, left, right, entity, cell, s_atlas.ready, s_atlas.completed,
      (unsigned long long)s_frame_witness, (unsigned long long)s_frame_bad,
      (unsigned long long)s_far_emitted, (unsigned long long)s_far_gated,
      s_last_far_entity, s_last_far_x, s_last_far_frame);
  fclose(f);
}
#endif

static int rom_byte(const uint8_t *rom, size_t size, unsigned a, unsigned *v) {
  if (a > 0xffffff || (a & 0xffff) < 0x8000) return 0;
  size_t off = (((a >> 16) & 0x7f) << 15) | (a & 0x7fff);
  if (off >= size) return 0;
  *v = rom[off];
  return 1;
}
static int rom_word(const uint8_t *rom, size_t size, unsigned a, unsigned *v) {
  unsigned lo, hi;
  if (!rom_byte(rom, size, a, &lo) || !rom_byte(rom, size, a + 1, &hi)) return 0;
  *v = lo | (hi << 8); return 1;
}

int LagoonWideEntities_Decode(const uint8_t *rom, size_t size,
    unsigned selector, unsigned frame, int x, int y,
    LagoonWideCell *cells, size_t capacity) {
  unsigned lo, hi, bank, table, ptr, dims, flags;
  if (!rom || !cells || selector > 0x48 || frame > 255 ||
      !rom_byte(rom, size, 0x999d + selector * 3, &lo) ||
      !rom_byte(rom, size, 0x999e + selector * 3, &hi) ||
      !rom_byte(rom, size, 0x999f + selector * 3, &bank)) return -1;
  table = lo | (hi << 8) | (bank << 16);
  if (!rom_word(rom, size, table + frame * 2, &ptr)) return -1;
  ptr |= table & 0xff0000;
  if (!rom_byte(rom, size, ptr++, &dims) || !rom_byte(rom, size, ptr++, &flags)) return -1;
  int count = 0;
  for (int grid = 0; grid < ((flags & 0x80) ? 2 : 1); ++grid) {
    if (grid && !rom_byte(rom, size, ptr++, &dims)) return -1;
    unsigned step = ((flags ^ grid) & 1) ? 16 : 8;
    if (!(dims >> 4) || !(dims & 15)) return -1;
    for (unsigned row = 0; row < (dims & 15); ++row)
      for (unsigned col = 0; col < (dims >> 4); ++col) {
        unsigned tile, attr, dx = 0, dy = 0;
        if (!rom_byte(rom, size, ptr++, &tile) || !rom_byte(rom, size, ptr++, &attr)) return -1;
        if (tile == 255) {
          if (attr == 255) continue;
          tile = attr;
          if (!rom_byte(rom, size, ptr++, &attr) ||
              !rom_byte(rom, size, ptr++, &dx) || !rom_byte(rom, size, ptr++, &dy)) return -1;
        }
        if ((size_t)count >= capacity) return -1;
        cells[count++] = (LagoonWideCell){ x + (int)(col * step) + (int8_t)dx,
            y + (int)(row * step) + (int8_t)dy, (uint16_t)(tile | (attr << 8)), (uint8_t)step };
      }
  }
  return count;
}

int LagoonWideEntities_Pixels(const uint16_t *vram, const uint16_t *cgram,
    unsigned base1, unsigned base2, const LagoonWideCell *c,
    uint16_t *pixels, int clip_native) {
  if (!vram || !cgram || !c || !pixels || c->size > 16 || !c->size) return 0;
  unsigned base = c->tile & 0x100 ? base2 : base1;
  unsigned palette = 0x80 + ((c->tile >> 9) & 7) * 16;
  memset(pixels, 0, 256 * sizeof(*pixels));
  int opaque = 0;
  for (unsigned y = 0; y < c->size; ++y)
    for (unsigned x = 0; x < c->size; ++x) {
      int screen_x = c->x + (int)x;
      if (clip_native && screen_x >= 0 && screen_x < 256) continue;
      unsigned sx = c->tile & 0x4000 ? c->size - 1 - x : x;
      unsigned sy = c->tile & 0x8000 ? c->size - 1 - y : y;
      unsigned tile = ((((c->tile & 255) >> 4) + (sy >> 3)) << 4) |
          (((c->tile & 15) + (sx >> 3)) & 15);
      unsigned address = (base + tile * 16 + (sy & 7)) & 0x7fff;
      /* The player owns its live 36-tile staging pack at word4000..423F.
       * Ordinary room cells must resolve wholly outside that pack. */
      if (address >= 0x4000 && address < 0x4240) return 0;
      uint32_t planes = vram[address] | ((uint32_t)vram[(address + 8) & 0x7fff] << 16);
      unsigned bit = 7 - (sx & 7);
      unsigned color = ((planes >> bit) & 1) | ((planes >> (bit + 7)) & 2) |
          ((planes >> (bit + 14)) & 4) | ((planes >> (bit + 21)) & 8);
      if (color) { pixels[y * c->size + x] = 0x8000 | cgram[palette + color]; opaque = 1; }
    }
  return opaque;
}


/* Ordinary Lagoon BG3 has only its bottom HUD; modal UI uses opaque glyphs
 * above y192. Inspect actual graphics, not a guessed menu selection variable. */
int LagoonWideEntities_SceneSettled(LagoonWideSceneGate *g,int ready,unsigned key,uint64_t generation) {
  if(!ready || g->key!=key || g->generation!=generation) {
    g->fields=0;g->key=key;g->generation=generation;
  }
  if(!ready)return 0;
  if(g->fields<3)++g->fields;
  return g->fields>=3;
}

void LagoonWideEntities_MaskRoomRow(uint8_t *pixels,unsigned width,
    int world_x,int world_y,unsigned room_width,unsigned room_height) {
  if(!pixels || width<256)return;
  int left=(int)(width-256)/2;
  for(unsigned x=0;x<width;++x) {
    int screen_x=(int)x-left;
    if(screen_x>=0 && screen_x<256)continue;
    int64_t map_x=(int64_t)world_x+screen_x;
    if(map_x<0 || (uint64_t)map_x>=room_width || world_y<0 || (unsigned)world_y>=room_height)
      memset(pixels+x*4,0,4);
  }
}

void LagoonWideEntities_BlankField(uint8_t *pixels,size_t pitch,unsigned width,unsigned height) {
  if(!pixels || width<256 || pitch<(size_t)width*4)return;
  unsigned left=(width-256)/2,right=width-256-left;
  for(unsigned y=0;y<height;++y) {
    memset(pixels+y*pitch,0,left*4);
    memset(pixels+y*pitch+(left+256)*4,0,right*4);
  }
}

int LagoonWideEntities_BoundedRoom(unsigned room,unsigned width,unsigned height) {
  return (room==2 && width==512 && height==512) ||
      (room==4 && width==768 && height==256) ||
      (room==5 && width==1536 && height==1280) ||
      (room==6 && width==512 && height==256);
}

int LagoonWideEntities_AuditedRoom(unsigned room,unsigned bg,unsigned obj,
    unsigned d0,unsigned d1,unsigned m0,unsigned m1) {
  if(m0!=0x7f0000 || m1!=0x7f4000 || d0!=d1)return 0;
  return (room==0 && bg==0x138000 && obj==0x198000 && d0==0x068000) ||
         (room>=3 && room<=6 && bg==0x128b77 && obj==0x1d8000 && d0==0x069298) ||
         (room==2 && bg==0x128000 && obj==0x1a8000 && d0==0x068e90);
}

int LagoonWideEntities_WorldCharacters(const uint16_t *vram,
    const uint16_t *witness,const uint8_t used[1024]) {
  if(!vram || !witness || !used)return 0;
  for(unsigned tile=0;tile<1024;++tile)
    if(used[tile] && memcmp(vram+tile*16,witness+tile*16,32))return 0;
  return 1;
}

int LagoonWideEntities_WorldTilemap(const uint16_t *vram,unsigned base,
    int tile_x,int tile_y,const uint16_t expected[720]) {
  if(!vram || !expected || base>0x7800 || tile_x<0 || tile_y<0)return 0;
  for(unsigned y=0;y<24;++y)for(unsigned x=0;x<30;++x) {
    unsigned tx=(unsigned)tile_x+x,ty=(unsigned)tile_y+y;
    unsigned a=base+((tx&32)<<5)+(ty&31)*32+(tx&31);
    if(vram[a&32767]!=expected[y*30+x])return 0;
  }
  return 1;
}

void LagoonWideBackground_Reset(LagoonWideBackground *b) {
  if(b)memset(b,0,sizeof(*b));
}
int LagoonWideBackground_Begin(LagoonWideBackground *b,uint64_t generation,
    unsigned room,unsigned source,unsigned bank,unsigned address,unsigned destination,unsigned length) {
  if(!b)return 0;
  if(bank==0x7F && address==0x7F80 && destination==0 && length==0x1000) {
    LagoonWideBackground_Reset(b);b->generation=generation;b->room=room;b->source=source;
  }
  b->left=0;
  unsigned remaining=kLagoonWideBackgroundBytes-b->completed;
  if(b->generation!=generation || b->room!=room || b->source!=source || b->ready ||
      bank!=0x7F || address!=0x7F80+b->completed || destination!=b->completed/2 ||
      !remaining || length!=(remaining>0x1000?0x1000:remaining))return 0;
  b->next=destination*2;b->left=length;return 1;
}
void LagoonWideBackground_Write(LagoonWideBackground *b,unsigned address,uint8_t value) {
  if(!b)return;
  int expected=b->left && address==b->next;
  if(!expected) {
    if(address<kLagoonWideBackgroundBytes)b->ready=0;
    b->left=0;return;
  }
  b->bytes[address]=value;b->next++;b->left--;b->completed++;
  if(b->completed==kLagoonWideBackgroundBytes)b->ready=1;
}

void LagoonWideTorches_Reset(LagoonWideTorches *t) {
  if(t)memset(t,0,sizeof(*t));
}
int LagoonWideTorches_Begin(LagoonWideTorches *t,uint64_t generation,
    unsigned room,unsigned source,unsigned bank,unsigned address,unsigned destination,unsigned length) {
  if(!t)return 0;
  if(t->generation!=generation || t->room!=room || t->source!=source) {
    LagoonWideTorches_Reset(t);t->generation=generation;t->room=room;t->source=source;
  }
  t->left=0;
  if(room<3 || room>6 || source!=0x128B77 || bank!=0x10 ||
      destination!=0x3B00 || length!=kLagoonWideTorchBytes ||
      (address!=0x8900 && address!=0x89C0 && address!=0x8A80))return 0;
  t->frame=(address-0x8900)/kLagoonWideTorchBytes;
  t->next=kLagoonWideTorchAddress;t->left=kLagoonWideTorchBytes;return 1;
}
void LagoonWideTorches_Write(LagoonWideTorches *t,unsigned address,uint8_t value,
    const uint8_t *rom,size_t size) {
  if(!t)return;
  if(!t->left || address!=t->next) {
    if(address>=kLagoonWideTorchAddress && address<kLagoonWideTorchAddress+kLagoonWideTorchBytes)
      t->seen=0;
    t->left=0;return;
  }
  unsigned offset=address-kLagoonWideTorchAddress;
  t->pending[offset]=value;t->next++;t->left--;
  if(t->left)return;
  unsigned source=0x108900+t->frame*kLagoonWideTorchBytes;
  /* Guard the actual animation selector/descriptor and its three pointers.
   * A modified animation table cannot silently borrow these frame identities. */
  unsigned v;
  if(!rom_word(rom,size,0x1DFF0,&v) || v!=0x0304 ||
      !rom_word(rom,size,0x1DFF2,&v) || v!=0xDA6B ||
      !rom_word(rom,size,0x1DFF4,&v) || v!=0x3B00 ||
      !rom_word(rom,size,0x1DFF6,&v) || v!=kLagoonWideTorchBytes) {t->seen=0;return;}
  for(unsigned frame=0;frame<3;++frame) {
    if(!rom_word(rom,size,0x1DA6B+frame*3,&v) || v!=0x8900+frame*kLagoonWideTorchBytes ||
        !rom_byte(rom,size,0x1DA6D+frame*3,&v) || v!=0x10) {t->seen=0;return;}
  }
  for(unsigned i=0;i<kLagoonWideTorchBytes;++i)
    if(!rom_byte(rom,size,source+i,&v) || v!=t->pending[i]) {t->seen=0;return;}
  memcpy(t->bytes[t->frame],t->pending,sizeof(t->pending));t->seen|=1u<<t->frame;
}
int LagoonWideTorches_Match(const LagoonWideTorches *t,const uint16_t *vram,
    uint64_t generation,unsigned room,unsigned source) {
  if(!t || !vram || t->left || t->generation!=generation || t->room!=room || t->source!=source)return -1;
  for(unsigned frame=0;frame<3;++frame)
    if((t->seen&(1u<<frame)) && !memcmp((const uint8_t*)vram+kLagoonWideTorchAddress,
        t->bytes[frame],kLagoonWideTorchBytes))return (int)frame;
  return -1;
}

int LagoonWideEntities_MenuOverlay(const uint16_t *vram,unsigned map,unsigned chars) {
  if(!vram) return 1;
  for(unsigned i=0;i<24*32;++i) {
    unsigned tile=vram[(map+i)&32767]&1023;
    for(unsigned row=0;row<8;++row) if(vram[(chars+tile*8+row)&32767]) return 1;
  }
  return 0;
}
void LagoonWideEntities_BlankMargins(uint16_t *depth,unsigned origin,unsigned left,unsigned right) {
  if(!depth || left>origin) return;
  for(unsigned x=origin-left;x<origin;++x)depth[x]=0x0500;
  for(unsigned x=origin+256;x<origin+256+right;++x)depth[x]=0x0500;
}

int LagoonWideEntities_CommonFrame(const uint8_t *rom, size_t size,
    const uint8_t *r, LagoonWideAnimation *a, int step, uint8_t *frame) {
  if (!rom || !r || !a || !frame || r[10] > 3 || a->phase > 3) return 0;
  unsigned type = r[0x18], kind = r[0x1b], table, delay, value;
  /* First proven ordinary forms. Special/status/death paths may mutate AI,
   * rewards or global state and are deliberately not presentation code. */
  if (type == 1 && (kind == 41 || kind == 59)) {
    if (!rom_word(rom, size, 0x1f885 + kind * 2, &table)) return 0;
    table |= 0x10000;
  } else if (type == 2 && kind <= 1 && !(r[1] & 7)) {
    table = 0x1f41a + kind * 17;
  } else return 0;
  if (!rom_byte(rom, size, table, &delay) || !delay) return 0;
  int idle = !(r[0] & 0x20) && ((r[0] & 1) || (r[11] & 0x80));
  if (idle) a->timer = a->phase = 0;
  else if (step && ++a->timer == delay) { a->timer = 0; a->phase = (a->phase + 1) & 3; }
  if (!rom_byte(rom, size, table + 1 + r[10] * 4 + a->phase, &value)) return 0;
  *frame = (uint8_t)(value + (type == 2 ? (r[0x2c] << 4) : 0));
  return 1;
}

void LagoonWideAtlas_Reset(LagoonWideAtlas *a) { if (a) memset(a, 0, sizeof(*a)); }
int LagoonWideAtlas_Begin(LagoonWideAtlas *a, uint64_t gen,
    unsigned room, unsigned source, unsigned bank, unsigned adr,
    unsigned dst, unsigned len) {
  if (!a) return 0;
  if (bank == 0x7f && adr == 0x7f80 && dst == 0x4400 && len == 0x1000) {
    LagoonWideAtlas_Reset(a); a->generation = gen; a->room = room; a->source = source;
  }
  a->transfer_left = 0;
  unsigned remaining = kLagoonWideAtlasBytes - a->completed;
  if (a->generation != gen || a->room != room || a->source != source || a->ready ||
      bank != 0x7f || adr != 0x7f80 + a->completed || dst != 0x4400 + a->completed / 2 ||
      len != (remaining > 0x1000 ? 0x1000 : remaining) || !len) return 0;
  a->transfer_next = dst * 2; a->transfer_left = len;
  return 1;
}
void LagoonWideAtlas_Write(LagoonWideAtlas *a, unsigned adr) {
  if (!a) return;
  int expected = a->transfer_left && adr == a->transfer_next;
  if (adr >= 0x8800 && adr < 0xb200) a->owned[adr - 0x8800] = expected;
  if (!expected) { a->transfer_left = 0; return; }
  a->transfer_next++; a->transfer_left--; a->completed++;
  if (a->completed == kLagoonWideAtlasBytes) a->ready = 1;
}
int LagoonWideAtlas_CellOwned(const LagoonWideAtlas *a, unsigned base1,
    unsigned base2, const LagoonWideCell *c) {
  if (!a || !a->ready || !c || (c->size != 8 && c->size != 16)) return 0;
  unsigned base = c->tile & 0x100 ? base2 : base1;
  for (unsigned y = 0; y < c->size; y += 8)
    for (unsigned x = 0; x < c->size; x += 8) {
      unsigned tile = ((((c->tile & 255) >> 4) + y / 8) << 4) |
          (((c->tile & 15) + x / 8) & 15);
      unsigned adr = ((base + tile * 16) & 0x7fff) * 2;
      if (adr < 0x8800 || adr + 32 > 0xb200) return 0;
      for (unsigned b = 0; b < 32; ++b) if (!a->owned[adr - 0x8800 + b]) return 0;
    }
  return 1;
}

#ifndef LAGOON_WIDE_ENTITIES_DECODER_ONLY
static unsigned ram_word(const uint8_t *ram, unsigned a) { return ram[a] | (ram[a + 1] << 8); }
static unsigned ram_pointer(const uint8_t *ram, unsigned a) { return ram_word(ram,a) | (ram[a+2]<<16); }
static int signed_word(unsigned v) { return (int16_t)v; }

static int decode_entity(const uint8_t *ram, unsigned entity, unsigned frame,
                         int projected, LagoonWideCell *cells) {
  unsigned a = 0x500 + entity, state = ram[a + 0x1b], ox, oy, type = ram[a + 0x18];
  if ((type != 1 && type != 2) || state > 0x3f || (state < 2 && type == 1) ||
      (type == 2 && ((ram[a + 1] & 1) || state >= 0x25))) return -1;
  unsigned offsets = type == 1 ? 0x1fb7f : 0x1f7e9;
  if (!rom_word(g_rom, kRomSize, offsets + state * 4, &ox) ||
      !rom_word(g_rom, kRomSize, offsets + state * 4 + 2, &oy)) return -1;
  int x = projected ? signed_word(ram_word(ram,a+2)-ram_word(ram,0x480)) : signed_word(ram_word(ram,a+12));
  int y = projected ? signed_word(ram_word(ram,a+4)-ram_word(ram,0x482)) : signed_word(ram_word(ram,a+14));
  int count = LagoonWideEntities_Decode(g_rom,kRomSize,ram[0x48e]+1,frame,
                                      x+signed_word(ox),y+signed_word(oy),cells,kCells);
  if (type == 2 && ((ram[a + 1] & 0x80) || (ram[a + 0x1f] & 2)))
    for (int i=0;i<count;++i) cells[i].tile=(cells[i].tile&~0xe00)|0xe00;
  return count;
}
static int find_cell(const uint8_t *oam,const LagoonWideCell *c,const uint8_t *used) {
  for (int slot=0;slot<128;++slot) {
    unsigned a=slot*4,h=(oam[512+slot/4]>>((slot&3)*2))&3;
    if (!used[slot] && (oam[a]|((h&1)<<8))==(unsigned)(c->x&511) &&
        oam[a+1]==(c->y&255) && ram_word(oam,a+2)==c->tile && ((h&2)?16:8)==c->size) return slot;
  }
  return -1;
}
static void observe_vram_byte(uint32_t adr,uint8_t value) {
  (void)value;
  LagoonWide_OnVramWrite(adr);
  if (s_atlas.generation!=RtlStateGeneration()) LagoonWideAtlas_Reset(&s_atlas);
  int was_owned=adr>=0x8800 && adr<0xb200 && s_atlas.owned[adr-0x8800];
  int expected=s_atlas.transfer_left && adr==s_atlas.transfer_next;
  LagoonWideAtlas_Write(&s_atlas,adr);
  /* An unrecognized same-frame overwrite must not leave cached far pixels. */
  if (was_owned && !expected && s_has_far && g_ppu) {
    s_object_count=0; s_has_far=0; PpuSetExtraObjects(g_ppu,NULL,0);
  }
}
static void observe_dma(const Dma *dma,unsigned channel,const DmaChannel *t) {
  if (!g_config.widescreen || !g_rom || !g_ppu) return;
  if (!t->fromB && (t->bAdr==0x18 || t->bAdr==0x19)) {
    unsigned channels=0;
    for (unsigned i=0;i<8;++i) channels+=dma->channel[i].dmaActive!=0;
    if (channels==1 && t->mode==1 && t->bAdr==0x18 && !t->fixed && !t->decrement &&
        g_ppu->vramIncrement==1 && g_ppu->vramIncrementOnHigh && !g_ppu->vramRemapMode) {
      LagoonWide_OnBackgroundDma(t->aBank,t->aAdr,g_ppu->vramPointer,t->size);
      LagoonWideAtlas_Begin(&s_atlas,RtlStateGeneration(),g_ram[0x48e],
          ram_pointer(g_ram,0x5d),t->aBank,t->aAdr,g_ppu->vramPointer,t->size);
    } else s_atlas.transfer_left=0;
  }
  if (channel!=0 || t->fromB || t->mode!=0 || t->fixed || t->decrement ||
      t->bAdr!=4 || t->aBank!=0x7e || t->aAdr!=0x2000 || t->size!=0x220) return;
  memcpy(s_frame.ram,g_ram,sizeof(s_frame.ram));
  memcpy(s_frame.oam,g_ram+0x2000,sizeof(s_frame.oam));
  s_frame.generation=RtlStateGeneration(); s_frame.serial=++s_serial; s_frame.valid=1; s_captures++;
}
void LagoonWideEntities_Init(void) {
  dma_set_start_observer(observe_dma); ppu_set_vram_write_log_hook(observe_vram_byte);
}
void LagoonWideEntities_Reset(void) {
  s_frame.valid=0; s_last_serial=0; s_object_count=0; s_has_far=0; s_pose_room_key=0; s_pose_generation=0;
  memset(s_pose,0,sizeof(s_pose)); memset(s_kind_witness,0,sizeof(s_kind_witness));
  LagoonWideAtlas_Reset(&s_atlas);
  if (g_ppu) PpuSetExtraObjects(g_ppu,NULL,0);
}
/* region0=all margin,1=seam strips,2=deep margin. Native opacity is always zero. */
static int draw_cell(const LagoonWideCell *c,unsigned anchor,uint32_t order,int region) {
  if (s_object_count>=kObjects || c->size>16 || c->y<=-(int)c->size || c->y>=224 ||
      c->x>=256+g_ppu->extraRightCur || c->x+c->size<=-g_ppu->extraLeftCur ||
      (c->x>=0 && c->x+c->size<=256)) return 0;
  uint16_t *pixels=s_pixels[s_object_count];
  if (!LagoonWideEntities_Pixels(PpuRenderVram(g_ppu),g_ppu->cgram,
      PPU_objTileAdr1(g_ppu),PPU_objTileAdr2(g_ppu),c,pixels,1)) return 0;
  int opaque=0;
  for (unsigned y=0;y<c->size;++y) for (unsigned x=0;x<c->size;++x) {
    int sx=c->x+(int)x,deep=sx < -16 || sx>=272;
    if ((region==1 && deep)||(region==2 && !deep)) pixels[y*c->size+x]=0;
    opaque|=pixels[y*c->size+x]!=0;
  }
  if (!opaque) return 0;
  s_objects[s_object_count++]=(PpuExtraObject){c->x,c->y,c->size,c->size,c->size,
      (c->tile>>12)&3,!!(c->tile&0x800),(uint8_t)anchor,order,pixels};
  return 1;
}
static int cells_owned(const LagoonWideCell *c,int count) {
  for(int i=0;i<count;++i) if(!LagoonWideAtlas_CellOwned(&s_atlas,
      PPU_objTileAdr1(g_ppu),PPU_objTileAdr2(g_ppu),&c[i])) return 0;
  return count>0;
}
static void update_pose(const uint8_t *ram,unsigned e,int queued) {
  const uint8_t *r=ram+0x500+e*64; WidePoseHistory *h=&s_pose[e];
  unsigned x=ram_word(r,2),y=ram_word(r,4);
  if ((r[0]&0xc0)!=0xc0) {h->valid=0;return;}
  int fresh=!h->valid || h->type!=r[0x18] || h->kind!=r[0x1b] ||
      abs(signed_word(x-h->x))>128 || abs(signed_word(y-h->y))>128;
  if (queued || fresh) h->animation=(LagoonWideAnimation){r[0x10],r[0x11]};
  if (h->serial!=s_frame.serial && !queued && !fresh) {
    unsigned steps=(uint8_t)(ram[0x408]-h->clock); uint8_t ignored;
    if (steps<=8) {
      for(unsigned i=0;i<steps;++i)
        if(!LagoonWideEntities_CommonFrame(g_rom,kRomSize,r,&h->animation,1,&ignored)) {h->valid=0;return;}
    } else h->animation=(LagoonWideAnimation){r[0x10],r[0x11]};
  }
  h->valid=1;h->type=r[0x18];h->kind=r[0x1b];h->clock=ram[0x408];h->serial=s_frame.serial;
  h->x=x;h->y=y;
}

void LagoonWideEntities_BeginFrame(int enabled) {
  s_object_count=0;s_has_far=0;PpuSetExtraObjects(g_ppu,NULL,0);
  if(!enabled || !s_frame.valid) {report_status("inactive-or-no-capture",0,0);return;}
  if(s_frame.generation!=RtlStateGeneration() || g_ppu->obsel!=2 || PPU_objPriority(g_ppu)) {
    report_status("unsupported-timeline-or-obj-layout",0,0);return;
  }
  if(memcmp(s_frame.oam,g_ppu->oam,512)||memcmp(s_frame.oam+512,g_ppu->highOam,32)) {
    report_status("oam-fingerprint-mismatch",0,0);return;
  }
  const uint8_t *ram=s_frame.ram;
  if(ram[0x48e]!=g_ram[0x48e] || ram[0x48c]>64) return;
  unsigned roomkey=ram_pointer(ram,0x5d) ^ ((unsigned)ram[0x48e]<<24);
  if(roomkey!=s_pose_room_key || s_pose_generation!=s_frame.generation) {
    memset(s_pose,0,sizeof(s_pose));memset(s_kind_witness,0,sizeof(s_kind_witness));s_pose_room_key=roomkey;s_pose_generation=s_frame.generation;
  }
  int atlas_ok=s_atlas.ready && s_atlas.generation==s_frame.generation &&
      s_atlas.room==ram[0x48e] && s_atlas.source==ram_pointer(ram,0x5d);
  uint8_t used[128]={0},queued[kEntities]={0},far[kEntities]={0};
  int cell_slot[kEntities][kCells],counts[kEntities]={0};
  LagoonWideCell cells[kEntities][kCells];
  memset(cell_slot,-1,sizeof(cell_slot));
  for(unsigned q=0;q<ram[0x48c];++q) {
    unsigned offset=ram_word(ram,0xec0+q*6),type=ram[0xec4+q*6];
    if(offset>0x800 || (offset&63)) continue;
    unsigned e=offset/64,a=0x500+offset;queued[e]=1;
    if(type!=1 && type!=2) continue;
    int n=decode_entity(ram,offset,ram[a+0x1c],0,cells[e]);counts[e]=n;
    if(n<0) continue;
    for(int i=0;i<n;++i) {
      if(cells[e][i].y<-(int)cells[e][i].size || cells[e][i].y>=240) continue;
      int slot=find_cell(s_frame.oam,&cells[e][i],used);
      if(slot<0) {s_rejected++;report_status("native-cell-mismatch",offset,i);return;}
      used[slot]=1;cell_slot[e][i]=slot;
    }
    LagoonWideAnimation astate={ram[a+0x10],ram[a+0x11]};uint8_t expected;
    if(LagoonWideEntities_CommonFrame(g_rom,kRomSize,ram+a,&astate,0,&expected)) {
      if(expected==ram[a+0x1c] && atlas_ok && cells_owned(cells[e],n)) {
        if(s_last_serial!=s_frame.serial) s_frame_witness++;
        s_kind_witness[type-1][ram[a+0x1b]]=1;
      } else if(expected!=ram[a+0x1c]) {
        s_kind_witness[type-1][ram[a+0x1b]]=0;
        if(s_last_serial!=s_frame.serial) {
          if(s_frame_bad<8 && getenv("LAGOON_WIDE_ENTITY_REPORT"))
            fprintf(stderr,"wide-far: queued frame mismatch entity=%03X type=%u kind=%u actual=%u expected=%u timer=%u phase=%u flags=%02X status=%02X motion=%02X direction=%u\n",
                offset,type,ram[a+0x1b],ram[a+0x1c],expected,ram[a+0x10],ram[a+0x11],ram[a],ram[a+1],ram[a+11],ram[a+10]);
          s_frame_bad++;
        }
      }
    }
  }
  for(unsigned e=1;e<kEntities;++e) update_pose(ram,e,queued[e]);
  int any_far=0;
  for(unsigned e=1;e<kEntities;++e) {
    unsigned a=0x500+e*64,type=ram[a+0x18],kind=ram[a+0x1b];
    if(queued[e] || !atlas_ok || !s_pose[e].valid || type<1 || type>2 || kind>=64 ||
        !s_kind_witness[type-1][kind]) continue;
    int x=signed_word(ram_word(ram,a+2)-ram_word(ram,0x480));
    int y=signed_word(ram_word(ram,a+4)-ram_word(ram,0x482));
    if((x>=-96 && x<352) || y < -32 || y>=288) continue;
    uint8_t frame;
    if(!LagoonWideEntities_CommonFrame(g_rom,kRomSize,ram+a,&s_pose[e].animation,0,&frame)) continue;
    int n=decode_entity(ram,e*64,frame,1,cells[e]);
    if(n<=0 || !cells_owned(cells[e],n)) {
      if(s_far_gated<8 && getenv("LAGOON_WIDE_ENTITY_REPORT"))
        fprintf(stderr,"wide-far: unowned entity=%03X kind=%u frame=%u cells=%d x=%d\n",e*64,kind,frame,n,x);
      s_far_gated++;continue;
    }
    int deep=1,intersects=0;
    for(int i=0;i<n;++i) {
      LagoonWideCell *c=&cells[e][i];
      if(c->x+c->size > -16 && c->x<272) deep=0;
      if(c->x<256+g_ppu->extraRightCur && c->x+c->size> -g_ppu->extraLeftCur &&
          c->y<224 && c->y+c->size>0) intersects=1;
    }
    if(!deep || !intersects) continue;
    counts[e]=n;far[e]=1;any_far=1;s_last_far_entity=e*64;s_last_far_x=x;s_last_far_frame=frame;
  }
  /* An unsupported native group in the same deep margins needs its own
   * composition/order contract. Reject new far rendering, retain old path. */
  if(any_far) for(unsigned slot=0;slot<ram[0x236] && slot<128;++slot) {
    if(used[slot]) continue;
    unsigned hi=(s_frame.oam[512+slot/4]>>((slot&3)*2))&3;
    int x=s_frame.oam[slot*4]|((hi&1)<<8),y=s_frame.oam[slot*4+1],sz=(hi&2)?16:8;
    if(x>=384)x-=512;if(y>=240)y-=256;
    if((x< -16 || x+sz>272) && y<224 && y+sz>0) {
      if(s_far_gated<8 && getenv("LAGOON_WIDE_ENTITY_REPORT"))
        fprintf(stderr,"wide-far: unsupported deep native slot=%u xy=%d,%d tile=%04X\n",slot,x,y,ram_word(s_frame.oam,slot*4+2));
      any_far=0;s_far_gated++;break;
    }
  }
  /* Never emit a partial far actor because a bounded host buffer filled. */
  if(any_far) {
    unsigned needed=0;
    for(unsigned e=1;e<kEntities;++e) if(counts[e]>0 && (queued[e]||far[e]))
      needed+=(unsigned)counts[e]*(queued[e]?2:1);
    if(needed>kObjects){any_far=0;s_far_gated++;}
  }
  unsigned rank[kEntities]={0};
  if(any_far) for(unsigned e=1;e<kEntities;++e) if((queued[e]||far[e]) && counts[e]>0) {
    unsigned ey=ram_word(ram,0x504+e*64)-ram_word(ram,0x482);ey&=65535;
    unsigned r=0;
    for(unsigned o=1;o<kEntities;++o) if((queued[o]||far[o]) && counts[o]>0) {
      unsigned oy=(ram_word(ram,0x504+o*64)-ram_word(ram,0x482))&65535;
      if(oy>ey || (oy==ey && o<e))r++;
    }
    rank[e]=r;
  }
  FILE *cell_report=NULL;
  const char *report=getenv("LAGOON_WIDE_ENTITY_REPORT");
  if(any_far && report && s_last_serial!=s_frame.serial) {
    char path[4096];
    if(snprintf(path,sizeof(path),"%s.cells.tsv",report)<(int)sizeof(path)) {
      cell_report=fopen(path,"a");
      if(cell_report && ftell(cell_report)==0)
        fprintf(cell_report,"host_frame\tserial\tclock\tentity\ttype\tkind\tframe\tx\ty\ttile\tsize\n");
    }
  }
  for(unsigned e=1;e<kEntities;++e) if(counts[e]>0 && (queued[e] || (any_far&&far[e]))) {
    for(int i=0;i<counts[e];++i) {
      if(queued[e] && cell_slot[e][i]<0) continue;
      if(!any_far) s_emitted+=draw_cell(&cells[e][i],cell_slot[e][i],1,0);
      else {
        if(queued[e])s_emitted+=draw_cell(&cells[e][i],cell_slot[e][i],1,1);
        int added=draw_cell(&cells[e][i],0,rank[e]*kCells+i+1,2);
        s_emitted+=added;
        if(far[e]) {
          s_far_emitted+=added;s_has_far|=added;
          if(added && cell_report) {
            unsigned a=0x500+e*64;uint8_t frame=0;
            LagoonWideAnimation copy=s_pose[e].animation;
            LagoonWideEntities_CommonFrame(g_rom,kRomSize,ram+a,&copy,0,&frame);
            fprintf(cell_report,"%d\t%u\t%u\t%03X\t%u\t%u\t%u\t%d\t%d\t%04X\t%u\n",
                snes_frame_counter,s_frame.serial,ram[0x408],e*64,ram[a+0x18],ram[a+0x1b],frame,
                cells[e][i].x,cells[e][i].y,cells[e][i].tile,cells[e][i].size);
          }
        }
      }
    }
  }
  if(cell_report)fclose(cell_report);
  if(s_last_serial!=s_frame.serial){s_validated++;s_last_serial=s_frame.serial;}
  PpuSetExtraObjects(g_ppu,s_objects,s_object_count);report_status("ready",0,0);
}
#endif /* runtime */
