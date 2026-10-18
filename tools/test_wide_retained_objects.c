/* Actual production retained-object packet with real PPU scanout. */
#define main ppu_extra_object_fixture_main
#include "../snesrecomp/tests/ppu/ppu_extra_object_test.c"
#undef main
#include "lagoon_wide_entities.h"
#include "config.h"
#include "snes/dma.h"
Config g_config;
const uint8_t *g_rom;
static DmaStartObserver dma_observer;
static unsigned queue_callbacks,queue_length;
void dma_set_start_observer(DmaStartObserver observer){dma_observer=observer;}
void LagoonWide_OnBackgroundDma(unsigned bank,unsigned address,unsigned destination,unsigned length){(void)bank;(void)address;(void)destination;(void)length;}
void LagoonWide_OnVramWrite(unsigned address){(void)address;}
void LagoonWide_OnParallaxQueueDma(unsigned length){++queue_callbacks;queue_length=length;}
uint8_t g_ram[0x20000];
Ppu *g_ppu=&p;
static uint64_t generation=7;
uint64_t RtlStateGeneration(void){return generation;}
static uint32_t wide[448];
static void queue_observer_test(void) {
  Dma dma={0};DmaChannel *t=&dma.channel[1];
  static uint8_t dummy_rom[1];g_rom=dummy_rom;g_config.widescreen=1;
  p.vramIncrement=1;p.vramIncrementOnHigh=1;p.vramRemapMode=0;
  t->mode=4;t->bAdr=0x16;t->aBank=0x7e;t->aAdr=0x2600;t->size=136;
  LagoonWideEntities_Init();assert(dma_observer);
  dma_observer(&dma,1,t);assert(queue_callbacks==1 && queue_length==136);
  dma_observer(&dma,2,t);assert(queue_callbacks==1);
  t->fromB=1;dma_observer(&dma,1,t);t->fromB=0;
  t->mode=3;dma_observer(&dma,1,t);t->mode=4;
  t->bAdr=0x15;dma_observer(&dma,1,t);t->bAdr=0x16;
  t->fixed=1;dma_observer(&dma,1,t);t->fixed=0;
  t->decrement=1;dma_observer(&dma,1,t);t->decrement=0;
  t->aBank=0x7f;dma_observer(&dma,1,t);t->aBank=0x7e;
  t->aAdr=0x2601;dma_observer(&dma,1,t);t->aAdr=0x2600;
  p.vramIncrement=32;dma_observer(&dma,1,t);p.vramIncrement=1;
  p.vramIncrementOnHigh=0;dma_observer(&dma,1,t);p.vramIncrementOnHigh=1;
  p.vramRemapMode=1;dma_observer(&dma,1,t);p.vramRemapMode=0;
  g_config.widescreen=0;dma_observer(&dma,1,t);g_config.widescreen=1;
  g_rom=NULL;dma_observer(&dma,1,t);g_rom=dummy_rom;
  g_ppu=NULL;dma_observer(&dma,1,t);g_ppu=&p;
  assert(queue_callbacks==1);g_rom=NULL;
  puts("room07 queue production observer: exact channel/mode/source/PPU context,14 negative contexts PASS");
}
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
  queue_observer_test();
  puts("retained objects: production deep copy, real PPU colors/native preservation, atlas/palette/record/OAM/generation refusal PASS");
  return 0;
}
