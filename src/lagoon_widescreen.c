/* Presentation-only expansion. The $01:8311 map resolver is reproduced as
 * read-only host lookups, never by moving the guest camera or replaying DMA.
 * No ROM data, graphics, or generated guest code belongs in this source. */
#include "lagoon_widescreen.h"
#include "lagoon_wide_entities.h"
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
static uint64_t s_state_generation;
static const char *s_config_path = "config.ini";

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
  LagoonWideEntities_Reset();
  WsShadowReset();
  s_was_active = 0;
  s_outdoor_frame=s_outdoor_present=0;
  memset(s_row_known,0,sizeof(s_row_known));
  memset(&s_scene_gate,0,sizeof(s_scene_gate));
  s_room_key = 0;
  s_state_generation = RtlStateGeneration();
}

static int audited_room_ready(void) {
  return !g_ram[0x100e] && (word(0x48e)!=2 || (word(0x53)==64 && word(0x55)==64)) &&
      LagoonWideEntities_AuditedRoom(word(0x48e),pointer(0x57),pointer(0x5d),
      pointer(0x69),pointer(0x75),pointer(0x88),pointer(0x8b));
}
static int has_modal_ui(void) {
  return ((g_ppu->screenEnabled[0]|g_ppu->screenEnabled[1])&4) &&
      (g_ppu->hScroll[2] || g_ppu->vScroll[2] ||
       LagoonWideEntities_MenuOverlay(PpuRenderVram(g_ppu),PPU_bgTilemapAdr(g_ppu,2),PPU_bgTileAdr(g_ppu,2)));
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
void LagoonWide_OnVramWrite(unsigned byte_address) {
  if(!g_ppu || !g_config.widescreen) return;
  /* Room loading can begin after the frame policy was selected. Catch the
   * first actual graphics write before a changed atlas can reach margins. */
  if(!audited_room_ready()) { s_scene_gate.fields=0; native_ui_frame(); return; }
  unsigned word_address=byte_address/2;
  unsigned map=PPU_bgTilemapAdr(g_ppu,2),chars=PPU_bgTileAdr(g_ppu,2);
  if(((word_address-map)<24*32 || (word_address-chars)<0x2000) && has_modal_ui())
    native_ui_frame();
}

void LagoonWide_BeginFrame(unsigned number) {
  (void)number;
  /* External/debugger loads can advance the timeline without the desktop
   * menu's on_reset hook. Never let pre-load shadow cells cross that jump,
   * including while this scene has no supported widescreen layout. */
  if (s_state_generation != RtlStateGeneration()) LagoonWide_Reset();
  if (!g_ppu) return;
  s_native_fallback=0;
  s_outdoor_frame=0;
  memset(s_row_known,0,sizeof(s_row_known));
  int modal_ui=has_modal_ui();
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
  int audited_room=LagoonWideEntities_SceneSettled(&s_scene_gate,
      audited_room_ready(),word(0x48e)+1,RtlStateGeneration());
  /*  sets100E before the room loader;  clears it
   * only after map, graphics, palette and entities are installed. */
  int active = audited_room && !g_ram[0x100e] && !modal_ui && g_config.widescreen && g_ppu->extraLeftRight &&
      (g_ram[0x500] & 0xc0) == 0xc0 && g_ram[0x50] == 0 &&
      !(g_ppu->mosaic & 3) &&
      (g_ppu->bgmode & 7) == 1 && !PPU_bigTiles(g_ppu, 0) &&
      !PPU_bigTiles(g_ppu, 1) && PPU_bgTilemapAdr(g_ppu, 0) == 0x7000 &&
      PPU_bgTilemapAdr(g_ppu, 1) == 0x7800 &&
      valid_pointer(pointer(0x88)) && valid_pointer(pointer(0x8b)) &&
      valid_pointer(pointer(0x69)) && valid_pointer(pointer(0x75));
  unsigned key = pointer(0x69) ^ pointer(0x75) ^ (g_ram[0x48e] << 24);
  if (!active || !s_was_active || key != s_room_key) WsShadowReset();
  s_was_active = active;
  s_room_key = key;
  LagoonWideEntities_BeginFrame(active);
  if (!active) { if(g_config.widescreen)native_ui_frame(); return; }
  s_outdoor_frame=word(0x48e)==2;
  s_extent_width=word(0x53)*8;s_extent_height=word(0x55)*8;
  PpuSetWidescreenLayerMask(g_ppu, 3);
  PpuSetWidescreenShadowNativeExactMask(g_ppu, 3);
  PpuSetWidescreenLayerClamp(g_ppu, 12); /* BG3 HUD remains native centered */
  for (int layer = 0; layer < 2; ++layer) {
    /* Preserve the actual PPU's phase, borrowing only lost high bits from
     * WRAM. The wrapped difference handles a one-frame boundary disagreement. */
    int camera_x = word(0x480 + layer * 4), camera_y = word(0x482 + layer * 4);
    int sx = g_ppu->hScroll[layer] & 0x3ff, sy = g_ppu->vScroll[layer] & 0x3ff;
    int wx = camera_x + ((sx - camera_x + 512) & 1023) - 512;
    int wy = camera_y + ((sy - camera_y + 512) & 1023) - 512;
    if (wx < 0) wx = sx;
    if (wy < 0) wy = sy;
    WsShadowSetWorld(layer, wx, wy);
    WsShadowSetScroll(layer, sx, sy);
    WsShadowSetBlankTile(layer, map_tile(layer, -16, -16));
  }
  WsShadowFrame(g_ppu);
  /* These ordinary-room retained maps are exact sources, not guesses about
   * circular VRAM headroom. Captured blank/uninitialized slots must yield to
   * the current room map. Native fragments still use actual guest VRAM. */
  for (int layer = 0; layer < 2; ++layer) {
    int wx = WsShadowWorldX(layer), wy = WsShadowWorldY(layer);
    int x0 = (wx - g_ppu->extraLeftRight - 8) / 8;
    int x1 = (wx + 256 + g_ppu->extraLeftRight + 8) / 8;
    for (int y = wy / 8; y <= (wy + 232) / 8 && y < 512; ++y)
      for (int x = x0 < 0 ? 0 : x0; x <= x1 && x < 4096; ++x)
        WsShadowForceTile(layer, x, y, map_tile(layer, x * 8, y * 8));
  }
}

/* Observe the actual raster, without adding a PPU line enhancer (which has
 * its own BG viewport semantics). SNES output row0 samples scrollY+1. */
void LagoonWide_ObserveRaster(const Ppu *ppu,unsigned line,void *context) {
  (void)context;
  if(!s_outdoor_frame || line<1 || line>224)return;
  if(!audited_room_ready() || word(0x48e)!=2 ||
      ppu->hScroll[0]!=ppu->hScroll[1] || ppu->vScroll[0]!=ppu->vScroll[1])return;
  int camera_x=(int)word(0x480),camera_y=(int)word(0x482);
  int sx=ppu->hScroll[0]&1023,sy=ppu->vScroll[0]&1023;
  s_row_world_x[line-1]=camera_x+((sx-camera_x+512)&1023)-512;
  s_row_world_y[line-1]=camera_y+((sy-camera_y+512)&1023)-512+(int)line;
  s_row_known[line-1]=1;
}

static void report_outdoor(unsigned frame) {
  const char *base=getenv("LAGOON_WIDE_ENTITY_REPORT");
  if(!base || (!s_outdoor_frame && word(0x48e)!=2))return;
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
      frame,word(0x48e),s_present_fallback,s_outdoor_present,s_scene_gate.fields,known,
      s_row_world_x[0],s_row_world_y[0],s_row_world_x[223],s_row_world_y[223],xc,yc,
      s_extent_width,s_extent_height);
  fclose(f);
}

/* Keep the completed field's policy with the presentation boundary. The
 * source field is const; the official compositor owns mutable destination. */
void LagoonWide_EndFrame(const uint8_t *field,unsigned number) {
  (void)field;
  s_present_fallback=s_native_fallback || !audited_room_ready() || has_modal_ui();
  s_outdoor_present=s_outdoor_frame && !s_present_fallback;
  if(s_outdoor_present)for(unsigned y=0;y<224;++y)
    if(!s_row_known[y]) {s_present_fallback=1;s_outdoor_present=0;break;}
  report_outdoor(number);
}
int LagoonWide_DrawFrame(uint8_t *dst,size_t pitch,const uint8_t *field,
    int width,int height,double alpha) {
  (void)alpha;
  if(!g_config.widescreen || (!s_present_fallback && !s_outdoor_present))return 0;
  RtlWidescreenPresent(dst,pitch,field,width,height);
  if(s_present_fallback)LagoonWideEntities_BlankField(dst,pitch,(unsigned)width,(unsigned)height);
  else for(unsigned y=0;y<(unsigned)height && y<224;++y)
    LagoonWideEntities_MaskRoomRow(dst+y*pitch,(unsigned)width,
        s_row_world_x[y],s_row_world_y[y],s_extent_width,s_extent_height);
  return 1;
}
