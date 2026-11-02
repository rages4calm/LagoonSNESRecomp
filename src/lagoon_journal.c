/* Current-adventure recap from original04D0 items and04E0 story flags.
 * Boss bits0..6 and Giles/elder bits10/11 are the same original facts used by
 * the achievement observer. Tablet specifics retain its source-qualified guide.
 * Route corroboration: https://mikesrpgcenter.com/lagoon/walkthrough.html . */
#include "lagoon_journal.h"
#include "lagoon_quest_guidance.h"
#include <string.h>

static int qualified;
static int bit(const uint8_t *r,unsigned base,unsigned n){return !!(r[base+n/8]&(0x80u>>(n&7)));}
static void milestone(LagoonJournalEntry *e,const char *label,int completed){
 unsigned n=e->milestone_count;if(n>=4)return;e->milestone[n]=label;
 if(completed)e->completed_mask|=(uint8_t)(1u<<n);e->milestone_count++;
}
void LagoonJournal_Init(const uint8_t *rom,size_t size){
 qualified=0;if(!rom||size!=0x100000)return;
 uint64_t h=UINT64_C(14695981039346656037);
 for(size_t i=0;i<size;i++)h=(h^rom[i])*UINT64_C(1099511628211);
 qualified=h==UINT64_C(0xfdda951f2c3ccdde);
}
int LagoonJournal_Get(const uint8_t *r,size_t size,LagoonJournalEntry *e){
 if(!e)return 0;memset(e,0,sizeof *e);
 if(!qualified||!r||size<LAGOON_JOURNAL_RAM_BYTES||(r[0x48e]|r[0x48f]<<8)>=75)return 0;
 for(unsigned i=0;i<7;i++)if(bit(r,0x4e0,i))e->defeated_boss_mask|=(uint8_t)(1u<<i);
 int boss=-1;for(unsigned i=0;i<7;i++)if(e->defeated_boss_mask&(1u<<i))boss=(int)i;
 if(boss<0){
  e->chapter="Atland and Gold Cave";
  if(bit(r,0x4e0,0x10)){
   e->title="Back to Gold Cave";
   e->detail="Ask Atland's old woman for help with Gold Cave's door.";
   if(bit(r,0x4d0,0x26)){
    e->title="Samson's locked door";
    e->detail="Use Samson's Key at Gold Cave's first-level locked door.";
   }
  }else{
   e->title="Bring Giles home";
   e->detail="Find Giles in Gold Cave and escort him to Atland.";
  }
  milestone(e,"Giles brought home",bit(r,0x4e0,0x10));
  milestone(e,"Samson defeated",bit(r,0x4e0,0));
 }else if(boss==0){
  e->chapter="Voloh and Philips Castle";
  e->title=bit(r,0x4e0,0x11)?"Inside Philips Castle":"The castle's spell";
  e->detail=bit(r,0x4e0,0x11)?"Seek the guardian of Philips Castle.":"Ask Voloh's villagers about the castle spell.";
  if(bit(r,0x4e0,0x11)&&bit(r,0x4d0,0x27)){
   e->title="The upper floor";e->detail="Use the Key of Philips to reach the upper floor.";
  }else if(bit(r,0x4e0,0x11)&&bit(r,0x4d0,0x28)){
   e->title="The prisoners below";e->detail="Explore the basement with the prison key and Movable Mantle.";
  }
  int spell=bit(r,0x4e0,0x11);
  milestone(e,"All three tablets collected",spell||(bit(r,0x4d0,0x36)&&bit(r,0x4d0,0x37)&&bit(r,0x4d0,0x38)));
  milestone(e,"Elder's spell learned",spell);
  milestone(e,"Natela defeated",bit(r,0x4e0,1));
 }else if(boss==1){
  e->chapter="Denegul and Dwarf Cave";e->title="A way through the mountain";
  e->detail="Seek advice in Denegul about Dwarf Cave.";
  if(bit(r,0x4d0,0x2e)){
   e->title="Cool the shallow lava";e->detail="Cool Dwarf Cave's shallow lava with the Freezing Pot.";
  }else if(bit(r,0x4d0,0x31)){
   e->title="Show the Moon Stone";e->detail="Show the Moon Stone to Denegul's old dwarf.";
  }else if(bit(r,0x4d0,0x30)){
   e->title="The blocked cave";e->detail="Clear Dwarf Cave's rocks with the Powerful Mirror.";
  }else if(bit(r,0x4d0,0x2f)){
   e->title="Return Thor's Pendant";e->detail="Return the pendant to Thor in Denegul.";
  }
  milestone(e,"Natela defeated",bit(r,0x4e0,1));milestone(e,"Eardon defeated",bit(r,0x4e0,2));
 }else if(boss==2){
  e->chapter="Poper and Siegfried Castle";e->title="Help the people of Poper";
  e->detail="Seek Duma in Siegfried Castle; Poper's people can help.";
  if(bit(r,0x4d0,0x2a)){
   e->title="Duma's chamber";e->detail="Use Duma's Key at his chamber in Siegfried Castle.";
  }else if(bit(r,0x4d0,0x29)){
   e->title="Bring the Mint to Poper";e->detail="Give Poper's priest the Mint.";
  }
  milestone(e,"Eardon defeated",bit(r,0x4e0,2));milestone(e,"Duma defeated",bit(r,0x4e0,3));
 }else if(boss==3){
  e->chapter="Lilaty and Ice Cave";e->title="Prepare for the cold";
  e->detail="Ask Lilaty's gnomes about the Nymphs and Ice Cave.";
  if(bit(r,0x4d0,0x34)){
   e->title="Into Ice Cave";e->detail="Equip the Fur Mantle before entering Ice Cave.";
  }else if(bit(r,0x4d0,0x33)){
   e->title="Find Lilaty's liar";e->detail="Question Lilaty's liar with Truth Fire equipped.";
  }
  milestone(e,"Duma defeated",bit(r,0x4e0,3));milestone(e,"Thimale defeated",bit(r,0x4e0,4));
 }else if(boss==4){
  e->chapter="Nymph Spring and Phantom Hill";e->title="Beyond the ice";
  e->detail="Continue through Nymph Spring and Phantom Hill.";
  milestone(e,"Thimale defeated",bit(r,0x4e0,4));milestone(e,"Ella defeated",bit(r,0x4e0,5));
 }else if(boss==5){
  e->chapter="Lagoon Castle";e->title="Reach the floating castle";
  e->detail="Reach Lagoon Castle and search for Felicia.";
  milestone(e,"Ella defeated",bit(r,0x4e0,5));milestone(e,"Battler defeated",bit(r,0x4e0,6));
 }else{
  e->chapter="The Secret Place";e->title="The final pursuit";
  e->detail="Continue to the Secret Place and seek its elder.";
  milestone(e,"Battler defeated",bit(r,0x4e0,6));
 }
 LagoonQuestGuidance q;
 if(LagoonQuestGuidance_Get(r,&q)){
  e->chapter="Voloh and Philips Castle";e->title=q.title;e->detail=q.detail;
  e->marker=q.marker;e->room=q.room;e->x=q.x;e->y=q.y;
 }
 return 1;
}
