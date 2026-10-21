/* Original ROM required; no game assets are embedded. Actual018EA6/9143,
 * 02A5BA through its three map uploads, and both original pulse palette paths
 * execute using real native NMI frames and RTI. Only hardware is modeled.
 * This is a bounded source/ownership/PPU oracle, not an uninterrupted cast. */
#include "../src/lagoon_wide_spell3.c"
#include "snes/interp816.h"
#include "snes/snes.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
Snes *g_snes;int snes_frame_counter;unsigned char g_snesrecomp_last_hdmaen;
uint16_t WsShadowTile(int l,int x,uint32_t y,uint16_t s,uint16_t a,uint16_t t){return t;}
bool WsShadowLayerActive(int l){return false;}
uint32_t WsShadowWorldX(int l){return 0;}uint32_t WsShadowWorldY(int l){return 0;}
uint32_t WsShadowScrollX(int l){return 0;}uint32_t WsShadowScrollY(int l){return 0;}
uint32_t WsShadowPresentWorldY(int l,int x){return 0;}
void WsShadowOnVramWrite(uint16_t a,uint16_t v){}
static uint8_t rom[0x100000],ram[0x20000],io[65536],rawv[65536],cg[512];
static Ppu p;static uint32_t native[256*224],wide[800*224];
static unsigned transfers,nmis,ops;static Interp816 *cpu;
static unsigned rw(const uint8_t *r,unsigned a){return r[a]|r[a+1]<<8;}
static uint8_t rd(void *context,uint32_t address) {
  unsigned b=address>>16,a=address&65535;
  if(b==0x7e||b==0x7f)return ram[(b-0x7e)*65536+a];
  if((b&127)<64&&a<8192)return ram[a];
  if((b&127)<64&&a<32768)return io[a];
  assert((b&127)<32&&a>=32768);return rom[offset(address&0x7fffff)];
}
static void wr(void *context,uint32_t address,uint8_t value) {
  unsigned b=address>>16,a=address&65535;
  if(b==0x7e||b==0x7f){ram[(b-0x7e)*65536+a]=value;return;}
  if((b&127)<64&&a<8192){ram[a]=value;return;}
  assert((b&127)<64&&a<32768);io[a]=value;
  if(a==0x4202||a==0x4203){unsigned m=io[0x4202]*io[0x4203];io[0x4216]=m;io[0x4217]=m>>8;}
  if(a!=0x420b)return;
  for(unsigned channel=0;channel<8;++channel)if(value&(1u<<channel)) {
    unsigned q=0x4300+channel*16,source=rw(io,q+2),bank=io[q+4],length=rw(io,q+5);
    if(io[q+1]==0x18) {
      unsigned destination=rw(io,0x2116);assert((channel==1||channel==3)&&io[q]==1&&io[0x2115]==0x80);
      LagoonWideSpell3_Dma(rom,sizeof rom,ram,31,channel,bank,source,destination,length,value==(1u<<channel));
      for(unsigned n=0;n<length;++n){unsigned a=(destination*2+n)&65535;uint8_t v=rd(NULL,(bank<<16)|((source+n)&65535));rawv[a]=v;LagoonWideSpell3_Write(a,v);}
      ++transfers;
    }else if(io[q+1]==0x22) {
      assert(channel==2&&io[q]==0&&bank==0x7e&&source==0x2400&&length==512);
      for(unsigned n=0;n<length;++n)cg[n]=rd(NULL,(bank<<16)|(source+n));
    }else assert(io[q+1]==4); /* Native OAM upload remains original. */
  }
}
static void init(unsigned bank,unsigned pc,unsigned depth) {
  cpu->k=bank;cpu->pc=pc;cpu->db=bank;cpu->dp=0;cpu->sp=0x1d0;
  cpu->a=cpu->x=cpu->y=0;cpu->mf=1;cpu->xf=0;
  cpu->e=cpu->d=cpu->c=cpu->n=cpu->v=cpu->z=0;cpu->i=1;
  ram[0x1d1]=0xef;ram[0x1d2]=0x7f;if(depth==3)ram[0x1d3]=bank;
}
static void run(unsigned stopbank,unsigned stop) {
  unsigned count=0;
  while(cpu->k!=stopbank||cpu->pc!=stop) {
    assert(++count<2000000);++ops;
    if(cpu->k==0&&((cpu->pc==0x89ac||cpu->pc==0x8ddb)&&(ram[0x23a]&32) ||
        cpu->pc==0x84e3&&ram[0x23c])){cpu->nmiWanted=true;++nmis;}
    interp816_runOpcode(cpu);
  }
}
static void copy_ppu(void) {
  memcpy(p.vram,rawv,sizeof rawv);memcpy(p.cgram,cg,sizeof cg);
}
static void scanout(unsigned flags) {
  PpuBeginDrawing(&p,(uint8_t*)native,256*4,flags);ppu_runLine(&p,0);
  for(unsigned y=1;y<=224;++y)ppu_runLine(&p,y);
}
static void prepare(unsigned width) {
  for(unsigned n=0;n<800*224;++n)wide[n]=0xDE123456;
  unsigned left=(width-256)/2;
  for(unsigned y=0;y<224;++y)memcpy(wide+y*800+left,native+y*256,1024);
}
static int draw(unsigned width){return LagoonWideSpell3_DrawMargins((uint8_t*)wide,800*4,width,224,&p,rom,sizeof rom,ram,31);}
static void witness(unsigned width) {
  prepare(width);assert(draw(width));unsigned left=(width-256)/2;
  for(unsigned y=0;y<224;++y)for(unsigned x=0;x<800;++x) {
    if(x>=left&&x<left+256)assert(wide[y*800+x]==native[y*256+x-left]);
    else if(x>=width)assert(wide[y*800+x]==0xDE123456);
    else assert(wide[y*800+x]==native[0]);
  }
}
int main(int argc,char **argv) {
  assert(argc==2||argc==3);FILE *f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  assert(source_ok(rom,sizeof rom));io[0x2115]=0x80;cpu=interp816_init(NULL,rd,wr);assert(cpu);
  memcpy(ram+0x50,rom+offset(0x05b2df)+0x14*56,56);
  ram[0x48e]=0x14;ram[0x88]=0;ram[0x89]=0;ram[0x8a]=0x7f;
  ram[0x8b]=0;ram[0x8c]=0x40;ram[0x8d]=0x7f;ram[0x537]=3;
  init(1,0x8ea6,2);run(1,0x7ff0);assert(cpu->sp==0x1d2&&transfers==2&&s_spell.stage==2);
  assert(hash(rawv+0x6000,4704)==UINT64_C(0xce8541573afdbdeb));
  /* Original world-page service shares7000 but has not acquired any spell
   * map. Its real channel3 job must not discard the separate3000 graphics. */
  init(0,0x8f8c,2);cpu->x=cpu->y=0x7000;run(0,0x7ff0);
  assert(cpu->sp==0x1d2&&s_spell.stage==2&&hash(rawv+0x6000,4704)==UINT64_C(0xce8541573afdbdeb));
  LagoonWideSpell3_Write(0xe000,rawv[0xe000]);assert(s_spell.stage==2);
  ram[0x417]=1;init(2,0xa5ba,2);run(2,0xa607);
  assert(transfers==6&&s_spell.stage==5&&!s_spell.left);
  assert(!memcmp(rawv+0xe000,rom+offset(0x11f37d),2048));
  for(unsigned n=0;n<4096;n+=2)assert(rw(rawv,0xf000+n)==0x1ff);
  /* World restoration withdraws ALL canvas authority. A subsequent genuine
   * original map sequence can reuse only the still-owned unchanged graphics. */
  init(0,0x8f8c,2);cpu->x=cpu->y=0x7000;run(0,0x7ff0);
  assert(s_spell.stage==2&&hash(rawv+0x6000,4704)==UINT64_C(0xce8541573afdbdeb));
  init(2,0xa5ba,2);run(2,0xa607);assert(s_spell.stage==5&&transfers==10);
  /* The exact original0128-white intro is intentionally narrower fallback. */
  ppu_reset(&p);p.inidisp=15;p.bgmode=9;p.obsel=2;p.bgTileAdr=0x600;
  p.bgXsc[0]=0x71;p.bgXsc[1]=0x79;p.bgXsc[2]=0x3c;
  p.screenEnabled[0]=p.screenEnabled[1]=31;p.cgwsel=0x30;
  for(unsigned i=0;i<128;++i)p.oam[i*2]=0xf000;
  copy_ppu();scanout(1);prepare(800);assert(!draw(800));
  init(2,0xa647,2);run(2,0x7ff0);assert(cpu->sp==0x1d2);
  unsigned phases=0,pixels=0;
  for(unsigned phase=0;phase<2;++phase) {
    init(2,phase?0xa65f:0xa65a,2);run(2,0x7ff0);assert(cpu->sp==0x1d2);copy_ppu();
    for(unsigned renderer=0;renderer<2;++renderer)for(unsigned brightness=0;brightness<16;++brightness) {
      p.inidisp=brightness;scanout(renderer?kPpuRenderFlags_NewRenderer:0);
      witness(800);witness(427);++phases;pixels+=(800+427)*224;
      /* Current CPU/PPU palette has advanced, but displayed field has not. */
      for(unsigned n=0;n<122;++n)p.cgram[n]^=0x7fff;
      if(brightness){prepare(800);assert(!draw(800));}
      for(unsigned n=0;n<122;++n)p.cgram[n]^=0x7fff;
    }
  }
  /* Synthetic variation proves the SDK2bpp palette range, not ownership of
   * guessed HUD art: each possible2bpp color/all8 palettes, flips and native
   * priorities still maps to the same current pulse backdrop. */
  for(unsigned sample=0;sample<4;++sample) {
    copy_ppu();
    for(unsigned i=0;i<1024;++i)p.vram[0x3c00+i]=(i%8)*0x400|0x2000|((i%4)*0x4000)|1;
    for(unsigned i=0;i<8;++i)p.vram[0x6008+i]=sample==0?0x00ff:sample==1?0xff00:sample==2?0xffff:0x55aa;
    for(unsigned phase=0;phase<2;++phase)for(unsigned renderer=0;renderer<2;++renderer) {
      for(unsigned n=0;n<122;++n)p.cgram[n]=phase?0x7fff:0;
      p.inidisp=15;scanout(renderer?kPpuRenderFlags_NewRenderer:0);witness(800);++phases;pixels+=800*224;
    }
  }
  if(argc==3) {
    uint16_t fixture[32768];char path[4096];snprintf(path,sizeof path,"%s.vram.bin",argv[2]);
    f=fopen(path,"rb");assert(f&&fread(fixture,1,sizeof fixture,f)==sizeof fixture);fclose(f);copy_ppu();
    memcpy(p.vram+0x6000,fixture+0x6000,0x2000);memcpy(p.vram+0x3c00,fixture+0x3c00,0x800);
    for(unsigned phase=0;phase<2;++phase)for(unsigned renderer=0;renderer<2;++renderer) {
      for(unsigned n=0;n<122;++n)p.cgram[n]=phase?0x7fff:0;
      p.inidisp=15;scanout(renderer?kPpuRenderFlags_NewRenderer:0);witness(800);++phases;pixels+=800*224;
    }
  }
  copy_ppu();
  p.inidisp=15;scanout(kPpuRenderFlags_NewRenderer);prepare(800);assert(draw(800));
  unsigned refused=0;uint8_t owner[sizeof s_spell];memcpy(owner,&s_spell,sizeof s_spell);
  for(unsigned a=0;a<65536;++a)if(owned(a)) {
    LagoonWideSpell3_Write(a,((uint8_t*)p.vram)[a]);assert(!draw(800));
    memcpy(&s_spell,owner,sizeof s_spell);++refused;
  }
  for(unsigned a=0;a<32768;++a)if(owned(a*2)||a==0x1ff0) {
    p.vram[a]^=1;assert(!draw(800));p.vram[a]^=1;++refused;
  }
  for(unsigned i=0;i<128;++i){p.cgram[i]^=1;assert(!draw(800));p.cgram[i]^=1;++refused;}
  for(unsigned i=0;i<32;++i){s_spell.key[i]^=1;assert(!draw(800));s_spell.key[i]^=1;++refused;}
  ++s_spell.generation;assert(!draw(800));memcpy(&s_spell,owner,sizeof s_spell);++refused;
  const unsigned lengths[]={0,1,4095,4096,4703};
  for(unsigned i=0;i<5;++i){s_spell.left=lengths[i];s_spell.stage=4;assert(!draw(800));++refused;}
  memcpy(&s_spell,owner,sizeof s_spell);
  const unsigned uploads[][4]={{0x7f,0x7f80,0x3000,0x1000},{0x7f,0x8f80,0x3800,0x260},
    {0x11,0xf37d,0x7000,0x800},{0x7e,0x6000,0x7c00,0x800},{0x7e,0x6000,0x7800,0x1000}};
  for(unsigned kind=0;kind<5;++kind)for(unsigned sample=0;sample<4;++sample) {
    memcpy(&s_spell,owner,sizeof s_spell);s_spell.stage=kind;s_spell.left=0;
    const unsigned *u=uploads[kind];
    LagoonWideSpell3_Dma(rom,sizeof rom,ram,31,1,u[0],u[1],u[2],u[3],1);
    unsigned prefix=sample==0?0:sample==1?1:sample==2?u[3]/2:u[3]-1;
    for(unsigned b=0;b<prefix;++b)LagoonWideSpell3_Write(u[2]*2+b,rawv[u[2]*2+b]);
    assert(!draw(800));++refused;
  }
  for(unsigned next=3;next<5;++next) {
    memcpy(&s_spell,owner,sizeof s_spell);s_spell.stage=2;
    const unsigned *u=uploads[next];
    LagoonWideSpell3_Dma(rom,sizeof rom,ram,31,1,u[0],u[1],u[2],u[3],1);
    for(unsigned b=0;b<u[3];++b)LagoonWideSpell3_Write(u[2]*2+b,rawv[u[2]*2+b]);
    assert(!draw(800));++refused;
  }
  memcpy(&s_spell,owner,sizeof s_spell);
  for(unsigned a=0;a<16;++a){p.cgadsub=1u<<a%8;assert(!draw(800));++refused;}p.cgadsub=0;
  const unsigned masks[]={1,2,4,8,0x40,0x80};for(unsigned i=0;i<6;++i){p.cgwsel=masks[i];assert(!draw(800));++refused;}p.cgwsel=0x30;
  p.mosaic=1;assert(!draw(800));p.mosaic=0;p.windowsel=1;assert(!draw(800));p.windowsel=0;
  p.screenWindowed[0]=1;assert(!draw(800));p.screenWindowed[0]=0;p.hScroll[0]=1;assert(!draw(800));p.hScroll[0]=0;
  p.inidisp=0x8f;assert(!draw(800));p.inidisp=15;
  g_snesrecomp_last_hdmaen=255;assert(!draw(800));g_snesrecomp_last_hdmaen=0;++refused;
  unsigned source_refusals=0;
  const unsigned regions[][2]={{0x11d0ac,3256},{0x11f37d,2048},{0x18ea6,0x190},{0x8d82,0x99},{0x2a5ba,0xcb},{0xfd53,12},{0x5b2df+0x14*56,56},{0x8884,0x49},{0x84de,0xb},{0x195b2,0xb8},{0x8e2c,6},{0x2b150,0x23}};
  for(unsigned j=0;j<sizeof regions/sizeof regions[0];++j)for(unsigned b=0;b<regions[j][1];++b) {
    rom[offset(regions[j][0])+b]^=1;assert(!draw(800));rom[offset(regions[j][0])+b]^=1;++source_refusals;
  }
  /* Missing/reordered/aliased map transfers never establish a canvas owner. */
  const unsigned changes[][5]={{1,0x91,0xf37d,0x7000,0x800},{1,0x11,0xf37e,0x7000,0x800},
    {1,0x11,0xf37d,0x7001,0x800},{1,0x11,0xf37d,0x7000,0x7ff},{0,0x11,0xf37d,0x7000,0x800}};
  for(unsigned i=0;i<5;++i){LagoonWideSpell3_Dma(rom,sizeof rom,ram,31,changes[i][0],changes[i][1],changes[i][2],changes[i][3],changes[i][4],1);assert(!draw(800));memcpy(&s_spell,owner,sizeof s_spell);++refused;}
  LagoonWideSpell3_Reset();assert(!draw(800));
  printf("spell3: original EA6/9143/NMI/RTI split4704-byte graphics, two02A5BA three-map sequences, two intervening original8F8C world-page jobs; %u NMI/%u opcodes; %u black/white brightness+renderer phases/%u canvas+native pixels; %u ownership/layout/palette/order refusals/%u source-byte refusals PASS\n",nmis,ops,phases,pixels,refused+5,source_refusals);
  interp816_free(cpu);return 0;
}
