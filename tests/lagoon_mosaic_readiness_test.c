/* Source-extracted geometry/readiness + unchanged real settling state machine.
 * Resource callbacks are controlled predicates here; actual original uploads,
 * source identity and native pixels require the separate affected live route. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "lagoon_wide_entities.h"
#include "snes/ppu.h"
static Ppu p,*g_ppu=&p;
static uint8_t g_ram[0x20000];static const uint8_t *g_rom;
static int s_using_retained,source_ok=1,upload_ok=1,map_ok=1,pointer_ok=1;
static unsigned word(unsigned a){return g_ram[a]|g_ram[a+1]<<8;}
static unsigned pointer(unsigned a){return word(a)|g_ram[a+2]<<16;}
static int retained_scene_matches(void){return 0;}
static int room_parallax(void){return 0;}
static int parallax_raster(const Ppu *ppu){(void)ppu;return 0;}
static int valid_pointer(unsigned value){(void)value;return pointer_ok;}
static int audited_room_resources(void){return source_ok;}
static int background_upload_ready(void){return upload_ok;}
static int native_world_map(void){return map_ok;}
int LagoonWideEntities_ParallaxMapForRoom(const uint16_t*v,const uint8_t*r,size_t n,unsigned room){(void)v;(void)r;(void)n;(void)room;return 0;}
#include "lagoon_readiness_under_test.inc"
static void setup(void){memset(&p,0,sizeof p);memset(g_ram,0,sizeof g_ram);p.bgmode=9;p.bgXsc[0]=0x71;p.bgXsc[1]=0x79;g_ram[0x500]=0xc0;source_ok=upload_ok=map_ok=pointer_ok=1;}
int main(void){setup();unsigned checks=0;
 for(unsigned mosaic=0;mosaic<256;mosaic++){
  p.mosaic=mosaic;assert(world_scene_geometry()==!(mosaic&3));
  assert(initial_world_source_ready()==(!(mosaic&3)||(mosaic&15)==15));checks++;
 }
 p.mosaic=255;LagoonWideSceneGate gate={0};
 for(unsigned field=0;field<48;field++){
  p.mosaic=((15-field/3)<<4)|15; /* Real FF,EF,...,0F source sequence. */
  int ready=initial_world_source_ready();assert(ready&&!world_scene_geometry());
  assert(LagoonWideEntities_SceneSettled(&gate,ready,43,7)==(field>=2));checks++;
 }
 p.mosaic=0;assert(world_scene_geometry());assert(LagoonWideEntities_SceneSettled(&gate,initial_world_source_ready(),43,7));
 int *predicates[]={&source_ok,&upload_ok,&map_ok,&pointer_ok};
 for(unsigned i=0;i<4;i++){
  p.mosaic=255;*predicates[i]=0;assert(!initial_world_source_ready());
  assert(!LagoonWideEntities_SceneSettled(&gate,initial_world_source_ready(),43,7));assert(!gate.fields);
  *predicates[i]=1;for(unsigned field=0;field<3;field++)assert(LagoonWideEntities_SceneSettled(&gate,initial_world_source_ready(),43,7)==(field==2));checks++;
 }
 assert(!LagoonWideEntities_SceneSettled(&gate,1,43,8));assert(gate.fields==1);
 assert(!LagoonWideEntities_SceneSettled(&gate,1,44,8));assert(gate.fields==1);
 setup();p.mosaic=255;
 p.bgmode=0;assert(!initial_world_source_ready());p.bgmode=9;
 p.bgmode|=16;assert(!initial_world_source_ready());p.bgmode=9;
 p.bgXsc[0]=0;assert(!initial_world_source_ready());p.bgXsc[0]=0x71;
 p.bgXsc[1]=0;assert(!initial_world_source_ready());p.bgXsc[1]=0x79;
 g_ram[0x500]=0;assert(!initial_world_source_ready());g_ram[0x500]=0xc0;
 g_ram[0x50]=1;assert(!initial_world_source_ready());g_ram[0x50]=0;
 assert(initial_world_source_ready()&&!world_scene_geometry());
 printf("PASS %u mosaic readiness cases; originalFF..0F all-BG source-only accumulation, partial flags/draw refusal unchanged, source/upload/map/pointer/mode/layout/actor/generation/key failures reset settling\n",checks);return 0;
}
