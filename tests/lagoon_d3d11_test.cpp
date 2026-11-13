#ifdef NDEBUG
#undef NDEBUG
#endif
#include "lagoon_d3d11.h"
extern "C" {
#include "desktop/config.h"
#include "util.h"
Config g_config;
}
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <cmath>
#include <algorithm>
static uint32_t source[64*64];
static uint32_t sample(int x,int y){if(x<0)x=0;if(x>63)x=63;if(y<0)y=0;if(y>63)y=63;return source[y*64+x];}
static uint32_t expected(int x,int y,bool enhanced){
  int qx=x/2,qy=y/2;uint32_t e=sample(qx,qy),b=sample(qx,qy-1),h=sample(qx,qy<29?qy+1:29),d=sample(qx-1,qy),f=sample(qx+1,qy);
  if(enhanced&&qy<30&&b!=h&&d!=f){
    if(!(x&1)&&!(y&1)&&d==b)e=d;
    if((x&1)&&!(y&1)&&b==f)e=f;
    if(!(x&1)&&(y&1)&&d==h)e=d;
    if((x&1)&&(y&1)&&h==f)e=f;
  }return e&0xffffff;
}
static uint32_t fractional(int x,int y,int extent){
 float px=(x+0.5f)*128.0f/extent,py=(y+0.5f)*128.0f/extent;
 if(py>=60)return sample((int)(px*0.5f),(int)(py*0.5f))&0xffffff;
 float span=128.0f/extent;
 auto position=[&](float p){float c=std::floor(p)+0.5f;return c+std::clamp((p-c)/span,-0.5f,0.5f)-0.5f;};
 float ax=position(px),ay=position(py);int ix=(int)std::floor(ax),iy=(int)std::floor(ay);float fx=ax-ix,fy=ay-iy;
 auto at=[&](int xx,int yy){return expected(std::clamp(xx,0,127),std::clamp(yy,0,59),true);};
 uint32_t a=at(ix,iy),b=at(ix+1,iy),c=at(ix,iy+1),d=at(ix+1,iy+1),result=0;
 for(int shift=0;shift<24;shift+=8){float top=((a>>shift)&255)*(1-fx)+((b>>shift)&255)*fx;
 float bottom=((c>>shift)&255)*(1-fx)+((d>>shift)&255)*fx;
 result|=(uint32_t)std::lround(top*(1-fy)+bottom*fy)<<shift;}
 return result;
}
int main(int argc,char **argv){
  assert(argc==2);assert(SDL_Init(SDL_INIT_VIDEO));
  SDL_Window *w=SDL_CreateWindow("Lagoon D3D11 vectors",128,128,SDL_WINDOW_HIDDEN);assert(w);
  g_config.display_aspect=kSnesDisplayAspect_SquarePixels8x7;g_config.shader="lagoon-epx";
  RendererFuncs renderer;assert(LagoonD3D11_Create("direct3d11",&renderer));assert(renderer.Initialize(w));
  const uint32_t palette[]={0x00225499,0x00bb6633,0x00ffffdd,0x00336622};
  unsigned rng=12345;for(unsigned i=0;i<4096;i++){rng=rng*1664525+1013904223;source[i]=palette[rng>>30];}
  unsigned changed=0;
  for(int mode=0;mode<4;mode++){
    int hh=mode==3?150:128;int ww=mode==3?174:mode==2?192:128;if(mode>=2)assert(SDL_SetWindowSize(w,ww,hh));
    SDL_PumpEvents();uint8 *p;int pitch;LagoonD3D11_TitleUi(mode==1);
    renderer.BeginDraw(64,64,&p,&pitch);assert(p);
    for(int y=0;y<64;y++)memcpy(p+y*pitch,source+y*64,256);
    std::string path=std::string(argv[1])+"/gpu-"+std::to_string(mode)+".bmp";
    LagoonD3D11_Capture(path.c_str());renderer.EndDraw();
    SDL_Surface *raw=SDL_LoadBMP(path.c_str());assert(raw);
    SDL_Surface *image=SDL_ConvertSurface(raw,SDL_PIXELFORMAT_ARGB8888);assert(image);
    assert(image->w==ww&&image->h==hh);
    for(int y=0;y<hh;y++)for(int x=0;x<ww;x++){
      uint32_t pixel;memcpy(&pixel,(char*)image->pixels+y*image->pitch+x*4,4);
      int xx=x-(ww-hh)/2;
      uint32_t want=xx<0||xx>=hh?0:mode==3?fractional(xx,y,hh):expected(xx,y,mode!=1);
      bool differs=false;for(int shift=0;shift<24;shift+=8)if(std::abs(int((pixel>>shift)&255)-int((want>>shift)&255))>(mode==3?1:0))differs=true;
      if(differs){fprintf(stderr,"GPU mismatch mode=%d x=%d y=%d actual=%06x expected=%06x\n",mode,x,y,pixel&0xffffff,want);return 2;}
      if(mode==0&&want!=(sample(x/2,y/2)&0xffffff))changed++;
    }
    SDL_DestroySurface(image);SDL_DestroySurface(raw);
  }
  assert(changed>100);renderer.Destroy();SDL_DestroyWindow(w);SDL_Quit();
  printf("PASS D3D11 GPU EPX exact pixels, HUD bypass, UI bypass, zero-alpha opaque input, resize/letterbox, fractional sharp reconstruction (%u enhanced samples)\n",changed);
}
