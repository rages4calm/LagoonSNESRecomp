#include "lagoon_actor_compute.h"
#include "snes/interp816.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define check(v) do {if(!(v)){fprintf(stderr,"FAIL line%d %s\n",__LINE__,#v);exit(2);}}while(0)
static uint8_t rom[0x100000],ram[0x20000],before[0x20000],expected[0x20000];
static unsigned word(const uint8_t*r,unsigned a){return r[a]|r[a+1]<<8;}
static void put(unsigned a,unsigned v){ram[a]=v;ram[a+1]=v>>8;}
uint8 cpu_read8(CpuState*c,uint8 bank,uint16 addr){check(!bank&&addr<8192);return c->open_bus=ram[addr];}
static uint8_t rd(void*ctx,uint32_t a){(void)ctx;unsigned b=a>>16,p=a&65535;if(b==0x7e||b==0x7f)return ram[(b-0x7e)*65536+p];if((b&127)<64&&p<8192)return ram[p];if((b&127)<32&&p>=32768)return rom[(b&31)*32768+(p&32767)];check(0);return 0;}
static void wr(void*ctx,uint32_t a,uint8_t v){(void)ctx;if((a>>16)==0x7e||(a>>16)==0x7f){ram[((a>>16)-0x7e)*65536+(a&65535)]=v;return;}check((a>>16)<64&&(a&65535)<8192);ram[a&65535]=v;}
static CpuState prepare(unsigned seed){
 for(unsigned n=0;n<sizeof ram;n++)ram[n]=(n*109)^(n>>4)^(seed*73);
 unsigned room=seed%63;memcpy(ram+0x50,rom+0x2b2df+room*56,56);put(0x48e,room);put(0x492,(ram[0x53]|ram[0x54]<<8)*8-16);put(0x494,(ram[0x55]|ram[0x56]<<8)*8-14);put(0x88,0);ram[0x8a]=0x7f;put(0x8b,0x4000);ram[0x8d]=0x7f;
 CpuState c={0};c.ram=ram;c.S=0x1d0;c.X=((seed%32)+1)*64;c.Y=seed*31337;c.A=seed*197;c.P=0x24|(seed&1)|(seed&2?0xc0:0);c.DB=c.PB=1;c.master_cycles=12345;c.coprocessor_master_cycles=12000;c.host_return_valid=2;cpu_p_to_mirrors(&c);
 static const unsigned edge[]={0,1,3,5,15,16,127,128,255,256,1023,2047,0x7fff,0x8000,0xfffb,0xffff};
 put(0x502+c.X,edge[seed%16]);put(0x504+c.X,edge[(seed>>4)%16]);put(0x506+c.X,seed*3571);put(0x508+c.X,(seed*17749)^0x8000);put(0x480,seed*103);put(0x482,seed*177);
 put(c.S+1,0x80c6);ram[0x406]=ram[0x100e]=0;ram[0x23d]=15;ram[0x275]=0x81;ram[0x520]=54;ram[0x521]=0;ram[0x500+c.X]=0xc0|(seed&63);ram[0x501+c.X]=(seed*17)&~0x20;ram[0x518+c.X]=2;c.PB=0;c.A=seed&255;c.X=seed*127;c.S=0x1b0;
 put(c.S+1,0x8bf4);put(c.S+7,seed&1?0x8bcc:0x8bb2);put(c.S+11,((seed%32)+1)*64);put(c.S+13,seed&2?0xc254:0xc282);ram[c.S+15]=1;
 unsigned actor=((seed%32)+1)*64;ram[0x500+actor]=0xc0;ram[0x518+actor]=2;
 ram[0x236]=seed&255;put(0x28,seed*103);put(0x2a,seed*127);put(0x2c,seed*199);ram[0x2e]=seed*73;
 return c;
}
int main(int argc,char**argv){
 check(argc==2);FILE*f=fopen(argv[1],"rb");check(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);unsigned cases=0,refusals=0;unsigned long long ops=0,cycles=0;
 for(unsigned seed=0;seed<8192;seed++){
  CpuState c=prepare(seed),initial=c;memcpy(before,ram,sizeof ram);check(LagoonActorCompute_CellApply(ram,&c));memcpy(expected,ram,sizeof ram);memcpy(ram,before,sizeof ram);
  Interp816*i=interp816_init(NULL,rd,wr);check(i);i->a=initial.A;i->x=initial.X;i->y=initial.Y;i->sp=initial.S;i->dp=0;i->db=1;i->k=0;i->pc=0x8211;i->mf=1;i->xf=0;i->e=0;i->n=initial._flag_N;i->z=initial._flag_Z;i->v=initial._flag_V;i->c=initial._flag_C;i->i=initial._flag_I;i->d=0;
  unsigned count=0;while(i->pc!=0x8290&&i->pc!=0x8216){check(++count<100);cycles+=interp816_runOpcode(i);}ops+=count;
  if(memcmp(ram,expected,sizeof ram)){for(unsigned k=0;k<sizeof ram;k++)if(ram[k]!=expected[k]){fprintf(stderr,"seed%u ram%x original%02x native%02x\n",seed,k,ram[k],expected[k]);break;}check(0);}
  if(i->a!=c.A||i->x!=c.X||interp816_getFlags(i)!=c.P)fprintf(stderr,"seed%u A%x/%x X%x/%x P%x/%x\n",seed,i->a,c.A,i->x,c.X,interp816_getFlags(i),c.P);
  check(i->a==c.A&&i->x==c.X&&i->y==c.Y&&i->sp==c.S&&i->db==c.DB&&i->dp==c.D&&i->k==c.PB&&interp816_getFlags(i)==c.P);interp816_free(i);
  memcpy(ram,before,sizeof ram);CpuState leaf=c;c=initial;RecompReturn out=123;check(LagoonActorCompute_CellRun(rom,sizeof rom,ram,&c,c.S,2,0x008bf5,&out,1));check(out==RECOMP_RETURN_NORMAL&&c.S==initial.S+2&&c.cycles==6&&c.master_cycles==initial.master_cycles+42&&c.coprocessor_master_cycles==initial.master_cycles&&c.open_bus==0x8b);check(!memcmp(ram,expected,sizeof ram)&&c.A==leaf.A&&c.P==leaf.P&&c.X==leaf.X&&c.Y==leaf.Y);cases++;
 }
 for(unsigned bad=0;bad<48;bad++){
  CpuState c=prepare(4);unsigned s=c.S,h=2,host=0x008bf5;size_t size=sizeof rom;int world=1;
  switch(bad){case 0:world=0;break;case 1:s++;break;case 2:h=3;break;case 3:host++;break;case 4:c.host_return_valid=3;break;case 5:c.PB=1;break;case 6:c.DB=0;break;case 7:c.D=1;break;case 8:c._flag_D=1;break;case 9:c.P|=8;break;case 10:c.emulation=1;break;case 11:c.m_flag=0;break;case 12:c.x_flag=1;break;case 13:c.A=256;break;case 14:c.S=0x105;break;case 15:c.S=0x1f1;break;case 16:ram[c.S+1]++;break;case 17:ram[c.S+7]++;break;case 18:ram[c.S+11]++;break;case 19:put(c.S+11,0);break;case 20:put(c.S+11,0x840);break;case 21:ram[c.S+13]++;break;case 22:ram[c.S+15]=0;break;case 23:ram[0x406]=1;break;case 24:ram[0x100e]=1;break;case 25:ram[0x23d]=0;break;case 26:ram[0x275]=0;break;case 27:ram[0x520]=0;break;case 28:ram[0x521]=1;break;case 29:ram[0x500+word(ram,c.S+11)]=0;break;case 30:ram[0x518+word(ram,c.S+11)]=3;break;case 31:size--;break;case 32:put(0x48e,63);break;case 33:ram[0x53]^=1;break;case 34:ram[0x492]^=1;break;case 35:ram[0x494]^=1;break;case 36:ram[0x88]^=1;break;case 37:ram[0x8d]^=1;break;case 38:put(0x48e,64);break;case 39:put(0x48e,74);break;default:ram[0x50+(bad-40)]^=1;break;}
  if(bad>=40&&bad!=40&&bad!=43&&bad!=44&&bad!=45&&bad!=46)continue;
  CpuState initial=c;memcpy(before,ram,sizeof ram);RecompReturn out=123;check(!LagoonActorCompute_CellRun(rom,size,ram,&c,s,h,host,&out,world));check(out==123&&!memcmp(ram,before,sizeof ram)&&!memcmp(&c,&initial,sizeof c));refusals++;
 }
 for(unsigned pos=0;pos<138+293+112;pos++){
  unsigned at=pos<138?0x211+pos:pos<431?0xb71+pos-138:0xc214+pos-431;CpuState c=prepare(4),initial=c;memcpy(before,ram,sizeof ram);RecompReturn out=123;rom[at]^=1;check(!LagoonActorCompute_CellRun(rom,sizeof rom,ram,&c,c.S,2,0x008bf5,&out,1));check(out==123&&!memcmp(ram,before,sizeof ram)&&!memcmp(&c,&initial,sizeof c));rom[at]^=1;refusals++;
 }
 printf("PASS %u original opcode OAM packer full128KiB/register/deadstack/RTS cases, %u source/mode/scene/ownership read-only refusals; opcodes=%llu cycles=%llu; no IO oracle\n",cases,refusals,ops,cycles);return 0;
}
