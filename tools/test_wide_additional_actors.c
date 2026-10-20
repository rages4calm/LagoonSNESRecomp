/* Additional-room ordinary presentation selectors against actual ROM opcodes.
 * No extracted asset/code bytes are embedded; provide the verified owner ROM. */
#include "lagoon_wide_entities.h"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t rom[0x100000],ram[0x20000];
static unsigned product,writes,cases,postcases,refusals,source_mutations;
static int town_only,field_only;
static unsigned word(const uint8_t *p,unsigned a){return p[a]|p[a+1]<<8;}
static uint8_t read_byte(void *m,uint32_t address) {
  (void)m;unsigned bank=address>>16,a=address&65535;
  if(bank==0x7e || bank==0x7f)return ram[(bank-0x7e)*65536+a];
  if((bank&127)<64 && a<8192)return ram[a];
  if((bank&127)<64 && (a==0x4216 || a==0x4217))return (uint8_t)(product>>((a-0x4216)*8));
  assert((bank&127)<32 && a>=32768);return rom[(bank&31)*32768+(a&32767)];
}
static void write_byte(void *m,uint32_t address,uint8_t value) {
  (void)m;unsigned bank=address>>16,a=address&65535;
  if((bank&127)<64 && a>=0x4202 && a<=0x4203) {
    ram[a]=value;product=ram[0x4202]*ram[0x4203];return;
  }
  assert((bank&127)<64 && a<8192);
  /* Full selector must write only its original scratch/timer/phase/frame. */
  assert(a==0x40 || a==0x41 || a==0x1024 || a==0x1025 || a==0x26 || a==0x27 ||
      a==0x550 || a==0x551 || a==0x55c || a==0x1d0 || a==0x1cf);
  ram[a]=value;++writes;
}
static unsigned table_delay(unsigned type,unsigned kind) {
  unsigned table=type==1?word(rom,0x8000+(0xf885+kind*2-0x8000)):0xf41a+kind*17;
  if(table<0x8000)return 0;
  return rom[0x8000+(table&32767)];
}
static void selectors(void) {
  static const unsigned enemies[][2]={{0x2c,31},{0x2c,32},{0x2c,33},{0x2f,25},{0x2f,27},{0x35,28}};
  static const unsigned towns[][2]={{0x19,69},{0x19,70},{0x19,71},{0x19,72},{0x19,73},
    {0x19,74},{0x19,75},{0x19,76},{0x19,77},{0x19,78},{0x1a,30},{0x1a,31},
    {0x1a,79},{0x1a,91},{0x1b,18},{0x1c,19},{0x1c,20},{0x1c,21},{0x1c,22},
    {0x1c,23},{0x1c,24},{0x1c,25},{0x1c,26}};
  static const unsigned fields[][2]={{0x18,15},{0x18,16},{0x1e,17},{0x1e,18},{0x20,21},{0x20,22}};
  Interp816 *cpu=interp816_init(NULL,read_byte,write_byte);assert(cpu);
  for(unsigned type=field_only?2:1;type<=(town_only?1:2);++type)
  for(unsigned n=0;n<(town_only?23:field_only?6:type==1?62:6);++n) {
    unsigned kind=town_only?towns[n][1]:field_only?fields[n][1]:type==1?n+2:enemies[n][1];
    unsigned room=town_only?towns[n][0]:field_only?fields[n][0]:type==1?0x34:enemies[n][0];
    unsigned delay=table_delay(type,kind);if(!delay)continue;
    for(unsigned dir=0;dir<4;++dir)for(unsigned phase=0;phase<4;++phase)
    for(unsigned tick=0;tick<4;++tick)for(unsigned idle=0;idle<4;++idle)
    for(unsigned variant=0;variant<(type==1?1:4);++variant) {
      memset(ram,0,sizeof(ram));uint8_t *r=ram+0x540;
      r[0]=idle==0?0xe8:idle==1?0xc9:idle==2?0xc8:0xe9;
      r[11]=idle==2?0x80:0;r[10]=(uint8_t)dir;r[0x18]=(uint8_t)type;
      r[0x1b]=(uint8_t)kind;r[0x10]=(uint8_t)(tick==0?0:tick==1?delay-1:tick==2?delay:255);
      r[0x11]=(uint8_t)phase;r[0x1c]=0xad;r[0x2c]=(uint8_t)variant;
      uint8_t before[64];memcpy(before,r,64);LagoonWideAnimation animation={r[16],r[17]};uint8_t frame;
      assert(LagoonWideEntities_FrameForRoom(rom,sizeof(rom),room,before,&animation,1,&frame));
      assert(!memcmp(before,r,64));
      cpu->pc=type==1?0xac4f:0xab31;cpu->k=1;cpu->db=1;cpu->dp=0;
      cpu->sp=0x1d0;cpu->x=0x40;cpu->y=0x1234;cpu->a=0;
      cpu->mf=1;cpu->xf=0;cpu->e=cpu->d=cpu->c=cpu->z=cpu->n=cpu->v=0;cpu->i=1;
      ram[0x1d1]=0xef;ram[0x1d2]=0x7f;writes=0;unsigned instructions=0;
      while(cpu->pc!=0x7ff0) {assert(++instructions<100);interp816_runOpcode(cpu);}
      assert(cpu->sp==0x1d2 && writes>0);
      if(r[16]!=animation.timer || r[17]!=animation.phase || r[28]!=frame) {
        fprintf(stderr,"selector type%u kind%u dir%u phase%u tick%u idle%u variant%u: actual%u/%u/%u expected%u/%u/%u\n",
          type,kind,dir,phase,tick,idle,variant,r[16],r[17],r[28],animation.timer,animation.phase,frame);
        assert(0);
      }
      before[16]=r[16];before[17]=r[17];before[28]=r[28];assert(!memcmp(before,r,64));
      ++cases;
      /* Native OAM witness validates the selector's real post-update state. */
      animation=(LagoonWideAnimation){r[16],r[17]};
      assert(LagoonWideEntities_FrameForRoom(rom,sizeof(rom),room,r,&animation,0,&frame));
      assert(frame==r[28] && animation.timer==r[16] && animation.phase==r[17]);++postcases;
    }
  }
  interp816_free(cpu);
}
static void negatives(void) {
  uint8_t r[64]={0};r[0]=0xe8;r[24]=2;r[27]=31;LagoonWideAnimation a={0};uint8_t frame;
  for(unsigned room=0;room<256;++room)if(room!=0x2c) {
    assert(!LagoonWideEntities_FrameForRoom(rom,sizeof(rom),room,r,&a,0,&frame));++refusals;
  }
  for(unsigned status=1;status<8;++status) {
    r[1]=(uint8_t)status;assert(!LagoonWideEntities_FrameForRoom(rom,sizeof(rom),0x2c,r,&a,0,&frame));++refusals;
  }
  r[1]=4;r[27]=29;assert(!LagoonWideEntities_FrameForRoom(rom,sizeof(rom),0x35,r,&a,0,&frame));++refusals;
  r[1]=0;assert(!LagoonWideEntities_FrameForRoom(rom,sizeof(rom),0x35,r,&a,0,&frame));++refusals;
  r[27]=31;r[10]=4;assert(!LagoonWideEntities_FrameForRoom(rom,sizeof(rom),0x2c,r,&a,0,&frame));++refusals;
  r[10]=0;a.phase=4;assert(!LagoonWideEntities_FrameForRoom(rom,sizeof(rom),0x2c,r,&a,0,&frame));++refusals;
  assert(!LagoonWideEntities_AnimationSource(rom,sizeof(rom)-1,1));++refusals;
  assert(!LagoonWideEntities_AnimationSource(NULL,sizeof(rom),1));++refusals;
  for(unsigned type=0;type<5;++type)if(type!=1 && type!=2) {
    assert(!LagoonWideEntities_AnimationSource(rom,sizeof(rom),type));++refusals;
  }
  const unsigned regions[][3]={{0xac4f,91,1},{0xab31,45,2},{0xab5e,131,2}};
  for(unsigned n=0;n<3;++n)for(unsigned k=0;k<regions[n][1];++k) {
    unsigned off=0x8000+((regions[n][0]+k)&32767);rom[off]^=1;
    assert(!LagoonWideEntities_AnimationSource(rom,sizeof(rom),regions[n][2]));
    rom[off]^=1;++source_mutations;
  }
  if(field_only) {
    const unsigned rooms[]={0x18,0x1d,0x1e,0x1f,0x20,0x21};
    r[10]=0;r[1]=0;a.phase=0;
    for(unsigned n=0;n<6;++n)for(unsigned type=1;type<=5;++type)for(unsigned kind=0;kind<256;++kind) {
      r[24]=(uint8_t)type;r[27]=(uint8_t)kind;
      int allowed=type==2 && ((rooms[n]==0x18 && (kind==15||kind==16)) ||
        (rooms[n]==0x1e && (kind==17||kind==18)) || (rooms[n]==0x20 && (kind==21||kind==22)));
      assert(!!LagoonWideEntities_FrameForRoom(rom,sizeof(rom),rooms[n],r,&a,0,&frame)==allowed);
      if(!allowed)++refusals;
    }
  }
  if(town_only) {
    r[24]=1;r[10]=0;r[1]=0;a.phase=0;
    for(unsigned room=0x19;room<=0x1c;++room)for(unsigned kind=0;kind<256;++kind) {
      int allowed=(room==0x19 && kind>=69 && kind<=78) ||
        (room==0x1a && (kind==30 || kind==31 || kind==79 || kind==91)) ||
        (room==0x1b && kind==18) || (room==0x1c && kind>=19 && kind<=26);
      r[27]=(uint8_t)kind;
      assert(!!LagoonWideEntities_FrameForRoom(rom,sizeof(rom),room,r,&a,0,&frame)==allowed);
      if(!allowed)++refusals;
    }
  }
}
int main(int argc,char **argv) {
  assert(argc==2 || (argc==3 && (!strcmp(argv[2],"--town-only") || !strcmp(argv[2],"--field-only"))));
  town_only=argc==3 && !strcmp(argv[2],"--town-only");field_only=argc==3 && !strcmp(argv[2],"--field-only");
  FILE *f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);
  assert(LagoonWideEntities_AnimationSource(rom,sizeof(rom),1));
  assert(LagoonWideEntities_AnimationSource(rom,sizeof(rom),2));
  selectors();negatives();
  printf("additional actor source: %u actual original full NPC/enemy selector calls, %u real post-state frame checks, %u refusals, %u every-source-byte mutations PASS\n",cases,postcases,refusals,source_mutations);
  return 0;
}
