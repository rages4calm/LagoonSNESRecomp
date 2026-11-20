#ifndef LAGOON_ZOOM_TRANSITION77_H
#define LAGOON_ZOOM_TRANSITION77_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
/* One completed, source-qualified outgoing zoom canvas. This is presentation
 * only: it never lends map/actor/source authority to an incoming field. The
 * caller retires it at positive incoming ownership and before authored UI. */
typedef struct LagoonZoomTransition77 {
  uint32_t pixels[1280*336];
  uint64_t generation;
  unsigned width,height,room,frame,valid,floor;
} LagoonZoomTransition77;
static inline void LagoonZoomTransition77_Reset(LagoonZoomTransition77 *s){s->valid=0;s->floor=15;}
static inline int LagoonZoomTransition77_Capture(LagoonZoomTransition77 *s,
    const uint8_t *src,size_t pitch,unsigned w,unsigned h,uint64_t generation,
    unsigned room,unsigned frame){
  if(!src||!room||w<256||w>1280||h<224||h>336||pitch<(size_t)w*4)return 0;
  for(unsigned y=0;y<h;y++)memcpy(s->pixels+y*w,src+y*pitch,w*4);
  s->generation=generation;s->width=w;s->height=h;s->room=room;s->frame=frame;
  s->valid=1;s->floor=15;return 1;
}
static inline int LagoonZoomTransition77_Apply(LagoonZoomTransition77 *s,
    uint8_t *dst,size_t pitch,unsigned w,unsigned h,uint64_t generation,
    int loading,unsigned brightness,unsigned room,unsigned frame){
  if(!s->valid)return 0;
  if(!dst||s->generation!=generation||s->width!=w||s->height!=h||pitch<(size_t)w*4){
    LagoonZoomTransition77_Reset(s);return 0;
  }
  int preloader=!loading&&room&&room!=s->room&&(unsigned)(frame-s->frame)==1;
  if(!loading&&!preloader){LagoonZoomTransition77_Reset(s);return 0;}
  unsigned level=(brightness&128)?0:brightness&15;
  if(level<s->floor)s->floor=level;
  if(level>s->floor)level=s->floor;
  for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
    uint32_t c=s->pixels[y*w+x];
    uint32_t out=(c&0xff000000u)|((((c>>16)&255)*level/15)<<16)|
        ((((c>>8)&255)*level/15)<<8)|((c&255)*level/15);
    memcpy(dst+y*pitch+x*4,&out,4);
  }
  return 1;
}
#endif
