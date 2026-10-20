/* Source-backed NPC19..1C policy tests. Placement is explicitly synthetic;
 * actual NPCs are separately checked through assisted runtime routes. */
#define LAGOON_WIDE_ACTOR_POLICY_NO_MAIN
#include "test_wide_additional_producer.c"
static unsigned kind_refusals,room_refusals;
static void bounded_kinds(void) {
  LagoonWideCell cells[64];
  const unsigned scenes[]={0,1,2,3,4,5,6,7,8,0x19,0x1a,0x1b,0x1c,
    0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37};
  for(unsigned n=0;n<sizeof(scenes)/sizeof(scenes[0]);++n)for(unsigned kind=64;kind<256;++kind) {
    /* Exact authored whitelist + descriptor is required for newly high IDs,
     * including native-anchored seams; no old scene gains this authority. */
    fixture(0x19,70);g_ram[0x55b]=(uint8_t)kind;
    put(0x48e,scenes[n]);
    int admitted=town_npc_kind(scenes[n],kind) && LagoonSceneRenderer_AdditionalRoom(g_ram);
    if(!admitted){assert(decode_entity(g_ram,0x40,0,0,cells)<0);++kind_refusals;}
  }
}
static void contexts(void) {
  fixture(0x19,70);far();
  const unsigned fields[]={0x48e,0x48f,0x50,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
    0x5d,0x5e,0x5f,0x69,0x6a,0x6b,0x75,0x76,0x77,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x100e};
  for(unsigned n=0;n<sizeof(fields)/sizeof(fields[0]);++n)for(unsigned captured=0;captured<2;++captured) {
    uint8_t *r=captured?s_frame.ram:g_ram;r[fields[n]]^=1;reject();r[fields[n]]^=1;
  }
  s_frame.ram[0x48d]=1;reject();s_frame.ram[0x48d]=0;
  ++generation;reject();--generation;p.inidisp=14;reject();p.inidisp=15;
  p.obsel=3;reject();p.obsel=2;p.oamaddh=0x80;reject();p.oamaddh=0;
  own_rom[0x8000+(0xac4f&32767)]^=1;reject();own_rom[0x8000+(0xac4f&32767)]^=1;
  s_atlas.ready=0;reject();s_atlas.ready=1;++s_atlas.generation;reject();--s_atlas.generation;
  s_atlas.source^=1;reject();s_atlas.source^=1;s_atlas.room^=1;reject();s_atlas.room^=1;
  s_kind_witness[0][70]=0;reject();
  fixture(0x19,70);oam_for_record(100,80,1);put(0x542,ram_word(g_ram,0x542)+1);
  capture();LagoonWideEntities_BeginFrame(1);assert(!s_kind_witness[0][70]);++refusals;
  oam_for_record(-140,80,0);reject();
  const unsigned masks[]={1,2,4,8,16,32,64,128};
  for(unsigned n=0;n<8;++n) {
    fixture(0x19,70);oam_for_record(250,80,1);
    ((uint8_t*)p.oam)[3]^=(uint8_t)masks[n];s_frame.oam[3]^=(uint8_t)masks[n];reject();
  }
  /* Mutable atlas overwrites revoke every byte of each actual decoded cell. */
  fixture(0x19,70);far();LagoonWideCell cell[64];int count=decode_entity(s_frame.ram,0x40,s_frame.ram[0x55c],1,cell);assert(count>0);
  for(int c=0;c<count;++c)for(unsigned y=0;y<cell[c].size;y+=8)for(unsigned x=0;x<cell[c].size;x+=8) {
    unsigned tile=((((cell[c].tile&255)>>4)+y/8)<<4)|(((cell[c].tile&15)+x/8)&15);
    unsigned base=cell[c].tile&256?0x5000:0x4000,address=(base+tile*16)*2;
    for(unsigned byte=0;byte<32;++byte){s_atlas.owned[address+byte-0x8800]=0;reject();s_atlas.owned[address+byte-0x8800]=1;}
  }
}
int main(int argc,char **argv) {
  assert(argc==3);baseline_root=argv[2];FILE *f=fopen(argv[1],"rb");
  assert(f&&fread(own_rom,1,sizeof(own_rom),f)==sizeof(own_rom));fclose(f);actor_type=1;
  const unsigned cases[][2]={{0x19,69},{0x19,70},{0x19,71},{0x19,72},{0x19,73},
    {0x19,74},{0x19,75},{0x19,76},{0x19,77},{0x19,78},{0x1a,30},{0x1a,31},
    {0x1a,79},{0x1a,91},{0x1b,18},{0x1c,19},{0x1c,20},{0x1c,21},{0x1c,22},
    {0x1c,23},{0x1c,24},{0x1c,25},{0x1c,26}};
  for(unsigned n=0;n<sizeof(cases)/sizeof(cases[0]);++n) {
    fixture(cases[n][0],cases[n][1]);far();
    oam_for_record(268,80,1);LagoonWideEntities_BeginFrame(1);
    if(!p.extraObjectCount){fprintf(stderr,"town seam kind=%u room=%02x\n",cases[n][1],cases[n][0]);assert(0);}
    native_cells+=(unsigned)p.extraObjectCount;
    for(unsigned i=0;i<p.extraObjectCount;++i) {
      const PpuExtraObject *o=&p.extraObjects[i];assert(o->order==1);
      for(unsigned y=0;y<o->height;++y)for(unsigned x=0;x<o->width;++x)
        if(o->x+(int)x>=0 && o->x+(int)x<256){assert(!o->pixels[y*o->stride+x]);++center_pixels;}
    }
  }
  bounded_kinds();contexts();
  printf("town production actor policy:23 actual ROM/loader NPC kinds, %u synthetic far cells/%u original-slot seams, %u native-opacity zero pixels, %u individual kind/oldscene refusals, %u context/source/OAM/atlas/center refusals PASS\n",far_cells,native_cells,center_pixels,kind_refusals,refusals);
  return 0;
}
