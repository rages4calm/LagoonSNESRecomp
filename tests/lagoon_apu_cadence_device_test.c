#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu_state.h"
#include "snes/snes.h"
#include "snes/apu.h"
#include "snes/spc.h"
#include "snes/dsp.h"
#include "snes/cart.h"
#include "snes/interp_bridge.h"
#include "apu_frame_clock.h"
#include "audio_trace.h"
#include "dsp_shadow.h"

CpuState g_cpu;
Snes *g_snes;
int snes_frame_counter=1;
uint64_t g_apu_timer0_total_ticks;
static bool g_extended_frame_timing=true,g_apu_frame_time_valid=true;
static RtlApuFrameClock g_apu_frame_clock;
static uint64_t g_apu_frame_start_master,g_apu_last_sync_master;
static unsigned depth;
static uint64_t observed_clocks[65536];
static unsigned observed_count;
void RtlApuLock(void){assert(depth==0);depth++;}
void RtlApuUnlock(void){assert(depth==1);depth--;}
static bool rtl_apu_port_observers_filter(uint16 adr,uint8 val){(void)adr;(void)val;return false;}
static void rtl_accumulate_apu_catchup(void){assert(!"unmapped legacy path is outside this device fixture");}
void snes_catchupApu(Snes*snes){(void)snes;assert(!"unmapped legacy path is outside this device fixture");}
static void rtl_sync_apu_to_cpu_locked(void);
#include "apu_device_under_test.h"
DspShadow *dsp_shadow_create(void){return NULL;}
void dsp_shadow_free(DspShadow*s){(void)s;}
void dsp_shadow_process(DspShadow*s,Dsp*d,int l,int r,int*ol,int*orr){(void)s;(void)d;*ol=l;*orr=r;}
/* Unit-only beam-probe side channel records actual SPC cycle at each unmodified
 * trace callback; no production event field or logging implementation changes. */
static void probe(uint16_t*v,uint16_t*h){assert(observed_count<65536);observed_clocks[observed_count++]=g_snes->apu->portClock;*v=*h=0;}
static void path(char*out,size_t cap,const char*dir,const char*name){assert(snprintf(out,cap,"%s/%s",dir,name)>0);}
static void writefile(const char*dir,const char*name,const void*data,size_t size){char p[2048];path(p,sizeof(p),dir,name);FILE*f=fopen(p,"wb");assert(f);assert(fwrite(data,1,size,f)==size);assert(!fclose(f));}
typedef struct Memory {SaveLoadInfo sli;unsigned char data[200000];size_t pos;bool saving;} Memory;
static void memory_io(SaveLoadInfo*sli,void*data,size_t size){Memory*m=(Memory*)sli;assert(m->pos+size<sizeof(m->data));if(m->saving)memcpy(m->data+m->pos,data,size);else memcpy(data,m->data+m->pos,size);m->pos+=size;}
static Memory state;
static void setup(void){
 static Snes snes;g_snes=&snes;memset(&snes,0,sizeof(snes));snes.apu=apu_init();apu_reset(snes.apu);
 Apu*a=snes.apu;a->romReadable=false;a->spc->pc=0x200;a->cpuCyclesLeft=0;
 /* MOV A,$F4 / MOV $F5,A / BRA $0200: real SPC reads and echoes CPU data. */
 const uint8_t program[]={0xE4,0xF4,0xC4,0xF5,0x2F,0xFA};memcpy(a->ram+0x200,program,sizeof(program));
 /* Audible deterministic noise channel exercises actual nonzero PCM. */
 dsp_write(a->dsp,0x0C,0x7F);dsp_write(a->dsp,0x1C,0x7F);
 dsp_write(a->dsp,0x00,0x7F);dsp_write(a->dsp,0x01,0x7F);
 dsp_write(a->dsp,0x05,0);dsp_write(a->dsp,0x07,0x7F);
 dsp_write(a->dsp,0x3D,1);dsp_write(a->dsp,0x6C,0x3F);dsp_write(a->dsp,0x4C,1);
 rtl_apu_clock_begin(&g_apu_frame_clock,0);
 assert(apu_schedulePortWrite(a,3,0,0));assert(apu_runToGuestCycle(a,0,100));
 audio_trace_set_beam_probe(probe);
}
typedef struct Target {uint64_t cpu_master,guest_cycle,target;uint8_t port,value;uint8_t pad[6];} Target;
typedef struct DeviceEvent {uint64_t clock,sample;uint8_t type,port,value;uint8_t pad[5];} DeviceEvent;
static Target targets[256];static unsigned target_count;
static DeviceEvent events[65536],requests[256];
static AudioTracePortEvent trace_events[65536];
int main(int argc,char**argv){
 assert(argc==3);const char*mode=argv[1],*out=argv[2];setup();Apu*a=g_snes->apu;
 uint64_t start_produced,unused;audio_trace_sample_clocks(&start_produced,&unused);uint32_t start_write=a->dsp->sampleWrite;
 if(!strcmp(mode,"restore")){
  assert(apu_runToGuestCycle(a,256,10000));
  state.sli.func=memory_io;state.saving=true;state.pos=0;apu_saveload(a,&state.sli);
  size_t size=state.pos;uint32_t saved_write=a->dsp->sampleWrite;ApuPortSched schedule;apu_port_sched_save(a,&schedule);
  uint64_t trace_before;audio_trace_sample_clocks(&trace_before,&unused);
  assert(apu_runToGuestCycle(a,1024,10000));uint64_t trace_after;audio_trace_sample_clocks(&trace_after,&unused);assert(trace_after>trace_before);
  state.saving=false;state.pos=0;apu_saveload(a,&state.sli);apu_port_sched_restore(a,&schedule);
  uint64_t trace_restored;audio_trace_sample_clocks(&trace_restored,&unused);
  assert(state.pos==size&&a->dsp->sampleWrite==saved_write&&a->portClock==schedule.clock);
  assert(trace_restored==trace_after&&trace_restored>trace_before);
  printf("PASS actual apu_saveload+port rollback restore rewinds DSPwrite=%u and portClock=%" PRIu64 "; actual audio_trace produced persists=%" PRIu64 " (snapshot=%" PRIu64 ")\n",saved_write,a->portClock,trace_restored,trace_before);
  apu_free(a);return 0;
 }
 uint64_t cpu_request_backlog_max=0;unsigned writes=0;
 for(unsigned i=0;i<2400;i++){
  /* Identical completed-opcode costs in every mode; DMA-size spans included. */
  g_cpu.master_cycles+=6+(i*37%89)*2+(i%173==0?2048:0);
  if(!strcmp(mode,"reference")){
   RtlApuLock();rtl_sync_apu_to_cpu_locked();RtlApuUnlock();
  }else if(!strcmp(mode,"shared")||(!strcmp(mode,"partitioned")&&i%17<9))rtl_apu_pace_check();
  else if(!strcmp(mode,"partitioned")){
   /* Interpreted reference region: sync each completed opcode. */
   RtlApuLock();rtl_sync_apu_to_cpu_locked();RtlApuUnlock();
  }
  if(i%73==3){
   /* Mandatory read synchronization observes the real SPC echo after writes. */
   RtlApuLock();rtl_sync_apu_to_cpu_locked();audio_trace_on_cpu_port_read(1,a->outPorts[1]);RtlApuUnlock();
  }
  if(i%73==72){
   uint64_t guest=rtl_apu_guest_cycle();
   uint64_t due=a->portTargetAnchor+(guest-a->portGuestAnchor);
   uint64_t backlog=due>a->portClock?due-a->portClock:0;
   if(backlog>cpu_request_backlog_max)cpu_request_backlog_max=backlog;
   RtlApuWrite(0x2140,(uint8_t)(1+writes));
   ApuPortWrite*w=&a->portQueue[(a->portQTail-1)&(APU_PORT_QUEUE_LEN-1)];
   Target*t=&targets[target_count++];t->cpu_master=g_cpu.master_cycles;t->guest_cycle=guest;t->target=w->target_cycle;t->port=w->port;t->value=w->val;
   assert(w->target_cycle==due);writes++;
  }
 }
 /* Mandatory final synchronization is present in every mode. */
 RtlApuLock();rtl_sync_apu_to_cpu_locked();RtlApuUnlock();
 assert(a->portQHead==a->portQTail&&writes==32);
 uint64_t total=0,oldest=0;unsigned count=audio_trace_copy_port_events(0,65536,trace_events,&oldest,&total);
 /* One initial anchor APPLY predates our probe. */
 assert(oldest==0&&count==total&&observed_count+1==count);
 unsigned n=0,request_count=0;
 for(unsigned i=1;i<count;i++){
  AudioTracePortEvent*t=&trace_events[i];
  if(t->type==AUDIO_TRACE_EV_CPU_PORT_WRITE){
   DeviceEvent*e=&requests[request_count++];e->clock=observed_clocks[i-1];e->sample=t->sample_idx;e->type=t->type;e->port=t->port;e->value=t->val;continue;
  }
  DeviceEvent*e=&events[n++];e->clock=observed_clocks[i-1];e->sample=t->sample_idx;e->type=t->type;e->port=t->port;e->value=t->val;
 }
 assert(request_count==writes);writefile(out,"request-observations.bin",requests,request_count*sizeof(DeviceEvent));
 writefile(out,"targets.bin",targets,target_count*sizeof(Target));writefile(out,"device-events.bin",events,n*sizeof(DeviceEvent));
 state.sli.func=memory_io;state.saving=true;state.pos=0;apu_saveload(a,&state.sli);writefile(out,"guest.bin",state.data,state.pos);
 ApuPortSched schedule={0};apu_port_sched_save(a,&schedule);writefile(out,"port-state.bin",&schedule,sizeof(schedule));
 unsigned audible=0;for(unsigned i=0;i<a->dsp->sampleWrite*2;i++)audible+=a->dsp->sampleBuffer[i]!=0;assert(audible>0);
 uint64_t produced;audio_trace_sample_clocks(&produced,&unused);assert(produced-start_produced==a->dsp->sampleWrite-start_write);
 printf("PASS %s:32 actual native CPU writes, %u latch/SPC events, %u nonzero PCM values; actual SPCclock=%" PRIu64 "; max request cursor backlog=%" PRIu64 "; process traceproduced=%" PRIu64 "\n",mode,n,audible,a->portClock,cpu_request_backlog_max,produced);
 apu_free(a);return 0;
}
