/* Original dictionary decodes in the real8869 order prove every resident
 * byte, including the BG tail left by shorter actor art. No asset arrays. */
#define main previous_parallax_main
#include "test_wide_parallax_sources.c"
#undef main
static unsigned decoded,source_refusals,byte_refusals,metadata_refusals;
static uint16_t resident[32768];
static void prepare(Interp816*c,unsigned room){
 memset(ram,0,sizeof ram);memset(io,0,sizeof io);memset(resident,0,sizeof resident);
 memcpy(ram+0x50,rom+off(0x05b2df+room*56),56);ram[0x48e]=room;
 for(unsigned pass=0;pass<2;++pass){unsigned ptr=pass?0x5d:0x57;
  source=ram[ptr]|ram[ptr+1]<<8|ram[ptr+2]<<16;maximum=source;
  ram[0xe0]=source;ram[0xe1]=source>>8;ram[0xe2]=source>>16;
  initcpu(c,0,0x9143,0x1d0,3);ram[0x1d3]=1;
  unsigned count=0;while(c->k!=1||c->pc!=0x7ff0){assert(++count<3000000);interp816_runOpcode(c);}
  if(!pass&&(room==8||room==9))assert(word(ram,0xf9)==11748);
  ++decoded;
 }
 memcpy((uint8_t*)resident+0x8800,ram+0x17f80,kLagoonWideAtlasBytes);
}
static void refused(LagoonWideAtlas *a){LagoonWideAtlas before=*a;
 assert(!LagoonWideAtlas_RebindResident(a,rom,sizeof rom,ram,resident,41));
 assert(!memcmp(a,&before,sizeof before));}
int main(int argc,char**argv){assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);Interp816*c=interp816_init(NULL,readb,writeb);
 for(unsigned room=8;room<=9;++room){prepare(c,room);LagoonWideAtlas empty={0},a=empty;
  assert(LagoonWideAtlas_RebindResident(&a,rom,sizeof rom,ram,resident,41));
  assert(a.ready&&a.completed==kLagoonWideAtlasBytes&&!a.transfer_left&&a.generation==41&&a.room==room);
  for(unsigned b=0;b<kLagoonWideAtlasBytes;++b)assert(a.owned[b]);
  LagoonWideCell cell={-180,30,0x2640,16};assert(LagoonWideAtlas_CellOwned(&a,0x4000,0x5000,&cell));
  for(unsigned b=0;b<kLagoonWideAtlasBytes;++b){((uint8_t*)resident)[0x8800+b]^=1;refused(&empty);((uint8_t*)resident)[0x8800+b]^=1;++byte_refusals;}
  const unsigned spans[][2]={{0x018869,42},{0x009143,445},{0x0188cd,20},{0x00908a,81},{0x12b297,10197},{room==8?0x1891e4:0x199f6c,room==8?3790:4068},{0x05b2df+room*56,56}};
  for(unsigned s=0;s<sizeof spans/sizeof*spans;++s)for(unsigned b=0;b<spans[s][1];++b){rom[off(spans[s][0])+b]^=1;refused(&empty);rom[off(spans[s][0])+b]^=1;++source_refusals;}
  for(unsigned b=0;b<56;++b){ram[0x50+b]^=1;refused(&empty);ram[0x50+b]^=1;++metadata_refusals;}
  refused(&a);a=empty;a.completed=1;refused(&a);a=empty;a.transfer_left=1;refused(&a);a=empty;a.ready=1;refused(&a);metadata_refusals+=4;
  ram[0x100e]=1;refused(&empty);ram[0x100e]=0;ram[0x48f]=1;refused(&empty);ram[0x48f]=0;metadata_refusals+=2;
  assert(!LagoonWideAtlas_RebindResident(&empty,rom,sizeof rom,ram,resident,0));++metadata_refusals;
  /* A post-rebind unknown write still revokes exactly its original tile. */
  a=empty;assert(LagoonWideAtlas_RebindResident(&a,rom,sizeof rom,ram,resident,41));LagoonWideAtlas_Write(&a,0x8800);assert(!LagoonWideAtlas_CellOwned(&a,0x4000,0x5000,&cell));
 }
 for(unsigned room=0;room<75;++room)if(room!=8&&room!=9){ram[0x48e]=room;memcpy(ram+0x50,rom+off(0x05b2df+room*56),56);LagoonWideAtlas a={0};refused(&a);++metadata_refusals;}
 printf("loaded actor atlas: %u original009143 source decodes;2 complete BG-tail-aware08/09 resident packs; %u resident-byte/%u source-byte/%u metadata/partial/unknown-family readonly refusals PASS\n",decoded,byte_refusals,source_refusals,metadata_refusals);
 interp816_free(c);return 0;
}
