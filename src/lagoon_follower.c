/* Permanent escorts consume Nasir's observed route. The integration applies
 * only intent and velocity before the original tile-collision/movement path;
 * no endpoint in this module authorizes a position, door or quest-state write. */
#include "lagoon_follower.h"
#include <string.h>

typedef struct Sample { uint16_t x,y,z; uint32_t serial; } Sample;
static struct {
  Sample points[LAGOON_FOLLOWER_HISTORY];
  uint64_t generation;
  uint32_t serial, cursor;
  unsigned head,count,room,actor,kind,last_tick;
  uint16_t observed_x,observed_y,expected_x,expected_y;
  uint8_t descriptor[56], valid_rom, observed, following, pending, pace,body_source;
  uint8_t joining,axis,tried,join_refused;
  uint16_t last_distance,blocked_x,blocked_y;
} s;
static void discard_call(void);
static unsigned word(const uint8_t*r,unsigned a){return r[a]|r[a+1]<<8;}
static unsigned absdiff(unsigned a,unsigned b){return a>b?a-b:b-a;}
static uint64_t hash(const uint8_t*r,size_t size,unsigned pc,unsigned n){
  size_t p=(pc>>16)*32768+(pc&32767);uint64_t h=UINT64_C(14695981039346656037);
  if(!r||p>size||n>size-p)return 0;
  while(n--)h=(h^r[p++])*UINT64_C(1099511628211);return h;
}
void LagoonFollower_Discard(void){uint8_t v=s.valid_rom,b=s.body_source;memset(&s,0,sizeof s);s.valid_rom=v;s.body_source=b;discard_call();}
void LagoonFollower_Reset(const uint8_t*r,size_t size){
  memset(&s,0,sizeof s);discard_call();
  s.valid_rom=hash(r,size,0x018c4d,7)==UINT64_C(0x4df33da483454035)&&
    hash(r,size,0x018d41,101)==UINT64_C(0x601c693bd3494753)&&
    hash(r,size,0x018da6,32)==UINT64_C(0x38eb4c07305e8bdd)&&
    hash(r,size,0x05cc41,22)==UINT64_C(0x941259f71206e717)&&
    hash(r,size,0x05cc57,33)==UINT64_C(0x76808cf19fc94554)&&
    hash(r,size,0x05ccab,31)==UINT64_C(0xe539f2cbf0fc068c)&&
    hash(r,size,0x05cd3d,25)==UINT64_C(0xd206427893a378bf)&&
    hash(r,size,0x018184,55)==UINT64_C(0x23b0dcfba0bcb3d3)&&
    hash(r,size,0x0280a0,137)==UINT64_C(0x789757cb6332dfac)&&
    hash(r,size,0x019bce,787)==UINT64_C(0xb6e0d065a5fe7925)&&
    hash(r,size,0x01a4d3,151)==UINT64_C(0x1f29c789be3f1c10)&&
    hash(r,size,0x029526,530)==UINT64_C(0xd1ceb882a459c18c)&&
    hash(r,size,0x02c834,94)==UINT64_C(0xb1a94c5708ff8697)&&
    hash(r,size,0x01c84a,279)==UINT64_C(0x11e0631944b7804d);
  /* Camera QoL may replace ONLY the following80CA JSR. The real80C7 body
   * collision call and the source-closed scanner/responses remain original. */
  static const uint8_t stock[]={0x20,0xb5,0xa7};
  static const uint8_t centered[]={0x20,0xd8,0x83};
  static const uint8_t dispatcher[]={0x5a,0x9b,0xd0,0x16,0xda,0x64,0x41,
    0x20,0xfb,0x83,0x20,0x26,0x84,0x20,0x52,0x84,0x20,0x7d,0x84,
    0xfa,0xad,0x19,0x05,0x8d,0x19,0x05,0x7a,0x4c,0xb5,0xa7,0xea,0xea,0xea,0xea,0xea};
  unsigned call_at=0x80ca,center_at=0x83d8;
  s.body_source=s.valid_rom&&size>center_at+sizeof dispatcher&&
    (!memcmp(r+call_at,stock,sizeof stock)||
      (!memcmp(r+call_at,centered,sizeof centered)&&!memcmp(r+center_at,dispatcher,sizeof dispatcher)))&&
    hash(r,size,0x0180c7,3)==UINT64_C(0xc4876c17cf697f6e)&&
    hash(r,size,0x01a0e1,126)==UINT64_C(0x45a9f93b3d634684)&&
    hash(r,size,0x01a15d,230)==UINT64_C(0x631ae22da0af98fa)&&
    hash(r,size,0x01a37e,58)==UINT64_C(0xbc04dec57f44bfda)&&
    hash(r,size,0x01a493,64)==UINT64_C(0xa9dc0c6a28648792);
}
int LagoonFollower_Role(const uint8_t*r,unsigned q){
  if(!s.valid_rom||!r||q<0x40||q>0x800||(q&63))return 0;
  unsigned a=0x500+q,k=r[a+27],room=word(r,0x48e),f=r[0x4e1]&3;
  if((r[a]&0xc1)!=0xc0||r[a+24]!=1||k>1||r[a+1]||
     (r[a+31]&4)||r[a+26]||r[0x41b]||r[0x417]>1||
     (r[0x500]&0xc1)!=0xc0||(r[0x501]&0xc3)||room>=75)return 0;
  if(k==1){if(!(f&1))return 0;}
  else if(f!=2)return 0;
  if(q==0x800)return (int)k+1;
  /* These source entry commands instantiate their sole NPC in slot540.
   * The same kind elsewhere is not proof of an escort role. */
  if(q!=0x40)return 0;
  return k?(room==7?2:0):((room==9||room==0x11||room==0x21)?1:0);
}
static Sample *at(unsigned n){return &s.points[(s.head+n)%LAGOON_FOLLOWER_HISTORY];}
static int same_scene(const uint8_t*r,uint64_t generation){
  return s.observed&&s.generation==generation&&s.room==word(r,0x48e)&&
    !memcmp(s.descriptor,r+0x50,sizeof s.descriptor);
}
static void append(unsigned x,unsigned y,unsigned z){
  if(s.count==LAGOON_FOLLOWER_HISTORY){s.head=(s.head+1)%LAGOON_FOLLOWER_HISTORY;s.count--;}
  Sample*p=at(s.count++);p->x=x;p->y=y;p->z=z;p->serial=++s.serial;
}
int LagoonFollower_Observe(const uint8_t*r,uint64_t generation){
  if(!s.valid_rom||!r)return 0;
  unsigned x=word(r,0x502),y=word(r,0x504),tick=r[0x408];
  /* Original01C643 uses417=0 for static redraw and1 for animation. Both
   * occur under the ordinary80BD actor owner; larger script modes refuse. */
  if((r[0x500]&0xc1)!=0xc0||(r[0x501]&0xc3)||r[0x41b]||r[0x417]>1){
    LagoonFollower_Discard();return 0;
  }
  if(!same_scene(r,generation)){
    LagoonFollower_Discard();s.generation=generation;s.room=word(r,0x48e);
    memcpy(s.descriptor,r+0x50,sizeof s.descriptor);s.observed=1;
    s.observed_x=x;s.observed_y=y;s.last_tick=tick;s.pace=3;
    append(x,y,word(r,0x514));return 1;
  }
  if(tick==s.last_tick)return 1;
  unsigned dx=absdiff(x,s.observed_x),dy=absdiff(y,s.observed_y);
  /* Ordinary movement is cardinal and at most5. A hit/script/teleport or
   * skipped main tick cannot create a new traversable path segment. */
  if((uint8_t)(tick-s.last_tick)!=1||dx+dy>5||(dx&&dy)){
    LagoonFollower_Discard();return 0;
  }
  if(dx+dy)s.pace=(uint8_t)(dx+dy);
  s.observed_x=x;s.observed_y=y;s.last_tick=tick;
  /* Stationary ticks retain the short spatial gap. Recording them as new
   * endpoints would eventually make an escort stand inside Nasir. */
  if(dx+dy)append(x,y,word(r,0x514));return 1;
}
static void vector(LagoonFollowerStep*out,unsigned x,unsigned y,unsigned tx,unsigned ty,unsigned limit,int prefer_y){
  int dx=(int)tx-(int)x,dy=(int)ty-(int)y;
  /* One cardinal original collision pass per tick: never cut a turn. */
  if(dx&&(!prefer_y||!dy)){if(dx>(int)limit)dx=limit;if(dx<-(int)limit)dx=-(int)limit;dy=0;}
  else {if(dy>(int)limit)dy=limit;if(dy<-(int)limit)dy=-(int)limit;}
  if(prefer_y&&dy)dx=0;
  out->dx=(int16_t)dx;out->dy=(int16_t)dy;out->moving=dx||dy;
  if(out->moving)out->direction=dx>0?0:dy>0?1:dx<0?2:3;
  out->target_x=(uint16_t)(x+dx);out->target_y=(uint16_t)(y+dy);
}
int LagoonFollower_Plan(const uint8_t*r,unsigned q,uint64_t generation,LagoonFollowerStep*out){
  int role=LagoonFollower_Role(r,q);
  if(!role){
    /* The ordinary main loop visits every actor. An unrelated enemy/NPC may
     * not retire the escort's cursor or a partially blocked join. */
    if(s.following&&q==s.actor){s.following=s.pending=0;s.joining=s.join_refused=0;}
    return 0;
  }
  if(!out||!same_scene(r,generation))return 0;
  unsigned a=0x500+q,x=word(r,a+2),y=word(r,a+4);
  if(s.following&&(s.actor!=q||s.kind!=(unsigned)role)){s.following=s.pending=0;}
  memset(out,0,sizeof *out);out->target_x=x;out->target_y=y;out->direction=r[a+10]&3;
  if(s.count<=LAGOON_FOLLOWER_DELAY)return 0;
  if(!s.following){
    if(s.join_refused&&s.blocked_x==x&&s.blocked_y==y)return 0;
    s.join_refused=0;
    /* Prefer an exact observed point. A new source-created follower can be
     * beside the route; join only a nearby old point using real collision. */
    unsigned best=0,distance=65535,eligible=s.count-LAGOON_FOLLOWER_DELAY;
    for(unsigned n=0;n<eligible;n++){
      Sample*p=at(n);unsigned d=absdiff(x,p->x)+absdiff(y,p->y);
      if(d<distance){distance=d;best=n;}
    }
    if(distance>32)return 0;
    Sample*p=at(best);s.cursor=p->serial;s.actor=q;s.kind=role;s.following=1;s.pending=0;
    s.joining=distance!=0;s.axis=0;s.tried=0;s.last_distance=0;
  }
  if(s.pending){
    /* Collision may leave us short of the requested point. Keep the same
     * waypoint; no missed step can be consumed on a guessed endpoint. */
    if(x==s.expected_x&&y==s.expected_y){s.cursor++;s.pending=0;s.joining=0;s.tried=0;}
  }
  Sample*goal=0;unsigned eligible=s.count-LAGOON_FOLLOWER_DELAY;
  for(unsigned n=0;n<eligible;n++)if(at(n)->serial==s.cursor){goal=at(n);break;}
  if(!goal){
    if(s.cursor>at(eligible-1)->serial)return 1; /* caught up to delayed trail */
    s.following=s.pending=0;return 0;
  }
  unsigned distance=absdiff(x,goal->x)+absdiff(y,goal->y);
  if(distance>32){s.following=s.pending=0;return 0;}
  if(s.joining&&s.pending&&distance>=s.last_distance){
    /* A real original collision pass failed to approach the short join point.
     * Try its other cardinal axis once; there is no search or guessed jump.
     * If both are blocked, hand authority back to the original escort AI. */
    s.tried|=(uint8_t)(1u<<s.axis);
    unsigned other=s.axis^1u;
    if((s.tried&(1u<<other))||(other?y==goal->y:x==goal->x)){
      s.following=s.pending=0;s.join_refused=1;s.blocked_x=x;s.blocked_y=y;return 0;
    }
    s.axis=(uint8_t)other;
  }
  out->connector=s.joining;
  vector(out,x,y,goal->x,goal->y,out->connector?(s.pace?s.pace:3):5,s.joining&&s.axis);
  if(s.joining&&out->moving)s.axis=out->dy!=0;
  s.last_distance=(uint16_t)distance;out->height=goal->z;
  /* A zero-distance stationary sample still consumes one source main tick. */
  if(!out->moving){s.cursor++;s.pending=0;}
  else {s.expected_x=goal->x;s.expected_y=goal->y;s.pending=1;}
  return 1;
}

#ifndef LAGOON_FOLLOWER_CORE_ONLY
#include "cpu_state.h"
#include "common_rtl.h"
#include "snes/interp_bridge.h"
#include <stdlib.h>
#include <stdio.h>
static struct {
  LagoonFollowerStep step;
  uint64_t generation;
  uint16_t parent_s,actor;
  uint8_t parent[2],tick,active,called;
} call;
static struct {
  const uint8_t *ram;
  uint64_t generation;
  uint16_t parent_s,actor;
  uint8_t parent[2],tick,active;
} body;
static struct {unsigned long long observed,planned,applied,connectors,refused,body_reads;} counts;
static const char *report_path;
static void report(void){
  if(!report_path||!*report_path)return;FILE*f=fopen(report_path,"w");if(!f)return;
  fprintf(f,"{\"schema\":\"Lagoon permanent follower v1\",\"observed\":%llu,\"planned\":%llu,\"applied\":%llu,\"connectors\":%llu,\"refused\":%llu,\"body_read_views\":%llu}\n",
    counts.observed,counts.planned,counts.applied,counts.connectors,counts.refused,counts.body_reads);fclose(f);
}
static void discard_call(void){memset(&call,0,sizeof call);memset(&body,0,sizeof body);}
static int owned_cpu(CpuState*c){
  return c&&c->ram&&s.valid_rom&&c->PB==1&&c->DB==1&&!c->D&&!c->emulation&&
    c->m_flag&&!c->x_flag&&!c->_flag_D&&(c->P&0x38)==0x20&&
    c->S>=0x103&&c->S<=0x1fd&&c->X<=0x800&&!(c->X&63)&&
    c->ram[c->S+1]==0xbd&&c->ram[c->S+2]==0x80&&
    !cpu_dispatch_has_entry(c,0x0181a4)&&!cpu_dispatch_has_entry(c,0x0181ac)&&
    !cpu_dispatch_has_entry(c,0x0181b6)&&!cpu_dispatch_has_entry(c,0x0181ba);
}
static void put(uint8_t*r,unsigned a,unsigned v){r[a]=(uint8_t)v;r[a+1]=(uint8_t)(v>>8);}
static unsigned unique_body_role(const uint8_t*r){
  unsigned a=LagoonFollower_Role(r,0x40)?0x40:0;
  if(LagoonFollower_Role(r,0x800)){if(a)return 0;a=0x800;}
  return a;
}
static void hook(CpuState*c,uint32_t pc){
  pc&=0x7fffff;
  if(pc==0x0180ca){body.active=0;return;}
  if(pc==0x0180c7){
    body.active=0;
    if(!c||!c->ram||!s.body_source||c->PB!=1||c->DB!=1||c->D||
       c->emulation||!c->m_flag||c->x_flag||c->_flag_D||(c->P&0x38)!=0x20||c->X||
       c->S<0x102||c->S>0x1ffd||c->ram[0x518]||
       !same_scene(c->ram,RtlStateGeneration())||!unique_body_role(c->ram)||
       cpu_dispatch_has_entry(c,0x0180c7)||cpu_dispatch_has_entry(c,0x0180ca)||
       cpu_dispatch_has_entry(c,0x01a10d)||cpu_dispatch_has_entry(c,0x01a110))return;
    body.ram=c->ram;body.generation=RtlStateGeneration();body.parent_s=c->S;body.actor=(uint16_t)unique_body_role(c->ram);
    memcpy(body.parent,c->ram+c->S+1,2);body.tick=c->ram[0x408];body.active=1;return;
  }
  if(pc==0x0181ac){
    discard_call();
    if(!owned_cpu(c)||(c->A&255)!=0x11){counts.refused++;return;}
    if(!c->X){if(LagoonFollower_Observe(c->ram,RtlStateGeneration()))counts.observed++;return;}
    LagoonFollowerStep step;
    if(!LagoonFollower_Plan(c->ram,c->X,RtlStateGeneration(),&step))return;
    call.step=step;call.generation=RtlStateGeneration();call.actor=c->X;call.parent_s=c->S;
    call.parent[0]=c->ram[c->S+1];call.parent[1]=c->ram[c->S+2];call.tick=c->ram[0x408];call.active=1;
    /* The real facing and velocity calls still execute. Do not synthesize a
     * jump request: 028199 would otherwise mutate Nasir's global jump state. */
    unsigned a=0x500+c->X;static const uint8_t intents[]={1,4,2,8};
    c->ram[a+0x3e]=0;c->ram[a+0x3f]=step.moving?intents[step.direction]:0;
    counts.planned++;if(step.connector)counts.connectors++;return;
  }
  if(!call.active)return;
  if(!owned_cpu(c)||call.generation!=RtlStateGeneration()||call.actor!=c->X||
     call.parent_s!=c->S||call.tick!=c->ram[0x408]||
     memcmp(call.parent,c->ram+c->S+1,2)||!LagoonFollower_Role(c->ram,c->X)||
     !same_scene(c->ram,call.generation)){
    discard_call();counts.refused++;return;
  }
  if(pc==0x0181b6){
    if((c->A&255)!=0x13){discard_call();counts.refused++;return;}
    call.called=1;return;
  }
  if(pc==0x0181ba){
    /* Real JSL0181B6 pushed B9,81,01 below its untouched80BD owner. Its
     * original child returned to this real RTS with the same live stack. */
    if(!call.called||c->ram[c->S-2]!=0xb9||c->ram[c->S-1]!=0x81||c->ram[c->S]!=1){
      discard_call();counts.refused++;return;
    }
    unsigned a=0x500+c->X;
    put(c->ram,a+6,(uint16_t)call.step.dx);put(c->ram,a+8,(uint16_t)call.step.dy);
    call.called=0;counts.applied++;
  }
}
int LagoonFollower_OwnsVelocity(unsigned a){
  unsigned base=0x500+call.actor;
  return call.active&&call.generation==RtlStateGeneration()&&
    ((a>=base+6&&a<=base+9));
}
uint8_t LagoonFollower_FilterRead(uint32_t a,uint8_t value,const CpuWramReadContext*c,const uint8_t*r){
  if(!body.active||!c||!r||body.ram!=r||!c->interpreter||c->instruction_pc!=0x01a110||
     c->pb!=1||!c->m||c->x||c->e||c->stack!=(unsigned)body.parent_s-2||
     c->y!=body.actor||a!=0x500u+body.actor||value!=r[a]||
     body.generation!=RtlStateGeneration()||body.tick!=r[0x408]||
     r[c->stack+1]!=0xc9||r[c->stack+2]!=0x80||
     memcmp(body.parent,r+body.parent_s+1,2)||unique_body_role(r)!=body.actor||
     !same_scene(r,body.generation)||!LagoonFollower_Role(r,body.actor))return value;
  counts.body_reads++;return value&~8u;
}
void LagoonFollower_BeforeFrame(void){
  static int report_initialized;
  if(!report_initialized){report_initialized=1;report_path=getenv("LAGOON_FOLLOWER_REPORT");if(report_path&&*report_path)atexit(report);}
  if(!s.valid_rom)return;
  /* Add is idempotent and composes with other title hooks. Retry safely if a
   * shared reset cleared registrations; no table-wide reset belongs to us. */
  if(!interp_bridge_add_pre_opcode_hook(0x0181ac,hook)||
     !interp_bridge_add_pre_opcode_hook(0x0181b6,hook)||
     !interp_bridge_add_pre_opcode_hook(0x0181ba,hook)||
     !interp_bridge_add_pre_opcode_hook(0x0180c7,hook)||
     !interp_bridge_add_pre_opcode_hook(0x0180ca,hook))LagoonFollower_Discard();
}
#else
static void discard_call(void){}
void LagoonFollower_BeforeFrame(void){}
int LagoonFollower_OwnsVelocity(unsigned a){(void)a;return 0;}
uint8_t LagoonFollower_FilterRead(uint32_t a,uint8_t v,const struct CpuWramReadContext*c,const uint8_t*r){(void)a;(void)c;(void)r;return v;}
#endif
