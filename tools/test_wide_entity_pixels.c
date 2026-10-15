#define main ppu_extra_object_fixture_main
#include "../snesrecomp/tests/ppu/ppu_extra_object_test.c"
#undef main
#include "lagoon_wide_entities.h"

static uint32_t wide_frame[448 * 224];
int main(void) {
  assert(ppu_extra_object_fixture_main() == 0);
  uint16_t decoded[256];
  for (unsigned flags = 0; flags < 4; ++flags) {
    memset(&p,0,sizeof(p)); ppu_reset(&p);
    p.inidisp=15;p.bgmode=1;p.obsel=2;p.screenEnabled[0]=16;
    for(unsigned i=0;i<128;++i)p.oam[i*2]=0xf000;
    for(unsigned i=0;i<0x8000;++i)p.vram[i]=(uint16_t)(i*997u+(i>>3)*71u);
    for(unsigned i=0;i<256;++i)p.cgram[i]=(uint16_t)((i*173u)&0x7fff);
    LagoonWideCell cell={268,0,(uint16_t)(0x3668|(flags<<14)),16};
    assert(LagoonWideEntities_Pixels(p.vram,p.cgram,0x4000,0x5000,&cell,decoded,1));
    PpuExtraObject copy={268,0,16,16,16,3,0,1,1,decoded};
    PpuBeginDrawing(&p,(uint8_t*)wide_frame,448*4,kPpuRenderFlags_NewRenderer);
    PpuSetExtraSpace(&p,96);
    PpuSetExtraObjects(&p,&copy,1);
    p.oam[0]=12;p.oam[1]=cell.tile;p.highOam[0]=2; /* same real native 16px OBJ */
    ppu_runLine(&p,0);
    for(unsigned row=0;row<16;++row) {
      ppu_runLine(&p,row+1);
      for(unsigned x=0;x<16;++x)
        assert(wide_frame[row*448+96+12+x]==wide_frame[row*448+96+268+x]);
    }
    /* A crossing host object must have zero opacity throughout native x. */
    cell.x=250;
    assert(LagoonWideEntities_Pixels(p.vram,p.cgram,0x4000,0x5000,&cell,decoded,1));
    for(unsigned y=0;y<16;++y)for(unsigned x=0;x<6;++x)assert(decoded[y*16+x]==0);
    cell.tile=1; /* never sample the dynamically staged player graphics */
    assert(!LagoonWideEntities_Pixels(p.vram,p.cgram,0x4000,0x5000,&cell,decoded,1));
  }
  /* Apply the production extent mask to pixels from the real PPU above.
   * The same actual right-margin object is preserved when in map and hidden
   * when its world position exceeds512, while native pixels stay bit exact. */
  uint32_t before[448];unsigned opaque=0;
  for(unsigned y=0;y<16;++y) {
    uint32_t *row=wide_frame+y*448;memcpy(before,row,sizeof(before));
    for(unsigned x=352;x<448;++x)opaque+=row[x]!=0;
    LagoonWideEntities_MaskRoomRow((uint8_t*)row,448,0,(int)y,512,512);
    assert(memcmp(row+96,before+96,256*4)==0);
    assert(memcmp(row+352,before+352,96*4)==0);
    LagoonWideEntities_MaskRoomRow((uint8_t*)row,448,256,(int)y,512,512);
    assert(memcmp(row+96,before+96,256*4)==0);
    for(unsigned x=352;x<448;++x)assert(row[x]==0);
  }
  assert(opaque>0);
  puts("wide entity pixels: actual PPU equivalence for four flips, native-center clipping, dynamic-player refusal, actual PPU room-bound/native preservation PASS");
  return 0;
}
