#include "../src/lagoon_quest_guidance.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static uint8_t rom[0x100000],ram[0x20000],before[0x20000];
static void set(unsigned base,unsigned n) { ram[base+n/8]|=0x80u>>(n&7); }
int main(int argc,char **argv) {
  assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
  assert(fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  LagoonQuestGuidance_Init(rom,sizeof rom);assert(qualified);
  LagoonQuestGuidance out;
  ram[0x48e]=9;set(0x4e0,0x17);set(0x4d0,0x15);
  memcpy(before,ram,sizeof ram);
  assert(LagoonQuestGuidance_Get(ram,&out)&&out.room==9&&out.marker);
  assert(!strcmp(out.title,"Tablet of Faith")&&!memcmp(before,ram,sizeof ram));
  set(0x4d0,0x36);assert(LagoonQuestGuidance_Get(ram,&out));
  assert(!strcmp(out.title,"Tablet of Wish")&&out.room==8&&!out.marker);
  set(0x4d0,0x37);assert(LagoonQuestGuidance_Get(ram,&out));
  assert(!strcmp(out.title,"Tablet of Hope")&&out.room==10&&out.x==48&&out.y==496&&out.marker);
  set(0x4d0,0x38);assert(LagoonQuestGuidance_Get(ram,&out));
  assert(!strcmp(out.title,"Return to Voloh")&&out.room==9&&!out.marker);
  set(0x4e0,0x11);assert(!LagoonQuestGuidance_Get(ram,&out));
  memset(ram,0,sizeof ram);ram[0x48e]=9;
  assert(!LagoonQuestGuidance_Get(ram,&out));
  set(0x4e0,0x17);set(0x4d0,0x15);
  for(unsigned room=0;room<75;room++) {
    ram[0x48e]=room;assert(LagoonQuestGuidance_Get(ram,&out)==(room>=8&&room<=10));
  }
  unsigned mutants=0;
  for(unsigned n=0;n<sizeof spans/sizeof *spans;n++)for(unsigned j=0;j<spans[n].bytes;j++) {
    unsigned o=((spans[n].pc>>16)&127)*32768+(spans[n].pc&32767)+j;
    rom[o]^=1;LagoonQuestGuidance_Init(rom,sizeof rom);assert(!qualified);
    rom[o]^=1;mutants++;
  }
  LagoonQuestGuidance_Init(NULL,sizeof rom);assert(!qualified);
  LagoonQuestGuidance_Init(rom,sizeof rom-1);assert(!qualified);
  LagoonQuestGuidance_Init(rom,sizeof rom);assert(qualified);
  assert(!LagoonQuestGuidance_Get(NULL,&out)&&!LagoonQuestGuidance_Get(ram,NULL));
  printf("PASS read-only tablet guidance: original four stages/completion/prerequisites/75room refusals/%u source-byte mutations\n",mutants);
  return 0;
}
