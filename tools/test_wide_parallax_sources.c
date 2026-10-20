/* Own verified original ROM required; no asset bytes are embedded.
 * Full original018A92/009143/00908A/0087B4 execute using actual native NMI
 * frames and original RTI. Hardware DMA alone is modeled; no timing claim.
 * Independent original-loader pages then qualify the real production helper. */
#include "lagoon_wide_entities.h"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
static uint8_t rom[0x100000],ram[0x20000],io[0x10000],vram[65536];
static unsigned source,maximum,jobs,page,scene,current;static Interp816 *maincpu;
static const unsigned rooms[]={7,0xc,0x12,0x13,0x28,0x29,0x2a};
static uint16_t original_pages[7][2048];
static unsigned sources[7][2],lengths[7][2];
static unsigned byte_mutants,source_mutants,room_refusals,packet_refusals;
static unsigned word(const uint8_t*p,unsigned a){return p[a]|p[a+1]<<8;}
static unsigned off(unsigned a){return (a>>16)*32768+(a&32767);}
static uint64_t fnv(const uint8_t*p,unsigned n){uint64_t h=UINT64_C(14695981039346656037);while(n--)h=(h^*p++)*UINT64_C(1099511628211);return h;}
static uint8_t readb(void*ctx,uint32_t a){(void)ctx;unsigned b=a>>16,w=a&65535;
 if(b==126||b==127)return ram[(b-126)*65536+w];
 if((b&127)<64&&w<8192)return ram[w];
 if((b&127)<64&&w<32768)return io[w];
 assert((b&127)<32&&w>=32768);
 if(a>=source&&a<source+0x3000&&a>maximum)maximum=a;
 return rom[off(a)];}
static void writeb(void*ctx,uint32_t a,uint8_t v){(void)ctx;unsigned b=a>>16,w=a&65535;
 if(b==126||b==127){ram[(b-126)*65536+w]=v;return;}
 if((b&127)<64&&w<8192){ram[w]=v;return;}
 assert((b&127)<64&&w<32768);io[w]=v;
 if(w==0x420b){assert(v==2&&io[0x4310]==1&&io[0x4311]==0x18);
  unsigned from=word(io,0x4312),bank=io[0x4314],to=word(io,0x2116),length=word(io,0x4315);
  assert(bank==127&&from==0x7f80&&length==2048&&to==0x7800+page*0x400);
  for(unsigned n=0;n<length;n++)vram[to*2+n]=readb(NULL,(bank<<16)|((from+n)&65535));
  assert(!memcmp(vram+to*2,ram+0x17f80,2048));
  unsigned length_source=maximum-source+1;assert(length_source>12&&length_source<0x3000);
  printf("PAGE %02X %u %06X %u %016llx %016llx\n",scene,page,source,length_source,
   (unsigned long long)fnv(rom+off(source),length_source),(unsigned long long)fnv(vram+to*2,2048));
  sources[current][page]=source;lengths[current][page]=length_source;
  jobs++;page++;
 }
}
static void initcpu(Interp816*c,unsigned bank,unsigned pc,unsigned s,unsigned depth){
 c->k=bank;c->pc=pc;c->db=bank;c->dp=0;c->sp=s;c->a=c->x=c->y=0;
 c->mf=1;c->xf=0;c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;
 ram[s+1]=0xef;ram[s+2]=0x7f;if(depth==3)ram[s+3]=bank;
}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);

 maincpu=interp816_init(NULL,readb,writeb);assert(maincpu);
 for(unsigned n=0;n<7;n++){current=n;scene=rooms[n];page=0;memset(ram,0,sizeof ram);memset(io,0,sizeof io);memset(vram,0,sizeof vram);
  memcpy(ram+0x50,rom+off(0x05b2df+scene*56),56);ram[0x48e]=scene;initcpu(maincpu,1,0x8a92,0x1d0,2);
  unsigned ops=0;while(maincpu->k!=1||maincpu->pc!=0x7ff0){
   assert(++ops<1000000);
   if(maincpu->k==0&&maincpu->pc==0x9143){source=ram[0xe0]|ram[0xe1]<<8|ram[0xe2]<<16;maximum=source;}
   /* Hardware NMI request lets the core create the real4-byte native frame,
    * read the actual ROM vector, execute full0087B4 and return via real RTI. */
   if(maincpu->k==0&&maincpu->pc==0x89ac&&(ram[0x23a]&0x20))maincpu->nmiWanted=true;
   interp816_runOpcode(maincpu);
  }
  assert(maincpu->sp==0x1d2&&page==2);
  for(unsigned w=0;w<2048;w++)original_pages[n][w]=word(vram,0xf000+w*2);
 }
 assert(jobs==14);interp816_free(maincpu);
 uint16_t resident[32768]={0};
 for(unsigned n=0;n<7;n++) {
  memcpy(resident+0x7800,original_pages[n],4096);
  assert(LagoonWideEntities_ParallaxMapForRoom(resident,rom,sizeof rom,rooms[n]));
  for(unsigned b=0;b<4096;b++) {
   ((uint8_t*)(resident+0x7800))[b]^=1;
   assert(!LagoonWideEntities_ParallaxMapForRoom(resident,rom,sizeof rom,rooms[n]));
   ((uint8_t*)(resident+0x7800))[b]^=1;byte_mutants++;
  }
  for(unsigned p=0;p<2;p++) {
   if(p && sources[n][0]==sources[n][1])continue;
   for(unsigned b=0;b<lengths[n][p];b++) {
    rom[off(sources[n][p])+b]^=1;
    assert(!LagoonWideEntities_ParallaxMapForRoom(resident,rom,sizeof rom,rooms[n]));
    rom[off(sources[n][p])+b]^=1;source_mutants++;
   }
  }
  for(unsigned r=0;r<256;r++) {
   int expected=0;
   for(unsigned j=0;j<7;j++)if(r==rooms[j] && !memcmp(original_pages[n],original_pages[j],4096))expected=1;
   assert(!!LagoonWideEntities_ParallaxMapForRoom(resident,rom,sizeof rom,r)==expected);
   room_refusals+=!expected;
  }
  for(unsigned p=0;p<2;p++) {
   uint16_t saved[1024];memcpy(saved,resident+0x7800+p*0x400,2048);
   memcpy(resident+0x7800+p*0x400,original_pages[(n+1)%7]+p*0x400,2048);
   int same=!memcmp(saved,resident+0x7800+p*0x400,2048);
   assert(!!LagoonWideEntities_ParallaxMapForRoom(resident,rom,sizeof rom,rooms[n])==same);
   memcpy(resident+0x7800+p*0x400,saved,2048);
  }
  uint16_t saved[1024];memcpy(saved,resident+0x7800,2048);
  memcpy(resident+0x7800,resident+0x7c00,2048);memcpy(resident+0x7c00,saved,2048);
  assert(!!LagoonWideEntities_ParallaxMapForRoom(resident,rom,sizeof rom,rooms[n])==
         !memcmp(original_pages[n],original_pages[n]+1024,2048));
  memcpy(resident+0x7800,original_pages[n],4096);
  assert(!LagoonWideEntities_ParallaxMapForRoom(NULL,rom,sizeof rom,rooms[n]));
  assert(!LagoonWideEntities_ParallaxMapForRoom(resident,NULL,sizeof rom,rooms[n]));
  assert(!LagoonWideEntities_ParallaxMapForRoom(resident,rom,sizeof rom-1,rooms[n]));
  assert(!LagoonWideEntities_ParallaxMapForRoom(resident,rom,sizeof rom+1,rooms[n]));
  unsigned camera[4]={362,614,90,153},shifted[4]={366,618,91,154};
  LagoonWideParallaxPacket packet={0};
  assert(LagoonWideParallaxPacket_CaptureRoom(&packet,resident,rom,sizeof rom,31,camera,rooms[n]));
  assert(LagoonWideParallaxPacket_Match(&packet,resident,rom,sizeof rom,31,camera,shifted));
  assert(!LagoonWideParallaxPacket_Match(&packet,resident,rom,sizeof rom,32,camera,shifted));packet_refusals++;
  for(unsigned w=0;w<2048;w++) {
   packet.pages[w]^=1;
   assert(!LagoonWideParallaxPacket_Match(&packet,resident,rom,sizeof rom,31,camera,shifted));
   packet.pages[w]^=1;packet_refusals++;
  }
  for(unsigned axis=0;axis<4;axis++) {
   unsigned old=camera[axis];camera[axis]^=4;
   assert(!LagoonWideParallaxPacket_Match(&packet,resident,rom,sizeof rom,31,camera,shifted));
   camera[axis]=old;packet_refusals++;
  }
  packet.valid=0;assert(!LagoonWideParallaxPacket_Match(&packet,resident,rom,sizeof rom,31,camera,shifted));packet_refusals++;
 }
 printf("PASS production room/page contracts: %u decoded-byte mutants, %u compressed-source mutants, %u incompatible/unsupported room refusals, %u retained-packet refusals\n",byte_mutants,source_mutants,room_refusals,packet_refusals);
 unsigned raster_checks=0;
 const unsigned anchors[]={0,3,508,511,512,1020,1023,1024,1536,1824,2047};
 for(unsigned n=0;n<7;n++)for(unsigned ax=0;ax<11;ax++)for(unsigned ay=0;ay<11;ay++) {
  unsigned x=anchors[ax],y=anchors[ay],camera[4]={x,y,x>>2,y>>2};
  unsigned limit=rooms[n]==7?1024:2048;
  int allowed=x<limit&&y<limit;
  assert(!!LagoonWideEntities_ParallaxRoomScroll(rooms[n],x,y,x>>2,y>>2)==allowed);raster_checks++;
  if(!allowed)continue;
  memcpy(resident+0x7800,original_pages[n],4096);LagoonWideParallaxPacket packet={0};
  assert(LagoonWideParallaxPacket_CaptureRoom(&packet,resident,rom,sizeof rom,31,camera,rooms[n]));
  for(int phase=-8;phase<=8;phase++) {
   int fullx=(int)x+phase,fully=(int)y-phase;
   unsigned raster[4]={(unsigned)fullx&1023,(unsigned)fully&1023,(unsigned)fullx>>2,(unsigned)fully>>2};
   int inbounds=fullx>=0&&fully>=0&&fullx<2048&&fully<2048;
   assert(!!LagoonWideEntities_ParallaxRaster(camera,raster)==inbounds);
   assert(!!LagoonWideParallaxPacket_Match(&packet,resident,rom,sizeof rom,31,camera,raster)==inbounds);raster_checks+=2;
   if(!inbounds)continue;
   for(unsigned axis=2;axis<4;axis++) {
    raster[axis]^=1;assert(!LagoonWideEntities_ParallaxRaster(camera,raster));
    assert(!LagoonWideParallaxPacket_Match(&packet,resident,rom,sizeof rom,31,camera,raster));raster[axis]^=1;raster_checks+=2;
   }
  }
 }
 assert(!LagoonWideEntities_ParallaxRoomScroll(0x26,0,0,0,0));
 assert(!LagoonWideEntities_ParallaxRoomScroll(0x12,2048,0,512,0));
 assert(!LagoonWideEntities_ParallaxRaster(NULL,anchors));
 printf("PASS full-camera/wrapped-raster quarter contract: %u bounded anchor/phase/mutation checks, old07bound preserved\n",raster_checks);
 puts("PASS original018A92+009143+00908A+0087B4 full true-frame calls:7 scenes,14 modeled DMA jobs; source/VRAM equality, no timing claim");return 0;
}
