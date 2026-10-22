#define LAGOON_NATIVE_SCRIPTS53_TEST 1
#include "../src/lagoon_native_scripts53.c"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static uint8_t rom[0x100000],ram[0x20000],snapshot[0x20000],native_ram[0x20000];
static uint64_t deadline,bus_extra;
static unsigned cases,cuts,refusals,opcodes,prefixes;
static uint8_t oracle_bus;
uint32_t cpu_region_speed(uint32_t address){(void)address;return 8;}
void cpu_instruction_timing_commit(CpuState *c){c->coprocessor_master_cycles=c->master_cycles;}
int interp_bridge_lle_master_deadline_reached(const CpuState *c){return deadline&&c->master_cycles>=deadline;}
int interp816_opcode_hook(uint32_t pc){(void)pc;return 0;}
static uint8_t value(uint32_t address){
    unsigned bank=address>>16,a=address&65535;
    if(bank==126||bank==127)return ram[(bank-126)*65536+a];
    if(bank<64&&a<8192)return ram[a];
    assert(bank<32&&a>=32768);return rom[bank*32768+(a&32767)];
}
uint8_t cpu_read8(CpuState *c,uint8_t bank,uint16_t a){return c->open_bus=value((unsigned)bank<<16|a);}
void cpu_write8(CpuState *c,uint8_t bank,uint16_t a,uint8_t v){assert(bank<64&&a<8192);ram[a]=v;c->open_bus=v;}
void cpu_write16(CpuState *c,uint8_t bank,uint16_t a,uint16_t v){cpu_write8(c,bank,a,v);cpu_write8(c,bank,a+1,v>>8);}
static uint8_t rd(void *data,uint32_t address){(void)data;bus_extra+=2;return oracle_bus=value(address);}
static void wr(void *data,uint32_t address,uint8_t v){(void)data;bus_extra+=2;oracle_bus=v;assert((address>>16)<64&&(address&65535)<8192);ram[address&65535]=v;}
static void from_cpu(Interp816 *i,const CpuState *c,uint32_t pc){
    i->a=c->A;i->x=c->X;i->y=c->Y;i->sp=c->S;i->dp=c->D;i->db=c->DB;i->k=c->PB;i->pc=pc;
    interp816_setFlags(i,c->P);i->e=c->emulation;
}
static void to_cpu(CpuState *c,const Interp816 *i){
    c->A=i->a;c->X=i->x;c->Y=i->y;c->S=i->sp;c->D=i->dp;c->DB=i->db;c->PB=i->k;
    c->P=interp816_getFlags((Interp816 *)i);cpu_p_to_mirrors(c);
}
static void step(Interp816 *i,CpuState *c){
    bus_extra=0;unsigned clocks=interp816_runOpcode(i);++opcodes;
    c->cycles+=clocks;c->master_cycles+=clocks*6+bus_extra;c->coprocessor_master_cycles=c->master_cycles;c->open_bus=oracle_bus;to_cpu(c,i);
}
static uint32_t pc_of(const Interp816 *i){return (unsigned)i->k<<16|i->pc;}
static void setup(CpuState *c,Interp816 *i,unsigned family,unsigned index,unsigned flags,unsigned stack){
    memset(&witness,0,sizeof witness);deadline=0;
    for(unsigned n=0;n<sizeof ram;++n)ram[n]=(uint8_t)(n*73+cases*17);
    memset(c,0,sizeof *c);c->ram=ram;c->PB=c->DB=1;c->S=stack;c->Y=0x340;c->X=0x1223;c->A=0x37;
    c->P=0x24|((flags&1)?1:0)|((flags&2)?2:0)|((flags&4)?0x40:0)|((flags&8)?0x80:0);cpu_p_to_mirrors(c);
    ram[0x9c]=family;ram[0x9d]=index;ram[0x48e]=index;ram[0x48f]=0;
    ram[0x500+c->Y]=0xc8;ram[0x518+c->Y]=1;ram[0x51b+c->Y]=index;
    static const unsigned calls[]={0x01809a,0x0180a0,0x018784};
    uint32_t landing=0;CpuState before=*c;
    assert(!LagoonNativeScripts53_Process(rom,sizeof rom,c,c,7,7,calls[family],&landing));assert(witness.phase==1);assert(!memcmp(c,&before,sizeof before));
    /* Execute the actual source JSR, and then the real selector/80E9 dispatcher.
     * NPC/event work before the shared tail is outside this finite test scope;
     * its qualified header inputs above are explicit, not simulated results. */
    from_cpu(i,c,calls[family]);step(i,c);assert(c->S==stack-2);
    from_cpu(i,c,0x01cc7b);
    unsigned count=0;while(pc_of(i)!=headers[family]){assert(++count<70);step(i,c);}
    assert(c->S==stack-2&&live(c,7));++prefixes;
}
static uint32_t compare(CpuState *c,Interp816 *i,uint32_t start,unsigned cut){
    CpuState original=*c;memcpy(snapshot,ram,sizeof ram);deadline=cut?c->master_cycles+cut:0;
    uint32_t landing=0;assert(LagoonNativeScripts53_Process(rom,sizeof rom,c,c,7,7,start,&landing));
    memcpy(native_ram,ram,sizeof ram);memcpy(ram,snapshot,sizeof ram);from_cpu(i,&original,start);
    unsigned count=0;while(original.cycles<c->cycles){if(deadline&&original.master_cycles>=deadline)break;assert(++count<40);step(i,&original);}
    if(pc_of(i)!=landing||c->A!=original.A||c->X!=original.X||c->Y!=original.Y||c->P!=original.P||c->open_bus!=original.open_bus||c->cycles!=original.cycles||c->master_cycles!=original.master_cycles){
        fprintf(stderr,"case %u start %06X cut %u land %06X/%06X A%04X/%04X X%04X/%04X Y%04X/%04X P%02X/%02X bus%02X/%02X cycles%llu/%llu master%llu/%llu\n",cases,start,cut,landing,pc_of(i),c->A,original.A,c->X,original.X,c->Y,original.Y,c->P,original.P,c->open_bus,original.open_bus,(unsigned long long)c->cycles,(unsigned long long)original.cycles,(unsigned long long)c->master_cycles,(unsigned long long)original.master_cycles);exit(2);
    }
    assert(c->S==original.S&&c->DB==original.DB&&c->PB==original.PB&&c->D==original.D);
    assert(c->m_flag==original.m_flag&&c->x_flag==original.x_flag&&c->emulation==original.emulation&&c->coprocessor_master_cycles==original.coprocessor_master_cycles);
    assert(!memcmp(native_ram,ram,sizeof ram));memcpy(ram,native_ram,sizeof ram);++cases;if(cut)++cuts;return landing;
}
static void header_case(unsigned family,unsigned index,unsigned flags,unsigned stack,unsigned cut){
    CpuState c;Interp816 *i=interp816_init(NULL,rd,wr);assert(i);setup(&c,i,family,index,flags,stack);
    uint32_t end=compare(&c,i,headers[family],cut);
    if(!cut)assert(end==(family?0x01cccb:0x01cc9a));interp816_free(i);
}
static void to_fetch(CpuState *c,Interp816 *i,unsigned family,unsigned index,unsigned flags){
    setup(c,i,family,index,flags,0x1d0);uint32_t landing=0;
    assert(LagoonNativeScripts53_Process(rom,sizeof rom,c,c,7,7,headers[family],&landing));
    if(!family){assert(landing==0x01cc9a);from_cpu(i,c,landing);unsigned n=0;while(pc_of(i)!=0x01cccb){assert(++n<6);step(i,c);}}
    else assert(landing==0x01cccb);
    assert(witness.phase==2&&word(ram+0xa1)==witness.pointer&&ram[0xa3]==5);
}
static void fetch_case(unsigned family,unsigned command,unsigned flags,unsigned start_kind,unsigned cut){
    CpuState c;Interp816 *i=interp816_init(NULL,rd,wr);assert(i);to_fetch(&c,i,family,0,flags);
    unsigned start=0x01cccb;
    if(start_kind){witness.phase=3;c.Y=start_kind==1?3:0;start=start_kind==1?0x01ccd8:0x01ccce;}
    unsigned pos=offset(0x050000|witness.pointer)+(start_kind==1?3:0);uint8_t old=rom[pos];rom[pos]=command;
    assert(LagoonNativeScripts53_Source(rom,sizeof rom));
    uint32_t end=compare(&c,i,start,cut);
    if(!cut)assert(end==(command==255?0x01ccda:0x01ccd5));
    rom[pos]=old;interp816_free(i);
}
static void refusal(unsigned bad){
    CpuState c;Interp816 *i=interp816_init(NULL,rd,wr);assert(i);to_fetch(&c,i,0,0,0);
    uint32_t pc=0x01cccb,landing=0;uint64_t gen=7,frame=7;const CpuState *current=&c;unsigned changed=0;uint8_t old=0;
    switch(bad){
    case 0:c.PB=2;break;case 1:c.DB=2;break;case 2:c.D=1;break;case 3:c.emulation=1;break;
    case 4:c.m_flag=0;break;case 5:c.x_flag=1;break;case 6:c._flag_D=1;break;case 7:c._flag_I=0;break;
    case 8:c.P^=8;break;case 9:c.S++;break;case 10:c.S=0x100;break;case 11:c.S=0x1f7;break;
    case 12:gen++;break;case 13:frame++;break;case 14:current=NULL;break;case 15:witness.phase=0;break;
    case 16:c.ram=snapshot;break;case 17:ram[0xa1]^=1;break;case 18:ram[0xa2]^=1;break;case 19:ram[0xa3]=6;break;
    case 20:pc=0x01ccce;break;case 21:pc=0x01ccd8;break;
    case 22:witness.phase=3;c.Y=0xffff;pc=0x01ccd8;break;
    case 23:changed=offset(0x050000|witness.pointer);old=rom[changed];rom[changed]=40;break;
    case 24:changed=offset(0x050000|witness.pointer);old=rom[changed];rom[changed]=254;break;
    default:
        if(bad<35)ram[c.S+1+bad-25]^=1;
        else {unsigned s=(bad-35)/2;changed=offset(source_spans[s].pc)+(bad&1?source_spans[s].length-1:0);old=rom[changed];rom[changed]^=1;}
        break;
    }
    CpuState before=c;memcpy(native_ram,ram,sizeof ram);
    assert(!LagoonNativeScripts53_Process(rom,sizeof rom,&c,current,gen,frame,pc,&landing));
    assert(!memcmp(&c,&before,sizeof c)&&!memcmp(native_ram,ram,sizeof ram));++refusals;
    if(changed)rom[changed]=old;interp816_free(i);
}
static void header_refusal(unsigned family,unsigned bad){
    CpuState c;Interp816 *i=interp816_init(NULL,rd,wr);assert(i);setup(&c,i,family,0,0,0x1d0);
    switch(bad){
    case 0:c.A|=0x100;break;case 1:ram[0x9c]=(family+1)%3;break;
    case 2:if(family==2)ram[0x48e]=counts[family];else ram[0x9d]=counts[family];break;
    case 3:c.Y=0;break;case 4:c.Y=0x801;break;case 5:c.Y=0x41;break;
    case 6:ram[0x500+c.Y]^=8;break;case 7:ram[0x518+c.Y]=2;break;case 8:ram[0x51b+c.Y]++;break;
    default:assert(0);
    }
    CpuState before=c;memcpy(native_ram,ram,sizeof ram);uint32_t landing=0;
    assert(!LagoonNativeScripts53_Process(rom,sizeof rom,&c,&c,7,7,headers[family],&landing));
    assert(!memcmp(&c,&before,sizeof c)&&!memcmp(native_ram,ram,sizeof ram));++refusals;interp816_free(i);
}
static void upper_stack_case(unsigned family,unsigned stack){
    CpuState c;Interp816 *i=interp816_init(NULL,rd,wr);assert(i);setup(&c,i,family,0,0,stack);
    assert(witness.stack_size==(stack==0x1f8?9:10));
    /* Source-executed native headers must admit the observed real callers. */
    assert(compare(&c,i,headers[family],0)==(family?0x01cccb:0x01cc9a));
    interp816_free(i);
    for(unsigned n=0;n<(stack==0x1f8?9:10);++n){
        i=interp816_init(NULL,rd,wr);setup(&c,i,family,0,0,stack);ram[c.S+1+n]^=1;
        CpuState before=c;memcpy(native_ram,ram,sizeof ram);uint32_t landing=0;
        assert(!LagoonNativeScripts53_Process(rom,sizeof rom,&c,&c,7,7,headers[family],&landing));
        assert(!memcmp(&c,&before,sizeof c)&&!memcmp(native_ram,ram,sizeof ram));++refusals;interp816_free(i);
    }
    /*0200 is not part of the physical stack and must not qualify ownership. */
    if(stack==0x1f8){i=interp816_init(NULL,rd,wr);setup(&c,i,family,0,0,stack);ram[0x200]^=1;assert(compare(&c,i,headers[family],0)==(family?0x01cccb:0x01cc9a));interp816_free(i);}
}
int main(int argc,char **argv){
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);assert(LagoonNativeScripts53_Source(rom,sizeof rom));
    for(unsigned family=0;family<3;++family)for(unsigned index=0;index<counts[family];++index)for(unsigned flags=0;flags<16;++flags)header_case(family,index,flags,(flags&1)?0x106:0x1f5,0);
    for(unsigned family=0;family<3;++family)for(unsigned cut=1;cut<=550;++cut)header_case(family,counts[family]-1,cut&15,0x1d0,cut);
    for(unsigned family=0;family<3;++family)for(unsigned command=0;command<=40;++command)for(unsigned kind=0;kind<3;++kind)for(unsigned flags=0;flags<16;++flags)fetch_case(family,command==40?255:command,flags,kind,0);
    for(unsigned family=0;family<3;++family)for(unsigned command=0;command<2;++command)for(unsigned kind=0;kind<3;++kind)for(unsigned cut=1;cut<=160;++cut)fetch_case(family,command?255:7,cut&15,kind,cut);
    for(unsigned bad=0;bad<35+22;++bad)refusal(bad);
    for(unsigned family=0;family<3;++family)for(unsigned bad=0;bad<(family?3:9);++bad)header_refusal(family,bad);
    for(unsigned family=0;family<3;++family){upper_stack_case(family,0x1f6);upper_stack_case(family,0x1f8);}
    printf("PASS native scripts53: %u actual-opcode windows, %u master-deadline cuts, %u context/source refusals, %u real JSR/dispatch prefixes, %u original opcodes; full WRAM/CPU/openbus/clocks exact; CC9A/child JSR/terminal RTS preserved\n",cases,cuts,refusals,prefixes,opcodes);return 0;
}
