/* Actual diagnostic/EndDraw source with a deterministic clock and GL call log. */
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char uint8;
typedef struct {int x,y,width,height;} SnesDisplayViewport;
typedef struct {unsigned gl_texture;int width,height;} GlTextureWithSize;
typedef void GlslShader;
static void *g_window;
static int g_draw_width=448,g_draw_height=224;
static GlTextureWithSize g_texture={1,448,224};
static GlslShader *g_glsl_shader;
static unsigned g_program,g_VAO;
static uint8 *g_screen_buffer;
static bool g_want_screenshot;
static double g_screenshot_flash_start_time=-1;
static struct {int display_aspect,ignore_aspect_ratio,linear_filtering;}g_config;
static void (*g_compute_viewport)(int,int,int,int,SnesDisplayViewport*);
static unsigned calls[256],clock_calls,registered;
static char trace[128];static unsigned trace_count;
static double now;
static const char *timing_env;
static int fail_alloc,fail_registration;
static void (*finish_callback)(void);
static void mark(unsigned char value){
 calls[value]++;if(trace_count+1<sizeof trace){trace[trace_count++]=(char)value;trace[trace_count]=0;}
 if(value=='U'||value=='I')now+=.0002;
 if(value=='D'||value=='H')now+=.0004;
 if(value=='S')now+=.003;
}
static double MonotonicSeconds(void){clock_calls++;return now;}
static const char *test_getenv(const char *name){assert(!strcmp(name,"SNESRECOMP_GL_TIMING"));return timing_env;}
static void *test_calloc(size_t n,size_t size){return fail_alloc?NULL:calloc(n,size);}
static int test_atexit(void(*callback)(void)){
 if(fail_registration)return 1;registered++;finish_callback=callback;return 0;
}
static void snesrecomp_sdl_get_drawable_size(void*w,int*x,int*y){(void)w;*x=1920;*y=1080;}
static int SnesDisplayAspect_Clamp(int value){return value;}
static void SnesDisplayAspect_ComputeViewport(int w,int h,int dw,int dh,int aspect,int ignore,bool unused,SnesDisplayViewport*v){
 (void)w;(void)h;(void)aspect;(void)ignore;(void)unused;*v=(SnesDisplayViewport){0,0,dw,dh};
}
static void SaveScreenshotBmp(int w,int h){(void)w;(void)h;mark('R');}
static bool DrawScreenshotFlash(double elapsed){(void)elapsed;mark('F');return false;}
#define GL_LINEAR 1
#define GL_NEAREST 0
#define glBindTexture(...) mark('B')
#define glTexSubImage2D(...) mark('U')
#define glTexImage2D(...) mark('I')
#define glClearColor(...) mark('C')
#define glClear(...) mark('c')
#define glViewport(...) mark('V')
#define glUseProgram(...) mark('P')
#define glTexParameteri(...) mark('T')
#define glBindVertexArray(...) mark('A')
#define glDrawArrays(...) mark('D')
#define GlslShader_Render(...) mark('H')
#define SDL_GL_SwapWindow(...) mark('S')
#define getenv test_getenv
#define calloc test_calloc
#define atexit test_atexit
#include "gl_timing_under_test.h"
#undef getenv
#undef calloc
#undef atexit

static int exists(const char *path){FILE*f=fopen(path,"rb");if(!f)return 0;fclose(f);return 1;}
int main(int argc,char **argv){
 assert(argc==3);const char*mode=argv[1];const char*path=argv[2];
 timing_env=path;
 if(!strcmp(mode,"off"))timing_env=NULL;
 if(!strcmp(mode,"empty"))timing_env="";
 if(!strcmp(mode,"zero"))timing_env="0";
 if(!strcmp(mode,"negative"))timing_env="-1";
 char too_long[1025];memset(too_long,'x',sizeof too_long-1);too_long[sizeof too_long-1]=0;
 if(!strcmp(mode,"long"))timing_env=too_long;
 if(!strcmp(mode,"alloc"))fail_alloc=1;
 if(!strcmp(mode,"register"))fail_registration=1;
 bool disabled=!strcmp(mode,"off")||!strcmp(mode,"empty")||!strcmp(mode,"zero")||
   !strcmp(mode,"negative")||!strcmp(mode,"long")||fail_alloc||fail_registration;
 if(!strcmp(mode,"existing")){FILE*f=fopen(path,"wb");assert(f);fputs("OWNER SENTINEL",f);assert(!fclose(f));}
 OpenGLRenderer_EndDraw();
 assert(!strcmp(trace,"BUCcVPTTADS"));
 assert(calls['R']==0&&calls['F']==0);
 if(disabled){
  assert(!g_gl_timings&&!registered&&!clock_calls&&!exists(path));
  puts("PASS disabled/error diagnostic leaves original GL calls and no clock/file output");return 0;
 }
 assert(g_gl_timing_count==1&&registered==1&&finish_callback==GlTimingFinish&&clock_calls==6);
 assert(g_gl_timings[0].sequence==1&&g_gl_timings[0].width==448&&g_gl_timings[0].drawable_width==1920);
 assert(!g_gl_timings[0].shader&&!g_gl_timings[0].resized);
 assert(fabs(g_gl_timings[0].upload-.0002)<1e-12);
 assert(fabs(g_gl_timings[0].draw-.0004)<1e-12);
 assert(fabs(g_gl_timings[0].swap-.003)<1e-12);
 if(strcmp(mode,"existing"))assert(!exists(path));
 if(!strcmp(mode,"enabled")){
  trace_count=0;trace[0]=0;g_draw_width=682;g_glsl_shader=(GlslShader*)1;
  OpenGLRenderer_EndDraw();assert(!strcmp(trace,"BICcHS"));
  trace_count=0;trace[0]=0;OpenGLRenderer_EndDraw();assert(!strcmp(trace,"BUCcHS"));
  assert(g_gl_timing_count==3&&g_gl_timings[1].resized&&!g_gl_timings[2].resized);
  assert(g_gl_timings[2].sequence==3&&g_gl_timings[2].shader&&g_gl_timings[2].width==682);
  assert(clock_calls==18&&!exists(path));
 }
 if(!strcmp(mode,"capacity")){
  for(unsigned i=1;i<kGlTimingCapacity+3;i++)OpenGLRenderer_EndDraw();
  assert(g_gl_timing_count==kGlTimingCapacity&&g_gl_timing_dropped==3);
  assert(clock_calls==kGlTimingCapacity*6&&!exists(path));
  assert(calls['B']==kGlTimingCapacity+3&&calls['S']==kGlTimingCapacity+3);
 }
 finish_callback();assert(!g_gl_timings);
 if(!strcmp(mode,"existing")){
  FILE*f=fopen(path,"rb");char text[32]={0};assert(f&&fread(text,1,sizeof text,f)==14);fclose(f);
  assert(!strcmp(text,"OWNER SENTINEL"));
 }else if(!strcmp(mode,"missing-parent"))assert(!exists(path));
 else{
  FILE*f=fopen(path,"r");assert(f);char line[512];unsigned lines=0;
  while(fgets(line,sizeof line,f))lines++;fclose(f);
  assert(lines==(!strcmp(mode,"capacity")?kGlTimingCapacity:!strcmp(mode,"enabled")?3:1)+1);
 }
 puts("PASS actual EndDraw call order, exact phase durations, bounded/deferred fresh-only CSV");return 0;
}
