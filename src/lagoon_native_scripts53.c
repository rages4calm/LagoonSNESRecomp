/* Finite original script header/command-fetch windows. Original dispatch,
 * command children, terminal RTS and diagnostic CC9A remain real boundaries. */
#include "lagoon_native_scripts53.h"
#include "snes/interp_bridge.h"
#include "cpu_trace.h"
#include <string.h>
typedef struct { uint32_t pc; unsigned length; uint64_t hash; } Span;
static const Span source_spans[] = {
    /* Filled from the pinned original source, independently checked by tests. */
    {0x01cc7b,96,UINT64_C(0xbd97949556654d60)},
    {0x0080e9,32,UINT64_C(0x0fc1177b6046227e)},
    {0x01809a,3,UINT64_C(0xc36d7317ce7a039e)},
    {0x018784,3,UINT64_C(0xc27ae417cdaadb00)},
    {0x0180a0,3,UINT64_C(0xc43eef17cf2af564)},
    {0x019b34,0x88,UINT64_C(0xbe94bd7ecc628c41)},
    {0x018772,34,UINT64_C(0x3e3748c45a18a9bc)},
    {0x01c8eb,90,UINT64_C(0xaa49711299929266)},
    {0x05c347,184,UINT64_C(0x6e418c2cb0aaa368)},
    {0x05ca0e,24,UINT64_C(0xb6bf4d51459ddfc7)},
    {0x05cb37,150,UINT64_C(0x92e05d9c933b3a87)}
};
static const unsigned table[] = {0x05c347,0x05ca0e,0x05cb37};
static const unsigned counts[] = {92,12,75};
static const unsigned headers[] = {0x01cc87,0x01cca0,0x01ccb6};
static const unsigned owners[] = {0x809c,0x80a2,0x8786};
static struct {
    const CpuState *cpu; const uint8_t *ram; uint64_t generation;
    uint16_t sp,pointer; uint8_t stack[10],stack_size,phase,family;
} witness;
static struct {uint64_t calls[3],headers[3],fetches[3],commands,ends,
                         instructions,deadlines,refused;} stats;
static unsigned word(const uint8_t *p) { return p[0] | p[1]<<8; }
static unsigned offset(unsigned pc) { return (pc>>16)*32768+(pc&32767); }
int LagoonNativeScripts53_Source(const uint8_t *rom,size_t size) {
    if (!rom || size!=0x100000) return 0;
    for (unsigned s=0;s<sizeof source_spans/sizeof *source_spans;++s) {
        const Span *v=source_spans+s;uint64_t h=UINT64_C(14695981039346656037);
        for(unsigned n=0;n<v->length;++n)h=(h^rom[offset(v->pc)+n])*UINT64_C(1099511628211);
        if(h!=v->hash)return 0;
    }
    return 1;
}
static int mode(const CpuState *c,int outer) {
    return c && c->ram && c->PB==1 && c->DB==1 && !c->D && !c->emulation &&
        c->m_flag && !c->x_flag && !c->_flag_D && c->_flag_I &&
        (c->P&0x3c)==0x24 && c->S>=(outer?0x106:0x104) && c->S<=(outer?0x1f8:0x1f6);
}
static int live(const CpuState *c,uint64_t generation) {
    return witness.phase && witness.cpu==c && witness.ram==c->ram &&
        witness.generation==generation && witness.sp==c->S &&
        witness.stack_size>=9 && witness.stack_size<=sizeof witness.stack &&
        !memcmp(witness.stack,c->ram+c->S+1,witness.stack_size);
}
static void nz(CpuState *c,unsigned v,unsigned sign) {
    c->_flag_Z=!v;c->_flag_N=!!(v&sign);cpu_mirrors_to_p(c);
}
static void a8(CpuState *c,unsigned value) {
    c->A=(c->A&0xff00)|(value&255);nz(c,value&255,128);
}
static unsigned fetch_pointer(CpuState *c) {
    unsigned p=cpu_read8_timed(c,0,0xa1);
    p|=cpu_read8_timed(c,0,0xa2)<<8;
    p|=cpu_read8_timed(c,0,0xa3)<<16;
    p=(p+c->Y)&0xffffff;
    return cpu_read8_timed(c,p>>16,p);
}
int LagoonNativeScripts53_Process(const uint8_t *rom,size_t size,CpuState *c,
    const CpuState *current,uint64_t generation,uint64_t frame,
    uint32_t pc,uint32_t *landing) {
    int family=pc==0x01809a?0:pc==0x0180a0?1:pc==0x018784?2:-1;
    if(family>=0)memset(&witness,0,sizeof witness);
    if(!landing || c!=current || generation!=frame || !mode(c,family>=0) ||
        !LagoonNativeScripts53_Source(rom,size)) {
        witness.phase=0;++stats.refused;return 0;
    }
    if(family>=0) {
        witness.cpu=c;witness.ram=c->ram;witness.generation=generation;
        witness.sp=c->S-2;witness.phase=1;witness.family=family;
        /* Source callers use1F6(room) and1F8(main). Include the real JSR
         * return plus remaining outer bytes only through physical stack1FF;
         * never mistake unrelated0200 WRAM for a saved stack byte. */
        unsigned available=0x1ff-witness.sp;
        witness.stack_size=available<sizeof witness.stack?available:sizeof witness.stack;
        witness.stack[0]=owners[family]&255;witness.stack[1]=owners[family]>>8;
        memcpy(witness.stack+2,c->ram+c->S+1,witness.stack_size-2);++stats.calls[family];
        return 0; /* Actual original JSR, dispatch and physical frame follow. */
    }
    if(!live(c,generation)){witness.phase=0;++stats.refused;return 0;}
    family=witness.family;
    if(pc==headers[family] && witness.phase==1) {
        unsigned index=family==2?word(c->ram+0x48e):c->ram[0x9d];
        if(c->ram[0x9c]!=family || index>=counts[family] || (c->A&0xff00)) {
            witness.phase=0;++stats.refused;return 0;
        }
        if(!family && (c->Y<0x40 || c->Y>0x800 || (c->Y&63) ||
            (c->ram[0x500+c->Y]&0xc8)!=0xc8 || c->ram[0x518+c->Y]!=1 ||
            c->ram[0x51b+c->Y]!=index)) {witness.phase=0;++stats.refused;return 0;}
        witness.pointer=word(rom+offset(table[family])+index*2);
        if(witness.pointer<0xc3ff || witness.pointer>0xce54) {
            witness.phase=0;++stats.refused;return 0;
        }
        witness.phase=2;++stats.headers[family];
    } else {
        int eligible=(pc==0x01cccb && witness.phase==2) ||
            (pc==0x01ccce && witness.phase==3) ||
            (pc==0x01ccd8 && witness.phase==3);
        unsigned y=pc==0x01cccb?0:c->Y;
        unsigned p=word(c->ram+0xa1);
        if(!eligible || p!=witness.pointer || c->ram[0xa3]!=5 ||
            (unsigned)p+y>0xce54 || (unsigned)p+y<0xc3ff) {
            witness.phase=0;++stats.refused;return 0;
        }
        unsigned command=rom[0x28000+p+y-0x8000];
        if(command>=40 && command!=255) {witness.phase=0;++stats.refused;return 0;}
        ++stats.fetches[family];
    }
    cpu_instruction_timing_commit(c);
    for(;;) {
        if(interp_bridge_lle_master_deadline_reached(c)) {
            witness.phase=0;++stats.deadlines;*landing=pc;return 1;
        }
        unsigned bytes=1,cycles=2,next=pc+1;
        switch(pc) {
        case 0x01cc87:bytes=3;cycles=5;next=0x01cc8a;break;
        case 0x01cc8a:case 0x01cca0:bytes=2;cycles=3;break;
        case 0x01cc8e:case 0x01cc94:case 0x01cca4:case 0x01ccaa:
        case 0x01ccbb:case 0x01ccc1:bytes=4;cycles=5;break;
        case 0x01cc92:case 0x01cc98:case 0x01cca8:case 0x01ccae:
        case 0x01ccb2:case 0x01ccbf:case 0x01ccc5:case 0x01ccc9:bytes=2;cycles=3;break;
        case 0x01ccb0:case 0x01ccc7:case 0x01ccd0:case 0x01ccd2:bytes=2;break;
        case 0x01ccb4:case 0x01ccd8:bytes=2;cycles=3;break;
        case 0x01ccb6:bytes=3;cycles=4;break;
        case 0x01cccb:bytes=3;cycles=3;break;
        case 0x01ccce:bytes=2;cycles=6;break;
        default:break;
        }
        cpu_trace_block(c,pc);c->open_bus=rom[offset(pc)+bytes-1];
        switch(pc) {
        case 0x01cc87:cpu_write16_timed(c,1,0x1030,c->Y);break;
        case 0x01cc8a:case 0x01cca0:a8(c,cpu_read8_timed(c,0,0x9d));next=pc+2;break;
        case 0x01cc8c:case 0x01cca2:case 0x01ccb9:
            c->_flag_C=!!(c->A&128);a8(c,c->A<<1);break;
        case 0x01cc8d:case 0x01cca3:case 0x01ccba:
            c->X=c->A;nz(c,c->X,32768);break;
        case 0x01cc8e:case 0x01cca4:case 0x01ccbb:
            a8(c,cpu_read8_timed(c,5,(uint16_t)(table[family]+c->X)));next=pc+4;break;
        case 0x01cc94:case 0x01ccaa:case 0x01ccc1:
            a8(c,cpu_read8_timed(c,5,(uint16_t)(table[family]+c->X+1)));next=pc+4;break;
        case 0x01cc92:case 0x01cca8:case 0x01ccbf:
            cpu_write8_timed(c,0,0xa1,c->A);next=pc+2;break;
        case 0x01cc98:case 0x01ccae:case 0x01ccc5:
            cpu_write8_timed(c,0,0xa2,c->A);next=pc+2;break;
        case 0x01ccb0:case 0x01ccc7:a8(c,5);next=pc+2;break;
        case 0x01ccb2:case 0x01ccc9:cpu_write8_timed(c,0,0xa3,c->A);next=pc+2;break;
        case 0x01ccb4:next=0x01cccb;break;
        case 0x01ccb6:a8(c,cpu_read8_timed(c,1,0x48e));next=pc+3;break;
        case 0x01cccb:c->Y=0;nz(c,c->Y,32768);next=0x01ccce;break;
        case 0x01ccce:a8(c,fetch_pointer(c));next=0x01ccd0;break;
        case 0x01ccd0:c->_flag_C=(c->A&255)>=255;nz(c,((c->A&255)-255)&255,128);next=0x01ccd2;break;
        case 0x01ccd2:cycles=c->_flag_Z?3:2;next=c->_flag_Z?0x01ccda:0x01ccd4;break;
        case 0x01ccd4:c->Y++;nz(c,c->Y,32768);break;
        case 0x01ccd8:next=0x01ccce;break;
        default:witness.phase=0;return 0;
        }
        c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2;++stats.instructions;
        pc=next;cpu_instruction_timing_commit(c);
        if(pc==0x01cc9a || (pc==0x01cccb && witness.phase==2)) {
            *landing=pc;return 1; /* In particular, never bypass the CC9A hook. */
        }
        if(pc==0x01ccd5) {
            witness.phase=3;++stats.commands;*landing=pc;return 1;
        }
        if(pc==0x01ccda) {
            witness.phase=0;++stats.ends;*landing=pc;return 1;
        }
    }
}
#ifndef LAGOON_NATIVE_SCRIPTS53_TEST
#include "common_rtl.h"
#include <stdio.h>
#include <stdlib.h>
static uint64_t frame_generation;
static int enabled=-1,report_enabled;
static void report(void) {
    if(report_enabled)fprintf(stderr,
        "[native-scripts53] calls=%llu,%llu,%llu headers=%llu,%llu,%llu fetches=%llu,%llu,%llu commands=%llu ends=%llu instructions=%llu deadlines=%llu refused=%llu enabled=%d\n",
        (unsigned long long)stats.calls[0],(unsigned long long)stats.calls[1],(unsigned long long)stats.calls[2],
        (unsigned long long)stats.headers[0],(unsigned long long)stats.headers[1],(unsigned long long)stats.headers[2],
        (unsigned long long)stats.fetches[0],(unsigned long long)stats.fetches[1],(unsigned long long)stats.fetches[2],
        (unsigned long long)stats.commands,(unsigned long long)stats.ends,(unsigned long long)stats.instructions,
        (unsigned long long)stats.deadlines,(unsigned long long)stats.refused,enabled);
}
static void hook(CpuState *c,uint32_t pc) {
    if(!enabled || c!=&g_cpu || !rtl_apu_absolute_periodic_enabled() || c->master_cycles<g_apu_last_sync_master)return;
    /* Bounded opt-in qualification evidence; no diagnostics or IO normally. */
    static uint16_t sampled;
    static const unsigned sites[]={0x01809a,0x0180a0,0x018784,0x01cc87,0x01cca0,0x01ccb6,0x01cccb,0x01ccce,0x01ccd8};
    if(report_enabled)for(unsigned n=0;n<sizeof sites/sizeof *sites;++n)if(pc==sites[n]&&!(sampled&(1u<<n))) {
        sampled|=1u<<n;
        fprintf(stderr,"[native-scripts53-context] pc=%06X PB=%02X DB=%02X D=%04X S=%04X A=%04X P=%02X M=%u X=%u E=%u I=%u decimal=%u source=%d phase=%u family=%u gen=%llu/%llu\n",
            pc,c->PB,c->DB,c->D,c->S,c->A,c->P,c->m_flag,c->x_flag,c->emulation,c->_flag_I,c->_flag_D,
            LagoonNativeScripts53_Source(g_rom,0x100000),witness.phase,witness.family,
            (unsigned long long)RtlStateGeneration(),(unsigned long long)frame_generation);
    }
    uint32_t landing;
    if(LagoonNativeScripts53_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),frame_generation,pc,&landing))
        interp_bridge_pre_opcode_redirect(landing);
}
void LagoonNativeScripts53_BeforeFrame(void) {
    if(enabled<0) {
        const char *v=getenv("LAGOON_NATIVE_SCRIPTS53");enabled=!v||!strcmp(v,"auto")||!strcmp(v,"on");
        v=getenv("LAGOON_NATIVE_SCRIPTS53_REPORT");report_enabled=v&&!strcmp(v,"1");
        if(report_enabled)atexit(report);
    }
    frame_generation=RtlStateGeneration();
    if(witness.phase && witness.generation!=frame_generation)witness.phase=0;
    static const unsigned sites[]={0x01809a,0x0180a0,0x018784,0x01cc87,0x01cca0,0x01ccb6,0x01cccb,0x01ccce,0x01ccd8};
    if(enabled)for(unsigned i=0;i<sizeof sites/sizeof *sites;++i)interp_bridge_add_pre_opcode_hook(sites[i],hook);
}
#endif
