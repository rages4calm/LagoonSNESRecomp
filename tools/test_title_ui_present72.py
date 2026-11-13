"""Isolated production GL presentation policy, with instrumented GL/SDL calls.

This exercises actual EndDraw source without a GPU; actual UI readback remains a
separate live check. Host source assertions guard the 1:1 title-only canvas and
SDL viewport/filter restoration; browser/rewind retain their original path.
"""
import argparse
import json
from pathlib import Path
import subprocess


def function(source, name):
    start = source.rfind('\n', 0, source.index(name + '(')) + 1
    brace = source.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end] + '\n'


FIXTURE = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "desktop/display_aspect.h"
typedef uint8_t uint8;
typedef struct {unsigned gl_texture;int width,height;} GlTextureWithSize;
typedef struct {unsigned texture;int width,height;} SnesHdGpuImage;
typedef struct {int sequence,width,height,drawable_width,drawable_height,shader,resized;
 double upload,draw,swap,other;} GlTiming;
enum {GL_TEXTURE0,GL_TEXTURE_2D,GL_BGRA,GL_UNSIGNED_INT_8_8_8_8_REV,GL_RGBA,
 GL_COLOR_BUFFER_BIT,GL_TEXTURE_MIN_FILTER,GL_TEXTURE_MAG_FILTER,GL_LINEAR,
 GL_NEAREST,GL_TRIANGLE_STRIP,kGlTimingCapacity=32};
static struct {int display_aspect;bool ignore_aspect_ratio,linear_filtering;
 struct {int enabled,scale;} mode7;} g_config;
static uint8 dummy_pixels[4],*g_screen_buffer=dummy_pixels;
static int g_draw_width=960,g_draw_height=540,g_window;
static unsigned g_program=7,g_VAO=1,g_ui_VAO=2;
static GlTextureWithSize g_texture={3,960,540};
static void *g_glsl_shader=(void *)1,*g_hd_frame=(void *)1,*g_hd_gpu;
static const uint32_t *g_hd_overlay;static size_t g_hd_overlay_pitch;
static bool g_hd_tried,g_title_ui_frame,g_want_screenshot;
static double g_screenshot_flash_start_time=-1;
static GlTiming *g_gl_timings;
static unsigned g_gl_timing_count,g_gl_timing_dropped;
static unsigned custom,shader,hd,quad,swap,filter_calls,last_filter,bound_vao;
static bool ui_attributes=true;
static int drawable_w=1920,drawable_h=1080;
static SnesDisplayViewport presented;
static void CustomViewport(int a,int b,int c,int d,SnesDisplayViewport *v){
 (void)a;(void)b;(void)c;(void)d;custom++;*v=(SnesDisplayViewport){3,4,777,555};
}
static void (*g_compute_viewport)(int,int,int,int,SnesDisplayViewport*)=CustomViewport;
static void GlTimingInit(void){}
static double MonotonicSeconds(void){return 0;}
static void snesrecomp_sdl_get_drawable_size(int window,int *w,int *h){(void)window;*w=drawable_w;*h=drawable_h;}
static void *SnesHdGpuCreate(void){return (void*)1;}
#define SnesHdGpuRender(...) (hd++,false)
#define glActiveTexture(...) ((void)0)
#define glBindTexture(...) ((void)0)
#define glTexSubImage2D(...) ((void)0)
#define glTexImage2D(...) ((void)0)
#define glClearColor(...) ((void)0)
#define glClear(...) ((void)0)
#define glUseProgram(...) ((void)0)
#define glBindVertexArray(vao) (bound_vao=(vao))
#define glTexParameteri(target,key,value) (filter_calls++,last_filter=(value))
#define glViewport(x,y,w,h) (presented=(SnesDisplayViewport){x,y,w,h})
#define glDrawArrays(...) (assert(bound_vao!=g_ui_VAO||ui_attributes),quad++)
static void GlslShader_Render(void *a,GlTextureWithSize *b,int x,int y,int w,int h){
 (void)a;(void)b;(void)x;(void)y;(void)w;(void)h;
 assert(bound_vao==g_VAO);shader++; /* Game shader disables only its own attributes. */
 if(bound_vao==g_ui_VAO)ui_attributes=false;
}
static void SaveScreenshotBmp(int w,int h){(void)w;(void)h;}
static bool DrawScreenshotFlash(double t){(void)t;return false;}
#define SDL_GL_SwapWindow(...) (swap++)
#include "title_setter.inc"
#include "end_draw.inc"
#include "opaque_policy.inc"
static void clear_counts(void){custom=shader=hd=quad=swap=filter_calls=0;last_filter=999;}
int main(void){
  int fx,fy,fw,fh;
  TitlePanelFit(1192,672,512,448,&fx,&fy,&fw,&fh);
  assert(fx==212&&fy==0&&fw==768&&fh==672);
  TitlePanelFit(512,448,512,448,&fx,&fy,&fw,&fh);
  assert(fx==0&&fy==0&&fw==512&&fh==448);

 uint32_t panel[4]={0xff112233,0xff223344,0xff334455,0xff445566};
 assert(TitlePanelOpaque(panel,2,2));
 panel[3]&=0xffffff;assert(!TitlePanelOpaque(panel,2,2));
 panel[3]=0x7f445566;assert(!TitlePanelOpaque(panel,2,2));
 assert(!TitlePanelOpaque(NULL,2,2)&&!TitlePanelOpaque(panel,0,2));
 g_config.display_aspect=0;g_config.ignore_aspect_ratio=true;
 g_config.linear_filtering=true;g_config.mode7.enabled=1;g_config.mode7.scale=3;
 unsigned char config_before[sizeof(g_config)];memcpy(config_before,&g_config,sizeof(g_config));
 /* A preceding custom game shader must not poison the next UI VAO. */
 OpenGLRenderer_EndDraw();assert(shader==1&&custom==1&&hd==1&&!quad&&swap==1);
 clear_counts();snesrecomp_opengl_set_title_ui_frame(true);OpenGLRenderer_EndDraw();
 assert(!g_title_ui_frame&&!shader&&!custom&&!hd&&quad==1&&swap==1);
 assert(bound_vao==g_ui_VAO&&filter_calls==2&&last_filter==GL_NEAREST);
 assert(presented.x==0&&presented.y==0&&presented.width==1920&&presented.height==1080);
 /* Authored16:9 UI in a4:3 window: letterbox, do not apply7:6 SNES pixels. */
 drawable_w=960;drawable_h=720;clear_counts();
 snesrecomp_opengl_set_title_ui_frame(true);OpenGLRenderer_EndDraw();
 assert(presented.x==0&&presented.y==90&&presented.width==960&&presented.height==540);
 assert(!shader&&!custom&&!hd&&bound_vao==g_ui_VAO);
 /* No persistent config mutation or shader/HD disable leaks to normal play. */
 clear_counts();OpenGLRenderer_EndDraw();
 assert(shader==1&&custom==1&&hd==1&&!quad&&swap==1&&bound_vao==g_VAO);
 assert(!memcmp(config_before,&g_config,sizeof(g_config)));
 /* Even a failed BeginDraw consumes the one-shot title flag. */
 clear_counts();g_screen_buffer=NULL;snesrecomp_opengl_set_title_ui_frame(true);
 OpenGLRenderer_EndDraw();assert(!g_title_ui_frame&&!shader&&!quad&&!swap);
 g_screen_buffer=dummy_pixels;OpenGLRenderer_EndDraw();
 assert(shader==1&&custom==1&&hd==1&&swap==1);
 puts("PASS title GL policy: square-pixel viewport, nearest independent UI VAO, no game shader/HD, one-shot failure reset, normal game/config restoration");
 return 0;
}
'''


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--sdk', type=Path, required=True)
    p.add_argument('--compiler', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    a = p.parse_args()
    desktop = a.sdk.resolve() / 'runner/src/desktop'
    host = (desktop / 'host_main.c').read_text()
    gl = (desktop / 'opengl.c').read_text()
    frozen = function(host, 'PresentFrozenWithOverlay')
    assert 'const int draw_w = title_canvas ? pw : base_w * draw_scale;' in frozen
    assert 'const int draw_h = title_canvas ? ph : base_h * draw_scale;' in frozen
    assert 'panel + (size_t)y * (size_t)pw, (size_t)pw * sizeof(*panel)' in frozen
    assert 'if (panel && !title_canvas)' in frozen
    assert '} else if (is_title)' in frozen
    assert 'TitlePanelFit(draw_w,draw_h,pw,ph' in frozen
    assert host.index('g_game->presentation_compose(present_buffer') < host.index('g_frozen_w = presentation_w')
    assert 'is_title && TitlePanelOpaque(panel, pw, ph)' in frozen
    assert frozen.count('snesrecomp_opengl_set_title_ui_frame(false)') == 2
    assert frozen.count('g_title_ui_frame = false;') == 2
    assert 'TITLE_OVERLAY_DISPLAY_SCREENSHOT' in frozen
    sdl = function(host, 'SdlRenderer_BeginDraw')
    assert 'if (!g_title_ui_frame && g_game->compute_viewport)' in sdl
    assert '!g_title_ui_frame && g_config.linear_filtering' in sdl
    assert 'g_sdl_title_filter_override = g_title_ui_frame;' in sdl
    init = function(gl, 'OpenGLRenderer_Init')
    assert 'glGenVertexArrays(1, &g_ui_VAO)' in init
    assert 'glBindVertexArray(g_ui_VAO)' in init
    assert 'glDeleteVertexArrays(1, &g_ui_VAO)' in function(gl, 'OpenGLRenderer_Destroy')
    a.out.mkdir(parents=True, exist_ok=True)
    (a.out / 'title_setter.inc').write_text(function(gl, 'snesrecomp_opengl_set_title_ui_frame'))
    (a.out / 'end_draw.inc').write_text(function(gl, 'OpenGLRenderer_EndDraw'))
    (a.out / 'opaque_policy.inc').write_text(function(host, 'TitlePanelOpaque') + function(host, 'TitlePanelFit'))
    fixture = a.out / 'title_ui_policy.c'
    fixture.write_text(FIXTURE)
    exe = a.out / ('title_ui_policy.exe' if __import__('os').name == 'nt' else 'title_ui_policy')
    subprocess.run([str(a.compiler), '-std=c11', '-O2', '-I', str(a.sdk / 'runner/src'), str(fixture), '-o', str(exe)], check=True)
    run = subprocess.run([str(exe.resolve())], check=True, text=True, capture_output=True)
    (a.out / 'receipt.json').write_text(json.dumps(dict(passed=True, scope='Isolated production policy with stub GL; actual GPU readback is separate.', output=run.stdout.strip()), indent=2) + '\n')
    print(run.stdout, end='')


if __name__ == '__main__':
    main()
