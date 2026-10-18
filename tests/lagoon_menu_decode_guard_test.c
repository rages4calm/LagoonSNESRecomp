#include "lagoon_menu_decode.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
uint8_t g_ram[0x20000];static uint8_t rom[0x100000],expected[0x20000],before[0x20000];const uint8_t*g_rom=rom;
uint8_t cpu_read8(CpuState*c,uint8_t bank,uint16_t addr){assert(!bank);c->open_bus=g_ram[addr];return c->open_bus;}
static void put(unsigned q,unsigned v,unsigned n){while(n--){g_ram[q++]=v;v>>=8;}}
static const struct{unsigned source,bg;} world_sources[]={{0x128000,1},{0x128B77,1},{0x129E89,1},{0x12B297,1},{0x12DA6C,1},{0x12E794,1},{0x12EE12,1},{0x138000,1},{0x139A7A,1},{0x13A421,1},{0x13B55D,1},{0x13C716,1},{0x13D745,1},{0x13F001,1},{0x148000,1},{0x149197,1},{0x14A1BC,1},{0x14ABBE,1},{0x14C061,1},{0x14D33D,1},{0x14DE0A,1},{0x158000,1},{0x15918D,1},{0x15A43D,1},{0x15B8AC,1},{0x15C815,1},{0x15D7E5,1},{0x15E7FB,1},{0x15F529,1},{0x168000,1},{0x168ED1,1},{0x188000,0},{0x1891E4,0},{0x18A0B2,0},{0x18AFB0,0},{0x18C636,0},{0x18E0A7,0},{0x198000,0},{0x199F6C,0},{0x19AF50,0},{0x19C637,0},{0x19E0DD,0},{0x19FD1D,0},{0x1A8000,0},{0x1A906F,0},{0x1A94FE,0},{0x1AAED9,0},{0x1ABF2D,0},{0x1ADBB7,0},{0x1AED4A,0},{0x1B8000,0},{0x1B84D8,0},{0x1BA17D,0},{0x1BB982,0},{0x1BD63D,0},{0x1BE46B,0},{0x1C8000,0},{0x1C92F5,0},{0x1CADA9,0},{0x1CB607,0},{0x1CB7F8,0},{0x1CC387,0},{0x1CD41D,0},{0x1CEAC1,0},{0x1CFA11,0},{0x1D8000,0},{0x1D9B77,0},{0x1DB67E,0},{0x1DC859,0},{0x1DD5C7,0}};
static CpuState setup(unsigned kind,unsigned menu){
 memset(g_ram,0x59,sizeof(g_ram));CpuState c={0};c.S=0x1d0;c.X=42;c.Y=1234;c.A=0xA508;c.DB=1;c.P=0x24;c.m_flag=1;c._flag_I=1;c.host_return_valid=3;c.master_cycles=100;c.cycles=10;
 g_ram[0x23d]=0x80;g_ram[0x275]=0x81;g_ram[0x100e]=0;g_ram[0xe4]=0;
 if(kind<3){unsigned id=kind+6,q=0x14e6+id*7;memcpy(g_ram+0xe0,rom+q,3);memcpy(g_ram+0x28a,rom+q+3,4);put(c.S+1,0x94d6,3);put(c.S+6,id==6?0x2bfd5:id==7?0x2bf76:0x2bf98,3);unsigned p[]={0xb6cb,0xb7f5,0xbc56},p2[]={0xb6c2,0xb7ec,0xbc4d};put(c.S+9,id==6?p[menu]:p2[menu],2);}
 else if(kind<73){unsigned source=world_sources[kind-3].source,bg=world_sources[kind-3].bg;put(0xe0,source,3);put(bg?0x57:0x5d,source,3);put(c.S+1,bg?0x18874:0x18889,3);put(c.S+4,0x87d5,2);put(c.S+6,0x8732,2);put(c.S+8,0x80e3,3);g_ram[c.S+11]=2;unsigned parents[]={0x2b73a,0x2b87d,0x2bc9b};put(c.S+12,parents[menu],3);}
 if(kind>=73){unsigned id=kind==73?9:0,q=0x14e6+id*7;memcpy(g_ram+0xe0,rom+q,3);memcpy(g_ram+0x28a,rom+q+3,4);put(c.S+1,0x94d6,3);
 unsigned parents[]={0x2b73a,0x2b87d,0x2bc9b};
 if(id==9){put(c.S+6,0x97eb,3);put(c.S+9,0x18727,3);put(c.S+12,0x80e3,3);g_ram[c.S+15]=2;put(c.S+16,parents[menu],3);}
 else{put(c.S+6,0x18898,3);put(c.S+9,0x87d5,2);put(c.S+11,0x8732,2);put(c.S+13,0x80e3,3);g_ram[c.S+16]=2;put(c.S+17,parents[menu],3);}}
 return c;
}
static unsigned rpc(unsigned s){return ((g_ram[s+3]<<16)|(((g_ram[s+2]<<8)|g_ram[s+1])+1));}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);
 for(unsigned kind=0;kind<5+sizeof(world_sources)/sizeof(*world_sources);kind++)for(unsigned menu=0;menu<3;menu++){
  CpuState c=setup(kind,menu),trial=c;memcpy(expected,g_ram,sizeof(expected));unsigned used,n;assert(LagoonMenuDecode_Expand(rom,sizeof(rom),expected,&trial,&used,&n));RecompReturn result=(RecompReturn)92;
  assert(LagoonMenuDecode_TryNative(&c,c.S,3,rpc(c.S),&result));assert(result==RECOMP_RETURN_NORMAL);assert(!memcmp(g_ram,expected,sizeof(expected)));assert(c.A==trial.A&&c.X==trial.X&&c.Y==trial.Y&&c.P==trial.P&&c.S==trial.S+3&&c.master_cycles==148&&c.cycles==16);
  for(unsigned bad=0;bad<8;bad++){
   c=setup(kind,menu);unsigned host=rpc(c.S);uint16_t logical=c.S;uint8_t hrv=3;
   if(bad==0)logical--;if(bad==1)hrv=2;if(bad==2)host++;if(bad==3)c.host_return_valid=0;if(bad==4)c.D=1;if(bad==5)g_ram[c.S+(kind<3?9:kind<73?12:kind==73?16:17)]^=1;if(bad==6)g_ram[0xe0]^=1;if(bad==7)g_ram[0x23d]=15;
   CpuState prior=c;memcpy(before,g_ram,sizeof(before));result=(RecompReturn)92;assert(!LagoonMenuDecode_TryNative(&c,logical,hrv,host,&result));assert(!memcmp(before,g_ram,sizeof(before))&&!memcmp(&prior,&c,sizeof(c))&&result==(RecompReturn)92);
  }
 }
 puts("PASS225 guarded open/close contexts fullRAM+CPU/frame;1800 refusals preserveRAM/CPU/out");return 0;
}
