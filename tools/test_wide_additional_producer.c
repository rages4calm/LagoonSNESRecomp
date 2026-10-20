/* Production OAM/atlas observer and far composition policy, using actual ROM
 * metasprites and actual loader-resident VRAM/CGRAM. Actor/OAM placement in
 * this unit fixture is synthetic; real spawned actors are checked separately
 * by the assisted off/auto runtime routes. This never certifies gameplay. */
#define main ppu_extra_object_fixture_main
#include "../snesrecomp/tests/ppu/ppu_extra_object_test.c"
#undef main
#include "../src/lagoon_wide_entities.c"
Config g_config;
static uint8_t own_rom[0x100000];
const uint8_t *g_rom=own_rom;
uint8_t g_ram[0x20000];
Ppu *g_ppu=&p;
static uint64_t generation=101;
uint64_t RtlStateGeneration(void){return generation;}
static DmaStartObserver observer;
void dma_set_start_observer(DmaStartObserver o){observer=o;}
void LagoonWide_OnBackgroundDma(unsigned a,unsigned b,unsigned c,unsigned d){(void)a;(void)b;(void)c;(void)d;}
void LagoonWide_OnVramWrite(unsigned a){(void)a;}
void LagoonWide_OnParallaxQueueDma(unsigned a){(void)a;}
static unsigned refusals,native_cells,far_cells,center_pixels;
static unsigned actor_type=2;
static const char *baseline_root;
static void put(unsigned a,unsigned value){g_ram[a]=(uint8_t)value;g_ram[a+1]=(uint8_t)(value>>8);}
static void read_dump(unsigned room,const char *suffix,void *data,size_t size) {
  char path[4096];snprintf(path,sizeof(path),"%s/baseline-%02x/dumps/sample-11.%s",baseline_root,room,suffix);
  FILE *f=fopen(path,"rb");assert(f&&fread(data,1,size,f)==size);fclose(f);
}
static void capture(void) {
  Dma dma={0};DmaChannel *c=&dma.channel[0];c->bAdr=4;c->aBank=0x7e;c->aAdr=0x2000;c->size=0x220;
  observer(&dma,0,c);assert(s_frame.valid);
}
static void oam_for_record(int screen_x,int screen_y,int queued) {
  put(0x54c,(unsigned)screen_x);put(0x54e,(unsigned)screen_y);
  unsigned cx=ram_word(g_ram,0x480),cy=ram_word(g_ram,0x482);
  put(0x542,cx+screen_x);put(0x544,cy+screen_y);
  g_ram[0x540]=queued?0xc8:0xc0;put(0x48c,queued?1:0);
  put(0xec0,0x40);g_ram[0xec4]=(uint8_t)actor_type;
  memset(g_ram+0x2000,0,544);
  for(unsigned slot=0;slot<128;++slot) {
    g_ram[0x2000+slot*4]=64;g_ram[0x2200+slot/4]|=(uint8_t)(1u<<((slot%4)*2));
  }
  LagoonWideCell cells[64];int count=decode_entity(g_ram,0x40,g_ram[0x55c],0,cells);assert(count>0);
  if(queued)for(int i=0;i<count;++i) {
    unsigned a=0x2000+i*4,shift=(i%4)*2;
    g_ram[a]=(uint8_t)cells[i].x;g_ram[a+1]=(uint8_t)cells[i].y;
    put(a+2,cells[i].tile);g_ram[0x2200+i/4]&=(uint8_t)~(3u<<shift);
    g_ram[0x2200+i/4]|=(uint8_t)((((cells[i].x>>8)&1)|(cells[i].size==16?2:0))<<shift);
  }
  g_ram[0x236]=queued?(uint8_t)count:0;
  memcpy(p.oam,g_ram+0x2000,512);memcpy(p.highOam,g_ram+0x2200,32);
  capture();
}
static void atlas(void) {
  /* Replay exact real loader protocol through production observer + every
   * actual resident-byte write. These are policy fixtures, not live DMA proof. */
  p.vramIncrement=1;p.vramIncrementOnHigh=1;p.vramRemapMode=0;
  for(unsigned done=0;done<kLagoonWideAtlasBytes;) {
    Dma dma={0};DmaChannel *c=&dma.channel[3];c->dmaActive=1;
    c->mode=1;c->bAdr=0x18;c->aBank=0x7f;c->aAdr=(uint16_t)(0x7f80+done);
    c->size=(uint16_t)(kLagoonWideAtlasBytes-done);if(c->size>0x1000)c->size=0x1000;
    p.vramPointer=(uint16_t)(0x4400+done/2);observer(&dma,3,c);
    for(unsigned b=0;b<c->size;++b)observe_vram_byte(0x8800+done+b,((uint8_t*)p.vram)[0x8800+done+b]);
    done+=c->size;
  }
  assert(s_atlas.ready&&s_atlas.completed==kLagoonWideAtlasBytes);
}
static void fixture(unsigned room,unsigned kind) {
  memset(&p,0,sizeof(p));ppu_reset(&p);p.inidisp=15;p.obsel=2;p.bgmode=9;
  read_dump(room,"wram.bin",g_ram,sizeof(g_ram));read_dump(room,"vram.bin",p.vram,sizeof(p.vram));
  read_dump(room,"cgram.bin",p.cgram,sizeof(p.cgram));
  p.bgXsc[0]=0x71;p.bgXsc[1]=g_ram[0x50]?0x7a:0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
  memset(g_ram+0x540,0,0x800);g_ram[0x540]=0xc8;g_ram[0x558]=(uint8_t)actor_type;g_ram[0x55b]=(uint8_t)kind;
  LagoonWideAnimation a={0};assert(LagoonWideEntities_FrameForRoom(g_rom,sizeof(own_rom),room,g_ram+0x540,&a,0,g_ram+0x55c));
  g_config.widescreen=1;LagoonWideEntities_Reset();LagoonWideEntities_Init();atlas();
  PpuSetExtraSpace(&p,213);oam_for_record(100,80,1);LagoonWideEntities_BeginFrame(1);
  assert(s_kind_witness[actor_type-1][kind] && !p.extraObjectCount);
}
static void far(void) {
  oam_for_record(-140,80,0);LagoonWideEntities_BeginFrame(1);
  assert(p.extraObjectCount>0 && s_has_far);far_cells+=(unsigned)p.extraObjectCount;
  for(unsigned i=0;i<p.extraObjectCount;++i) {
    const PpuExtraObject *o=&p.extraObjects[i];assert(o->x+o->width<=-16);
    for(unsigned y=0;y<o->height;++y)for(unsigned x=0;x<o->width;++x)
      if(o->x+(int)x>=0 && o->x+(int)x<256) {assert(!o->pixels[y*o->stride+x]);++center_pixels;}
  }
}
static void reject(void) {LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);++refusals;}
static void negatives(void) {
  fixture(0x2c,31);far();
  /* Both current and captured exact room resource contexts are mandatory. */
  const unsigned fields[]={0x48e,0x48f,0x50,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
    0x5d,0x5e,0x5f,0x69,0x6a,0x6b,0x75,0x76,0x77,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x100e};
  for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0]);++i)for(unsigned captured=0;captured<2;++captured) {
    unsigned address=fields[i];uint8_t *r=captured?s_frame.ram:g_ram;r[address]^=1;
    reject();r[address]^=1;
  }
  s_frame.ram[0x48d]=1;reject();s_frame.ram[0x48d]=0;
  s_frame.ram[0x48c]=65;reject();s_frame.ram[0x48c]=0;
  ++generation;reject();--generation;
  p.inidisp=14;reject();p.inidisp=15;p.obsel=3;reject();p.obsel=2;
  p.oamaddh=0x80;reject();p.oamaddh=0;p.oam[0]^=1;reject();p.oam[0]^=1;
  unsigned source=0x8000+(0xab5e&32767);own_rom[source]^=1;reject();own_rom[source]^=1;
  s_atlas.ready=0;reject();s_atlas.ready=1;++s_atlas.generation;reject();--s_atlas.generation;
  s_atlas.source^=1;reject();s_atlas.source^=1;s_atlas.room^=1;reject();s_atlas.room^=1;
  s_frame.ram[0x541]=4;reject();s_frame.ram[0x541]=0;
  s_kind_witness[1][31]=0;reject();
  fixture(0x2c,31);far();
  LagoonWideCell emitted[64];int cells=decode_entity(s_frame.ram,0x40,s_frame.ram[0x55c],1,emitted);
  assert(cells>0);
  for(int i=0;i<cells;++i)for(unsigned y=0;y<emitted[i].size;y+=8)
    for(unsigned x=0;x<emitted[i].size;x+=8) {
      unsigned tile=((((emitted[i].tile&255)>>4)+y/8)<<4)|(((emitted[i].tile&15)+x/8)&15);
      unsigned base=emitted[i].tile&0x100?0x5000:0x4000,address=(base+tile*16)*2;
      for(unsigned byte=0;byte<32;++byte) {
        s_atlas.owned[address+byte-0x8800]=0;reject();s_atlas.owned[address+byte-0x8800]=1;
      }
    }
  LagoonWideEntities_BeginFrame(1);assert(p.extraObjectCount>0);
  unsigned tile=emitted[0].tile,base=tile&0x100?0x5000:0x4000;
  unsigned address=(base+(tile&255)*16)*2;
  observe_vram_byte(address,0x5c);assert(!p.extraObjectCount&&!s_has_far);++refusals;
  /* A later malformed native projection revokes an earlier kind witness. */
  fixture(0x2c,31);oam_for_record(100,80,1);
  put(0x542,ram_word(g_ram,0x542)+1);capture();LagoonWideEntities_BeginFrame(1);
  assert(!s_kind_witness[1][31]);++refusals;
  oam_for_record(-140,80,0);reject();
  /* Full actual OAM metadata owns position, palette, flips, size and priority. */
  for(unsigned field=0;field<5;++field) {
    fixture(0x2c,31);oam_for_record(250,80,1);
    if(field<4) {((uint8_t*)p.oam)[field]^=1;s_frame.oam[field]^=1;}
    else {p.highOam[0]^=2;s_frame.oam[512]^=2;}
    reject();
  }
  const unsigned attribute_masks[]={2,4,8,16,32,64,128};
  for(unsigned i=0;i<sizeof(attribute_masks)/sizeof(attribute_masks[0]);++i) {
    fixture(0x2c,31);oam_for_record(250,80,1);
    ((uint8_t*)p.oam)[3]^=(uint8_t)attribute_masks[i];s_frame.oam[3]^=(uint8_t)attribute_masks[i];
    reject();
  }
}
#ifndef LAGOON_WIDE_ACTOR_POLICY_NO_MAIN
int main(int argc,char **argv) {
  assert(argc==3);baseline_root=argv[2];FILE *f=fopen(argv[1],"rb");
  assert(f&&fread(own_rom,1,sizeof(own_rom),f)==sizeof(own_rom));fclose(f);
  const unsigned cases[][2]={{0x2c,31},{0x2c,32},{0x2c,33},{0x2f,25},{0x2f,27},{0x35,28}};
  for(unsigned n=0;n<6;++n) {
    fixture(cases[n][0],cases[n][1]);far();
    oam_for_record(250,80,1);LagoonWideEntities_BeginFrame(1);assert(p.extraObjectCount>0);
    native_cells+=(unsigned)p.extraObjectCount;
    for(unsigned i=0;i<p.extraObjectCount;++i) {
      const PpuExtraObject *o=&p.extraObjects[i];assert(o->order==1);
      for(unsigned y=0;y<o->height;++y)for(unsigned x=0;x<o->width;++x)
        if(o->x+(int)x>=0 && o->x+(int)x<256) {assert(!o->pixels[y*o->stride+x]);++center_pixels;}
    }
  }
  negatives();printf("additional production actor policy: six ordinary kind cases in three actual ROM/loader atlas families, %u synthetic far cells/%u original-slot seam cells, %u zero native-center pixels, %u context/source/OAM/atlas/phase refusals PASS\n",far_cells,native_cells,center_pixels,refusals);
  return 0;
}
#endif
