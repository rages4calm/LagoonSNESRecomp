#define LAGOON_WIDE_ACTOR_POLICY_NO_MAIN
#include "test_wide_additional_producer.c"
static int zoom=112;
int LagoonWide_ViewExtraY(void){return zoom;}
int LagoonWide_ViewExtraX(int extra){(void)extra;return 256;}
static uint8_t captured[0x20000],bird[64];
static void input(const char*pfx,const char*sfx,void*dst,size_t n){char path[4096];snprintf(path,sizeof path,"%s.%s",pfx,sfx);FILE*f=fopen(path,"rb");assert(f&&fread(dst,1,n,f)==n);fclose(f);}
static unsigned missing,emitted;
static void queued_setup(const char*prefix){memset(&p,0,sizeof p);input(prefix,"wram.bin",g_ram,sizeof g_ram);input(prefix,"vram.bin",p.vram,sizeof p.vram);input(prefix,"cgram.bin",p.cgram,sizeof p.cgram);memcpy(bird,g_ram+0x9c0,64);memset(g_ram+0x540,0,0x800);memcpy(g_ram+0x540,bird,64);actor_type=g_ram[0x558];p.obsel=2;p.inidisp=15;p.bgmode=9;p.bgXsc[0]=0x71;p.bgXsc[1]=g_ram[0x50]?0x7a:0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;g_config.widescreen=1;LagoonWideEntities_Reset();LagoonWideEntities_Init();atlas();PpuSetExtraSpace(&p,43);}
int main(int argc,char**argv){assert(argc==3);FILE*f=fopen(argv[1],"rb");assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);queued_setup(argv[2]);oam_for_record(-26,0,1);LagoonWideCell c[64];int n=decode_entity(g_ram,0x40,g_ram[0x55c],0,c);assert(n>0);
 for(int i=0;i<n;i++)if(c[i].y<-(int)c[i].size||c[i].y>=240){g_ram[0x2000+i*4]=64;g_ram[0x2200+i/4]|=1u<<((i%4)*2);missing++;}
 assert(missing);memcpy(p.oam,g_ram+0x2000,512);memcpy(p.highOam,g_ram+0x2200,32);capture();LagoonWideEntities_BeginFrame(1);
 for(unsigned i=0;i<p.extraObjectCount;i++){const PpuExtraObject*o=&p.extraObjects[i];if(o->y<-(int)o->height)emitted++;}
 printf("queued bird cells=%d original omitted=%u extended objects=%zu restored=%u\n",n,missing,p.extraObjectCount,emitted);assert(emitted);
 zoom=0;capture();LagoonWideEntities_BeginFrame(1);for(unsigned i=0;i<p.extraObjectCount;i++)assert(p.extraObjects[i].y>=-(int)p.extraObjects[i].height);
 zoom=112;unsigned unowned=0;for(int i=0;i<n;i++)if(c[i].y<-(int)c[i].size){unsigned base=c[i].tile&256?0x5000:0x4000;unowned=(base+(c[i].tile&255)*16)*2-0x8800;break;}
 assert(unowned<sizeof s_atlas.owned);s_atlas.owned[unowned]=0;capture();LagoonWideEntities_BeginFrame(1);for(unsigned i=0;i<p.extraObjectCount;i++)assert(p.extraObjects[i].y>=-(int)p.extraObjects[i].height);
 printf("PASS source-owned queued cells above old raster restored,1x omission and unowned-tile refusals preserved\n");return 0;}
