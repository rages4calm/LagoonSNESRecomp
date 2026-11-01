/* Original new-game RAM reset, witnessed at its real frameless80AF jump.
 * Cold reset803D is already qualified separately. No return is synthesized. */
#include "lagoon_native_reset63.h"
#include "snes/interp_bridge.h"
#include <string.h>
#include <stdlib.h>
static struct {const CpuState*cpu;const uint8_t*ram;uint64_t generation;uint32_t next;int armed;} reset_witness;
static struct {uint64_t armed,instructions,complete,deadlines,refused;} reset_stats;
int LagoonNativeReset63_Source(const uint8_t*r,size_t n){if(!r||n!=0x100000||memcmp(r+0xaf,"\x5c\x04\x93\x00",4))return 0;uint64_t h=UINT64_C(14695981039346656037);for(unsigned k=0x1304;k<0x132f;++k)h=(h^r[k])*UINT64_C(1099511628211);return h==UINT64_C(0xb7b3d1b2b4e79ebe);}
static int reset_mode(const CpuState*c){return c&&c->ram&&!c->PB&&!c->DB&&!c->D&&c->S==0x1ff&&!c->emulation&&c->m_flag&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x3c)==0x24;}
static void reset_nz(CpuState*c,unsigned v,unsigned sign){c->_flag_Z=!v;c->_flag_N=!!(v&sign);cpu_mirrors_to_p(c);}
int LagoonNativeReset63_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t generation,uint64_t frame,uint32_t pc,uint32_t*landing){
 if(!landing||c!=primary||generation!=frame||!reset_mode(c)||!LagoonNativeReset63_Source(r,n)){reset_witness.armed=0;++reset_stats.refused;return 0;}
 if(pc==0x80af){reset_witness.armed=0;if(c->host_return_valid!=2)return 0;reset_witness.cpu=c;reset_witness.ram=c->ram;reset_witness.generation=generation;reset_witness.next=0x9304;reset_witness.armed=1;return 0;}
 if(!reset_witness.armed||reset_witness.cpu!=c||reset_witness.ram!=c->ram||reset_witness.generation!=generation||reset_witness.next!=pc){reset_witness.armed=0;++reset_stats.refused;return 0;}
 if(pc==0x9304)++reset_stats.armed;
 for(;;){
 if(interp_bridge_source_opcode_deadline_reached(c)){reset_witness.next=pc;++reset_stats.deadlines;*landing=pc;return 1;}
 unsigned bytes,cycles,next,wrote=0;
 switch(pc){
 case 0x9304:case 0x9310:case 0x931c:bytes=3;cycles=3;next=pc+3;break;
 case 0x9307:case 0x9313:bytes=3;cycles=5;next=pc+3;break;
 case 0x930a:case 0x9316:case 0x9325:case 0x932c:bytes=1;cycles=2;next=pc+1;break;
 case 0x930b:case 0x9317:bytes=3;cycles=3;next=pc+3;break;
 case 0x930e:case 0x931a:case 0x9326:case 0x932d:bytes=2;cycles=c->_flag_Z?2:3;next=c->_flag_Z?pc+2:pc+(pc<0x9320?-7:-5);break;
 case 0x931f:bytes=2;cycles=2;next=pc+2;break;
 case 0x9321:case 0x9328:bytes=4;cycles=5;next=pc+4;break;
 default:reset_witness.armed=0;return 0;}
 uint64_t prior=c->master_cycles;c->open_bus=r[(pc&32767)+bytes-1];c->coprocessor_master_cycles=c->master_cycles;c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2;
 switch(pc){
 case 0x9304:case 0x9310:case 0x931c:c->X=pc==0x9304?0x40:pc==0x9310?0x2c0:0x4000;reset_nz(c,c->X,32768);break;
 case 0x9307:case 0x9313:cpu_write8_timed(c,c->DB,c->X,0);wrote=1;break;
 case 0x930a:case 0x9316:case 0x9325:case 0x932c:c->X=(uint16_t)(c->X+1);reset_nz(c,c->X,32768);break;
 case 0x930b:case 0x9317:{unsigned v=pc==0x930b?0x100:0x2000;c->_flag_C=c->X>=v;reset_nz(c,(uint16_t)(c->X-v),32768);break;}
 case 0x931f:c->A&=0xff00;reset_nz(c,0,128);break;
 case 0x9321:case 0x9328:cpu_write8_timed(c,pc==0x9321?0x7e:0x7f,c->X,c->A);wrote=1;break;
 default:break;}
 ++reset_stats.instructions;if(!interp_bridge_commit_source_opcode(c,prior,wrote))abort();pc=next;
 if(pc==0x932f){reset_witness.armed=0;++reset_stats.complete;*landing=pc;return 1;}
 }
}
#ifndef LAGOON_NATIVE_RESET63_TEST
#include "common_rtl.h"
#include <stdio.h>
static uint64_t reset_frame;static int reset_enabled=-1,reset_report;
static void reset_exit(void){if(reset_report)fprintf(stderr,"[native-reset63] armed=%llu instructions=%llu complete=%llu deadlines=%llu refused=%llu\n",(unsigned long long)reset_stats.armed,(unsigned long long)reset_stats.instructions,(unsigned long long)reset_stats.complete,(unsigned long long)reset_stats.deadlines,(unsigned long long)reset_stats.refused);}
static void reset_hook(CpuState*c,uint32_t pc){if(!reset_enabled||c!=&g_cpu)return;if(!interp_bridge_source_opcode_available(c))return;if(!cpu_aot_rom_mapping_matches(0x80af,0xaf,4)){reset_witness.armed=0;return;}for(unsigned k=0x1304;k<0x132f;++k)if(!cpu_aot_rom_mapping_matches(0x8000u+k,k,1)){reset_witness.armed=0;return;}uint32_t landing;if(LagoonNativeReset63_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),reset_frame,pc,&landing))interp_bridge_pre_opcode_redirect(landing);}
void LagoonNativeReset63_BeforeFrame(void){if(reset_enabled<0){const char*v=getenv("LAGOON_NATIVE_RESET63");reset_enabled=!v||!strcmp(v,"on");v=getenv("LAGOON_NATIVE_WAIT_REPORT");reset_report=v&&!strcmp(v,"1");if(reset_report)atexit(reset_exit);}reset_frame=RtlStateGeneration();if(reset_witness.armed&&reset_witness.generation!=reset_frame)reset_witness.armed=0;if(reset_enabled){const unsigned pcs[]={0x80af,0x9304,0x9307,0x930a,0x930b,0x930e,0x9310,0x9313,0x9316,0x9317,0x931a,0x931c,0x931f,0x9321,0x9325,0x9326,0x9328,0x932c,0x932d};for(unsigned k=0;k<sizeof pcs/sizeof *pcs;++k)interp_bridge_add_pre_opcode_hook(pcs[k],reset_hook);}}
#endif
