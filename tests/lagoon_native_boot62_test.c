#define LAGOON_NATIVE_BOOT62_TEST 1
#include "../src/lagoon_native_boot62.c"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static uint8_t rom[0x100000],ram[0x20000],before[0x20000],actual_ram[0x20000],io[65536],actual_io[65536];
static uint64_t deadline,bus_extra;static uint8_t oracle_open_bus;
static unsigned cases,completions,interruptions,refusals;
uint32_t cpu_region_speed(uint32_t address){(void)address;return 8;}
void cpu_instruction_timing_commit(CpuState*c){c->coprocessor_master_cycles=c->master_cycles;}
bool interp_bridge_commit_source_opcode(CpuState*c,uint64_t prior,bool wrote){(void)wrote;assert(prior<=c->master_cycles);cpu_instruction_timing_commit(c);return true;}
bool interp_bridge_source_opcode_deadline_reached(const CpuState*c){return deadline&&c->master_cycles>=deadline;}
int interp816_opcode_hook(uint32_t address){(void)address;return 0;}
static uint8_t value(uint32_t address){int32_t o=cpu_wram_offset(address>>16,address);if(o>=0)return ram[o];if((address&65535)>=32768)return rom[((address>>16)&31)*32768+(address&32767)];return io[address&65535];}
uint8_t cpu_read8(CpuState*c,uint8_t b,uint16_t a){return c->open_bus=value((uint32_t)b<<16|a);}
static void store(uint32_t address,uint8_t v){int32_t o=cpu_wram_offset(address>>16,address);if(o>=0)ram[o]=v;else io[address&65535]=v;}
void cpu_write8(CpuState*c,uint8_t b,uint16_t a,uint8_t v){c->open_bus=v;store((uint32_t)b<<16|a,v);}
static uint8_t rd(void*ctx,uint32_t a){(void)ctx;bus_extra+=2;return oracle_open_bus=value(a);}
static void wr(void*ctx,uint32_t a,uint8_t v){(void)ctx;bus_extra+=2;oracle_open_bus=v;store(a,v);}
static void from(Interp816*i,const CpuState*c,uint32_t pc){i->a=c->A;i->x=c->X;i->y=c->Y;i->sp=c->S;i->dp=c->D;i->db=c->DB;i->k=c->PB;i->pc=pc;i->e=c->emulation;interp816_setFlags(i,c->P);}
static void opcode(Interp816*i,CpuState*c){bus_extra=0;unsigned n=interp816_runOpcode(i);c->cycles+=n;c->master_cycles+=n*6+bus_extra;c->coprocessor_master_cycles=c->master_cycles;c->A=i->a;c->X=i->x;c->Y=i->y;c->S=i->sp;c->D=i->dp;c->DB=i->db;c->PB=i->k;c->P=interp816_getFlags(i);cpu_p_to_mirrors(c);c->open_bus=oracle_open_bus;}
static void setup(CpuState*c,Interp816*i,unsigned flags){
 memset(&witness,0,sizeof witness);deadline=0;for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8_t)(n*73+flags*17);memset(io,0,sizeof io);memset(c,0,sizeof*c);c->ram=ram;c->X=c->S=0x1ff;c->Y=0x1234;c->P=0x24|((flags&1)?1:0)|((flags&2)?0x40:0)|((flags&4)?0x80:0)|((flags&8)?2:0);cpu_p_to_mirrors(c);from(i,c,0x8033);
 const uint32_t prefix[]={0x8033,0x8035,0x8038,0x803a};uint32_t landing=0;
 for(unsigned n=0;n<4;n++){assert(i->pc==prefix[n]);CpuState old=*c;assert(!LagoonNativeBoot62_Process(rom,sizeof rom,c,c,7,7,i->pc,&landing));assert(!memcmp(&old,c,sizeof old));opcode(i,c);}
 assert(i->pc==0x803d&&witness.phase==1&&witness.next==0x803d);
}
static void equal(const CpuState*a,const CpuState*b,const Interp816*i,uint32_t pc){
 assert(!memcmp(ram,actual_ram,sizeof ram));assert(!memcmp(io,actual_io,sizeof io));assert(i->pc==pc&&!i->k);
 assert(a->A==b->A&&a->X==b->X&&a->Y==b->Y&&a->S==b->S&&a->D==b->D&&a->DB==b->DB&&a->PB==b->PB&&a->P==b->P&&a->open_bus==b->open_bus);
 assert(a->cycles==b->cycles&&a->master_cycles==b->master_cycles&&a->coprocessor_master_cycles==b->coprocessor_master_cycles);
}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonNativeBoot62_Source(rom,sizeof rom));
 const uint64_t cuts[]={0,1,20,37,100,1000,100000,500000,1000000,3000000,6000000,9000000,12000000};
 for(unsigned flags=0;flags<16;flags++)for(unsigned cut=0;cut<sizeof cuts/sizeof *cuts;cut++){
  CpuState c;Interp816*i=interp816_init(NULL,rd,wr);setup(&c,i,flags);if(cut&1){pace_allowed=0;for(unsigned n=0;n<24;n++){CpuState old=c;uint32_t unused;assert(!LagoonNativeBoot62_Process(rom,sizeof rom,&c,&c,7,7,i->pc,&unused));assert(!memcmp(&old,&c,sizeof c));opcode(i,&c);}pace_allowed=1;}
  unsigned startpc=i->pc;CpuState reference=c;memcpy(before,ram,sizeof ram);deadline=cuts[cut]?c.master_cycles+cuts[cut]:0;uint32_t landing=0;assert(LagoonNativeBoot62_Process(rom,sizeof rom,&c,&c,7,7,startpc,&landing));cpu_mirrors_to_p(&c);memcpy(actual_ram,ram,sizeof ram);memcpy(actual_io,io,sizeof io);memcpy(ram,before,sizeof ram);memset(io,0,sizeof io);io[0x2100]=0x80;
  while(reference.cycles<c.cycles)opcode(i,&reference);equal(&c,&reference,i,landing);cases++;
  if(landing!=0x8059){interruptions++;deadline=0;assert(LagoonNativeBoot62_Process(rom,sizeof rom,&c,&c,7,7,landing,&landing));cpu_mirrors_to_p(&c);memcpy(actual_ram,ram,sizeof ram);memcpy(actual_io,io,sizeof io);memcpy(ram,before,sizeof ram);memset(io,0,sizeof io);io[0x2100]=0x80;from(i,&reference,i->pc);
   /* Reference RAM already reflected the interrupted prefix; restore that exact
    * prefix by replaying from the original source entry for the terminal check. */
   CpuState reset;setup(&reset,i,flags);while(reset.cycles<c.cycles)opcode(i,&reset);equal(&c,&reset,i,landing);cases++;
  }else completions++;
  assert(landing==0x8059);interp816_free(i);
 }
 for(unsigned n=0x13;n<0x59;n++){rom[n]^=1;assert(!LagoonNativeBoot62_Source(rom,sizeof rom));rom[n]^=1;refusals++;}
 assert(!LagoonNativeBoot62_Source(NULL,sizeof rom));assert(!LagoonNativeBoot62_Source(rom,sizeof rom-1));refusals+=2;
 for(unsigned n=0;n<12;n++){
  CpuState c;Interp816*i=interp816_init(NULL,rd,wr);setup(&c,i,0);if(n==0)c.S--;if(n==1)c.DB=1;if(n==2)c.D=1;if(n==3)c.PB=1;if(n==4)c.emulation=1;if(n==5)c.host_return_valid=3;if(n==6){c.P|=8;cpu_p_to_mirrors(&c);}if(n==7){c.P&=~4;cpu_p_to_mirrors(&c);}if(n==8){c.P|=16;cpu_p_to_mirrors(&c);}if(n==9)c.ram=NULL;
  CpuState old=c;uint32_t landing=0;assert(!LagoonNativeBoot62_Process(rom,sizeof rom,&c,&c,n==10?8:7,n==11?8:7,0x803d,&landing));assert(!memcmp(&c,&old,sizeof c));refusals++;interp816_free(i);
 }
 printf("PASS boot62: %u original-opcode comparisons, %u uninterrupted completions, %u interrupted/resumed windows, %u source/mode/stack/generation refusals; full128KiB RAM+IO, registers, flags, openbus and CPU/master/copro clocks; constant8master unitbus\n",cases,completions,interruptions,refusals);return 0;
}
