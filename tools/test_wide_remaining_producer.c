/* Actual production observer with loader-resident graphics. Actor placement
 * and OAM composition are synthetic policy fixtures; opcode draw parity and
 * actual spawned gameplay are separate tests. */
#define LAGOON_WIDE_ACTOR_POLICY_NO_MAIN
#include "test_wide_additional_producer.c"
static unsigned held_positive,pose_refusals,source_cases,source_unsupported;
static void fresh(unsigned room,unsigned type,unsigned kind,unsigned frame,unsigned status){
 memset(&p,0,sizeof(p));ppu_reset(&p);p.inidisp=15;p.obsel=2;p.bgmode=9;
 read_dump(room,"wram.bin",g_ram,sizeof(g_ram));read_dump(room,"vram.bin",p.vram,sizeof(p.vram));read_dump(room,"cgram.bin",p.cgram,sizeof(p.cgram));
 p.bgXsc[0]=0x71;p.bgXsc[1]=g_ram[0x50]?0x7a:0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
 memset(g_ram+0x540,0,0x800);g_ram[0x540]=0xc8;g_ram[0x541]=status;g_ram[0x558]=actor_type=type;g_ram[0x55b]=kind;g_ram[0x55c]=frame;
 g_config.widescreen=1;LagoonWideEntities_Reset();LagoonWideEntities_Init();atlas();PpuSetExtraSpace(&p,213);
 oam_for_record(100,80,1);LagoonWideEntities_BeginFrame(1);
}
static void camera_far(void){
 unsigned oldx=ram_word(g_ram,0x542),oldy=ram_word(g_ram,0x544);
 put(0x480,ram_word(g_ram,0x480)+240);oam_for_record(-140,80,0);put(0x542,oldx);put(0x544,oldy);capture();
 LagoonWideEntities_BeginFrame(1);
}
static void current_pose_mutants(void){
 fresh(0x14,2,14,33,4);assert(s_native_pose[1].valid);assert(!s_kind_witness[1][14]);camera_far();
 assert(native_pose_matches(s_frame.ram,1)&&p.extraObjectCount>0);++held_positive;
 assert(!s_kind_witness[1][14]);assert(!native_pose_matches(s_frame.ram,2));++pose_refusals;
 for(unsigned b=0;b<64;++b)for(unsigned target=0;target<3;++target){
  if(b>=12&&b<16)continue;uint8_t*r=target==0?s_native_pose[1].record:target==1?s_frame.ram+0x540:g_ram+0x540;
  unsigned mask=b==0?1:1;r[b]^=mask;assert(!native_pose_matches(s_frame.ram,1));r[b]^=mask;++pose_refusals;
 }
 /* Queue membership and native coordinates are the only normalized fields;
  * each native projection must still match its own current camera exactly. */
 s_frame.ram[0x540]^=8;assert(native_pose_matches(s_frame.ram,1));s_frame.ram[0x540]^=8;
 for(unsigned target=0;target<2;++target){uint8_t*r=target?s_frame.ram:g_ram;for(unsigned b=12;b<16;++b){r[0x540+b]^=1;assert(!native_pose_matches(s_frame.ram,1));r[0x540+b]^=1;++pose_refusals;}}
 const unsigned fields[]={0x48e,0x48f,0x50,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5d,0x5e,0x5f,0x69,0x6a,0x6b,0x75,0x76,0x77,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x90,0x91};
 for(unsigned target=0;target<2;++target)for(unsigned b=0;b<sizeof(fields)/sizeof(fields[0]);++b){uint8_t*r=target?s_frame.ram:g_ram;unsigned at=fields[b];r[at]^=1;assert(!native_pose_matches(s_frame.ram,1));r[at]^=1;++pose_refusals;}
 ++s_native_pose[1].generation;assert(!native_pose_matches(s_frame.ram,1));--s_native_pose[1].generation;++pose_refusals;
 for(unsigned b=0;b<sizeof(s_native_pose[1].atlas)/sizeof(uint16_t);++b){s_native_pose[1].atlas[b]^=1;assert(!native_pose_matches(s_frame.ram,1));s_native_pose[1].atlas[b]^=1;++pose_refusals;}
 for(unsigned b=0;b<256;++b){p.cgram[b]^=1;assert(!native_pose_matches(s_frame.ram,1));p.cgram[b]^=1;++pose_refusals;}
 assert(native_pose_matches(s_frame.ram,1));
 ++s_native_pose[1].source;assert(!native_pose_matches(s_frame.ram,1));--s_native_pose[1].source;++pose_refusals;
 /* A stale own witness cannot be substituted by a different entity or kind. */
 s_native_pose[2]=s_native_pose[1];s_native_pose[1].valid=0;
 assert(!native_pose_matches(s_frame.ram,1));++pose_refusals;
 /* Current original draw source is independently valid without a held pose. */
 LagoonWideEntities_BeginFrame(1);assert(p.extraObjectCount>0);
}
int main(int argc,char**argv){assert(argc==3);baseline_root=argv[2];FILE*f=fopen(argv[1],"rb");assert(f&&fread(own_rom,1,sizeof(own_rom),f)==sizeof(own_rom));fclose(f);
 current_pose_mutants();
 const unsigned cases[][4]={{9,1,47,0},{0xb,2,5,0},{0xc,2,9,0},{0xd,1,27,0},{0x10,2,12,0},{0x11,1,61,0},{0x14,2,13,0},{0x15,2,14,33},{0x26,2,23,0},{0x26,2,26,0},{0x28,2,24,0},{0x29,1,7,0},{0x38,2,34,0},{0x3d,1,34,0},{0x3e,2,30,0},{0x44,2,36,0},{0x46,2,37,0},{0x49,1,36,0},{0x4a,1,35,1},{0xa,5,8,0x3e}};
 for(unsigned n=0;n<sizeof(cases)/sizeof(cases[0]);++n){unsigned room=cases[n][0],type=cases[n][1],kind=cases[n][2],frame=cases[n][3];fresh(room,type,kind,frame,(room==0x14||room==0x15)?4:0);
  LagoonWideCell cells[64];int count=decode_entity(g_ram,0x40,frame,0,cells);assert(count>0);
  if(!cells_owned(cells,count)){++source_unsupported;continue;}
  assert(s_native_pose[1].valid);camera_far();assert(native_pose_matches(s_frame.ram,1));assert(p.extraObjectCount>0);++held_positive;++source_cases;
  for(unsigned i=0;i<p.extraObjectCount;++i){const PpuExtraObject*o=&p.extraObjects[i];for(unsigned y=0;y<o->height;++y)for(unsigned x=0;x<o->width;++x)if(o->x+(int)x>=0&&o->x+(int)x<256){assert(!o->pixels[y*o->stride+x]);++center_pixels;}}
 }
 printf("remaining producer: %u synthetic unchanged-own-pose positives/%u source-family cases; %u dynamic/unowned art fixtures retain native fallback; %u full-record/entity/source/atlas/palette/DP90/current-captured/projection refusals PASS\n",held_positive,source_cases,source_unsupported,pose_refusals);return 0;}
