/* External actual scene; original native PPU rerenders each original all-BG
 * mosaic size. No image/ROM/state bytes are embedded and no live timing claim. */
#define main LagoonPpuFixtureMain
#include "../snesrecomp/tests/ppu/ppu_extra_object_test.c"
#undef main
#define LAGOON_WIDE_ENTITIES_DECODER_ONLY
#include "../src/lagoon_wide_entities.c"
#include "../src/lagoon_scene_renderer.c"
static uint8_t ram[0x20000],rom[0x100000];
static Ppu base;
static uint32_t original[256*224],captured[256*224],wide[800*224];
static unsigned stages,native_words,margin_words,refusals;
static void read_exact(const char *prefix,const char *suffix,void *dst,size_t size) {
 char path[4096];snprintf(path,sizeof path,"%s.%s",prefix,suffix);FILE*f=fopen(path,"rb");assert(f&&fread(dst,1,size,f)==size&&fgetc(f)==EOF);fclose(f);
}
static unsigned number(const char *json,const char *name){char token[128];snprintf(token,sizeof token,"\"%s\"",name);const char*a=strstr(json,token);assert(a);return strtoul(strchr(a,':')+1,NULL,10);}
static void load(const char *rom_path,const char *prefix){
 FILE*f=fopen(rom_path,"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom&&fgetc(f)==EOF);fclose(f);
 memset(&p,0,sizeof p);ppu_reset(&p);
 read_exact(prefix,"wram.bin",ram,sizeof ram);read_exact(prefix,"vram.bin",p.vram,sizeof p.vram);read_exact(prefix,"cgram.bin",p.cgram,sizeof p.cgram);
 uint8_t oam[544];read_exact(prefix,"oam.bin",oam,sizeof oam);memcpy(p.oam,oam,512);memcpy(p.highOam,oam+512,32);
 read_exact(prefix,"fb.bgrx",captured,sizeof captured);
 char path[4096],json[16384];snprintf(path,sizeof path,"%s.regs.json",prefix);f=fopen(path,"rb");assert(f);size_t n=fread(json,1,sizeof(json)-1,f);assert(feof(f));json[n]=0;fclose(f);
 p.inidisp=number(json,"inidisp");p.obsel=number(json,"obsel");p.bgmode=number(json,"bgmode");p.mosaic=number(json,"mosaic");p.bgTileAdr=number(json,"bg_tile_adr");
 p.cgwsel=number(json,"cgwsel");p.cgadsub=number(json,"cgadsub");p.screenEnabled[0]=number(json,"tm");p.screenEnabled[1]=number(json,"ts");
 unsigned a[4];const char*b=strchr(strstr(json,"\"bgsc\""),'[');assert(sscanf(b,"[%u, %u, %u, %u]",a,a+1,a+2,a+3)==4);for(unsigned i=0;i<4;i++)p.bgXsc[i]=a[i];
 b=strchr(strstr(json,"\"hscroll\""),'[');assert(sscanf(b,"[%u, %u, %u, %u]",a,a+1,a+2,a+3)==4);for(unsigned i=0;i<4;i++)p.hScroll[i]=a[i];
 b=strchr(strstr(json,"\"vscroll\""),'[');assert(sscanf(b,"[%u, %u, %u, %u]",a,a+1,a+2,a+3)==4);for(unsigned i=0;i<4;i++)p.vScroll[i]=a[i];
 assert(p.mosaic==255&&p.inidisp==1);base=p;
}
static unsigned beam_only;
static void original_field(unsigned fast,unsigned before_size){
 unsigned actual=p.mosaic;p.mosaic=((before_size-1)<<4)|15;
 PpuBeginDrawing(&p,(uint8_t*)original,256*4,fast?kPpuRenderFlags_NewRenderer:0);
 if(!beam_only)ppu_runLine(&p,0);
 if(before_size!=(actual>>4)+1)ppu_write(&p,6,(uint8_t)actual); /* Actual late-NMI register side effects. */
 else p.mosaic=actual;
 for(unsigned line=1;line<=224;line++)ppu_runLine(&p,line);
}
static void capture_mosaic(unsigned generation){
 /* Original native-width PPU is only the independent reference. The copied
  * scene packet uses the real widened strict OAM policy, with no extra actors. */
 p.extraLeftCur=p.extraRightCur=213;p.wsOamLeftHintStrict=p.wsOamRightHintStrict=1;
 for(unsigned line=1;line<=224;line++)LagoonSceneRenderer_ObserveRaster(&p,line,ram,rom,sizeof rom,generation,LAGOON_SCENE_INCOMING_MOSAIC);
 LagoonSceneRenderer_EndFrame(100,generation,LAGOON_SCENE_INCOMING_MOSAIC);
}
static unsigned planar(unsigned tile,unsigned x,unsigned y,unsigned base_adr){
 if(tile&0x4000)x=7-x;if(tile&0x8000)y=7-y;
 unsigned address=(base_adr+(tile&1023)*16+y)&32767,bits=p.vram[address],more=p.vram[(address+8)&32767],bit=7-x;
 return ((bits>>bit)&1)|((bits>>(bit+8)&1)<<1)|((more>>bit&1)<<2)|((more>>(bit+8)&1)<<3);
}
static uint32_t scalar_margin(unsigned x,unsigned y,unsigned width){
 int left=(int)(width-256)/2,n=p.mosaic/16+1,sx=(int)x-left,rem=sx%n;if(rem<0)rem+=n;
 int phase=(p.renderFlags&kPpuRenderFlags_NewRenderer)?0:p.mosaicStartLine;
 int vertical=(p.renderFlags&kPpuRenderFlags_NewRenderer)&&n>1?p.lastMosaicModulo:n;
 int mx=sx-rem,my=((int)y+1-phase)/vertical*vertical+phase;
 int world_x=(int)word(ram,0x480)+(int)x-left,world_y=(int)word(ram,0x482)+(int)y+1;
 if(world_x<0||world_y<0||world_x>=(int)word(ram,0x53)*8||world_y>=(int)word(ram,0x55)*8)return 0;
 unsigned color=p.cgram[0],depth=0;
 for(unsigned layer=0;layer<2;layer++){
  int qx=(int)p.hScroll[layer]+mx,qy=(int)p.vScroll[layer]+my;unsigned tile;
  if(layer==1){tile=p.vram[0x7800+((qy>>8)&1)*1024+((qy>>3)&31)*32+((qx>>3)&31)];}
  else{
   if(qx<0||qy<0||qx>=(int)word(ram,0x53)*8||qy>=(int)word(ram,0x55)*8)continue;
   unsigned meta=ram[0x10000+(qy>>4)*128+(qx>>4)],adr=(ptr(ram,0x69)>>16)*32768+(ptr(ram,0x69)&32767)+meta*8+((qx&8)>>2)+((qy&8)>>1);
   tile=rom[adr]|rom[adr+1]<<8;
  }
  unsigned c=planar(tile,qx&7,qy&7,0),z=((tile&0x2000)?12:8)-layer;
  if(c&&z>depth){depth=z;color=p.cgram[((tile>>10)&7)*16+c];}
 }
 unsigned r=color&31,g=(color>>5)&31,b=(color>>10)&31,level=p.inidisp;
 return ((((r<<3)|(r>>2))*level/15)<<16)|((((g<<3)|(g>>2))*level/15)<<8)|(((b<<3)|(b>>2))*level/15);
}
static void negative_controls(void){
 unsigned source_offset=0x13*32768+(0xfd84&32767);
 for(unsigned bad=0;bad<16;bad++){
  p=base;original_field(1,16);uint8_t room=ram[0x48e],bg=ram[0x57],source=rom[source_offset];
  switch(bad){
   case 0:ram[0x48e]^=128;break;case 1:ram[0x57]^=1;break;
   case 2:p.vram[0x7800]^=1;break;case 3:rom[source_offset]^=1;break;
   case 4:p.mosaic=254;break;case 5:p.mosaic=0;break;case 6:p.inidisp=128;break;
   case 7:p.bgmode=1;break;case 8:p.lastMosaicModulo=0;break;
   case 9:p.mosaicModulo[17]^=1;break;case 10:p.hScroll[1]++;break;
   case 11:p.extraObjectCount=1;break;case 12:p.oamaddh=128;break;
   case 13:p.windowsel=1;break;case 14:p.cgadsub=1;break;case 15:p.bgXsc[1]=0x79;break;
  }
  capture_mosaic(71);assert(!s_packets[s_complete].valid);refusals++;
  ram[0x48e]=room;ram[0x57]=bg;rom[source_offset]=source;
 }
 for(unsigned bad=0;bad<7;bad++){
  p=base;original_field(1,16);p.extraLeftCur=p.extraRightCur=213;p.wsOamLeftHintStrict=p.wsOamRightHintStrict=1;
  for(unsigned line=1;line<=224;line++){
   if(line==100)switch(bad){case 0:p.cgram[0]^=1;break;case 1:p.oam[0]^=1;break;
    case 2:p.vramWriteCount++;break;case 3:p.mosaicModulo[100]^=1;break;
    case 4:p.renderFlags^=kPpuRenderFlags_NewRenderer;break;case 5:p.mosaicStartLine++;break;case 6:p.inidisp++;break;}
   int authority=bad==5&&line>=100?LAGOON_SCENE_WORLD:LAGOON_SCENE_INCOMING_MOSAIC;
   LagoonSceneRenderer_ObserveRaster(&p,line,ram,rom,sizeof rom,71,authority);
  }
  LagoonSceneRenderer_EndFrame(100,71,LAGOON_SCENE_INCOMING_MOSAIC);assert(!s_packets[s_complete].valid);refusals++;
 }
}
int main(int argc,char **argv){
 assert(argc==3);load(argv[1],argv[2]);LagoonSceneRenderer_Reset();s_mode=2;
 unsigned captured_matches=0,minimum_captured=256*224,best_fast=0,best_late=0;
 for(beam_only=0;beam_only<2;beam_only++)for(unsigned fast=0;fast<2;fast++)for(unsigned late=0;late<2;late++)for(unsigned size=1;size<=16;size++){
  p=base;p.mosaic=((size-1)<<4)|15;p.inidisp=size==1?15:17-size;
  original_field(fast,late?(size==16?1:size+1):size);
  if(size==16){unsigned differences=0;for(unsigned i=0;i<256*224;i++)differences+=original[i]!=captured[i];
   if(!differences)captured_matches++;if(differences<minimum_captured){minimum_captured=differences;best_fast=fast;best_late=late;}}
  capture_mosaic(71);assert(s_packets[s_complete].valid);
  unsigned width=682,left=(width-256)/2;memset(wide,0,sizeof wide);
  for(unsigned y=0;y<224;y++)memcpy(wide+y*width+left,original+y*256,256*4);
  if(!LagoonSceneRenderer_TryPresent((uint8_t*)wide,width*4,(uint8_t*)wide,width*4,width,224,71)){
   SceneMismatch *m=&s_mismatch[s_mismatch_count-1];fprintf(stderr,"fast%u late%u size%u native refusal mismatches%llu first%u,%u candidate%08x native%08x\n",fast,late,size,(unsigned long long)s_stats.mismatches,m->x,m->y,m->candidate,m->reference);return 2;
  }
  for(unsigned y=0;y<224;y++){
   assert(!memcmp(wide+y*width+left,original+y*256,256*4));native_words+=256;
   for(unsigned x=0;x<width;x++)if(x<left||x>=left+256){assert(wide[y*width+x]==scalar_margin(x,y,width));margin_words++;}
  }
  uint32_t saved=wide[left];wide[left]^=0x01000000;uint32_t before=wide[0];
  assert(!LagoonSceneRenderer_TryPresent((uint8_t*)wide,width*4,(uint8_t*)wide,width*4,width,224,71));assert(wide[0]==before);wide[left]=saved;refusals++;
  assert(!LagoonSceneRenderer_TryPresent((uint8_t*)wide,width*4,(uint8_t*)wide,width*4,width,224,72));refusals++;
  stages++;
 }
 beam_only=1;negative_controls();
 printf("PASS incoming mosaic %u original-PPU size/backend/cache/frame-start pairs (including beam1..224 without0), %u native words unchanged, %u independent scalar margin words, %u refusal controls. Old captured-after-field FF matches%u (closest%uwords fast%u late%u); corrected live immutable packet/native gate is separate proof.\n",stages,native_words,margin_words,refusals,captured_matches,minimum_captured,best_fast,best_late);return 0;
}
