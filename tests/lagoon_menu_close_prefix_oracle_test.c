/* Original ROM is supplied privately by the user; table contains only room
 * addresses/dimensions observed by the qualified campaign, never ROM assets. */
#include "lagoon_menu_close.h"
#include "snes/interp816.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
static unsigned char rom[0x100000],ram[0x20000],fast[0x20000];
static unsigned char rd(void*m,unsigned a){(void)m;unsigned b=a>>16,q=a&65535;if(b==126||b==127)return ram[(b-126)*65536+q];if((b&127)<64&&q<8192)return ram[q];if((b&127)<32&&q>=32768)return rom[(b&31)*32768+(q&32767)];fprintf(stderr,"badread%x\n",a);abort();}
static void wr(void*m,unsigned a,unsigned char v){(void)m;unsigned b=a>>16,q=a&65535;if(b==126||b==127){ram[(b-126)*65536+q]=v;return;}if((b&127)<64&&q<8192){ram[q]=v;return;}fprintf(stderr,"badwrite%x\n",a);abort();}
static void put(unsigned q,unsigned v,unsigned n){while(n--){ram[q++]=v;v>>=8;}}
static void compare(CpuState*input,CpuState*c,unsigned pc,unsigned stop){Interp816*i=interp816_init(NULL,rd,wr);assert(i);i->a=input->A;i->x=input->X;i->y=input->Y;i->sp=input->S;i->pc=pc&65535;i->k=pc>>16;i->db=input->DB;i->dp=input->D;i->mf=input->m_flag;i->xf=input->x_flag;i->e=input->emulation;i->i=input->_flag_I;i->v=input->_flag_V;i->c=input->_flag_C;i->d=input->_flag_D;i->n=input->_flag_N;i->z=input->_flag_Z;
 unsigned steps=0;while((((unsigned)i->k<<16)|i->pc)!=stop){assert(++steps<5000000);interp816_runOpcode(i);}assert(!memcmp(ram,fast,sizeof(ram)));if(i->a!=c->A||i->x!=c->X||i->y!=c->Y||i->sp!=c->S||interp816_getFlags(i)!=c->P)fprintf(stderr,"stateA%04x/%04x X%04x/%04x Y%04x/%04x S%04x/%04x P%02x/%02x\n",i->a,c->A,i->x,c->X,i->y,c->Y,i->sp,c->S,interp816_getFlags(i),c->P);assert(i->a==c->A&&i->x==c->X&&i->y==c->Y&&i->sp==c->S&&i->dp==c->D&&i->db==c->DB&&interp816_getFlags(i)==c->P);interp816_free(i);}
static CpuState state(unsigned seed){CpuState c={0};c.S=0x1d0;c.A=0xa508;c.X=0x8234;c.Y=0x9832;c.P=0x24|(seed&1)|(seed&2?0xc0:0);c.m_flag=1;c.DB=2;c.PB=2;cpu_p_to_mirrors(&c);return c;}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);
 unsigned cases=0;
 for(unsigned seed=0;seed<4;seed++)for(unsigned x=0;x<34;x+=3)for(unsigned y=0;y<32;y+=3)for(unsigned edge=0;edge<3;edge++){
  for(unsigned q=0;q<sizeof(ram);q++)ram[q]=(unsigned char)(q*109+seed*73);
  put(0xb8,edge==0?x*8:edge==1?0xfff8:0x7ff8,2);put(0xba,edge==0?y*8:edge==1?0xfff8:0x8000,2);
  put(0x88,0x7f0000,3);put(0x8b,0x7f4000,3);put(0x69,0x068cd0,3);put(0x75,0x068cd0,3);
  memcpy(fast,ram,sizeof(ram));CpuState c=state(seed);c.DB=c.PB=1;c.X=x;c.Y=y;CpuState orig=c;
  assert(LagoonMenuClose_PrepareTile(rom,sizeof(rom),fast,&c));compare(&orig,&c,0x018a10,0x018a2f);cases++;
 }

 unsigned suffix=0;
 for(unsigned seed=0;seed<4;seed++)for(unsigned x=0;x<34;x++)for(unsigned y=0;y<32;y++){
  for(unsigned q=0;q<sizeof(ram);q++)ram[q]=(unsigned char)(q*109+seed*73);
  put(0xb8,seed==0?0x78:seed==1?0xfff8:seed==2?0x7ff8:0x8000,2);put(0xba,seed==0?0x98:seed==1?0xfff8:seed==2?0x7ff8:0x8000,2);
  put(0x480,seed==0?0x80:seed==1?0:seed==2?0x8000:0x7fff,2);
  CpuState c=state(seed);c.DB=c.PB=1;c.X=0x77c;c.Y=y;put(c.S+1,x,2);CpuState orig=c;memcpy(fast,ram,sizeof(ram));unsigned landing=0;
  assert(LagoonMenuClose_AdvanceTile(fast,&c,&landing));assert(landing==(x==33&&y==31?0x018a5d:0x018a10));compare(&orig,&c,0x018a32,landing);suffix++;
 }
 printf("PASS%u parent-suffix original-opcode cases: full128KiB CPU and deadstack\n",suffix);
 printf("PASS%u parent-prefix original-opcode cases: full128KiB CPU and deadstack\n",cases);return 0;}
