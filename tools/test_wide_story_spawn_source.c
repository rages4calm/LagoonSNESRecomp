/* Own verified original ROM required; no copied assets are embedded.
 * Source-authored room entry scripts are traversed with the original fixed
 * command operand spans. Every discovered opcode15 executes the actual full
 * D20C NPC initializer through its real RTS. Branches establish source
 * feasibility only, not storyflag correlation or live quest completion. */
#include "lagoon_wide_entities.h"
#include "snes/interp816.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint8_t rom[0x100000],ram[0x20000];
static unsigned word(const uint8_t *p,unsigned a){return p[a]|p[a+1]<<8;}
static void put(unsigned a,unsigned v){ram[a]=(uint8_t)v;ram[a+1]=(uint8_t)(v>>8);}
static unsigned offset(unsigned a){return (a>>16)*32768+(a&32767);}
static uint8_t rd(void *ctx,uint32_t a) {
  (void)ctx;unsigned b=a>>16,p=a&65535;
  if(b==126||b==127)return ram[(b-126)*65536+p];
  if((b&127)<64&&p<8192)return ram[p];
  if((b&127)<64&&(p==0x4214||p==0x4215)) {
    unsigned q=ram[0x4206]?word(ram,0x4204)/ram[0x4206]:65535;
    return (uint8_t)(q>>((p-0x4214)*8));
  }
  assert(b<32&&p>=32768);return rom[offset(a)];
}
static void wr(void *ctx,uint32_t a,uint8_t v) {
  (void)ctx;unsigned b=a>>16,p=a&65535;
  if(b==126||b==127){ram[(b-126)*65536+p]=v;return;}
  if((b&127)<64&&p>=0x4204&&p<=0x4206){ram[p]=v;return;}
  assert((b&127)<64&&p<8192);ram[p]=v;
}
static void run(Interp816 *c,unsigned pc,unsigned entity) {
  c->k=c->db=1;c->pc=pc;c->dp=0;c->sp=0x1d0;c->x=entity;c->y=0;c->a=0;
  c->mf=1;c->xf=0;c->e=c->d=c->c=c->v=c->n=c->z=0;c->i=1;
  put(c->sp+1,0x7fef);unsigned count=0;
  while(c->k!=1||c->pc!=0x7ff0){assert(++count<10000);interp816_runOpcode(c);}
  assert(c->sp==0x1d2);
}
static const unsigned args[]={1,1,3,1,1,1,1,1,1,1,1,1,1,2,1,7,0,3,3,3,0,8,2,0,1,1,1,1,0,0,0,0,0,1,3,0,0,0,0,0};
static int reachable(unsigned room,unsigned target) {
  static uint8_t seen[65536];static unsigned todo[65536];memset(seen,0,sizeof seen);
  unsigned read=0,write=0,start=word(rom,offset(0x05cb37)+room*2);todo[write++]=start;
  while(read<write) {
    unsigned pc=todo[read++];assert(pc>=0x8000&&pc<65536);
    if(seen[pc])continue;seen[pc]=1;if(pc==target)return 1;
    unsigned at=offset(0x050000|pc),op=rom[at];if(op==255)continue;
    assert(op<sizeof args/sizeof *args);unsigned next=pc+1+args[op];
    if(op==0x11||op==0x12||op==0x13) {
      assert(rom[at+3]==5);assert(write<65536);todo[write++]=word(rom,at+1);
      if(op==0x11)continue;
    }
    if(op==0x0f&&rom[at+5]!=room)continue;
    if(op==0x10&&room!=2)continue;
    assert(write<65536);todo[write++]=next;
  }
  return 0;
}
int main(int argc,char **argv) {
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 const unsigned commands[][2]={
  {0x17,0x05ccd0},
  {0x21,0x05cd43},
  {0x29,0x05cd75},
  {0x4a,0x05ce4a},
  {0x4a,0x05ce40},
  {0x17,0x05ccd9},
  {0x21,0x05cd4c},
  {0x00,0x05cbe9},
  {0x02,0x05cc1c},
  {0x07,0x05cc4d},
  {0x29,0x05cd7f},
  {0x29,0x05cd62},
  {0x02,0x05cc25},
  {0x09,0x05cc65},
  {0x29,0x05cd88},
  {0x29,0x05cd6b},
  {0x11,0x05ccbd},
 };
 Interp816*c=interp816_init(NULL,rd,wr);assert(c);unsigned tested=0,cells_checked=0;
 for(unsigned n=0;n<sizeof commands/sizeof *commands;n++) {
  unsigned room=commands[n][0],pc=commands[n][1],at=offset(pc);assert(rom[at]==0x15&&reachable(room,pc&65535));
  memset(ram,0,sizeof ram);memcpy(ram+0x50,rom+offset(0x05b2df+room*56),56);put(0x48e,room);
  put(0xa1,pc+1);ram[0xa3]=5;run(c,0xd20c,0);
  assert(c->y==8&&ram[0x558]==1&&word(ram,0x542)==word(rom,at+1)&&word(ram,0x544)==word(rom,at+3));
  assert(ram[0x55b]==rom[at+5]&&ram[0x54a]==rom[at+6]&&ram[0x54b]==255&&ram[0x55a]==rom[at+7]);
  assert(ram[0x540]==rom[offset(0x01d27a)+rom[at+8]]);
  printf("SPAWN room%02X command%06X NPC%u XY%u/%u actualflags%02X\n",room,pc,ram[0x55b],word(ram,0x542),word(ram,0x544),ram[0x540]);
  /* Actual produced actor, with an explicitly synthetic native-visible camera
   * position and queue bit. No follower flag is supplied for these story roles. */
  ram[0x540]|=8;put(0x54c,100);put(0x54e,100);
  LagoonWideAnimation animation={ram[0x550],ram[0x551]};uint8_t frame=255;
  assert(LagoonWideEntities_FrameForContext(rom,sizeof rom,ram,ram+0x540,&animation,1,&frame));
  run(c,0xac4f,0x40);assert(ram[0x55c]==frame);
  LagoonWideCell cells[64];int count=LagoonWideEntities_RecordCells(rom,sizeof rom,ram,0x40,frame,0,cells,64);
  assert(count>0);ram[0x236]=0;memset(ram+0x2000,0,544);ram[0x2a2]=255;run(c,0xc1c6,0x40);
  assert(count==(int)ram[0x236]);
  for(int k=0;k<count;k++) {
    unsigned a=0x2000+k*4,h=(ram[0x2200+k/4]>>((k%4)*2))&3;
    assert((unsigned)(cells[k].x&511)==(ram[a]|(h&1)<<8));
    assert((unsigned)(cells[k].y&255)==ram[a+1]);
    assert(cells[k].tile==word(ram,a+2)&&cells[k].size==((h&2)?16:8));cells_checked++;
  }
  tested++;
 }
 interp816_free(c);printf("PASS %u source-roomentry reachable opcode15 commands/full originalD20C initializers and actualAC4F/C1C6/8B71/8211 chains/%u independentcells versusproduction; realframes/all8payloadbytes/createdrecord/sourcebranch graph; syntheticcamerafixture/no livequest/flagcorrelation claim\n",tested,cells_checked);return 0;
}
