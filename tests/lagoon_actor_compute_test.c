#include "lagoon_actor_compute.h"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef assert
#define assert(v) do{if(!(v)){fprintf(stderr,"ASSERT line%d: %s\n",__LINE__,#v);exit(2);}}while(0)
static uint8_t rom[0x100000],ram[0x20000],before[0x20000],after[0x20000];
static void put(unsigned q,unsigned v){ram[q]=v;ram[q+1]=v>>8;}
static void scene(unsigned room){
 memcpy(ram+0x50,rom+0x2b2df+room*56,56);put(0x48e,room);
 put(0x492,(ram[0x53]|ram[0x54]<<8)*8-16);put(0x494,(ram[0x55]|ram[0x56]<<8)*8-14);
 put(0x88,0);ram[0x8a]=0x7f;put(0x8b,0x4000);ram[0x8d]=0x7f;
}
uint8 cpu_read8(CpuState*c,uint8 bank,uint16 addr){assert(!bank&&addr<8192);return c->open_bus=ram[addr];}
static uint8_t rd(void*ctx,uint32_t a){(void)ctx;unsigned lo=a&65535,bank=a>>16;if((bank&127)<64&&lo<8192)return ram[lo];if((bank&127)<32&&lo>=32768)return rom[(bank&31)*32768+(lo&32767)];assert(0);return 0;}
static void wr(void*ctx,uint32_t a,uint8_t v){(void)ctx;assert((a>>16)<64&&(a&65535)<8192);ram[a&65535]=v;}
static CpuState prepare(unsigned kind,unsigned actor,unsigned seed){
 for(unsigned q=0;q<sizeof ram;q++)ram[q]=(uint8_t)(q*109+seed*73);
 CpuState c={0};c.A=seed*12345;c.X=actor*64;c.Y=seed*7777;c.P=0x24|(seed&1)|(seed&2?0xc0:0)|(seed&4?0x10:0);c.P&=~0x10;c.DB=c.PB=1;c.S=0x1d0;c.ram=ram;c.master_cycles=12345;c.coprocessor_master_cycles=12300;cpu_p_to_mirrors(&c);
 static const unsigned edge[]={0,1,15,16,17,31,32,63,64,79,80,159,160,175,176,191,192,287,288,351,352,32767,32768,0xff9f,0xffa0,0xffdf,0xffe0,65535};
 put(0x502+c.X,edge[seed%28]);put(0x504+c.X,edge[(seed*7+3)%28]);put(0x480,edge[(seed*13+9)%28]);put(0x482,edge[(seed*11+5)%28]);put(0x492,edge[(seed*19+7)%28]);put(0x494,edge[(seed*17+11)%28]);put(0xa4,seed*10909);
 put(c.S+1,kind?0xa4e7:0xa4d8);put(c.S+3,0x80c6);put(0x48e,3);ram[0x406]=ram[0x100e]=0;ram[0x23d]=15;ram[0x275]=0x81;ram[0x520]=10;ram[0x521]=0;ram[0x500+c.X]=0xc0;ram[0x518+c.X]=2;return c;
}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);unsigned positives=0,refusals=0,production=0;unsigned long long opcodes=0,cycles=0;
 for(unsigned kind=0;kind<2;kind++)for(unsigned actor=0;actor<33;actor++)for(unsigned seed=0;seed<112;seed++){
  CpuState c=prepare(kind,actor,seed),initial=c;memcpy(before,ram,sizeof ram);assert(LagoonActorCompute_Apply(kind,ram,&c));memcpy(after,ram,sizeof ram);memcpy(ram,before,sizeof ram);
  Interp816*i=interp816_init(NULL,rd,wr);assert(i);i->a=initial.A;i->x=initial.X;i->y=initial.Y;i->sp=initial.S;i->dp=0;i->db=i->k=1;i->pc=kind?0xa5aa:0xa56a;i->mf=1;i->xf=0;i->e=0;i->n=initial._flag_N;i->z=initial._flag_Z;i->v=initial._flag_V;i->c=initial._flag_C;i->i=initial._flag_I;i->d=0;
  unsigned ops=0;while(i->pc!=(kind?0xa623:0xa5a9)){assert(++ops<100);cycles+=interp816_runOpcode(i);}opcodes+=ops;
  assert(!memcmp(ram,after,sizeof ram));assert(i->a==c.A&&i->x==c.X&&i->y==c.Y&&i->sp==c.S&&i->db==c.DB&&i->dp==c.D&&i->k==c.PB&&interp816_getFlags(i)==c.P);assert(c.cycles==initial.cycles&&c.master_cycles==initial.master_cycles&&c.coprocessor_master_cycles==initial.coprocessor_master_cycles);interp816_free(i);positives++;
 }
 for(unsigned kind=0;kind<2;kind++)for(unsigned bad=0;bad<33;bad++){
  CpuState c=prepare(kind,1,4);scene(3);c.host_return_valid=2;unsigned s=c.S,h=2,host=kind?0x01a4e8:0x01a4d9;size_t size=sizeof rom;int world=1;unsigned mutation=0;
  switch(bad){case 0:world=0;break;case 1:s++;break;case 2:h=3;break;case 3:host++;break;case 4:c.host_return_valid=3;break;case 5:c.PB=0;break;case 6:c.DB=0;break;case 7:c.D=1;break;case 8:c._flag_D=1;break;case 9:c.P|=8;break;case 10:c.emulation=1;break;case 11:c.m_flag=0;break;case 12:c.x_flag=1;break;case 13:c.X=0;break;case 14:c.X=0x840;break;case 15:c.X++;break;case 16:ram[c.S+1]++;break;case 17:ram[c.S+3]++;break;case 18:ram[0x48e]=2;break;case 19:ram[0x48e]=7;break;case 20:ram[0x406]=1;break;case 21:ram[0x100e]=1;break;case 22:ram[0x23d]=128;break;case 23:ram[0x275]=1;break;case 24:ram[0x520]=0;break;case 25:ram[0x521]=1;break;case 26:ram[0x500+c.X]=0;break;case 27:ram[0x518+c.X]=3;break;case 28:size--;break;case 29:mutation=0xa4d3;break;case 30:mutation=kind?0xa5aa:0xa56a;break;case 31:c.S=0x1fc;break;case 32:ram[0x48f]=1;break;}
  if(mutation)rom[mutation]^=1;CpuState initial=c;memcpy(before,ram,sizeof ram);RecompReturn out=123;
  assert(!LagoonActorCompute_Run(kind,rom,size,ram,&c,s,h,host,&out,world));assert(out==123&&!memcmp(ram,before,sizeof ram)&&!memcmp(&c,&initial,sizeof c));if(mutation)rom[mutation]^=1;refusals++;
 }
 for(unsigned room=0;room<63;room++)
 for(unsigned kind=0;kind<2;kind++)for(unsigned actor=1;actor<33;actor++)for(unsigned seed=0;seed<16;seed++){
  CpuState c=prepare(kind,actor,seed),expected=c;scene(room);c.host_return_valid=2;expected.host_return_valid=2;assert(LagoonActorCompute_Apply(kind,ram,&expected));memcpy(after,ram,sizeof ram);c=prepare(kind,actor,seed);scene(room);c.host_return_valid=2;RecompReturn out=123;unsigned s=c.S;uint64_t master=c.master_cycles;assert(LagoonActorCompute_Run(kind,rom,sizeof rom,ram,&c,s,2,kind?0x01a4e8:0x01a4d9,&out,1));assert(out==RECOMP_RETURN_NORMAL&&c.S==s+2&&c.master_cycles==master+42&&c.cycles==6&&c.coprocessor_master_cycles==master&&c.open_bus==0xa4);assert(!memcmp(ram,after,sizeof ram)&&c.A==expected.A&&c.P==expected.P&&c.X==expected.X&&c.Y==expected.Y);production++;
 }
 for(unsigned room=0;room<75;room++)for(unsigned bad=0;bad<69;bad++){
  CpuState c=prepare(0,1,4);scene(room);c.host_return_valid=2;unsigned mutation=0;
  if(room<63){if(bad<56){mutation=0x2b2df+room*56+bad;rom[mutation]^=1;}else if(bad<67){static const unsigned fields[]={0x50,0x53,0x54,0x55,0x56,0x6c,0x6d,0x6e,0x78,0x79,0x7a};ram[fields[bad-56]]^=1;}else if(bad==67)ram[0x492]^=1;else ram[0x494]^=1;}
  CpuState initial=c;memcpy(before,ram,sizeof ram);RecompReturn out=123;assert(!LagoonActorCompute_Run(0,rom,sizeof rom,ram,&c,c.S,2,0x01a4d9,&out,1));assert(out==123&&!memcmp(ram,before,sizeof ram)&&!memcmp(&c,&initial,sizeof c));if(mutation)rom[mutation]^=1;refusals++;
 }
 printf("PASS%u full128KiB/CPU/flags/deadstack originalleaf cases, %u readonly productionrefusals,%u realJSR/RTS entries across63ordinary descriptors; originalopcodes=%llu originalcycles=%llu\n",positives,refusals,production,opcodes,cycles);return 0;
}
