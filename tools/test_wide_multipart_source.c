/* Original full C284 -> C2CD/main handler -> 8B71 ->8211 is the independent
 * metadata oracle. Arranged policy records are not campaign progression. */
#define main previous_remaining_main
#include "test_wide_remaining_actors.c"
#undef main
#include "lagoon_wide_multipart.h"
static unsigned cases,exact_cells,refusals,layer_cases,suppressed,unowned,scalar_pixels,center_pixels,dispatch_pads;
static uint16_t pixel_vram[32768],pixel_cgram[256];
static const unsigned pairs[][2]={{0x42,2},{0x43,3},{0x43,4},{0x43,5},
 {0x44,6},{0x45,7},{0x45,8},{0x45,9},{0x46,10},{0x46,15},{0x48,14}};
static void setup(unsigned room,unsigned kind,unsigned frame,unsigned mode,int x,int y){
 memset(ram,0,sizeof ram);put(0x48e,room);put(0x480,256);put(0x482,128);
 put(0x48c,1);put(0xec0,0x40);put(0xec2,y);put(0xec4,3);ram[0xec5]=mode*37;
 ram[0x540]=0xc8;ram[0x541]=(mode&1)|((mode&2)?0x80:0);ram[0x55f]=(mode>>2)&1;
 ram[0x558]=3;ram[0x55b]=kind;ram[0x55c]=frame;ram[0x555]=mode*17;
 put(0x542,256+x);put(0x544,128+y);put(0x54c,x);put(0x54e,y);
 ram[0x57b]=0;ram[0x57c]=2;ram[0x57d]=mode&4?16:7;
 ram[0x575]=12;ram[0x576]=13;ram[0x577]=14;
}
static void compare(const LagoonWideCell *cells,int n){
 unsigned total=0;
 for(int i=0;i<n;++i)if(cells[i].y>=-16&&cells[i].y<240){
  unsigned o=0x2000+total*4,h=(ram[0x2200+total/4]>>((total%4)*2))&3;
  assert((unsigned)(cells[i].x&511)==(ram[o]|(h&1)<<8));
  assert((unsigned)(cells[i].y&255)==ram[o+1]);
  assert(cells[i].tile==word(ram,o+2)&&cells[i].size==((h&2)?16:8));
  ++total;++exact_cells;
 }assert(total==ram[0x236]);
}
static int owned(const LagoonWideCell *c){
 unsigned base=c->tile&256?0x5000:0x4000;
 for(unsigned y=0;y<c->size;y+=8)for(unsigned x=0;x<c->size;x+=8){
  unsigned tile=(((c->tile&255)/16+y/8)*16)|(((c->tile&15)+x/8)&15);
  unsigned a=(base+tile*16)&32767;if(a<0x4300||a+16>0x5900)return 0;
 }return 1;
}
static void scalar(const LagoonWideCell *c){
 uint16_t pixels[256];LagoonWideEntities_Pixels(pixel_vram,pixel_cgram,0x4000,0x5000,c,pixels,1);
 for(unsigned y=0;y<c->size;++y)for(unsigned x=0;x<c->size;++x){
  unsigned sx=c->tile&0x4000?c->size-1-x:x,sy=c->tile&0x8000?c->size-1-y:y;
  unsigned tile=(((c->tile&255)/16+sy/8)*16)|(((c->tile&15)+sx/8)&15);
  unsigned base=c->tile&256?0x5000:0x4000,addr=(base+tile*16+(sy&7))&32767,color=0;
  for(unsigned plane=0;plane<4;++plane)
   color|=((pixel_vram[(addr+(plane/2)*8)&32767]>>((plane&1)*8+7-(sx&7)))&1)<<plane;
  unsigned expected=color?0x8000|pixel_cgram[128+((c->tile>>9)&7)*16+color]:0;
  if(c->x+(int)x>=0&&c->x+(int)x<256){expected=0;++center_pixels;}
  assert(pixels[y*c->size+x]==expected);++scalar_pixels;
 }
}
#ifndef LAGOON_WIDE_MULTIPART_NO_MAIN
int main(int argc,char **argv){
 assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 for(unsigned i=0;i<32768;++i)pixel_vram[i]=(uint16_t)(i*3907+191);
 for(unsigned i=0;i<256;++i)pixel_cgram[i]=(uint16_t)(i*103&32767);
 Interp816 *cpu=interp816_init(NULL,read_byte,write_byte);assert(cpu);
 const int xs[]={-24,0,48,128,208,248,280},ys[]={-16,0,80,128,239,256};
 LagoonWideCell cells[256],sentinel[256];memset(sentinel,0xa5,sizeof sentinel);
 for(unsigned j=0;j<sizeof pairs/sizeof *pairs;++j)for(unsigned frame=0;frame<255;++frame){
  if(!LagoonWideEntities_CurrentSource(rom,sizeof rom,pairs[j][0]+1,frame))continue;
  for(unsigned mode=0;mode<8;++mode){
   unsigned sample=frame+mode+j;setup(pairs[j][0],pairs[j][1],frame,mode,xs[sample%7],ys[(sample/7)%6]);
   memcpy(cells,sentinel,sizeof cells);int n=LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,cells,256);assert(n>=0);
   /* C284 invokes actual hardware divide projection and original full queue
    * carrier. No reimplementation of the draw result creates expected OAM. */
   ram[0x2a2]=255;run(cpu,0xc284);compare(cells,n);++cases;
   if(mode&1)++layer_cases;if(!n)++suppressed;
   for(int i=0;i<n;++i)if(!owned(&cells[i]))++unowned;else scalar(&cells[i]);
   if(n){memcpy(cells,sentinel,sizeof cells);assert(LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,cells,n-1)<0);assert(!memcmp(cells,sentinel,sizeof cells));++refusals;}
  }
 }
 setup(0x43,3,0,1,48,128);assert(LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,cells,256)>0);
 for(unsigned pad=0;pad<256;++pad){
  setup(0x43,3,0,0,48,128);ram[0xec5]=pad;
  int n=LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,cells,256);assert(n>0);
  cpu->pc=0xc0aa;cpu->k=cpu->db=1;cpu->dp=0;cpu->sp=0x1d0;cpu->x=0x40;cpu->y=cpu->a=0;
  cpu->mf=1;cpu->xf=0;cpu->e=cpu->d=cpu->c=cpu->n=cpu->v=cpu->z=0;cpu->i=1;
  ram[0x1d1]=0xef;ram[0x1d2]=0x7f;ram[0x2a2]=255;unsigned steps=0;
  while(cpu->k!=1||cpu->pc!=0x7ff0){assert(++steps<100000);interp816_runOpcode(cpu);}
  compare(cells,n);++dispatch_pads;
 }
 setup(0x43,3,0,1,48,128);
 /* Missing/unknown layer and main-frame source refuse the complete group,
  * leaving every output byte untouched. Suppressed layers need no frame. */
 for(unsigned i=0;i<3;++i){unsigned a=0x575+i,old=ram[a];ram[a]=0;memcpy(cells,sentinel,sizeof cells);assert(LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,cells,256)<0);assert(!memcmp(cells,sentinel,sizeof cells));ram[a]=old;++refusals;}
 const unsigned spans[][2]={{0x1c284,407},{0x1fd9f,128},{0x1c486,41},{0x1c026,164},
   {0x05ace6,9},{0x05c1b2,3}};
 for(unsigned j=0;j<sizeof spans/sizeof *spans;++j)for(unsigned i=0;i<spans[j][1];++i){unsigned o=(spans[j][0]>>16)*32768+(spans[j][0]&32767)+i;rom[o]^=1;
  memcpy(cells,sentinel,sizeof cells);assert(LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,cells,256)<0);assert(!memcmp(cells,sentinel,sizeof cells));rom[o]^=1;++refusals;}
 const unsigned malformed[]={0x48c,0x48f,0xec0,0xec2,0xec4,0x558,0x55b,0x54c,0x54e};
 for(unsigned i=0;i<sizeof malformed/sizeof *malformed;++i){ram[malformed[i]]^=0x80;assert(LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,cells,256)<0);ram[malformed[i]]^=0x80;++refusals;}
 for(unsigned kind=0;kind<16;++kind){if(kind>=3&&kind<=5)continue;ram[0x55b]=kind;assert(LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,cells,256)<0);++refusals;}
 printf("multipart: %u original full C284 carrier cases/%u exact original OAM cells, %u optional-layer cases/%u original suppressed main bodies, %u original C0AA byte-dispatch padding variants; %u transactional/source/packet/unknown-part refusals; %u independent planar pixels/%u center-zero PASS. %u metadata cells require unowned-resource fallback (no authority granted here).\n",cases,exact_cells,layer_cases,suppressed,dispatch_pads,refusals,scalar_pixels,center_pixels,unowned);
 interp816_free(cpu);return 0;
}
#endif
