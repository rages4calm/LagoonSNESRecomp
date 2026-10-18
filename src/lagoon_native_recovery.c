/* Reenter only witnessed original recovery waits, without fabricating frames. */
#include "lagoon_native_recovery.h"
#include "snes/interp_bridge.h"
#include "common_rtl.h"
#include <string.h>
static int bytes(unsigned pc,const unsigned char *v,unsigned count) {
  unsigned offset=((pc>>16)&127)*0x8000+(pc&0x7fff);
  return g_rom && offset+count<=0x100000 && !memcmp(g_rom+offset,v,count);
}
static int recovery_source(const InterpAotContinuation *d) {
  static const unsigned char wait[]={0xAD,0x38,0x02,0x0D,0x39,0x02,0xD0,0x01,
    0x6B,0xA9,0x40,0x0C,0x3A,0x02,0xAD,0x3A,0x02,0x29,0x40,0xD0,0xF9,0x6B};
  static const unsigned char call[]={0x22,0x85,0x89,0x00};
  static const unsigned char save_x[]={0xDA},restore_x[]={0xFA,0x60};
  static const unsigned char hp[]={0xA5,0xA4,0xCD,0x20,0x05,0xD0,0xF0,0x80,0x28};
  static const unsigned char mp[]={0xA5,0xA4,0xCD,0x22,0x05,0xD0,0xF0};
  static const unsigned char hp_owner[]={0x20,0x87,0xD4},mp_owner[]={0x20,0xB1,0xD4};
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
    {0x01D4C8,0x008985,0x01D4CC,0x01B815,{0x00898D,0x00899A},0,1,0,0,1,3,2,2}
  };
  if(!configured && g_rom)configured=interp_bridge_set_aot_continuations(entries,4,recovery_source);
}
