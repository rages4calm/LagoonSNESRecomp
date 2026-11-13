/* Synthetic admitted-packet composition/lifetime vectors. Actual room/OAM
 * source admission and temporal gameplay captures remain separate evidence. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#define main cloud_base_vectors_main
#include "lagoon_scene_renderer_test.c"
#undef main
static uint8_t view[2400*336];
static void present(ScenePacket *s){
  s_view_serial=0;render(s,400);
  for(unsigned y=0;y<224;y++)memcpy(expected+y*1616,s_pixels+y*400,1600);
  assert(LagoonSceneRenderer_TryPresent(output,1616,expected,1616,400,224,s->generation));
  assert(LagoonSceneRenderer_DrawView(view,2400,600,336,output,1616,400,224,s->generation));
}
int main(void){
  char *args[]={"cloud-vectors",NULL};assert(cloud_base_vectors_main(1,args)==0);
  setup();p.cgram[225]=31<<5;p.cgram[209]=31<<10;
  for(unsigned tile=0;tile<32;tile++)for(unsigned row=0;row<8;row++)p.vram[0x4000+tile*16+row]=255;
  uint8_t *oam=(uint8_t*)p.oam;
  oam[8*4]=248;oam[8*4+1]=64;oam[8*4+2]=0;oam[8*4+3]=0x3c;
  oam[9*4]=248;oam[9*4+1]=64;oam[9*4+2]=4;oam[9*4+3]=0x3a;
  p.highOam[2]=3|(3<<2);
  LagoonSceneRenderer_Reset();s_mode=2;capture(100);
  ScenePacket *s=&s_packets[s_complete];s->room=0x28;s->cloud_tick=200;
  s->cloud.valid=1;s->cloud.count=1;s->cloud.anchor_x=-8;s->cloud.anchor_y=64;
  s->cloud.cells[0]=(LagoonWideCell){-8,64,0x3c00,16};s->cloud.native_slots[8]=1;
  present(s);assert(s_cloud_motion.ready&&s_cloud_graphics.valid);
  int initial=s_cloud_motion.world_x,initialy=s_cloud_motion.world_y;
  /* Narrow guest cloud vanishes from the new complete OAM. Fresh unchanged
   * cloud graphics remain owned; expanded cosmetic lifespan must continue. */
  s->cloud.valid=0;((uint8_t*)s->oam)[8*4+1]=240;
  for(unsigned field=1;field<=12;field++){
    s->cloud_tick=(uint8_t)(200+field/2);s->serial++;present(s);
    assert(s_cloud_motion.ready);
    assert(s_cloud_motion.world_x==initial+(int)(field/2)*6);
    assert(s_cloud_motion.world_y==initialy);
    int prev=s_cloud_motion.world_x;present(s);assert(s_cloud_motion.world_x==prev);
  }
  /* Unrelated actor palette words do not revoke this decorative source. */
  s->pal[209]^=1;s->serial++;present(s);assert(s_cloud_motion.ready);
  /* Color changes belong to the fresh raster, while source-owned character
   * identity and cloud trajectory remain continuous. Damage/fade colors must
   * never reset the seed or flash the cloud back to a remapped native anchor. */
  int before_palette=s_cloud_motion.world_x;
  s->pal[225]^=1;s->serial++;present(s);
  assert(s_cloud_motion.ready&&s_cloud_graphics.valid&&s_cloud_motion.world_x==before_palette);
  unsigned fresh_color_pixels=0;
  for(unsigned y=0;y<336;y++)for(unsigned x=0;x<600;x++){
    uint32_t pixel;memcpy(&pixel,view+y*2400+x*4,4);
    fresh_color_pixels+=pixel==s_rgb15[s->pal[225]&32767];
  }
  assert(fresh_color_pixels>0);
  /* Replacing a cloud character still revokes its retained structural owner. */
  s->vram[0x4000]^=1;s->serial++;present(s);assert(!s_cloud_motion.ready&&!s_cloud_graphics.valid);
  puts("PASS cloud76 compositor: fixed main-tick velocity, independent native retirement, repeated presents, fresh palette colors, exact character revocation");
}
