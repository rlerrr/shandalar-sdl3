#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "cardartlib/src/palette.h"
#include "drawcardlib/Drawcardlib.h"
#include "deckdll/src/card_db.h"
#include "game_support.h"
#include "global_state.h"
#include "global_strings.h"
#include "duel_engine.h"

typedef ptrdiff_t INT_PTR;
typedef struct
{
  int bigcard_player;
  int bigcard_card;
  int smallcard_player;
  int smallcard_card;
  char *prompt;
  int dialog_mode;
} dialog_context_t;

typedef struct
{
  int player;
  int card;
  target_t targets[20];
  int number_of_targets;
} dialog_stack_entry_t;
STATIC_ASSERT(sizeof(dialog_stack_entry_t) == 0xac, dialog_stack_entry_t_wrong_size);

#define DISPLAYED_PLAYER_CARD_INSTANCE(player_, card_) global_displayed_card_instances[(player_)][(card_)]
#define DISPLAYED_PLAYER_CARD_ROW(player_) global_displayed_card_instances[(player_)]

extern card_ptr_t global_raw_cards_storage[2000];
extern int g_showlist_smallcard_width;
extern int g_showlist_smallcard_height;

// GLOBAL: MAGIC 0x006f6df8
// GLOBAL: SHANDALAR 0x005b7658
RECT g_big_card_choice_full_card_rect;
// GLOBAL: MAGIC 0x006f6e08
// GLOBAL: SHANDALAR 0x005b7668
dialog_context_t *g_big_card_choice_dialog_context;
// GLOBAL: MAGIC 0x006f6e10
// GLOBAL: SHANDALAR 0x005b7670
RECT g_big_card_choice_small_card_rect;
// GLOBAL: MAGIC 0x006f6e20
// GLOBAL: SHANDALAR 0x005b7680
int g_big_card_choice_state_006f6e20;
// GLOBAL: MAGIC 0x006f6e28
// GLOBAL: SHANDALAR 0x005b7688
char g_big_card_choice_window_text_buffer[0xc8];
// GLOBAL: MAGIC 0x008950b4
// GLOBAL: SHANDALAR 0x008a92b4
int g_big_card_choice_timeout_ms;

// GLOBAL: MAGIC 0x00777848
// GLOBAL: SHANDALAR 0x0078e5d4
int g_duel_cached_response_state;
// GLOBAL: MAGIC 0x007ab14c
// GLOBAL: SHANDALAR 0x007bf34c
int g_duel_cached_state_007abce0;
// GLOBAL: MAGIC 0x007ab430
// GLOBAL: SHANDALAR 0x007bf630
int g_duel_cached_graveyard_player_0[500];
// GLOBAL: MAGIC 0x008a8c18
// GLOBAL: SHANDALAR 0x008bce18
int g_duel_cached_unk_008b44d0_player_1;
// GLOBAL: MAGIC 0x008a99f8
// GLOBAL: SHANDALAR 0x008bdbf8
int g_duel_cached_active_cards_count_player_0;
// GLOBAL: MAGIC 0x008a99fc
// GLOBAL: SHANDALAR 0x008bdbfc
int g_duel_cached_active_cards_count_player_1;
// GLOBAL: MAGIC 0x008b1140
// GLOBAL: SHANDALAR 0x008c5340
dialog_stack_entry_t g_duel_cached_stack_entries[32];
// GLOBAL: MAGIC 0x008b2940
// GLOBAL: SHANDALAR 0x008c6af0
int g_duel_cached_life_player_0;
// GLOBAL: MAGIC 0x008b3400
// GLOBAL: SHANDALAR 0x008c75a0
int g_duel_cached_phase_stops_player_1[0x26];
// GLOBAL: MAGIC 0x008b5140
// GLOBAL: SHANDALAR 0x008c92c0
int g_duel_cached_exile_player_1[500];
// GLOBAL: MAGIC 0x008cdac0
// GLOBAL: SHANDALAR 0x008e1c40
int g_duel_cached_library_player_0[500];
// GLOBAL: MAGIC 0x008ce4f8
// GLOBAL: SHANDALAR 0x008e2678
int g_duel_cached_library_count_player_0;
// GLOBAL: MAGIC 0x008ce500
// GLOBAL: SHANDALAR 0x008e2680
int g_duel_cached_exile_count_player_1;
// GLOBAL: MAGIC 0x008cefb4
// GLOBAL: SHANDALAR 0x008e3100
int g_duel_cached_library_count_player_1;
// GLOBAL: MAGIC 0x008966d0
// GLOBAL: SHANDALAR 0x008aa8d0
int g_duel_cached_ante_count_player_1;
// GLOBAL: MAGIC 0x0091a940
// GLOBAL: SHANDALAR 0x0092ea90
int g_duel_cached_graveyard_player_1[500];
// GLOBAL: MAGIC 0x0091b400
// GLOBAL: SHANDALAR 0x0092f550
int g_duel_cached_library_player_1[500];
// GLOBAL: MAGIC 0x0091c4f4
// GLOBAL: SHANDALAR 0x00930634
int g_duel_cached_unk_008b44d0_player_0;
// GLOBAL: MAGIC 0x00924820
// GLOBAL: SHANDALAR 0x00938950
int g_duel_cached_exile_player_0[500];
// GLOBAL: MAGIC 0x00924ff0
// GLOBAL: SHANDALAR 0x00939120
int g_duel_cached_ante_player_1[16];
// GLOBAL: MAGIC 0x00925d34
// GLOBAL: SHANDALAR 0x00939e60
int g_duel_cached_life_player_1;
// GLOBAL: MAGIC 0x00925f9c
// GLOBAL: SHANDALAR 0x0093a0cc
int g_duel_cached_graveyard_count_player_0;
// GLOBAL: MAGIC 0x00925360
// GLOBAL: SHANDALAR 0x00939490
int g_duel_cached_phase_stops_player_0[0x26];
// GLOBAL: MAGIC 0x009266a8
// GLOBAL: SHANDALAR 0x0093a7d8
int g_duel_cached_exile_count_player_0;
// GLOBAL: MAGIC 0x009397d0
// GLOBAL: SHANDALAR 0x0094d8f0
int g_duel_cached_ante_player_0[16];
// GLOBAL: MAGIC 0x0093a7f4
// GLOBAL: SHANDALAR 0x0094e914
int g_duel_cached_graveyard_count_player_1;
// GLOBAL: MAGIC 0x0093a97c
// GLOBAL: SHANDALAR 0x0094ea9c
int g_duel_cached_state_007abce4;
// GLOBAL: MAGIC 0x0093d84c
// GLOBAL: SHANDALAR 0x0095196c
int g_duel_cached_ante_count_player_0;
// GLOBAL: MAGIC 0x00708da8
char g_card_rules_text_buffer_00708da8[0x190];
// GLOBAL: MAGIC 0x00708f48
// GLOBAL: SHANDALAR 0x007325c0
char g_activation_rules_text_buffer_00708f48[0x190];
// GLOBAL: MAGIC 0x00709100
char g_card_title_buffer_00709100[0x34];
// GLOBAL: MAGIC 0x00709150
// GLOBAL: SHANDALAR 0x007327c8
char g_activation_card_title_buffer_00709150[0x34];

// GLOBAL: MAGIC 0x008b4e44
char g_dialog_saved_player_name[50];
// GLOBAL: MAGIC 0x008b4e76
char g_dialog_saved_opponent_name[50];
// GLOBAL: MAGIC 0x008b4ea8
char g_dialog_saved_deck_name[50];

unsigned int refresh_duel_display_cache(void);
unsigned int get_displayed_card_original_internal_id(int player, int card);
unsigned int get_displayed_card_display_pic_info(int *displayed_player_and_card, int player, int card);
unsigned int get_displayed_card_special_counters(int player, int card);
int get_displayed_card_color_flags(int player, int card);
int get_displayed_card_eot_toughness(int player, int card);
void get_displayed_card_counter_power_toughness(int player, int card, int *power, int *toughness);
void apply_displayed_card_sleight_words(int player, int card, char *text);
void apply_displayed_card_hack_words(int player, int card, char *text);
unsigned int get_displayed_card_display_flags(int player, int card);
unsigned int get_displayed_card_base_internal_id(int player, int card);
void get_displayed_card_parent(int *displayed_player_and_card, int player, int card);
void replace_text_token(char *text, char *search, int case_sensitive, char *replace);
void replace_hack_color_words(char *text, color_t source_color, color_t target_color, int parenthesize);
void replace_sleight_color_words(char *text, color_t source_color, int target_color, int parenthesize);
int get_card_display_pic_num(card_id_t card_id, int player, int card);
unsigned int draw_displayed_full_card(HDC dc, RECT *rect, card_ptr_t *raw_card, int player, int card, int draw_mode, int expand_text_box);
void draw_displayed_card_overlaid_full_card(HDC dc, RECT *rect, int player, int card);
void draw_special_effect_full_card(HDC dc, RECT *rect, card_id_t card_id, int player, int card);
int get_original_internal_card_id(int player, int card);
int get_card_internal_id(int player, int card);
card_id_t get_card_id(int player, int card);
INT_PTR CALLBACK big_card_choice_dialog_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
int raw_do_dialog(int bigcard_player,
                  int bigcard_card,
                  int smallcard_player,
                  int smallcard_card,
                  char *prompt,
                  int dialog_mode);

// FUNCTION: MAGIC 0x0044a3bf
// FUNCTION: SHANDALAR 0x00453f42
char *get_displayed_card_name(int player, int card)
{
  card_id_t transformed_card_id;
  char temporary_name[52];
  int source_card_data;
  int source_player;
  unsigned int choice;
  char *card_name;
  unsigned int type;
  int csvid;

  if (PLAYER_CARD_INSTANCE(player, card).internal_card_id != -1)
  {
    type = PLAYER_CARD_INSTANCE(player, card).internal_card_id;
  }
  else
  {
    type = PLAYER_CARD_INSTANCE(player, card).original_internal_card_id;
  }

  csvid = CardIDFromType(type);
  card_name = "";
  if (csvid != -1)
  {
    if (csvid == g_activation_display_internal_card_id)
    {
      csvid = CardIDFromType(get_original_internal_card_id(player, card));
    }

    choice = (unsigned int)PLAYER_CARD_INSTANCE(player, card).display_pic_csv_id;
    source_player = (int)(char)PLAYER_CARD_INSTANCE(player, card).damage_source_player;
    source_card_data = PLAYER_CARD_INSTANCE(player, card).damage_source_card;

    if (csvid == g_damage_display_internal_card_id)
    {
      strcpy(g_dialog_card_title_buffer, gs_cardtitle_damage_008cfd30);
    }
    else if (csvid == g_hunting_display_internal_card_id)
    {
      sprintf(g_dialog_card_title_buffer, gs_cardtitle_hunting_00926750, get_hunting_subtype_name(PLAYER_CARD_INSTANCE(player, card).info_slot));
    }
    else if (csvid == g_multiblock_display_internal_card_id)
    {
      strcpy(g_dialog_card_title_buffer, gs_multiblock_creature_008cf040);
    }
    else if (csvid == g_effect_display_internal_card_id)
    {
      strcpy(g_dialog_card_title_buffer, global_legacy_names[choice].effect_title);
    }
    else if (csvid == g_legacy_display_internal_card_id)
    {
      strcpy(g_dialog_card_title_buffer, global_legacy_names[choice].legacy_title);
    }
    else if (csvid == g_card_back_display_internal_card_id)
    {
      strcpy(g_dialog_card_title_buffer, gs_cardtitle_draw_a_card_008b4330);
    }
    else
    {
      g_dialog_card_title_buffer[0] = '\0';
    }

    if (csvid == g_effect_display_internal_card_id && 0 < (int)PLAYER_CARD_INSTANCE(player, card).eot_toughness)
    {
      strcpy(temporary_name, g_dialog_card_title_buffer);
      extract_numbered_text_choice(g_dialog_card_title_buffer, temporary_name, PLAYER_CARD_INSTANCE(player, card).eot_toughness);
    }

    transformed_card_id = get_card_id(source_player, source_card_data);
    if (transformed_card_id == CARD_ID_FAERIE_DRAGON ||
        transformed_card_id == CARD_ID_WHIMSY)
    {
      strcpy(g_dialog_card_title_buffer, global_legacy_names[transformed_card_id].effect_title);
    }

    if (g_dialog_card_title_buffer[0] == '\0')
    {
      card_name = global_raw_cards_storage[csvid].full_name;
    }
    else
    {
      card_name = g_dialog_card_title_buffer;
    }
  }

  return card_name;
}

// FUNCTION: MAGIC 0x0044a796
// FUNCTION: SHANDALAR 0x00454311
void append_displayed_card_name(int player, int card)
{
  char *card_name;

  card_name = get_displayed_card_name(player, card);
  if (card_name != NULL)
  {
    strcat(g_ui_message_buffer, card_name);
  }
}

// FUNCTION: MAGIC 0x004483be
// FUNCTION: SHANDALAR 0x00451f3b
int displayed_card_indices_invalid(int player, int card)
{
  if (player != 0 && player != 1)
  {
    return 1;
  }

  if (card < 0 || card > 0x96)
  {
    return 1;
  }

  return 0;
}

// FUNCTION: MAGIC 0x004486de
// FUNCTION: SHANDALAR 0x0045225b
int get_displayed_card_blocking(int player, int card)
{
  int blocking;

  if (displayed_card_indices_invalid(player, card) != 0)
  {
    return 0;
  }

  EnterCriticalSection(&g_duel_render_lock);
  blocking = (int)(char)DISPLAYED_PLAYER_CARD_INSTANCE(player, card).blocking;
  LeaveCriticalSection(&g_duel_render_lock);
  return blocking;
}

// FUNCTION: MAGIC 0x004487d8
// FUNCTION: SHANDALAR 0x00452355
int get_displayed_card_internal_id(int player, int card)
{
  int internal_card_id;

  if (displayed_card_indices_invalid(player, card) != 0)
  {
    return -1;
  }

  EnterCriticalSection(&g_duel_render_lock);
  internal_card_id = DISPLAYED_PLAYER_CARD_INSTANCE(player, card).internal_card_id;
  LeaveCriticalSection(&g_duel_render_lock);
  return internal_card_id;
}

// FUNCTION: MAGIC 0x00448857
// FUNCTION: SHANDALAR 0x004523d4
card_id_t get_displayed_card_id(int player, int card)
{
  int internal_card_id;

  if (displayed_card_indices_invalid(player, card) != 0)
  {
    return -1;
  }

  internal_card_id = get_displayed_card_internal_id(player, card);
  if (internal_card_id == -1)
  {
    return -1;
  }
  else
    return global_cards_data[internal_card_id].id;
}

// FUNCTION: MAGIC 0x00449057
// FUNCTION: SHANDALAR 0x00452bd4
int get_displayed_card_color_flags(int player, int card)
{
  int displayed_color;

  if (displayed_card_indices_invalid(player, card) != 0)
  {
    return 0;
  }

  EnterCriticalSection(&g_duel_render_lock);
  displayed_color = (int)(char)DISPLAYED_PLAYER_CARD_INSTANCE(player, card).color;
  LeaveCriticalSection(&g_duel_render_lock);

  return displayed_color;
}

// FUNCTION: MAGIC 0x00449645
// FUNCTION: SHANDALAR 0x004531c2
void apply_displayed_card_sleight_words(int player, int card, char *text)
{
  int color_index;

  EnterCriticalSection(&g_duel_render_lock);
  for (color_index = 1; color_index < 6; ++color_index)
  {
    if (DISPLAYED_PLAYER_CARD_INSTANCE(player, card).color_id[color_index] != '\0')
    {
      replace_sleight_color_words(text, color_index, (int)DISPLAYED_PLAYER_CARD_INSTANCE(player, card).color_id[color_index], 0);
    }
  }
  LeaveCriticalSection(&g_duel_render_lock);
}

// FUNCTION: MAGIC 0x00449706
// FUNCTION: SHANDALAR 0x00453287
void apply_displayed_card_hack_words(int player, int card, char *text)
{
  int color_index;

  EnterCriticalSection(&g_duel_render_lock);
  for (color_index = 1; color_index < 6; ++color_index)
  {
    if (DISPLAYED_PLAYER_CARD_INSTANCE(player, card).hack_mode[color_index] != '\0')
    {
      replace_hack_color_words(text, color_index, (int)DISPLAYED_PLAYER_CARD_INSTANCE(player, card).hack_mode[color_index], 0);
    }
  }
  LeaveCriticalSection(&g_duel_render_lock);
}

// FUNCTION: MAGIC 0x00449990
// FUNCTION: SHANDALAR 0x00453511
unsigned int get_displayed_card_base_internal_id(int player, int card)
{
  int result;

  if (displayed_card_indices_invalid(player, card) != 0)
    return -1;

  EnterCriticalSection(&g_duel_render_lock);
  result = DISPLAYED_PLAYER_CARD_INSTANCE(player, card).original_internal_card_id;
  LeaveCriticalSection(&g_duel_render_lock);
  return result;
}

// FUNCTION: MAGIC 0x00449a0f
// FUNCTION: SHANDALAR 0x00453590
void get_displayed_card_parent(int *displayed_player_and_card, int player, int card)
{
  if (displayed_card_indices_invalid(player, card) != 0)
    return;

  if (displayed_player_and_card == NULL)
    return;

  EnterCriticalSection(&g_duel_render_lock);
  displayed_player_and_card[0] = DISPLAYED_PLAYER_CARD_INSTANCE(player, card).parent_controller;
  displayed_player_and_card[1] = DISPLAYED_PLAYER_CARD_INSTANCE(player, card).parent_card;
  LeaveCriticalSection(&g_duel_render_lock);
}

// FUNCTION: MAGIC 0x00448c93
// FUNCTION: SHANDALAR 0x00452810
unsigned int get_displayed_card_display_pic_info(int *displayed_player_and_card, int player, int card)
{
  unsigned int display_info;

  if (displayed_card_indices_invalid(player, card) != 0)
  {
    if (displayed_player_and_card != NULL)
    {
      displayed_player_and_card[0] = (int)(char)DISPLAYED_PLAYER_CARD_ROW(player)[card].damage_source_player;
      displayed_player_and_card[1] = DISPLAYED_PLAYER_CARD_ROW(player)[card].damage_source_card;
    }
    return 0xffffffff;
  }

  EnterCriticalSection(&g_duel_render_lock);
  if (displayed_player_and_card != NULL)
  {
    displayed_player_and_card[0] = (int)(char)DISPLAYED_PLAYER_CARD_ROW(player)[card].damage_source_player;
    displayed_player_and_card[1] = DISPLAYED_PLAYER_CARD_ROW(player)[card].damage_source_card;
  }
  display_info = DISPLAYED_PLAYER_CARD_ROW(player)[card].display_pic_info;
  LeaveCriticalSection(&g_duel_render_lock);

  return display_info;
}

// FUNCTION: MAGIC 0x00448490
// FUNCTION: SHANDALAR 0x0045200d
unsigned int get_displayed_card_special_counters(int player, int card)
{
  unsigned int info_slot;

  if (displayed_card_indices_invalid(player, card) != 0)
  {
    return 0;
  }

  EnterCriticalSection(&g_duel_render_lock);
  info_slot = DISPLAYED_PLAYER_CARD_INSTANCE(player, card).info_slot;
  LeaveCriticalSection(&g_duel_render_lock);

  return info_slot;
}

// FUNCTION: MAGIC 0x004491cd
// FUNCTION: SHANDALAR 0x00452d4a
int get_displayed_card_eot_toughness(int player, int card)
{
  int displayed_value;

  if (displayed_card_indices_invalid(player, card) != 0)
  {
    return 0;
  }

  EnterCriticalSection(&g_duel_render_lock);
  displayed_value = DISPLAYED_PLAYER_CARD_INSTANCE(player, card).eot_toughness;
  LeaveCriticalSection(&g_duel_render_lock);

  return displayed_value;
}

// FUNCTION: MAGIC 0x00449249
// FUNCTION: SHANDALAR 0x00452dc6
void get_displayed_card_counter_power_toughness(int player, int card, int *power, int *toughness)
{
  if (power == NULL || toughness == NULL)
  {
  }
  else if (displayed_card_indices_invalid(player, card))
  {
  }
  else
  {
    EnterCriticalSection(&g_duel_render_lock);
    *power = DISPLAYED_PLAYER_CARD_ROW(player)[card].counter_power;
    *toughness = DISPLAYED_PLAYER_CARD_ROW(player)[card].counter_toughness;
    LeaveCriticalSection(&g_duel_render_lock);
  }
}

// FUNCTION: MAGIC 0x00449898
// FUNCTION: SHANDALAR 0x00453419
unsigned int get_displayed_card_display_flags(int player, int card)
{
  unsigned int displayed_flags;

  if (displayed_card_indices_invalid(player, card) != 0)
  {
    return 0;
  }

  EnterCriticalSection(&g_duel_render_lock);
  displayed_flags = DISPLAYED_PLAYER_CARD_INSTANCE(player, card).token_status;
  LeaveCriticalSection(&g_duel_render_lock);

  return displayed_flags;
}

// FUNCTION: MAGIC 0x00494e91
// FUNCTION: SHANDALAR 0x0046592b
void replace_text_token(char *text, char *search, int case_sensitive, char *replace)
{
  int write_pos;
  char rewritten_text[1000];
  size_t search_length;
  int found_match;
  char *cursor;

  if (text == NULL || search == NULL || replace == NULL)
  {
  }
  else if (strlen(text) == 0 || strlen(search) == 0)
  {
  }
  else
  {
    search_length = strlen(search);
    cursor = text;
    rewritten_text[0] = '\0';
    write_pos = 0;
    while (*cursor != '\0')
    {
      found_match = 0;
      if ((case_sensitive != 0 && strncmp(cursor, search, search_length) == 0) ||
          (case_sensitive == 0 && _strnicmp(cursor, search, search_length) == 0))
      {
        found_match = 1;
      }

      if (found_match != 0)
      {
        strcat(rewritten_text, replace);
        write_pos += strlen(replace);
        cursor += search_length;
      }
      else
      {
        rewritten_text[write_pos] = *cursor;
        ++cursor;
        ++write_pos;
        rewritten_text[write_pos] = '\0';
      }
    }
    strcpy(text, rewritten_text);
  }
}

// FUNCTION: MAGIC 0x0049511c
// FUNCTION: SHANDALAR 0x00465bb6
void replace_hack_color_words(char *text, color_t source_color, color_t target_color, int parenthesize)
{
  char search_text[100];
  int color_index;
  char replacement_text[100];

  for (color_index = 0; gs_words_to_hack_count_008cd924 > color_index; ++color_index)
  {
    sprintf(search_text, "|H%s", gs_words_to_hack_00789c80[source_color][color_index]);
    if (parenthesize != 0)
    {
      sprintf(replacement_text, "|H%s", gs_words_hacked_0091d260[target_color][color_index]);
    }
    else
    {
      strcpy(replacement_text, gs_words_hacked_0091d260[target_color][color_index]);
    }
    replace_text_token(text, search_text, 1, replacement_text);
  }
}

// FUNCTION: MAGIC 0x00495217
// FUNCTION: SHANDALAR 0x00465cb1
void replace_sleight_color_words(char *text, color_t source_color, int target_color, int parenthesize)
{
  char search_text[100];
  int color_index;
  char replacement_text[100];

  for (color_index = 0; gs_words_to_sleight_count_00789c34 > color_index; ++color_index)
  {
    sprintf(search_text, "|S%s", gs_words_to_sleight_00781bd0[source_color][color_index]);
    if (parenthesize != 0)
    {
      sprintf(replacement_text, "|S%s", gs_words_sleighted_008a9c10[target_color][color_index]);
    }
    else
    {
      strcpy(replacement_text, gs_words_sleighted_008a9c10[target_color][color_index]);
    }
    replace_text_token(text, search_text, 1, replacement_text);
  }
}

// FUNCTION: MAGIC 0x004a583e
// FUNCTION: SHANDALAR 0x005579be
int get_original_internal_card_id(int player, int card)
{
  return PLAYER_CARD_INSTANCE(player, card).original_internal_card_id;
}

// FUNCTION: MAGIC 0x004a587c
// FUNCTION: SHANDALAR 0x005579fc
int get_card_internal_id(int player, int card)
{
  return PLAYER_CARD_INSTANCE(player, card).internal_card_id;
}

// FUNCTION: MAGIC 0x004a58ba
// FUNCTION: SHANDALAR 0x00557a3a
card_id_t get_card_id(int player, int card)
{
  if (player == -1 || card == -1)
  {
    return -1;
  }
  else
  {
    int internal_card_id = get_card_internal_id(player, card);
    if (internal_card_id == -1)
    {
      return -1;
    }
    else
    {
      return global_cards_data[internal_card_id].id;
    }
  }
}

// FUNCTION: MAGIC 0x004a63b8
// FUNCTION: SHANDALAR 0x00558538
int do_dialog(int who_chooses,
              int bigcard_player,
              int bigcard_card,
              int smallcard_player,
              int smallcard_card,
              const char *options,
              int ai_choice)
{
  int dialog_mode;
  int unused_padding;
  char displayed_options[600];
  int remaining_choice;
  int dialog_result;
  int option_index;
  unsigned int at_start_of_line;

  if (who_chooses == g_other_player && (g_duel_network_flags & 2) != 0)
  {
    TENTATIVE_wait_for_network_result(who_chooses, 0xd);
    ai_choice = g_dialog_result_network_packet.result;
  }

  if (g_duel_ai_mode_state == 1)
  {
    return ai_choice;
  }

  if (who_chooses < 0 || bigcard_player < 0 || bigcard_card < 0)
  {
    return ai_choice;
  }

  snprintf(displayed_options, sizeof(displayed_options), "%s", options);
  strcpy(g_ui_message_buffer, "");
  if (who_chooses == g_other_player)
  {
    sprintf(g_ui_message_buffer, gs_prompt_new_full_card_0091b150, g_duel_message_text);
    strcat(g_ui_message_buffer, "\n\n");
    at_start_of_line = 1;
    remaining_choice = ai_choice;
    for (option_index = 0;
         option_index < (int)sizeof(displayed_options) && displayed_options[option_index] != '\0';
         ++option_index)
    {
      if (at_start_of_line != 0 && displayed_options[option_index] == ' ')
      {
        unused_padding = remaining_choice;
        --remaining_choice;
        if (unused_padding == 0)
        {
          displayed_options[option_index] = '>';
          break;
        }
      }

      at_start_of_line = displayed_options[option_index] == '\n';
    }
  }
  else
  {
    append_displayed_card_name(bigcard_player, bigcard_card);
    strcat(g_ui_message_buffer, "...\n");
  }

  strcat(g_ui_message_buffer, displayed_options);
  if (PLAYER_CARD_INSTANCE(bigcard_player, bigcard_card).internal_card_id == -1 ||
      PLAYER_CARD_INSTANCE(smallcard_player, smallcard_card).internal_card_id == -1)
  {
    TENTATIVE_reassess_all_cards(1, 0xff);
  }

  if (who_chooses == g_active_player && g_duel_network_state == 0)
  {
    dialog_mode = 1;
  }
  else
  {
    dialog_mode = 0;
  }

  dialog_result =
      raw_do_dialog(bigcard_player, bigcard_card, smallcard_player, smallcard_card, g_ui_message_buffer, dialog_mode);

  if (who_chooses == g_active_player && (g_duel_network_flags & 2) != 0)
  {
    g_dialog_result_network_packet.packet_type = '\r';
    g_dialog_result_network_packet.result = dialog_result;
    TENTATIVE_send_network_result(who_chooses, 0xd);
  }

  if (who_chooses == g_active_player && g_duel_network_state == 0)
  {
    return dialog_result;
  }
  else
  {
    return ai_choice;
  }
}

// FUNCTION: MAGIC 0x004a55c7
// FUNCTION: SHANDALAR 0x00540f38
int raw_do_dialog(int bigcard_player,
                  int bigcard_card,
                  int smallcard_player,
                  int smallcard_card,
                  char *prompt,
                  int dialog_mode)
{
  dialog_context_t context;
  INT_PTR dialog_result;

  KillTimer((HWND)g_duel_window_hwnd, g_duel_timer_id);
  if (smallcard_card == 0xff)
  {
    smallcard_card = -1;
  }

  if (bigcard_player != -1 && bigcard_card != -1)
  {
    if (get_displayed_card_id(bigcard_player, bigcard_card) == -1)
    {
      refresh_duel_display_cache();
    }
  }

  if (smallcard_player != -1 && smallcard_card != -1)
  {
    if (get_displayed_card_id(smallcard_player, smallcard_card) == -1)
    {
      refresh_duel_display_cache();
    }
  }

  context.bigcard_player = bigcard_player;
  context.bigcard_card = bigcard_card;
  context.smallcard_player = smallcard_player;
  context.smallcard_card = smallcard_card;
  context.prompt = prompt;
  context.dialog_mode = dialog_mode;

  dialog_result = DialogBoxParamA(g_app_instance, (LPCSTR)0xdf, (HWND)g_duel_window_hwnd, big_card_choice_dialog_proc, (LPARAM)&context);
  if (dialog_result == 0)
  {
    return -1;
  }
  else
  {
    return dialog_result - 1;
  }
}

// FUNCTION: MAGIC 0x00446e4b
// FUNCTION: SHANDALAR 0x004509c9
unsigned int refresh_duel_display_cache(void)
{
  int stack_card;
  int stack_player;
  int target_index;
  int zone_index;
  unsigned int needs_refresh;

  EnterCriticalSection(&g_duel_render_lock);
  needs_refresh = memcmp(global_displayed_card_instances, global_card_instances, 0x161e8);
  memcpy(global_displayed_card_instances, global_card_instances,
         sizeof(global_displayed_card_instances));
  if (g_active_cards_count[0] != g_duel_cached_active_cards_count_player_0 ||
      g_active_cards_count[1] != g_duel_cached_active_cards_count_player_1)
  {
    needs_refresh = 1;
  }
  g_duel_cached_active_cards_count_player_0 = g_active_cards_count[0];
  g_duel_cached_active_cards_count_player_1 = g_active_cards_count[1];
  if (g_life[0] != g_duel_cached_life_player_0 || g_life[1] != g_duel_cached_life_player_1)
  {
    needs_refresh = 1;
  }
  g_duel_cached_life_player_0 = g_life[0];
  g_duel_cached_life_player_1 = g_life[1];
  if (g_poison_counters[0] != g_duel_cached_state_007abce0 ||
      g_duel_cached_state_007abce4 != g_poison_counters[1])
  {
    needs_refresh = 1;
  }
  g_duel_cached_state_007abce0 = g_poison_counters[0];
  g_duel_cached_state_007abce4 = g_poison_counters[1];
  if (g_lich_active[0] != g_duel_cached_unk_008b44d0_player_0 ||
      g_lich_active[1] != g_duel_cached_unk_008b44d0_player_1)
  {
    needs_refresh = 1;
  }
  g_duel_cached_unk_008b44d0_player_0 = g_lich_active[0];
  g_duel_cached_unk_008b44d0_player_1 = g_lich_active[1];

  needs_refresh |= memcmp(g_duel_cached_raw_mana_player_0, g_raw_mana_available, 0x1c);
  needs_refresh |= memcmp(g_duel_cached_raw_mana_player_1, g_raw_mana_available + 1, 0x1c);
  memcpy(g_duel_cached_raw_mana_player_0, g_raw_mana_available,
         sizeof(g_duel_cached_raw_mana_player_0));
  memcpy(g_duel_cached_raw_mana_player_1, g_raw_mana_available + 1,
         sizeof(g_duel_cached_raw_mana_player_1));

  needs_refresh |= memcmp(g_duel_cached_graveyard_player_0, global_graveyard_slots, 2000);
  needs_refresh |= memcmp(g_duel_cached_graveyard_player_1, global_graveyard_slots[1], 2000);
  g_duel_cached_graveyard_count_player_0 = 0;
  for (zone_index = 0; zone_index < 500 && global_graveyard_slots[0][zone_index] != -1; ++zone_index)
  {
    g_duel_cached_graveyard_player_0[g_duel_cached_graveyard_count_player_0] = CardIDFromType(global_graveyard_slots[0][zone_index]);
    ++g_duel_cached_graveyard_count_player_0;
  }
  g_duel_cached_graveyard_count_player_1 = 0;
  for (zone_index = 0; zone_index < 500 && global_graveyard_slots[1][zone_index] != -1; ++zone_index)
  {
    g_duel_cached_graveyard_player_1[g_duel_cached_graveyard_count_player_1] = CardIDFromType(global_graveyard_slots[1][zone_index]);
    ++g_duel_cached_graveyard_count_player_1;
  }

  needs_refresh |= memcmp(g_duel_cached_exile_player_0, global_exile, 2000);
  needs_refresh |= memcmp(g_duel_cached_exile_player_1, global_exile[1], 2000);
  g_duel_cached_exile_count_player_0 = 0;
  for (zone_index = 0; zone_index < 500 && global_exile[0][zone_index] != -1; ++zone_index)
  {
    g_duel_cached_exile_player_0[g_duel_cached_exile_count_player_0] = CardIDFromType(global_exile[0][zone_index]);
    ++g_duel_cached_exile_count_player_0;
  }
  g_duel_cached_exile_count_player_1 = 0;
  for (zone_index = 0; zone_index < 500 && global_exile[1][zone_index] != -1; ++zone_index)
  {
    g_duel_cached_exile_player_1[g_duel_cached_exile_count_player_1] = CardIDFromType(global_exile[1][zone_index]);
    ++g_duel_cached_exile_count_player_1;
  }

  needs_refresh |= memcmp(g_duel_cached_library_player_0, global_library, 2000);
  needs_refresh |= memcmp(g_duel_cached_library_player_1, global_library[1], 2000);
  g_duel_cached_library_count_player_0 = 0;
  for (zone_index = 0; zone_index < 500 && global_library[0][zone_index] != -1; ++zone_index)
  {
    g_duel_cached_library_player_0[g_duel_cached_library_count_player_0] = CardIDFromType(global_library[0][zone_index]);
    ++g_duel_cached_library_count_player_0;
  }
  g_duel_cached_library_count_player_1 = 0;
  for (zone_index = 0; zone_index < 500 && global_library[1][zone_index] != -1; ++zone_index)
  {
    g_duel_cached_library_player_1[g_duel_cached_library_count_player_1] = CardIDFromType(global_library[1][zone_index]);
    ++g_duel_cached_library_count_player_1;
  }

  g_multiblock_creature_count = 0;
  for (zone_index = 0; global_stack_cards[zone_index].player != -1 && zone_index < 0x20; ++zone_index)
  {
    if (g_stack_prompt_flags[zone_index] != 0)
    {
      stack_player = global_stack_cards[zone_index].player;
      stack_card = global_stack_cards[zone_index].card;
      g_duel_cached_stack_entries[g_multiblock_creature_count].player = stack_player;
      g_duel_cached_stack_entries[g_multiblock_creature_count].card = stack_card;
      g_duel_cached_stack_entries[g_multiblock_creature_count].number_of_targets =
          (int)(char)PLAYER_CARD_INSTANCE(stack_player, stack_card).number_of_targets;
      for (target_index = 0;
           target_index < (int)(char)PLAYER_CARD_INSTANCE(stack_player, stack_card).number_of_targets;
           ++target_index)
      {
        g_duel_cached_stack_entries[g_multiblock_creature_count].targets[target_index].player =
            PLAYER_CARD_INSTANCE(stack_player, stack_card).targets[target_index].player;
        g_duel_cached_stack_entries[g_multiblock_creature_count].targets[target_index].card =
            PLAYER_CARD_INSTANCE(stack_player, stack_card).targets[target_index].card;
      }
      ++g_multiblock_creature_count;
    }
  }

  zone_index = 0;
  g_duel_cached_ante_count_player_1 = 0;
  for (; zone_index < 0x10 && global_ante_cards[1][zone_index] != -1; ++zone_index)
  {
    ++g_duel_cached_ante_count_player_1;
  }
  memcpy(g_duel_cached_ante_player_1, global_ante_cards[1], sizeof(g_duel_cached_ante_player_1));
  zone_index = 0;
  g_duel_cached_ante_count_player_0 = 0;
  for (; zone_index < 0x10 && global_ante_cards[0][zone_index] != -1; ++zone_index)
  {
    ++g_duel_cached_ante_count_player_0;
  }
  memcpy(g_duel_cached_ante_player_0, global_ante_cards, sizeof(g_duel_cached_ante_player_0));

  for (zone_index = 0; zone_index <= 0x25; ++zone_index)
  {
    if ((int)(char)g_duel_phase_stop_settings[0].phase_flags[zone_index] != g_duel_cached_phase_stops_player_0[zone_index])
    {
      needs_refresh |= 1;
    }
    if ((int)(char)g_duel_phase_stop_settings[1].phase_flags[zone_index] != g_duel_cached_phase_stops_player_1[zone_index])
    {
      needs_refresh |= 1;
    }
    g_duel_cached_phase_stops_player_0[zone_index] = (int)(char)g_duel_phase_stop_settings[0].phase_flags[zone_index] & PHASE_STOP_ENABLED;
    g_duel_cached_phase_stops_player_1[zone_index] = (int)(char)g_duel_phase_stop_settings[1].phase_flags[zone_index] & PHASE_STOP_ENABLED;
  }

  if (g_duel_cached_response_state != g_response_selection_in_progress)
  {
    needs_refresh = 1;
  }
  g_duel_cached_response_state = g_response_selection_in_progress;
  LeaveCriticalSection(&g_duel_render_lock);
  return needs_refresh;
}

// FUNCTION: MAGIC 0x00449ac3
// FUNCTION: SHANDALAR 0x00453644
unsigned int get_displayed_card_original_internal_id(int player, int card)
{
  int displayed_card_type;

  if (displayed_card_indices_invalid(player, card) != 0)
  {
    return -1;
  }

  EnterCriticalSection(&g_duel_render_lock);
  displayed_card_type = DISPLAYED_PLAYER_CARD_INSTANCE(player, card).original_internal_card_id;
  LeaveCriticalSection(&g_duel_render_lock);
  return displayed_card_type;
}

// FUNCTION: MAGIC 0x00559999
// FUNCTION: SHANDALAR 0x0056fce9
unsigned int draw_displayed_full_card(HDC dc, RECT *rect, card_ptr_t *raw_card, int player, int card, int draw_mode, int expand_text_box)
{
  char rules_text[1000];
  int number_of_colors;
  unsigned int color_test;
  int color_index;
  card_ptr_t card_data;
  int card_version;
  card_id_t card_id;

  if (dc == NULL || rect == NULL || raw_card == NULL)
  {
    return 0;
  }

  card_id = get_displayed_card_id(player, card);
  if (card_id == -1)
  {
    return 0;
  }

  memcpy(&card_data, global_raw_cards_storage + card_id, sizeof(card_data));
  color_test = get_displayed_card_color_flags(player, card);
  number_of_colors = 0;
  for (color_index = 1; color_index <= 5; ++color_index)
  {
    if ((color_test & (1 << (unsigned char)color_index)) != 0)
    {
      ++number_of_colors;
    }
  }

  if (number_of_colors > 1)
  {
    card_data.color = 4;
  }
  else
  {
    if ((color_test & 2) != 0)
    {
      card_data.color = 1;
    }
    else if ((color_test & 0x20) != 0)
    {
      card_data.color = 8;
    }
    else if ((color_test & 8) != 0)
    {
      card_data.color = 5;
    }
    else if ((color_test & 0x10) != 0)
    {
      card_data.color = 7;
    }
    else if ((color_test & 4) != 0)
    {
      card_data.color = 2;
    }
  }

  strcpy(rules_text, global_raw_cards_storage[card_id].rules_text);
  apply_displayed_card_sleight_words(player, card, rules_text);
  apply_displayed_card_hack_words(player, card, rules_text);
  card_data.rules_text = rules_text;
  card_version = get_card_display_pic_num(card_id, player, card);
  return DrawFullCard(dc,
                      rect,
                      &card_data,
                      card_version,
                      draw_mode,
                      expand_text_box,
                      gs_illus_00789130);
}

// FUNCTION: MAGIC 0x00559bc1
// FUNCTION: SHANDALAR 0x0056ff11
void draw_displayed_card_overlaid_full_card(HDC dc, RECT *rect, int player, int card)
{
  card_id_t card_id;
  int displayed_player_and_card[2];
  int number_of_colors;
  unsigned int color_test;
  int color_index;
  card_ptr_t card_data;
  int card_version;
  int displayed_type;

  if (dc == NULL || rect == NULL)
  {
    return;
  }

  displayed_type = get_displayed_card_base_internal_id(player, card);
  card_id = CardIDFromType(displayed_type);
  if ((card_id == g_activation_display_internal_card_id && get_displayed_card_original_internal_id(player, card) == g_draw_card_placeholder_internal_card_id) || card_id == g_card_back_display_internal_card_id)
  {
    DrawCardBack(dc, rect);
    return;
  }

  get_displayed_card_parent(displayed_player_and_card, player, card);
  card_version = get_card_display_pic_num(card_id, displayed_player_and_card[0], displayed_player_and_card[1]);
  if (card_id == g_damage_display_internal_card_id ||
      card_id == g_hunting_display_internal_card_id ||
      card_id == g_multiblock_display_internal_card_id ||
      card_id == g_effect_display_internal_card_id ||
      card_id == g_legacy_display_internal_card_id)
  {
    if (get_displayed_card_id(displayed_player_and_card[0], displayed_player_and_card[1]) != -1)
    {
      draw_special_effect_full_card(dc, rect, card_id, displayed_player_and_card[0], displayed_player_and_card[1]);
    }
    return;
  }

  memcpy(&card_data, global_raw_cards_storage + card_id, sizeof(card_data));
  color_test = get_displayed_card_color_flags(player, card);
  number_of_colors = 0;
  for (color_index = 1; color_index <= 5; ++color_index)
  {
    if ((color_test & (1 << (unsigned char)color_index)) != 0)
    {
      ++number_of_colors;
    }
  }

  if (number_of_colors > 1)
  {
    card_data.color = 4;
  }
  else
  {
    if ((color_test & 2) != 0)
    {
      card_data.color = 1;
    }
    else if ((color_test & 0x20) != 0)
    {
      card_data.color = 8;
    }
    else if ((color_test & 8) != 0)
    {
      card_data.color = 5;
    }
    else if ((color_test & 0x10) != 0)
    {
      card_data.color = 7;
    }
    else if ((color_test & 4) != 0)
    {
      card_data.color = 2;
    }
  }

  DrawFullCard(dc, rect, &card_data, card_version, 2, g_duel_interface_options.expand_text_box_on_big_card, gs_illus_00789130);
}

// FUNCTION: MAGIC 0x00559e9c
// FUNCTION: SHANDALAR 0x005701e5
void draw_special_effect_full_card(HDC dc, RECT *rect, card_id_t card_id, int player, int card)
{
  unsigned int final_display_flags;
  char transformed_rules_text[400];
  int appended_text;
  int displayed_power;
  char value_text[100];
  int displayed_toughness;
  unsigned int display_flags;
  card_id_t transformed_card_id;
  char temporary_name[52];
  int single_color;
  unsigned int display_info;
  int displayed_player_and_card[2];
  int ability_index;
  card_ptr_t card_data;
  unsigned int display_version;

  if (dc == NULL || rect == NULL)
  {
    return;
  }

  display_info = get_displayed_card_display_pic_info(displayed_player_and_card, player, card);
  display_version = (unsigned short)((display_info >> 0x10) & 0xffff);
  card_data.id = display_info & 0xffff;

  if (card_id == g_damage_display_internal_card_id)
  {
    sprintf(g_card_title_buffer_00709100, "%s: %d", gs_cardtitle_damage_008cfd30, get_displayed_card_special_counters(player, card));
    single_color = single_color_test_bit_to_color_t(get_displayed_card_color_flags(player, card));
    if (single_color == 1 ||
        single_color == 2 ||
        single_color == 4 ||
        single_color == 3 ||
        single_color == 5)
    {
      sprintf(g_card_title_buffer_00709100 + strlen(g_card_title_buffer_00709100), " (%s)", gs_color_word_capitalized_00709390[single_color]);
    }
  }
  else if (card_id == g_hunting_display_internal_card_id)
  {
    sprintf(g_card_title_buffer_00709100, gs_cardtitle_hunting_00926750, get_hunting_subtype_name(get_displayed_card_special_counters(player, card)));
  }
  else if (card_id == g_multiblock_display_internal_card_id)
  {
    strcpy(g_card_title_buffer_00709100, gs_multiblock_creature_008cf040);
  }
  else if (card_id == g_effect_display_internal_card_id)
  {
    strcpy(g_card_title_buffer_00709100, global_legacy_names[card_data.id].effect_title);
  }
  else if (card_id == g_legacy_display_internal_card_id)
  {
    strcpy(g_card_title_buffer_00709100, global_legacy_names[card_data.id].legacy_title);
  }
  else
  {
    g_card_title_buffer_00709100[0] = '\0';
  }

  if (card_id == g_effect_display_internal_card_id && get_displayed_card_eot_toughness(player, card) > 0)
  {
    strcpy(temporary_name, g_card_title_buffer_00709100);
    extract_numbered_text_choice(g_card_title_buffer_00709100, temporary_name, get_displayed_card_eot_toughness(player, card));
  }

  transformed_card_id = get_displayed_card_id(displayed_player_and_card[0], displayed_player_and_card[1]);
  if (card_id == g_effect_display_internal_card_id &&
      (transformed_card_id == CARD_ID_FAERIE_DRAGON ||
       transformed_card_id == CARD_ID_WHIMSY))
  {
    strcpy(g_card_title_buffer_00709100, global_legacy_names[transformed_card_id].effect_title);
  }

  card_data.name = g_card_title_buffer_00709100;
  card_data.full_name = card_data.name;
  card_data.expansion = 0xffffffff;
  card_data.color = 0xffffffff;
  card_data.card_type = 0;
  card_data.subtype = 0xffffffff;
  card_data.type_text = NULL;
  card_data.db_card_type_2 = 0xffffffff;
  card_data.rarity = 0;
  for (ability_index = 0; ability_index < 10; ++ability_index)
  {
    ((char *)&card_data.req)[ability_index] = 0;
  }
  card_data.unused0x3c = 0xffffffff;
  card_data.artist = NULL;
  card_data.num_pics = 0;
  card_data.ai_against_color = card_data.ai_for_color = card_data.ai_counts_as_color = 0;
  card_data.ai_against_land = card_data.ai_for_land = card_data.ai_counts_as_land = 0;
  card_data.expansion_rarity = 0;

  if (card_id == g_damage_display_internal_card_id)
  {
    strcpy(g_card_rules_text_buffer_00708da8, global_legacy_names[card_data.id].damage_text);
  }
  else if (card_id == g_hunting_display_internal_card_id)
  {
    strcpy(g_card_rules_text_buffer_00708da8, global_legacy_names[card_data.id].effect_text);
  }
  else if (card_id == g_multiblock_display_internal_card_id)
  {
    strcpy(g_card_rules_text_buffer_00708da8, gs_multiblock_shadow_blocker_00925e70);
  }
  else if (card_id == g_effect_display_internal_card_id)
  {
    strcpy(g_card_rules_text_buffer_00708da8, global_legacy_names[card_data.id].effect_text);
  }
  else if (card_id == g_legacy_display_internal_card_id)
  {
    strcpy(g_card_rules_text_buffer_00708da8, global_legacy_names[card_data.id].legacy_text);
  }
  else
  {
    g_card_rules_text_buffer_00708da8[0] = '\0';
  }

  if ((get_displayed_card_display_flags(player, card) & 0x2000000) != 0)
  {
    apply_displayed_card_sleight_words(displayed_player_and_card[0], displayed_player_and_card[1], g_card_rules_text_buffer_00708da8);
    apply_displayed_card_hack_words(displayed_player_and_card[0], displayed_player_and_card[1], g_card_rules_text_buffer_00708da8);
  }
  else
  {
    apply_displayed_card_sleight_words(player, card, g_card_rules_text_buffer_00708da8);
    apply_displayed_card_hack_words(player, card, g_card_rules_text_buffer_00708da8);
  }

  replace_text_token(g_card_rules_text_buffer_00708da8, "|H1", 1, "");
  replace_text_token(g_card_rules_text_buffer_00708da8, "|H2", 1, "");
  replace_text_token(g_card_rules_text_buffer_00708da8, "|H3", 1, "");
  replace_text_token(g_card_rules_text_buffer_00708da8, "|H4", 1, "");
  replace_text_token(g_card_rules_text_buffer_00708da8, "|H", 1, "");
  replace_text_token(g_card_rules_text_buffer_00708da8, "|S1", 1, "");
  replace_text_token(g_card_rules_text_buffer_00708da8, "|S2", 1, "");
  replace_text_token(g_card_rules_text_buffer_00708da8, "|S3", 1, "");
  replace_text_token(g_card_rules_text_buffer_00708da8, "|S4", 1, "");
  replace_text_token(g_card_rules_text_buffer_00708da8, "|S", 1, "");

  get_displayed_card_counter_power_toughness(player, card, &displayed_power, &displayed_toughness);
  sprintf(value_text, "+%d/+%d", displayed_power, displayed_toughness);
  replace_text_token(g_card_rules_text_buffer_00708da8, "+|n/+|n", 0, value_text);
  sprintf(value_text, "+0/+%d", displayed_toughness);
  replace_text_token(g_card_rules_text_buffer_00708da8, "+0/+|n", 0, value_text);
  sprintf(value_text, "+%d/+0", displayed_power);
  replace_text_token(g_card_rules_text_buffer_00708da8, "+|n/+0", 0, value_text);
  sprintf(value_text, "%d/-0", displayed_power);
  replace_text_token(g_card_rules_text_buffer_00708da8, "-|n/-0", 0, value_text);
  sprintf(value_text, "+%d/%d", displayed_power, displayed_toughness);
  replace_text_token(g_card_rules_text_buffer_00708da8, "+|n/-|n", 0, value_text);
  sprintf(value_text, "%d", displayed_toughness);
  replace_text_token(g_card_rules_text_buffer_00708da8, "|n", 0, value_text);

  if (card_data.id == 0x21b)
  {
    display_flags = get_displayed_card_special_counters(player, card);
    appended_text = 0;
    strcpy(value_text, "");
    if ((display_flags & 0x20) != 0)
    {
      if (appended_text != 0)
      {
        strcat(value_text, ", ");
      }
      strcat(value_text, gs_ability_word_008b4de0[0]);
      appended_text = 1;
    }
    if ((display_flags & 0x100) != 0)
    {
      if (appended_text != 0)
      {
        strcat(value_text, ", ");
      }
      strcat(value_text, gs_ability_word_008b4de0[4]);
      appended_text = 1;
    }
    if ((display_flags & 0x40) != 0)
    {
      if (appended_text != 0)
      {
        strcat(value_text, ", ");
      }
      strcat(value_text, gs_ability_word_008b4de0[2]);
      appended_text = 1;
    }
    if ((display_flags & 0x80) != 0)
    {
      if (appended_text != 0)
      {
        strcat(value_text, ", ");
      }
      strcat(value_text, gs_ability_word_008b4de0[3]);
      appended_text = 1;
    }
    replace_text_token(g_card_rules_text_buffer_00708da8, "|A", 0, value_text);
    get_displayed_card_counter_power_toughness(player, card, &displayed_power, &displayed_toughness);
    sprintf(value_text, "-%d/-%d", displayed_power, displayed_toughness);
    replace_text_token(g_card_rules_text_buffer_00708da8, "-|n/-|n", 0, value_text);
  }

  if (card_id == g_effect_display_internal_card_id && get_displayed_card_eot_toughness(player, card) > 0)
  {
    strcpy(transformed_rules_text, g_card_rules_text_buffer_00708da8);
    extract_numbered_text_choice(g_card_rules_text_buffer_00708da8, transformed_rules_text, get_displayed_card_eot_toughness(player, card));
  }

  if (card_id == g_damage_display_internal_card_id)
  {
    final_display_flags = get_displayed_card_display_flags(player, card);
    if (g_card_rules_text_buffer_00708da8[0] != '\0' && g_card_rules_text_buffer_00708da8[0] != '\n')
    {
      strcat(g_card_rules_text_buffer_00708da8, "\n");
    }
    if ((final_display_flags & 0x100000) != 0)
    {
      strcat(g_card_rules_text_buffer_00708da8, gs_ability_word_008b4de0[4]);
      strcat(g_card_rules_text_buffer_00708da8, "\n");
    }
    if ((final_display_flags & 0x80000) != 0)
    {
      strcat(g_card_rules_text_buffer_00708da8, gs_ability_word_008b4de0[3]);
    }
  }

  card_data.rules_text = g_card_rules_text_buffer_00708da8;
  card_data.flavor_text = "";
  card_data.power = 0;
  card_data.toughness = 0;
  card_data.sleight_color = 0;
  *(uint32_t *)&card_data.hack_colors = 0;
  DrawFullCard(dc, rect, &card_data, display_version, 2, g_duel_interface_options.expand_text_box_on_big_card, gs_illus_00789130);
}

// FUNCTION: MAGIC 0x00506fa0
// FUNCTION: SHANDALAR 0x004c5ca0
INT_PTR CALLBACK big_card_choice_dialog_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  HGDIOBJ timer_font;
  char card_coordinates[12];
  HDC paint_dc;
  PAINTSTRUCT paint_struct;
  int picture_number;
  card_id_t card_id;
  card_id_t card_image_number;
  int hovered_player_and_card[2];
  POINT cursor_point;
  card_id_t bigcard_image;
  card_id_t smallcard_image;
  RECT before_drag_rect;
  unsigned int was_dragged;
  RECT after_drag_rect;
  HDC erase_dc;
  RECT erase_rect;
  LRESULT current_selection;
  UINT notification_code;
  HGDIOBJ old_font;
  LOGFONTA listbox_font_info;
  HFONT bold_font;
  HANDLE listbox_font_handle;
  int smallcard_player_and_card[2];
  HWND smallcard_window;
  RECT listbox_rect;
  int listbox_top;
  int listbox_right_margin;
  RECT client_rect;

  switch (msg)
  {
  case WM_INITDIALOG:
    g_big_card_choice_state_006f6e20 = 0;
    if (g_duel_network_state != 0)
    {
      SetTimer(hwnd, 1, 2000, (TIMERPROC)0);
    }

    GetWindowRect(GetDlgItem(hwnd, 0x3fd), &listbox_rect);
    MapWindowPoints((HWND)0, hwnd, (LPPOINT)&listbox_rect, 2);
    GetClientRect(hwnd, &client_rect);
    listbox_top = listbox_rect.top;
    listbox_right_margin = client_rect.right - listbox_rect.right;
    InflateRect(&client_rect, -listbox_top, -listbox_right_margin);
    CopyRect(&g_big_card_choice_full_card_rect, &client_rect);
    g_big_card_choice_full_card_rect.right = listbox_rect.left - listbox_top;
    CopyRect(&g_big_card_choice_small_card_rect, &client_rect);
    g_big_card_choice_small_card_rect.left = listbox_rect.left;
    g_big_card_choice_small_card_rect.top = listbox_rect.bottom;
    if (g_big_card_choice_small_card_rect.bottom - g_big_card_choice_small_card_rect.top <
        g_big_card_choice_small_card_rect.right - g_big_card_choice_small_card_rect.left)
    {
      g_big_card_choice_small_card_rect.right =
          g_big_card_choice_small_card_rect.bottom - g_big_card_choice_small_card_rect.top + g_big_card_choice_small_card_rect.left;
    }
    else
    {
      g_big_card_choice_small_card_rect.bottom =
          g_big_card_choice_small_card_rect.right - g_big_card_choice_small_card_rect.left + g_big_card_choice_small_card_rect.top;
    }

    g_big_card_choice_dialog_context = (dialog_context_t *)lparam;
    SetDlgItemTextA(hwnd, 0x3fd, g_big_card_choice_dialog_context->prompt);
    SendDlgItemMessageA(hwnd, 0x3fd, 0x401, g_big_card_choice_dialog_context->dialog_mode, 0);
    if (g_big_card_choice_dialog_context->smallcard_player != -1)
    {
      smallcard_player_and_card[0] = g_big_card_choice_dialog_context->smallcard_player;
      smallcard_player_and_card[1] = g_big_card_choice_dialog_context->smallcard_card;
      smallcard_window = CreateWindowExA(0,
                                         "MAGICGAME_BigCardCardClass",
                                         "BigCard small card",
                                         0x50000000,
                                         g_big_card_choice_small_card_rect.left,
                                         g_big_card_choice_small_card_rect.top,
                                         g_showlist_smallcard_width,
                                         g_showlist_smallcard_height,
                                         hwnd,
                                         (HMENU)1,
                                         g_app_instance,
                                         smallcard_player_and_card);
    }

    listbox_font_handle = (HANDLE)SendDlgItemMessageA(hwnd, 0x3fd, WM_GETFONT, 0, 0);
    GetObjectA(listbox_font_handle, sizeof(listbox_font_info), &listbox_font_info);
    listbox_font_info.lfWeight = 700;
    bold_font = CreateFontIndirectA(&listbox_font_info);
    SendDlgItemMessageA(hwnd, 0x3fd, WM_SETFONT, (WPARAM)bold_font, 0);
    SetFocus(GetDlgItem(hwnd, 0x3fd));
    if (SendDlgItemMessageA(hwnd, 0x3fd, 0x400, 0, 0) == 0 || g_big_card_choice_dialog_context->dialog_mode == 0)
    {
      SetTimer(hwnd, 1, g_big_card_choice_timeout_ms, (TIMERPROC)0);
    }
    return 0;

  case WM_COMMAND:
    if ((lparam & 0xffff) == 1 || (lparam & 0xffff) == 2)
    {
      SendMessageA(hwnd, WM_COMMAND, 0x3fd, (LPARAM)GetDlgItem(hwnd, 0x3fd));
    }
    else if ((wparam & 0xffff) == 0x3fd)
    {
      notification_code = HIWORD(wparam);
      current_selection = SendDlgItemMessageA(hwnd, 0x3fd, 0x400, 0, 0);
      if (notification_code != 0)
      {
        old_font = (HGDIOBJ)SendDlgItemMessageA(hwnd, 0x3fd, WM_GETFONT, 0, 0);
        SendDlgItemMessageA(hwnd, 0x3fd, WM_SETFONT, 0, 0);
        DeleteObject(old_font);
        EndDialog(hwnd, notification_code);
      }
      else if (current_selection == 0 || g_big_card_choice_dialog_context->dialog_mode == 0)
      {
        old_font = (HGDIOBJ)SendDlgItemMessageA(hwnd, 0x3fd, WM_GETFONT, 0, 0);
        SendDlgItemMessageA(hwnd, 0x3fd, WM_SETFONT, 0, 0);
        DeleteObject(old_font);
        EndDialog(hwnd, notification_code);
      }
    }
    return 1;

  case WM_PALETTEISCHANGING:
  case WM_PALETTECHANGED:
  case WM_QUERYNEWPALETTE:
    return handle_button_palette_message(hwnd, msg, (HWND)wparam, lparam);

  case WM_ERASEBKGND:
    erase_dc = (HDC)wparam;
    ApplyCardArtPaletteToDc(erase_dc);
    GetClientRect(hwnd, &erase_rect);
    if (g_magicgame_big_card_choice_pic != 0)
    {
      DrawBitmapToRect(erase_dc, &erase_rect, (HBITMAP)g_magicgame_big_card_choice_pic);
    }
    else
    {
      FillRect(erase_dc, &erase_rect, GetStockObject(0));
    }
    return 1;

  case WM_LBUTTONDOWN:
    GetWindowRect(hwnd, &before_drag_rect);
    SendMessageA(hwnd, WM_SYSCOMMAND, 0xf012, 0);
    GetWindowRect(hwnd, &after_drag_rect);
    if (abs(before_drag_rect.top - after_drag_rect.top) +
            abs(before_drag_rect.left - after_drag_rect.left) <
        5)
    {
      was_dragged = 0;
    }
    else
    {
      was_dragged = 1;
    }
    if (was_dragged == 0)
    {
      SendMessageA(hwnd, WM_COMMAND, 0x3fd, (LPARAM)GetDlgItem(hwnd, 0x3fd));
    }
    return 1;

  case WM_MOUSEMOVE:
  case WM_RBUTTONDOWN:
    cursor_point.x = lparam & 0xffff;
    cursor_point.y = HIWORD(lparam);
    if ((msg == WM_MOUSEMOVE && g_duel_interface_options.layout != 2) || (msg == WM_RBUTTONDOWN && g_duel_interface_options.layout == 2))
    {
      bigcard_image = get_displayed_card_id(g_big_card_choice_dialog_context->bigcard_player, g_big_card_choice_dialog_context->bigcard_card);
      if (g_big_card_choice_dialog_context->smallcard_player == -1 || g_big_card_choice_dialog_context->smallcard_card == -1)
      {
        smallcard_image = -1;
      }
      else
      {
        smallcard_image = get_displayed_card_id(g_big_card_choice_dialog_context->smallcard_player, g_big_card_choice_dialog_context->smallcard_card);
      }

      if (bigcard_image != -1 && PtInRect(&g_big_card_choice_full_card_rect, cursor_point) != 0)
      {
        hovered_player_and_card[0] = g_big_card_choice_dialog_context->bigcard_player;
        hovered_player_and_card[1] = g_big_card_choice_dialog_context->bigcard_card;
        SendMessageA(g_duel_card_preview_window_hwnd, 0x401, (WPARAM)bigcard_image, (LPARAM)hovered_player_and_card);
      }
      else if (smallcard_image != -1)
      {
        if (PtInRect(&g_big_card_choice_small_card_rect, cursor_point) != 0)
        {
          hovered_player_and_card[0] = g_big_card_choice_dialog_context->smallcard_player;
          hovered_player_and_card[1] = g_big_card_choice_dialog_context->smallcard_card;
          SendMessageA(g_duel_card_preview_window_hwnd, 0x401, (WPARAM)smallcard_image, (LPARAM)hovered_player_and_card);
        }
      }
    }
    return 0;

  case WM_PAINT:
    paint_dc = BeginPaint(hwnd, &paint_struct);
    if (paint_dc != 0)
    {
      ApplyCardArtPaletteToDc(paint_dc);
      card_image_number = get_displayed_card_id(g_big_card_choice_dialog_context->bigcard_player, g_big_card_choice_dialog_context->bigcard_card);
      card_id = CardIDFromType(get_displayed_card_original_internal_id(g_big_card_choice_dialog_context->bigcard_player, g_big_card_choice_dialog_context->bigcard_card));
      if (card_image_number != -1)
      {
        picture_number = get_card_display_pic_num(card_image_number,
                                                  g_big_card_choice_dialog_context->bigcard_player,
                                                  g_big_card_choice_dialog_context->bigcard_card);
        if (card_image_number == g_card_back_display_internal_card_id)
        {
          DrawCardBack(paint_dc, &g_big_card_choice_full_card_rect);
        }
        else if (card_image_number == g_damage_display_internal_card_id ||
                 card_image_number == g_hunting_display_internal_card_id ||
                 card_image_number == g_multiblock_display_internal_card_id ||
                 card_image_number == g_effect_display_internal_card_id ||
                 card_image_number == g_legacy_display_internal_card_id)
        {
          draw_special_effect_full_card(paint_dc,
                                        &g_big_card_choice_full_card_rect,
                                        card_image_number,
                                        g_big_card_choice_dialog_context->bigcard_player,
                                        g_big_card_choice_dialog_context->bigcard_card);
        }
        else if (card_image_number == g_activation_display_internal_card_id)
        {
          draw_displayed_card_overlaid_full_card(paint_dc, &g_big_card_choice_full_card_rect, g_big_card_choice_dialog_context->bigcard_player, g_big_card_choice_dialog_context->bigcard_card);
        }
        else
        {
          draw_displayed_full_card(paint_dc,
                                   &g_big_card_choice_full_card_rect,
                                   global_raw_cards_storage + card_image_number,
                                   g_big_card_choice_dialog_context->bigcard_player,
                                   g_big_card_choice_dialog_context->bigcard_card,
                                   0x12,
                                   0);
        }

        if (g_duel_dialog_refresh_state != 0)
        {
          sprintf(card_coordinates, "%d,%d", g_big_card_choice_dialog_context->bigcard_player, g_big_card_choice_dialog_context->bigcard_card);
          SetBkMode(paint_dc, 1);
          SetTextColor(paint_dc, 0);
          TextOutA(paint_dc,
                   g_big_card_choice_full_card_rect.left + 5,
                   g_big_card_choice_full_card_rect.top +
                       ((g_big_card_choice_full_card_rect.bottom - g_big_card_choice_full_card_rect.top) * 0x14) / 100,
                   card_coordinates,
                   strlen(card_coordinates));
          SetTextColor(paint_dc, 0xffffff);
          TextOutA(paint_dc,
                   g_big_card_choice_full_card_rect.left + 4,
                   g_big_card_choice_full_card_rect.top +
                       ((g_big_card_choice_full_card_rect.bottom - g_big_card_choice_full_card_rect.top) * 0x14) / 100 - 1,
                   card_coordinates,
                   strlen(card_coordinates));
        }

        (void)picture_number;
      }
      else if (card_id != -1 &&
               card_id != g_card_back_display_internal_card_id &&
               card_id != g_damage_display_internal_card_id &&
               card_id != g_hunting_display_internal_card_id &&
               card_id != g_multiblock_display_internal_card_id &&
               card_id != g_effect_display_internal_card_id &&
               card_id != g_legacy_display_internal_card_id &&
               card_id != g_activation_display_internal_card_id)
      {
        DrawFullCard(paint_dc, &g_big_card_choice_full_card_rect, global_raw_cards_storage + card_id, 0, 0x12, 0, gs_illus_00789130);
        if (g_duel_dialog_refresh_state != 0)
        {
          sprintf(card_coordinates, "%d,%d", g_big_card_choice_dialog_context->bigcard_player, g_big_card_choice_dialog_context->bigcard_card);
          SetBkMode(paint_dc, 1);
          SetTextColor(paint_dc, 0);
          TextOutA(paint_dc,
                   g_big_card_choice_full_card_rect.left + 5,
                   g_big_card_choice_full_card_rect.top +
                       ((g_big_card_choice_full_card_rect.bottom - g_big_card_choice_full_card_rect.top) * 0x14) / 100,
                   card_coordinates,
                   strlen(card_coordinates));
          SetTextColor(paint_dc, 0xffffff);
          TextOutA(paint_dc,
                   g_big_card_choice_full_card_rect.left + 4,
                   g_big_card_choice_full_card_rect.top +
                       ((g_big_card_choice_full_card_rect.bottom - g_big_card_choice_full_card_rect.top) * 0x14) / 100 - 1,
                   card_coordinates,
                   strlen(card_coordinates));
        }
      }

      EndPaint(hwnd, &paint_struct);
    }
    InvalidateRect(GetDlgItem(hwnd, 0x3fd), NULL, TRUE);
    UpdateWindow(GetDlgItem(hwnd, 0x3fd));
    return 1;

  case WM_TIMER:
    timer_font = (HGDIOBJ)SendDlgItemMessageA(hwnd, 0x3fd, WM_GETFONT, 0, 0);
    SendDlgItemMessageA(hwnd, 0x3fd, WM_SETFONT, 0, 0);
    DeleteObject(timer_font);
    EndDialog(hwnd, 0);
    return 1;

  default:
    return 0;
  }
}
