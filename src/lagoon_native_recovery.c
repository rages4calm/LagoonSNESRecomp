/* Reenter only witnessed original recovery waits, without fabricating frames. */
#include "lagoon_native_recovery.h"
#include "lagoon_native_revival.h"
#include "lagoon_native_camera.h"
#include "lagoon_native_endgame.h"
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
/* Original PHA flags (one byte), then PHX (two bytes), remain below
 * each REAL render/wait frame and above its source-owned JSR9346 frame. */
static const InterpAotContinuation flash_entries[]={
  {0x019364,0x01C643,0x019368,0x019321,{0x01C694,0x01C694},0,1,0,0,1,3,3,2},
  {0x019364,0x01C643,0x019368,0x01B449,{0x01C694,0x01C694},0,1,0,0,1,3,3,2},
  {0x01936D,0x0081B5,0x019371,0x019321,{0x0081BD,0x0081BD},0,1,0,0,1,3,3,2},
  {0x01936D,0x0081B5,0x019371,0x01B449,{0x0081BD,0x0081BD},0,1,0,0,1,3,3,2}
};
static int flash_source(void) {
  static const unsigned char call[]={0x20,0x46,0x93};
  return source_hash(0x019346,0x3C)==UINT64_C(0x6C8672DD755B970E) &&
    source_hash(0x01C643,0x98)==UINT64_C(0xD9D6D43B8EC714F4) &&
    source_hash(0x0081AB,0x4C)==UINT64_C(0x64F433499AC470F7) &&
    source_hash(0x0084DE,0xB)==UINT64_C(0xAE13AFE6E9FF1FC7) &&
    bytes(0x01931E,call,3) && bytes(0x01B446,call,3);
}
/* Original victory dialogue/audio are interpreted, including their real RTLs.
 * Inline 80E9 still consumes its own frame and dispatches original table DATA.
 * Post-dispatch waits can resume only with a separately witnessed B81A frame. */
static const InterpAotContinuation script_entries[]={
  {0x01B833,0x028282,0x01B837,0x01B819,{0x02828C,0x02828C},0,1,0,0,1,3,2,2},
  {0x01B841,0x0090DB,0x01B845,0x01B819,{0x0090F3,0x00910D,0x009129,0x009139},0,1,0,0,1,3,0,2},
  {0x01B8DD,0x0084E0,0x01B8E1,0x01B8A5,{0x0084E8,0x0084E8},0,1,0,0,1,3,0,2},
  {0x01B8DD,0x0084E0,0x01B8E1,0x01B8AD,{0x0084E8,0x0084E8},0,1,0,0,1,3,0,2},
  {0x01B8DD,0x0084E0,0x01B8E1,0x01B8B5,{0x0084E8,0x0084E8},0,1,0,0,1,3,0,2},
  {0x01B8DD,0x0084E0,0x01B8E1,0x01B8BD,{0x0084E8,0x0084E8},0,1,0,0,1,3,0,2},
  {0x01B8DD,0x0084E0,0x01B8E1,0x01B8C5,{0x0084E8,0x0084E8},0,1,0,0,1,3,0,2},
  {0x01B905,0x018703,0x01B909,0x01B819,{0x01876C,0x01876C},0,1,0,0,1,3,2,2}
};
static int script_source(void) {
  return source_hash(0x01B816,0x78)==UINT64_C(0x4D5DB436AE71B2EC) &&
    source_hash(0x028282,0x0B)==UINT64_C(0x9F5D6D4A4C793014) &&
    source_hash(0x0090DB,0x5F)==UINT64_C(0xE20AA48F28666155) &&
    source_hash(0x0080E9,0x20)==UINT64_C(0xD98520CA8A4BEFBC);
}
static int script_wait_source(void) {
  return script_source() &&
    source_hash(0x01B88E,0x54)==UINT64_C(0x0C689F3AEFD6E75C) &&
    source_hash(0x0084E0,9)==UINT64_C(0x7866A8232F0855B1);
}
static int script_descriptor(const InterpAotContinuation *d) {
  if(!d)return 0;
  for(unsigned i=0;i<sizeof(script_entries)/sizeof(script_entries[0]);++i) {
    if(memcmp(d,script_entries+i,sizeof(*d)))continue;
    if(i<2)return script_source();
    if(i<7)return script_wait_source();
    return script_source() &&
      source_hash(0x01B8E2,0x29)==UINT64_C(0x259AD0D55DFEE6FD) &&
      source_hash(0x018703,0x6F)==UINT64_C(0xF9632676A3CE04F2);
  }
  return 0;
}
static int recovery_source(const InterpAotContinuation *d) {
  if(!d)return 0;
  if(script_descriptor(d) || LagoonNativeRevivalSource(d) || LagoonNativeEndgameSource(d))return 1;
  /* The earlier source contracts authorize only two RTL sites. Never allow
   * an extension of the shared descriptor ABI to broaden those old guards. */
  if(d->return_sites[2] || d->return_sites[3])return 0;
  if(d->continuation_pc24==0x019368 || d->continuation_pc24==0x019371) {
    for(unsigned i=0;i<sizeof(flash_entries)/sizeof(flash_entries[0]);++i)
      if(!memcmp(d,flash_entries+i,sizeof(*d)))return flash_source();
    return 0;
  }
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
static struct {uint64_t generation;uint16_t sp;unsigned phase;
  uint32_t owner;unsigned char stack[7];} script_parent;
static struct {unsigned long long dialogue,audio,reload,waits[5],ancestors[5],refusals;} script_counts;
static int script_owner_index(uint32_t pc) {
  for(unsigned i=2;i<7;++i)if(script_entries[i].owner_return_pc24==pc)return (int)i-2;
  return -1;
}
static int script_mode(const CpuState *cpu) {
  return cpu && cpu->ram && cpu->DB==1 && !cpu->D && cpu->m_flag==1 &&
    !cpu->x_flag && !cpu->emulation;
}
static int script_stack_live(const CpuState *cpu) {
  return script_parent.phase && script_mode(cpu) && cpu->PB==1 &&
    script_parent.generation==RtlStateGeneration() && script_parent.sp<=0x1ff8 &&
    script_wait_source() && !memcmp(script_parent.stack,cpu->ram+script_parent.sp+1,7);
}
static int script_hook(unsigned event,const CpuState *cpu,uint32_t source,uint32_t target,
    const InterpAotContinuation *d,uint16_t owner_entry_s,InterpAotAncestor *proposal) {
  if(event==INTERP_CONT_DISCARD){memset(&script_parent,0,sizeof(script_parent));return 0;}
  if(event==INTERP_CONT_CALL && source==0x01B8DD) {
    /* A new matching call retires even an unconsumed or refused older token. */
    memset(&script_parent,0,sizeof(script_parent));
    if(!script_mode(cpu) || cpu->PB!=0 || target!=0x0084E0 || cpu->S>0x1ff8 || !script_wait_source())return 0;
    const unsigned char *p=cpu->ram+cpu->S+1;
    uint32_t owner=(uint16_t)(p[3]|p[4]<<8)+1u;
    owner|=0x010000;
    if(p[0]!=0xE0 || p[1]!=0xB8 || p[2]!=1 || p[5]!=0x18 || p[6]!=0xB8 ||
        script_owner_index(owner)<0)return 0;
    script_parent.generation=RtlStateGeneration();script_parent.sp=cpu->S;
    script_parent.owner=owner;script_parent.phase=1;
    memcpy(script_parent.stack,p,7);return 0;
  }
  if(event==INTERP_CONT_TAKE && d && script_descriptor(d)) {
    if(target==0x01B837)script_counts.dialogue++;
    else if(target==0x01B845)script_counts.audio++;
    else if(target==0x01B909)script_counts.reload++;
    else if(target==0x01B8E1) {
      int i=script_owner_index(d->owner_return_pc24);
      if(script_parent.phase!=1 || source!=0x0084E8 || !script_stack_live(cpu) ||
          cpu->S!=script_parent.sp+3 || owner_entry_s!=cpu->S ||
          d->owner_return_pc24!=script_parent.owner) {
        script_parent.phase=0;script_counts.refusals++;return 0;
      }
      script_parent.phase=2;script_counts.waits[i]++;
    }
    return 0;
  }
  if(event!=INTERP_CONT_RETURN || !script_parent.phase || !d)return 0;
  if(d->owner_return_pc24==0x01B819){script_parent.phase=0;return 0;}
  if(d->continuation_pc24!=0x01B8E1)return 0;
  int i=script_owner_index(d->owner_return_pc24);
  int ok=script_parent.phase==2 && i>=0 && proposal && script_descriptor(d) &&
    script_stack_live(cpu) && cpu->S==script_parent.sp+5 &&
    owner_entry_s==script_parent.sp+3 && target==script_parent.owner &&
    d->owner_return_pc24==script_parent.owner;
  script_parent.phase=0;
  if(!ok){script_counts.refusals++;return 0;}
  proposal->pc24=target;proposal->owner_return_pc24=0x01B819;
  proposal->entry_s=cpu->S;proposal->owner_frame=2;
  script_counts.ancestors[i]++;return 1;
}
static struct {unsigned long long takes[2],hp[2],mp[2],refused[2];unsigned long long flash_render[2],flash_wait[2];
  unsigned registered,registration_attempts;unsigned long long failed_source_mask;} counts;
static const char *report_path;
static unsigned owner_index(uint32_t pc){return pc==0x01C6AF;}
static void report(void) {
  if(!report_path || !*report_path)return;
  FILE *f=fopen(report_path,"wb");if(!f)return;
  fprintf(f,"{\"registered\":%u,\"registration_attempts\":%u,\"failed_source_mask\":%llu,\"main\":{\"takes\":%llu,\"hp_to_mp\":%llu,\"mp_to_plx\":%llu,\"refusals\":%llu},"
    "\"render\":{\"takes\":%llu,\"hp_to_mp\":%llu,\"mp_to_plx\":%llu,\"refusals\":%llu},"
    "\"death_flash\":{\"9321\":{\"render_takes\":%llu,\"wait_takes\":%llu},"
    "\"B449\":{\"render_takes\":%llu,\"wait_takes\":%llu}},"
    "\"boss_script\":{\"dialogue_takes\":%llu,\"audio_takes\":%llu,\"reload_takes\":%llu,"
    "\"wait_takes\":[%llu,%llu,%llu,%llu,%llu],\"ancestor_returns\":[%llu,%llu,%llu,%llu,%llu],\"refusals\":%llu},"
    "\"revival\":{\"wait_takes\":%llu,\"ancestor_returns\":%llu,\"refusals\":%llu},"
    "\"camera_tail\":{\"calls\":%llu,\"armed\":%llu,\"proposals\":%llu,\"takes\":%llu,\"refusals\":%llu},",
    counts.registered,counts.registration_attempts,counts.failed_source_mask,counts.takes[0],counts.hp[0],counts.mp[0],counts.refused[0],
    counts.takes[1],counts.hp[1],counts.mp[1],counts.refused[1],
    counts.flash_render[0],counts.flash_wait[0],counts.flash_render[1],counts.flash_wait[1],script_counts.dialogue,script_counts.audio,script_counts.reload,
    script_counts.waits[0],script_counts.waits[1],script_counts.waits[2],script_counts.waits[3],script_counts.waits[4],
    script_counts.ancestors[0],script_counts.ancestors[1],script_counts.ancestors[2],script_counts.ancestors[3],script_counts.ancestors[4],script_counts.refusals,
    LagoonNativeRevivalGetStats()->wait_takes,LagoonNativeRevivalGetStats()->ancestor_returns,LagoonNativeRevivalGetStats()->refusals,
    LagoonNativeCameraGetStats()->calls,LagoonNativeCameraGetStats()->armed,LagoonNativeCameraGetStats()->proposals,
    LagoonNativeCameraGetStats()->takes,LagoonNativeCameraGetStats()->refusals);
  const LagoonNativeEndgameStats *e=LagoonNativeEndgameGetStats();
  fprintf(f,"\"endgame_inline\":{\"calls\":[%llu,%llu,%llu],\"armed\":[%llu,%llu,%llu],"
    "\"proposals\":[%llu,%llu,%llu],\"takes\":[%llu,%llu,%llu],\"refusals\":%llu,\"thor_wait_takes\":[",
    e->calls[0],e->calls[1],e->calls[2],e->armed[0],e->armed[1],e->armed[2],
    e->proposals[0],e->proposals[1],e->proposals[2],e->takes[0],e->takes[1],e->takes[2],e->refusals);
  for(unsigned i=0;i<13;i++)fprintf(f,"%s%llu",i?",":"",e->waits[i]);
  fprintf(f,"],\"queue_ancestors\":%llu}}\n",e->queue_ancestors);fclose(f);
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
  LagoonNativeCameraObserve(event,cpu,source,target,d,owner_entry_s);
  if(LagoonNativeEndgameHook(event,cpu,source,target,d,owner_entry_s,proposal))return 1;
  if(LagoonNativeRevivalHook(event,cpu,source,target,d,owner_entry_s,proposal))return 1;
  if(script_hook(event,cpu,source,target,d,owner_entry_s,proposal))return 1;
  if(event==INTERP_CONT_DISCARD){memset(&parent,0,sizeof(parent));return 0;}
  if(event==INTERP_CONT_TAKE && d && (target==0x019368 || target==0x019371)) {
    unsigned owner=d->owner_return_pc24==0x01B449;
    if(target==0x019368)counts.flash_render[owner]++;else counts.flash_wait[owner]++;
    return 0;
  }
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
  LagoonNativeCameraBeforeFrame();
  LagoonNativeEndgameBeforeFrame();
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
    InterpAotContinuation all[64];memcpy(all,entries,sizeof(entries));
    memcpy(all+5,boss_entries,sizeof(boss_entries));
    memcpy(all+9,flash_entries,sizeof(flash_entries));
    unsigned total=13;
    memcpy(all+total,script_entries,sizeof(script_entries));total+=sizeof(script_entries)/sizeof(script_entries[0]);
    total+=LagoonNativeRevivalDescriptors(all+total,64-total);
    total+=LagoonNativeEndgameDescriptors(all+total,64-total);
    if(report_path && *report_path){counts.registration_attempts++;counts.failed_source_mask=0;
      for(unsigned i=0;i<total;i++)if(!recovery_source(all+i))counts.failed_source_mask|=1ULL<<i;}
    configured=interp_bridge_set_aot_continuations(all,total,recovery_source);
    if(configured){interp_bridge_set_aot_continuation_hook(recovery_hook);counts.registered=total;}
  }
}
