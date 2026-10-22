/* Bounded independent scene drawing. Guest simulation, raster fallback and the
 * reference field remain authoritative. No framebuffer enters the packet. */
#include "lagoon_scene_renderer.h"
#include "lagoon_wide_entities.h"
#include "snes/ppu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  unsigned count,lines,frame,serial,valid;
  uint32_t vram_writes,oam_writes;
  int wx,wy,world_width,world_height;
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
} SceneMismatch;
static SceneMismatch s_mismatch[64];
static unsigned s_mismatch_count;
static uint32_t s_pixels[W*H];
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
  ScenePacket *s=&s_packets[s_build];
  if(line==1) {
    s->valid=0;s->lines=0;s->generation=generation;
    if(!p||!supported(p,r,rom,size,authority))return;
    s->valid=1;s->lines=1;
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
  if(!++s_serial){memset(s_plane_keys,0,sizeof(s_plane_keys));s_serial=1;s_cache_serial=0;}
  s->frame=frame;s->serial=s_serial;
  if(s->valid)++s_stats.packets;else ++s_stats.refused_packets;
  s_complete=s_build;s_build^=1;s_packets[s_build].valid=0;
}
static uint64_t graphics_row(const ScenePacket *s,unsigned tile,unsigned y,unsigned base,unsigned bpp) {
  if(tile&0x8000)y=7-y;
  unsigned adr=(base+(tile&1023)*(bpp==2?8:16)+y)&32767;
  unsigned kind=bpp==4;
  if(s_plane_keys[kind][adr]!=s->serial) {
    unsigned planes=s->vram[adr];
    uint64_t row=s_spread[planes&255]|(s_spread[planes>>8]<<1);
    if(kind){planes=s->vram[(adr+8)&32767];row|=(s_spread[planes&255]<<2)|(s_spread[planes>>8]<<3);}
    s_plane_rows[kind][adr]=row;s_plane_keys[kind][adr]=s->serial;
  }
  return s_plane_rows[kind][adr];
}
static unsigned graphics(const ScenePacket *s,unsigned tile,unsigned x,unsigned y,unsigned base,unsigned bpp) {
  if(tile&0x4000)x=7-x;
  return (unsigned)(graphics_row(s,tile,y,base,bpp)>>(x*8))&255;
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
      if(qx<0||qy<0||qx>=s->world_width||qy>=s->world_height){x+=count;continue;}
      unsigned meta=s->maps[layer][(qy>>4)*128+(qx>>4)];
      tile=s->defs[layer][meta*4+((qx&8)>>3)+((qy&8)>>2)];
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
  if(a->generation!=b->generation||a->wx!=b->wx||a->wy!=b->wy||a->native_x!=b->native_x||a->native_y!=b->native_y||
    a->incoming_mosaic!=b->incoming_mosaic||a->mosaic_y_phase!=b->mosaic_y_phase||a->mosaic_backend!=b->mosaic_backend||
    a->mosaic_y_size!=b->mosaic_y_size||
    a->world_width!=b->world_width||a->world_height!=b->world_height||a->count!=b->count||
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
