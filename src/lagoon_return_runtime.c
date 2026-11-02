/* Source-owned production adapter. Original C7F6 normalization, door scan,
 * C8B5 payload reads, 018772 loader and architectural return all execute.
 * The only guest-state substitution is DP40..42 +Y after a complete guarded
 * request. PC, stack, room, items and story flags are never written here. */
#include "lagoon_return_runtime.h"
#include "lagoon_scene_renderer.h"
#include "lagoon_menu_presentation.h"
#include "lagoon_follower.h"
#include "common_rtl.h"
#include "cpu_state.h"
#include "snes/interp_bridge.h"
#include "snes/ppu.h"
#include "snes_runahead.h"
#include "netplay/snes_netplay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct {uint64_t tick,generation;unsigned initialized,queued,host_blocked;} runtime;
static unsigned word(const uint8_t*r,unsigned a){return r[a]|(unsigned)r[a+1]<<8;}
static void failure(const char *why,const CpuState*c,uint32_t pc){
 fprintf(stderr,"[lagoon-return] contract-refused reason=%s pc=%06x tick=%llu generation=%llu S=%04x Y=%04x\n",
  why,pc,(unsigned long long)runtime.tick,(unsigned long long)runtime.generation,c?c->S:0,c?c->Y:0);
 fflush(stderr);_Exit(87);
}
static void generation(void){
 uint64_t now=RtlStateGeneration();if(now==runtime.generation)return;
 if(runtime.queued)fprintf(stderr,"[lagoon-return] cancelled reason=timeline-change\n");
 runtime.generation=now;runtime.queued=0;LagoonReturn_Reset(now);
}
static int host_blocked(void){return runtime.host_blocked||snes_runahead_frames()||snes_netplay_active();}
static LagoonReturnView base_view(void){
 return (LagoonReturnView){g_ram,sizeof g_ram,runtime.generation,runtime.tick,0,(uint8_t)host_blocked(),0};
}
static int scene(void){
 const Ppu*p=g_ppu;
 if(!p||g_cpu.ram!=g_ram||g_ram[0x48f]||g_ram[0x48e]>=75||g_ram[0x100e]||
  p->inidisp!=15||p->bgmode!=9||p->mosaic||p->obsel!=2||p->setini||
  p->bgTileAdr!=0x600||p->bgXsc[0]!=0x71||p->bgXsc[2]!=0x3c||
  p->cgadsub||(p->cgwsel&1)||p->windowsel||p->screenWindowed[0]||p->screenWindowed[1]||
  p->screenEnabled[0]!=31||p->hScroll[2]||p->vScroll[2]||!LagoonSceneRenderer_ExactRoom(g_ram))return 0;
 if(p->bgXsc[1]!=(LagoonSceneRenderer_ParallaxRoom(g_ram)?0x7a:0x79))return 0;
 return LagoonMenuPresentation_Identify(p,g_ram)==LAGOON_MENU_NONE;
}
static LagoonReturnView full_view(void){
 LagoonReturnView v=base_view();v.scene_qualified=(uint8_t)scene();
 if(v.scene_qualified)for(unsigned q=0x40;q<=0x800;q+=0x40)
  if(LagoonFollower_Role(g_ram,q)){v.escort_active=1;break;}
 return v;
}
static LagoonReturnCpu cpu_view(const CpuState*c,uint32_t pc){
 return (LagoonReturnCpu){pc,c->S,c->Y,c->D,c->PB,c->DB,
  c->m_flag,c->x_flag,c->emulation,c->P,c->_flag_D};
}
static void opcode(CpuState*c,uint32_t pc){
 if(!runtime.initialized||!c||c!=&g_cpu||c->ram!=g_ram)return;
 if(!interp_bridge_source_opcode_available(c)){
  if(runtime.queued&&LagoonReturn_InFlight())failure("source-callback-lost",c,pc);
  return;
 }
 pc&=0x7fffff;generation();
 /* Normal play observes only an actual matched ordinary door. No expensive
  * PPU/actor checks run at the other three sites without a queued request. */
 if(pc==0x01c8b5){
  LagoonReturnCpu call=cpu_view(c,pc);
  if(runtime.queued){
   LagoonReturnView v=base_view();LagoonReturnPlan plan;
   if(v.host_blocked&&LagoonReturn_InFlight())failure("host-mode-changed",c,pc);
   int consumed=LagoonReturn_Consume(&v,&call,&plan);
   if(consumed<0)failure("incomplete-door-contract",c,pc);
   if(consumed){
    /* Original source next reads this exact original ROM payload. Its loader
     * writes room/coordinates/actors itself; no architectural frame is made. */
    g_ram[0x40]=(uint8_t)plan.target_payload;
    g_ram[0x41]=(uint8_t)(plan.target_payload>>8);
    g_ram[0x42]=(uint8_t)(plan.target_payload>>16);
    c->Y=0;runtime.queued=0;
    fprintf(stderr,"[lagoon-return] applied source=%02x target=%02x source_payload=%06x target_payload=%06x tick=%llu S=%04x inside-entrance\n",
     plan.source_room,plan.target_room,plan.source_payload,plan.target_payload,(unsigned long long)runtime.tick,c->S);
    return;
   }
  }
  LagoonReturnView v=full_view();
  if(v.scene_qualified&&LagoonReturn_WitnessDoor(&v,&call))
   fprintf(stderr,"[lagoon-return] witnessed room=%02x tick=%llu S=%04x Y=%04x real-main-call\n",
    word(g_ram,0x48e),(unsigned long long)runtime.tick,c->S,c->Y);
  return;
 }
 if(!runtime.queued)return;
 if(pc==0x01c88d){
  if(LagoonReturn_InFlight()&&!c->m_flag&&c->A==0xffff)failure("virtual-door-not-found",c,pc);
  return;
 }
 if(pc==0x01c7f6){
  if(LagoonReturn_InFlight())failure("virtual-door-reentry",c,pc);
  LagoonReturnCpu call=cpu_view(c,pc);LagoonReturnView v=full_view();
  if(LagoonReturn_Begin(&v,&call))
   fprintf(stderr,"[lagoon-return] begun tick=%llu S=%04x original-C7F6\n",(unsigned long long)runtime.tick,c->S);
  else {LagoonReturnStatus status;LagoonReturn_Status(&v,&status);runtime.queued=0;LagoonReturn_Cancel();
   fprintf(stderr,"[lagoon-return] cancelled reason=entry-contract state_reason=%u %s S=%04x P=%02x DB=%02x D=%04x\n",
    status.reason,status.message,c->S,c->P,c->DB,c->D);}
  return;
 }
 if(pc==0x0180a3){
  LagoonReturnCpu call=cpu_view(c,pc);LagoonReturnView v=base_view();
  if(LagoonReturn_End(&v,&call)<0)failure("unconsumed-virtual-door",c,pc);
  runtime.queued=0;LagoonReturn_Cancel();
  fprintf(stderr,"[lagoon-return] cancelled reason=no-virtual-door-activation\n");
 }
}
void LagoonReturnRuntime_OnRom(const uint8_t*rom,size_t size){
 memset(&runtime,0,sizeof runtime);LagoonReturn_Init(rom,size);
 runtime.generation=RtlStateGeneration();LagoonReturn_Reset(runtime.generation);runtime.initialized=1;
 static const uint32_t sites[]={0x01c7f6,0x01c8b5,0x0180a3,0x01c88d};
 for(unsigned i=0;i<sizeof sites/sizeof *sites;i++)if(!interp_bridge_add_pre_opcode_hook(sites[i],opcode))
  failure("hook-capacity",NULL,sites[i]);
}
void LagoonReturnRuntime_BeforeFrame(uint64_t tick,int blocked){
 if(!runtime.initialized)return;generation();runtime.tick=tick;runtime.host_blocked=blocked!=0;
}
void LagoonReturnRuntime_AfterFrame(void){
 if(!runtime.initialized)return;generation();
 int activated=runtime.queued&&LagoonReturn_InFlight();
 LagoonReturnView v=full_view();LagoonReturn_Observe(&v);
 if(activated&&!LagoonReturn_InFlight())failure("activated-history-lost",&g_cpu,0);
 if(runtime.queued){LagoonReturnStatus status;LagoonReturn_Status(&v,&status);
  /* NMI/deadline delivery can split the original short sequence between
   * fields. Once the attribute view activated, keep its remaining reads armed;
   * clearing here because the ORIGINAL door latch became1 would strand C88B. */
  if(LagoonReturn_InFlight()){
   if(v.host_blocked)failure("host-mode-changed-in-flight",&g_cpu,0);
  }else if(status.reason!=LAGOON_RETURN_PENDING){runtime.queued=0;LagoonReturn_Cancel();}}
}
uint8_t LagoonReturnRuntime_FilterRead(uint32_t a,uint8_t value,const CpuWramReadContext*c,const uint8_t*ram){
 /* This is on every guest WRAM read. Default-off fast path does not query
  * generations, actors, scenes, PPU state or any device. */
 if(!runtime.queued||!c||ram!=g_ram||!c->interpreter||c->pb!=1||
  !((c->instruction_pc==0x01c81b&&a==0x2c1)||
    (c->instruction_pc==0x01c892&&a>=0x102b&&a<=0x102c)||
    (c->instruction_pc==0x01c89b&&a>=0x102d&&a<=0x102e)))return value;
 generation();if(!runtime.queued)return value;
 LagoonReturnRead read={c->instruction_pc,c->stack,c->pb,c->m,c->x,c->e};
 /* Begin already authenticated scene/escort/source. The five exact data reads
  * need only the current timeline and original RAM; do not rehash PPU tiles. */
 LagoonReturnView v={g_ram,sizeof g_ram,runtime.generation,runtime.tick,0,0,0};
 return LagoonReturn_FilterRead(&v,&read,a,value);
}
void LagoonReturnRuntime_GetStatus(LagoonReturnStatus*out){
 if(!runtime.initialized){LagoonReturn_Status(NULL,out);return;}
 generation();LagoonReturnView v=full_view();LagoonReturn_Status(&v,out);
}
int LagoonReturnRuntime_Request(LagoonReturnStatus*out){
 if(!runtime.initialized){LagoonReturn_Status(NULL,out);return 0;}
 generation();LagoonReturnView v=full_view();LagoonReturnStatus status;
 int accepted=LagoonReturn_Request(&v,&status);if(out)*out=status;
 if(accepted){runtime.queued=1;fprintf(stderr,"[lagoon-return] requested room=%02x entrance=%02x tick=%llu generation=%llu\n",
  word(g_ram,0x48e),status.entrance_room,(unsigned long long)runtime.tick,(unsigned long long)runtime.generation);}
 else fprintf(stderr,"[lagoon-return] request-refused reason=%u %s\n",status.reason,status.message);
 return accepted;
}
