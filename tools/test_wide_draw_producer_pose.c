/* Original JSR C026 / RTS pair, then two actual original NPC-loop iterations
 * around a modeled OAM DMA. The current CPU pose intentionally advances;
 * the completed producer's literal full OAM still owns the displayed pose. */
#define LAGOON_WIDE_MULTIPART_PRODUCER_NO_MAIN
#include "test_wide_multipart_producer.c"
CpuState g_cpu;
static uint8_t fixture_ram[sizeof g_ram],after[sizeof g_ram],display_oam[544];
static uint16_t fixture_vram[32768],fixture_palette[256];
static unsigned positive_pixels,refusal_count,sort_cases,auxiliary_cases;
static void cpu_base(Interp816*c,unsigned pc,unsigned x){
  c->pc=pc;c->k=c->db=1;c->dp=0;c->sp=0x1d0;c->x=x;c->a=c->y=0;
  c->mf=1;c->xf=0;c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;
}
static void sync_cpu(const Interp816*c){
  memset(&g_cpu,0,sizeof g_cpu);g_cpu.ram=g_ram;g_cpu.PB=c->k;g_cpu.DB=c->db;
  g_cpu.D=c->dp;g_cpu.S=c->sp;g_cpu.m_flag=c->mf;g_cpu.x_flag=c->xf;
  g_cpu.emulation=c->e;g_cpu._flag_D=c->d;g_cpu.P=c->mf?32:0;g_cpu.master_cycles=1000;
}
static void step_to(Interp816*c,unsigned pc){
  unsigned n=0;while(c->k!=1||c->pc!=pc){assert(++n<500000);interp816_runOpcode(c);}
}
static void advance_npc(Interp816*c){
  /* Actual original INC0408, then the isolated actual actor-loop body. Other
   * actors/frame services are not fabricated or claimed as campaign proof. */
  cpu_base(c,0x808b,0x300);interp816_runOpcode(c);assert(c->pc==0x808e);
  c->pc=0x80b2;step_to(c,0x80cd);
}
static void setup_pose(void){
  ppu_reset(&p);memcpy(g_ram,fixture_ram,sizeof g_ram);
  memcpy(p.vram,fixture_vram,sizeof fixture_vram);memcpy(p.cgram,fixture_palette,sizeof fixture_palette);
  assert(ram_word(g_ram,0x48e)==9&&g_ram[0x818]==1&&g_ram[0x81b]==54);
  p.inidisp=15;p.obsel=2;p.bgmode=9;p.bgXsc[0]=0x71;p.bgXsc[1]=g_ram[0x50]?0x7a:0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
  g_config.widescreen=1;LagoonWideEntities_Reset();LagoonWideEntities_Init();PpuSetExtraSpace(&p,213);
  LagoonWideEntities_RebindLoaded();assert(s_atlas.ready);
  empty_oam();g_ram[0x2a2]=255;
  Interp816*c=interp816_init(NULL,oracle_read,oracle_write);assert(c);
  /* Disclosed world-position assist primes original near-cull animation,
   * then returns the same original NPC to its far fixture point. No frame,
   * timer, type, queue membership or OAM is fabricated. */
  put(0x802,640);put(0x804,912);advance_npc(c);assert(g_ram[0x800]&8);
  put(0x802,784);put(0x804,905);advance_npc(c);assert(!(g_ram[0x800]&8));
  /* Isolated actor iterations above are not complete field rebuilds. Restore
   * the external fixture's exact original other-actor queue before drawing;
   * the far NPC is absent from that original queue, as in an actual rebuild. */
  memcpy(g_ram+0x48c,fixture_ram+0x48c,2);memcpy(g_ram+0xec0,fixture_ram+0xec0,0x180);
  cpu_base(c,0x80f1,0);sync_cpu(c);LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f1);assert(s_draw_producer.begun);
  step_to(c,0x80f4);sync_cpu(c);g_cpu.master_cycles=2000;
  LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f4);
  if(!s_draw_producer.ready){fprintf(stderr,"commit sp%04x/%04x physical%02x%02x parent%d context%d tick%u/%u actor%d queue%d\n",g_cpu.S,s_draw_producer.stack,g_ram[g_cpu.S],g_ram[g_cpu.S-1],memcmp(s_draw_producer.parent,g_ram+g_cpu.S+1,3),producer_context(s_draw_producer.ram,g_ram),s_draw_producer.ram[0x408],g_ram[0x408],memcmp(s_draw_producer.ram+0x500,g_ram+0x500,0x840),memcmp(s_draw_producer.ram+0xec0,g_ram+0xec0,0x180));for(unsigned k=0x500;k<0xd40;++k)if(s_draw_producer.ram[k]!=g_ram[k])fprintf(stderr,"diff%04x:%02x/%02x\n",k,s_draw_producer.ram[k],g_ram[k]);}
  assert(s_draw_producer.ready);
  assert(s_draw_producer.ram[0x236]==g_ram[0x236]&&g_ram[0x236]>0);
  memcpy(display_oam,g_ram+0x2000,544);
  advance_npc(c);assert(memcmp(g_ram+0x800,s_draw_producer.ram+0x800,64));
  assert(!memcmp(display_oam,g_ram+0x2000,544));
  memcpy(p.oam,display_oam,512);memcpy(p.highOam,display_oam+512,32);capture();assert(s_frame.producer_valid);
  advance_npc(c);interp816_free(c);
  uint8_t frame;assert(!LagoonWideEntities_CurrentFrame(own_rom,sizeof own_rom,s_frame.ram,g_ram,0x300,&frame));
  if(!producer_pose_frame(s_frame.ram,12,&frame)){
    const uint8_t*r=s_frame.producer_ram+0x800;LagoonWideAnimation an={r[16],r[17]};uint8_t f=255;
    fprintf(stderr,"producer fail valid%d context%d/%d centered%d rflags%02x now%02x type%u/%u kind%u/%u frame%u timer%u phase%u anim%d expected%u\n",s_frame.producer_valid,producer_context(s_frame.producer_ram,s_frame.ram),producer_context(s_frame.producer_ram,g_ram),pose_centered(s_frame.producer_ram,0x800),r[0],g_ram[0x800],r[24],g_ram[0x818],r[27],g_ram[0x81b],r[28],r[16],r[17],LagoonWideEntities_FrameForContext(own_rom,sizeof own_rom,s_frame.producer_ram,r,&an,0,&f),f);
  }
  assert(producer_pose_frame(s_frame.ram,12,&frame));
  memcpy(after,g_ram,sizeof g_ram);LagoonWideEntities_BeginFrame(1);
  assert(!memcmp(after,g_ram,sizeof g_ram));assert(s_has_far&&p.extraObjectCount&&s_producer_cells);
}
static void check_pixels(void){
  uint8_t frame;assert(producer_pose_frame(s_frame.ram,12,&frame));
  LagoonWideCell cells[64];int n=decode_entity(s_frame.producer_ram,0x300,frame,0,cells);assert(n>0);
  for(int k=0;k<n;++k){const LagoonWideCell*c=&cells[k];const PpuExtraObject*o=NULL;
    for(unsigned j=0;j<p.extraObjectCount;++j)if(p.extraObjects[j].x==c->x&&p.extraObjects[j].y==c->y)o=&p.extraObjects[j];assert(o);
    for(unsigned y=0;y<c->size;++y)for(unsigned x=0;x<c->size;++x){
      unsigned sx=c->tile&0x4000?c->size-1-x:x,sy=c->tile&0x8000?c->size-1-y:y;
      unsigned tile=(((c->tile&255)/16+sy/8)*16)|(((c->tile&15)+sx/8)&15);
      unsigned at=((c->tile&256?0x5000:0x4000)+tile*16+(sy&7))&32767,color=0;
      for(unsigned plane=0;plane<4;++plane)color|=((p.vram[(at+(plane/2)*8)&32767]>>((plane&1)*8+7-(sx&7)))&1)<<plane;
      unsigned value=color?0x8000|p.cgram[128+((c->tile>>9)&7)*16+color]:0;
      if(c->x+(int)x>=-16&&c->x+(int)x<272)value=0;
      assert(o->pixels[y*o->stride+x]==value);positive_pixels++;
    }
  }
}
static void no_producer(void){uint8_t f;assert(!producer_pose_frame(s_frame.ram,12,&f));++refusal_count;}
static void sorts(void){
  Interp816*c=interp816_init(NULL,oracle_read,oracle_write);assert(c);
  for(unsigned n=1;n<=64;++n)for(unsigned mode=0;mode<6;++mode){
    uint8_t before[kProducerRamBytes];memcpy(g_ram,fixture_ram,sizeof g_ram);
    memset(g_ram+0xec0,0xa5,0x180);put(0x48c,n);
    for(unsigned i=0;i<n;++i){unsigned q=0xec0+i*6;
      put(q,i*64);put(q+2,mode==0?100:mode==1?i:mode==2?64-i:mode==3?(i&1?65535:0):mode==4?(i*7103u+n*951u)&65535:(i%3)*100);
      g_ram[q+4]=i;g_ram[q+5]=255-i;
    }
    memcpy(before,g_ram,sizeof before);cpu_base(c,0xc026,0);step_to(c,0xc08b);
    assert(producer_sorted_queue(before,g_ram));++sort_cases;
  }interp816_free(c);
}
static void auxiliary_context(void){
  /* Finite context-policy test, not a created live enemy. The old0x1050
   * packet would read outside its bounds here. Check every actor offset,
   * including the exact8800 endpoint, and both original special-state gates. */
  unsigned room=0;while(room<75&&!LagoonWideEntities_AllowedKind(room,2,26))++room;
  assert(room<75&&sizeof s_frame.producer_ram==0x8801);
  for(unsigned e=0;e<33;++e)for(unsigned gate=0;gate<2;++gate)
    for(unsigned phase=0;phase<16;++phase){
      memset(s_frame.producer_ram,0,sizeof s_frame.producer_ram);
      uint8_t*ram=s_frame.producer_ram,*r=ram+0x500+e*64;
      ram[0x48e]=room;ram[0x408]=phase;r[24]=2;r[27]=26;
      if(gate)r[11]=0x80;else r[0]=1;
      LagoonWideAnimation animation={0,0};uint8_t frame;
      ram[0x8000+e*64]=1;
      assert(LagoonWideEntities_FrameForContext(own_rom,sizeof own_rom,ram,r,&animation,0,&frame));
      assert(frame==12+((phase&12)>>2));++auxiliary_cases;
    }
  for(unsigned at=0x8000;at<=0x8800;at+=64){
    setup_pose();LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f1);assert(s_draw_producer.begun);
    g_ram[at]^=1;LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f4);
    assert(!s_draw_producer.ready);capture();no_producer();
  }
}
int main(int argc,char**argv){
  assert(argc==3);baseline_root=argv[2];FILE*f=fopen(argv[1],"rb");assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);
  load_fixture("wram.bin",fixture_ram,sizeof fixture_ram);load_fixture("vram.bin",fixture_vram,sizeof fixture_vram);load_fixture("cgram.bin",fixture_palette,sizeof fixture_palette);
  memcpy(g_ram,fixture_ram,sizeof g_ram);
  for(unsigned y=908;y<=916;y+=8)for(unsigned x=636;x<=644;x+=8){
    put(0xb8,x);put(0xba,y);oracle_entry(0,0x8385);
    assert((g_ram[0x2c0]&15)==0&&g_ram[0x2c1]==0);
  }
  setup_pose();check_pixels();
  for(unsigned i=0;i<544;++i){setup_pose();s_draw_producer.oam[i]^=1;capture();assert(!s_frame.producer_valid);no_producer();}
  setup_pose();++generation;capture();no_producer();--generation;
  setup_pose();s_frame.producer_valid=0;no_producer();
  setup_pose();g_ram[0x81b]^=1;no_producer();
  setup_pose();g_ram[0x800]=0;no_producer();
  setup_pose();g_ram[0x480]^=1;no_producer();
  setup_pose();g_ram[0x50]^=1;no_producer();
  setup_pose();observe_vram_byte(0x8800,((uint8_t*)p.vram)[0x8800]);no_producer();
  setup_pose();LagoonWideEntities_Reset();no_producer();
  for(unsigned mode=0;mode<9;++mode){
    setup_pose();switch(mode){case 0:g_cpu.PB=2;break;case 1:g_cpu.DB=2;break;case 2:g_cpu.D=1;break;case 3:g_cpu.emulation=1;break;case 4:g_cpu.m_flag=0;break;case 5:g_cpu.x_flag=1;break;case 6:g_cpu._flag_D=1;break;case 7:g_cpu.P|=8;break;case 8:g_cpu.S=0xff;break;}
    LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f1);assert(!s_draw_producer.begun&&!s_draw_producer.ready);capture();no_producer();
  }
  for(unsigned i=0;i<6;++i){setup_pose();own_rom[0x80f1+i]^=1;LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f1);assert(!s_draw_producer.begun);capture();no_producer();own_rom[0x80f1+i]^=1;}
  setup_pose();LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f1);assert(s_draw_producer.begun);
  g_ram[g_cpu.S-1]^=1;LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f4);assert(!s_draw_producer.ready);capture();no_producer();
  setup_pose();LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f1);assert(s_draw_producer.begun);
  ++g_cpu.S;LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f4);assert(!s_draw_producer.ready);capture();no_producer();
  setup_pose();LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f1);assert(s_draw_producer.begun);
  g_ram[g_cpu.S+1]^=1;LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f4);assert(!s_draw_producer.ready);capture();no_producer();
  setup_pose();LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f1);g_ram[0x81c]^=1;
  LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f4);assert(!s_draw_producer.ready);capture();no_producer();
  setup_pose();LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f1);g_ram[0xec0]^=1;
  LagoonWideEntities_ObserveDraw(&g_cpu,0x0180f4);assert(!s_draw_producer.ready);capture();no_producer();
  auxiliary_context();sorts();
  printf("draw producer: actual0180F1 JSR/fullC026/physical80F4 RTS, two original movingNPC54 actor-loop steps around modeledDMA; oldCurrentFrame still refuses advanced CPU; new frozen producer emits exact %u planar values, %u OAM/generation/descriptor/identity/write/cold/auxiliary refusals; %u originalC026 stable unsigned-Y sort cases (1..64, ties/pads/extremes); %u enemy26 special-context cases through8000+offset bounded8800 PASS; guest state unchanged by witness/render\n",positive_pixels,refusal_count,sort_cases,auxiliary_cases);
  return 0;
}
