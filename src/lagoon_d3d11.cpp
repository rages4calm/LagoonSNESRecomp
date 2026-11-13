/* Windows-only windowed flip presenter. SDL owns the window/input, never an
 * exclusive DXGI output. Guest pixels remain ordinary opaque BGRA uploads. */
#include "lagoon_d3d11.h"
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <vector>
#include <string>
#include <cstdio>
#include <cstring>
extern "C" {
#include "desktop/sdl_compat.h"
#include "desktop/config.h"
#include "util.h"
}
#if !SNESRECOMP_SDL3
#include <SDL_syswm.h>
#endif
namespace {
ID3D11Device *device;
ID3D11DeviceContext *context;
IDXGISwapChain1 *swapchain;
ID3D11RenderTargetView *target;
ID3D11Texture2D *texture;
ID3D11ShaderResourceView *resource;
ID3D11VertexShader *vertex;
ID3D11PixelShader *pixel;
ID3D11Buffer *constants;
SDL_Window *window;
int output_w,output_h,source_w,source_h,texture_w,texture_h;
bool title_ui,lost;
std::vector<uint32_t> surface;
std::string capture;
template<class T> void release(T *&p){if(p){p->Release();p=nullptr;}}
bool check(HRESULT h,const char *operation){
  if(SUCCEEDED(h))return true;
  std::fprintf(stderr,"[d3d11] %s failed: 0x%08lx\n",operation,(unsigned long)h);return false;
}
const char shader[]=R"(
cbuffer Frame:register(b0){int width;int height;int enhanced;int hud;float2 footprint;float2 padding;};
Texture2D<float4> image:register(t0);
struct V{float4 position:SV_POSITION;float2 uv:TEXCOORD0;};
V vs(uint id:SV_VertexID){V v;v.uv=float2((id<<1)&2,id&2);v.position=float4(v.uv*float2(2,-2)+float2(-1,1),0,1);return v;}
float3 load(int2 p){return image.Load(int3(clamp(p,int2(0,0),int2(width-1,height-1)),0)).rgb;}
)"
#include "lagoon_mmpx_shader.inc"
R"(
float3 epx(int2 halfcell){
 halfcell=clamp(halfcell,int2(0,0),int2(width*2-1,(height-hud)*2-1));
 float2 p=(float2(halfcell)+0.5)*0.5;int2 q=int2(p);float3 e=load(q);
 if(enhanced!=0&&q.y<height-hud){
  float3 b=load(q+int2(0,-1)),h=load(int2(q.x,min(q.y+1,height-hud-1))),d=load(q+int2(-1,0)),f=load(q+int2(1,0));
  if(any(b!=h)&&any(d!=f)){
   bool right=frac(p.x)>=0.5,down=frac(p.y)>=0.5;
   if(!down&&!right&&all(d==b))e=d;
   if(!down&&right&&all(b==f))e=f;
   if(down&&!right&&all(d==h))e=d;
   if(down&&right&&all(h==f))e=f;
  }
 }
 return e;
}
float4 ps(V v):SV_TARGET{
 float2 p=v.uv*float2(width,height);int2 q=int2(p+0.00001);
 float2 grid=p*2.0,span=max(footprint,float2(0.00001,0.00001));
 if(enhanced==0||q.y>=height-hud)return float4(load(q),1);
 // Reconstruct EPX's 2x lattice, then integrate only its one-output-pixel
 // edge footprint. Integer-aligned pixels remain exact; fractional zoom no
 // longer alternates between visibly uneven pixel widths. No frame history.
 float2 center=floor(grid)+0.5;
 float2 at=center+clamp((grid-center)/span,-0.5,0.5)-0.5;
 int2 lo=int2(floor(at));float2 blend=frac(at);
 float3 a=enhanced==2?lerp(mmpx(lo),mmpx(lo+int2(1,0)),blend.x):lerp(epx(lo),epx(lo+int2(1,0)),blend.x);
 float3 b=enhanced==2?lerp(mmpx(lo+int2(0,1)),mmpx(lo+int2(1,1)),blend.x):lerp(epx(lo+int2(0,1)),epx(lo+int2(1,1)),blend.x);
 return float4(lerp(a,b,blend.y),1);
})";
void destroy(){
  if(context){context->ClearState();context->Flush();}
  release(resource);release(texture);release(constants);release(pixel);release(vertex);
  release(target);release(swapchain);release(context);release(device);
  texture_w=texture_h=output_w=output_h=0;
}
bool initialize(SDL_Window *w){
  window=w;lost=false;
#if SNESRECOMP_SDL3
  HWND hwnd=(HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(w),SDL_PROP_WINDOW_WIN32_HWND_POINTER,nullptr);
#else
  SDL_SysWMinfo info;SDL_VERSION(&info.version);
  if(!SDL_GetWindowWMInfo(w,&info))return false;
  HWND hwnd=info.info.win.window;
#endif
  if(!hwnd)return false;
  D3D_FEATURE_LEVEL requested[]={D3D_FEATURE_LEVEL_11_0};
  HRESULT h=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,
      requested,1,D3D11_SDK_VERSION,&device,nullptr,&context);
  bool warp=FAILED(h);
  if(warp){
    std::fprintf(stderr,"[d3d11] hardware unavailable; trying the Windows WARP device\n");
    h=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        requested,1,D3D11_SDK_VERSION,&device,nullptr,&context);
  }
  if(!check(h,"create hardware device")){destroy();return false;}
  IDXGIDevice *dxdevice=nullptr;IDXGIAdapter *adapter=nullptr;IDXGIFactory2 *factory=nullptr;
  h=device->QueryInterface(__uuidof(IDXGIDevice),(void**)&dxdevice);
  if(SUCCEEDED(h))h=dxdevice->GetAdapter(&adapter);
  if(SUCCEEDED(h))h=adapter->GetParent(__uuidof(IDXGIFactory2),(void**)&factory);
  DXGI_SWAP_CHAIN_DESC1 desc={};desc.Width=1;desc.Height=1;desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
  desc.SampleDesc.Count=1;desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;desc.BufferCount=2;
  desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;desc.Scaling=DXGI_SCALING_STRETCH;
  desc.AlphaMode=DXGI_ALPHA_MODE_IGNORE;
  if(SUCCEEDED(h))h=factory->CreateSwapChainForHwnd(device,hwnd,&desc,nullptr,nullptr,&swapchain);
  if(factory)factory->MakeWindowAssociation(hwnd,DXGI_MWA_NO_ALT_ENTER);
  release(factory);release(adapter);release(dxdevice);
  if(!check(h,"create windowed flip swapchain")){destroy();return false;}
  ID3DBlob *vs=nullptr,*ps=nullptr,*error=nullptr;
  h=D3DCompile(shader,sizeof(shader)-1,"lagoon-epx",nullptr,nullptr,"vs","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&vs,&error);
  if(error){std::fprintf(stderr,"%s",(char*)error->GetBufferPointer());release(error);}
  if(SUCCEEDED(h))h=D3DCompile(shader,sizeof(shader)-1,"lagoon-epx",nullptr,nullptr,"ps","ps_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&ps,&error);
  if(error){std::fprintf(stderr,"%s",(char*)error->GetBufferPointer());release(error);}
  if(SUCCEEDED(h))h=device->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&vertex);
  if(SUCCEEDED(h))h=device->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&pixel);
  release(vs);release(ps);
  D3D11_BUFFER_DESC cb={};cb.ByteWidth=32;cb.Usage=D3D11_USAGE_DEFAULT;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
  if(SUCCEEDED(h))h=device->CreateBuffer(&cb,nullptr,&constants);
  if(!check(h,"compile/create EPX pipeline")){destroy();return false;}
  std::fprintf(stderr,"[d3d11] windowed flip-discard, %s feature level 11.0, GPU EPX available\n",warp?"WARP":"hardware");
  return true;
}
void size(int *w,int *h){
#if SNESRECOMP_SDL3
  SDL_GetWindowSizeInPixels(window,w,h);
#else
  SDL_GetWindowSize(window,w,h);
#endif
}
void begin(int w,int h,uint8 **p,int *pitch){
  *p=nullptr;*pitch=0;
  if(w<1||h<1||w>2048||h>1024)return;
  if(lost){destroy();if(!initialize(window))return;}
  source_w=w;source_h=h;surface.resize((size_t)w*h);
  *p=(uint8*)surface.data();*pitch=w*4;
}
void screenshot(){
  if(capture.empty())return;
  ID3D11Texture2D *back=nullptr,*staging=nullptr;
  HRESULT h=swapchain->GetBuffer(0,__uuidof(ID3D11Texture2D),(void**)&back);
  D3D11_TEXTURE2D_DESC desc={};if(SUCCEEDED(h)){
    back->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;desc.MiscFlags=0;
    h=device->CreateTexture2D(&desc,nullptr,&staging);
  }
  if(SUCCEEDED(h)){
    context->CopyResource(staging,back);D3D11_MAPPED_SUBRESOURCE mapped;
    h=context->Map(staging,0,D3D11_MAP_READ,0,&mapped);
    if(SUCCEEDED(h)){
#if SNESRECOMP_SDL3
      SDL_Surface *s=SDL_CreateSurfaceFrom(output_w,output_h,SDL_PIXELFORMAT_ARGB8888,mapped.pData,mapped.RowPitch);
      if(s){SDL_SaveBMP(s,capture.c_str());SDL_DestroySurface(s);}
#else
      SDL_Surface *s=SDL_CreateRGBSurfaceWithFormatFrom(mapped.pData,output_w,output_h,32,mapped.RowPitch,SDL_PIXELFORMAT_ARGB8888);
      if(s){SDL_SaveBMP(s,capture.c_str());SDL_FreeSurface(s);}
#endif
      context->Unmap(staging,0);
    }
  }
  check(h,"capture display");release(staging);release(back);capture.clear();
}
void end(){
  if(!device||surface.empty())return;
  int w=0,h=0;size(&w,&h);if(w<=0||h<=0)return;
  HRESULT hr=S_OK;
  if(w!=output_w||h!=output_h){
    context->OMSetRenderTargets(0,nullptr,nullptr);release(target);
    hr=swapchain->ResizeBuffers(0,w,h,DXGI_FORMAT_UNKNOWN,0);
    ID3D11Texture2D *back=nullptr;
    if(SUCCEEDED(hr))hr=swapchain->GetBuffer(0,__uuidof(ID3D11Texture2D),(void**)&back);
    if(SUCCEEDED(hr))hr=device->CreateRenderTargetView(back,nullptr,&target);
    release(back);if(!check(hr,"resize")){lost=true;return;}output_w=w;output_h=h;
  }
  if(source_w!=texture_w||source_h!=texture_h){
    ID3D11ShaderResourceView *none=nullptr;context->PSSetShaderResources(0,1,&none);
    release(resource);release(texture);
    D3D11_TEXTURE2D_DESC desc={};desc.Width=source_w;desc.Height=source_h;desc.MipLevels=desc.ArraySize=1;
    desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DYNAMIC;
    desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;desc.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
    hr=device->CreateTexture2D(&desc,nullptr,&texture);
    if(SUCCEEDED(hr))hr=device->CreateShaderResourceView(texture,nullptr,&resource);
    if(!check(hr,"create upload texture")){lost=true;return;}texture_w=source_w;texture_h=source_h;
  }
  D3D11_MAPPED_SUBRESOURCE mapped;
  hr=context->Map(texture,0,D3D11_MAP_WRITE_DISCARD,0,&mapped);
  if(!check(hr,"upload map")){lost=true;return;}
  for(int y=0;y<source_h;y++)std::memcpy((char*)mapped.pData+y*mapped.RowPitch,surface.data()+(size_t)y*source_w,source_w*4);
  context->Unmap(texture,0);
  SnesDisplayViewport v;
  SnesDisplayAspect_ComputeViewport(source_w,source_h,w,h,
      title_ui?kSnesDisplayAspect_SquarePixels8x7:SnesDisplayAspect_Clamp(g_config.display_aspect),
      !title_ui&&g_config.ignore_aspect_ratio,false,&v);
  D3D11_VIEWPORT viewport={(float)v.x,(float)v.y,(float)v.width,(float)v.height,0,1};
  context->RSSetViewports(1,&viewport);context->OMSetRenderTargets(1,&target,nullptr);
  const float black[4]={0,0,0,1};context->ClearRenderTargetView(target,black);
  struct Frame {int width,height,enhanced,hud;float footprint[2],padding[2];};
  Frame data={source_w,source_h,!title_ui&&g_config.shader?(std::strstr(g_config.shader,"lagoon-mmpx")?2:std::strstr(g_config.shader,"lagoon-epx")?1:0):0,title_ui?0:34,
      {2.0f*source_w/v.width,2.0f*source_h/v.height},{0,0}};
  context->UpdateSubresource(constants,0,nullptr,&data,0,0);
  context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context->VSSetShader(vertex,nullptr,0);context->PSSetShader(pixel,nullptr,0);
  context->PSSetConstantBuffers(0,1,&constants);context->PSSetShaderResources(0,1,&resource);
  context->Draw(3,0);screenshot();
  /* Hidden qualification windows have no desktop scanout. Some drivers block
   * occluded Present for250ms even though GPU readback completed; do not turn
   * a frame-stepped render check into4fps. Visible game/OBS still use Present
   * and their configured vsync unchanged. */
  if(SDL_GetWindowFlags(window)&SDL_WINDOW_HIDDEN){context->Flush();return;}
  hr=swapchain->Present(g_config.vsync?1:0,0);
  if(FAILED(hr)){check(hr,"present");lost=true;}
}
void reconfigure(){} // constants, aspect and vsync are read each presentation.
}
extern "C" int LagoonD3D11_Create(const char *id,RendererFuncs *f){
  if(!id||std::strcmp(id,"direct3d11"))return 0;
  *f={initialize,destroy,size,begin,end,reconfigure};return 1;
}
extern "C" void LagoonD3D11_TitleUi(int active){title_ui=active!=0;}
extern "C" void LagoonD3D11_Capture(const char *path){if(path)capture=path;}
