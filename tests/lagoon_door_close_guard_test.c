/* Reuse the original menu fixtures and independently qualified RAM bodies.
 * Only the newly admitted, real ordinary-door physical owner is different. */
#define main previous_close_guard_main
#include "lagoon_menu_close_guard_test.c"
#undef main
static CpuState door_close(unsigned kind) {
  CpuState c=synthetic(kind?1:0,0x02b73a,0);
  c.S=0x1d0;
  /* Copy the original immediate child frame, then the source door/main chain. */
  if(!kind){
    const uint8_t f[]={0xe3,0x80,0,1,0xe5,0x87,1,0x32,0x87};
    memcpy(g_ram+c.S+1,f,sizeof f);
  }else if(kind==1){
    const uint8_t f[]={0x80,0x8a,1,0x31,0x8a,0,0,0x40,0x87};
    memcpy(g_ram+c.S+1,f,sizeof f);
  }else{
    c.PB=c.DB=1;
    put(c.S+(kind==3?3:1),kind==4?0x8743:0x8740,2);
    if(kind==3)put(c.S+1,12,2);
    if(kind==2){
      c.X=12;c.Y=7;put(0xb8,96,2);put(0xba,56,2);
      put(0x88,0x7f0000,3);put(0x8b,0x7f4000,3);
      put(0x69,0x068cd0,3);put(0x75,0x068cd0,3);
    }
  }
  unsigned at=c.S+(kind<2?10:kind==3?5:3);
  put(at,0x018792,3);put(at+3,0xc8e4,2);put(at+5,0x80a2,2);
  g_ram[0x100e]=255;put(0x48e,2,2);g_ram[0x500]=0xc0;put(0x520,10,2);
  return c;
}
int main(int argc,char**argv) {
  assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);g_rom=rom;
  LagoonMenuClose_BeforeFrame();assert(registrations==5);
  const unsigned next[]={0x02c64d,0x008a96,0x018a2f,0x018a10,0x018af2};
  unsigned n=0;
  for(unsigned k=0;k<5;k++){
    CpuState c=door_close(k);positive(k,&c,sites[k],next[k]);
    for(unsigned bad=0;bad<25;bad++){
      c=door_close(k);unsigned at=c.S+(k<2?10:k==3?5:3);
      if(bad<7)g_ram[at+bad]^=64;
      else switch(bad){
        case 7:g_ram[0x100e]=1;break;case 8:g_ram[0x23d]=15;break;
        case 9:g_ram[0x275]=0;break;case 10:put(0x48e,75,2);break;
        case 11:g_ram[0x500]=0;break;case 12:g_ram[0x520]=0;break;
        case 13:g_ram[0x521]=1;break;case 14:c.m_flag=0;break;
        case 15:c.x_flag=1;break;case 16:c.emulation=1;break;
        case 17:c.D=1;break;case 18:c._flag_D=1;break;
        case 19:c.S=0xff;break;case 20:c.S=0x1ea;break;
        case 21:c.PB^=1;break;case 22:c.DB^=1;break;
        case 23:rom[0xc8e2]^=1;break;case 24:rom[0x80a0]^=1;break;
      }
      refuse(k,&c,sites[k],"new physical door owner readonly refusal");n++;
      if(bad==23)rom[0xc8e2]^=1;if(bad==24)rom[0x80a0]^=1;
    }
  }
  printf("PASS%u/5 new door callbacks; %u readonly owner/mode/source refusals; %u/%u checks\n",positives,n,checks-failures,checks);
  return failures?1:0;
}
