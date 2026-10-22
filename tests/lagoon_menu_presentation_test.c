/* Unit layout vectors are synthetic. Optional external dump prefix exercises
 * real identity/resources and writes private original-pixel PPM previews. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/lagoon_menu_presentation.c"
static uint32_t native[256*224];
static uint32_t color(unsigned x,unsigned y) {return ((0x40u+y%64)<<24)|((x*7919+y*101)&0xFFFFFFu);}
static void synthetic(void) {
  for(unsigned i=0;i<256*224;++i)native[i]=0x65000000u;
  for(unsigned y=0;y<224;++y)for(unsigned x=8;x<248;++x)
    if(x<40 || x>=216 || (y>=32 && y<200))native[y*256+x]=color(x,y);
  for(unsigned y=0;y<224;++y)if(y<32 || y>=200) {
    for(unsigned x=40;x<88;++x)native[y*256+x]=color(40+(x-40)%24,y);
    for(unsigned x=88;x<168;++x)native[y*256+x]=color(x,y);
    for(unsigned x=168;x<216;++x)native[y*256+x]=color(168+(x-168)%24,y);
  }
}
static void read_exact(const char *prefix,const char *suffix,void *dst,size_t size) {
  char path[2048];snprintf(path,sizeof(path),"%s.%s",prefix,suffix);FILE *f=fopen(path,"rb");assert(f);assert(fread(dst,1,size,f)==size);assert(fgetc(f)==EOF);fclose(f);
}
static void ppm(const char *root,unsigned width,const uint8_t *pixels,size_t pitch) {
  char path[2048];snprintf(path,sizeof(path),"%s/%u.ppm",root,width);FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n%u 224\n255\n",width);
  for(unsigned y=0;y<224;++y)for(unsigned x=0;x<width;++x){const uint8_t *p=pixels+y*pitch+x*4;fputc(p[2],f);fputc(p[1],f);fputc(p[0],f);}fclose(f);
}
int main(int argc,char **argv) {
  unsigned widths[]={256,258,280,308,342,448,684,800};
  synthetic();uint32_t original[256*224];memcpy(original,native,sizeof(native));
  for(unsigned i=0;i<sizeof(widths)/sizeof(widths[0]);++i) {
    unsigned w=widths[i],e=(w-256)/2;size_t pitch=(w+7)*4;
    uint32_t *out=malloc(pitch*224);assert(out);memset(out,0xA5,pitch*224);
    assert(layout((uint8_t *)out,pitch,native,w,LAGOON_MENU_ITEMS));
    if(w==256)for(unsigned y=0;y<224;++y)assert(!memcmp((uint8_t *)out+y*pitch,native+y*256,1024));
    for(unsigned y=32;y<200;++y)for(unsigned x=40;x<216;++x)
      assert(out[y*(w+7)+e+x]==native[y*256+x]);
    for(unsigned y=0;y<224;++y)for(unsigned x=w;x<w+7;++x)assert(out[y*(w+7)+x]==0xA5A5A5A5u);
    for(unsigned y=0;y<32;++y)assert(!memcmp(out+y*(w+7)+e+88,native+y*256+88,80*4));
    assert(!memcmp(original,native,sizeof(native)));free(out);
  }
  /* Equipment's longer title and lower divider use their own source joins. */
  synthetic();
  for(unsigned y=154;y<156;++y)for(unsigned x=24;x<232;++x)native[y*256+x]=color(24,y);
  for(unsigned i=0;i<sizeof(widths)/sizeof(widths[0]);++i) {
    unsigned w=widths[i],e=(w-256)/2;size_t pitch=(w+7)*4;
    uint32_t *out=malloc(pitch*224);assert(out);memset(out,0xA5,pitch*224);
    assert(layout((uint8_t *)out,pitch,native,w,LAGOON_MENU_EQUIPMENT));
    for(unsigned y=32;y<200;++y)for(unsigned x=40;x<224;++x)
      assert(out[y*(w+7)+e+x]==native[y*256+x]);
    for(unsigned y=0;y<32;++y)assert(!memcmp(out+y*(w+7)+e+56,native+y*256+56,144*4));
    for(unsigned y=154;y<156;++y)for(unsigned x=e+24;x<e+232;++x)assert(out[y*(w+7)+x]==native[y*256+40]);
    native[8*256+40]^=1;memset(out,0xA5,pitch*224);
    assert(!layout((uint8_t *)out,pitch,native,w,LAGOON_MENU_EQUIPMENT));
    for(unsigned n=0;n<(w+7)*224;++n)assert(out[n]==0xA5A5A5A5u);
    native[8*256+40]^=1;native[154*256+24]^=1;
    assert(!layout((uint8_t *)out,pitch,native,w,LAGOON_MENU_EQUIPMENT));native[154*256+24]^=1;
    free(out);
  }
  Ppu *p=calloc(1,sizeof(Ppu));uint8_t *ram=calloc(1,0x20000);assert(p&&ram);
  assert(LagoonMenuPresentation_Identify(p,ram)==LAGOON_MENU_NONE);
  if(argc==3) {
    read_exact(argv[1],"vram.bin",p->vram,sizeof(p->vram));read_exact(argv[1],"wram.bin",ram,0x20000);read_exact(argv[1],"fb.bgrx",native,sizeof(native));
    p->inidisp=15;p->bgmode=9;p->bgTileAdr=0x600;p->bgXsc[0]=0x71;p->bgXsc[1]=0x79;p->bgXsc[2]=0x3C;p->screenEnabled[0]=p->screenEnabled[1]=31;
    LagoonMenuPanel panel=LagoonMenuPresentation_Identify(p,ram);assert(panel!=LAGOON_MENU_NONE);
    p->vram[0]^=1;assert(LagoonMenuPresentation_Identify(p,ram)==LAGOON_MENU_NONE);p->vram[0]^=1;
    p->vram[0x7000]^=1;assert(LagoonMenuPresentation_Identify(p,ram)==LAGOON_MENU_NONE);p->vram[0x7000]^=1;
    ram[0x275]^=1;assert(LagoonMenuPresentation_Identify(p,ram)==LAGOON_MENU_NONE);ram[0x275]^=1;
    p->bgmode^=1;assert(LagoonMenuPresentation_Identify(p,ram)==LAGOON_MENU_NONE);p->bgmode^=1;
    p->hScroll[0]=1;assert(LagoonMenuPresentation_Identify(p,ram)==LAGOON_MENU_NONE);p->hScroll[0]=0;
    p->inidisp=0x80;assert(LagoonMenuPresentation_Identify(p,ram)==LAGOON_MENU_NONE);p->inidisp=15;
    ram[0xE0]^=1;assert(LagoonMenuPresentation_Identify(p,ram)==LAGOON_MENU_NONE);ram[0xE0]^=1;
    p->vram[0x7000+32+13]^=1;assert(LagoonMenuPresentation_Identify(p,ram)==LAGOON_MENU_NONE);p->vram[0x7000+32+13]^=1;
    for(unsigned i=1;i<sizeof(widths)/sizeof(widths[0]);++i) {
      unsigned w=widths[i],e=(w-256)/2;size_t pitch=(w+7)*4;uint8_t *field=calloc(224,pitch),*out=malloc(224*pitch);assert(field&&out);memset(out,0xA5,224*pitch);
      for(unsigned y=0;y<224;++y)memcpy(field+y*pitch+e*4,native+y*256,1024);
      assert(LagoonMenuPresentation_Draw(out,pitch,field,pitch,w,224,p,ram,1));
      unsigned content_width=panel==LAGOON_MENU_EQUIPMENT?184:176;
      for(unsigned y=32;y<200;++y)assert(!memcmp(out+y*pitch+(e+40)*4,native+y*256+40,content_width*4));
      for(unsigned y=0;y<224;++y)for(unsigned x=w*4;x<pitch;++x)assert(out[y*pitch+x]==0xA5);
      unsigned title_left=panel==LAGOON_MENU_EQUIPMENT?56:88;
      unsigned title_width=panel==LAGOON_MENU_EQUIPMENT?144:80;
      for(unsigned y=0;y<32;++y)assert(!memcmp(out+y*pitch+(e+title_left)*4,native+y*256+title_left,title_width*4));
      /* Every composed pixel must be one original pixel from the same row. */
      for(unsigned y=0;y<224;++y)for(unsigned x=0;x<w;++x) {
        uint32_t pixel=((uint32_t *)(out+y*pitch))[x];unsigned found=0;
        for(unsigned sx=0;sx<256;++sx)if(pixel==native[y*256+sx]){found=1;break;}
        assert(found);
      }
      ppm(argv[2],w,out,pitch);
      assert(LagoonMenuPresentation_Draw(field,pitch,field,pitch,w,224,p,ram,1));
      for(unsigned y=0;y<224;++y)assert(!memcmp(field+y*pitch,out+y*pitch,w*4));
      memset(out,0xA5,224*pitch);assert(!LagoonMenuPresentation_Draw(out,pitch,field,pitch,w,224,p,ram,0));
      for(unsigned y=0;y<224*pitch;++y)assert(out[y]==0xA5);
      p->setini=1;assert(!LagoonMenuPresentation_Draw(out,pitch,field,pitch,w,224,p,ram,1));p->setini=0;
      for(unsigned y=0;y<224*pitch;++y)assert(out[y]==0xA5);
      free(out);free(field);
    }
    printf("PASS actual panel=%u identity/mutation guards, source contents, widths, padded rows, in-place safety and disabled refusal\n",(unsigned)panel);
  }
  free(p);free(ram);puts("PASS synthetic min/native/eight widths, pixel/alpha identity, title centering, padded rows, immutable source");return 0;
}
