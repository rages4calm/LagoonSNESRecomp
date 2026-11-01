#define LAGOON_NATIVE_SPC63_TEST 1
#include "../src/lagoon_native_spc63.c"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static uint8_t rom[0x100000],ram[0x20000],before[0x20000],actual_ram[0x20000],io[65536],actual_io[65536];
static unsigned reads,reply,delay;
static uint64_t deadline,bus_extra;static uint8_t oracle_open_bus;
static unsigned cases,refusals;
uint32_t cpu_region_speed(uint32_t address){(void)address;return 8;}
void cpu_instruction_timing_commit(CpuState*c){c->coprocessor_master_cycles=c->master_cycles;}
bool interp_bridge_commit_source_opcode(CpuState*c,uint64_t prior,bool wrote){(void)wrote;assert(prior<=c->master_cycles);cpu_instruction_timing_commit(c);return true;}
bool interp_bridge_source_opcode_deadline_reached(const CpuState*c){return deadline&&c->master_cycles>=deadline;}
int interp816_opcode_hook(uint32_t address){(void)address;return 0;}
static uint8_t value(uint32_t address){if((address&65535)==0x2140&&(address>>16)<0x40)return reads++<delay?(reply^1):reply;int32_t o=cpu_wram_offset(address>>16,address);if(o>=0)return ram[o];if((address&65535)>=32768)return rom[((address>>16)&31)*32768+(address&32767)];return io[address&65535];}
uint8_t cpu_read8(CpuState*c,uint8_t b,uint16_t a){return c->open_bus=value((uint32_t)b<<16|a);}
static void store(uint32_t address,uint8_t v){int32_t o=cpu_wram_offset(address>>16,address);if(o>=0)ram[o]=v;else io[address&65535]=v;}
void cpu_write8(CpuState*c,uint8_t b,uint16_t a,uint8_t v){c->open_bus=v;store((uint32_t)b<<16|a,v);}
uint16_t cpu_read16(CpuState*c,uint8_t b,uint16_t a){unsigned v=cpu_read8(c,b,a);return v|cpu_read8(c,b,(uint16_t)(a+1))<<8;}
void cpu_write16(CpuState*c,uint8_t b,uint16_t a,uint16_t v){cpu_write8(c,b,a,v);cpu_write8(c,b,(uint16_t)(a+1),v>>8);}
static uint8_t rd(void*ctx,uint32_t a){(void)ctx;bus_extra+=2;return oracle_open_bus=value(a);}
static void wr(void*ctx,uint32_t a,uint8_t v){(void)ctx;bus_extra+=2;oracle_open_bus=v;store(a,v);}
static void from(Interp816*i,const CpuState*c,uint32_t pc){i->a=c->A;i->x=c->X;i->y=c->Y;i->sp=c->S;i->dp=c->D;i->db=c->DB;i->k=c->PB;i->pc=pc;i->e=c->emulation;interp816_setFlags(i,c->P);}
static void opcode(Interp816*i,CpuState*c){bus_extra=0;unsigned n=interp816_runOpcode(i);c->cycles+=n;c->master_cycles+=n*6+bus_extra;c->coprocessor_master_cycles=c->master_cycles;c->A=i->a;c->X=i->x;c->Y=i->y;c->S=i->sp;c->D=i->dp;c->DB=i->db;c->PB=i->k;c->P=interp816_getFlags(i);cpu_p_to_mirrors(c);c->open_bus=oracle_open_bus;}


bool interp_bridge_source_read8(CpuState*c,uint32_t address,uint8_t*out){*out=cpu_read8(c,address>>16,address);return true;}
static void equal(const CpuState*a,const CpuState*b,const Interp816*i,uint32_t pc){
 if(memcmp(ram,actual_ram,sizeof ram)||memcmp(io,actual_io,sizeof io)||i->pc!=(pc&65535)||i->k!=12||
 a->A!=b->A||a->X!=b->X||a->Y!=b->Y||a->S!=b->S||a->D!=b->D||a->DB!=b->DB||a->P!=b->P||a->open_bus!=b->open_bus||
 a->cycles!=b->cycles||a->master_cycles!=b->master_cycles||a->coprocessor_master_cycles!=b->coprocessor_master_cycles){
 fprintf(stderr,"FAIL case%u pc=%04x/%04x A=%04x/%04x X=%04x/%04x P=%02x/%02x bus=%02x/%02x clocks=%llu/%llu\n",cases,pc,i->pc,a->A,b->A,a->X,b->X,a->P,b->P,a->open_bus,b->open_bus,(unsigned long long)a->master_cycles,(unsigned long long)b->master_cycles);exit(2);}
}

static void setup(CpuState*c,unsigned seed){memset(&spc_witness,0,sizeof spc_witness);memset(ram,0,sizeof ram);memset(io,0,sizeof io);memset(c,0,sizeof*c);c->ram=ram;c->PB=12;c->S=0x1fa;c->P=0x24|((seed&1)?1:0)|((seed&2)?0x40:0)|((seed&4)?0x80:0)|((seed&8)?2:0);cpu_p_to_mirrors(c);ram[0x1fb]=0x5b;ram[0x1fc]=0xff;ram[0x1d]=0x80;ram[0x1e]=0x0d;
 if(seed%3){const unsigned packs[]={0xc8000,0xd8d46,0xfa12e,0xfba6b,0xfce57,0xe8000,0xe9d15,0xec0ab,0xf8000,0xfd429};unsigned id=seed%10,p=packs[id];c->DB=seed%3;ram[0x1c]=p;ram[0x1d]=p>>8;ram[0x1e]=p>>16;unsigned ret=id<2?0xfee2:0xff33;ram[0x1fb]=ret;ram[0x1fc]=ret>>8;}
 uint32_t unused;assert(!LagoonNativeSpc63_Process(rom,sizeof rom,c,c,7,7,0xcff69,&unused));assert(spc_witness.armed);c->S--;c->A=(uint16_t)(seed*911);c->X=0x1234;c->Y=0x4321;reply=c->A&255;delay=seed%11;reads=0;}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonNativeSpc63_Source(rom,sizeof rom));const unsigned pcs[]={0xcff8a,0xcff8d,0xcff9a,0xcff9d,0xcffc3,0xcffc6};const unsigned cuts[]={1,20,40,80,1000};
 for(unsigned seed=0;seed<128;seed++)for(unsigned op=0;op<6;op++)for(unsigned cut=0;cut<5;cut++){
 CpuState c;setup(&c,seed);CpuState ref=c;memcpy(before,ram,sizeof ram);deadline=cuts[cut];uint32_t landing=0;assert(LagoonNativeSpc63_Process(rom,sizeof rom,&c,&c,7,7,pcs[op],&landing));unsigned native_reads=reads;cpu_mirrors_to_p(&c);memcpy(actual_ram,ram,sizeof ram);memcpy(actual_io,io,sizeof io);memcpy(ram,before,sizeof ram);memset(io,0,sizeof io);reads=0;Interp816*i=interp816_init(NULL,rd,wr);from(i,&ref,pcs[op]);while(ref.cycles<c.cycles)opcode(i,&ref);equal(&c,&ref,i,landing);assert(reads==native_reads);cases++;
 if(landing==pcs[op]||landing==pcs[op]+3||landing==pcs[op]-3){deadline=c.master_cycles+1000;CpuState start=c;unsigned prior_reads=reads;memcpy(before,ram,sizeof ram);if(LagoonNativeSpc63_Process(rom,sizeof rom,&c,&c,7,7,landing,&landing)){native_reads=reads;cpu_mirrors_to_p(&c);memcpy(actual_ram,ram,sizeof ram);memcpy(actual_io,io,sizeof io);memcpy(ram,before,sizeof ram);reads=prior_reads;from(i,&start,i->pc);while(start.cycles<c.cycles)opcode(i,&start);equal(&c,&start,i,landing);assert(reads==native_reads);cases++;}}interp816_free(i);}
 for(unsigned k=0x67e9b;k<0x68000;k++){rom[k]^=1;assert(!LagoonNativeSpc63_Source(rom,sizeof rom));rom[k]^=1;refusals++;}
 for(unsigned k=0x68000;k<0x68d46;k++){rom[k]^=1;assert(!LagoonNativeSpc63_Source(rom,sizeof rom));rom[k]^=1;refusals++;}
 for(unsigned k=0;k<15;k++){CpuState c;setup(&c,0);if(k==0)c.S--;if(k==1)c.DB=1;if(k==2)c.D=1;if(k==3)c.PB=1;if(k==4)c.emulation=1;if(k==5)c.m_flag=0;if(k==6)c.x_flag=1;if(k==7)c._flag_D=1;if(k==8)c.ram=NULL;if(k==9)c.ram[0x1fb]^=1;if(k==10)c.ram[0x1e]^=1;if(k==11){c.P&=~4;cpu_p_to_mirrors(&c);}CpuState old=c;uint32_t landing;assert(!LagoonNativeSpc63_Process(rom,sizeof rom,&c,k==12?NULL:&c,k==13?8:7,k==14?8:7,0xcff8a,&landing));assert(!memcmp(&old,&c,sizeof c));refusals++;}
 printf("PASS spc63: %u original-opcode/deadline/resume cases, %u source/mode/stack/generation refusals; scripted changing device replies, full state and clocks; real SPC device replay separate\n",cases,refusals);return 0;}
