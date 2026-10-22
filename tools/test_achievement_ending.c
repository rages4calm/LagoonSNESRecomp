/* Focused policy proof using the real external ROM, synthetic CPU boundaries.
 * This does not simulate a campaign or claim a complete credits playback. */
#include "../src/lagoon_achievement_ending.c"
#include <assert.h>
#include <stdio.h>
static uint8_t rom[0x100000],ram[0x20000],before[0x20000],other_ram[0x20000];
static CpuState cpu;
static unsigned assertions;
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
  printf("PASS ending witness %u assertions: real source spans,caller/mode/linear-stack/generation guards,terminal byte1099==7 with all256 independent109A palette phases,one-shot terminal event; CPU/RAM unchanged; synthetic boundaries,not campaign coverage\n",assertions);
  return 0;
}
