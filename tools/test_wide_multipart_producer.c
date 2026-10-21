/* Production observer with actual room43 loader graphics. Actor positions,
 * states and queue arrangements below are synthetic policy cases, not live
 * campaign coverage. The original C284 creates expected native OAM. */
#define LAGOON_WIDE_PROJECTILE_POLICY_NO_MAIN
#include "test_wide_projectile_producer.c"
#include "snes/interp816.h"
static unsigned oracle_product;
static uint8_t oracle_read(void *ctx,uint32_t address){
 (void)ctx;unsigned b=address>>16,a=address&65535;
 if(b==0x7e||b==0x7f)return g_ram[(b-0x7e)*65536+a];
 if((b&127)<64&&a<8192)return g_ram[a];
 if((b&127)<64&&(a==0x4216||a==0x4217))return oracle_product>>((a-0x4216)*8);
 if((b&127)<64&&(a==0x4214||a==0x4215))return (g_ram[0x4206]?ram_word(g_ram,0x4204)/g_ram[0x4206]:65535)>>((a-0x4214)*8);
 assert((b&127)<32&&a>=32768);return own_rom[(b&31)*32768+(a&32767)];
}
static void oracle_write(void *ctx,uint32_t address,uint8_t value){
 (void)ctx;unsigned b=address>>16,a=address&65535;
 if(b==0x7e||b==0x7f){g_ram[(b-0x7e)*65536+a]=value;return;}
 if((b&127)<64&&a>=0x4202&&a<=0x4206){g_ram[a]=value;oracle_product=g_ram[0x4202]*g_ram[0x4203];return;}
 assert((b&127)<64&&a<8192);g_ram[a]=value;
}
static void oracle_entry(unsigned offset,unsigned entry){
 Interp816 *c=interp816_init(NULL,oracle_read,oracle_write);assert(c);
 c->pc=entry;c->k=c->db=1;c->dp=0;c->sp=0x1d0;c->x=offset;c->y=c->a=0;
 c->mf=1;c->xf=0;c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;
 g_ram[0x1d1]=0xef;g_ram[0x1d2]=0x7f;unsigned steps=0;
 while(c->k!=1||c->pc!=0x7ff0){assert(++steps<100000);interp816_runOpcode(c);}
 assert(c->sp==0x1d2);interp816_free(c);
}
static void oracle_draw(unsigned offset){oracle_entry(offset,0xc284);}
static void common_owner(void){
 assert(LagoonWideDeath_Begin(&s_death_atlas,generation,g_rom,sizeof own_rom,127,0x7f80,0x4300,512));
 for(unsigned i=0;i<512;++i)LagoonWideDeath_Write(&s_death_atlas,0x8600+i,((uint8_t*)p.vram)[0x8600+i]);
 assert(LagoonWideDeath_Match(&s_death_atlas,p.vram,g_rom,sizeof own_rom,generation));
}
static void multipart_setup(unsigned kind,unsigned layers,int x){
 memset(&p,0,sizeof p);ppu_reset(&p);load_fixture("wram.bin",g_ram,sizeof g_ram);
 load_fixture("vram.bin",p.vram,sizeof p.vram);load_fixture("cgram.bin",p.cgram,sizeof p.cgram);
 assert(ram_word(g_ram,0x48e)==0x43);p.inidisp=15;p.obsel=2;p.bgmode=9;
 p.bgXsc[0]=0x71;p.bgXsc[1]=g_ram[0x50]?0x7a:0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
 memset(g_ram+0x540,0,0x980);empty_oam();g_config.widescreen=1;
 LagoonWideEntities_Reset();LagoonWideEntities_Init();atlas();common_owner();PpuSetExtraSpace(&p,213);
 g_ram[0x540]=0xc8;g_ram[0x541]=layers?1:0;g_ram[0x558]=3;g_ram[0x55b]=kind;g_ram[0x55c]=0;
 put(0x542,ram_word(g_ram,0x480)+x);put(0x544,ram_word(g_ram,0x482)+100);put(0x54c,x);put(0x54e,100);
 put(0x48c,1);put(0xec0,0x40);put(0xec2,100);put(0xec4,0xff03);
 g_ram[0x575]=4;g_ram[0x576]=g_ram[0x577]=255;
 g_ram[0x57b]=0;g_ram[0x57c]=g_ram[0x57d]=16;g_ram[0x2a2]=255;
 oracle_draw(0x40);finish();assert(s_multipart_packets&&s_multipart_matched);++setups;
 assert(p.extraObjectCount>0);scalar();
}
#ifndef LAGOON_WIDE_MULTIPART_PRODUCER_NO_MAIN
int main(int argc,char **argv){
 assert(argc==3);baseline_root=argv[2];FILE *f=fopen(argv[1],"rb");
 assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);
 for(unsigned kind=3;kind<=5;++kind){multipart_setup(kind,0,0);multipart_setup(kind,0,255);}
 multipart_setup(3,1,0);assert(s_has_death);LagoonWideEntities_RetainScene();
 assert(s_retained_objects.has_common&&LagoonWideEntities_RetainedSceneValid());
 p.vram[0x4300]^=1;assert(!LagoonWideEntities_RetainedSceneValid());++refused_cases;
 multipart_setup(3,1,0);observe_vram_byte(0x8600,((uint8_t*)p.vram)[0x8600]);
 assert(!p.extraObjectCount&&!s_has_death);++refused_cases;
 multipart_setup(3,1,0);LagoonWideEntities_RetainScene();Dma d={0};DmaChannel *c=&d.channel[1];
 c->dmaActive=1;c->mode=1;c->bAdr=0x18;c->aBank=127;c->aAdr=0x7f80;c->size=512;
 p.vramPointer=0x4300;observer(&d,1,c);assert(!p.extraObjectCount&&!s_retained_objects.valid);++refused_cases;
 multipart_setup(3,0,0);assert(!s_has_far&&!s_has_projectile&&!s_has_magic&&!s_has_death);
 observe_vram_byte(0x8800,((uint8_t*)p.vram)[0x8800]);
 assert(!p.extraObjectCount);++refused_cases; /* Unknown even same-value room-atlas write revokes now. */
 multipart_setup(3,0,0);memset(&d,0,sizeof d);c=&d.channel[3];
 c->dmaActive=1;c->mode=1;c->bAdr=0x18;c->aBank=127;c->aAdr=0x7f80;c->size=0x1000;
 p.vramPointer=0x4400;observer(&d,3,c);assert(!p.extraObjectCount&&!s_atlas.ready);++refused_cases;
 multipart_setup(3,0,0);g_ram[0x55c]^=1;g_ram[0x55b]=5;LagoonWideEntities_BeginFrame(1);
 assert(p.extraObjectCount>0);scalar(); /* Captured original OAM packet survives next CPU work. */
 multipart_setup(3,0,0);s_frame.ram[0x55c]=255;rejected();
 multipart_setup(3,1,0);s_frame.ram[0x575]=12;rejected(); /* Whole group, no partial borrowed layer. */
 multipart_setup(3,0,0);s_frame.ram[0x55b]=11;rejected();
 multipart_setup(3,0,0);s_frame.ram[0x57b]=16;s_frame.ram[0x541]=1;s_frame.ram[0x57c]=s_frame.ram[0x57d]=16;
 LagoonWideEntities_BeginFrame(1);assert(p.extraObjectCount>0);scalar(); /* Suppressed layers ignored. */
 multipart_setup(3,0,0);s_atlas.ready=0;rejected();
 multipart_setup(3,1,0);s_death_atlas.ready=0;rejected();
 multipart_setup(3,0,0);++s_frame.generation;rejected();
 multipart_setup(3,0,0);p.oam[0]^=1;rejected();
 multipart_setup(3,0,0);g_ram[0x480]^=1;rejected();
 printf("multipart producer: %u synthetic original-C284 OAM setups, %u ownership/unknown-layer/current-packet/generation/OAM/camera/retained refusals, %u independent native-OAM planar pixels/%u center zeros PASS; room43 graphics originate in separate actual loader fixture.\n",setups,refused_cases,scalar_pixels,center_zero);
 return 0;
}
#endif
