/* Actual metadata plus isolated exploration/persistence tests. No guest CPU,
 * gameplay or collision outcome is fabricated by this data-model harness. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *test_root;
const char *RtlSaveRoot(void){return test_root;}
int LagoonSceneRenderer_ExactRoom(const unsigned char *r){return r[0x1fffe]==0xa5;}
unsigned char g_ram[0x20000];
#define ram g_ram
#include "../src/lagoon_map.c"
static void word(unsigned at,unsigned v){ram[at]=v;ram[at+1]=v>>8;}
static void ptr(unsigned at,unsigned v){word(at,v);ram[at+2]=v>>16;}
int main(int argc,char **argv){
 assert(argc==3);test_root=argv[2];
 FILE *f=fopen(argv[1],"rb");assert(f);unsigned char *rom=malloc(0x100000);assert(fread(rom,1,0x100000,f)==0x100000);fclose(f);
 LagoonMap_Init(rom,0x100000);LagoonQuestGuidance_Init(rom,0x100000);
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
 /* The retired Select+L chord no longer opens the map. The host's L2 or
  * M callback issues this same title command at the frame boundary. */
 assert(!LagoonMap_PollOpen(SNES_PAD_SELECT|SNES_PAD_L));
 LagoonMap_Command(LAGOON_MAP_TOGGLE);assert(LagoonMap_PollOpen(0));
 const uint32_t *image;int w,h;assert(LagoonMap_Image(&image,&w,&h)&&w==640&&h==520);
 LagoonMap_Command(LAGOON_MAP_NEXT);assert(selected==0);
 LagoonMap_Command(LAGOON_MAP_CLOSE);assert(!LagoonMap_Image(&image,&w,&h));
 LagoonMap_Save();assert(!dirty);
 unsigned saved=checksum((uint8_t*)cells,sizeof(cells));
 memset(cells,0,sizeof(cells));save_loaded=0;load_save();assert(!save_invalid&&checksum((uint8_t*)cells,sizeof(cells))==saved);
 /* Pause/navigation is host-only. Geometry and flags are unchanged. */
 assert(!memcmp(before,ram,sizeof(ram)));
 LagoonMap_Command(LAGOON_MAP_TOGGLE);assert(LagoonMap_PollOpen(0));LagoonMap_Observe(ram,2);assert(!open&&current==0);
 ram[0x100e]=1;LagoonMap_Observe(ram,2);assert(current==75);LagoonMap_Command(LAGOON_MAP_TOGGLE);assert(!LagoonMap_PollOpen(0));
 /* Integrate the source-qualified tablet hint and purple objective marker.
  * Only the normal observation discovers terrain around synthetic Nasir XY;
  * reading/drawing an objective cannot reveal its distant cell or grant items. */
 ram[0x100e]=0;word(0x48e,10);word(0x53,rooms[10].w/8);word(0x55,rooms[10].h/8);
 word(0x502,40);word(0x504,40);ram[0x4e0+0x17/8]|=0x80u>>(0x17&7);
 ram[0x4d0+0x15/8]|=0x80u>>(0x15&7); /* Original prerequisite, fixture only. */
 ram[0x4d0+0x36/8]|=0x80u>>(0x36&7);ram[0x4d0+0x37/8]|=0x80u>>(0x37&7);
 LagoonMap_Observe(ram,3);assert(current==10&&!cells[10][31*128+3]);
 LagoonMap_Command(LAGOON_MAP_TOGGLE);assert(LagoonMap_PollOpen(0));
 uint8_t *fog=malloc(sizeof cells);assert(fog);memcpy(fog,cells,sizeof cells);memcpy(before,ram,sizeof ram);
 LagoonQuestGuidance objective;assert(LagoonQuestGuidance_Get(ram,&objective));
 assert(!strcmp(objective.title,"Tablet of Hope")&&objective.marker&&objective.room==10&&objective.x==48&&objective.y==496);
 assert(LagoonMap_Image(&image,&w,&h)&&w==640&&h==520);
 unsigned cw=rooms[10].w/16,ch=rooms[10].h/16;double scale=416.0/cw;if(296.0/ch<scale)scale=296.0/ch;
 int ox=24+(416-(int)(cw*scale))/2,oy=92+(296-(int)(ch*scale))/2;
 int mx=ox+(int)(objective.x*scale/16),my=oy+(int)(objective.y*scale/16);
 assert(image[my*MW+mx+3]==0xffda8becu);
 assert(!memcmp(before,ram,sizeof ram)&&!memcmp(fog,cells,sizeof cells)&&!cells[10][31*128+3]);
 ram[0x4d0+0x38/8]|=0x80u>>(0x38&7);memcpy(before,ram,sizeof ram);
 assert(LagoonQuestGuidance_Get(ram,&objective)&&!strcmp(objective.title,"Return to Voloh")&&!objective.marker);
 assert(LagoonMap_Image(&image,&w,&h)&&!memcmp(before,ram,sizeof ram)&&!memcmp(fog,cells,sizeof cells));
 ram[0x4e0+0x11/8]|=0x80u>>(0x11&7);memcpy(before,ram,sizeof ram);
 assert(!LagoonQuestGuidance_Get(ram,&objective));assert(LagoonMap_Image(&image,&w,&h));
 assert(!memcmp(before,ram,sizeof ram)&&!memcmp(fog,cells,sizeof cells));free(fog);LagoonMap_Command(LAGOON_MAP_CLOSE);
 /* Foreign/corrupt file is kept, does not become explored data or get overwritten. */
 f=fopen(save_path,"wb");assert(f);fputs("foreign exploration",f);fclose(f);
 save_loaded=save_invalid=0;memset(cells,0,sizeof(cells));load_save();assert(save_invalid&&!visited(0));dirty=1;LagoonMap_Save();
 f=fopen(save_path,"rb");char b[20]={0};assert(fread(b,1,19,f)==19&&!strcmp(b,"foreign exploration"));fclose(f);
 printf("PASS original75room metadata/%udoors; explored fog, no guest writes, 520px panel/host opener command, read-only tablet hint/marker without fog or item grants, generation/menu refusal, exact persistence and corrupt-file preservation\n",doors);
 free(rom);return 0;
}
