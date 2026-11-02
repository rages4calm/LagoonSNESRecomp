/* Production return-request model. Native execution integration is separate.
 * Source: main0180A0/01C7F6..C8EB, cell normalization01CA05..CA23,
 * room/door tables05B2DF/058059 and actual source door payloads.
 * A virtual door uses original instructions, never a fabricated call/return.
 * Availability requires the dungeon's ORIGINAL boss-victory bit, an exact
 * original main-entrance payload, safe stationary Nasir and no nearby threats.
 * Natural entrance history is preferred. A cleared save loaded inside may
 * use the unambiguous canonical entrance, never invented coordinates.
 * This does not claim offscreen enemies cannot respawn after a cleared boss. */
#include "lagoon_return.h"
#include <string.h>

enum { ROOMS=75,DOORS=512,NONE=255 };
typedef struct Door {uint32_t payload;uint16_t sx,sy,x,y;uint8_t from,to,facing,flags;} Door;
typedef struct Room {uint32_t table;uint16_t w,h,first,count;uint8_t group;uint64_t table_hash;} Room;
typedef struct Entry {uint32_t payload;uint16_t x,y;uint8_t room,group,active;} Entry;
typedef struct Pending {Door door;uint64_t tick;uint8_t valid,returning;} Pending;
typedef struct State {
 const uint8_t *rom;size_t size;int qualified;
 Room rooms[ROOMS];Door doors[DOORS];unsigned door_count;
 uint64_t generation,last_tick;uint16_t last_x,last_y;
 uint8_t have_observation,last_room,stationary,path[ROOMS],path_count;
 Entry entry;Pending pending;
 LagoonReturnPlan request;uint8_t requested,begun,read_mask;
 uint16_t entry_stack;
 uint64_t code_hash,cell_hash;
} State;
static State s;
static const char *const names[]={"","Gold Cave","Philips Castle","Dwarf Cave","Siegfried Castle","Ice Cave","Lagoon Castle"};
static const uint8_t outside[]={NONE,2,8,0x10,0x1d,0x25,0x2a};
static const uint8_t entrance[]={NONE,3,0x0a,0x12,0x1e,0x26,0x2b};
static const uint8_t boss_bit[]={NONE,0,1,2,3,4,6};
static unsigned word(const uint8_t*r,unsigned a){return r[a]|(unsigned)r[a+1]<<8;}
static uint32_t ptr(const uint8_t*r,unsigned a){return word(r,a)|(uint32_t)r[a+2]<<16;}
static const uint8_t *rom_at(uint32_t a,unsigned n){
 size_t o=((a>>16)&127)*32768+(a&32767);
 return s.rom&&(a&0x8000)&&o<=s.size&&n<=s.size-o?s.rom+o:NULL;
}
static uint64_t hash(const uint8_t*r,size_t n){uint64_t h=UINT64_C(14695981039346656037);while(n--)h=(h^*r++)*UINT64_C(1099511628211);return h;}
static unsigned difference(unsigned a,unsigned b){return a>b?a-b:b-a;}
static unsigned world_distance(unsigned a,unsigned b){unsigned d=(a-b)&65535u;return d>32768?65536-d:d;}
static int view_ok(const LagoonReturnView*v){return v&&v->ram&&v->size>=LAGOON_RETURN_RAM_BYTES;}
static int code_ok(void){
 const uint8_t*a=rom_at(0x01c7f6,246),*b=rom_at(0x01ca05,31),*c=rom_at(0x0180a0,3);
 return a&&b&&c&&c[0]==0x20&&c[1]==0xf6&&c[2]==0xc7&&hash(a,246)==s.code_hash&&hash(b,31)==s.cell_hash;
}
static unsigned group_name(const char*n){
 static const char *const original[]={"","GOLD CAVE","PHILIPS CASTLE","DWARF CAVE","SIEGFRIED CASTLE","ICE CAVE","LAGOON CASTLE"};
 if(!strcmp(n,"SILENCE CAVE"))return 3;
 for(unsigned i=1;i<7;i++)if(!strcmp(n,original[i]))return i;return 0;
}
static int table_ok(unsigned room){
 if(room>=ROOMS)return 0;
 const Room*r=&s.rooms[room];const uint8_t*table=rom_at(r->table,r->count*11+2),*pointer=rom_at(0x058059+room*2,2);
 return table&&pointer&&word(pointer,0)==(r->table&65535)&&hash(table,r->count*11+2)==r->table_hash;
}
static int payload_ok(const Door*d){
 const uint8_t*p=rom_at(d->payload,7),*xy=rom_at(d->payload-4,4),*table=rom_at(0x058059+d->from*2,2);
 if(!p||!xy||!table||d->to>=ROOMS||d->facing>3||!table_ok(d->from)||
  word(xy,0)!=d->sx||word(xy,2)!=d->sy||word(table,0)!=(s.rooms[d->from].table&65535))return 0;
 return word(p,0)==d->x&&word(p,2)==d->y&&p[4]==d->to&&p[5]==d->facing&&p[6]==d->flags&&
  d->x<s.rooms[d->to].w&&d->y<s.rooms[d->to].h;
}
void LagoonReturn_Cancel(void){s.requested=s.begun=s.read_mask=0;memset(&s.request,0,sizeof s.request);}
void LagoonReturn_Reset(uint64_t generation){
 s.generation=generation;s.last_tick=0;s.last_x=s.last_y=0;s.have_observation=0;s.last_room=NONE;s.stationary=0;
 memset(&s.entry,0,sizeof s.entry);memset(&s.pending,0,sizeof s.pending);s.path_count=0;LagoonReturn_Cancel();
}
void LagoonReturn_Init(const uint8_t*rom,size_t size){
 memset(&s,0,sizeof s);s.rom=rom;s.size=size;LagoonReturn_Reset(0);
 if(!rom||size!=0x100000||hash(rom,size)!=UINT64_C(0xfdda951f2c3ccdde))return;
 for(unsigned i=0;i<ROOMS;i++){
  const uint8_t*p=rom_at(0x05b2df+i*56,56),*np=rom_at(0x048000+i*2,2),*dp=rom_at(0x058059+i*2,2);
  if(!p||!np||!dp)return;Room*r=&s.rooms[i];r->w=(uint16_t)(word(p,3)*8);r->h=(uint16_t)(word(p,5)*8);
  if(r->w<256||r->w>2048||r->h<224||r->h>2048)return;
  unsigned address=0x040000|word(np,0);char name[32];unsigned k;
  for(k=0;k<sizeof name-1;k++){
   const uint8_t*c=rom_at(address+k,1);if(!c)return;if(*c==255)break;
   if(*c==254)name[k]=' ';else if(*c>=93&&*c<=118)name[k]=(char)(*c-28);else if(*c>='A'&&*c<='Z')name[k]=(char)*c;else return;
  }if(k==sizeof name-1)return;name[k]=0;r->group=(uint8_t)group_name(name);
  /* Boss-room display names are resource aliases, not progression authority. */
  if(i>=0x40){static const uint8_t b[]={1,2,3,4,5,0,6,0,0,0,0};r->group=b[i-0x40];}
  r->table=0x050000|word(dp,0);r->first=(uint16_t)s.door_count;
  for(unsigned n=0;n<64;n++){
   unsigned a=r->table+n*11;const uint8_t*d=rom_at(a,11);if(!d)return;if(word(d,0)==65535)break;
   if(s.door_count==DOORS||d[8]>=ROOMS||d[9]>3)return;
   s.doors[s.door_count++]=(Door){a+4,(uint16_t)word(d,0),(uint16_t)word(d,2),
    (uint16_t)word(d,4),(uint16_t)word(d,6),(uint8_t)i,d[8],d[9],d[10]};r->count++;
   if(n==63)return;
  }
  const uint8_t*complete=rom_at(r->table,r->count*11+2);if(!complete)return;
  r->table_hash=hash(complete,r->count*11+2);
 }
 s.code_hash=hash(rom_at(0x01c7f6,246),246);s.cell_hash=hash(rom_at(0x01ca05,31),31);s.qualified=1;
}
static const Door *door_at(unsigned room,unsigned y){
 if(room>=ROOMS||y<4||(y-4)%11)return NULL;
 const Room*r=&s.rooms[room];unsigned n=(y-4)/11;return n<r->count?&s.doors[r->first+n]:NULL;
}
static const Door *canonical_door(unsigned group){
 if(!group||group>=7||!table_ok(outside[group]))return NULL;
 const Room*r=&s.rooms[outside[group]];const Door*first=NULL;
 for(unsigned i=0;i<r->count;i++){
  const Door*d=&s.doors[r->first+i];if(d->to!=entrance[group])continue;
  if(!first)first=d;
  else if(d->x!=first->x||d->y!=first->y||d->facing!=first->facing||d->flags!=first->flags)return NULL;
 }
 return first&&payload_ok(first)?first:NULL;
}
static int cpu_ok(const LagoonReturnView*v,const LagoonReturnCpu*c,uint32_t pc,int m){
 return view_ok(v)&&c&&c->pc24==pc&&c->pb==1&&c->db==1&&!c->d&&c->m==m&&!c->x&&!c->e&&!c->decimal&&
  (c->p&0x38)==(m?0x20:0)&&
  c->stack<=0x1ffd&&word(v->ram,c->stack+1)==0x80a2;
}
int LagoonReturn_WitnessDoor(const LagoonReturnView*v,const LagoonReturnCpu*c){
 if(!s.qualified||!cpu_ok(v,c,0x01c8b5,0)||v->generation!=s.generation||v->host_blocked||!code_ok()||
  !s.have_observation||word(v->ram,0x48e)!=s.last_room)return 0;
 unsigned room=word(v->ram,0x48e);const Door*d=door_at(room,c->y);
 if(!d||ptr(v->ram,0x40)!=s.rooms[room].table||word(v->ram,0x102b)!=d->sx||word(v->ram,0x102d)!=d->sy||
  v->ram[0x100e]||(v->ram[0x500]&0xc0)!=0xc0||(v->ram[0x501]&0xf3)||!payload_ok(d))return 0;
 s.pending=(Pending){*d,v->tick,1,0};return 1;
}
static void adopt_pending(unsigned room){
 Door*d=&s.pending.door;unsigned g=s.rooms[room].group;
 if(s.pending.returning&&s.entry.active&&g==s.entry.group&&room==s.entry.room){
  s.path_count=1;s.path[0]=(uint8_t)room;
 }else if(!s.pending.returning&&g&&room==entrance[g]&&d->from==outside[g]){
  s.entry=(Entry){d->payload,d->x,d->y,(uint8_t)room,(uint8_t)g,1};s.path_count=1;s.path[0]=(uint8_t)room;
 }else if(s.entry.active&&g==s.entry.group){
  unsigned n;for(n=0;n<s.path_count;n++)if(s.path[n]==room)break;
  if(n<s.path_count)s.path_count=(uint8_t)(n+1);
  else if(s.path_count<ROOMS)s.path[s.path_count++]=(uint8_t)room;
  else{s.entry.active=0;s.path_count=0;}
 }else{s.entry.active=0;s.path_count=0;}
 s.pending.valid=0;
}
void LagoonReturn_Observe(const LagoonReturnView*v){
 if(!view_ok(v))return;
 if(v->generation!=s.generation||(s.have_observation&&v->tick<s.last_tick))LagoonReturn_Reset(v->generation);
 if(!s.qualified||!v->scene_qualified||v->ram[0x100e])return;
 unsigned room=word(v->ram,0x48e),x=word(v->ram,0x502),y=word(v->ram,0x504);if(room>=ROOMS)return;
 if(word(v->ram,0x53)*8!=s.rooms[room].w||word(v->ram,0x55)*8!=s.rooms[room].h)return;
 int matched=s.pending.valid&&v->tick>=s.pending.tick&&v->tick-s.pending.tick<=900&&
  room==s.pending.door.to&&difference(x,s.pending.door.x)+difference(y,s.pending.door.y)<=16;
 if(matched)adopt_pending(room);
 else if(s.have_observation&&room!=s.last_room){s.entry.active=0;s.path_count=0;s.pending.valid=0;LagoonReturn_Cancel();}
 if(s.pending.valid&&v->tick>=s.pending.tick&&v->tick-s.pending.tick>900)s.pending.valid=0;
 /* A normal save carries story progress, not host door history. Once its
  * dungeon is cleared, the authenticated original area and its equivalent
  * canonical entry rows are enough to recover the SAME original destination.
  * A genuine current-room row still drives the original door scanner/loader. */
 unsigned g=s.rooms[room].group;
 if(!s.entry.active&&room<0x40&&g&&(v->ram[0x4e0+boss_bit[g]/8]&(0x80u>>(boss_bit[g]&7)))){
  const Door*d=canonical_door(g);
  if(d){s.entry=(Entry){d->payload,d->x,d->y,d->to,(uint8_t)g,1};s.path_count=1;s.path[0]=(uint8_t)room;}
 }
 if(s.have_observation&&room==s.last_room&&x==s.last_x&&y==s.last_y&&v->tick>s.last_tick){if(s.stationary<2)s.stationary++;}
 else if(!s.have_observation||room!=s.last_room||x!=s.last_x||y!=s.last_y)s.stationary=0;
 s.last_room=(uint8_t)room;s.last_x=(uint16_t)x;s.last_y=(uint16_t)y;s.last_tick=v->tick;s.have_observation=1;
 if(s.requested&&(v->generation!=s.request.generation||room!=s.request.source_room||
    v->tick<s.request.request_tick||v->tick-s.request.request_tick>120))LagoonReturn_Cancel();
}
static const Door *path_door(unsigned room){
 if(!s.entry.active||!s.path_count||s.path[s.path_count-1]!=room)return NULL;
 unsigned to=s.path_count>1?s.path[s.path_count-2]:NONE;const Room*r=&s.rooms[room];
 for(unsigned i=0;i<r->count;i++){
  const Door*d=&s.doors[r->first+i];
  if((to==NONE||d->to==to)&&d->sx*32<s.rooms[room].w&&d->sy*32<s.rooms[room].h&&payload_ok(d)){
   /* Original scan chooses the FIRST source XY match. Never arm a later
    * alias which could authorize a different earlier destination. */
   unsigned j;for(j=0;j<i;j++)if(s.doors[r->first+j].sx==d->sx&&s.doors[r->first+j].sy==d->sy)break;
   if(j==i)return d;
  }
 }return NULL;
}
static int entry_source_ok(void){
 for(unsigned i=0;i<s.door_count;i++){
  const Door*d=&s.doors[i];
  if(d->payload==s.entry.payload&&d->from==outside[s.entry.group])return payload_ok(d)&&
   d->to==s.entry.room&&d->x==s.entry.x&&d->y==s.entry.y;
 }return 0;
}
static LagoonReturnReason reason(const LagoonReturnView*v){
 if(!s.qualified||!code_ok())return LAGOON_RETURN_SOURCE;
 if(!view_ok(v)||!v->scene_qualified||v->generation!=s.generation||!s.have_observation||
  v->tick<s.last_tick||v->tick-s.last_tick>1||v->ram[0x100e])return LAGOON_RETURN_WORLD;
 const uint8_t*r=v->ram;unsigned room=word(r,0x48e);if(room>=ROOMS||!s.rooms[room].group)return LAGOON_RETURN_AREA;
 if(room>=0x40)return LAGOON_RETURN_BOSS;
 if(!table_ok(room))return LAGOON_RETURN_SOURCE;
 if(v->host_blocked)return LAGOON_RETURN_HOST;
 unsigned g=s.rooms[room].group;
 if(!(r[0x4e0+boss_bit[g]/8]&(0x80u>>(boss_bit[g]&7))))return LAGOON_RETURN_UNCLEARED;
 if(!s.entry.active||s.entry.group!=g)return canonical_door(g)?LAGOON_RETURN_NO_ENTRY:LAGOON_RETURN_SOURCE;
 if(!entry_source_ok())return LAGOON_RETURN_SOURCE;
 if(!path_door(room))return LAGOON_RETURN_PATH;
 if(v->escort_active)return LAGOON_RETURN_ESCORT;
 if((r[0x500]&0xc1)!=0xc0||r[0x501]||r[0x518]||r[0x41b]||r[0x417]>1||word(r,0x514)||!word(r,0x520)||
  (r[0x51b]&1)||(r[0x2c1]&15))return LAGOON_RETURN_ACTION;
 if(s.stationary<2||word(r,0x502)!=s.last_x||word(r,0x504)!=s.last_y)return LAGOON_RETURN_MOVING;
 if(!word(r,0x504)||word(r,0x502)>=s.rooms[room].w||word(r,0x504)>=s.rooms[room].h)return LAGOON_RETURN_ACTION;
 /* Original0180AF/01AB31 and02973A identify kind2 as ordinary hostile
  * actors; kind1 is an NPC (escorts are refused separately). Unknown active
  * kinds near Nasir may be a projectile/effect and conservatively refuse.
  * Distant respawns do not defeat the already-completed dungeon-boss gate. */
 for(unsigned a=0x540;a<0xd40;a+=64)if((r[a]&0xc0)==0xc0&&r[a+0x18]!=1&&
  world_distance(word(r,a+2),word(r,0x502))<=160&&world_distance(word(r,a+4),word(r,0x504))<=128&&
  (r[a+0x18]!=2||word(r,a+0x20)))return LAGOON_RETURN_ACTIVITY;
 if(room==s.entry.room&&difference(word(r,0x502),s.entry.x)+difference(word(r,0x504),s.entry.y)<=32)return LAGOON_RETURN_ALREADY_THERE;
 return LAGOON_RETURN_READY;
}
void LagoonReturn_Status(const LagoonReturnView*v,LagoonReturnStatus*out){
 if(!out)return;memset(out,0,sizeof *out);out->reason=reason(v);
 static const char*const messages[]={"Return inside the dungeon's main entrance.","Return is unavailable for this version of the game.",
  "Wait for the area to finish loading.","Return is available inside supported dungeons.","Leave the boss arena first.",
  "Defeat this dungeon's boss before returning.","The main entrance is unavailable for this area.","No safe route to the main entrance is available.",
  "Stop moving before opening this menu.","Stand safely on clear ground first.","Finish the escort before returning.",
  "Wait for nearby activity to clear.","Return is unavailable with the current play settings.","You are already near the entrance.","Returning to the dungeon's main entrance."};
 out->message=messages[out->reason];
 if(s.entry.active){out->dungeon=names[s.entry.group];out->entrance_room=s.entry.room;out->entrance_x=s.entry.x;out->entrance_y=s.entry.y;}
 if(s.requested&&out->reason==LAGOON_RETURN_READY){out->reason=LAGOON_RETURN_PENDING;out->message=messages[out->reason];}
}
int LagoonReturn_Request(const LagoonReturnView*v,LagoonReturnStatus*out){
 LagoonReturnStatus status;LagoonReturn_Status(v,&status);if(out)*out=status;
 if(status.reason!=LAGOON_RETURN_READY)return 0;
 const Door*d=path_door(word(v->ram,0x48e));if(!d)return 0;
 s.request=(LagoonReturnPlan){v->generation,v->tick,d->payload,s.entry.payload,d->sx,d->sy,
  (uint16_t)(d->payload-4-s.rooms[d->from].table+4),d->from,s.entry.room};
 s.requested=1;s.begun=s.read_mask=0;return 1;
}
int LagoonReturn_Begin(const LagoonReturnView*v,const LagoonReturnCpu*c){
 if(!s.requested||s.begun)return 0;
 if(!cpu_ok(v,c,0x01c7f6,1)||reason(v)!=LAGOON_RETURN_READY){LagoonReturn_Cancel();return 0;}
 if(v->generation!=s.request.generation||v->tick<s.request.request_tick||v->tick-s.request.request_tick>120)return 0;
 s.begun=1;s.read_mask=0;s.entry_stack=c->stack;return 1;
}
int LagoonReturn_InFlight(void){return s.requested&&s.begun&&(s.read_mask&1);}
int LagoonReturn_End(const LagoonReturnView*v,const LagoonReturnCpu*c){
 if(!s.requested||!s.begun||!c||c->pc24!=0x0180a3)return 0;
 int failed=LagoonReturn_InFlight()?-1:0;
 if(!view_ok(v)||v->generation!=s.request.generation||c->stack!=s.entry_stack+2||
   c->pb!=1||c->db!=1||c->d||!c->m||c->x||c->e||c->decimal||(c->p&0x38)!=0x20)failed=-1;
 LagoonReturn_Cancel();return failed;
}
uint8_t LagoonReturn_FilterRead(const LagoonReturnView*v,const LagoonReturnRead*c,uint32_t a,uint8_t value){
 if(!s.requested||!s.begun||!view_ok(v)||!c||v->generation!=s.request.generation||v->tick<s.request.request_tick||
  v->tick-s.request.request_tick>120||word(v->ram,0x48e)!=s.request.source_room||c->stack!=s.entry_stack||
  c->pb!=1||c->x||c->e)return value;
 if(c->pc24==0x01c81b&&c->m==1&&a==0x2c1&&!(value&15)){
  s.read_mask|=1;return (uint8_t)((value&0xf0)|1);
 }
 if(!(s.read_mask&1)||c->m)return value;
 if(c->pc24==0x01c892&&(a==0x102b||a==0x102c)){
  s.read_mask|=(uint8_t)(a==0x102b?2:4);return (uint8_t)(s.request.source_cell_x>>(8*(a-0x102b)));
 }
 if(c->pc24==0x01c89b&&(s.read_mask&6)==6&&(a==0x102d||a==0x102e)){
  s.read_mask|=(uint8_t)(a==0x102d?8:16);return (uint8_t)(s.request.source_cell_y>>(8*(a-0x102d)));
 }
 return value;
}
int LagoonReturn_Consume(const LagoonReturnView*v,const LagoonReturnCpu*c,LagoonReturnPlan*out){
 int failure=LagoonReturn_InFlight()?-1:0;
 if(out)memset(out,0,sizeof *out);
 if(!out||!s.requested||!s.begun||s.read_mask!=31||!cpu_ok(v,c,0x01c8b5,0)||!code_ok()||
  v->generation!=s.request.generation||v->tick<s.request.request_tick||v->tick-s.request.request_tick>120||
  c->stack!=s.entry_stack||c->y!=s.request.source_y||word(v->ram,0x48e)!=s.request.source_room||
  ptr(v->ram,0x40)!=s.rooms[s.request.source_room].table||v->ram[0x100e]||
  !(v->ram[0x51b]&1)||word(v->ram,0x102b)!=(word(v->ram,0x502)>>5)||
  word(v->ram,0x102d)!=((word(v->ram,0x504)-1)>>5))return failure;
 const Door*d=door_at(s.request.source_room,c->y);if(!d||d->payload!=s.request.source_payload||!payload_ok(d))return failure;
 const Door*target=NULL;for(unsigned i=0;i<s.door_count;i++)if(s.doors[i].payload==s.request.target_payload&&
  s.doors[i].from==outside[s.entry.group]){target=&s.doors[i];break;}
 if(!target||!payload_ok(target)||target->to!=s.entry.room||!s.entry.active)return failure;
 *out=s.request;s.pending=(Pending){*target,v->tick,1,1};LagoonReturn_Cancel();return 1;
}
