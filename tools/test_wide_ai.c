/* Bounded read-policy proof using an external original ROM and the captured
 * Elf Field WRAM. CPU boundaries below are synthetic; no campaign is played.
 * The original interpreter independently executes both real visibility gates. */
#include "../src/lagoon_wide_ai.c"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>

static uint8_t rom[0x100000],ram[0x20000],fixture[0x20000],before[0x20000];
static CpuState cpu;
static CpuWramReadContext context;
static unsigned checks,opcodes,gate_actor;
static int use_filter;
#define CHECK(x) do{assert(x);checks++;}while(0)
static void put(unsigned a,unsigned v){ram[a]=(uint8_t)v;ram[a+1]=(uint8_t)(v>>8);}
static void setup(int ai){
  memcpy(ram,fixture,sizeof ram);memset(&cpu,0,sizeof cpu);
  cpu.ram=ram;cpu.P=0x20;cpu.m_flag=1;cpu.PB=cpu.DB=ai?2:1;
  cpu.X=ai?0x1c0:0x200;cpu.Y=0x1234;cpu.S=0x1d0;
  put(cpu.S+1,ai?0x9572:0x80cc);put(cpu.S+3,0x806e);ram[cpu.S+5]=2;put(0x3b0,0x180);gate_actor=0x700;
  context=(CpuWramReadContext){ai?0x02973a:0x01ab31,cpu.S,cpu.Y,cpu.PB,1,0,0,1};
  LagoonWideAI_Init(rom,sizeof rom,&cpu);LagoonWideAI_BeginFrame(9,213,1);
}
static void arm(void){LagoonWideAI_ObserveOpcode(&cpu,context.instruction_pc,9);}
static uint8_t read_gate(void){
  CpuState saved=cpu;memcpy(before,ram,sizeof ram);
  uint8_t result=LagoonWideAI_FilterRead(gate_actor,ram[gate_actor],&context,ram,9);
  CHECK(!memcmp(&saved,&cpu,sizeof cpu));CHECK(!memcmp(before,ram,sizeof ram));return result;
}
static void refuse(void){uint8_t got=read_gate();if(got!=ram[gate_actor])fprintf(stderr,"refusal failed at check%u source%d\n",checks,source_ok());CHECK(got==ram[gate_actor]);}
static void accept(void){CHECK(read_gate()==(ram[gate_actor]|8));}
static uint8_t bus_read(void *unused,uint32_t address){
  (void)unused;int32_t offset=cpu_wram_offset(address>>16,(uint16_t)address);
  if(offset>=0){uint8_t v=ram[offset];return use_filter?LagoonWideAI_FilterRead(offset,v,&context,ram,9):v;}
  CHECK((address>>16)<32&&(address&0xffff)>=0x8000);
  return rom[(address>>16)*32768+(address&32767)];
}
static void bus_write(void *unused,uint32_t address,uint8_t value){
  (void)unused;int32_t offset=cpu_wram_offset(address>>16,(uint16_t)address);
  CHECK(offset>=0);ram[offset]=value;
}
static void sync_cpu(const Interp816 *i){
  cpu.A=i->a;cpu.X=i->x;cpu.Y=i->y;cpu.S=i->sp;cpu.D=i->dp;cpu.DB=i->db;cpu.PB=i->k;
  cpu.m_flag=i->mf;cpu.x_flag=i->xf;cpu.emulation=i->e;cpu._flag_D=i->d;
  cpu.P=(i->mf?32:0)|(i->xf?16:0)|(i->d?8:0)|(i->c?1:0)|(i->n?128:0)|(i->z?2:0)|(i->v?64:0)|(i->i?4:0);
}
static void original_gate(int ai,int filtered){
  setup(ai);use_filter=filtered;Interp816 *i=interp816_init(NULL,bus_read,bus_write);CHECK(i);
  i->k=i->db=ai?2:1;i->pc=ai?0x973a:0xab31;i->dp=0;i->sp=cpu.S;i->x=cpu.X;i->y=cpu.Y;
  i->mf=1;i->xf=i->e=i->d=0;
  for(unsigned n=0;n<3;n++){
    sync_cpu(i);context=(CpuWramReadContext){((uint32_t)i->k<<16)|i->pc,i->sp,i->y,i->k,i->mf,i->xf,i->e,1};
    if(n==0)LagoonWideAI_ObserveOpcode(&cpu,context.instruction_pc,9);
    interp816_runOpcode(i);opcodes++;
  }
  if(ai)CHECK(i->pc==(filtered?0x9744:0x9741));
  else CHECK(i->pc==(filtered?0xab38:0xab28));
  CHECK(ram[0x700]==0xc0);interp816_free(i);
}
static uint8_t result_ram[0x20000];
static void original_kind3(int filtered,unsigned distance){
  setup(1);use_filter=filtered;ram[0x700]=filtered?0xc0:0xc8;
  /* These are the four unmodified coordinate copies made by real9526 before
   * its second JSR9574. Keep the actual captured extra-view enemy in place. */
  put(0x103c,832-distance);put(0x103e,64);put(0x1040,832);put(0x1042,64);
  Interp816 *i=interp816_init(NULL,bus_read,bus_write);CHECK(i);
  i->k=i->db=2;i->pc=0x973a;i->sp=cpu.S;i->x=cpu.X;i->y=cpu.Y;i->dp=0;
  i->mf=1;i->xf=i->e=i->d=0;
  unsigned steps=0;
  while(i->k!=2||i->pc!=0x9573){
    CHECK(++steps<200);sync_cpu(i);
    context=(CpuWramReadContext){((uint32_t)i->k<<16)|i->pc,i->sp,i->y,i->k,i->mf,i->xf,i->e,1};
    if(i->pc==0x973a)LagoonWideAI_ObserveOpcode(&cpu,context.instruction_pc,9);
    interp816_runOpcode(i);opcodes++;
  }
  CHECK(i->sp==cpu.S+2);CHECK(ram[0x73f]==(distance<=112?2:0));
  /* Normalize only the deliberately different original flag byte, then
   * compare every actual AI write, stack residue, scratch byte and queue. */
  ram[0x700]=0xc0;
  if(!filtered)memcpy(result_ram,ram,sizeof ram);else CHECK(!memcmp(result_ram,ram,sizeof ram));
  interp816_free(i);
}
static void centered_patch(void){
  static const uint8_t code[35]={0x5a,0x9b,0xd0,0x16,0xda,0x64,0x41,0x20,0xfb,0x83,
    0x20,0x26,0x84,0x20,0x52,0x84,0x20,0x7d,0x84,0xfa,0xad,0x19,0x05,0x8d,0x19,0x05,
    0x7a,0x4c,0xb5,0xa7,0xea,0xea,0xea,0xea,0xea};
  rom[0x8408]=128;rom[0x8432]=128;rom[0x845f]=112;rom[0x8489]=112;rom[0x87f6]=128;
  memset(rom+0x809d,0xea,3);rom[0x80cb]=0xd8;rom[0x80cc]=0x83;memcpy(rom+0x83d8,code,sizeof code);
}
int main(int argc,char **argv){
  CHECK(argc==3);FILE *f=fopen(argv[1],"rb");CHECK(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  f=fopen(argv[2],"rb");CHECK(f&&fread(fixture,1,sizeof fixture,f)==sizeof fixture);fclose(f);
  CHECK(word(fixture,0x48e)==8&&word(fixture,0x702)==832&&word(fixture,0x480)==368&&fixture[0x700]==0xc0);
  setup(1);CHECK(source_ok());CHECK(ordinary(ram));arm();accept();refuse();
  CHECK(LagoonWideAI_AIReads()==1&&LagoonWideAI_AnimationReads()==0);
  setup(1);memcpy(ram+0x740,ram+0x700,64);gate_actor=0x740;cpu.X=0x200;
  put(0x3b0,0x200);put(cpu.S+1,0x954a);arm();accept();
  setup(0);arm();accept();CHECK(LagoonWideAI_AnimationReads()==1);
  setup(0);context.interpreter=0;context.instruction_pc=UINT32_MAX;accept();
  setup(1);context.interpreter=0;context.instruction_pc=UINT32_MAX;refuse();
  setup(1);refuse();
  setup(1);arm();context.instruction_pc++;refuse();
  setup(1);arm();context.y++;refuse();
  setup(1);arm();context.pb=1;refuse();
  setup(1);arm();context.stack++;refuse();
  setup(1);cpu.X++;arm();refuse();
  setup(1);CpuState impostor=cpu;LagoonWideAI_ObserveOpcode(&impostor,0x02973a,9);refuse();
  setup(1);arm();CHECK(LagoonWideAI_FilterRead(0x700,0xc0,&context,ram,10)==0xc0);
  setup(1);arm();LagoonWideAI_BeginFrame(10,213,1);refuse();
  const unsigned bad_mode[]={0,1,2,3,4,5};
  for(unsigned j=0;j<sizeof bad_mode/sizeof *bad_mode;j++){
    setup(1);switch(j){case 0:cpu.DB=1;break;case 1:cpu.D=1;break;case 2:cpu.emulation=1;break;
      case 3:cpu.m_flag=0;break;case 4:cpu.x_flag=1;break;case 5:cpu._flag_D=1;break;}
    arm();refuse();
  }
  for(int extra=-1;extra<=273;extra++)if(extra<=0||extra>272){setup(1);LagoonWideAI_BeginFrame(9,extra,1);arm();refuse();}
  setup(1);LagoonWideAI_BeginFrame(9,213,0);arm();refuse();
  const unsigned fields[]={0x406,0x100e,0x23d,0x275,0x500,0x518,0x520,0x521,0x50,0x53,0x55,0x6c,0x78,0x88,0x8a,0x8b,0x8d,0x492,0x494,0x701,0x718,0x71e};
  for(unsigned n=0;n<sizeof fields/sizeof *fields;n++){
    setup(1);ram[fields[n]]^=fields[n]==0x500?0x40:fields[n]==0x520?ram[fields[n]]:1;arm();refuse();
  }
  setup(1);put(0x48e,0x3f);arm();refuse();
  setup(1);put(cpu.S+1,0x954b);arm();refuse();
  setup(1);put(cpu.S+3,0x806f);arm();refuse();
  setup(1);ram[cpu.S+5]=1;arm();refuse();
  const unsigned invalid_stack[]={0x1fa,0x1fe,0x200,0x1fff};
  for(unsigned n=0;n<sizeof invalid_stack/sizeof *invalid_stack;n++){
    setup(1);cpu.S=context.stack=invalid_stack[n];put(cpu.S+1,0x9572);
    put(cpu.S+3,0x806e);ram[cpu.S+5]=2;arm();refuse();
    if(invalid_stack[n]>=0x1fe){setup(0);cpu.S=context.stack=invalid_stack[n];put(cpu.S+1,0x80cc);arm();refuse();}
  }
  setup(1);put(0x3b0,0x1c1);arm();refuse();
  setup(1);put(0x3b0,0x200);arm();refuse();
  setup(0);put(cpu.S+1,0x80cd);arm();refuse();
  setup(0);context.interpreter=0;context.instruction_pc=UINT32_MAX;cpu.X=0x240;refuse();
  const int positions[]={-214,-213,-1,0,255,256,468,469};
  for(unsigned n=0;n<sizeof positions/sizeof *positions;n++){
    setup(1);put(0x702,368+positions[n]);arm();
    if(positions[n]>=-213&&positions[n]<469&&(positions[n]<0||positions[n]>=256))accept();else refuse();
  }
  setup(1);put(0x704,224);arm();refuse();
  setup(1);put(0x704,0xffff);arm();refuse();
  setup(1);put(0x702,1536);put(0x480,1072);arm();refuse();
  setup(1);arm();CHECK(LagoonWideAI_FilterRead(0x701,ram[0x701],&context,ram,9)==ram[0x701]);
  setup(1);arm();CHECK(LagoonWideAI_FilterRead(0x700,0x80,&context,ram,9)==0x80);
  /* Every byte of each pinned source span revokes eligibility, including warm
   * state; source tests do not write or copy any ROM to disk. */
  unsigned mutated=0;
  for(unsigned n=0;n<sizeof source/sizeof *source;n++)for(unsigned j=0;j<source[n].bytes;j++){
    unsigned at=(source[n].pc>>16)*32768+(source[n].pc&32767)+j;
    setup(1);rom[at]^=1;LagoonWideAI_BeginFrame(9,213,1);arm();refuse();rom[at]^=1;mutated++;
  }
  setup(1);rom[0x2b2df+8*56]^=1;arm();refuse();rom[0x2b2df+8*56]^=1;
  original_gate(1,0);original_gate(1,1);original_gate(0,0);original_gate(0,1);
  const unsigned distances[]={32,111,112,113,336};
  for(unsigned n=0;n<sizeof distances/sizeof *distances;n++){
    original_kind3(0,distances[n]);original_kind3(1,distances[n]);
  }
  /* Authored aggro remains untouched: kind3's original distance is112. */
  CHECK(rom[0x117b6]==0x70&&rom[0x117b7]==0);
  centered_patch();setup(1);CHECK(source_ok());arm();accept();
  setup(0);context.interpreter=0;context.instruction_pc=UINT32_MAX;accept();
  const unsigned patched[]={0x809d,0x80cb,0x83d8,0x8408,0x8432,0x845f,0x8489};
  for(unsigned j=0;j<sizeof patched/sizeof *patched;j++){
    setup(1);rom[patched[j]]^=1;LagoonWideAI_BeginFrame(9,213,1);arm();refuse();rom[patched[j]]^=1;
  }
  printf("wide AI PASS: %u checks, %u original gate opcodes, %u source-byte mutations; no guest flag/queue writes\n",checks,opcodes,mutated);
  return 0;
}
