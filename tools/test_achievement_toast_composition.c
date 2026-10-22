/* UI fixture, not a live unlock: draw the production ToastImage on an external
 * native P6 scene using main.c's actual top-right anchor. P6 goes to stdout;
 * the compact verification summary goes to stderr. Never writes progress. */
#include "lagoon_achievement_ui.h"
#include "lagoon_achievements.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

static const LagoonAchievementRow earned={"story.finished","A World Beyond Darkness",
  "Complete the final ending.",1,1,1700000000,false};
size_t LagoonAchievements_Count(void){return 1;}
bool LagoonAchievements_GetRow(size_t index,LagoonAchievementRow *out){
  if(index||!out)return false;*out=earned;return true;
}
size_t LagoonAchievements_Unlocked(void){return 1;}
const char *LagoonAchievements_Status(void){return "UI fixture - no live unlock";}

int main(int argc,char **argv){
  unsigned char progress_before[sizeof earned];memcpy(progress_before,&earned,sizeof earned);
  assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
  char magic[3]={0};int width,height,max;
  assert(fscanf(f,"%2s%d%d%d",magic,&width,&height,&max)==4);
  assert(!strcmp(magic,"P6")&&width>=148&&width<=2048&&height>=36&&height<=1024&&max==255);
  int delimiter=fgetc(f);assert(delimiter=='\n'||delimiter==' '||delimiter=='\r');
  if(delimiter=='\r'){assert(fgetc(f)=='\n');}
  size_t count=(size_t)width*height;uint32_t *pixels=malloc(count*4),*original=malloc(count*4);
  assert(pixels&&original);
  for(size_t i=0;i<count;i++){
    unsigned char rgb[3];assert(fread(rgb,1,3,f)==3);
    pixels[i]=0xff000000u|(uint32_t)rgb[0]<<16|(uint32_t)rgb[1]<<8|rgb[2];
  }
  assert(fgetc(f)==EOF);fclose(f);memcpy(original,pixels,count*4);
  LagoonAchievementUI_Reset();LagoonAchievementUI_StartToast(0,100);
  const uint32_t *image;int iw,ih;
  assert(LagoonAchievementUI_ToastImage(&image,&iw,&ih)&&iw==144&&ih==32);
  /* Exact host anchor and opaque row copies, not an independently enlarged UI. */
  int x=width-iw-4,y=4;if(x<0)x=0;
  assert(x>=0&&y>=0&&x+iw<=width&&y+ih<=height);
  for(int row=0;row<ih;row++)memcpy(pixels+(size_t)(y+row)*width+x,image+(size_t)row*iw,(size_t)iw*4);
  unsigned changed=0;
  for(int py=0;py<height;py++)for(int px=0;px<width;px++){
    size_t at=(size_t)py*width+px;int inside=px>=x&&px<x+iw&&py>=y&&py<y+ih;
    if(!inside)assert(pixels[at]==original[at]);else assert(pixels[at]==image[(py-y)*iw+px-x]);
    changed+=pixels[at]!=original[at];
  }
  assert(changed>0&&changed<=(unsigned)(iw*ih));
  assert(!memcmp(&earned,progress_before,sizeof earned));
#ifdef _WIN32
  assert(_setmode(_fileno(stdout),_O_BINARY)!=-1);
#endif
  printf("P6\n%d %d\n255\n",width,height);
  for(size_t i=0;i<count;i++){
    unsigned char rgb[]={(unsigned char)(pixels[i]>>16),(unsigned char)(pixels[i]>>8),(unsigned char)pixels[i]};
    assert(fwrite(rgb,1,3,stdout)==3);
  }
  fprintf(stderr,"UI fixture PASS: field%d x%d toast%d x%d anchor%d,%d changed%u outside_changes0\n",
    width,height,iw,ih,x,y,changed);
  free(original);free(pixels);return 0;
}
