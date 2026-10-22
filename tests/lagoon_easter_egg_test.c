/* Meaningful read-only host-cameo policy tests, using an external original
 * ROM and an existing immutable Atland checkpoint. No guest assets embedded. */
#include "../src/lagoon_easter_egg.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "snes/interp816.h"
static uint8_t rom[0x100000],ram[0x20000],before[0x20000];
static Ppu ppu;
static uint16_t font_vram[32768];
static uint32_t frame[800*224],copy[800*224];
static unsigned assertions;
#define CHECK(x) do{assert(x);assertions++;}while(0)
static void put(unsigned a,unsigned value){ram[a]=(uint8_t)value;ram[a+1]=(uint8_t)(value>>8);}
/* Independent placement oracle: execute the actual original $01:8385 tile
 * probe, on this test-only RAM, rather than trusting our nav implementation. */
static unsigned oracle_product;
int interp816_opcode_hook(uint32_t address){(void)address;return 0;}
static uint8_t oracle_read(void *context,uint32_t address){
  (void)context;unsigned bank=address>>16,adr=address&65535;
  if(bank==0x7e||bank==0x7f)return ram[(bank-0x7e)*65536+adr];
  if((bank&127)<64&&adr<8192)return ram[adr];
  if((bank&127)<64&&(adr==0x4216||adr==0x4217))return oracle_product>>((adr-0x4216)*8);
  if((bank&127)<64&&(adr==0x4214||adr==0x4215))return (ram[0x4206]?word(ram,0x4204)/ram[0x4206]:65535)>>((adr-0x4214)*8);
  assert((bank&127)<32&&adr>=32768);return rom[(bank&31)*32768+(adr&32767)];
}
static void oracle_write(void *context,uint32_t address,uint8_t value){
  (void)context;unsigned bank=address>>16,adr=address&65535;
  if(bank==0x7e||bank==0x7f){ram[(bank-0x7e)*65536+adr]=value;return;}
  if((bank&127)<64&&adr>=0x4202&&adr<=0x4206){ram[adr]=value;oracle_product=ram[0x4202]*ram[0x4203];return;}
  assert((bank&127)<64&&adr<8192);ram[adr]=value;
}
static void check_original_placement(void){
  static const int8_t points[][2]={{-10,-9},{0,-9},{10,-9},{-10,0},{0,0},
    {10,0},{-10,9},{0,9},{10,9},{-24,0},{24,0},{0,-24},{0,24},{0,32}};
  memcpy(before,ram,sizeof ram);
  for(unsigned point=0;point<sizeof points/sizeof *points;point++){
    put(0xb8,X+points[point][0]);put(0xba,Y+points[point][1]);
    Interp816 *cpu=interp816_init(NULL,oracle_read,oracle_write);assert(cpu);
    cpu->pc=0x8385;cpu->k=cpu->db=1;cpu->dp=0;cpu->sp=0x1d0;cpu->a=cpu->x=cpu->y=0;
    cpu->mf=1;cpu->xf=0;cpu->e=cpu->d=cpu->c=cpu->n=cpu->v=cpu->z=0;cpu->i=1;
    ram[0x1d1]=0xef;ram[0x1d2]=0x7f;unsigned steps=0;
    while(cpu->k!=1||cpu->pc!=0x7ff0){assert(++steps<100000);interp816_runOpcode(cpu);}
    CHECK(cpu->sp==0x1d2);CHECK(!(ram[0x2c1]&128));
    CHECK((ram[0x2c0]&15)==0||(ram[0x2c0]&15)==3||(ram[0x2c0]&15)==5);
    interp816_free(cpu);
  }
  memcpy(ram,before,sizeof ram);
}
static void setup(void){
  put(0x48e,0);memcpy(ram+0x50,s.descriptor,56);ram[0x500]=0xc0;ram[0x501]=0;
  ram[0x100e]=ram[0x41b]=ram[0x417]=0;put(0x502,X);put(0x504,Y+32);put(0x514,0);put(0x520,18);ram[0x50a]=3;
  put(0x480,1088);put(0x482,896);
  memset(&ppu,0,sizeof ppu);memcpy(ppu.vram,font_vram,sizeof font_vram);ppu.inidisp=15;ppu.bgmode=9;ppu.obsel=2;
  ppu.bgXsc[0]=0x71;ppu.bgXsc[1]=0x79;ppu.bgXsc[2]=0x3c;ppu.bgTileAdr=0x600;
  ppu.screenEnabled[0]=31;ppu.hScroll[0]=ppu.hScroll[1]=1088;
  ppu.vScroll[0]=ppu.vScroll[1]=896;
}
static void capture(uint64_t generation){
  LagoonEasterEgg_BeginFrame(generation);
  for(unsigned row=1;row<=224;row++)LagoonEasterEgg_ObserveRaster(&ppu,row,ram,generation,0);
  LagoonEasterEgg_EndFrame(generation);LagoonEasterEgg_Observe(ram,sizeof ram,generation,1);
}
static void fresh(void){LagoonEasterEgg_Init(rom,sizeof rom);setup();capture(1);LagoonEasterEgg_Draw((uint8_t*)frame,256*4,256,224,0,1);}
static unsigned tick;
static void advance(void){tick+=10000;LagoonEasterEgg_Tick(0,tick);LagoonEasterEgg_Tick(SNES_PAD_A,tick);}
int main(int argc,char **argv){
  assert(argc==4);FILE *f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  f=fopen(argv[2],"rb");assert(f&&fread(ram,1,sizeof ram,f)==sizeof ram);fclose(f);
  f=fopen(argv[3],"rb");assert(f&&fread(font_vram,1,sizeof font_vram,f)==sizeof font_vram);fclose(f);
  check_original_placement();
  fresh();CHECK(s.qualified&&s.valid&&s.near&&s.complete.valid&&s.complete.lines==224);
  capture(1);CHECK(!LagoonEasterEgg_TryInteract(SNES_PAD_Y)); /* no Draw yet */
  CHECK(LagoonEasterEgg_Draw((uint8_t*)frame,256*4,256,224,0,1));
  CHECK(!LagoonEasterEgg_TryInteract(SNES_PAD_Y)); /* held edge stays silent */
  LagoonEasterEgg_TryInteract(0);CHECK(LagoonEasterEgg_TryInteract(SNES_PAD_Y));LagoonEasterEgg_Close();
  fresh();CHECK(!LagoonEasterEgg_Draw((uint8_t*)frame,256*4,256,224,0,2));CHECK(!LagoonEasterEgg_TryInteract(SNES_PAD_Y));
  fresh();put(0x480,560);put(0x482,128);ppu.hScroll[0]=ppu.hScroll[1]=560;ppu.vScroll[0]=ppu.vScroll[1]=128;capture(1);
  CHECK(s.valid&&s.near&&s.complete.valid); /* incoherent large diagnostic teleport */
  CHECK(!LagoonEasterEgg_Draw((uint8_t*)frame,256*4,256,224,0,1));CHECK(!LagoonEasterEgg_TryInteract(SNES_PAD_Y));
  fresh();CHECK(!LagoonEasterEgg_Draw((uint8_t*)frame,256*4-1,256,224,0,1));CHECK(!LagoonEasterEgg_TryInteract(SNES_PAD_Y));
  fresh();
  memcpy(before,ram,sizeof ram);uint64_t rom_before=source_hash(rom,sizeof rom,0x008000,sizeof rom);
  for(unsigned width=256;width<=800;width+=2){
    for(unsigned i=0;i<800*224;i++)frame[i]=0x12345678;
    CHECK(LagoonEasterEgg_Draw((uint8_t*)frame,width*4,width,224,0,1));
    unsigned changed=0,opaque=0,margin=(width-256)/2;
    for(unsigned yy=0;yy<224;yy++)for(unsigned xx=0;xx<width;xx++){
      int sx=(int)xx-(int)margin-X+s.complete.x+10,sy=(int)yy-Y+s.complete.y+35;
      if(sx>=0&&sx<20&&sy>=0&&sy<35){unsigned index=cameo_frames[0][sy*20+sx];
        if(index){CHECK(frame[yy*width+xx]==cameo_palette[index]);opaque++;}}
      if(frame[yy*width+xx]!=0x12345678)changed++;
    }
    CHECK(opaque>100&&changed>=opaque);
  }
  CHECK(!memcmp(before,ram,sizeof ram)&&rom_before==source_hash(rom,sizeof rom,0x008000,sizeof rom));
  /* A foreground player cell keeps native depth order at the visitor's feet. */
  fresh();int body_x=0,body_y=0;
  CHECK(LagoonEasterEgg_CollisionBody(ram,1,&body_x,&body_y)&&body_x==X&&body_y==Y);
  CHECK(!LagoonEasterEgg_CollisionBody(ram,2,&body_x,&body_y));
  int left=X-s.complete.x-10,top=Y-s.complete.y-35;
  ppu.oam[0]=(uint16_t)((top+27)<<8|(left+6));ppu.oam[1]=0;
  for(unsigned y=0;y<8;y++)ppu.vram[0x4000+y]=255;
  capture(1);unsigned masked=0;for(unsigned i=0;i<20*35;i++)masked+=s.complete.foreground[i];CHECK(masked==64);
  for(unsigned i=0;i<256*224;i++)frame[i]=0xff123456;
  CHECK(LagoonEasterEgg_Draw((uint8_t*)frame,256*4,256,224,0,1));
  for(unsigned y=27;y<35;y++)for(unsigned x=6;x<14;x++)CHECK(frame[(top+y)*256+left+x]==0xff123456);
  fresh();
  /* Actual source colors animate; no stale/current-camera substitutes. */
  LagoonEasterEgg_Draw((uint8_t*)frame,256*4,256,224,0,1);memcpy(copy,frame,256*224*4);
  LagoonEasterEgg_Draw((uint8_t*)frame,256*4,256,224,360,1);CHECK(memcmp(copy,frame,256*224*4)!=0);
  for(unsigned mutation=0;mutation<13;mutation++){
    fresh();LagoonEasterEgg_BeginFrame(1);
    for(unsigned row=1;row<=224;row++){
      Ppu use=ppu;
      if(row==100)switch(mutation){
        case 0:use.inidisp=14;break;case 1:use.mosaic=15;break;case 2:use.windowsel=1;break;
        case 3:use.hScroll[0]++;break;case 4:use.vScroll[0]=use.vScroll[1]=897;break;
        case 5:use.bgmode=1;break;case 6:use.bgXsc[0]^=1;break;case 7:use.screenEnabled[0]=0;break;
        case 8:use.setini=1;break;case 9:use.cgadsub=1;break;case 10:use.oamaddh=128;break;
        case 11:use.hScroll[2]=1;break;case 12:use.screenWindowed[1]=1;break;
      }
      LagoonEasterEgg_ObserveRaster(&use,row,ram,1,0);
    }
    LagoonEasterEgg_EndFrame(1);CHECK(!s.complete.valid&&!LagoonEasterEgg_TryInteract(SNES_PAD_Y));
  }
  fresh();LagoonEasterEgg_BeginFrame(1);
  for(unsigned row=1;row<=224;row++)LagoonEasterEgg_ObserveRaster(&ppu,row,ram,1,row==100);
  LagoonEasterEgg_EndFrame(1);CHECK(!s.complete.valid);
  fresh();LagoonEasterEgg_BeginFrame(1);
  for(unsigned row=1;row<=224;row++)if(row!=120)LagoonEasterEgg_ObserveRaster(&ppu,row,ram,1,0);
  LagoonEasterEgg_EndFrame(1);CHECK(!s.complete.valid);
  for(unsigned mutation=0;mutation<10;mutation++){
    fresh();switch(mutation){
      case 0:put(0x48e,9);break;case 1:ram[0x100e]=1;break;case 2:ram[0x41b]=1;break;
      case 3:ram[0x417]=2;break;case 4:ram[0x50]^=1;break;case 5:put(0x520,0);break;
      case 6:ram[0x500]=0;break;case 7:ram[0x501]=1;break;case 8:put(0x502,65535);break;
      case 9:ram[0x78]^=1;break;
    }
    LagoonEasterEgg_Observe(ram,sizeof ram,1,1);CHECK(!s.valid&&!LagoonEasterEgg_TryInteract(SNES_PAD_Y));
  }
  fresh();put(0x502,X-32);put(0x504,Y);ram[0x50a]=2;LagoonEasterEgg_Observe(ram,sizeof ram,1,1);CHECK(!s.near);
  ram[0x50a]=0;LagoonEasterEgg_Observe(ram,sizeof ram,1,1);CHECK(s.near);
  put(0x514,1);LagoonEasterEgg_Observe(ram,sizeof ram,1,1);CHECK(!s.near);
  fresh();ram[0x540]=0xc0;put(0x542,X+16);put(0x544,Y);LagoonEasterEgg_Observe(ram,sizeof ram,1,1);CHECK(!s.valid);
  ram[0x540]=0;fresh();unsigned map=0x14000+(Y>>4)*128+(X>>4);uint8_t old=ram[map];ram[map]=0;
  /* Force an actual solid original metatile at the fixed site. */
  unsigned solid=0;while(solid<256){unsigned at=3*32768+solid*4;if((rom[at]&128)&&(rom[at+1]&128)&&(rom[at+2]&128)&&(rom[at+3]&128))break;solid++;}
  CHECK(solid<256);ram[map]=(uint8_t)solid;LagoonEasterEgg_Observe(ram,sizeof ram,1,1);CHECK(!s.valid);ram[map]=old;
  fresh();CHECK(LagoonDialogueFont_Ready());
  memset(frame,0x12,sizeof frame);LagoonEasterEgg_Draw((uint8_t*)frame,256*4,256,224,0,1);
  for(unsigned y=0;y<224;y++)for(unsigned x=0;x<256;x++){
    int sx=(int)x-X+s.complete.x+10,sy=(int)y-Y+s.complete.y+35;
    if(sx<0||sx>=20||sy<0||sy>=35)CHECK(frame[y*256+x]==0x12121212); /* no prompt */
  }
  ppu.vram[0x6000+0x1e2*8]^=1;capture(1);
  CHECK(LagoonDialogueFont_Ready()); /* unrelated NPC/event art must not block talking */
  fresh();ppu.vram[0x6000]^=1;capture(1);LagoonEasterEgg_Draw((uint8_t*)frame,256*4,256,224,0,1);
  CHECK(!LagoonDialogueFont_Ready()&&!LagoonEasterEgg_TryInteract(SNES_PAD_Y));
  fresh();CHECK(LagoonEasterEgg_TryInteract(SNES_PAD_Y));LagoonEasterEgg_PollOpen(SNES_PAD_Y);
  LagoonEasterEgg_Tick(0,100);CHECK(s.revealed==0);LagoonEasterEgg_Tick(0,249);CHECK(s.revealed==2);
  LagoonEasterEgg_Tick(SNES_PAD_A,250);CHECK(s.page==0&&s.revealed==strlen(dialogue[0]));
  CHECK(!LagoonEasterEgg_TakeDiscovery(NULL));LagoonEasterEgg_Close();
  fresh();CHECK(!LagoonEasterEgg_TakeDiscovery(NULL));CHECK(LagoonEasterEgg_TryInteract(SNES_PAD_Y));
  CHECK(LagoonEasterEgg_IsOpen()&&LagoonEasterEgg_PollOpen(SNES_PAD_Y));
  LagoonEasterEgg_Tick(SNES_PAD_Y,0);CHECK(s.page==0);advance();CHECK(s.page==1&&!LagoonEasterEgg_TakeDiscovery(NULL));
  tick+=10000;LagoonEasterEgg_Tick(0,tick);
  const uint32_t *image=NULL;int width=0,height=0;CHECK(LagoonEasterEgg_Image(&image,&width,&height)&&image&&width==512&&height==448);
  FILE *preview=fopen("dialogue.ppm","wb");assert(preview);fprintf(preview,"P6\n%d %d\n255\n",width,height);
  for(int i=0;i<width*height;i++){uint32_t c=image[i];fputc(c>>16&255,preview);fputc(c>>8&255,preview);fputc(c&255,preview);}fclose(preview);
  advance();uint64_t discovered=0;CHECK(!LagoonEasterEgg_IsOpen()&&LagoonEasterEgg_TakeDiscovery(&discovered)&&discovered==1);
  CHECK(!LagoonEasterEgg_TakeDiscovery(NULL));
  fresh();LagoonEasterEgg_TryInteract(SNES_PAD_Y);LagoonEasterEgg_PollOpen(SNES_PAD_Y);LagoonEasterEgg_Tick(SNES_PAD_B,0);CHECK(!LagoonEasterEgg_IsOpen()&&!LagoonEasterEgg_TakeDiscovery(NULL));
  fresh();LagoonEasterEgg_TryInteract(SNES_PAD_Y);LagoonEasterEgg_PollOpen(SNES_PAD_Y);advance();LagoonEasterEgg_Observe(ram,sizeof ram,2,1);CHECK(!LagoonEasterEgg_IsOpen()&&!LagoonEasterEgg_TakeDiscovery(NULL)&&!s.complete.valid);
  fresh();memcpy(before,ram,sizeof ram);capture(1);LagoonEasterEgg_Draw((uint8_t*)frame,256*4,256,224,0,1);LagoonEasterEgg_TryInteract(SNES_PAD_Y);LagoonEasterEgg_PollOpen(SNES_PAD_Y);advance();advance();LagoonEasterEgg_Image(&image,&width,&height);LagoonEasterEgg_TakeDiscovery(&discovered);CHECK(!memcmp(before,ram,sizeof ram));
  printf("PASS cameo %u assertions: original8385 collision14 points,source-colored animation all273 even widths256..800,224-row raster/fade/HDMA/menu/generation refusals,near-facing deliberateY,final-page-only one-shot event,cancel/load silence; host module guest RAM/ROM unchanged\n",assertions);
  return 0;
}
