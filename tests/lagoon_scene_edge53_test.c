/* Synthetic source packets, actual SDK shadow lookup and PPU scanout. These
 * vectors qualify the bounded fill/cache policy, never a new runtime room. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "snes/ppu.h"
#include "snes/snes.h"
#include "snes/ws_shadow.h"
Snes *g_snes;
int snes_frame_counter;
unsigned char g_snesrecomp_last_hdmaen;
#define LAGOON_WIDE_ENTITIES_DECODER_ONLY
#include "../src/lagoon_wide_entities.c"
#include "../src/lagoon_scene_renderer.c"
static Ppu p;
static uint8_t ram[0x20000],rom[0x100000];
static uint32_t reference[800*224],output[800*224],old_pixels[800*224];
static unsigned fields,words,old_differences,refusals,cache_cases;
static void setword(uint8_t *r,unsigned a,unsigned v){r[a]=v;r[a+1]=v>>8;}
static void pointer(unsigned a,unsigned v){setword(ram,a,v);ram[a+2]=v>>16;}
static unsigned source_tile(unsigned layer,int x,int y){
    /* The real title always passes(-16,-16) to SetBlankTile. Its source
     * metatile quadrant is0 even when the eventual negative pixel is not. */
    unsigned meta=(x<0||y<0||x>=2048||y>=2048)?ram[0x52]:ram[0x10000+layer*0x4000+(y>>4)*128+(x>>4)];
    unsigned a=0x30000+meta*8+((x&8)>>2)+((y&8)>>1);
    unsigned tile=rom[a]|rom[a+1]<<8;return layer?tile&0xdfff:tile;
}
static void setup(unsigned wx,unsigned wy,unsigned extra,unsigned fill){
    memset(&p,0,sizeof p);ppu_reset(&p);memset(ram,0,sizeof ram);memset(rom,0,sizeof rom);
    pointer(0x57,0x138000);pointer(0x5d,0x198000);pointer(0x69,0x068000);pointer(0x75,0x068000);
    pointer(0x88,0x7f0000);pointer(0x8b,0x7f4000);setword(ram,0x53,192);setword(ram,0x55,192);
    setword(ram,0x480,wx);setword(ram,0x482,wy-1);ram[0x52]=fill;
    p.inidisp=15;p.bgmode=9;p.obsel=2;p.bgXsc[0]=0x71;p.bgXsc[1]=0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;p.screenEnabled[0]=p.screenEnabled[1]=31;
    p.hScroll[0]=p.hScroll[1]=wx&1023;p.vScroll[0]=p.vScroll[1]=(wy-1)&1023;
    for(unsigned n=0;n<128;++n)((uint8_t*)p.oam)[n*4+1]=240;
    for(unsigned n=0;n<256;++n)p.cgram[n]=(n*1171+0x31)&32767;
    for(unsigned tile=0;tile<64;++tile)for(unsigned y=0;y<8;++y){
        p.vram[tile*16+y]=0x00ffu|(((tile+y)&1)?0x5500:0xaa00);
        p.vram[tile*16+y+8]=(uint16_t)((tile*0x311+y*0x1357)&0xffff);
    }
    for(unsigned n=0;n<256;++n)for(unsigned q=0;q<4;++q){
        unsigned tile=1+(n*7+q*11)%63;
        tile|=((n+q)&7)<<10;tile|=((n^q)&1)?0x2000:0;tile|=(q&1)?0x4000:0;tile|=(q&2)?0x8000:0;
        setword(rom,0x30000+n*8+q*2,tile);
    }
    for(unsigned l=0;l<2;++l)for(unsigned n=0;n<0x4000;++n)ram[0x10000+l*0x4000+n]=(n*17+l*29)&255;
    /* Arbitrary native circular history differs from world map on purpose. */
    for(unsigned n=0;n<0x800;++n){p.vram[0x7000+n]=(n*31)&63;p.vram[0x7800+n]=((n*13)&63)|0x2400;}
    PpuBeginDrawing(&p,(uint8_t*)reference,(256+2*extra)*4,kPpuRenderFlags_NewRenderer);
    PpuSetExtraSpace(&p,extra);PpuSetWidescreenShadowNativeExactMask(&p,3);
    WsShadowReset();
    for(unsigned l=0;l<2;++l){
        WsShadowSetWorld(l,wx,wy-1);WsShadowSetScroll(l,p.hScroll[l],p.vScroll[l]);
        WsShadowSetBlankTile(l,source_tile(l,-16,-16));
    }
    WsShadowFrame(&p);
    /* Seed only actual nonnegative world cells from authored definitions. */
    int minx=(int)wx-(int)extra-8,maxx=(int)wx+256+(int)extra+8;
    for(unsigned l=0;l<2;++l)for(unsigned y=(wy-1)/8;y<=(wy+224)/8;++y)
        for(int x=minx/8;x<=maxx/8;++x)if(x>=0)
            WsShadowForceTile(l,(unsigned)x,y,(uint16_t)source_tile(l,x*8,(int)y*8));
    LagoonSceneRenderer_Reset();s_mode=2;
}
static void capture(void){
    for(unsigned y=1;y<=224;++y)LagoonSceneRenderer_ObserveRaster(&p,y,ram,rom,sizeof rom,73,LAGOON_SCENE_WORLD);
    LagoonSceneRenderer_EndFrame(100,73,LAGOON_SCENE_WORLD);
    assert(s_packets[s_complete].valid&&s_packets[s_complete].map_fill);
}
static void field(unsigned wx,unsigned wy,unsigned extra,unsigned fill){
    setup(wx,wy,extra,fill);unsigned width=256+2*extra;capture();ScenePacket *s=&s_packets[s_complete];
    assert(s->fill_meta==fill&&s->wx==(int)wx&&s->wy==(int)wy);
    ppu_runLine(&p,0);for(unsigned y=1;y<=224;++y)ppu_runLine(&p,y);
    render(s,width);unsigned differences=0;
    for(unsigned n=0;n<width*224;++n)if(s_pixels[n]!=reference[n]){
        if(!differences)fprintf(stderr,"first PPU mismatch wx%u wy%u width%u fill%u at%u,%u candidate%08X ref%08X\n",wx,wy,width,fill,n%width,n/width,s_pixels[n],reference[n]);++differences;
    }
    assert(!differences);words+=width*224;fields++;
    /* Former black-margin behavior must fail the unchanged whole-field gate. */
    s->map_fill=0;s->serial=++s_serial;s_background.valid=0;render(s,width);memcpy(old_pixels,s_pixels,width*224*4);
    unsigned old_bad=0;for(unsigned n=0;n<width*224;++n)old_bad+=old_pixels[n]!=reference[n];
    if(wx<extra){assert(old_bad);old_differences+=old_bad;}
    s->map_fill=1;s->serial=++s_serial;s_background.valid=0;
    memset(output,0x59,sizeof output);
    assert(LagoonSceneRenderer_TryPresent((uint8_t*)output,width*4,(uint8_t*)reference,width*4,width,224,73));
    assert(!memcmp(output,reference,width*224*4));
    /* Cache reuse must retain fill meta and enabling policy as complete keys. */
    render(s,width);render(s,width);assert(same_background(s,width));
    s->map_fill=0;assert(!same_background(s,width));s->map_fill=1;
    s->fill_meta^=1;assert(!same_background(s,width));s->fill_meta^=1;
    assert(same_background(s,width));cache_cases+=2;
    ScenePacket *other=&s_packets[s_build];*other=*s;assert(same_source(s,other));
    other->map_fill^=1;assert(!same_source(s,other));other->map_fill^=1;
    other->fill_meta^=1;assert(!same_source(s,other));cache_cases+=2;
    reference[extra]^=1;memset(output,0x59,sizeof output);
    assert(!LagoonSceneRenderer_TryPresent((uint8_t*)output,width*4,(uint8_t*)reference,width*4,width,224,73));
    for(unsigned n=0;n<width*224;++n)assert(output[n]==0x59595959);++refusals;
}
static void policy(void){
    for(unsigned bad=0;bad<10;++bad){
        setup(269,673,272,37);
        switch(bad){
        case 0:ram[0x48f]=1;break;case 1:ram[0x57]^=1;break;case 2:ram[0x69]^=1;break;
        case 3:setword(ram,0x53,128);setword(ram,0x55,128);break;
        case 4:p.inidisp=14;break;case 5:p.mosaic=1;break;
        case 6:ram[0x100e]=1;break;case 7:p.bgmode=1;break;
        case 8:p.bgXsc[0]=0x70;break;case 9:p.windowsel=1;break;
        }
        LagoonSceneRenderer_ObserveRaster(&p,1,ram,rom,sizeof rom,73,LAGOON_SCENE_WORLD);
        ScenePacket *s=&s_packets[s_build];
        if(bad==3)assert(s->valid&&!s->map_fill);else assert(!s->valid);++refusals;
    }
    setup(269,673,272,37);p.mosaic=15;p.lastMosaicModulo=1;
    for(unsigned y=0;y<=224;++y)p.mosaicModulo[y]=y;
    LagoonSceneRenderer_ObserveRaster(&p,1,ram,rom,sizeof rom,73,LAGOON_SCENE_INCOMING_MOSAIC);
    assert(s_packets[s_build].valid&&!s_packets[s_build].map_fill);++refusals;
    /* An independently valid ordinary cave packet keeps its former policy. */
    setup(269,673,272,37);ram[0x48e]=3;setword(ram,0x53,224);setword(ram,0x55,204);
    pointer(0x57,0x128b77);pointer(0x5d,0x1d8000);pointer(0x69,0x069298);pointer(0x75,0x069298);
    LagoonSceneRenderer_ObserveRaster(&p,1,ram,rom,sizeof rom,73,LAGOON_SCENE_WORLD);
    assert(s_packets[s_build].valid&&!s_packets[s_build].map_fill);++refusals;
}
int main(void){
    const unsigned extras[]={43,96,213,272};
    for(unsigned i=0;i<4;++i)for(unsigned phase=0;phase<8;++phase)for(unsigned fill=0;fill<2;++fill)
        field(extras[i]-8+phase,673+phase,extras[i],fill?255:37);
    field(269,672,272,37);field(264,672,272,37);policy();
    printf("PASS edge53: %u actual fast-PPU/shadow whole fields, %u exact words, %u former black-fill mismatched words, %u cache/source-key controls, %u preserved reference/packet refusals; widths342/448/682/800, all fine phases and distinct fill quadrants; native circular history preserved\n",fields,words,old_differences,cache_cases,refusals);return 0;
}
