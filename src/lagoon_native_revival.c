/* Consume the original item and resume only through genuine guest frames.
 * This policy never edits CPU, inventory, HP, audio, clocks or guest stack. */
#include "lagoon_native_revival.h"
#include "common_rtl.h"
#include <string.h>
static const InterpAotContinuation revival_entry={
  0x01CDDC,0x0081B5,0x01CDE0,0x01D8D4,{0x0081BD,0x0081BD,0,0},
  0,1,0,0,1,3,4,2
};
static uint64_t source_hash(unsigned pc,unsigned count) {
  unsigned offset=((pc>>16)&127)*0x8000+(pc&0x7fff);
  uint64_t h=UINT64_C(1469598103934665603);
  if(!g_rom || offset+count>0x100000)return 0;
  for(unsigned i=0;i<count;i++)h=(h^g_rom[offset+i])*UINT64_C(1099511628211);
  return h;
}
static int source_ok(void) {
  return source_hash(0x01CDB7,0x2C)==UINT64_C(0xFEC520769DB3F04A) &&
    source_hash(0x01D8CE,12)==UINT64_C(0x1B0DF27690B9BC6B) &&
    source_hash(0x01D3CD,28)==UINT64_C(0x36B035BDB4F4FAB8) &&
    source_hash(0x01C643,0x98)==UINT64_C(0xD9D6D43B8EC714F4) &&
    source_hash(0x0081AB,0x4C)==UINT64_C(0x64F433499AC470F7) &&
    source_hash(0x0084DE,0xB)==UINT64_C(0xAE13AFE6E9FF1FC7);
}
unsigned LagoonNativeRevivalDescriptors(InterpAotContinuation *out,unsigned cap) {
  if(!out || !cap)return 0;
  *out=revival_entry;return 1;
}
int LagoonNativeRevivalSource(const InterpAotContinuation *d) {
  return d && !memcmp(d,&revival_entry,sizeof(*d)) && source_ok();
}
static struct {uint64_t generation;uint16_t sp;uint32_t outer;
  unsigned phase;unsigned char stack[11];} witness;
static LagoonNativeRevivalStats stats;
const LagoonNativeRevivalStats *LagoonNativeRevivalGetStats(void){return &stats;}
static int mode_ok(const CpuState *cpu,unsigned pb) {
  return cpu && cpu->ram && cpu->PB==pb && cpu->DB==1 && !cpu->D &&
    cpu->m_flag==1 && !cpu->x_flag && !cpu->emulation;
}
static int outer_source(uint32_t pc) {
  unsigned addr=pc-3,offset=((addr>>16)&127)*0x8000+(addr&0x7fff);
  unsigned target=(pc==0x01BAFB || pc==0x01DD22)?0x9226:
    (pc==0x01D45F || pc==0x01D5B0)?0xD8CE:0;
  /* The health child reaches D8CE by its original tail through the balanced
   * refill. Direct item-use callers have no intervening guest frame. */
  if(target==0x9226 &&
     (source_hash(0x019226,0x34)!=UINT64_C(0x848F89262BE52B3B) ||
      source_hash(0x01D481,6)!=UINT64_C(0x5DE7E52D0F2A36C4) ||
      source_hash(0x01D487,0x54)!=UINT64_C(0x4AAB8E91E2A24C28)))return 0;
  return target && g_rom && g_rom[offset]==0x20 &&
    g_rom[offset+1]==(target&255) && g_rom[offset+2]==(target>>8);
}
static int live(const CpuState *cpu,unsigned pb) {
  return witness.phase && witness.generation==RtlStateGeneration() &&
    mode_ok(cpu,pb) && source_ok() && outer_source(witness.outer) &&
    witness.sp<=0x1ff4 && !memcmp(witness.stack,cpu->ram+witness.sp+1,11);
}
int LagoonNativeRevivalHook(unsigned event,const CpuState *cpu,uint32_t source,
    uint32_t target,const InterpAotContinuation *d,uint16_t owner_entry_s,
    InterpAotAncestor *proposal) {
  if(event==INTERP_CONT_DISCARD){memset(&witness,0,sizeof(witness));return 0;}
  if(event==INTERP_CONT_CALL && source==0x01CDDC) {
    /* A refused, nested or synchronous new call cannot inherit old authority. */
    memset(&witness,0,sizeof(witness));
    if(target!=0x0081B5 || !mode_ok(cpu,0) || cpu->S>0x1ff4 || !source_ok())return 0;
    const unsigned char *s=cpu->ram+cpu->S+1;
    uint32_t outer=0x010000u+(uint16_t)((s[9]|(s[10]<<8))+1);
    if(s[0]!=0xDF || s[1]!=0xCD || s[2]!=1 || s[7]!=0xD3 ||
       s[8]!=0xD8 || !outer_source(outer))return 0;
    witness.generation=RtlStateGeneration();witness.sp=cpu->S;
    witness.outer=outer;witness.phase=1;memcpy(witness.stack,s,11);return 0;
  }
  if(event==INTERP_CONT_TAKE && d && target==0x01CDE0) {
    if(!LagoonNativeRevivalSource(d)) {witness.phase=0;return 0;}
    stats.wait_takes++;
    if(witness.phase!=1 || source!=0x0081BD || !live(cpu,1) ||
       cpu->S!=witness.sp+3 || owner_entry_s!=witness.sp+7) {
      witness.phase=0;stats.refusals++;return 0;
    }
    witness.phase=2;return 0;
  }
  if(event!=INTERP_CONT_RETURN || !d || d->continuation_pc24!=0x01CDE0)return 0;
  int valid=witness.phase==2 && target==0x01D8D4 &&
    LagoonNativeRevivalSource(d) && live(cpu,1) &&
    cpu->S==witness.sp+9 && owner_entry_s==witness.sp+7 && proposal;
  /* Retire before returning even when a caller supplies no proposal storage. */
  witness.phase=0;
  if(!valid){stats.refusals++;return 0;}
  proposal->pc24=0x01D8D4;proposal->owner_return_pc24=witness.outer;
  proposal->entry_s=cpu->S;proposal->owner_frame=2;
  stats.ancestor_returns++;return 1;
}
