/* Optional sword boundary cue, derived from01:B6BD's real rectangle data.
 * Four short warm-gold corner marks keep the scene visible during an attack. */
#include "lagoon_sword_range.h"
#include <string.h>

static const int16_t stock[4][4]={
  {0,28,-8,12},{-24,8,0,17},{-28,0,-12,8},{-8,24,-17,0}
};
static const int16_t cleave[4][4]={
  {0,38,-11,15},{-27,11,0,38},{-38,0,-15,11},{-11,27,-38,0}
};
static int signed_word(const uint8_t *p){return (int16_t)(p[0]|p[1]<<8);}
static int table_matches(const uint8_t *p,const int16_t table[4][4]){
  for(unsigned d=0;d<4;++d)for(unsigned e=0;e<4;++e)
    if(signed_word(p+d*8+e*2)!=table[d][e])return 0;
  return 1;
}
int LagoonSwordRange_Build(const uint8_t *rom,size_t size,unsigned facing,
    int hero_x,int hero_y,LagoonSwordRangeImage *out){
  if(!out)return 0;memset(out,0,sizeof(*out));
  if(!rom||size!=0x100000||facing>3||hero_x<-32768||hero_x>32767||
      hero_y<-32768||hero_y>32767)return 0;
  uint64_t hash=UINT64_C(14695981039346656037);
  for(unsigned n=0xb6bd;n<0xb6f0;++n)
    hash=(hash^rom[n])*UINT64_C(1099511628211);
  if(hash!=UINT64_C(0x43c0fac92a364173)||
      (!table_matches(rom+0xb6f0,stock)&&!table_matches(rom+0xb6f0,cleave)))return 0;
  const uint8_t *p=rom+0xb6f0+facing*8;
  int left=signed_word(p),right=signed_word(p+2);
  int top=signed_word(p+4),bottom=signed_word(p+6);
  unsigned w=(unsigned)(right-left+1),h=(unsigned)(bottom-top+1);
  if(!w||!h||w>LAGOON_SWORD_RANGE_SIDE||h>LAGOON_SWORD_RANGE_SIDE)return 0;
  out->x=hero_x+left;out->y=hero_y+top;out->width=w;out->height=h;
  const uint16_t gold=0x8000u|31u|(24u<<5)|(9u<<10);
  for(unsigned n=0;n<5;++n){
    if(n<w){
      out->pixels[n]=out->pixels[w-1-n]=gold;
      out->pixels[(h-1)*LAGOON_SWORD_RANGE_SIDE+n]=gold;
      out->pixels[(h-1)*LAGOON_SWORD_RANGE_SIDE+w-1-n]=gold;
    }
    if(n<h){
      out->pixels[n*LAGOON_SWORD_RANGE_SIDE]=gold;
      out->pixels[n*LAGOON_SWORD_RANGE_SIDE+w-1]=gold;
      out->pixels[(h-1-n)*LAGOON_SWORD_RANGE_SIDE]=gold;
      out->pixels[(h-1-n)*LAGOON_SWORD_RANGE_SIDE+w-1]=gold;
    }
  }
  return 1;
}
