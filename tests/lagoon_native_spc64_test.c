#define LAGOON_NATIVE_SPC63_TEST 1
#include "../src/lagoon_native_spc63.c"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static uint8_t rom[0x100000],ram[0x20000],before[0x20000],actual_ram[0x20000],io[65536],actual_io[65536];
static unsigned reads,reply,delay,events;
static struct {uint32_t a;uint8_t v;} trace[4096],native_trace[4096];
static uint64_t deadline,bus_extra;static uint8_t oracle_open_bus;
static unsigned cases,refusals;
uint32_t cpu_region_speed(uint32_t address){return (address&65535)>=0x2140&&(address&65535)<=0x2143&&(address>>16)<0x40?6:8;}
void cpu_instruction_timing_commit(CpuState*c){c->coprocessor_master_cycles=c->master_cycles;}
bool interp_bridge_commit_source_opcode(CpuState*c,uint64_t prior,bool wrote){(void)wrote;assert(prior<=c->master_cycles);cpu_instruction_timing_commit(c);return true;}
bool interp_bridge_source_opcode_deadline_reached(const CpuState*c){return deadline&&c->master_cycles>=deadline;}
int interp816_opcode_hook(uint32_t address){(void)address;return 0;}
static uint8_t value(uint32_t address){if((address&65535)==0x2140&&(address>>16)<0x40)return reads++<delay?(reply^1):reply;int32_t o=cpu_wram_offset(address>>16,address);if(o>=0)return ram[o];if((address&65535)>=32768)return rom[((address>>16)&31)*32768+(address&32767)];return io[address&65535];}
uint8_t cpu_read8(CpuState*c,uint8_t b,uint16_t a){return c->open_bus=value((uint32_t)b<<16|a);}
static void store(uint32_t address,uint8_t v){if((address&65535)>=0x2140&&(address&65535)<=0x2143&&(address>>16)<0x40){assert(events<4096);trace[events].a=address;trace[events++].v=v;if((address&65535)==0x2140){reply=v;delay=reads+2;}}int32_t o=cpu_wram_offset(address>>16,address);if(o>=0)ram[o]=v;else io[address&65535]=v;}
void cpu_write8(CpuState*c,uint8_t b,uint16_t a,uint8_t v){c->open_bus=v;store((uint32_t)b<<16|a,v);}
uint16_t cpu_read16(CpuState*c,uint8_t b,uint16_t a){unsigned v=cpu_read8(c,b,a);return v|cpu_read8(c,b,(uint16_t)(a+1))<<8;}
void cpu_write16(CpuState*c,uint8_t b,uint16_t a,uint16_t v){cpu_write8(c,b,a,v);cpu_write8(c,b,(uint16_t)(a+1),v>>8);}
static uint8_t rd(void*ctx,uint32_t a){(void)ctx;bus_extra+=cpu_region_speed(a)-6;return oracle_open_bus=value(a);}
static void wr(void*ctx,uint32_t a,uint8_t v){(void)ctx;bus_extra+=cpu_region_speed(a)-6;oracle_open_bus=v;store(a,v);}
static void from(Interp816*i,const CpuState*c,uint32_t pc){i->a=c->A;i->x=c->X;i->y=c->Y;i->sp=c->S;i->dp=c->D;i->db=c->DB;i->k=c->PB;i->pc=pc;i->e=c->emulation;interp816_setFlags(i,c->P);}
static void opcode(Interp816*i,CpuState*c){bus_extra=0;unsigned n=interp816_runOpcode(i);c->cycles+=n;c->master_cycles+=n*6+bus_extra;c->coprocessor_master_cycles=c->master_cycles;c->A=i->a;c->X=i->x;c->Y=i->y;c->S=i->sp;c->D=i->dp;c->DB=i->db;c->PB=i->k;c->P=interp816_getFlags(i);cpu_p_to_mirrors(c);c->open_bus=oracle_open_bus;}


bool interp_bridge_source_read8(CpuState*c,uint32_t address,uint8_t*out){*out=cpu_read8(c,address>>16,address);return true;}
bool interp_bridge_source_read16(CpuState*c,uint32_t l,uint32_t h,uint16_t*out){unsigned v=cpu_read8(c,l>>16,l);*out=v|cpu_read8(c,h>>16,h)<<8;return true;}
bool interp_bridge_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_source_write16(CpuState*c,uint32_t l,uint32_t h,uint16_t v){cpu_write8(c,l>>16,l,v);cpu_write8(c,h>>16,h,v>>8);return true;}

static void equal(const CpuState*a,const CpuState*b,const Interp816*i,uint32_t pc){
 if(memcmp(ram,actual_ram,sizeof ram)||memcmp(io,actual_io,sizeof io)||i->pc!=(pc&65535)||i->k!=12||
 a->A!=b->A||a->X!=b->X||a->Y!=b->Y||a->S!=b->S||a->D!=b->D||a->DB!=b->DB||a->P!=b->P||a->open_bus!=b->open_bus||
 a->cycles!=b->cycles||a->master_cycles!=b->master_cycles||a->coprocessor_master_cycles!=b->coprocessor_master_cycles){
 fprintf(stderr,"FAIL case%u pc=%04x/%04x A=%04x/%04x X=%04x/%04x P=%02x/%02x bus=%02x/%02x clocks=%llu/%llu\n",cases,pc,i->pc,a->A,b->A,a->X,b->X,a->P,b->P,a->open_bus,b->open_bus,(unsigned long long)a->master_cycles,(unsigned long long)b->master_cycles);exit(2);}
}

static void setup(CpuState*c,unsigned seed){memset(&spc_witness,0,sizeof spc_witness);memset(ram,0,sizeof ram);memset(io,0,sizeof io);memset(c,0,sizeof*c);c->ram=ram;c->PB=12;c->S=0x1fa;c->P=0x24|((seed&1)?1:0)|((seed&2)?0x40:0)|((seed&4)?0x80:0)|((seed&8)?2:0);cpu_p_to_mirrors(c);ram[0x1fb]=0x5b;ram[0x1fc]=0xff;ram[0x1d]=0x80;ram[0x1e]=0x0d;
 if(seed%3){const unsigned packs[]={0xc8000,0xd8d46,0xfa12e,0xfba6b,0xfce57,0xe8000,0xe9d15,0xec0ab,0xf8000,0xfd429};unsigned id=seed%10,p=packs[id];c->DB=seed%3;ram[0x1c]=p;ram[0x1d]=p>>8;ram[0x1e]=p>>16;unsigned ret=id<2?0xfee2:0xff33;ram[0x1fb]=ret;ram[0x1fc]=ret>>8;}
 uint32_t unused;assert(!LagoonNativeSpc63_Process(rom,sizeof rom,c,c,7,7,0xcff69,&unused));assert(spc_witness.armed);c->ram[c->S]=spc_witness.saved_p;c->S--;c->A=(uint16_t)(seed*911);c->X=0x1234;c->Y=0x4321;reply=c->A&255;delay=seed%11;reads=0;}

int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonNativeSpc63_Source(rom,sizeof rom));const unsigned cuts[]={1,30,100,500,2000};
for(unsigned seed=0;seed<48;seed++)for(unsigned op=0;op<sizeof spc64_ops/sizeof *spc64_ops;op++){
 unsigned pc=spc64_ops[op][0];if(pc==0xcff8a||pc==0xcff8d||pc==0xcff9a||pc==0xcff9d||pc==0xcffc3||pc==0xcffc6)continue;
 for(unsigned cut=0;cut<5;cut++){
 CpuState c;setup(&c,seed);events=0;memset(trace,0,sizeof trace);if(!spc64_ops[op][3]){c.P&=~32;cpu_p_to_mirrors(&c);}c.Y=(seed*13)&1023;c.X=seed%5+1;
 if(pc>=0xcffa4&&pc<=0xcffbf){c.S--;ram[c.S+1]=(uint8_t)(seed*3);}
 io[0x2141]=c.A>>8;reply=c.A&255;delay=seed%5;reads=0;unsigned initial_reply=reply,initial_delay=delay;
 CpuState ref=c;memcpy(before,ram,sizeof ram);uint8_t initial_io[65536];memcpy(initial_io,io,sizeof io);deadline=cuts[cut];uint32_t landing=0;
 assert(LagoonNativeSpc63_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing));unsigned native_reads=reads,native_events=events;cpu_mirrors_to_p(&c);memcpy(actual_ram,ram,sizeof ram);memcpy(actual_io,io,sizeof io);memcpy(native_trace,trace,sizeof trace);
 memcpy(ram,before,sizeof ram);memcpy(io,initial_io,sizeof io);reply=initial_reply;delay=initial_delay;reads=events=0;memset(trace,0,sizeof trace);Interp816*i=interp816_init(NULL,rd,wr);from(i,&ref,pc);while(ref.cycles<c.cycles)opcode(i,&ref);equal(&c,&ref,i,landing);assert(reads==native_reads&&events==native_events&&!memcmp(trace,native_trace,sizeof trace));cases++;
 if(landing!=0xcffca&&cut==4){deadline=c.master_cycles+500;CpuState start=c;memcpy(before,ram,sizeof ram);memcpy(initial_io,io,sizeof io);initial_reply=reply;initial_delay=delay;unsigned prior_reads=reads,prior_events=events;
  if(LagoonNativeSpc63_Process(rom,sizeof rom,&c,&c,7,7,landing,&landing)){native_reads=reads;native_events=events;cpu_mirrors_to_p(&c);memcpy(actual_ram,ram,sizeof ram);memcpy(actual_io,io,sizeof io);memcpy(native_trace,trace,sizeof trace);memcpy(ram,before,sizeof ram);memcpy(io,initial_io,sizeof io);reply=initial_reply;delay=initial_delay;reads=prior_reads;events=prior_events;from(i,&start,i->pc);while(start.cycles<c.cycles)opcode(i,&start);equal(&c,&start,i,landing);assert(reads==native_reads&&events==native_events&&!memcmp(trace,native_trace,sizeof trace));cases++;}}
 interp816_free(i);
 }}

/* New M0 handshake admission and saved-PHP/header frame fences. */
for(unsigned n=0;n<18;n++){CpuState c;setup(&c,0);c.P&=~32;cpu_p_to_mirrors(&c);
 if(n==0){c.P|=32;cpu_p_to_mirrors(&c);}if(n==1)c.x_flag=1;if(n==2)c.D=1;if(n==3)c.DB=3;if(n==4)c.PB=1;if(n==5)c.emulation=1;if(n==6)c._flag_D=1;if(n==7)c._flag_I=0;if(n==8)c.S--;if(n==9)ram[spc_witness.s+1]^=1;if(n==10)ram[spc_witness.s]^=1;if(n==11)ram[0x1c]^=1;if(n==15)rom[0x67f6a]^=1;if(n==17)c.ram=NULL;
 CpuState old=c;memcpy(before,ram,sizeof ram);uint32_t landing=0;assert(!LagoonNativeSpc63_Process(rom,sizeof rom,&c,n==12?NULL:&c,n==13?8:7,n==14?8:7,0xcff72,n==16?NULL:&landing));assert(!memcmp(&old,&c,sizeof c)&&!memcmp(before,ram,sizeof ram));if(n==15)rom[0x67f6a]^=1;refusals++;}
printf("PASS spc64: %u NEW payload/header/word-handshake and deadline/resume cases, %u NEW wide-mode/header-frame immutable refusals; full RAM/IO/registers/clocks and ordered port writes; scripted device, live APU proof separate\n",cases,refusals);return 0;}
