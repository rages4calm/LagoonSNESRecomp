/* Real production source guard; all original saved bytes are bridge-owned. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/lagoon_native_recovery.c"
static uint8_t rom[0x100000],ram[0x20000],snapshot[0x20000];
const uint8_t *g_rom;
uint64_t RtlStateGeneration(void){return 1;}
int interp_bridge_set_aot_continuations(const InterpAotContinuation*d,unsigned n,InterpAotContinuationGuard guard){(void)d;(void)n;(void)guard;return 1;}
void interp_bridge_set_aot_continuation_hook(InterpAotContinuationHook hook){(void)hook;}
int main(int argc,char **argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);
 assert(fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);g_rom=rom;unsigned checks=0;
 const unsigned spans[][2]={{0x9346,0x3c},{0xc643,0x98},{0x1ab,0x4c},{0x4de,0xb},{0x931e,3},{0xb446,3}};
 for(unsigned k=0;k<4;k++){
  assert(recovery_source(flash_entries+k));checks++;
  for(unsigned s=0;s<sizeof spans/sizeof spans[0];s++)for(unsigned n=0;n<spans[s][1];n++){
   unsigned at=spans[s][0]+n;rom[at]^=1;assert(!recovery_source(flash_entries+k));rom[at]^=1;checks++;
  }
  InterpAotContinuation d=flash_entries[k];
  for(unsigned n=0;n<sizeof d;n++){((uint8_t*)&d)[n]^=1;assert(!recovery_source(&d));((uint8_t*)&d)[n]^=1;checks++;}
  CpuState cpu={0};cpu.ram=ram;memcpy(snapshot,ram,sizeof ram);CpuState before=cpu;
  recovery_hook(INTERP_CONT_TAKE,&cpu,d.return_sites[0],d.continuation_pc24,&d,0,NULL);
  assert(!memcmp(ram,snapshot,sizeof ram));assert(!memcmp(&cpu,&before,sizeof cpu));checks++;
 }
 assert(counts.flash_render[0]==1&&counts.flash_render[1]==1);
 assert(counts.flash_wait[0]==1&&counts.flash_wait[1]==1);
 /* Refuse null ROM and do not authorize the SPC poll as a native entry. */
 g_rom=NULL;for(unsigned k=0;k<4;k++){assert(!recovery_source(flash_entries+k));checks++;}
 g_rom=rom;InterpAotContinuation poll=flash_entries[2];poll.continuation_pc24=0x01937c;
 assert(!recovery_source(&poll));checks++;
 printf("PASS %u original death-flash source/metadata/readonly cases; both owner sites and both real waits, SPC poll unpromoted\n",checks);
 return 0;
}
