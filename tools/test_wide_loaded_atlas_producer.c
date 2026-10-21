/* Actual loaded-room resident data with the real producer. No guest writes
 * by the rebind or producer; independent far-pixel lookup verifies output. */
#define LAGOON_WIDE_ACTOR_POLICY_NO_MAIN
#include "test_wide_additional_producer.c"
static uint8_t ram_before[0x20000];
static uint16_t vram_before[32768],colors_before[256];
static void input_prefix(const char*prefix,const char*suffix,void*out,size_t size){char file[4096];snprintf(file,sizeof file,"%s.%s",prefix,suffix);FILE*f=fopen(file,"rb");assert(f&&fread(out,1,size,f)==size);fclose(f);}
static unsigned scalar(void) {
  unsigned pixels=0,matched=0;
  /* These are the two actual Elf Field actors newly visible after this load.
   * Their source animation state, rather than stale guest frame bytes, selects
   * the frame. All plane/palette/flip lookup below is independent of Pixels(). */
  const unsigned actors[]={0x600,0x680};
  for(unsigned e=0;e<2;++e) {
    unsigned off=actors[e],index=off/64,a=0x500+off;uint8_t frame;
    assert(LagoonWideEntities_FrameForContext(g_rom,sizeof own_rom,s_frame.ram,
      s_frame.ram+a,&s_pose[index].animation,0,&frame));
    LagoonWideCell cells[64];int n=decode_entity(s_frame.ram,off,frame,1,cells);assert(n>0);
    for(int k=0;k<n;++k) {
      const LagoonWideCell*c=&cells[k];const PpuExtraObject*o=NULL;
      for(unsigned i=0;i<p.extraObjectCount;++i) {
        const PpuExtraObject*candidate=&p.extraObjects[i];
        if(candidate->x==c->x&&candidate->y==c->y&&candidate->width==c->size&&
            candidate->priority==((c->tile>>12)&3)&&candidate->math==!!(c->tile&0x800)&&
            candidate->order>1&&(candidate->order-2)%kMultipartCells==(unsigned)k) {
          assert(!o);o=candidate;
        }
      }
      assert(o);++matched;
      for(unsigned y=0;y<c->size;++y)for(unsigned x=0;x<c->size;++x) {
        unsigned sx=c->tile&0x4000?c->size-1-x:x,sy=c->tile&0x8000?c->size-1-y:y;
        unsigned tile=(((c->tile&255)/16+sy/8)*16)|(((c->tile&15)+sx/8)&15);
        unsigned addr=((c->tile&256?0x5000:0x4000)+tile*16+(sy&7))&32767,color=0;
        for(unsigned plane=0;plane<4;++plane)
          color|=((p.vram[(addr+(plane/2)*8)&32767]>>((plane&1)*8+7-(sx&7)))&1)<<plane;
        unsigned value=color?0x8000|p.cgram[128+((c->tile>>9)&7)*16+color]:0;
        int screen=o->x+(int)x;
        if(screen>=-16&&screen<272)value=0;
        assert(o->pixels[y*o->stride+x]==value);++pixels;
      }
    }
  }
  assert(matched==s_far_emitted);return pixels;
}
int main(int argc,char**argv){assert(argc==3);FILE*f=fopen(argv[1],"rb");assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);
 ppu_reset(&p);input_prefix(argv[2],"wram.bin",g_ram,sizeof g_ram);input_prefix(argv[2],"vram.bin",p.vram,sizeof p.vram);input_prefix(argv[2],"cgram.bin",p.cgram,sizeof p.cgram);
 assert(ram_word(g_ram,0x48e)==8&&!g_ram[0x100e]);p.inidisp=15;p.obsel=2;p.bgmode=9;p.bgXsc[0]=0x71;p.bgXsc[1]=0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
 memcpy(p.oam,g_ram+0x2000,512);memcpy(p.highOam,g_ram+0x2200,32);g_config.widescreen=1;PpuSetExtraSpace(&p,213);
 LagoonWideEntities_Reset();LagoonWideEntities_Init();capture();LagoonWideEntities_BeginFrame(1);assert(!s_atlas.ready&&!s_has_far);unsigned old=(unsigned)p.extraObjectCount;
 memcpy(ram_before,g_ram,sizeof g_ram);memcpy(vram_before,p.vram,sizeof p.vram);memcpy(colors_before,p.cgram,sizeof p.cgram);
 LagoonWideEntities_RebindLoaded();assert(s_atlas.ready&&s_atlas.generation==generation);LagoonWideEntities_BeginFrame(1);
 assert(s_has_far&&s_far_emitted&&p.extraObjectCount>old);unsigned pixels=scalar(),far=(unsigned)s_far_emitted;
 assert(!memcmp(ram_before,g_ram,sizeof g_ram)&&!memcmp(vram_before,p.vram,sizeof p.vram)&&!memcmp(colors_before,p.cgram,sizeof p.cgram));
 /* A cold generation/reset never invokes resident rebind implicitly. */
 generation++;LagoonWideEntities_Reset();capture();LagoonWideEntities_BeginFrame(1);assert(!s_atlas.ready&&!s_has_far);
 printf("loaded actor producer: actual room08 complete resident snapshot, %u added farcells/%u independent planar pixels; cold-reset/no-load guard and full RAM/VRAM/palette unchanged PASS\n",far,pixels);return 0;
}
