/* Original BB4B animation/C41B draw and BCCA queue/cull are executed
 * independently. No AI, collision results, graphics or source code are copied. */
#define main previous_remaining_main
#include "test_wide_remaining_actors.c"
#undef main
static uint8_t current[0x20000];
static unsigned frame_calls,draw_calls,cells_equal,cull_calls,refusals,sort_calls;
static void execute(Interp816*c,unsigned entry,unsigned x){
 c->pc=entry;c->k=c->db=1;c->dp=0;c->sp=0x1d0;c->x=x;c->y=0;c->a=0;
 c->mf=1;c->xf=0;c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;
 ram[0x1d1]=0xef;ram[0x1d2]=0x7f;
 unsigned steps=0;while(c->k!=1||c->pc!=0x7ff0){assert(++steps<100000);interp816_runOpcode(c);}assert(c->sp==0x1d2);
}
static void setup(unsigned kind,unsigned clock,unsigned direction,unsigned offset,int x,int y){
 memset(ram,0,sizeof ram);put(0x48e,0x14);put(0x480,256);put(0x482,128);
 unsigned a=0xd40+offset;ram[a]=kind;ram[a+16]=direction;ram[a+17]=1;ram[a+18]=clock;
 put(a+1,256+x);put(a+3,128+y);put(a+9,x);put(a+11,y);
 put(0x48c,1);put(0xec0,offset);put(0xec2,y);put(0xec4,4);
}
static void compare(const LagoonWideCell*c,int n){unsigned count=0;
 for(int i=0;i<n;++i)if(c[i].y>=-16&&c[i].y<240){unsigned a=0x2000+count*4,h=(ram[0x2200+count/4]>>((count%4)*2))&3;
  assert((unsigned)(c[i].x&511)==(ram[a]|(h&1)<<8));assert((unsigned)(c[i].y&255)==ram[a+1]);
  assert(c[i].tile==word(ram,a+2)&&c[i].size==((h&2)?16:8));++count;++cells_equal;
 }assert(count==ram[0x236]);
}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);Interp816*c=interp816_init(NULL,read_byte,write_byte);assert(c);
 const int xs[]={-96,-24,0,248,280,351},ys[]={-32,-16,80,239,287};
 for(unsigned kind=11;kind<=33;++kind)for(unsigned clock=0;clock<256;++clock)for(unsigned direction=0;direction<8;++direction){
  unsigned sample=(kind+clock+direction)%30,offset=((kind+clock+direction)%16)*24;
  setup(kind,clock,direction,offset,xs[sample%6],ys[sample/6]);execute(c,0xbb4b,offset);++frame_calls;
  memcpy(current,ram,sizeof ram);LagoonWideCell cells[64];int n=LagoonWideEntities_ProjectileCells(rom,sizeof rom,ram,current,0,cells,64);assert(n>0);
  ram[0x2a2]=255;execute(c,0xc41b,offset);++draw_calls;compare(cells,n);
  for(int i=0;i<n;++i){unsigned base=cells[i].tile&256?0x5000:0x4000;for(unsigned y=0;y<cells[i].size;y+=8)for(unsigned x=0;x<cells[i].size;x+=8){unsigned t=(((cells[i].tile&255)/16+y/8)*16)|(((cells[i].tile&15)+x/8)&15);assert(base+t*16>=0x4400&&base+t*16+16<=0x5900);}}
 }
 /* Real BCCA movement+projection+queue. Crossing cull bounds deletes the
  * projectile and records no queued draw; the host must not extend lifetime. */
 for(int x=-104;x<=360;x+=8)for(int y=-40;y<=296;y+=8){setup(11,0,0,0,x,y);put(0x48c,0);execute(c,0xbcca,0);++cull_calls;unsigned alive=x>=-96&&x<352&&y>=-32&&y<288;
  assert(!!ram[0xd40]==alive&&word(ram,0x48c)==alive);if(alive){assert(word(ram,0xec4)==4&&word(ram,0xec2)==(unsigned)(y&65535));ram[0xd4f]=0x12;}
  memcpy(current,ram,sizeof ram);LagoonWideCell cells[64];assert((LagoonWideEntities_ProjectileCells(rom,sizeof rom,ram,current,0,cells,64)>0)==alive);
 }
 /* Execute the original stable sort/dispatch over actual ordinary NPC and
  * projectile entries with three ties, deliberately appended unsorted. */
 for(unsigned sample=0;sample<6;++sample){setup(11,0,0,0,280,80);ram[0xd4f]=0x12;
  const unsigned ys0[6][3]={{70,80,90},{80,80,80},{90,80,70},{80,70,80},{70,80,70},{80,90,80}};
  unsigned key[6]={ys0[sample][0],ys0[sample][1],ys0[sample][2],ys0[sample][1],ys0[sample][0],ys0[sample][2]};
  for(unsigned j=0;j<3;++j){unsigned a=0x540+j*64;ram[a]=0xc8;ram[a+24]=1;ram[a+27]=41;ram[a+28]=0;put(a+12,100);put(a+14,key[j]);put(a+2,356);put(a+4,128+key[j]);}
  for(unsigned j=0;j<3;++j){unsigned a=0xd40+j*24;ram[a]=11;ram[a+15]=0x12;ram[a+17]=1;put(a+9,280);put(a+11,key[3+j]);put(a+1,536);put(a+3,128+key[3+j]);}
  put(0x48c,6);for(unsigned j=0;j<6;++j){put(0xec0+j*6,j<3?0x40+j*64:(j-3)*24);put(0xec2+j*6,key[j]);put(0xec4+j*6,j<3?1:4);}
  /* Stop immediately before actual dispatch, after original stable sort.
   * Do not run NPC animation or replace any guest outcome. */
  c->pc=0xc026;c->k=c->db=1;c->dp=0;c->sp=0x1d0;c->x=c->y=c->a=0;c->mf=1;c->xf=0;c->e=c->d=c->c=c->n=c->v=c->z=0;c->i=1;
  unsigned steps=0;while(c->pc!=0xc08b){assert(++steps<10000);interp816_runOpcode(c);}++sort_calls;
  for(unsigned j=0;j<6;++j){unsigned rank=0;for(unsigned o=0;o<6;++o)if(key[o]>key[j]||(key[o]==key[j]&&o<j))rank++;
   assert(word(ram,0xec0+rank*6)==(j<3?0x40+j*64:(j-3)*24));assert(word(ram,0xec4+rank*6)==(j<3?1:4));}
 }
 setup(11,0,0,0,280,80);ram[0xd4f]=0x12;memcpy(current,ram,sizeof ram);LagoonWideCell cells[64];assert(LagoonWideEntities_ProjectileCells(rom,sizeof rom,ram,current,0,cells,64)>0);
 for(unsigned n=0;n<24;++n){current[0xd40+n]^=1;assert(LagoonWideEntities_ProjectileCells(rom,sizeof rom,ram,current,0,cells,64)<0);current[0xd40+n]^=1;++refusals;}
 const unsigned source[][2]={{0x1bb08,67},{0x1bb4b,383},{0x1bcca,145},{0x1c026,164},{0x1c41b,51},{0x1fe1f,136}};
 for(unsigned j=0;j<6;++j)for(unsigned n=0;n<source[j][1];++n){unsigned at=((source[j][0]>>16)&127)*32768+(source[j][0]&32767)+n;rom[at]^=1;assert(LagoonWideEntities_ProjectileCells(rom,sizeof rom,ram,current,0,cells,64)<0);rom[at]^=1;++refusals;}
 for(unsigned kind=0;kind<256;++kind){if(kind>=11&&kind<=33)continue;ram[0xd40]=current[0xd40]=kind;assert(LagoonWideEntities_ProjectileCells(rom,sizeof rom,ram,current,0,cells,64)<0);++refusals;}
 printf("projectiles: %u original BB4B frames/%u C41B draws/%u exact OAM cells, %u actual original cull cases, %u original stable unsigned-y tie sorts; %u source/full-record/unowned-kind refusals PASS\n",frame_calls,draw_calls,cells_equal,cull_calls,sort_calls,refusals);interp816_free(c);return 0;
}
