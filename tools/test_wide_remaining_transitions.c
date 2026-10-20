/* Actual production outgoing/fade packet with fresh real loader maps.
 * Changing only the descriptor below is a labelled protocol fixture, not a
 * live door transition; actual original-door off/auto routes are separate. */
#include "snes/ws_shadow.h"
#define WsShadowOnVramWrite fixture_WsShadowOnVramWrite
#define WsShadowTile fixture_WsShadowTile
#define WsShadowLayerActive fixture_WsShadowLayerActive
#define WsShadowWorldX fixture_WsShadowWorldX
#define WsShadowWorldY fixture_WsShadowWorldY
#define WsShadowPresentWorldY fixture_WsShadowPresentWorldY
#define WsShadowScrollX fixture_WsShadowScrollX
#define WsShadowScrollY fixture_WsShadowScrollY
#define main ppu_transition_fixture_main
#include "../snesrecomp/tests/ppu/ppu_extra_object_test.c"
#undef main
#undef WsShadowOnVramWrite
#undef WsShadowTile
#undef WsShadowLayerActive
#undef WsShadowWorldX
#undef WsShadowWorldY
#undef WsShadowPresentWorldY
#undef WsShadowScrollX
#undef WsShadowScrollY

#include "config.h"
#include "snes/dma.h"
Config g_config;static uint8_t own_rom[0x100000];const uint8_t*g_rom=own_rom;
uint8_t g_ram[0x20000];Ppu*g_ppu=&p;uint8_t g_snesrecomp_last_hdmaen;
static uint64_t generation=29;uint64_t RtlStateGeneration(void){return generation;}
void dma_set_start_observer(DmaStartObserver observer){(void)observer;}
#include "../src/lagoon_widescreen.c"
static unsigned packets,destinations,refusals;
uint64_t LagoonState_LoadGeneration(void){return 0;}
static void file(const char*root,unsigned room,const char*suffix,void*out,size_t size){char path[4096];snprintf(path,sizeof(path),"%s/baseline-%02x/dumps/sample-11.%s",root,room,suffix);FILE*f=fopen(path,"rb");assert(f&&fread(out,1,size,f)==size);fclose(f);}
static void load(const char*root,unsigned room){
 memset(&p,0,sizeof(p));ppu_reset(&p);p.inidisp=15;p.bgmode=9;p.obsel=2;p.bgXsc[0]=0x71;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
 file(root,room,"wram.bin",g_ram,sizeof(g_ram));file(root,room,"vram.bin",p.vram,sizeof(p.vram));file(root,room,"cgram.bin",p.cgram,sizeof(p.cgram));
 p.bgXsc[1]=g_ram[0x50]?0x7a:0x79;p.hScroll[0]=word(0x480)&1023;p.vScroll[0]=word(0x482)&1023;p.hScroll[1]=word(0x484)&1023;p.vScroll[1]=word(0x486)&1023;
 memset(&s_retained,0,sizeof(s_retained));memset(&s_parallax_queue,0,sizeof(s_parallax_queue));s_using_retained=0;s_world_chars_ready=1;
 memcpy(s_world_chars,p.vram,sizeof(s_world_chars));memset(s_world_char_used,1,sizeof(s_world_char_used));
 PpuSetExtraObjects(&p,NULL,0);LagoonWideEntities_Reset();assert(native_world_map());retain_completed_scene();assert(s_retained.valid&&retained_scene_matches());++packets;
}
int main(int argc,char**argv){assert(argc==3);FILE*f=fopen(argv[1],"rb");assert(f&&fread(own_rom,1,sizeof(own_rom),f)==sizeof(own_rom));fclose(f);g_config.widescreen=1;
 const unsigned rooms[]={1,2,9,10,11,12,13,14,15,16,17,18,19,20,21,22,38,39,40,41,43,56,57,58,59,60,61,62,64,65,66,67,68,69,70,71,72,73,74};
 for(unsigned n=0;n<sizeof(rooms)/sizeof(rooms[0]);++n){unsigned room=rooms[n];load(argv[2],room);
  uint8_t saved[0x40];memcpy(saved,g_ram+0x50,sizeof(saved));unsigned savedroom=word(0x48e);
  /* Any exact incoming descriptor may coexist with still-current outgoing
   * raster; changing a map/source byte revokes before incoming DMA completes. */
  for(unsigned dst=0;dst<75;++dst){memcpy(g_ram+0x50,own_rom+0x2b2df+dst*56,56);g_ram[0x48e]=dst;g_ram[0x48f]=0;g_ram[0x100e]=1;
   for(unsigned brightness=1;brightness<=15;brightness+=7){p.inidisp=brightness;assert(retained_scene_matches());++destinations;}
  }
  memcpy(g_ram+0x50,saved,sizeof(saved));g_ram[0x48e]=savedroom;g_ram[0x100e]=0;p.inidisp=15;
  for(unsigned b=0;b<sizeof(s_retained.descriptor);++b){unsigned before=s_retained.descriptor[b];s_retained.descriptor[b]^=1;
   /* Only fields consumed by exact scene descriptor grant authority. Unused
    * authored sound/script metadata does not alter rendered outgoing maps. */
   unsigned at=0x50+b;if(at==0x50||(at>=0x53&&at<=0x59)||(at>=0x5d&&at<=0x5f)||(at>=0x69&&at<=0x6b)||(at>=0x75&&at<=0x77)||(at>=0x88&&at<=0x8d)||(g_ram[0x50]&&at>=0x7e&&at<=0x83)){assert(!retained_scene_matches());++refusals;}s_retained.descriptor[b]=before;
  }
  ++generation;assert(!retained_scene_matches());--generation;++refusals;
  for(unsigned i=0;i<0x1000;++i){p.vram[0x7000+i]^=1;assert(!retained_scene_matches());p.vram[0x7000+i]^=1;++refusals;}
  for(unsigned i=0;i<0x4000;i+=16){p.vram[i]^=1;assert(!retained_scene_matches());p.vram[i]^=1;++refusals;}
  g_ram[0x5d]^=1;assert(!retained_scene_matches());g_ram[0x5d]^=1;++refusals;
  g_snesrecomp_last_hdmaen=1;assert(!retained_scene_matches());g_snesrecomp_last_hdmaen=0;++refusals;
  p.bgmode=0;assert(!retained_scene_matches());p.bgmode=9;++refusals;
  assert(retained_scene_matches());LagoonWide_OnVramWrite(0xe000);assert(!s_retained.valid);++refusals;
 }
 printf("remaining transition protocol: %u actual-loader outgoing packets, %u all75 exact incoming/fade descriptor checks, %u used-character/full-tilemap/source/generation/layout/DMA-order refusals PASS\n",packets,destinations,refusals);return 0;}
