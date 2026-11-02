/* Synthetic read-only terrain vectors; no ROM art or game execution. */
#define main scene_renderer_existing_test_main
#include "lagoon_scene_renderer_test.c"
#undef main
#ifdef LAGOON_SCENERY_SOURCE_TEST
/* Reuse the existing bounded original-loader oracle, with independent memory.
 * Only its fourteen page loads run below, not its old exhaustive mutant audit.
 * No ROM bytes, decoded art or state are written to disk by this test. */
#define main scenery_page_loader_existing_main
#define rom scenery_loader_rom
#define ram scenery_loader_ram
#define vram scenery_loader_vram
#define word scenery_loader_word
#define source scenery_loader_source
#include "../tools/test_wide_parallax_sources.c"
#undef source
#undef word
#undef vram
#undef ram
#undef rom
#undef main
#endif

static unsigned pixel_checks,refusals,retained_checks,parallax_windows,source_rooms;
static unsigned random_word(unsigned *seed) {
  *seed=*seed*1664525u+1013904223u;return *seed>>16;
}
static uint32_t bytes_hash(const void *data,size_t bytes){
  const uint8_t *p=data;uint32_t h=0;while(bytes--)h=h*33+*p++;return h;
}
/* This oracle decodes bitplanes directly, independently of the production row
 * caches. It always uses world metatiles, including the original native center. */
static uint32_t terrain_oracle(const ScenePacket *s,unsigned x,unsigned y) {
  unsigned color=s->pal[0],priority=0;
  for(unsigned l=0;l<2;++l) {
    unsigned sx=x,sy=y,tile;
    if(l&&s->parallax){
      sx=(unsigned)((int)x-s->wx+(int)s->bg2_x);
      sy=(unsigned)((int)y-s->wy+(int)s->bg2_y);
      tile=s->vram[0x7800+(sy/256%2)*1024+(sy/8%32)*32+sx/8%32];
    }else{
      unsigned id=s->maps[l][(y/16)*128+x/16];
      tile=s->defs[l][id*4+(y%16/8)*2+x%16/8];
      if(l)tile&=~0x2000u;
    }
    unsigned c=scalar_graphics(s,tile,sx%8,sy%8,0,4);
    unsigned z=(tile&0x2000?12:8)-l;
    if(c&&z>priority){priority=z;color=s->pal[(tile>>10&7)*16+c];}
  }
  return 0xff000000u|rgb(color&32767);
}
static void compare(unsigned room,uint64_t generation,unsigned w,unsigned h) {
  /* The vectors below intentionally replace the private packet's synthetic
   * terrain after normal capture. Re-seal only that fixture; runtime production
   * can remember a source solely from a fully qualified EndFrame. */
  remember_map_source(&s_packets[s_complete]);
  s_map_identity_valid=1;s_map_identity_lines=H;
  s_map_identity_room=room;s_map_identity_generation=generation;
  ScenePacket *s=&s_packets[s_complete];
  uint32_t packet_hash=bytes_hash(s,sizeof *s),ram_hash=bytes_hash(ram,sizeof ram);
  uint32_t rom_hash=bytes_hash(rom,sizeof rom),ppu_hash=bytes_hash(&p,sizeof p);
  LagoonSceneRendererStats before=s_stats;
  const uint32_t *image=NULL;unsigned iw=0,ih=0,ww=0,wh=0;
  assert(LagoonSceneRenderer_MapImage(room,generation,&image,&iw,&ih,&ww,&wh));
  assert(image&&ww==w&&wh==h);
  unsigned scale=w>1024||h>1024?2:1;
  assert(iw==(w+scale-1)/scale&&ih==(h+scale-1)/scale&&iw<=1024&&ih<=1024);
  for(unsigned y=0;y<ih;++y)for(unsigned x=0;x<iw;++x){
    assert(image[y*iw+x]==terrain_oracle(s,x*scale,y*scale));++pixel_checks;
  }
  /* Same immutable packet is a cheap cache hit, without touching packet bytes
   * or presentation stats. Cached plane rows are host-only decoding metadata. */
  const uint32_t *again=NULL;
  assert(LagoonSceneRenderer_MapImage(room,generation,&again,&iw,&ih,&ww,&wh)&&again==image);
  assert(packet_hash==bytes_hash(s,sizeof *s)&&ram_hash==bytes_hash(ram,sizeof ram)&&
    rom_hash==bytes_hash(rom,sizeof rom)&&ppu_hash==bytes_hash(&p,sizeof p)&&
    !memcmp(&before,&s_stats,sizeof(before)));
}
static void source_cases(void) {
  setup();ram[0x53]=ram[0x55]=192;LagoonSceneRenderer_Reset();capture(72);
  ScenePacket *s=&s_packets[s_complete];assert(s->valid&&s->room==0);
  unsigned seed=19;
  for(unsigned i=0;i<32768;++i)s->vram[i]=random_word(&seed);
  for(unsigned i=0;i<256;++i)s->pal[i]=random_word(&seed)&32767;
  for(unsigned l=0;l<2;++l) {
    for(unsigned i=0;i<16384;++i)s->maps[l][i]=random_word(&seed);
    for(unsigned i=0;i<1024;++i)s->defs[l][i]=random_word(&seed);
  }
  s->serial=++s_serial;compare(0,72,1536,1536);
  /* Camera position, raw circular tilemap placement, OAM and extra actors are
   * not composited as a second HUD or character layer into the terrain map. */
  uint32_t first=s_map_pixels[0];s->oam[0]^=1;s->wx+=500;s->wy+=500;
  s->serial=++s_serial;compare(0,72,1536,1536);assert(s_map_pixels[0]==first);
  /* New palette/art state on a later packet must replace the current cache. */
  memset(s->vram,0,sizeof(s->vram));s->pal[0]=31<<10;s->serial=++s_serial;
  compare(0,72,1536,1536);assert(s_map_pixels[0]==0xff0000ffu);

  setup();ram[0x48e]=2;ram[0x53]=ram[0x55]=64;
  pointer(0x57,0x128000);pointer(0x5d,0x1a8000);
  pointer(0x69,0x068e90);pointer(0x75,0x068e90);
  LagoonSceneRenderer_Reset();capture(73);s=&s_packets[s_complete];assert(s->valid&&s->room==2);
  for(unsigned i=0;i<32768;++i)s->vram[i]=random_word(&seed);
  for(unsigned i=0;i<256;++i)s->pal[i]=random_word(&seed)&32767;
  for(unsigned l=0;l<2;++l){
    for(unsigned i=0;i<16384;++i)s->maps[l][i]=random_word(&seed);
    for(unsigned i=0;i<1024;++i)s->defs[l][i]=random_word(&seed);
  }
  s->serial=++s_serial;compare(2,73,512,512);
  /* Largest admitted128x128 metatile map, with last row/column sampled. */
  s->world_width=s->world_height=2048;s->serial=++s_serial;compare(2,73,2048,2048);
}
static void refuse(unsigned room,uint64_t generation) {
  const uint32_t *image=(const uint32_t *)1;unsigned w=1,h=1,ww=1,wh=1;
  assert(!LagoonSceneRenderer_MapImage(room,generation,&image,&w,&h,&ww,&wh));
  assert(!image&&!w&&!h&&!ww&&!wh&&!s_map_valid);++refusals;
}
static void guards(void) {
  setup();ram[0x53]=ram[0x55]=192;LagoonSceneRenderer_Reset();capture(74);
  ScenePacket *s=&s_packets[s_complete];assert(s->valid);
  compare(0,74,1536,1536);refuse(2,74);refuse(75,74);refuse(0,75);
  for(unsigned i=0;i<10;++i){
    SceneMapSource before=s_map_source;
    unsigned valid=s_map_identity_valid,lines=s_map_identity_lines,identity_room=s_map_identity_room;
    uint64_t identity_generation=s_map_identity_generation;int mode=s_mode;
    switch(i){
      case 0:s_map_source.valid=0;break;case 1:s_map_identity_lines=223;break;
      case 2:s_map_identity_room=2;break;case 3:s_map_identity_generation=75;break;
      case 4:s_map_identity_valid=0;break;case 5:s_map_source.world_width=2049;break;
      case 6:s_map_source.world_height=2049;break;case 7:s_map_source.world_width=0;break;
      case 8:s_map_source.world_height=0;break;case 9:s_mode=0;break;
    }
    refuse(0,74);s_map_source=before;s_map_identity_valid=valid;s_map_identity_lines=lines;
    s_map_identity_room=identity_room;s_map_identity_generation=identity_generation;s_mode=mode;
  }
  LagoonSceneRenderer_Reset();refuse(0,74);
}
static void retained(void){
  setup();ram[0x53]=ram[0x55]=192;LagoonSceneRenderer_Reset();capture(80);
  const uint32_t *image;unsigned w,h;assert(LagoonSceneRenderer_MapImage(0,80,&image,&w,&h,NULL,NULL));
  uint32_t first=image[0];unsigned source_serial=s_map_source.serial;
  /* The complete world packet refuses a mid-field OAM write; immutable map
   * source remains usable after a fresh full-raster terrain identity check. */
  for(unsigned line=1;line<=H;++line){if(line==100){p.oam[0]^=1;p.oamWriteCount++;}
    LagoonSceneRenderer_ObserveRaster(&p,line,ram,rom,sizeof rom,80,LAGOON_SCENE_WORLD);}
  LagoonSceneRenderer_EndFrame(101,80,LAGOON_SCENE_WORLD);assert(!s_packets[s_complete].valid);
  assert(LagoonSceneRenderer_MapImage(0,80,&image,&w,&h,NULL,NULL)&&image[0]==first);
  assert(s_map_source.serial==source_serial);++retained_checks;
  for(unsigned line=1;line<=H;++line){
    if(line==100){p.vram[0x7000]^=1;p.vramWriteCount++;}
    LagoonSceneRenderer_ObserveRaster(&p,line,ram,rom,sizeof rom,80,LAGOON_SCENE_WORLD);
  }
  LagoonSceneRenderer_EndFrame(102,80,LAGOON_SCENE_WORLD);assert(!s_packets[s_complete].valid);
  assert(LagoonSceneRenderer_MapImage(0,80,&image,&w,&h,NULL,NULL)&&image[0]==first);++retained_checks;
  /* Two more failed fields overwrite both actor packets. Only the dedicated
   * 74KiB terrain snapshot can preserve this unchanged current-area source. */
  p.hScroll[1]++;
  for(unsigned n=0;n<2;++n){capture(80);assert(!s_packets[s_complete].valid);
    assert(LagoonSceneRenderer_MapImage(0,80,&image,&w,&h,NULL,NULL)&&image[0]==first);++retained_checks;}
  /* A frame with no raster observations cannot reuse last field's identity. */
  LagoonSceneRenderer_EndFrame(105,80,LAGOON_SCENE_WORLD);refuse(0,80);
  for(unsigned bad=0;bad<15;++bad){
    setup();ram[0x53]=ram[0x55]=192;LagoonSceneRenderer_Reset();capture(81);
    p.hScroll[1]++;uint64_t generation=81;int authority=LAGOON_SCENE_WORLD;
    switch(bad){
      case 0:ram[0x10000]^=1;break;case 1:ram[0x14000]^=1;break;
      case 2:rom[0x30000]^=1;break;case 3:p.vram[0]^=1;break;
      case 4:p.cgram[0]^=1;break;case 5:ram[0x100e]=1;break;
      case 6:p.inidisp=14;break;case 7:p.mosaic=1;break;
      case 8:ram[0x50]=1;break;case 9:ram[0x48e]=1;break;
      case 10:generation=82;break;case 11:authority=0;break;
      case 12:p.bgXsc[0]^=1;break;case 13:p.screenEnabled[0]=15;break;
      case 14:p.windowsel=1;break;
    }
    for(unsigned line=1;line<=H;++line)
      LagoonSceneRenderer_ObserveRaster(&p,line,ram,rom,sizeof rom,generation,authority);
    LagoonSceneRenderer_EndFrame(106,generation,authority);assert(!s_packets[s_complete].valid);
    refuse(0,81);if(bad==9)refuse(1,81);if(bad==10)refuse(0,82);
  }
  /* OBJ art/palettes and circular tilemap uploads are irrelevant to terrain;
   * unchanged BG resources alone qualify reuse, never world replacement. */
  setup();ram[0x53]=ram[0x55]=192;LagoonSceneRenderer_Reset();capture(83);
  p.hScroll[1]++;p.vram[0x4000]^=1;p.vram[0x7000]^=1;p.cgram[128]^=1;capture(83);
  assert(!s_packets[s_complete].valid&&LagoonSceneRenderer_MapImage(0,83,&image,&w,&h,NULL,NULL));++retained_checks;
  s_serial=~0u;capture(83);assert(!s_packets[s_complete].valid);refuse(0,83);
  assert(sizeof s_map_source<75*1024);
}
static void parallax_vectors(void){
  setup();ram[0x53]=ram[0x55]=192;LagoonSceneRenderer_Reset();capture(84);
  ScenePacket *s=&s_packets[s_complete];s->parallax=1;
  unsigned seed=191;
  for(unsigned i=0;i<32768;++i)s->vram[i]=random_word(&seed);
  for(unsigned i=0;i<128;++i)s->pal[i]=random_word(&seed)&32767;
  for(unsigned l=0;l<2;++l){
    for(unsigned i=0;i<16384;++i)s->maps[l][i]=random_word(&seed);
    for(unsigned i=0;i<1024;++i)s->defs[l][i]=random_word(&seed);
  }
  const int anchors[][2]={{0,0},{3,255},{255,256},{511,512},{1023,1020},{1536,1824}};
  const unsigned widths[]={256,342,682,800};
  for(unsigned phase=0;phase<6;++phase){
    s->wx=anchors[phase][0];s->wy=anchors[phase][1]+1;
    s->native_x=(unsigned)s->wx&1023;s->native_y=(unsigned)s->wy&1023;
    s->bg2_x=(unsigned)s->wx/4;s->bg2_y=(unsigned)(s->wy-1)/4+1;
    s->world_width=phase<4?1024:2048;s->world_height=phase<4?1024:2048;
    s->serial=++s_serial;compare(0,84,s->world_width,s->world_height);
    /* Full-zone BG2 projection must be identical to the original native and
     * widened live-window equation, including negative/page wrapping positions.
     * Dividing each world pixel by4 would fail this scalar identity. */
    for(unsigned wi=0;wi<4;++wi)for(unsigned y=0;y<H;++y){
      unsigned width=widths[wi];int left=(int)(width-256)/2;
      uint8_t depth[800]={0};uint16_t colors[800]={0};
      background_row(s,width,y,left,1,depth,colors);
      for(unsigned x=0;x<width;++x){
        int wx=s->wx+(int)x-left,wy=s->wy+(int)y;
        unsigned sx=(unsigned)(wx-s->wx+(int)s->bg2_x),sy=(unsigned)(wy-s->wy+(int)s->bg2_y);
        unsigned tile=s->vram[0x7800+(sy/256%2)*1024+(sy/8%32)*32+sx/8%32];
        unsigned c=scalar_graphics(s,tile,sx%8,sy%8,0,4),z=tile&0x2000?11:7;
        assert(depth[x]==(c?z:0)&&colors[x]==(c?s->pal[(tile>>10&7)*16+c]:0));++parallax_windows;
      }
    }
  }
}
#ifdef LAGOON_SCENERY_SOURCE_TEST
static void owned_parallax(const char *path){
  FILE *f=fopen(path,"rb");assert(f&&fread(scenery_loader_rom,1,sizeof scenery_loader_rom,f)==sizeof scenery_loader_rom);fclose(f);
  maincpu=interp816_init(NULL,readb,writeb);assert(maincpu);
  for(unsigned n=0;n<7;++n){
    current=n;scene=rooms[n];page=0;memset(scenery_loader_ram,0,sizeof scenery_loader_ram);
    memset(io,0,sizeof io);memset(scenery_loader_vram,0,sizeof scenery_loader_vram);
    memcpy(scenery_loader_ram+0x50,scenery_loader_rom+off(0x05b2df+scene*56),56);
    scenery_loader_ram[0x48e]=scene;initcpu(maincpu,1,0x8a92,0x1d0,2);
    unsigned ops=0;
    while(maincpu->k!=1||maincpu->pc!=0x7ff0){
      assert(++ops<1000000);
      if(maincpu->k==0&&maincpu->pc==0x9143){scenery_loader_source=scenery_loader_ram[0xe0]|scenery_loader_ram[0xe1]<<8|scenery_loader_ram[0xe2]<<16;maximum=scenery_loader_source;}
      if(maincpu->k==0&&maincpu->pc==0x89ac&&(scenery_loader_ram[0x23a]&0x20))maincpu->nmiWanted=true;
      interp816_runOpcode(maincpu);
    }
    assert(maincpu->sp==0x1d2&&page==2);
    for(unsigned w=0;w<2048;++w)original_pages[n][w]=scenery_loader_word(scenery_loader_vram,0xf000+w*2);
  }
  assert(jobs==14);interp816_free(maincpu);
  for(unsigned n=0;n<7;++n){
    setup();memcpy(rom,scenery_loader_rom,sizeof rom);
    memcpy(ram+0x50,rom+off(0x05b2df+rooms[n]*56),56);ram[0x48e]=rooms[n];
    memcpy(p.vram+0x7800,original_pages[n],4096);p.bgXsc[1]=0x7a;
    unsigned cx=n?1533:383,cy=n?1021:511;
    ram[0x480]=cx;ram[0x481]=cx>>8;ram[0x482]=cy;ram[0x483]=cy>>8;
    ram[0x484]=cx>>2;ram[0x485]=cx>>10;ram[0x486]=cy>>2;ram[0x487]=cy>>10;
    p.hScroll[0]=cx&1023;p.vScroll[0]=cy&1023;p.hScroll[1]=cx>>2;p.vScroll[1]=cy>>2;
    unsigned seed=103+n;
    for(unsigned i=0;i<16384;++i)p.vram[i]=random_word(&seed);
    for(unsigned i=0;i<128;++i)p.cgram[i]=random_word(&seed)&32767;
    LagoonSceneRenderer_Reset();assert(LagoonSceneRenderer_ParallaxRoom(ram));
    assert(supported(&p,ram,rom,sizeof rom,LAGOON_SCENE_WORLD));capture(90+n);
    ScenePacket *s=&s_packets[s_complete];assert(s->valid&&s->parallax&&s->room==rooms[n]);
    compare(rooms[n],90+n,word(ram,0x53)*8,word(ram,0x55)*8);++source_rooms;
    const uint32_t *image;unsigned width,height;
    p.hScroll[1]++;capture(90+n);assert(!s_packets[s_complete].valid);
    assert(LagoonSceneRenderer_MapImage(rooms[n],90+n,&image,&width,&height,NULL,NULL));++retained_checks;
    p.vram[0x7800]^=1;capture(90+n);assert(!s_packets[s_complete].valid);refuse(rooms[n],90+n);
    p.vram[0x7800]^=1;rom[off(sources[n][0])]^=1;capture(90+n);
    assert(!s_packets[s_complete].valid);refuse(rooms[n],90+n);
  }
}
#endif
#ifndef LAGOON_SCENERY72_ENTRY
#define LAGOON_SCENERY72_ENTRY main
#endif
int LAGOON_SCENERY72_ENTRY(int argc,char **argv){
  source_cases();guards();retained();parallax_vectors();
#ifdef LAGOON_SCENERY_SOURCE_TEST
  assert(argc==2);owned_parallax(argv[1]);
#else
  (void)argv;assert(argc==1);
#endif
  printf("PASS full-zone scenery: %u independent ARGB pixel checks, %u stale/context refusals, %u retained-source checks, %u parallax window samples, %u source-owned parallax rooms; no guest/presentation mutation\n",pixel_checks,refusals,retained_checks,parallax_windows,source_rooms);
  printf("Terrain source is %zu bytes; no objects, HUD, framebuffer or historical-room cache\n",sizeof s_map_source);
  return 0;
}
