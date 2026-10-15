/* Actual production retained-object packet with real PPU scanout. */
#define main ppu_extra_object_fixture_main
#include "../snesrecomp/tests/ppu/ppu_extra_object_test.c"
#undef main
#include "lagoon_wide_entities.h"
#include "config.h"
#include "snes/dma.h"
Config g_config;
const uint8_t *g_rom;
void dma_set_start_observer(DmaStartObserver observer){(void)observer;}
void LagoonWide_OnBackgroundDma(unsigned bank,unsigned address,unsigned destination,unsigned length){(void)bank;(void)address;(void)destination;(void)length;}
void LagoonWide_OnVramWrite(unsigned address){(void)address;}
uint8_t g_ram[0x20000];
Ppu *g_ppu=&p;
static uint64_t generation=7;
uint64_t RtlStateGeneration(void){return generation;}
static uint32_t wide[448];
int main(void) {
  setup(1,kPpuRenderFlags_NewRenderer);
  uint16_t colors[]={0x83E0,0x801F};
  PpuExtraObject source={268,0,2,1,2,3,0,64,1,colors};
  PpuSetExtraObjects(&p,&source,1);
  uint8_t before[PPU_SAVESTATE_MEM_SIZE],ram_before[sizeof(g_ram)];
  memcpy(before,p.cgram,sizeof(before));memcpy(ram_before,g_ram,sizeof(g_ram));
  LagoonWideEntities_RetainScene();assert(LagoonWideEntities_RetainedSceneValid());
  colors[0]=0;source.x=20;
  LagoonWideEntities_BeginRetained(1);
  assert(p.extraObjectCount==1&&p.extraObjects[0].x==268&&p.extraObjects[0].pixels[0]==0x83E0);
  PpuBeginDrawing(&p,(uint8_t*)wide,448*4,kPpuRenderFlags_NewRenderer);
  PpuSetExtraSpace(&p,96);ppu_runLine(&p,0);ppu_runLine(&p,1);
  assert(wide[96+268]==0x00FF00&&wide[96+269]==0xFF0000);
  for(unsigned x=96;x<96+256;++x)assert(wide[x]==0xFF0000);
  assert(!memcmp(before,p.cgram,sizeof(before))&&!memcmp(ram_before,g_ram,sizeof(g_ram)));
  p.vram[0x4400]^=1;assert(!LagoonWideEntities_RetainedSceneValid());p.vram[0x4400]^=1;
  p.cgram[3]^=1;assert(!LagoonWideEntities_RetainedSceneValid());p.cgram[3]^=1;
  g_ram[0x540]=1;assert(!LagoonWideEntities_RetainedSceneValid());g_ram[0x540]=0;
  g_ram[0x500]=1;assert(LagoonWideEntities_RetainedSceneValid());g_ram[0x500]=0;
  p.oam[64*2]^=1;assert(!LagoonWideEntities_RetainedSceneValid());p.oam[64*2]^=1;
  p.highOam[64/4]^=1;assert(!LagoonWideEntities_RetainedSceneValid());p.highOam[64/4]^=1;
  assert(LagoonWideEntities_RetainedSceneValid());generation++;assert(!LagoonWideEntities_RetainedSceneValid());
  LagoonWideEntities_BeginRetained(1);assert(!p.extraObjectCount);
  LagoonWideEntities_RetainScene();assert(LagoonWideEntities_RetainedSceneValid());
  LagoonWideEntities_Reset();assert(!LagoonWideEntities_RetainedSceneValid());
  puts("retained objects: production deep copy, real PPU colors/native preservation, atlas/palette/record/OAM/generation refusal PASS");
  return 0;
}
