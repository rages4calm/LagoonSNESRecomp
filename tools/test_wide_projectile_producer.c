/* Production observer/PPU and one actual room14 loader. The arranged records
 * and OAM are synthetic policy tests; the assisted live fixture is separate. */
#define LAGOON_WIDE_ACTOR_POLICY_NO_MAIN
#include "test_wide_additional_producer.c"
static unsigned setups,refused_cases,scalar_pixels,center_zero,order_cases;
static void load_fixture(const char *suffix,void *target,size_t size) {
  char path[4096];snprintf(path,sizeof path,"%s.%s",baseline_root,suffix);
  FILE *f=fopen(path,"rb");assert(f&&fread(target,1,size,f)==size);fclose(f);
}
static void empty_oam(void) {
  memset(g_ram+0x2000,0,544);
  for(unsigned i=0;i<128;++i){g_ram[0x2000+i*4]=64;g_ram[0x2200+i/4]|=1u<<((i%4)*2);}
  g_ram[0x236]=0;
}
static void cell_oam(const LagoonWideCell *c) {
  unsigned slot=g_ram[0x236]++,at=0x2000+slot*4,shift=(slot%4)*2;
  g_ram[at]=c->x;g_ram[at+1]=c->y;put(at+2,c->tile);
  g_ram[0x2200+slot/4]&=~(3u<<shift);
  g_ram[0x2200+slot/4]|=(((c->x>>8)&1)|(c->size==16?2:0))<<shift;
}
static void projectile(unsigned j,unsigned kind,unsigned frame,int x,int y) {
  unsigned at=0xd40+j*24;g_ram[at]=kind;g_ram[at+15]=frame;g_ram[at+17]=1;
  put(at+1,ram_word(g_ram,0x480)+x);put(at+3,ram_word(g_ram,0x482)+y);
  put(at+9,x);put(at+11,y);
  put(0xec0+j*6,j*24);put(0xec2+j*6,y);put(0xec4+j*6,4);
}
static void finish(void) {
  memcpy(p.oam,g_ram+0x2000,512);memcpy(p.highOam,g_ram+0x2200,32);
  capture();LagoonWideEntities_BeginFrame(1);
}
static void projectile_setup(int mixed,int y0,int y1) {
  memset(&p,0,sizeof p);ppu_reset(&p);
  load_fixture("wram.bin",g_ram,sizeof g_ram);load_fixture("vram.bin",p.vram,sizeof p.vram);
  load_fixture("cgram.bin",p.cgram,sizeof p.cgram);assert(ram_word(g_ram,0x48e)==0x14);
  p.inidisp=15;p.obsel=2;p.bgmode=9;p.bgXsc[0]=0x71;p.bgXsc[1]=g_ram[0x50]?0x7a:0x79;
  p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
  memset(g_ram+0x540,0,0x980);empty_oam();g_config.widescreen=1;
  LagoonWideEntities_Reset();LagoonWideEntities_Init();atlas();PpuSetExtraSpace(&p,213);
  projectile(0,13,0xdd,351,y0);projectile(1,11,0x12,280,y1);put(0x48c,2);
  if(y1>y0){uint8_t q[6];memcpy(q,g_ram+0xec0,6);memcpy(g_ram+0xec0,g_ram+0xec6,6);memcpy(g_ram+0xec6,q,6);}
  for(unsigned q=0;q<2;++q) {
    LagoonWideCell cells[64];int n=LagoonWideEntities_ProjectileCells(g_rom,sizeof own_rom,g_ram,g_ram,q,cells,64);assert(n>0);
    for(int i=0;i<n;++i)if(cells[i].y>=-16&&cells[i].y<240)cell_oam(&cells[i]);
  }
  if(mixed){unsigned at=0x540;g_ram[at]=0xc0;g_ram[at+1]=4;g_ram[at+24]=2;g_ram[at+27]=13;g_ram[at+28]=0;
    put(at+2,ram_word(g_ram,0x480)+360);put(at+4,ram_word(g_ram,0x482)+100);put(at+12,360);put(at+14,100);}
  finish();assert(p.extraObjectCount>0&&s_has_projectile);if(mixed)assert(s_has_far);++setups;
}
static void scalar(void) {
  for(unsigned j=0;j<p.extraObjectCount;++j) {
    const PpuExtraObject *o=&p.extraObjects[j];unsigned slot=o->oamSlot,hi=(p.highOam[slot/4]>>((slot%4)*2))&3;
    unsigned tile=((uint8_t*)p.oam)[slot*4+2]|((uint8_t*)p.oam)[slot*4+3]<<8;
    int native_x=((uint8_t*)p.oam)[slot*4]|((hi&1)<<8);if(native_x>=384)native_x-=512;
    /* Virtual ordinary cells have no native projectile anchor. */
    if(native_x!=o->x)continue;
    unsigned size=hi&2?16:8;assert(size==o->width);
    for(unsigned y=0;y<o->height;++y)for(unsigned x=0;x<o->width;++x) {
      unsigned sx=tile&0x4000?size-1-x:x,sy=tile&0x8000?size-1-y:y;
      unsigned t=(((tile&255)/16+sy/8)*16)|(((tile&15)+sx/8)&15);
      unsigned at=((tile&256?0x5000:0x4000)+t*16+(sy&7))&32767,color=0;
      for(unsigned plane=0;plane<4;++plane)color|=((p.vram[(at+(plane/2)*8)&32767]>>((plane&1)*8+7-(sx&7)))&1)<<plane;
      unsigned expected=color?(0x8000|p.cgram[128+((tile>>9)&7)*16+color]):0;
      int screen=o->x+(int)x;
      if(screen>=0&&screen<256)expected=0;
      if(o->pixels[y*o->stride+x]!=expected){fprintf(stderr,"scalar xy%d,%d order%u far%d slot%u tile%04x bit%u,%u actual%04x expected%04x\n",o->x,o->y,o->order,s_has_far,slot,tile,x,y,o->pixels[y*o->stride+x],expected);assert(0);}++scalar_pixels;
    }
  }
  LagoonWideCell edge={250,80,0x188c,8};uint16_t pixels[256];
  LagoonWideEntities_Pixels(p.vram,p.cgram,0x4000,0x5000,&edge,pixels,1);
  for(unsigned y=0;y<8;++y)for(unsigned x=0;x<6;++x){assert(!pixels[y*8+x]);++center_zero;}
}
static void rejected(void){LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);++refused_cases;}
#ifndef LAGOON_WIDE_PROJECTILE_POLICY_NO_MAIN
int main(int argc,char **argv) {
  assert(argc==3);baseline_root=argv[2];FILE *f=fopen(argv[1],"rb");
  assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);
  projectile_setup(0,100,100);scalar();assert(!s_has_far);projectile_setup(1,100,100);scalar();
  for(unsigned j=0;j<p.extraObjectCount;++j){const PpuExtraObject *o=&p.extraObjects[j];
    /* EqualY source projectile offsets0/24 occupy ranks0/1 before the ordinary
     * group. The second starts at independent ordinal1*128+2. */
    if(o->x<320&&o->x>256)assert(o->order==130);if(o->x>=340&&o->x<352)assert(o->order==2);}
  ++order_cases;projectile_setup(1,80,120);scalar();++order_cases;projectile_setup(1,120,80);scalar();++order_cases;
  /* The next CPU frame can advance/recycle all projectile bytes while the
   * unchanged completed native OAM packet still owns this displayed pose. */
  for(unsigned at=0;at<24;++at){projectile_setup(0,100,100);g_ram[0xd40+at]^=1;g_ram[0xd58+at]^=1;
    LagoonWideEntities_BeginFrame(1);assert(p.extraObjectCount>0&&s_has_projectile);scalar();}
  projectile_setup(0,100,100);g_ram[0xec4]^=1;g_ram[0xeca]^=1;
  LagoonWideEntities_BeginFrame(1);assert(p.extraObjectCount>0&&s_has_projectile);scalar();
  /* Invalid captured source/frame authority is still refused. */
  projectile_setup(0,100,100);s_frame.ram[0xd40]=s_frame.ram[0xd58]=1;rejected();
  projectile_setup(0,100,100);s_frame.ram[0xd4f]=s_frame.ram[0xd67]=255;rejected();
  projectile_setup(0,100,100);g_ram[0x480]^=1;rejected();
  projectile_setup(0,100,100);p.oam[0]^=1;rejected();
  projectile_setup(0,100,100);++s_frame.generation;rejected();
  projectile_setup(0,100,100);++s_atlas.generation;rejected();
  projectile_setup(0,100,100);s_atlas.ready=0;rejected();
  projectile_setup(0,100,100);p.inidisp=14;rejected();
  projectile_setup(0,100,100);p.bgmode=0;rejected();
  projectile_setup(0,100,100);g_ram[0x100e]=1;rejected();
  projectile_setup(0,100,100);s_frame.oam[2]^=1;rejected();
  projectile_setup(0,100,100);uint8_t q[6];memcpy(q,g_ram+0xec0,6);memcpy(g_ram+0xec0,g_ram+0xec6,6);memcpy(g_ram+0xec6,q,6);capture();rejected();
  projectile_setup(1,100,100);LagoonWideEntities_RetainScene();assert(LagoonWideEntities_RetainedSceneValid());
  g_ram[0xd40]^=1;assert(!LagoonWideEntities_RetainedSceneValid());++refused_cases;
  projectile_setup(0,100,100);observe_vram_byte(0x8800,((uint8_t*)p.vram)[0x8800]);assert(!p.extraObjectCount&&!s_has_projectile);++refused_cases;
  projectile_setup(0,100,100);Dma dma={0};DmaChannel *channel=&dma.channel[3];
  channel->dmaActive=1;channel->mode=1;channel->bAdr=0x18;channel->aBank=127;channel->aAdr=0x7f80;channel->size=0x1000;
  p.vramPointer=0x4400;observer(&dma,3,channel);assert(!p.extraObjectCount&&!s_has_projectile&&!s_atlas.ready);++refused_cases;
  projectile_setup(1,100,100);for(unsigned i=0;i<256;++i)p.cgram[i]^=(i*71)&32767;
  LagoonWideEntities_BeginFrame(1);assert(p.extraObjectCount>0);scalar();
  printf("projectile producer: %u synthetic setups, %u full-record/queue/OAM/owner/generation/order/layout/retained refusals, %u independent planar nativeOAM pixels/%u native-center zeros, %u combined y-order cases PASS\n",setups,refused_cases,scalar_pixels,center_zero,order_cases);
  return 0;
}

#endif
