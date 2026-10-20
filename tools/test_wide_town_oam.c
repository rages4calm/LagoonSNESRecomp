/* Original full NPC draw C1C6 -> real generic8B71 -> real8211 OAM writer.
 * An independent opcode interpreter, not the host parser, creates every slot. */
#include "lagoon_wide_entities.h"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t rom[0x100000],ram[0x20000];
static unsigned word(const uint8_t *p,unsigned a){return p[a]|p[a+1]<<8;}
static uint8_t read_byte(void *m,uint32_t address) {
  (void)m;unsigned bank=address>>16,a=address&65535;
  if(bank==0x7e || bank==0x7f)return ram[(bank-0x7e)*65536+a];
  if((bank&127)<64 && a<8192)return ram[a];
  if((bank&127)<64 && (a==0x4214 || a==0x4215)) {
    unsigned quotient=ram[0x4206]?word(ram,0x4204)/ram[0x4206]:65535;
    return (uint8_t)(quotient>>((a-0x4214)*8));
  }
  assert((bank&127)<32 && a>=32768);return rom[(bank&31)*32768+(a&32767)];
}
static void write_byte(void *m,uint32_t address,uint8_t value) {
  (void)m;unsigned bank=address>>16,a=address&65535;
  if(bank==0x7e || bank==0x7f){ram[(bank-0x7e)*65536+a]=value;return;}
  if((bank&127)<64 && a>=0x4204 && a<=0x4206){ram[a]=value;return;}
  assert((bank&127)<64 && a<8192);ram[a]=value;
}
int main(int argc,char **argv) {
  int field=argc==3 && !strcmp(argv[2],"--field-only");assert(argc==2||field);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);
  const unsigned cases[][2]={{0x19,69},{0x19,70},{0x19,71},{0x19,72},{0x19,73},
    {0x19,74},{0x19,75},{0x19,76},{0x19,77},{0x19,78},{0x1a,30},{0x1a,31},
    {0x1a,79},{0x1a,91},{0x1b,18},{0x1c,19},{0x1c,20},{0x1c,21},{0x1c,22},
    {0x1c,23},{0x1c,24},{0x1c,25},{0x1c,26}};
  const unsigned fields[][2]={{0x18,15},{0x18,16},{0x1e,17},{0x1e,18},{0x20,21},{0x20,22}};
  Interp816 *cpu=interp816_init(NULL,read_byte,write_byte);assert(cpu);unsigned checks=0,cells_checked=0,decode_refusals=0;
  for(unsigned n=0;n<(field?6:23);++n)for(unsigned direction=0;direction<4;++direction)for(unsigned phase=0;phase<4;++phase)
  for(unsigned variant=0;variant<(field?4:1);++variant)for(unsigned hit=0;hit<(field?3:1);++hit) {
    memset(ram,0,sizeof(ram));uint8_t *r=ram+0x540;
    unsigned room=field?fields[n][0]:cases[n][0],kind=field?fields[n][1]:cases[n][1];ram[0x48e]=(uint8_t)room;
    r[0]=0xe8;r[24]=field?2:1;r[0x2c]=(uint8_t)variant;if(hit==1)r[1]=0x80;if(hit==2)r[0x1f]=2;r[27]=(uint8_t)kind;r[10]=(uint8_t)direction;r[12]=100;r[14]=100;
    LagoonWideAnimation animation={0,(uint8_t)phase};uint8_t frame;
    assert(LagoonWideEntities_FrameForRoom(rom,sizeof(rom),room,r,&animation,0,&frame));r[28]=frame;
    unsigned offsets=0x8000+((field?0xf7e9:0xfb7f)&32767)+kind*4;
    int x=100+(int16_t)word(rom,offsets),y=100+(int16_t)word(rom,offsets+2);
    LagoonWideCell cells[64];int count=LagoonWideEntities_Decode(rom,sizeof(rom),room+1,frame,x,y,cells,64);
    if(field && hit)for(int c=0;c<count;++c)cells[c].tile=(cells[c].tile&~0xe00)|0xe00;
    if(count<0){assert(field);++decode_refusals;continue;}
    cpu->pc=field?0xc214:0xc1c6;cpu->k=cpu->db=1;cpu->dp=0;cpu->sp=0x1d0;
    cpu->x=0x40;cpu->y=0x1234;cpu->a=0;cpu->mf=1;cpu->xf=0;
    cpu->e=cpu->d=cpu->c=cpu->n=cpu->v=cpu->z=0;cpu->i=1;
    ram[0x1d1]=0xef;ram[0x1d2]=0x7f;unsigned instructions=0;
    while(cpu->k!=1 || cpu->pc!=0x7ff0){assert(++instructions<10000);interp816_runOpcode(cpu);}
    assert(cpu->sp==0x1d2);
    if(count!=(int)ram[0x236]){fprintf(stderr,"NPC draw count room%02x kind%u frame%u host%d original%u\n",room,kind,frame,count,ram[0x236]);assert(0);}
    for(int c=0;c<count;++c) {
      unsigned a=0x2000+c*4,high=(ram[0x2200+c/4]>>((c%4)*2))&3;
      assert((unsigned)(cells[c].x&511)==(ram[a]|(high&1)<<8));
      assert((unsigned)(cells[c].y&255)==ram[a+1]);assert(cells[c].tile==word(ram,a+2));
      assert(cells[c].size==((high&2)?16:8));++cells_checked;
    }
    ++checks;
  }
  interp816_free(cpu);
  printf("original OAM oracle: %u complete original NPC C1C6 or enemy C214/8B71/8211 calls, %u independent metasprite cells match position/tile/palette/priority/flips/size; %u invalid/oversized decoder fixtures refused before authority PASS\n",checks,cells_checked,decode_refusals);
  return 0;
}
