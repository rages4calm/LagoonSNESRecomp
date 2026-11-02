#include "../src/lagoon_item_guide.h"
#include "../src/lagoon_menu_presentation.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t rom[0x100000],ram[0x20000],before[0x20000];
static LagoonMenuPanel panel;
static unsigned identifications;
/* Models deliberately delegate PPU authority to the existing independently
 * tested classifier. This harness tests that the delegate's refusal is obeyed;
 * it is not a synthetic claim of a genuine live Items graphics fixture. */
LagoonMenuPanel LagoonMenuPresentation_Identify(const struct Ppu*p,const uint8_t*r){
 assert(p&&r==ram);identifications++;return panel;
}
static void own(unsigned id){ram[0x4d0+id/8]|=(uint8_t)(0x80u>>(id&7));}
static void original_name(unsigned id,char *out){
 unsigned at=0x200a0+id*2;
 unsigned p=0x20000+((rom[at]|rom[at+1]<<8)&32767),n=0;
 for(;n<63;n++){
  unsigned c=rom[p+n];if(c==255)break;
  if(c==254)c=' ';else if(c==92)c='\'';else if(c>=93&&c<=118)c+=4;
  out[n]=(char)c;
 }
 assert(n<63);out[n]=0;
}
int main(int argc,char **argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);
 assert(fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 LagoonItemGuideEntry e;assert(!LagoonItemGuide_Get(0x20,&e));
 LagoonItemGuide_Init(rom,sizeof rom);assert(LagoonItemGuide_Count()==25);
 unsigned prior=0;for(size_t i=0;i<LagoonItemGuide_Count();i++){
  assert(LagoonItemGuide_GetAt(i,&e)&&e.id>prior&&e.name&&e.description&&e.kind);prior=e.id;
  char original[64];original_name(e.id,original);assert(!strcmp(original,e.name));
  LagoonItemGuideEntry copy;assert(LagoonItemGuide_Get(e.id,&copy)&&!strcmp(copy.name,e.name));
 }
 assert(!LagoonItemGuide_GetAt(LagoonItemGuide_Count(),&e)&&!e.name);
 assert(LagoonItemGuide_Get(0x20,&e)&&e.atland_price==10&&e.voloh_price==50);
 assert(LagoonItemGuide_Get(0x21,&e)&&e.voloh_price==10000&&strstr(e.description,"current area"));
 assert(LagoonItemGuide_Get(0x22,&e)&&e.atland_price==20&&strstr(e.description,"20 MP"));
 assert(LagoonItemGuide_Get(0x23,&e)&&e.voloh_price==700);
 assert(!LagoonItemGuide_Get(0x39,&e)&&!e.name);
 unsigned char dummy=0;const struct Ppu *p=(const struct Ppu*)&dummy;
 panel=LAGOON_MENU_ITEMS;
 for(unsigned row=0;row<4;row++)for(unsigned col=0;col<7;col++){
  memset(ram,0,sizeof ram);ram[0x1061]=(uint8_t)col;ram[0x1062]=(uint8_t)row;
  unsigned id=0x20+row*7+col;own(id);
  memcpy(before,ram,sizeof ram);LagoonItemGuideEntry expected;
  int known=LagoonItemGuide_Get(id,&expected);
  assert(LagoonItemGuide_FromInventory(p,ram,sizeof ram,&e)==known);
  if(known)assert(e.id==id);assert(!memcmp(before,ram,sizeof ram));
 }
 memset(ram,0,sizeof ram);own(0x20);
 for(unsigned mode=0;mode<=3;mode++){
  panel=(LagoonMenuPanel)mode;
  assert(LagoonItemGuide_FromInventory(p,ram,sizeof ram,&e)==(mode==LAGOON_MENU_ITEMS));
 }
 panel=LAGOON_MENU_ITEMS;
 ram[0x1061]=7;assert(!LagoonItemGuide_FromInventory(p,ram,sizeof ram,&e));
 ram[0x1061]=0;ram[0x1062]=4;assert(!LagoonItemGuide_FromInventory(p,ram,sizeof ram,&e));
 ram[0x1062]=0;memset(ram+0x4d0,0,16);assert(!LagoonItemGuide_FromInventory(p,ram,sizeof ram,&e));
 ram[0x469]=0x70;panel=LAGOON_MENU_NONE;own(0x20);
 assert(!LagoonItemGuide_FromInventory(p,ram,sizeof ram,&e)); /* stale shop choice */
 unsigned before_calls=identifications;
 assert(!LagoonItemGuide_FromInventory(NULL,ram,sizeof ram,&e));
 assert(!LagoonItemGuide_FromInventory(p,NULL,sizeof ram,&e));
 assert(!LagoonItemGuide_FromInventory(p,ram,LAGOON_ITEM_GUIDE_INVENTORY_RAM_BYTES-1,&e));
 assert(identifications==before_calls);
 assert(!LagoonItemGuide_Owned(ram,sizeof ram,0x1f));
 assert(!LagoonItemGuide_Owned(ram,sizeof ram,0x39));
 assert(!LagoonItemGuide_Owned(ram,LAGOON_ITEM_GUIDE_RAM_BYTES-1,0x20));
 assert(LagoonItemGuide_Owned(ram,LAGOON_ITEM_GUIDE_RAM_BYTES,0x20));
 rom[0xfffff]^=1;LagoonItemGuide_Init(rom,sizeof rom);assert(!LagoonItemGuide_Count());
 assert(!LagoonItemGuide_Get(0x20,&e)&&!LagoonItemGuide_Owned(ram,sizeof ram,0x20));
 rom[0xfffff]^=1;LagoonItemGuide_Init(rom,sizeof rom);assert(LagoonItemGuide_Count()==25);
 puts("PASS itemguide:all25original-name mappings/prices,28grid selections,ownership,bounds,PPUauthority delegation,stale shop/source refusals; no guest writes");
 return 0;
}
