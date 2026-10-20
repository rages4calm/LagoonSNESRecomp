/* Compile this test against the ACTUAL production module included below.
 * External original ROM input only; no assets are embedded in the test. */
#include <assert.h>
#include "../src/lagoon_campaign_debug.c"
uint8 g_ram[0x20000];
const uint8 *g_rom;
static uint32_t redirected;
static uint64_t generation=1;
uint64_t RtlStateGeneration(void){return generation;}
static CpuState fixture_cpu;
static void fixture_setup(void) {
 memset(&fixture_cpu,0,sizeof fixture_cpu);memset(g_ram,0,sizeof g_ram);generation=1;
 fixture_cpu.ram=g_ram;fixture_cpu.PB=fixture_cpu.DB=1;fixture_cpu.S=0x1ffb;
 fixture_cpu.Y=4;fixture_cpu.A=0x2A;fixture_cpu.m_flag=1;fixture_cpu.P=CPU_P_M;
 g_ram[0x1ffc]=0xa2;g_ram[0x1ffd]=0x80;g_ram[0x40]=0x61;g_ram[0x41]=0xc5;g_ram[0x42]=5;
 g_ram[0x502]=176;g_ram[0x504]=0x10;g_ram[0x505]=4;
 s_fixture_pending=1;s_fixture_cpu=&fixture_cpu;s_fixture_sp=fixture_cpu.S;
 s_fixture_generation=1;s_fixture_tick=s_tick=10;s_fixture_descriptor=0x05c561;s_enabled=1;
}
int snes_runahead_frames(void) { return 0; }
int snes_netplay_active(void) { return 0; }
bool interp_bridge_add_pre_opcode_hook(uint32_t pc,InterpPreOpcodeHook hook) {
  return pc==0x01c8b5 && hook==at_door_check;
}
void interp_bridge_pre_opcode_redirect(uint32_t pc) { redirected=pc; }

int main(int argc,char **argv) {
  if (argc!=2) return 2;
  FILE *f=fopen(argv[1],"rb"); if (!f) return 2;
  fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);
  if (size<0x30000 || size>0x400000) return 2;
  uint8_t *rom=malloc((size_t)size);if (!rom) return 2;
  if (fread(rom,1,(size_t)size,f)!=(size_t)size) return 2;
  fclose(f);g_rom=rom;
  assert(valid_descriptor(0x05817d)); /* GoldCave→Samson */
  assert(valid_descriptor(0x058783)); /* IceCave→Thimale */
  assert(valid_descriptor(0x058fbd)); /* EvilSpirit→finalscriptroom */
  assert(valid_descriptor(0x05C561)); /* Authored NPCscript9 original room2A. */
  assert(!valid_descriptor(0x05C560));assert(!valid_descriptor(0x05C562));
  for(unsigned i=0;i<23;i++){
    unsigned off=offset(0x05C553)+i;rom[off]^=1;assert(!valid_descriptor(0x05C561));rom[off]^=1;
  }
  for(unsigned i=0;i<2;i++){
    unsigned off=offset(0x05C359)+i;rom[off]^=1;assert(!valid_descriptor(0x05C561));rom[off]^=1;
  }
  assert(!valid_descriptor(0x05817c));
  assert(!valid_descriptor(0x018000));
  assert(!valid_descriptor(0x7e0040));
  CpuState cpu={0};cpu.PB=1;cpu.DB=1;cpu.S=0x1ffb;cpu.Y=4;
  g_ram[0x1ffc]=0xa2;g_ram[0x1ffd]=0x80;g_ram[0x500]=0xc0;
  g_ram[0x40]=0xef;g_ram[0x41]=0x80;g_ram[0x42]=5;
  g_ram[0x102b]=19;g_ram[0x102d]=2; /* actual Atland north door */
  s_plan[0]=(CampaignWarp){10,0x05817d};s_count=1;s_enabled=1;s_tick=9;
  at_door_check(&cpu,0x01c8b5);assert(!redirected && s_next==0);
  s_tick=10;cpu.DB=0;
  at_door_check(&cpu,0x01c8b5);assert(!redirected && s_next==0);
  cpu.DB=1;cpu.x_flag=1;
  at_door_check(&cpu,0x01c8b5);assert(!redirected && s_next==0);
  cpu.x_flag=0;g_ram[0x100e]=0xff;
  at_door_check(&cpu,0x01c8b5);assert(!redirected && s_next==0);
  g_ram[0x100e]=0;g_ram[0x1ffc]=0xa1;
  at_door_check(&cpu,0x01c8b5);assert(!redirected && s_next==0);
  g_ram[0x1ffc]=0xa2;
  uint8_t before[sizeof(g_ram)];memcpy(before,g_ram,sizeof(g_ram));
  at_door_check(&cpu,0x01c8b5);
  assert(!redirected && s_next==1 && cpu.S==0x1ffb && cpu.Y==0);
  assert(g_ram[0x40]==0x7d && g_ram[0x41]==0x81 && g_ram[0x42]==5);
  memcpy(before+0x40,g_ram+0x40,3);
  assert(!memcmp(before,g_ram,sizeof(g_ram))); /* exactly pointer3bytes */
  at_door_check(&cpu,0x01c8b5);assert(!redirected && s_next==1);
  unsigned fixture_checks=0;
  fixture_setup();CpuState fixture_before=fixture_cpu;memcpy(before,g_ram,sizeof g_ram);
  at_fixture_room_store(&fixture_cpu,0x01C8CA);assert(fixture_cpu.A==0x3F&&!s_fixture_pending);
  fixture_before.A=0x3F;assert(!memcmp(&fixture_cpu,&fixture_before,sizeof fixture_cpu));assert(!memcmp(before,g_ram,sizeof g_ram));fixture_checks++;
  fixture_cpu.A=0x2A;at_fixture_room_store(&fixture_cpu,0x01C8CA);assert(fixture_cpu.A==0x2A);fixture_checks++;
  for(unsigned m=0;m<27;m++){
    fixture_setup();CpuState *use=&fixture_cpu;uint32_t pc=0x01C8CA;
    switch(m){
      case 0:fixture_cpu.PB=0;break;case 1:fixture_cpu.DB=0;break;case 2:fixture_cpu.D=1;break;
      case 3:fixture_cpu.emulation=1;break;case 4:fixture_cpu.m_flag=0;break;case 5:fixture_cpu.x_flag=1;break;
      case 6:fixture_cpu._flag_D=1;break;case 7:fixture_cpu.P|=CPU_P_D;break;case 8:fixture_cpu.P|=CPU_P_X;break;
      case 9:fixture_cpu.P=0;break;case 10:fixture_cpu.S++;break;case 11:g_ram[0x1ffc]^=1;break;
      case 12:fixture_cpu.Y=5;break;case 13:fixture_cpu.A=0x3F;break;case 14:fixture_cpu.A=0x012A;break;
      case 15:g_ram[0x100e]=1;break;case 16:s_fixture_descriptor++;break;case 17:g_ram[0x40]^=1;break;
      case 18:g_ram[0x42]=4;break;case 19:generation++;break;case 20:s_tick=9;break;case 21:s_tick=12;break;
      case 22:g_ram[0x502]++;break;case 23:g_ram[0x504]++;break;case 24:fixture_cpu.ram=NULL;break;
      case 25:pc++;break;case 26:use=NULL;break;
    }
    fixture_before=fixture_cpu;memcpy(before,g_ram,sizeof g_ram);at_fixture_room_store(use,pc);
    assert(!s_fixture_pending);assert(!memcmp(&fixture_cpu,&fixture_before,sizeof fixture_cpu)&&!memcmp(before,g_ram,sizeof g_ram));fixture_checks++;
  }
  for(unsigned i=0;i<112;i++){
    fixture_setup();unsigned off=offset(0x01C87B)+i;rom[off]^=1;
    fixture_before=fixture_cpu;at_fixture_room_store(&fixture_cpu,0x01C8CA);assert(!memcmp(&fixture_cpu,&fixture_before,sizeof fixture_cpu));rom[off]^=1;fixture_checks++;
  }
  for(unsigned i=0;i<23;i++){
    fixture_setup();unsigned off=offset(0x05C553)+i;rom[off]^=1;
    fixture_before=fixture_cpu;at_fixture_room_store(&fixture_cpu,0x01C8CA);assert(!memcmp(&fixture_cpu,&fixture_before,sizeof fixture_cpu));rom[off]^=1;fixture_checks++;
  }
  free(rom);printf("campaign production guard/descriptor tests passed; exact2Ascriptpayload/25sourcebyte mutants; %u synthetic3Fselector positive/one-shot/source/mode/CPU/stack/generation/position refusal checks; successfulfixturechangesonlyA\n",fixture_checks);return 0;
}
