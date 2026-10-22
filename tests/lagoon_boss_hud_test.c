/* External actual original-room/HP snapshots. No game data embedded. The
 * optional generated preview is private; gameplay and clocks stay untouched. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/lagoon_boss_hud.c"
#define main sdk_ppu_fixture_main
#define p sdk_ppu_fixture_p
#define frame sdk_ppu_fixture_frame
#define setup sdk_ppu_fixture_setup
#define pixels sdk_ppu_fixture_pixels
#include "../snesrecomp/tests/ppu/ppu_extra_object_test.c"
#undef pixels
#undef setup
#undef frame
#undef p
#undef main
static uint8_t rom[0x100000],ram[0x20000],before[0x20000];static Ppu p;
static uint32_t field[256*224],out[256*224];
static uint32_t oracle[256*224];
static uint8_t wide_field[224*(800*4+16)],wide_out[sizeof wide_field];
static void observe(void){LagoonBossHud_ObserveField(&p,ram,71,123,0);}
static void refuse_ram(unsigned a,unsigned change){
  uint8_t old=ram[a];ram[a]^=change;observe();assert(!packet.valid);ram[a]=old;
}
static void refusals(void){
  unsigned addresses[]={0x48e,0x50,0x9a,0x100e,0x558,0x55b};
  for(unsigned i=0;i<sizeof addresses/sizeof *addresses;++i)refuse_ram(addresses[i],1);
  refuse_ram(0x98,ram[0x98]);
  if(word(ram,0x48e)==0x46){refuse_ram(0x4e0,1);refuse_ram(0x540,0x80);
    refuse_ram(0x560,1);refuse_ram(0x600,0x80);refuse_ram(0x618,1);refuse_ram(0x61b,1);
  }else{refuse_ram(0x540,0x80);refuse_ram(0x560,1);refuse_ram(0x580,0x80);
    refuse_ram(0x598,1);refuse_ram(0x59b,1);}
  unsigned source_addresses[]={0x15470,0xc59f,0x8ce8,
    word(ram,0x48e)==0x46?0x29424:0x29433};
  for(unsigned i=0;i<sizeof source_addresses/sizeof *source_addresses;++i){
    rom[source_addresses[i]]^=1;observe();assert(!packet.valid);rom[source_addresses[i]]^=1;
  }
  uint16_t old=p.vram[0x6e80];p.vram[0x6e80]^=1;observe();assert(!packet.valid);p.vram[0x6e80]=old;
  old=p.vram[0x3f48];p.vram[0x3f48]=0x2dd9;observe();assert(!packet.valid);p.vram[0x3f48]=old;
  p.mosaic=1;observe();assert(!packet.valid);p.mosaic=0;
  p.screenWindowed[0]=1;observe();assert(!packet.valid);p.screenWindowed[0]=0;
  p.cgadsub=1;observe();assert(!packet.valid);p.cgadsub=0;
  p.renderFlags=0;observe();assert(!packet.valid);p.renderFlags=kPpuRenderFlags_NoSpriteLimits;
  LagoonBossHud_ObserveField(&p,ram,71,123,1);assert(!packet.valid);
  LagoonBossHud_ObserveField(&p,ram,0,123,0);assert(!packet.valid);
  observe();assert(packet.valid);
  unsigned widths[]={256,342,448,682,800};
  for(unsigned k=0;k<sizeof widths/sizeof *widths;++k){
    unsigned w=widths[k],stride=w*4+16,left=(w-256)/2;
    memset(wide_field,0xa5,sizeof wide_field);
    for(unsigned y=0;y<224;++y)memcpy(wide_field+y*stride+left*4,field+y*256,1024);
    memcpy(wide_out,wide_field,sizeof wide_field);
    assert(LagoonBossHud_Draw(wide_out,stride,wide_field,stride,w,224,71));
    for(unsigned y=0;y<224;++y)for(unsigned x=0;x<stride;++x)
      if(y<BAR_Y||y>=BAR_Y+8||x<(left+BAR_X)*4||x>=(left+BAR_X+128)*4)
        assert(wide_out[y*stride+x]==wide_field[y*stride+x]);
    memcpy(wide_out,wide_field,sizeof wide_field);
    assert(!LagoonBossHud_Draw(wide_out,stride,wide_field,stride,w,224,72));
    wide_field[BAR_Y*stride+(left+BAR_X)*4]^=1;
    assert(!LagoonBossHud_Draw(wide_out,stride,wide_field,stride,w,224,71));
    assert(wide_out[BAR_Y*stride+(left+BAR_X)*4]!=(wide_field[BAR_Y*stride+(left+BAR_X)*4]));
  }
  printf("PASS source/phase/graphics/generation/pixel refusal guards; five widths and row padding\n");
}
static void load(const char*prefix,const char*suffix,void*dst,size_t n){
  char path[4096];snprintf(path,sizeof path,"%s.%s",prefix,suffix);FILE*f=fopen(path,"rb");assert(f&&fread(dst,1,n,f)==n);fclose(f);
}
static void setup(const char*prefix){
  memset(&p,0,sizeof p);load(prefix,"wram.bin",ram,sizeof ram);load(prefix,"vram.bin",p.vram,sizeof p.vram);
  load(prefix,"cgram.bin",p.cgram,sizeof p.cgram);load(prefix,"oam.bin",p.oam,512);
  char path[4096];snprintf(path,sizeof path,"%s.oam.bin",prefix);FILE*f=fopen(path,"rb");assert(f);fseek(f,512,SEEK_SET);assert(fread(p.highOam,1,32,f)==32);fclose(f);
  load(prefix,"fb.bgrx",field,sizeof field);memcpy(out,field,sizeof out);
  p.inidisp=15;p.obsel=2;p.bgmode=9;p.bgTileAdr=0x600;p.bgXsc[0]=0x71;p.bgXsc[1]=0x79;p.bgXsc[2]=0x3c;
  /* Exact fixed boss arena scroll from the actual external fixture. */
  p.hScroll[0]=p.hScroll[1]=word(ram,0x480)&1023;p.vScroll[0]=p.vScroll[1]=word(ram,0x482)&1023;
  p.screenEnabled[0]=p.screenEnabled[1]=31;p.renderFlags=kPpuRenderFlags_NoSpriteLimits;
  LagoonBossHud_Init(rom,sizeof rom);
}
int main(int argc,char**argv){
  assert(argc>=3);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  setup(argv[2]);memcpy(before,ram,sizeof before);LagoonBossHud_ObserveField(&p,ram,71,123,0);
  if(argc==4&&!strcmp(argv[3],"--refuse")){
    assert(!packet.valid);assert(!LagoonBossHud_Draw((uint8_t*)out,1024,(uint8_t*)field,1024,256,224,71));
    assert(!memcmp(out,field,sizeof field)&&!memcmp(before,ram,sizeof before));
    puts("PASS other actual phase preserved without HUD changes");return 0;
  }
  if(!packet.valid){unsigned a,b,c;fprintf(stderr,"refusal source%d phase%d font%llx bar%04x menu%02x target%04x load%u\n",source_ok(),phase_health(ram,&a,&b,&c),(unsigned long long)hash((uint8_t*)(p.vram+0x6e80),144),p.vram[0x3f48],ram[0x275],word(ram,0x9a),ram[0x100e]);}
  assert(packet.valid);unsigned matched=0;
  for(unsigned y=0;y<8;++y)for(unsigned x=0;x<128;++x){unsigned a=(BAR_Y+y)*256+64+x;
    if(field[a]==packet.old_pixels[y*128+x])++matched;
    else if(matched<3)fprintf(stderr,"mismatch %u,%u old%08x field%08x\n",x,y,packet.old_pixels[y*128+x],field[a]);
  }
  printf("originalsource match %u/1024 HP%u/%u phase%u camera%u,%u\n",matched,packet.health,packet.maximum,packet.phase,word(ram,0x480),word(ram,0x482));
  assert(matched==1024);assert(LagoonBossHud_Draw((uint8_t*)out,1024,(uint8_t*)field,1024,256,224,71));
  assert(!memcmp(before,ram,sizeof before));unsigned changed=0;
  for(unsigned y=0;y<224;++y)for(unsigned x=0;x<256;++x){if(out[y*256+x]!=field[y*256+x]){assert(x>=64&&x<192&&y>=207&&y<215);++changed;}}
  if(argc==4){FILE*g=fopen(argv[3],"wb");assert(g);fprintf(g,"P6\n256 224\n255\n");for(unsigned i=0;i<256*224;++i){fputc(out[i]>>16,g);fputc(out[i]>>8,g);fputc(out[i],g);}fclose(g);}
  refusals();
  unsigned scaled=(packet.health*256+packet.maximum-1)/packet.maximum;
  for(unsigned j=0;j<16;++j)p.vram[0x3f48+j]=0x2dd0+(j<scaled/16?8:j==scaled/16?(scaled&15)/2:0);
  PpuBeginDrawing(&p,(uint8_t*)oracle,1024,kPpuRenderFlags_NewRenderer|kPpuRenderFlags_NoSpriteLimits);
  for(unsigned line=0;line<=224;++line)ppu_runLine(&p,line);
  for(unsigned y=0;y<8;++y)for(unsigned x=0;x<128;++x){unsigned i=(BAR_Y+y)*256+BAR_X+x;
    if(out[i]!=oracle[i])fprintf(stderr,"oracle %u,%u new%08x reference%08x\n",x,y,out[i],oracle[i]);
    assert(out[i]==oracle[i]);
  }
  printf("PASS source-only HUD overlay %u changedpixels, original RAM unchanged\n",changed);
}
