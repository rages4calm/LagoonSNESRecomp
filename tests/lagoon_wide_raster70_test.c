/* Synthetic coordinate/extent vectors, not gameplay or room qualification. */
#include "../src/lagoon_wide_raster.h"
#include <assert.h>
#include <stdio.h>

static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while(0)
static void phases(void) {
  unsigned camera[4]={193,254,193,254},scroll[4]={193,249,193,254};
  LagoonWideRasterRow row;
  for(unsigned line=1;line<=224;++line) {
    CHECK(LagoonWideRaster_SampleRow(camera,scroll,line,&row));
    CHECK(row.x[0]==193&&row.x[1]==193);
    CHECK(row.y[0]==249+(int)line&&row.y[1]==254+(int)line);
    /* The opposite direction must preserve both actual layers as well. */
    scroll[1]=259;scroll[3]=254;
    CHECK(LagoonWideRaster_SampleRow(camera,scroll,line,&row));
    CHECK(row.y[0]==259+(int)line&&row.y[1]==254+(int)line);
    scroll[1]=249;
  }
  /* High world bits belong to each source layer, never to its neighbor. */
  unsigned separate[4]={1024,1024,0,0},zero[4]={0};
  CHECK(LagoonWideRaster_SampleRow(separate,zero,1,&row));
  CHECK(row.x[0]==1024&&row.y[0]==1025&&row.x[1]==0&&row.y[1]==1);
  for(unsigned bad=0;bad<4;++bad) {
    LagoonWideRasterRow sentinel={{71,72},{73,74}};row=sentinel;
    CHECK(!LagoonWideRaster_SampleRow(bad==0?NULL:camera,bad==1?NULL:scroll,
        bad==2?0:bad==3?225:1,&row));
    CHECK(!memcmp(&row,&sentinel,sizeof(row)));
  }
  CHECK(!LagoonWideRaster_SampleRow(camera,scroll,1,NULL));
}
static void wrapping(void) {
  static const struct {unsigned camera,scroll;int world;} cases[]={
    {1023,0,1024},{1024,1023,1023},{0,1023,-1},
    {0,1019,-5},{1024,5,1029},{1024,1019,1019},
    {1280,251,1275},{1280,261,1285}
  };
  for(unsigned i=0;i<sizeof(cases)/sizeof(*cases);++i) {
    unsigned camera[4]={cases[i].camera,cases[i].camera,cases[i].camera,cases[i].camera};
    unsigned scroll[4]={cases[i].scroll,cases[i].scroll,cases[i].scroll,cases[i].scroll};
    LagoonWideRasterRow row;
    CHECK(LagoonWideRaster_SampleRow(camera,scroll,1,&row));
    CHECK(row.x[0]==cases[i].world&&row.x[1]==cases[i].world);
    CHECK(row.y[0]==cases[i].world+1&&row.y[1]==cases[i].world+1);
  }
}
static void clipping(void) {
  static const unsigned widths[]={256,398,682,960};
  unsigned char storage[960*4+32];
  for(unsigned w=0;w<sizeof(widths)/sizeof(*widths);++w) {
    unsigned width=widths[w],left=(width-256)/2;
    for(unsigned mode=0;mode<4;++mode) {
      LagoonWideRasterRow row={{193,193},{250,255}};
      if(mode==1)row.x[1]=203;
      if(mode>=2)row.y[1]=512;
      unsigned layers=mode==3?1:2;
      memset(storage,0x5a,sizeof(storage));
      LagoonWideRaster_MaskRoomRow(storage+16,width,&row,512,512,layers);
      /* Extents project into [left-193,left+319), narrowed by ten pixels
       * at the right if the second source layer is ten pixels farther east. */
      int first=(int)left-193,last=(int)left+319-(mode==1?10:0);
      for(unsigned x=0;x<width;++x)for(unsigned byte=0;byte<4;++byte) {
        int native=x>=left&&x<left+256;
        int inside=(int)x>=first&&(int)x<last&&mode!=2;
        CHECK(storage[16+x*4+byte]==(native||inside?0x5a:0));
      }
      for(unsigned i=0;i<16;++i)CHECK(storage[i]==0x5a);
      for(unsigned i=16+width*4;i<sizeof(storage);++i)CHECK(storage[i]==0x5a);
    }
  }
  /* Invalid calls must not touch even a byte of the destination. */
  LagoonWideRasterRow row={{0},{0}};memset(storage,0x5a,sizeof(storage));
  LagoonWideRaster_MaskRoomRow(storage+16,255,&row,512,512,2);
  LagoonWideRaster_MaskRoomRow(storage+16,960,NULL,512,512,2);
  LagoonWideRaster_MaskRoomRow(storage+16,960,&row,512,512,0);
  LagoonWideRaster_MaskRoomRow(storage+16,960,&row,512,512,3);
  for(unsigned i=0;i<sizeof(storage);++i)CHECK(storage[i]==0x5a);
}
int main(void) {phases();wrapping();clipping();printf("PASS wide raster70: %u coordinate/extent checks; native center preserved\n",checks);return 0;}
