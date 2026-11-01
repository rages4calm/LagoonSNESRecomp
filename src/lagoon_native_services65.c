/* Original persistence, scene-clear and VRAM-fill source windows.
 * Calls, returns and unyieldable interrupt handlers remain original boundaries. */
#include "lagoon_native_services65.h"
#include "snes/interp_bridge.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
static const struct {unsigned pc,n;uint64_t hash;} spans[]={
 {0x936a,311,UINT64_C(0x8ad3ca78507bdbf3)},
 {0x834b,121,UINT64_C(0x0b19d774a6620fdb)},
 {0x02c5c6,15,UINT64_C(0x3c6b70c2b6d72270)}};
static const struct {unsigned pc,frame,max_depth;} roots[]={
 {0x9370,3,2},{0x93a2,2,0},{0x93bb,3,0},{0x93db,2,4},
 {0x942b,2,0},{0x9470,2,0},{0x9480,3,2},
 {0x834b,3,2},{0x8365,3,2},{0x837f,3,2},{0x8399,3,2},
 {0x02c5c6,3,0}};
typedef struct {const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,db;uint8_t frame[3];int armed;} Owner;
static Owner owners[sizeof roots/sizeof *roots];
static struct {uint64_t armed,persistence,scene,vram,deadlines,refused;} services_stats;
int LagoonServices65_Source(const uint8_t*r,size_t n){
 if(!r||n!=0x100000)return 0;
 for(unsigned j=0;j<sizeof spans/sizeof *spans;j++){unsigned at=(spans[j].pc>>16)*32768+(spans[j].pc&32767);uint64_t h=UINT64_C(14695981039346656037);for(unsigned k=0;k<spans[j].n;k++)h=(h^r[at+k])*UINT64_C(1099511628211);if(h!=spans[j].hash)return 0;}return 1;
}
static int mode(const CpuState*c){return c&&c->ram&&c->DB<=2&&!c->D&&!c->emulation&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x1c)==4&&!!(c->P&32)==!!c->m_flag;}
static int family(unsigned pc){
 if(pc>=0x9370&&pc<=0x93a1)return 0;if(pc>=0x93a2&&pc<=0x93ba)return 1;
 if(pc>=0x93bb&&pc<=0x93da)return 2;if(pc>=0x93db&&pc<=0x942a)return 3;
 if(pc>=0x942b&&pc<=0x945d)return 4;if(pc>=0x9470&&pc<=0x947f)return 5;
 if(pc>=0x9480&&pc<=0x94a0)return 6;
 if(pc>=0x83b0&&pc<=0x83c2)return 7;
 if(pc>=0x02c5c6&&pc<0x02c5d5)return 11;return -1;
}
static int arm(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,unsigned pc){
 for(unsigned j=0;j<sizeof roots/sizeof *roots;j++)if(roots[j].pc==pc){
  if(j>=7&&j<11)for(unsigned k=7;k<11;k++)owners[k].armed=0;
  Owner*w=owners+j;w->armed=0;if(c!=primary||gen!=frame||!mode(c)||!c->m_flag||c->PB!=(pc>>16)||c->S<0x120||c->S>0x1fc||!LagoonServices65_Source(r,n))return 0;
  w->cpu=c;w->ram=c->ram;w->gen=gen;w->s=c->S;w->db=c->DB;memcpy(w->frame,c->ram+c->S+1,roots[j].frame);w->armed=1;++services_stats.armed;return 1;
 }
 return 0;
}
/* pc, source bytes, base cycles, accumulator width. No JSR/JSL/RTS/RTL.
 * Those instructions execute through the real bridge with real nested frames. */
static const unsigned ops[][4]={
 {0x9370,1,4,1},{0x9371,3,3,1},{0x9374,3,5,1},{0x9377,3,5,1},{0x937a,4,5,1},{0x937e,4,5,1},
 {0x9385,1,2,1},{0x9386,3,3,1},{0x9389,2,0,1},{0x938e,2,3,1},{0x9390,3,5,0},{0x9393,4,6,0},{0x9397,4,6,0},{0x939b,3,3,0},{0x939e,2,3,0},{0x93a0,1,5,1},
 {0x93a2,3,3,1},{0x93a5,4,5,1},{0x93a9,4,5,1},{0x93ad,4,5,1},{0x93b4,1,2,1},{0x93b5,3,3,1},{0x93b8,2,0,1},
 {0x93bb,2,2,1},{0x93c0,2,2,1},{0x93c2,2,0,1},{0x93c4,2,2,1},{0x93c9,2,2,1},{0x93cb,2,0,1},{0x93d1,2,2,1},{0x93d8,2,2,1},
 {0x93db,1,4,1},{0x93dc,1,4,1},{0x93e0,3,3,1},{0x93e3,3,5,1},{0x93e6,2,6,1},{0x93e8,3,5,1},{0x93ee,1,2,1},{0x93ef,3,3,1},{0x93f2,2,0,1},
 {0x93f4,3,3,1},{0x93f7,1,2,1},{0x93f8,2,6,1},{0x93fa,4,5,1},{0x93fe,2,0,1},{0x9400,2,2,1},{0x9402,1,5,1},{0x9403,1,5,1},
 {0x9408,1,2,1},{0x9409,1,2,1},{0x940a,3,3,1},{0x940d,2,0,1},{0x940f,2,3,1},{0x9411,3,3,0},{0x9414,2,7,0},{0x9416,3,5,0},{0x9419,2,0,0},{0x941b,3,3,0},{0x941e,2,3,0},{0x9420,3,3,0},{0x9423,3,3,0},{0x9426,2,3,0},{0x9428,1,5,1},{0x9429,1,5,1},
 {0x942b,2,3,1},{0x942d,1,2,1},{0x942e,1,2,1},{0x942f,2,3,1},{0x9431,1,2,1},{0x9432,2,3,1},{0x9434,4,6,0},{0x9438,2,4,0},{0x943a,4,6,0},{0x943e,2,4,0},{0x9440,4,6,0},{0x9444,2,4,0},{0x9446,3,3,0},{0x9449,2,3,0},{0x944b,4,5,1},{0x944f,2,3,1},{0x9451,4,5,1},{0x9455,2,3,1},{0x9457,4,5,1},{0x945b,2,3,1},
 {0x9470,1,2,1},{0x9471,3,4,1},{0x9474,3,4,1},{0x9477,3,4,1},{0x947a,2,2,1},{0x947c,3,4,1},
 {0x9480,1,4,1},{0x9481,3,3,1},{0x9484,2,2,1},{0x9486,4,5,1},{0x948a,1,2,1},{0x948b,3,3,1},{0x948e,2,0,1},{0x9493,3,3,1},{0x9496,3,5,1},{0x9499,1,2,1},{0x949a,3,3,1},{0x949d,2,0,1},{0x949f,1,5,1},
 {0x83b0,2,3,1},{0x83b2,1,3,0},{0x83b3,1,2,0},{0x83b4,3,3,0},{0x83b7,3,5,0},{0x83ba,1,2,0},{0x83bb,2,0,0},{0x83bd,3,3,0},{0x83c0,2,3,0},
 {0x02c5c6,2,3,1},{0x02c5c8,3,3,1},{0x02c5cb,4,5,1},{0x02c5cf,1,2,1},{0x02c5d0,3,3,1},{0x02c5d3,2,0,1}
};
static unsigned offset(unsigned pc){return (pc>>16)*32768+(pc&32767);}
static void nz(CpuState*c,unsigned v,unsigned bits){unsigned mask=bits==8?255:65535;c->_flag_Z=!(v&mask);c->_flag_N=!!(v&(1u<<(bits-1)));cpu_mirrors_to_p(c);}
static void a(CpuState*c,unsigned v){if(c->m_flag)c->A=(c->A&0xff00)|(v&255);else c->A=v;nz(c,v,c->m_flag?8:16);}
static void cmp(CpuState*c,unsigned x,unsigned y,unsigned bits){unsigned mask=bits==8?255:65535;c->_flag_C=(x&mask)>=(y&mask);nz(c,(x-y)&mask,bits);}
static void adc(CpuState*c,unsigned v){unsigned mask=c->m_flag?255:65535,sign=c->m_flag?128:32768,x=c->A&mask,y=v&mask,sum=x+y+c->_flag_C;c->_flag_C=sum>mask;c->_flag_V=!!((~(x^y)&(x^sum))&sign);a(c,sum);}
static unsigned read8(CpuState*c,unsigned p){uint8_t v;if(!interp_bridge_source_read8(c,p&0xffffff,&v))abort();c->master_cycles+=cpu_region_speed(p&0xffffff)-6;return v;}
static unsigned read16(CpuState*c,unsigned l,unsigned h){uint16_t v;if(!interp_bridge_source_read16(c,l&0xffffff,h&0xffffff,&v))abort();c->master_cycles+=cpu_region_speed(l&0xffffff)-6+cpu_region_speed(h&0xffffff)-6;return v;}
static unsigned readm(CpuState*c,unsigned p,int linear){return c->m_flag?read8(c,p):read16(c,p,linear?(p+1)&0xffffff:(p&0xff0000)|((p+1)&65535));}
static void write8(CpuState*c,unsigned p,unsigned v){if(!interp_bridge_source_write8(c,p&0xffffff,v))abort();c->master_cycles+=cpu_region_speed(p&0xffffff)-6;}
static void writem(CpuState*c,unsigned p,unsigned v,int linear){if(c->m_flag)write8(c,p,v);else{unsigned h=linear?(p+1)&0xffffff:(p&0xff0000)|((p+1)&65535);if(!interp_bridge_source_write16(c,p,h,v))abort();c->master_cycles+=cpu_region_speed(p)-6+cpu_region_speed(h)-6;}}
static void push(CpuState*c,unsigned v,unsigned n){if(n==2){write8(c,c->S,v>>8);--c->S;}write8(c,c->S,v);--c->S;}
static unsigned pull(CpuState*c){unsigned v=read8(c,++c->S);return v|read8(c,++c->S)<<8;}
int LagoonServices65_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,uint32_t pc,uint32_t*landing){
 if(!landing||c!=primary||gen!=frame||!mode(c)||!LagoonServices65_Source(r,n)){memset(owners,0,sizeof owners);++services_stats.refused;return 0;}
 for(unsigned j=0;j<sizeof roots/sizeof *roots;j++)if(pc==roots[j].pc)arm(r,n,c,primary,gen,frame,pc);
 int f=family(pc);if(f==7){f=-1;for(unsigned j=7;j<11;j++)if(owners[j].armed){if(f>=0){f=-1;break;}f=j;}}
 if(f<0)return 0;Owner*w=owners+f;unsigned executed=0;
 for(;;){
  if(!w->armed||w->cpu!=c||w->ram!=c->ram||w->gen!=gen||c->PB!=(pc>>16)||c->DB!=w->db||c->S>w->s||c->S+roots[f].max_depth<w->s||memcmp(w->frame,c->ram+w->s+1,roots[f].frame)){w->armed=0;++services_stats.refused;if(!executed)return 0;*landing=pc;return 1;}
  const unsigned*op=0;for(unsigned k=0;k<sizeof ops/sizeof *ops;k++)if(ops[k][0]==pc&&ops[k][3]==c->m_flag){op=ops[k];break;}
  if(!op){if(!executed)return 0;*landing=pc;return 1;}
  if(interp_bridge_source_opcode_deadline_reached(c)){++services_stats.deadlines;*landing=pc;return 1;}
  unsigned at=offset(pc),bytes=op[1],cycles=op[2],next=pc+bytes,im=bytes>=2?r[at+1]:0;unsigned operand=im|(bytes>=3?r[at+2]<<8:0);unsigned dp=im,abs=((unsigned)c->DB<<16)|operand,lp=operand|(bytes==4?r[at+3]<<16:0);uint64_t prior=c->master_cycles;
  if(!cycles){unsigned take=(pc==0x93c2||pc==0x93cb||pc==0x93fe)?c->_flag_Z:!c->_flag_Z;cycles=take?3:2;if(take)next=pc+bytes+(int8_t)im;}
  c->coprocessor_master_cycles=prior;c->open_bus=r[at+bytes-1];
  /* Bus observations keep the original opcode-start watermark. Charge the
   * exact opcode/internal/fetch total only after real data accesses finish. */
  switch(pc){
  case 0x9370:case 0x93db:case 0x9480:push(c,c->X,2);break;
  case 0x93dc:push(c,c->Y,2);break;
  case 0x9371:case 0x93a2:case 0x9481:case 0x9493:case 0x02c5c8:c->X=operand;nz(c,c->X,16);break;
  case 0x9374:write8(c,abs,c->X);write8(c,(abs&0xff0000)|((abs+1)&65535),c->X>>8);break;
  case 0x9377:a(c,readm(c,(abs+c->X)&0xffffff,1));break;
  case 0x937a:case 0x937e:case 0x93a9:case 0x93ad:case 0x9486:case 0x02c5cb:writem(c,(lp+c->X)&0xffffff,c->A,1);break;
  case 0x9385:case 0x93b4:case 0x9408:case 0x948a:case 0x9499:case 0x02c5cf:c->X=(uint16_t)(c->X+1);nz(c,c->X,16);break;
  case 0x9386:case 0x93b5:case 0x948b:case 0x949a:case 0x02c5d0:cmp(c,c->X,operand,16);break;
  case 0x938e:case 0x940f:case 0x9432:case 0x83b0:c->P&=~32;cpu_p_to_mirrors(c);break;
  case 0x9390:case 0x9477:a(c,readm(c,abs,0));break;
  case 0x9393:case 0x9397:writem(c,lp,c->A,1);break;
  case 0x939b:case 0x9423:case 0x9446:case 0x83bd:a(c,c->A&operand);break;
  case 0x939e:case 0x9426:case 0x9449:case 0x83c0:c->P|=32;cpu_p_to_mirrors(c);break;
  case 0x93a0:case 0x9403:case 0x9429:case 0x949f:c->X=pull(c);nz(c,c->X,16);break;
  case 0x93a5:case 0x9434:case 0x943a:case 0x9440:case 0x944b:case 0x9451:case 0x9457:a(c,readm(c,(lp+c->X)&0xffffff,1));break;
  case 0x93bb:case 0x93c4:case 0x93d1:case 0x93d8:case 0x9400:case 0x941b:case 0x9420:case 0x9484:case 0x83b4:a(c,operand);break;
  case 0x93c0:case 0x93c9:cmp(c,c->A,operand,8);break;
  case 0x93e0:case 0x93f4:case 0x9411:c->Y=operand;nz(c,c->Y,16);break;
  case 0x93e3:write8(c,abs,c->Y);write8(c,(abs&0xff0000)|((abs+1)&65535),c->Y>>8);break;
  case 0x93e6:case 0x93f8:case 0x9414:{unsigned p=read8(c,dp);p|=read8(c,dp+1)<<8;p|=read8(c,dp+2)<<16;a(c,readm(c,(p+c->Y)&0xffffff,1));break;}
  case 0x93e8:writem(c,(abs+c->Y)&0xffffff,c->A,1);break;
  case 0x93ee:case 0x9409:c->Y=(uint16_t)(c->Y+1);nz(c,c->Y,16);break;
  case 0x93ef:case 0x940a:cmp(c,c->Y,operand,16);break;
  case 0x93f7:c->X=c->Y;nz(c,c->X,16);break;
  case 0x93fa:cmp(c,c->A,readm(c,(lp+c->X)&0xffffff,1),8);break;
  case 0x9402:case 0x9428:c->Y=pull(c);nz(c,c->Y,16);break;
  case 0x9416:cmp(c,c->A,readm(c,abs,0),16);break;
  case 0x941e:next=0x9423;break;
  case 0x942b:case 0x9438:case 0x943e:case 0x9444:case 0x944f:case 0x9455:case 0x945b:writem(c,dp,c->A,0);break;
  case 0x942d:c->_flag_C=!!(c->A&128);a(c,(c->A&255)*2);break;
  case 0x942e:case 0x9470:c->_flag_C=0;cpu_mirrors_to_p(c);break;
  case 0x942f:adc(c,read8(c,dp));break;
  case 0x9431:case 0x83b3:c->X=c->A;nz(c,c->X,16);break;
  case 0x9471:adc(c,readm(c,abs,0));break;
  case 0x9474:case 0x947c:case 0x83b7:writem(c,abs,c->A,0);break;
  case 0x947a:adc(c,im);break;
  case 0x9496:write8(c,(abs+c->X)&0xffffff,0);break;
  case 0x83b2:c->A=(uint16_t)((c->A>>8)|(c->A<<8));nz(c,c->A&255,8);break;
  case 0x83ba:c->X=(uint16_t)(c->X-1);nz(c,c->X,16);break;
  case 0x02c5c6:a(c,read8(c,dp));break;
  default:break;
  }
  c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2;
  if(f==11)++services_stats.scene;else if(f>=7)++services_stats.vram;else ++services_stats.persistence;
  ++executed;if(!interp_bridge_commit_source_opcode(c,prior,false))abort();pc=next;
 }
}
#ifndef LAGOON_SERVICES65_TEST
#include "common_rtl.h"
#include <stdio.h>
static uint64_t frame_gen;static int enabled=-1,report,scene_enabled=1,vram_enabled=1;
static void stats_exit(void){if(report)fprintf(stderr,"[native-services65] armed=%llu persistence=%llu scene=%llu vram=%llu deadlines=%llu refused=%llu\n",(unsigned long long)services_stats.armed,(unsigned long long)services_stats.persistence,(unsigned long long)services_stats.scene,(unsigned long long)services_stats.vram,(unsigned long long)services_stats.deadlines,(unsigned long long)services_stats.refused);}
static int mapped(void){for(unsigned j=0;j<sizeof spans/sizeof *spans;j++){unsigned at=offset(spans[j].pc);for(unsigned k=0;k<spans[j].n;k++)if(!cpu_aot_rom_mapping_matches(spans[j].pc+k,at+k,1))return 0;}return 1;}
static void hook(CpuState*c,uint32_t pc){if(!enabled||c!=&g_cpu||!interp_bridge_source_opcode_available(c))return;if(!mapped()){memset(owners,0,sizeof owners);return;}if((family(pc)==11&&!scene_enabled)||(family(pc)==7&&!vram_enabled))return;uint32_t landing;if(LagoonServices65_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),frame_gen,pc,&landing))interp_bridge_pre_opcode_redirect(landing);}
static bool observe(CpuState*c,uint16_t s,uint8_t h,uint32_t host,unsigned pc){
 if(!enabled||c!=&g_cpu||!c->ram||c->S!=s||!mapped())return false;
 unsigned j;for(j=0;j<sizeof roots/sizeof *roots;j++)if(roots[j].pc==pc)break;if(j==sizeof roots/sizeof *roots||h!=roots[j].frame||c->host_return_valid!=h||s<0x120||s>0x1fc)return false;
 unsigned p=c->ram[s+1]|c->ram[s+2]<<8,bank=h==3?c->ram[s+3]:c->PB;
 if(((bank<<16)|((p+1)&65535))==host)arm(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),frame_gen,pc);
 return false;
}
#define OBS(name,pc) bool name(CpuState*c,uint16_t s,uint8_t h,uint32_t p,RecompReturn*out){(void)out;return observe(c,s,h,p,pc);}
OBS(LagoonServices65_Save,0x9370) OBS(LagoonServices65_Sign,0x93a2) OBS(LagoonServices65_Check,0x93bb) OBS(LagoonServices65_Read,0x93db) OBS(LagoonServices65_Select,0x942b) OBS(LagoonServices65_Sum,0x9470) OBS(LagoonServices65_Format,0x9480)
OBS(LagoonServices65_Fill0,0x834b) OBS(LagoonServices65_Fill1,0x8365) OBS(LagoonServices65_Fill2,0x837f) OBS(LagoonServices65_Fill3,0x8399)
#undef OBS
void LagoonServices65_BeforeFrame(void){if(enabled<0){const char*v=getenv("LAGOON_TIMING65");enabled=!v||strcmp(v,"0");v=getenv("LAGOON_TIMING65_SCENE");scene_enabled=!v||strcmp(v,"0");v=getenv("LAGOON_TIMING65_VRAM");vram_enabled=!v||strcmp(v,"0");v=getenv("LAGOON_NATIVE_WAIT_REPORT");report=v&&!strcmp(v,"1");if(report)atexit(stats_exit);}frame_gen=RtlStateGeneration();for(unsigned j=0;j<sizeof roots/sizeof *roots;j++)if(owners[j].armed&&owners[j].gen!=frame_gen)owners[j].armed=0;
 if(enabled){for(unsigned j=0;j<sizeof roots/sizeof *roots;j++)interp_bridge_add_pre_opcode_hook(roots[j].pc,hook);for(unsigned j=0;j<sizeof ops/sizeof *ops;j++)interp_bridge_add_pre_opcode_hook(ops[j][0],hook);}}
#endif
