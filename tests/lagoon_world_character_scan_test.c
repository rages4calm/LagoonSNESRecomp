/* Own original ROM required. No map/art bytes are embedded in this test. */
#include "snes/interp816.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WORLD_SCAN_BENCH
#include <windows.h>
#endif
static uint8_t rom[0x100000],g_ram[0x20000],ram_before[0x20000];
static const uint8_t *g_rom=rom;
typedef struct { uint16_t vram[32768]; } Ppu;
static Ppu ppu,*g_ppu=&ppu;
static uint8_t s_world_char_used[1024],reference[1024];
static unsigned parallax,cases,real_maps;
static uint64_t source_reads,old_reads,new_reads,ops_total;
static int room_parallax(void){return parallax;}
static const uint16_t *PpuRenderVram(const Ppu *p){return p->vram;}
#include "world_character_scan_under_test.h"
static unsigned offset(unsigned a){assert((a&65535)>=32768&&(a>>16)<32);return (a>>16)*32768+(a&32767);}
static void put(unsigned a,unsigned v){g_ram[a]=(uint8_t)v;g_ram[a+1]=(uint8_t)(v>>8);}
static void triple(unsigned a,unsigned v){put(a,v);g_ram[a+2]=(uint8_t)(v>>16);}
static uint8_t readb(void *ctx,uint32_t a){
  (void)ctx;unsigned b=a>>16,w=a&65535;
  if(b==0x7e||b==0x7f)return g_ram[(b-0x7e)*65536+w];
  if((b&127)<64&&w<8192)return g_ram[w];
  return rom[offset(a)];
}
static void writeb(void *ctx,uint32_t a,uint8_t v){
  (void)ctx;unsigned b=a>>16,w=a&65535;
  if(b==0x7e||b==0x7f){g_ram[(b-0x7e)*65536+w]=v;return;}
  assert((b&127)<64&&w<8192);g_ram[w]=v;
}
static void compare(void){
  memcpy(ram_before,g_ram,sizeof g_ram);
  memset(s_world_char_used,0,sizeof s_world_char_used);source_reads=0;
  discover_reference();memcpy(reference,s_world_char_used,sizeof reference);old_reads+=source_reads;
  uint64_t before=source_reads;
  memset(s_world_char_used,0,sizeof s_world_char_used);source_reads=0;discover_candidate();new_reads+=source_reads;
  if(memcmp(reference,s_world_char_used,sizeof reference)){
    fprintf(stderr,"used bits differ case%u dims%u/%u parallax%u\n",cases,word(0x53),word(0x55),parallax);exit(2);
  }
  assert(!memcmp(ram_before,g_ram,sizeof g_ram));
  if(cases<75)printf("ROOM %02X old_reads%llu new_reads%llu\n",cases,
      (unsigned long long)before,(unsigned long long)source_reads);
#ifndef WORLD_SCAN_BENCH
  /* Compare planes separately as well: a full second plane must not mask a
   * missing bit from the first plane's union. */
  unsigned tw=word(0x53),th=word(0x55);if(tw>256)tw=256;if(th>256)th=256;
  for(unsigned layer=0;layer<2;++layer){
    memset(s_world_char_used,0,sizeof s_world_char_used);
    discover_layer_reference(layer,tw,th);memcpy(reference,s_world_char_used,sizeof reference);
    memset(s_world_char_used,0,sizeof s_world_char_used);mark_map_characters(layer,tw,th);
    assert(!memcmp(reference,s_world_char_used,sizeof reference));
  }
  assert(!memcmp(ram_before,g_ram,sizeof g_ram));
#endif
  ++cases;
}
static void original_map(unsigned room){
  memset(g_ram,0,sizeof g_ram);
  memcpy(g_ram+0x50,rom+offset(0x05b2df+room*56),56);put(0x48e,room);
  Interp816 *c=interp816_init(NULL,readb,writeb);assert(c);
  c->k=c->db=2;c->pc=0xc5c6;c->sp=0x1d0;c->mf=1;c->xf=0;c->i=1;
  g_ram[0x1d1]=0xef;g_ram[0x1d2]=0x7f;g_ram[0x1d3]=2;
  unsigned ops=0;
  while(c->k!=2||c->pc!=0x7ff0){assert(++ops<1000000);interp816_runOpcode(c);}
  assert(c->sp==0x1d3);ops_total+=ops;interp816_free(c);
  triple(0x88,0x7f0000);triple(0x8b,0x7f4000);parallax=g_ram[0x50]!=0;
  compare();++real_maps;
}
#ifdef WORLD_SCAN_BENCH
static volatile unsigned sink;
static double bench(int candidate){
  LARGE_INTEGER begin,end,frequency;assert(QueryPerformanceFrequency(&frequency));
  assert(QueryPerformanceCounter(&begin));
  for(unsigned n=0;n<100;++n){
    if(candidate)discover_candidate();else discover_reference();
    sink+=s_world_char_used[n%1024];
  }
  assert(QueryPerformanceCounter(&end));
  return (double)(end.QuadPart-begin.QuadPart)*1000/(double)frequency.QuadPart;
}
#endif
int main(int argc,char **argv){
  assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  for(unsigned n=0;n<32768;++n)ppu.vram[n]=(uint16_t)(n%3);
#ifdef WORLD_SCAN_BENCH
  for(unsigned room=0x14;room<=0x2a;room+=0x16){
    original_map(room);
    for(unsigned pass=0;pass<3;++pass){
      double old_ms,new_ms;
      if(pass&1){new_ms=bench(1);old_ms=bench(0);}else{old_ms=bench(0);new_ms=bench(1);}
      printf("BENCH room%02X pass%u calls100 old_ms%.6f new_ms%.6f\n",room,pass,old_ms,new_ms);
    }
  }
  return 0;
#else
  for(unsigned room=0;room<75;++room)original_map(room);
  uint64_t real_old=old_reads,real_new=new_reads;
  static const unsigned extents[]={0,1,2,3,4,31,32,127,255,256,257,65535};
  for(unsigned pattern=0;pattern<3;++pattern)for(unsigned px=0;px<2;++px)
    for(unsigned wi=0;wi<sizeof extents/sizeof *extents;++wi)
      for(unsigned hi=0;hi<sizeof extents/sizeof *extents;++hi){
        memset(g_ram,0,sizeof g_ram);parallax=px;
        put(0x53,extents[wi]);put(0x55,extents[hi]);g_ram[0x52]=255;
        triple(0x88,0x7f0000);triple(0x8b,0x7f4000);
        /* Definitions may be WRAM too; no assumed ROM immutability is lent. */
        triple(0x69,pattern==2?0x7e4000:0x068000);triple(0x75,pattern==2?0x7e4001:0x069720);
        for(unsigned n=0;n<32768;++n)g_ram[0x10000+n]=(uint8_t)(pattern==0?7:pattern==1?n:n*131+cases);
        for(unsigned n=0;n<2049;++n)g_ram[0x4000+n]=(uint8_t)(n*37+cases);
        compare();
      }
  /* Changed map/definition bytes cannot borrow a prior scan. */
  for(unsigned n=0;n<256;++n){g_ram[0x10000+n]^=(uint8_t)(n+1);g_ram[0x4000+n]^=0xff;compare();}
  printf("PASS world character discovery %u cases/%u original room maps/%llu original opcodes; exact1024 used bits/read-only RAM; odd/empty/capped extents,fill,parallax,WRAM definitions and mutations\n",
      cases,real_maps,(unsigned long long)ops_total);
  printf("READS real75 %llu -> %llu; all %llu -> %llu (source reads, not wall time)\n",
      (unsigned long long)real_old,(unsigned long long)real_new,
      (unsigned long long)old_reads,(unsigned long long)new_reads);
  return 0;
#endif
}
