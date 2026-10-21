/* Complete original0080B9 prefix+bank02 command versus host computation.
 * Native compute intentionally consumes no guest clocks; terminalRTL stays real.
 * Source comparison stops BEFORE that RTL with the exact original caller frame. */
#include "lagoon_native_actor_commands.h"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef assert
#define assert(v) do{if(!(v)){fprintf(stderr,"line%d %s\n",__LINE__,#v);exit(2);}}while(0)
static uint8_t rom[0x100000],ram[0x20000],expected[0x20000],before[0x20000],last_byte;
static unsigned cases,refusals,mutations;static uint64_t opcodes,cycles;
static uint8_t read8(void*ctx,uint32_t pc){(void)ctx;int32_t off=cpu_wram_offset(pc>>16,pc);if(off>=0)return last_byte=ram[off];assert((pc&65535)>=32768);return last_byte=rom[((pc>>16)&31)*32768+(pc&32767)];}
static void write8(void*ctx,uint32_t pc,uint8_t value){(void)ctx;int32_t off=cpu_wram_offset(pc>>16,pc);assert(off>=0);ram[off]=value;last_byte=value;}
static void put(unsigned a,unsigned v){ram[a]=v;ram[a+1]=v>>8;}
static CpuState prepare(unsigned selector,unsigned n,unsigned flags,unsigned intent,unsigned tier){
 memset(ram,0xa5,sizeof ram);CpuState c={0};c.PB=0;c.DB=1;c.A=selector;c.X=(n%32+1)*64;c.Y=(n&1)?0xfffe:0x1234;c.S=(n&2)?0x1ff0:0x180;c.ram=ram;c.P=0x24|(n&1)|(n&2?0xc0:0)|(n&4?2:0);cpu_p_to_mirrors(&c);
 c.cycles=41;c.master_cycles=12345;c.coprocessor_master_cycles=12300;c.open_bus=0x5a;
 put(c.S+1,selector==0x11?0x81af:0x81b9);ram[c.S+3]=1;put(c.S+4,0x80bd);
 ram[0x501+c.X]=flags;ram[0x53f+c.X]=intent;ram[0x50a+c.X]=tier;ram[0x50b+c.X]=intent;ram[0x51a+c.X]=tier;
 ram[0x518+c.X]=2;ram[0x500+c.X]=0xc8;ram[0x500]=0xc0;ram[0x520]=54;ram[0x521]=0;ram[0x406]=ram[0x100e]=0;ram[0x23d]=15;ram[0x275]=0x81;put(0x48e,n%75);
 return c;
}
static void compare(unsigned selector,unsigned flags,unsigned intent,unsigned tier,unsigned n){
 CpuState c=prepare(selector,n,flags,intent,tier),initial=c;memcpy(before,ram,sizeof ram);
 assert(LagoonNativeActorCommands_Apply(rom,sizeof rom,ram,&c,selector));memcpy(expected,ram,sizeof ram);
 assert(c.cycles==initial.cycles&&c.master_cycles==initial.master_cycles&&c.coprocessor_master_cycles==initial.coprocessor_master_cycles);
 memcpy(ram,before,sizeof ram);Interp816*i=interp816_init(NULL,read8,write8);assert(i);i->a=initial.A;i->x=initial.X;i->y=initial.Y;i->sp=initial.S;i->dp=initial.D;i->db=initial.DB;i->k=0;i->pc=0x80b9;i->e=0;interp816_setFlags(i,initial.P);
 unsigned ops=0,source_cycles=0;while(i->k!=0||i->pc!=0x80e5){assert(++ops<100);source_cycles+=interp816_runOpcode(i);}
 unsigned first=0;while(first<sizeof ram&&ram[first]==expected[first])first++;
 if(first!=sizeof ram||c.A!=i->a||c.X!=i->x||c.Y!=i->y||c.S!=i->sp||c.D!=i->dp||c.DB!=i->db||c.PB!=i->k||c.P!=interp816_getFlags(i)||c.open_bus!=last_byte){fprintf(stderr,"STATE sel%02x flags%02x intent%02x tier%02x n%u ram%05x A%04x/%04x X%04x/%04x Y%04x/%04x S%04x/%04x P%02x/%02x DB%02x/%02x bus%02x/%02x\n",selector,flags,intent,tier,n,first,c.A,i->a,c.X,i->x,c.Y,i->y,c.S,i->sp,c.P,interp816_getFlags(i),c.DB,i->db,c.open_bus,last_byte);assert(0);}
 /* The actual original RTL still consumes the untouched real3-byte frame. */
 source_cycles+=interp816_runOpcode(i);assert(i->k==1&&i->pc==(selector==0x11?0x81b0:0x81ba)&&i->sp==initial.S+3);
 opcodes+=ops+1;cycles+=source_cycles;cases++;interp816_free(i);
}
static void decline(CpuState*c,uint64_t gen,uint64_t frame,int normal){CpuState before_cpu=*c;memcpy(before,ram,sizeof ram);assert(!LagoonNativeActorCommands_Try(rom,sizeof rom,c,gen,frame,normal));assert(!memcmp(&before_cpu,c,sizeof*c)&&!memcmp(before,ram,sizeof ram));refusals++;}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonNativeActorCommands_Source(rom,sizeof rom));
 for(unsigned flags=0;flags<256;flags++)for(unsigned intent=0;intent<256;intent++)compare(0x11,flags,intent,(flags+intent)&255,flags^intent);
 for(unsigned tier=0;tier<256;tier++)for(unsigned direction=0;direction<256;direction++)compare(0x13,tier^direction,direction,tier,tier^direction);
 for(unsigned selector=0x11;selector<=0x13;selector+=2)for(unsigned n=0;n<32;n++){
  CpuState c=prepare(selector,n,0,1,2);assert(LagoonNativeActorCommands_Try(rom,sizeof rom,&c,9,9,1));
 }
 for(unsigned n=0;n<30;n++){
  CpuState c=prepare(0x11,0,0,1,2);
  switch(n){case 0:c.PB=1;break;case 1:c.DB=2;break;case 2:c.D=1;break;case 3:c.emulation=1;break;case 4:c.m_flag=0;break;case 5:c.x_flag=1;break;case 6:c._flag_D=1;break;case 7:c.P|=8;break;case 8:c.A=0x111;break;case 9:c.X=0;break;case 10:c.X=0x840;break;case 11:c.X=0x41;break;case 12:c.S=0x100;break;case 13:c.S=0x1ffe;break;case 14:ram[c.S+1]^=1;break;case 15:ram[c.S+3]=2;break;case 16:ram[c.S+4]^=1;break;case 17:ram[0x518+c.X]=1;break;case 18:ram[0x500+c.X]=0;break;case 19:ram[0x500]=0;break;case 20:ram[0x520]=0;break;case 21:ram[0x521]=1;break;case 22:ram[0x406]=1;break;case 23:ram[0x100e]=1;break;case 24:ram[0x23d]=14;break;case 25:ram[0x275]=0;break;case 26:put(0x48e,75);break;case 27:c.A=0x12;break;case 28:decline(&c,8,9,1);continue;case 29:decline(&c,9,9,0);continue;}
  decline(&c,9,9,1);
 }
 const unsigned pcs[]={0x0080b9,0x018184,0x028000,0x0280a0,0x028129,0x0180af},lengths[]={0x30,0x37,0x42,0x89,0x103,0x18};
 for(unsigned s=0;s<6;s++)for(unsigned b=0;b<lengths[s];b++){
  unsigned p=((pcs[s]>>16)&127)*32768+(pcs[s]&32767)+b;CpuState c=prepare(0x11,0,0,1,2);rom[p]^=1;decline(&c,9,9,1);rom[p]^=1;mutations++;
 }
 printf("PASS actor command prefix %u fullRAM/register/flags/deadstack cases,64 realJSLframe entries,%u readonlyrefusals,%u source-byte mutations,%llu originalopcodes/%llu sourcecycles; native math preserves entry clocks, actualRTL retained\n",cases,refusals,mutations,(unsigned long long)opcodes,(unsigned long long)cycles);return 0;
}
