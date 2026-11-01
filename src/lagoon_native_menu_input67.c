/* Original finite menu eligibility/input/selection parent. Real child calls,
 * sound, input sampling, dispatch and RTL are interpreter-owned boundaries.
 * B369..B370 is an inline pointer table: fingerprinted DATA, never executed. */
#include "lagoon_native_menu_input67.h"
#include "snes/interp_bridge.h"
#include <string.h>
#include <stdlib.h>
static uint64_t hash_menu67(const uint8_t*r,unsigned at,unsigned n){uint64_t h=UINT64_C(14695981039346656037);for(unsigned j=0;j<n;j++)h=(h^r[at+j])*UINT64_C(1099511628211);return h;}
int LagoonMenuInput67_Source(const uint8_t*r,size_t n){return r&&n==0x100000&&hash_menu67(r,0x132f6,0x7b)==UINT64_C(0x72e25035367c10a7)&&hash_menu67(r,0xb3,0x36)==UINT64_C(0x2ba6cf9e950f5246)&&r[0x10002]==0xf6&&r[0x10003]==0xb2;}
typedef struct{const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,next;uint8_t frame[7];int armed;} MenuOwner67;
static MenuOwner67 menu67;
static struct{uint64_t entries,input,selection,setup,deadlines,refused;} menu_stats67;
static int mode_menu67(const CpuState*c){return c&&c->ram&&c->PB==2&&c->DB==2&&!c->D&&!c->emulation&&c->m_flag&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x3c)==0x24;}
static int arm_menu67(CpuState*c,uint64_t gen){memset(&menu67,0,sizeof menu67);if(!mode_menu67(c)||c->S<0x120||c->S>0x1ff8||c->ram[c->S+1]!=0xe3||c->ram[c->S+2]!=0x80||c->ram[c->S+3]!=0||c->ram[c->S+4]>2)return 0;menu67.cpu=c;menu67.ram=c->ram;menu67.gen=gen;menu67.s=c->S;menu67.next=0x02b2f6;memcpy(menu67.frame,c->ram+c->S+1,7);menu67.armed=1;menu_stats67.entries++;return 1;}
/* pc, source bytes, original base cycles. */
static const unsigned menu_ops67[][3]={
 {0x02b2f6,3,4},{0x02b2f9,2,2},{0x02b2fb,2,0},{0x02b2fd,3,4},{0x02b300,2,2},{0x02b302,2,0},
 {0x02b308,2,2},{0x02b30a,3,4},{0x02b310,3,4},
 {0x02b31a,3,4},{0x02b31d,2,0},{0x02b31f,3,4},{0x02b322,2,2},{0x02b324,2,0},{0x02b326,3,4},{0x02b329,2,2},{0x02b32b,2,0},
 {0x02b32d,2,2},{0x02b32f,2,0},{0x02b331,3,6},{0x02b334,2,3},{0x02b336,3,6},{0x02b339,2,2},
 {0x02b33f,2,2},{0x02b341,3,6},{0x02b347,2,3},{0x02b34c,2,2},{0x02b34e,3,4},{0x02b351,2,2},{0x02b357,2,2},{0x02b362,3,4}};
static const unsigned menu_calls67[][3]={
 {0x02b304,4,0x02b308},{0x02b30d,3,0x02b310},{0x02b313,3,0x02b316},{0x02b316,4,0x02b31a},
 {0x02b33b,4,0x02b33f},{0x02b344,3,0x02b347},{0x02b349,3,0x02b34c},{0x02b353,4,0x02b357},{0x02b359,4,0x02b35d},{0x02b35d,4,0x02b361},{0x02b365,4,0}};
static unsigned menu_offset67(unsigned pc){return (pc>>16)*32768+(pc&32767);}
static void menu_nz67(CpuState*c,unsigned v){c->_flag_Z=!(v&255);c->_flag_N=!!(v&128);cpu_mirrors_to_p(c);}
static void menu_a67(CpuState*c,unsigned v){c->A=(c->A&0xff00)|(v&255);menu_nz67(c,v);}
static unsigned menu_read67(CpuState*c,unsigned p){uint8_t v;if(!interp_bridge_source_read8(c,p,&v))abort();c->master_cycles+=cpu_region_speed(p)-6;return v;}
static void menu_write67(CpuState*c,unsigned p,unsigned v){if(!interp_bridge_source_write8(c,p,v))abort();c->master_cycles+=cpu_region_speed(p)-6;}
int LagoonMenuInput67_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,uint32_t pc,uint32_t*landing){
 if(!landing||c!=primary||gen!=frame||!mode_menu67(c)||!LagoonMenuInput67_Source(r,n)){menu67.armed=0;menu_stats67.refused++;return 0;}
 if(pc==0x02b2f6&&!arm_menu67(c,gen))return 0;
 if(!menu67.armed||menu67.cpu!=c||menu67.ram!=c->ram||menu67.gen!=gen||menu67.s!=c->S||menu67.next!=pc||memcmp(menu67.frame,c->ram+c->S+1,7)){menu67.armed=0;menu_stats67.refused++;return 0;}
 if(pc==0x02b361){menu67.armed=0;return 0;}
 for(unsigned j=0;j<sizeof menu_calls67/sizeof*menu_calls67;j++)if(menu_calls67[j][0]==pc){menu67.next=menu_calls67[j][2];if(!menu67.next)menu67.armed=0;return 0;}
 const unsigned*op=NULL;for(unsigned j=0;j<sizeof menu_ops67/sizeof*menu_ops67;j++)if(menu_ops67[j][0]==pc){op=menu_ops67[j];break;}if(!op){menu67.armed=0;return 0;}
 if(interp_bridge_source_opcode_deadline_reached(c)){menu_stats67.deadlines++;return 0;}
 unsigned at=menu_offset67(pc),bytes=op[1],cycles=op[2],im=r[at+1],operand=im|(bytes==3?r[at+2]<<8:0),address=0x020000|operand,next=pc+bytes;uint64_t prior=c->master_cycles;
 if(!cycles){unsigned take=pc==0x02b31d?c->_flag_N:(pc==0x02b2fb||pc==0x02b32b)?c->_flag_Z:!c->_flag_Z;cycles=take?3:2;if(take)next=pc+bytes+(int8_t)im;}
 c->coprocessor_master_cycles=prior;c->open_bus=r[at+bytes-1];
 CpuWramReadContext context={.instruction_pc=pc,.stack=c->S,.y=c->Y,.pb=c->PB,.m=c->m_flag,.x=c->x_flag,.e=c->emulation,.interpreter=1};const CpuWramReadContext*old=cpu_swap_wram_read_context(&context);
 switch(pc){
 case 0x02b2f6:case 0x02b2fd:case 0x02b31a:case 0x02b326:case 0x02b362:menu_a67(c,menu_read67(c,address));break;
 case 0x02b2f9:case 0x02b300:case 0x02b329:menu_a67(c,c->A&im);break;
 case 0x02b308:case 0x02b339:case 0x02b33f:case 0x02b34c:case 0x02b351:case 0x02b357:menu_a67(c,im);break;
 case 0x02b30a:case 0x02b34e:menu_write67(c,address,c->A);break;
 case 0x02b310:menu_write67(c,address,0);break;
 case 0x02b31f:menu_a67(c,c->A|menu_read67(c,address));break;
 case 0x02b322:case 0x02b32d:c->_flag_Z=!(c->A&im&255);cpu_mirrors_to_p(c);break;
 case 0x02b331:case 0x02b336:{unsigned v=(menu_read67(c,address)+(pc==0x02b331?-1:1))&255;menu_write67(c,address,v);menu_nz67(c,v);break;}
 case 0x02b341:{unsigned v=menu_read67(c,address);c->_flag_Z=!(v&c->A&255);cpu_mirrors_to_p(c);menu_write67(c,address,v&~c->A);break;}
 case 0x02b334:case 0x02b347:next=pc+bytes+(int8_t)im;break;
 default:break;
 }
 cpu_swap_wram_read_context(old);c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2;menu67.next=next;
 if(pc>=0x02b31a&&pc<=0x02b32b)menu_stats67.input++;else if(pc>=0x02b32d&&pc<=0x02b347)menu_stats67.selection++;else menu_stats67.setup++;
 if(!interp_bridge_commit_source_opcode(c,prior,false))abort();*landing=next;return 1;
}
static int menu_mapped67(unsigned pc){if(pc==0x02b2f6){const unsigned spans[][2]={{0x02b2f6,0x7b},{0x0080b3,0x36},{0x028002,2}};for(unsigned j=0;j<3;j++)for(unsigned k=0;k<spans[j][1];k++)if(!cpu_aot_rom_mapping_matches(spans[j][0]+k,menu_offset67(spans[j][0])+k,1))return 0;return 1;}if(pc==0x02b361)return cpu_aot_rom_mapping_matches(pc,menu_offset67(pc),1);for(unsigned j=0;j<sizeof menu_ops67/sizeof*menu_ops67;j++)if(menu_ops67[j][0]==pc)return cpu_aot_rom_mapping_matches(pc,menu_offset67(pc),menu_ops67[j][1]);for(unsigned j=0;j<sizeof menu_calls67/sizeof*menu_calls67;j++)if(menu_calls67[j][0]==pc)return cpu_aot_rom_mapping_matches(pc,menu_offset67(pc),menu_calls67[j][1]);return 0;}
#ifndef LAGOON_MENU_INPUT67_TEST
#include "common_rtl.h"
#include <stdio.h>
static uint64_t menu_generation67;static int menu_enabled67=-1,menu_report67;
static void menu_report_exit67(void){if(menu_report67)fprintf(stderr,"[native-menu-input67] entries=%llu input=%llu selection=%llu setup=%llu deadlines=%llu refused=%llu\n",(unsigned long long)menu_stats67.entries,(unsigned long long)menu_stats67.input,(unsigned long long)menu_stats67.selection,(unsigned long long)menu_stats67.setup,(unsigned long long)menu_stats67.deadlines,(unsigned long long)menu_stats67.refused);}
static void menu_hook67(CpuState*c,uint32_t pc){if(!menu_enabled67||c!=&g_cpu||!interp_bridge_source_opcode_available(c))return;if(!menu_mapped67(pc)){menu67.armed=0;return;}uint32_t landing;if(LagoonMenuInput67_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),menu_generation67,pc,&landing))interp_bridge_pre_opcode_redirect(landing);}
void LagoonMenuInput67_BeforeFrame(void){if(menu_enabled67<0){const char*v=getenv("LAGOON_TIMING67");menu_enabled67=!v||strcmp(v,"0");v=getenv("LAGOON_NATIVE_WAIT_REPORT");menu_report67=v&&!strcmp(v,"1");if(menu_report67)atexit(menu_report_exit67);}menu_generation67=RtlStateGeneration();if(menu67.gen!=menu_generation67)menu67.armed=0;if(menu_enabled67){for(unsigned j=0;j<sizeof menu_ops67/sizeof*menu_ops67;j++)interp_bridge_add_pre_opcode_hook(menu_ops67[j][0],menu_hook67);for(unsigned j=0;j<sizeof menu_calls67/sizeof*menu_calls67;j++)interp_bridge_add_pre_opcode_hook(menu_calls67[j][0],menu_hook67);interp_bridge_add_pre_opcode_hook(0x02b361,menu_hook67);}}
#endif
