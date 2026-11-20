/* Actual original spawn and status4 dispatch oracle. Supply the owner ROM;
 * no ROM, extracted graphics or generated game code is embedded here. */
#include "lagoon_wide_entities.h"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t rom[0x100000],ram[0x20000],before[0x20000];
static unsigned instructions,refusals,mutations;
static unsigned word(unsigned a){return ram[a]|ram[a+1]<<8;}
static void put(unsigned a,unsigned v){ram[a]=(uint8_t)v;ram[a+1]=(uint8_t)(v>>8);}
static uint8_t read8(void*m,uint32_t a){
  (void)m;unsigned b=a>>16,p=a&65535;
  if(b==0x7e||b==0x7f)return ram[(b-0x7e)*65536+p];
  if((b&127)<64&&p<8192)return ram[p];
  assert((b&127)<32&&p>=32768);return rom[(b&31)*32768+(p&32767)];
}
static void write8(void*m,uint32_t a,uint8_t v){
  (void)m;unsigned b=a>>16,p=a&65535;
  if(b==0x7e||b==0x7f){ram[(b-0x7e)*65536+p]=v;return;}
  assert((b&127)<64&&p<8192);ram[p]=v;
}
static void run_banked(unsigned bank,unsigned pc,unsigned x,unsigned y,int rtl){
  Interp816*c=interp816_init(NULL,read8,write8);assert(c);
  c->k=c->db=bank;c->pc=pc;c->x=x;c->y=y;c->sp=0x1f0;c->dp=0;
  c->mf=1;c->xf=c->e=c->d=0;
  ram[0x1f1]=0xef;ram[0x1f2]=0x7f;if(rtl)ram[0x1f3]=(uint8_t)bank;
  unsigned n=0;while(c->k!=bank||c->pc!=0x7ff0){assert(++n<1000);interp816_runOpcode(c);}
  instructions+=n;assert(c->sp==0x1f2+(unsigned)rtl);interp816_free(c);
}
static void run(unsigned pc,unsigned x,unsigned y,int rtl){run_banked(1,pc,x,y,rtl);}
static void reject(void){
  uint8_t frame=0xad;memcpy(before,ram,sizeof ram);
  assert(!LagoonWideEntities_DormantJellyFrame(rom,sizeof rom,ram,0x40,&frame));
  assert(frame==0xad&&!memcmp(before,ram,sizeof ram));++refusals;
}
static void negatives(void){
  const unsigned fields[]={0,1,11,16,17,24,27,28,29,44};
  for(unsigned n=0;n<sizeof fields/sizeof *fields;n++){
    unsigned a=0x540+fields[n],old=ram[a];
    for(unsigned value=0;value<256;value++)if(value!=old){ram[a]=(uint8_t)value;reject();}
    ram[a]=(uint8_t)old;
  }
  const unsigned state[]={0x48e,0x48f,0x8040};
  for(unsigned n=0;n<sizeof state/sizeof *state;n++){
    unsigned old=ram[state[n]];ram[state[n]]^=1;reject();ram[state[n]]=(uint8_t)old;
  }
  const unsigned spans[][2]={{0x018c06,18},{0x018c54,74},{0x018c9e,74},
    {0x018ce8,89},{0x01ab31,45},{0x01b131,42},{0x01b15b,76},
    {0x05b387,56},{0x05965d,271}};
  for(unsigned n=0;n<sizeof spans/sizeof *spans;n++)for(unsigned k=0;k<spans[n][1];k++){
    unsigned a=(spans[n][0]>>16)*32768+(spans[n][0]&32767)+k;
    rom[a]^=1;reject();rom[a]^=1;++mutations;
  }
  uint8_t f=0xad;
  assert(!LagoonWideEntities_DormantJellyFrame(NULL,sizeof rom,ram,0x40,&f));
  assert(!LagoonWideEntities_DormantJellyFrame(rom,sizeof rom-1,ram,0x40,&f));
  assert(!LagoonWideEntities_DormantJellyFrame(rom,sizeof rom,NULL,0x40,&f));
  assert(!LagoonWideEntities_DormantJellyFrame(rom,sizeof rom,ram,0x40,NULL));
  for(unsigned entity=0;entity<=0x840;entity++)if(entity<0x40||entity>0x800||(entity&63))
    assert(!LagoonWideEntities_DormantJellyFrame(rom,sizeof rom,ram,entity,&f));
  assert(f==0xad);
}
int main(int argc,char**argv){
  assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  unsigned authored=0,selectors=0;
  for(unsigned n=26;n<=29;n++){
    memset(ram,0,sizeof ram);ram[0x7b]=0x5d;ram[0x7c]=0x96;ram[0x7d]=5;
    run(0x8c54,0,n*9,1);uint8_t*r=ram+0x540;
    assert(r[0]==0xc0&&r[24]==2&&r[27]==1&&r[1]==4&&r[28]==0&&r[29]==0&&r[16]==0&&r[17]==0&&r[44]==0);++authored;
    ram[0x48e]=3;put(0x54c,word(0x542));put(0x54e,word(0x544));uint8_t literal=255,preview=255;
    /* Original AI9574 refuses status4 before movement. The wide AI filter
     * also requires low status bits zero, so it grants no dormant override. */
    memcpy(before,ram,sizeof ram);run_banked(2,0x9574,0,0,0);
    assert(!memcmp(before,ram,sizeof ram)&&r[0]==0xc0&&r[11]==255);
    assert(LagoonWideEntities_CurrentFrame(rom,sizeof rom,ram,ram,0x40,&literal)&&literal==0);
    LagoonWideAnimation a={r[16],r[17]};assert(!LagoonWideEntities_FrameForContext(rom,sizeof rom,ram,r,&a,0,&literal));
    memcpy(before,ram,sizeof ram);
    assert(LagoonWideEntities_DormantJellyFrame(rom,sizeof rom,ram,0x40,&preview)&&preview==20);
    assert(!memcmp(before,ram,sizeof ram));
    LagoonWideCell expected[64],actual[64];
    memset(expected,0,sizeof expected);memset(actual,0,sizeof actual);
    int count=LagoonWideEntities_RecordCells(rom,sizeof rom,ram,0x40,preview,1,expected,64);assert(count>0);
    if(n==26)negatives();
    r[0]|=8;run(0xab31,0x40,0,0);assert(r[28]==preview&&r[1]==4&&r[17]==0&&ram[0x8040]==1);++selectors;
    assert(LagoonWideEntities_RecordCells(rom,sizeof rom,ram,0x40,r[28],1,actual,64)==count);
    assert(!memcmp(expected,actual,(size_t)count*sizeof *actual));
    /* The helper never replaces already-real emergence frames20..25. */
    for(unsigned phase=0;phase<6;phase++){r[0]=0xc0;r[17]=(uint8_t)phase;r[28]=(uint8_t)(20+phase);reject();
      assert(LagoonWideEntities_CurrentFrame(rom,sizeof rom,ram,ram,0x40,&literal)&&literal==20+phase);}
  }
  printf("PASS %u original authored spawns, %u original status4 first-draw oracles; %u original opcodes; %u readonly refusals/%u source-byte mutations. No fullgame/GPU proof.\n",authored,selectors,instructions,refusals,mutations);
}
