/* Actual production boss-script policy: original frames, no fabricated guest writes. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/lagoon_native_recovery.c"
static uint8_t rom[0x100000],ram[0x20000],snapshot[0x20000];
const uint8_t *g_rom;
static uint64_t generation=1;
uint64_t RtlStateGeneration(void){return generation;}
int interp_bridge_set_aot_continuations(const InterpAotContinuation*d,unsigned n,InterpAotContinuationGuard guard){(void)d;(void)n;(void)guard;return 1;}
int interp_bridge_set_aot_tail_hook(uint32_t source,InterpAotTailHook hook){(void)source;(void)hook;return 1;}
int interp_bridge_set_aot_indirect_long_tail_hook(uint32_t source,InterpAotTailHook hook){(void)source;(void)hook;return 1;}
void interp_bridge_set_aot_continuation_hook(InterpAotContinuationHook h){(void)h;}
static CpuState cpu;
static unsigned checks;
static void event(unsigned e,uint32_t source,uint32_t target,const InterpAotContinuation*d,uint16_t entry) {
  memcpy(snapshot,ram,sizeof(ram));CpuState before=cpu;
  assert(!recovery_hook(e,&cpu,source,target,d,entry,NULL));
  assert(!memcmp(snapshot,ram,sizeof(ram))&&!memcmp(&before,&cpu,sizeof(cpu)));checks++;
}
static void setup(unsigned i) {
  memset(&cpu,0,sizeof(cpu));memset(ram,0,sizeof(ram));generation=1;g_rom=rom;
  cpu.ram=ram;cpu.S=0x1fe0;cpu.PB=0;cpu.DB=1;cpu.m_flag=1;cpu.P=CPU_P_M;
  uint16_t ret=(uint16_t)(script_entries[i+2].owner_return_pc24-1);
  uint8_t stack[]={0xE0,0xB8,1,(uint8_t)ret,(uint8_t)(ret>>8),0x18,0xB8};
  memcpy(ram+cpu.S+1,stack,7);event(INTERP_CONT_DISCARD,0,0,NULL,0);
  event(INTERP_CONT_CALL,0x01B8DD,0x0084E0,NULL,0);
  assert(script_parent.phase==1);
}
static void take(unsigned i) {
  cpu.PB=1;cpu.S=0x1fe3;
  event(INTERP_CONT_TAKE,0x0084E8,0x01B8E1,script_entries+i+2,cpu.S);
}
static int returned(unsigned i) {
  InterpAotAncestor out={0};cpu.S=0x1fe5;
  memcpy(snapshot,ram,sizeof(ram));CpuState before=cpu;
  int ok=recovery_hook(INTERP_CONT_RETURN,&cpu,0,script_entries[i+2].owner_return_pc24,
    script_entries+i+2,0x1fe3,&out);
  assert(!memcmp(snapshot,ram,sizeof(ram))&&!memcmp(&before,&cpu,sizeof(cpu)));checks++;
  if(ok){assert(out.pc24==script_entries[i+2].owner_return_pc24&&out.entry_s==cpu.S&&
    out.owner_return_pc24==0x01B819&&out.owner_frame==2);}
  return ok;
}
int main(int argc,char**argv) {
  assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);assert(fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);g_rom=rom;
  for(unsigned i=0;i<8;++i) {
    assert(recovery_source(script_entries+i));checks++;
    for(unsigned b=0;b<sizeof(InterpAotContinuation);++b) {
      InterpAotContinuation bad=script_entries[i];((unsigned char*)&bad)[b]^=1;
      assert(!recovery_source(&bad));checks++;
    }
  }
  /* Every exact opcode/table byte matters, including the complete four-RTL
   * audio branch body and original inline dispatch/table. */
  const unsigned spans[][2]={{0x01B816,0x78},{0x028282,11},{0x0090DB,0x5f},{0x0080E9,0x20},
    {0x01B88E,0x54},{0x0084E0,9},{0x01B8E2,0x29},{0x018703,0x6f}};
  for(unsigned s=0;s<8;s++)for(unsigned b=0;b<spans[s][1];++b) {
    unsigned off=((spans[s][0]>>16)&127)*0x8000+(spans[s][0]&0x7fff)+b;
    unsigned which=s<4?1:s<6?2:7;
    rom[off]^=1;assert(!recovery_source(script_entries+which));rom[off]^=1;checks++;
  }
  for(unsigned i=0;i<5;i++) {
    setup(i);take(i);assert(script_parent.phase==2);assert(returned(i));assert(!returned(i));
    setup(i);assert(!returned(i)); /* no validated TAKE */
    for(unsigned b=0;b<7;b++) {
      setup(i);ram[0x1fe1+b]^=1;take(i);assert(!returned(i));
      setup(i);take(i);ram[0x1fe1+b]^=1;assert(!returned(i));
      setup(i);ram[0x1fe1+b]^=1;event(INTERP_CONT_CALL,0x01B8DD,0x0084E0,NULL,0);
      assert(!script_parent.phase);
    }
    setup(i);cpu.ram=NULL;event(INTERP_CONT_CALL,0x01B8DD,0x0084E0,NULL,0);assert(!script_parent.phase);cpu.ram=ram;
    setup(i);cpu.PB=1;cpu.S=0x1fe3;event(INTERP_CONT_TAKE,0x0084E7,0x01B8E1,script_entries+i+2,cpu.S);assert(!returned(i));
    setup(i);generation++;take(i);assert(!returned(i));
    setup(i);take(i);generation++;assert(!returned(i));
    setup(i);event(INTERP_CONT_DISCARD,0,0,NULL,0);take(i);assert(!returned(i));
    setup(i);take(i);event(INTERP_CONT_CALL,0x01B8DD,0x0084E0,NULL,0);assert(!returned(i));
    setup(i);cpu.PB=1;event(INTERP_CONT_CALL,0x01B8DD,0x0084E0,NULL,0);assert(!script_parent.phase);
    setup(i);take(i);event(INTERP_CONT_CALL,0x01B8DD,0x123456,NULL,0);assert(!returned(i));
    setup(i);take(i);event(INTERP_CONT_TAKE,0x0084E8,0x01B8E1,script_entries+i+2,cpu.S);assert(!returned(i));
    for(unsigned field=0;field<7;field++) {
      setup(i);take(i);
      switch(field){case 0:cpu.m_flag=0;break;case 1:cpu.x_flag=1;break;case 2:cpu.emulation=1;break;
        case 3:cpu.DB=0;break;case 4:cpu.D=1;break;case 5:cpu.PB=0;break;case 6:cpu.S--;break;}
      if(field==6){InterpAotAncestor out;assert(!recovery_hook(INTERP_CONT_RETURN,&cpu,0,
        script_entries[i+2].owner_return_pc24,script_entries+i+2,0x1fe3,&out));checks++;}
      else assert(!returned(i));
    }
    setup(i);take(i);rom[0xB8DD]^=1;assert(!returned(i));rom[0xB8DD]^=1;
    setup(i);take(i);assert(!returned((i+1)%5));
  }
  /* Source verified genuine seven-byte ownership is separate from the SDK's
   * five-byte child token. Unknown ancestors never gain native authority. */
  printf("PASS %u actual boss-script source/metadata/seven-byte ancestor policy cases; five real owner sites, readonly one-shot refusals\n",checks);
  return 0;
}
