#include "../src/lagoon_cloud_motion76.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
static LagoonCloudView75 seed(int x,int y,unsigned style){
  LagoonCloudView75 c={0};c.valid=1;c.count=1;c.anchor_x=x;c.anchor_y=y;
  c.cells[0]=(LagoonWideCell){x,y,(uint16_t)(style?0x38a0:0x38a4),16};return c;
}
int main(void){
  unsigned checked=0;
  for(unsigned w=256;w<=1280;w+=2){
    int left=-((int)w-256)/2;LagoonCloudMotion76 state={0};LagoonCloudMotion76Sample a,b;
    LagoonCloudView75 c=seed(-176,20,0);
    assert(LagoonCloudMotion76_Step(&state,3,0x28,253,700,100,left,-56,w,336,&c,1,&a));
    int lastx=a.x,lasty=a.y;assert(lastx==left-176);
    for(unsigned n=1;n<700;n++){
      /* Original narrow cloud can retire/respawn/stylechange at any time.
       * Half-rate main tick reproduces the measured one tick/two fields. */
      uint8_t tick=(uint8_t)(253+n/2);
      c=seed((int)((n*6)%432)-176,(int)(n%256)-32,n&1);
      const LagoonCloudView75 *source=(n%73)<60?&c:NULL;
      assert(LagoonCloudMotion76_Step(&state,3,0x28,tick,700,100,left,-56,w,336,source,1,&b));
      if(!b.wraps){assert(b.x-lastx==((n&1)?0:6));assert(b.y==lasty);}
      else assert(b.x<left&&b.x+176<=left+6);
      assert(LagoonCloudMotion76_Step(&state,3,0x28,tick,700,100,left,-56,w,336,source,1,&a));
      assert(a.x==b.x&&a.y==b.y);lastx=b.x;lasty=b.y;checked++;
    }
    /* Camera movement is subtracted once, never multiplied by zoom. */
    int oldx=lastx,oldy=lasty;
    assert(LagoonCloudMotion76_Step(&state,3,0x28,state.tick,710,110,left,-56,w,336,NULL,1,&b));
    assert(b.x==oldx-10&&b.y==oldy-10);
    assert(!LagoonCloudMotion76_Step(&state,4,0x28,state.tick,710,110,left,-56,w,336,NULL,1,&b));
    assert(!state.ready);
    assert(!LagoonCloudMotion76_Step(&state,4,0x29,1,710,110,left,-56,w,336,&c,1,&b));
    assert(!LagoonCloudMotion76_Step(&state,4,0x28,1,710,110,left,-56,w,336,&c,0,&b));
  }
  printf("PASS cloud76 %u temporal velocity/lifespan/wrap/repeated-field vectors, camera and generation/resource refusals\n",checked);
}
