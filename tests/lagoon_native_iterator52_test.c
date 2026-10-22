#define LAGOON_NATIVE_ITERATOR52_TEST 1
#include "../src/lagoon_native_iterator52.c"
#include "snes/interp816.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static uint8_t rom[0x100000],ram[0x20000],snapshot[0x20000],native_ram[0x20000];
static uint64_t deadline, bus_extra;
static unsigned cases, interruptions, refusals, opcodes;
static uint8_t oracle_open_bus;
uint32_t cpu_region_speed(uint32_t address) { (void)address;return 8; }
void cpu_instruction_timing_commit(CpuState *c) { c->coprocessor_master_cycles=c->master_cycles; }
int interp_bridge_lle_master_deadline_reached(const CpuState *c) { return deadline && c->master_cycles>=deadline; }
int interp816_opcode_hook(uint32_t address) { (void)address;return 0; }
static uint8_t read_value(uint32_t address) {
    unsigned bank=address>>16,a=address&65535;
    if (bank==126 || bank==127) return ram[(bank-126)*65536+a];
    if (bank<64 && a<8192) return ram[a];
    assert(bank<32 && a>=32768);return rom[bank*32768+(a&32767)];
}
uint8_t cpu_read8(CpuState *c,uint8_t bank,uint16_t address) { return c->open_bus=read_value((unsigned)bank<<16|address); }
static uint8_t rd(void *data,uint32_t address) { (void)data;bus_extra+=2;return oracle_open_bus=read_value(address); }
static void wr(void *data,uint32_t address,uint8_t value) {
    (void)data;bus_extra+=2;oracle_open_bus=value;
    unsigned bank=address>>16,a=address&65535;
    assert(bank<64 && a<8192);ram[a]=value;
}
static void from_cpu(Interp816 *i,const CpuState *c,uint32_t pc) {
    i->a=c->A;i->x=c->X;i->y=c->Y;i->sp=c->S;i->dp=c->D;i->db=c->DB;i->k=c->PB;i->pc=pc;
    interp816_setFlags(i,c->P);i->e=c->emulation;
}
static void to_cpu(CpuState *c,const Interp816 *i) {
    c->A=i->a;c->X=i->x;c->Y=i->y;c->S=i->sp;c->D=i->dp;c->DB=i->db;c->PB=i->k;
    c->P=interp816_getFlags((Interp816 *)i);cpu_p_to_mirrors(c);
}
static void step(Interp816 *i,CpuState *c) {
    bus_extra=0;
    unsigned cycles=interp816_runOpcode(i);++opcodes;
    c->cycles+=cycles;c->master_cycles+=cycles*6+bus_extra;
    c->coprocessor_master_cycles=c->master_cycles;c->open_bus=oracle_open_bus;
    to_cpu(c,i);
}
static uint32_t pc_of(const Interp816 *i) { return (unsigned)i->k<<16|i->pc; }
static void setup(CpuState *c,Interp816 *i,unsigned flags,unsigned hidden) {
    memset(&witness,0,sizeof witness);deadline=0;
    for (unsigned n=0;n<sizeof ram;++n)ram[n]=(uint8_t)(n*73+cases*17);
    for (unsigned n=0;n<=0x800;n+=64)ram[0x500+n]=0;
    memset(c,0,sizeof *c);c->ram=ram;c->PB=c->DB=1;c->S=0x1d0;c->Y=0x9234;
    c->A=hidden|0x37;c->X=0x340;c->P=0x24|((flags&1)?1:0)|((flags&2)?2:0)|((flags&4)?0x40:0)|((flags&8)?0x80:0);
    cpu_p_to_mirrors(c);
    uint32_t landing=0;CpuState before=*c;
    assert(!LagoonNativeIterator52_Process(rom,sizeof rom,c,c,7,7,0x01c68f,&landing));
    assert(!memcmp(c,&before,sizeof before));assert(witness.phase==1);
    from_cpu(i,c,0x01c68f);step(i,c);assert(pc_of(i)==0x01c695);
    assert(!LagoonNativeIterator52_Process(rom,sizeof rom,c,c,7,7,pc_of(i),&landing));
    assert(witness.phase==2);step(i,c);step(i,c);assert(pc_of(i)==0x01c69b);
}
static void compare_case(unsigned flags,unsigned hidden,unsigned active,int post,unsigned cut) {
    CpuState c;Interp816 *i=interp816_init(NULL,rd,wr);assert(i);setup(&c,i,flags,hidden);
    if (active<33)ram[0x500+active*64]=0xc0|(active&63);
    if (post) { witness.phase=3;witness.x=c.X=active*64;from_cpu(i,&c,0x01c6af); }
    CpuState original=c;
    uint32_t start=post?0x01c6af:0x01c69b,landing=0;
    memcpy(snapshot,ram,sizeof ram);
    deadline=cut?c.master_cycles+cut:0;
    assert(LagoonNativeIterator52_Process(rom,sizeof rom,&c,&c,7,7,start,&landing));
    assert(landing>=0x01c69b && landing<=0x01c6c1);
    memcpy(native_ram,ram,sizeof ram);memcpy(ram,snapshot,sizeof ram);
    from_cpu(i,&original,start);
    unsigned count=0;
    while (original.cycles < c.cycles) {
        if (deadline && original.master_cycles>=deadline)break;
        assert(++count<500);step(i,&original);
        if (pc_of(i)==0x01c6a4 || pc_of(i)==0x01c6c1)break;
    }
    if (pc_of(i)!=landing || c.A!=original.A || c.X!=original.X || c.P!=original.P ||
        c.cycles!=original.cycles || c.master_cycles!=original.master_cycles || c.open_bus!=original.open_bus) {
        fprintf(stderr,"case%u cut%u post%d active%u landing%06X oracle%06X A%04X/%04X X%04X/%04X P%02X/%02X cycles%llu/%llu master%llu/%llu bus%02X/%02X\n",
            cases,cut,post,active,landing,pc_of(i),c.A,original.A,c.X,original.X,c.P,original.P,
            (unsigned long long)c.cycles,(unsigned long long)original.cycles,
            (unsigned long long)c.master_cycles,(unsigned long long)original.master_cycles,c.open_bus,original.open_bus);exit(2);
    }
    assert(c.Y==original.Y && c.S==original.S && c.DB==original.DB && c.D==original.D && c.PB==original.PB);
    assert(c.m_flag==original.m_flag && c.x_flag==original.x_flag && c.emulation==original.emulation);
    assert(c.coprocessor_master_cycles==original.coprocessor_master_cycles);
    assert(!memcmp(native_ram,ram,sizeof ram));assert(!memcmp(snapshot,ram,sizeof ram));
    interp816_free(i);++cases;if(cut)++interruptions;
}
int main(int argc,char **argv) {
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
    assert(LagoonNativeIterator52_Source(rom,sizeof rom));
    unsigned hidden[]={0,0x100,0xff00};
    for(unsigned flags=0;flags<16;++flags)for(unsigned hi=0;hi<3;++hi)
        for(unsigned active=0;active<=33;++active)compare_case(flags,hidden[hi],active,0,0);
    for(unsigned active=0;active<33;++active)compare_case(active&15,0xab00,active,1,0);
    /* Sweep every master-clock cut through a full inactive pass; this includes
     * every interior opcode, M0/M1 transition and taken/final branch boundary. */
    for(unsigned cut=1;cut<=11000;++cut)compare_case(cut&15,0x7f00,33,0,cut);
    for(unsigned cut=1;cut<=350;++cut)compare_case(cut&15,0x7f00,32,1,cut);
    for(unsigned bad=0;bad<20+0x98;++bad) {
        CpuState c;Interp816 *i=interp816_init(NULL,rd,wr);setup(&c,i,0,0);
        uint64_t gen=7,frame=7;const CpuState *current=&c;uint32_t pc=0x01c69b,landing=0;
        switch(bad) {
        case 0:c.PB=2;break;case 1:c.DB=2;break;case 2:c.D=1;break;
        case 3:c.emulation=1;break;case 4:c.x_flag=1;break;case 5:c.m_flag=0;break;
        case 6:c._flag_D=1;break;case 7:c.P^=8;break;case 8:c._flag_I=0;break;
        case 9:c.S++;break;case 10:c.X=1;break;case 11:gen++;break;case 12:frame++;break;
        case 13:current=NULL;break;case 14:witness.phase=0;break;
        case 15:ram[c.S+1]^=1;break;case 16:ram[c.S+9]^=1;break;
        case 17:pc=0x01c6af;break;case 18:c.X=0x840;break;case 19:c.ram=snapshot;break;
        default:rom[0xc643+bad-20]^=1;break;
        }
        CpuState before=c;memcpy(native_ram,ram,sizeof ram);
        assert(!LagoonNativeIterator52_Process(rom,sizeof rom,&c,current,gen,frame,pc,&landing));
        assert(!memcmp(&c,&before,sizeof c)&&!memcmp(native_ram,ram,sizeof ram));++refusals;
        if(bad>=20)rom[0xc643+bad-20]^=1;
        interp816_free(i);
    }
    printf("PASS native iterator52: %u actual-opcode cases, %u master-deadline cuts, %u context/source refusals, %u original opcodes; full WRAM/CPU/openbus/clocks exact; active calls/final sequence remain original\n",cases,interruptions,refusals,opcodes);
    return 0;
}
