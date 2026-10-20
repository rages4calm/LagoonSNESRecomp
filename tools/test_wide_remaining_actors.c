/* Original full NPC draw C1C6 -> real generic8B71 -> real8211 OAM writer.
 * An independent opcode interpreter, not the host parser, creates every slot. */
#include "lagoon_wide_entities.h"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t rom[0x100000],ram[0x20000];
static unsigned product;
static unsigned word(const uint8_t *p,unsigned a){return p[a]|p[a+1]<<8;}
static uint8_t read_byte(void *m,uint32_t address) {
  (void)m;unsigned bank=address>>16,a=address&65535;
  if(bank==0x7e || bank==0x7f)return ram[(bank-0x7e)*65536+a];
  if((bank&127)<64 && a<8192)return ram[a];
  if((bank&127)<64 && (a==0x4216 || a==0x4217))return (uint8_t)(product>>((a-0x4216)*8));
  if((bank&127)<64 && (a==0x4214 || a==0x4215)) {
    unsigned quotient=ram[0x4206]?word(ram,0x4204)/ram[0x4206]:65535;
    return (uint8_t)(quotient>>((a-0x4214)*8));
  }
  assert((bank&127)<32 && a>=32768);return rom[(bank&31)*32768+(a&32767)];
}
static void write_byte(void *m,uint32_t address,uint8_t value) {
  (void)m;unsigned bank=address>>16,a=address&65535;
  if(bank==0x7e || bank==0x7f){ram[(bank-0x7e)*65536+a]=value;return;}
  if((bank&127)<64 && (a==0x4202 || a==0x4203)){ram[a]=value;product=ram[0x4202]*ram[0x4203];return;}
  if((bank&127)<64 && a>=0x4204 && a<=0x4206){ram[a]=value;return;}
  assert((bank&127)<64 && a<8192);ram[a]=value;
}

static void put(unsigned a,unsigned v){ram[a]=v;ram[a+1]=v>>8;}
static void run(Interp816 *cpu,unsigned entry) {
  cpu->pc=entry;cpu->k=cpu->db=1;cpu->dp=0;cpu->sp=0x1d0;cpu->x=0x40;cpu->y=0x1234;cpu->a=0;
  cpu->mf=1;cpu->xf=0;cpu->e=cpu->d=cpu->c=cpu->n=cpu->v=cpu->z=0;cpu->i=1;
  ram[0x1d1]=0xef;ram[0x1d2]=0x7f;unsigned instructions=0;
  while(cpu->k!=1||cpu->pc!=0x7ff0){if(++instructions>=100000){fprintf(stderr,"run entry%04x room%02x type%u kind%u flags%02x status%02x PC%02x:%04x SP%04x\n",entry,ram[0x48e],ram[0x558],ram[0x55b],ram[0x540],ram[0x541],cpu->k,cpu->pc,cpu->sp);fprintf(stderr,"frame%u ptr%02x%02x%02x dims%u,%u selector%u OAM%u a%04x mf%d\n",ram[0x55c],ram[0xe],ram[0xd],ram[0xc],ram[0x284],ram[0x285],ram[0x29e],ram[0x236],cpu->a,cpu->mf);assert(0);}interp816_runOpcode(cpu);}
  assert(cpu->sp==0x1d2);
}
/* Newly source-qualified and previously unsupported scripted/prop contracts.
 * IDs only, no ROM pixels, code bytes, or extracted authored records. */
static const unsigned actors[][3]={
  {0x00,1,39},
  {0x00,1,40},
  {0x00,1,41},
  {0x00,1,42},
  {0x00,1,43},
  {0x00,1,44},
  {0x00,1,45},
  {0x00,1,46},
  {0x00,1,55},
  {0x00,1,59},
  {0x00,1,60},
  {0x00,5,0},
  {0x00,5,1},
  {0x01,1,10},
  {0x01,1,11},
  {0x02,1,11},
  {0x02,1,12},
  {0x02,1,13},
  {0x02,1,14},
  {0x02,1,15},
  {0x02,5,1},
  {0x03,2,0},
  {0x03,2,1},
  {0x03,5,2},
  {0x04,2,0},
  {0x05,2,0},
  {0x05,2,2},
  {0x08,2,3},
  {0x08,2,4},
  {0x09,1,0},
  {0x09,1,16},
  {0x09,1,47},
  {0x09,1,48},
  {0x09,1,49},
  {0x09,1,50},
  {0x09,1,51},
  {0x09,1,52},
  {0x09,1,53},
  {0x09,1,54},
  {0x09,1,56},
  {0x09,1,57},
  {0x09,1,58},
  {0x0a,5,8},
  {0x0a,5,10},
  {0x0b,2,5},
  {0x0b,2,6},
  {0x0b,5,3},
  {0x0c,2,9},
  {0x0c,2,10},
  {0x0c,5,1},
  {0x0d,1,27},
  {0x0d,1,28},
  {0x0d,1,29},
  {0x0d,2,7},
  {0x0d,2,8},
  {0x0d,5,4},
  {0x10,2,11},
  {0x10,2,12},
  {0x11,1,0},
  {0x11,1,17},
  {0x11,1,61},
  {0x11,1,62},
  {0x11,1,63},
  {0x11,1,64},
  {0x11,1,65},
  {0x11,1,66},
  {0x11,1,67},
  {0x11,1,68},
  {0x11,5,0},
  {0x11,5,1},
  {0x14,2,13},
  {0x14,2,14},
  {0x15,2,13},
  {0x15,2,14},
  {0x17,1,35},
  {0x17,1,38},
  {0x17,5,1},
  {0x19,5,0},
  {0x19,5,1},
  {0x1b,5,1},
  {0x1e,5,6},
  {0x1e,5,7},
  {0x21,1,0},
  {0x21,1,33},
  {0x26,2,23},
  {0x26,2,26},
  {0x26,5,1},
  {0x28,2,24},
  {0x29,1,7},
  {0x29,1,9},
  {0x38,2,34},
  {0x39,2,34},
  {0x3a,5,1},
  {0x3d,1,34},
  {0x3e,2,30},
  {0x3e,2,35},
  {0x3e,5,1},
  {0x3f,5,9},
  {0x40,5,9},
  {0x42,5,11},
  {0x44,2,36},
  {0x44,5,12},
  {0x45,5,13},
  {0x46,1,5},
  {0x46,2,37},
  {0x47,1,2},
  {0x47,1,4},
  {0x47,1,6},
  {0x49,1,36},
  {0x49,1,37},
  {0x4a,1,35},
};
int main(int argc,char **argv){
  assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);
  Interp816 *cpu=interp816_init(NULL,read_byte,write_byte);assert(cpu);
  unsigned selectors=0,poststates=0,oam=0,cells_count=0,invalid=0,held=0,mutants=0;
  for(unsigned n=0;n<sizeof(actors)/sizeof(actors[0]);++n){
    unsigned room=actors[n][0],type=actors[n][1],kind=actors[n][2];
    assert(LagoonWideEntities_AllowedKind(room,type,kind));
    for(unsigned direction=0;direction<4;++direction)for(unsigned phase=0;phase<(type==2&&kind==12?7:4);++phase)
    for(unsigned variant=0;variant<(type==2?4:1);++variant)for(unsigned idle=0;idle<3;++idle)
    for(unsigned tick=0;tick<3;++tick)for(unsigned subtype=0;subtype<(kind==37?6:1);++subtype){
      memset(ram,0,sizeof(ram));put(0x48e,room);uint8_t*r=ram+0x540;
      r[0]=idle==0?0xe8:0xc9;r[10]=direction;r[24]=type;r[27]=kind;r[44]=variant;
      r[16]=tick==0?0:tick==1?1:255;r[17]=phase;r[28]=0xad;ram[0x408]=tick==0?0:tick==1?4:12;ram[0x8040]=idle==2;r[0x21]=subtype;
      LagoonWideAnimation a={r[16],r[17]};uint8_t frame;uint8_t before[64];memcpy(before,r,64);
      int ordinary=LagoonWideEntities_FrameForContext(rom,sizeof(rom),ram,r,&a,1,&frame);
      if(ordinary){
        run(cpu,type==1?0xac4f:0xab31);
        if(r[16]!=a.timer||r[17]!=a.phase||r[28]!=frame){fprintf(stderr,"selector room%02x type%u kind%u d%u p%u v%u idle%u tick%u actual%u/%u/%u host%u/%u/%u\n",room,type,kind,direction,phase,variant,idle,tick,r[16],r[17],r[28],a.timer,a.phase,frame);assert(0);}
        before[16]=r[16];before[17]=r[17];before[28]=r[28];assert(!memcmp(before,r,64));++selectors;
        a=(LagoonWideAnimation){r[16],r[17]};assert(LagoonWideEntities_FrameForContext(rom,sizeof(rom),ram,r,&a,0,&frame));assert(frame==r[28]&&a.timer==r[16]&&a.phase==r[17]);++poststates;
      }else{if(type==5)frame=0x3e;else{++invalid;continue;}}
      /* Keep original ABI fixture explicit: M1/X0, DB1, DP0, A-high0. */
      r[28]=frame;put(0x54c,100);put(0x54e,100);put(0x542,228);put(0x544,228);put(0x480,128);put(0x482,128);
      r[0x15]=phase*3;put(0x90,(unsigned)(int16_t)(tick==0?0:tick==1?-3:7));
      if(type==2&&(kind==13||kind==14))r[1]=4;
      if(type==2&&tick==1)r[1]|=0x80;if(type==2&&tick==2)r[0x1f]|=2;
      LagoonWideCell cells[64];int count=LagoonWideEntities_RecordCells(rom,sizeof(rom),ram,0x40,frame,0,cells,64);
      if(count<0){++invalid;continue;}
      LagoonWideCell projected[64];assert(LagoonWideEntities_RecordCells(rom,sizeof(rom),ram,0x40,frame,1,projected,64)==count);assert(!memcmp(cells,projected,count*sizeof(*cells)));
      ram[0x2a2]=0xff;run(cpu,type==1?0xc1c6:type==2?0xc214:0xc44e);
      if(count!=(int)ram[0x236]){fprintf(stderr,"OAM count room%02x type%u kind%u frame%u host%d guest%u\n",room,type,kind,frame,count,ram[0x236]);for(int z=0;z<count;++z)fprintf(stderr,"cell%d %d,%d size%d\n",z,cells[z].x,cells[z].y,cells[z].size);assert(0);}
      for(int i=0;i<count;++i){unsigned at=0x2000+i*4,hi=(ram[0x2200+i/4]>>((i%4)*2))&3;
        if((unsigned)(cells[i].x&511)!=(ram[at]|(hi&1)<<8)||(unsigned)(cells[i].y&255)!=ram[at+1]||cells[i].tile!=word(ram,at+2)||cells[i].size!=((hi&2)?16:8)){fprintf(stderr,"OAM room%02x type%u kind%u frame%u cell%d host%d,%d,%04x,%u guest%u,%u,%04x,%u\n",room,type,kind,frame,i,cells[i].x,cells[i].y,cells[i].tile,cells[i].size,ram[at]|(hi&1)<<8,ram[at+1],word(ram,at+2),(hi&2)?16:8);assert(0);}++cells_count;}
      ++oam;if(type==5)++held;
    }
  }

  /* Status4 is never simulated. Independently execute the real draw for
   * every bounded current-frame value that the host parser can decode. */
  const unsigned current_poses[][2]={{0x14,13},{0x15,14},{0x35,29}};
  for(unsigned n=0;n<3;++n)for(unsigned frame=0;frame<255;++frame) {
    memset(ram,0,sizeof(ram));put(0x48e,current_poses[n][0]);uint8_t*r=ram+0x540;
    r[0]=0xc8;r[1]=4;r[24]=2;r[27]=current_poses[n][1];r[28]=frame;put(0x54c,100);put(0x54e,100);
    LagoonWideAnimation a={0};uint8_t predicted;assert(!LagoonWideEntities_FrameForContext(rom,sizeof(rom),ram,r,&a,1,&predicted));
    LagoonWideCell cells[64];int count=LagoonWideEntities_RecordCells(rom,sizeof(rom),ram,0x40,frame,0,cells,64);
    if(count<0){++invalid;continue;}
    int visible=0;for(int i=0;i<count;++i)if(cells[i].y<240&&cells[i].y>=-(int)cells[i].size)cells[visible++]=cells[i];count=visible;
    ram[0x2a2]=0xff;run(cpu,0xc214);if(count!=(int)ram[0x236]){fprintf(stderr,"current pose kind%u frame%u count%d actual%u\n",r[27],frame,count,ram[0x236]);for(int z=0;z<count;++z)fprintf(stderr,"cell%d %d,%d size%d\n",z,cells[z].x,cells[z].y,cells[z].size);assert(0);}
    for(int i=0;i<count;++i){unsigned at=0x2000+i*4,hi=(ram[0x2200+i/4]>>((i%4)*2))&3;
      assert((unsigned)(cells[i].x&511)==(ram[at]|(hi&1)<<8));assert((unsigned)(cells[i].y&255)==ram[at+1]);
      assert(cells[i].tile==word(ram,at+2)&&cells[i].size==((hi&2)?16:8));++cells_count;
    }++oam;
  }
  /* Non-ordinary/death status never borrows the predictive frame service. */
  for(unsigned status=1;status<8;++status){memset(ram,0,sizeof(ram));put(0x48e,0x14);uint8_t*r=ram+0x540;r[24]=2;r[27]=13;r[1]=status;r[0]=0xe8;LagoonWideAnimation a={0};uint8_t frame;assert(!LagoonWideEntities_FrameForContext(rom,sizeof(rom),ram,r,&a,1,&frame));++mutants;}
  const unsigned regions[][3]={{0x1abe1,59,12},{0x1ac1c,20,26},{0x1ac30,31,37},{0x1c1c6,78,1},{0x1c214,112,2},{0x1c44e,97,5}};
  for(unsigned j=0;j<6;++j)for(unsigned i=0;i<regions[j][1];++i){
    memset(ram,0,sizeof(ram));uint8_t*r=ram+0x540;unsigned kind=regions[j][2];put(0x48e,kind==12?0x10:kind==26?0x26:kind==37?0x46:kind==1?1:kind==2?3:0x0a);r[24]=kind<=5?kind==5?5:kind==1?1:2:2;r[27]=kind<=5?kind==1?10:kind==2?0:8:kind;r[0]=(kind==26||kind==37)?0xc9:0xe8;r[28]=0x3e;
    unsigned at=((regions[j][0]>>16)&127)*32768+(regions[j][0]&32767)+i;rom[at]^=1;LagoonWideAnimation a={0};uint8_t frame;LagoonWideCell cells[64];
    if(j<3)assert(!LagoonWideEntities_FrameForContext(rom,sizeof(rom),ram,r,&a,1,&frame));else assert(LagoonWideEntities_RecordCells(rom,sizeof(rom),ram,0x40,0x3e,0,cells,64)<0);rom[at]^=1;++mutants;
  }
  interp816_free(cpu);printf("remaining actors: %u actual selectors/%u poststates; %u full original C1C6/C214/C44E OAM calls/%u exact cells (%u prop calls), %u invalid parser/source fixtures refused; %u status/every-new-source-byte refusals PASS\n",selectors,poststates,oam,cells_count,held,invalid,mutants);return 0;
}
