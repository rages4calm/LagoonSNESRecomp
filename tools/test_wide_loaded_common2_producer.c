/* Real Voloh snapshot and production observer/producer. Guest actor placements
 * are unchanged; only the successful-load host ownership call is applied. */
#define LAGOON_WIDE_ACTOR_POLICY_NO_MAIN
#include "test_wide_additional_producer.c"
static uint8_t original_ram[0x20000];
static uint16_t original_vram[32768],original_cgram[256];
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
  p.inidisp=15;p.obsel=2;p.bgmode=9;p.bgXsc[0]=0x71;
  p.bgXsc[1]=0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
  memcpy(p.oam,g_ram+0x2000,512);memcpy(p.highOam,g_ram+0x2200,32);
  g_config.widescreen=1;PpuSetExtraSpace(&p,213);
  memcpy(original_ram,g_ram,sizeof g_ram);memcpy(original_vram,p.vram,sizeof p.vram);
  memcpy(original_cgram,p.cgram,sizeof p.cgram);
  LagoonWideEntities_Reset();LagoonWideEntities_Init();capture();
  LagoonWideEntities_BeginFrame(1);assert(!s_atlas.ready&&!s_has_far);
  LagoonWideEntities_RebindLoaded();assert(s_atlas.ready);
  LagoonWideEntities_BeginFrame(1);assert(s_has_far&&s_far_emitted);
  uint8_t frame;assert(LagoonWideEntities_CurrentFrame(g_rom,sizeof own_rom,
    s_frame.ram,g_ram,0x300,&frame));
  assert(frame==42&&g_ram[0x818]==1&&g_ram[0x81b]==54);
  LagoonWideCell cells[64];int n=decode_entity(s_frame.ram,0x300,frame,1,cells);
  assert(n>0);unsigned checked=0,pixels=0;
  for(int k=0;k<n;++k) {
    const LagoonWideCell *c=&cells[k];const PpuExtraObject *o=NULL;
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
    }
  }
  assert(checked==s_far_emitted);
  assert(!memcmp(original_ram,g_ram,sizeof g_ram));
  assert(!memcmp(original_vram,p.vram,sizeof p.vram));
  assert(!memcmp(original_cgram,p.cgram,sizeof p.cgram));
  p.oam[0]^=1;LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);
  p.oam[0]^=1;generation++;LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);
  LagoonWideEntities_Reset();capture();LagoonWideEntities_BeginFrame(1);
  assert(!s_atlas.ready&&!s_has_far);
  printf("loaded common2 producer: actual NPC54 frame42, %u added far cells/%u independent planar pixels; RAM/VRAM/palette unchanged, whole-OAM/generation/cold-reset refusals PASS\n",checked,pixels);
  return 0;
}
