/* Original draw-before-shake phase; private original room40 loader input.
 * Counter arrangements below are synthetic opcode tests, not live coverage. */
#define main unused_remaining_main
#include "test_wide_remaining_actors.c"
#undef main
#include "lagoon_wide_multipart.h"
static void runx(Interp816*c,unsigned entry,unsigned x){
 c->pc=entry;c->k=c->db=1;c->dp=0;c->sp=0x1d0;c->x=x;c->y=c->a=0;
 c->mf=1;c->xf=0;c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;
 ram[0x1d1]=0xef;ram[0x1d2]=0x7f;unsigned n=0;
 while(c->k!=1||c->pc!=0x7ff0){assert(++n<100000);interp816_runOpcode(c);}assert(c->sp==0x1d2);
}
static unsigned refused;
static void refusal(void){int out=12345;assert(!LagoonWideMultipart_PropDrawOffset(rom,sizeof rom,ram,0,&out)&&out==12345);++refused;}
int main(int argc,char**argv){
 assert(argc==3);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 static uint8_t seed[sizeof ram],before[sizeof ram];f=fopen(argv[2],"rb");assert(f&&fread(seed,1,sizeof seed,f)==sizeof seed);fclose(f);
 Interp816*c=interp816_init(NULL,read_byte,write_byte);unsigned cases=0,cells=0,legacy=0;
 for(unsigned phase=1;phase<42;phase++){
  memcpy(ram,seed,sizeof ram);int16_t used=word(rom,0xde11+(phase-1)*2),next=word(rom,0xde11+phase*2);
  put(0x90,used);ram[0x3ae]=255;ram[0x3af]=phase;ram[0x236]=0;ram[0x2a2]=255;memset(ram+0x2000,0,544);
  runx(c,0xc44e,0x80);unsigned slots=ram[0x236];uint8_t oam[544];memcpy(oam,ram+0x2000,544);
  runx(c,0xd8da,0);assert(ram[0x3af]==phase+1&&(int16_t)word(ram,0x90)==next);
  put(0x48c,1);put(0xec0,0x80);put(0xec2,word(ram,0x58e));put(0xec4,0xff05);
  memcpy(before,ram,sizeof ram);int recovered=12345;
  assert(LagoonWideMultipart_PropDrawOffset(rom,sizeof rom,ram,0,&recovered)&&recovered==used);
  assert(!memcmp(before,ram,sizeof ram));
  LagoonWideCell a[64],b[64];int na=LagoonWideEntities_RecordCells(rom,sizeof rom,ram,0x80,0x3e,0,a,64);
  int nb=LagoonWideEntities_Decode(rom,sizeof rom,0,0x3e,(int16_t)word(ram,0x58c)-16,
      (int16_t)word(ram,0x58e)-2*(ram[0x595]/3)-46+recovered,b,64);
  assert(na==6&&nb==6);if(memcmp(a,b,na*sizeof *a))legacy++;
  unsigned matched=0;for(int j=0;j<nb;j++){
   if(b[j].y< -16||b[j].y>=240)continue;assert(matched<slots);
   unsigned h=(oam[512+matched/4]>>((matched&3)*2))&3;
   assert((b[j].x&511)==(oam[matched*4]|((h&1)<<8))&&(b[j].y&255)==oam[matched*4+1]&&
       b[j].tile==word(oam,matched*4+2)&&b[j].size==((h&2)?16:8));matched++;cells++;
  }assert(matched==slots);cases++;
 }
 memcpy(before,ram,sizeof ram);
 const unsigned bytes[]={0x48e,0x48f,0xec0,0xec1,0xec2,0xec4,0x582,0x584,0x58c,0x58e,0x598,0x59b,0x59c,0x90,0x91,0x92,0x93};
 for(unsigned i=0;i<sizeof bytes/sizeof *bytes;i++){ram[bytes[i]]^=1;refusal();ram[bytes[i]]^=1;}
 ram[0x580]^=0x40;refusal();ram[0x580]^=0x40;
 ram[0x3ae]=0;refusal();ram[0x3ae]=before[0x3ae];
 for(unsigned p=0;p<256;p++)if(p<2||p>42){ram[0x3af]=p;refusal();}ram[0x3af]=before[0x3af];
 const unsigned ranges[][2]={{0x180f1,12},{0x1c026,164},{0x1d8da,31},{0x1d979,2},{0x1ddad,100},{0x1de11,84},{0x5acbc,9},{0x5c10a,3}};
 for(unsigned r=0;r<sizeof ranges/sizeof *ranges;r++)for(unsigned j=0;j<ranges[r][1];j++){
  unsigned a=ranges[r][0],o=(a>>16)*32768+(a&32767)+j;rom[o]^=1;refusal();rom[o]^=1;
 }
 assert(!memcmp(before,ram,sizeof ram));
 printf("prop shake source: %u original C44E->D8DA/DDAD phases/%u exact nativeOAM cells, %u legacy phase differences; %u transactional source/phase/slot/pose refusals PASS. No guest or captured RAM mutation by helper.\n",cases,cells,legacy,refused);
 interp816_free(c);return 0;
}
