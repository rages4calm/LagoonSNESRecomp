#define LAGOON_NATIVE_DECODE63_TEST 1
#include "../src/lagoon_native_decode63.c"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static uint8_t rom[0x100000],ram[0x20000],before[0x20000],actual_ram[0x20000],io[65536],actual_io[65536];
static uint64_t deadline,bus_extra;static uint8_t oracle_open_bus;
static unsigned cases,refusals;
uint32_t cpu_region_speed(uint32_t address){(void)address;return 8;}
void cpu_instruction_timing_commit(CpuState*c){c->coprocessor_master_cycles=c->master_cycles;}
bool interp_bridge_commit_source_opcode(CpuState*c,uint64_t prior,bool wrote){(void)wrote;assert(prior<=c->master_cycles);cpu_instruction_timing_commit(c);return true;}
bool interp_bridge_source_opcode_deadline_reached(const CpuState*c){return deadline&&c->master_cycles>=deadline;}
int interp816_opcode_hook(uint32_t address){(void)address;return 0;}
static uint8_t value(uint32_t address){int32_t o=cpu_wram_offset(address>>16,address);if(o>=0)return ram[o];if((address&65535)>=32768)return rom[((address>>16)&31)*32768+(address&32767)];return io[address&65535];}
uint8_t cpu_read8(CpuState*c,uint8_t b,uint16_t a){return c->open_bus=value((uint32_t)b<<16|a);}
static void store(uint32_t address,uint8_t v){int32_t o=cpu_wram_offset(address>>16,address);if(o>=0)ram[o]=v;else io[address&65535]=v;}
void cpu_write8(CpuState*c,uint8_t b,uint16_t a,uint8_t v){c->open_bus=v;store((uint32_t)b<<16|a,v);}
uint16_t cpu_read16(CpuState*c,uint8_t b,uint16_t a){unsigned v=cpu_read8(c,b,a);return v|cpu_read8(c,b,(uint16_t)(a+1))<<8;}
void cpu_write16(CpuState*c,uint8_t b,uint16_t a,uint16_t v){cpu_write8(c,b,a,v);cpu_write8(c,b,(uint16_t)(a+1),v>>8);}
static uint8_t rd(void*ctx,uint32_t a){(void)ctx;bus_extra+=2;return oracle_open_bus=value(a);}
static void wr(void*ctx,uint32_t a,uint8_t v){(void)ctx;bus_extra+=2;oracle_open_bus=v;store(a,v);}
static void from(Interp816*i,const CpuState*c,uint32_t pc){i->a=c->A;i->x=c->X;i->y=c->Y;i->sp=c->S;i->dp=c->D;i->db=c->DB;i->k=c->PB;i->pc=pc;i->e=c->emulation;interp816_setFlags(i,c->P);}
static void opcode(Interp816*i,CpuState*c){bus_extra=0;unsigned n=interp816_runOpcode(i);c->cycles+=n;c->master_cycles+=n*6+bus_extra;c->coprocessor_master_cycles=c->master_cycles;c->A=i->a;c->X=i->x;c->Y=i->y;c->S=i->sp;c->D=i->dp;c->DB=i->db;c->PB=i->k;c->P=interp816_getFlags(i);cpu_p_to_mirrors(c);c->open_bus=oracle_open_bus;}

static void equal(const CpuState*a,const CpuState*b,const Interp816*i,uint32_t pc){
 if(memcmp(ram,actual_ram,sizeof ram)||memcmp(io,actual_io,sizeof io)||i->pc!=pc||i->k||
 a->A!=b->A||a->X!=b->X||a->Y!=b->Y||a->S!=b->S||a->D!=b->D||a->DB!=b->DB||a->P!=b->P||a->open_bus!=b->open_bus||
 a->cycles!=b->cycles||a->master_cycles!=b->master_cycles||a->coprocessor_master_cycles!=b->coprocessor_master_cycles){
 fprintf(stderr,"FAIL case%u pc=%04x/%04x A=%04x/%04x X=%04x/%04x P=%02x/%02x bus=%02x/%02x clocks=%llu/%llu\n",cases,pc,i->pc,a->A,b->A,a->X,b->X,a->P,b->P,a->open_bus,b->open_bus,(unsigned long long)a->master_cycles,(unsigned long long)b->master_cycles);exit(2);}
}
static void setup(CpuState*c,unsigned seed){
 memset(&decode_witness,0,sizeof decode_witness);for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8_t)(n*73+seed*17);memset(io,0,sizeof io);memset(c,0,sizeof*c);
 c->ram=ram;c->S=0x1e0;c->DB=seed%3;c->P=0x24|((seed&1)?1:0)|((seed&2)?0x40:0)|((seed&4)?0x80:0)|((seed&8)?2:0);cpu_p_to_mirrors(c);
 uint32_t unused;CpuState old=*c;assert(!LagoonNativeDecode63_Process(rom,sizeof rom,c,c,7,7,0x9143,&unused));assert(!memcmp(c,&old,sizeof old));assert(decode_witness.armed);
 c->S-=6;c->A=(uint16_t)(seed*911);c->X=(uint16_t)(seed*2017);c->Y=(uint16_t)(seed*97);
 /* Preserve the physical owner frame; arbitrary dictionary bytes exercise
  * high index carries and both traversal branches without assuming a result. */
}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonNativeDecode63_Source(rom,sizeof rom));
 const uint64_t cuts[]={1,20,80,500,2000};
 for(unsigned seed=0;seed<64;seed++)for(unsigned op=0;op<sizeof decode_ops/sizeof *decode_ops;op++)for(unsigned cut=0;cut<sizeof cuts/sizeof *cuts;cut++){
  CpuState c;setup(&c,seed);unsigned pc=decode_ops[op][0];CpuState ref=c;memcpy(before,ram,sizeof ram);deadline=cuts[cut];uint32_t landing=0;
  assert(LagoonNativeDecode63_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing));cpu_mirrors_to_p(&c);memcpy(actual_ram,ram,sizeof ram);memcpy(actual_io,io,sizeof io);memcpy(ram,before,sizeof ram);memset(io,0,sizeof io);
  Interp816*i=interp816_init(NULL,rd,wr);from(i,&ref,pc);while(ref.cycles<c.cycles)opcode(i,&ref);equal(&c,&ref,i,landing);cases++;
  /* Resume from an actual stopped source PC with the very same live frame. */
  if(landing>=0x925d&&landing<0x92c7&&cut==4){deadline=c.master_cycles+500;CpuState start=c;memcpy(before,ram,sizeof ram);
   if(LagoonNativeDecode63_Process(rom,sizeof rom,&c,&c,7,7,landing,&landing)){cpu_mirrors_to_p(&c);memcpy(actual_ram,ram,sizeof ram);memcpy(actual_io,io,sizeof io);memcpy(ram,before,sizeof ram);from(i,&start,i->pc);while(start.cycles<c.cycles)opcode(i,&start);equal(&c,&start,i,landing);cases++;}}
  interp816_free(i);
 }
 for(unsigned k=0x1143;k<0x1304;k++){rom[k]^=1;assert(!LagoonNativeDecode63_Source(rom,sizeof rom));rom[k]^=1;refusals++;}
 for(unsigned k=0;k<16;k++){CpuState c;setup(&c,0);if(k==0)c.S=0x100;if(k==1)c.S=0x1e1;if(k==2)c.DB=4;if(k==3)c.D=1;if(k==4)c.PB=1;if(k==5)c.emulation=1;if(k==6)c.m_flag=0;if(k==7)c.x_flag=1;if(k==8)c._flag_D=1;if(k==9)c.ram=NULL;if(k==10)c.ram[0x1e1]^=1;if(k==11)c.P^=16;if(k==12){c.P&=~4;cpu_p_to_mirrors(&c);}
 CpuState old=c;uint32_t landing;assert(!LagoonNativeDecode63_Process(rom,sizeof rom,&c,k==13?NULL:&c,k==14?8:7,k==15?8:7,0x925d,&landing));assert(!memcmp(&old,&c,sizeof c));refusals++;}
 printf("PASS decode63: %u source/deadline/resume comparisons, %u source/mode/stack/generation refusals; full RAM+IO/registers/openbus/clocks, constant8master bus\n",cases,refusals);return 0;}
