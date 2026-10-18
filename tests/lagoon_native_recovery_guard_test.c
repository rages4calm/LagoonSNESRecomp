/* Guard-only test: original code comes from the owner's pinned clean ROM.
 * The framework's real token/stack/generation logic is tested separately. */
#include "lagoon_native_recovery.h"
#include "snes/interp_bridge.h"
#include "common_rtl.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t rom[0x100000],snapshot[0x100000];
const uint8_t *g_rom;
static InterpAotContinuation entries[12];
static InterpAotContinuationGuard source_guard;
static unsigned registrations,checks;
uint64_t RtlStateGeneration(void){return 1;}
void interp_bridge_set_aot_continuation_hook(InterpAotContinuationHook hook){(void)hook;}
int interp_bridge_set_aot_continuations(const InterpAotContinuation *d,
    unsigned n,InterpAotContinuationGuard guard) {
  assert(n<=12);memcpy(entries,d,n*sizeof(*d));registrations=n;source_guard=guard;
  return 0; /* Keep registration retryable for null/source tests. */
}
static void denied(InterpAotContinuation d) {
  InterpAotContinuation before=d;
  assert(!source_guard(&d));assert(!memcmp(&d,&before,sizeof(d)));checks++;
}
int main(int argc,char **argv) {
  assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
  assert(fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);
  LagoonNativeRecovery_BeforeFrame();assert(!registrations);
  g_rom=rom;LagoonNativeRecovery_BeforeFrame();assert(registrations==9&&source_guard);
  for(unsigned n=0;n<9;n++){assert(source_guard(entries+n));checks++;}
  InterpAotContinuation good=entries[4],d;
  assert(good.call_pc24==0x01930f && good.continuation_pc24==0x019313);
  assert(good.owner_return_pc24==0x019294 && !good.saved_stack_bytes && good.owner_frame==2);
  for(unsigned n=0;n<15;n++){
    d=good;
    switch(n){
      case 0:d.call_pc24++;break;case 1:d.wait_pc24++;break;
      case 2:d.owner_return_pc24++;break;case 3:d.return_sites[0]++;break;
      case 4:d.return_sites[1]++;break;case 5:d.d=1;break;case 6:d.m=0;break;
      case 7:d.x=1;break;case 8:d.e=1;break;case 9:d.db=0;break;
      case 10:d.wait_frame=2;break;case 11:d.saved_stack_bytes=1;break;
      case 12:d.owner_frame=3;break;case 13:d.continuation_pc24++;break;
      case 14:d.continuation_pc24=0x019314;break;
    }
    denied(d);
  }
  const unsigned spans[][2]={{0x985,22},{0x9291,3},{0x92c3,0x2e},{0x9306,0x18},{0xe9,0x20}};
  for(unsigned s=0;s<sizeof(spans)/sizeof(spans[0]);s++)for(unsigned i=0;i<spans[s][1];i++){
    unsigned at=spans[s][0]+i;rom[at]^=1;denied(good);rom[at]^=1;
    assert(source_guard(&good));checks++;
  }
  /* The native region ends BEFORE the original audio/render call. */
  rom[0x931e]^=1;assert(source_guard(&good));rom[0x931e]^=1;checks++;
  memcpy(snapshot,rom,sizeof(rom));assert(source_guard(&good));
  assert(!memcmp(rom,snapshot,sizeof(rom)));checks++;
  g_rom=NULL;denied(good);
  printf("PASS %u recovery source/metadata checks; nine registrations, old guards retained\n",checks);
  return 0;
}
