/* ROM-free vectors prove independent tile decoding and fail-closed presentation.
 * Runtime route evidence is separate; these synthetic vectors do not qualify a room. */
#include <assert.h>
#define LAGOON_WIDE_ENTITIES_DECODER_ONLY
#include "../src/lagoon_wide_entities.c"
#include "../src/lagoon_scene_renderer.c"
/* Exercise the actual shared shadow API without changing SDK source. */
#include "../snesrecomp/runner/src/snes/ws_shadow.c"
int snes_frame_counter;
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
static void cave_setup(void) {
  setup();ram[0x48e]=3;
  pointer(0x57,0x128b77);pointer(0x5d,0x1d8000);
  pointer(0x69,0x069298);pointer(0x75,0x069298);
  ram[0x53]=224;ram[0x55]=204;
  ram[0x480]=128;ram[0x481]=1;p.hScroll[0]=p.hScroll[1]=384;
  /* Deliberately different from Atland's red definitions. Native VRAM remains
   * red; only room-owned map margins resolve the cave green tile/palette. */
  for(unsigned i=0;i<4;++i){rom[0x31298+i*2]=1;rom[0x31299+i*2]=4;}
  for(unsigned y=0;y<8;++y)p.vram[16+y]=255;
  p.cgram[17]=31<<5;
}
static void cave_vectors(void) {
  cave_setup();LagoonSceneRenderer_Reset();s_mode=2;
  unsigned definitions=0;assert(scene_resources(ram,&definitions)&&definitions==0x31298);
  capture(19);ScenePacket *s=&s_packets[s_complete];assert(s->valid);
  assert(s->world_width==1792&&s->world_height==1632&&s->defs[0][0]==0x401&&s->defs[1][0]==0x401);
  render(s,682);
  for(unsigned y=0;y<224;++y)for(unsigned x=0;x<682;++x) {
    uint32_t color=(x>=213&&x<469)?0xff0000:0x00ff00;
    assert(s_pixels[y*682+x]==color);
    memcpy(expected+(y*682+x)*4,&color,4);
  }
  assert(LagoonSceneRenderer_TryPresent(output,682*4,expected,682*4,682,224,19));
  /* Exact descriptor, finite extent, ordinary raster only. No broad caves,
   * high room byte aliases, fades, menu, parallax, math or layout permission. */
  for(unsigned bad=0;bad<20;++bad) {
    cave_setup();LagoonSceneRenderer_Reset();s_mode=2;
    switch(bad) {
      case 0:ram[0x48e]=4;break;case 1:ram[0x48f]=1;break;
      case 2:ram[0x57]^=1;break;case 3:ram[0x5d]^=1;break;
      case 4:ram[0x69]^=1;break;case 5:ram[0x75]^=1;break;
      case 6:ram[0x88]^=1;break;case 7:ram[0x8b]^=1;break;
      case 8:ram[0x53]--;break;case 9:ram[0x55]++;break;
      case 10:ram[0x100e]=1;break;case 11:ram[0x50]=2;break;
      case 12:p.inidisp=14;break;case 13:p.bgXsc[1]=0x7a;break;
      case 14:p.hScroll[1]++;break;case 15:p.cgadsub=1;break;
      case 16:p.windowsel=1;break;case 17:p.obsel=3;break;
      case 18:p.mosaic=1;break;case 19:p.screenEnabled[0]=15;break;
    }
    capture(19);assert(!s_packets[s_complete].valid);
    assert(!LagoonSceneRenderer_TryPresent(output,682*4,expected,682*4,682,224,19));
  }
}
static void additional_cave_vectors(void) {
  static const unsigned cases[][3]={{4,96,32},{5,192,160},{6,64,32}};
  for(unsigned k=0;k<sizeof(cases)/sizeof(cases[0]);++k) {
    cave_setup();ram[0x48e]=cases[k][0];ram[0x53]=cases[k][1];ram[0x55]=cases[k][2];
    LagoonSceneRenderer_Reset();s_mode=2;
    unsigned definitions=0;assert(scene_resources(ram,&definitions)&&definitions==0x31298);
    capture(21);ScenePacket *s=&s_packets[s_complete];assert(s->valid);
    assert(s->world_width==(int)cases[k][1]*8&&s->world_height==(int)cases[k][2]*8);
    /* A narrow room clips the actual world, including below its finite height;
     * the native center preserves its independently streamed circular tilemap. */
    render(s,682);
    for(unsigned y=0;y<224;++y)for(unsigned x=0;x<682;++x) {
      int qx=s->wx+(int)x-213,qy=s->wy+(int)y;
      uint32_t color=x>=213&&x<469?0xff0000:
        qx>=0&&qx<s->world_width&&qy>=0&&qy<s->world_height?0x00ff00:0;
      assert(s_pixels[y*682+x]==color);memcpy(expected+(y*682+x)*4,&color,4);
    }
    assert(LagoonSceneRenderer_TryPresent(output,682*4,expected,682*4,682,224,21));
    for(unsigned bad=0;bad<23;++bad) {
      cave_setup();ram[0x48e]=cases[k][0];ram[0x53]=cases[k][1];ram[0x55]=cases[k][2];
      LagoonSceneRenderer_Reset();s_mode=2;
      switch(bad) {
        case 0:ram[0x53]--;break;case 1:ram[0x53]++;break;
        case 2:ram[0x55]--;break;case 3:ram[0x55]++;break;
        case 4:ram[0x48f]=1;break;case 5:ram[0x48e]=7;break;
        case 6:ram[0x57]^=1;break;case 7:ram[0x5d]^=1;break;
        case 8:ram[0x69]^=1;break;case 9:ram[0x75]^=1;break;
        case 10:ram[0x88]^=1;break;case 11:ram[0x8b]^=1;break;
        case 12:ram[0x100e]=1;break;case 13:ram[0x50]=2;break;
        case 14:p.inidisp=14;break;case 15:p.bgXsc[1]=0x7a;break;
        case 16:p.hScroll[1]++;break;case 17:p.cgadsub=1;break;
        case 18:p.windowsel=1;break;case 19:p.obsel=3;break;
        case 20:p.mosaic=1;break;case 21:p.screenEnabled[0]=15;break;
        case 22:ram[0x53]=224;ram[0x55]=204;break;
      }
      capture(21);assert(!s_packets[s_complete].valid);
      assert(!LagoonSceneRenderer_TryPresent(output,682*4,expected,682*4,682,224,21));
    }
    cave_setup();ram[0x48e]=cases[k][0];ram[0x53]=cases[k][1];ram[0x55]=cases[k][2];
    LagoonSceneRenderer_Reset();s_mode=2;capture(21);
    assert(s_packets[s_complete].valid);
    assert(!LagoonSceneRenderer_TryPresent(output,682*4,expected,682*4,682,224,22));
  }
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
/* Synthetic source pixels exercise independent anchors/page wrapping and finite
 * clipping. These packets do not pretend to satisfy the owned-ROM source gate. */
static void parallax_pixels(void) {
  setup();LagoonSceneRenderer_Reset();s_mode=2;capture(22);
  ScenePacket *s=&s_packets[s_complete];assert(s->valid);s->parallax=1;
  s->bg2_x=251;s->bg2_y=247;
  for(unsigned y=0;y<8;++y)s->vram[16+y]=255;
  s->pal[17]=31<<5;
  for(unsigned n=0;n<2048;++n)s->vram[0x7800+n]=0x2401;
  const unsigned widths[]={256,342,682,800};
  const int positions[][2]={{0,0},{1023,0},{0,1000},{1023,1000}};
  for(unsigned k=0;k<4;++k)for(unsigned w=0;w<4;++w) {
    unsigned width=widths[w];int left=(int)(width-256)/2;
    s->wx=positions[k][0];s->wy=positions[k][1];s->serial=++s_serial;
    render(s,width);
    for(unsigned y=0;y<224;++y)for(unsigned x=0;x<width;++x) {
      int native=(int)x>=left&&(int)x<left+256;
      int qx=s->wx+(int)x-left,qy=s->wy+(int)y;
      uint32_t color=native||(qx>=0&&qx<1024&&qy>=0&&qy<1024)?0x00ff00:0;
      assert(s_pixels[y*width+x]==color);
    }
  }
  /* An independent scalar oracle uses varied flipped tiles and palettes in
   * both32x32 pages, at wraps255/256 and511/512 and both margin boundaries. */
  for(unsigned n=0;n<32768;++n)s->vram[n]=(n*7919+127)&65535;
  for(unsigned n=0;n<256;++n)s->pal[n]=(n*1237)&32767;
  for(unsigned phase=0;phase<4;++phase) {
    s->bg2_x=251+phase*3;s->bg2_y=247+phase*89;s->serial=++s_serial;
    for(unsigned w=0;w<4;++w)for(unsigned y=0;y<224;++y) {
      unsigned width=widths[w];int left=(int)(width-256)/2;
      uint8_t depth[800]={0};uint16_t colors[800]={0};
      background_row(s,width,y,left,1,depth,colors);
      for(unsigned x=0;x<width;++x) {
        unsigned qx=(unsigned)((int)s->bg2_x+(int)x-left),qy=s->bg2_y+y;
        unsigned tile=s->vram[0x7800+((qy/256)&1)*1024+((qy/8)&31)*32+((qx/8)&31)];
        unsigned c=scalar_graphics(s,tile,qx&7,qy&7,0,4),z=tile&0x2000?11:7;
        assert(depth[x]==(c?z:0));
        assert(colors[x]==(c?s->pal[((tile>>10)&7)*16+c]:0));
      }
    }
  }
  ScenePacket *other=&s_packets[s_complete^1];memcpy(other,s,sizeof(*s));
  assert(same_source(s,other));other->bg2_x++;assert(!same_source(s,other));other->bg2_x--;
  other->bg2_y++;assert(!same_source(s,other));other->bg2_y--;
  other->parallax=0;assert(!same_source(s,other));
}
static void room07_setup(const uint8_t *owned_rom,const uint16_t *owned_vram) {
  setup();memcpy(rom,owned_rom,sizeof(rom));memcpy(p.vram,owned_vram,sizeof(p.vram));
  ram[0x48e]=7;ram[0x50]=2;ram[0x53]=ram[0x55]=128;
  pointer(0x57,0x129e89);pointer(0x5d,0x1a906f);
  pointer(0x69,0x069720);pointer(0x75,0x069720);
  pointer(0x7e,0x11fb7d);pointer(0x81,0x11fb7d);
  p.bgXsc[1]=0x7a;
  ram[0x480]=128;ram[0x481]=1;ram[0x482]=0;ram[0x483]=2;
  ram[0x484]=96;ram[0x486]=128;
  p.hScroll[0]=384;p.vScroll[0]=512;p.hScroll[1]=96;p.vScroll[1]=128;
}
/* Optional actual assets stay external. Both page hashes and compressed source
 * must pass before positive production packet capture is asserted. */
static void room07_guards(const char *rom_path,const char *vram_path) {
  static uint8_t owned_rom[1048576];static uint16_t owned_vram[32768];
  FILE *f=fopen(rom_path,"rb");assert(f&&fread(owned_rom,1,sizeof(owned_rom),f)==sizeof(owned_rom));fclose(f);
  f=fopen(vram_path,"rb");assert(f&&fread(owned_vram,1,sizeof(owned_vram),f)==sizeof(owned_vram));fclose(f);
  room07_setup(owned_rom,owned_vram);LagoonSceneRenderer_Reset();s_mode=2;
  assert(supported(&p,ram,rom,sizeof(rom),1));capture(22);
  ScenePacket *s=&s_packets[s_complete];assert(s->valid&&s->parallax&&s->bg2_x==96&&s->bg2_y==129);
  render(s,682);memcpy(expected,s_pixels,682*224*4);
  assert(LagoonSceneRenderer_TryPresent(output,682*4,expected,682*4,682,224,22));
  assert(!LagoonSceneRenderer_TryPresent(output,682*4,expected,682*4,682,224,23));
  for(unsigned bad=0;bad<32;++bad) {
    room07_setup(owned_rom,owned_vram);LagoonSceneRenderer_Reset();s_mode=2;
    switch(bad) {
      case 0:ram[0x48f]=1;break;case 1:ram[0x53]--;break;case 2:ram[0x55]++;break;
      case 3:ram[0x57]^=1;break;case 4:ram[0x5d]^=1;break;
      case 5:ram[0x69]^=1;break;case 6:ram[0x75]^=1;break;
      case 7:ram[0x88]^=1;break;case 8:ram[0x8b]^=1;break;
      case 9:ram[0x7e]^=1;break;case 10:ram[0x81]^=1;break;
      case 11:ram[0x50]=0;break;case 12:ram[0x100e]=1;break;
      case 13:ram[0x484]++;break;case 14:ram[0x486]++;break;
      case 15:p.hScroll[1]++;break;case 16:p.vScroll[1]++;break;
      case 17:p.hScroll[0]=1024;break;case 18:p.bgXsc[0]^=1;break;
      case 19:p.bgXsc[1]=0x79;break;case 20:p.bgXsc[2]^=1;break;
      case 21:p.inidisp=14;break;case 22:p.cgadsub=1;break;
      case 23:p.windowsel=1;break;case 24:p.bgmode=1;break;
      case 25:p.obsel=3;break;case 26:p.mosaic=1;break;
      case 27:p.screenEnabled[0]=15;break;case 28:rom[0x8fb7d]^=1;break;
      case 29:p.vram[0x7800]^=1;break;case 30:p.vram[0x7fff]^=1;break;
      case 31:ram[0x48e]=8;break;
    }
    capture(22);assert(!s_packets[s_complete].valid);
  }
  /* Mid-raster source writes, anchor/register changes and state load remain
   * refusals, even if the first line was independently source-qualified. */
  for(unsigned bad=0;bad<4;++bad) {
    room07_setup(owned_rom,owned_vram);LagoonSceneRenderer_Reset();s_mode=2;
    for(unsigned line=1;line<=224;++line) {
      if(line==100){if(bad==0)p.hScroll[1]++;if(bad==1)p.vramWriteCount++;if(bad==2)p.vram[0x7800]^=1;}
      LagoonSceneRenderer_ObserveRaster(&p,line,ram,rom,sizeof(rom),bad==3&&line>100?23:22,1);
    }
    LagoonSceneRenderer_EndFrame(101,22,1);assert(!s_packets[s_complete].valid);
  }
  puts("PASS room07 actual compressed source/page guards:1positive,32entry refusals,4raster refusals,generation refusal");
}
static void room08_setup(void) {
  setup();ram[0x48e]=8;ram[0x53]=192;ram[0x55]=128;
  pointer(0x57,0x12b297);pointer(0x5d,0x1891e4);
  pointer(0x69,0x06999c);pointer(0x75,0x06999c);
}
static void room08_vectors(void) {
  room08_setup();LagoonSceneRenderer_Reset();s_mode=2;
  unsigned definitions=0;assert(scene_resources(ram,&definitions)&&definitions==0x3199c);
  /* Native streamed tile0 is red, owned world-definition tile1 green; the
   * backdrop is deliberately blue so clipping must explicitly produce black. */
  for(unsigned i=0;i<4;++i){rom[0x3199c+i*2]=1;rom[0x3199d+i*2]=4;}
  for(unsigned y=0;y<8;++y)p.vram[16+y]=255;
  p.cgram[0]=31<<10;p.cgram[17]=31<<5;capture(23);
  ScenePacket *s=&s_packets[s_complete];assert(s->valid&&s->finite_clip&&!s->parallax);
  assert(s->world_width==1536&&s->world_height==1024);
  const unsigned widths[]={256,342,682,800};
  const int positions[][2]={{0,0},{1535,0},{0,1000},{1535,1000}};
  for(unsigned k=0;k<4;++k)for(unsigned w=0;w<4;++w) {
    unsigned width=widths[w];int left=(int)(width-256)/2;
    s->wx=positions[k][0];s->wy=positions[k][1];s->serial=++s_serial;
    render(s,width);
    for(unsigned y=0;y<224;++y)for(unsigned x=0;x<width;++x) {
      int native=(int)x>=left&&(int)x<left+256;
      int qx=s->wx+(int)x-left,qy=s->wy+(int)y;
      uint32_t color=native?0xff0000:qx>=0&&qx<1536&&qy>=0&&qy<1024?0x00ff00:0;
      assert(s_pixels[y*width+x]==color);
    }
  }
  s->wx=0;s->wy=0;s->count=1;s->objects[0]=(SceneObject){.x=-8,.y=0,.width=1,.height=1,.pixels={0x83e0},.priority=3,.slot=0,.order=1};
  render(s,682);assert(!s_pixels[205]); /* Even authored OBJ outside the finite world is clipped. */
  ScenePacket *other=&s_packets[s_complete^1];memcpy(other,s,sizeof(*s));
  assert(same_source(s,other));other->finite_clip=0;assert(!same_source(s,other));
  for(unsigned bad=0;bad<32;++bad) {
    room08_setup();LagoonSceneRenderer_Reset();s_mode=2;
    switch(bad) {
      case 0:ram[0x48f]=1;break;case 1:ram[0x48e]=9;break;
      case 2:ram[0x53]--;break;case 3:ram[0x53]++;break;
      case 4:ram[0x55]--;break;case 5:ram[0x55]++;break;
      case 6:ram[0x57]^=1;break;case 7:ram[0x5d]^=1;break;
      case 8:ram[0x69]^=1;break;case 9:ram[0x75]^=1;break;
      case 10:ram[0x88]^=1;break;case 11:ram[0x8b]^=1;break;
      case 12:ram[0x100e]=1;break;case 13:ram[0x50]=2;break;
      case 14:p.inidisp=14;break;case 15:p.bgXsc[1]=0x7a;break;
      case 16:p.hScroll[1]++;break;case 17:p.vScroll[1]++;break;
      case 18:p.bgmode=1;break;case 19:p.bgTileAdr^=1;break;
      case 20:p.cgadsub=1;break;case 21:p.windowsel=1;break;
      case 22:p.obsel=3;break;case 23:p.mosaic=1;break;
      case 24:p.screenEnabled[0]=15;break;case 25:p.screenWindowed[0]=1;break;
      case 26:p.setini=1;break;case 27:p.hScroll[2]=1;break;
      case 28:p.vScroll[2]=1;break;case 29:p.oamaddh=0x80;break;
      case 30:p.cgwsel|=1;break;case 31:ram[0x53]=128;break;
    }
    capture(23);assert(!s_packets[s_complete].valid);
  }
  room08_setup();LagoonSceneRenderer_Reset();s_mode=2;capture(23);assert(s_packets[s_complete].valid);
  assert(!LagoonSceneRenderer_TryPresent(output,682*4,expected,682*4,682,224,24));
  for(unsigned bad=0;bad<5;++bad) {
    room08_setup();LagoonSceneRenderer_Reset();s_mode=2;
    for(unsigned line=1;line<=224;++line) {
      if(line==100){if(bad==0)p.hScroll[1]++;if(bad==1)p.vramWriteCount++;
        if(bad==2)p.cgram[1]^=1;if(bad==3)p.oamWriteCount++;}
      LagoonSceneRenderer_ObserveRaster(&p,line,ram,rom,sizeof(rom),bad==4&&line>100?24:23,1);
    }
    LagoonSceneRenderer_EndFrame(101,23,1);assert(!s_packets[s_complete].valid);
  }
}
static void read_owned(const char *path,void *bytes,size_t n) {
  FILE *f=fopen(path,"rb");assert(f&&fread(bytes,1,n,f)==n);assert(fgetc(f)==EOF);fclose(f);
}
/* Actual original-loader RAM/VRAM and ROM remain external, never embedded.
 * Scalar rows independently decode the captured source, not final framebuffer pixels. */
static unsigned room08_source_tile(const ScenePacket *s,unsigned layer,int x,int y) {
  unsigned meta=x<0||y<0||x>=2048||y>=2048?ram[0x52]:s->maps[layer][(y/16)*128+x/16];
  unsigned tile=s->defs[layer][meta*4+((x&8)?1:0)+((y&8)?2:0)];
  return layer?tile&0xdfff:tile;
}
static void room08_shadow_zero(void) {
  const int phases[]={-3,-2,-1,0,1,2,-1,-3,0};
  unsigned long checks=0;unsigned old_wrong=0,native_row=0;
  for(unsigned k=0;k<sizeof(phases)/sizeof(phases[0]);++k) {
    int phase=phases[k],bias=phase<0?2048:0;
    p.hScroll[0]=p.hScroll[1]=384;p.vScroll[0]=p.vScroll[1]=phase&1023;
    ram[0x480]=128;ram[0x481]=1;ram[0x482]=ram[0x483]=0;
    ram[0x484]=128;ram[0x485]=1;ram[0x486]=ram[0x487]=0;
    capture(23);ScenePacket *s=&s_packets[s_complete];assert(s->valid&&s->wx==384&&s->wy==phase+1);
    if(k==0||k==7)WsShadowReset(); /* Both reset and existing-key phase changes. */
    for(unsigned layer=0;layer<2;++layer) {
      WsShadowSetWorld(layer,384,phase+bias);WsShadowSetScroll(layer,384,phase&1023);
    }
    WsShadowFrame(&p);
    for(unsigned layer=0;layer<2;++layer) {
      int world_x=(int)WsShadowWorldX(layer),world_y=(int)WsShadowWorldY(layer);
      /* Same exact key/source distinction as the title's audited08 path. */
      int x0=(world_x-213-8)/8,x1=(world_x+256+213+8)/8;
      for(int y=world_y/8;y<=(world_y+232)/8&&y<512;++y)
        for(int x=x0<0?0:x0;x<=x1&&x<4096;++x)
          WsShadowForceTile(layer,x,y,room08_source_tile(s,layer,x*8,y*8-bias));
      for(unsigned y=0;y<224;++y)for(int x=-213;x<469;++x) {
        if(x>=0&&x<256)continue;
        int physical_y=phase+(int)y+1;
        if(physical_y<0)continue; /* The independent finite mask blacks these pixels. */
        unsigned expected_tile=room08_source_tile(s,layer,384+x,physical_y);
        unsigned wrapped_y=(p.vScroll[layer]+y+1)&1023;
        unsigned got=WsShadowTile(layer,x,wrapped_y,p.hScroll[layer],0,0xdead);
        assert(got==expected_tile);
        assert(scalar_graphics(s,got,(384+x)&7,physical_y&7,0,4)==
          scalar_graphics(s,expected_tile,(384+x)&7,physical_y&7,0,4));
        ++checks;
        if(phase<0&&room08_source_tile(s,layer,384+x,(phase&1023)+(int)y+1)!=expected_tile)++old_wrong;
      }
      if(phase==-1&&k==2)for(unsigned x=384;x<640;x+=8) {
        unsigned real=s->vram[0x7000+layer*0x800+((x/8)&31)+((x&256)?0x400:0)];
        assert(real==room08_source_tile(s,layer,x,0));++native_row;
      }
    }
  }
  assert(old_wrong>0&&native_row==64);WsShadowReset();
  printf("PASS actual WsShadow API negative08 Y/key-bias/phase/reset: %lu margin pixels,64native row0 tiles; old raw-scroll policy differs at %u source samples\n",checks,old_wrong);
}
static void room08_owned(const char *rom_path,const char *ram_path,const char *vram_path) {
  room08_setup();read_owned(rom_path,rom,sizeof(rom));read_owned(ram_path,ram,sizeof(ram));
  read_owned(vram_path,p.vram,sizeof(p.vram));
  assert(word(ram,0x48e)==8&&word(ram,0x53)==192&&word(ram,0x55)==128);
  p.hScroll[0]=p.hScroll[1]=word(ram,0x480);p.vScroll[0]=p.vScroll[1]=word(ram,0x482);
  for(unsigned n=0;n<256;++n)p.cgram[n]=(n*1237)&32767;
  LagoonSceneRenderer_Reset();s_mode=2;capture(23);ScenePacket *s=&s_packets[s_complete];assert(s->valid&&s->finite_clip);
  const unsigned widths[]={256,342,682,800};
  for(unsigned w=0;w<4;++w)for(unsigned y=0;y<224;++y) {
    unsigned width=widths[w];int left=(int)(width-256)/2;
    uint8_t depth[800]={0};uint16_t colors[800];for(unsigned x=0;x<width;++x)colors[x]=s->pal[0];
    background_row(s,width,y,left,0,depth,colors);background_row(s,width,y,left,1,depth,colors);
    for(unsigned x=0;x<width;++x) {
      unsigned ez=0,ec=s->pal[0];
      for(unsigned l=0;l<2;++l) {
        int native=(int)x>=left&&(int)x<left+256;
        int qx=(native?(int)s->native_x:s->wx)+(int)x-left,qy=(native?(int)s->native_y:s->wy)+(int)y;
        unsigned tile;
        if(native)tile=s->vram[(0x7000+l*0x800+((qy/8)&31)*32+((qx/8)&31)+((qx&256)?0x400:0))&32767];
        else {
          if(qx<0||qy<0||qx>=1536||qy>=1024)continue;
          unsigned meta=s->maps[l][(qy/16)*128+qx/16];tile=s->defs[l][meta*4+((qx&8)?1:0)+((qy&8)?2:0)];if(l)tile&=0xdfff;
        }
        unsigned c=scalar_graphics(s,tile,qx&7,qy&7,0,4),z=(tile&0x2000?12:8)-l;
        if(c&&z>ez){ez=z;ec=s->pal[((tile>>10)&7)*16+c];}
      }
      assert(depth[x]==ez&&colors[x]==ec);
    }
  }
  room08_shadow_zero();
  puts("PASS room08 actual ROM/loader RAM/VRAM scalar oracle independent pixels465920; synthetic edges1863680,32entry/5rasterrefusals,generation");
}
int main(int argc,char **argv) {
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
  cave_vectors();additional_cave_vectors();cache_oracle();parallax_pixels();room08_vectors();
  assert(argc==1||argc==3||argc==5);if(argc==3)room07_guards(argv[1],argv[2]);
  if(argc==5){assert(!strcmp(argv[1],"room08"));room08_owned(argv[2],argv[3],argv[4]);}
  puts("PASS independent planar rendering, immutable cached packets, full-word/pitch gate, verify/auto, reset and raster refusal");return 0;
}
