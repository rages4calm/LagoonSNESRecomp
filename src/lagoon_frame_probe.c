/* Optional read-only movement/logic cadence probe. Buffer until exit: disk I/O
 * during each field would contaminate the wall-time measurement. */
#include "lagoon_frame_probe.h"
#include "common_rtl.h"
#include "cpu_state.h"
#include <stdio.h>
#include <stdlib.h>

enum { kProbeCapacity = 36000 };
typedef struct Probe {
  unsigned frame, room, x, y, vx, vy, clock, nmis, flags, loading, actors, enemies, hp;
  unsigned pc, index, queue, request, visible_enemies, logic_updates;
  double seconds, guest_seconds;
} Probe;
static Probe *samples;
static unsigned count;
static const char *output;
static unsigned word(unsigned a) { return g_ram[a] | g_ram[a+1]<<8; }
static void finish(void) {
  if (!samples) return;
  FILE *f=fopen(output,"w");
  if (f) {
    fputs("frame,seconds,guest_ms,room,x,y,vx,vy,logic_clock,nmis,flags,loading,actors,enemies,hp,last_interp_pc,index,sort_queue,request,visible_enemies,logic_updates\n",f);
    for(unsigned i=0;i<count;++i) {
      const Probe *p=samples+i;
      fprintf(f,"%u,%.9f,%.6f,%u,%u,%u,%d,%d,%u,%u,%u,%u,%u,%u,%u,%06X,%04X,%u,%u,%u,%u\n",
        p->frame,p->seconds,p->guest_seconds*1000,p->room,p->x,p->y,
        (int)(int16_t)p->vx,(int)(int16_t)p->vy,p->clock,p->nmis,p->flags,
        p->loading,p->actors,p->enemies,p->hp,p->pc,p->index,p->queue,p->request,
        p->visible_enemies,p->logic_updates);
    }
    fclose(f);
  }
  free(samples);samples=NULL;
}
void LagoonFrameProbe_Record(const SnesDesktopHostFrameStats *s) {
  static int configured;
  if (!configured) {
    configured=1;output=getenv("LAGOON_FRAME_PROBE");
    if(output && *output) { samples=calloc(kProbeCapacity,sizeof(*samples)); if(samples)atexit(finish); }
  }
  if(!samples || count==kProbeCapacity)return;
  static uint64_t generation;
  uint64_t current_generation=RtlStateGeneration();
  unsigned delta=count&&generation==current_generation?
      (uint8_t)(g_ram[0x408]-samples[count-1].clock):0;
  generation=current_generation;
  Probe *p=samples+count++;
  *p=(Probe){.frame=s->frame,.seconds=s->run_seconds,.guest_seconds=s->guest_seconds,
    .room=word(0x48e),.x=word(0x502),.y=word(0x504),.vx=word(0x506),.vy=word(0x508),
    .clock=g_ram[0x408],.nmis=g_ram[0x418],.flags=g_ram[0x500],.loading=g_ram[0x100e],.hp=word(0x520),
    .pc=g_interp816_cur_pc,.index=g_cpu.X,.queue=g_ram[0x48c],.request=g_ram[0x23a],
    .logic_updates=delta};
  for(unsigned a=0x540;a<0xd40;a+=64)if((g_ram[a]&0xc0)==0xc0) {
    ++p->actors;if(g_ram[a+0x18]==2||g_ram[a+0x18]==3){
      ++p->enemies;if(g_ram[a]&8)++p->visible_enemies;
    }
  }
}
