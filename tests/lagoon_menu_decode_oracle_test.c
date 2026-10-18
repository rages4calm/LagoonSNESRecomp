/* Independent original-opcode oracle; consumes the developer's own ROM.
 * Stream addresses are metadata, never embedded game payloads. */
#include "lagoon_menu_decode.h"
#include "snes/interp816.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static const unsigned sources[]={0x128000,0x128b77,0x129e89,0x12b297,0x12da6c,0x12e794,0x12ee12,0x138000,0x139a7a,0x13a421,0x13b55d,0x13c716,0x13d745,0x13f001,0x148000,0x149197,0x14a1bc,0x14abbe,0x14c061,0x14d33d,0x14de0a,0x158000,0x15918d,0x15a43d,0x15b8ac,0x15c815,0x15d7e5,0x15e7fb,0x15f529,0x168000,0x168ed1,0x188000,0x1891e4,0x18a0b2,0x18afb0,0x18c636,0x18e0a7,0x198000,0x199f6c,0x19af50,0x19c637,0x19e0dd,0x19fd1d,0x1a8000,0x1a906f,0x1a94fe,0x1aaed9,0x1abf2d,0x1adbb7,0x1aed4a,0x1b8000,0x1b84d8,0x1ba17d,0x1bb982,0x1bd63d,0x1be46b,0x1c8000,0x1c92f5,0x1cada9,0x1cb607,0x1cb7f8,0x1cc387,0x1cd41d,0x1ceac1,0x1cfa11,0x1d8000,0x1d9b77,0x1db67e,0x1dc859,0x1dd5c7,0x10e9c3,0x10c5ae,0x10c45a,0x10bd00,0x11be00};
static unsigned char rom[0x100000],ram[0x20000],fast[0x20000];
static unsigned char rd(void *mem,unsigned a){(void)mem;unsigned b=a>>16,q=a&65535;if(b==126||b==127)return ram[(b-126)*65536+q];if((b&127)<64 && q<8192)return ram[q];if((b&127)<32&&q>=32768)return rom[(b&31)*32768+(q&32767)];fprintf(stderr,"bad read %x\n",a);abort();}
static void wr(void *mem,unsigned a,unsigned char v){(void)mem;unsigned b=a>>16,q=a&65535;if(b==126||b==127){ram[(b-126)*65536+q]=v;return;}if((b&127)<64&&q<8192){ram[q]=v;return;}fprintf(stderr,"bad write %x\n",a);abort();}
int main(int argc,char **argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);assert(fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);
 for(unsigned seed=0;seed<4;++seed)for(unsigned id=0;id<sizeof(sources)/sizeof(*sources);++id){
  for(unsigned i=0;i<sizeof(ram);++i)ram[i]=(unsigned char)(i*109+seed*73);
  unsigned source=sources[id];ram[0xe0]=source;ram[0xe1]=source>>8;ram[0xe2]=source>>16;ram[0xe4]=0;memcpy(fast,ram,sizeof(ram));
  CpuState c={0};c.S=0x1d0;c.X=id*7;c.Y=0x1234;c.A=0xA508;c.P=0x64|(seed&1)|(seed==3?8:0);c.m_flag=1;c.DB=1;c._flag_I=1;c._flag_V=1;c._flag_C=seed&1;c._flag_D=seed==3;
  Interp816 *in=interp816_init(NULL,rd,wr);assert(in);in->a=c.A;in->x=c.X;in->y=c.Y;in->sp=c.S;in->pc=0x9143;in->k=0;in->db=1;in->dp=0;in->mf=1;in->xf=0;in->e=0;in->i=1;in->v=1;in->c=seed&1;in->d=seed==3;
  unsigned steps=0;while(in->k || in->pc!=0x9303){assert(++steps<10000000);assert(!in->stopped);interp816_runOpcode(in);}
  unsigned used=0,n=0;assert(LagoonMenuDecode_Expand(rom,sizeof(rom),fast,&c,&used,&n));
  unsigned bad=0;for(unsigned i=0;i<sizeof(ram);++i)if(ram[i]!=fast[i]){if(bad++<8)fprintf(stderr,"id%u seed%u RAM%x original%02x fast%02x\n",id,seed,i,ram[i],fast[i]);}
  assert(!bad);
  if(in->a!=c.A||in->x!=c.X||in->y!=c.Y||in->sp!=c.S||in->dp!=c.D||in->db!=c.DB||interp816_getFlags(in)!=c.P)fprintf(stderr,"CPU id%u seed%u A %04x/%04x X %04x/%04x Y %04x/%04x S %04x/%04x D %04x/%04x P %02x/%02x\n",id,seed,in->a,c.A,in->x,c.X,in->y,c.Y,in->sp,c.S,in->dp,c.D,interp816_getFlags(in),c.P);
  assert(in->a==c.A&&in->x==c.X&&in->y==c.Y&&in->sp==c.S&&in->dp==c.D&&in->db==c.DB&&interp816_getFlags(in)==c.P);
  printf("PASS source%06X seed%u full128KiB+AXYSDDBP flags, used%u produced%u original_opcodes%u\n",source,seed,used,n,steps);interp816_free(in);
 }
 puts("PASS75 original streams across4 poisoned full-WRAM/CPU seeds (300 cases)");
 return 0;
}
