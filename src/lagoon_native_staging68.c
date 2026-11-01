/* Source-timed fallback continuations for already compiled player-pattern
 * staging. Keep native C4AF/C579, hardware setup, child calls and RTL intact. */
#include "lagoon_native_staging68.h"
#include "snes/interp_bridge.h"
#include <string.h>
#include <stdlib.h>
typedef struct {const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,db,next,armed;uint8_t frame[3];} StagingOwner68;
static StagingOwner68 staging68;
static struct {const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,db,pc,armed;} staging_call68;
static struct {uint64_t entries,body,sword,effects,deadlines,refused;} staging_stats68;
static unsigned staging_extra68;
int LagoonStaging68_Source(const uint8_t*r,size_t n){if(!r||n!=0x100000||memcmp(r+0x80e7,"\x22\xaf\xc4\x01",4)||memcmp(r+0xc6c7,"\x22\xaf\xc4\x01",4))return 0;uint64_t h=UINT64_C(14695981039346656037);for(unsigned j=0xc4af;j<0xc59f;j++)h=(h^r[j])*UINT64_C(1099511628211);return h==UINT64_C(0x1ceb89a531f9ea8b);}
/* pc, bytes, base cycles, M and exact temporary-PHA depth. All lookup data,
 * existing hardware setup, real child calls and terminal return are absent. */
static const unsigned staging_ops68[][5]={
 {0x01c4ce,3,6,0,0},
 {0x01c4d1,1,4,0,0},
 {0x01c4d2,3,3,0,2},
 {0x01c4d5,2,4,0,2},
 {0x01c4d7,3,6,0,2},
 {0x01c4da,1,5,0,2},
 {0x01c4db,3,3,0,0},
 {0x01c4e1,1,2,0,0},
 {0x01c4e2,1,2,0,0},
 {0x01c4e3,1,2,0,0},
 {0x01c4e4,1,2,0,0},
 {0x01c4e5,2,7,0,0},
 {0x01c4e7,2,4,0,0},
 {0x01c4e9,3,3,0,0},
 {0x01c4ec,2,0,0,0},
 {0x01c4ee,3,5,0,0},
 {0x01c4f1,3,3,0,0},
 {0x01c4f4,1,2,0,0},
 {0x01c4f5,1,2,0,0},
 {0x01c4f6,1,2,0,0},
 {0x01c4f7,1,2,0,0},
 {0x01c4f8,1,2,0,0},
 {0x01c4f9,3,3,0,0},
 {0x01c4fc,3,6,0,0},
 {0x01c4ff,1,4,0,0},
 {0x01c500,3,3,0,2},
 {0x01c503,2,4,0,2},
 {0x01c505,3,6,0,2},
 {0x01c508,1,5,0,2},
 {0x01c509,3,3,0,0},
 {0x01c50f,1,2,0,0},
 {0x01c510,1,2,0,0},
 {0x01c511,2,7,0,0},
 {0x01c513,1,2,0,0},
 {0x01c514,1,2,0,0},
 {0x01c515,3,3,0,0},
 {0x01c518,2,0,0,0},
 {0x01c51a,3,5,0,0},
 {0x01c51d,3,3,0,0},
 {0x01c520,1,2,0,0},
 {0x01c521,1,2,0,0},
 {0x01c522,1,2,0,0},
 {0x01c523,1,2,0,0},
 {0x01c524,1,2,0,0},
 {0x01c525,3,3,0,0},
 {0x01c528,3,6,0,0},
 {0x01c52b,1,4,0,0},
 {0x01c52c,3,3,0,2},
 {0x01c52f,2,4,0,2},
 {0x01c531,3,6,0,2},
 {0x01c534,1,5,0,2},
 {0x01c535,3,3,0,0},
 {0x01c53b,1,2,0,0},
 {0x01c53c,1,2,0,0},
 {0x01c53d,2,7,0,0},
 {0x01c53f,1,2,0,0},
 {0x01c540,1,2,0,0},
 {0x01c541,3,3,0,0},
 {0x01c544,2,0,0,0},
 {0x01c546,3,5,0,0},
 {0x01c549,3,3,0,0},
 {0x01c54c,1,2,0,0},
 {0x01c54d,1,2,0,0},
 {0x01c54e,1,2,0,0},
 {0x01c54f,1,2,0,0},
 {0x01c550,3,3,0,0},
 {0x01c553,3,6,0,0},
 {0x01c556,1,4,0,0},
 {0x01c557,3,3,0,2},
 {0x01c55a,2,4,0,2},
 {0x01c55c,3,6,0,2},
 {0x01c55f,1,5,0,2},
 {0x01c560,3,3,0,0},
 {0x01c566,1,2,0,0},
 {0x01c567,1,2,0,0},
 {0x01c568,2,7,0,0},
 {0x01c56a,1,2,0,0},
 {0x01c56b,1,2,0,0},
 {0x01c56c,3,3,0,0},
 {0x01c56f,2,0,0,0},
 {0x01c571,2,3,0,0},
 {0x01c573,1,3,1,0},
 {0x01c574,2,2,1,1},
 {0x01c576,1,3,1,1},
 {0x01c577,1,4,1,1},
};
static const unsigned staging_boundaries68[][2]={
 {0x01c4de,3},
 {0x01c50c,3},
 {0x01c538,3},
 {0x01c563,3},
 {0x01c578,1},
};

static unsigned staging_offset68(unsigned pc){return (pc>>16)*32768+(pc&32767);}
static int staging_mode68(const CpuState*c){return c&&c->ram&&c->PB==1&&c->DB==1&&!c->D&&!c->emulation&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x1c)==4&&!!(c->P&32)==!!c->m_flag;}
static int staging_caller68(unsigned pc){return pc==0x0180e7||pc==0x01c6c7;}
static int staging_mapped68(unsigned pc){
 if(pc==0x01c4af){for(unsigned p=0x01c4af;p<0x01c59f;p++)if(!cpu_aot_rom_mapping_matches(p,staging_offset68(p),1))return 0;return 1;}
 if(staging_caller68(pc))return cpu_aot_rom_mapping_matches(pc,staging_offset68(pc),4);
 if(pc==0x0180eb||pc==0x01c6cb)return cpu_aot_rom_mapping_matches(pc,staging_offset68(pc),1);
 for(unsigned j=0;j<sizeof staging_ops68/sizeof*staging_ops68;j++)if(staging_ops68[j][0]==pc)return cpu_aot_rom_mapping_matches(pc,staging_offset68(pc),staging_ops68[j][1]);
 for(unsigned j=0;j<sizeof staging_boundaries68/sizeof*staging_boundaries68;j++)if(staging_boundaries68[j][0]==pc)return cpu_aot_rom_mapping_matches(pc,staging_offset68(pc),staging_boundaries68[j][1]);return 0;
}
static int staging_arm68(CpuState*c,uint64_t gen,unsigned caller){staging68.armed=0;if(!staging_mode68(c)||!c->m_flag||!staging_caller68(caller)||c->S<0x120||c->S>0x1ffc||!staging_mapped68(caller)||(c->ram[c->S+1]|c->ram[c->S+2]<<8)!=((caller+3)&65535)||c->ram[c->S+3]!=(caller>>16))return 0;staging68.cpu=c;staging68.ram=c->ram;staging68.gen=gen;staging68.s=c->S;staging68.db=c->DB;staging68.next=0;memcpy(staging68.frame,c->ram+c->S+1,3);staging68.armed=1;staging_stats68.entries++;return 1;}
static int staging_take68(CpuState*c,uint64_t gen){unsigned caller=staging_call68.pc;int ok=staging_call68.armed&&staging_call68.cpu==c&&staging_call68.ram==c->ram&&staging_call68.gen==gen&&staging_call68.s==c->S+3&&staging_call68.db==c->DB;staging_call68.armed=0;return ok&&staging_arm68(c,gen,caller);}
/* The generated parent remains authoritative. Fresh paired native entry may
 * witness either real JSL owner even when its caller itself was native. */
static int staging_observe68(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,unsigned s,unsigned h,unsigned host){staging68.armed=staging_call68.armed=0;if(c!=primary||gen!=frame||!staging_mode68(c)||c->S!=s||h!=3||c->host_return_valid!=3||(host!=0x0180eb&&host!=0x01c6cb)||!LagoonStaging68_Source(r,n)||!staging_mapped68(0x01c4af))return 0;staging_call68.armed=0;return staging_arm68(c,gen,host-4);}
static void staging_nz68(CpuState*c,unsigned v,unsigned bits){unsigned mask=bits==8?255:65535;c->_flag_Z=!(v&mask);c->_flag_N=!!(v&(1u<<(bits-1)));cpu_mirrors_to_p(c);}
static void staging_a68(CpuState*c,unsigned v){c->A=c->m_flag?((c->A&0xff00)|(v&255)):(v&65535);staging_nz68(c,v,c->m_flag?8:16);}
static unsigned staging_rd68(CpuState*c,unsigned p,unsigned n){uint8_t b;uint16_t v;p&=0xffffff;if(n==1){if(!interp_bridge_source_read8(c,p,&b))abort();staging_extra68+=cpu_region_speed(p)-6;return b;}unsigned high=(p+1)&0xffffff;if(!interp_bridge_source_read16(c,p,high,&v))abort();staging_extra68+=cpu_region_speed(p)-6+cpu_region_speed(high)-6;return v;}
static void staging_wr68(CpuState*c,unsigned p,unsigned v,unsigned n){p&=0xffffff;if(n==1){if(!interp_bridge_source_write8(c,p,v))abort();staging_extra68+=cpu_region_speed(p)-6;}else{unsigned high=(p+1)&0xffffff;if(!interp_bridge_source_write16(c,p,high,v))abort();staging_extra68+=cpu_region_speed(p)-6+cpu_region_speed(high)-6;}}
int LagoonStaging68_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,uint32_t pc,uint32_t*landing){
 if(pc==0x0180eb||pc==0x01c6cb){staging68.armed=staging_call68.armed=0;return 0;}
 if(!landing||c!=primary||gen!=frame||!staging_mode68(c)||!LagoonStaging68_Source(r,n)){staging68.armed=staging_call68.armed=0;staging_stats68.refused++;return 0;}
 if(staging_caller68(pc)){staging68.armed=staging_call68.armed=0;if(c->S>=0x123&&c->S<=0x1fff&&staging_mapped68(pc)){staging_call68.cpu=c;staging_call68.ram=c->ram;staging_call68.gen=gen;staging_call68.s=c->S;staging_call68.db=c->DB;staging_call68.pc=pc;staging_call68.armed=1;}return 0;}
 if(pc==0x01c4af){if(staging_call68.armed)staging_take68(c,gen);return 0;}
 StagingOwner68*w=&staging68;
 if(!w->armed||w->cpu!=c||w->ram!=c->ram||w->gen!=gen||w->db!=c->DB||memcmp(w->frame,c->ram+w->s+1,3)||(w->next&&w->next!=pc)){w->armed=0;staging_stats68.refused++;return 0;}
 for(unsigned j=0;j<sizeof staging_boundaries68/sizeof*staging_boundaries68;j++)if(staging_boundaries68[j][0]==pc){if(c->S!=w->s){w->armed=0;return 0;}if(pc==0x01c578)w->armed=0;else w->next=pc+3;return 0;}
 const unsigned*op=NULL;for(unsigned j=0;j<sizeof staging_ops68/sizeof*staging_ops68;j++)if(staging_ops68[j][0]==pc){op=staging_ops68[j];break;}if(!op||c->m_flag!=op[3]||c->S+op[4]!=w->s){w->armed=0;staging_stats68.refused++;return 0;}
 if(interp_bridge_source_opcode_deadline_reached(c)){staging_stats68.deadlines++;return 0;}
 unsigned at=staging_offset68(pc),bytes=op[1],cycles=op[2],im=bytes>1?r[at+1]:0,operand=im|(bytes>2?r[at+2]<<8:0),next=pc+bytes,width=c->m_flag?1:2;uint64_t prior=c->master_cycles;staging_extra68=0;c->coprocessor_master_cycles=prior;c->open_bus=r[at+bytes-1];
 if(!cycles){cycles=c->_flag_Z?2:3;if(!c->_flag_Z)next=pc+bytes+(int8_t)im;}
 CpuWramReadContext context={.instruction_pc=pc,.stack=c->S,.y=c->Y,.pb=c->PB,.m=c->m_flag,.x=c->x_flag,.e=c->emulation,.interpreter=1};const CpuWramReadContext*old=cpu_swap_wram_read_context(&context);
 switch(r[at]){
 case 0xbd:staging_a68(c,staging_rd68(c,0x010000+operand+c->X,width));break;
 case 0xad:staging_a68(c,staging_rd68(c,0x010000|operand,width));break;
 case 0xa5:staging_a68(c,staging_rd68(c,im,width));break;
 case 0xa9:staging_a68(c,operand);break;
 case 0x48:if(width==2){staging_wr68(c,c->S,c->A>>8,1);c->S--;}staging_wr68(c,c->S,c->A,1);c->S--;break;
 case 0x68:c->S++;{unsigned v=staging_rd68(c,c->S,width);if(width==2)c->S++;staging_a68(c,v);}break;
 case 0x29:staging_a68(c,c->A&operand);break;
 case 0x05:staging_a68(c,c->A|staging_rd68(c,im,width));break;
 case 0x99:staging_wr68(c,0x010000+operand+c->Y,c->A,width);break;
 case 0xe8:c->X=(uint16_t)(c->X+1);staging_nz68(c,c->X,16);break;
 case 0xc8:c->Y=(uint16_t)(c->Y+1);staging_nz68(c,c->Y,16);break;
 case 0xe6:{unsigned v=(staging_rd68(c,im,width)+1)&(width==1?255:65535);if(width==2)staging_wr68(c,im+1,v>>8,1);staging_wr68(c,im,v,1);staging_nz68(c,v,width==1?8:16);break;}
 case 0x1a:staging_a68(c,c->A+1);break;
 case 0xc9:{unsigned mask=width==1?255:65535;c->_flag_C=(c->A&mask)>=(operand&mask);staging_nz68(c,c->A-operand,width==1?8:16);break;}
 case 0x0a:c->_flag_C=!!(c->A&(width==1?128:32768));staging_a68(c,c->A<<1);break;
 case 0xaa:c->X=c->A;staging_nz68(c,c->X,16);break;
 case 0xa0:c->Y=operand;staging_nz68(c,c->Y,16);break;
 case 0xc0:c->_flag_C=c->Y>=operand;staging_nz68(c,c->Y-operand,16);break;
 case 0xe2:c->P|=im;cpu_p_to_mirrors(c);break;
 case 0xeb:c->A=(uint16_t)((c->A<<8)|(c->A>>8));staging_nz68(c,c->A,8);break;
 case 0xd0:break;
 default:abort();
 }
 cpu_swap_wram_read_context(old);c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2+staging_extra68;w->next=next;if(pc<0x01c4ee)staging_stats68.body++;else if(pc<0x01c51a)staging_stats68.sword++;else staging_stats68.effects++;if(!interp_bridge_commit_source_opcode(c,prior,false))abort();*landing=next;return 1;
}
static void staging_retain68(CpuState*c,uint64_t gen,unsigned pc){StagingOwner68*w=&staging68;if(w->armed&&(!c||w->cpu!=c||w->ram!=c->ram||w->gen!=gen||c->S>w->s||memcmp(w->frame,c->ram+w->s+1,3)||pc<0x01c4af||pc>=0x01c59f||(w->next&&pc>=0x01c4ce&&pc<0x01c579&&pc!=w->next)))w->armed=0;if(staging_call68.armed){int pre=c&&pc==staging_call68.pc&&c->S==staging_call68.s;int post=c&&pc==0x01c4af&&c->S+3==staging_call68.s;if(!c||staging_call68.cpu!=c||staging_call68.ram!=c->ram||staging_call68.gen!=gen||(!pre&&!post))staging_call68.armed=0;}}
#ifndef LAGOON_STAGING68_TEST
#include "common_rtl.h"
#include <stdio.h>
static uint64_t staging_generation68;static int staging_enabled68=-1,staging_report68;
static void staging_exit68(void){if(staging_report68)fprintf(stderr,"[native-staging68] entries=%llu body=%llu sword=%llu effects=%llu deadlines=%llu refused=%llu\n",(unsigned long long)staging_stats68.entries,(unsigned long long)staging_stats68.body,(unsigned long long)staging_stats68.sword,(unsigned long long)staging_stats68.effects,(unsigned long long)staging_stats68.deadlines,(unsigned long long)staging_stats68.refused);}
bool LagoonStaging68_Entry(CpuState*c,uint16_t s,uint8_t h,uint32_t host,RecompReturn*out){(void)out;if(staging_enabled68)staging_observe68(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),staging_generation68,s,h,host);return false;}
static void staging_hook68(CpuState*c,uint32_t pc){if(!staging_enabled68||c!=&g_cpu||!interp_bridge_source_opcode_available(c))return;if(!staging_mapped68(pc)){staging68.armed=staging_call68.armed=0;return;}uint32_t landing;if(LagoonStaging68_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),staging_generation68,pc,&landing))interp_bridge_pre_opcode_redirect(landing);}
void LagoonStaging68_BeforeFrame(void){if(staging_enabled68<0){const char*v=getenv("LAGOON_TIMING68");staging_enabled68=!v||strcmp(v,"0");v=getenv("LAGOON_NATIVE_WAIT_REPORT");staging_report68=v&&!strcmp(v,"1");if(staging_report68)atexit(staging_exit68);}staging_generation68=RtlStateGeneration();staging_retain68(&g_cpu,staging_generation68,interp_bridge_lle_resume_pc());if(staging_enabled68){const unsigned sites[]={0x0180e7,0x0180eb,0x01c6c7,0x01c6cb,0x01c4af};for(unsigned j=0;j<sizeof sites/sizeof*sites;j++)if(!interp_bridge_add_pre_opcode_hook(sites[j],staging_hook68))abort();for(unsigned j=0;j<sizeof staging_ops68/sizeof*staging_ops68;j++)if(!interp_bridge_add_pre_opcode_hook(staging_ops68[j][0],staging_hook68))abort();for(unsigned j=0;j<sizeof staging_boundaries68/sizeof*staging_boundaries68;j++)if(!interp_bridge_add_pre_opcode_hook(staging_boundaries68[j][0],staging_hook68))abort();}}
#endif
