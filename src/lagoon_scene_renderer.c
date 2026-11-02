/* Bounded independent scene drawing. Guest simulation, raster fallback and the
 * reference field remain authoritative. No framebuffer enters the packet. */
#include "lagoon_scene_renderer.h"
#include "lagoon_wide_entities.h"
#include "snes/ppu.h"
#include "sha256.h"
#include "crc32.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <wchar.h>
/* These two public Win32 ABI declarations keep the renderer independent of
 * windows.h's unrelated extern-inline TEB/fiber definitions under MinGW. */
__declspec(dllimport) int __stdcall MultiByteToWideChar(unsigned int,unsigned long,const char *,int,wchar_t *,int);
__declspec(dllimport) int __stdcall MoveFileExW(const wchar_t *,const wchar_t *,unsigned long);
enum { ARCHIVE_CP_UTF8=65001,ARCHIVE_MB_ERR_INVALID_CHARS=8,
       ARCHIVE_MOVEFILE_REPLACE_EXISTING=1,ARCHIVE_MOVEFILE_WRITE_THROUGH=8 };
#endif

enum { W=800,H=224,OBJECTS=512,OBJECT_SIDE=48 };
typedef struct SceneObject {
  int32_t x,y;
  uint16_t width,height,pixels[OBJECT_SIDE*OBJECT_SIDE];
  uint8_t priority,slot;
  uint32_t order;
} SceneObject;
typedef struct SceneOamPolicy {
  int16_t x[128];
  uint8_t allow[128],clip_left[128];
} SceneOamPolicy;
typedef struct SceneOamPolicyInputs {
  uint16_t left_extra,right_extra;
  uint8_t left_strict,right_strict,motion_on;
  uint8_t left[16],right[16],grace[128];
  int edge_clip;
} SceneOamPolicyInputs;
typedef struct ScenePacket {
  uint8_t regs[PPU_SAVESTATE_REGS_SIZE],maps[2][16384];
  uint16_t defs[2][1024],vram[32768],pal[256],oam[256];
  uint8_t high[32];
  SceneObject objects[OBJECTS];
  unsigned count,lines,frame,serial,valid,room;
  uint32_t vram_writes,oam_writes;
  int wx,wy,world_width,world_height;
  unsigned map_fill,fill_meta;
  unsigned native_x,native_y,bg2_x,bg2_y,parallax,finite_clip;
  unsigned incoming_mosaic,mosaic_size,brightness,mosaic_y_phase,mosaic_backend,mosaic_y_size;
  SceneOamPolicy policy;
  SceneOamPolicyInputs policy_inputs;
  uint64_t generation;
} ScenePacket;
static ScenePacket s_packets[2];
static unsigned s_build,s_complete=1,s_serial,s_cache_serial,s_cache_width;
static int s_mode=2;
static int s_edge_clip=1;
static char s_report[2048];
static const char *s_status="reset";
static unsigned s_report_frame,s_report_width;
static int s_exit_registered;
typedef struct SceneMismatch {
  unsigned frame,width,x,y,count,minx,miny,maxx,maxy;
  uint32_t candidate,reference;
  int wx,wy,world_width,world_height;
  unsigned map_fill,fill_meta;
} SceneMismatch;
static SceneMismatch s_mismatch[64];
static unsigned s_mismatch_count;
static uint32_t s_pixels[W*H];
/* One current-area image, not an album of extracted room art. A frozen modal
 * can reuse this image without decoding terrain on every UI presentation. */
static uint32_t s_map_pixels[1024*1024];
static unsigned s_map_valid,s_map_room,s_map_serial,s_map_width,s_map_height;
static uint64_t s_map_generation;
/* One terrain-only source, not another multi-megabyte actor packet. Keeping the
 * last fully qualified field avoids losing the map when an OAM or scroll change
 * refuses world replacement. Every refused field must independently prove the
 * same current room/generation and exact terrain bytes before this is usable. */
typedef struct SceneMapSource {
  uint8_t maps[2][16384];
  uint16_t defs[2][1024],patterns[16384],pages[2048],pal[128];
  unsigned valid,room,serial,parallax,bg2_x,bg2_y;
  int wx,wy,world_width,world_height;
  uint64_t generation;
} SceneMapSource;
static SceneMapSource s_map_source;
static unsigned s_map_identity_valid,s_map_identity_lines,s_map_identity_room;
static unsigned s_map_identity_seen;
static uint64_t s_map_identity_generation;
static int archive_remember(const SceneMapSource *source);
/* Source-decoded BG1/BG2 only. Moving objects still compose afresh, and the
 * normal whole-field reference comparison still runs before presentation.
 * Keep complete input bytes: neither a room/resource label nor a hash grants
 * reuse after an upload, camera change, mosaic change or new generation. */
typedef struct SceneBackgroundCache {
  uint8_t regs[PPU_SAVESTATE_REGS_SIZE],maps[2][16384];
  uint16_t defs[2][1024],vram[32768],pal[256];
  uint8_t depth[W*H];
  uint16_t colors[W*H];
  unsigned valid,width,native_x,native_y,bg2_x,bg2_y,parallax,finite_clip;
  unsigned map_fill,fill_meta;
  unsigned incoming_mosaic,mosaic_size,brightness,mosaic_y_phase,mosaic_backend,mosaic_y_size;
  int wx,wy,world_width,world_height;
  uint64_t generation;
} SceneBackgroundCache;
static SceneBackgroundCache s_background;
static uint64_t s_background_builds,s_background_reuses;
static LagoonSceneRendererStats s_stats;
static uint64_t s_mosaic_verified,s_mosaic_replaced;
/* Immutable packet serials key source-only decoding. A row packs eight palette
 * indices into bytes; mirrored samples select another byte without redecoding.
 * These caches never contain final/reference framebuffer pixels. */
static uint64_t s_plane_rows[2][32768],s_spread[256];
static unsigned s_plane_keys[2][32768];
static uint32_t s_rgb15[32768];
static int s_tables_ready;
static uint32_t rgb(unsigned c) {
  unsigned r=c&31,g=(c>>5)&31,b=(c>>10)&31;
  return (((r<<3)|(r>>2))<<16)|(((g<<3)|(g>>2))<<8)|(b<<3)|(b>>2);
}
static void init_tables(void) {
  if(s_tables_ready)return;
  for(unsigned i=0;i<256;++i)for(unsigned x=0;x<8;++x)
    s_spread[i]|=(uint64_t)((i>>(7-x))&1)<<(x*8);
  for(unsigned i=0;i<32768;++i)s_rgb15[i]=rgb(i);
  s_tables_ready=1;
}

static unsigned word(const uint8_t *r,unsigned a){return r[a]|r[a+1]<<8;}
static unsigned ptr(const uint8_t *r,unsigned a){return word(r,a)|r[a+2]<<16;}
typedef struct AdditionalFamily { unsigned bg,obj,definitions; } AdditionalFamily;
typedef struct AdditionalRoom { unsigned room,width,height,family,flags,page0,page1; } AdditionalRoom;
static const AdditionalFamily s_additional_families[]={
  {0x15a43d,0x1b84d8,0x078c00},
  {0x15a43d,0x1ba17d,0x0794dc},
  {0x15b8ac,0x1ba17d,0x079c14},
  {0x15c815,0x1cc387,0x07a0d8},
  {0x13d745,0x1db67e,0x06d090},
  {0x13d745,0x1b8000,0x06d090},
  {0x12e794,0x1db67e,0x06d984},
  {0x14abbe,0x1a94fe,0x06cb0c},
  {0x14c061,0x1aaed9,0x06dc14},
  {0x14c061,0x19af50,0x06dc14},
  {0x14c061,0x1aed4a,0x06dc14},
  {0x14d33d,0x1abf2d,0x06e2f8},
  {0x12b297,0x1aaed9,0x06999c},
  {0x12ee12,0x1dc859,0x06e5ac},
  {0x149197,0x18afb0,0x06b3c4},
  {0x12b297,0x199f6c,0x06999c},
  {0x13a421,0x1d9b77,0x06a310},
  {0x12da6c,0x1d9b77,0x06a97c},
  {0x148000,0x1c8000,0x06ad44},
  {0x12da6c,0x18a0b2,0x06a97c},
  {0x12da6c,0x199f6c,0x06a97c},
  {0x168000,0x188000,0x06b874},
  {0x13b55d,0x188000,0x06bc74},
  {0x13c716,0x1c92f5,0x06c294},
  {0x14a1bc,0x1c92f5,0x06c880},
  {0x13c716,0x1cada9,0x06c294},
  {0x14de0a,0x1adbb7,0x06ec64},
  {0x13f001,0x1cb607,0x06f20c},
  {0x158000,0x1cb7f8,0x07848c},
  {0x158000,0x1dd5c7,0x07848c},
  {0x14edd5,0x1cb7f8,0x06f694},
  {0x15918d,0x1cb7f8,0x078860},
  {0x15d7e5,0x1bb982,0x07a5a0},
  {0x15d7e5,0x1aed4a,0x07a5a0},
  {0x168ed1,0x199f6c,0x07ab38},
  {0x15e7fb,0x1cd41d,0x07add8},
  {0x138000,0x198000,0x068000},
  {0x128b77,0x19c637,0x069298},
  {0x148000,0x18c636,0x06ad44},
  {0x13c716,0x19e0dd,0x06c294},
  {0x14c061,0x1bd63d,0x06dc14},
  {0x14de0a,0x1ceac1,0x06ec64},
  {0x15f529,0x1be46b,0x06fc3c},
  {0x15f529,0x18e0a7,0x06fc3c},
  {0x158000,0x19fd1d,0x07848c},
  {0x15a43d,0x1cfa11,0x0794dc},
  {0x15c815,0x1cada9,0x07a0d8}
};
/* Exact original descriptors, not authority inferred from shared scenery. */
static const AdditionalRoom s_additional_rooms[]={
  {0x2c,256,192,0},{0x2d,128,64,0},{0x2e,128,64,0},
  {0x2f,256,192,1},{0x30,64,64,1},{0x31,64,64,1},
  {0x32,256,192,2},{0x33,96,64,2},{0x34,128,64,2},
  {0x35,256,192,3},{0x36,96,64,3},{0x37,96,64,3},
  {0x19,128,128,4},{0x1a,32,32,4},{0x1b,32,64,5},{0x1c,96,128,6},
  {0x18,192,224,7},{0x1d,128,128,7},{0x1e,128,128,8},
  {0x1f,64,64,8},{0x20,128,128,9},{0x21,64,64,10},
  {0x22,128,128,11},{0x23,64,64,12},{0x24,96,96,13},{0x25,64,64,14},
  {0x09,128,128,15,0,0x000000,0x000000},
  {0x0a,128,128,16,0,0x000000,0x000000},
  {0x0b,256,256,17,0,0x000000,0x000000},
  {0x0c,192,224,18,1,0x14fb7f,0x169cf6},
  {0x0d,256,256,19,0,0x000000,0x000000},
  {0x0e,32,32,20,0,0x000000,0x000000},
  {0x0f,32,32,20,0,0x000000,0x000000},
  {0x10,256,256,14,0,0x000000,0x000000},
  {0x11,128,160,21,0,0x000000,0x000000},
  {0x12,64,256,22,1,0x13f633,0x13fa19},
  {0x13,64,256,22,1,0x13f633,0x13fa19},
  {0x14,128,256,23,0,0x000000,0x000000},
  {0x15,128,128,23,0,0x000000,0x000000},
  {0x16,64,64,24,0,0x000000,0x000000},
  {0x17,96,96,25,0,0x000000,0x000000},
  {0x26,256,256,26,0,0x000000,0x000000},
  {0x27,64,96,27,0,0x000000,0x000000},
  {0x28,224,256,28,1,0x14f82e,0x12fe89},
  {0x29,32,64,29,1,0x14f82e,0x12fe89},
  {0x2a,64,160,30,1,0x13fd84,0x11fe76},
  {0x2b,128,160,31,0,0x000000,0x000000},
  {0x38,224,160,32,0,0x000000,0x000000},
  {0x39,128,96,32,0,0x000000,0x000000},
  {0x3a,128,64,33,0,0x000000,0x000000},
  {0x3b,32,32,32,0,0x000000,0x000000},
  {0x3c,96,64,34,0,0x000000,0x000000},
  {0x3d,32,64,34,0,0x000000,0x000000},
  {0x3e,256,256,35,0,0x000000,0x000000},
  {0x3f,192,192,36,0,0x000000,0x000000},
  {0x40,32,32,37,0,0x000000,0x000000},
  {0x41,32,32,38,0,0x000000,0x000000},
  {0x42,32,32,39,0,0x000000,0x000000},
  {0x43,32,32,40,0,0x000000,0x000000},
  {0x44,32,64,41,0,0x000000,0x000000},
  {0x45,32,32,33,0,0x000000,0x000000},
  {0x46,32,32,42,0,0x000000,0x000000},
  {0x47,32,32,43,0,0x000000,0x000000},
  {0x48,32,32,44,0,0x000000,0x000000},
  {0x49,32,32,45,0,0x000000,0x000000},
  {0x4a,32,32,46,0,0x000000,0x000000}
};
int LagoonSceneRenderer_AdditionalRoom(const uint8_t *r) {
  if(!r||ptr(r,0x88)!=0x7f0000||ptr(r,0x8b)!=0x7f4000)return 0;
  for(unsigned i=0;i<sizeof(s_additional_rooms)/sizeof(s_additional_rooms[0]);++i) {
    const AdditionalRoom *room=&s_additional_rooms[i];
    if(word(r,0x48e)!=room->room||word(r,0x53)!=room->width||word(r,0x55)!=room->height||r[0x50]!=room->flags)continue;
    const AdditionalFamily *family=&s_additional_families[room->family];
    return ptr(r,0x57)==family->bg&&ptr(r,0x5d)==family->obj&&
      ptr(r,0x69)==family->definitions&&ptr(r,0x75)==family->definitions&&
      (!room->flags||(ptr(r,0x7e)==room->page0&&ptr(r,0x81)==room->page1));
  }
  return 0;
}
int LagoonSceneRenderer_ParallaxRoom(const uint8_t *r) {
  if(!r)return 0;
  if(r[0x50]==1)return LagoonSceneRenderer_AdditionalRoom(r);
  return word(r,0x48e)==7 && r[0x50]==2 && word(r,0x53)==128 && word(r,0x55)==128 &&
    ptr(r,0x57)==0x129e89 && ptr(r,0x5d)==0x1a906f &&
    ptr(r,0x69)==0x069720 && ptr(r,0x75)==0x069720 &&
    ptr(r,0x88)==0x7f0000 && ptr(r,0x8b)==0x7f4000 &&
    ptr(r,0x7e)==0x11fb7d && ptr(r,0x81)==0x11fb7d;
}
/* Exact ordinary scene resources, not permission for arbitrary Mode1 rooms.
 * Both maps use the same immutable ROM definition bank within each descriptor. */
static int scene_resources(const uint8_t *r,unsigned *definitions) {
  if(!r || ptr(r,0x88)!=0x7f0000 || ptr(r,0x8b)!=0x7f4000 ||
      ptr(r,0x69)!=ptr(r,0x75))return 0;
  unsigned room=word(r,0x48e),d=ptr(r,0x69);
  if(room==0 && ptr(r,0x57)==0x138000 && ptr(r,0x5d)==0x198000 && d==0x068000) {
    if(!word(r,0x53)||word(r,0x53)>256||!word(r,0x55)||word(r,0x55)>256)return 0;
  }else if((room==1&&word(r,0x53)==64&&word(r,0x55)==96&&ptr(r,0x57)==0x139a7a&&ptr(r,0x5d)==0x1a8000&&d==0x0689e0)||
      (room==2&&word(r,0x53)==64&&word(r,0x55)==64&&ptr(r,0x57)==0x128000&&ptr(r,0x5d)==0x1a8000&&d==0x068e90)) {
    /* Preserve the existing widescreen/fade policy; qualify stable custom pixels. */
  }else if(LagoonSceneRenderer_AdditionalRoom(r)) {
    /* Existing character/PPU/raster ownership must still qualify this packet. */
  }else if(room==8 && word(r,0x53)==192 && word(r,0x55)==128 &&
      ptr(r,0x57)==0x12b297 && ptr(r,0x5d)==0x1891e4 && d==0x06999c) {
    /* Elf Field's ordinary map producer has an exact finite1536x1024 extent.
     * Its animated characters still require the existing source-owned witness. */
  }else if(room==7 && word(r,0x53)==128 && word(r,0x55)==128 &&
      ptr(r,0x57)==0x129e89 && ptr(r,0x5d)==0x1a906f && d==0x069720 &&
      ptr(r,0x7e)==0x11fb7d && ptr(r,0x81)==0x11fb7d) {
    /* BG2 is a separately owned static32x64 page pair, not this WRAM map. */
  }else {
    /* Each finite room is admitted independently; sharing art does not grant
     * permission to arbitrary rooms, map dimensions or loading/fading fields. */
    static const struct { unsigned room,width,height; } caves[]={
      {3,224,204},{4,96,32},{5,192,160},{6,64,32}
    };
    unsigned match=0;
    for(unsigned i=0;i<sizeof(caves)/sizeof(caves[0]);++i)
      if(room==caves[i].room && word(r,0x53)==caves[i].width &&
          word(r,0x55)==caves[i].height)match=1;
    if(!match || ptr(r,0x57)!=0x128b77 || ptr(r,0x5d)!=0x1d8000 || d!=0x069298)return 0;
  }
  if(definitions)*definitions=((d>>16)&0x7f)*0x8000+(d&0x7fff);
  return 1;
}
int LagoonSceneRenderer_ExactRoom(const uint8_t *r) {
  return r && (word(r,0x48e)!=0 || (word(r,0x53)==192&&word(r,0x55)==192)) && scene_resources(r,NULL) &&
    r[0x50]==(LagoonSceneRenderer_ParallaxRoom(r)?(word(r,0x48e)==7?2:1):0);
}
static void write_report(void) {
  if(!s_report[0])return;
  FILE *f=fopen(s_report,"w");if(!f)return;
  fprintf(f,"{\"mode\":\"%s\",\"status\":\"%s\",\"frame\":%u,\"width\":%u,"
    "\"packets\":%llu,\"refused_packets\":%llu,\"renders\":%llu,\"independent_pixels\":%llu,"
    "\"verified_presents\":%llu,\"replaced_presents\":%llu,\"mismatches\":%llu,\"fallbacks\":%llu,\"mismatch_details\":[",
    s_mode==2?"auto":s_mode?"verify":"off",s_status,s_report_frame,s_report_width,
    (unsigned long long)s_stats.packets,(unsigned long long)s_stats.refused_packets,
    (unsigned long long)s_stats.renders,(unsigned long long)s_stats.independent_pixels,
    (unsigned long long)s_stats.verified_presents,(unsigned long long)s_stats.replaced_presents,
    (unsigned long long)s_stats.mismatches,(unsigned long long)s_stats.fallbacks);
  for(unsigned i=0;i<s_mismatch_count;++i) {
    const SceneMismatch *m=&s_mismatch[i];
    fprintf(f,"%s{\"frame\":%u,\"width\":%u,\"x\":%u,\"y\":%u,\"candidate\":%u,\"reference\":%u,\"count\":%u,"
      "\"minx\":%u,\"maxx\":%u,\"miny\":%u,\"maxy\":%u,\"wx\":%d,\"wy\":%d,\"world_width\":%d,\"world_height\":%d}",
      i?",":"",m->frame,m->width,m->x,m->y,m->candidate,m->reference,m->count,
      m->minx,m->maxx,m->miny,m->maxy,m->wx,m->wy,m->world_width,m->world_height);
  }
  fprintf(f,"],\"mosaic_native_verified_presents\":%llu,\"mosaic_margin_presents\":%llu,"
      "\"background_builds\":%llu,\"background_reuses\":%llu}\n",
      (unsigned long long)s_mosaic_verified,(unsigned long long)s_mosaic_replaced,
      (unsigned long long)s_background_builds,(unsigned long long)s_background_reuses);
  fclose(f);
}
/* Diagnostics never flush from a frame/present/audio-sensitive path. */
static void report(const char *status,unsigned frame,unsigned width) {
  s_status=status;s_report_frame=frame;s_report_width=width;
}
void LagoonSceneRenderer_Reset(void) {
  init_tables();
  memset(s_packets,0,sizeof(s_packets));s_build=0;s_complete=1;
  s_background.valid=0;
  s_map_valid=s_map_source.valid=s_map_identity_valid=s_map_identity_lines=s_map_identity_seen=0;
  s_cache_serial=0;s_cache_width=0;++s_stats.resets;
  const char *m=getenv("LAGOON_SCENE_RENDERER");
  s_mode=m&&!strcmp(m,"off")?0:!m||!strcmp(m,"auto")?2:1;
  const char *clip=getenv("SNESRECOMP_WS_OBJ_EDGE_CLIP");s_edge_clip=!clip||!*clip||*clip!='0';
  const char *p=getenv("LAGOON_SCENE_RENDERER_REPORT");
  snprintf(s_report,sizeof(s_report),"%s",p?p:"");
  if(!s_exit_registered){atexit(write_report);s_exit_registered=1;}
  report("reset",0,0);
}
static int parallax_raster(const Ppu *p,const uint8_t *r) {
  unsigned camera[4]={word(r,0x480),word(r,0x482),word(r,0x484),word(r,0x486)};
  unsigned raster[4]={p->hScroll[0]&1023,p->vScroll[0]&1023,p->hScroll[1]&1023,p->vScroll[1]&1023};
  return LagoonWideEntities_ParallaxRaster(camera,raster);
}
static int mosaic_vertical_table(const Ppu *p,unsigned size) {
  if(size<1||size>16)return 0;
  for(unsigned y=1;y<=H;++y)if(p->mosaicModulo[y]!=(y/size)*size)return 0;
  return 1;
}
static int supported(const Ppu *p,const uint8_t *r,const uint8_t *rom,size_t size,int authority) {
  int parallax=LagoonSceneRenderer_ParallaxRoom(r);
  int incoming=authority==LAGOON_SCENE_INCOMING_MOSAIC;
  return p&&authority&&r&&rom&&size==0x100000&&scene_resources(r,NULL)&&
    (incoming || !r[0x100e])&&r[0x50]==(parallax?(word(r,0x48e)==7?2:1):0)&&word(r,0x48c)<=64&&
    word(r,0x53)>0&&word(r,0x53)<=256&&word(r,0x55)>0&&word(r,0x55)<=256&&
    (incoming?(p->inidisp>0&&p->inidisp<=15&&(p->mosaic&15)==15&&!p->extraObjectCount&&
       ((p->renderFlags&kPpuRenderFlags_NewRenderer)?(p->lastMosaicModulo>=1&&p->lastMosaicModulo<=16):p->mosaicStartLine<=1)):
      (authority==LAGOON_SCENE_WORLD&&p->inidisp==15&&!p->mosaic))&&p->bgmode==9&&p->obsel==2&&!p->setini&&
    p->bgXsc[0]==0x71&&p->bgXsc[1]==(parallax?0x7a:0x79)&&p->bgXsc[2]==0x3c&&p->bgTileAdr==0x600&&
    !p->cgadsub&&!(p->cgwsel&1)&&!p->windowsel&&!p->screenWindowed[0]&&!p->screenWindowed[1]&&
    p->screenEnabled[0]==31&&
    (parallax?(LagoonWideEntities_ParallaxRoomScroll(word(r,0x48e),word(r,0x480),word(r,0x482),word(r,0x484),word(r,0x486))&&
      parallax_raster(p,r)&&
      LagoonWideEntities_ParallaxMapForRoom(PpuRenderVram(p),rom,size,word(r,0x48e))):
      (p->hScroll[0]==p->hScroll[1]&&p->vScroll[0]==p->vScroll[1]))&&
    !p->hScroll[2]&&!p->vScroll[2]&&!(p->oamaddh&0x80)&&p->extraObjectCount<=OBJECTS&&
    (!p->extraObjectCount||p->extraObjects);
}
/* This grants only reuse of immutable terrain already qualified by supported().
 * It never authorizes a new world packet, actor, raster or framebuffer. Ignore
 * OAM and the camera's partial circular tilemap; neither is used in the map. */
static int map_identity_matches(const Ppu *p,const uint8_t *r,const uint8_t *rom,
    size_t size,uint64_t generation,int authority) {
  const SceneMapSource *m=&s_map_source;unsigned definitions=0;
  if(!m->valid||!p||!r||!rom||size!=0x100000||authority!=LAGOON_SCENE_WORLD||
      generation!=m->generation||word(r,0x48e)!=m->room||
      !LagoonSceneRenderer_ExactRoom(r)||!scene_resources(r,&definitions)||
      word(r,0x53)*8!=(unsigned)m->world_width||word(r,0x55)*8!=(unsigned)m->world_height||
      r[0x100e]||p->inidisp!=15||p->mosaic||p->bgmode!=9||p->obsel!=2||p->setini||
      p->bgXsc[0]!=0x71||p->bgXsc[1]!=(m->parallax?0x7a:0x79)||
      p->bgXsc[2]!=0x3c||p->bgTileAdr!=0x600||p->cgadsub||(p->cgwsel&1)||
      p->windowsel||p->screenWindowed[0]||p->screenWindowed[1]||p->screenEnabled[0]!=31||
      p->hScroll[2]||p->vScroll[2]||
      (unsigned)LagoonSceneRenderer_ParallaxRoom(r)!=m->parallax)return 0;
  const uint16_t *v=PpuRenderVram(p);
  return !memcmp(m->maps[0],r+0x10000,sizeof(m->maps[0]))&&
    !memcmp(m->maps[1],r+0x14000,sizeof(m->maps[1]))&&
    !memcmp(m->defs[0],rom+definitions,sizeof(m->defs[0]))&&
    !memcmp(m->defs[1],rom+definitions,sizeof(m->defs[1]))&&
    !memcmp(m->patterns,v,sizeof(m->patterns))&&
    !memcmp(m->pal,p->cgram,sizeof(m->pal))&&
    (!m->parallax||(!memcmp(m->pages,v+0x7800,sizeof(m->pages))&&
      LagoonWideEntities_ParallaxRoomScroll(m->room,word(r,0x480),word(r,0x482),word(r,0x484),word(r,0x486))&&
      LagoonWideEntities_ParallaxMapForRoom(v,rom,size,m->room)));
}
static void map_observe_identity(const Ppu *p,unsigned line,const uint8_t *r,
    const uint8_t *rom,size_t size,uint64_t generation,int authority) {
  if(line==1) {
    s_map_identity_seen=1;
    s_map_identity_room=r?word(r,0x48e):75;s_map_identity_generation=generation;
    if(s_map_source.valid&&(s_map_source.room!=s_map_identity_room||s_map_source.generation!=generation))
      s_map_source.valid=s_map_valid=0;
    s_map_identity_lines=1;
    s_map_identity_valid=map_identity_matches(p,r,rom,size,generation,authority);
  }else if(s_map_identity_valid) {
    if(!p||!r||line!=s_map_identity_lines+1||generation!=s_map_identity_generation||
        word(r,0x48e)!=s_map_identity_room||authority!=LAGOON_SCENE_WORLD)s_map_identity_valid=0;
    else {
      s_map_identity_lines=line;
      if(line==H&&!map_identity_matches(p,r,rom,size,generation,authority))s_map_identity_valid=0;
    }
  }
}
static void remember_map_source(const ScenePacket *s) {
  SceneMapSource *m=&s_map_source;
  memcpy(m->maps,s->maps,sizeof(m->maps));memcpy(m->defs,s->defs,sizeof(m->defs));
  memcpy(m->patterns,s->vram,sizeof(m->patterns));
  if(s->parallax)memcpy(m->pages,s->vram+0x7800,sizeof(m->pages));
  memcpy(m->pal,s->pal,sizeof(m->pal));
  m->valid=1;m->room=s->room;m->generation=s->generation;m->serial=s->serial;
  m->parallax=s->parallax;m->bg2_x=s->bg2_x;m->bg2_y=s->bg2_y;
  m->wx=s->wx;m->wy=s->wy;m->world_width=s->world_width;m->world_height=s->world_height;
}
static int objects_equal(const ScenePacket *s,const Ppu *p) {
  if(s->count!=p->extraObjectCount||(s->count&&!p->extraObjects))return 0;
  for(unsigned i=0;i<s->count;++i) {
    const SceneObject *a=&s->objects[i];const PpuExtraObject *b=&p->extraObjects[i];
    if(!b->pixels||a->x!=b->x||a->y!=b->y||a->width!=b->width||a->height!=b->height||
      a->priority!=b->priority||a->slot!=b->oamSlot||a->order!=b->order||b->stride<b->width)return 0;
    for(unsigned y=0;y<a->height;++y)if(memcmp(a->pixels+y*a->width,b->pixels+y*b->stride,a->width*2))return 0;
  }return 1;
}
/* Copy the existing PPU's current OAM placement policy. This is presentation
 * metadata captured after the real first-line evaluator, not another motion
 * heuristic and not permission to unpark arbitrary nine-bit X coordinates. */
static void object_policy(SceneOamPolicy *s,const Ppu *p) {
  const uint8_t *o=(const uint8_t *)p->oam;
  for(unsigned slot=0;slot<128;++slot) {
    unsigned high=(p->highOam[slot/4]>>((slot%4)*2))&3,n=(high&2)?16:8;
    int x=o[slot*4]|((high&1)<<8);
    int right=(p->wsOamRightHint[slot/8]>>(slot&7))&1;
    int left=(p->wsOamLeftHint[slot/8]>>(slot&7))&1;
    int grace=p->wsOamMotionGraceOn&&p->wsOamMotionGrace[slot];
    if(x>=256+p->extraRightCur||(x>=256&&p->wsOamRightHintStrict&&!right))x-=512;
    s->x[slot]=(int16_t)x;
    s->allow[slot]=!p->wsOamLeftHintStrict||x>=0||x+(int)n>0||
      x+(int)n<=-p->extraLeftCur||left||grace;
    s->clip_left[slot]=s_edge_clip&&p->wsOamLeftHintStrict&&!left&&!grace;
  }
}
/* OAM bytes are checked independently on every raster line. Its placement
 * policy also uses these exact host inputs, normally constant for the field.
 * Equality permits reusing only that pure derivation. A changed input still
 * runs the original policy comparison, including changes with no visible
 * effect; it never acquires or refuses a packet merely from this fingerprint. */
static void object_policy_inputs(SceneOamPolicyInputs *s,const Ppu *p) {
  s->left_extra=p->extraLeftCur;s->right_extra=p->extraRightCur;
  s->left_strict=p->wsOamLeftHintStrict;s->right_strict=p->wsOamRightHintStrict;
  s->motion_on=p->wsOamMotionGraceOn;s->edge_clip=s_edge_clip;
  memcpy(s->left,p->wsOamLeftHint,sizeof(s->left));
  memcpy(s->right,p->wsOamRightHint,sizeof(s->right));
  memcpy(s->grace,p->wsOamMotionGrace,sizeof(s->grace));
}
static int object_policy_inputs_equal(const SceneOamPolicyInputs *s,const Ppu *p) {
  return s->left_extra==p->extraLeftCur&&s->right_extra==p->extraRightCur&&
    s->left_strict==p->wsOamLeftHintStrict&&s->right_strict==p->wsOamRightHintStrict&&
    s->motion_on==p->wsOamMotionGraceOn&&s->edge_clip==s_edge_clip&&
    !memcmp(s->left,p->wsOamLeftHint,sizeof(s->left))&&
    !memcmp(s->right,p->wsOamRightHint,sizeof(s->right))&&
    (!s->motion_on||!memcmp(s->grace,p->wsOamMotionGrace,sizeof(s->grace)));
}
void LagoonSceneRenderer_ObserveRaster(const Ppu *p,unsigned line,const uint8_t *r,
    const uint8_t *rom,size_t size,uint64_t generation,int authority) {
  if(!s_mode||line<1||line>H)return;
  map_observe_identity(p,line,r,rom,size,generation,authority);
  ScenePacket *s=&s_packets[s_build];
  if(line==1) {
    s->valid=0;s->lines=0;s->generation=generation;
    if(!p||!supported(p,r,rom,size,authority))return;
    s->valid=1;s->lines=1;s->room=word(r,0x48e);
    s->incoming_mosaic=authority==LAGOON_SCENE_INCOMING_MOSAIC;
    s->mosaic_size=PPU_mosaicSize(p);s->brightness=p->inidisp&15;
    s->mosaic_backend=!!(p->renderFlags&kPpuRenderFlags_NewRenderer);
    s->mosaic_y_phase=s->mosaic_backend?0:p->mosaicStartLine;
    s->mosaic_y_size=s->mosaic_backend&&s->mosaic_size>1?p->lastMosaicModulo:s->mosaic_size;
    /* NMI may replace MOSAIC after line0 initialized its vertical table.
     * Keep the actual validated table's size, not the newer register size. */
    if(s->incoming_mosaic&&s->mosaic_backend&&s->mosaic_size>1&&
        !mosaic_vertical_table(p,s->mosaic_y_size)){s->valid=0;return;}
    memcpy(s->regs,&p->inidisp,sizeof(s->regs));
    memcpy(s->maps[0],r+0x10000,16384);memcpy(s->maps[1],r+0x14000,16384);
    unsigned definitions;
    if(!scene_resources(r,&definitions)){s->valid=0;return;}
    memcpy(s->defs[0],rom+definitions,2048);memcpy(s->defs[1],rom+definitions,2048);
    memcpy(s->vram,PpuRenderVram(p),sizeof(s->vram));memcpy(s->pal,p->cgram,sizeof(s->pal));
    memcpy(s->oam,p->oam,sizeof(s->oam));memcpy(s->high,p->highOam,sizeof(s->high));
    object_policy(&s->policy,p);
    object_policy_inputs(&s->policy_inputs,p);
    s->vram_writes=p->vramWriteCount;s->oam_writes=p->oamWriteCount;
    int x=word(r,0x480),y=word(r,0x482);
    s->wx=x+(((p->hScroll[0]&1023)-x+512)&1023)-512;
    s->wy=y+(((p->vScroll[0]&1023)-y+512)&1023)-512+1;
    s->native_x=p->hScroll[0];s->native_y=p->vScroll[0]+1;
    s->parallax=LagoonSceneRenderer_ParallaxRoom(r);s->finite_clip=word(r,0x48e)==1||word(r,0x48e)==2||word(r,0x48e)==8||LagoonSceneRenderer_AdditionalRoom(r);s->bg2_x=p->hScroll[1];s->bg2_y=p->vScroll[1]+1;
    s->world_width=word(r,0x53)*8;s->world_height=word(r,0x55)*8;
    /* Atland's ordinary widened shadow repeats its original833E fill tile
     * beyond signed map zero. Finite rooms and mosaic keep their own policy. */
    s->map_fill=!s->incoming_mosaic&&word(r,0x48e)==0&&word(r,0x53)==192&&word(r,0x55)==192;
    s->fill_meta=r[0x52];
    s->count=(unsigned)p->extraObjectCount;
    for(unsigned i=0;i<s->count;++i) {
      const PpuExtraObject *b=&p->extraObjects[i];SceneObject *a=&s->objects[i];
      if(!b->pixels||!b->width||!b->height||b->width>OBJECT_SIDE||b->height>OBJECT_SIDE||b->stride<b->width||b->priority>3||b->oamSlot>127||!b->order){s->valid=0;break;}
      a->x=b->x;a->y=b->y;a->width=b->width;a->height=b->height;
      a->priority=b->priority;a->slot=b->oamSlot;a->order=b->order;
      for(unsigned yy=0;yy<b->height;++yy)memcpy(a->pixels+yy*b->width,b->pixels+yy*b->stride,b->width*2);
    }
  }else if(s->valid) {
    if(!p||!authority||(authority==LAGOON_SCENE_INCOMING_MOSAIC)!=s->incoming_mosaic||generation!=s->generation||line!=s->lines+1||
      (s->incoming_mosaic&&(s->mosaic_backend!=!!(p->renderFlags&kPpuRenderFlags_NewRenderer)||
        (s->mosaic_backend&&s->mosaic_size>1&&s->mosaic_y_size!=p->lastMosaicModulo)||
        (s->mosaic_backend&&s->mosaic_size>1&&
          (p->mosaicModulo[line]!=(line/s->mosaic_y_size)*s->mosaic_y_size||
           (line==H&&!mosaic_vertical_table(p,s->mosaic_y_size))))||
        (!s->mosaic_backend&&s->mosaic_y_phase!=p->mosaicStartLine)))||
      memcmp(s->regs,&p->inidisp,sizeof(s->regs))||p->vramWriteCount!=s->vram_writes||
      p->oamWriteCount!=s->oam_writes||memcmp(s->pal,p->cgram,sizeof(s->pal))||
      memcmp(s->oam,p->oam,sizeof(s->oam))||memcmp(s->high,p->highOam,sizeof(s->high)))s->valid=0;
    else {
      if(!object_policy_inputs_equal(&s->policy_inputs,p)) {
        SceneOamPolicy policy;object_policy(&policy,p);
        if(memcmp(&policy,&s->policy,sizeof(policy))){s->valid=0;return;}
      }
      s->lines=line;if(line==H&&
        (memcmp(s->vram,PpuRenderVram(p),sizeof(s->vram))||!objects_equal(s,p)))s->valid=0;
    }
  }
}
void LagoonSceneRenderer_EndFrame(unsigned frame,uint64_t generation,int authority) {
  if(!s_mode)return;
  ScenePacket *s=&s_packets[s_build];
  s->valid=s->valid&&s->lines==H&&s->generation==generation&&authority&&
      ((authority==LAGOON_SCENE_INCOMING_MOSAIC)==s->incoming_mosaic);
  if(!++s_serial){memset(s_plane_keys,0,sizeof(s_plane_keys));s_serial=1;s_cache_serial=0;
    s_map_valid=s_map_source.valid=s_map_identity_valid=0;}
  s->frame=frame;s->serial=s_serial;
  if(s->valid)++s_stats.packets;else ++s_stats.refused_packets;
  if(s->valid&&!s->incoming_mosaic&&s->brightness==15&&s->room<75&&
      s->world_width>0&&s->world_width<=2048&&s->world_height>0&&s->world_height<=2048) {
    remember_map_source(s);
    s_map_identity_valid=1;s_map_identity_lines=H;
    s_map_identity_room=s->room;s_map_identity_generation=s->generation;
  }else if(!s_map_identity_seen||s_map_identity_lines!=H||s_map_identity_generation!=generation||
      authority!=LAGOON_SCENE_WORLD)s_map_identity_valid=0;
  s_map_identity_seen=0;
  s_complete=s_build;s_build^=1;s_packets[s_build].valid=0;
}
static uint64_t graphics_row_source(const uint16_t *vram,unsigned serial,unsigned tile,unsigned y,unsigned base,unsigned bpp) {
  if(tile&0x8000)y=7-y;
  unsigned adr=(base+(tile&1023)*(bpp==2?8:16)+y)&32767;
  unsigned kind=bpp==4;
  if(s_plane_keys[kind][adr]!=serial) {
    unsigned planes=vram[adr];
    uint64_t row=s_spread[planes&255]|(s_spread[planes>>8]<<1);
    if(kind){planes=vram[(adr+8)&32767];row|=(s_spread[planes&255]<<2)|(s_spread[planes>>8]<<3);}
    s_plane_rows[kind][adr]=row;s_plane_keys[kind][adr]=serial;
  }
  return s_plane_rows[kind][adr];
}
static uint64_t graphics_row(const ScenePacket *s,unsigned tile,unsigned y,unsigned base,unsigned bpp) {
  return graphics_row_source(s->vram,s->serial,tile,y,base,bpp);
}
static unsigned graphics(const ScenePacket *s,unsigned tile,unsigned x,unsigned y,unsigned base,unsigned bpp) {
  if(tile&0x4000)x=7-x;
  return (unsigned)(graphics_row(s,tile,y,base,bpp)>>(x*8))&255;
}
int LagoonSceneRenderer_MapImage(unsigned room,uint64_t generation,
    const uint32_t **pixels,unsigned *width,unsigned *height,
    unsigned *world_width,unsigned *world_height) {
  if(pixels)*pixels=NULL;
  if(width)*width=0;if(height)*height=0;
  if(world_width)*world_width=0;if(world_height)*world_height=0;
  const SceneMapSource *s=&s_map_source;
  if(!pixels||!width||!height||!s_mode||!s->valid||
      !s_map_identity_valid||s_map_identity_lines!=H||room>=75||
      s_map_identity_room!=room||s_map_identity_generation!=generation||
      s->room!=room||s->generation!=generation||
      s->world_width<=0||s->world_width>2048||
      s->world_height<=0||s->world_height>2048) {
    s_map_valid=0;return 0;
  }
  unsigned scale=s->world_width>1024||s->world_height>1024?2:1;
  unsigned w=((unsigned)s->world_width+scale-1)/scale;
  unsigned h=((unsigned)s->world_height+scale-1)/scale;
  if(!s_map_valid||s_map_room!=room||s_map_generation!=generation||
      s_map_serial!=s->serial||s_map_width!=w||s_map_height!=h) {
    for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x) {
      unsigned qx=x*scale,qy=y*scale,depth=0,color=s->pal[0];
      for(unsigned layer=0;layer<2;++layer) {
        unsigned tx=qx,ty=qy,tile;
        if(layer&&s->parallax) {
          /* A parallax scene has no single camera-independent BG2 placement.
           * Freeze the actual qualified quarter-scroll view: every point in
           * its live window samples exactly the original BG2 equation. BG1
           * stays full-size terrain; neither layer's art is stretched. */
          tx=(unsigned)((int)qx+(int)s->bg2_x-s->wx);
          ty=(unsigned)((int)qy+(int)s->bg2_y-s->wy);
          tile=s->pages[((ty>>8)&1)*0x400+((ty>>3)&31)*32+((tx>>3)&31)];
        }else {
          unsigned meta=s->maps[layer][(qy>>4)*128+(qx>>4)];
          tile=s->defs[layer][meta*4+((qx&8)>>3)+((qy&8)>>2)];
        }
        /* Match the original ordinary-world metatile uploader's BG2 priority,
         * not the camera's partially streamed circular VRAM tilemap. */
        if(layer&&!s->parallax)tile&=0xdfff;
        unsigned px=(tile&0x4000)?7-(tx&7):tx&7;
        unsigned c=(unsigned)(graphics_row_source(s->patterns,s->serial,tile,ty&7,0,4)>>(px*8))&255;
        unsigned z=((tile&0x2000)?12:8)-layer;
        if(c&&z>depth){depth=z;color=s->pal[((tile>>10)&7)*16+c];}
      }
      s_map_pixels[y*w+x]=0xff000000u|s_rgb15[color&32767];
    }
    s_map_valid=1;s_map_room=room;s_map_generation=generation;
    s_map_serial=s->serial;s_map_width=w;s_map_height=h;
  }
  *pixels=s_map_pixels;*width=w;*height=h;
  if(world_width)*world_width=(unsigned)s->world_width;
  if(world_height)*world_height=(unsigned)s->world_height;
  return 1;
}
/* The guest streams its native circular tilemap independently of retained world
 * definitions. In particular its last visible row can intentionally still carry
 * the previous upload. Match that authored history using copied tile data, not
 * pixels from the reference field. Both audited maps are 64x32 eight-pixel tiles. */
static void background_row(const ScenePacket *s,unsigned width,unsigned y,int left,
    unsigned layer,uint8_t *depth,uint16_t *colors) {
  for(unsigned x=0;x<width;) {
    int native=(int)x>=left&&(int)x<left+256;
    int static_bg2=s->parallax&&layer==1;
    int qx=static_bg2?(int)s->bg2_x+(int)x-left:
      native?(int)s->native_x+(int)x-left:s->wx+(int)x-left;
    int qy=static_bg2?(int)s->bg2_y+(int)y:
      native?(int)s->native_y+(int)y:s->wy+(int)y;
    unsigned boundary=(int)x<left?(unsigned)left:native?(unsigned)left+256:width;
    unsigned count=8-(qx&7);if(count>boundary-x)count=boundary-x;
    unsigned tile;
    if(static_bg2) {
      /* Actual BG2 anchor and32x64 layout repeat the source-owned page pair.
       * The same rule serves native and margins; no BG1 anchor or map is used. */
      tile=s->vram[0x7800+(((unsigned)qy>>8)&1)*0x400+
        (((unsigned)qy>>3)&31)*32+(((unsigned)qx>>3)&31)];
    }else if(native) {
      unsigned address=0x7000+layer*0x800+((qy>>3)&31)*32+((qx>>3)&31)+((qx&256)?0x400:0);
      tile=s->vram[address&32767];
    }else {
      if(qx<0||qy<0) {
        if(!s->map_fill){x+=count;continue;}
        /* WsShadowSetBlankTile uses map_tile(layer,-16,-16), hence quadrant0
         * of the source fill metatile, not a guessed wrapped map location. */
        tile=s->defs[layer][s->fill_meta*4];
      } else {
        if(qx>=s->world_width||qy>=s->world_height){x+=count;continue;}
        unsigned meta=s->maps[layer][(qy>>4)*128+(qx>>4)];
        tile=s->defs[layer][meta*4+((qx&8)>>3)+((qy&8)>>2)];
      }
      if(layer)tile&=0xdfff;
    }
    uint64_t row=graphics_row(s,tile,qy&7,0,4);
    unsigned palette=((tile>>10)&7)*16,z=((tile&0x2000)?12:8)-layer;
    for(unsigned dx=0;dx<count;++dx) {
      unsigned px=(qx+dx)&7;if(tile&0x4000)px=7-px;
      unsigned c=(unsigned)(row>>(px*8))&255;
      if(c&&z>depth[x+dx]){depth[x+dx]=z;colors[x+dx]=s->pal[palette+c];}
    }
    x+=count;
  }
}
/* Native-aligned mosaic blocks extend mathematically into the margins. The
 * generic PPU's clamped native mosaicModulo table is deliberately not used.
 * BG1 margins use the proven room map; static parallax BG2 uses its complete
 * authored pages. Native samples use the actual circular PPU map history. */
static int mosaic_x(int x,unsigned size) {
  int remainder=x%(int)size;if(remainder<0)remainder+=(int)size;
  return x-remainder;
}
static void mosaic_background_row(const ScenePacket *s,unsigned width,unsigned y,
    int left,unsigned layer,uint8_t *depth,uint16_t *colors) {
  unsigned my=((y+1-s->mosaic_y_phase)/s->mosaic_y_size)*s->mosaic_y_size+s->mosaic_y_phase;
  int static_bg2=s->parallax&&layer==1;
  for(unsigned x=0;x<width;++x) {
    int mx=mosaic_x((int)x-left,s->mosaic_size),native=mx>=0&&mx<256;
    int qx=(static_bg2?(int)s->bg2_x:native?(int)s->native_x:s->wx)+mx;
    int qy=(static_bg2?(int)s->bg2_y:native?(int)s->native_y:s->wy)-1+(int)my;
    unsigned tile;
    if(static_bg2)tile=s->vram[0x7800+(((unsigned)qy>>8)&1)*0x400+
        (((unsigned)qy>>3)&31)*32+(((unsigned)qx>>3)&31)];
    else if(native)tile=s->vram[(0x7000+layer*0x800+((qy>>3)&31)*32+
        ((qx>>3)&31)+((qx&256)?0x400:0))&32767];
    else {
      if(qx<0||qy<0||qx>=s->world_width||qy>=s->world_height)continue;
      unsigned meta=s->maps[layer][(qy>>4)*128+(qx>>4)];
      tile=s->defs[layer][meta*4+((qx&8)>>3)+((qy&8)>>2)];
      if(layer)tile&=0xdfff;
    }
    unsigned c=graphics(s,tile,qx&7,qy&7,0,4);
    unsigned z=((tile&0x2000)?12:8)-layer;
    if(c&&z>depth[x]){depth[x]=z;colors[x]=s->pal[((tile>>10)&7)*16+c];}
  }
}
static int same_background(const ScenePacket *s,unsigned width) {
  const SceneBackgroundCache *b=&s_background;
  return b->valid&&b->width==width&&b->generation==s->generation&&
    b->wx==s->wx&&b->wy==s->wy&&b->world_width==s->world_width&&b->world_height==s->world_height&&
    b->map_fill==s->map_fill&&b->fill_meta==s->fill_meta&&
    b->native_x==s->native_x&&b->native_y==s->native_y&&b->bg2_x==s->bg2_x&&b->bg2_y==s->bg2_y&&
    b->parallax==s->parallax&&b->finite_clip==s->finite_clip&&
    b->incoming_mosaic==s->incoming_mosaic&&b->mosaic_size==s->mosaic_size&&b->brightness==s->brightness&&
    b->mosaic_y_phase==s->mosaic_y_phase&&b->mosaic_backend==s->mosaic_backend&&b->mosaic_y_size==s->mosaic_y_size&&
    !memcmp(b->regs,s->regs,sizeof(b->regs))&&!memcmp(b->maps,s->maps,sizeof(b->maps))&&
    !memcmp(b->defs,s->defs,sizeof(b->defs))&&!memcmp(b->vram,s->vram,sizeof(b->vram))&&
    !memcmp(b->pal,s->pal,sizeof(b->pal));
}
static void remember_background(const ScenePacket *s,unsigned width) {
  SceneBackgroundCache *b=&s_background;
  memcpy(b->regs,s->regs,sizeof(b->regs));memcpy(b->maps,s->maps,sizeof(b->maps));
  memcpy(b->defs,s->defs,sizeof(b->defs));memcpy(b->vram,s->vram,sizeof(b->vram));
  memcpy(b->pal,s->pal,sizeof(b->pal));
  b->width=width;b->generation=s->generation;b->wx=s->wx;b->wy=s->wy;
  b->world_width=s->world_width;b->world_height=s->world_height;
  b->map_fill=s->map_fill;b->fill_meta=s->fill_meta;
  b->native_x=s->native_x;b->native_y=s->native_y;b->bg2_x=s->bg2_x;b->bg2_y=s->bg2_y;
  b->parallax=s->parallax;b->finite_clip=s->finite_clip;
  b->incoming_mosaic=s->incoming_mosaic;b->mosaic_size=s->mosaic_size;b->brightness=s->brightness;
  b->mosaic_y_phase=s->mosaic_y_phase;b->mosaic_backend=s->mosaic_backend;b->mosaic_y_size=s->mosaic_y_size;
  b->valid=1; /* Input key only until a second matching render needs these BGs. */
}
static void render(const ScenePacket *s,unsigned width) {
  int left=(int)(width-256)/2;
  int background_match=same_background(s,width);
  int reuse_background=background_match&&s_background.valid==2;
  if(reuse_background)++s_background_reuses;else ++s_background_builds;
  for(unsigned y=0;y<H;++y) {
    uint64_t rank[W];uint16_t obj[W];uint8_t priority[W];
    for(unsigned x=0;x<width;++x){rank[x]=UINT64_MAX;priority[x]=0;obj[x]=0;}
    for(unsigned slot=0;slot<128;++slot) {
      const uint8_t *o=(const uint8_t *)s->oam+slot*4;
      unsigned high=(s->high[slot/4]>>((slot%4)*2))&3,n=(high&2)?16:8;
      unsigned dy=(y-o[1])&255;if(dy>=n)continue;
      if(!s->policy.allow[slot])continue;
      int ox=s->policy.x[slot];
      /* This incoming class grants no new far-actor/parked-OAM authority.
       * Preserve only native objects and their actual edge-straddling cells. */
      if(s->incoming_mosaic&&(ox>=256||ox+(int)n<=0))continue;
      unsigned tile=o[2]|o[3]<<8,sy=(tile&0x8000)?n-1-dy:dy;
      for(unsigned dx=0;dx<n;++dx) {
        int nx=ox+(int)dx,x=nx+left;if((nx<0&&s->policy.clip_left[slot])||x<0||x>=(int)width)continue;
        uint64_t order=(uint64_t)slot<<32;if(order>=rank[x])continue;
        unsigned sx=(tile&0x4000)?n-1-dx:dx;
        unsigned ch=(((((tile&255)>>4)+(sy>>3))<<4)|(((tile&15)+(sx>>3))&15));
        unsigned c=graphics(s,ch,sx&7,sy&7,(tile&0x100)?0x5000:0x4000,4);
        if(c){rank[x]=order;priority[x]=2+4*((tile>>12)&3);obj[x]=s->pal[128+((tile>>9)&7)*16+c];}
      }
    }
    for(unsigned i=0;i<s->count;++i) {
      const SceneObject *a=&s->objects[i];int dy=(int)y-a->y;if(dy<0||dy>=a->height)continue;
      for(unsigned dx=0;dx<a->width;++dx) {
        int x=a->x+(int)dx+left;if(x<0||x>=(int)width)continue;
        uint16_t c=a->pixels[dy*a->width+dx];uint64_t order=((uint64_t)a->slot<<32)|a->order;
        if((c&0x8000)&&order<rank[x]){rank[x]=order;priority[x]=2+4*a->priority;obj[x]=c&0x7fff;}
      }
    }
    uint8_t depth[W];uint16_t colors[W];
    if(reuse_background) {
      memcpy(depth,s_background.depth+y*width,width);
      memcpy(colors,s_background.colors+y*width,width*sizeof(*colors));
    } else {
      memset(depth,0,width);
      for(unsigned x=0;x<width;++x)colors[x]=s->pal[0];
      if(s->incoming_mosaic) {
        mosaic_background_row(s,width,y,left,0,depth,colors);
        mosaic_background_row(s,width,y,left,1,depth,colors);
      } else {
        background_row(s,width,y,left,0,depth,colors);
        background_row(s,width,y,left,1,depth,colors);
      }
      /* A continuously moving camera misses every field. Do not also copy a
       * decoded field that cannot be reused; wait for a repeated exact key. */
      if(background_match) {
        memcpy(s_background.depth+y*width,depth,width);
        memcpy(s_background.colors+y*width,colors,width*sizeof(*colors));
      }
    }
    for(unsigned x=0;x<width;++x)if(priority[x]>depth[x]){depth[x]=priority[x];colors[x]=obj[x];}
    unsigned yy=s->incoming_mosaic?((y+1-s->mosaic_y_phase)/s->mosaic_y_size)*s->mosaic_y_size+s->mosaic_y_phase:y+1;
    if(s->incoming_mosaic)for(unsigned xx=0;xx<256;++xx) {
      unsigned sx=xx/s->mosaic_size*s->mosaic_size;
      unsigned tile=s->vram[0x3c00+(yy/8)*32+sx/8],palette=((tile>>10)&7)*4,pz=(tile&0x2000)?15:1;
      unsigned c=graphics(s,tile,sx&7,yy&7,0x6000,2),x=left+xx;
      if(c&&pz>depth[x])colors[x]=s->pal[palette+c];
    }
    else for(unsigned xx=0;xx<256;xx+=8) {
      unsigned tile=s->vram[0x3c00+(yy/8)*32+xx/8],palette=((tile>>10)&7)*4,pz=(tile&0x2000)?15:1;
      uint64_t row=graphics_row(s,tile,yy&7,0x6000,2);
      for(unsigned dx=0;dx<8;++dx) {
        unsigned px=(tile&0x4000)?7-dx:dx,c=(unsigned)(row>>(px*8))&255,x=left+xx+dx;
        if(c&&pz>depth[x])colors[x]=s->pal[palette+c];
      }
    }
    for(unsigned x=0;x<width;++x) {
      int outside=(s->parallax||s->finite_clip)&&((int)x<left||(int)x>=left+256)&&
        (s->wx+(int)x-left<0||s->wx+(int)x-left>=s->world_width||
         s->wy+(int)y<0||s->wy+(int)y>=s->world_height);
      uint32_t color=s_rgb15[colors[x]&32767];
      if(s->incoming_mosaic&&s->brightness!=15)color=
          ((((color>>16)&255)*s->brightness/15)<<16)|
          ((((color>>8)&255)*s->brightness/15)<<8)|((color&255)*s->brightness/15);
      s_pixels[y*width+x]=outside?0:color;
    }
  }
  if(!background_match)remember_background(s,width);
  else if(!reuse_background)s_background.valid=2;
  ++s_stats.renders;s_stats.independent_pixels+=(uint64_t)width*H;
}
static int same_source(const ScenePacket *a,const ScenePacket *b) {
  if(a->generation!=b->generation||a->room!=b->room||a->wx!=b->wx||a->wy!=b->wy||a->native_x!=b->native_x||a->native_y!=b->native_y||
    a->incoming_mosaic!=b->incoming_mosaic||a->mosaic_y_phase!=b->mosaic_y_phase||a->mosaic_backend!=b->mosaic_backend||
    a->mosaic_y_size!=b->mosaic_y_size||
    a->world_width!=b->world_width||a->world_height!=b->world_height||a->count!=b->count||
    a->map_fill!=b->map_fill||a->fill_meta!=b->fill_meta||
    a->parallax!=b->parallax||a->finite_clip!=b->finite_clip||a->bg2_x!=b->bg2_x||a->bg2_y!=b->bg2_y||
    memcmp(a->regs,b->regs,sizeof(a->regs))||memcmp(&a->policy,&b->policy,sizeof(a->policy))||
    memcmp(a->oam,b->oam,sizeof(a->oam))||memcmp(a->high,b->high,sizeof(a->high))||
    memcmp(a->pal,b->pal,sizeof(a->pal))||memcmp(a->vram,b->vram,sizeof(a->vram))||
    memcmp(a->maps,b->maps,sizeof(a->maps))||memcmp(a->defs,b->defs,sizeof(a->defs)))return 0;
  for(unsigned i=0;i<a->count;++i) {
    const SceneObject *x=&a->objects[i],*y=&b->objects[i];
    if(x->x!=y->x||x->y!=y->y||x->width!=y->width||x->height!=y->height||x->priority!=y->priority||
      x->slot!=y->slot||x->order!=y->order||memcmp(x->pixels,y->pixels,x->width*x->height*2))return 0;
  }
  return 1;
}
int LagoonSceneRenderer_TryPresent(uint8_t *dst,size_t pitch,const uint8_t *ref,
    size_t ref_pitch,unsigned width,unsigned height,uint64_t generation) {
  if(!s_mode)return 0;
  ScenePacket *s=&s_packets[s_complete];
  if(!s->valid||s->generation!=generation||!dst||!ref||width<256||width>W||width%2||height!=H||pitch<width*4||ref_pitch<width*4){
    ++s_stats.fallbacks;report("unsupported",s->frame,width);return 0;
  }
  if(s_cache_serial!=s->serial||s_cache_width!=width) {
    /* Only reuse the immediately previous presented packet's candidate when
     * every rendering input is byte-identical. No guest clock is skipped. */
    const ScenePacket *previous=&s_packets[s_complete^1];
    if(s_cache_width!=width||s_cache_serial!=previous->serial||!same_source(s,previous))render(s,width);
    s_cache_serial=s->serial;s_cache_width=width;
  }
  unsigned first=s->incoming_mosaic?(width-256)/2:0;
  unsigned compared=s->incoming_mosaic?256:width;
  for(unsigned y=0;y<H;++y)if(memcmp(s_pixels+y*width+first,ref+y*ref_pitch+first*4,compared*4)) {
    if(s_report[0]&&s_mismatch_count<64) {
      SceneMismatch *m=&s_mismatch[s_mismatch_count++];memset(m,0,sizeof(*m));
      m->frame=s->frame;m->width=width;m->wx=s->wx;m->wy=s->wy;
      m->world_width=s->world_width;m->world_height=s->world_height;m->minx=width;m->miny=H;
      for(unsigned yy=y;yy<H;++yy)for(unsigned xx=first;xx<first+compared;++xx) {
        uint32_t r;memcpy(&r,ref+yy*ref_pitch+xx*4,4);uint32_t c=s_pixels[yy*width+xx];
        if(c==r)continue;
        if(!m->count){m->x=xx;m->y=yy;m->candidate=c;m->reference=r;}
        ++m->count;if(xx<m->minx)m->minx=xx;if(xx>m->maxx)m->maxx=xx;
        if(yy<m->miny)m->miny=yy;if(yy>m->maxy)m->maxy=yy;
      }
    }
    ++s_stats.mismatches;++s_stats.fallbacks;report("mismatch",s->frame,width);return 0;
  }
  ++s_stats.verified_presents;
  if(s->incoming_mosaic)++s_mosaic_verified;
  if(s_mode==2){
    for(unsigned y=0;y<H;++y) {
      if(s->incoming_mosaic) {
        memcpy(dst+y*pitch,s_pixels+y*width,first*4);
        memcpy(dst+y*pitch+(first+256)*4,s_pixels+y*width+first+256,(width-first-256)*4);
      }else memcpy(dst+y*pitch,s_pixels+y*width,width*4);
    }
    ++s_stats.replaced_presents;if(s->incoming_mosaic)++s_mosaic_replaced;
  }
  report(s->incoming_mosaic?(s_mode==2?"mosaic-margins":"mosaic-native-verified"):
      (s_mode==2?"replaced":"verified"),s->frame,width);return s_mode==2;
}
void LagoonSceneRenderer_GetStats(LagoonSceneRendererStats *out){if(out)*out=s_stats;}

/* Visited-map storage is deliberately separate from both live packet admission
 * and live decoded-row caches. Records carry only terrain, never actors, CPU,
 * pointers, generations or C struct padding. I/O occurs only through explicit
 * Load/Save calls made by the paused map/normal shutdown owner. */
enum { ARCHIVE_ROOMS=75,ARCHIVE_HEADER=80,ARCHIVE_DATA=73984,
       ARCHIVE_RECORD=36+ARCHIVE_DATA+4,ARCHIVE_MAX=ARCHIVE_HEADER+ARCHIVE_ROOMS*ARCHIVE_RECORD };
typedef struct ArchiveRoomContract {
  unsigned width,height,parallax;uint32_t descriptor_crc;uint8_t defs_sha[32];
} ArchiveRoomContract;
static ArchiveRoomContract s_archive_contract[ARCHIVE_ROOMS];
static SceneMapSource *s_archive;
static const uint8_t *s_archive_rom;
static unsigned s_archive_ready,s_archive_count,s_archive_dirty,s_archive_revision;
static unsigned s_archive_loaded,s_archive_blocked,s_archive_disk_present;
static uint32_t s_archive_catalog_crc;
static char s_archive_path[1024];
static uint8_t s_archive_disk_sha[32];
static uint16_t s_archive_page_check[32768];
static uint32_t s_visited_pixels[1024*1024];
static unsigned s_visited_valid,s_visited_room,s_visited_revision,s_visited_width,s_visited_height;
static const uint8_t archive_rom_sha[32]={
  0x46,0xc8,0x11,0xf0,0xca,0xcf,0xfe,0x8f,0x20,0xe1,0xd6,0x30,0x72,0xd2,0x5d,0x7c,
  0x47,0xe9,0xbb,0x3f,0xd5,0x12,0x48,0x51,0xfd,0x05,0xac,0xa9,0x88,0x4d,0x21,0xfb};
static unsigned archive_u32(const uint8_t *p){return (unsigned)p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static void archive_put32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(v>>(i*8));}
static int archive_i32(uint32_t v){return v<=INT_MAX?(int)v:-1-(int)(UINT32_MAX-v);}
static void archive_words_out(uint8_t **at,const uint16_t *p,size_t count){
  while(count--){*(*at)++=(uint8_t)*p;*(*at)++=(uint8_t)(*p++>>8);}
}
static void archive_words_in(const uint8_t **at,uint16_t *p,size_t count){
  while(count--){*p++=(uint16_t)((*at)[0]|(*at)[1]<<8);*at+=2;}
}
#ifdef _WIN32
static wchar_t *archive_wide(const char *s){
  int n=MultiByteToWideChar(ARCHIVE_CP_UTF8,ARCHIVE_MB_ERR_INVALID_CHARS,s,-1,NULL,0);if(!n)return NULL;
  wchar_t *p=malloc((size_t)n*sizeof *p);if(p&&!MultiByteToWideChar(ARCHIVE_CP_UTF8,ARCHIVE_MB_ERR_INVALID_CHARS,s,-1,p,n)){free(p);p=NULL;}return p;
}
static FILE *archive_open(const char *path,const char *mode){
  wchar_t *p=archive_wide(path),m[8];unsigned n=0;while(mode[n]&&n<7){m[n]=(wchar_t)mode[n];++n;}m[n]=0;
  FILE *f=p?_wfopen(p,m):NULL;free(p);return f;
}
static int archive_replace(const char *from,const char *to){
  wchar_t *a=archive_wide(from),*b=archive_wide(to);int good=a&&b&&MoveFileExW(a,b,ARCHIVE_MOVEFILE_REPLACE_EXISTING|ARCHIVE_MOVEFILE_WRITE_THROUGH);free(a);free(b);return good;
}
static void archive_remove(const char *path){wchar_t*p=archive_wide(path);if(p){_wremove(p);free(p);}}
#else
static FILE *archive_open(const char *path,const char *mode){return fopen(path,mode);}
static int archive_replace(const char *from,const char *to){return rename(from,to)==0;}
static void archive_remove(const char *path){(void)remove(path);}
#endif
static void archive_changed(void){s_archive_dirty=1;if(!++s_archive_revision)s_archive_revision=1;s_visited_valid=0;}
int LagoonSceneRenderer_ArchiveOnRomInit(const uint8_t *rom,size_t size){
  free(s_archive);s_archive=NULL;s_archive_rom=NULL;
  s_archive_ready=s_archive_count=s_archive_dirty=s_archive_revision=0;
  s_archive_loaded=s_archive_blocked=s_archive_disk_present=s_visited_valid=0;s_archive_path[0]=0;
  memset(s_archive_contract,0,sizeof s_archive_contract);memset(s_archive_disk_sha,0,sizeof s_archive_disk_sha);
  if(!rom||size!=0x100000)return 0;uint8_t hash[32];sha256_compute(rom,size,hash);
  if(memcmp(hash,archive_rom_sha,32))return 0;
  for(unsigned room=0;room<ARCHIVE_ROOMS;++room){
    const uint8_t *r=rom+0x2b2df+room*56;ArchiveRoomContract *c=&s_archive_contract[room];
    c->width=word(r,3)*8;c->height=word(r,5)*8;c->parallax=r[0]!=0;
    unsigned definitions=ptr(r,0x19),other=ptr(r,0x25);
    size_t offset=(definitions>>16)*32768+(definitions&32767);
    if(!c->width||c->width>2048||!c->height||c->height>2048||definitions!=other||offset>size||size-offset<2048)return 0;
    c->descriptor_crc=crc32_compute(r,56);sha256_compute(rom+offset,2048,c->defs_sha);
  }
  s_archive=calloc(ARCHIVE_ROOMS,sizeof *s_archive);if(!s_archive)return 0;
  s_archive_catalog_crc=crc32_compute(rom+0x2b2df,ARCHIVE_ROOMS*56);
  s_archive_rom=rom;s_archive_ready=1;init_tables();return 1;
}
unsigned LagoonSceneRenderer_ArchivedRoomCount(void){return s_archive_ready?s_archive_count:0;}
int LagoonSceneRenderer_ArchivePersistenceBlocked(void){return s_archive_blocked!=0;}
static int archive_source_valid(const SceneMapSource *s){
  if(!s_archive_ready||!s||!s->valid||s->room>=ARCHIVE_ROOMS)return 0;
  const ArchiveRoomContract *c=&s_archive_contract[s->room];
  if(s->world_width!=(int)c->width||s->world_height!=(int)c->height||s->parallax!=c->parallax||
      memcmp(s->defs[0],s->defs[1],sizeof s->defs[0]))return 0;
  uint8_t definitions[2048],*at=definitions,hash[32];archive_words_out(&at,s->defs[0],1024);
  sha256_compute(definitions,sizeof definitions,hash);if(memcmp(hash,c->defs_sha,32))return 0;
  for(unsigned i=0;i<128;++i)if(s->pal[i]&0x8000)return 0;
  if(s->parallax){
    if(s->wx<0||s->wx>=2048||s->wy<1||s->wy>2048||
        s->bg2_x!=(unsigned)s->wx/4||s->bg2_y!=(unsigned)(s->wy-1)/4+1)return 0;
    memcpy(s_archive_page_check+0x7800,s->pages,sizeof s->pages);
    if(!LagoonWideEntities_ParallaxMapForRoom(s_archive_page_check,s_archive_rom,0x100000,s->room))return 0;
  }
  return 1;
}
static int archive_same(const SceneMapSource *a,const SceneMapSource *b){
  return a->valid&&a->room==b->room&&a->world_width==b->world_width&&a->world_height==b->world_height&&
    a->parallax==b->parallax&&(!a->parallax||(a->wx==b->wx&&a->wy==b->wy&&a->bg2_x==b->bg2_x&&a->bg2_y==b->bg2_y&&
      !memcmp(a->pages,b->pages,sizeof a->pages)))&&
    !memcmp(a->maps,b->maps,sizeof a->maps)&&!memcmp(a->defs,b->defs,sizeof a->defs)&&
    !memcmp(a->patterns,b->patterns,sizeof a->patterns)&&!memcmp(a->pal,b->pal,sizeof a->pal);
}
static int archive_remember(const SceneMapSource *s){
  if(!s_archive_ready||!s||s->room>=ARCHIVE_ROOMS)return 0;
  if(archive_same(&s_archive[s->room],s))return 1;if(!archive_source_valid(s))return 0;
  SceneMapSource *dest=&s_archive[s->room];if(!dest->valid)++s_archive_count;
  memcpy(dest,s,sizeof *dest);dest->serial=0;dest->generation=0;
  if(!dest->parallax){dest->wx=dest->wy=0;dest->bg2_x=dest->bg2_y=0;memset(dest->pages,0,sizeof dest->pages);}
  archive_changed();return 1;
}
int LagoonSceneRenderer_ArchiveCommit(unsigned room,uint64_t generation){
  const ScenePacket *p=&s_packets[s_complete];const SceneMapSource *s=&s_map_source;
  if(!s_archive_ready||room>=ARCHIVE_ROOMS||!p->valid||p->lines!=H||p->incoming_mosaic||
      p->room!=room||p->generation!=generation||!s->valid||s->room!=room||s->generation!=generation||
      s->serial!=p->serial||!s_map_identity_valid||s_map_identity_lines!=H||
      s_map_identity_room!=room||s_map_identity_generation!=generation)return 0;
  return archive_remember(s);
}
static void archive_encode(uint8_t *data,const SceneMapSource *s){
  archive_put32(data,s->room);archive_put32(data+4,s->parallax);
  archive_put32(data+8,(unsigned)s->world_width);archive_put32(data+12,(unsigned)s->world_height);
  archive_put32(data+16,(uint32_t)s->wx);archive_put32(data+20,(uint32_t)s->wy);
  archive_put32(data+24,s->bg2_x);archive_put32(data+28,s->bg2_y);
  archive_put32(data+32,s_archive_contract[s->room].descriptor_crc);
  uint8_t *at=data+36;memcpy(at,s->maps,sizeof s->maps);at+=sizeof s->maps;
  archive_words_out(&at,s->defs[0],1024);archive_words_out(&at,s->defs[1],1024);
  archive_words_out(&at,s->patterns,16384);archive_words_out(&at,s->pages,2048);archive_words_out(&at,s->pal,128);
  archive_put32(at,crc32_compute(data,ARCHIVE_RECORD-4));
}
static int archive_decode(const uint8_t *data,SceneMapSource *s){
  unsigned room=archive_u32(data);if(room>=ARCHIVE_ROOMS||
      archive_u32(data+32)!=s_archive_contract[room].descriptor_crc||
      archive_u32(data+ARCHIVE_RECORD-4)!=crc32_compute(data,ARCHIVE_RECORD-4))return 0;
  memset(s,0,sizeof *s);s->room=room;s->valid=1;s->parallax=archive_u32(data+4);
  s->world_width=archive_i32(archive_u32(data+8));s->world_height=archive_i32(archive_u32(data+12));
  s->wx=archive_i32(archive_u32(data+16));s->wy=archive_i32(archive_u32(data+20));
  s->bg2_x=archive_u32(data+24);s->bg2_y=archive_u32(data+28);
  const uint8_t *at=data+36;memcpy(s->maps,at,sizeof s->maps);at+=sizeof s->maps;
  archive_words_in(&at,s->defs[0],1024);archive_words_in(&at,s->defs[1],1024);
  archive_words_in(&at,s->patterns,16384);archive_words_in(&at,s->pages,2048);archive_words_in(&at,s->pal,128);
  if(!s->parallax){if(s->wx||s->wy||s->bg2_x||s->bg2_y)return 0;
    for(unsigned i=0;i<2048;++i)if(s->pages[i])return 0;}
  return archive_source_valid(s);
}
/* 0=missing,1=validated immutable bytes,-1=foreign/corrupt/I/O error. Entire
 * bounded file validates before ANY room is imported, so a bad late record
 * cannot partially poison the in-memory UI archive. */
static int archive_read(const char *path,uint8_t **bytes,size_t *size,uint8_t hash[32]){
  *bytes=NULL;*size=0;errno=0;FILE *f=archive_open(path,"rb");if(!f)return errno==ENOENT?0:-1;
  uint8_t header[ARCHIVE_HEADER];int good=fread(header,1,sizeof header,f)==sizeof header;
  unsigned count=good?archive_u32(header+56):ARCHIVE_ROOMS+1;
  size_t length=ARCHIVE_HEADER+(size_t)count*ARCHIVE_RECORD;
  good=good&&count<=ARCHIVE_ROOMS&&!memcmp(header,"LGSCN001",8)&&archive_u32(header+8)==1&&
    archive_u32(header+12)==ARCHIVE_HEADER&&!memcmp(header+16,archive_rom_sha,32)&&
    archive_u32(header+48)==0x100000&&archive_u32(header+52)==ARCHIVE_RECORD&&
    archive_u32(header+60)==count*ARCHIVE_RECORD&&archive_u32(header+68)==s_archive_catalog_crc&&
    !archive_u32(header+72)&&archive_u32(header+76)==crc32_compute(header,76)&&length<=ARCHIVE_MAX;
  uint8_t *data=good?malloc(length):NULL;good=good&&data;
  if(good){memcpy(data,header,sizeof header);good=fread(data+ARCHIVE_HEADER,1,length-ARCHIVE_HEADER,f)==length-ARCHIVE_HEADER&&fgetc(f)==EOF&&!ferror(f);}
  if(fclose(f))good=0;
  if(good)good=archive_u32(header+64)==crc32_compute(data+ARCHIVE_HEADER,length-ARCHIVE_HEADER);
  SceneMapSource *decoded=good?malloc(sizeof *decoded):NULL;good=good&&decoded;
  unsigned previous=ARCHIVE_ROOMS;
  for(unsigned i=0;good&&i<count;++i){
    good=archive_decode(data+ARCHIVE_HEADER+(size_t)i*ARCHIVE_RECORD,decoded)&&
      (i==0||decoded->room>previous);if(good)previous=decoded->room;
  }
  free(decoded);if(!good){free(data);return -1;}
  sha256_compute(data,length,hash);*bytes=data;*size=length;return 1;
}
static int archive_path(const char *path){
  if(!s_archive_ready||!path||!*path||strlen(path)>=sizeof s_archive_path)return 0;
  if(s_archive_loaded)return !strcmp(path,s_archive_path);
  memcpy(s_archive_path,path,strlen(path)+1);return 1;
}
int LagoonSceneRenderer_ArchiveLoad(const char *path){
  if(s_archive_blocked)return -1;if(!archive_path(path)){s_archive_blocked=1;return -1;}
  if(s_archive_loaded)return s_archive_disk_present?1:0;s_archive_loaded=1;
  uint8_t *data=NULL,hash[32];size_t size=0;int status=archive_read(path,&data,&size,hash);
  if(status<0){s_archive_blocked=1;return -1;}s_archive_disk_present=status==1;
  if(status==1){
    SceneMapSource *decoded=malloc(sizeof *decoded);if(!decoded){free(data);s_archive_blocked=1;return -1;}
    unsigned count=archive_u32(data+56);
    for(unsigned i=0;i<count;++i){
      (void)archive_decode(data+ARCHIVE_HEADER+(size_t)i*ARCHIVE_RECORD,decoded);
      if(!s_archive[decoded->room].valid){memcpy(&s_archive[decoded->room],decoded,sizeof *decoded);++s_archive_count;}
    }
    free(decoded);memcpy(s_archive_disk_sha,hash,32);if(!++s_archive_revision)s_archive_revision=1;s_visited_valid=0;
  }
  free(data);return status;
}
static int archive_disk_unchanged(void){
  uint8_t *data=NULL,hash[32];size_t size=0;int status=archive_read(s_archive_path,&data,&size,hash);free(data);
  if(status<0||(status==1)!=s_archive_disk_present||(status==1&&memcmp(hash,s_archive_disk_sha,32))){s_archive_blocked=1;return 0;}return 1;
}
int LagoonSceneRenderer_ArchiveSave(const char *path){
  if(LagoonSceneRenderer_ArchiveLoad(path)<0)return -1;if(!s_archive_dirty)return 0;
  if(!archive_disk_unchanged())return -1;
  size_t size=ARCHIVE_HEADER+(size_t)s_archive_count*ARCHIVE_RECORD;
  uint8_t *data=calloc(1,size);if(!data)return -1;
  memcpy(data,"LGSCN001",8);archive_put32(data+8,1);archive_put32(data+12,ARCHIVE_HEADER);
  memcpy(data+16,archive_rom_sha,32);archive_put32(data+48,0x100000);archive_put32(data+52,ARCHIVE_RECORD);
  archive_put32(data+56,s_archive_count);archive_put32(data+60,s_archive_count*ARCHIVE_RECORD);
  archive_put32(data+68,s_archive_catalog_crc);unsigned count=0;
  for(unsigned room=0;room<ARCHIVE_ROOMS;++room)if(s_archive[room].valid)
    archive_encode(data+ARCHIVE_HEADER+(size_t)count++*ARCHIVE_RECORD,&s_archive[room]);
  if(count!=s_archive_count){free(data);return -1;}
  archive_put32(data+64,crc32_compute(data+ARCHIVE_HEADER,size-ARCHIVE_HEADER));archive_put32(data+76,crc32_compute(data,76));
  char temporary[1040];if(snprintf(temporary,sizeof temporary,"%s.tmp",path)>=(int)sizeof temporary){free(data);return -1;}
  FILE *f=archive_open(temporary,"wbx");if(!f){free(data);return -1;}
  int good=fwrite(data,1,size,f)==size;if(fclose(f))good=0;
  if(good)good=archive_disk_unchanged();if(good)good=archive_replace(temporary,path);
  if(!good){archive_remove(temporary);free(data);return -1;}
  sha256_compute(data,size,s_archive_disk_sha);free(data);s_archive_disk_present=1;s_archive_dirty=0;return 1;
}
/* Independent row cache: browsing old sources cannot pollute a live packet's
 * serial-keyed decoder. Rebuilt only for this one offline image, while paused. */
static uint64_t s_visited_rows[16384];static uint8_t s_visited_row_valid[16384];
static uint64_t visited_row(const SceneMapSource *s,unsigned tile,unsigned y){
  if(tile&0x8000)y=7-y;unsigned adr=(tile&1023)*16+y;
  if(!s_visited_row_valid[adr]){unsigned a=s->patterns[adr],b=s->patterns[adr+8];
    s_visited_rows[adr]=s_spread[a&255]|s_spread[a>>8]<<1|s_spread[b&255]<<2|s_spread[b>>8]<<3;s_visited_row_valid[adr]=1;}
  return s_visited_rows[adr];
}
int LagoonSceneRenderer_VisitedMapImage(unsigned room,const uint32_t **pixels,
    unsigned *width,unsigned *height,unsigned *world_width,unsigned *world_height){
  if(pixels)*pixels=NULL;if(width)*width=0;if(height)*height=0;
  if(world_width)*world_width=0;if(world_height)*world_height=0;
  if(!pixels||!width||!height||!s_archive_ready||room>=ARCHIVE_ROOMS||!s_archive[room].valid)return 0;
  const SceneMapSource *s=&s_archive[room];unsigned scale=s->world_width>1024||s->world_height>1024?2:1;
  unsigned w=((unsigned)s->world_width+scale-1)/scale,h=((unsigned)s->world_height+scale-1)/scale;
  if(!s_visited_valid||s_visited_room!=room||s_visited_revision!=s_archive_revision){
    memset(s_visited_row_valid,0,sizeof s_visited_row_valid);
    for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){
      unsigned qx=x*scale,qy=y*scale,depth=0,color=s->pal[0];
      for(unsigned layer=0;layer<2;++layer){unsigned tx=qx,ty=qy,tile;
        if(layer&&s->parallax){tx=(unsigned)((int)qx+(int)s->bg2_x-s->wx);ty=(unsigned)((int)qy+(int)s->bg2_y-s->wy);
          tile=s->pages[((ty>>8)&1)*1024+((ty>>3)&31)*32+((tx>>3)&31)];
        }else{unsigned meta=s->maps[layer][(qy>>4)*128+(qx>>4)];tile=s->defs[layer][meta*4+((qx&8)>>3)+((qy&8)>>2)];}
        if(layer&&!s->parallax)tile&=0xdfff;unsigned px=tile&0x4000?7-(tx&7):tx&7;
        unsigned c=(unsigned)(visited_row(s,tile,ty&7)>>(px*8))&255,z=(tile&0x2000?12:8)-layer;
        if(c&&z>depth){depth=z;color=s->pal[((tile>>10)&7)*16+c];}
      }
      s_visited_pixels[y*w+x]=0xff000000u|s_rgb15[color&32767];
    }
    s_visited_valid=1;s_visited_room=room;s_visited_revision=s_archive_revision;s_visited_width=w;s_visited_height=h;
  }
  *pixels=s_visited_pixels;*width=s_visited_width;*height=s_visited_height;
  if(world_width)*world_width=(unsigned)s->world_width;if(world_height)*world_height=(unsigned)s->world_height;return 1;
}
