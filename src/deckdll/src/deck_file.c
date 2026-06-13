#include "deck_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// FUNCTION: DECKDLL 0x1000edea
static int read_deck_line(FILE *file, char *dest)
{
  int len;
  int ch;

  for (len = 0; (ch = fgetc(file)) != '\n' && ch != -1;)
  {
    *dest = (char)ch;
    dest++;
    len++;
  }

  *dest = 0;
  if (ch == -1)
    return -1;
  else
    return len;
}

// FUNCTION: DECKDLL 0x1000ee5d
static int is_deck_card_line(char *line)
{
  if (*line == '.' && line[1] != 'v')
    return 1;
  return 0;
}

// FUNCTION: DECKDLL 0x1000ee98
static int is_deck_version_line(char *line)
{
  if (*line == 'v')
    return 1;
  if (*line == '.' && line[1] == 'v')
    return 1;
  return 0;
}

static int read_metadata_line(FILE *file, char *buffer, int size)
{
  size_t len;
  if (!fgets(buffer, size, file))
    return 0;
  if (buffer[0] != ';')
    return 0;
  len = strlen(buffer);
  if (len > 0 && buffer[len - 1] == '\n')
    buffer[len - 1] = '\0';
  return 1;
}

int deck_file_save(const char *filename, const DeckFileEntry *entries, int entry_count)
{
  FILE *f;
  int i;

  f = fopen(filename, "wt");
  if (f == NULL)
    return 0;

  fprintf(f, ";New Deck\n");
  fprintf(f, ";Saved by SDL deck core\n");
  fprintf(f, ";\n");
  fprintf(f, ";\n");
  fprintf(f, ";2026-06-13\n");
  fprintf(f, ";1\n");
  fprintf(f, ";SDL\n");
  fprintf(f, ";Portable deck scaffold\n");
  fprintf(f, "\n");

  for (i = 0; i < entry_count; ++i)
    fprintf(f, ".%d\t%d\t%s\n",
            entries[i].csvid,
            entries[i].amount,
            entries[i].name ? entries[i].name : "");

  fclose(f);
  return 1;
}

int deck_file_load(const char *filename, DeckFileAddCardFn add_card, void *userdata, int *out_card_count)
{
  FILE *f;
  char txt[500];
  int num;
  int csvid;
  int loaded_cards = 0;
  int i;

  f = fopen(filename, "rt");
  if (f == NULL)
    return 0;

  for (i = 0; i < 8; ++i) {
    if (!read_metadata_line(f, txt, sizeof(txt))) {
      fclose(f);
      return 0;
    }
  }

  while ((int)read_deck_line(f, txt) != -1 && is_deck_version_line(txt) == 0)
  {
    if (is_deck_card_line(txt))
    {
      if (sscanf(txt, ".%d %d", &csvid, &num) == 2 && num > 0)
      {
        if (add_card && add_card(csvid, num, userdata))
          loaded_cards += num;
      }
    }
  }

  fclose(f);
  if (out_card_count)
    *out_card_count = loaded_cards;
  return 1;
}
