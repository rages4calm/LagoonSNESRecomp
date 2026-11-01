/* Source-timed fallback for row iteration, queue pointer setup and HUD fill.
 * Existing optimized MenuClose hooks run first. Every call/return and child
 * remains original; no optimized algorithm or queue upload is duplicated. */
#include "lagoon_native_tiles68.h"
#include "snes/interp_bridge.h"
#include <string.h>
#include <stdlib.h>
static unsigned tile_offset68(unsigned pc){return (pc>>16)*32768+(pc&32767);}
static const struct{unsigned pc,n;uint64_t hash;}tile_spans68[]={
 {0x0189f5,0x9d,UINT64_C(0xfebc134bf91bb6f9)},
 {0x018acd,0x67,UINT64_C(0x00da55454500d393)},
 {0x008aa4,0x31,UINT64_C(0x4a2fc0d8151ac00e)},
 {0x018311,0x74,UINT64_C(0x9e17ed9893b82194)}};
int LagoonTiles68_Source(const uint8_t*r,size_t n){if(!r||n!=0x100000)return 0;for(unsigned j=0;j<sizeof tile_spans68/sizeof*tile_spans68;j++){uint64_t h=UINT64_C(14695981039346656037);for(unsigned k=0;k<tile_spans68[j].n;k++)h=(h^r[tile_offset68(tile_spans68[j].pc)+k])*UINT64_C(1099511628211);if(h!=tile_spans68[j].hash)return 0;}return !memcmp(r+0x873e,"\x20\xf5\x89",3)&&!memcmp(r+0x87a8,"\x20\xf5\x89",3)&&!memcmp(r+0x8741,"\x20\xcd\x8a",3);}
typedef struct{const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s;uint8_t frame[2];int armed,fresh;} TileOwner68;
typedef struct{const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,pc;uint8_t outer[4];int armed;} TileCall68;
static TileOwner68 tile_owner68[3];static TileCall68 tile_pending68[3];
static struct{uint64_t entries,rows,queue,hud,deadlines,refused;}tile_stats68;
static unsigned tile_extra68;
static int tile_mode68(const CpuState*c){return c&&c->ram&&c->PB==1&&c->DB==1&&!c->D&&!c->emulation&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x1c)==4&&!!(c->P&32)==!!c->m_flag;}
static unsigned tile_family68(unsigned pc){return pc>=0x018acd?2:pc>=0x018a5e?1:0;}
static const unsigned tile_roots68[]={0x0189f5,0x018a5e,0x018acd};
/* pc,length,basecycles,M,depth below real owner's JSR frame. Native prefixes
 * and old optimized redirects may legitimately execute before a callback;
 * ownership therefore uses the real frame and exact per-PC register depth. */
static const unsigned tile_ops68[][5]={
 {0x0189f5,3,5,1,0},{0x0189f8,2,4,1,0},{0x0189fa,3,3,1,0},{0x0189fd,2,3,1,0},{0x0189ff,3,5,0,0},{0x018a02,1,2,0,0},{0x018a03,3,3,0,0},{0x018a06,2,4,0,0},{0x018a08,3,3,0,0},{0x018a0b,2,3,0,0},{0x018a0d,3,3,1,0},
 {0x018a10,1,4,1,0},{0x018a11,2,3,1,2},{0x018a13,2,4,0,2},{0x018a15,1,2,0,2},{0x018a16,1,2,0,2},{0x018a17,1,2,0,2},{0x018a18,3,5,0,2},{0x018a1b,2,4,0,2},{0x018a1d,1,2,0,2},{0x018a1e,1,2,0,2},{0x018a1f,1,2,0,2},{0x018a20,3,5,0,2},{0x018a23,3,3,0,2},{0x018a26,2,3,0,2},
 {0x018a32,1,5,1,2},{0x018a33,2,3,1,0},{0x018a35,2,4,0,0},{0x018a37,1,2,0,0},{0x018a38,3,3,0,0},{0x018a3b,2,4,0,0},{0x018a3d,3,3,0,0},{0x018a40,2,3,0,0},{0x018a42,1,2,1,0},{0x018a43,3,3,1,0},{0x018a46,2,0,1,0},{0x018a48,2,3,1,0},{0x018a4a,2,4,0,0},{0x018a4c,1,2,0,0},{0x018a4d,3,3,0,0},{0x018a50,2,4,0,0},{0x018a52,3,3,0,0},{0x018a55,2,3,0,0},{0x018a57,1,2,1,0},{0x018a58,3,3,1,0},{0x018a5b,2,0,1,0},
 {0x018a5e,3,5,1,0},{0x018a61,3,5,1,0},{0x018a64,3,5,1,0},{0x018a67,3,5,1,0},{0x018a6a,3,3,1,0},{0x018a6d,3,5,1,0},{0x018a70,3,5,1,0},{0x018a73,3,3,1,0},{0x018a76,2,4,1,0},{0x018a78,2,3,1,0},{0x018a7a,3,4,1,0},{0x018a81,2,3,1,0},{0x018a83,2,0,1,0},{0x018a85,3,3,1,0},{0x018a88,2,4,1,0},{0x018a8a,3,6,1,0},
 {0x018ad1,2,3,1,0},{0x018ad3,3,3,0,0},{0x018ad6,3,3,0,0},{0x018ad9,4,6,0,0},{0x018add,1,2,0,0},{0x018ade,1,2,0,0},{0x018adf,3,3,0,0},{0x018ae2,2,0,0,0},{0x018ae4,3,5,0,0},{0x018ae7,3,3,0,0},{0x018aea,3,5,0,0},{0x018aed,3,3,0,0},{0x018af0,2,3,0,0},
 {0x018af6,3,3,1,0},{0x018af9,2,4,1,0},{0x018afb,2,2,1,0},{0x018afd,2,3,1,0},{0x018aff,3,3,1,0},{0x018b02,3,5,1,0},{0x018b05,2,2,1,0},{0x018b07,3,4,1,0},{0x018b0e,3,3,1,0},{0x018b11,2,4,1,0},{0x018b13,2,2,1,0},{0x018b15,2,3,1,0},{0x018b17,3,3,1,0},{0x018b1a,3,5,1,0},{0x018b21,2,2,1,0},{0x018b27,2,2,1,0},{0x018b2d,2,2,1,0}};
static const unsigned tile_calls68[][3]={{0x01873e,3,0},{0x0187a8,3,0},{0x018741,3,2},{0x018a2f,3,1}};
static const unsigned tile_boundaries68[][2]={{0x018a28,4},{0x018a2c,3},{0x018a2f,3},{0x018a5d,1},{0x018a7d,4},{0x018a8d,4},{0x018a91,1},{0x018acd,4},{0x018af2,4},{0x018b0a,4},{0x018b1d,4},{0x018b23,4},{0x018b29,4},{0x018b2f,4},{0x018b33,1}};
static int tile_mapped68(unsigned pc){for(unsigned j=0;j<3;j++)if(pc==tile_roots68[j]){for(unsigned at=0;at<sizeof tile_spans68/sizeof*tile_spans68;at++)for(unsigned k=0;k<tile_spans68[at].n;k++)if(!cpu_aot_rom_mapping_matches(tile_spans68[at].pc+k,tile_offset68(tile_spans68[at].pc)+k,1))return 0;const unsigned callers[]={0x01873e,0x0187a8,0x018741,0x018a2f};for(unsigned k=0;k<4;k++)if(!cpu_aot_rom_mapping_matches(callers[k],tile_offset68(callers[k]),3))return 0;return 1;}for(unsigned j=0;j<sizeof tile_ops68/sizeof*tile_ops68;j++)if(tile_ops68[j][0]==pc)return cpu_aot_rom_mapping_matches(pc,tile_offset68(pc),tile_ops68[j][1]);for(unsigned j=0;j<sizeof tile_calls68/sizeof*tile_calls68;j++)if(tile_calls68[j][0]==pc)return cpu_aot_rom_mapping_matches(pc,tile_offset68(pc),tile_calls68[j][1]);for(unsigned j=0;j<sizeof tile_boundaries68/sizeof*tile_boundaries68;j++)if(tile_boundaries68[j][0]==pc)return cpu_aot_rom_mapping_matches(pc,tile_offset68(pc),tile_boundaries68[j][1]);return 0;}
static int tile_frame68(CpuState*c,unsigned root){unsigned ret=c->ram[c->S+1]|c->ram[c->S+2]<<8,call=0x010000|((ret-2)&65535);if(root==0&&ret!=0x8740&&ret!=0x87aa)return 0;if(root==1&&ret!=0x8a31)return 0;if(root==2&&ret!=0x8743)return 0;return tile_mapped68(call);}
static int tile_arm68(CpuState*c,uint64_t gen,unsigned root){TileOwner68*w=tile_owner68+root;w->armed=0;if(!tile_mode68(c)||!c->m_flag||c->S<0x120||c->S>0x1ffd||!tile_frame68(c,root))return 0;if(root==1){TileOwner68*p=tile_owner68;if(!p->armed||p->cpu!=c||p->ram!=c->ram||p->gen!=gen||p->s!=c->S+4||memcmp(p->frame,c->ram+p->s+1,2))return 0;}w->cpu=c;w->ram=c->ram;w->gen=gen;w->s=c->S;memcpy(w->frame,c->ram+c->S+1,2);w->armed=w->fresh=1;tile_stats68.entries++;return 1;}
static int tile_take68(CpuState*c,uint64_t gen,unsigned root){TileCall68*p=tile_pending68+root;int ok=p->armed&&p->cpu==c&&p->ram==c->ram&&p->gen==gen&&p->s==c->S+2&&tile_mapped68(p->pc)&&!memcmp(p->outer,c->ram+p->s+1,4);p->armed=0;return ok&&tile_arm68(c,gen,root);}
static int tile_observe68(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,unsigned s,unsigned h,unsigned host,unsigned root){if(root>1||c!=primary||gen!=frame||!c||!c->ram||c->S!=s||h!=2||c->host_return_valid!=2||s<0x120||s>0x1ffd||host!=(0x010000|(((c->ram[s+1]|c->ram[s+2]<<8)+1)&65535))||!LagoonTiles68_Source(r,n)||!tile_mapped68(tile_roots68[root]))return 0;int ok=tile_arm68(c,gen,root);if(ok)tile_pending68[root].armed=0;return ok;}
static void tile_nz68(CpuState*c,unsigned v,unsigned bits){unsigned mask=bits==8?255:65535;c->_flag_Z=!(v&mask);c->_flag_N=!!(v&(1u<<(bits-1)));cpu_mirrors_to_p(c);}
static void tile_a68(CpuState*c,unsigned v){c->A=c->m_flag?((c->A&0xff00)|(v&255)):(v&65535);tile_nz68(c,v,c->m_flag?8:16);}
static unsigned tile_read68(CpuState*c,unsigned p,unsigned bytes){uint8_t b;uint16_t w;if(bytes==1){if(!interp_bridge_source_read8(c,p,&b))abort();tile_extra68+=cpu_region_speed(p)-6;return b;}unsigned hi=(p&0xff0000)|((p+1)&65535);if(!interp_bridge_source_read16(c,p,hi,&w))abort();tile_extra68+=cpu_region_speed(p)-6+cpu_region_speed(hi)-6;return w;}
static void tile_write68(CpuState*c,unsigned p,unsigned v,unsigned bytes){if(bytes==1){if(!interp_bridge_source_write8(c,p,v))abort();tile_extra68+=cpu_region_speed(p)-6;}else{unsigned hi=(p&0xff0000)|((p+1)&65535);if(!interp_bridge_source_write16(c,p,hi,v))abort();tile_extra68+=cpu_region_speed(p)-6+cpu_region_speed(hi)-6;}}
static void tile_adc68(CpuState*c,unsigned y,int subtract){unsigned x=c->A,z=subtract?(y^65535):y,sum=x+z+c->_flag_C;c->_flag_C=sum>65535;c->_flag_V=!!((~(x^z)&(x^sum))&32768);tile_a68(c,sum);}
int LagoonTiles68_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,uint32_t pc,uint32_t*landing){
 for(unsigned j=0;j<sizeof tile_calls68/sizeof*tile_calls68;j++)if(tile_calls68[j][0]==pc){unsigned root=tile_calls68[j][2];TileCall68*p=tile_pending68+root;p->armed=0;if(c==primary&&gen==frame&&tile_mode68(c)&&c->m_flag&&c->S>=0x124&&c->S<=0x1ffb&&LagoonTiles68_Source(r,n)&&tile_mapped68(pc)){p->cpu=c;p->ram=c->ram;p->gen=gen;p->s=c->S;p->pc=pc;memcpy(p->outer,c->ram+c->S+1,4);p->armed=1;tile_owner68[root].armed=0;}return 0;}
 if(!landing||c!=primary||gen!=frame||!tile_mode68(c)||!LagoonTiles68_Source(r,n)){memset(tile_owner68,0,sizeof tile_owner68);memset(tile_pending68,0,sizeof tile_pending68);tile_stats68.refused++;return 0;}unsigned root=tile_family68(pc);TileOwner68*w=tile_owner68+root;
 if(pc==tile_roots68[root]&&!w->armed&&!tile_take68(c,gen,root))return 0;for(unsigned j=0;j<3;j++)if(pc!=tile_roots68[j])tile_pending68[j].armed=0;
 if(pc==0x018a5d||pc==0x018a91||pc==0x018b33){if(w->armed&&c->S==w->s)w->armed=0;return 0;}
 const unsigned*op=NULL;for(unsigned j=0;j<sizeof tile_ops68/sizeof*tile_ops68;j++)if(tile_ops68[j][0]==pc){op=tile_ops68[j];break;}
 if(!w->armed||w->cpu!=c||w->ram!=c->ram||w->gen!=gen||memcmp(w->frame,c->ram+w->s+1,2)){w->armed=0;tile_stats68.refused++;return 0;}
 if(root==1){TileOwner68*p=tile_owner68;if(!p->armed||p->cpu!=c||p->ram!=c->ram||p->gen!=gen||p->s!=w->s+4||memcmp(p->frame,c->ram+p->s+1,2)){w->armed=0;tile_stats68.refused++;return 0;}}
 if(!op)return 0;if(w->s!=c->S+op[4]||c->m_flag!=op[3]){w->armed=0;tile_stats68.refused++;return 0;}if(interp_bridge_source_opcode_deadline_reached(c)){tile_stats68.deadlines++;return 0;}
 unsigned at=tile_offset68(pc),bytes=op[1],cycles=op[2],im=bytes>1?r[at+1]:0,operand=im|(bytes>=3?r[at+2]<<8:0),address=0x010000|operand,next=pc+bytes,mbytes=c->m_flag?1:2;uint64_t prior=c->master_cycles;tile_extra68=0;
 if(!cycles){cycles=c->_flag_Z?2:3;if(!c->_flag_Z)next=pc+bytes+(int8_t)im;}c->coprocessor_master_cycles=prior;c->open_bus=r[at+bytes-1];CpuWramReadContext context={.instruction_pc=pc,.stack=c->S,.y=c->Y,.pb=c->PB,.m=c->m_flag,.x=c->x_flag,.e=c->emulation,.interpreter=1};const CpuWramReadContext*old=cpu_swap_wram_read_context(&context);
 switch(pc){
 case 0x0189f5:case 0x018a5e:case 0x018a64:c->X=tile_read68(c,address,2);tile_nz68(c,c->X,16);break;
 case 0x0189f8:case 0x018a76:case 0x018a88:case 0x018af9:case 0x018b11:tile_write68(c,im,c->X,2);break;
 case 0x0189fa:c->Y=operand;tile_nz68(c,c->Y,16);break;
 case 0x0189fd:case 0x018a11:case 0x018a33:case 0x018a48:case 0x018ad1:c->P&=~32;cpu_p_to_mirrors(c);break;
 case 0x018a0b:case 0x018a26:case 0x018a40:case 0x018a55:case 0x018af0:c->P|=32;cpu_p_to_mirrors(c);break;
 case 0x0189ff:case 0x018a13:case 0x018a1b:case 0x018a35:case 0x018a4a:case 0x018a81:tile_a68(c,tile_read68(c,bytes==2?im:address,mbytes));break;
 case 0x018a02:c->_flag_C=1;cpu_mirrors_to_p(c);break;
 case 0x018a03:tile_adc68(c,operand,1);break;
 case 0x018a06:case 0x018a3b:case 0x018a50:tile_write68(c,im,c->A,mbytes);break;
 case 0x018a08:case 0x018a23:case 0x018a3d:case 0x018a52:case 0x018aed:tile_a68(c,c->A&operand);break;
 case 0x018a0d:case 0x018a6a:case 0x018a73:case 0x018a85:case 0x018ad3:case 0x018af6:case 0x018aff:case 0x018b0e:case 0x018b17:c->X=operand;tile_nz68(c,c->X,16);break;
 case 0x018a10:tile_write68(c,c->S,c->X>>8,1);tile_write68(c,c->S-1,c->X,1);c->S-=2;break;
 case 0x018a15:case 0x018a16:case 0x018a17:case 0x018a1d:case 0x018a1e:case 0x018a1f:c->_flag_C=c->A&1;tile_a68(c,c->A>>1);break;
 case 0x018a18:case 0x018a20:case 0x018aea:case 0x018b07:tile_write68(c,address,c->A,mbytes);break;
 case 0x018a32:c->X=tile_read68(c,c->S+1,2);c->S+=2;tile_nz68(c,c->X,16);break;
 case 0x018a37:case 0x018a4c:c->_flag_C=0;cpu_mirrors_to_p(c);break;
 case 0x018a38:case 0x018a4d:tile_adc68(c,operand,0);break;
 case 0x018a42:case 0x018add:case 0x018ade:c->X=(c->X+1)&65535;tile_nz68(c,c->X,16);break;
 case 0x018a43:case 0x018adf:c->_flag_C=c->X>=operand;tile_nz68(c,c->X-operand,16);break;
 case 0x018a57:c->Y=(c->Y+1)&65535;tile_nz68(c,c->Y,16);break;
 case 0x018a58:c->_flag_C=c->Y>=operand;tile_nz68(c,c->Y-operand,16);break;
 case 0x018a61:case 0x018a67:case 0x018a6d:case 0x018a70:case 0x018ae4:case 0x018b02:case 0x018b1a:tile_write68(c,address,c->X,2);break;
 case 0x018a78:case 0x018afd:case 0x018b15:tile_write68(c,im,pc==0x018a78?0:c->A,1);break;
 case 0x018a7a:tile_write68(c,address,0,1);break;
 case 0x018a8a:{unsigned v=(tile_read68(c,address,1)+1)&255;tile_write68(c,address,v,1);tile_nz68(c,v,8);break;}
 case 0x018ad6:case 0x018ae7:case 0x018afb:case 0x018b05:case 0x018b13:case 0x018b21:case 0x018b27:case 0x018b2d:tile_a68(c,operand);break;
 case 0x018ad9:tile_write68(c,((unsigned)r[at+3]<<16|operand)+c->X,c->A,2);break;
 default:break;}
 w->fresh=0;cpu_swap_wram_read_context(old);c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2+tile_extra68;if(root==0)tile_stats68.rows++;else if(root==1)tile_stats68.queue++;else tile_stats68.hud++;if(!interp_bridge_commit_source_opcode(c,prior,false))abort();*landing=next;return 1;
}
static void tile_retain68(CpuState*c,uint64_t gen,unsigned pc){for(unsigned j=0;j<3;j++){TileOwner68*w=tile_owner68+j;unsigned in=j==0?(pc>=0x0189f5&&pc<=0x018a5d):j==1?(pc>=0x018a5e&&pc<=0x018a91):(pc>=0x018acd&&pc<=0x018b33);if(w->armed&&(!c||w->cpu!=c||w->ram!=c->ram||w->gen!=gen||c->S>w->s||memcmp(w->frame,c->ram+w->s+1,2)||(c->S==w->s&&!in)))w->armed=0;TileCall68*p=tile_pending68+j;if(p->armed){int pre=c&&pc==p->pc&&c->S==p->s,post=c&&pc==tile_roots68[j]&&c->S+2==p->s;if(!c||p->cpu!=c||p->ram!=c->ram||p->gen!=gen||(!pre&&!post)||memcmp(p->outer,c->ram+p->s+1,4))p->armed=0;}}}
#ifndef LAGOON_TILES68_TEST
#include "common_rtl.h"
#include <stdio.h>
static uint64_t tile_generation68;static int tile_enabled68=-1,tile_report68;
static void tile_report_exit68(void){if(tile_report68)fprintf(stderr,"[native-tiles68] entries=%llu rows=%llu queue=%llu hud=%llu deadlines=%llu refused=%llu\n",(unsigned long long)tile_stats68.entries,(unsigned long long)tile_stats68.rows,(unsigned long long)tile_stats68.queue,(unsigned long long)tile_stats68.hud,(unsigned long long)tile_stats68.deadlines,(unsigned long long)tile_stats68.refused);}
static bool tile_entry68(CpuState*c,uint16_t s,uint8_t h,uint32_t host,unsigned root){if(tile_enabled68)tile_observe68(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),tile_generation68,s,h,host,root);return false;}
bool LagoonTiles68_RowsEntry(CpuState*c,uint16_t s,uint8_t h,uint32_t host,RecompReturn*out){(void)out;return tile_entry68(c,s,h,host,0);}
bool LagoonTiles68_QueueEntry(CpuState*c,uint16_t s,uint8_t h,uint32_t host,RecompReturn*out){(void)out;return tile_entry68(c,s,h,host,1);}
static void tile_hook68(CpuState*c,uint32_t pc){if(!tile_enabled68||c!=&g_cpu||!interp_bridge_source_opcode_available(c))return;if(!tile_mapped68(pc)){memset(tile_owner68,0,sizeof tile_owner68);memset(tile_pending68,0,sizeof tile_pending68);return;}uint32_t landing;if(LagoonTiles68_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),tile_generation68,pc,&landing))interp_bridge_pre_opcode_redirect(landing);}
void LagoonTiles68_BeforeFrame(void){if(tile_enabled68<0){const char*v=getenv("LAGOON_TIMING68");tile_enabled68=!v||strcmp(v,"0");v=getenv("LAGOON_NATIVE_WAIT_REPORT");tile_report68=v&&!strcmp(v,"1");if(tile_report68)atexit(tile_report_exit68);}tile_generation68=RtlStateGeneration();tile_retain68(&g_cpu,tile_generation68,interp_bridge_lle_resume_pc());if(tile_enabled68){for(unsigned j=0;j<sizeof tile_ops68/sizeof*tile_ops68;j++)if(!interp_bridge_add_pre_opcode_hook(tile_ops68[j][0],tile_hook68))abort();for(unsigned j=0;j<sizeof tile_calls68/sizeof*tile_calls68;j++)if(!interp_bridge_add_pre_opcode_hook(tile_calls68[j][0],tile_hook68))abort();for(unsigned j=0;j<sizeof tile_boundaries68/sizeof*tile_boundaries68;j++)if(!interp_bridge_add_pre_opcode_hook(tile_boundaries68[j][0],tile_hook68))abort();}}
#endif
