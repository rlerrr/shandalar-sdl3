#include "defs.h"
#include "card_db.h"
#include "platform/legacy_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *CsvParseNextField(char **txt);

static char *read_whole_file(const char *filename, size_t *out_size)
{
  FILE *file;
  long size;
  char *buffer;

  if (out_size)
    *out_size = 0;
  file = fopen(filename, "rb");
  if (!file)
    return NULL;
  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return NULL;
  }
  size = ftell(file);
  if (size < 0) {
    fclose(file);
    return NULL;
  }
  rewind(file);
  buffer = (char *)malloc((size_t)size + 1);
  if (!buffer) {
    fclose(file);
    return NULL;
  }
  if (fread(buffer, 1, (size_t)size, file) != (size_t)size) {
    free(buffer);
    fclose(file);
    return NULL;
  }
  fclose(file);
  buffer[size] = '\0';
  if (out_size)
    *out_size = (size_t)size;
  return buffer;
}

// GLOBAL: DECKDLL 0x100355d8
// GLOBAL: MAGIC 0x0056eab4
// GLOBAL: SHANDALAR 0x0058397c
static char read_db_empty_rules[1] = "";

// GLOBAL: DECKDLL 0x100355ec
// GLOBAL: MAGIC 0x0056eac8
// GLOBAL: SHANDALAR 0x00583990
static char read_db_empty_flavor[1] = "";

// GLOBAL: DECKDLL 0x10033e48
// GLOBAL: MAGIC 0x0057b668
// GLOBAL: SHANDALAR 0x005843f0
static const char *const_db_artist_names[] = {
    "None",
    "Amy Weber",
    "Andi Rusu",
    "Anson Maddocks",
    "Anthony Waters",
    "Brian Snoddy",
    "Bryon Wackwitz",
    "Christopher Rush",
    "Cornelius Brudi",
    "Dameon Willich",
    "Dan Frazier",
    "Daniel Gelon",
    "Dennis Detwiller",
    "Douglas Shuler",
    "Drew Tucker",
    "Edward Beard\xB8 Jr.",
    "Fay Jones",
    "Frank Frazier",
    "Harold McNeill",
    "Heather Hudson",
    "Jeff A. Menges",
    "Jesper Myrfors",
    "Julie Baroh",
    "Justin Hampton",
    "Kaja & Phil Foglio",
    "Kaja Foglio",
    "Ken Meyer Jr.",
    "Kerstin Kaman",
    "Kev Brockschmidt",
    "Kristen Bishop",
    "Liz Danforth",
    "Margaret Organ-Kean",
    "Mark Poole",
    "Mark Tedin",
    "Melissa Benson",
    "Michael Whelan",
    "Mike Kimble",
    "Nicola Leonard",
    "N\xE8N\xE8 Thomas",
    "Pat Morrissey",
    "Pete Venters",
    "Phil Foglio",
    "Quinton Hoover",
    "Randy Asplund-Faith",
    "Richard Kane-Ferguson",
    "Richard Thomas",
    "Rob Alexander",
    "Ron Spencer",
    "Sandra Everingham",
    "Scott Kirschner",
    "Susan Van Camp",
    "Tom W\xE4nerstrand",
    "Unknown",
};

// GLOBAL: DECKDLL 0x101427b4
// GLOBAL: MAGIC 0x008b40d0
// GLOBAL: SHANDALAR 0x008c8250
int global_available_slots;

// GLOBAL: DECKDLL 0x10147220
// GLOBAL: MAGIC 0x008d0350
// GLOBAL: SHANDALAR 0x008e44a0
card_ptr_t global_raw_cards_storage[2000];

// GLOBAL: SHANDALAR 0x007a82e4
// GLOBAL: MAGIC 0x00791558
// GLOBAL: DECKDLL 0x1012d7d0
static char *global_rarities_csv_raw;

// GLOBAL: DECKDLL 0x1012df3c
// GLOBAL: MAGIC 0x007a7d70
// GLOBAL: SHANDALAR 0x007beafc
char *global_base_txt;

// GLOBAL: MAGIC 0x00777e60
// GLOBAL: SHANDALAR 0x0078ebf0
// GLOBAL: SHANDALAR 0x00583994
name_table_entry_t global_legacy_names[2000];

// FUNCTION: DECKDLL 0x1001a940
// FUNCTION: MAGIC 0x00452cf0
// FUNCTION: SHANDALAR 0x004424b0
int read_db_guts(char *cards_dat_filename)
{
  int init_idx;               /* ebp - 0x80c */
  int flavor_out_idx;         /* ebp - 0x808 */
  int flavor_card_idx;        /* ebp - 0x804 */
  char flavor_text_buf[1000]; /* ebp - 0x800 */
  char *flavor_in;            /* ebp - 0x418 */
  int rules_out_idx;          /* ebp - 0x414 */
  int rules_card_idx;         /* ebp - 0x410 */
  char rules_text_buf[1000];  /* ebp - 0x40c */
  char *rules_in;             /* ebp - 0x24 */
  int req_idx;                /* ebp - 0x20 */
  int exp_idx;                /* ebp - 0x1c */
  card_ptr_t *cp;
  size_t record_size;
  int card_idx; /* ebp - 0x10 */
  FILE *cards_dat;

  cards_dat = fopen(cards_dat_filename, "rb");
  if (cards_dat == NULL)
    return 0;

  fread(&global_available_slots, 4, 1, cards_dat);
  fread(&record_size, 4, 1, cards_dat);

  global_base_txt = (char *)malloc(record_size);
  if (global_base_txt == NULL)
  {
    fclose(cards_dat);
    return 0;
  }

  fread(global_raw_cards_storage, 0x98, global_available_slots, cards_dat);
  fread(global_base_txt, 1, record_size, cards_dat);
  fclose(cards_dat);

  for (card_idx = 0; card_idx < global_available_slots; card_idx += 1)
  {
    *(int *)&global_raw_cards_storage[card_idx].full_name += (int)global_base_txt;
    *(int *)&global_raw_cards_storage[card_idx].name += (int)global_base_txt;
    *(int *)&global_raw_cards_storage[card_idx].type_text += (int)global_base_txt;
    *(int *)&global_raw_cards_storage[card_idx].rules_text += (int)global_base_txt;
    *(int *)&global_raw_cards_storage[card_idx].flavor_text += (int)global_base_txt;

    if (_strcmpi(global_raw_cards_storage[card_idx].rules_text, "None") == 0)
      global_raw_cards_storage[card_idx].rules_text = read_db_empty_rules;

    if ((_strcmpi(global_raw_cards_storage[card_idx].flavor_text, "None") == 0) ||
        (_strcmpi(global_raw_cards_storage[card_idx].flavor_text, "Blank") == 0))
      global_raw_cards_storage[card_idx].flavor_text = read_db_empty_flavor;

    global_raw_cards_storage[card_idx].artist =
        const_db_artist_names[(int)global_raw_cards_storage[card_idx].artist];

    if (global_raw_cards_storage[card_idx].num_pics == 0)
      global_raw_cards_storage[card_idx].num_pics = 1;
  }

  for (exp_idx = 0; exp_idx < global_available_slots; exp_idx += 1)
    if (global_raw_cards_storage[exp_idx].expansion & 0x40)
      global_raw_cards_storage[exp_idx].expansion = 0x80;

  for (req_idx = 0; req_idx < global_available_slots; req_idx += 1)
    if ((signed char)global_raw_cards_storage[req_idx].req.req_colorless == 0x11)
      global_raw_cards_storage[req_idx].req.req_colorless = 10;

  for (rules_card_idx = 0; rules_card_idx < global_available_slots;
       rules_card_idx += 1)
  {
    rules_out_idx = 0;
    for (rules_in = global_raw_cards_storage[rules_card_idx].rules_text;
         *rules_in;)
    {
      if (((*rules_in == '\\') && (rules_in[1] == '\\')) ||
          ((*rules_in == '\\') && (rules_in[1] == 'n')))
      {
        rules_in += 1;
        rules_text_buf[rules_out_idx] = '\n';
        rules_out_idx += 1;
      }
      else
      {
        rules_text_buf[rules_out_idx] = *rules_in;
        rules_out_idx += 1;
      }

      rules_in += 1;
    }

    rules_text_buf[rules_out_idx] = '\0';
    strcpy(global_raw_cards_storage[rules_card_idx].rules_text, rules_text_buf);
  }

  for (flavor_card_idx = 0; flavor_card_idx < global_available_slots;
       flavor_card_idx += 1)
  {
    flavor_out_idx = 0;
    for (flavor_in = global_raw_cards_storage[flavor_card_idx].flavor_text;
         *flavor_in;)
    {
      if (((*flavor_in == '\\') && (flavor_in[1] == '\\')) ||
          ((*flavor_in == '\\') && (flavor_in[1] == 'n')))
      {
        flavor_in += 1;
        flavor_text_buf[flavor_out_idx] = '\n';
        flavor_out_idx += 1;
      }
      else
      {
        flavor_text_buf[flavor_out_idx] = *flavor_in;
        flavor_out_idx += 1;
      }

      flavor_in += 1;
    }

    flavor_text_buf[flavor_out_idx] = '\0';
    strcpy(global_raw_cards_storage[flavor_card_idx].flavor_text, flavor_text_buf);
  }

#define SET_HACK(idx, col) \
  *(int *)&global_raw_cards_storage[(idx)].hack_colors = (col)

  for (init_idx = 0; init_idx < global_available_slots; init_idx += 1)
  {
    global_raw_cards_storage[init_idx].sleight_color = 0;
    SET_HACK(init_idx, 0);
  }

  global_raw_cards_storage[0x227].sleight_color = 0x28;
  global_raw_cards_storage[8].sleight_color = 2;
  global_raw_cards_storage[0x10].sleight_color = 0x20;
  global_raw_cards_storage[0x13].sleight_color = 2;
  global_raw_cards_storage[0x16].sleight_color = 0x10;
  global_raw_cards_storage[0x17].sleight_color = 4;
  global_raw_cards_storage[0x20].sleight_color = 0x10;
  global_raw_cards_storage[0x21].sleight_color = 2;
  global_raw_cards_storage[0x22].sleight_color = 4;
  global_raw_cards_storage[0x23].sleight_color = 8;
  global_raw_cards_storage[0x24].sleight_color = 0x10;
  global_raw_cards_storage[0x25].sleight_color = 0x20;
  global_raw_cards_storage[0x33].sleight_color = 0x20;
  global_raw_cards_storage[0x34].sleight_color = 4;
  global_raw_cards_storage[0x3a].sleight_color = 8;
  global_raw_cards_storage[0x3b].sleight_color = 2;
  global_raw_cards_storage[0x51].sleight_color = 2;
  global_raw_cards_storage[100].sleight_color = 0x20;
  global_raw_cards_storage[0x69].sleight_color = 8;
  global_raw_cards_storage[0x7b].sleight_color = 0x10;
  global_raw_cards_storage[0x80].sleight_color = 0x20;
  global_raw_cards_storage[0x8e].sleight_color = 2;
  global_raw_cards_storage[0x8f].sleight_color = 8;
  global_raw_cards_storage[0x1b7].sleight_color = 4;
  global_raw_cards_storage[0x367].sleight_color = 2;
  global_raw_cards_storage[0xaf].sleight_color = 2;
  global_raw_cards_storage[0xc4].sleight_color = 0x20;
  global_raw_cards_storage[199].sleight_color = 4;
  global_raw_cards_storage[200].sleight_color = 0x10;
  global_raw_cards_storage[0x30d].sleight_color = 0x20;
  global_raw_cards_storage[0xee].sleight_color = 0x30;
  global_raw_cards_storage[0x16c].sleight_color = 4;
  global_raw_cards_storage[0xf2].sleight_color = 2;
  global_raw_cards_storage[0xf5].sleight_color = 4;
  global_raw_cards_storage[0xf6].sleight_color = 2;
  global_raw_cards_storage[0x353].sleight_color = 2;
  global_raw_cards_storage[0x11b].sleight_color = 2;
  global_raw_cards_storage[0x11c].sleight_color = 0x20;
  global_raw_cards_storage[0x120].sleight_color = 8;

  SET_HACK(0x125, 2);
  SET_HACK(7, 8);
  SET_HACK(0x18, 2);
  SET_HACK(0x1a, 0x10);
  SET_HACK(0x132, 0x10);
  SET_HACK(0x2d, 0x30);
  SET_HACK(0x4d, 2);
  SET_HACK(0x56, 0x20);
  SET_HACK(0x5f, 8);
  SET_HACK(0x66, 0x10);
  SET_HACK(0x3a9, 0x10);
  SET_HACK(0x1ab, 4);
  SET_HACK(0x7f, 4);
  SET_HACK(0x86, 2);
  SET_HACK(0x88, 2);
  SET_HACK(0x154, 4);
  SET_HACK(0x90, 8);
  SET_HACK(0x93, 8);
  SET_HACK(0x96, 4);
  SET_HACK(0x2cf, 2);
  SET_HACK(0x367, 2);
  SET_HACK(0xae, 2);
  SET_HACK(0xba, 4);
  SET_HACK(0xda, 4);
  SET_HACK(0x30e, 4);
  SET_HACK(0xde, 8);
  SET_HACK(900, 2);
  SET_HACK(0xfd, 4);
  SET_HACK(0x109, 0x10);
  SET_HACK(0x123, 2);

  global_raw_cards_storage[0x1c2].sleight_color = 0x10;
  global_raw_cards_storage[0x225].sleight_color = 2;
  global_raw_cards_storage[0xd2].sleight_color = 2;
  global_raw_cards_storage[0x60].sleight_color = 0x10;

  SET_HACK(0x196, 4);
  SET_HACK(0x1a0, 8);
  SET_HACK(0x1b5, 8);
  SET_HACK(0x1b8, 4);
  SET_HACK(0x1c5, 4);
  SET_HACK(0x1c7, 4);
  SET_HACK(9, 0x12);
  SET_HACK(0xc, 10);
  SET_HACK(0xbd, 0x30);
  SET_HACK(0xd4, 0x28);
  SET_HACK(0xd8, 0x22);
  SET_HACK(0xdb, 2);
  SET_HACK(0xf1, 0x18);
  SET_HACK(0xfc, 0xc);
  SET_HACK(0xfe, 0x24);
  SET_HACK(0x102, 6);
  SET_HACK(0x10a, 0x14);
  SET_HACK(0x60, 0x10);

  // For some reason NOT included in deck builder?
#ifndef DECKDLL
  global_raw_cards_storage[603].sleight_color = 0x10;
  global_raw_cards_storage[605].sleight_color = 0x10;
  global_raw_cards_storage[704].sleight_color = 0x10;
  global_raw_cards_storage[730].sleight_color = 0x20;
  global_raw_cards_storage[844].sleight_color = 2;
  global_raw_cards_storage[338].sleight_color = 0x10;
  global_raw_cards_storage[596].sleight_color = 2;
  global_raw_cards_storage[316].sleight_color = 2;
  global_raw_cards_storage[374].sleight_color = 0x20;
  global_raw_cards_storage[692].sleight_color = 0x20;
  global_raw_cards_storage[355].sleight_color = 4;
  global_raw_cards_storage[793].sleight_color = 4;
  global_raw_cards_storage[564].sleight_color = 0x20;
  global_raw_cards_storage[579].sleight_color = 0x30;
  global_raw_cards_storage[660].sleight_color = 0x12;
  global_raw_cards_storage[335].sleight_color = 8;
  global_raw_cards_storage[375].sleight_color = 0x20;
  global_raw_cards_storage[686].sleight_color = 0x10;
  global_raw_cards_storage[621].sleight_color = 0x10;
  global_raw_cards_storage[345].sleight_color = 0x20;

  SET_HACK(612, 4);
  SET_HACK(590, 8);
  SET_HACK(774, 0x20);
  SET_HACK(552, 8);
  SET_HACK(715, 0x10);
  SET_HACK(940, 4);
  SET_HACK(323, 8);
  SET_HACK(730, 0x10);
  SET_HACK(827, 4);
  SET_HACK(951, 4);
  SET_HACK(315, 0x10);
  SET_HACK(335, 8);
  SET_HACK(727, 2);
  SET_HACK(796, 0x20);
  SET_HACK(385, 4);
  SET_HACK(299, 0x10);
  SET_HACK(621, 4);
  SET_HACK(332, 0x10);
  SET_HACK(389, 0xa);
#endif

#undef SET_HACK

  return global_available_slots;
}

// FUNCTION: DECKDLL 0x1001b2a2
// FUNCTION: SHANDALAR 0x00442f9b
// FUNCTION: MAGIC 0x004537d8
int ReadLegacyCsv(const char *filename)
{
  char *next;
  int result;
  size_t size;
  int card_index;
  char *line;

  result = 0;

  for (card_index = 0; card_index < global_available_slots; ++card_index)
  {
    global_legacy_names[card_index].damage_text = "";
    global_legacy_names[card_index].effect_title = "";
    global_legacy_names[card_index].effect_text = "";
    global_legacy_names[card_index].legacy_title = "";
    global_legacy_names[card_index].legacy_text = "";
  }

  global_rarities_csv_raw = read_whole_file(filename, &size);
  if (global_rarities_csv_raw != NULL)
  {
    line = global_rarities_csv_raw;
    line = strchr(line, '\n') + 1;

    for (card_index = 0; card_index < global_available_slots; ++card_index)
    {
      next = line;

      next = CsvParseNextField(&line);
      line = next;

      next = CsvParseNextField(&line);
      line = next;

      next = CsvParseNextField(&line);
      global_legacy_names[card_index].damage_text = line;
      line = next;

      next = CsvParseNextField(&line);
      global_legacy_names[card_index].effect_title = line;
      line = next;

      next = CsvParseNextField(&line);
      global_legacy_names[card_index].effect_text = line;
      line = next;

      next = CsvParseNextField(&line);
      global_legacy_names[card_index].legacy_title = line;
      line = next;

      next = CsvParseNextField(&line);
      global_legacy_names[card_index].legacy_text = line;
      line = next;
    }

    result = 1;
  }

  (void)size;
  return result;
}

// FUNCTION: SHANDALAR 0x00443209
// FUNCTION: DECKDLL 0x1001b510
// FUNCTION: MAGIC 0x00453a46
char *CsvParseNextField(char **txt)
{
  char *cr;
  char *next;
  char *comma;
  char *p;
  char *field;

  field = *txt;

  if (*field == 0x22)
  {
    field += 1;
    p = strchr(field, 0x22);
    *p = '\0';
    p++;
    if (*p == 0x2c)
      next = p + 1;
    else
      next = p + 2;
  }
  else
  {
    comma = strchr(field, 0x2c);
    cr = strchr(field, 0xd);
    if (comma != NULL && cr > comma)
    {
      *comma = '\0';
      next = comma + 1;
    }
    else
    {
      *cr = '\0';
      next = cr + 2;
    }
  }

  *txt = field;
  return next;
}

// FUNCTION: DECKDLL 0x1001b4df
// FUNCTION: MAGIC 0x00453a15
// FUNCTION: SHANDALAR 0x004431d8
void FreeRaritiesCsvRaw(void)
{
  if (global_rarities_csv_raw != NULL)
  {
    free(global_rarities_csv_raw);
  }
  global_rarities_csv_raw = NULL;
}

// FUNCTION: DECKDLL 0x1001b832
// FUNCTION: MAGIC 0x00453d68
int ReadDeckbuilderCsv(const char *filename, OrigRarities *orig_rarities)
{
  int rval;         /* ebp - 0x88 */
  size_t size;      /* ebp - 0x80 */
  char *next;       /* ebp - 0x7c */
  char set_id;      /* ebp - 0x78 */
  int i;            /* ebp - 0x74 */
  char *line;       /* ebp - 0x70 */

  char set_names[7][15] = {
      "Antiquities",
      "Arabian",
      "Astral",
      "Dark",
      "Legends",
      "Promo",
      "Unlimited",
  };

  rval = 0;
  for (i = 0; i < global_available_slots; i++)
  {
    orig_rarities[i].set = SET_INVALID;
  }

  global_rarities_csv_raw = read_whole_file(filename, &size);
  if (global_rarities_csv_raw)
  {
    line = global_rarities_csv_raw;
    line = strchr(line, '\n') + 1;
    line = strchr(line, '\n') + 1;

    for (i = 0; i < global_available_slots; i = i + 1)
    {
      next = line;
      next = CsvParseNextField(&line);
      line = next;
      next = CsvParseNextField(&line);
      line = next;
      next = CsvParseNextField(&line);

      for (set_id = '\0'; set_id < '\a'; set_id++)
      {
        if (!strcmp(set_names[set_id], line))
          break;
      }

      if (set_id != '\a')
      {
        orig_rarities[i].set = set_id;

        line = next;
        next = CsvParseNextField(&line);
        orig_rarities[i].rarity = *line;

        line = next;
        next = CsvParseNextField(&line);
        orig_rarities[i].exp_rarities[0] = *line;

        line = next;
        next = CsvParseNextField(&line);
        orig_rarities[i].exp_rarities[1] = *line;

        line = next;
        next = CsvParseNextField(&line);
        orig_rarities[i].exp_rarities[2] = *line;

        line = next;
        next = CsvParseNextField(&line);
        orig_rarities[i].exp_rarities[3] = *line;
      }
      else
      {
        orig_rarities[i].set = SET_INVALID;
        orig_rarities[i].exp_rarities[0] = '-';
        orig_rarities[i].exp_rarities[1] = '-';
        orig_rarities[i].exp_rarities[2] = '-';
        orig_rarities[i].exp_rarities[3] = '-';
      }

      line = next;
      line = strchr(line, '\n') + 1;
    }

    rval = 1;
  }

  (void)size;
  return rval;
}
