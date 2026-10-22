/* Reuse the independently clocked original-opcode bus and CPU model. */
#include "cpu_state.h"
#include "snes/interp_bridge.h"
#define NATIVE_SCRIPT_CLOSURE
#define main old_inline_main
#define interp_tier_run_call_frame old_call_boundary
#define interp_tier_dispatch_tail old_tail_boundary
#include "lagoon_inline_dispatch_oracle_test.c"
#undef main
#undef interp_tier_run_call_frame
#undef interp_tier_dispatch_tail
static RecompReturn dialog_stop(CpuState*c,unsigned pc){(void)c;pending=1;boundary=pc;inherit=0;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;}
RecompReturn interp_tier_run_call_frame(CpuState*c,uint32 pc,uint32 site,uint8 n,uint32*out){(void)site;(void)n;if(out)*out=pc;return dialog_stop(c,pc);}
RecompReturn interp_tier_dispatch_tail(CpuState*c,uint32 pc,uint32 site,uint16 s,uint8 h){(void)site;(void)s;(void)h;return dialog_stop(c,pc);}
#include "dialog_services_under_test.h"
static unsigned complete_count,call_count,deadline_count;
static void dialog_compare(unsigned pc,unsigned variant,unsigned flags,unsigned limit){
 for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8)(n*31+variant*17+cases);
 memset(io,0,sizeof io);memset(ram+0x4800,0,sizeof ram-0x4800);
 const uint8 messages[][8]={{255},{32,255},{13,32,255},{17,28,32,255}};
 memcpy(ram+0x5000,messages[variant%4],8);
 ram[0x50+0x400]=(unsigned[]){0,4,0x40,0xff}[variant%4];ram[0x451]=0x80;
 ram[0x440]=0x42;ram[0x441]=0;ram[0xd0]=0;ram[0xd1]=0x18;ram[0xd2]=0x7e;ram[0x1800]=255;
 ram[0x238]=ram[0x239]=0;ram[0x41a]=variant&1;ram[0x41b]=variant&2;
 unsigned off=dialog_offset(pc),owner=(variant&1)?0x1fd0:0x180;
 CpuState initial={0};initial.ram=ram;initial.PB=2;initial.DB=(variant&2)?2:1;
 initial.S=owner-off;initial.A=(variant*0x1111)&65535;initial.X=(variant&1)?0xffff:0;initial.Y=variant*17;
 initial.P=CPU_P_M|(flags&1?0x81:0)|(flags&2?0x42:0)|(flags&4?8:0);cpu_p_to_mirrors(&initial);
 unsigned rtl=pc==0x2826c||pc==0x28277||pc==0x28282||pc==0x2828d||pc==0x28298||pc==0x28274||pc==0x2827f||pc==0x2828a||pc==0x28295||pc==0x282a9;
 initial.host_return_valid=rtl?3:2;ram[owner+1]=0x4f;ram[owner+2]=0x90;ram[owner+3]=2;
 memcpy(before,ram,sizeof ram);CpuState actual=initial;pending=boundary=inherit=0;g_recomp_stack_top=0;deadline=limit;
 RecompReturn out=dialog_body(&actual,pc);cpu_mirrors_to_p(&actual);
 if(!pending){CHECK(out==RECOMP_RETURN_NORMAL);boundary=0x029050;complete_count++;}else if(boundary==pc||actual.master_cycles>=deadline)deadline_count++;else call_count++;
 CHECK(g_recomp_stack_top==0);memcpy(expected,ram,sizeof ram);memcpy(expected_io,io,sizeof io);
 memcpy(ram,before,sizeof ram);memset(io,0,sizeof io);Interp816*i=make_interp(&initial,pc);unsigned ops=0;uint64 cyc=0,last=0;bus_extra=0;
 while(cyc<actual.cycles){CHECK(++ops<100000);last=cyc*6+bus_extra;cyc+=interp816_runOpcode(i);}
 unsigned different=0;while(different<sizeof ram&&expected[different]==ram[different])different++;
 if(different!=sizeof ram||memcmp(io,expected_io,sizeof io)||i->a!=actual.A||i->x!=actual.X||i->y!=actual.Y||i->sp!=actual.S||i->db!=actual.DB||i->dp!=actual.D||i->k!=actual.PB||interp816_getFlags(i)!=actual.P||((unsigned)i->k<<16|i->pc)!=boundary){fprintf(stderr,"DIALOG STATE pc%06x case%u off%u RAM%05x A%04x/%04x S%04x/%04x P%02x/%02x PC%06x/%06x\n",pc,cases,off,different,i->a,actual.A,i->sp,actual.S,interp816_getFlags(i),actual.P,((unsigned)i->k<<16|i->pc),boundary);CHECK(0);}
 if(cyc!=actual.cycles||cyc*6+bus_extra!=actual.master_cycles||last!=actual.coprocessor_master_cycles){fprintf(stderr,"DIALOG CLOCK pc%06x cpu%llu/%llu master%llu/%llu copro%llu/%llu\n",pc,(unsigned long long)cyc,(unsigned long long)actual.cycles,(unsigned long long)(cyc*6+bus_extra),(unsigned long long)actual.master_cycles,(unsigned long long)last,(unsigned long long)actual.coprocessor_master_cycles);CHECK(0);}
 interp816_free(i);cases++;
}
int main(int argc,char**argv){CHECK(argc==2);FILE*f=fopen(argv[1],"rb");CHECK(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);g_rom=rom;
 const unsigned deadlines[]={1,20,80,200,500,1200,5000};
 for(unsigned k=0;k<sizeof dialog_roots/sizeof*dialog_roots;k++)for(unsigned v=0;v<8;v++)for(unsigned flags=0;flags<8;flags++)for(unsigned phase=0;phase<sizeof deadlines/sizeof*deadlines;phase++)dialog_compare(dialog_roots[k],v,flags,deadlines[phase]);
 CHECK(complete_count&&call_count&&deadline_count);printf("PASS dialogue %u original-opcode cases/%u physical returns/%u external entries/%u deadline entries; fullRAM/IO/stack/registers/flags/productionregion CPU-master-copro clocks; externalcall frames retained, no fake child/wait completion\n",cases,complete_count,call_count,deadline_count);return 0;}
