/* Original sprite parent/row iteration, both encodings. All real children,
 * including the existing qualified CellRun/Apply child, retain their owners. */
#include "lagoon_native_sprites67.h"
#include "snes/interp_bridge.h"
#include <string.h>
#include <stdlib.h>
static const unsigned roots_s67[]={0x8b71,0x8bd0,0x8c96};
typedef struct {const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,db,armed,fresh;uint8_t frame[3];} SpriteOwner67;
static SpriteOwner67 sprite_owners67[3];
static struct {uint64_t entries,parent,rows,alternate,deadlines,refused;} sprite_stats67;
typedef struct {const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,db,pc,armed;} SpriteCall67;
static SpriteCall67 sprite_calls67[3];
static unsigned extra_s67;
int LagoonSprites67_Source(const uint8_t*r,size_t n){if(!r||n!=0x100000)return 0;uint64_t h=UINT64_C(14695981039346656037);for(unsigned j=0xb4b;j<0xd0b;j++)h=(h^r[j])*UINT64_C(1099511628211);return h==UINT64_C(0xd2f1f4e972d7c129);}
static int base_mode_s67(const CpuState*c){return c&&c->ram&&c->DB<=2&&!c->D&&!c->emulation&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x1c)==4&&!!(c->P&32)==!!c->m_flag;}
static int mode_s67(const CpuState*c){return base_mode_s67(c)&&!c->PB;}
static int call_s67(const uint8_t*r,size_t n,unsigned pc){
 if(!r||n!=0x100000||(pc>>16)>=32||(pc&65535)<0x8000||(pc&65535)>0xfffc)return -1;unsigned at=(pc>>16)*32768+(pc&32767);
 if((pc==0x8bb0||pc==0x8bca)&&r[at]==0x20&&r[at+1]==0xd0&&r[at+2]==0x8b)return 1;
 if(r[at]!=0x22||r[at+3])return -1;unsigned target=r[at+1]|r[at+2]<<8;return target==0x8b71?0:target==0x8c96?2:-1;
}
static int frame_s67(const uint8_t*r,CpuState*c,unsigned root){
 unsigned p=c->ram[c->S+1]|c->ram[c->S+2]<<8;
 if(root==1)return (p==0x8bb2||p==0x8bcc)&&cpu_aot_rom_mapping_matches(p-2,(p-2)&32767,3);
 unsigned bank=c->ram[c->S+3];if(bank>=32||p<0x8003)return 0;unsigned at=bank*32768+((p-3)&32767),target=roots_s67[root];return cpu_aot_rom_mapping_matches((bank<<16)|(p-3),at,4)&&r[at]==0x22&&r[at+1]==(target&255)&&r[at+2]==(target>>8)&&!r[at+3];
}
static int arm_s67(const uint8_t*r,CpuState*c,uint64_t gen,unsigned root){SpriteOwner67*w=sprite_owners67+root;w->armed=0;unsigned bytes=root==1?2:3;if(!mode_s67(c)||!c->m_flag||c->S<0x120||c->S>0x1fff-bytes||!frame_s67(r,c,root))return 0;w->cpu=c;w->ram=c->ram;w->gen=gen;w->s=c->S;w->db=c->DB;memcpy(w->frame,c->ram+c->S+1,bytes);w->armed=w->fresh=1;++sprite_stats67.entries;return 1;}
static int take_call_s67(const uint8_t*r,CpuState*c,uint64_t gen,unsigned root){SpriteCall67*p=sprite_calls67+root;unsigned size=root==1?2:3;int ok=p->armed&&p->cpu==c&&p->ram==c->ram&&p->gen==gen&&p->s==c->S+size&&p->db==c->DB&&(c->ram[c->S+1]|c->ram[c->S+2]<<8)==((p->pc+size)&65535)&&(size==2||c->ram[c->S+3]==(p->pc>>16));p->armed=0;return ok&&arm_s67(r,c,gen,root);}
static void retire_landing_s67(CpuState*c,uint64_t gen,unsigned pc){if(!c)return;for(unsigned j=0;j<3;j++){SpriteOwner67*w=sprite_owners67+j;unsigned size=j==1?2:3,return_pc=((j==1?0:w->frame[2])<<16)|(((w->frame[0]|w->frame[1]<<8)+1)&65535);if(w->armed&&w->cpu==c&&w->ram==c->ram&&w->gen==gen&&c->S==w->s+size&&pc==return_pc)w->armed=w->fresh=0;}}
static void retain_suspended_s67(CpuState*c,uint64_t gen,unsigned pc){unsigned live=(pc>=0x8b4b&&pc<0x8d36)||(pc>=0x8211&&pc<0x829b);for(unsigned j=0;j<3;j++){SpriteOwner67*w=sprite_owners67+j;if(w->armed&&(!live||w->cpu!=c||w->ram!=c->ram||w->gen!=gen||c->S>w->s||memcmp(w->frame,c->ram+w->s+1,j==1?2:3)))w->armed=w->fresh=0;SpriteCall67*p=sprite_calls67+j;if(p->armed&&(pc!=roots_s67[j]||p->cpu!=c||p->gen!=gen||p->s!=c->S+(j==1?2:3)))p->armed=0;}}
/* pc, bytes, cycles, M, exact saved-register depth; original calls/returns absent. */
static const unsigned sprite_ops67[][5]={
 {0x8b71,2,2,1,0},{0x8b73,2,0,1,0},{0x8b76,1,4,1,0},{0x8b77,1,4,1,2},{0x8b78,1,3,1,4},
 {0x8b7f,1,4,1,5},{0x8b80,2,3,1,4},{0x8b82,1,2,0,4},{0x8b83,1,2,0,4},{0x8b84,2,7,0,4},{0x8b86,2,4,0,4},{0x8b88,3,3,0,4},{0x8b8b,2,3,0,4},{0x8b8d,2,3,1,4},{0x8b8f,2,3,1,4},{0x8b91,3,3,1,4},
 {0x8b94,2,6,1,4},{0x8b96,1,2,1,4},{0x8b97,1,3,1,4},{0x8b98,1,2,1,5},{0x8b99,1,2,1,5},{0x8b9a,1,2,1,5},{0x8b9b,1,2,1,5},{0x8b9c,3,4,1,5},{0x8b9f,1,4,1,5},{0x8ba0,2,2,1,4},{0x8ba2,3,4,1,4},
 {0x8ba5,2,6,1,4},{0x8ba7,1,2,1,4},{0x8ba8,1,3,1,4},{0x8ba9,2,2,1,5},{0x8bab,2,3,1,5},{0x8bad,1,4,1,5},{0x8bae,2,0,1,4},
 {0x8bb3,2,6,1,4},{0x8bb5,1,2,1,4},{0x8bb6,1,3,1,4},{0x8bb7,1,2,1,5},{0x8bb8,1,2,1,5},{0x8bb9,1,2,1,5},{0x8bba,1,2,1,5},{0x8bbb,3,4,1,5},{0x8bbe,1,4,1,5},{0x8bbf,2,2,1,4},{0x8bc1,3,4,1,4},{0x8bc4,2,3,1,4},{0x8bc6,2,2,1,4},{0x8bc8,2,3,1,4},{0x8bcd,1,5,1,4},{0x8bce,1,5,1,2},
 {0x8bd0,2,3,1,0},{0x8bd2,1,2,1,0},{0x8bd3,1,2,1,0},{0x8bd4,1,2,1,0},{0x8bd5,1,2,1,0},{0x8bd6,3,4,1,0},{0x8bd9,3,4,1,0},{0x8bdc,3,5,1,0},{0x8bdf,2,4,1,0},{0x8be1,3,5,1,0},{0x8be4,2,4,1,0},{0x8be6,3,4,1,0},{0x8be9,3,4,1,0},{0x8bec,2,4,1,0},{0x8bee,1,4,1,0},{0x8bef,2,4,1,2},{0x8bf1,1,4,1,2},
 {0x8bf5,1,5,1,4},{0x8bf6,2,4,1,2},{0x8bf8,1,5,1,2},{0x8bf9,2,4,1,0},{0x8bfb,2,3,1,0},{0x8bfd,3,5,0,0},{0x8c00,1,2,0,0},{0x8c01,2,4,0,0},{0x8c03,2,4,0,0},{0x8c05,3,3,0,0},{0x8c08,2,3,0,0},{0x8c0a,3,6,1,0},{0x8c0d,2,0,1,0},{0x8c0f,2,3,1,0},{0x8c11,3,5,0,0},{0x8c14,1,2,0,0},{0x8c15,2,4,0,0},{0x8c17,2,4,0,0},{0x8c19,3,3,0,0},{0x8c1c,2,3,0,0},{0x8c1e,3,6,1,0},{0x8c21,2,0,1,0},
 {0x8c96,1,4,1,0},{0x8c97,1,4,1,2},{0x8c98,1,3,1,4},{0x8c9f,2,3,1,5},{0x8ca1,2,3,1,5},{0x8ca3,1,4,1,5},{0x8ca4,2,3,1,4},{0x8ca6,1,2,0,4},{0x8ca7,1,2,0,4},{0x8ca8,2,7,0,4},{0x8caa,2,4,0,4},{0x8cac,3,3,0,4},{0x8caf,2,7,0,4},{0x8cb1,3,5,0,4},{0x8cb4,3,3,0,4},{0x8cb7,2,3,0,4},{0x8cb9,2,3,1,4},{0x8cbb,1,2,1,4},{0x8cbc,1,2,1,4},{0x8cbd,3,5,1,4},{0x8cc0,2,4,1,4},{0x8cc2,3,5,1,4},{0x8cc5,2,4,1,4},{0x8cc7,3,4,1,4},{0x8cca,3,4,1,4},
 {0x8ccd,2,6,1,4},{0x8ccf,2,3,1,4},{0x8cd1,1,2,1,4},{0x8cd2,2,6,1,4},{0x8cd4,3,4,1,4},{0x8cd7,3,4,1,4},{0x8cda,2,3,1,4},{0x8cdc,1,2,1,4},{0x8ce0,2,3,1,4},{0x8ce2,2,4,0,4},{0x8ce4,1,2,0,4},{0x8ce5,3,3,0,4},{0x8ce8,2,4,0,4},{0x8cea,3,3,0,4},{0x8ced,2,3,0,4},{0x8cef,3,6,1,4},{0x8cf2,2,0,1,4},{0x8cf4,2,3,1,4},{0x8cf6,2,4,0,4},{0x8cf8,1,2,0,4},{0x8cf9,3,3,0,4},{0x8cfc,2,4,0,4},{0x8cfe,3,3,0,4},{0x8d01,2,3,0,4},{0x8d03,3,6,1,4},{0x8d06,2,0,1,4},{0x8d08,1,5,1,4},{0x8d09,1,5,1,2}};
static void nz_s67(CpuState*c,unsigned v,unsigned bits){unsigned mask=bits==8?255:65535;c->_flag_Z=!(v&mask);c->_flag_N=!!(v&(1u<<(bits-1)));cpu_mirrors_to_p(c);}
static void a_s67(CpuState*c,unsigned v){c->A=c->m_flag?((c->A&0xff00)|(v&255)):(v&65535);nz_s67(c,v,c->m_flag?8:16);}
static unsigned rd_s67(CpuState*c,unsigned p,unsigned bytes){uint8_t b;uint16_t w;p&=0xffffff;if(bytes==1){if(!interp_bridge_source_read8(c,p,&b))abort();extra_s67+=cpu_region_speed(p)-6;return b;}unsigned h=(p+1)&0xffffff;if(!interp_bridge_source_read16(c,p,h,&w))abort();extra_s67+=cpu_region_speed(p)-6+cpu_region_speed(h)-6;return w;}
static void wr_s67(CpuState*c,unsigned p,unsigned v,unsigned bytes){p&=0xffffff;if(bytes==1){if(!interp_bridge_source_write8(c,p,v))abort();extra_s67+=cpu_region_speed(p)-6;}else{unsigned h=(p+1)&0xffffff;if(!interp_bridge_source_write16(c,p,h,v))abort();extra_s67+=cpu_region_speed(p)-6+cpu_region_speed(h)-6;}}
static unsigned rm_s67(CpuState*c,unsigned p){return rd_s67(c,p,c->m_flag?1:2);}
static void wm_s67(CpuState*c,unsigned p,unsigned v){wr_s67(c,p,v,c->m_flag?1:2);}
static void push_s67(CpuState*c,unsigned v,unsigned bytes){if(bytes==2){wr_s67(c,c->S,v>>8,1);c->S--;}wr_s67(c,c->S,v,1);c->S--;}
static unsigned pull_s67(CpuState*c,unsigned bytes){unsigned v=rd_s67(c,c->S+1,bytes);c->S+=bytes;return v;}
static void adc_s67(CpuState*c,unsigned y){unsigned mask=c->m_flag?255:65535,sign=c->m_flag?128:32768,x=c->A&mask,sum=x+(y&mask)+c->_flag_C;c->_flag_C=sum>mask;c->_flag_V=!!((~(x^y)&(x^sum))&sign);a_s67(c,sum);}
int LagoonSprites67_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,uint32_t pc,uint32_t*landing){
 retire_landing_s67(c,gen,pc);
 int called=call_s67(r,n,pc);if(called>=0){SpriteOwner67*w=sprite_owners67+called;SpriteCall67*p=sprite_calls67+called;w->armed=w->fresh=p->armed=0;unsigned bytes=called==1?3:4;if(c==primary&&gen==frame&&base_mode_s67(c)&&c->PB==(pc>>16)&&c->m_flag&&c->S>=0x124&&c->S<=0x1fff&&LagoonSprites67_Source(r,n)&&cpu_aot_rom_mapping_matches(pc,(pc>>16)*32768+(pc&32767),bytes)){p->cpu=c;p->ram=c->ram;p->gen=gen;p->s=c->S;p->db=c->DB;p->pc=pc;p->armed=1;}return 0;}
 if(!landing||c!=primary||gen!=frame||!mode_s67(c)||!LagoonSprites67_Source(r,n)){memset(sprite_owners67,0,sizeof sprite_owners67);sprite_stats67.refused++;return 0;}
 unsigned root=pc<0x8bd0?0:pc<0x8c24?1:2;if(pc<0x8b71||pc>=0x8d0b)return 0;
 for(unsigned j=0;j<3;j++)if(pc==roots_s67[j]&&!take_call_s67(r,c,gen,j)&&(!sprite_owners67[j].armed||!sprite_owners67[j].fresh)){sprite_owners67[j].armed=0;return 0;}
 /* CellRun's FF FF escape also branches to the original 8C23 RTS, with
  * PHX/JSR still on the stack. Only the owner's real return depth retires it. */
 if(pc==0x8b75||pc==0x8bcf||pc==0x8c23||pc==0x8d0a){SpriteOwner67*w=sprite_owners67+root;if(c->S==w->s){w->armed=w->fresh=0;}return 0;}
 SpriteOwner67*w=sprite_owners67+root;const unsigned*op=NULL;for(unsigned j=0;j<sizeof sprite_ops67/sizeof*sprite_ops67;j++)if(sprite_ops67[j][0]==pc){op=sprite_ops67[j];break;}if(!op)return 0;
 if(!w->armed||w->cpu!=c||w->ram!=c->ram||w->gen!=gen||w->db!=c->DB||w->s!=c->S+op[4]||memcmp(w->frame,c->ram+w->s+1,root==1?2:3)||c->m_flag!=op[3]){w->armed=0;sprite_stats67.refused++;return 0;}
 if(interp_bridge_source_opcode_deadline_reached(c)){sprite_stats67.deadlines++;return 0;}
 unsigned bytes=op[1],cycles=op[2],at=pc&32767,im=bytes>1?r[at+1]:0,operand=im|(bytes==3?r[at+2]<<8:0),address=((unsigned)c->DB<<16)|operand,next=pc+bytes;uint64_t prior=c->master_cycles;extra_s67=0;c->coprocessor_master_cycles=prior;c->open_bus=r[at+bytes-1];
 if(!cycles){unsigned take=pc==0x8bae?!c->_flag_N:!c->_flag_Z;cycles=take?3:2;if(take)next=pc+bytes+(int8_t)im;}
 CpuWramReadContext context={.instruction_pc=pc,.stack=c->S,.y=c->Y,.pb=c->PB,.m=c->m_flag,.x=c->x_flag,.e=c->emulation,.interpreter=1};const CpuWramReadContext*old_context=cpu_swap_wram_read_context(&context);
 switch(pc){
 case 0x8b71:c->_flag_C=(c->A&255)>=im;nz_s67(c,(c->A-im)&255,8);break;
 case 0x8b76:case 0x8bee:case 0x8bf1:case 0x8c96:push_s67(c,c->X,2);break;
 case 0x8b77:case 0x8c97:push_s67(c,c->Y,2);break;
 case 0x8b78:case 0x8b97:case 0x8ba8:case 0x8bb6:case 0x8c98:push_s67(c,c->A,1);break;
 case 0x8b7f:case 0x8b9f:case 0x8bad:case 0x8bbe:case 0x8ca3:a_s67(c,pull_s67(c,1));break;
 case 0x8b80:case 0x8bfb:case 0x8c0f:case 0x8ca4:case 0x8ce0:case 0x8cf4:c->P&=~32;cpu_p_to_mirrors(c);break;
 case 0x8b8b:case 0x8c08:case 0x8c1c:case 0x8cb7:case 0x8ced:case 0x8d01:c->P|=32;cpu_p_to_mirrors(c);break;
 case 0x8b82:case 0x8bd3:case 0x8bd4:case 0x8bd5:case 0x8ca6:{unsigned mask=c->m_flag?255:65535;c->_flag_C=!!(c->A&(c->m_flag?128:32768));a_s67(c,(c->A&mask)<<1);break;}
 case 0x8b83:case 0x8ca7:c->Y=c->A;nz_s67(c,c->Y,16);break;
 case 0x8b84:case 0x8b94:case 0x8ba5:case 0x8bb3:case 0x8ca8:case 0x8caf:case 0x8ccd:case 0x8cd2:{unsigned p=rd_s67(c,im,1);p|=rd_s67(c,im+1,1)<<8;p|=rd_s67(c,im+2,1)<<16;a_s67(c,rm_s67(c,(p+c->Y)&0xffffff));break;}
 case 0x8b86:case 0x8b8f:case 0x8bab:case 0x8bc8:case 0x8c03:case 0x8c17:case 0x8ca1:case 0x8caa:case 0x8ccf:case 0x8cda:case 0x8ce8:case 0x8cfc:wm_s67(c,im,c->A);break;
 case 0x8b88:case 0x8ba0:case 0x8ba9:case 0x8bbf:case 0x8c05:case 0x8c19:case 0x8cb4:case 0x8cea:case 0x8cfe:a_s67(c,c->A&operand);break;
 case 0x8b8d:case 0x8bc4:case 0x8bd0:case 0x8c9f:case 0x8ce2:case 0x8cf6:a_s67(c,rm_s67(c,im));break;
 case 0x8b91:case 0x8cac:c->Y=operand;nz_s67(c,c->Y,16);break;
 case 0x8b96:case 0x8ba7:case 0x8bb5:case 0x8cbb:case 0x8cbc:case 0x8cd1:case 0x8cdc:c->Y=(uint16_t)(c->Y+1);nz_s67(c,c->Y,16);break;
 case 0x8b98:case 0x8b99:case 0x8b9a:case 0x8b9b:case 0x8bb7:case 0x8bb8:case 0x8bb9:case 0x8bba:c->_flag_C=c->A&1;a_s67(c,(c->A&255)>>1);break;
 case 0x8b9c:case 0x8ba2:case 0x8bbb:case 0x8bc1:case 0x8bd6:case 0x8be9:case 0x8cb1:case 0x8cca:wm_s67(c,address,c->A);break;
 case 0x8bc6:a_s67(c,c->A^im);break;
 case 0x8bcd:case 0x8d08:c->Y=pull_s67(c,2);nz_s67(c,c->Y,16);break;
 case 0x8bce:case 0x8bf5:case 0x8bf8:case 0x8d09:c->X=pull_s67(c,2);nz_s67(c,c->X,16);break;
 case 0x8bd2:a_s67(c,c->A+1);break;
 case 0x8bd9:wr_s67(c,address,0,1);break;
 case 0x8bdc:case 0x8be1:case 0x8cbd:case 0x8cc2:c->X=rd_s67(c,address,2);nz_s67(c,c->X,16);break;
 case 0x8bdf:case 0x8be4:case 0x8bf6:case 0x8bf9:case 0x8cc0:case 0x8cc5:wr_s67(c,im,c->X,2);break;
 case 0x8be6:case 0x8bfd:case 0x8c11:case 0x8cc7:a_s67(c,rm_s67(c,address));break;
 case 0x8bec:case 0x8bef:c->X=rd_s67(c,im,2);nz_s67(c,c->X,16);break;
 case 0x8c00:case 0x8c14:case 0x8ce4:case 0x8cf8:c->_flag_C=0;cpu_mirrors_to_p(c);break;
 case 0x8c01:case 0x8c15:adc_s67(c,rm_s67(c,im));break;
 case 0x8c0a:case 0x8c1e:case 0x8cef:case 0x8d03:{unsigned v=(rd_s67(c,address,1)-1)&255;wr_s67(c,address,v,1);nz_s67(c,v,8);break;}
 case 0x8cb9:wr_s67(c,im,0,1);break;
 case 0x8cd4:a_s67(c,c->A&rm_s67(c,address));break;
 case 0x8cd7:a_s67(c,c->A|rm_s67(c,address));break;
 case 0x8ce5:case 0x8cf9:adc_s67(c,operand);break;
 default:break;
 }
 w->fresh=0;cpu_swap_wram_read_context(old_context);c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2+extra_s67;if(!interp_bridge_commit_source_opcode(c,prior,false))abort();if(root==0)++sprite_stats67.parent;else if(root==1)++sprite_stats67.rows;else ++sprite_stats67.alternate;*landing=next;return 1;
}
static int mapped_s67(unsigned pc){if(pc==0x8b75||pc==0x8bcf||pc==0x8c23||pc==0x8d0a)return cpu_aot_rom_mapping_matches(pc,pc&32767,1);if(pc==0x8b71||pc==0x8bd0||pc==0x8c96){for(unsigned p=0x8b4b;p<0x8d0b;p++)if(!cpu_aot_rom_mapping_matches(p,p&32767,1))return 0;return 1;}for(unsigned j=0;j<sizeof sprite_ops67/sizeof*sprite_ops67;j++)if(sprite_ops67[j][0]==pc)return cpu_aot_rom_mapping_matches(pc,pc&32767,sprite_ops67[j][1]);return 0;}
#ifndef LAGOON_SPRITES67_TEST
#include "common_rtl.h"
#include <stdio.h>
static int enabled_s67=-1,report_s67;static uint64_t gen_s67;
static void stats_exit_s67(void){if(report_s67)fprintf(stderr,"[native-sprites67] entries=%llu parent=%llu rows=%llu alternate=%llu deadlines=%llu refused=%llu\n",(unsigned long long)sprite_stats67.entries,(unsigned long long)sprite_stats67.parent,(unsigned long long)sprite_stats67.rows,(unsigned long long)sprite_stats67.alternate,(unsigned long long)sprite_stats67.deadlines,(unsigned long long)sprite_stats67.refused);}
static void hook_s67(CpuState*c,uint32_t pc){if(!enabled_s67||c!=&g_cpu||!interp_bridge_source_opcode_available(c))return;retire_landing_s67(c,RtlStateGeneration(),pc);if(call_s67(g_rom,0x100000,pc)<0&&!mapped_s67(pc))return;uint32_t landing;if(LagoonSprites67_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),gen_s67,pc,&landing))interp_bridge_pre_opcode_redirect(landing);}
static bool entry_s67(CpuState*c,uint16_t s,uint8_t h,uint32_t host,unsigned root){unsigned bytes=root==1?2:3;if(enabled_s67&&c==&g_cpu&&c->ram&&c->S==s&&s>=0x120&&s<=0x1fff-bytes&&h==bytes&&c->host_return_valid==bytes&&mapped_s67(roots_s67[root])&&RtlStateGeneration()==gen_s67&&LagoonSprites67_Source(g_rom,0x100000)&&((((root==1?c->PB:c->ram[s+3])<<16)|((c->ram[s+1]|c->ram[s+2]<<8)+1)&65535)==host)){if(arm_s67(g_rom,c,gen_s67,root))sprite_calls67[root].armed=0;}return false;}
#define SPRITE_OBS(name,root) bool name(CpuState*c,uint16_t s,uint8_t h,uint32_t p,RecompReturn*out){(void)out;return entry_s67(c,s,h,p,root);}
SPRITE_OBS(LagoonSprites67_ParentEntry,0) SPRITE_OBS(LagoonSprites67_RowsEntry,1) SPRITE_OBS(LagoonSprites67_AlternateEntry,2)
#undef SPRITE_OBS
void LagoonSprites67_BeforeFrame(void){if(enabled_s67<0){const char*v=getenv("LAGOON_TIMING67");enabled_s67=!v||strcmp(v,"0");v=getenv("LAGOON_NATIVE_WAIT_REPORT");report_s67=v&&!strcmp(v,"1");if(report_s67)atexit(stats_exit_s67);}gen_s67=RtlStateGeneration();retain_suspended_s67(&g_cpu,gen_s67,interp_bridge_lle_resume_pc());if(enabled_s67){static unsigned callers[64],count;static int discovered;if(!discovered&&g_rom&&LagoonSprites67_Source(g_rom,0x100000)){for(unsigned at=0;at+4<=0x100000;at++){unsigned pc=(at/32768)*65536+32768+(at&32767);if(call_s67(g_rom,0x100000,pc)>=0){if(count==64)abort();callers[count++]=pc;}}discovered=1;}for(unsigned j=0;j<count;j++){unsigned pc=callers[j],bytes=pc==0x8bb0||pc==0x8bca?3:4;if(!interp_bridge_add_pre_opcode_hook(pc,hook_s67)||!interp_bridge_add_pre_opcode_hook(pc+bytes,hook_s67))abort();}static const unsigned returns[]={0x8b75,0x8bcf,0x8c23,0x8d0a};for(unsigned j=0;j<4;j++)if(!interp_bridge_add_pre_opcode_hook(returns[j],hook_s67))abort();for(unsigned j=0;j<sizeof sprite_ops67/sizeof*sprite_ops67;j++)if(!interp_bridge_add_pre_opcode_hook(sprite_ops67[j][0],hook_s67))abort();}}
#endif
