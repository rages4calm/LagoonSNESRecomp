/* Actual source weapon staging/OAM plus real PPU extra-object scanout. */
#include "snes/ppu.h"
#define main extra_obj_reference_main
#define pixels existing_reference_pixels
#define object existing_reference_object
#include "../snesrecomp/tests/ppu/ppu_extra_object_test.c"
#undef object
#undef pixels
#undef main
#include "../src/lagoon_sword_visual.c"
#include <stdlib.h>
static uint8_t rom[0x100000],ram[0x20000];
static uint32_t rendered[256*224];
static void put(unsigned a,unsigned v){ram[a]=(uint8_t)v;ram[a+1]=(uint8_t)(v>>8);}
static void pose(unsigned n){
 memset(ram,0,sizeof ram);memset(&p,0,sizeof p);ppu_reset(&p);
 p.inidisp=15;p.obsel=2;p.bgmode=1;p.screenEnabled[0]=16;
 for(unsigned i=0;i<128;i++)p.oam[i*2]=0xf000;
 for(unsigned i=0;i<256;i++)p.cgram[i]=0x7fff;
 ram[0x500]=0xc0;ram[0x501]=16;ram[0x417]=1;ram[0x51c]=(uint8_t)(0x24+n);ram[0x51d]=(uint8_t)n;ram[0x50a]=(uint8_t)(n/3);
 put(0x502,500);put(0x504,500);put(0x480,372);put(0x482,388);put(0x50c,128);put(0x50e,112);
 int x=128+(int16_t)word(rom,0xe2d4+(0x24+n)*4),y=112+(int16_t)word(rom,0xe2d6+(0x24+n)*4);
 for(unsigned q=0;q<4;q++){
  unsigned data=word(rom,0xf286+(n+1)*8+q*2),from=0x88000+(data&511)*32;
  for(unsigned j=0;j<16;j++)p.vram[0x4000+(25+q)*16+j]=(uint16_t)word(rom,from+j*2);
  p.oam[(40+q)*2]=(uint16_t)(((x+(q&1)*8)&255)|((y+(q>>1)*8)<<8));
  p.oam[(40+q)*2+1]=(uint16_t)((data&0xfe00)|(25+q));
 }
}
static unsigned first_pixel(const PpuExtraObject*o,int*x,int*y){
 for(unsigned n=0;n<o->width*o->height;n++)if(o->pixels[n]&0x8000){*x=o->x+n%o->stride;*y=o->y+n/o->stride;return n;}
 assert(0);return 0;
}
static void scan(unsigned fast){
 memset(rendered,0,sizeof rendered);PpuBeginDrawing(&p,(uint8_t*)rendered,256*4,fast?kPpuRenderFlags_NewRenderer:0);
 ppu_runLine(&p,0);for(unsigned line=1;line<=224;line++)ppu_runLine(&p,line);
}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(source(rom,sizeof rom));
 unsigned forms=0,mutants=0,refused=0;
 static uint16_t sentinel_pixel=0x83e0;
 PpuExtraObject sentinel={10,100,1,1,1,1,0,127,1,&sentinel_pixel};
 for(unsigned n=0;n<12;n++){
  pose(n);PpuSetExtraObjects(&p,&sentinel,1);
  uint8_t before[sizeof ram],guest[PPU_SAVESTATE_MEM_SIZE];memcpy(before,ram,sizeof ram);memcpy(guest,p.cgram,sizeof guest);
  assert(LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,1));assert(p.extraObjectCount==2);
  assert(!memcmp(&p.extraObjects[0],&sentinel,sizeof sentinel));
  assert(!memcmp(before,ram,sizeof ram)&&!memcmp(guest,p.cgram,sizeof guest));
  const PpuExtraObject*o=&p.extraObjects[1];assert(o->priority==2&&o->math==0&&o->oamSlot>=40&&o->oamSlot<=43&&o->order==1);
  unsigned colored=0;for(unsigned i=0;i<SIDE*SIDE;i++)colored+=(o->pixels[i]&0x8000)!=0;assert(colored>=12);
  assert(LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,1)&&p.extraObjectCount==2); /* idempotent */
  assert(!LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,0)&&p.extraObjectCount==1);forms++;
 }
 const unsigned spans[][2]={{0x01c156,37},{0x01c4ee,44},{0x01c579,38},{0x01e364,48},{0x01f286,104}};
 for(unsigned q=0;q<5;q++)for(unsigned n=0;n<spans[q][1];n++){
  unsigned at=(spans[q][0]>>16)*32768+(spans[q][0]&32767)+n;pose(2);rom[at]^=1;
  assert(!LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,1)&&p.extraObjectCount==0);rom[at]^=1;mutants++;
 }
 for(unsigned b=0;b<128;b++){
  pose(2);((uint8_t*)p.vram)[0x8000+25*32+b]^=1;
  assert(!LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,1));refused++;
 }
 for(unsigned n=0;n<16;n++){
  pose(2);
  switch(n){
   case 0:p.inidisp=14;break;case 1:p.obsel=3;break;case 2:p.oamaddh=128;break;
   case 3:p.bgmode=7;break;case 4:ram[0x100e]=1;break;case 5:ram[0x41b]=1;break;
   case 6:ram[0x417]=2;break;case 7:ram[0x500]=0;break;case 8:ram[0x501]=0;break;
   case 9:ram[0x515]=1;break;case 10:ram[0x51d]=255;break;case 11:ram[0x50a]=3;break;
   case 12:ram[0x51c]++;break;case 13:ram[0x50c]++;break;case 14:p.oam[40*2]++;break;
   case 15:p.oam[41*2+1]^=0x200;break;
  }
  assert(!LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,1));refused++;
 }
 for(unsigned fast=0;fast<2;fast++){
  pose(2);assert(LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,1));
  int x,y;first_pixel(p.extraObjects,&x,&y);scan(fast);assert(rendered[y*256+x]==0xffffff);
  /* High-priority BG1 still occludes the extension, in both PPU renderers. */
  p.screenEnabled[0]=17;p.bgXsc[0]=0x10;p.cgram[1]=31<<10;
  for(unsigned i=0;i<1024;i++)p.vram[0x1000+i]=0x2001;
  for(unsigned i=0;i<8;i++)p.vram[16+i]=0x00ff;
  scan(fast);assert(rendered[y*256+x]==0xff);
  for(unsigned i=0;i<1024;i++)p.vram[0x1000+i]=1;
  scan(fast);assert(rendered[y*256+x]==0xffffff);
  p.windowsel=2u<<16;p.window1left=(uint8_t)x;p.window1right=(uint8_t)x;p.screenWindowed[0]=16;
  scan(fast);assert(rendered[y*256+x]==0xff);
 }
 printf("PASS sword visual: %u exact sourceposes, %u sourcebytemutants, %u atlas refusals; read-only OAM/VRAM/RAM, append/idempotence, real legacy/fast PPU depth/windows\n",forms,mutants,refused);
 return 0;
}
