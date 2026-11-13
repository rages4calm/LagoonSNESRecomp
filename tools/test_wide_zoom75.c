/* Detached world pixels start below original row189. Exercise the real
 * production cell painter, not a duplicate bounds implementation. */
#define LAGOON_WIDE_ACTOR_POLICY_NO_MAIN
#include "test_wide_additional_producer.c"
static int extra_y=112;
int LagoonWide_ViewExtraY(void){return extra_y;}
int LagoonWide_ViewExtraX(int extra){(void)extra;return 256;}
static unsigned samples;
static void visible(int x,int y){s_object_count=0;LagoonWideCell c={x,y,0x2640,8};assert(draw_cell(&c,0,2,0));assert(s_object_count==1);const PpuExtraObject *o=&s_objects[0];assert(o->x==x&&o->y==y);for(unsigned i=0;i<64;++i){int sx=x+i%8,sy=y+i/8;unsigned expected=sx>=0&&sx<256&&sy>=0&&sy<(extra_y?190:224)?0:0x8015;assert(o->pixels[i]==expected);++samples;}}
int main(void){p.obsel=2;p.extraRightCur=43;for(unsigned y=0;y<8;++y)p.vram[0x4400+y]=255;for(unsigned i=0;i<256;++i)p.cgram[i]=0x15;
 visible(100,190);visible(100,191);visible(100,188);visible(100,224);visible(100,320);visible(-8,100);visible(256,100);visible(100,-8);
 /* Native pixels remain untouched, and the unchanged1x path still clips all224 rows. */
 s_object_count=0;LagoonWideCell core={100,180,0x2640,8};assert(!draw_cell(&core,0,2,0));extra_y=0;s_object_count=0;core.y=190;assert(!draw_cell(&core,0,2,0));
 printf("PASS %u expanded-view cell pixels, rows190/191, vertical/seam cells, native-center and1x refusal\n",samples);return 0;}
