/* Real production observers/PPU with loader-resident room/common0 graphics.
 * Records and OAM are explicitly synthetic policy fixtures, separate from
 * full original opcode/NMI source proof and genuine assisted combat routes. */
#define LAGOON_WIDE_ACTOR_POLICY_NO_MAIN
#include "test_wide_additional_producer.c"
static unsigned synthetic_positive,refused,scalar_count,center_zero;
static void load(const char*suffix,void*data,size_t size){char path[4096];snprintf(path,sizeof(path),"%s/baseline-0b-loader/dumps/sample-11.%s",baseline_root,suffix);FILE*f=fopen(path,"rb");assert(f&&fread(data,1,size,f)==size);fclose(f);}
static void common(void){Dma d={0};DmaChannel*c=&d.channel[1];c->dmaActive=1;c->mode=1;c->bAdr=0x18;c->aBank=127;c->aAdr=0x7f80;c->size=512;p.vramPointer=0x4300;observer(&d,1,c);for(unsigned b=0;b<512;++b)observe_vram_byte(0x8600+b,((uint8_t*)p.vram)[0x8600+b]);assert(LagoonWideDeath_Match(&s_death_atlas,p.vram,g_rom,sizeof own_rom,generation));}
static void record(unsigned offset,unsigned kind,unsigned frame,int status,int x,int y,int queued){unsigned a=0x500+offset;memset(g_ram+a,0,64);g_ram[a]=queued?0xc8:0xc0;g_ram[a+1]=status;g_ram[a+24]=2;g_ram[a+27]=kind;g_ram[a+28]=frame;put(a+2,ram_word(g_ram,0x480)+x);put(a+4,ram_word(g_ram,0x482)+y);put(a+12,x);put(a+14,y);}
static void empty_oam(void){memset(g_ram+0x2000,0,544);for(unsigned n=0;n<128;++n){g_ram[0x2000+n*4]=64;g_ram[0x2200+n/4]|=(uint8_t)(1u<<((n%4)*2));}g_ram[0x236]=0;}
static void native_death(unsigned entity){LagoonWideCell c[64];int n=decode_entity(g_ram,entity,g_ram[0x51c+entity],0,c);assert(n>0);unsigned count=0;for(int i=0;i<n;++i){if(c[i].y< -16||c[i].y>=240)continue;unsigned a=0x2000+count*4,shift=(count%4)*2;g_ram[a]=c[i].x;g_ram[a+1]=c[i].y;put(a+2,c[i].tile);g_ram[0x2200+count/4]&=(uint8_t)~(3u<<shift);g_ram[0x2200+count/4]|=(uint8_t)((((c[i].x>>8)&1)|(c[i].size==16?2:0))<<shift);++count;}g_ram[0x236]=count;}
static void finish(void){memcpy(p.oam,g_ram+0x2000,512);memcpy(p.highOam,g_ram+0x2200,32);capture();LagoonWideEntities_BeginFrame(1);}
static void death_setup(unsigned frame,int mixed){memset(&p,0,sizeof p);ppu_reset(&p);p.inidisp=15;p.obsel=2;p.bgmode=9;p.bgXsc[0]=0x71;p.bgXsc[1]=0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
 load("wram.bin",g_ram,sizeof g_ram);load("vram.bin",p.vram,sizeof p.vram);load("cgram.bin",p.cgram,sizeof p.cgram);memset(g_ram+0x540,0,0x800);g_config.widescreen=1;LagoonWideEntities_Reset();LagoonWideEntities_Init();atlas();common();PpuSetExtraSpace(&p,213);empty_oam();
 if(mixed){record(0x40,5,0,0,-140,80,0);record(0x80,6,frame,1,308,100,1);put(0x48c,1);put(0xec0,0x80);put(0xec2,100);put(0xec4,2);native_death(0x80);}else{record(0x40,5,frame,1,-140,80,0);put(0x48c,0);}
 finish();assert(p.extraObjectCount>0&&!s_native_pose[1].valid);++synthetic_positive;
}
static void rejected(void){LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);++refused;}
static void scalar(unsigned entity){LagoonWideCell cells[64];int n=decode_entity(s_frame.ram,entity,s_frame.ram[0x51c+entity],1,cells);assert(n>0);
 for(unsigned q=0;q<p.extraObjectCount;++q){const PpuExtraObject*o=&p.extraObjects[q];int found=-1;for(int j=0;j<n;++j)if(cells[j].x==o->x&&cells[j].y==o->y)found=j;if(found<0)continue;const LagoonWideCell*c=&cells[found];
  for(unsigned y=0;y<o->height;++y)for(unsigned x=0;x<o->width;++x){unsigned sx=c->tile&0x4000?c->size-1-x:x,sy=c->tile&0x8000?c->size-1-y:y,t=(((c->tile&255)/16+sy/8)*16)|(((c->tile&15)+sx/8)&15);unsigned a=((c->tile&256?0x5000:0x4000)+t*16+(sy&7))&32767,color=0;
   for(unsigned plane=0;plane<4;++plane)color|=((p.vram[(a+(plane/2)*8)&32767]>>((plane&1)*8+7-(sx&7)))&1)<<plane;
   unsigned expected=color?(0x8000|p.cgram[128+((c->tile>>9)&7)*16+color]):0;if(o->x+(int)x>=-16&&o->x+(int)x<272)expected=0;assert(o->pixels[y*o->stride+x]==expected);++scalar_count;
  }
 }
 cells[0].x=250;uint16_t pixels[256];LagoonWideEntities_Pixels(p.vram,p.cgram,0x4000,0x5000,cells,pixels,1);for(unsigned y=0;y<cells[0].size;++y)for(unsigned x=0;x<6&&x<cells[0].size;++x){assert(!pixels[y*cells[0].size+x]);++center_zero;}
}
int main(int argc,char**argv){assert(argc==3);baseline_root=argv[2];FILE*f=fopen(argv[1],"rb");assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);
 for(unsigned frame=4;frame<=11;++frame){death_setup(frame,0);assert(s_has_death&&s_has_far);scalar(0x40);death_setup(frame,1);assert(s_has_death&&s_has_far);scalar(0x80);LagoonWideDeath_Reset(&s_death_atlas);rejected();}
 death_setup(6,0);for(unsigned at=0;at<64;++at)for(unsigned target=0;target<2;++target){uint8_t*r=target?g_ram:s_frame.ram;r[0x540+at]^=1;rejected();r[0x540+at]^=1;}
 death_setup(6,0);for(unsigned b=0;b<512;++b){((uint8_t*)p.vram)[0x8600+b]^=1;rejected();s_death_atlas.bytes[b]^=1;rejected();s_death_atlas.bytes[b]^=1;((uint8_t*)p.vram)[0x8600+b]^=1;}
 death_setup(6,0);++s_death_atlas.generation;rejected();--s_death_atlas.generation;++generation;rejected();--generation;death_setup(6,0);++s_frame.generation;rejected();
 death_setup(6,0);s_death_atlas.ready=0;rejected();death_setup(6,0);--s_death_atlas.completed;rejected();death_setup(6,0);s_death_atlas.left=1;rejected();
 death_setup(6,0);p.inidisp=14;rejected();death_setup(6,0);p.bgmode=0;rejected();death_setup(6,0);g_ram[0x100e]=1;rejected();death_setup(6,0);p.oam[0]^=1;rejected();
 /* Any real unknown global write clears cached queued AND far death cells. */
 death_setup(6,0);observe_vram_byte(0x8600,((uint8_t*)p.vram)[0x8600]);assert(!p.extraObjectCount&&!s_has_death&&!s_death_atlas.ready);++refused;
 death_setup(6,1);observe_vram_byte(0x87ff,((uint8_t*)p.vram)[0x87ff]);assert(!p.extraObjectCount&&!s_has_death&&!s_death_atlas.ready);++refused;
 death_setup(6,0);for(unsigned j=0;j<256;++j)p.cgram[j]^=(j*71)&32767;LagoonWideEntities_BeginFrame(1);assert(p.extraObjectCount>0);scalar(0x40);++synthetic_positive;
 /* A different DMA channel cannot establish a common0 owner. */
 death_setup(6,0);LagoonWideDeath_Reset(&s_death_atlas);Dma d={0};DmaChannel*c=&d.channel[2];c->dmaActive=1;c->mode=1;c->bAdr=0x18;c->aBank=127;c->aAdr=0x7f80;c->size=512;p.vramPointer=0x4300;observer(&d,2,c);for(unsigned b=0;b<512;++b)observe_vram_byte(0x8600+b,((uint8_t*)p.vram)[0x8600+b]);rejected();
 printf("death producer: %u synthetic fixture setups, all8 queued-death+ordinary-far/current-death frames; %u observer/source/current-record/byte/order/generation/layout refusals, %u scalar sprite pixels/%u native-center zero samples PASS\n",synthetic_positive,refused,scalar_count,center_zero);return 0;}
