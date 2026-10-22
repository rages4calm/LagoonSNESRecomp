#include "lagoon_menu_dma.h"
#include "lagoon_door_owner.h"
static unsigned word(const uint8_t*r,unsigned q){return r[q]|r[q+1]<<8;}
static unsigned triple(const uint8_t*r,unsigned q){return word(r,q)|r[q+2]<<16;}
static uint64_t hash(const uint8_t*r,unsigned n){uint64_t h=UINT64_C(14695981039346656037);while(n--)h=(h^*r++)*UINT64_C(1099511628211);return h;}
static int owner(unsigned p){return p==0x02b73a||p==0x02b87d||p==0x02bc9b;}
int LagoonMenuDma_Service(const uint8_t*rom,size_t size,uint8_t*r,const CpuState*c,
 uint32_t pc,LagoonMenuDmaAuthority a,LagoonMenuDmaWrite wr,void*ctx){
 if(!rom||size!=0x100000||!r||!c||!wr||(pc!=0x0089ac&&pc!=0x0089a7)||!a.forced_blank||a.hdma||!a.idle)return 0;
 unsigned s=c->S;
 if(!c->m_flag||c->x_flag||c->emulation||c->D||c->_flag_D||(c->P&8)||c->PB||c->DB!=1||s<0x100||s>0x1e9)return 0;
 if(!(r[0x23d]&128)||r[0x275]!=0x81||(r[0x100e]&&r[0x100e]!=255)||r[0x23a]!=(pc==0x89a7?0:0x20)||r[0x264])return 0;
 unsigned ret=triple(r,s+1),parent=word(r,s+9);
 unsigned source=triple(r,8),dest=word(r,0x28a),length=word(r,0x28c);
 if(source==0x7e4800){
  /* Original028976 filled this exact HUD backing store. Its0400-byte tile
   * queue remains pending and unchanged; bit40 has not been requested yet. */
  if(s>0x1e4||ret!=0x0289b9||word(r,s+6)!=0x8993||word(r,s+10)!=0x8972||triple(r,s+16)!=0x018ad0||word(r,s+19)!=0x8743||
      !((!r[0x100e]&&triple(r,s+21)==0x0080e3&&r[s+24]==2&&owner(triple(r,s+25)))||LagoonDoorOwner(rom,size,r,s+21))||word(r,0x238)!=0x400||dest!=0x3c00||length!=0x600)return 0;
  if(hash(rom+0x1096b,0x51)!=UINT64_C(0x1f3df5c0fec1a109)||hash(rom+0x8acd,0x29)!=UINT64_C(0x6775aa0a59bb058d))return 0;
  for(unsigned q=0x4800;q<0x4e00;q+=2)if(word(r,q)!=0x3d8c)return 0;
 }else if(source==0x7e3000){
  /* Original899B sets the source and falls through to the interpreter poll.
   * Its original request/poll/RTL instructions still execute normally. */
  if(pc!=0x89ac||s>0x1f3||ret!=0x018af5||word(r,s+4)!=0x8743||
      !((!r[0x100e]&&triple(r,s+6)==0x0080e3&&r[s+9]==2&&owner(triple(r,s+10)))||LagoonDoorOwner(rom,size,r,s+6))||word(r,0x238)!=0x400||dest!=0x3c00||length!=0x800)return 0;
  if(hash(rom+0x99b,0xc)!=UINT64_C(0xf13054bb7e801f26)||hash(rom+0x8acd,0x29)!=UINT64_C(0x6775aa0a59bb058d))return 0;
  for(unsigned q=0x3000;q<0x3800;q+=2)if(word(r,q)!=0x01ff)return 0;
 }else{
  if(word(r,0x238))return 0;
 if(ret!=0x0090a1&&ret!=0x0090d8)return 0;
 unsigned n;
 if(source<0x7f7f80||source>0x7faf80||(source-0x7f7f80)%0x1000)return 0;
 n=(source-0x7f7f80)/0x1000;
 unsigned last,total,base;
 if(triple(r,s+6)==0x0188df&&word(r,s+11)==0x87d5&&word(r,s+13)==0x8732&&
     ((!r[0x100e]&&triple(r,s+15)==0x0080e3&&r[s+18]==2&&owner(triple(r,s+19)))||LagoonDoorOwner(rom,size,r,s+15))){
  if(parent==0x887d){last=3;total=0x37c0;base=0;}
  else if(parent==0x8892){last=2;total=0x2a00;base=0x4400;}
  else return 0;
 }else if(triple(r,s+6)==0x0094e3){
  /* Original common graphics loader preserves two real saved-X frames.
   * Common9 is called by the world-loader before BG/OBJ; common0 afterward.
   * The exact outer submenu frame excludes ordinary room-loading callers. */
  if(hash(rom+0x14a1,0x45)!=UINT64_C(0x1d3849a43513aa9f))return 0;
  if(s<=0x1e8&&triple(r,s+11)==0x0097eb&&triple(r,s+14)==0x018727&&
      ((!r[0x100e]&&triple(r,s+17)==0x0080e3&&r[s+20]==2&&owner(triple(r,s+21)))||LagoonDoorOwner(rom,size,r,s+17))){
   if(hash(rom+0x1525,7)!=UINT64_C(0xcc9872e0336fb10b)||hash(rom+0x8703,0x31)!=UINT64_C(0x8dd0d30feb8eddd5)||hash(rom+0x17e6,6)!=UINT64_C(0x041e0ddcae4eb98e))return 0;
   last=1;total=0x1f00;base=0x6000;
  }else if(s<=0x1e7&&triple(r,s+11)==0x018898&&word(r,s+14)==0x87d5&&word(r,s+16)==0x8732&&
      ((!r[0x100e]&&triple(r,s+18)==0x0080e3&&r[s+21]==2&&owner(triple(r,s+22)))||LagoonDoorOwner(rom,size,r,s+18))){
   if(hash(rom+0x14e6,7)!=UINT64_C(0x2f9dfa327c597981))return 0;
   last=0;total=0x200;base=0x4300;
  }else return 0;
 }else return 0;
 if(n>last||dest!=base+n*0x800||length!=(n==last?total-n*0x1000:0x1000)||word(r,0x1000)!=total-n*0x1000||ret!=(n==last?0x0090d8:0x0090a1))return 0;
 }
 /* Pin the original consumer, trigger/clear, poll, producer and parent calls.
  * No fallible operation or guest callback occurs after these checks. */
 if(hash(rom+0x84b,0x38)!=UINT64_C(0x2b1eec1eca0b79f0)||hash(rom+0x8be,0xb)!=UINT64_C(0x4247c64e6ec6154f)||hash(rom+0x9a7,0xd)!=UINT64_C(0x81364db374fb949a)||hash(rom+0x108a,0x51)!=UINT64_C(0xa4d117801c8e3fec)||hash(rom+0x8869,0x78)!=UINT64_C(0x136150341005b748)||hash(rom+0x8730,3)!=UINT64_C(0xc51f7f17cfe9f982)||hash(rom+0x87d0,6)!=UINT64_C(0x61c0615c3345fa6c))return 0;
 /* Original entry LDA20/TSB request, after every fallible guard. */
 if(pc==0x89a7)r[0x23a]|=0x20;
 /* Original00884B..8875, then its genuine420B channel1 transfer. Only its
  * scheduling moves: forced blank permits writes throughout the field. */
 wr(ctx,0x2116,dest);wr(ctx,0x2117,dest>>8);
 wr(ctx,0x4310,1);wr(ctx,0x4311,0x18);
 wr(ctx,0x4312,source);wr(ctx,0x4313,source>>8);wr(ctx,0x4314,source>>16);
 wr(ctx,0x4315,length);wr(ctx,0x4316,length>>8);
 wr(ctx,0x420b,2);
 /* cpu_write8's420B handler completes DMA and charges every transferred byte
  * synchronously. Publish completion only afterward; keep the original poll. */
 r[0x23a]&=(uint8_t)~0x20;
 return 1;
}
bool LagoonMenuDma_Run(const uint8_t*rom,size_t size,uint8_t*r,CpuState*c,
 uint16_t entry_s,uint8_t hrv,uint32_t host_return_pc24,RecompReturn*out,
 LagoonMenuDmaAuthority a,LagoonMenuDmaWrite wr,void*ctx){
 if(!c||!r||!out||c->S!=entry_s||hrv!=3||c->host_return_valid!=3||entry_s<0x100||entry_s>0x1e9)return false;
 unsigned ret=triple(r,entry_s+1),actual=(ret&0xff0000)|((ret+1)&65535);
 if(actual!=host_return_pc24)return false;
 if(!LagoonMenuDma_Service(rom,size,r,c,0x89a7,a,wr,ctx))return false;
 /* Original one completed poll: LDA023A/AND20 leaves A.low=0,N=0,Z=1.
  * All other CPU flags and registers survive. Preserve original22-cycle
  * request/poll/RTL CPU budget, in addition to the genuine DMA charge. */
 c->A&=0xff00;c->_flag_Z=1;c->_flag_N=0;c->P=(c->P&~0x82)|2;
 c->cycles+=22;c->master_cycles+=128;c->coprocessor_master_cycles=c->master_cycles;c->master_cycles+=48;
 cpu_read8(c,0,++c->S);cpu_read8(c,0,++c->S);cpu_read8(c,0,++c->S);
 *out=RECOMP_RETURN_NORMAL;return true;
}
#ifndef LAGOON_MENU_DMA_TEST
#include "common_rtl.h"
#include "snes/ppu.h"
#include "snes/dma.h"
#include "snes/interp_bridge.h"
static void write(void*ctx,uint16_t addr,uint8_t value){cpu_write8((CpuState*)ctx,0,addr,value);}
static LagoonMenuDmaAuthority authority(void){
 LagoonMenuDmaAuthority a={!!PPU_forcedBlank(g_ppu),g_snesrecomp_last_hdmaen,!g_dma->dmaBusy};
 if(!g_ppu->vramIncrementOnHigh||g_ppu->vramIncrement!=1||g_ppu->vramRemapMode)a.idle=0;
 for(unsigned n=0;n<8;n++)if(g_dma->channel[n].dmaActive||g_dma->channel[n].hdmaActive)a.idle=0;
 return a;
}
static int logging(void){static int enabled=-1;if(enabled<0){const char*v=getenv("SNESRECOMP_DMA_LOG");enabled=v&&v[0]=='1'&&!v[1];}return enabled;}
static void poll(CpuState*c,uint32_t pc){
 if(c!=&g_cpu||!g_ppu||!g_dma||triple(g_ram,8)!=0x7e3000)return;
 uint64_t before=c->master_cycles;
 int done=LagoonMenuDma_Service(g_rom,0x100000,g_ram,c,pc,authority(),write,c);
 if(done&&logging())fprintf(stderr,"[menu-dma-poll] src=7E3000 dest=3C00 bytes=2048 blank=1 master_before=%llu master_after=%llu delta=%llu\n",(unsigned long long)before,(unsigned long long)c->master_cycles,(unsigned long long)(c->master_cycles-before));
}
void LagoonMenuDma_BeforeFrame(void){static int ready;if(!ready&&g_rom)ready=interp_bridge_add_pre_opcode_hook(0x0089ac,poll);}
bool LagoonMenuDma_TryNative(CpuState*c,uint16_t entry_s,uint8_t hrv,uint32_t host_return_pc24,RecompReturn*out){
 if(c!=&g_cpu||!g_ppu||!g_dma)return false;
 LagoonMenuDmaAuthority a=authority();
 unsigned source=triple(g_ram,8),dest=word(g_ram,0x28a),length=word(g_ram,0x28c);
 uint64_t before=c->master_cycles;
 bool done=LagoonMenuDma_Run(g_rom,0x100000,g_ram,c,entry_s,hrv,host_return_pc24,out,a,write,c);
 if(!done&&logging()&&a.forced_blank&&c->S>=0x100&&c->S<=0x1df){
  fprintf(stderr,"[menu-dma-pending] src=%06X dest=%04X bytes=%u db=%02X s=%04X req=%02X queue=%04X pending=%02X stack=",source,dest,length,c->DB,c->S,g_ram[0x23a],word(g_ram,0x238),g_ram[0x264]);
  for(unsigned j=1;j<=30;j++)fprintf(stderr,"%02X",g_ram[c->S+j]);
  fputc('\n',stderr);
 }
 if(done&&logging())fprintf(stderr,"[menu-dma] src=%06X dest=%04X bytes=%u blank=%u master_before=%llu master_after=%llu delta=%llu\n",source,dest,length,a.forced_blank,(unsigned long long)before,(unsigned long long)c->master_cycles,(unsigned long long)(c->master_cycles-before));
 return done;
}
#endif
