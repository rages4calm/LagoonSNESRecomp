#include "lagoon_transition_present.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
enum { MAX_WIDTH=800,MAX_HEIGHT=224,TRACE_CAP=512 };
static uint32_t pixels[MAX_WIDTH*MAX_HEIGHT];
static unsigned saved_w,saved_h,valid,saved_room,saved_frame;
static uint64_t saved_generation;
static unsigned fade_floor=15;
static unsigned long long captures,holds,blank_holds,preloader_holds,floor_holds,incoming_retirements;
static const char *report_path;
typedef struct TransitionTrace {
  unsigned frame,room,saved_room,loading,coherent,brightness,level,floor,event;
} TransitionTrace;
static TransitionTrace trace_rows[TRACE_CAP];
static unsigned trace_count,trace_truncated;
static void report(void){
  if(!report_path)return;
  FILE*f=fopen(report_path,"w");if(!f)return;
  fprintf(f,"{\"captures\":%llu,\"held_fields\":%llu,\"blank_fields\":%llu,\"preloader_fields\":%llu,"
      "\"floor_clamped_fields\":%llu,\"incoming_retirements\":%llu,\"trace_truncated\":%u,\"phase_trace\":[",
      captures,holds,blank_holds,preloader_holds,floor_holds,incoming_retirements,trace_truncated);
  static const char*events[]={"coherent","hold","no-cache","preloader","incoming"};
  for(unsigned i=0;i<trace_count;i++){
    const TransitionTrace*t=&trace_rows[i];
    fprintf(f,"%s{\"frame\":%u,\"room\":%u,\"saved_room\":%u,\"loading\":%u,\"coherent\":%u,"
        "\"brightness\":%u,\"level\":%u,\"floor\":%u,\"event\":\"%s\"}",i?",":"",t->frame,t->room,
        t->saved_room,t->loading,t->coherent,t->brightness,t->level,t->floor,events[t->event]);
  }
  fputs("]}\n",f);fclose(f);
}
static void initialize(void){
  static int initialized;if(initialized)return;initialized=1;
  report_path=getenv("LAGOON_TRANSITION_REPORT");
  if(report_path&&*report_path)atexit(report);else report_path=NULL;
}
static void trace(unsigned frame,unsigned room,unsigned loading,unsigned coherent,
    unsigned brightness,unsigned level,unsigned event){
  if(!report_path)return;
  if(trace_count==TRACE_CAP){trace_truncated=1;return;}
  trace_rows[trace_count++]=(TransitionTrace){frame,room,saved_room,loading,coherent,
      brightness,level,fade_floor,event};
}
void LagoonTransition_Reset(void){valid=0;fade_floor=15;}
void LagoonTransition_ObserveIncoming(uint64_t generation,unsigned frame){
  initialize();
  /* Only the title's independently proven incoming native source may call
   * this. A later refused packet must never resurrect the outgoing canvas. */
  if(valid&&saved_generation==generation)incoming_retirements++;
  valid=0;saved_generation=generation;fade_floor=15;
  trace(frame,0,1,1,0,0,4);
}
int LagoonTransition_Apply(uint8_t *dst,size_t pitch,unsigned w,unsigned h,
    uint64_t generation,int coherent,int loading,unsigned brightness,unsigned room_key,
    unsigned frame){
  initialize();
  if(!dst||w<256||w>MAX_WIDTH||!h||h>MAX_HEIGHT||pitch<w*4)return 0;
  if(saved_generation!=generation){valid=0;saved_generation=generation;fade_floor=15;}
  unsigned level=brightness&128?0:brightness&15;
  /* Observe the original outgoing fade even while its current raster is
   * coherent. The loader later raises INIDISP while its new maps/characters
   * are incomplete; that register bounce cannot brighten the old room again.
   * A forced blank therefore owns all subsequent incoherent loader fields
   * until an independently qualified incoming presentation retires this cache. */
  int preloader=!loading&&!coherent&&valid&&saved_w==w&&saved_h==h&&
      saved_room&&room_key&&saved_room!=room_key&&(unsigned)(frame-saved_frame)==1;
  if(valid&&saved_w==w&&saved_h==h&&(loading||preloader)&&level<fade_floor)fade_floor=level;
  if(coherent){
    if(!loading&&brightness==15){
      for(unsigned y=0;y<h;y++)memcpy(pixels+y*w,dst+y*pitch,w*4);
      saved_w=w;saved_h=h;saved_room=room_key;saved_frame=frame;valid=1;fade_floor=15;captures++;
    }
    if(loading)trace(frame,room_key,1,1,brightness,level,0);
    return 0;
  }
  if(!valid||saved_w!=w||saved_h!=h){
    if(loading)trace(frame,room_key,1,0,brightness,level,2);
    return 0;
  }
  /* Original doorway scripts replace the area descriptor and clear actors
   * one field before $100E is set. The completed old canvas is still the
   * correct outgoing presentation during that boundary. Allow exactly one
   * such field, and only between two independently qualified authored rooms.
   * Same-room UI, unsupported descriptors and prolonged non-loader failures
   * cannot borrow this exception. */
  if(!loading){
    if(!preloader)return 0;
    preloader_holds++;
  }else if(level>fade_floor){level=fade_floor;floor_holds++;}
  holds++;if(!level)blank_holds++;
  trace(frame,room_key,!!loading,0,brightness,level,loading?1:3);
  for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
    uint32_t c=pixels[y*w+x];
    unsigned b=(c&255)*level/15,g=((c>>8)&255)*level/15,r=((c>>16)&255)*level/15;
    uint32_t out=(c&0xff000000u)|(r<<16)|(g<<8)|b;memcpy(dst+y*pitch+x*4,&out,4);
  }
  return 1;
}
