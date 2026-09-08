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

/* CARDS.DAT always contains the original 32-bit, 0x98-byte record layout.
 * Pointer-like fields in this structure are offsets into the text block that
 * follows the records (artist is an index into const_db_artist_names). */
typedef struct card_disk_record_t
{
  int32_t id;
  uint32_t full_name_offset;
  uint32_t name_offset;
  uint32_t expansion;
  uint32_t color;
  uint32_t card_type;
  uint32_t subtype;
  uint32_t type_text_offset;
  uint32_t db_card_type_2;
  uint32_t rarity;
  casting_cost_t req;
  uint8_t unused0x31;
  uint16_t unused0x32;
  uint32_t flags;
  uint32_t unused0x38;
  uint32_t unused0x3c;
  uint32_t artist_index;
  int32_t num_pics;
  uint32_t mana_cost_text_offset;
  uint32_t ai_modifiers1;
  uint32_t ai_modifiers2;
  uint16_t ai_inc_power_toughness;
  uint16_t ai_power_toughness;
  int16_t ai_base_value;
  int16_t ai_dependencies;
  int32_t sleight_color;
  int16_t ai_against_color;
  int16_t ai_for_color;
  int16_t ai_counts_as_color;
  int16_t ai_against_land;
  int16_t ai_for_land;
  int16_t ai_counts_as_land;
  int32_t ai_abilities;
  int32_t expansion_rarity;
  uint32_t rules_text_offset;
  uint32_t flavor_text_offset;
  int32_t power;
  int32_t toughness;
  int16_t types[2];
  char abilities[10];
  int8_t mana_source_colors;
  int8_t inflatable;
  uint8_t hack_colors;
  uint8_t currently_zero_for_all_cards;
  uint8_t enchant_type;
  uint8_t hack_mode;
} card_disk_record_t;

STATIC_ASSERT(sizeof(card_disk_record_t) == 0x98, card_disk_record_must_match_cards_dat);

static char *resolve_card_text(uint32_t offset, size_t text_size)
{
  char *text;
  if (offset >= text_size)
    return NULL;
  text = global_base_txt + offset;
  return memchr(text, '\0', text_size - offset) != NULL ? text : NULL;
}

static int convert_card_disk_record(card_ptr_t *dst, const card_disk_record_t *src,
                                    size_t text_size)
{
  memset(dst, 0, sizeof(*dst));
  dst->id = (card_id_t)src->id;
  dst->full_name = resolve_card_text(src->full_name_offset, text_size);
  dst->name = resolve_card_text(src->name_offset, text_size);
  dst->type_text = resolve_card_text(src->type_text_offset, text_size);
  dst->rules_text = resolve_card_text(src->rules_text_offset, text_size);
  dst->flavor_text = resolve_card_text(src->flavor_text_offset, text_size);
  if (!dst->full_name || !dst->name || !dst->type_text || !dst->rules_text ||
      !dst->flavor_text ||
      src->artist_index >= sizeof(const_db_artist_names) / sizeof(const_db_artist_names[0]))
    return 0;

  dst->expansion = src->expansion;
  dst->color = src->color;
  dst->card_type = src->card_type;
  dst->subtype = src->subtype;
  dst->db_card_type_2 = src->db_card_type_2;
  dst->rarity = src->rarity;
  dst->req = src->req;
  dst->unused0x31 = src->unused0x31;
  dst->unused0x32 = src->unused0x32;
  dst->flags = (CardPtrFlags)src->flags;
  dst->unused0x38 = src->unused0x38;
  dst->unused0x3c = src->unused0x3c;
  dst->artist = const_db_artist_names[src->artist_index];
  dst->num_pics = src->num_pics;
  dst->mana_cost_text = src->mana_cost_text_offset
                            ? resolve_card_text(src->mana_cost_text_offset, text_size)
                            : NULL;
  if (src->mana_cost_text_offset && !dst->mana_cost_text)
    return 0;
  dst->ai_modifiers1 = src->ai_modifiers1;
  dst->ai_modifiers2 = src->ai_modifiers2;
  dst->ai_inc_power_toughness = src->ai_inc_power_toughness;
  dst->ai_power_toughness = src->ai_power_toughness;
  dst->ai_base_value = src->ai_base_value;
  dst->ai_dependencies = src->ai_dependencies;
  dst->sleight_color = src->sleight_color;
  dst->ai_against_color = src->ai_against_color;
  dst->ai_for_color = src->ai_for_color;
  dst->ai_counts_as_color = src->ai_counts_as_color;
  dst->ai_against_land = src->ai_against_land;
  dst->ai_for_land = src->ai_for_land;
  dst->ai_counts_as_land = src->ai_counts_as_land;
  dst->ai_abilities = src->ai_abilities;
  dst->expansion_rarity = src->expansion_rarity;
  dst->power = src->power;
  dst->toughness = src->toughness;
  memcpy(dst->types, src->types, sizeof(dst->types));
  memcpy(dst->abilities, src->abilities, sizeof(dst->abilities));
  dst->mana_source_colors = src->mana_source_colors;
  dst->inflatable = src->inflatable;
  dst->hack_colors = src->hack_colors;
  dst->currently_zero_for_all_cards = src->currently_zero_for_all_cards;
  dst->enchant_type = src->enchant_type;
  dst->hack_mode = src->hack_mode;
  return 1;
}

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
  uint32_t text_size;
  card_disk_record_t *disk_records;
  int card_idx; /* ebp - 0x10 */
  FILE *cards_dat;

  cards_dat = fopen(cards_dat_filename, "rb");
  if (cards_dat == NULL)
    return 0;

  if (fread(&global_available_slots, sizeof(uint32_t), 1, cards_dat) != 1 ||
      fread(&text_size, sizeof(text_size), 1, cards_dat) != 1 ||
      global_available_slots < 0 || global_available_slots > 2000 || text_size == 0)
  {
    fclose(cards_dat);
    return 0;
  }

  disk_records = malloc((size_t)global_available_slots * sizeof(*disk_records));
  global_base_txt = malloc((size_t)text_size + 1);
  if (disk_records == NULL || global_base_txt == NULL)
  {
    free(disk_records);
    free(global_base_txt);
    global_base_txt = NULL;
    fclose(cards_dat);
    return 0;
  }

  if (fread(disk_records, sizeof(*disk_records), (size_t)global_available_slots, cards_dat) !=
          (size_t)global_available_slots ||
      fread(global_base_txt, 1, text_size, cards_dat) != text_size)
  {
    free(disk_records);
    free(global_base_txt);
    global_base_txt = NULL;
    fclose(cards_dat);
    return 0;
  }
  fclose(cards_dat);
  global_base_txt[text_size] = '\0';

  for (card_idx = 0; card_idx < global_available_slots; card_idx += 1)
  {
    if (!convert_card_disk_record(&global_raw_cards_storage[card_idx],
                                  &disk_records[card_idx], text_size))
    {
      free(disk_records);
      free(global_base_txt);
      global_base_txt = NULL;
      return 0;
    }

    if (_strcmpi(global_raw_cards_storage[card_idx].rules_text, "None") == 0)
      global_raw_cards_storage[card_idx].rules_text = read_db_empty_rules;

    if ((_strcmpi(global_raw_cards_storage[card_idx].flavor_text, "None") == 0) ||
        (_strcmpi(global_raw_cards_storage[card_idx].flavor_text, "Blank") == 0))
      global_raw_cards_storage[card_idx].flavor_text = read_db_empty_flavor;

    if (global_raw_cards_storage[card_idx].num_pics == 0)
      global_raw_cards_storage[card_idx].num_pics = 1;
  }
  free(disk_records);

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
