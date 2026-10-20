/* Exact town19 animation descriptor and genuine original DMA setup.
 * The original DF88 selector and 008F19 NMI configure the test bus; the
 * production ownership helper consumes actual source bytes in bus order. */
#include "lagoon_wide_entities.h"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t rom[0x100000],ram[0x20000];
static uint16_t vram[32768];
static LagoonWideTorches torches;
static unsigned dma_jobs,mutations,refusals;
static unsigned off(unsigned a){return ((a>>16)&127)*32768+(a&32767);}
static unsigned word(const uint8_t *p,unsigned a){return p[a]|p[a+1]<<8;}
static uint8_t read_byte(void *m,uint32_t address) {
  (void)m;unsigned bank=address>>16,a=address&65535;
  if(bank==0x7e || bank==0x7f)return ram[(bank-0x7e)*65536+a];
  if((bank&127)<64 && a<8192)return ram[a];
  assert((bank&127)<32 && a>=32768);return rom[(bank&31)*32768+(a&32767)];
}
static void write_byte(void *m,uint32_t address,uint8_t value) {
  (void)m;unsigned bank=address>>16,a=address&65535;
  assert((bank&127)<64);
  if(a<8192 || a==0x2116 || a==0x2117 || (a>=0x4330 && a<=0x4336)){ram[a]=value;return;}
  assert(a==0x420b && value==8 && ram[0x4330]==1 && ram[0x4331]==0x18);
  unsigned source=word(ram,0x4332),source_bank=ram[0x4334],length=word(ram,0x4335),destination=word(ram,0x2116);
  assert(LagoonWideTorches_Begin(&torches,26,0x19,0x13d745,source_bank,source,destination,length));
  for(unsigned n=0;n<length;++n) {
    unsigned at=destination*2+n;uint8_t b=read_byte(NULL,(source_bank<<16)|((source+n)&65535));
    ((uint8_t*)vram)[at]=b;LagoonWideTorches_Write(&torches,at,b,rom,sizeof(rom));
  }
  ++dma_jobs;
}
static void run(Interp816 *cpu,unsigned bank,unsigned pc) {
  cpu->pc=(uint16_t)pc;cpu->k=cpu->db=(uint8_t)bank;cpu->dp=0;cpu->sp=0x1d0;
  cpu->mf=1;cpu->xf=0;cpu->e=cpu->d=cpu->c=cpu->n=cpu->v=cpu->z=0;cpu->i=1;
  ram[0x1d1]=0xef;ram[0x1d2]=0x7f;unsigned instructions=0;
  while(cpu->k!=bank || cpu->pc!=0x7ff0){assert(++instructions<1000);interp816_runOpcode(cpu);}
  assert(cpu->sp==0x1d2);
}
static void transfer(unsigned frame,const uint8_t *bytes) {
  assert(LagoonWideTorches_Begin(&torches,26,0x19,0x13d745,0x10,0x97c0+frame*480,0x3b00,480));
  for(unsigned n=0;n<480;++n)LagoonWideTorches_Write(&torches,0x7600+n,bytes[n],rom,sizeof(rom));
}
int main(int argc,char **argv) {
  assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);
  assert(kLagoonWideTorchMaxBytes>=480 && LagoonWideTorches_Bytes(0x19,0x13d745)==480);
  Interp816 *cpu=interp816_init(NULL,read_byte,write_byte);assert(cpu);
  for(unsigned phase=0;phase<2;++phase)for(unsigned tick=0;tick<=4;++tick) {
    memset(ram,0,sizeof(ram));LagoonWideTorches_Reset(&torches);ram[0x409]=(uint8_t)tick;ram[0x40a]=(uint8_t)phase;
    cpu->a=7;run(cpu,1,0xdf88);unsigned expected=tick==4?(phase+1)%2:phase;
    assert(word(ram,0x410)==0x97c0+expected*480 && ram[0x412]==0x10 && word(ram,0x413)==480 && word(ram,0x415)==0x3b00);
    assert(ram[0x23a]&4);run(cpu,0,0x8f19);assert(!(ram[0x23a]&4));
    assert(LagoonWideTorches_Match(&torches,vram,26,0x19,0x13d745)==(int)expected);
  }
  interp816_free(cpu);
  for(unsigned phase=0;phase<2;++phase) {
    uint8_t bytes[480];memcpy(bytes,rom+off(0x1097c0+phase*480),480);
    for(unsigned bad=0;bad<480;++bad) {
      LagoonWideTorches_Reset(&torches);bytes[bad]^=1;transfer(phase,bytes);assert(!torches.seen);bytes[bad]^=1;++mutations;
    }
    for(unsigned n=0;n<494;++n) {
      unsigned address=n<8?off(0x1e018)+n:n<14?off(0x1dcb1)+n-8:off(0x1097c0+phase*480)+n-14;
      rom[address]^=1;LagoonWideTorches_Reset(&torches);transfer(phase,bytes);assert(!torches.seen);rom[address]^=1;++mutations;
    }
    LagoonWideTorches_Reset(&torches);transfer(phase,bytes);memcpy((uint8_t*)vram+0x7600,bytes,480);
    for(unsigned n=0;n<480;++n){((uint8_t*)vram)[0x7600+n]^=1;assert(LagoonWideTorches_Match(&torches,vram,26,0x19,0x13d745)<0);((uint8_t*)vram)[0x7600+n]^=1;++mutations;}
    assert(LagoonWideTorches_Match(&torches,vram,27,0x19,0x13d745)<0);++refusals;
    LagoonWideTorches_Write(&torches,0x77df,0,rom,sizeof(rom));assert(!torches.seen);++refusals;
    assert(LagoonWideTorches_Begin(&torches,26,0x19,0x13d745,0x10,0x97c0,0x3b00,480));
    LagoonWideTorches_Write(&torches,0x7601,bytes[0],rom,sizeof(rom));assert(!torches.left&&!torches.seen);++refusals;
  }
  for(unsigned bad=0;bad<8;++bad) {
    unsigned room=0x19,source=0x13d745,bank=0x10,at=0x97c0,to=0x3b00,length=480;
    if(bad==0)room=0x1a;if(bad==1)source++;if(bad==2)bank++;if(bad==3)at++;
    if(bad==4)at+=960;if(bad==5)to++;if(bad==6)length--;if(bad==7)length++;
    LagoonWideTorches_Reset(&torches);assert(!LagoonWideTorches_Begin(&torches,26,room,source,bank,at,to,length));++refusals;
  }
  printf("town19 animation: %u original DF88/008F19 configured complete DMA jobs/two resident phases, %u descriptor/table/source/transfer/VRAM byte mutations, %u context/generation/overwrite/order refusals PASS\n",dma_jobs,mutations,refusals);
  return 0;
}
