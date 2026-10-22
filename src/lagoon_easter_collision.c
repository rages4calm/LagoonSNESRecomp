/* An ordinary stationary NPC footprint, resolved before original movement.
 * Original A39E/A493 body checks and A1CF/A1FB responses give18x10 clearance.
 * The real A4EB ADCs and camera service run unchanged, using a source-guarded
 * read-only zero-velocity view when a requested step crosses that footprint. */
#include "lagoon_easter_collision.h"
#include "common_rtl.h"
#include "snes/interp_bridge.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static LagoonEasterCollisionBody body_query;
static int qualified;
static unsigned long long blocked_steps,operand_reads;
static struct {
  const uint8_t *ram;
  uint64_t generation;
  uint16_t parent_s,parent_y,x,y,height;
  int16_t dx,dy;
  uint8_t parent[2],tick,flags,active,blocked_x,blocked_y;
} view;
static unsigned word(const uint8_t*r,unsigned a){return r[a]|r[a+1]<<8;}
static unsigned distance(int a,int b){return (unsigned)(a>b?a-b:b-a);}
static uint64_t hash(const uint8_t*rom,size_t size,unsigned pc,unsigned length){
  size_t at=(pc>>16)*32768+(pc&32767);
  uint64_t h=UINT64_C(14695981039346656037);
  if(!rom||at>size||length>size-at)return 0;
  while(length--)h=(h^rom[at++])*UINT64_C(1099511628211);
  return h;
}
static int crossing_axis(int relative,int delta,unsigned radius){
  if(!delta)return 0;
  /* A loaded checkpoint may already overlap a newly solid visitor. Permit
   * motion outward instead of trapping the player or teleporting him out. */
  if(distance(relative,0)<radius)
    return relative<0?delta>0:relative>0?delta<0:0;
  /* Segment tests also stop a larger cardinal displacement that would pass
   * completely through the footprint between two endpoint samples. */
  return relative<0?relative+delta>-(int)radius:relative+delta<(int)radius;
}
static void blocked_axes(int x,int y,int dx,int dy,int bx,int by,
    uint8_t *stop_x,uint8_t *stop_y){
  *stop_x=distance(y,by)<10&&crossing_axis(x-bx,dx,18);
  int after_x=x+(*stop_x?0:dx);
  *stop_y=distance(after_x,bx)<18&&crossing_axis(y-by,dy,10);
}
static void hook(CpuState*c,uint32_t pc){
  pc&=0x7fffff;
  view.active=0;
  if(pc!=0x0180c4||!qualified||!body_query||!c||!c->ram||
     c->PB!=1||c->DB!=1||c->D||c->emulation||!c->m_flag||c->x_flag||
     c->_flag_D||(c->P&0x38)!=0x20||c->X||c->S<0x106||c->S>0x1ffd||
     cpu_dispatch_has_entry(c,0x0180c4)||RtlIsRollbackLoad())return;
  const uint8_t*r=c->ram;
  if((r[0x500]&0xc0)!=0xc0||(r[0x501]&0xc3)||r[0x518])return;
  int bx,by;
  uint64_t generation=RtlStateGeneration();
  if(!body_query(r,generation,&bx,&by))return;
  int dx=(int16_t)word(r,0x506),dy=(int16_t)word(r,0x508);
  if(!dx&&!dy)return;
  unsigned x=word(r,0x502),y=word(r,0x504);
  uint8_t stop_x,stop_y;
  blocked_axes((int)x,(int)y,dx,dy,bx,by,&stop_x,&stop_y);
  if(!stop_x&&!stop_y)return;
  view.ram=r;view.generation=generation;view.parent_s=c->S;view.parent_y=c->Y;
  view.parent[0]=r[c->S+1];view.parent[1]=r[c->S+2];
  view.x=(uint16_t)x;view.y=(uint16_t)y;view.dx=(int16_t)dx;view.dy=(int16_t)dy;
  view.height=(uint16_t)word(r,0x514);view.flags=r[0x501];view.tick=r[0x408];
  view.blocked_x=stop_x;view.blocked_y=stop_y;view.active=1;blocked_steps++;
  const char *trace=getenv("LAGOON_EASTER_COLLISION_TRACE");
  if(trace&&*trace&&strcmp(trace,"0")&&blocked_steps<=16){
    fprintf(stderr,"[cameo-collision] blocked step=%llu x=%u y=%u dx=%d dy=%d\n",
        blocked_steps,x,y,dx,dy);fflush(stderr);
  }
}
void LagoonEasterCollision_Init(const uint8_t*rom,size_t size,
    LagoonEasterCollisionBody body){
  memset(&view,0,sizeof view);body_query=body;blocked_steps=operand_reads=0;
  qualified=size==0x100000&&
    hash(rom,size,0x0180c4,3)==UINT64_C(0xc4b73c17cf9251a0)&&
    hash(rom,size,0x01a4d3,24)==UINT64_C(0x09abeb222846112b)&&
    hash(rom,size,0x01a4eb,90)==UINT64_C(0x574af2c738971269)&&
    hash(rom,size,0x01a39e,26)==UINT64_C(0xfb315b32de5fd7c6)&&
    hash(rom,size,0x01a493,64)==UINT64_C(0xa9dc0c6a28648792);
}
void LagoonEasterCollision_BeforeFrame(void){
  if(!qualified)return;
  if(!interp_bridge_add_pre_opcode_hook(0x0180c4,hook)||
     !interp_bridge_add_pre_opcode_hook(0x0180c7,hook))view.active=0;
}
uint8_t LagoonEasterCollision_FilterRead(uint32_t address,uint8_t value,
    const CpuWramReadContext*c,const uint8_t*r){
  if(!qualified||!view.active||!r||r!=view.ram||!c||c->pb!=1||c->m||c->x||c->e||
     c->stack!=(unsigned)view.parent_s-4||c->y!=view.parent_y||
     view.generation!=RtlStateGeneration()||view.tick!=r[0x408]||
     view.flags!=r[0x501]||view.height!=word(r,0x514)||RtlIsRollbackLoad()||
     memcmp(view.parent,r+view.parent_s+1,2)||word(r,c->stack+1)!=0xa4d5||
     word(r,c->stack+3)!=0x80c6||word(r,0x504)!=view.y||
     (int16_t)word(r,0x506)!=view.dx||(int16_t)word(r,0x508)!=view.dy)return value;
  unsigned axis;
  if(address==0x506||address==0x507){axis=0;if(!view.blocked_x)return value;}
  else if(address==0x508||address==0x509){axis=1;if(!view.blocked_y)return value;}
  else return value;
  unsigned expected_x=(uint16_t)(view.x+(axis&&!view.blocked_x?view.dx:0));
  if(word(r,0x502)!=expected_x)return value;
  if(c->interpreter&&(c->instruction_pc&0x7fffff)!=(axis?0x01a4fb:0x01a4f1))return value;
  int bx,by;
  if(!body_query(r,view.generation,&bx,&by))return value;
  uint8_t stop_x,stop_y;
  blocked_axes(view.x,view.y,view.dx,view.dy,bx,by,&stop_x,&stop_y);
  if((axis?!stop_y:!stop_x)||stop_x!=view.blocked_x||stop_y!=view.blocked_y)return value;
  operand_reads++;return 0;
}
unsigned long long LagoonEasterCollision_BlockedSteps(void){return blocked_steps;}
unsigned long long LagoonEasterCollision_FilteredReads(void){return operand_reads;}
