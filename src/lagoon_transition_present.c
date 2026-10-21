#include "lagoon_transition_present.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
enum { MAX_WIDTH=800,MAX_HEIGHT=224 };
static uint32_t pixels[MAX_WIDTH*MAX_HEIGHT];
static unsigned saved_w,saved_h,valid;
static uint64_t saved_generation;
static unsigned long long captures,holds,blank_holds;
static const char *report_path;
static void report(void){if(report_path){FILE*f=fopen(report_path,"w");if(f){fprintf(f,"{\"captures\":%llu,\"held_fields\":%llu,\"blank_fields\":%llu}\n",captures,holds,blank_holds);fclose(f);}}}
void LagoonTransition_Reset(void){valid=0;}
int LagoonTransition_Apply(uint8_t *dst,size_t pitch,unsigned w,unsigned h,
    uint64_t generation,int coherent,int loading,unsigned brightness){
  static int initialized;if(!initialized){initialized=1;report_path=getenv("LAGOON_TRANSITION_REPORT");if(report_path&&*report_path)atexit(report);}
  if(!dst||w<256||w>MAX_WIDTH||!h||h>MAX_HEIGHT||pitch<w*4)return 0;
  if(saved_generation!=generation){valid=0;saved_generation=generation;}
  if(coherent){
    if(!loading&&brightness==15){
      for(unsigned y=0;y<h;y++)memcpy(pixels+y*w,dst+y*pitch,w*4);
      saved_w=w;saved_h=h;valid=1;captures++;
    }return 0;
  }
  if(!loading||!valid||saved_w!=w||saved_h!=h)return 0;
  unsigned level=brightness&128?0:brightness&15;holds++;if(!level)blank_holds++;
  for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
    uint32_t c=pixels[y*w+x];
    unsigned b=(c&255)*level/15,g=((c>>8)&255)*level/15,r=((c>>16)&255)*level/15;
    uint32_t out=(c&0xff000000u)|(r<<16)|(g<<8)|b;memcpy(dst+y*pitch+x*4,&out,4);
  }
  return 1;
}
