/* Independent EPX/Scale2x implementation from the published decision rules:
 * https://www.scale2x.it/algorithm . No imported implementation or game assets.
 * The optional GPU preset is the intended runtime implementation; this CPU
 * oracle exists for deterministic tests and offline comparisons only. */
#include "lagoon_epx_reference.h"
static uint32_t pixel(const uint32_t *source, size_t pitch,unsigned x,unsigned y) {
  return ((const uint32_t *)((const uint8_t *)source + y*pitch))[x];
}
int LagoonEpxReference2x(const uint32_t *source,size_t source_pitch,
    unsigned width,unsigned height,uint32_t *destination,size_t destination_pitch) {
  if(!source || !destination || !width || !height || width>4096 || height>4096 ||
      source_pitch%4 || destination_pitch%4 || source_pitch<width*4u ||
      destination_pitch<width*8u) return 0;
  unsigned hud = height>32 ? height-32 : 0;
  for(unsigned y=0;y<height;++y) {
    uint32_t *top=(uint32_t *)((uint8_t *)destination+y*2*destination_pitch);
    uint32_t *bottom=(uint32_t *)((uint8_t *)top+destination_pitch);
    for(unsigned x=0;x<width;++x) {
      uint32_t e=pixel(source,source_pitch,x,y),out[4]={e,e,e,e};
      if(y<hud) {
        uint32_t b=pixel(source,source_pitch,x,y ? y-1:y);
        uint32_t h=pixel(source,source_pitch,x,y+1<hud ? y+1:y);
        uint32_t d=pixel(source,source_pitch,x ? x-1:x,y);
        uint32_t f=pixel(source,source_pitch,x+1<width ? x+1:x,y);
        if(b!=h && d!=f) {
          if(d==b)out[0]=d;
          if(b==f)out[1]=f;
          if(d==h)out[2]=d;
          if(h==f)out[3]=f;
        }
      }
      top[x*2]=out[0];top[x*2+1]=out[1];
      bottom[x*2]=out[2];bottom[x*2+1]=out[3];
    }
  }
  return 1;
}
