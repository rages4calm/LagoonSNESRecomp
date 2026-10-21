#include "lagoon_actor_compute.h"
#include "snes/interp816.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define check(v) do {if(!(v)){fprintf(stderr,"FAIL line%d %s\n",__LINE__,#v);exit(2);}}while(0)
static uint8_t rom[0x100000],ram[0x20000],before[0x20000],expected[0x20000];
static void put(unsigned a,unsigned v){ram[a]=v;ram[a+1]=v>>8;}
uint8 cpu_read8(CpuState*c,uint8 bank,uint16 addr){check(!bank&&addr<8192);return c->open_bus=ram[addr];}
static uint8_t rd(void*ctx,uint32_t a){(void)ctx;unsigned b=a>>16,p=a&65535;if((b&127)<64&&p<8192)return ram[p];if((b&127)<32&&p>=32768)return rom[(b&31)*32768+(p&32767)];check(0);return 0;}
static void wr(void*ctx,uint32_t a,uint8_t v){(void)ctx;check((a>>16)<64&&(a&65535)<8192);ram[a&65535]=v;}
static CpuState prepare(unsigned seed){
 for(unsigned n=0;n<sizeof ram;n++)ram[n]=(n*109)^(n>>4)^(seed*73);
 unsigned room=seed%63;memcpy(ram+0x50,rom+0x2b2df+room*56,56);put(0x48e,room);put(0x492,(ram[0x53]|ram[0x54]<<8)*8-16);put(0x494,(ram[0x55]|ram[0x56]<<8)*8-14);put(0x88,0);ram[0x8a]=0x7f;put(0x8b,0x4000);ram[0x8d]=0x7f;
 CpuState c={0};c.ram=ram;c.S=0x1d0;c.X=((seed%32)+1)*64;c.Y=seed*31337;c.A=seed*197;c.P=0x24|(seed&1)|(seed&2?0xc0:0);c.DB=c.PB=1;c.master_cycles=12345;c.coprocessor_master_cycles=12000;c.host_return_valid=2;cpu_p_to_mirrors(&c);
 static const unsigned edge[]={0,1,3,5,15,16,127,128,255,256,1023,2047,0x7fff,0x8000,0xfffb,0xffff};
 put(0x502+c.X,edge[seed%16]);put(0x504+c.X,edge[(seed>>4)%16]);put(0x506+c.X,seed*3571);put(0x508+c.X,(seed*17749)^0x8000);
 put(c.S+1,0xa4d5);put(c.S+3,0x80c6);ram[0x406]=ram[0x100e]=0;ram[0x23d]=15;ram[0x275]=0x81;ram[0x520]=54;ram[0x521]=0;ram[0x500+c.X]=0xc0;ram[0x501+c.X]=(seed*17)&~0x20;ram[0x518+c.X]=2;return c;
}
int main(int argc,char**argv){
 check(argc==2);FILE*f=fopen(argv[1],"rb");check(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);unsigned cases=0,refusals=0;unsigned long long ops=0,cycles=0;
 for(unsigned seed=0;seed<8192;seed++){
  CpuState c=prepare(seed),initial=c;memcpy(before,ram,sizeof ram);check(LagoonActorCompute_IntegrateApply(ram,&c));memcpy(expected,ram,sizeof ram);memcpy(ram,before,sizeof ram);
  Interp816*i=interp816_init(NULL,rd,wr);check(i);i->a=initial.A;i->x=initial.X;i->y=initial.Y;i->sp=initial.S;i->dp=0;i->db=i->k=1;i->pc=0xa4eb;i->mf=1;i->xf=0;i->e=0;i->n=initial._flag_N;i->z=initial._flag_Z;i->v=initial._flag_V;i->c=initial._flag_C;i->i=initial._flag_I;i->d=0;
  unsigned count=0;while(i->pc!=0xa544){check(++count<40);cycles+=interp816_runOpcode(i);}ops+=count;check(!memcmp(ram,expected,sizeof ram));check(i->a==c.A&&i->x==c.X&&i->y==c.Y&&i->sp==c.S&&i->db==c.DB&&i->dp==c.D&&i->k==c.PB&&interp816_getFlags(i)==c.P);interp816_free(i);
  memcpy(ram,before,sizeof ram);CpuState leaf=c;c=initial;RecompReturn out=123;check(LagoonActorCompute_Run(2,rom,sizeof rom,ram,&c,c.S,2,0x01a4d6,&out,1));check(out==RECOMP_RETURN_NORMAL&&c.S==initial.S+2&&c.cycles==6&&c.master_cycles==initial.master_cycles+42&&c.coprocessor_master_cycles==initial.master_cycles&&c.open_bus==0xa4);check(!memcmp(ram,expected,sizeof ram)&&c.A==leaf.A&&c.P==leaf.P&&c.X==leaf.X&&c.Y==leaf.Y);cases++;
 }
 for(unsigned value=0;value<256;value++)if(value&0x20){
  CpuState c=prepare(value),initial=c;ram[0x501+c.X]=value;memcpy(before,ram,sizeof ram);RecompReturn out=123;check(!LagoonActorCompute_Run(2,rom,sizeof rom,ram,&c,c.S,2,0x01a4d6,&out,1));check(out==123&&!memcmp(ram,before,sizeof ram)&&!memcmp(&c,&initial,sizeof c));refusals++;
 }
 for(unsigned pos=0;pos<90;pos++){
  CpuState c=prepare(0),initial=c;memcpy(before,ram,sizeof ram);RecompReturn out=123;rom[0xa4eb+pos]^=1;check(!LagoonActorCompute_Run(2,rom,sizeof rom,ram,&c,c.S,2,0x01a4d6,&out,1));check(out==123&&!memcmp(ram,before,sizeof ram)&&!memcmp(&c,&initial,sizeof c));rom[0xa4eb+pos]^=1;refusals++;
 }
 printf("PASS %u whole-RAM/register original integration cases and qualified RTS calls across63ordinary rooms/32enemy slots, %u jump/source read-only refusals; original opcodes=%llu cycles=%llu\n",cases,refusals,ops,cycles);return 0;
}
