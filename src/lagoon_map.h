#ifndef LAGOON_MAP_H
#define LAGOON_MAP_H
#include <stddef.h>
#include <stdint.h>
enum { LAGOON_MAP_ROOMS=75, LAGOON_MAP_SIDE=128, LAGOON_MAP_CELLS=16384 };
enum { LAGOON_MAP_TOGGLE=1,LAGOON_MAP_CLOSE,LAGOON_MAP_PREVIOUS,LAGOON_MAP_NEXT,LAGOON_MAP_HOME };
/* Title-owned exploration only: no guest writes, no ROM graphics in the save. */
void LagoonMap_Init(const uint8_t *rom,size_t size);
void LagoonMap_Observe(const uint8_t *ram,uint64_t generation);
void LagoonMap_Command(unsigned command);
int LagoonMap_IsOpen(void);
int LagoonMap_PollOpen(uint32_t human);
void LagoonMap_Tick(uint32_t human,uint32_t ticks);
int LagoonMap_Image(const uint32_t **image,int *width,int *height);
void LagoonMap_Save(void);
/* Diagnostics exercise precisely the same opener/closer as human input. */
void LagoonMap_AfterFrame(unsigned frame);
#endif
