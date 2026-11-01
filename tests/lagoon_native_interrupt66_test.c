#define LAGOON_INTERRUPT66_TEST 1
#include "../src/lagoon_native_interrupt66.c"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
static uint8_t rom[0x100000];
typedef struct {uint8_t ram[0x20000],sram[8192],io[65536],vram[65536];unsigned va;uint64_t writes;} Memory;
static Memory mem,before,actual;
static uint8_t*ram=mem.ram;
static uint64_t deadline,bus_extra,read_master,release_at;static uint8_t oracle_open_bus;static unsigned cases,refusals,mapping_cases;static unsigned bad_mapping=~0u;
int cpu_aot_rom_mapping_matches(uint32_t pc,uint32_t offset,unsigned n){assert(n&&n<=4&&offset==(pc&32767));return !(bad_mapping>=pc&&bad_mapping<pc+n);}
uint32_t cpu_region_speed(uint32_t address){return (address&65535)>=0x2100&&(address&65535)<0x2200&&(address>>16)<0x40?6:8;}
void cpu_instruction_timing_commit(CpuState*c){c->coprocessor_master_cycles=c->master_cycles;}
bool interp_bridge_commit_source_opcode(CpuState*c,uint64_t prior,bool wrote){assert(!wrote&&prior<=c->master_cycles);cpu_instruction_timing_commit(c);return true;}
bool interp_bridge_source_opcode_deadline_reached(const CpuState*c){return deadline&&c->master_cycles>=deadline;}
int interp816_opcode_hook(uint32_t address){(void)address;return 0;}
static uint8_t value(uint32_t address){if((address>>16)<3&&((address&65535)==0x4210||(address&65535)==0x4212)){unsigned a=address&65535;uint8_t v=mem.io[a];if(a==0x4210){v=(v&127)|(read_master>=release_at?128:0);mem.io[a]=v&127;}else v=(v&254)|(read_master<release_at);mem.writes=(mem.writes^(address*257u+v))*UINT64_C(1099511628211);return v;}int32_t o=cpu_wram_offset(address>>16,address);if(o>=0)return ram[o];if((address>>16)==0x70)return mem.sram[address&8191];if((address&65535)>=32768)return rom[((address>>16)&31)*32768+(address&32767)];return mem.io[address&65535];}
static void store(uint32_t address,uint8_t v){int32_t o=cpu_wram_offset(address>>16,address);mem.writes=(mem.writes^(address*257u+v))*UINT64_C(1099511628211);if(o>=0)ram[o]=v;else if((address>>16)==0x70)mem.sram[address&8191]=v;else {mem.io[address&65535]=v;switch(address&65535){case 0x2116:mem.va=(mem.va&0xff00)|v;break;case 0x2117:mem.va=(mem.va&255)|(v<<8);break;case 0x2118:case 0x2119:{unsigned high=(address&1);mem.vram[((mem.va*2)+high)&65535]=v;if(high==!!(mem.io[0x2115]&128))mem.va=(mem.va+1)&65535;break;}}}}
uint8_t cpu_read8(CpuState*c,uint8_t b,uint16_t a){read_master=c->master_cycles;return c->open_bus=value((uint32_t)b<<16|a);}
void cpu_write8(CpuState*c,uint8_t b,uint16_t a,uint8_t v){c->open_bus=v;store((uint32_t)b<<16|a,v);}
uint16_t cpu_read16(CpuState*c,uint8_t b,uint16_t a){unsigned v=cpu_read8(c,b,a);return v|cpu_read8(c,b,(uint16_t)(a+1))<<8;}
void cpu_write16(CpuState*c,uint8_t b,uint16_t a,uint16_t v){cpu_write8(c,b,a,v);cpu_write8(c,b,(uint16_t)(a+1),v>>8);}
static uint8_t rd(void*ctx,uint32_t a){(void)ctx;bus_extra+=cpu_region_speed(a)-6;return oracle_open_bus=value(a);}
static void wr(void*ctx,uint32_t a,uint8_t v){(void)ctx;bus_extra+=cpu_region_speed(a)-6;oracle_open_bus=v;store(a,v);}
static void from(Interp816*i,const CpuState*c,uint32_t pc){i->a=c->A;i->x=c->X;i->y=c->Y;i->sp=c->S;i->dp=c->D;i->db=c->DB;i->k=c->PB;i->pc=pc;i->e=c->emulation;interp816_setFlags(i,c->P);}
static void opcode(Interp816*i,CpuState*c){bus_extra=0;read_master=c->master_cycles;unsigned n=interp816_runOpcode(i);c->cycles+=n;c->master_cycles+=n*6+bus_extra;c->coprocessor_master_cycles=c->master_cycles;c->A=i->a;c->X=i->x;c->Y=i->y;c->S=i->sp;c->D=i->dp;c->DB=i->db;c->PB=i->k;c->P=interp816_getFlags(i);cpu_p_to_mirrors(c);c->open_bus=oracle_open_bus;}


bool interp_bridge_source_read8(CpuState*c,uint32_t address,uint8_t*out){*out=cpu_read8(c,address>>16,address);return true;}
bool interp_bridge_source_read16(CpuState*c,uint32_t l,uint32_t h,uint16_t*out){unsigned v=cpu_read8(c,l>>16,l);*out=v|cpu_read8(c,h>>16,h)<<8;return true;}
bool interp_bridge_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_source_write16(CpuState*c,uint32_t l,uint32_t h,uint16_t v){cpu_write8(c,l>>16,l,v);cpu_write8(c,h>>16,h,v>>8);return true;}



bool interp_bridge_interrupt_source_read8(CpuState*c,uint32_t a,uint8_t*out){*out=cpu_read8(c,a>>16,a);return true;}
bool interp_bridge_interrupt_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_commit_interrupt_source_opcode(CpuState*c,uint64_t prior,bool wrote){return interp_bridge_commit_source_opcode(c,prior,wrote);}
static void equal(const CpuState*a,const CpuState*b,const Interp816*i,unsigned pc){
 if(memcmp(&mem,&actual,sizeof mem)||i->pc!=(pc&65535)||i->k!=(pc>>16)||a->A!=b->A||a->X!=b->X||a->Y!=b->Y||a->S!=b->S||a->D!=b->D||a->DB!=b->DB||a->P!=b->P||a->open_bus!=b->open_bus||a->cycles!=b->cycles||a->master_cycles!=b->master_cycles||a->coprocessor_master_cycles!=b->coprocessor_master_cycles){fprintf(stderr,"FAIL case%u pc=%06x/%02x%04x A=%04x/%04x X=%04x/%04x Y=%04x/%04x S=%04x/%04x P=%02x/%02x bus=%02x/%02x cyc=%llu/%llu clocks=%llu/%llu memory=%d writes=%llu/%llu\n",cases,pc,i->k,i->pc,a->A,b->A,a->X,b->X,a->Y,b->Y,a->S,b->S,a->P,b->P,a->open_bus,b->open_bus,(unsigned long long)a->cycles,(unsigned long long)b->cycles,(unsigned long long)a->master_cycles,(unsigned long long)b->master_cycles,memcmp(&mem,&actual,sizeof mem),(unsigned long long)mem.writes,(unsigned long long)actual.writes);exit(2);}}

static void setup66(CpuState*c,unsigned seed,int irq,unsigned pc){
 memset(&nmi66,0,sizeof nmi66);memset(&reset66,0,sizeof reset66);memset(&mem,0,sizeof mem);for(unsigned j=0;j<0x20000;j++)ram[j]=j*73+seed*17;
 memset(c,0,sizeof*c);c->ram=ram;c->S=0x1e0;c->DB=seed%3;c->A=seed*911;c->X=seed*2017;c->Y=seed*97;c->P=(irq&&(seed&16)?0:32)|4;cpu_p_to_mirrors(c);if(irq){c->S+=4;c->P=(c->P&~4)|1;cpu_p_to_mirrors(c);cpu_push_interrupt_frame_at(c,0x12da99);c->P|=4;cpu_p_to_mirrors(c);}else{ram[c->S+1]=0x98;ram[c->S+2]=0xda;ram[c->S+3]=0x12;}uint32_t unused;
 assert(!LagoonInterrupt66_Process(rom,sizeof rom,c,c,7,7,irq?0x87b4:0x8120,irq?0x87b4:0,irq?c->S:0,&unused));
 if(irq){Interp816*i=interp816_init(NULL,rd,wr);from(i,c,0x87b4);while(i->pc!=0x87c3)opcode(i,c);interp816_free(i);if(pc!=0x87c3)assert(LagoonInterrupt66_Process(rom,sizeof rom,c,c,7,7,0x87c3,0x87b4,nmi66.s,&unused));}
 c->cycles=c->master_cycles=c->coprocessor_master_cycles=0;c->P=(c->P&0x3c)|((seed&1)?1:0)|((seed&2)?2:0)|((seed&4)?64:0)|((seed&8)?128:0);cpu_p_to_mirrors(c);release_at=seed*17;mem.io[0x4210]=seed*11;mem.io[0x4212]=seed*7;for(unsigned j=0x4218;j<0x4220;j++)mem.io[j]=j*13+seed;mem.writes=0;
}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonInterrupt66_Source(rom,sizeof rom));
for(unsigned root=0;root<2;root++){assert(mapped66(root?0x8120:0x87b4));for(unsigned j=0;j<sizeof spans66/sizeof*spans66;j++)for(unsigned k=0;k<spans66[j].n;k++){bad_mapping=spans66[j].lo+k;assert(!mapped66(root?0x8120:0x87b4));mapping_cases++;}bad_mapping=~0u;}
for(unsigned op=0;op<sizeof ops66/sizeof*ops66;op++){assert(mapped66(ops66[op][0]));for(unsigned k=0;k<ops66[op][1];k++){bad_mapping=ops66[op][0]+k;assert(!mapped66(ops66[op][0]));mapping_cases++;}bad_mapping=~0u;}assert(!mapped66(0x8925));
for(unsigned seed=0;seed<256;seed++)for(unsigned op=0;op<sizeof ops66/sizeof*ops66;op++){
 unsigned pc=ops66[op][0],irq=pc>=0x87b4;CpuState c;setup66(&c,seed,irq,pc);CpuState ref=c;before=mem;deadline=5000;uint32_t landing=0;assert(LagoonInterrupt66_Process(rom,sizeof rom,&c,&c,7,7,pc,irq?0x87b4:0,irq?nmi66.s:0,&landing));cpu_mirrors_to_p(&c);actual=mem;mem=before;Interp816*i=interp816_init(NULL,rd,wr);from(i,&ref,pc);opcode(i,&ref);equal(&c,&ref,i,landing);cases++;interp816_free(i);
}
/* Source hardware polls to real completion; NMI's actual PLP/PLY/PLX/PLA/RTI
 * consume the genuine saved prologue and four-byte architectural frame. */
for(unsigned seed=0;seed<32;seed++)for(unsigned irq=0;irq<2;irq++){
 CpuState c;setup66(&c,seed,irq,irq?0x88d3:0x8148);before=mem;CpuState ref=c;unsigned pc=irq?0x88d3:0x8148,target=irq?0x12da99:0x8152;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,pc);deadline=128;unsigned steps=0;
 while(pc!=target&&steps++<10000){if(c.master_cycles>=deadline)deadline=c.master_cycles+128;uint32_t landing;if(LagoonInterrupt66_Process(rom,sizeof rom,&c,&c,7,7,pc,irq?0x87b4:0,irq?nmi66.s:0,&landing)){pc=landing;from(i,&c,pc);}else{opcode(i,&c);pc=((unsigned)i->k<<16)|i->pc;}}
 assert(pc==target);actual=mem;mem=before;from(i,&ref,irq?0x88d3:0x8148);steps=0;while((((unsigned)i->k<<16)|i->pc)!=target&&steps++<10000)opcode(i,&ref);equal(&c,&ref,i,target);cases++;interp816_free(i);
}
for(unsigned fault=0;fault<14;fault++){CpuState c;setup66(&c,0,1,0x88d3);unsigned owner=0x87b4,stack=nmi66.s;switch(fault){case 0:c.P|=8;cpu_p_to_mirrors(&c);break;case 1:c.D=1;break;case 2:c.S--;break;case 3:ram[stack+1]^=1;break;case 4:c.DB=3;break;case 5:c.P|=16;cpu_p_to_mirrors(&c);break;case 6:c.emulation=1;break;case 7:owner++;break;case 8:stack++;break;case 9:rom[0x7b4]^=1;break;case 10:nmi66.gen++;break;case 11:nmi66.cpu=0;break;case 12:ram[c.S+1]^=1;break;case 13:nmi66.phase=1;break;}CpuState old=c;before=mem;uint32_t landing;assert(!LagoonInterrupt66_Process(rom,sizeof rom,&c,&c,7,7,0x88d3,owner,stack,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));if(fault==9)rom[0x7b4]^=1;refusals++;}
printf("PASS %u exact owner/opcode mapping cases\n",mapping_cases);
printf("PASS interrupt66: %u NEW original opcode and real hardware-poll/RTI cases; %u immutable owner/source/mode refusals\n",cases,refusals);return 0;}
