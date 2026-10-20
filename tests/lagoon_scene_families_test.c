/* Private external assets only. Independent source sampling and complete
 * original animation protocols; no game pixels or states are embedded. */
#define main LagoonLegacySceneTestMain
#include "lagoon_scene_renderer_test.c"
#undef main
static unsigned long pixel_checks,edge_checks,resource_refusals,animation_refusals;
static void file_path(char *out,size_t n,const char *base,const char *prefix,unsigned room,const char *tag,const char *suffix) {
  assert(snprintf(out,n,"%s/%s%02x/dumps/%s.%s",base,prefix,room,tag,suffix)>0);
}
static void registers(const char *path) {
  char b[16384];FILE*f=fopen(path,"rb");assert(f);size_t n=fread(b,1,sizeof(b)-1,f);assert(!ferror(f)&&feof(f));fclose(f);b[n]=0;
  unsigned x[4],y[4];char *a=strstr(b,"\"hscroll\""),*c=strstr(b,"\"vscroll\"");assert(a&&c);
  assert(sscanf(strchr(a,'['),"[%u, %u, %u, %u]",x,x+1,x+2,x+3)==4);
  assert(sscanf(strchr(c,'['),"[%u, %u, %u, %u]",y,y+1,y+2,y+3)==4);
  for(unsigned i=0;i<4;++i){p.hScroll[i]=x[i];p.vScroll[i]=y[i];}
}
static void refuse_resources(void) {
  static const unsigned bytes[]={0x48e,0x48f,0x50,0x53,0x54,0x55,0x56,
    0x57,0x58,0x59,0x5d,0x5e,0x5f,0x69,0x6a,0x6b,0x75,0x76,0x77,0x88,0x89,0x8a,0x8b,0x8c,0x8d};
  for(unsigned i=0;i<sizeof(bytes)/sizeof(bytes[0]);++i) {
    unsigned mask=bytes[i]==0x48e?0x80:1;
    ram[bytes[i]]^=mask;assert(!LagoonSceneRenderer_AdditionalRoom(ram));
    assert(!scene_resources(ram,NULL));ram[bytes[i]]^=mask;++resource_refusals;
  }
}
static void scalar_rows(ScenePacket *s) {
  const unsigned widths[]={256,342,682,800};
  for(unsigned w=0;w<4;++w)for(unsigned y=0;y<224;++y) {
    unsigned width=widths[w];int left=(int)(width-256)/2;
    uint8_t depth[800]={0};uint16_t color[800];for(unsigned x=0;x<width;++x)color[x]=s->pal[0];
    background_row(s,width,y,left,0,depth,color);background_row(s,width,y,left,1,depth,color);
    for(unsigned x=0;x<width;++x) {
      unsigned ez=0,ec=s->pal[0];
      for(unsigned l=0;l<2;++l) {
        int native=(int)x>=left&&(int)x<left+256;
        int qx=(native?(int)s->native_x:s->wx)+(int)x-left,qy=(native?(int)s->native_y:s->wy)+(int)y;
        unsigned tile;
        if(native)tile=s->vram[(0x7000+l*0x800+((qy/8)&31)*32+((qx/8)&31)+((qx&256)?0x400:0))&32767];
        else {
          if(qx<0||qy<0||qx>=s->world_width||qy>=s->world_height)continue;
          unsigned meta=s->maps[l][(qy/16)*128+qx/16];tile=s->defs[l][meta*4+((qx&8)?1:0)+((qy&8)?2:0)];if(l)tile&=0xdfff;
        }
        unsigned c=scalar_graphics(s,tile,qx&7,qy&7,0,4),z=(tile&0x2000?12:8)-l;
        if(c&&z>ez){ez=z;ec=s->pal[((tile>>10)&7)*16+c];}
      }
      assert(depth[x]==ez&&color[x]==ec);++pixel_checks;
    }
  }
}
static void finite_edges(ScenePacket *s) {
  ScenePacket copy=*s;s=&copy;s->count=0;
  memset(s->vram,0,sizeof(s->vram));memset(s->maps,0,sizeof(s->maps));
  memset(s->defs,0,sizeof(s->defs));memset(s->high,0,sizeof(s->high));
  for(unsigned i=0;i<128;++i)((uint8_t*)s->oam)[i*4+1]=240;
  for(unsigned y=0;y<8;++y)s->vram[y]=255;
  s->pal[0]=31<<10;s->pal[1]=31;s->native_x=s->native_y=128;
  const int positions[][2]={{0,0},{s->world_width-1,0},{0,s->world_height-24},{s->world_width-1,s->world_height-24}};
  for(unsigned k=0;k<4;++k) {
    s->wx=positions[k][0];s->wy=positions[k][1];s->serial=++s_serial;render(s,682);
    for(unsigned y=0;y<224;++y)for(unsigned x=0;x<682;++x) {
      int native=x>=213&&x<469,qx=s->wx+(int)x-213,qy=s->wy+(int)y;
      uint32_t color=native||(qx>=0&&qx<s->world_width&&qy>=0&&qy<s->world_height)?0xff0000:0;
      assert(s_pixels[y*682+x]==color);++edge_checks;
    }
  }
}
static void transfer(LagoonWideTorches *t,unsigned room,unsigned frame,const uint8_t bytes[64]) {
  assert(LagoonWideTorches_Begin(t,24,room,0x15c815,0x10,0xbb00+frame*64,0x3b00,64));
  for(unsigned i=0;i<64;++i)LagoonWideTorches_Write(t,0x7600+i,bytes[i],rom,sizeof(rom));
}
static unsigned animation_snapshot(unsigned room) {
  LagoonWideTorches t;unsigned matches=0,phase=0;
  for(unsigned frame=0;frame<4;++frame) {
    uint8_t bytes[64];memcpy(bytes,rom+0x83b00+frame*64,64);LagoonWideTorches_Reset(&t);transfer(&t,room,frame,bytes);
    assert(t.seen==(1u<<frame));int m=LagoonWideTorches_Match(&t,p.vram,24,room,0x15c815);
    if(m>=0){assert((unsigned)m==frame);++matches;phase=frame;}
  }
  assert(matches==1);return phase;
}
static void animation_protocol(unsigned room) {
  LagoonWideTorches t;uint16_t vr[32768]={0};
  for(unsigned frame=0;frame<4;++frame) {
    uint8_t bytes[64];memcpy(bytes,rom+0x83b00+frame*64,64);
    LagoonWideTorches_Reset(&t);assert(LagoonWideTorches_Begin(&t,24,room,0x15c815,0x10,0xbb00+frame*64,0x3b00,64));
    memcpy((uint8_t*)vr+0x7600,bytes,64);
    for(unsigned n=0;n<64;++n) {
      assert(LagoonWideTorches_Match(&t,vr,24,room,0x15c815)<0);
      LagoonWideTorches_Write(&t,0x7600+n,bytes[n],rom,sizeof(rom));
    }
    assert(LagoonWideTorches_Match(&t,vr,24,room,0x15c815)==(int)frame);
    for(unsigned n=0;n<64;++n) {
      ((uint8_t*)vr)[0x7600+n]^=1;assert(LagoonWideTorches_Match(&t,vr,24,room,0x15c815)<0);((uint8_t*)vr)[0x7600+n]^=1;++animation_refusals;
    }
    assert(LagoonWideTorches_Match(&t,vr,25,room,0x15c815)<0);++animation_refusals;
    for(unsigned n=0;n<84;++n) {
      unsigned a=n<8?0xe058+n:n<20?0xdd8b+n-8:0x83b00+frame*64+n-20;
      rom[a]^=1;LagoonWideTorches_Reset(&t);transfer(&t,room,frame,bytes);
      assert(!t.seen&&LagoonWideTorches_Match(&t,vr,24,room,0x15c815)<0);rom[a]^=1;++animation_refusals;
    }
    LagoonWideTorches_Reset(&t);assert(LagoonWideTorches_Begin(&t,24,room,0x15c815,0x10,0xbb00+frame*64,0x3b00,64));
    LagoonWideTorches_Write(&t,0x7601,bytes[0],rom,sizeof(rom));assert(!t.left&&!t.seen);++animation_refusals;
  }
  for(unsigned bad=0;bad<8;++bad) {
    LagoonWideTorches_Reset(&t);unsigned bank=0x10,source=0x15c815,at=0xbb00,to=0x3b00,len=64;
    if(bad==0)bank++;if(bad==1)source++;if(bad==2)at++;if(bad==3)at=0xbc00;
    if(bad==4)to++;if(bad==5)len--;if(bad==6)len++;if(bad==7)at=0xb7c0;
    assert(!LagoonWideTorches_Begin(&t,24,room,source,bank,at,to,len));++animation_refusals;
  }
}
int main(int argc,char **argv) {
  int town=argc==5 && !strcmp(argv[4],"--town-only");
  assert(argc==4 || town);const char *tags[]={"entered","sample-03","sample-07","sample-11"};unsigned animation_matches=0;
  for(unsigned room=town?0x19:0x2c;room<=(town?0x1c:0x37);++room) {
    unsigned phases=0;
    for(unsigned tag=0;tag<(town?1:4);++tag) {
      char path[2048];setup();read_owned(argv[1],rom,sizeof(rom));
      const char *name=town?"sample-11":tags[tag];
      file_path(path,sizeof(path),argv[2],argv[3],room,name,"wram.bin");read_owned(path,ram,sizeof(ram));
      file_path(path,sizeof(path),argv[2],argv[3],room,name,"vram.bin");read_owned(path,p.vram,sizeof(p.vram));
      file_path(path,sizeof(path),argv[2],argv[3],room,name,"cgram.bin");read_owned(path,p.cgram,sizeof(p.cgram));
      file_path(path,sizeof(path),argv[2],argv[3],room,name,"regs.json");registers(path);
      assert(word(ram,0x48e)==room&&LagoonSceneRenderer_AdditionalRoom(ram));
      LagoonSceneRenderer_Reset();s_mode=2;capture(24);ScenePacket *s=&s_packets[s_complete];assert(s->valid&&s->finite_clip&&!s->parallax);
      scalar_rows(s);if(!tag){finite_edges(s);refuse_resources();}
      if(!town && room>=0x35){phases|=1u<<animation_snapshot(room);++animation_matches;}
      if(!tag) {
        assert(!LagoonSceneRenderer_TryPresent(output,682*4,expected,682*4,682,224,25));
        for(unsigned bad=0;bad<8;++bad) {
          Ppu saved=p;int authority=1;
          if(bad==0)p.inidisp=14;if(bad==1)p.bgXsc[1]=0x7a;if(bad==2)p.hScroll[1]++;
          if(bad==3)p.cgadsub=1;if(bad==4)p.mosaic=1;if(bad==5)p.screenWindowed[0]=1;
          if(bad==6)p.extraObjectCount=513;if(bad==7)authority=0;
          assert(!supported(&p,ram,rom,sizeof(rom),authority));p=saved;++resource_refusals;
        }
      }
    }
    if(!town && room>=0x35){assert(phases==15);animation_protocol(room);}
  }
  if(town) {
    printf("PASS4exactNPCrooms/4actual loader snapshots; %lu independent source pixels; %lu finite-edge pixels; %lu resource/layout/authority refusals;4generation refusals; no loading/fade authority added\n",pixel_checks,edge_checks,resource_refusals);
    return 0;
  }
  printf("PASS12exactrooms/48loader snapshots; %lu independent source pixels; %lu finite-edge pixels; %lu resource/layout/authority refusals;12generation refusals; %u actual animation phase matches; %lu animation protocol refusals\n",pixel_checks,edge_checks,resource_refusals,animation_matches,animation_refusals);
  return 0;
}
