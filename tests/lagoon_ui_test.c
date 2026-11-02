#include "../src/lagoon_ui.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

int main(void){
  enum{W=176,H=48};uint32_t buffer[W*H+2],capital[W*H];
  buffer[0]=buffer[W*H+1]=0x12345678;uint32_t *p=buffer+1;
  for(int i=0;i<W*H;i++)p[i]=LAGOON_UI_BG;
  LagoonUI_Fill(p,W,H,INT_MIN,INT_MIN,4,4,LAGOON_UI_FG);
  LagoonUI_Frame(p,W,H,-7,-8,80,60);
  LagoonUI_Text(p,W,H,-3,-4,"Map\nAtland",LAGOON_UI_FG,1);
  LagoonUI_TextSmall(p,W,H,W-4,H-6,"Clipped",LAGOON_UI_FG);
  assert(buffer[0]==0x12345678&&buffer[W*H+1]==0x12345678);
  for(int i=0;i<W*H;i++)p[i]=LAGOON_UI_BG;
  LagoonUI_Text(p,W,H,0,0,"Atland",LAGOON_UI_FG,1);memcpy(capital,p,sizeof capital);
  for(int i=0;i<W*H;i++)p[i]=LAGOON_UI_BG;
  LagoonUI_Text(p,W,H,0,0,"ATLAND",LAGOON_UI_FG,1);
  assert(memcmp(p,capital,sizeof capital)); /* real lowercase, no forced caps */
  assert(LagoonUI_TextWidth("Atland",2)==2*LagoonUI_TextWidth("Atland",1));
  assert(LagoonUI_TextWidth("iii",1)<LagoonUI_TextWidth("WWW",1));
  assert(LagoonUI_TextWidth("Atland\nGold Cave",1)==LagoonUI_TextWidth("Gold Cave",1));
  assert(LagoonUI_TextWidth(NULL,1)==0&&LagoonUI_TextWidth("x",0)==0);
  for(int i=0;i<W*H;i++)p[i]=LAGOON_UI_BG;
  LagoonUI_TextFit(p,W,H,4,4,"A long title which cannot overflow into another column",LAGOON_UI_FG,1,64);
  for(int y=0;y<H;y++)for(int x=70;x<W;x++)assert(p[y*W+x]==LAGOON_UI_BG);
  assert(buffer[0]==0x12345678&&buffer[W*H+1]==0x12345678);
  puts("PASS shared UI: clipped negative/extreme/offscreen rectangles and glyphs preserve canaries; lowercase/proportional metrics, scaling/newlines and elision respect available columns");
  return 0;
}
