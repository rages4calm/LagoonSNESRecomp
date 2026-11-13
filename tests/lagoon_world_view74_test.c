/* Geometry/composition vectors, not a gameplay or GPU qualification. */
#define main renderer_vectors_main
#include "lagoon_scene_renderer_test.c"
#undef main
static uint8_t view[(1280*4+16)*336+32];
int main(void){
  char *args[]={"vectors",NULL};assert(renderer_vectors_main(1,args)==0);
  setup();LagoonSceneRenderer_Reset();s_mode=2;capture(91);
  render(&s_packets[s_complete],400);
  for(unsigned y=0;y<224;y++)memcpy(expected+y*(400*4+16),s_pixels+y*400,400*4);
  assert(LagoonSceneRenderer_TryPresent(output,400*4+16,expected,400*4+16,400,224,91));
  /* Raw HUD-underlay bytes must never be relocated into the new view. */
  for(unsigned y=192;y<224;y++)memset(output+y*(400*4+16),0x46,400*4);
  memset(output+191*(400*4+16),0x55,400*4); /* Old HUD top must never float over world. */
  for(unsigned scale=125;scale<=150;scale+=25){
    unsigned w=400*scale/100,h=224*scale/100,pitch=w*4+16;
    memset(view,0xa5,sizeof(view));
    assert(LagoonSceneRenderer_DrawView(view,pitch,w,h,output,400*4+16,400,224,91));
    const ScenePacket *s=&s_packets[s_complete];
    int left=s->wx-view_origin(s->wx,s->world_width,w,256);
    int top=s->wy-view_origin(s->wy,s->world_height,h,224);
    for(unsigned y=0;y<190;y++)assert(!memcmp(view+(top+y)*pitch+left*4,output+y*(400*4+16)+72*4,256*4));
    for(unsigned y=h-34;y<h;y++)for(unsigned x=0;x<w;x++){
      uint32_t c;memcpy(&c,view+y*pitch+x*4,4);assert(c!=0x46464646&&c!=0x55555555);
    }
    uint32_t old_hud;memcpy(&old_hud,view+(top+191)*pitch+(left+128)*4,4);assert(old_hud==0x00ff0000);
    uint32_t corner;memcpy(&corner,view+(h-1)*pitch,4);assert(corner==0x00ff0000);
    for(unsigned y=0;y<h;y++)for(unsigned x=w*4;x<pitch;x++)assert(view[y*pitch+x]==0xa5);
    assert(view[h*pitch]==0xa5);
    assert(!LagoonSceneRenderer_DrawView(view,pitch,w,h,output,400*4+16,400,224,92));
  }
  /* Screen-owned Game Over frame must not follow an edge-clamped camera.
   * This synthetic admitted packet tests composition, not source admission. */
  ScenePacket *death=&s_packets[s_complete];death->gameover=1;
  for(unsigned row=0;row<8;row++)death->vram[0x6000+row]=0xffff;
  for(unsigned y=86;y<170;y++)for(unsigned x=46;x<208;x++) {
    uint32_t marker=0x00aabbcc;memcpy(output+y*(400*4+16)+(72+x)*4,&marker,4);
  }
  assert(LagoonSceneRenderer_DrawView(view,2400,600,336,output,1616,400,224,91));
  uint32_t panel;memcpy(&panel,view+(336/2)*2400+(600/2)*4,4);assert(panel==0x00aabbcc);
  death->gameover=0;
  /* A relocated cloud must reveal the complete actor underneath its OLD
   * position, including native-left seam pixels. Bare terrain is insufficient. */
  setup();p.cgram[225]=31<<5;p.cgram[209]=31<<10;
  for(unsigned tile=0;tile<32;tile++)for(unsigned row=0;row<8;row++)p.vram[0x4000+tile*16+row]=255;
  uint8_t *oam=(uint8_t*)p.oam;
  oam[8*4]=248;oam[8*4+1]=64;oam[8*4+2]=0;oam[8*4+3]=0x3c;
  oam[9*4]=248;oam[9*4+1]=64;oam[9*4+2]=4;oam[9*4+3]=0x3a;
  p.highOam[2]=3|(3<<2);
  LagoonSceneRenderer_Reset();s_mode=2;capture(93);ScenePacket *cloud=&s_packets[s_complete];
  cloud->cloud.valid=1;cloud->cloud.count=1;cloud->cloud.anchor_x=-8;cloud->cloud.anchor_y=64;
  cloud->cloud.cells[0]=(LagoonWideCell){-8,64,0x3c00,16};cloud->cloud.native_slots[8]=1;
  render(cloud,400);for(unsigned y=0;y<224;y++)memcpy(expected+y*1616,s_pixels+y*400,1600);
  assert(LagoonSceneRenderer_TryPresent(output,1616,expected,1616,400,224,93));
  assert(LagoonSceneRenderer_DrawView(view,2400,600,336,output,1616,400,224,93));
  int lx=cloud->wx-view_origin(cloud->wx,cloud->world_width,600,256);
  int ty=cloud->wy-view_origin(cloud->wy,cloud->world_height,336,224);
  uint32_t behind;memcpy(&behind,view+(ty+68)*2400+(lx-7)*4,4);assert(behind==0x0000ff);
  assert(view_origin(0,2048,600,256)==0);
  assert(view_origin(1792,2048,600,256)==1448);
  assert(view_origin(0,256,600,256)==-172);
  LagoonSceneRenderer_Reset();assert(!LagoonSceneRenderer_DrawView(view,2400,600,336,output,1616,400,224,91));
  puts("PASS: zoom125/150, original center, source-owned HUD, pitch canaries, map edges, generation/reset refusal");
  return 0;
}
