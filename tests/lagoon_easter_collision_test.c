/* Execute the real original80C4->A4D3->A4EB movement prefix. Only the title
 * body query and runtime generation/registration services are mocked here. */
#include "../src/lagoon_easter_collision.c"
#include "snes/interp816.h"
#include <assert.h>
static uint8_t rom[0x100000],ram[0x20000],before[0x20000];
static uint64_t generation=1;
static int rollback,has_aot,body_enabled=1;
static unsigned steps,opcodes,refusals,registrations;
static CpuWramReadContext context;
uint64_t RtlStateGeneration(void){return generation;}
bool RtlIsRollbackLoad(void){return rollback!=0;}
int cpu_dispatch_has_entry(CpuState*c,uint32_t pc){(void)c;(void)pc;return has_aot;}
bool interp_bridge_add_pre_opcode_hook(uint32_t pc,InterpPreOpcodeHook f){assert(pc==0x180c4||pc==0x180c7);assert(f);registrations++;return true;}
int interp816_opcode_hook(uint32_t address){(void)address;return 0;}
static void put(unsigned a,int value){ram[a]=(uint8_t)value;ram[a+1]=(uint8_t)(value>>8);}
static int body(const uint8_t*r,uint64_t gen,int*x,int*y){
  if(!body_enabled||r!=ram||gen!=generation||word(r,0x48e))return 0;
  *x=1232;*y=1008;return 1;
}
static uint8_t rd(void*data,uint32_t address){
  (void)data;unsigned bank=address>>16,a=address&65535;
  if(bank==126||bank==127){a+=(bank-126)*65536;return LagoonEasterCollision_FilterRead(a,ram[a],&context,ram);}
  if((bank&127)<64&&a<8192)return LagoonEasterCollision_FilterRead(a,ram[a],&context,ram);
  assert((bank&127)<32&&a>=32768);return rom[(bank&127)*32768+(a&32767)];
}
static void wr(void*data,uint32_t address,uint8_t value){
  (void)data;unsigned bank=address>>16,a=address&65535;
  if(bank==126||bank==127){ram[(bank-126)*65536+a]=value;return;}
  assert((bank&127)<64&&a<8192);ram[a]=value;
}
static void sync_cpu(const Interp816*i,CpuState*c){
  memset(c,0,sizeof *c);c->ram=ram;c->S=i->sp;c->X=i->x;c->Y=i->y;c->A=i->a;
  c->PB=i->k;c->DB=i->db;c->D=i->dp;c->m_flag=i->mf;c->x_flag=i->xf;c->emulation=i->e;
  c->_flag_D=i->d;c->P=(i->mf?32:0)|(i->xf?16:0)|(i->d?8:0)|(i->i?4:0);
}
/* Independent signed segment/reference rule, separate from the production
 * helper. A point already inside may move away, never farther inward. */
static int reference(int start,int delta,int center,int radius){
  if(!delta)return 0;
  int d=start-center;
  if(d<=-radius)return delta>0&&start+delta>center-radius;
  if(d>=radius)return delta<0&&start+delta<center+radius;
  return d<0?delta>0:d>0?delta<0:0;
}
static void reset(int x,int y,int dx,int dy){
  memset(ram,0,sizeof ram);put(0x502,x);put(0x504,y);put(0x506,dx);put(0x508,dy);
  ram[0x500]=0xc8;put(0x520,18);ram[0x408]=21;put(0x1d1,0x7fef);
  rollback=has_aot=0;body_enabled=1;memset(&view,0,sizeof view);
}
static void source_step(int x,int y,int dx,int dy){
  reset(x,y,dx,dy);
  int stop_x=abs(y-1008)<10&&reference(x,dx,1232,18);
  int expected_x=x+(stop_x?0:dx);
  int stop_y=abs(expected_x-1232)<18&&reference(y,dy,1008,10);
  int expected_y=y+(stop_y?0:dy);
  Interp816*i=interp816_init(NULL,rd,wr);assert(i);i->pc=0x80c4;i->k=i->db=1;
  i->dp=0;i->sp=0x1d0;i->a=i->x=0;i->y=12;i->mf=1;i->xf=i->e=i->d=i->c=i->v=i->n=i->z=0;i->i=1;
  unsigned count=0;
  while(i->k!=1||i->pc!=0xa4d6){
    assert(++count<80);unsigned pc=(unsigned)i->k<<16|i->pc;
    context=(CpuWramReadContext){pc,i->sp,i->y,i->k,i->mf,i->xf,i->e,1};
    if(pc==0x180c4){
      CpuState cpu,was;sync_cpu(i,&cpu);was=cpu;memcpy(before,ram,sizeof ram);
      hook(&cpu,pc);assert(!memcmp(&cpu,&was,sizeof cpu));assert(!memcmp(before,ram,sizeof ram));
    }
    interp816_runOpcode(i);opcodes++;
  }
  assert(word(ram,0x502)==(unsigned)expected_x&&word(ram,0x504)==(unsigned)expected_y);
  assert((int16_t)word(ram,0x506)==dx&&(int16_t)word(ram,0x508)==dy);
  assert(i->sp==0x1ce); /* original balanced innerJSR; outerJSR remains live */
  interp816_free(i);steps++;
}
static CpuState arm(void){
  reset(1212,1008,5,0);CpuState c={0};c.ram=ram;c.PB=c.DB=c.m_flag=1;c.P=32;c.S=0x1d0;c.Y=12;
  hook(&c,0x180c4);assert(view.active);put(c.S-3,0xa4d5);put(c.S-1,0x80c6);
  context=(CpuWramReadContext){0x1a4f1,0x1cc,12,1,0,0,0,1};return c;
}
int main(int argc,char**argv){
  assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  LagoonEasterCollision_Init(rom,sizeof rom,body);assert(qualified);LagoonEasterCollision_BeforeFrame();assert(registrations==2);
  const int speeds[]={1,3,5};
  for(int x=1192;x<=1272;x++)for(int y=984;y<=1032;y++)for(unsigned speed=0;speed<3;speed++)for(unsigned direction=0;direction<4;direction++){
    int d=speeds[speed];source_step(x,y,direction==0?d:direction==2?-d:0,direction==1?d:direction==3?-d:0);
  }
  /* Fast swept crossings cannot tunnel; diagonal requests retain the free
   * axis so an edge/corner does not pin Nasir in place. */
  const int extra[][4]={{1170,1008,124,0},{1294,1008,-124,0},{1232,956,0,104},{1232,1060,0,-104},
    {1212,996,5,5},{1252,996,-5,5},{1212,1020,5,-5},{1252,1020,-5,-5},
    {1222,1008,-5,0},{1242,1008,5,0},{1232,1003,0,-5},{1232,1013,0,5}};
  for(unsigned n=0;n<sizeof extra/sizeof *extra;n++)source_step(extra[n][0],extra[n][1],extra[n][2],extra[n][3]);
  for(unsigned bad=0;bad<20;bad++){
    arm();uint32_t address=0x506;
    switch(bad){
      case 0:context.pb=2;break;case 1:context.m=1;break;case 2:context.x=1;break;case 3:context.e=1;break;
      case 4:context.stack--;break;case 5:context.y++;break;case 6:context.instruction_pc++;break;
      case 7:generation++;break;case 8:ram[0x408]++;break;case 9:ram[0x501]^=0x10;break;
      case 10:ram[0x514]++;break;case 11:ram[view.parent_s+1]^=1;break;case 12:ram[context.stack+1]^=1;break;
      case 13:ram[context.stack+3]^=1;break;case 14:ram[0x502]++;break;case 15:ram[0x504]++;break;
      case 16:ram[0x506]++;break;case 17:rollback=1;break;case 18:body_enabled=0;break;case 19:address=0x508;break;
    }
    memcpy(before,ram,sizeof ram);assert(LagoonEasterCollision_FilterRead(address,ram[address],&context,ram)==ram[address]);
    assert(!memcmp(before,ram,sizeof ram));refusals++;
  }
  for(unsigned bad=0;bad<13;bad++){
    CpuState c=arm();
    switch(bad){case 0:c.PB=2;break;case 1:c.DB=2;break;case 2:c.D=1;break;case 3:c.emulation=1;break;
      case 4:c.m_flag=0;break;case 5:c.x_flag=1;break;case 6:c.X=0x40;break;case 7:c.S=0x100;break;
      case 8:c._flag_D=1;break;case 9:c.P|=8;break;case 10:has_aot=1;break;case 11:ram[0x518]=1;break;case 12:body_enabled=0;break;}
    memcpy(before,ram,sizeof ram);hook(&c,0x180c4);assert(!view.active&&!memcmp(before,ram,sizeof ram));refusals++;
  }
  arm();context.interpreter=0;context.instruction_pc=~0u;assert(LagoonEasterCollision_FilterRead(0x506,5,&context,ram)==0);
  context.interpreter=1;context.instruction_pc=0x1a4f1;hook(NULL,0x180c7);assert(!view.active);
  const unsigned spans[][2]={{0x180c4,3},{0x1a4d3,24},{0x1a4eb,90},{0x1a39e,26},{0x1a493,64}};
  unsigned mutants=0;for(unsigned i=0;i<sizeof spans/sizeof *spans;i++)for(unsigned j=0;j<spans[i][1];j++){
    unsigned at=(spans[i][0]>>16)*32768+(spans[i][0]&32767)+j;rom[at]^=1;
    LagoonEasterCollision_Init(rom,sizeof rom,body);assert(!qualified);rom[at]^=1;mutants++;
  }
  printf("PASS cameo collision: %u original-opcode steps (%u opcodes), all four directions at1/3/5px, swept fast crossings, corner sliding and overlap escape; %u context refusals, %u exact source-byte refusals; hooks/readview never write guestRAM/CPU\n",steps,opcodes,refusals,mutants);
  return 0;
}
