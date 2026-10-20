/* Actual title provenance and extracted actual SDK policy. Synthetic frames
 * exercise refusal, not guest gameplay or a claimed original death outcome. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/lagoon_native_endgame.c"
static unsigned char rom[0x100000],ram[0x20000],snapshot[0x20000];
const uint8_t *g_rom;
static uint64_t generation=1;
uint64_t RtlStateGeneration(void){return generation;}
#include "endgame_policy_under_test.h"
static CpuState cpu;
static unsigned checks;
static void put(unsigned p,unsigned v){ram[p]=v;ram[p+1]=v>>8;}
static void setup(unsigned i,unsigned selector) {
  memset(&cpu,0,sizeof(cpu));memset(ram,0,sizeof(ram));generation=1;g_rom=rom;
  cpu.ram=ram;cpu.S=0x1F00;cpu.PB=0;cpu.DB=1;cpu.A=selector;
  cpu.X=0x40;cpu.Y=0xABCD;cpu.m_flag=1;cpu.P=CPU_P_M;
  put(cpu.S+1,owners[i].call+3);ram[cpu.S+3]=1;put(cpu.S+4,owners[i].owner-1);
  interp_bridge_set_aot_continuation_hook(LagoonNativeEndgameHook);
  continuation_discard_tokens();LagoonNativeEndgameBeforeFrame();
}
static void arm(unsigned i) {
  CpuState before=cpu;memcpy(snapshot,ram,sizeof(ram));
  (void)continuation_arm(&cpu,0x0080E9,owners[i].call,3,1);
  assert(!memcmp(&before,&cpu,sizeof(cpu))&&!memcmp(snapshot,ram,sizeof(ram)));checks++;
}
static void post(unsigned i,unsigned selector) {
  unsigned at=0x8000+(owners[i].table&32767)+selector*2;
  unsigned target=0x010000|word(rom+at),sp=cpu.S;
  cpu.S+=3;cpu.PB=1;cpu.A=target>>8&255;
  ram[0]=selector;put(1,owners[i].call+3);ram[3]=ram[6]=1;put(4,target);put(sp+2,cpu.Y);
}
static int take(unsigned target) {
  CpuState before=cpu;memcpy(snapshot,ram,sizeof(ram));InterpAotAncestor out={0};
  int result=continuation_indirect_long_tail(&cpu,0x008106,target,&out);
  assert(!memcmp(&before,&cpu,sizeof(cpu))&&!memcmp(snapshot,ram,sizeof(ram)));checks++;
  if(result)assert(out.pc24==target&&out.owner_frame==2&&out.entry_s==cpu.S&&
    out.owner_return_pc24==owners[inline_witness.index].owner);
  return result;
}
static void mutant(unsigned n,int after) {
  switch(n) {
    case 0:cpu.PB=after?0:1;break;case 1:cpu.DB=0;break;
    case 2:cpu.D=1;break;case 3:cpu.m_flag=0;break;
    case 4:cpu.x_flag=1;break;case 5:cpu.emulation=1;break;
    case 6:cpu.S++;break;case 7:cpu.S=0x1FFF;break;
    case 8:generation++;break;case 9:cpu.ram=NULL;break;
    case 10:cpu._flag_D=1;break;case 11:cpu.P|=CPU_P_D;break;
    case 12:cpu.P&=~CPU_P_M;break;case 13:cpu.P|=CPU_P_X;break;
    case 14:cpu.X++;break;case 15:cpu.Y++;break;
    case 16:cpu.A|=0x100;break;
  }
}
static void queue_setup(void) {
  setup(0,11);cpu.S=0x1E00;put(cpu.S+1,0x93F9);ram[cpu.S+3]=1;
  put(cpu.S+4,0x93DB);put(cpu.S+6,0x9293);
  assert(interp_bridge_set_aot_continuations(thor_entries,13,LagoonNativeEndgameSource));
  assert(continuation_arm(&cpu,0x008D82,0x0193F6,3,1));assert(queue.phase==1);
  /* The last real queue CALL has the same existing outer frames. */
  put(cpu.S+1,0x940B);assert(continuation_arm(&cpu,0x008D82,0x019408,3,1));
  unsigned old=cpu.S;cpu.S+=3;cpu.PB=1;
  InterpAotContinuation out={0};uint16_t owner=0;
  assert(continuation_take(&cpu,0x008DFD,0x01940C,old,&out,&owner));
  assert(owner==cpu.S&&queue.phase==2);cpu.S+=2;
}
int main(int argc,char**argv) {
  assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);g_rom=rom;
  assert(dispatch_source()&&thor_source());
  const unsigned selectors[]={11,10,12,0,1,3,6,10,11};
  const unsigned which[]={0,1,1,2,2,2,2,2,2};
  for(unsigned n=0;n<9;n++) {
    unsigned i=which[n],sel=selectors[n];setup(i,sel);arm(i);assert(inline_witness.phase==1);
    unsigned target=inline_witness.target;post(i,sel);assert(take(target));assert(!take(target));
    setup(i,sel);arm(i);target=inline_witness.target;post(i,sel);assert(take(target));
    continuation_event(INTERP_CONT_TAKE,&cpu,0x008106,target,NULL,cpu.S);assert(!inline_witness.phase);
  }
  setup(0,11);post(0,11);assert(!take(0x019382)); /* forged bytes alone */
  for(unsigned set=0;set<2;set++) {
    const SourceSpan *spans=set?wait_spans:dispatch_spans;
    unsigned count=set?sizeof(wait_spans)/sizeof(*wait_spans):sizeof(dispatch_spans)/sizeof(*dispatch_spans);
    for(unsigned s=0;s<count;s++)for(unsigned b=0;b<spans[s].count;b++) {
      unsigned at=((spans[s].pc>>16)&127)*0x8000+(spans[s].pc&32767)+b;
      if(!set) {
        setup(0,11);rom[at]^=1;arm(0);post(0,11);assert(!take(0x019382));rom[at]^=1;
        setup(0,11);arm(0);post(0,11);rom[at]^=1;assert(!take(0x019382));rom[at]^=1;
      }
      rom[at]^=1;assert(!LagoonNativeEndgameSource(thor_entries));rom[at]^=1;checks++;
    }
  }
  for(unsigned b=0;b<5;b++) {
    setup(0,11);ram[cpu.S+1+b]^=1;arm(0);post(0,11);assert(!take(0x019382));
  }
  for(unsigned n=0;n<17;n++) {
    setup(0,11);mutant(n,0);arm(0);
    if(n!=6&&n!=7&&n!=9)post(0,11);
    if(n!=8&&n!=14&&n!=15)assert(!take(0x019382));
    setup(0,11);arm(0);post(0,11);mutant(n,1);assert(!take(0x019382));
  }
  for(unsigned b=0;b<2;b++) {
    setup(0,11);arm(0);post(0,11);ram[cpu.S+1+b]^=1;assert(!take(0x019382));
    setup(0,11);arm(0);post(0,11);ram[inline_witness.sp+2+b]^=1;assert(!take(0x019382));
  }
  for(unsigned b=0;b<7;b++){setup(0,11);arm(0);post(0,11);ram[b]^=1;assert(!take(0x019382));}
  setup(0,11);arm(0);post(0,11);generation++;assert(!take(0x019382));
  setup(0,11);arm(0);continuation_discard_tokens();post(0,11);assert(!take(0x019382));
  setup(0,11);arm(0);continuation_event(INTERP_CONT_CALL,&cpu,0x018000,0x008000,NULL,0);post(0,11);assert(!take(0x019382));
  for(unsigned i=0;i<3;i++)for(unsigned sel=0;sel<16;sel++) {
    setup(i,sel);arm(i);unsigned target=0x010000|word(rom+0x8000+(owners[i].table&32767)+sel*2);
    post(i,sel);assert(take(target)==approved(i,target));
  }
  for(unsigned i=0;i<13;i++){assert(LagoonNativeEndgameSource(thor_entries+i));
    InterpAotContinuation d=thor_entries[i];d.owner_return_pc24++;assert(!LagoonNativeEndgameSource(&d));checks+=2;}
  /* All new descriptors retain interpretation for decimal arithmetic and
   * inconsistent packed-width caches, at both real-call arm and RTL take. */
  for(unsigned i=0;i<13;i++)for(unsigned bad=10;bad<14;bad++) {
    for(unsigned when=0;when<2;when++) {
      setup(0,11);cpu.S=0x1E00;const InterpAotContinuation*d=thor_entries+i;
      put(cpu.S+1,d->continuation_pc24-1);ram[cpu.S+3]=1;
      put(cpu.S+4,d->owner_return_pc24-1);
      assert(interp_bridge_set_aot_continuations(thor_entries,13,LagoonNativeEndgameSource));
      if(!when)mutant(bad,0);
      uint64_t token=continuation_arm(&cpu,d->wait_pc24,d->call_pc24,3,1);
      if(!when){assert(!token);checks++;continue;}
      assert(token);unsigned before=cpu.S;cpu.S+=3;cpu.PB=1;mutant(bad,1);
      InterpAotContinuation result={0};uint16_t owner=0;
      assert(!continuation_take(&cpu,d->return_sites[0],d->continuation_pc24,before,&result,&owner));checks++;
    }
  }
  queue_setup();InterpAotAncestor out={0};assert(continuation_ancestor(&cpu,thor_entries+12,queue.sp+3,&out));
  assert(out.pc24==0x0193DC&&out.owner_return_pc24==0x019294&&out.entry_s==cpu.S);checks++;
  assert(!continuation_ancestor(&cpu,thor_entries+12,queue.sp+3,&out));
  for(unsigned n=0;n<17;n++) {queue_setup();mutant(n,1);
    if(n==14||n==15||n==16)continue; /* queue helpers may change AX/Y */
    assert(!continuation_ancestor(&cpu,thor_entries+12,queue.sp+3,&out));checks++;}
  for(unsigned b=0;b<4;b++){queue_setup();ram[queue.sp+4+b]^=1;assert(!continuation_ancestor(&cpu,thor_entries+12,queue.sp+3,&out));checks++;}
  printf("PASS %u actual endgame-title/SDK guard checks; complete source/inline-payload/frame/CPU/RAM/mode/generation mutants, one-shot9landing authority, all13wait descriptors and real-frame queue ancestor; synthetic guard scope, read-only guest state\n",checks);
  return 0;
}
