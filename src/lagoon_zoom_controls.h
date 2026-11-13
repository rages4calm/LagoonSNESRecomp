#ifndef LAGOON_ZOOM_CONTROLS_H
#define LAGOON_ZOOM_CONTROLS_H
/* Host event edge state, independent of SNES buttons and guest simulation. */
typedef struct LagoonZoomButton {unsigned held,consumed;} LagoonZoomButton;
static int LagoonZoomButton_Update(LagoonZoomButton *state,int pressed,int reserved,int eligible,int *activate){
  *activate=0;
  if(!pressed){int consumed=(int)state->consumed;state->held=state->consumed=0;return consumed;}
  if(!state->held){state->held=1;state->consumed=reserved!=0;*activate=state->consumed&&eligible;}
  return (int)state->consumed;
}
static unsigned LagoonZoom_Next(unsigned current){return current==100?125:current==125?150:100;}
#endif
