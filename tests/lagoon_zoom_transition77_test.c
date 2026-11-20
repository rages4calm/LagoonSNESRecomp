#include "../src/lagoon_zoom_transition77.h"
#include <assert.h>
#include <stdio.h>
static LagoonZoomTransition77 hold;
static uint8_t source[(1280*4+16)*336],dest[(1280*4+16)*336+16];
static uint32_t pixel(unsigned x,unsigned y,unsigned pitch){uint32_t c;memcpy(&c,dest+y*pitch+x*4,4);return c;}
static void capture(unsigned w,unsigned h,unsigned pitch,unsigned room,unsigned frame){
  for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
    uint32_t c=0xffe1964b;memcpy(source+y*pitch+x*4,&c,4);
  }
  assert(LagoonZoomTransition77_Capture(&hold,source,pitch,w,h,9,room,frame));
}
int main(void){
  const unsigned widths[]={500,596,800,1280},heights[]={280,336};unsigned cases=0;
  for(unsigned wi=0;wi<4;wi++)for(unsigned hi=0;hi<2;hi++){
    unsigned w=widths[wi],h=heights[hi],pitch=w*4+16;
    capture(w,h,pitch,8,100);memset(dest,0xa5,sizeof dest);
    assert(LagoonZoomTransition77_Apply(&hold,dest,pitch,w,h,9,1,10,8,101));
    for(unsigned y=0;y<h;y++){
      for(unsigned x=0;x<w;x++)assert(pixel(x,y,pitch)==0xff966432);
      for(unsigned x=w*4;x<pitch;x++)assert(dest[y*pitch+x]==0xa5);
    }
    assert(dest[h*pitch]==0xa5); /* expanded margins and HUD retain footprint */
    assert(LagoonZoomTransition77_Apply(&hold,dest,pitch,w,h,9,1,15,0,102));
    assert(pixel(0,0,pitch)==0xff966432); /* incomplete incoming register bounce */
    assert(LagoonZoomTransition77_Apply(&hold,dest,pitch,w,h,9,1,128,0,103));
    assert(pixel(w-1,h-1,pitch)==0xff000000);
    assert(LagoonZoomTransition77_Apply(&hold,dest,pitch,w,h,9,1,15,9,104));
    assert(pixel(0,0,pitch)==0xff000000); /* forced blank cannot brighten old room */
    LagoonZoomTransition77_Reset(&hold); /* positive incoming source / authored UI */
    assert(!LagoonZoomTransition77_Apply(&hold,dest,pitch,w,h,9,1,15,9,105));
    capture(w,h,pitch,8,100);
    assert(LagoonZoomTransition77_Apply(&hold,dest,pitch,w,h,9,0,15,9,101));
    memset(dest,0xa5,sizeof dest);
    assert(!LagoonZoomTransition77_Apply(&hold,dest,pitch,w,h,9,0,15,9,102));
    assert(!hold.valid&&pixel(0,0,pitch)==0xa5a5a5a5); /* one field only */
    assert(!LagoonZoomTransition77_Apply(&hold,dest,pitch,w,h,9,1,15,9,103));
    capture(w,h,pitch,8,100);
    assert(!LagoonZoomTransition77_Apply(&hold,dest,pitch,w,h,9,0,15,8,101));
    assert(!hold.valid); /* same-room failed source cannot leave a latent cache */
    capture(w,h,pitch,8,100);
    assert(!LagoonZoomTransition77_Apply(&hold,dest,pitch,w,h,10,1,15,9,101));
    assert(!hold.valid); /* state load/reset */
    capture(w,h,pitch,8,100);
    assert(!LagoonZoomTransition77_Apply(&hold,dest,pitch,w+2,h,9,1,15,9,101));
    assert(!hold.valid); /* changing aspect/zoom */
    capture(w,h,pitch,8,100);
    assert(!LagoonZoomTransition77_Apply(&hold,dest,16,w,h,9,1,15,9,101));
    assert(!hold.valid);
    ++cases;
  }
  assert(!LagoonZoomTransition77_Capture(&hold,source,5120,1281,336,9,8,100));
  assert(!LagoonZoomTransition77_Capture(&hold,source,5120,1280,337,9,8,100));
  assert(!LagoonZoomTransition77_Capture(&hold,source,5120,1280,336,9,0,100));
  printf("PASS zoom outgoing canvas: %u dimension combinations; complete footprint, monotonic fade, incoming/UI/reset/aspect retirement, one-field preloader and pitch canaries\n",cases);
  return 0;
}
