#define LAGOON_STAGING68_TEST 1
#include "../src/lagoon_native_staging68.c"
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


bool interp_bridge_source_read8(CpuState*c,uint32_t address,uint8_t*out){assert(active_context&&active_context->instruction_pc>=0x01c4ce&&active_context->instruction_pc<0x01c578&&active_context->interpreter==1);*out=cpu_read8(c,address>>16,address);return true;}
bool interp_bridge_source_read16(CpuState*c,uint32_t l,uint32_t h,uint16_t*out){unsigned v=cpu_read8(c,l>>16,l);*out=v|cpu_read8(c,h>>16,h)<<8;return true;}
bool interp_bridge_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_source_write16(CpuState*c,uint32_t l,uint32_t h,uint16_t v){cpu_write8(c,l>>16,l,v);cpu_write8(c,h>>16,h,v>>8);return true;}



bool interp_bridge_interrupt_source_read8(CpuState*c,uint32_t a,uint8_t*out){*out=cpu_read8(c,a>>16,a);return true;}
bool interp_bridge_interrupt_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_commit_interrupt_source_opcode(CpuState*c,uint64_t prior,bool wrote){return interp_bridge_commit_source_opcode(c,prior,wrote);}
static void equal(const CpuState*a,const CpuState*b,const Interp816*i,unsigned pc){
 if(memcmp(&mem,&actual,sizeof mem)||i->pc!=(pc&65535)||i->k!=(pc>>16)||a->A!=b->A||a->X!=b->X||a->Y!=b->Y||a->S!=b->S||a->D!=b->D||a->DB!=b->DB||a->P!=b->P||a->open_bus!=b->open_bus||a->cycles!=b->cycles||a->master_cycles!=b->master_cycles||a->coprocessor_master_cycles!=b->coprocessor_master_cycles){fprintf(stderr,"FAIL case%u pc=%06x/%02x%04x A=%04x/%04x X=%04x/%04x Y=%04x/%04x S=%04x/%04x P=%02x/%02x bus=%02x/%02x cyc=%llu/%llu clocks=%llu/%llu memory=%d writes=%llu/%llu\n",cases,pc,i->k,i->pc,a->A,b->A,a->X,b->X,a->Y,b->Y,a->S,b->S,a->P,b->P,a->open_bus,b->open_bus,(unsigned long long)a->cycles,(unsigned long long)b->cycles,(unsigned long long)a->master_cycles,(unsigned long long)b->master_cycles,memcmp(&mem,&actual,sizeof mem),(unsigned long long)mem.writes,(unsigned long long)actual.writes);exit(2);}}





static void staging_setup68(CpuState*c,unsigned seed,unsigned caller,unsigned observer){
 memset(&staging68,0,sizeof staging68);memset(&staging_call68,0,sizeof staging_call68);memset(&mem,0,sizeof mem);for(unsigned j=0;j<0x20000;j++)ram[j]=j*73+seed*17;memset(c,0,sizeof*c);c->ram=ram;c->S=0x1ff;c->DB=c->PB=1;c->A=seed*911;c->X=seed*2017;c->Y=seed*97;c->P=0x24;cpu_p_to_mirrors(c);deadline=0;filter_variant=0;bad_mapping=~0u;
 uint32_t landing;if(!observer){assert(!LagoonStaging68_Process(rom,sizeof rom,c,c,7,7,caller,&landing));assert(staging_call68.armed);}else assert(!staging_call68.armed);Interp816*i=interp816_init(NULL,rd,wr);from(i,c,caller);opcode(i,c);assert(i->pc==0xc4af&&c->S==0x1fc);interp816_free(i);
 if(observer){c->host_return_valid=3;CpuState saved=*c;before=mem;assert(staging_observe68(rom,sizeof rom,c,c,7,7,c->S,3,caller+4));assert(!memcmp(c,&saved,sizeof saved)&&!memcmp(&before,&mem,sizeof mem));}else{assert(!LagoonStaging68_Process(rom,sizeof rom,c,c,7,7,0x01c4af,&landing));assert(staging68.armed);}
}
static int staging_interested68(unsigned pc){if(pc==0x01c4af||staging_caller68(pc)||pc==0x0180eb||pc==0x01c6cb)return 1;for(unsigned j=0;j<sizeof staging_ops68/sizeof*staging_ops68;j++)if(staging_ops68[j][0]==pc)return 1;for(unsigned j=0;j<sizeof staging_boundaries68/sizeof*staging_boundaries68;j++)if(staging_boundaries68[j][0]==pc)return 1;return 0;}
static void staging_one68(CpuState*c,unsigned pc){CpuState ref=*c;before=mem;uint32_t landing;static const CpuWramReadContext sentinel={.instruction_pc=0x123456};active_context=&sentinel;assert(LagoonStaging68_Process(rom,sizeof rom,c,c,7,7,pc,&landing));assert(active_context==&sentinel);active_context=0;actual=mem;mem=before;Interp816*i=interp816_init(NULL,rd,wr);from(i,&ref,pc);opcode(i,&ref);equal(c,&ref,i,landing);cases++;interp816_free(i);mem=actual;}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonStaging68_Source(rom,sizeof rom));
assert(staging_mapped68(0x01c4af));for(unsigned pc=0x01c4af;pc<0x01c59f;pc++){bad_mapping=pc;assert(!staging_mapped68(0x01c4af));mapping_cases++;}bad_mapping=~0u;
for(unsigned caller=0;caller<2;caller++)for(unsigned k=0;k<4;k++){unsigned pc=caller?0x01c6c7:0x0180e7;bad_mapping=pc+k;assert(!staging_mapped68(pc));mapping_cases++;}bad_mapping=~0u;
for(unsigned j=0;j<sizeof staging_ops68/sizeof*staging_ops68;j++){unsigned pc=staging_ops68[j][0];assert(staging_mapped68(pc));for(unsigned k=0;k<staging_ops68[j][1];k++){bad_mapping=pc+k;assert(!staging_mapped68(pc));mapping_cases++;}bad_mapping=~0u;}
/* No replacement of initial multiplier setup or the already-native C579. */
for(unsigned pc=0x01c4b0;pc<0x01c4ce;pc++)assert(!staging_interested68(pc));for(unsigned pc=0x01c579;pc<0x01c59f;pc++)assert(!staging_interested68(pc));
/* Every byte seed, hiddenA, both carry inputs, word and byte PHA/PLA depths,
 * exact original bus effects and high-first word RMW writes. */
for(unsigned seed=0;seed<256;seed++)for(unsigned j=0;j<sizeof staging_ops68/sizeof*staging_ops68;j++){
 CpuState c;staging_setup68(&c,seed,seed&1?0x01c6c7:0x0180e7,seed&1);const unsigned*op=staging_ops68[j];c.S=staging68.s-op[4];c.P=4|(op[3]?32:0)|((seed&1)?1:0)|((seed&2)?2:0)|((seed&4)?64:0)|((seed&8)?128:0);cpu_p_to_mirrors(&c);c.A=seed*911;c.X=seed*257;c.Y=seed*97;staging68.next=op[0];staging_one68(&c,op[0]);
}
/* Whole finite fallback context. Actual original hardware-setup instructions,
 * all four original C579 calls and every child copy instruction execute in
 * the byte-bus oracle; their results and returns are never fabricated.
 * This fixture proves CPU/bus parity, not hardware multiplier latency. */
for(unsigned seed=0;seed<16;seed++)for(unsigned which=0;which<2;which++)for(unsigned observer=0;observer<2;observer++){
 unsigned caller=which?0x01c6c7:0x0180e7;CpuState c;staging_setup68(&c,seed,caller,observer);ram[0x51c]=seed%8;ram[0x51d]=seed%12;ram[0x51e]=seed%31;ram[0x51f]=seed%12;mem.io[0x4216]=(seed%8)*50;mem.io[0x4217]=0;
 before=mem;CpuState ref=c;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,0x01c4af);unsigned pc=0x01c4af,steps=0,stop=caller+4;deadline=128;
 while(pc!=stop&&steps++<100000){if(c.master_cycles>=deadline)deadline=c.master_cycles+128;uint32_t landing;if(staging_interested68(pc)&&LagoonStaging68_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing)){pc=landing;from(i,&c,pc);}else{opcode(i,&c);pc=(unsigned)i->k<<16|i->pc;}}
 assert(pc==stop&&c.S==0x1ff&&!staging68.armed);actual=mem;mem=before;from(i,&ref,0x01c4af);steps=0;while((((unsigned)i->k<<16)|i->pc)!=stop&&steps++<100000)opcode(i,&ref);equal(&c,&ref,i,pc);cases++;interp816_free(i);
}
/* Source/mode/physical frame/generation/current sequence/deadline are exact. */
for(unsigned fault=0;fault<23;fault++){
 CpuState c;staging_setup68(&c,0,0x0180e7,0);c.P&=~32;cpu_p_to_mirrors(&c);staging68.next=0x01c4e1;uint64_t gen=7,frame=7;const CpuState*primary=&c;
 switch(fault){case 0:staging68.armed=0;break;case 1:staging68.cpu=0;break;case 2:staging68.ram=0;break;case 3:staging68.gen++;break;case 4:gen++;break;case 5:frame++;break;case 6:primary=0;break;case 7:c.D=1;break;case 8:c.DB=2;break;case 9:c.PB=2;break;case 10:c.S--;break;case 11:staging68.next++;break;case 12:c.P|=8;cpu_p_to_mirrors(&c);break;case 13:c.P&=~4;cpu_p_to_mirrors(&c);break;case 14:c.P|=16;cpu_p_to_mirrors(&c);break;case 15:c.P|=32;cpu_p_to_mirrors(&c);break;case 16:c.emulation=1;break;case 17:ram[staging68.s+1]^=1;break;case 18:ram[staging68.s+3]^=1;break;case 19:rom[0x80e7]^=1;break;case 20:deadline=c.master_cycles;break;case 21:rom[0xc579]^=1;break;case 22:rom[0xc4cc]^=1;break;}
 CpuState saved=c;before=mem;uint32_t landing;assert(!LagoonStaging68_Process(rom,sizeof rom,&c,primary,gen,frame,0x01c4e1,&landing));assert(!memcmp(&saved,&c,sizeof c)&&!memcmp(&before,&mem,sizeof mem));if(fault==19)rom[0x80e7]^=1;if(fault==21)rom[0xc579]^=1;if(fault==22)rom[0xc4cc]^=1;refusals++;
}
/* Restored/plausible root without real call or fresh native entry cannot arm. */
{CpuState c;staging_setup68(&c,0,0x0180e7,0);staging68.armed=staging_call68.armed=0;uint32_t landing;CpuState saved=c;assert(!LagoonStaging68_Process(rom,sizeof rom,&c,&c,7,7,0x01c4af,&landing));assert(!staging68.armed&&!memcmp(&saved,&c,sizeof c));refusals++;}
for(unsigned fault=0;fault<10;fault++){CpuState c;staging_setup68(&c,0,0x0180e7,1);unsigned s=c.S,h=3,host=0x0180eb;switch(fault){case 0:c.host_return_valid=0;break;case 1:h=2;break;case 2:s++;break;case 3:host++;break;case 4:ram[c.S+1]^=1;break;case 5:ram[c.S+3]^=1;break;case 6:c.P&=~32;cpu_p_to_mirrors(&c);break;case 7:bad_mapping=0x0180e7;break;case 8:bad_mapping=0x01c4ec;break;case 9:c.DB=2;break;}CpuState saved=c;before=mem;assert(!staging_observe68(rom,sizeof rom,&c,&c,7,7,s,h,host));assert(!staging68.armed&&!memcmp(&saved,&c,sizeof c)&&!memcmp(&before,&mem,sizeof mem));bad_mapping=~0u;refusals++;}
for(unsigned fault=0;fault<6;fault++){CpuState c;staging_setup68(&c,0,0x0180e7,0);switch(fault){case 0:staging_retain68(&c,7,0x01c4af);assert(staging68.armed);break;case 1:staging_retain68(&c,7,0x01c579);assert(staging68.armed);break;case 2:staging_retain68(&c,7,0x0180eb);assert(!staging68.armed);break;case 3:staging_retain68(&c,8,0x01c4e1);assert(!staging68.armed);break;case 4:c.S+=3;staging_retain68(&c,7,0x01c4e1);assert(!staging68.armed);break;case 5:ram[staging68.s+3]^=1;staging_retain68(&c,7,0x01c4e1);assert(!staging68.armed);break;}if(fault>=2)refusals++;}
printf("PASS staging68: %u full-state original opcode/actual-call/whole-parent/real-child-copy/return cases; %u exact mapping refusals; %u immutable source/owner/mode/deadline refusals\n",cases,mapping_cases,refusals);return 0;
}
