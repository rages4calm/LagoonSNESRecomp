#define LAGOON_MAP68_TEST 1
#include "../src/lagoon_native_map68.c"
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
int cpu_aot_rom_mapping_matches(uint32_t pc,uint32_t offset,unsigned n){assert(n&&n<=4&&offset==((pc>>16)*32768+(pc&32767)));return !(bad_mapping>=pc&&bad_mapping<pc+n);}
uint32_t cpu_region_speed(uint32_t address){unsigned b=address>>16,a=address&65535;if(b>=0x40&&b<=0x7f)return 8;if(b>=0xc0)return 8;if(a>=0x8000)return 8;if(a<0x2000)return 8;if(a<0x4000)return 6;if(a<0x4200)return 12;if(a<0x6000)return 6;return 8;}
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


bool interp_bridge_source_read8(CpuState*c,uint32_t address,uint8_t*out){assert(active_context&&active_context->instruction_pc>=0x02c5d5&&active_context->instruction_pc<0x02c64d&&active_context->interpreter==1);*out=cpu_read8(c,address>>16,address);return true;}
bool interp_bridge_source_read16(CpuState*c,uint32_t l,uint32_t h,uint16_t*out){unsigned v=cpu_read8(c,l>>16,l);*out=v|cpu_read8(c,h>>16,h)<<8;return true;}
bool interp_bridge_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_source_write16(CpuState*c,uint32_t l,uint32_t h,uint16_t v){cpu_write8(c,l>>16,l,v);cpu_write8(c,h>>16,h,v>>8);return true;}



bool interp_bridge_interrupt_source_read8(CpuState*c,uint32_t a,uint8_t*out){*out=cpu_read8(c,a>>16,a);return true;}
bool interp_bridge_interrupt_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_commit_interrupt_source_opcode(CpuState*c,uint64_t prior,bool wrote){return interp_bridge_commit_source_opcode(c,prior,wrote);}
static void equal(const CpuState*a,const CpuState*b,const Interp816*i,unsigned pc){
 if(memcmp(&mem,&actual,sizeof mem)||i->pc!=(pc&65535)||i->k!=(pc>>16)||a->A!=b->A||a->X!=b->X||a->Y!=b->Y||a->S!=b->S||a->D!=b->D||a->DB!=b->DB||a->P!=b->P||a->open_bus!=b->open_bus||a->cycles!=b->cycles||a->master_cycles!=b->master_cycles||a->coprocessor_master_cycles!=b->coprocessor_master_cycles){fprintf(stderr,"FAIL case%u pc=%06x/%02x%04x A=%04x/%04x X=%04x/%04x Y=%04x/%04x S=%04x/%04x P=%02x/%02x bus=%02x/%02x cyc=%llu/%llu clocks=%llu/%llu memory=%d writes=%llu/%llu\n",cases,pc,i->k,i->pc,a->A,b->A,a->X,b->X,a->Y,b->Y,a->S,b->S,a->P,b->P,a->open_bus,b->open_bus,(unsigned long long)a->cycles,(unsigned long long)b->cycles,(unsigned long long)a->master_cycles,(unsigned long long)b->master_cycles,memcmp(&mem,&actual,sizeof mem),(unsigned long long)mem.writes,(unsigned long long)actual.writes);exit(2);}}




static void map_setup68(CpuState*c,unsigned seed){
 memset(&map_owner68,0,sizeof map_owner68);memset(&mem,0,sizeof mem);for(unsigned j=0;j<0x20000;j++)ram[j]=j*73+seed*17;memset(c,0,sizeof*c);c->ram=ram;c->S=0x1ff;c->DB=c->PB=1;c->A=13;c->X=seed*2017;c->Y=seed*97;c->P=0x24;cpu_p_to_mirrors(c);deadline=0;filter_variant=0;bad_mapping=~0u;
 Interp816*i=interp816_init(NULL,rd,wr);from(i,c,0x018096);unsigned steps=0;
 while((((unsigned)i->k<<16)|i->pc)!=0x02c5c6&&steps++<80){unsigned pc=(unsigned)i->k<<16|i->pc;if(pc==0x0080e6){c->host_return_valid=3;CpuState saved=*c;before=mem;assert(map_observe68(rom,sizeof rom,c,c,7,7,c->S,3,0x0080e4));assert(!memcmp(c,&saved,sizeof saved)&&!memcmp(&before,&mem,sizeof mem));}opcode(i,c);}
 assert(i->k==2&&i->pc==0xc5c6&&c->S==0x1f8&&c->DB==2&&map_owner68.armed);interp816_free(i);
}
static void map_one68(CpuState*c,unsigned pc){CpuState ref=*c;before=mem;uint32_t landing;static const CpuWramReadContext sentinel={.instruction_pc=0x123456};active_context=&sentinel;assert(LagoonMap68_Process(rom,sizeof rom,c,c,7,7,pc,&landing));assert(active_context==&sentinel);active_context=0;actual=mem;mem=before;Interp816*i=interp816_init(NULL,rd,wr);from(i,&ref,pc);opcode(i,&ref);equal(c,&ref,i,landing);cases++;interp816_free(i);mem=actual;}
static int map_interested68(unsigned pc){if(pc==0x02c64d)return 1;for(unsigned j=0;j<sizeof map_ops68/sizeof*map_ops68;j++)if(map_ops68[j][0]==pc)return 1;return 0;}
static void map_word68(unsigned p,unsigned v){ram[p]=v;ram[p+1]=v>>8;}
static void map_pointer68(unsigned p,unsigned v){map_word68(p,v);ram[p+2]=v>>16;}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonMap68_Source(rom,sizeof rom));
assert(map_mapped68(0x0080e6));const unsigned spans[][2]={{0x0080b3,0x36},{0x02801a,2},{0x02c5c6,0x88}};
for(unsigned j=0;j<3;j++)for(unsigned k=0;k<spans[j][1];k++){bad_mapping=spans[j][0]+k;assert(!map_mapped68(0x0080e6));mapping_cases++;}bad_mapping=~0u;
for(unsigned j=0;j<sizeof map_ops68/sizeof*map_ops68;j++){unsigned pc=map_ops68[j][0];assert(map_mapped68(pc));for(unsigned k=0;k<map_ops68[j][1];k++){bad_mapping=pc+k;assert(!map_mapped68(pc));mapping_cases++;}bad_mapping=~0u;}
/* Initial clear is already owned by services65. It is deliberately not a
 * source substitution in this module; no optimized map/tile/HUD code is copied. */
for(unsigned pc=0x02c5c6;pc<0x02c5d5;pc++)assert(!map_interested68(pc));
/* Every byte seed, full hiddenA, physical PHA/PHY depth, exact widths and
 * original RMW high-first write order compare against SDK's interpreter. */
for(unsigned seed=0;seed<256;seed++)for(unsigned j=0;j<sizeof map_ops68/sizeof*map_ops68;j++){
 CpuState c;map_setup68(&c,seed);const unsigned*op=map_ops68[j];c.S=map_owner68.s-op[4];c.P=4|(op[3]?32:0)|((seed&1)?1:0)|((seed&2)?2:0)|((seed&4)?64:0)|((seed&8)?128:0);cpu_p_to_mirrors(&c);c.A=seed*911;c.X=seed*257;c.Y=seed*97;map_owner68.next=op[0];map_owner68.fresh=op[0]==0x02c5d5;if(map_owner68.fresh)c.X=0x8000;map_one68(&c,op[0]);
}
/* Whole real dispatcher+32KiB clear+two-plane metatile expansion+physical
 * RTL/PLB/RTL. The clear executes ORIGINAL instructions, not a fabricated
 * result. Private WRAM tables supply finite map inputs, not new ROM content. */
for(unsigned seed=0;seed<3;seed++)for(unsigned width=1;width<4;width++)for(unsigned height=1;height<3;height++){
 CpuState c;map_setup68(&c,seed);ram[0x52]=seed+17;map_word68(0x53,width*4);map_word68(0x55,height*4);map_pointer68(0x63,0x7e8000);map_pointer68(0x66,0x7e9000);map_pointer68(0x6f,0x7ea000);map_pointer68(0x72,0x7eb000);
 for(unsigned j=0;j<0x1000;j++){ram[0x8000+j]=(j+seed)&15;ram[0xa000+j]=(j*3+seed)&15;ram[0x9000+j]=j*7+seed;ram[0xb000+j]=j*11+seed;}
 before=mem;CpuState ref=c;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,0x02c5c6);unsigned pc=0x02c5c6,steps=0;deadline=128;
 while(pc!=0x01809a&&steps++<200000){if(c.master_cycles>=deadline)deadline=c.master_cycles+128;uint32_t landing;if(map_interested68(pc)&&LagoonMap68_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing)){pc=landing;from(i,&c,pc);}else{opcode(i,&c);pc=(unsigned)i->k<<16|i->pc;}}
 assert(pc==0x01809a&&c.S==0x1ff&&c.DB==1&&!map_owner68.armed);actual=mem;mem=before;from(i,&ref,0x02c5c6);steps=0;while((((unsigned)i->k<<16)|i->pc)!=0x01809a&&steps++<200000)opcode(i,&ref);equal(&c,&ref,i,pc);cases++;interp816_free(i);
}
/* Full source/mapping/dispatch/native entry metadata, owner frame, mode,
 * current source sequence, deadlines and nonlocal unwind refusals are immutable. */
for(unsigned fault=0;fault<24;fault++){
 CpuState c;map_setup68(&c,0);c.X=0x8000;map_owner68.next=0x02c5d5;uint64_t gen=7,frame=7;const CpuState*primary=&c;
 switch(fault){case 0:map_owner68.armed=0;break;case 1:map_owner68.cpu=0;break;case 2:map_owner68.ram=0;break;case 3:map_owner68.gen++;break;case 4:gen++;break;case 5:frame++;break;case 6:primary=0;break;case 7:c.D=1;break;case 8:c.DB=1;break;case 9:c.PB=1;break;case 10:c.S--;break;case 11:c.X=0;break;case 12:c.P|=8;cpu_p_to_mirrors(&c);break;case 13:c.P&=~4;cpu_p_to_mirrors(&c);break;case 14:c.P|=16;cpu_p_to_mirrors(&c);break;case 15:c.P&=~32;cpu_p_to_mirrors(&c);break;case 16:c.emulation=1;break;case 17:ram[map_owner68.s+1]^=1;break;case 18:ram[map_owner68.s+7]^=1;break;case 19:rom[0x1001a]^=1;break;case 20:rom[0xc5]^=1;break;case 21:deadline=c.master_cycles;break;case 22:map_owner68.next++;break;case 23:rom[0x1463d]^=1;break;}
 CpuState saved=c;before=mem;uint32_t landing;assert(!LagoonMap68_Process(rom,sizeof rom,&c,primary,gen,frame,0x02c5d5,&landing));assert(!memcmp(&c,&saved,sizeof c)&&!memcmp(&before,&mem,sizeof mem));if(fault==19)rom[0x1001a]^=1;if(fault==20)rom[0xc5]^=1;if(fault==23)rom[0x1463d]^=1;refusals++;
}
/* Genuine first-opcode deadline retains the consumed actual entry witness. */
{CpuState c;map_setup68(&c,0);c.X=0x8000;deadline=c.master_cycles;CpuState saved=c;before=mem;uint32_t landing;assert(!LagoonMap68_Process(rom,sizeof rom,&c,&c,7,7,0x02c5d5,&landing));assert(!memcmp(&c,&saved,sizeof c)&&!memcmp(&before,&mem,sizeof mem));deadline=0;map_one68(&c,0x02c5d5);}
for(unsigned fault=0;fault<7;fault++){CpuState c;map_setup68(&c,0);switch(fault){case 0:map_retain68(&c,7,0x0080e6);assert(map_owner68.armed);break;case 1:map_retain68(&c,7,0x02c5cb);assert(map_owner68.armed);break;case 2:map_retain68(&c,7,0x012345);assert(!map_owner68.armed);break;case 3:map_retain68(&c,8,0x02c5cb);assert(!map_owner68.armed);break;case 4:c.S+=7;map_retain68(&c,7,0x02c5cb);assert(!map_owner68.armed);break;case 5:ram[map_owner68.s+4]^=1;map_retain68(&c,7,0x02c5cb);assert(!map_owner68.armed);break;case 6:map_retain68(&c,7,0x02c5f9);assert(!map_owner68.armed);break;}if(fault>=2)refusals++;}
/* Native observer authority is exact entry metadata plus the actual physical
 * frame and selected dispatcher target. An unrelated/restored entry cannot
 * arm the map merely by presenting the same code address. */
for(unsigned fault=0;fault<17;fault++){
 CpuState c;map_setup68(&c,0);c.PB=0;unsigned s=c.S,h=3,host=0x0080e4;uint64_t gen=7,frame=7;const CpuState*primary=&c;
 switch(fault){case 0:c.host_return_valid=0;break;case 1:h=2;break;case 2:host++;break;case 3:s++;break;case 4:gen++;break;case 5:frame++;break;case 6:primary=0;break;case 7:ram[c.S+1]^=1;break;case 8:ram[c.S+2]^=1;break;case 9:ram[c.S+3]=1;break;case 10:ram[c.S+4]=3;break;case 11:ram[0x10]^=1;break;case 12:ram[0x11]^=1;break;case 13:ram[0x12]=1;break;case 14:ram[0x13]=14;break;case 15:bad_mapping=0x0080e0;break;case 16:c.PB=2;break;}
 CpuState saved=c;before=mem;assert(!map_observe68(rom,sizeof rom,&c,primary,gen,frame,s,h,host));assert(!map_owner68.armed&&!memcmp(&saved,&c,sizeof c)&&!memcmp(&before,&mem,sizeof mem));bad_mapping=~0u;refusals++;
}
printf("PASS map68: %u full-state original opcode/actual-dispatch/clear/map-expansion/return cases; %u exact mapping refusals; %u immutable source/owner/mode/deadline refusals\n",cases,mapping_cases,refusals);return 0;
}
