/* The approved centered-camera patch retains the REAL JSR80CA owner, but
 * reaches actor animation through JMP83F3 rather than another guest call.
 * Lend that witnessed frame to the original native dispatcher without making
 * a frame, skipping camera work, changing guest state or guessing exit modes. */
#include "lagoon_native_camera.h"
#include "common_rtl.h"
#include <string.h>

static struct {
  uint64_t generation;
  const CpuState *cpu;
  const uint8_t *ram;
  uint16_t sp;
  uint8_t stack[2],phase;
} camera;
static LagoonNativeCameraStats stats;

static uint64_t camera_hash(unsigned pc,unsigned count) {
  unsigned at=((pc>>16)&127)*0x8000+(pc&0x7fff);
  uint64_t h=UINT64_C(1469598103934665603);
  if(!g_rom || at+count>0x100000)return 0;
  for(unsigned i=0;i<count;i++)h=(h^g_rom[at+i])*UINT64_C(1099511628211);
  return h;
}
static int camera_source(void) {
  /* Full patched main caller; complete camera/strip helpers and their tables,
   * including all35 dispatcher bytes and four centered threshold operands;
   * original paired actor selector/table. A modified helper cannot borrow
   * the balanced source contract of the approved camera patch. */
  return camera_hash(0x01807A,0x53)==UINT64_C(0xAC6A40F151257DAF) &&
    camera_hash(0x018218,0x4EB)==UINT64_C(0xE86A6CB1D0E5F630) &&
    camera_hash(0x01A7B5,0x21)==UINT64_C(0x56AF10E34AEB3CCF);
}
static int camera_mode(const CpuState *cpu) {
  return cpu && cpu->ram && cpu->PB==1 && cpu->DB==1 && !cpu->D &&
    cpu->m_flag==1 && !cpu->x_flag && !cpu->emulation && !cpu->_flag_D &&
    (cpu->P&(CPU_P_M|CPU_P_X|CPU_P_D))==CPU_P_M && cpu->S<=0x1FFD;
}
static int camera_live(const CpuState *cpu) {
  return camera.phase && camera_mode(cpu) && camera.cpu==cpu &&
    camera.ram==cpu->ram && camera.generation==RtlStateGeneration() &&
    camera.sp==cpu->S && camera_source() &&
    !memcmp(camera.stack,cpu->ram+cpu->S+1,2);
}
static int camera_tail(const CpuState *cpu,uint32_t source,uint32_t target,
    InterpAotAncestor *proposal) {
  if(source!=0x0183F3)return 0;
  int valid=target==0x01A7B5 && proposal && camera.phase==1 && camera_live(cpu);
  /* Consume even a refused/duplicate proposal. Only a new REAL JSR can arm. */
  camera.phase=0;
  if(!valid){stats.refusals++;return 0;}
  proposal->pc24=target;proposal->owner_return_pc24=0x0180CD;
  proposal->entry_s=cpu->S;proposal->owner_frame=2;
  camera.phase=2;stats.proposals++;return 1;
}
void LagoonNativeCameraObserve(unsigned event,const CpuState *cpu,uint32_t source,
    uint32_t target,const InterpAotContinuation *entry,uint16_t owner_entry_s) {
  if(event==INTERP_CONT_DISCARD){memset(&camera,0,sizeof(camera));return;}
  if(event==INTERP_CONT_CALL && source==0x0180CA) {
    memset(&camera,0,sizeof(camera));
    if(target!=0x0183D8)return;
    stats.calls++;
    if(!camera_mode(cpu) || !camera_source() || cpu->ram[cpu->S+1]!=0xCC ||
        cpu->ram[cpu->S+2]!=0x80){stats.refusals++;return;}
    camera.cpu=cpu;camera.ram=cpu->ram;camera.sp=cpu->S;
    camera.generation=RtlStateGeneration();camera.phase=1;
    memcpy(camera.stack,cpu->ram+cpu->S+1,2);stats.armed++;return;
  }
  if(event==INTERP_CONT_TAKE && !entry && source==0x0183F3 && target==0x01A7B5) {
    if(camera.phase==2 && owner_entry_s==camera.sp && camera_live(cpu))stats.takes++;
    else stats.refusals++;
    camera.phase=0;
  }
}
const LagoonNativeCameraStats *LagoonNativeCameraGetStats(void){return &stats;}
void LagoonNativeCameraBeforeFrame(void) {
  if(g_rom)(void)interp_bridge_set_aot_tail_hook(0x0183F3,camera_tail);
}
