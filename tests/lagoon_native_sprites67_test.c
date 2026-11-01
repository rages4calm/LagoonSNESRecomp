#define LAGOON_SPRITES67_TEST 1
#include "../src/lagoon_native_sprites67.c"
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


bool interp_bridge_source_read8(CpuState*c,uint32_t address,uint8_t*out){assert(active_context&&active_context->instruction_pc>=0x8b71&&active_context->instruction_pc<0x8d0b&&active_context->stack==c->S&&active_context->y==c->Y&&active_context->interpreter==1);*out=cpu_read8(c,address>>16,address);return true;}
bool interp_bridge_source_read16(CpuState*c,uint32_t l,uint32_t h,uint16_t*out){unsigned v=cpu_read8(c,l>>16,l);*out=v|cpu_read8(c,h>>16,h)<<8;return true;}
bool interp_bridge_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_source_write16(CpuState*c,uint32_t l,uint32_t h,uint16_t v){cpu_write8(c,l>>16,l,v);cpu_write8(c,h>>16,h,v>>8);return true;}



bool interp_bridge_interrupt_source_read8(CpuState*c,uint32_t a,uint8_t*out){*out=cpu_read8(c,a>>16,a);return true;}
bool interp_bridge_interrupt_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_commit_interrupt_source_opcode(CpuState*c,uint64_t prior,bool wrote){return interp_bridge_commit_source_opcode(c,prior,wrote);}
static void equal(const CpuState*a,const CpuState*b,const Interp816*i,unsigned pc){
 if(memcmp(&mem,&actual,sizeof mem)||i->pc!=(pc&65535)||i->k!=(pc>>16)||a->A!=b->A||a->X!=b->X||a->Y!=b->Y||a->S!=b->S||a->D!=b->D||a->DB!=b->DB||a->P!=b->P||a->open_bus!=b->open_bus||a->cycles!=b->cycles||a->master_cycles!=b->master_cycles||a->coprocessor_master_cycles!=b->coprocessor_master_cycles){fprintf(stderr,"FAIL case%u pc=%06x/%02x%04x A=%04x/%04x X=%04x/%04x Y=%04x/%04x S=%04x/%04x P=%02x/%02x bus=%02x/%02x cyc=%llu/%llu clocks=%llu/%llu memory=%d writes=%llu/%llu\n",cases,pc,i->k,i->pc,a->A,b->A,a->X,b->X,a->Y,b->Y,a->S,b->S,a->P,b->P,a->open_bus,b->open_bus,(unsigned long long)a->cycles,(unsigned long long)b->cycles,(unsigned long long)a->master_cycles,(unsigned long long)b->master_cycles,memcmp(&mem,&actual,sizeof mem),(unsigned long long)mem.writes,(unsigned long long)actual.writes);exit(2);}}


static int interested_s67(unsigned pc){if(call_s67(rom,sizeof rom,pc)>=0||pc==0x8b75||pc==0x8bcf||pc==0x8c23||pc==0x8d0a)return 1;for(unsigned j=0;j<sizeof sprite_ops67/sizeof*sprite_ops67;j++)if(sprite_ops67[j][0]==pc)return 1;return 0;}
static void setup_s67(CpuState*c,unsigned seed,unsigned root){
 memset(sprite_owners67,0,sizeof sprite_owners67);memset(sprite_calls67,0,sizeof sprite_calls67);memset(&mem,0,sizeof mem);for(unsigned j=0;j<0x20000;j++)ram[j]=j*73+seed*17;filter_variant=0;mem.writes=0;memset(c,0,sizeof*c);c->ram=ram;c->S=0x1e0+(root==1?2:3);c->PB=root==1?0:1;c->DB=seed%3;c->A=seed*911;c->X=seed*2017;c->Y=seed*97;c->P=0x24|((seed&1)?1:0)|((seed&2)?2:0)|((seed&4)?64:0)|((seed&8)?128:0);cpu_p_to_mirrors(c);
 unsigned caller=root==0?0x1c27f:root==1?0x8bb0:0x1c151;uint32_t ignored;assert(!LagoonSprites67_Process(rom,sizeof rom,c,c,7,7,caller,&ignored));Interp816*i=interp816_init(NULL,rd,wr);from(i,c,caller);opcode(i,c);assert(i->pc==roots_s67[root]&&i->k==0);interp816_free(i);assert(take_call_s67(rom,c,7,root));c->cycles=c->master_cycles=c->coprocessor_master_cycles=0;
 ram[0x20]=0;ram[0x21]=0x30;ram[0x22]=0x7e;ram[0xc]=0;ram[0xd]=0x31;ram[0xe]=0x7e;mem.writes=0;
}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonSprites67_Source(rom,sizeof rom));
for(unsigned root=0;root<3;root++){assert(mapped_s67(roots_s67[root]));for(unsigned pc=0x8b4b;pc<0x8d0b;pc++){bad_mapping=pc;assert(!mapped_s67(roots_s67[root]));mapping_cases++;}bad_mapping=~0u;}
for(unsigned j=0;j<sizeof sprite_ops67/sizeof*sprite_ops67;j++){assert(mapped_s67(sprite_ops67[j][0]));for(unsigned k=0;k<sprite_ops67[j][1];k++){bad_mapping=sprite_ops67[j][0]+k;assert(!mapped_s67(sprite_ops67[j][0]));mapping_cases++;}bad_mapping=~0u;}
for(unsigned seed=0;seed<256;seed++)for(unsigned j=0;j<sizeof sprite_ops67/sizeof*sprite_ops67;j++){
 unsigned pc=sprite_ops67[j][0],root=pc<0x8bd0?0:pc<0x8c24?1:2;CpuState c;setup_s67(&c,seed,root);c.S-=sprite_ops67[j][4];if(!sprite_ops67[j][3]){c.P&=~32;cpu_p_to_mirrors(&c);}CpuState ref=c;before=mem;deadline=5000;uint32_t landing;assert(LagoonSprites67_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing));assert(!active_context);cpu_mirrors_to_p(&c);actual=mem;mem=before;Interp816*i=interp816_init(NULL,rd,wr);from(i,&ref,pc);opcode(i,&ref);equal(&c,&ref,i,landing);cases++;interp816_free(i);
}
/* Full real parent entry-to-return with synthetic WRAM streams. The ORIGINAL
 * utility and Cell/OAM children, JSR/JSL/RTS/RTL execute; never fake a child.
 * This is a source service fixture, not game-content or campaign evidence. */
for(unsigned root=0;root<3;root++)for(unsigned variant=0;variant<5;variant++){
 CpuState c;setup_s67(&c,variant,root);memset(ram+0x200,0,0x200);c.A=0;c.Y=0;ram[0x3000]=0;ram[0x3001]=0x31;ram[0x27f]=16;ram[0x281]=16;ram[0x2a1]=ram[0x2a2]=0;ram[0x29e]=0;ram[0x236]=variant==4?128:0;ram[0x284]=2;ram[0x285]=2;ram[0x2e]=0;
 for(unsigned j=0;j<32;j++)ram[0x3100+j]=j+1;
 if(root==0){ram[0x3100]=0x11;ram[0x3101]=variant==2?0x80:0;ram[0x3102]=1;ram[0x3103]=2;if(variant==2){ram[0x3104]=0x11;ram[0x3105]=3;ram[0x3106]=4;}if(variant==3)ram[0x3102]=ram[0x3103]=255;if(variant==0)c.A=255;}
 else if(root==2){ram[0x3100]=2;ram[0x3101]=2;}
 before=mem;CpuState ref=c;unsigned pc=roots_s67[root],target=root==0?0x1c283:root==1?0x8bb3:0x1c155;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,pc);deadline=128;unsigned steps=0;
 while(pc!=target&&steps++<30000){if(c.master_cycles>=deadline)deadline=c.master_cycles+128;retire_landing_s67(&c,7,pc);uint32_t landing;if(interested_s67(pc)&&LagoonSprites67_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing)){pc=landing;from(i,&c,pc);}else{opcode(i,&c);pc=((unsigned)i->k<<16)|i->pc;}}
 if(pc!=target){fprintf(stderr,"full source root%u variant%u stopped%06x\n",root,variant,pc);return 3;}assert(!active_context);cpu_mirrors_to_p(&c);actual=mem;mem=before;from(i,&ref,roots_s67[root]);steps=0;while((((unsigned)i->k<<16)|i->pc)!=target&&steps++<30000)opcode(i,&ref);equal(&c,&ref,i,target);cases++;interp816_free(i);
}
/* Cell stream FF FF takes ORIGINAL8C34 BEQ8C23. That shared RTS returns
 * from the deeper child to8BF5; only the final outer8C23 retires row ownership.
 * Require every selected scalar, including the tail, to stay native while
 * comparing the complete real JSR/RTS service with original interpretation. */
for(unsigned bank=0;bank<3;bank++){
 CpuState c;setup_s67(&c,bank,1);memset(ram+0x200,0,0x200);c.A=0;c.Y=0;
 ram[0x284]=ram[0x285]=2;memset(ram+0x3100,255,128);
 before=mem;CpuState ref=c;unsigned owner_s=c.S,pc=0x8bd0,steps=0,child_returns=0,outer_returns=0,native_tail=0;
 Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,pc);deadline=128;
 while(pc!=0x8bb3&&steps++<30000){
  if(c.master_cycles>=deadline)deadline=c.master_cycles+128;
  int shared=pc==0x8c23,child=shared&&c.S<owner_s;uint32_t landing;
  if(shared)assert(sprite_owners67[1].armed);
  int accelerated=interested_s67(pc)&&LagoonSprites67_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing);
  if(shared){assert(!accelerated);if(child){assert(sprite_owners67[1].armed);child_returns++;}else{assert(c.S==owner_s&&!sprite_owners67[1].armed);outer_returns++;}}
  for(unsigned j=0;j<sizeof sprite_ops67/sizeof*sprite_ops67;j++)if(sprite_ops67[j][0]==pc){assert(accelerated);if(pc>=0x8bf5&&pc<=0x8c21)native_tail++;}
  if(accelerated){pc=landing;from(i,&c,pc);}else{opcode(i,&c);pc=((unsigned)i->k<<16)|i->pc;}
  if(child)assert(pc==0x8bf5);
 }
 assert(pc==0x8bb3&&child_returns&&outer_returns==1&&native_tail&&!sprite_owners67[1].armed);
 assert(!active_context);cpu_mirrors_to_p(&c);actual=mem;mem=before;from(i,&ref,0x8bd0);steps=0;
 while(i->pc!=0x8bb3&&steps++<30000)opcode(i,&ref);
 equal(&c,&ref,i,0x8bb3);cases++;interp816_free(i);
}
for(unsigned fault=0;fault<13;fault++){CpuState c;setup_s67(&c,0,1);switch(fault){case 0:c.D=1;break;case 1:c.S--;break;case 2:c.P|=8;cpu_p_to_mirrors(&c);break;case 3:ram[c.S+1]^=1;break;case 4:c.DB=3;break;case 5:c.emulation=1;break;case 6:c.P|=16;cpu_p_to_mirrors(&c);break;case 7:c.P&=~4;cpu_p_to_mirrors(&c);break;case 8:rom[0xb4b]^=1;break;case 9:sprite_owners67[1].gen++;break;case 10:sprite_owners67[1].cpu=0;break;case 11:sprite_owners67[1].ram=0;break;case 12:sprite_owners67[1].armed=0;break;}CpuState old=c;before=mem;uint32_t landing;assert(!LagoonSprites67_Process(rom,sizeof rom,&c,&c,7,7,0x8bd2,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));if(fault==8)rom[0xb4b]^=1;refusals++;}

/* Plausible restored frames are not entry authority. Explicit source returns,
 * caller landings and non-owner field suspension retire same-generation state. */
for(unsigned root=0;root<3;root++){
 for(unsigned kind=0;kind<5;kind++){
  CpuState c;setup_s67(&c,0,root);uint32_t landing;unsigned pc=roots_s67[root];
  if(kind==0){sprite_owners67[root].armed=0;}
  if(kind==1){sprite_owners67[root].fresh=0;}
  if(kind==2){unsigned ret=root==0?0x8bcf:root==1?0x8c23:0x8d0a;assert(!LagoonSprites67_Process(rom,sizeof rom,&c,&c,7,7,ret,&landing));}
  if(kind==3){c.S+=root==1?2:3;retire_landing_s67(&c,7,root==0?0x1c283:root==1?0x8bb3:0x1c155);c.S-=root==1?2:3;}
  if(kind==4)retain_suspended_s67(&c,7,0x0180af);
  CpuState old=c;before=mem;assert(!LagoonSprites67_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));refusals++;
 }
 unsigned caller=root==0?0x1c27f:root==1?0x8bb0:0x1c151,bytes=root==1?3:4;
 for(unsigned k=0;k<bytes;k++){CpuState c;setup_s67(&c,0,root);sprite_owners67[root].armed=0;bad_mapping=caller+k;assert(!frame_s67(rom,&c,root));assert(!arm_s67(rom,&c,7,root));bad_mapping=~0u;mapping_cases++;}
}
printf("PASS sprites67: %u full-state original opcode/service cases, %u mapping refusals, %u immutable source/owner/mode refusals\n",cases,mapping_cases,refusals);return 0;}
