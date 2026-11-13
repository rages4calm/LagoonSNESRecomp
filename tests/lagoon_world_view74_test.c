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
  /* The HUD is moved as source bytes, independently of world terrain. */
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
    for(unsigned y=7;y<23;y++)assert(!memcmp(view+(h-32+y)*pitch+((w-256)/2+64)*4,output+(192+y)*(400*4+16)+(72+64)*4,128*4));
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
  assert(view_origin(0,2048,600,256)==0);
  assert(view_origin(1792,2048,600,256)==1448);
  assert(view_origin(0,256,600,256)==-172);
  LagoonSceneRenderer_Reset();assert(!LagoonSceneRenderer_DrawView(view,2400,600,336,output,1616,400,224,91));
  puts("PASS: zoom125/150, original center, relocated HUD, pitch canaries, map edges, generation/reset refusal");
  return 0;
}
