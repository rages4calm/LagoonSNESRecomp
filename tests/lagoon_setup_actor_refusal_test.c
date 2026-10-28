/* Link without any generated guest routine: setup mode must refuse before the
 * native prefix changes guest state or attempts its terminal generated RTL. */
#include "lagoon_native_actor_commands.h"
#include <stdio.h>
#include <string.h>

int main(void) {
  uint8_t ram[0x20000],before[sizeof ram];
  CpuState cpu={0},old;
  memset(ram,0xa5,sizeof ram);memcpy(before,ram,sizeof ram);
  cpu.ram=ram;cpu.A=0x11;cpu.S=0x180;cpu.host_return_valid=3;old=cpu;
  RecompReturn out=(RecompReturn)99;
  if(LagoonNativeActorCommands_Continue(NULL,0,&cpu,cpu.S,3,0x0181b0,&out,9,9,1)||
     memcmp(ram,before,sizeof ram)||memcmp(&cpu,&old,sizeof cpu)||out!=(RecompReturn)99) {
    fputs("FAIL: ROM-free setup mutated guest state or accepted continuation.\n",stderr);
    return 1;
  }
  puts("PASS: ROM-free setup refuses actor continuation without generated code or mutations.");
  return 0;
}
