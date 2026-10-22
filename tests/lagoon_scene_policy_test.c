/* Exact policy-input reuse, checked against actual SDK OBJ placement and the
 * previous full-derivation raster path. No game ROM or owner data is used. */
#define main sdk_sprite_suite_main
#define WsShadowTile legacy_harness_shadow_tile
#include "../snesrecomp/tests/ppu/ppu_sprite_limit_test.c"
#undef main
#undef WsShadowTile
uint16_t WsShadowTile(int layer,int screen_x,uint32_t wrapped_y,
    uint16_t scroll,uint16_t address,uint16_t tile) {
  (void)layer;(void)screen_x;(void)wrapped_y;(void)scroll;(void)address;return tile;
}
#include "../snesrecomp/runner/src/snes/ppu.c"
#include <assert.h>
#define LAGOON_WIDE_ENTITIES_DECODER_ONLY
#include "../src/lagoon_wide_entities.c"
#ifndef LAGOON_SCENE_POLICY_SOURCE
#define LAGOON_SCENE_POLICY_SOURCE "../src/lagoon_scene_renderer.c"
#endif
#include LAGOON_SCENE_POLICY_SOURCE
#ifdef _WIN32
#include <windows.h>
static double now_seconds(void) {
  LARGE_INTEGER n,d;QueryPerformanceCounter(&n);QueryPerformanceFrequency(&d);
  return (double)n.QuadPart/(double)d.QuadPart;
}
#else
#include <time.h>
static double now_seconds(void) {struct timespec t;timespec_get(&t,TIME_UTC);return t.tv_sec+t.tv_nsec/1e9;}
#endif
static uint8_t scene_ram[131072],scene_rom[1048576];
enum { Extra=213,Width=256+2*Extra };
static uint32_t pixels[Width];
static const uint8_t hints[16];
static void field(Ppu *p,int x) {
  memset(pixels,0,sizeof pixels);setup_one_sprite_line(p,x&511);
  ppu_runLine(p,0);ppu_runLine(p,1);
}
static void prime(Ppu *p,int x) {
  ppu_reset(p);PpuBeginDrawing(p,(uint8_t*)pixels,sizeof pixels,kPpuRenderFlags_NewRenderer);
  PpuSetExtraSpace(p,Extra);PpuWsSetOamLeftHints(p,hints);PpuWsSetOamRightHints(p,hints);
  field(p,x+5);field(p,x);assert(p->wsOamMotionGrace[0]==4);
}
static unsigned cases,accepted,refused,metadata_cases;
static uint64_t decision_hash=UINT64_C(14695981039346656037);
static void hash_bytes(const void *v,size_t n) {
  const uint8_t *p=v;while(n--){decision_hash^=*p++;decision_hash*=UINT64_C(1099511628211);}
}
static void put_pointer(unsigned a,unsigned value) {
  scene_ram[a]=value;scene_ram[a+1]=value>>8;scene_ram[a+2]=value>>16;
}
static void scene_setup(Ppu *p) {
  ppu_reset(p);memset(scene_ram,0,sizeof scene_ram);
  put_pointer(0x57,0x138000);put_pointer(0x5d,0x198000);
  put_pointer(0x69,0x068000);put_pointer(0x75,0x068000);
  put_pointer(0x88,0x7f0000);put_pointer(0x8b,0x7f4000);
  scene_ram[0x53]=128;scene_ram[0x55]=128;scene_ram[0x480]=128;scene_ram[0x482]=128;
  p->inidisp=15;p->bgmode=9;p->obsel=2;p->bgXsc[0]=0x71;p->bgXsc[1]=0x79;
  p->bgXsc[2]=0x3c;p->bgTileAdr=0x600;p->screenEnabled[0]=31;
  p->hScroll[0]=p->hScroll[1]=128;p->vScroll[0]=p->vScroll[1]=128;
  p->extraLeftCur=p->extraRightCur=213;
  p->wsOamLeftHintStrict=p->wsOamRightHintStrict=p->wsOamMotionGraceOn=1;
  s_edge_clip=1;
  for(unsigned slot=0;slot<128;++slot) {
    unsigned x=(slot*67)&511;setup_solid_obj(p,slot,x,240);
    p->wsOamMotionGrace[slot]=slot%5;
    if(slot%3==0)p->wsOamLeftHint[slot/8]|=1u<<(slot&7);
    if(slot%7==0)p->wsOamRightHint[slot/8]|=1u<<(slot&7);
  }
}
enum { MetadataBytes=4+3+16+16+128+1 };
static void mutate(Ppu *p,unsigned which,unsigned value) {
  if(which<4) {
    uint16_t *n=which<2?&p->extraLeftCur:&p->extraRightCur;
    unsigned shift=(which&1)*8;*n=(uint16_t)((*n&~(255u<<shift))|(value<<shift));
  } else if(which==4)p->wsOamLeftHintStrict=value;
  else if(which==5)p->wsOamRightHintStrict=value;
  else if(which==6)p->wsOamMotionGraceOn=value;
  else if(which<23)p->wsOamLeftHint[which-7]=value;
  else if(which<39)p->wsOamRightHint[which-23]=value;
  else if(which<167)p->wsOamMotionGrace[which-39]=value;
  else s_edge_clip=(int)value;
}
static void metadata_equivalence(Ppu *p) {
  SceneOamPolicy before,after;SceneOamPolicyInputs input;
  scene_setup(p);Ppu original=*p;int edge=s_edge_clip;
  object_policy(&before,p);object_policy_inputs(&input,p);
  assert(object_policy_inputs_equal(&input,p));
  for(unsigned which=0;which<MetadataBytes;++which)for(unsigned value=0;value<256;++value) {
    *p=original;s_edge_clip=edge;mutate(p,which,value);
    object_policy(&after,p);
    if(object_policy_inputs_equal(&input,p))assert(!memcmp(&before,&after,sizeof before));
    hash_bytes(&after,sizeof after);++metadata_cases;
  }
  /* Disabled grace bytes are irrelevant; changing them must stay reusable. */
  scene_setup(p);p->wsOamMotionGraceOn=0;object_policy_inputs(&input,p);
  memset(p->wsOamMotionGrace,255,sizeof p->wsOamMotionGrace);
  assert(object_policy_inputs_equal(&input,p));
}
static void raster_case(Ppu *p,unsigned which,unsigned value,unsigned line) {
  scene_setup(p);LagoonSceneRenderer_Reset();s_mode=1;
  SceneOamPolicy original,current;object_policy(&original,p);int expected=1;
  for(unsigned y=1;y<=224;++y) {
    if(y==line)mutate(p,which,value);
    object_policy(&current,p);
    if(memcmp(&original,&current,sizeof original))expected=0;
    LagoonSceneRenderer_ObserveRaster(p,y,scene_ram,scene_rom,sizeof scene_rom,7,LAGOON_SCENE_WORLD);
  }
  LagoonSceneRenderer_EndFrame(cases,7,LAGOON_SCENE_WORLD);
  int actual=!!s_packets[s_complete].valid;assert(actual==expected);
  hash_bytes(&actual,sizeof actual);++cases;if(actual)++accepted;else++refused;
}
static void actual_sdk_vectors(Ppu *p) {
  for(unsigned mode=0;mode<8;++mode) {
    scene_setup(p);p->wsOamLeftHintStrict=mode&1;p->wsOamRightHintStrict=!!(mode&2);
    p->wsOamMotionGraceOn=!!(mode&4);SceneOamPolicy policy;object_policy(&policy,p);
    for(unsigned slot=0;slot<128;++slot) {
      unsigned index=slot*2,n=(p->highOam[slot/4]>>(2*(slot&3)))&2?16:8;
      assert(policy.x[slot]==PpuDecodeOamX(p,index));
      assert(policy.allow[slot]==PpuWidescreenOamLeftHintAllows(p,index,policy.x[slot],n,p->extraLeftCur));
    }
  }
  /* Retain the shared big-jump fix and its actual pixel evaluator. */
  prime(p,-90);field(p,320);assert(!p->wsOamMotionGrace[0]&&pixels[Extra-192]==0);
  prime(p,-190);field(p,320);assert(p->wsOamMotionGrace[0]==4&&pixels[Extra-192]);
}
static double benchmark(Ppu *p,unsigned fields) {
  scene_setup(p);LagoonSceneRenderer_Reset();s_mode=1;
  double start=now_seconds();
  for(unsigned frame=0;frame<fields;++frame) {
    for(unsigned y=1;y<=224;++y) {
#if defined(__clang__) || defined(__GNUC__)
      __asm__ __volatile__("" : : "r"(p) : "memory");
#endif
      LagoonSceneRenderer_ObserveRaster(p,y,scene_ram,scene_rom,sizeof scene_rom,7,LAGOON_SCENE_WORLD);
    }
    assert(s_packets[s_build].valid);
  }
  return (now_seconds()-start)*1000;
}
int main(int argc,char **argv) {
  Ppu *p=ppu_init();assert(p);actual_sdk_vectors(p);metadata_equivalence(p);
  for(unsigned which=0;which<MetadataBytes;++which) {
    raster_case(p,which,0,100);raster_case(p,which,255,100);
  }
  /* Both changed-but-equivalent inputs and actual policy changes are retained. */
  assert(accepted>0&&refused>0);
  for(unsigned line=2;line<=224;++line)raster_case(p,4,0,line);
  unsigned fields=argc==2?(unsigned)strtoul(argv[1],NULL,10):1500;assert(fields&&fields<=20000);
  benchmark(p,20);double ms=benchmark(p,fields);ppu_free(p);
  printf("{\"passed\":true,\"metadata_cases\":%u,\"raster_cases\":%u,\"accepted\":%u,\"refused\":%u,\"decision_hash\":\"%016llx\",\"actual_sdk_policy_vectors\":1024,\"actual_big_jump_pixels_preserved\":true,\"benchmark_fields\":%u,\"benchmark_ms\":%.6f}\n",metadata_cases,cases,accepted,refused,(unsigned long long)decision_hash,fields,ms);
  return 0;
}
