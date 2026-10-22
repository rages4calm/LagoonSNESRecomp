/* Full original upload routines, statically translated by source PC.
 * Every device access keeps the interpreter's opcode-start observation point.
 * Polls use the actual APU; deadline unwinds retain the real physical frame. */
/* The mapped original bridge flushes accumulated opcode time BEFORE a port
 * access, publishes this exact master watermark, then drains AFTER each write.
 * Boot's relative/pending/progress policy is deliberately original-only. Pure
 * source-oracle CPUs have no live device authority and use their explicit bus. */
static void upload_apu_sync(CpuState*c,bool before){
 if(c!=&g_cpu)return;
 RtlApuLock();if(before)g_apu_last_sync_master=c->master_cycles;
 rtl_sync_apu_to_cpu_locked();RtlApuUnlock();
}
static unsigned upload_port_read(CpuState*c,unsigned a,bool wide){
 upload_apu_sync(c,true);return wide?cpu_read16(c,c->DB,a):cpu_read8(c,c->DB,a);
}
static void upload_port_write(CpuState*c,unsigned a,unsigned v,bool wide){
 upload_apu_sync(c,true);if(wide)cpu_write16(c,c->DB,a,v);else cpu_write8(c,c->DB,a,v);upload_apu_sync(c,false);
}
static void upload_write(CpuState*c,unsigned a,unsigned v){
 if(a>=0x2140&&a<=0x2143)upload_port_write(c,a,v,false);else cpu_write8(c,c->DB,a,v);
}
static void upload_nz(CpuState*c,unsigned a,unsigned width){unsigned mask=width==1?255:65535,sign=width==1?128:32768;c->_flag_Z=!(a&mask);c->_flag_N=!!(a&sign);cpu_mirrors_to_p(c);}
static void upload_a(CpuState*c,unsigned a){if(c->m_flag)c->A=(c->A&0xff00)|(a&255);else c->A=a;upload_nz(c,c->A,c->m_flag?1:2);}
static void upload_cmp(CpuState*c,unsigned a,unsigned b,unsigned width){unsigned mask=width==1?255:65535;c->_flag_C=(a&mask)>=(b&mask);upload_nz(c,(a-b)&mask,width);}
static void upload_adc(CpuState*c,unsigned value){unsigned a=c->A&255,b=value&255,sum=a+b+c->_flag_C;c->_flag_C=sum>255;c->_flag_V=!!((~(a^b)&(a^sum))&128);upload_a(c,sum);}
static uint32_t upload_pointer(CpuState*c){unsigned p=cpu_read8_timed(c,0,0x1c);p|=cpu_read8_timed(c,0,0x1d)<<8;p|=cpu_read8_timed(c,0,0x1e)<<16;return(p+c->Y)&0xffffff;}
static unsigned upload_long(CpuState*c){unsigned p=upload_pointer(c),a=cpu_read8_timed(c,p>>16,p);if(!c->m_flag){p=(p+1)&0xffffff;a|=cpu_read8_timed(c,p>>16,p)<<8;}return a;}
static const struct {uint32_t pointer,length;uint64_t hash;} upload_packs[]={
 {0x0c8000,24764,UINT64_C(0xb36f260378f7d934)},
 {0x0d8d46,28556,UINT64_C(0x804ad309af479599)},
 {0x0fa12e,6461,UINT64_C(0x1366f356cc6088e1)},
 {0x0fba6b,5100,UINT64_C(0xc0c86d80094f14a8)},
 {0x0fce57,1490,UINT64_C(0x4aa0346eddcb4b2e)},
 {0x0e8000,7445,UINT64_C(0x3a857526da7d58a6)},
 {0x0e9d15,9110,UINT64_C(0x141213c21c2cc7f5)},
 {0x0ec0ab,8028,UINT64_C(0xdf132f503ac6f21e)},
 {0x0f8000,8494,UINT64_C(0x999adb07d17671c8)},
 {0x0fd429,7900,UINT64_C(0x5e90eb241276b539)},
 {0x0d8000,3398,UINT64_C(0x5b367902e5ef1975)}};
static int upload_resource(const uint8_t*rom,unsigned p){for(unsigned i=0;i<sizeof upload_packs/sizeof*upload_packs;i++)if(p==upload_packs[i].pointer)return source_hash(rom,p,upload_packs[i].length)==upload_packs[i].hash;return 0;}
static int upload_begin(const uint8_t*rom,CpuState*c,unsigned pc,unsigned bytes,RecompReturn*out){cpu_instruction_timing_commit(c);if((c==&g_cpu&&!music_timeline_valid(c))||interp_bridge_lle_master_deadline_reached(c)){*out=interp_bridge_lle_yield_unwind(c,pc);return 0;}cpu_trace_block(c,pc);c->coprocessor_master_cycles=c->master_cycles;c->open_bus=rom[((pc>>16)&127)*32768+(pc&32767)+bytes-1];return 1;}
static void upload_charge(CpuState*c,unsigned cycles,unsigned bytes){c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2;}
static void upload_push(CpuState*c,unsigned a){cpu_write8_timed(c,0,c->S,a);--c->S;}
static unsigned upload_pull(CpuState*c){return cpu_read8_timed(c,0,++c->S);}
static RecompReturn upload_return(CpuState*c,unsigned n,unsigned host,unsigned site){unsigned p=upload_pull(c);p|=upload_pull(c)<<8;unsigned bank=n==3?upload_pull(c):c->PB;c->PB=bank;unsigned actual=(bank<<16)|((p+1)&65535);cpu_instruction_timing_commit(c);if(actual!=host)return interp_tier_dispatch_rewritten_return(c,actual,site);return RECOMP_RETURN_NORMAL;}
bool LagoonNativeMusic_UploadTry(const uint8_t*rom,size_t size,CpuState*c,const CpuState*current,uint64_t generation,uint64_t frame,bool frame_valid,unsigned service,uint16_t entry_s,uint8_t hrv,uint32_t host,RecompReturn*out){
 static const unsigned target[]={0x0cfe9b,0x0cfeec,0x0cff69};
 if(!rom||size!=0x100000||!c||c!=current||!c->ram||!out||service>=3||!frame_valid||generation!=frame||c->PB!=12||(c->DB!=1&&c->DB!=2&&(service!=2||c->DB!=0))||c->D||!c->m_flag||c->x_flag||c->emulation||c->_flag_D||(c->P&(CPU_P_M|CPU_P_X|CPU_P_D))!=CPU_P_M||c->S!=entry_s||c->S<0x100||c->S>0x1ff8||hrv!=(service==2?2:3)||c->host_return_valid!=hrv)return false;
 unsigned known=service==0?(host==0x0cf833||host==0x0cf931):service==1?(host==0x0cf84e||host==0x0cf94c):(host==0x0cfee3||host==0x0cff34||host==0x0cff5c);
 if(!known||word(c->ram+c->S+1)!=((host-1)&65535)||(hrv==3&&c->ram[c->S+3]!=12)||source_hash(rom,0x0084de,11)!=UINT64_C(0xae13afe6e9ff1fc7)||source_hash(rom,0x0cf800,508)!=UINT64_C(0x42057ae75dd8341b)||source_hash(rom,0x0cfe9b,357)!=UINT64_C(0xe213df57703b685f))return false;
 unsigned pointer=0;
 if(service==2){pointer=c->ram[0x1c]|c->ram[0x1d]<<8|c->ram[0x1e]<<16;}
 else{unsigned id=c->A&255;if(!id||id>(service?8:2))return false;unsigned at=(service?0x67fd8:0x67fcc)+id*4;pointer=rom[at]|rom[at+1]<<8|rom[at+2]<<16;
  /* No store in the source setup may alias the live physical return. */
  unsigned addresses[]={service?0x43f:0x43e,service?0x436:0x434};for(unsigned i=0;i<2;i++)if(addresses[i]>c->S&&addresses[i]<=c->S+hrv)return false;}
 /* DB0 is admitted only for the exact original boot JSR and complete IPLpack.
  * DB1/2 are the source-proven lowWRAM/IO mirror contexts. */
 if(c->DB==0&&(host!=0x0cff5c||pointer!=0x0d8000))return false;
 if(!upload_resource(rom,pointer))return false;
 LagoonNativeEndgameConsumeMusic(service+3);
 unsigned pc=target[service],base=pc;unsigned v;RecompReturn r;
#define UB(at,n) do{if(!upload_begin(rom,c,(at),(n),out))return true;}while(0)
#define UC(cyc,n) upload_charge(c,cyc,n)
#define US(at,cyc,n,stmt) do{UB(at,n);stmt;UC(cyc,n);}while(0)
#define UA(at,n,val) US(at,c->m_flag?2:3,n,upload_a(c,val))
#define UI(at,reg) US(at,2,1,c->reg=(uint16_t)(c->reg+1);upload_nz(c,c->reg,2))
#define UX(at) US(at,3,1,c->A=(c->A>>8)|(c->A<<8);a8(c,c->A))
#define UP(at) US(at,3,2,c->P|=CPU_P_M;cpu_p_to_mirrors(c))
#define UR(at,mask) US(at,3,2,c->P&=~(mask);cpu_p_to_mirrors(c))
#define UW(at,adr,val) US(at,4,3,upload_write(c,adr,val))
 if(service!=2){
  UP(base);US(base+2,3,1,upload_push(c,c->A));UA(base+3,2,0);UX(base+5);US(base+6,4,1,upload_a(c,upload_pull(c)));
  US(base+7,4,3,cpu_write8_timed(c,c->DB,service?0x43f:0x43e,c->A));US(base+10,4,3,cpu_write8_timed(c,c->DB,service?0x436:0x434,c->A));
  US(base+13,2,1,c->_flag_C=!!(c->A&128);upload_a(c,c->A<<1));US(base+14,2,1,c->_flag_C=!!(c->A&128);upload_a(c,c->A<<1));US(base+15,2,1,c->X=c->A;upload_nz(c,c->X,2));
  for(unsigned k=0;k<3;k++){unsigned at=base+16+k*7;if(k)UI(at-1,X);US(at,5,4,upload_a(c,cpu_read8_timed(c,12,(service?0xffd8:0xffcc)+c->X)));US(at+4,3,2,cpu_write8_timed(c,0,0x1c+k,c->A));}
  UW(base+36,0x2140,0);UW(base+39,0x2141,0);UW(base+42,0x2142,0);
  /* Exact source JSL frames. Real84DE retains original NMI poll; no completion is made. */
  const unsigned waits[]={base+45,base+49,base+58,base+62};
  for(unsigned k=0;k<4;k++){
   if(k==2){UA(base+53,2,255);UW(base+55,0x2140,c->A);}
   unsigned at=waits[k];UB(at,4);upload_push(c,12);upload_push(c,(at+3)>>8);upload_push(c,at+3);UC(8,4);c->PB=0;c->host_return_valid=3;
   unsigned next=0;r=interp_tier_run_call_frame(c,0x0084de,at,3,&next);
   if(r!=RECOMP_RETURN_NORMAL){*out=r;return true;}
   if(next!=at+4){*out=interp_tier_dispatch_rewritten_return(c,next,at);return true;}
  }
  UW(base+66,0x4200,0);
  UB(base+69,3);upload_push(c,(base+71)>>8);upload_push(c,base+71);UC(6,3);c->host_return_valid=2;
  /* Source-owned child, with the actual JSR frame just made above. */
  if(!LagoonNativeMusic_UploadTry(rom,size,c,current,generation,frame,frame_valid,2,c->S,2,base+72,&r)){unsigned next=0;r=interp_tier_run_call_frame(c,0x0cff69,base+69,2,&next);if(r==RECOMP_RETURN_NORMAL&&next!=base+72){*out=interp_tier_dispatch_rewritten_return(c,next,base+69);return true;}}
  if(r!=RECOMP_RETURN_NORMAL){*out=r;return true;}c->host_return_valid=hrv;
  US(base+72,4,3,upload_a(c,cpu_read8(c,c->DB,0x4210)));UA(base+75,2,0x81);UW(base+77,0x4200,c->A);
  UB(base+80,1);UC(6,1);*out=upload_return(c,3,host,base+80);return true;
 }
 /* Complete original FF69 IPL header/payload/acknowledgment loop. */
 US(0x0cff69,3,1,cpu_mirrors_to_p(c);upload_push(c,c->P));UR(0x0cff6a,0x30);US(0x0cff6c,3,3,c->Y=0;upload_nz(c,0,2));UA(0x0cff6f,3,0xbbaa);
ff72:UB(0x0cff72,3);v=upload_port_read(c,0x2140,true);upload_cmp(c,c->A,v,2);UC(5,3);
 US(0x0cff75,c->_flag_Z?2:3,2,(void)0);if(!c->_flag_Z)goto ff72;
 UP(0x0cff77);UA(0x0cff79,2,0xcc);US(0x0cff7b,3,2,(void)0);goto ffa3;
ff7d:US(0x0cff7d,6,2,upload_a(c,upload_long(c)));UI(0x0cff7f,Y);UX(0x0cff80);UA(0x0cff81,2,0);US(0x0cff83,3,2,(void)0);goto ff90;
ff85:UX(0x0cff85);US(0x0cff86,6,2,upload_a(c,upload_long(c)));UI(0x0cff88,Y);UX(0x0cff89);
ff8a:UB(0x0cff8a,3);v=upload_port_read(c,0x2140,false);upload_cmp(c,c->A,v,1);UC(4,3);US(0x0cff8d,c->_flag_Z?2:3,2,(void)0);if(!c->_flag_Z)goto ff8a;
 US(0x0cff8f,2,1,upload_a(c,c->A+1));
ff90:UR(0x0cff90,0x20);UB(0x0cff92,3);upload_port_write(c,0x2140,c->A,true);UC(5,3);UP(0x0cff95);US(0x0cff97,2,1,--c->X;upload_nz(c,c->X,2));US(0x0cff98,c->_flag_Z?2:3,2,(void)0);if(!c->_flag_Z)goto ff85;
ff9a:UB(0x0cff9a,3);v=upload_port_read(c,0x2140,false);upload_cmp(c,c->A,v,1);UC(4,3);US(0x0cff9d,c->_flag_Z?2:3,2,(void)0);if(!c->_flag_Z)goto ff9a;
ff9f:US(0x0cff9f,2,2,upload_adc(c,3));US(0x0cffa1,c->_flag_Z?3:2,2,(void)0);if(c->_flag_Z)goto ff9f;
ffa3:US(0x0cffa3,3,1,upload_push(c,c->A));UR(0x0cffa4,0x20);US(0x0cffa6,7,2,upload_a(c,upload_long(c)));UI(0x0cffa8,Y);UI(0x0cffa9,Y);US(0x0cffaa,2,1,c->X=c->A;upload_nz(c,c->X,2));US(0x0cffab,7,2,upload_a(c,upload_long(c)));UI(0x0cffad,Y);UI(0x0cffae,Y);
 UB(0x0cffaf,3);upload_port_write(c,0x2142,c->A,true);UC(5,3);UP(0x0cffb2);US(0x0cffb4,3,3,upload_cmp(c,c->X,1,2));UA(0x0cffb7,2,0);US(0x0cffb9,2,1,v=(c->A&255)*2+c->_flag_C;c->_flag_C=!!(v&256);upload_a(c,v));UW(0x0cffba,0x2141,c->A);US(0x0cffbd,2,2,upload_adc(c,0x7f));US(0x0cffbf,4,1,upload_a(c,upload_pull(c)));UW(0x0cffc0,0x2140,c->A);
ffc3:UB(0x0cffc3,3);v=upload_port_read(c,0x2140,false);upload_cmp(c,c->A,v,1);UC(4,3);US(0x0cffc6,c->_flag_Z?2:3,2,(void)0);if(!c->_flag_Z)goto ffc3;US(0x0cffc8,c->_flag_V?3:2,2,(void)0);if(c->_flag_V)goto ff7d;
 US(0x0cffca,4,1,c->P=upload_pull(c);cpu_p_to_mirrors(c));UB(0x0cffcb,1);UC(6,1);*out=upload_return(c,2,host,0x0cffcb);return true;
#undef UB
#undef UC
#undef US
#undef UA
#undef UI
#undef UX
#undef UP
#undef UR
#undef UW
}
