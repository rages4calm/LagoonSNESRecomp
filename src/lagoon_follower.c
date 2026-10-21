/* Permanent escorts navigate toward Nasir's recent position. The integration applies
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
static const uint8_t *nav_rom;
static size_t nav_rom_size;
static uint8_t nav_source;
/* A bounded local search, using the NPC's own leading-edge collision probes.
 * The route is disposable; no history backlog or destination ever writes XY. */
enum { NAV_SIDE=129, NAV_NODES=NAV_SIDE*NAV_SIDE, NAV_GRID=4, NAV_LIMIT=4096 };
static struct {
  uint16_t x[1024],y[1024],count,cursor,gx,gy,last_x,last_y;
  unsigned actor,kind,age,blocked,retry;
} nav;
static struct {
  uint16_t cost[NAV_NODES],parent[NAV_NODES],heap[NAV_NODES+1],pos[NAV_NODES];
  uint8_t closed[NAV_NODES];
  unsigned used;
  int bx,by,tx,ty;
} search;
static unsigned word(const uint8_t*r,unsigned a){return r[a]|r[a+1]<<8;}
static unsigned absdiff(unsigned a,unsigned b){return a>b?a-b:b-a;}
static uint64_t hash(const uint8_t*r,size_t size,unsigned pc,unsigned n){
  size_t p=(pc>>16)*32768+(pc&32767);uint64_t h=UINT64_C(14695981039346656037);
  if(!r||p>size||n>size-p)return 0;
  while(n--)h=(h^r[p++])*UINT64_C(1099511628211);return h;
}
void LagoonFollower_Discard(void){uint8_t v=s.valid_rom,b=s.body_source;memset(&s,0,sizeof s);s.valid_rom=v;s.body_source=b;memset(&nav,0,sizeof nav);discard_call();}
void LagoonFollower_Reset(const uint8_t*r,size_t size){
  memset(&s,0,sizeof s);memset(&nav,0,sizeof nav);discard_call();
  nav_rom=r;nav_rom_size=size;
  nav_source=hash(r,size,0x018385,83)==UINT64_C(0xaa6815d0b23960aa)&&
    hash(r,size,0x019bce,1100)==UINT64_C(0xc463c68b4606f575);
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
    /* Thor's authored Wish lead owns +1F bit04 and the target at +39/+3B. */
    hash(r,size,0x02c92f,77)==UINT64_C(0xd4f135da5070060f)&&
    hash(r,size,0x029689,110)==UINT64_C(0x35e34f5bb88e9122)&&
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
/* Read exactly the current source collision map, never the coarse explored map.
 * Type1 probes at 019E0A/9E8E/9F12/9F96 read CURRENT leading edge before adding
 * velocity. Each candidate edge checks every pixel, including backing out of
 * an overlap. Tags1/2/4 and solid80 block NPCs; 0/3/5 permit ordinary walking. */
static int nav_read(const uint8_t*r,unsigned a,unsigned*v){
  unsigned b=(a>>16)&127,p=a&65535;
  if(b==126||b==127){*v=r[(b-126)*65536+p];return 1;}
  if(b<32&&p>=32768){size_t o=b*32768+(p&32767);if(o<nav_rom_size){*v=nav_rom[o];return 1;}}
  return 0;
}
static int nav_tile(const uint8_t*r,int x,int y,unsigned*v){
  unsigned w=word(r,0x53)*8,h=word(r,0x55)*8;
  if(x<0||y<0||(unsigned)x>=w||(unsigned)y>=h||w>2048||h>2048)return 0;
  unsigned map=word(r,0x8b)|r[0x8d]<<16,defs=word(r,0x78)|r[0x7a]<<16,meta;
  unsigned index=((unsigned)y&0xfff0)*8+((unsigned)x>>4);
  if(index>=16384||!nav_read(r,map+index,&meta))return 0;
  return nav_read(r,defs+meta*4+((x&8)>>3)+((y&8)>>2),v);
}
static int nav_step(const uint8_t*r,unsigned x,unsigned y,unsigned d,unsigned pixels){
  static const int8_t probe[4][3][2]={{{11,0},{11,-8},{11,8}},{{0,9},{-10,9},{10,9}},
    {{-11,0},{-11,-8},{-11,8}},{{0,-9},{-10,-9},{10,-9}}};
  static const int8_t vx[]={1,0,-1,0},vy[]={0,1,0,-1};
  if(d>3||pixels>8)return 0;
  for(unsigned n=0;n<pixels;n++){
    for(unsigned q=0;q<3;q++){
      unsigned a;if(!nav_tile(r,(int)x+probe[d][q][0],(int)y+probe[d][q][1],&a)||
        (a&128)||!((a&15)==0||(a&15)==3||(a&15)==5))return 0;
    }
    x+=vx[d];y+=vy[d];
  }
  return 1;
}
static unsigned heuristic(unsigned n){
  int x=search.bx+(int)(n%NAV_SIDE)*NAV_GRID,y=search.by+(int)(n/NAV_SIDE)*NAV_GRID;
  return (unsigned)((x>search.tx?x-search.tx:search.tx-x)+(y>search.ty?y-search.ty:search.ty-y))/NAV_GRID;
}
static int earlier(unsigned a,unsigned b){
  unsigned ha=heuristic(a),hb=heuristic(b),fa=search.cost[a]+ha,fb=search.cost[b]+hb;
  return fa<fb||(fa==fb&&ha<hb);
}
static void heap_up(unsigned p){
  unsigned n=search.heap[p];
  while(p>1&&earlier(n,search.heap[p/2])){search.heap[p]=search.heap[p/2];search.pos[search.heap[p]]=p;p/=2;}
  search.heap[p]=n;search.pos[n]=p;
}
static unsigned heap_pop(void){
  unsigned result=search.heap[1],n=search.heap[search.used--],p=1;
  while(p*2<=search.used){unsigned c=p*2;if(c<search.used&&earlier(search.heap[c+1],search.heap[c]))c++;
    if(!earlier(search.heap[c],n))break;
    search.heap[p]=search.heap[c];search.pos[search.heap[p]]=p;p=c;
  }
  if(search.used){search.heap[p]=n;search.pos[n]=p;}search.pos[result]=0;return result;
}
static int nav_route(const uint8_t*r,unsigned x,unsigned y,unsigned tx,unsigned ty){
  memset(&search,0,sizeof search);memset(search.cost,255,sizeof search.cost);
  search.bx=(int)x-256;search.by=(int)y-256;search.tx=tx;search.ty=ty;
  unsigned start=64*NAV_SIDE+64,finish=start,best=heuristic(start);
  search.cost[start]=0;search.parent[start]=65535;search.heap[++search.used]=start;search.pos[start]=1;
  static const int8_t dx[]={1,0,-1,0},dy[]={0,1,0,-1};
  unsigned examined=0;
  while(search.used&&examined++<NAV_LIMIT){
    unsigned n=heap_pop();search.closed[n]=1;
    unsigned h=heuristic(n);if(h<best){best=h;finish=n;}
    int cx=n%NAV_SIDE,cy=n/NAV_SIDE,px=search.bx+cx*NAV_GRID,py=search.by+cy*NAV_GRID;
    if(px<0||py<0)continue;
    if(absdiff((unsigned)px,tx)+absdiff((unsigned)py,ty)<=5){finish=n;break;}
    for(unsigned d=0;d<4;d++){
      int nx=cx+dx[d],ny=cy+dy[d];if(nx<0||ny<0||nx>=NAV_SIDE||ny>=NAV_SIDE)continue;
      unsigned m=ny*NAV_SIDE+nx,g=search.cost[n]+1;
      if(search.closed[m]||g>=search.cost[m]||!nav_step(r,px,py,d,NAV_GRID))continue;
      search.cost[m]=g;search.parent[m]=n;
      if(!search.pos[m])search.heap[++search.used]=m;
      heap_up(search.pos[m]?search.pos[m]:search.used);
    }
  }
  nav.count=nav.cursor=0;nav.gx=tx;nav.gy=ty;nav.age=0;nav.blocked=0;nav.retry=0;
  if(finish==start)return 0;
  unsigned n=finish;
  while(n!=start&&nav.count<1024){
    nav.x[nav.count]=(uint16_t)(search.bx+(int)(n%NAV_SIDE)*NAV_GRID);
    nav.y[nav.count++]=(uint16_t)(search.by+(int)(n/NAV_SIDE)*NAV_GRID);n=search.parent[n];
  }
  if(n!=start){nav.count=0;return 0;}
  for(unsigned i=0;i<nav.count/2;i++){
    unsigned j=nav.count-1-i;uint16_t v=nav.x[i];nav.x[i]=nav.x[j];nav.x[j]=v;
    v=nav.y[i];nav.y[i]=nav.y[j];nav.y[j]=v;
  }
  return 1;
}
int LagoonFollower_Plan(const uint8_t*r,unsigned q,uint64_t generation,LagoonFollowerStep*out){
  int role=LagoonFollower_Role(r,q);
  if(!role){if(s.following&&q==s.actor){s.following=0;nav.count=0;}return 0;}
  if(!out||!same_scene(r,generation)||!nav_source)return 0;
  unsigned a=0x500+q,x=word(r,a+2),y=word(r,a+4);
  memset(out,0,sizeof *out);out->target_x=x;out->target_y=y;out->direction=r[a+10]&3;
  /* A valid ordinary escort is held, rather than released to the stock chase,
   * while gathering a new trail. Loads never retain an old waypoint backlog. */
  if(nav.actor!=q||nav.kind!=(unsigned)role){memset(&nav,0,sizeof nav);nav.actor=q;nav.kind=role;}
  s.actor=q;s.kind=role;s.following=1;
  if(!s.count)return 1;
  unsigned bootstrap=s.count<=LAGOON_FOLLOWER_DELAY;
  Sample*goal=at(bootstrap?s.count-1:s.count-1-LAGOON_FOLLOWER_DELAY);
  unsigned tx=goal->x,ty=goal->y;
  if(absdiff(x,tx)+absdiff(y,ty)<=(bootstrap?18u:5u)){nav.count=0;return 1;}
  if(nav.count&&x==nav.last_x&&y==nav.last_y)nav.blocked++;else nav.blocked=0;
  while(nav.cursor<nav.count){
    unsigned nx=nav.x[nav.cursor],ny=nav.y[nav.cursor];
    int crossed=(x==nx&&y==ny)||
      (nav.last_y==y&&ny==y&&nx>=(x<nav.last_x?x:nav.last_x)&&nx<=(x>nav.last_x?x:nav.last_x))||
      (nav.last_x==x&&nx==x&&ny>=(y<nav.last_y?y:nav.last_y)&&ny<=(y>nav.last_y?y:nav.last_y));
    if(!crossed)break;nav.cursor++;
  }
  unsigned changed=absdiff(nav.gx,tx)+absdiff(nav.gy,ty);
  if(!nav.count||nav.cursor>=nav.count||changed>=16||nav.blocked>=2||++nav.age>=16){
    if(nav.retry&&changed<16){nav.retry--;return 1;}
    if(!nav_route(r,x,y,tx,ty)){nav.retry=8;return 1;}
  }
  if(nav.cursor>=nav.count)return 1;
  unsigned pace=s.pace?s.pace:3;
  unsigned next=nav.cursor;
  while(next+1<nav.count&&
    ((nav.x[next]==x&&nav.x[next+1]==x&&
      ((nav.y[next]>y&&nav.y[next+1]>nav.y[next])||(nav.y[next]<y&&nav.y[next+1]<nav.y[next])))||
     (nav.y[next]==y&&nav.y[next+1]==y&&
      ((nav.x[next]>x&&nav.x[next+1]>nav.x[next])||(nav.x[next]<x&&nav.x[next+1]<nav.x[next]))))&&
    absdiff(x,nav.x[next])+absdiff(y,nav.y[next])<pace)next++;
  vector(out,x,y,nav.x[next],nav.y[next],pace,0);
  if(out->moving&&!nav_step(r,x,y,out->direction,absdiff(x,out->target_x)+absdiff(y,out->target_y))){
    nav.count=0;out->moving=0;out->dx=out->dy=0;out->target_x=x;out->target_y=y;
  }
  nav.last_x=x;nav.last_y=y;out->connector=1;out->height=goal->z;
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
