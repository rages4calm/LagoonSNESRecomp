/* Original room40 graphics and original prop draw/reset opcodes. Synthetic
 * camera/prior arrangements qualify finite source policy, not natural margins. */
#define LAGOON_WIDE_MULTIPART_COMPOSITE_NO_MAIN
#include "test_wide_multipart_composite_producer.c"
static unsigned tail_setups,tail_refusals,tail_current,tail_alternate;
static void tail_run(unsigned entry){
 Interp816*c=interp816_init(NULL,oracle_read,oracle_write);assert(c);
 c->pc=entry;c->k=c->db=1;c->dp=0;c->sp=0x1d0;c->x=entry==0xc44e?0x80:0;c->y=c->a=0;
 c->mf=1;c->xf=0;c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;
 g_ram[0x1d1]=0xef;g_ram[0x1d2]=0x7f;unsigned n=0;
 while(c->k!=1||c->pc!=0x7ff0){assert(++n<100000);interp816_runOpcode(c);}assert(c->sp==0x1d2);interp816_free(c);
}
static void tail_setup(int prior,int invisible){
 load_room(0);put(0x480,128);put(0x482,invisible?512:32);unsigned a=0x580;
 g_ram[a]=0xc9;g_ram[a+24]=5;g_ram[a+27]=9;g_ram[a+28]=0x3e;
 put(a+2,128);put(a+4,60);put(a+12,0);put(a+14,(uint16_t)(60-ram_word(g_ram,0x482)));
 g_ram[0x3ae]=255;g_ram[0x3af]=42;put(0x90,prior);put(0x92,prior);g_ram[0x2a2]=255;
 tail_run(0xc44e);tail_run(0xd8da);assert(!g_ram[0x3ae]&&!ram_word(g_ram,0x90));
 put(0x48c,1);put(0xec0,0x80);put(0xec2,ram_word(g_ram,0x58e));put(0xec4,0xff05);
 finish();tail_setups++;
 if(invisible){assert(!p.extraObjectCount);return;}
 if(prior!=0&&prior!=1){assert(!p.extraObjectCount);tail_refusals++;return;}
 assert(p.extraObjectCount);scalar();
 LagoonWideCell raw[64],queued[64],public_cells[64],far[64];
 int n=decode_entity(s_frame.ram,0x80,0x3e,0,raw);assert(n==6);
 assert(decode_queued_entity(s_frame.ram,0x80,0x3e,queued,0)==6);
 assert(LagoonWideEntities_RecordCells(g_rom,sizeof own_rom,s_frame.ram,0x80,0x3e,0,public_cells,64)==6);
 assert(decode_entity(s_frame.ram,0x80,0x3e,1,far)==6);
 assert(!memcmp(raw,public_cells,sizeof raw[0]*6)&&!memcmp(raw,far,sizeof raw[0]*6));
 if(prior){assert(memcmp(raw,queued,sizeof raw[0]*6));tail_alternate++;}
 else{assert(!memcmp(raw,queued,sizeof raw[0]*6));tail_current++;}
}
static void refuse(void){LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);tail_refusals++;}
int main(int argc,char**argv){
 assert(argc==3);FILE*f=fopen(argv[1],"rb");assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);room_prefix[0]=argv[2];
 for(int prior=-128;prior<128;prior++)tail_setup(prior,0);
 tail_setup(0,0); /* Stable resting group takes precedence even with stale42. */
 tail_setup(1,1);LagoonWideCell c[64];assert(decode_queued_entity(s_frame.ram,0x80,0x3e,c,0)==6);
 for(unsigned i=0;i<6;i++)assert(c[i].y< -16);assert(c[0].y==-498); /* No anchor: current0, not+1. */
 tail_setup(1,0);
 /* Conflicting partial current/alternate groups must never merge. Change one
  * original tail cell to current0 while every other cell remains original+1. */
 assert(decode_queued_entity(s_frame.ram,0x80,0x3e,c,0)==6);
 uint8_t used[128]={0};int first=-1;for(unsigned i=0;i<6;i++)if(c[i].y>=-16&&c[i].y<240){first=find_cell(s_frame.oam,c+i,used);break;}
 assert(first>=0);((uint8_t*)p.oam)[first*4+1]--;s_frame.oam[first*4+1]--;refuse();
 tail_setup(1,0);s_atlas.ready=0;refuse();
 tail_setup(1,0);s_atlas.generation++;refuse();
 tail_setup(1,0);s_atlas.source^=1;refuse();
 tail_setup(1,0);s_frame.generation++;refuse();
 tail_setup(1,0);g_ram[0x480]^=1;refuse();
 tail_setup(1,0);s_frame.ram[0x92]^=1;refuse();
 tail_setup(1,0);s_frame.ram[0x3ae]=255;refuse();
 tail_setup(1,0);s_frame.ram[0x3af]=41;refuse();
 tail_setup(1,0);own_rom[0xde00]^=1;refuse();own_rom[0xde00]^=1;
 tail_setup(1,0);unsigned tile=((uint8_t*)p.oam)[2]|((uint8_t*)p.oam)[3]<<8;
 unsigned address=((tile&0x100?0x5000:0x4000)+(tile&255)*16)*2;assert(address>=0x8800&&address<0xb200);
 s_atlas.owned[address-0x8800]=0;refuse();
 tail_setup(1,0);LagoonWideEntities_RetainScene();assert(LagoonWideEntities_RetainedNativeAnchors()&&LagoonWideEntities_RetainedSceneValid());
 g_ram[0x90]^=1;assert(LagoonWideEntities_RetainedSceneValid());LagoonWideEntities_BeginRetained(1);assert(p.extraObjectCount);scalar();
 observe_vram_byte(0x8800,((uint8_t*)p.vram)[0x8800]);assert(!p.extraObjectCount&&!LagoonWideEntities_RetainedSceneValid());tail_refusals++;
 /* Competing complete groups are deliberately constructed by two ORIGINAL
  * C44E calls, never by decoder-generated anchors. Current0 must still win. */
 tail_setup(1,0);tail_run(0xc44e);finish();scalar();
 LagoonWideCell current[64],alternative[64];
 assert(decode_entity(s_frame.ram,0x80,0x3e,0,current)==6&&
     decode_entity_offset(s_frame.ram,0x80,0x3e,0,alternative,1)==6);
 assert(queued_group_native_match(current,6)&&queued_group_native_match(alternative,6));
 assert(decode_queued_entity(s_frame.ram,0x80,0x3e,c,0)==6&&!memcmp(c,current,6*sizeof *c));tail_current++;
 assert(tail_setups==271&&tail_refusals==266&&tail_current==3&&tail_alternate==14);
 printf("prop tail producer:%u originaldraw/reset setups, %u whole-prior/partial/owner/camera/generation/source/retained refusals, %u current-zero preferred/%u tailalternates; %u independent nativeOAM pixels/%u centerzeros PASS; no-anchor declines, public/current/far unchanged.\n",tail_setups,tail_refusals,tail_current,tail_alternate,scalar_pixels,center_zero);return 0;
}
