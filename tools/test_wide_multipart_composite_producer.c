/* Actual room40/41 loader graphics plus arranged source-policy records.
 * Full original C284 writes native OAM; no decoder output constructs anchors.
 * This does not certify natural boss states/AI or a complete playthrough. */
#define LAGOON_WIDE_MULTIPART_PRODUCER_NO_MAIN
#include "test_wide_multipart_producer.c"
static const char *room_prefix[2];
static unsigned composite_setups,large_groups,composite_refusals,mixed_orders,opaque_overlap;
static void composite_scalar(void){
 LagoonWideCell groups[3][128];int counts[3]={0,0,0};unsigned first=s_frame.ram[0xec4]==4?1:0;
 counts[first]=LagoonWideMultipart_PacketCells(g_rom,sizeof own_rom,s_frame.ram,first,groups[first],128);assert(counts[first]>0);
 if(first){counts[0]=LagoonWideEntities_ProjectilePacketCells(g_rom,sizeof own_rom,s_frame.ram,0,groups[0],128);assert(counts[0]>0);}
 if(s_has_far){counts[first+1]=LagoonWideEntities_RecordCells(g_rom,sizeof own_rom,s_frame.ram,0x80,0x3e,1,groups[first+1],128);assert(counts[first+1]>0);}
 for(unsigned j=0;j<p.extraObjectCount;++j){const PpuExtraObject *o=&p.extraObjects[j];unsigned tile,size=o->width;
  if(s_has_far&&o->order>=2){unsigned rank=(o->order-2)/128,index=(o->order-2)%128;
   assert(rank<3&&index<(unsigned)counts[rank]);const LagoonWideCell *c=&groups[rank][index];assert(c->x==o->x&&c->y==o->y&&c->size==size);tile=c->tile;
  }else{unsigned slot=o->oamSlot;tile=((uint8_t*)p.oam)[slot*4+2]|((uint8_t*)p.oam)[slot*4+3]<<8;}
  for(unsigned y=0;y<size;++y)for(unsigned x=0;x<size;++x){
   unsigned sx=tile&0x4000?size-1-x:x,sy=tile&0x8000?size-1-y:y;
   unsigned t=(((tile&255)/16+sy/8)*16)|(((tile&15)+sx/8)&15);
   unsigned at=((tile&256?0x5000:0x4000)+t*16+(sy&7))&32767,color=0;
   for(unsigned plane=0;plane<4;++plane)color|=((p.vram[(at+(plane/2)*8)&32767]>>((plane&1)*8+7-(sx&7)))&1)<<plane;
   unsigned expected=color?0x8000|p.cgram[128+((tile>>9)&7)*16+color]:0;int px=o->x+(int)x;
   if(px>=0&&px<256){expected=0;++center_zero;}
   if(s_has_far&&((o->order==1&&(px< -16||px>=272))||(o->order>=2&&px>=-16&&px<272)))expected=0;
   assert(o->pixels[y*o->stride+x]==expected);++scalar_pixels;
  }
 }
}
static void load_room(unsigned kind){
 baseline_root=room_prefix[kind];memset(&p,0,sizeof p);ppu_reset(&p);
 load_fixture("wram.bin",g_ram,sizeof g_ram);load_fixture("vram.bin",p.vram,sizeof p.vram);load_fixture("cgram.bin",p.cgram,sizeof p.cgram);
 assert(ram_word(g_ram,0x48e)==0x40+kind);p.inidisp=15;p.obsel=2;p.bgmode=9;
 p.bgXsc[0]=0x71;p.bgXsc[1]=g_ram[0x50]?0x7a:0x79;p.bgXsc[2]=0x3c;p.bgTileAdr=0x600;
 memset(g_ram+0x540,0,0x980);empty_oam();g_config.widescreen=1;
 LagoonWideEntities_Reset();LagoonWideEntities_Init();atlas();common_owner();PpuSetExtraSpace(&p,213);
}
static void composite_setup(unsigned kind,unsigned flags,unsigned frame,int x,int far_prop){
 load_room(kind);g_ram[0x540]=0xc8;g_ram[0x541]=flags;g_ram[0x558]=3;g_ram[0x55b]=kind;g_ram[0x55c]=frame;
 put(0x542,ram_word(g_ram,0x480)+x);put(0x544,ram_word(g_ram,0x482)+100);put(0x54c,x);put(0x54e,100);
 put(0x48c,1);put(0xec0,0x40);put(0xec2,100);put(0xec4,0xff03);
 g_ram[0x575]=4;g_ram[0x576]=5;g_ram[0x577]=6;
 if(!kind&&flags==0x21&&frame==6){g_ram[0x575]=6;g_ram[0x576]=6;g_ram[0x577]=255;}
 g_ram[0x57b]=0;g_ram[0x57c]=1;g_ram[0x57d]=2;g_ram[0x2a2]=255;
 LagoonWideCell cells[256];int n=LagoonWideMultipart_PacketCells(g_rom,sizeof own_rom,g_ram,0,cells,256);assert(n>=0);
 if(n>64)++large_groups;
 oracle_draw(0x40);
 if(far_prop){
  assert(!kind);unsigned a=0x580;g_ram[a]=0xc0;g_ram[a+24]=5;g_ram[a+27]=9;g_ram[a+28]=0x3e;
  put(a+2,ram_word(g_ram,0x480)-100);put(a+4,ram_word(g_ram,0x482)+100);put(a+12,-100);put(a+14,100);
 }
 finish();assert(s_multipart_packets&&s_multipart_matched);++composite_setups;
 assert(p.extraObjectCount>0);composite_scalar();
 if(far_prop)assert(s_has_far);
}
static void refuse_composite(void){LagoonWideEntities_BeginFrame(1);assert(!p.extraObjectCount);++composite_refusals;}
#ifndef LAGOON_WIDE_MULTIPART_COMPOSITE_NO_MAIN
int main(int argc,char **argv){
 assert(argc==4);room_prefix[0]=argv[2];room_prefix[1]=argv[3];FILE *f=fopen(argv[1],"rb");
 assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);
 for(unsigned kind=0;kind<2;++kind)for(unsigned edge=0;edge<2;++edge){
  composite_setup(kind,0,0,edge?255:0,0);
  composite_setup(kind,1,1,edge?255:0,0);assert(s_has_death);
 }
 /* Complete Samson main+secondary+three common layers exceeds the former
  *64cell bound. Refuse nothing partially and keep native anchors intact. */
 composite_setup(0,0x21,6,0,0);assert(large_groups);
 composite_setup(0,0x21,6,0,1);
 unsigned first_far=65535,last_boss=0;
 for(unsigned i=0;i<p.extraObjectCount;++i){const PpuExtraObject *o=&p.extraObjects[i];
  if(o->x< -80){if(o->order<first_far)first_far=o->order;}
  else if(o->order>last_boss)last_boss=o->order;
 }
 assert(first_far>=130&&last_boss<first_far);++mixed_orders;
 composite_setup(0,0x21,6,-64,1);
 for(unsigned a=0;a<p.extraObjectCount;++a)for(unsigned b=0;b<p.extraObjectCount;++b){
  const PpuExtraObject *one=&p.extraObjects[a],*two=&p.extraObjects[b];
  if(one->order<2||one->order>=130||two->order<130)continue;
  for(unsigned y=0;y<one->height;++y)for(unsigned x=0;x<one->width;++x){
   int tx=one->x+(int)x-two->x,ty=one->y+(int)y-two->y;
   if(tx>=0&&ty>=0&&tx<(int)two->width&&ty<(int)two->height&&
       one->pixels[y*one->stride+x]&&two->pixels[ty*two->stride+tx])++opaque_overlap;
  }
 }
 assert(opaque_overlap);++mixed_orders;
 /* Original BB05 ordering appends the projectile first at equalY. Execute
  * its original C41B and then the78cell original boss carrier to form every
  * native anchor; deep ranks are projectile0,boss1,farprop2. */
 composite_setup(0,0x21,6,0,1);empty_oam();projectile(0,11,0x12,280,100);
 put(0xec0,0);put(0xec2,100);put(0xec4,0xff04);put(0xec6,0x40);put(0xec8,100);put(0xeca,0xff03);put(0x48c,2);
 g_ram[0x2a2]=255;oracle_entry(0,0xc41b);oracle_draw(0x40);finish();assert(s_has_far&&s_has_projectile);composite_scalar();
 unsigned far_order=65535,projectile_order=65535,boss_order=65535;
 for(unsigned i=0;i<p.extraObjectCount;++i){const PpuExtraObject *o=&p.extraObjects[i];if(o->order<2)continue;
  if(o->x< -80&&o->order<far_order)far_order=o->order;
  else if(o->x>260&&o->order<projectile_order)projectile_order=o->order;
  else if(o->x<128&&o->order<boss_order)boss_order=o->order;
 }
 assert(projectile_order==2&&boss_order>=130&&far_order>=258);++mixed_orders;
 composite_setup(0,0,0,0,0);s_frame.ram[0x55c]=255;refuse_composite();
 composite_setup(0,4,0,0,0);s_frame.ram[0x575]=0;refuse_composite();
 composite_setup(0,1,0,0,0);s_death_atlas.ready=0;refuse_composite();
 composite_setup(1,0,0,0,0);s_frame.ram[0x55c]=128;refuse_composite();
 composite_setup(1,0,0,0,0);s_frame.ram[0xec0]=0x80;memcpy(s_frame.ram+0x580,s_frame.ram+0x540,64);refuse_composite();
 composite_setup(0,1,0,0,0);LagoonWideEntities_RetainScene();assert(LagoonWideEntities_RetainedSceneValid());
 p.vram[0x4300]^=1;assert(!LagoonWideEntities_RetainedSceneValid());++composite_refusals;
 composite_setup(0,0,0,0,0);observe_vram_byte(0x8800,((uint8_t*)p.vram)[0x8800]);assert(!p.extraObjectCount);++composite_refusals;
 composite_setup(0,0,0,0,0);++s_frame.generation;refuse_composite();
 composite_setup(1,0,0,0,0);p.oam[0]^=1;refuse_composite();
 composite_setup(0,0,0,0,0);g_ram[0x480]^=1;refuse_composite();
 printf("composite producer: %u synthetic original-C284 setups, %u >64cell whole groups, %u global-slot/resource/unknown-frame/OAM/generation/current/retained refusals; %u mixed128stride orders/%u opaque overlap pixels, %u independent native-OAM pixels/%u centerzero PASS. Actual loader graphics; synthetic poses remain separate from live boss coverage.\n",composite_setups,large_groups,composite_refusals,mixed_orders,opaque_overlap,scalar_pixels,center_zero);
 return 0;
}
#endif
