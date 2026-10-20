/* Production title ancestor policy; original ROM, no guest writes by hooks. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/lagoon_native_recovery.c"
static uint8_t rom[0x100000],ram[0x20000],snapshot[0x20000];
const uint8_t *g_rom;
static uint64_t generation=1;
uint64_t RtlStateGeneration(void){return generation;}
int interp_bridge_set_aot_continuations(const InterpAotContinuation *d,unsigned n,InterpAotContinuationGuard guard){(void)d;(void)n;(void)guard;return 1;}
int interp_bridge_set_aot_tail_hook(uint32_t source,InterpAotTailHook hook){(void)source;(void)hook;return 1;}
void interp_bridge_set_aot_continuation_hook(InterpAotContinuationHook hook){(void)hook;}
static CpuState cpu;
static unsigned checks;
static void setup(unsigned owner) {
  memset(&cpu,0,sizeof(cpu));memset(ram,0,sizeof(ram));cpu.ram=ram;
  cpu.S=0x1fe0;cpu.DB=cpu.PB=1;cpu.m_flag=1;generation=1;g_rom=rom;
  uint16_t ret=(uint16_t)(boss_entries[owner+2].owner_return_pc24-1);
  uint8_t stack[]={0x34,0x12,(uint8_t)ret,(uint8_t)(ret>>8)};
  memcpy(ram+cpu.S+1,stack,4);
  recovery_hook(INTERP_CONT_DISCARD,NULL,0,0,NULL,0,NULL);
  recovery_hook(INTERP_CONT_TAKE,&cpu,0x81bd,0x1b80f,boss_entries+owner+2,cpu.S+2,NULL);
  assert(parent.phase==1);checks++;
}
static InterpAotContinuation child(int mp) {
  InterpAotContinuation d={mp?0x01D4C8:0x01D49E,0x008985,
    mp?0x01D4CC:0x01D4A2,mp?0x01B815:0x01B812,{0x00898D,0x00899A},0,1,0,0,1,3,2,2};
  assert(recovery_source(&d));return d;
}
static int returned(int mp) {
  InterpAotContinuation d=child(mp);InterpAotAncestor out;
  memcpy(snapshot,ram,sizeof(ram));CpuState before=cpu;
  int ok=recovery_hook(INTERP_CONT_RETURN,&cpu,0,d.owner_return_pc24,&d,cpu.S-2,&out);
  assert(!memcmp(snapshot,ram,sizeof(ram)));assert(!memcmp(&before,&cpu,sizeof(cpu)));checks++;
  if(ok){assert(out.pc24==d.owner_return_pc24&&out.owner_return_pc24==parent.owner);
    assert(out.entry_s==cpu.S+2&&out.owner_frame==2);}
  return ok;
}
int main(int argc,char **argv) {
  assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);
  assert(fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);g_rom=rom;
  for(unsigned owner=0;owner<2;owner++) {
    setup(owner);assert(returned(0));assert(parent.phase==2);assert(!returned(0));
    assert(returned(1));assert(!parent.phase);assert(!returned(1));
    setup(owner);assert(!returned(1));assert(returned(0));
    setup(owner);InterpAotAncestor done;
    assert(!recovery_hook(INTERP_CONT_RETURN,&cpu,0,parent.owner,boss_entries+owner+2,parent.entry_s,&done));
    assert(!parent.phase);assert(!returned(0)); /* Both children synchronous. */
    for(unsigned bad=0;bad<13;bad++) {
      setup(owner);
      switch(bad){case 0:generation++;break;case 1:cpu.m_flag=0;break;
        case 2:cpu.x_flag=1;break;case 3:cpu.emulation=1;break;case 4:cpu.DB=2;break;
        case 5:cpu.D=1;break;case 6:cpu.PB=0;break;case 7:cpu.S--;break;
        case 8:ram[parent.sp+1]^=1;break;case 9:ram[parent.sp+2]^=1;break;
        case 10:ram[parent.sp+3]^=1;break;case 11:ram[parent.sp+4]^=1;break;
        case 12:rom[0xb802]^=1;break;}
      assert(!returned(0));assert(!parent.phase);
      if(bad==12)rom[0xb802]^=1;
    }
    setup(owner);recovery_hook(INTERP_CONT_CALL,&cpu,0x1b80b,0x81b5,NULL,0,NULL);
    assert(!returned(0)); /* Even refused/new non-suspending call clears it. */
    setup(owner);cpu.S-=7;recovery_hook(INTERP_CONT_CALL,&cpu,0x1d49e,0x8985,NULL,0,NULL);
    assert(parent.phase==1);cpu.S+=7;assert(returned(0));
    setup(owner);cpu.S-=8;recovery_hook(INTERP_CONT_CALL,&cpu,0x1d49e,0x8985,NULL,0,NULL);
    assert(!parent.phase);cpu.S+=8;assert(!returned(0)); /* Nested child. */
    setup(owner);cpu.S-=4;InterpAotContinuation mp=child(1);
    recovery_hook(INTERP_CONT_TAKE,&cpu,0x899a,0x1d4cc,&mp,cpu.S+2,NULL);
    assert(parent.phase==2);cpu.S+=4;assert(returned(1)); /* Synchronous HP. */
    setup(owner);recovery_hook(INTERP_CONT_DISCARD,NULL,0,0,NULL,0,NULL);assert(!returned(0));
  }
  /* Whole source guard: each new owner's source span and metadata refuses. */
  const unsigned spans[][2]={{0xb7fe,0x1c},{0xc643,0x98},{0x1ab,0x4c},{0x4de,0xb},{0x80ca,3},{0xc6ac,3}};
  for(unsigned k=0;k<4;k++) {
    assert(recovery_source(boss_entries+k));
    for(unsigned a=0;a<sizeof(spans)/sizeof(spans[0]);a++)for(unsigned n=0;n<spans[a][1];n++) {
      unsigned at=spans[a][0]+n;rom[at]^=1;assert(!recovery_source(boss_entries+k));rom[at]^=1;checks++;
    }
    InterpAotContinuation d=boss_entries[k];
    for(unsigned n=0;n<sizeof(d);n++){uint8_t*b=(uint8_t*)&d;b[n]^=1;
      assert(!recovery_source(&d));b[n]^=1;checks++;}
  }
  /* Existing centered-camera source alternative keeps the original owner
   * through balanced saved registers and JMP A7B5. Every byte is exact. */
  const uint8_t camera[]={0x5A,0x9B,0xD0,0x16,0xDA,0x64,0x41,
    0x20,0xFB,0x83,0x20,0x26,0x84,0x20,0x52,0x84,0x20,0x7D,0x84,
    0xFA,0xAD,0x19,0x05,0x8D,0x19,0x05,0x7A,0x4C,0xB5,0xA7,
    0xEA,0xEA,0xEA,0xEA,0xEA};
  uint8_t stock_camera[35],stock_early[3];memcpy(stock_camera,rom+0x83d8,35);
  memcpy(stock_early,rom+0x809d,3);memcpy(rom+0x83d8,camera,35);
  memcpy(rom+0x809d,"\xea\xea\xea",3);rom[0x80cb]=0xd8;rom[0x80cc]=0x83;
  for(unsigned owner=0;owner<2;owner++){setup(owner);assert(returned(0));assert(returned(1));}
  const unsigned camera_spans[][2]={{0x83d8,35},{0x809d,3},{0x80c7,3},{0x80ca,3}};
  for(unsigned k=0;k<4;k++)for(unsigned a=0;a<4;a++)for(unsigned n=0;n<camera_spans[a][1];n++){
    unsigned at=camera_spans[a][0]+n;rom[at]^=1;assert(!recovery_source(boss_entries+k));rom[at]^=1;checks++;
  }
  memcpy(rom+0x83d8,stock_camera,35);memcpy(rom+0x809d,stock_early,3);
  rom[0x80cb]=0xb5;rom[0x80cc]=0xa7;
  printf("PASS %u production ancestor policy/source cases, both owners, readonly refusal and one-shot phases\n",checks);
  return 0;
}
