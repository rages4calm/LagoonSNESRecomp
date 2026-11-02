/* The original sword qualification plus optional cue geometry/real PPU checks. */
#define LAGOON_SWORD_VISUAL_TEST_ENTRY sword_visual_existing_main
#include "lagoon_sword_visual_test.c"

static const int16_t qol_bounds[4][4]={
 {0,38,-11,15},{-27,11,0,38},{-38,0,-15,11},{-11,27,-38,0}
};
static unsigned cue_cases,edge_checks,refusals,clipped_pixels;
static void patch_qol(void){
 for(unsigned d=0;d<4;++d)for(unsigned e=0;e<4;++e){
  unsigned at=0xb6f0+d*8+e*2;uint16_t value=(uint16_t)qol_bounds[d][e];
  rom[at]=(uint8_t)value;rom[at+1]=(uint8_t)(value>>8);
 }
}
static void move_hero(unsigned n,int x,int y){
 pose(n);put(0x50c,(uint16_t)x);put(0x50e,(uint16_t)y);
 put(0x480,(uint16_t)(500-x));put(0x482,(uint16_t)(500-y));
 int ox=x+(int16_t)word(rom,0xe2d4+(0x24+n)*4);
 int oy=y+(int16_t)word(rom,0xe2d6+(0x24+n)*4);
 for(unsigned q=0;q<4;++q){
  unsigned slot=40+q,xx=(unsigned)(ox+(int)(q&1)*8)&511;
  unsigned yy=(unsigned)(oy+(int)(q>>1)*8)&255;
  p.oam[slot*2]=(uint16_t)((xx&255)|(yy<<8));
  p.highOam[slot/4]=(uint8_t)((p.highOam[slot/4]&~(3u<<((slot&3)*2)))|
      ((xx>>8)<<((slot&3)*2)));
 }
}
static int is_corner(unsigned x,unsigned y,unsigned w,unsigned h){
 return ((y==0||y==h-1)&&(x<5||x>=w-5))||
        ((x==0||x==w-1)&&(y<5||y>=h-5));
}
static void geometry(const uint8_t original[32]){
 for(unsigned mode=0;mode<2;++mode){
  memcpy(rom+0xb6f0,original,32);if(mode)patch_qol();
  for(unsigned n=0;n<12;++n)for(unsigned extension=0;extension<2;++extension){
   pose(n);LagoonSwordVisual_SetRangeCue(1);
   uint8_t before[sizeof ram],guest[PPU_SAVESTATE_MEM_SIZE];
   memcpy(before,ram,sizeof ram);memcpy(guest,p.cgram,sizeof guest);
   assert(LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,extension));
   assert(p.extraObjectCount==1+extension);
   const PpuExtraObject *o=&p.extraObjects[p.extraObjectCount-1];
   unsigned at=0xb6f0+(n/3)*8;
   int left=(int16_t)word(rom,at),right=(int16_t)word(rom,at+2);
   int top=(int16_t)word(rom,at+4),bottom=(int16_t)word(rom,at+6);
   assert(o->pixels==range_image.pixels&&o->x==128+left&&o->y==112+top);
   assert(o->width==right-left+1&&o->height==bottom-top+1&&o->stride==48);
   assert(o->width<=48&&o->height<=48&&o->priority==2&&o->math==0&&o->order==2);
   assert(o->oamSlot>=40&&o->oamSlot<=43);
   unsigned colored=0;
   for(unsigned y=0;y<48;++y)for(unsigned x=0;x<48;++x){
    unsigned value=o->pixels[y*48+x];
    int expected=x<o->width&&y<o->height&&is_corner(x,y,o->width,o->height);
    assert(!!(value&0x8000)==expected);if(expected){assert(value==0xa71f);colored++;}
    edge_checks++;
   }
   assert(colored==36&&!memcmp(before,ram,sizeof ram)&&!memcmp(guest,p.cgram,sizeof guest));
   assert(LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,extension)&&p.extraObjectCount==1+extension);
   LagoonSwordVisual_SetRangeCue(0);
   assert(LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,extension)==!!extension);
   assert(p.extraObjectCount==extension);
   assert(!LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,0)&&!p.extraObjectCount);cue_cases++;
  }
 }
 memcpy(rom+0xb6f0,original,32);
}
static void guards(const uint8_t original[32]){
 LagoonSwordVisual_SetRangeCue(1);
 for(unsigned i=0;i<51+32;++i){
  unsigned at=i<51?0xb6bd+i:0xb6f0+i-51;rom[at]^=1;pose(2);
  assert(!LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,0)&&!p.extraObjectCount);
  rom[at]^=1;refusals++;
 }
 for(unsigned i=0;i<12;++i){
  pose(2);assert(LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,0)&&p.extraObjectCount==1);
  switch(i){
   case 0:p.inidisp=14;break;case 1:p.obsel=3;break;case 2:p.bgmode=7;break;
   case 3:ram[0x100e]=1;break;case 4:ram[0x41b]=1;break;case 5:ram[0x501]=0;break;
   case 6:ram[0x51d]=255;break;case 7:ram[0x51c]++;break;case 8:ram[0x50c]++;break;
   case 9:p.oam[40*2]++;break;case 10:p.vram[0x4000+25*16]^=1;break;
   case 11:ram[0x515]=1;break;
  }
  assert(!LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,0)&&!p.extraObjectCount);refusals++;
 }
 /* Copied presentation arrays must not retain stale owned cue payloads. */
 static PpuExtraObject copied[512];static uint16_t sentinel_pixel=0x83e0;
 PpuExtraObject sentinel={1,2,1,1,1,1,0,127,1,&sentinel_pixel};
 pose(2);PpuSetExtraObjects(&p,&sentinel,1);
 assert(LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,1)&&p.extraObjectCount==3);
 memcpy(copied,p.extraObjects,3*sizeof(*copied));PpuSetExtraObjects(&p,copied,3);
 LagoonSwordVisual_SetRangeCue(0);
 assert(!LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,0)&&p.extraObjectCount==1);
 assert(!memcmp(p.extraObjects,&sentinel,sizeof sentinel));
 LagoonSwordVisual_SetRangeCue(1);
 for(unsigned base=510;base<=512;++base){
  pose(2);for(unsigned i=0;i<base;++i)copied[i]=sentinel;
  PpuSetExtraObjects(&p,copied,base);
  assert(LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,1)==(base<512));
  assert(p.extraObjectCount==512);
  for(unsigned i=0;i<base;++i)assert(!memcmp(&p.extraObjects[i],&sentinel,sizeof sentinel));
 }
 LagoonSwordRangeImage out;assert(!LagoonSwordRange_Build(rom,sizeof rom,4,0,0,&out));
 assert(!LagoonSwordRange_Build(rom,sizeof rom-1,0,0,0,&out));
 memcpy(rom+0xb6f0,original,32);
}
static void clipping_and_depth(void){
 patch_qol();LagoonSwordVisual_SetRangeCue(1);
 const int positions[4][3]={{2,250,112},{3,128,220},{6,5,112},{9,128,4}};
 for(unsigned fast=0;fast<2;++fast)for(unsigned f=0;f<4;++f){
  move_hero(positions[f][0],positions[f][1],positions[f][2]);
  assert(LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,0));
  const PpuExtraObject *o=&p.extraObjects[0];scan(fast);unsigned colored=0;
  for(unsigned y=0;y<224;++y)for(unsigned x=0;x<256;++x)if(rendered[y*256+x]==0xffc64a){
   int xx=(int)x-o->x,yy=(int)y-o->y;
   assert(xx>=0&&yy>=0&&xx<o->width&&yy<o->height&&is_corner(xx,yy,o->width,o->height));
   colored++;clipped_pixels++;
  }
  assert(colored&&colored<36);
 }
 for(unsigned fast=0;fast<2;++fast){
  pose(2);assert(LagoonSwordVisual_Prepare(&p,rom,sizeof rom,ram,0));scan(fast);
  unsigned at=0;while(at<256*224&&rendered[at]!=0xffc64a)at++;
  assert(at<256*224);unsigned x=at%256,y=at/256;
  p.screenEnabled[0]=17;p.bgXsc[0]=0x10;p.cgram[1]=31<<10;
  for(unsigned i=0;i<1024;++i)p.vram[0x1000+i]=0x2001;
  for(unsigned i=0;i<8;++i)p.vram[16+i]=0x00ff;
  scan(fast);assert(rendered[y*256+x]==0xff);
  for(unsigned i=0;i<1024;++i)p.vram[0x1000+i]=1;
  scan(fast);assert(rendered[y*256+x]==0xffc64a);
  p.windowsel=2u<<16;p.window1left=(uint8_t)x;p.window1right=(uint8_t)x;p.screenWindowed[0]=16;
  scan(fast);assert(rendered[y*256+x]==0xff);
 }
}
int main(int argc,char **argv){
 assert(!sword_visual_existing_main(argc,argv));
 uint8_t original[32];memcpy(original,rom+0xb6f0,32);
 geometry(original);guards(original);clipping_and_depth();
 LagoonSwordVisual_SetRangeCue(0);memcpy(rom+0xb6f0,original,32);
 printf("PASS sword cue: %u stock/QoL pose+extension cases, %u exact corner pixels, %u source/scene refusals, %u clipped PPU pixels; no guest writes, toggle/copied-list cleanup, capacity, legacy/fast foreground+windows\n",cue_cases,edge_checks,refusals,clipped_pixels);
 return 0;
}
