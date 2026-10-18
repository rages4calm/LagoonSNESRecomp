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

static void compare(CpuState*input,CpuState*c,unsigned pc,unsigned stop){Interp816*i=interp816_init(NULL,rd,wr);assert(i);i->a=input->A;i->x=input->X;i->y=input->Y;i->sp=input->S;i->pc=pc&65535;i->k=pc>>16;i->db=input->DB;i->dp=input->D;i->mf=input->m_flag;i->xf=input->x_flag;i->e=input->emulation;i->i=input->_flag_I;i->v=input->_flag_V;i->c=input->_flag_C;i->d=input->_flag_D;i->n=input->_flag_N;i->z=input->_flag_Z;
 unsigned steps=0;while((((unsigned)i->k<<16)|i->pc)!=stop){assert(++steps<5000000);interp816_runOpcode(i);}assert(!memcmp(ram,fast,sizeof(ram)));if(i->a!=c->A||i->x!=c->X||i->y!=c->Y||i->sp!=c->S||interp816_getFlags(i)!=c->P)fprintf(stderr,"stateA%04x/%04x X%04x/%04x Y%04x/%04x S%04x/%04x P%02x/%02x\n",i->a,c->A,i->x,c->X,i->y,c->Y,i->sp,c->S,interp816_getFlags(i),c->P);assert(i->a==c->A&&i->x==c->X&&i->y==c->Y&&i->sp==c->S&&i->dp==c->D&&i->db==c->DB&&interp816_getFlags(i)==c->P);interp816_free(i);}
static CpuState state(unsigned seed){CpuState c={0};c.S=0x1d0;c.A=0xa508;c.X=0x8234;c.Y=0x9832;c.P=0x24|(seed&1)|(seed&2?0xc0:0);c.m_flag=1;c.DB=2;c.PB=2;cpu_p_to_mirrors(&c);return c;}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);unsigned cases=0;
 for(unsigned seed=0;seed<4;seed++)for(unsigned a=0;a<4;a++)for(unsigned x=0;x<4;x++)for(unsigned y=0;y<4;y++)for(unsigned st=0;st<3;st++){
  for(unsigned q=0;q<sizeof(ram);q++)ram[q]=(uint8_t)(q*109+seed*73);memcpy(fast,ram,sizeof(ram));CpuState c=state(seed);c.S=0x100+st*0x60;c.A=a*0x5555;c.X=x*0x5555;c.Y=y*0x5555;c.PB=1;c.DB=1;CpuState orig=c;
  assert(LagoonMenuClose_FillHud(fast,&c));compare(&orig,&c,0x018ad1,0x018af2);cases++;
 }
 for(unsigned bad=0;bad<10;bad++){
  memset(fast,0xa5,sizeof(fast));memcpy(ram,fast,sizeof(ram));CpuState c=state(0);c.PB=1;c.DB=1;
  switch(bad){case 0:c.PB^=1;break;case 1:c.DB=2;break;case 2:c.m_flag=0;break;case 3:c.x_flag=1;break;case 4:c.emulation=1;break;case 5:c.D=1;break;case 6:c.P|=8;break;case 7:c._flag_D=1;break;case 8:c.S=0xff;break;case 9:c.S=0x1f1;break;}
  CpuState before=c;assert(!LagoonMenuClose_FillHud(fast,&c));assert(!memcmp(ram,fast,sizeof(ram))&&!memcmp(&c,&before,sizeof(c)));
 }
 printf("PASS%u HUDfill original-opcode full128KiB/CPU/flags/stack cases and10 readonly refusals\n",cases);return 0;}
