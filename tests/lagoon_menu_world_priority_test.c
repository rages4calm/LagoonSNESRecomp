/* Protocol test: synthetic menu callback, real DrawFrame/transition cache.
 * Actual menu identity/resources have their separate captured-panel test. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "lagoon_widescreen.h"
#include "lagoon_wide_entities.h"
#include "lagoon_wide_spell3.h"
#include "lagoon_menu_presentation.h"
#include "lagoon_scene_renderer.h"
#include "lagoon_transition_present.h"
#include "common_rtl.h"
#include "config.h"
#include "widescreen.h"
#include "snes/ppu.h"
uint8 g_ram[0x20000]; const uint8 *g_rom; Ppu *g_ppu; Config g_config;
static int s_present_fallback,s_was_active,s_outdoor_present,s_present_incoming_mosaic;
static int s_row_world_x[224],s_row_world_y[224];
static unsigned s_extent_width,s_extent_height,s_completed_frame;
static unsigned word(unsigned a){return g_ram[a]|g_ram[a+1]<<8;}
static unsigned menu,scene_calls,blank_calls,mask_calls;
static uint32_t field[800*224],dst[800*224];static Ppu ppu;
uint64_t RtlStateGeneration(void){return 7;}
void RtlWidescreenPresent(uint8_t*d,size_t p,const uint8_t*s,int w,int h){for(int y=0;y<h;y++)memcpy(d+y*p,s+y*w*4,w*4);}
int LagoonWideSpell3_DrawMargins(uint8_t*d,size_t p,unsigned w,unsigned h,const Ppu*p2,const uint8_t*r,size_t z,const uint8_t*ram,uint64_t g){(void)d;(void)p;(void)w;(void)h;(void)p2;(void)r;(void)z;(void)ram;(void)g;return 0;}
int LagoonMenuPresentation_Draw(uint8_t*d,size_t p,const uint8_t*s,size_t sp,unsigned w,unsigned h,const Ppu*ppu2,const uint8_t*r,int wide){(void)s;(void)sp;(void)ppu2;(void)r;if(!menu||!wide)return 0;for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++)((uint32_t*)(d+y*p))[x]=0xff102030;return 1;}
void LagoonWideEntities_BlankField(uint8_t*d,size_t p,unsigned w,unsigned h){blank_calls++;unsigned left=(w-256)/2;for(unsigned y=0;y<h;y++){memset(d+y*p,0,left*4);memset(d+y*p+(left+256)*4,0,(w-left-256)*4);}}
void LagoonWideEntities_MaskRoomRow(uint8_t*d,unsigned w,int x,int y,unsigned rw,unsigned rh){(void)d;(void)w;(void)x;(void)y;(void)rw;(void)rh;mask_calls++;}
int LagoonSceneRenderer_TryPresent(uint8_t*d,size_t p,const uint8_t*r,size_t rp,unsigned w,unsigned h,uint64_t g){(void)d;(void)p;(void)r;(void)rp;(void)w;(void)h;(void)g;scene_calls++;return 0;}
int LagoonSceneRenderer_ExactRoom(const uint8_t*r){(void)r;return 1;}
#include "lagoon_draw_frame_under_test.inc"
int main(void){
 g_ppu=&ppu;g_ppu->inidisp=15;unsigned checks=0;
 for(unsigned w=256;w<=800;w+=34)for(unsigned vector=0;vector<4;vector++){
  unsigned fallback=!(vector&1),loading=!(vector&2);
  size_t pitch=w*4;g_config.widescreen=1;s_present_fallback=fallback;s_was_active=1;s_outdoor_present=1;s_present_incoming_mosaic=0;s_completed_frame=2;g_ram[0x100e]=loading?255:0;g_ram[0x48e]=1;
  for(unsigned i=0;i<w*224;i++)field[i]=0xff345678;
  LagoonTransition_Reset();assert(!LagoonTransition_Apply((uint8_t*)field,pitch,w,224,7,1,0,15,1,1));
  menu=1;scene_calls=blank_calls=mask_calls=0;
  assert(LagoonWide_DrawFrame((uint8_t*)dst,pitch,(uint8_t*)field,w,224,1));
  for(unsigned i=0;i<w*224;i++)assert(dst[i]==0xff102030);
  assert(!scene_calls&&!blank_calls&&!mask_calls);
  /* Unknown/mutated panels retain normal world/fallback/loader policy. */
  menu=0;s_present_fallback=1;s_outdoor_present=0;
  assert(LagoonWide_DrawFrame((uint8_t*)dst,pitch,(uint8_t*)field,w,224,1));assert(scene_calls==1&&blank_calls==1);
  for(unsigned i=0;i<w*224;i++)assert(dst[i]==0xff345678);
  /* Proven incoming source with a refused independent mosaic packet still
   * preserves the current native center; no old-room canvas may replace it. */
  s_present_incoming_mosaic=1;for(unsigned i=0;i<w*224;i++)field[i]=0xffabcdef;
  assert(LagoonWide_DrawFrame((uint8_t*)dst,pitch,(uint8_t*)field,w,224,1));
  unsigned left=(w-256)/2;for(unsigned y=0;y<224;y++)for(unsigned x=0;x<w;x++)
    assert(dst[y*w+x]==(x>=left&&x<left+256?0xffabcdef:0));
  g_config.widescreen=0;menu=1;scene_calls=0;assert(!LagoonWide_DrawFrame((uint8_t*)dst,pitch,(uint8_t*)field,w,224,1));assert(!scene_calls);checks++;
 }
 printf("PASS %u protocols: verified menu and proven incoming native center own output across loading/idle, active/fallback,256..800; refused mosaic keeps native center/blank margins; unknown/disabled fallback preserved\n",checks);return 0;
}
