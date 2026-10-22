/* Actual original linear DMA consumer remains the MMIO/VRAM oracle. */
#define main previous_menu_dma_main
#include "lagoon_menu_dma_test.c"
#undef main
static CpuState door_dma(unsigned kind,unsigned n) {
  CpuState c=prepare(kind,n,0);unsigned at=c.S+(kind<2?15:kind==2?17:18);
  put(at,0x018792,3);put(at+3,0xC8E4,2);put(at+5,0x80A2,2);
  ram[0x100E]=255;put(0x48E,2,2);ram[0x500]=0xC0;put(0x520,10,2);return c;
}
int main(int argc,char**argv) {
  assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  unsigned count=0,refusals=0;
  for(unsigned kind=0;kind<4;kind++)for(unsigned n=0;n<(kind==0?4:kind==1?3:kind==2?2:1);n++) {
    CpuState c=door_dma(kind,n);memcpy(before,ram,sizeof ram);
    static Bus fast,old;memset(&fast,0,sizeof fast);memset(&old,0,sizeof old);fast.c=&c;
    LagoonMenuDmaAuthority a={1,0,1};
    assert(LagoonMenuDma_Service(rom,sizeof rom,ram,&c,0x89AC,a,mmio,&fast));
    static uint8_t after[0x20000];memcpy(after,ram,sizeof ram);memcpy(ram,before,sizeof ram);original(&old);
    assert(!memcmp(ram,after,sizeof ram)&&fast.count==10&&old.count==10&&fast.bytes==old.bytes&&
        !memcmp(fast.addr,old.addr,sizeof fast.addr)&&!memcmp(fast.val,old.val,sizeof fast.val)&&!memcmp(fast.vram,old.vram,sizeof fast.vram));count++;
    for(unsigned bad=0;bad<12;bad++) {
      c=door_dma(kind,n);unsigned at=c.S+(kind<2?15:kind==2?17:18);
      if(bad<7)ram[at+bad]^=64;
      else switch(bad){case 7:a.forced_blank=0;break;case 8:a.hdma=1;break;case 9:a.idle=0;break;case 10:ram[0x100E]=1;break;case 11:rom[0xC8E2]^=1;break;}
      CpuState saved=c;memcpy(before,ram,sizeof ram);memset(&fast,0,sizeof fast);
      assert(!LagoonMenuDma_Service(rom,sizeof rom,ram,&c,0x89AC,a,mmio,&fast));
      assert(!fast.count&&!memcmp(before,ram,sizeof ram)&&!memcmp(&saved,&c,sizeof c));refusals++;
      a=(LagoonMenuDmaAuthority){1,0,1};if(bad==11)rom[0xC8E2]^=1;
    }
  }
  printf("PASS%u door transfers match original MMIO/order/VRAM; %u readonly refusals\n",count,refusals);
}
