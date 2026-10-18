/* Reenter only witnessed original recovery waits, without fabricating frames. */
#include "lagoon_native_recovery.h"
#include "snes/interp_bridge.h"
#include "common_rtl.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
static int bytes(unsigned pc,const unsigned char *v,unsigned count) {
  unsigned offset=((pc>>16)&127)*0x8000+(pc&0x7fff);
  return g_rom && offset+count<=0x100000 && !memcmp(g_rom+offset,v,count);
}
static uint64_t source_hash(unsigned pc,unsigned count) {
  unsigned offset=((pc>>16)&127)*0x8000+(pc&0x7fff);
  uint64_t h=UINT64_C(1469598103934665603);
  if(!g_rom || offset+count>0x100000)return 0;
  for(unsigned i=0;i<count;++i)h=(h^g_rom[offset+i])*UINT64_C(1099511628211);
  return h;
}
static const InterpAotContinuation boss_entries[]={
  {0x01B802,0x01C643,0x01B806,0x0180CD,{0x01C694,0x01C694},0,1,0,0,1,3,2,2},
  {0x01B802,0x01C643,0x01B806,0x01C6AF,{0x01C694,0x01C694},0,1,0,0,1,3,2,2},
  {0x01B80B,0x0081B5,0x01B80F,0x0180CD,{0x0081BD,0x0081BD},0,1,0,0,1,3,2,2},
  {0x01B80B,0x0081B5,0x01B80F,0x01C6AF,{0x0081BD,0x0081BD},0,1,0,0,1,3,2,2}
};
static int main_actor_source(void) {
  static const unsigned char stock_call[]={0x20,0xB5,0xA7};
  static const unsigned char centered_call[]={0x20,0xD8,0x83};
  static const unsigned char collision[]={0x20,0xE1,0xA0},nop[]={0xEA,0xEA,0xEA};
  static const unsigned char centered_dispatch[]={
    0x5A,0x9B,0xD0,0x16,0xDA,0x64,0x41,
    0x20,0xFB,0x83,0x20,0x26,0x84,0x20,0x52,0x84,0x20,0x7D,0x84,
    0xFA,0xAD,0x19,0x05,0x8D,0x19,0x05,
    0x7A,0x4C,0xB5,0xA7,0xEA,0xEA,0xEA,0xEA,0xEA};
  /* The existing approved camera QoL replaces this JSR with a balanced
   * PHY/(PHX..PLX)/PLY then JMP A7B5. Both source-owned alternatives retain
   * the SAME real 80CD owner frame. Unknown camera trampolines refuse. */
  return bytes(0x0180CA,stock_call,3) ||
    (bytes(0x0180CA,centered_call,3) && bytes(0x0180C7,collision,3) &&
     bytes(0x01809D,nop,3) && bytes(0x0183D8,centered_dispatch,sizeof(centered_dispatch)));
}
static int boss_source(void) {
  static const unsigned char main_call[]={0x20,0xB5,0xA7};
  return source_hash(0x01B7FE,0x1C)==UINT64_C(0x9BABEB69C189836D) &&
    source_hash(0x01C643,0x98)==UINT64_C(0xD9D6D43B8EC714F4) &&
    source_hash(0x0081AB,0x4C)==UINT64_C(0x64F433499AC470F7) &&
    source_hash(0x0084DE,0xB)==UINT64_C(0xAE13AFE6E9FF1FC7) &&
    main_actor_source() && bytes(0x01C6AC,main_call,3);
}
static int recovery_source(const InterpAotContinuation *d) {
  if(d->continuation_pc24==0x01B806 || d->continuation_pc24==0x01B80F) {
    for(unsigned i=0;i<sizeof(boss_entries)/sizeof(boss_entries[0]);++i)
      if(!memcmp(d,boss_entries+i,sizeof(*d)))return boss_source();
    return 0;
  }
  static const unsigned char wait[]={0xAD,0x38,0x02,0x0D,0x39,0x02,0xD0,0x01,
    0x6B,0xA9,0x40,0x0C,0x3A,0x02,0xAD,0x3A,0x02,0x29,0x40,0xD0,0xF9,0x6B};
  static const unsigned char call[]={0x22,0x85,0x89,0x00};
  static const unsigned char save_x[]={0xDA},restore_x[]={0xFA,0x60};
  static const unsigned char hp[]={0xA5,0xA4,0xCD,0x20,0x05,0xD0,0xF0,0x80,0x28};
  static const unsigned char mp[]={0xA5,0xA4,0xCD,0x22,0x05,0xD0,0xF0};
  static const unsigned char hp_owner[]={0x20,0x87,0xD4},mp_owner[]={0x20,0xB1,0xD4};
  /* The original death-table JSL consumes only its own three-byte frame.
   * After the real queue wait, the outer JSR92C3 frame still returns to9294.
   * Reenter only four RAM instructions, then retain original931E/audio path.
   * The bridge witnesses the wait, owner bytes and timeline; no frame is made. */
  if(d->continuation_pc24==0x019313) {
    static const unsigned char owner[]={0x20,0xC3,0x92};
    return d->call_pc24==0x01930F && d->wait_pc24==0x008985 &&
      d->owner_return_pc24==0x019294 && d->return_sites[0]==0x00898D &&
      d->return_sites[1]==0x00899A && !d->d && d->m==1 && !d->x && !d->e &&
      d->db==1 && d->wait_frame==3 && !d->saved_stack_bytes && d->owner_frame==2 &&
      bytes(0x008985,wait,sizeof(wait)) && bytes(0x019291,owner,sizeof(owner)) &&
      source_hash(0x0192C3,0x2E)==UINT64_C(0xE1DBDD26BFA53066) &&
      source_hash(0x019306,0x18)==UINT64_C(0xA6BDAFB7EA82D8D8) &&
      source_hash(0x0080E9,0x20)==UINT64_C(0xD98520CA8A4BEFBC);
  }
  unsigned hp_call=d->owner_return_pc24==0x01B812 ? 0x01B80F :
      d->owner_return_pc24==0x01D484 ? 0x01D481 : 0;
  unsigned mp_call=d->owner_return_pc24==0x01B815 ? 0x01B812 :
      d->owner_return_pc24==0x01D4AE ? 0x01D4AB : 0;
  return bytes(0x008985,wait,sizeof(wait)) && bytes(d->call_pc24,call,sizeof(call)) &&
    bytes(0x01D4D9,restore_x,sizeof(restore_x)) &&
    (d->continuation_pc24==0x01D4A2 ?
      hp_call && bytes(0x01D487,save_x,sizeof(save_x)) && bytes(hp_call,hp_owner,sizeof(hp_owner)) &&
        bytes(0x01D4A2,hp,sizeof(hp)) :
      d->continuation_pc24==0x01D4CC && mp_call && bytes(0x01D4B1,save_x,sizeof(save_x)) &&
        bytes(mp_call,mp_owner,sizeof(mp_owner)) && bytes(0x01D4CC,mp,sizeof(mp)));
}
/* Only a validated REAL B80B wait take owns this ancestor witness. The
 * four original bytes are saved X followed by the original actor JSR frame.
 * Child tokens still own their own seven-byte frames and actual wait returns. */
static struct {uint64_t generation;uint16_t sp,entry_s;uint32_t owner;
  unsigned phase;unsigned char stack[4];} parent;
static struct {unsigned long long takes[2],hp[2],mp[2],refused[2];unsigned registered,registration_attempts,failed_source_mask;} counts;
static const char *report_path;
static unsigned owner_index(uint32_t pc){return pc==0x01C6AF;}
static void report(void) {
  if(!report_path || !*report_path)return;
  FILE *f=fopen(report_path,"wb");if(!f)return;
  fprintf(f,"{\"registered\":%u,\"registration_attempts\":%u,\"failed_source_mask\":%u,\"main\":{\"takes\":%llu,\"hp_to_mp\":%llu,\"mp_to_plx\":%llu,\"refusals\":%llu},"
    "\"render\":{\"takes\":%llu,\"hp_to_mp\":%llu,\"mp_to_plx\":%llu,\"refusals\":%llu}}\n",
    counts.registered,counts.registration_attempts,counts.failed_source_mask,counts.takes[0],counts.hp[0],counts.mp[0],counts.refused[0],
    counts.takes[1],counts.hp[1],counts.mp[1],counts.refused[1]);fclose(f);
}
static int parent_live(const CpuState *cpu) {
  return parent.phase && parent.generation==RtlStateGeneration() && cpu &&
    cpu->m_flag==1 && !cpu->x_flag && !cpu->emulation && cpu->DB==1 &&
    !cpu->D && parent.sp<=0x1ffb && boss_source() &&
    !memcmp(parent.stack,cpu->ram+parent.sp+1,4);
}
static int recovery_hook(unsigned event,const CpuState *cpu,uint32_t source,
    uint32_t target,const InterpAotContinuation *d,uint16_t owner_entry_s,
    InterpAotAncestor *proposal) {
  if(event==INTERP_CONT_DISCARD){memset(&parent,0,sizeof(parent));return 0;}
  if(event==INTERP_CONT_CALL && source==0x01B80B) {
    /* Even a refused/non-suspending new call invalidates the older ancestor. */
    memset(&parent,0,sizeof(parent));return 0;
  }
  if(event==INTERP_CONT_TAKE && d && target==0x01B80F) {
    if(!recovery_source(d) || !cpu || cpu->S>0x1ffb || owner_entry_s!=cpu->S+2)return 0;
    parent.generation=RtlStateGeneration();parent.sp=cpu->S;parent.entry_s=owner_entry_s;
    parent.owner=d->owner_return_pc24;parent.phase=1;
    memcpy(parent.stack,cpu->ram+cpu->S+1,4);counts.takes[owner_index(parent.owner)]++;
    return 0;
  }
  if(event==INTERP_CONT_CALL && parent.phase &&
     (source==0x01D49E || source==0x01D4C8)) {
    /* A nested/unrelated refill must not reuse an earlier ancestor. Seven
     * bytes are the REAL wait frame, child saved X and child JSR frame. */
    if(!parent_live(cpu) || cpu->S+7!=parent.sp ||
       (source==0x01D49E && parent.phase!=1))parent.phase=0;
    return 0;
  }
  if(event==INTERP_CONT_TAKE && parent.phase && d &&
     d->continuation_pc24==0x01D4CC && d->owner_return_pc24==0x01B815) {
    /* HP can finish synchronously. A validated original MP wait frame proves
     * the subsequent phase without inventing a child return or guest frame. */
    if(parent_live(cpu) && cpu->S+4==parent.sp && owner_entry_s==cpu->S+2)
      parent.phase=2;
    else parent.phase=0;
    return 0;
  }
  if(event!=INTERP_CONT_RETURN || !parent.phase || !d || !proposal)return 0;
  /* Both refills can complete synchronously inside the original outer native
   * call. Its actual final owner return retires the ancestor immediately. */
  if(d->owner_return_pc24==parent.owner){parent.phase=0;return 0;}
  unsigned which=owner_index(parent.owner);
  int hp=d->continuation_pc24==0x01D4A2 && d->owner_return_pc24==0x01B812 && parent.phase==1;
  int mp=d->continuation_pc24==0x01D4CC && d->owner_return_pc24==0x01B815 && parent.phase==2;
  if(!hp && !mp)return 0;
  if(!parent_live(cpu) || cpu->PB!=1 || cpu->S!=parent.sp || owner_entry_s+2!=parent.sp ||
     !recovery_source(d)) {parent.phase=0;counts.refused[which]++;return 0;}
  proposal->pc24=target;proposal->owner_return_pc24=parent.owner;
  proposal->entry_s=parent.entry_s;proposal->owner_frame=2;
  if(hp){parent.phase=2;counts.hp[which]++;}else{parent.phase=0;counts.mp[which]++;}
  return 1;
}
void LagoonNativeRecovery_BeforeFrame(void) {
  static int configured,report_initialized;
  if(!report_initialized){report_initialized=1;report_path=getenv("LAGOON_NATIVE_RECOVERY_REPORT");
    if(report_path && *report_path)atexit(report);}

  static const InterpAotContinuation entries[]={
    {0x01D49E,0x008985,0x01D4A2,0x01D484,{0x00898D,0x00899A},0,1,0,0,1,3,2,2},
    {0x01D4C8,0x008985,0x01D4CC,0x01D4AE,{0x00898D,0x00899A},0,1,0,0,1,3,2,2},
    {0x01D49E,0x008985,0x01D4A2,0x01B812,{0x00898D,0x00899A},0,1,0,0,1,3,2,2},
    {0x01D4C8,0x008985,0x01D4CC,0x01B815,{0x00898D,0x00899A},0,1,0,0,1,3,2,2},
    {0x01930F,0x008985,0x019313,0x019294,{0x00898D,0x00899A},0,1,0,0,1,3,0,2}
  };
  if(!configured && g_rom) {
    InterpAotContinuation all[9];memcpy(all,entries,sizeof(entries));
    memcpy(all+5,boss_entries,sizeof(boss_entries));
    if(report_path && *report_path){counts.registration_attempts++;counts.failed_source_mask=0;
      for(unsigned i=0;i<9;i++)if(!recovery_source(all+i))counts.failed_source_mask|=1u<<i;}
    configured=interp_bridge_set_aot_continuations(all,9,recovery_source);
    if(configured){interp_bridge_set_aot_continuation_hook(recovery_hook);counts.registered=9;}
  }
}
