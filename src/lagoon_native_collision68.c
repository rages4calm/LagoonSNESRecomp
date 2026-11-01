/* Original collision parent and directional probe scalars. Existing sampler,
 * dispatch jumps, child calls and returns remain original instructions. */
#include "lagoon_native_collision68.h"
#include "snes/interp_bridge.h"
#include <string.h>
#include <stdlib.h>

typedef struct {const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,x,armed,fresh;uint8_t frame[2];} CollisionOwner68;
static CollisionOwner68 collision68;
static struct {const CpuState*cpu;const uint8_t*ram;uint64_t gen;unsigned s,x,armed;} pending68;
static struct {uint64_t entries,parent,probes,deadlines,refused;} collision_stats68;
static unsigned extra68;
int LagoonCollision68_Source(const uint8_t*r,size_t n){if(!r||n!=0x100000||memcmp(r+0x80c1,"\x20\xce\x9b",3))return 0;uint64_t h=UINT64_C(14695981039346656037);for(unsigned j=0x9bce;j<0xa01a;j++)h=(h^r[j])*UINT64_C(1099511628211);return h==UINT64_C(0xc463c68b4606f575);}
static int collision_mode68(const CpuState*c){return c&&c->ram&&c->PB==1&&c->DB==1&&!c->D&&!c->emulation&&!c->x_flag&&!c->_flag_D&&c->_flag_I&&(c->P&0x1c)==4&&!!(c->P&32)==!!c->m_flag;}
/* pc, instruction bytes, base cycles, M, exact saved-register depth, opcode.
 * Source pointer/coordinate tables are fingerprinted DATA and absent here. */
static const unsigned collision_ops68[][6]={
 {0x019bef,3,4,1,0,0xad},
 {0x019bf2,2,2,1,0,0x29},
 {0x019bf4,2,0,1,0,0xd0},
 {0x019bf6,3,4,1,0,0xad},
 {0x019bf9,2,0,1,0,0x30},
 {0x019bfb,2,2,1,0,0xa9},
 {0x019bfd,2,3,1,0,0x85},
 {0x019bff,3,4,1,0,0xad},
 {0x019c02,1,4,1,0,0x5a},
 {0x019c03,1,2,1,2,0x0a},
 {0x019c04,1,2,1,2,0xa8},
 {0x019c05,3,5,1,2,0xb9},
 {0x019c08,2,3,1,2,0x85},
 {0x019c0a,3,5,1,2,0xb9},
 {0x019c0d,2,3,1,2,0x85},
 {0x019c0f,1,5,1,2,0x7a},
 {0x019c1b,3,5,1,0,0xbd},
 {0x019c1e,2,0,1,0,0x30},
 {0x019c20,2,2,1,0,0xa9},
 {0x019c22,2,3,1,0,0x85},
 {0x019c24,3,5,1,0,0xbd},
 {0x019c27,1,4,1,0,0x5a},
 {0x019c28,1,2,1,2,0x0a},
 {0x019c29,1,2,1,2,0xa8},
 {0x019c2a,3,5,1,2,0xb9},
 {0x019c2d,2,3,1,2,0x85},
 {0x019c2f,3,5,1,2,0xb9},
 {0x019c32,2,3,1,2,0x85},
 {0x019c34,1,5,1,2,0x7a},
 {0x019c40,3,3,1,0,0xa0},
 {0x019c43,2,3,1,0,0xc2},
 {0x019c45,3,5,0,0,0xad},
 {0x019c48,1,2,0,0,0x18},
 {0x019c49,3,6,0,0,0x79},
 {0x019c4c,2,4,0,0,0x85},
 {0x019c4e,3,5,0,0,0xad},
 {0x019c51,1,2,0,0,0x18},
 {0x019c52,3,6,0,0,0x79},
 {0x019c55,2,4,0,0,0x85},
 {0x019c57,3,3,0,0,0x29},
 {0x019c5a,2,3,0,0,0xe2},
 {0x019c5f,3,4,1,0,0xad},
 {0x019c62,2,2,1,0,0x29},
 {0x019c64,3,5,1,0,0x99},
 {0x019c67,1,2,1,0,0xc8},
 {0x019c68,1,2,1,0,0xc8},
 {0x019c69,1,2,1,0,0xc8},
 {0x019c6a,1,2,1,0,0xc8},
 {0x019c6b,2,5,1,0,0xc6},
 {0x019c6d,2,0,1,0,0xd0},
 {0x019c6f,3,4,1,0,0xad},
 {0x019c72,2,0,1,0,0xf0},
 {0x019c77,3,3,1,0,0xa0},
 {0x019c7a,2,3,1,0,0xc2},
 {0x019c7c,3,5,0,0,0xad},
 {0x019c7f,1,2,0,0,0x18},
 {0x019c80,3,6,0,0,0x79},
 {0x019c83,2,4,0,0,0x85},
 {0x019c85,3,5,0,0,0xad},
 {0x019c88,1,2,0,0,0x18},
 {0x019c89,3,6,0,0,0x79},
 {0x019c8c,2,4,0,0,0x85},
 {0x019c8e,3,3,0,0,0x29},
 {0x019c91,2,3,0,0,0xe2},
 {0x019c96,3,4,1,0,0xad},
 {0x019c99,2,2,1,0,0x29},
 {0x019c9b,3,5,1,0,0x99},
 {0x019c9e,1,2,1,0,0xc8},
 {0x019c9f,1,2,1,0,0xc8},
 {0x019ca0,1,2,1,0,0xc8},
 {0x019ca1,1,2,1,0,0xc8},
 {0x019ca2,2,5,1,0,0xc6},
 {0x019ca4,2,0,1,0,0xd0},
 {0x019ca6,3,4,1,0,0xad},
 {0x019ca9,2,0,1,0,0xf0},
 {0x019cae,3,4,1,0,0xad},
 {0x019cb1,3,4,1,0,0x0d},
 {0x019cb4,2,0,1,0,0xd0},
 {0x019cb6,3,4,1,0,0xad},
 {0x019cb9,3,4,1,0,0x0d},
 {0x019cbc,2,0,1,0,0xd0},
 {0x019cbe,2,2,1,0,0xa9},
 {0x019cc0,3,6,1,0,0x1c},
 {0x019cc4,2,3,1,0,0xc2},
 {0x019cc6,3,5,0,0,0xad},
 {0x019cc9,1,2,0,0,0x18},
 {0x019cca,3,3,0,0,0x69},
 {0x019ccd,3,5,0,0,0x8d},
 {0x019cd0,3,5,0,0,0x9c},
 {0x019cd3,3,5,0,0,0x9c},
 {0x019cd6,3,3,0,0,0x29},
 {0x019cd9,2,3,0,0,0xe2},
 {0x019cdb,2,2,1,0,0xa9},
 {0x019cdd,3,4,1,0,0x8d},
 {0x019ce1,2,3,1,0,0xc2},
 {0x019ce3,3,5,0,0,0xad},
 {0x019ce6,1,2,0,0,0x38},
 {0x019ce7,3,3,0,0,0xe9},
 {0x019cea,3,5,0,0,0x8d},
 {0x019ced,3,5,0,0,0x9c},
 {0x019cf0,3,5,0,0,0x9c},
 {0x019cf3,3,3,0,0,0x29},
 {0x019cf6,2,3,0,0,0xe2},
 {0x019cf8,2,2,1,0,0xa9},
 {0x019cfa,3,4,1,0,0x8d},
 {0x019cfe,3,3,1,0,0xa0},
 {0x019d01,2,3,1,0,0xc2},
 {0x019d03,3,5,0,0,0xad},
 {0x019d06,1,2,0,0,0x18},
 {0x019d07,3,6,0,0,0x79},
 {0x019d0a,2,4,0,0,0x85},
 {0x019d0c,3,5,0,0,0xad},
 {0x019d0f,1,2,0,0,0x18},
 {0x019d10,3,6,0,0,0x79},
 {0x019d13,2,4,0,0,0x85},
 {0x019d15,3,3,0,0,0x29},
 {0x019d18,2,3,0,0,0xe2},
 {0x019d1d,3,4,1,0,0xad},
 {0x019d20,2,2,1,0,0x29},
 {0x019d22,3,5,1,0,0x99},
 {0x019d25,1,2,1,0,0xc8},
 {0x019d26,1,2,1,0,0xc8},
 {0x019d27,1,2,1,0,0xc8},
 {0x019d28,1,2,1,0,0xc8},
 {0x019d29,2,5,1,0,0xc6},
 {0x019d2b,2,0,1,0,0xd0},
 {0x019d2d,3,4,1,0,0xad},
 {0x019d30,2,0,1,0,0xf0},
 {0x019d35,3,3,1,0,0xa0},
 {0x019d38,2,3,1,0,0xc2},
 {0x019d3a,3,5,0,0,0xad},
 {0x019d3d,1,2,0,0,0x18},
 {0x019d3e,3,6,0,0,0x79},
 {0x019d41,2,4,0,0,0x85},
 {0x019d43,3,5,0,0,0xad},
 {0x019d46,1,2,0,0,0x18},
 {0x019d47,3,6,0,0,0x79},
 {0x019d4a,2,4,0,0,0x85},
 {0x019d4c,3,3,0,0,0x29},
 {0x019d4f,2,3,0,0,0xe2},
 {0x019d54,3,4,1,0,0xad},
 {0x019d57,2,2,1,0,0x29},
 {0x019d59,3,5,1,0,0x99},
 {0x019d5c,1,2,1,0,0xc8},
 {0x019d5d,1,2,1,0,0xc8},
 {0x019d5e,1,2,1,0,0xc8},
 {0x019d5f,1,2,1,0,0xc8},
 {0x019d60,2,5,1,0,0xc6},
 {0x019d62,2,0,1,0,0xd0},
 {0x019d64,3,4,1,0,0xad},
 {0x019d67,2,0,1,0,0xf0},
 {0x019d6c,3,4,1,0,0xad},
 {0x019d6f,3,4,1,0,0x0d},
 {0x019d72,2,0,1,0,0xd0},
 {0x019d74,3,4,1,0,0xad},
 {0x019d77,3,4,1,0,0x0d},
 {0x019d7a,2,0,1,0,0xd0},
 {0x019d7c,2,2,1,0,0xa9},
 {0x019d7e,3,6,1,0,0x1c},
 {0x019d82,2,3,1,0,0xc2},
 {0x019d84,3,5,0,0,0xad},
 {0x019d87,1,2,0,0,0x18},
 {0x019d88,3,3,0,0,0x69},
 {0x019d8b,3,5,0,0,0x8d},
 {0x019d8e,3,5,0,0,0x9c},
 {0x019d91,3,5,0,0,0x9c},
 {0x019d94,3,3,0,0,0x29},
 {0x019d97,2,3,0,0,0xe2},
 {0x019d99,3,4,1,0,0x9c},
 {0x019d9d,2,3,1,0,0xc2},
 {0x019d9f,3,5,0,0,0xad},
 {0x019da2,1,2,0,0,0x38},
 {0x019da3,3,3,0,0,0xe9},
 {0x019da6,3,5,0,0,0x8d},
 {0x019da9,3,5,0,0,0x9c},
 {0x019dac,3,5,0,0,0x9c},
 {0x019daf,3,3,0,0,0x29},
 {0x019db2,2,3,0,0,0xe2},
 {0x019db4,2,2,1,0,0xa9},
 {0x019db6,3,4,1,0,0x8d},
 {0x019e0a,3,3,1,0,0xa0},
 {0x019e0d,2,3,1,0,0xc2},
 {0x019e0f,3,6,0,0,0xbd},
 {0x019e12,1,2,0,0,0x18},
 {0x019e13,3,6,0,0,0x79},
 {0x019e16,2,4,0,0,0x85},
 {0x019e18,3,6,0,0,0xbd},
 {0x019e1b,1,2,0,0,0x18},
 {0x019e1c,3,6,0,0,0x79},
 {0x019e1f,2,4,0,0,0x85},
 {0x019e21,3,3,0,0,0x29},
 {0x019e24,2,3,0,0,0xe2},
 {0x019e29,3,4,1,0,0xad},
 {0x019e2c,2,0,1,0,0x30},
 {0x019e2e,2,2,1,0,0x29},
 {0x019e30,1,4,1,0,0x5a},
 {0x019e31,1,2,1,2,0x0a},
 {0x019e32,1,2,1,2,0xa8},
 {0x019e33,3,5,1,2,0xb9},
 {0x019e36,2,3,1,2,0x85},
 {0x019e38,3,5,1,2,0xb9},
 {0x019e3b,2,3,1,2,0x85},
 {0x019e3d,1,5,1,2,0x7a},
 {0x019e41,1,2,1,0,0xc8},
 {0x019e42,1,2,1,0,0xc8},
 {0x019e43,1,2,1,0,0xc8},
 {0x019e44,1,2,1,0,0xc8},
 {0x019e45,2,5,1,0,0xc6},
 {0x019e47,2,0,1,0,0xd0},
 {0x019e49,3,5,1,0,0xbd},
 {0x019e4c,2,2,1,0,0x29},
 {0x019e4e,3,5,1,0,0x9d},
 {0x019e6a,2,3,1,0,0xc2},
 {0x019e6c,3,6,0,0,0xbd},
 {0x019e6f,1,2,0,0,0x18},
 {0x019e70,3,3,0,0,0x69},
 {0x019e73,3,3,0,0,0x29},
 {0x019e76,1,2,0,0,0x38},
 {0x019e77,3,3,0,0,0xe9},
 {0x019e7a,3,6,0,0,0x9d},
 {0x019e7d,3,6,0,0,0x9e},
 {0x019e80,3,3,0,0,0x29},
 {0x019e83,2,3,0,0,0xe2},
 {0x019e85,3,5,1,0,0xbd},
 {0x019e88,2,2,1,0,0x09},
 {0x019e8a,3,5,1,0,0x9d},
 {0x019e8e,3,3,1,0,0xa0},
 {0x019e91,2,3,1,0,0xc2},
 {0x019e93,3,6,0,0,0xbd},
 {0x019e96,1,2,0,0,0x18},
 {0x019e97,3,6,0,0,0x79},
 {0x019e9a,2,4,0,0,0x85},
 {0x019e9c,3,6,0,0,0xbd},
 {0x019e9f,1,2,0,0,0x18},
 {0x019ea0,3,6,0,0,0x79},
 {0x019ea3,2,4,0,0,0x85},
 {0x019ea5,3,3,0,0,0x29},
 {0x019ea8,2,3,0,0,0xe2},
 {0x019ead,3,4,1,0,0xad},
 {0x019eb0,2,0,1,0,0x30},
 {0x019eb2,2,2,1,0,0x29},
 {0x019eb4,1,4,1,0,0x5a},
 {0x019eb5,1,2,1,2,0x0a},
 {0x019eb6,1,2,1,2,0xa8},
 {0x019eb7,3,5,1,2,0xb9},
 {0x019eba,2,3,1,2,0x85},
 {0x019ebc,3,5,1,2,0xb9},
 {0x019ebf,2,3,1,2,0x85},
 {0x019ec1,1,5,1,2,0x7a},
 {0x019ec5,1,2,1,0,0xc8},
 {0x019ec6,1,2,1,0,0xc8},
 {0x019ec7,1,2,1,0,0xc8},
 {0x019ec8,1,2,1,0,0xc8},
 {0x019ec9,2,5,1,0,0xc6},
 {0x019ecb,2,0,1,0,0xd0},
 {0x019ecd,3,5,1,0,0xbd},
 {0x019ed0,2,2,1,0,0x29},
 {0x019ed2,3,5,1,0,0x9d},
 {0x019eee,2,3,1,0,0xc2},
 {0x019ef0,3,6,0,0,0xbd},
 {0x019ef3,1,2,0,0,0x38},
 {0x019ef4,3,3,0,0,0xe9},
 {0x019ef7,3,3,0,0,0x29},
 {0x019efa,1,2,0,0,0x18},
 {0x019efb,3,3,0,0,0x69},
 {0x019efe,3,6,0,0,0x9d},
 {0x019f01,3,6,0,0,0x9e},
 {0x019f04,3,3,0,0,0x29},
 {0x019f07,2,3,0,0,0xe2},
 {0x019f09,3,5,1,0,0xbd},
 {0x019f0c,2,2,1,0,0x09},
 {0x019f0e,3,5,1,0,0x9d},
 {0x019f12,3,3,1,0,0xa0},
 {0x019f15,2,3,1,0,0xc2},
 {0x019f17,3,6,0,0,0xbd},
 {0x019f1a,1,2,0,0,0x18},
 {0x019f1b,3,6,0,0,0x79},
 {0x019f1e,2,4,0,0,0x85},
 {0x019f20,3,6,0,0,0xbd},
 {0x019f23,1,2,0,0,0x18},
 {0x019f24,3,6,0,0,0x79},
 {0x019f27,2,4,0,0,0x85},
 {0x019f29,3,3,0,0,0x29},
 {0x019f2c,2,3,0,0,0xe2},
 {0x019f31,3,4,1,0,0xad},
 {0x019f34,2,0,1,0,0x30},
 {0x019f36,2,2,1,0,0x29},
 {0x019f38,1,4,1,0,0x5a},
 {0x019f39,1,2,1,2,0x0a},
 {0x019f3a,1,2,1,2,0xa8},
 {0x019f3b,3,5,1,2,0xb9},
 {0x019f3e,2,3,1,2,0x85},
 {0x019f40,3,5,1,2,0xb9},
 {0x019f43,2,3,1,2,0x85},
 {0x019f45,1,5,1,2,0x7a},
 {0x019f49,1,2,1,0,0xc8},
 {0x019f4a,1,2,1,0,0xc8},
 {0x019f4b,1,2,1,0,0xc8},
 {0x019f4c,1,2,1,0,0xc8},
 {0x019f4d,2,5,1,0,0xc6},
 {0x019f4f,2,0,1,0,0xd0},
 {0x019f51,3,5,1,0,0xbd},
 {0x019f54,2,2,1,0,0x29},
 {0x019f56,3,5,1,0,0x9d},
 {0x019f72,2,3,1,0,0xc2},
 {0x019f74,3,6,0,0,0xbd},
 {0x019f77,1,2,0,0,0x18},
 {0x019f78,3,3,0,0,0x69},
 {0x019f7b,3,3,0,0,0x29},
 {0x019f7e,1,2,0,0,0x38},
 {0x019f7f,3,3,0,0,0xe9},
 {0x019f82,3,6,0,0,0x9d},
 {0x019f85,3,6,0,0,0x9e},
 {0x019f88,3,3,0,0,0x29},
 {0x019f8b,2,3,0,0,0xe2},
 {0x019f8d,3,5,1,0,0xbd},
 {0x019f90,2,2,1,0,0x09},
 {0x019f92,3,5,1,0,0x9d},
 {0x019f96,3,3,1,0,0xa0},
 {0x019f99,2,3,1,0,0xc2},
 {0x019f9b,3,6,0,0,0xbd},
 {0x019f9e,1,2,0,0,0x18},
 {0x019f9f,3,6,0,0,0x79},
 {0x019fa2,2,4,0,0,0x85},
 {0x019fa4,3,6,0,0,0xbd},
 {0x019fa7,1,2,0,0,0x18},
 {0x019fa8,3,6,0,0,0x79},
 {0x019fab,2,4,0,0,0x85},
 {0x019fad,3,3,0,0,0x29},
 {0x019fb0,2,3,0,0,0xe2},
 {0x019fb5,3,4,1,0,0xad},
 {0x019fb8,2,0,1,0,0x30},
 {0x019fba,2,2,1,0,0x29},
 {0x019fbc,1,4,1,0,0x5a},
 {0x019fbd,1,2,1,2,0x0a},
 {0x019fbe,1,2,1,2,0xa8},
 {0x019fbf,3,5,1,2,0xb9},
 {0x019fc2,2,3,1,2,0x85},
 {0x019fc4,3,5,1,2,0xb9},
 {0x019fc7,2,3,1,2,0x85},
 {0x019fc9,1,5,1,2,0x7a},
 {0x019fcd,1,2,1,0,0xc8},
 {0x019fce,1,2,1,0,0xc8},
 {0x019fcf,1,2,1,0,0xc8},
 {0x019fd0,1,2,1,0,0xc8},
 {0x019fd1,2,5,1,0,0xc6},
 {0x019fd3,2,0,1,0,0xd0},
 {0x019fd5,3,5,1,0,0xbd},
 {0x019fd8,2,2,1,0,0x29},
 {0x019fda,3,5,1,0,0x9d},
 {0x019ff6,2,3,1,0,0xc2},
 {0x019ff8,3,6,0,0,0xbd},
 {0x019ffb,1,2,0,0,0x38},
 {0x019ffc,3,3,0,0,0xe9},
 {0x019fff,3,3,0,0,0x29},
 {0x01a002,1,2,0,0,0x18},
 {0x01a003,3,3,0,0,0x69},
 {0x01a006,3,6,0,0,0x9d},
 {0x01a009,3,6,0,0,0x9e},
 {0x01a00c,3,3,0,0,0x29},
 {0x01a00f,2,3,0,0,0xe2},
 {0x01a011,3,5,1,0,0xbd},
 {0x01a014,2,2,1,0,0x09},
 {0x01a016,3,5,1,0,0x9d},
};
static const unsigned collision_boundaries68[][2]={
 {0x019c10,3},
 {0x019c35,3},
 {0x019c5c,3},
 {0x019c74,3},
 {0x019c93,3},
 {0x019cab,3},
 {0x019cc3,1},
 {0x019ce0,1},
 {0x019cfd,1},
 {0x019d1a,3},
 {0x019d32,3},
 {0x019d51,3},
 {0x019d69,3},
 {0x019d81,1},
 {0x019d9c,1},
 {0x019db9,1},
 {0x019e26,3},
 {0x019e3e,3},
 {0x019e51,1},
 {0x019e8d,1},
 {0x019eaa,3},
 {0x019ec2,3},
 {0x019ed5,1},
 {0x019f11,1},
 {0x019f2e,3},
 {0x019f46,3},
 {0x019f59,1},
 {0x019f95,1},
 {0x019fb2,3},
 {0x019fca,3},
 {0x019fdd,1},
 {0x01a019,1},
 {0x019bee,1}
};

static unsigned collision_offset68(unsigned pc){return (pc>>16)*32768+(pc&32767);}
static int collision_mapped68(unsigned pc){
 if(pc==0x019bce){for(unsigned p=0x019bce;p<0x01a01a;p++)if(!cpu_aot_rom_mapping_matches(p,collision_offset68(p),1))return 0;return cpu_aot_rom_mapping_matches(0x0180c1,0x80c1,3);}
 if(pc==0x0180c1)return cpu_aot_rom_mapping_matches(pc,0x80c1,3);
 if(pc==0x0180c4)return cpu_aot_rom_mapping_matches(pc,0x80c4,3);
 for(unsigned j=0;j<sizeof collision_ops68/sizeof*collision_ops68;j++)if(collision_ops68[j][0]==pc)return cpu_aot_rom_mapping_matches(pc,collision_offset68(pc),collision_ops68[j][1]);
 for(unsigned j=0;j<sizeof collision_boundaries68/sizeof*collision_boundaries68;j++)if(collision_boundaries68[j][0]==pc)return cpu_aot_rom_mapping_matches(pc,collision_offset68(pc),collision_boundaries68[j][1]);return 0;
}
static int collision_arm68(CpuState*c,uint64_t gen){
 int ok=pending68.armed&&pending68.cpu==c&&pending68.ram==c->ram&&pending68.gen==gen&&pending68.s==c->S+2&&pending68.x==c->X;
 pending68.armed=0;collision68.armed=collision68.fresh=0;
 if(!ok||!collision_mode68(c)||!c->m_flag||c->S<0x120||c->S>0x1ffd||c->ram[c->S+1]!=0xc3||c->ram[c->S+2]!=0x80)return 0;
 collision68.cpu=c;collision68.ram=c->ram;collision68.gen=gen;collision68.s=c->S;collision68.x=c->X;memcpy(collision68.frame,c->ram+c->S+1,2);collision68.armed=collision68.fresh=1;collision_stats68.entries++;return 1;
}
static void collision_nz68(CpuState*c,unsigned v,unsigned bits){unsigned mask=bits==8?255:65535;c->_flag_Z=!(v&mask);c->_flag_N=!!(v&(1u<<(bits-1)));cpu_mirrors_to_p(c);}
static void collision_a68(CpuState*c,unsigned v){c->A=c->m_flag?((c->A&0xff00)|(v&255)):(v&65535);collision_nz68(c,v,c->m_flag?8:16);}
static unsigned collision_rd68(CpuState*c,unsigned p,unsigned bytes){uint8_t b;uint16_t v;p&=0xffffff;if(bytes==1){if(!interp_bridge_source_read8(c,p,&b))abort();extra68+=cpu_region_speed(p)-6;return b;}unsigned high=(p+1)&0xffffff;if(!interp_bridge_source_read16(c,p,high,&v))abort();extra68+=cpu_region_speed(p)-6+cpu_region_speed(high)-6;return v;}
static void collision_wr68(CpuState*c,unsigned p,unsigned v,unsigned bytes){p&=0xffffff;if(bytes==1){if(!interp_bridge_source_write8(c,p,v))abort();extra68+=cpu_region_speed(p)-6;}else{unsigned high=(p+1)&0xffffff;if(!interp_bridge_source_write16(c,p,high,v))abort();extra68+=cpu_region_speed(p)-6+cpu_region_speed(high)-6;}}
static void collision_adc68(CpuState*c,unsigned v,int subtract){unsigned mask=c->m_flag?255:65535,sign=c->m_flag?128:32768,x=c->A&mask,y=v&mask,sum;if(subtract)y^=mask;sum=x+y+c->_flag_C;c->_flag_C=sum>mask;c->_flag_V=!!((~(x^y)&(x^sum))&sign);collision_a68(c,sum);}
int LagoonCollision68_Process(const uint8_t*r,size_t n,CpuState*c,const CpuState*primary,uint64_t gen,uint64_t frame,uint32_t pc,uint32_t*landing){
 if(pc==0x0180c4){collision68.armed=collision68.fresh=pending68.armed=0;return 0;}
 if(!landing||c!=primary||gen!=frame||!collision_mode68(c)||!LagoonCollision68_Source(r,n)){collision68.armed=collision68.fresh=pending68.armed=0;collision_stats68.refused++;return 0;}
 if(pc==0x0180c1){collision68.armed=collision68.fresh=pending68.armed=0;if(c->m_flag&&c->X<=0x800&&!(c->X&63)&&c->S>=0x124&&c->S<=0x1fff&&collision_mapped68(pc)){pending68.cpu=c;pending68.ram=c->ram;pending68.gen=gen;pending68.s=c->S;pending68.x=c->X;pending68.armed=1;}return 0;}
 if(pc==0x019bce){if(pending68.armed)collision_arm68(c,gen);else if(!collision68.fresh)collision68.armed=0;return 0;}
 CollisionOwner68*w=&collision68;
 if(!w->armed||w->cpu!=c||w->ram!=c->ram||w->gen!=gen||w->x!=c->X||memcmp(w->frame,c->ram+w->s+1,2)){w->armed=w->fresh=0;collision_stats68.refused++;return 0;}
 for(unsigned j=0;j<sizeof collision_boundaries68/sizeof*collision_boundaries68;j++)if(collision_boundaries68[j][0]==pc){if(r[collision_offset68(pc)]==0x60&&c->S==w->s)w->armed=w->fresh=0;return 0;}
 const unsigned*op=NULL;for(unsigned j=0;j<sizeof collision_ops68/sizeof*collision_ops68;j++)if(collision_ops68[j][0]==pc){op=collision_ops68[j];break;}if(!op)return 0;
 if(c->m_flag!=op[3]||w->s!=c->S+op[4]){w->armed=w->fresh=0;collision_stats68.refused++;return 0;}
 if(interp_bridge_source_opcode_deadline_reached(c)){collision_stats68.deadlines++;return 0;}
 unsigned at=collision_offset68(pc),bytes=op[1],cycles=op[2],im=bytes>1?r[at+1]:0,operand=im|(bytes==3?r[at+2]<<8:0),next=pc+bytes,width=c->m_flag?1:2,address=0;uint64_t prior=c->master_cycles;extra68=0;w->fresh=0;
 if(!cycles){unsigned take=op[5]==0x30?c->_flag_N:op[5]==0xf0?c->_flag_Z:!c->_flag_Z;cycles=take?3:2;if(take)next=pc+bytes+(int8_t)im;}
 c->coprocessor_master_cycles=prior;c->open_bus=r[at+bytes-1];
 CpuWramReadContext ctx={.instruction_pc=pc,.stack=c->S,.y=c->Y,.pb=c->PB,.m=c->m_flag,.x=c->x_flag,.e=c->emulation,.interpreter=1};const CpuWramReadContext*old=cpu_swap_wram_read_context(&ctx);
 switch(op[5]){
 case 0xa9:collision_a68(c,operand);break;
 case 0xa5:collision_a68(c,collision_rd68(c,im,width));break;
 case 0xad:collision_a68(c,collision_rd68(c,0x010000|operand,width));break;
 case 0xbd:collision_a68(c,collision_rd68(c,0x010000+operand+c->X,width));break;
 case 0xb9:collision_a68(c,collision_rd68(c,0x010000+operand+c->Y,width));break;
 case 0xa0:c->Y=operand;collision_nz68(c,c->Y,16);break;
 case 0x85:collision_wr68(c,im,c->A,width);break;
 case 0x8d:collision_wr68(c,0x010000|operand,c->A,width);break;
 case 0x9d:collision_wr68(c,0x010000+operand+c->X,c->A,width);break;
 case 0x99:collision_wr68(c,0x010000+operand+c->Y,c->A,width);break;
 case 0x9c:collision_wr68(c,0x010000|operand,0,width);break;
 case 0x9e:collision_wr68(c,0x010000+operand+c->X,0,width);break;
 case 0x29:collision_a68(c,c->A&operand);break;
 case 0x09:collision_a68(c,c->A|operand);break;
 case 0x0d:collision_a68(c,c->A|collision_rd68(c,0x010000|operand,width));break;
 case 0x69:collision_adc68(c,operand,0);break;
 case 0x79:collision_adc68(c,collision_rd68(c,0x010000+operand+c->Y,width),0);break;
 case 0xe9:collision_adc68(c,operand,1);break;
 case 0x18:c->_flag_C=0;cpu_mirrors_to_p(c);break;
 case 0x38:c->_flag_C=1;cpu_mirrors_to_p(c);break;
 case 0x0a:c->_flag_C=!!(c->A&(c->m_flag?128:32768));collision_a68(c,c->A<<1);break;
 case 0xa8:c->Y=c->A;collision_nz68(c,c->Y,16);break;
 case 0xc8:c->Y=(uint16_t)(c->Y+1);collision_nz68(c,c->Y,16);break;
 case 0xc6:{unsigned v=(collision_rd68(c,im,width)-1)&(width==1?255:65535);collision_wr68(c,im,v,width);collision_nz68(c,v,width==1?8:16);break;}
 case 0x1c:address=0x010000|operand;{unsigned v=collision_rd68(c,address,width);c->_flag_Z=!(v&c->A&(width==1?255:65535));cpu_mirrors_to_p(c);collision_wr68(c,address,v&~c->A,width);}break;
 case 0x5a:collision_wr68(c,c->S,c->Y>>8,1);c->S--;collision_wr68(c,c->S,c->Y,1);c->S--;break;
 case 0x7a:c->Y=collision_rd68(c,c->S+1,2);c->S+=2;collision_nz68(c,c->Y,16);break;
 case 0xc2:c->P&=~im;cpu_p_to_mirrors(c);break;
 case 0xe2:c->P|=im;cpu_p_to_mirrors(c);break;
 case 0x30:case 0xd0:case 0xf0:break;
 default:abort();
 }
 cpu_swap_wram_read_context(old);c->cycles+=cycles;c->master_cycles+=cycles*6+bytes*2+extra68;if(pc<0x019c40)collision_stats68.parent++;else collision_stats68.probes++;if(!interp_bridge_commit_source_opcode(c,prior,false))abort();*landing=next;return 1;
}
#ifndef LAGOON_COLLISION68_TEST
#include "common_rtl.h"
#include <stdio.h>
static int enabled68=-1,report68;static uint64_t generation68;
static void collision_exit68(void){if(report68)fprintf(stderr,"[native-collision68] entries=%llu parent=%llu probes=%llu deadlines=%llu refused=%llu\n",(unsigned long long)collision_stats68.entries,(unsigned long long)collision_stats68.parent,(unsigned long long)collision_stats68.probes,(unsigned long long)collision_stats68.deadlines,(unsigned long long)collision_stats68.refused);}
static void collision_hook68(CpuState*c,uint32_t pc){if(!enabled68||c!=&g_cpu||!interp_bridge_source_opcode_available(c))return;if(!collision_mapped68(pc)){collision68.armed=collision68.fresh=pending68.armed=0;return;}uint32_t landing;if(LagoonCollision68_Process(g_rom,0x100000,c,&g_cpu,RtlStateGeneration(),generation68,pc,&landing))interp_bridge_pre_opcode_redirect(landing);}
bool LagoonCollision68_Entry(CpuState*c,uint16_t s,uint8_t h,uint32_t host,RecompReturn*out){(void)out;if(enabled68&&c==&g_cpu&&c->S==s&&h==2&&c->host_return_valid==2&&host==0x0180c4&&RtlStateGeneration()==generation68&&LagoonCollision68_Source(g_rom,0x100000)&&collision_mapped68(0x019bce))collision_arm68(c,generation68);return false;}
void LagoonCollision68_BeforeFrame(void){if(enabled68<0){const char*v=getenv("LAGOON_TIMING68");enabled68=!v||strcmp(v,"0");v=getenv("LAGOON_NATIVE_WAIT_REPORT");report68=v&&!strcmp(v,"1");if(report68)atexit(collision_exit68);}generation68=RtlStateGeneration();if(collision68.gen!=generation68)collision68.armed=collision68.fresh=0;if(pending68.gen!=generation68)pending68.armed=0;if(enabled68){if(!interp_bridge_add_pre_opcode_hook(0x0180c1,collision_hook68)||!interp_bridge_add_pre_opcode_hook(0x0180c4,collision_hook68)||!interp_bridge_add_pre_opcode_hook(0x019bce,collision_hook68))abort();for(unsigned j=0;j<sizeof collision_ops68/sizeof*collision_ops68;j++)if(!interp_bridge_add_pre_opcode_hook(collision_ops68[j][0],collision_hook68))abort();for(unsigned j=0;j<sizeof collision_boundaries68/sizeof*collision_boundaries68;j++)if(!interp_bridge_add_pre_opcode_hook(collision_boundaries68[j][0],collision_hook68))abort();}}
#endif
