#include "lagoon_menu_close.h"
#include <stdlib.h>
#include <string.h>
static unsigned word(const uint8_t*r,unsigned q){return r[q]|r[q+1]<<8;}
static unsigned triple(const uint8_t*r,unsigned q){return word(r,q)|r[q+2]<<16;}
static void put(uint8_t*r,unsigned q,unsigned v){r[q]=v;r[q+1]=v>>8;}
static void nz(CpuState*c,unsigned v,int wide){unsigned n=wide?0x8000:0x80;c->_flag_Z=!v;c->_flag_N=!!(v&n);c->P=(c->P&~0x82)|(c->_flag_Z?2:0)|(c->_flag_N?0x80:0);}
static void carry(CpuState*c,int v){c->_flag_C=!!v;c->P=(c->P&~1)|!!v;}
static void overflow(CpuState*c,int v){c->_flag_V=!!v;c->P=(c->P&~0x40)|(v?0x40:0);}
static void a8(CpuState*c,unsigned v){c->A=(c->A&0xff00)|(v&255);nz(c,v&255,0);}
static void a16(CpuState*c,unsigned v){c->A=v&65535;nz(c,c->A,1);}
static void x16(CpuState*c,unsigned v){c->X=v&65535;nz(c,c->X,1);}
static void y16(CpuState*c,unsigned v){c->Y=v&65535;nz(c,c->Y,1);}
static void cmp(CpuState*c,unsigned a,unsigned b,int wide){unsigned mask=wide?65535:255;carry(c,a>=b);nz(c,(a-b)&mask,wide);}
static void add(CpuState*c,unsigned b,int wide){unsigned a=c->A,mask=wide?65535:255,sign=wide?32768:128;a&=mask;unsigned v=a+b;carry(c,v>mask);overflow(c,(~(a^b)&(a^v)&sign)!=0);if(wide)a16(c,v);else a8(c,v);}
static void width(CpuState*c,int wide){c->m_flag=!wide;c->P=(c->P&~0x20)|(wide?0:0x20);}
static int valid(const CpuState*c){return c&&c->m_flag&&!c->x_flag&&!c->emulation&&!c->D&&!c->_flag_D&&!(c->P&8)&&c->S>=0x100&&c->S<=0x1f0;}
static int romread(const uint8_t*rom,size_t size,unsigned address,unsigned *out){unsigned bank=address>>16,low=address&65535;if((bank&127)>=32||low<32768)return 0;size_t q=(bank&31)*32768+(low&32767);if(q>=size)return 0;*out=rom[q];return 1;}
static int rw(const uint8_t*rom,size_t size,unsigned address,unsigned*out){unsigned a,b;if(!romread(rom,size,address,&a)||!romread(rom,size,address+1,&b))return 0;*out=a|b<<8;return 1;}
/* Original02C5C6..C64C: initialize the32KiB map then expand both metatile
 * planes. No device access, wait, clock, stack frame or return is synthesized. */
int LagoonMenuClose_BuildMap(const uint8_t*rom,size_t size,uint8_t*ram,CpuState*cpu){
 if(!rom||!ram||!valid(cpu))return 0;unsigned w=word(ram,0x53),h=word(ram,0x55);if((w&3)||(h&3)||w<4||h<4||w>256||h>256)return 0;w/=4;h/=4;
 uint8_t*r=malloc(0x20000);if(!r)return 0;memcpy(r,ram,0x20000);CpuState c=*cpu;
 memset(r+0x10000,r[0x52],0x8000);put(r,0x1068,w);put(r,0x106e,w);put(r,0x106a,h);put(r,0x106c,0);
 unsigned base=triple(r,0x63),defs=triple(r,0x66),base2=triple(r,0x6f),defs2=triple(r,0x72);unsigned index=0;
 for(unsigned row=0;row<h;row++)for(unsigned col=0;col<w;col++,index++){
  unsigned x=row*256+col*2,t,t2,a,b,d,e;
  if(!romread(rom,size,base+index,&t)||!romread(rom,size,base2+index,&t2)||!rw(rom,size,defs+t*4,&a)||!rw(rom,size,defs+t*4+2,&b)||!rw(rom,size,defs2+t2*4,&d)||!rw(rom,size,defs2+t2*4+2,&e)){free(r);return 0;}
  put(r,0x10000+x,a);put(r,0x10080+x,b);put(r,0x14000+x,d);put(r,0x14080+x,e);
  /* Original PHY16 then temporary PHA8; include the dead hardware-stack bytes. */
  put(r,c.S-1,index);r[c.S-2]=t2;
  if(col+1==w){put(r,0x1068,(row+2)*w);put(r,0x106c,row+1);}
 }
 c.A=h;c.X=(h-1)*256+w*2;c.Y=w*h;carry(&c,1);overflow(&c,0);nz(&c,h,1);width(&c,0);
 memcpy(ram,r,0x20000);free(r);*cpu=c;return 1;
}
/* Original0089EC..8A95 specialized to the loader's1x1 rectangles. If the
 * original queue fills, stop BEFORE its original PHX/PHY/cooperative wait. */
int LagoonMenuClose_QueueTile(uint8_t*ram,CpuState*cpu,unsigned*landing){
 if(!ram||!landing||!valid(cpu))return 0;unsigned source=triple(ram,8),slot=word(ram,0x238);if((source!=0x2d0&&source!=0x2e0)||ram[source]!=1||ram[source+1]!=1||(cpu->A&0xff00)||slot>0x77c||(slot&3))return 0;
 /* All reads/writes below are bounded by the validated fixed1x1 layout.
  * There is no fallible operation after this point, so no whole-WRAM copy. */
 uint8_t*r=ram;CpuState c=*cpu;unsigned s=c.S;
 put(r,s-1,c.X);put(r,s-3,c.Y);c.S-=4;
 a8(&c,r[0x215]);r[0x217]=c.A;a8(&c,r[0x299]);a8(&c,c.A&3);x16(&c,c.A&255);a8(&c,r[0x2a3+c.X]);carry(&c,0);add(&c,r[0x216],0);r[0x218]=c.A;
 x16(&c,slot);y16(&c,0);a8(&c,r[source]);r[0x21b]=c.A;y16(&c,1);a8(&c,r[source+1]);r[0x21d]=c.A;y16(&c,2);
 a8(&c,r[0x21b]);r[0x21c]=c.A;a8(&c,r[0x217]);r[0x219]=c.A;a8(&c,r[0x218]);r[0x21a]=c.A;
 width(&c,1);a16(&c,word(r,source+c.Y));put(r,0x2602+c.X,c.A);y16(&c,c.Y+2);cmp(&c,c.A,65535,1);
 if(!c._flag_Z){a16(&c,word(r,0x219));put(r,0x2600+c.X,c.A);x16(&c,c.X+4);put(r,0x238,c.X);}
 a16(&c,c.A&255);width(&c,0);cmp(&c,c.X,0x780,1);
 unsigned next;
 if(c._flag_Z)next=0x008a97;
 else{
  width(&c,1);put(r,0x219,word(r,0x219)+1);nz(&c,word(r,0x219),1);a16(&c,word(r,0x219));nz(&c,c.A&31,1);
  if(c._flag_Z){a16(&c,c.A-1);a16(&c,c.A&0xffe0);a16(&c,c.A^0x400);put(r,0x219,c.A);}
  a16(&c,c.A&255);width(&c,0);--r[0x21c];nz(&c,r[0x21c],0);width(&c,1);a16(&c,word(r,0x217));put(r,c.S-1,c.A);a16(&c,c.A&0xfc00);put(r,0x219,c.A);a16(&c,word(r,c.S-1));carry(&c,0);add(&c,0x20,1);a16(&c,c.A&0x3ff);a16(&c,c.A|word(r,0x219));put(r,0x217,c.A);a16(&c,c.A&255);width(&c,0);--r[0x21d];nz(&c,r[0x21d],0);
  y16(&c,word(r,c.S+1));c.S+=2;x16(&c,word(r,c.S+1));c.S+=2;next=0x008a96;
 }
 *cpu=c;*landing=next;return 1;
}
#ifndef LAGOON_MENU_CLOSE_TEST
#include "common_rtl.h"
#include "snes/interp_bridge.h"
static uint64_t hash(const uint8_t*r,unsigned n){uint64_t h=UINT64_C(14695981039346656037);while(n--)h=(h^*r++)*UINT64_C(1099511628211);return h;}
static int menu(unsigned pc){return pc==0x02b73a||pc==0x02b87d||pc==0x02bc9b;}
static void close_hook(CpuState*c,uint32_t pc){
 unsigned s=c->S;if(!g_rom||!valid(c)||s>0x1e9||!(g_ram[0x23d]&128)||g_ram[0x275]!=0x81||g_ram[0x100e])return;
 if(pc==0x02c5c6){
  if(c->PB!=2||c->DB!=2||triple(g_ram,s+1)!=0x0080e3||g_ram[s+4]!=1||triple(g_ram,s+5)!=0x0187e5||word(g_ram,s+8)!=0x8732||triple(g_ram,s+10)!=0x0080e3||g_ram[s+13]!=2||!menu(triple(g_ram,s+14)))return;
  if(hash(g_rom+0x145c6,0x88)!=UINT64_C(0x610feb01f813f643))return;
  if(LagoonMenuClose_BuildMap(g_rom,0x100000,g_ram,c))interp_bridge_pre_opcode_redirect(0x02c64d);
 }else if(pc==0x0089ec){
  unsigned ret=triple(g_ram,s+1);
  if(c->PB||c->DB!=1||(ret!=0x018a80&&ret!=0x018a90)||word(g_ram,s+4)!=0x8a31||word(g_ram,s+8)!=0x8740||triple(g_ram,s+10)!=0x0080e3||g_ram[s+13]!=2||!menu(triple(g_ram,s+14)))return;
  if(hash(g_rom+0x9ec,0xb8)!=UINT64_C(0x2037f99e6ce56f55))return;
  unsigned landing;if(LagoonMenuClose_QueueTile(g_ram,c,&landing))interp_bridge_pre_opcode_redirect(landing);
 }
}
void LagoonMenuClose_BeforeFrame(void){static int ready;if(!ready&&g_rom){int a=interp_bridge_add_pre_opcode_hook(0x02c5c6,close_hook);int b=interp_bridge_add_pre_opcode_hook(0x0089ec,close_hook);ready=a&&b;}}
#endif


