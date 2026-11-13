/* ROM-free rendering invariants for refused transition fields, not campaign proof. */
#include "../src/lagoon_zoom_placement.h"
#include <assert.h>
#include <stdio.h>
static uint8_t source[(400*4+16)*224],dest[(600*4+16)*336+16];
static uint32_t pixel(unsigned x,unsigned y){uint32_t c;memcpy(&c,dest+y*(600*4+16)+x*4,4);return c;}
int main(void){
 const unsigned sp=400*4+16,pitch=600*4+16;
 for(unsigned y=0;y<224;y++)for(unsigned x=0;x<400;x++){
   uint32_t color=(x==72+30&&y>=20&&y<36)?0x00ff3366:0x00102030;
   if(y>=190)color=0x00fedcba; /* HUD must not become map-space imagery. */
   memcpy(source+y*sp+x*4,&color,4);
 }
 memset(dest,0xa5,sizeof dest);
 assert(LagoonZoomPlacement_CopyWorld(dest,pitch,600,336,source,sp,400,224,172,56,190));
 /* The one-pixel-wide16px marker must retain its exact footprint at1.5x;
  * the retired fallback would enlarge it into a24px shape. */
 unsigned count=0;
 for(unsigned y=0;y<336;y++)for(unsigned x=0;x<600;x++)if(pixel(x,y)==0x00ff3366)count++;
 assert(count==16&&pixel(202,76)==0x00ff3366&&pixel(203,76)!=0x00ff3366);
 assert(pixel(202,92)!=0x00ff3366&&pixel(300,300)==0);
 assert(pixel(300,246)==0); /* no source HUD copied into the world */
 for(unsigned y=0;y<336;y++)for(unsigned x=600*4;x<pitch;x++)assert(dest[y*pitch+x]==0xa5);
 assert(dest[pitch*336]==0xa5);
 /* Edge-clamped native windows can start outside the expanded canvas. */
 assert(LagoonZoomPlacement_CopyWorld(dest,pitch,600,336,source,sp,400,224,0,0,190));
 assert(pixel(30,20)==0x00ff3366&&pixel(0,0)==0x00102030);
 assert(!LagoonZoomPlacement_CopyWorld(dest,16,600,336,source,sp,400,224,0,0,190));
 assert(!LagoonZoomPlacement_CopyWorld(dest,pitch,300,336,source,sp,400,224,0,0,190));
 puts("PASS: transition pixel scale, world origin, HUD exclusion, edge clipping and padded-pitch canaries");return 0;
}
