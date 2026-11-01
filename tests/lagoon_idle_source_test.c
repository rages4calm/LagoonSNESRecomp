#include "lagoon_idle_poll.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv) {
  assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);
  uint8_t *rom=malloc(0x100000);assert(rom&&fread(rom,1,0x100000,f)==0x100000);fclose(f);
  assert(LagoonIdle_SourceMatches(rom+0x814e,rom+0x8088,rom+0x922));
  const unsigned offsets[]={0x814e,0x8088,0x922},lengths[]={10,3,3};
  for(unsigned w=0;w<3;++w)for(unsigned i=0;i<lengths[w];++i) {
    rom[offsets[w]+i]^=1;
    assert(!LagoonIdle_SourceMatches(rom+0x814e,rom+0x8088,rom+0x922));
    rom[offsets[w]+i]^=1;
  }
  assert(!LagoonIdle_SourceMatches(NULL,rom+0x8088,rom+0x922));
  assert(!LagoonIdle_SourceMatches(rom+0x814e,NULL,rom+0x922));
  assert(!LagoonIdle_SourceMatches(rom+0x814e,rom+0x8088,NULL));
  free(rom);puts("PASS: original idle source and all16 changed-byte/null refusals");return 0;
}
