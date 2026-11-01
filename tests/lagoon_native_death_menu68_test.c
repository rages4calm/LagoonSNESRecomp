#define LAGOON_DEATH_MENU68_TEST 1
#include "../src/lagoon_native_death_menu68.c"
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
int cpu_aot_rom_mapping_matches(uint32_t pc,uint32_t offset,unsigned n){assert(n&&n<=4&&offset==death_offset68(pc));return !(bad_mapping>=pc&&bad_mapping<pc+n);}
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


bool interp_bridge_source_read8(CpuState*c,uint32_t address,uint8_t*out){assert(active_context&&active_context->instruction_pc>=0x02d54e&&active_context->instruction_pc<=0x02d5db&&active_context->stack==c->S&&active_context->y==c->Y&&active_context->interpreter==1);*out=cpu_read8(c,address>>16,address);return true;}
bool interp_bridge_source_read16(CpuState*c,uint32_t l,uint32_t h,uint16_t*out){unsigned v=cpu_read8(c,l>>16,l);*out=v|cpu_read8(c,h>>16,h)<<8;return true;}
bool interp_bridge_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_source_write16(CpuState*c,uint32_t l,uint32_t h,uint16_t v){cpu_write8(c,l>>16,l,v);cpu_write8(c,h>>16,h,v>>8);return true;}



bool interp_bridge_interrupt_source_read8(CpuState*c,uint32_t a,uint8_t*out){*out=cpu_read8(c,a>>16,a);return true;}
bool interp_bridge_interrupt_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_commit_interrupt_source_opcode(CpuState*c,uint64_t prior,bool wrote){return interp_bridge_commit_source_opcode(c,prior,wrote);}
static void equal(const CpuState*a,const CpuState*b,const Interp816*i,unsigned pc){
 if(memcmp(&mem,&actual,sizeof mem)||i->pc!=(pc&65535)||i->k!=(pc>>16)||a->A!=b->A||a->X!=b->X||a->Y!=b->Y||a->S!=b->S||a->D!=b->D||a->DB!=b->DB||a->P!=b->P||a->open_bus!=b->open_bus||a->cycles!=b->cycles||a->master_cycles!=b->master_cycles||a->coprocessor_master_cycles!=b->coprocessor_master_cycles){fprintf(stderr,"FAIL case%u pc=%06x/%02x%04x A=%04x/%04x X=%04x/%04x Y=%04x/%04x S=%04x/%04x P=%02x/%02x bus=%02x/%02x cyc=%llu/%llu clocks=%llu/%llu memory=%d writes=%llu/%llu\n",cases,pc,i->k,i->pc,a->A,b->A,a->X,b->X,a->Y,b->Y,a->S,b->S,a->P,b->P,a->open_bus,b->open_bus,(unsigned long long)a->cycles,(unsigned long long)b->cycles,(unsigned long long)a->master_cycles,(unsigned long long)b->master_cycles,memcmp(&mem,&actual,sizeof mem),(unsigned long long)mem.writes,(unsigned long long)actual.writes);exit(2);}}

static int death_interested68(unsigned pc){if(pc==0x0080e0||pc==0x02d5c2||pc==0x02d5dd)return 1;for(unsigned j=0;j<sizeof death_ops68/sizeof*death_ops68;j++)if(death_ops68[j][0]==pc)return 1;for(unsigned j=0;j<sizeof death_calls68/sizeof*death_calls68;j++)if(death_calls68[j][0]==pc)return 1;return 0;}
static unsigned use_native_observer68;
static void death_setup68(CpuState*c,unsigned seed,unsigned child,unsigned defer){
 memset(death_owner68,0,sizeof death_owner68);memset(death_pending68,0,sizeof death_pending68);memset(&mem,0,sizeof mem);for(unsigned j=0;j<0x20000;j++)ram[j]=j*73+seed*17;memset(c,0,sizeof*c);c->ram=ram;c->S=0x1ff;c->DB=1;c->PB=1;c->A=14;c->X=seed*2017;c->Y=seed*97;c->P=0x24;cpu_p_to_mirrors(c);deadline=0;filter_variant=0;
 Interp816*i=interp816_init(NULL,rd,wr);from(i,c,0x018096);unsigned steps=0;while((((unsigned)i->k<<16)|i->pc)!=0x02d54e&&steps++<80){unsigned pc=(unsigned)i->k<<16|i->pc;uint32_t landing;if(pc==0x0080e0&&!use_native_observer68)assert(!LagoonDeathMenu68_Process(rom,sizeof rom,c,c,7,7,pc,&landing));if(pc==0x0080e6&&use_native_observer68){c->host_return_valid=3;assert(death_observe_dispatch68(rom,sizeof rom,c,c,7,7,c->S,3,0x0080e4));}opcode(i,c);}assert(i->k==2&&i->pc==0xd54e&&c->S==0x1f8&&c->DB==2);if(defer!=0x02d54e)assert(death_take68(c,7,0));
 if(child){death_owner68[0].next=0x02d577;uint32_t landing;assert(!LagoonDeathMenu68_Process(rom,sizeof rom,c,c,7,7,0x02d577,&landing));from(i,c,0x02d577);opcode(i,c);assert(i->pc==0xd59c&&c->S==0x1f6);if(defer!=0x02d59c)assert(death_take68(c,7,1));}
 interp816_free(i);
}
static void death_one68(CpuState*c,unsigned pc){CpuState ref=*c;before=mem;uint32_t landing;static const CpuWramReadContext sentinel={.instruction_pc=0x123456};active_context=&sentinel;assert(LagoonDeathMenu68_Process(rom,sizeof rom,c,c,7,7,pc,&landing));assert(active_context==&sentinel);active_context=0;cpu_mirrors_to_p(c);actual=mem;mem=before;Interp816*i=interp816_init(NULL,rd,wr);from(i,&ref,pc);opcode(i,&ref);equal(c,&ref,i,landing);cases++;interp816_free(i);mem=actual;}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonDeathMenu68_Source(rom,sizeof rom));
/* Exact complete source/table/dispatcher mapping and each current instruction. */
const unsigned roots[]={0x0080e0,0x02d54e,0x02d59c};
for(unsigned root=0;root<3;root++){unsigned pc=roots[root];assert(death_mapped68(pc));const unsigned spans[][2]={{0x0080b3,0x36},{0x02801c,2},{0x02d54e,0x94}};for(unsigned j=0;j<3;j++)for(unsigned k=0;k<spans[j][1];k++){unsigned at=spans[j][0]+k;if(root==0&&j==2)continue;if(root==2&&(at<0x02d59c||at>=0x02d5c3))continue;bad_mapping=at;assert(!death_mapped68(pc));mapping_cases++;}bad_mapping=~0u;}
for(unsigned j=0;j<sizeof death_ops68/sizeof*death_ops68;j++)for(unsigned k=0;k<death_ops68[j][1];k++){bad_mapping=death_ops68[j][0]+k;assert(!death_mapped68(death_ops68[j][0]));mapping_cases++;}bad_mapping=~0u;
for(unsigned j=0;j<sizeof death_calls68/sizeof*death_calls68;j++)for(unsigned k=0;k<death_calls68[j][1];k++){bad_mapping=death_calls68[j][0]+k;assert(!death_mapped68(death_calls68[j][0]));mapping_cases++;}bad_mapping=~0u;for(unsigned pc=0x02d5de;pc<0x02d5e2;pc++)assert(!death_mapped68(pc));
/* Full A high byte, N/V/Z/C, every byte input and both owner depths. */
for(unsigned seed=0;seed<256;seed++)for(unsigned op=0;op<sizeof death_ops68/sizeof*death_ops68;op++){
 unsigned pc=death_ops68[op][0],child=pc>=0x02d59c&&pc<0x02d5c3;CpuState c;death_setup68(&c,seed,child,(pc==0x02d54e||pc==0x02d59c)?pc:0);death_owner68[child].next=pc;c.A=(seed*257)^0xa500;c.Y=seed*257;c.P=0x24|((seed&1)?1:0)|((seed&2)?2:0)|((seed&4)?64:0)|((seed&8)?128:0);cpu_p_to_mirrors(&c);ram[0x22e]=seed;ram[0x22f]=seed^0x5a;ram[1]=seed;death_one68(&c,pc);
}
/* Actual original895D sampler including nested JSRs/RTL; release/select,
 * navigation and confirm route to actual rendering/persistence boundaries. */
for(unsigned input=0;input<8;input++)for(unsigned selector=0;selector<2;selector++){
 CpuState c;death_setup68(&c,selector,0,0);for(unsigned j=0x21e;j<0x236;j++)ram[j]=0;ram[0x21e]=input==2?0x80:0;ram[0x21f]=input==0?4:input==1?8:input==3?0x10:input==4?12:0;ram[1]=selector;unsigned pc=input==5?0x02d566:0x02d57a,target=input==2||input==3?(selector?0x02d5d0:0x02d5c8):input==5?0x02d56a:input>=6?0x02d57e:0x02d597;death_owner68[0].next=pc;before=mem;CpuState ref=c;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,pc);unsigned steps=0;
 while(pc!=target&&steps++<1000){uint32_t landing;if(death_interested68(pc)&&LagoonDeathMenu68_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing)){pc=landing;from(i,&c,pc);}else{opcode(i,&c);pc=(unsigned)i->k<<16|i->pc;}}assert(pc==target);actual=mem;mem=before;unsigned begin=input==5?0x02d566:0x02d57a;from(i,&ref,begin);steps=0;while((((unsigned)i->k<<16)|i->pc)!=target&&steps++<1000)opcode(i,&ref);equal(&c,&ref,i,pc);cases++;interp816_free(i);
}
/* Actual root release before selection child: stop at real JSR instead of
 * inventing rendering completion. Real child pointer/PPU setup stops at89EC. */
for(unsigned seed=0;seed<256;seed++){
 CpuState c;death_setup68(&c,seed,1,0x02d59c);ram[1]=seed;death_owner68[1].next=0x02d59c;before=mem;CpuState ref=c;unsigned pc=0x02d59c;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,pc);unsigned steps=0;while(pc!=0x02d5ba&&steps++<100){uint32_t landing;assert(LagoonDeathMenu68_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing));pc=landing;from(i,&c,pc);}assert(pc==0x02d5ba);actual=mem;mem=before;from(i,&ref,0x02d59c);steps=0;while(i->pc!=0xd5ba&&steps++<100)opcode(i,&ref);equal(&c,&ref,i,pc);cases++;interp816_free(i);
}
for(unsigned fault=0;fault<24;fault++){
 CpuState c;death_setup68(&c,0,0,0);death_owner68[0].next=0x02d56a;uint64_t gen=7,frame=7;const CpuState*primary=&c;switch(fault){case 0:ram[death_owner68[0].s+1]^=1;break;case 1:ram[death_owner68[0].s+4]^=1;break;case 2:ram[death_owner68[0].s+7]^=1;break;case 3:c.S--;break;case 4:c.DB=1;break;case 5:c.PB=1;break;case 6:c.D=1;break;case 7:c.emulation=1;break;case 8:c.P|=16;cpu_p_to_mirrors(&c);break;case 9:c.P&=~32;cpu_p_to_mirrors(&c);break;case 10:c.P|=8;cpu_p_to_mirrors(&c);break;case 11:c.P&=~4;cpu_p_to_mirrors(&c);break;case 12:gen++;break;case 13:frame++;break;case 14:primary=0;break;case 15:death_owner68[0].gen++;break;case 16:death_owner68[0].ram=0;break;case 17:death_owner68[0].next++;break;case 18:deadline=c.master_cycles;break;case 19:rom[0x155de]^=1;break;case 20:rom[0xe0]^=1;break;case 21:death_owner68[0].armed=0;break;case 22:rom[0x1001c]^=1;break;case 23:death_owner68[0].cpu=0;break;}CpuState old=c;before=mem;uint32_t landing;assert(!LagoonDeathMenu68_Process(rom,sizeof rom,&c,primary,gen,frame,0x02d56a,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));if(fault==19)rom[0x155de]^=1;if(fault==20)rom[0xe0]^=1;if(fault==22)rom[0x1001c]^=1;refusals++;
}
/* Root deadline preserves only its freshly consumed genuine call witness. */
for(unsigned child=0;child<2;child++){CpuState c;unsigned pc=child?0x02d59c:0x02d54e;death_setup68(&c,0,child,pc);deadline=c.master_cycles;CpuState old=c;before=mem;uint32_t landing;assert(!LagoonDeathMenu68_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));deadline=0;death_one68(&c,pc);}
/* Genuine pending calls survive their destination yield with exact outer
 * frame; unrelated dispatch, nonlocal unwind, changed outer frame or current
 * caller mapping never let a stale pending observation arm a later root. */
for(unsigned child=0;child<2;child++)for(unsigned fault=0;fault<6;fault++){
 CpuState c;unsigned pc=child?0x02d59c:0x02d54e;death_setup68(&c,0,child,pc);uint32_t landing;
 switch(fault){case 0:death_retain68(&c,7,pc);break;case 1:{unsigned pb=c.PB,v=ram[0x10];c.PB=0;ram[0x10]=0;assert(!LagoonDeathMenu68_Process(rom,sizeof rom,&c,&c,7,7,0x0080e0,&landing));c.PB=pb;ram[0x10]=v;break;}case 2:death_retain68(&c,7,0x012345);break;case 3:ram[death_pending68[child].s+(child?7:4)]^=1;break;case 4:bad_mapping=death_pending68[child].pc+1;break;case 5:assert(!LagoonDeathMenu68_Process(rom,sizeof rom,&c,&c,7,7,0x02d56a,&landing));break;}
 if(!fault){death_one68(&c,pc);continue;}CpuState old=c;before=mem;assert(!LagoonDeathMenu68_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));bad_mapping=~0u;refusals++;
}
/* Generated paired dispatcher entry observation uses the physical frame from
 * original JSL80E0 plus native return-context metadata; no guest state changes.
 * It complements the independent source pre-opcode observation route. */
use_native_observer68=1;
for(unsigned seed=0;seed<256;seed++){CpuState c;death_setup68(&c,seed,0,0x02d54e);assert(death_pending68[0].armed);death_one68(&c,0x02d54e);}
for(unsigned fault=0;fault<24;fault++){
 CpuState c;death_setup68(&c,0,0,0x02d54e);c.PB=0;memset(death_pending68,0,sizeof death_pending68);memset(death_owner68,0,sizeof death_owner68);unsigned s=c.S,h=3,host=0x0080e4;uint64_t gen=7,frame=7;const CpuState*primary=&c;
 switch(fault){case 0:ram[s+1]^=1;break;case 1:ram[s+4]=3;break;case 2:h=2;break;case 3:c.host_return_valid=2;break;case 4:host++;break;case 5:s++;break;case 6:gen++;break;case 7:frame++;break;case 8:primary=0;break;case 9:ram[0x10]=0;break;case 10:ram[0x13]=0;break;case 11:c.DB=1;break;case 12:c.PB=2;break;case 13:c.emulation=1;break;case 14:c.D=1;break;case 15:c.P&=~32;cpu_p_to_mirrors(&c);break;case 16:c.P|=16;cpu_p_to_mirrors(&c);break;case 17:c.P|=8;cpu_p_to_mirrors(&c);break;case 18:c.P&=~4;cpu_p_to_mirrors(&c);break;case 19:rom[0x155de]^=1;break;case 20:bad_mapping=0x0080e0;break;case 21:bad_mapping=0x02801c;break;case 22:bad_mapping=0x02d54f;break;case 23:c.ram=0;break;}
 CpuState old=c;before=mem;assert(!death_observe_dispatch68(rom,sizeof rom,&c,primary,gen,frame,s,h,host));assert(!death_pending68[0].armed&&!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));if(fault==19)rom[0x155de]^=1;bad_mapping=~0u;refusals++;
}
use_native_observer68=0;
/* Unknown/restored root and child frames never self-arm. */
for(unsigned child=0;child<2;child++){CpuState c;death_setup68(&c,0,child,0);memset(death_owner68,0,sizeof death_owner68);memset(death_pending68,0,sizeof death_pending68);CpuState old=c;before=mem;uint32_t landing;assert(!LagoonDeathMenu68_Process(rom,sizeof rom,&c,&c,7,7,child?0x02d59c:0x02d54e,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));refusals++;}
printf("PASS death menu68: %u original opcode/real-dispatch/real895D/death selection routes; %u exact mapping refusals; %u immutable ownership/deadline refusals\n",cases,mapping_cases,refusals);return 0;}
