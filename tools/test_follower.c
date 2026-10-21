/* Synthetic map/route policy fixtures execute the real original facing,
 * velocity, tile collision and actor integration opcodes. They establish
 * bounded mechanics; live escort scripts are separate private route evidence. */
#include "../src/lagoon_follower.c"
#include "snes/interp816.h"
#include <assert.h>
static uint8_t rom[0x100000],ram[0x20000];
static uint64_t generation=1;
static int has_aot;
static unsigned hooks,opcodes,steps,refusals;
static int use_body_filter;
static CpuWramReadContext read_context;
static CpuState last_cpu;
uint64_t RtlStateGeneration(void){return generation;}
int cpu_dispatch_has_entry(CpuState*c,uint32_t p){(void)c;(void)p;return has_aot;}
bool interp_bridge_add_pre_opcode_hook(uint32_t p,InterpPreOpcodeHook h){(void)p;(void)h;return true;}
static unsigned off(unsigned p){return (p>>16)*32768+(p&32767);}
static void triple(unsigned p,unsigned v){put(ram,p,v);ram[p+2]=(uint8_t)(v>>16);}
static uint8_t rd(void*ctx,uint32_t a){
  (void)ctx;unsigned bank=a>>16,p=a&65535;
  if(bank==126||bank==127){unsigned o=(bank-126)*65536+p;return use_body_filter?LagoonFollower_FilterRead(o,ram[o],&read_context,ram):ram[o];}
  if((bank&127)<64&&p<8192)return use_body_filter?LagoonFollower_FilterRead(p,ram[p],&read_context,ram):ram[p];
  if((bank&127)<64&&p>=0x4214&&p<=0x4217){
    unsigned v=p<0x4216?(ram[0x4206]?word(ram,0x4204)/ram[0x4206]:65535):ram[0x4202]*ram[0x4203];
    return (uint8_t)(v>>((p&1)*8));
  }
  assert(bank<32&&p>=32768);return rom[off(a)];
}
static void wr(void*ctx,uint32_t a,uint8_t v){
  (void)ctx;unsigned bank=a>>16,p=a&65535;
  if(bank==126||bank==127){ram[(bank-126)*65536+p]=v;return;}
  if((bank&127)<64&&p>=0x4202&&p<=0x4206){ram[p]=v;return;}
  assert((bank&127)<64&&p<8192);ram[p]=v;
}
static void sync_cpu(Interp816*i,CpuState*c){
  memset(c,0,sizeof *c);c->ram=ram;c->A=i->a;c->X=i->x;c->Y=i->y;c->S=i->sp;
  c->D=i->dp;c->DB=i->db;c->PB=i->k;c->m_flag=i->mf;c->x_flag=i->xf;c->emulation=i->e;
  c->_flag_C=i->c;c->_flag_Z=i->z;c->_flag_N=i->n;c->_flag_V=i->v;c->_flag_I=i->i;c->_flag_D=i->d;
  c->P=(i->mf?32:0)|(i->xf?16:0)|(i->c?1:0)|(i->z?2:0)|(i->i?4:0)|(i->d?8:0)|(i->v?64:0)|(i->n?128:0);
}
static void run(unsigned entry,unsigned actor,unsigned stop,int use_hook){
  Interp816*i=interp816_init(NULL,rd,wr);assert(i);i->k=i->db=(uint8_t)(entry>>16);i->pc=(uint16_t)entry;
  i->dp=0;i->sp=0x1d0;i->x=actor;i->a=i->y=0;i->mf=1;i->xf=i->e=i->d=i->c=i->v=i->n=i->z=0;i->i=1;
  put(ram,i->sp+1,entry==0x018184?0x80bd:0x7fef);
  if((entry>>16)==2)ram[i->sp+3]=2;
  unsigned n=0;
  while((((unsigned)i->k<<16)|i->pc)!=stop){
    assert(++n<4000);unsigned pc=((unsigned)i->k<<16)|i->pc;
    read_context=(CpuWramReadContext){pc,i->sp,i->y,i->k,i->mf,i->xf,i->e,1};
    if(use_hook&&(pc==0x0181ac||pc==0x0181b6||pc==0x0181ba||pc==0x0180c7||pc==0x0180ca)){
      CpuState c,b;sync_cpu(i,&c);b=c;hook(&c,pc);assert(!memcmp(&b,&c,sizeof c));hooks++;
    }
    interp816_runOpcode(i);opcodes++;
  }
  sync_cpu(i,&last_cpu);
  if(stop==0x0180ca&&use_hook){CpuState before=last_cpu;hook(&last_cpu,stop);assert(!memcmp(&before,&last_cpu,sizeof before));}
  interp816_free(i);
}
static void reset(unsigned kind,unsigned room){
  memset(ram,0,sizeof ram);LagoonFollower_Reset(rom,sizeof rom);has_aot=0;
  ram[0x500]=0xc0;ram[0x51a]=2;ram[0x50b]=ram[0x519]=255;ram[0x51b]=0;
  put(ram,0x48e,room);put(ram,0x492,1000);put(ram,0x494,1000);
  put(ram,0x502,200);put(ram,0x504,200);ram[0x51]=1;
  put(ram,0x53,128);put(ram,0x55,128);
  ram[0x417]=1; /* Actual ordinary gameplay's animated mode. */
  triple(0x88,0x7f0000);triple(0x8b,0x7f4000);triple(0x78,0x7f8000);triple(0x6c,0x7f9000);
  ram[0x4e1]=kind?1:2;unsigned a=0xd00;
  ram[a]=0xc8;ram[a+24]=1;ram[a+27]=kind;ram[a+11]=ram[a+25]=255;
  put(ram,a+2,200);put(ram,a+4,200);
}
static void player(unsigned direction,unsigned pace){
  static const uint8_t intents[]={1,4,2,8};
  ram[0x53f]=direction<4?intents[direction]:0;ram[0x51a]=(uint8_t)(pace-1);
  run(0x0280a0,0,0x027ff0,0);run(0x0280ea,0,0x027ff0,0);
  run(0x019bce,0,0x017ff0,0);run(0x01a4d3,0,0x017ff0,0);
}
static void follower(void){run(0x018184,0x800,0x0180c7,1);steps++;}
static void ordinary(unsigned q,unsigned type){
  unsigned a=0x500+q;ram[a]=0xc8;ram[a+24]=(uint8_t)type;ram[a+27]=4;
  ram[a+11]=ram[a+25]=255;put(ram,a+2,800);put(ram,a+4,800);
  run(0x018184,q,0x0180c7,1);
}
/* Independent original-opcode oracle for the navigator's safe leading-edge
 * predicate. All supported nibble values/upper flags and all solid bytes run
 * through the real NPC collision dispatcher; unknown table slots never run. */
static unsigned navigator_probe_oracle(void){
  unsigned cases=0;
  for(unsigned attr=0;attr<256;attr++)for(unsigned d=0;d<4;d++)for(unsigned phase=0;phase<8;phase++){
    reset(1,7);assert(nav_source);
    memset(ram+0x18000,(int)attr,4);unsigned x=200+phase,y=200+(phase*3)%8;
    put(ram,0xd02,x);put(ram,0xd04,y);
    unsigned pass=!(attr&128)&&((attr&15)==0||(attr&15)==3||(attr&15)==5);
    uint8_t before[sizeof ram];memcpy(before,ram,sizeof ram);
    assert((unsigned)nav_step(ram,x,y,d,1)==pass);assert(!memcmp(before,ram,sizeof ram));
    if(!(attr&128)&&(attr&15)>5)continue;
    ram[0xd19]=(uint8_t)d;put(ram,0xd06,d==0?1:d==2?65535:0);put(ram,0xd08,d==1?1:d==3?65535:0);
    run(0x019bce,0x800,0x017ff0,0);
    assert(!!(word(ram,0xd06)||word(ram,0xd08))==pass);
    run(0x01a4d3,0x800,0x017ff0,0);cases++;
    if(pass){assert(word(ram,0xd02)==x+(d==0)-(d==2));assert(word(ram,0xd04)==y+(d==1)-(d==3));}
  }
  return cases;
}
static void solid(unsigned cx,unsigned cy){ram[0x14000+cy*128+cx]=1;memset(ram+0x18004,0x80,4);}
static unsigned walk_nav_route(unsigned tx,unsigned ty,unsigned pace){
  uint8_t before[sizeof ram];memcpy(before,ram,sizeof ram);
  assert(nav_route(ram,word(ram,0xd02),word(ram,0xd04),tx,ty));assert(!memcmp(before,ram,sizeof ram));
  unsigned ticks=0;
  for(unsigned n=0;n<nav.count;n++)while(word(ram,0xd02)!=nav.x[n]||word(ram,0xd04)!=nav.y[n]){
    assert(++ticks<1024);unsigned x=word(ram,0xd02),y=word(ram,0xd04);LagoonFollowerStep out;memset(&out,0,sizeof out);
    vector(&out,x,y,nav.x[n],nav.y[n],pace,0);assert(out.moving);
    assert(nav_step(ram,x,y,out.direction,absdiff(x,out.target_x)+absdiff(y,out.target_y)));
    ram[0xd19]=out.direction;put(ram,0xd06,(uint16_t)out.dx);put(ram,0xd08,(uint16_t)out.dy);
    run(0x019bce,0x800,0x017ff0,0);run(0x01a4d3,0x800,0x017ff0,0);
    assert(word(ram,0xd02)==out.target_x&&word(ram,0xd04)==out.target_y);
    assert(absdiff(x,word(ram,0xd02))+absdiff(y,word(ram,0xd04))<=pace);
  }
  assert(absdiff(word(ram,0xd02),tx)+absdiff(word(ram,0xd04),ty)<=7);
  return ticks;
}
static void navigator_obstacles(void){
  unsigned ticks=0,cases=0;
  for(unsigned kind=0;kind<2;kind++)for(unsigned pace=3;pace<=5;pace+=2){
    /* Full-height obstruction between actor and target: turn around an end,
     * preserving the wider NPC footprint rather than trying Nasir's point. */
    reset(kind,0x14);for(unsigned y=10;y<17;y++)solid(16,y);
    put(ram,0xd02,200);put(ram,0xd04,200);ticks+=walk_nav_route(320,200,pace);cases++;
    /* A 32px stair-shaped corridor bends right/up/right between solid banks.
     * Each selected vector is checked by actual original collision/integration. */
    reset(kind,0x14);
    for(unsigned cy=8;cy<24;cy++)for(unsigned cx=8;cx<28;cx++){
      int horizontal=(cy>=18&&cy<=19&&cx<=19)||(cy>=12&&cy<=13&&cx>=18);
      int vertical=cx>=18&&cx<=19&&cy>=12&&cy<=19;
      if(!horizontal&&!vertical)solid(cx,cy);
    }
    put(ram,0xd02,200);put(ram,0xd04,303);ticks+=walk_nav_route(384,207,pace);cases++;
    /* Reported sub-tile corner: lower right probe hits a wall although the
     * centerline is free. Original clamps x405 exactly; route must move up. */
    reset(kind,7);for(unsigned y=40;y<44;y++)for(unsigned x=26;x<31;x++)solid(x,y);
    for(unsigned y=42;y<44;y++)for(unsigned x=20;x<26;x++)solid(x,y);
    put(ram,0xd02,405);put(ram,0xd04,657);assert(!nav_step(ram,405,657,0,1));
    assert(nav_step(ram,405,657,3,1));ticks+=walk_nav_route(448,624,pace);cases++;
    /* Long oscillating player history ends where it began. Waiting follower
     * must head to the recent target and stop; never replay the old laps. */
    reset(kind,0x14);put(ram,0xd02,180);put(ram,0xd04,200);
    for(unsigned t=0;t<200;t++){
      ram[0x408]=(uint8_t)t;put(ram,0x502,200+3*(t%40<20?t%40:40-t%40));
      assert(LagoonFollower_Observe(ram,generation));
    }
    for(unsigned t=200;t<300;t++){
      ram[0x408]=(uint8_t)t;assert(LagoonFollower_Observe(ram,generation));
      unsigned x=word(ram,0xd02),y=word(ram,0xd04);follower();
      assert(absdiff(x,word(ram,0xd02))+absdiff(y,word(ram,0xd04))<=s.pace);
    }
    assert(absdiff(word(ram,0xd02),word(ram,0x502))<=23);cases++;
  }
  for(unsigned kind=0;kind<2;kind++){
    /* Cold SRAM/state load has no movement history. Standing Nasir must still
     * get an ordinary walking escort, then a stable18px bootstrap gap. */
    reset(kind,0x14);put(ram,0x502,320);put(ram,0x504,200);
    for(unsigned t=0;t<70;t++){
      ram[0x408]=(uint8_t)t;assert(LagoonFollower_Observe(ram,generation));
      unsigned x=word(ram,0xd02),y=word(ram,0xd04);follower();
      assert(absdiff(x,word(ram,0xd02))+absdiff(y,word(ram,0xd04))<=3);
    }
    assert(s.count==1);assert(absdiff(word(ram,0xd02),320)+absdiff(word(ram,0xd04),200)<=18);
    unsigned resting_x=word(ram,0xd02),resting_y=word(ram,0xd04);
    for(unsigned t=70;t<90;t++){ram[0x408]=(uint8_t)t;LagoonFollower_Observe(ram,generation);follower();}
    assert(word(ram,0xd02)==resting_x&&word(ram,0xd04)==resting_y);cases++;
    /* Change the collision geometry AFTER a route exists. The cached forward
     * vector must be discarded and a real detour walked, without wall passage. */
    reset(kind,0x14);put(ram,0x502,320);put(ram,0x504,200);
    for(unsigned t=0;t<240;t++){
      ram[0x408]=(uint8_t)t;assert(LagoonFollower_Observe(ram,generation));
      if(t==5)for(unsigned cy=8;cy<18;cy++)solid(14,cy);
      unsigned x=word(ram,0xd02),y=word(ram,0xd04);follower();
      assert(absdiff(x,word(ram,0xd02))+absdiff(y,word(ram,0xd04))<=3);
      if(t>=5&&word(ram,0xd04)>=137&&word(ram,0xd04)<=278)
        assert(word(ram,0xd02)<224||word(ram,0xd02)>=240);
    }
    assert(absdiff(word(ram,0xd02),320)+absdiff(word(ram,0xd04),200)<=18);cases++;
  }
  printf("PASS navigation %u Giles/Thor obstacle/backlog cases, %u selected steps through original NPC collision/integration\n",cases,ticks);
}
/* Exact original Thor lead setup and arrival branches. Coordinates below are
 * synthetic branch fixtures, not a natural quest playthrough. The source owns
 * the Wish target until it stops for dialogue; the navigator cannot claim it. */
static void thor_lead_oracle(void){
  static const int delta[][2]={{0,0},{15,0},{-15,0},{0,15},{0,-15},{15,-15},{16,0},{0,16},{-16,0},{0,-16}};
  for(unsigned n=0;n<sizeof delta/sizeof *delta;n++){
    reset(0,8);ram[0x41b]=1;
    run(0x02c95d,0x800,0x027ff0,0);
    assert((ram[0xd1f]&4)&&word(ram,0xd39)==0x100&&word(ram,0xd3b)==0x260&&!ram[0x41b]);
    assert(!LagoonFollower_Role(ram,0x800));
    uint8_t snapshot[sizeof ram];memcpy(snapshot,ram,sizeof ram);
    LagoonFollowerStep step;assert(!LagoonFollower_Plan(ram,0x800,generation,&step));assert(!memcmp(snapshot,ram,sizeof ram));
    put(ram,0xd02,0x100+delta[n][0]);put(ram,0xd04,0x260+delta[n][1]);
    run(0x029689,0x7c0,0x027ff0,0);
    if(n<6){assert(!(ram[0xd1f]&4)&&(ram[0x4e6]&128)&&ram[0xd0a]==1&&(ram[0xd00]&1));}
    else {assert((ram[0xd1f]&4)&&!(ram[0x4e6]&128)&&!ram[0xd1a]&&!(ram[0xd00]&1));}
    assert(!LagoonFollower_Role(ram,0x800));
  }
  printf("PASS Thor original lead setup/arrival: 10 original-opcode target and 15/16px threshold fixtures refuse native navigation\n");
}
int main(int argc,char**argv){
  setvbuf(stdout,NULL,_IONBF,0);
  assert(argc==2||argc==3);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  thor_lead_oracle();
  unsigned probe_cases=navigator_probe_oracle();navigator_obstacles();
  printf("PASS navigation leadingedge predicate: %u original-opcode cases, all256byte attributes and8 subtilephases readonly\n",probe_cases);
  for(unsigned kind=0;kind<2;kind++)for(unsigned pace=3;pace<=5;pace+=2){
    reset(kind,0x14);
    for(unsigned t=0;t<90;t++){
      ram[0x408]=(uint8_t)t;assert(LagoonFollower_Observe(ram,generation));
      unsigned oldx=word(ram,0xd02),oldy=word(ram,0xd04);
      /* Real source reads direction and speed tier; stops and four corners. */
      unsigned direction=t<16?0:t<32?1:t<48?2:t<64?3:4;
      player(direction,pace);ordinary(0x40,1);ordinary(0x80,2);follower();
      assert(absdiff(word(ram,0xd02),oldx)+absdiff(word(ram,0xd04),oldy)<=pace);
      assert(!(word(ram,0xd06)&&word(ram,0xd08)));
      assert(absdiff(word(ram,0xd06),0)<=5||word(ram,0xd06)>=65531);
    }
    unsigned gap=absdiff(word(ram,0xd02),word(ram,0x502))+absdiff(word(ram,0xd04),word(ram,0x504));
    if(!(gap>=6*pace-5&&gap<=6*pace+5))fprintf(stderr,"open-route kind%u pace%u finalgap%u position%u,%u target%u,%u\n",kind,pace,gap,word(ram,0xd02),word(ram,0xd04),word(ram,0x502),word(ram,0x504));
    assert(gap>=6*pace-5&&gap<=6*pace+5);
  }
  /* Core refuses state-generation/room/descriptor gaps, foreign roles and
   * scripted target walking without ever changing the caller's RAM. */
  reset(1,7);uint8_t before[sizeof ram];LagoonFollowerStep out;
  for(unsigned t=0;t<8;t++){ram[0x408]=t;put(ram,0x502,200+3*t);LagoonFollower_Observe(ram,generation);}
  memcpy(before,ram,sizeof ram);assert(!LagoonFollower_Plan(ram,0x80,generation,&out));assert(!memcmp(before,ram,sizeof ram));refusals++;
  ram[0xd1f]=4;assert(!LagoonFollower_Plan(ram,0x800,generation,&out));ram[0xd1f]=0;refusals++;
  generation++;assert(!LagoonFollower_Plan(ram,0x800,generation,&out));refusals++;
  put(ram,0x48e,8);assert(!LagoonFollower_Plan(ram,0x800,generation,&out));refusals++;
  reset(1,7);ram[0x408]=1;LagoonFollower_Observe(ram,generation);put(ram,0x502,900);ram[0x408]=2;assert(!LagoonFollower_Observe(ram,generation));refusals++;
  /* An actual solid original collision cell blocks the requested connector;
   * no host X/Y write can place the follower on the far side. */
  reset(1,7);put(ram,0xd02,180);for(unsigned t=0;t<10;t++){ram[0x408]=t;put(ram,0x502,200+3*t);LagoonFollower_Observe(ram,generation);}
  for(unsigned yy=0;yy<128;yy++)ram[0x14000+yy*128+12]=1;
  memset(ram+0x18004,0x80,4);unsigned initial=word(ram,0xd02);
  for(unsigned t=10;t<25;t++){ram[0x408]=t;LagoonFollower_Observe(ram,generation);follower();assert(word(ram,0xd02)<192);}
  assert(word(ram,0xd02)<=initial+5);
  /* Existing native body means this hook cannot own an interpreted site. */
  reset(1,7);has_aot=1;CpuState c;memset(&c,0,sizeof c);c.ram=ram;c.PB=c.DB=c.m_flag=1;c.P=32;c.S=0x1d0;c.A=0x11;c.X=0x800;put(ram,c.S+1,0x80bd);
  memcpy(before,ram,sizeof ram);hook(&c,0x0181ac);assert(!memcmp(before,ram,sizeof ram));assert(!call.active);refusals++;
  unsigned mutants=0;
  const unsigned spans[][2]={{0x018c4d,7},{0x018d41,101},{0x018da6,32},{0x05cc41,22},{0x05cc57,33},{0x05ccab,31},{0x05cd3d,25},{0x018184,55},{0x0280a0,137},{0x019bce,787},{0x01a4d3,151},{0x029526,530},{0x02c834,94},{0x02c92f,77},{0x029689,110},{0x01c84a,279}};
  for(unsigned n=0;n<sizeof spans/sizeof *spans;n++)for(unsigned j=0;j<spans[n][1];j++){
    unsigned a=off(spans[n][0])+j;rom[a]^=1;LagoonFollower_Reset(rom,sizeof rom);
    assert(!s.valid_rom&&!LagoonFollower_Observe(ram,generation));rom[a]^=1;mutants++;
  }
  const unsigned nav_spans[][2]={{0x018385,83},{0x019bce,1100}};
  unsigned nav_mutants=0;
  for(unsigned n=0;n<sizeof nav_spans/sizeof *nav_spans;n++)for(unsigned j=0;j<nav_spans[n][1];j++){
    unsigned a=off(nav_spans[n][0])+j;rom[a]^=1;LagoonFollower_Reset(rom,sizeof rom);assert(!nav_source);rom[a]^=1;nav_mutants++;
  }
  printf("PASS navigation source qualification: %u mutated lookup/collision/probe/table bytes refused\n",nav_mutants);
  for(unsigned bad=0;bad<23;bad++){
    reset(1,7);for(unsigned t=0;t<8;t++){ram[0x408]=t;put(ram,0x502,200+3*t);LagoonFollower_Observe(ram,generation);}
    memset(&c,0,sizeof c);c.ram=ram;c.PB=c.DB=c.m_flag=1;c.P=32;c.S=0x1d0;c.A=0x11;c.X=0x800;put(ram,c.S+1,0x80bd);
    hook(&c,0x0181ac);assert(call.active);c.A=0x13;hook(&c,0x0181b6);assert(call.called);
    ram[c.S-2]=0xb9;ram[c.S-1]=0x81;ram[c.S]=1;
    switch(bad){
      case 0:c.PB=2;break;case 1:c.DB=2;break;case 2:c.D=1;break;
      case 3:c.emulation=1;break;case 4:c.m_flag=0;break;case 5:c.x_flag=1;break;
      case 6:c._flag_D=1;break;case 7:c.P|=8;break;case 8:c.P^=32;break;
      case 9:c.S--;break;case 10:c.X=0x40;break;case 11:ram[c.S+1]^=1;break;
      case 12:ram[c.S-2]^=1;break;case 13:ram[c.S-1]^=1;break;case 14:ram[c.S]^=1;break;
      case 15:generation++;break;case 16:ram[0x408]++;break;case 17:ram[0x48e]++;break;
      case 18:ram[0x4e1]=0;break;case 19:ram[0xd1f]|=4;break;case 20:call.called=0;break;
      case 21:ram[0x417]=2;break;case 22:ram[0x41b]=1;break;
    }
    memcpy(before,ram,sizeof ram);hook(&c,0x0181ba);assert(!memcmp(before,ram,sizeof ram));assert(!call.active);refusals++;
  }
  reset(0,9);for(unsigned t=0;t<8;t++){ram[0x408]=t;put(ram,0x502,200+3*t);LagoonFollower_Observe(ram,generation);}
  ram[0x501]=0x20;put(ram,0x514,0x800);put(ram,0x516,0x280);uint8_t vertical[4];memcpy(vertical,ram+0x514,4);
  follower();assert(!memcmp(vertical,ram+0x514,4)&&ram[0x501]==0x20);
  assert(call.active);LagoonFollower_Discard();assert(!call.active&&!LagoonFollower_OwnsVelocity(0xd06));
  /* Authored Giles540 is FIRST; later unrelated actors must preserve its
   * cached route just as actors before global ThorD00 must do. */
  reset(1,7);memcpy(ram+0x540,ram+0xd00,64);memset(ram+0xd00,0,64);
  for(unsigned t=0;t<40;t++){
    ram[0x408]=t;LagoonFollower_Observe(ram,generation);player(t<20?0:4,5);
    run(0x018184,0x40,0x0180c7,1);ordinary(0x80,2);ordinary(0xc0,1);
  }
  assert(s.following&&s.actor==0x40&&absdiff(word(ram,0x542),word(ram,0x502))<=35);
  if(argc==3){
    /* Fresh ORIGINAL actual room15 loader bytes, including its collision
     * map/descriptor and Thor460,992 vs Nasir472,980. No actor XY edit. */
    f=fopen(argv[2],"rb");assert(f&&fread(ram,1,sizeof ram,f)==sizeof ram);fclose(f);
    assert(word(ram,0x48e)==0x15&&word(ram,0xd02)==460&&word(ram,0xd04)==992);
    LagoonFollower_Reset(rom,sizeof rom);generation++;unsigned moved=0;
    for(unsigned t=0;t<45;t++){
      ram[0x408]=t;assert(LagoonFollower_Observe(ram,generation));
      player(t<20?3:4,5);ordinary(0x40,1);ordinary(0x80,2);
      unsigned x=word(ram,0xd02),y=word(ram,0xd04);follower();
      unsigned d=absdiff(x,word(ram,0xd02))+absdiff(y,word(ram,0xd04));
      assert(d<=5);moved+=d;
    }
    assert(moved&&s.following);
    assert(absdiff(word(ram,0xd02),word(ram,0x502))<=5);
    assert(absdiff(word(ram,0xd04),word(ram,0x504))>=25&&absdiff(word(ram,0xd04),word(ram,0x504))<=35);
    printf("PASS authentic original-loader Thor navigation: %u actual walking pixels, joins up-route at capped pace, retains about30px stopped gap\n",moved);
  }
  unsigned body_cases=0;
  for(unsigned policy=0;policy<2;policy++){
    reset(0,0x15);assert(s.body_source);put(ram,0x502,472);put(ram,0x504,950);
    put(ram,0xd02,472);put(ram,0xd04,960);ram[0x53f]=4;ram[0x51a]=4;
    use_body_filter=(int)policy;
    for(unsigned t=0;t<8;t++){
      ram[0x408]=t;
      /* Entire ORIGINAL player actor update, actual tile collision, movement,
       * actual80C7JSR/scanner/response, then original escort own body handling. */
      run(0x018184,0,0x0180ca,1);run(0x018184,0x800,0x0180ca,1);
      if(!t){assert(s.count==1);assert(word(ram,0x504)==(policy?955:950));}
    }
    assert(policy?word(ram,0x504)==990:word(ram,0x504)<990);
    if(policy)assert(counts.body_reads&&s.count>6);
    body_cases++;
  }
  /* Unrelated NPC and enemy stay exactly original, including original bytes
   * and returned CPU. No role merely borrows the active quest flag. */
  for(unsigned type=1;type<=2;type++){
    uint8_t original[sizeof ram];CpuState original_cpu;
    for(unsigned policy=0;policy<2;policy++){
      reset(0,0x15);put(ram,0x502,472);put(ram,0x504,950);put(ram,0x520,80);
      put(ram,0xd02,472);put(ram,0xd04,960);ram[0xd18]=(uint8_t)type;ram[0xd1b]=4;
      ram[0x53f]=4;ram[0x51a]=4;use_body_filter=(int)policy;
      run(0x018184,0,0x0180ca,1);
      if(!policy){memcpy(original,ram,sizeof ram);original_cpu=last_cpu;}
      else {assert(!memcmp(original,ram,sizeof ram));assert(!memcmp(&original_cpu,&last_cpu,sizeof last_cpu));}
    }
    body_cases++;
  }
  use_body_filter=0;
  for(unsigned bad=0;bad<18;bad++){
    reset(0,0x15);assert(LagoonFollower_Observe(ram,generation));
    memset(&c,0,sizeof c);c.ram=ram;c.PB=c.DB=c.m_flag=1;c.P=32;c.S=0x1d0;
    hook(&c,0x0180c7);assert(body.active);
    CpuWramReadContext context={0x01a110,0x1ce,0x800,1,1,0,0,1};
    put(ram,context.stack+1,0x80c9);unsigned address=0xd00;
    switch(bad){
      case 0:context.interpreter=0;break;case 1:context.instruction_pc++;break;
      case 2:context.pb=2;break;case 3:context.m=0;break;case 4:context.x=1;break;
      case 5:context.e=1;break;case 6:context.stack--;break;case 7:context.y=0x40;break;
      case 8:address++;break;case 9:ram[context.stack+1]^=1;break;case 10:ram[context.stack+2]^=1;break;
      case 11:ram[c.S+1]^=1;break;case 12:generation++;break;case 13:ram[0x408]++;break;
      case 14:ram[0x48e]++;break;case 15:ram[0x4e1]=0;break;
      case 16:ram[0xd1f]|=4;break;case 17:hook(&c,0x0180ca);break;
    }
    memcpy(before,ram,sizeof ram);uint8_t value=ram[address];
    assert(LagoonFollower_FilterRead(address,value,&context,ram)==value);
    assert(!memcmp(before,ram,sizeof ram));refusals++;
  }
  for(unsigned bad=0;bad<15;bad++){
    reset(0,0x15);LagoonFollower_Observe(ram,generation);
    memset(&c,0,sizeof c);c.ram=ram;c.PB=c.DB=c.m_flag=1;c.P=32;c.S=0x1d0;
    switch(bad){
      case 0:c.PB=2;break;case 1:c.DB=2;break;case 2:c.D=1;break;
      case 3:c.emulation=1;break;case 4:c.m_flag=0;break;case 5:c.x_flag=1;break;
      case 6:c._flag_D=1;break;case 7:c.P|=8;break;case 8:c.X=0x800;break;
      case 9:c.S=0x100;break;case 10:c.S=0x1ffe;break;case 11:has_aot=1;break;
      case 12:ram[0x518]=1;break;case 13:ram[0x41b]=1;break;case 14:generation++;break;
    }
    hook(&c,0x0180c7);assert(!body.active);refusals++;
  }
  reset(1,7);LagoonFollower_Observe(ram,generation);memcpy(ram+0x540,ram+0xd00,64);
  memset(&c,0,sizeof c);c.ram=ram;c.PB=c.DB=c.m_flag=1;c.P=32;c.S=0x1d0;
  hook(&c,0x0180c7);assert(!body.active);refusals++;
  const unsigned body_spans[][2]={{0x0180c7,3},{0x01a0e1,126},{0x01a15d,230},{0x01a37e,58},{0x01a493,64},{0x0180ca,3}};
  unsigned body_mutants=0;
  for(unsigned n=0;n<sizeof body_spans/sizeof *body_spans;n++)for(unsigned j=0;j<body_spans[n][1];j++){
    unsigned a=off(body_spans[n][0])+j;rom[a]^=1;LagoonFollower_Reset(rom,sizeof rom);assert(!s.body_source);rom[a]^=1;body_mutants++;
  }
  /* Exact approved centered-camera continuation works. A different apparent
   * camera trampoline must not acquire the read-view authority. */
  uint8_t original_dispatch[35],original_call[3];memcpy(original_dispatch,rom+0x83d8,35);memcpy(original_call,rom+0x80ca,3);
  const uint8_t centered_dispatch[]={0x5a,0x9b,0xd0,0x16,0xda,0x64,0x41,0x20,0xfb,0x83,0x20,0x26,0x84,0x20,0x52,0x84,0x20,0x7d,0x84,0xfa,0xad,0x19,0x05,0x8d,0x19,0x05,0x7a,0x4c,0xb5,0xa7,0xea,0xea,0xea,0xea,0xea};
  memcpy(rom+0x83d8,centered_dispatch,35);memcpy(rom+0x80ca,"\x20\xd8\x83",3);
  LagoonFollower_Reset(rom,sizeof rom);assert(s.valid_rom&&s.body_source);
  for(unsigned j=0;j<35;j++){rom[0x83d8+j]^=1;LagoonFollower_Reset(rom,sizeof rom);assert(s.valid_rom&&!s.body_source);rom[0x83d8+j]^=1;body_mutants++;}
  memcpy(rom+0x83d8,original_dispatch,35);memcpy(rom+0x80ca,original_call,3);
  reset(0,0x15);assert(s.body_source);LagoonFollower_Discard();assert(s.body_source&&!body.active);
  printf("PASS original body collision: %u full-source policy cases, bootstrap reversal + readonly scanner/mode/stack/role/generation refusals\n",body_cases);
  printf("PASS body source/camera qualification: %u mutated source bytes refused, stock and exact centered continuation accepted\n",body_mutants);
  printf("PASS follower %u actual source steps / %u hooks / %u opcodes / %u refusal groups / %u original source-byte mutations\n",steps,hooks,opcodes,refusals,mutants);
  return 0;
}
