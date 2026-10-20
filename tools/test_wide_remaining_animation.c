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

static const unsigned owners[][3]={
 {1,0x139a7a,0},{2,0x128000,1},{3,0x128b77,2},{4,0x128b77,2},{5,0x128b77,2},{6,0x128b77,2},{8,0x12b297,3},
 {0x19,0x13d745,7},{0x1e,0x14c061,8},{0x1f,0x14c061,8},{0x20,0x14c061,8},{0x21,0x14c061,8},{0x22,0x14d33d,9},
 {0x32,0x15b8ac,14},{0x33,0x15b8ac,14},{0x34,0x15b8ac,14},{0x35,0x15c815,15},{0x36,0x15c815,15},{0x37,0x15c815,15},{9,0x12b297,3},{0xb,0x12da6c,4},{0xd,0x12da6c,4},
 {0x11,0x168000,5},{0x14,0x13c716,6},{0x15,0x13c716,6},{0x26,0x14de0a,10},{0x2a,0x14edd5,12},
 {0x38,0x15d7e5,11},{0x39,0x15d7e5,11},{0x3a,0x15d7e5,11},{0x3e,0x15e7fb,16},{0x49,0x15a43d,13}
};
static unsigned source(unsigned phase){unsigned p=off(table)+phase*3;return word(rom,p)|rom[p+2]<<16;}
static unsigned frame_index(unsigned phase){unsigned unique=0;for(unsigned i=0;i<=phase;++i){unsigned s=source(i);int first=1;for(unsigned j=0;j<i;++j)if(s==source(j))first=0;if(first){if(s==source(phase))return unique;unique++;}}assert(0);return 0;}
static void transfer_source(unsigned source_address,const uint8_t *bytes){
 assert(LagoonWideTorches_Begin(&torches,29,room,bg,source_address>>16,source_address&65535,0x3b00,length));
 for(unsigned b=0;b<length;++b)LagoonWideTorches_Write(&torches,0x7600+b,bytes[b],rom,sizeof(rom));
}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);
 Interp816*cpu=interp816_init(NULL,read_byte,write_byte);assert(cpu);
 for(unsigned n=0;n<sizeof(owners)/sizeof(owners[0]);++n){
  room=owners[n][0];bg=owners[n][1];index=owners[n][2];descriptor=0x1dfe0+index*8;unsigned dp=off(descriptor);
  phases=rom[dp+1];table=0x10000|word(rom,dp+2);length=word(rom,dp+6);base=source(0)&65535;
  assert(length<=512&&LagoonWideTorches_Bytes(room,bg)==length);
  for(unsigned phase=0;phase<phases;++phase)for(unsigned tick=0;tick<=rom[dp];++tick){
   memset(ram,0,sizeof(ram));LagoonWideTorches_Reset(&torches);ram[0x409]=tick;ram[0x40a]=phase;
   cpu->a=index;run(cpu,1,0xdf88);unsigned expected=tick==rom[dp]?(phase+1)%phases:phase;
   assert(word(ram,0x410)==(source(expected)&65535)&&ram[0x412]==(source(expected)>>16)&&word(ram,0x413)==length&&word(ram,0x415)==0x3b00);
   assert(ram[0x23a]&4);run(cpu,0,0x8f19);assert(!(ram[0x23a]&4));if(LagoonWideTorches_Match(&torches,vram,27,room,bg)!=(int)frame_index(expected)){fprintf(stderr,"animation room%02x index%u phase%u tick%u expected%u actual%d seen%x source%06x len%u\n",room,index,phase,tick,frame_index(expected),LagoonWideTorches_Match(&torches,vram,27,room,bg),torches.seen,source(expected),length);assert(0);}
  }
  for(unsigned phase=0;phase<phases;++phase){
   int first=1;for(unsigned j=0;j<phase;++j)if(source(j)==source(phase))first=0;if(!first)continue;
   uint8_t bytes[512];unsigned address=source(phase);memcpy(bytes,rom+off(address),length);
   for(unsigned b=0;b<length;++b){bytes[b]^=1;LagoonWideTorches_Reset(&torches);transfer_source(address,bytes);assert(!torches.seen);bytes[b]^=1;++mutations;}
   for(unsigned b=0;b<8+phases*3+length;++b){unsigned at=b<8?off(descriptor)+b:b<8+phases*3?off(table)+b-8:off(address)+b-8-phases*3;
    rom[at]^=1;LagoonWideTorches_Reset(&torches);transfer_source(address,bytes);assert(!torches.seen);rom[at]^=1;++mutations;}
   LagoonWideTorches_Reset(&torches);transfer_source(address,bytes);memcpy((uint8_t*)vram+0x7600,bytes,length);
   assert(LagoonWideTorches_Match(&torches,vram,29,room,bg)==(int)frame_index(phase));
   for(unsigned b=0;b<length;++b){((uint8_t*)vram)[0x7600+b]^=1;assert(LagoonWideTorches_Match(&torches,vram,29,room,bg)<0);((uint8_t*)vram)[0x7600+b]^=1;++mutations;}
  }
 }
 interp816_free(cpu);
 /* Complete room/source matrix: sharing a BG resource grants no animation.
  * This also preserves old2/3/4/8 unique-frame limits after table refactor. */
 for(unsigned id=0;id<75;++id){
  unsigned dp=off(0x05b2df+id*56),actual_bg=rom[dp+7]|rom[dp+8]<<8|rom[dp+9]<<16;
  for(unsigned c=0;c<17;++c){descriptor=0x1dfe0+c*8;table=0x10000|word(rom,off(descriptor)+2);length=word(rom,off(descriptor)+6);phases=rom[off(descriptor)+1];
   int owned=0;for(unsigned k=0;k<sizeof(owners)/sizeof(owners[0]);++k)if(owners[k][0]==id&&owners[k][1]==actual_bg&&owners[k][2]==c)owned=1;
   for(unsigned phase=0;phase<phases;++phase){unsigned at=source(phase);LagoonWideTorches_Reset(&torches);assert(!!LagoonWideTorches_Begin(&torches,29,id,actual_bg,at>>16,at&65535,0x3b00,length)==owned);++refusals;}
  }
 }
 printf("all exact animation owners: %u original DF88/008F19 full DMA jobs, %u descriptor/table/source/ordered-byte/resident-byte mutants, %u all75 room/17descriptor ownership checks PASS\n",dma_jobs,mutations,refusals);return 0;
}
