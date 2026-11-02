/* Owner-ROM offline fixtures: actual descriptors/pages, synthetic BG graphics.
 * No campaign/GPU claim. All archive files remain in the private fresh output. */
#define LAGOON_SCENERY_SOURCE_TEST
#define LAGOON_SCENERY72_ENTRY scenery_previous_main
#include "lagoon_scene_map72_test.c"
static unsigned archive_pixels,archive_refusals;
static void path_join(char *out,size_t n,const char *dir,const char *name){assert(snprintf(out,n,"%s/%s",dir,name)<(int)n);}
static void file_write(const char *path,const uint8_t *p,size_t n){FILE*f=archive_open(path,"wb");assert(f&&fwrite(p,1,n,f)==n&&!fclose(f));}
static uint8_t *file_read(const char *path,size_t *n){FILE*f=archive_open(path,"rb");assert(f&&!fseek(f,0,SEEK_END));long size=ftell(f);assert(size>=0&&!fseek(f,0,SEEK_SET));uint8_t*p=malloc((size_t)size+1);assert(p&&fread(p,1,(size_t)size,f)==(size_t)size&&!fclose(f));*n=(size_t)size;return p;}
static void seed_room(unsigned room,uint64_t generation){
  setup();memcpy(rom,scenery_loader_rom,sizeof rom);memcpy(ram+0x50,rom+off(0x05b2df+room*56),56);ram[0x48e]=room;
  memset(ram+0x480,0,8);p.hScroll[0]=p.hScroll[1]=p.vScroll[0]=p.vScroll[1]=0;
  ram[0x500]=0xc0;ram[0x520]=18;p.bgXsc[1]=ram[0x50]?0x7a:0x79;
  unsigned rng=room+197;
  for(unsigned i=0;i<16384;++i)p.vram[i]=random_word(&rng);
  for(unsigned i=0;i<128;++i)p.cgram[i]=random_word(&rng)&32767;
  for(unsigned i=0;i<16384;++i){ram[0x10000+i]=(uint8_t)(i*3+room);ram[0x14000+i]=(uint8_t)(i*7+room);}
  if(ram[0x50]){unsigned index=7;for(unsigned i=0;i<7;++i)if(rooms[i]==room)index=i;assert(index<7);memcpy(p.vram+0x7800,original_pages[index],4096);}
  LagoonSceneRenderer_Reset();assert(supported(&p,ram,rom,sizeof rom,LAGOON_SCENE_WORLD));capture(generation);
  assert(s_packets[s_complete].valid&&s_map_source.room==room);
}
static uint32_t archived_image(unsigned room){
  const uint32_t *image=NULL;unsigned w=0,h=0,ww=0,wh=0;
  uint32_t r=bytes_hash(ram,sizeof ram),rp=bytes_hash(rom,sizeof rom),pp=bytes_hash(&p,sizeof p);
  uint32_t packet=bytes_hash(&s_packets[s_complete],sizeof(ScenePacket));LagoonSceneRendererStats before=s_stats;
  assert(LagoonSceneRenderer_VisitedMapImage(room,&image,&w,&h,&ww,&wh)&&image&&w&&h&&w<=1024&&h<=1024);
  assert(ww==s_archive_contract[room].width&&wh==s_archive_contract[room].height);
  uint32_t sum=crc32_compute((const uint8_t *)image,(size_t)w*h*4);archive_pixels+=w*h;
  assert(r==bytes_hash(ram,sizeof ram)&&rp==bytes_hash(rom,sizeof rom)&&pp==bytes_hash(&p,sizeof p)&&
    packet==bytes_hash(&s_packets[s_complete],sizeof(ScenePacket))&&!memcmp(&before,&s_stats,sizeof before));return sum;
}
static void repair_checksums(uint8_t *data,size_t size,unsigned room){
  uint8_t *record=data+ARCHIVE_HEADER+(size_t)room*ARCHIVE_RECORD;
  archive_put32(record+ARCHIVE_RECORD-4,crc32_compute(record,ARCHIVE_RECORD-4));
  archive_put32(data+64,crc32_compute(data+ARCHIVE_HEADER,size-ARCHIVE_HEADER));archive_put32(data+76,crc32_compute(data,76));
}
static void rejected(const char *path,const uint8_t *bad,size_t size){
  file_write(path,bad,size);uint8_t before[32],after[32];sha256_compute(bad,size,before);
  assert(LagoonSceneRenderer_ArchiveOnRomInit(scenery_loader_rom,sizeof scenery_loader_rom));seed_room(3,900);
  assert(LagoonSceneRenderer_ArchiveCommit(3,900)&&LagoonSceneRenderer_ArchivedRoomCount()==1);
  assert(LagoonSceneRenderer_ArchiveLoad(path)==-1&&LagoonSceneRenderer_ArchivePersistenceBlocked());
  assert(LagoonSceneRenderer_ArchivedRoomCount()==1&&LagoonSceneRenderer_ArchiveSave(path)==-1);
  size_t bytes;uint8_t *current=file_read(path,&bytes);sha256_compute(current,bytes,after);free(current);
  assert(bytes==size&&!memcmp(before,after,32));++archive_refusals;
}
int main(int argc,char **argv){
  assert(argc==3);owned_parallax(argv[1]);
  /* Explicit UTF8 leaf avoids Windows' legacy narrow-main argv codepage. */
  char path[1024],bad_path[1024],temporary[1040];path_join(path,sizeof path,argv[2],"visited-\xce\xb2.maps");path_join(bad_path,sizeof bad_path,argv[2],"corrupt.maps");
  assert(LagoonSceneRenderer_ArchiveOnRomInit(scenery_loader_rom,sizeof scenery_loader_rom));
  assert(!LagoonSceneRenderer_ArchivedRoomCount()&&!LagoonSceneRenderer_ArchivePersistenceBlocked());
  seed_room(0,100);assert(!LagoonSceneRenderer_ArchivedRoomCount()); /* EndFrame alone is not committed authority. */
  assert(!LagoonSceneRenderer_ArchiveCommit(1,100)&&!LagoonSceneRenderer_ArchiveCommit(0,101));
  assert(LagoonSceneRenderer_ArchiveCommit(0,100));unsigned revision=s_archive_revision;
  assert(LagoonSceneRenderer_ArchiveCommit(0,100)&&s_archive_revision==revision);
  p.hScroll[1]++;capture(100);assert(!s_packets[s_complete].valid&&!LagoonSceneRenderer_ArchiveCommit(0,100));
  assert(LagoonSceneRenderer_ArchiveOnRomInit(scenery_loader_rom,sizeof scenery_loader_rom));
  uint32_t expected[75];
  for(unsigned room=0;room<75;++room){
    seed_room(room,100+room);assert(LagoonSceneRenderer_ArchiveCommit(room,100+room));
    assert(LagoonSceneRenderer_ArchivedRoomCount()==room+1);
    const uint32_t *live,*old;unsigned w,h,ow,oh;
    assert(LagoonSceneRenderer_MapImage(room,100+room,&live,&w,&h,NULL,NULL));
    assert(LagoonSceneRenderer_VisitedMapImage(room,&old,&ow,&oh,NULL,NULL)&&w==ow&&h==oh&&!memcmp(live,old,(size_t)w*h*4));
    expected[room]=archived_image(room);
  }
  LagoonSceneRenderer_Reset();assert(LagoonSceneRenderer_ArchivedRoomCount()==75&&archived_image(0)==expected[0]);
  const uint32_t *none=(const uint32_t*)1;unsigned nw=1,nh=1;
  assert(!LagoonSceneRenderer_VisitedMapImage(75,&none,&nw,&nh,NULL,NULL)&&!none&&!nw&&!nh);
  assert(LagoonSceneRenderer_ArchiveSave(path)==1&&LagoonSceneRenderer_ArchiveSave(path)==0);
  size_t size;uint8_t *golden=file_read(path,&size);assert(size==ARCHIVE_MAX&&size<6*1024*1024);
  assert(!memcmp(golden,"LGSCN001",8)&&archive_u32(golden+56)==75&&archive_u32(golden+52)==74024);
  assert(golden[ARCHIVE_HEADER+36+32768+4096]==(s_archive[0].patterns[0]&255));
  assert(golden[ARCHIVE_HEADER+36+32768+4096+1]==(s_archive[0].patterns[0]>>8));
  assert(LagoonSceneRenderer_ArchiveOnRomInit(scenery_loader_rom,sizeof scenery_loader_rom)&&LagoonSceneRenderer_ArchiveLoad(path)==1);
  assert(LagoonSceneRenderer_ArchivedRoomCount()==75);
  for(unsigned room=0;room<75;++room)assert(archived_image(room)==expected[room]);
  /* A fresh committed view wins over disk lazily loaded afterwards. */
  assert(LagoonSceneRenderer_ArchiveOnRomInit(scenery_loader_rom,sizeof scenery_loader_rom));seed_room(0,700);p.cgram[0]^=1;capture(700);
  assert(LagoonSceneRenderer_ArchiveCommit(0,700));uint32_t updated=archived_image(0);assert(updated!=expected[0]);
  assert(LagoonSceneRenderer_ArchiveLoad(path)==1&&LagoonSceneRenderer_ArchivedRoomCount()==75&&archived_image(0)==updated);
  assert(LagoonSceneRenderer_ArchiveSave(path)==1);
  assert(LagoonSceneRenderer_ArchiveOnRomInit(scenery_loader_rom,sizeof scenery_loader_rom)&&LagoonSceneRenderer_ArchiveLoad(path)==1&&archived_image(0)==updated);
  /* Restore only this test's known fixture; no owner input file is touched. */
  file_write(path,golden,size);
  uint8_t *bad=malloc(size+1);assert(bad);
  const unsigned offsets[]={0,8,12,16,48,52,56,60,64,68,72,76,ARCHIVE_HEADER+400,ARCHIVE_HEADER+ARCHIVE_RECORD+100,size-1};
  for(unsigned i=0;i<sizeof offsets/sizeof*offsets;++i){memcpy(bad,golden,size);bad[offsets[i]]^=1;rejected(bad_path,bad,size);}
  const size_t lengths[]={0,1,79,80,81,ARCHIVE_HEADER+ARCHIVE_RECORD-1,ARCHIVE_MAX-1};
  for(unsigned i=0;i<sizeof lengths/sizeof*lengths;++i)rejected(bad_path,golden,lengths[i]);
  memcpy(bad,golden,size);bad[size]=0;rejected(bad_path,bad,size+1);
  for(unsigned kind=0;kind<14;++kind){
    memcpy(bad,golden,size);uint8_t *record=bad+ARCHIVE_HEADER;
    switch(kind){
      case 0:archive_put32(record,75);break;case 1:archive_put32(record+4,1);break;
      case 2:archive_put32(record+8,2049);break;case 3:archive_put32(record+12,0);break;
      case 4:archive_put32(record+16,1);break;case 5:record[36+32768]^=1;break;
      case 6:archive_put32(record+32,1);break;
      case 7:archive_put32(bad+ARCHIVE_HEADER+ARCHIVE_RECORD,0);repair_checksums(bad,size,1);break;
      case 8:bad[ARCHIVE_HEADER+7*ARCHIVE_RECORD+36+32768+4096+32768]^=1;repair_checksums(bad,size,7);break;
      case 9:bad[16]^=1;break; /* foreignROM with otherwise correct checksums */
      case 10:archive_put32(bad+56,76);archive_put32(bad+60,76*ARCHIVE_RECORD);break;
      case 11:record[36+32768+4096+32768+4096+1]^=0x80;break;
      case 12:archive_put32(bad+ARCHIVE_HEADER+7*ARCHIVE_RECORD+16,4);repair_checksums(bad,size,7);break;
      case 13:bad[ARCHIVE_HEADER+74*ARCHIVE_RECORD+36+32768]^=1;repair_checksums(bad,size,74);break;
    }
    repair_checksums(bad,size,0);rejected(bad_path,bad,size);
  }
  /* Dirty save never replaces a file externally changed after lazy load. */
  file_write(path,golden,size);assert(LagoonSceneRenderer_ArchiveOnRomInit(scenery_loader_rom,sizeof scenery_loader_rom)&&LagoonSceneRenderer_ArchiveLoad(path)==1);
  seed_room(0,800);p.cgram[0]^=2;capture(800);assert(LagoonSceneRenderer_ArchiveCommit(0,800));
  file_write(path,(const uint8_t *)"foreign",7);assert(LagoonSceneRenderer_ArchiveSave(path)==-1&&LagoonSceneRenderer_ArchivePersistenceBlocked());
  size_t n;uint8_t *remaining=file_read(path,&n);assert(n==7&&!memcmp(remaining,"foreign",7));free(remaining);
  /* An existing temp file is not ours to truncate or remove. */
  file_write(path,golden,size);assert(LagoonSceneRenderer_ArchiveOnRomInit(scenery_loader_rom,sizeof scenery_loader_rom)&&LagoonSceneRenderer_ArchiveLoad(path)==1);
  seed_room(0,801);p.cgram[0]^=2;capture(801);assert(LagoonSceneRenderer_ArchiveCommit(0,801));
  assert(snprintf(temporary,sizeof temporary,"%s.tmp",path)<(int)sizeof temporary);file_write(temporary,(const uint8_t *)"keep",4);
  assert(LagoonSceneRenderer_ArchiveSave(path)==-1);remaining=file_read(temporary,&n);assert(n==4&&!memcmp(remaining,"keep",4));free(remaining);archive_remove(temporary);
  scenery_loader_rom[0]^=1;assert(!LagoonSceneRenderer_ArchiveOnRomInit(scenery_loader_rom,sizeof scenery_loader_rom));scenery_loader_rom[0]^=1;
  assert(!LagoonSceneRenderer_ArchivedRoomCount()&&!LagoonSceneRenderer_VisitedMapImage(0,&none,&nw,&nh,NULL,NULL));
  file_write(path,golden,size);free(bad);free(golden);
  printf("PASS visited scenery:75 actual descriptor/page fixtures, %u archived pixels, %u corrupt/truncation/resource refusals; exact fresh/offline images, committed-only capture, generation persistence, lazy merge/latestseen, boundedLECRC format, preserved foreign/concurrent/temp files, no guest/live-packet writes\n",archive_pixels,archive_refusals);
  return 0;
}
