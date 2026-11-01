/* Interrupt-owned source instructions: one opcode per callback. Original
 * hardware reads, architectural frame, DMA, calls and RTI stay real. */
#include "lagoon_native_interrupt66.h"
#include "snes/interp_bridge.h"
#include <string.h>
#include <stdlib.h>
static const struct {unsigned lo,n;uint64_t hash;} spans66[]={
 {0x87b4,374,UINT64_C(0xbc1b850d01f67e1e)},
 {0x8120,50,UINT64_C(0x6cb6b16f03dbbcd6)}};
typedef struct {const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,db,saved,phase;uint8_t frame[4],registers[7];} Witness66;
static Witness66 nmi66,reset66;
static struct {uint64_t entries,nmi,poll,input,tail,reset,refused;} stats66;
int LagoonInterrupt66_Source(const uint8_t*r,size_t n){
 if(!r||n!=0x100000)return 0;
 for(unsigned j=0;j<sizeof spans66/sizeof*spans66;j++){uint64_t h=UINT64_C(14695981039346656037);for(unsigned k=0;k<spans66[j].n;k++)h=(h^r[(spans66[j].lo&32767)+k])*UINT64_C(1099511628211);if(h!=spans66[j].hash)return 0;}return 1;
}
static int mode66(const CpuState*c){return c&&c->ram&&!c->PB&&c->DB<=2&&!c->D&&!c->emulation&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x1c)==4&&!!(c->P&32)==!!c->m_flag;}
static int arm66(Witness66*w,CpuState*c,uint64_t gen,unsigned frame){
 memset(w,0,sizeof*w);if(!mode66(c)||c->S<0x120||c->S>0x1ff-frame)return 0;w->cpu=c;w->ram=c->ram;w->gen=gen;w->s=c->S;w->db=c->DB;w->saved=c->m_flag?6:7;memcpy(w->frame,c->ram+w->s+1,frame);w->phase=1;return 1;
}
static const unsigned ops66[][3]={
 {0x87c3,3,4},{0x87c6,2,2},{0x87c8,3,4},{0x87cb,3,4},{0x87ce,2,0},
 {0x88d3,3,4},{0x88d6,2,2},{0x88d8,2,0},
 {0x88da,3,4},{0x88dd,3,4},{0x88e0,3,4},{0x88e3,3,4},
 {0x88e6,3,4},{0x88e9,3,4},{0x88ec,3,4},{0x88ef,3,4},
 {0x88f2,3,4},{0x88f5,3,4},{0x88f8,3,4},{0x88fb,3,4},
 {0x88fe,3,4},{0x8901,3,4},{0x8904,3,4},{0x8907,3,4},
 {0x890a,3,4},{0x890d,1,2},{0x890e,2,0},{0x8910,3,4},
 {0x8913,3,4},{0x8916,2,2},{0x8918,3,6},{0x891b,3,4},
 {0x891e,1,2},{0x891f,3,4},{0x8922,3,6},
 {0x8148,3,4},{0x814b,2,0},{0x814d,2,2},{0x814f,1,2},{0x8150,2,0}};
/* Verify the entire owner at entry, then the COMPLETE current opcode on
 * every callback. No mapping epoch/cache assumption: each executed byte is
 * checked at execution; the full source fingerprint is still checked below. */
static int mapped66(unsigned pc){
 if(pc==0x87b4||pc==0x8120){for(unsigned j=0;j<sizeof spans66/sizeof*spans66;j++)for(unsigned k=0;k<spans66[j].n;k++)if(!cpu_aot_rom_mapping_matches(spans66[j].lo+k,(spans66[j].lo&32767)+k,1))return 0;return 1;}
 for(unsigned j=0;j<sizeof ops66/sizeof*ops66;j++)if(ops66[j][0]==pc)return cpu_aot_rom_mapping_matches(pc,pc&32767,ops66[j][1]);
 return 0;
}
static void nz66(CpuState*c,unsigned v){c->_flag_Z=!(v&255);c->_flag_N=!!(v&128);cpu_mirrors_to_p(c);}
static void a66(CpuState*c,unsigned v){c->A=(c->A&0xff00)|(v&255);nz66(c,v);}
static unsigned read66(CpuState*c,unsigned address,int irq){uint8_t v;int ok=irq?interp_bridge_interrupt_source_read8(c,address,&v):interp_bridge_source_read8(c,address,&v);if(!ok)abort();c->master_cycles+=cpu_region_speed(address)-6;return v;}
static void write66(CpuState*c,unsigned address,unsigned v,int irq){int ok=irq?interp_bridge_interrupt_source_write8(c,address,v):interp_bridge_source_write8(c,address,v);if(!ok)abort();c->master_cycles+=cpu_region_speed(address)-6;}
int LagoonInterrupt66_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,uint32_t pc,uint32_t owner,uint16_t owner_s,uint32_t*landing){
 if(!landing||c!=primary||gen!=frame||!mode66(c)||!LagoonInterrupt66_Source(r,n)){nmi66.phase=reset66.phase=0;++stats66.refused;return 0;}
 int irq=pc>=0x87b4&&pc<=0x8922;Witness66*w=irq?&nmi66:&reset66;unsigned size=irq?4:3;
 if(irq&&(owner!=0x87b4||(!w->phase&&pc!=0x87b4)))return 0;
 if(pc==0x87b4){if(c->S!=owner_s)return 0;if(arm66(w,c,gen,4))++stats66.entries;return 0;}
 if(pc==0x8120){if(c->m_flag)arm66(w,c,gen,3);return 0;}
 if(!w->phase||w->cpu!=c||w->ram!=c->ram||w->gen!=gen||w->db!=c->DB||!c->m_flag||memcmp(w->frame,c->ram+w->s+1,size)||(irq&&(w->s!=owner_s||c->S+w->saved!=w->s))||(!irq&&c->S!=w->s)){w->phase=0;++stats66.refused;return 0;}
 if(irq){
  if(pc==0x87c3&&w->phase==1){memcpy(w->registers,c->ram+c->S+1,w->saved);w->phase=2;}
  if(w->phase!=2||memcmp(w->registers,c->ram+c->S+1,w->saved)){w->phase=0;++stats66.refused;return 0;}
 }
 const unsigned*op=NULL;for(unsigned j=0;j<sizeof ops66/sizeof*ops66;j++)if(ops66[j][0]==pc){op=ops66[j];break;}if(!op)return 0;
 if(!irq&&interp_bridge_source_opcode_deadline_reached(c))return 0;
 unsigned bytes=op[1],cycles=op[2],at=pc&32767,im=bytes>=2?r[at+1]:0,operand=im|(bytes==3?r[at+2]<<8:0),address=((unsigned)c->DB<<16)|operand,next=pc+bytes;uint64_t prior=c->master_cycles;c->coprocessor_master_cycles=prior;c->open_bus=r[at+bytes-1];
 if(!cycles){unsigned take=(pc==0x814b||pc==0x87ce)?!c->_flag_N:pc==0x890e?!c->_flag_C:!c->_flag_Z;cycles=take?3:2;if(take)next=pc+bytes+(int8_t)im;}
 switch(pc){
 case 0x87c3:{unsigned v=read66(c,address,irq),a=c->A&255;c->_flag_C=a>=v;nz66(c,(a-v)&255);break;}
 case 0x87c6:case 0x814d:case 0x8916:a66(c,im);break;
 case 0x87cb:case 0x88d3:case 0x88da:case 0x88e0:case 0x88e6:case 0x88ec:case 0x88f2:case 0x88f8:case 0x88fe:case 0x8904:case 0x890a:case 0x8910:case 0x891b:case 0x8148:a66(c,read66(c,address,irq));break;
 case 0x87c8:case 0x88dd:case 0x88e3:case 0x88e9:case 0x88ef:case 0x88f5:case 0x88fb:case 0x8901:case 0x8907:case 0x8913:case 0x891f:write66(c,address,c->A,irq);break;
 case 0x88d6:a66(c,c->A&im);break;
 case 0x890d:c->_flag_C=c->A&1;a66(c,(c->A&255)>>1);break;
 case 0x8918:{unsigned v=read66(c,address,irq);c->_flag_Z=!(v&c->A&255);cpu_mirrors_to_p(c);write66(c,address,v&~c->A,irq);break;}
 case 0x891e:a66(c,c->A-1);break;
 case 0x8922:{unsigned v=(read66(c,address,irq)+1)&255;write66(c,address,v,irq);nz66(c,v);break;}
 case 0x814f:a66(c,c->A+1);break;
 default:break;
 }
 c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2;
 if(irq){++stats66.nmi;if(pc<=0x88d8&&pc>=0x88d3)++stats66.poll;else if(pc>=0x88da&&pc<0x890a)++stats66.input;else ++stats66.tail;if(!interp_bridge_commit_interrupt_source_opcode(c,prior,false))abort();}
 else {++stats66.reset;if(!interp_bridge_commit_source_opcode(c,prior,false))abort();}
 *landing=next;return 1;
}
#ifndef LAGOON_INTERRUPT66_TEST
#include "common_rtl.h"
#include <stdio.h>
static int enabled66=-1,report66;static uint64_t gen66;
static void report_exit66(void){if(report66)fprintf(stderr,"[native-interrupt66] entries=%llu nmi=%llu poll=%llu input=%llu tail=%llu reset=%llu refused=%llu\n",(unsigned long long)stats66.entries,(unsigned long long)stats66.nmi,(unsigned long long)stats66.poll,(unsigned long long)stats66.input,(unsigned long long)stats66.tail,(unsigned long long)stats66.reset,(unsigned long long)stats66.refused);}

static void hook66(CpuState*c,uint32_t pc){if(!enabled66||c!=&g_cpu||!mapped66(pc))return;uint32_t root=0;uint16_t stack=0;if(pc>=0x87b4){if(!interp_bridge_interrupt_source_owner(c,&root,&stack))return;}else if(!interp_bridge_source_opcode_available(c))return;uint32_t landing;if(LagoonInterrupt66_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),gen66,pc,root,stack,&landing))interp_bridge_pre_opcode_redirect(landing);}
bool LagoonInterrupt66_ResetEntry(CpuState*c,uint16_t s,uint8_t h,uint32_t host,RecompReturn*out){(void)out;if(enabled66&&c==&g_cpu&&c->ram&&s==c->S&&s>=0x120&&s<=0x1fc&&h==3&&c->host_return_valid==3&&mapped66(0x8120)&&(((unsigned)c->ram[s+3]<<16)|((c->ram[s+1]|c->ram[s+2]<<8)+1)&65535)==host&&RtlStateGeneration()==gen66&&LagoonInterrupt66_Source(g_rom,0x100000)&&mode66(c)&&c->m_flag)arm66(&reset66,c,gen66,3);return false;}
void LagoonInterrupt66_BeforeFrame(void){if(enabled66<0){const char*v=getenv("LAGOON_TIMING66");enabled66=!v||strcmp(v,"0");v=getenv("LAGOON_NATIVE_WAIT_REPORT");report66=v&&!strcmp(v,"1");if(report66)atexit(report_exit66);}gen66=RtlStateGeneration();if(nmi66.gen!=gen66)nmi66.phase=0;if(reset66.gen!=gen66)reset66.phase=0;if(enabled66){interp_bridge_add_pre_opcode_hook(0x87b4,hook66);interp_bridge_add_pre_opcode_hook(0x8120,hook66);for(unsigned j=0;j<sizeof ops66/sizeof*ops66;j++)if(!interp_bridge_add_pre_opcode_hook(ops66[j][0],hook66))abort();}}
#endif
