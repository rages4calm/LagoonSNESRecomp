/* Actual source-created room09 NPC47 RAM/VRAM/palette, original C026 sort/draw, then modeled
 * completed OAM DMA. Captured CPU and OAM phases differ, so current snapshot OAM
 * is not asserted coherent. No enemy pose, world position or art is synthesized.
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
static void original_far_npc(void) {
  uint8_t before[sizeof g_ram];memcpy(before,g_ram,sizeof before);
  LagoonWideCell cells[64];int n=decode_entity(g_ram,0x80,g_ram[0x59c],0,cells);
  assert(n>0);g_ram[0x236]=0;g_ram[0x2a2]=255;
  Interp816 *c=interp816_init(NULL,original_read,original_write);assert(c);
  c->pc=0xc1c6;c->k=c->db=1;c->dp=0;c->sp=0x1d0;c->a=c->y=0;c->x=0x80;
  c->mf=1;c->xf=0;c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;
  g_ram[0x1d1]=0xef;g_ram[0x1d2]=0x7f;unsigned steps=0;
  while(c->k!=1||c->pc!=0x7ff0){assert(++steps<100000);interp816_runOpcode(c);}
  assert(g_ram[0x236]==n);
  for(int i=0;i<n;++i) {
    const LagoonWideCell *cell=&cells[i];unsigned h=(g_ram[0x2200+i/4]>>((i&3)*2))&3;
    assert((g_ram[0x2000+i*4]|(h&1)<<8)==((unsigned)cell->x&511));
    assert(g_ram[0x2001+i*4]==((unsigned)cell->y&255));
    assert(ram_word(g_ram,0x2002+i*4)==cell->tile);
    assert((h&2?16:8)==cell->size);
  }
  interp816_free(c);memcpy(g_ram,before,sizeof before);
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
  assert(ram_word(g_ram,0x48e)==9&&!g_ram[0x100e]);
  assert(g_ram[0x598]==1&&g_ram[0x59b]==47&&g_ram[0x59c]==0&&g_ram[0x580]==0xc0);
  assert(ram_word(g_ram,0x582)==784&&ram_word(g_ram,0x584)==912);
  p.inidisp=15;p.obsel=2;p.bgmode=9;p.bgXsc[0]=0x71;
  p.bgXsc[1]=g_ram[0x50]?0x7a:0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
  original_far_npc();original_draw();
  memcpy(p.oam,g_ram+0x2000,512);memcpy(p.highOam,g_ram+0x2200,32);
  g_config.widescreen=1;PpuSetExtraSpace(&p,213);
  memcpy(original_ram,g_ram,sizeof g_ram);memcpy(original_vram,p.vram,sizeof p.vram);
  memcpy(original_cgram,p.cgram,sizeof p.cgram);
  LagoonWideEntities_Reset();LagoonWideEntities_Init();capture();
  LagoonWideEntities_BeginFrame(1);assert(!s_atlas.ready&&!s_has_far);
  LagoonWideEntities_RebindLoaded();assert(s_atlas.ready&&s_atlas.completed==kLagoonWideAtlasBytes);
  for(unsigned i=0;i<kLagoonWideAtlasBytes;++i)assert(s_atlas.owned[i]);
  LagoonWideEntities_BeginFrame(1);assert(s_has_far&&s_far_emitted);
  uint8_t frame;assert(LagoonWideEntities_CurrentFrame(g_rom,kRomSize,s_frame.ram,g_ram,0x80,&frame));
  assert(frame==0);LagoonWideCell cells[64];int n=decode_entity(s_frame.ram,0x80,frame,1,cells);
  assert(n>0);unsigned checked=0,pixels=0,opaque=0;
  for(int k=0;k<n;++k) {
    const LagoonWideCell *c=&cells[k];const PpuExtraObject *o=NULL;
    assert(LagoonWideAtlas_CellOwned(&s_atlas,0x4000,0x5000,c));
    for(unsigned i=0;i<p.extraObjectCount;++i) {
      const PpuExtraObject *candidate=&p.extraObjects[i];
      if(candidate->x==c->x&&candidate->y==c->y&&candidate->width==c->size&&
          candidate->priority==((c->tile>>12)&3)&&candidate->math==!!(c->tile&0x800)&&
          candidate->order>1&&(candidate->order-2)%kMultipartCells==(unsigned)k) {
        assert(!o);o=candidate;
      }
    }
    assert(o);++checked;
    for(unsigned y=0;y<c->size;++y)for(unsigned x=0;x<c->size;++x) {
      unsigned sx=c->tile&0x4000?c->size-1-x:x,sy=c->tile&0x8000?c->size-1-y:y;
      unsigned tile=(((c->tile&255)/16+sy/8)*16)|(((c->tile&15)+sx/8)&15);
      unsigned addr=((c->tile&256?0x5000:0x4000)+tile*16+(sy&7))&32767,color=0;
      for(unsigned plane=0;plane<4;++plane)
        color|=((p.vram[(addr+(plane/2)*8)&32767]>>((plane&1)*8+7-(sx&7)))&1)<<plane;
      unsigned value=color?0x8000|p.cgram[128+((c->tile>>9)&7)*16+color]:0;
      int screen=c->x+(int)x;if(screen>=-16&&screen<272)value=0;
      assert(o->pixels[y*o->stride+x]==value);++pixels;
      opaque+=value!=0&&screen>=-213&&screen<469;
    }
  }
  assert(checked==s_far_emitted&&opaque);
  assert(!memcmp(original_ram,g_ram,sizeof g_ram));
  assert(!memcmp(original_vram,p.vram,sizeof p.vram));
  assert(!memcmp(original_cgram,p.cgram,sizeof p.cgram));
  p.vram[0x5442]^=1;LagoonWideEntities_BeginFrame(1);assert(s_has_far);p.vram[0x5442]^=1;
  LagoonWideCell tail={-180,30,0x21b0,8};assert(!LagoonWideAtlas_CellOwned(&s_atlas,0x4000,0x5000,&tail));
  p.oam[0]^=1;LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);p.oam[0]^=1;
  ++generation;LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);--generation;
  LagoonWideEntities_BeginFrame(1);assert(s_has_far);
  unsigned base=cells[0].tile&256?0x5000:0x4000;
  unsigned address=((base+(cells[0].tile&255)*16)&32767)*2;
  observe_vram_byte(address,((uint8_t*)p.vram)[address]);
  assert(!p.extraObjectCount);LagoonWideEntities_BeginFrame(1);assert(!s_has_far);
  ++generation;LagoonWideEntities_Reset();capture();LagoonWideEntities_BeginFrame(1);
  assert(!s_atlas.ready&&!s_has_far);
  printf("loaded common3 producer: originalC1C6 farNPC47 cells and originalC026 native packet; original-created NPC with disclosed valid-worldXY assist, %u added far cells/%u independent planar values/%u visible opaque margin pixels; whole10752 ordinary atlas only, common3 remainder unowned, guest unchanged, OAM/generation/write/cold guards PASS\n",checked,pixels,opaque);
  return 0;
}
