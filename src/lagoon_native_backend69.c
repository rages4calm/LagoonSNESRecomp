/* Opt-in architectural native executor. The generated source stays private;
 * a build without it must never pretend the requested backend is available. */
#include "lagoon_native_backend69.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int requested69;
static void backend_failure69(const char*reason){fprintf(stderr,"[native-blocks69] contract-refused reason=%s\n",reason);fflush(stderr);_Exit(87);}
#ifdef LAGOON_NATIVE_BLOCKS69_ENABLED
#include "common_rtl.h"
#include "native_blocks.h"
#include "lagoon_native_camera68.h"
#include "snes/interp_bridge.h"
#include "snes/interp816.h"
#include "snes/snes.h"
#include "state_dump.h"
extern Snes *g_snes;
extern const NativeBlockProgram lagoon_reset69_program;
#ifdef LAGOON_NATIVE_BLOCKS69_CAMERA_ENABLED
extern const NativeBlockProgram lagoon_camera69_program;
#endif
static const char *endpoint69;
static uint32_t endpoint_pc69=0x8059;
static FILE *trace69;
static void trace_opcode69(CpuState *c,uint32_t pc){
 if(!trace69||snes_frame_counter!=32)return;
 fprintf(trace69,"%d %06X %04X %04X %04X %04X %04X %02X %02X %02X %u %02X %llu %llu %llu %llu\n",snes_frame_counter,pc,c->A,c->X,c->Y,c->S,c->D,c->DB,c->PB,c->P,c->emulation,c->open_bus,(unsigned long long)c->cycles,(unsigned long long)c->master_cycles,(unsigned long long)c->coprocessor_master_cycles,(unsigned long long)interp_bridge_apu_pending_master_get());
}
typedef struct EndpointStream69 {SaveLoadInfo base;FILE *file;int error;} EndpointStream69;
static void endpoint_write69(SaveLoadInfo *base,void *bytes,size_t size){
 EndpointStream69 *s=(EndpointStream69*)base;
 if(fwrite(bytes,1,size,s->file)!=size)s->error=1;
}
/* Diagnostic stop before a selected opcode, never a game-route qualification.
 * This observer does not restore a snapshot, redirect the guest or touch saves. */
static void endpoint_capture69(CpuState *c,uint32_t pc){
 if(!endpoint69||pc!=endpoint_pc69)return;
 char dir[2048],path[2200];size_t n=strlen(endpoint69);
 if(n>=sizeof(dir))backend_failure69("endpoint-path-too-long");
 memcpy(dir,endpoint69,n+1);char *slash=strrchr(dir,'/'),*back=strrchr(dir,'\\');
 if(back&&(!slash||back>slash))slash=back;
 if(!slash)backend_failure69("endpoint-path-must-be-qualified");*slash=0;
 if(snes_state_dump(dir,"endpoint",NULL,0,0,0,(uint32_t)snes_frame_counter))backend_failure69("endpoint-state-dump");
 snprintf(path,sizeof(path),"%s/endpoint.hardware.bin",dir);
 FILE *hw=fopen(path,"wb");if(!hw)backend_failure69("endpoint-hardware-open");
 EndpointStream69 stream={{endpoint_write69,NULL},hw,0};
 RtlApuLock();bool saved_e=g_snes->cpu->e;
 snes_saveload(g_snes,&stream.base);g_snes->cpu->e=saved_e;RtlApuUnlock();
 if(fclose(hw)||stream.error)backend_failure69("endpoint-hardware-write");
 FILE *f=fopen(endpoint69,"w");if(!f)backend_failure69("endpoint-json-open");
 fprintf(f,"{\"schema\":1,\"pc\":%u,\"a\":%u,\"x\":%u,\"y\":%u,\"s\":%u,\"d\":%u,\"db\":%u,\"pb\":%u,\"p\":%u,\"m\":%u,\"index8\":%u,\"e\":%u,\"open_bus\":%u,\"host_return_valid\":%u,\"cycles\":%llu,\"master\":%llu,\"coprocessor_master\":%llu,\"beam_master\":%llu,\"apu_pending_master\":%llu,\"frame\":%d,\"hardware_version\":%u}\n",
 pc,c->A,c->X,c->Y,c->S,c->D,c->DB,c->PB,c->P,c->m_flag,c->x_flag,c->emulation,c->open_bus,c->host_return_valid,(unsigned long long)c->cycles,(unsigned long long)c->master_cycles,(unsigned long long)c->coprocessor_master_cycles,(unsigned long long)g_snes->beamMasterLast,(unsigned long long)interp_bridge_apu_pending_master_get(),snes_frame_counter,snes_saveload_get_version());
 if(fclose(f))backend_failure69("endpoint-json-write");
 Interp816RuntimeStats counts;InterpNativeOnlyBridgeStats bridge;
 interp816_runtime_stats_get(&counts);interp_bridge_native_only_stats_get(&bridge);
 uint64_t decoded=0,invocations=0;
 for(unsigned i=1;i<INTERP816_SCOPE_COUNT;i++){decoded+=counts.dispatches[i];invocations+=counts.invocations[i];}
 fprintf(stderr,"[native-blocks69] diagnostic-endpoint pc=%06X generated_native_opcodes=%llu interpreted_opcodes=%llu runtime_invocations=%llu\n",pc,(unsigned long long)bridge.generated_native_opcodes,(unsigned long long)decoded,(unsigned long long)invocations);
 fflush(stderr);_Exit(88);
}
static int registered69,bound69;
static int identity_reported69;
static const NativeBlockProgram *bound_program69;
static uint64_t generation69;
static const uint8_t*image69;
static size_t image_size69;
#endif
void LagoonNativeBackend69_OnRom(const uint8_t*rom,size_t size){
 const char*v=getenv("LAGOON_NATIVE_BACKEND69");requested69=v&&!strcmp(v,"1");
 if(v&&strcmp(v,"0")&&strcmp(v,"1"))backend_failure69("invalid-policy-value");
#ifdef LAGOON_NATIVE_BLOCKS69_ENABLED
 interp_bridge_clear_native_program();
 if(trace69){fclose(trace69);trace69=NULL;}
 const char *trace_path=getenv("LAGOON_NATIVE_TRACE69");
 if(trace_path){trace69=fopen(trace_path,"w");if(!trace69)backend_failure69("trace-open");}
 endpoint69=getenv("LAGOON_NATIVE_ENDPOINT69");
 const char *site=getenv("LAGOON_NATIVE_ENDPOINT69_PC");endpoint_pc69=0x8059;
 if(site){
  if(!endpoint69||strlen(site)!=6||strspn(site,"0123456789abcdefABCDEF")!=6)backend_failure69("invalid-endpoint-pc");
  endpoint_pc69=(uint32_t)strtoul(site,NULL,16);
 }
 registered69=bound69=identity_reported69=0;bound_program69=NULL;image69=NULL;image_size69=size;
 if(requested69){
  const NativeBlockProgram *programs[]={&lagoon_reset69_program,
#ifdef LAGOON_NATIVE_BLOCKS69_CAMERA_ENABLED
   &lagoon_camera69_program,
#endif
  };
  if(!interp_bridge_register_native_programs(programs,sizeof(programs)/sizeof(*programs),rom,size))backend_failure69("source-registration");
  registered69=1;fprintf(stderr,"[native-blocks69] catalog-attested programs=%zu source_sha256=%s\n",sizeof(programs)/sizeof(*programs),lagoon_reset69_program.source_sha256);
 }
#else
 (void)rom;(void)size;if(requested69)backend_failure69("generated-program-not-built");
#endif
}
void LagoonNativeBackend69_BeforeFrame(void){
#ifdef LAGOON_NATIVE_BLOCKS69_ENABLED
 if(trace69&&(!interp_bridge_add_pre_opcode_hook(0x8148,trace_opcode69)||!interp_bridge_add_pre_opcode_hook(0x814b,trace_opcode69)))backend_failure69("trace-wait-registration");
 if(trace69&&snes_frame_counter>32){if(fclose(trace69))backend_failure69("trace-write");trace69=NULL;}
 if(trace69&&snes_frame_counter==32){
  for(size_t i=0;i<lagoon_reset69_program.instruction_count;i++){
   uint32_t pc=lagoon_reset69_program.instructions[i].key.pc24;
   if(pc>=0x8100&&pc<0x8300&&!interp_bridge_add_pre_opcode_hook(pc,trace_opcode69))backend_failure69("trace-registration");
  }
 }
 if(endpoint69&&!interp_bridge_add_pre_opcode_hook(endpoint_pc69,endpoint_capture69))backend_failure69("endpoint-observer-registration");
 if(!requested69)return;
 uint64_t generation=RtlStateGeneration();
 if(!registered69||!g_rom)backend_failure69("runtime-image-unavailable");
 const NativeBlockProgram *program=&lagoon_reset69_program;
 int camera=LagoonCamera68_Source(g_rom,image_size69);
 if(camera==2){
#ifdef LAGOON_NATIVE_BLOCKS69_CAMERA_ENABLED
  program=&lagoon_camera69_program;
#else
  backend_failure69("camera-profile-not-built");
#endif
 }else if(camera!=1)backend_failure69("unknown-camera-source-profile");
 if(!identity_reported69){
  fprintf(stderr,"[native-blocks69] registered program=%s profile=%u entries=%zu source_sha256=%s\n",program->name,program->profile,program->instruction_count,program->source_sha256);identity_reported69=1;
 }
 if(!bound69||generation69!=generation||image69!=g_rom||bound_program69!=program){
  if(!interp_bridge_bind_native_program(program,g_rom,image_size69,generation))backend_failure69("runtime-image-binding");
  image69=g_rom;generation69=generation;bound69=1;bound_program69=program;
  fprintf(stderr,"[native-blocks69] bound generation=%llu profile=%u\n",(unsigned long long)generation,program->profile);
 }
#endif
}
