/* Clean user ROM only. Execute the original linear consumer and420B trigger
 * instructions as an independent MMIO-order oracle. This deliberately is not
 * an oracle for the unrelated timer/OAM/audio work of an entire NMI. */
#include "lagoon_menu_dma.h"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#undef assert
#define assert(v) do{if(!(v)){fprintf(stderr,"ASSERT line%d: %s\n",__LINE__,#v);exit(2);}}while(0)
#define abort() do{fprintf(stderr,"ABORT line%d\n",__LINE__);exit(3);}while(0)
static uint8_t rom[0x100000],ram[0x20000],before[0x20000];
typedef struct Bus {unsigned count,addr[16],val[16],bytes,service_request;uint8_t regs[0x10000],vram[0x10000];CpuState*c;} Bus;
static void original(Bus*);
uint8 cpu_read8(CpuState*c,uint8 bank,uint16 addr){assert(!bank&&addr<8192);c->open_bus=ram[addr];return c->open_bus;}
static void mmio(void*ctx,uint16_t q,uint8_t v){Bus*b=ctx;assert(b->count<16);b->addr[b->count]=q;b->val[b->count++]=v;b->regs[q]=v;
 if(q==0x420b){assert(v==2);unsigned src=b->regs[0x4312]|b->regs[0x4313]<<8,bank=b->regs[0x4314],n=b->regs[0x4315]|b->regs[0x4316]<<8,dest=(b->regs[0x2116]|b->regs[0x2117]<<8)*2;assert((bank==126||bank==127)&&n&&src+n<=65536&&dest+n<=65536);memcpy(b->vram+dest,ram+(bank-126)*0x10000+src,n);b->bytes+=n;if(b->c)b->c->master_cycles+=20+8*n;}}
static uint8_t rd(void*ctx,uint32_t a){(void)ctx;unsigned q=a&65535,b=a>>16;if(b<64&&q<8192)return ram[q];if(b==0&&q>=32768)return rom[q&32767];abort();}
static void wr(void*ctx,uint32_t a,uint8_t v){Bus*b=ctx;unsigned q=a&65535;if(q<8192){ram[q]=v;if(q==0x23a&&(v&32)&&b->service_request)original(b);return;}mmio(ctx,q,v);}
static void put(unsigned q,unsigned v,unsigned n){while(n--){ram[q++]=v;v>>=8;}}
static CpuState prepare(unsigned kind,unsigned n,unsigned own){
 for(unsigned q=0;q<sizeof ram;q++)ram[q]=(uint8_t)(q*109+own*73+n);
 CpuState c={0};c.P=0x25;c.A=0x5520;c.X=0x9876;c.Y=0x1234;c.S=0x1c0;c.DB=1;c.ram=ram;c.master_cycles=123456;cpu_p_to_mirrors(&c);
 unsigned totals[]={0x37c0,0x2a00,0x1f00,0x200},lasts[]={3,2,1,0},bases[]={0,0x4400,0x6000,0x4300};unsigned total=totals[kind],last=lasts[kind];
 put(c.S+1,n==last?0x0090d8:0x0090a1,3);put(c.S+6,0x0188df,3);put(c.S+9,kind?0x8892:0x887d,2);put(c.S+11,0x87d5,2);put(c.S+13,0x8732,2);put(c.S+15,0x0080e3,3);ram[c.S+18]=2;static const unsigned owners[]={0x02b73a,0x02b87d,0x02bc9b};put(c.S+19,owners[own],3);
 if(kind==2){put(c.S+6,0x0094e3,3);put(c.S+11,0x0097eb,3);put(c.S+14,0x018727,3);put(c.S+17,0x0080e3,3);ram[c.S+20]=2;put(c.S+21,owners[own],3);}
 if(kind==3){put(c.S+6,0x0094e3,3);put(c.S+11,0x018898,3);put(c.S+14,0x87d5,2);put(c.S+16,0x8732,2);put(c.S+18,0x0080e3,3);ram[c.S+21]=2;put(c.S+22,owners[own],3);}
 ram[0x23d]=128;ram[0x275]=0x81;ram[0x100e]=0;ram[0x23a]=32;ram[0x264]=0;put(0x238,0,2);put(8,0x7f7f80+n*0x1000,3);put(0x28a,bases[kind]+n*0x800,2);put(0x28c,n==last?total-n*0x1000:0x1000,2);put(0x1000,total-n*0x1000,2);return c;
}
static CpuState hud(unsigned kind,unsigned own){
 CpuState c=prepare(0,0,own);unsigned s=c.S;ram[0x23a]=32;put(0x238,0x400,2);put(8,kind?0x7e3000:0x7e4800,3);put(0x28a,0x3c00,2);put(0x28c,kind?0x800:0x600,2);
 const unsigned owners[]={0x02b73a,0x02b87d,0x02bc9b};
 if(!kind){put(s+1,0x0289b9,3);put(s+6,0x8993,2);put(s+10,0x8972,2);put(s+16,0x018ad0,3);put(s+19,0x8743,2);put(s+21,0x0080e3,3);ram[s+24]=2;put(s+25,owners[own],3);}
 else{put(s+1,0x018af5,3);put(s+4,0x8743,2);put(s+6,0x0080e3,3);ram[s+9]=2;put(s+10,owners[own],3);}
 for(unsigned q=kind?0x3000:0x4800;q<(kind?0x3800:0x4e00);q+=2)put(q,kind?0x01ff:0x3d8c,2);
 return c;
}
static void original(Bus*b){Interp816*i=interp816_init(b,rd,wr);assert(i);i->e=0;i->mf=1;i->xf=0;i->db=0;i->k=0;i->pc=0x884b;
 while(i->pc!=0x8883)interp816_runOpcode(i);
 i->pc=0x88be;while(i->pc!=0x88c9)interp816_runOpcode(i);
 interp816_free(i);
}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);unsigned positives=0,refusals=0;
 for(unsigned seed=0;seed<4;seed++)for(unsigned kind=0;kind<4;kind++)for(unsigned n=0;n<(kind==0?4:kind==1?3:kind==2?2:1);n++)for(unsigned own=0;own<3;own++){
  CpuState c=prepare(kind,n,own);c.A=(uint16_t)(seed*0x5555);c.X=(uint16_t)(seed*0x3333);c.Y=(uint16_t)(seed*0x7777);c.P=0x24|(seed&1)|(seed&2?0xc0:0);cpu_p_to_mirrors(&c);CpuState saved=c;memcpy(before,ram,sizeof ram);static Bus fast,old;memset(&fast,0,sizeof fast);memset(&old,0,sizeof old);fast.c=&c;LagoonMenuDmaAuthority a={1,0,1};assert(LagoonMenuDma_Service(rom,sizeof rom,ram,&c,0x89ac,a,mmio,&fast));unsigned length=fast.bytes;assert(length&&c.master_cycles==saved.master_cycles+20+8*length);c.master_cycles=saved.master_cycles;assert(!memcmp(&c,&saved,sizeof c));static uint8_t after[0x20000];memcpy(after,ram,sizeof ram);memcpy(ram,before,sizeof ram);original(&old);assert(!memcmp(ram,after,sizeof ram));assert(fast.count==10&&old.count==10&&!memcmp(fast.addr,old.addr,sizeof fast.addr)&&!memcmp(fast.val,old.val,sizeof fast.val)&&!memcmp(fast.vram,old.vram,sizeof fast.vram)&&old.bytes==fast.bytes);positives++;
  for(unsigned bad=0;bad<52;bad++){
   c=prepare(kind,n,own);a=(LagoonMenuDmaAuthority){1,0,1};uint32_t pc=0x89ac;size_t size=sizeof rom;
   unsigned skip=0;
   if(bad<22){
    static const unsigned oldbytes[]={1,2,3,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21};
    static const unsigned common9[]={1,2,3,6,7,8,11,12,13,14,15,16,17,18,19,20,21,22,23};
    static const unsigned common0[]={1,2,3,6,7,8,11,12,13,14,15,16,17,18,19,20,21,22,23,24};
    const unsigned *offset=kind<2?oldbytes:kind==2?common9:common0;
    unsigned count=kind==3?20:19;
    if(bad>=count)skip=1;else ram[c.S+offset[bad]]^=0x80;
   }
   else switch(bad){case 22:a.forced_blank=0;break;case 23:a.hdma=1;break;case 24:a.idle=0;break;case 25:pc++;break;case 26:c.PB=1;break;case 27:c.DB=0;break;case 28:c.m_flag=0;break;case 29:c.x_flag=1;break;case 30:c.D=1;break;case 31:c.P|=8;break;case 32:c._flag_D=1;break;case 33:c.emulation=1;break;case 34:ram[0x23d]=0;break;case 35:ram[0x275]=0;break;case 36:ram[0x100e]=1;break;case 37:ram[0x23a]=0;break;case 38:ram[0x23a]|=64;break;case 39:ram[0x238]=4;break;case 40:ram[0x264]=1;break;case 41:ram[8]++;break;case 42:ram[10]=126;break;case 43:ram[0x28a]++;break;case 44:ram[0x28c]++;break;case 45:ram[0x1000]++;break;case 46:size--;break;case 47:c.S=0xff;break;case 48:c.S=0x1ea;break;case 49:rom[0x84b]^=1;break;case 50:rom[0x108a]^=1;break;case 51:rom[0x8869]^=1;break;}
   /* S+4/5 is the caller's savedX, deliberately opaque and unconstrained. */
   if(skip)continue;
   saved=c;memcpy(before,ram,sizeof ram);static Bus untouched;memset(&untouched,0,sizeof untouched);assert(!LagoonMenuDma_Service(rom,size,ram,&c,pc,a,mmio,&untouched));assert(!untouched.count&&!memcmp(before,ram,sizeof ram)&&!memcmp(&c,&saved,sizeof c));refusals++;
   if(bad==49)rom[0x84b]^=1;if(bad==50)rom[0x108a]^=1;if(bad==51)rom[0x8869]^=1;
  }
 }
 unsigned entries=0,entry_refusals=0;
 for(unsigned kind=0;kind<4;kind++)for(unsigned n=0;n<(kind==0?4:kind==1?3:kind==2?2:1);n++)for(unsigned seed=0;seed<4;seed++){
  CpuState c=prepare(kind,n,seed%3);ram[0x23a]=0;c.host_return_valid=3;c.P=0x24|(seed&1)|(seed&2?0xc0:0);cpu_p_to_mirrors(&c);CpuState initial=c;unsigned ret=ram[c.S+1]|ram[c.S+2]<<8;memcpy(before,ram,sizeof ram);static Bus fast;memset(&fast,0,sizeof fast);fast.c=&c;RecompReturn out=123;LagoonMenuDmaAuthority a={1,0,1};
  assert(LagoonMenuDma_Run(rom,sizeof rom,ram,&c,c.S,3,ret+1,&out,a,mmio,&fast));assert(out==RECOMP_RETURN_NORMAL&&c.master_cycles==initial.master_cycles+20+8*fast.bytes+176&&c.cycles==initial.cycles+22&&c.coprocessor_master_cycles==c.master_cycles-48&&c.open_bus==0);static uint8_t after[0x20000];memcpy(after,ram,sizeof ram);memcpy(ram,before,sizeof ram);static Bus old;memset(&old,0,sizeof old);old.service_request=1;
  Interp816*i=interp816_init(&old,rd,wr);assert(i);i->a=initial.A;i->x=initial.X;i->y=initial.Y;i->sp=initial.S;i->dp=initial.D;i->db=initial.DB;i->k=initial.PB;i->pc=0x89a7;i->mf=1;i->xf=0;i->e=0;i->n=initial._flag_N;i->z=initial._flag_Z;i->v=initial._flag_V;i->c=initial._flag_C;i->i=initial._flag_I;i->d=initial._flag_D;
  unsigned ops=0;while(i->pc!=ret+1){assert(++ops<20);interp816_runOpcode(i);}
  assert(!memcmp(ram,after,sizeof ram)&&i->a==c.A&&i->x==c.X&&i->y==c.Y&&i->sp==c.S&&i->db==c.DB&&i->dp==c.D&&i->k==c.PB&&interp816_getFlags(i)==c.P);assert(!memcmp(old.addr,fast.addr,sizeof old.addr)&&!memcmp(old.val,fast.val,sizeof old.val)&&!memcmp(old.vram,fast.vram,sizeof old.vram));interp816_free(i);entries++;
  for(unsigned bad=0;bad<7;bad++){
   c=prepare(kind,n,seed%3);ram[0x23a]=0;c.host_return_valid=3;unsigned s=c.S,hrv=3,host=(ram[s+1]|ram[s+2]<<8)+1;
   switch(bad){case 0:s++;break;case 1:hrv=0;break;case 2:hrv=2;break;case 3:c.host_return_valid=0;break;case 4:host++;break;case 5:ram[0x23a]=32;break;case 6:ram[c.S+1]++;break;}
   initial=c;memcpy(before,ram,sizeof ram);static Bus none;memset(&none,0,sizeof none);out=123;assert(!LagoonMenuDma_Run(rom,sizeof rom,ram,&c,s,hrv,host,&out,a,mmio,&none));assert(out==123&&!none.count&&!memcmp(before,ram,sizeof ram)&&!memcmp(&initial,&c,sizeof c));entry_refusals++;
  }
 }
 /* New common ownership pins are not trusted just because the old consumer
  * matches. Mutate every producer/table byte and the last outer stack byte. */
 unsigned common_refusals=0;
 for(unsigned kind=2;kind<4;kind++) {
  const unsigned regions[][2]={{0x14a1,0x45},{kind==2?0x1525:0x14e6,7},{0x8703,kind==2?0x31:0},{0x17e6,kind==2?6:0}};
  for(unsigned region=0;region<4;region++)for(unsigned byte=0;byte<regions[region][1];byte++) {
   CpuState c=prepare(kind,0,0);CpuState initial=c;memcpy(before,ram,sizeof ram);
   unsigned offset=regions[region][0]+byte;rom[offset]^=1;static Bus none;memset(&none,0,sizeof none);
   assert(!LagoonMenuDma_Service(rom,sizeof rom,ram,&c,0x89ac,(LagoonMenuDmaAuthority){1,0,1},mmio,&none));
   assert(!none.count&&!memcmp(before,ram,sizeof ram)&&!memcmp(&initial,&c,sizeof c));rom[offset]^=1;common_refusals++;
  }
  CpuState c=prepare(kind,0,0);unsigned old=c.S;uint8_t stack[25];memcpy(stack,ram+old+1,25);c.S=kind==2?0x1e9:0x1e8;memcpy(ram+c.S+1,stack,25);CpuState initial=c;memcpy(before,ram,sizeof ram);static Bus none;memset(&none,0,sizeof none);
  assert(!LagoonMenuDma_Service(rom,sizeof rom,ram,&c,0x89ac,(LagoonMenuDmaAuthority){1,0,1},mmio,&none));assert(!none.count&&!memcmp(before,ram,sizeof ram)&&!memcmp(&initial,&c,sizeof c));common_refusals++;
 }
 unsigned hud_cases=0,hud_refusals=0;
 for(unsigned kind=0;kind<2;kind++)for(unsigned own=0;own<3;own++){
  CpuState c=hud(kind,own),initial=c;memcpy(before,ram,sizeof ram);static Bus fast,old;memset(&fast,0,sizeof fast);memset(&old,0,sizeof old);fast.c=&c;
  assert(LagoonMenuDma_Service(rom,sizeof rom,ram,&c,0x89ac,(LagoonMenuDmaAuthority){1,0,1},mmio,&fast));assert(c.master_cycles==initial.master_cycles+20+8*fast.bytes);static uint8_t after[0x20000];memcpy(after,ram,sizeof ram);memcpy(ram,before,sizeof ram);original(&old);assert(!memcmp(ram,after,sizeof ram)&&!memcmp(old.addr,fast.addr,sizeof old.addr)&&!memcmp(old.val,fast.val,sizeof old.val)&&!memcmp(old.vram,fast.vram,sizeof old.vram));hud_cases++;
  for(unsigned byte=0;byte<(kind?0x800:0x600);byte++){
   c=hud(kind,own);ram[(kind?0x3000:0x4800)+byte]^=1;initial=c;memcpy(before,ram,sizeof ram);static Bus none;memset(&none,0,sizeof none);
   assert(!LagoonMenuDma_Service(rom,sizeof rom,ram,&c,0x89ac,(LagoonMenuDmaAuthority){1,0,1},mmio,&none));assert(!none.count&&!memcmp(before,ram,sizeof ram)&&!memcmp(&initial,&c,sizeof c));hud_refusals++;
  }
  const unsigned f0[]={1,2,3,6,7,10,11,16,17,18,19,20,21,22,23,24,25,26,27};
  const unsigned f1[]={1,2,3,4,5,6,7,8,9,10,11,12};
  for(unsigned n=0;n<(kind?12:19)+9;n++){
   c=hud(kind,own);LagoonMenuDmaAuthority auth={1,0,1};unsigned addr=0x89ac;size_t size=sizeof rom;
   unsigned fields=kind?12:19;
   if(n<fields)ram[c.S+(kind?f1[n]:f0[n])]^=64;
   else switch(n-fields){case 0:ram[0x238]++;break;case 1:ram[0x28a]++;break;case 2:ram[0x28c]++;break;case 3:auth.forced_blank=0;break;case 4:auth.hdma=1;break;case 5:auth.idle=0;break;case 6:ram[0x23a]|=64;break;case 7:ram[0x264]=2;break;case 8:size--;break;}
   initial=c;memcpy(before,ram,sizeof ram);static Bus none;memset(&none,0,sizeof none);
   assert(!LagoonMenuDma_Service(rom,size,ram,&c,addr,auth,mmio,&none));assert(!none.count&&!memcmp(before,ram,sizeof ram)&&!memcmp(&initial,&c,sizeof c));hud_refusals++;
  }
  if(!kind){c=hud(kind,own);ram[0x23a]=0;c.host_return_valid=3;initial=c;memcpy(before,ram,sizeof ram);static Bus native;memset(&native,0,sizeof native);native.c=&c;RecompReturn out=123;
   assert(LagoonMenuDma_Run(rom,sizeof rom,ram,&c,c.S,3,0x0289ba,&out,(LagoonMenuDmaAuthority){1,0,1},mmio,&native));assert(out==RECOMP_RETURN_NORMAL&&c.S==initial.S+3&&c.master_cycles==initial.master_cycles+20+8*1536+176&&c.open_bus==2&&ram[0x238]==0&&ram[0x239]==4);
   memcpy(after,ram,sizeof ram);memcpy(ram,before,sizeof ram);static Bus oracle;memset(&oracle,0,sizeof oracle);oracle.service_request=1;
   Interp816*i=interp816_init(&oracle,rd,wr);assert(i);i->a=initial.A;i->x=initial.X;i->y=initial.Y;i->sp=initial.S;i->dp=initial.D;i->db=initial.DB;i->k=0;i->pc=0x89a7;i->mf=1;i->xf=0;i->e=0;i->n=initial._flag_N;i->z=initial._flag_Z;i->v=initial._flag_V;i->c=initial._flag_C;i->i=initial._flag_I;i->d=initial._flag_D;
   unsigned ops=0;while(i->pc!=0x89ba){assert(++ops<20);interp816_runOpcode(i);}
   assert(!memcmp(ram,after,sizeof ram)&&i->a==c.A&&i->x==c.X&&i->y==c.Y&&i->sp==c.S&&i->db==c.DB&&i->dp==c.D&&i->k==2&&interp816_getFlags(i)==c.P);assert(!memcmp(oracle.addr,native.addr,sizeof native.addr)&&!memcmp(oracle.val,native.val,sizeof native.val)&&!memcmp(oracle.vram,native.vram,sizeof native.vram));interp816_free(i);hud_cases++;
  }
 }
 printf("PASS%u HUD original-consumer/native cases,%u HUD source/owner/authority readonly refusals\n",hud_cases,hud_refusals);
 printf("PASS%u common producer/table/stack readonly refusals\n",common_refusals);
 printf("PASS%u original-consumer cases/%u readonly refusals; %u original89A7..89B3 fullRAM/CPU/stack/MMIO entries/%u entry refusals\n",positives,refusals,entries,entry_refusals);return 0;
}
