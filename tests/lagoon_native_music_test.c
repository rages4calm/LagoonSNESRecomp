/* Source-owned leaf writers, including their observable ordered APU access. */
#include "cpu_state.h"
#include "snes/interp816.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL line%d case%u %s\n",__LINE__,cases,#x);exit(2);}}while(0)
static unsigned cases,refusals,fallbacks,pending,boundary;static uint64_t deadline,generation=7;
static uint8_t rom[0x100000],ram[0x20000],before[0x20000],expected[0x20000],io[65536],expected_io[65536];
const uint8_t*g_rom;CpuState g_cpu;uint8_t g_memsel;
static unsigned takes[3],stack_override,mutate_return,port_entry_s;
uint64_t RtlStateGeneration(void){return generation;}
void LagoonNativeEndgameConsumeMusic(unsigned w){CHECK(w<3);takes[w]++;}
void LagoonNativeEndgameRefuseMusic(unsigned w){CHECK(w<3);refusals++;}
void cpu_instruction_timing_commit(CpuState*c){c->coprocessor_master_cycles=c->master_cycles;}
int interp_bridge_lle_master_deadline_reached(const CpuState*c){return deadline&&c->master_cycles>=deadline;}
RecompReturn interp_bridge_lle_yield_unwind(CpuState*c,uint32_t p){(void)c;pending=1;boundary=p;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;}
RecompReturn interp_tier_dispatch_rewritten_return(CpuState*c,uint32_t p,uint32_t site){(void)c;(void)site;pending=1;boundary=p;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;}
static uint16_t fallback_s;static uint8_t fallback_h;
RecompReturn interp_tier_dispatch_tail(CpuState*c,uint32_t p,uint32_t site,uint16_t s,uint8_t h){(void)c;CHECK(p==site);fallbacks++;boundary=p;fallback_s=s;fallback_h=h;return (RecompReturn)RECOMP_RETURN_LLE_UNWIND_BASE;}
#include "music_bus_under_test.h"
typedef struct{uint64_t cycles,master,copro;unsigned port,value;}Event;
static Event events[4],expected_events[4];static unsigned event_count;
static void event(CpuState*c,unsigned port,unsigned value){CHECK(event_count<4);events[event_count++]=(Event){c->cycles,c->master_cycles,c->coprocessor_master_cycles,port,value};if(mutate_return)ram[port_entry_s+mutate_return]^=1;}
static uint8_t peek(unsigned a){int32_t p=cpu_wram_offset(a>>16,a);if(p>=0)return ram[p];if((a&65535)>=0x8000)return rom[((a>>16)&31)*32768+(a&32767)];return io[a&65535];}
static void poke(unsigned a,uint8_t v){int32_t p=cpu_wram_offset(a>>16,a);if(p>=0)ram[p]=v;else io[a&65535]=v;}
uint8_t cpu_read8(CpuState*c,uint8_t b,uint16_t a){return c->open_bus=peek(((unsigned)b<<16)|a);}
void cpu_write8(CpuState*c,uint8_t b,uint16_t a,uint8_t v){c->open_bus=v;if(a>=0x2140&&a<=0x2142)event(c,a,v);poke(((unsigned)b<<16)|a,v);}
#include "../src/lagoon_native_music.c"
static uint64_t bus_extra;
static uint8_t rd(void*context,uint32_t a){CpuState*c=context;bus_extra+=cpu_region_speed(a)-6;return c->open_bus=peek(a);}
static void wr(void*context,uint32_t a,uint8_t v){CpuState*c=context;bus_extra+=cpu_region_speed(a)-6;c->open_bus=v;if((a&65535)>=0x2140&&(a&65535)<=0x2142)event(c,a&65535,v);poke(a,v);}
static Interp816* make(CpuState*c,unsigned p){Interp816*i=interp816_init(c,rd,wr);CHECK(i);cpu_mirrors_to_p(c);i->a=c->A;i->x=c->X;i->y=c->Y;i->dp=c->D;i->db=c->DB;i->k=p>>16;i->pc=p;i->sp=c->S;i->e=c->emulation;interp816_setFlags(i,c->P);return i;}
static CpuState setup(unsigned w,unsigned caller,unsigned value,unsigned flags){
 for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8_t)(n*31+cases*7);memset(io,0,sizeof io);event_count=0;
 CpuState c={0};c.ram=ram;c.PB=0x0c;c.DB=1;c.S=stack_override?stack_override+3:(cases&1)?0x1fd0:0x180;c.A=value;c.X=0xffff-cases;c.Y=cases*17;c.P=CPU_P_M|(flags&0xc7);cpu_p_to_mirrors(&c);
 Interp816*i=make(&c,0x0c0000|calls[w][caller]);bus_extra=0;CHECK(interp816_runOpcode(i)==8);CHECK(i->pc==(targets[w]&65535)&&i->k==0x0c&&i->sp==c.S-3);c.S=i->sp;port_entry_s=c.S;interp816_free(i);c.host_return_valid=3;c.cycles=100;c.master_cycles=1000000;c.coprocessor_master_cycles=1000000;event_count=0;return c;
}
static void compare(unsigned w,unsigned caller,unsigned value,unsigned flags,unsigned limit){
 CpuState initial=setup(w,caller,value,flags);memcpy(before,ram,sizeof ram);CpuState c=initial;RecompReturn out=RECOMP_RETURN_NORMAL;pending=boundary=0;deadline=limit?initial.master_cycles+limit:0;
 CHECK(LagoonNativeMusic_Try(rom,sizeof rom,&c,&c,generation,generation,w,c.S,3,0x0c0000|calls[w][caller]+4,&out));
 unsigned stop=pending?boundary:(0x0c0000|calls[w][caller]+4);CHECK(pending||out==RECOMP_RETURN_NORMAL);cpu_mirrors_to_p(&c);memcpy(expected,ram,sizeof ram);memcpy(expected_io,io,sizeof io);unsigned expected_count=event_count;memcpy(expected_events,events,sizeof events);
 memcpy(ram,before,sizeof ram);memset(io,0,sizeof io);event_count=0;CpuState ref=initial;Interp816*i=make(&ref,targets[w]);uint64_t cycles=0;bus_extra=0;unsigned ops=0;
 while(cycles<c.cycles-initial.cycles){CHECK(++ops<=8);unsigned n=interp816_runOpcode(i);cycles+=n;ref.cycles=initial.cycles+cycles;ref.master_cycles=initial.master_cycles+cycles*6+bus_extra;ref.coprocessor_master_cycles=ref.master_cycles;}
 CHECK(((unsigned)i->k<<16|i->pc)==stop);CHECK(i->a==c.A&&i->x==c.X&&i->y==c.Y&&i->sp==c.S&&i->dp==c.D&&i->db==c.DB&&i->k==c.PB&&interp816_getFlags(i)==c.P);
 CHECK(!memcmp(expected,ram,sizeof ram)&&!memcmp(expected_io,io,sizeof io));if(c.cycles!=ref.cycles||c.master_cycles!=ref.master_cycles||c.coprocessor_master_cycles!=ref.coprocessor_master_cycles||c.open_bus!=ref.open_bus){fprintf(stderr,"CLOCK limit%u cpu%llu/%llu master%llu/%llu copro%llu/%llu open%02x/%02x\n",limit,(unsigned long long)c.cycles,(unsigned long long)ref.cycles,(unsigned long long)c.master_cycles,(unsigned long long)ref.master_cycles,(unsigned long long)c.coprocessor_master_cycles,(unsigned long long)ref.coprocessor_master_cycles,c.open_bus,ref.open_bus);CHECK(0);}
 CHECK(event_count==expected_count&&!memcmp(events,expected_events,event_count*sizeof*events));
 if(event_count){CHECK(event_count==1&&events[0].cycles==115&&events[0].master==1000108&&events[0].copro==1000108&&events[0].value==(value&255));}
 interp816_free(i);cases++;
}
static void negatives(void){for(unsigned w=0;w<3;w++)for(unsigned n=0;n<30;n++){
 CpuState c=setup(w,0,0xa580,0xc3),saved;unsigned s=c.S;uint8_t h=3;unsigned host=0x0c0000|calls[w][0]+4;const uint8_t*r=rom;size_t size=sizeof rom;const CpuState*current=&c;uint64_t frame=generation;unsigned writer=w;uint8_t old=0;unsigned at=0;
 switch(n){case 0:r=NULL;break;case 1:size--;break;case 2:current=NULL;break;case 3:c.ram=NULL;break;case 4:frame++;break;case 5:frame=0;break;case 6:writer=3;break;case 7:c.PB=1;break;case 8:c.DB=0;break;case 9:c.D=1;break;case 10:c.m_flag=0;break;case 11:c.x_flag=1;break;case 12:c.emulation=1;break;case 13:c._flag_D=1;break;case 14:c.P|=CPU_P_X;break;case 15:s++;break;case 16:h=2;break;case 17:c.host_return_valid=2;break;case 18:c.S=0;break;case 19:c.S=0x1ffd;break;case 20:ram[c.S+3]=1;break;case 21:host++;break;case 22:ram[c.S+1]^=1;break;case 23:ram[c.S+2]^=1;break;case 24:at=0x67800;break;case 25:at=0x678fd;break;case 26:at=0x67e00+(targets[w]&65535)-0xfe00;break;case 27:case 28:case 29:c.S=s=0x437+w+n-27;ram[s+1]=(uint8_t)(host-1);ram[s+2]=(uint8_t)((host-1)>>8);ram[s+3]=0x0c;break;}
 if(at){old=rom[at];rom[at]^=1;}memcpy(before,ram,sizeof ram);saved=c;RecompReturn result=RECOMP_RETURN_SKIP_2;CHECK(!LagoonNativeMusic_Try(r,size,&c,current,generation,frame,writer,s,h,host,&result));CHECK(result==RECOMP_RETURN_SKIP_2&&!memcmp(&saved,&c,sizeof c)&&!memcmp(before,ram,sizeof ram)&&!event_count);if(at)rom[at]=old;refusals++;
}}
static void fallback_policy(void){for(unsigned w=0;w<3;w++){
 g_cpu=setup(w,0,127,0);RecompReturn out=RECOMP_RETURN_NORMAL;unsigned before_calls=fallbacks;CpuState saved=g_cpu;memcpy(before,ram,sizeof ram);deadline=0;
 /* BeforeFrame generation deliberately unregistered: preserve real original entry. */
 bool(*fn[])(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*)={LagoonNativeMusic_Port0,LagoonNativeMusic_Port1,LagoonNativeMusic_Port2};
 CHECK(fn[w](&g_cpu,g_cpu.S,3,0x0c0000|calls[w][0]+4,&out));CHECK(fallbacks==before_calls+1&&boundary==targets[w]&&fallback_s==g_cpu.S&&fallback_h==3&&!memcmp(&saved,&g_cpu,sizeof saved)&&!memcmp(before,ram,sizeof ram)&&!event_count);
 }LagoonNativeMusic_BeforeFrame();for(unsigned w=0;w<3;w++){g_cpu=setup(w,0,0xffff,0xc3);RecompReturn out=RECOMP_RETURN_SKIP_2;deadline=0;bool(*fn[])(CpuState*,uint16_t,uint8_t,uint32_t,RecompReturn*)={LagoonNativeMusic_Port0,LagoonNativeMusic_Port1,LagoonNativeMusic_Port2};unsigned prior=takes[w];CHECK(fn[w](&g_cpu,g_cpu.S,3,0x0c0000|calls[w][0]+4,&out));CHECK(out==RECOMP_RETURN_NORMAL&&takes[w]==prior+1&&event_count==1);}}
int main(int argc,char**argv){CHECK(argc==2);FILE*f=fopen(argv[1],"rb");CHECK(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);g_rom=rom;
 const unsigned limits[]={0,1,21,22,23,43,44,45,59,60,61,79,80,81,107,108,109,137,138,139,169,170,171,213,214};
 for(unsigned w=0;w<3;w++)for(unsigned caller=0;caller<4;caller++)for(unsigned v=0;v<16;v++)for(unsigned flags=0;flags<8;flags++)for(unsigned phase=0;phase<sizeof limits/sizeof*limits;phase++)compare(w,caller,v*0x1111,((flags&1)?0x81:0)|((flags&2)?0x42:0)|((flags&4)?4:0),limits[phase]);
 for(unsigned w=0;w<3;w++){stack_override=0x43a+w;compare(w,0,0xa580,0xc3,0);stack_override=0;for(unsigned mutate=1;mutate<=3;mutate++){mutate_return=mutate;compare(w,0,0xa580,0xc3,0);}mutate_return=0;}
 negatives();fallback_policy();printf("PASS music writers %u original-opcode cases, %u read-only preflight refusals/%u original-entry fallback boundaries: all12 realJSLowners,8originalinstructions/physicalRTL, allflags/hiddenA/deadlines/partialSTA-RTL, fullRAM/IO/regs/stack/regionclocks; orderedAPUvalue+CPU/master-at-access exact (legacyopcode-start, NOT physicalbus-cycle claim)\n",cases,refusals-fallbacks,fallbacks);return 0;}
