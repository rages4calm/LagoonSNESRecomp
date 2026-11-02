/* Read-only item help for the clean USA game. 02B73C's item selection,
 * 01D401's dispatcher, 01D056's shop records and 0191C2's regeneration branch
 * establish effects/prices. Consumables20..24 converge on01D8CE ->01CDB7,
 * which clears the selected04D0 inventory bit and resets0536 toFF.
 * Original0480A0 +2*inventoryID establishes every
 * displayed name (in particular2C=Movable Mantle,31=Moon Stone).
 * No guest bus reads, activation or item grants.
 * Descriptive corroboration: https://mikesrpgcenter.com/lagoon/items.html . */
#include "lagoon_item_guide.h"
#include "lagoon_menu_presentation.h"
#include <string.h>

static int qualified;
static const LagoonItemGuideEntry entries[] = {
 {0x20,"Healing Pot","Recovery","Recover 30 HP. Consumed on use.",10,50},
 {0x21,"Shiny Ball","Recovery","Boost HP and MP recovery in the current area. Consumed on use.",0,10000},
 {0x22,"Shiny Stone","Recovery","Recover 20 MP. Consumed on use.",20,0},
 {0x23,"Elixir","Recovery","Refill HP to your level's maximum. Consumed on use.",0,700},
 {0x24,"Bright Stone","Recovery","Refill MP to your level's maximum. Consumed on use.",0,0},
 {0x25,"Life Ball","Equipment","Equip before a fatal hit to revive with full HP.",0,0},
 {0x26,"Samson's Key","Key","Opens Samson's locked door in Gold Cave.",0,0},
 {0x27,"Key of Philips","Key","Opens the way to the upper floor of Philips Castle.",0,0},
 {0x28,"Key of Prison","Key","Opens the prison gates in Philips Castle.",0,0},
 {0x29,"Mint","Quest","Bring to Poper's priest to help the sick villagers.",0,0},
 {0x2a,"Duma's Key","Key","Opens Duma's chamber in Siegfried Castle.",0,0},
 {0x2b,"Key of Siegfried","Key","Opens the way to Siegfried Castle's upper floor.",0,0},
 {0x2c,"Movable Mantle","Quest","Use to rescue the captive elves in Philips Castle.",0,0},
 {0x2d,"Ancient Book","Quest","Read about the Moon Blade and the struggle against darkness.",0,0},
 {0x2e,"Freezing Pot","Quest","Use at the shallow lava in Dwarf Cave to reveal a passage.",0,0},
 {0x2f,"Thor's Pendant","Quest","Return to Thor in Denegul.",0,0},
 {0x30,"Powerful Mirror","Quest","Use on the rocks blocking Dwarf Cave.",0,0},
 {0x31,"Moon Stone","Quest","Show to Denegul's dwarf; later use in Lagoon Castle's moonlight.",0,0},
 {0x32,"Angel's Bell","Quest","Use at Nymph Spring after passing through Ice Cave.",0,0},
 {0x33,"Truth Fire","Equipment","Equip while questioning Lilaty's liar.",0,0},
 {0x34,"Fur Mantle","Equipment","Equip in Ice Cave to protect against the cold.",0,0},
 {0x35,"Statue","Quest","Use by Felicia's glass prison in Lagoon Castle.",0,0},
 {0x36,"Tablet of Faith","Quest","Bring with the other two tablets to Voloh's elder.",0,0},
 {0x37,"Tablet of Wish","Quest","Bring with the other two tablets to Voloh's elder.",0,0},
 {0x38,"Tablet of Hope","Quest","Bring with the other two tablets to Voloh's elder.",0,0}
};

void LagoonItemGuide_Init(const uint8_t *rom,size_t size){
 qualified=0;if(!rom||size!=0x100000)return;
 uint64_t h=UINT64_C(14695981039346656037);
 for(size_t i=0;i<size;i++)h=(h^rom[i])*UINT64_C(1099511628211);
 qualified=h==UINT64_C(0xfdda951f2c3ccdde);
}
size_t LagoonItemGuide_Count(void){return qualified?sizeof entries/sizeof *entries:0;}
int LagoonItemGuide_GetAt(size_t index,LagoonItemGuideEntry *out){
 if(!out)return 0;memset(out,0,sizeof *out);
 if(index>=LagoonItemGuide_Count())return 0;*out=entries[index];return 1;
}
int LagoonItemGuide_Get(unsigned id,LagoonItemGuideEntry *out){
 if(!out)return 0;memset(out,0,sizeof *out);if(!qualified)return 0;
 for(size_t i=0;i<sizeof entries/sizeof *entries;i++)if(entries[i].id==id){*out=entries[i];return 1;}
 return 0;
}
int LagoonItemGuide_Owned(const uint8_t *ram,size_t size,unsigned id){
 if(!qualified||!ram||size<LAGOON_ITEM_GUIDE_RAM_BYTES||id<0x20||id>0x38)return 0;
 return !!(ram[0x4d0+id/8]&(0x80u>>(id&7)));
}
int LagoonItemGuide_FromInventory(const struct Ppu *ppu,const uint8_t *ram,
                                 size_t size,LagoonItemGuideEntry *out){
 if(!out)return 0;memset(out,0,sizeof *out);
 if(!qualified||!ppu||!ram||size<LAGOON_ITEM_GUIDE_INVENTORY_RAM_BYTES||
    LagoonMenuPresentation_Identify(ppu,ram)!=LAGOON_MENU_ITEMS)return 0;
 unsigned column=ram[0x1061],row=ram[0x1062];
 if(column>6||row>3)return 0;
 unsigned id=0x20+row*7+column;
 if(!LagoonItemGuide_Owned(ram,size,id))return 0;
 return LagoonItemGuide_Get(id,out);
}
