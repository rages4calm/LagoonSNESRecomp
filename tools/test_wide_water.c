/* Source facts only; the owner supplies original USA ROM bytes at runtime. */
#include "lagoon_wide_entities.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t rom[0x100000];
static uint16_t vram[32768];
static unsigned off(unsigned pc){return ((pc>>16)&31)*0x8000+(pc&0x7fff);}
static void transfer(LagoonWideTorches *t,unsigned frame,int corrupt) {
  unsigned start=off(0x108300+frame*0x180);
  assert(LagoonWideTorches_Begin(t,21,2,0x128000,0x10,0x8300+frame*0x180,0x3b00,0x180));
  for(unsigned i=0;i<0x180;++i)
    LagoonWideTorches_Write(t,0x7600+i,rom[start+i]^(i==(unsigned)corrupt),rom,sizeof(rom));
}
int main(int argc,char **argv) {
  assert(argc==2);FILE*f=fopen(argv[1],"rb");
  assert(f&&fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);
  const uint8_t descriptor[]={4,12,0x42,0xda,0,0x3b,0x80,1};
  assert(!memcmp(rom+off(0x01dfe8),descriptor,8));
  assert(LagoonWideTorches_Bytes(2,0x128000)==0x180);
  assert(!LagoonWideTorches_Bytes(2,0x139a7a));
  LagoonWideTorches t;LagoonWideTorches_Reset(&t);
  for(unsigned phase=0;phase<12;++phase) {
    unsigned a=off(0x01da42+phase*3),frame=phase%4;
    assert((rom[a]|rom[a+1]<<8)==0x8300+frame*0x180&&rom[a+2]==0x10);
    transfer(&t,frame,-1);assert(t.seen&(1u<<frame));
    memcpy((uint8_t*)vram+0x7600,rom+off(0x108300+frame*0x180),0x180);
    assert(LagoonWideTorches_Match(&t,vram,21,2,0x128000)==(int)frame);
  }
  assert(t.seen==15);
  for(unsigned frame=0;frame<4;++frame)for(unsigned n=0;n<0x180;++n) {
    LagoonWideTorches_Reset(&t);transfer(&t,frame,(int)n);assert(!t.seen);
  }
  for(unsigned n=0;n<8;++n) {
    rom[off(0x01dfe8)+n]^=1;LagoonWideTorches_Reset(&t);transfer(&t,0,-1);
    assert(!t.seen);rom[off(0x01dfe8)+n]^=1;
  }
  for(unsigned n=0;n<36;++n) {
    rom[off(0x01da42)+n]^=1;LagoonWideTorches_Reset(&t);transfer(&t,0,-1);
    assert(!t.seen);rom[off(0x01da42)+n]^=1;
  }
  transfer(&t,3,-1);
  memcpy((uint8_t*)vram+0x7600,rom+off(0x108780),0x180);
  for(unsigned n=0;n<0x180;++n) {
    ((uint8_t*)vram)[0x7600+n]^=1;
    assert(LagoonWideTorches_Match(&t,vram,21,2,0x128000)<0);
    ((uint8_t*)vram)[0x7600+n]^=1;
  }
  assert(LagoonWideTorches_Match(&t,vram,22,2,0x128000)<0);
  assert(LagoonWideTorches_Match(&t,vram,21,1,0x128000)<0);
  assert(!LagoonWideTorches_Begin(&t,21,2,0x128000,0x10,0x8301,0x3b00,0x180));
  assert(!LagoonWideTorches_Begin(&t,21,2,0x128000,0x11,0x8300,0x3b00,0x180));
  assert(!LagoonWideTorches_Begin(&t,21,2,0x128000,0x10,0x8300,0x3b01,0x180));
  assert(!LagoonWideTorches_Begin(&t,21,2,0x128000,0x10,0x8300,0x3b00,0x17f));
  assert(!LagoonWideTorches_Begin(&t,21,1,0x139a7a,0x10,0x8300,0x3b00,0x180));
  assert(LagoonWideTorches_Begin(&t,21,2,0x128000,0x10,0x8300,0x3b00,0x180));
  LagoonWideTorches_Write(&t,0x7600,rom[off(0x108300)],rom,sizeof(rom));
  assert(LagoonWideTorches_Match(&t,vram,21,2,0x128000)<0);
  LagoonWideTorches_Write(&t,0x7602,0,rom,sizeof(rom));assert(!t.left&&!t.seen);
  transfer(&t,0,-1);LagoonWideTorches_Write(&t,0x777f,0,rom,sizeof(rom));assert(!t.seen);
  LagoonWideTorches_Reset(&t);assert(!t.seen&&!t.generation);
  puts("room02 original water:12 source phases/4 graphics variants,1536 transfer-byte mutations,44 descriptor/table mutations,384 VRAM-byte mutations, partial/order/overwrite/context/reset refusal PASS");
  return 0;
}
