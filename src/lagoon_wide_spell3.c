/* Original 02:A5BA presents a finite32x32 BG image, not a wider map. Preserve
 * that image and extend only its proven transparent canvas with the current
 * PPU backdrop. Original black/white palette pulses keep their original cadence. */
#include "lagoon_wide_spell3.h"
#include "snes/ppu.h"
#include <string.h>
extern unsigned char g_snesrecomp_last_hdmaen;
enum { kGraphicsBytes=0x1260 };
static struct {
  uint8_t graphics[kGraphicsBytes],key[32];
  uint64_t generation;
  unsigned stage,next,left,kind,completed;
} s_spell;
static unsigned word(const uint8_t *p,unsigned a){return p[a]|p[a+1]<<8;}
static unsigned offset(unsigned a){return (a>>16)*32768+(a&32767);}
static uint64_t hash(const uint8_t *p,unsigned n){
  uint64_t h=UINT64_C(14695981039346656037);
  while(n--)h=(h^*p++)*UINT64_C(1099511628211);return h;
}
static int source_ok(const uint8_t *rom,size_t size) {
  static const struct {unsigned address,length;uint64_t hash;} spans[]={
    {0x018ea6,0x190,UINT64_C(0x0cb2bedbac90a068)},
    {0x0088cd,0xe0,UINT64_C(0x595ded927e5ddcd6)},
    {0x008d82,0x99,UINT64_C(0xc9debd1d7d3a653a)},
    {0x009143,445,UINT64_C(0x734e7c5c9157de09)},
    {0x00908a,81,UINT64_C(0xa4d117801c8e3fec)},
    {0x00884b,56,UINT64_C(0x2b1eec1eca0b79f0)},
    {0x0089a7,13,UINT64_C(0x81364db374fb949a)},
    {0x0087b4,120,UINT64_C(0xeeb909fdbd678929)},
    {0x008884,0x49,UINT64_C(0x681e81559f64d4f9)},
    {0x0084de,0xb,UINT64_C(0xf8b8e33fd34a2075)},
    {0x0195b2,0xb8,UINT64_C(0xff0b67d274847a1a)},
    {0x008e2c,6,UINT64_C(0x9a668cbaf5e0b6ed)},
    {0x05b2df,56*75,UINT64_C(0x136a6a85d3d45cb2)},
    {0x02a3f0,0x2f,UINT64_C(0xf4aea983061c4553)},
    {0x02a5ba,0xcb,UINT64_C(0x14e4c8ff40bc1ebc)},
    {0x02b150,0x23,UINT64_C(0xcc4aec7b1eba2352)},
    {0x00996c,21,UINT64_C(0xaa829a0921ef4bc0)},
    {0x00fd53,12,UINT64_C(0x7b1581b7579549a1)},
    {0x11f37d,2048,UINT64_C(0x821048b1c5e90328)},
    {0x11d0ac,3256,UINT64_C(0xf1529f9a85eaaed1)}
  };
  if(!rom || size!=0x100000)return 0;
  for(unsigned i=0;i<sizeof spans/sizeof spans[0];++i)
    if(hash(rom+offset(spans[i].address),spans[i].length)!=spans[i].hash)return 0;
  return 1;
}
/* Stable original descriptor fields only. DP51 and CPU frame/input flags are
 * intentionally excluded: the CPU may already be preparing its next operation. */
static int scene_key(const uint8_t *rom,const uint8_t *r,uint8_t key[32]) {
  static const uint8_t fields[]={0x50,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
    0x5d,0x5e,0x5f,0x69,0x6a,0x6b,0x75,0x76,0x77,0x7e,0x7f,0x80,0x81,0x82,0x83};
  unsigned room=word(r,0x48e);
  if(room>=75 || word(r,0x88)!=0 || r[0x8a]!=0x7f ||
      word(r,0x8b)!=0x4000 || r[0x8d]!=0x7f)return 0;
  const uint8_t *d=rom+offset(0x05b2df)+room*56;
  for(unsigned i=0;i<sizeof fields;++i) {
    unsigned a=fields[i];if(r[a]!=d[a-0x50])return 0;key[i]=r[a];
  }
  memcpy(key+23,r+0x88,6);key[29]=r[0x48e];key[30]=r[0x48f];key[31]=0;
  return 1;
}
void LagoonWideSpell3_Reset(void){memset(&s_spell,0,sizeof s_spell);}
/* World maps share these destinations before/after the finite spell. Their
 * writes revoke a spell canvas, not a different, untouched graphics resource.
 * A new canvas still needs all three original map jobs in their original order. */
static void withdraw_maps(void) {
  if(s_spell.stage>=2 && !(s_spell.left&&s_spell.kind<=2)) {
    s_spell.stage=2;s_spell.next=s_spell.left=s_spell.kind=s_spell.completed=0;
  }else LagoonWideSpell3_Reset();
}
static int graphics_address(unsigned a){return a>=0x6000&&a<0x7260;}
static int owned(unsigned a){return (a>=0x6000&&a<0x7260)||
    (a>=0xe000&&a<0xe800)||(a>=0xf000&&a<0x10000);}
void LagoonWideSpell3_Dma(const uint8_t *rom,size_t size,const uint8_t *r,
    uint64_t generation,unsigned channel,unsigned bank,unsigned source,
    unsigned destination,unsigned length,int original_mode) {
  unsigned bytes=length?length:65536,first=(destination&32767)*2;int touches=0,graphics=0;
  for(unsigned n=0;n<bytes;++n) {unsigned a=(first+n)&65535;
    if(owned(a))touches=1;if(graphics_address(a))graphics=1;
  }
  if(!touches)return;
  int first_map=r && original_mode && channel==1 && bank==0x11 && source==0xf37d &&
    destination==0x7000 && length==0x800 && r[0x537]==3 && r[0x417]==1;
  /* Before acquisition these are ordinary world destinations. In particular
   * original8F8C can update7000 after EA6 loaded the independent3000 pack. */
  if(!graphics && s_spell.stage<=2 && !s_spell.left && !first_map)return;
  uint8_t key[32];
  if(!r || !original_mode || channel!=1 || !source_ok(rom,size) ||
      !scene_key(rom,r,key)) {if(graphics)LagoonWideSpell3_Reset();else withdraw_maps();return;}
  /* A fresh original loader starts a new owner; its second transfer cannot
   * borrow readiness from a previous spell or partial decompression. */
  if(bank==0x7f && source==0x7f80 && destination==0x3000 && length==0x1000 && r[0x537]==3) {
    LagoonWideSpell3_Reset();s_spell.generation=generation;memcpy(s_spell.key,key,32);
    s_spell.kind=1;
  }else {
    if(s_spell.left || s_spell.generation!=generation || memcmp(s_spell.key,key,32)) {
      if(graphics)LagoonWideSpell3_Reset();else withdraw_maps();return;
    }
    if(s_spell.stage==1 && bank==0x7f && source==0x8f80 && destination==0x3800 && length==0x260)
      s_spell.kind=2;
    else if(s_spell.stage>=2 && bank==0x11 && source==0xf37d && destination==0x7000 && length==0x800 &&
        r[0x537]==3 && r[0x417]==1)s_spell.kind=3;
    else if(s_spell.stage==3 && bank==0x7e && source==0x6000 && destination==0x7c00 && length==0x800)
      s_spell.kind=4;
    else if(s_spell.stage==4 && bank==0x7e && source==0x6000 && destination==0x7800 && length==0x1000)
      s_spell.kind=5;
    else {if(graphics)LagoonWideSpell3_Reset();else withdraw_maps();return;}
  }
  s_spell.next=first;s_spell.left=length;s_spell.completed=0;
  if(s_spell.kind==3)s_spell.stage=2;
}
void LagoonWideSpell3_Write(unsigned a,uint8_t value) {
  if(!owned(a)) {if(s_spell.left)withdraw_maps();return;}
  if(!s_spell.left || a!=s_spell.next) {
    if(graphics_address(a))LagoonWideSpell3_Reset();
    else if(s_spell.stage>=3 || s_spell.left)withdraw_maps();
    return;
  }
  if(s_spell.kind<=2)s_spell.graphics[a-0x6000]=value;
  else {
    /* The actual original map bytes are rechecked at draw time; blank writes
     * must already match their source and cannot create transient readiness. */
    if(s_spell.kind>=4 && value!=((a&1)?1:255)){withdraw_maps();return;}
  }
  ++s_spell.next;++s_spell.completed;
  if(!--s_spell.left) {
    s_spell.stage=s_spell.kind;
    if(s_spell.stage==2 && hash(s_spell.graphics,kGraphicsBytes)!=UINT64_C(0xce8541573afdbdeb))
      LagoonWideSpell3_Reset();
  }
}
static int layout(const Ppu *p) {
  if(!p || (p->inidisp&0x80) || p->bgmode!=9 || p->obsel!=2 ||
      p->bgXsc[0]!=0x71 || (p->bgXsc[1]!=0x79&&p->bgXsc[1]!=0x7a) ||
      p->bgXsc[2]!=0x3c || p->bgTileAdr!=0x600 || p->setini || p->mosaic ||
      p->windowsel || p->wbgobjlog || p->screenWindowed[0] || p->screenWindowed[1] ||
      p->screenEnabled[0]!=31 || p->screenEnabled[1]!=31 || p->cgadsub ||
      (p->cgwsel&~0x30))return 0;
  for(unsigned i=0;i<3;++i)if(p->hScroll[i]||p->vScroll[i])return 0;
  return 1;
}
static void native_objects(const Ppu *p,uint8_t occupied[224][256]) {
  const uint8_t *o=(const uint8_t*)p->oam;
  for(unsigned slot=0;slot<128;++slot) {
    unsigned a=slot*4,h=(p->highOam[slot/4]>>((slot&3)*2))&3;
    int sx=o[a]|((h&1)<<8),sy=o[a+1],side=(h&2)?16:8;
    if(sx>=384)sx-=512;if(sy>=240)sy-=256;
    int x0=sx<0?0:sx,y0=sy<0?0:sy,x1=sx+side>256?256:sx+side,y1=sy+side>224?224:sy+side;
    if(x0<x1)for(int y=y0;y<y1;++y)memset(occupied[y]+x0,1,(size_t)(x1-x0));
  }
}
/* Rendering latches can precede the current CPU phase. A source-transparent
 * native cell is a refusal witness, never the source of new pixels. Do not
 * infer its background through an overlapping original object. */
static int displayed_backdrop(const uint8_t *dst,size_t pitch,unsigned left,
    const Ppu *p,const uint16_t *v,unsigned rgb) {
  unsigned halves[2]={0};uint8_t occupied[224][256]={{0}};native_objects(p,occupied);
  for(unsigned y=0;y<224;++y)for(unsigned x=0;x<256;++x) {
    /* PPU output row0 scans native line1, hence source Y=displayY+1. */
    if((v[0x7000+((y+1)/8)*32+x/8]&0x3ff)!=0x392 || occupied[y][x])continue;
    const uint32_t *row=(const uint32_t*)(dst+y*pitch);
    if((row[left+x]&0xffffff)!=rgb)return 0;
    ++halves[x>=128];
  }
  return halves[0]>=128&&halves[1]>=128;
}
int LagoonWideSpell3_DrawMargins(uint8_t *dst,size_t pitch,unsigned width,
    unsigned height,const Ppu *p,const uint8_t *rom,size_t size,
    const uint8_t *r,uint64_t generation) {
  uint8_t key[32];
  if(!dst || !r || width<=256 || width>800 || height!=224 || pitch<width*4 ||
      s_spell.stage!=5 || s_spell.left || generation!=s_spell.generation ||
      g_snesrecomp_last_hdmaen || !layout(p) || !source_ok(rom,size) || !scene_key(rom,r,key) ||
      memcmp(key,s_spell.key,32))return 0;
  const uint16_t *v=PpuRenderVram(p);
  if(memcmp(v+0x3000,s_spell.graphics,kGraphicsBytes) ||
      memcmp(v+0x7000,rom+offset(0x11f37d),2048))return 0;
  for(unsigned i=0x7800;i<0x8000;++i)if(v[i]!=0x1ff)return 0;
  /* Original BG2 blank-map character and authored BG1 border are transparent.
   * BG3 uses the still-current HUD map3C00, not the uploaded BG2 page7C00.
   * In Mode1 its2bpp colors can only index0..31; the uniform pulse palette
   * below therefore makes every possible BG3 pixel equal to the backdrop. */
  for(unsigned i=0x1ff0;i<0x2000;++i)if(v[i])return 0;
  for(unsigned i=0x3920;i<0x3930;++i)if(v[i])return 0;
  unsigned color=p->cgram[0];if(color!=0&&color!=0x7fff)return 0;
  for(unsigned i=1;i<122;++i)if(p->cgram[i]!=color)return 0;
  for(unsigned i=0;i<6;++i)if(p->cgram[122+i]!=word(rom,offset(0x00fd53)+i*2))return 0;
  unsigned brightness=p->inidisp&15,rgb=0;
  for(unsigned c=0;c<3;++c){unsigned x=(color>>(c*5))&31;
    rgb|=(((x<<3)|(x>>2))*brightness/15)<<(16-c*8);}
  unsigned left=(width-256)/2;
  if(!displayed_backdrop(dst,pitch,left,p,v,rgb))return 0;
  for(unsigned y=0;y<height;++y) {
    uint32_t *row=(uint32_t*)(dst+y*pitch);
    for(unsigned x=0;x<left;++x)row[x]=rgb;
    for(unsigned x=left+256;x<width;++x)row[x]=rgb;
  }
  return 1;
}
