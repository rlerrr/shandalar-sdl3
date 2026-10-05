#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "cardartlib/src/palette.h"
#include "game_support.h"
#include "magic/src/duel_runtime.h"
#include "global_state.h"
#include "global_strings.h"

int GetNextManaSymbol(char **text);

extern HWND g_spell_minimized_hwnd;

// GLOBAL: MAGIC 0x007a7d74
// GLOBAL: SHANDALAR 0x007beb00
HWND g_duel_tooltip_window;

// GLOBAL: MAGIC 0x007aa928
// GLOBAL: SHANDALAR 0x007beb20
int g_saved_stop_phase_player;

// GLOBAL: MAGIC 0x007aaeec
// GLOBAL: SHANDALAR 0x007bf0ec
int g_response_prompt_enabled;

// GLOBAL: MAGIC 0x008a8d78
// GLOBAL: SHANDALAR 0x008bcf78
HWND g_duel_player_battlefield_window;

// GLOBAL: MAGIC 0x008a8dec
// GLOBAL: SHANDALAR 0x008bcfec
HWND g_duel_opponent_battlefield_window;

// GLOBAL: MAGIC 0x0093d840
// GLOBAL: SHANDALAR 0x00951960
int g_target_selection_active;

int show_spell_window_for_target_selection(void);
int restore_spell_window_after_target_selection(void);

// FUNCTION: MAGIC 0x0055d91b
// FUNCTION: SHANDALAR 0x00573c35
int get_text_width_with_mana_symbols(HDC dc, char *text)
{
  int char_width;
  int text_width;
  char mana_symbol;
  TEXTMETRICA text_metrics;
  ABC abc_widths;

  if (text == NULL)
  {
    return 0;
  }

  GetTextMetricsA(dc, &text_metrics);
  text_width = 0;
  while (*text != '\0')
  {
    mana_symbol = (char)GetNextManaSymbol(&text);
    if (mana_symbol != '\0')
    {
      text_width += text_metrics.tmHeight;
    }
    else
    {
      if (GetCharABCWidthsA(dc, (int)*text, (int)*text, &abc_widths))
      {
        text_width = abc_widths.abcB + abc_widths.abcC + abc_widths.abcA + text_width;
      }
      else
      {
        GetCharWidthA(dc, (int)*text, (int)*text, &char_width);
        text_width += char_width;
      }
      ++text;
    }
  }

  return text_width;
}

// FUNCTION: MAGIC 0x0049535a
// FUNCTION: SHANDALAR 0x00465df5
int get_tooltip_text_width(HWND window, char *text)
{
  HDC dc;
  int text_width;
  HGDIOBJ font;
  char window_text[200];
  TEXTMETRICA text_metrics;

  if (window == NULL)
  {
    return 0;
  }

  font = (HGDIOBJ)SendMessageA(window, 0x31, 0, 0);
  if (text != NULL)
  {
    strcpy(window_text, text);
  }
  else
  {
    GetWindowTextA(window, window_text, 200);
  }

  dc = GetDC(window);
  ApplyCardArtPaletteToDc(dc);
  if (font != NULL)
  {
    SelectObject(dc, font);
  }

  text_width = get_text_width_with_mana_symbols(dc, window_text);
  GetTextMetricsA(dc, &text_metrics);
  text_width += text_metrics.tmHeight * 3;
  ReleaseDC(window, dc);

  return text_width;
}

// FUNCTION: MAGIC 0x004480e4
// FUNCTION: SHANDALAR 0x00451c62
void set_duel_tooltip_text(char *text)
{
  int tooltip_height;
  POINT cursor_pos;
  int tooltip_width;
  int tooltip_y;
  int tooltip_x;
  int screen_width;
  int screen_height;
  char *tooltip_text;

  if (text == NULL)
  {
    tooltip_text = "";
  }
  else
  {
    tooltip_text = text;
  }

  if (g_duel_interface_options.layout == 0)
  {
    return;
  }

  if (*tooltip_text == '\0')
  {
    ShowWindow(g_duel_tooltip_window, 0);
    SetWindowTextA(g_duel_tooltip_window, tooltip_text);
  }
  else
  {
    screen_width = GetSystemMetrics(0);
    screen_height = GetSystemMetrics(1);
    if (g_duel_interface_options.directive_tracks_mouse != 0)
    {
      if ((GetSystemMetrics(1) * 2) / 100 > 0xc)
      {
        tooltip_height = (GetSystemMetrics(1) * 2) / 100;
      }
      else
      {
        tooltip_height = 0xc;
      }

      tooltip_width = get_tooltip_text_width(g_duel_tooltip_window, tooltip_text);
      tooltip_width += tooltip_height * 2;
      GetCursorPos(&cursor_pos);
      cursor_pos.y += GetSystemMetrics(0xe);
      if (screen_width < cursor_pos.x + tooltip_width)
      {
        cursor_pos.x = screen_width - tooltip_width;
      }
      if (screen_height < cursor_pos.y + tooltip_height)
      {
        cursor_pos.y = screen_height - tooltip_height;
      }
      tooltip_x = cursor_pos.x;
      tooltip_y = cursor_pos.y;
    }
    else
    {
      if ((GetSystemMetrics(1) * 3) / 100 > 0x12)
      {
        tooltip_height = (GetSystemMetrics(1) * 3) / 100;
      }
      else
      {
        tooltip_height = 0x12;
      }

      tooltip_width = get_tooltip_text_width(g_duel_tooltip_window, tooltip_text);
      tooltip_width += tooltip_height * 2;
      tooltip_x = (screen_width - (screen_width * 0x14) / 100) - tooltip_width;
      tooltip_y = (screen_height - tooltip_height) / 2;
    }

    SetWindowPos(g_duel_tooltip_window, (HWND)0, tooltip_x, tooltip_y, tooltip_width, tooltip_height, 4);
    SetWindowTextA(g_duel_tooltip_window, tooltip_text);
    ShowWindow(g_duel_tooltip_window, 5);
    BringWindowToTop(g_duel_tooltip_window);
  }
}

// FUNCTION: MAGIC 0x004466b5
// FUNCTION: SHANDALAR 0x00450233
int run_target_selection_modal(int who_chooses,
                               int player,
                               char *prompt,
                               int allow_cancel,
                               int type_flags,
                               int color_flags,
                               int owner,
                               int zone_flags,
                               int *out_selection_code,
                               target_t *out_target,
                               int allow_ai_player,
                               int allow_human_player)
{
  POINT cleanup_cursor_pos;                /* ebp - 0x3d4 */
  HWND cleanup_cursor_window;              /* ebp - 0x3cc */
  POINT cursor_pos;                        /* ebp - 0x3c8 */
  HWND cursor_window;                      /* ebp - 0x3c0 */
  char prompt_buffer[200];                 /* ebp - 0x3bc */
  char status_text[500];                   /* ebp - 0x2f4 */
  target_selection_result_t action_result; /* ebp - 0x100 */
  int result;                              /* ebp - 0xf0 */
  WPARAM thread_exit_code;                 /* ebp - 0xec */
  target_selection_request_t request;      /* ebp - 0xe8 */

  thread_exit_code = 0;
  if (prompt != NULL)
  {
    strcpy(prompt_buffer, prompt);
    strcpy(status_text, gs_waiting_for_opponent_007a7d30);
    strcat(status_text, prompt);
  }
  else
  {
    strcpy(prompt_buffer, "");
    strcpy(status_text, gs_waiting_for_opponent_007a7d30);
  }

  EnterCriticalSection(&g_duel_render_lock);
  if (g_saved_stop_phase != -1)
  {
    if ((g_duel_network_flags & 2) == 0)
    {
      g_saved_stop_phase = -1;
    }

    if (IsWindowVisible(g_duel_opponent_battlefield_window))
    {
      InvalidateRect(g_duel_opponent_battlefield_window, NULL, 1);
    }
    else
    {
      InvalidateRect(g_duel_player_battlefield_window, NULL, 1);
    }
  }
  LeaveCriticalSection(&g_duel_render_lock);

  TENTATIVE_reassess_all_cards(0, 0xff);
  GetCursorPos(&cursor_pos);
  cursor_window = WindowFromPoint(cursor_pos);
  SendMessageA(cursor_window, 0x20, (WPARAM)cursor_window, 0x2000001);

  request.player = player;
  request.type_flags = type_flags;
  request.color_flags = color_flags;
  request.owner = owner;
  request.zone_flags = zone_flags;
  request.allow_ai_player = allow_ai_player;
  request.allow_human_player = allow_human_player;
  strcpy(request.prompt, prompt_buffer);
  request.allow_cancel = allow_cancel;

  if (g_active_player == who_chooses || (g_duel_network_flags & 2) == 0)
  {
    if (g_target_selection_active != 0)
    {
      play_sound_effect(0x25);
      g_target_selection_active = 0;
    }

    if (g_response_prompt_enabled != 0 && g_current_phase == 10 && g_response_action_taken == 0)
    {
      result = 0;
      action_result.selection_code = -2;
      action_result.target_card = -2;
      action_result.target_player = -1;
      thread_exit_code = 0;
      g_stop_phase_player = -1;
      g_stop_phase = -1;
      g_recorded_action_player = 0;
      g_recorded_action_phase = 0;
      g_recorded_action_controller = 0;
    }
    else
    {
      result = SendMessageA(g_duel_window_hwnd, 0x403, (WPARAM)&request, (LPARAM)&action_result);
    }

    if ((g_duel_network_flags & 2) != 0)
    {
      g_target_selection_network_packet.packet_type = 0xc;
      g_target_selection_network_packet.result = result;
      g_target_selection_network_packet.selection_code = action_result.selection_code;
      g_target_selection_network_packet.thread_exit_code = 1 - thread_exit_code;
      if (g_stop_phase_player == -1)
      {
        g_target_selection_network_packet.previous_player = g_stop_phase_player;
      }
      else
      {
        g_target_selection_network_packet.previous_player = 1 - g_stop_phase_player;
      }
      g_target_selection_network_packet.previous_phase = g_stop_phase;
      g_target_selection_network_packet.target_card = action_result.target_card;
      if (result != 0)
      {
        g_target_selection_network_packet.target_player = 1 - action_result.target_player;
      }
      else
      {
        g_target_selection_network_packet.target_player = action_result.target_player;
      }
      g_target_selection_network_packet.aux_player = g_recorded_action_player;
      g_target_selection_network_packet.aux_phase = g_recorded_action_phase;
      g_target_selection_network_packet.aux_controller = 1 - g_recorded_action_controller;
      TENTATIVE_send_network_result(who_chooses, 0xc);
      Sleep(0xfa);
      send_battlefield_status_packet(who_chooses);
    }
  }
  else
  {
    set_duel_prompt_text(gs_waiting_for_opponent_007a7d30);
    g_target_selection_active = 1;
    TENTATIVE_wait_for_network_result(who_chooses, 0xc);
    result = g_target_selection_network_packet.result;
    action_result.selection_code = g_target_selection_network_packet.selection_code;
    g_previous_stop_phase_player = g_target_selection_network_packet.previous_player;
    g_previous_stop_phase = g_target_selection_network_packet.previous_phase;
    action_result.target_card = g_target_selection_network_packet.target_card;
    action_result.target_player = g_target_selection_network_packet.target_player;
    g_recorded_action_player = g_target_selection_network_packet.aux_player;
    g_recorded_action_phase = g_target_selection_network_packet.aux_phase;
    g_recorded_action_controller = g_target_selection_network_packet.aux_controller;
    thread_exit_code = (WPARAM)g_target_selection_network_packet.thread_exit_code;
    receive_battlefield_status_packet(who_chooses);
    set_duel_prompt_text("");
  }

  *out_selection_code = action_result.selection_code;
  out_target->player = action_result.target_player;
  out_target->card = action_result.target_card;
  set_duel_tooltip_text(NULL);

  if (action_result.selection_code == -5)
  {
    abort_duel_run((DWORD)thread_exit_code);
  }

  {
    GetCursorPos(&cleanup_cursor_pos);
    cleanup_cursor_window = WindowFromPoint(cleanup_cursor_pos);
    SendMessageA(cleanup_cursor_window, 0x20, (WPARAM)cleanup_cursor_window, 0x2000001);
    EnterCriticalSection(&g_duel_render_lock);
    if (g_target_selection_status_code == -2)
    {
      if (g_stop_phase == -1)
      {
        if ((g_duel_network_flags & 2) != 0)
        {
          g_saved_stop_phase_player = g_stop_phase_player;
          g_saved_stop_phase = g_stop_phase;
        }
        else
        {
          g_saved_stop_phase_player = g_saved_stop_phase = -1;
        }
      }
      else
      {
        g_saved_stop_phase_player = g_stop_phase_player;
        g_saved_stop_phase = g_stop_phase;
      }
    }
    else if ((g_duel_network_flags & 2) != 0)
    {
      g_saved_stop_phase_player = g_stop_phase_player;
      g_saved_stop_phase = g_stop_phase;
    }
    else
    {
      g_saved_stop_phase_player = g_saved_stop_phase = -1;
    }

    if (IsWindowVisible(g_duel_opponent_battlefield_window))
    {
      InvalidateRect(g_duel_opponent_battlefield_window, NULL, 1);
    }
    else
    {
      InvalidateRect(g_duel_player_battlefield_window, NULL, 1);
    }
    LeaveCriticalSection(&g_duel_render_lock);
    return result;
  }
}

// FUNCTION: MAGIC 0x004c0080
// FUNCTION: SHANDALAR 0x004c0efa
int C_real_select_target(int who_chooses,
                         int allowed_controller,
                         int preferred_controller,
                         int zone,
                         int required_type,
                         int illegal_type,
                         unsigned int required_abilities,
                         unsigned int illegal_abilities,
                         unsigned int required_color,
                         unsigned int illegal_color,
                         int extra,
                         int required_subtype,
                         int power_requirement,
                         int toughness_requirement,
                         unsigned int special,
                         unsigned int required_state,
                         unsigned int illegal_state,
                         char *prompt,
                         int allow_cancel,
                         target_t *ret_tgt)
{
  char *selection_prompt;
  int selection_code;
  target_t selected_target;
  int show_validation_message;
  char validation_message[200];
  char target_error[200];
  int retry_selection;
  int selected_internal_card_id;
  int valid_cards[60];
  int valid_count;
  int current_card;
  int current_player;
  int valid_players[60];
  int who_chooses_copy;
  int required_color_copy;
  int normalized_preferred_controller;
  int required_type_copy;
  int normalized_allowed_controller;
  int allow_ai_player;
  int result;
  int window_was_visible;
  int allow_human_player;

  if (((who_chooses == 1 && (g_duel_network_flags & 2) == 0) || g_duel_ai_mode_state == 1) || g_duel_network_state != 0)
  {
    if ((g_ai_search_flags & 1) != 0 && (allowed_controller & 2) != 0)
    {
      preferred_controller = allowed_controller;
    }

    who_chooses_copy = who_chooses;
    if ((allowed_controller & 2) != 0)
    {
      normalized_allowed_controller = -1;
    }
    else
    {
      normalized_allowed_controller = allowed_controller & 1;
    }
    if ((preferred_controller & 2) != 0)
    {
      normalized_preferred_controller = -1;
    }
    else
    {
      normalized_preferred_controller = preferred_controller & 1;
    }
    required_type_copy = required_type;
    required_color_copy = required_color;

    if (zone == 0 || (zone & TARGET_ZONE_PLAYERS) != 0)
    {
      if ((preferred_controller & 2) != 0)
      {
        allow_human_player = 1;
        allow_ai_player = 1;
      }
      else
      {
        if ((preferred_controller & 1) != 0)
        {
          allow_human_player = 0;
          allow_ai_player = 1;
        }
        else
        {
          allow_human_player = 1;
          allow_ai_player = 0;
        }
      }
    }
    else
    {
      allow_human_player = 0;
      allow_ai_player = 0;
    }

    if (g_spell_fizzled == 1)
    {
      return 0;
    }

    valid_count = 0;
    for (current_player = 0; current_player < 2; ++current_player)
    {
      for (current_card = 0; current_card < g_active_cards_count[current_player]; ++current_card)
      {
        if (PLAYER_CARD_INSTANCE(current_player, current_card).internal_card_id != -1)
        {
          if (C_real_validate_target(current_player,
                                     current_card,
                                     NULL,
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
                                     power_requirement,
                                     toughness_requirement,
                                     special,
                                     required_state,
                                     illegal_state) != 0)
          {
            valid_players[valid_count] = current_player;
            valid_cards[valid_count] = current_card;
            ++valid_count;
          }
        }
      }
    }

    if (allow_ai_player != 0)
    {
      valid_players[valid_count] = 1;
      valid_cards[valid_count] = -1;
      ++valid_count;
    }
    if (allow_human_player != 0)
    {
      valid_players[valid_count] = 0;
      valid_cards[valid_count] = -1;
      ++valid_count;
    }

    if (valid_count == 0)
    {
      return 0;
    }

    if (g_duel_ai_mode_state == 1)
    {
      g_ai_recorded_choice = internal_rand(valid_count);
      g_ai_recorded_action = (((valid_players[g_ai_recorded_choice] == 0) ? 0 : 0x100) | (valid_cards[g_ai_recorded_choice] & 0xff)) | 0x4000;
      g_ai_recorded_action_type = 3;
      record_ai_action_selection();
    }
    else
    {
      g_ai_recorded_action_type = 3;
      replay_ai_action_selection();
      if (g_shandalar_difficulty == 0)
      {
        // Preserve the original buggy index-based replay on the easiest difficulty.
        if (g_ai_recorded_choice == 99 || valid_count <= g_ai_recorded_choice)
        {
          g_ai_recorded_choice = internal_rand(valid_count);
        }
      }
      else
      {
        if (g_ai_recorded_choice == 99 || g_ai_recorded_action == -1 ||
            (g_ai_recorded_action & 0x4000) == 0)
        {
          g_spell_fizzled = 1;
          return 0;
        }

        // Replay the target itself: the candidate list can differ from the trial.
        selected_target.player = (g_ai_recorded_action >> 8) & 1;
        selected_target.card = g_ai_recorded_action & 0xff;
        if (selected_target.card == 0xff)
        {
          selected_target.card = -1;
        }
        for (current_card = 0; current_card < valid_count; ++current_card)
        {
          if (valid_players[current_card] == selected_target.player &&
              valid_cards[current_card] == selected_target.card)
          {
            break;
          }
        }
        if (current_card == valid_count)
        {
          g_spell_fizzled = 1;
          return 0;
        }
        g_ai_recorded_choice = current_card;
      }
    }

    g_target_player_choice = valid_players[g_ai_recorded_choice];
    ret_tgt->player = valid_players[g_ai_recorded_choice];
    ret_tgt->card = valid_cards[g_ai_recorded_choice];
    result = 1;
    return result;
  }
  else
  {
    if (zone == 0 || (zone & TARGET_ZONE_PLAYERS) != 0)
    {
      if ((allowed_controller & 2) != 0)
      {
        allow_human_player = 1;
        allow_ai_player = 1;
      }
      else
      {
        if ((allowed_controller & 1) != 0)
        {
          allow_human_player = 0;
          allow_ai_player = 1;
        }
        else
        {
          allow_human_player = 1;
          allow_ai_player = 0;
        }
      }
    }
    else
    {
      allow_human_player = 0;
      allow_ai_player = 0;
    }

    window_was_visible = show_spell_window_for_target_selection();
    retry_selection = 1;
    while (retry_selection != 0)
    {
      if (prompt != NULL)
      {
        selection_prompt = prompt;
      }
      else
      {
        selection_prompt = g_ui_message_buffer;
      }

      result = run_target_selection_modal(who_chooses,
                                          -1,
                                          selection_prompt,
                                          allow_cancel,
                                          -1,
                                          -1,
                                          -1,
                                          -1,
                                          &selection_code,
                                          &selected_target,
                                          allow_ai_player,
                                          allow_human_player);
      if (result != 0)
      {
        selected_internal_card_id = PLAYER_CARD_INSTANCE(selected_target.player, selected_target.card).internal_card_id;
        if (selected_internal_card_id >= 0 && (special & TARGET_SPECIAL_ALLOW_MULTIBLOCKER) == 0 && global_cards_data[selected_internal_card_id].code_pointer == card_two_headed_giant_of_foriys_legacy)
        {
          show_validation_message = 1;
          sprintf(validation_message, gs_illegal_target_with_reason_008ce7e0, gs_illegal_target_why_type_008a9a00);
          if (g_duel_ai_mode_state != 1)
          {
            set_duel_prompt_text(validation_message);
            Sleep(2000);
            set_duel_prompt_text("");
          }
        }
        else
        {
          if (C_real_validate_target(selected_target.player,
                                     selected_target.card,
                                     target_error,
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
                                     power_requirement,
                                     toughness_requirement,
                                     special,
                                     required_state,
                                     illegal_state) != 0)
          {
            show_validation_message = 0;
            ret_tgt->player = selected_target.player;
            ret_tgt->card = selected_target.card;
            retry_selection = 0;
          }
          else
          {
            show_validation_message = 1;
            if (strlen(target_error) != 0)
            {
              sprintf(validation_message, gs_illegal_target_with_reason_008ce7e0, target_error);
            }
            else
            {
              strcpy(validation_message, gs_illegal_target_008b3e50);
            }

            if (g_duel_ai_mode_state != 1)
            {
              set_duel_prompt_text(validation_message);
              Sleep(2000);
              set_duel_prompt_text("");
            }
          }
        }
      }
      else
      {
        if (selection_code == -3)
        {
        }
        else if (selection_code == -2)
        {
          if (g_stop_phase_player == -1 && g_stop_phase == -1)
          {
            ret_tgt->player = selected_target.player;
            ret_tgt->card = selected_target.card;
            retry_selection = 0;
          }
          else if ((zone & TARGET_ZONE_0x2000) != 0)
          {
            ret_tgt->player = -1;
            ret_tgt->card = -3;
            retry_selection = 0;
          }
        }
      }
    }

    if (window_was_visible == 0)
    {
      restore_spell_window_after_target_selection();
    }
    if (result != 0)
    {
      g_ai_action_replay_available = 0;
    }
    set_duel_prompt_text("");
    strcpy(g_text_lines[0], "");
  }

  return result;
}

// FUNCTION: MAGIC 0x0048930a
// FUNCTION: SHANDALAR 0x004cbed2
int show_spell_window_for_target_selection(void)
{
  int visible;

  visible = IsWindowVisible(g_spell_minimized_hwnd);
  if (!visible && IsWindowVisible((HWND)g_duel_phase_display_window_hwnd))
  {
    SendMessageA((HWND)g_duel_phase_display_window_hwnd, 0x111, 0x65, 0);
  }

  return visible;
}

// FUNCTION: MAGIC 0x00489362
// FUNCTION: SHANDALAR 0x004cbf2a
int restore_spell_window_after_target_selection(void)
{
  int hidden;

  hidden = !IsWindowVisible(g_spell_minimized_hwnd);
  if (!hidden && !IsWindowVisible((HWND)g_duel_phase_display_window_hwnd))
  {
    SendMessageA((HWND)g_duel_phase_display_window_hwnd, 0x111, 0x66, 0);
  }

  return hidden;
}
