/* Source fallback for the original metatile expansion after services65's
 * clear. Existing optimized BuildMap remains first and authoritative. */
#include "lagoon_native_map68.h"
#include "snes/interp_bridge.h"
#include <string.h>
#include <stdlib.h>
static uint64_t map_hash68(const uint8_t*r,unsigned at,unsigned n){uint64_t h=UINT64_C(14695981039346656037);for(unsigned j=0;j<n;j++)h=(h^r[at+j])*UINT64_C(1099511628211);return h;}
int LagoonMap68_Source(const uint8_t*r,size_t n){return r&&n==0x100000&&map_hash68(r,0x145c6,0x88)==UINT64_C(0x610feb01f813f643)&&map_hash68(r,0xb3,0x36)==UINT64_C(0x2ba6cf9e950f5246)&&r[0x1001a]==0xc6&&r[0x1001b]==0xc5;}
typedef struct {const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,next,armed,fresh;uint8_t frame[7];} MapOwner68;
static MapOwner68 map_owner68;
static struct {uint64_t entries,instructions,deadlines,refused;} map_stats68;
static unsigned map_extra68;
/* pc, instruction length, base cycles, M, original saved-register depth. */
static const unsigned map_ops68[][5]={
 {0x02c5d5,2,3,1,0},
 {0x02c5d7,2,4,0,0},
 {0x02c5d9,1,2,0,0},
 {0x02c5da,1,2,0,0},
 {0x02c5db,3,5,0,0},
 {0x02c5de,3,5,0,0},
 {0x02c5e1,2,4,0,0},
 {0x02c5e3,1,2,0,0},
 {0x02c5e4,1,2,0,0},
 {0x02c5e5,3,5,0,0},
 {0x02c5e8,3,3,0,0},
 {0x02c5eb,3,5,0,0},
 {0x02c5ee,3,5,0,0},
 {0x02c5f1,1,3,0,0},
 {0x02c5f2,1,2,0,0},
 {0x02c5f3,1,4,0,0},
 {0x02c5f4,3,3,0,2},
 {0x02c5f7,2,3,0,2},
 {0x02c5f9,2,6,1,2},
 {0x02c5fb,1,3,1,2},
 {0x02c5fc,2,6,1,3},
 {0x02c5fe,2,3,1,3},
 {0x02c600,1,2,0,3},
 {0x02c601,1,2,0,3},
 {0x02c602,1,2,0,3},
 {0x02c603,2,7,0,3},
 {0x02c605,4,6,0,3},
 {0x02c609,1,2,0,3},
 {0x02c60a,1,2,0,3},
 {0x02c60b,2,7,0,3},
 {0x02c60d,4,6,0,3},
 {0x02c611,3,3,0,3},
 {0x02c614,2,3,0,3},
 {0x02c616,1,4,1,3},
 {0x02c617,2,3,1,2},
 {0x02c619,1,2,0,2},
 {0x02c61a,1,2,0,2},
 {0x02c61b,1,2,0,2},
 {0x02c61c,2,7,0,2},
 {0x02c61e,4,6,0,2},
 {0x02c622,1,2,0,2},
 {0x02c623,1,2,0,2},
 {0x02c624,2,7,0,2},
 {0x02c626,4,6,0,2},
 {0x02c62a,1,5,0,2},
 {0x02c62b,1,2,0,0},
 {0x02c62c,1,2,0,0},
 {0x02c62d,1,2,0,0},
 {0x02c62e,3,5,0,0},
 {0x02c631,2,0,0,0},
 {0x02c633,3,5,0,0},
 {0x02c636,1,2,0,0},
 {0x02c637,3,5,0,0},
 {0x02c63a,3,5,0,0},
 {0x02c63d,3,8,0,0},
 {0x02c640,3,5,0,0},
 {0x02c643,3,5,0,0},
 {0x02c646,2,0,0,0},
 {0x02c648,3,3,0,0},
 {0x02c64b,2,3,0,0},
};
static unsigned map_offset68(unsigned pc){return (pc>>16)*32768+(pc&32767);}
static int map_mapped68(unsigned pc){
 if(pc==0x0080e6){for(unsigned j=0;j<0x36;j++)if(!cpu_aot_rom_mapping_matches(0x0080b3+j,0xb3+j,1))return 0;if(!cpu_aot_rom_mapping_matches(0x02801a,0x1001a,2))return 0;for(unsigned j=0;j<0x88;j++)if(!cpu_aot_rom_mapping_matches(0x02c5c6+j,0x145c6+j,1))return 0;return 1;}
 if(pc==0x02c64d)return cpu_aot_rom_mapping_matches(pc,map_offset68(pc),1);
 for(unsigned j=0;j<sizeof map_ops68/sizeof*map_ops68;j++)if(map_ops68[j][0]==pc)return cpu_aot_rom_mapping_matches(pc,map_offset68(pc),map_ops68[j][1]);return 0;
}
static int map_mode68(const CpuState*c){return c&&c->ram&&c->DB==2&&!c->D&&!c->emulation&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x1c)==4&&!!(c->P&32)==!!c->m_flag;}
/* Actual paired native JSL80E0 entry metadata, before its original JML.
 * Observation changes no guest instruction, stack, register, clock or RAM. */
static int map_observe68(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,unsigned s,unsigned h,unsigned host){
 map_owner68.armed=map_owner68.fresh=0;
 if(c!=primary||gen!=frame||!map_mode68(c)||!c->m_flag||c->PB||c->S!=s||s<0x120||s>0x1ff8||h!=3||c->host_return_valid!=3||host!=0x0080e4||c->ram[s+1]!=0xe3||c->ram[s+2]!=0x80||c->ram[s+3]!=0||c->ram[s+4]>2||c->ram[0x10]!=0xc6||c->ram[0x11]!=0xc5||c->ram[0x12]!=2||c->ram[0x13]!=13||!LagoonMap68_Source(r,n)||!map_mapped68(0x0080e6))return 0;
 map_owner68.cpu=c;map_owner68.ram=c->ram;map_owner68.gen=gen;map_owner68.s=s;map_owner68.next=0x02c5d5;memcpy(map_owner68.frame,c->ram+s+1,7);map_owner68.armed=map_owner68.fresh=1;map_stats68.entries++;return 1;
}
static void map_nz68(CpuState*c,unsigned v,unsigned bits){unsigned mask=bits==8?255:65535;c->_flag_Z=!(v&mask);c->_flag_N=!!(v&(1u<<(bits-1)));cpu_mirrors_to_p(c);}
static void map_a68(CpuState*c,unsigned v){c->A=c->m_flag?((c->A&0xff00)|(v&255)):(v&65535);map_nz68(c,v,c->m_flag?8:16);}
static unsigned map_rd68(CpuState*c,unsigned p,unsigned n){uint8_t b;uint16_t v;p&=0xffffff;if(n==1){if(!interp_bridge_source_read8(c,p,&b))abort();map_extra68+=cpu_region_speed(p)-6;return b;}unsigned high=(p+1)&0xffffff;if(!interp_bridge_source_read16(c,p,high,&v))abort();map_extra68+=cpu_region_speed(p)-6+cpu_region_speed(high)-6;return v;}
static void map_wr68(CpuState*c,unsigned p,unsigned v,unsigned n){p&=0xffffff;if(n==1){if(!interp_bridge_source_write8(c,p,v))abort();map_extra68+=cpu_region_speed(p)-6;}else{unsigned high=(p+1)&0xffffff;if(!interp_bridge_source_write16(c,p,high,v))abort();map_extra68+=cpu_region_speed(p)-6+cpu_region_speed(high)-6;}}
int LagoonMap68_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,uint32_t pc,uint32_t*landing){
 MapOwner68*w=&map_owner68;
 if(!landing||c!=primary||gen!=frame||!map_mode68(c)||c->PB!=2||!LagoonMap68_Source(r,n)){w->armed=w->fresh=0;map_stats68.refused++;return 0;}
 if(pc==0x02c64d){w->armed=w->fresh=0;return 0;}
 if(!w->armed||w->cpu!=c||w->ram!=c->ram||w->gen!=gen||w->next!=pc||memcmp(w->frame,c->ram+w->s+1,7)){w->armed=w->fresh=0;map_stats68.refused++;return 0;}
 const unsigned*op=NULL;for(unsigned j=0;j<sizeof map_ops68/sizeof*map_ops68;j++)if(map_ops68[j][0]==pc){op=map_ops68[j];break;}
 if(!op||c->m_flag!=op[3]||c->S+op[4]!=w->s||(w->fresh&&(pc!=0x02c5d5||c->X!=0x8000))){w->armed=w->fresh=0;map_stats68.refused++;return 0;}
 if(interp_bridge_source_opcode_deadline_reached(c)){map_stats68.deadlines++;return 0;}
 unsigned at=map_offset68(pc),bytes=op[1],cycles=op[2],im=bytes>1?r[at+1]:0,operand=im|(bytes>2?r[at+2]<<8:0),next=pc+bytes,width=c->m_flag?1:2;uint64_t prior=c->master_cycles;map_extra68=0;c->coprocessor_master_cycles=prior;c->open_bus=r[at+bytes-1];w->fresh=0;
 if(!cycles){cycles=c->_flag_Z?2:3;if(!c->_flag_Z)next=pc+bytes+(int8_t)im;}
 CpuWramReadContext context={.instruction_pc=pc,.stack=c->S,.y=c->Y,.pb=c->PB,.m=c->m_flag,.x=c->x_flag,.e=c->emulation,.interpreter=1};const CpuWramReadContext*old=cpu_swap_wram_read_context(&context);
 switch(r[at]){
 case 0xc2:c->P&=~im;cpu_p_to_mirrors(c);break;
 case 0xe2:c->P|=im;cpu_p_to_mirrors(c);break;
 case 0xa5:map_a68(c,map_rd68(c,im,width));break;
 case 0xad:map_a68(c,map_rd68(c,0x020000|operand,width));break;
 case 0x8d:map_wr68(c,0x020000|operand,c->A,width);break;
 case 0x9c:map_wr68(c,0x020000|operand,0,width);break;
 case 0x4a:c->_flag_C=c->A&1;map_a68(c,(c->A&(c->m_flag?255:65535))>>1);break;
 case 0x0a:c->_flag_C=!!(c->A&(c->m_flag?128:32768));map_a68(c,c->A<<1);break;
 case 0xa0:c->Y=operand;map_nz68(c,c->Y,16);break;
 case 0xeb:c->A=(uint16_t)((c->A<<8)|(c->A>>8));map_nz68(c,c->A,8);break;
 case 0xaa:c->X=c->A;map_nz68(c,c->X,16);break;
 case 0xa8:c->Y=c->A;map_nz68(c,c->Y,16);break;
 case 0x5a:map_wr68(c,c->S,c->Y>>8,1);c->S--;map_wr68(c,c->S,c->Y,1);c->S--;break;
 case 0x7a:c->Y=map_rd68(c,c->S+1,2);c->S+=2;map_nz68(c,c->Y,16);break;
 case 0x48:map_wr68(c,c->S,c->A,1);c->S--;break;
 case 0x68:c->S++;map_a68(c,map_rd68(c,c->S,1));break;
 case 0x29:map_a68(c,c->A&operand);break;
 case 0xb7:{unsigned p=map_rd68(c,im,1);p|=map_rd68(c,im+1,1)<<8;p|=map_rd68(c,im+2,1)<<16;map_a68(c,map_rd68(c,(p+c->Y)&0xffffff,width));break;}
 case 0x9f:{unsigned p=operand|(r[at+3]<<16);map_wr68(c,(p+c->X)&0xffffff,c->A,width);break;}
 case 0xc8:c->Y=(uint16_t)(c->Y+1);map_nz68(c,c->Y,16);break;
 case 0xe8:c->X=(uint16_t)(c->X+1);map_nz68(c,c->X,16);break;
 case 0xcc:{unsigned v=map_rd68(c,0x020000|operand,2);c->_flag_C=c->Y>=v;map_nz68(c,c->Y-v,16);break;}
 case 0xcd:{unsigned v=map_rd68(c,0x020000|operand,width);c->_flag_C=(c->A&(width==1?255:65535))>=v;map_nz68(c,c->A-v,width==1?8:16);break;}
 case 0x18:c->_flag_C=0;cpu_mirrors_to_p(c);break;
 case 0x6d:{unsigned x=c->A,v=map_rd68(c,0x020000|operand,2),sum=x+v+c->_flag_C;c->_flag_C=sum>65535;c->_flag_V=!!((~(x^v)&(x^sum))&32768);map_a68(c,sum);break;}
 case 0xee:{unsigned p=0x020000|operand,v=(map_rd68(c,p,2)+1)&65535;/* Original RMW word writes high byte first. */map_wr68(c,p+1,v>>8,1);map_wr68(c,p,v,1);map_nz68(c,v,16);break;}
 case 0xd0:break;
 default:abort();
 }
 cpu_swap_wram_read_context(old);c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2+map_extra68;w->next=next;map_stats68.instructions++;if(!interp_bridge_commit_source_opcode(c,prior,false))abort();*landing=next;return 1;
}
static void map_retain68(CpuState*c,uint64_t gen,unsigned pc){MapOwner68*w=&map_owner68;if(w->armed&&(!c||w->cpu!=c||w->ram!=c->ram||w->gen!=gen||c->S>w->s||memcmp(w->frame,c->ram+w->s+1,7)||(!((pc>=0x02c5c6&&pc<=0x02c64d)||(w->fresh&&pc==0x0080e6)))||(pc>=0x02c5d5&&pc<0x02c64d&&pc!=w->next)))w->armed=w->fresh=0;}
#ifndef LAGOON_MAP68_TEST
#include "common_rtl.h"
#include <stdio.h>
static uint64_t map_generation68;static int map_enabled68=-1,map_report68;
static void map_exit68(void){if(map_report68)fprintf(stderr,"[native-map68] entries=%llu instructions=%llu deadlines=%llu refused=%llu\n",(unsigned long long)map_stats68.entries,(unsigned long long)map_stats68.instructions,(unsigned long long)map_stats68.deadlines,(unsigned long long)map_stats68.refused);}
bool LagoonMap68_DispatchEntry(CpuState*c,uint16_t s,uint8_t h,uint32_t host,RecompReturn*out){(void)out;if(map_enabled68)map_observe68(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),map_generation68,s,h,host);return false;}
static void map_hook68(CpuState*c,uint32_t pc){if(!map_enabled68||c!=&g_cpu||!interp_bridge_source_opcode_available(c))return;if(!map_mapped68(pc)){map_owner68.armed=map_owner68.fresh=0;return;}uint32_t landing;if(LagoonMap68_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),map_generation68,pc,&landing))interp_bridge_pre_opcode_redirect(landing);}
void LagoonMap68_BeforeFrame(void){if(map_enabled68<0){const char*v=getenv("LAGOON_TIMING68");map_enabled68=!v||strcmp(v,"0");v=getenv("LAGOON_NATIVE_WAIT_REPORT");map_report68=v&&!strcmp(v,"1");if(map_report68)atexit(map_exit68);}map_generation68=RtlStateGeneration();map_retain68(&g_cpu,map_generation68,interp_bridge_lle_resume_pc());if(map_enabled68){for(unsigned j=0;j<sizeof map_ops68/sizeof*map_ops68;j++)if(!interp_bridge_add_pre_opcode_hook(map_ops68[j][0],map_hook68))abort();if(!interp_bridge_add_pre_opcode_hook(0x02c64d,map_hook68))abort();}}
#endif
