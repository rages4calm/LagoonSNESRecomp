#include "lagoon_wide_entities.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t rom[0x100000];
static void put(unsigned a,unsigned v) {rom[((a>>16)&127)*32768+(a&32767)]=(uint8_t)v;}
static void tables(void) {
  put(0x1f885+41*2,0);put(0x1f886+41*2,0xf0);put(0x1f000,3);
  put(0x1f41a,2);
  for(unsigned i=0;i<16;++i){put(0x1f001+i,40+i);put(0x1f41b+i,10+i);}
}
static void animation(void) {
  uint8_t r[64]={0},frame=0;tables();r[0]=0xe0;r[0x18]=1;r[0x1b]=41;r[10]=3;
  LagoonWideAnimation a={2,2};
  assert(LagoonWideEntities_CommonFrame(rom,sizeof(rom),r,&a,0,&frame)&&frame==54&&a.timer==2);
  assert(LagoonWideEntities_CommonFrame(rom,sizeof(rom),r,&a,1,&frame)&&frame==55&&a.timer==0&&a.phase==3);
  a.timer=2;assert(LagoonWideEntities_CommonFrame(rom,sizeof(rom),r,&a,1,&frame)&&frame==52&&a.phase==0);
  r[0]=0xc1;a.timer=2;a.phase=3;
  assert(LagoonWideEntities_CommonFrame(rom,sizeof(rom),r,&a,1,&frame)&&frame==52&&a.phase==0&&a.timer==0);
  r[0]=0xc0;r[11]=0x80;a.phase=2;
  assert(LagoonWideEntities_CommonFrame(rom,sizeof(rom),r,&a,0,&frame)&&frame==52&&a.phase==0);
  r[0]=0xe0;r[11]=0;r[0x18]=2;r[0x1b]=0;r[0x2c]=2;r[10]=1;a.timer=1;a.phase=2;
  assert(LagoonWideEntities_CommonFrame(rom,sizeof(rom),r,&a,1,&frame)&&frame==49&&a.phase==3);
  r[1]=1;assert(!LagoonWideEntities_CommonFrame(rom,sizeof(rom),r,&a,0,&frame));
  r[1]=0;r[0x1b]=12;assert(!LagoonWideEntities_CommonFrame(rom,sizeof(rom),r,&a,0,&frame));
  r[0x18]=1;r[0x1b]=2;assert(!LagoonWideEntities_CommonFrame(rom,sizeof(rom),r,&a,0,&frame));
  r[0x1b]=41;r[10]=4;assert(!LagoonWideEntities_CommonFrame(rom,sizeof(rom),r,&a,0,&frame));
  r[10]=0;assert(!LagoonWideEntities_CommonFrame(rom,16,r,&a,0,&frame));
}
static void load_chunk(LagoonWideAtlas *a,unsigned n) {
  unsigned len=n==0x2000?0xa00:0x1000;
  assert(LagoonWideAtlas_Begin(a,7,3,0x118000,0x7f,0x7f80+n,0x4400+n/2,len));
  for(unsigned b=0;b<len;++b)LagoonWideAtlas_Write(a,0x8800+n+b);
}
static void ownership(void) {
  LagoonWideAtlas a;LagoonWideAtlas_Reset(&a);
  LagoonWideCell c={400,10,0x2640,16};
  load_chunk(&a,0);assert(!a.ready&&!LagoonWideAtlas_CellOwned(&a,0x4000,0x5000,&c));
  assert(!LagoonWideAtlas_Begin(&a,7,3,0x118000,0x7f,0x9f80,0x5400,0xa00));
  load_chunk(&a,0x1000);load_chunk(&a,0x2000);
  assert(a.ready&&a.completed==0x2a00&&LagoonWideAtlas_CellOwned(&a,0x4000,0x5000,&c));
  LagoonWideAtlas_Write(&a,0x8803);
  assert(!LagoonWideAtlas_CellOwned(&a,0x4000,0x5000,&c));
  c.tile=0x2650;assert(LagoonWideAtlas_CellOwned(&a,0x4000,0x5000,&c));
  c.tile=0x2601;assert(!LagoonWideAtlas_CellOwned(&a,0x4000,0x5000,&c));
  assert(!LagoonWideAtlas_Begin(&a,8,3,0x118000,0x7f,0x8f80,0x4c00,0x1000));
  LagoonWideAtlas_Reset(&a);load_chunk(&a,0);
  assert(!LagoonWideAtlas_Begin(&a,7,4,0x118000,0x7f,0x8f80,0x4c00,0x1000));
  LagoonWideAtlas_Write(&a,0x9800);assert(!a.ready);
  LagoonWideAtlas_Reset(&a);assert(!a.ready&&!a.completed);
}
static void modal(void) {
  static uint16_t vram[32768],depth[800];
  for(unsigned i=0;i<1024;++i)vram[0x3c00+i]=511;
  assert(!LagoonWideEntities_MenuOverlay(vram,0x3c00,0x6000));
  vram[0x3c00+24*32]=1;vram[0x6008]=1; /* ordinary bottom HUD */
  assert(!LagoonWideEntities_MenuOverlay(vram,0x3c00,0x6000));
  vram[0x3c00+2*32+2]=1;
  assert(LagoonWideEntities_MenuOverlay(vram,0x3c00,0x6000));
  vram[0x3c00+2*32+2]=511;
  assert(!LagoonWideEntities_MenuOverlay(vram,0x3c00,0x6000));
  for(unsigned i=0;i<800;++i)depth[i]=0x9999;
  LagoonWideEntities_BlankMargins(depth,272,213,213);
  for(unsigned i=0;i<800;++i)assert(depth[i]==((i>=59&&i<272)||(i>=528&&i<741)?0x0500:0x9999));
}
static void room_bounds(void) {
  uint8_t pixels[344*4];
  for(int world_x=-4;world_x<=260;world_x+=132)for(int world_y=-1;world_y<=512;world_y+=513) {
    memset(pixels,0x91,sizeof(pixels));
    LagoonWideEntities_MaskRoomRow(pixels,342,world_x,world_y,512,512);
    for(unsigned x=0;x<344;++x)for(unsigned b=0;b<4;++b) {
      int sx=(int)x-43;
      int clipped=x<342 && (sx<0 || sx>=256) &&
          (world_y<0 || world_y>=512 || world_x+sx<0 || world_x+sx>=512);
      assert(pixels[x*4+b]==(clipped?0:0x91));
    }
  }
  memset(pixels,0x91,sizeof(pixels));
  LagoonWideEntities_MaskRoomRow(pixels,342,0,511,512,512);
  assert(pixels[0]==0 && pixels[43*4]==0x91 && pixels[341*4]==0x91);
  LagoonWideEntities_MaskRoomRow(pixels,342,256,511,512,512);
  assert(pixels[299*4]==0 && pixels[298*4]==0x91 && pixels[342*4]==0x91);
}
static void scene_settlement(void) {
  LagoonWideSceneGate g={0};
  assert(!LagoonWideEntities_SceneSettled(&g,1,1,7));
  assert(!LagoonWideEntities_SceneSettled(&g,1,1,7));
  assert(LagoonWideEntities_SceneSettled(&g,1,1,7));
  assert(!LagoonWideEntities_SceneSettled(&g,1,4,7));
  assert(!LagoonWideEntities_SceneSettled(&g,1,4,7));
  assert(LagoonWideEntities_SceneSettled(&g,1,4,7));
  assert(!LagoonWideEntities_SceneSettled(&g,0,4,7));
  assert(!LagoonWideEntities_SceneSettled(&g,1,4,7));
  assert(!LagoonWideEntities_SceneSettled(&g,1,4,8));
  assert(!LagoonWideEntities_SceneSettled(&g,1,4,8));
  assert(LagoonWideEntities_SceneSettled(&g,1,4,8));
}
static void field_fallback(void) {
  uint8_t pixels[3*344*4];memset(pixels,0x91,sizeof(pixels));
  LagoonWideEntities_BlankField(pixels,344*4,342,2);
  for(unsigned y=0;y<3;++y)for(unsigned x=0;x<344*4;++x)
    assert(pixels[y*344*4+x]==((y<2 && (x<43*4 || (x>=299*4 && x<342*4)))?0:0x91));
}
static void audited_rooms(void) {
  assert(LagoonWideEntities_BoundedRoom(4,768,256));
  assert(LagoonWideEntities_BoundedRoom(5,1536,1280));
  assert(LagoonWideEntities_BoundedRoom(6,512,256));
  assert(!LagoonWideEntities_BoundedRoom(4,768,512));
  for(unsigned room=4;room<=6;++room)
    assert(LagoonWideEntities_AuditedRoom(room,0x128b77,0x1d8000,0x069298,0x069298,0x7f0000,0x7f4000));
  assert(!LagoonWideEntities_AuditedRoom(7,0x128b77,0x1d8000,0x069298,0x069298,0x7f0000,0x7f4000));
  assert(LagoonWideEntities_AuditedRoom(0,0x138000,0x198000,0x068000,0x068000,0x7f0000,0x7f4000));
  assert(LagoonWideEntities_AuditedRoom(2,0x128000,0x1a8000,0x068e90,0x068e90,0x7f0000,0x7f4000));
  assert(LagoonWideEntities_AuditedRoom(3,0x128b77,0x1d8000,0x069298,0x069298,0x7f0000,0x7f4000));
  assert(!LagoonWideEntities_AuditedRoom(2,0x138000,0x198000,0x068000,0x068000,0x7f0000,0x7f4000));
  assert(!LagoonWideEntities_AuditedRoom(0,0x128b77,0x198000,0x068000,0x068000,0x7f0000,0x7f4000));
  assert(!LagoonWideEntities_AuditedRoom(0,0x138000,0x198000,0x068000,0x069298,0x7f0000,0x7f4000));
  assert(!LagoonWideEntities_AuditedRoom(3,0x128b77,0x1d8000,0x069298,0x069298,0x7f0000,0x7f8000));
}
static void world_tilemap(void) {
  static uint16_t vram[32768],witness[16384];uint8_t used[1024]={0};
  uint16_t expected[720];used[7]=1;
  assert(LagoonWideEntities_WorldCharacters(vram,witness,used));
  vram[0x3C00]=123; /* BG3 map aliases unowned character960. */
  assert(LagoonWideEntities_WorldCharacters(vram,witness,used));
  vram[7*16+15]=1;
  assert(!LagoonWideEntities_WorldCharacters(vram,witness,used));
  vram[7*16+15]=0;
  int tx=29,ty=29; /* cross both horizontal/vertical circular seams */
  for(unsigned y=0;y<24;++y)for(unsigned x=0;x<30;++x) {
    unsigned a=0x7000+((((unsigned)tx+x)&32)<<5)+((((unsigned)ty+y)&31)*32)+(((unsigned)tx+x)&31);
    expected[y*30+x]=(uint16_t)(1+x+y*30);vram[a]=expected[y*30+x];
  }
  assert(LagoonWideEntities_WorldTilemap(vram,0x7000,tx,ty,expected));
  vram[0x7000+((((unsigned)tx)&32)<<5)+(((unsigned)ty&31)*32)+((unsigned)tx&31)]^=1;
  assert(!LagoonWideEntities_WorldTilemap(vram,0x7000,tx,ty,expected));
  assert(!LagoonWideEntities_WorldTilemap(NULL,0x7000,tx,ty,expected));
  assert(!LagoonWideEntities_WorldTilemap(vram,0x7000,-1,ty,expected));
}
static void background_transfer(void) {
  LagoonWideBackground b;LagoonWideBackground_Reset(&b);
  assert(!b.ready && !b.completed);
  assert(!LagoonWideBackground_Begin(&b,1,0,0x138000,0x7F,0x8F80,0x800,0x1000));
  for(unsigned done=0;done<kLagoonWideBackgroundBytes;) {
    unsigned length=kLagoonWideBackgroundBytes-done;if(length>0x1000)length=0x1000;
    assert(LagoonWideBackground_Begin(&b,1,0,0x138000,0x7F,0x7F80+done,done/2,length));
    assert(!b.ready);
    for(unsigned i=0;i<length;++i) {
      LagoonWideBackground_Write(&b,done+i,(uint8_t)((done+i)^0x93));
      assert(b.ready==((done+i+1)==kLagoonWideBackgroundBytes));
    }
    done+=length;
  }
  for(unsigned i=0;i<kLagoonWideBackgroundBytes;++i)assert(b.bytes[i]==(uint8_t)(i^0x93));
  LagoonWideBackground_Write(&b,0xC000,7);assert(b.ready); /* unrelated font */
  LagoonWideBackground_Write(&b,12,8);assert(!b.ready); /* unknown overwrite */
  assert(LagoonWideBackground_Begin(&b,2,3,0x128B77,0x7F,0x7F80,0,0x1000));
  LagoonWideBackground_Write(&b,0,5);assert(b.completed==1 && !b.ready);
  LagoonWideBackground_Write(&b,2,6);assert(!b.left && !b.ready); /* missing byte */
  assert(!LagoonWideBackground_Begin(&b,3,3,0x128B77,0x7F,0x7F81,0,0x1000));
  assert(LagoonWideBackground_Begin(&b,3,4,0x128B77,0x7F,0x7F80,0,0x1000));
  assert(!LagoonWideBackground_Begin(&b,3,5,0x128B77,0x7F,0x7F80,0,0x800));
  LagoonWideBackground_Reset(&b);assert(!b.ready && !b.completed && !b.generation);
}
static void torch_transfer(void) {
  LagoonWideTorches t;LagoonWideTorches_Reset(&t);
  static uint16_t vram[32768];
  put(0x1DFF0,4);put(0x1DFF1,3);put(0x1DFF2,0x6B);put(0x1DFF3,0xDA);
  put(0x1DFF4,0);put(0x1DFF5,0x3B);put(0x1DFF6,0xC0);put(0x1DFF7,0);
  for(unsigned frame=0;frame<3;++frame) {
    unsigned a=0x8900+frame*0xC0;
    put(0x1DA6B+frame*3,a);put(0x1DA6C+frame*3,a>>8);put(0x1DA6D+frame*3,0x10);
    for(unsigned i=0;i<0xC0;++i)put(0x100000+a+i,(i^0x5C)+frame);
  }
  assert(LagoonWideTorches_Match(&t,vram,1,4,0x128B77)<0);
  for(unsigned frame=0;frame<3;++frame) {
    assert(LagoonWideTorches_Begin(&t,1,4,0x128B77,0x10,0x8900+frame*0xC0,0x3B00,0xC0));
    for(unsigned i=0;i<0xC0;++i) {
      assert(LagoonWideTorches_Match(&t,vram,1,4,0x128B77)<0);
      LagoonWideTorches_Write(&t,0x7600+i,(uint8_t)((i^0x5C)+frame),rom,sizeof(rom));
    }
    assert(t.seen==((1u<<(frame+1))-1));
    memcpy((uint8_t*)vram+0x7600,t.bytes[frame],0xC0);
    assert(LagoonWideTorches_Match(&t,vram,1,4,0x128B77)==(int)frame);
  }
  /* Render snapshot may still name any actually completed prior phase. */
  memcpy((uint8_t*)vram+0x7600,t.bytes[0],0xC0);
  assert(LagoonWideTorches_Match(&t,vram,1,4,0x128B77)==0);
  assert(LagoonWideTorches_Match(&t,vram,2,4,0x128B77)<0);
  assert(LagoonWideTorches_Match(&t,vram,1,5,0x128B77)<0);
  assert(LagoonWideTorches_Match(&t,vram,1,4,0x138000)<0);
  assert(!LagoonWideTorches_Begin(&t,1,4,0x128B77,0x10,0x8900,0x3B00,0xBF));
  LagoonWideTorches_Write(&t,0x8800,0,rom,sizeof(rom));assert(t.seen==7);
  LagoonWideTorches_Write(&t,0x7600,0,rom,sizeof(rom));assert(!t.seen);
  assert(LagoonWideTorches_Begin(&t,1,4,0x128B77,0x10,0x8900,0x3B00,0xC0));
  LagoonWideTorches_Write(&t,0x7601,0,rom,sizeof(rom));assert(!t.left&&!t.seen);
  assert(LagoonWideTorches_Begin(&t,1,4,0x128B77,0x10,0x8900,0x3B00,0xC0));
  for(unsigned i=0;i<0xC0;++i)LagoonWideTorches_Write(&t,0x7600+i,0,rom,sizeof(rom));
  assert(!t.seen); /* source mismatch cannot authorize an invented frame */
  assert(LagoonWideTorches_Begin(&t,2,5,0x128B77,0x10,0x8900,0x3B00,0xC0));
  for(unsigned i=0;i<0xC0;++i)LagoonWideTorches_Write(&t,0x7600+i,(uint8_t)(i^0x5C),rom,sizeof(rom));
  assert(t.seen==1);
  put(0x1DA6D,0x11); /* Changed source table cannot borrow audited graphics. */
  assert(LagoonWideTorches_Begin(&t,2,5,0x128B77,0x10,0x8900,0x3B00,0xC0));
  for(unsigned i=0;i<0xC0;++i)LagoonWideTorches_Write(&t,0x7600+i,(uint8_t)(i^0x5C),rom,sizeof(rom));
  assert(!t.seen);put(0x1DA6D,0x10);
  assert(!LagoonWideTorches_Begin(&t,2,7,0x128B77,0x10,0x8900,0x3B00,0xC0)&&!t.seen);
  LagoonWideTorches_Reset(&t);assert(!t.seen&&!t.generation);
}
int main(void){torch_transfer();background_transfer();world_tilemap();animation();ownership();modal();audited_rooms();field_fallback();scene_settlement();room_bounds();puts("wide actor state: pure post-frame/step, idle/variant/refusal, segmented atlas, partial/wrong-room/overwrite/reset PASS");return 0;}
