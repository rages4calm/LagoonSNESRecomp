/* Real original timer and NMI-counter opcodes, from the owner's verified ROM.
 * The interrupt probe executes891B..8921 on an independent saved-register CPU,
 * representing the original NMI's register save/restore around this body.
 * This proves bounded timer/NMI ordering, not the entire NMI/DMA implementation. */
#include "lagoon_fast_menus.h"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t rom[0x100000],ram[0x20000],before[0x20000];
static unsigned instruction_pc,reads,nmis,brightness[32],brightness_count;
static Interp816 *active;
static void put(unsigned q,unsigned v,unsigned n){while(n--){ram[q++]=v;v>>=8;}}
static CpuWramReadContext context(void){CpuWramReadContext c={.instruction_pc=instruction_pc,.stack=active->sp,.pb=active->k,.m=active->mf,.x=active->xf,.e=active->e,.interpreter=1};return c;}
static uint8_t rd(void*ctx,uint32_t q){
 (void)ctx;unsigned bank=q>>16,lo=q&65535;
 if(bank==0x7e||bank==0x7f)return ram[(bank-0x7e)*65536+lo];
 if((bank&127)<64&&lo<8192){if(lo==0x23c){CpuWramReadContext c=context();if(instruction_pc==0x84e3)++reads;return LagoonFastMenus_FilterRead(lo,ram[lo],&c,ram);}return ram[lo];}
 if((bank&127)<32&&lo>=32768)return rom[(bank&31)*32768+(lo&32767)];
 fprintf(stderr,"unexpected read%06x\n",q);abort();
}
static void wr(void*ctx,uint32_t q,uint8_t v){
 (void)ctx;unsigned bank=q>>16,lo=q&65535;
 if(bank==0&&lo==0x2100){assert(brightness_count<32);brightness[brightness_count++]=v;return;}
 assert((bank&127)<64&&lo<8192);if(lo==0x23c){CpuWramReadContext c=context();v=LagoonFastMenus_Filter(lo,v,&c,ram);if(instruction_pc==0x84e0){assert(v==1||v==3);reads=0;}}ram[lo]=v;
}
static void nmi_counter(void){
 Interp816*guest=active;unsigned saved_pc=instruction_pc,was=ram[0x23c];
 active=interp816_init(NULL,rd,wr);assert(active);active->pc=0x891b;active->k=active->db=0;active->mf=true;active->xf=false;active->e=false;
 unsigned ops=0;while(active->pc!=0x8922){assert(++ops<=3);instruction_pc=active->pc;interp816_runOpcode(active);}
 assert(ops==3&&ram[0x23c]==(uint8_t)(was-1));interp816_free(active);active=guest;instruction_pc=saved_pc;reads=0;nmis++;
}
static void start(unsigned pc,unsigned s,unsigned seed){
 active=interp816_init(NULL,rd,wr);assert(active);active->a=3;active->x=0x2468;active->y=0x1357;active->sp=s;active->pc=pc;active->k=active->db=0;active->dp=0;active->mf=true;active->xf=false;active->e=false;active->i=true;active->c=seed&1;active->v=!!(seed&2);active->d=false;reads=nmis=brightness_count=0;
}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);LagoonFastMenus_Reset(rom,sizeof(rom));
 const unsigned owners[]={0x02b73a,0x02b87d,0x02bc9b},boundaries[]={0,0x84e0,0x84e3,0x84e6,0x84e8};unsigned cases=0;
 for(unsigned seed=0;seed<4;seed++)for(unsigned owner=0;owner<3;owner++)for(unsigned dir=0;dir<2;dir++)for(unsigned level=0;level<16;level++)for(unsigned edge=0;edge<5;edge++){
  for(unsigned q=0;q<sizeof(ram);q++)ram[q]=(uint8_t)(q*109+seed*73);
  unsigned s=0x1d0,inner=dir?0x008471:0x0083d8;put(s+1,inner,3);put(s+4,dir?0x02c675:0x02c65e,3);put(s+7,0x0080e3,3);ram[s+10]=1;put(s+11,dir?0x018765:0x018708,3);put(s+14,0x0080e3,3);ram[s+17]=2;put(s+18,owners[owner],3);ram[0x23d]=level;memcpy(before,ram,sizeof(ram));start(0x84e0,s,seed);
  unsigned ops=0,injected=0;while((((unsigned)active->k<<16)|active->pc)!=inner+1){
   assert(++ops<100);instruction_pc=((unsigned)active->k<<16)|active->pc;
   if(edge&&!injected&&instruction_pc==boundaries[edge]){nmi_counter();injected=1;}
   /* Subsequent normal field servicing for genuine waits; never condition the
    * original NMI decrement itself on the counter being nonzero. */
   if(instruction_pc==0x84e3&&reads>=4)nmi_counter();
   interp816_runOpcode(active);
  }
  assert(!edge||injected);assert(ops<60);before[0x23c]=ram[0x23c];assert(!memcmp(before,ram,sizeof(ram)));assert(active->a==0&&active->x==0x2468&&active->y==0x1357&&active->sp==s+3&&active->db==0&&active->dp==0&&active->mf&&!active->xf&&!active->e&&active->i&&!active->d&&active->c==(bool)(seed&1)&&active->v==(bool)!!(seed&2)&&active->z&&!active->n);
  if((dir?level<=14:level>=1)&&level%3&&!edge){assert(ops==4&&ram[0x23c]==1&&nmis==0);}
  interp816_free(active);cases++;
 }
 unsigned fades=0;
 for(unsigned seed=0;seed<4;seed++)for(unsigned owner=0;owner<3;owner++)for(unsigned dir=0;dir<2;dir++)for(unsigned kind=0;kind<3;kind++){
  for(unsigned q=0;q<sizeof(ram);q++)ram[q]=(uint8_t)(q*109+seed*73);
  unsigned s=0x1d3,outer=kind==1?(dir?0x02b6e6:0x02b6b9):(dir?0x02c675:0x02c65e);
  put(s+1,outer,3);put(s+4,0x0080e3,3);ram[s+7]=1;put(s+8,dir?0x018765:0x018708,3);put(s+11,0x0080e3,3);ram[s+14]=2;put(s+15,kind==2?0x02ffff:owners[owner],3);ram[0x23d]=dir?0x80:15;ram[0x275]=0x81;start(dir?0x8458:0x83c4,s,seed);unsigned ops=0;
  while((((unsigned)active->k<<16)|active->pc)!=outer+1){assert(++ops<3000);instruction_pc=((unsigned)active->k<<16)|active->pc;if(instruction_pc==0x84e3&&reads>=4)nmi_counter();interp816_runOpcode(active);}
  assert(brightness_count==16);for(unsigned n=0;n<16;n++)assert(brightness[n]==(dir?n:(n<15?14-n:128)));
  assert(nmis==(kind==0?5:kind==1?15:45));assert(active->sp==s+3&&active->x==0x2468&&active->y==0x1357&&ram[0x23d]==(dir?15:128));interp816_free(active);fades++;
 }
 /* Regression witness: the rejected zero-store model really becomesFF at the
  * first original read when the actual NMI body runs betweenSTA andLDA. */
 ram[0x23c]=0;start(0x84e3,0x1d0,0);nmi_counter();assert(ram[0x23c]==255);instruction_pc=0x84e3;interp816_runOpcode(active);assert(active->a==255&&!active->z);interp816_free(active);
 printf("PASS%u timer/NMI boundary interleavings,%u fullfades (all16 writes), originalzero-store FF regression witnessed\n",cases,fades);return 0;
}
