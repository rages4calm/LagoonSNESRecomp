/* Original room40 graphics, original activation/draw/shake/audio opcodes.
 * Camera and prior-offset arrangements are disclosed synthetic policy cases. */
#define LAGOON_WIDE_MULTIPART_COMPOSITE_NO_MAIN
#include "test_wide_multipart_composite_producer.c"
static unsigned onset_setups,onset_refusals,onset_ports;
static void onset_write(void*ctx,uint32_t address,uint8_t value){
 unsigned b=address>>16,a=address&65535;
 if((b&127)<64&&a>=0x2140&&a<=0x2143){assert(a==0x2141||a==0x2142);++onset_ports;return;}
 oracle_write(ctx,address,value);
}
static void onset_run(unsigned entry,unsigned x){
 unsigned bank=entry>>16;Interp816*c=interp816_init(NULL,oracle_read,onset_write);assert(c);
 c->pc=entry&65535;c->k=bank;c->db=1;c->dp=0;c->sp=0x1d0;c->x=x;c->y=c->a=0;
 c->mf=1;c->xf=0;c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;
 g_ram[0x1d1]=0xef;g_ram[0x1d2]=0x7f;unsigned n=0;
 while(c->k!=bank||c->pc!=0x7ff0){assert(++n<100000);interp816_runOpcode(c);}assert(c->sp==0x1d2);interp816_free(c);
}
static void onset_setup(int prior,int alternate){
 load_room(0);put(0x480,128);put(0x482,32);unsigned a=0x580;
 g_ram[a]=0xc9;g_ram[a+24]=5;g_ram[a+27]=9;g_ram[a+28]=0x3e;
 put(a+2,128);put(a+4,60);put(a+12,0);put(a+14,28);
 g_ram[0x3ae]=255;g_ram[0x3af]=42;onset_run(0x01de00,0);
 assert(!ram_word(g_ram,0x90)&&!ram_word(g_ram,0x92));
 put(0x90,prior);put(0x92,prior);g_ram[0x390]=0;g_ram[0x3b5]=21;
 onset_run(alternate?0x029f70:0x029072,alternate?0:0x40);
 assert(g_ram[0x3ae]&&g_ram[0x3af]==0);g_ram[0x2a2]=255;onset_run(0x01c44e,0x80);
 unsigned ports=onset_ports;onset_run(0x01d8da,0);assert(onset_ports==ports+2);
 put(0x48c,1);put(0xec0,0x80);put(0xec2,28);put(0xec4,0xff05);
 finish();++onset_setups;
 if(prior||alternate){assert(!p.extraObjectCount);++onset_refusals;}
 else{
  assert(p.extraObjectCount);scalar();
  LagoonWideCell current[64],far[64],public_cells[64],queued[64];
  int n=decode_entity(s_frame.ram,0x80,0x3e,0,current);assert(n==6);
  assert(decode_entity(s_frame.ram,0x80,0x3e,1,far)==n&&
      LagoonWideEntities_RecordCells(g_rom,sizeof own_rom,s_frame.ram,0x80,0x3e,0,public_cells,64)==n&&
      decode_queued_entity(s_frame.ram,0x80,0x3e,queued,0)==n);
  assert(!memcmp(current,far,n*sizeof *far)&&!memcmp(current,public_cells,n*sizeof *far)&&memcmp(current,queued,n*sizeof *far));
 }
}
static void onset_refuse(void){LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);++onset_refusals;}
int main(int argc,char**argv){
 assert(argc==3);FILE*f=fopen(argv[1],"rb");assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);room_prefix[0]=argv[2];
 onset_setup(0,0);
 for(int prior=-128;prior<128;++prior)if(prior)onset_setup(prior,0);
 onset_setup(0,1);
 onset_setup(0,0);s_frame.ram[0x90]^=1;onset_refuse();
 onset_setup(0,0);s_frame.ram[0x92]^=1;onset_refuse();
 onset_setup(0,0);s_frame.ram[0x3af]=0;onset_refuse();
 onset_setup(0,0);s_frame.ram[0x3ae]=0;onset_refuse();
 onset_setup(0,0);s_atlas.ready=0;onset_refuse();
 onset_setup(0,0);++s_atlas.generation;onset_refuse();
 onset_setup(0,0);s_atlas.source^=1;onset_refuse();
 onset_setup(0,0);unsigned tile=((uint8_t*)p.oam)[2]|((uint8_t*)p.oam)[3]<<8;
 unsigned address=((tile&0x100?0x5000:0x4000)+(tile&255)*16)*2;
 assert(address>=0x8800&&address<0xb200);s_atlas.owned[address-0x8800]=0;onset_refuse();
 onset_setup(0,0);p.oam[0]^=1;onset_refuse();
 onset_setup(0,0);++s_frame.generation;onset_refuse();
 onset_setup(0,0);g_ram[0x480]^=1;onset_refuse();
 onset_setup(0,0);own_rom[0x11072]^=1;onset_refuse();own_rom[0x11072]^=1;
 onset_setup(0,0);LagoonWideEntities_RetainScene();assert(LagoonWideEntities_RetainedNativeAnchors()&&LagoonWideEntities_RetainedSceneValid());
 g_ram[0x90]^=1;assert(LagoonWideEntities_RetainedSceneValid());LagoonWideEntities_BeginRetained(1);assert(p.extraObjectCount);scalar();
 observe_vram_byte(0x8800,((uint8_t*)p.vram)[0x8800]);assert(!p.extraObjectCount&&!LagoonWideEntities_RetainedSceneValid());++onset_refusals;
 printf("prop onset producer: %u original activation/draw/shake/audio setups, %u whole-prior/alternate/phase/OAM/owner/generation/camera/retained refusals, %u independent nativeOAM pixels/%u centerzeros PASS; public/current/far unchanged.\n",onset_setups,onset_refusals,scalar_pixels,center_zero);return 0;
}
