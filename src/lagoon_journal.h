#ifndef LAGOON_JOURNAL_H
#define LAGOON_JOURNAL_H
#include <stddef.h>
#include <stdint.h>
enum { LAGOON_JOURNAL_RAM_BYTES=0x4f0 };

typedef struct LagoonJournalEntry {
 const char *chapter;
 const char *title;
 const char *detail;
 const char *milestone[4];
 uint8_t milestone_count,completed_mask,defeated_boss_mask;
 /* A marker exists only when the existing exact tablet guide provides it. */
 uint8_t marker,room;
 uint16_t x,y;
} LagoonJournalEntry;

/* Stateless presentation: loading a save immediately changes the next answer.
 * No invented conversation history or separate quest-save database is maintained. */
void LagoonJournal_Init(const uint8_t *rom,size_t size);
int LagoonJournal_Get(const uint8_t *ram,size_t size,LagoonJournalEntry *out);
#endif
