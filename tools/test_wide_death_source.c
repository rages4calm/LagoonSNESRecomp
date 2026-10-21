/* Exact common0 resource, actual decompressor/queue/NMI/RTI and modeled real
 * hardware DMA. Original C214 death drawing consumes actual frames; no B413,
 * reward, AI or future animation service is executed. Own pinned ROM required. */
#define main previous_parallax_main
#define writeb previous_page_writeb
#include "test_wide_parallax_sources.c"
#undef writeb
#undef main
static LagoonWideDeathAtlas original_atlas;
static uint16_t resident[32768];
static uint8_t current_ram[0x20000];
static unsigned metadata_calls,oam_cells,owner_mutants,source_refusals,record_refusals;
static void hardware(void*ctx,uint32_t address,uint8_t value){(void)ctx;unsigned bank=address>>16,a=address&65535;
 if(bank==126||bank==127){ram[(bank-126)*65536+a]=value;return;}
 if((bank&127)<64&&a<8192){ram[a]=value;return;}
 assert((bank&127)<64&&a<32768);io[a]=value;
 if(a==0x4202||a==0x4203){unsigned result=io[0x4202]*io[0x4203];io[0x4216]=result;io[0x4217]=result>>8;}
 if(a==0x4206){unsigned result=value?word(io,0x4204)/value:65535;io[0x4214]=result;io[0x4215]=result>>8;}
 if(a==0x420b){assert(value==2&&io[0x4310]==1&&io[0x4311]==0x18);
  unsigned from=word(io,0x4312),bank=io[0x4314],to=word(io,0x2116),length=word(io,0x4315);
  assert(bank==127&&from==0x7f80&&length==512&&to==0x4300);
  assert(LagoonWideDeath_Begin(&original_atlas,31,rom,sizeof rom,bank,from,to,length));
  for(unsigned n=0;n<length;++n){uint8_t value=readb(NULL,(bank<<16)|((from+n)&65535));vram[to*2+n]=value;LagoonWideDeath_Write(&original_atlas,to*2+n,value);}
  ++jobs;
 }
}
static void put(unsigned a,unsigned v){ram[a]=v;ram[a+1]=v>>8;}
static void original_draw(Interp816*c){
 initcpu(c,1,0xc214,0x1d0,2);c->x=0x40;
 unsigned instructions=0;while(c->k!=1||c->pc!=0x7ff0){assert(++instructions<100000);interp816_runOpcode(c);}
 assert(c->sp==0x1d2);++metadata_calls;
}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);Interp816*c=interp816_init(NULL,readb,hardware);assert(c);
 initcpu(c,0,0x94a1,0x1d0,3);c->db=1;unsigned instructions=0;
 while(c->k!=0||c->pc!=0x7ff0){assert(++instructions<1000000);
  if(c->k==0&&c->pc==0x9143){source=ram[0xe0]|ram[0xe1]<<8|ram[0xe2]<<16;maximum=source;}
  if(c->k==0&&c->pc==0x89ac&&(ram[0x23a]&32))c->nmiWanted=true;
  interp816_runOpcode(c);
 }
 assert(c->sp==0x1d3&&jobs==1&&source==0x11be00&&maximum-source+1==168);
 assert(fnv(rom+off(source),168)==UINT64_C(0x14bc246e905bf71a));assert(fnv(vram+0x8600,512)==UINT64_C(0x84462e34625605fe));
 memcpy(resident,vram,sizeof resident);assert(LagoonWideDeath_Match(&original_atlas,resident,rom,sizeof rom,31));
 for(unsigned b=0;b<512;++b){
  ((uint8_t*)resident)[0x8600+b]^=1;assert(!LagoonWideDeath_Match(&original_atlas,resident,rom,sizeof rom,31));
  original_atlas.bytes[b]^=1;assert(!LagoonWideDeath_Match(&original_atlas,resident,rom,sizeof rom,31));
  original_atlas.bytes[b]^=1;((uint8_t*)resident)[0x8600+b]^=1;owner_mutants+=2;
  LagoonWideDeathAtlas unknown=original_atlas;LagoonWideDeath_Write(&unknown,0x8600+b,original_atlas.bytes[b]);assert(!LagoonWideDeath_Match(&unknown,resident,rom,sizeof rom,31));++owner_mutants;
 }
 const unsigned regions[][2]={{0x11be00,168},{0x94e6,7},{0x94a1,69},{0x18893,6},{0x9143,445},{0x908a,81},{0x884b,56},{0x89a7,13},{0x87b4,120}};
 for(unsigned j=0;j<9;++j)for(unsigned b=0;b<regions[j][1];++b){unsigned a=off(regions[j][0])+b;rom[a]^=1;assert(!LagoonWideDeath_Match(&original_atlas,resident,rom,sizeof rom,31));rom[a]^=1;++source_refusals;}
 const unsigned params[][4]={{126,0x7f80,0x4300,512},{127,0x7f81,0x4300,512},{127,0x7f80,0x4301,512},{127,0x7f80,0x4300,511},{127,0x7f80,0x4300,513}};
 for(unsigned j=0;j<5;++j){LagoonWideDeathAtlas a=original_atlas;assert(!LagoonWideDeath_Begin(&a,31,rom,sizeof rom,params[j][0],params[j][1],params[j][2],params[j][3]));assert(!LagoonWideDeath_Match(&a,resident,rom,sizeof rom,31));++owner_mutants;}
 assert(!LagoonWideDeath_Match(&original_atlas,resident,rom,sizeof rom,32));++owner_mutants;
 for(unsigned n=0;n<512;++n){LagoonWideDeathAtlas a;assert(LagoonWideDeath_Begin(&a,31,rom,sizeof rom,127,0x7f80,0x4300,512));for(unsigned b=0;b<n;++b)LagoonWideDeath_Write(&a,0x8600+b,original_atlas.bytes[b]);assert(!LagoonWideDeath_Match(&a,resident,rom,sizeof rom,31));++owner_mutants;LagoonWideDeath_Write(&a,0x8600+n+1,0);assert(!LagoonWideDeath_Match(&a,resident,rom,sizeof rom,31));++owner_mutants;}
 for(unsigned frame=4;frame<=11;++frame)for(unsigned height=0;height<256;++height)for(unsigned sample=0;sample<4;++sample){
  memset(ram,0,sizeof ram);memset(io,0,sizeof io);unsigned kind=sample&1?36:0,room=kind?0x44:3;
  put(0x48e,room);uint8_t*r=ram+0x540;r[0]=0xc0;r[1]=(uint8_t)(sample==0?1:sample==1?0x81:sample==2?3:255);r[24]=2;r[27]=kind;r[28]=frame;r[0x15]=height;r[0x1f]=sample*2;r[16]=sample*79;r[17]=sample*3;
  put(0x480,256);put(0x482,256);int x=sample&1?-140:100,y=sample==0?100:sample==1?-16:sample==2?239:288;
  put(0x542,256+x);put(0x544,256+y);put(0x54c,x);put(0x54e,y);put(0x90,sample==2?(unsigned)-7:sample*2);
  memcpy(current_ram,ram,sizeof ram);uint8_t actual;assert(LagoonWideEntities_CurrentFrame(rom,sizeof rom,ram,current_ram,0x40,&actual)&&actual==frame);
  LagoonWideCell cells[64];int n=LagoonWideEntities_RecordCells(rom,sizeof rom,ram,0x40,frame,1,cells,64);assert(n>0);
  for(int i=0;i<n;++i)assert(LagoonWideDeath_CellOwned(&original_atlas,0x4000,0x5000,&cells[i]));
  int visible=0;for(int i=0;i<n;++i)if(cells[i].y>=-16&&cells[i].y<240)cells[visible++]=cells[i];n=visible;
  ram[0x2a2]=255;original_draw(c);assert(n==ram[0x236]);
  for(int i=0;i<n;++i){unsigned a=0x2000+i*4,h=(ram[0x2200+i/4]>>((i%4)*2))&3;assert((unsigned)(cells[i].x&511)==(ram[a]|(h&1)<<8));assert((unsigned)(cells[i].y&255)==ram[a+1]);assert(cells[i].tile==word(ram,a+2)&&cells[i].size==((h&2)?16:8));++oam_cells;}
 }
 /* Adjacent frames/current mismatches cannot borrow common death authority. */
 memset(ram,0,sizeof ram);put(0x48e,3);ram[0x540]=0xc0;ram[0x541]=1;ram[0x558]=2;ram[0x55b]=0;put(0x542,100);put(0x544,100);put(0x54c,100);put(0x54e,100);
 for(unsigned frame=0;frame<256;++frame){ram[0x55c]=frame;memcpy(current_ram,ram,sizeof ram);uint8_t actual;int expected=frame>=4&&frame<=11;assert(!!LagoonWideEntities_CurrentFrame(rom,sizeof rom,ram,current_ram,0x40,&actual)==expected);if(!expected)++record_refusals;}
 ram[0x55c]=4;memcpy(current_ram,ram,sizeof ram);for(unsigned b=0;b<64;++b){current_ram[0x540+b]^=1;uint8_t actual;assert(!LagoonWideEntities_CurrentFrame(rom,sizeof rom,ram,current_ram,0x40,&actual));current_ram[0x540+b]^=1;++record_refusals;}
 for(unsigned b=0;b<43;++b){rom[off(0x1b413)+b]^=1;uint8_t actual;assert(!LagoonWideEntities_CurrentFrame(rom,sizeof rom,ram,current_ram,0x40,&actual));rom[off(0x1b413)+b]^=1;++source_refusals;}
 for(unsigned tile=0;tile<512;++tile){LagoonWideCell cell={0,0,(uint16_t)tile,8};int expected=tile>=48&&tile<=63;assert(!!LagoonWideDeath_CellOwned(&original_atlas,0x4000,0x5000,&cell)==expected);if(!expected)++owner_mutants;}
 printf("death source: original94A1/9143/908A/NMI/RTI one512-byte DMA from11BE00/168; %u original C214 calls/%u exact OAM cells for all8 deathframes/all256 heights; %u common-source/%u byte/order/generation/dynamic-pack/%u current-frame refusals PASS\n",metadata_calls,oam_cells,source_refusals,owner_mutants,record_refusals);interp816_free(c);return 0;
}
