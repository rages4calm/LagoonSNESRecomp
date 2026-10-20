/* Six exact field contexts. Actual ROM/loader resources; explicitly synthetic
 * actor/OAM placement. Genuine original spawns have separate runtime proofs. */
#define LAGOON_WIDE_ACTOR_POLICY_NO_MAIN
#include "test_wide_additional_producer.c"
static unsigned kind_refusals;
static const unsigned cases[][2]={{0x18,15},{0x18,16},{0x1e,17},{0x1e,18},{0x20,21},{0x20,22}};
static void contexts(unsigned room,unsigned kind) {
  fixture(room,kind);far();
  const unsigned fields[]={0x48e,0x48f,0x50,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
    0x5d,0x5e,0x5f,0x69,0x6a,0x6b,0x75,0x76,0x77,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x100e};
  for(unsigned n=0;n<sizeof(fields)/sizeof(fields[0]);++n)for(unsigned captured=0;captured<2;++captured) {
    uint8_t *r=captured?s_frame.ram:g_ram;r[fields[n]]^=1;reject();r[fields[n]]^=1;
  }
  s_frame.ram[0x48d]=1;reject();s_frame.ram[0x48d]=0;
  s_frame.ram[0x48c]=65;reject();s_frame.ram[0x48c]=0;
  ++generation;reject();--generation;p.inidisp=14;reject();p.inidisp=15;
  p.obsel=3;reject();p.obsel=2;p.oamaddh=0x80;reject();p.oamaddh=0;
  own_rom[0x8000+(0xab5e&32767)]^=1;reject();own_rom[0x8000+(0xab5e&32767)]^=1;
  s_atlas.ready=0;reject();s_atlas.ready=1;++s_atlas.generation;reject();--s_atlas.generation;
  s_atlas.source^=1;reject();s_atlas.source^=1;s_atlas.room^=1;reject();s_atlas.room^=1;
  s_frame.ram[0x541]=4;reject();s_frame.ram[0x541]=0;
  s_kind_witness[1][kind]=0;reject();
  fixture(room,kind);oam_for_record(100,80,1);put(0x542,ram_word(g_ram,0x542)+1);
  capture();LagoonWideEntities_BeginFrame(1);assert(!s_kind_witness[1][kind]);++refusals;
  oam_for_record(-140,80,0);reject();
  for(unsigned bit=0;bit<8;++bit) {
    fixture(room,kind);oam_for_record(250,80,1);
    ((uint8_t*)p.oam)[3]^=(uint8_t)(1u<<bit);s_frame.oam[3]^=(uint8_t)(1u<<bit);reject();
  }
  fixture(room,kind);far();LagoonWideCell cells[64];int count=decode_entity(s_frame.ram,0x40,s_frame.ram[0x55c],1,cells);assert(count>0);
  for(int c=0;c<count;++c)for(unsigned y=0;y<cells[c].size;y+=8)for(unsigned x=0;x<cells[c].size;x+=8) {
    unsigned tile=((((cells[c].tile&255)>>4)+y/8)<<4)|(((cells[c].tile&15)+x/8)&15);
    unsigned base=cells[c].tile&256?0x5000:0x4000,address=(base+tile*16)*2;
    for(unsigned b=0;b<32;++b){s_atlas.owned[address+b-0x8800]=0;reject();s_atlas.owned[address+b-0x8800]=1;}
  }
}
static void kinds(void) {
  const unsigned rooms[]={0x18,0x1d,0x1e,0x1f,0x20,0x21};LagoonWideCell cells[64];
  for(unsigned n=0;n<6;++n) {
    read_dump(rooms[n],"wram.bin",g_ram,sizeof(g_ram));
    for(unsigned type=1;type<=5;++type)for(unsigned kind=0;kind<256;++kind) {
      g_ram[0x558]=(uint8_t)type;g_ram[0x55b]=(uint8_t)kind;
      if(type!=2 || !field_enemy_kind(rooms[n],kind)) {
        assert(decode_entity(g_ram,0x40,0,0,cells)<0);++kind_refusals;
      }
    }
  }
}
int main(int argc,char **argv) {
  assert(argc==3);baseline_root=argv[2];FILE*f=fopen(argv[1],"rb");
  assert(f&&fread(own_rom,1,sizeof(own_rom),f)==sizeof(own_rom));fclose(f);
  for(unsigned n=0;n<6;++n) {
    unsigned room=cases[n][0],kind=cases[n][1];fixture(room,kind);far();
    oam_for_record(250,80,1);LagoonWideEntities_BeginFrame(1);assert(p.extraObjectCount>0);
    native_cells+=(unsigned)p.extraObjectCount;
    for(unsigned c=0;c<p.extraObjectCount;++c) {
      const PpuExtraObject*o=&p.extraObjects[c];assert(o->order==1);
      for(unsigned y=0;y<o->height;++y)for(unsigned x=0;x<o->width;++x)
        if(o->x+(int)x>=0&&o->x+(int)x<256){assert(!o->pixels[y*o->stride+x]);++center_pixels;}
    }
    contexts(room,kind);
  }
  kinds();
  printf("field production policy: six actual ROM/loader enemy kinds, %u synthetic far cells/%u original-slot seams, %u native-opacity zero pixels, %u kind/NPC/prop refusals, %u context/source/OAM/atlas/center refusals PASS\n",far_cells,native_cells,center_pixels,kind_refusals,refusals);
  return 0;
}
