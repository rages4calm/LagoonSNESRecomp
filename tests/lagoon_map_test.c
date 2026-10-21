/* Actual metadata plus isolated exploration/persistence tests. No guest CPU,
 * gameplay or collision outcome is fabricated by this data-model harness. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *test_root;
const char *RtlSaveRoot(void){return test_root;}
int LagoonSceneRenderer_ExactRoom(const unsigned char *r){return r[0x1fffe]==0xa5;}
#include "../src/lagoon_map.c"
static unsigned char ram[0x20000];
static void word(unsigned at,unsigned v){ram[at]=v;ram[at+1]=v>>8;}
static void ptr(unsigned at,unsigned v){word(at,v);ram[at+2]=v>>16;}
int main(int argc,char **argv){
 assert(argc==3);test_root=argv[2];
 FILE *f=fopen(argv[1],"rb");assert(f);unsigned char *rom=malloc(0x100000);assert(fread(rom,1,0x100000,f)==0x100000);fclose(f);
 LagoonMap_Init(rom,0x100000);
 if(!available){fprintf(stderr,"metadata refused\n");return 2;}
 unsigned doors=0;for(unsigned i=0;i<75;i++)doors+=rooms[i].count;
 assert(rooms[0].w==1536&&rooms[0].h==1536&&doors>200);
 /* Real descriptor dimensions, synthetic observation only. */
 word(0x53,192);word(0x55,192);word(0x502,640);word(0x504,96);word(0x520,18);ram[0x500]=0xc0;ram[0x1fffe]=0xa5;
 ptr(0x78,0x068000);memset(ram+0x14000,0,16384);
 unsigned char before[sizeof(ram)];memcpy(before,ram,sizeof(ram));
 LagoonMap_Observe(ram,1);assert(current==0&&visited(0));assert(!memcmp(before,ram,sizeof(ram)));
 for(unsigned i=1;i<75;i++)assert(!visited(i));
 assert(cells[0][6*128+40]&0x80);
 LagoonMap_Command(LAGOON_MAP_TOGGLE);assert(LagoonMap_PollOpen(0));
 const uint32_t *image;int w,h;assert(LagoonMap_Image(&image,&w,&h)&&w==640&&h==448);
 LagoonMap_Command(LAGOON_MAP_NEXT);assert(selected==0);
 LagoonMap_Command(LAGOON_MAP_CLOSE);assert(!LagoonMap_Image(&image,&w,&h));
 LagoonMap_Save();assert(!dirty);
 unsigned saved=checksum((uint8_t*)cells,sizeof(cells));
 memset(cells,0,sizeof(cells));save_loaded=0;load_save();assert(!save_invalid&&checksum((uint8_t*)cells,sizeof(cells))==saved);
 /* Pause/navigation is host-only. Geometry and flags are unchanged. */
 assert(!memcmp(before,ram,sizeof(ram)));
 LagoonMap_Command(LAGOON_MAP_TOGGLE);assert(LagoonMap_PollOpen(0));LagoonMap_Observe(ram,2);assert(!open&&current==0);
 ram[0x100e]=1;LagoonMap_Observe(ram,2);assert(current==75);LagoonMap_Command(LAGOON_MAP_TOGGLE);assert(!LagoonMap_PollOpen(0));
 /* Foreign/corrupt file is kept, does not become explored data or get overwritten. */
 f=fopen(save_path,"wb");assert(f);fputs("foreign exploration",f);fclose(f);
 save_loaded=save_invalid=0;memset(cells,0,sizeof(cells));load_save();assert(save_invalid&&!visited(0));dirty=1;LagoonMap_Save();
 f=fopen(save_path,"rb");char b[20]={0};assert(fread(b,1,19,f)==19&&!strcmp(b,"foreign exploration"));fclose(f);
 printf("PASS original75room metadata/%udoors; explored fog, no guest writes, modal controls, generation/menu refusal, exact persistence and corrupt-file preservation\n",doors);
 free(rom);return 0;
}
