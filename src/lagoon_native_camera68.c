/* The original/approved centered camera dispatcher, one source opcode at a
 * time. Original helpers, RTS and the existing 83F3 tail owner stay intact. */
#include "lagoon_native_camera68.h"
#include "snes/interp_bridge.h"
#include <stdlib.h>
#include <string.h>
static uint64_t camera_hash68(const uint8_t*r,unsigned at,unsigned n){uint64_t h=UINT64_C(14695981039346656037);for(unsigned j=0;j<n;j++)h=(h^r[at+j])*UINT64_C(1099511628211);return h;}
/* Return 1 for retail, 2 for the exact approved camera patch. */
int LagoonCamera68_Source(const uint8_t*r,size_t n){if(!r||n!=0x100000)return 0;uint64_t main=camera_hash68(r,0x807a,0x6d),body=camera_hash68(r,0x83d8,0xcf);if(main==UINT64_C(0xd9eab56ea8b810fb)&&body==UINT64_C(0x08e89eb1796bd8ae))return 1;if(main==UINT64_C(0x6ea0a0f489b2db1f)&&body==UINT64_C(0x5cca54139931d2a1))return 2;return 0;}
typedef struct{const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,next,variant,armed;uint8_t frame[2];}CameraOwner68;
static CameraOwner68 camera68;
static struct{const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,variant,armed;}camera_call68;
static struct{uint64_t entries,stock,center,deadlines,refused;}camera_stats68;
static unsigned camera_extra68;
/* pc, source bytes, base cycles, exact saved-register depth; tables are DATA. */
static const unsigned camera_stock_ops68[][4]={
 {0x0183d8,2,3,0},{0x0183da,2,3,0},{0x0183dc,3,4,0},{0x0183df,2,0,0},
 {0x0183e1,1,4,0},{0x0183e2,1,2,2},{0x0183e3,1,2,2},{0x0183e4,3,5,2},
 {0x0183e7,2,3,2},{0x0183e9,3,5,2},{0x0183ec,2,3,2},{0x0183ee,1,5,2}};
static const unsigned camera_center_ops68[][4]={
 {0x0183d8,1,4,0},{0x0183d9,1,2,2},{0x0183da,2,0,2},{0x0183dc,1,4,2},
 {0x0183dd,2,3,4},{0x0183eb,1,5,4},{0x0183ec,3,4,2},{0x0183ef,3,4,2},{0x0183f2,1,5,2}};
static int camera_mode68(const CpuState*c){return c&&c->ram&&c->PB==1&&c->DB==1&&!c->D&&!c->emulation&&c->m_flag&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x3c)==0x24;}
static const unsigned*camera_op68(unsigned variant,unsigned pc){const unsigned(*ops)[4]=variant==1?camera_stock_ops68:camera_center_ops68;unsigned count=variant==1?sizeof camera_stock_ops68/sizeof*camera_stock_ops68:sizeof camera_center_ops68/sizeof*camera_center_ops68;for(unsigned j=0;j<count;j++)if(ops[j][0]==pc)return ops[j];return NULL;}
static void camera_nz68(CpuState*c,unsigned v,unsigned bits){unsigned mask=bits==8?255:65535;c->_flag_Z=!(v&mask);c->_flag_N=!!(v&(1u<<(bits-1)));cpu_mirrors_to_p(c);}
static void camera_a68(CpuState*c,unsigned v){c->A=(c->A&0xff00)|(v&255);camera_nz68(c,v,8);}
static unsigned camera_read68(CpuState*c,unsigned a){uint8_t v;if(!interp_bridge_source_read8(c,a,&v))abort();camera_extra68+=cpu_region_speed(a)-6;return v;}
static void camera_write68(CpuState*c,unsigned a,unsigned v){if(!interp_bridge_source_write8(c,a,v))abort();camera_extra68+=cpu_region_speed(a)-6;}
static void camera_push68(CpuState*c,unsigned v){camera_write68(c,c->S--,v>>8);camera_write68(c,c->S--,v);}
static unsigned camera_pull68(CpuState*c){unsigned v=camera_read68(c,++c->S);return v|camera_read68(c,++c->S)<<8;}
int LagoonCamera68_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,uint32_t pc,uint32_t*landing){
 unsigned variant=LagoonCamera68_Source(r,n);if(!landing||c!=primary||gen!=frame||!camera_mode68(c)||!variant){camera68.armed=camera_call68.armed=0;camera_stats68.refused++;return 0;}
 unsigned caller=variant==1?0x01809d:0x0180ca,return_pc=caller+3;
 if(pc==0x01809d||pc==0x0180ca){camera68.armed=camera_call68.armed=0;if(pc==caller&&c->S>=0x122&&c->S<=0x1fff){camera_call68.cpu=c;camera_call68.ram=c->ram;camera_call68.gen=gen;camera_call68.s=c->S;camera_call68.variant=variant;camera_call68.armed=1;}return 0;}
 if(pc==0x0180a0||pc==0x0180cd){camera68.armed=camera_call68.armed=0;return 0;}
 if(pc==0x0183d8&&!(camera68.armed&&camera68.next==pc)){unsigned ok=camera_call68.armed&&camera_call68.cpu==c&&camera_call68.ram==c->ram&&camera_call68.gen==gen&&camera_call68.variant==variant&&camera_call68.s==c->S+2&&c->S>=0x120&&c->S<=0x1ffd&&(c->ram[c->S+1]|c->ram[c->S+2]<<8)==((return_pc-1)&65535);camera_call68.armed=0;camera68.armed=0;if(!ok){camera_stats68.refused++;return 0;}camera68.cpu=c;camera68.ram=c->ram;camera68.gen=gen;camera68.s=c->S;camera68.next=pc;camera68.variant=variant;memcpy(camera68.frame,c->ram+c->S+1,2);camera68.armed=1;camera_stats68.entries++;}
 if(!camera68.armed||camera68.cpu!=c||camera68.ram!=c->ram||camera68.gen!=gen||camera68.variant!=variant||camera68.next!=pc||memcmp(camera68.frame,c->ram+camera68.s+1,2)){camera68.armed=0;camera_stats68.refused++;return 0;}
 /* Keep the existing source tail/ancestor observer and original JMP/RTS. */
 if((variant==1&&(pc==0x0183ef||pc==0x0183fa))||(variant==2&&pc==0x0183f3)){camera68.armed=0;return 0;}
 if(variant==2&&(pc==0x0183df||pc==0x0183e2||pc==0x0183e5||pc==0x0183e8)){if(c->S+4!=camera68.s){camera68.armed=0;return 0;}camera68.next=pc+3;return 0;}
 const unsigned*op=camera_op68(variant,pc);if(!op||c->S+op[3]!=camera68.s){camera68.armed=0;camera_stats68.refused++;return 0;}
 if(interp_bridge_source_opcode_deadline_reached(c)){camera_stats68.deadlines++;return 0;}
 unsigned at=pc&65535,bytes=op[1],cycles=op[2],im=bytes>1?r[at+1]:0,operand=im|(bytes==3?r[at+2]<<8:0),next=pc+bytes;uint64_t prior=c->master_cycles;camera_extra68=0;c->coprocessor_master_cycles=prior;c->open_bus=r[at+bytes-1];
 if(!cycles){unsigned take=variant==1?c->_flag_N:!c->_flag_Z;cycles=take?3:2;if(take)next=pc+bytes+(int8_t)im;}
 CpuWramReadContext context={.instruction_pc=pc,.stack=c->S,.y=c->Y,.pb=c->PB,.m=c->m_flag,.x=c->x_flag,.e=c->emulation,.interpreter=1};const CpuWramReadContext*old=cpu_swap_wram_read_context(&context);
 if(variant==1)switch(pc){
 case 0x0183d8:case 0x0183da:camera_write68(c,im,0);break;
 case 0x0183dc:camera_a68(c,camera_read68(c,0x010000|operand));break;
 case 0x0183e1:camera_push68(c,c->Y);break;
 case 0x0183e2:c->_flag_C=!!(c->A&128);camera_a68(c,c->A<<1);break;
 case 0x0183e3:c->Y=c->A;camera_nz68(c,c->Y,16);break;
 case 0x0183e4:case 0x0183e9:camera_a68(c,camera_read68(c,(0x010000+operand+c->Y)&0xffffff));break;
 case 0x0183e7:case 0x0183ec:camera_write68(c,im,c->A);break;
 case 0x0183ee:c->Y=camera_pull68(c);camera_nz68(c,c->Y,16);break;
 default:break;
 }else switch(pc){
 case 0x0183d8:camera_push68(c,c->Y);break;
 case 0x0183d9:c->Y=c->X;camera_nz68(c,c->Y,16);break;
 case 0x0183dc:camera_push68(c,c->X);break;
 case 0x0183dd:camera_write68(c,im,0);break;
 case 0x0183eb:c->X=camera_pull68(c);camera_nz68(c,c->X,16);break;
 case 0x0183ec:camera_a68(c,camera_read68(c,0x010000|operand));break;
 case 0x0183ef:camera_write68(c,0x010000|operand,c->A);break;
 case 0x0183f2:c->Y=camera_pull68(c);camera_nz68(c,c->Y,16);break;
 default:break;
 }
 cpu_swap_wram_read_context(old);c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2+camera_extra68;camera68.next=next;if(variant==1)camera_stats68.stock++;else camera_stats68.center++;if(!interp_bridge_commit_source_opcode(c,prior,false))abort();*landing=next;return 1;
}
static int camera_mapped68(unsigned variant,unsigned pc){
 if(pc==0x01809d||pc==0x0180ca||pc==0x0183d8){for(unsigned k=0;k<0x6d;k++)if(!cpu_aot_rom_mapping_matches(0x01807a+k,0x807a+k,1))return 0;for(unsigned k=0;k<0xcf;k++)if(!cpu_aot_rom_mapping_matches(0x0183d8+k,0x83d8+k,1))return 0;return 1;}
 if(pc==0x0180a0||pc==0x0180cd)return cpu_aot_rom_mapping_matches(pc,pc&65535,1);
 if(variant==2&&(pc==0x0183df||pc==0x0183e2||pc==0x0183e5||pc==0x0183e8||pc==0x0183f3))return cpu_aot_rom_mapping_matches(pc,pc&65535,3);
 if(variant==1&&(pc==0x0183ef||pc==0x0183fa))return cpu_aot_rom_mapping_matches(pc,pc&65535,pc==0x0183fa?1:3);
 const unsigned*op=camera_op68(variant,pc);return op&&cpu_aot_rom_mapping_matches(pc,pc&65535,op[1]);
}
#ifndef LAGOON_CAMERA68_TEST
#include "common_rtl.h"
#include <stdio.h>
static uint64_t camera_generation68;static int camera_enabled68=-1,camera_report68;
static void camera_exit68(void){if(camera_report68)fprintf(stderr,"[native-camera68] entries=%llu stock=%llu center=%llu deadlines=%llu refused=%llu\n",(unsigned long long)camera_stats68.entries,(unsigned long long)camera_stats68.stock,(unsigned long long)camera_stats68.center,(unsigned long long)camera_stats68.deadlines,(unsigned long long)camera_stats68.refused);}
static void camera_hook68(CpuState*c,uint32_t pc){if(!camera_enabled68||c!=&g_cpu||!interp_bridge_source_opcode_available(c))return;unsigned variant=LagoonCamera68_Source(g_rom,0x100000);if(!variant||!camera_mapped68(variant,pc)){camera68.armed=camera_call68.armed=0;return;}uint32_t landing;if(LagoonCamera68_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),camera_generation68,pc,&landing))interp_bridge_pre_opcode_redirect(landing);}
void LagoonCamera68_BeforeFrame(void){if(camera_enabled68<0){const char*v=getenv("LAGOON_TIMING68");camera_enabled68=!v||strcmp(v,"0");v=getenv("LAGOON_NATIVE_WAIT_REPORT");camera_report68=v&&!strcmp(v,"1");if(camera_report68)atexit(camera_exit68);}camera_generation68=RtlStateGeneration();if(camera68.gen!=camera_generation68)camera68.armed=0;if(camera_call68.gen!=camera_generation68)camera_call68.armed=0;if(camera_enabled68){unsigned variant=LagoonCamera68_Source(g_rom,0x100000);if(!variant)return;const unsigned observe[]={0x01809d,0x0180ca,0x0180a0,0x0180cd};for(unsigned j=0;j<sizeof observe/sizeof*observe;j++)if(!interp_bridge_add_pre_opcode_hook(observe[j],camera_hook68))abort();const unsigned boundaries[2]={variant==1?0x0183ef:0x0183f3,variant==1?0x0183fa:0x0183f3};for(unsigned j=0;j<2;j++)if(!interp_bridge_add_pre_opcode_hook(boundaries[j],camera_hook68))abort();const unsigned(*ops)[4]=variant==1?camera_stock_ops68:camera_center_ops68;unsigned count=variant==1?sizeof camera_stock_ops68/sizeof*camera_stock_ops68:sizeof camera_center_ops68/sizeof*camera_center_ops68;for(unsigned j=0;j<count;j++)if(!interp_bridge_add_pre_opcode_hook(ops[j][0],camera_hook68))abort();if(variant==2)for(unsigned pc=0x0183df;pc<=0x0183e8;pc+=3)if(!interp_bridge_add_pre_opcode_hook(pc,camera_hook68))abort();}}
#endif
