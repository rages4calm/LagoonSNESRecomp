/* The real original cloud floor dispatcher and complete door selection. */
#define main previous_qol_main
#include "lagoon_qol_test.c"
#undef main
#include "snes/interp816.h"
static uint8_t source[0x100000],initial[0x20000];
static Interp816 *cpu;
static CpuWramReadContext context;
static uint8_t read_bus(void*ctx,uint32_t addr){
 (void)ctx;unsigned b=(addr>>16)&127,a=addr&65535;
 if(b==126||b==127)return g_ram[(b-126)*65536+a];
 if(a>=32768)return source[b*32768+(a&32767)];
 if(a>=0x4214&&a<=0x4217){unsigned v=a<0x4216?(g_ram[0x4206]?getword(0x4204)/g_ram[0x4206]:65535):g_ram[0x4202]*g_ram[0x4203];return (uint8_t)(v>>((a&1)*8));}
 assert(a<8192);return qol_cloud_read(a,g_ram[a],&context);
}
static void write_bus(void*ctx,uint32_t addr,uint8_t v){(void)ctx;unsigned b=(addr>>16)&127,a=addr&65535;
 if(b==126||b==127){g_ram[(b-126)*65536+a]=v;return;}if(a>=0x4202&&a<=0x4206){g_ram[a]=v;return;}if(a>=8192)fprintf(stderr,"unknownwrite %06X pc%04X\n",addr,cpu->pc);assert(a<8192);g_ram[a]=v;
}
static unsigned test_point(unsigned x,unsigned y,int qualified){
 memcpy(g_ram,initial,sizeof initial);memset(&g_cloud,0,sizeof g_cloud);g_cloud_source=qualified;
 g_ram[0x500]=0xc0;g_ram[0x501]=g_ram[0x100e]=g_ram[0x41b]=g_ram[0x4e1]=g_ram[0x51b]=0;g_ram[0x417]=1;
 g_ram[0x502]=(uint8_t)x;g_ram[0x503]=(uint8_t)(x>>8);g_ram[0x504]=(uint8_t)y;g_ram[0x505]=(uint8_t)(y>>8);
 cpu=interp816_init(NULL,read_bus,write_bus);assert(cpu);cpu->k=cpu->db=1;cpu->pc=0xc7f6;cpu->sp=0x1d0;cpu->mf=1;cpu->xf=cpu->e=cpu->dp=0;cpu->i=1;cpu->x=cpu->y=0;
 g_ram[0x1d1]=0xa2;g_ram[0x1d2]=0x80;
 unsigned n=0;
 while(cpu->pc!=0xc8cd&&cpu->pc!=0xc958&&cpu->pc!=0xc83f){if(++n>=4000){fprintf(stderr,"loop at x%u y%u PC%04X Y%04X dp%06x probe%u,%u tag%u active%u stack%04X ret%04X M%d\n",x,y,cpu->pc,cpu->y,getword(0x40)|(g_ram[0x42]<<16),getword(0xb8),getword(0xba),g_ram[0x2c1],g_cloud.active,cpu->sp,getword(cpu->sp+1),cpu->mf);assert(0);}context=(CpuWramReadContext){((unsigned)cpu->k<<16)|cpu->pc,cpu->sp,cpu->y,cpu->k,cpu->mf,cpu->xf,cpu->e,1};interp816_runOpcode(cpu);}
 unsigned result=cpu->pc;
 if(result==0xc8cd){assert(g_ram[0x48e]==0x2b);assert(getword(0x502)==512&&getword(0x504)==972);}
 interp816_free(cpu);return result;
}
int main(int argc,char**argv){
 assert(argc==3);FILE*f=fopen(argv[1],"rb");assert(f&&fread(source,1,sizeof source,f)==sizeof source);fclose(f);
 f=fopen(argv[2],"rb");assert(f&&fread(initial,1,sizeof initial,f)==sizeof initial);fclose(f);
 g_recovery_rom=source;g_recovery_rom_size=sizeof source;assert(qol_cloud_original(source,sizeof source));
 unsigned points[][2]={{176,129},{335,129},{176,192},{335,192},{192,113},{319,113},{192,208},{319,208},{176,113},{335,208},{256,160}};
 for(unsigned n=0;n<sizeof points/sizeof*points;n++)assert(test_point(points[n][0],points[n][1],1)==0xc8cd);
 assert(test_point(175,160,1)==0xc958);assert(test_point(336,160,1)==0xc958);
 assert(test_point(256,112,1)==0xc958);assert(test_point(256,209,1)==test_point(256,209,0));
 assert(test_point(176,160,0)==0xc958);
 unsigned guards=0;
 for(unsigned n=0;n<12;n++){
  memcpy(g_ram,initial,sizeof initial);memset(&g_cloud,0,sizeof g_cloud);g_cloud_source=1;
  g_ram[0x500]=0xc0;g_ram[0x501]=g_ram[0x100e]=g_ram[0x41b]=0;g_ram[0x417]=1;g_ram[0x518]=0;
  g_ram[0x502]=176;g_ram[0x503]=0;g_ram[0x504]=160;g_ram[0x505]=0;g_ram[0xb8]=176;g_ram[0xb9]=0;g_ram[0xba]=159;g_ram[0xbb]=0;g_ram[0x2c1]=4;g_ram[0x1d1]=0xa2;g_ram[0x1d2]=0x80;
  CpuWramReadContext c={0x01c81b,0x1d0,0,1,1,0,0,1};
  switch(n){case 0:c.interpreter=0;break;case 1:c.pb=2;break;case 2:c.m=0;break;case 3:c.x=1;break;case 4:c.e=1;break;case 5:c.stack--;break;case 6:c.instruction_pc++;break;case 7:g_ram[0x50]^=1;break;case 8:g_ram[0x48e]=0x29;break;case 9:g_ram[0x501]=32;break;case 10:g_ram[0xb8]++;break;case 11:g_ram[0x100e]=255;break;}
  assert(qol_cloud_read(0x2c1,4,&c)==4&&!g_cloud.active);guards++;
 }
 /* The second half needs the witnessed current floor query; stale state
  * must never remap the original door walker's cell comparisons. */
 for(unsigned n=0;n<6;n++){
  memcpy(g_ram,initial,sizeof initial);memset(&g_cloud,0,sizeof g_cloud);g_cloud_source=1;
  g_ram[0x500]=0xc0;g_ram[0x501]=g_ram[0x100e]=g_ram[0x41b]=0;g_ram[0x417]=1;g_ram[0x518]=0;
  g_ram[0x502]=176;g_ram[0x503]=0;g_ram[0x504]=160;g_ram[0x505]=0;g_ram[0xb8]=176;g_ram[0xb9]=0;g_ram[0xba]=159;g_ram[0xbb]=0;g_ram[0x2c1]=4;g_ram[0x1d1]=0xa2;g_ram[0x1d2]=0x80;
  CpuWramReadContext c={0x01c81b,0x1d0,0,1,1,0,0,1};assert(qol_cloud_read(0x2c1,4,&c)==1);
  g_ram[0x40]=0x61;g_ram[0x41]=0x88;g_ram[0x42]=5;g_ram[0x102b]=5;g_ram[0x102c]=0;
  c.instruction_pc=0x01c892;c.m=0;assert(qol_cloud_read(0x102b,5,&c)==6);
  switch(n){case 0:state_generation++;break;case 1:g_ram[0x408]++;break;case 2:g_ram[0x502]++;break;case 3:g_ram[0x40]++;break;case 4:c.y=1;break;case 5:g_cloud.active=0;break;}
  uint8_t before[sizeof g_ram];memcpy(before,g_ram,sizeof g_ram);
  assert(qol_cloud_read(0x102b,5,&c)==5);assert(!memcmp(before,g_ram,sizeof g_ram));guards++;
 }
 const unsigned spans[][2]={{0x80a0,3},{0xc7f6,247},{0xca05,31},{0x8385,83},{0x2bc0f,56},{0x280ad,2},{0x28861,90}};
 unsigned mutations=0;
 for(unsigned i=0;i<sizeof spans/sizeof*spans;i++)for(unsigned j=0;j<spans[i][1];j++){
  source[spans[i][0]+j]^=1;assert(!qol_cloud_original(source,sizeof source));source[spans[i][0]+j]^=1;mutations++;
 }
 printf("PASS cloud54 %u context refusals and %u source byte mutations\n",guards,mutations);
 printf("PASS cloud54 11 original-source doorway landing/corner points, 4 outer misses and disabled-source control\n");return 0;
}
