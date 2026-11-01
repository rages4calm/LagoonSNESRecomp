#define LAGOON_CAMERA68_TEST 1
#include "../src/lagoon_native_camera68.c"
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
static unsigned offset67(unsigned pc){return (pc>>16)*32768+(pc&32767);}
int cpu_aot_rom_mapping_matches(uint32_t pc,uint32_t offset,unsigned n){assert(n&&n<=4&&offset==offset67(pc));return !(bad_mapping>=pc&&bad_mapping<pc+n);}
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


bool interp_bridge_source_read8(CpuState*c,uint32_t address,uint8_t*out){assert(active_context&&active_context->pb==1&&active_context->m==1&&active_context->x==0&&active_context->interpreter==1);*out=cpu_read8(c,address>>16,address);return true;}
bool interp_bridge_source_read16(CpuState*c,uint32_t l,uint32_t h,uint16_t*out){unsigned v=cpu_read8(c,l>>16,l);*out=v|cpu_read8(c,h>>16,h)<<8;return true;}
bool interp_bridge_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_source_write16(CpuState*c,uint32_t l,uint32_t h,uint16_t v){cpu_write8(c,l>>16,l,v);cpu_write8(c,h>>16,h,v>>8);return true;}



bool interp_bridge_interrupt_source_read8(CpuState*c,uint32_t a,uint8_t*out){*out=cpu_read8(c,a>>16,a);return true;}
bool interp_bridge_interrupt_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_commit_interrupt_source_opcode(CpuState*c,uint64_t prior,bool wrote){return interp_bridge_commit_source_opcode(c,prior,wrote);}
static void equal(const CpuState*a,const CpuState*b,const Interp816*i,unsigned pc){
 if(memcmp(&mem,&actual,sizeof mem)||i->pc!=(pc&65535)||i->k!=(pc>>16)||a->A!=b->A||a->X!=b->X||a->Y!=b->Y||a->S!=b->S||a->D!=b->D||a->DB!=b->DB||a->P!=b->P||a->open_bus!=b->open_bus||a->cycles!=b->cycles||a->master_cycles!=b->master_cycles||a->coprocessor_master_cycles!=b->coprocessor_master_cycles){fprintf(stderr,"FAIL case%u pc=%06x/%02x%04x A=%04x/%04x X=%04x/%04x Y=%04x/%04x S=%04x/%04x P=%02x/%02x bus=%02x/%02x cyc=%llu/%llu clocks=%llu/%llu memory=%d writes=%llu/%llu\n",cases,pc,i->k,i->pc,a->A,b->A,a->X,b->X,a->Y,b->Y,a->S,b->S,a->P,b->P,a->open_bus,b->open_bus,(unsigned long long)a->cycles,(unsigned long long)b->cycles,(unsigned long long)a->master_cycles,(unsigned long long)b->master_cycles,memcmp(&mem,&actual,sizeof mem),(unsigned long long)mem.writes,(unsigned long long)actual.writes);exit(2);}}

static uint8_t stock_rom[0x100000];
static void source_variant68(unsigned variant){memcpy(rom,stock_rom,sizeof rom);if(variant==2){static const uint8_t center[]={0x5a,0x9b,0xd0,0x16,0xda,0x64,0x41,0x20,0xfb,0x83,0x20,0x26,0x84,0x20,0x52,0x84,0x20,0x7d,0x84,0xfa,0xad,0x19,0x05,0x8d,0x19,0x05,0x7a,0x4c,0xb5,0xa7,0xea,0xea,0xea,0xea,0xea};memcpy(rom+0x83d8,center,sizeof center);memset(rom+0x809d,0xea,3);rom[0x80cb]=0xd8;rom[0x80cc]=0x83;rom[0x8408]=rom[0x8432]=128;rom[0x845f]=rom[0x8489]=112;}assert(LagoonCamera68_Source(rom,sizeof rom)==(int)variant);}
static void setup68(CpuState*c,unsigned seed,unsigned variant){source_variant68(variant);memset(&camera68,0,sizeof camera68);memset(&camera_call68,0,sizeof camera_call68);memset(&mem,0,sizeof mem);for(unsigned j=0;j<sizeof mem.ram;j++)ram[j]=j*73+seed*17;memset(c,0,sizeof*c);c->ram=ram;c->S=0x1ff;c->DB=c->PB=1;c->A=seed*911;c->X=seed*2017;c->Y=seed*97;c->P=0x24|((seed&1)?1:0)|((seed&2)?2:0)|((seed&4)?64:0)|((seed&8)?128:0);cpu_p_to_mirrors(c);deadline=0;filter_variant=0;bad_mapping=~0u;}
static void enter68(CpuState*c,unsigned variant){unsigned caller=variant==1?0x01809d:0x0180ca;uint32_t landing;CpuState old=*c;assert(!LagoonCamera68_Process(rom,sizeof rom,c,c,7,7,caller,&landing));assert(camera_call68.armed&&!memcmp(c,&old,sizeof old));Interp816*i=interp816_init(NULL,rd,wr);from(i,c,caller);opcode(i,c);assert(i->pc==0x83d8&&c->S==0x1fd);interp816_free(i);}
static void one68(CpuState*c,unsigned pc){CpuState ref=*c;before=mem;uint32_t landing=0;assert(LagoonCamera68_Process(rom,sizeof rom,c,c,7,7,pc,&landing));cpu_mirrors_to_p(c);actual=mem;mem=before;Interp816*i=interp816_init(NULL,rd,wr);from(i,&ref,pc);opcode(i,&ref);equal(c,&ref,i,landing);cases++;interp816_free(i);mem=actual;}
static int interested68(unsigned variant,unsigned pc){return camera_op68(variant,pc)||(variant==2&&(pc==0x0183df||pc==0x0183e2||pc==0x0183e5||pc==0x0183e8||pc==0x0183f3))||(variant==1&&(pc==0x0183ef||pc==0x0183fa));}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(stock_rom,1,sizeof stock_rom,f)==sizeof stock_rom);fclose(f);
/* Opcode units use actual source JSR ownership; each opcode is compared to the
 * original interpreter at its exact saved-register depth, including hiddenA. */
for(unsigned variant=1;variant<=2;variant++){const unsigned(*ops)[4]=variant==1?camera_stock_ops68:camera_center_ops68;unsigned count=variant==1?sizeof camera_stock_ops68/sizeof*camera_stock_ops68:sizeof camera_center_ops68/sizeof*camera_center_ops68;
 for(unsigned seed=0;seed<256;seed++)for(unsigned j=0;j<count;j++){CpuState c;setup68(&c,seed,variant);enter68(&c,variant);uint32_t landing;assert(LagoonCamera68_Process(rom,sizeof rom,&c,&c,7,7,0x0183d8,&landing));camera68.next=ops[j][0];c.S=camera68.s-ops[j][3];c.Y=seed*257;one68(&c,ops[j][0]);}
 /* Exact owner/main/source/current-opcode mapping refusal checks. */
 source_variant68(variant);for(unsigned root=0;root<2;root++){unsigned pc=root?0x0183d8:(variant==1?0x01809d:0x0180ca);assert(camera_mapped68(variant,pc));for(unsigned k=0;k<0x6d;k++){bad_mapping=0x01807a+k;assert(!camera_mapped68(variant,pc));mapping_cases++;}for(unsigned k=0;k<0xcf;k++){bad_mapping=0x0183d8+k;assert(!camera_mapped68(variant,pc));mapping_cases++;}bad_mapping=~0u;}
 for(unsigned j=1;j<count;j++)for(unsigned k=0;k<ops[j][1];k++){bad_mapping=ops[j][0]+k;assert(!camera_mapped68(variant,ops[j][0]));mapping_cases++;}bad_mapping=~0u;
 if(variant==1)for(unsigned pc=0x0183f2;pc<0x0183fa;pc++)assert(!camera_mapped68(variant,pc));
 /* Whole retail dispatcher+original direction JMP/helper/RTS; whole centered
  * dispatcher with four REAL JSR/helpers/RTS. Original DMA skip condition is
  * explicit RAM fixture496=FF, not a fabricated helper result. */
 for(unsigned seed=0;seed<32;seed++)for(unsigned actor=0;actor<2;actor++)for(unsigned dir=0;dir<5;dir++){
  CpuState c;setup68(&c,seed,variant);c.X=actor?64:0;c.A&=255;ram[0x519]=dir==4?0xff:dir;ram[0x496]=255;ram[0x502]=128;ram[0x503]=0;ram[0x504]=112;ram[0x505]=0;ram[0x480]=ram[0x481]=ram[0x482]=ram[0x483]=0;enter68(&c,variant);before=mem;CpuState ref=c;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,0x0183d8);unsigned pc=0x0183d8,steps=0,stop=variant==1?0x0180a0:0x0183f3;deadline=128;
  while(pc!=stop&&steps++<3000){if(c.master_cycles>=deadline)deadline=c.master_cycles+128;uint32_t landing;if(interested68(variant,pc)&&LagoonCamera68_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing)){pc=landing;from(i,&c,pc);}else{opcode(i,&c);pc=((unsigned)i->k<<16)|i->pc;}}
  assert(pc==stop&&c.S==(variant==1?0x1ff:0x1fd));actual=mem;mem=before;from(i,&ref,0x0183d8);steps=0;while((((unsigned)i->k<<16)|i->pc)!=stop&&steps++<3000)opcode(i,&ref);equal(&c,&ref,i,stop);cases++;interp816_free(i);
  if(variant==2){uint32_t landing;CpuState old=c;assert(!LagoonCamera68_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing));assert(!camera68.armed&&!memcmp(&old,&c,sizeof c));}
 }
 /* Every wrong owner/mode/source/frame/generation/deadline is immutable. */
 for(unsigned fault=0;fault<18;fault++){CpuState c;setup68(&c,0,variant);enter68(&c,variant);uint32_t landing;assert(LagoonCamera68_Process(rom,sizeof rom,&c,&c,7,7,0x0183d8,&landing));unsigned pc=landing;uint64_t gen=7,frame=7;const CpuState*primary=&c;switch(fault){case 0:c.P|=8;cpu_p_to_mirrors(&c);break;case 1:c.D=1;break;case 2:c.S--;break;case 3:c.DB=2;break;case 4:c.P|=16;cpu_p_to_mirrors(&c);break;case 5:c.emulation=1;break;case 6:c.PB=2;break;case 7:rom[0x83f8]^=1;break;case 8:camera68.gen++;break;case 9:camera68.cpu=0;break;case 10:ram[camera68.s+1]^=1;break;case 11:camera68.next++;break;case 12:gen++;break;case 13:frame++;break;case 14:primary=0;break;case 15:c.P&=~4;cpu_p_to_mirrors(&c);break;case 16:deadline=c.master_cycles;break;case 17:c.P&=~32;cpu_p_to_mirrors(&c);break;}CpuState old=c;before=mem;assert(!LagoonCamera68_Process(rom,sizeof rom,&c,primary,gen,frame,pc,&landing));assert(!memcmp(&old,&c,sizeof c)&&!memcmp(&before,&mem,sizeof mem));refusals++;}
 /* A plausible restored frame without witnessing the source call is refused. */
 {CpuState c;setup68(&c,0,variant);enter68(&c,variant);camera_call68.armed=0;CpuState old=c;before=mem;uint32_t landing;assert(!LagoonCamera68_Process(rom,sizeof rom,&c,&c,7,7,0x0183d8,&landing));assert(!memcmp(&old,&c,sizeof c)&&!memcmp(&before,&mem,sizeof mem));refusals++;}
 /* Entry deadline must resume the witnessed same opcode without a new call. */
 {CpuState c;setup68(&c,0,variant);enter68(&c,variant);deadline=c.master_cycles;CpuState old=c;before=mem;uint32_t landing;assert(!LagoonCamera68_Process(rom,sizeof rom,&c,&c,7,7,0x0183d8,&landing));assert(!memcmp(&old,&c,sizeof c)&&!memcmp(&before,&mem,sizeof mem));deadline=0;one68(&c,0x0183d8);refusals++;}
}
printf("PASS camera68: %u full-state original opcode/actual-call/helper routes; %u mapping refusals; %u immutable owner/mode/deadline refusals\n",cases,mapping_cases,refusals);return 0;}
