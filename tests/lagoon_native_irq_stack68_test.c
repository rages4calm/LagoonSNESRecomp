/* Reuse the existing byte-bus/original CPU oracle fixture, not its old matrix. */
#define main fixture66_unused_main
#include "lagoon_native_interrupt66_test.c"
#undef main
#define LAGOON_IRQ_STACK68_TEST 1
#include "../src/lagoon_native_irq_stack68.c"
static const CpuWramReadContext* irq_context68;
const CpuWramReadContext*cpu_swap_wram_read_context(const CpuWramReadContext*c){const CpuWramReadContext*old=irq_context68;irq_context68=c;return old;}
static void init68(CpuState*c,unsigned seed){
 memset(&irq68,0,sizeof irq68);memset(&mem,0,sizeof mem);for(unsigned j=0;j<sizeof mem.ram;j++)ram[j]=j*73+seed*17;
 memset(c,0,sizeof*c);c->ram=ram;c->S=0x1f0;c->DB=seed%3;c->A=seed*911;c->X=seed*2017;c->Y=seed*97;c->P=((seed&1)?32:0)|((seed&2)?1:0)|((seed&4)?2:0)|((seed&8)?64:0)|((seed&16)?128:0);cpu_p_to_mirrors(c);
 cpu_push_interrupt_frame_at(c,0x12da99);c->P|=4;cpu_p_to_mirrors(c);c->PB=0;
 c->cycles=c->master_cycles=c->coprocessor_master_cycles=0;read_master=0;release_at=0;mem.io[0x4210]=128;mem.io[0x4212]=0;mem.writes=0;
}
static unsigned advance68(CpuState*c,Interp816*i,unsigned pc,unsigned stack,int use_native){uint32_t landing=0;if(use_native&&LagoonIrqStack68_Process(rom,sizeof rom,c,c,7,7,pc,0x87b4,stack,&landing)){from(i,c,landing);return landing;}opcode(i,c);return ((unsigned)i->k<<16)|i->pc;}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonIrqStack68_Source(rom,sizeof rom));unsigned count=0,blocked=0,mapping=0;
 for(unsigned seed=0;seed<256;seed++){
  CpuState c;init68(&c,seed);unsigned stack=c.S,pc=0x87b4;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,pc);
  while(pc!=0x87c3){CpuState ref=c;before=mem;uint32_t landing;assert(LagoonIrqStack68_Process(rom,sizeof rom,&c,&c,7,7,pc,0x87b4,stack,&landing));actual=mem;mem=before;from(i,&ref,pc);opcode(i,&ref);equal(&c,&ref,i,landing);mem=actual;from(i,&c,landing);pc=landing;count++;}
  /* Actual saved bytes from the full prologue, real PLP/restores and RTI. */
  pc=0x8925;from(i,&c,pc);
  while(pc!=0x8929){CpuState ref=c;before=mem;uint32_t landing;assert(LagoonIrqStack68_Process(rom,sizeof rom,&c,&c,7,7,pc,0x87b4,stack,&landing));actual=mem;mem=before;from(i,&ref,pc);opcode(i,&ref);equal(&c,&ref,i,landing);mem=actual;from(i,&c,landing);pc=landing;count++;}
  assert(!irq68.armed);opcode(i,&c);assert(i->pc==0xda99&&i->k==0x12);count++;interp816_free(i);
 }
 /* Whole real original NMI, including its body, real children and final RTI. */
 for(unsigned seed=0;seed<24;seed++){
  CpuState c;init68(&c,seed);CpuState ref=c;unsigned stack=c.S,pc=0x87b4,steps=0;before=mem;Interp816*i=interp816_init(NULL,rd,wr);from(i,&c,pc);
  while(pc!=0x12da99&&steps++<500000)pc=advance68(&c,i,pc,stack,1);
  assert(pc==0x12da99);actual=mem;mem=before;from(i,&ref,0x87b4);steps=0;while((((unsigned)i->k<<16)|i->pc)!=0x12da99&&steps++<500000)opcode(i,&ref);equal(&c,&ref,i,pc);count++;interp816_free(i);
 }
 for(unsigned fault=0;fault<17;fault++){
  CpuState c;init68(&c,1);unsigned stack=c.S;uint32_t landing;assert(LagoonIrqStack68_Process(rom,sizeof rom,&c,&c,7,7,0x87b4,0x87b4,stack,&landing));unsigned owner=0x87b4;uint64_t gen=7,frame=7;const CpuState*primary=&c;
  switch(fault){case 0:c.S--;break;case 1:ram[stack+1]^=1;break;case 2:c.DB++;break;case 3:gen++;break;case 4:frame++;break;case 5:primary=NULL;break;case 6:c.D=1;break;case 7:c.emulation=1;break;case 8:c.P|=16;cpu_p_to_mirrors(&c);break;case 9:c.P|=8;cpu_p_to_mirrors(&c);break;case 10:c.P&=~4;cpu_p_to_mirrors(&c);break;case 11:owner++;break;case 12:stack++;break;case 13:rom[0x7b4]^=1;break;case 14:irq68.cpu=NULL;break;case 15:irq68.next++;break;case 16:c.P^=32;cpu_p_to_mirrors(&c);break;}
  CpuState old=c;before=mem;assert(!LagoonIrqStack68_Process(rom,sizeof rom,&c,primary,gen,frame,0x87b5,owner,stack,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));if(fault==13)rom[0x7b4]^=1;blocked++;
 }
 /* Each saved register byte and architectural frame byte is immutable when
  * entering the epilogue, for BOTH actual accumulator save widths. */
 for(unsigned m=0;m<2;m++)for(unsigned fault=0;fault<(m?10u:11u);fault++){
  CpuState c;init68(&c,m);unsigned stack=c.S,pc=0x87b4;uint32_t landing;
  while(pc!=0x87c3){assert(LagoonIrqStack68_Process(rom,sizeof rom,&c,&c,7,7,pc,0x87b4,stack,&landing));pc=landing;}
  ram[c.S+1+fault]^=1;CpuState old=c;before=mem;assert(!LagoonIrqStack68_Process(rom,sizeof rom,&c,&c,7,7,0x8925,0x87b4,stack,&landing));assert(!memcmp(&c,&old,sizeof c)&&!memcmp(&mem,&before,sizeof mem));blocked++;
 }
 for(unsigned j=0;j<sizeof irq_ops68/sizeof*irq_ops68;j++){unsigned pc=irq_ops68[j][0];assert(irq_mapped68(pc));for(unsigned k=0;k<irq_ops68[j][1];k++){bad_mapping=pc+k;assert(!irq_mapped68(pc));mapping++;}bad_mapping=~0u;}
 for(unsigned p=0x87b4;p<0x87c3;p++){bad_mapping=p;assert(!irq_mapped68(0x87b4));mapping++;}for(unsigned p=0x8925;p<=0x8929;p++){bad_mapping=p;assert(!irq_mapped68(0x87b4));mapping++;}bad_mapping=~0u;
 printf("PASS irq-stack68: %u original prologue/restores/real RTI/full NMI cases; %u immutable refusals; %u mapping refusals\n",count,blocked,mapping);return 0;
}
