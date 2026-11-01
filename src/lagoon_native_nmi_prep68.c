/* Original NMI request-bit control and DMA/PPU register preparation only.
 * 88C3 STA420B remains the actual interpreter DMA trigger, with its stalls and
 * opcode-only APU accounting. No DMA or elapsed clocks are manufactured here. */
#include "lagoon_native_nmi_prep68.h"
#include "snes/interp_bridge.h"
#include <string.h>
#include <stdlib.h>
typedef struct{const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,db,saved,next,temp,temporary,armed;uint8_t frame[4],registers[7];}NmiPrepOwner68;
static NmiPrepOwner68 nmi_prep68;
static struct{uint64_t entries,instructions,dma_boundaries,refused;}nmi_prep_stats68;
static unsigned nmi_prep_extra68;
int LagoonNmiPrep68_Source(const uint8_t*r,size_t n){if(!r||n!=0x100000)return 0;uint64_t h=UINT64_C(14695981039346656037);for(unsigned k=0;k<374;k++)h=(h^r[0x7b4+k])*UINT64_C(1099511628211);return h==UINT64_C(0xbc1b850d01f67e1e);}
/* pc, original bytes, base CPU cycles, additional temporary-PHA depth.
 * The original DMA trigger88C3 and later calls are deliberately absent. */
static const unsigned nmi_prep_ops68[][4]={
 {0x87d0,1,3,0},{0x87d1,3,3,1},{0x87d4,3,5,1},{0x87d7,2,2,1},{0x87d9,3,4,1},
 {0x87dc,2,2,1},{0x87de,3,4,1},{0x87e1,3,3,1},{0x87e4,3,5,1},{0x87e7,2,2,1},
 {0x87e9,3,4,1},{0x87ec,3,3,1},{0x87ef,3,5,1},{0x87f2,2,2,1},{0x87f4,3,4,1},
 {0x87f7,2,2,1},{0x87f9,3,6,1},{0x87fc,1,4,1},{0x87fd,1,2,0},{0x87fe,2,0,0},
 {0x8800,1,3,0},{0x8801,3,4,1},{0x8804,3,4,1},{0x8807,2,0,1},{0x8809,2,2,1},
 {0x880b,3,4,1},{0x880e,2,2,1},{0x8810,3,4,1},{0x8813,2,2,1},{0x8815,3,4,1},
 {0x8818,2,2,1},{0x881a,3,4,1},{0x881d,2,2,1},{0x881f,3,4,1},{0x8822,3,4,1},
 {0x8825,3,4,1},{0x8828,3,4,1},{0x882b,3,4,1},{0x882e,2,2,1},{0x8830,3,4,1},
 {0x8833,3,4,1},{0x8836,3,4,1},{0x8839,2,2,1},{0x883b,3,4,1},{0x883e,2,2,1},
 {0x8840,3,6,1},{0x8843,3,6,1},{0x8846,1,4,1},{0x8847,1,2,0},{0x8848,2,0,0},
 {0x884a,1,3,0},{0x884b,3,4,1},{0x884e,3,4,1},{0x8851,3,4,1},{0x8854,3,4,1},
 {0x8857,2,2,1},{0x8859,3,4,1},{0x885c,2,2,1},{0x885e,3,4,1},{0x8861,2,3,1},
 {0x8863,3,4,1},{0x8866,2,3,1},{0x8868,3,4,1},{0x886b,2,3,1},{0x886d,3,4,1},
 {0x8870,3,5,1},{0x8873,3,5,1},{0x8876,3,4,1},{0x8879,2,2,1},{0x887b,3,4,1},
 {0x887e,2,2,1},{0x8880,3,6,1},{0x8883,1,4,1},{0x8884,1,2,0},{0x8885,2,0,0},
 {0x8887,1,3,0},{0x8888,2,2,1},{0x888a,3,4,1},{0x888d,2,2,1},{0x888f,3,4,1},
 {0x8892,2,2,1},{0x8894,3,4,1},{0x8897,2,2,1},{0x8899,3,4,1},{0x889c,2,2,1},
 {0x889e,3,4,1},{0x88a1,2,2,1},{0x88a3,3,4,1},{0x88a6,2,2,1},{0x88a8,3,4,1},
 {0x88ab,2,2,1},{0x88ad,3,4,1},{0x88b0,3,4,1},{0x88b3,2,2,1},{0x88b5,3,4,1},
 {0x88b8,2,2,1},{0x88ba,3,6,1},{0x88bd,1,4,1},{0x88be,3,4,0},{0x88c1,2,0,0},
 {0x88c6,3,4,0}};
static int nmi_prep_mode68(const CpuState*c){return c&&c->ram&&!c->PB&&c->DB<=2&&!c->D&&!c->emulation&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x1c)==4&&!!(c->P&32)==!!c->m_flag;}
static void nmi_prep_nz68(CpuState*c,unsigned v,unsigned bits){c->_flag_Z=!(v&(bits==8?255:65535));c->_flag_N=!!(v&(1u<<(bits-1)));cpu_mirrors_to_p(c);}
static void nmi_prep_a68(CpuState*c,unsigned v){c->A=(c->A&0xff00)|(v&255);nmi_prep_nz68(c,v,8);}
static unsigned nmi_prep_read68(CpuState*c,unsigned p){uint8_t v;if(!interp_bridge_interrupt_source_read8(c,p,&v))abort();nmi_prep_extra68+=cpu_region_speed(p)-6;return v;}
static void nmi_prep_write68(CpuState*c,unsigned p,unsigned v){if(!interp_bridge_interrupt_source_write8(c,p,v))abort();nmi_prep_extra68+=cpu_region_speed(p)-6;}
int LagoonNmiPrep68_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,uint32_t pc,uint32_t owner,uint16_t stack,uint32_t*landing){
 if(!landing||c!=primary||gen!=frame||!nmi_prep_mode68(c)||owner!=0x87b4||!LagoonNmiPrep68_Source(r,n)){nmi_prep68.armed=0;nmi_prep_stats68.refused++;return 0;}
 NmiPrepOwner68*w=&nmi_prep68;
 if(pc==0x87b4){memset(w,0,sizeof*w);if(c->S!=stack||stack<0x128||stack>0x1fb)return 0;w->cpu=c;w->ram=c->ram;w->gen=gen;w->s=stack;w->db=c->DB;w->saved=c->m_flag?6:7;memcpy(w->frame,c->ram+stack+1,4);w->registers[0]=c->P;w->registers[1]=c->Y;w->registers[2]=c->Y>>8;w->registers[3]=c->X;w->registers[4]=c->X>>8;w->registers[5]=c->A;if(!c->m_flag)w->registers[6]=c->A>>8;w->armed=1;nmi_prep_stats68.entries++;return 0;}
 const unsigned*op=NULL;for(unsigned j=0;j<sizeof nmi_prep_ops68/sizeof*nmi_prep_ops68;j++)if(nmi_prep_ops68[j][0]==pc){op=nmi_prep_ops68[j];break;}if(!op&&pc!=0x88c3)return 0;
 unsigned depth=op?op[3]:0;
 if(!w->armed||w->cpu!=c||w->ram!=c->ram||w->gen!=gen||w->s!=stack||w->db!=c->DB||!c->m_flag||c->S+w->saved+depth!=stack||memcmp(w->frame,c->ram+stack+1,4)||memcmp(w->registers,c->ram+stack-w->saved+1,w->saved)||(depth&&(!w->temporary||c->ram[c->S+1]!=w->temp))||(w->next?w->next!=pc:(pc!=0x87d0&&pc!=0x87fd))){w->armed=0;nmi_prep_stats68.refused++;return 0;}
 if(pc==0x88c3){w->next=0x88c6;nmi_prep_stats68.dma_boundaries++;return 0;}
 unsigned bytes=op[1],cycles=op[2],at=pc&32767,im=bytes>1?r[at+1]:0,operand=im|(bytes==3?r[at+2]<<8:0),address=((unsigned)c->DB<<16)|operand,next=pc+bytes;uint64_t prior=c->master_cycles;nmi_prep_extra68=0;c->coprocessor_master_cycles=prior;c->open_bus=r[at+bytes-1];
 if(!cycles){unsigned take=(pc==0x8807||pc==0x88c1)?c->_flag_Z:!c->_flag_N;cycles=take?3:2;if(take)next=pc+bytes+(int8_t)im;}
 CpuWramReadContext context={.instruction_pc=pc,.stack=c->S,.y=c->Y,.pb=0,.m=1,.x=0,.e=0,.interpreter=1};const CpuWramReadContext*old=cpu_swap_wram_read_context(&context);
 switch(pc){
 case 0x87d0:case 0x8800:case 0x884a:case 0x8887:w->temporary=1;w->temp=c->A&255;nmi_prep_write68(c,c->S--,c->A);break;
 case 0x87fc:case 0x8846:case 0x8883:case 0x88bd:nmi_prep_a68(c,nmi_prep_read68(c,++c->S));w->temporary=0;break;
 case 0x87d1:case 0x87e1:case 0x87ec:c->X=operand;nmi_prep_nz68(c,c->X,16);break;
 case 0x87d4:case 0x87e4:case 0x87ef:case 0x8873:nmi_prep_write68(c,address,c->X);nmi_prep_write68(c,(address&0xff0000)|((address+1)&65535),c->X>>8);break;
 case 0x87d7:case 0x87dc:case 0x87e7:case 0x87f2:case 0x87f7:case 0x8809:case 0x880e:case 0x8813:case 0x8818:case 0x881d:case 0x882e:case 0x883e:case 0x8857:case 0x885c:case 0x887e:case 0x8888:case 0x888d:case 0x8892:case 0x8897:case 0x889c:case 0x88a1:case 0x88a6:case 0x88ab:case 0x88b8:nmi_prep_a68(c,im);break;
 case 0x87d9:case 0x87de:case 0x87e9:case 0x87f4:case 0x880b:case 0x8810:case 0x8815:case 0x881a:case 0x881f:case 0x8825:case 0x882b:case 0x8830:case 0x8833:case 0x883b:case 0x884e:case 0x8854:case 0x8859:case 0x885e:case 0x8863:case 0x8868:case 0x886d:case 0x887b:case 0x888a:case 0x888f:case 0x8894:case 0x8899:case 0x889e:case 0x88a3:case 0x88a8:case 0x88ad:case 0x88b5:nmi_prep_write68(c,address,c->A);break;
 case 0x87f9:case 0x8840:case 0x8880:case 0x88ba:{unsigned v=nmi_prep_read68(c,address);c->_flag_Z=!(v&c->A&255);cpu_mirrors_to_p(c);nmi_prep_write68(c,address,v&~c->A);break;}
 case 0x8843:{unsigned v=(nmi_prep_read68(c,address)+1)&255;nmi_prep_write68(c,address,v);nmi_prep_nz68(c,v,8);break;}
 case 0x87fd:case 0x8847:case 0x8884:c->_flag_C=!!(c->A&128);nmi_prep_a68(c,c->A<<1);break;
 case 0x8801:case 0x8822:case 0x8828:case 0x8836:case 0x884b:case 0x8851:case 0x8876:case 0x88b0:case 0x88be:nmi_prep_a68(c,nmi_prep_read68(c,address));break;
 case 0x8804:nmi_prep_a68(c,c->A|nmi_prep_read68(c,address));break;
 case 0x8839:case 0x8879:case 0x88b3:nmi_prep_a68(c,c->A|im);break;
 case 0x8861:case 0x8866:case 0x886b:nmi_prep_a68(c,nmi_prep_read68(c,im));break;
 case 0x8870:{unsigned lo=nmi_prep_read68(c,address);c->X=lo|(nmi_prep_read68(c,(address&0xff0000)|((address+1)&65535))<<8);nmi_prep_nz68(c,c->X,16);break;}
 case 0x88c6:nmi_prep_write68(c,address,0);w->armed=0;break;
 default:break;
 }
 cpu_swap_wram_read_context(old);c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2+nmi_prep_extra68;w->next=next;nmi_prep_stats68.instructions++;if(!interp_bridge_commit_interrupt_source_opcode(c,prior,false))abort();*landing=next;return 1;
}
static int nmi_prep_mapped68(unsigned pc){if(pc==0x87b4){for(unsigned k=0;k<374;k++)if(!cpu_aot_rom_mapping_matches(0x87b4+k,0x7b4+k,1))return 0;return 1;}if(pc==0x88c3)return cpu_aot_rom_mapping_matches(pc,pc&32767,3);for(unsigned j=0;j<sizeof nmi_prep_ops68/sizeof*nmi_prep_ops68;j++)if(nmi_prep_ops68[j][0]==pc)return cpu_aot_rom_mapping_matches(pc,pc&32767,nmi_prep_ops68[j][1]);return 0;}
#ifndef LAGOON_NMI_PREP68_TEST
#include "common_rtl.h"
#include <stdio.h>
static uint64_t nmi_prep_generation68;static int nmi_prep_enabled68=-1,nmi_prep_report68;
static void nmi_prep_exit68(void){if(nmi_prep_report68)fprintf(stderr,"[native-nmi-prep68] entries=%llu instructions=%llu dma_boundaries=%llu refused=%llu\n",(unsigned long long)nmi_prep_stats68.entries,(unsigned long long)nmi_prep_stats68.instructions,(unsigned long long)nmi_prep_stats68.dma_boundaries,(unsigned long long)nmi_prep_stats68.refused);}
static void nmi_prep_hook68(CpuState*c,uint32_t pc){if(!nmi_prep_enabled68||c!=&g_cpu)return;uint32_t owner,landing;uint16_t stack;if(!nmi_prep_mapped68(pc)){nmi_prep68.armed=0;return;}if(interp_bridge_interrupt_source_owner(c,&owner,&stack)&&LagoonNmiPrep68_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),nmi_prep_generation68,pc,owner,stack,&landing))interp_bridge_pre_opcode_redirect(landing);}
/* Register after66's pure87B4 observer, before68's source PHA redirect. */
void LagoonNmiPrep68_BeforeFrame(void){if(nmi_prep_enabled68<0){const char*v=getenv("LAGOON_TIMING68");nmi_prep_enabled68=!v||strcmp(v,"0");v=getenv("LAGOON_NATIVE_WAIT_REPORT");nmi_prep_report68=v&&!strcmp(v,"1");if(nmi_prep_report68)atexit(nmi_prep_exit68);}nmi_prep_generation68=RtlStateGeneration();if(nmi_prep68.gen!=nmi_prep_generation68)nmi_prep68.armed=0;if(nmi_prep_enabled68){if(!interp_bridge_add_pre_opcode_hook(0x87b4,nmi_prep_hook68)||!interp_bridge_add_pre_opcode_hook(0x88c3,nmi_prep_hook68))abort();for(unsigned j=0;j<sizeof nmi_prep_ops68/sizeof*nmi_prep_ops68;j++)if(!interp_bridge_add_pre_opcode_hook(nmi_prep_ops68[j][0],nmi_prep_hook68))abort();}}
#endif
