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
static unsigned room=0x19,bg=0x13d745,base=0x97c0,length=480,phases=2,descriptor=0x1e018,table=0x1dcb1,index=7;
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
  assert(LagoonWideTorches_Begin(&torches,27,room,bg,source_bank,source,destination,length));
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
  assert(LagoonWideTorches_Begin(&torches,27,room,bg,0x10,base+frame*length,0x3b00,length));
  for(unsigned n=0;n<length;++n)LagoonWideTorches_Write(&torches,0x7600+n,bytes[n],rom,sizeof(rom));
}
int main(int argc,char **argv) {
  int field=argc==3 && !strcmp(argv[2],"--field-only");assert(argc==2||field);
  if(field){room=0x1e;bg=0x14c061;base=0x9b80;length=416;phases=8;descriptor=0x1e020;table=0x1dce1;index=8;}
  FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);
  assert(kLagoonWideTorchMaxBytes>=length && LagoonWideTorches_Bytes(room,bg)==length);
  Interp816 *cpu=interp816_init(NULL,read_byte,write_byte);assert(cpu);
  for(unsigned scene=0;scene<(field?4:1);++scene) {
  if(field)room=0x1e + scene;
  for(unsigned phase=0;phase<phases;++phase)for(unsigned tick=0;tick<=4;++tick) {
    memset(ram,0,sizeof(ram));LagoonWideTorches_Reset(&torches);ram[0x409]=(uint8_t)tick;ram[0x40a]=(uint8_t)phase;
    cpu->a=(uint16_t)index;run(cpu,1,0xdf88);unsigned expected=tick==4?(phase+1)%phases:phase;
    assert(word(ram,0x410)==base+expected*length && ram[0x412]==0x10 && word(ram,0x413)==length && word(ram,0x415)==0x3b00);
    assert(ram[0x23a]&4);run(cpu,0,0x8f19);assert(!(ram[0x23a]&4));
    assert(LagoonWideTorches_Match(&torches,vram,27,room,bg)==(int)expected);
  }
  }
  if(field)room=0x1e;
  interp816_free(cpu);
  for(unsigned phase=0;phase<phases;++phase) {
    uint8_t bytes[512];memcpy(bytes,rom+off(0x100000+base+phase*length),length);
    for(unsigned bad=0;bad<length;++bad) {
      LagoonWideTorches_Reset(&torches);bytes[bad]^=1;transfer(phase,bytes);assert(!torches.seen);bytes[bad]^=1;++mutations;
    }
    for(unsigned n=0;n<8+phases*3+length;++n) {
      unsigned address=n<8?off(descriptor)+n:n<8+phases*3?off(table)+n-8:off(0x100000+base+phase*length)+n-8-phases*3;
      rom[address]^=1;LagoonWideTorches_Reset(&torches);transfer(phase,bytes);assert(!torches.seen);rom[address]^=1;++mutations;
    }
    LagoonWideTorches_Reset(&torches);transfer(phase,bytes);memcpy((uint8_t*)vram+0x7600,bytes,length);
    for(unsigned n=0;n<length;++n){((uint8_t*)vram)[0x7600+n]^=1;assert(LagoonWideTorches_Match(&torches,vram,27,room,bg)<0);((uint8_t*)vram)[0x7600+n]^=1;++mutations;}
    assert(LagoonWideTorches_Match(&torches,vram,28,room,bg)<0);++refusals;
    LagoonWideTorches_Write(&torches,0x7600+length-1,0,rom,sizeof(rom));assert(!torches.seen);++refusals;
    assert(LagoonWideTorches_Begin(&torches,27,room,bg,0x10,base,0x3b00,length));
    LagoonWideTorches_Write(&torches,0x7601,bytes[0],rom,sizeof(rom));assert(!torches.left&&!torches.seen);++refusals;
  }
  for(unsigned bad=0;bad<8;++bad) {
    unsigned badroom=room,source=bg,bank=0x10,at=base,to=0x3b00,badlength=length;
    if(bad==0)badroom=0x1a;if(bad==1)source++;if(bad==2)bank++;if(bad==3)at++;
    if(bad==4)at+=phases*length;if(bad==5)to++;if(bad==6)badlength--;if(bad==7)badlength++;
    LagoonWideTorches_Reset(&torches);assert(!LagoonWideTorches_Begin(&torches,27,badroom,source,bank,at,to,badlength));++refusals;
  }
  if(field) {
    const unsigned old[][4]={{1,0x139a7a,0x8000,384},{2,0x128000,0x8300,384},
      {3,0x128b77,0x8900,192},{4,0x128b77,0x8900,192},{5,0x128b77,0x8900,192},
      {6,0x128b77,0x8900,192},{8,0x12b297,0x8b40,192},{0x19,0x13d745,0x97c0,480},
      {0x35,0x15c815,0xbb00,64},{0x36,0x15c815,0xbb00,64},{0x37,0x15c815,0xbb00,64}};
    for(unsigned n=0;n<sizeof(old)/sizeof(old[0]);++n) {
      unsigned count=old[n][0]==2 || old[n][0]>=0x35?4:old[n][0]>=3 && old[n][0]<=6?3:2;
      for(unsigned phase=count;phase<=8;++phase) {
        LagoonWideTorches_Reset(&torches);
        assert(!LagoonWideTorches_Begin(&torches,27,old[n][0],old[n][1],0x10,old[n][2]+phase*old[n][3],0x3b00,old[n][3]));++refusals;
      }
      LagoonWideTorches_Reset(&torches);
      assert(!LagoonWideTorches_Begin(&torches,27,old[n][0],old[n][1],0x10,0x9b80,0x3b00,416));++refusals;
    }
    for(unsigned n=0;n<2;++n) {
      LagoonWideTorches_Reset(&torches);
      assert(!LagoonWideTorches_Begin(&torches,27,n?0x1d:0x18,0x14abbe,0x10,0x9b80,0x3b00,416));++refusals;
    }
  }
  printf("original animation: %u original DF88/008F19 configured complete DMA jobs/source resident phases, %u descriptor/table/source/transfer/VRAM byte mutations, %u context/generation/overwrite/order refusals PASS\n",dma_jobs,mutations,refusals);
  return 0;
}
