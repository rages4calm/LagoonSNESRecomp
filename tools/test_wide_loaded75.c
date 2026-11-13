/* Independent original009143 decodes qualify save-load actor prefixes; no
 * embedded graphics and no gameplay/scheduler claim. */
#define LAGOON_ATLAS75_NO_MAIN
#include "qualify_wide_loaded75.c"
#undef LAGOON_ATLAS75_NO_MAIN
static uint16_t resident[32768];
static unsigned accepted,refused;
static void refusal(LagoonWideAtlas *a){LagoonWideAtlas old=*a;assert(!LagoonWideAtlas_RebindResident(a,rom,sizeof rom,ram,resident,77));assert(!memcmp(a,&old,sizeof old));++refused;}
int main(int argc,char**argv){assert(argc>=2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);Interp816*c=interp816_init(NULL,readb,writeb);
 for(unsigned room=0;room<75;++room){
  if(room==0||room==8||room==9||room==0x14)continue; /* Established full/tail-aware contracts are tested separately. */
  memset(ram,0,sizeof ram);memset(io,0,sizeof io);memset(resident,0x6b,sizeof resident);
  memcpy(ram+0x50,rom+off(0x05b2df+room*56),56);ram[0x48e]=room;
  unsigned ptr=ram[0x5d]|ram[0x5e]<<8|ram[0x5f]<<16,output=run_decode(c,ptr);
  unsigned owned=output<0x2800?output:0x2800;owned&=~31u;
  memcpy((uint8_t*)resident+0x8800,ram+0x17f80,owned);
  LagoonWideAtlas a={0};assert(LagoonWideAtlas_RebindResident(&a,rom,sizeof rom,ram,resident,77));++accepted;
  assert(a.ready&&a.completed==owned&&a.generation==77&&a.room==room&&a.source==ptr);
  for(unsigned i=0;i<kLagoonWideAtlasBytes;++i)assert(a.owned[i]==(i<owned));
  /* The complete-tile prefix is independently determined by original output;
   * every unproduced historical tail remains unowned and ignored. */
  for(unsigned at=0;at<owned;at+=127){((uint8_t*)resident)[0x8800+at]^=1;LagoonWideAtlas bad={0};refusal(&bad);((uint8_t*)resident)[0x8800+at]^=1;}
  unsigned sourcebyte=off(ptr)+3;rom[sourcebyte]^=1;LagoonWideAtlas bad={0};refusal(&bad);rom[sourcebyte]^=1;
  ram[0x53]^=1;refusal(&bad);ram[0x53]^=1;
  unsigned descriptor=off(0x05b2df+room*56)+5;rom[descriptor]^=1;refusal(&bad);rom[descriptor]^=1;
  bad.completed=1;refusal(&bad);bad.completed=0;bad.transfer_left=1;refusal(&bad);
  if(owned<kLagoonWideAtlasBytes){((uint8_t*)resident)[0x8800+owned]^=1;LagoonWideAtlas tail={0};assert(LagoonWideAtlas_RebindResident(&tail,rom,sizeof rom,ram,resident,77));assert(!tail.owned[owned]);}
 }
 for(int i=2;i<argc;++i){char path[4096];snprintf(path,sizeof path,"%s.wram.bin",argv[i]);FILE*state=fopen(path,"rb");assert(state&&fread(ram,1,sizeof ram,state)==sizeof ram);fclose(state);snprintf(path,sizeof path,"%s.vram.bin",argv[i]);state=fopen(path,"rb");assert(state&&fread(resident,1,sizeof resident,state)==sizeof resident);fclose(state);LagoonWideAtlas a={0};int result=LagoonWideAtlas_RebindResident(&a,rom,sizeof rom,ram,resident,77);printf("ACTUAL room%02X rebind=%d owned=%u\n",word(ram,0x48e),result,a.completed);assert(result);}
 interp816_free(c);printf("PASS %u later-room original-decoder prefixes; %u resident/source/descriptor/partial refusal cases; no tail ownership\n",accepted,refused);return 0;}
