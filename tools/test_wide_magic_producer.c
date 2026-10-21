#define LAGOON_WIDE_PROJECTILE_POLICY_NO_MAIN
#include "test_wide_projectile_producer.c"
#include "snes/interp816.h"
static unsigned magic_setups,magic_refusals;
static void magic_setup_at(int mixed,int y0,int y1){
 memset(&p,0,sizeof p);ppu_reset(&p);load_fixture("wram.bin",g_ram,sizeof g_ram);load_fixture("vram.bin",p.vram,sizeof p.vram);load_fixture("cgram.bin",p.cgram,sizeof p.cgram);
 assert(ram_word(g_ram,0x48e)==0x14&&g_ram[0x537]==0);p.inidisp=15;p.obsel=2;p.bgmode=9;p.bgXsc[0]=0x71;p.bgXsc[1]=g_ram[0x50]?0x7a:0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
 memset(g_ram+0x540,0,0x980);empty_oam();g_config.widescreen=1;LagoonWideEntities_Reset();LagoonWideEntities_Init();atlas();PpuSetExtraSpace(&p,213);
 Dma dma={0};DmaChannel *c=&dma.channel[1];c->dmaActive=1;c->mode=1;c->bAdr=0x18;c->aBank=127;c->aAdr=0x7f80;c->size=1248;p.vramPointer=0x5c00;observer(&dma,1,c);
 for(unsigned b=0;b<1248;++b)observe_vram_byte(0xb800+b,((uint8_t*)p.vram)[0xb800+b]);
 assert(s_magic_atlas.ready);projectile(0,1,0x16,351,y0);g_ram[0xd51]=0;g_ram[0xd52]=2;projectile(1,1,0x16,280,y1);g_ram[0xd69]=0;g_ram[0xd6a]=2;put(0x48c,2);
 if(y1>y0){uint8_t row[6];memcpy(row,g_ram+0xec0,6);memcpy(g_ram+0xec0,g_ram+0xec6,6);memcpy(g_ram+0xec6,row,6);}
 for(unsigned q=0;q<2;++q){LagoonWideCell cells[64];int n=LagoonWideEntities_MagicPacketCells(g_rom,sizeof own_rom,g_ram,q,cells,64);assert(n>0);for(int i=0;i<n;++i)if(cells[i].y>=-16&&cells[i].y<240)cell_oam(&cells[i]);}
 if(mixed){unsigned at=0x540;g_ram[at]=0xc0;g_ram[at+1]=4;g_ram[at+24]=2;g_ram[at+27]=13;g_ram[at+28]=0;put(at+2,ram_word(g_ram,0x480)+352);put(at+4,ram_word(g_ram,0x482)+100);put(at+12,352);put(at+14,100);}
 finish();assert(p.extraObjectCount>0&&s_has_magic&&!s_has_projectile);if(mixed)assert(s_has_far);++magic_setups;
}
static void magic_setup(int mixed){magic_setup_at(mixed,100,100);}
static void magic_scalar(void){
 for(unsigned j=0;j<p.extraObjectCount;++j){const PpuExtraObject *o=&p.extraObjects[j];unsigned slot=128;
  for(unsigned k=0;k<128;++k){unsigned h=(p.highOam[k/4]>>((k%4)*2))&3;unsigned xx=((uint8_t*)p.oam)[k*4]|((h&1)<<8),yy=((uint8_t*)p.oam)[k*4+1];
   if(xx==((unsigned)o->x&511)&&yy==((unsigned)o->y&255)&&o->width==((h&2)?16:8)){slot=k;break;}}
  if(slot>=128)continue;
  unsigned hi=(p.highOam[slot/4]>>((slot%4)*2))&3;unsigned tile=((uint8_t*)p.oam)[slot*4+2]|((uint8_t*)p.oam)[slot*4+3]<<8;
  int nx=((uint8_t*)p.oam)[slot*4]|((hi&1)<<8);if(nx>=384)nx-=512;if(nx!=o->x)continue;
  unsigned size=hi&2?16:8;assert(o->width==size);
  for(unsigned y=0;y<size;++y)for(unsigned x=0;x<size;++x){unsigned sx=tile&0x4000?size-1-x:x,sy=tile&0x8000?size-1-y:y;
   unsigned t=(((tile&255)/16+sy/8)*16)|(((tile&15)+sx/8)&15),adr=((tile&256?0x5000:0x4000)+t*16+(sy&7))&32767,color=0;
   for(unsigned plane=0;plane<4;++plane)color|=((p.vram[(adr+(plane/2)*8)&32767]>>((plane&1)*8+7-(sx&7)))&1)<<plane;
   unsigned expected=color?(0x8000|p.cgram[128+((tile>>9)&7)*16+color]):0;int screen=o->x+x,deep=screen < -16||screen>=272;
   if((screen>=0&&screen<256)||(s_has_far&&((o->order==1&&deep)||(o->order!=1&&!deep))))expected=0;
   assert(o->pixels[y*o->stride+x]==expected);++scalar_pixels;
  }
 }
}
static void magic_rejected(void){LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount&&!s_has_magic);++magic_refusals;}
/* Independent full original sort/draw creates reference native OAM for the
 * synthetic mixed group, including a deliberately forced far actor. This is a
 * composition policy check, never a claim that guest culling spawned it. */
static uint8_t reference_ram[0x20000];
static unsigned reference_product,composition_pixels,composition_overlap;
static uint8_t reference_read(void *ctx,uint32_t address){(void)ctx;unsigned b=address>>16,a=address&65535;
 if(b==126||b==127)return reference_ram[(b-126)*65536+a];
 if((b&127)<64&&a<8192)return reference_ram[a];
 if((b&127)<64&&(a==0x4216||a==0x4217))return reference_product>>((a-0x4216)*8);
 if((b&127)<64&&(a==0x4214||a==0x4215)){unsigned result=reference_ram[0x4206]?ram_word(reference_ram,0x4204)/reference_ram[0x4206]:65535;return result>>((a-0x4214)*8);}
 assert((b&127)<32&&a>=32768);return own_rom[(b&31)*32768+(a&32767)];
}
static void reference_write(void*ctx,uint32_t address,uint8_t value){(void)ctx;unsigned b=address>>16,a=address&65535;
 if(b==126||b==127){reference_ram[(b-126)*65536+a]=value;return;}
 if((b&127)<64&&(a==0x4202||a==0x4203)){reference_ram[a]=value;reference_product=reference_ram[0x4202]*reference_ram[0x4203];return;}
 if((b&127)<64&&a>=0x4204&&a<=0x4206){reference_ram[a]=value;return;}
 assert((b&127)<64&&a<8192);reference_ram[a]=value;
}
static void reference_put(unsigned a,unsigned v){reference_ram[a]=v;reference_ram[a+1]=v>>8;}
static unsigned reference_color(unsigned slot,int x,int y){unsigned h=(reference_ram[0x2200+slot/4]>>((slot%4)*2))&3;
 unsigned a=0x2000+slot*4,tile=ram_word(reference_ram,a+2),size=h&2?16:8;int xx=reference_ram[a]|((h&1)<<8),yy=reference_ram[a+1];if(xx>=384)xx-=512;if(yy>=240)yy-=256;
 if(x<xx||x>=xx+(int)size||y<yy||y>=yy+(int)size)return 0;unsigned sx=x-xx,sy=y-yy;if(tile&0x4000)sx=size-1-sx;if(tile&0x8000)sy=size-1-sy;
 unsigned t=(((tile&255)/16+sy/8)*16)|(((tile&15)+sx/8)&15),adr=((tile&256?0x5000:0x4000)+t*16+(sy&7))&32767,color=0;
 for(unsigned plane=0;plane<4;++plane)color|=((p.vram[(adr+(plane/2)*8)&32767]>>((plane&1)*8+7-(sx&7)))&1)<<plane;
 return color?0x8000|p.cgram[128+((tile>>9)&7)*16+color]:0;
}
static void composed(void){
 memcpy(reference_ram,g_ram,sizeof reference_ram);reference_put(0x48c,3);
 for(unsigned j=0;j<2;++j){reference_put(0xec0+j*6,j*24);reference_put(0xec2+j*6,ram_word(reference_ram,0xd4b+j*24));reference_ram[0xec4+j*6]=4;reference_ram[0xec5+j*6]=255-j;}
 reference_put(0xecc,0x40);reference_put(0xece,100);reference_ram[0xed0]=2;reference_ram[0xed1]=255;
 reference_ram[0x236]=0;reference_ram[0x2a2]=255;
 Interp816*c=interp816_init(NULL,reference_read,reference_write);assert(c);c->pc=0xc026;c->k=c->db=1;c->dp=0;c->sp=0x1d0;c->a=c->x=c->y=0;c->mf=1;c->xf=0;c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;reference_ram[0x1d1]=0xef;reference_ram[0x1d2]=0x7f;
 unsigned steps=0;while(c->k!=1||c->pc!=0x7ff0){assert(++steps<100000);interp816_runOpcode(c);}assert(c->sp==0x1d2);interp816_free(c);
 for(int y=0;y<224;++y){uint16_t depth[160]={0},colors[160]={0};uint64_t orders[160];memset(orders,255,sizeof orders);PpuComposeExtraObjects(p.extraObjects,p.extraObjectCount,y,272,160,0,depth,colors,orders);
  for(int x=272;x<432;++x){unsigned expected=0,overlap=0;for(unsigned slot=0;slot<reference_ram[0x236];++slot){unsigned color=reference_color(slot,x,y);if(color){if(!expected)expected=color;++overlap;}}
   assert(colors[x-272]==expected);++composition_pixels;if(overlap>1)++composition_overlap;
  }
 }
}
int main(int argc,char**argv){assert(argc==3);baseline_root=argv[2];FILE*f=fopen(argv[1],"rb");assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);
 magic_setup(0);magic_scalar();
 LagoonWideCell seam={250,100,0x2be1,8};uint16_t pixels[256];
 assert(LagoonWideEntities_Pixels(p.vram,p.cgram,0x4000,0x5000,&seam,pixels,1));
 for(unsigned y=0;y<8;++y)for(unsigned x=0;x<6;++x){assert(!pixels[y*8+x]);++center_zero;}
 magic_setup(1);magic_scalar();
 /* The next CPU can already change input/owner/clock: unchanged native OAM owns this displayed pose. */
 for(unsigned b=0;b<24;++b){magic_setup(0);g_ram[0xd40+b]^=1;g_ram[0xd58+b]^=1;LagoonWideEntities_BeginFrame(1);assert(p.extraObjectCount&&s_has_magic);magic_scalar();}
 magic_setup(0);g_ram[0x537]=4;LagoonWideEntities_BeginFrame(1);assert(p.extraObjectCount&&s_has_magic);magic_scalar();
 /* Original stable depth sort ignores the unused byte after every type. */
 magic_setup(1);for(unsigned padding=0;padding<256;++padding){s_frame.ram[0xec5]=padding;s_frame.ram[0xecb]=255-padding;LagoonWideEntities_BeginFrame(1);assert(p.extraObjectCount&&s_has_magic&&s_has_far);magic_scalar();}
 magic_setup_at(1,100,100);magic_scalar();composed();magic_setup_at(1,96,104);magic_scalar();composed();magic_setup_at(1,104,96);magic_scalar();composed();assert(composition_overlap>0);
 magic_setup(0);s_frame.ram[0x537]=4;magic_rejected();magic_setup(0);++s_magic_atlas.generation;magic_rejected();magic_setup(0);++s_magic_atlas.room;magic_rejected();
 magic_setup(0);s_frame.ram[0xd51]=s_frame.ram[0xd69]=1;magic_rejected();magic_setup(0);s_frame.ram[0xd4f]=s_frame.ram[0xd67]=255;magic_rejected();
 magic_setup(0);g_ram[0x480]^=1;magic_rejected();magic_setup(0);p.oam[0]^=1;magic_rejected();magic_setup(0);++s_frame.generation;magic_rejected();
 magic_setup(0);s_magic_atlas.ready=0;magic_rejected();magic_setup(0);p.inidisp=14;magic_rejected();magic_setup(0);p.bgmode=0;magic_rejected();magic_setup(0);g_ram[0x100e]=1;magic_rejected();
 magic_setup(0);LagoonWideEntities_RetainScene();assert(LagoonWideEntities_RetainedSceneValid()&&LagoonWideEntities_RetainedNativeAnchors());p.vram[0x5c00]^=1;assert(!LagoonWideEntities_RetainedSceneValid());++magic_refusals;
 magic_setup(0);LagoonWideEntities_RetainScene();assert(LagoonWideEntities_RetainedSceneValid());observe_vram_byte(0xb800,((uint8_t*)p.vram)[0xb800]);assert(!p.extraObjectCount&&!s_has_magic&&!LagoonWideEntities_RetainedSceneValid());++magic_refusals;
 magic_setup(0);LagoonWideEntities_RetainScene();Dma dma={0};DmaChannel *c=&dma.channel[1];c->dmaActive=1;c->mode=1;c->bAdr=0x18;c->aBank=127;c->aAdr=0x7f80;c->size=1248;p.vramPointer=0x5c00;observer(&dma,1,c);assert(!p.extraObjectCount&&!s_has_magic&&!LagoonWideEntities_RetainedSceneValid());++magic_refusals;
 magic_setup(0);LagoonWideEntities_RetainScene();LagoonWideEntities_BeginFrame(0);assert(!p.extraObjectCount&&!s_has_magic);LagoonWideEntities_BeginRetained(1);assert(p.extraObjectCount&&s_has_magic);observe_vram_byte(0xb800,((uint8_t*)p.vram)[0xb800]);assert(!p.extraObjectCount&&!LagoonWideEntities_RetainedSceneValid());++magic_refusals;
 magic_setup(0);for(unsigned b=0;b<s_magic_atlas.length;++b){((uint8_t*)p.vram)[0xb800+b]^=1;magic_rejected();((uint8_t*)p.vram)[0xb800+b]^=1;LagoonWideEntities_BeginFrame(1);assert(p.extraObjectCount&&s_has_magic);}
 magic_setup(0);for(unsigned b=0;b<256;++b)p.cgram[b]^=(b*71)&32767;LagoonWideEntities_BeginFrame(1);assert(p.extraObjectCount&&s_has_magic);magic_scalar();
 printf("magic composition:%u pixels/%u overlaps checked against full original C026/C41B/C214 OAM\n",composition_pixels,composition_overlap);
 printf("magic producer:%u synthetic setups,%u resource/source/OAM/alias/generation/currentpacket/retained refusals,%u independentplanar nativeOAM pixels/%u nativecenterzero PASS\n",magic_setups,magic_refusals,scalar_pixels,center_zero);return 0;
}
