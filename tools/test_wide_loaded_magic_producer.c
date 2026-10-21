/* Actual real-R homing RAM/VRAM/palette, original C026 sort/draw, then modeled
 * completed OAM DMA. Captured CPU and OAM phases differ, so current snapshot OAM
 * is not asserted coherent. No actor/projectile pose or art is synthesized.
 * Extension immutability and live timing/center evidence are separate checks. */
#define LAGOON_WIDE_ACTOR_POLICY_NO_MAIN
#include "test_wide_additional_producer.c"
#include "snes/interp816.h"
static uint8_t original_ram[0x20000];
static uint16_t original_vram[32768],original_cgram[256];
static unsigned product;
static uint8_t original_read(void *ctx,uint32_t address) {
  (void)ctx;unsigned b=address>>16,a=address&65535;
  if(b==126||b==127)return g_ram[(b-126)*65536+a];
  if((b&127)<64&&a<8192)return g_ram[a];
  if((b&127)<64&&(a==0x4216||a==0x4217))return product>>((a-0x4216)*8);
  if((b&127)<64&&(a==0x4214||a==0x4215)) {
    unsigned v=g_ram[0x4206]?ram_word(g_ram,0x4204)/g_ram[0x4206]:65535;
    return v>>((a-0x4214)*8);
  }
  assert((b&127)<32&&a>=32768);return own_rom[(b&31)*32768+(a&32767)];
}
static void original_write(void *ctx,uint32_t address,uint8_t value) {
  (void)ctx;unsigned b=address>>16,a=address&65535;
  if(b==126||b==127){g_ram[(b-126)*65536+a]=value;return;}
  if((b&127)<64&&(a==0x4202||a==0x4203)){g_ram[a]=value;product=g_ram[0x4202]*g_ram[0x4203];return;}
  if((b&127)<64&&a>=0x4204&&a<=0x4206){g_ram[a]=value;return;}
  assert((b&127)<64&&a<8192);g_ram[a]=value;
}
static void original_draw(void) {
  g_ram[0x236]=0;g_ram[0x2a2]=255;
  Interp816 *c=interp816_init(NULL,original_read,original_write);assert(c);
  c->pc=0xc026;c->k=c->db=1;c->dp=0;c->sp=0x1d0;c->a=c->x=c->y=0;
  c->mf=1;c->xf=0;c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;
  g_ram[0x1d1]=0xef;g_ram[0x1d2]=0x7f;unsigned steps=0;
  while(c->k!=1||c->pc!=0x7ff0){assert(++steps<100000);interp816_runOpcode(c);}
  assert(c->sp==0x1d2);interp816_free(c);
}
static void read_prefix(const char *prefix,const char *suffix,void *out,size_t n) {
  char name[4096];snprintf(name,sizeof name,"%s.%s",prefix,suffix);
  FILE *f=fopen(name,"rb");assert(f&&fread(out,1,n,f)==n);fclose(f);
}
int main(int argc,char **argv) {
  assert(argc==3);FILE *f=fopen(argv[1],"rb");
  assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);
  ppu_reset(&p);read_prefix(argv[2],"wram.bin",g_ram,sizeof g_ram);
  read_prefix(argv[2],"vram.bin",p.vram,sizeof p.vram);
  read_prefix(argv[2],"cgram.bin",p.cgram,sizeof p.cgram);
  assert(ram_word(g_ram,0x48e)==0x14&&!g_ram[0x100e]&&g_ram[0x537]==13);
  original_draw();
  p.inidisp=15;p.obsel=2;p.bgmode=9;p.bgXsc[0]=0x71;
  p.bgXsc[1]=g_ram[0x50]?0x7a:0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
  memcpy(p.oam,g_ram+0x2000,512);memcpy(p.highOam,g_ram+0x2200,32);
  g_config.widescreen=1;PpuSetExtraSpace(&p,213);
  memcpy(original_ram,g_ram,sizeof g_ram);memcpy(original_vram,p.vram,sizeof p.vram);
  memcpy(original_cgram,p.cgram,sizeof p.cgram);
  LagoonWideEntities_Reset();LagoonWideEntities_Init();capture();
  LagoonWideEntities_BeginFrame(1);assert(!s_magic_atlas.ready&&!s_has_magic);
  unsigned before=(unsigned)p.extraObjectCount;
  LagoonWideEntities_RebindLoaded();assert(s_magic_atlas.ready&&!s_atlas.ready);
  LagoonWideEntities_BeginFrame(1);
  if(!s_has_magic) {
    fprintf(stderr,"magic diagnostic: objects%u before%u rejected%u validated%u ready%u match%d\n",
      (unsigned)p.extraObjectCount,before,(unsigned)s_rejected,(unsigned)s_validated,s_magic_atlas.ready,
      LagoonWideMagic_Match(&s_magic_atlas,p.vram,g_rom,kRomSize,generation,0x14,13));
    for(unsigned q=0;q<s_frame.ram[0x48c];++q)if(s_frame.ram[0xec4+q*6]==4) {
      LagoonWideCell diagnostic[64];int n=LagoonWideEntities_MagicPacketCells(g_rom,kRomSize,s_frame.ram,q,diagnostic,64);
      fprintf(stderr,"queue%u off%u n%d",q,ram_word(s_frame.ram,0xec0+q*6),n);
      uint8_t used[128]={0};for(int i=0;i<n;++i)fprintf(stderr," cell%d:%d,%d,%x owned%d slot%d",i,diagnostic[i].x,diagnostic[i].y,diagnostic[i].tile,
        LagoonWideMagic_CellOwned(&s_magic_atlas,0x4000,0x5000,diagnostic+i),find_cell(s_frame.oam,diagnostic+i,used));
      fprintf(stderr,"\n");
    }
  }
  assert(s_has_magic&&p.extraObjectCount>before);
  LagoonWideCell cells[1024];int count=0;unsigned real=0;
  for(unsigned queue=0;queue<s_frame.ram[0x48c];++queue) {
    unsigned q=0xec0+queue*6,offset=ram_word(s_frame.ram,q);
    if(s_frame.ram[q+4]!=4||offset>=0x180||offset%24)continue;
    if(s_frame.ram[0xd40+offset]!=8)continue;
    int n=LagoonWideEntities_MagicPacketCells(g_rom,kRomSize,s_frame.ram,queue,cells+count,1024-count);
    assert(n>0&&!s_frame.ram[0xd51+offset]);count+=n;++real;
  }
  assert(real);unsigned checked=0,pixels=0,opaque=0,seam=0;
  for(unsigned i=before;i<p.extraObjectCount;++i) {
    const PpuExtraObject *o=&p.extraObjects[i];const LagoonWideCell *c=NULL;
    for(int k=0;k<count;++k)if(cells[k].x==o->x&&cells[k].y==o->y&&cells[k].size==o->width&&
        o->priority==((cells[k].tile>>12)&3)&&o->math==!!(cells[k].tile&0x800)) {c=&cells[k];break;}
    assert(c&&o->order==1);++checked;seam+=c->x<256&&c->x+(int)c->size>256;
    for(unsigned y=0;y<c->size;++y)for(unsigned x=0;x<c->size;++x) {
      unsigned sx=c->tile&0x4000?c->size-1-x:x,sy=c->tile&0x8000?c->size-1-y:y;
      unsigned tile=(((c->tile&255)/16+sy/8)*16)|(((c->tile&15)+sx/8)&15);
      unsigned addr=((c->tile&256?0x5000:0x4000)+tile*16+(sy&7))&32767,color=0;
      for(unsigned plane=0;plane<4;++plane)
        color|=((p.vram[(addr+(plane/2)*8)&32767]>>((plane&1)*8+7-(sx&7)))&1)<<plane;
      unsigned value=color?0x8000|p.cgram[128+((c->tile>>9)&7)*16+color]:0;
      int screen=c->x+(int)x;if(screen>=0&&screen<256)value=0;
      assert(o->pixels[y*o->stride+x]==value);++pixels;opaque+=value!=0;
    }
  }
  assert(checked&&opaque&&seam&&checked==s_magic_emitted);
  assert(!memcmp(original_ram,g_ram,sizeof g_ram));
  assert(!memcmp(original_vram,p.vram,sizeof p.vram));
  assert(!memcmp(original_cgram,p.cgram,sizeof p.cgram));
  p.vram[0x4400]^=1;LagoonWideEntities_BeginFrame(1);
  assert(!s_atlas.ready&&s_has_magic&&p.extraObjectCount);p.vram[0x4400]^=1;
  p.vram[0x5c00]^=1;LagoonWideEntities_BeginFrame(1);assert(!s_has_magic&&!p.extraObjectCount);p.vram[0x5c00]^=1;
  LagoonWideCell mixed=cells[0];mixed.tile=0x2088;
  assert(!LagoonWideMagic_CellOwned(&s_magic_atlas,0x4000,0x5000,&mixed));
  uint8_t used[128]={0};int slot=find_cell(s_frame.oam,cells,used);assert(slot>=0);
  ((uint8_t*)p.oam)[slot*4+2]^=1;s_frame.oam[slot*4+2]^=1;
  LagoonWideEntities_BeginFrame(1);assert(!s_has_magic&&!p.extraObjectCount);
  ((uint8_t*)p.oam)[slot*4+2]^=1;s_frame.oam[slot*4+2]^=1;
  p.oam[0]^=1;LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);p.oam[0]^=1;
  ++generation;LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);--generation;
  LagoonWideEntities_BeginFrame(1);assert(p.extraObjectCount);
  unsigned spell=s_frame.ram[0x537];s_frame.ram[0x537]=8;LagoonWideEntities_BeginFrame(1);assert(!s_has_magic);s_frame.ram[0x537]=spell;
  LagoonWideEntities_BeginFrame(1);assert(p.extraObjectCount);
  observe_vram_byte(0xb800,((uint8_t*)p.vram)[0xb800]);
  assert(!s_magic_atlas.ready&&!p.extraObjectCount);
  LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);
  ++generation;LagoonWideEntities_Reset();capture();LagoonWideEntities_BeginFrame(1);
  assert(!s_magic_atlas.ready&&!s_has_magic);
  s_magic_atlas.length=1;LagoonWideEntities_RebindLoaded();assert(!s_magic_atlas.ready);
  printf("loaded magic producer: originalC026 from %u actual kind8 homing packets, %u added cells/%u crossing seam/%u independent planar values/%u opaque margin pixels; extension leaves guest unchanged; normalatlas corruption independent, mixedtile/partialpacket/whole-OAM/generation/form/magicbyte/write/cold/partialDMA refusals PASS\n",
      real,checked,seam,pixels,opaque);return 0;
}
