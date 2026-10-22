/* External original Atland fixture; real original A4D3/A7B5 and C026/C44E.
 * CPU/PPU timing arrangements below are finite source-policy cases. Actual
 * moving input and reference/native-center checks live in the batch receipt. */
#define LAGOON_WIDE_MULTIPART_PRODUCER_NO_MAIN
#include "test_wide_multipart_producer.c"
CpuState g_cpu;
static uint8_t start_ram[sizeof g_ram],unchanged[sizeof g_ram];
static uint16_t start_vram[32768],start_pal[256];
static unsigned policy_refusals,prop_pixels,original_cells,original_projection;
static unsigned prop=0x1c0;
static void cpu_base(Interp816 *c,unsigned pc,unsigned x){
  c->pc=pc;c->k=c->db=1;c->dp=0;c->sp=0x1d0;c->x=x;c->a=c->y=0;
  c->mf=1;c->xf=0;c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;
}
static void sync_cpu(const Interp816 *c){
  memset(&g_cpu,0,sizeof g_cpu);g_cpu.ram=g_ram;g_cpu.PB=c->k;g_cpu.DB=c->db;
  g_cpu.D=c->dp;g_cpu.S=c->sp;g_cpu.m_flag=c->mf;g_cpu.x_flag=c->xf;
  g_cpu.emulation=c->e;g_cpu._flag_D=c->d;g_cpu.P=c->mf?32:0;g_cpu.master_cycles=1000;
}
static void step_to(Interp816*c,unsigned pc){
  unsigned n=0;while(c->k!=1||c->pc!=pc){assert(++n<500000);interp816_runOpcode(c);}
}
static void prop_setup(void){
  ppu_reset(&p);memcpy(g_ram,start_ram,sizeof g_ram);
  memcpy(p.vram,start_vram,sizeof start_vram);memcpy(p.cgram,start_pal,sizeof start_pal);
  p.inidisp=15;p.obsel=2;p.bgmode=9;p.bgXsc[0]=0x71;p.bgXsc[1]=0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
  p.hScroll[0]=p.hScroll[1]=ram_word(g_ram,0x480)&1023;
  p.vScroll[0]=p.vScroll[1]=ram_word(g_ram,0x482)&1023;
  g_config.widescreen=1;LagoonWideEntities_Reset();LagoonWideEntities_Init();PpuSetExtraSpace(&p,213);
  LagoonWideEntities_RebindLoaded();assert(s_atlas.ready);
  assert(g_ram[0x500+prop+24]==5&&g_ram[0x500+prop+28]==0x3e);
  Interp816 *c=interp816_init(NULL,oracle_read,oracle_write);assert(c);
  /* Execute original actor culling/projection and literal type5 animation.
   * The prop remains outside the near draw queue, as in the live fixture. */
  unsigned before_n=ram_word(g_ram,0x48c);oracle_entry(prop,0xa4d3);oracle_entry(prop,0xa7b5);
  assert(ram_word(g_ram,0x48c)==before_n&&!(g_ram[0x500+prop]&8));
  assert(pose_centered(g_ram,0x500+prop)&&g_ram[0x500+prop+28]==0x3e);++original_projection;
  empty_oam();g_ram[0x2a2]=255;
  cpu_base(c,0x80f1,0);sync_cpu(c);LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f1);
  assert(s_draw_producer.begun);step_to(c,0x80f4);sync_cpu(c);g_cpu.master_cycles=2000;
  LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f4);assert(s_draw_producer.ready);
  memcpy(p.oam,g_ram+0x2000,512);memcpy(p.highOam,g_ram+0x2200,32);capture();assert(s_frame.producer_valid);
  interp816_free(c);
  /* Real next original actor pass after the disclosed5px CPU camera assist.
   * Its new0C/0E differ; the displayed OAM/PPU still name the prior producer. */
  put(0x480,ram_word(g_ram,0x480)+5);oracle_entry(prop,0xa4d3);oracle_entry(prop,0xa7b5);
  assert(pose_centered(g_ram,0x500+prop)&&!producer_context(s_frame.producer_ram,g_ram));
  uint8_t f;assert(!current_pose_frame(s_frame.ram,prop/64,&f));assert(producer_prop_frame(prop/64,&f)&&f==0x3e);
}
static void verify_original(void){
  uint8_t saved[sizeof g_ram];memcpy(saved,g_ram,sizeof saved);
  memcpy(g_ram,s_frame.producer_ram,kProducerRamBytes);empty_oam();
  /* ActualC44E calls real008B71 and writes the six native cells using the
   * frozen literal projection; no metasprite/OAM cells are fabricated. */
  oracle_entry(prop,0xc44e);
  LagoonWideCell cells[64];int n=decode_entity(s_frame.producer_ram,prop,0x3e,0,cells);
  assert(n==6&&g_ram[0x236]==6);
  for(int i=0;i<n;++i){unsigned at=0x2000+i*4,hi=g_ram[0x2200+i/4]>>((i&3)*2)&3;
    assert(g_ram[at]==(uint8_t)cells[i].x&&g_ram[at+1]==(uint8_t)cells[i].y);
    assert(ram_word(g_ram,at+2)==cells[i].tile&&hi==(((cells[i].x>>8)&1)|2));++original_cells;
  }
  memcpy(g_ram,saved,sizeof saved);
}
static void verify_pixels(void){
  memcpy(unchanged,g_ram,sizeof unchanged);LagoonWideEntities_BeginFrame(1);
  assert(!memcmp(unchanged,g_ram,sizeof unchanged)&&s_has_far&&p.extraObjectCount);
  LagoonWideCell cells[64];int n=decode_entity(s_frame.producer_ram,prop,0x3e,0,cells);assert(n==6);
  for(int i=0;i<n;++i){const LagoonWideCell*c=&cells[i];const PpuExtraObject*o=NULL;
    for(unsigned j=0;j<p.extraObjectCount;++j)if(p.extraObjects[j].x==c->x&&p.extraObjects[j].y==c->y)o=&p.extraObjects[j];assert(o);
    for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x){
      unsigned tile=(((c->tile&255)/16+y/8)*16)|(((c->tile&15)+x/8)&15);
      unsigned at=0x4000+tile*16+(y&7),color=0;
      for(unsigned b=0;b<4;++b)color|=((p.vram[at+(b/2)*8]>>((b&1)*8+7-(x&7)))&1)<<b;
      unsigned value=color?0x8000|p.cgram[0xa0+color]:0;
      assert(o->pixels[y*o->stride+x]==value);++prop_pixels;
    }
  }
}
static void refuse(void){uint8_t f;assert(!producer_prop_frame(prop/64,&f));++policy_refusals;}
int main(int argc,char**argv){
  assert(argc==3);baseline_root=argv[2];FILE*f=fopen(argv[1],"rb");assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);
  load_fixture("wram.bin",start_ram,sizeof start_ram);load_fixture("vram.bin",start_vram,sizeof start_vram);load_fixture("cgram.bin",start_pal,sizeof start_pal);
  prop_setup();verify_original();verify_pixels();
  /* An unrelated BG palette change remains valid; the original producer's
   * actual static cells consume only the live OBJ colorsA1..AF. */
  prop_setup();for(unsigned i=0;i<128;++i)p.cgram[i]^=0x7fff;verify_pixels();
  prop_setup();for(unsigned i=0xa1;i<=0xaf;++i)p.cgram[i]^=0x1234;verify_pixels();
  for(unsigned layer=0;layer<2;++layer)for(unsigned axis=0;axis<2;++axis){prop_setup();(axis?p.vScroll:p.hScroll)[layer]^=1;refuse();}
  for(unsigned side=0;side<3;++side)for(unsigned i=0;i<56;++i){prop_setup();uint8_t*ram=side==0?g_ram:side==1?s_frame.ram:s_frame.producer_ram;ram[0x50+i]^=1;refuse();}
  for(unsigned side=0;side<3;++side)for(unsigned at=0x90;at<=0x93;++at){prop_setup();uint8_t*ram=side==0?g_ram:side==1?s_frame.ram:s_frame.producer_ram;ram[at]^=1;refuse();}
  const unsigned metadata[]={0x48e,0x48f,0x4e1,0x100e};
  for(unsigned side=0;side<3;++side)for(unsigned i=0;i<4;++i){prop_setup();uint8_t*ram=side==0?g_ram:side==1?s_frame.ram:s_frame.producer_ram;ram[metadata[i]]^=1;refuse();}
  for(unsigned i=0;i<64;++i){if(i>=12&&i<16||i==0)continue;prop_setup();g_ram[0x500+prop+i]^=1;refuse();}
  for(unsigned i=0;i<64;++i){prop_setup();s_frame.producer_ram[0x500+prop+i]^=1;refuse();}
  prop_setup();g_ram[0x500+prop]=0;refuse();prop_setup();s_frame.producer_valid=0;refuse();
  for(unsigned i=0;i<97;++i){prop_setup();own_rom[0xc44e + i]^=1;refuse();own_rom[0xc44e + i]^=1;}
  prop_setup();s_frame.generation++;LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);++policy_refusals;
  for(unsigned i=0;i<544;++i){prop_setup();((uint8_t*)(i<512?(void*)p.oam:(void*)p.highOam))[i<512?i:i-512]^=1;LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);++policy_refusals;}
  prop_setup();s_atlas.ready=0;LagoonWideEntities_BeginFrame(1);assert(!s_has_far);++policy_refusals;
  prop_setup();observe_vram_byte(0x8800,((uint8_t*)p.vram)[0x8800]);LagoonWideEntities_BeginFrame(1);assert(!s_has_far);++policy_refusals;
  printf("PASS Atland static3E producer: %u actualA4D3/A7B5 projections, %u originalC44E/OAM cells; %u independent4bpp pixels incl liveOBJ/unrelatedBGpalette, %u context/source/scroll/currentidentity/generation/OAM/atlas refusals; guest RAM unchanged\n",original_projection,original_cells,prop_pixels,policy_refusals);
}
