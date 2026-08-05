#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include "cardartlib/src/assert.h"
#include "cardartlib/src/palette.h"
#include "deckdll/src/magsnd.h"
#include "deckdll/src/shared_resources.h"
#include "drawcardlib/Drawcardlib.h"
#include "game_support.h"
#include "magic/src/duel_runtime.h"
#include "global_strings.h"

typedef ptrdiff_t INT_PTR;

typedef int(__cdecl *in_play_card_callback_t)(int source_player,
                                              int source_card,
                                              int player,
                                              int card,
                                              int internal_card_id);

extern card_ptr_t global_raw_cards_storage[2000];
extern int g_ai_search_flags;
extern const char *g_duel_sound_filenames[];
extern char global_duelsounds_path[0x110];

int GetNextManaSymbol(char **text);
int raw_do_dialog(int bigcard_player, int bigcard_card, int smallcard_player, int smallcard_card,
                  char *prompt, int dialog_mode);
int ReadPalette(char *palette_text_path, char *palette_binary_path);
int legacy_clear_graphics_page_stub(int page, int color);
int legacy_load_pcx_into_page_no_palette_stub(const char *filename);
int legacy_load_pcx_into_page_stub(int page, int x, int y, const char *filename, void *palette);
int legacy_blit_graphics_rect_stub(int dst, int dst_x, int dst_y, int width, int height,
                                   int src, int src_x, int src_y, int src_width, int src_height);
void append_to_trace_txt(char *text);
int choose_a_color_dialog(int player, const char *prompt, int use_color_names_instead_of_land, int ai_choice, unsigned int available_colors);

int begin_mana_payment_record(void);
int end_mana_payment_record(void);
int restore_recorded_mana_to_pool(int player);
int refresh_stack_proxy_card(int prompt_flag);
int set_stack_damage_targets(void);
int count_colored_cards_in_hand(int player);

int process_response_actions(int reason_for_trigger_controller, const char *prompt);
void ReadCsvFieldByCsvid(char *out, int csvid, int field, const char *csv_name);
void show_card_image_dialog(unsigned int internal_card_id, int unk1, char *prompt, int unk2);
int update_duel_selection_display(int player, int phase);
void display_duel_prompt_text(char *text);
int GetCardRarity(int internal_card_id);
#ifdef SHANDALAR
int GetUiTickCount(void);
#endif
int process_killed_card(int player, int card);
char *get_displayed_card_name(int player, int card);
int dispatch_trigger_twice_once_with_each_player_as_reason(int reason_for_trig, trigger_t trig, const char *prompt, int a4);
int allow_response(int response_player, int phase, char *prompt, int event_code);
int prompt_for_life_total(int player, char *prompt, int initial_life_total);

void position_duel_prompt_context_window(HWND prompt_hwnd);
void setup_duel_prompt_context_text_dc(HWND prompt_hwnd, HDC dc, int *rect_values);
void restack_duel_child_windows(void);
void AddCardToCLPacket(int card_in_packet);
int GetCardFromCLPacket(int packet_index);
void initialize_card_instance(int player, int internal_card_id, int card);
void show_duel_system_error(const char *err_msg);
void get_landwalk_evasion_masks(unsigned int *out_landwalk_mask, unsigned int *out_basic_land_mask);
int can_block_attacker_with_abilities(int blocker_player,
                                      int blocker_card,
                                      int attacker_player,
                                      int attacker_card,
                                      unsigned int attacker_abilities,
                                      unsigned int evasion_mask);

// GLOBAL: MAGIC 0x00573080
// GLOBAL: SHANDALAR 0x00593e20
const unsigned char wizard_card_color_preference_table[18] = {
    0x00,
    0x00,
    0x00,
    0x01,
    0x04,
    0x02,
    0x02,
    0x01,
    0x05,
    0x03,
    0x05,
    0x04,
    0x04,
    0x03,
    0x01,
    0x05,
    0x02,
    0x05,
};

int can_target(target_definition_t *td)
{
  return td != NULL;
}

// FUNCTION: MAGIC 0x0043edab
// FUNCTION: SHANDALAR 0x0040a62b
int put_card_on_stack(int player, int card, int mode)
{
  int trace_counter;                          // ebp-0x348
  char prompt_text[300];                      // ebp-0x344
  int tmp_mana;                               // ebp-0x218
  char trace_text[500];                       // ebp-0x214
  int saved_unk_008ce508;                     // ebp-0x20
  int saved_max_x_value;                      // ebp-0x1c
  int chosen_x_value;                         // ebp-0x18
  int color_index;                            // ebp-0x14
  int saved_current_casting_internal_card_id; // ebp-0x10
  int internal_card_id;                       // ebp-0xc
  card_data_t *card_data;                     // ebp-0x8
  int saved_unk_008ce4f4;                     // ebp-0x4

  internal_card_id = PLAYER_CARD_INSTANCE(player, card).internal_card_id;
  color_index = (int)single_color_test_bit_to_color_t((int)(char)global_cards_data[internal_card_id].color);
  card_data = &global_cards_data[internal_card_id];

  if (mode != 0)
  {
    goto respond_to_card_on_stack;
  }

  if (TRACE_ENABLED)
  {
    trace_counter = g_duel_trace_counter;
    ++g_duel_trace_counter;
    sprintf(trace_text, "%d: Player #%d is casting %s(%d).\n", trace_counter, player, global_cards_data[PLAYER_CARD_INSTANCE(player, card).internal_card_id].name, card);
    append_to_trace_txt(trace_text);
  }

  if (!can_pay_card_mana_cost(player, player, card))
  {
    return 0;
  }

  g_ai_recorded_action = -1;
  g_adventure_world_state = 0;
  g_max_x_value = -1;

  if ((((unsigned char)card_data->type & 0x3c) != 0 ||
       (int)card_data->id == 0x107 ||
       (int)card_data->id == 0x27) &&
      dispatch_event_to_single_card(player, card, 0x74, 1 - player, -1) == 0)
  {
    return 0;
  }

  g_spell_fizzled = -1;

  saved_current_casting_internal_card_id = g_current_casting_internal_card_id;
  saved_unk_008ce508 = g_current_spell_player;
  saved_unk_008ce4f4 = g_current_spell_card;

  g_current_casting_internal_card_id = internal_card_id;
  g_current_spell_player = player;
  g_current_spell_card = card;

  push_card_onto_stack(player, card, 0x71, player, 0);
  begin_mana_payment_record();

  if (g_adventure_world_state != 0)
  {
  }
  else
  {
    if ((player == g_active_player || (g_duel_network_flags & 2) != 0) && g_duel_ai_mode_state != 1)
    {
      if ((card_data->type & 0x40) != 0)
      {
        g_mana_charge[6] = (int)(char)card_data->cc[0];
        g_mana_charge[6] += (int)(char)card_data->cc[1];
      }
      else
      {
        g_mana_charge[color_index] = (int)(char)card_data->cc[0];
        g_mana_charge[0] += (int)(char)card_data->cc[1];
      }

      charge_mana_w_global_cost_mod(player, card, 0, 0);
    }
    else if ((card_data->type & 0x40) != 0)
    {
      charge_mana_w_global_cost_mod(player, card, 6, (int)(char)card_data->cc[1] + (int)(char)card_data->cc[0]);
    }
    else
    {
      if ((char)card_data->cc[0] != 0)
      {
        g_mana_charge[color_index] = (int)(char)card_data->cc[0];
      }
      if (0 < (char)card_data->cc[1])
      {
        g_mana_charge[0] = (int)(char)card_data->cc[1];
      }

      charge_mana_w_global_cost_mod(player, card, 0, 0);

      if ((char)card_data->cc[1] == -1)
      {
        if (player != g_active_player)
        {
          if (g_duel_ai_mode_state == 1)
          {
            switch (internal_rand(2))
            {
            case 0:
              tmp_mana = has_mana(player, COLOR_ANY, 1);
              if (tmp_mana != 0)
              {
                g_ai_recorded_choice = chosen_x_value = internal_rand(tmp_mana) + 1;
              }
              else
              {
                g_ai_recorded_choice = chosen_x_value = internal_rand(has_mana(player, COLOR_ANY, 1) + 1);
              }
              break;

            case 1:
              g_ai_recorded_choice = chosen_x_value = has_mana(player, COLOR_ANY, 1);
              if (g_life[0] < g_ai_recorded_choice && internal_rand(3) == 0)
              {
                g_ai_recorded_choice = chosen_x_value = g_life[0];
              }
              if (g_max_x_value != -1 && g_max_x_value < g_ai_recorded_choice)
              {
                g_ai_recorded_choice = chosen_x_value = g_max_x_value;
              }
              break;
            }

            record_ai_action_selection();
          }
          else
          {
            replay_ai_action_selection();
            chosen_x_value = g_ai_recorded_choice;
          }

          saved_max_x_value = g_max_x_value;
          g_max_x_value = chosen_x_value;
          charge_mana(player, 0, -1);
          g_max_x_value = saved_max_x_value;
        }
        else
        {
          saved_max_x_value = g_max_x_value;
          g_max_x_value = has_mana(player, COLOR_ANY, 1);
          charge_mana(player, 0, -1);
          g_max_x_value = saved_max_x_value;
        }

        if (g_x_value == 0)
        {
          g_ai_modifier -= 100;
        }
      }
    }
  }

  g_max_x_value = -1;

  if (internal_card_id != -1)
  {
    g_current_casting_player = player;
    g_duel_state_008b2880 = 1;
    --g_duel_summary.hand_counts[player];

    if ((card_data->type & 2) != 0)
    {
      ++g_duel_summary.creature_counts[player];
    }
    if ((card_data->type & 0x40) != 0)
    {
      ++g_duel_summary.artifact_counts[player];
    }
    if ((card_data->type & 4) != 0)
    {
      ++g_duel_summary.enchantment_counts[player];
    }

    g_card_types_in_play[player] |= card_data->type;
    PLAYER_CARD_INSTANCE(player, card).state |= STATE_INVISIBLE;
    g_land_can_be_played |= 0x20;

    if (player == g_active_player || (g_duel_network_flags & 2) != 0 || g_duel_ai_mode_state != 1)
    {
      PLAYER_CARD_INSTANCE(player, card).state |= STATE_SUMMONSICK_BOTH;
    }
    else
    {
      PLAYER_CARD_INSTANCE(player, card).state |= STATE_SUMMONSICK_NOATTACK;
    }

    g_current_casting_internal_card_id = saved_current_casting_internal_card_id;
    g_current_spell_player = saved_unk_008ce508;
    g_current_spell_card = saved_unk_008ce4f4;

    if (g_spell_fizzled == 1)
    {
      end_mana_payment_record();
      goto finish_put_card_on_stack;
    }

    if (dispatch_event(player, card, 0x6c) != 0)
    {
      g_spell_fizzled = 1;
    }
    if (g_spell_fizzled == 1)
    {
      restore_recorded_mana_to_pool(player);
    }
    end_mana_payment_record();

    if (g_spell_fizzled == 1)
    {
      goto finish_put_card_on_stack;
    }

    if (player == g_other_player && (g_duel_network_flags & 2) == 0 && g_duel_ai_mode_state == 1 && card_data->cc[1] == -1)
    {
      g_ai_modifier -= count_colored_cards_in_hand(player) * 12;
    }
    return 1;

  respond_to_card_on_stack:
    saved_current_casting_internal_card_id = g_current_casting_internal_card_id;
    saved_unk_008ce508 = g_current_spell_player;
    saved_unk_008ce4f4 = g_current_spell_card;

    g_current_casting_internal_card_id = internal_card_id;
    g_current_spell_player = player;
    g_current_spell_card = card;

    if ((card_data->type & 1) == 0)
    {
      refresh_stack_proxy_card(1);
    }

    if ((card_data->type != 1) &&
        ((card_data->type != 0x20) || ((card_data->extra_ability & EA_MANA_SOURCE) == 0)))
    {
      if (g_duel_state_007a7d78 != 0)
      {
        g_duel_state_007a7d78 |= 2;
      }

      sprintf(g_ui_message_buffer, (const char *)gs_trying_to_cast_007a79c0, get_displayed_card_name(g_current_spell_player, g_current_spell_card));
      allow_response(-2, g_current_phase, g_ui_message_buffer, 0xd3);
      if ((g_duel_state_007a7d78 & 2) != 0)
      {
        request_duel_display_refresh_if_human(player, card, 2, 1);
        Sleep(3000);
      }
    }

    g_current_casting_internal_card_id = saved_current_casting_internal_card_id;
    g_current_spell_player = saved_unk_008ce508;
    g_current_spell_card = saved_unk_008ce4f4;

    if (PLAYER_CARD_INSTANCE(player, card).internal_card_id == -1)
    {
      g_spell_fizzled = 1;
    }

    g_land_can_be_played &= ~0x20;
    *(unsigned int *)&PLAYER_CARD_INSTANCE(player, card).state |= (((unsigned int)player < 1 ? 0 : 0x400000) | 0x80);

    if (g_duel_ai_mode_state != 1)
    {
      set_stack_damage_targets();
    }

    if (g_spell_fizzled == 1)
    {
      goto finish_put_card_on_stack;
    }

    if (player == g_other_player && g_duel_ai_mode_state != 1 && g_battlefield_ui_flags == 0 && (((unsigned char)card_data->type & 0x7e) != 0))
    {
      LoadTextSectionLines(global_ui_strings_filename, "PROMPT_CAST1");
      sprintf(prompt_text, g_text_lines[0], g_duel_message_text);
      if ((char)card_data->cc[1] == -1)
      {
        sprintf(prompt_text, g_text_lines[1], g_duel_message_text, g_x_value);
      }

      if (PLAYER_CARD_INSTANCE(player, card).number_of_targets != 0)
      {
        if (PLAYER_CARD_INSTANCE(player, card).number_of_targets == 1)
        {
          raw_do_dialog(player,
                        card,
                        PLAYER_CARD_INSTANCE(player, card).targets[0].player,
                        PLAYER_CARD_INSTANCE(player, card).targets[0].card,
                        prompt_text,
                        0);
        }
        else
        {
          raw_do_dialog(player, card, -1, -1, prompt_text, 0);
        }
      }
      else if (0)
      {
        // Unreachable in practice, but present in the original binary.
        raw_do_dialog(player, card, (g_ai_recorded_action >> 8), (g_ai_recorded_action & 0xff), prompt_text, 0);
      }
      else
      {
        raw_do_dialog(player, card, -1, -1, prompt_text, 0);
      }
    }

    if (g_duel_ai_mode_state != 1)
    {
      request_duel_display_refresh_if_human(player, card, 2, 1);
    }
  }
finish_put_card_on_stack:
  if (g_spell_fizzled == 1)
  {
    ++g_duel_summary.hand_counts[player];

    if ((card_data->type & 2) != 0)
    {
      --g_duel_summary.creature_counts[player];
    }
    if ((card_data->type & 0x40) != 0)
    {
      --g_duel_summary.artifact_counts[player];
    }
    if ((card_data->type & 4) != 0)
    {
      --g_duel_summary.enchantment_counts[player];
    }

    *(unsigned int *)&PLAYER_CARD_INSTANCE(player, card).state &= 0xffffff5d;
    *(unsigned int *)&PLAYER_CARD_INSTANCE(player, card).state &= 0xfffcffff;

    if (player == g_other_player && (g_duel_network_flags & 2) == 0)
    {
      g_recorded_action_cursor = 1;
    }
    g_spell_fizzled = 0;
    obliterate_top_card_of_stack();
    g_land_can_be_played &= ~0x20;
    return 0;
  }

  TENTATIVE_set_timestamps(player, card);
  return 1;
}

// FUNCTION: MAGIC 0x0043f9ed
// FUNCTION: SHANDALAR 0x0040b26d
int resolve_card_on_stack(int player, int card)
{
  int internal_card_id;
  char *card_name;
  int saved_trigger_cause;
  int saved_trigger_cause_controller;

  internal_card_id = PLAYER_CARD_INSTANCE(player, card).internal_card_id;
  single_color_test_bit_to_color_t((int)(char)global_cards_data[internal_card_id].color);

  if (internal_card_id == -1)
  {
    obliterate_top_card_of_stack();
    return 0;
  }

  saved_trigger_cause_controller = g_trigger_cause_controller;
  saved_trigger_cause = g_trigger_cause;

  g_trigger_cause_controller = player;
  g_trigger_cause = card;
  dispatch_trigger_twice_once_with_each_player_as_reason(g_current_player, TRIGGER_SPELL_CAST, gs_casting_00896400, 0);
  g_trigger_cause_controller = saved_trigger_cause_controller;
  g_trigger_cause = saved_trigger_cause;

  if (global_cards_data[internal_card_id].type != 0x01 &&
      (global_cards_data[internal_card_id].type != 0x20 || (global_cards_data[internal_card_id].extra_ability & 0x1000) == 0))
  {
    sprintf(g_ui_message_buffer, gs_cast_008b4720, get_displayed_card_name(player, card));
    allow_response(-2, g_current_phase, g_ui_message_buffer, 0x6c);
  }

  PLAYER_CARD_INSTANCE(player, card).state &= ~0x20;
  PLAYER_CARD_INSTANCE(player, card).state |= 2;
  g_land_can_be_played &= ~0x20;

  if (PLAYER_CARD_INSTANCE(player, card).internal_card_id != internal_card_id)
  {
    obliterate_top_card_of_stack();
    return 0;
  }

  g_card_types_in_play[player] |= (unsigned char)global_cards_data[internal_card_id].type;

  if (g_duel_ai_mode_state != 1)
  {
    if ((global_cards_data[internal_card_id].type & 2) != 0)
    {
      play_sound_effect(0x11);
    }
    if ((global_cards_data[internal_card_id].type & 0x40) != 0)
    {
      play_sound_effect(0);
    }
    if ((global_cards_data[internal_card_id].type & 4) != 0)
    {
      play_sound_effect(3);
    }
    if ((global_cards_data[internal_card_id].type & 0x10) != 0)
    {
      play_sound_effect(6);
    }
    if ((global_cards_data[internal_card_id].type & 0x20) != 0)
    {
      play_sound_effect(7);
    }
    if ((global_cards_data[internal_card_id].type & 8) != 0)
    {
      play_sound_effect(0x10);
    }
  }

  resolve_top_card_on_stack();
  process_damage_prevention(player);
  g_battlefield_extra_ability_flags &= ~1;
  pending_killed_card_handler = process_killed_card;
  regenerate_or_graveyard_triggers();
  process_damage_prevention(player);

  g_current_casting_player = -1;
  g_current_casting_internal_card_id = g_current_casting_player;

  if ((global_cards_data[internal_card_id].type & 1) != 0)
  {
    ++g_duel_summary.land_entries;
  }

  if (g_spell_fizzled == 1)
  {
    if (g_duel_ai_mode_state != 1)
    {
      load_text("prompts.txt", "PROMPT_FIZZLE");
      set_duel_prompt_text(g_ui_message_buffer);
      Sleep(2000);
      set_duel_prompt_text("");
    }
    g_recorded_action_cursor = 1;
    g_spell_fizzled = 0;
    return 0;
  }

  g_spell_fizzled = 0;
  TENTATIVE_reassess_all_cards(0, 0xff);
  return 1;
}

// FUNCTION: MAGIC 0x004456ad
// FUNCTION: SHANDALAR 0x00410f46
int dispatch_trigger(int player, trigger_t trig, const char *prompt, int TENTATIVE_allow_response)
{
  int saved_trig;          // ebp-0x24
  int saved_DAT_00789714;  // ebp-0x20
  int saved_DAT_007aadec;  // ebp-0x1c
  int result;              // ebp-0x18
  int loop_card;           // ebp-0x14
  int saved_DAT_007ab2bc;  // ebp-0x10
  unsigned int saved_ui;   // ebp-0xc
  int saved_current_turn;  // ebp-0x8
  int saved_current_phase; // ebp-0x4

  ++g_nested_trigger_depth;

  saved_current_phase = g_current_phase;
  saved_DAT_007ab2bc = g_activation_dispatch_flags;
  saved_current_turn = g_current_turn;
  saved_DAT_007aadec = g_current_action_event_code;

  g_current_action_event_code = trig;
  g_current_turn = player;

  saved_DAT_00789714 = g_response_processing_flags;
  saved_trig = trig;

  do
  {
    if ((g_duel_network_flags & 2) != 0)
    {
      g_response_processing_flags = 1;
    }
    else if (player != 0)
    {
      g_response_processing_flags = 2;
    }
    else
    {
      g_response_processing_flags = 1;
    }

    g_trigger_condition = (trigger_t)saved_trig;

    saved_ui = g_response_action_mask;
    if (TENTATIVE_allow_response != 0)
    {
      g_response_action_mask = 0x30;
    }
    else
    {
      g_response_action_mask = 0;
    }

    g_spell_fizzled = 0;
    g_duel_state_current_card = g_spell_fizzled;
    g_activation_dispatch_flags = g_duel_state_current_card;

    result = process_response_actions(player, prompt);

    g_response_action_mask = saved_ui;
    g_response_card_type_mask = g_response_action_mask & 0x30;
    g_response_processing_flags = saved_DAT_00789714;

    {
      int mask;
      mask = (result == 1);
      mask -= 1;
      mask &= ~1;
      mask += 6;

      if ((g_activation_dispatch_flags & mask) != 0)
      {
        continue;
      }
    }

    if (TENTATIVE_allow_response != 0 && result != 0)
    {
      continue;
    }

    break;

  } while (1);

  g_trigger_condition = (trigger_t)-1;
  --g_nested_trigger_depth;

  if (g_nested_trigger_depth == 0)
  {
    for (player = 0; player < 2; ++player)
    {
      for (loop_card = 0; loop_card < g_active_cards_count[player]; ++loop_card)
      {
        PLAYER_CARD_INSTANCE(player, loop_card).state &= ~0x100;
      }
    }

    if (g_trigger_dispatch_depth == 0)
    {
      g_response_window_serial = 0;
      if (g_duel_ai_mode_state != 1)
      {
        g_phase_stop_suppressed = 0;
      }
    }
  }

  g_activation_dispatch_flags = saved_DAT_007ab2bc;
  g_current_turn = saved_current_turn;
  g_current_action_event_code = saved_DAT_007aadec;
  g_current_phase = saved_current_phase;

  return 0;
}

// FUNCTION: MAGIC 0x00446da7
// FUNCTION: SHANDALAR 0x00450925
void show_duel_card_preview(unsigned int internal_card_id, int player, int card, int unk1, char *prompt, int unk2)
{
  (void)unk1;
  (void)prompt;
  (void)unk2;

  // The original stores these params to locals even if it won't use them.
  {
    int preview_player;
    int preview_card;

    preview_player = player;
    preview_card = card;

    if (IsWindowVisible((HWND)g_duel_card_preview_window_hwnd))
    {
      if (player != -1 && card != -1)
      {
        SendMessageA((HWND)g_duel_card_preview_window_hwnd, 0x401, CardIDFromType(internal_card_id), (LPARAM)&preview_player);
      }
      else
      {
        SendMessageA((HWND)g_duel_card_preview_window_hwnd, 0x401, CardIDFromType(internal_card_id), 0);
      }
    }
  }
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

// FUNCTION: MAGIC 0x004a616d
// FUNCTION: SHANDALAR 0x005582ed
int update_duel_selection_display_if_human(int player, int phase)
{
  if (g_duel_ai_mode_state == 1)
  {
    return 0;
  }
  return update_duel_selection_display(player, phase);
}

// FUNCTION: MAGIC 0x004a6355
// FUNCTION: SHANDALAR 0x005584d5
void show_card_preview_if_human(unsigned int internal_card_id, int unk1, char *prompt, int unk2)
{
  if (g_duel_ai_mode_state == 1)
  {
    return;
  }

  if (g_duel_active != 0)
  {
    show_duel_card_preview(internal_card_id, -1, -1, unk1, prompt, unk2);
  }
  else
  {
    show_card_image_dialog(internal_card_id, unk1, prompt, unk2);
  }
}

// FUNCTION: MAGIC 0x004b58e2
// FUNCTION: SHANDALAR 0x0056c0e5
int is_card_color_compatible_with_mask(int card_color, int color_mask, int compatibility_level)
{
  if (card_color == 1 || color_mask == 1)
  {
    return 1;
  }

  card_color = (int)single_color_test_bit_to_color_t(card_color);
  color_mask = (int)single_color_test_bit_to_color_t(color_mask);

  if ((char)wizard_card_color_preference_table[color_mask * 3] == card_color)
  {
    return 1;
  }

  if (compatibility_level > 1 && (char)wizard_card_color_preference_table[color_mask * 3 + 1] == card_color)
  {
    return 1;
  }

  if (compatibility_level > 2 && (char)wizard_card_color_preference_table[color_mask * 3 + 2] == card_color)
  {
    return 1;
  }

  if (compatibility_level > 3)
  {
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

// FUNCTION: MAGIC 0x00449bef
// FUNCTION: SHANDALAR 0x00453770
void copy_opponent_name_prefix(char *name)
{
  char *source;

  if (name == 0)
  {
    return;
  }

  source = g_saved_player_name;
  while (*source != '\0' && *source != '-')
  {
    *name = *source;
    ++source;
    ++name;
  }
  *name = '\0';
}

void default_target_definition(int player, int card, target_definition_t *td, int type)
{
  memset(td, 0, sizeof(*td));
  td->who_chooses = player;
  td->allowed_controller = ANYBODY;
  td->preferred_controller = 1 - player;
  td->zone = TARGET_ZONE_IN_PLAY;
  td->required_type = type;
  td->player = player;
  td->card = card;
}

// FUNCTION: MAGIC 0x004464f0
// FUNCTION: SHANDALAR 0x00411d90
int is_in_play(int player, int card)
{
  if (global_card_instances[player][card].internal_card_id == -1)
    return 0;

  return (global_card_instances[player][card].state & (STATE_OUBLIETTED | STATE_INVISIBLE | STATE_IN_PLAY)) == STATE_IN_PLAY;
}

// FUNCTION: MAGIC 0x005001e0
// FUNCTION: SHANDALAR 0x004ba309
int get_hacked_color(int player, int card, int orig_color)
{
  if ((char)global_card_instances[player][card].hack_mode[orig_color] == 0)
  {
    return orig_color;
  }
  else
  {
    return (char)global_card_instances[player][card].hack_mode[orig_color];
  }
}

// FUNCTION: MAGIC 0x0050026d
// FUNCTION: SHANDALAR 0x004ba392
int get_sleighted_color(int player, int card, int orig_color)
{
  if ((char)global_card_instances[player][card].color_id[orig_color] == 0)
  {
    return orig_color;
  }
  else
  {
    return (char)global_card_instances[player][card].color_id[orig_color];
  }
}

// FUNCTION: MOK 0x004358f0
// FUNCTION: MAGIC 0x00442763
// FUNCTION: SHANDALAR 0x0040dffd
color_t single_color_test_bit_to_color_t(color_test_t color_test)
{
  if ((color_test & COLOR_TEST_BLACK) != 0)
  {
    return COLOR_BLACK;
  }
  if ((color_test & COLOR_TEST_BLUE) != 0)
  {
    return COLOR_BLUE;
  }
  if ((color_test & COLOR_TEST_GREEN) != 0)
  {
    return COLOR_GREEN;
  }
  if ((color_test & COLOR_TEST_RED) != 0)
  {
    return COLOR_RED;
  }
  if ((color_test & COLOR_TEST_WHITE) != 0)
  {
    return COLOR_WHITE;
  }

  return COLOR_COLORLESS;
}

// FUNCTION: MAGIC 0x004832f4
// FUNCTION: SHANDALAR 0x00486e05
int graveyard_has_type(int player, int type)
{
  int result;
  int graveyard_index;
  int done;

  result = 0;
  if (player == -1 || player == g_active_player)
  {
    for (graveyard_index = 0, done = 0; graveyard_index < 500 && done == 0; ++graveyard_index)
    {
      if (global_graveyard_slots[g_active_player][graveyard_index] != -1)
      {
        if ((type & global_cards_data[global_graveyard_slots[g_active_player][graveyard_index]].type) != 0)
        {
          result = 1;
          done = result;
        }
      }
      else
      {
        done = 1;
      }
    }
  }

  if ((player == -1 || player == g_other_player) && result == 0)
  {
    for (graveyard_index = 0, done = 0; graveyard_index < 500 && done == 0; ++graveyard_index)
    {
      if (global_graveyard_slots[g_other_player][graveyard_index] != -1)
      {
        if ((type & global_cards_data[global_graveyard_slots[g_other_player][graveyard_index]].type) != 0)
        {
          result = 1;
          done = result;
        }
      }
      else
      {
        done = 1;
      }
    }
  }

  return result;
}

// FUNCTION: MAGIC 0x00484581
// FUNCTION: SHANDALAR 0x0048808b
int is_basic_land_internal_card_id_of_color(int internal_card_id, color_t color)
{
  int result;

  result = 0;
  if ((char)global_cards_data[internal_card_id].subtype == 13 && global_cards_data[internal_card_id].id == g_duel_special_land_card_ids[color])
  {
    result = 1;
  }
  if ((char)global_cards_data[internal_card_id].subtype == 12 && ((1 << color) & (int)*(char *)&PLAYER_CARD_INSTANCE(g_affected_card_controller, g_affected_card).mana_color) != 0)
  {
    result = 1;
  }

  return result;
}

// FUNCTION: MAGIC 0x004a62d7
// FUNCTION: SHANDALAR 0x00558457
int select_from_graveyard_with_dialog(int player,
                                      int *graveyard,
                                      void *available,
                                      int count,
                                      int prompt,
                                      int a6,
                                      int selected,
                                      int a8,
                                      int a9)
{
  if (g_duel_ai_mode_state == 1)
  {
    return 1;
  }
  return select_multiple_cards_from_card_list(player, graveyard, 0, available, count, prompt, a6, selected, a8, a9);
}

int get_sleighted_color_test(int player, int card, int orig_color_test)
{
  (void)player;
  (void)card;
  return orig_color_test;
}

// FUNCTION: MAGIC 0x004a6245
// FUNCTION: SHANDALAR 0x005583c5
INT_PTR show_damage_assignment_cardlist(int *internal_card_ids, int *damage_amounts, int count, int title, int allow_cancel, char *prompt)
{
  if (g_duel_ai_mode_state == 1)
  {
    return 1;
  }

  return show_cardlist(internal_card_ids, damage_amounts, 0, count, (void *)title, (unsigned int)allow_cancel, prompt);
}

// FUNCTION: MAGIC 0x004a628e
// FUNCTION: SHANDALAR 0x0055840e
INT_PTR show_selectable_cardlist(int *graveyard,
                                 int *available,
                                 int count,
                                 void *window_title,
                                 unsigned int require_selection,
                                 char *prompt)
{
  if (g_duel_ai_mode_state == 1)
  {
    return 1;
  }

  return show_cardlist(graveyard, 0, available, count, window_title, require_selection, prompt);
}

// FUNCTION: MAGIC 0x004b5f03
// FUNCTION: SHANDALAR 0x0056c705
int find_internal_card_id_by_csv_id(card_id_t card_id)
{
  int result;
  int i;

  if (card_id == -1)
  {
    return -1;
  }

  result = -1;
  for (i = 0; i < g_card_count + 0x10; ++i)
  {
    if (global_cards_data[i].id == card_id)
    {
      return i;
    }
  }

  return result;
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

// FUNCTION: MAGIC 0x00481e25
// FUNCTION: SHANDALAR 0x0048592f
int card_two_headed_giant_of_foriys_legacy(int player, int card, event_t event)
{
  if (event == 0x78 && card == g_affected_card && player == g_affected_card_controller)
  {
    if ((int)(char)PLAYER_CARD_INSTANCE((int)(char)PLAYER_CARD_INSTANCE(player, card).damage_source_player,
                                        PLAYER_CARD_INSTANCE(player, card).damage_source_card)
                .blocking == g_attacking_card ||
        (int)(char)PLAYER_CARD_INSTANCE((int)(char)PLAYER_CARD_INSTANCE(player, card).damage_source_player,
                                        PLAYER_CARD_INSTANCE(player, card).damage_source_card)
                .blocking ==
            (int)(char)PLAYER_CARD_INSTANCE(g_attacking_card_controller, g_attacking_card).blocking)
    {
      g_event_result = 1;
    }
  }

  dispatch_function_to_all_cards_in_play(player, card, reattach_if_attached_to_source, -1);

  if (g_trigger_condition == 0xdf && card == g_affected_card && player == g_affected_card_controller && g_current_turn == player && player != g_current_player && player == g_trigger_cause_controller && card == g_trigger_cause)
  {
    if (event == 0x7d)
    {
      g_event_result |= 2;
    }
    if (event == 0x7e)
    {
      g_battlefield_extra_ability_flags |= 4;
      PLAYER_CARD_INSTANCE((int)(char)PLAYER_CARD_INSTANCE(player, card).damage_source_player,
                           PLAYER_CARD_INSTANCE(player, card).damage_source_card)
          .token_status |= 0x8000000;
    }
  }

  if ((g_trigger_condition == 0xcc || event == 199) && card == g_affected_card && player == g_affected_card_controller && g_current_turn == player)
  {
    if (event == 0x7d)
    {
      g_event_result |= 2;
    }
    if (event == 0x7e || event == 199)
    {
      kill_card(player, card, KILL_BURY);
      g_battlefield_extra_ability_flags &= 0xfffffffb;

      *(unsigned int *)((char *)&PLAYER_CARD_INSTANCE((int)(char)PLAYER_CARD_INSTANCE(player, card).damage_source_player,
                                                      PLAYER_CARD_INSTANCE(player, card).damage_source_card) +
                        0x18) &= 0xf7ffffff;
    }
  }

  return 0;
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
        if ((instance->state & 0x800022) == 2)
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

// FUNCTION: MAGIC 0x004821f5
// FUNCTION: SHANDALAR 0x00485d06
int reattach_if_attached_to_source(int source_player, int source_card, int test_player, int test_card, int internal_card_id)
{
  (void)internal_card_id;

  if ((int)(char)PLAYER_CARD_INSTANCE(test_player, test_card).damage_target_player == source_player &&
      PLAYER_CARD_INSTANCE(test_player, test_card).damage_target_card == source_card)
  {
    PLAYER_CARD_INSTANCE(test_player, test_card).damage_target_player =
        PLAYER_CARD_INSTANCE(source_player, source_card).damage_source_player;
    PLAYER_CARD_INSTANCE(test_player, test_card).damage_target_card =
        PLAYER_CARD_INSTANCE(source_player, source_card).damage_source_card;
  }

  return 0;
}

// FUNCTION: MAGIC 0x0044837e
// FUNCTION: SHANDALAR 0x00451efb
void notify_library_shuffled(int deck_owner)
{
  HWND hwnd;

  if (deck_owner == 0)
  {
    hwnd = g_duel_main_window_hwnd;
  }
  else
  {
    hwnd = g_duel_status_window_hwnd;
  }

  SendMessageA(hwnd, 0x400, 0, 0);
}

// FUNCTION: MAGIC 0x004b5b7a
// FUNCTION: SHANDALAR 0x0056c37c
void send_library_contents(int deck_owner)
{
  int done;
  int index;

  done = 0;
  index = 0;
  while (done == 0)
  {
    if (global_library[deck_owner][index] != -1 && index < 500)
      AddCardToCLPacket(global_library[deck_owner][index]);
    else
      done = 1;
    ++index;
  }

  AddCardToCLPacket(-1);
  TENTATIVE_send_network_result(0, 4);
}

// FUNCTION: MAGIC 0x004b5c19
// FUNCTION: SHANDALAR 0x0056c41b
void receive_library_contents(int deck_owner)
{
  int index;
  int done;
  int packet_card;

  done = 0;
  index = 0;
  TENTATIVE_wait_for_network_result(1, 4);
  while (done == 0)
  {
    packet_card = GetCardFromCLPacket(index);
    if (packet_card == -1)
    {
      done = 1;
    }
    else
    {
      global_library[deck_owner][index] = packet_card;
      ++index;
    }
  }
}

// FUNCTION: MAGIC 0x004b59b2
// FUNCTION: SHANDALAR 0x0056c1b5
void shuffle_duel_library(int player, int deck_owner)
{
  int swap_slot;    /* [ebp-0x14] */
  int temp;         /* [ebp-0x10] */
  int unused;       /* [ebp-0x0c] */
  int current_slot; /* [ebp-0x08] */
  int deck_size;    /* [ebp-0x04] */

  if (g_duel_ai_mode_state != 1)
  {
    play_sound_effect(WAV_SHUFFLE);
    notify_library_shuffled(deck_owner);
  }

  if ((g_duel_network_flags & 2) == 0 || g_active_player == player)
  {
    deck_size = 500;
    for (current_slot = 0; current_slot < 500; ++current_slot)
    {
      if (global_library[deck_owner][current_slot] == -1)
      {
        deck_size = current_slot;
        break;
      }
    }

    for (current_slot = 0; current_slot < deck_size; ++current_slot)
    {
      swap_slot = current_slot + internal_rand(deck_size - current_slot);
      if (global_library[deck_owner][swap_slot] != -1)
      {
        temp = global_library[deck_owner][swap_slot];
        global_library[deck_owner][swap_slot] = global_library[deck_owner][current_slot];
        global_library[deck_owner][current_slot] = temp;
      }
    }

    for (current_slot = deck_size; current_slot < 500; ++current_slot)
    {
      global_library[deck_owner][current_slot] = -1;
    }

    if ((g_duel_network_flags & 2) != 0)
    {
      send_library_contents(deck_owner);
    }
  }
  else
  {
    receive_library_contents(deck_owner);
  }
}

// FUNCTION: MAGIC 0x004b5c94
// FUNCTION: SHANDALAR 0x0056c496
void remove_card_from_deck(int player, int position)
{
  int current_slot;

  for (current_slot = position; current_slot < 499; ++current_slot)
  {
    global_library[player][current_slot] = global_library[player][current_slot + 1];
  }

  global_library[player][499] = -1;
}

// FUNCTION: MAGIC 0x004b0b53
// FUNCTION: SHANDALAR 0x0046db2e
int regenerate_or_graveyard_triggers(void)
{
  if (pending_killed_card_handler == 0 || g_regenerate_or_graveyard_triggers_in_progress != 0)
  {
    return 0;
  }

  g_regenerate_or_graveyard_triggers_in_progress = 1;

  g_land_can_be_played |= 0x200;
  allow_response(-2, g_current_phase, gs_use_regeneration_effects_0091c680, 0x70);
  g_land_can_be_played &= ~0x200;

  dispatch_trigger_twice_once_with_each_player_as_reason(g_current_player, TRIGGER_GRAVEYARD_ORDER, gs_graveyard_order_0091cbd0, 0);
  dispatch_trigger_twice_once_with_each_player_as_reason(g_current_player, TRIGGER_GRAVEYARD_FROM_PLAY, gs_cards_to_graveyard_008a8ed0, 0);

  pending_killed_card_handler = 0;
  g_regenerate_or_graveyard_triggers_in_progress = 0;

  TENTATIVE_reassess_all_cards(0, 0xff);
  return 0;
}

// FUNCTION: MAGIC 0x005510dc
// FUNCTION: SHANDALAR 0x0049f31f
int has_permanent_of_type(int player, int type_mask)
{
  int current_card;

  for (current_card = 0; current_card < g_active_cards_count[player]; ++current_card)
  {
    if (is_in_play(player, current_card) && (type_mask & global_cards_data[PLAYER_CARD_INSTANCE(player, current_card).internal_card_id].type) != 0)
    {
      return 1;
    }
  }

  return 0;
}

// FUNCTION: MAGIC 0x004b15f7
// FUNCTION: SHANDALAR 0x0046e5d2
void remove_card_from_graveyard(int player, int graveyard_index)
{
  int i = graveyard_index;
  for (; i < 499; ++i)
  {
    global_graveyard_slots[player][i] = global_graveyard_slots[player][i + 1];
  }
  global_graveyard_slots[player][499] = -1;
}

// FUNCTION: MOK 0x0047936d
// FUNCTION: MAGIC 0x004b42aa
int show_deck(int player, int *cards, int count, void *context, int suppress_done_txt, char *prompt)
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
    result = show_cardlist_if_human(cards, count, context, suppress_done_txt, prompt);
    if ((player == g_active_player) && ((g_duel_network_flags & 2) != 0))
    {
      g_network_result_packet.result = result;
      g_network_result_packet.packet_type = 0x19;
      TENTATIVE_send_network_result(player, 0x19);
    }
  }

  return result;
}

// FUNCTION: MOK 0x00459AA0
// FUNCTION: MAGIC 0x00485060
// FUNCTION: SHANDALAR 0x00488b70
int dispatch_function_to_all_cards_in_play(int player, int card, in_play_card_callback_t callback, int who_to_check)
{
  int saved_event_result;
  int callback_result;
  int test_player;
  int test_card;
  int result;

  saved_event_result = g_event_result;
  callback_result = 0;
  g_event_result = -1;
  for (test_player = player;
       test_player > -1 && test_player < 2 && callback_result == 0;
       test_player += (player == 0) ? 1 : -1)
  {
    if (who_to_check == -1 || test_player == who_to_check)
    {
      for (test_card = 0; test_card < g_active_cards_count[test_player] && callback_result == 0; ++test_card)
      {
        if (is_in_play(test_player, test_card))
        {
          callback_result = callback(player,
                                     card,
                                     test_player,
                                     test_card,
                                     global_card_instances[test_player][test_card].internal_card_id);
        }
      }
    }
  }

  result = g_event_result;
  g_event_result = saved_event_result;
  return result;
}

// FUNCTION: MAGIC 0x004964dd
// FUNCTION: SHANDALAR 0x00466f70
int get_card_display_pic_num(card_id_t card_id, int player, int card)
{
  int num_pics;

  if (card_id == -1)
  {
    return 0;
  }
  if (player == -1 || card == -1)
  {
    return 0;
  }
  num_pics = global_raw_cards_storage[card_id].num_pics;
  if (num_pics <= 1)
  {
    return 0;
  }
  else
  {
    return (player + card) % num_pics;
  }
}

// FUNCTION: MAGIC 0x0050047c
// FUNCTION: SHANDALAR 0x004ba5a1
int damage_creature(int target_player, int target_card, int amount, int source_player, int source_card)
{
  int source_internal_card_id;
  int result;
  int damage_player;

  if (target_player == -1 || source_player == -1 || amount <= 0)
  {
    return -1;
  }

  if (target_card == -1)
  {
    damage_player = target_player;
  }
  else
  {
    damage_player = source_player;
  }

  result = add_card_to_hand(damage_player, g_damage_card_internal_card_id);
  if (result != -1)
  {
    PLAYER_CARD_INSTANCE(damage_player, result).state |=
        ((-(damage_player != 0)) & 0x1000) | 2;
    PLAYER_CARD_INSTANCE(damage_player, result).damage_target_player = (char)target_player;
    PLAYER_CARD_INSTANCE(damage_player, result).damage_target_card = target_card;
    PLAYER_CARD_INSTANCE(damage_player, result).info_slot = amount;
    PLAYER_CARD_INSTANCE(damage_player, result).damage_source_player = (char)source_player;
    PLAYER_CARD_INSTANCE(damage_player, result).damage_source_card = source_card;
    if (source_card != -1)
    {
      if (PLAYER_CARD_INSTANCE(source_player, source_card).internal_card_id == -1 ||
          PLAYER_CARD_INSTANCE(source_player, source_card).internal_card_id == g_stack_proxy_internal_card_id)
      {
        source_internal_card_id = PLAYER_CARD_INSTANCE(source_player, source_card).original_internal_card_id;
      }
      else
      {
        source_internal_card_id = PLAYER_CARD_INSTANCE(source_player, source_card).internal_card_id;
      }

      PLAYER_CARD_INSTANCE(damage_player, result).color = PLAYER_CARD_INSTANCE(source_player, source_card).color;
      if ((global_cards_data[source_internal_card_id].type & TYPE_ARTIFACT) != 0)
      {
        PLAYER_CARD_INSTANCE(damage_player, result).color |= 0x40;
      }

      PLAYER_CARD_INSTANCE(damage_player, result).eot_toughness =
          (unsigned int)(unsigned char)global_cards_data[source_internal_card_id].type;

      if ((PLAYER_CARD_INSTANCE(source_player, source_card).state & 4) != 0)
      {
        if (g_current_phase == 0x19)
        {
          PLAYER_CARD_INSTANCE(damage_player, result).token_status |= 0x100000;
        }
        if (g_current_phase == 0x1a)
        {
          PLAYER_CARD_INSTANCE(damage_player, result).token_status |= 0x40000;
        }
      }
      if (global_cards_data[source_internal_card_id].id == g_effect_display_internal_card_id || global_cards_data[source_internal_card_id].id == g_legacy_display_internal_card_id || global_cards_data[source_internal_card_id].id == g_multiblock_display_internal_card_id)
      {
        *(unsigned int *)&PLAYER_CARD_INSTANCE(damage_player, result).display_pic_csv_id =
            *(unsigned int *)&PLAYER_CARD_INSTANCE(source_player, source_card).display_pic_csv_id;
      }
      else
      {
        *(unsigned int *)&PLAYER_CARD_INSTANCE(damage_player, result).display_pic_csv_id =
            (get_card_display_pic_num(global_cards_data[source_internal_card_id].id, source_player, source_card) << 16) |
            global_cards_data[source_internal_card_id].id;
      }
    }
    else
    {
      *(unsigned int *)&PLAYER_CARD_INSTANCE(damage_player, result).display_pic_csv_id = 0xef;
    }
    g_land_can_be_played |= 2;
  }

  return result;
}

// FUNCTION: MAGIC 0x004b4654
// FUNCTION: SHANDALAR 0x0056ae5d
int add_card_to_hand(int player, int internal_card_id)
{
  int card;

  if (internal_card_id == -1)
  {
    return -1;
  }

  for (card = 0; card < 0x96; ++card)
  {
    if (PLAYER_CARD_INSTANCE(player, card).original_internal_card_id == -1)
    {
      initialize_card_instance(player, internal_card_id, card);
      if (g_active_cards_count[player] <= card)
      {
        g_active_cards_count[player] = card + 1;
      }
      return card;
    }
  }

  show_duel_system_error("AddCard error: No more room in cd to add a card");
  return -1;
}

int calculate_attack_rating(int player, int card);

// FUNCTION: MAGIC 0x004b4720
// FUNCTION: SHANDALAR 0x0056af29
void initialize_card_instance(int player, int internal_card_id, int card)
{
  int i;

  PLAYER_CARD_INSTANCE(player, card).original_internal_card_id = internal_card_id;
  PLAYER_CARD_INSTANCE(player, card).internal_card_id = PLAYER_CARD_INSTANCE(player, card).original_internal_card_id;
  PLAYER_CARD_INSTANCE(player, card).dummy3 = 0;

  if (player == 0)
  {
    PLAYER_CARD_INSTANCE(player, card).state = 0;
  }
  else
  {
    PLAYER_CARD_INSTANCE(player, card).state = 0x1000;
  }

  PLAYER_CARD_INSTANCE(player, card).damage_on_card = 0;
  PLAYER_CARD_INSTANCE(player, card).damage_target_player = (char)-1;
  PLAYER_CARD_INSTANCE(player, card).damage_target_card = -1;
  PLAYER_CARD_INSTANCE(player, card).damage_source_player = (char)-1;
  PLAYER_CARD_INSTANCE(player, card).damage_source_card = -1;

  PLAYER_CARD_INSTANCE(player, card).power = global_cards_data[internal_card_id].power;
  PLAYER_CARD_INSTANCE(player, card).toughness = global_cards_data[internal_card_id].toughness;
  PLAYER_CARD_INSTANCE(player, card).counter_power = 0;
  PLAYER_CARD_INSTANCE(player, card).counter_toughness = 0;

  PLAYER_CARD_INSTANCE(player, card).color = (char)global_cards_data[internal_card_id].color;
  PLAYER_CARD_INSTANCE(player, card).mana_color = PLAYER_CARD_INSTANCE(player, card).color;

  PLAYER_CARD_INSTANCE(player, card).blocking = 0xff;
  PLAYER_CARD_INSTANCE(player, card).unknown0x37 = 0;
  PLAYER_CARD_INSTANCE(player, card).kill_code = 0;

  PLAYER_CARD_INSTANCE(player, card).eot_toughness = 0;
  PLAYER_CARD_INSTANCE(player, card).info_slot = PLAYER_CARD_INSTANCE(player, card).eot_toughness;
  PLAYER_CARD_INSTANCE(player, card).timestamp = -1;

  PLAYER_CARD_INSTANCE(player, card).token_status = 0;
  PLAYER_CARD_INSTANCE(player, card).regen_status = 0x08000000;

  PLAYER_CARD_INSTANCE(player, card).attack_rating = calculate_attack_rating(player, card);

  PLAYER_CARD_INSTANCE(player, card).special_counters = 0;
  PLAYER_CARD_INSTANCE(player, card).unknown0x14 = 0;
  PLAYER_CARD_INSTANCE(player, card).number_of_targets = 0;
  PLAYER_CARD_INSTANCE(player, card).untap_status = 0;
  PLAYER_CARD_INSTANCE(player, card).upkeep_flags = PLAYER_CARD_INSTANCE(player, card).untap_status;

  for (i = 0; i < 7; ++i)
  {
    PLAYER_CARD_INSTANCE(player, card).mana_to_untap[i] = 0;
  }

  PLAYER_CARD_INSTANCE(player, card).upkeep_colorless = 0;
  PLAYER_CARD_INSTANCE(player, card).upkeep_black = 0;
  PLAYER_CARD_INSTANCE(player, card).upkeep_blue = 0;
  PLAYER_CARD_INSTANCE(player, card).upkeep_green = 0;
  PLAYER_CARD_INSTANCE(player, card).upkeep_red = 0;
  PLAYER_CARD_INSTANCE(player, card).upkeep_white = 0;
  PLAYER_CARD_INSTANCE(player, card).upkeep_artmana = 0;

  for (i = 0; i < 6; ++i)
  {
    PLAYER_CARD_INSTANCE(player, card).color_id[i] = 0;
    PLAYER_CARD_INSTANCE(player, card).hack_mode[i] = 0;
  }

  for (i = 0; i < 19; ++i)
  {
    PLAYER_CARD_INSTANCE(player, card).targets[i].player = (char)-1;
    PLAYER_CARD_INSTANCE(player, card).targets[i].card = -1;
  }

  PLAYER_CARD_INSTANCE(player, card).parent_controller = -1;
  PLAYER_CARD_INSTANCE(player, card).parent_card = -1;

  PLAYER_CARD_INSTANCE(player, card).counters = 0;

  if ((global_cards_data[internal_card_id].extra_ability & EA_MANA_SOURCE))
  {
    if (global_cards_data[internal_card_id].type == 1 || global_cards_data[internal_card_id].type == 0x40)
    {
      PLAYER_CARD_INSTANCE(player, card).color = 1;
    }

    switch (global_cards_data[internal_card_id].id)
    {
    case 0x134:
    case 0x27b:
      PLAYER_CARD_INSTANCE(player, card).mana_color = 0x10;
      break;

    case 0x139:
      PLAYER_CARD_INSTANCE(player, card).mana_color = 2;
      break;

    case 300:
    case 0x272:
      PLAYER_CARD_INSTANCE(player, card).mana_color = 1;
      break;

    case 0xf:
    case 0x193:
      PLAYER_CARD_INSTANCE(player, card).mana_color = 0x3e;
      break;

    default:
      break;
    }
  }

  TENTATIVE_set_timestamps(player, card);
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

// FUNCTION: MAGIC 0x00500135
// FUNCTION: SHANDALAR 0x004ba25e
int create_a_card_type(int internal_card_id)
{
  int new_internal_card_id;

  for (new_internal_card_id = g_card_count; new_internal_card_id < g_card_count + 0x10; ++new_internal_card_id)
  {
    if (*(int *)&global_cards_data[new_internal_card_id].id == -1)
    {
      memcpy(&global_cards_data[new_internal_card_id], &global_cards_data[internal_card_id], 0x48);
      return new_internal_card_id;
    }
  }

  show_duel_system_error("AddType error: No room in ci to add another type");
  return -1;
}

// FUNCTION: MAGIC 0x00442b42
// FUNCTION: SHANDALAR 0x0040e3dc
int dispatch_event_to_single_card(int player,
                                  int card,
                                  event_t event,
                                  int new_attacking_card_controller,
                                  int new_attacking_card)
{
  int saved_unk_008b3270;
  int result;

  if (PLAYER_CARD_INSTANCE(player, card).internal_card_id == -1)
  {
    return 0;
  }

  if (event == EVENT_CAN_ACTIVATE)
  {
    if ((PLAYER_CARD_INSTANCE(player, card).state & STATE_OUBLIETTED) != 0 ||
        (PLAYER_CARD_INSTANCE(player, card).upkeep_flags & UPKEEP_UPKEEP_CANTPAY) != 0)
    {
      return 0;
    }
  }

  push_affected_card_stack();

  g_event_result = 0;
  g_affected_card_controller = player;
  g_affected_card = card;
  g_attacking_card_controller = new_attacking_card_controller;
  g_attacking_card = new_attacking_card;

  saved_unk_008b3270 = g_activation_event_flags;

  result = (*global_cards_data[PLAYER_CARD_INSTANCE(player, card).internal_card_id].code_pointer)(player, card, event);

  if (result != 0x63 && (g_land_can_be_played & 0x224) != 0 && (event == EVENT_CAN_CAST || event == EVENT_CAN_ACTIVATE))
  {
    if (is_nonactivated_mana_source(player, card) == 0)
    {
      g_activation_event_flags = saved_unk_008b3270;
      pop_affected_card_stack();
      return 0;
    }
  }

  g_last_single_card_event_result = g_event_result;
  pop_affected_card_stack();
  return result;
}

// FUNCTION: MAGIC 0x004428be
// FUNCTION: SHANDALAR 0x0040e158
int dispatch_event(int player, int card, event_t event)
{
  int result;                   // ebp-0x8
  trigger_t saved_trigger_cond; // ebp-0x4

  push_affected_card_stack();

  g_event_result = 0;
  g_affected_card_controller = player;
  g_affected_card = card;
  g_attacking_card_controller = 1 - player;
  g_attacking_card = -1;

  saved_trigger_cond = g_trigger_condition;
  if (event != EVENT_TRIGGER && event != EVENT_RESOLVE_TRIGGER)
  {
    g_trigger_condition = -1;
  }

  C_dispatch_event_raw(event);

  g_affected_card_controller = -1;
  g_trigger_condition = saved_trigger_cond;

  result = g_event_result;
  pop_affected_card_stack();
  return result;
}

// FUNCTION: MAGIC 0x0043e18b
// FUNCTION: SHANDALAR 0x00409a0b
int draw_card_for_player(int player)
{
  int candidate_ok;      // ebp-0xc
  int drawn_card;        // ebp-0x8
  int desired_type_mask; // ebp-0x4

  g_duel_state_0091c4fc = 0;
  dispatch_trigger(player, TRIGGER_REPLACE_CARD_DRAW, gs_draw_a_card_008961c0, 1);
  if (g_duel_state_0091c4fc != 0)
  {
    return 0;
  }

  if (player == g_active_player)
  {
    if (g_selected_wizard_color == -1)
    {
      drawn_card = global_library[player][0];
      if (drawn_card != -1)
      {
        remove_card_from_deck(player, 0);
        drawn_card = add_card_to_hand(player, drawn_card);
      }
    }
    else
    {
      drawn_card = add_card_to_hand(player, DrawRandomCardFromInitialLibrary(g_selected_wizard_color));
    }

    if (drawn_card != -1 && PLAYER_CARD_INSTANCE(player, drawn_card).internal_card_id != -1)
    {
      TENTATIVE_reassess_all_cards(0, 0x30);
      if (g_duel_ai_mode_state != 1)
      {
        load_text(global_ui_strings_filename, "PROMPT_DRAWACARD");
        if (g_duel_active != 0)
        {
          show_duel_card_preview(PLAYER_CARD_INSTANCE(player, drawn_card).internal_card_id,
                                 player,
                                 drawn_card,
                                 0xf6,
                                 g_text_lines[0],
                                 1);
        }
        else
        {
          show_card_preview_if_human(PLAYER_CARD_INSTANCE(player, drawn_card).internal_card_id,
                                     0xf6,
                                     g_text_lines[0],
                                     1);
        }
      }
    }
    else if (g_duel_ai_mode_state != 1)
    {
      load_text(global_ui_strings_filename, "PROMPT_DRAWACARD");
      set_duel_prompt_text(g_text_lines[1]);
      Sleep(0x9c4);
      set_duel_prompt_text("");
      exit_duel_thread(0);
    }
    else
    {
      g_life[g_active_player] = -99;
      g_life[1 - g_active_player] = 0x14;
    }
  }
  else
  {
    if (g_opponent_initial_library_index == -1)
    {
      switch (internal_rand(3))
      {
      case 0:
        desired_type_mask = 1;
        break;
      case 1:
        desired_type_mask = 2;
        break;
      case 2:
        desired_type_mask = 0x3c;
        break;
      }

      do
      {
        drawn_card = internal_rand(g_card_count);
        candidate_ok = is_card_color_compatible_with_mask((int)(char)global_cards_data[drawn_card].color,
                                                          g_current_encounter_color,
                                                          g_encounter_color_match_mode);
        if (candidate_ok != 0 && GetCardRarity(drawn_card) > g_current_encounter_strength)
        {
          candidate_ok = 0;
        }
      } while (candidate_ok == 0 ||
               (desired_type_mask & (unsigned char)global_cards_data[drawn_card].type) == 0 ||
               (global_cards_data[drawn_card].extra_ability & 0x40) != 0);

      drawn_card = add_card_to_hand(player, drawn_card);
    }
    else if (global_library[player][0] == -1)
    {
      drawn_card = -1;
    }
    else
    {
      drawn_card = add_card_to_hand(player, global_library[player][0]);
      if (drawn_card != -1)
      {
        remove_card_from_deck(player, 0);
      }
    }

    if (drawn_card == -1)
    {
      if (g_duel_ai_mode_state != 1)
      {
        load_text(global_ui_strings_filename, "PROMPT_DRAWACARD");
        set_duel_prompt_text(g_text_lines[2]);
        Sleep(0x9c4);
        set_duel_prompt_text("");
        exit_duel_thread(1);
      }
      else
      {
        g_life[1 - g_active_player] = -99;
        g_life[g_active_player] = 0x14;
      }
    }

    TENTATIVE_reassess_all_cards(0, 0x30);
  }

  ++g_duel_summary.hand_counts[player];
  ++g_duel_summary.cards_drawn;
  if (g_duel_ai_mode_state != 1 && drawn_card != -1)
  {
    play_sound_effect(2);
  }
  if (drawn_card != -1)
  {
    PLAYER_CARD_INSTANCE(player, drawn_card).state |= 1;
  }

  return drawn_card;
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

  if (in_play != 0 && g_current_phase == 0x15 && player == g_current_player && (state & 0x8000) != 0 && (state & 4) == 0 && can_attack(player, card) != 0)
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
    if (in_play != 0 && (state & 0x10) == 0 && ((type & TYPE_CREATURE) != 0 || (state & 0x3000000) != 0))
    {
      if (player == g_current_player && can_attack(player, card) != 0 && (state & 0x10000) == 0)
      {
        result = 0x10;
        goto finish_get_available_card_action;
      }
      if (player != g_current_player && g_attacking_creature_count != 0 && (state & 8) == 0)
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

      if ((state & 0x10) == 0 && (type & TYPE_CREATURE) != 0 && g_response_selection_in_progress == 0 && player == g_current_player && g_current_phase <= 0x1a && can_attack(player, card) != 0 && (((PLAYER_CARD_INSTANCE(player, card).state & 0x30000) == 0) || (global_cards_data[PLAYER_CARD_INSTANCE(player, card).internal_card_id].type & TYPE_CREATURE) == 0))
      {
        g_attack_action_available = 1;
      }

      if ((((extra_ability & 0x1000) != 0 && (state & 0x10) == 0 && (((PLAYER_CARD_INSTANCE(player, card).state & 0x30000) == 0) || (global_cards_data[PLAYER_CARD_INSTANCE(player, card).internal_card_id].type & TYPE_CREATURE) == 0)) || ((extra_ability & 1) != 0 && (g_response_card_type_mask & 0x10) != 0) || ((extra_ability & 2) != 0 && (g_response_card_type_mask & 0x20) != 0)) && ((g_land_can_be_played & 4) == 0 || (extra_ability & 0x5004) != 0))
      {
        g_activation_event_flags &= ~2;
        if ((state & 0x20) == 0 && dispatch_event_to_single_card(player, card, EVENT_CAN_ACTIVATE, 1 - player, -1) != 0)
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
      if ((state & 0xa0) != 0)
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
          if (player == g_current_player && (g_land_can_be_played & 1) == 0 && (g_current_phase == 0x14 || g_current_phase == 0x1e))
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

        if ((((g_active_player == g_current_player && (((g_response_selection_in_progress != 0 && (type & 0x30) != 0) || g_current_phase == 0x14) || g_current_phase == 0x1e)) || (g_active_player != g_current_player && g_response_selection_in_progress != 0 && ((type & 0x10) != 0 || (type & 0x20) != 0))) && (unused_18 = can_pay_card_mana_cost(player, player, card)) != 0 && (((g_land_can_be_played & 4) == 0 || (extra_ability & 0x3004) != 0) && ((type & 0x42) != 0 || dispatch_event_to_single_card(player, card, EVENT_CAN_CAST, 1 - player, -1) != 0))))
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

// FUNCTION: MAGIC 0x004e1cd1
// FUNCTION: SHANDALAR 0x00426f3c
void redraw_shandalar_duel_screen(int view_player, int present_after_draw)
{
  /*
   * MAGIC keeps Shandalar's two-argument screen redraw hook, but compiles it
   * as a no-op. SHANDALAR's legacy body is only the no-active-duel fallback;
   * active duels redraw through the duel window message path instead.
   */
  (void)view_player;
  (void)present_after_draw;
}

// FUNCTION: MOK 0x0049d510
// FUNCTION: MAGIC 0x004eaf09
// FUNCTION: SHANDALAR 0x00441699
int has_mana(int player, color_t color, int amount)
{
  int result;                   // ebp-0x2c
  int available_mana;           // ebp-0x28
  unsigned int interchangeable; // ebp-0x24
  color_t effective_color;      // ebp-0x20
  int current_color;            // ebp-0x1c
  unsigned int mana_color;      // ebp-0x18
  int special_mana;             // ebp-0x14
  int artifact_mana;            // ebp-0x10
  int xmana;                    // ebp-0xc
  int artifact_special_mana;    // ebp-0x8
  int mana_source_color;        // ebp-0x4 (only low byte used)

  if (color == COLOR_ARTIFACT)
  {
    effective_color = COLOR_ANY;
  }
  else if (color == COLOR_COLORLESS)
  {
    effective_color = COLOR_ANY;
  }
  else
  {
    effective_color = color;
  }

  if (amount == 0)
  {
    return 1;
  }

  interchangeable = 0;
  for (current_color = 0; current_color < 10 && g_mana_color_conversions[player][current_color] != -1; ++current_color)
  {
    mana_source_color = (unsigned short)g_mana_color_conversions[player][current_color];
    mana_color = HIWORD(g_mana_color_conversions[player][current_color]);
    if (effective_color == (color_t)mana_color)
    {
      interchangeable |= 1 << (unsigned char)mana_source_color;
    }
  }

  available_mana = g_raw_mana_available[player][effective_color];
  for (current_color = COLOR_COLORLESS; current_color < 7; ++current_color)
  {
    if (effective_color != (color_t)current_color && (interchangeable & (1 << (unsigned char)current_color)) != 0)
    {
      available_mana += g_raw_mana_available[player][current_color];
    }
  }

  artifact_mana = g_raw_mana_available[player][COLOR_ARTIFACT];

  special_mana = g_special_mana_pool[player][effective_color];
  for (current_color = COLOR_COLORLESS; current_color < 7; ++current_color)
  {
    if (effective_color != (color_t)current_color && (interchangeable & (1 << (unsigned char)current_color)) != 0)
    {
      special_mana += g_special_mana_pool[player][current_color];
    }
  }

  xmana = 0;
  for (current_color = 0; current_color < 0x32 && g_raw_mana_available_hex[player][current_color] != -1; ++current_color)
  {
    if (effective_color == COLOR_ANY)
    {
      xmana += g_raw_mana_available_hex[player][current_color] >> 16;
    }
    else if ((g_raw_mana_available_hex[player][current_color] & (1 << (unsigned char)effective_color)) != 0)
    {
      xmana += g_raw_mana_available_hex[player][current_color] >> 16;
    }
    else
    {
      if ((interchangeable & g_raw_mana_available_hex[player][current_color]) != 0)
      {
        xmana += g_raw_mana_available_hex[player][current_color] >> 16;
      }
    }
  }

  artifact_special_mana = g_special_mana_pool[player][COLOR_ARTIFACT];

  if (color == COLOR_ANY || color == COLOR_COLORLESS)
  {
    result = ((available_mana - artifact_mana) + special_mana - artifact_special_mana) + xmana;
  }
  else
  {
    result = (available_mana + special_mana) + xmana;
  }

  if (result >= amount)
  {
    return result;
  }
  else
  {
    return 0;
  }
}

// FUNCTION: MAGIC 0x0043fdb3
// FUNCTION: SHANDALAR 0x0040b633
int can_pay_card_mana_cost(int player, int target_player, int target_card)
{
  int color;
  int internal_card_id;
  int result;

  result = 1;
  if (player == -1 || target_player == -1 || target_card == -1)
  {
    return 0;
  }

  internal_card_id = PLAYER_CARD_INSTANCE(target_player, target_card).internal_card_id;
  if (internal_card_id == -1)
  {
    return 0;
  }

  if ((global_cards_data[internal_card_id].type & TYPE_ARTIFACT) != 0)
  {
    if (has_mana(player,
                 COLOR_ARTIFACT,
                 (int)(char)global_cards_data[internal_card_id].cc[0] +
                     (int)(char)global_cards_data[internal_card_id].cc[1]) == 0)
    {
      result = 0;
    }
  }
  else
  {
    color = single_color_test_bit_to_color_t((int)(char)global_cards_data[internal_card_id].color);
    if (has_mana_w_global_cost_mod(player,
                                   target_card,
                                   color,
                                   (int)(char)global_cards_data[internal_card_id].cc[0]) == 0)
    {
      result = 0;
    }

    if ((int)(char)global_cards_data[internal_card_id].cc[1] > 0)
    {
      if (has_mana_w_global_cost_mod(player,
                                     target_card,
                                     COLOR_ANY,
                                     (int)(char)global_cards_data[internal_card_id].cc[0] +
                                         (int)(char)global_cards_data[internal_card_id].cc[1]) == 0)
      {
        result = 0;
      }
    }
  }

  return result;
}

// FUNCTION: MAGIC 0x00442dac
// FUNCTION: SHANDALAR 0x0040e646
void push_affected_card_stack(void)
{
  if (g_affected_card_context_depth >= 0x20)
    return;

  g_affected_card_context_stack[g_affected_card_context_depth][0] = g_affected_card_controller;
  g_affected_card_context_stack[g_affected_card_context_depth][1] = g_affected_card;
  g_affected_card_context_stack[g_affected_card_context_depth][2] = g_affected_internal_card_id;
  g_affected_card_context_stack[g_affected_card_context_depth][3] = g_affected_card_color;
  g_affected_card_context_stack[g_affected_card_context_depth][4] = g_attacking_card_controller;
  g_affected_card_context_stack[g_affected_card_context_depth][5] = g_attacking_card;
  g_affected_card_context_stack[g_affected_card_context_depth][6] = g_event_result;
  g_affected_card_context_depth += 1;
}

// FUNCTION: MAGIC 0x00442e62
// FUNCTION: SHANDALAR 0x0040e6fc
void pop_affected_card_stack(void)
{
  if (g_affected_card_context_depth > 0)
  {
    g_affected_card_context_depth -= 1;
  }

  g_affected_card_controller = g_affected_card_context_stack[g_affected_card_context_depth][0];
  g_affected_card = g_affected_card_context_stack[g_affected_card_context_depth][1];
  g_affected_internal_card_id = g_affected_card_context_stack[g_affected_card_context_depth][2];
  g_affected_card_color = g_affected_card_context_stack[g_affected_card_context_depth][3];
  g_attacking_card_controller = g_affected_card_context_stack[g_affected_card_context_depth][4];
  g_attacking_card = g_affected_card_context_stack[g_affected_card_context_depth][5];
  g_event_result = g_affected_card_context_stack[g_affected_card_context_depth][6];
}

// FUNCTION: MAGIC 0x00445b56
// FUNCTION: SHANDALAR 0x004113ef
int can_pay_untap_cost(int player, int card)
{
  int total_paid;
  color_t current_color;

  if ((PLAYER_CARD_INSTANCE(player, card).state & STATE_TAPPED) == 0)
  {
    return 0;
  }

  total_paid = 0;
  current_color = COLOR_BLACK;
  for (; current_color < 7; ++current_color)
  {
    if (PLAYER_CARD_INSTANCE(player, card).mana_to_untap[current_color] != 0)
    {
      if (has_mana(player, current_color,
                   PLAYER_CARD_INSTANCE(player, card).mana_to_untap[current_color]) == 0)
      {
        return 0;
      }
      total_paid += PLAYER_CARD_INSTANCE(player, card).mana_to_untap[current_color];
    }
  }

  if (has_mana(player, COLOR_ANY, PLAYER_CARD_INSTANCE(player, card).mana_to_untap[0] + total_paid) == 0)
  {
    return 0;
  }

  dispatch_event_to_single_card(player, card, EVENT_CHECK_UNTAP_PAYMENT, 1 - player, -1);
  if (g_last_single_card_event_result != 0)
    return 0;

  return 1;
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
  return -1;
}

// FUNCTION: MAGIC 0x0055117d
// FUNCTION: SHANDALAR 0x0049f3c0
void dispatch_three_arg_callback_to_cards_in_play(int(__cdecl *callback)(int, int, int), int who_to_check)
{
  int test_card;
  int test_player;

  for (test_player = 0; test_player < 2; ++test_player)
  {
    if (who_to_check == -1 || test_player == who_to_check)
    {
      for (test_card = 0; test_card < g_active_cards_count[test_player]; ++test_card)
      {
        if (is_in_play(test_player, test_card))
        {
          callback(test_player,
                   test_card,
                   global_card_instances[test_player][test_card].internal_card_id);
        }
      }
    }
  }
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
    PLAYER_CARD_INSTANCE(player, card).targets[(char)PLAYER_CARD_INSTANCE(player, card).number_of_targets] = selected_target;
    ++PLAYER_CARD_INSTANCE(player, card).number_of_targets;
    return 1;
  }

  return 0;
}

// FUNCTION: MAGIC 0x004eab5c
// FUNCTION: SHANDALAR 0x004412ec
void declare_mana_available_hex(int player, color_test_t colors, int amount)
{
  int i;

  if ((int)colors > 0)
  {
    for (i = 0; i < 0x32; ++i)
    {
      if (g_raw_mana_available_hex[player][i] == -1)
      {
        g_raw_mana_available_hex[player][i] = (amount << 16) | colors;
        g_raw_mana_available_hex[player][i + 1] = -1;
        break;
      }
    }
  }
}

// FUNCTION: MAGIC 0x004eac0c
// FUNCTION: SHANDALAR 0x0044139c
void undeclare_mana_available_hex(int player, color_test_t color, int amount)
{
  int j;     /* [ebp-0x0c] */
  int i;     /* [ebp-0x08] */
  int found; /* [ebp-0x04] */

  found = 0;
  i = 0;

  while (i < 0x32)
  {
    if (g_raw_mana_available_hex[player][i] == -1)
    {
      break;
    }

    if (found != 0)
    {
      break;
    }

    if (g_raw_mana_available_hex[player][i] == ((amount << 16) | color))
    {
      found = 1;
      j = i;
      while (j < 0x31)
      {
        g_raw_mana_available_hex[player][j] = g_raw_mana_available_hex[player][j + 1];
        ++j;
      }
    }

    ++i;
  }
}

// FUNCTION: MAGIC 0x004eada9
// FUNCTION: SHANDALAR 0x00441539
int declare_mana_available(int player, color_t color, int amount)
{
  g_special_mana_pool[player][color] += amount;
  g_special_mana_pool[player][COLOR_ANY] += amount;
  return g_special_mana_pool[player][color];
}

// FUNCTION: MAGIC 0x004eadeb
// FUNCTION: SHANDALAR 0x0044157b
int undeclare_mana_available(int player, color_t color, int amount)
{
  g_special_mana_pool[player][color] -= amount;
  g_special_mana_pool[player][COLOR_ANY] -= amount;
  return g_special_mana_pool[player][color];
}

// FUNCTION: MAGIC 0x004eaec1
// FUNCTION: SHANDALAR 0x00441651
int undeclare_mana_available_and_produce_it(int player, color_t color, int amount)
{
  undeclare_mana_available(player, color, amount);
  produce_mana(player, color, amount);
  return g_raw_mana_available[player][color];
}

// FUNCTION: MAGIC 0x005513d7
// FUNCTION: SHANDALAR 0x0049f61a
void add_special_counters(int player, int card, int amount)
{
  if ((PLAYER_CARD_INSTANCE(player, card).special_counters & 0xffu) == 0xffu)
    return;

  PLAYER_CARD_INSTANCE(player, card).special_counters =
      ((PLAYER_CARD_INSTANCE(player, card).special_counters + amount) & 0xff) |
      (PLAYER_CARD_INSTANCE(player, card).special_counters & 0xffffff00);

  if (g_duel_ai_mode_state != 1)
  {
    play_sound_effect(ICON_HASTE);
  }
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

// FUNCTION: MOK 0x0044e050
// FUNCTION: MAGIC 0x004e1d6d
int legacy_blit_graphics_rect_stub(int dst, int dst_x, int dst_y, int width, int height,
                                   int src, int src_x, int src_y, int src_width, int src_height)
{
  return 0;
}

// FUNCTION: MAGIC 0x004e1d91
int legacy_load_pcx_into_page_stub(int page, int x, int y, const char *filename, void *palette)
{
  return 1;
}

// FUNCTION: MAGIC 0x004e1da6
int legacy_load_pcx_into_page_no_palette_stub(const char *filename)
{
  return 1;
}

// FUNCTION: MAGIC 0x004e1c81
int legacy_clear_graphics_page_stub(int page, int color)
{
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

// FUNCTION: MAGIC 0x0048463d
// FUNCTION: SHANDALAR 0x00488147
int card_has_basic_land_type(int player, int card, int land_color)
{
  int result;
  int internal_card_id;

  result = 0;

  if (global_card_instances[player][card].internal_card_id != -1)
  {
    internal_card_id = global_card_instances[player][card].internal_card_id;
  }
  else
  {
    internal_card_id = global_card_instances[player][card].original_internal_card_id;
  }

  if (player == -1 || card == -1)
  {
    return 0;
  }

  if ((char)global_cards_data[internal_card_id].subtype == 13 && global_cards_data[internal_card_id].id == g_duel_special_land_card_ids[land_color])
  {
    result = 1;
  }
  if ((char)global_cards_data[internal_card_id].subtype == 12 && ((1 << (unsigned char)land_color) & (int)(char)global_card_instances[player][card].mana_color) != 0)
  {
    result = 1;
  }

  return result;
}

// FUNCTION: MAGIC 0x004817fd
// FUNCTION: SHANDALAR 0x00485308
int ante_drawn_card(int player)
{
  int ante_slot = 0;
  int card_in_hand;

  while (global_ante_cards[player][ante_slot] != -1 && ante_slot < 0x10)
  {
    ++ante_slot;
  }

  if (ante_slot < 0x10)
  {
    card_in_hand = draw_card_for_player(player);
    global_ante_cards[player][ante_slot] = PLAYER_CARD_INSTANCE(player, card_in_hand).internal_card_id;
    --g_duel_summary.hand_counts[player];
    load_text("promptsX1.txt", "ANTE_A_CARD");
    if (g_active_player == player)
    {
      do_dialog(player, player, card_in_hand, -1, -1, g_text_lines[0], 0);
    }
    else
    {
      do_dialog(player, player, card_in_hand, -1, -1, g_text_lines[1], 0);
    }

    PLAYER_CARD_INSTANCE(player, card_in_hand).internal_card_id = -1;

    return 1;
  }
  else
  {
    return 0;
  }
}

// FUNCTION: MAGIC 0x00483489
// FUNCTION: SHANDALAR 0x00486f9a
int card_matches_excluded_land_type(int player, int card, int excluded_internal_card_id)
{
  int internal_card_id;

  internal_card_id = PLAYER_CARD_INSTANCE(player, card).internal_card_id;

  if (excluded_internal_card_id == -1)
  {
    return 0;
  }

  if (global_cards_data[excluded_internal_card_id].code_pointer == (int)global_cards_data[internal_card_id].code_pointer)
  {
    return 1;
  }

  if ((int)*(char *)&global_cards_data[internal_card_id].subtype == 0xc)
  {
    if ((int)*(char *)&global_cards_data[excluded_internal_card_id].subtype != 0xc)
    {
      if (((int)*(char *)&global_cards_data[excluded_internal_card_id].color & (int)*(char *)&global_cards_data[internal_card_id].color) != 0)
      {
        return 1;
      }
    }
  }

  return 0;
}

// FUNCTION: MAGIC 0x004bff5a
// FUNCTION: SHANDALAR 0x004c0dd4
int has_pending_damage(int player, int card)
{
  int result;
  int test_card;
  int test_player;

  result = 0;
  for (test_player = 0; test_player < 2 && result == 0; ++test_player)
  {
    for (test_card = 0; test_card < g_active_cards_count[test_player] && result == 0; ++test_card)
    {
      if (PLAYER_CARD_INSTANCE(test_player, test_card).internal_card_id == g_damage_card_internal_card_id &&
          PLAYER_CARD_INSTANCE(test_player, test_card).damage_target_player == player &&
          PLAYER_CARD_INSTANCE(test_player, test_card).damage_target_card == card)
      {
        result = 1;
      }
    }
  }
  return result;
}

// FUNCTION: MAGIC 0x004c081a
// FUNCTION: SHANDALAR 0x004c1695
int is_basic_land(int player, int card)
{
  int result;

  result = 0;
  if ((int)(char)global_cards_data[PLAYER_CARD_INSTANCE(player, card).internal_card_id].subtype == 0xd)
  {
    result = 1;
  }

  return result;
}

// FUNCTION: MAGIC 0x004c0880
// FUNCTION: SHANDALAR 0x004c16fb
int basic_land_matches_type_index(int internal_card_id, int extra)
{
  int result;

  result = 0;
  if (extra == 0 && (extra == internal_card_id || global_cards_data[internal_card_id].id == g_duel_special_land_card_ids[1]))
  {
    result = 1;
  }
  if (extra == 1 && (extra == internal_card_id || global_cards_data[internal_card_id].id == g_duel_special_land_card_ids[2]))
  {
    result = 1;
  }
  if (extra == 2 && (extra == internal_card_id || global_cards_data[internal_card_id].id == g_duel_special_land_card_ids[3]))
  {
    result = 1;
  }
  if (extra == 3 && (extra == internal_card_id || global_cards_data[internal_card_id].id == g_duel_special_land_card_ids[4]))
  {
    result = 1;
  }
  if (extra == 4 && (extra == internal_card_id || global_cards_data[internal_card_id].id == g_duel_special_land_card_ids[5]))
  {
    result = 1;
  }
  return result;
}

// FUNCTION: MAGIC 0x004848a0
// FUNCTION: SHANDALAR 0x004883aa
int gain_life(int player, int amount, ...)
{
  int saved_trigger_cause_controller;
  int saved_trigger_cause;
  int num_cards_drawn;

  g_life[player] += amount;

  if ((g_battlefield_extra_ability_flags & 0x00200000) != 0)
  {
    saved_trigger_cause_controller = g_trigger_cause_controller;
    saved_trigger_cause = g_trigger_cause;
    push_affected_card_stack();

    g_trigger_cause_controller = player;
    g_trigger_cause = -1;
    g_life_gained = amount;
    dispatch_trigger_twice_once_with_each_player_as_reason(g_current_player, TRIGGER_GAIN_LIFE, &gs_gain_life_007895e0[0], 0);

    g_trigger_cause_controller = saved_trigger_cause_controller;
    g_trigger_cause = saved_trigger_cause;
    pop_affected_card_stack();
  }

  if (g_lich_active[player] != 0)
  {
    for (num_cards_drawn = 0; num_cards_drawn < amount; ++num_cards_drawn)
    {
      draw_card_for_player(player);
    }
  }

  return 0;
}

// FUNCTION: MAGIC 0x00445663
// FUNCTION: SHANDALAR 0x00410efc
int dispatch_trigger_twice_once_with_each_player_as_reason(int reason_for_trig, trigger_t trig, const char *prompt, int a4)
{
  dispatch_trigger(reason_for_trig, trig, prompt, a4);
  dispatch_trigger(1 - reason_for_trig, trig, prompt, a4);
  return 1;
}

// FUNCTION: MAGIC 0x004b1670
// FUNCTION: SHANDALAR 0x0046e64b
void move_card_to_exile(int player, int internal_card_id)
{
  int exile_index;

  if (g_duel_ai_mode_state != 1 && (global_cards_data[internal_card_id].type & TYPE_CREATURE) != 0)
  {
    play_sound_effect(WAV_DESTROY);
  }

  for (exile_index = 0; exile_index < 500; exile_index++)
  {
    if (global_exile[player][exile_index] == -1)
    {
      global_exile[player][exile_index] = internal_card_id;
      return;
    }
  }
}

// FUNCTION: MAGIC 0x004b0c12
// FUNCTION: SHANDALAR 0x0046dbed
int process_killed_card(int player, int card)
{
  int saved_trigger_cause_controller;
  int saved_trigger_cause;
  int kill_mode;
  int internal_card_id;

  internal_card_id = PLAYER_CARD_INSTANCE(player, card).internal_card_id;
  kill_mode = (int)(char)PLAYER_CARD_INSTANCE(player, card).kill_code;
  if (kill_mode == 0)
  {
    return 0;
  }

  if ((PLAYER_CARD_INSTANCE(player, card).token_status & 8) != 0)
  {
  }
  else
  {
    if (kill_mode != 4 &&
        (((unsigned char)global_cards_data[internal_card_id].type & TYPE_CREATURE) != 0) &&
        (PLAYER_CARD_INSTANCE(player, card).state & 0x20) == 0)
    {
      ++g_duel_summary.creatures_died;
    }

    if (((unsigned char)global_cards_data[internal_card_id].type & 0xc7) != 0)
    {
      push_affected_card_stack();
      g_event_result = 0;
      g_affected_card_controller = player;
      g_affected_card = card;
      g_attacking_card_controller = 1 - player;
      g_attacking_card = -1;
      C_dispatch_event_raw(0x77);
      if (g_event_result > 0)
      {
        PLAYER_CARD_INSTANCE(player, card).token_status &= ~STATUS_DYING;
        pop_affected_card_stack();
        return 0;
      }

      kill_mode = (int)(char)PLAYER_CARD_INSTANCE(player, card).kill_code;
      pop_affected_card_stack();
    }

    if ((unsigned char)global_cards_data[internal_card_id].type & 0x80)
    {
    }
    else if ((PLAYER_CARD_INSTANCE(player, card).token_status & STATUS_TOKEN) == 0)
    {
      if (kill_mode != 4)
      {
        if (kill_mode != 3 && g_duel_ai_mode_state != 1)
        {
          if (((unsigned char)global_cards_data[PLAYER_CARD_INSTANCE(player, card).original_internal_card_id].type & TYPE_CREATURE) != 0)
          {
            play_sound_effect(WAV_KILL);
          }
          else if (((unsigned char)global_cards_data[PLAYER_CARD_INSTANCE(player, card).original_internal_card_id].type & TYPE_SPELL) == 0)
          {
            play_sound_effect(WAV_BURIED);
          }
        }

        move_card_to_graveyard(player, card);
        if (kill_mode == 3)
        {
          dispatch_trigger_twice_once_with_each_player_as_reason(g_current_player, TRIGGER_GRAVEYARD_FROM_PLAY, gs_cards_to_graveyard_008a8ed0, 0);
        }
      }
      else
      {
        move_card_to_exile((PLAYER_CARD_INSTANCE(player, card).state & STATE_OWNED_BY_OPPONENT) >> 12,
                           PLAYER_CARD_INSTANCE(player, card).original_internal_card_id);
      }
    }
  }

  push_affected_card_stack();
  saved_trigger_cause_controller = g_trigger_cause_controller;
  saved_trigger_cause = g_trigger_cause;
  g_trigger_cause_controller = player;
  g_trigger_cause = card;
  if (((unsigned int)(unsigned char)global_cards_data[internal_card_id].type & 0x1047) != 0)
  {
    dispatch_trigger_twice_once_with_each_player_as_reason(g_current_player, TRIGGER_LEAVE_PLAY, gs_card_leaving_play_007aaef0, 0);
  }
  g_trigger_cause_controller = saved_trigger_cause_controller;
  g_trigger_cause = saved_trigger_cause;
  pop_affected_card_stack();

  if (((unsigned char)global_cards_data[internal_card_id].type & TYPE_CREATURE) != 0)
  {
    --g_duel_summary.creature_counts[player];
  }
  if (((unsigned char)global_cards_data[internal_card_id].type & TYPE_ARTIFACT) != 0)
  {
    --g_duel_summary.artifact_counts[player];
  }
  if (((unsigned char)global_cards_data[internal_card_id].type & TYPE_ENCHANTMENT) != 0)
  {
    --g_duel_summary.enchantment_counts[player];
  }

  PLAYER_CARD_INSTANCE(player, card).internal_card_id = -1;
  PLAYER_CARD_INSTANCE(player, card).kill_code = 0;
  PLAYER_CARD_INSTANCE(player, card).unknown0x14 = 0;
  if (g_duel_ai_mode_state != 1)
  {
    request_duel_display_refresh_if_human(player, card, 7, 2);
  }

  clear_card_attachments(player, card);
  if (((unsigned int)(unsigned char)global_cards_data[internal_card_id].type & 0x1047) != 0)
  {
    reassess_all_cards_and_mana();
  }

  return 0;
}

// FUNCTION: MAGIC 0x004b08e6
// FUNCTION: SHANDALAR 0x0046d8c1
void kill_card(int player, int card, kill_t kill_mode)
{
  int internal_card_id;

  if (player == -1 || card == -1)
  {
    return;
  }

  if ((PLAYER_CARD_INSTANCE(player, card).token_status & 0x80) != 0)
  {
    return;
  }

  PLAYER_CARD_INSTANCE(player, card).token_status |= 0x80;
  internal_card_id = PLAYER_CARD_INSTANCE(player, card).internal_card_id;
  if (internal_card_id == -1)
  {
    return;
  }

  if ((PLAYER_CARD_INSTANCE(player, card).state & 2) == 0)
  {
    kill_mode = 3;
  }

  if ((PLAYER_CARD_INSTANCE(player, card).token_status & 8) != 0 || kill_mode == 3 || kill_mode == 4 ||
      (global_cards_data[internal_card_id].type & 0x43) == 0 || global_cards_data[internal_card_id].type == 0x80)
  {
    PLAYER_CARD_INSTANCE(player, card).kill_code = kill_mode;
    process_killed_card(player, card);
  }
  else
  {
    PLAYER_CARD_INSTANCE(player, card).kill_code = kill_mode;
    PLAYER_CARD_INSTANCE(player, card).state |= 2;
    PLAYER_CARD_INSTANCE(player, card).unknown0x14 = 0xd6;
    pending_killed_card_handler = process_killed_card;
  }
}

// FUNCTION: MAGIC 0x004b117e
// FUNCTION: SHANDALAR 0x0046e159
void clear_card_attachments(int player, int card)
{
  int current_card;   /* ebp - 0x970 */
  int linked_count;   /* ebp - 0x96c */
  int linked_type;    /* ebp - 0x968 */
  int current_player; /* ebp - 0x964 */
  int linked_cards[600];

  linked_count = 0;
  for (current_player = 0; current_player < 2; ++current_player)
  {
    for (current_card = 0; current_card < g_active_cards_count[current_player]; ++current_card)
    {
      if (is_in_play(current_player, current_card) &&
          PLAYER_CARD_INSTANCE(current_player, current_card).damage_target_player == player &&
          PLAYER_CARD_INSTANCE(current_player, current_card).damage_target_card == card &&
          (current_player != player || current_card != card))
      {
        linked_type = global_cards_data[PLAYER_CARD_INSTANCE(current_player, current_card).internal_card_id].type;
        if ((linked_type & 0x43) != 0)
        {
          PLAYER_CARD_INSTANCE(current_player, current_card).damage_target_player = -1;
          PLAYER_CARD_INSTANCE(current_player, current_card).damage_target_card = -1;
        }
        else
        {
          linked_cards[linked_count * 2] = current_player;
          linked_cards[linked_count * 2 + 1] = current_card;
          ++linked_count;
        }
      }
    }
  }

  while (linked_count != 0)
  {
    --linked_count;
    kill_card(linked_cards[linked_count * 2], linked_cards[linked_count * 2 + 1], 2);
  }

  PLAYER_CARD_INSTANCE(player, card).damage_on_card = 0;
  PLAYER_CARD_INSTANCE(player, card).counter_toughness = PLAYER_CARD_INSTANCE(player, card).damage_on_card;
  PLAYER_CARD_INSTANCE(player, card).counter_power = PLAYER_CARD_INSTANCE(player, card).counter_toughness;
  PLAYER_CARD_INSTANCE(player, card).blocking = 0xff;
}

// FUNCTION: MAGIC 0x0043e668
// FUNCTION: SHANDALAR 0x00409ee8
void discard(int player, int flags, int player_who_controls_effect)
{
  int tries;         // ebp - 0x4cc
  int selected_card; // ebp - 0x4c8
  target_t target;
  char prompt[600];
  char prompt_line_2[300];
  int dialog_result;
  char prompt_line_1[300];
  int current_card;
  unsigned int found;

  tries = 0;

  if (g_duel_summary.hand_counts[player] <= 0)
    return;

  if ((g_other_player == player && (g_duel_network_flags & 2) == 0 && (g_duel_summary.hand_counts[player] + g_hand_count_adjustment) <= 0))
    return;

  if (((g_active_player == player || (g_duel_network_flags & 2) != 0) && g_duel_ai_mode_state != 1) && flags == 0)
  {
    load_text(global_ui_strings_filename, "PROMPT_DISCARDACARD");
    C_real_select_target(player,
                         player,
                         player,
                         TARGET_ZONE_HAND,
                         TYPE_NONE,
                         TYPE_NONE,
                         0,
                         0,
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
                         0,
                         &target);
    selected_card = target.card;
  }
  else
  {
    found = 0;
    if ((g_duel_network_flags & 2) != 0 && g_other_player == player)
    {
      TENTATIVE_wait_for_network_result(player, 0x14);
      selected_card = g_network_result_packet.result;
      if (selected_card == -1)
        found = 0;
      else
        found = 1;
    }
    else
    {
      tries = 0;
      do
      {
        selected_card = internal_rand(g_active_cards_count[player]);
        if (PLAYER_CARD_INSTANCE(player, selected_card).internal_card_id != -1 && (PLAYER_CARD_INSTANCE(player, selected_card).state & 2) == 0 && (PLAYER_CARD_INSTANCE(player, selected_card).state & 0x20) == 0)
        {
          found = 1;
        }
      } while (found == 0 && ++tries < 999);

      if (found == 0)
      {
        for (current_card = 0; current_card < g_active_cards_count[player] && found == 0; current_card++)
        {
          if (PLAYER_CARD_INSTANCE(player, current_card).internal_card_id != -1 && (PLAYER_CARD_INSTANCE(player, current_card).state & 2) == 0 && (PLAYER_CARD_INSTANCE(player, current_card).state & 0x20) == 0)
          {
            found = 1;
            selected_card = current_card;
          }
        }
      }

      if ((g_duel_network_flags & 2) != 0 && g_active_player == player)
      {
        if (found == 0)
        {
          selected_card = -1;
        }
        g_network_result_packet.result = selected_card;
        g_network_result_packet.packet_type = 0x14;
        TENTATIVE_send_network_result(player, 0x14);
      }
    }
  }

  if ((g_player_special_effect_flags[player] & 2) == 0 || player_who_controls_effect != 0)
  {
    if (g_other_player == player && (g_duel_network_flags & 2) == 0 && g_duel_ai_mode_state != 1)
    {
      if (flags)
      {
        load_text(global_ui_strings_filename, "PROMPT_DISCARDACARD");
        do_dialog(player, player, selected_card, -1, -1, g_text_lines[1], 0);
      }
      else
      {
        load_text(global_ui_strings_filename, "PROMPT_DISCARDACARD");
        do_dialog(player, player, selected_card, -1, -1, g_text_lines[2], 0);
      }
    }
    discard_card_from_hand(player, selected_card);
  }
  else
  {
    load_text(global_ui_strings_filename, "PROMPT_DISCARDACARD");
    strcpy(prompt_line_1, g_text_lines[3]);
    strcpy(prompt_line_2, g_text_lines[4]);
    sprintf(prompt, " %s\n %s", prompt_line_1, prompt_line_2);
    dialog_result = do_dialog(player, player, selected_card, -1, -1, prompt, 0);
    if (dialog_result == 0)
    {
      real_put_on_top_of_deck(player, PLAYER_CARD_INSTANCE(player, selected_card).internal_card_id);
      PLAYER_CARD_INSTANCE(player, selected_card).internal_card_id = -1;
      request_duel_display_refresh_if_human(player, selected_card, 10, 1);
      if (g_duel_ai_mode_state != 1)
      {
        play_sound_effect(WAV_DISCARD);
      }
      --g_duel_summary.hand_counts[player];
    }
    else
    {
      discard_card_from_hand(player, selected_card);
    }
  }
}

// FUNCTION: MAGIC 0x004eae35
// FUNCTION: SHANDALAR 0x004415c5
int produce_mana(int player, color_t color, int amount)
{
  g_raw_mana_available[player][color] += amount;
  g_raw_mana_available[player][7] += amount;
  return g_raw_mana_available[player][color];
}

int select_target(int player, int card, target_definition_t *td, const char *prompt, target_t *ret_location)
{
  (void)player;
  (void)card;
  (void)td;
  (void)prompt;
  if (ret_location != NULL)
  {
    ret_location->player = -1;
    ret_location->card = -1;
  }
  return 0;
}

// FUNCTION: MAGIC 0x004c0a36
// FUNCTION: SHANDALAR 0x004ecec6
int ClampIntToRange(int a1, int a2, int a3)
{
  if (a1 < a2)
  {
    a1 = a2;
  }
  if (a3 < a1)
  {
    a1 = a3;
  }
  return a1;
}

// FUNCTION: MOK 0x0042de60
// FUNCTION: MAGIC 0x00432f00
// FUNCTION: SHANDALAR 0x004143e0
int charge_mana(int player, color_t color, int amount)
{
  int idx8;               /* [ebp-0xd4] */
  int saved_raw_mana[8];  /* [ebp-0xd0] */
  int saved_x_value;      /* [ebp-0xb0] */
  int saved_mana_cost[8]; /* [ebp-0xac] */
  int saved_max_x_value;  /* [ebp-0x8c] */

  int special_amount_to_take;           /* [ebp-0x88] */
  int chosen_color;                     /* [ebp-0x84] */
  unsigned int special_color;           /* [ebp-0x80] */
  unsigned int produced_color;          /* [ebp-0x7c] */
  color_test_t payable_color_mask;      /* [ebp-0x78] */
  color_test_t produced_special_colors; /* [ebp-0x74] */
  int color_count;                      /* [ebp-0x70] */
  int amount_to_take;                   /* [ebp-0x6c] */
  int cost_color;                       /* [ebp-0x68] */
  int only_variable_costs;              /* [ebp-0x64] */
  int available_mana;                   /* [ebp-0x60] */
  int old_tapped_state;                 /* [ebp-0x5c] */
  int done;                             /* [ebp-0x58] */
  color_test_t payable_colors;          /* [ebp-0x54] */

  int allow_special_x_payment;                     /* [ebp-0x50] */
  int mana_paid_by_color[7];                       /* [ebp-0x4c] */
  int allow_generic_payment;                       /* [ebp-0x30] */
  int selected_card;                               /* [ebp-0x2c] */
  int allow_mana_sources;                          /* [ebp-0x28] */
  int allow_interrupt_speed_mana_sources;          /* [ebp-0x24] */
  int max_colorless;                               /* [ebp-0x20] */
  int slot;                                        /* [ebp-0x1c] */
  int i;                                           /* [ebp-0x18] */
  unsigned int old_tapped_state_before_activation; /* [ebp-0x14] */
  int current_internal_card_id;                    /* [ebp-0x10] */
  int mana_paid_total;                             /* [ebp-0x0c] */
  int temp;                                        /* [ebp-0x08] */
  int include_auto_payment;                        /* [ebp-0x04] */

  if ((g_land_can_be_played & LCBP_CARD_BEING_COPIED) != 0)
  {
    goto skipped_for_copied_card;
  }

  {
    if (g_mana_charge[MANA_CHARGE_POWER_ARTIFACT_REDUCTION] > 0)
    {
      if (color == COLOR_COLORLESS)
      {
        if (amount != -1)
        {
          amount = MAX(amount - g_mana_charge[MANA_CHARGE_POWER_ARTIFACT_REDUCTION] * 2, 1);
        }
      }
      else if (g_mana_charge[COLOR_COLORLESS] > 0)
      {
        g_mana_charge[COLOR_COLORLESS] = MAX(g_mana_charge[COLOR_COLORLESS] - g_mana_charge[MANA_CHARGE_POWER_ARTIFACT_REDUCTION] * 2, 1);
      }
    }

    g_mana_charge[color] += amount;
    mana_paid_total = 0;
    for (i = 0; i < 7; ++i)
    {
      mana_paid_by_color[i] = 0;
    }

    max_colorless = has_mana(player, COLOR_ARTIFACT, 1) - has_mana(player, COLOR_ANY, 1);

    include_auto_payment = 1;
    if (g_recorded_action_player != 0 || (g_other_player == player && (g_duel_network_flags & 2) == 0) || g_duel_ai_mode_state == 1 || g_duel_network_state != 0)
    {
      allow_special_x_payment = 1;
      allow_generic_payment = 1;
      allow_mana_sources = 1;
      if ((g_other_player == player && (g_duel_network_flags & 2) == 0) || g_duel_ai_mode_state == 1 || g_duel_network_state != 0)
      {
        allow_interrupt_speed_mana_sources = 1;
        temp = 1;
      }
      else
      {
        temp = 0;
        allow_interrupt_speed_mana_sources = temp;
      }
    }
    else
    {
      temp = 0;
      allow_interrupt_speed_mana_sources = temp;
      allow_mana_sources = allow_interrupt_speed_mana_sources;
      allow_generic_payment = allow_mana_sources;
      allow_special_x_payment = allow_generic_payment;
    }

    temp = 1;

    if (g_other_player == player && (g_duel_network_flags & 2) == 0)
    {
      for (i = COLOR_COLORLESS; i < 7; ++i)
      {
        if (g_mana_charge[i] == -1)
        {
          available_mana = has_mana(player, i, 1);
          if (g_duel_ai_mode_state == 1)
          {
            if (internal_rand(3) == 0 || available_mana <= 1)
            {
              old_tapped_state = available_mana;
            }
            else
            {
              old_tapped_state = internal_rand(available_mana - 1) + 1;
            }

            if (g_max_x_value != -1)
            {
              old_tapped_state = MIN(g_max_x_value, old_tapped_state);
            }

            g_ai_recorded_choice = old_tapped_state;
            record_ai_action_selection();
          }
          else
          {
            replay_ai_action_selection();
            if (g_ai_recorded_choice == 99)
            {
              g_ai_recorded_choice = 0;
            }
            old_tapped_state = g_ai_recorded_choice;
          }
        }
      }

      g_max_x_value = old_tapped_state;
    }

    if (g_mana_charge[MANA_CHARGE_POWER_ARTIFACT_REDUCTION] > 0)
    {
      g_x_value = g_mana_charge[MANA_CHARGE_POWER_ARTIFACT_REDUCTION] * 2;
    }
    else
    {
      g_x_value = 0;
    }

    if (g_max_x_value == 0)
    {
      for (i = 0; i < 7; ++i)
      {
        if (g_mana_charge[i] == -1)
        {
          g_mana_charge[i] = 0;
        }
      }
    }

    for (i = 0, payable_colors = 0; i < 7; ++i)
    {
      if (g_mana_charge[i] == -1)
      {
        payable_colors = 1;
      }
    }

    if (include_auto_payment != 0 && is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0)
    {
      auto_pay_colored_mana(player, mana_paid_by_color, &mana_paid_total, max_colorless);
      copy_mana_pool_to_display();
    }

    if (allow_special_x_payment != 0 && is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0 && payable_colors != 0)
    {
      auto_pay_variable_mana(player, mana_paid_by_color, &mana_paid_total, max_colorless, &g_x_value, g_max_x_value);
      copy_mana_pool_to_display();
    }

    if (allow_generic_payment != 0 && is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0)
    {
      auto_pay_generic_mana(player, mana_paid_by_color, &mana_paid_total, max_colorless, &g_x_value, g_max_x_value);
      copy_mana_pool_to_display();
    }

    if (allow_mana_sources != 0 && is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0)
    {
      if (is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0)
      {
        activate_mana_sources_for_payment(player, mana_paid_by_color, &mana_paid_total, 0x1e, temp);
      }
      if (is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0)
      {
        activate_mana_sources_for_payment(player, mana_paid_by_color, &mana_paid_total, 0x1c, temp);
      }
    }

    if (allow_interrupt_speed_mana_sources != 0 && is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0)
    {
      if (is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0)
      {
        activate_mana_sources_for_payment(player, mana_paid_by_color, &mana_paid_total, 0x14, temp);
      }
      if (is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0)
      {
        activate_mana_sources_for_payment(player, mana_paid_by_color, &mana_paid_total, 0x04, temp);
      }
      if (is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0)
      {
        activate_mana_sources_for_payment(player, mana_paid_by_color, &mana_paid_total, 0x1a, temp);
      }
      if (is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0)
      {
        activate_mana_sources_for_payment(player, mana_paid_by_color, &mana_paid_total, 0x18, temp);
      }
      if (is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0)
      {
        activate_mana_sources_for_payment(player, mana_paid_by_color, &mana_paid_total, 0x10, temp);
      }
      if (is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0)
      {
        activate_mana_sources_for_payment(player, mana_paid_by_color, &mana_paid_total, 0x00, temp);
      }
    }

    if ((g_other_player == player && (g_duel_network_flags & 2) == 0) || g_duel_ai_mode_state == 1 || g_duel_network_state != 0)
    {
      if (is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0 && payable_colors == 0)
      {
        g_spell_fizzled = 1;
      }
    }
    else if (is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0)
    {
      done = 0;
      while (done == 0 && is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0)
      {
        only_variable_costs = 1;
        for (i = 0; i < 7; ++i)
        {
          if (g_mana_charge[i] > 0)
          {
            only_variable_costs = 0;
          }
        }

        format_mana_payment_prompt(g_ui_message_buffer, g_mana_charge, g_x_value, g_max_x_value);
        selected_card =
            select_card_for_action(player, player, player, 0, 0, g_ui_message_buffer, only_variable_costs != 0 ? 3 : 1);

        if (g_target_player_choice == -1 && (selected_card == -1 || selected_card == -2))
        {
          if (g_target_selection_status_code == -2)
          {
            if (g_stop_phase_player == -1 && g_stop_phase == -1)
            {
              if (selected_card == -1)
              {
                g_spell_fizzled = 1;
              }
              done = 1;
            }
            else if (only_variable_costs != 0)
            {
              done = 1;
            }
          }
          else if (g_target_selection_status_code == -3 && player == g_recorded_action_controller && g_recorded_action_phase != -1)
          {
            if (g_raw_mana_available[player][g_recorded_action_phase] > 0 && (g_mana_charge[g_recorded_action_phase] != 0 || g_mana_charge[COLOR_COLORLESS] != 0 || g_mana_charge[COLOR_ARTIFACT] != 0) && (g_recorded_action_phase != COLOR_ARTIFACT || g_mana_charge[COLOR_ARTIFACT] != 0))
            {
              if (g_mana_charge[g_recorded_action_phase] == 0)
              {
                if (g_mana_charge[COLOR_ARTIFACT] == 0)
                {
                  cost_color = COLOR_COLORLESS;
                }
                else
                {
                  cost_color = COLOR_ARTIFACT;
                }
              }
              else
              {
                cost_color = g_recorded_action_phase;
              }

              amount_to_take =
                  get_mana_payment_amount(g_mana_charge, cost_color, g_raw_mana_available[player], g_recorded_action_phase, g_recorded_action_player, g_max_x_value, g_x_value);
              spend_mana_for_cost(
                  g_mana_charge, cost_color, amount_to_take, &g_x_value, g_max_x_value, player, g_recorded_action_phase, mana_paid_by_color, &mana_paid_total);
              copy_mana_pool_to_display();
            }

            if (g_raw_mana_available[player][g_recorded_action_phase] > 0)
            {
              produced_special_colors = 0;
              for (slot = 0; slot < 10 && g_mana_color_conversions[player][slot] != -1; ++slot)
              {
                produced_color = (unsigned short)g_mana_color_conversions[player][slot];
                special_color = ((unsigned int)g_mana_color_conversions[player][slot] >> 16) & 0xffff;
                if (produced_color == g_recorded_action_phase)
                {
                  produced_special_colors |= 1 << (unsigned char)special_color;
                }
              }

              payable_color_mask = 0;
              for (i = 0; i < 7; ++i)
              {
                if (g_mana_charge[i] != 0)
                {
                  payable_color_mask |= 1 << (unsigned char)i;
                }
              }

              produced_special_colors &= payable_color_mask;
              if (produced_special_colors != 0)
              {
                color_count = 0;
                for (i = 0; i < 7; ++i)
                {
                  if ((produced_special_colors & (1 << (unsigned char)i)) != 0)
                  {
                    ++color_count;
                  }
                }

                if (color_count == 1)
                {
                  chosen_color = single_color_test_bit_to_color_t(produced_special_colors);
                }
                else
                {
                  load_text(global_ui_strings_filename, "PROMPT_GRABMANA_DUALUSE");
                  chosen_color = choose_a_color(player, g_text_lines[0], 1, g_recorded_action_phase, produced_special_colors);
                }

                special_amount_to_take =
                    get_mana_payment_amount(g_mana_charge, chosen_color, g_raw_mana_available[player], g_recorded_action_phase, g_recorded_action_player, g_max_x_value, g_x_value);
                spend_mana_for_cost(
                    g_mana_charge, chosen_color, special_amount_to_take, &g_x_value, g_max_x_value, player, g_recorded_action_phase, mana_paid_by_color, &mana_paid_total);
                copy_mana_pool_to_display();
              }
            }
          }
        }
        else
        {
          if (g_target_player_choice == -1 || selected_card != -1)
          {
            current_internal_card_id = (global_card_instances[player] + selected_card)->internal_card_id;
            if ((global_cards_data[current_internal_card_id].extra_ability & EA_MANA_SOURCE) != 0 &&
                ((global_cards_data[current_internal_card_id].type & TYPE_INTERRUPT) != 0 ||
                 ((((global_card_instances[player] + selected_card)->state & STATE_IN_PLAY) != 0) &&
                  ((global_card_instances[player] + selected_card)->state & STATE_OUBLIETTED) == 0 &&
                  ((global_card_instances[player] + selected_card)->state & STATE_TAPPED) == 0 &&
                  ((((global_card_instances[player] + selected_card)->state &
                     (STATE_SUMMONSICK_NOATTACK | STATE_SUMMONSICK_NOTAP)) == 0) ||
                   (global_cards_data[(global_card_instances[player] + selected_card)->internal_card_id].type & TYPE_CREATURE) == 0))))
            {
              for (idx8 = 0; idx8 <= 7; ++idx8)
              {
                saved_raw_mana[idx8] = g_raw_mana_available[player][idx8];
              }

              saved_x_value = g_x_value;
              g_x_value = 0;
              saved_max_x_value = g_max_x_value;
              g_max_x_value = -1;
              for (idx8 = 0; idx8 < 7; ++idx8)
              {
                saved_mana_cost[idx8] = g_mana_charge[idx8];
                g_mana_charge[idx8] = 0;
              }

              if (((global_card_instances[player] + selected_card)->state & 0x800002) == STATE_IN_PLAY)
              {
                if ((global_cards_data[current_internal_card_id].type & TYPE_LAND) != 0 ||
                    dispatch_event_to_single_card(player, selected_card, EVENT_CAN_ACTIVATE, 1 - player, -1) != 0)
                {
                  push_card_onto_stack(player, selected_card, EVENT_RESOLVE_ACTIVATION, player, 0);
                  g_required_mana_color_mask = get_required_mana_color_mask(saved_mana_cost);
                  g_produced_mana_color = -1;
                  old_tapped_state_before_activation = (global_card_instances[player] + selected_card)->state & STATE_TAPPED;
                  dispatch_event_to_single_card(player, selected_card, EVENT_ACTIVATE, 1 - player, -1);
                  g_required_mana_color_mask = 0;
                  if (g_spell_fizzled == 1)
                  {
                    g_spell_fizzled = 0;
                    obliterate_top_card_of_stack();
                  }
                  else
                  {
                    if (old_tapped_state_before_activation == 0 && ((global_card_instances[player] + selected_card)->state & STATE_TAPPED) != 0)
                    {
                      dispatch_event(player, selected_card, EVENT_TAP_CARD);
                    }
                    if (g_duel_ai_mode_state != 1)
                    {
                      play_sound_effect(WAV_TAP);
                    }
                    resolve_top_card_on_stack();
                    TENTATIVE_reassess_all_cards(0, 0xff);
                  }
                }
              }
              else
              {
                resolve_cast_card(player, selected_card);
                TENTATIVE_reassess_all_cards(0, 0xff);
              }

              g_x_value = saved_x_value;
              g_max_x_value = saved_max_x_value;
              for (idx8 = 0; idx8 < 7; ++idx8)
              {
                g_mana_charge[idx8] = saved_mana_cost[idx8];
              }

              for (idx8 = 0; idx8 <= 7; ++idx8)
              {
                g_raw_mana_available[player][idx8] -= saved_raw_mana[idx8];
              }

              max_colorless = 0;
              auto_pay_colored_mana(player, mana_paid_by_color, &mana_paid_total, max_colorless);
              auto_pay_variable_mana(player, mana_paid_by_color, &mana_paid_total, max_colorless, &g_x_value, g_max_x_value);
              auto_pay_generic_mana(player, mana_paid_by_color, &mana_paid_total, max_colorless, &g_x_value, g_max_x_value);

              for (idx8 = 0; idx8 <= 7; ++idx8)
              {
                g_raw_mana_available[player][idx8] += saved_raw_mana[idx8];
              }

              copy_mana_pool_to_display();
            }
          }
        }
      }
    }
  }

skipped_for_copied_card:
  if (g_spell_fizzled == 1)
  {
    refund_paid_mana(mana_paid_by_color);
    for (i = 0; i < 7; ++i)
    {
      produce_mana(player, i, mana_paid_by_color[i]);
      mana_paid_by_color[i] = 0;
    }
    mana_paid_total = 0;
    g_x_value = 0;
  }

  for (i = 0; i < 8; ++i)
  {
    g_mana_charge[i] = 0;
  }
  g_max_x_value = -1;

  return mana_paid_total;
}

// FUNCTION: MAGIC 0x004346ab
// FUNCTION: SHANDALAR 0x00415b89
int is_mana_cost_paid(int *mana_cost, int required_colorless, int max_x)
{
  int color;

  for (color = 0; color < 7; color++)
  {
    if (mana_cost[color] > 0)
    {
      return 0;
    }

    if (mana_cost[color] == -1 && max_x != -1 && required_colorless < max_x)
    {
      return 0;
    }

    if (mana_cost[color] == -1 && max_x == -1)
    {
      return 0;
    }
  }

  return 1;
}

// FUNCTION: MAGIC 0x00434746
// FUNCTION: SHANDALAR 0x00415c24
unsigned int get_required_mana_color_mask(int *mana)
{
  int color;
  unsigned int result;

  result = 0;
  for (color = 0; color < 7; ++color)
  {
    if (mana[color] != 0)
    {
      result = result | (1 << (unsigned char)color);
    }
  }

  return result;
}

// FUNCTION: MAGIC 0x0043479e
// FUNCTION: SHANDALAR 0x00415c7c
int format_mana_payment_prompt(char *prompt, int *mana_cost, int x_paid, int max_x)
{
  char symbol[8];               /* [ebp-0x1b8] */
  int total_generic;            /* [ebp-0x1b0] */
  char tmp[300];                /* [ebp-0x1ac] */
  char mana_text[100];          /* [ebp-0x80] */
  int stack_card;               /* [ebp-0x1c] */
  size_t unused_layout_padding; /* [ebp-0x18] (intentionally unused, but stabilizes layout) */
  unsigned int action;          /* [ebp-0x14] */
  int i;                        /* [ebp-0x10] */
  int color;                    /* [ebp-0x0c] */
  unsigned int msg;             /* [ebp-0x08] */
  int stack_player;             /* [ebp-0x04] */

  (void)prompt;

  strcpy(tmp, "");

  action = get_current_stack_action();
  if (action != 0xffffffffU)
  {
    msg = (action >> 16) & 0xff;
    stack_player = global_stack_cards[g_stack_size - 1].player;
    stack_card = global_stack_cards[g_stack_size - 1].card;
    format_stack_action_text(tmp, msg, stack_player, stack_card);
  }

  mana_text[0] = '\0';

  if (mana_cost[COLOR_COLORLESS] == -1 || mana_cost[COLOR_ARTIFACT] == -1)
  {
    if (max_x == -1)
    {
      sprintf(mana_text + strlen(mana_text), gs_mana_so_far_008a9010, "|X", x_paid);
    }
    else if (x_paid < max_x)
    {
      sprintf(mana_text + strlen(mana_text), gs_mana_so_far_max_00791420, "|X", x_paid, max_x);
    }
  }
  else if (mana_cost[COLOR_COLORLESS] != 0 || mana_cost[COLOR_ARTIFACT] != 0)
  {
    total_generic = mana_cost[COLOR_ARTIFACT] + mana_cost[COLOR_COLORLESS];
    for (; total_generic > 9; total_generic -= 10)
    {
      strcat(mana_text, "|10");
    }
    if (total_generic != 0)
    {
      sprintf(mana_text + strlen(mana_text), "|%d", total_generic);
    }
  }

  for (color = COLOR_BLACK; color < 6; color++)
  {
    symbol[0] = '|';
    if (color == COLOR_BLACK)
    {
      symbol[1] = 'B';
    }
    else if (color == COLOR_BLUE)
    {
      symbol[1] = 'U';
    }
    else if (color == COLOR_GREEN)
    {
      symbol[1] = 'G';
    }
    else if (color == COLOR_RED)
    {
      symbol[1] = 'R';
    }
    else
    {
      symbol[1] = 'W';
    }
    symbol[2] = '\0';

    if (mana_cost[color] == -1)
    {
      if (max_x == -1)
      {
        sprintf(mana_text + strlen(mana_text), gs_mana_so_far_008a9010, symbol, x_paid);
      }
      else if (x_paid < max_x)
      {
        sprintf(mana_text + strlen(mana_text), gs_mana_so_far_max_00791420, symbol, x_paid, max_x);
      }
    }
    else if (mana_cost[color] != 0)
    {
      for (i = 0; i < mana_cost[color]; i++)
      {
        strcat(mana_text, symbol);
      }
    }
  }

  strcpy(g_ui_message_buffer, tmp);
  strcat(g_ui_message_buffer, ", ");
  sprintf(tmp, gs_tap_for_mana_0091c230, mana_text);
  strcat(g_ui_message_buffer, tmp);

  return 0;
}

// FUNCTION: MAGIC 0x00435c91
// FUNCTION: SHANDALAR 0x0041716b
int record_paid_mana(int color, int amount)
{
  if (0 < g_mana_payment_record_depth)
  {
    g_recorded_mana_payments[g_mana_payment_record_depth - 1][color] += amount;
    return 1;
  }
  else
    return 0;
}

// FUNCTION: MAGIC 0x004eae77
// FUNCTION: SHANDALAR 0x00441607
int unproduce_mana(int player, int color, int amount)
{
  g_raw_mana_available[player][color] -= amount;
  g_raw_mana_available[player][7] -= amount;
  return g_raw_mana_available[player][color];
}

// FUNCTION: MAGIC 0x00435b18
// FUNCTION: SHANDALAR 0x00416ff1
void spend_mana_for_cost(int *mana_cost,
                         int cost_color,
                         int amount,
                         int *special_mana,
                         int max_x,
                         int player,
                         int mana_color,
                         int *mana_paid_by_color,
                         int *total_mana_paid)
{
  (void)max_x;

  unproduce_mana(player, mana_color, amount);
  if (mana_cost[cost_color] == -1)
  {
    *special_mana += amount;
  }
  else
  {
    mana_cost[cost_color] -= amount;
  }
  mana_paid_by_color[mana_color] += amount;
  record_paid_mana(mana_color, amount);
  *total_mana_paid += amount;
}

// FUNCTION: MAGIC 0x0043410a
// FUNCTION: SHANDALAR 0x004155ee
void auto_pay_colored_mana(int player, int *mana_paid_by_color, int *total_mana_paid, int max_colorless)
{
  unsigned short produced_color;
  unsigned int special_mana;
  int slot;
  int color;

  for (color = 0; color < 7; ++color)
  {
    while (g_mana_charge[color] > 0 && g_raw_mana_available[player][color] > 0)
    {
      spend_mana_for_cost(g_mana_charge, color, 1, (int *)0, 0, player, color, mana_paid_by_color, total_mana_paid);
    }
  }

  while (g_mana_charge[6] > 0 && g_raw_mana_available[player][0] > 0 && max_colorless < g_mana_charge[6])
  {
    spend_mana_for_cost(g_mana_charge, 6, 1, (int *)0, 0, player, 0, mana_paid_by_color, total_mana_paid);
  }

  slot = 0;
  while (slot < 10 && g_mana_color_conversions[player][slot] != -1)
  {
    produced_color = (unsigned short)g_mana_color_conversions[player][slot];
    special_mana = (unsigned int)g_mana_color_conversions[player][slot];
    while (g_raw_mana_available[player][produced_color] > 0 && g_mana_charge[special_mana >> 16] > 0)
    {
      spend_mana_for_cost(
          g_mana_charge, special_mana >> 16, 1, (int *)0, 0, player, produced_color, mana_paid_by_color, total_mana_paid);
    }
    slot = slot + 1;
  }
}

// FUNCTION: MAGIC 0x004342b3
// FUNCTION: SHANDALAR 0x00415794
void auto_pay_variable_mana(int player,
                            int *mana_paid_by_color,
                            int *total_mana_paid,
                            int max_colorless,
                            int *special_mana_override,
                            int max_x)
{
  unsigned short produced_color;
  unsigned int special_mana;
  int slot;
  int color;

  for (color = 0; color < 7; ++color)
  {
    if (g_mana_charge[color] == -1)
    {
      while (g_raw_mana_available[player][color] > 0 && (g_x_value < max_x || max_x == -1))
      {
        spend_mana_for_cost(
            g_mana_charge, color, 1, special_mana_override, max_x, player, color, mana_paid_by_color, total_mana_paid);
      }
    }
  }

  if (g_mana_charge[6] == -1)
  {
    while (g_raw_mana_available[player][0] > 0 && max_colorless < g_mana_charge[6] &&
           (g_x_value < max_x || max_x == -1))
    {
      spend_mana_for_cost(
          g_mana_charge, 6, 1, special_mana_override, max_x, player, 0, mana_paid_by_color, total_mana_paid);
    }
  }

  slot = 0;
  while (slot < 10 && g_mana_color_conversions[player][slot] != -1)
  {
    produced_color = (unsigned short)g_mana_color_conversions[player][slot];
    special_mana = (unsigned int)g_mana_color_conversions[player][slot];
    if (g_mana_charge[special_mana >> 16] == -1)
    {
      while (g_raw_mana_available[player][produced_color] > 0 && (g_x_value < max_x || max_x == -1))
      {
        spend_mana_for_cost(g_mana_charge,
                            special_mana >> 16,
                            1,
                            special_mana_override,
                            max_x,
                            player,
                            produced_color,
                            mana_paid_by_color,
                            total_mana_paid);
      }
    }
    slot = slot + 1;
  }
}

// FUNCTION: MAGIC 0x004344b0
// FUNCTION: SHANDALAR 0x0041598e
void auto_pay_generic_mana(int player,
                           int *mana_paid_by_color,
                           int *total_mana_paid,
                           int max_colorless,
                           int *special_mana_override,
                           int max_x)
{
  int color;

  if (g_mana_charge[COLOR_ARTIFACT] != 0)
  {
    for (color = 0; color < 7; ++color)
    {
      if (g_mana_charge[COLOR_ARTIFACT] > 0)
      {
        while (g_raw_mana_available[player][color] > 0 && max_colorless < g_mana_charge[COLOR_ARTIFACT])
        {
          spend_mana_for_cost(g_mana_charge, COLOR_ARTIFACT, 1, (int *)0, 0, player, color, mana_paid_by_color, total_mana_paid);
        }
      }
      else
      {
        while (g_raw_mana_available[player][color] > 0 && g_mana_charge[COLOR_ARTIFACT] == -1 &&
               (max_x == -1 || max_colorless < max_x - g_x_value))
        {
          spend_mana_for_cost(g_mana_charge,
                              COLOR_ARTIFACT,
                              1,
                              special_mana_override,
                              max_x,
                              player,
                              color,
                              mana_paid_by_color,
                              total_mana_paid);
        }
      }
    }
  }

  if (g_mana_charge[COLOR_COLORLESS] != 0)
  {
    for (color = 0; color < 7; ++color)
    {
      if (color != COLOR_ARTIFACT)
      {
        if (g_mana_charge[COLOR_COLORLESS] == -1)
        {
          while (g_raw_mana_available[player][color] > 0 && (g_x_value < max_x || max_x == -1))
          {
            spend_mana_for_cost(g_mana_charge,
                                COLOR_COLORLESS,
                                1,
                                special_mana_override,
                                max_x,
                                player,
                                color,
                                mana_paid_by_color,
                                total_mana_paid);
          }
        }
        else
        {
          while (g_mana_charge[COLOR_COLORLESS] > 0 && g_raw_mana_available[player][color] > 0)
          {
            spend_mana_for_cost(g_mana_charge,
                                COLOR_COLORLESS,
                                1,
                                (int *)0,
                                0,
                                player,
                                color,
                                mana_paid_by_color,
                                total_mana_paid);
          }
        }
      }
    }
  }
}

// FUNCTION: MAGIC 0x00435b88
// FUNCTION: SHANDALAR 0x00417061
int get_mana_payment_amount(int *mana_cost,
                            int cost_color,
                            int *mana_available,
                            int mana_color,
                            int allow_multi_pay,
                            int max_x,
                            int x_paid)
{
  int amount;

  if (allow_multi_pay != 0)
  {
    if (mana_cost[cost_color] == -1)
    {
      if (max_x == -1)
      {
        amount = mana_available[mana_color];
      }
      else
      {
        amount = mana_available[mana_color] < max_x - x_paid ? mana_available[mana_color] : max_x - x_paid;
      }
    }
    else
    {
      amount = mana_cost[cost_color] >= mana_available[mana_color] ? mana_available[mana_color] : mana_cost[cost_color];
    }
  }
  else
  {
    amount = 1;
  }

  return amount;
}

// FUNCTION: MAGIC 0x00435db0
// FUNCTION: SHANDALAR 0x0041728a
int refund_paid_mana(int *mana_paid_by_color)
{
  int color;

  if (g_mana_payment_record_depth > 0)
  {
    for (color = 0; color < 7; ++color)
    {
      g_recorded_mana_payments[g_mana_payment_record_depth - 1][color] -= mana_paid_by_color[color];
    }

    return 1;
  }
  else
  {
    return 0;
  }
}

// FUNCTION: MAGIC 0x00435643
// FUNCTION: SHANDALAR 0x00416b1c
int can_activate_mana_source(int player, int card, int activation_flags)
{
  int csvid;
  int result;
  int internal_card_id;

  internal_card_id = PLAYER_CARD_INSTANCE(player, card).internal_card_id;
  csvid = global_cards_data[internal_card_id].id;
  result = 1;

  if ((PLAYER_CARD_INSTANCE(player, card).state & 0x800002) == 2 && (global_cards_data[internal_card_id].extra_ability & 0x1000) != 0 && (global_cards_data[internal_card_id].extra_ability & 0x10000) == 0)
  {
    if ((activation_flags & 1) && (internal_card_id <= 4 || csvid == 0x366 || PLAYER_CARD_INSTANCE(player, card).mana_color == '@'))
    {
      result = 0;
    }
    if ((activation_flags & 2) && (global_cards_data[internal_card_id].type & TYPE_LAND) != 0 && internal_card_id > 4 && csvid != 0x366 && PLAYER_CARD_INSTANCE(player, card).mana_color != '@')
    {
      result = 0;
    }
    if ((activation_flags & 4) && (PLAYER_CARD_INSTANCE(player, card).state & 0x40000) != 0)
    {
      result = 0;
    }
    if ((activation_flags & 8) && (global_cards_data[internal_card_id].type & TYPE_ARTIFACT) != 0)
    {
      result = 0;
    }
    if ((activation_flags & 0x10) && (global_cards_data[internal_card_id].type & TYPE_CREATURE) != 0)
    {
      result = 0;
    }
  }
  else
  {
    result = 0;
  }

  return result;
}

// FUNCTION: MAGIC 0x00435881
// FUNCTION: SHANDALAR 0x00416d5a
int activate_mana_source_for_payment(int player, int card)
{
  int old_max_x_value;
  int result;
  int saved_mana_cost[8];
  int old_x_value;
  int color;
  unsigned int original_state;

  if ((PLAYER_CARD_INSTANCE(player, card).state & STATE_TAPPED) != 0 || (((PLAYER_CARD_INSTANCE(player, card).state & STATE_SUMMONSICK_BOTH) != 0) && (global_cards_data[PLAYER_CARD_INSTANCE(player, card).internal_card_id].type & TYPE_CREATURE) != 0))
  {
    return 0;
  }

  old_x_value = g_x_value;
  g_x_value = 0;
  old_max_x_value = g_max_x_value;
  g_max_x_value = -1;
  for (color = 0; color < 7; ++color)
  {
    saved_mana_cost[color] = g_mana_charge[color];
    g_mana_charge[color] = 0;
  }
  push_card_onto_stack(player, card, 0x72, player, 0);
  g_required_mana_color_mask = get_required_mana_color_mask(saved_mana_cost);
  g_produced_mana_color = -1;
  original_state = PLAYER_CARD_INSTANCE(player, card).state;
  dispatch_event_to_single_card(player, card, EVENT_ACTIVATE, 1 - player, -1);
  g_required_mana_color_mask = 0;
  if (g_spell_fizzled == 1)
  {
    g_spell_fizzled = 0;
    obliterate_top_card_of_stack();
    result = 0;
  }
  else
  {
    if ((original_state & STATE_TAPPED) == 0 && (PLAYER_CARD_INSTANCE(player, card).state & STATE_TAPPED) != 0)
    {
      dispatch_event(player, card, EVENT_TAP_CARD);
    }
    if (g_duel_ai_mode_state != 1)
    {
      play_sound_effect(WAV_TAP);
    }
    resolve_top_card_on_stack();
    result = 1;
  }
  g_max_x_value = old_max_x_value;
  g_x_value = old_x_value;
  for (color = 0; color < 7; ++color)
  {
    g_mana_charge[color] = saved_mana_cost[color];
  }

  return result;
}

// FUNCTION: MAGIC 0x00434af9
// FUNCTION: SHANDALAR 0x00415fd7
int activate_mana_sources_for_payment(int player,
                                      int *mana_paid_by_color,
                                      int *total_mana_paid,
                                      int activation_flags,
                                      int include_special_mana)
{
  int special_color;
  int produced_color;
  int scratch;
  int dual_mana_slot;
  int max_colorless;
  int color_scratch;
  int color;
  int can_activate;
  int current_card;
  int any_variable_costs;

  any_variable_costs = 0;
  for (color = 0; color < 7; ++color)
  {
    if (g_mana_charge[color] == -1)
    {
      any_variable_costs = 1;
    }
  }

  if (player == g_other_player && (g_duel_network_flags & 2) != 0)
  {
    current_card = 0;
    while (current_card != -1)
    {
      TENTATIVE_wait_for_network_result(player, 0x10);
      current_card = g_network_result_packet.result;
      if (current_card != -1 && activate_mana_source_for_payment(player, current_card) != 0)
      {
        max_colorless = has_mana(player, COLOR_ARTIFACT, 1) - has_mana(player, COLOR_ANY, 1);
        auto_pay_colored_mana(player, mana_paid_by_color, total_mana_paid, max_colorless);
        auto_pay_variable_mana(player, mana_paid_by_color, total_mana_paid, max_colorless, &g_x_value, g_max_x_value);
        auto_pay_generic_mana(player, mana_paid_by_color, total_mana_paid, max_colorless, &g_x_value, g_max_x_value);
      }
    }
    copy_mana_pool_to_display();
  }
  else
  {
    if (is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0)
    {
      for (current_card = 0; current_card < g_active_cards_count[player]; ++current_card)
      {
        if (can_activate_mana_source(player, current_card, activation_flags) != 0 &&
            dispatch_event_to_single_card(player, current_card, EVENT_CAN_ACTIVATE, 1 - player, -1) != 0)
        {
          can_activate = 0;
          for (color = 0; color < 7; ++color)
          {
            if (g_mana_charge[color] > 0 && (((unsigned int)(1 << (unsigned char)color) & (unsigned int)(unsigned char)PLAYER_CARD_INSTANCE(player, current_card).mana_color) != 0))
            {
              can_activate = 1;
            }
            else if (g_mana_charge[color] == -1 && g_x_value < g_max_x_value && g_max_x_value != -1 && (((unsigned int)(1 << (unsigned char)color) & (unsigned int)(unsigned char)PLAYER_CARD_INSTANCE(player, current_card).mana_color) != 0))
            {
              can_activate = 1;
            }
          }
          if (include_special_mana != 0 && !can_activate)
          {
            dual_mana_slot = 0;
            while (dual_mana_slot < 10 && g_mana_color_conversions[player][dual_mana_slot] != -1)
            {
              produced_color = (unsigned short)g_mana_color_conversions[player][dual_mana_slot];
              if (g_mana_charge[g_mana_color_conversions[player][dual_mana_slot] >> 16] > 0 && (((unsigned int)(1 << produced_color) & (unsigned int)(unsigned char)PLAYER_CARD_INSTANCE(player, current_card).mana_color) != 0))
              {
                can_activate = 1;
              }
              else if (g_mana_charge[g_mana_color_conversions[player][dual_mana_slot] >> 16] == -1 && g_x_value < g_max_x_value && g_max_x_value != -1 && (((unsigned int)(1 << produced_color) & (unsigned int)(unsigned char)PLAYER_CARD_INSTANCE(player, current_card).mana_color) != 0))
              {
                can_activate = 1;
              }
              ++dual_mana_slot;
            }
          }
          if (can_activate)
          {
            if ((g_duel_network_flags & 2) != 0)
            {
              g_network_result_packet.packet_type = 0x10;
              g_network_result_packet.result = current_card;
              TENTATIVE_send_network_result(player, 0x10);
              Sleep(0x32);
            }
            if (activate_mana_source_for_payment(player, current_card) != 0)
            {
              max_colorless = has_mana(player, COLOR_ARTIFACT, 1) - has_mana(player, COLOR_ANY, 1);
              auto_pay_colored_mana(player, mana_paid_by_color, total_mana_paid, max_colorless);
              auto_pay_variable_mana(player, mana_paid_by_color, total_mana_paid, max_colorless, &g_x_value, g_max_x_value);
              auto_pay_generic_mana(player, mana_paid_by_color, total_mana_paid, max_colorless, &g_x_value, g_max_x_value);
            }
          }
        }
      }
    }

    if (is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0 &&
        (g_mana_charge[0] != 0 || g_mana_charge[6] != 0))
    {
      for (current_card = 0; current_card < g_active_cards_count[player]; ++current_card)
      {
        if (can_activate_mana_source(player, current_card, activation_flags) != 0 &&
            dispatch_event_to_single_card(player, current_card, EVENT_CAN_ACTIVATE, 1 - player, -1) != 0)
        {
          can_activate = 0;
          if (g_mana_charge[0] < 1)
          {
            if (g_mana_charge[0] == -1 && g_x_value < g_max_x_value && g_max_x_value != -1)
            {
              can_activate = 1;
            }
            else if (g_mana_charge[6] < 1)
            {
              if (g_mana_charge[6] == -1 && g_x_value < g_max_x_value && g_max_x_value != -1)
              {
                can_activate = 1;
              }
            }
            else
            {
              can_activate = 1;
            }
          }
          else
          {
            can_activate = 1;
          }
          if (PLAYER_CARD_INSTANCE(player, current_card).mana_color == '@' && g_mana_charge[6] == 0)
          {
            can_activate = 0;
          }
          if (can_activate)
          {
            if ((g_duel_network_flags & 2) != 0)
            {
              g_network_result_packet.packet_type = 0x10;
              g_network_result_packet.result = current_card;
              TENTATIVE_send_network_result(player, 0x10);
              Sleep(0x32);
            }
            if (activate_mana_source_for_payment(player, current_card) != 0)
            {
              max_colorless = has_mana(player, COLOR_ARTIFACT, 1) - has_mana(player, COLOR_ANY, 1);
              auto_pay_colored_mana(player, mana_paid_by_color, total_mana_paid, max_colorless);
              auto_pay_variable_mana(player, mana_paid_by_color, total_mana_paid, max_colorless, &g_x_value, g_max_x_value);
              auto_pay_generic_mana(player, mana_paid_by_color, total_mana_paid, max_colorless, &g_x_value, g_max_x_value);
            }
          }
        }
      }
    }

    if (is_mana_cost_paid(g_mana_charge, g_x_value, g_max_x_value) == 0 &&
        any_variable_costs && g_max_x_value == -1)
    {
      for (current_card = 0; current_card < g_active_cards_count[player]; ++current_card)
      {
        if (can_activate_mana_source(player, current_card, activation_flags) != 0 &&
            dispatch_event_to_single_card(player, current_card, EVENT_CAN_ACTIVATE, 1 - player, -1) != 0)
        {
          can_activate = 0;
          for (color = 1; color < 7; ++color)
          {
            if (g_max_x_value == -1 && g_mana_charge[color] == -1 && (((unsigned int)(1 << (unsigned char)color) & (unsigned int)(unsigned char)PLAYER_CARD_INSTANCE(player, current_card).mana_color) != 0))
            {
              can_activate = 1;
            }
            else if (g_max_x_value == -1 && g_mana_charge[0] == -1)
            {
              can_activate = 1;
            }
            else if (g_max_x_value == -1 && g_mana_charge[6] == -1)
            {
              can_activate = 1;
            }
          }
          if (PLAYER_CARD_INSTANCE(player, current_card).mana_color == '@' && g_mana_charge[6] != -1)
          {
            can_activate = 0;
          }
          if (include_special_mana != 0 && !can_activate)
          {
            dual_mana_slot = 0;
            while (dual_mana_slot < 10 && g_mana_color_conversions[player][dual_mana_slot] != -1)
            {
              produced_color = (unsigned short)g_mana_color_conversions[player][dual_mana_slot];
              special_color = ((unsigned int)g_mana_color_conversions[player][dual_mana_slot] >> 16) & 0xffff;
              if (g_max_x_value == -1 && g_mana_charge[special_color] == -1 && (((unsigned int)(1 << produced_color) & (unsigned int)(unsigned char)PLAYER_CARD_INSTANCE(player, current_card).mana_color) != 0))
              {
                can_activate = 1;
              }
              ++dual_mana_slot;
            }
          }
          if (can_activate)
          {
            if ((g_duel_network_flags & 2) != 0)
            {
              g_network_result_packet.packet_type = 0x10;
              g_network_result_packet.result = current_card;
              TENTATIVE_send_network_result(player, 0x10);
              Sleep(0x32);
            }
            if (activate_mana_source_for_payment(player, current_card) != 0)
            {
              max_colorless = has_mana(player, COLOR_ARTIFACT, 1) - has_mana(player, COLOR_ANY, 1);
              auto_pay_colored_mana(player, mana_paid_by_color, total_mana_paid, max_colorless);
              auto_pay_variable_mana(player, mana_paid_by_color, total_mana_paid, max_colorless, &g_x_value, g_max_x_value);
              auto_pay_generic_mana(player, mana_paid_by_color, total_mana_paid, max_colorless, &g_x_value, g_max_x_value);
            }
          }
        }
      }
    }

    if ((g_duel_network_flags & 2) != 0)
    {
      g_network_result_packet.packet_type = 0x10;
      g_network_result_packet.result = -1;
      TENTATIVE_send_network_result(player, 0x10);
    }
    copy_mana_pool_to_display();
  }

  return 1;
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

        if (((PLAYER_CARD_INSTANCE(current_player, current_card).state & 0x800002) != 2) &&
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
            ((PLAYER_CARD_INSTANCE(g_target_player_choice, candidate_cards[candidate_count]).state & 2) != 0))
        {
          goto select_card_for_action_retry;
        }
        if (((global_cards_data[selected_internal_id].type & 2) != 0) &&
            ((PLAYER_CARD_INSTANCE(g_target_player_choice, candidate_cards[candidate_count]).state & 4) != 0))
        {
          goto select_card_for_action_retry;
        }

        if (g_current_phase > 0x14 && g_current_phase < 0x1e)
        {
          if ((global_cards_data[selected_internal_id].type & 2) == 0 || (PLAYER_CARD_INSTANCE(g_target_player_choice, candidate_cards[candidate_count]).state & 2) == 0)
          {
            goto select_card_for_action_retry;
          }
          goto select_card_for_action_return_candidate;
        }

        if ((global_cards_data[selected_internal_id].type & 0x4b) == 0 || (PLAYER_CARD_INSTANCE(g_target_player_choice, candidate_cards[candidate_count]).state & 2) != 0)
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

// FUNCTION: MOK 0x0049d710
// FUNCTION: MAGIC 0x004eb23d
// FUNCTION: SHANDALAR 0x004419ca
int has_mana_w_global_cost_mod(int player, int card, color_t color, int amount)
{
  int result;

  result = 0;

  if (amount == 0)
  {
    if (g_mana_cost_modifiers_by_color[single_color_test_bit_to_color_t(PLAYER_CARD_INSTANCE(player, card).color)] > 0)
    {
      return has_mana(
          player, 7, g_mana_cost_modifiers_by_color[single_color_test_bit_to_color_t(PLAYER_CARD_INSTANCE(player, card).color)]);
    }

    return 1;
  }
  else
  {
    result = has_mana(player, color, amount);
    if (result != 0)
    {
      if (g_mana_cost_modifiers_by_color[single_color_test_bit_to_color_t(PLAYER_CARD_INSTANCE(player, card).color)] > 0)
      {
        return has_mana(
            player,
            7,
            g_mana_cost_modifiers_by_color[single_color_test_bit_to_color_t(PLAYER_CARD_INSTANCE(player, card).color)] + amount);
      }
      else
      {
        return result;
      }
    }
  }

  return 0;
}

// FUNCTION: MOK 0x004a09a0
// FUNCTION: MAGIC 0x004ef850
// FUNCTION: SHANDALAR 0x004a9910
int create_legacy_effect(int player, int card, int legacy_iid, int target_player, int target_card)
{
  int i;
  int legacy_card;
  int source_internal_card_id;

  legacy_card = add_card_to_hand(player, legacy_iid);
  if (legacy_card != -1 && card != -1)
  {
    PLAYER_CARD_INSTANCE(player, legacy_card).state = ((player != 0) ? STATE_OWNED_BY_OPPONENT : 0) | STATE_IN_PLAY;
    PLAYER_CARD_INSTANCE(player, legacy_card).mana_color = PLAYER_CARD_INSTANCE(player, card).mana_color;
    PLAYER_CARD_INSTANCE(player, legacy_card).color = PLAYER_CARD_INSTANCE(player, card).color;
    PLAYER_CARD_INSTANCE(player, legacy_card).damage_source_player = (char)player;
    PLAYER_CARD_INSTANCE(player, legacy_card).damage_source_card = card;

    if (PLAYER_CARD_INSTANCE(player, card).internal_card_id == -1 || PLAYER_CARD_INSTANCE(player, card).internal_card_id == g_stack_proxy_internal_card_id)
    {
      source_internal_card_id = PLAYER_CARD_INSTANCE(player, card).original_internal_card_id;
    }
    else
    {
      source_internal_card_id = PLAYER_CARD_INSTANCE(player, card).internal_card_id;
    }

    *(unsigned int *)&PLAYER_CARD_INSTANCE(player, legacy_card).display_pic_csv_id =
        ((unsigned int)get_card_display_pic_num(global_cards_data[source_internal_card_id].id, player, card) << 16) |
        (unsigned int)global_cards_data[source_internal_card_id].id;
    PLAYER_CARD_INSTANCE(player, legacy_card).damage_target_player = (char)target_player;
    PLAYER_CARD_INSTANCE(player, legacy_card).damage_target_card = target_card;

    if (target_player != -1 && target_card != -1)
    {
      PLAYER_CARD_INSTANCE(target_player, target_card).regen_status |= 0x0f000000;
    }

    PLAYER_CARD_INSTANCE(player, legacy_card).token_status |= PLAYER_CARD_INSTANCE(player, card).token_status & 6;
    for (i = 0; i < 6; ++i)
    {
      PLAYER_CARD_INSTANCE(player, legacy_card).color_id[i] = PLAYER_CARD_INSTANCE(player, card).color_id[i];
      PLAYER_CARD_INSTANCE(player, legacy_card).hack_mode[i] = PLAYER_CARD_INSTANCE(player, card).hack_mode[i];
    }
  }

  return legacy_card;
}

// FUNCTION: MAGIC 0x0043ece1
// FUNCTION: SHANDALAR 0x0040a561
int resolve_cast_card(int player, int card)
{
  int force_special_mode;

  force_special_mode = 0;
  if (g_current_action_event_code == 0xd3)
  {
    g_duel_state_00791418 = 1;
    force_special_mode = 1;
  }

  if (!put_card_on_stack(player, card, 0))
  {
  }
  else
  {
    if (!put_card_on_stack(player, card, 1))
    {
    }
    else
    {
      if (!resolve_card_on_stack(player, card))
      {
      }
      else
      {
        if (force_special_mode != 0)
        {
          g_duel_state_00791418 = 0;
        }
        return 1;
      }
    }
  }

  if (force_special_mode != 0)
  {
    g_duel_state_00791418 = 0;
  }
  return 0;
}

// FUNCTION: MAGIC 0x0043ec38
// FUNCTION: SHANDALAR 0x0040a4b8
int discard_card_from_hand(int player, int card)
{
  dispatch_event_to_single_card(player, card, EVENT_DISCARD, 1 - player, -1);
  move_card_to_graveyard(player, card);
  PLAYER_CARD_INSTANCE(player, card).internal_card_id = -1;
  request_duel_display_refresh_if_human(player, card, 11, 1);
  if (g_duel_ai_mode_state != 1)
  {
    play_sound_effect(WAV_DISCARD);
  }
  --g_duel_summary.hand_counts[player];
  return 0;
}

// FUNCTION: MAGIC 0x00446c95
// FUNCTION: SHANDALAR 0x00450813
void request_duel_display_refresh(int player, int card, int unk1, int unk2)
{
  (void)player;
  (void)card;
  (void)unk1;
  (void)unk2;
  SendMessageA((HWND)g_duel_window_hwnd, 0x464, 0xff, 0);
}

// FUNCTION: MAGIC 0x004a61a1
// FUNCTION: SHANDALAR 0x00558321
void request_duel_display_refresh_if_human(int player, int card, int unk1, int unk2)
{
  if (g_duel_ai_mode_state == 1)
    return;

  request_duel_display_refresh(player, card, unk1, unk2);
}

// FUNCTION: MAGIC 0x004b14f5
// FUNCTION: SHANDALAR 0x0046e4d0
void move_card_to_graveyard(int player, int card)
{
  int graveyard_index;
  int original_internal_card_id;
  unsigned int graveyard_player;

  original_internal_card_id = PLAYER_CARD_INSTANCE(player, card).original_internal_card_id;
  graveyard_player = (PLAYER_CARD_INSTANCE(player, card).state & 0x1000) != 0;
  g_graveyard_card_types[graveyard_player] |= global_cards_data[original_internal_card_id].type;

  graveyard_index = 0;
  while (graveyard_index < 500)
  {
    if (global_graveyard_slots[graveyard_player][graveyard_index] == -1)
    {
      break;
    }
    ++graveyard_index;
  }

  if (graveyard_index < 500)
  {
    global_graveyard_slots[graveyard_player][graveyard_index] = original_internal_card_id;
  }
}

// FUNCTION: MAGIC 0x004b5cf5
// FUNCTION: SHANDALAR 0x0056c4f7
void put_card_on_bottom_of_library(int player, int internal_card_id)
{
  int deck_position;

  deck_position = 0;
  while (deck_position < 500 && global_library[player][deck_position] != -1)
  {
    ++deck_position;
  }
  if (deck_position < 500)
  {
    global_library[player][deck_position] = internal_card_id;
  }
}

// FUNCTION: MAGIC 0x004e698f
// FUNCTION: SHANDALAR 0x004c5bf6
void get_landwalk_evasion_masks(unsigned int *out_landwalk_mask, unsigned int *out_basic_land_mask)
{
  int color;
  unsigned int bits2;
  unsigned int bits1;

  bits2 = 0;
  bits1 = bits2;

  for (color = 1; color <= 5; ++color)
  {
    if (g_landwalk_basiclandtypes_controlled[0][color] > 0)
    {
      bits1 |= 1u << (color - 1);
    }
    if (g_basiclandtypes_controlled[0][color] > 0)
    {
      bits2 |= 1u << (color - 1);
    }
  }

  if (out_landwalk_mask != NULL)
  {
    *out_landwalk_mask = bits1;
  }
  if (out_basic_land_mask != NULL)
  {
    *out_basic_land_mask = bits2;
  }
}

// FUNCTION: MAGIC 0x004827ad
// FUNCTION: SHANDALAR 0x004862c0
unsigned int get_card_color_after_hacks(int player, int card)
{
  unsigned int result;
  int base_color;

  result = 0;
  base_color = global_cards_data[PLAYER_CARD_INSTANCE(player, card).internal_card_id].color;
  if ((base_color & 2) != 0)
  {
    result |= 1 << (unsigned char)get_hacked_color(player, card, 1);
  }
  if ((base_color & 4) != 0)
  {
    result |= 1 << (unsigned char)get_hacked_color(player, card, 2);
  }
  if ((base_color & 8) != 0)
  {
    result |= 1 << (unsigned char)get_hacked_color(player, card, 3);
  }
  if ((base_color & 0x10) != 0)
  {
    result |= 1 << (unsigned char)get_hacked_color(player, card, 4);
  }
  if ((base_color & 0x20) != 0)
  {
    result |= 1 << (unsigned char)get_hacked_color(player, card, 5);
  }
  return result;
}

// FUNCTION: MAGIC 0x00441f16
// FUNCTION: SHANDALAR 0x0040d7b2
int C_get_abilities(int player, int card, event_t event, int new_attacking_card)
{
  card_instance_t *test_instance;
  unsigned int current_color;
  int test_card;
  card_instance_t *instance;
  unsigned int current_abilities;
  int cleanup_card;
  int test_player;
  unsigned int result;

  instance = &PLAYER_CARD_INSTANCE(player, card);
  ++g_affected_card_context_index;
  current_abilities = g_adventure_duel_internal_card_id;
  if (g_duel_active != 0)
  {
    push_affected_card_stack();
  }
  g_affected_card_controller = player;
  g_affected_card = card;
  g_affected_internal_card_id = instance->internal_card_id;
  g_affected_card_color = (int)(char)global_cards_data[g_affected_internal_card_id].color;
  g_attacking_card = new_attacking_card;

  switch (event)
  {
  case EVENT_POWER:
    if ((instance->state & 0x800002) == 2)
    {
      result = (int)global_cards_data[g_affected_internal_card_id].power & 0xffffbfff;
    }
    else
    {
      result = (unsigned int)global_cards_data[g_affected_internal_card_id].power;
    }
    result += instance->counter_power;
    if ((instance->regen_status & 0x04000000) == 0)
    {
      g_event_result = (int)instance->power;
      goto post_dispatch;
    }
    else
    {
      instance->regen_status &= 0xfbffffff;
    }
    break;

  case EVENT_TOUGHNESS:
    if ((instance->state & 0x800002) == 2)
    {
      result = (int)global_cards_data[g_affected_internal_card_id].toughness & 0xffffbfff;
    }
    else
    {
      result = (unsigned int)global_cards_data[g_affected_internal_card_id].toughness;
    }
    result += instance->counter_toughness;
    if ((instance->regen_status & 0x02000000) == 0)
    {
      g_event_result = (int)instance->toughness;
      goto post_dispatch;
    }
    else
    {
      instance->regen_status &= 0xfdffffff;
    }
    break;

  case EVENT_ABILITIES:
    result = global_cards_data[g_affected_internal_card_id].static_ability | (instance->regen_status & 0x07000000);
    if ((result & 0x1ff81f) != 0)
    {
      current_color = 0;
      for (test_card = 0; test_card < 5; ++test_card)
      {
        if ((result & (1 << test_card)) != 0)
        {
          current_color |= 1 << (get_hacked_color(player, card, test_card + 1) - 1);
        }
        if ((result & (0x800 << test_card)) != 0)
        {
          current_color |= 0x800 << (get_sleighted_color(player, card, test_card + 1) - 1);
        }
      }
      result &= 0xffe007e0;
      result |= current_color;
    }
    if ((instance->regen_status & 0x08000000) == 0)
    {
      g_event_result = instance->regen_status;
      goto post_dispatch;
    }
    else
    {
      instance->regen_status &= 0xf7ffffff;
    }
    break;

  case EVENT_CHANGE_TYPE:
    if (((instance->internal_card_id >= g_damage_card_internal_card_id) &&
         (g_damage_card_internal_card_id + 0x2d > instance->internal_card_id)) ||
        instance->internal_card_id == -1)
    {
      g_event_result = instance->internal_card_id;
      goto post_dispatch;
    }

    result = instance->original_internal_card_id;
    if ((instance->regen_status & 0x01000000) == 0)
    {
      g_event_result = instance->internal_card_id;
      goto post_dispatch;
    }
    else
    {
      instance->internal_card_id = instance->original_internal_card_id;
      instance->regen_status &= 0xfeffffff;
      instance->mana_color = global_cards_data[instance->original_internal_card_id].color;
    }

    instance->destroys_if_blocked = 0;
    break;

  case EVENT_RECALC_DAMAGE:
    result = (unsigned int)(short)instance->damage_on_card;
    break;

  case EVENT_SET_COLOR:
    result = (unsigned int)(char)global_cards_data[g_affected_internal_card_id].color;
    break;

  default:
    result = 0;
  }

  g_event_result = result;
  if (g_duel_active != 0)
  {
    C_dispatch_event_raw(event);
    if (((g_land_can_be_played & 0x10000) != 0) && (event == EVENT_CHANGE_TYPE))
    {
      g_land_can_be_played &= ~0x10000;
      instance->internal_card_id = g_event_result;
      g_land_can_be_played |= 0x20000;
      C_dispatch_event_raw(event);
      g_land_can_be_played &= ~0x20000;
    }
  }

  if (event == EVENT_POWER)
  {
    if ((int)g_event_result < 0)
    {
      g_event_result = 0;
    }
    if ((instance->token_status & 0x4000) != 0)
    {
      g_event_result <<= 1;
    }
  }

post_dispatch:
  result = g_event_result;
  if (is_in_play(player, card) && event == EVENT_TOUGHNESS && (global_cards_data[g_affected_internal_card_id].type & TYPE_CREATURE) != 0 && ((int)result <= 0 || (int)result <= (int)(short)instance->damage_on_card) && ((PLAYER_CARD_INSTANCE(player, card).token_status & 0x04000000) == 0) && g_trigger_condition == -1 && (g_land_can_be_played & 0x204) == 0)
  {
    kill_card(player, card, KILL_DESTROY);
    regenerate_or_graveyard_triggers();
  }

  if (g_duel_active != 0)
  {
    pop_affected_card_stack();
  }
  if (event == EVENT_POWER)
  {
    instance->power = (short)result;
  }
  if (event == EVENT_TOUGHNESS)
  {
    instance->toughness = (short)result;
  }
  if (event == EVENT_ABILITIES)
  {
    instance->regen_status = result;
  }
  if (event == EVENT_CHANGE_TYPE)
  {
    instance->internal_card_id = result;
    if ((global_cards_data[result].extra_ability & 0x1000) != 0)
    {
      switch (global_cards_data[result].id)
      {
      case 0x13e:
        break;

      case 0x366:
        instance->mana_color = (unsigned char)instance->info_slot;
        break;

      case 0x12c:
      case 0x272:
        instance->mana_color = 1;
        break;

      case 0x139:
        instance->mana_color = 2;
        break;

      case 0x134:
      case 0x27b:
        instance->mana_color = 0x10;
        break;

      case 0xf:
      case 0x193:
        instance->mana_color = 0x3e;
        break;

      default:
        if (global_cards_data[result].subtype == 0x0c && (instance->token_status & 2) != 0)
        {
          instance->mana_color = (unsigned char)get_card_color_after_hacks(player, card);
        }
        else
        {
          instance->mana_color = global_cards_data[result].color;
        }
        break;
      }
    }

    if ((instance->token_status & 0x40) != 0 && (global_cards_data[instance->internal_card_id].type & TYPE_CREATURE) == 0)
    {
      for (test_player = 0; test_player < 2; ++test_player)
      {
        test_instance = global_card_instances[test_player];
        for (cleanup_card = 0; cleanup_card < g_active_cards_count[test_player]; ++cleanup_card, ++test_instance)
        {
          if (test_instance->internal_card_id == -1 || (test_instance->state & (STATE_OUBLIETTED | STATE_IN_PLAY)) != 2)
          {
            continue;
          }

          if (test_instance->damage_target_player == player && test_instance->damage_target_card == card &&
              (global_cards_data[test_instance->internal_card_id].type & TYPE_ENCHANTMENT) != 0 &&
              global_raw_cards_storage[global_cards_data[test_instance->internal_card_id].id].subtype == 0x2c)
          {
            kill_card(test_player, cleanup_card, KILL_SACRIFICE);
          }
        }
      }
    }

    if ((instance->state & 2) != 0)
    {
      g_battlefield_extra_ability_flags |= global_cards_data[instance->internal_card_id].extra_ability & 0x1ffc0000;
    }
  }

  g_adventure_duel_internal_card_id = current_abilities;
  return result;
}

// FUNCTION: MAGIC 0x00500a13
// FUNCTION: SHANDALAR 0x004bab38
void damage_player(int target_player, int amount, int source_player, int source_card)
{
  damage_creature(target_player, -1, amount, source_player, source_card);
}

// FUNCTION: MOK 0x004302c0
// FUNCTION: MAGIC 0x00435e27
// FUNCTION: SHANDALAR 0x00417301
int charge_mana_w_global_cost_mod(int player, int card, int color, int amount)
{
  int result;

  g_mana_charge[0] += g_mana_cost_modifiers_by_color[single_color_test_bit_to_color_t(PLAYER_CARD_INSTANCE(player, card).color)];
  result = charge_mana(player, color, amount) - g_mana_cost_modifiers_by_color[single_color_test_bit_to_color_t(PLAYER_CARD_INSTANCE(player, card).color)];
  if (g_spell_fizzled == 1)
  {
    return 0;
  }
  return result;
}

// FUNCTION: MAGIC 0x0044331f
// FUNCTION: SHANDALAR 0x0040ebb9
int play_sound_effect(wav_t sound_id)
{
  char path[264];
  int replacement_result;
  int sound_num;
  Sound sound;

  sound.volume = 300;
  sound.sampleRate = 0;
  sound.pan = 0;
  sound.field_C = 0;
  sound.field_10 = 0;
  sound.field_14 = 0;
  sound.loadId = sound_id;
  sound.flags = 0;

  if (g_duel_ai_mode_state == 1)
  {
    return 0;
  }

  sound_num = sound_id;
  if (sound_num <= 0x13)
  {
    sound_play(sound_num, 0);
  }
  else if (sound_num <= 0x26)
  {
    if (sound_is_loaded(sound_id, &sound_num) == 0)
    {
      replacement_result = sound_get_lru(&sound_num, 0x14, 0x16);
      if (replacement_result == 0)
      {
        sound_unload(sound_num);
      }
      else if (replacement_result != 1)
      {
        return 0;
      }

      strcpy(path, global_duelsounds_path);
      strcat(path, "\\");
      strcat(path, g_duel_sound_filenames[sound_id]);
      sound_load(path, sound_num, &sound);
    }

    sound_play(sound_num, 0);
  }
  else if (sound_num <= 0x2d)
  {
    if (sound_is_loaded(sound_id, &sound_num) == 0)
    {
      replacement_result = sound_get_lru(&sound_num, 0x27, 0x27);
      if (replacement_result == 0)
      {
        sound_unload(sound_num);
      }
      else if (replacement_result != 1)
      {
        return 0;
      }

      strcpy(path, global_duelsounds_path);
      strcat(path, "\\");
      strcat(path, g_duel_sound_filenames[sound_id + 1]);
      sound_load(path, sound_num, &sound);
    }

    sound_play(sound_num, 0);
  }
  else if (sound_num <= 0x3b)
  {
    sound.volume = 400;
    if (sound_is_loaded(sound_id, &sound_num) == 0)
    {
      if (sound_id != WAV_CATATAP)
      {
        sound.flags |= 4U;
      }
      else
      {
        sound.field_14 = -1;
      }

      strcpy(path, global_duelsounds_path);
      strcat(path, "\\");
      strcat(path, g_duel_sound_filenames[sound_id + 2]);
      sound_load(path, sound_num, &sound);
      sound_play(sound_num, &sound);
    }
    else
    {
      if (sound_id != WAV_CATATAP)
      {
        sound.flags |= 4U;
      }
      else
      {
        sound.field_14 = -1;
      }

      sound_play(sound_num, &sound);
    }
  }
  else if (sound_num <= 0x40)
  {
    if (sound_is_loaded(sound_id, &sound_num) == 0)
    {
      replacement_result = sound_get_lru(&sound_num, 0x3c, 0x40);
      if (replacement_result == 0)
      {
        sound_unload(sound_num);
      }
      else if (replacement_result != 1)
      {
        return 0;
      }

      strcpy(path, global_duelsounds_path);
      strcat(path, "\\");
      strcat(path, g_duel_sound_filenames[sound_id + 2]);
      sound_load(path, sound_num, &sound);
    }

    sound_play(sound_num, 0);
  }
  else if (sound_num <= 0x44)
  {
    if (sound_is_loaded(sound_id, &sound_num) == 0)
    {
      replacement_result = sound_get_lru(&sound_num, 0x41, 0x44);
      if (replacement_result == 0)
      {
        sound_unload(sound_num);
      }
      else if (replacement_result != 1)
      {
        return 0;
      }

      strcpy(path, global_duelsounds_path);
      strcat(path, "\\");
      strcat(path, g_duel_sound_filenames[sound_id + 3]);
      sound_load(path, sound_num, &sound);
    }

    sound_play(sound_num, 0);
  }
  else
  {
    return 0;
  }

  return 1;
}

// FUNCTION: MAGIC 0x0044125c
// FUNCTION: SHANDALAR 0x0040cadc
int can_attack(int player, int card)
{
  int saved_affected_card;
  int saved_affected_card_controller;
  int saved_event_result;
  int saved_spell_fizzled;
  int internal_card_id;
  int result;

  saved_affected_card = g_affected_card;
  saved_affected_card_controller = g_affected_card_controller;
  saved_spell_fizzled = g_spell_fizzled;
  saved_event_result = g_event_result;

  internal_card_id = PLAYER_CARD_INSTANCE(player, card).internal_card_id;
  if (((global_cards_data[internal_card_id].subtype == 0) && ((PLAYER_CARD_INSTANCE(player, card).token_status & 0x800) == 0)) ||
      (((global_cards_data[internal_card_id].type & TYPE_CREATURE) == 0) && ((PLAYER_CARD_INSTANCE(player, card).state & 0x01000000) == 0)) ||
      ((PLAYER_CARD_INSTANCE(player, card).state & 0x810010) != 0) || ((PLAYER_CARD_INSTANCE(player, card).token_status & 0x8000) != 0))
  {
    result = 0;
    g_event_result = saved_event_result;
    g_spell_fizzled = saved_spell_fizzled;
    g_affected_card_controller = saved_affected_card_controller;
    g_affected_card = saved_affected_card;
  }
  else
  {
    g_event_result = 0;
    g_affected_card_controller = player;
    g_affected_card = card;
    global_cards_data[internal_card_id].code_pointer(player, card, 0x79);
    if (g_event_result == 0)
    {
      g_event_result = saved_event_result;
      g_spell_fizzled = saved_spell_fizzled;
      g_affected_card_controller = saved_affected_card_controller;
      g_affected_card = saved_affected_card;

      if ((player == g_other_player) && ((g_duel_network_flags & 2) == 0))
      {
        push_affected_card_stack();
        g_event_result = 0;
        g_affected_card_controller = player;
        g_affected_card = card;
        dispatch_three_arg_callback_to_cards_in_play(check_attached_aura_can_pay_cost, -1);
        result = g_event_result;
        pop_affected_card_stack();
        if (result != 0)
        {
          return 0;
        }
      }

      if ((((g_player_special_effect_flags[1 - player] & 1) != 0) || ((g_battlefield_extra_ability_flags & 0x04000000) != 0)) &&
          ((result = dispatch_event(player, card, 0x79)) != 0))
      {
        return 0;
      }

      result = 1;
    }
    else
    {
      result = 0;
      g_event_result = saved_event_result;
      g_spell_fizzled = saved_spell_fizzled;
      g_affected_card_controller = saved_affected_card_controller;
      g_affected_card = saved_affected_card;
    }
  }

  return result;
}

// FUNCTION: MAGIC 0x005001c4
// FUNCTION: SHANDALAR 0x004ba2ed
void invalidate_dynamic_card_type(int internal_card_id)
{
  global_cards_data[internal_card_id].id = -1;
}

// FUNCTION: MAGIC 0x00483190
int find_matching_active_control_effect(int player, int card, int source_player, int source_card, int internal_card_id)
{
  int result = 0;

  if (global_cards_data[internal_card_id].id ==
          global_cards_data[PLAYER_CARD_INSTANCE(player, card).internal_card_id].id &&
      ((PLAYER_CARD_INSTANCE(source_player, source_card).token_status & 0x01000000) != 0))
  {
    g_event_result = (source_player << 16) | source_card;
    result = 1;
  }

  return result;
}

// FUNCTION: MAGIC 0x00483242
int find_matching_inactive_control_effect(int player, int card, int source_player, int source_card, int internal_card_id)
{
  int result = 0;

  if (global_cards_data[internal_card_id].id ==
          global_cards_data[PLAYER_CARD_INSTANCE(player, card).internal_card_id].id &&
      ((PLAYER_CARD_INSTANCE(source_player, source_card).token_status & 0x01000000) == 0))
  {
    g_event_result = (source_player << 16) | source_card;
    result = 1;
  }

  return result;
}

// FUNCTION: MAGIC 0x004b413c
// FUNCTION: SHANDALAR 0x0056a3a7
int select_damage_card_from_list(int player,
                                 int *internal_card_ids,
                                 int *damage_amounts,
                                 int count,
                                 int title,
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
    selected = (int)show_damage_assignment_cardlist(internal_card_ids, damage_amounts, count, title, allow_cancel, prompt);
    if ((g_duel_network_flags & 2) != 0)
    {
      g_network_result_packet.result = selected;
      g_network_result_packet.packet_type = 0x19;
      TENTATIVE_send_network_result(player, 0x19);
    }
  }

  return selected;
}

// FUNCTION: MAGIC 0x0055d802
// FUNCTION: SHANDALAR 0x00573b1c
void extract_numbered_text_choice(char *out, char *in, int choice)
{
  char *write_ptr;
  int done;
  int matched;
  char *scan;

  if (out == NULL || in == NULL)
  {
    return;
  }

  done = 0;
  matched = 0;
  scan = in;
  while (done == 0)
  {
    while (*scan != '\0' && strncmp(scan, "|#", 2) != 0)
    {
      ++scan;
    }

    if (*scan == '\0')
    {
      done = 1;
    }
    else
    {
      ++scan;
      ++scan;
      if (atoi(scan) == choice)
      {
        matched = 1;
        ++scan;
        write_ptr = out;
        while (*scan != '\0' && strncmp(scan, "|#", 2) != 0)
        {
          *write_ptr = *scan;
          ++scan;
          ++write_ptr;
        }
        *write_ptr = '\0';
        done = 1;
      }
    }
  }
}

// FUNCTION: MAGIC 0x00495311
// FUNCTION: SHANDALAR 0x00465dac
char *get_hunting_subtype_name(int value)
{
  if (value < 0 || value >= 0xd4)
  {
    return "";
  }
  else
    return gs_hunting_subtype_name_00926930[value];
}

// FUNCTION: MAGIC 0x004a61fe
int show_cardlist_if_human(int *cards,
                           int count,
                           void *window_title,
                           unsigned int require_selection,
                           char *prompt)
{
  if (g_duel_ai_mode_state == 1)
  {
    return 1;
  }

  return show_cardlist(cards, 0, 0, count, window_title, require_selection, prompt);
}

// FUNCTION: MAGIC 0x004ec616
// FUNCTION: SHANDALAR 0x0046a315
void position_duel_prompt_context_window(HWND prompt_hwnd)
{
  RECT rect1; // ebp-0x20
  RECT rect2; // ebp-0x10

  if (IsWindowVisible((HWND)g_duel_phase_display_window_hwnd))
  {
    GetWindowRect((HWND)g_duel_phase_display_window_hwnd, &rect1);
  }
  else if (IsWindowVisible((HWND)g_duel_attack_phase_window_hwnd))
  {
    GetWindowRect(g_attack_phase_window_hwnd, &rect1);
  }
  else
  {
    GetWindowRect(g_duel_help_owner_hwnd, &rect1);
    GetWindowRect(prompt_hwnd, &rect2);
    rect1.bottom -= (rect2.bottom - rect2.top) / 2;
  }

  SetWindowPos(prompt_hwnd, (HWND)0, rect1.left, rect1.bottom, 0, 0, 5);
}

// FUNCTION: MAGIC 0x004ec6c3
// FUNCTION: SHANDALAR 0x0046a3c1
void setup_duel_prompt_context_text_dc(HWND prompt_hwnd, HDC dc, int *rect_values)
{
  RECT rect;        // ebp-0x4c
  HGDIOBJ wnd_font; // ebp-0x3c
  TEXTMETRICA tm;   // ebp-0x38

  wnd_font = (HGDIOBJ)GetWindowLongA(prompt_hwnd, g_duel_ui_initialized);
  if (wnd_font != (HGDIOBJ)0)
  {
    SelectObject(dc, wnd_font);
  }

  GetTextMetricsA(dc, &tm);

  *rect_values += tm.tmHeight / 2;
  rect_values[1] += 4;
  rect_values[3] -= 3;

  if (IsWindowVisible(g_duel_client_window))
  {
    GetWindowRect(g_duel_client_window, &rect);
    MapWindowPoints((HWND)0, prompt_hwnd, (LPPOINT)&rect, 2);
    *rect_values = max(rect.right + tm.tmHeight / 2, *rect_values);
  }

  if (IsWindowVisible(g_duel_main_window))
  {
    GetWindowRect(g_duel_main_window, &rect);
    MapWindowPoints((HWND)0, prompt_hwnd, (LPPOINT)&rect, 2);
    *rect_values = max(rect.right + tm.tmHeight / 2, *rect_values);
  }

  SetMapMode(dc, 8);
  SetWindowExtEx(dc, rect_values[2] - *rect_values, 0x14, (LPSIZE)0);
  SetViewportExtEx(dc, rect_values[2] - *rect_values, rect_values[3] - rect_values[1], (LPSIZE)0);
}

// FUNCTION: MAGIC 0x004ec223
// FUNCTION: SHANDALAR 0x00469f23
void set_duel_prompt_context(HWND prompt_hwnd, char *text, unsigned int button_flags)
{
  HDC dc2;                 // ebp-0x74
  RECT rect1;              // ebp-0x70
  HDC dc;                  // ebp-0x60
  RECT rect2;              // ebp-0x5c
  SIZE text_size;          // ebp-0x4c
  int border;              // ebp-0x44
  int icon_width;          // ebp-0x40
  RECT rect3;              // ebp-0x3c
  int extra_width;         // ebp-0x2c
  int gap;                 // ebp-0x28
  int y;                   // ebp-0x24
  unsigned int text_width; // ebp-0x20
  int padding;             // ebp-0x1c
  int x;                   // ebp-0x18
  int client_h;            // ebp-0x14
  RECT rect4;              // ebp-0x10

  border = 4;
  padding = 4;
  gap = 7;

  if (text == NULL || strlen(text) == 0)
  {
    ShowWindow(prompt_hwnd, 0);
  }

  if (button_flags == 0 || (button_flags & 1) == 0)
  {
    ShowWindow(g_duel_client_window, 0);
  }

  if (button_flags == 0 || (button_flags & 2) == 0)
  {
    ShowWindow(g_duel_main_window, 0);
  }

  extra_width = 0;
  dc = GetDC(prompt_hwnd);
  GetClientRect(prompt_hwnd, &rect2);

  client_h = (rect2.bottom - rect2.top) - padding * 2;
  setup_duel_prompt_context_text_dc(prompt_hwnd, dc, (int *)&rect2.left);

  GetTextExtentPoint32A(dc, g_duel_status_text, strlen(g_duel_status_text), &text_size);
  icon_width = text_size.cx + client_h / 2;

  rect2.right = 2000;
  text_width = (unsigned int)(unsigned short)CalcDrawManaText(dc, &rect2, text);
  ReleaseDC(prompt_hwnd, dc);

  if ((button_flags & 2) != 0 || (button_flags & 1) != 0)
  {
    SetWindowPos(g_duel_main_window, (HWND)0, 0, 0, icon_width, client_h, 6);
    extra_width = extra_width + icon_width + gap;
  }

  if ((button_flags & 1) != 0)
  {
    SetWindowPos(g_duel_client_window, (HWND)0, 0, 0, icon_width, client_h, 6);
    extra_width = extra_width + icon_width + gap;
  }

  if (text != NULL && strlen(text) != 0)
  {
    dc2 = GetDC(prompt_hwnd);
    GetClientRect(prompt_hwnd, &rect1);
    setup_duel_prompt_context_text_dc(prompt_hwnd, dc2, (int *)&rect1.left);
    rect1.right = 2000;
    text_width = (unsigned int)(unsigned short)CalcDrawManaText(dc2, &rect1, text);
    ReleaseDC(prompt_hwnd, dc2);
    SetWindowTextA(prompt_hwnd, text);
  }
  else
  {
    text_width = 0;
    // STRING: MAGIC 0x0057b054
    SetWindowTextA(prompt_hwnd, "");
  }

  GetWindowRect(prompt_hwnd, &rect4);
  SetWindowPos(prompt_hwnd,
               (HWND)0,
               0,
               0,
               border * 2 + extra_width + text_width + 0x19,
               rect4.bottom - rect4.top,
               6);

  GetClientRect(prompt_hwnd, &rect4);
  x = rect4.left + border;

  if ((button_flags & 2) != 0)
  {
    GetWindowRect(g_duel_main_window, &rect3);
    x = x + gap;
    y = (rect4.bottom - rect4.top) / 2 - (rect3.bottom - rect3.top) / 2;
    SetWindowPos(g_duel_main_window, (HWND)0, x, y, 0, 0, 5);
  }

  if ((button_flags & 1) != 0)
  {
    GetWindowRect(g_duel_client_window, &rect3);
    x = x + (rect3.right - rect3.left) + gap;
    y = (rect4.bottom - rect4.top) / 2 - (rect3.bottom - rect3.top) / 2;
    SetWindowPos(g_duel_client_window, (HWND)0, x, y, 0, 0, 5);
  }

  position_duel_prompt_context_window(prompt_hwnd);
  InvalidateRect(g_duel_client_window, (RECT *)0, 1);
  InvalidateRect(g_duel_main_window, (RECT *)0, 1);
  InvalidateRect(prompt_hwnd, (RECT *)0, 1);

  if ((button_flags & 1) != 0)
  {
    ShowWindow(g_duel_client_window, 5);
  }

  if ((button_flags & 2) != 0)
  {
    ShowWindow(g_duel_main_window, 5);
  }

  if (text != NULL)
  {
    if (strlen(text) != 0)
    {
      ShowWindow(prompt_hwnd, 5);
    }
  }

  restack_duel_child_windows();
  UpdateWindow(prompt_hwnd);
}

// FUNCTION: MAGIC 0x004480a6
// FUNCTION: SHANDALAR 0x00451c24
void display_duel_prompt_text(char *text)
{
  char *msg;

  if (text != NULL)
  {
    msg = text;
  }
  else
  {
    msg = g_empty_duel_prompt_text;
  }

  set_duel_prompt_context(g_duel_prompt_context_hwnd, msg, 0);
}

// FUNCTION: MAGIC 0x004a61d6
// FUNCTION: SHANDALAR 0x00558356
void set_duel_prompt_text(char *text)
{
  strcpy(g_current_duel_prompt_text, text);
  display_duel_prompt_text(text);
}

// FUNCTION: MAGIC 0x00482a97
// FUNCTION: SHANDALAR 0x004865a8
int has_effect_source_type(int player, int card, unsigned int flags)
{
  card_instance_t *instance;
  int source_internal_card_id;
  int found;
  int test_card;
  int source_player;
  int source_card;
  int test_player;

  if (player == -1 || flags == 0)
  {
    return 0;
  }

  found = 0;
  for (test_player = 0; test_player < 2 && found == 0; ++test_player)
  {
    for (test_card = 0; test_card < g_active_cards_count[test_player] && found == 0; ++test_card)
    {
      instance = &PLAYER_CARD_INSTANCE(test_player, test_card);
      if (instance->internal_card_id == g_damage_card_internal_card_id &&
          is_in_play(test_player, test_card) != 0 &&
          (int)(char)instance->damage_target_player == player &&
          instance->damage_target_card == card &&
          instance->info_slot != 0)
      {
        source_player = (int)(char)instance->damage_source_player;
        source_card = instance->damage_source_card;

        if (PLAYER_CARD_INSTANCE(source_player, source_card).internal_card_id != -1 &&
            global_cards_data[PLAYER_CARD_INSTANCE(source_player, source_card).internal_card_id].id == g_legacy_display_internal_card_id)
        {
          source_player =
              PLAYER_CARD_INSTANCE((int)(char)PLAYER_CARD_INSTANCE(source_player, source_card).damage_source_player,
                                   source_card)
                  .info_slot;
        }

        if (PLAYER_CARD_INSTANCE(source_player, source_card).internal_card_id != -1)
        {
          source_internal_card_id = PLAYER_CARD_INSTANCE(source_player, source_card).internal_card_id;
        }
        else
        {
          source_internal_card_id = PLAYER_CARD_INSTANCE(source_player, source_card).original_internal_card_id;
        }

        if ((flags & (unsigned char)global_cards_data[source_internal_card_id].type) != 0)
        {
          found = 1;
        }
      }
    }
  }

  return found;
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
    PLAYER_CARD_INSTANCE(player, card).targets[PLAYER_CARD_INSTANCE(player, card).number_of_targets] = selected_target;
    ++PLAYER_CARD_INSTANCE(player, card).number_of_targets;
    return 1;
  }

  return 0;
}

/* target_source_mode:
 *   0 = scan direct player/card targets
 *   1 = scan damage cards by damage_target_player/card
 *   2 = scan damage cards by damage_source_player/card
 */
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
      if ((required_state & TARGET_STATE_SUMMONING_SICK) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & 0x30000) == 0x30000)
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
      if ((illegal_state & TARGET_STATE_SUMMONING_SICK) != 0 && (PLAYER_CARD_INSTANCE(tgt_player, tgt_card).state & 0x30000) != 0)
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

// FUNCTION: MAGIC 0x00551240
// FUNCTION: SHANDALAR 0x0049f483
void add_special_counter(int player, int card)
{
  if ((PLAYER_CARD_INSTANCE(player, card).special_counters & 0xffu) == 0xffu)
  {
    return;
  }

  /* The exe increments only the low byte and preserves the upper 3 bytes. */
  PLAYER_CARD_INSTANCE(player, card).special_counters =
      ((PLAYER_CARD_INSTANCE(player, card).special_counters + 1) & 0xff) |
      (PLAYER_CARD_INSTANCE(player, card).special_counters & 0xffffff00);

  if (g_duel_ai_mode_state != 1)
  {
    play_sound_effect(ICON_HASTE);
  }
}

// FUNCTION: MAGIC 0x005514cd
// FUNCTION: SHANDALAR 0x0049f710
void remove_special_counters(int player, int card, int amount)
{
  global_card_instances[player][card].special_counters =
      ((global_card_instances[player][card].special_counters - amount) & 0xff) |
      (global_card_instances[player][card].special_counters & 0xffffff00);
}

// FUNCTION: MAGIC 0x00551572
// FUNCTION: SHANDALAR 0x0049f7b5
void set_special_counters(int player, int card, int amount)
{
  if (amount > 0xff)
  {
    amount = 0xff;
  }

  global_card_instances[player][card].special_counters =
      (global_card_instances[player][card].special_counters & 0xffffff00) | (unsigned char)amount;
}

// FUNCTION: MAGIC 0x005515f5
// FUNCTION: SHANDALAR 0x0049f838
unsigned int C_get_special_counters(int player, int card)
{
  return global_card_instances[player][card].special_counters & 0xff;
}

// FUNCTION: MAGIC 0x00441514
// FUNCTION: SHANDALAR 0x0040cd94
int has_vigilance(int player, int card)
{
  if (PLAYER_CARD_INSTANCE(player, card).state & STATE_VIGILANCE)
    return 1;

  return 0;
}

// FUNCTION: MAGIC 0x004418c6
// FUNCTION: SHANDALAR 0x0040d146
int can_block_attacker_with_abilities(int blocker_player,
                                      int blocker_card,
                                      int attacker_player,
                                      int attacker_card,
                                      unsigned int attacker_abilities,
                                      unsigned int land_bits)
{
#define BLOCKER_INSTANCE (*(card_instance_t *)((char *)&global_card_instances[0][blocker_card] + blocker_player * sizeof(global_card_instances[0])))
  int result;
  int internal_card_id;

  internal_card_id = BLOCKER_INSTANCE.internal_card_id;

  if ((((unsigned char)global_cards_data[internal_card_id].type & TYPE_CREATURE) == 0) &&
      ((BLOCKER_INSTANCE.state & 0x02000000) == 0))
  {
    return 0;
  }

  if (internal_card_id == -1 || (BLOCKER_INSTANCE.state & 0x800018) != 0)
  {
    return 0;
  }

  if (((BLOCKER_INSTANCE.state & 4) != 0) && has_vigilance(blocker_player, blocker_card) == 0)
  {
    return 0;
  }

  if ((BLOCKER_INSTANCE.state == 0) & 0x402)
  {
    return 0;
  }

  if ((attacker_abilities & 0x20) != 0)
  {
    if (((unsigned int)C_get_abilities(blocker_player, blocker_card, 0x34, -1) & 0x420) == 0)
    {
      return 0;
    }
  }

  if ((attacker_abilities & 0x1ff800) != 0)
  {
    if ((attacker_abilities &
         (0x800 << (unsigned char)(single_color_test_bit_to_color_t((int)(char)BLOCKER_INSTANCE.color) - 1))) != 0)
    {
      return 0;
    }
  }

  if ((((unsigned char)global_cards_data[BLOCKER_INSTANCE.internal_card_id].type & TYPE_ARTIFACT) != 0) && (attacker_abilities & 0x10000) != 0)
  {
    return 0;
  }

  if ((land_bits & attacker_abilities & 0x1f) != 0)
  {
    return 0;
  }

  push_affected_card_stack();
  result = 1;
  g_affected_card_controller = blocker_player;
  g_affected_card = blocker_card;
  g_attacking_card_controller = attacker_player;
  g_attacking_card = attacker_card;
  g_event_result = 0;
  C_dispatch_event_raw(0x78);
  if (g_event_result > 0)
  {
    result = 0;
  }
  pop_affected_card_stack();

  return result;
#undef BLOCKER_INSTANCE
}

// FUNCTION: MAGIC 0x00443ee2
// FUNCTION: SHANDALAR 0x0040f77c
int push_card_onto_stack(int player, int card, int event, int extra, int prompt)
{
  int should_push;
  int stack_card;
  int saved_timestamp;

  if (g_stack_size >= 32)
    return 0;

  g_stack_data[g_stack_size] = PLAYER_CARD_INSTANCE(player, card).internal_card_id;
  g_stack_data[g_stack_size] |= event << 16;
  g_stack_data[g_stack_size] |= extra << 24;
  if (event != EVENT_RESOLVE_SPELL && event != EVENT_RESOLVE_TRIGGER && PLAYER_CARD_INSTANCE(player, card).internal_card_id > 4)
  {
    stack_card = add_card_to_hand(player, g_stack_proxy_internal_card_id);
    if (stack_card != -1)
    {
      saved_timestamp = PLAYER_CARD_INSTANCE(player, stack_card).timestamp;
      memcpy(&PLAYER_CARD_INSTANCE(player, stack_card), &PLAYER_CARD_INSTANCE(player, card), sizeof(card_instance_t));
      PLAYER_CARD_INSTANCE(player, stack_card).internal_card_id = g_stack_proxy_internal_card_id;
      PLAYER_CARD_INSTANCE(player, stack_card).unknown0x14 = 0;
      PLAYER_CARD_INSTANCE(player, stack_card).kill_code = 0;
      if (PLAYER_CARD_INSTANCE(player, card).internal_card_id != -1)
        PLAYER_CARD_INSTANCE(player, stack_card).original_internal_card_id = PLAYER_CARD_INSTANCE(player, card).internal_card_id;
      else
        PLAYER_CARD_INSTANCE(player, stack_card).original_internal_card_id = PLAYER_CARD_INSTANCE(player, card).original_internal_card_id;
      *(int *)&PLAYER_CARD_INSTANCE(player, stack_card).display_pic_csv_id =
          *(int *)&PLAYER_CARD_INSTANCE(player, card).display_pic_csv_id;
      PLAYER_CARD_INSTANCE(player, stack_card).state |= 2;
      PLAYER_CARD_INSTANCE(player, stack_card).parent_controller = player;
      PLAYER_CARD_INSTANCE(player, stack_card).parent_card = card;
      PLAYER_CARD_INSTANCE(player, stack_card).timestamp = saved_timestamp;
      should_push = 1;
    }
    else
    {
      should_push = 0;
    }
  }
  else
  {
    stack_card = card;
    should_push = 1;
  }

  if (should_push)
  {
    global_stack_cards[g_stack_size].player = player;
    global_stack_cards[g_stack_size].card = stack_card;
    global_stack_damage_targets[g_stack_size].player = PLAYER_CARD_INSTANCE(player, card).damage_target_player;
    global_stack_damage_targets[g_stack_size].card = PLAYER_CARD_INSTANCE(player, card).damage_target_card;
    if (g_trigger_condition == -1)
    {
      g_stack_trigger_condition[g_stack_size] = g_current_phase;
    }
    else
    {
      g_stack_trigger_condition[g_stack_size] = g_trigger_condition;
    }
    if (g_duel_ai_mode_state != 1)
    {
      g_stack_prompt_flags[g_stack_size] = prompt;
    }
    ++g_stack_size;
    global_stack_cards[g_stack_size].player = -1;
  }

  return 0;
}

// FUNCTION: MAGIC 0x004eaceb
// FUNCTION: SHANDALAR 0x0044147b
void declare_mana_color_conversion(int player, int color_to_produce, int color_to_consume)
{
  int slot;
  int found;

  if (color_to_produce > 0 && color_to_consume > 0)
  {
    found = 0;
    slot = 0;
    for (; slot < 10 && !found; ++slot)
    {
      if (g_mana_color_conversions[player][slot] == -1)
      {
        found = 1;
        g_mana_color_conversions[player][slot] = MAKELONG(color_to_produce, color_to_consume);
        g_mana_color_conversions[player][slot + 1] = -1;
      }
    }
  }
}

// FUNCTION: MAGIC 0x00551334
// FUNCTION: SHANDALAR 0x0049f577
void remove_special_counter(int player, int card)
{
  /* The exe only decrements the low byte of special_counters and preserves the upper 3 bytes. */
  global_card_instances[player][card].special_counters =
      (global_card_instances[player][card].special_counters & 0xffffff00) |
      ((global_card_instances[player][card].special_counters - 1) & 0xff);
}

// FUNCTION: MAGIC 0x00444b74
// FUNCTION: SHANDALAR 0x0041040d
int obliterate_top_card_of_stack(void)
{
  int card;
  int current_internal;
  int player;

  if (g_stack_size <= 0)
  {
    return 0;
  }

  --g_stack_size;
  player = global_stack_cards[g_stack_size].player;
  card = global_stack_cards[g_stack_size].card;
  current_internal = PLAYER_CARD_INSTANCE(player, card).internal_card_id;
  if (g_stack_proxy_internal_card_id == current_internal)
  {
    PLAYER_CARD_INSTANCE(player, card).internal_card_id = -1;
  }
  global_stack_cards[g_stack_size].player = -1;

  return 0;
}

// FUNCTION: MAGIC 0x00444505
// FUNCTION: SHANDALAR 0x0040fd9f
int resolve_top_card_on_stack(void)
{
  int player;           /* [ebp-0xc] */
  int current_internal; /* [ebp-0x8] */
  int card;             /* [ebp-0x4] */

  if (g_stack_size <= 0)
  {
    return 0;
  }

  --g_stack_size;
  player = global_stack_cards[g_stack_size].player;
  card = global_stack_cards[g_stack_size].card;
  current_internal = PLAYER_CARD_INSTANCE(player, card).internal_card_id;
  if (current_internal == g_stack_proxy_internal_card_id)
  {
    current_internal = PLAYER_CARD_INSTANCE(player, card).original_internal_card_id;
  }
  if (PLAYER_CARD_INSTANCE(player, card).internal_card_id != -1)
  {
    if ((((unsigned int)g_stack_data[g_stack_size] >> 0x10) & 0xff) == 0x7e)
    {
      dispatch_card_trigger_event(player,
                                  card,
                                  ((unsigned int)g_stack_data[g_stack_size] >> 0x10) & 0xff,
                                  ((int)g_stack_data[g_stack_size]) >> 0x18);
    }
    else
    {
      if ((PLAYER_CARD_INSTANCE(player, card).upkeep_flags & 8) != 0)
      {
        if ((PLAYER_CARD_INSTANCE(player, card).upkeep_flags & 0x200) == 0)
        {
          dispatch_event_to_single_card(player, card, EVENT_UPKEEP_COSTS_UNPAID, 1 - player, -1);
          if (PLAYER_CARD_INSTANCE(player, card).internal_card_id != -1 &&
              (PLAYER_CARD_INSTANCE(player, card).upkeep_flags & 2) != 0)
          {
            kill_card(PLAYER_CARD_INSTANCE(player, card).parent_controller,
                      PLAYER_CARD_INSTANCE(player, card).parent_card,
                      KILL_BURY);
          }
        }
        PLAYER_CARD_INSTANCE(PLAYER_CARD_INSTANCE(player, card).parent_controller,
                             PLAYER_CARD_INSTANCE(player, card).parent_card)
            .upkeep_flags &= ~0x208;
        PLAYER_CARD_INSTANCE(PLAYER_CARD_INSTANCE(player, card).parent_controller,
                             PLAYER_CARD_INSTANCE(player, card).parent_card)
            .upkeep_flags |= 4;
      }
      else
      {
        if ((PLAYER_CARD_INSTANCE(player, card).upkeep_flags & 0x80) != 0)
        {
          if ((PLAYER_CARD_INSTANCE(player, card).upkeep_flags & 0x40) != 0)
          {
            PLAYER_CARD_INSTANCE(PLAYER_CARD_INSTANCE(player, card).parent_controller,
                                 PLAYER_CARD_INSTANCE(player, card).parent_card)
                .state &= ~0x10;
            dispatch_event_to_single_card(PLAYER_CARD_INSTANCE(player, card).parent_controller,
                                          PLAYER_CARD_INSTANCE(player, card).parent_card,
                                          EVENT_UNTAP_CARD,
                                          1 - player,
                                          -1);
          }
          PLAYER_CARD_INSTANCE(PLAYER_CARD_INSTANCE(player, card).parent_controller,
                               PLAYER_CARD_INSTANCE(player, card).parent_card)
              .upkeep_flags &= ~0x80;
        }
        else
        {
          dispatch_event_to_single_card(player,
                                        card,
                                        ((unsigned int)g_stack_data[g_stack_size] >> 0x10) & 0xff,
                                        1 - player,
                                        -1);
        }
      }
    }
    if (PLAYER_CARD_INSTANCE(player, card).internal_card_id == g_stack_proxy_internal_card_id)
    {
      kill_card(player, card, KILL_REMOVE);
    }
  }
  global_stack_cards[g_stack_size].player = -1;
  reassess_all_cards_and_mana();
  if (g_nested_trigger_depth <= 1 && ((g_land_can_be_played & 0x200) == 0 || g_stack_size == 0))
  {
    process_damage_prevention(g_current_player);
    regenerate_or_graveyard_triggers();
  }

  return 0;
}

// FUNCTION: MAGIC 0x004482f3
// FUNCTION: SHANDALAR 0x00451e70
int copy_mana_pool_to_display(void)
{
  if (g_duel_ai_mode_state != 1)
  {
    EnterCriticalSection(&g_duel_render_lock);
    memcpy(g_duel_cached_raw_mana_player_0, g_raw_mana_available[0], 0x1c);
    memcpy(g_duel_cached_raw_mana_player_1, g_raw_mana_available[1], 0x1c);
    LeaveCriticalSection(&g_duel_render_lock);
    SendMessageA(g_duel_active_popup_window, 0x432, 0, 0);
    SendMessageA(g_duel_last_active_window, 0x432, 0, 0);
  }

  return 0;
}

// FUNCTION: MAGIC 0x00441d78
// FUNCTION: SHANDALAR 0x0040d614
void reassess_all_cards_and_mana(void)
{
  int player;
  int card;
  card_instance_t *instance;

  for (player = 0; player < 2; ++player)
  {
    instance = global_card_instances[player];
    for (card = 0; card < g_active_cards_count[player]; ++card)
    {
      if ((instance->state & 0x800002) == 2 && instance->internal_card_id != -1)
      {
        instance->regen_status |= 0x0f000000;
        C_get_abilities(player, card, EVENT_CHANGE_TYPE, -1);
      }
      ++instance;
    }
  }

  C_count_colors_of_lands_in_play();
  count_mana();

  for (player = 0; player < 2; ++player)
  {
    instance = global_card_instances[player];
    for (card = 0; card < g_active_cards_count[player]; ++card)
    {
      if (is_in_play(player, card) && (global_cards_data[instance->internal_card_id].type & TYPE_CREATURE) && (short)instance->damage_on_card < C_get_abilities(player, card, EVENT_TOUGHNESS, -1))
      {
        C_get_abilities(player, card, EVENT_POWER, -1);
      }
      ++instance;
    }
  }
}

// FUNCTION: MAGIC 0x004afa4b
// FUNCTION: SHANDALAR 0x0046ca26
void process_damage_prevention(int player)
{
  int did_setup;    /* ebp-0x18 */
  int test_card;    /* ebp-0x14 */
  int test_player;  /* ebp-0x10 */
  int best_score;   /* ebp-0x0c */
  int saved_777854; /* ebp-0x08 */
  int temp;         /* ebp-0x04 */

  if ((g_land_can_be_played & 2) == 0)
  {
    return;
  }
  g_land_can_be_played &= ~2;
  g_land_can_be_played |= 4;
  TENTATIVE_reassess_all_cards(0, 0xff);

  for (test_player = 0; test_player < 2; ++test_player)
  {
    for (test_card = 0; test_card < g_active_cards_count[test_player]; ++test_card)
    {
      if (PLAYER_CARD_INSTANCE(test_player, test_card).internal_card_id == g_damage_card_internal_card_id &&
          is_in_play(test_player, test_card) != 0 &&
          (PLAYER_CARD_INSTANCE(test_player, test_card).state & 0x10) == 0)
      {
        dispatch_event(test_player, test_card, 0x21);
      }
    }
  }

  did_setup = 0;
setup:
  if (((g_duel_network_flags & 2) == 0) && g_duel_ai_mode_state != 1 && g_ai_action_replay_available == 0)
  {
    start_ai_decision_search(9, 0xf);
    best_score = -99999;
    did_setup = 1;
  }

after_setup:
  if (g_ai_decision_code == 9 && did_setup)
  {
    reset_ai_search_trial_state();
    g_ai_side_score_player_1 = 0;
    g_ai_side_score_player_0 = g_ai_side_score_player_1;
    g_duel_state_008a8de4 = g_ai_side_score_player_0;
    g_ai_modifier = g_duel_state_008a8de4;
  }

  if (allow_response(-2, -1, gs_damage_prevention_00939200, 0x8e) != 0)
  {
    goto setup;
  }

  C_dispatch_event_raw(0x25);
  if ((g_battlefield_extra_ability_flags & 0x00040000) != 0)
  {
    dispatch_trigger_twice_once_with_each_player_as_reason(g_current_player, TRIGGER_END_DAMAGE_PREV, gs_end_damage_prevention_00789740, 0);
  }

  for (test_player = 0; test_player < 2; ++test_player)
  {
    for (test_card = 0; test_card < g_active_cards_count[test_player]; ++test_card)
    {
      if (PLAYER_CARD_INSTANCE(test_player, test_card).internal_card_id == g_damage_card_internal_card_id &&
          is_in_play(test_player, test_card) != 0 &&
          (PLAYER_CARD_INSTANCE(test_player, test_card).state & 0x10) == 0)
      {
        dispatch_event(test_player, test_card, 0x6e);
      }
    }
  }

  dispatch_trigger_twice_once_with_each_player_as_reason(g_current_player, TRIGGER_DEAL_DAMAGE, gs_damage_dealing_00777ab0, 0);

  for (test_player = 0; test_player < 2; ++test_player)
  {
    for (test_card = 0; test_card < g_active_cards_count[test_player]; ++test_card)
    {
      if (PLAYER_CARD_INSTANCE(test_player, test_card).internal_card_id == g_damage_card_internal_card_id &&
          is_in_play(test_player, test_card) != 0)
      {
        if ((PLAYER_CARD_INSTANCE(test_player, test_card).state & 0x10) != 0)
        {
          kill_card(test_player, test_card, 1);
        }
        else
        {
          g_land_can_be_played |= 2;
        }
      }
    }
  }

  kill_creatures_with_lethal_damage();
  g_land_can_be_played &= ~4;

  if (g_duel_ai_mode_state == 1 && did_setup != 0 && g_ai_decision_code == 9)
  {
    regenerate_or_graveyard_triggers();
    temp = g_ai_modifier + ai_opinion_of_gamestate(g_other_player);
    if (temp > best_score)
    {
      best_score = temp;
      save_recorded_ai_actions();
      saved_777854 = g_ai_search_flags;
    }
    if (g_ai_search_target_depth == 999)
    {
      g_ai_search_target_depth = -1;
    }
    g_ai_search_force_pass = 0;
#ifdef SHANDALAR
    if ((g_game_time_scale * g_ai_search_time_limit) / 5 < GetUiTickCount())
#else
    if (get_duel_time_units_if_available() > (g_game_time_scale * g_ai_search_time_limit) / 5)
#endif
    {
      g_duel_ai_mode_state = 0;
      g_ai_search_target_depth = -1;
      g_ai_search_flags = saved_777854;
    }
    g_land_can_be_played |= 4;
    goto after_setup;
  }

  if (g_duel_ai_mode_state != 1 && did_setup != 0)
  {
    g_ai_action_replay_available = 0;
  }
}

// FUNCTION: MAGIC 0x004aff25
// FUNCTION: SHANDALAR 0x0046cf00
void kill_creatures_with_lethal_damage(void)
{
  int player;
  int card;

  for (player = 0; player < 2; ++player)
  {
    for (card = 0; card < g_active_cards_count[player]; ++card)
    {
      if (is_in_play(player, card) &&
          (global_cards_data[PLAYER_CARD_INSTANCE(player, card).internal_card_id].type & TYPE_CREATURE))
      {
        if (C_get_abilities(player, card, EVENT_TOUGHNESS, -1) <= (short)PLAYER_CARD_INSTANCE(player, card).damage_on_card)
        {
          if (g_duel_ai_mode_state != 1)
          {
            play_sound_effect(WAV_KILL);
          }
          kill_card(player, card, 2);
        }
      }
    }
  }
}

// FUNCTION: MAGIC 0x0044aa01
// FUNCTION: SHANDALAR 0x00403381
int should_skip_phase(int player)
{
  if ((g_duel_network_flags & 2) != 0)
  {
    if (g_stop_phase == -1)
    {
      if (g_previous_stop_phase == -1)
      {
        return 0;
      }
    }

    if (g_duel_ai_mode_state == 1)
    {
      return 0;
    }

    return 0;

    /* Dead code (kept for matching): MSVC /Od still emits this after the return. */
    if (g_previous_stop_phase == g_current_phase && g_current_player == g_previous_stop_phase_player)
    {
      return 0;
    }

    if (g_stop_phase == g_current_phase && g_current_player == g_stop_phase_player)
    {
      return 0;
    }

    if (g_previous_stop_phase == -1 && player == g_other_player)
    {
      if (((char)g_duel_phase_stop_settings[g_current_player].phase_flags[g_current_phase] & PHASE_STOP_OPPONENT) != 0)
      {
        return 0;
      }
    }

    if (g_stop_phase == -1 && player == g_active_player)
    {
      if (((char)g_duel_phase_stop_settings[g_current_player].phase_flags[g_current_phase] & PHASE_STOP_ENABLED) != 0)
      {
        return 0;
      }
    }

    if (!(g_stop_phase == g_current_phase && g_current_player == g_stop_phase_player))
    {
      if (g_previous_stop_phase == -1)
      {
        return 1;
      }
    }

    if (!(g_previous_stop_phase == g_current_phase && g_current_player == g_previous_stop_phase_player))
    {
      if (g_stop_phase == -1)
      {
        return 1;
      }
    }

    if (g_trigger_condition != -1 && g_stop_phase == g_current_phase)
    {
      return 1;
    }

    if (g_trigger_condition != -1 && g_previous_stop_phase == g_current_phase)
    {
      return 1;
    }

    return 0;
  }

  if (g_stop_phase == -1)
  {
    return 0;
  }
  if (g_duel_ai_mode_state == 1)
  {
    return 0;
  }

  if (g_stop_phase > g_current_phase)
  {
    return 1;
  }
  if (g_current_player != g_stop_phase_player)
  {
    return 1;
  }

  if (g_trigger_condition != -1)
  {
    if (g_stop_phase == g_current_phase)
    {
      return 1;
    }
  }

  if (g_stop_phase < g_current_phase)
  {
    if (g_current_player == g_stop_phase_player)
    {
      g_stop_phase_player = -1;
      g_stop_phase = g_stop_phase_player;
    }
  }

  return 0;
}

// FUNCTION: MAGIC 0x0044541f
// FUNCTION: SHANDALAR 0x00410cb8
int can_stop_for_phase(int use_current_player_stops)
{
  int can_respond;

  can_respond = 0;
  if (use_current_player_stops != 0)
  {
    if (g_current_player == g_active_player &&
        (((int)(char)g_duel_phase_stop_settings[g_current_player].phase_flags[g_current_phase]) & PHASE_STOP_ENABLED) != 0)
    {
      can_respond = 1;
    }

    if (g_current_player == g_other_player &&
        (((int)(char)g_duel_phase_stop_settings[g_current_player].phase_flags[g_current_phase]) & PHASE_STOP_OPPONENT) != 0)
    {
      can_respond = 1;
    }

    if (g_current_player == g_active_player)
    {
      if (g_current_player == g_stop_phase_player && g_current_phase == g_stop_phase)
      {
        can_respond = 1;
      }
    }
    else if (g_current_player == g_previous_stop_phase_player && g_previous_stop_phase == g_current_phase)
    {
      can_respond = 1;
    }
    if (should_skip_phase(g_current_player) == 0 && can_respond != 0)
    {
      return 1;
    }
    return 0;
  }

  /* use_current_player_stops == 0 */
  if (g_current_player == g_active_player &&
      (((int)(char)g_duel_phase_stop_settings[g_current_player].phase_flags[g_current_phase]) & PHASE_STOP_OPPONENT) != 0)
  {
    can_respond = 1;
  }

  if (g_current_player == g_other_player &&
      (((int)(char)g_duel_phase_stop_settings[g_current_player].phase_flags[g_current_phase]) & PHASE_STOP_ENABLED) != 0)
  {
    can_respond = 1;
  }

  if (g_current_player == g_other_player)
  {
    if (g_current_player == g_stop_phase_player && g_current_phase == g_stop_phase)
    {
      can_respond = 1;
    }
  }
  else if (g_current_player == g_previous_stop_phase_player && g_previous_stop_phase == g_current_phase)
  {
    can_respond = 1;
  }

  if (should_skip_phase(1 - g_current_player) == 0 && can_respond != 0)
  {
    return 1;
  }
  return 0;
}

// FUNCTION: MAGIC 0x004460d3
// FUNCTION: SHANDALAR 0x0041196b
void reset_upkeep_costs(void)
{
  int player;
  int card;
  int i;

  for (player = 0; player < 2; ++player)
  {
    for (card = 0; card < g_active_cards_count[player]; ++card)
    {
      if ((PLAYER_CARD_INSTANCE(player, card).upkeep_flags & UPKEEP_UPKEEP_DONE) == 0)
      {
        if (is_in_play(player, card))
        {
          for (i = 0; i < 7; ++i)
          {
            PLAYER_CARD_INSTANCE(player, card).mana_to_untap[i] = 0;
            PLAYER_CARD_INSTANCE(player, card).upkeep_cost[i] = PLAYER_CARD_INSTANCE(player, card).mana_to_untap[i];
          }

          PLAYER_CARD_INSTANCE(player, card).upkeep_flags = 0;
          dispatch_event(player, card, 0x85);
          dispatch_event(player, card, 0x84);
        }
      }
    }
  }
}

// FUNCTION: MAGIC 0x00447f80
int get_duel_time_units(void)
{
  int ticks = GetTickCount();
  int diff = ticks - g_affected_card_context_flags_0093f4b4 - g_duel_tick_adjustment;
  int result = diff / 0x37;
  return result;
}

// FUNCTION: MAGIC 0x004e1b6f
int get_duel_time_units_if_available(void)
{
  return get_duel_time_units();
}

// FUNCTION: MAGIC 0x004b082f
// FUNCTION: SHANDALAR 0x0046d80a
int dispatch_card_trigger_event(int player, int card, event_t event, int extra)
{
  if (event == EVENT_TRIGGER && ((PLAYER_CARD_INSTANCE(player, card).state & STATE_PROCESSING) != 0 || g_ai_search_result != 0))
  {
    return 0;
  }

  if (g_trigger_condition < 200)
  {
    return 0;
  }

  g_event_result = 0;
  g_affected_card_controller = player;
  g_affected_card = card;
  g_duel_state_008cfd20 = extra;
  g_attacking_card = -1;
  C_dispatch_event_raw(event);
  return g_event_result;
}

// FUNCTION: MAGIC 0x0044295b
// FUNCTION: SHANDALAR 0x0040e1f5
void C_dispatch_event_raw(event_t event)
{
  card_instance_t *instance;
  unsigned int timestamp_slot;
  int card;
  unsigned int saved_trigger_condition;
  int player;

  saved_trigger_condition = g_adventure_duel_internal_card_id;
  g_ai_search_retry_state = event;
  for (timestamp_slot = 0; (int)timestamp_slot < 500; ++timestamp_slot)
  {
    if (TENTATIVE_timestamp_player[timestamp_slot] == -1)
    {
      break;
    }

    player = TENTATIVE_timestamp_player[timestamp_slot];
    card = TENTATIVE_timestamp_card[timestamp_slot];
    instance = &PLAYER_CARD_INSTANCE(player, card);
    if (instance->timestamp != (int)timestamp_slot)
    {
      continue;
    }
    if (instance->internal_card_id == -1)
    {
      continue;
    }
    if ((instance->state & 0x800000) != 0)
    {
      continue;
    }
    if ((instance->state & 2) != 0 || (instance->state & 0x20) != 0)
    {

      g_opponent_starting_card_id_3 = player * 0x80 + card;
      (*global_cards_data[instance->internal_card_id].code_pointer)(player, card, event);
      if (event == EVENT_DECLARE_ATTACKERS && player == g_current_player && (instance->state & 0x14) == 4 && !has_vigilance(player, card))
      {
        instance->state |= 0x10;
        g_produced_mana_color = -1;
        dispatch_event(player, card, EVENT_TAP_CARD);
        process_damage_prevention(g_current_player);
        regenerate_or_graveyard_triggers();
      }
    }
  }
  if (event == EVENT_DECLARE_ATTACKERS && player == g_current_player)
  {
    reassess_all_cards_and_mana();
  }
  if (g_opponent_starting_card_id_2 != -1)
  {
    (*global_cards_data[g_opponent_starting_card_id_2].code_pointer)(0, 0x94, event);
  }
  g_adventure_duel_internal_card_id = saved_trigger_condition;
}

// FUNCTION: MAGIC 0x004a6d0e
// FUNCTION: SHANDALAR 0x00558e90
void count_mana(void)
{
  card_instance_t *instance;
  int current_card;
  int player;

  for (current_card = 0; current_card <= 7; ++current_card)
  {
    g_mana_cost_modifiers_by_color[current_card] = 0;
    g_special_mana_pool[1][current_card] = g_mana_cost_modifiers_by_color[current_card];
    g_special_mana_pool[0][current_card] = g_special_mana_pool[1][current_card];
  }

  g_raw_mana_available_hex[1][0] = -1;
  g_raw_mana_available_hex[0][0] = g_raw_mana_available_hex[1][0];
  g_mana_color_conversions[1][0] = -1;
  g_mana_color_conversions[0][0] = g_mana_color_conversions[1][0];
  for (player = 0; player < 2; ++player)
  {
    instance = global_card_instances[player];
    for (current_card = 0; current_card < g_active_cards_count[player];
         ++current_card, ++instance)
    {
      if (instance->internal_card_id == -1)
      {
        continue;
      }

      if ((global_cards_data[instance->internal_card_id].extra_ability & 0x1000) != 0 && (instance->state & 0x800012) == 2)
      {
        dispatch_event(player, current_card, EVENT_COUNT_MANA);
      }
      if (global_cards_data[instance->internal_card_id].id == 0xee && (instance->state & 0x800012) == 2)
      {
        dispatch_event_to_single_card(player, current_card, EVENT_COUNT_MANA, -1, -1);
      }
      if (global_cards_data[instance->internal_card_id].id == 100 && (instance->state & 0x800002) == 2)
      {
        dispatch_event_to_single_card(player, current_card, EVENT_COUNT_MANA, -1, -1);
      }
    }
  }
  if ((g_battlefield_extra_ability_flags & 0x08000000) != 0)
  {
    for (player = 0; player < 2; ++player)
    {
      instance = global_card_instances[player];
      for (current_card = 0; current_card < g_active_cards_count[player];
           ++current_card, ++instance)
      {
        if (instance->internal_card_id == -1)
        {
          continue;
        }

        if (global_cards_data[instance->internal_card_id].code_pointer == card_fellwar_stone && (instance->state & 0x800012) == 2)
        {
          dispatch_event_to_single_card(player, current_card, EVENT_VARIABLE_MANA_SRC, -1, -1);
        }
      }
    }
  }
}
// FUNCTION: MAGIC 0x004a6faa
// FUNCTION: SHANDALAR 0x0055912c
void C_count_colors_of_lands_in_play(void)
{
  int player;
  int card;
  int color;
  int internal_card_id;

  for (color = 0; color <= 7; ++color)
  {
    g_landwalk_basiclandtypes_controlled[0][color] = 0;
    g_basiclandtypes_controlled[0][color] = g_landwalk_basiclandtypes_controlled[0][color];
  }
  g_mana_pool_colorless_components[1] = 0;
  g_mana_pool_colorless_components[0] = g_mana_pool_colorless_components[1];

  for (player = 0; player < 2; ++player)
  {
    for (card = 0; card < g_active_cards_count[player]; ++card)
    {
      internal_card_id = PLAYER_CARD_INSTANCE(player, card).internal_card_id;
      if (is_in_play(player, card) && (global_cards_data[internal_card_id].type & TYPE_LAND) != 0)
      {
        g_mana_pool_colorless_components[player] |= PLAYER_CARD_INSTANCE(player, card).mana_color;
        if (internal_card_id < 5)
        {
          ++g_basiclandtypes_controlled[player][internal_card_id];
        }
        else if (internal_card_id > 0xe && internal_card_id < g_card_count)
        {
          ++g_basiclandtypes_controlled[player][0];
        }
        else
        {
          switch (global_cards_data[internal_card_id].id)
          {
          case 0xc:
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 1)];
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 4)];
            break;
          case 9:
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 1)];
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 3)];
            break;
          case 0xbd:
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 5)];
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 4)];
            break;
          case 0xd4:
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 5)];
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 3)];
            break;
          case 0xd8:
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 5)];
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 1)];
            break;
          case 0xf1:
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 3)];
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 4)];
            break;
          case 0xfc:
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 3)];
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 2)];
            break;
          case 0xfe:
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 5)];
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 2)];
            break;
          case 0x102:
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 1)];
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 2)];
            break;
          case 0x10a:
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 2)];
            ++g_basiclandtypes_controlled[player][get_hacked_color(player, card, 4)];
            break;
          case 0xef:
            ++g_basiclandtypes_controlled[player][1];
            break;
          case 0x7e:
            ++g_basiclandtypes_controlled[player][2];
            break;
          case 0x5b:
            ++g_basiclandtypes_controlled[player][3];
            break;
          case 0xa4:
            ++g_basiclandtypes_controlled[player][4];
            break;
          case 0xbc:
            ++g_basiclandtypes_controlled[player][5];
            break;
          default:
            ++g_basiclandtypes_controlled[player][0];
            break;
          }
        }
        ++g_basiclandtypes_controlled[player][7];
      }
    }
  }
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
          (PLAYER_CARD_INSTANCE(player, current_card).state & 0x800002) == 2 &&
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

// FUNCTION: MAGIC 0x004823a5
// FUNCTION: SHANDALAR 0x00485eb8
int resolve_control_aura_conflict(int player, int card)
{
  card_instance_t *instance; /* [ebp-0x20] */
  int target_player;         /* [ebp-0x1c] */
  int target_card;           /* [ebp-0x18] */
  int loop_player;           /* [ebp-0x14] */
  int loop_card;             /* [ebp-0x10] */
  int found_legacy;          /* [ebp-0x0c] */
  int saved_player;          /* [ebp-0x08] */
  int saved_card;            /* [ebp-0x04] */

  found_legacy = 0;

  target_player = (int)(&PLAYER_CARD_INSTANCE(player, card))->damage_target_player;
  target_card = (&PLAYER_CARD_INSTANCE(player, card))->damage_target_card;

  loop_player = target_player;
  for (loop_card = 0; loop_card < g_active_cards_count[loop_player]; ++loop_card)
  {
    instance = &PLAYER_CARD_INSTANCE(loop_player, loop_card);
    if (is_in_play(loop_player, loop_card) &&
        instance->internal_card_id == g_control_aura_legacy_internal_card_id &&
        instance->damage_target_player == target_player &&
        instance->damage_target_card == target_card &&
        (PLAYER_CARD_INSTANCE((int)instance->damage_source_player, instance->damage_source_card).state & STATE_TAPPED) == 0)
    {
      found_legacy = 1;
      saved_player = instance->targets[0].player;
      saved_card = instance->targets[0].card;
      instance->targets[0].player = player;
      instance->targets[0].card = card;
      instance->info_slot = 1;
    }
  }

  for (loop_player = 0; loop_player < 2; ++loop_player)
  {
    for (loop_card = 0; loop_card < g_active_cards_count[loop_player]; ++loop_card)
    {
      instance = &PLAYER_CARD_INSTANCE(loop_player, loop_card);
      if (is_in_play(loop_player, loop_card) &&
          (global_cards_data[instance->internal_card_id].id == 0x2c ||
           global_cards_data[instance->internal_card_id].id == 0xea ||
           instance->internal_card_id == g_control_aura_special_internal_card_id) &&
          instance->damage_target_player == target_player &&
          instance->damage_target_card == target_card &&
          (instance->token_status & 0x1000000) != 0)
      {
        if (found_legacy == 0)
        {
          instance->token_status &= ~0x1000000;
        }

        if (saved_player == -1)
        {
          (&PLAYER_CARD_INSTANCE(player, card))->damage_source_player = (int8_t)loop_player;
          (&PLAYER_CARD_INSTANCE(player, card))->damage_source_card = loop_card;
        }
        else
        {
          (&PLAYER_CARD_INSTANCE(player, card))->damage_source_player = (int8_t)saved_player;
          (&PLAYER_CARD_INSTANCE(player, card))->damage_source_card = saved_card;
        }
      }
    }
  }

  return !found_legacy;
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

// FUNCTION: MAGIC 0x004b4110
// FUNCTION: SHANDALAR 0x0056a37b
void exit_duel_thread(int player)
{
  abort_duel_run((DWORD)player);
}

// FUNCTION: MAGIC 0x004b5d72
// FUNCTION: SHANDALAR 0x0056c574
void real_put_on_top_of_deck(int player, int internal_card_id)
{
  int index;

  for (index = 499; index > 0; --index)
  {
    global_library[player][index] = global_library[player][index - 1];
  }
  global_library[player][0] = internal_card_id;
}

// FUNCTION: MAGIC 0x004b6002
// FUNCTION: SHANDALAR 0x0056c804
void show_duel_system_error(const char *err_msg)
{
#ifdef SHANDALAR
  assert_noabort(0, "D:\\Newmagic\\multiplayer\\sid\\Minit.c", 0x76b, err_msg);
#else
  char buf[500];

  if (g_duel_ai_mode_state != 1)
  {
    LoadTextSectionLines(global_ui_strings_filename, "PROMPT_SYSERR");
    sprintf(buf, g_text_lines[1], err_msg);
    MessageBoxA((HWND)g_duel_window_hwnd, buf, g_text_lines[0], 0);
  }
#endif
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
              (PLAYER_CARD_INSTANCE(player, card).token_status & 0x04000000) != 0)
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
