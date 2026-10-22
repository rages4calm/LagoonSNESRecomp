/* Complete source parents through their first real external child/return/deadline. */
#include "parent_infrastructure.h"
#include "parent_under_test.h"
static void enter(CpuState*c,unsigned pc,unsigned kind){
 Interp816*i=make_interp(c,pc);unsigned target=parent_pcs[kind],ops=0;
 do{CHECK(++ops<100);interp816_runOpcode(i);}while((((unsigned)i->k<<16)|i->pc)!=target);
 copy_cpu(c,i);interp816_free(i);c->host_return_valid=kind==1||kind==2||kind==3?2:3;
}
int main(int argc,char**argv){CHECK(argc==2);FILE*f=fopen(argv[1],"rb");CHECK(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 unsigned done=0,stops=0,yields=0;
 for(unsigned kind=0;kind<8;kind++)for(unsigned variant=0;variant<64;variant++)for(unsigned phase=0;phase<8;phase++){
  for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*7+variant*31+kind*13);memset(io,0,sizeof io);
  CpuState c={0};c.ram=ram;c.S=(variant&1)?0x1fd0:0x180;c.PB=1;c.DB=1;c.P=0x24|((variant&2)?0xc1:0);c.A=kind==1?30:kind==2?39:0x15;c.X=variant*2;c.Y=variant*3;cpu_p_to_mirrors(&c);
  ram[0x497]=(variant&1)?0x80:0;ram[0x48e]=(variant&2)?2:0x40+(variant&15);ram[0x51]=(variant&4)?4:0;ram[0x84]=variant;
  ram[0x4e0]=(uint8)variant;ram[0x4e2]=(uint8)(variant&0x24);ram[0x41a]=(variant&1)?1:0;ram[0x450]=(variant&2)?2:0;ram[0x370]=(uint8)(variant*3);
  if(kind>=5){ram[0x497]=0;ram[0x48e]=kind==7?2:0x40+(variant&15);ram[0x51]=kind==6?4:0;ram[0x4e0]=0;}
  unsigned site=kind==0?0x01872c:kind==1||kind==2?0x01ccd5:kind==3?0x028493:kind==4?0x009110:0x01872c;
  if(kind==3)c.PB=2;if(kind==4)c.PB=0;
  enter(&c,site,kind);CpuState initial=c;memcpy(before,ram,sizeof ram);
  pending=target_handoff=inherit=boundary=0;g_recomp_stack_top=0;deadline=phase?phase*42:0;RecompReturn rr=parent_body(parent_pcs[kind],&c);unsigned stop=boundary;
  CHECK(!g_recomp_stack_top&&!inherit);if(pending){if(target_handoff)stops++;else yields++;}else{CHECK(rr==RECOMP_RETURN_NORMAL);done++;}
  cpu_mirrors_to_p(&c);memcpy(expected,ram,sizeof ram);CpuState actual=c;
  memcpy(ram,before,sizeof ram);Interp816*i=make_interp(&initial,parent_pcs[kind]);uint64 cyc=0,last=0;bus_extra=0;unsigned ops=0;
  if(!pending){stop=kind==0?0x018730:kind==1||kind==2?0x01ccd8:kind==3?0x028496:kind==4?0x0080e4:0x018730;}
  while(cyc<actual.cycles){CHECK(++ops<2000);last=cyc*6+bus_extra;cyc+=interp816_runOpcode(i);}
  CHECK((((unsigned)i->k<<16)|i->pc)==stop);
  unsigned first=0;while(first<sizeof ram&&ram[first]==expected[first])first++;
  /* NORMAL host return leavesPB restoration to its real paired caller. */
  unsigned expected_pb=pending?i->k:initial.PB;
  if(first!=sizeof ram||actual.A!=i->a||actual.X!=i->x||actual.Y!=i->y||actual.S!=i->sp||actual.DB!=i->db||actual.D!=i->dp||actual.P!=interp816_getFlags(i)||actual.PB!=expected_pb||actual.cycles!=cyc||actual.master_cycles!=cyc*6+bus_extra||actual.coprocessor_master_cycles!=last){
   fprintf(stderr,"STATE parent%06x variant%u phase%u stop%06x RAM%05x A%04x/%04x X%04x/%04x Y%04x/%04x S%04x/%04x PB%02x/%02x P%02x/%02x CPU%llu/%llu MASTER%llu/%llu COPRO%llu/%llu\n",parent_pcs[kind],variant,phase,stop,first,actual.A,i->a,actual.X,i->x,actual.Y,i->y,actual.S,i->sp,actual.PB,expected_pb,actual.P,interp816_getFlags(i),(unsigned long long)actual.cycles,(unsigned long long)cyc,(unsigned long long)actual.master_cycles,(unsigned long long)(cyc*6+bus_extra),(unsigned long long)actual.coprocessor_master_cycles,(unsigned long long)last);CHECK(0);
  }
  cases++;interp816_free(i);
 }
 printf("PASS bank parents %u source cases/%u physical returns/%u real child entries/%u deadlines, actualCCD5 inline ancestor and finite806F/CD90/D3CD closure, fullRAM/flags/stack/region-bus clocks\n",cases,done,stops,yields);return 0;
}
