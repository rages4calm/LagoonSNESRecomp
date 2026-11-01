#define LAGOON_SCHEDULER67_TEST 1
#include "../src/lagoon_native_scheduler67.c"
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


bool interp_bridge_source_read8(CpuState*c,uint32_t address,uint8_t*out){assert(active_context&&active_context->instruction_pc==0x0180b2&&active_context->stack==c->S&&active_context->y==c->Y&&active_context->interpreter==1);*out=cpu_read8(c,address>>16,address);return true;}
bool interp_bridge_source_read16(CpuState*c,uint32_t l,uint32_t h,uint16_t*out){unsigned v=cpu_read8(c,l>>16,l);*out=v|cpu_read8(c,h>>16,h)<<8;return true;}
bool interp_bridge_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_source_write16(CpuState*c,uint32_t l,uint32_t h,uint16_t v){cpu_write8(c,l>>16,l,v);cpu_write8(c,h>>16,h,v>>8);return true;}



bool interp_bridge_interrupt_source_read8(CpuState*c,uint32_t a,uint8_t*out){*out=cpu_read8(c,a>>16,a);return true;}
bool interp_bridge_interrupt_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_commit_interrupt_source_opcode(CpuState*c,uint64_t prior,bool wrote){return interp_bridge_commit_source_opcode(c,prior,wrote);}
static void equal(const CpuState*a,const CpuState*b,const Interp816*i,unsigned pc){
 if(memcmp(&mem,&actual,sizeof mem)||i->pc!=(pc&65535)||i->k!=(pc>>16)||a->A!=b->A||a->X!=b->X||a->Y!=b->Y||a->S!=b->S||a->D!=b->D||a->DB!=b->DB||a->P!=b->P||a->open_bus!=b->open_bus||a->cycles!=b->cycles||a->master_cycles!=b->master_cycles||a->coprocessor_master_cycles!=b->coprocessor_master_cycles){fprintf(stderr,"FAIL case%u pc=%06x/%02x%04x A=%04x/%04x X=%04x/%04x Y=%04x/%04x S=%04x/%04x P=%02x/%02x bus=%02x/%02x cyc=%llu/%llu clocks=%llu/%llu memory=%d writes=%llu/%llu\n",cases,pc,i->k,i->pc,a->A,b->A,a->X,b->X,a->Y,b->Y,a->S,b->S,a->P,b->P,a->open_bus,b->open_bus,(unsigned long long)a->cycles,(unsigned long long)b->cycles,(unsigned long long)a->master_cycles,(unsigned long long)b->master_cycles,memcmp(&mem,&actual,sizeof mem),(unsigned long long)mem.writes,(unsigned long long)actual.writes);exit(2);}}

static int interested67(unsigned pc){if(pc==0x01807a||pc==0x0180cd||(pc>=0x0180bb&&pc<=0x0180ca&&(pc-0x0180bb)%3==0))return 1;for(unsigned j=0;j<sizeof ops67/sizeof*ops67;j++)if(ops67[j][0]==pc)return 1;return 0;}
static void setup67(CpuState*c,unsigned seed,int init){
 memset(&scheduler67,0,sizeof scheduler67);memset(&init67,0,sizeof init67);memset(&mem,0,sizeof mem);for(unsigned j=0;j<0x20000;j++)ram[j]=j*73+seed*17;
 memset(c,0,sizeof*c);c->ram=ram;c->S=0x1ff;c->DB=init?2:1;c->PB=init?2:1;c->A=seed*911;c->X=seed*2017;c->Y=seed*97;c->P=0x24|((seed&1)?1:0)|((seed&2)?2:0)|((seed&4)?64:0)|((seed&8)?128:0);cpu_p_to_mirrors(c);deadline=0;filter_variant=seed&1;
 if(init){/* REAL original JSR D991 creates the physical D993 frame. */Interp816*i=interp816_init(NULL,rd,wr);from(i,c,0x02d991);opcode(i,c);assert(i->pc==0xd9bb&&c->S==0x1fd);interp816_free(i);}
}
static void one67(CpuState*c,unsigned pc){CpuState ref=*c;before=mem;uint32_t landing=0;assert(LagoonScheduler67_Process(rom,sizeof rom,c,c,7,7,pc,&landing));cpu_mirrors_to_p(c);actual=mem;mem=before;Interp816*i=interp816_init(NULL,rd,wr);from(i,&ref,pc);opcode(i,&ref);equal(c,&ref,i,landing);cases++;interp816_free(i);mem=actual;}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonScheduler67_Source(rom,sizeof rom));
for(unsigned root=0;root<3;root++){unsigned pc=root==0?0x01807a:root==1?0x0180af:0x02d9bb;assert(mapped67(pc));unsigned start=root==2?0x02d9bb:0x01807a,n=root==2?0x52:0x6d;for(unsigned k=0;k<n;k++){bad_mapping=start+k;assert(!mapped67(pc));mapping_cases++;}if(root==2)for(unsigned k=0;k<3;k++){bad_mapping=0x02d991+k;assert(!mapped67(pc));mapping_cases++;}bad_mapping=~0u;}
for(unsigned j=0;j<sizeof ops67/sizeof*ops67;j++){unsigned pc=ops67[j][0];assert(mapped67(pc));for(unsigned k=0;k<ops67[j][1];k++){bad_mapping=pc+k;assert(!mapped67(pc));mapping_cases++;}bad_mapping=~0u;}assert(!mapped67(0x02da0c));
/* Complete constructor, original JSR and RTS, field deadlines and QoL writes. */
for(unsigned seed=0;seed<32;seed++){
 CpuState c;setup67(&c,seed,1);before=mem;CpuState ref=c;unsigned pc=0x02d9bb;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,pc);unsigned steps=0;deadline=128;
 while(pc!=0x02d994&&steps++<18000){if(c.master_cycles>=deadline)deadline=c.master_cycles+128;uint32_t landing;if(interested67(pc)&&LagoonScheduler67_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing)){pc=landing;from(i,&c,pc);}else{opcode(i,&c);pc=((unsigned)i->k<<16)|i->pc;}}
 assert(pc==0x02d994&&c.S==0x1ff);assert(ram[0x520]==(filter_variant?18:10)&&ram[0x526]==(filter_variant?11:5));actual=mem;mem=before;from(i,&ref,0x02d9bb);steps=0;while(i->pc!=0xd994&&steps++<18000)opcode(i,&ref);equal(&c,&ref,i,pc);cases++;interp816_free(i);
}
/* Every inline scheduler opcode, all33 actual record indices, active/inactive
 * flag values, both carry inputs and full accumulator high-byte retention. */
for(unsigned seed=0;seed<256;seed++)for(unsigned op=0;op<14;op++)for(unsigned record=0;record<33;record++){
 CpuState c;setup67(&c,seed,0);unsigned pc=ops67[op][0];assert(arm67(&scheduler67,&c,7,0x0180af));scheduler67.next=pc;c.X=record*64;scheduler67.x=c.X;c.P=(c.P&~32)|(ops67[op][3]?32:0);cpu_p_to_mirrors(&c);one67(&c,pc);
}
/* Walk all33 inactive records using native inline opcodes and REAL JSL/RTL
 * for0281DE. Its original implementation runs; no callee result is faked. */
for(unsigned seed=0;seed<4;seed++){
 CpuState c;setup67(&c,seed,0);for(unsigned x=0;x<=0x800;x+=64)ram[0x500+x]=0;before=mem;CpuState ref=c;unsigned pc=0x0180af;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,pc);unsigned steps=0;deadline=256;
 while(pc!=0x0180e3&&steps++<100000){if(c.master_cycles>=deadline)deadline=c.master_cycles+256;uint32_t landing;if(interested67(pc)&&LagoonScheduler67_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing)){pc=landing;from(i,&c,pc);}else{opcode(i,&c);pc=((unsigned)i->k<<16)|i->pc;}}
 assert(pc==0x0180e3&&c.X==0x840&&c.S==0x1ff);actual=mem;mem=before;from(i,&ref,0x0180af);steps=0;while(i->pc!=0x80e3&&steps++<100000)opcode(i,&ref);equal(&c,&ref,i,pc);cases++;interp816_free(i);
}
for(unsigned fault=0;fault<17;fault++){CpuState c;setup67(&c,0,0);assert(arm67(&scheduler67,&c,7,0x0180af));scheduler67.next=0x0180b2;c.X=0;scheduler67.x=0;uint64_t gen=7,frame=7;const CpuState*primary=&c;switch(fault){case 0:c.P|=8;cpu_p_to_mirrors(&c);break;case 1:c.D=1;break;case 2:c.S--;break;case 3:c.DB=2;break;case 4:c.P|=16;cpu_p_to_mirrors(&c);break;case 5:c.emulation=1;break;case 6:c.PB=2;break;case 7:rom[0x80af]^=1;break;case 8:scheduler67.gen++;break;case 9:scheduler67.cpu=0;break;case 10:c.X=64;break;case 11:scheduler67.next++;break;case 12:gen++;break;case 13:frame++;break;case 14:primary=0;break;case 15:c.P&=~4;cpu_p_to_mirrors(&c);break;case 16:deadline=c.master_cycles=500;break;}CpuState old=c;before=mem;uint32_t landing;assert(!LagoonScheduler67_Process(rom,sizeof rom,&c,primary,gen,frame,0x0180b2,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));if(fault==7)rom[0x80af]^=1;refusals++;}
/* Real active actor selector4 reaches its ORIGINAL8201 RTS. The inline
 * branch and actual80BB JSR/8184 dispatch/RTS must preserve caller ownership. */
for(unsigned seed=0;seed<16;seed++)for(unsigned record=0;record<33;record++){
 CpuState c;setup67(&c,seed,0);c.A&=255;c.X=record*64;ram[0x500+c.X]=0xc0;ram[0x518+c.X]=4;assert(arm67(&scheduler67,&c,7,0x0180af));scheduler67.next=0x0180b2;scheduler67.x=c.X;before=mem;CpuState ref=c;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,0x0180b2);unsigned pc=0x0180b2,steps=0;
 while(pc!=0x0180be&&steps++<100){uint32_t landing;if(interested67(pc)&&LagoonScheduler67_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing)){pc=landing;from(i,&c,pc);}else{opcode(i,&c);pc=((unsigned)i->k<<16)|i->pc;}}
 assert(pc==0x0180be&&c.S==0x1ff&&c.X==record*64);actual=mem;mem=before;from(i,&ref,0x0180b2);steps=0;while(i->pc!=0x80be&&steps++<100)opcode(i,&ref);equal(&c,&ref,i,pc);uint32_t landing;assert(!LagoonScheduler67_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing));assert(scheduler67.armed&&scheduler67.next==0x0180c1);cases++;interp816_free(i);
}
/* Constructor frame/source/mode/sequence/generation refusals stay immutable. */
for(unsigned fault=0;fault<12;fault++){CpuState c;setup67(&c,0,1);uint32_t landing;assert(LagoonScheduler67_Process(rom,sizeof rom,&c,&c,7,7,0x02d9bb,&landing));switch(fault){case 0:ram[init67.s+1]^=1;break;case 1:ram[init67.s+2]^=1;break;case 2:c.X++;break;case 3:c.S--;break;case 4:c.DB=3;break;case 5:init67.gen++;break;case 6:init67.ram=0;break;case 7:c.P&=~32;cpu_p_to_mirrors(&c);break;case 8:c.P&=~4;cpu_p_to_mirrors(&c);break;case 9:rom[0x15a0c]^=1;break;case 10:init67.next++;break;case 11:deadline=c.master_cycles;break;}CpuState old=c;before=mem;assert(!LagoonScheduler67_Process(rom,sizeof rom,&c,&c,7,7,0x02d9be,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));if(fault==9)rom[0x15a0c]^=1;refusals++;}
/* A genuine active branch lends authority to original children only in order;
 * a changed physical frame/X or missing JSR boundary invalidates the owner. */
for(unsigned fault=0;fault<4;fault++){CpuState c;setup67(&c,0,0);c.X=0;assert(arm67(&scheduler67,&c,7,0x0180af));scheduler67.x=0;scheduler67.next=0x0180bb;uint32_t landing;if(fault==0)c.X=64;if(fault==1)c.S-=2;if(fault==2)scheduler67.next=0x0180ca;if(fault==3)c.P&=~32;cpu_p_to_mirrors(&c);CpuState old=c;before=mem;assert(!LagoonScheduler67_Process(rom,sizeof rom,&c,&c,7,7,0x0180bb,&landing));assert(!scheduler67.armed&&!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));refusals++;}
/* Pure observation of real807A uses inherited01FF without borrowing0200 globals. */
{CpuState c;setup67(&c,0,0);uint32_t landing;CpuState old=c;assert(!LagoonScheduler67_Process(rom,sizeof rom,&c,&c,7,7,0x01807a,&landing));assert(scheduler67.armed&&scheduler67.count==0&&!memcmp(&c,&old,sizeof c));ram[0x200]^=1;assert(LagoonScheduler67_Process(rom,sizeof rom,&c,&c,7,7,0x0180af,&landing));}
/* Full source identities include all approved camera caller bytes only. */
rom[0x809d]=rom[0x809e]=rom[0x809f]=0xea;rom[0x80cb]=0xd8;rom[0x80cc]=0x83;assert(LagoonScheduler67_Source(rom,sizeof rom));rom[0x80cb]^=1;assert(!LagoonScheduler67_Source(rom,sizeof rom));
printf("PASS %u exact source/owner/current-opcode mapping refusals\n",mapping_cases);
printf("PASS scheduler67: %u full-state original opcodes/constructor/real-call routes; %u immutable refusals\n",cases,refusals);return 0;}
