/* Compile this test against the ACTUAL production module included below.
 * External original ROM input only; no assets are embedded in the test. */
#include <assert.h>
#include "../src/lagoon_campaign_debug.c"
uint8 g_ram[0x20000];
const uint8 *g_rom;
static uint32_t redirected;
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
  free(rom);puts("campaign production guard/descriptor tests passed");return 0;
}
