#ifndef SHANDALAR_DECK_FILE_H
#define SHANDALAR_DECK_FILE_H

typedef struct DeckFileEntry {
  int csvid;
  int amount;
  const char *name;
} DeckFileEntry;

typedef int (*DeckFileAddCardFn)(int csvid, int amount, void *userdata);

int deck_file_save(const char *filename, const DeckFileEntry *entries, int entry_count);
int deck_file_load(const char *filename, DeckFileAddCardFn add_card, void *userdata, int *out_card_count);

#endif
