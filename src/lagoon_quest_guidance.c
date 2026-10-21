/* Tablet quest guidance, derived from the original Thor/elder scripts and
 * original Hope chest. This never discovers fog, grants items or sets flags. */
#include "lagoon_quest_guidance.h"
#include <string.h>

typedef struct Span { unsigned pc, bytes; uint64_t digest; } Span;
static const Span spans[] = {
  {0x05c3ff,80,UINT64_C(0x337e279595b08c92)}, /* Thor dialogue/award branches. */
  {0x05c622,38,UINT64_C(0xf06c5b63322c831c)}, /* Voloh elder branches. */
  {0x01cde3,19,UINT64_C(0x5a1179b525e83a7a)}, /* Item bit test. */
  {0x01cd8d,19,UINT64_C(0xcc03b14d009d4a4a)}, /* Story bit test. */
  {0x05cc57,33,UINT64_C(0x76808cf19fc94554)}, /* Original Voloh Thor placement. */
  {0x059050,2,UINT64_C(0x0a5bea07b6c11e58)}, /* Room0A chest list pointer. */
  {0x0590ed,7,UINT64_C(0xfe8b58c2067c983d)} /* x1,y15,flag3A,item38. */
};
static int qualified;
static uint64_t hash(const uint8_t *r, unsigned pc, unsigned n) {
  size_t o=((pc>>16)&127)*32768+(pc&32767);
  uint64_t v=UINT64_C(14695981039346656037);
  while(n--)v=(v^r[o++])*UINT64_C(1099511628211);
  return v;
}
void LagoonQuestGuidance_Init(const uint8_t *r,size_t size) {
  qualified=0;
  if(!r||size!=0x100000)return;
  for(unsigned n=0;n<sizeof spans/sizeof *spans;n++)
    if(hash(r,spans[n].pc,spans[n].bytes)!=spans[n].digest)return;
  qualified=1;
}
static int bit(const uint8_t *r,unsigned base,unsigned n) {
  return !!(r[base+n/8]&(0x80u>>(n&7)));
}
int LagoonQuestGuidance_Get(const uint8_t *r,LagoonQuestGuidance *out) {
  if(!out)return 0;
  memset(out,0,sizeof *out);
  if(!qualified||!r)return 0;
  unsigned room=r[0x48e]|r[0x48f]<<8;
  /* Restrict these early-game hints to the actual quest region. Flag11 is
   * the elder's learned-spell branch; collecting tablets must not revive an
   * already completed quest, including after its items have been consumed. */
  if(room<8||room>10||bit(r,0x4e0,0x11)||
     !bit(r,0x4e0,0x17)||!bit(r,0x4d0,0x15))return 0;
  int faith=bit(r,0x4d0,0x36),wish=bit(r,0x4d0,0x37),hope=bit(r,0x4d0,0x38);
  if(!faith) {
    out->title="Tablet of Faith";
    out->detail="Speak to Thor near Voloh's south entrance.";
    out->room=9;out->x=368;out->y=864;
  } else if(!wish) {
    out->title="Tablet of Wish";
    out->detail="Follow Thor out of Voloh. Speak to him when he stops.";
    out->room=8;
    /* Thor's scripted stop, not a guessed destination, owns this location. */
    out->marker=0;return 1;
  } else if(!hope) {
    out->title="Tablet of Hope";
    out->detail="Go northeast to Philips Castle. Cross the broken bridge, then jump west to the chest.";
    out->room=10;out->x=48;out->y=496;
  } else {
    out->title="Return to Voloh";
    out->detail="Take all three tablets to the elder in Voloh.";
    out->room=9;
    out->marker=0;return 1;
  }
  out->marker=1;return 1;
}
