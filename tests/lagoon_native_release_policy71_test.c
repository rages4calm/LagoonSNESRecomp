/* Policy/backstop fixture: no game ROM, generated game code or owner saves. */
#include "lagoon_native_backend69.h"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned reads,writes;
static const char *marker;
static Interp816 unchanged;
int interp816_opcode_hook(uint32_t pc){(void)pc;return 0;}
static uint8_t read_byte(void *context,uint32_t address){(void)context;(void)address;++reads;return 0xea;}
static void write_byte(void *context,uint32_t address,uint8_t value){(void)context;(void)address;(void)value;++writes;}
static void save_handler(void){FILE *f=fopen(marker,"w");assert(f);fputs("synthetic-save-handler",f);assert(!fclose(f));}
static void refused(const Interp816 *cpu,const char *reason,void *context){
  (void)reason;(void)context;
  Interp816RuntimeStats stats;interp816_runtime_stats_get(&stats);
  assert(!reads&&!writes&&!memcmp(cpu,&unchanged,sizeof(*cpu)));
  assert(stats.invocations[INTERP816_SCOPE_MAIN]==1&&stats.blocked[INTERP816_SCOPE_MAIN]==1);
  assert(!stats.fetches[INTERP816_SCOPE_MAIN]&&!stats.dispatches[INTERP816_SCOPE_MAIN]&&!stats.completed[INTERP816_SCOPE_MAIN]);
  puts("STRICT before_fetch=1 bus_reads=0 bus_writes=0 dispatches=0 guest_unchanged=1");fflush(stdout);
}
static const char *value(const char *name){const char *v=getenv(name);return v?v:"unset";}
static void flip_strict(void){
  const char *v=interp816_native_only_enabled()?"0":"1";
#ifdef _WIN32
  assert(!_putenv_s("SNESRECOMP_NATIVE_ONLY",v));
#else
  assert(!setenv("SNESRECOMP_NATIVE_ONLY",v,1));
#endif
}
int main(int argc,char **argv){
  assert(argc==3);marker=argv[2];assert(!atexit(save_handler));
  if(!strcmp(argv[1],"early"))(void)interp816_native_only_enabled();
  if(!strcmp(argv[1],"onrom"))LagoonNativeBackend69_OnRom(NULL,0);
  else LagoonNativeBackend69_ConfigureExecution();
  if(!strcmp(argv[1],"twice"))LagoonNativeBackend69_ConfigureExecution();
  if(!strcmp(argv[1],"flip")){flip_strict();LagoonNativeBackend69_ConfigureExecution();}
  printf("POLICY backend=%s strict=%s boot=%s latched=%d\n",value("LAGOON_NATIVE_BACKEND69"),
      value("SNESRECOMP_NATIVE_ONLY"),value("LAGOON_NATIVE_BOOT62"),interp816_native_only_enabled());fflush(stdout);
  if(!strcmp(argv[1],"step")){
    Interp816 *cpu=interp816_init(NULL,read_byte,write_byte);assert(cpu);
    cpu->pc=0x8000;cpu->a=0x1234;cpu->x=0x5678;cpu->y=0x9abc;cpu->sp=0x1ff;
    cpu->e=false;interp816_setFlags(cpu,0x24);interp816_set_runtime_scope(cpu,INTERP816_SCOPE_MAIN);
    cpu->native_only_failure=refused;unchanged=*cpu;
    assert(interp816_runOpcode(cpu)>0);
    Interp816RuntimeStats stats;interp816_runtime_stats_get(&stats);
    assert(reads==1&&!writes&&stats.fetches[INTERP816_SCOPE_MAIN]==1&&stats.dispatches[INTERP816_SCOPE_MAIN]==1&&
        stats.completed[INTERP816_SCOPE_MAIN]==1&&!stats.blocked[INTERP816_SCOPE_MAIN]);
    puts("REFERENCE real_nop=1 bus_reads=1 dispatches=1");interp816_free(cpu);
  }
  return 0;
}
