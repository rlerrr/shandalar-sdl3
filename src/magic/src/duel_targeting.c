#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>

#include "cardartlib/src/assert.h"
#include "cardartlib/src/palette.h"
#include "deckdll/src/card_db.h"
#include "deckdll/src/magsnd.h"
#include "deckdll/src/shared_resources.h"
#include "drawcardlib/Drawcardlib.h"
#include "duel_engine.h"
#include "game_support.h"
#include "magic/src/duel_runtime.h"
#include "global_strings.h"
#include "shared_startup.h"

int can_target(target_definition_t *td)
{
  return td != NULL;
}

// FUNCTION: MAGIC 0x0048194e
// FUNCTION: SHANDALAR 0x00485458
int select_best_land_target_by_score(int preferred_player, int only_player, int *target_data)
{
  card_instance_t *instance; /* [ebp-0x84] */
  int card;                  /* [ebp-0x80] */
  int mana_colors[8];        /* [ebp-0x7c] */
  unsigned int colors;       /* [ebp-0x5c] */
  int card_colors[8];        /* [ebp-0x58] */
  int current_score;         /* [ebp-0x38] */
  int color;                 /* [ebp-0x34] */
  int best_score;            /* [ebp-0x30] */
  int land_scores[8];        /* [ebp-0x2c] */
  int best_card_color;       /* [ebp-0x0c] */
  int player;                /* [ebp-0x08] */
  int best_land_color;       /* [ebp-0x04] */

  target_data[1] = -1;
  *target_data = target_data[1];

  for (color = 0; color <= 5; ++color)
  {
    card_colors[color] = 0;
    land_scores[color] = card_colors[color];
    mana_colors[color] = land_scores[color];
  }

  for (player = 0; player < 2; ++player)
  {
    if (only_player != -1 && player != only_player)
      continue;

    for (card = 0; card < g_active_cards_count[player]; ++card)
    {
      instance = &PLAYER_CARD_INSTANCE(player, card);
      if (instance->internal_card_id != -1)
      {
        if ((instance->state & (STATE_OUBLIETTED | STATE_INVISIBLE | STATE_IN_PLAY)) == STATE_IN_PLAY)
        {
          if ((global_cards_data[instance->internal_card_id].extra_ability & EA_MANA_SOURCE) != 0)
          {
            for (color = 1; color <= 5; ++color)
            {
              if ((((int)(char)instance->mana_color) & (1 << (unsigned char)color)) != 0)
              {
                ++mana_colors[color];
              }
            }
          }

          if (((int)instance->color & 0x3e) != 0)
          {
            for (color = 1; color <= 5; ++color)
            {
              if (((int)instance->color & (1 << (unsigned char)color)) != 0)
              {
                ++land_scores[color];
              }
            }
          }
        }
        else if (preferred_player == only_player || only_player == -1)
        {
          colors = (unsigned int)(char)global_cards_data[instance->internal_card_id].color;
          for (color = 1; color <= 5; ++color)
          {
            if ((colors & (1 << (unsigned char)color)) != 0)
            {
              ++card_colors[color];
            }
          }
        }
      }
    }
  }

  best_card_color = 0;
  best_land_color = best_card_color;

  for (color = 1; color <= 5; ++color)
  {
    if (land_scores[color] > land_scores[best_land_color])
    {
      best_land_color = color;
    }
    if (card_colors[color] > card_colors[best_card_color])
    {
      best_card_color = color;
    }
  }

  best_score = -1;

  for (player = 0; player < 2; ++player)
  {
    if (only_player != -1 && player != only_player)
      continue;

    for (card = 0; card < g_active_cards_count[player]; ++card)
    {
      instance = &PLAYER_CARD_INSTANCE(player, card);
      if (is_in_play(player, card) != 0)
      {
        if ((global_cards_data[instance->internal_card_id].type & TYPE_LAND) != 0)
        {
          current_score = 0;

          if ((global_cards_data[instance->internal_card_id].extra_ability & EA_ACT_ABILITY) != 0)
          {
            current_score += 2;
            if ((instance->state & STATE_TAPPED) != 0)
            {
              current_score += 3;
            }
          }

          if ((global_cards_data[instance->internal_card_id].extra_ability & EA_MANA_SOURCE) != 0)
          {
            ++current_score;

            colors = (unsigned int)(char)instance->mana_color;
            for (color = 1; color <= 5; ++color)
            {
              if ((colors & (1 << (unsigned char)color)) != 0)
              {
                current_score += ((4 - mana_colors[color]) > 0) ? (4 - mana_colors[color]) : 0;
              }
            }

            if (best_land_color != 0 && (colors & (1 << (unsigned char)best_land_color)) != 0)
            {
              ++current_score;
            }
            if (preferred_player == only_player || only_player == -1)
            {
              if (best_card_color != 0 && (colors & (1 << (unsigned char)best_card_color)) != 0)
              {
                ++current_score;
              }
            }
          }

          if ((global_cards_data[instance->internal_card_id].type & TYPE_CREATURE) != 0)
          {
            ++current_score;
          }

          if (current_score > best_score)
          {
            best_score = current_score;
            *target_data = player;
            target_data[1] = card;
          }
        }
      }
    }
  }
  if (*target_data == -1)
    return 0;
  else
    return 1;
}

// FUNCTION: MAGIC 0x00483e3e
// FUNCTION: SHANDALAR 0x0048794a
int choose_best_card_from_library(int player, unsigned int type_mask)
{
  int best_score;
  int best_slot;
  int current_slot;
  int current_internal_id;
  int current_score;
  int mana_score;
  int abilities;
  int bonus;

  best_score = -999;
  best_slot = -1;

  if (player == -1)
  {
    return best_slot;
  }

  for (current_slot = 0; current_slot < 500 && global_library[player][current_slot] != -1; ++current_slot)
  {
    current_internal_id = global_library[player][current_slot];

    if (type_mask == 0xffffffff || (type_mask & (unsigned int)global_cards_data[current_internal_id].type) != 0)
    {
      current_score = (int)global_cards_data[current_internal_id].power + (int)global_cards_data[current_internal_id].toughness;

      if ((int)(char)global_cards_data[current_internal_id].cc[1] == -1)
      {
        mana_score =
            g_basiclandtypes_controlled[player][7] - (int)(char)global_cards_data[current_internal_id].cc[0] - 1;
      }
      else
      {
        mana_score = (int)(char)global_cards_data[current_internal_id].cc[1];
      }
      mana_score = (int)(char)global_cards_data[current_internal_id].cc[0] + mana_score;

      if (mana_score <= g_basiclandtypes_controlled[player][7])
      {
        current_score += mana_score;
      }

      abilities = (int)global_cards_data[current_internal_id].static_ability;
      while (abilities != 0)
      {
        if ((abilities & 1) != 0)
        {
          ++current_score;
        }
        abilities >>= 1;
      }

      if ((global_cards_data[current_internal_id].extra_ability & 0x1000) != 0)
      {
        bonus = 8 - g_basiclandtypes_controlled[player][7];
        if (bonus < 2)
        {
          bonus = 1;
        }
        current_score += bonus;
      }

      if ((global_cards_data[current_internal_id].extra_ability & 1) != 0)
      {
        current_score += mana_score;
      }

      if ((global_cards_data[current_internal_id].type & TYPE_ARTIFACT) != 0)
      {
        ++current_score;
      }

      if (best_score < current_score)
      {
        best_score = current_score;
        best_slot = current_slot;
      }
    }
  }

  return best_slot;
}

// FUNCTION: MAGIC 0x004a62d7
// FUNCTION: SHANDALAR 0x00558457
int select_from_graveyard_with_dialog(int player,
                                      int *graveyard,
                                      void *available,
                                      int count,
                                       char **prompt,
                                      int a6,
                                       int *selected,
                                      int a8,
                                      int a9)
{
  if (g_duel_ai_mode_state == 1)
  {
    return 1;
  }
  return select_multiple_cards_from_card_list(player, graveyard, 0, available, count, prompt, a6, selected, a8, a9);
}

// FUNCTION: MAGIC 0x004a67bd
// FUNCTION: SHANDALAR 0x00558941
int choose_a_number(int player, char *prompt, int maxnum)
{
  int chosen;

  if (g_duel_ai_mode_state == 1)
  {
    return maxnum;
  }

  if (player == g_other_player && (g_duel_network_flags & 2) != 0)
  {
    TENTATIVE_wait_for_network_result(player, 0xe);
    maxnum = g_dialog_result_network_packet.result;
  }

  chosen = prompt_for_life_total(player, prompt, maxnum);

  if (player == g_active_player && (g_duel_network_flags & 2) != 0)
  {
    g_dialog_result_network_packet.packet_type = 0xe;
    g_dialog_result_network_packet.result = chosen;
    TENTATIVE_send_network_result(player, 0xe);
  }

  return chosen;
}

// FUNCTION: MAGIC 0x004a686a
// FUNCTION: SHANDALAR 0x005589ed
int choose_a_color(int player, const char *prompt, int use_color_names_instead_of_land, int ai_choice, unsigned int available_colors)
{
  int chosen_color;

  if (g_duel_ai_mode_state == 1)
  {
    return ai_choice;
  }

  if (g_other_player == player && (g_duel_network_flags & 2) != 0)
  {
    TENTATIVE_wait_for_network_result(player, 0xf);
    return g_dialog_result_network_packet.result;
  }

  chosen_color = choose_a_color_dialog(player, prompt, use_color_names_instead_of_land, ai_choice, available_colors);
  if (player == g_active_player && (g_duel_network_flags & 2) != 0)
  {
    g_dialog_result_network_packet.packet_type = '\x0f';
    g_dialog_result_network_packet.result = chosen_color;
    TENTATIVE_send_network_result(player, 0xf);
  }

  return chosen_color;
}

// FUNCTION: MAGIC 0x004b413c
// FUNCTION: SHANDALAR 0x0056a3a7
int select_damage_card_from_list(int player,
                                 int *internal_card_ids,
                                 int *damage_amounts,
                                 int count,
                                 char *title,
                                 int allow_cancel,
                                 char *prompt)
{
  int selected;

  selected = -1;

  if (player == g_other_player && (g_duel_network_flags & 2) != 0)
  {
    TENTATIVE_wait_for_network_result(player, 0x19);
    selected = g_network_result_packet.result;
  }

  if (player == g_active_player && g_duel_ai_mode_state != 1)
  {
    selected = show_damage_assignment_cardlist(internal_card_ids, damage_amounts, count, title, allow_cancel, prompt);
    if ((g_duel_network_flags & 2) != 0)
    {
      g_network_result_packet.result = selected;
      g_network_result_packet.packet_type = 0x19;
      TENTATIVE_send_network_result(player, 0x19);
    }
  }

  return selected;
}

// FUNCTION: MAGIC 0x004b41f2
// FUNCTION: SHANDALAR 0x0056a45f
int select_card_from_graveyard(int player,
                               int *graveyard,
                               int *allowed_cards,
                               int count,
                               const char *prompt,
                               int allow_cancel,
                               char *title)
{
  int result;

  result = -1;

  if (g_other_player == player && (g_duel_network_flags & 2) != 0)
  {
    TENTATIVE_wait_for_network_result(player, 0x19);
    result = g_network_result_packet.result;
  }

  if (g_active_player == player && g_duel_ai_mode_state != 1)
  {
    result = (int)show_selectable_cardlist(graveyard,
                                           allowed_cards,
                                           count,
                                           (void *)prompt,
                                           (unsigned int)allow_cancel,
                                           title);
    if ((g_duel_network_flags & 2) != 0)
    {
      g_network_result_packet.packet_type = 0x19;
      g_network_result_packet.result = result;
      TENTATIVE_send_network_result(player, 0x19);
    }
  }

  return result;
}

#ifndef SHANDALAR
// FUNCTION: MOK 0x0047936d
// FUNCTION: MAGIC 0x004b42aa
int show_deck(int player, int *cards, int count, char *title, int require_selection, char *prompt)
{
  int y_step;
  int selected_index;
  int display_count;
  int y_positions[500];
  int x_positions[500];
  int y;
  int x;
  int valid_count;
  int valid_graveyards[500];
  int i;
  int result;

  if (((player == g_other_player) && ((g_duel_network_flags & 2) != 0)) && g_duel_active != 0)
  {
    TENTATIVE_wait_for_network_result(player, 0x19);
    return g_network_result_packet.result;
  }
  else if (((player == g_other_player) && ((g_duel_network_flags & 2) == 0)) || g_duel_ai_mode_state == 1)
  {
    valid_count = 0;
    for (i = 0; i < count; ++i)
    {
      if (cards[i] != -1)
      {
        valid_graveyards[valid_count] = i;
        ++valid_count;
      }
    }

    g_ai_recorded_choice = internal_rand(valid_count);
    if (player != g_active_player)
    {
      if (g_duel_ai_mode_state == 1)
      {
        record_ai_action_selection();
      }
      else
      {
        replay_ai_action_selection();
      }
    }

    return valid_graveyards[g_ai_recorded_choice];
  }
  else if (g_duel_active == 0)
  {
    ReadPalette("todpal.tr", (char *)0);
    legacy_clear_graphics_page_stub(0, 0);
    SelectPalette(g_duel_screen_dc, g_duel_palette, 0);
    legacy_load_pcx_into_page_no_palette_stub("advfac64.pic");
    legacy_load_pcx_into_page_stub(1, 0, 0, "seedeck.pic", &g_duel_palette_state);
    legacy_blit_graphics_rect_stub(g_duel_state_0057a75c, 0, 0, 0x280, 0x1e0, g_world_map_state, 0, 0, global_screen_width, global_screen_height);

    x = 0;
    display_count = 0;
    for (i = 0; i < count; ++i)
    {
      if (cards[i] != -1 && (i == 0 || cards[i - 1] != cards[i]))
      {
        ++display_count;
      }
    }

    valid_count = (display_count - 1) / 5;
    if (valid_count == 0)
    {
      valid_count = 1;
    }
    y_step = 0x54 / valid_count;

    x = 0x60;
    valid_count = 0;
    y = 0x10;
    for (i = 0; i < count; ++i)
    {
      if (cards[i] != -1 && (i == 0 || cards[i - 1] != cards[i]))
      {
        x_positions[valid_count] = x + 4;
        y_positions[valid_count] = y;
        valid_graveyards[valid_count] = i;
        ++valid_count;
        x += 0x38;
        if (x >= 0x140)
        {
          x = 0x60;
          y += y_step;
        }
      }
    }

    return selected_index;
  }
  else
  {
    result = show_cardlist_if_human(cards, count, title, require_selection, prompt);
    if ((player == g_active_player) && ((g_duel_network_flags & 2) != 0))
    {
      g_network_result_packet.result = result;
      g_network_result_packet.packet_type = 0x19;
      TENTATIVE_send_network_result(player, 0x19);
    }
  }

  return result;
}
#endif

// FUNCTION: MAGIC 0x004bd7d0
// FUNCTION: SHANDALAR 0x004be650
int real_target_available(int *num_valid_targets,
                          target_scan_mode_t target_source_mode,
                          int who_chooses,
                          unsigned int allowed_controller,
                          unsigned int preferred_controller,
                          target_zone_t zone,
                          type_t required_type,
                          type_t illegal_type,
                          keyword_t required_abilities,
                          keyword_t illegal_abilities,
                          color_test_t required_color,
                          color_test_t illegal_color,
                          int extra,
                          subtype_in_card_data_t required_subtype,
                          int required_power,
                          int required_toughness,
                          target_special_t special,
                          target_state_t required_state,
                          target_state_t illegal_state)
{
  int target_card;
  int player_index;
  int target_is_valid;
  int found_any;
  int stop_on_first;
  int count;
  int done;
  int target_player;
  int current_card;
  int scan_player;

  stop_on_first = num_valid_targets == NULL;
  count = 0;
  found_any = 0;

  if (target_source_mode != 0 && target_source_mode != 1 && target_source_mode != 2)
  {
    return 0;
  }

  done = 0;
  for (player_index = 0; player_index < 2; ++player_index)
  {
    if (C_real_validate_target(player_index,
                               -1,
                               (char *)0,
                               who_chooses,
                               allowed_controller,
                               preferred_controller,
                               zone,
                               required_type,
                               illegal_type,
                               required_abilities,
                               illegal_abilities,
                               required_color,
                               illegal_color,
                               extra,
                               required_subtype,
                               required_power,
                               required_toughness,
                               special,
                               required_state,
                               illegal_state) != 0)
    {
      found_any = 1;
      ++count;
      if (stop_on_first)
      {
        done = 1;
      }
    }
  }

  if (g_active_player == who_chooses || (g_duel_network_flags & 2) != 0)
  {
    if ((allowed_controller & 2) == 0)
    {
      scan_player = 1;
    }
    else
    {
      scan_player = 0;
    }
  }
  else if ((preferred_controller & 2) != 0 || (preferred_controller & 1) != 0)
  {
    scan_player = 1;
  }
  else
  {
    scan_player = 0;
  }

  for (player_index = 0;
       player_index < 2;
       ++player_index, scan_player = 1 - scan_player)
  {
    if (!done)
    {
      for (current_card = 0;
           current_card < (g_active_cards_count[1] > g_active_cards_count[0] ? g_active_cards_count[1]
                                                                             : g_active_cards_count[0]) &&
           !done;
           ++current_card)
      {
        if (PLAYER_CARD_INSTANCE(scan_player, current_card).internal_card_id != -1)
        {
          if (target_source_mode == 0)
          {
            target_player = scan_player;
            target_card = current_card;
            target_is_valid = 1;
          }
          else if (target_source_mode == 1)
          {
            if (PLAYER_CARD_INSTANCE(scan_player, current_card).internal_card_id == g_damage_card_internal_card_id)
            {
              target_player = (char)PLAYER_CARD_INSTANCE(scan_player, current_card).damage_target_player;
              target_card = PLAYER_CARD_INSTANCE(scan_player, current_card).damage_target_card;
              target_is_valid = 1;
            }
            else
            {
              target_is_valid = 0;
            }
          }
          else if (PLAYER_CARD_INSTANCE(scan_player, current_card).internal_card_id == g_damage_card_internal_card_id)
          {
            target_player = (char)PLAYER_CARD_INSTANCE(scan_player, current_card).damage_source_player;
            target_card = PLAYER_CARD_INSTANCE(scan_player, current_card).damage_source_card;
            target_is_valid = 1;
          }
          else
          {
            target_is_valid = 0;
          }

          if (target_is_valid && C_real_validate_target(target_player,
                                                        target_card,
                                                        (char *)0,
                                                        who_chooses,
                                                        allowed_controller,
                                                        preferred_controller,
                                                        zone,
                                                        required_type,
                                                        illegal_type,
                                                        required_abilities,
                                                        illegal_abilities,
                                                        required_color,
                                                        illegal_color,
                                                        extra,
                                                        required_subtype,
                                                        required_power,
                                                        required_toughness,
                                                        special,
                                                        required_state,
                                                        illegal_state) != 0)
          {
            found_any = 1;
            ++count;
            if (stop_on_first)
            {
              done = 1;
            }
          }
        }
      }
    }
  }

  if (num_valid_targets != NULL)
  {
    *num_valid_targets = count;
  }
  return found_any;
}

// FUNCTION: MAGIC 0x004bdc06
// FUNCTION: SHANDALAR 0x004bea85
unsigned int C_real_validate_target(int tgt_player,
                                    int tgt_card,
                                    char *return_error_str,
                                    int who_chooses,
                                    int allowed_controller,
                                    int preferred_controller,
                                    target_zone_t zone,
                                    type_t required_type,
                                    type_t illegal_type,
                                    keyword_t required_abilities,
                                    keyword_t illegal_abilities,
                                    color_test_t required_color,
                                    color_test_t illegal_color,
                                    int extra,
                                    subtype_in_card_data_t required_subtype,
                                    int power_requirement,
                                    int toughness_requirement,
                                    target_special_t special,
                                    target_state_t required_state,
                                    target_state_t illegal_state)
{
  /* Keep locals as a single struct to stabilize MSVC 4.20 /Od stack layout. */
  int tmp124; /* local_124 */
  int tmp120; /* local_120 */
  int unused_11c;
  int tmp118; /* local_118 */
  int tmp114; /* local_114 */
  int unused_110;
  int is_not_blocking;                /* local_10c */
  int is_not_attacking;               /* local_108 */
  unsigned int toughness_pt_val;      /* local_104 */
  unsigned int toughness_pt_cmp;      /* local_100 */
  unsigned int power_pt_val;          /* local_fc */
  unsigned int power_pt_cmp;          /* local_f8 */
  subtype_in_card_data_t alt_subtype; /* local_f4 */
  int illegal_type_match;             /* local_f0 */
  int illegal_type_iid;               /* local_ec */
  int type_match;                     /* local_e8 */
  int type_iid;                       /* local_e4 */
  int required_controller;            /* local_e0 */
  int required_owner;                 /* local_dc */
  int player_allowed_ai;              /* local_d8 */
  int player_allowed_human;           /* local_d4 */
  int is_illegal;                     /* bVar20 */
  char errbuf[200];                   /* local_d0 */
  unsigned int is_valid;              /* local_4 */

  if (tgt_player == -1)
  {
    if (return_error_str != NULL)
    {
      // STRING: SHANDALAR 0x005899a0
      strcpy(return_error_str, "");
    }
    return 0;
  }

  if (tgt_player != -1 && tgt_card != -1)
  {
    if (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).internal_card_id == -1)
    {
      if (return_error_str != NULL)
      {
        // STRING: SHANDALAR 0x005899a4
        strcpy(return_error_str, "");
      }
      return 0;
    }
  }

  is_illegal = 0;
  // STRING: SHANDALAR 0x005899a8
  strcpy(errbuf, "");

  if (tgt_card == -1)
  {
    if (zone == 0 || (zone & TARGET_ZONE_PLAYERS) != 0)
    {
      if ((((g_active_player == who_chooses) || ((g_duel_network_flags & 2) != 0)) && ((allowed_controller & ANYBODY) != 0)) || (who_chooses == AI && ((preferred_controller & ANYBODY) != 0)))
      {
        player_allowed_human = AI;
        player_allowed_ai = 1;
      }
      else if ((((g_active_player == who_chooses) || ((g_duel_network_flags & 2) != 0)) && ((allowed_controller & AI) != 0)) || (who_chooses == AI && ((preferred_controller & AI) != 0)))
      {
        player_allowed_human = HUMAN;
        player_allowed_ai = 1;
      }
      else
      {
        if ((g_active_player == who_chooses) || ((g_duel_network_flags & 2) != 0))
        {
        }
        else
        {
          player_allowed_human = AI;
          player_allowed_ai = 0;
        }
      }
    }
    else
    {
      player_allowed_human = HUMAN;
      player_allowed_ai = 0;
    }

    if ((tgt_player == HUMAN && player_allowed_human == 0) || (tgt_player == AI && player_allowed_ai == 0))
    {
      is_illegal = 1;
      strcat(errbuf, gs_illegal_target_why_player_008b3110);
    }
  }
  else
  {
    if ((g_active_player == who_chooses) || ((g_duel_network_flags & 2) != 0))
    {
      if ((allowed_controller & TARGET_PLAYER_OWNER) != 0)
      {
        if ((allowed_controller & ANYBODY) != 0)
        {
          required_owner = -1;
        }
        else if ((allowed_controller & AI) != 0)
        {
          required_owner = AI;
        }
        else
        {
          required_owner = HUMAN;
        }

        if ((allowed_controller & TARGET_PLAYER_OWNER_AND_CONTROLLER) == TARGET_PLAYER_OWNER_AND_CONTROLLER)
        {
          required_controller = required_owner;
        }
        else
        {
          required_controller = -1;
        }
      }
      else
      {
        if ((allowed_controller & ANYBODY) != 0)
        {
          required_controller = -1;
        }
        else if ((allowed_controller & AI) != 0)
        {
          required_controller = AI;
        }
        else
        {
          required_controller = HUMAN;
        }

        if ((allowed_controller & TARGET_PLAYER_OWNER_AND_CONTROLLER) == TARGET_PLAYER_OWNER_AND_CONTROLLER)
        {
          required_owner = required_controller;
        }
        else
        {
          required_owner = -1;
        }
      }
    }
    else
    {
      if ((preferred_controller & TARGET_PLAYER_OWNER) != 0)
      {
        if ((preferred_controller & ANYBODY) != 0)
        {
          required_owner = -1;
        }
        else if ((preferred_controller & AI) != 0)
        {
          required_owner = AI;
        }
        else
        {
          required_owner = HUMAN;
        }

        if ((preferred_controller & TARGET_PLAYER_OWNER_AND_CONTROLLER) == TARGET_PLAYER_OWNER_AND_CONTROLLER)
        {
          required_controller = required_owner;
        }
        else
        {
          required_controller = -1;
        }
      }
      else
      {
        if ((preferred_controller & ANYBODY) != 0)
        {
          required_controller = -1;
        }
        else if ((preferred_controller & AI) != 0)
        {
          required_controller = AI;
        }
        else
        {
          required_controller = HUMAN;
        }

        if ((preferred_controller & TARGET_PLAYER_OWNER_AND_CONTROLLER) == TARGET_PLAYER_OWNER_AND_CONTROLLER)
        {
          required_owner = required_controller;
        }
        else
        {
          required_owner = -1;
        }
      }
    }

    if ((PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_OUBLIETTED) != 0)
    {
      is_illegal = 1;
      strcat(errbuf, gs_illegal_target_why_cant_target_this_0091a6e0);
    }

    if ((PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_CANNOT_TARGET) != 0)
    {
      is_illegal = 1;
      strcat(errbuf, gs_illegal_target_why_cant_target_this_0091a6e0);
    }

    if (zone != 0 && (((PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_IN_PLAY) != 0 && (zone & TARGET_ZONE_IN_PLAY) == 0) || ((PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_IN_PLAY) == 0 && (zone & TARGET_ZONE_HAND) == 0)))
    {
      is_illegal = 1;
      strcat(errbuf, gs_illegal_target_why_where_0091cd00);
    }

    if (required_controller != -1 && tgt_player != required_controller)
    {
      is_illegal = 1;
      strcat(errbuf, gs_illegal_target_why_controller_0091bbe0);
    }

    if (required_owner != -1)
    {
      if (((PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_OWNED_BY_OPPONENT) != 0 && required_owner == HUMAN) ||
          ((PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_OWNED_BY_OPPONENT) == 0 && required_owner != HUMAN))
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_owner_007aacc0);
      }
    }

    if (required_type != TYPE_NONE)
    {
      if ((special & TARGET_SPECIAL_USE_ORIGINAL_TYPE) != 0)
      {
        type_iid = PLAYER_CARD_INSTANCE(tgt_player, tgt_card).original_internal_card_id;
      }
      else
      {
        type_iid = PLAYER_CARD_INSTANCE(tgt_player, tgt_card).internal_card_id;
      }

      type_match = 0;
      if (((required_type & global_cards_data[type_iid].type) != TYPE_NONE) || ((required_type & TARGET_TYPE_NONCREATURE_CAN_BLOCK) != TYPE_NONE && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_NONCREATURE_CAN_BLOCK) != 0))
      {
        type_match = 1;
      }
      if ((required_type & TYPE_EFFECT) != TYPE_NONE && (global_cards_data[type_iid].id == g_effect_display_internal_card_id || global_cards_data[PLAYER_CARD_INSTANCE(tgt_player, tgt_card).internal_card_id].id == g_hunting_display_internal_card_id))
      {
        type_match = 1;
      }
      if ((required_type & TARGET_TYPE_TOKEN) != TYPE_NONE && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).token_status & STATUS_TOKEN) != 0)
      {
        type_match = 1;
      }
      if ((required_type & TARGET_TYPE_DAMAGE_LEGACY) != TYPE_NONE && global_cards_data[type_iid].id == g_damage_display_internal_card_id)
      {
        type_match = 1;
      }
      if ((required_type & TARGET_TYPE_HACK_SLEIGHT_LEGACY) != TYPE_NONE && global_cards_data[type_iid].id == g_legacy_display_internal_card_id)
      {
        type_match = 1;
      }
      if ((required_type & TARGET_TYPE_DRAW_CARD_LEGACY) != TYPE_NONE && global_cards_data[type_iid].id == g_card_back_display_internal_card_id)
      {
        type_match = 1;
      }
      if (!type_match)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_type_008a9a00);
      }
    }

    if (illegal_type != TYPE_NONE)
    {
      if ((special & TARGET_SPECIAL_USE_ORIGINAL_TYPE) != 0)
      {
        illegal_type_iid = PLAYER_CARD_INSTANCE(tgt_player, tgt_card).original_internal_card_id;
      }
      else
      {
        illegal_type_iid = PLAYER_CARD_INSTANCE(tgt_player, tgt_card).internal_card_id;
      }

      illegal_type_match = 0;
      if ((illegal_type & global_cards_data[illegal_type_iid].type) != TYPE_NONE)
      {
        illegal_type_match = 1;
      }
      if ((illegal_type & TYPE_EFFECT) != TYPE_NONE && (global_cards_data[illegal_type_iid].id == g_effect_display_internal_card_id || global_cards_data[illegal_type_iid].id == g_hunting_display_internal_card_id))
      {
        illegal_type_match = 1;
      }
      if ((illegal_type & TARGET_TYPE_TOKEN) != TYPE_NONE && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).token_status & STATUS_TOKEN) != 0)
      {
        illegal_type_match = 1;
      }
      if ((illegal_type & TARGET_TYPE_DAMAGE_LEGACY) != TYPE_NONE && global_cards_data[illegal_type_iid].id == g_damage_display_internal_card_id)
      {
        illegal_type_match = 1;
      }
      if ((illegal_type & TARGET_TYPE_HACK_SLEIGHT_LEGACY) != TYPE_NONE && global_cards_data[illegal_type_iid].id == g_legacy_display_internal_card_id)
      {
        illegal_type_match = 1;
      }
      if ((illegal_type & TARGET_TYPE_DRAW_CARD_LEGACY) != TYPE_NONE && global_cards_data[illegal_type_iid].id == g_card_back_display_internal_card_id)
      {
        illegal_type_match = 1;
      }
      if (illegal_type_match)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_type_008a9a00);
      }
    }

    if (required_abilities != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).regen_status & required_abilities) != required_abilities)
    {
      is_illegal = 1;
      strcat(errbuf, gs_illegal_target_why_abilities_0091a810);
    }
    if (illegal_abilities != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).regen_status & illegal_abilities) != 0)
    {
      is_illegal = 1;
      strcat(errbuf, gs_illegal_target_why_abilities_0091a810);
    }
    if (required_color != COLOR_TEST_0 && (required_color & PLAYER_CARD_INSTANCE(tgt_player, tgt_card).color) == COLOR_TEST_0)
    {
      is_illegal = 1;
      strcat(errbuf, gs_illegal_target_why_color_008ce3c0);
    }
    if (illegal_color != COLOR_TEST_0 && (illegal_color & PLAYER_CARD_INSTANCE(tgt_player, tgt_card).color) != COLOR_TEST_0)
    {
      is_illegal = 1;
      strcat(errbuf, gs_illegal_target_why_color_008ce3c0);
    }

    if (extra != -1)
    {
      if ((special & TARGET_SPECIAL_NOT_LAND_SUBTYPE) != 0)
      {
        if (card_matches_excluded_land_type(tgt_player, tgt_card, extra) != 0)
        {
          is_illegal = 1;
          strcat(errbuf, gs_illegal_target_why_card_type_008cfde0);
        }
      }
      else if ((special & TARGET_SPECIAL_BASIC_LAND) != 0)
      {
        if (basic_land_matches_type_index(PLAYER_CARD_INSTANCE(tgt_player, tgt_card).internal_card_id, extra) == 0)
        {
          is_illegal = 1;
          strcat(errbuf, gs_illegal_target_why_card_type_008cfde0);
        }
      }
      else
      {
        if (extra <= 4)
        {
          if (card_has_basic_land_type(tgt_player, tgt_card, extra + 1) == 0)
          {
            is_illegal = 1;
            strcat(errbuf, gs_illegal_target_why_card_type_008cfde0);
          }
        }
        else if (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).internal_card_id != extra && global_cards_data[extra].id != global_cards_data[PLAYER_CARD_INSTANCE(tgt_player, tgt_card).internal_card_id].id)
        {
          is_illegal = 1;
          strcat(errbuf, gs_illegal_target_why_card_type_008cfde0);
        }
      }
    }

    if (required_subtype != ~SUB_WALL)
    {
      if ((special & TARGET_SPECIAL_0x10) != 0)
      {
        if (required_subtype == (SUB_DJINN | SUB_MERFOLK))
        {
          alt_subtype = SUB_RAT;
        }
        else if (required_subtype == SUB_RAT)
        {
          alt_subtype = SUB_DJINN | SUB_MERFOLK;
        }
        else if (required_subtype == SUB_ENCHANT_WORLD)
        {
          alt_subtype = SUB_ENCHANT_WORLD | SUB_MERFOLK;
        }
        else if (required_subtype == (SUB_ENCHANT_WORLD | SUB_MERFOLK))
        {
          alt_subtype = SUB_ENCHANT_WORLD;
        }
        else if (required_subtype == 0x49)
        {
          alt_subtype = 0x4a;
        }
        else if (required_subtype == 0x4a)
        {
          alt_subtype = 0x49;
        }
        else if (required_subtype == 0x56)
        {
          alt_subtype = 0x57;
        }
        else if (required_subtype == 0x57)
        {
          alt_subtype = 0x56;
        }
        else if (required_subtype == 0x59)
        {
          alt_subtype = 0x5a;
        }
        else if (required_subtype == 0x5a)
        {
          alt_subtype = 0x59;
        }
        else if (required_subtype == 0x69)
        {
          alt_subtype = 0x6a;
        }
        else if (required_subtype == 0x6a)
        {
          alt_subtype = 0x69;
        }
        else
        {
          alt_subtype = required_subtype;
        }
      }
      else
      {
        alt_subtype = required_subtype;
      }

      if ((subtype_in_card_data_t)global_raw_cards_storage[global_cards_data[PLAYER_CARD_INSTANCE(tgt_player, tgt_card).internal_card_id].id].subtype != required_subtype && (subtype_in_card_data_t)global_raw_cards_storage[global_cards_data[PLAYER_CARD_INSTANCE(tgt_player, tgt_card).internal_card_id].id].subtype != alt_subtype)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_subtype_008cea20);
      }
    }

    if (power_requirement != -1)
    {
      power_pt_val = power_requirement & TARGET_PT_MASK;
      power_pt_cmp = power_requirement & 0xf000;
      if ((power_pt_cmp == 0 && PLAYER_CARD_INSTANCE(tgt_player, tgt_card).power != (int)power_pt_val) || (power_pt_cmp == TARGET_PT_GREATER_OR_EQUAL && PLAYER_CARD_INSTANCE(tgt_player, tgt_card).power < (int)power_pt_val) || (power_pt_cmp == TARGET_PT_LESSER_OR_EQUAL && PLAYER_CARD_INSTANCE(tgt_player, tgt_card).power > (int)power_pt_val))
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_power_00781aa0);
      }
    }

    if (toughness_requirement != -1)
    {
      toughness_pt_val = toughness_requirement & TARGET_PT_MASK;
      toughness_pt_cmp = toughness_requirement & 0xf000;
      if ((toughness_pt_cmp == 0 && PLAYER_CARD_INSTANCE(tgt_player, tgt_card).toughness != (int)toughness_pt_val) || (toughness_pt_cmp == TARGET_PT_GREATER_OR_EQUAL && PLAYER_CARD_INSTANCE(tgt_player, tgt_card).toughness < (int)toughness_pt_val) || (toughness_pt_cmp == TARGET_PT_LESSER_OR_EQUAL && PLAYER_CARD_INSTANCE(tgt_player, tgt_card).toughness > (int)toughness_pt_val))
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_toughness_007894b0);
      }
    }

    if (special != 0)
    {
      if ((special & TARGET_SPECIAL_WALL) != 0 && (subtype_in_card_data_t)global_raw_cards_storage[global_cards_data[PLAYER_CARD_INSTANCE(tgt_player, tgt_card).internal_card_id].id].subtype != 0xc5 && global_cards_data[PLAYER_CARD_INSTANCE(tgt_player, tgt_card).internal_card_id].subtype != 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_walls_008b34c0);
      }
      if ((special & TARGET_SPECIAL_NON_WALL) != 0 && ((subtype_in_card_data_t)global_raw_cards_storage[global_cards_data[PLAYER_CARD_INSTANCE(tgt_player, tgt_card).internal_card_id].id].subtype == 0xc5 || global_cards_data[PLAYER_CARD_INSTANCE(tgt_player, tgt_card).internal_card_id].subtype == 0))
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_walls_008b34c0);
      }
      if ((special & TARGET_SPECIAL_SPELL_ON_STACK) != 0 && ((g_current_spell_player == -1 || tgt_player != g_current_spell_player) || tgt_card != g_current_spell_card || (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_SUMMONSICK) != 0))
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_spell_0091be40);
      }
      if ((special & TARGET_SPECIAL_BASIC_LAND) != 0 && is_basic_land(tgt_player, tgt_card) == 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_basic_land_0093d860);
      }
      if ((special & TARGET_SPECIAL_ARTIFACT_CREATURE) != 0 && (global_cards_data[PLAYER_CARD_INSTANCE(tgt_player, tgt_card).internal_card_id].type & (TYPE_ARTIFACT | TYPE_CREATURE)) != (TYPE_ARTIFACT | TYPE_CREATURE))
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_artifact_creature_008ced40);
      }
      if ((special & TARGET_SPECIAL_DAMAGE_PLAYER) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).damage_target_player != who_chooses || PLAYER_CARD_INSTANCE(tgt_player, tgt_card).damage_target_card != -1))
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_target_player_0091c360);
      }
      if ((special & TARGET_SPECIAL_DJINN_OR_EFREET) != 0 && global_cards_data[PLAYER_CARD_INSTANCE(tgt_player, tgt_card).internal_card_id].subtype != 6 && global_cards_data[PLAYER_CARD_INSTANCE(tgt_player, tgt_card).internal_card_id].subtype != 5)
      {
        is_illegal = 1;
        strcat(errbuf, ",djinn/efreet");
      }
    }

    if (required_state != 0)
    {
      if ((required_state & TARGET_STATE_TAPPED) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_TAPPED) == 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_tapped_00925080);
      }
      if ((required_state & TARGET_STATE_ATTACKING) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_ATTACKING) == 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_attacking_00896540);
      }
      if ((required_state & TARGET_STATE_ATTACKED) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_ATTACKED) == 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_attacked_0091bfc0);
      }
      if ((required_state & TARGET_STATE_ISBLOCKED) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_ISBLOCKED) == 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_blocked_0091caa0);
      }
      if ((required_state & TARGET_STATE_BLOCKING) != 0 && (tgt_player == g_current_player || (int)(char)PLAYER_CARD_INSTANCE(tgt_player, tgt_card).blocking == -1))
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_blocking_007ab160);
      }
      if ((required_state & TARGET_STATE_IN_COMBAT) != 0)
      {
        is_not_blocking = 0;
        is_not_attacking = is_not_blocking;

        if ((PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_ATTACKING) == 0)
        {
          is_not_attacking = 1;
        }

        if (g_current_phase > 0x14 && g_current_phase < 0x1e && tgt_player != g_current_player)
        {
          if (tgt_player == g_current_player || (int)(char)PLAYER_CARD_INSTANCE(tgt_player, tgt_card).blocking == -1)
          {
            is_not_blocking = 1;
          }
        }
        else
        {
          is_not_blocking = 1;
        }

        if (is_not_attacking != 0)
        {
          if (is_not_blocking != 0)
          {
            is_illegal = 1;
            strcat(errbuf, gs_illegal_target_why_attacking_blocking_008cc850);
          }
        }
      }
      if ((required_state & TARGET_STATE_ENCHANTED) != 0)
      {
        tmp118 = 0;
        for (unused_110 = 0; unused_110 < 2; ++unused_110)
        {
          for (tmp114 = 0; tmp114 < g_active_cards_count[unused_110]; ++tmp114)
          {
            if (global_card_instances[unused_110][tmp114].internal_card_id != -1 &&
                (global_cards_data[global_card_instances[unused_110][tmp114].internal_card_id].type & TYPE_ENCHANTMENT) != 0 &&
                global_card_instances[unused_110][tmp114].damage_target_player == tgt_player &&
                global_card_instances[unused_110][tmp114].damage_target_card == tgt_card)
            {
              tmp118 = 1;
            }
          }
        }
        if (!tmp118)
        {
          is_illegal = 1;
          strcat(errbuf, gs_illegal_target_why_enchanted_008cf3c0);
        }
      }
      if ((required_state & TARGET_STATE_JUST_CAST) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_SUMMONSICK) == 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_casted_00939050);
      }
      if ((required_state & TARGET_STATE_SPELL_RESOLVED) != 0 && ((PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_SUMMONSICK) == 0 || (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_INVISIBLE) != 0))
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_cast_resolved_008ce6a0);
      }
      if ((required_state & TARGET_STATE_DAMAGED) != 0 && has_pending_damage(tgt_player, tgt_card) == 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_damaged_007911b0);
      }
      if ((required_state & TARGET_STATE_COULD_UNTAP) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).untap_status & UNTAP_STATUS_COULD_UNTAP) == 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_can_untap_008b3be0);
      }
      if ((required_state & TARGET_STATE_WILL_UNTAP) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).untap_status & UNTAP_STATUS_WILL_UNTAP) == 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_will_untap_008ceb50);
      }
      if ((required_state & TARGET_STATE_SUMMONING_SICK) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_SUMMONSICK_BOTH) == STATE_SUMMONSICK_BOTH)
      {
        is_illegal = 1;
        strcat(errbuf, ",summoning sickness");
      }
    }

    if (illegal_state != 0)
    {
      if ((illegal_state & TARGET_STATE_TAPPED) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_TAPPED) != 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_tapped_00925080);
      }
      if ((((illegal_state & TARGET_STATE_ATTACKING) != 0) || ((illegal_state & TARGET_STATE_IN_COMBAT) != 0)) && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_ATTACKING) != 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_attacking_00896540);
      }
      if ((illegal_state & TARGET_STATE_ATTACKED) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_ATTACKED) != 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_attacked_0091bfc0);
      }
      if ((illegal_state & TARGET_STATE_ISBLOCKED) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_ISBLOCKED) != 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_blocked_0091caa0);
      }
      if (((illegal_state & TARGET_STATE_BLOCKING) != 0) || ((illegal_state & TARGET_STATE_IN_COMBAT) != 0))
      {
        if ((int)(char)PLAYER_CARD_INSTANCE(tgt_player, tgt_card).blocking != -1 && tgt_player != g_current_player)
        {
          is_illegal = 1;
          strcat(errbuf, gs_illegal_target_why_blocking_007ab160);
        }
      }
      if ((illegal_state & TARGET_STATE_ENCHANTED) != 0)
      {
        tmp124 = 0;
        for (unused_11c = 0; unused_11c < 2; ++unused_11c)
        {
          for (tmp120 = 0; tmp120 < g_active_cards_count[unused_11c]; ++tmp120)
          {
            if (global_card_instances[unused_11c][tmp120].internal_card_id != -1 &&
                (global_cards_data[global_card_instances[unused_11c][tmp120].internal_card_id].type & TYPE_ENCHANTMENT) != 0 &&
                global_card_instances[unused_11c][tmp120].damage_target_player == tgt_player &&
                global_card_instances[unused_11c][tmp120].damage_target_card == tgt_card)
            {
              tmp124 = 1;
            }
          }
        }
        if (tmp124)
        {
          is_illegal = 1;
          strcat(errbuf, gs_illegal_target_why_enchanted_008cf3c0);
        }
      }
      if ((illegal_state & TARGET_STATE_JUST_CAST) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_SUMMONSICK) != 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_casted_00939050);
      }
      if ((illegal_state & TARGET_STATE_SPELL_RESOLVED) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_SUMMONSICK) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_INVISIBLE) == 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_cast_resolved_008ce6a0);
      }
      if ((illegal_state & TARGET_STATE_DAMAGED) != 0 && has_pending_damage(tgt_player, tgt_card) != 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_damaged_007911b0);
      }
      if ((illegal_state & TARGET_STATE_COULD_UNTAP) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).untap_status & UNTAP_STATUS_COULD_UNTAP) != 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_can_untap_008b3be0);
      }
      if ((illegal_state & TARGET_STATE_WILL_UNTAP) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).untap_status & UNTAP_STATUS_WILL_UNTAP) != 0)
      {
        is_illegal = 1;
        strcat(errbuf, gs_illegal_target_why_will_untap_008ceb50);
      }
      if ((illegal_state & TARGET_STATE_SUMMONING_SICK) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & STATE_SUMMONSICK_BOTH) != 0)
      {
        is_illegal = 1;
        strcat(errbuf, ",summoning sickness");
      }
    }
  }

  if (is_illegal != 0)
  {
    is_valid = 0;
  }
  else
  {
    is_valid = 1;
  }
  if (return_error_str != NULL)
  {
    if (errbuf[0] == '\0')
    {
      *return_error_str = '\0';
    }
    else
    {
      strcpy(return_error_str, errbuf + 1);
    }
  }

  return is_valid;
}

// FUNCTION: MAGIC 0x004e9c50
// FUNCTION: SHANDALAR 0x00507a50
int select_card_for_action(int player,
                           int arg_2,
                           int player_to_check,
                           unsigned int required_type,
                           unsigned int required_color,
                           char *prompt,
                           int dialog_mode)
{
  unsigned int modal_required_type; /* ebp - 0x20c */
  int stack_card;                   /* ebp - 0x208 */
  int action_msg;                   /* ebp - 0x204 */
  int stack_player;                 /* ebp - 0x200 */
  int candidate_cards[60];          /* ebp - 0x1fc */
  int candidate_count;              /* ebp - 0x10c (reused as random candidate index) */
  target_t selected_target;         /* ebp - 0x108 */
  int action;                       /* ebp - 0x100 (get_current_stack_action() result) */
  int current_card;                 /* ebp - 0xfc */
  int current_player;               /* ebp - 0xf8 */
  int selected_internal_id;         /* ebp - 0xf4 */
  int candidate_players[60];        /* ebp - 0xf0 */

  if (g_spell_fizzled == 1 || (g_duel_ai_mode_state == 1 && g_active_player == player))
  {
    return -1;
  }

  if ((g_ai_search_flags & 1) != 0)
  {
    player_to_check = arg_2;
  }

  if ((player == g_other_player && (g_duel_network_flags & 2) == 0) || g_duel_ai_mode_state == 1 || g_duel_network_state != 0)
  {
    candidate_count = 0;

    for (current_player = 0; current_player < 2; current_player++)
    {
      if (player_to_check != -1 && player_to_check != current_player)
      {
        continue;
      }

      for (current_card = 0; current_card < g_active_cards_count[current_player]; current_card++)
      {
        if (required_type == (unsigned int)-2 || PLAYER_CARD_INSTANCE(current_player, current_card).internal_card_id == -1)
        {
          continue;
        }

        if (((PLAYER_CARD_INSTANCE(current_player, current_card).state & (STATE_OUBLIETTED | STATE_IN_PLAY)) != STATE_IN_PLAY) &&
            !(g_active_player == player && g_duel_network_state != 0))
        {
          continue;
        }

        if (!(((int)required_type <= 0) ||
              ((required_type &
                (unsigned char)global_cards_data[PLAYER_CARD_INSTANCE(current_player, current_card).internal_card_id].type) != 0)))
        {
          continue;
        }

        if (!((required_color == 1 || required_color == 0) ||
              (((int)required_color & (int)PLAYER_CARD_INSTANCE(current_player, current_card).mana_color) != 0)))
        {
          continue;
        }

        candidate_players[candidate_count] = current_player;
        candidate_cards[candidate_count] = current_card;
        candidate_count++;
      }

      if (((int)required_type <= 0) && (required_color == 1 || required_color == 0))
      {
        candidate_players[candidate_count] = current_player;
        candidate_cards[candidate_count] = -1;
        candidate_count++;
      }
    }

    if (candidate_count == 0)
    {
      return -1;
    }

    if (g_active_player == player)
    {
    select_card_for_action_retry:
      candidate_count = internal_rand(candidate_count);
      g_target_player_choice = candidate_players[candidate_count];

      if (g_duel_network_state != 0)
      {
        g_target_selection_status_code = 0;
        if (internal_rand(0x20) == 0 || g_response_selection_in_progress != 0)
        {
          g_target_selection_status_code = (int)0xfffffffeU;
          g_stop_phase = -1;
          g_stop_phase_player = g_stop_phase;
          return -1;
        }

        if (g_active_player != g_target_player_choice)
        {
          goto select_card_for_action_retry;
        }

        selected_internal_id = PLAYER_CARD_INSTANCE(g_target_player_choice, candidate_cards[candidate_count]).internal_card_id;
        if (((global_cards_data[selected_internal_id].type & 1) != 0) &&
            ((PLAYER_CARD_INSTANCE(g_target_player_choice, candidate_cards[candidate_count]).state & STATE_IN_PLAY) != 0))
        {
          goto select_card_for_action_retry;
        }
        if (((global_cards_data[selected_internal_id].type & 2) != 0) &&
            ((PLAYER_CARD_INSTANCE(g_target_player_choice, candidate_cards[candidate_count]).state & STATE_ATTACKING) != 0))
        {
          goto select_card_for_action_retry;
        }

        if (g_current_phase > 0x14 && g_current_phase < 0x1e)
        {
          if ((global_cards_data[selected_internal_id].type & 2) == 0 || (PLAYER_CARD_INSTANCE(g_target_player_choice, candidate_cards[candidate_count]).state & STATE_IN_PLAY) == 0)
          {
            goto select_card_for_action_retry;
          }
          goto select_card_for_action_return_candidate;
        }

        if ((global_cards_data[selected_internal_id].type & 0x4b) == 0 || (PLAYER_CARD_INSTANCE(g_target_player_choice, candidate_cards[candidate_count]).state & STATE_IN_PLAY) != 0)
        {
          goto select_card_for_action_retry;
        }
      }

    select_card_for_action_return_candidate:
      return candidate_cards[candidate_count];
    }

    if (g_duel_ai_mode_state == 1)
    {
      g_ai_recorded_choice = internal_rand(candidate_count);
      g_ai_recorded_action = ((((candidate_players[g_ai_recorded_choice] == 0) - 1) & 0x100) |
                              (candidate_cards[g_ai_recorded_choice] & 0xff) |
                              0x4000);
      record_ai_action_selection();
    }
    else
    {
      replay_ai_action_selection();
      if (g_ai_recorded_choice == 99)
      {
        g_ai_recorded_choice = internal_rand(candidate_count);
      }
    }

    g_target_player_choice = candidate_players[g_ai_recorded_choice];
    return candidate_cards[g_ai_recorded_choice];
  }

  if (required_type != 0 && required_type != 0xff)
  {
    strcpy(g_ui_message_buffer, "");
    action = get_current_stack_action();
    if (action != -1)
    {
      action_msg = ((unsigned int)action >> 0x10) & 0xff;
      stack_player = global_stack_cards[g_stack_size - 1].player;
      stack_card = global_stack_cards[g_stack_size - 1].card;
      format_stack_action_text(g_ui_message_buffer,
                               action_msg,
                               stack_player,
                               stack_card);
    }
  }

  if (required_type == 0 || required_type == 0xff || required_type == (unsigned int)-2)
  {
    modal_required_type = 0xffffffffU;
  }
  else
  {
    modal_required_type = required_type;
  }

  run_target_selection_modal(player,
                             arg_2,
                             prompt,
                             dialog_mode,
                             modal_required_type,
                             required_color,
                             0xffffffff,
                             0xffffffff,
                             &g_target_selection_status_code,
                             &selected_target,
                             0,
                             0);

  strcpy(g_text_lines[0], "");
  if (selected_target.player != -1)
  {
    g_ai_action_replay_available = 0;
  }

  g_target_player_choice = selected_target.player;
  return selected_target.card;
}

// FUNCTION: MAGIC 0x00551638
// FUNCTION: SHANDALAR 0x0049f87b
int select_target_creature_and_store(int player, unsigned int preferred_controller, int card)
{
  target_t selected_target;

  if (preferred_controller == -1)
  {
    preferred_controller = 2;
  }

  if (C_real_select_target(player,
                           2,
                           preferred_controller,
                           TARGET_ZONE_IN_PLAY,
                           TYPE_CREATURE,
                           TYPE_NONE,
                           0,
                           get_protections_from(player, card),
                           0,
                           0,
                           -1,
                           ~SUB_WALL,
                           -1,
                           -1,
                           0,
                           0,
                           0,
                           g_text_lines[0],
                           1,
                           &selected_target) != 0)
  {
    global_card_instances[player][card].targets[(char)global_card_instances[player][card].number_of_targets].card = selected_target.card;
    global_card_instances[player][card].targets[(char)global_card_instances[player][card].number_of_targets].player = selected_target.player;
    ++global_card_instances[player][card].number_of_targets;
    return 1;
  }
  else
  {
    return 0;
  }
}

// FUNCTION: MAGIC 0x005517b4
// FUNCTION: SHANDALAR 0x0049f9f7
int select_target_creature_and_store_without_protection(int player, unsigned int preferred_controller, int card)
{
  target_t selected_target;

  if (preferred_controller == -1)
  {
    preferred_controller = 2;
  }

  if (C_real_select_target(player,
                           2,
                           preferred_controller,
                           TARGET_ZONE_IN_PLAY,
                           TYPE_CREATURE,
                           TYPE_NONE,
                           0,
                           0,
                           COLOR_TEST_0,
                           COLOR_TEST_0,
                           -1,
                           -1,
                           -1,
                           -1,
                           0,
                           0,
                           0,
                           g_text_lines[0],
                           1,
                           &selected_target) != 0)
  {
    SET_TARGET(PLAYER_CARD_INSTANCE(player, card).targets[(char)PLAYER_CARD_INSTANCE(player, card).number_of_targets], selected_target);
    ++PLAYER_CARD_INSTANCE(player, card).number_of_targets;
    return 1;
  }

  return 0;
}

// FUNCTION: MAGIC 0x00551921
// FUNCTION: SHANDALAR 0x0049fb64
int choose_creature_to_sacrifice(int player)
{
  int best_card;
  target_t target;
  int current_card;
  int best_value;
  int internal_card_id;
  int card_value;

  if ((player == g_active_player || (g_duel_network_flags & 2) != 0) && g_duel_ai_mode_state != 1)
  {
    if (!C_real_select_target(player, player, player, TARGET_ZONE_IN_PLAY, TYPE_CREATURE, TYPE_NONE, 0, 0,
                              COLOR_TEST_0, COLOR_TEST_0, -1, ~SUB_WALL, -1, -1, 0, 0, 0,
                              g_text_lines[0], 0, &target))
    {
      best_card = -1;
    }
    else
    {
      best_card = target.card;
    }
  }
  else
  {
    best_card = -1;
    best_value = 0x7fff;
    for (current_card = 0; current_card < g_active_cards_count[player]; ++current_card)
    {
      internal_card_id = PLAYER_CARD_INSTANCE(player, current_card).internal_card_id;
      if (internal_card_id != -1 &&
          (PLAYER_CARD_INSTANCE(player, current_card).state & (STATE_OUBLIETTED | STATE_IN_PLAY)) == STATE_IN_PLAY &&
          (global_cards_data[internal_card_id].type & TYPE_CREATURE) != 0 &&
          PLAYER_CARD_INSTANCE(player, current_card).kill_code != 3 &&
          (PLAYER_CARD_INSTANCE(player, current_card).state & STATE_CANNOT_TARGET) == 0)
      {
        card_value = C_get_abilities(player, current_card, EVENT_POWER, -1) + 2;
        card_value *= C_get_abilities(player, current_card, EVENT_TOUGHNESS, -1) + 2;
        if (card_value < best_value)
        {
          best_value = card_value;
          best_card = current_card;
        }
      }
    }
  }

  if (best_card != -1 && g_duel_ai_mode_state != 1)
  {
    play_sound_effect(WAV_SACRFICE);
  }
  return best_card;
}

// FUNCTION: MAGIC 0x00551b60
// FUNCTION: SHANDALAR 0x0049fda3
int select_target_land_and_store(int player, int preferred_controller, int card)
{
  target_t selected_target;

  if (preferred_controller == -1)
  {
    preferred_controller = 2;
  }

  if (C_real_select_target(player,
                           2,
                           preferred_controller,
                           TARGET_ZONE_IN_PLAY,
                           TYPE_LAND,
                           TYPE_NONE,
                           0,
                           get_protections_from(player, card),
                           0,
                           0,
                           -1,
                           ~SUB_WALL,
                           -1,
                           -1,
                           0,
                           0,
                           0,
                           g_text_lines[0],
                           1,
                           &selected_target) != 0)
  {
    SET_TARGET(PLAYER_CARD_INSTANCE(player, card).targets[PLAYER_CARD_INSTANCE(player, card).number_of_targets], selected_target);
    ++PLAYER_CARD_INSTANCE(player, card).number_of_targets;
    return 1;
  }

  return 0;
}

// FUNCTION: MAGIC 0x00551e49
// FUNCTION: SHANDALAR 0x004a008c
int sacrifice_a_land(int player)
{
  target_t target;

  if (C_real_select_target(player, player, player, TARGET_ZONE_IN_PLAY, TYPE_LAND, TYPE_NONE, 0, 0,
                           COLOR_TEST_0, COLOR_TEST_0, -1, ~SUB_WALL, -1, -1, 0, 0, 0,
                           g_text_lines[0], 0, &target))
  {

    if (g_duel_ai_mode_state != 1)
    {
      play_sound_effect(WAV_SACRFICE);
    }
    kill_card(target.player, target.card, KILL_SACRIFICE);

    return 1;
  }
  return 0;
}

// FUNCTION: MAGIC 0x00551ed7
// FUNCTION: SHANDALAR 0x004a011a
int select_target_artifact_and_store(int player, unsigned int preferred_controller, int card)
{
  target_t selected_target;

  if (preferred_controller == -1)
  {
    preferred_controller = 2;
  }

  if (C_real_select_target(player,
                           2,
                           preferred_controller,
                           TARGET_ZONE_IN_PLAY,
                           TYPE_ARTIFACT,
                           TYPE_NONE,
                           0,
                           get_protections_from(player, card),
                           COLOR_TEST_0,
                           COLOR_TEST_0,
                           -1,
                           ~SUB_WALL,
                           -1,
                           -1,
                           0,
                           0,
                           0,
                           g_text_lines[0],
                           1,
                           &selected_target))
  {
    global_card_instances[player][card].targets[global_card_instances[player][card].number_of_targets].player = selected_target.player;
    global_card_instances[player][card].targets[global_card_instances[player][card].number_of_targets].card = selected_target.card;
    ++global_card_instances[player][card].number_of_targets;

    return 1;
  }
  else
    return 0;
}
