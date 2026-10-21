/* Read-only decoding of an original queued multipart boss display carrier.
 * The original C284/C2CD/kind handlers remain responsible for all guest work.
 */
#include "lagoon_wide_multipart.h"
#include <string.h>

static unsigned word(const uint8_t *p,unsigned a){return p[a]|(unsigned)p[a+1]<<8;}
static unsigned at(unsigned a){return ((a>>16)&127)*32768+(a&32767);}
static int source(const uint8_t *rom,size_t size,unsigned a,unsigned n,uint64_t expected){
  unsigned o=at(a);uint64_t h=UINT64_C(14695981039346656037);
  if(o>size || n>size-o)return 0;
  for(unsigned i=0;i<n;++i)h=(h^rom[o+i])*UINT64_C(1099511628211);
  return h==expected;
}
static int allowed(const uint8_t *rom,size_t size,unsigned room,unsigned kind){
  static const struct {unsigned room,kind,record;uint64_t record_hash;
      unsigned pointer;uint64_t pointer_hash;} authored[]={
    {0x42,2,0x05acd2,UINT64_C(0x41c06a1e152456bb),0x05c17a,UINT64_C(0xf241d31c709bb01a)},
    {0x43,3,0x05ace6,UINT64_C(0x9a36f6c014538ba4),0x05c1b2,UINT64_C(0x33f91f1b7500c1be)},
    {0x43,4,0x05acef,UINT64_C(0x86678a304fd5eeab),0x05c1b2,UINT64_C(0x33f91f1b7500c1be)},
    {0x43,5,0x05acf8,UINT64_C(0x1a0c6dc17f458076),0x05c1b2,UINT64_C(0x33f91f1b7500c1be)},
    {0x44,6,0x05ad03,UINT64_C(0x089c3ddb3f0e46f7),0x05c1ea,UINT64_C(0xe0dc761870077f98)},
    {0x45,7,0x05ad4d,UINT64_C(0x03ad397291e72aea),0x05c222,UINT64_C(0x1b7e4c19b2f02ade)},
    {0x45,8,0x05ad56,UINT64_C(0x929ada11be329b43),0x05c222,UINT64_C(0x1b7e4c19b2f02ade)},
    {0x45,9,0x05ad5f,UINT64_C(0x43ee347fabd72376),0x05c222,UINT64_C(0x1b7e4c19b2f02ade)},
    {0x46,10,0x05ad73,UINT64_C(0x9fbd3a62250297b6),0x05c25a,UINT64_C(0x833006195db19e68)},
    {0x46,15,0x05ad85,UINT64_C(0xa8cabc69155aa9c3),0x05c25a,UINT64_C(0x833006195db19e68)},
    {0x48,14,0x05adb6,UINT64_C(0x09b048dc6da03fc7),0x05c2ca,UINT64_C(0x90c8ab1a869af8eb)}
  };
  for(unsigned i=0;i<sizeof authored/sizeof *authored;++i)
    if(room==authored[i].room&&kind==authored[i].kind)
      return source(rom,size,authored[i].record,9,authored[i].record_hash)&&
        source(rom,size,authored[i].pointer,3,authored[i].pointer_hash);
  return 0;
}
static int append(const uint8_t *rom,size_t size,unsigned selector,unsigned frame,
    int x,int y,int palette,LagoonWideCell out[256],unsigned *total){
  if(!LagoonWideEntities_CurrentSource(rom,size,selector,frame))return 0;
  int n=LagoonWideEntities_Decode(rom,size,selector,frame,x,y,out+*total,256-*total);
  if(n<0)return 0;
  if(palette)for(int i=0;i<n;++i)out[*total+i].tile=(out[*total+i].tile&~0xe00)|0xe00;
  *total+=(unsigned)n;return 1;
}
int LagoonWideMultipart_PacketCells(const uint8_t *rom,size_t size,
    const uint8_t *packet,unsigned queue,LagoonWideCell *cells,size_t capacity){
  if(!rom||size!=0x100000||!packet||!cells||queue>=64||
      word(packet,0x48c)>64||queue>=word(packet,0x48c))return -1;
  unsigned q=0xec0+queue*6,offset=word(packet,q),room=word(packet,0x48e);
  /* Original M1 sort/dispatch reads the type BYTE. The following queue byte
   * is unused padding and remains FF in genuine loaded boss packets. */
  if(offset<0x40||offset>0x800||(offset&63)||packet[q+4]!=3)return -1;
  const uint8_t *r=packet+0x500+offset;
  unsigned kind=r[27];
  if(r[24]!=3||(r[0]&0xc0)!=0xc0||!allowed(rom,size,room,kind)||
      word(packet,q+2)!=word(r,14)||
      word(r,12)!=((word(r,2)-word(packet,0x480))&65535)||
      word(r,14)!=((word(r,4)-word(packet,0x482))&65535))return -1;
  /* Whole original dispatch, optional layers, main handlers, offsets and
   * hardware-divide projection. CurrentSource additionally binds all decoded
   * frame table pointers and streams. No arbitrary room or frame acceptance. */
  if(!source(rom,size,0x01c284,407,UINT64_C(0xda4d72bc9efd2cf6))||
      !source(rom,size,0x01fd9f,128,UINT64_C(0xdb1178af8533bc53))||
      !source(rom,size,0x01c026,164,UINT64_C(0x1af59bf53030a61c))||
      !source(rom,size,0x01c486,41,UINT64_C(0x2eb39b5302337bbe)))return -1;
  int x=(int16_t)word(r,12),y=(int16_t)(word(r,14)-2*(r[21]/3));
  LagoonWideCell result[256];unsigned total=0;
  if(r[1]&1)for(unsigned i=0;i<3;++i){
    unsigned layer=r[59+i];if(layer==16)continue;
    unsigned o=at(0x01fdff)+(layer&7)*4;
    int lx=(int16_t)(x+(int16_t)word(rom,o));
    int ly=(int16_t)(y+(int16_t)word(rom,o+2));
    if(!append(rom,size,0,r[53+i],lx,ly,0,result,&total))return -1;
  }
  /* Original C3A0 suppresses kind7 main body while global55F bit0 is set.
   * Other captured parts and optional layers retain their original order. */
  if(kind!=7||!(packet[0x55f]&1)){
    static const uint8_t offsets[16]={0,0,0x34,0x38,0x3c,0x3c,0x40,0x44,
      0x48,0x48,0x4c,0x50,0x54,0x58,0x5c,0x4c};
    unsigned o=at(0x01fd9f)+offsets[kind];
    int bx=(int16_t)(x+(int16_t)word(rom,o));
    int by=(int16_t)(y+(int16_t)word(rom,o+2));
    if(!append(rom,size,room+1,r[28],bx,by,(r[1]&0x80)||(r[31]&2),result,&total))return -1;
  }
  if(total>capacity)return -1;
  memcpy(cells,result,total*sizeof(*cells));return (int)total;
}
