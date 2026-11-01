#ifndef LAGOON_WIDE_RASTER_H
#define LAGOON_WIDE_RASTER_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct LagoonWideRasterRow {
  int x[2],y[2];
} LagoonWideRasterRow;

/* RAM supplies only each layer's lost world-coordinate high bits. Actual
 * raster scroll supplies its phase, independently: ordinary NMI updates may
 * reach BG1 and BG2 in different fields. Signed wrap preserves an upward step.
 * Source/layout/generation and the special parallax contract remain the
 * caller's responsibility; this helper grants no scene authority. */
static inline int LagoonWideRaster_SampleRow(const unsigned camera[4],
    const unsigned scroll[4],unsigned line,LagoonWideRasterRow *row) {
  if(!camera||!scroll||!row||line<1||line>224)return 0;
  LagoonWideRasterRow result;
  for(unsigned layer=0;layer<2;++layer) {
    int cx=(int)camera[layer*2],cy=(int)camera[layer*2+1];
    int sx=(int)(scroll[layer*2]&1023),sy=(int)(scroll[layer*2+1]&1023);
    result.x[layer]=cx+((sx-cx+512)&1023)-512;
    result.y[layer]=cy+((sy-cy+512)&1023)-512+(int)line;
  }
  *row=result;return 1;
}

/* Each ordinary world layer owns a finite map extent. A phase-lag field may
 * show two distinct world coordinates; clip the margins to both source
 * extents instead of discarding the entire field. Static parallax callers
 * keep their original BG1-only extent policy by supplying layers=1.
 * The native256-pixel center is never changed. */
static inline void LagoonWideRaster_MaskRoomRow(uint8_t *pixels,unsigned width,
    const LagoonWideRasterRow *row,unsigned room_width,unsigned room_height,
    unsigned layers) {
  if(!pixels||!row||width<256||layers<1||layers>2)return;
  int left=(int)(width-256)/2;
  for(unsigned x=0;x<width;++x) {
    int screen_x=(int)x-left;
    if(screen_x>=0&&screen_x<256)continue;
    for(unsigned layer=0;layer<layers;++layer) {
      int64_t map_x=(int64_t)row->x[layer]+screen_x;
      if(map_x<0||(uint64_t)map_x>=room_width||row->y[layer]<0||
          (unsigned)row->y[layer]>=room_height) {
        memset(pixels+x*4,0,4);break;
      }
    }
  }
}
#endif
