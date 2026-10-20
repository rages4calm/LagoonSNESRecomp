/* Original real-opcode producer proof. No extracted code/art is embedded.
 * Every current-pose frame admitted by production is independently drawn by
 * original 008B71->8211. AI/animation dispatch is never simulated here. */
#define main previous_remaining_source_main
#include "test_wide_remaining_actors.c"
#undef main
static uint8_t now_ram[0x20000];
static unsigned calls,exact_cells,frame_cases,refusals,metadata_cases,projection_cases;
static void original(Interp816*c,unsigned entry,unsigned bank,unsigned frame,unsigned entity){
 c->pc=entry;c->k=bank;c->db=1;c->dp=0;c->sp=0x1d0;c->x=entity;c->y=0x1234;c->a=frame;
 c->mf=1;c->xf=0;c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;
 ram[0x1d1]=0xef;ram[0x1d2]=0x7f;ram[0x1d3]=1;
 unsigned instructions=0;while(c->k!=1||c->pc!=0x7ff0){assert(++instructions<100000);interp816_runOpcode(c);}
 assert(c->sp==(bank?0x1d2:0x1d3));++calls;
}
static void equal_oam(LagoonWideCell*cells,int count){
 int visible=0;for(int i=0;i<count;++i)if(cells[i].y>=-16&&cells[i].y<240)cells[visible++]=cells[i];
 assert(visible==ram[0x236]);
 for(int i=0;i<visible;++i){unsigned a=0x2000+i*4,h=(ram[0x2200+i/4]>>((i%4)*2))&3;
  assert((unsigned)(cells[i].x&511)==(ram[a]|(h&1)<<8));assert((unsigned)(cells[i].y&255)==ram[a+1]);
  assert(cells[i].tile==word(ram,a+2)&&cells[i].size==((h&2)?16:8));++exact_cells;
 }
}
static unsigned first_frame(unsigned selector){for(unsigned f=0;f<255;++f)if(LagoonWideEntities_CurrentSource(rom,sizeof(rom),selector,f))return f;assert(0);return 0;}
static void record(unsigned room,unsigned type,unsigned kind,unsigned frame,unsigned entity,int x,int y){
 memset(ram,0,sizeof(ram));put(0x48e,room);uint8_t*r=ram+0x500+entity;r[0]=0xc0;r[1]=type==2?4:0;r[24]=type;r[27]=kind;r[28]=frame;
 if(type==1&&kind<2){ram[0x4e1]=kind?1:2;}
 put(0x480,256);put(0x482,128);put(0x502+entity,256+x);put(0x504+entity,128+y);put(0x50c+entity,x);put(0x50e + entity,y);
}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof(rom),f)==sizeof(rom));fclose(f);Interp816*c=interp816_init(NULL,read_byte,write_byte);assert(c);
 /* Independently derive each finite pointer table's first payload boundary;
  * shared backreferences never extend a table into another authored payload. */
 for(unsigned selector=0;selector<76;++selector){unsigned a=0x999d+selector*3,t=word(rom,a&32767)|rom[(a+2)&32767]<<16,bound=(t&0xff0000)|65535;
  for(unsigned n=0;n<255&&t+n*2<bound;++n){unsigned at=((t>>16)&127)*32768+((t+n*2)&32767),ptr=(t&0xff0000)|word(rom,at);if(ptr>=t&&ptr<bound)bound=ptr;}
  unsigned extent=(bound-t)/2;for(unsigned frame=0;frame<256;++frame){int expected=frame<extent&&frame<255&&!(selector==0&&(frame<4||frame==168||frame==169));assert(LagoonWideEntities_CurrentSource(rom,sizeof(rom),selector,frame)==expected);}
 }
 const int ys[]={80,-16,239,-32};const unsigned palettes[]={0,0x0f,0x70};
 for(unsigned selector=0;selector<76;++selector)for(unsigned frame=0;frame<256;++frame){
  if(!LagoonWideEntities_CurrentSource(rom,sizeof(rom),selector,frame)){++refusals;continue;}++frame_cases;
  for(unsigned y=0;y<4;++y)for(unsigned pal=0;pal<3;++pal){memset(ram,0,sizeof(ram));LagoonWideCell cells[64];int n=LagoonWideEntities_Decode(rom,sizeof(rom),selector,frame,100,ys[y],cells,64);assert(n>0);
   for(int i=0;i<n;++i){if(pal==1)cells[i].tile=(cells[i].tile&~0xe00)|0xe00;if(pal==2)cells[i].tile=(cells[i].tile&~0x3000)|0x3000;}
   ram[0x2a1]=selector;ram[0x2a2]=255;ram[0x29e]=palettes[pal];put(0x27f,100);put(0x281,ys[y]);original(c,0x8b71,0,frame,0x40);equal_oam(cells,n);
  }
  /* Table pointer and both ends of its actual stream revoke warm authority. */
  unsigned a=0x999d+selector*3,t=word(rom,a&32767)|rom[(a+2)&32767]<<16;
  unsigned entry=((t>>16)&127)*32768+((t+frame*2)&32767),stream=(t&0xff0000)|word(rom,entry),start=((stream>>16)&127)*32768+(stream&32767);
  const unsigned muts[]={a&32767,entry,entry+1,start,start+1};
  for(unsigned j=0;j<5;++j){rom[muts[j]]^=1;assert(!LagoonWideEntities_CurrentSource(rom,sizeof(rom),selector,frame));rom[muts[j]]^=1;++refusals;}
 }
 /* Every whitelisted ordinary kind exercises original full draw metadata.
  * First finite entry / all hit+priority/height/DP90 branches; every frame was checked by the generic producer above. */
 for(unsigned room=0;room<75;++room)for(unsigned type=1;type<=5;++type){if(type!=1&&type!=2&&type!=5)continue;
  for(unsigned kind=0;kind<(type==1?92:type==2?38:16);++kind){if(!LagoonWideEntities_AllowedKind(room,type,kind))continue;
   unsigned selector=type==5||(type==1&&kind<2)?0:room+1;
   for(unsigned sample=0;sample<4;++sample){unsigned frame=type==5?0x3e:first_frame(selector),entity=type==1&&kind<2?0x800:0x40;
    record(room,type,kind,frame,entity,100,ys[sample]);uint8_t*r=ram+0x500+entity;
    r[0x15]=sample*25;r[0x1d]=sample;r[0x10]=sample*13;r[0x11]=sample*2;
    if(type==2&&sample==1)r[1]|=0x80;if(type==2&&sample==2)r[0x1f]|=2;ram[0x50]=room==7?2:0;put(0x90,(unsigned)(int16_t)(sample==1?-7:sample==2?9:0));
    memcpy(now_ram,ram,sizeof(ram));uint8_t actual;
    assert(LagoonWideEntities_CurrentFrame(rom,sizeof(rom),ram,now_ram,entity,&actual)&&actual==frame);
    LagoonWideCell cells[64];int n=LagoonWideEntities_RecordCells(rom,sizeof(rom),ram,entity,frame,1,cells,64);assert(n>0);
    ram[0x2a2]=255;original(c,type==1?0xc1c6:type==2?0xc214:0xc44e,1,0,entity);equal_oam(cells,n);++metadata_cases;
   }
  }
 }
 /* Actual original projection+culling produces the current record before
  * AB31 is skipped for a far actor. No AI body is called by this test. */
 for(int x=-224;x<=448;x+=8)for(int y=-40;y<=296;y+=8){record(0x14,2,13,0,0x40,x,y);put(0x54c,0x2222);put(0x54e,0x3333);original(c,0xa5aa,1,0,0x40);assert(word(ram,0x54c)==(unsigned)(x&65535)&&word(ram,0x54e)==(unsigned)(y&65535));++projection_cases;unsigned queued=x>=-96&&x<352&&y>=-32&&y<288;
  original(c,0xa624,1,0,0x40);assert(word(ram,0x48c)==queued);assert(!!(ram[0x540]&8)==queued);if(queued)assert(word(ram,0xec0)==0x40&&word(ram,0xec2)==(unsigned)(y&65535)&&word(ram,0xec4)==2);
 }
 record(0x14,2,13,0,0x40,-140,80);memcpy(now_ram,ram,sizeof(ram));uint8_t frame;assert(LagoonWideEntities_CurrentFrame(rom,sizeof(rom),ram,now_ram,0x40,&frame));
 for(unsigned at=0;at<64;++at){now_ram[0x540+at]^=1;assert(!LagoonWideEntities_CurrentFrame(rom,sizeof(rom),ram,now_ram,0x40,&frame));now_ram[0x540+at]^=1;++refusals;}
 const unsigned src[][2]={{0x180b2,24},{0x180cd,22},{0x1a4d3,24},{0x1a5aa,191},{0x8b4b,331},{0x8d0b,43},{0x8211,138},{0x1c402,25},{0x1c486,41},{0x1c214,112}};
 for(unsigned j=0;j<10;++j)for(unsigned i=0;i<src[j][1];++i){unsigned at=((src[j][0]>>16)&127)*32768+(src[j][0]&32767)+i;rom[at]^=1;assert(!LagoonWideEntities_CurrentFrame(rom,sizeof(rom),ram,now_ram,0x40,&frame));rom[at]^=1;++refusals;}
 ram[0x541]=now_ram[0x541]=1;assert(!LagoonWideEntities_CurrentFrame(rom,sizeof(rom),ram,now_ram,0x40,&frame));++refusals;
 ram[0x541]=now_ram[0x541]=4;ram[0x55c]=now_ram[0x55c]=255;assert(!LagoonWideEntities_CurrentFrame(rom,sizeof(rom),ram,now_ram,0x40,&frame));++refusals;
 /* Approved camera trampoline is a complete alternative source contract. */
 record(0x14,2,13,0,0x40,-140,80);memcpy(now_ram,ram,sizeof(ram));
 static const uint8_t camera[]={0x5a,0x9b,0xd0,0x16,0xda,0x64,0x41,0x20,0xfb,0x83,0x20,0x26,0x84,0x20,0x52,0x84,0x20,0x7d,0x84,0xfa,0xad,0x19,0x05,0x8d,0x19,0x05,0x7a,0x4c,0xb5,0xa7,0xea,0xea,0xea,0xea,0xea};
 uint8_t old[35];memcpy(old,rom+0x83d8,35);memcpy(rom+0x83d8,camera,35);memset(rom+0x809d,0xea,3);rom[0x80cb]=0xd8;rom[0x80cc]=0x83;
 assert(LagoonWideEntities_CurrentFrame(rom,sizeof(rom),ram,now_ram,0x40,&frame));
 for(unsigned n=0;n<35;++n){rom[0x83d8+n]^=1;assert(!LagoonWideEntities_CurrentFrame(rom,sizeof(rom),ram,now_ram,0x40,&frame));rom[0x83d8+n]^=1;++refusals;}
 for(unsigned n=0;n<3;++n){rom[0x809d+n]^=1;assert(!LagoonWideEntities_CurrentFrame(rom,sizeof(rom),ram,now_ram,0x40,&frame));rom[0x809d+n]^=1;++refusals;}
 memcpy(rom+0x83d8,old,35);rom[0x809d]=0x20;rom[0x809e]=0xd8;rom[0x809f]=0x83;rom[0x80cb]=0xb5;rom[0x80cc]=0xa7;
 assert(LagoonWideEntities_CurrentFrame(rom,sizeof(rom),ram,now_ram,0x40,&frame));
 printf("current actors: %u finite source frames; %u full original opcode calls/%u exact OAM cells, %u complete type/kind metadata cases, %u original projection cases; %u frame/table/stream/source/current-record/death/FF refusals PASS\n",frame_cases,calls,exact_cells,metadata_cases,projection_cases,refusals);interp816_free(c);return 0;
}
