/* Execute the real JSL and every original80E9 instruction before asking the
 * actual SDK/title hook to lend the remaining original two-byte owner. */
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "lagoon_endgame_host_stubs.h"
#include "../src/lagoon_native_endgame.c"
static uint8_t rom[0x100000],ram[0x20000],before[0x20000],expected[0x20000];
const uint8_t *g_rom;
static uint64_t generation=1;
uint64_t RtlStateGeneration(void){return generation;}
#include "endgame_policy_under_test.h"
static uint8_t rd(void*ctx,uint32_t a) {
  (void)ctx;unsigned b=a>>16,p=a&65535;
  if((b==0x7E||b==0x7F))return ram[(b-0x7E)*65536+p];
  if((b&127)<0x40 && p<0x2000)return ram[p];
  assert(b<32&&p>=32768);return rom[b*32768+(p&32767)];
}
static void wr(void*ctx,uint32_t a,uint8_t v) {
  (void)ctx;assert((a>>16)<0x40&&(a&65535)<0x2000);ram[a&65535]=v;
}
static void sync(CpuState*c,const Interp816*i,unsigned cycles) {
  c->A=i->a;c->X=i->x;c->Y=i->y;c->S=i->sp;c->D=i->dp;c->PB=i->k;c->DB=i->db;
  c->P=interp816_getFlags((Interp816*)i);c->emulation=i->e;cpu_p_to_mirrors(c);c->cycles=cycles;
}
static void put(uint8_t*r,unsigned p,unsigned v){r[p]=v;r[p+1]=v>>8;}
int main(int argc,char**argv) {
  assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);g_rom=rom;
  interp_bridge_set_aot_continuation_hook(LagoonNativeEndgameHook);LagoonNativeEndgameBeforeFrame();
  const unsigned stacks[]={0x180,0x1FD0},ys[]={0,0x8000,0xFFFF},high[]={0,0x100,0xFF00};
  unsigned count=0,takes=0,unknown=0;
  for(unsigned owner=0;owner<3;owner++)for(unsigned sel=0;sel<16;sel++)
  for(unsigned s=0;s<2;s++)for(unsigned y=0;y<3;y++)for(unsigned x=0;x<2;x++)
  for(unsigned hi=0;hi<3;hi++)for(unsigned flags=0;flags<16;flags++) {
    for(unsigned n=0;n<sizeof ram;n++)ram[n]=(uint8_t)(n*73+count*17);
    unsigned pre=stacks[s],call=owners[owner].call;
    put(ram,pre+1,owners[owner].owner-1);memcpy(before,ram,sizeof ram);memcpy(expected,ram,sizeof ram);
    Interp816*i=interp816_init(NULL,rd,wr);assert(i);i->pc=call&65535;i->k=i->db=1;
    i->a=high[hi]|sel;i->x=x?0x840:0;i->y=ys[y];i->sp=pre;i->dp=i->e=0;
    unsigned p=CPU_P_M|((flags&1)?CPU_P_C:0)|((flags&2)?CPU_P_V:0)|
      ((flags&4)?CPU_P_D:0)|((flags&8)?CPU_P_I:0);
    interp816_setFlags(i,p);CpuState c={0};c.ram=ram;continuation_discard_tokens();
    unsigned cycles=interp816_runOpcode(i);assert(i->k==0&&i->pc==0x80E9&&i->sp==pre-3);
    sync(&c,i,cycles);CpuState saved=c;uint8_t saved_ram[0x20000];memcpy(saved_ram,ram,sizeof ram);
    (void)continuation_arm(&c,0x0080E9,call,3,1);
    assert(!memcmp(&c,&saved,sizeof c)&&!memcmp(saved_ram,ram,sizeof ram));
    unsigned ops=0;
    while(i->pc!=0x8106||i->k!=0){assert(++ops<=32);cycles+=interp816_runOpcode(i);}
    cycles+=interp816_runOpcode(i);assert(i->k==1&&i->sp==pre);
    unsigned target=0x010000|i->pc;
    /* Independent source semantics include hiddenA-high table addressing,
     * long-address carry and the real consumed/dead return-frame bytes. */
    unsigned index=high[hi]|(sel*2),table=call+3+index+1;
    unsigned lo=rd(NULL,table),hh=rd(NULL,table+1);
    put(expected,pre-2,call+3);expected[pre]=1;
    expected[0]=sel;put(expected,1,call+3);expected[3]=expected[6]=1;
    expected[4]=lo;expected[5]=hh;put(expected,pre-1,ys[y]);
    assert(target==(0x010000|lo|hh<<8));assert(!memcmp(expected,ram,sizeof ram));
    unsigned want_p=(p&~(CPU_P_N|CPU_P_Z|CPU_P_C))|(ys[y]&0x8000?CPU_P_N:0)|(ys[y]?0:CPU_P_Z);
    assert(i->a==(high[hi]|hh)&&i->x==(x?0x840:0)&&i->y==ys[y]&&i->db==1&&i->dp==0&&
      interp816_getFlags(i)==want_p);
    sync(&c,i,cycles);saved=c;memcpy(saved_ram,ram,sizeof ram);InterpAotAncestor out={0};
    int result=continuation_indirect_long_tail(&c,0x008106,target,&out);
    assert(!memcmp(&c,&saved,sizeof c)&&!memcmp(saved_ram,ram,sizeof ram));
    int allowed=!hi && !(flags&4) && approved(owner,target);
    assert(result==allowed);
    if(result){assert(out.pc24==target&&out.entry_s==pre&&out.owner_frame==2&&out.owner_return_pc24==owners[owner].owner);
      continuation_event(INTERP_CONT_TAKE,&c,0x008106,target,NULL,c.S);takes++;}
    else if(hi)unknown++;
    assert(!continuation_indirect_long_tail(&c,0x008106,target,&out));
    interp816_free(i);count++;
  }
  assert(count==27648&&takes==1824&&unknown==18432);
  printf("PASS %u actual JSL/original80E9/realDC8106/actualSDK-title cases;full128KiBRAM+registers/flags/hiddenA/longtablecarry/realconsumed3byteframe+remaining2byteowner/deadPHYbytes; actualtakes=%u unknown-hiddenA-refusals=%u; no childbody or scheduler/APU claim\n",count,takes,unknown);
  return 0;
}
