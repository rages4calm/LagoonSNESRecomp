#define LAGOON_MENU_INPUT67_TEST 1
#include "../src/lagoon_native_menu_input67.c"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
static uint8_t rom[0x100000];
typedef struct {uint8_t ram[0x20000],sram[8192],io[65536],vram[65536];unsigned va;uint64_t writes;} Memory;
static Memory mem,before,actual;
static uint8_t*ram=mem.ram;
static uint64_t deadline,bus_extra,read_master,release_at;static uint8_t oracle_open_bus;static unsigned cases,refusals,mapping_cases;static unsigned bad_mapping=~0u;
static const CpuWramReadContext*active_context;
const CpuWramReadContext*cpu_swap_wram_read_context(const CpuWramReadContext*c){const CpuWramReadContext*old=active_context;active_context=c;return old;}
static unsigned filter_variant;
int cpu_aot_rom_mapping_matches(uint32_t pc,uint32_t offset,unsigned n){assert(n&&n<=4&&offset==menu_offset67(pc));return !(bad_mapping>=pc&&bad_mapping<pc+n);}
uint32_t cpu_region_speed(uint32_t address){return (address&65535)>=0x2100&&(address&65535)<0x2200&&(address>>16)<0x40?6:8;}
void cpu_instruction_timing_commit(CpuState*c){c->coprocessor_master_cycles=c->master_cycles;}
bool interp_bridge_commit_source_opcode(CpuState*c,uint64_t prior,bool wrote){assert(!wrote&&prior<=c->master_cycles);cpu_instruction_timing_commit(c);return true;}
bool interp_bridge_source_opcode_deadline_reached(const CpuState*c){return deadline&&c->master_cycles>=deadline;}
int interp816_opcode_hook(uint32_t address){(void)address;return 0;}
static uint8_t value(uint32_t address){if((address>>16)<3&&((address&65535)==0x4210||(address&65535)==0x4212)){unsigned a=address&65535;uint8_t v=mem.io[a];if(a==0x4210){v=(v&127)|(read_master>=release_at?128:0);mem.io[a]=v&127;}else v=(v&254)|(read_master<release_at);mem.writes=(mem.writes^(address*257u+v))*UINT64_C(1099511628211);return v;}int32_t o=cpu_wram_offset(address>>16,address);if(o>=0)return ram[o];if((address>>16)==0x70)return mem.sram[address&8191];if((address&65535)>=32768)return rom[((address>>16)&31)*32768+(address&32767)];return mem.io[address&65535];}
static void store(uint32_t address,uint8_t v){int32_t o=cpu_wram_offset(address>>16,address);mem.writes=(mem.writes^(address*257u+v))*UINT64_C(1099511628211);if(o>=0){if(filter_variant&&o==0x520&&v==10&&ram[0x500]==0xc0&&ram[0x52c]==1&&ram[0x50b]==255&&ram[0x522]==0&&ram[0x524]==0&&ram[0x526]==0)v=18;if(filter_variant&&o==0x526&&v==5&&ram[0x520]==18&&ram[0x524]==10&&ram[0x522]==3)v=11;ram[o]=v;}else if((address>>16)==0x70)mem.sram[address&8191]=v;else {mem.io[address&65535]=v;switch(address&65535){case 0x2116:mem.va=(mem.va&0xff00)|v;break;case 0x2117:mem.va=(mem.va&255)|(v<<8);break;case 0x2118:case 0x2119:{unsigned high=(address&1);mem.vram[((mem.va*2)+high)&65535]=v;if(high==!!(mem.io[0x2115]&128))mem.va=(mem.va+1)&65535;break;}}}}
uint8_t cpu_read8(CpuState*c,uint8_t b,uint16_t a){read_master=c->master_cycles;return c->open_bus=value((uint32_t)b<<16|a);}
void cpu_write8(CpuState*c,uint8_t b,uint16_t a,uint8_t v){c->open_bus=v;store((uint32_t)b<<16|a,v);}
uint16_t cpu_read16(CpuState*c,uint8_t b,uint16_t a){unsigned v=cpu_read8(c,b,a);return v|cpu_read8(c,b,(uint16_t)(a+1))<<8;}
void cpu_write16(CpuState*c,uint8_t b,uint16_t a,uint16_t v){cpu_write8(c,b,a,v);cpu_write8(c,b,(uint16_t)(a+1),v>>8);}
static uint8_t rd(void*ctx,uint32_t a){(void)ctx;bus_extra+=cpu_region_speed(a)-6;return oracle_open_bus=value(a);}
static void wr(void*ctx,uint32_t a,uint8_t v){(void)ctx;bus_extra+=cpu_region_speed(a)-6;oracle_open_bus=v;store(a,v);}
static void from(Interp816*i,const CpuState*c,uint32_t pc){i->a=c->A;i->x=c->X;i->y=c->Y;i->sp=c->S;i->dp=c->D;i->db=c->DB;i->k=c->PB;i->pc=pc;i->e=c->emulation;interp816_setFlags(i,c->P);}
static void opcode(Interp816*i,CpuState*c){bus_extra=0;read_master=c->master_cycles;unsigned n=interp816_runOpcode(i);c->cycles+=n;c->master_cycles+=n*6+bus_extra;c->coprocessor_master_cycles=c->master_cycles;c->A=i->a;c->X=i->x;c->Y=i->y;c->S=i->sp;c->D=i->dp;c->DB=i->db;c->PB=i->k;c->P=interp816_getFlags(i);cpu_p_to_mirrors(c);c->open_bus=oracle_open_bus;}


bool interp_bridge_source_read8(CpuState*c,uint32_t address,uint8_t*out){assert(active_context&&active_context->instruction_pc>=0x02b2f6&&active_context->instruction_pc<=0x02b362&&active_context->stack==c->S&&active_context->y==c->Y&&active_context->interpreter==1);*out=cpu_read8(c,address>>16,address);return true;}
bool interp_bridge_source_read16(CpuState*c,uint32_t l,uint32_t h,uint16_t*out){unsigned v=cpu_read8(c,l>>16,l);*out=v|cpu_read8(c,h>>16,h)<<8;return true;}
bool interp_bridge_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_source_write16(CpuState*c,uint32_t l,uint32_t h,uint16_t v){cpu_write8(c,l>>16,l,v);cpu_write8(c,h>>16,h,v>>8);return true;}



bool interp_bridge_interrupt_source_read8(CpuState*c,uint32_t a,uint8_t*out){*out=cpu_read8(c,a>>16,a);return true;}
bool interp_bridge_interrupt_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_commit_interrupt_source_opcode(CpuState*c,uint64_t prior,bool wrote){return interp_bridge_commit_source_opcode(c,prior,wrote);}
static void equal(const CpuState*a,const CpuState*b,const Interp816*i,unsigned pc){
 if(memcmp(&mem,&actual,sizeof mem)||i->pc!=(pc&65535)||i->k!=(pc>>16)||a->A!=b->A||a->X!=b->X||a->Y!=b->Y||a->S!=b->S||a->D!=b->D||a->DB!=b->DB||a->P!=b->P||a->open_bus!=b->open_bus||a->cycles!=b->cycles||a->master_cycles!=b->master_cycles||a->coprocessor_master_cycles!=b->coprocessor_master_cycles){fprintf(stderr,"FAIL case%u pc=%06x/%02x%04x A=%04x/%04x X=%04x/%04x Y=%04x/%04x S=%04x/%04x P=%02x/%02x bus=%02x/%02x cyc=%llu/%llu clocks=%llu/%llu memory=%d writes=%llu/%llu\n",cases,pc,i->k,i->pc,a->A,b->A,a->X,b->X,a->Y,b->Y,a->S,b->S,a->P,b->P,a->open_bus,b->open_bus,(unsigned long long)a->cycles,(unsigned long long)b->cycles,(unsigned long long)a->master_cycles,(unsigned long long)b->master_cycles,memcmp(&mem,&actual,sizeof mem),(unsigned long long)mem.writes,(unsigned long long)actual.writes);exit(2);}}

static int menu_interested67(unsigned pc){if(pc==0x02b361)return 1;for(unsigned j=0;j<sizeof menu_ops67/sizeof*menu_ops67;j++)if(menu_ops67[j][0]==pc)return 1;for(unsigned j=0;j<sizeof menu_calls67/sizeof*menu_calls67;j++)if(menu_calls67[j][0]==pc)return 1;return 0;}
static void menu_setup67(CpuState*c,unsigned seed){
 memset(&menu67,0,sizeof menu67);memset(&mem,0,sizeof mem);for(unsigned j=0;j<0x20000;j++)ram[j]=j*73+seed*17;memset(c,0,sizeof*c);c->ram=ram;c->S=0x1ff;c->DB=1;c->PB=1;c->A=1;c->X=seed*2017;c->Y=seed*97;c->P=0x24;cpu_p_to_mirrors(c);deadline=0;filter_variant=0;
 /* Original main018096 JSL80B9, real PHB/PHYP/selector and nested80E0
  * JSL80E6/JML produce the root's physical seven-byte dispatcher chain. */
 Interp816*i=interp816_init(NULL,rd,wr);from(i,c,0x018096);unsigned steps=0;while((((unsigned)i->k<<16)|i->pc)!=0x02b2f6&&steps++<80)opcode(i,c);assert(i->k==2&&i->pc==0xb2f6&&c->S==0x1f8&&c->DB==2);interp816_free(i);assert(arm_menu67(c,7));
}
static void menu_one67(CpuState*c,unsigned pc){CpuState ref=*c;before=mem;uint32_t landing;static const CpuWramReadContext sentinel={.instruction_pc=0x123456};active_context=&sentinel;assert(LagoonMenuInput67_Process(rom,sizeof rom,c,c,7,7,pc,&landing));assert(active_context==&sentinel);active_context=0;cpu_mirrors_to_p(c);actual=mem;mem=before;Interp816*i=interp816_init(NULL,rd,wr);from(i,&ref,pc);opcode(i,&ref);equal(c,&ref,i,landing);cases++;interp816_free(i);mem=actual;}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonMenuInput67_Source(rom,sizeof rom));
/* Full source/dispatcher/table mapping at real entry, exact current op/call. */
assert(menu_mapped67(0x02b2f6));const unsigned spans[][2]={{0x02b2f6,0x7b},{0x0080b3,0x36},{0x028002,2}};for(unsigned j=0;j<3;j++)for(unsigned k=0;k<spans[j][1];k++){bad_mapping=spans[j][0]+k;assert(!menu_mapped67(0x02b2f6));mapping_cases++;}bad_mapping=~0u;
for(unsigned j=0;j<sizeof menu_ops67/sizeof*menu_ops67;j++){unsigned pc=menu_ops67[j][0];assert(menu_mapped67(pc));for(unsigned k=0;k<menu_ops67[j][1];k++){bad_mapping=pc+k;assert(!menu_mapped67(pc));mapping_cases++;}bad_mapping=~0u;}
for(unsigned j=0;j<sizeof menu_calls67/sizeof*menu_calls67;j++)for(unsigned k=0;k<menu_calls67[j][1];k++){bad_mapping=menu_calls67[j][0]+k;assert(!menu_mapped67(menu_calls67[j][0]));mapping_cases++;}bad_mapping=~0u;for(unsigned pc=0x02b369;pc<0x02b371;pc++)assert(!menu_mapped67(pc));
/* Every byte input and selector value, all scalar opcodes, full flags and
 * high A retention. Each witness starts with the actual source dispatcher. */
for(unsigned seed=0;seed<256;seed++)for(unsigned op=0;op<sizeof menu_ops67/sizeof*menu_ops67;op++){
 CpuState c;menu_setup67(&c,seed);unsigned pc=menu_ops67[op][0];menu67.next=pc;c.A=(seed*257)^0xa500;c.P=0x24|((seed&1)?1:0)|((seed&2)?2:0)|((seed&4)?64:0)|((seed&8)?128:0);cpu_p_to_mirrors(&c);ram[0x53f]=seed;ram[0x501]=seed;ram[0x22e]=seed;ram[0x22f]=seed^0x5a;ram[0x105a]=seed;menu_one67(&c,pc);
}
/* Whole eligibility parent and original nested RTL/PLB/RTL return chain;
 * permitted entry path stops at realF1A5 call, never invents its result. */
for(unsigned seed=0;seed<256;seed++){
 CpuState c;menu_setup67(&c,seed);ram[0x53f]=seed;ram[0x501]=seed;before=mem;CpuState ref=c;unsigned target=(seed&0x30)&&!(seed&3)?0x02b304:0x01809a,pc=0x02b2f6;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,pc);unsigned steps=0;while(pc!=target&&steps++<100){uint32_t landing;if(menu_interested67(pc)&&LagoonMenuInput67_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing)){pc=landing;from(i,&c,pc);}else{opcode(i,&c);pc=((unsigned)i->k<<16)|i->pc;}}assert(pc==target);actual=mem;mem=before;from(i,&ref,0x02b2f6);steps=0;while((((unsigned)i->k<<16)|i->pc)!=target&&steps++<100)opcode(i,&ref);equal(&c,&ref,i,pc);cases++;interp816_free(i);
}
/* Original895D input edge sampler runs in full with its real nested JSRs,
 * PHX/PLX and RTL. Scalar input loops stop at genuine sound/cancel/dispatch
 * callsites; selection changes and flags are compared before those calls. */
for(unsigned input=0;input<6;input++)for(unsigned selector=0;selector<256;selector++){
 CpuState c;menu_setup67(&c,selector);for(unsigned j=0x21e;j<0x236;j++)ram[j]=0;ram[0x21e]=input==2?0x80:input==3?0x10:0;ram[0x21f]=input==0?4:input==1?8:input==4?12:0;ram[0x105a]=selector;menu67.next=0x02b316;before=mem;CpuState ref=c;unsigned pc=0x02b316,target=input==2?0x02b365:input==3?0x02b349:input==5?0x02b31a:0x02b33b;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,pc);unsigned steps=0;deadline=c.master_cycles+100;
 while(pc!=target&&steps++<1000){if(c.master_cycles>=deadline)deadline=c.master_cycles+100;uint32_t landing;if(menu_interested67(pc)&&LagoonMenuInput67_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing)){pc=landing;from(i,&c,pc);}else{opcode(i,&c);pc=((unsigned)i->k<<16)|i->pc;}}assert(pc==target&&c.S==0x1f8);actual=mem;mem=before;from(i,&ref,0x02b316);steps=0;while((((unsigned)i->k<<16)|i->pc)!=target&&steps++<1000)opcode(i,&ref);equal(&c,&ref,i,pc);cases++;interp816_free(i);
}
for(unsigned fault=0;fault<21;fault++){
 CpuState c;menu_setup67(&c,0);menu67.next=0x02b31a;uint64_t gen=7,frame=7;const CpuState*primary=&c;switch(fault){case 0:ram[menu67.s+1]^=1;break;case 1:ram[menu67.s+4]^=1;break;case 2:ram[menu67.s+7]^=1;break;case 3:c.S--;break;case 4:c.DB=1;break;case 5:c.PB=1;break;case 6:c.D=1;break;case 7:c.emulation=1;break;case 8:c.P|=16;cpu_p_to_mirrors(&c);break;case 9:c.P&=~32;cpu_p_to_mirrors(&c);break;case 10:c.P|=8;cpu_p_to_mirrors(&c);break;case 11:c.P&=~4;cpu_p_to_mirrors(&c);break;case 12:gen++;break;case 13:frame++;break;case 14:primary=0;break;case 15:menu67.gen++;break;case 16:menu67.ram=0;break;case 17:menu67.next++;break;case 18:deadline=c.master_cycles;break;case 19:rom[0x13369]^=1;break;case 20:rom[0xe0]^=1;break;}CpuState old=c;before=mem;uint32_t landing;assert(!LagoonMenuInput67_Process(rom,sizeof rom,&c,primary,gen,frame,0x02b31a,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));if(fault==19)rom[0x13369]^=1;if(fault==20)rom[0xe0]^=1;refusals++;
}
printf("PASS menu input67: %u original opcode/real-dispatch/eligibility/real895D input routes; %u exact mapping refusals; %u immutable ownership/deadline refusals\n",cases,mapping_cases,refusals);return 0;}
