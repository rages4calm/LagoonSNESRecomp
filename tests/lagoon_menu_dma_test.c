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
 if(q==0x420b){assert(v==2);unsigned src=b->regs[0x4312]|b->regs[0x4313]<<8,bank=b->regs[0x4314],n=b->regs[0x4315]|b->regs[0x4316]<<8,dest=(b->regs[0x2116]|b->regs[0x2117]<<8)*2;assert(bank==127&&n&&src+n<=65536&&dest+n<=65536);memcpy(b->vram+dest,ram+0x10000+src,n);b->bytes+=n;if(b->c)b->c->master_cycles+=20+8*n;}}
static uint8_t rd(void*ctx,uint32_t a){(void)ctx;unsigned q=a&65535,b=a>>16;if(b<64&&q<8192)return ram[q];if(b==0&&q>=32768)return rom[q&32767];abort();}
static void wr(void*ctx,uint32_t a,uint8_t v){Bus*b=ctx;unsigned q=a&65535;if(q<8192){ram[q]=v;if(q==0x23a&&(v&32)&&b->service_request)original(b);return;}mmio(ctx,q,v);}
static void put(unsigned q,unsigned v,unsigned n){while(n--){ram[q++]=v;v>>=8;}}
static CpuState prepare(unsigned kind,unsigned n,unsigned own){
 for(unsigned q=0;q<sizeof ram;q++)ram[q]=(uint8_t)(q*109+own*73+n);
 CpuState c={0};c.P=0x25;c.A=0x5520;c.X=0x9876;c.Y=0x1234;c.S=0x1c0;c.DB=1;c.ram=ram;c.master_cycles=123456;cpu_p_to_mirrors(&c);
 unsigned total=kind?0x2a00:0x37c0,last=kind?2:3;
 put(c.S+1,n==last?0x0090d8:0x0090a1,3);put(c.S+6,0x0188df,3);put(c.S+9,kind?0x8892:0x887d,2);put(c.S+11,0x87d5,2);put(c.S+13,0x8732,2);put(c.S+15,0x0080e3,3);ram[c.S+18]=2;static const unsigned owners[]={0x02b73a,0x02b87d,0x02bc9b};put(c.S+19,owners[own],3);
 ram[0x23d]=128;ram[0x275]=0x81;ram[0x100e]=0;ram[0x23a]=32;ram[0x264]=0;put(0x238,0,2);put(8,0x7f7f80+n*0x1000,3);put(0x28a,(kind?0x4400:0)+n*0x800,2);put(0x28c,n==last?total-n*0x1000:0x1000,2);put(0x1000,total-n*0x1000,2);return c;
}
static void original(Bus*b){Interp816*i=interp816_init(b,rd,wr);assert(i);i->e=0;i->mf=1;i->xf=0;i->db=0;i->k=0;i->pc=0x884b;
 while(i->pc!=0x8883)interp816_runOpcode(i);
 i->pc=0x88be;while(i->pc!=0x88c9)interp816_runOpcode(i);
 interp816_free(i);
}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);unsigned positives=0,refusals=0;
 for(unsigned seed=0;seed<4;seed++)for(unsigned kind=0;kind<2;kind++)for(unsigned n=0;n<(kind?3:4);n++)for(unsigned own=0;own<3;own++){
  CpuState c=prepare(kind,n,own);c.A=(uint16_t)(seed*0x5555);c.X=(uint16_t)(seed*0x3333);c.Y=(uint16_t)(seed*0x7777);c.P=0x24|(seed&1)|(seed&2?0xc0:0);cpu_p_to_mirrors(&c);CpuState saved=c;memcpy(before,ram,sizeof ram);static Bus fast,old;memset(&fast,0,sizeof fast);memset(&old,0,sizeof old);fast.c=&c;LagoonMenuDmaAuthority a={1,0,1};assert(LagoonMenuDma_Service(rom,sizeof rom,ram,&c,0x89ac,a,mmio,&fast));unsigned length=fast.bytes;assert(length&&c.master_cycles==saved.master_cycles+20+8*length);c.master_cycles=saved.master_cycles;assert(!memcmp(&c,&saved,sizeof c));static uint8_t after[0x20000];memcpy(after,ram,sizeof ram);memcpy(ram,before,sizeof ram);original(&old);assert(!memcmp(ram,after,sizeof ram));assert(fast.count==10&&old.count==10&&!memcmp(fast.addr,old.addr,sizeof fast.addr)&&!memcmp(fast.val,old.val,sizeof fast.val)&&!memcmp(fast.vram,old.vram,sizeof fast.vram)&&old.bytes==fast.bytes);positives++;
  for(unsigned bad=0;bad<52;bad++){
   c=prepare(kind,n,own);a=(LagoonMenuDmaAuthority){1,0,1};uint32_t pc=0x89ac;size_t size=sizeof rom;
   if(bad<22)ram[c.S+1+bad]^=0x80;
   else switch(bad){case 22:a.forced_blank=0;break;case 23:a.hdma=1;break;case 24:a.idle=0;break;case 25:pc++;break;case 26:c.PB=1;break;case 27:c.DB=0;break;case 28:c.m_flag=0;break;case 29:c.x_flag=1;break;case 30:c.D=1;break;case 31:c.P|=8;break;case 32:c._flag_D=1;break;case 33:c.emulation=1;break;case 34:ram[0x23d]=0;break;case 35:ram[0x275]=0;break;case 36:ram[0x100e]=1;break;case 37:ram[0x23a]=0;break;case 38:ram[0x23a]|=64;break;case 39:ram[0x238]=4;break;case 40:ram[0x264]=1;break;case 41:ram[8]++;break;case 42:ram[10]=126;break;case 43:ram[0x28a]++;break;case 44:ram[0x28c]++;break;case 45:ram[0x1000]++;break;case 46:size--;break;case 47:c.S=0xff;break;case 48:c.S=0x1ea;break;case 49:rom[0x84b]^=1;break;case 50:rom[0x108a]^=1;break;case 51:rom[0x8869]^=1;break;}
   /* S+4/5 is the caller's savedX, deliberately opaque and unconstrained. */
   if(bad==3||bad==4||bad==21){if(bad>=49)abort();continue;}
   saved=c;memcpy(before,ram,sizeof ram);static Bus untouched;memset(&untouched,0,sizeof untouched);assert(!LagoonMenuDma_Service(rom,size,ram,&c,pc,a,mmio,&untouched));assert(!untouched.count&&!memcmp(before,ram,sizeof ram)&&!memcmp(&c,&saved,sizeof c));refusals++;
   if(bad==49)rom[0x84b]^=1;if(bad==50)rom[0x108a]^=1;if(bad==51)rom[0x8869]^=1;
  }
 }
 unsigned entries=0,entry_refusals=0;
 for(unsigned kind=0;kind<2;kind++)for(unsigned n=0;n<(kind?3:4);n++)for(unsigned seed=0;seed<4;seed++){
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
 printf("PASS%u original-consumer cases/%u readonly refusals; %u original89A7..89B3 fullRAM/CPU/stack/MMIO entries/%u entry refusals\n",positives,refusals,entries,entry_refusals);return 0;
}
