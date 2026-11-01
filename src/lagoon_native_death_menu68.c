/* Finite original death/retry menu. Original dispatch, waits, sound, renderer,
 * persistence and every JSR/JSL/RTS/RTL keep their real stack and effects. */
#include "lagoon_native_death_menu68.h"
#include "snes/interp_bridge.h"
#include <string.h>
#include <stdlib.h>
static uint64_t hash_death68(const uint8_t*r,unsigned at,unsigned n){uint64_t h=UINT64_C(14695981039346656037);for(unsigned j=0;j<n;j++)h=(h^r[at+j])*UINT64_C(1099511628211);return h;}
int LagoonDeathMenu68_Source(const uint8_t*r,size_t n){return r&&n==0x100000&&hash_death68(r,0x1554e,0x94)==UINT64_C(0x8d13454be4483827)&&hash_death68(r,0xb3,0x36)==UINT64_C(0x2ba6cf9e950f5246)&&r[0x1001c]==0x4e&&r[0x1001d]==0xd5;}
typedef struct{const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,next;uint8_t frame[7];int armed,fresh;} DeathOwner68;
typedef struct{const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,pc;uint8_t frame[7];int armed;} DeathCall68;
static DeathOwner68 death_owner68[2];static DeathCall68 death_pending68[2];
static struct{uint64_t entries,parent,child,deadlines,refused;} death_stats68;
static unsigned death_offset68(unsigned pc){return (pc>>16)*32768+(pc&32767);}
static int death_mode68(const CpuState*c){return c&&c->ram&&c->DB==2&&!c->D&&!c->emulation&&c->m_flag&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x3c)==0x24;}
static int death_owner_valid68(const DeathOwner68*w,CpuState*c,uint64_t gen,unsigned bytes){return w->armed&&w->cpu==c&&w->ram==c->ram&&w->gen==gen&&w->s==c->S&&!memcmp(w->frame,c->ram+w->s+1,bytes);}
static int death_mapped68(unsigned pc);
static int death_take68(CpuState*c,uint64_t gen,unsigned child){DeathCall68*p=death_pending68+child;DeathOwner68*w=death_owner68+child;unsigned n=child?2:7,root=child?0x02d59c:0x02d54e;int ok=p->armed&&p->cpu==c&&p->ram==c->ram&&p->gen==gen&&c->S+(child?2:3)==p->s&&c->S>=0x120&&c->S<=0x1fff-n&&death_mapped68(p->pc)&&!memcmp(p->frame,c->ram+p->s+1,child?7:4);p->armed=0;w->armed=0;if(!ok)return 0;unsigned ret=c->ram[c->S+1]|c->ram[c->S+2]<<8;if(child){DeathOwner68*parent=death_owner68;if(!parent->armed||parent->cpu!=c||parent->ram!=c->ram||parent->gen!=gen||parent->s!=c->S+2||memcmp(parent->frame,c->ram+parent->s+1,7)||ret!=(p->pc+2&65535))return 0;}else if(ret!=0x80e3||c->ram[c->S+3]!=0||c->ram[c->S+4]>2)return 0;w->cpu=c;w->ram=c->ram;w->gen=gen;w->s=c->S;w->next=root;memcpy(w->frame,c->ram+c->S+1,n);w->armed=w->fresh=1;death_stats68.entries++;return 1;}
/* pc, source length, original base cycles. No real calls or returns. */
static const unsigned death_ops68[][3]={
 {0x02d54e,2,2},{0x02d554,2,2},{0x02d55a,2,2},{0x02d560,2,2},
 {0x02d56a,3,4},{0x02d56d,3,4},{0x02d570,2,2},{0x02d572,2,0},{0x02d574,3,4},
 {0x02d57e,3,4},{0x02d581,2,0},{0x02d583,3,4},{0x02d586,2,2},{0x02d588,2,0},{0x02d58a,2,2},{0x02d58c,2,0},
 {0x02d58e,3,4},{0x02d591,1,2},{0x02d592,2,2},{0x02d594,3,4},{0x02d59a,2,3},
 {0x02d59c,3,4},{0x02d59f,1,2},{0x02d5a0,1,2},{0x02d5a1,3,5},{0x02d5a4,2,3},{0x02d5a6,3,5},{0x02d5a9,2,3},
 {0x02d5ab,2,2},{0x02d5ad,2,3},{0x02d5af,3,3},{0x02d5b2,3,5},{0x02d5b5,2,2},{0x02d5b7,3,4},
 {0x02d5c3,3,4},{0x02d5c6,2,0},{0x02d5cc,2,2},{0x02d5ce,2,0},{0x02d5d8,3,4},{0x02d5db,2,3}};
static const unsigned death_calls68[][3]={
 {0x02d550,4,0x02d554},{0x02d556,4,0x02d55a},{0x02d55c,4,0x02d560},{0x02d562,4,0x02d566},{0x02d566,4,0x02d56a},
 {0x02d577,3,0x02d57a},{0x02d57a,4,0x02d57e},{0x02d597,3,0x02d59a},{0x02d5ba,4,0x02d5be},{0x02d5be,4,0x02d5c2},
 {0x02d5c8,4,0x02d5cc},{0x02d5d0,4,0x02d5d4},{0x02d5d4,4,0x02d5d8}};
static int death_mapped68(unsigned pc){if(pc==0x0080e0){for(unsigned j=0;j<0x36;j++)if(!cpu_aot_rom_mapping_matches(0x0080b3+j,0xb3+j,1))return 0;return cpu_aot_rom_mapping_matches(0x02801c,0x1001c,2);}if(pc==0x02d54e){for(unsigned j=0;j<0x94;j++)if(!cpu_aot_rom_mapping_matches(pc+j,death_offset68(pc)+j,1))return 0;return death_mapped68(0x0080e0);}if(pc==0x02d59c){for(unsigned j=0;j<0x27;j++)if(!cpu_aot_rom_mapping_matches(pc+j,death_offset68(pc)+j,1))return 0;return 1;}if(pc==0x02d5c2||pc==0x02d5dd)return cpu_aot_rom_mapping_matches(pc,death_offset68(pc),1);for(unsigned j=0;j<sizeof death_ops68/sizeof*death_ops68;j++)if(death_ops68[j][0]==pc)return cpu_aot_rom_mapping_matches(pc,death_offset68(pc),death_ops68[j][1]);for(unsigned j=0;j<sizeof death_calls68/sizeof*death_calls68;j++)if(death_calls68[j][0]==pc)return cpu_aot_rom_mapping_matches(pc,death_offset68(pc),death_calls68[j][1]);return 0;}
/* Pure observation of the generated dispatcher's real paired JSL entry.
 * No guest instruction, cycle, stack byte, return or callee result is created. */
static int death_observe_dispatch68(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,unsigned s,unsigned h,unsigned host){
 death_pending68[0].armed=0;
 if(c!=primary||gen!=frame||!death_mode68(c)||c->PB!=0||c->S!=s||s<0x120||s>0x1ff8||h!=3||c->host_return_valid!=3||host!=0x0080e4||c->ram[s+1]!=0xe3||c->ram[s+2]!=0x80||c->ram[s+3]!=0||c->ram[s+4]>2||c->ram[0x10]!=0x4e||c->ram[0x11]!=0xd5||c->ram[0x12]!=2||c->ram[0x13]!=14||!LagoonDeathMenu68_Source(r,n)||!death_mapped68(0x02d54e))return 0;
 memset(death_owner68,0,sizeof death_owner68);memset(death_pending68,0,sizeof death_pending68);DeathCall68*p=death_pending68;p->cpu=c;p->ram=c->ram;p->gen=gen;p->s=s+3;p->pc=0x0080e0;memcpy(p->frame,c->ram+p->s+1,4);p->armed=1;return 1;
}
static void death_nz68(CpuState*c,unsigned v,unsigned bits){unsigned mask=bits==8?255:65535;c->_flag_Z=!(v&mask);c->_flag_N=!!(v&(1u<<(bits-1)));cpu_mirrors_to_p(c);}
static void death_a68(CpuState*c,unsigned v){c->A=(c->A&0xff00)|(v&255);death_nz68(c,v,8);}
static unsigned death_read68(CpuState*c,unsigned p){uint8_t v;if(!interp_bridge_source_read8(c,p,&v))abort();c->master_cycles+=cpu_region_speed(p)-6;return v;}
static void death_write68(CpuState*c,unsigned p,unsigned v,unsigned n){if(n==1){if(!interp_bridge_source_write8(c,p,v))abort();}else if(!interp_bridge_source_write16(c,p,(p&0xff0000)|((p+1)&65535),v))abort();c->master_cycles+=cpu_region_speed(p)-6;if(n==2)c->master_cycles+=cpu_region_speed((p&0xff0000)|((p+1)&65535))-6;}
int LagoonDeathMenu68_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,uint32_t pc,uint32_t*landing){
 if(pc==0x0080e0){DeathCall68*p=death_pending68;memset(death_pending68,0,sizeof death_pending68);if(c==primary&&gen==frame&&death_mode68(c)&&c->PB==0&&c->S>=0x124&&c->S<=0x1ffb&&c->ram[0x10]==0x4e&&c->ram[0x11]==0xd5&&c->ram[0x12]==2&&LagoonDeathMenu68_Source(r,n)&&death_mapped68(pc)){memset(death_owner68,0,sizeof death_owner68);p->cpu=c;p->ram=c->ram;p->gen=gen;p->s=c->S;p->pc=pc;memcpy(p->frame,c->ram+c->S+1,4);p->armed=1;}return 0;}
 if(pc!=0x02d54e)death_pending68[0].armed=0;if(pc!=0x02d59c)death_pending68[1].armed=0;
 if(!landing||c!=primary||gen!=frame||!death_mode68(c)||c->PB!=2||!LagoonDeathMenu68_Source(r,n)){memset(death_owner68,0,sizeof death_owner68);memset(death_pending68,0,sizeof death_pending68);death_stats68.refused++;return 0;}
 unsigned child=pc>=0x02d59c&&pc<=0x02d5c2;DeathOwner68*w=death_owner68+child;
 if((pc==0x02d54e||pc==0x02d59c)&&!(w->fresh&&w->next==pc&&death_owner_valid68(w,c,gen,child?2:7))&&!death_take68(c,gen,child))return 0;
 if(!death_owner_valid68(w,c,gen,child?2:7)||w->next!=pc){w->armed=0;death_stats68.refused++;return 0;}
 if(child){DeathOwner68*p=death_owner68;if(!p->armed||p->cpu!=c||p->gen!=gen||p->ram!=c->ram||p->s!=c->S+2||memcmp(p->frame,c->ram+p->s+1,7)){w->armed=0;death_stats68.refused++;return 0;}}
 if(pc==0x02d5c2||pc==0x02d5dd){w->armed=0;return 0;}
 for(unsigned j=0;j<sizeof death_calls68/sizeof*death_calls68;j++)if(death_calls68[j][0]==pc){w->next=death_calls68[j][2];if(pc==0x02d577||pc==0x02d597){DeathCall68*p=death_pending68+1;p->cpu=c;p->ram=c->ram;p->gen=gen;p->s=c->S;p->pc=pc;memcpy(p->frame,c->ram+c->S+1,7);p->armed=1;}return 0;}
 const unsigned*op=NULL;for(unsigned j=0;j<sizeof death_ops68/sizeof*death_ops68;j++)if(death_ops68[j][0]==pc){op=death_ops68[j];break;}if(!op){w->armed=0;return 0;}if(interp_bridge_source_opcode_deadline_reached(c)){death_stats68.deadlines++;return 0;}
 unsigned at=death_offset68(pc),bytes=op[1],cycles=op[2],im=bytes>1?r[at+1]:0,operand=im|(bytes==3?r[at+2]<<8:0),address=0x020000|operand,next=pc+bytes;uint64_t prior=c->master_cycles;
 if(!cycles){unsigned take=pc==0x02d581?c->_flag_N:(pc==0x02d572||pc==0x02d58c)?c->_flag_Z:!c->_flag_Z;cycles=take?3:2;if(take)next=pc+bytes+(int8_t)im;}
 c->coprocessor_master_cycles=prior;c->open_bus=r[at+bytes-1];CpuWramReadContext context={.instruction_pc=pc,.stack=c->S,.y=c->Y,.pb=c->PB,.m=c->m_flag,.x=c->x_flag,.e=c->emulation,.interpreter=1};const CpuWramReadContext*old=cpu_swap_wram_read_context(&context);
 switch(pc){
 case 0x02d54e:case 0x02d554:case 0x02d55a:case 0x02d560:case 0x02d5ab:case 0x02d5b5:death_a68(c,im);break;
 case 0x02d56a:case 0x02d57e:case 0x02d583:case 0x02d58e:case 0x02d59c:case 0x02d5c3:case 0x02d5d8:death_a68(c,death_read68(c,address));break;
 case 0x02d56d:death_a68(c,c->A|death_read68(c,address));break;
 case 0x02d570:case 0x02d592:death_a68(c,c->A&im);break;
 case 0x02d574:death_write68(c,address,0,1);break;
 case 0x02d586:case 0x02d58a:c->_flag_Z=!(c->A&im&255);cpu_mirrors_to_p(c);break;
 case 0x02d591:death_a68(c,c->A+1);break;
 case 0x02d594:case 0x02d5b7:death_write68(c,address,c->A,1);break;
 case 0x02d59a:next=pc+bytes+(int8_t)im;break;
 case 0x02d59f:c->_flag_C=!!(c->A&128);death_a68(c,(c->A&255)<<1);break;
 case 0x02d5a0:c->Y=c->A;death_nz68(c,c->Y,16);break;
 case 0x02d5a1:case 0x02d5a6:death_a68(c,death_read68(c,(address+c->Y)&0xffffff));break;
 case 0x02d5a4:case 0x02d5a9:case 0x02d5ad:case 0x02d5db:death_write68(c,im,c->A,1);break;
 case 0x02d5af:c->X=operand;death_nz68(c,c->X,16);break;
 case 0x02d5b2:death_write68(c,address,c->X,2);break;
 case 0x02d5cc:c->_flag_C=(c->A&255)>=im;death_nz68(c,(c->A&255)-im,8);break;
 default:break;
 }
 w->fresh=0;cpu_swap_wram_read_context(old);c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2;w->next=next;if(child)death_stats68.child++;else death_stats68.parent++;if(!interp_bridge_commit_source_opcode(c,prior,false))abort();*landing=next;return 1;
}
/* A pending call may cross a field boundary only at its actual original call,
 * dispatcher jump or destination, with the original outer stack unchanged. */
static void death_retain68(CpuState*c,uint64_t gen,unsigned pc){
 for(unsigned j=0;j<2;j++){
  DeathOwner68*w=death_owner68+j;unsigned bytes=j?2:7;
  if(w->armed&&(!c||w->cpu!=c||w->ram!=c->ram||w->gen!=gen||c->S>w->s||memcmp(w->frame,c->ram+w->s+1,bytes)||(c->S==w->s&&pc!=w->next)))w->armed=0;
  DeathCall68*p=death_pending68+j;unsigned pushed=j?2:3,root=j?0x02d59c:0x02d54e;
  if(p->armed){int pre=c&&pc==p->pc&&c->S==p->s;int post=c&&(pc==root||(!j&&pc==0x0080e6))&&c->S+pushed==p->s;
   if(!c||p->cpu!=c||p->ram!=c->ram||p->gen!=gen||(!pre&&!post)||memcmp(p->frame,c->ram+p->s+1,j?7:4))p->armed=0;
  }
 }
}
#ifndef LAGOON_DEATH_MENU68_TEST
#include "common_rtl.h"
#include <stdio.h>
static uint64_t death_generation68;static int death_enabled68=-1,death_report68;
static void death_report_exit68(void){if(death_report68)fprintf(stderr,"[native-death-menu68] entries=%llu parent=%llu child=%llu deadlines=%llu refused=%llu\n",(unsigned long long)death_stats68.entries,(unsigned long long)death_stats68.parent,(unsigned long long)death_stats68.child,(unsigned long long)death_stats68.deadlines,(unsigned long long)death_stats68.refused);}
bool LagoonDeathMenu68_DispatchEntry(CpuState*c,uint16_t s,uint8_t h,uint32_t host,RecompReturn*out){(void)out;if(death_enabled68)death_observe_dispatch68(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),death_generation68,s,h,host);return false;}
static void death_hook68(CpuState*c,uint32_t pc){if(!death_enabled68||c!=&g_cpu||!interp_bridge_source_opcode_available(c))return;if(!death_mapped68(pc)){memset(death_owner68,0,sizeof death_owner68);memset(death_pending68,0,sizeof death_pending68);return;}uint32_t landing;if(LagoonDeathMenu68_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),death_generation68,pc,&landing))interp_bridge_pre_opcode_redirect(landing);}
void LagoonDeathMenu68_BeforeFrame(void){if(death_enabled68<0){const char*v=getenv("LAGOON_TIMING68");death_enabled68=!v||strcmp(v,"0");v=getenv("LAGOON_NATIVE_WAIT_REPORT");death_report68=v&&!strcmp(v,"1");if(death_report68)atexit(death_report_exit68);}death_generation68=RtlStateGeneration();death_retain68(&g_cpu,death_generation68,interp_bridge_lle_resume_pc());if(death_enabled68){for(unsigned j=0;j<sizeof death_ops68/sizeof*death_ops68;j++)if(!interp_bridge_add_pre_opcode_hook(death_ops68[j][0],death_hook68))abort();for(unsigned j=0;j<sizeof death_calls68/sizeof*death_calls68;j++)if(!interp_bridge_add_pre_opcode_hook(death_calls68[j][0],death_hook68))abort();if(!interp_bridge_add_pre_opcode_hook(0x0080e0,death_hook68)||!interp_bridge_add_pre_opcode_hook(0x02d5c2,death_hook68)||!interp_bridge_add_pre_opcode_hook(0x02d5dd,death_hook68))abort();}}
#endif
