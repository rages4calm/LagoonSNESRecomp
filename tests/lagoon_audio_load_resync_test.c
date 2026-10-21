/* Existing real APU/SPC/DSP proof; no production load policy is changed. */
#include "snes/apu.h"
#include "snes/dsp_shadow.h"
#include "audio_trace.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

uint64_t g_apu_timer0_total_ticks;
int snes_frame_counter;
DspShadow *dsp_shadow_create(void) { return NULL; }
void dsp_shadow_free(DspShadow *p) { (void)p; }
void dsp_shadow_process(DspShadow *p,Dsp *d,int l,int r,int *ol,int *or_) {
  (void)p;(void)d;*ol=l;*or_=r;
}
enum { MAX_PCM=8192, MAX_EVENTS=65536, BLOB_CAP=120000 };
typedef struct Event {uint8_t kind,port,value;uint64_t clock;} Event;
typedef struct Capture {
  int16_t pcm[MAX_PCM*2]; unsigned samples,drops,nonzero,event_count;
  Event events[MAX_EVENTS];
} Capture;
static Capture *capture;
static Apu *observed_apu;
static void event(unsigned kind,unsigned port,unsigned value) {
  if (!capture) return;
  assert(capture->event_count<MAX_EVENTS);
  Event *e=&capture->events[capture->event_count++];
  e->kind=(uint8_t)kind;e->port=(uint8_t)port;e->value=(uint8_t)value;
  e->clock=observed_apu->portClock;
}
void audio_trace_on_sample(int16_t l,int16_t r,int dropped,uint32_t fill) {
  (void)fill;if(!capture)return;assert(capture->samples<MAX_PCM);
  capture->pcm[capture->samples*2]=l;capture->pcm[capture->samples*2+1]=r;
  capture->samples++;capture->drops+=(unsigned)dropped;
  if(l||r)capture->nonzero++;
}
void audio_trace_on_reg_write(uint8_t a,uint8_t v) {event(4,a,v);}
void audio_trace_on_consume(uint64_t r,uint32_t n,uint32_t a) {(void)r;(void)n;(void)a;}
void audio_trace_on_cpu_port_apply(uint8_t p,uint8_t v) {event(1,p,v);}
void audio_trace_on_spc_port_read(uint8_t p,uint8_t v) {event(2,p,v);}
void audio_trace_on_spc_port_write(uint8_t p,uint8_t v) {event(3,p,v);}

typedef struct Blob {SaveLoadInfo base;uint8_t bytes[BLOB_CAP];size_t used,pos;bool load;} Blob;
static void bytes(SaveLoadInfo *s,void *data,size_t n) {
  Blob *b=(Blob*)s;
  if(b->load){assert(b->pos+n<=b->used);memcpy(data,b->bytes+b->pos,n);b->pos+=n;}
  else{assert(b->used+n<=BLOB_CAP);memcpy(b->bytes+b->used,data,n);b->used+=n;}
}
static void save(Apu *a,Blob *b){memset(b,0,sizeof *b);b->base.func=bytes;apu_saveload(a,&b->base);}
static void load(Apu *a,Blob *b){b->pos=0;b->load=true;apu_saveload(a,&b->base);assert(b->pos==b->used);b->load=false;}

static size_t ring_begin(void) {
  return offsetof(Apu,pad)+6u-offsetof(Apu,ram)+offsetof(Dsp,sampleBuffer)-offsetof(Dsp,ram);
}
static size_t ring_end(void) {
  return offsetof(Apu,pad)+6u-offsetof(Apu,ram)+sizeof(Dsp)-offsetof(Dsp,ram);
}
static void same_synthesis(const Blob *a,const Blob *b) {
  assert(a->used==b->used);assert(ring_end()<a->used);
  assert(!memcmp(a->bytes,b->bytes,ring_begin()));
  assert(!memcmp(a->bytes+ring_end(),b->bytes+ring_end(),a->used-ring_end()));
}
static void init_fixture(Apu *a) {
  apu_reset(a);
  const uint8_t code[]={0xe4,0xf4,0xc4,0xf5,0xbc,0xc4,0xf6,0x2f,0xf7};
  memcpy(a->ram+0x200,code,sizeof code);a->spc->pc=0x200;
  a->romReadable=false;a->cpuCyclesLeft=0;
  dsp_write(a->dsp,0x00,127);dsp_write(a->dsp,0x01,127);
  dsp_write(a->dsp,0x0c,127);dsp_write(a->dsp,0x1c,127);
  dsp_write(a->dsp,0x05,0);dsp_write(a->dsp,0x07,127);
  dsp_write(a->dsp,0x3d,1);dsp_write(a->dsp,0x6c,0x3f);
  dsp_write(a->dsp,0x4c,1);
  for(unsigned i=0;i<262144;i++)apu_cycle(a);
  assert(dsp_available(a->dsp)==DSP_SAMPLE_RING);
}
static unsigned check_trim_api(void) {
  const uint32_t fills[]={0,1,2135,2136,2137,4096,8192};
  const uint32_t reads[]={0,12345,UINT32_MAX-4095};
  const uint32_t keeps[]={0,2136,8192,UINT32_MAX};
  unsigned cases=0;
  for(unsigned f=0;f<sizeof fills/sizeof *fills;f++)
  for(unsigned r=0;r<sizeof reads/sizeof *reads;r++)
  for(unsigned k=0;k<sizeof keeps/sizeof *keeps;k++) {
    Dsp a;memset(&a,0xa5,sizeof a);
    for(unsigned i=0;i<DSP_SAMPLE_RING*2;i++)a.sampleBuffer[i]=(int16_t)(i*13u+7u);
    a.sampleRead=reads[r];a.sampleWrite=reads[r]+fills[f];
    Dsp before=a,expected=a;uint32_t keep=keeps[k]<fills[f]?keeps[k]:fills[f];
    uint32_t n=dsp_trimSamples(&a,keeps[k]);assert(n==fills[f]-keep);
    expected.sampleRead+=n;assert(!memcmp(&a,&expected,sizeof a));
    assert(dsp_available(&a)==keep);
    for(uint32_t i=0;i<keep;i++) {
      int16_t l,r0,ol,orr;dsp_peek(&a,i,&l,&r0);dsp_peek(&before,n+i,&ol,&orr);
      assert(l==ol&&r0==orr);
    }
    cases++;
  }
  return cases;
}
/* Compile the current production helper and desktop reset/load code. */
static bool rollback_load,absolute_enabled=true,load_success,restored_absolute_enabled=true;
static uint64_t guest_generation=1,g_state_generation=1,g_audio_timeline_generation=1;
static unsigned rewind_shutdowns,rewind_configures;
static unsigned lock_depth,clear_calls,game_resets,osd_calls;
static Snes host_snes;
Snes *g_snes=&host_snes;
static double s_render_phase,s_render_occ_ema;
static bool s_render_priming;
static int s_render_starved,s_render_fade_pos,s_render_fade_out;
static unsigned char scratch[32],*g_audiobuffer_cur,*g_audiobuffer_end;
static bool g_audio_primed;
static void *g_audio_stream;
static bool record_order;
static char call_order[128];static unsigned call_count;
static void note(char c){if(record_order){assert(call_count+1<sizeof call_order);call_order[call_count++]=c;call_order[call_count]=0;}}
void RtlApuLock(void){assert(lock_depth==0);lock_depth++;note('L');}
void RtlApuUnlock(void){assert(lock_depth==1);lock_depth--;note('U');}
uint64_t RtlStateGeneration(void){return guest_generation;}
static void restore_execution_stub(void){absolute_enabled=restored_absolute_enabled;}
static void snes_rewind_shutdown(void){rewind_shutdowns++;note('W');}
static void snes_rewind_configure(void){rewind_configures++;note('R');}
bool RtlIsRollbackLoad(void){return rollback_load;}
bool rtl_apu_absolute_periodic_enabled(void){return absolute_enabled;}
static void SDL_ClearAudioStream(void *stream){assert(stream);assert(!lock_depth);clear_calls++;note('C');}
static void GameReset(void){assert(!lock_depth);game_resets++;note('G');}
static void snes_osd_push(const char *text,int ms){assert(text&&ms>0);osd_calls++;note('O');}
static void actual_file_gate(bool error);
static bool RtlTrySaveLoad(int cmd,int slot){assert(cmd==0&&slot==8);if(load_success){actual_file_gate(false);guest_generation++;}return load_success;}
#define SNESRECOMP_SDL3 1
#define kSaveLoad_Load 0
#include "audio_load_under_test.h"
static void delivery_sentinel(void){s_render_phase=.5;s_render_occ_ema=123;s_render_priming=false;s_render_starved=7;s_render_fade_pos=9;s_render_fade_out=10;}
static void assert_delivery_reset(void){assert(s_render_phase==0&&s_render_occ_ema==-1&&s_render_priming&&s_render_starved==1&&s_render_fade_pos==0&&s_render_fade_out==RTL_AUDIO_FADE_FRAMES);}
static unsigned check_actual_helper_and_load_gates(void) {
  const uint32_t fills[]={0,1,2135,2136,2137,4096,8192};unsigned cases=0;
  for(unsigned pointers=0;pointers<4;pointers++)for(unsigned flags=0;flags<4;flags++)for(unsigned f=0;f<7;f++) {
    Apu a={0};Dsp d;memset(&d,0x5a,sizeof d);d.sampleRead=UINT32_MAX-1023;d.sampleWrite=d.sampleRead+fills[f];Dsp expected=d;
    a.dsp=pointers>=3?&d:NULL;host_snes.apu=pointers>=2?&a:NULL;g_snes=pointers?&host_snes:NULL;
    rollback_load=(flags&1)!=0;absolute_enabled=(flags&2)!=0;
    bool authorized=pointers==3&&!rollback_load&&absolute_enabled;
    if(authorized&&fills[f]>2136)expected.sampleRead+=fills[f]-2136;
    delivery_sentinel();RtlApuLock();rtl_reset_loaded_audio();RtlApuUnlock();
    assert(!memcmp(&d,&expected,sizeof d));if(authorized)assert_delivery_reset();else assert(s_render_phase==.5&&s_render_occ_ema==123);cases++;
  }
  Apu a={0};Dsp d={0};a.dsp=&d;host_snes.apu=&a;g_snes=&host_snes;
  for(unsigned flags=0;flags<8;flags++)for(unsigned f=0;f<7;f++)for(unsigned file=0;file<2;file++) {
    bool error=(flags&1)!=0;rollback_load=(flags&2)!=0;absolute_enabled=(flags&4)!=0;restored_absolute_enabled=absolute_enabled;
    d.sampleRead=123;d.sampleWrite=123+fills[f];Dsp expected=d;
    if(!error&&!rollback_load&&absolute_enabled&&fills[f]>2136)expected.sampleRead+=fills[f]-2136;
    delivery_sentinel();if(file)actual_file_gate(error);else actual_memory_gate(error);
    assert(!memcmp(&d,&expected,sizeof d));
    if(error)assert(s_render_phase==.5&&s_render_occ_ema==123&&!s_render_priming&&s_render_starved==7&&s_render_fade_pos==9&&s_render_fade_out==10);
    else assert_delivery_reset();cases++;
  }
  /* Actual desktop source must release APU before clearing SDL stream. */
  record_order=true;g_audio_stream=(void*)1;
  for(unsigned success=0;success<2;success++) {
    load_success=success!=0;rollback_load=false;absolute_enabled=true;restored_absolute_enabled=true;g_state_generation=guest_generation;g_audio_timeline_generation=guest_generation;
    d.sampleRead=0;d.sampleWrite=8192;g_audiobuffer_cur=scratch+3;g_audiobuffer_end=scratch+12;g_audio_primed=true;
    clear_calls=game_resets=osd_calls=call_count=rewind_shutdowns=rewind_configures=0;call_order[0]=0;
    bool ok=LoadStateSlot(8);assert(ok==load_success);
    if(success){assert(!strcmp(call_order,"LULUGLUC"));assert(clear_calls==1&&game_resets==1&&!osd_calls);assert(g_audiobuffer_end==g_audiobuffer_cur&&!g_audio_primed);assert(dsp_available(&d)==2136);}
    else{assert(!strcmp(call_order,"O"));assert(!clear_calls&&!game_resets&&osd_calls==1);assert(g_audiobuffer_end==scratch+12&&g_audio_primed);assert(dsp_available(&d)==8192);}
    unsigned before_clear=clear_calls;actual_generation_gate();actual_generation_gate();
    assert(clear_calls==before_clear&&rewind_shutdowns==success&&rewind_configures==success);
    cases++;
  }
  for(unsigned success=0;success<2;success++) {
    call_count=0;call_order[0]=0;clear_calls=game_resets=0;
    actual_resume_gate(success!=0);
    assert(!strcmp(call_order,success?"GLUC":""));assert(clear_calls==success&&game_resets==success);cases++;
  }
  for(unsigned flags=0;flags<16;flags++)for(unsigned file=0;file<2;file++) {
    bool error=(flags&1)!=0;rollback_load=(flags&2)!=0;absolute_enabled=(flags&4)!=0;restored_absolute_enabled=(flags&8)!=0;
    d.sampleRead=0;d.sampleWrite=8192;
    if(file)actual_file_gate(error);else actual_memory_gate(error);
    assert(dsp_available(&d)==(!error&&!rollback_load&&restored_absolute_enabled?2136u:8192u));cases++;
  }
  call_count=clear_calls=rewind_shutdowns=rewind_configures=0;call_order[0]=0;
  g_state_generation=guest_generation;g_audio_timeline_generation=guest_generation;guest_generation++;
  actual_generation_gate();assert(!strcmp(call_order,"LUCWR"));actual_generation_gate();
  assert(clear_calls==1&&rewind_shutdowns==1&&rewind_configures==1);cases++;
  record_order=false;g_audio_stream=NULL;return cases;
}
static void enable_echo(Dsp *d) {
  dsp_write(d,0x6c,0x1f);dsp_write(d,0x2c,48);dsp_write(d,0x3c,40);
  dsp_write(d,0x0d,20);dsp_write(d,0x6d,0x30);dsp_write(d,0x7d,1);
  dsp_write(d,0x0f,64);dsp_write(d,0x1f,32);dsp_write(d,0x4d,1);
}
int main(void) {
  unsigned cases=check_trim_api(),guards=check_actual_helper_and_load_gates();
  Apu *seed=apu_init(),*control=apu_init(),*trimmed=apu_init();
  assert(seed&&control&&trimmed);init_fixture(seed);
  Blob initial={0},before={0},after={0},end_control={0},end_trimmed={0};
  save(seed,&initial);Blob input=initial;
  load(control,&input);load(trimmed,&input);
  assert(!memcmp(initial.bytes,input.bytes,initial.used));
  ApuPortSched sched={0};apu_port_sched_save(seed,&sched);
  apu_port_sched_restore(control,&sched);apu_port_sched_restore(trimmed,&sched);
  save(trimmed,&before);uint32_t old_write=trimmed->dsp->sampleWrite;
  host_snes.apu=trimmed;g_snes=&host_snes;rollback_load=false;absolute_enabled=true;
  RtlApuLock();rtl_reset_loaded_audio();RtlApuUnlock();assert(dsp_available(trimmed->dsp)==2136);
  assert(trimmed->dsp->sampleWrite==old_write);save(trimmed,&after);
  same_synthesis(&before,&after);
  size_t read_offset=offsetof(Apu,pad)+6u-offsetof(Apu,ram)+offsetof(Dsp,sampleRead)-offsetof(Dsp,ram);
  for(size_t i=0;i<before.used;i++)if(i<read_offset||i>=read_offset+sizeof(uint32_t))
    assert(before.bytes[i]==after.bytes[i]);
  enable_echo(control->dsp);enable_echo(trimmed->dsp);
  for(unsigned i=0;i<6;i++) {
    uint64_t when=sched.clock+100+i*19000;
    assert(apu_schedulePortWrite(control,0,(uint8_t)(i*31+3),when));
    assert(apu_schedulePortWrite(trimmed,0,(uint8_t)(i*31+3),when));
  }
  Capture *a=calloc(1,sizeof *a),*b=calloc(1,sizeof *b);assert(a&&b);
  observed_apu=control;capture=a;for(unsigned i=0;i<131072;i++)apu_cycle(control);
  observed_apu=trimmed;capture=b;for(unsigned i=0;i<131072;i++)apu_cycle(trimmed);
  capture=NULL;assert(a->samples==4096&&b->samples==4096);
  assert(a->nonzero&&a->nonzero==b->nonzero);
  assert(memcmp(seed->ram+0x3000,control->ram+0x3000,0x800));
  assert(!memcmp(a->pcm,b->pcm,a->samples*2*sizeof(int16_t)));
  assert(a->drops==4096&&b->drops==0);
  assert(a->event_count==b->event_count&&a->event_count>100);
  assert(!memcmp(a->events,b->events,a->event_count*sizeof(Event)));
  ApuPortSched ca={0},cb={0};apu_port_sched_save(control,&ca);apu_port_sched_save(trimmed,&cb);
  assert(!memcmp(&ca,&cb,sizeof ca));save(control,&end_control);save(trimmed,&end_trimmed);
  same_synthesis(&end_control,&end_trimmed);
  assert(!memcmp(initial.bytes,input.bytes,initial.used));
  printf("PASS existing dsp_trimSamples:%u cases; extracted production helper/load/desktop gates:%u cases; snapshot serialized bytes differ only sampleRead before future execution; future PCM:%u nonzero:%u, portevents:%u, portClock:%llu equal; echo writes active; overflowcontrol:%u trimmed:%u; serializedsize:%zu ring:[%zu,%zu)\n",cases,guards,a->samples,a->nonzero,a->event_count,(unsigned long long)control->portClock,a->drops,b->drops,initial.used,ring_begin(),ring_end());
  free(a);free(b);apu_free(seed);apu_free(control);apu_free(trimmed);return 0;
}
