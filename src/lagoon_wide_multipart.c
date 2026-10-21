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
    {0x40,0,0x05acb3,UINT64_C(0x82c7acff4fb1bd7d),0x05c10a,UINT64_C(0xaa256a1a94cb4dfd)},
    {0x41,1,0x05acc7,UINT64_C(0x8f10c9392d93cc48),0x05c142,UINT64_C(0x409a361c0d2fb521)},
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
int LagoonWideMultipart_PropDrawOffset(const uint8_t *rom,size_t size,
    const uint8_t *p,unsigned queue,int *draw_offset){
  if(!rom||size!=0x100000||!p||!draw_offset||word(p,0x48e)!=0x40||
      word(p,0x48c)>64||queue>=word(p,0x48c)||queue>=64)return 0;
  unsigned q=0xec0+queue*6;
  const uint8_t *r=p+0x580;unsigned phase=p[0x3af];
  if(word(p,q)!=0x80||p[q+4]!=5||r[24]!=5||r[27]!=9||r[28]!=0x3e||
      (r[0]&0xc0)!=0xc0||word(r,2)!=128||word(r,4)!=60||
      word(p,q+2)!=word(r,14)||
      word(r,12)!=((word(r,2)-word(p,0x480))&65535)||
      word(r,14)!=((word(r,4)-word(p,0x482))&65535)||
      !p[0x3ae]||phase<1||phase>42)return 0;
  if(!source(rom,size,0x0180f1,12,UINT64_C(0xbe646d510b3b48ea))||
      !source(rom,size,0x01c026,164,UINT64_C(0x1af59bf53030a61c))||
      !source(rom,size,0x01d8da,31,UINT64_C(0xe5fb5c1ea62cc247))||
      !source(rom,size,0x01d979,2,UINT64_C(0x0983a707b60a6ebf))||
      !source(rom,size,0x01ddad,100,UINT64_C(0xbd87810c8f70831c))||
      !source(rom,size,0x01de11,84,UINT64_C(0x9ee85f4097be1a2c))||
      !source(rom,size,0x05acbc,9,UINT64_C(0x9240a936949bd754))||
      !source(rom,size,0x05c10a,3,UINT64_C(0xaa256a1a94cb4dfd)))return 0;
  unsigned table=at(0x01de11);
  if(word(p,0x90)!=word(rom,table+(phase-1)*2)||word(p,0x92)!=word(p,0x90))return 0;
  if(phase==1){
    /* The FF activation can start from the resting zero offset. Its first
     * post-draw DDAD update is already -4 at the DMA snapshot. Zero is only
     * a finite native-anchored candidate: the caller must match every cell;
     * interrupted/nonzero prior shakes therefore still refuse as a group. */
    if(p[0x3ae]!=255||
        !source(rom,size,0x029072,46,UINT64_C(0xd5533e0030732ac3))||
        !source(rom,size,0x029f70,88,UINT64_C(0x464f6e34b6a60891)))return 0;
    *draw_offset=0;return 1;
  }
  *draw_offset=(int16_t)word(rom,table+(phase-2)*2);return 1;
}
static int append(const uint8_t *rom,size_t size,unsigned selector,unsigned frame,
    int x,int y,int palette,LagoonWideCell out[256],unsigned *total){
  /* Original8B71 begins CMP#FF/BNE/RTL. A hidden component neither reads its
   * frame table nor owns pixels; remaining actual components still form one
   * transactional carrier. The complete generic source is checked below. */
  if(frame==255)return 1;
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
      (kind<2&&offset!=0x40)||
      word(packet,q+2)!=word(r,14)||
      word(r,12)!=((word(r,2)-word(packet,0x480))&65535)||
      word(r,14)!=((word(r,4)-word(packet,0x482))&65535))return -1;
  /* Whole original dispatch, optional layers, main handlers, offsets and
   * hardware-divide projection. CurrentSource additionally binds all decoded
   * frame table pointers and streams. No arbitrary room or frame acceptance. */
  if(!source(rom,size,0x01c284,407,UINT64_C(0xda4d72bc9efd2cf6))||
      !source(rom,size,0x01fd9f,128,UINT64_C(0xdb1178af8533bc53))||
      !source(rom,size,0x01c026,164,UINT64_C(0x1af59bf53030a61c))||
      !source(rom,size,0x008b4b,331,UINT64_C(0x679426345d98810a))||
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
  if(kind==0){
    /* C326 uses globals in the original primary boss slot only. Its first
     * component precedes C402, so it retains selector0/default palette. */
    if(r[1]&4){
      unsigned o=at(0x01fd9f)+16;
      if(!append(rom,size,0,r[53],(int16_t)(x+(int16_t)word(rom,o)),
          (int16_t)(y+(int16_t)word(rom,o+2)),0,result,&total))return -1;
    }
    unsigned o=at(0x01fd9f)+(r[28]&3)*4;
    int palette=(r[1]&0x80)||(r[31]&2);
    if(!append(rom,size,room+1,r[28],(int16_t)(x+(int16_t)word(rom,o)),
        (int16_t)(y+(int16_t)word(rom,o+2)),palette,result,&total))return -1;
    if(r[1]&0x20){
      /* Original C356 replaces DP42 with uncorrected global54E for this
       * component only; it does not borrow the body jump-height anchor. */
      o=at(0x01fd9f)+20;
      if(!append(rom,size,room+1,8,(int16_t)(x+(int16_t)word(rom,o)),
          (int16_t)(word(r,14)+(int16_t)word(rom,o+2)),palette,result,&total))return -1;
    }
  }else if(kind==1){
    /* C368 skips projection only for signed-negative frames. FF is the
     * source-proven no-draw sentinel; other unqualified frames refuse. */
    if(r[28]!=255){
      if(r[28]&0x80)return -1;
      unsigned o=at(0x01fd9f)+24+r[28]*4;
      if(!append(rom,size,room+1,r[28],(int16_t)(x+(int16_t)word(rom,o)),
          (int16_t)(y+(int16_t)word(rom,o+2)),(r[1]&0x80)||(r[31]&2),result,&total))return -1;
    }
  /* Original C3A0 suppresses kind7 main body while global55F bit0 is set.
   * Other captured parts and optional layers retain their original order. */
  }else if(kind!=7||!(packet[0x55f]&1)){
    static const uint8_t offsets[16]={0,0,0x34,0x38,0x3c,0x3c,0x40,0x44,
      0x48,0x48,0x4c,0x50,0x54,0x58,0x5c,0x4c};
    unsigned o=at(0x01fd9f)+offsets[kind];
    int bx=(int16_t)(x+(int16_t)word(rom,o));
    int by=(int16_t)(y+(int16_t)word(rom,o+2));
    if(!append(rom,size,room+1,r[28],bx,by,(r[1]&0x80)||(r[31]&2),result,&total))return -1;
  }
  /* Original8C7F checks the signed HIGH byte, then low>=F0 for negative
   * positions. This includes negative16-bit aliases below-256: their actual
   * OAM Y is the top-edge lowbyte, not an extrapolated offscreen position. */
  for(unsigned i=0;i<total;++i){
    unsigned sy=(uint16_t)result[i].y;
    if((sy&0x8000)&&(sy&255)>=240)result[i].y=(int)(sy&255)-256;
  }
  if(total>capacity)return -1;
  memcpy(cells,result,total*sizeof(*cells));return (int)total;
}
