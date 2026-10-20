/* Real host observer/PPU composition with original loader graphics; actor
 * records/OAM in these fixtures are synthetic. Actual spawned routes separate. */
#define LAGOON_WIDE_ACTOR_POLICY_NO_MAIN
#include "test_wide_additional_producer.c"
static unsigned positives,negatives_count,zero_center,scalar_pixels;
static void input(const char *name,const char *tag,const char *suffix,void*data,size_t size){char file[4096];snprintf(file,sizeof(file),"%s/%s/dumps/%s.%s",baseline_root,name,tag,suffix);FILE*f=fopen(file,"rb");assert(f&&fread(data,1,size,f)==size);fclose(f);}
static void unseen(unsigned type,unsigned kind,unsigned frame){
 memset(&p,0,sizeof(p));ppu_reset(&p);p.inidisp=15;p.obsel=2;p.bgmode=9;p.bgXsc[0]=0x71;p.bgXsc[1]=0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
 const char *name=type==1?"baseline-room21":"baseline-actor-14-2-13";const char *tag=type==1?"sample-11":"near";
 input(name,tag,"wram.bin",g_ram,sizeof(g_ram));input(name,tag,"vram.bin",p.vram,sizeof(p.vram));input(name,tag,"cgram.bin",p.cgram,sizeof(p.cgram));
 p.bgXsc[1]=g_ram[0x50]?0x7a:0x79;memset(g_ram+0x540,0,0x800);g_ram[0x558]=actor_type=type;g_ram[0x55b]=kind;g_ram[0x55c]=frame;g_ram[0x541]=type==2?4:0;
 g_config.widescreen=1;LagoonWideEntities_Reset();LagoonWideEntities_Init();atlas();PpuSetExtraSpace(&p,213);oam_for_record(-140,80,0);LagoonWideEntities_BeginFrame(1);
 assert(!s_native_pose[1].valid&&!s_kind_witness[type-1][kind]);assert(p.extraObjectCount>0);++positives;
}
static void failure(void){LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);++negatives_count;}
static void scalar_check(void){
 for(unsigned n=0;n<p.extraObjectCount;++n){const PpuExtraObject*o=&p.extraObjects[n];
  for(unsigned y=0;y<o->height;++y)for(unsigned x=0;x<o->width;++x){if(o->x+(int)x>=0&&o->x+(int)x<256){assert(!o->pixels[y*o->stride+x]);++zero_center;}++scalar_pixels;}
 }
 LagoonWideCell c[64];int n=decode_entity(s_frame.ram,0x40,s_frame.ram[0x55c],1,c);assert(n>0);
 /* Independent planar lookup/flip/palette oracle for each emitted pixel. */
 for(unsigned k=0;k<p.extraObjectCount;++k){const PpuExtraObject*o=&p.extraObjects[k];int match=-1;for(int i=0;i<n;++i)if(c[i].x==o->x&&c[i].y==o->y)match=i;assert(match>=0);const LagoonWideCell*cell=&c[match];
  for(unsigned y=0;y<o->height;++y)for(unsigned x=0;x<o->width;++x){unsigned sx=cell->tile&0x4000?cell->size-1-x:x,sy=cell->tile&0x8000?cell->size-1-y:y;unsigned t=(((cell->tile&255)/16+sy/8)*16)|(((cell->tile&15)+sx/8)&15);unsigned address=((cell->tile&256?0x5000:0x4000)+t*16+(sy&7))&32767;unsigned color=0;
   for(unsigned plane=0;plane<4;++plane){unsigned bytes=p.vram[(address+(plane/2)*8)&32767]>>((plane&1)*8);color|=((bytes>>(7-(sx&7)))&1)<<plane;}
   unsigned expected=color?(0x8000|p.cgram[128+((cell->tile>>9)&7)*16+color]):0;
   if(o->x+(int)x>=-16&&o->x+(int)x<272)expected=0;assert(o->pixels[y*o->stride+x]==expected);
  }
 }
}
int main(int argc,char**argv){assert(argc==3);baseline_root=argv[2];FILE*f=fopen(argv[1],"rb");assert(f&&fread(own_rom,1,sizeof(own_rom),f)==sizeof(own_rom));fclose(f);
 unseen(2,13,0);scalar_check();
 /* New pose/timer/world position can change without an old native anchor. */
 for(unsigned field=0;field<4;++field){unseen(2,13,0);unsigned at=field==0?0x550:field==1?0x55d:field==2?0x55c:0x542;g_ram[at]+=1;if(field==3)g_ram[0x54c]+=1;capture();LagoonWideEntities_BeginFrame(1);assert(!s_native_pose[1].valid&&p.extraObjectCount>0);++positives;scalar_check();}
 unseen(1,33,0);scalar_check();
 /* Current CGRAM is live palette-index interpretation, not a retained pose
  * palette. Changes are legitimate and must yield the exact current colors. */
 unseen(2,13,0);for(unsigned color=0;color<256;++color)p.cgram[color]^=(uint16_t)((color*37)&32767);LagoonWideEntities_BeginFrame(1);assert(p.extraObjectCount>0);scalar_check();
 LagoonWideCell crossing[64];int crossing_count=decode_entity(s_frame.ram,0x40,0,1,crossing);assert(crossing_count>0);crossing[0].x=250;uint16_t crossing_pixels[256];LagoonWideEntities_Pixels(p.vram,p.cgram,0x4000,0x5000,&crossing[0],crossing_pixels,1);
 for(unsigned y=0;y<crossing[0].size;++y)for(unsigned x=0;x<crossing[0].size&&x<6;++x){assert(!crossing_pixels[y*crossing[0].size+x]);++zero_center;}

 for(unsigned at=0;at<64;++at)for(unsigned target=0;target<2;++target){unseen(2,13,0);uint8_t*r=target?g_ram:s_frame.ram;r[0x540+at]^=1;failure();}
 const unsigned context[]={0x48e,0x48f,0x50,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5d,0x5e,0x5f,0x69,0x6a,0x6b,0x75,0x76,0x77,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x90,0x91,0x100e};
 for(unsigned j=0;j<sizeof(context)/sizeof(context[0]);++j)for(unsigned target=0;target<2;++target){unseen(2,13,0);(target?g_ram:s_frame.ram)[context[j]]^=1;failure();}
 unseen(2,13,0);++generation;failure();--generation;unseen(2,13,0);++s_frame.generation;failure();
 for(unsigned q=0;q<9;++q){unseen(2,13,0);switch(q){case 0:p.inidisp=14;break;case 1:p.bgmode=0;break;case 2:p.mosaic=1;break;case 3:p.setini=1;break;case 4:p.cgadsub=1;break;case 5:p.windowsel=1;break;case 6:p.obsel=3;break;case 7:p.oamaddh=128;break;case 8:p.bgXsc[1]^=1;break;}failure();}
 unseen(2,13,0);p.oam[0]^=1;failure();unseen(2,13,0);s_frame.valid=0;failure();unseen(2,13,0);s_frame.ram[0x48d]=1;failure();
 unseen(2,13,0);s_atlas.ready=0;failure();unseen(2,13,0);++s_atlas.generation;failure();unseen(2,13,0);s_atlas.source^=1;failure();
 unseen(2,13,0);LagoonWideCell cs[64];int count=decode_entity(g_ram,0x40,0,1,cs);assert(count>0);for(int i=0;i<count;++i){unsigned tile=cs[i].tile&255,base=cs[i].tile&256?0x5000:0x4000;for(unsigned y=0;y<cs[i].size;y+=8)for(unsigned x=0;x<cs[i].size;x+=8){unsigned t=((tile/16+y/8)*16)|((tile%16+x/8)&15),at=(base+t*16)*2;for(unsigned z=0;z<32;++z){s_atlas.owned[at+z-0x8800]=0;failure();s_atlas.owned[at+z-0x8800]=1;}}}
 unseen(2,13,0);own_rom[0xc07c&32767]^=1;failure();own_rom[0xc07c&32767]^=1;
 printf("current producer: %u synthetic unseen fixture setups (four distinct coherent pose changes), %u actual observer/context/source/record/generation/layout/atlas refusals; %u scalar sprite pixels/%u zero-native-center pixels PASS\n",positives,negatives_count,scalar_pixels,zero_center);return 0;}
