/* Reuse only the existing original CPU and byte-bus fixture, not its matrix. */
#define interp_bridge_interrupt_source_read8 fixture_irq_read68
#define interp_bridge_interrupt_source_write8 fixture_irq_write68
#define main fixture66_unused_main
#include "lagoon_native_interrupt66_test.c"
#undef main
#undef interp_bridge_interrupt_source_read8
#undef interp_bridge_interrupt_source_write8
bool interp_bridge_interrupt_source_read8(CpuState*,uint32_t,uint8_t*);
bool interp_bridge_interrupt_source_write8(CpuState*,uint32_t,uint8_t);
#define LAGOON_NMI_PREP68_TEST 1
#include "../src/lagoon_native_nmi_prep68.c"
static const CpuWramReadContext*prep_context68;
const CpuWramReadContext*cpu_swap_wram_read_context(const CpuWramReadContext*c){const CpuWramReadContext*old=prep_context68;prep_context68=c;return old;}
bool interp_bridge_interrupt_source_read8(CpuState*c,uint32_t a,uint8_t*out){assert(prep_context68&&prep_context68->pb==0&&prep_context68->m==1&&prep_context68->x==0&&prep_context68->interpreter==1);return fixture_irq_read68(c,a,out);}
bool interp_bridge_interrupt_source_write8(CpuState*c,uint32_t a,uint8_t v){assert(prep_context68&&prep_context68->pb==0&&prep_context68->m==1&&prep_context68->x==0&&prep_context68->interpreter==1);return fixture_irq_write68(c,a,v);}
static void prep_init68(CpuState*c,unsigned seed){memset(&nmi_prep68,0,sizeof nmi_prep68);memset(&mem,0,sizeof mem);for(unsigned j=0;j<sizeof mem.ram;j++)ram[j]=j*73+seed*17;memset(c,0,sizeof*c);c->ram=ram;c->S=0x1f0;c->DB=seed%3;c->A=seed*911;c->X=seed*2017;c->Y=seed*97;c->P=((seed&1)?32:0)|((seed&2)?1:0)|((seed&4)?2:0)|((seed&8)?64:0)|((seed&16)?128:0);cpu_p_to_mirrors(c);cpu_push_interrupt_frame_at(c,0x12da99);c->P|=4;cpu_p_to_mirrors(c);c->PB=0;c->cycles=c->master_cycles=c->coprocessor_master_cycles=0;read_master=release_at=0;mem.io[0x4210]=128;mem.io[0x4212]=0;mem.writes=0;ram[0x23a]=seed;ram[0x238]=seed;ram[0x239]=seed^128;}
static void prep_enter68(CpuState*c){uint32_t landing;unsigned stack=c->S;CpuState old=*c;before=mem;assert(!LagoonNmiPrep68_Process(rom,sizeof rom,c,c,7,7,0x87b4,0x87b4,stack,&landing));assert(nmi_prep68.armed&&!memcmp(c,&old,sizeof old)&&!memcmp(&mem,&before,sizeof mem));Interp816*i=interp816_init(NULL,rd,wr);from(i,c,0x87b4);while(i->pc!=0x87c3)opcode(i,c);interp816_free(i);assert(!memcmp(nmi_prep68.registers,ram+stack-nmi_prep68.saved+1,nmi_prep68.saved));}
static void prep_one68(CpuState*c,unsigned pc){CpuState ref=*c;before=mem;uint32_t landing;assert(LagoonNmiPrep68_Process(rom,sizeof rom,c,c,7,7,pc,0x87b4,nmi_prep68.s,&landing));actual=mem;mem=before;Interp816*i=interp816_init(NULL,rd,wr);from(i,&ref,pc);opcode(i,&ref);equal(c,&ref,i,landing);mem=actual;interp816_free(i);cases++;}
static int prep_interested68(unsigned pc){if(pc==0x87b4||pc==0x88c3)return 1;for(unsigned j=0;j<sizeof nmi_prep_ops68/sizeof*nmi_prep_ops68;j++)if(nmi_prep_ops68[j][0]==pc)return 1;return 0;}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonNmiPrep68_Source(rom,sizeof rom));
/* Each original opcode at the exact saved-register and temporary-PHA depth.
 * The real hardware frame and original complete prologue create ownership. */
for(unsigned seed=0;seed<256;seed++)for(unsigned j=0;j<sizeof nmi_prep_ops68/sizeof*nmi_prep_ops68;j++){
 CpuState c;prep_init68(&c,seed);prep_enter68(&c);const unsigned*op=nmi_prep_ops68[j];
 if(op[3]){Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,0x87d0);nmi_prep68.temp=c.A&255;nmi_prep68.temporary=1;opcode(i,&c);interp816_free(i);}
 nmi_prep68.next=op[0];prep_one68(&c,op[0]);
}
/* All256 request masks run the complete original NMI. Real original88C3 and
 * any nested trigger instructions execute in the CPU oracle with the fixture
 * byte bus; this proves CPU/bus parity, NOT physical DMA/device stall timing.
 * Real PPU/DMA effects and complete sound phases are checked by live pairs. */
for(unsigned seed=0;seed<256;seed++){
 CpuState c;prep_init68(&c,seed);unsigned stack=c.S,pc=0x87b4,steps=0;CpuState ref=c;before=mem;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,pc);unsigned native=0,trigger=0;
 while(pc!=0x12da99&&steps++<500000){uint32_t landing;int accepted=0;if(prep_interested68(pc)){CpuState old=c;if(pc==0x88c3)trigger++;accepted=LagoonNmiPrep68_Process(rom,sizeof rom,&c,&c,7,7,pc,0x87b4,stack,&landing);if(pc==0x88c3)assert(!accepted&&!memcmp(&old,&c,sizeof old));}if(accepted){native++;pc=landing;from(i,&c,pc);}else{opcode(i,&c);pc=((unsigned)i->k<<16)|i->pc;}}
 assert(pc==0x12da99&&!nmi_prep68.armed&&native>=9);if(seed&0xf0)assert(trigger==1);actual=mem;mem=before;from(i,&ref,0x87b4);steps=0;while((((unsigned)i->k<<16)|i->pc)!=0x12da99&&steps++<500000)opcode(i,&ref);equal(&c,&ref,i,pc);interp816_free(i);cases++;
}
/* Guard mutations at a real body entry, saved byte and hardware frame. */
for(unsigned fault=0;fault<20;fault++){
 CpuState c;prep_init68(&c,1);prep_enter68(&c);unsigned stack=nmi_prep68.s,owner=0x87b4;uint64_t gen=7,frame=7;const CpuState*primary=&c;uint32_t landing;nmi_prep68.next=0x87fd;
 switch(fault){case 0:c.S--;break;case 1:ram[stack+1]^=1;break;case 2:c.DB++;break;case 3:gen++;break;case 4:frame++;break;case 5:primary=NULL;break;case 6:c.D=1;break;case 7:c.emulation=1;break;case 8:c.P|=16;cpu_p_to_mirrors(&c);break;case 9:c.P|=8;cpu_p_to_mirrors(&c);break;case 10:c.P&=~4;cpu_p_to_mirrors(&c);break;case 11:owner++;break;case 12:stack++;break;case 13:rom[0x8c3]^=1;break;case 14:nmi_prep68.cpu=NULL;break;case 15:nmi_prep68.next++;break;case 16:c.P&=~32;cpu_p_to_mirrors(&c);break;case 17:ram[c.S+1]^=1;break;case 18:nmi_prep68.ram=NULL;break;case 19:nmi_prep68.armed=0;break;}
 CpuState old=c;before=mem;assert(!LagoonNmiPrep68_Process(rom,sizeof rom,&c,primary,gen,frame,0x87fd,owner,stack,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));if(fault==13)rom[0x8c3]^=1;refusals++;
}
/* Temporary savedA cannot be corrupted, reused at the wrong depth, or borrowed
 * without witnessing the actual original PHA callback. */
for(unsigned fault=0;fault<3;fault++){CpuState c;prep_init68(&c,1);prep_enter68(&c);nmi_prep68.next=0x87d0;uint32_t landing;assert(LagoonNmiPrep68_Process(rom,sizeof rom,&c,&c,7,7,0x87d0,0x87b4,nmi_prep68.s,&landing));if(fault==0)ram[c.S+1]^=1;if(fault==1)nmi_prep68.temporary=0;if(fault==2)c.S++;CpuState old=c;before=mem;assert(!LagoonNmiPrep68_Process(rom,sizeof rom,&c,&c,7,7,0x87d1,0x87b4,nmi_prep68.s,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));refusals++;}
for(unsigned j=0;j<sizeof nmi_prep_ops68/sizeof*nmi_prep_ops68;j++){unsigned pc=nmi_prep_ops68[j][0];assert(nmi_prep_mapped68(pc));for(unsigned k=0;k<nmi_prep_ops68[j][1];k++){bad_mapping=pc+k;assert(!nmi_prep_mapped68(pc));mapping_cases++;}bad_mapping=~0u;}
for(unsigned k=0;k<374;k++){bad_mapping=0x87b4+k;assert(!nmi_prep_mapped68(0x87b4));mapping_cases++;}bad_mapping=~0u;for(unsigned k=0;k<3;k++){bad_mapping=0x88c3+k;assert(!nmi_prep_mapped68(0x88c3));mapping_cases++;}bad_mapping=~0u;assert(!nmi_prep_mapped68(0x88c9));
printf("PASS nmi-prep68: %u original opcode/real prologue/full NMI request-mask CPU-bus cases; %u mapping refusals; %u immutable owner/frame/mode refusals; actual DMA triggers unchanged\n",cases,mapping_cases,refusals);return 0;}
