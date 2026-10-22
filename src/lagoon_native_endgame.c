/* Original inline-table dispatch and Thor script continuations. All hooks are
 * read-only: real CALL/JML/RTL instructions own every lent guest frame. */
#include "lagoon_native_endgame.h"
#include "common_rtl.h"
#include <string.h>

typedef struct {unsigned pc,count;uint64_t hash;} SourceSpan;
static uint64_t hash(unsigned pc,unsigned count) {
  unsigned at=((pc>>16)&127)*0x8000+(pc&0x7fff);
  uint64_t h=UINT64_C(1469598103934665603);
  if(!g_rom || at+count>0x100000)return 0;
  for(unsigned i=0;i<count;i++)h=(h^g_rom[at+i])*UINT64_C(1099511628211);
  return h;
}
static int spans_ok(const SourceSpan *s,unsigned count) {
  for(unsigned i=0;i<count;i++)if(hash(s[i].pc,s[i].count)!=s[i].hash)return 0;
  return 1;
}
static const SourceSpan dispatch_spans[]={
  {0x0080E9,0x20,UINT64_C(0xD98520CA8A4BEFBC)},
  {0x019291,3,UINT64_C(0xBBEA194FD569EA94)},
  {0x0192C3,0x2E,UINT64_C(0xE1DBDD26BFA53066)},
  {0x01B816,3,UINT64_C(0xBE28294FD75169B3)},
  {0x01B81A,0x5B,UINT64_C(0x56F0DFAFDAC8B909)},
  {0x01B5BF,3,UINT64_C(0xBDE4304FD717A8CC)},
  {0x01B936,0x27,UINT64_C(0x939B1EBAF23B5CF3)},
  {0x019382,0x8B,UINT64_C(0x6D643911AD1E59D1)},
  {0x01B88E,0x7D,UINT64_C(0x9398E98327EDFA8C)},
  {0x01B95D,0x49,UINT64_C(0xD141C74D66735581)}
};
typedef struct {unsigned call,owner,table;} InlineOwner;
static const InlineOwner owners[]={
  {0x0192CD,0x019294,0x0192D1},
  {0x01B848,0x01B819,0x01B84C},
  {0x01B939,0x01B5C2,0x01B93D}
};
static int mode(const CpuState *c,unsigned pb) {
  return c && c->ram && c->PB==pb && c->DB==1 && !c->D &&
    c->m_flag==1 && !c->x_flag && !c->emulation && !c->_flag_D &&
    (c->P&(CPU_P_M|CPU_P_X|CPU_P_D))==CPU_P_M;
}
static unsigned word(const unsigned char *s){return s[0]|s[1]<<8;}
static int approved(unsigned i,unsigned target) {
  if(i==0)return target==0x019382;
  if(i==1)return target==0x01B88E || target==0x01B8E2;
  return target==0x01B95D || target==0x01B962 || target==0x01B968 ||
    target==0x01B972 || target==0x01B982 || target==0x01B9A5;
}
static struct {
  const CpuState *cpu;const unsigned char *ram;uint64_t generation;
  unsigned phase,index,target,selector;uint16_t sp,x,y;unsigned char stack[5];
} inline_witness;
static LagoonNativeEndgameStats stats;
/* The bank-table dispatcher has a real JSL owner, unlike80E9's consumed
 * inline frame. Its selector18 reaches the complete HUD-number body only. */
static const SourceSpan hud_spans[]={
  {0x0080B9,0x30,UINT64_C(0x20E6758A603BECFE)},
  {0x028030,2,UINT64_C(0x9A7AC600C5585C57)},
  {0x02D50B,0x43,UINT64_C(0x806CEB29FA113308)},
  {0x00992D,0x3F,UINT64_C(0xAE38002A0E0BD9DD)},
  {0x009879,0x59,UINT64_C(0x504714A03684607F)},
  {0x0089EC,0xB8,UINT64_C(0x62AA24CDE65DA2AF)},
  {0x018B27,6,UINT64_C(0x42D3CE918B9320C3)},
  {0x019442,6,UINT64_C(0x42D3CE918B9320C3)},
  {0x019473,6,UINT64_C(0x42D3CE918B9320C3)}
};
static struct {
  const CpuState *cpu;const unsigned char *ram;uint64_t generation;
  uint16_t sp,x,y;unsigned phase;unsigned char stack[7];
} hud;
static int hud_source(void){return spans_ok(hud_spans,sizeof(hud_spans)/sizeof(*hud_spans));}
static int hud_mode(const CpuState *c,unsigned pb) {
  return c && c->ram && c->PB==pb && c->DB==2 && !c->D && c->m_flag==1 &&
    !c->x_flag && !c->emulation && !c->_flag_D &&
    (c->P&(CPU_P_M|CPU_P_X|CPU_P_D))==CPU_P_M;
}
static int hud_metadata(const CpuState *c) {
  return c->A==0xB && c->ram[0x10]==0xB && c->ram[0x11]==0xD5 &&
    c->ram[0x12]==2 && c->ram[0x13]==0x18;
}
static int hud_live(const CpuState *c) {
  return hud.phase && hud_mode(c,2) && hud.cpu==c && hud.ram==c->ram &&
    hud.generation==RtlStateGeneration() && c->S==hud.sp && c->X==hud.x &&
    c->Y==hud.y && hud_metadata(c) && !memcmp(c->ram+c->S+1,hud.stack,7) && hud_source();
}
static int hud_tail(const CpuState *c,uint32_t source,uint32_t target,InterpAotAncestor *out) {
  int valid=out && source==0x0080E6 && target==0x02D50B && hud.phase==1 && hud_live(c);
  hud.phase=0;
  if(!valid){stats.hud_refusals++;return 0;}
  *out=(InterpAotAncestor){target,0x0080E4,c->S,3};
  hud.phase=2;stats.hud_proposals++;return 1;
}
static int dispatch_source(void) {
  return spans_ok(dispatch_spans,sizeof(dispatch_spans)/sizeof(*dispatch_spans));
}
static int inline_live(const CpuState *c) {
  unsigned i=inline_witness.index;
  return inline_witness.phase && mode(c,1) && inline_witness.cpu==c &&
    inline_witness.ram==c->ram && inline_witness.generation==RtlStateGeneration() &&
    c->S==inline_witness.sp+3 && c->X==inline_witness.x && c->Y==inline_witness.y &&
    !memcmp(c->ram+c->S+1,inline_witness.stack+3,2) &&
    c->ram[0]==inline_witness.selector && word(c->ram+1)==((owners[i].call+3)&65535) && c->ram[3]==1 &&
    word(c->ram+4)==(inline_witness.target&65535) && c->ram[6]==1 &&
    /* PHY/PLY really overwrites two now-dead inline return bytes. */
    word(c->ram+inline_witness.sp+2)==inline_witness.y &&
    c->A==(inline_witness.target>>8 &255) && dispatch_source();
}
static int inline_tail(const CpuState *c,uint32_t source,uint32_t target,
    InterpAotAncestor *out) {
  if(source!=0x008106)return 0;
  unsigned i=inline_witness.index;
  int valid=out && inline_witness.phase==1 && target==inline_witness.target && inline_live(c);
  inline_witness.phase=0;
  if(!valid){stats.refusals++;return 0;}
  *out=(InterpAotAncestor){target,owners[i].owner,c->S,2};
  inline_witness.phase=2;stats.proposals[i]++;return 1;
}

#define WAIT(call,callee,next,owner,ret) \
  {call,callee,next,owner,{ret,0,0,0},0,1,0,0,1,3,0,2}
static const InterpAotContinuation thor_entries[]={
  WAIT(0x019389,0x0080B9,0x01938D,0x019294,0x0080E5),
  WAIT(0x0193B7,0x0081B5,0x0193BB,0x019294,0x0081BD),
  WAIT(0x0193BD,0x0080B9,0x0193C1,0x019294,0x0080E5),
  WAIT(0x0193C3,0x0084E0,0x0193C7,0x019294,0x0084E8),
  WAIT(0x0193C9,0x0080B9,0x0193CD,0x019294,0x0080E5),
  WAIT(0x0193CF,0x0084E0,0x0193D3,0x019294,0x0084E8),
  WAIT(0x0193E3,0x0080B9,0x0193E7,0x019294,0x0080E5),
  WAIT(0x0193E9,0x0084E0,0x0193ED,0x019294,0x0084E8),
  WAIT(0x0193EF,0x0080B9,0x0193F3,0x019294,0x0080E5),
  WAIT(0x0193F6,0x008D82,0x0193FA,0x0193DC,0x008DFD),
  WAIT(0x0193FC,0x008D82,0x019400,0x0193DC,0x008DFD),
  WAIT(0x019402,0x008D82,0x019406,0x0193DC,0x008DFD),
  WAIT(0x019408,0x008D82,0x01940C,0x0193DC,0x008DFD)
};
#undef WAIT
/* Full consumers/helpers, not an invented promise about their global modes.
 * A descriptor takes only after its real RTL with live M1X0/DB1/D0/E0. */
static const SourceSpan wait_spans[]={
  {0x019382,0x8B,UINT64_C(0x6D643911AD1E59D1)},
  {0x0080B9,0x30,UINT64_C(0x20E6758A603BECFE)},
  {0x0081AB,0x4C,UINT64_C(0x64F433499AC470F7)},
  {0x0084DE,0xB,UINT64_C(0xAE13AFE6E9FF1FC7)},
  {0x008D82,0x99,UINT64_C(0xAD49C1C9711365D4)},
  {0x018DC6,0x20,UINT64_C(0x8FCBD12C4BC4E934)},
  {0x01C643,0x98,UINT64_C(0xD9D6D43B8EC714F4)}
};
static int thor_source(void){return dispatch_source() && spans_ok(wait_spans,sizeof(wait_spans)/sizeof(*wait_spans));}
static int descriptor_index(const InterpAotContinuation *d) {
  if(d)for(unsigned i=0;i<13;i++)if(!memcmp(d,thor_entries+i,sizeof(*d)))return (int)i;
  return -1;
}
unsigned LagoonNativeEndgameDescriptors(InterpAotContinuation *out,unsigned cap) {
  if(!out || cap<13)return 0;memcpy(out,thor_entries,sizeof(thor_entries));return 13;
}
int LagoonNativeEndgameSource(const InterpAotContinuation *d) {
  return descriptor_index(d)>=0 && thor_source();
}
static struct {const CpuState *cpu;const unsigned char *ram;uint64_t generation;
  uint16_t sp;unsigned phase;unsigned char stack[4];} queue;
static int queue_live(const CpuState*c) {
  return queue.phase && mode(c,1) && c==queue.cpu && c->ram==queue.ram &&
    queue.generation==RtlStateGeneration() && thor_source() &&
    !memcmp(queue.stack,c->ram+queue.sp+4,4);
}
int LagoonNativeEndgameHook(unsigned event,const CpuState *c,uint32_t source,
    uint32_t target,const InterpAotContinuation *d,uint16_t owner_s,InterpAotAncestor *out) {
  if(event==INTERP_CONT_DISCARD){memset(&inline_witness,0,sizeof(inline_witness));memset(&queue,0,sizeof(queue));memset(&hud,0,sizeof(hud));return 0;}
  if(event==INTERP_CONT_CALL) {
    memset(&hud,0,sizeof(hud));
    if(source==0x0080E0) {
      stats.hud_calls++;
      if(target==0x0080E6 && hud_mode(c,0) && hud_metadata(c) && c->S>=0x100 && c->S<=0x1FF8 &&
          c->ram[c->S+1]==0xE3 && c->ram[c->S+2]==0x80 && !c->ram[c->S+3] &&
          c->ram[c->S+7]==1 &&
          (word(c->ram+c->S+5)==0x8B2C || word(c->ram+c->S+5)==0x9447 ||
           word(c->ram+c->S+5)==0x9478) && hud_source()) {
        hud.cpu=c;hud.ram=c->ram;hud.generation=RtlStateGeneration();hud.sp=c->S;
        hud.x=c->X;hud.y=c->Y;memcpy(hud.stack,c->ram+c->S+1,7);hud.phase=1;stats.hud_armed++;
      }else stats.hud_refusals++;
    }
    /* There are no guest calls inside80E9. Any intervening call retires it. */
    memset(&inline_witness,0,sizeof(inline_witness));
    for(unsigned i=0;i<3;i++)if(source==owners[i].call) {
      stats.calls[i]++;
      if(target!=0x0080E9 || !mode(c,0) || c->S<0x100 || c->S>0x1FFA ||
          c->A>=16 || !dispatch_source() || word(c->ram+c->S+1)!=((source+3)&65535) ||
          c->ram[c->S+3]!=1 || word(c->ram+c->S+4)!=((owners[i].owner-1)&65535)) {
        stats.refusals++;break;
      }
      unsigned at=0x8000+(owners[i].table&32767)+c->A*2;
      unsigned chosen=0x010000|word(g_rom+at);
      if(!approved(i,chosen))break;
      inline_witness.cpu=c;inline_witness.ram=c->ram;inline_witness.generation=RtlStateGeneration();
      inline_witness.sp=c->S;inline_witness.x=c->X;inline_witness.y=c->Y;
      inline_witness.target=chosen;inline_witness.selector=c->A;inline_witness.index=i;inline_witness.phase=1;
      memcpy(inline_witness.stack,c->ram+c->S+1,5);stats.armed[i]++;break;
    }
    if(source==0x0193F6) {
      memset(&queue,0,sizeof(queue));
      if(target==0x008D82 && mode(c,0) && c->S>=0x100 && c->S<=0x1FF8 &&
          word(c->ram+c->S+1)==0x93F9 && c->ram[c->S+3]==1 &&
          word(c->ram+c->S+4)==0x93DB && word(c->ram+c->S+6)==0x9293 && thor_source()) {
        queue.cpu=c;queue.ram=c->ram;queue.generation=RtlStateGeneration();
        queue.sp=c->S;queue.phase=1;memcpy(queue.stack,c->ram+c->S+4,4);
      }
    }
  }
  if(event==INTERP_CONT_TAKE && !d && source==0x008106) {
    if(inline_witness.phase==2 && target==inline_witness.target && owner_s==c->S && inline_live(c))
      stats.takes[inline_witness.index]++;
    else stats.refusals++;
    inline_witness.phase=0;
  }
  if(event==INTERP_CONT_TAKE && !d && source==0x0080E6) {
    if(hud.phase==2 && target==0x02D50B && owner_s==c->S && hud_live(c))stats.hud_takes++;
    else stats.hud_refusals++;
    hud.phase=0;
  }
  int index=descriptor_index(d);
  if(event==INTERP_CONT_TAKE && index>=0) {
    stats.waits[index]++;
    if(index==12) {
      if(queue_live(c) && c->S==queue.sp+3 && owner_s==c->S)queue.phase=2;
      else{queue.phase=0;stats.refusals++;}
    }
  }
  if(event==INTERP_CONT_RETURN && index==12) {
    int valid=out && queue.phase==2 && target==0x0193DC && queue_live(c) &&
      c->S==queue.sp+5 && owner_s==queue.sp+3;
    queue.phase=0;
    if(!valid){stats.refusals++;return 0;}
    *out=(InterpAotAncestor){target,0x019294,c->S,2};stats.queue_ancestors++;return 1;
  }
  return 0;
}
const LagoonNativeEndgameStats *LagoonNativeEndgameGetStats(void){return &stats;}
void LagoonNativeEndgameConsumeInline(unsigned family) {
  /* Native DC replaces this one witness, without retiring unrelated waits. */
  memset(&inline_witness, 0, sizeof inline_witness);
  if (family < 6) ++stats.native_inline[family];
}
void LagoonNativeEndgameBeforeFrame(void) {
  if(g_rom)(void)interp_bridge_set_aot_indirect_long_tail_hook(0x008106,inline_tail);
  if(g_rom)(void)interp_bridge_set_aot_indirect_long_call_tail_hook(0x0080E6,hud_tail);
}
