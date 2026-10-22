/* ROM-free cache properties. Authored room/actor authority and live pixels are
 * qualified separately: these synthetic packets never grant either. */
#define main scene_renderer_existing_test_main
#include "lagoon_scene_renderer_test.c"
#undef main

static uint32_t reference_pixels[800*224];
static unsigned byte_refusals,scalar_refusals,exact_fields;

static void exact_uncached(ScenePacket *s,unsigned width) {
  render(s,width);memcpy(reference_pixels,s_pixels,width*224*4);
  s_background.valid=0;render(s,width);
  assert(!memcmp(reference_pixels,s_pixels,width*224*4));++exact_fields;
}
static void prime(ScenePacket *s,unsigned width) {
  s_background.valid=0;render(s,width);assert(s_background.valid==1);
  render(s,width);assert(s_background.valid==2);
  uint64_t before=s_background_reuses;render(s,width);
  assert(s_background_reuses==before+1);
}
static void bytes(ScenePacket *s,unsigned width,void *data,size_t size) {
  uint8_t *p=data;
  for(size_t i=0;i<size;++i) {
    p[i]^=1;assert(!same_background(s,width));p[i]^=1;++byte_refusals;
  }
  assert(same_background(s,width));
}
#define SCALAR(field) do { \
  s->field^=1;assert(!same_background(s,width));s->field^=1; \
  assert(same_background(s,width));++scalar_refusals; \
} while(0)
static void keys(void) {
  setup();LagoonSceneRenderer_Reset();capture(9);
  ScenePacket *s=&s_packets[s_complete];unsigned width=800;prime(s,width);
  bytes(s,width,s->regs,sizeof(s->regs));bytes(s,width,s->maps,sizeof(s->maps));
  bytes(s,width,s->defs,sizeof(s->defs));bytes(s,width,s->vram,sizeof(s->vram));
  bytes(s,width,s->pal,sizeof(s->pal));
  SCALAR(generation);SCALAR(wx);SCALAR(wy);SCALAR(world_width);SCALAR(world_height);
  SCALAR(native_x);SCALAR(native_y);SCALAR(bg2_x);SCALAR(bg2_y);
  SCALAR(parallax);SCALAR(finite_clip);SCALAR(incoming_mosaic);SCALAR(mosaic_size);
  SCALAR(brightness);SCALAR(mosaic_y_phase);SCALAR(mosaic_backend);SCALAR(mosaic_y_size);
  assert(!same_background(s,682));++scalar_refusals;
  /* Objects do not lend authority to the BG key, and still compose afresh. */
  s->policy.x[0]++;assert(same_background(s,width));s->policy.x[0]--;
  s->oam[0]^=1;assert(same_background(s,width));s->oam[0]^=1;
  s->serial++;assert(same_background(s,width));
  LagoonSceneRenderer_Reset();assert(!s_background.valid);
}
static void scenes(void) {
  const unsigned widths[]={342,448,682,800};
  for(unsigned wi=0;wi<4;++wi)for(unsigned mosaic=0;mosaic<2;++mosaic)
    for(unsigned parallax=0;parallax<2;++parallax) {
      unsigned width=widths[wi];setup();LagoonSceneRenderer_Reset();capture(10);
      ScenePacket *s=&s_packets[s_complete];s->parallax=parallax;
      s->bg2_x=251;s->bg2_y=247;s->finite_clip=1;
      s->incoming_mosaic=mosaic;s->mosaic_size=s->mosaic_y_size=3;s->mosaic_y_phase=1;
      for(unsigned y=0;y<8;++y)s->vram[0x4000+y]=255;
      s->pal[129]=31<<5;uint8_t *o=(uint8_t*)s->oam;
      o[0]=64;o[1]=20;o[2]=0;o[3]=0x30;s->policy.x[0]=64;s->policy.allow[0]=1;
      prime(s,width);
      for(unsigned step=0;step<12;++step) {
        s->serial++;s->policy.x[0]=64+step*3;
        /* The reuse path and a forced full BG redraw must produce the same
         * complete field, including changing object coverage/priority. */
        exact_uncached(s,width);render(s,width);
      }
      /* Representative changed inputs must rebuild and still match an
       * uncached render; the exhaustive byte checks above cover every key. */
      s->pal[1]^=31<<5;exact_uncached(s,width);
      s->vram[0]^=255;exact_uncached(s,width);
      s->maps[0][0]^=1;exact_uncached(s,width);
      s->defs[1][0]^=0x2000;exact_uncached(s,width);
      s->wx++;s->native_x++;exact_uncached(s,width);
      s->wy++;s->native_y++;exact_uncached(s,width);
      s->generation++;exact_uncached(s,width);
      /* Every new moving-camera key stays key-only: avoid decoded-field
       * copies that would never be reused during continuous scrolling. */
      for(unsigned step=0;step<8;++step) {
        s->wx++;s->native_x++;render(s,width);assert(s_background.valid==1);
      }
    }
}
int main(void) {
  keys();scenes();
  printf("PASS background cache: %u complete-byte refusals, %u scalar/width refusals, %u cached/uncached whole fields across342/448/682/800 and ordinary/parallax/mosaic; objects redraw, camera changes and resets invalidate\n",byte_refusals,scalar_refusals,exact_fields);
  return 0;
}
