/* ROM-free platform qualification: create the desktop renderer's real GL3.3
 * core context, draw/read a pixel and enumerate controllers without opening one.
 * This tests host graphics availability, not Lagoon gameplay or input mappings. */
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_opengl.h>
#include <stdio.h>

typedef const GLubyte *(APIENTRY *GetStringFn)(GLenum);
typedef void (APIENTRY *ClearColorFn)(GLfloat,GLfloat,GLfloat,GLfloat);
typedef void (APIENTRY *ClearFn)(GLbitfield);
typedef void (APIENTRY *ReadPixelsFn)(GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,void *);
typedef void (APIENTRY *FinishFn)(void);
typedef GLenum (APIENTRY *GetErrorFn)(void);
typedef void (APIENTRY *ViewportFn)(GLint,GLint,GLsizei,GLsizei);

static int close_probe(SDL_Window *window,SDL_GLContext context,int status) {
  if(context)SDL_GL_DestroyContext(context);
  if(window)SDL_DestroyWindow(window);
  SDL_Quit();return status;
}

int main(void) {
  SDL_SetMainReady();
  if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMEPAD)) {
    fprintf(stderr,"FAIL SDL initialization: %s\n",SDL_GetError());return 1;
  }
  if(!SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE)||
     !SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3)||
     !SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3)||
     !SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1)) {
    fprintf(stderr,"FAIL core context attributes: %s\n",SDL_GetError());
    return close_probe(NULL,NULL,1);
  }
  SDL_Window *window=SDL_CreateWindow("Lagoon platform probe",64,64,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
  if(!window) {
    fprintf(stderr,"FAIL hidden OpenGL window: %s\n",SDL_GetError());
    return close_probe(NULL,NULL,1);
  }
  SDL_GLContext context=SDL_GL_CreateContext(window);
  if(!context||!SDL_GL_MakeCurrent(window,context)) {
    fprintf(stderr,"FAIL GL3.3 core context: %s\n",SDL_GetError());
    return close_probe(window,context,1);
  }
  GetStringFn get_string=(GetStringFn)SDL_GL_GetProcAddress("glGetString");
  ClearColorFn clear_color=(ClearColorFn)SDL_GL_GetProcAddress("glClearColor");
  ClearFn clear=(ClearFn)SDL_GL_GetProcAddress("glClear");
  ReadPixelsFn read_pixels=(ReadPixelsFn)SDL_GL_GetProcAddress("glReadPixels");
  FinishFn finish=(FinishFn)SDL_GL_GetProcAddress("glFinish");
  GetErrorFn get_error=(GetErrorFn)SDL_GL_GetProcAddress("glGetError");
  ViewportFn viewport=(ViewportFn)SDL_GL_GetProcAddress("glViewport");
  if(!get_string||!clear_color||!clear||!read_pixels||!finish||!get_error||!viewport) {
    fprintf(stderr,"FAIL required OpenGL entry points: %s\n",SDL_GetError());
    return close_probe(window,context,1);
  }
  const char *version=(const char *)get_string(GL_VERSION);
  const char *renderer=(const char *)get_string(GL_RENDERER);
  int major=0,minor=0;
  printf("video_driver=%s\nGL_VERSION=%s\nGL_RENDERER=%s\n",SDL_GetCurrentVideoDriver(),
         version?version:"unknown",renderer?renderer:"unknown");
  if(!version||sscanf(version,"%d.%d",&major,&minor)!=2||major<3||(major==3&&minor<3)) {
    fputs("FAIL actual OpenGL version is below3.3.\n",stderr);
    return close_probe(window,context,1);
  }
  viewport(0,0,64,64);clear_color(0.125f,0.5f,0.75f,1.0f);clear(GL_COLOR_BUFFER_BIT);finish();
  unsigned char pixel[4]={0};read_pixels(16,16,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
  GLenum error=get_error();
  printf("readback_rgba=%u,%u,%u,%u gl_error=%u\n",pixel[0],pixel[1],pixel[2],pixel[3],(unsigned)error);
  if(error!=GL_NO_ERROR||pixel[0]<29||pixel[0]>35||pixel[1]<125||pixel[1]>131||
     pixel[2]<188||pixel[2]>194||pixel[3]<252) {
    fputs("FAIL OpenGL clear/readback.\n",stderr);return close_probe(window,context,1);
  }
  SDL_PumpEvents();int count=0;SDL_JoystickID *ids=SDL_GetGamepads(&count);
  if(!ids) {
    fprintf(stderr,"FAIL controller enumeration: %s\n",SDL_GetError());
    return close_probe(window,context,1);
  }
  printf("gamepads=%d\n",count);
  for(int i=0;i<count&&i<4;i++) {
    const char *name=SDL_GetGamepadNameForID(ids[i]);
    printf("gamepad[%d]=%s\n",i,name?name:"unnamed");
  }
  SDL_free(ids);
  puts("PASS: real hidden GL3.3+ context, draw/readback and SDL controller enumeration.");
  return close_probe(window,context,0);
}
