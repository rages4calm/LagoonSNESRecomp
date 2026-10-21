/* Actual real-kill RAM/VRAM/palette and staged original OAM bytes. Model one
 * completed OAM DMA boundary; live timing/center evidence is a separate route.
 * No actor, pose or guest pixel bytes are synthesized or changed. */
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
  assert(ram_word(g_ram,0x48e)==0x14&&!g_ram[0x100e]);
  assert(g_ram[0x618]==2&&g_ram[0x61b]==13&&g_ram[0x601]&1);
  assert(!ram_word(g_ram,0x620)&&g_ram[0x61c]>=4&&g_ram[0x61c]<=11);
  p.inidisp=15;p.obsel=2;p.bgmode=9;p.bgXsc[0]=0x71;
  p.bgXsc[1]=g_ram[0x50]?0x7a:0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
  memcpy(p.oam,g_ram+0x2000,512);memcpy(p.highOam,g_ram+0x2200,32);
  g_config.widescreen=1;PpuSetExtraSpace(&p,213);
  memcpy(original_ram,g_ram,sizeof g_ram);memcpy(original_vram,p.vram,sizeof p.vram);
  memcpy(original_cgram,p.cgram,sizeof p.cgram);
  LagoonWideEntities_Reset();LagoonWideEntities_Init();capture();
  LagoonWideEntities_BeginFrame(1);assert(!s_death_atlas.ready&&!s_has_death);
  unsigned before=(unsigned)p.extraObjectCount;
  LagoonWideEntities_RebindLoaded();assert(s_death_atlas.ready&&!s_atlas.ready);
  LagoonWideEntities_BeginFrame(1);assert(s_has_death&&p.extraObjectCount>before);
  unsigned queue=0;
  while(queue<g_ram[0x48c]&&ram_word(g_ram,0xec0+queue*6)!=0x100)++queue;
  assert(queue<g_ram[0x48c]);LagoonWideCell cells[64];
  int n=decode_queued_entity(s_frame.ram,0x100,s_frame.ram[0x61c],cells,queue);assert(n>0);
  unsigned checked=0,pixels=0,opaque=0;
  for(unsigned i=before;i<p.extraObjectCount;++i) {
    const PpuExtraObject *o=&p.extraObjects[i];const LagoonWideCell *c=NULL;
    for(int k=0;k<n;++k)if(cells[k].x==o->x&&cells[k].y==o->y&&cells[k].size==o->width&&
        o->priority==((cells[k].tile>>12)&3)&&o->math==!!(cells[k].tile&0x800)) {
      assert(!c);c=&cells[k];
    }
    assert(c&&o->order==1);++checked;
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
  assert(checked&&opaque&&checked==s_death_emitted);
  assert(!memcmp(original_ram,g_ram,sizeof g_ram));
  assert(!memcmp(original_vram,p.vram,sizeof p.vram));
  assert(!memcmp(original_cgram,p.cgram,sizeof p.cgram));
  p.oam[0]^=1;LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);p.oam[0]^=1;
  ++generation;LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);--generation;
  LagoonWideEntities_BeginFrame(1);assert(p.extraObjectCount);
  observe_vram_byte(0x8600,((uint8_t*)p.vram)[0x8600]);
  assert(!s_death_atlas.ready&&!p.extraObjectCount);
  LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);
  generation++;LagoonWideEntities_Reset();capture();LagoonWideEntities_BeginFrame(1);
  assert(!s_death_atlas.ready&&!s_has_death);
  s_death_atlas.completed=1;LagoonWideEntities_RebindLoaded();assert(!s_death_atlas.ready);
  printf("loaded death producer: real slain NPC100 frame%u source OAM packet, %u added seam cells/%u independent planar pixels/%u opaque margin pixels; guest arrays unchanged, whole-OAM/generation/unknown-write/cold-reset/partial refusals PASS\n",
      g_ram[0x61c],checked,pixels,opaque);
  return 0;
}
