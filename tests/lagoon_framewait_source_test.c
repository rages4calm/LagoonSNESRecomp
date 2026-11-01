#include "../src/lagoon_idle_poll.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char**argv){
 assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f);uint8_t *rom=malloc(0x100000);assert(rom&&fread(rom,1,0x100000,f)==0x100000);fclose(f);
 uint8_t p[5],r[5],d[7];memcpy(p,rom+0x4e3,5);memcpy(r,rom+0x4de,5);memcpy(d,rom+0x91b,7);assert(LagoonFrameWait_SourceMatches(p,r,d));unsigned cases=1;
 uint8_t *parts[]={p,r,d};const unsigned sizes[]={5,5,7};
 for(unsigned a=0;a<3;a++)for(unsigned n=0;n<sizes[a];n++){parts[a][n]^=1;assert(!LagoonFrameWait_SourceMatches(p,r,d));parts[a][n]^=1;cases++;}
 assert(!LagoonFrameWait_SourceMatches(NULL,r,d));assert(!LagoonFrameWait_SourceMatches(p,NULL,d));assert(!LagoonFrameWait_SourceMatches(p,r,NULL));cases+=3;
 printf("PASS %u frame-wait source admissions: original,17 single-byte mutations,3 null refusals\n",cases);free(rom);return 0;
}
