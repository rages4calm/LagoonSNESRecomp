#ifndef LAGOON_ZOOM_PLACEMENT_H
#define LAGOON_ZOOM_PLACEMENT_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* A refused loader field is still the current source, never a retained room.
 * Keep its source-pixel footprint on the detached canvas. The caller provides
 * the same native origin as the qualified world renderer. Unproven margins
 * remain blank; no source pixel is enlarged or sampled from a previous field.
 * rows excludes HUD when the current upload does not qualify its ownership. */
static int LagoonZoomPlacement_CopyWorld(uint8_t *dst,size_t pitch,unsigned width,unsigned height,
    const uint8_t *source,size_t sp,unsigned sw,unsigned sh,int native_left,int native_top,unsigned rows){
  if(!dst||!source||sw<256||width<sw||height<sh||pitch<width*4||sp<sw*4||rows>sh)return 0;
  int x0=native_left-(int)(sw-256)/2;
  for(unsigned y=0;y<height;y++)memset(dst+y*pitch,0,width*4);
  int begin=x0<0?-x0:0,end=(int)sw;
  if(x0+end>(int)width)end=(int)width-x0;
  if(end<=begin)return 1;
  for(unsigned y=0;y<rows;y++){
    int dy=native_top+(int)y;
    if(dy>=0&&dy<(int)height)memcpy(dst+dy*pitch+(x0+begin)*4,source+y*sp+begin*4,(size_t)(end-begin)*4);
  }
  return 1;
}
#endif
