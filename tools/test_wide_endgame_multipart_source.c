/* Actual original018DC6 actor templates and originalC284/OAM, not invented
 * display records. Pose variants below are source policy tests; live actors
 * and actual resident graphics are independently checked by assisted routes. */
#define LAGOON_WIDE_MULTIPART_NO_MAIN
#include "test_wide_multipart_source.c"
static unsigned initializers,poses,mutations,packet_refusals;
static void initialize(Interp816*c,unsigned template,unsigned offset){
  c->pc=0x8dc6;c->k=c->db=1;c->dp=0;c->sp=0x1d0;
  c->x=offset-0x40;c->y=0;c->a=template;
  c->mf=1;c->xf=0;c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;
  ram[0x1d1]=0xef;ram[0x1d2]=0x7f;unsigned steps=0;
  while(c->k!=1||c->pc!=0x7ff0){assert(++steps<1000);interp816_runOpcode(c);}
  assert(c->sp==0x1d2);++initializers;
}
static unsigned make(Interp816*c,unsigned template,unsigned offset,unsigned frame,unsigned mode,int x){
  memset(ram,0,sizeof ram);initialize(c,template,offset);
  unsigned a=0x500+offset;assert(ram[a+24]==3&&ram[a+27]==(template==0?11:template==1?13:12));
  put(0x48e,0x47);put(0x480,0);put(0x482,32);put(0x48c,1);
  put(0xec0,offset);put(0xec2,96);ram[0xec4]=3;ram[0xec5]=255;
  ram[a]|=8;ram[a+28]=frame;ram[a+1]=(mode&1)?0x80:0;
  ram[a+31]=(mode&2)?2:0;ram[a+21]=mode*7;
  put(a+2,x);put(a+4,128);put(a+12,x);put(a+14,96);
  ram[0x2a2]=255;return a;
}
static void cmp_cells(const LagoonWideCell*c,int n){
  unsigned total=0;
  for(int i=0;i<n;++i)if(c[i].y>=-16&&c[i].y<240){
    unsigned a=0x2000+total*4,hi=(ram[0x2200+total/4]>>((total%4)*2))&3;
    assert((c[i].x&511)==(ram[a]|((hi&1)<<8)));
    assert((c[i].y&255)==ram[a+1]&&c[i].tile==word(ram,a+2));
    assert(c[i].size==((hi&2)?16:8));++total;++exact_cells;
  }assert(total==ram[0x236]);
}
static void draw(Interp816*c,unsigned offset){
  c->pc=0xc284;c->k=c->db=1;c->dp=0;c->sp=0x1d0;c->x=offset;c->y=c->a=0;
  c->mf=1;c->xf=0;c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;
  ram[0x1d1]=0xef;ram[0x1d2]=0x7f;unsigned steps=0;
  while(c->k!=1||c->pc!=0x7ff0){assert(++steps<100000);interp816_runOpcode(c);}
}
int main(int argc,char**argv){
  assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  Interp816*c=interp816_init(NULL,read_byte,write_byte);assert(c);
  LagoonWideCell out[256],sentinel[256];memset(sentinel,0xa5,sizeof sentinel);
  for(unsigned t=0;t<3;++t)for(unsigned slot=t==1?0x80:0x40;slot<=(t==1?0x240:0x40);slot+=64)
    for(unsigned frame=0;frame<255;++frame)if(LagoonWideEntities_CurrentSource(rom,sizeof rom,0x48,frame))
      for(unsigned mode=0;mode<4;++mode){
        make(c,t,slot,frame,mode,mode&1?252:-8);
        int n=LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,out,256);assert(n>=0);
        draw(c,slot);cmp_cells(out,n);++poses;
        if(n){memcpy(out,sentinel,sizeof out);assert(LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,out,n-1)<0);assert(!memcmp(out,sentinel,sizeof out));++packet_refusals;}
      }
  const unsigned spans[][2]={{0x18dc6,32},{0x18de6,192},{0x1b48d,205},{0x1b55a,25},{0x19382,114},{0x5c292,3}};
  make(c,0,0x40,28,0,252);
  for(unsigned j=0;j<sizeof spans/sizeof *spans;++j)for(unsigned i=0;i<spans[j][1];++i){
    unsigned a=((spans[j][0]>>16)&127)*32768+(spans[j][0]&32767)+i;rom[a]^=1;memcpy(out,sentinel,sizeof out);
    assert(LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,out,256)<0&&!memcmp(out,sentinel,sizeof out));rom[a]^=1;++mutations;
  }
  for(unsigned kind=11;kind<=13;++kind)for(unsigned offset=0x40;offset<=0x800;offset+=64){
    unsigned t=kind==11?0:kind==12?2:1;
    make(c,t,offset,kind==11?28:kind==12?1:18,0,252);
    int valid=kind==13?(offset>=0x80&&offset<=0x240):offset==0x40;
    if(!valid){memcpy(out,sentinel,sizeof out);assert(LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,out,256)<0&&!memcmp(out,sentinel,sizeof out));++packet_refusals;}
  }
  make(c,0,0x40,28,0,252);ram[0x48e]=0x46;
  assert(LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,out,256)<0);++packet_refusals;
  printf("endgame multipart source: %u original018DC6 initializer executions; %u source-policy poses/%u originalC284 OAM cells; %u source-byte and %u transactional/slot/room refusals PASS\n",initializers,poses,exact_cells,mutations,packet_refusals);
  interp816_free(c);return 0;
}
