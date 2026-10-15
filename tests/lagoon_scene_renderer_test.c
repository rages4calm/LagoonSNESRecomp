/* ROM-free vectors prove independent tile decoding and fail-closed presentation.
 * Runtime route evidence is separate; these synthetic vectors do not qualify a room. */
#include <assert.h>
#include "../src/lagoon_scene_renderer.c"
static Ppu p;
static uint8_t ram[131072],rom[1048576];
static uint8_t expected[(800*4+16)*224],output[(800*4+16)*224];
static void pointer(unsigned a,unsigned value){ram[a]=value;ram[a+1]=value>>8;ram[a+2]=value>>16;}
static void setup(void) {
  memset(&p,0,sizeof(p));memset(ram,0,sizeof(ram));memset(rom,0,sizeof(rom));
  pointer(0x57,0x138000);pointer(0x5d,0x198000);pointer(0x69,0x068000);
  pointer(0x75,0x068000);pointer(0x88,0x7f0000);pointer(0x8b,0x7f4000);
  ram[0x53]=128;ram[0x55]=128;p.inidisp=15;p.bgmode=9;p.obsel=2;
  p.bgXsc[0]=0x71;p.bgXsc[1]=0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;p.screenEnabled[0]=31;
  ram[0x480]=128;ram[0x482]=128;p.hScroll[0]=p.hScroll[1]=128;p.vScroll[0]=p.vScroll[1]=128;
  for(unsigned i=0;i<128;++i)((uint8_t *)p.oam)[i*4+1]=240;
  for(unsigned y=0;y<8;++y)p.vram[y]=255; /* tile0 color1, from planar data */
  p.cgram[1]=31; /* red */
}
static void capture(uint64_t generation) {
  for(unsigned i=1;i<=224;++i)LagoonSceneRenderer_ObserveRaster(&p,i,ram,rom,sizeof(rom),generation,1);
  LagoonSceneRenderer_EndFrame(100,generation,1);
}
static void source(unsigned width,size_t pitch) {
  memset(expected,0xa5,sizeof(expected));
  for(unsigned y=0;y<224;++y)for(unsigned x=0;x<width;++x){uint32_t c=0xff0000;memcpy(expected+y*pitch+x*4,&c,4);}
  memset(output,0x59,sizeof(output));
}
static unsigned scalar_graphics(const ScenePacket *s,unsigned tile,unsigned x,unsigned y,unsigned base,unsigned bpp) {
  if(tile&0x4000)x=7-x;if(tile&0x8000)y=7-y;
  unsigned adr=(base+(tile&1023)*(bpp==2?8:16)+y)&32767;
  uint32_t planes=s->vram[adr];if(bpp==4)planes|=(uint32_t)s->vram[(adr+8)&32767]<<16;
  unsigned c=0;for(unsigned i=0;i<bpp;++i)c|=((planes>>(7-x+i*8))&1)<<i;return c;
}
static void cache_oracle(void) {
  ScenePacket *s=&s_packets[0];memset(s,0,sizeof(*s));s->serial=++s_serial;
  unsigned seed=5;
  for(unsigned i=0;i<32768;++i){seed=seed*1664525u+1013904223u;s->vram[i]=seed>>16;}
  for(unsigned i=0;i<256;++i)s->pal[i]=(i*7919)&32767;
  for(unsigned l=0;l<2;++l)for(unsigned i=0;i<16384;++i)s->maps[l][i]=i;
  for(unsigned l=0;l<2;++l)for(unsigned i=0;i<1024;++i)s->defs[l][i]=(i*7919)&65535;
  for(unsigned i=0;i<1024;++i)for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x)
    for(unsigned bpp=2;bpp<=4;bpp+=2)
      assert(graphics(s,i|(i<<6),x,y,bpp==2?0x6000:0x4000,bpp)==scalar_graphics(s,i|(i<<6),x,y,bpp==2?0x6000:0x4000,bpp));
  s->native_x=137;s->native_y=143;s->wx=3;s->wy=143;s->world_width=s->world_height=1024;
  const unsigned widths[]={256,342,682,800};
  for(unsigned wi=0;wi<4;++wi)for(unsigned y=0;y<224;++y) {
    unsigned width=widths[wi];int left=(int)(width-256)/2;
    uint8_t depth[800]={0};uint16_t colors[800];for(unsigned x=0;x<width;++x)colors[x]=s->pal[0];
    background_row(s,width,y,left,0,depth,colors);background_row(s,width,y,left,1,depth,colors);
    for(unsigned x=0;x<width;++x) {
      unsigned expected_z=0,expected_color=s->pal[0];
      for(unsigned l=0;l<2;++l) {
        int native=(int)x>=left&&(int)x<left+256;
        int qx=(native?(int)s->native_x:s->wx)+(int)x-left,qy=(native?(int)s->native_y:s->wy)+(int)y;
        unsigned tile;
        if(native)tile=s->vram[(0x7000+l*0x800+((qy>>3)&31)*32+((qx>>3)&31)+((qx&256)?0x400:0))&32767];
        else {if(qx<0||qy<0||qx>=s->world_width||qy>=s->world_height)continue;
          unsigned meta=s->maps[l][(qy>>4)*128+(qx>>4)];tile=s->defs[l][meta*4+((qx&8)>>3)+((qy&8)>>2)];if(l)tile&=0xdfff;}
        unsigned c=scalar_graphics(s,tile,qx&7,qy&7,0,4),z=((tile&0x2000)?12:8)-l;
        if(c&&z>expected_z){expected_z=z;expected_color=s->pal[((tile>>10)&7)*16+c];}
      }
      assert(depth[x]==expected_z&&colors[x]==expected_color);
    }
  }
}
int main(void) {
  setup();LagoonSceneRenderer_Reset();s_mode=2;
  for(unsigned width=256;width<=800;width+=272) {
    size_t pitch=width*4+16;capture(7);source(width,pitch);
    /* The left margin can extend beyond the world at wx128: position camera
     * away from that edge for the widest vector. */
    if(width>512){ram[0x481]=1;p.hScroll[0]=p.hScroll[1]=384;capture(7);}
    assert(LagoonSceneRenderer_TryPresent(output,pitch,expected,pitch,width,224,7));
    uint64_t first_render_count=s_stats.renders;
    capture(7);assert(LagoonSceneRenderer_TryPresent(output,pitch,expected,pitch,width,224,7));
    assert(s_stats.renders==first_render_count); /* identical next field can reuse */
    for(unsigned y=0;y<224;++y){assert(!memcmp(output+y*pitch,expected+y*pitch,width*4));for(unsigned x=width*4;x<pitch;++x)assert(output[y*pitch+x]==0x59);}
    uint64_t count=s_stats.renders;
    memset(ram,0,sizeof(ram));memset(&p,0,sizeof(p));memset(rom,0,sizeof(rom));
    assert(LagoonSceneRenderer_TryPresent(output,pitch,expected,pitch,width,224,7));
    assert(s_stats.renders==count); /* sealed packet never consults live guest */
    expected[3]=1;memset(output,0x59,sizeof(output));
    unsigned details=s_mismatch_count;s_report[0]='@';s_report[1]=0; /* collect only; no I/O */
    assert(!LagoonSceneRenderer_TryPresent(output,pitch,expected,pitch,width,224,7));
    assert(s_mismatch_count==details+1&&s_mismatch[details].count==1&&
      s_mismatch[details].x==0&&s_mismatch[details].y==0&&s_mismatch[details].candidate==0xff0000&&
      s_mismatch[details].reference==0x01ff0000);s_report[0]=0;
    for(unsigned i=0;i<sizeof(output);++i)assert(output[i]==0x59); /* alpha is compared too */
    expected[3]=0;s_mode=1;
    assert(!LagoonSceneRenderer_TryPresent(output,pitch,expected,pitch,width,224,7));
    assert(!LagoonSceneRenderer_TryPresent(output,pitch,expected,pitch,width,224,8));
    s_mode=2;setup();
  }
  /* Missing line, changed register, memory write, changing palette, incomplete
   * generation, malformed extra object, room/authority refusal. */
  for(unsigned bad=0;bad<8;++bad) {
    setup();LagoonSceneRenderer_Reset();s_mode=2;
    PpuExtraObject invalid={0};
    if(bad==5){invalid.width=17;invalid.height=1;invalid.stride=17;invalid.pixels=p.cgram;p.extraObjectCount=1;p.extraObjects=&invalid;}
    if(bad==6)ram[0x48e]=1;
    for(unsigned i=1;i<=224;++i) {
      if(i==100){if(bad==0)continue;if(bad==1)p.mosaic=1;if(bad==2)++p.vramWriteCount;
        if(bad==3)p.cgram[1]=1;}
      LagoonSceneRenderer_ObserveRaster(&p,i,ram,rom,sizeof(rom),bad==4&&i>100?8:7,bad!=7);
    }
    LagoonSceneRenderer_EndFrame(101,7,1);source(256,1024);
    assert(!LagoonSceneRenderer_TryPresent(output,1024,expected,1024,256,224,7));
  }
  setup();LagoonSceneRenderer_Reset();s_mode=2;p.extraObjectCount=1;
  /* First-line object pointer must be checked before use. */
  p.extraObjects=NULL;
  /* supported rejects this malformed host input. */
  assert(!supported(&p,ram,rom,sizeof(rom),1));
  setup();capture(7);LagoonSceneRenderer_Reset();s_mode=2;
  assert(!LagoonSceneRenderer_TryPresent(output,1024,expected,1024,256,224,7));
  setup();capture(7);
  assert(!LagoonSceneRenderer_TryPresent(output,1023,expected,1024,256,224,7));
  assert(!LagoonSceneRenderer_TryPresent(output,1024,expected,1024,256,223,7));
  assert(!LagoonSceneRenderer_TryPresent(output,1024,expected,1024,255,224,7));
  /* Native OBJ, BG3 text priority, copied margin cells and tie ordering are
   * independently reconstructed, rather than copied from the reference. */
  setup();
  for(unsigned yy=0;yy<8;++yy){p.vram[0x4000+yy]=255;p.vram[0x6008+yy]=255;}
  p.cgram[129]=31<<5;p.cgram[5]=31<<10;
  uint8_t *o=(uint8_t *)p.oam;o[0]=8;o[1]=6;o[2]=0;o[3]=0x30;
  p.vram[0x3c00+32+1]=0x2401; /* high-priority BG3, palette1, tile1 */
  uint16_t cell[1]={0x801f};PpuExtraObject extra={-1,7,1,1,1,3,0,0,1,cell};
  p.extraObjects=&extra;p.extraObjectCount=1;capture(7);
  ScenePacket *packet=&s_packets[s_complete];assert(packet->valid);render(packet,528);
  assert(s_pixels[7*528+136+8]==0x000000ff); /* BG3 wins over OBJ */
  assert(s_pixels[6*528+136+8]==0x0000ff00); /* OBJ outside UI tile */
  assert(s_pixels[7*528+135]==0x00ff0000); /* copied extra cell in margin */
  memset(cell,0,sizeof(cell));assert(packet->objects[0].pixels[0]==0x801f);
  packet->vram[17]=0x80;packet->serial=++s_serial; /* new immutable-source identity */
  assert(graphics(packet,1,0,1,0,4)==1&&graphics(packet,0x4001,7,1,0,4)==1);
  assert(graphics(packet,0x8001,0,6,0,4)==1);
  packet->vram[17]=0x8000;packet->serial=++s_serial;
  assert(graphics(packet,1,0,1,0,4)==2); /* same address, new packet invalidates cache */
  ScenePacket *other=&s_packets[s_complete^1];memcpy(other,packet,sizeof(*other));
  assert(same_source(packet,other));
  ++other->native_x;assert(!same_source(packet,other));--other->native_x;
  other->pal[1]^=1;assert(!same_source(packet,other));other->pal[1]^=1;
  other->vram[0]^=1;assert(!same_source(packet,other));other->vram[0]^=1;
  other->oam[0]^=1;assert(!same_source(packet,other));other->oam[0]^=1;
  other->objects[0].pixels[0]^=1;assert(!same_source(packet,other));other->objects[0].pixels[0]^=1;
  /* Native tilemap upload history can differ from retained maps at a streamer
   * boundary. Decode its bottom row from VRAM; keep world margins independent. */
  setup();for(unsigned yy=0;yy<8;++yy)p.vram[16+yy]=255;p.cgram[17]=31<<5;
  for(unsigned xx=0;xx<64;++xx)p.vram[0x7000+12*32+(xx&31)+(xx&32?0x400:0)]=0x0401;
  capture(7);packet=&s_packets[s_complete];render(packet,528);
  assert(s_pixels[223*528+136]==0x0000ff00&&s_pixels[223*528+391]==0x0000ff00);
  assert(s_pixels[223*528+135]==0x00ff0000&&s_pixels[223*528+392]==0x00ff0000);
  assert(s_pixels[222*528+136]==0x00ff0000);
  /* Actual nine-bit placement policy: preserve admitted right-edge partials,
   * exclude unhinted parked cells, and retain already-authorized left grace. */
  setup();p.wsOamLeftHintStrict=p.wsOamRightHintStrict=1;p.extraLeftCur=p.extraRightCur=213;
  for(unsigned yy=0;yy<8;++yy)p.vram[0x4000+yy]=255;p.cgram[129]=31<<5;
  o=(uint8_t *)p.oam;o[0]=252;o[1]=10;o[3]=0x30;capture(7);render(&s_packets[s_complete],682);
  assert(s_pixels[10*682+472]==0x0000ff00); /* x259, beyond native right edge */
  o[0]=64;p.highOam[0]=1;o[1]=219;capture(7);render(&s_packets[s_complete],682);
  assert(s_pixels[219*682+21]==0); /* raw320 parked -> -192, refused */
  o[0]=78;p.wsOamMotionGraceOn=1;p.wsOamMotionGrace[0]=4;capture(7);
  render(&s_packets[s_complete],682);assert(s_pixels[219*682+35]==0x0000ff00);
  assert(s_pixels[223*682+35]==0x0000ff00); /* bottom-row source cell clipping */
  setup();for(unsigned line=1;line<=224;++line){if(line==100)p.wsOamLeftHintStrict=1;
    LagoonSceneRenderer_ObserveRaster(&p,line,ram,rom,sizeof(rom),7,1);}
  LagoonSceneRenderer_EndFrame(102,7,1);assert(!s_packets[s_complete].valid);
  cache_oracle();
  puts("PASS independent planar rendering, immutable cached packets, full-word/pitch gate, verify/auto, reset and raster refusal");return 0;
}
