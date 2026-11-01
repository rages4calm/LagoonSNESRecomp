#define LAGOON_PLAYER_INTENT68_TEST 1
#include "../src/lagoon_native_player_intent68.c"
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
int cpu_aot_rom_mapping_matches(uint32_t pc,uint32_t offset,unsigned n){assert(n&&n<=4&&offset==intent_offset68(pc));return !(bad_mapping>=pc&&bad_mapping<pc+n);}
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


bool interp_bridge_source_read8(CpuState*c,uint32_t address,uint8_t*out){assert(active_context&&active_context->instruction_pc>=0x0280a0&&active_context->instruction_pc<=0x028125&&active_context->stack==c->S&&active_context->y==c->Y&&active_context->interpreter==1);*out=cpu_read8(c,address>>16,address);return true;}
bool interp_bridge_source_read16(CpuState*c,uint32_t l,uint32_t h,uint16_t*out){unsigned v=cpu_read8(c,l>>16,l);*out=v|cpu_read8(c,h>>16,h)<<8;return true;}
bool interp_bridge_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_source_write16(CpuState*c,uint32_t l,uint32_t h,uint16_t v){cpu_write8(c,l>>16,l,v);cpu_write8(c,h>>16,h,v>>8);return true;}



bool interp_bridge_interrupt_source_read8(CpuState*c,uint32_t a,uint8_t*out){*out=cpu_read8(c,a>>16,a);return true;}
bool interp_bridge_interrupt_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_commit_interrupt_source_opcode(CpuState*c,uint64_t prior,bool wrote){return interp_bridge_commit_source_opcode(c,prior,wrote);}
static void equal(const CpuState*a,const CpuState*b,const Interp816*i,unsigned pc){
 if(memcmp(&mem,&actual,sizeof mem)||i->pc!=(pc&65535)||i->k!=(pc>>16)||a->A!=b->A||a->X!=b->X||a->Y!=b->Y||a->S!=b->S||a->D!=b->D||a->DB!=b->DB||a->P!=b->P||a->open_bus!=b->open_bus||a->cycles!=b->cycles||a->master_cycles!=b->master_cycles||a->coprocessor_master_cycles!=b->coprocessor_master_cycles){fprintf(stderr,"FAIL case%u pc=%06x/%02x%04x A=%04x/%04x X=%04x/%04x Y=%04x/%04x S=%04x/%04x P=%02x/%02x bus=%02x/%02x cyc=%llu/%llu clocks=%llu/%llu memory=%d writes=%llu/%llu\n",cases,pc,i->k,i->pc,a->A,b->A,a->X,b->X,a->Y,b->Y,a->S,b->S,a->P,b->P,a->open_bus,b->open_bus,(unsigned long long)a->cycles,(unsigned long long)b->cycles,(unsigned long long)a->master_cycles,(unsigned long long)b->master_cycles,memcmp(&mem,&actual,sizeof mem),(unsigned long long)mem.writes,(unsigned long long)actual.writes);exit(2);}}

static int intent_interested68(unsigned pc){if(pc==0x0280d9||pc==0x02811b||pc==0x028128)return 1;for(unsigned j=0;j<sizeof intent_ops68/sizeof*intent_ops68;j++)if(intent_ops68[j][0]==pc)return 1;return 0;}
static unsigned intent_test_selector68=17,intent_test_slot68;
static void intent_setup68(CpuState*c,unsigned seed,unsigned observer,unsigned defer){
 memset(&intent_owner68,0,sizeof intent_owner68);memset(&intent_pending68,0,sizeof intent_pending68);memset(&mem,0,sizeof mem);for(unsigned j=0;j<0x20000;j++)ram[j]=j*73+seed*17;memset(c,0,sizeof*c);c->ram=ram;c->S=0x1ff;c->DB=1;c->PB=1;c->A=intent_test_selector68;c->X=intent_test_slot68;c->Y=seed*97;c->P=0x24;cpu_p_to_mirrors(c);deadline=0;filter_variant=0;
 Interp816*i=interp816_init(NULL,rd,wr);from(i,c,0x018096);unsigned root=intent_test_selector68==17?0x0280a0:0x0280ea;unsigned steps=0;while((((unsigned)i->k<<16)|i->pc)!=root&&steps++<80){unsigned pc=(unsigned)i->k<<16|i->pc;uint32_t landing;if(pc==0x0080e0&&!observer)assert(!LagoonPlayerIntent68_Process(rom,sizeof rom,c,c,7,7,pc,&landing));if(pc==0x0080e6&&observer){c->host_return_valid=3;assert(intent_observe68(rom,sizeof rom,c,c,7,7,c->S,3,0x0080e4));}opcode(i,c);}assert(i->k==2&&i->pc==(root&65535)&&c->S==0x1f8&&c->DB==2);if(!defer)assert(intent_take68(c,7,root));interp816_free(i);
}
static void intent_one68(CpuState*c,unsigned pc){CpuState ref=*c;before=mem;uint32_t landing;static const CpuWramReadContext sentinel={.instruction_pc=0x123456};active_context=&sentinel;assert(LagoonPlayerIntent68_Process(rom,sizeof rom,c,c,7,7,pc,&landing));assert(active_context==&sentinel);active_context=0;cpu_mirrors_to_p(c);actual=mem;mem=before;Interp816*i=interp816_init(NULL,rd,wr);from(i,&ref,pc);opcode(i,&ref);equal(c,&ref,i,landing);cases++;interp816_free(i);mem=actual;}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonPlayerIntent68_Source(rom,sizeof rom));
const unsigned spans[][2]={{0x0080b3,0x36},{0x028022,2},{0x028026,2},{0x0280a0,0x18c}};assert(intent_mapped68(0x0280a0));for(unsigned j=0;j<4;j++)for(unsigned k=0;k<spans[j][1];k++){bad_mapping=spans[j][0]+k;assert(!intent_mapped68(0x0280a0));mapping_cases++;}bad_mapping=~0u;
for(unsigned j=0;j<sizeof intent_ops68/sizeof*intent_ops68;j++)for(unsigned k=0;k<intent_ops68[j][1];k++){bad_mapping=intent_ops68[j][0]+k;assert(!intent_mapped68(intent_ops68[j][0]));mapping_cases++;}bad_mapping=~0u;for(unsigned pc=0x0280da;pc<0x0280ea;pc++)assert(!intent_mapped68(pc));
/* All byte patterns/high accumulator, widths/flags, ROM table index and
 * complete state/clock/context comparison against actual original opcodes. */
for(unsigned seed=0;seed<256;seed++)for(unsigned op=0;op<sizeof intent_ops68/sizeof*intent_ops68;op++){
 unsigned pc=intent_ops68[op][0];intent_test_selector68=pc<0x0280ea?17:19;CpuState c;intent_setup68(&c,seed,seed&1,pc==0x0280a0||pc==0x0280ea);intent_owner68.next=pc;c.A=(seed*257)^0xa500;c.Y=seed*257;c.P=0x24|((seed&1)?1:0)|((seed&2)?2:0)|((seed&4)?64:0)|((seed&8)?128:0);cpu_p_to_mirrors(&c);ram[0x501]=seed;ram[0x53f]=seed^0x5a;ram[0x50a]=seed;intent_one68(&c,pc);
}
intent_test_selector68=17;
/* Whole player routine through actual handlerRTL / savedDB PLB / outerRTL.
 * No fabricated return, stack frame or result; player inputs cover ordinary,
 * locked-facing, invalid/diagonal and zero-intent branches. */
for(unsigned observer=0;observer<2;observer++)for(unsigned status=0;status<256;status++)for(unsigned input=0;input<16;input++){
 CpuState c;intent_setup68(&c,status,observer,1);ram[0x501]=status;ram[0x53f]=input;ram[0x50a]=status&7;before=mem;CpuState ref=c;unsigned pc=0x0280a0;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,pc);unsigned steps=0;
 while(pc!=0x01809a&&steps++<100){uint32_t landing;if(intent_interested68(pc)&&LagoonPlayerIntent68_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing)){pc=landing;from(i,&c,pc);}else{opcode(i,&c);pc=(unsigned)i->k<<16|i->pc;}}assert(pc==0x01809a&&c.S==0x1ff&&c.DB==1);actual=mem;mem=before;from(i,&ref,0x0280a0);steps=0;while((((unsigned)i->k<<16)|i->pc)!=0x01809a&&steps++<100)opcode(i,&ref);equal(&c,&ref,i,pc);cases++;interp816_free(i);
}
for(unsigned fault=0;fault<28;fault++){
 CpuState c;intent_setup68(&c,0,0,0);intent_owner68.next=0x0280ab;uint64_t gen=7,frame=7;const CpuState*primary=&c;switch(fault){case 0:ram[intent_owner68.s+1]^=1;break;case 1:ram[intent_owner68.s+4]^=1;break;case 2:ram[intent_owner68.s+7]^=1;break;case 3:c.S--;break;case 4:c.DB=1;break;case 5:c.PB=1;break;case 6:c.D=1;break;case 7:c.emulation=1;break;case 8:c.P|=16;cpu_p_to_mirrors(&c);break;case 9:c.P&=~32;cpu_p_to_mirrors(&c);break;case 10:c.P|=8;cpu_p_to_mirrors(&c);break;case 11:c.P&=~4;cpu_p_to_mirrors(&c);break;case 12:gen++;break;case 13:frame++;break;case 14:primary=0;break;case 15:intent_owner68.gen++;break;case 16:intent_owner68.ram=0;break;case 17:intent_owner68.next++;break;case 18:deadline=c.master_cycles;break;case 19:rom[0x100da]^=1;break;case 20:rom[0xe0]^=1;break;case 21:intent_owner68.armed=0;break;case 22:rom[0x10022]^=1;break;case 23:intent_owner68.cpu=0;break;case 24:c.X=0x40;break;case 25:c.X=1;break;case 26:c.ram=0;break;case 27:c.X=0x840;break;}CpuState old=c;before=mem;uint32_t landing;assert(!LagoonPlayerIntent68_Process(rom,sizeof rom,&c,primary,gen,frame,0x0280ab,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));if(fault==19)rom[0x100da]^=1;if(fault==20)rom[0xe0]^=1;if(fault==22)rom[0x10022]^=1;refusals++;
}
for(unsigned fault=0;fault<9;fault++){
 CpuState c;intent_setup68(&c,0,1,1);uint32_t landing;switch(fault){case 0:intent_retain68(&c,7,0x0280a0);break;case 1:ram[intent_pending68.s+4]^=1;break;case 2:bad_mapping=0x0080e1;break;case 3:intent_retain68(&c,7,0x012345);break;case 4:{unsigned pb=c.PB,v=ram[0x10];c.PB=0;ram[0x10]=0;assert(!LagoonPlayerIntent68_Process(rom,sizeof rom,&c,&c,7,7,0x0080e0,&landing));c.PB=pb;ram[0x10]=v;break;}case 5:memset(&intent_pending68,0,sizeof intent_pending68);break;case 6:deadline=c.master_cycles;break;case 7:intent_retain68(&c,8,0x0280a0);break;case 8:c.X=0x40;break;}
 if(!fault){intent_one68(&c,0x0280a0);continue;}CpuState old=c;before=mem;assert(!LagoonPlayerIntent68_Process(rom,sizeof rom,&c,&c,7,7,0x0280a0,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));if(fault==6){deadline=0;intent_one68(&c,0x0280a0);}else refusals++;bad_mapping=~0u;
}
/* Pure native entry refuses forged metadata/mode/mapping before guest changes. */
for(unsigned fault=0;fault<25;fault++){
 CpuState c;intent_setup68(&c,0,1,1);c.PB=0;intent_pending68.armed=0;unsigned s=c.S,h=3,host=0x0080e4;uint64_t gen=7,frame=7;const CpuState*primary=&c;
 switch(fault){case 0:ram[s+1]^=1;break;case 1:ram[s+4]=3;break;case 2:h=2;break;case 3:c.host_return_valid=2;break;case 4:host++;break;case 5:s++;break;case 6:gen++;break;case 7:frame++;break;case 8:primary=0;break;case 9:ram[0x10]=0;break;case 10:ram[0x13]=0;break;case 11:c.DB=1;break;case 12:c.PB=2;break;case 13:c.emulation=1;break;case 14:c.D=1;break;case 15:c.P&=~32;cpu_p_to_mirrors(&c);break;case 16:c.P|=16;cpu_p_to_mirrors(&c);break;case 17:c.P|=8;cpu_p_to_mirrors(&c);break;case 18:c.P&=~4;cpu_p_to_mirrors(&c);break;case 19:rom[0x100da]^=1;break;case 20:bad_mapping=0x0080e0;break;case 21:bad_mapping=0x028022;break;case 22:bad_mapping=0x0280a1;break;case 23:c.ram=0;break;case 24:c.X=0x41;break;}
 CpuState old=c;before=mem;assert(!intent_observe68(rom,sizeof rom,&c,primary,gen,frame,s,h,host));assert(!intent_pending68.armed&&!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));if(fault==19)rom[0x100da]^=1;bad_mapping=~0u;refusals++;
}
/* Both paired leaves across every authored64-byte actor slot. The ordinary
 * enemy optimization is outside this oracle and remains first in production;
 * these tests compare the scalar fallback directly to original instructions.
 * Status/locked-facing, valid/negative direction and speed-tier overflow are
 * exercised without assuming an actor's role. Hidden A has all256 opcode
 * seeds above; real dispatcher routes retain its original AND00FF result. */
for(unsigned slot=0;slot<33;slot++)for(unsigned selector=0;selector<2;selector++)for(unsigned seed=0;seed<32;seed++){
 intent_test_slot68=slot*64;intent_test_selector68=selector?19:17;CpuState c;intent_setup68(&c,seed,seed&1,1);unsigned a=0x500+c.X;ram[a+1]=seed*29;ram[a+0x3f]=(seed*7)&15;ram[a+10]=seed&7;ram[a+11]=(seed&3)?seed&7:255;ram[a+26]=seed*17;before=mem;CpuState ref=c;unsigned pc=selector?0x0280ea:0x0280a0;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,pc);unsigned steps=0;
 while(pc!=0x01809a&&steps++<100){uint32_t landing;if(intent_interested68(pc)&&LagoonPlayerIntent68_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing)){pc=landing;from(i,&c,pc);}else{opcode(i,&c);pc=(unsigned)i->k<<16|i->pc;}}assert(pc==0x01809a&&c.S==0x1ff&&c.DB==1&&c.X==slot*64);actual=mem;mem=before;from(i,&ref,selector?0x0280ea:0x0280a0);steps=0;while((((unsigned)i->k<<16)|i->pc)!=0x01809a&&steps++<100)opcode(i,&ref);equal(&c,&ref,i,pc);cases++;interp816_free(i);
}
intent_test_slot68=0;intent_test_selector68=17;
printf("PASS actor intent68: %u original opcode/real-dispatch/paired wholeRTL routes; %u exact mapping refusals; %u immutable ownership/deadline refusals\n",cases,mapping_cases,refusals);return 0;}
