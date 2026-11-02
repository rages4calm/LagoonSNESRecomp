/* Adapter contract tests with the real source ROM/model and deliberately
 * stubbed host scene/hook services. This does NOT execute the native loader. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lagoon_return_runtime.h"
#include "lagoon_menu_presentation.h"
#include "common_rtl.h"
#include "cpu_state.h"
#include "snes/ppu.h"
#include "snes/interp_bridge.h"

uint8 g_ram[0x20000];
CpuState g_cpu;
static Ppu test_ppu;
Ppu *g_ppu=&test_ppu;
static uint8_t rom[0x100000],saved_ram[sizeof g_ram];
static uint64_t epoch=100,tick;
static unsigned generation_queries,scene_queries,role_queries,hook_count;
static int exact_scene=1,escort,menu,ahead,network,fail_hook,source_available=1;
static struct {uint32_t pc;InterpPreOpcodeHook fn;} hooks[4];
static const uint16_t stack_pointer=0x1fe0;
typedef struct TestDoor {unsigned table,offset,payload,sx,sy,x,y,to;} TestDoor;
static TestDoor incoming,back;

uint64_t RtlStateGeneration(void){generation_queries++;return epoch;}
int snes_runahead_frames(void){return ahead;}
int snes_netplay_active(void){return network;}
bool interp_bridge_source_opcode_available(const CpuState *c){return c==&g_cpu&&source_available;}
int LagoonSceneRenderer_ExactRoom(const uint8_t *r){assert(r==g_ram);scene_queries++;return exact_scene;}
int LagoonSceneRenderer_ParallaxRoom(const uint8_t *r){assert(r==g_ram);return 0;}
int LagoonFollower_Role(const uint8_t *r,unsigned q){assert(r==g_ram);role_queries++;return escort&&q==0x40;}
LagoonMenuPanel LagoonMenuPresentation_Identify(const Ppu *p,const uint8_t *r){
 assert(p==g_ppu&&r==g_ram);return (LagoonMenuPanel)menu;
}
bool interp_bridge_add_pre_opcode_hook(uint32_t pc,InterpPreOpcodeHook fn){
 if(fail_hook)return false;
 for(unsigned i=0;i<hook_count;i++)if(hooks[i].pc==pc&&hooks[i].fn==fn)return true;
 assert(hook_count<4);hooks[hook_count].pc=pc;hooks[hook_count++].fn=fn;return true;
}
static void run_hook_on(CpuState *c,uint32_t pc){
 unsigned normalized=pc&0x7fffff;
 for(unsigned i=0;i<hook_count;i++)if(hooks[i].pc==normalized){hooks[i].fn(c,pc);return;}
 assert(!"unregistered pre-opcode hook");
}
static void run_hook(uint32_t pc){run_hook_on(&g_cpu,pc);}
static const uint8_t *at(unsigned address){return rom+((address>>16)&127)*32768+(address&32767);}
static unsigned rd16(const uint8_t *r,unsigned a){return r[a]|(unsigned)r[a+1]<<8;}
static void put(unsigned a,unsigned n){g_ram[a]=(uint8_t)n;g_ram[a+1]=(uint8_t)(n>>8);}
static void ptr(unsigned a,unsigned n){put(a,n);g_ram[a+2]=(uint8_t)(n>>16);}
static TestDoor door(unsigned from,unsigned to){
 unsigned table=0x50000|rd16(at(0x058059+from*2),0);
 for(unsigned n=0;n<64;n++){
  const uint8_t *p=at(table+n*11);if(rd16(p,0)==65535)break;
  if(p[8]==to)return (TestDoor){table,n*11+4,table+n*11+4,
   rd16(p,0),rd16(p,2),rd16(p,4),rd16(p,6),p[8]};
 }
 assert(!"missing actual ROM door");return (TestDoor){0};
}
static void cpu_mode(unsigned m){
 g_cpu.S=stack_pointer;g_cpu.Y=0;g_cpu.D=0;g_cpu.PB=g_cpu.DB=1;
 g_cpu.m_flag=(uint8_t)m;g_cpu.x_flag=g_cpu.emulation=g_cpu._flag_D=0;g_cpu.P=(uint8_t)(m?0x20:0);
}
static void set_world(unsigned room,unsigned x,unsigned y){
 memset(g_ram,0,sizeof g_ram);const uint8_t *descriptor=at(0x05b2df+room*56);
 put(0x48e,room);put(0x53,rd16(descriptor,3));put(0x55,rd16(descriptor,5));
 put(0x502,x);put(0x504,y);put(0x520,18);g_ram[0x500]=0xc0;put(stack_pointer+1,0x80a2);
}
static void field(void){LagoonReturnRuntime_BeforeFrame(++tick,0);LagoonReturnRuntime_AfterFrame();}
static void enter(TestDoor d){
 put(0x502,d.sx*32+16);put(0x504,d.sy*32+1);field();
 ptr(0x40,d.table);put(0x102b,d.sx);put(0x102d,d.sy);cpu_mode(0);g_cpu.Y=(uint16_t)d.offset;
 run_hook(0x01c8b5);set_world(d.to,d.x,d.y);field();
}
static void setup(void){
 epoch++;tick=0;exact_scene=source_available=1;escort=menu=ahead=network=fail_hook=0;
 memset(&g_cpu,0,sizeof g_cpu);g_cpu.ram=g_ram;cpu_mode(1);
 memset(g_ppu,0,sizeof *g_ppu);g_ppu->inidisp=15;g_ppu->bgmode=9;g_ppu->obsel=2;
 g_ppu->bgTileAdr=0x600;g_ppu->bgXsc[0]=0x71;g_ppu->bgXsc[1]=0x79;g_ppu->bgXsc[2]=0x3c;
 g_ppu->screenEnabled[0]=31;
 LagoonReturnRuntime_OnRom(rom,sizeof rom);assert(hook_count==4);
 set_world(2,128,128);field();incoming=door(2,3);enter(incoming);enter(door(3,4));back=door(4,3);
 put(0x502,384);put(0x504,128);g_ram[0x4e0]=0x80;
 field();field();field();LagoonReturnStatus status;LagoonReturnRuntime_GetStatus(&status);
 if(status.reason!=LAGOON_RETURN_READY)fprintf(stderr,"setup refused:%u %s\n",status.reason,status.message);
 assert(status.reason==LAGOON_RETURN_READY&&status.entrance_room==3);
}
static uint8_t filtered(uint32_t pc,unsigned m,unsigned a,uint8_t value){
 CpuWramReadContext context={pc,stack_pointer,0,1,(uint8_t)m,0,0,1};
 return LagoonReturnRuntime_FilterRead(a,value,&context,g_ram);
}
static void request_and_begin(void){
 LagoonReturnStatus status;assert(LagoonReturnRuntime_Request(&status));cpu_mode(1);
 LagoonReturnRuntime_BeforeFrame(++tick,0);run_hook(0x01c7f6);
}
static void activate(unsigned complete){
 request_and_begin();assert(filtered(0x01c81b,1,0x2c1,0x20)==0x21);
 assert(LagoonReturn_InFlight());
 /* The ORIGINAL opcode sequence performs these writes in the live runtime.
  * The fixture reproduces that input; this is not adapter-created state. */
 g_ram[0x51b]=1;put(0x102b,rd16(g_ram,0x502)>>5);put(0x102d,(rd16(g_ram,0x504)-1)>>5);
 if(complete){
  assert(filtered(0x01c892,0,0x102b,g_ram[0x102b])==(uint8_t)back.sx);
  assert(filtered(0x01c892,0,0x102c,g_ram[0x102c])==(uint8_t)(back.sx>>8));
  assert(filtered(0x01c89b,0,0x102d,g_ram[0x102d])==(uint8_t)back.sy);
  assert(filtered(0x01c89b,0,0x102e,g_ram[0x102e])==(uint8_t)(back.sy>>8));
 }
 ptr(0x40,back.table);cpu_mode(0);g_cpu.Y=(uint16_t)back.offset;
}
static void require_ready(void){LagoonReturnStatus status;LagoonReturnRuntime_GetStatus(&status);assert(status.reason==LAGOON_RETURN_READY);}
static void refusal_tests(void){
 LagoonReturnStatus status;setup();
 /* Live host scene qualification, not stale RAM menu bytes. */
 menu=LAGOON_MENU_ITEMS;assert(!LagoonReturnRuntime_Request(&status)&&status.reason==LAGOON_RETURN_WORLD);menu=0;
 exact_scene=0;assert(!LagoonReturnRuntime_Request(&status)&&status.reason==LAGOON_RETURN_WORLD);exact_scene=1;
 g_ppu->inidisp=0x80;assert(!LagoonReturnRuntime_Request(&status));g_ppu->inidisp=15;
 escort=1;assert(!LagoonReturnRuntime_Request(&status)&&status.reason==LAGOON_RETURN_ESCORT);escort=0;
 ahead=1;assert(!LagoonReturnRuntime_Request(&status)&&status.reason==LAGOON_RETURN_HOST);ahead=0;
 network=1;assert(!LagoonReturnRuntime_Request(&status)&&status.reason==LAGOON_RETURN_HOST);network=0;
 LagoonReturnRuntime_BeforeFrame(tick,1);assert(!LagoonReturnRuntime_Request(&status)&&status.reason==LAGOON_RETURN_HOST);
 LagoonReturnRuntime_BeforeFrame(tick,0);require_ready();
 /* CPU/stack/mode mismatches cancel the queued request without guest writes. */
 for(unsigned which=0;which<5;which++){
  setup();assert(LagoonReturnRuntime_Request(&status));cpu_mode(1);
  switch(which){case 0:g_cpu._flag_D=1;break;case 1:g_cpu.P|=8;break;case 2:g_cpu.DB=2;break;
   case 3:g_cpu.x_flag=1;break;case 4:put(stack_pointer+1,0x9999);break;}
  memcpy(saved_ram,g_ram,sizeof g_ram);run_hook(0x01c7f6);assert(!LagoonReturn_InFlight());
  assert(filtered(0x01c81b,1,0x2c1,0)==0&&!memcmp(saved_ram,g_ram,sizeof g_ram));
 }
 setup();request_and_begin(); /* Wrong CPU object cannot consume or activate. */
 CpuState other=g_cpu;run_hook_on(&other,0x01c8b5);assert(!LagoonReturn_InFlight());
 assert(filtered(0x01c81b,1,0x2c1,0)==1&&LagoonReturn_InFlight());
 epoch++;LagoonReturnRuntime_BeforeFrame(++tick,0);
 assert(filtered(0x01c892,0,0x102b,0x9a)==0x9a&&!LagoonReturn_InFlight());
 LagoonReturnRuntime_AfterFrame();LagoonReturnRuntime_GetStatus(&status);assert(status.reason==LAGOON_RETURN_MOVING);
 field();field();require_ready(); /* Cleared save retains canonical return after history reset. */
 setup();assert(LagoonReturnRuntime_Request(&status));cpu_mode(1);source_available=0;
 run_hook(0x01c7f6);assert(filtered(0x01c81b,1,0x2c1,0)==0&&!LagoonReturn_InFlight());
 source_available=1;run_hook(0x01c7f6);assert(filtered(0x01c81b,1,0x2c1,0)==1&&LagoonReturn_InFlight());
 epoch++;LagoonReturnRuntime_BeforeFrame(++tick,0);
}
static void successful_consume_test(int loaded){
 setup();
 if(loaded){epoch++;field();field();field();require_ready();}
 activate(0);
 /* The original door latch is expected while active; AfterFrame must not
  * disarm the virtual comparisons if NMI split the instruction sequence. */
 LagoonReturnRuntime_AfterFrame();assert(LagoonReturn_InFlight());
 assert(filtered(0x01c892,0,0x102b,g_ram[0x102b])==(uint8_t)back.sx);
 assert(filtered(0x01c892,0,0x102c,g_ram[0x102c])==(uint8_t)(back.sx>>8));
 assert(filtered(0x01c89b,0,0x102d,g_ram[0x102d])==(uint8_t)back.sy);
 assert(filtered(0x01c89b,0,0x102e,g_ram[0x102e])==(uint8_t)(back.sy>>8));
 g_cpu.A=0x1234;g_cpu.X=0x5678;g_cpu.host_return_valid=2;
 CpuState original=g_cpu;memcpy(saved_ram,g_ram,sizeof g_ram);run_hook(0x81c8b5);
 assert(g_cpu.Y==0);g_cpu.Y=original.Y;assert(!memcmp(&original,&g_cpu,sizeof g_cpu));g_cpu.Y=0;
 assert(rd16(g_ram,0x40)==(incoming.payload&65535)&&g_ram[0x42]==(incoming.payload>>16));
 for(unsigned a=0;a<sizeof g_ram;a++)if(a<0x40||a>0x42)assert(g_ram[a]==saved_ram[a]);
 assert(rd16(g_ram,0x48e)==4&&!LagoonReturn_InFlight());
 /* Consume is one-shot; other instructions use untouched genuine data. */
 assert(filtered(0x01c892,0,0x102b,0x7d)==0x7d);
 set_world(3,incoming.x,incoming.y);g_ram[0x4e0]=0x80;field();field();field();
 LagoonReturnStatus status;LagoonReturnRuntime_GetStatus(&status);assert(status.reason==LAGOON_RETURN_ALREADY_THERE);
}
static void fast_path_tests(void){
 setup();unsigned g=generation_queries,p=scene_queries,r=role_queries;
 for(unsigned n=0;n<100000;n++)assert(filtered(0x01c81b,1,0x2c1,0x70)==0x70);
 assert(g==generation_queries&&p==scene_queries&&r==role_queries);
 LagoonReturnStatus status;assert(LagoonReturnRuntime_Request(&status));
 g=generation_queries;p=scene_queries;r=role_queries;
 for(unsigned n=0;n<100000;n++)assert(filtered(0x019999,1,0x2c1,0x70)==0x70);
 assert(g==generation_queries&&p==scene_queries&&r==role_queries);
 CpuWramReadContext bad={0x01c81b,stack_pointer,0,1,1,0,0,0};
 assert(LagoonReturnRuntime_FilterRead(0x2c1,0x70,&bad,g_ram)==0x70);
 bad.interpreter=1;bad.pb=2;assert(LagoonReturnRuntime_FilterRead(0x2c1,0x70,&bad,g_ram)==0x70);
 bad.pb=1;assert(LagoonReturnRuntime_FilterRead(0x2c1,0x70,&bad,saved_ram)==0x70);
 assert(g==generation_queries&&p==scene_queries&&r==role_queries);
}
int main(int argc,char **argv){
 assert(argc>=2);FILE *f=fopen(argv[1],"rb");assert(f);assert(fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 if(argc==3){
  if(!strcmp(argv[2],"hook-capacity")){fail_hook=1;LagoonReturnRuntime_OnRom(rom,sizeof rom);}
  else if(!strcmp(argv[2],"terminator")){setup();activate(0);g_cpu.A=0xffff;run_hook(0x01c88d);}
  else if(!strcmp(argv[2],"incomplete")){setup();activate(0);run_hook(0x01c8b5);}
  else if(!strcmp(argv[2],"resume")){setup();activate(0);cpu_mode(1);g_cpu.S+=2;run_hook(0x0180a3);}
  else if(!strcmp(argv[2],"host-change")){setup();activate(0);LagoonReturnRuntime_BeforeFrame(tick,1);LagoonReturnRuntime_AfterFrame();}
  else if(!strcmp(argv[2],"reentry")){setup();activate(0);cpu_mode(1);run_hook(0x01c7f6);}
  else if(!strcmp(argv[2],"source-lost")){setup();activate(0);source_available=0;run_hook(0x01c8b5);}
  else if(!strcmp(argv[2],"timeout")){setup();activate(0);tick+=121;LagoonReturnRuntime_BeforeFrame(tick,0);LagoonReturnRuntime_AfterFrame();}
  assert(!"fatal contract did not stop");
 }
 fast_path_tests();refusal_tests();successful_consume_test(0);successful_consume_test(1);
 puts("PASS runtime ADAPTER ONLY: four real hooks, P/decimal/scene/escort/host/generation guards, 200000 no-work read fast paths, split-field latch, natural-entry and cleared-loaded one-shot DP40..42/Y-only substitution. Native loader execution still requires integration qualification.");
 return 0;
}
