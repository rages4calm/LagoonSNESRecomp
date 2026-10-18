/* Own clean ROM required; no copied asset bytes are embedded in this test. */
#include "lagoon_wide_entities.h"
#include "lagoon_menu_decode.h"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t rom[0x100000],ram[0x20000],fast[0x20000];
static uint16_t vram[32768];
static LagoonWideRetainedMap map;
static uint8_t queue_source[136];
static void queue_job(LagoonWideParallaxQueue *queue,unsigned count,uint64_t generation) {
  for(unsigned n=0;n<count;++n) {
    unsigned a=queue_source[(n/2)*4]|queue_source[(n/2)*4+1]<<8;
    assert(LagoonWideParallaxQueue_Write(queue,a*2+(n&1),queue_source[(n/2)*4+2+(n&1)],generation));
  }
}
static uint8_t read_byte(void *m,uint32_t a) {
  (void)m;unsigned bank=a>>16,off=a&65535;
  if(bank==126 || bank==127)return ram[(bank-126)*65536+off];
  if((bank&127)<64 && off<8192)return ram[off];
  assert((bank&127)<32 && off>=32768);
  return rom[(bank&31)*32768+(off&32767)];
}
static void write_byte(void *m,uint32_t a,uint8_t v) {
  (void)m;unsigned bank=a>>16,off=a&65535;
  if(bank==126 || bank==127){ram[(bank-126)*65536+off]=v;return;}
  assert((bank&127)<64 && off<8192);ram[off]=v;
}
int main(int argc,char **argv) {
  assert(argc==2);FILE *f=fopen(argv[1],"rb");
  assert(f && fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);
  ram[0xE0]=0x7D;ram[0xE1]=0xFB;ram[0xE2]=0x11;
  ram[0x1D1]=0xFF;ram[0x1D2]=0x0F;ram[0x1D3]=0;
  memcpy(fast,ram,sizeof(ram));
  CpuState c={0};c.S=0x1D0;c.m_flag=1;c.P=0x24;
  unsigned consumed=0,produced=0;
  assert(LagoonMenuDecode_Expand(rom,sizeof(rom),fast,&c,&consumed,&produced));
  assert(consumed==761 && produced==2052);
  Interp816 *i=interp816_init(NULL,read_byte,write_byte);assert(i);
  i->pc=0x9143;i->sp=0x1D0;i->mf=1;i->xf=0;i->e=0;i->i=1;
  unsigned steps=0;
  while(i->k || i->pc!=0x1000){assert(++steps<1000000);interp816_runOpcode(i);}
  assert(!memcmp(ram,fast,sizeof(ram)));interp816_free(i);
  for(unsigned page=0;page<2;++page)for(unsigned n=0;n<1024;++n)
    vram[0x7800+page*0x400+n]=ram[0x17F80+n*2]|ram[0x17F81+n*2]<<8;
  assert(LagoonWideEntities_ParallaxMap(vram,rom,sizeof(rom)));
  assert(!LagoonWideEntities_ParallaxMap(vram,rom,0x8FB7D+760));
  assert(!LagoonWideEntities_ParallaxMap(NULL,rom,sizeof(rom)));
  assert(!LagoonWideEntities_ParallaxMap(vram,NULL,sizeof(rom)));
  for(unsigned page=0;page<2;++page) {
    for(unsigned n=0;n<1024;++n) {
      vram[0x7800+page*0x400+n]^=1;
      assert(!LagoonWideEntities_ParallaxMap(vram,rom,sizeof(rom)));
      vram[0x7800+page*0x400+n]^=1;
    }
  }
  for(unsigned n=0;n<761;++n) {
    rom[0x8FB7D+n]^=1;assert(!LagoonWideEntities_ParallaxMap(vram,rom,sizeof(rom)));
    rom[0x8FB7D+n]^=1;
  }
  for(unsigned n=0;n<1024;++n) {
    assert(LagoonWideEntities_ParallaxScroll(n,1023-n,n>>2,(1023-n)>>2));
    assert(!LagoonWideEntities_ParallaxScroll(n,1023-n,(n>>2)^1,(1023-n)>>2));
    assert(!LagoonWideEntities_ParallaxScroll(n,1023-n,n>>2,((1023-n)>>2)^1));
  }
  assert(!LagoonWideEntities_ParallaxScroll(1024,0,256,0));
  assert(!LagoonWideEntities_ParallaxScroll(0,1024,0,256));
  LagoonWideParallaxPacket packet={0};
  unsigned camera[4]={362,614,90,153};
  assert(LagoonWideParallaxPacket_Capture(&packet,vram,rom,sizeof(rom),27,camera));
  assert(LagoonWideParallaxPacket_Match(&packet,vram,rom,sizeof(rom),27,camera,camera));
  assert(!LagoonWideParallaxPacket_Match(&packet,vram,rom,sizeof(rom),28,camera,camera));
  for(unsigned n=0;n<0x800;++n) {
    vram[0x7800+n]^=1;
    assert(!LagoonWideParallaxPacket_Match(&packet,vram,rom,sizeof(rom),27,camera,camera));
    vram[0x7800+n]^=1;
    packet.pages[n]^=1;
    assert(!LagoonWideParallaxPacket_Match(&packet,vram,rom,sizeof(rom),27,camera,camera));
    packet.pages[n]^=1;
  }
  for(unsigned n=0;n<761;++n) {
    rom[0x8FB7D+n]^=1;
    assert(!LagoonWideParallaxPacket_Match(&packet,vram,rom,sizeof(rom),27,camera,camera));
    rom[0x8FB7D+n]^=1;
  }
  for(unsigned n=0;n<4;++n) {
    camera[n]^=4;
    assert(!LagoonWideParallaxPacket_Match(&packet,vram,rom,sizeof(rom),27,camera,camera));
    assert(!LagoonWideParallaxPacket_Capture(&packet,vram,rom,sizeof(rom),27,camera));
    assert(!packet.valid);camera[n]^=4;
    assert(LagoonWideParallaxPacket_Capture(&packet,vram,rom,sizeof(rom),27,camera));
    packet.camera[n]^=4;
    assert(!LagoonWideParallaxPacket_Match(&packet,vram,rom,sizeof(rom),27,camera,camera));
    packet.camera[n]^=4;
  }
  /* A coherently shifted quartet still cannot impersonate this packet's
   * enclosing retained camera witness. Real raster movement remains allowed. */
  packet.camera[0]+=4;packet.camera[1]+=4;packet.camera[2]++;packet.camera[3]++;
  assert(!LagoonWideParallaxPacket_Match(&packet,vram,rom,sizeof(rom),27,camera,camera));
  assert(LagoonWideParallaxPacket_Capture(&packet,vram,rom,sizeof(rom),27,camera));
  unsigned shifted[4]={366,618,91,154};
  assert(LagoonWideParallaxPacket_Match(&packet,vram,rom,sizeof(rom),27,camera,shifted));
  assert(!LagoonWideParallaxPacket_Capture(&packet,vram,rom,0x8FB7D+760,27,camera));
  assert(!packet.valid);
  assert(!LagoonWideParallaxPacket_Match(&packet,vram,rom,sizeof(rom),27,camera,camera));
  assert(!LagoonWideParallaxPacket_Capture(NULL,vram,rom,sizeof(rom),27,camera));
  assert(!LagoonWideParallaxPacket_Match(NULL,vram,rom,sizeof(rom),27,camera,camera));
  /* Synthetic original top-edge row. Explicit34-word circular addresses and
   * two authored tiles are independent of the production queue matcher. */
  map.definitions[0][0]=0x2c08;map.definitions[0][1]=0x2c09;
  for(unsigned n=0;n<34;++n) {
    unsigned a=n<20?0x758c+n:0x7180+n-20;
    queue_source[n*4]=(uint8_t)a;queue_source[n*4+1]=(uint8_t)(a>>8);
    queue_source[n*4+2]=(uint8_t)(n&1?9:8);queue_source[n*4+3]=0x2c;
  }
  LagoonWideParallaxQueue queue={0};
  assert(LagoonWideParallaxQueue_Begin(&queue,&map,queue_source,136,camera,27));
  assert(LagoonWideParallaxQueue_Pending(&queue));queue_job(&queue,67,27);
  assert(LagoonWideParallaxQueue_Pending(&queue));
  assert(LagoonWideParallaxQueue_Write(&queue,0x718d*2+1,0x2c,27));
  assert(!LagoonWideParallaxQueue_Pending(&queue) && queue.valid);
  assert(!LagoonWideParallaxQueue_Write(&queue,0x718d*2+1,0x2c,27));
  for(unsigned n=0;n<136;++n) {
    memset(&queue,0,sizeof(queue));queue_source[n]^=1;
    assert(!LagoonWideParallaxQueue_Begin(&queue,&map,queue_source,136,camera,27));
    assert(!queue.valid);queue_source[n]^=1;
  }
  for(unsigned n=0;n<68;++n) {
    memset(&queue,0,sizeof(queue));
    assert(LagoonWideParallaxQueue_Begin(&queue,&map,queue_source,136,camera,27));
    queue_job(&queue,n,27);
    unsigned a=queue_source[(n/2)*4]|queue_source[(n/2)*4+1]<<8;
    unsigned v=queue_source[(n/2)*4+2+(n&1)];
    assert(!LagoonWideParallaxQueue_Write(&queue,a*2+(n&1),v^1,27));assert(!queue.valid);
    assert(LagoonWideParallaxQueue_Begin(&queue,&map,queue_source,136,camera,27));
    queue_job(&queue,n,27);
    assert(!LagoonWideParallaxQueue_Write(&queue,a*2+(n&1),v,28));assert(!queue.valid);
  }
  assert(LagoonWideParallaxQueue_Begin(&queue,&map,queue_source,136,camera,27));
  assert(!LagoonWideParallaxQueue_Write(&queue,0x758c*2+1,0x2c,27)); /* swapped low/high */
  assert(LagoonWideParallaxQueue_Begin(&queue,&map,queue_source,136,camera,27));
  assert(LagoonWideParallaxQueue_Write(&queue,0x758c*2,8,27));
  assert(!LagoonWideParallaxQueue_Write(&queue,0x758c*2,8,27)); /* duplicated low */
  assert(LagoonWideParallaxQueue_Begin(&queue,&map,queue_source,136,camera,27));
  assert(!LagoonWideParallaxQueue_Write(&queue,0x758d*2,9,27)); /* missing pair */
  assert(LagoonWideParallaxQueue_Begin(&queue,&map,queue_source,136,camera,27));
  assert(!LagoonWideParallaxQueue_Begin(&queue,&map,queue_source,136,camera,27)); /* interrupted job */
  assert(!queue.valid && !LagoonWideParallaxQueue_Pending(&queue));
  assert(!LagoonWideParallaxQueue_Begin(&queue,&map,queue_source,135,camera,27));
  assert(!LagoonWideParallaxQueue_Begin(&queue,&map,queue_source,132,camera,27));
  assert(LagoonWideParallaxQueue_Begin(&queue,&map,queue_source,136,camera,27));
  queue_job(&queue,68,27);
  assert(LagoonWideParallaxQueue_Begin(&queue,&map,queue_source,136,camera,28));
  assert(!LagoonWideParallaxQueue_Write(&queue,0x758c*2,8,27)); /* previous timeline */
  puts("room07 parallax: original decoder/full128KiB, both map pages, every source/map-word mutation, quarter raster anchors PASS");
  puts("room07 outgoing packet: both complete pages/source, timeline, four captured/raster anchors and partial-source refusal PASS");
  puts("room07 original34-word top-edge queue: source/map/destination, all136 byte mutations, ordered68 writes, partial/duplicate/missing/reordered/next-job/timeline refusal PASS");
  return 0;
}
