/* Exact first-shake activation, original draw and original audio callee.
 * The two real SPC-port stores are recorded, without inventing any feedback. */
#define main unused_remaining_main
#include "test_wide_remaining_actors.c"
#undef main
#include "lagoon_wide_multipart.h"
static unsigned port_writes,source_steps;
static void onset_write(void *ctx,uint32_t address,uint8_t value){
 unsigned bank=address>>16,a=address&65535;
 if((bank&127)<64&&a>=0x2140&&a<=0x2143){assert(a==0x2141||a==0x2142);++port_writes;return;}
 write_byte(ctx,address,value);
}
static void bank_run(Interp816*c,unsigned entry,unsigned x){
 unsigned bank=entry>>16;c->pc=entry&65535;c->k=bank;c->db=1;c->dp=0;c->sp=0x1d0;c->x=x;c->y=c->a=0;
 c->mf=1;c->xf=0;c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;ram[0x1d1]=0xef;ram[0x1d2]=0x7f;
 unsigned n=0;while(c->k!=bank||c->pc!=0x7ff0){assert(++n<100000);interp816_runOpcode(c);}assert(c->sp==0x1d2);source_steps+=n;
}
static unsigned candidate_matches(void){
 LagoonWideCell cells[64];int n=LagoonWideEntities_Decode(rom,sizeof rom,0,0x3e,
     (int16_t)word(ram,0x58c)-16,(int16_t)word(ram,0x58e)-46,cells,64);assert(n==6);
 unsigned matched=0;for(int i=0;i<n;++i){
  if(cells[i].y< -16||cells[i].y>=240)continue;
  if(matched>=ram[0x236])return 0;unsigned a=0x2000+matched*4,h=(ram[0x2200+matched/4]>>((matched&3)*2))&3;
  if((cells[i].x&511)!=(ram[a]|((h&1)<<8))||(cells[i].y&255)!=ram[a+1]||
      cells[i].tile!=word(ram,a+2)||cells[i].size!=((h&2)?16:8))return 0;++matched;
 }return matched==ram[0x236];
}
static void queue_prop(void){put(0x48c,1);put(0xec0,0x80);put(0xec2,word(ram,0x58e));put(0xec4,0xff05);}
int main(int argc,char **argv){
 assert(argc==3);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 static uint8_t seed[sizeof ram],before[sizeof ram];f=fopen(argv[2],"rb");assert(f&&fread(seed,1,sizeof seed,f)==sizeof seed);fclose(f);
 Interp816*c=interp816_init(NULL,read_byte,onset_write);unsigned nonzero=0,alternate=0,mutants=0;
 for(int prior=-128;prior<128;++prior){
  memcpy(ram,seed,sizeof ram);ram[0x3ae]=255;ram[0x3af]=42;bank_run(c,0x01de00,0);
  assert(!ram[0x3ae]&&!word(ram,0x90)&&!word(ram,0x92));
  put(0x90,prior);put(0x92,prior);ram[0x390]=0;bank_run(c,0x029072,0x40);
  assert(ram[0x3ae]==255&&!ram[0x3af]&&(int16_t)word(ram,0x90)==prior);
  ram[0x236]=0;ram[0x2a2]=255;memset(ram+0x2000,0,544);bank_run(c,0x01c44e,0x80);
  unsigned writes=port_writes;bank_run(c,0x01d8da,0);assert(port_writes==writes+2);
  assert(ram[0x3af]==1&&word(ram,0x90)==0xfffc&&word(ram,0x92)==0xfffc);queue_prop();
  memcpy(before,ram,sizeof ram);int offset=12345;
  assert(LagoonWideMultipart_PropDrawOffset(rom,sizeof rom,ram,0,&offset)&&offset==0&&!memcmp(before,ram,sizeof ram));
  if(prior){assert(!candidate_matches());++nonzero;}else assert(candidate_matches());
 }
 /* All actual alternate-activation RNG seeds stay outside FF, and therefore
  * cannot borrow this first-phase candidate even when their old offset is0. */
 for(unsigned random=0;random<256;++random){
  memcpy(ram,seed,sizeof ram);ram[0x390]=0;ram[0x3b5]=random;bank_run(c,0x029f70,0);
  assert(ram[0x3ae]>=0x70&&ram[0x3ae]<=0xc7&&!ram[0x3af]);bank_run(c,0x01d8da,0);queue_prop();
  int offset=12345;assert(!LagoonWideMultipart_PropDrawOffset(rom,sizeof rom,ram,0,&offset)&&offset==12345);++alternate;
 }
 memcpy(ram,seed,sizeof ram);ram[0x3ae]=255;ram[0x3af]=1;put(0x90,0xfffc);put(0x92,0xfffc);queue_prop();
 const unsigned ranges[][2]={{0x029072,46},{0x029f70,88}};
 for(unsigned r=0;r<2;++r)for(unsigned j=0;j<ranges[r][1];++j){unsigned a=ranges[r][0],o=(a>>16)*32768+(a&32767)+j;rom[o]^=1;
  int offset=12345;assert(!LagoonWideMultipart_PropDrawOffset(rom,sizeof rom,ram,0,&offset)&&offset==12345);rom[o]^=1;++mutants;}
 printf("prop onset source: original reset->activation->C44E->D8DA/audio; %u nonzero-prior whole-OAM refusals/%u actual alternate activations/%u source-byte refusals; %u original opcodes/%u original SPC-port stores PASS. Only resting-zero candidate matches.\n",nonzero,alternate,mutants,source_steps,port_writes);
 interp816_free(c);return 0;
}
