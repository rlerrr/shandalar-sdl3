#include "defs.h"

enum OrigRaritySet
{
  SET_INVALID = 0xFF,
  SET_ANTIQUITIES = 0,
  SET_ARABIAN_NIGHTS = 1,
  SET_ASTRAL = 2,
  SET_THE_DARK = 3,
  SET_LEGENDS = 4,
  SET_PROMO = 5,
  SET_UNLIMITED = 6,
  SET_8TH_EDITION = 7,
  SET_FALLEN_EMPIRES = 8,
  SET_TEMPEST = 9,
  SET_RANDOM = 10
};

typedef struct OrigRarities_t
{
  int8_t set;
  char rarity;
  char exp_rarities[4];
} OrigRarities;

typedef struct
{
  char *damage_text;
  char *effect_title;
  char *effect_text;
  char *legacy_title;
  char *legacy_text;
} name_table_entry_t;
STATIC_ASSERT(sizeof(name_table_entry_t) == 0x14, name_table_entry_t_wrong_size);

extern name_table_entry_t global_legacy_names[2000];
extern char *global_base_txt;

void FreeRaritiesCsvRaw(void);
int read_db_guts(char *cards_dat_filename);
int ReadLegacyCsv(const char *filename);
int ReadDeckbuilderCsv(const char *filename, OrigRarities *orig_rarities);
