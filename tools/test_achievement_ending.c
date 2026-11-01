/* Focused policy proof using the real external ROM, synthetic CPU boundaries.
 * This does not simulate a campaign or claim a complete credits playback. */
#include "../src/lagoon_achievement_ending.c"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
static uint8_t rom[0x100000],ram[0x20000],before[0x20000],other_ram[0x20000];
static CpuState cpu;
static unsigned assertions;
static unsigned real_prefix_cases,real_prefix_steps;
#define CHECK(x) do{assert(x);assertions++;}while(0)
static void setup(void){
  LagoonAchievementEnding_Init(rom,sizeof rom);LagoonAchievementEnding_Reset(1);
  memset(&cpu,0,sizeof cpu);memset(ram,0,sizeof ram);cpu.ram=ram;cpu.P=CPU_P_M;
  cpu.m_flag=1;cpu.PB=2;cpu.DB=2;cpu.S=0x1ef0;
  ram[cpu.S+1]=0x3c;ram[cpu.S+2]=0xde;ram[cpu.S+3]=2;
  ram[0x1099]=7;
}
static int observe(unsigned pc,unsigned generation){
  CpuState original=cpu;memcpy(before,ram,sizeof ram);
  int result=LagoonAchievementEnding_Observe(&cpu,pc,generation);
  CHECK(!memcmp(&original,&cpu,sizeof cpu)&&!memcmp(before,ram,sizeof ram));return result;
}
int interp816_opcode_hook(uint32_t address){(void)address;return 0;}
static uint8_t rd(void *context,uint32_t address){
  (void)context;unsigned bank=address>>16,at=address&65535;
  if((bank&0x7f)<0x40&&at<0x2000)return ram[at];
  if(at>=0x8000)return rom[(bank&31)*32768+(at&32767)];
  assert(!"unexpected prefix bus read");return 0;
}
static void wr(void *context,uint32_t address,uint8_t value){
  (void)context;assert(((address>>16)&0x7f)<0x40&&(address&65535)<0x2000);
  ram[address&65535]=value;
}
static void alternate_setup(unsigned seed){
  setup();memset(ram,0,sizeof ram);
  /* Caller-boundary ancestry retained from matched final-ending WRAM:
   * S=01ED there, +8..9=18B8. The new inner JSL/PHB/JSL bytes below are
   * produced ONLY by the original SDK instructions, never injected. */
  ram[0x1f5]=0x18;ram[0x1f6]=0xb8;
  Interp816 *i=interp816_init(NULL,rd,wr);i->a=seed;i->x=seed*83;
  i->y=seed*197;i->sp=0x1f4;i->dp=0;i->db=1;i->k=1;i->pc=0xb911;
  i->e=0;interp816_setFlags(i,0x25);
  unsigned steps=0;
  while(((unsigned)i->k<<16|i->pc)!=0x02e99d&&steps++<40){
    interp816_runOpcode(i);real_prefix_steps++;
  }
  CHECK(steps<40&&i->sp==0x1ed&&i->db==2);
  cpu.A=i->a;cpu.X=i->x;cpu.Y=i->y;cpu.S=i->sp;cpu.D=i->dp;
  cpu.DB=i->db;cpu.PB=i->k;cpu.P=interp816_getFlags(i);cpu_p_to_mirrors(&cpu);
  interp816_free(i);CHECK(frame_kind(&cpu)==2);real_prefix_cases++;
}
int main(int argc,char **argv){
  assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  setup();CHECK(source_ok());CHECK(!observe(0x02e9bd,1));CHECK(!observe(0x02e99d,1));
  cpu.DB=0x7e;CHECK(observe(0x02e9bd,1));CHECK(!observe(0x02e9bd,1));
  CHECK(!observe(0x02e99d,1));CHECK(!observe(0x02e9bd,1));
  setup();CHECK(!observe(0x02e99d,1));CHECK(!observe(0x02e9bd,2));
  CHECK(!observe(0x02e99d,2));CHECK(observe(0x02e9bd,2));
  setup();CHECK(!observe(0x02e99d,1));LagoonAchievementEnding_Reset(1);CHECK(!observe(0x02e9bd,1));
  setup();CHECK(!observe(0x02e99c,1));CHECK(!observe(0x02e9bd,1));
  /* The entry counter is not constrained: E99D owns its initialization.
   * Only the balanced terminal boundary must carry the completed byte7. */
  setup();ram[0x1099]=0;ram[0x109a]=0x55;CHECK(!observe(0x02e99d,1));
  ram[0x1099]=7;ram[0x109a]=0;CHECK(observe(0x02e9bd,1));
  for(unsigned counter=0;counter<256;counter++)if(counter!=7){
    setup();CHECK(!observe(0x02e99d,1));ram[0x1099]=(uint8_t)counter;
    CHECK(!observe(0x02e9bd,1));ram[0x1099]=7;CHECK(!observe(0x02e9bd,1));
  }
  /* The adjacent byte is independently animated by the actual ED91 source;
   * real owner states carry208. Every palette phase must allow completion. */
  for(unsigned palette=0;palette<256;palette++){
    setup();CHECK(!observe(0x02e99d,1));ram[0x109a]=(uint8_t)palette;
    CHECK(observe(0x02e9bd,1));CHECK(!observe(0x02e9bd,1));
  }
  for(unsigned mutation=0;mutation<15;mutation++){
    setup();CHECK(!observe(0x02e99d,1));
    switch(mutation){
      case 0:cpu.PB=1;break;case 1:cpu.D=1;break;case 2:cpu.emulation=1;break;
      case 3:cpu.m_flag=0;break;case 4:cpu.x_flag=1;break;case 5:cpu._flag_D=1;break;
      case 6:cpu.P|=CPU_P_D;break;case 7:cpu.P|=CPU_P_X;break;case 8:cpu.P&=~CPU_P_M;break;
      case 9:ram[cpu.S+1]^=1;break;case 10:ram[cpu.S+2]^=1;break;case 11:ram[cpu.S+3]^=1;break;
      case 12:cpu.S++;break;case 13:cpu.S=0x1ffd;break;case 14:cpu.S=3;break;
    }
    CHECK(!observe(0x02e9bd,1));
  }
  setup();CHECK(!observe(0x02e99d,1));memcpy(other_ram,ram,sizeof ram);cpu.ram=other_ram;
  CHECK(!observe(0x02e9bd,1));
  setup();CHECK(!observe(0x02e99d,1));CpuState other=cpu;
  CHECK(!LagoonAchievementEnding_Observe(&other,0x02e9bd,1));
  CHECK(!LagoonAchievementEnding_Observe(NULL,0x02e99d,1));
  static const unsigned spans[][2]={{0x02e99d,0x22},{0x02edcb,0x1d7},{0x02de39,6}};
  for(unsigned i=0;i<3;i++)for(unsigned n=0;n<spans[i][1];n++){
    setup();CHECK(!observe(0x02e99d,1));unsigned at=(spans[i][0]>>16)*32768+(spans[i][0]&32767)+n;
    rom[at]^=1;CHECK(!observe(0x02e9bd,1));CHECK(!observe(0x02e99d,1));rom[at]^=1;
  }
  setup();LagoonAchievementEnding_Init(rom,sizeof rom-1);CHECK(!observe(0x02e99d,1));CHECK(!observe(0x02e9bd,1));
  setup();CHECK(!observe(0x02e99d,1));CHECK(!observe(0x01e9bd,1));CHECK(observe(0x02e9bd,1));
  for(unsigned seed=0;seed<16;seed++){
    alternate_setup(seed);CHECK(!observe(0x02e99d,1));
    ram[0x1099]=7;ram[0x109a]=seed*17;cpu.DB=0x7e;
    CHECK(observe(0x02e9bd,1));CHECK(!observe(0x02e9bd,1));
  }
  for(unsigned byte=0;byte<9;byte++){
    alternate_setup(0);CHECK(!observe(0x02e99d,1));ram[cpu.S+1+byte]^=1;
    ram[0x1099]=7;CHECK(!observe(0x02e9bd,1));CHECK(!observe(0x02e99d,1));
  }
  for(unsigned mutation=0;mutation<15;mutation++){
    alternate_setup(0);CHECK(!observe(0x02e99d,1));ram[0x1099]=7;
    switch(mutation){
      case 0:cpu.PB=1;break;case 1:cpu.D=1;break;case 2:cpu.emulation=1;break;
      case 3:cpu.m_flag=0;break;case 4:cpu.x_flag=1;break;case 5:cpu._flag_D=1;break;
      case 6:cpu.P|=CPU_P_D;break;case 7:cpu.P|=CPU_P_X;break;case 8:cpu.P&=~CPU_P_M;break;
      case 9:cpu.S++;break;case 10:cpu.S=3;break;case 11:cpu.S=0x1ff7;break;
      case 12:cpu.S=0x1ffd;break;case 13:LagoonAchievementEnding_Reset(1);break;
      case 14:LagoonAchievementEnding_Init(rom,sizeof rom-1);break;
    }
    CHECK(!observe(0x02e9bd,1));
  }
  for(unsigned palette=0;palette<256;palette++){
    alternate_setup(0);CHECK(!observe(0x02e99d,1));ram[0x1099]=7;ram[0x109a]=palette;
    CHECK(observe(0x02e9bd,1));
  }
  static const unsigned extra_spans[][2]={{0x01b90b,14},{0x0080b9,0x30},{0x028020,2},{0x01b816,4}};
  for(unsigned i=0;i<4;i++)for(unsigned n=0;n<extra_spans[i][1];n++){
    alternate_setup(0);CHECK(!observe(0x02e99d,1));
    unsigned at=(extra_spans[i][0]>>16)*32768+(extra_spans[i][0]&32767)+n;
    rom[at]^=1;ram[0x1099]=7;CHECK(!observe(0x02e9bd,1));CHECK(!observe(0x02e99d,1));rom[at]^=1;
  }
  /* Neither another valid caller nor a fresh object/generation may inherit
   * the original E99D arm, even when S and the completion byte match. */
  alternate_setup(0);CHECK(!observe(0x02e99d,1));
  memcpy(ram+cpu.S+1,"\x3c\xde\x02",3);ram[0x1099]=7;CHECK(!observe(0x02e9bd,1));
  setup();cpu.S=0x1ed;memcpy(ram+cpu.S+1,"\x3c\xde\x02",3);CHECK(!observe(0x02e99d,1));
  memcpy(ram+cpu.S+1,"\xe3\x80\x00\x01\x16\xb9\x01\x18\xb8",9);
  CHECK(!observe(0x02e9bd,1));
  alternate_setup(0);CHECK(!observe(0x02e99d,1));ram[0x1099]=7;CHECK(!observe(0x02e9bd,2));
  alternate_setup(0);CHECK(!observe(0x02e99d,1));ram[0x1099]=7;
  CpuState alternate_other=cpu;CHECK(!LagoonAchievementEnding_Observe(&alternate_other,0x02e9bd,1));
  alternate_setup(0);CHECK(!observe(0x02e99d,1));ram[0x1099]=7;
  memcpy(other_ram,ram,sizeof ram);cpu.ram=other_ram;CHECK(!observe(0x02e9bd,1));
  printf("PASS alternate ending %u original SDK call-prefix cases/%u opcodes: original B911/B913, bank2 selector10, real PHB/JSL80E0/JML80E6 frames, retained actual outer ancestry; strict additive caller/source/kind guards\n",real_prefix_cases,real_prefix_steps);
  printf("PASS ending witness %u assertions: real source spans,caller/mode/linear-stack/generation guards,terminal byte1099==7 with all256 independent109A palette phases,one-shot terminal event; CPU/RAM unchanged; synthetic boundaries,not campaign coverage\n",assertions);
  return 0;
}
