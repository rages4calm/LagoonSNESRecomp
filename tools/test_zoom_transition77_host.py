"""Execute the production zoom compositor with isolated source/UI witnesses."""
from pathlib import Path
import argparse, hashlib, json, subprocess

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--compiler',required=True)
p.add_argument('--out',required=True,type=Path)
a=p.parse_args();repo=Path(__file__).resolve().parents[1];out=a.out.resolve();out.mkdir(parents=True,exist_ok=True)
source=(repo/'src/lagoon_widescreen.c').read_text(encoding='utf-8')
start=source.index('void LagoonWide_PresentationCompose(')
end=source.index('\nvoid LagoonWide_PrepareFrame(',start)
production=source[start:end]
prefix=r'''
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "lagoon_zoom_transition77.h"
#include "lagoon_zoom_placement.h"
static unsigned char memory[131072],rom[1048576],*g_ram=memory,*g_rom=rom;
static struct {unsigned inidisp;} ppu,*g_ppu=&ppu;
static unsigned s_completed_frame,s_zoom_world_fallbacks,s_zoom_authored_fallbacks;
static int s_present_incoming_mosaic,draw_ok,menu,modal,exact=1;
static unsigned view_room=7;
static uint64_t generation=9;
static struct {uint64_t generation;unsigned width,height,room,frame;int left,top,valid;}s_zoom_geometry;
static LagoonZoomTransition77 s_zoom_outgoing;
enum {LAGOON_MENU_NONE=0};
static unsigned word(unsigned a){return g_ram[a]|g_ram[a+1]<<8;}
static void set(unsigned a,unsigned v){g_ram[a]=v;g_ram[a+1]=v>>8;}
static uint64_t RtlStateGeneration(void){return generation;}
static int LagoonSceneRenderer_GameOver(const void*r,const void*q,unsigned n){(void)r;(void)q;(void)n;return 0;}
static int LagoonSceneRenderer_DrawView(uint8_t*d,size_t pitch,unsigned w,unsigned h,const uint8_t*s,size_t sp,unsigned sw,unsigned sh,uint64_t g){
 (void)s;(void)sp;(void)sw;(void)sh;(void)g;if(!draw_ok)return 0;
 for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){uint32_t c=0xffe1964b;memcpy(d+y*pitch+x*4,&c,4);}return 1;
}
static void LagoonBossHud_DrawDetached(void*d,size_t p,int w,int h,const void*s,size_t sp,int sw,uint64_t g){(void)d;(void)p;(void)w;(void)h;(void)s;(void)sp;(void)sw;(void)g;}
static int LagoonSceneRenderer_ViewPlacementRoom(unsigned w,unsigned h,uint64_t g,int*l,int*t,unsigned*r){(void)w;(void)h;(void)g;*l=31;*t=41;*r=view_room;return 1;}
static int LagoonMenuPresentation_Identify(const void*p,const void*r){(void)p;(void)r;return menu;}
static int has_modal_ui(void){return modal;}
static int LagoonSceneRenderer_ExactRoom(const void*r){(void)r;return exact;}
static int world_map_geometry(void){return 1;}
static int view_axis(int c,int m,int e,int n){(void)m;return -(e-n)/2+c%5;}
static void LagoonSceneRenderer_DrawHud(void*d,size_t p,int w,int h,uint64_t g){(void)d;(void)p;(void)w;(void)h;(void)g;}
static void report_zoom_placement(int k,int l,int t,int w,int h){(void)k;(void)l;(void)t;(void)w;(void)h;}
'''
suffix=r'''
static uint8_t src[400*224*4],dst[(900*4+16)*336];
static uint32_t pixel(unsigned x,unsigned y,unsigned pitch){uint32_t c;memcpy(&c,dst+y*pitch+x*4,4);return c;}
static void present(void){LagoonWide_PresentationCompose(dst,2416,600,336,src,1600,400,224);}
static void fresh(void){
 generation=9;view_room=7;g_ppu=&ppu;set(0x48e,7);set(0x53,128);set(0x55,128);set(0x480,999);set(0x482,777);
 g_ram[0x100e]=0;g_ppu->inidisp=15;s_completed_frame=100;
 menu=modal=s_present_incoming_mosaic=0;exact=draw_ok=1;
 LagoonZoomTransition77_Reset(&s_zoom_outgoing);present();
 assert(s_zoom_outgoing.valid&&s_zoom_geometry.left==31&&s_zoom_geometry.top==41&&s_zoom_geometry.room==7);
 draw_ok=0;
}
int main(void){
 for(unsigned i=0;i<400*224;i++){uint32_t c=0xff123456;memcpy(src+i*4,&c,4);}
 fresh();g_ram[0x100e]=255;g_ppu->inidisp=10;s_completed_frame++;
 present();assert(pixel(0,0,2416)==0xff966432&&pixel(599,335,2416)==0xff966432);
 g_ppu->inidisp=15;s_completed_frame++;present();assert(pixel(0,0,2416)==0xff966432);
 g_ppu->inidisp=128;present();assert(pixel(599,335,2416)==0xff000000);
 g_ppu->inidisp=15;present();assert(pixel(599,335,2416)==0xff000000);
 fresh();g_ram[0x100e]=255;menu=1;present();
 assert(!s_zoom_outgoing.valid&&pixel(0,0,2416)==0xff123456&&pixel(599,335,2416)==0xff123456);
 fresh();g_ram[0x100e]=255;modal=1;present();assert(!s_zoom_outgoing.valid&&pixel(0,0,2416)==0xff123456);
 fresh();g_ram[0x100e]=255;s_present_incoming_mosaic=1;present();assert(!s_zoom_outgoing.valid);
 fresh();g_ram[0x100e]=255;set(0x48e,8);view_room=8;draw_ok=1;present();
 assert(!s_zoom_outgoing.valid&&s_zoom_geometry.room==8); /* accepted incoming, flag still set */
 draw_ok=0;present();assert(!s_zoom_outgoing.valid&&pixel(0,0,2416)!=0xffe1964b);
 fresh();g_ram[0x100e]=255;g_ppu=NULL;present();assert(!s_zoom_outgoing.valid);
 fresh();g_ram[0x100e]=255;generation++;present();assert(!s_zoom_outgoing.valid);
 fresh();g_ram[0x100e]=255;LagoonWide_PresentationCompose(dst,3616,900,336,src,1600,400,224);assert(!s_zoom_outgoing.valid);
 fresh();s_completed_frame=101;set(0x48e,8);present();assert(s_zoom_outgoing.valid&&pixel(0,0,2416)==0xffe1964b);
 s_completed_frame=102;present();assert(!s_zoom_outgoing.valid); /* no stuck old-world canvas */
 fresh();s_completed_frame=101;present();assert(!s_zoom_outgoing.valid); /* ordinary source failure */
 fresh();g_ram[0x100e]=255;exact=0;present();assert(s_zoom_outgoing.valid); /* incomplete loader descriptor only */
 LagoonZoomTransition77_Reset(&s_zoom_outgoing);present();assert(!s_zoom_outgoing.valid); /* no loader-only capture */
 puts("PASS production compositor: complete zoom fade, completed origin, menu/dialogue/incoming/reset/aspect retirement, bounded preloader and no latent-source capture");return 0;
}
'''
fixture=out/'host.c';fixture.write_text(prefix+production+suffix,encoding='utf-8')
binary=out/'host.exe'
compile=subprocess.run([a.compiler,'-std=c11','-O2','-I'+str(repo/'src'),str(fixture),'-o',str(binary)],capture_output=True,text=True)
(out/'build.log').write_text(compile.stdout+compile.stderr)
compile.check_returncode()
run=subprocess.run([str(binary)],capture_output=True,text=True);print(run.stdout+run.stderr)
receipt=dict(passed=run.returncode==0,scope='Source-extracted actual PresentationCompose with isolated source/UI witnesses; not gameplay or GPU qualification.',source_sha256=hashlib.sha256((repo/'src/lagoon_widescreen.c').read_bytes()).hexdigest(),helper_sha256=hashlib.sha256((repo/'src/lagoon_zoom_transition77.h').read_bytes()).hexdigest(),exit_code=run.returncode,stdout=run.stdout,stderr=run.stderr)
(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');run.check_returncode()
