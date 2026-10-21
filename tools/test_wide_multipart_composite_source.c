/* Full original C284/C326/C368 OAM oracle. These are finite source-policy
 * arrangements, not campaign/AI progression. No ROM graphics are copied. */
#define LAGOON_WIDE_MULTIPART_NO_MAIN
#include "test_wide_multipart_source.c"
static unsigned composite_cases,composite_cells,capacity_refusals,source_refusals,max_cells,above64;
static void composite_setup(unsigned kind,unsigned frame,unsigned flags,unsigned aux,
    unsigned height,int x,int y){
 setup(0x40+kind,kind,frame,0,x,y);ram[0x541]=flags;ram[0x555]=height;
 ram[0x55f]=(flags>>2)&2;ram[0x575]=aux;ram[0x576]=5;ram[0x577]=6;
 ram[0x57b]=0;ram[0x57c]=7;ram[0x57d]=(flags&2)?16:3;
}
int main(int argc,char **argv){
 assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
 Interp816 *cpu=interp816_init(NULL,read_byte,write_byte);assert(cpu);
 for(unsigned i=0;i<32768;++i)pixel_vram[i]=(uint16_t)(i*3907+191);
 for(unsigned i=0;i<256;++i)pixel_cgram[i]=(uint16_t)(i*103&32767);
 const unsigned heights[]={0,2,3,255},auxs[]={4,11,12,255};
 const int xs[]={-24,0,128,248,280,48},ys[]={-16,80,239,128,0,256};
 LagoonWideCell cells[256],sentinel[256];memset(sentinel,0xa5,sizeof sentinel);
 for(unsigned kind=0;kind<2;++kind)for(unsigned frame_index=0;frame_index<(kind?8:10);++frame_index){
  unsigned frame=frame_index==(kind?7:9)?255:frame_index;
  for(unsigned mode=0;mode<32;++mode){
   unsigned flags=(mode&1)|((mode&2)?4:0)|((mode&4)?0x20:0)|((mode&8)?0x80:0);
   for(unsigned h=0;h<4;++h)for(unsigned a=0;a<4;++a)for(unsigned pos=0;pos<6;++pos){
    composite_setup(kind,frame,flags,auxs[a],heights[h],xs[pos],ys[pos]);
    if(mode&16)ram[0x55f]|=2;
    memcpy(cells,sentinel,sizeof cells);int n=LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,cells,256);assert(n>=0);
    ram[0x2a2]=255;run(cpu,0xc284);
    unsigned visible=0;for(int i=0;i<n;++i)if(cells[i].y>=-16&&cells[i].y<240)++visible;
    if(visible!=ram[0x236])fprintf(stderr,"case kind%u frame%u flags%02x height%u aux%u pos%d,%d cells%d visible%u native%u\n",kind,frame,flags,heights[h],auxs[a],xs[pos],ys[pos],n,visible,ram[0x236]);
    compare(cells,n);++composite_cases;composite_cells+=n;if((unsigned)n>max_cells)max_cells=n;if(n>64)++above64;
    /* A bounded sample independently reconstructs all four bitplanes, flip
     * addressing and native-column clipping; no source pixels form expected. */
    if(!(mode&7)&&h==a)for(int i=0;i<n;++i)if(owned(&cells[i]))scalar(&cells[i]);
    if(n){memcpy(cells,sentinel,sizeof cells);assert(LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,cells,n-1)<0);assert(!memcmp(cells,sentinel,sizeof cells));++capacity_refusals;}
   }
  }
 }
 /* Globals in C326/C368 cannot lend authority to another actor slot. */
 for(unsigned kind=0;kind<2;++kind){composite_setup(kind,0,0,4,0,100,100);
  memcpy(ram+0x580,ram+0x540,64);put(0xec0,0x80);memcpy(cells,sentinel,sizeof cells);
  assert(LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,cells,256)<0);assert(!memcmp(cells,sentinel,sizeof cells));++source_refusals;
 }
 const unsigned spans[][2]={{0x05acb3,9},{0x05c10a,3},{0x008b4b,331},
   {0x01c326,66},{0x01fd9f,96}};
 composite_setup(0,0,4,4,0,100,100);
 for(unsigned j=0;j<sizeof spans/sizeof *spans;++j)for(unsigned i=0;i<spans[j][1];++i){
  unsigned o=(spans[j][0]>>16)*32768+(spans[j][0]&32767)+i;rom[o]^=1;
  memcpy(cells,sentinel,sizeof cells);assert(LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,cells,256)<0);
  assert(!memcmp(cells,sentinel,sizeof cells));rom[o]^=1;++source_refusals;
 }
 composite_setup(1,0,0,4,0,100,100);
 for(unsigned frame=7;frame<255;++frame){ram[0x55c]=frame;assert(LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,cells,256)<0);++source_refusals;}
 /* A component with a dynamic/unqualified common0 frame refuses every
  * component, while a real FF sentinel is independently proven above. */
 composite_setup(0,0,4,0,0,100,100);memcpy(cells,sentinel,sizeof cells);
 assert(LagoonWideMultipart_PacketCells(rom,sizeof rom,ram,0,cells,256)<0);assert(!memcmp(cells,sentinel,sizeof cells));++source_refusals;
 printf("composite multipart: %u full original C284/C326/C368 carriers, %u decoded cells/%u exact OAM cells (max%u, above64=%u); %u capacity +%u source/global-slot/unknown-component refusals; %u independent planar pixels/%u center-zero PASS. Authored kinds0/1 only; no AI/lifetime/resource ownership granted.\n",composite_cases,composite_cells,exact_cells,max_cells,above64,capacity_refusals,source_refusals,scalar_pixels,center_pixels);
 interp816_free(cpu);return 0;
}
