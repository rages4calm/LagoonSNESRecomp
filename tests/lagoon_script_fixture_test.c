#include "../src/lagoon_script_fixture.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"failure line%d case%u\n",__LINE__,n);return 2;}}while(0)
static uint8_t rom[0x100000],ram[0x20000],copy[0x20000];
int main(int argc,char**argv){unsigned n=0;CHECK(argc==2);FILE*f=fopen(argv[1],"rb");CHECK(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 const char *invalid[]={NULL,"","0","030","39x"," 30","31","0x1e","30,39","40","Text","text "};
 for(unsigned k=0;k<sizeof invalid/sizeof*invalid;k++){CHECK(!LagoonScriptFixture_Parse(invalid[k]));n++;}
 CHECK(LagoonScriptFixture_Parse("30")==30&&LagoonScriptFixture_Parse("39")==39&&LagoonScriptFixture_Parse("text")==40);
 const unsigned selectors[]={30,39,40};
 for(unsigned which=0;which<3;which++)for(n=0;n<38;n++){unsigned sel=selectors[which];
  memset(ram,0,sizeof ram);CpuState c={0};c.ram=ram;c.PB=c.DB=1;c.m_flag=1;c.P=CPU_P_M;c.S=0x180;c.Y=0x740;c.A=0xc3;
  ram[0x181]=0x9c;ram[0x182]=0x80;ram[0x9d]=0;ram[0xc40]=0xc8;ram[0xc58]=1;ram[0xc5b]=0;ram[0x1030]=0x40;ram[0x1031]=7;ram[0xa1]=0xff;ram[0xa2]=0xc3;
  unsigned used=0,selector=sel;uint64_t gen=17,frame=17;const uint8_t *r=rom;size_t size=sizeof rom;const CpuState*current=&c;
  unsigned at=0;uint8_t old=0;
  switch(n){case 1:r=NULL;break;case 2:size--;break;case 3:current=NULL;break;case 4:c.ram=NULL;break;case 5:used=1;break;case 6:selector=31;break;case 7:gen++;break;case 8:c.PB=0;break;case 9:c.DB=2;break;case 10:c.D=1;break;case 11:c.m_flag=0;break;case 12:c.x_flag=1;break;case 13:c.emulation=1;break;case 14:c._flag_D=1;break;case 15:c.P|=CPU_P_X;break;case 16:c.S=0xffff;break;case 17:c.X=1;break;case 18:c.Y=1;break;case 19:ram[0x181]^=1;break;case 20:ram[0x182]^=1;break;case 21:ram[0x9c]=1;break;case 22:ram[0x9d]=0x80;break;case 23:ram[0xc40]=0;break;case 24:ram[0xc58]=2;break;case 25:ram[0xc5b]=0x80;break;case 26:ram[0x1030]^=1;break;case 27:ram[0xa1]^=1;break;case 28:ram[0xa2]^=1;break;case 29:c.A^=1;break;
  case 30:at=0x809a;break;case 31:at=0x9b34;break;case 32:at=0xcc99;break;case 33:at=0xe9;break;case 34:at=0x2cab2;break;case 35:at=0x2c45b;break;case 36:at=0x2c45d;break;case 37:at=0x2c44f;break;}
  if(at){old=rom[at];rom[at]^=1;}memcpy(copy,ram,sizeof ram);CpuState before=c;unsigned before_used=used;
  int applied=LagoonScriptFixture_Apply(r,size,&c,current,gen,frame,selector,&used);
  CHECK(applied==(n==0));CHECK(!memcmp(&before,&c,sizeof c));
  if(!n){CHECK(used==1&&ram[0xa1]==(sel==30?0xaf:sel==39?0xb1:0x5b)&&ram[0xa2]==(sel==30?0xc6:sel==39?0xca:0xc4));copy[0xa1]=ram[0xa1];copy[0xa2]=ram[0xa2];CHECK(!memcmp(copy,ram,sizeof ram));CHECK(!LagoonScriptFixture_Apply(r,size,&c,current,gen,frame,selector,&used));}
  else CHECK(used==before_used&&!memcmp(copy,ram,sizeof ram));if(at)rom[at]=old;
 }
 printf("PASS fixture30/39/text: 3 positive pointer-only writes;111 read-only source/context/mode/owner/generation/one-shot refusals;12 bad environment values; no PC/stack/CPU/request/outcome mutation\n");return 0;
}
