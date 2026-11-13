#include "../src/lagoon_zoom_controls.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 LagoonZoomButton key={0};int activate=-1;
 assert(LagoonZoomButton_Update(&key,1,1,1,&activate)&&activate);
 assert(LagoonZoomButton_Update(&key,1,1,1,&activate)&&!activate);
 assert(LagoonZoomButton_Update(&key,0,0,1,&activate)&&!activate);
 assert(!LagoonZoomButton_Update(&key,1,0,1,&activate)&&!activate);
 assert(!LagoonZoomButton_Update(&key,1,1,1,&activate)&&!activate);
 assert(!LagoonZoomButton_Update(&key,0,0,1,&activate)&&!activate);
 assert(LagoonZoomButton_Update(&key,1,1,0,&activate)&&!activate);
 assert(LagoonZoomButton_Update(&key,1,1,1,&activate)&&!activate);
 assert(LagoonZoomButton_Update(&key,0,0,1,&activate)&&!activate);
 assert(LagoonZoomButton_Update(&key,1,1,1,&activate)&&activate);
 assert(LagoonZoom_Next(100)==125&&LagoonZoom_Next(125)==150&&LagoonZoom_Next(150)==100);
 puts("PASS zoom hotkey edge state: repeat, modifier release order, ordinary Z, modal/nonlive gating and100/125/150 cycle");return 0;
}
