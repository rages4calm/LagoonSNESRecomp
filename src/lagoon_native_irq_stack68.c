/* Only the original NMI register saves/restores. Hardware entry and RTI remain
 * interpreter-owned. The existing66 body observes entry before this callback. */
#include "lagoon_native_irq_stack68.h"
#include "snes/interp_bridge.h"
#include <string.h>
#include <stdlib.h>
typedef struct{const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,db,m,saved,next;uint8_t frame[4],registers[7];int armed;} IrqStack68;
static IrqStack68 irq68;
static struct{uint64_t entries,pushes,pops,setup,refused;} irq_stats68;
static const unsigned irq_ops68[][3]={
 {0x87b4,1,0},{0x87b5,1,4},{0x87b6,1,4},{0x87b7,1,3},
 {0x87b8,2,3},{0x87ba,2,3},{0x87bc,1,3},{0x87bd,2,2},
 {0x87bf,1,3},{0x87c0,1,4},{0x87c1,2,3},
 {0x8925,1,4},{0x8926,1,5},{0x8927,1,5},{0x8928,1,0}};
int LagoonIrqStack68_Source(const uint8_t*r,size_t n){
 static const uint8_t prologue[]={0x48,0xda,0x5a,0x08,0xc2,0x20,0xe2,0x20,0x48,0xa9,0,0xeb,0x68,0xc2,0x10};
 static const uint8_t epilogue[]={0x28,0x7a,0xfa,0x68,0x40};
 return r&&n==0x100000&&!memcmp(r+0x7b4,prologue,sizeof prologue)&&!memcmp(r+0x925,epilogue,sizeof epilogue);
}
static int irq_mode68(const CpuState*c){return c&&c->ram&&!c->PB&&c->DB<=2&&!c->D&&!c->emulation&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x1c)==4&&!!(c->P&32)==!!c->m_flag;}
static unsigned irq_read68(CpuState*c,unsigned p){uint8_t v;if(!interp_bridge_interrupt_source_read8(c,p,&v))abort();return v;}
static void irq_write68(CpuState*c,unsigned p,unsigned v){if(!interp_bridge_interrupt_source_write8(c,p,v))abort();}
static void irq_push68(CpuState*c,unsigned v,unsigned bytes){if(bytes==2){irq_write68(c,c->S,v>>8);c->S--;}irq_write68(c,c->S,v);c->S--;}
static unsigned irq_pull68(CpuState*c,unsigned bytes){unsigned v=irq_read68(c,c->S+1);c->S++;if(bytes==2){v|=irq_read68(c,c->S+1)<<8;c->S++;}return v;}
static void irq_nz68(CpuState*c,unsigned v,unsigned bits){c->_flag_Z=!(v&(bits==8?255:65535));c->_flag_N=!!(v&(1u<<(bits-1)));cpu_mirrors_to_p(c);}
static unsigned irq_depth68(unsigned pc,unsigned m){switch(pc){case 0x87b4:return 0;case 0x87b5:return m?1:2;case 0x87b6:return m?3:4;case 0x87b7:return m?5:6;case 0x87bc:case 0x87b8:case 0x87ba:case 0x87c1:case 0x8925:return m?6:7;case 0x87bd:case 0x87bf:case 0x87c0:return m?7:8;case 0x8926:return m?5:6;case 0x8927:return m?3:4;case 0x8928:return m?1:2;default:return 0;}}
int LagoonIrqStack68_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,uint32_t pc,uint32_t owner,uint16_t stack,uint32_t*landing){
 if(!landing||c!=primary||gen!=frame||!irq_mode68(c)||owner!=0x87b4||!LagoonIrqStack68_Source(r,n)){irq68.armed=0;irq_stats68.refused++;return 0;}
 if(pc==0x87b4){memset(&irq68,0,sizeof irq68);if(c->S!=stack||stack<0x128||stack>0x1fb)return 0;irq68.cpu=c;irq68.ram=c->ram;irq68.gen=gen;irq68.s=stack;irq68.db=c->DB;irq68.m=c->m_flag;irq68.saved=c->m_flag?6:7;irq68.next=pc;memcpy(irq68.frame,c->ram+stack+1,4);irq68.armed=1;irq_stats68.entries++;}
 IrqStack68*w=&irq68;const unsigned*op=NULL;for(unsigned j=0;j<sizeof irq_ops68/sizeof*irq_ops68;j++)if(irq_ops68[j][0]==pc){op=irq_ops68[j];break;}if(!op)return 0;
 unsigned expected_m=pc<=0x87b8?w->m:pc==0x87ba?0:pc<=0x8925?1:w->m;
 if(!w->armed||w->cpu!=c||w->ram!=c->ram||w->gen!=gen||w->s!=stack||w->db!=c->DB||w->next!=pc||c->S+irq_depth68(pc,w->m)!=stack||c->m_flag!=expected_m||memcmp(w->frame,c->ram+stack+1,4)||(pc>=0x8925&&memcmp(w->registers+(w->saved-(stack-c->S)),c->ram+c->S+1,stack-c->S))){w->armed=0;irq_stats68.refused++;return 0;}
 unsigned bytes=op[1],cycles=op[2],memory=0,next=pc+bytes;uint64_t prior=c->master_cycles;c->coprocessor_master_cycles=prior;c->open_bus=r[pc&32767];if(bytes==2)c->open_bus=r[(pc+1)&32767];
 CpuWramReadContext ctx={.instruction_pc=pc,.stack=c->S,.y=c->Y,.pb=0,.m=c->m_flag,.x=0,.e=0,.interpreter=1};const CpuWramReadContext*old=cpu_swap_wram_read_context(&ctx);
 switch(pc){
 case 0x87b4:memory=c->m_flag?1:2;cycles=memory==1?3:4;irq_push68(c,c->A,memory);irq_stats68.pushes++;break;
 case 0x87b5:case 0x87b6:memory=2;irq_push68(c,pc==0x87b5?c->X:c->Y,2);irq_stats68.pushes++;break;
 case 0x87b7:memory=1;irq_push68(c,c->P,1);irq_stats68.pushes++;break;
 case 0x87b8:c->P&=~32;cpu_p_to_mirrors(c);irq_stats68.setup++;break;
 case 0x87ba:c->P|=32;cpu_p_to_mirrors(c);irq_stats68.setup++;break;
 case 0x87bc:memory=1;irq_push68(c,c->A,1);irq_stats68.pushes++;break;
 case 0x87bd:c->A&=0xff00;irq_nz68(c,0,8);irq_stats68.setup++;break;
 case 0x87bf:c->A=(c->A>>8)|((c->A&255)<<8);irq_nz68(c,c->A,8);irq_stats68.setup++;break;
 case 0x87c0:memory=1;c->A=(c->A&0xff00)|irq_pull68(c,1);irq_nz68(c,c->A,8);irq_stats68.pops++;break;
 case 0x87c1:c->P&=~16;cpu_p_to_mirrors(c);memcpy(w->registers,c->ram+c->S+1,w->saved);next=0x87c3;irq_stats68.setup++;break;
 case 0x8925:memory=1;c->P=irq_pull68(c,1);cpu_p_to_mirrors(c);irq_stats68.pops++;break;
 case 0x8926:case 0x8927:{memory=2;unsigned v=irq_pull68(c,2);if(pc==0x8926)c->Y=v;else c->X=v;irq_nz68(c,v,16);irq_stats68.pops++;break;}
 case 0x8928:memory=c->m_flag?1:2;cycles=memory==1?4:5;{unsigned v=irq_pull68(c,memory);c->A=c->m_flag?(c->A&0xff00)|v:v;irq_nz68(c,v,c->m_flag?8:16);}irq_stats68.pops++;w->armed=0;break;
 default:abort();}
 cpu_swap_wram_read_context(old);c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2+memory*2;w->next=pc==0x87c1?0x8925:next;
 if(!interp_bridge_commit_interrupt_source_opcode(c,prior,false))abort();*landing=next;return 1;
}
static int irq_mapped68(unsigned pc){if(pc==0x87b4){for(unsigned p=0x87b4;p<0x87c3;p++)if(!cpu_aot_rom_mapping_matches(p,p&32767,1))return 0;for(unsigned p=0x8925;p<=0x8929;p++)if(!cpu_aot_rom_mapping_matches(p,p&32767,1))return 0;return 1;}for(unsigned j=0;j<sizeof irq_ops68/sizeof*irq_ops68;j++)if(irq_ops68[j][0]==pc)return cpu_aot_rom_mapping_matches(pc,pc&32767,irq_ops68[j][1]);return 0;}
#ifndef LAGOON_IRQ_STACK68_TEST
#include "common_rtl.h"
#include <stdio.h>
static int irq_enabled68=-1,irq_report68;static uint64_t irq_gen68;
static void irq_exit68(void){if(irq_report68)fprintf(stderr,"[native-irq-stack68] entries=%llu pushes=%llu pops=%llu setup=%llu refused=%llu\n",(unsigned long long)irq_stats68.entries,(unsigned long long)irq_stats68.pushes,(unsigned long long)irq_stats68.pops,(unsigned long long)irq_stats68.setup,(unsigned long long)irq_stats68.refused);}
static void irq_hook68(CpuState*c,uint32_t pc){if(!irq_enabled68||c!=&g_cpu)return;uint32_t owner,landing;uint16_t stack;if(!irq_mapped68(pc)){irq68.armed=0;return;}if(interp_bridge_interrupt_source_owner(c,&owner,&stack)&&LagoonIrqStack68_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),irq_gen68,pc,owner,stack,&landing))interp_bridge_pre_opcode_redirect(landing);}
void LagoonIrqStack68_BeforeFrame(void){if(irq_enabled68<0){const char*v=getenv("LAGOON_TIMING68");irq_enabled68=!v||strcmp(v,"0");v=getenv("LAGOON_NATIVE_WAIT_REPORT");irq_report68=v&&!strcmp(v,"1");if(irq_report68)atexit(irq_exit68);}irq_gen68=RtlStateGeneration();if(irq68.gen!=irq_gen68)irq68.armed=0;if(irq_enabled68)for(unsigned j=0;j<sizeof irq_ops68/sizeof*irq_ops68;j++)if(!interp_bridge_add_pre_opcode_hook(irq_ops68[j][0],irq_hook68))abort();}
#endif
