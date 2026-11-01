#define LAGOON_COLLISION68_TEST 1
#include "../src/lagoon_native_collision68.c"
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


bool interp_bridge_source_read8(CpuState*c,uint32_t address,uint8_t*out){assert(active_context&&active_context->instruction_pc>=0x019bce&&active_context->instruction_pc<0x01a01a&&active_context->stack==c->S&&active_context->y==c->Y&&active_context->interpreter==1);*out=cpu_read8(c,address>>16,address);return true;}
bool interp_bridge_source_read16(CpuState*c,uint32_t l,uint32_t h,uint16_t*out){unsigned v=cpu_read8(c,l>>16,l);*out=v|cpu_read8(c,h>>16,h)<<8;return true;}
bool interp_bridge_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_source_write16(CpuState*c,uint32_t l,uint32_t h,uint16_t v){cpu_write8(c,l>>16,l,v);cpu_write8(c,h>>16,h,v>>8);return true;}



bool interp_bridge_interrupt_source_read8(CpuState*c,uint32_t a,uint8_t*out){*out=cpu_read8(c,a>>16,a);return true;}
bool interp_bridge_interrupt_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_commit_interrupt_source_opcode(CpuState*c,uint64_t prior,bool wrote){return interp_bridge_commit_source_opcode(c,prior,wrote);}
static void equal(const CpuState*a,const CpuState*b,const Interp816*i,unsigned pc){
 if(memcmp(&mem,&actual,sizeof mem)||i->pc!=(pc&65535)||i->k!=(pc>>16)||a->A!=b->A||a->X!=b->X||a->Y!=b->Y||a->S!=b->S||a->D!=b->D||a->DB!=b->DB||a->P!=b->P||a->open_bus!=b->open_bus||a->cycles!=b->cycles||a->master_cycles!=b->master_cycles||a->coprocessor_master_cycles!=b->coprocessor_master_cycles){fprintf(stderr,"FAIL case%u pc=%06x/%02x%04x A=%04x/%04x X=%04x/%04x Y=%04x/%04x S=%04x/%04x P=%02x/%02x bus=%02x/%02x cyc=%llu/%llu clocks=%llu/%llu memory=%d writes=%llu/%llu\n",cases,pc,i->k,i->pc,a->A,b->A,a->X,b->X,a->Y,b->Y,a->S,b->S,a->P,b->P,a->open_bus,b->open_bus,(unsigned long long)a->cycles,(unsigned long long)b->cycles,(unsigned long long)a->master_cycles,(unsigned long long)b->master_cycles,memcmp(&mem,&actual,sizeof mem),(unsigned long long)mem.writes,(unsigned long long)actual.writes);exit(2);}}



static void setup68(CpuState*c,unsigned seed){
 memset(&collision68,0,sizeof collision68);memset(&pending68,0,sizeof pending68);memset(&mem,0,sizeof mem);for(unsigned j=0;j<0x20000;j++)ram[j]=j*73+seed*17;filter_variant=0;deadline=0;bad_mapping=~0u;mem.writes=0;
 memset(c,0,sizeof*c);c->ram=ram;c->S=0x1ff;c->PB=c->DB=1;c->A=seed*911;c->X=(seed%33)*64;c->Y=seed*97;c->P=0x24|((seed&1)?1:0)|((seed&2)?2:0)|((seed&4)?64:0)|((seed&8)?128:0);cpu_p_to_mirrors(c);
 uint32_t landing;assert(!LagoonCollision68_Process(rom,sizeof rom,c,c,7,7,0x0180c1,&landing));assert(pending68.armed);Interp816*i=interp816_init(NULL,rd,wr);from(i,c,0x0180c1);opcode(i,c);assert(i->pc==0x9bce&&c->S==0x1fd);interp816_free(i);assert(collision_arm68(c,7));c->cycles=c->master_cycles=c->coprocessor_master_cycles=0;mem.writes=0;
}
static void one68(CpuState*c,unsigned pc){CpuState ref=*c;before=mem;uint32_t landing=0;assert(LagoonCollision68_Process(rom,sizeof rom,c,c,7,7,pc,&landing));actual=mem;mem=before;Interp816*i=interp816_init(NULL,rd,wr);from(i,&ref,pc);opcode(i,&ref);equal(c,&ref,i,landing);cases++;interp816_free(i);mem=actual;}
static int interested68(unsigned pc){if(pc==0x0180c1||pc==0x0180c4||pc==0x019bce)return 1;for(unsigned j=0;j<sizeof collision_ops68/sizeof*collision_ops68;j++)if(collision_ops68[j][0]==pc)return 1;for(unsigned j=0;j<sizeof collision_boundaries68/sizeof*collision_boundaries68;j++)if(collision_boundaries68[j][0]==pc)return 1;return 0;}
static void pointer68(unsigned at,unsigned v){ram[at]=v;ram[at+1]=v>>8;ram[at+2]=v>>16;}
static void word68(unsigned at,unsigned v){ram[at]=v;ram[at+1]=v>>8;}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonCollision68_Source(rom,sizeof rom));
/* Mapping includes the entire original owner/table region and each byte of
 * every accelerated instruction. No data-table entry may become a callback. */
assert(collision_mapped68(0x019bce));for(unsigned pc=0x019bce;pc<0x01a01a;pc++){bad_mapping=pc;assert(!collision_mapped68(0x019bce));mapping_cases++;}bad_mapping=~0u;
for(unsigned k=0;k<3;k++){bad_mapping=0x0180c1+k;assert(!collision_mapped68(0x019bce));mapping_cases++;}bad_mapping=~0u;
for(unsigned j=0;j<sizeof collision_ops68/sizeof*collision_ops68;j++){unsigned pc=collision_ops68[j][0];assert(collision_mapped68(pc));for(unsigned k=0;k<collision_ops68[j][1];k++){bad_mapping=pc+k;assert(!collision_mapped68(pc));mapping_cases++;}bad_mapping=~0u;}
for(unsigned pc=0x019be2;pc<0x019bee;pc++)assert(!interested68(pc));for(unsigned pc=0x019dba;pc<0x019e0a;pc++)assert(!interested68(pc));
/* All256 byte seeds, both carry inputs, full A high byte, exact PHY depth,
 * all supported actor slots, 8/16-bit operands and original bus side effects. */
for(unsigned seed=0;seed<256;seed++)for(unsigned j=0;j<sizeof collision_ops68/sizeof*collision_ops68;j++){
 CpuState c;setup68(&c,seed);const unsigned*op=collision_ops68[j];c.P=(c.P&~32)|(op[3]?32:0);cpu_p_to_mirrors(&c);c.S-=op[4];one68(&c,op[0]);
}
/* Entire actual original dispatcher+generic and four direction probes. The
 * sampler8385 executes every original instruction including its own saved
 * registers and physical RTS. Nothing returns a fabricated collision value. */
for(unsigned seed=0;seed<8;seed++)for(unsigned type=0;type<3;type++)for(unsigned direction=0;direction<4;direction++)for(unsigned blocked=0;blocked<2;blocked++){
 CpuState c;setup68(&c,seed);c.A&=255;ram[0x518+c.X]=type;ram[0x519+c.X]=direction;ram[0x501]=0;ram[0x519]=direction;word68(0x502+c.X,128);word68(0x504+c.X,128);
 pointer68(0x88,0x7e3000);pointer68(0x8b,0x7e4000);pointer68(0x78,0x7e5000);pointer68(0x6c,0x7e6000);memset(ram+0x3000,0,0x2000);for(unsigned j=0;j<0x1000;j++)ram[0x5000+j]=(j&1)?0:(blocked?0x80:0);memset(ram+0x6000,0,0x2000);
 before=mem;CpuState ref=c;unsigned pc=0x019bce,steps=0;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,pc);deadline=128;
 while(pc!=0x0180c4&&steps++<10000){if(c.master_cycles>=deadline)deadline=c.master_cycles+128;uint32_t landing;if(interested68(pc)&&LagoonCollision68_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing)){pc=landing;from(i,&c,pc);}else{opcode(i,&c);pc=((unsigned)i->k<<16)|i->pc;}}
 assert(pc==0x0180c4&&c.S==0x1ff);assert(!collision68.armed);actual=mem;mem=before;from(i,&ref,0x019bce);steps=0;while((i->k!=1||i->pc!=0x80c4)&&steps++<10000)opcode(i,&ref);equal(&c,&ref,i,pc);cases++;interp816_free(i);
}
/* Exact source contexts for follower's four landing/collision read filters. */
for(unsigned j=0;j<4;j++){CpuState c;setup68(&c,j);unsigned pc=0x019e29+j*0x84;assert(collision68.armed);one68(&c,pc);}
/* Unknown/restored root must not acquire ownership from plausible stackbytes.
 * Mode, width, generation, mapping, frame, owner, deadline refusal is immutable. */
for(unsigned fault=0;fault<21;fault++){
 CpuState c;setup68(&c,0);uint32_t landing;unsigned pc=0x019bef;const CpuState*primary=&c;uint64_t gen=7,frame=7;
 switch(fault){case 0:collision68.armed=0;break;case 1:collision68.cpu=0;break;case 2:collision68.ram=0;break;case 3:collision68.gen++;break;case 4:gen++;break;case 5:frame++;break;case 6:primary=0;break;case 7:c.D=1;break;case 8:c.DB=2;break;case 9:c.PB=2;break;case 10:c.S--;break;case 11:c.X+=64;break;case 12:c.P|=8;cpu_p_to_mirrors(&c);break;case 13:c.P&=~4;cpu_p_to_mirrors(&c);break;case 14:c.P|=16;cpu_p_to_mirrors(&c);break;case 15:c.P&=~32;cpu_p_to_mirrors(&c);break;case 16:c.emulation=1;break;case 17:ram[collision68.s+1]^=1;break;case 18:ram[collision68.s+2]^=1;break;case 19:rom[0x9be2]^=1;break;case 20:deadline=c.master_cycles=500;break;}
 CpuState saved=c;before=mem;assert(!LagoonCollision68_Process(rom,sizeof rom,&c,primary,gen,frame,pc,&landing));assert(!memcmp(&saved,&c,sizeof c)&&!memcmp(&before,&mem,sizeof mem));if(fault==19)rom[0x9be2]^=1;refusals++;
}
{CpuState c;setup68(&c,0);collision68.armed=collision68.fresh=0;assert(!pending68.armed);uint32_t landing;CpuState saved=c;assert(!LagoonCollision68_Process(rom,sizeof rom,&c,&c,7,7,0x019bce,&landing));assert(!collision68.armed&&!memcmp(&saved,&c,sizeof c));refusals++;}
/* A real call observation is consumed exactly once. Matching-looking bytes,
 * stale generation, another CPU/RAM or a different pre/post physical stack
 * cannot lend ownership to the generated parent's continuation. */
for(unsigned fault=0;fault<10;fault++){
 CpuState c;setup68(&c,0);collision68.armed=collision68.fresh=0;pending68.cpu=&c;pending68.ram=ram;pending68.gen=7;pending68.s=c.S+2;pending68.x=c.X;pending68.armed=1;
 switch(fault){case 0:pending68.armed=0;break;case 1:pending68.cpu=0;break;case 2:pending68.ram=0;break;case 3:pending68.gen++;break;case 4:pending68.s++;break;case 5:pending68.x++;break;case 6:ram[c.S+1]^=1;break;case 7:ram[c.S+2]^=1;break;case 8:c.P&=~32;cpu_p_to_mirrors(&c);break;case 9:c.DB=2;break;}
 CpuState saved=c;before=mem;assert(!collision_arm68(&c,7));assert(!pending68.armed&&!collision68.armed&&!memcmp(&saved,&c,sizeof c)&&!memcmp(&before,&mem,sizeof mem));refusals++;
}
printf("PASS collision68: %u full-state original opcode/real-dispatch/sampler/probe cases; %u exact mapping refusals; %u immutable source/owner/mode/deadline refusals\n",cases,mapping_cases,refusals);return 0;
}
