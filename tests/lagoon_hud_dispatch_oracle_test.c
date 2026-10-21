/* Real selector18 call/DC source path plus independent title/SDK refusal cases.
 * Reuse the existing original-opcode bus and actual extracted SDK policy. */
#define main old_inline_oracle_main
#include "lagoon_endgame_inline_oracle_test.c"
#undef main
static unsigned checks;
static CpuState c;
static void hud_wr(void *ctx,uint32_t address,uint8_t value) {
 unsigned bank=address>>16;
 if(bank==0x7e||bank==0x7f){ram[(bank-0x7e)*65536+(address&65535)]=value;return;}
 wr(ctx,address,value);
}
static void setup_hud(void) {
 memset(ram,0,sizeof ram);memset(&c,0,sizeof c);generation=1;g_rom=rom;
 c.ram=ram;c.S=0x1f00;c.DB=2;c.P=CPU_P_M;c.m_flag=1;c.A=0xb;c.X=0x840;c.Y=0x8123;
 ram[0x10]=0xb;ram[0x11]=0xd5;ram[0x12]=2;ram[0x13]=0x18;
 ram[c.S+1]=0xe3;ram[c.S+2]=0x80;ram[c.S+3]=0;ram[c.S+4]=1;
 put(ram,c.S+5,0x8b2c);ram[c.S+7]=1;
 interp_bridge_set_aot_continuation_hook(LagoonNativeEndgameHook);
 continuation_discard_tokens();LagoonNativeEndgameBeforeFrame();
}
static void arm_hud(void) {
 CpuState saved=c;memcpy(before,ram,sizeof ram);
 (void)continuation_arm(&c,0x0080e6,0x0080e0,3,1);
 assert(!memcmp(&saved,&c,sizeof c)&&!memcmp(before,ram,sizeof ram));checks++;
}
static int take_hud(const CpuState *cpu,unsigned source,unsigned target) {
 CpuState saved=*cpu;memcpy(before,ram,sizeof ram);InterpAotAncestor out={0};
 int ok=continuation_indirect_long_call_tail(cpu,source,target,&out);
 assert(!memcmp(&saved,cpu,sizeof saved)&&!memcmp(before,ram,sizeof ram));
 if(ok)assert(out.entry_s==cpu->S&&out.owner_frame==3&&out.owner_return_pc24==0x0080e4&&out.pc24==0x02d50b);
 checks++;return ok;
}
static void mutations(void) {
 setup_hud();arm_hud();c.PB=2;assert(take_hud(&c,0x80e6,0x2d50b));
 continuation_event(INTERP_CONT_TAKE,&c,0x80e6,0x2d50b,NULL,c.S);assert(!hud.phase&&stats.hud_takes);
 assert(!take_hud(&c,0x80e6,0x2d50b));
 for(unsigned stage=0;stage<2;stage++)for(unsigned k=0;k<35;k++) {
  setup_hud();if(stage){arm_hud();c.PB=2;}
  CpuState other;
  switch(k){
   case 0:c.A^=0x100;break;case 1:c.X++;break;case 2:c.Y++;break;
   case 3:c.S++;break;case 4:c.DB=1;break;case 5:c.D=1;break;
   case 6:c.emulation=1;break;case 7:c.m_flag=0;break;case 8:c.x_flag=1;break;
   case 9:c.P^=CPU_P_M;break;case 10:c.P^=CPU_P_X;break;
   case 11:c.P|=CPU_P_D;break;case 12:c._flag_D=1;break;
   case 13:ram[0x10]^=1;break;case 14:ram[0x11]^=1;break;
   case 15:ram[0x12]^=1;break;case 16:ram[0x13]^=1;break;
   case 17:case 18:case 19:case 20:case 21:case 22:case 23:ram[c.S+1+k-17]^=1;break;
   case 24:generation++;break;case 25:c.PB=3;break;
   case 26:continuation_event(INTERP_CONT_DISCARD,NULL,0,0,NULL,0);break;
   case 27:continuation_event(INTERP_CONT_CALL,&c,0x8181,0x8484,NULL,0);break;
   case 28:c.S=0xff;break;case 29:c.S=0x1ff9;break;
   case 30:break;case 31:break;case 32:break;case 33:break;
   case 34:memcpy(expected,ram,sizeof ram);c.ram=expected;break;
  }
  if(!stage){arm_hud();c.PB=2;}
  other=c;
  int ok=take_hud(k==30?&other:&c,k==31?0x8106:0x80e6,k==32?0x2d50c:0x2d50b);
  /* Before arming, X/Y, generation, discard and unrelated calls do not
   * invalidate a later REAL source call; the witness captures that state. */
  int allowed=!stage&&(k==1||k==2||k==20||k==24||k==26||k==27||k==33||k==34);
  if(stage&&k==33)allowed=1;
  if(ok!=allowed)fprintf(stderr,"mutation stage=%u k=%u ok=%d allowed=%d\n",stage,k,ok,allowed);
  assert(ok==allowed);
 }
 for(unsigned stage=0;stage<2;stage++)for(unsigned s=0;s<sizeof hud_spans/sizeof *hud_spans;s++)
 for(unsigned n=0;n<hud_spans[s].count;n++) {
  setup_hud();if(stage){arm_hud();c.PB=2;}
  unsigned pc=hud_spans[s].pc,off=((pc>>16)&127)*32768+(pc&32767)+n;rom[off]^=1;
  if(!stage){arm_hud();c.PB=2;}
  assert(!take_hud(&c,0x80e6,0x2d50b));rom[off]^=1;
 }
 setup_hud();arm_hud();c.PB=2;
 assert(!continuation_tail_site(0x4c,0x80e6));assert(!continuation_tail_site(0x5c,0x80e6));
 assert(continuation_tail_site(0xdc,0x80e6));
 assert(!interp_bridge_set_aot_indirect_long_call_tail_hook(0x1000000,hud_tail));
 assert(interp_bridge_set_aot_indirect_long_call_tail_hook(0,NULL));assert(!take_hud(&c,0x80e6,0x2d50b));
}
static InterpAotAncestor forged_owner;
static int forged_hud(const CpuState *cpu,uint32_t source,uint32_t target,InterpAotAncestor *out) {
 (void)cpu;(void)source;(void)target;*out=forged_owner;return 1;
}
static void sdk_guards(void) {
 for(unsigned k=0;k<13;k++){
  setup_hud();c.PB=2;forged_owner=(InterpAotAncestor){0x02d50b,0x0080e4,c.S,3};
  assert(interp_bridge_set_aot_indirect_long_call_tail_hook(0x80e6,forged_hud));
  switch(k){case 0:break;case 1:forged_owner.owner_frame=2;break;
   case 2:forged_owner.entry_s++;break;case 3:forged_owner.pc24++;break;
   case 4:forged_owner.owner_return_pc24++;break;case 5:forged_owner.owner_return_pc24=0x0280e4;break;
   case 6:forged_owner.owner_return_pc24=0x1000000;break;case 7:c.PB=1;break;
   case 8:c.emulation=1;break;case 9:c.ram=NULL;break;
   case 10:c.S=forged_owner.entry_s=0xff;break;case 11:c.S=forged_owner.entry_s=0x1ffd;break;
   case 12:break;
  }
  InterpAotAncestor out={0};assert(continuation_indirect_long_call_tail(&c,0x80e6,0x02d50b,k==12?NULL:&out)==(k==0));checks++;
 }
 assert(!continuation_indirect_long_call_tail(NULL,0x80e6,0x02d50b,NULL));checks++;
}
static void actual_opcodes(void) {
 unsigned callers[]={0x018b27,0x019442,0x019473},stacks[]={0x180,0x1fe0},ys[]={0,0x8000,0xffff},dbs[]={0,1,2,0x7e};
 unsigned count=0;
 for(unsigned ci=0;ci<3;ci++)for(unsigned si=0;si<2;si++)for(unsigned yi=0;yi<3;yi++)
 for(unsigned di=0;di<4;di++)for(unsigned flags=0;flags<8;flags++)for(unsigned val=0;val<3;val++){
  setup_hud();memset(ram,0,sizeof ram);unsigned pre=stacks[si],x=ci?0:0x840;
  ram[0x522]=val==0?0:val==1?97:255;ram[0x523]=val==2?255:0;
  Interp816 *i=interp816_init(NULL,rd,hud_wr);assert(i);i->pc=callers[ci]&65535;i->k=1;i->db=dbs[di];
  i->a=0;i->x=x;i->y=ys[yi];i->sp=pre;i->dp=i->e=0;
  interp816_setFlags(i,CPU_P_M|((flags&1)?CPU_P_C:0)|((flags&2)?CPU_P_V:0)|((flags&4)?CPU_P_I:0));
  unsigned ops=0,cycles=0;
  while(!(i->k==0&&i->pc==0x80e6)){assert(++ops<100);cycles+=interp816_runOpcode(i);}
  sync(&c,i,cycles);arm_hud();assert(hud.phase==1);
  assert(i->sp==pre-7&&i->db==2&&i->a==0xb&&i->x==x&&i->y==ys[yi]);
  cycles+=interp816_runOpcode(i);sync(&c,i,cycles);assert(take_hud(&c,0x80e6,0x2d50b));
  continuation_event(INTERP_CONT_TAKE,&c,0x80e6,0x2d50b,NULL,c.S);assert(!hud.phase);
  while(!(i->k==0&&i->pc==0x80e4)){assert(++ops<10000);interp816_runOpcode(i);}
  assert(i->sp==pre-4&&i->db==2&&i->a==0x7c&&i->x==x&&i->y==0x2c0);
  interp816_runOpcode(i);assert(i->db==dbs[di]&&i->sp==pre-3&&i->pc==0x80e5);
  interp816_runOpcode(i);assert(i->k==1&&i->pc==(callers[ci]+6)%65536&&i->sp==pre);
  interp816_free(i);count++;
 }
 printf("PASS %u actual original call/preparation/DC/fullHUDchildren/PLB/RTL and actualSDK-title accepts; unit bus only\n",count);
}
int main(int argc,char **argv) {
 assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);g_rom=rom;
 mutations();sdk_guards();printf("PASS %u read-only SDK/title mutation and full-span byte guards\n",checks);
 actual_opcodes();return 0;
}
