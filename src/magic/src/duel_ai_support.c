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

// GLOBAL: MAGIC 0x00573080
// GLOBAL: SHANDALAR 0x00593e20
const unsigned char wizard_card_color_preference_table[18] = {
    0x00, 0x00, 0x00, 0x01, 0x04, 0x02, 0x02, 0x01, 0x05,
    0x03, 0x05, 0x04, 0x04, 0x03, 0x01, 0x05, 0x02, 0x05};

// FUNCTION: SHANDALAR 0x005225a9
int network_random_boolean(int player)
{
  int result;

  if (player == g_active_player)
  {
    result = rand() % 2;
    g_network_result_packet.result = 1 - result;
    g_network_result_packet.packet_type = 0x18;
    TENTATIVE_send_network_result(player, 0x18);
  }
  else
  {
    TENTATIVE_wait_for_network_result(player, 0x18);
    result = g_network_result_packet.result;
  }

  return result;
}

// FUNCTION: MAGIC 0x00445918
// FUNCTION: SHANDALAR 0x004111b1
int TENTATIVE_set_timestamps(int player, int card)
{
  int slot;

  for (slot = 0; slot < 500; slot++)
  {
    if (TENTATIVE_timestamp_player[slot] == -1)
    {
      TENTATIVE_timestamp_player[slot] = player;
      TENTATIVE_timestamp_card[slot] = card;
      global_card_instances[player][card].timestamp = slot;
      return slot;
    }
  }
  assert(0, __FILE__, __LINE__,
         "Timestamp table exhausted: player=%d card=%d iid=%d card_name=\"%s\" "
         "active_counts=%d,%d phase=%d ai_mode=%d stack=%d\n",
         player, card,
         (player >= 0 && player < 2 && card >= 0 && card < 151)
             ? PLAYER_CARD_INSTANCE(player, card).internal_card_id
             : -1,
         (player >= 0 && player < 2 && card >= 0 && card < 151 &&
          PLAYER_CARD_INSTANCE(player, card).internal_card_id >= 0 &&
          PLAYER_CARD_INSTANCE(player, card).internal_card_id < g_card_count)
             ? global_cards_data[PLAYER_CARD_INSTANCE(player, card).internal_card_id].name
             : "<invalid>",
         g_active_cards_count[0], g_active_cards_count[1], g_current_phase,
         g_duel_ai_mode_state, g_stack_size);
  return -1;
}

// FUNCTION: MAGIC 0x0046489e
// FUNCTION: SHANDALAR 0x0052234e
int DrawRandomCardFromInitialLibrary(int library_index)
{
  int rand_pick;    // ebp-0x14
  int chosen_csvid; // ebp-0x10
  int total_cards;  // ebp-0xc
  int running_pick; // ebp-0x8
  int i;            // ebp-0x4

  if (library_index == -1)
  {
    return -1;
  }

  total_cards = 0;
  for (i = 0; i < 200; ++i)
  {
    total_cards += g_initial_library[library_index][i].numcards;
  }

  if (total_cards == 0)
  {
    return -1;
  }

  rand_pick = internal_rand(total_cards);
  running_pick = rand_pick;

  for (i = 0; i < 200; ++i)
  {
    running_pick -= g_initial_library[library_index][i].numcards;
    if (running_pick < 0)
    {
      chosen_csvid = g_initial_library[library_index][i].csvid;
      if (g_duel_ai_mode_state != 1)
      {
        --g_initial_library[library_index][i].numcards;
      }
      break;
    }
  }

  for (i = 0; i < g_card_count; ++i)
  {
    if ((int)global_cards_data[i].id == chosen_csvid)
    {
      return i;
    }
  }
}

// FUNCTION: MAGIC 0x00464a57
// FUNCTION: SHANDALAR 0x00522508
int internal_rand(int maximum)
{
  return (maximum > 1) ? rand() % maximum : 0;
}

// FUNCTION: MAGIC 0x00464a84
// FUNCTION: SHANDALAR 0x00522535
int network_random(int player, int maximum)
{
  if (player == g_active_player)
  {
    if (maximum > 1)
    {
      g_network_result_packet.result = rand() % maximum;
    }
    else
    {
      g_network_result_packet.result = 0;
    }

    g_network_result_packet.packet_type = 0x18;
    TENTATIVE_send_network_result(player, 0x18);
  }
  else
  {
    TENTATIVE_wait_for_network_result(player, 0x18);
  }

  return g_network_result_packet.result;
}

// FUNCTION: MAGIC 0x004a6968
// FUNCTION: SHANDALAR 0x00558aea
void TENTATIVE_reassess_all_cards(int view_player, int present_after_draw)
{
  int card;   /* ebp-0x0c */
  int flags;  /* ebp-0x08 */
  int player; /* ebp-0x04 */

  flags = 0xffff;

  if ((flags & 1) != 0)
  {
    count_mana();
  }

  if (g_response_selection_in_progress == 0 && g_trigger_condition == -1)
  {
    g_attack_action_available = 0;
  }

  if ((flags & 2) != 0)
  {
    for (player = 0; player < 2; ++player)
    {
      for (card = 0; card < g_active_cards_count[player]; ++card)
      {
        if (PLAYER_CARD_INSTANCE(player, card).internal_card_id != -1 &&
            ((unsigned int)global_cards_data[PLAYER_CARD_INSTANCE(player, card).internal_card_id].type & 2) != 0)
        {
          C_get_abilities(player, card, 0x3c, -1);
        }
      }
    }

    C_count_colors_of_lands_in_play();

    for (player = 0; player < 2; ++player)
    {
      for (card = 0; card < g_active_cards_count[player]; ++card)
      {
        if (PLAYER_CARD_INSTANCE(player, card).internal_card_id != -1)
        {
          if (((unsigned int)global_cards_data[PLAYER_CARD_INSTANCE(player, card).internal_card_id].type & 2) != 0 ||
              (PLAYER_CARD_INSTANCE(player, card).token_status & STATUS_CANNOT_BE_DESTROYED) != 0)
          {
            C_get_abilities(player, card, 0x34, -1);
          }
          if (((unsigned int)global_cards_data[PLAYER_CARD_INSTANCE(player, card).internal_card_id].type & 2) != 0)
          {
            C_get_abilities(player, card, 0x32, -1);
            C_get_abilities(player, card, 0x33, -1);
          }
        }
      }
    }
  }

  if (g_duel_ai_mode_state == 1)
    return;

  g_activation_event_flags = 0;
  for (player = 0; player < 2; ++player)
  {
    for (card = 0; card < g_active_cards_count[player]; ++card)
    {
      if (PLAYER_CARD_INSTANCE(player, card).internal_card_id != -1)
      {
        PLAYER_CARD_INSTANCE(player, card).unknown0x70 = get_available_card_action(player, card);
      }
    }
  }

  if (g_duel_active != 0)
  {
    SendMessageA((HWND)g_duel_window_hwnd, 0x464, flags, 0);
  }
  else
  {
    redraw_shandalar_duel_screen(view_player, present_after_draw);
  }
}
// FUNCTION: MAGIC 0x004b0047
// FUNCTION: SHANDALAR 0x0046d022
int get_available_card_action(int player, int card)
{
  card_instance_t *instance;  /* ebp-0x2c */
  unsigned int unused_28;     /* ebp-0x28 */
  unsigned int state;         /* ebp-0x24 */
  unsigned int type;          /* ebp-0x20 */
  int trigger_result;         /* ebp-0x1c */
  unsigned int unused_18;     /* ebp-0x18 */
  unsigned int upkeep_flags;  /* ebp-0x14 */
  int result;                 /* ebp-0x10 */
  int internal_card_id;       /* ebp-0xc */
  int in_play;                /* ebp-0x8 */
  unsigned int extra_ability; /* ebp-0x4 */

  instance = &PLAYER_CARD_INSTANCE(player, card);
  state = instance->state;
  upkeep_flags = instance->upkeep_flags;
  in_play = is_in_play(player, card);
  internal_card_id = instance->internal_card_id;
  type = (unsigned int)global_cards_data[internal_card_id].type;
  extra_ability = (unsigned int)global_cards_data[internal_card_id].extra_ability;

  g_activation_scan_flags = 1;
  instance->state |= 0x800;

  if (in_play != 0 && g_current_phase == 0x15 && player == g_current_player && (state & STATE_UNKNOWN8000) != 0 && (state & STATE_ATTACKING) == 0 && can_attack(player, card) != 0)
  {
    result = 2;
    goto finish_get_available_card_action;
  }

  if (in_play != 0 && g_trigger_condition != -1)
  {
    if (instance->unknown0x14 == g_trigger_condition)
    {
      if (g_current_turn == player)
      {
        trigger_result = 2;
      }
      else
      {
        trigger_result = 0;
      }
    }
    else
    {
      trigger_result = ClampIntToRange(dispatch_card_trigger_event(player, card, EVENT_TRIGGER, player), 0, 2);
    }

    if (trigger_result != 0)
    {
      g_activation_dispatch_flags |= 1 << ((unsigned char)trigger_result);
      g_duel_state_current_card += 1;
      result = trigger_result;
      goto finish_get_available_card_action;
    }
    else
    {
      result = 0;
      goto finish_get_available_card_action;
    }
  }
  if (in_play == 0 && g_trigger_condition != -1 && global_cards_data[internal_card_id].code_pointer != card_death_ward)
  {
    result = 0;
  }
  else if (in_play != 0 && g_current_phase == 1)
  {
    push_affected_card_stack();
    g_affected_card_controller = player;
    g_affected_card = card;
    g_event_result = 0;
    C_dispatch_event_raw(EVENT_TRIGGER);
    result = g_event_result;
    pop_affected_card_stack();
  }
  else if (player == g_other_player)
  {
    if ((g_current_action_event_code == 4 && (upkeep_flags & 1) != 0) || (g_current_action_event_code == 10 && internal_card_id == g_draw_card_placeholder_internal_card_id))
    {
      g_activation_event_flags |= 3;
      g_activation_dispatch_flags |= 4;
      result = 2;
      goto finish_get_available_card_action;
    }

    instance->state &= ~0x800;
    result = 0;
  }
  else if (g_response_selection_in_progress == 0 && (g_current_phase == 0x15 || g_current_phase == 0x17))
  {
    if (in_play != 0 && (state & STATE_TAPPED) == 0 && ((type & TYPE_CREATURE) != 0 || (state & (STATE_NONCREATURE_CAN_ATTACK | STATE_NONCREATURE_CAN_BLOCK)) != 0))
    {
      if (player == g_current_player && can_attack(player, card) != 0 && (state & STATE_SUMMONSICK_NOATTACK) == 0)
      {
        result = 0x10;
        goto finish_get_available_card_action;
      }
      if (player != g_current_player && g_attacking_creature_count != 0 && (state & STATE_BLOCKING) == 0)
      {
        result = 0x20;
        goto finish_get_available_card_action;
      }
    }

    instance->state &= ~0x800;
    result = 0;
  }
  else
  {
    if (in_play)
    {
      if (g_current_action_event_code == 4 && (upkeep_flags & 1) != 0)
      {
        g_activation_event_flags |= 3;
        g_activation_dispatch_flags |= 4;
        result = 2;
        goto finish_get_available_card_action;
      }

      if (g_current_action_event_code == 4 && (upkeep_flags & 0x10) != 0 && (upkeep_flags & 0x88) == 0 && can_pay_untap_cost(player, card) != 0)
      {
        g_activation_dispatch_flags |= 2;
        result = 8;
        goto finish_get_available_card_action;
      }

      if ((state & STATE_TAPPED) == 0 && (type & TYPE_CREATURE) != 0 && g_response_selection_in_progress == 0 && player == g_current_player && g_current_phase <= 0x1a && can_attack(player, card) != 0 && (((PLAYER_CARD_INSTANCE(player, card).state & STATE_SUMMONSICK_BOTH) == 0) || (global_cards_data[PLAYER_CARD_INSTANCE(player, card).internal_card_id].type & TYPE_CREATURE) == 0))
      {
        g_attack_action_available = 1;
      }

      if ((((extra_ability & 0x1000) != 0 && (state & STATE_TAPPED) == 0 && (((PLAYER_CARD_INSTANCE(player, card).state & STATE_SUMMONSICK_BOTH) == 0) || (global_cards_data[PLAYER_CARD_INSTANCE(player, card).internal_card_id].type & TYPE_CREATURE) == 0)) || ((extra_ability & 1) != 0 && (g_response_card_type_mask & 0x10) != 0) || ((extra_ability & 2) != 0 && (g_response_card_type_mask & 0x20) != 0)) && ((g_land_can_be_played & LCBP_DAMAGE_PREVENTION) == 0 || (extra_ability & 0x5004) != 0))
      {
        g_activation_event_flags &= ~2;
        if ((state & STATE_INVISIBLE) == 0 && dispatch_event_to_single_card(player, card, EVENT_CAN_ACTIVATE, 1 - player, -1) != 0)
        {
          if ((g_activation_event_flags & 2) != 0)
          {
            g_activation_dispatch_flags |= 4;
            result = 2;
            goto finish_get_available_card_action;
          }

          g_activation_dispatch_flags |= 2;
          result = 8;
          goto finish_get_available_card_action;
        }
      }
    }
    else
    {
      if ((state & (STATE_SUMMONSICK | STATE_INVISIBLE)) != 0)
      {
        instance->state &= ~0x800;
        result = 0;
        goto finish_get_available_card_action;
      }

      unused_28 = single_color_test_bit_to_color_t((int)(char)global_cards_data[internal_card_id].color);
      if (g_response_selection_in_progress == 0 || (g_response_card_type_mask & type) != 0)
      {
        if ((type & TYPE_LAND) != 0)
        {
          if (player == g_current_player && (g_land_can_be_played & LCBP_LAND_HAS_BEEN_PLAYED) == 0 && (g_current_phase == 0x14 || g_current_phase == 0x1e))
          {
            result = 4;
            goto finish_get_available_card_action;
          }
          else
          {
            instance->state &= ~0x800;
            result = 0;
            goto finish_get_available_card_action;
          }
        }

        if ((((g_active_player == g_current_player && (((g_response_selection_in_progress != 0 && (type & 0x30) != 0) || g_current_phase == 0x14) || g_current_phase == 0x1e)) || (g_active_player != g_current_player && g_response_selection_in_progress != 0 && ((type & 0x10) != 0 || (type & 0x20) != 0))) && (unused_18 = can_pay_card_mana_cost(player, player, card)) != 0 && (((g_land_can_be_played & LCBP_DAMAGE_PREVENTION) == 0 || (extra_ability & 0x3004) != 0) && ((type & 0x42) != 0 || dispatch_event_to_single_card(player, card, EVENT_CAN_CAST, 1 - player, -1) != 0))))
        {
          result = 4;
          goto finish_get_available_card_action;
        }
      }
    }

    instance->state &= ~0x800;
    result = 0;
  }

finish_get_available_card_action:
  g_activation_scan_flags = 0;
  return result;
}

// FUNCTION: MAGIC 0x004b5274
// FUNCTION: SHANDALAR 0x0056ba7d
int calculate_attack_rating(int player, int card)
{
  int power2;
  int toughness_masked;
  keyword_t static_ability;
  int score;
  int internal_card_id;

  internal_card_id = PLAYER_CARD_INSTANCE(player, card).internal_card_id;
  static_ability = global_cards_data[internal_card_id].static_ability;

  power2 = (((int)global_cards_data[internal_card_id].power & 0xffffbfffU) * 2);
  if (global_cards_data[internal_card_id].subtype == 0)
  {
    power2 = 0;
  }

  toughness_masked = ((int)global_cards_data[internal_card_id].toughness & 0xffffbfffU);
  score = (int)((power2 + 2) * (toughness_masked + 1)) / 2;

  if (((PLAYER_CARD_INSTANCE(player, card).state & STATE_TAPPED) != 0) && (g_current_player == player))
  {
    score -= 1;
  }

  if ((static_ability & 0x80) != 0)
  {
    score = (score * 3) / 2;
  }
  if ((static_ability & 0x100) != 0)
  {
    score = (score * 3) / 2;
  }
  if ((global_cards_data[internal_card_id].extra_ability & 3) != 0)
  {
    score = (score * 3) / 2;
  }
  if ((static_ability & 0x40) != 0)
  {
    score = (int)((toughness_masked + 1) * score) / 2;
  }
  if ((static_ability & 0x200) != 0)
  {
    score = (score * 3) / 2;
  }

  score = g_ai_combat_value_weights[player + 2] * score;
  score = (score + ((score >> 0x1f) & 7)) >> 3;
  return score;
}

// FUNCTION: MAGIC 0x004e4ff3
// FUNCTION: SHANDALAR 0x004c4263
int load_recorded_action_target(int a1)
{
  if (g_duel_ai_mode_state != 1)
  {
    g_ai_recorded_action = g_recorded_action_targets[a1 + g_recorded_action_count];
    if (g_ai_recorded_action != -1)
    {
      g_ai_recorded_action &= 0xfff;
    }
  }
  return 0;
}

// FUNCTION: MOK 0x00499010
// FUNCTION: MAGIC 0x004e503e
// FUNCTION: SHANDALAR 0x004c42ae
int load_recorded_action_code(int a1)
{
  if (g_duel_ai_mode_state != 1)
  {
    g_loaded_recorded_action_code = g_recorded_action_codes[a1 + g_recorded_action_count];
    if (g_loaded_recorded_action_code == 99)
    {
      g_loaded_recorded_action_code = 0;
    }
  }
  return 0;
}

// FUNCTION: MAGIC 0x004e51a6
// FUNCTION: SHANDALAR 0x004c4415
int get_recorded_action_count(void)
{
  return g_recorded_action_count;
}

// FUNCTION: MAGIC 0x004e51bb
// FUNCTION: SHANDALAR 0x004c442a
void rewind_recorded_action(void)
{
  if (g_recorded_action_count > 0)
  {
    --g_recorded_action_count;
  }
  else
  {
    g_recorded_action_count = 0;
  }
}
