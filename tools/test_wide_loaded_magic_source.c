/* Execute original equipped-pack loader/decompressor/queue/NMI/RTI. Only
 * hardware MMIO/DMA is modeled; no decompressed asset bytes are embedded. */
#define main old_parallax_main
#define writeb old_writeb
#include "test_wide_parallax_sources.c"
#undef writeb
#undef main
static LagoonWideMagicAtlas loaded[16];
static uint16_t resident[16][32768];
static unsigned owner_refusals,source_refusals,partial_refusals;
static void hardware(void *ctx,uint32_t address,uint8_t value) {
  (void)ctx;unsigned bank=address>>16,a=address&65535;
  if(bank==126||bank==127){ram[(bank-126)*65536+a]=value;return;}
  if((bank&127)<64&&a<8192){ram[a]=value;return;}
  assert((bank&127)<64&&a<32768);io[a]=value;
  if(a==0x4202||a==0x4203){unsigned v=io[0x4202]*io[0x4203];io[0x4216]=v;io[0x4217]=v>>8;}
  if(a==0x420b) {
    unsigned from=word(io,0x4312),source_bank=io[0x4314];
    unsigned to=word(io,0x2116),length=word(io,0x4315);
    assert(value==2&&io[0x4310]==1&&io[0x4311]==0x18);
    assert(LagoonWideMagic_Begin(&loaded[scene],44,0x14,rom,sizeof rom,
        scene,source_bank,from,to,length));
    for(unsigned n=0;n<length;++n) {
      uint8_t b=readb(NULL,(source_bank<<16)|((from+n)&65535));
      vram[to*2+n]=b;LagoonWideMagic_Write(&loaded[scene],to*2+n,b);
    }
    assert(loaded[scene].ready);memcpy(resident[scene],vram,sizeof vram);++jobs;
  }
}
static void refused(LagoonWideMagicAtlas *a,unsigned spell) {
  LagoonWideMagicAtlas before=*a;
  assert(!LagoonWideMagic_RebindResident(a,resident[spell],rom,sizeof rom,44,0x14,spell));
  assert(!memcmp(a,&before,sizeof before));
}
int main(int argc,char **argv) {
  assert(argc==2);FILE *f=fopen(argv[1],"rb");
  assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  Interp816 *c=interp816_init(NULL,readb,hardware);assert(c);
  for(scene=0;scene<16;++scene)if(scene!=3) {
    memset(ram,0,sizeof ram);memset(io,0,sizeof io);ram[0x537]=scene;
    initcpu(c,1,0x8ea6,0x1d0,2);unsigned n=0;
    while(c->k!=1||c->pc!=0x7ff0) {
      assert(++n<1000000);
      if(c->k==0&&c->pc==0x89ac&&(ram[0x23a]&32))c->nmiWanted=true;
      interp816_runOpcode(c);
    }
    assert(c->sp==0x1d2);LagoonWideMagicAtlas a={0};
    assert(LagoonWideMagic_RebindResident(&a,resident[scene],rom,sizeof rom,44,0x14,scene));
    assert(!memcmp(a.bytes,loaded[scene].bytes,a.length));
    assert(a.ready&&a.length==a.completed&&!a.left&&a.next==0xb800+a.length);
  }
  assert(jobs==15);
  const unsigned selected[]={0,4,8,12,14};
  for(unsigned j=0;j<5;++j) {
    unsigned spell=selected[j];LagoonWideMagicAtlas a={0};
    assert(LagoonWideMagic_RebindResident(&a,resident[spell],rom,sizeof rom,44,0x14,spell));
    for(unsigned b=0;b<a.length;++b) {
      ((uint8_t*)resident[spell])[0xb800+b]^=1;LagoonWideMagicAtlas empty={0};refused(&empty,spell);
      ((uint8_t*)resident[spell])[0xb800+b]^=1;++owner_refusals;
      LagoonWideMagicAtlas changed=a;LagoonWideMagic_Write(&changed,0xb800+b,a.bytes[b]);
      assert(!LagoonWideMagic_Match(&changed,resident[spell],rom,sizeof rom,44,0x14,spell));++owner_refusals;
    }
    for(unsigned n=0;n<=a.length;++n) {
      LagoonWideMagicAtlas partial={0};partial.length=a.length;
      partial.completed=n;partial.left=a.length-n;refused(&partial,spell);++partial_refusals;
    }
    assert(!LagoonWideMagic_Match(&a,resident[spell],rom,sizeof rom,45,0x14,spell));
    assert(!LagoonWideMagic_Match(&a,resident[spell],rom,sizeof rom,44,0x15,spell));
    for(unsigned other=0;other<256;++other) {
      int expected=other<=2?j==0:other>=4&&other<=7?j==1:other>=8&&other<=11?j==2:
          other==12||other==13||other==15?j==3:other==14?j==4:0;
      assert(!!LagoonWideMagic_Match(&a,resident[spell],rom,sizeof rom,44,0x14,other)==expected);
    }
  }
  const unsigned shared[][2]={{0x18ea6,130},{0x18f28,224},{0x188cd,20},
    {0x9143,445},{0x908a,81},{0x884b,56},{0x89a7,13},{0x87b4,120}};
  for(unsigned j=0;j<8;++j)for(unsigned b=0;b<shared[j][1];++b) {
    rom[off(shared[j][0])+b]^=1;
    for(unsigned k=0;k<5;++k){LagoonWideMagicAtlas empty={0};refused(&empty,selected[k]);}
    rom[off(shared[j][0])+b]^=1;++source_refusals;
  }
  const unsigned compressed[][2]={{0x11cd14,920},{0x11dd64,1437},{0x11e301,1704},
    {0x11efea,915},{0x11e9a9,1601}};
  for(unsigned j=0;j<5;++j)for(unsigned b=0;b<compressed[j][1];++b) {
    rom[off(compressed[j][0])+b]^=1;LagoonWideMagicAtlas empty={0};refused(&empty,selected[j]);
    rom[off(compressed[j][0])+b]^=1;++source_refusals;
  }
  LagoonWideMagicAtlas empty={0};
  assert(!LagoonWideMagic_RebindResident(&empty,resident[12],rom,sizeof rom,0,0x14,13));
  assert(!LagoonWideMagic_RebindResident(&empty,resident[12],rom,sizeof rom,44,75,13));
  assert(!LagoonWideMagic_RebindResident(&empty,resident[12],rom,sizeof rom,44,0x14,3));
  assert(!LagoonWideMagic_RebindResident(&empty,resident[12],rom,sizeof rom,44,0x14,16));
  assert(!LagoonWideMagic_RebindResident(NULL,resident[12],rom,sizeof rom,44,0x14,13));
  assert(!LagoonWideMagic_RebindResident(&empty,NULL,rom,sizeof rom,44,0x14,13));
  assert(!LagoonWideMagic_RebindResident(&empty,resident[12],NULL,sizeof rom,44,0x14,13));
  assert(!LagoonWideMagic_RebindResident(&empty,resident[12],rom,sizeof rom-1,44,0x14,13));
  interp816_free(c);
  printf("loaded magic:15 original8EA6/decompress/queue/NMI/RTI/DMA packs, five complete families; %u resident/write/%u original-source/%u partial readonly refusals; form/room/generation/invalid/null/size guards PASS\n",
      owner_refusals,source_refusals,partial_refusals);return 0;
}
