/* A diagnostic provenance test, not an oracle for an actual Lagoon spell.
 * Execute the real SPC/APU/DSP under different catch-up chunk boundaries.
 * Old port trace beam coordinates observe the caller CPU, not SPC event time.
 */
#include "snes/apu.h"
#include "snes/dsp_shadow.h"
#include "audio_trace.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

uint64_t g_apu_timer0_total_ticks;
int snes_frame_counter;
static uint64_t host_batch_end;
void RtlApuLock(void) {}
void RtlApuUnlock(void) {}
DspShadow *dsp_shadow_create(void) { return NULL; }
void dsp_shadow_free(DspShadow *p) { (void)p; }
void dsp_shadow_process(DspShadow *p,Dsp *d,int l,int r,int *ol,int *or_) {
  (void)p;(void)d;*ol=l;*or_=r;
}
static void probe(uint16_t *v,uint16_t *h) {
  uint64_t master=host_batch_end*21;
  *v=(uint16_t)(master/1364%262);*h=(uint16_t)(master%1364);
}
typedef struct Save { SaveLoadInfo base;FILE *out; } Save;
static void save_bytes(SaveLoadInfo *s,void *data,size_t count) {
  assert(fwrite(data,1,count,((Save*)s)->out)==count);
}
static FILE *open_file(const char *dir,const char *name,const char *mode) {
  char path[4096];assert(snprintf(path,sizeof path,"%s/%s",dir,name)>0);
  FILE *out=fopen(path,mode);assert(out);return out;
}
int main(int argc,char **argv) {
  assert(argc==3);unsigned chunk=(unsigned)strtoul(argv[1],NULL,10);
  assert(chunk&&chunk<=32768);Apu *a=apu_init();apu_reset(a);
  /* Original fixture program: copy CPU input0 to SPC output1, then output2
   * gets input+1. Repeats forever; no guessed SPC result callback is used. */
  const uint8_t code[]={0xe4,0xf4,0xc4,0xf5,0xbc,0xc4,0xf6,0x2f,0xf7};
  memcpy(a->ram+0x200,code,sizeof code);a->spc->pc=0x200;
  a->romReadable=false;a->cpuCyclesLeft=0;
  /* A real noise voice makes PCM equality non-vacuous. */
  dsp_write(a->dsp,0x00,127);dsp_write(a->dsp,0x01,127);
  dsp_write(a->dsp,0x0c,127);dsp_write(a->dsp,0x1c,127);
  dsp_write(a->dsp,0x05,0);dsp_write(a->dsp,0x07,127);
  dsp_write(a->dsp,0x3d,1);dsp_write(a->dsp,0x6c,0x3f);
  dsp_write(a->dsp,0x4c,1);
  audio_trace_set_beam_probe(probe);
  const uint64_t times[]={0,100,317,811,1337,6666,16384,21817,30000};
  for(unsigned i=0;i<sizeof times/sizeof *times;i++)
    assert(apu_schedulePortWrite(a,0,(uint8_t)(i*23),times[i]));
  for(unsigned begin=0;begin<32768;begin+=chunk) {
    host_batch_end=begin+chunk;if(host_batch_end>32768)host_batch_end=32768;
    snes_frame_counter=(int)(host_batch_end*21/(1364*262));
    while(a->portClock<host_batch_end)apu_cycle(a);
  }
  Save s={0};s.base.func=save_bytes;s.out=open_file(argv[2],"apu-state.bin","wb");
  apu_saveload(a,&s.base);assert(!fclose(s.out));
  ApuPortSched sched={0};apu_port_sched_save(a,&sched);
  FILE *out=open_file(argv[2],"port-scheduler.bin","wb");
  assert(fwrite(&sched,1,sizeof sched,out)==sizeof sched);assert(!fclose(out));
  AudioTracePortEvent *ev=calloc(AUDIO_TRACE_PORT_RING,sizeof *ev);assert(ev);
  uint64_t oldest,total;uint32_t count=audio_trace_copy_port_events(0,AUDIO_TRACE_PORT_RING,ev,&oldest,&total);
  assert(!oldest&&count==total&&count>=30);
  out=open_file(argv[2],"events.tsv","w");
  fprintf(out,"frame\tvpos\thpos\ttype\tport\tvalue\tsample\n");
  for(uint32_t i=0;i<count;i++)fprintf(out,"%u\t%u\t%u\t%u\t%u\t%u\t%llu\n",ev[i].frame,ev[i].vpos,ev[i].hpos,ev[i].type,ev[i].port,ev[i].val,(unsigned long long)ev[i].sample_idx);
  assert(!fclose(out));free(ev);
  char path[4096];snprintf(path,sizeof path,"%s/pcm.wav",argv[2]);
  uint64_t start,frames;assert(!audio_trace_dump_wav(path,0,0,&start,&frames));assert(start==0&&frames==1024);
  printf("PASS realAPU chunk%u cycles%llu events%u samples%llu\n",chunk,(unsigned long long)a->portClock,count,(unsigned long long)frames);
  apu_free(a);return 0;
}
