#define LAGOON_TILES68_TEST 1
#include "../src/lagoon_native_tiles68.c"
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
int cpu_aot_rom_mapping_matches(uint32_t pc,uint32_t offset,unsigned n){assert(n&&n<=4&&offset==tile_offset68(pc));return !(bad_mapping>=pc&&bad_mapping<pc+n);}
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


bool interp_bridge_source_read8(CpuState*c,uint32_t address,uint8_t*out){assert(active_context&&active_context->instruction_pc>=0x0189f5&&active_context->instruction_pc<=0x018b2d&&active_context->stack==c->S&&active_context->y==c->Y&&active_context->interpreter==1);*out=cpu_read8(c,address>>16,address);return true;}
bool interp_bridge_source_read16(CpuState*c,uint32_t l,uint32_t h,uint16_t*out){unsigned v=cpu_read8(c,l>>16,l);*out=v|cpu_read8(c,h>>16,h)<<8;return true;}
bool interp_bridge_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_source_write16(CpuState*c,uint32_t l,uint32_t h,uint16_t v){cpu_write8(c,l>>16,l,v);cpu_write8(c,h>>16,h,v>>8);return true;}



bool interp_bridge_interrupt_source_read8(CpuState*c,uint32_t a,uint8_t*out){*out=cpu_read8(c,a>>16,a);return true;}
bool interp_bridge_interrupt_source_write8(CpuState*c,uint32_t a,uint8_t v){cpu_write8(c,a>>16,a,v);return true;}
bool interp_bridge_commit_interrupt_source_opcode(CpuState*c,uint64_t prior,bool wrote){return interp_bridge_commit_source_opcode(c,prior,wrote);}
static void equal(const CpuState*a,const CpuState*b,const Interp816*i,unsigned pc){
 if(memcmp(&mem,&actual,sizeof mem)||i->pc!=(pc&65535)||i->k!=(pc>>16)||a->A!=b->A||a->X!=b->X||a->Y!=b->Y||a->S!=b->S||a->D!=b->D||a->DB!=b->DB||a->P!=b->P||a->open_bus!=b->open_bus||a->cycles!=b->cycles||a->master_cycles!=b->master_cycles||a->coprocessor_master_cycles!=b->coprocessor_master_cycles){fprintf(stderr,"FAIL case%u pc=%06x/%02x%04x A=%04x/%04x X=%04x/%04x Y=%04x/%04x S=%04x/%04x P=%02x/%02x bus=%02x/%02x cyc=%llu/%llu clocks=%llu/%llu memory=%d writes=%llu/%llu\n",cases,pc,i->k,i->pc,a->A,b->A,a->X,b->X,a->Y,b->Y,a->S,b->S,a->P,b->P,a->open_bus,b->open_bus,(unsigned long long)a->cycles,(unsigned long long)b->cycles,(unsigned long long)a->master_cycles,(unsigned long long)b->master_cycles,memcmp(&mem,&actual,sizeof mem),(unsigned long long)mem.writes,(unsigned long long)actual.writes);exit(2);}}

static int tile_interested68(unsigned pc){for(unsigned j=0;j<sizeof tile_ops68/sizeof*tile_ops68;j++)if(tile_ops68[j][0]==pc)return 1;for(unsigned j=0;j<sizeof tile_calls68/sizeof*tile_calls68;j++)if(tile_calls68[j][0]==pc)return 1;for(unsigned j=0;j<sizeof tile_boundaries68/sizeof*tile_boundaries68;j++)if(tile_boundaries68[j][0]==pc)return 1;return 0;}
static void tile_setup68(CpuState*c,unsigned seed,unsigned root,unsigned observer){
 memset(tile_owner68,0,sizeof tile_owner68);memset(tile_pending68,0,sizeof tile_pending68);memset(&mem,0,sizeof mem);for(unsigned j=0;j<0x20000;j++)ram[j]=j*73+seed*17;memset(c,0,sizeof*c);c->ram=ram;c->S=0x1ff;c->DB=c->PB=1;c->A=seed*257;c->X=seed;c->Y=seed*97;c->P=0x24;cpu_p_to_mirrors(c);deadline=0;filter_variant=0;
 unsigned caller=root==2?0x018741:(seed&1)?0x0187a8:0x01873e;uint32_t landing;Interp816*i=interp816_init(NULL,rd,wr);from(i,c,caller);if(!observer||root==2)assert(!LagoonTiles68_Process(rom,sizeof rom,c,c,7,7,caller,&landing));opcode(i,c);unsigned firstroot=root==2?2:0;assert(((unsigned)i->k<<16|i->pc)==tile_roots68[firstroot]);
 if(observer&&root!=2){c->host_return_valid=2;assert(tile_observe68(rom,sizeof rom,c,c,7,7,c->S,2,caller+3,0));}else assert(tile_take68(c,7,firstroot));
 if(root==1){from(i,c,0x018a10);opcode(i,c);if(!observer)assert(!LagoonTiles68_Process(rom,sizeof rom,c,c,7,7,0x018a2f,&landing));from(i,c,0x018a2f);opcode(i,c);assert(i->pc==0x8a5e);if(observer){c->host_return_valid=2;assert(tile_observe68(rom,sizeof rom,c,c,7,7,c->S,2,0x018a32,1));}else assert(tile_take68(c,7,1));}
 interp816_free(i);
}
static void tile_one68(CpuState*c,unsigned pc){CpuState ref=*c;before=mem;uint32_t landing;static const CpuWramReadContext sentinel={.instruction_pc=0x123456};active_context=&sentinel;assert(LagoonTiles68_Process(rom,sizeof rom,c,c,7,7,pc,&landing));assert(active_context==&sentinel);active_context=0;cpu_mirrors_to_p(c);actual=mem;mem=before;Interp816*i=interp816_init(NULL,rd,wr);from(i,&ref,pc);opcode(i,&ref);equal(c,&ref,i,landing);cases++;interp816_free(i);mem=actual;}
static void tile_route68(CpuState*c,unsigned begin,unsigned end,unsigned limit){before=mem;CpuState ref=*c;unsigned pc=begin;Interp816*i=interp816_init(NULL,rd,wr);from(i,c,pc);unsigned steps=0;while(pc!=end&&steps++<limit){uint32_t landing;if(tile_interested68(pc)&&LagoonTiles68_Process(rom,sizeof rom,c,c,7,7,pc,&landing)){pc=landing;from(i,c,pc);}else{opcode(i,c);pc=(unsigned)i->k<<16|i->pc;}}if(pc!=end){fprintf(stderr,"route timeout %06x target%06x\n",pc,end);exit(2);}actual=mem;mem=before;from(i,&ref,begin);steps=0;while((((unsigned)i->k<<16)|i->pc)!=end&&steps++<limit)opcode(i,&ref);equal(c,&ref,i,pc);cases++;interp816_free(i);mem=actual;}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonTiles68_Source(rom,sizeof rom));
for(unsigned root=0;root<3;root++){unsigned pc=tile_roots68[root];assert(tile_mapped68(pc));for(unsigned span=0;span<sizeof tile_spans68/sizeof*tile_spans68;span++)for(unsigned k=0;k<tile_spans68[span].n;k++){bad_mapping=tile_spans68[span].pc+k;assert(!tile_mapped68(pc));mapping_cases++;}bad_mapping=~0u;}
for(unsigned j=0;j<sizeof tile_ops68/sizeof*tile_ops68;j++)for(unsigned k=0;k<tile_ops68[j][1];k++){bad_mapping=tile_ops68[j][0]+k;assert(!tile_mapped68(tile_ops68[j][0]));mapping_cases++;}bad_mapping=~0u;
for(unsigned j=0;j<sizeof tile_calls68/sizeof*tile_calls68;j++)for(unsigned k=0;k<tile_calls68[j][1];k++){bad_mapping=tile_calls68[j][0]+k;assert(!tile_mapped68(tile_calls68[j][0]));mapping_cases++;}bad_mapping=~0u;
/* Every byte/high A/flag seed and exact original saved-X depth/width. */
for(unsigned seed=0;seed<256;seed++)for(unsigned op=0;op<sizeof tile_ops68/sizeof*tile_ops68;op++){
 unsigned pc=tile_ops68[op][0],root=tile_family68(pc);CpuState c;tile_setup68(&c,seed,root,seed&1);if(tile_ops68[op][4]){Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,0x018a10);opcode(i,&c);interp816_free(i);}c.A=(seed*257)^0xa500;c.X=seed*257;c.Y=seed*97;c.P=(tile_ops68[op][3]?0x24:4)|((seed&1)?1:0)|((seed&2)?2:0)|((seed&4)?64:0)|((seed&8)?128:0);cpu_p_to_mirrors(&c);tile_one68(&c,pc);
}
/* One complete final tile iteration: original coordinate helper008AA4,
 * original8311 tile fetch, actual childJSR8A5E and original queue89EC/RTL,
 * then parent PLX/row tail and actual root RTS. No fake child result. */
for(unsigned seed=0;seed<16;seed++){
 CpuState c;tile_setup68(&c,seed,0,seed&1);memset(ram+0x200,0,0x100);ram[0x50]=seed&1;ram[0x52]=0;ram[0xb8]=0;ram[0xb9]=0;ram[0xba]=0;ram[0xbb]=0;ram[0x88]=0;ram[0x89]=0;ram[0x8a]=0x7f;ram[0x8b]=0;ram[0x8c]=0x40;ram[0x8d]=0x7f;ram[0x69]=ram[0x75]=0;ram[0x6a]=ram[0x76]=0x80;ram[0x6b]=ram[0x77]=4;c.X=33;c.Y=31;unsigned end=(ram[c.S+1]|ram[c.S+2]<<8)+1;tile_route68(&c,0x018a10,0x010000|end,3000);
}
/* Entire 2KiB HUD scalar fill before the genuine upload JSL899B. Actual
 * enclosing JSR8ACD supplies ownership; the fixture begins atAD1, after the
 * preceding hardware-wait child. No child result/upload completion is invented. */
for(unsigned seed=0;seed<8;seed++){CpuState c;tile_setup68(&c,seed,2,0);tile_route68(&c,0x018ad1,0x018af2,20000);assert(c.X==0x800);for(unsigned j=0x3000;j<0x3800;j+=2)assert(ram[j]==255&&ram[j+1]==1);}
for(unsigned fault=0;fault<26;fault++){
 CpuState c;tile_setup68(&c,0,0,0);uint64_t gen=7,frame=7;const CpuState*primary=&c;switch(fault){case 0:ram[tile_owner68[0].s+1]^=1;break;case 1:ram[tile_owner68[0].s+2]^=1;break;case 2:c.S--;break;case 3:c.DB=2;break;case 4:c.PB=2;break;case 5:c.D=1;break;case 6:c.emulation=1;break;case 7:c.P|=16;cpu_p_to_mirrors(&c);break;case 8:c.P&=~32;cpu_p_to_mirrors(&c);break;case 9:c.P|=8;cpu_p_to_mirrors(&c);break;case 10:c.P&=~4;cpu_p_to_mirrors(&c);break;case 11:gen++;break;case 12:frame++;break;case 13:primary=0;break;case 14:tile_owner68[0].gen++;break;case 15:tile_owner68[0].ram=0;break;case 16:deadline=c.master_cycles;break;case 17:rom[0x8af0]^=1;break;case 18:rom[0xaa4]^=1;break;case 19:tile_owner68[0].armed=0;break;case 20:tile_owner68[0].cpu=0;break;case 21:c.ram=0;break;case 22:tile_retain68(&c,7,0x012345);break;case 23:rom[0x873e]^=1;break;case 24:rom[0x8311]^=1;break;case 25:rom[0x8a32]^=1;break;}CpuState old=c;before=mem;uint32_t landing;assert(!LagoonTiles68_Process(rom,sizeof rom,&c,primary,gen,frame,0x0189f8,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));if(fault==17)rom[0x8af0]^=1;if(fault==18)rom[0xaa4]^=1;if(fault==23)rom[0x873e]^=1;if(fault==24)rom[0x8311]^=1;if(fault==25)rom[0x8a32]^=1;refusals++;
}
/* Native observer is pure and rejects forged return metadata or caller map. */
for(unsigned fault=0;fault<17;fault++){
 CpuState c;tile_setup68(&c,0,0,1);tile_owner68[0].armed=0;unsigned s=c.S,h=2,host=0x018741;uint64_t gen=7,frame=7;const CpuState*primary=&c;switch(fault){case 0:h=3;break;case 1:c.host_return_valid=3;break;case 2:s++;break;case 3:host++;break;case 4:ram[s+1]^=1;break;case 5:c.DB=2;break;case 6:c.PB=2;break;case 7:gen++;break;case 8:frame++;break;case 9:primary=0;break;case 10:bad_mapping=0x01873f;break;case 11:bad_mapping=0x0189f6;break;case 12:c.ram=0;break;case 13:c.P|=8;cpu_p_to_mirrors(&c);break;case 14:c.P&=~4;cpu_p_to_mirrors(&c);break;case 15:rom[0x8af0]^=1;break;case 16:c.S++;break;}CpuState old=c;before=mem;assert(!tile_observe68(rom,sizeof rom,&c,primary,gen,frame,s,h,host,0));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));bad_mapping=~0u;if(fault==15)rom[0x8af0]^=1;refusals++;
}
/* Pending calls and fresh entry across source deadlines retain original
 * ownership only at the real destination, with unchanged outer stack/caller. */
for(unsigned root=0;root<3;root++){
 CpuState c;tile_setup68(&c,0,root,0);unsigned pc=root==2?0x018ad1:tile_roots68[root];deadline=c.master_cycles;CpuState old=c;before=mem;uint32_t landing;assert(!LagoonTiles68_Process(rom,sizeof rom,&c,&c,7,7,pc,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));deadline=0;tile_one68(&c,pc);
}
for(unsigned fault=0;fault<6;fault++){
 CpuState c;tile_setup68(&c,0,0,0);memset(tile_owner68,0,sizeof tile_owner68);unsigned ret=ram[c.S+1]|ram[c.S+2]<<8;/* Recreate pending only by a fresh original JSR, never by a fake frame. */
 c.S+=2;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,0x01873e);uint32_t landing;assert(!LagoonTiles68_Process(rom,sizeof rom,&c,&c,7,7,0x01873e,&landing));opcode(i,&c);interp816_free(i);assert((unsigned)(ram[c.S+1]|ram[c.S+2]<<8)==ret);
 switch(fault){case 0:tile_retain68(&c,7,0x0189f5);break;case 1:tile_retain68(&c,7,0x012345);break;case 2:ram[tile_pending68[0].s+4]^=1;break;case 3:bad_mapping=0x01873f;break;case 4:tile_pending68[0].gen++;break;case 5:tile_pending68[0].armed=0;break;}
 if(!fault){tile_one68(&c,0x0189f5);continue;}CpuState old=c;before=mem;assert(!LagoonTiles68_Process(rom,sizeof rom,&c,&c,7,7,0x0189f5,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));bad_mapping=~0u;refusals++;
}
printf("PASS tiles68: %u original opcode/real-call/tile-helper/HUD-fill routes; %u exact mapping refusals; %u immutable ownership/deadline refusals\n",cases,mapping_cases,refusals);return 0;}
