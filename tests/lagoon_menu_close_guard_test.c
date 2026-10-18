/* Metadata-only production hook proof. Optional private captures supplement the
 * synthetic stack/context built from observed entry-frame metadata. No ROM,
 * graphics, RAM snapshot or save data is embedded in this source. */
#include "lagoon_menu_close.h"
#include "snes/interp_bridge.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
uint8 g_ram[0x20000]; const uint8 *g_rom;
static uint8 rom[0x100000],saved_ram[0x20000];
static InterpPreOpcodeHook hooks[5];static uint32 sites[5],redirect;static unsigned registrations,checks,failures,positives,refusals;
bool interp_bridge_add_pre_opcode_hook(uint32_t pc,InterpPreOpcodeHook h){if(registrations>=5)return false;sites[registrations]=pc;hooks[registrations++]=h;return true;}
void interp_bridge_pre_opcode_redirect(uint32_t pc){redirect=pc;}
#define CHECK(c,msg) do{checks++;if(!(c)){failures++;fprintf(stderr,"FAIL %s\n",msg);}}while(0)
static void put(unsigned q,unsigned v,unsigned n){while(n--){g_ram[q++]=(uint8)v;v>>=8;}}
static CpuState synthetic(int kind,unsigned owner,unsigned variant){
 for(unsigned i=0;i<sizeof(g_ram);i++)g_ram[i]=(uint8)(i*109+73);
 CpuState c={0};c.A=8;c.X=0x840;c.Y=8;c.S=0x1e1;c.DB=kind?1:2;c.PB=kind?0:2;c.P=0x24;c.ram=g_ram;cpu_p_to_mirrors(&c);
 g_ram[0x23d]=0x80;g_ram[0x275]=0x81;g_ram[0x100e]=0;
 /* Exact frame layouts independently observed at C5C6 and 89EC. */
 static const uint8 map[]={0xe3,0x80,0,1,0xe5,0x87,1,0x32,0x87,0xe3,0x80,0,2,0x3a,0xb7,2};
 static const uint8 queue[]={0x80,0x8a,1,0x31,0x8a,0,0,0x40,0x87,0xe3,0x80,0,2,0x3a,0xb7,2};
 memcpy(g_ram+c.S+1,kind?queue:map,16);put(c.S+14,owner,3);
 if(!kind){g_ram[0x52]=3;put(0x53,0x40,2);put(0x55,0x60,2);put(0x63,0x07c46c,3);put(0x66,0x068cd0,3);put(0x6f,0x07c5ec,3);put(0x72,0x068cd0,3);}
 else{unsigned source=variant&1?0x2e0:0x2d0;put(8,source,3);g_ram[source]=g_ram[source+1]=1;put(source+2,0x1234,2);put(0x238,variant&2?0x77c:0,2);g_ram[0x215]=0;g_ram[0x216]=0x70;g_ram[0x299]=0;g_ram[0x2a3]=0;if(variant&1)put(c.S+1,0x018a90,3);}
 return c;
}
static void refuse(int kind,CpuState*c,uint32 pc,const char*label){CpuState before=*c;memcpy(saved_ram,g_ram,sizeof(g_ram));redirect=0xdeadbeef;hooks[kind](c,pc);refusals++;CHECK(!memcmp(&before,c,sizeof(*c)),label);CHECK(!memcmp(saved_ram,g_ram,sizeof(g_ram)),label);CHECK(redirect==0xdeadbeef,label);}
static void positive(int kind,CpuState*c,uint32 pc,unsigned expected){redirect=0;hooks[kind](c,pc);positives++;CHECK(redirect==expected,"positive registered callback redirect");}
int main(int argc,char**argv){
 assert(argc==2||argc==3);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);g_rom=rom;
 LagoonMenuClose_BeforeFrame();CHECK(registrations==5&&sites[0]==0x02c5c6&&sites[1]==0x0089ec&&sites[2]==0x018a10&&sites[3]==0x018a32&&sites[4]==0x018ad1,"exact production registrations");LagoonMenuClose_BeforeFrame();CHECK(registrations==5,"idempotent production registration");
 const unsigned owners[]={0x02b73a,0x02b87d,0x02bc9b};
 for(unsigned owner=0;owner<3;owner++)for(int kind=0;kind<2;kind++){
  uint32 pc=sites[kind];CpuState c;
  for(unsigned v=0;v<(kind?4:1);v++){c=synthetic(kind,owners[owner],v);positive(kind,&c,pc,kind?(v&2?0x008a97:0x008a96):0x02c64d);if(kind&&(v&2))CHECK(c.S==0x1dd,"fullqueue retains original saved X/Y frame");}
  for(int mode=0;mode<14;mode++){
   c=synthetic(kind,owners[owner],0);
   switch(mode){case 0:c.m_flag=0;break;case 1:c.x_flag=1;break;case 2:c.emulation=1;break;case 3:c.D=1;break;case 4:c._flag_D=1;break;case 5:c.P|=8;break;case 6:c.DB^=1;break;case 7:c.PB^=1;break;case 8:c.S=0xff;break;case 9:c.S=0x1ea;break;case 10:g_ram[0x23d]&=127;break;case 11:g_ram[0x275]=0;break;case 12:g_ram[0x100e]=1;break;case 13:g_rom=NULL;break;}
   refuse(kind,&c,pc,"mode/source/scene refusal");g_rom=rom;
  }
  for(unsigned offset=1;offset<=16;offset++){
   if(kind&&(offset==6||offset==7))continue;
   c=synthetic(kind,owners[owner],0);g_ram[c.S+offset]^=0x40;refuse(kind,&c,pc,"real return/frame byte refusal");
  }
  c=synthetic(kind,owners[owner],0);refuse(kind,&c,pc+1,"foreign PC readonly refusal");
  c=synthetic(kind,owners[owner],0);unsigned off=kind?0x9ec:0x145c6;rom[off]^=1;refuse(kind,&c,pc,"original instruction hash refusal");rom[off]^=1;
  c=synthetic(kind,owners[owner],0);if(kind)put(8,0x2c0,3);else put(0x63,0x004000,3);refuse(kind,&c,pc,"unsupported data source readonly refusal");
  c=synthetic(kind,owners[owner],0);if(kind)g_ram[0x2d0]=2;else put(0x53,3,2);refuse(kind,&c,pc,"unsupported dimensions readonly refusal");
  if(kind){c=synthetic(kind,owners[owner],0);put(0x238,0x780,2);refuse(kind,&c,pc,"invalid queue cursor readonly refusal");c=synthetic(kind,owners[owner],0);c.A|=0x100;refuse(kind,&c,pc,"unsupported high accumulator readonly refusal");}
 }

 /* New parent prefix: exact submenu owner and genuine outer JSR/dispatcher. */
 for(unsigned owner=0;owner<3;owner++){
  CpuState c=synthetic(1,owners[owner],0);c.PB=c.DB=1;c.X=12;c.Y=7;
  put(c.S+1,0x8740,2);put(c.S+3,0x0080e3,3);g_ram[c.S+6]=2;put(c.S+7,owners[owner],3);
  put(0xb8,96,2);put(0xba,56,2);put(0x88,0x7f0000,3);put(0x8b,0x7f4000,3);put(0x69,0x068cd0,3);put(0x75,0x068cd0,3);
  CpuState good=c;memcpy(saved_ram,g_ram,sizeof(g_ram));positive(2,&c,0x018a10,0x018a2f);CHECK(c.S==good.S-2,"parent retains PHX frame for original queue call");
  uint8 snapshot[0x20000];memcpy(g_ram,saved_ram,sizeof(g_ram));memcpy(snapshot,g_ram,sizeof(g_ram));
  for(unsigned bad=0;bad<23;bad++){
   c=good;memcpy(g_ram,snapshot,sizeof(g_ram));uint32 pc=0x018a10;
   if(bad<9)g_ram[c.S+bad+1]^=64;
   else switch(bad){case 9:c.PB=0;break;case 10:c.DB=0;break;case 11:c.X=34;break;case 12:c.Y=32;break;case 13:c.P|=8;break;case 14:c._flag_D=1;break;case 15:c.m_flag=0;break;case 16:c.x_flag=1;break;case 17:c.D=1;break;case 18:pc++;break;case 19:put(0x69,0x004000,3);break;case 20:g_ram[0x23d]=0;break;case 21:g_ram[0x275]=0;break;case 22:g_ram[0x100e]=1;break;}
   refuse(2,&c,pc,"parent prefix transactional refusal");
  }
  for(unsigned code=0;code<3;code++){
   const unsigned addr[]={0x8a10,0xaa4,0x8311};c=good;memcpy(g_ram,snapshot,sizeof(g_ram));rom[addr[code]]^=1;refuse(2,&c,0x018a10,"parent source identity refusal");rom[addr[code]]^=1;
  }
 }


 for(unsigned owner=0;owner<3;owner++)for(unsigned branch=0;branch<3;branch++){
  CpuState c=synthetic(1,owners[owner],0);c.PB=c.DB=1;c.X=0x77c;c.Y=branch==2?31:7;
  put(c.S+1,branch?33:12,2);put(c.S+3,0x8740,2);put(c.S+5,0x0080e3,3);g_ram[c.S+8]=2;put(c.S+9,owners[owner],3);
  CpuState good=c;uint8 snapshot[0x20000];memcpy(snapshot,g_ram,sizeof(g_ram));positive(3,&c,0x018a32,branch==2?0x018a5d:0x018a10);CHECK(c.S==good.S+2,"suffix consumes original PHX only");
  for(unsigned bad=0;bad<25;bad++){
   c=good;memcpy(g_ram,snapshot,sizeof(g_ram));uint32 pc=0x018a32;
   if(bad<9)g_ram[c.S+bad+3]^=64;
   else switch(bad){case 9:c.PB=0;break;case 10:c.DB=0;break;case 11:put(c.S+1,34,2);break;case 12:c.Y=32;break;case 13:c.P|=8;break;case 14:c._flag_D=1;break;case 15:c.m_flag=0;break;case 16:c.x_flag=1;break;case 17:c.D=1;break;case 18:pc++;break;case 19:g_ram[0x23d]=0;break;case 20:g_ram[0x275]=0;break;case 21:g_ram[0x100e]=1;break;case 22:c.emulation=1;break;case 23:c.S=0xff;break;case 24:c.S=0x1ea;break;}
   refuse(3,&c,pc,"parent suffix readonly refusal");
  }
  for(unsigned code=0;code<2;code++){
   const unsigned addr[]={0x8a32,0x89fd};c=good;memcpy(g_ram,snapshot,sizeof(g_ram));rom[addr[code]]^=1;refuse(3,&c,0x018a32,"suffix code identity refusal");rom[addr[code]]^=1;
  }
 }



 /* Exact original HUD RAM fill inside the owned submenu-close parent. */
 for(unsigned owner=0;owner<3;owner++){
  CpuState c=synthetic(1,owners[owner],0);c.PB=c.DB=1;
  put(c.S+1,0x8743,2);put(c.S+3,0x0080e3,3);g_ram[c.S+6]=2;put(c.S+7,owners[owner],3);
  CpuState good=c;uint8 snapshot[0x20000];memcpy(snapshot,g_ram,sizeof(g_ram));positive(4,&c,0x018ad1,0x018af2);CHECK(c.S==good.S,"HUD preserves original stack seam");
  for(unsigned offset=1;offset<=9;offset++){
   c=good;memcpy(g_ram,snapshot,sizeof(g_ram));g_ram[c.S+offset]^=64;refuse(4,&c,0x018ad1,"HUD caller/frame readonly refusal");
  }
  for(unsigned bad=0;bad<13;bad++){
   c=good;memcpy(g_ram,snapshot,sizeof(g_ram));uint32 pc=0x018ad1;
   switch(bad){case 0:c.PB^=1;break;case 1:c.DB=2;break;case 2:c.m_flag=0;break;case 3:c.x_flag=1;break;case 4:c.emulation=1;break;case 5:c.D=1;break;case 6:c.P|=8;break;case 7:c._flag_D=1;break;case 8:c.S=0xff;break;case 9:pc++;break;case 10:g_ram[0x23d]=0;break;case 11:g_ram[0x275]=0;break;case 12:g_ram[0x100e]=1;break;}
   refuse(4,&c,pc,"HUD mode/scene/foreignPC readonly refusal");
  }
  for(unsigned byte=0;byte<0x29;byte++){
   c=good;memcpy(g_ram,snapshot,sizeof(g_ram));rom[0x8acd+byte]^=1;refuse(4,&c,0x018ad1,"HUD complete source readonly refusal");rom[0x8acd+byte]^=1;
  }
 }
 if(argc==3){
  for(unsigned n=0;n<5;n++){
   char path[1024];const char*tag=n?"0089ec":"02c5c6";unsigned index=n?n-1:0;
   snprintf(path,sizeof(path),"%s/capture-%s-%u.cpu",argv[2],tag,index);f=fopen(path,"rb");assert(f);CpuState c;assert(fread(&c,1,sizeof(c),f)==sizeof(c));fclose(f);
   snprintf(path,sizeof(path),"%s/capture-%s-%u.ram",argv[2],tag,index);f=fopen(path,"rb");assert(f&&fread(g_ram,1,sizeof(g_ram),f)==sizeof(g_ram));fclose(f);c.ram=g_ram;
   redirect=0;hooks[!!n](&c,sites[!!n]);CHECK(redirect==(!n?0x02c64d:0x008a96),"actual captured entry accepted");positives++;
  }
 }
 printf("production guard: %u/%u checks, %u positives, %u readonly refusals\n",checks-failures,checks,positives,refusals);return failures?1:0;
}
