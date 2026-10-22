#include "lagoon_wide_ai.h"
#include "cpu_state.h"
#include <string.h>

typedef struct Span {unsigned pc,bytes;uint64_t hash;} Span;
static const Span source[]={
  {0x028042,0x5e,UINT64_C(0x3bdda5884f57b837)},
  {0x029526,0x82,UINT64_C(0xbd43a7e3c8c68802)},
  {0x02973a,0x1e,UINT64_C(0x7a1754c8b87ce881)},
  {0x0180af,0x1b,UINT64_C(0x2fca83fd1f864bc1)},
  {0x01a7b5,0x21,UINT64_C(0x0cffe57f0d106c45)},
  {0x01ab31,7,UINT64_C(0x72fd7650e48b3ff6)},
  {0x018b84,81,UINT64_C(0xe58d1f2c1e447da1)}
};
static struct {
  const uint8_t *rom;size_t size;const CpuState *cpu;
  uint8_t descriptors[63][56];
  uint64_t generation;int extra,enabled;
  unsigned long long ai_reads,animation_reads;
  struct {const uint8_t *ram;uint32_t pc;uint16_t x,s,y;uint8_t pb,armed;} op;
} wide;
static unsigned word(const uint8_t *r,unsigned a){return r[a]|r[a+1]<<8;}
static uint64_t hash(unsigned pc,unsigned n,uint64_t basis){
  size_t p=(pc>>16)*32768+(pc&32767);uint64_t h=basis;
  if(!wide.rom||p>wide.size||n>wide.size-p)return 0;
  while(n--)h=(h^wide.rom[p++])*UINT64_C(1099511628211);return h;
}
static int source_ok(void){
  if(wide.size!=0x100000)return 0;
  for(unsigned i=0;i<sizeof source/sizeof *source;i++)
    if(hash(source[i].pc,source[i].bytes,UINT64_C(14695981039346656037))!=source[i].hash)return 0;
  const uint8_t *call=wide.rom+0x80ca;
  if(!memcmp(call,"\x20\xb5\xa7",3))return 1;
  /* The approved centered-camera tail retains the real JSR80CA frame. Bind
   * the same complete patched source contract as lagoon_native_camera.c. */
  return !memcmp(call,"\x20\xd8\x83",3)&&
    hash(0x01807a,0x53,UINT64_C(1469598103934665603))==UINT64_C(0xac6a40f151257daf)&&
    hash(0x018218,0x4eb,UINT64_C(1469598103934665603))==UINT64_C(0xe86a6cb1d0e5f630);
}
void LagoonWideAI_Init(const uint8_t *rom,size_t size,const CpuState *cpu){
  memset(&wide,0,sizeof wide);wide.rom=rom;wide.size=size;wide.cpu=cpu;
  if(rom&&size==0x100000)memcpy(wide.descriptors,rom+0x2b2df,sizeof wide.descriptors);
}
void LagoonWideAI_BeginFrame(uint64_t generation,int extra,int ready){
  wide.op.armed=0;wide.generation=generation;wide.extra=extra;
  wide.enabled=ready&&extra>0&&extra<=272&&wide.cpu&&source_ok();
}
static int mode(const CpuState *c,unsigned pb){
  return c&&c==wide.cpu&&c->ram&&c->PB==pb&&c->DB==pb&&!c->D&&!c->emulation&&
    c->m_flag&&!c->x_flag&&!c->_flag_D&&(c->P&0x38)==0x20;
}
void LagoonWideAI_ObserveOpcode(CpuState *c,uint32_t pc,uint64_t generation){
  wide.op.armed=0;
  if(!wide.enabled||generation!=wide.generation||(pc!=0x02973a&&pc!=0x01ab31)||!mode(c,pc>>16))return;
  wide.op.ram=c->ram;wide.op.pc=pc;wide.op.x=c->X;wide.op.s=c->S;
  wide.op.y=c->Y;wide.op.pb=c->PB;wide.op.armed=1;
}
static int ordinary(const uint8_t *r){
  unsigned room=word(r,0x48e);if(room>=63||r[0x406]||r[0x100e]||r[0x23d]!=15||r[0x275]!=0x81||
    (r[0x500]&0xc0)!=0xc0||r[0x518]||!r[0x520]||r[0x521])return 0;
  const uint8_t *d=wide.descriptors[room];
  if(memcmp(d,wide.rom+0x2b2df+room*56,56))return 0;
  static const uint8_t fields[]={0x50,0x53,0x54,0x55,0x56,0x6c,0x6d,0x6e,0x78,0x79,0x7a};
  for(unsigned i=0;i<sizeof fields;i++)if(r[fields[i]]!=d[fields[i]-0x50])return 0;
  unsigned w=word(r,0x53)*8,h=word(r,0x55)*8;
  return w>=256&&w<=2048&&h>=224&&h<=2048&&word(r,0x492)==w-16&&word(r,0x494)==h-14&&
    word(r,0x88)==0&&r[0x8a]==0x7f&&word(r,0x8b)==0x4000&&r[0x8d]==0x7f;
}
uint8_t LagoonWideAI_FilterRead(uint32_t address,uint8_t value,const CpuWramReadContext *c,
    const uint8_t *r,uint64_t generation){
  if(!wide.enabled||generation!=wide.generation||!r||!c||!c->m||c->x||c->e||
     (value&8)||(c->pb!=1&&c->pb!=2))return value;
  unsigned ai=c->pb==2,base=ai?0x540:0x500;
  if(address<base||address>0xd00||((address-base)&63)||value!=r[address])return value;
  unsigned index=address-base,actor=address,s=c->stack;
  /* Real active records end at $0D00; the AI roster is based at0540 rather
   * than0500. Never confuse its rotating X with the main actor-loop X. */
  if((ai?index>0x7c0:index<0x40||index>0x800)||s<0x100||s>(ai?0x1f9:0x1fd)||
     (value&0xc0)!=0xc0||r[actor+0x18]!=2||(r[actor+1]&7)||r[actor+0x1e])return value;
  if(ai){
    unsigned ret=word(r,s+1),rot=word(r,0x3b0);
    if(rot>0x780||(rot&127)||(ret!=0x954a&&ret!=0x9572)||
       index!=rot+(ret==0x9572?64:0)||word(r,s+3)!=0x806e||r[s+5]!=2)return value;
  }else if(word(r,s+1)!=0x80cc)return value;
  if(c->interpreter){
    if(!wide.op.armed||wide.op.pc!=c->instruction_pc||wide.op.pc!=(ai?0x02973a:0x01ab31)||
       wide.op.ram!=r||wide.op.x!=index||wide.op.s!=s||wide.op.y!=c->y||wide.op.pb!=c->pb)return value;
    wide.op.armed=0;
  }else{
    /* The only native extension is AB31's already source-bound actor dispatch.
     * No native AI973A body is authorized without an instruction witness. */
    const CpuState *cpu=wide.cpu;
    if(ai||c->instruction_pc!=UINT32_MAX||!mode(cpu,1)||cpu->ram!=r||cpu->X!=index||cpu->S!=s||cpu->Y!=c->y)return value;
  }
  if(!ordinary(r))return value;
  int x=(int16_t)(word(r,actor+2)-word(r,0x480));
  int y=(int16_t)(word(r,actor+4)-word(r,0x482));
  if(x>=0&&x<256)return value;
  if(x < -wide.extra||x>=256+wide.extra||y<0||y>=224)return value;
  if(word(r,actor+2)>=word(r,0x53)*8||word(r,actor+4)>=word(r,0x55)*8)return value;
  if(ai)wide.ai_reads++;else wide.animation_reads++;
  return value|8;
}
unsigned long long LagoonWideAI_AIReads(void){return wide.ai_reads;}
unsigned long long LagoonWideAI_AnimationReads(void){return wide.animation_reads;}
