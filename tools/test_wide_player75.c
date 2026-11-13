/* Original player draw metadata and actual native OAM are the authority.
 * External private snapshots supply all art/state; no asset bytes embedded. */
#define LAGOON_WIDE_ACTOR_POLICY_NO_MAIN
#include "test_wide_additional_producer.c"
static int zoom=112;
int LagoonWide_ViewExtraY(void){return zoom;}
int LagoonWide_ViewExtraX(int extra){(void)extra;return 256;}
static uint8_t captured[0x20000],actual_oam[544];
static unsigned positives,player_refusals,player_pixels,below;
static void file(const char*prefix,const char*suffix,void*dst,size_t n){char path[4096];snprintf(path,sizeof path,"%s.%s",prefix,suffix);FILE*f=fopen(path,"rb");assert(f&&fread(dst,1,n,f)==n);fclose(f);}
static void reject_player(void){uint8_t used[128]={0},old[128]={0};LagoonWideCell c[64];int slots[64];assert(!player_packet_cells(captured,actual_oam,used,c,slots));assert(!memcmp(used,old,128));++player_refusals;}
int main(int argc,char**argv){assert(argc==3);FILE*f=fopen(argv[1],"rb");assert(f&&fread(own_rom,1,sizeof own_rom,f)==sizeof own_rom);fclose(f);file(argv[2],"wram.bin",captured,sizeof captured);file(argv[2],"oam.bin",actual_oam,sizeof actual_oam);file(argv[2],"vram.bin",p.vram,sizeof p.vram);file(argv[2],"cgram.bin",p.cgram,sizeof p.cgram);p.obsel=2;
 uint8_t used[128]={0};LagoonWideCell cells[64];int slots[64];int n=player_packet_cells(captured,actual_oam,used,cells,slots);printf("player decoded=%d room%02X frame%u xy=%u,%u\n",n,ram_word(captured,0x48e),captured[0x51c],ram_word(captured,0x50c),ram_word(captured,0x50e));assert(n>0);++positives;
 for(int i=0;i<n;i++){uint16_t reference[256];cell_pixels(p.vram,p.cgram,0x4000,0x5000,&cells[i],reference,0,1);s_object_count=0;int emitted=draw_cell_kind(&cells[i],slots[i],1,0,1);for(unsigned y=0;y<8;y++)for(unsigned x=0;x<8;x++){int sx=cells[i].x+x,sy=cells[i].y+y;unsigned expected=sx>=0&&sx<256&&sy>=0&&sy<190?0:reference[y*8+x];unsigned actual=emitted?s_objects[0].pixels[y*8+x]:0;assert(actual==expected);player_pixels++;if(sy>=190&&actual)below++;}}
 assert(below);printf("player lower opaque player_pixels=%u\n",below);
 unsigned saved_frame=captured[0x51c];captured[0x3ba]^=1;reject_player();captured[0x3ba]^=1;captured[0x51c]=70;reject_player();captured[0x51c]=saved_frame;
 unsigned slot=slots[0];actual_oam[slot*4]^=1;reject_player();actual_oam[slot*4]^=1;
 unsigned old=actual_oam[slot*4],hi=actual_oam[512+slot/4];actual_oam[slot*4]=64;actual_oam[512+slot/4]|=1u<<((slot%4)*2);reject_player();actual_oam[slot*4]=old;actual_oam[512+slot/4]=hi;
 own_rom[0xc0ca]^=1;reject_player();own_rom[0xc0ca]^=1;
 printf("PASS original player grid %u group, %u source/metadata/OAM/parked-X refusals, %u source-pixel comparisons incl%u below old HUD\n",positives,player_refusals,player_pixels,below);return 0;}
