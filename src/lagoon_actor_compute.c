/* Compute the original per-enemy RAM leaves on the host. This is deliberately
 * scoped to exact ordinary room descriptors00..3E and type2 enemies,
 * not an emulator overclock.
 * No AI, collision, tick, DMA, NMI or audio work is skipped or fabricated. */
#include "lagoon_actor_compute.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static unsigned word(const uint8_t*r,unsigned q){return r[q]|r[q+1]<<8;}
static void put(uint8_t*r,unsigned q,unsigned v){r[q]=v;r[q+1]=v>>8;}
static uint64_t hash(const uint8_t*r,unsigned n){uint64_t h=UINT64_C(14695981039346656037);while(n--)h=(h^*r++)*UINT64_C(1099511628211);return h;}
/* Finite original room descriptors, individually qualified. The ending and all
 * boss arenas3F..4A are deliberately absent. $018B84 copies each56-byte record
 * to0050 and derives the dimensions/bounds. Rendering/parallax differences do
 * not alter these RAM-only leaf operations, but an unknown descriptor or a
 * half-loaded header cannot gain authority merely from a room number. */
static const uint64_t ordinary_descriptors[]={
 UINT64_C(0xab072f6fd13163b3), UINT64_C(0xd8f94ebdbf660f91), UINT64_C(0x1b1a825e2f6696da),
 UINT64_C(0x73318a063606bb26), UINT64_C(0xbb41021f56988f94), UINT64_C(0x1db02fd339c9ca48),
 UINT64_C(0xf0132542ff93452a), UINT64_C(0x6e5d4a4e740e40cb), UINT64_C(0x8dbe20321f790213),
 UINT64_C(0x9d942f20dc4cda16), UINT64_C(0x5e70ee6b99a1b47a), UINT64_C(0xfc8f92adfb8c90cf),
 UINT64_C(0xa300e61186d52d51), UINT64_C(0x5cc6e815cf56d08b), UINT64_C(0x2f2b2860c83b27c4),
 UINT64_C(0x60fd281d9456efea), UINT64_C(0xb49fca7372c3e5ad), UINT64_C(0x38e468d4097eea9c),
 UINT64_C(0xfc725f2af602c04c), UINT64_C(0x04a088cde2c21dba), UINT64_C(0x9237b76d09df8ec6),
 UINT64_C(0xa9354cbd696f80e7), UINT64_C(0x420ca4b765d4d2b0), UINT64_C(0xeb370e99e6d582ad),
 UINT64_C(0xe37215220e29452c), UINT64_C(0x15036927b8d2e3b3), UINT64_C(0xb260213eecae466b),
 UINT64_C(0xeb815eff035fe60b), UINT64_C(0x7ec56f03b008355d), UINT64_C(0x9ac36cd122f3b3b5),
 UINT64_C(0x7ca558b6861d4d57), UINT64_C(0x24a915ca1aa33535), UINT64_C(0xc43566fd8a410c20),
 UINT64_C(0x5ea22a04e43d4f6e), UINT64_C(0x124fa9fa2435bf7f), UINT64_C(0x783d8d0dd3fd1de1),
 UINT64_C(0xacdc940ef7d94351), UINT64_C(0xd9c99173e30c3d6c), UINT64_C(0xa080e656d19ec03e),
 UINT64_C(0xc04879fda693a720), UINT64_C(0xd4f8d19b453b06bf), UINT64_C(0x15ae330094d51bf3),
 UINT64_C(0x5f8513711df52250), UINT64_C(0x2abab2f8555c9ae6), UINT64_C(0x0d28f3aa2ce8faba),
 UINT64_C(0x44186013c676f90c), UINT64_C(0xc932b072d89dd496), UINT64_C(0x85493c443d7c5ff1),
 UINT64_C(0xb9c3717e12b479b8), UINT64_C(0x532ffc299c948c92), UINT64_C(0xc5ba309db1581195),
 UINT64_C(0x22638fbfee5f33f3), UINT64_C(0x56c0ccc64b268de5), UINT64_C(0xa1e4bc836f3a658e),
 UINT64_C(0x0f355ec155f9f967), UINT64_C(0x42f6d8a621488ca3), UINT64_C(0x64dfc81a6f6ca562),
 UINT64_C(0x34da177f7637340a), UINT64_C(0xe941a0a65c68e2fd), UINT64_C(0x599a87556ddf9050),
 UINT64_C(0x143df24dbe04369d), UINT64_C(0x0d656e2f3353f3b7), UINT64_C(0x9c168e849cf3eb85)
};
static int ordinary_scene(const uint8_t*rom,const uint8_t*r){
 unsigned room=word(r,0x48e);if(room>=sizeof ordinary_descriptors/sizeof *ordinary_descriptors)return 0;
 const uint8_t*d=rom+0x2b2df+room*56;
 if(hash(d,56)!=ordinary_descriptors[room]||hash(rom+0x8b84,81)!=UINT64_C(0xe58d1f2c1e447da1))return 0;
 static const uint8_t fields[]={0x50,0x53,0x54,0x55,0x56,0x6c,0x6d,0x6e,0x78,0x79,0x7a};
 for(unsigned i=0;i<sizeof fields;i++)if(r[fields[i]]!=d[fields[i]-0x50])return 0;
 unsigned width=word(r,0x53)*8,height=word(r,0x55)*8;
 return width&&width<=2048&&height&&height<=2048&&word(r,0x492)==width-16&&word(r,0x494)==height-14&&
   word(r,0x88)==0&&r[0x8a]==0x7f&&word(r,0x8b)==0x4000&&r[0x8d]==0x7f;
}
static void flag(CpuState*c,unsigned bit,unsigned value){c->P=(c->P&~bit)|(value?bit:0);}
static int valid(const CpuState*c){return c&&c->m_flag&&!c->x_flag&&!c->emulation&&!c->D&&!c->_flag_D&&!(c->P&8)&&c->PB==1&&c->DB==1&&c->S>=0x104&&c->S<=0x1fb&&c->X<=0x800&&!(c->X&63);}
int LagoonActorCompute_Apply(unsigned kind,uint8_t*r,CpuState*c){
 if(kind>1||!r||!valid(c))return 0;
 unsigned x=c->X,a=word(r,0x502+x),b=word(r,0x504+x),result;
 if(!kind){
  if(a<16)a=16;else if(a>=word(r,0x492))a=word(r,0x492);
  put(r,0x502+x,a);
  unsigned carry=b>=16;if(b<16)b=16;else{carry=b>=word(r,0x494);if(carry)b=word(r,0x494);}
  put(r,0x504+x,b);result=b&255;c->_flag_C=carry;flag(c,1,carry);
 }else{
  unsigned cx=word(r,0x480),cy=word(r,0x482),dx=(a-cx)&65535,dy=(b-cy)&65535;
  put(r,0x50c+x,dx);put(r,0x50e + x,dy);
  /* Original PHA16 order and all subsequent PLA/PHA pairs leave these exact
   * four dead stack bytes, despite ending with the original S. */
  put(r,c->S-1,dx);put(r,c->S-3,dy);
  unsigned bits=word(r,0xa4);
  if((dy-0x120)&0x8000)bits|=4;
  if((dy-0xb0)&0x8000)bits|=0x400;
  if(!((dy-0xffe0)&0x8000))bits|=8;
  if(!((dy-0x50)&0x8000))bits|=0x800;
  if((dx-0x160)&0x8000)bits|=1;
  if((dx-0xc0)&0x8000)bits|=0x100;
  if(!((dx-0xffa0)&0x8000))bits|=2;
  unsigned final=!((dx-0x40)&0x8000);if(final)bits|=0x200;
  put(r,0xa4,bits);result=(final?0x200:dx)&255;
  c->_flag_C=dx>=0x40;flag(c,1,c->_flag_C);
  c->_flag_V=!!((b^cy)&(b^dy)&0x8000);flag(c,0x40,c->_flag_V);
 }
 c->A=result;c->_flag_N=0;c->_flag_Z=!result;flag(c,0x80,0);flag(c,2,!result);return 1;
}
int LagoonActorCompute_IntegrateApply(uint8_t*r,CpuState*c){
 if(!r||!valid(c)||(r[0x501+c->X]&0x20))return 0;
 unsigned x=c->X,a=word(r,0x504+x),b=word(r,0x508+x),sum=a+b;
 put(r,0x502+x,word(r,0x502+x)+word(r,0x506+x));put(r,0x504+x,sum);
 c->A=0;c->_flag_C=sum>65535;c->_flag_V=!!((~(a^b)&(a^sum))&0x8000);c->_flag_N=0;c->_flag_Z=1;
 flag(c,1,c->_flag_C);flag(c,0x40,c->_flag_V);flag(c,0x80,0);flag(c,2,1);return 1;
}
bool LagoonActorCompute_Run(unsigned kind,const uint8_t*rom,size_t size,uint8_t*r,CpuState*c,uint16_t entry_s,uint8_t hrv,uint32_t host,RecompReturn*out,int normal_world){
 if(kind>2||!rom||size!=0x100000||!r||!out||!valid(c)||!normal_world||c->S!=entry_s||hrv!=2||c->host_return_valid!=2||c->X<0x40)return false;
 if(!ordinary_scene(rom,r))return false;
 unsigned s=c->S,ret=kind==2?0xa4d5:kind?0xa4e7:0xa4d8;
 if(host!=(0x010000|((ret+1)&65535))||word(r,s+1)!=ret||word(r,s+3)!=0x80c6||r[0x406]||r[0x100e]||r[0x23d]!=15||r[0x275]!=0x81||!r[0x520]||r[0x521]||(r[0x500+c->X]&0xc0)!=0xc0||r[0x518+c->X]!=2)return false;
 if(hash(rom+0xa4d3,0x18)!=UINT64_C(0x09abeb222846112b))return false;
 if(kind==2){if(hash(rom+0xa4eb,0x5a)!=UINT64_C(0x574af2c738971269)||!LagoonActorCompute_IntegrateApply(r,c))return false;}
 else if(hash(rom+(kind?0xa5aa:0xa56a),kind?0x7a:0x40)!=(kind?UINT64_C(0x55f5e93f5e4ec5f7):UINT64_C(0x8d84ee94fd1f84c5))||!LagoonActorCompute_Apply(kind,r,c))return false;
 /* Preserve the selected precise leaf's actual RTS: six CPU cycles, opcode
  * fetch plus two slow-WRAM reads =42 master clocks. Only pure computation
  * moves to the host; no elapsed-clock subtraction or guest-frame synthesis. */
 c->coprocessor_master_cycles=c->master_cycles;c->cycles+=6;c->master_cycles+=42;
 cpu_read8(c,0,++c->S);cpu_read8(c,0,++c->S);*out=RECOMP_RETURN_NORMAL;return true;
}
/* $8385 samples the two current metatile maps and their quadrant definitions.
 * Preflight every byte before writing anything. Only immutable ROM or full high
 * WRAM can be read: low-WRAM aliases, IO and writes overlapping lookup tables
 * must remain original. In particular this cannot bypass an IO read or alias
 * its PHY/PHA/dead-stack writes back into its own table input. */
static int sample_byte(const uint8_t*rom,size_t size,const uint8_t*r,unsigned a,unsigned*v){
 a&=0xffffff;unsigned bank=a>>16,lo=a&65535;
 if(bank==0x7e||bank==0x7f){unsigned offset=(bank-0x7e)*65536+lo;if(offset<0x2000)return 0;*v=r[offset];return 1;}
 bank&=127;
 if(bank<32&&lo>=32768){size_t offset=bank*32768+(lo&32767);if(offset<size){*v=rom[offset];return 1;}}
 return 0;
}
static int sample_word(const uint8_t*rom,size_t size,const uint8_t*r,unsigned a,unsigned*v){
 unsigned lo,hi;if(!sample_byte(rom,size,r,a,&lo)||!sample_byte(rom,size,r,a+1,&hi))return 0;*v=lo|(hi<<8);return 1;
}
static unsigned sample_pointer(const uint8_t*r,unsigned a){return word(r,a)|r[a+2]<<16;}
int LagoonActorCompute_SampleApply(const uint8_t*rom,size_t size,uint8_t*r,CpuState*c){
 if(!rom||size!=0x100000||!r||!valid(c)||c->S<0x106)return 0;
 unsigned x=word(r,0xb8),y=word(r,0xba),base=((y&0xfff0)<<3)&65535;
 unsigned index=((x>>4)+base)&65535,quadrant=((x&8)>>3)|((y&8)>>2),m0,m1,a0,a1;
 if(!sample_word(rom,size,r,sample_pointer(r,0x88)+index,&m0)||
    !sample_word(rom,size,r,sample_pointer(r,0x8b)+index,&m1)||
    !sample_word(rom,size,r,sample_pointer(r,0x78)+((m1&255)<<2)+quadrant,&a1)||
    !sample_word(rom,size,r,sample_pointer(r,0x6c)+((m0&255)<<2)+quadrant,&a0))return 0;
 /* PHY16 then PHA16 of both map words. The pops leave all six exact bytes. */
 put(r,c->S-1,c->Y);put(r,c->S-3,m0);put(r,c->S-5,m1);
 put(r,0xb4,quadrant);put(r,0x2c1,a1);r[0x2c0]=(uint8_t)a0;c->A=a0&255;
 c->_flag_C=0;flag(c,1,0);
 c->_flag_V=!!((~((x>>4)^base)&((x>>4)^index))&0x8000);flag(c,0x40,c->_flag_V);
 c->_flag_N=!!(c->Y&0x8000);c->_flag_Z=!c->Y;flag(c,0x80,c->_flag_N);flag(c,2,c->_flag_Z);
 return 1;
}
bool LagoonActorCompute_SampleRun(const uint8_t*rom,size_t size,uint8_t*r,CpuState*c,uint16_t entry_s,uint8_t hrv,uint32_t host,RecompReturn*out,int normal_world){
 if(!rom||size!=0x100000||!r||!out||!valid(c)||!normal_world||c->S!=entry_s||c->S<0x106||hrv!=2||c->host_return_valid!=2||c->X<0x40)return false;
 unsigned s=c->S,ret=word(r,s+1);
 if(!ordinary_scene(rom,r)||
    (ret!=0x9e28&&ret!=0x9eac&&ret!=0x9f30&&ret!=0x9fb4)||
    host!=(0x010000|((ret+1)&65535))||word(r,s+3)!=0x80c3||
    r[0x406]||r[0x100e]||r[0x23d]!=15||r[0x275]!=0x81||!r[0x520]||r[0x521]||
    (r[0x500+c->X]&0xc0)!=0xc0||r[0x518+c->X]!=2)return false;
 if(hash(rom+0x8385,83)!=UINT64_C(0xaa6815d0b23960aa)||
    hash(rom+0x9bce,1100)!=UINT64_C(0xc463c68b4606f575)||
    memcmp(rom+0x80c1,"\x20\xce\x9b",3))return false;
 if(!LagoonActorCompute_SampleApply(rom,size,r,c))return false;
 c->coprocessor_master_cycles=c->master_cycles;c->cycles+=6;c->master_cycles+=42;
 cpu_read8(c,0,++c->S);cpu_read8(c,0,++c->S);*out=RECOMP_RETURN_NORMAL;return true;
}
/* Close the complete RAM-only branch before the original render-queue IO.
 * Offscreen does not mean idle: original AI, tile/body collision, animation and
 * lifetime services outside this wrapper still execute. Only the same movement,
 * clamp, projection and no-queue branch can move to the host. */
int LagoonActorCompute_OffscreenApply(uint8_t*r,CpuState*c){
 if(!r||!valid(c)||c->S<0x106||(r[0x501+c->X]&0x20))return 0;
 unsigned x=c->X,a=(word(r,0x502+x)+word(r,0x506+x))&65535,b=(word(r,0x504+x)+word(r,0x508+x))&65535;
 if(a<16)a=16;else if(a>=word(r,0x492))a=word(r,0x492);
 if(b<16)b=16;else if(b>=word(r,0x494))b=word(r,0x494);
 unsigned dx=(a-word(r,0x480))&65535,dy=(b-word(r,0x482))&65535,bits=0;
 if((dy-0x120)&0x8000)bits|=4;if(!((dy-0xffe0)&0x8000))bits|=8;
 if((dx-0x160)&0x8000)bits|=1;if(!((dx-0xffa0)&0x8000))bits|=2;
 if(bits==15)return 0; /* The original next branch would touch4202/4216. */
 unsigned s=c->S;c->S=s-2;
 put(r,s-1,0xa4d5);(void)LagoonActorCompute_IntegrateApply(r,c);
 put(r,s-1,0xa4d8);(void)LagoonActorCompute_Apply(0,r,c);
 r[0x500+x]&=0xe7;r[0xa4]=r[0xa5]=0;
 put(r,s-1,0xa4e7);(void)LagoonActorCompute_Apply(1,r,c);c->S=s;
 c->A=r[0xa4];c->_flag_C=c->A>=15;c->_flag_N=!!((c->A-15)&128);c->_flag_Z=c->A==15;
 flag(c,1,c->_flag_C);flag(c,0x80,c->_flag_N);flag(c,2,c->_flag_Z);return 1;
}
bool LagoonActorCompute_OffscreenRun(const uint8_t*rom,size_t size,uint8_t*r,CpuState*c,uint16_t entry_s,uint8_t hrv,uint32_t host,RecompReturn*out,int normal_world){
 if(!rom||size!=0x100000||!r||!out||!valid(c)||!normal_world||c->S!=entry_s||c->S<0x106||hrv!=2||c->host_return_valid!=2||c->X<0x40)return false;
 unsigned s=c->S;
 if(host!=0x0180c7||word(r,s+1)!=0x80c6||!ordinary_scene(rom,r)||r[0x406]||r[0x100e]||r[0x23d]!=15||r[0x275]!=0x81||!r[0x520]||r[0x521]||
    (r[0x500+c->X]&0xc0)!=0xc0||r[0x518+c->X]!=2||
    hash(rom+0xa4d3,0x196)!=UINT64_C(0x247eeb84f5ee7b57)||memcmp(rom+0x80c4,"\x20\xd3\xa4",3))return false;
 if(!LagoonActorCompute_OffscreenApply(r,c))return false;
 c->coprocessor_master_cycles=c->master_cycles;c->cycles+=6;c->master_cycles+=42;
 cpu_read8(c,0,++c->S);cpu_read8(c,0,++c->S);*out=RECOMP_RETURN_NORMAL;return true;
}
/* Original $008211 OAM cell packer. Its enclosing stream decoder and the
 * real render-queue multiplier remain original. Only the RAM cell write and
 * high-bit packing move to the host, with the complete ordinary-enemy draw
 * stack retained as ownership evidence. */
static int cell_valid(const CpuState*c){
 if(!c||c->PB||c->A>255||c->S<0x106||c->S>0x1f0)return 0;
 CpuState t=*c;t.PB=1;t.X=0;return valid(&t);
}
int LagoonActorCompute_CellApply(uint8_t*r,CpuState*c){
 if(!r||!cell_valid(c))return 0;
 unsigned n=r[0x236];
 if(n>=128){c->A=n;c->_flag_N=1;c->_flag_Z=0;flag(c,0x80,1);flag(c,2,0);return 1;}
 put(r,0x2000+n*4,word(r,0x28));put(r,0x2001+n*4,word(r,0x2a));put(r,0x2002+n*4,word(r,0x2c));
 unsigned slot=n&3,ret=slot==0?0x8249:slot==1?0x8257:slot==2?0x8267:0x8279;
 put(r,c->S-1,ret);r[0x29]&=1;
 unsigned a=((r[0x2e]<<1)&255)|r[0x29],carry=r[0x2e]>>7;
 if(slot<3){for(unsigned k=0;k<slot*2;k++){carry=a>>7;a=(a<<1)&255;}}
 else for(unsigned k=0;k<3;k++){unsigned next=a&1;a=(a>>1)|(carry<<7);carry=next;}
 r[0x26d]=a;c->X=n>>2;
 static const unsigned masks[]={0xfc,0xf3,0xcf,0x3f};
 c->A=r[0x2200+c->X]=(r[0x2200+c->X]&masks[slot])|a;
 r[0x236]=n+1;c->_flag_C=carry;c->_flag_N=!!((n+1)&128);c->_flag_Z=0;
 flag(c,1,carry);flag(c,0x80,c->_flag_N);flag(c,2,0);return 1;
}
bool LagoonActorCompute_CellRun(const uint8_t*rom,size_t size,uint8_t*r,CpuState*c,uint16_t entry_s,uint8_t hrv,uint32_t host,RecompReturn*out,int normal_world){
 if(!rom||size!=0x100000||!r||!out||!cell_valid(c)||!normal_world||c->S!=entry_s||hrv!=2||c->host_return_valid!=2||host!=0x008bf5)return false;
 unsigned s=c->S,x=word(r,s+11),outer=word(r,s+13),draw=word(r,s+7);
 if(word(r,s+1)!=0x8bf4||(draw!=0x8bcc&&draw!=0x8bb2)||(outer!=0xc282&&outer!=0xc254)||r[s+15]!=1||x<0x40||x>0x800||(x&63)||
    !ordinary_scene(rom,r)||r[0x406]||r[0x100e]||r[0x23d]!=15||r[0x275]!=0x81||!r[0x520]||r[0x521]||(r[0x500+x]&0xc0)!=0xc0||r[0x518+x]!=2||
    hash(rom+0x211,138)!=UINT64_C(0x1b15a374826ab834)||hash(rom+0xb71,293)!=UINT64_C(0x8bf2496a513f478d)||hash(rom+0xc214,112)!=UINT64_C(0xd94970056e0ec0f2))return false;
 if(!LagoonActorCompute_CellApply(r,c))return false;
 c->coprocessor_master_cycles=c->master_cycles;c->cycles+=6;c->master_cycles+=42;
 cpu_read8(c,0,++c->S);cpu_read8(c,0,++c->S);*out=RECOMP_RETURN_NORMAL;return true;
}
#ifndef LAGOON_ACTOR_COMPUTE_TEST
#include "common_rtl.h"
#include "snes/ppu.h"
static uint64_t calls[6];static int mode=-1,report;
static void summary(void){if(report)fprintf(stderr,"[actor-compute] clamp=%llu project=%llu sample=%llu integrate=%llu offscreen=%llu cells=%llu scope=source-qualified-rooms00-3E-enemy2 enabled=%d\n",(unsigned long long)calls[0],(unsigned long long)calls[1],(unsigned long long)calls[2],(unsigned long long)calls[3],(unsigned long long)calls[4],(unsigned long long)calls[5],mode);}
static void configure(void){
 if(mode<0){const char*v=getenv("LAGOON_ACTOR_COMPUTE");mode=!v||!strcmp(v,"auto")||!strcmp(v,"on");const char*t=getenv("LAGOON_ACTOR_COMPUTE_REPORT");report=t&&!strcmp(t,"1");atexit(summary);}
}
static bool run(unsigned kind,CpuState*c,uint16_t s,uint8_t h,uint32_t pc,RecompReturn*out){
 configure();
 if(!mode||c!=&g_cpu||!g_ppu)return false;
 bool ok=LagoonActorCompute_Run(kind,g_rom,0x100000,g_ram,c,s,h,pc,out,!PPU_forcedBlank(g_ppu));if(ok)calls[kind==2?3:kind]++;return ok;
}
bool LagoonActorCompute_Clamp(CpuState*c,uint16_t s,uint8_t h,uint32_t pc,RecompReturn*out){return run(0,c,s,h,pc,out);}
bool LagoonActorCompute_Project(CpuState*c,uint16_t s,uint8_t h,uint32_t pc,RecompReturn*out){return run(1,c,s,h,pc,out);}
bool LagoonActorCompute_Integrate(CpuState*c,uint16_t s,uint8_t h,uint32_t pc,RecompReturn*out){return run(2,c,s,h,pc,out);}
bool LagoonActorCompute_Sample(CpuState*c,uint16_t s,uint8_t h,uint32_t pc,RecompReturn*out){
 configure();if(!mode||c!=&g_cpu||!g_ppu)return false;
 bool ok=LagoonActorCompute_SampleRun(g_rom,0x100000,g_ram,c,s,h,pc,out,!PPU_forcedBlank(g_ppu));if(ok)calls[2]++;return ok;
}
bool LagoonActorCompute_Offscreen(CpuState*c,uint16_t s,uint8_t h,uint32_t pc,RecompReturn*out){
 configure();if(!mode||c!=&g_cpu||!g_ppu)return false;
 bool ok=LagoonActorCompute_OffscreenRun(g_rom,0x100000,g_ram,c,s,h,pc,out,!PPU_forcedBlank(g_ppu));if(ok)calls[4]++;return ok;
}
bool LagoonActorCompute_Cell(CpuState*c,uint16_t s,uint8_t h,uint32_t pc,RecompReturn*out){
 configure();if(!mode||c!=&g_cpu||!g_ppu)return false;
 bool ok=LagoonActorCompute_CellRun(g_rom,0x100000,g_ram,c,s,h,pc,out,!PPU_forcedBlank(g_ppu));if(ok)calls[5]++;return ok;
}
#endif
