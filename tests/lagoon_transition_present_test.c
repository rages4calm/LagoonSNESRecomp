#include "lagoon_transition_present.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static uint32_t dst[802*224],reference[802*224];
static void fill(uint32_t color){for(unsigned i=0;i<802*224;i++)dst[i]=color;}
static void check(unsigned width,uint32_t color){
  for(unsigned y=0;y<224;y++)for(unsigned x=0;x<width;x++)assert(dst[y*802+x]==color);
}
static uint32_t faded(unsigned level){
  return 0xff000000u|((240*level/15)<<16)|((120*level/15)<<8)|(60*level/15);
}
int main(void){
 unsigned checks=0;
 for(unsigned width=256;width<=800;width+=34){
  size_t pitch=802*4;
  fill(0xfff0783cu);memcpy(reference,dst,sizeof dst);
  LagoonTransition_Reset();
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,0,1,15,1,1));
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,1,0,15,1,1));
  /* Real temporal order: outgoing brightness decreases; it must never be
   * rebrightened by the loader's later INIDISP bounce over incomplete data. */
  for(int level=15;level>=0;level--){
   memset(dst,0,sizeof dst);assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,0,1,(unsigned)level,2,2+(15-level)));
   check(width,faded((unsigned)level));checks++;
  }
  for(unsigned i=0;i<74;i++){
   fill(0xffabcdef);assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,0,1,15,2,85+i));
   for(unsigned y=0;y<224;y++)for(unsigned x=0;x<width;x++)if(dst[y*802+x]!=0xff000000u){
    fprintf(stderr,"FAIL outgoing room resurrected after blank: width=%u frame=%u got=%08x\n",width,85+i,dst[y*802+x]);return 73;
   }
   checks++;
  }
  /* Coherent outgoing fields also contribute to the low-water mark. */
  fill(0xfff0783cu);assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,1,0,15,1,200));
  for(unsigned level=15;level>=1;--level){
   fill(0xff345678);assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,1,1,level,2,216-level));
   check(width,0xff345678);
  }
  fill(0xffabcdef);assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,0,1,15,2,216));check(width,faded(1));
  fill(0xffabcdef);assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,0,1,128,2,217));check(width,0xff000000u);
  fill(0xffabcdef);assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,0,1,15,2,218));check(width,0xff000000u);
  memcpy(dst,reference,sizeof dst);assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,5,0,0,15,1,219));assert(!memcmp(dst,reference,sizeof dst));
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width-1,224,5,0,1,15,1,220));assert(!memcmp(dst,reference,sizeof dst));
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,1,15,1,221));
  memcpy(dst,reference,sizeof dst);
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,1,0,15,10,1));
  memset(dst,0,sizeof dst);
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,15,10,1)); /* same-room UI */
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,15,0,1)); /* unqualified descriptor */
  assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,15,9,2));check(width,0xfff0783cu);
  assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,15,9,2)); /* repeated presentation */
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,15,9,3)); /* next field refuses */
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,15,8,50)); /* stale old scene */
  assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,1,14,9,3));check(width,faded(14));
#ifndef TRANSITION_TEST_LEGACY
  /* A proven incoming native field withdraws old pixels across a subsequent
   * refused field, not merely during the one presentation that proved it. */
  LagoonTransition_ObserveIncoming(6,4);
  fill(0xff123456);assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,1,15,9,5));check(width,0xff123456);
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,15,9,5));check(width,0xff123456);
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,1,0,15,9,6));
  fill(0);assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,1,15,8,7));check(width,0xff123456);
  LagoonTransition_ObserveIncoming(7,8); /* generation change cannot lend pixels */
  fill(0xffabcdef);assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,7,0,1,15,8,9));check(width,0xffabcdef);
#endif
  /* Only the exact permitted preloader handoff can lower the floor before
   *100E is set. Generic UI/unknown/stale refusals cannot poison a later fade. */
  fill(0xfff0783cu);assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,1,0,15,10,1000));
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,128,10,1001));
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,128,0,1001));
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,128,9,1002));
  fill(0);assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,1,15,9,1003));check(width,0xfff0783cu);
  fill(0xfff0783cu);assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,1,0,15,10,2000));
  fill(0xffabcdef);assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,128,9,2001));check(width,0xff000000u);
  fill(0xffabcdef);assert(LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,1,15,9,2002));check(width,0xff000000u);
  LagoonTransition_Reset();
  assert(!LagoonTransition_Apply((uint8_t*)dst,pitch,width,224,6,0,0,15,9,1));
 }
 printf("PASS %u transition temporal fields: outgoing fade/forcedblank floor survives loader rebound, coherent fade observation, persistent incoming retirement,256..800 stride, fresh preloader/repeated/stale/unknown/same-room/generation guards\n",checks);
 return checks?0:1;
}
