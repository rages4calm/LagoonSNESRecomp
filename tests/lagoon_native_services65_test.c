#define LAGOON_SERVICES65_TEST 1
#include "../src/lagoon_native_services65.c"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
static uint8_t rom[0x100000];
typedef struct {uint8_t ram[0x20000],sram[8192],io[65536],vram[65536];unsigned va;uint64_t writes;} Memory;
static Memory mem,before,actual;
static uint8_t*ram=mem.ram;
static uint64_t deadline,bus_extra;static uint8_t oracle_open_bus;static unsigned cases,refusals;
uint32_t cpu_region_speed(uint32_t address){return (address&65535)>=0x2100&&(address&65535)<0x2200&&(address>>16)<0x40?6:8;}
void cpu_instruction_timing_commit(CpuState*c){c->coprocessor_master_cycles=c->master_cycles;}
bool interp_bridge_commit_source_opcode(CpuState*c,uint64_t prior,bool wrote){assert(!wrote&&prior<=c->master_cycles);cpu_instruction_timing_commit(c);return true;}
bool interp_bridge_source_opcode_deadline_reached(const CpuState*c){return deadline&&c->master_cycles>=deadline;}
int interp816_opcode_hook(uint32_t address){(void)address;return 0;}
static uint8_t value(uint32_t address){int32_t o=cpu_wram_offset(address>>16,address);if(o>=0)return ram[o];if((address>>16)==0x70)return mem.sram[address&8191];if((address&65535)>=32768)return rom[((address>>16)&31)*32768+(address&32767)];return mem.io[address&65535];}
static void store(uint32_t address,uint8_t v){int32_t o=cpu_wram_offset(address>>16,address);mem.writes=(mem.writes^(address*257u+v))*UINT64_C(1099511628211);if(o>=0)ram[o]=v;else if((address>>16)==0x70)mem.sram[address&8191]=v;else {mem.io[address&65535]=v;switch(address&65535){case 0x2116:mem.va=(mem.va&0xff00)|v;break;case 0x2117:mem.va=(mem.va&255)|(v<<8);break;case 0x2118:case 0x2119:{unsigned high=(address&1);mem.vram[((mem.va*2)+high)&65535]=v;if(high==!!(mem.io[0x2115]&128))mem.va=(mem.va+1)&65535;break;}}}}
uint8_t cpu_read8(CpuState*c,uint8_t b,uint16_t a){return c->open_bus=value((uint32_t)b<<16|a);}
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


static void equal(const CpuState*a,const CpuState*b,const Interp816*i,unsigned pc){
 if(memcmp(&mem,&actual,sizeof mem)||i->pc!=(pc&65535)||i->k!=(pc>>16)||a->A!=b->A||a->X!=b->X||a->Y!=b->Y||a->S!=b->S||a->D!=b->D||a->DB!=b->DB||a->P!=b->P||a->open_bus!=b->open_bus||a->cycles!=b->cycles||a->master_cycles!=b->master_cycles||a->coprocessor_master_cycles!=b->coprocessor_master_cycles){fprintf(stderr,"FAIL case%u pc=%06x/%02x%04x A=%04x/%04x X=%04x/%04x Y=%04x/%04x S=%04x/%04x P=%02x/%02x bus=%02x/%02x cyc=%llu/%llu clocks=%llu/%llu memory=%d writes=%llu/%llu\n",cases,pc,i->k,i->pc,a->A,b->A,a->X,b->X,a->Y,b->Y,a->S,b->S,a->P,b->P,a->open_bus,b->open_bus,(unsigned long long)a->cycles,(unsigned long long)b->cycles,(unsigned long long)a->master_cycles,(unsigned long long)b->master_cycles,memcmp(&mem,&actual,sizeof mem),(unsigned long long)mem.writes,(unsigned long long)actual.writes);exit(2);}}
static void setup(CpuState*c,unsigned seed,unsigned root){
 memset(owners,0,sizeof owners);memset(&mem,0,sizeof mem);for(unsigned n=0;n<0x20000;n++)ram[n]=(n*73+seed*17);for(unsigned n=0;n<8192;n++)mem.sram[n]=n*19+seed;mem.io[0x2115]=128;
 memset(c,0,sizeof*c);c->ram=ram;c->S=0x1e0;c->DB=seed%3;c->PB=roots[root].pc>>16;c->P=0x24|((seed&1)?1:0)|((seed&2)?0x40:0)|((seed&4)?0x80:0)|((seed&8)?2:0);cpu_p_to_mirrors(c);ram[c->S+1]=0xff;ram[c->S+2]=0x95;ram[c->S+3]=c->PB;assert(arm(rom,sizeof rom,c,c,7,7,roots[root].pc));c->A=seed*911;c->X=(seed*2017)&65535;c->Y=(seed*97)&65535;
 ram[0x40]=0;ram[0x41]=1;ram[0x42]=0x70;ram[0x43]=0;ram[0x44]=0xf;ram[0x45]=0x70;ram[0x46]=0x10;ram[0x47]=0xf;ram[0x48]=0x70;
}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonServices65_Source(rom,sizeof rom));const unsigned cuts[]={1,30,100,500,2000};
for(unsigned seed=0;seed<32;seed++)for(unsigned op=0;op<sizeof ops/sizeof *ops;op++)for(unsigned cut=0;cut<5;cut++){
 unsigned pc=ops[op][0],root=family(pc);CpuState c;setup(&c,seed,root);if(!ops[op][3]){c.P&=~32;cpu_p_to_mirrors(&c);}if(root==0&&pc>0x9370)c.S-=2;if(root==3&&pc>0x93dc)c.S-=4;else if(root==3&&pc==0x93dc)c.S-=2;if(pc==0x9403||pc==0x9429)c.S=owners[root].s-2;if(root==6&&pc>0x9480)c.S-=2;if(root>=7&&root<11)c.S-=2;
 CpuState ref=c;before=mem;deadline=cuts[cut];uint32_t landing=0;assert(LagoonServices65_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing));cpu_mirrors_to_p(&c);actual=mem;mem=before;Interp816*i=interp816_init(NULL,rd,wr);from(i,&ref,pc);while(ref.cycles<c.cycles)opcode(i,&ref);equal(&c,&ref,i,landing);cases++;interp816_free(i);
}
/* Complete real entry->return flows. Original JSR/JSL/returns run through the
 * independent interpreter, so checksum/signature callees and stack effects
 * are never invented. Fresh and corrupt SRAM plus mirrored save/load. */
for(unsigned root=0;root<12;root++)for(unsigned seed=0;seed<3;seed++){
 CpuState c;setup(&c,seed,root);c.A=seed&1;c.X=0x1234;c.Y=0x5678;if(root>=7&&root<11){ram[0x02b5]=0;ram[0x2a7]=1;ram[0x2a8]=1;ram[0x2a9]=1;ram[0x2aa]=1;}if(root==11)ram[0x52]=seed;if((root==2||root==3)&&seed){
 /* Obtain valid redundant save data by executing the original save service;
  * then damage just one copy to exercise actual mirror fallback/repair. */
 CpuState prep=c;prep.PB=0;Interp816*original=interp816_init(NULL,rd,wr);from(original,&prep,0x9370);unsigned limit=0;while(original->pc!=0x9600&&limit++<100000)opcode(original,&prep);assert(original->pc==0x9600);interp816_free(original);if(seed==1)mem.sram[0xf00]^=1;else mem.sram[0x1f10]^=1;mem.writes=0;
 }before=mem;CpuState ref=c;unsigned pc=roots[root].pc,target=root==11?0x02c5d5:0x9600;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,pc);deadline=128;unsigned steps=0;
 while(pc!=target&&steps++<400000){if(c.master_cycles>=deadline)deadline=c.master_cycles+128;uint32_t landing;if(LagoonServices65_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing)){pc=landing;from(i,&c,pc);}else{opcode(i,&c);pc=(i->k<<16)|i->pc;}}
 if(pc!=target){fprintf(stderr,"route root%u stopped %06x\n",root,pc);return 3;}cpu_mirrors_to_p(&c);actual=mem;mem=before;from(i,&ref,roots[root].pc);steps=0;while((((unsigned)i->k<<16)|i->pc)!=target&&steps++<1000000)opcode(i,&ref);equal(&c,&ref,i,target);cases++;interp816_free(i);
}
for(unsigned fault=0;fault<13;fault++){CpuState c;setup(&c,0,5);switch(fault){case 0:c.P|=8;cpu_p_to_mirrors(&c);break;case 1:c.D=1;break;case 2:c.S-=1;break;case 3:ram[c.S+1]^=1;break;case 4:c.DB=3;break;case 5:c.P|=0x10;cpu_p_to_mirrors(&c);break;case 6:c.emulation=1;break;case 7:c.P&=~4;cpu_p_to_mirrors(&c);break;case 8:c.P&=~32;cpu_p_to_mirrors(&c);break;case 9:rom[offset(0x936a)]^=1;break;case 10:owners[5].gen++;break;case 11:owners[5].cpu=0;break;case 12:owners[5].ram=0;break;}CpuState old=c;before=mem;uint32_t landing;assert(!LagoonServices65_Process(rom,sizeof rom,&c,&c,7,7,0x9471,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));if(fault==9)rom[offset(0x936a)]^=1;refusals++;}
printf("PASS services65: %u NEW opcode/deadline and complete original service routes; %u guard refusals; full RAM/SRAM/VRAM/IO/ordered writes/registers/clocks\n",cases,refusals);return 0;}
