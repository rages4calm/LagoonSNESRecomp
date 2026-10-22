/* External original-loader checkpoint, real source-created Thor/orbs or Bird,
 * full originalC026 draw and modeled completed DMA. The CPU checkpoint need
 * not be the earlier live OAM phase; actual runtime proof is separate. */
#define LAGOON_WIDE_MULTIPART_PRODUCER_NO_MAIN
#include "test_wide_multipart_producer.c"
static uint8_t input_ram[sizeof g_ram],draw_ram[sizeof g_ram];
static uint16_t input_vram[32768],input_palette[256];
static unsigned requested_kind,initial_count,expected_carriers,checks,policy_position;
static unsigned cell_refusals;
static void refuse_actual(unsigned where){
  LagoonWideEntities_BeginFrame(1);
  if(p.extraObjectCount){fprintf(stderr,"refusal%u retained%zuobjects kind%u\n",where,p.extraObjectCount,requested_kind);assert(0);}
  ++refused_cases;
}
static void setup_actual(void){
  ppu_reset(&p);memcpy(g_ram,input_ram,sizeof g_ram);
  memcpy(p.vram,input_vram,sizeof input_vram);memcpy(p.cgram,input_palette,sizeof input_palette);
  assert(ram_word(g_ram,0x48e)==0x47&&g_ram[0x558]==3&&g_ram[0x55b]==requested_kind&&!g_ram[0x100e]);
  p.inidisp=15;p.obsel=2;p.bgmode=9;p.bgXsc[0]=0x71;
  p.bgXsc[1]=g_ram[0x50]?0x7a:0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
  g_config.widescreen=1;LagoonWideEntities_Reset();LagoonWideEntities_Init();
  atlas();common_owner();PpuSetExtraSpace(&p,213);
  /* Separate synthetic seam policy. The real arena clamp keeps these actors
   * inside256; never describe this translated record as live margin proof. */
  if(policy_position){put(0x542,ram_word(g_ram,0x480)+252);put(0x54c,252);}
  empty_oam();g_ram[0x2a2]=255;oracle_entry(0,0xc026);
  memcpy(p.oam,g_ram+0x2000,512);memcpy(p.highOam,g_ram+0x2200,32);capture();
  memcpy(draw_ram,g_ram,sizeof g_ram);
  uint64_t before=s_multipart_packets;
  LagoonWideEntities_BeginFrame(1);assert(s_multipart_packets-before>=expected_carriers);
  assert(s_multipart_matched&&s_rejected==0);
  assert(!memcmp(draw_ram,g_ram,sizeof g_ram)&&!memcmp(input_vram,p.vram,sizeof input_vram)&&!memcmp(input_palette,p.cgram,sizeof input_palette));
  ++checks;
}
int main(int argc,char**argv){
  assert(argc==4);FILE*f=fopen(argv[1],"rb");assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);
  baseline_root=argv[2];requested_kind=(unsigned)strtoul(argv[3],NULL,0);
  assert(requested_kind==11||requested_kind==12);
  load_fixture("wram.bin",input_ram,sizeof input_ram);load_fixture("vram.bin",input_vram,sizeof input_vram);load_fixture("cgram.bin",input_palette,sizeof input_palette);
  expected_carriers=requested_kind==11?9:1;
  setup_actual();initial_count=p.extraObjectCount;
  /* Bird is a genuine current source carrier even when wholly inside the
   * native viewport. No artificial far Bird position/pose is claimed. */
  if(requested_kind==12)assert(s_multipart_matched>0);
  policy_position=1;setup_actual();assert(p.extraObjectCount>0);scalar();
  for(unsigned i=0;i<544;++i){setup_actual();((uint8_t*)s_frame.oam)[i]^=1;refuse_actual(i);}
  setup_actual();++s_frame.generation;refuse_actual(544);
  setup_actual();++s_atlas.generation;refuse_actual(545);
  setup_actual();s_atlas.ready=0;refuse_actual(546);
  setup_actual();g_ram[0x480]^=1;refuse_actual(547);
  setup_actual();g_ram[0x48e]^=1;refuse_actual(548);
  setup_actual();s_frame.ram[0x48e]^=1;refuse_actual(549);
  setup_actual();s_frame.ram[0x55b]=16;refuse_actual(550);
  setup_actual();own_rom[0x8de6]^=1;refuse_actual(551);own_rom[0x8de6]^=1;
  setup_actual();
  for(unsigned q=0;q<s_frame.ram[0x48c];++q)if(ram_word(s_frame.ram,0xec0+q*6)==0x40&&s_frame.ram[0xec4+q*6]==3){
    LagoonWideCell cells[128];int n=LagoonWideMultipart_PacketCells(own_rom,sizeof own_rom,s_frame.ram,q,cells,128);assert(n>0);
    for(int k=0;k<n;++k)for(unsigned y=0;y<cells[k].size;y+=8)for(unsigned x=0;x<cells[k].size;x+=8){
      unsigned tile=(((cells[k].tile&255)/16+y/8)*16)|(((cells[k].tile&15)+x/8)&15);
      unsigned address=((cells[k].tile&256?0x5000:0x4000)+tile*16)*2;assert(address>=0x8800&&address+32<=0xb200);
      for(unsigned b=0;b<32;++b){s_atlas.owned[address-0x8800+b]=0;refuse_actual(552);s_atlas.owned[address-0x8800+b]=1;++cell_refusals;}
    }
  }
  setup_actual();Dma d={0};DmaChannel*ch=&d.channel[1];
  ch->dmaActive=1;ch->mode=1;ch->bAdr=0x18;ch->aBank=127;ch->aAdr=0x7f80;ch->size=0x1000;
  p.vramPointer=0x4400;observer(&d,1,ch);assert(!p.extraObjectCount&&!s_atlas.ready);
  setup_actual();observe_vram_byte(0x8800,((uint8_t*)p.vram)[0x8800]);assert(!p.extraObjectCount);
  setup_actual();LagoonWideEntities_Reset();LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);
  printf("endgame producer kind%u: actual-checkpoint originalC026 carrier group positive (natural margin objects%u); %u modeled completed-DMA setups, including explicit synthetic X252 seam policy; %u complete native-OAM planar pixels/%u native-center zeros; %u OAM/source/descriptor/generation/owner refusals (including%u mixed-cell bytes), partialDMA/unknownwrite/cold PASS; guest arrays unchanged\n",requested_kind,initial_count,checks,scalar_pixels,center_zero,refused_cases,cell_refusals);
  return 0;
}
