/* Original byte-wide IPL acknowledgment polls during unmapped boot upload.
 * Never an idle replay: each CMP reads the actual asynchronous APU port. */
#include "lagoon_native_spc63.h"
#include "snes/interp_bridge.h"
#include <string.h>
#include <stdlib.h>
static struct {const CpuState*cpu;const uint8_t*ram;uint64_t generation;
 uint16_t s;uint32_t pointer;uint8_t db,saved_p,ret[2];int armed;} spc_witness;
static int spc64_enabled=1;
static struct {uint64_t armed,reads,branches,deadlines,refused,payload64,words64,writes64;} spc_stats;
int LagoonNativeSpc63_Source(const uint8_t*r,size_t n){
 if(!r||n!=0x100000)return 0;uint64_t h=UINT64_C(14695981039346656037);
 for(unsigned k=0x67f69;k<0x67fcc;++k)h=(h^r[k])*UINT64_C(1099511628211);
 if(h!=UINT64_C(0xbfffc88dd0db851e))return 0;
 h=UINT64_C(14695981039346656037);
 for(unsigned k=0x67e9b;k<0x68000;++k)h=(h^r[k])*UINT64_C(1099511628211);
 if(h!=UINT64_C(0x92b9c182f5a7c275))return 0;
 h=UINT64_C(14695981039346656037);
 for(unsigned k=0x68000;k<0x68d46;++k)h=(h^r[k])*UINT64_C(1099511628211);
 return h==UINT64_C(0xf92530e36c298aab);
}
static int spc_mode(const CpuState*c){return c&&c->ram&&c->PB==12&&c->DB<=2&&!c->D&&
 !c->emulation&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x1c)==4&&!!(c->P&0x20)==!!c->m_flag;}
static unsigned spc_pointer(const CpuState*c){return c->ram[0x1c]|c->ram[0x1d]<<8|c->ram[0x1e]<<16;}
static int spc_pack(const uint8_t*r,unsigned p){
 static const struct {unsigned p,n;uint64_t hash;} packs[]={
 {0x0c8000,24764,UINT64_C(0xb36f260378f7d934)},{0x0d8d46,28556,UINT64_C(0x804ad309af479599)},
 {0x0fa12e,6461,UINT64_C(0x1366f356cc6088e1)},{0x0fba6b,5100,UINT64_C(0xc0c86d80094f14a8)},
 {0x0fce57,1490,UINT64_C(0x4aa0346eddcb4b2e)},{0x0e8000,7445,UINT64_C(0x3a857526da7d58a6)},
 {0x0e9d15,9110,UINT64_C(0x141213c21c2cc7f5)},{0x0ec0ab,8028,UINT64_C(0xdf132f503ac6f21e)},
 {0x0f8000,8494,UINT64_C(0x999adb07d17671c8)},{0x0fd429,7900,UINT64_C(0x5e90eb241276b539)},
 {0x0d8000,3398,UINT64_C(0x5b367902e5ef1975)}};
 for(unsigned k=0;k<sizeof packs/sizeof *packs;++k)if(p==packs[k].p){
  uint64_t h=UINT64_C(1469598103934665603);unsigned at=(p>>16)*32768+(p&32767);
  for(unsigned j=0;j<packs[k].n;++j)h=(h^r[at+j])*UINT64_C(1099511628211);
  return h==packs[k].hash;
 }
 return 0;
}
/* Width-pinned remaining source instructions; PHP, PLP and RTS remain real
 * interpreter boundaries. Header PHA/PLA use the actual child stack. */
static const unsigned spc64_ops[][4]={
 {0xcff6a,2,3,1},{0xcff6c,3,3,0},{0xcff6f,3,3,0},{0xcff72,3,5,0},{0xcff75,2,0,0},{0xcff77,2,3,0},{0xcff79,2,2,1},{0xcff7b,2,3,1},
 {0xcff7d,2,6,1},{0xcff7f,1,2,1},{0xcff80,1,3,1},{0xcff81,2,2,1},{0xcff83,2,3,1},{0xcff85,1,3,1},{0xcff86,2,6,1},{0xcff88,1,2,1},{0xcff89,1,3,1},
 {0xcff8a,3,4,1},{0xcff8d,2,0,1},{0xcff8f,1,2,1},{0xcff90,2,3,1},{0xcff92,3,5,0},{0xcff95,2,3,0},{0xcff97,1,2,1},{0xcff98,2,0,1},
 {0xcff9a,3,4,1},{0xcff9d,2,0,1},{0xcff9f,2,2,1},{0xcffa1,2,0,1},{0xcffa3,1,3,1},{0xcffa4,2,3,1},{0xcffa6,2,7,0},{0xcffa8,1,2,0},{0xcffa9,1,2,0},{0xcffaa,1,2,0},{0xcffab,2,7,0},{0xcffad,1,2,0},{0xcffae,1,2,0},{0xcffaf,3,5,0},
 {0xcffb2,2,3,0},{0xcffb4,3,3,1},{0xcffb7,2,2,1},{0xcffb9,1,2,1},{0xcffba,3,4,1},{0xcffbd,2,2,1},{0xcffbf,1,4,1},{0xcffc0,3,4,1},{0xcffc3,3,4,1},{0xcffc6,2,0,1},{0xcffc8,2,0,1}
};
static void spc64_nz(CpuState*c,unsigned v,unsigned bits){unsigned mask=bits==8?255:65535;c->_flag_Z=!(v&mask);c->_flag_N=!!(v&(1u<<(bits-1)));cpu_mirrors_to_p(c);}
static void spc64_a(CpuState*c,unsigned v){if(c->m_flag){c->A=(c->A&0xff00)|(v&255);spc64_nz(c,v,8);}else{c->A=v;spc64_nz(c,v,16);}}
static void spc64_cmp(CpuState*c,unsigned a,unsigned b,unsigned bits){unsigned mask=bits==8?255:65535;c->_flag_C=(a&mask)>=(b&mask);spc64_nz(c,(a-b)&mask,bits);}
static void spc64_adc(CpuState*c,unsigned b){unsigned a=c->A&255,sum=a+b+c->_flag_C;c->_flag_C=sum>255;c->_flag_V=!!((~(a^b)&(a^sum))&128);spc64_a(c,sum);}
static int spc64_is_input(unsigned pc){return pc==0xcff7d||pc==0xcff86||pc==0xcffa6||pc==0xcffab;}
static int spc64_input_valid(const CpuState*c){unsigned p=spc_pointer(c)+c->Y;return p<0x200000&&(p&65535)>=32768&&(c->m_flag||(p&65535)!=65535);}
static void spc64_input(CpuState*c){unsigned p=cpu_read8_timed(c,0,0x1c)|cpu_read8_timed(c,0,0x1d)<<8|cpu_read8_timed(c,0,0x1e)<<16;p=(p+c->Y)&0xffffff;unsigned v=cpu_read8_timed(c,p>>16,p);if(!c->m_flag){p=(p+1)&0xffffff;v|=cpu_read8_timed(c,p>>16,p)<<8;}spc64_a(c,v);}
int LagoonNativeSpc63_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,
 uint64_t generation,uint64_t frame,uint32_t pc,uint32_t*landing){
 if(!landing||c!=primary||generation!=frame||!spc_mode(c)||!LagoonNativeSpc63_Source(r,n)){
  spc_witness.armed=0;++spc_stats.refused;return 0;
 }
 if(pc==0x0cff69){
  spc_witness.armed=0;
  if(!c->m_flag||c->S<0x100||c->S>0x1ff8)return 0;
  unsigned ret=c->ram[c->S+1]|c->ram[c->S+2]<<8,p=spc_pointer(c);
  int context=ret==0xff5b?!c->DB&&p==0x0d8000:
    ret==0xfee2?c->DB&&(p==0x0c8000||p==0x0d8d46):
    ret==0xff33?c->DB&&p!=0x0c8000&&p!=0x0d8d46&&p!=0x0d8000:0;
  if(!context||!spc_pack(r,p))return 0;
  spc_witness.cpu=c;spc_witness.ram=c->ram;spc_witness.generation=generation;spc_witness.s=c->S;
  spc_witness.pointer=p;spc_witness.db=c->DB;CpuState status=*c;cpu_mirrors_to_p(&status);spc_witness.saved_p=status.P;memcpy(spc_witness.ret,c->ram+c->S+1,2);spc_witness.armed=1;++spc_stats.armed;return 0;
 }
 if(pc==0x0cffca){spc_witness.armed=0;return 0;}
 if(!spc_witness.armed||spc_witness.cpu!=c||spc_witness.ram!=c->ram||
   spc_witness.generation!=generation||c->DB!=spc_witness.db||c->ram[spc_witness.s]!=spc_witness.saved_p||
   c->S+(pc>=0xcffa4&&pc<=0xcffbf?2:1)!=spc_witness.s||
   memcmp(spc_witness.ret,c->ram+spc_witness.s+1,2)||
   spc_pointer(c)!=spc_witness.pointer){
  spc_witness.armed=0;++spc_stats.refused;return 0;
 }
 unsigned executed=0;
 for(;;){
  const unsigned*op=0;for(unsigned k=0;k<sizeof spc64_ops/sizeof *spc64_ops;++k)if(spc64_ops[k][0]==pc){op=spc64_ops[k];break;}
  if((!spc64_enabled&&pc!=0xcff8a&&pc!=0xcff8d&&pc!=0xcff9a&&pc!=0xcff9d&&pc!=0xcffc3&&pc!=0xcffc6)||!op||c->m_flag!=op[3]){if(!executed)return 0;*landing=pc;return 1;}
  if(interp_bridge_source_opcode_deadline_reached(c)){++spc_stats.deadlines;*landing=pc;return 1;}
  if(spc64_is_input(pc)&&!spc64_input_valid(c)){if(!executed)return 0;*landing=pc;return 1;}
  unsigned cycles=op[2],bytes=op[1],next=pc+bytes,wrote=0;uint64_t prior=c->master_cycles;
  c->coprocessor_master_cycles=prior;c->open_bus=r[(pc&32767)+0x60000+bytes-1];
  if(!cycles){unsigned take=pc==0xcffa1?c->_flag_Z:pc==0xcffc8?c->_flag_V:!c->_flag_Z;
   cycles=take?3:2;if(take)next=pc+bytes+(int8_t)r[(pc&32767)+0x60001];}
  unsigned port=((unsigned)c->DB<<16)|0x2140,v=0;
  if(pc==0xcff72){uint16_t word;if(!interp_bridge_source_read16(c,port,port+1,&word))abort();spc64_cmp(c,c->A,word,16);++spc_stats.words64;}
  if(pc==0xcff8a||pc==0xcff9a||pc==0xcffc3){uint8_t byte;if(!interp_bridge_source_read8(c,port,&byte))abort();spc64_cmp(c,c->A,byte,8);++spc_stats.reads;}
  if(pc==0xcff92||pc==0xcffaf){unsigned a=port+(pc==0xcffaf?2:0);if(!interp_bridge_source_write16(c,a,a+1,c->A))abort();++spc_stats.writes64;}
  if(pc==0xcffba||pc==0xcffc0){if(!interp_bridge_source_write8(c,port+(pc==0xcffba?1:0),c->A))abort();++spc_stats.writes64;}
  c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2;
  if(pc==0xcff72||pc==0xcff92||pc==0xcffaf)c->master_cycles+=2*(cpu_region_speed(port)-6);
  if(pc==0xcff8a||pc==0xcff9a||pc==0xcffc3||pc==0xcffba||pc==0xcffc0)c->master_cycles+=cpu_region_speed(port)-6;
  switch(pc){
  case 0xcff6a:c->P&=~0x30;cpu_p_to_mirrors(c);break;
  case 0xcff6c:c->Y=0;spc64_nz(c,0,16);break;
  case 0xcff6f:c->A=0xbbaa;spc64_nz(c,c->A,16);break;
  case 0xcff77:case 0xcff95:case 0xcffb2:c->P|=32;cpu_p_to_mirrors(c);break;
  case 0xcff79:spc64_a(c,0xcc);break;
  case 0xcff7b:next=0xcffa3;break;
  case 0xcff7d:case 0xcff86:case 0xcffa6:case 0xcffab:spc64_input(c);break;
  case 0xcff7f:case 0xcff88:case 0xcffa8:case 0xcffa9:case 0xcffad:case 0xcffae:c->Y=(uint16_t)(c->Y+1);spc64_nz(c,c->Y,16);break;
  case 0xcff80:case 0xcff85:case 0xcff89:c->A=(uint16_t)((c->A>>8)|(c->A<<8));spc64_nz(c,c->A&255,8);break;
  case 0xcff81:case 0xcffb7:spc64_a(c,0);break;
  case 0xcff83:next=0xcff90;break;
  case 0xcff8f:spc64_a(c,c->A+1);break;
  case 0xcff90:case 0xcffa4:c->P&=~32;cpu_p_to_mirrors(c);break;
  case 0xcff97:c->X=(uint16_t)(c->X-1);spc64_nz(c,c->X,16);break;
  case 0xcff9f:spc64_adc(c,3);break;
  case 0xcffa3:cpu_write8_timed(c,0,c->S,c->A);--c->S;wrote=1;break;
  case 0xcffaa:c->X=c->A;spc64_nz(c,c->X,16);break;
  case 0xcffb4:spc64_cmp(c,c->X,1,16);break;
  case 0xcffb9:v=(c->A&255)*2+c->_flag_C;c->_flag_C=!!(v&256);spc64_a(c,v);break;
  case 0xcffbd:spc64_adc(c,0x7f);break;
  case 0xcffbf:spc64_a(c,cpu_read8_timed(c,0,++c->S));break;
  default:break;
  }
  if(pc==0xcff8d||pc==0xcff9d||pc==0xcffc6)++spc_stats.branches;else if(pc!=0xcff8a&&pc!=0xcff9a&&pc!=0xcffc3)++spc_stats.payload64;
  ++executed;if(!interp_bridge_commit_source_opcode(c,prior,wrote))abort();pc=next;
  if(pc==0xcffca){*landing=pc;return 1;}
 }
}

#ifndef LAGOON_NATIVE_SPC63_TEST
#include "common_rtl.h"
#include <stdio.h>
static uint64_t spc_frame;static int spc_enabled=-1,spc_report;
void LagoonNativeSpc63_ObserveEntry(CpuState*c,uint16_t s,uint8_t h,uint32_t host){
 if(spc_enabled!=1||c!=&g_cpu||!c->ram||c->S!=s||s>0x1ff8||h!=2||c->host_return_valid!=2||
  (host!=0x0cfee3&&host!=0x0cff34&&host!=0x0cff5c))return;
 unsigned physical=c->ram[s+1]|c->ram[s+2]<<8;if(((physical+1)&65535)!=(host&65535))return;
 uint32_t unused;LagoonNativeSpc63_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),spc_frame,0xcff69,&unused);
}
static void spc_exit(void){if(spc_report)fprintf(stderr,"[native-spc63] armed=%llu reads=%llu branches=%llu deadlines=%llu refused=%llu payload64=%llu words64=%llu writes64=%llu\n",
 (unsigned long long)spc_stats.armed,(unsigned long long)spc_stats.reads,(unsigned long long)spc_stats.branches,(unsigned long long)spc_stats.deadlines,(unsigned long long)spc_stats.refused,(unsigned long long)spc_stats.payload64,(unsigned long long)spc_stats.words64,(unsigned long long)spc_stats.writes64);}
static void spc_hook(CpuState*c,uint32_t pc){
 if(!spc_enabled||c!=&g_cpu)return;
 /* An unyieldable nested observation is original-only. Retain the witnessed
  * real frame; resume only after all callback/source/frame guards pass again. */
 if(!interp_bridge_source_opcode_available(c))return;
 for(unsigned k=0x67f69;k<0x67fcc;++k)if(!cpu_aot_rom_mapping_matches(0x0c8000u+(k&32767),k,1)){spc_witness.armed=0;return;}
 uint32_t landing;
 if(LagoonNativeSpc63_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),spc_frame,pc,&landing))interp_bridge_pre_opcode_redirect(landing);
}
void LagoonNativeSpc63_BeforeFrame(void){
 if(spc_enabled<0){const char*timing=getenv("LAGOON_TIMING64");spc64_enabled=!timing||strcmp(timing,"0");const char*v=getenv("LAGOON_NATIVE_SPC63");spc_enabled=!v||!strcmp(v,"on");v=getenv("LAGOON_NATIVE_WAIT_REPORT");spc_report=v&&!strcmp(v,"1");if(spc_report)atexit(spc_exit);}
 spc_frame=RtlStateGeneration();if(spc_witness.armed&&spc_witness.generation!=spc_frame)spc_witness.armed=0;
 if(spc_witness.armed&&!spc_pack(g_rom,spc_witness.pointer))spc_witness.armed=0;
 if(spc_enabled){const unsigned pcs[]={0x0cff69,0x0cff8a,0x0cff8d,0x0cff9a,0x0cff9d,0x0cffc3,0x0cffc6,0x0cffca};
  for(unsigned k=0;k<sizeof pcs/sizeof *pcs;++k)interp_bridge_add_pre_opcode_hook(pcs[k],spc_hook);
  for(unsigned k=0;k<sizeof spc64_ops/sizeof *spc64_ops;++k)interp_bridge_add_pre_opcode_hook(spc64_ops[k][0],spc_hook);}
}
#endif
