#ifndef LAGOON_IDLE_POLL_H
#define LAGOON_IDLE_POLL_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Original poll, counter reset and NMI increment must agree before declaring
 * a stable interrupt poll. Declaration grants no IRQ/HDMA/epoch exemption. */
static inline bool LagoonIdle_SourceMatches(const uint8_t *poll,
    const uint8_t *reset,const uint8_t *increment) {
  static const uint8_t poll_bytes[]={0xad,0x18,0x04,0xc9,0x02,0x90,0xf9,0x4c,0x88,0x80};
  static const uint8_t reset_bytes[]={0x9c,0x18,0x04};
  static const uint8_t increment_bytes[]={0xee,0x18,0x04};
  return poll && reset && increment &&
    !memcmp(poll,poll_bytes,sizeof poll_bytes) &&
    !memcmp(reset,reset_bytes,sizeof reset_bytes) &&
    !memcmp(increment,increment_bytes,sizeof increment_bytes);
}
/* Original display wait: NMI decrements023C while the CPU polls it. */
static inline bool LagoonFrameWait_SourceMatches(const uint8_t *poll,
    const uint8_t *request,const uint8_t *decrement) {
  static const uint8_t p[]={0xad,0x3c,0x02,0xd0,0xfb};
  static const uint8_t r[]={0xa9,0x01,0x8d,0x3c,0x02};
  static const uint8_t d[]={0xad,0x3c,0x02,0x3a,0x8d,0x3c,0x02};
  return poll&&request&&decrement&&!memcmp(poll,p,sizeof p)&&
    !memcmp(request,r,sizeof r)&&!memcmp(decrement,d,sizeof d);
}
#endif
