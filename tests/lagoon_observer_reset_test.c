/* Actual observer clauses/state machines; only source identity is a fixture. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "lagoon_wide_entities.h"
#include "spell_state_under_test.h"
typedef struct State {LagoonWideMagicAtlas magic;LagoonWideDeathAtlas death;LagoonWideAtlas atlas;LagoonWideBackground background;SpellState spell;uint64_t clear_bytes;}State;
static State states[2],*state;
static uint64_t generation=1;
static bool source_valid=true;
static uint8_t ram[0x1000];
static uint64_t RtlStateGeneration(void){return generation;}
static void *counted_memset(void *p,int v,size_t n){state->clear_bytes+=n;return memset(p,v,n);}
int LagoonWideMagic_Source(const uint8_t *rom,size_t size,unsigned spell){(void)rom;(void)size;(void)spell;return source_valid;}
int LagoonWideDeath_Source(const uint8_t *rom,size_t size){(void)rom;(void)size;return source_valid;}
static int source_ok(const uint8_t *rom,size_t size){(void)rom;(void)size;return source_valid;}
static int scene_key(const uint8_t *rom,const uint8_t *r,uint8_t key[32]){(void)rom;(void)r;memset(key,0x42,32);return source_valid;}
#define s_magic_atlas state->magic
#define s_death_atlas state->death
#define s_atlas state->atlas
#define s_background state->background
#define s_spell state->spell
#define memset counted_memset
#include "observer_under_test.h"
#undef memset
static unsigned compared;
static void compare(void){
 State a=states[0],b=states[1];a.clear_bytes=b.clear_bytes=0;
#define ACK(field) do{if(a.field.generation!=b.field.generation){assert(a.field.generation==0&&b.field.generation==generation);b.field.generation=0;}}while(0)
 ACK(magic);ACK(death);ACK(atlas);ACK(background);
#undef ACK
 assert(!memcmp(&a,&b,sizeof a));compared++;
}
static void write_byte(unsigned address,uint8_t value){
 for(unsigned mode=0;mode<2;mode++){
  state=&states[mode];
  if(mode)optimized_spell_write(address,value);else reference_spell_write(address,value);
  if(mode)optimized_generation();else reference_generation();
  LagoonWideMagic_Write(&state->magic,address,value);LagoonWideDeath_Write(&state->death,address,value);
  LagoonWideAtlas_Write(&state->atlas,address);
  LagoonWideBackground_Write(&state->background,address,value);
 }
 compare();
}
static void reset_all(void){
 for(unsigned mode=0;mode<2;mode++){state=&states[mode];LagoonWideMagic_Reset(&state->magic);LagoonWideDeath_Reset(&state->death);LagoonWideAtlas_Reset(&state->atlas);LagoonWideBackground_Reset(&state->background);LagoonWideSpell3_Reset();}
 compare();
}
static void begin_atlas(unsigned bank,unsigned address,unsigned destination,unsigned length){
 for(unsigned mode=0;mode<2;mode++){state=&states[mode];int ok=LagoonWideAtlas_Begin(&state->atlas,generation,20,0x123456,bank,address,destination,length);assert(ok==(bank==127&&address==0x7f80&&destination==0x4400&&length==0x1000));}compare();
}
static void begin_spell(bool good){
 for(unsigned mode=0;mode<2;mode++){state=&states[mode];LagoonWideSpell3_Dma(NULL,0,ram,generation,1,127,0x7f80,0x3000,0x1000,good);}compare();
}
int main(void){
 unsigned cases=0;ram[0x537]=3;ram[0x417]=1;
 for(unsigned i=0;i<16384;i++)write_byte(i,(uint8_t)i);
 uint64_t old_clear=states[0].clear_bytes,new_clear=states[1].clear_bytes;
 assert(old_clear>UINT64_C(400000000)&&new_clear<40000);cases++;
 reset_all();for(unsigned i=0;i<32;i++)write_byte(0xf000+i,1);cases++;
 /* Successful load/rewind invalidates static owners before new writes. */
 generation=2;reset_all();for(unsigned i=0;i<64;i++)write_byte(0xb800+i,0xff);cases++;
 begin_atlas(127,0x7f80,0x4400,0x1000);for(unsigned i=0;i<64;i++)write_byte(0x8800+i,0);assert(states[1].atlas.completed==64&&states[1].atlas.transfer_left==4032);cases++;
 write_byte(0x9999,0);assert(!states[1].atlas.transfer_left&&!states[1].atlas.ready);cases++;
 begin_atlas(126,0x7f80,0x4400,0x1000);for(unsigned i=0;i<8;i++)write_byte(0x8800+i,0);cases++;
 /* A new generation revokes partial ownership; generation rollback does too. */
 generation=3;write_byte(0x9000,0);assert(!states[1].atlas.completed&&!states[1].atlas.ready);generation=2;write_byte(0x9001,0);cases++;
 for(unsigned mode=0;mode<2;mode++){state=&states[mode];assert(LagoonWideBackground_Begin(&state->background,generation,20,1,127,0x7f80,0,0x1000));assert(LagoonWideDeath_Begin(&state->death,generation,NULL,0,127,0x7f80,0x4300,512));assert(LagoonWideMagic_Begin(&state->magic,generation,20,NULL,0,13,127,0x7f80,0x5c00,2016));}compare();
 for(unsigned i=0;i<16;i++)write_byte(i,0);assert(!states[1].magic.left&&!states[1].death.left);cases++;
 source_valid=false;for(unsigned mode=0;mode<2;mode++){state=&states[mode];assert(!LagoonWideDeath_Begin(&state->death,generation,NULL,0,127,0x7f80,0x4300,512));assert(!LagoonWideMagic_Begin(&state->magic,generation,20,NULL,0,13,127,0x7f80,0x5c00,2016));}compare();write_byte(0xb800,1);source_valid=true;cases++;
 begin_spell(true);for(unsigned i=0;i<128;i++)write_byte(0x6000+i,(uint8_t)i);assert(states[1].spell.left==3968);write_byte(0x6500,0);assert(!states[1].spell.generation&&!states[1].spell.kind);for(unsigned i=0;i<64;i++)write_byte(0x6000+i,0);cases++;
 begin_spell(false);for(unsigned i=0;i<128;i++)write_byte(0x6000+i,0);cases++;
 begin_spell(true);for(unsigned i=0;i<4096;i++)write_byte(0x6000+i,0);assert(states[1].spell.stage==1);for(unsigned i=0;i<64;i++)write_byte(0xe000+i,0);assert(states[1].spell.stage==1);write_byte(0x6000,0);assert(states[1].spell.stage==0);cases++;
 reset_all();for(unsigned mode=0;mode<2;mode++){state=&states[mode];State z={0},copy=*state;copy.clear_bytes=0;assert(!memcmp(&copy,&z,sizeof z));}cases++;
 printf("PASS actual observer old/new: %u scenarios, %u state comparisons; cold16384 writes clear %llu -> %llu bytes; shared write/Begin/reset semantics unchanged; only empty generation acknowledgment differs\n",cases,compared,(unsigned long long)old_clear,(unsigned long long)new_clear);return 0;
}
