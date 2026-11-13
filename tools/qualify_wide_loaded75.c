#define main unused_parallax_main
#include "test_wide_parallax_sources.c"
#undef main
static unsigned run_decode(Interp816*c,unsigned address){source=maximum=address;ram[0xe0]=address;ram[0xe1]=address>>8;ram[0xe2]=address>>16;initcpu(c,0,0x9143,0x1d0,3);ram[0x1d3]=1;unsigned n=0;while(c->k!=1||c->pc!=0x7ff0){assert(++n<3000000);interp816_runOpcode(c);}return word(ram,0xf9);}
#ifndef LAGOON_ATLAS75_NO_MAIN
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);Interp816*c=interp816_init(NULL,readb,writeb);assert(c);
 puts("/* Immutable ROM source/output hashes only; original009143 qualified offline. */");
 for(unsigned room=0;room<75;++room){memset(ram,0,sizeof ram);memset(io,0,sizeof io);unsigned desc=0x05b2df+room*56;memcpy(ram+0x50,rom+off(desc),56);unsigned ptr=ram[0x5d]|ram[0x5e]<<8|ram[0x5f]<<16;unsigned output=run_decode(c,ptr);unsigned length=maximum-source+1;unsigned prefix=output<0x2800?output:0x2800;prefix&=~31u;
 printf("  {%#x,%u,%u,UINT64_C(0x%016llx),UINT64_C(0x%016llx),UINT64_C(0x%016llx)}, /* %02X out=%u */\n",ptr,length,prefix,(unsigned long long)fnv(rom+off(ptr),length),(unsigned long long)fnv(ram+0x17f80,prefix),(unsigned long long)fnv(rom+off(desc),56),room,output);
 }
 interp816_free(c);return 0;}

#endif
