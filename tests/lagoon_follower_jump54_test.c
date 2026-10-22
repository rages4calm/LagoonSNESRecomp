/* Focused new jump corridor: real original player jump, NPC collision and XY integration. */
#define main previous_follower_main
#include "../tools/test_follower.c"
#undef main
int main(int argc,char**argv){
 assert(argc==2||argc==3);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 unsigned cases=0;
 for(unsigned dir=0;dir<4;dir++){
  reset(0,10);use_body_filter=1;
  unsigned sx=dir==2?320:200,sy=dir==3?320:200;
  put(ram,0x502,sx);put(ram,0x504,sy);put(ram,0xd02,sx);put(ram,0xd04,sy);
  memset(ram+0x18004,4,4);
  for(unsigned y=0;y<1024;y+=16)for(unsigned x=0;x<1024;x+=16)
   if((dir%2==0?x:y)>=240&&(dir%2==0?x:y)<272)ram[0x14000+(y&65520)*8+(x>>4)]=1;
  assert(LagoonFollower_Observe(ram,generation));
  ram[0x50a]=(uint8_t)dir;ram[0x53e]=128;run(0x028199,0,0x027ff0,0);ram[0x53e]=0;
  unsigned airborne=0,replayed=0;
  for(unsigned t=1;t<150;t++){
   ram[0x408]=(uint8_t)t;assert(LagoonFollower_Observe(ram,generation));
   if(ram[0x501]&32){player(dir,5);airborne++;}
   else player(4,5);
   unsigned ox=word(ram,0xd02),oy=word(ram,0xd04),pz=word(ram,0x514),pf=ram[0x501];
   follower();
   assert(word(ram,0x514)==pz&&ram[0x501]==pf); /* never changes Nasir physics */
   assert(absdiff(ox,word(ram,0xd02))+absdiff(oy,word(ram,0xd04))<=5);
   if(call.step.connector==2)replayed++;
  }
  printf("dir%u air%u replay%u p%u,%u f%u,%u q%u rec%u active%u\n",dir,airborne,replayed,word(ram,0x502),word(ram,0x504),word(ram,0xd02),word(ram,0xd04),jumps.count,jumps.recording.count,jumps.count?jumps.q[jumps.head].active:0);fflush(stdout);assert(airborne>10&&replayed>10);
  assert(dir==0?word(ram,0xd02)>272:dir==2?word(ram,0xd02)<240:dir==1?word(ram,0xd04)>272:word(ram,0xd04)<240);
  assert(!word(ram,0xd14)&&!ram[0xd01]);cases++;
 }

 if(argc==3){
  f=fopen(argv[2],"rb");assert(f&&fread(ram,1,sizeof ram,f)==sizeof ram);fclose(f);
  assert(word(ram,0x48e)==10);LagoonFollower_Reset(rom,sizeof rom);has_aot=0;use_body_filter=1;
  ram[0x500]=0xc0;ram[0x501]=ram[0x41b]=ram[0x100e]=0;ram[0x417]=1;ram[0x518]=0;ram[0x51a]=2;ram[0x4e1]=2;ram[0x50b]=ram[0x519]=255;ram[0x50a]=2;ram[0x408]=0;
  memset(ram+0xd00,0,64);ram[0xd00]=0xc8;ram[0xd18]=1;ram[0xd0b]=ram[0xd19]=255;
  put(ram,0x502,224);put(ram,0x504,480);put(ram,0x514,0);put(ram,0xd02,224);put(ram,0xd04,480);
  assert(LagoonFollower_Observe(ram,generation));ram[0x53e]=128;run(0x028199,0,0x027ff0,0);ram[0x53e]=0;
  unsigned replayed=0,saved=0;static uint8_t midair[0x20000];
  for(unsigned n=1;n<180;n++){
   ram[0x408]=(uint8_t)n;assert(LagoonFollower_Observe(ram,generation));player((ram[0x501]&32)?2:4,5);follower();if(call.step.connector==2)replayed++;
   if(!saved&&word(ram,0xd02)>=160&&word(ram,0xd02)<192&&word(ram,0xd14)){memcpy(midair,ram,sizeof ram);saved=1;}
  }
  assert(word(ram,0x502)==109&&word(ram,0xd02)<160&&replayed>10&&!word(ram,0xd14));
  printf("PASS actualPhilips original-map west bridge gap, player%u escort%u, %u witnessed jump fields\n",word(ram,0x502),word(ram,0xd02),replayed);
  assert(saved);memcpy(ram,midair,sizeof ram);generation++;LagoonFollower_Reset(rom,sizeof rom);put(ram,0x502,109);put(ram,0x504,480);put(ram,0x514,0);ram[0x501]=0;
  for(unsigned n=1;n<40;n++){ram[0x408]++;assert(LagoonFollower_Observe(ram,generation));follower();}
  printf("resume f%u,%u z%u queue%u step%u desc%u center?\n",word(ram,0xd02),word(ram,0xd04),word(ram,0xd14),jumps.count,call.step.connector,memcmp(ram+0x50,rom+0x2b50f,56));fflush(stdout);assert(word(ram,0xd02)<160&&!word(ram,0xd14));
  printf("PASS actualPhilips midair save generation recovery, bounded source-map path to ground, no XY snap\n");
  for(unsigned n=0;n<13;n++){
   memcpy(ram,midair,sizeof ram);LagoonFollower_Reset(rom,sizeof rom);put(ram,0x502,109);put(ram,0x504,480);put(ram,0x514,0);ram[0x501]=0;
   assert(LagoonFollower_Observe(ram,generation));unsigned q=0x800;
   switch(n){case 0:put(ram,0xd02,159);break;case 1:put(ram,0xd02,192);break;case 2:put(ram,0xd04,455);break;case 3:put(ram,0xd04,521);break;
    case 4:put(ram,0xd14,0);break;case 5:put(ram,0xd14,0x3001);break;case 6:q=0x40;break;case 7:ram[0x48e]=11;break;
    case 8:ram[0x50]^=1;break;case 9:put(ram,0x502,176);break;case 10:put(ram,0x504,16);break;
    case 11:ram[0xd1b]=1;ram[0x4e1]=1;break;case 12:rom[0x2b50f]^=1;break;}
   LagoonFollowerStep o;uint8_t before[sizeof ram];memcpy(before,ram,sizeof ram);LagoonFollower_Plan(ram,q,generation,&o);
   assert(!jumps.count&&!memcmp(before,ram,sizeof ram));if(n==12)rom[0x2b50f]^=1;
  }
  puts("PASS actualPhilips midair recovery: 13 room/role/source/height/bank/bounds refusals, all guest bytes readonly");

 }
 /* Exact live read-view boundary and independent guard mutations. */
 for(unsigned n=0;n<15;n++){
  reset(0,10);LagoonFollower_Observe(ram,generation);memset(ram+0x18000,4,4);
  call.active=1;call.actor=0x800;call.parent_s=0x1d0;call.generation=generation;call.tick=ram[0x408];
  call.step=(LagoonFollowerStep){5,0,205,200,1408,0,1,2};
  put(ram,0x1d1,0x80c3);put(ram,0xb8,211);put(ram,0xba,200);ram[0x2c1]=4;
  CpuWramReadContext c={0x019e29,0x1d0,0,1,1,0,0,1};
  assert(jump_filter_read(0x2c1,4,&c,ram)==3);
  switch(n){case 0:c.interpreter=0;break;case 1:c.pb=2;break;case 2:c.m=0;break;case 3:c.x=1;break;case 4:c.e=1;break;case 5:c.stack--;break;case 6:c.instruction_pc++;break;case 7:c.y=1;break;case 8:ram[0x1d1]++;break;case 9:ram[0xb8]++;break;case 10:ram[0x408]++;break;case 11:generation++;break;case 12:ram[0x50]++;break;case 13:ram[0xd1f]|=4;break;case 14:put(ram,0xd02,201);break;}
  assert(jump_filter_read(0x2c1,4,&c,ram)==4);cases++;
 }
 printf("PASS jump54 15 exact source-PC/owner/mode/probe/role/generation read-view refusals\n");
 /* Solid walls cannot acquire gap authority from a forged recorded arc. */
 reset(0,10);memset(ram+0x18004,128,4);ram[0x14000+192*8+(240>>4)]=1;
 JumpPath p={0};p.count=3;p.p[0]=(Sample){228,200,0,0};p.p[1]=(Sample){233,200,1408,0};p.p[2]=(Sample){238,200,0,0};
 assert(!jump_check(ram,&p));
 /* A script/load/teleport/descriptor change never retains crossing authority. */
 for(unsigned n=0;n<4;n++){
  reset(0,10);LagoonFollower_Observe(ram,generation);jumps.count=1;jumps.q[0]=p;
  if(n==0)generation++;if(n==1)ram[0x48e]++;if(n==2)ram[0x50]++;if(n==3)ram[0x41b]=1;
  LagoonFollower_Observe(ram,generation);assert(!jumps.count);cases++;
 }
 printf("PASS jump54 %u direction/reset groups, original opcodes=%u, bounded speed, solid refusal, player jump untouched\n",cases,opcodes);return 0;
}
