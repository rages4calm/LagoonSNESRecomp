/* Own verified original ROM required; no copied assets are embedded.
 * The real loader tail 018C4D calls both original follower initializers and
 * returns through its actual RTS. Original AC4F and the complete original
 * C1C6/8B71/8211 chain then qualify the production presentation decoder.
 * Camera coordinates/animation phases below are synthetic unit fixtures,
 * not a live escort or proof of hardware divider latency. */
#include "lagoon_wide_entities.h"
#include "snes/interp816.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static uint8_t rom[0x100000],ram[0x20000];
static unsigned word(const uint8_t *p,unsigned a){return p[a]|p[a+1]<<8;}
static void put(unsigned a,unsigned v){ram[a]=(uint8_t)v;ram[a+1]=(uint8_t)(v>>8);}
static unsigned offset(unsigned a){return (a>>16)*32768+(a&32767);}
static uint8_t rd(void *ctx,uint32_t a) {
  (void)ctx;unsigned b=a>>16,p=a&65535;
  if(b==126||b==127)return ram[(b-126)*65536+p];
  if((b&127)<64&&p<8192)return ram[p];
  if((b&127)<64&&(p==0x4214||p==0x4215)) {
    unsigned q=ram[0x4206]?word(ram,0x4204)/ram[0x4206]:65535;
    return (uint8_t)(q>>((p-0x4214)*8));
  }
  assert(b<32&&p>=32768);return rom[offset(a)];
}
static void wr(void *ctx,uint32_t a,uint8_t v) {
  (void)ctx;unsigned b=a>>16,p=a&65535;
  if(b==126||b==127){ram[(b-126)*65536+p]=v;return;}
  if((b&127)<64&&p>=0x4204&&p<=0x4206){ram[p]=v;return;}
  assert((b&127)<64&&p<8192);ram[p]=v;
}
static void run(Interp816 *c,unsigned pc,unsigned entity) {
  c->k=c->db=1;c->pc=pc;c->dp=0;c->sp=0x1d0;c->x=entity;c->y=0;c->a=0;
  c->mf=1;c->xf=0;c->e=c->d=c->c=c->v=c->n=c->z=0;c->i=1;
  put(c->sp+1,0x7fef);unsigned count=0;
  while(c->k!=1||c->pc!=0x7ff0){assert(++count<10000);interp816_runOpcode(c);}
  assert(c->sp==0x1d2);
}
int main(int argc,char **argv) {
  assert(argc==2);FILE*f=fopen(argv[1],"rb");assert(f&&fread(rom,1,sizeof rom,f)==sizeof rom);fclose(f);
  Interp816 *c=interp816_init(NULL,rd,wr);assert(c);
  unsigned initializers=0,draws=0,total_cells=0,parser_refusals=0,source_refusals=0,provenance_checks=0;
  for(unsigned room=0;room<75;room++)for(unsigned flags=0;flags<4;flags++)
  for(unsigned direction=0;direction<4;direction++)for(unsigned camera=0;camera<2;camera++) {
    memset(ram,0,sizeof ram);
    memcpy(ram+0x50,rom+offset(0x05b2df+room*56),56);put(0x48e,room);
    ram[0x4e1]=(uint8_t)flags;ram[0x50a]=(uint8_t)direction;
    put(0x502,720+camera*80);put(0x504,600+camera*96);
    put(0x480,620+camera*80);put(0x482,500+camera*96);
    unsigned before_x=word(ram,0x502),before_y=word(ram,0x504);
    run(c,0x8c4d,0);initializers++;
    assert(ram[0xd00]==(flags?0xc0:0));
    if(!flags)continue;
    unsigned kind=(flags&1)?1:0;
    assert(ram[0xd18]==1&&ram[0xd1b]==kind&&ram[0xd0b]==255&&ram[0xd0a]==direction);
    unsigned t=offset(0x018db6)+direction*4;
    unsigned nasir_offsets=offset(0x018da6)+direction*4;
    unsigned dx=word(rom,nasir_offsets),dy=word(rom,nasir_offsets+2);
    assert(word(ram,0xd02)==(uint16_t)(before_x+word(rom,t)+(flags==3?dx:0)));
    assert(word(ram,0xd04)==(uint16_t)(before_y+word(rom,t+2)+(flags==3?dy:0)));
    assert(word(ram,0x502)==(uint16_t)(before_x+(flags==3?2:1)*dx));
    assert(word(ram,0x504)==(uint16_t)(before_y+(flags==3?2:1)*dy));
    /* Both flags execute BOTH actual initializers, including the first one's
     * displacement of Nasir; the second intentionally overwrites slot D00. */
    uint8_t initialized[64];memcpy(initialized,ram+0xd00,64);
    for(unsigned phase=0;phase<4;phase++)for(unsigned walking=0;walking<2;walking++) {
      memcpy(ram+0xd00,initialized,64);ram[0xd00]|=8; /* Source main draw-queue fixture. */
      ram[0xd0b]=walking?0:255;
      ram[0xd10]=0;ram[0xd11]=(uint8_t)phase;
      put(0xd0c,word(ram,0xd02)-word(ram,0x480));put(0xd0e,word(ram,0xd04)-word(ram,0x482));
      LagoonWideAnimation animation={ram[0xd10],ram[0xd11]};uint8_t frame=255;
      int predicted=LagoonWideEntities_FrameForContext(rom,sizeof rom,ram,ram+0xd00,&animation,1,&frame);
      assert(predicted);
      run(c,0xac4f,0x800);if(ram[0xd1c]!=frame){fprintf(stderr,"frame room%02x flags%u dir%u phase%u walking%u kind%u actual%u expected%u timer%u/%u phase%u/%u\n",room,flags,direction,phase,walking,kind,ram[0xd1c],frame,ram[0xd10],animation.timer,ram[0xd11],animation.phase);assert(0);}
      LagoonWideCell cells[64];int count=LagoonWideEntities_RecordCells(rom,sizeof rom,ram,0x800,frame,0,cells,64);
      if(count<0){parser_refusals++;continue;}
      ram[0x236]=0;memset(ram+0x2000,0,544);ram[0x2a2]=255;
      run(c,0xc1c6,0x800);assert(count==(int)ram[0x236]);
      for(int n=0;n<count;n++) {
        unsigned a=0x2000+n*4,h=(ram[0x2200+n/4]>>((n%4)*2))&3;
        assert((unsigned)(cells[n].x&511)==(ram[a]|(h&1)<<8));
        assert((unsigned)(cells[n].y&255)==ram[a+1]);
        if(cells[n].tile!=word(ram,a+2)||cells[n].size!=((h&2)?16:8)){fprintf(stderr,"OAM room%02x flags%u dir%u phase%u walking%u cell%d frame%u actual%04x expected%04x size%u/%u\n",room,flags,direction,phase,walking,n,frame,word(ram,a+2),cells[n].tile,(h&2)?16:8,cells[n].size);assert(0);}total_cells++;
      }
      draws++;
    }
    /* Representative source/slot/flag mutations are independent from the
     * successful original initializers above; they must never acquire draw
     * authority merely from sharing the global selector-zero table. */
    if(room==3&&direction==0&&!camera&&flags!=3) {
      memcpy(ram+0xd00,initialized,64);ram[0xd00]|=8;
      LagoonWideAnimation animation={0,0};uint8_t frame=0;
      assert(LagoonWideEntities_FrameForContext(rom,sizeof rom,ram,ram+0xd00,&animation,0,&frame));
      LagoonWideCell cells[64];
      const unsigned spans[][2]={{0x018c4d,7},{0x018d41,101},{0x018da6,32}};
      for(unsigned s=0;s<3;s++)for(unsigned n=0;n<spans[s][1];n++) {
        unsigned at=offset(spans[s][0])+n;rom[at]^=1;animation=(LagoonWideAnimation){0,0};
        assert(!LagoonWideEntities_FrameForContext(rom,sizeof rom,ram,ram+0xd00,&animation,0,&frame));
        assert(LagoonWideEntities_RecordCells(rom,sizeof rom,ram,0x800,frame,0,cells,64)<0);
        rom[at]^=1;source_refusals+=2;
      }
      for(unsigned f=0;f<256;f++) {
        ram[0x4e1]=(uint8_t)f;animation=(LagoonWideAnimation){0,0};
        int valid=kind?!!(f&1):((f&3)==2);
        assert(!!LagoonWideEntities_FrameForContext(rom,sizeof rom,ram,ram+0xd00,&animation,0,&frame)==valid);
        assert((LagoonWideEntities_RecordCells(rom,sizeof rom,ram,0x800,frame,0,cells,64)>=0)==valid);
        provenance_checks+=2;
      }
      ram[0x4e1]=(uint8_t)flags;
      for(unsigned slot=0;slot<32;slot++) {
        uint8_t *r=ram+0x500+slot*64;memcpy(r,initialized,64);animation=(LagoonWideAnimation){0,0};
        assert(!LagoonWideEntities_FrameForContext(rom,sizeof rom,ram,r,&animation,0,&frame));
        assert(LagoonWideEntities_RecordCells(rom,sizeof rom,ram,slot*64,frame,0,cells,64)<0);
        provenance_checks+=2;
      }
    }
  }
  interp816_free(c);
  printf("PASS original follower loader-tail/8D41/8D4F: %u full original calls across75rooms/4flags/4directions/2camera fixtures; %u actualAC4F/C1C6/8B71/8211 chains and %u independent OAMcells match production; %u bounded parser refusals; no liveescort/hardware timing claim\n",initializers,draws,total_cells,parser_refusals);
  printf("PASS follower provenance: %u initializer/call/table source-byte refusals, %u exhaustive flag and wrong-slot checks\n",source_refusals,provenance_checks);
  return 0;
}
