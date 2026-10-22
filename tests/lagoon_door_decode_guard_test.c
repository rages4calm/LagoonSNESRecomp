/* Reuse the qualified decoder fixtures; exercise the NEW physical door owner. */
#define main previous_menu_guard_main
#include "lagoon_menu_decode_guard_test.c"
#undef main
static CpuState door(unsigned kind) {
  CpuState c=setup(kind,0);unsigned at=c.S+(kind<73?8:kind==73?12:13);
  put(at,0x018792,3);put(at+3,0xC8E4,2);put(at+5,0x80A2,2);
  g_ram[0x100E]=255;put(0x48E,2,2);g_ram[0x500]=0xC0;put(0x520,10,2);
  return c;
}
int main(int argc,char**argv) {
  assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  const unsigned kinds[]={3,10,46,73,74};unsigned refusals=0;
  for(unsigned j=0;j<sizeof kinds/sizeof*kinds;j++) {
    unsigned kind=kinds[j];CpuState c=door(kind),trial=c;
    memcpy(expected,g_ram,sizeof expected);unsigned used,n;
    assert(LagoonMenuDecode_Expand(rom,sizeof rom,expected,&trial,&used,&n));
    RecompReturn result=(RecompReturn)92;
    assert(LagoonMenuDecode_TryNative(&c,c.S,3,rpc(c.S),&result));
    assert(!memcmp(g_ram,expected,sizeof expected)&&c.S==trial.S+3&&c.A==trial.A&&c.X==trial.X&&c.Y==trial.Y&&c.P==trial.P);
    for(unsigned bad=0;bad<15;bad++) {
      c=door(kind);unsigned at=c.S+(kind<73?8:kind==73?12:13);
      if(bad<7)g_ram[at+bad]^=64;
      else switch(bad){case 7:g_ram[0x100E]=1;break;case 8:g_ram[0x23D]=15;break;case 9:g_ram[0x500]=0;break;case 10:g_ram[0x521]=1;break;case 11:g_ram[0x48E]=75;break;case 12:c.DB=9;break;case 13:rom[0x878F]^=1;break;case 14:rom[0xC8E2]^=1;break;}
      /* DB is handled by the existing decoder's semantics, not a new claim. */
      if(bad==12)continue;
      CpuState prior=c;memcpy(before,g_ram,sizeof before);result=(RecompReturn)92;
      assert(!LagoonMenuDecode_TryNative(&c,c.S,3,rpc(c.S),&result));
      assert(!memcmp(before,g_ram,sizeof before)&&!memcmp(&prior,&c,sizeof c)&&result==(RecompReturn)92);refusals++;
      if(bad==13)rom[0x878F]^=1;if(bad==14)rom[0xC8E2]^=1;
    }
  }
  printf("PASS5 original-stream door contexts fullRAM/CPU/frame; %u readonly door-owner refusals\n",refusals);
}
