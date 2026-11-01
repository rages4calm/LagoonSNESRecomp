/* Inline scheduler: retain actual inherited stack, calls, child effects and
 * nonlocal unwinds. Constructor: retain actual JSR D991 frame and final RTS.
 * Every accelerated callback completes exactly one original source opcode. */
#include "lagoon_native_scheduler67.h"
#include "snes/interp_bridge.h"
#include <string.h>
#include <stdlib.h>
static uint64_t hash67(const uint8_t*r,unsigned at,unsigned n){uint64_t h=UINT64_C(14695981039346656037);for(unsigned j=0;j<n;j++)h=(h^r[at+j])*UINT64_C(1099511628211);return h;}
int LagoonScheduler67_Source(const uint8_t*r,size_t n){if(!r||n!=0x100000)return 0;uint64_t h=hash67(r,0x807a,0x6d);return (h==UINT64_C(0xd9eab56ea8b810fb)||h==UINT64_C(0x6ea0a0f489b2db1f))&&hash67(r,0x159bb,0x52)==UINT64_C(0x44ebff9bfd14131c)&&!memcmp(r+0x15991,"\x20\xbb\xd9",3);}
typedef struct{const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,db,next,x,count;uint8_t frame[3];int armed;} Owner67;
static Owner67 scheduler67,init67;
static struct{uint64_t entries,scheduler,init,deadlines,refused;} stats67;
static unsigned offset67(unsigned pc){return (pc>>16)*32768+(pc&32767);}
static int mode67(const CpuState*c){return c&&c->ram&&!c->D&&!c->emulation&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x1c)==4&&!!(c->P&32)==!!c->m_flag;}
static int arm67(Owner67*w,CpuState*c,uint64_t gen,unsigned pc){memset(w,0,sizeof*w);if(!mode67(c)||!c->m_flag||c->S<0x120||c->S>0x1ffc)return 0;if(pc==0x02d9bb&&(c->ram[c->S+1]!=0x93||c->ram[c->S+2]!=0xd9))return 0;w->cpu=c;w->ram=c->ram;w->gen=gen;w->s=c->S;w->db=c->DB;w->next=pc==0x01807a?0x0180af:pc;w->x=c->X;w->count=pc==0x02d9bb?2:(c->S<=0x1ff?(0x1ff-c->S<3?0x1ff-c->S:3):3);memcpy(w->frame,c->ram+c->S+1,w->count);w->armed=1;stats67.entries++;return 1;}
/* pc, bytes, base cycles, M. Calls and returns deliberately absent. */
static const unsigned ops67[][4]={
 {0x0180af,3,3,1},{0x0180b2,3,5,1},{0x0180b5,2,2,1},{0x0180b7,2,2,1},{0x0180b9,2,0,1},
 {0x0180d1,2,3,1},{0x0180d3,1,2,0},{0x0180d4,1,2,0},{0x0180d5,3,3,0},{0x0180d8,1,2,0},{0x0180d9,3,3,0},{0x0180dc,2,3,0},{0x0180de,3,3,1},{0x0180e1,2,0,1},
 {0x02d9bb,3,3,1},{0x02d9be,3,5,1},{0x02d9c1,1,2,1},{0x02d9c2,3,3,1},{0x02d9c5,2,0,1},
 {0x02d9c7,2,2,1},{0x02d9c9,3,4,1},{0x02d9cc,3,3,1},{0x02d9cf,3,5,1},{0x02d9d2,3,3,1},{0x02d9d5,3,5,1},{0x02d9d8,2,2,1},{0x02d9da,3,4,1},{0x02d9dd,1,2,1},{0x02d9de,3,4,1},{0x02d9e1,2,2,1},{0x02d9e3,3,4,1},{0x02d9e6,2,2,1},{0x02d9e8,3,4,1},{0x02d9eb,3,3,1},{0x02d9ee,3,5,1},{0x02d9f1,3,5,1},{0x02d9f4,3,3,1},{0x02d9f7,3,5,1},{0x02d9fa,3,3,1},{0x02d9fd,3,5,1},{0x02da00,3,3,1},{0x02da03,3,5,1},{0x02da06,3,3,1},{0x02da09,3,5,1}};
static void nz67(CpuState*c,unsigned v,unsigned bits){unsigned mask=bits==8?255:65535;c->_flag_Z=!(v&mask);c->_flag_N=!!(v&(1u<<(bits-1)));cpu_mirrors_to_p(c);}
static void a67(CpuState*c,unsigned v){c->A=c->m_flag?((c->A&0xff00)|(v&255)):(v&65535);nz67(c,v,c->m_flag?8:16);}
static void cmp67(CpuState*c,unsigned x,unsigned v,unsigned bits){unsigned mask=bits==8?255:65535;c->_flag_C=(x&mask)>=(v&mask);nz67(c,x-v,bits);}
static unsigned read67(CpuState*c,unsigned p){uint8_t v;if(!interp_bridge_source_read8(c,p,&v))abort();c->master_cycles+=cpu_region_speed(p)-6;return v;}
static void write67(CpuState*c,unsigned p,unsigned v,unsigned bytes){if(bytes==1){if(!interp_bridge_source_write8(c,p,v))abort();}else if(!interp_bridge_source_write16(c,p,(p&0xff0000)|((p+1)&65535),v))abort();c->master_cycles+=cpu_region_speed(p)-6;if(bytes==2)c->master_cycles+=cpu_region_speed((p&0xff0000)|((p+1)&65535))-6;}
int LagoonScheduler67_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,uint32_t pc,uint32_t*landing){
 if(!landing||c!=primary||gen!=frame||!mode67(c)||!LagoonScheduler67_Source(r,n)){scheduler67.armed=init67.armed=0;stats67.refused++;return 0;}
 int init=pc>>16==2;Owner67*w=init?&init67:&scheduler67;
 if((init&&(c->PB!=2||c->DB>2))||(!init&&(c->PB!=1||c->DB!=1))){w->armed=0;return 0;}
 if(pc==0x01807a){arm67(w,c,gen,pc);return 0;}
 if(pc==0x0180af||pc==0x02d9bb){if(!arm67(w,c,gen,pc))return 0;}
 if(!w->armed||w->cpu!=c||w->ram!=c->ram||w->gen!=gen||w->s!=c->S||w->db!=c->DB||memcmp(w->frame,c->ram+w->s+1,w->count)||w->next!=pc||w->x!=c->X){w->armed=0;stats67.refused++;return 0;}
 /* Track each original call boundary. A child that unwinds or changes X may
  * never lend its frame or register state to a later scheduler callback. */
 if(pc>=0x0180bb&&pc<=0x0180ca&&(pc-0x0180bb)%3==0){if(!c->m_flag||c->X>0x800||(c->X&63)){w->armed=0;return 0;}w->next=pc+3;return 0;}
 if(pc==0x0180cd){if(!c->m_flag||c->X>0x800||(c->X&63)){w->armed=0;return 0;}w->next=0x0180d1;return 0;}
 const unsigned*op=NULL;for(unsigned j=0;j<sizeof ops67/sizeof*ops67;j++)if(ops67[j][0]==pc&&ops67[j][3]==c->m_flag){op=ops67[j];break;}if(!op){w->armed=0;return 0;}
 if(interp_bridge_source_opcode_deadline_reached(c)){stats67.deadlines++;return 0;}
 unsigned at=offset67(pc),bytes=op[1],cycles=op[2],im=bytes>1?r[at+1]:0,operand=im|(bytes==3?r[at+2]<<8:0),next=pc+bytes,address=((unsigned)c->DB<<16)|operand;uint64_t prior=c->master_cycles;
 if(!cycles){int take=!c->_flag_Z;cycles=take?3:2;if(take)next=pc+bytes+(int8_t)im;}
 c->coprocessor_master_cycles=prior;c->open_bus=r[at+bytes-1];
 CpuWramReadContext context={.instruction_pc=pc,.stack=c->S,.y=c->Y,.pb=c->PB,.m=c->m_flag,.x=c->x_flag,.e=c->emulation,.interpreter=1};const CpuWramReadContext*old_context=cpu_swap_wram_read_context(&context);
 switch(pc){
 case 0x0180af:case 0x02d9bb:case 0x02d9cc:case 0x02d9d2:case 0x02d9eb:case 0x02d9f4:case 0x02d9fa:case 0x02da00:case 0x02da06:c->X=operand;nz67(c,c->X,16);break;
 case 0x0180b2:a67(c,read67(c,(address+c->X)&0xffffff));break;
 case 0x0180b5:a67(c,c->A&im);break;
 case 0x0180b7:cmp67(c,c->A,im,8);break;
 case 0x0180d1:c->P&=~32;cpu_p_to_mirrors(c);break;
 case 0x0180d3:a67(c,c->X);break;
 case 0x0180d4:c->_flag_C=0;cpu_mirrors_to_p(c);break;
 case 0x0180d5:{unsigned x=c->A,sum=x+operand+c->_flag_C;c->_flag_C=sum>65535;c->_flag_V=!!((~(x^operand)&(x^sum))&32768);a67(c,sum);break;}
 case 0x0180d8:c->X=c->A;nz67(c,c->X,16);break;
 case 0x0180d9:a67(c,c->A&operand);break;
 case 0x0180dc:c->P|=32;cpu_p_to_mirrors(c);break;
 case 0x0180de:case 0x02d9c2:cmp67(c,c->X,operand,16);break;
 case 0x02d9be:write67(c,(address+c->X)&0xffffff,0,1);break;
 case 0x02d9c1:c->X=(uint16_t)(c->X+1);nz67(c,c->X,16);break;
 case 0x02d9c7:case 0x02d9d8:case 0x02d9e1:case 0x02d9e6:a67(c,im);break;
 case 0x02d9c9:case 0x02d9da:case 0x02d9de:case 0x02d9e3:case 0x02d9e8:write67(c,address,c->A,1);break;
 case 0x02d9cf:case 0x02d9d5:case 0x02d9ee:case 0x02d9f1:case 0x02d9f7:case 0x02d9fd:case 0x02da03:case 0x02da09:write67(c,address,c->X,2);break;
 case 0x02d9dd:a67(c,c->A+1);break;
 default:break;
 }
 cpu_swap_wram_read_context(old_context);c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2;w->next=next;w->x=c->X;if(init)stats67.init++;else stats67.scheduler++;if(!interp_bridge_commit_source_opcode(c,prior,false))abort();*landing=next;return 1;
}
static int mapped67(unsigned pc){
 if(pc==0x01807a||pc==0x0180af||pc==0x02d9bb){unsigned start=pc==0x02d9bb?0x02d9bb:0x01807a,n=pc==0x02d9bb?0x52:0x6d;for(unsigned k=0;k<n;k++)if(!cpu_aot_rom_mapping_matches(start+k,offset67(start)+k,1))return 0;if(pc==0x02d9bb&&!cpu_aot_rom_mapping_matches(0x02d991,0x15991,3))return 0;return 1;}
 if(pc>=0x0180bb&&pc<=0x0180ca&&(pc-0x0180bb)%3==0)return cpu_aot_rom_mapping_matches(pc,offset67(pc),3);
 if(pc==0x0180cd)return cpu_aot_rom_mapping_matches(pc,offset67(pc),4);
 for(unsigned j=0;j<sizeof ops67/sizeof*ops67;j++)if(ops67[j][0]==pc)return cpu_aot_rom_mapping_matches(pc,offset67(pc),ops67[j][1]);return 0;
}
#ifndef LAGOON_SCHEDULER67_TEST
#include "common_rtl.h"
#include <stdio.h>
static uint64_t generation67;static int enabled67=-1,report67;
static void report_exit67(void){if(report67)fprintf(stderr,"[native-scheduler67] entries=%llu scheduler=%llu init=%llu deadlines=%llu refused=%llu\n",(unsigned long long)stats67.entries,(unsigned long long)stats67.scheduler,(unsigned long long)stats67.init,(unsigned long long)stats67.deadlines,(unsigned long long)stats67.refused);}
static void hook67(CpuState*c,uint32_t pc){if(!enabled67||c!=&g_cpu||!interp_bridge_source_opcode_available(c))return;if(!mapped67(pc)){scheduler67.armed=init67.armed=0;return;}uint32_t landing;if(LagoonScheduler67_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),generation67,pc,&landing))interp_bridge_pre_opcode_redirect(landing);}
void LagoonScheduler67_BeforeFrame(void){if(enabled67<0){const char*v=getenv("LAGOON_TIMING67");enabled67=!v||strcmp(v,"0");v=getenv("LAGOON_NATIVE_WAIT_REPORT");report67=v&&!strcmp(v,"1");if(report67)atexit(report_exit67);}generation67=RtlStateGeneration();if(scheduler67.gen!=generation67)scheduler67.armed=0;if(init67.gen!=generation67)init67.armed=0;if(enabled67){interp_bridge_add_pre_opcode_hook(0x01807a,hook67);for(unsigned pc=0x0180bb;pc<=0x0180ca;pc+=3)interp_bridge_add_pre_opcode_hook(pc,hook67);interp_bridge_add_pre_opcode_hook(0x0180cd,hook67);for(unsigned j=0;j<sizeof ops67/sizeof*ops67;j++)interp_bridge_add_pre_opcode_hook(ops67[j][0],hook67);}}
#endif
