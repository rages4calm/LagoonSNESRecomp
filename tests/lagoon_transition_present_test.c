#include "lagoon_transition_present.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static uint32_t dst[802*224],reference[802*224];
int main(void){
 unsigned checks=0;
 for(unsigned width=256;width<=800;width+=34){
  size_t pitch=802*4;
  for(unsigned i=0;i<802*224;i++)reference[i]=dst[i]=0xfff0783cu;
  LagoonTransition_Reset();
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,0,1,15,1,1));
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,1,0,15,1,1));
  for(unsigned level=0;level<16;level++){
   memset(dst,0,sizeof dst);assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,0,1,level,1,1));
   for(unsigned y=0;y<224;y++)for(unsigned x=0;x<width;x++)assert(dst[y*802+x]==(0xff000000u|((240*level/15)<<16)|((120*level/15)<<8)|(60*level/15)));
   checks++;
  }
  memcpy(dst,reference,sizeof dst);assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,0,0,15,1,1));assert(!memcmp(dst,reference,sizeof dst));
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width-1,224,5,0,1,15,1,1));
  assert(!memcmp(dst,reference,sizeof dst));
  assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,0,1,128,1,1));
  for(unsigned y=0;y<224;y++)for(unsigned x=0;x<width;x++)assert(dst[y*802+x]==0xff000000u);
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,1,15,1,1));
  memcpy(dst,reference,sizeof dst);
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,1,0,15,10,1));
  memset(dst,0,sizeof dst);
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,15,10,1)); /* same-room UI */
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,15,0,1)); /* unqualified descriptor */
  assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,15,9,2));
  for(unsigned y=0;y<224;y++)for(unsigned x=0;x<width;x++)assert(dst[y*802+x]==reference[y*802+x]);
  assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,15,9,2)); /* repeated presentation */
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,15,9,3)); /* next field refuses */
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,15,8,50)); /* stale old scene */
  assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,1,14,9,3)); /* original loader continues */
  LagoonTransition_Reset();
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,15,9,1));
 }
 puts("PASS transition whole-canvas fades256..800, loader/blank, fresh one-field descriptor handoff, repeated presents, stale/unknown/same-room refusal");
 return checks?0:1;
}
