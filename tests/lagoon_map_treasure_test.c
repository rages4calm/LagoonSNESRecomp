/* Actual ROM treasure catalogue, synthetic observed fog and original flags.
 * No guest CPU, item acquisition or unopened location is fabricated. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *test_root;
const char *RtlSaveRoot(void){return test_root;}
int LagoonSceneRenderer_ExactRoom(const unsigned char*r){return r[0x1fffe]==0xa5;}
unsigned char g_ram[0x20000];
#define ram g_ram
#include "../src/lagoon_map.c"
static void word(unsigned a,unsigned v){ram[a]=v;ram[a+1]=v>>8;}
static void observe(unsigned room,unsigned x,unsigned y,unsigned generation){
 word(0x48e,room);word(0x53,rooms[room].w/8);word(0x55,rooms[room].h/8);
 word(0x502,x);word(0x504,y);word(0x520,18);ram[0x500]=0xc0;ram[0x1fffe]=0xa5;
 word(0x78,0x8000);ram[0x7a]=6;uint8_t before[sizeof ram];memcpy(before,ram,sizeof ram);
 LagoonMap_Observe(ram,generation);assert(!memcmp(before,ram,sizeof ram));assert(current==room);
}
int main(int argc,char**argv){
 assert(argc==4);test_root=argv[3];FILE*f=fopen(argv[1],"rb");assert(f);uint8_t*rom=malloc(0x100000);assert(fread(rom,1,0x100000,f)==0x100000);fclose(f);
 LagoonMap_Init(rom,0x100000);LagoonQuestGuidance_Init(rom,0x100000);assert(available&&chest_count==33);
 assert(!strcmp(area(0),"ATLAND")&&!strcmp(area(1),"WORSHIP SITE")&&!strcmp(area(7),"GOLD CAVE"));
 assert(!strcmp(area(40),"PHANTOM HILL")&&!strcmp(area(74),"LAGOON CASTLE"));
 assert(!rooms[63].name[0]&&!strcmp(area(63),"EXPLORED AREA")&&!strcmp(area(75),"EXPLORED AREA"));
 for(unsigned i=0;i<75;i++)assert((rooms[i].name[0]!=0)==(i!=63));
 f=fopen(argv[2],"r");assert(f);unsigned n=0,room,x,y,flag;
 while(fscanf(f,"%u %u %u %u",&room,&x,&y,&flag)==4){assert(n<chest_count);MapChest*c=chests+n;
  assert(c->room==room&&c->x==x*32+16&&c->y==y*32+16&&c->flag==flag&&!c->opened&&!chest_seen(c));n++;}
 fclose(f);assert(n==33);unsigned doors=0;for(unsigned i=0;i<75;i++)doors+=rooms[i].count;assert(doors==353);
 MapChest*c=chests;unsigned r=c->room;assert(r==3);
 /* Possessing a progress flag alone never reveals an unseen location. */
 ram[0x4c0+c->flag/8]|=0x80u>>(c->flag&7);observe(r,rooms[r].w-16,rooms[r].h-16,1);
 assert(c->opened&&!chest_seen(c));LagoonMap_Command(LAGOON_MAP_TOGGLE);assert(LagoonMap_PollOpen(0));
 const uint32_t*image;int w,h;assert(LagoonMap_Image(&image,&w,&h));
 unsigned cw=rooms[r].w/16,ch=rooms[r].h/16;double scale=416.0/cw;if(296.0/ch<scale)scale=296.0/ch;
 int ox=24+(416-(int)(cw*scale))/2,oy=92+(296-(int)(ch*scale))/2;
 int mx=ox+(int)(c->x*scale/16),my=oy+(int)(c->y*scale/16);
 assert(image[my*MW+mx-1]==0xff0b121cu);
 ram[0x4c0+c->flag/8]&=~(0x80u>>(c->flag&7));observe(r,c->x+48,c->y,1);assert(!c->opened&&chest_seen(c));
 assert(LagoonMap_Image(&image,&w,&h)&&image[my*MW+mx-1]==GOLD);
 ram[0x4c0+c->flag/8]|=0x80u>>(c->flag&7);observe(r,c->x+48,c->y,1);
 assert(LagoonMap_Image(&image,&w,&h)&&image[my*MW+mx-1]==BG);
 for(unsigned i=0;i<chest_count;i++){
  MapChest*t=chests+i;ram[0x4c0+t->flag/8]|=0x80u>>(t->flag&7);observe(r,c->x+48,c->y,1);assert(t->opened);
  ram[0x4c0+t->flag/8]&=~(0x80u>>(t->flag&7));observe(r,c->x+48,c->y,1);assert(!t->opened);
 }
 observe(r,c->x+48,c->y,2);assert(!open&&chest_seen(c)&&!c->opened);
 printf("PASS33 independently catalogued treasures/353 bounded original exits; hidden flags cannot reveal locations, discovered closed/open markers, all33 MSB-first flags, timeline rewind and no guest writes\n");
 free(rom);return 0;
}
