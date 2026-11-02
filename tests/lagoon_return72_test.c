/* Real ROM metadata + synthetic opcode/settled-field witnesses. These vectors
 * verify only the pure model, not an executed native loader or gameplay route. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/lagoon_return.c"
static uint8_t rom[0x100000],ram[0x2000],before[0x2000];
static LagoonReturnView v;
static const uint16_t sp=0x1fe0;
static void put(unsigned a,unsigned n){ram[a]=(uint8_t)n;ram[a+1]=(uint8_t)(n>>8);}
static void set_world(unsigned room,unsigned x,unsigned y){
 memset(ram,0,sizeof ram);put(0x48e,room);put(0x53,s.rooms[room].w/8);put(0x55,s.rooms[room].h/8);
 put(0x502,x);put(0x504,y);put(0x520,18);ram[0x500]=0xc0;put(sp+1,0x80a2);
 v.scene_qualified=1;v.host_blocked=v.escort_active=0;
}
static void observe(void){memcpy(before,ram,sizeof ram);LagoonReturn_Observe(&v);assert(!memcmp(before,ram,sizeof ram));}
static LagoonReturnCpu cpu(uint32_t pc,unsigned y,unsigned m){return (LagoonReturnCpu){pc,sp,(uint16_t)y,0,1,1,(uint8_t)m,0,0,(uint8_t)(m?0x20:0),0};}
static const Door *link(unsigned from,unsigned to){
 for(unsigned i=0;i<s.rooms[from].count;i++){const Door*d=&s.doors[s.rooms[from].first+i];if(d->to==to)return d;}return NULL;
}
static void normal_door(const Door*d){
 assert(d);unsigned g=word(ram,0x48e);assert(g==d->from);
 put(0x502,d->sx*32+16);put(0x504,d->sy*32+1);v.tick++;observe();
 put(0x40,s.rooms[g].table);ram[0x42]=5;put(0x102b,d->sx);put(0x102d,d->sy);
 LagoonReturnCpu c=cpu(0x01c8b5,d->payload-s.rooms[g].table,0);
 memcpy(before,ram,sizeof ram);assert(LagoonReturn_WitnessDoor(&v,&c));assert(!memcmp(before,ram,sizeof ram));
 /* Loading and incomplete descriptors cannot grant a new ready field. */
 ram[0x100e]=1;v.scene_qualified=0;v.tick++;observe();
 set_world(d->to,d->x,d->y);v.tick++;observe();
}
static void setup(unsigned group){
 v=(LagoonReturnView){ram,sizeof ram,group+20,1,1,0,0};LagoonReturn_Reset(v.generation);
 set_world(outside[group],128,128);observe();
 normal_door(link(outside[group],entrance[group]));
 assert(s.entry.active&&s.entry.group==group&&s.entry.room==entrance[group]);
 const Room*r=&s.rooms[entrance[group]];const Door*next=NULL;
 for(unsigned i=0;i<r->count;i++){
  const Door*d=&s.doors[r->first+i];if(d->to!=d->from&&d->to<0x40&&s.rooms[d->to].group==group&&link(d->to,d->from)){next=d;break;}
 }
 /* Ice Cave has one ordinary area; returning from its far side is still
  * useful and must reload the witnessed arrival through the original loader. */
 if(next)normal_door(next);
 unsigned room=word(ram,0x48e);put(0x502,s.rooms[room].w/2);put(0x504,s.rooms[room].h/2);
 ram[0x4e0+boss_bit[group]/8]|=(uint8_t)(0x80u>>(boss_bit[group]&7));
 for(unsigned i=0;i<3;i++){v.tick++;observe();}
 LagoonReturnStatus status;LagoonReturn_Status(&v,&status);
 if(status.reason!=LAGOON_RETURN_READY)fprintf(stderr,"group%u room%02x reason%u %s path%u stationary%u\n",group,room,status.reason,status.message,s.path_count,s.stationary);
 assert(status.reason==LAGOON_RETURN_READY);
 assert(status.entrance_room==entrance[group]);
}
static uint8_t filtered(uint32_t pc,unsigned m,unsigned a,uint8_t original){
 LagoonReturnRead c={pc,sp,1,(uint8_t)m,0,0};memcpy(before,ram,sizeof ram);
 uint8_t result=LagoonReturn_FilterRead(&v,&c,a,original);assert(!memcmp(before,ram,sizeof ram));return result;
}
static LagoonReturnCpu activate(void){
 LagoonReturnStatus status;assert(LagoonReturn_Request(&v,&status));
 LagoonReturn_Status(&v,&status);assert(status.reason==LAGOON_RETURN_PENDING);
 assert(!LagoonReturn_Request(&v,&status));
 LagoonReturnCpu c=cpu(0x01c7f6,0,1);v.tick++;
 memcpy(before,ram,sizeof ram);assert(LagoonReturn_Begin(&v,&c));assert(!memcmp(before,ram,sizeof ram));
 assert(!LagoonReturn_InFlight());assert(filtered(0x01c81b,1,0x2c1,0x20)==0x21);
 assert(LagoonReturn_InFlight());
 /* Original instructions, not the model, perform these fixture writes. */
 ram[0x51b]=1;put(0x102b,word(ram,0x502)>>5);put(0x102d,(word(ram,0x504)-1)>>5);
 const Door*d=path_door(word(ram,0x48e));assert(d);
 assert(filtered(0x01c892,0,0x102b,ram[0x102b])==(d->sx&255));
 assert(filtered(0x01c892,0,0x102c,ram[0x102c])==(d->sx>>8));
 assert(filtered(0x01c89b,0,0x102d,ram[0x102d])==(d->sy&255));
 assert(filtered(0x01c89b,0,0x102e,ram[0x102e])==(d->sy>>8));
 return cpu(0x01c8b5,d->payload-s.rooms[d->from].table,0);
}
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);assert(fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 LagoonReturn_Init(rom,sizeof rom);assert(s.qualified&&s.door_count==361);
 for(unsigned group=1;group<7;group++){
  setup(group);Entry original=s.entry;
  unsigned source_room=word(ram,0x48e);put(0x40,s.rooms[source_room].table);ram[0x42]=5;
  LagoonReturnCpu c=activate();LagoonReturnPlan plan;memcpy(before,ram,sizeof ram);
  assert(LagoonReturn_Consume(&v,&c,&plan)==1);assert(!memcmp(before,ram,sizeof ram));
  assert(plan.target_payload==original.payload&&plan.target_room==original.room&&plan.source_room==source_room);
  assert(!LagoonReturn_InFlight()&&LagoonReturn_Consume(&v,&c,&plan)==0);
  set_world(original.room,original.x,original.y);
  ram[0x4e0+boss_bit[group]/8]|=(uint8_t)(0x80u>>(boss_bit[group]&7));v.tick++;observe();
  assert(s.entry.active&&s.path_count==1&&s.path[0]==original.room);
  for(unsigned n=0;n<2;n++){v.tick++;observe();}
  LagoonReturnStatus out;LagoonReturn_Status(&v,&out);assert(out.reason==LAGOON_RETURN_ALREADY_THERE);
 }
 setup(1);LagoonReturnStatus status;
 uint8_t saved[sizeof ram];memcpy(saved,ram,sizeof ram);
 for(unsigned which=0;which<15;which++){
  memcpy(ram,saved,sizeof ram);v.scene_qualified=1;v.host_blocked=v.escort_active=0;
  switch(which){
   case 0:ram[0x4e0]&=127;break;
   case 1:ram[0x100e]=1;break;
   case 2:put(0x514,1);break;
   case 3:ram[0x501]=4;break;
   case 4:ram[0x41b]=1;break;
   case 5:ram[0x417]=2;break;
   case 6:put(0x520,0);break;
   case 7:ram[0x540]=0xc0;ram[0x558]=2;put(0x542,word(ram,0x502)+80);put(0x544,word(ram,0x504));put(0x560,1);break;
   case 8:v.escort_active=1;break;
   case 9:v.host_blocked=1;break;
   case 10:v.scene_qualified=0;break;
   case 11:ram[0x51b]=1;break;
   case 12:ram[0x2c1]=4;break;
   case 13:put(0x502,word(ram,0x502)+1);break;
   case 14:put(0x48e,0x40);break;
  }
  memcpy(before,ram,sizeof ram);LagoonReturn_Status(&v,&status);assert(status.reason!=LAGOON_RETURN_READY);
  assert(!LagoonReturn_Request(&v,&status)&&!memcmp(before,ram,sizeof ram));
 }
 memcpy(ram,saved,sizeof ram);v.scene_qualified=1;v.host_blocked=v.escort_active=0;
 ram[0x540]=0xc0;ram[0x558]=2;put(0x542,word(ram,0x502)+400);put(0x544,word(ram,0x504));put(0x560,1);
 LagoonReturn_Status(&v,&status);assert(status.reason==LAGOON_RETURN_READY); /* Far respawn. */
 put(0x542,word(ram,0x502));ram[0x558]=1;LagoonReturn_Status(&v,&status);assert(status.reason==LAGOON_RETURN_READY); /* Ordinary NPC. */
 v.escort_active=1;LagoonReturn_Status(&v,&status);assert(status.reason==LAGOON_RETURN_ESCORT);v.escort_active=0;
 ram[0x558]=2;put(0x560,0);LagoonReturn_Status(&v,&status);assert(status.reason==LAGOON_RETURN_READY); /* Defeated actor. */
 ram[0x558]=4;LagoonReturn_Status(&v,&status);assert(status.reason==LAGOON_RETURN_ACTIVITY); /* Unknown close projectile/effect. */
 memcpy(ram,saved,sizeof ram);v.scene_qualified=1;v.host_blocked=v.escort_active=0;
 LagoonReturn_Reset(++v.generation);observe();LagoonReturn_Status(&v,&status);
 assert(status.reason==LAGOON_RETURN_MOVING&&s.entry.active); /* Cleared load recovers only canonical ROM entry. */
 for(unsigned n=0;n<2;n++){v.tick++;observe();}LagoonReturn_Status(&v,&status);assert(status.reason==LAGOON_RETURN_READY);
 for(unsigned group=1;group<7;group++){
  LagoonReturn_Reset(++v.generation);
  unsigned room=entrance[group];
  /* Exercise a fresh loaded save without any observed door or prior path. */
  set_world(room,s.rooms[room].w/2,s.rooms[room].h/2);observe();
  LagoonReturn_Status(&v,&status);assert(status.reason==LAGOON_RETURN_UNCLEARED&&!s.entry.active);
  ram[0x4e0+boss_bit[group]/8]|=(uint8_t)(0x80u>>(boss_bit[group]&7));
  for(unsigned n=0;n<3;n++){v.tick++;observe();}
  const Door*canonical=canonical_door(group);assert(canonical);
  assert(s.entry.active&&s.entry.payload==canonical->payload&&s.entry.room==entrance[group]);
  LagoonReturn_Status(&v,&status);assert(status.reason==LAGOON_RETURN_READY);
  put(0x40,s.rooms[room].table);ram[0x42]=5;
  LagoonReturnCpu loaded=activate();LagoonReturnPlan loaded_plan;
  assert(LagoonReturn_Consume(&v,&loaded,&loaded_plan)==1&&loaded_plan.target_payload==canonical->payload);
  set_world(room,canonical->x,canonical->y);
  ram[0x4e0+boss_bit[group]/8]|=(uint8_t)(0x80u>>(boss_bit[group]&7));v.tick++;observe();
  assert(s.path_count==1&&s.path[0]==room); /* Returned path begins at MAIN entrance. */
 }
 setup(1);
 const Door*first=canonical_door(1);assert(first);
 const Door*duplicate=NULL;
 for(unsigned n=0;n<s.rooms[2].count;n++){
  const Door*d=&s.doors[s.rooms[2].first+n];if(d!=first&&d->to==3){duplicate=d;break;}
 }
 assert(duplicate);unsigned changed=((duplicate->payload>>16)&127)*32768+(duplicate->payload&32767);
 rom[changed]^=1; /* One equivalent doorway now points at a DIFFERENT landing. */
 LagoonReturn_Reset(++v.generation);v.tick++;observe();LagoonReturn_Status(&v,&status);
 assert(!canonical_door(1)&&!s.entry.active&&status.reason==LAGOON_RETURN_SOURCE);
 assert(!LagoonReturn_Request(&v,&status));rom[changed]^=1;
 setup(1);const Door*preferred=link(2,3)+1;assert(preferred->from==2&&preferred->to==3);
 LagoonReturn_Reset(++v.generation);set_world(2,128,128);observe();normal_door(preferred);
 assert(s.entry.active&&s.entry.payload==preferred->payload); /* Real session row remains preferred. */
 setup(1);unsigned a=s.entry.payload,off=((a>>16)&127)*32768+(a&32767);
 rom[off]^=1;LagoonReturn_Status(&v,&status);assert(status.reason==LAGOON_RETURN_SOURCE);rom[off]^=1;
 setup(1);put(0x40,s.rooms[word(ram,0x48e)].table);ram[0x42]=5;
 LagoonReturnCpu c=activate();c.y+=11;LagoonReturnPlan plan;
 assert(LagoonReturn_Consume(&v,&c,&plan)==-1); /* Stop, never continue a partial virtual door. */
 LagoonReturn_Cancel();assert(!LagoonReturn_InFlight());
 setup(1);assert(LagoonReturn_Request(&v,&status));c=cpu(0x01c7f6,0,1);c.db=2;
 assert(!LagoonReturn_Begin(&v,&c)&&!s.requested);
 setup(1);assert(LagoonReturn_Request(&v,&status));c=cpu(0x01c7f6,0,1);c.decimal=1;
 assert(!LagoonReturn_Begin(&v,&c)&&!s.requested);
 setup(1);assert(LagoonReturn_Request(&v,&status));c=cpu(0x01c7f6,0,1);c.p|=8;
 assert(!LagoonReturn_Begin(&v,&c)&&!s.requested);
 setup(1);assert(LagoonReturn_Request(&v,&status));c=cpu(0x01c7f6,0,1);assert(LagoonReturn_Begin(&v,&c));
 assert(filtered(0x01c81b,1,0x2c1,4)==4&&!LagoonReturn_InFlight());
 c=cpu(0x0180a3,0,1);c.stack+=2;assert(!LagoonReturn_End(&v,&c)&&!s.requested);
 setup(1);assert(LagoonReturn_Request(&v,&status));c=cpu(0x01c7f6,0,1);assert(LagoonReturn_Begin(&v,&c));
 assert(filtered(0x01c81b,1,0x2c1,0)==1&&LagoonReturn_InFlight());
 c=cpu(0x0180a3,0,1);c.stack+=2;assert(LagoonReturn_End(&v,&c)==-1&&!s.requested);
 rom[0]^=1;LagoonReturn_Init(rom,sizeof rom);assert(!s.qualified);rom[0]^=1;
 puts("PASS return MODEL ONLY:361door records,six natural and six cleared-loaded canonical entrances,preferred session row,ambiguous/changed canonical refusal,one-shotreadviews,15safety refusals,source/generation/partial-contract guards; no RAM/PC/stack writes. Native loader needs actual-route qualification.");
 return 0;
}
