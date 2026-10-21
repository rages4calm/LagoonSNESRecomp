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
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,0,1,15));
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,1,0,15));
  for(unsigned level=0;level<16;level++){
   memset(dst,0,sizeof dst);assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,0,1,level));
   for(unsigned y=0;y<224;y++)for(unsigned x=0;x<width;x++)assert(dst[y*802+x]==(0xff000000u|((240*level/15)<<16)|((120*level/15)<<8)|(60*level/15)));
   checks++;
  }
  memcpy(dst,reference,sizeof dst);assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,0,0,15));assert(!memcmp(dst,reference,sizeof dst));
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width-1,224,5,0,1,15));
  assert(!memcmp(dst,reference,sizeof dst));
  assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,0,1,128));
  for(unsigned y=0;y<224;y++)for(unsigned x=0;x<width;x++)assert(dst[y*802+x]==0xff000000u);
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,1,15));
 }
 puts("PASS transition whole-canvas fade, nativewidth..ultrawide, loader-only, generation/size refusal, no ordinary UI changes");
 return checks?0:1;
}
