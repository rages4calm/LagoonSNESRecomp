/* Read-only ordinary NPC and nondeath enemy sprite presentation. Capture at the real OAM
 * DMA boundary; never widen guest culling, run guest drawing again, or alter
 * the native viewport. Unsupported streams and ownership fail closed. */
#include "lagoon_wide_entities.h"
#ifndef LAGOON_WIDE_ENTITIES_DECODER_ONLY
#include "common_rtl.h"
#include "config.h"
#include "lagoon_widescreen.h"
#include "lagoon_scene_renderer.h"
#include "snes/dma.h"
#include "snes/ppu.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { kEntities = 33, kCells = 64, kObjects = 512, kRomSize = 0x100000 };
#ifndef LAGOON_WIDE_ENTITIES_DECODER_ONLY
typedef struct WideEntitySnapshot {
  uint8_t ram[0x8801], oam[544];
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
typedef struct WideNativePose {
  uint8_t record[64];
  uint16_t atlas[0x1500],palette[256];
  uint64_t generation;
  unsigned room,source,width,height,dp90,valid;
} WideNativePose;
static WideNativePose s_native_pose[kEntities];
static WidePoseHistory s_pose[kEntities];
static uint8_t s_kind_witness[2][128];
static unsigned s_pose_room_key, s_last_far_entity, s_last_far_frame;
static uint64_t s_pose_generation;
static int s_last_far_x;
static uint64_t s_far_emitted, s_frame_witness, s_frame_bad, s_far_gated, s_current_emitted;
static int s_has_far;

static PpuExtraObject s_objects[kObjects];
static uint16_t s_pixels[kObjects][256];
static PpuExtraObject s_retained_draw[kObjects];
static uint16_t s_retained_draw_pixels[kObjects][256];
static size_t s_object_count;
static struct {
  PpuExtraObject objects[kObjects];
  uint16_t pixels[kObjects][256],atlas[0x1500],palette[256];
  uint8_t records[0x800],oam[544];
  uint64_t generation;
  size_t count;
  int valid,native_anchors;
  uint8_t obsel,oam_priority;
} s_retained_objects;
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
  fprintf(f, "status=%s serial=%u captures=%llu validated=%llu rejected=%llu emitted=%llu live_objects=%zu left=%u right=%u entity=%u cell=%u atlas_ready=%u atlas_bytes=%u frame_witness=%llu frame_bad=%llu far_cells=%llu far_gated=%llu last_far=%03X far_x=%d far_frame=%u current_cells=%llu\n",
      status, s_frame.serial, (unsigned long long)s_captures, (unsigned long long)s_validated,
      (unsigned long long)s_rejected, (unsigned long long)s_emitted, s_object_count, left, right, entity, cell, s_atlas.ready, s_atlas.completed,
      (unsigned long long)s_frame_witness, (unsigned long long)s_frame_bad,
      (unsigned long long)s_far_emitted, (unsigned long long)s_far_gated,
      s_last_far_entity, s_last_far_x, s_last_far_frame, (unsigned long long)s_current_emitted);
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

unsigned LagoonWideRetainedMap_Tile(const LagoonWideRetainedMap *map,int layer,int x,int y) {
  if(!map || layer<0 || layer>1)return 0;
  unsigned meta=x<0 || y<0 || x>=2048 || y>=2048 ? map->fill :
      map->maps[layer][(y>>4)*128+(x>>4)];
  unsigned tile=map->definitions[layer][meta*4+((x&8)>>3)+((y&8)>>2)];
  return layer ? tile&0xDFFF : tile;
}

int LagoonWideEntities_Decode(const uint8_t *rom, size_t size,
    unsigned selector, unsigned frame, int x, int y,
    LagoonWideCell *cells, size_t capacity) {
  unsigned lo, hi, bank, table, ptr, dims, flags;
  if (!rom || !cells || selector > 0x4b || frame > 255 ||
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

/* Room 01 uses the ordinary map producer, with its authored finite extent. */
int LagoonWideEntities_BoundedRoom(unsigned room,unsigned width,unsigned height) {
  return (room==1 && width==512 && height==768) ||
      (room==2 && width==512 && height==512) ||
      (room==3 && width==1792 && height==1632) ||
      (room==4 && width==768 && height==256) ||
      (room==5 && width==1536 && height==1280) ||
      (room==6 && width==512 && height==256) ||
      (room==7 && width==1024 && height==1024) ||
      (room==8 && width==1536 && height==1024);
}

int LagoonWideEntities_AuditedRoom(unsigned room,unsigned bg,unsigned obj,
    unsigned d0,unsigned d1,unsigned m0,unsigned m1) {
  if(m0!=0x7f0000 || m1!=0x7f4000 || d0!=d1)return 0;
  return (room==0 && bg==0x138000 && obj==0x198000 && d0==0x068000) ||
         (room>=3 && room<=6 && bg==0x128b77 && obj==0x1d8000 && d0==0x069298) ||
         (room==1 && bg==0x139a7a && obj==0x1a8000 && d0==0x0689e0) ||
         (room==2 && bg==0x128000 && obj==0x1a8000 && d0==0x068e90) ||
         (room==7 && bg==0x129e89 && obj==0x1a906f && d0==0x069720) ||
         (room==8 && bg==0x12b297 && obj==0x1891e4 && d0==0x06999c);
}

int LagoonWideEntities_WorldCharacters(const uint16_t *vram,
    const uint16_t *witness,const uint8_t used[1024]) {
  if(!vram || !witness || !used)return 0;
  for(unsigned tile=0;tile<1024;++tile)
    if(used[tile] && memcmp(vram+tile*16,witness+tile*16,32))return 0;
  return 1;
}

int LagoonWideEntities_ParallaxScroll(unsigned x0,unsigned y0,unsigned x1,unsigned y1) {
  return x0<1024 && y0<1024 && x1==(x0>>2) && y1==(y0>>2);
}
int LagoonWideEntities_ParallaxRoomScroll(unsigned room,unsigned x0,unsigned y0,unsigned x1,unsigned y1) {
  if(room==7)return LagoonWideEntities_ParallaxScroll(x0,y0,x1,y1);
  if(room!=0x0c&&room!=0x12&&room!=0x13&&room!=0x28&&room!=0x29&&room!=0x2a)return 0;
  return x0<2048&&y0<2048&&x1==(x0>>2)&&y1==(y0>>2);
}
int LagoonWideEntities_ParallaxRaster(const unsigned camera[4],const unsigned raster[4]) {
  if(!camera||!raster||camera[0]>=2048||camera[1]>=2048)return 0;
  for(unsigned axis=0;axis<2;++axis) {
    int full=(int)camera[axis]+(((int)(raster[axis]&1023)-(int)camera[axis]+512)&1023)-512;
    if(full<0||full>=2048||raster[axis+2]!=((unsigned)full>>2))return 0;
  }
  return 1;
}
typedef struct ParallaxPageContract {
  unsigned source,length;
  uint64_t compressed,decoded;
} ParallaxPageContract;
static const ParallaxPageContract s_parallax_pages[]={
  {0x11fb7d,761,UINT64_C(0x6497f7a95d47a572),UINT64_C(0xad60d4b1913c658c)},
  {0x14fb7f,612,UINT64_C(0x3129bd7523470bea),UINT64_C(0x472693e1dbc90dc7)},
  {0x169cf6,594,UINT64_C(0x85603bb271291317),UINT64_C(0x3c45c81e7d5a4b2a)},
  {0x13f633,998,UINT64_C(0x1dff563a63ecbac2),UINT64_C(0x7773ded907f39543)},
  {0x13fa19,875,UINT64_C(0xa13473f55bb85060),UINT64_C(0x0041c62aa52fee4e)},
  {0x14f82e,849,UINT64_C(0x2a93aa0e0da14cf7),UINT64_C(0xcd0f5847cb1a5f72)},
  {0x12fe89,188,UINT64_C(0xf50c5c36dd14e62e),UINT64_C(0xac453d764e7f11cf)},
  {0x13fd84,522,UINT64_C(0x4d59f1f674fb8cd0),UINT64_C(0x1f49b2e687614fe4)},
  {0x11fe76,167,UINT64_C(0xa27c64e02b0a643e),UINT64_C(0xa68c107862a10fa5)}
};
int LagoonWideEntities_ParallaxMapForRoom(const uint16_t *vram,const uint8_t *rom,size_t size,unsigned room) {
  unsigned indices[2];
  switch(room) {
    case 7:indices[0]=indices[1]=0;break;
    case 0x0c:indices[0]=1;indices[1]=2;break;
    case 0x12:case 0x13:indices[0]=3;indices[1]=4;break;
    case 0x28:case 0x29:indices[0]=5;indices[1]=6;break;
    case 0x2a:indices[0]=7;indices[1]=8;break;
    default:return 0;
  }
  if(!vram||!rom||size!=0x100000)return 0;
  for(unsigned page=0;page<2;++page) {
    const ParallaxPageContract *c=&s_parallax_pages[indices[page]];
    unsigned source=((c->source>>16)&127)*32768+(c->source&32767);
    if(source>size||c->length>size-source)return 0;
    uint64_t h=UINT64_C(14695981039346656037);
    for(unsigned i=0;i<c->length;++i)h=(h^rom[source+i])*UINT64_C(1099511628211);
    if(h!=c->compressed)return 0;
    h=UINT64_C(14695981039346656037);
    for(unsigned i=0;i<1024;++i) {
      uint16_t value=vram[0x7800+page*0x400+i];
      h=(h^(value&255))*UINT64_C(1099511628211);
      h=(h^(value>>8))*UINT64_C(1099511628211);
    }
    if(h!=c->decoded)return 0;
  }
  return 1;
}
int LagoonWideEntities_ParallaxMap(const uint16_t *vram,const uint8_t *rom,size_t size) {
  return LagoonWideEntities_ParallaxMapForRoom(vram,rom,size,7);
}

int LagoonWideParallaxPacket_CaptureRoom(LagoonWideParallaxPacket *packet,
    const uint16_t *vram,const uint8_t *rom,size_t size,uint64_t generation,
    const unsigned camera[4],unsigned room) {
  if(!packet)return 0;
  packet->valid=0;
  if(!camera || !LagoonWideEntities_ParallaxRoomScroll(room,camera[0],camera[1],camera[2],camera[3]) ||
      !LagoonWideEntities_ParallaxMapForRoom(vram,rom,size,room))return 0;
  memcpy(packet->pages,vram+0x7800,sizeof(packet->pages));
  memcpy(packet->camera,camera,sizeof(packet->camera));
  packet->generation=generation;packet->room=room;packet->valid=1;return 1;
}
int LagoonWideParallaxPacket_Capture(LagoonWideParallaxPacket *packet,
    const uint16_t *vram,const uint8_t *rom,size_t size,uint64_t generation,const unsigned camera[4]) {
  return LagoonWideParallaxPacket_CaptureRoom(packet,vram,rom,size,generation,camera,7);
}
int LagoonWideParallaxPacket_Match(const LagoonWideParallaxPacket *packet,
    const uint16_t *vram,const uint8_t *rom,size_t size,uint64_t generation,
    const unsigned camera[4],const unsigned raster[4]) {
  return packet && packet->valid && packet->generation==generation && camera && raster &&
      !memcmp(packet->camera,camera,sizeof(packet->camera)) &&
      LagoonWideEntities_ParallaxRoomScroll(packet->room,packet->camera[0],packet->camera[1],packet->camera[2],packet->camera[3]) &&
      LagoonWideEntities_ParallaxRaster(camera,raster) &&
      LagoonWideEntities_ParallaxMapForRoom(vram,rom,size,packet->room) &&
      !memcmp(packet->pages,vram+0x7800,sizeof(packet->pages));
}

int LagoonWideParallaxQueue_Pending(const LagoonWideParallaxQueue *queue) {
  return queue && queue->valid && queue->next<68;
}
int LagoonWideParallaxQueue_Begin(LagoonWideParallaxQueue *queue,
    const LagoonWideRetainedMap *map,const uint8_t *source,size_t size,
    const unsigned camera[4],uint64_t generation) {
  if(!queue)return 0;
  int pending=LagoonWideParallaxQueue_Pending(queue);queue->valid=0;
  if(pending || !map || !source || size!=136 || !camera ||
      !LagoonWideEntities_ParallaxScroll(camera[0],camera[1],camera[2],camera[3]))return 0;
  int left=(int)(camera[0]/8)-1,top=(int)(camera[1]/8);
  for(unsigned n=0;n<34;++n) {
    int x=left+(int)n;
    unsigned address=0x7000+((top&31)*32)+(x&31)+((x&32)?0x400:0);
    unsigned actual=source[n*4]|source[n*4+1]<<8;
    unsigned tile=source[n*4+2]|source[n*4+3]<<8;
    if(actual!=address || tile!=LagoonWideRetainedMap_Tile(map,0,x*8,top*8))return 0;
    queue->address[n]=(uint16_t)address;queue->tile[n]=(uint16_t)tile;
  }
  queue->generation=generation;queue->next=0;queue->valid=1;return 1;
}
int LagoonWideParallaxQueue_Write(LagoonWideParallaxQueue *queue,
    unsigned address,unsigned value,uint64_t generation) {
  if(!queue)return 0;
  if(!LagoonWideParallaxQueue_Pending(queue) || queue->generation!=generation ||
      address!=(unsigned)queue->address[queue->next/2]*2+(queue->next&1) ||
      value!=((queue->tile[queue->next/2]>>((queue->next&1)*8))&255)) {
    queue->valid=0;return 0;
  }
  ++queue->next;return 1;
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

/* Exact original descriptor sequences. A frame's upload length need not be
 * its source stride (E010), and descriptor entries may ping-pong (E030).
 * Eight unique source identities bound storage; no arithmetic aliases. */
typedef struct AnimationContract {
  unsigned descriptor,table,bytes,entries,frames,delay,sources[12];
} AnimationContract;
static const AnimationContract s_animation_contracts[]={
  {0x01dfe0,0x01d9e3,384,2,2,4,{0x108000,0x108180}},
  {0x01dfe8,0x01da42,384,12,4,4,{0x108300,0x108480,0x108600,0x108780,0x108300,0x108480,0x108600,0x108780,0x108300,0x108480,0x108600,0x108780}},
  {0x01dff0,0x01da6b,192,3,3,4,{0x108900,0x1089c0,0x108a80}},
  {0x01dff8,0x01da79,192,2,2,4,{0x108b40,0x108c00}},
  {0x01e000,0x01db1d,128,4,4,4,{0x108cc0,0x108d40,0x108dc0,0x108e40}},
  {0x01e008,0x01db2e,512,3,3,7,{0x108ec0,0x1090c0,0x1092c0}},
  {0x01e010,0x01dc1b,160,6,6,4,{0x1094c0,0x109540,0x1095c0,0x109640,0x1096c0,0x109740}},
  {0x01e018,0x01dcb1,480,2,2,4,{0x1097c0,0x1099a0}},
  {0x01e020,0x01dce1,416,8,8,4,{0x109b80,0x109d20,0x109ec0,0x10a060,0x10a200,0x10a3a0,0x10a540,0x10a6e0}},
  {0x01e028,0x01dcfe,128,3,3,4,{0x10a880,0x10a900,0x10a980}},
  {0x01e030,0x01dd27,384,6,4,4,{0x10aa00,0x10ab80,0x10ad00,0x10ae80,0x10ad00,0x10ab80}},
  {0x01e038,0x01dd9c,192,2,2,4,{0x10b000,0x10b0c0}},
  {0x01e040,0x01dd6f,384,4,4,4,{0x10b180,0x10b300,0x10b480,0x10b600}},
  {0x01e048,0x01df82,128,2,2,4,{0x10b880,0x10b900}},
  {0x01e050,0x01dd80,192,2,2,4,{0x10b980,0x10ba40}},
  {0x01e058,0x01dd8b,64,4,4,4,{0x10bb00,0x10bb40,0x10bb80,0x10bbc0}},
  {0x01e060,0x01dda7,128,2,2,4,{0x10bc00,0x10bc80}}
};
static const struct { unsigned room,source,contract; } s_animation_rooms[]={
  {0x01,0x139a7a,0},
  {0x02,0x128000,1},
  {0x03,0x128b77,2},
  {0x04,0x128b77,2},
  {0x05,0x128b77,2},
  {0x06,0x128b77,2},
  {0x08,0x12b297,3},
  {0x19,0x13d745,7},
  {0x1e,0x14c061,8},
  {0x1f,0x14c061,8},
  {0x20,0x14c061,8},
  {0x21,0x14c061,8},
  {0x22,0x14d33d,9},
  {0x32,0x15b8ac,14},
  {0x33,0x15b8ac,14},
  {0x34,0x15b8ac,14},
  {0x35,0x15c815,15},
  {0x36,0x15c815,15},
  {0x37,0x15c815,15},
  {0x09,0x12b297,3},
  {0x0b,0x12da6c,4},
  {0x0d,0x12da6c,4},
  {0x11,0x168000,5},
  {0x14,0x13c716,6},
  {0x15,0x13c716,6},
  {0x26,0x14de0a,10},
  {0x2a,0x14edd5,12},
  {0x38,0x15d7e5,11},
  {0x39,0x15d7e5,11},
  {0x3a,0x15d7e5,11},
  {0x3e,0x15e7fb,16},
  {0x49,0x15a43d,13}
};
static const AnimationContract *animation_contract(unsigned room,unsigned source) {
  for(unsigned i=0;i<sizeof(s_animation_rooms)/sizeof(s_animation_rooms[0]);++i)
    if(s_animation_rooms[i].room==room && s_animation_rooms[i].source==source)
      return &s_animation_contracts[s_animation_rooms[i].contract];
  return NULL;
}
static unsigned animation_source(const AnimationContract *c,unsigned frame) {
  unsigned unique=0;
  for(unsigned i=0;i<c->entries;++i) {
    int first=1;
    for(unsigned j=0;j<i;++j)if(c->sources[i]==c->sources[j]){first=0;break;}
    if(first && unique++==frame)return c->sources[i];
  }
  return 0;
}
unsigned LagoonWideTorches_Bytes(unsigned room,unsigned source) {
  const AnimationContract *c=animation_contract(room,source);return c?c->bytes:0;
}
void LagoonWideTorches_Reset(LagoonWideTorches *t) {if(t)memset(t,0,sizeof(*t));}
int LagoonWideTorches_Begin(LagoonWideTorches *t,uint64_t generation,
    unsigned room,unsigned source,unsigned bank,unsigned address,unsigned destination,unsigned length) {
  if(!t)return 0;
  if(t->generation!=generation || t->room!=room || t->source!=source) {
    LagoonWideTorches_Reset(t);t->generation=generation;t->room=room;t->source=source;
  }
  t->left=0;const AnimationContract *c=animation_contract(room,source);
  if(!c || c->frames>8 || c->bytes>kLagoonWideTorchMaxBytes || destination!=0x3b00 || length!=c->bytes)return 0;
  unsigned frame=0;
  for(;frame<c->frames;++frame)if(animation_source(c,frame)==((bank<<16)|address))break;
  if(frame==c->frames)return 0;
  t->frame=frame;t->next=kLagoonWideTorchAddress;t->left=c->bytes;return 1;
}
void LagoonWideTorches_Write(LagoonWideTorches *t,unsigned address,uint8_t value,
    const uint8_t *rom,size_t size) {
  if(!t)return;
  const AnimationContract *c=animation_contract(t->room,t->source);
  unsigned bytes=c?c->bytes:0;
  if(!t->left || address!=t->next) {
    if(address>=kLagoonWideTorchAddress && address<kLagoonWideTorchAddress+bytes)t->seen=0;
    t->left=0;return;
  }
  unsigned at=address-kLagoonWideTorchAddress;
  t->pending[at]=value;t->next++;t->left--;
  if(t->left)return;
  unsigned v;
  if(!c || t->frame>=c->frames || !rom_word(rom,size,c->descriptor,&v) || v!=((c->entries<<8)|c->delay) ||
      !rom_word(rom,size,c->descriptor+2,&v) || v!=(c->table&65535) ||
      !rom_word(rom,size,c->descriptor+4,&v) || v!=0x3b00 ||
      !rom_word(rom,size,c->descriptor+6,&v) || v!=bytes){t->seen=0;return;}
  for(unsigned i=0;i<c->entries;++i)
    if(!rom_word(rom,size,c->table+i*3,&v) || v!=(c->sources[i]&65535) ||
        !rom_byte(rom,size,c->table+i*3+2,&v) || v!=(c->sources[i]>>16)){t->seen=0;return;}
  unsigned source=animation_source(c,t->frame);
  for(unsigned i=0;i<bytes;++i)
    if(!rom_byte(rom,size,source+i,&v) || v!=t->pending[i]){t->seen=0;return;}
  memcpy(t->bytes[t->frame],t->pending,bytes);t->seen|=1u<<t->frame;
}
int LagoonWideTorches_Match(const LagoonWideTorches *t,const uint16_t *vram,
    uint64_t generation,unsigned room,unsigned source) {
  if(!t || !vram || t->left || t->generation!=generation || t->room!=room || t->source!=source)return -1;
  const AnimationContract *c=animation_contract(room,source);
  if(!c)return -1;
  for(unsigned frame=0;frame<c->frames;++frame)
    if((t->seen&(1u<<frame)) && !memcmp((const uint8_t*)vram+kLagoonWideTorchAddress,t->bytes[frame],c->bytes))return (int)frame;
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

static int ordinary_frame(const uint8_t *rom, size_t size,
    const uint8_t *r, LagoonWideAnimation *a, int step, uint8_t *frame,int additional) {
  if (!rom || !r || !a || !frame || r[10] > 3 || a->phase > ((additional&&r[24]==2&&r[27]==12)?6:3)) return 0;
  unsigned type = r[0x18], kind = r[0x1b], table, delay, value;
  /* First proven ordinary forms. Special/status/death paths may mutate AI,
   * rewards or global state and are deliberately not presentation code. */
  if (type == 1 && ((kind == 41 || kind == 59) || (additional && kind<128))) {
    if (!rom_word(rom, size, 0x1f885 + kind * 2, &table)) return 0;
    table |= 0x10000;
  } else if (type == 2 && (kind <= 1 || additional) && !(r[1] & 7)) {
    table = 0x1f41a + kind * 17;
  } else return 0;
  if (!rom_byte(rom, size, table, &delay) || !delay) return 0;
  int idle = !(r[0] & 0x20) && ((r[0] & 1) || (r[11] & 0x80));
  if (additional && type==2 && kind==12 && !idle) {
    if(step && ++a->timer==delay){a->timer=0;if(++a->phase==7)a->phase=0;}
    if(!rom_byte(rom,size,table+1+r[10]*4,&value))return 0;
    *frame=(uint8_t)(value+a->phase);return 1;
  }
  if (idle) a->timer = a->phase = 0;
  else if (step && ++a->timer == delay) { a->timer = 0; a->phase = (a->phase + 1) & 3; }
  if (!rom_byte(rom, size, table + 1 + r[10] * 4 + a->phase, &value)) return 0;
  *frame = (uint8_t)(value + (type == 2 ? (r[0x2c] << 4) : 0));
  return 1;
}

int LagoonWideEntities_CommonFrame(const uint8_t *rom,size_t size,
    const uint8_t *r,LagoonWideAnimation *a,int step,uint8_t *frame) {
  return ordinary_frame(rom,size,r,a,step,frame,0);
}
static int source_hash(const uint8_t *rom,size_t size,unsigned address,unsigned length,uint64_t expected) {
  uint64_t hash=UINT64_C(14695981039346656037);unsigned value;
  for(unsigned i=0;i<length;++i) {
    if(!rom_byte(rom,size,address+i,&value))return 0;
    hash=(hash^value)*UINT64_C(1099511628211);
  }
  return hash==expected;
}
int LagoonWideEntities_AnimationSource(const uint8_t *rom,size_t size,unsigned type) {
  if(!rom || size!=kRomSize)return 0;
  /* Source instruction boundaries include the original active-bit dispatch,
   * ordinary status jump table, normal selector and shared idle reset. The
   * status4 B131 and death B413 entries never borrow the normal AB5E contract. */
  if(type==1)return source_hash(rom,size,0x01AC4F,91,UINT64_C(0xa89cd07e01c8c275));
  return type==2 && source_hash(rom,size,0x01AB31,45,UINT64_C(0x7ace581df63ce6dc)) &&
    source_hash(rom,size,0x01AB5E,131,UINT64_C(0x04eeafcdc7754770));
}
typedef struct ActorKindContract { uint64_t npc[2],enemy;uint16_t prop; } ActorKindContract;
/* Exact authored lists plus explicitly observed original script spawns.
 * An ID admits a draw decoder, never its AI or arbitrary source graphics. */
static const ActorKindContract s_actor_kinds[75]={
  [0x00]={{UINT64_C(0x18807f8000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0003},
  [0x01]={{UINT64_C(0x0000000000000c00),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x02]={{UINT64_C(0x000000000000f800),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0002},
  [0x03]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000003),0x0004},
  [0x04]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000001),0x0000},
  [0x05]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000005),0x0000},
  [0x06]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x07]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x08]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000018),0x0000},
  [0x09]={{UINT64_C(0x077f800000010001),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x0a]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0500},
  [0x0b]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000060),0x0008},
  [0x0c]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000600),0x0002},
  [0x0d]={{UINT64_C(0x0000000038000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000180),0x0010},
  [0x0e]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x0f]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x10]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000001800),0x0000},
  [0x11]={{UINT64_C(0xe000000000020001),UINT64_C(0x000000000000001f)},UINT64_C(0x0000000000000000),0x0003},
  [0x12]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x13]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x14]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000006000),0x0000},
  [0x15]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000006000),0x0000},
  [0x16]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x17]={{UINT64_C(0x0000004800000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0002},
  [0x18]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000018000),0x0000},
  [0x19]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000007fe0)},UINT64_C(0x0000000000000000),0x0003},
  [0x1a]={{UINT64_C(0x00000000c0000000),UINT64_C(0x0000000008008000)},UINT64_C(0x0000000000000000),0x0000},
  [0x1b]={{UINT64_C(0x0000000000040000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0002},
  [0x1c]={{UINT64_C(0x0000000007f80000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x1d]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x1e]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000060000),0x00c0},
  [0x1f]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x20]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000600000),0x0000},
  [0x21]={{UINT64_C(0x0000000200000001),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x22]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000180000),0x0000},
  [0x23]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x24]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000007ff0000)},UINT64_C(0x0000000000000000),0x0000},
  [0x25]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x26]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000004800000),0x0002},
  [0x27]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x28]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000001000000),0x0000},
  [0x29]={{UINT64_C(0x0000000000000380),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x2a]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x2b]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x2c]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000380000000),0x0000},
  [0x2d]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x2e]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x2f]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x000000000a000000),0x0000},
  [0x30]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x31]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x32]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x33]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x34]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x35]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000030000000),0x0000},
  [0x36]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x37]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x38]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000400000000),0x0000},
  [0x39]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000400000000),0x0000},
  [0x3a]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0002},
  [0x3b]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x3c]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x3d]={{UINT64_C(0x0000000400000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x3e]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000840000000),0x0002},
  [0x3f]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0200},
  [0x40]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0200},
  [0x41]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x42]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0800},
  [0x43]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x44]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000001000000000),0x1000},
  [0x45]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x2000},
  [0x46]={{UINT64_C(0x0000000000000020),UINT64_C(0x0000000000000000)},UINT64_C(0x0000002000000000),0x0000},
  [0x47]={{UINT64_C(0x0000000000000054),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x48]={{UINT64_C(0x0000000000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x49]={{UINT64_C(0x0000003000000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
  [0x4a]={{UINT64_C(0x0000000800000000),UINT64_C(0x0000000000000000)},UINT64_C(0x0000000000000000),0x0000},
};
int LagoonWideEntities_AllowedKind(unsigned room,unsigned type,unsigned kind) {
  if(room>=75)return 0;
  const ActorKindContract *c=&s_actor_kinds[room];
  if(type==1 && kind<128) {
    if(kind<2)return 1; /* Source loader creates the flagged global follower in slot800. */
    if(room>=0x2c && room<=0x37 && kind>=2 && kind<64)return 1; /* Preserve proven prior selector scope. */
    return (int)((c->npc[kind/64]>>(kind%64))&1);
  }
  if(type==2 && kind<64)return (int)((c->enemy>>kind)&1);
  if(type==5 && kind<16)return (c->prop>>kind)&1;
  return 0;
}
static int town_npc_kind(unsigned room,unsigned kind) {return LagoonWideEntities_AllowedKind(room,1,kind);}
static int field_room(unsigned room) {return room==0x18 || (room>=0x1d&&room<=0x23)||room==0x25;}
static int field_enemy_kind(unsigned room,unsigned kind) {return LagoonWideEntities_AllowedKind(room,2,kind);}
static int follower_record(const uint8_t *rom,size_t size,const uint8_t *ram,const uint8_t *r) {
  if(r[24]!=1)return 1;
  unsigned room=ram[0x48e]|ram[0x48f]<<8,kind=r[27];
  /* These original entry scripts also create ordinary slot-based NPC0/1,
   * and Lagon's conditional NPC8. They do not borrow follower flag authority. */
  if((kind<2 && r!=ram+0xd00)||(room==0x29&&kind==8)) {
    static const struct {unsigned room,kind,entry,length;uint64_t hash;} scripts[]={
      {0,1,0x05cbcd,38,UINT64_C(0x361559ef14a81ced)},
      {7,1,0x05cc41,22,UINT64_C(0x941259f71206e717)},
      {9,0,0x05cc57,33,UINT64_C(0x76808cf19fc94554)},
      {0x11,0,0x05ccab,31,UINT64_C(0xe539f2cbf0fc068c)},
      {0x21,0,0x05cd3d,25,UINT64_C(0xd206427893a378bf)},
      {0x29,8,0x05cd56,60,UINT64_C(0x2db342b5984787c0)}
    };
    uintptr_t offset=(uintptr_t)r-(uintptr_t)(ram+0x500);
    if(offset>=0x800||(offset&63)||!source_hash(rom,size,0x01d20c,113,UINT64_C(0x06104d02789e20cc)))return 0;
    for(unsigned i=0;i<sizeof(scripts)/sizeof(scripts[0]);++i)
      if(room==scripts[i].room&&kind==scripts[i].kind)
        return source_hash(rom,size,scripts[i].entry,scripts[i].length,scripts[i].hash);
    return 0;
  }
  if(kind>=2)return 1;
  return r==ram+0xd00 && (r[27]?(ram[0x4e1]&1):((ram[0x4e1]&3)==2)) &&
    source_hash(rom,size,0x018c4d,7,UINT64_C(0x4df33da483454035)) &&
    source_hash(rom,size,0x018d41,101,UINT64_C(0x601c693bd3494753)) &&
    source_hash(rom,size,0x018da6,32,UINT64_C(0x38eb4c07305e8bdd));
}
int LagoonWideEntities_FrameForRoom(const uint8_t *rom,size_t size,unsigned room,
    const uint8_t *r,LagoonWideAnimation *a,int step,uint8_t *frame) {
  if(!r || !LagoonWideEntities_AllowedKind(room,r[0x18],r[0x1b]) ||
      !LagoonWideEntities_AnimationSource(rom,size,r[0x18]))return 0;
  if(r[0x18]==2 && r[0x1b]==12 && !source_hash(rom,size,0x01abe1,59,UINT64_C(0x116f3118893f0789)))return 0;
  if(r[0x18]==2 && (r[0x1b]==26 || r[0x1b]==37) && !(r[0]&0x20) && ((r[0]&1)||(r[11]&0x80)))return 0;
  return ordinary_frame(rom,size,r,a,step,frame,1);
}
int LagoonWideEntities_FrameForContext(const uint8_t *rom,size_t size,const uint8_t *ram,
    const uint8_t *r,LagoonWideAnimation *a,int step,uint8_t *frame) {
  if(!ram||!r||!a||!frame||!follower_record(rom,size,ram,r))return 0;
  unsigned room=ram[0x48e]|ram[0x48f]<<8;
  if(r[0x18]==2 && r[0x1b]==37 && !(r[0]&0x20) && ((r[0]&1)||(r[11]&0x80))) {
    if((r[1]&7)||r[10]>3||a->phase>3||r[0x21]>=6||!LagoonWideEntities_AllowedKind(room,2,37)||
        !LagoonWideEntities_AnimationSource(rom,size,2)||!source_hash(rom,size,0x01ac30,31,UINT64_C(0x7499138d345643a6)))return 0;
    unsigned base;if(!rom_byte(rom,size,0x01ac49+r[0x21],&base))return 0;
    *frame=(uint8_t)(base+((ram[0x408]&4)>>2));return 1;
  }
  if(r[0x18]==2 && r[0x1b]==26 && !(r[0]&0x20) && ((r[0]&1)||(r[11]&0x80))) {
    if((r[1]&7)||r[10]>3||a->phase>3||!LagoonWideEntities_AllowedKind(room,2,26)||!LagoonWideEntities_AnimationSource(rom,size,2)||
        !source_hash(rom,size,0x01ac1c,20,UINT64_C(0x69550dc73897bd12)))return 0;
    uintptr_t offset=(uintptr_t)r-(uintptr_t)(ram+0x500);
    if(offset>0x800 || (offset&63))return 0;
    if(ram[0x8000+offset]){*frame=12+((ram[0x408]&12)>>2);return 1;}
    return ordinary_frame(rom,size,r,a,step,frame,1);
  }
  return LagoonWideEntities_FrameForRoom(rom,size,room,r,a,step,frame);
}
static unsigned record_word(const uint8_t *ram,unsigned at){return ram[at]|ram[at+1]<<8;}
/* Finite original frame-table extents, independently executed through real
 * 008B71/8211 for every admitted entry. Hashes cover table pointers and every
 * admitted stream, including shared NPC frames. These are source identities,
 * not copied graphics. No cache can lend authority after a ROM mutation. */
typedef struct CurrentFrameSource { unsigned table,count,base,length; uint64_t hash; } CurrentFrameSource;
static const CurrentFrameSource s_current_frames[76]={
  {0x009a84,252,0x009a84,3468,UINT64_C(0x2e69abe018e1d1e4)}, /* 00 */
  {0x00a810,104,0x00a810,1008,UINT64_C(0xe4f8658bd96b3daf)}, /* 01 */
  {0x00ac00,25,0x00a8e0,856,UINT64_C(0x7b4b5782ceafa841)}, /* 02 */
  {0x00ac00,25,0x00a8e0,856,UINT64_C(0x7b4b5782ceafa841)}, /* 03 */
  {0x00ac38,46,0x00ac38,926,UINT64_C(0xe68d9dff6bdbba49)}, /* 04 */
  {0x00ac38,46,0x00ac38,926,UINT64_C(0xe68d9dff6bdbba49)}, /* 05 */
  {0x00ac38,46,0x00ac38,926,UINT64_C(0xe68d9dff6bdbba49)}, /* 06 */
  {0x00ac38,46,0x00ac38,926,UINT64_C(0xe68d9dff6bdbba49)}, /* 07 */
  {0x00afd6,14,0x00a2a8,3458,UINT64_C(0xbc7ec60a59c70146)}, /* 08 */
  {0x00b02a,36,0x00b02a,757,UINT64_C(0xa0346984ddc50169)}, /* 09 */
  {0x00a810,104,0x00a810,1008,UINT64_C(0xe4f8658bd96b3daf)}, /* 0A */
  {0x00b02a,36,0x00b02a,757,UINT64_C(0xa0346984ddc50169)}, /* 0B */
  {0x00b31f,24,0x00b31f,818,UINT64_C(0xf56ec5426c47c258)}, /* 0C */
  {0x00b651,32,0x00b651,705,UINT64_C(0x2e85c99c80680bb7)}, /* 0D */
  {0x00b912,19,0x00b912,584,UINT64_C(0xd8d0b79c9fcd843a)}, /* 0E */
  {0x00b912,19,0x00b912,584,UINT64_C(0xd8d0b79c9fcd843a)}, /* 0F */
  {0x00b912,19,0x00b912,584,UINT64_C(0xd8d0b79c9fcd843a)}, /* 10 */
  {0x00bb5a,37,0x00bb5a,1135,UINT64_C(0x8822540c302f6101)}, /* 11 */
  {0x00bfc9,40,0x00a8e0,6044,UINT64_C(0xb77d91550a787dd0)}, /* 12 */
  {0x00bfc9,40,0x00a8e0,6044,UINT64_C(0xb77d91550a787dd0)}, /* 13 */
  {0x00bfc9,40,0x00a8e0,6044,UINT64_C(0xb77d91550a787dd0)}, /* 14 */
  {0x00c07c,42,0x00c07c,1142,UINT64_C(0x29bb62baaa639e83)}, /* 15 */
  {0x00c07c,42,0x00c07c,1142,UINT64_C(0x29bb62baaa639e83)}, /* 16 */
  {0x00c07c,42,0x00c07c,1142,UINT64_C(0x29bb62baaa639e83)}, /* 17 */
  {0x00c4f2,13,0x00c4f2,102,UINT64_C(0x0f7883eb18635871)}, /* 18 */
  {0x00c558,32,0x00c558,1340,UINT64_C(0x388eec09aea8496f)}, /* 19 */
  {0x00ca94,73,0x00a8e0,8907,UINT64_C(0x2422ac6c3ebcf0f3)}, /* 1A */
  {0x00ca94,73,0x00a8e0,8907,UINT64_C(0x2422ac6c3ebcf0f3)}, /* 1B */
  {0x00a810,104,0x00a810,1008,UINT64_C(0xe4f8658bd96b3daf)}, /* 1C */
  {0x00ca94,73,0x00a8e0,8907,UINT64_C(0x2422ac6c3ebcf0f3)}, /* 1D */
  {0x00ca94,73,0x00a8e0,8907,UINT64_C(0x2422ac6c3ebcf0f3)}, /* 1E */
  {0x00cbab,16,0x00cbab,356,UINT64_C(0xeefca16c8693fa25)}, /* 1F */
  {0x00cbab,16,0x00cbab,356,UINT64_C(0xeefca16c8693fa25)}, /* 20 */
  {0x00cd0f,24,0x00cd0f,768,UINT64_C(0x40ec09ce944398ac)}, /* 21 */
  {0x03cd7a,28,0x03cd7a,1486,UINT64_C(0xbf814474dbbd3f87)}, /* 22 */
  {0x00d00f,24,0x00d00f,962,UINT64_C(0x09a4550bb993b3b1)}, /* 23 */
  {0x00d00f,24,0x00d00f,962,UINT64_C(0x09a4550bb993b3b1)}, /* 24 */
  {0x00a810,104,0x00a810,1008,UINT64_C(0xe4f8658bd96b3daf)}, /* 25 */
  {0x00d00f,24,0x00d00f,962,UINT64_C(0x09a4550bb993b3b1)}, /* 26 */
  {0x00d3d1,21,0x00d3d1,469,UINT64_C(0x2b4b01bebe7edb1f)}, /* 27 */
  {0x00d5a6,2,0x00d5a6,16,UINT64_C(0xec4a29c690e38037)}, /* 28 */
  {0x00d5b6,14,0x00d5b6,728,UINT64_C(0xdc691f296d448dc9)}, /* 29 */
  {0x00d88e,31,0x009c7c,16054,UINT64_C(0xd3d6986fa588972b)}, /* 2A */
  {0x00d5b6,14,0x00d5b6,728,UINT64_C(0xdc691f296d448dc9)}, /* 2B */
  {0x00d5b6,14,0x00d5b6,728,UINT64_C(0xdc691f296d448dc9)}, /* 2C */
  {0x00db32,36,0x00db32,1390,UINT64_C(0xee17d8c2f3857ed9)}, /* 2D */
  {0x00db32,36,0x00db32,1390,UINT64_C(0xee17d8c2f3857ed9)}, /* 2E */
  {0x00db32,36,0x00db32,1390,UINT64_C(0xee17d8c2f3857ed9)}, /* 2F */
  {0x00e0a0,20,0x00e0a0,766,UINT64_C(0xd1e7cf6c6bc2002e)}, /* 30 */
  {0x00e0a0,20,0x00e0a0,766,UINT64_C(0xd1e7cf6c6bc2002e)}, /* 31 */
  {0x00e0a0,20,0x00e0a0,766,UINT64_C(0xd1e7cf6c6bc2002e)}, /* 32 */
  {0x00e0a0,20,0x00e0a0,766,UINT64_C(0xd1e7cf6c6bc2002e)}, /* 33 */
  {0x00e0a0,20,0x00e0a0,766,UINT64_C(0xd1e7cf6c6bc2002e)}, /* 34 */
  {0x00e0a0,20,0x00e0a0,766,UINT64_C(0xd1e7cf6c6bc2002e)}, /* 35 */
  {0x00e39e,26,0x00e39e,1038,UINT64_C(0x63026187fc3cd01a)}, /* 36 */
  {0x00e39e,26,0x00e39e,1038,UINT64_C(0x63026187fc3cd01a)}, /* 37 */
  {0x00e39e,26,0x00e39e,1038,UINT64_C(0x63026187fc3cd01a)}, /* 38 */
  {0x00e7ac,12,0x00e7ac,1339,UINT64_C(0xdce28a5ea33b04b1)}, /* 39 */
  {0x00e7ac,12,0x00e7ac,1339,UINT64_C(0xdce28a5ea33b04b1)}, /* 3A */
  {0x00e7ac,12,0x00e7ac,1339,UINT64_C(0xdce28a5ea33b04b1)}, /* 3B */
  {0x00e7ac,12,0x00e7ac,1339,UINT64_C(0xdce28a5ea33b04b1)}, /* 3C */
  {0x00e7ac,12,0x00e7ac,1339,UINT64_C(0xdce28a5ea33b04b1)}, /* 3D */
  {0x00a810,104,0x00a810,1008,UINT64_C(0xe4f8658bd96b3daf)}, /* 3E */
  {0x00ece7,24,0x00ece7,1030,UINT64_C(0xee9e558a5d33f300)}, /* 3F */
  {0x00f0ed,31,0x00f0ed,2412,UINT64_C(0x18c65dc5a1f27a58)}, /* 40 */
  {0x03af90,9,0x03af90,1550,UINT64_C(0xd2c520c40384aa0d)}, /* 41 */
  {0x03b59e,7,0x03b59e,1735,UINT64_C(0x9e0fc75eb0b4ced0)}, /* 42 */
  {0x03bc65,10,0x03bc65,2232,UINT64_C(0x414ea6fbd3b18159)}, /* 43 */
  {0x03c51d,12,0x03c51d,731,UINT64_C(0xdadc59eb8b8ec256)}, /* 44 */
  {0x03c7f8,23,0x03c7f8,1410,UINT64_C(0x43d5f5d876542472)}, /* 45 */
  {0x03cd7a,28,0x03cd7a,1486,UINT64_C(0xbf814474dbbd3f87)}, /* 46 */
  {0x03d348,23,0x03d348,3474,UINT64_C(0x1a2e8285af711dd3)}, /* 47 */
  {0x03e0da,37,0x03e0da,5932,UINT64_C(0xfa6665d102508321)}, /* 48 */
  {0x03f806,2,0x03f806,32,UINT64_C(0x2df57c12c7e97f87)}, /* 49 */
  {0x00d5a6,2,0x00d5a6,16,UINT64_C(0xec4a29c690e38037)}, /* 4A */
  {0x00c4f2,13,0x00c4f2,102,UINT64_C(0x0f7883eb18635871)}, /* 4B */
};
int LagoonWideEntities_CurrentSource(const uint8_t *rom,size_t size,unsigned selector,unsigned frame) {
  if(selector>=76 || frame==255)return 0;
  const CurrentFrameSource *f=&s_current_frames[selector];
  /* Four dynamic WRAM packs and two empty records are deliberately outside
   * the fixed source/atlas contract. */
  if(frame>=f->count || (selector==0&&(frame<4||frame==168||frame==169)))return 0;
  unsigned lo,hi,bank;
  if(!rom_byte(rom,size,0x999d+selector*3,&lo)||!rom_byte(rom,size,0x999e + selector*3,&hi)||
      !rom_byte(rom,size,0x999f+selector*3,&bank)||(lo|(hi<<8)|(bank<<16))!=f->table ||
      !source_hash(rom,size,f->base,f->length,f->hash))return 0;
  static const uint8_t stock_call[]={0x20,0xb5,0xa7},center_call[]={0x20,0xd8,0x83};
  static const uint8_t center_dispatch[]={
    0x5a,0x9b,0xd0,0x16,0xda,0x64,0x41,0x20,0xfb,0x83,0x20,0x26,0x84,
    0x20,0x52,0x84,0x20,0x7d,0x84,0xfa,0xad,0x19,0x05,0x8d,0x19,0x05,
    0x7a,0x4c,0xb5,0xa7,0xea,0xea,0xea,0xea,0xea};
  unsigned call=0x80ca;
  if(!rom||size<0x10000)return 0;
  if(memcmp(rom+call,stock_call,3)) {
    if(memcmp(rom+call,center_call,3)||memcmp(rom+0x83d8,center_dispatch,sizeof(center_dispatch))||
       memcmp(rom+0x809d,"\xea\xea\xea",3))return 0;
  }else if(memcmp(rom+0x809d,center_call,3))return 0;
  if(!source_hash(rom,size,0x0180b2,24,UINT64_C(0xcde1e7a18b1664d9)))return 0;
  if(!source_hash(rom,size,0x0180cd,22,UINT64_C(0x35e70cb4b5b4d9d2)))return 0;
  if(!source_hash(rom,size,0x01a4d3,24,UINT64_C(0x09abeb222846112b)))return 0;
  if(!source_hash(rom,size,0x01a5aa,191,UINT64_C(0xacc4bafd082e0a60)))return 0;
  if(!source_hash(rom,size,0x008b4b,331,UINT64_C(0x679426345d98810a)))return 0;
  if(!source_hash(rom,size,0x008d0b,43,UINT64_C(0xe45ef8fb2dd76ec9)))return 0;
  if(!source_hash(rom,size,0x008211,138,UINT64_C(0x1b15a374826ab834)))return 0;
  if(!source_hash(rom,size,0x01c402,25,UINT64_C(0xe455ff655b106f9a)))return 0;
  if(!source_hash(rom,size,0x01c486,41,UINT64_C(0x2eb39b5302337bbe)))return 0;
  return 1;
}
int LagoonWideEntities_CurrentFrame(const uint8_t *rom,size_t size,const uint8_t *captured,
    const uint8_t *current,unsigned entity,uint8_t *frame) {
  if(!captured||!current||!frame||entity>0x800||(entity&63))return 0;
  const uint8_t *r=captured+0x500+entity,*now=current+0x500+entity;
  unsigned room=captured[0x48e]|captured[0x48f]<<8,type=r[24],kind=r[27];
  if((r[0]&0xc8)!=0xc0 || memcmp(r,now,64) || captured[0x48e]!=current[0x48e] ||
      captured[0x48f]!=current[0x48f] || captured[0x90]!=current[0x90] || captured[0x91]!=current[0x91] ||
      !LagoonWideEntities_AllowedKind(room,type,kind) || !follower_record(rom,size,captured,r) ||
      !follower_record(rom,size,current,now) || (type!=1&&type!=2&&type!=5) ||
      (type==2&&(r[1]&1)&&kind<37) || (type==5&&r[28]!=0x3e))return 0;
  for(unsigned q=0;q<2;++q) {
    const uint8_t *ram=q?current:captured,*record=ram+0x500+entity;
    unsigned wx=record[2]|record[3]<<8,wy=record[4]|record[5]<<8;
    unsigned cx=ram[0x480]|ram[0x481]<<8,cy=ram[0x482]|ram[0x483]<<8;
    if((record[12]|record[13]<<8)!=((wx-cx)&65535) ||
       (record[14]|record[15]<<8)!=((wy-cy)&65535))return 0;
  }
  if((type==1&&!source_hash(rom,size,0x01c1c6,78,UINT64_C(0x923d6369f8f0853c))) ||
     (type==2&&!source_hash(rom,size,0x01c214,112,UINT64_C(0xd94970056e0ec0f2))) ||
     (type==5&&!source_hash(rom,size,0x01c44e,97,UINT64_C(0x036e1148b4ed807b))))return 0;
  unsigned selector=type==5||(type==1&&kind<2)?0:room+1;
  if(!LagoonWideEntities_CurrentSource(rom,size,selector,r[28]))return 0;
  *frame=r[28];return 1;
}

int LagoonWideEntities_RecordCells(const uint8_t *rom,size_t size,const uint8_t *ram,
    unsigned entity,unsigned frame,int projected,LagoonWideCell *cells,size_t capacity) {
  if(!ram||!rom||!cells||entity>0x800||(entity&63))return -1;
  unsigned a=0x500+entity,type=ram[a+24],kind=ram[a+27],room=record_word(ram,0x48e);
  if(!LagoonWideEntities_AllowedKind(room,type,kind)||!follower_record(rom,size,ram,ram+a))return -1;
  if((type==1&&!source_hash(rom,size,0x01c1c6,78,UINT64_C(0x923d6369f8f0853c))) ||
     (type==2&&!source_hash(rom,size,0x01c214,112,UINT64_C(0xd94970056e0ec0f2))) ||
     (type==5&&!source_hash(rom,size,0x01c44e,97,UINT64_C(0x036e1148b4ed807b))))return -1;
  if((type==1&&kind>91)||(type==2&&(((ram[a+1]&1)&&kind<37)||kind>37)))return -1;
  int x=projected?(int16_t)(record_word(ram,a+2)-record_word(ram,0x480)):(int16_t)record_word(ram,a+12);
  int y=projected?(int16_t)(record_word(ram,a+4)-record_word(ram,0x482)):(int16_t)record_word(ram,a+14);
  unsigned selector=room+1,ox=0,oy=0;
  if(type==5) {
    if(frame!=0x3e)return -1;
    selector=0;x-=16;y-=2*(ram[a+0x15]/3)+46;y+=(int16_t)record_word(ram,0x90);
  }else if(type==1||type==2) {
    unsigned offsets=type==1?0x1fb7f:0x1f7e9;
    if(!rom_word(rom,size,offsets+kind*4,&ox)||!rom_word(rom,size,offsets+kind*4+2,&oy))return -1;
    x+=(int16_t)ox;y+=(int16_t)oy;
    if(type==1&&kind<2)selector=0;
  }else return -1;
  int count=LagoonWideEntities_Decode(rom,size,selector,frame,x,y,cells,capacity);
  if(type==1&&ram[0x50]==2)
    for(int i=0;i<count;++i)cells[i].tile=(cells[i].tile&~0x3000)|0x3000;
  if(type==2&&((ram[a+1]&0x80)||(ram[a+0x1f]&2)))
    for(int i=0;i<count;++i)cells[i].tile=(cells[i].tile&~0xe00)|0xe00;
  return count;
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

static int decode_entity(const uint8_t *ram,unsigned entity,unsigned frame,int projected,LagoonWideCell *cells) {
  unsigned room=ram_word(ram,0x48e);
  if((room==1||room==2||room>=9)&&!LagoonSceneRenderer_ExactRoom(ram))return -1;
  unsigned a=0x500+entity;
  if((ram[a+24]==5 || (ram[a+24]==1&&ram[a+27]<2)) &&
      (g_ppu->bgmode!=9 || g_ppu->bgXsc[0]!=0x71 || g_ppu->bgXsc[1]!=(ram[0x50]?0x7a:0x79) ||
       g_ppu->bgXsc[2]!=0x3c || g_ppu->bgTileAdr!=0x600 || g_ppu->inidisp!=15 || g_ppu->mosaic ||
       g_ppu->setini || g_ppu->cgadsub || g_ppu->windowsel || (g_ppu->cgwsel&1)))return -1;
  return LagoonWideEntities_RecordCells(g_rom,kRomSize,ram,entity,frame,projected,cells,kCells);
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
  if(channel==1 && !t->fromB && t->mode==4 && t->bAdr==0x16 &&
      !t->fixed && !t->decrement && t->aBank==0x7e && t->aAdr==0x2600 &&
      g_ppu->vramIncrement==1 && g_ppu->vramIncrementOnHigh && !g_ppu->vramRemapMode)
    LagoonWide_OnParallaxQueueDma(t->size);
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
  s_retained_objects.valid=0;
  s_frame.valid=0; s_last_serial=0; s_object_count=0; s_has_far=0; s_pose_room_key=0; s_pose_generation=0;
  memset(s_pose,0,sizeof(s_pose));memset(s_native_pose,0,sizeof(s_native_pose)); memset(s_kind_witness,0,sizeof(s_kind_witness));
  LagoonWideAtlas_Reset(&s_atlas);
  if (g_ppu) PpuSetExtraObjects(g_ppu,NULL,0);
}
/* A native-anchored margin cell can outlive the CPU clearing the outgoing
 * actor records: its original, unchanged OAM slot still owns the displayed
 * position/attributes, and its actual atlas/palette own every retained pixel.
 * Virtual far cells have no such independent witness and still bind all RAM. */
static int retained_native_anchor(const PpuExtraObject *o) {
  if(o->order!=1 || o->oamSlot>=128 || g_ppu->obsel!=2 || PPU_objPriority(g_ppu))return 0;
  unsigned slot=o->oamSlot,hi=(g_ppu->highOam[slot/4]>>((slot%4)*2))&3;
  const uint8_t *bytes=(const uint8_t*)g_ppu->oam;
  unsigned size=(hi&2)?16:8,tile=ram_word(bytes,slot*4+2);
  if(o->width!=size || o->height!=size || o->stride<size ||
      ((unsigned)o->x&511)!=(bytes[slot*4]|((hi&1)<<8)) ||
      ((unsigned)o->y&255)!=bytes[slot*4+1] ||
      o->priority!=((tile>>12)&3) || o->math!=!!(tile&0x800))return 0;
  unsigned base=(tile&0x100)?PPU_objTileAdr2(g_ppu):PPU_objTileAdr1(g_ppu);
  for(unsigned y=0;y<size;y+=8)for(unsigned x=0;x<size;x+=8) {
    unsigned ch=((((tile&255)>>4)+y/8)<<4)|(((tile&15)+x/8)&15);
    unsigned address=(base+ch*16)&0x7fff;
    if(address<0x4400 || address+16>0x5900)return 0;
  }
  LagoonWideCell cell={o->x,o->y,(uint16_t)tile,(uint8_t)size};uint16_t expected[256];
  if(!LagoonWideEntities_Pixels(PpuRenderVram(g_ppu),g_ppu->cgram,
      PPU_objTileAdr1(g_ppu),PPU_objTileAdr2(g_ppu),&cell,expected,1))return 0;
  for(unsigned y=0;y<size;++y)
    if(memcmp(o->pixels+y*o->stride,expected+y*size,size*sizeof(uint16_t)))return 0;
  return 1;
}
/* Decode current native OAM only within eight pixels of the independently
 * captured anchor. Modulo wrap has exactly one solution in this small range.
 * Original world/map ownership is still required by the retained BG caller. */
static int retained_current_cell(const PpuExtraObject *o,LagoonWideCell *cell) {
  unsigned slot=o->oamSlot,hi=(g_ppu->highOam[slot/4]>>((slot%4)*2))&3;
  const uint8_t *bytes=(const uint8_t*)g_ppu->oam;
  unsigned size=(hi&2)?16:8,tile=ram_word(bytes,slot*4+2),xs=bytes[slot*4]|((hi&1)<<8),ys=bytes[slot*4+1];
  if(size!=o->width || size!=o->height)return 0;
  int x=o->x,y=o->y,xfound=0,yfound=0;
  for(int delta=-8;delta<=8;++delta) {
    if(((unsigned)(o->x+delta)&511)==xs){x=o->x+delta;xfound++;}
    if(((unsigned)(o->y+delta)&255)==ys){y=o->y+delta;yfound++;}
  }
  if(xfound!=1 || yfound!=1)return 0;
  unsigned base=(tile&0x100)?PPU_objTileAdr2(g_ppu):PPU_objTileAdr1(g_ppu);
  for(unsigned yy=0;yy<size;yy+=8)for(unsigned xx=0;xx<size;xx+=8) {
    unsigned ch=((((tile&255)>>4)+yy/8)<<4)|(((tile&15)+xx/8)&15);
    unsigned address=(base+ch*16)&0x7fff;
    if(address<0x4400 || address+16>0x5900)return 0;
  }
  *cell=(LagoonWideCell){x,y,(uint16_t)tile,(uint8_t)size};return 1;
}
static int retained_native_current(void) {
  uint8_t owned[128]={0};
  for(size_t i=0;i<s_retained_objects.count;++i) {
    const PpuExtraObject *o=&s_retained_objects.objects[i];LagoonWideCell cell;
    uint16_t pixels[256];owned[o->oamSlot]=1;
    if(!retained_current_cell(o,&cell) || !LagoonWideEntities_Pixels(PpuRenderVram(g_ppu),g_ppu->cgram,
        PPU_objTileAdr1(g_ppu),PPU_objTileAdr2(g_ppu),&cell,pixels,1))return 0;
  }
  /* Do not silently omit an unsupported actor newly entering the margins. */
  const uint8_t *current=(const uint8_t*)g_ppu->oam;
  for(unsigned slot=0;slot<128;++slot)if(!owned[slot]) {
    unsigned hi=(g_ppu->highOam[slot/4]>>((slot%4)*2))&3;
    unsigned oldhi=(s_retained_objects.oam[512+slot/4]>>((slot%4)*2))&3;
    if(!memcmp(current+slot*4,s_retained_objects.oam+slot*4,4) && hi==oldhi)continue;
    for(unsigned old=0;old<2;++old) {
      const uint8_t *bytes=old?s_retained_objects.oam:current;unsigned h=old?oldhi:hi;
      int x=bytes[slot*4]|((h&1)<<8),y=bytes[slot*4+1],size=(h&2)?16:8;
      if(x>=384)x-=512;if(y>=240)y-=256;
      if((x<0 || x+size>256) && y<224 && y+size>0)return 0;
    }
  }
  return 1;
}
int LagoonWideEntities_RetainedNativeAnchors(void) {return s_retained_objects.valid && s_retained_objects.native_anchors;}
void LagoonWideEntities_RetainScene(void) {
  s_retained_objects.valid=0;
  if(!g_ppu || g_ppu->extraObjectCount>kObjects)return;
  s_retained_objects.count=g_ppu->extraObjectCount;
  s_retained_objects.native_anchors=s_retained_objects.count!=0;
  for(size_t i=0;i<s_retained_objects.count;++i) {
    const PpuExtraObject *o=&g_ppu->extraObjects[i];
    if(!o->pixels || o->width>16 || o->height>16 || o->stride<o->width)return;
    if(!retained_native_anchor(o))s_retained_objects.native_anchors=0;
    s_retained_objects.objects[i]=*o;
    for(unsigned y=0;y<o->height;++y)
      memcpy(s_retained_objects.pixels[i]+y*o->width,o->pixels+y*o->stride,o->width*2);
    s_retained_objects.objects[i].pixels=s_retained_objects.pixels[i];
    s_retained_objects.objects[i].stride=o->width;
  }
  memcpy(s_retained_objects.atlas,PpuRenderVram(g_ppu)+0x4400,sizeof(s_retained_objects.atlas));
  memcpy(s_retained_objects.palette,g_ppu->cgram,sizeof(s_retained_objects.palette));
  memcpy(s_retained_objects.records,g_ram+0x540,sizeof(s_retained_objects.records));
  memcpy(s_retained_objects.oam,g_ppu->oam,512);
  memcpy(s_retained_objects.oam+512,g_ppu->highOam,32);
  s_retained_objects.obsel=g_ppu->obsel;s_retained_objects.oam_priority=PPU_objPriority(g_ppu);
  s_retained_objects.generation=RtlStateGeneration();s_retained_objects.valid=1;
}
void LagoonWideEntities_RetainedDiagnostic(unsigned *kind,unsigned *address,
    unsigned *captured,unsigned *current) {
  *kind=*address=*captured=*current=0;
  if(!s_retained_objects.valid || !g_ppu || s_retained_objects.generation!=RtlStateGeneration()) {
    *kind=1;return;
  }
  if(!s_retained_objects.count)return;
  if(s_retained_objects.obsel!=g_ppu->obsel || s_retained_objects.oam_priority!=PPU_objPriority(g_ppu)) {
    *kind=6;return;
  }
  for(unsigned i=0;!s_retained_objects.native_anchors && i<sizeof(s_retained_objects.records);++i)
    if(s_retained_objects.records[i]!=g_ram[0x540+i]) {
      *kind=2;*address=0x540+i;*captured=s_retained_objects.records[i];*current=g_ram[0x540+i];return;
    }
  for(unsigned i=0;i<sizeof(s_retained_objects.atlas)/sizeof(uint16_t);++i)
    if(s_retained_objects.atlas[i]!=PpuRenderVram(g_ppu)[0x4400+i]) {
      *kind=3;*address=0x4400+i;*captured=s_retained_objects.atlas[i];*current=PpuRenderVram(g_ppu)[0x4400+i];return;
    }
  for(unsigned i=0;i<256;++i)if(s_retained_objects.palette[i]!=g_ppu->cgram[i]) {
    *kind=4;*address=i;*captured=s_retained_objects.palette[i];*current=g_ppu->cgram[i];return;
  }
  if(s_retained_objects.native_anchors) {
    if(!retained_native_current())*kind=7;
    return;
  }
  for(size_t i=0;i<s_retained_objects.count;++i) {
    const PpuExtraObject *o=&s_retained_objects.objects[i];if(o->order!=1)continue;
    unsigned slot=o->oamSlot;
    for(unsigned j=slot*4;j<slot*4+4;++j)
      if(s_retained_objects.oam[j]!=((uint8_t*)g_ppu->oam)[j]) {
        *kind=5;*address=j;*captured=s_retained_objects.oam[j];*current=((uint8_t*)g_ppu->oam)[j];return;
      }
    unsigned j=512+slot/4,mask=3u<<((slot%4)*2);
    if((s_retained_objects.oam[j]^g_ppu->highOam[slot/4])&mask) {
      *kind=5;*address=j;*captured=s_retained_objects.oam[j];*current=g_ppu->highOam[slot/4];return;
    }
  }
}
int LagoonWideEntities_RetainedSceneValid(void) {
  if(!s_retained_objects.valid || !g_ppu || s_retained_objects.generation!=RtlStateGeneration())return 0;
  if(!s_retained_objects.count)return 1;
  if(s_retained_objects.obsel!=g_ppu->obsel || s_retained_objects.oam_priority!=PPU_objPriority(g_ppu))return 0;
  if((!s_retained_objects.native_anchors && memcmp(s_retained_objects.records,g_ram+0x540,sizeof(s_retained_objects.records))) ||
      memcmp(s_retained_objects.atlas,PpuRenderVram(g_ppu)+0x4400,sizeof(s_retained_objects.atlas)) ||
      memcmp(s_retained_objects.palette,g_ppu->cgram,sizeof(s_retained_objects.palette)))return 0;
  if(s_retained_objects.native_anchors)return retained_native_current();
  for(size_t i=0;i<s_retained_objects.count;++i) {
    const PpuExtraObject *o=&s_retained_objects.objects[i];
    /* Native-anchored cells must retain their actual slot. Virtual deep
     * ordering instead binds to the unchanged complete ordinary records. */
    if(o->order==1) {
      unsigned slot=o->oamSlot;
      if(memcmp(s_retained_objects.oam+slot*4,(uint8_t*)g_ppu->oam+slot*4,4) ||
          (((s_retained_objects.oam[512+slot/4]^g_ppu->highOam[slot/4])>>((slot%4)*2))&3))return 0;
    }
  }
  return 1;
}
void LagoonWideEntities_BeginRetained(int enabled) {
  s_object_count=0;s_has_far=0;
  PpuSetExtraObjects(g_ppu,NULL,0);
  if(enabled && LagoonWideEntities_RetainedSceneValid()) {
    if(!s_retained_objects.native_anchors)PpuSetExtraObjects(g_ppu,s_retained_objects.objects,s_retained_objects.count);
    else {
      for(size_t i=0;i<s_retained_objects.count;++i) {
        const PpuExtraObject *o=&s_retained_objects.objects[i];LagoonWideCell cell;
        if(!retained_current_cell(o,&cell) || !LagoonWideEntities_Pixels(PpuRenderVram(g_ppu),g_ppu->cgram,
            PPU_objTileAdr1(g_ppu),PPU_objTileAdr2(g_ppu),&cell,s_retained_draw_pixels[i],1))return;
        s_retained_draw[i]=(PpuExtraObject){cell.x,cell.y,cell.size,cell.size,cell.size,
            (cell.tile>>12)&3,!!(cell.tile&0x800),o->oamSlot,1,s_retained_draw_pixels[i]};
      }
      PpuSetExtraObjects(g_ppu,s_retained_draw,s_retained_objects.count);
    }
  }
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
        if(!LagoonWideEntities_FrameForContext(g_rom,kRomSize,ram,r,&h->animation,1,&ignored)) {h->valid=0;return;}
    } else h->animation=(LagoonWideAnimation){r[0x10],r[0x11]};
  }
  h->valid=1;h->type=r[0x18];h->kind=r[0x1b];h->clock=ram[0x408];h->serial=s_frame.serial;
  h->x=x;h->y=y;
}
static int pose_record_equal(const uint8_t *a,const uint8_t *b) {
  for(unsigned i=0;i<64;++i) {
    if(i>=12&&i<16)continue; /* Camera-derived native coordinates. */
    if(i==0){if((a[i]^b[i])&~8)return 0;} /* Original draw-queue membership. */
    else if(a[i]!=b[i])return 0;
  }
  return 1;
}
static int pose_centered(const uint8_t *ram,unsigned a) {
  return ram_word(ram,a+12)==((ram_word(ram,a+2)-ram_word(ram,0x480))&65535) &&
    ram_word(ram,a+14)==((ram_word(ram,a+4)-ram_word(ram,0x482))&65535);
}
static int native_pose_matches(const uint8_t *ram,unsigned e) {
  const WideNativePose *w=&s_native_pose[e];unsigned a=0x500+e*64;
  return w->valid && LagoonSceneRenderer_ExactRoom(ram) && LagoonSceneRenderer_ExactRoom(g_ram) &&
    ram_word(ram,0x48e)==ram_word(g_ram,0x48e) && w->generation==s_frame.generation && w->room==ram_word(ram,0x48e) &&
    w->source==ram_pointer(ram,0x5d) && w->width==ram_word(ram,0x53) && w->height==ram_word(ram,0x55) &&
    w->dp90==ram_word(ram,0x90) && w->dp90==ram_word(g_ram,0x90) &&
    pose_record_equal(w->record,ram+a) && pose_record_equal(ram+a,g_ram+a) &&
    pose_centered(ram,a) && pose_centered(g_ram,a) &&
    !memcmp(w->atlas,PpuRenderVram(g_ppu)+0x4400,sizeof(w->atlas)) &&
    !memcmp(w->palette,g_ppu->cgram,sizeof(w->palette));
}

static int current_pose_frame(const uint8_t *ram,unsigned e,uint8_t *frame) {
  return LagoonSceneRenderer_ExactRoom(ram) && LagoonSceneRenderer_ExactRoom(g_ram) &&
    !ram[0x100e] && !g_ram[0x100e] && g_ppu->inidisp==15 && g_ppu->bgmode==9 &&
    g_ppu->bgXsc[0]==0x71 && g_ppu->bgXsc[1]==(ram[0x50]?0x7a:0x79) &&
    g_ppu->bgXsc[2]==0x3c && g_ppu->bgTileAdr==0x600 && !g_ppu->mosaic &&
    !g_ppu->setini && !g_ppu->cgadsub && !g_ppu->windowsel && !(g_ppu->cgwsel&1) &&
    LagoonWideEntities_CurrentFrame(g_rom,kRomSize,ram,g_ram,e*64,frame);
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
  if(ram_word(ram,0x48e)!=ram_word(g_ram,0x48e) || ram_word(ram,0x48c)>64) return;
  unsigned room=ram_word(ram,0x48e);int additional=room==1 || room==2 || room>=9;
  /* The OAM snapshot and current scene must independently name this exact
   * authored descriptor. Room numbers or shared OBJ art alone grant nothing. */
  if(additional && (room!=ram_word(g_ram,0x48e) || ram_word(ram,0x48c)>64 || ram[0x100e] || g_ram[0x100e] ||
      g_ppu->inidisp!=15 || !LagoonSceneRenderer_ExactRoom(ram) ||
      !LagoonSceneRenderer_ExactRoom(g_ram) || g_ppu->bgmode!=9 || g_ppu->bgXsc[0]!=0x71 ||
      g_ppu->bgXsc[1]!=(ram[0x50]?0x7a:0x79) || g_ppu->bgXsc[2]!=0x3c || g_ppu->bgTileAdr!=0x600 ||
      g_ppu->mosaic || g_ppu->setini || g_ppu->cgadsub || g_ppu->windowsel || (g_ppu->cgwsel&1))) {
    report_status("unsupported-additional-actor-scene",0,0);return;
  }
  unsigned roomkey=ram_pointer(ram,0x5d) ^ ((unsigned)ram[0x48e]<<24);
  if(roomkey!=s_pose_room_key || s_pose_generation!=s_frame.generation) {
    memset(s_pose,0,sizeof(s_pose));memset(s_native_pose,0,sizeof(s_native_pose));memset(s_kind_witness,0,sizeof(s_kind_witness));s_pose_room_key=roomkey;s_pose_generation=s_frame.generation;
  }
  int atlas_ok=s_atlas.ready && s_atlas.generation==s_frame.generation &&
      s_atlas.room==ram[0x48e] && s_atlas.source==ram_pointer(ram,0x5d);
  uint8_t used[128]={0},queued[kEntities]={0},far[kEntities]={0},far_frames[kEntities]={0},current_path[kEntities]={0};
  int cell_slot[kEntities][kCells],counts[kEntities]={0};
  LagoonWideCell cells[kEntities][kCells];
  memset(cell_slot,-1,sizeof(cell_slot));
  for(unsigned q=0;q<ram[0x48c];++q) {
    unsigned offset=ram_word(ram,0xec0+q*6),type=ram[0xec4+q*6];
    if(offset>0x800 || (offset&63)) continue;
    unsigned e=offset/64,a=0x500+offset;queued[e]=1;
    if((type!=1 && type!=2 && type!=5) || type!=ram[a+0x18]) continue;
    int n=decode_entity(ram,offset,ram[a+0x1c],0,cells[e]);counts[e]=n;
    if(n<0) continue;
    for(int i=0;i<n;++i) {
      if(cells[e][i].y<-(int)cells[e][i].size || cells[e][i].y>=240) continue;
      int slot=find_cell(s_frame.oam,&cells[e][i],used);
      if(slot<0) {s_rejected++;report_status("native-cell-mismatch",offset,i);return;}
      used[slot]=1;cell_slot[e][i]=slot;
    }
    int anchored=0;
    for(int i=0;i<n;++i)if(cell_slot[e][i]>=0 && cells[e][i].x<256 && cells[e][i].x+cells[e][i].size>0 &&
        cells[e][i].y<224 && cells[e][i].y+cells[e][i].size>0)anchored=1;
    WideNativePose *w=&s_native_pose[e];
    if(anchored && LagoonSceneRenderer_ExactRoom(ram) && LagoonSceneRenderer_ExactRoom(g_ram) && pose_centered(ram,a) && atlas_ok && cells_owned(cells[e],n) &&
        pose_record_equal(ram+a,g_ram+a)) {
      memcpy(w->record,ram+a,64);memcpy(w->atlas,PpuRenderVram(g_ppu)+0x4400,sizeof(w->atlas));
      memcpy(w->palette,g_ppu->cgram,sizeof(w->palette));w->generation=s_frame.generation;
      w->room=room;w->source=ram_pointer(ram,0x5d);w->width=ram_word(ram,0x53);w->height=ram_word(ram,0x55);
      w->dp90=ram_word(ram,0x90);w->valid=1;
    }else if(!pose_centered(ram,a))w->valid=0;
    LagoonWideAnimation astate={ram[a+0x10],ram[a+0x11]};uint8_t expected;
    if(LagoonWideEntities_FrameForContext(g_rom,kRomSize,ram,ram+a,&astate,0,&expected)) {
      int centered=!additional ||
        (ram_word(ram,a+12)==((ram_word(ram,a+2)-ram_word(ram,0x480))&65535) &&
         ram_word(ram,a+14)==((ram_word(ram,a+4)-ram_word(ram,0x482))&65535));
      if(expected==ram[a+0x1c] && centered && atlas_ok && cells_owned(cells[e],n)) {
        if(s_last_serial!=s_frame.serial) s_frame_witness++;
        s_kind_witness[type-1][ram[a+0x1b]]=1;
      } else if(expected!=ram[a+0x1c] || !centered) {
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
    if(queued[e] || !atlas_ok || !LagoonWideEntities_AllowedKind(room,type,kind))continue;
    int ordinary=(type==1||type==2) && kind<(type==1?128:64) && s_pose[e].valid && s_kind_witness[type-1][kind];
    int held=native_pose_matches(ram,e);
    int x=signed_word(ram_word(ram,a+2)-ram_word(ram,0x480));
    int y=signed_word(ram_word(ram,a+4)-ram_word(ram,0x482));
    if((x>=-96 && x<352) || y < -32 || y>=288) continue;
    uint8_t frame;int used_current=0;
    if(!ordinary || !LagoonWideEntities_FrameForContext(g_rom,kRomSize,ram,ram+a,&s_pose[e].animation,0,&frame)) {
      uint8_t current_frame;int current=!held&&current_pose_frame(ram,e,&current_frame);
      if(!held&&!current)continue;
      used_current=current;frame=current?current_frame:ram[a+0x1c]; /* Exact current guest pose; no attack/AI extrapolation. */
    }
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
    counts[e]=n;far[e]=1;far_frames[e]=frame;current_path[e]=used_current;any_far=1;s_last_far_entity=e*64;s_last_far_x=x;s_last_far_frame=frame;
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
        fprintf(cell_report,"host_frame\tserial\tclock\tentity\ttype\tkind\tframe\tx\ty\ttile\tsize\tcurrent_record\n");
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
          s_far_emitted+=added;s_has_far|=added;if(current_path[e])s_current_emitted+=added;
          if(added && cell_report) {
            unsigned a=0x500+e*64;uint8_t frame=far_frames[e];
            fprintf(cell_report,"%d\t%u\t%u\t%03X\t%u\t%u\t%u\t%d\t%d\t%04X\t%u\t%u\n",
                snes_frame_counter,s_frame.serial,ram[0x408],e*64,ram[a+0x18],ram[a+0x1b],frame,
                cells[e][i].x,cells[e][i].y,cells[e][i].tile,cells[e][i].size,current_path[e]);
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
