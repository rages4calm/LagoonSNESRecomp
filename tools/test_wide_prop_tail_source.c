/* Terminal draw-before-reset source oracle. Original loader data is private;
 * arranged prior offsets below are negative policy fixtures, not gameplay. */
#define main unused_remaining_main
#include "test_wide_remaining_actors.c"
#undef main
#include "lagoon_wide_multipart.h"
static unsigned steps,refusals,mutants;
static void tail_run(Interp816*c,unsigned entry,unsigned x){
 c->pc=entry;c->k=c->db=1;c->dp=0;c->sp=0x1d0;c->x=x;c->y=c->a=0;
 c->mf=1;c->xf=0;c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;
 ram[0x1d1]=0xef;ram[0x1d2]=0x7f;unsigned n=0;
 while(c->k!=1||c->pc!=0x7ff0){assert(++n<100000);interp816_runOpcode(c);}assert(c->sp==0x1d2);steps+=n;
}
static int matches(int offset){
 LagoonWideCell cells[64];int n=LagoonWideEntities_Decode(rom,sizeof rom,0,0x3e,
   (int16_t)word(ram,0x58c)-16,(int16_t)word(ram,0x58e)-46+offset,cells,64);assert(n==6);
 unsigned j=0;for(int i=0;i<n;++i){
  if(cells[i].y< -16||cells[i].y>=240)continue;if(j>=ram[0x236])return 0;
  unsigned a=0x2000+j*4,h=ram[0x2200+j/4]>>((j&3)*2)&3;
  if((cells[i].x&511)!=(ram[a]|((h&1)<<8))||(cells[i].y&255)!=ram[a+1]||
      cells[i].tile!=word(ram,a+2)||cells[i].size!=((h&2)?16:8))return 0;j++;
 }return j&&j==ram[0x236];
}
static void refuse(void){int v=12345;assert(!LagoonWideMultipart_PropTailDrawOffset(rom,sizeof rom,ram,0,&v)&&v==12345);refusals++;}
int main(int argc,char**argv){
 assert(argc==3);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 static uint8_t seed[sizeof ram],before[sizeof ram];f=fopen(argv[2],"rb");assert(f&&fread(seed,1,sizeof seed,f)==sizeof seed);fclose(f);
 Interp816*c=interp816_init(NULL,read_byte,write_byte);unsigned alternatives=0,current=0;
 for(int prior=-128;prior<128;++prior){
  memcpy(ram,seed,sizeof ram);ram[0x3ae]=255;ram[0x3af]=42;put(0x90,prior);put(0x92,prior);
  ram[0x236]=0;ram[0x2a2]=255;memset(ram+0x2000,0,544);tail_run(c,0xc44e,0x80);tail_run(c,0xd8da,0);
  assert(!ram[0x3ae]&&ram[0x3af]==42&&!word(ram,0x90)&&!word(ram,0x92));
  put(0x48c,1);put(0xec0,0x80);put(0xec2,word(ram,0x58e));put(0xec4,0xff05);
  memcpy(before,ram,sizeof ram);int candidate=12345;
  assert(LagoonWideMultipart_PropTailDrawOffset(rom,sizeof rom,ram,0,&candidate)&&candidate==1&&!memcmp(before,ram,sizeof ram));
  if(prior==1)assert(matches(candidate));else{assert(!matches(candidate));alternatives++;}
  if(prior==0){assert(matches(0));current++;}
 }
 /* Resting zero remains indistinguishable in RAM and must win through the
  * original complete OAM group, rather than unconditionally selecting+1. */
 memcpy(ram,seed,sizeof ram);ram[0x3ae]=0;ram[0x3af]=42;put(0x90,0);put(0x92,0);
 put(0x48c,1);put(0xec0,0x80);put(0xec2,word(ram,0x58e));put(0xec4,0xff05);
 const unsigned fields[]={0x48e,0x48f,0xec0,0xec1,0xec2,0xec4,0x582,0x584,0x58c,0x58e,0x598,0x59b,0x59c,0x90,0x91,0x92,0x93};
 for(unsigned j=0;j<sizeof fields/sizeof *fields;j++){ram[fields[j]]^=1;refuse();ram[fields[j]]^=1;}
 ram[0x580]^=0x40;refuse();ram[0x580]^=0x40;
 for(unsigned v=1;v<256;v++){ram[0x3ae]=v;refuse();}ram[0x3ae]=0;
 for(unsigned v=0;v<256;v++)if(v!=42){ram[0x3af]=v;refuse();}ram[0x3af]=42;
 const unsigned ranges[][2]={{0x0180f1,12},{0x01c026,164},{0x01d8da,31},{0x01d979,2},{0x01ddad,100},{0x01de11,84},{0x05acbc,9},{0x05c10a,3}};
 for(unsigned r=0;r<sizeof ranges/sizeof *ranges;r++)for(unsigned j=0;j<ranges[r][1];j++){
  unsigned a=ranges[r][0],o=(a>>16)*32768+(a&32767)+j;rom[o]^=1;refuse();rom[o]^=1;mutants++;
 }
 printf("prop tail source:256original C44E->D8DA/DE00 cases, %u noncandidate original OAM groups, %u current-zero control, %u source bytes/%u total refusals, %u original opcodes PASS. No guest or captured RAM writes by helper.\n",alternatives,current,mutants,refusals,steps);
 interp816_free(c);return 0;
}
