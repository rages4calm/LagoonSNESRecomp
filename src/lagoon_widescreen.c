/* Presentation-only expansion. The $01:8311 map resolver is reproduced as
 * read-only host lookups, never by moving the guest camera or replaying DMA.
 * No ROM data, graphics, or generated guest code belongs in this source. */
#include "lagoon_widescreen.h"
#include "lagoon_wide_entities.h"
#include "lagoon_menu_presentation.h"
#include "lagoon_state.h"
#include "lagoon_scene_renderer.h"
#include "common_rtl.h"
#include "config.h"
#include "widescreen.h"
#include "snes/ppu.h"
#include "snes/ws_shadow.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double s_aspect = 16.0 / 9.0; /* 0 means drawable aspect */
static int s_was_active;
static int s_native_fallback;
static int s_present_fallback;
static int s_outdoor_frame;
static int s_outdoor_present;
static int s_row_world_x[224],s_row_world_y[224];
static uint8_t s_row_known[224];
static unsigned s_extent_width,s_extent_height;
static LagoonWideSceneGate s_scene_gate;
static unsigned s_room_key;
/* Static audited rooms share BG1/2 character space [0,$4000) words. Once
 * witnessed during ordinary field play, text may overlay that same scenery.
 * An inventory replacing either map or graphics cannot borrow this authority. */
static uint16_t s_world_chars[0x4000];
static unsigned s_world_char_key;
static uint8_t s_world_char_used[1024];
static int s_world_chars_ready;
static int s_initial_world_authority;
static LagoonWideBackground s_background;
static LagoonWideTorches s_torches;
static uint64_t s_state_generation;
static uint64_t s_seen_load_generation;
static int s_using_retained;
static unsigned s_retained_invalidation_kind,s_retained_invalidation_address;
static LagoonWideParallaxQueue s_parallax_queue;
static struct {
  LagoonWideRetainedMap map;
  LagoonWideTorches torches;
  LagoonWideParallaxPacket parallax;
  uint16_t characters[0x4000];
  uint8_t used[1024];
  uint64_t generation;
  unsigned room,width,height,bg_source,camera[4];
  int valid;
} s_retained;
static const char *s_config_path = "config.ini";
static int retained_scene_matches(void);

void LagoonWide_SetConfigPath(const char *path) { if (path) s_config_path = path; }

static unsigned word(unsigned a) { return g_ram[a] | (g_ram[a + 1] << 8); }
static unsigned pointer(unsigned a) { return word(a) | (g_ram[a + 2] << 16); }

/* This is deliberately not snes_read: presentation must not tick the guest
 * bus or acknowledge registers. The map resolver only permits WRAM/LoROM. */
static unsigned byte_at(unsigned a) {
  unsigned bank = (a >> 16) & 255, off = a & 65535;
  if (bank == 0x7e || bank == 0x7f)
    return g_ram[((bank - 0x7e) << 16) | off];
  if ((bank & 0x7f) < 0x20 && off >= 0x8000)
    return g_rom[((bank & 0x1f) << 15) | (off & 0x7fff)];
  return 0;
}

static int valid_pointer(unsigned a) {
  unsigned bank = (a >> 16) & 255;
  return bank == 0x7e || bank == 0x7f ||
      ((bank & 0x7f) < 0x20 && (a & 65535) >= 0x8000);
}

static unsigned map_tile(int layer, int x, int y) {
  unsigned meta;
  if (x < 0 || y < 0 || x >= 2048 || y >= 2048)
    meta = g_ram[0x52]; /* same out-of-map fill as $01:833E */
  else
    meta = byte_at(pointer(layer ? 0x8b : 0x88) + (y >> 4) * 128 + (x >> 4));
  unsigned a = pointer(layer ? 0x75 : 0x69) + meta * 8 +
      ((x & 8) >> 2) + ((y & 8) >> 1);
  unsigned tile = byte_at(a) | (byte_at(a + 1) << 8);
  return layer ? tile & 0xdfff : tile;
}
static unsigned view_tile(int layer,int x,int y) {
  return s_using_retained ? LagoonWideRetainedMap_Tile(&s_retained.map,layer,x,y) : map_tile(layer,x,y);
}
static unsigned view_room(void) {return s_using_retained?s_retained.room:word(0x48e);}
static unsigned view_camera(unsigned index) {return s_using_retained?s_retained.camera[index]:word(0x480+index*2);}

static void read_aspect(const char *value) {
  if (!strcmp(value, "window")) s_aspect = 0;
  else if (!strcmp(value, "16:9")) s_aspect = 16.0 / 9;
  else if (!strcmp(value, "16:10")) s_aspect = 1.6;
  else if (!strcmp(value, "21:9")) s_aspect = 21.0 / 9;
  else if (!strcmp(value, "32:9")) s_aspect = 32.0 / 9;
}

static void read_ini(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) return;
  char line[512], section[64] = "";
  while (fgets(line, sizeof(line), f)) {
    char name[64], value[64];
    if (sscanf(line, " [%63[^]]]", name) == 1) {
      snprintf(section, sizeof(section), "%s", name);
    } else if (!strcmp(section, "Lagoon") &&
               sscanf(line, " %63[^= ] = %63s", name, value) == 2 &&
               !strcmp(name, "WideAspect")) read_aspect(value);
  }
  fclose(f);
}

void LagoonWide_AfterConfig(void) {
  LagoonWideEntities_Init();
  LagoonSceneRenderer_Reset();
  s_aspect = 16.0 / 9;
  read_ini(s_config_path);
  read_ini("config.local.ini");
  const char *aspect = getenv("LAGOON_WIDE_ASPECT");
  if (aspect) read_aspect(aspect);
}

void LagoonWide_PrepareFrame(int drawable_w, int drawable_h, int *width, int *height) {
  *height = 224;
  *width = 256;
  if (g_config.widescreen) {
    double aspect = s_aspect;
    if (!aspect) aspect = drawable_w > 0 && drawable_h > 0 ?
        (double)drawable_w / drawable_h : 16.0 / 9;
    SnesDisplayFrame frame = SnesDisplayAspect_ComputeAdaptiveFrame(
        256, 224, kPpuBufWidth, aspect,
        SnesDisplayAspect_Clamp(g_config.display_aspect));
    *width = frame.width;
  }
}

void LagoonWide_Reset(void) {
  LagoonSceneRenderer_Reset();
  s_retained.valid=0;s_using_retained=0;
  s_retained_invalidation_kind=1;s_retained_invalidation_address=0;
  memset(&s_parallax_queue,0,sizeof(s_parallax_queue));
  LagoonWideEntities_Reset();
  WsShadowReset();
  s_was_active = 0;
  s_world_chars_ready=0;
  s_initial_world_authority=0;LagoonWideBackground_Reset(&s_background);
  LagoonWideTorches_Reset(&s_torches);
  s_outdoor_frame=s_outdoor_present=0;
  memset(s_row_known,0,sizeof(s_row_known));
  memset(&s_scene_gate,0,sizeof(s_scene_gate));
  s_room_key = 0;
  s_state_generation = RtlStateGeneration();
}

static int room_bounds_enabled(unsigned room) { return (room==1 || room==2) || (room>=3 && room<=8); }
static int room07_parallax(void) {
  return word(0x48e)==7 && g_ram[0x50]==2 && pointer(0x57)==0x129e89 &&
      pointer(0x5d)==0x1a906f && pointer(0x69)==0x069720 && pointer(0x75)==0x069720 &&
      pointer(0x88)==0x7f0000 && pointer(0x8b)==0x7f4000 &&
      pointer(0x7e)==0x11fb7d && pointer(0x81)==0x11fb7d &&
      word(0x53)==128 && word(0x55)==128 && !g_snesrecomp_last_hdmaen;
}
static int view_parallax(void) {
  return s_using_retained ? s_retained.parallax.valid : room07_parallax();
}
static int audited_room_resources(void) {
  unsigned room=word(0x48e);
  return (room!=7 || room07_parallax()) && (!room_bounds_enabled(room) ||
      LagoonWideEntities_BoundedRoom(room,word(0x53)*8,word(0x55)*8)) &&
      LagoonWideEntities_AuditedRoom(word(0x48e),pointer(0x57),pointer(0x5d),
      pointer(0x69),pointer(0x75),pointer(0x88),pointer(0x8b));
}
static int background_upload_ready(void) {
  return s_background.ready && s_background.generation==RtlStateGeneration() &&
      s_background.room==word(0x48e) && s_background.source==pointer(0x57);
}
static int audited_room_ready(void) {
  if(s_using_retained)return retained_scene_matches();
  return audited_room_resources() && (!g_ram[0x100e] ||
      (s_initial_world_authority && background_upload_ready()));
}
static int has_modal_ui(void) {
  return ((g_ppu->screenEnabled[0]|g_ppu->screenEnabled[1])&4) &&
      (g_ppu->hScroll[2] || g_ppu->vScroll[2] ||
       LagoonWideEntities_MenuOverlay(PpuRenderVram(g_ppu),PPU_bgTilemapAdr(g_ppu,2),PPU_bgTileAdr(g_ppu,2)));
}
static unsigned world_resource_key(void) {
  return pointer(0x69) ^ pointer(0x75) ^ (g_ram[0x48e]<<24);
}
static int native_world_map(void) {
  int parallax=room07_parallax();
  if(PPU_bgTileAdr(g_ppu,0) || PPU_bgTileAdr(g_ppu,1) ||
      PPU_bgTilemapAdr(g_ppu,0)!=0x7000 || PPU_bgTilemapAdr(g_ppu,1)!=0x7800 ||
      !PPU_bgTilemapWider(g_ppu,0) ||
      (parallax ? (PPU_bgTilemapHigher(g_ppu,0) || PPU_bgTilemapWider(g_ppu,1) ||
        !PPU_bgTilemapHigher(g_ppu,1) ||
        !LagoonWideEntities_ParallaxScroll(word(0x480),word(0x482),word(0x484),word(0x486)) ||
        !LagoonWideEntities_ParallaxMap(PpuRenderVram(g_ppu),g_rom,0x100000)) :
        !PPU_bgTilemapWider(g_ppu,1)))return 0;
  uint16_t expected[720];
  for(unsigned layer=0;layer<2;++layer) {
    if(parallax && layer==1)continue; /* Static authored map owns its complete raster. */
    int cx=word(0x480+layer*4),cy=word(0x482+layer*4);
    int wx=cx+(((g_ppu->hScroll[layer]&1023)-cx+512)&1023)-512;
    int wy=cy+(((g_ppu->vScroll[layer]&1023)-cy+512)&1023)-512;
    if(wx<0 || wy<0)return 0;
    int tx=wx/8+1,ty=wy/8+1;
    for(unsigned y=0;y<24;++y)for(unsigned x=0;x<30;++x)
      expected[y*30+x]=(uint16_t)map_tile(layer,(tx+(int)x)*8,(ty+(int)y)*8);
    if(!LagoonWideEntities_WorldTilemap(PpuRenderVram(g_ppu),0x7000+layer*0x800,
        tx,ty,expected))return 0;
  }
  return 1;
}
static int retained_scene_matches(void) {
  unsigned destination=word(0x48e);
  if(!s_retained.valid || LagoonWideParallaxQueue_Pending(&s_parallax_queue) ||
      s_retained.generation!=RtlStateGeneration() ||
      g_snesrecomp_last_hdmaen ||
      !(destination==0 || destination==1 || destination==2 || (destination>=3 && destination<=6) ||
        (destination==7 && room07_parallax()) || destination==8) ||
      (g_ppu->bgmode&7)!=1 || (g_ppu->mosaic&3) ||
      PPU_bigTiles(g_ppu,0) || PPU_bigTiles(g_ppu,1) ||
      PPU_bgTileAdr(g_ppu,0)!=0 || PPU_bgTileAdr(g_ppu,1)!=0 ||
      PPU_bgTilemapAdr(g_ppu,0)!=0x7000 || PPU_bgTilemapAdr(g_ppu,1)!=0x7800 ||
      !PPU_bgTilemapWider(g_ppu,0) ||
      (s_retained.parallax.valid ? (PPU_bgTilemapHigher(g_ppu,0) ||
        PPU_bgTilemapWider(g_ppu,1) || !PPU_bgTilemapHigher(g_ppu,1)) :
        !PPU_bgTilemapWider(g_ppu,1)) ||
      !LagoonWideEntities_RetainedSceneValid())return 0;
  const uint16_t *vram=PpuRenderVram(g_ppu);
  if(s_retained.parallax.valid) {
    unsigned raster[4]={g_ppu->hScroll[0]&1023,g_ppu->vScroll[0]&1023,
        g_ppu->hScroll[1]&1023,g_ppu->vScroll[1]&1023};
    if(s_retained.room!=7 || !LagoonWideParallaxPacket_Match(&s_retained.parallax,
        vram,g_rom,0x100000,RtlStateGeneration(),s_retained.camera,raster))return 0;
  }
  int torch_frame=LagoonWideTorches_Match(&s_retained.torches,vram,
      RtlStateGeneration(),s_retained.room,s_retained.bg_source);
  for(unsigned tile=0;tile<1024;++tile)if(s_retained.used[tile] &&
      memcmp(vram+tile*16,s_retained.characters+tile*16,32) &&
      !(tile>=944 && tile<944+LagoonWideTorches_Bytes(s_retained.room,s_retained.bg_source)/32 && torch_frame>=0))return 0;
  uint16_t expected[720];
  for(unsigned layer=0;layer<2;++layer) {
    if(layer==1 && s_retained.parallax.valid)continue;
    int cx=s_retained.camera[layer*2],cy=s_retained.camera[layer*2+1];
    int wx=cx+(((g_ppu->hScroll[layer]&1023)-cx+512)&1023)-512;
    int wy=cy+(((g_ppu->vScroll[layer]&1023)-cy+512)&1023)-512;
    if(wx<0 || wy<0)return 0;
    int tx=wx/8+1,ty=wy/8+1;
    for(unsigned y=0;y<24;++y)for(unsigned x=0;x<30;++x)
      expected[y*30+x]=(uint16_t)LagoonWideRetainedMap_Tile(&s_retained.map,layer,
          (tx+(int)x)*8,(ty+(int)y)*8);
    if(!LagoonWideEntities_WorldTilemap(vram,0x7000+layer*0x800,tx,ty,expected))return 0;
  }
  return 1;
}
static void retain_completed_scene(void) {
  if(s_using_retained || !s_world_chars_ready || g_snesrecomp_last_hdmaen ||
      !native_world_map())return;
  s_retained.valid=0;
  s_retained.parallax.valid=0;
  if(room07_parallax()) {
    unsigned camera[4];for(unsigned i=0;i<4;++i)camera[i]=word(0x480+i*2);
    if(!LagoonWideParallaxPacket_Capture(&s_retained.parallax,PpuRenderVram(g_ppu),
        g_rom,0x100000,RtlStateGeneration(),camera))return;
  }
  for(unsigned layer=0;layer<2;++layer) {
    /* Every audited map lives in these two complete retained WRAM pages. */
    memcpy(s_retained.map.maps[layer],g_ram+0x10000+layer*0x4000,0x4000);
    unsigned definitions=pointer(layer?0x75:0x69);
    for(unsigned i=0;i<1024;++i)
      s_retained.map.definitions[layer][i]=(uint16_t)(byte_at(definitions+i*2)|
          (byte_at(definitions+i*2+1)<<8));
  }
  s_retained.map.fill=g_ram[0x52];
  memcpy(s_retained.characters,PpuRenderVram(g_ppu),sizeof(s_retained.characters));
  memcpy(s_retained.used,s_world_char_used,sizeof(s_retained.used));
  s_retained.torches=s_torches;
  s_retained.room=word(0x48e);s_retained.width=word(0x53)*8;s_retained.height=word(0x55)*8;
  s_retained.bg_source=pointer(0x57);
  for(unsigned i=0;i<4;++i)s_retained.camera[i]=word(0x480+i*2);
  s_retained.generation=RtlStateGeneration();
  LagoonWideEntities_RetainScene();s_retained.valid=1;
}
static int capture_world_characters(const uint8_t *upload) {
    /* BG3 map $3C00 aliases unused BG1 character slots. Own only characters
     * actually referenced by the complete room, including its fill tile. */
    memset(s_world_char_used,0,sizeof(s_world_char_used));
    unsigned tw=word(0x53),th=word(0x55);
    if(tw>256)tw=256;if(th>256)th=256;
    for(unsigned layer=0;layer<2;++layer) {
      if(layer==1 && room07_parallax()) {
        const uint16_t *vram=PpuRenderVram(g_ppu);
        for(unsigned i=0;i<0x800;++i)s_world_char_used[vram[0x7800+i]&1023]=1;
        continue;
      }
      s_world_char_used[map_tile(layer,-16,-16)&1023]=1;
      for(unsigned y=0;y<th;++y)for(unsigned x=0;x<tw;++x)
        s_world_char_used[map_tile(layer,(int)x*8,(int)y*8)&1023]=1;
    }
    unsigned torch_bytes=LagoonWideTorches_Bytes(word(0x48e),pointer(0x57));
    if(upload) {
      for(unsigned tile=kLagoonWideBackgroundBytes/32;tile<1024;++tile)
        if(s_world_char_used[tile] && (tile<944 || tile>=944+torch_bytes/32))return 0;
      memset(s_world_chars,0,sizeof(s_world_chars));
      memcpy(s_world_chars,upload,kLagoonWideBackgroundBytes);
      int torches=0;
      for(unsigned tile=944;tile<944+torch_bytes/32;++tile)torches|=s_world_char_used[tile];
      if(torches) {
        int frame=LagoonWideTorches_Match(&s_torches,PpuRenderVram(g_ppu),
            RtlStateGeneration(),word(0x48e),pointer(0x57));
        if(frame<0)return 0;
        memcpy(s_world_chars+0x3B00,s_torches.bytes[frame],torch_bytes);
      }
    } else memcpy(s_world_chars,PpuRenderVram(g_ppu),sizeof(s_world_chars));
    if(!LagoonWideEntities_WorldCharacters(PpuRenderVram(g_ppu),s_world_chars,s_world_char_used))return 0;
    s_world_char_key=world_resource_key();s_world_chars_ready=1;return 1;
}
static void refresh_torches(void) {
  if(!s_world_chars_ready || s_world_char_key!=world_resource_key())return;
  int frame=LagoonWideTorches_Match(&s_torches,PpuRenderVram(g_ppu),
      RtlStateGeneration(),word(0x48e),pointer(0x57));
  if(frame>=0)memcpy(s_world_chars+0x3B00,s_torches.bytes[frame],
      LagoonWideTorches_Bytes(word(0x48e),pointer(0x57)));
}
static int upload_characters_owned(void) {
  for(unsigned tile=0;tile<1024;++tile)if(s_world_char_used[tile]) {
    if(tile<kLagoonWideBackgroundBytes/32) {
      if(memcmp(s_world_chars+tile*16,s_background.bytes+tile*32,32))return 0;
    } else if(tile<944 || tile>=944+LagoonWideTorches_Bytes(word(0x48e),pointer(0x57))/32 ||
        LagoonWideTorches_Match(&s_torches,PpuRenderVram(g_ppu),
            RtlStateGeneration(),word(0x48e),pointer(0x57))<0)return 0;
  }
  return 1;
}
static int world_scene_geometry(void) {
  if(s_using_retained)return retained_scene_matches();
  if(room07_parallax() && (PPU_bgTileAdr(g_ppu,0) || PPU_bgTileAdr(g_ppu,1) ||
      !PPU_bgTilemapWider(g_ppu,0) || PPU_bgTilemapHigher(g_ppu,0) ||
      PPU_bgTilemapWider(g_ppu,1) || !PPU_bgTilemapHigher(g_ppu,1) ||
      !LagoonWideEntities_ParallaxScroll(g_ppu->hScroll[0]&1023,g_ppu->vScroll[0]&1023,
          g_ppu->hScroll[1]&1023,g_ppu->vScroll[1]&1023) ||
      !LagoonWideEntities_ParallaxMap(PpuRenderVram(g_ppu),g_rom,0x100000)))return 0;
  return (g_ram[0x500]&0xC0)==0xC0 && (!g_ram[0x50] || room07_parallax()) &&
      !(g_ppu->mosaic&3) && (g_ppu->bgmode&7)==1 &&
      !PPU_bigTiles(g_ppu,0) && !PPU_bigTiles(g_ppu,1) &&
      PPU_bgTilemapAdr(g_ppu,0)==0x7000 && PPU_bgTilemapAdr(g_ppu,1)==0x7800 &&
      valid_pointer(pointer(0x88)) && valid_pointer(pointer(0x8b)) &&
      valid_pointer(pointer(0x69)) && valid_pointer(pointer(0x75));
}
static int unsafe_modal_ui(void) {
  if(s_using_retained)return !retained_scene_matches();
  if(!has_modal_ui())return 0;
  return !s_world_chars_ready || s_world_char_key!=world_resource_key() ||
      !LagoonWideEntities_WorldCharacters(PpuRenderVram(g_ppu),s_world_chars,s_world_char_used) ||
      !native_world_map();
}

static void menu_line(Ppu *ppu,uint y,bool sub,void *context) {
  (void)y;(void)context;
  LagoonWideEntities_BlankMargins(ppu->bgBuffers[sub].data,kPpuExtraLeftRight,
                                  ppu->extraLeftCur,ppu->extraRightCur);
}
static void native_ui_frame(void) {
  s_native_fallback=1;
  PpuSetWidescreenLayerMask(g_ppu,0);PpuSetWidescreenLayerClamp(g_ppu,15);
  PpuSetWidescreenShadowNativeExactMask(g_ppu,0);PpuSetExtraObjects(g_ppu,NULL,0);
  PpuSetWidescreenLineEnhancer(g_ppu,menu_line,NULL);
}
void LagoonWide_OnBackgroundDma(unsigned bank,unsigned address,unsigned destination,unsigned length) {
  if(!g_ppu || !g_config.widescreen)return;
  if(s_background.generation!=RtlStateGeneration())LagoonWideBackground_Reset(&s_background);
  if(s_retained.valid)
    LagoonWideTorches_Begin(&s_retained.torches,RtlStateGeneration(),s_retained.room,s_retained.bg_source,
        bank,address,destination,length);
  LagoonWideTorches_Begin(&s_torches,RtlStateGeneration(),word(0x48e),pointer(0x57),
      bank,address,destination,length);
  int begins=bank==0x7F && address==0x7F80 && destination==0 && length==0x1000;
  if(begins) {
    if(s_retained.valid){s_retained_invalidation_kind=2;s_retained_invalidation_address=destination;}
    s_retained.valid=0;
  }
  /* Inventory uploads cannot claim a fresh world through stale room metadata. */
  if(begins && !g_ram[0x100e]) {s_background.left=0;return;}
  LagoonWideBackground_Begin(&s_background,RtlStateGeneration(),word(0x48e),pointer(0x57),
      bank,address,destination,length);
  if(begins) {
    LagoonWideTorches_Reset(&s_torches);
    s_world_chars_ready=s_initial_world_authority=0;s_scene_gate.fields=0;
    native_ui_frame();
  }
}

void LagoonWide_OnVramWrite(unsigned byte_address) {
  if(!g_ppu || !g_config.widescreen) return;
  if(LagoonWideParallaxQueue_Pending(&s_parallax_queue)) {
    unsigned value=(g_ppu->vram[byte_address/2]>>((byte_address&1)*8))&255;
    if(s_retained.valid && LagoonWideParallaxQueue_Write(&s_parallax_queue,
        byte_address,value,RtlStateGeneration()))return;
    s_parallax_queue.valid=0;s_retained.valid=0;native_ui_frame();
  }
  /* A partially replaced static parallax page has no authority this field,
   * even if its first words still resemble the previous complete page. */
  if(room07_parallax() && byte_address>=0xF000)native_ui_frame();
  /* A new character/map upload ends outgoing authority even if bytes happen
   * to resemble a common corridor in the previous room. */
  if(byte_address<kLagoonWideBackgroundBytes || byte_address>=0xE000) {
    if(s_retained.valid){s_retained_invalidation_kind=3;s_retained_invalidation_address=byte_address;}
    s_retained.valid=0;
  }
  if(s_retained.valid && (s_retained.torches.left ||
      (byte_address>=kLagoonWideTorchAddress && byte_address<kLagoonWideTorchAddress+LagoonWideTorches_Bytes(s_retained.room,s_retained.bg_source))))
    LagoonWideTorches_Write(&s_retained.torches,byte_address,
        (uint8_t)(g_ppu->vram[byte_address/2]>>((byte_address&1)*8)),g_rom,0x100000);
  if(s_background.generation!=RtlStateGeneration())LagoonWideBackground_Reset(&s_background);
  if(s_background.left || byte_address<kLagoonWideBackgroundBytes)
    LagoonWideBackground_Write(&s_background,byte_address,
        (uint8_t)(g_ppu->vram[byte_address/2]>>((byte_address&1)*8)));
  if(s_torches.left || (byte_address>=kLagoonWideTorchAddress &&
      byte_address<kLagoonWideTorchAddress+LagoonWideTorches_Bytes(word(0x48e),pointer(0x57))))
    LagoonWideTorches_Write(&s_torches,byte_address,
        (uint8_t)(g_ppu->vram[byte_address/2]>>((byte_address&1)*8)),g_rom,0x100000);
  if(g_ram[0x100e] && byte_address>=0xE000 && byte_address<0x10000)
    s_initial_world_authority=0;
  /* A recognized torch DMA may be partial at this callback. The completed
   * packet is checked again before presentation; never expose partial data. */
  if(s_using_retained && s_retained.valid && byte_address>=kLagoonWideTorchAddress &&
      byte_address<kLagoonWideTorchAddress+LagoonWideTorches_Bytes(word(0x48e),pointer(0x57)))return;
  /* Room loading can begin after the frame policy was selected. Catch the
   * first actual graphics write before a changed atlas can reach margins. */
  if(!audited_room_ready()) { s_scene_gate.fields=0; s_world_chars_ready=0; native_ui_frame(); return; }
  unsigned word_address=byte_address/2;
  /* A text/font DMA does not own the world layers. A changed BG1/2
   * character word does; reject it immediately, and recheck completed maps
   * in EndFrame before the mutable compositor can expose either margin. */
  if(word_address<0x4000 && s_world_char_used[word_address/16] && has_modal_ui() &&
      (!s_world_chars_ready || PpuRenderVram(g_ppu)[word_address]!=s_world_chars[word_address]))
    native_ui_frame();
}

void LagoonWide_OnParallaxQueueDma(unsigned length) {
  /* $00:8809..8843 drains the original34-word top-edge map queue at NMI;
   * the authored07->06 doorway may change RAM later in that same field.
   * Keep only this proven old-world job; never replay or skip its DMA. */
  if(!g_ppu || !g_config.widescreen)return;
  if(!s_retained.valid || !s_retained.parallax.valid ||
      !((word(0x48e)==7 && room07_parallax()) ||
        (word(0x48e)==6 && audited_room_resources())))return;
  int was_pending=LagoonWideParallaxQueue_Pending(&s_parallax_queue);
  if(!retained_scene_matches() || !LagoonWideParallaxQueue_Begin(&s_parallax_queue,
      &s_retained.map,g_ram+0x2600,length,s_retained.camera,RtlStateGeneration())) {
    s_parallax_queue.valid=0;s_retained.valid=0;
    /* Other ordinary scroll queues keep their existing live-world policy.
     * They can never lend an outgoing packet to a later room or loader. */
    if(was_pending || s_using_retained || word(0x48e)!=7 || g_ram[0x100e])native_ui_frame();
  }
}

void LagoonWide_BeginFrame(unsigned number) {
  (void)number;
  /* External/debugger loads can advance the timeline without the desktop
   * menu's on_reset hook. Never let pre-load shadow cells cross that jump,
   * including while this scene has no supported widescreen layout. */
  if (s_state_generation != RtlStateGeneration()) LagoonWide_Reset();
  if (!g_ppu) return;
  s_using_retained=0;
  if((g_ram[0x100e] || word(0x48e)!=s_retained.room) && retained_scene_matches())
    s_using_retained=1;
  s_native_fallback=0;
  s_outdoor_frame=0;
  memset(s_row_known,0,sizeof(s_row_known));
  if(!audited_room_resources() || (s_world_chars_ready && s_world_char_key!=world_resource_key()))
    s_world_chars_ready=0;
  s_initial_world_authority=0;
  refresh_torches();
  /* $100E spans the final fade, after the full world graphics/map upload.
   * Authorize that first visible world from actual completed bytes, never
   * from the loading flag or a prior quiet/text-free frame alone. */
  if(audited_room_resources() && background_upload_ready() && world_scene_geometry() && native_world_map()) {
    if(!s_world_chars_ready)capture_world_characters(s_background.bytes);
    if(s_world_chars_ready && s_world_char_key==world_resource_key() && upload_characters_owned() &&
        LagoonWideEntities_WorldCharacters(PpuRenderVram(g_ppu),s_world_chars,s_world_char_used))
      s_initial_world_authority=1;
  }
  /* A successful full v3 state load restores the completed raster together
   * with RAM and all PPU state. Its exact native map can prove coherence on
   * the first field, unlike a room loader whose RAM can run ahead of raster.
   * Do not carry this exception through an ordinary reset or changed scene. */
  uint64_t load=LagoonState_LoadGeneration();
  int fresh_load=load!=s_seen_load_generation;
  s_seen_load_generation=load; /* One classification attempt; never lend to a later room. */
  if(fresh_load && !g_ram[0x100e] && audited_room_resources() &&
      world_scene_geometry() && native_world_map() &&
      capture_world_characters(NULL)) {
    s_scene_gate.generation=RtlStateGeneration();s_scene_gate.key=word(0x48e)+1;
    s_scene_gate.fields=3;s_seen_load_generation=load;
  }
  int modal_ui=unsafe_modal_ui();
  PpuSetWidescreenLineEnhancer(g_ppu,NULL,NULL);
  /* Lagoon parks unused OAM at x=320, leaving old tile/y bytes intact. The
   * generic widened decoder would resurrect them as ghost menu/HUD sprites.
   * Until a title-specific off-screen object producer exists, only native
   * OAM and sprites straddling the native edge are authorized. */
  static const uint8_t no_margin_objects[16] = {0};
  PpuWsSetOamLeftHints(g_ppu, no_margin_objects);
  PpuWsSetOamRightHints(g_ppu, no_margin_objects);
  /* Clamp title/dialogue/menu layers; allow ordinary retained-map rooms only.
   * Parallax rooms ($50 != 0) need a different BG2 resolver. */
  PpuSetWidescreenLayerMask(g_ppu, 0);
  PpuSetWidescreenLayerClamp(g_ppu, 15);
  /* Retained RAM can switch to the next room before the completed raster
   * stops showing the old one. Require two prior ready field boundaries. */
  int audited_room=s_using_retained || LagoonWideEntities_SceneSettled(&s_scene_gate,
      audited_room_ready(),word(0x48e)+1,RtlStateGeneration());
  /*  sets100E before the room loader;  clears it
   * only after map, graphics, palette and entities are installed. */
  int active=audited_room && !modal_ui && g_config.widescreen &&
      g_ppu->extraLeftRight && world_scene_geometry();
  unsigned key = view_room()<<24;
  if (!active || !s_was_active || key != s_room_key) WsShadowReset();
  s_was_active = active;
  s_room_key = key;
  if(s_using_retained)LagoonWideEntities_BeginRetained(active);
  else LagoonWideEntities_BeginFrame(active);
  if (!active) { if(g_config.widescreen)native_ui_frame(); return; }
  if(!s_world_chars_ready && !has_modal_ui() && native_world_map())
    capture_world_characters(NULL);
  s_outdoor_frame=room_bounds_enabled(view_room());
  s_extent_width=s_using_retained?s_retained.width:word(0x53)*8;
  s_extent_height=s_using_retained?s_retained.height:word(0x55)*8;
  PpuSetWidescreenLayerMask(g_ppu, 3);
  PpuSetWidescreenShadowNativeExactMask(g_ppu, view_parallax()?1:3);
  PpuSetWidescreenLayerClamp(g_ppu, 12); /* BG3 HUD remains native centered */
  for (int layer = 0; layer < 2; ++layer) {
    /* BG2 is fully owned static32x64 PPU data with its own per-line scroll.
     * Leave its exact native wrapping active; never seed it from BG1 maps. */
    if(layer==1 && view_parallax())continue;
    /* Preserve the actual PPU's phase, borrowing only lost high bits from
     * WRAM. The wrapped difference handles a one-frame boundary disagreement. */
    int camera_x = view_camera(layer*2), camera_y = view_camera(layer*2+1);
    int sx = g_ppu->hScroll[layer] & 0x3ff, sy = g_ppu->vScroll[layer] & 0x3ff;
    int wx = camera_x + ((sx - camera_x + 512) & 1023) - 512;
    int wy = camera_y + ((sy - camera_y + 512) & 1023) - 512;
    if (wx < 0) wx = sx;
    if (wy < 0) wy = sy;
    WsShadowSetWorld(layer, wx, wy);
    WsShadowSetScroll(layer, sx, sy);
    WsShadowSetBlankTile(layer, view_tile(layer, -16, -16));
  }
  WsShadowFrame(g_ppu);
  /* These ordinary-room retained maps are exact sources, not guesses about
   * circular VRAM headroom. Captured blank/uninitialized slots must yield to
   * the current room map. Native fragments still use actual guest VRAM. */
  for (int layer = 0; layer < 2; ++layer) {
    if(layer==1 && view_parallax())continue;
    int wx = WsShadowWorldX(layer), wy = WsShadowWorldY(layer);
    int x0 = (wx - g_ppu->extraLeftRight - 8) / 8;
    int x1 = (wx + 256 + g_ppu->extraLeftRight + 8) / 8;
    for (int y = wy / 8; y <= (wy + 232) / 8 && y < 512; ++y)
      for (int x = x0 < 0 ? 0 : x0; x <= x1 && x < 4096; ++x)
        WsShadowForceTile(layer, x, y, view_tile(layer, x * 8, y * 8));
  }
}

/* Observe the actual raster, without adding a PPU line enhancer (which has
 * its own BG viewport semantics). SNES output row0 samples scrollY+1. */
void LagoonWide_ObserveRaster(const Ppu *ppu,unsigned line,void *context) {
  (void)context;
  LagoonSceneRenderer_ObserveRaster(ppu,line,g_ram,g_rom,0x100000,RtlStateGeneration(),
      !s_using_retained && s_was_active && !s_native_fallback && s_world_chars_ready &&
      (word(0x48e)==0 || word(0x48e)==3 || word(0x48e)==4 ||
       word(0x48e)==5 || word(0x48e)==6 || word(0x48e)==7) && !g_snesrecomp_last_hdmaen);
  if(!s_outdoor_frame || line<1 || line>224)return;
  if((s_using_retained ? !s_retained.valid : !audited_room_ready()) || !room_bounds_enabled(view_room()))return;
  if(view_parallax()) {
    if(!LagoonWideEntities_ParallaxScroll(ppu->hScroll[0]&1023,ppu->vScroll[0]&1023,
        ppu->hScroll[1]&1023,ppu->vScroll[1]&1023))return;
  } else if(ppu->hScroll[0]!=ppu->hScroll[1] || ppu->vScroll[0]!=ppu->vScroll[1])return;
  int camera_x=(int)view_camera(0),camera_y=(int)view_camera(1);
  int sx=ppu->hScroll[0]&1023,sy=ppu->vScroll[0]&1023;
  s_row_world_x[line-1]=camera_x+((sx-camera_x+512)&1023)-512;
  s_row_world_y[line-1]=camera_y+((sy-camera_y+512)&1023)-512+(int)line;
  s_row_known[line-1]=1;
}

static void report_outdoor(unsigned frame) {
  const char *base=getenv("LAGOON_WIDE_ENTITY_REPORT");
  if(!base || (!s_outdoor_frame && !room_bounds_enabled(view_room())))return;
  char path[1200];if(snprintf(path,sizeof(path),"%s.bounds.tsv",base)>=(int)sizeof(path))return;
  FILE *f=fopen(path,"ab");if(!f)return;
  fseek(f,0,SEEK_END);
  if(ftell(f)==0)fprintf(f,"frame\troom\tfallback\tmask\tsettled\tknown\tx0\ty0\tx223\ty223\txchanges\tychanges\twidth\theight\n");
  unsigned known=0,xc=0,yc=0;
  for(unsigned y=0;y<224;++y) {
    known+=s_row_known[y]!=0;
    if(y && s_row_world_x[y]!=s_row_world_x[0])++xc;
    if(s_row_world_y[y]!=s_row_world_y[0]+(int)y)++yc;
  }
  fprintf(f,"%u\t%u\t%d\t%d\t%u\t%u\t%d\t%d\t%d\t%d\t%u\t%u\t%u\t%u\n",
      frame,view_room(),s_present_fallback,s_outdoor_present,s_scene_gate.fields,known,
      s_row_world_x[0],s_row_world_y[0],s_row_world_x[223],s_row_world_y[223],xc,yc,
      s_extent_width,s_extent_height);
  fclose(f);
}

static void report_scene(unsigned frame) {
  const char *base=getenv("LAGOON_WIDE_ENTITY_REPORT");if(!base)return;
  char path[1200];if(snprintf(path,sizeof(path),"%s.scene.tsv",base)>=(int)sizeof(path))return;
  FILE *f=fopen(path,"ab");if(!f)return;fseek(f,0,SEEK_END);
  if(!ftell(f))fprintf(f,"frame\troom\tloading\tbg_complete\tbg_ready\tchars_ready\tearly_authority\tmap_ready\tmodal\tactive\tfallback\tbrightness\ttm\ttorch_seen\ttorch_render\tretained_active\tretained_room\tretained_valid\tparallax_valid\tretained_actor_valid\tretained_invalidation_kind\tretained_invalidation_address\tactor_refusal_kind\tactor_refusal_address\tactor_captured\tactor_current\tactor_native_anchors\n");
  int map_ready=world_scene_geometry() && native_world_map();
  unsigned actor_kind,actor_address,actor_captured,actor_current;
  LagoonWideEntities_RetainedDiagnostic(&actor_kind,&actor_address,&actor_captured,&actor_current);
  fprintf(f,"%u\t%u\t%u\t%u\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%u\t%u\t%u\t%d\t%d\t%u\t%d\t%d\t%d\t%u\t%u\t%u\t%u\t%u\t%u\t%d\n",frame,
      word(0x48e),g_ram[0x100e],s_background.completed,background_upload_ready(),
      s_world_chars_ready,s_initial_world_authority,map_ready,has_modal_ui(),s_was_active,
      s_present_fallback,g_ppu->inidisp,g_ppu->screenEnabled[0],s_torches.seen,
      LagoonWideTorches_Match(&s_torches,PpuRenderVram(g_ppu),RtlStateGeneration(),
          word(0x48e),pointer(0x57)),s_using_retained,s_retained.room,
      s_retained.valid,s_retained.parallax.valid,LagoonWideEntities_RetainedSceneValid(),
      s_retained_invalidation_kind,s_retained_invalidation_address,
      actor_kind,actor_address,actor_captured,actor_current,
      LagoonWideEntities_RetainedNativeAnchors());fclose(f);
}

/* Keep the completed field's policy with the presentation boundary. The
 * source field is const; the official compositor owns mutable destination. */
void LagoonWide_EndFrame(const uint8_t *field,unsigned number) {
  (void)field;
  refresh_torches();
  s_present_fallback=s_native_fallback || LagoonWideParallaxQueue_Pending(&s_parallax_queue) ||
      !audited_room_ready() || unsafe_modal_ui() ||
      (word(0x48e)==7 && !world_scene_geometry()) ||
      (!s_using_retained && g_ram[0x100e] && (!native_world_map() || !s_world_chars_ready ||
      !LagoonWideEntities_WorldCharacters(PpuRenderVram(g_ppu),s_world_chars,s_world_char_used)));
  s_outdoor_present=s_outdoor_frame && !s_present_fallback;
  if(s_outdoor_present)for(unsigned y=0;y<224;++y)
    if(!s_row_known[y]) {s_present_fallback=1;s_outdoor_present=0;break;}
  LagoonSceneRenderer_EndFrame(number,RtlStateGeneration(),
      !s_present_fallback && !s_using_retained && s_was_active &&
      (word(0x48e)==0 || word(0x48e)==3 || word(0x48e)==4 ||
       word(0x48e)==5 || word(0x48e)==6 || word(0x48e)==7) && !g_snesrecomp_last_hdmaen);
  if(!s_present_fallback && s_was_active)retain_completed_scene();
  report_outdoor(number);
  report_scene(number);
}
int LagoonWide_DrawFrame(uint8_t *dst,size_t pitch,const uint8_t *field,
    int width,int height,double alpha) {
  (void)alpha;
  if(!g_config.widescreen)return 0;
  RtlWidescreenPresent(dst,pitch,field,width,height);
  if(s_present_fallback) {
    if(!LagoonMenuPresentation_Draw(dst,pitch,field,(size_t)width*4,
        (unsigned)width,(unsigned)height,g_ppu,g_ram,g_config.widescreen))
      LagoonWideEntities_BlankField(dst,pitch,(unsigned)width,(unsigned)height);
  }
  else if(s_outdoor_present)for(unsigned y=0;y<(unsigned)height && y<224;++y)
    LagoonWideEntities_MaskRoomRow(dst+y*pitch,(unsigned)width,
        s_row_world_x[y],s_row_world_y[y],s_extent_width,s_extent_height);
  LagoonSceneRenderer_TryPresent(dst,pitch,dst,pitch,(unsigned)width,(unsigned)height,RtlStateGeneration());
  return 1;
}
