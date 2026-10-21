/* Actual original room40 graphics; synthetic camera/queue/shake arrangements.
 * Original C44E then full D8DA create OAM and the post-draw phase mismatch. */
#define LAGOON_WIDE_MULTIPART_COMPOSITE_NO_MAIN
#include "test_wide_multipart_composite_producer.c"
static unsigned shake_setups,shake_refusals;
static void shake_setup(unsigned phase){
 uint64_t rejected_before=s_rejected,validated_before=s_validated;
 load_room(0);put(0x480,128);put(0x482,32);
 unsigned a=0x580;g_ram[a]=0xc9;g_ram[a+24]=5;g_ram[a+27]=9;g_ram[a+28]=0x3e;
 put(a+2,128);put(a+4,60);put(a+12,0);put(a+14,28);
 put(0x90,ram_word(own_rom,0xde11+(phase-1)*2));g_ram[0x3ae]=255;g_ram[0x3af]=phase;
 g_ram[0x2a2]=255;oracle_entry(0x80,0xc44e);oracle_entry(0,0xd8da);
 put(0x48c,1);put(0xec0,0x80);put(0xec2,28);put(0xec4,0xff05);finish();
 assert(s_validated>validated_before&&p.extraObjectCount&&s_rejected==rejected_before);scalar();++shake_setups;
 LagoonWideCell current[64],far[64],public_cells[64],queued_cells[64];
 int n=decode_entity(s_frame.ram,0x80,0x3e,0,current);
 assert(n==6&&decode_entity(s_frame.ram,0x80,0x3e,1,far)==n&&
     LagoonWideEntities_RecordCells(g_rom,sizeof own_rom,s_frame.ram,0x80,0x3e,0,public_cells,64)==n);
 assert(!memcmp(current,far,n*sizeof *far)&&!memcmp(current,public_cells,n*sizeof *far));
 assert(decode_queued_entity(s_frame.ram,0x80,0x3e,queued_cells,0)==n);
 assert(memcmp(current,queued_cells,n*sizeof *far));
}
static void shake_refuse(void){LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);++shake_refusals;}
int main(int argc,char**argv){
 assert(argc==3);FILE*f=fopen(argv[1],"rb");assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);room_prefix[0]=argv[2];
 for(unsigned phase=1;phase<42;phase++)shake_setup(phase);
 shake_setup(1);s_frame.ram[0x3af]=1;shake_refuse();
 shake_setup(1);s_frame.ram[0x90]^=1;shake_refuse();
 shake_setup(1);s_frame.ram[0x92]^=1;shake_refuse();
 shake_setup(1);s_frame.ram[0x3ae]=0;shake_refuse();
 shake_setup(1);s_atlas.ready=0;shake_refuse();
 shake_setup(1);++s_atlas.generation;shake_refuse();
 shake_setup(1);s_atlas.source^=1;shake_refuse();
 /* The exact resource below is independently decoded from the original OAM. */
 shake_setup(1);unsigned tile=((uint8_t*)p.oam)[2]|((uint8_t*)p.oam)[3]<<8;
 unsigned address=((tile&0x100?0x5000:0x4000)+(tile&255)*16)*2;
 assert(address>=0x8800&&address<0xb200);s_atlas.owned[address-0x8800]=0;shake_refuse();
 shake_setup(1);p.oam[0]^=1;shake_refuse();
 shake_setup(1);++s_frame.generation;shake_refuse();
 shake_setup(1);g_ram[0x480]^=1;shake_refuse();
 shake_setup(1);own_rom[0x80f1]^=1;shake_refuse();own_rom[0x80f1]^=1;
 shake_setup(1);LagoonWideEntities_RetainScene();assert(LagoonWideEntities_RetainedNativeAnchors()&&LagoonWideEntities_RetainedSceneValid());
 g_ram[0x90]^=1;assert(LagoonWideEntities_RetainedSceneValid());LagoonWideEntities_BeginRetained(1);assert(p.extraObjectCount);scalar();
 observe_vram_byte(0x8800,((uint8_t*)p.vram)[0x8800]);assert(!p.extraObjectCount&&!LagoonWideEntities_RetainedSceneValid());++shake_refusals;
 printf("queued prop shake producer: %u original-draw synthetic setups, %u phase/source/owner/OAM/generation/camera/retained refusals; %u independent nativeOAM pixels/%u centerzeros PASS. Public/current/far decode semantics unchanged.\n",shake_setups,shake_refusals,scalar_pixels,center_zero);return 0;
}
