/* Shared title-owned host UI. All drawing is clipped to its destination;
 * it has no game-memory, input, device, file, font-cache or storage access. */
#include "lagoon_ui.h"
#include "lagoon_ui_font.generated.h"
#include <stddef.h>
#include <string.h>
#include <limits.h>

void LagoonUI_Fill(uint32_t *dst,int width,int height,int x,int y,int w,int h,uint32_t colour){
  if(!dst||width<=0||height<=0||w<=0||h<=0)return;
  int64_t x1=(int64_t)x+w,y1=(int64_t)y+h;
  if(x<0)x=0;if(y<0)y=0;if(x1>width)x1=width;if(y1>height)y1=height;
  for(int yy=y;yy<y1;yy++)for(int xx=x;xx<x1;xx++)dst[(size_t)yy*width+xx]=colour;
}
static void outline(uint32_t *d,int sw,int sh,int x,int y,int w,int h,uint32_t c){
  LagoonUI_Fill(d,sw,sh,x,y,w,1,c);LagoonUI_Fill(d,sw,sh,x,y+h-1,w,1,c);
  LagoonUI_Fill(d,sw,sh,x,y,1,h,c);LagoonUI_Fill(d,sw,sh,x+w-1,y,1,h,c);
}
void LagoonUI_Frame(uint32_t *d,int sw,int sh,int x,int y,int w,int h){
  if(w<12||h<12)return;
  LagoonUI_Fill(d,sw,sh,x,y,w,h,LAGOON_UI_BG);
  outline(d,sw,sh,x+1,y+1,w-2,h-2,0xff6e7076);
  outline(d,sw,sh,x+2,y+2,w-4,h-4,LAGOON_UI_FG);
  outline(d,sw,sh,x+3,y+3,w-6,h-6,LAGOON_UI_FG);
  outline(d,sw,sh,x+5,y+5,w-10,h-10,0xff77766e);
}
void LagoonUI_Cursor(uint32_t *d,int sw,int sh,int x,int y,uint32_t c){
  for(int row=0;row<11;row++){int n=row<=5?row+1:11-row;LagoonUI_Fill(d,sw,sh,x,y+row,n,1,c);}
}
static unsigned glyph(unsigned char c){return c>=32&&c<=126?c-32:'?'-32;}
static int measure(const char *s,int scale,int small){
  if(!s||scale<1||scale>8)return 0;int64_t line=0,longest=0;
  for(;*s;s++){if(*s=='\n'){if(line>longest)longest=line;line=0;}else line+=(small?ui_small_advance:ui_body_advance)[glyph((unsigned char)*s)]*scale;}
  if(line>longest)longest=line;return longest>INT_MAX?INT_MAX:(int)longest;
}
int LagoonUI_TextWidth(const char *s,int scale){return measure(s,scale,0);}
int LagoonUI_SmallWidth(const char *s){return measure(s,1,1);}
static uint32_t blend(uint32_t a,uint32_t b,unsigned coverage){
  unsigned r=(((a>>16)&255)*(255-coverage)+((b>>16)&255)*coverage+127)/255;
  unsigned g=(((a>>8)&255)*(255-coverage)+((b>>8)&255)*coverage+127)/255;
  unsigned bl=((a&255)*(255-coverage)+(b&255)*coverage+127)/255;
  return 0xff000000u|(r<<16)|(g<<8)|bl;
}
static void draw(uint32_t *d,int sw,int sh,int x,int y,const char *s,uint32_t c,int scale,int small){
  if(!d||!s||sw<=0||sh<=0||scale<1||scale>8)return;
  int first=x,gw=small?16:20,gh=small?12:20;
  for(;*s;s++){
    if(*s=='\n'){x=first;y+=gh*scale;continue;}
    unsigned g=glyph((unsigned char)*s);const unsigned char *a=small?ui_small_alpha[g]:ui_body_alpha[g];
    for(int yy=0;yy<gh;yy++)for(int xx=0;xx<gw;xx++){
      unsigned at=(unsigned)(yy*gw+xx),v=a[at/2];v=((at&1)?v&15:v>>4)*17;if(!v)continue;
      for(int dy=0;dy<scale;dy++)for(int dx=0;dx<scale;dx++){
        int64_t px=(int64_t)x+(xx-2)*scale+dx,py=(int64_t)y+yy*scale+dy;
        if(px>=0&&px<sw&&py>=0&&py<sh){size_t p=(size_t)py*sw+(size_t)px;d[p]=blend(d[p],c,v);}
      }
    }
    int step=(small?ui_small_advance:ui_body_advance)[g]*scale;
    if(x>INT_MAX-step)break;x+=step;
  }
}
void LagoonUI_Text(uint32_t *d,int w,int h,int x,int y,const char *s,uint32_t c,int scale){draw(d,w,h,x,y,s,c,scale,0);}
void LagoonUI_TextSmall(uint32_t *d,int w,int h,int x,int y,const char *s,uint32_t c){draw(d,w,h,x,y,s,c,1,1);}
static void fit(uint32_t *d,int w,int h,int x,int y,const char *s,uint32_t c,int scale,int maxw,int small){
  if(!s||maxw<=0)return;
  if(measure(s,scale,small)<=maxw){draw(d,w,h,x,y,s,c,scale,small);return;}
  char text[256];size_t n=0;int used=0,dots=measure("...",scale,small);
  if(dots>maxw)return;
  while(s[n]&&s[n]!='\n'&&n<sizeof text-4){int step=(small?ui_small_advance:ui_body_advance)[glyph((unsigned char)s[n])]*scale;if(used+step+dots>maxw)break;text[n]=s[n];used+=step;n++;}
  memcpy(text+n,"...",4);draw(d,w,h,x,y,text,c,scale,small);
}
void LagoonUI_TextFit(uint32_t *d,int w,int h,int x,int y,const char *s,uint32_t c,int scale,int maxw){fit(d,w,h,x,y,s,c,scale,maxw,0);}
void LagoonUI_SmallFit(uint32_t *d,int w,int h,int x,int y,const char *s,uint32_t c,int maxw){fit(d,w,h,x,y,s,c,1,maxw,1);}
