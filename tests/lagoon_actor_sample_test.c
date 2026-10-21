#include "lagoon_actor_compute.h"
#include "snes/interp816.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define check(v) do { if(!(v)){fprintf(stderr,"FAIL line%d: %s\n",__LINE__,#v);exit(2);} }while(0)
static uint8_t rom[0x100000],ram[0x20000],initial_ram[0x20000],expected_ram[0x20000];
static void put(unsigned q,unsigned v){ram[q]=v;ram[q+1]=v>>8;}
static void pointer(unsigned q,unsigned v){put(q,v);ram[q+2]=v>>16;}
static void scene(unsigned room){
 memcpy(ram+0x50,rom+0x2b2df+room*56,56);put(0x48e,room);
 put(0x492,(ram[0x53]|ram[0x54]<<8)*8-16);put(0x494,(ram[0x55]|ram[0x56]<<8)*8-14);
 pointer(0x88,0x7f0000);pointer(0x8b,0x7f4000);
}
uint8 cpu_read8(CpuState*c,uint8 bank,uint16 addr){check(!bank&&addr<8192);return c->open_bus=ram[addr];}
static uint8_t rd(void*ctx,uint32_t a){(void)ctx;unsigned bank=a>>16,lo=a&65535;
 if(bank==0x7e||bank==0x7f)return ram[(bank-0x7e)*65536+lo];
 if((bank&127)<64&&lo<8192)return ram[lo];
 if((bank&127)<32&&lo>=32768)return rom[(bank&31)*32768+(lo&32767)];
 check(0);return 0;
}
static void wr(void*ctx,uint32_t a,uint8_t v){(void)ctx;check((a>>16)<64&&(a&65535)<8192);ram[a&65535]=v;}
static CpuState prepare(unsigned seed){
 for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8_t)((n*109)^(n>>4)^(seed*73));
 CpuState c={0};c.ram=ram;c.S=0x1d0;c.X=((seed%32)+1)*64;c.Y=(seed*31337)&65535;c.A=seed*197;c.P=0x24|(seed&1)|(seed&2?0xc0:0);c.DB=c.PB=1;c.master_cycles=12345;c.coprocessor_master_cycles=12000;c.host_return_valid=2;cpu_p_to_mirrors(&c);
 static const unsigned edges[]={0,1,7,8,15,16,17,127,128,255,256,1023,1024,2047,2048,0x7fff,0x8000,0xffef,0xfff0,0xffff};
 put(0xb8,edges[seed%20]);put(0xba,edges[(seed*7+3)%20]);pointer(0x88,0x7e2000);pointer(0x8b,0x7e8000);pointer(0x6c,0x7fc000);pointer(0x78,0x7fd000);
 put(c.S+1,0x9e28+(seed%4)*0x84);put(c.S+3,0x80c3);put(0x48e,8);ram[0x406]=ram[0x100e]=0;ram[0x23d]=15;ram[0x275]=0x81;ram[0x520]=54;ram[0x521]=0;ram[0x500+c.X]=0xc0;ram[0x518+c.X]=2;return c;
}
static void exact_registers(const CpuState*c,Interp816*i){
 check(i->a==c->A&&i->x==c->X&&i->y==c->Y&&i->sp==c->S&&i->db==c->DB&&i->dp==c->D&&i->k==c->PB&&interp816_getFlags(i)==c->P);
}
int main(int argc,char**argv){
 check(argc==2);FILE*f=fopen(argv[1],"rb");check(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 unsigned positive=0,production=0,refusals=0;unsigned long long ops=0,cycles=0;
 for(unsigned seed=0;seed<2560;seed++){
  CpuState c=prepare(seed);if(seed&2){pointer(0x6c,0x058000);pointer(0x78,0x068000);}if(seed&1){pointer(0x88,0x078000);pointer(0x8b,0x088000);put(0xb8,seed%2048);put(0xba,(seed*7)%2048);}CpuState initial=c;memcpy(initial_ram,ram,sizeof ram);check(LagoonActorCompute_SampleApply(rom,sizeof rom,ram,&c));memcpy(expected_ram,ram,sizeof ram);memcpy(ram,initial_ram,sizeof ram);
  Interp816*i=interp816_init(NULL,rd,wr);check(i);i->a=initial.A;i->x=initial.X;i->y=initial.Y;i->sp=initial.S;i->dp=0;i->db=i->k=1;i->pc=0x8385;i->mf=1;i->xf=0;i->e=0;i->n=initial._flag_N;i->z=initial._flag_Z;i->v=initial._flag_V;i->c=initial._flag_C;i->i=initial._flag_I;i->d=0;
  unsigned count=0;while(i->pc!=0x83d7){check(++count<100);cycles+=interp816_runOpcode(i);}ops+=count;check(!memcmp(ram,expected_ram,sizeof ram));exact_registers(&c,i);check(c.cycles==initial.cycles&&c.master_cycles==initial.master_cycles&&c.coprocessor_master_cycles==initial.coprocessor_master_cycles);interp816_free(i);positive++;
  c=prepare(seed);scene(seed%63);put(0xb8,seed%2048);put(0xba,(seed*7)%2048);memcpy(initial_ram,ram,sizeof ram);CpuState expected=c;check(LagoonActorCompute_SampleApply(rom,sizeof rom,ram,&expected));memcpy(expected_ram,ram,sizeof ram);memcpy(ram,initial_ram,sizeof ram);initial=c;
  RecompReturn out=123;unsigned ret=ram[c.S+1]|ram[c.S+2]<<8;
  check(LagoonActorCompute_SampleRun(rom,sizeof rom,ram,&c,c.S,2,0x010000+ret+1,&out,1));check(out==RECOMP_RETURN_NORMAL&&c.S==initial.S+2&&c.cycles==6&&c.master_cycles==initial.master_cycles+42&&c.coprocessor_master_cycles==initial.master_cycles&&c.open_bus==0x9e + (seed%4)/2);
  check(!memcmp(ram,expected_ram,sizeof ram)&&c.A==expected.A&&c.P==expected.P&&c.X==expected.X&&c.Y==expected.Y);production++;
 }
 for(unsigned bad=0;bad<40;bad++){
  CpuState c=prepare(0);scene(8);unsigned s=c.S,h=2,pc=0x019e29;int normal=1;size_t size=sizeof rom;unsigned mutation=0;
  switch(bad){case 0:normal=0;break;case 1:s++;break;case 2:h=3;break;case 3:pc++;break;case 4:c.host_return_valid=3;break;case 5:c.PB=0;break;case 6:c.DB=0;break;case 7:c.D=1;break;case 8:c._flag_D=1;break;case 9:c.P|=8;break;case 10:c.emulation=1;break;case 11:c.m_flag=0;break;case 12:c.x_flag=1;break;case 13:c.X=0;break;case 14:c.X=0x840;break;case 15:c.X++;break;case 16:ram[c.S+1]++;break;case 17:ram[c.S+3]++;break;case 18:ram[0x48e]=2;break;case 19:ram[0x48e]=7;break;case 20:ram[0x406]=1;break;case 21:ram[0x100e]=1;break;case 22:ram[0x23d]=128;break;case 23:ram[0x275]=1;break;case 24:ram[0x520]=0;break;case 25:ram[0x521]=1;break;case 26:ram[0x500+c.X]=0;break;case 27:ram[0x518+c.X]=3;break;case 28:size--;break;case 29:mutation=0x8385;break;case 30:mutation=0x9bce;break;case 31:c.S=0x105;break;case 32:ram[0x48f]=1;break;case 33:pointer(0x88,0x002100);break;case 34:pointer(0x8b,0x002000);break;case 35:pointer(0x6c,0x7e02c1);break;case 36:pointer(0x78,0x7e01cc);break;case 37:mutation=0x80c1;break;case 38:c.S=0x1fc;break;case 39:ram[0x518+c.X]=1;break;}
  if(mutation)rom[mutation]^=1;CpuState before=c;memcpy(initial_ram,ram,sizeof ram);RecompReturn out=123;
  check(!LagoonActorCompute_SampleRun(rom,size,ram,&c,s,h,pc,&out,normal));check(out==123&&!memcmp(ram,initial_ram,sizeof ram)&&!memcmp(&c,&before,sizeof c));if(mutation)rom[mutation]^=1;refusals++;
 }
 for(unsigned pos=0;pos<1186;pos++){
  unsigned q=pos<83?0x8385+pos:pos<1183?0x9bce + pos-83:0x80c1+pos-1183;CpuState c=prepare(0);scene(8);CpuState before=c;memcpy(initial_ram,ram,sizeof ram);rom[q]^=1;RecompReturn out=123;
  check(!LagoonActorCompute_SampleRun(rom,sizeof rom,ram,&c,c.S,2,0x019e29,&out,1));check(out==123&&!memcmp(ram,initial_ram,sizeof ram)&&!memcmp(&c,&before,sizeof c));rom[q]^=1;refusals++;
 }
 printf("PASS %u whole-RAM/register/deadstack original-opcode sampler cases, %u real JSR/RTS calls, %u read-only source/mode/stack/pointer refusals; original opcodes=%llu cycles=%llu\n",positive,production,refusals,ops,cycles);
 return 0;
}
