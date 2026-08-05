#include <stdio.h>
#include <string.h>
#include "game_support.h"
#include "global_strings.h"

char *get_displayed_card_name(int player, int card);

// FUNCTION: MAGIC 0x00443898
// FUNCTION: SHANDALAR 0x0040f132
unsigned int get_current_stack_action(void)
{
  if (g_stack_size == 0)
  {
    return 0xffffffffU;
  }

  return (unsigned int)g_duel_state_table_0093917c[g_stack_size];
}

// FUNCTION: MAGIC 0x00444c43
// FUNCTION: SHANDALAR 0x004104db
char *format_stack_action_text(char *out, int msg, int player, int card)
{
  char tmp[300];
  char *prefix;

  if (out == (char *)0)
  {
    return (char *)0;
  }

  if (msg == 0x71)
  {
    prefix = gs_casting_008b4850;
  }
  else if (msg == 0x72)
  {
    prefix = gs_activating_007912e0;
  }
  else if (msg == 0x7e)
  {
    prefix = gs_processing_008ce290;
  }
  else
  {
    prefix = "";
  }

  if (player >= 0 && card >= 0)
  {
    sprintf(tmp, "%s: %s", prefix, get_displayed_card_name(player, card));
  }
  else
  {
    strcpy(tmp, prefix);
  }

  strcpy(out, tmp);
  return out;
}
