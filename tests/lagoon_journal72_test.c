#include "../src/lagoon_journal.h"
#include "../src/lagoon_quest_guidance.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t rom[0x100000],ram[0x20000],before[0x20000];
static void set(unsigned base,unsigned n){ram[base+n/8]|=(uint8_t)(0x80u>>(n&7));}
static void check_readonly(LagoonJournalEntry *e){
 memcpy(before,ram,sizeof ram);assert(LagoonJournal_Get(ram,sizeof ram,e));
 assert(!memcmp(before,ram,sizeof ram));assert(e->chapter&&e->title&&e->detail);
 assert(e->milestone_count>=1&&e->milestone_count<=4);
 assert(e->completed_mask<(1u<<e->milestone_count));
}
int main(int argc,char **argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);
 assert(fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 LagoonJournal_Init(rom,sizeof rom);LagoonQuestGuidance_Init(rom,sizeof rom);
 LagoonJournalEntry e;
 check_readonly(&e);assert(!strcmp(e.title,"Bring Giles home")&&!e.completed_mask);
 set(0x4e0,0x10);check_readonly(&e);assert(!strcmp(e.title,"Back to Gold Cave")&&e.completed_mask==1);
 set(0x4d0,0x26);check_readonly(&e);assert(!strcmp(e.title,"Samson's locked door"));
 set(0x4e0,0);ram[0x48e]=9;check_readonly(&e);
 assert(!strcmp(e.chapter,"Voloh and Philips Castle")&&!e.completed_mask&&!e.marker);
 set(0x4e0,0x17);set(0x4d0,0x15);check_readonly(&e);
 assert(!strcmp(e.title,"Tablet of Faith")&&e.marker&&e.room==9);
 set(0x4d0,0x36);check_readonly(&e);assert(!strcmp(e.title,"Tablet of Wish")&&!e.marker);
 set(0x4d0,0x37);check_readonly(&e);assert(!strcmp(e.title,"Tablet of Hope")&&e.marker&&e.room==10);
 set(0x4d0,0x38);check_readonly(&e);assert(!strcmp(e.title,"Return to Voloh")&&!e.marker&&e.completed_mask==1);
 set(0x4e0,0x11);check_readonly(&e);assert(!strcmp(e.title,"Inside Philips Castle")&&e.completed_mask==3);
 /* The elder consumes tablets: completion must not roll back with possession. */
 memset(ram+0x4d0,0,16);check_readonly(&e);assert(e.completed_mask==3&&!e.marker);
 /* No history is cached: loading an earlier adventure immediately changes it. */
 memset(ram,0,sizeof ram);check_readonly(&e);assert(!strcmp(e.title,"Bring Giles home"));
 memset(ram+0x4c0,255,16);check_readonly(&e);assert(!e.defeated_boss_mask&&!e.completed_mask);
 for(unsigned mask=0;mask<128;mask++){
  memset(ram,0,sizeof ram);for(unsigned i=0;i<7;i++)if(mask&(1u<<i))set(0x4e0,i);
  check_readonly(&e);assert(e.defeated_boss_mask==mask);
 }
 /* Present key items refine advice; missing consumed items never fabricate
  * an NPC conversation or remove a completed story milestone. */
 memset(ram,0,sizeof ram);set(0x4e0,1);set(0x4d0,0x2f);check_readonly(&e);
 assert(!strcmp(e.title,"Return Thor's Pendant"));set(0x4d0,0x30);check_readonly(&e);
 assert(!strcmp(e.title,"The blocked cave"));set(0x4d0,0x31);check_readonly(&e);
 assert(!strcmp(e.title,"Show the Moon Stone"));set(0x4d0,0x2e);check_readonly(&e);
 assert(!strcmp(e.title,"Cool the shallow lava"));
 memset(ram,0,sizeof ram);set(0x4e0,2);set(0x4d0,0x29);check_readonly(&e);
 assert(!strcmp(e.title,"Bring the Mint to Poper"));set(0x4d0,0x2a);check_readonly(&e);
 assert(!strcmp(e.title,"Duma's chamber"));
 memset(ram,0,sizeof ram);set(0x4e0,3);set(0x4d0,0x33);check_readonly(&e);
 assert(!strcmp(e.title,"Find Lilaty's liar"));set(0x4d0,0x34);check_readonly(&e);
 assert(!strcmp(e.title,"Into Ice Cave"));
 memset(ram,0,sizeof ram);ram[0x48e]=74;check_readonly(&e);
 ram[0x48e]=75;assert(!LagoonJournal_Get(ram,sizeof ram,&e)&&!e.title);
 ram[0x48e]=0;ram[0x48f]=1;assert(!LagoonJournal_Get(ram,sizeof ram,&e));
 assert(!LagoonJournal_Get(NULL,sizeof ram,&e));
 assert(!LagoonJournal_Get(ram,LAGOON_JOURNAL_RAM_BYTES-1,&e));
 ram[0x48f]=0;
 assert(LagoonJournal_Get(ram,LAGOON_JOURNAL_RAM_BYTES,&e));
 assert(!LagoonJournal_Get(ram,sizeof ram,NULL));
 memset(ram,0,sizeof ram);
 unsigned positions[]={0,0x10d401,0x10000,0xfffff};
 for(unsigned i=0;i<sizeof positions/sizeof *positions;i++){
  unsigned at=positions[i]%sizeof rom;rom[at]^=1;LagoonJournal_Init(rom,sizeof rom);
  assert(!LagoonJournal_Get(ram,sizeof ram,&e));rom[at]^=1;
 }
 LagoonJournal_Init(NULL,sizeof rom);assert(!LagoonJournal_Get(ram,sizeof ram,&e));
 LagoonJournal_Init(rom,sizeof rom-1);assert(!LagoonJournal_Get(ram,sizeof ram,&e));
 LagoonJournal_Init(rom,sizeof rom);check_readonly(&e);
 puts("PASS journal: all128 boss masks, exact4tablet stages/completion, earlier-save rollback, invalid-source/size/room refusals; no guest writes or invented history");
 return 0;
}
