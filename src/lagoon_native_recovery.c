/* Reenter only witnessed original recovery waits, without fabricating frames. */
#include "lagoon_native_recovery.h"
#include "snes/interp_bridge.h"
#include "common_rtl.h"
#include <string.h>
static int bytes(unsigned pc,const unsigned char *v,unsigned count) {
  unsigned offset=((pc>>16)&127)*0x8000+(pc&0x7fff);
  return g_rom && offset+count<=0x100000 && !memcmp(g_rom+offset,v,count);
}
static uint64_t source_hash(unsigned pc,unsigned count) {
  unsigned offset=((pc>>16)&127)*0x8000+(pc&0x7fff);
  uint64_t h=UINT64_C(1469598103934665603);
  if(!g_rom || offset+count>0x100000)return 0;
  for(unsigned i=0;i<count;++i)h=(h^g_rom[offset+i])*UINT64_C(1099511628211);
  return h;
}
static int recovery_source(const InterpAotContinuation *d) {
  static const unsigned char wait[]={0xAD,0x38,0x02,0x0D,0x39,0x02,0xD0,0x01,
    0x6B,0xA9,0x40,0x0C,0x3A,0x02,0xAD,0x3A,0x02,0x29,0x40,0xD0,0xF9,0x6B};
  static const unsigned char call[]={0x22,0x85,0x89,0x00};
  static const unsigned char save_x[]={0xDA},restore_x[]={0xFA,0x60};
  static const unsigned char hp[]={0xA5,0xA4,0xCD,0x20,0x05,0xD0,0xF0,0x80,0x28};
  static const unsigned char mp[]={0xA5,0xA4,0xCD,0x22,0x05,0xD0,0xF0};
  static const unsigned char hp_owner[]={0x20,0x87,0xD4},mp_owner[]={0x20,0xB1,0xD4};
  /* The original death-table JSL consumes only its own three-byte frame.
   * After the real queue wait, the outer JSR92C3 frame still returns to9294.
   * Reenter only four RAM instructions, then retain original931E/audio path.
   * The bridge witnesses the wait, owner bytes and timeline; no frame is made. */
  if(d->continuation_pc24==0x019313) {
    static const unsigned char owner[]={0x20,0xC3,0x92};
    return d->call_pc24==0x01930F && d->wait_pc24==0x008985 &&
      d->owner_return_pc24==0x019294 && d->return_sites[0]==0x00898D &&
      d->return_sites[1]==0x00899A && !d->d && d->m==1 && !d->x && !d->e &&
      d->db==1 && d->wait_frame==3 && !d->saved_stack_bytes && d->owner_frame==2 &&
      bytes(0x008985,wait,sizeof(wait)) && bytes(0x019291,owner,sizeof(owner)) &&
      source_hash(0x0192C3,0x2E)==UINT64_C(0xE1DBDD26BFA53066) &&
      source_hash(0x019306,0x18)==UINT64_C(0xA6BDAFB7EA82D8D8) &&
      source_hash(0x0080E9,0x20)==UINT64_C(0xD98520CA8A4BEFBC);
  }
  unsigned hp_call=d->owner_return_pc24==0x01B812 ? 0x01B80F :
      d->owner_return_pc24==0x01D484 ? 0x01D481 : 0;
  unsigned mp_call=d->owner_return_pc24==0x01B815 ? 0x01B812 :
      d->owner_return_pc24==0x01D4AE ? 0x01D4AB : 0;
  return bytes(0x008985,wait,sizeof(wait)) && bytes(d->call_pc24,call,sizeof(call)) &&
    bytes(0x01D4D9,restore_x,sizeof(restore_x)) &&
    (d->continuation_pc24==0x01D4A2 ?
      hp_call && bytes(0x01D487,save_x,sizeof(save_x)) && bytes(hp_call,hp_owner,sizeof(hp_owner)) &&
        bytes(0x01D4A2,hp,sizeof(hp)) :
      d->continuation_pc24==0x01D4CC && mp_call && bytes(0x01D4B1,save_x,sizeof(save_x)) &&
        bytes(mp_call,mp_owner,sizeof(mp_owner)) && bytes(0x01D4CC,mp,sizeof(mp)));
}
void LagoonNativeRecovery_BeforeFrame(void) {
  static int configured;
  static const InterpAotContinuation entries[]={
    {0x01D49E,0x008985,0x01D4A2,0x01D484,{0x00898D,0x00899A},0,1,0,0,1,3,2,2},
    {0x01D4C8,0x008985,0x01D4CC,0x01D4AE,{0x00898D,0x00899A},0,1,0,0,1,3,2,2},
    {0x01D49E,0x008985,0x01D4A2,0x01B812,{0x00898D,0x00899A},0,1,0,0,1,3,2,2},
    {0x01D4C8,0x008985,0x01D4CC,0x01B815,{0x00898D,0x00899A},0,1,0,0,1,3,2,2},
    {0x01930F,0x008985,0x019313,0x019294,{0x00898D,0x00899A},0,1,0,0,1,3,0,2}
  };
  if(!configured && g_rom)configured=interp_bridge_set_aot_continuations(entries,
      (unsigned)(sizeof(entries)/sizeof(entries[0])),recovery_source);
}
