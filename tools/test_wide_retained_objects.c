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
static void diagnostic(unsigned expected,unsigned expected_address) {
  unsigned kind,address,captured,current;
  uint8_t ram_before[sizeof(g_ram)],ppu_before[PPU_SAVESTATE_MEM_SIZE];
  memcpy(ram_before,g_ram,sizeof(g_ram));memcpy(ppu_before,p.cgram,sizeof(ppu_before));
  LagoonWideEntities_RetainedDiagnostic(&kind,&address,&captured,&current);
  assert(kind==expected && address==expected_address);
  if(expected>1 && expected<6)assert(captured!=current);
  assert(!memcmp(ram_before,g_ram,sizeof(g_ram))&&!memcmp(ppu_before,p.cgram,sizeof(ppu_before)));
}
static void native_anchor_test(void) {
  setup(1,kPpuRenderFlags_NewRenderer);p.obsel=2;memset(g_ram,0,sizeof(g_ram));
  p.cgram[129]=0x3e0;
  for(unsigned y=0;y<8;++y)p.vram[0x4400+y]=0xff;
  uint8_t *oam=(uint8_t*)p.oam;unsigned slot=64;
  oam[slot*4]=12;oam[slot*4+1]=0;oam[slot*4+2]=0x40;oam[slot*4+3]=0;
  p.highOam[slot/4]=1;
  LagoonWideCell cell={268,0,0x40,8};uint16_t pixels[256];
  assert(LagoonWideEntities_Pixels(p.vram,p.cgram,0x4000,0x5000,&cell,pixels,1));
  PpuExtraObject objects[2]={{268,0,8,8,8,0,0,64,1,pixels},{268,0,8,8,8,0,0,64,2,pixels}};
  PpuSetExtraObjects(&p,objects,1);LagoonWideEntities_RetainScene();
  assert(LagoonWideEntities_RetainedNativeAnchors() && LagoonWideEntities_RetainedSceneValid());
  /* Original room loading may clear/reinitialize actor RAM while old native
   * OAM remains displayed. Only source-pinned native cells survive this. */
  g_ram[0x540]=0xc1;g_ram[0x542]=96;
  assert(LagoonWideEntities_RetainedSceneValid());diagnostic(0,0);
  oam[slot*4]++;oam[slot*4+1]+=2;
  assert(LagoonWideEntities_RetainedSceneValid());LagoonWideEntities_BeginRetained(1);
  assert(p.extraObjectCount==1 && p.extraObjects[0].x==269 && p.extraObjects[0].y==2 && p.extraObjects[0].pixels[0]==pixels[0]);
  oam[slot*4]--;oam[slot*4+1]-=2;
  oam[slot*4]+=9;assert(!LagoonWideEntities_RetainedSceneValid());oam[slot*4]-=9;
  oam[slot*4+1]+=9;assert(!LagoonWideEntities_RetainedSceneValid());oam[slot*4+1]-=9;
  for(unsigned i=slot*4+2;i<slot*4+4;++i) {
    oam[i]^=1;assert(!LagoonWideEntities_RetainedSceneValid());oam[i]^=1;
  }
  for(unsigned bit=1;bit<=2;bit<<=1) {
    p.highOam[slot/4]^=bit;assert(!LagoonWideEntities_RetainedSceneValid());p.highOam[slot/4]^=bit;
  }
  p.vram[0x4400]^=1;assert(!LagoonWideEntities_RetainedSceneValid());p.vram[0x4400]^=1;
  p.cgram[129]^=1;assert(!LagoonWideEntities_RetainedSceneValid());p.cgram[129]^=1;
  p.obsel^=1;assert(!LagoonWideEntities_RetainedSceneValid());diagnostic(6,0);p.obsel^=1;
  p.oamaddh^=0x80;assert(!LagoonWideEntities_RetainedSceneValid());p.oamaddh^=0x80;
  generation++;assert(!LagoonWideEntities_RetainedSceneValid());generation--;
  /* An unowned native slot newly crossing the viewport cannot be omitted. */
  uint8_t saved[4];memcpy(saved,oam+slot*4-4,4);
  oam[slot*4-4]=255;oam[slot*4-3]=0;p.highOam[(slot-1)/4]|=1u<<(((slot-1)%4)*2);
  assert(!LagoonWideEntities_RetainedSceneValid());memcpy(oam+slot*4-4,saved,4);p.highOam[(slot-1)/4]=0;
  /* A changed captured pixel/attribute or a virtual cell cannot obtain the
   * independent native-source exemption. Existing full RAM witness remains. */
  PpuSetExtraObjects(&p,objects,1);
  pixels[0]^=1;LagoonWideEntities_RetainScene();assert(!LagoonWideEntities_RetainedNativeAnchors());pixels[0]^=1;
  g_ram[0x540]^=1;assert(!LagoonWideEntities_RetainedSceneValid());g_ram[0x540]^=1;
  objects[0].x++;LagoonWideEntities_RetainScene();assert(!LagoonWideEntities_RetainedNativeAnchors());objects[0].x--;
  objects[0].priority++;LagoonWideEntities_RetainScene();assert(!LagoonWideEntities_RetainedNativeAnchors());objects[0].priority--;
  objects[0].order=2;LagoonWideEntities_RetainScene();assert(!LagoonWideEntities_RetainedNativeAnchors());
  g_ram[0x540]^=1;assert(!LagoonWideEntities_RetainedSceneValid());g_ram[0x540]^=1;
  objects[0].order=1;PpuSetExtraObjects(&p,objects,2);LagoonWideEntities_RetainScene();
  assert(!LagoonWideEntities_RetainedNativeAnchors());
  g_ram[0x540]^=1;assert(!LagoonWideEntities_RetainedSceneValid());g_ram[0x540]^=1;
  PpuSetExtraObjects(&p,NULL,0);LagoonWideEntities_RetainScene();assert(!LagoonWideEntities_RetainedNativeAnchors());
  puts("retained native anchors: exact original OAM/pixels/layout, RAM clearing, mixed/virtual refusal, source/OAM/generation mutations PASS");
}
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
  diagnostic(0,0);
  p.vram[0x4400]^=1;assert(!LagoonWideEntities_RetainedSceneValid());diagnostic(3,0x4400);p.vram[0x4400]^=1;
  p.cgram[3]^=1;assert(!LagoonWideEntities_RetainedSceneValid());diagnostic(4,3);p.cgram[3]^=1;
  g_ram[0x540]=1;assert(!LagoonWideEntities_RetainedSceneValid());diagnostic(2,0x540);g_ram[0x540]=0;
  g_ram[0x500]=1;assert(LagoonWideEntities_RetainedSceneValid());g_ram[0x500]=0;
  p.oam[64*2]^=1;assert(!LagoonWideEntities_RetainedSceneValid());diagnostic(5,64*4);p.oam[64*2]^=1;
  p.highOam[64/4]^=1;assert(!LagoonWideEntities_RetainedSceneValid());diagnostic(5,512+64/4);p.highOam[64/4]^=1;
  assert(LagoonWideEntities_RetainedSceneValid());generation++;assert(!LagoonWideEntities_RetainedSceneValid());
  diagnostic(1,0);
  LagoonWideEntities_BeginRetained(1);assert(!p.extraObjectCount);
  LagoonWideEntities_RetainScene();assert(LagoonWideEntities_RetainedSceneValid());
  LagoonWideEntities_Reset();assert(!LagoonWideEntities_RetainedSceneValid());
  native_anchor_test();
  queue_observer_test();
  puts("retained objects: production deep copy, real PPU colors/native preservation, atlas/palette/record/OAM/generation refusal PASS");
  return 0;
}
