/* Original Lagoon 8x16 dialogue lettering and frame shapes decoded at runtime.
 * 02:8B89 uses 02:8C29's pair of tile words for each encoded character.
 * 02:89E3..8A38 selects the eight border tiles below. Guest memory is const. */
#include "lagoon_dialogue_font.h"
#include "snes/ppu.h"
#include <string.h>

static uint16_t pairs[256][2];
static uint8_t tiles[512][8][8];
static int qualified,ready;
static uint64_t hash_bytes(const uint8_t *bytes,size_t size){
  uint64_t hash=UINT64_C(14695981039346656037);
  while(size--)hash=(hash^*bytes++)*UINT64_C(1099511628211);
  return hash;
}
void LagoonDialogueFont_Init(const uint8_t *rom,size_t size){
  ready=qualified=0;memset(pairs,0,sizeof pairs);memset(tiles,0,sizeof tiles);
  if(!rom||size!=0x100000||
      hash_bytes(rom+0x10c29,1024)!=UINT64_C(0xd1f903123ac1ef9c))return;
  for(unsigned c=0;c<256;c++)for(unsigned half=0;half<2;half++){
    unsigned at=0x10c29+c*4+half*2;
    pairs[c][half]=(uint16_t)(rom[at]|rom[at+1]<<8);
  }
  qualified=1;
}
int LagoonDialogueFont_Capture(const Ppu *ppu){
  ready=0;
  if(!qualified||!ppu||ppu->bgmode!=9||ppu->bgTileAdr!=0x600)return 0;
  /* Entire resident BG3 font/frame bank is immutable in the verified scene.
   * Hash words in SNES little-endian order, independent of host byte order. */
  uint64_t hash=UINT64_C(14695981039346656037);
  for(unsigned i=0x6000;i<0x7000;i++){
    hash=(hash^(ppu->vram[i]&255))*UINT64_C(1099511628211);
    hash=(hash^(ppu->vram[i]>>8))*UINT64_C(1099511628211);
  }
  if(hash!=UINT64_C(0x3099d7e1e3b9d5d6))return 0;
  for(unsigned tile=0;tile<512;tile++)for(unsigned y=0;y<8;y++){
    uint16_t planes=ppu->vram[0x6000+tile*8+y];
    for(unsigned x=0;x<8;x++)tiles[tile][y][x]=(uint8_t)(
        ((planes>>(7-x))&1)|((planes>>(14-x))&2));
  }
  ready=1;return 1;
}
int LagoonDialogueFont_Ready(void){return ready;}
static unsigned encode(unsigned char c){
  if(c>='a'&&c<='z')return c-4;
  if((c>='A'&&c<='Z')||(c>='0'&&c<='9')||c==','||c=='.'||c=='?'||c==':'||c=='-')return c;
  switch(c){case ' ':return 0xfe;case '!':return 0x2f;case '\'':return 0x5c;
    case '(':return 0x2a;case ')':return 0x2b;default:return '?';}
}
static void pixel(uint32_t *dst,int stride,int height,int x,int y,uint32_t color){
  if(x>=0&&x<stride&&y>=0&&y<height)dst[(size_t)y*stride+x]=color;
}
void LagoonDialogueFont_DrawChar(uint32_t *dst,int stride,int height,int x,int y,
    unsigned char c,int scale){
  if(!ready||!dst||stride<=0||height<=0||scale<1||scale>8)return;
  unsigned code=encode(c);
  for(unsigned half=0;half<2;half++){
    unsigned word=pairs[code][half],tile=word&1023;
    if(tile>=512)continue;
    for(unsigned yy=0;yy<8;yy++)for(unsigned xx=0;xx<8;xx++){
      unsigned sy=word&0x8000?7-yy:yy,sx=word&0x4000?7-xx:xx;
      if(tiles[tile][sy][sx]!=3)continue;
      for(int dy=0;dy<scale;dy++)for(int dx=0;dx<scale;dx++)
        pixel(dst,stride,height,x+(int)xx*scale+dx,
            y+(int)(half*8+yy)*scale+dy,0xffffffff);
    }
  }
}
void LagoonDialogueFont_DrawBox(uint32_t *dst,int stride,int height,int x,int y,
    int width,int box_height,int scale){
  if(!ready||!dst||stride<=0||height<=0||scale<1||scale>8||
      width<16*scale||box_height<16*scale)return;
  int span=8*scale;
  for(int yy=0;yy<box_height;yy++)for(int xx=0;xx<width;xx++){
    unsigned tile=0,sx=(unsigned)(xx%span)/scale,sy=(unsigned)(yy%span)/scale;
    int left=xx<span,right=xx>=width-span,top=yy<span,bottom=yy>=box_height-span;
    if(right)sx=(unsigned)(xx-(width-span))/scale;
    if(bottom)sy=(unsigned)(yy-(box_height-span))/scale;
    if(top)tile=left?0x16c:right?0x16e:0x16d;
    else if(bottom)tile=left?0x17c:right?0x17e:0x17d;
    else if(left)tile=0x16f;else if(right)tile=0x17f;
    unsigned color=tile?tiles[tile][sy][sx]:1;
    pixel(dst,stride,height,x+xx,y+yy,color==3?0xffffffff:color==2?0xff303030:0xff000000);
  }
}
