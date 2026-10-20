/* Exercise the actual readonly production policy, including all four RTL slots. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/lagoon_native_revival.c"
static unsigned char rom[0x100000],ram[0x20000],snapshot[0x20000];
const uint8_t *g_rom;
static uint64_t generation=1;
uint64_t RtlStateGeneration(void){return generation;}
static CpuState cpu;
static unsigned checks;
static uint16_t initial_s=0x1fe0;
static int hook(unsigned event,unsigned source,unsigned target,
    const InterpAotContinuation *d,unsigned owner,InterpAotAncestor *out) {
  CpuState before=cpu;memcpy(snapshot,ram,sizeof(ram));
  int ok=LagoonNativeRevivalHook(event,&cpu,source,target,d,(uint16_t)owner,out);
  assert(!memcmp(&before,&cpu,sizeof(cpu)) && !memcmp(snapshot,ram,sizeof(ram)));
  checks++;return ok;
}
static void setup(unsigned outer) {
  memset(&cpu,0,sizeof(cpu));memset(ram,0,sizeof(ram));cpu.ram=ram;
  cpu.S=initial_s;cpu.PB=0;cpu.DB=cpu.m_flag=1;generation=1;g_rom=rom;
  uint16_t r=(uint16_t)(outer-1);
  unsigned char stack[]={0xDF,0xCD,1,0x34,0x12,0x78,0x56,0xD3,0xD8,(uint8_t)r,(uint8_t)(r>>8)};
  memcpy(ram+cpu.S+1,stack,sizeof(stack));
  hook(INTERP_CONT_DISCARD,0,0,NULL,0,NULL);
  hook(INTERP_CONT_CALL,0x01CDDC,0x0081B5,NULL,0,NULL);
}
static void take(const InterpAotContinuation *d) {
  cpu.S=initial_s+3;cpu.PB=1;
  hook(INTERP_CONT_TAKE,0x0081BD,0x01CDE0,d,initial_s+7,NULL);
}
static int returned(const InterpAotContinuation *d,InterpAotAncestor *out) {
  cpu.S=initial_s+9;cpu.PB=1;
  return hook(INTERP_CONT_RETURN,0,0x01D8D4,d,initial_s+7,out);
}
int main(int argc,char **argv) {
  assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
  assert(fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);g_rom=rom;
  InterpAotContinuation d;assert(LagoonNativeRevivalDescriptors(&d,1)==1);
  assert(!LagoonNativeRevivalDescriptors(&d,0));assert(!LagoonNativeRevivalDescriptors(NULL,1));
  assert(LagoonNativeRevivalSource(&d));
  /* Every descriptor byte, including unused return_sites[2]/[3], is guarded. */
  for(unsigned i=0;i<sizeof(d);i++) {
    InterpAotContinuation bad=d;((unsigned char*)&bad)[i]^=1;
    assert(!LagoonNativeRevivalSource(&bad));checks++;
  }
  const unsigned span[][2]={{0xCDB7,44},{0xD8CE,12},{0xD3CD,28},
    {0xC643,0x98},{0x01AB,0x4C},{0x04DE,0xB}};
  /* Bank-zero physical offsets equal PC&7fff; bank-one equal PC&ffff. */
  for(unsigned i=0;i<sizeof(span)/sizeof(span[0]);i++)for(unsigned j=0;j<span[i][1];j++) {
    rom[span[i][0]+j]^=1;assert(!LagoonNativeRevivalSource(&d));
    rom[span[i][0]+j]^=1;checks++;
  }
  const unsigned owners[]={0x01BAFB,0x01DD22,0x01D45F,0x01D5B0};
  for(unsigned i=0;i<4;i++) {
    InterpAotAncestor out;
    setup(owners[i]);assert(witness.phase==1);take(&d);assert(witness.phase==2);
    assert(returned(&d,&out));assert(out.pc24==0x01D8D4 && out.owner_return_pc24==owners[i]);
    assert(out.entry_s==initial_s+9 && out.owner_frame==2);assert(!returned(&d,&out));
    setup(owners[i]);assert(!returned(&d,&out)); /* No real TAKE. */
    setup(owners[i]);take(&d);assert(!returned(&d,NULL));assert(!returned(&d,&out));
    /* Mutate each witnessed byte after CALL and separately after TAKE. */
    for(unsigned stage=0;stage<2;stage++)for(unsigned b=0;b<11;b++) {
      setup(owners[i]);if(stage)take(&d);ram[initial_s+1+b]^=1;
      if(!stage)take(&d);assert(!returned(&d,&out));assert(!witness.phase);
    }
    for(unsigned stage=0;stage<2;stage++)for(unsigned bad=0;bad<10;bad++) {
      setup(owners[i]);if(stage)take(&d);
      switch(bad){case 0:generation++;break;case 1:cpu.DB=2;break;
        case 2:cpu.D=1;break;case 3:cpu.m_flag=0;break;case 4:cpu.x_flag=1;break;
        case 5:cpu.emulation=1;break;case 6:rom[0xCDB7]^=1;break;
        case 7:rom[(owners[i]-3)&65535]^=1;break;
        case 8:hook(INTERP_CONT_DISCARD,0,0,NULL,0,NULL);break;
        case 9:cpu.S=initial_s;cpu.PB=0;hook(INTERP_CONT_CALL,0x01CDDC,0x123456,NULL,0,NULL);break;}
      if(!stage)take(&d);assert(!returned(&d,&out));
      if(bad==6)rom[0xCDB7]^=1;
      if(bad==7)rom[(owners[i]-3)&65535]^=1;
    }
    for(unsigned slot=2;slot<4;slot++) {
      setup(owners[i]);take(&d);InterpAotContinuation bad=d;bad.return_sites[slot]=0x81BD;
      assert(!returned(&bad,&out));assert(!returned(&d,&out));
      setup(owners[i]);bad=d;bad.return_sites[slot]=0x81BD;take(&bad);assert(!returned(&d,&out));
    }
    setup(owners[i]);take(&d);cpu.S=initial_s+8;cpu.PB=1;
    assert(!hook(INTERP_CONT_RETURN,0,0x01D8D4,&d,initial_s+7,&out));
    setup(owners[i]);cpu.S=initial_s+3;cpu.PB=1;
    hook(INTERP_CONT_TAKE,0x0081BD,0x01CDE0,&d,initial_s+6,NULL);assert(!returned(&d,&out));
  }
  setup(0x01ABCD);assert(!witness.phase);take(&d);InterpAotAncestor out;assert(!returned(&d,&out));
  const unsigned ancestor_span[][2]={{0x9226,0x34},{0xD481,6},{0xD487,0x54}};
  for(unsigned k=0;k<3;k++)for(unsigned j=0;j<ancestor_span[k][1];j++) {
    setup(owners[0]);rom[ancestor_span[k][0]+j]^=1;take(&d);
    assert(!returned(&d,&out));rom[ancestor_span[k][0]+j]^=1;
  }
  setup(owners[0]);cpu.PB=1;
  hook(INTERP_CONT_CALL,0x01CDDC,0x0081B5,NULL,0,NULL);assert(!witness.phase);
  setup(owners[0]);cpu.PB=0;cpu.S=initial_s+3;
  hook(INTERP_CONT_TAKE,0x0081BD,0x01CDE0,&d,initial_s+7,NULL);assert(!returned(&d,&out));
  setup(owners[0]);take(&d);cpu.PB=0;cpu.S=initial_s+9;
  assert(!hook(INTERP_CONT_RETURN,0,0x01D8D4,&d,initial_s+7,&out));
  setup(owners[0]);cpu.PB=1;cpu.S=initial_s+4;
  hook(INTERP_CONT_TAKE,0x0081BD,0x01CDE0,&d,initial_s+7,NULL);assert(!returned(&d,&out));
  setup(owners[0]);take(&d);cpu.PB=0;cpu.S=initial_s-1;
  hook(INTERP_CONT_CALL,0x01CDDC,0x0081B5,NULL,0,NULL);assert(!witness.phase);
  assert(!returned(&d,&out)); /* Actual nested frame cannot borrow the old token. */
  /* Both inclusive valid bound and overflow of the eleven actual bytes. */
  initial_s=0x1ff4;setup(owners[0]);assert(witness.phase);take(&d);assert(returned(&d,&out));
  initial_s=0x1ff5;setup(owners[0]);assert(!witness.phase);
  printf("%u production revival policy/source checks PASS\n",checks);return 0;
}
