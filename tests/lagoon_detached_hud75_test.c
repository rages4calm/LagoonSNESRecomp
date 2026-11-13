/* ROM-free regression for relocated corrected health bars: original-world
 * underlay must never leak into the detached HUD. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../src/lagoon_boss_hud.c"
#include <assert.h>
#include <stdio.h>
static uint32_t field_pixels[400*224],target[(600+4)*336];
int main(void){
  memset(field_pixels,0,sizeof field_pixels);for(unsigned i=0;i<(600+4)*336;i++)target[i]=0x12345678;
  packet.valid=1;packet.generation=75;
  for(unsigned i=0;i<1024;i++){
    packet.new_pixels[i]=0x00ff00; /* field_pixels has world under a transparent bar */
    packet.hud_pixels[i]=(i%8)?0:0x0000ff;
    field_pixels[(207+i/128)*400+72+64+i%128]=packet.new_pixels[i];
  }
  assert(!LagoonBossHud_DrawDetached((uint8_t*)target,604*4,600,336,(uint8_t*)field_pixels,400*4,400,76));
  assert(target[0]==0x12345678);
  assert(LagoonBossHud_DrawDetached((uint8_t*)target,604*4,600,336,(uint8_t*)field_pixels,400*4,400,75));
  unsigned changed=0;
  for(unsigned y=0;y<336;y++)for(unsigned x=0;x<604;x++){
    unsigned hud=y>=319&&y<327&&x>=236&&x<364;
    uint32_t want=hud?(((y-319)*128+x-236)%8?0:0x0000ff):0x12345678;
    assert(target[y*604+x]==want);changed+=hud;
  }
  field_pixels[207*400+136]^=1;assert(!LagoonBossHud_DrawDetached((uint8_t*)target,604*4,600,336,(uint8_t*)field_pixels,400*4,400,75));
  printf("PASS %u detached corrected bar pixels, no old world underlay, field_pixels/generation gates and pitch canaries\n",changed);
}
