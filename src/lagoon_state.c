/* The guest blob's emulator CPU is not the active native/interpreter CPU.
 * Keep the actual execution cursor and shared runtime residue with every state.
 * A complete preflight validates this extension before generic guest mutation. */
#include "lagoon_state.h"
#include "lagoon_follower.h"
#include "lagoon_soundtrack.h"
#include "beam_frame_driver.h"
#include "common_rtl.h"
#include "cpu_state.h"
#include "snes/saveload.h"
#include "snes/ppu.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

enum { kMagic = 0x3245474c, kVersion = 3, kAudioBytes = 16 };
typedef struct StateCursor { SnesBeamFrameState beam; PpuPortState ports; } StateCursor;
typedef struct StateHeader { uint32_t magic, version, bytes, checksum; } StateHeader;
typedef struct StateBuffer {
  SaveLoadInfo base;
  uint8_t *data;
  size_t capacity, position;
  int error;
} StateBuffer;
static int pending_execution;
static uint64_t load_generation;
uint64_t LagoonState_LoadGeneration(void) { return load_generation; }
static StateCursor pending_cursor;
const char *LagoonState_Identity(void) { return "lagoon-native-state-v3"; }
static uint32_t checksum(const uint8_t *data, size_t size) {
  uint32_t hash=2166136261u;
  for(size_t i=0;i<size;++i)hash=(hash^data[i])*16777619u;
  return hash;
}
static void write_buffer(SaveLoadInfo *sli, void *data, size_t size) {
  StateBuffer *b=(StateBuffer *)sli;
  if(size>b->capacity-b->position){b->error=1;return;}
  memcpy(b->data+b->position,data,size);b->position+=size;
}
static void read_buffer(SaveLoadInfo *sli, void *data, size_t size) {
  StateBuffer *b=(StateBuffer *)sli;
  if(size>b->capacity-b->position){b->error=1;return;}
  memcpy(data,b->data+b->position,size);b->position+=size;
}
static int valid_body(const StateHeader *h,const uint8_t *body,size_t size) {
  if(h->magic!=kMagic || h->version!=kVersion || h->bytes!=size ||
      size<sizeof(StateCursor)+RtlExecutionStateSize() ||
      checksum(body,size)!=h->checksum)return 0;
  SnesBeamFrameState beam;memcpy(&beam,body,sizeof(beam));
  if((beam.resume_pc && !cpu_pc24_resumable(beam.resume_pc)) || beam.wai_halted>1 ||
      beam.reserved[0] || beam.reserved[1] || beam.reserved[2])return 0;
  size_t exec=RtlExecutionStateSize();
  return RtlValidateExecutionState(body+sizeof(StateCursor),exec) &&
      ppu_raster_state_valid(body+sizeof(StateCursor)+exec,size-sizeof(StateCursor)-exec);
}
bool LagoonState_Preflight(const void *data,size_t size) {
  pending_execution=0;
  uint32_t header[2],current;
  if(size<sizeof(header))return false;
  memcpy(header,data,sizeof(header));
  if(header[0]!=0x52544c53u)return false;
  size_t guest=RtlSnapshotGuestSize(&current);
  if(header[1]!=current){
    fprintf(stderr,"[lagoon-state] state schema unsupported; original file unchanged\n");
    return false;
  }
  if(!guest || size<guest)return false;
  if(size<=guest+kAudioBytes){
    fprintf(stderr,"[lagoon-state] older test state omitted execution data; start a fresh game and save with this build\n");
    return false;
  }
  if(size<guest+kAudioBytes+sizeof(StateHeader))return false;
  if(!LagoonSoundtrack_Preflight(data,guest+kAudioBytes))return false;
  const uint8_t *extra=(const uint8_t *)data+guest+kAudioBytes;
  StateHeader h;memcpy(&h,extra,sizeof(h));
  return valid_body(&h,extra+sizeof(h),size-guest-kAudioBytes-sizeof(h));
}
void LagoonState_SaveExtra(SaveLoadInfo *sli) {
  LagoonSoundtrack_SaveExtra(sli);
  size_t exec_end=sizeof(StateCursor)+RtlExecutionStateSize();
  size_t bytes=exec_end+ppu_raster_state_size();
  uint8_t *body=malloc(bytes);
  if(!body){RtlStateStreamFail(sli);return;}
  StateCursor cursor;memset(&cursor,0,sizeof(cursor));
  snes_beam_frame_driver_capture(&cursor.beam);ppu_capture_ports(g_ppu,&cursor.ports);
  memcpy(body,&cursor,sizeof(cursor));
  StateBuffer b={{write_buffer,NULL},body,exec_end,sizeof(cursor),0};
  RtlSaveExecutionState(&b.base);
  if(b.error || b.position!=exec_end){free(body);RtlStateStreamFail(sli);return;}
  ppu_raster_state_save(body+exec_end);
  StateHeader h={kMagic,kVersion,(uint32_t)bytes,checksum(body,bytes)};
  sli->func(sli,&h,sizeof(h));sli->func(sli,body,bytes);free(body);
}
void LagoonState_LoadExtra(SaveLoadInfo *sli,uint32_t version) {
  pending_execution=0;
  LagoonSoundtrack_LoadExtra(sli,version);
  size_t bytes=RtlStateBytesRemaining(sli);
  if(!bytes){RtlStateStreamFail(sli);return;}
  StateHeader h;
  if(bytes<sizeof(h)){RtlStateStreamFail(sli);return;}
  sli->func(sli,&h,sizeof(h));
  size_t exec_end=sizeof(pending_cursor)+RtlExecutionStateSize();
  if(h.bytes<exec_end || bytes!=sizeof(h)+h.bytes){
    RtlStateStreamFail(sli);return;
  }
  uint8_t *body=malloc(h.bytes);
  if(!body){RtlStateStreamFail(sli);return;}
  sli->func(sli,body,h.bytes);
  if(!valid_body(&h,body,h.bytes)){free(body);RtlStateStreamFail(sli);return;}
  memcpy(&pending_cursor,body,sizeof(pending_cursor));
  StateBuffer b={{read_buffer,NULL},body,exec_end,sizeof(pending_cursor),0};
  pending_execution=RtlLoadExecutionState(&b.base) && !b.error && b.position==b.capacity;
  /* The validated journal belongs to this snapshot, never to the cold boot
   * that happened to precede it. Keep pending post-vblank writes as well. */
  if(pending_execution)
    pending_execution=ppu_raster_state_load(body+exec_end,h.bytes-exec_end);
  free(body);
  if(!pending_execution)RtlStateStreamFail(sli);
}
void LagoonState_OnLoaded(uint32_t version) {
  LagoonFollower_Discard();
  if(pending_execution){
    RtlApplyExecutionState();
    snes_beam_frame_driver_restore(&pending_cursor.beam);
    ppu_restore_ports(g_ppu,&pending_cursor.ports);
    ++load_generation;
    pending_execution=0;
  }
  LagoonSoundtrack_OnStateLoaded(version);
}
