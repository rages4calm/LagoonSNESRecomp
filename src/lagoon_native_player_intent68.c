/* Original selectors11/13 intent and velocity for valid actor slots. Existing ordinary-enemy
 * ActorCommands optimization retains priority and remains unchanged. */
#include "lagoon_native_player_intent68.h"
#include "snes/interp_bridge.h"
#include <string.h>
#include <stdlib.h>
static uint64_t intent_hash68(const uint8_t*r,unsigned at,unsigned n){uint64_t h=UINT64_C(14695981039346656037);for(unsigned j=0;j<n;j++)h=(h^r[at+j])*UINT64_C(1099511628211);return h;}
int LagoonPlayerIntent68_Source(const uint8_t*r,size_t n){return r&&n==0x100000&&intent_hash68(r,0x100a0,0x89)==UINT64_C(0x789757cb6332dfac)&&intent_hash68(r,0x10129,0x103)==UINT64_C(0xf9986ec0b2b32cab)&&intent_hash68(r,0xb3,0x36)==UINT64_C(0x2ba6cf9e950f5246)&&r[0x10022]==0xa0&&r[0x10023]==0x80&&r[0x10026]==0xea&&r[0x10027]==0x80;}
typedef struct{const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,next,x,root;uint8_t frame[7];int armed,fresh;} IntentOwner68;
typedef struct{const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,x,root;uint8_t frame[4];int armed;} IntentCall68;
static IntentOwner68 intent_owner68;static IntentCall68 intent_pending68;
static struct{uint64_t entries,instructions,deadlines,refused;} intent_stats68;
static unsigned intent_offset68(unsigned pc){return (pc>>16)*32768+(pc&32767);}
static int intent_mode68(const CpuState*c){return c&&c->ram&&c->DB==2&&c->X<=0x800&&!(c->X&63)&&!c->D&&!c->emulation&&c->m_flag&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x3c)==0x24;}
static const unsigned intent_ops68[][3]={
 {0x0280a0,3,5},{0x0280a3,2,2},{0x0280a5,2,0},{0x0280a7,2,2},{0x0280a9,2,0},{0x0280ab,3,5},{0x0280ae,2,2},{0x0280b0,2,0},{0x0280b2,1,2},{0x0280b3,3,5},{0x0280b6,2,0},{0x0280b8,3,5},{0x0280bb,2,3},
 {0x0280bd,3,5},{0x0280c0,1,2},{0x0280c1,2,0},{0x0280c3,3,5},{0x0280c6,2,2},{0x0280c8,2,3},{0x0280ca,3,5},{0x0280cd,2,2},{0x0280cf,2,3},{0x0280d1,2,2},{0x0280d3,3,5},{0x0280d6,3,5},
{0x0280ea,3,5},{0x0280ed,2,0},{0x0280ef,3,5},{0x0280f2,1,2},{0x0280f3,1,2},{0x0280f4,1,2},{0x0280f5,1,2},{0x0280f6,3,4},{0x0280f9,3,5},{0x0280fc,1,2},{0x0280fd,1,2},{0x0280fe,1,2},{0x0280ff,3,4},{0x028102,1,2},{0x028103,3,5},{0x028106,3,5},{0x028109,3,5},{0x02810c,3,5},{0x02810f,3,5},{0x028112,3,5},{0x028115,3,5},{0x028118,3,5},{0x02811c,3,5},{0x02811f,3,5},{0x028122,3,5},{0x028125,3,5}};
static int intent_mapped68(unsigned pc){if(pc==0x0080e0){for(unsigned j=0;j<0x36;j++)if(!cpu_aot_rom_mapping_matches(0x0080b3+j,0xb3+j,1))return 0;return cpu_aot_rom_mapping_matches(0x028022,0x10022,2)&&cpu_aot_rom_mapping_matches(0x028026,0x10026,2);}if(pc==0x0280a0||pc==0x0280ea){for(unsigned j=0;j<0x18c;j++)if(!cpu_aot_rom_mapping_matches(0x0280a0+j,0x100a0+j,1))return 0;return intent_mapped68(0x0080e0);}if(pc==0x0280d9||pc==0x02811b||pc==0x028128)return cpu_aot_rom_mapping_matches(pc,intent_offset68(pc),1);for(unsigned j=0;j<sizeof intent_ops68/sizeof*intent_ops68;j++)if(intent_ops68[j][0]==pc)return cpu_aot_rom_mapping_matches(pc,intent_offset68(pc),intent_ops68[j][1]);return 0;}
static unsigned intent_selected68(const CpuState*c){if(c->ram[0x12]!=2)return 0;if(c->ram[0x10]==0xa0&&c->ram[0x11]==0x80&&c->ram[0x13]==17)return 0x0280a0;if(c->ram[0x10]==0xea&&c->ram[0x11]==0x80&&c->ram[0x13]==19)return 0x0280ea;return 0;}
static int intent_valid68(CpuState*c,uint64_t gen){IntentOwner68*w=&intent_owner68;return w->armed&&w->cpu==c&&w->ram==c->ram&&w->gen==gen&&w->s==c->S&&w->x==c->X&&!memcmp(w->frame,c->ram+w->s+1,7);}
static int intent_take68(CpuState*c,uint64_t gen,unsigned root){IntentCall68*p=&intent_pending68;IntentOwner68*w=&intent_owner68;int ok=p->armed&&p->cpu==c&&p->ram==c->ram&&p->gen==gen&&p->x==c->X&&p->root==root&&p->s==c->S+3&&c->S>=0x120&&c->S<=0x1ff8&&intent_mapped68(0x0080e0)&&!memcmp(p->frame,c->ram+p->s+1,4)&&c->ram[c->S+1]==0xe3&&c->ram[c->S+2]==0x80&&c->ram[c->S+3]==0&&c->ram[c->S+4]<=2;p->armed=0;w->armed=0;if(!ok)return 0;w->cpu=c;w->ram=c->ram;w->gen=gen;w->s=c->S;w->next=w->root=root;w->x=c->X;memcpy(w->frame,c->ram+c->S+1,7);w->armed=w->fresh=1;intent_stats68.entries++;return 1;}
/* Paired native entry is a pure observation of an existing physical JSL.
 * No guest instruction, stack byte, clock or return is manufactured. */
static int intent_observe68(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,unsigned s,unsigned h,unsigned host){
 intent_pending68.armed=0;if(c!=primary||gen!=frame||!intent_mode68(c)||c->PB!=0||c->S!=s||s<0x120||s>0x1ff8||h!=3||c->host_return_valid!=3||host!=0x0080e4||c->ram[s+1]!=0xe3||c->ram[s+2]!=0x80||c->ram[s+3]!=0||c->ram[s+4]>2||!intent_selected68(c)||!LagoonPlayerIntent68_Source(r,n)||!intent_mapped68(0x0280a0))return 0;
 intent_owner68.armed=0;IntentCall68*p=&intent_pending68;p->cpu=c;p->ram=c->ram;p->gen=gen;p->s=s+3;p->x=c->X;p->root=intent_selected68(c);memcpy(p->frame,c->ram+p->s+1,4);p->armed=1;return 1;
}
static void intent_nz68(CpuState*c,unsigned v,unsigned bits){unsigned mask=bits==8?255:65535;c->_flag_Z=!(v&mask);c->_flag_N=!!(v&(1u<<(bits-1)));cpu_mirrors_to_p(c);}
static void intent_a68(CpuState*c,unsigned v){c->A=(c->A&0xff00)|(v&255);intent_nz68(c,v,8);}
static unsigned intent_read68(CpuState*c,unsigned p){uint8_t v;if(!interp_bridge_source_read8(c,p,&v))abort();c->master_cycles+=cpu_region_speed(p)-6;return v;}
static void intent_write68(CpuState*c,unsigned p,unsigned v){if(!interp_bridge_source_write8(c,p,v))abort();c->master_cycles+=cpu_region_speed(p)-6;}
int LagoonPlayerIntent68_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,uint32_t pc,uint32_t*landing){
 if(pc==0x0080e0){IntentCall68*p=&intent_pending68;p->armed=0;if(c==primary&&gen==frame&&intent_mode68(c)&&c->PB==0&&c->S>=0x124&&c->S<=0x1ffb&&intent_selected68(c)&&LagoonPlayerIntent68_Source(r,n)&&intent_mapped68(pc)){intent_owner68.armed=0;p->cpu=c;p->ram=c->ram;p->gen=gen;p->s=c->S;p->x=c->X;p->root=intent_selected68(c);memcpy(p->frame,c->ram+c->S+1,4);p->armed=1;}return 0;}
 unsigned root=pc<0x0280ea?0x0280a0:0x0280ea;if(pc!=root)intent_pending68.armed=0;if(!landing||c!=primary||gen!=frame||!intent_mode68(c)||c->PB!=2||!LagoonPlayerIntent68_Source(r,n)){intent_owner68.armed=intent_pending68.armed=0;intent_stats68.refused++;return 0;}
 IntentOwner68*w=&intent_owner68;if(pc==root&&!(w->fresh&&w->next==pc&&intent_valid68(c,gen))&&!intent_take68(c,gen,root))return 0;
 if(!intent_valid68(c,gen)||w->root!=root||w->next!=pc){w->armed=0;intent_stats68.refused++;return 0;}if(pc==0x0280d9||pc==0x02811b||pc==0x028128){w->armed=0;return 0;}
 const unsigned*op=NULL;for(unsigned j=0;j<sizeof intent_ops68/sizeof*intent_ops68;j++)if(intent_ops68[j][0]==pc){op=intent_ops68[j];break;}if(!op){w->armed=0;return 0;}if(interp_bridge_source_opcode_deadline_reached(c)){intent_stats68.deadlines++;return 0;}
 unsigned at=intent_offset68(pc),bytes=op[1],cycles=op[2],im=bytes>1?r[at+1]:0,operand=im|(bytes==3?r[at+2]<<8:0),address=0x020000|operand,next=pc+bytes;uint64_t prior=c->master_cycles;
 if(!cycles){unsigned take=pc==0x0280c1?!c->_flag_C:(pc==0x0280b6||pc==0x0280ed)?c->_flag_N:pc==0x0280b0?c->_flag_Z:!c->_flag_Z;cycles=take?3:2;if(take)next=pc+bytes+(int8_t)im;}
 c->coprocessor_master_cycles=prior;c->open_bus=r[at+bytes-1];CpuWramReadContext context={.instruction_pc=pc,.stack=c->S,.y=c->Y,.pb=c->PB,.m=c->m_flag,.x=c->x_flag,.e=c->emulation,.interpreter=1};const CpuWramReadContext*old=cpu_swap_wram_read_context(&context);
 switch(pc){case 0x0280a0:case 0x0280ab:case 0x0280bd:case 0x0280c3:case 0x0280ca:case 0x0280ea:case 0x0280ef:case 0x0280f9:intent_a68(c,intent_read68(c,address+c->X));break;
 case 0x0280a3:case 0x0280a7:c->_flag_Z=!(c->A&im&255);cpu_mirrors_to_p(c);break;
 case 0x0280ae:case 0x0280c6:case 0x0280cd:intent_a68(c,c->A&im);break;
 case 0x0280b2:case 0x028102:c->Y=c->A;intent_nz68(c,c->Y,16);break;
 case 0x0280b3:case 0x028103:case 0x028109:case 0x02810f:case 0x028115:intent_a68(c,intent_read68(c,(address+c->Y)&0xffffff));break;
 case 0x0280b8:case 0x0280d3:case 0x0280d6:case 0x028106:case 0x02810c:case 0x028112:case 0x028118:intent_write68(c,address+c->X,c->A);break;
 case 0x0280bb:case 0x0280c8:case 0x0280cf:next=pc+bytes+(int8_t)im;break;
 case 0x0280c0:c->_flag_C=c->A&1;intent_a68(c,(c->A&255)>>1);break;
 case 0x0280d1:intent_a68(c,im);break;
case 0x0280f2:case 0x0280f3:case 0x0280f4:case 0x0280f5:case 0x0280fc:case 0x0280fd:c->_flag_C=!!(c->A&128);intent_a68(c,(c->A&255)<<1);break;
case 0x0280f6:intent_write68(c,address,c->A);break;
case 0x0280fe:c->_flag_C=0;cpu_mirrors_to_p(c);break;
case 0x0280ff:{unsigned x=c->A&255,y=intent_read68(c,address),sum=x+y+c->_flag_C;c->_flag_C=sum>255;c->_flag_V=!!((~(x^y)&(x^sum))&128);intent_a68(c,sum);break;}
case 0x02811c:case 0x02811f:case 0x028122:case 0x028125:intent_write68(c,address+c->X,0);break;
default:break;}
 w->fresh=0;cpu_swap_wram_read_context(old);c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2;w->next=next;intent_stats68.instructions++;if(!interp_bridge_commit_source_opcode(c,prior,false))abort();*landing=next;return 1;
}
static void intent_retain68(CpuState*c,uint64_t gen,unsigned pc){IntentOwner68*w=&intent_owner68;if(w->armed&&(!c||w->cpu!=c||w->ram!=c->ram||w->gen!=gen||w->x!=c->X||w->s!=c->S||pc!=w->next||memcmp(w->frame,c->ram+w->s+1,7)))w->armed=0;IntentCall68*p=&intent_pending68;if(p->armed){int pre=c&&pc==0x0080e0&&c->S==p->s,post=c&&(pc==0x0080e6||pc==p->root)&&c->S+3==p->s;if(!c||p->cpu!=c||p->ram!=c->ram||p->gen!=gen||p->x!=c->X||(!pre&&!post)||memcmp(p->frame,c->ram+p->s+1,4))p->armed=0;}}
#ifndef LAGOON_PLAYER_INTENT68_TEST
#include "common_rtl.h"
#include <stdio.h>
static uint64_t intent_generation68;static int intent_enabled68=-1,intent_report68;
static void intent_report_exit68(void){if(intent_report68)fprintf(stderr,"[native-player-intent68] entries=%llu instructions=%llu deadlines=%llu refused=%llu\n",(unsigned long long)intent_stats68.entries,(unsigned long long)intent_stats68.instructions,(unsigned long long)intent_stats68.deadlines,(unsigned long long)intent_stats68.refused);}
bool LagoonPlayerIntent68_DispatchEntry(CpuState*c,uint16_t s,uint8_t h,uint32_t host,RecompReturn*out){(void)out;if(intent_enabled68)intent_observe68(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),intent_generation68,s,h,host);return false;}
static void intent_hook68(CpuState*c,uint32_t pc){if(!intent_enabled68||c!=&g_cpu||!interp_bridge_source_opcode_available(c))return;if(!intent_mapped68(pc)){intent_owner68.armed=intent_pending68.armed=0;return;}uint32_t landing;if(LagoonPlayerIntent68_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),intent_generation68,pc,&landing))interp_bridge_pre_opcode_redirect(landing);}
void LagoonPlayerIntent68_BeforeFrame(void){if(intent_enabled68<0){const char*v=getenv("LAGOON_TIMING68");intent_enabled68=!v||strcmp(v,"0");v=getenv("LAGOON_NATIVE_WAIT_REPORT");intent_report68=v&&!strcmp(v,"1");if(intent_report68)atexit(intent_report_exit68);}intent_generation68=RtlStateGeneration();intent_retain68(&g_cpu,intent_generation68,interp_bridge_lle_resume_pc());if(intent_enabled68){for(unsigned j=0;j<sizeof intent_ops68/sizeof*intent_ops68;j++)if(!interp_bridge_add_pre_opcode_hook(intent_ops68[j][0],intent_hook68))abort();if(!interp_bridge_add_pre_opcode_hook(0x0080e0,intent_hook68)||!interp_bridge_add_pre_opcode_hook(0x0280d9,intent_hook68)||!interp_bridge_add_pre_opcode_hook(0x02811b,intent_hook68)||!interp_bridge_add_pre_opcode_hook(0x028128,intent_hook68))abort();}}
#endif
