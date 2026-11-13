#ifndef LAGOON_CLOUD_MOTION76_H
#define LAGOON_CLOUD_MOTION76_H
/* Presentation-only temporal model. The caller must bind every draw to the
 * same exact room28/generation/ROM/cloud graphics+palette ownership. This helper
 * never reads/writes guest memory or advances the guest RNG. */
#include "lagoon_cloud_view75.h"
typedef struct LagoonCloudMotion76 {
  LagoonCloudView75 shape,seed;
  uint64_t generation;
  int world_x,world_y;
  unsigned room,ready;
  uint8_t tick;
} LagoonCloudMotion76;
typedef struct LagoonCloudMotion76Sample {
  const LagoonCloudView75 *shape;
  int x,y;
  unsigned wraps;
} LagoonCloudMotion76Sample;
static int LagoonCloud76_Clamp(int a,int lo,int hi){return a<lo?lo:a>hi?hi:a;}
static void LagoonCloud76_SeedY(LagoonCloudMotion76 *s,int cy,int top,unsigned h){
  int sy=LagoonCloud76_Clamp(s->seed.anchor_y,-32,223);
  s->world_y=cy+top-32+LagoonCloud75_FloorDiv((sy+32)*((int)h+32),256);
}
/* tick is the original main-loop$0408 clock, NOT presentation serial/frame.
 * The measured source cloud updates six pixels per main-loop tick; in the
 * checked room this is one tick every two60Hz fields. Width must never multiply
 * velocity. Call once or many times for the same tick: results remain stable.
 *
 * source is the current exact native-OAM cloud witness, optionally NULL when
 * the guest has retired/not spawned its own narrow cloud. Those guest lifetime
 * edges do NOT retire/reposition the presentation cloud while still visible. */
static int LagoonCloudMotion76_Step(LagoonCloudMotion76 *s,
    uint64_t generation,unsigned room,uint8_t tick,int cx,int cy,
    int left,int top,unsigned width,unsigned height,
    const LagoonCloudView75 *source,int owned,LagoonCloudMotion76Sample *out){
  if(out)memset(out,0,sizeof *out);
  if(!s||!out)return 0;
  if(!owned||room!=0x28||width<256||width>1280||height<224||height>336||
      left>0||top>0||left+(int)width<256||top+(int)height<224){memset(s,0,sizeof *s);return 0;}
  int valid_source=source&&source->valid&&source->count>0&&source->count<=64;
  if(!s->ready||s->generation!=generation||s->room!=room){
    memset(s,0,sizeof *s);
    if(!valid_source)return 0;
    s->generation=generation;s->room=room;s->tick=tick;s->ready=1;
    s->shape=s->seed=*source;
    int x,y;LagoonCloudView75_RemapAnchor(source,left,top,width,height,&x,&y);
    s->world_x=cx+x;s->world_y=cy+y;
  }else{
    unsigned elapsed=(uint8_t)(tick-s->tick);
    /* Ordinary presentation observes every field; larger gaps require a fresh
     * source seed rather than an unbounded extrapolation across a lost scene. */
    if(elapsed>4){s->ready=0;return LagoonCloudMotion76_Step(s,generation,room,tick,cx,cy,left,top,width,height,source,owned,out);}
    s->tick=tick;
    if(valid_source)s->seed=*source;
    s->world_x+=(int)elapsed*6;
  }
  int right=cx+left+(int)width;
  int period=(int)width+176;
  if(s->world_x>=right){
    int extra=s->world_x-right;
    out->wraps=1+(unsigned)(extra/period);
    s->world_x=cx+left-176+extra%period;
    s->shape=s->seed;
    LagoonCloud76_SeedY(s,cy,top,height);
  }
  out->shape=&s->shape;out->x=s->world_x-cx;out->y=s->world_y-cy;
  return 1;
}
#endif
