/* Original $00:9143 dictionary expansion, expressed as bounded host operations.
 * No timing, input, interrupt, decoder yield or DMA is synthesized here. The
 * guarded menu caller continues through the original upload queue afterward. */
#include "lagoon_menu_decode.h"
#include <stdlib.h>
#include <string.h>
static void nz(CpuState *c,unsigned v,int wide) {
  unsigned mask=wide?65535:255,sign=wide?32768:128;
  c->_flag_Z=!(v&mask);c->_flag_N=!!(v&sign);
  c->P=(c->P&~0x82)|(c->_flag_Z?2:0)|(c->_flag_N?128:0);
}
static void a8(CpuState*c,unsigned v){c->A=(c->A&0xff00)|(v&255);nz(c,v,0);}
static void x16(CpuState*c,unsigned v){c->X=v;nz(c,v,1);}
static void y16(CpuState*c,unsigned v){c->Y=v;nz(c,v,1);}
static unsigned word(const uint8_t*r,unsigned q){return r[q]|r[q+1]<<8;}
static void putword(uint8_t*r,unsigned q,unsigned v){r[q]=v;r[q+1]=v>>8;}
static unsigned checked_read(const uint8_t*r,unsigned q,int *bad){if(q>=0x20000){*bad=1;return 0;}return r[q];}
static int expand(const uint8_t *rom,size_t size,uint8_t*r,CpuState*c,
    unsigned *consumed,unsigned *produced) {
  int bad=0;unsigned steps=0,entry_s=c->S,base=r[0xe0]|r[0xe1]<<8|r[0xe2]<<16;
  if(!rom||!r||c->S<0x120||c->S>0x1f3||!c->m_flag||c->x_flag||c->emulation)return 0;
#define TICK() do{if(bad||++steps>1000000)return 0;}while(0)
#define R(q) checked_read(r,(q),&bad)
#define W(q,v) do{unsigned _q=(q);if(_q>=0x20000)return 0;r[_q]=(uint8_t)(v);}while(0)
#define PUSH(v) do{if(c->S<0x108)return 0;putword(r,c->S-1,(v));c->S-=2;}while(0)
#define POPX() do{if(c->S+2>entry_s)return 0;x16(c,word(r,c->S+1));c->S+=2;}while(0)
#define SOURCE() do{unsigned _a=base+c->Y,_b=(_a>>16)&255,_off=_a&65535;size_t _q=((_b&31)<<15)|(_off&32767);if((_b&127)>=32||_off<32768||_q>=size)return 0;a8(c,rom[_q]);}while(0)
#define INCY() y16(c,(uint16_t)(c->Y+1))
#define INCX() x16(c,(uint16_t)(c->X+1))
#define DECX() x16(c,(uint16_t)(c->X-1))
#define LDX(q) x16(c,word(r,(q)))
#define STX(q) putword(r,(q),c->X)
#define CPX(q) do{unsigned _v=word(r,(q));c->_flag_C=c->X>=_v;c->P=(c->P&~1)|c->_flag_C;nz(c,(uint16_t)(c->X-_v),1);}while(0)
  PUSH(c->X);PUSH(c->Y);
  c->m_flag=0;c->P&=~0x20;c->A=0;nz(c,0,1);c->D=0;PUSH(0);
  c->x_flag=0;c->P&=~0x10;c->m_flag=1;c->P|=0x20;
  /* Original PHA/XBA/PLA clears B and leaves one dead zero byte on stack. */
  W(c->S,0);c->A=0;nz(c,0,0);x16(c,0);STX(0xf9);y16(c,8);
L915D:
  TICK();x16(c,0x101);a8(c,0);
  do{W(0x12110+c->X,c->A);DECX();}while(!c->_flag_N);
  SOURCE();W(0xe3,c->A);INCY();SOURCE();W(0xe5,c->A);INCY();
  SOURCE();unsigned low=c->A&255;INCY();SOURCE();unsigned hi=c->A&255;
  c->A=low|(hi<<8);nz(c,c->A,1);putword(r,0xe7,c->A);INCY();
  W(c->S,c->A); /* Original temporary PHA8/XBA/PLA8 leaves this dead byte. */
  c->A&=255;nz(c,c->A,0);LDX(0xe7);INCX();STX(0xf7);LDX(0xe3);STX(0xf5);
  if(c->_flag_Z)goto L92E5;
  x16(c,1);
  do{SOURCE();W(0x12000+c->X,c->A);INCX();INCY();W(0xf5,R(0xf5)-1);nz(c,R(0xf5),0);}while(!c->_flag_Z);
  a8(c,R(0xe3));W(0xf5,c->A);x16(c,1);
  do{SOURCE();W(0x12330+c->X,c->A);INCX();INCY();W(0xf5,R(0xf5)-1);nz(c,R(0xf5),0);}while(!c->_flag_Z);
  a8(c,R(0xe3));W(0xf5,c->A);x16(c,1);
  do{SOURCE();W(0x12440+c->X,c->A);INCX();INCY();W(0xf5,R(0xf5)-1);nz(c,R(0xf5),0);}while(!c->_flag_Z);
  LDX(0xe3);DECX();STX(0xf5);x16(c,1);STX(0xe9);
  W(0xeb,0);W(0xec,0);W(0xed,0);W(0xee,0);
L91D7:
  TICK();LDX(0xe9);a8(c,R(0x12000+c->X));W(0xeb,c->A);x16(c,c->A);
  a8(c,R(0x12110+c->X));W(0xf7,c->A);LDX(0xe9);W(0x12220+c->X,c->A);
  a8(c,c->X);LDX(0xeb);W(0x12110+c->X,c->A);LDX(0xe9);INCX();STX(0xe9);
  LDX(0xf5);DECX();STX(0xf5);if(!c->_flag_N)goto L91D7;
  LDX(0xe7);INCX();STX(0xf7);x16(c,1);STX(0xf5);W(0xe9,0);W(0xea,0);
L920D:
  TICK();LDX(0xf7);DECX();STX(0xf7);if(!c->_flag_Z)goto L921E;
L9214:
  a8(c,R(0xe5));if(c->_flag_Z)goto L9300;goto L915D;
L921E:
  SOURCE();INCY();W(0xeb,c->A);W(0xe9,c->A);LDX(0xe9);
  a8(c,R(0x12110+c->X));W(0xf3,c->A);if(!c->_flag_Z)goto L923B;
  a8(c,c->X);LDX(0xf9);if(c->X>=0x6000)return 0;W(0x17f7c+c->X,c->A);INCX();STX(0xf9);goto L920D;
L923B:
  LDX(0xe9);a8(c,R(0x12110+c->X));W(0xe9,c->A);POPX();INCX();a8(c,0);
  W(0x12800+c->X,c->A);INCX();W(0x12800+c->X,c->A);INCX();W(0x12800+c->X,c->A);INCX();W(0x12800+c->X,c->A);PUSH(c->X);goto L9297;
L925D:
  TICK();a8(c,R(0xeb));W(0xed,c->A);x16(c,c->A);a8(c,R(0x12110+c->X));if(c->_flag_Z)goto L92BC;
  x16(c,c->A);CPX(0xe9);if(c->_flag_N)goto L928F;
  a8(c,R(0xe9));W(0xeb,c->A);LDX(0xed);a8(c,R(0x12110+c->X));W(0xe9,c->A);
L927C:
  TICK();LDX(0xe9);a8(c,R(0x12220+c->X));W(0xe9,c->A);if(c->_flag_Z)goto L92B8;
  x16(c,c->A);CPX(0xeb);if(c->_flag_N||c->_flag_Z)goto L9297;goto L927C;
L928F:
  LDX(0xed);a8(c,R(0x12110+c->X));W(0xe9,c->A);
L9297:
  LDX(0xe9);a8(c,R(0x12440+c->X));W(0xed,c->A);a8(c,c->X);POPX();INCX();
  if(c->X>=0x5000)return 0;W(0x12800+c->X,c->A);INCX();a8(c,R(0xed));W(0x12800+c->X,c->A);PUSH(c->X);
  LDX(0xe9);a8(c,R(0x12330+c->X));W(0xeb,c->A);goto L925D;
L92B8:
  a8(c,R(0xed));W(0xeb,c->A);
L92BC:
  a8(c,R(0xeb));LDX(0xf9);if(c->X>=0x6000)return 0;W(0x17f7c+c->X,c->A);INCX();STX(0xf9);
  POPX();a8(c,R(0x12800+c->X));W(0xed,c->A);DECX();a8(c,R(0x12800+c->X));W(0xe9,c->A);
  if(c->_flag_Z)goto L92E0;
  DECX();a8(c,R(0xed));W(0xeb,c->A);PUSH(c->X);goto L925D;
L92E0:
  DECX();PUSH(c->X);goto L920D;
L92E5:
  LDX(0xe7);DECX();STX(0xf7);
L92EA:
  TICK();SOURCE();INCY();LDX(0xf9);if(c->X>=0x6000)return 0;W(0x17f7c+c->X,c->A);INCX();STX(0xf9);LDX(0xf7);DECX();STX(0xf7);if(!c->_flag_N)goto L92EA;goto L9214;
L9300:
  if(consumed)*consumed=c->Y;if(produced)*produced=word(r,0xf9);
  POPX();y16(c,word(r,c->S+1));c->S+=2;POPX();
  return !bad && c->S==entry_s;
#undef TICK
#undef R
#undef W
#undef PUSH
#undef POPX
#undef SOURCE
#undef INCY
#undef INCX
#undef DECX
#undef LDX
#undef STX
#undef CPX
}
int LagoonMenuDecode_Expand(const uint8_t *rom,size_t size,uint8_t *ram,CpuState *cpu,unsigned *consumed,unsigned *produced) {
  if(!ram||!cpu)return 0;
  uint8_t *work=malloc(0x20000);if(!work)return 0;memcpy(work,ram,0x20000);CpuState trial=*cpu;
  unsigned used=0,n=0;
  int ok=expand(rom,size,work,&trial,&used,&n);
  if(ok){memcpy(ram,work,0x20000);*cpu=trial;if(consumed)*consumed=used;if(produced)*produced=n;}
  free(work);return ok;
}

#ifndef LAGOON_MENU_DECODE_TEST
#include "common_rtl.h"
static uint64_t hash(const uint8_t*p,size_t n){uint64_t h=UINT64_C(14695981039346656037);while(n--)h=(h^*p++)*UINT64_C(1099511628211);return h;}
static uint32_t triple(const uint8_t*r,unsigned q){return r[q]|r[q+1]<<8|r[q+2]<<16;}
/* Called only at the original native entry after its deadline/refresh seam.
 * Refusal is read-only. The original native decoder remains the fallback. */
bool LagoonMenuDecode_TryNative(CpuState*cpu,uint16_t entry_s,uint8_t hrv,uint32_t host_return_pc24,RecompReturn*out) {
  if(!cpu||!out||cpu->S!=entry_s||hrv!=3||cpu->host_return_valid!=3||
      entry_s<0x140||entry_s>0x1ec||!cpu->m_flag||cpu->x_flag||cpu->emulation||cpu->D||cpu->PB||
      !(g_ram[0x23d]&0x80)||g_ram[0x275]!=0x81||g_ram[0x100e]||g_ram[0xe4]||
      hash(g_rom+0x1143,0x1c1)!=UINT64_C(0x1ab145d7fd2e3ea6))return false;
  unsigned s=entry_s,ret=triple(g_ram,s+1),source=triple(g_ram,0xe0);
  uint32_t actual_return=(ret&0xff0000)|((ret+1)&0xffff);
  if(actual_return!=host_return_pc24)return false;
  static const struct{unsigned id,source,destination,length,consumed,produced;uint64_t digest;} cases[]={
    {6,0x10E9C3,0x7000,0x800,448,2052,UINT64_C(0xd98018bf64e0ae5e)},
    {7,0x10C5AE,0x0000,0xA00,9237,14084,UINT64_C(0x267092c7eb7add5b)},
    {8,0x10C45A,0x6E00,0x270,340,628,UINT64_C(0x9fe7c30d7f251578)}};
  unsigned expected_used=0,expected_output=0;
  if(ret==0x0094D6) {
    unsigned caller=triple(g_ram,s+6),parent=word(g_ram,s+9);
    for(unsigned i=0;i<3;i++) {
      unsigned id=cases[i].id,q=0x14e6+id*7;
      int context=id==6 ? caller==0x02BFD5&&(parent==0xB6CB||parent==0xB7F5||parent==0xBC56) :
        caller==(id==7?0x02BF76:0x02BF98)&&(parent==0xB6C2||parent==0xB7EC||parent==0xBC4D);
      size_t off=((source>>16)&31)*32768+(source&32767);
      if(source==cases[i].source&&context&&triple(g_rom,q)==source&&
          word(g_ram,0x28a)==cases[i].destination&&word(g_ram,0x28c)==cases[i].length&&
          hash(g_rom+off,cases[i].consumed)==cases[i].digest) {
        expected_used=cases[i].consumed;expected_output=cases[i].produced;break;
      }
    }
  }
  /* Original world-loader JSR/JSL frames must lead back to the submenu close,
   * rather than an unrelated room transition through this shared loader. */
  if(!expected_used&&(ret==0x018874||ret==0x018889)&&
      word(g_ram,s+4)==0x87d5&&word(g_ram,s+6)==0x8732&&
      triple(g_ram,s+8)==0x0080e3&&g_ram[s+11]==2) {
    unsigned menu=triple(g_ram,s+12);
    /* Metadata only: original streams observed in the73-room campaign.
     * All70 were independently compared against the original decoder. */
    static const struct{unsigned source,consumed,produced;uint64_t digest;} worlds[]={
      {0x128000,2935,3812,UINT64_C(0x6741aeb6dd87aabc)},
      {0x128b77,4882,5700,UINT64_C(0x89ad4ed2a62f53c4)},
      {0x129e89,5134,5732,UINT64_C(0x5485c194ffbcf85c)},
      {0x12b297,10197,11748,UINT64_C(0x6e89ef854007fce9)},
      {0x12da6c,3368,3972,UINT64_C(0x9aaefb22bf4e6d73)},
      {0x12e794,1662,1828,UINT64_C(0x37035e058cab6c88)},
      {0x12ee12,4215,4964,UINT64_C(0xb004c4613d934998)},
      {0x138000,6778,8420,UINT64_C(0xee5eca56428db0f9)},
      {0x139a7a,2471,3556,UINT64_C(0x9ec1a4a0c37c1ef0)},
      {0x13a421,4412,4932,UINT64_C(0x09c8f10bc0671f23)},
      {0x13b55d,4537,4964,UINT64_C(0xa33f60072aa2368b)},
      {0x13c716,4143,4996,UINT64_C(0xb47a5fc498de5922)},
      {0x13d745,6332,8324,UINT64_C(0x81f619eaff1ad3c7)},
      {0x13f001,1586,1732,UINT64_C(0x79fda87156f80828)},
      {0x148000,4503,5892,UINT64_C(0xc3e7673e5087aab9)},
      {0x149197,4133,4708,UINT64_C(0x90853dd3ceafba36)},
      {0x14a1bc,2562,2884,UINT64_C(0xf75f5dc0b72ff004)},
      {0x14abbe,5283,5732,UINT64_C(0x71e6179ee9de8bf2)},
      {0x14c061,4828,6116,UINT64_C(0x3ad3a1d9290240ff)},
      {0x14d33d,2765,3588,UINT64_C(0xca0c022a0a14d033)},
      {0x14de0a,4043,4740,UINT64_C(0xa2db36975f0af4f4)},
      {0x158000,4493,5476,UINT64_C(0x85a3a1ed2bf77476)},
      {0x15918d,4784,6340,UINT64_C(0xa86cac2c75f1a543)},
      {0x15a43d,5231,6660,UINT64_C(0xdcbe6f2e48891e06)},
      {0x15b8ac,3945,5060,UINT64_C(0xeba2590d12c42485)},
      {0x15c815,4048,4868,UINT64_C(0x81754c878b1b1a7f)},
      {0x15d7e5,4118,5700,UINT64_C(0xf961daf4572040cd)},
      {0x15e7fb,3374,3844,UINT64_C(0xec71c0b16a2a6130)},
      {0x15f529,2746,3844,UINT64_C(0x7230dbc6dffd8ce5)},
      {0x168000,3793,4260,UINT64_C(0xeb627d50a3eb1d2b)},
      {0x168ed1,3621,5252,UINT64_C(0xffd5505fe5f625f0)},
      {0x188000,4580,5924,UINT64_C(0xca59b51c38c1e0e0)},
      {0x1891e4,3790,4676,UINT64_C(0x211fd048116f5093)},
      {0x18a0b2,3838,4964,UINT64_C(0x86ab24b001e91b65)},
      {0x18afb0,5766,7460,UINT64_C(0x2870327a12f90d06)},
      {0x18c636,6769,7908,UINT64_C(0x5b22dd8d7a018e82)},
      {0x18e0a7,8000,10692,UINT64_C(0x73ae961ffb92d004)},
      {0x198000,8044,10052,UINT64_C(0x7571e59545cc6376)},
      {0x199f6c,4068,6020,UINT64_C(0xab5820fd503b78a1)},
      {0x19af50,5863,7428,UINT64_C(0xec79c6b2e782a3db)},
      {0x19c637,6822,8100,UINT64_C(0xdd9329a454e8bb75)},
      {0x19e0dd,7232,10244,UINT64_C(0xe2969360ba02bc27)},
      {0x19fd1d,530,964,UINT64_C(0xae786f36a29a9977)},
      {0x1a8000,4207,5924,UINT64_C(0x4dfb198ee42455f1)},
      {0x1a906f,1167,2052,UINT64_C(0x2013fb7ce848c6b9)},
      {0x1a94fe,6619,8996,UINT64_C(0x16a8065dc87b3650)},
      {0x1aaed9,4180,4804,UINT64_C(0x3a035e940508814e)},
      {0x1abf2d,7306,9412,UINT64_C(0x4975311794c22b8d)},
      {0x1adbb7,4499,5796,UINT64_C(0xa7431b8fac73a584)},
      {0x1aed4a,4602,6852,UINT64_C(0xa19b0152d6705f8e)},
      {0x1b8000,1240,1956,UINT64_C(0xb96f2d56ed51b896)},
      {0x1b84d8,7333,10244,UINT64_C(0xf6f082d7d9f01d75)},
      {0x1ba17d,6149,8548,UINT64_C(0x419c3700424f9841)},
      {0x1bb982,7355,9508,UINT64_C(0x9b8aa6d029630cf8)},
      {0x1bd63d,3630,5060,UINT64_C(0x086e26dada473696)},
      {0x1be46b,7020,10084,UINT64_C(0xe4801f2776bcfbdc)},
      {0x1c8000,4853,5924,UINT64_C(0x1a0eb36e4e250118)},
      {0x1c92f5,6836,8324,UINT64_C(0xf85f490615c7b5be)},
      {0x1cada9,2142,2948,UINT64_C(0xd0b25d11bada3377)},
      {0x1cb607,497,2692,UINT64_C(0x7fd4acd45756b9ec)},
      {0x1cb7f8,2959,6724,UINT64_C(0xfaf794f3db95f7c9)},
      {0x1cc387,4246,6116,UINT64_C(0x758a45180b02dcc2)},
      {0x1cd41d,5796,7012,UINT64_C(0x02b281a879b1bf45)},
      {0x1ceac1,3920,6916,UINT64_C(0xcca169a51faa3567)},
      {0x1cfa11,527,2692,UINT64_C(0xc6b6e4339ac38359)},
      {0x1d8000,7031,8708,UINT64_C(0x139b2ceb7abc6da9)},
      {0x1d9b77,6919,8612,UINT64_C(0x2b9d9f08b3b00846)},
      {0x1db67e,4571,8196,UINT64_C(0xd8c7ef13fce15517)},
      {0x1dc859,3438,7172,UINT64_C(0xc3310960b70d89c8)},
      {0x1dd5c7,3751,5188,UINT64_C(0x0140922d0c7eb2b8)}};
    if(menu==0x02b73a||menu==0x02b87d||menu==0x02bc9b)for(unsigned i=0;i<sizeof(worlds)/sizeof(*worlds);i++) {
      size_t off=((source>>16)&31)*32768+(source&32767);
      if(source==worlds[i].source&&
          triple(g_ram,ret==0x018874?0x57:0x5d)==source&&
          hash(g_rom+off,worlds[i].consumed)==worlds[i].digest) {
        expected_used=worlds[i].consumed;expected_output=worlds[i].produced;
      }
    }
  }
  /* Common world tiles and HUD also reload on submenu close. Authorize only
   * their exact nested original caller chains, never ordinary room loading. */
  if(!expected_used&&ret==0x0094d6) {
    unsigned caller=triple(g_ram,s+6),menu=0,id=99;
    if(caller==0x0097eb&&triple(g_ram,s+9)==0x018727&&
        triple(g_ram,s+12)==0x0080e3&&g_ram[s+15]==2){menu=triple(g_ram,s+16);id=9;}
    if(caller==0x018898&&word(g_ram,s+9)==0x87d5&&word(g_ram,s+11)==0x8732&&
        triple(g_ram,s+13)==0x0080e3&&g_ram[s+16]==2){menu=triple(g_ram,s+17);id=0;}
    if(menu==0x02b73a||menu==0x02b87d||menu==0x02bc9b) {
      unsigned q=0x14e6+id*7;
      unsigned want_source=id==9?0x10bd00:0x11be00;
      unsigned want_used=id==9?1882:168,want_output=id==9?7716:548;
      uint64_t want_hash=id==9?UINT64_C(0xb9968dcd70b4fe72):UINT64_C(0x14bc246e905bf71a);
      size_t off=((source>>16)&31)*32768+(source&32767);
      if(source==want_source&&triple(g_rom,q)==source&&
          word(g_ram,0x28a)==word(g_rom,q+3)&&word(g_ram,0x28c)==word(g_rom,q+5)&&
          hash(g_rom+off,want_used)==want_hash){expected_used=want_used;expected_output=want_output;}
    }
  }
  if(!expected_used)return false;
  uint8_t*work=malloc(0x20000);if(!work)return false;memcpy(work,g_ram,0x20000);CpuState trial=*cpu;
  unsigned used=0,n=0;
  if(!expand(g_rom,0x100000,work,&trial,&used,&n)||used!=expected_used||n!=expected_output||trial.S!=entry_s){free(work);return false;}
  memcpy(g_ram,work,0x20000);free(work);
  cpu->A=trial.A;cpu->X=trial.X;cpu->Y=trial.Y;cpu->D=trial.D;cpu->P=trial.P;cpu_p_to_mirrors(cpu);
  /* The original final RTL consumes only this previously validated JSL frame. */
  cpu->coprocessor_master_cycles=cpu->master_cycles;cpu->cycles+=6;cpu->master_cycles+=48;
  cpu_read8(cpu,0,++cpu->S);cpu_read8(cpu,0,++cpu->S);cpu_read8(cpu,0,++cpu->S);
  *out=RECOMP_RETURN_NORMAL;return true;
}
#endif
