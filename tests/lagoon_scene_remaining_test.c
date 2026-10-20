/* Fresh remaining-room loader snapshots exercise the exact production renderer.
 * Background scalar oracle is independent; source resource matrix uses actual
 * original descriptors. This is neither full campaign nor synthetic live actors. */
#define LAGOON_SCENE_FAMILY_NO_MAIN
#include "lagoon_scene_families_test.c"
#undef main
static unsigned snapshots,contexts,native_ui_refusals;
static unsigned regvalue(const char*b,const char*key){char needle[80];snprintf(needle,sizeof(needle),"\"%s\"",key);char*p=strstr(b,needle);assert(p);unsigned value;assert(sscanf(strchr(p,':')+1,"%u",&value)==1);return value;}
static void actual_registers(const char*path){registers(path);char b[16384];FILE*f=fopen(path,"rb");assert(f);size_t n=fread(b,1,sizeof(b)-1,f);assert(feof(f));fclose(f);b[n]=0;
 p.inidisp=regvalue(b,"inidisp");p.bgmode=regvalue(b,"bgmode");p.obsel=regvalue(b,"obsel");p.mosaic=regvalue(b,"mosaic");p.bgTileAdr=regvalue(b,"bg_tile_adr");
 unsigned values[4];char*a=strstr(b,"\"bgsc\"");assert(a&&sscanf(strchr(a,'['),"[%u, %u, %u, %u]",values,values+1,values+2,values+3)==4);for(unsigned i=0;i<4;++i)p.bgXsc[i]=values[i];
 p.setini=regvalue(b,"setini");p.windowsel=regvalue(b,"windowsel");p.screenEnabled[0]=regvalue(b,"tm");p.screenWindowed[0]=regvalue(b,"tmw");p.screenWindowed[1]=regvalue(b,"tsw");p.cgwsel=regvalue(b,"cgwsel");p.cgadsub=regvalue(b,"cgadsub");
}
static void scalar_remaining(ScenePacket*s){
 const unsigned widths[]={256,342,682,800};
 for(unsigned w=0;w<4;++w)for(unsigned y=0;y<224;++y){unsigned width=widths[w];int left=(width-256)/2;
  uint8_t depth[800]={0};uint16_t color[800];for(unsigned x=0;x<width;++x)color[x]=s->pal[0];
  background_row(s,width,y,left,0,depth,color);background_row(s,width,y,left,1,depth,color);
  for(unsigned x=0;x<width;++x){unsigned ez=0,ec=s->pal[0];for(unsigned l=0;l<2;++l){
   int native=(int)x>=left&&(int)x<left+256,layered=s->parallax&&l==1;
   int qx=(layered?(int)s->bg2_x:native?(int)s->native_x:s->wx)+(int)x-left;
   int qy=(layered?(int)s->bg2_y:native?(int)s->native_y:s->wy)+(int)y;unsigned tile;
   if(layered)tile=s->vram[0x7800+(((unsigned)qy>>8)&1)*0x400+(((unsigned)qy>>3)&31)*32+(((unsigned)qx>>3)&31)];
   else if(native)tile=s->vram[(0x7000+l*0x800+((qy/8)&31)*32+((qx/8)&31)+((qx&256)?0x400:0))&32767];
   else{if(qx<0||qy<0||qx>=s->world_width||qy>=s->world_height)continue;unsigned meta=s->maps[l][(qy/16)*128+qx/16];tile=s->defs[l][meta*4+((qx&8)?1:0)+((qy&8)?2:0)];if(l)tile&=0xdfff;}
   unsigned c=scalar_graphics(s,tile,qx&7,qy&7,0,4),z=(tile&0x2000?12:8)-l;
   if(c&&z>ez){ez=z;ec=s->pal[((tile>>10)&7)*16+c];}
  }assert(depth[x]==ez&&color[x]==ec);++pixel_checks;}
 }
}
static void descriptor_matrix(void){
 static const unsigned fields[]={0x48e,0x48f,0x50,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5d,0x5e,0x5f,0x69,0x6a,0x6b,0x75,0x76,0x77,0x88,0x89,0x8a,0x8b,0x8c,0x8d};
 for(unsigned room=0;room<75;++room){memset(ram,0,sizeof(ram));unsigned at=0x2b2df+room*56;
  memcpy(ram+0x50,rom+at,56);pointer(0x88,0x7f0000);pointer(0x8b,0x7f4000);ram[0x48e]=room;
  assert(LagoonSceneRenderer_ExactRoom(ram));++contexts;
  for(unsigned i=0;i<sizeof(fields)/sizeof(fields[0]);++i){unsigned mask=fields[i]==0x48e?128:1;ram[fields[i]]^=mask;if(LagoonSceneRenderer_ExactRoom(ram)){fprintf(stderr,"descriptor room%02x field%04x remains accepted\n",room,fields[i]);assert(0);}ram[fields[i]]^=mask;++resource_refusals;}
  if(ram[0x50])for(unsigned i=0x7e;i<=0x83;++i){ram[i]^=1;assert(!LagoonSceneRenderer_ExactRoom(ram));ram[i]^=1;++resource_refusals;}
 }
}
int main(int argc,char**argv){
 int isolated=argc==4&&!strcmp(argv[3],"--isolated");assert(argc==3||isolated);const unsigned rooms[]={1,2,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,38,39,40,41,43,56,57,58,59,60,61,62,64,65,66,67,68,69,70,71,72,73,74};
 read_owned(argv[1],rom,sizeof(rom));descriptor_matrix();
 for(unsigned n=0;n<(isolated?2:sizeof(rooms)/sizeof(rooms[0]));++n){unsigned room=isolated?(n?0x3f:0x2a):rooms[n];setup();read_owned(argv[1],rom,sizeof(rom));char path[2048];
  if(isolated)snprintf(path,sizeof(path),"%s/isolated-%02x-off/dumps/sample-11.wram.bin",argv[2],room);else file_path(path,sizeof(path),argv[2],"baseline-",room,"sample-11","wram.bin");read_owned(path,ram,sizeof(ram));
  if(isolated)snprintf(path,sizeof(path),"%s/isolated-%02x-off/dumps/sample-11.vram.bin",argv[2],room);else file_path(path,sizeof(path),argv[2],"baseline-",room,"sample-11","vram.bin");read_owned(path,p.vram,sizeof(p.vram));
  if(isolated)snprintf(path,sizeof(path),"%s/isolated-%02x-off/dumps/sample-11.cgram.bin",argv[2],room);else file_path(path,sizeof(path),argv[2],"baseline-",room,"sample-11","cgram.bin");read_owned(path,p.cgram,sizeof(p.cgram));
  if(isolated)snprintf(path,sizeof(path),"%s/isolated-%02x-off/dumps/sample-11.regs.json",argv[2],room);else file_path(path,sizeof(path),argv[2],"baseline-",room,"sample-11","regs.json");actual_registers(path);
  assert(word(ram,0x48e)==room&&LagoonSceneRenderer_ExactRoom(ram));
  LagoonSceneRenderer_Reset();s_mode=2;capture(29);ScenePacket*s=&s_packets[s_complete];if(!s->valid){assert(p.bgmode!=9||p.inidisp!=15||p.windowsel||p.cgadsub);++native_ui_refusals;continue;}assert(s->finite_clip||s->parallax);
  scalar_remaining(s);finite_edges(s);++snapshots;
  Ppu saved=p;for(unsigned bad=0;bad<12;++bad){int authority=1;
   if(bad==0)p.inidisp=14;if(bad==1)p.bgXsc[1]^=3;if(bad==2)p.hScroll[1]++;
   if(bad==3)p.cgadsub=1;if(bad==4)p.mosaic=1;if(bad==5)p.screenWindowed[0]=1;
   if(bad==6)p.extraObjectCount=513;if(bad==7)authority=0;if(bad==8)ram[0x100e]=1;
   if(bad==9)ram[0x48d]=1;if(bad==10)p.windowsel=1;if(bad==11)p.obsel=3;
   assert(!supported(&p,ram,rom,sizeof(rom),authority));p=saved;ram[0x100e]=ram[0x48d]=0;++resource_refusals;
  }
  assert(!LagoonSceneRenderer_TryPresent(output,682*4,expected,682*4,682,224,30));
 }
 printf("remaining scenes: %u original descriptor contexts, %u actual fresh stable loader snapshots/%u actual native-UI refusals, %lu independent scalar source pixels/%lu finite-edge pixels, %lu descriptor/raster/authority refusals PASS\n",contexts,snapshots,native_ui_refusals,pixel_checks,edge_checks,resource_refusals);return 0;
}
