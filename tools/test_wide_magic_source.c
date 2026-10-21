#define main old_main
#define writeb old_writeb
#include "test_wide_parallax_sources.c"
#undef writeb
#undef main
#include "../src/lagoon_wide_entities.c"
static LagoonWideMagicAtlas loaded[16];
static uint16_t resident[16][32768];
static unsigned owner_cases,source_cases,metadata_cases,cell_cases,kind_cases;
static void hw(void*ctx,uint32_t a,uint8_t v){unsigned b=a>>16,w=a&65535;
 if(b==126||b==127){ram[(b-126)*65536+w]=v;return;}if((b&127)<64&&w<8192){ram[w]=v;return;}assert((b&127)<64&&w<32768);io[w]=v;
 if(w==0x4202||w==0x4203){unsigned r=io[0x4202]*io[0x4203];io[0x4216]=r;io[0x4217]=r>>8;}
 if(w==0x420b&&v==2&&scene!=3){unsigned from=word(io,0x4312),bank=io[0x4314],to=word(io,0x2116),length=word(io,0x4315);
  assert(bank==127&&from==0x7f80&&io[0x4310]==1&&io[0x4311]==0x18);
  assert(LagoonWideMagic_Begin(&loaded[scene],33,0x14,rom,sizeof rom,scene,bank,from,to,length));
  for(unsigned n=0;n<length;n++){uint8_t value=readb(NULL,(bank<<16)|((from+n)&65535));vram[to*2+n]=value;LagoonWideMagic_Write(&loaded[scene],to*2+n,value);}
  assert(loaded[scene].ready);memcpy(resident[scene],vram,sizeof vram);assert(LagoonWideMagic_Match(&loaded[scene],resident[scene],rom,sizeof rom,33,0x14,scene));++jobs;
 }
}
static void execute(Interp816*c,unsigned pc,unsigned x){initcpu(c,1,pc,0x1d0,2);c->x=x;unsigned n=0;while(c->k!=1||c->pc!=0x7ff0){assert(++n<100000);interp816_runOpcode(c);}assert(c->sp==0x1d2);}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);Interp816*c=interp816_init(NULL,readb,hw);
 for(scene=0;scene<16;scene++){memset(ram,0,sizeof ram);memset(io,0,sizeof io);ram[0x537]=scene;initcpu(c,1,0x8ea6,0x1d0,2);unsigned n=0;while(c->k!=1||c->pc!=0x7ff0){assert(++n<1000000);if(c->k==0&&c->pc==0x9143){source=ram[0xe0]|ram[0xe1]<<8|ram[0xe2]<<16;maximum=source;}if(c->k==0&&c->pc==0x89ac&&(ram[0x23a]&32))c->nmiWanted=true;interp816_runOpcode(c);}assert(c->sp==0x1d2);}assert(jobs==15);
 const unsigned selected[5]={0,4,8,12,14};
 for(unsigned j=0;j<5;++j){unsigned spell=selected[j];LagoonWideMagicAtlas *a=&loaded[spell];
  for(unsigned b=0;b<a->length;++b){((uint8_t*)resident[spell])[0xb800+b]^=1;assert(!LagoonWideMagic_Match(a,resident[spell],rom,sizeof rom,33,0x14,spell));((uint8_t*)resident[spell])[0xb800+b]^=1;++owner_cases;
   a->bytes[b]^=1;assert(!LagoonWideMagic_Match(a,resident[spell],rom,sizeof rom,33,0x14,spell));a->bytes[b]^=1;++owner_cases;
   LagoonWideMagicAtlas copy=*a;LagoonWideMagic_Write(&copy,0xb800+b,copy.bytes[b]);assert(!copy.ready);++owner_cases;}
  for(unsigned n=0;n<a->length;++n){LagoonWideMagicAtlas copy;assert(LagoonWideMagic_Begin(&copy,33,0x14,rom,sizeof rom,spell,127,0x7f80,0x5c00,a->length));for(unsigned b=0;b<n;++b)LagoonWideMagic_Write(&copy,0xb800+b,a->bytes[b]);assert(!copy.ready);LagoonWideMagic_Write(&copy,0xb801+n,0);assert(!copy.ready&&!copy.left);owner_cases+=2;}
  for(unsigned other=0;other<256;++other){int expected=magic_form(other)==magic_form(spell);assert(!!LagoonWideMagic_Match(a,resident[spell],rom,sizeof rom,33,0x14,other)==expected);if(!expected)++owner_cases;}
  assert(!LagoonWideMagic_Match(a,resident[spell],rom,sizeof rom,34,0x14,spell));assert(!LagoonWideMagic_Match(a,resident[spell],rom,sizeof rom,33,0x15,spell));owner_cases+=2;
 }
 const unsigned spans[][2]={{0x18ea6,130},{0x18f28,224},{0x188cd,20},{0x9143,445},{0x908a,81},{0x884b,56},{0x89a7,13},{0x87b4,120}};
 for(unsigned j=0;j<8;++j)for(unsigned b=0;b<spans[j][1];++b){unsigned at=off(spans[j][0])+b;rom[at]^=1;for(unsigned k=0;k<5;++k)assert(!LagoonWideMagic_Source(rom,sizeof rom,selected[k]));rom[at]^=1;++source_cases;}
 for(unsigned j=0;j<5;++j){const MagicSource*m=&magic_sources[j];for(unsigned b=0;b<m->compressed;++b){rom[off(m->source)+b]^=1;assert(!LagoonWideMagic_Source(rom,sizeof rom,selected[j]));rom[off(m->source)+b]^=1;++source_cases;}}
 scene=0;
 for(unsigned kind=1;kind<=8;++kind)for(unsigned clock=0;clock<256;++clock)for(unsigned direction=0;direction<8;++direction){memset(ram,0,sizeof ram);memset(io,0,sizeof io);unsigned offset=((kind+clock+direction)%16)*24,a=0xd40+offset;
  ram[a]=kind;ram[a+16]=direction;ram[a+17]=0;ram[a+18]=clock;ram[0x48e]=0x14;ram[0x48c]=1;ram[0xec0]=offset;ram[0xec1]=offset>>8;ram[0xec4]=4;
  ram[a+1]=24;ram[a+2]=2;ram[a+3]=228;ram[a+9]=24;ram[a+10]=1;ram[a+11]=100;ram[0x481]=1;ram[0x482]=128;ram[0xec2]=100;
  ram[0xec5]=clock;execute(c,0xbb4b,offset);++metadata_cases;LagoonWideCell cells[64];int count=LagoonWideEntities_MagicPacketCells(rom,sizeof rom,ram,0,cells,64);if(count<=0){fprintf(stderr,"badkind%u clock%u dir%u frame%u count%d\n",kind,clock,direction,ram[a+15],count);assert(0);}
  ram[0x2a2]=255;execute(c,0xc41b,offset);assert((unsigned)count==ram[0x236]);for(int i=0;i<count;++i){unsigned at=0x2000+i*4,high=(ram[0x2200+i/4]>>((i%4)*2))&3;
   assert((unsigned)(cells[i].x&511)==(ram[at]|((high&1)<<8)));assert((unsigned)(cells[i].y&255)==ram[at+1]);assert(cells[i].tile==word(ram,at+2)&&cells[i].size==((high&2)?16:8));++cell_cases;}
  for(unsigned j=0;j<5;++j){int owned=1;for(int i=0;i<count;++i)owned&=LagoonWideMagic_CellOwned(&loaded[selected[j]],0x4000,0x5000,&cells[i]);if(owned)++kind_cases;}
 }
 /* Actual mainloop order is BB05 before the ascending actor pass. Execute
  * the real projectile updater then original actor projection/queue append;
  * original C026 must preserve the equalY projectile-first carrier. */
 for(unsigned type=0;type<=2;++type){
  memset(ram,0,sizeof ram);memset(io,0,sizeof io);ram[0x481]=1;ram[0x482]=128;
  for(unsigned j=0;j<2;++j){unsigned a=0xd40+j*24;ram[a]=1;ram[a+1]=0x18+j*16;ram[a+2]=2;ram[a+3]=0xe4;}
  execute(c,0xbb05,0);assert(word(ram,0x48c)==2&&ram[0xec4]==4&&ram[0xeca]==4);
  ram[0x500]=0xc0;ram[0x518]=type;ram[0x502]=0x20;ram[0x503]=1;ram[0x504]=0xe4;execute(c,0xa5aa,0);execute(c,0xa624,0);
  assert(word(ram,0x48c)==3&&ram[0xed0]==type&&word(ram,0xec2)==word(ram,0xece));
  ram[0xec5]=0xff;ram[0xecb]=0x71;ram[0xed1]=0xfe;
  initcpu(c,1,0xc026,0x1d0,2);unsigned sortops=0;while(c->pc!=0xc08b){assert(++sortops<10000);interp816_runOpcode(c);}
  assert(ram[0xec4]==4&&word(ram,0xec0)==0&&ram[0xeca]==4&&word(ram,0xec6)==24&&ram[0xed0]==type);
 }
 printf("magic source:15 complete original equippedspell9143/NMI/RTI/DMA packs,BGspell3 excluded; %u original BB4B/C41B draws/%u exact OAM cells/%u wholepack groups; %u owner/order/byte/alias/generation and%u source refusals PASS\n",metadata_cases,cell_cases,kind_cases,owner_cases,source_cases);interp816_free(c);return 0;
}
