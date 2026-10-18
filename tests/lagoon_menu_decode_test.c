#include "lagoon_menu_decode.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t rom[32768],ram[0x20000],prior[0x20000];
static CpuState state(void){CpuState c={0};c.S=0x1d0;c.X=0x2a;c.Y=0x1234;c.A=8;c.P=0x24;c.m_flag=1;c._flag_I=1;return c;}
static void setup(void){memset(ram,0,sizeof(ram));memset(rom,0,sizeof(rom));ram[0xe0]=0x80;ram[0xe1]=0x80;}
int main(void){unsigned used,n;setup();CpuState c=state();
 rom[0x8a]=4;memcpy(rom+0x8c,"test",4);
 assert(LagoonMenuDecode_Expand(rom,sizeof(rom),ram,&c,&used,&n));
 assert(used==16 && n==4 && !memcmp(ram+0x17f7c,"test",4));assert(c.S==0x1d0 && c.X==0x2a && c.Y==0x1234 && c.D==0 && c.m_flag && !c.x_flag);
 setup();c=state();rom[0x88]=1;rom[0x8a]=2;rom[0x8c]=0xaa;rom[0x8d]=0x10;rom[0x8e]=0x20;rom[0x8f]=rom[0x90]=0xaa;
 assert(LagoonMenuDecode_Expand(rom,sizeof(rom),ram,&c,&used,&n));
 assert(used==17 && n==4 && !memcmp(ram+0x17f7c,"\x10\x20\x10\x20",4));
 setup();c=state();rom[0x88]=0;rom[0x89]=1; /* Unterminated chained chunks fail transactionally. */
 memcpy(prior,ram,sizeof(ram));CpuState before=c;used=0xdeadbeef;n=0xbadf00d;
 assert(!LagoonMenuDecode_Expand(rom,0x8c,ram,&c,&used,&n));assert(!memcmp(prior,ram,sizeof(ram)) && !memcmp(&before,&c,sizeof(c)));assert(used==0xdeadbeef && n==0xbadf00d);
 c=state();c.emulation=1;assert(!LagoonMenuDecode_Expand(rom,sizeof(rom),ram,&c,&used,&n));
 puts("PASS raw/dictionary output, preserved X/Y/S, native widths, malformed stream transaction and emulation refusal");return 0;}
