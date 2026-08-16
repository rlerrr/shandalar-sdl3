#include <windows.h>
#include <stdlib.h>
#include <string.h>

#include "cardartlib/src/palette.h"
#include "magic/src/duel_engine.h"
#include "magic/src/game_support.h"
#include "magic/src/global_duel_ui_ids.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "platform/winuser_imgui_bridge.h"

extern int load_text_with_tab_escapes(char *filename, char *section_name);
extern LRESULT handle_duel_inactive_cursor(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
extern void delete_and_close_object(HANDLE obj);
extern LONG get_card_window_hidden_flag(HWND hwnd);
extern int global_available_slots;
extern HWND g_duel_help_owner_hwnd;
extern int g_showlist_smallcard_width;
extern int g_showlist_smallcard_height;
extern int g_duel_window_userdata_player_offset;
extern int g_duel_window_userdata_card_offset;
extern int g_duel_selected_card_window;
extern card_ptr_t global_raw_cards_storage[2000];
extern int g_duel_modal_action_active;
extern target_selection_request_t g_duel_action_request_copy;
extern HWND g_duel_toggleable_status_window_hwnd;
extern int g_duel_mode_flags;
extern int g_duel_network_flags;
extern int g_shared_startup_completed;
extern int g_response_prompt_enabled;
extern char global_base_directory[];
extern char g_graveyard_menu_view_exile_text[];
extern char g_graveyard_menu_view_antes_text[];
int copy_cached_exile_cards_and_get_count(void *cards, int player);
card_id_t get_displayed_card_id(int player, int card);
unsigned int get_displayed_card_ui_flags(int player, int card);
void get_displayed_card_attachment(int *player_and_card, int player, int card);
int find_battlefield_card_window(HWND hwnd, int *player_and_card, int *unused, HWND *child_hwnd);
int get_battlefield_card_stagger_offset(HWND hwnd);
void reset_battlefield_layout_positions(HWND hwnd);
void get_next_battlefield_card_position(HWND parent, int *rect, int value, int *x, int *y, int flag);
int count_hidden_battlefield_descendants(HWND hwnd, HWND hidden_parent);
int get_displayed_card_blocking(int player, int card);
void get_current_duel_selection(int *selected_player, int *selected_card);
void get_next_phase_stop(int *player, phase_t *phase, char *unused);
void show_territory_options_dialog(HWND hwnd);
void refresh_duel_window(HWND hwnd);
void set_player_directive_value(int player, int value);
void layout_duel_child_windows(HWND hwnd, int layout);
void position_duel_prompt_context_window(HWND hwnd);
void resize_duel_hand_window(HWND hwnd);
void save_duel_interface_options_to_registry(void);
void apply_duel_backdrop_art(int player, int color, int variant);
INT_PTR CALLBACK dlgproc_duel_interface_options(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

#ifndef SHANDALAR
extern HWND global_main_hwnd;
#define DUEL_MAIN_WINDOW_HWND global_main_hwnd
#else
#define DUEL_MAIN_WINDOW_HWND g_main_window_hwnd
#endif

extern int g_cardclass_player_window_long_offset;
extern int g_cardclass_card_window_long_offset;
extern int g_cardclass_hidden_parent_window_long_offset;
extern int g_cardclass_snapshot_window_long_offset;
extern int g_duel_window_userdata_snapshot_offset;
extern int g_duel_window_userdata_8_offset;

// GLOBAL: MAGIC 0x0055e178
// GLOBAL: SHANDALAR 0x0057f108
int g_magicgame_territory_window_extra_bytes = 0x10;

// GLOBAL: MAGIC 0x006ab370
// GLOBAL: SHANDALAR 0x00602fc0
char g_territory_menu_default_action_text[0x68];
// GLOBAL: MAGIC 0x006ab3d8
// GLOBAL: SHANDALAR 0x00603028
char g_territory_menu_action_20_text[0x68];
// GLOBAL: MAGIC 0x006ab548
// GLOBAL: SHANDALAR 0x00603198
char g_territory_menu_action_21_text[0x68];
// GLOBAL: MAGIC 0x006ab440
// GLOBAL: SHANDALAR 0x00603090
char g_territory_menu_action_22_text[0x68];
// GLOBAL: MAGIC 0x006ab4a8
// GLOBAL: SHANDALAR 0x006030f8
char g_territory_menu_action_23_text[0x64];
// GLOBAL: MAGIC 0x006abb10
// GLOBAL: SHANDALAR 0x00603760
char g_territory_menu_action_24_text[0x68];
// GLOBAL: MAGIC 0x006ab760
// GLOBAL: SHANDALAR 0x006033b0
char g_territory_menu_action_25_text[0x68];
// GLOBAL: MAGIC 0x006abaa8
// GLOBAL: SHANDALAR 0x006036f8
char g_territory_menu_end_turn_text[0x68];
// GLOBAL: MAGIC 0x006ab868
// GLOBAL: SHANDALAR 0x006034b8
char g_territory_menu_action_1_text[0x68];
// GLOBAL: MAGIC 0x006ab690
// GLOBAL: SHANDALAR 0x006032e0
char g_territory_menu_action_30_text[0x68];
// GLOBAL: MAGIC 0x006ab6f8
// GLOBAL: SHANDALAR 0x00603348
char g_territory_menu_action_31_text[0x68];
// GLOBAL: MAGIC 0x006ab7c8
// GLOBAL: SHANDALAR 0x00603418
char g_territory_menu_main_window_text[0x38];
// GLOBAL: MAGIC 0x006ab938
// GLOBAL: SHANDALAR 0x00603588
char g_territory_menu_submenu_text[0x38];
// GLOBAL: MAGIC 0x006ab50c
// GLOBAL: SHANDALAR 0x0060315c
HMENU g_territory_submenu;
// GLOBAL: MAGIC 0x006ab618
// GLOBAL: SHANDALAR 0x00603268
char g_territory_menu_action_2_to_5_text[0x78];
// GLOBAL: MAGIC 0x006ab864
// GLOBAL: SHANDALAR 0x006034b4
HMENU g_territory_popup_menu;
// GLOBAL: MAGIC 0x006ab970
// GLOBAL: SHANDALAR 0x006035c0
char g_territory_menu_action_10_text[0x68];
// GLOBAL: MAGIC 0x006abd80
// GLOBAL: SHANDALAR 0x006039d0
char g_territory_menu_your_battlefield_text[0x68];
// GLOBAL: MAGIC 0x006ab800
// GLOBAL: SHANDALAR 0x00603450
char g_territory_menu_skip_full_card_text[2][0x32];
// GLOBAL: MAGIC 0x006ab8d0
// GLOBAL: SHANDALAR 0x00603520
char g_territory_menu_draw_response_text[2][0x32];
// GLOBAL: MAGIC 0x006ab9d8
// GLOBAL: SHANDALAR 0x00603628
char g_territory_menu_action_19_1a_text[0x68];
// GLOBAL: MAGIC 0x006aba40
// GLOBAL: SHANDALAR 0x00603690
char g_territory_menu_action_1b_text[0x68];
// GLOBAL: MAGIC 0x006ab5b0
// GLOBAL: SHANDALAR 0x00603200
char g_territory_menu_show_options_text[0x68];
// GLOBAL: MAGIC 0x006abb78
// GLOBAL: SHANDALAR 0x006037c8
char g_territory_menu_action_18_text[0x68];
// GLOBAL: MAGIC 0x006abbe0
// GLOBAL: SHANDALAR 0x00603830
char g_territory_menu_action_6f_text[0x68];
// GLOBAL: MAGIC 0x006abc48
// GLOBAL: SHANDALAR 0x00603898
char g_territory_menu_action_69_text[0x68];
// GLOBAL: MAGIC 0x006ab510
// GLOBAL: SHANDALAR 0x00603160
char g_territory_menu_action_68_text[0x38];
// GLOBAL: MAGIC 0x006abcb0
// GLOBAL: SHANDALAR 0x00603900
char g_territory_menu_opponent_battlefield_text[0x68];
// GLOBAL: MAGIC 0x006abd18
// GLOBAL: SHANDALAR 0x00603968
char g_territory_menu_action_20_25_text[0x68];

// GLOBAL: MAGIC 0x006ab614
// GLOBAL: SHANDALAR 0x00603264
HMENU g_territory_debug_menu_a;

// GLOBAL: MAGIC 0x006ab96c
// GLOBAL: SHANDALAR 0x006035bc
HMENU g_territory_debug_menu_b;

// GLOBAL: MAGIC 0x006ab680
// GLOBAL: SHANDALAR 0x006032d0
int g_territory_command_packet[3];

// GLOBAL: MAGIC 0x00789720
// GLOBAL: SHANDALAR 0x007a04b0
char g_custom_duel_action_text[0x100];

// FUNCTION: MAGIC 0x00538e3d
// FUNCTION: SHANDALAR 0x0055d661
void get_next_phase_stop(int *player, phase_t *phase, char *text)
{
  int switch_phase;
  int next_player;
  char phase_text[100];
  int current_player;
  phase_t next_phase;
  int current_phase;

  get_current_duel_selection(&current_phase, &current_player);
  next_player = current_player;
  switch_phase = current_phase;
  switch (next_player)
  {
  case 1:
    next_phase = 4;
    strcpy(phase_text, "Upkeep phase");
    break;
  case 2:
  case 3:
  case 4:
  case 5:
    next_phase = 10;
    strcpy(phase_text, "Draw phase");
    break;
  case 10:
    next_phase = 0x14;
    strcpy(phase_text, "Main phase (pre-combat)");
    break;
  case 0x14:
    next_phase = 0x15;
    strcpy(phase_text, "Main phase (combat)");
    break;
  case 0x15:
    next_phase = 0x16;
    strcpy(phase_text, "Attack fast effects phase");
    break;
  case 0x16:
    next_phase = 0x17;
    strcpy(phase_text, "Choose defenders phase");
    break;
  case 0x17:
    next_phase = 0x18;
    strcpy(phase_text, "Block fast effects phase");
    break;
  case 0x18:
    next_phase = 0x19;
    strcpy(phase_text, "Resolve 1st strike");
    break;
  case 0x19:
  case 0x1a:
    next_phase = 0x1b;
    strcpy(phase_text, "Resolve attack");
    break;
  case 0x1b:
    next_phase = 0x1e;
    strcpy(phase_text, "Main phase (post-combat)");
    break;
  case 0x1e:
    next_phase = 0x1f;
    strcpy(phase_text, "Discard phase");
    break;
  case 0x1f:
    next_phase = 0x20;
    strcpy(phase_text, "Cleanup phase");
    break;
  case 0x20:
  case 0x21:
  case 0x22:
  case 0x25:
    next_phase = 0;
    switch_phase = 1 - current_phase;
    strcpy(phase_text, "Start of next turn");
    break;
  default:
    next_phase = -1;
    strcpy(phase_text, "next phase");
    break;
  }

  if (player != NULL)
  {
    *player = switch_phase;
  }
  if (phase != NULL)
  {
    *phase = next_phase;
  }
  if (text != NULL)
  {
    strcpy(text, phase_text);
  }
}

// FUNCTION: MAGIC 0x004893e0
// FUNCTION: SHANDALAR 0x004a5070
void show_territory_options_dialog(HWND hwnd)
{
  if (DialogBoxParamA(g_app_instance, (LPCSTR)0xe1, hwnd, dlgproc_duel_interface_options, 0) != 0)
  {
    if (g_duel_interface_options.player_territory_color == -1)
    {
      apply_duel_backdrop_art(0, g_last_duel_player_primary_color, g_duel_interface_options.player_territory_type);
    }
    else
    {
      apply_duel_backdrop_art(0, g_duel_interface_options.player_territory_color, g_duel_interface_options.player_territory_type);
    }
    LockWindowUpdate(g_duel_window_hwnd);
    set_player_directive_value(0, 0);
    set_player_directive_value(1, 0);
    layout_duel_child_windows(g_duel_window_hwnd, g_duel_interface_options.layout);
    position_duel_prompt_context_window(g_duel_prompt_context_hwnd);
    refresh_duel_window(g_duel_player_battlefield_window_hwnd);
    refresh_duel_window(g_duel_help_owner_hwnd);
    resize_duel_hand_window(g_duel_full_card_window_hwnd);
    resize_duel_hand_window(g_duel_life_window_hwnd);
    LockWindowUpdate((HWND)0);
    SendMessageA(g_duel_full_card_window_hwnd, 0x435, 0, 0);
    SendMessageA(g_duel_life_window_hwnd, 0x435, 0, 0);
    SendMessageA(g_duel_player_battlefield_window_hwnd, 0x435, 0, 0);
    SendMessageA(g_duel_help_owner_hwnd, 0x435, 0, 0);
    SendMessageA(g_duel_attack_phase_window_hwnd, 0x435, 0, 0);
    SendMessageA(g_duel_phase_display_window_hwnd, 0x435, 0, 0);
    save_duel_interface_options_to_registry();
  }
}

// FUNCTION: MAGIC 0x004d2495
// FUNCTION: SHANDALAR 0x00578a71
int card_window_matches_player_and_card(HWND hwnd, int *player_and_card)
{
  LONG player;
  LONG card;

  if (hwnd == (HWND)0 || player_and_card == NULL)
  {
    return 0;
  }

  player = GetWindowLongA(hwnd, g_cardclass_player_window_long_offset);
  card = GetWindowLongA(hwnd, g_cardclass_card_window_long_offset);
  if (player_and_card[0] == player && player_and_card[1] == card)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// FUNCTION: MAGIC 0x004d2517
// FUNCTION: SHANDALAR 0x00578af3
int card_window_matches_card_id(HWND hwnd, card_id_t card_id)
{
  LONG player;
  LONG card;
  card_id_t displayed_card_id;

  if (hwnd == (HWND)0 || card_id < 0)
  {
    return 0;
  }

  player = GetWindowLongA(hwnd, g_cardclass_player_window_long_offset);
  card = GetWindowLongA(hwnd, g_cardclass_card_window_long_offset);
  displayed_card_id = get_displayed_card_id(player, card);
  if (displayed_card_id == card_id)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// FUNCTION: MAGIC 0x004d259b
// FUNCTION: SHANDALAR 0x00578b77
card_id_t get_card_window_displayed_card_id(HWND hwnd)
{
  LONG player;
  LONG card;
  card_id_t card_id;

  if (hwnd == (HWND)0)
  {
    return -1;
  }

  player = GetWindowLongA(hwnd, g_cardclass_player_window_long_offset);
  card = GetWindowLongA(hwnd, g_cardclass_card_window_long_offset);
  card_id = get_displayed_card_id(player, card);
  return card_id;
}

// FUNCTION: MAGIC 0x004e9a76
// FUNCTION: SHANDALAR 0x00507883
int count_hidden_battlefield_descendants_for_card(HWND hwnd, int *player_and_card)
{
  int card_count;
  int loop_index;
  HWND *card_windows;
  HWND found_window;

  card_windows = (HWND *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
  card_count = GetWindowLongA(hwnd, g_duel_window_userdata_card_offset);
  found_window = (HWND)0;
  for (loop_index = 0; loop_index < card_count; loop_index++)
  {
    if (card_window_matches_player_and_card(card_windows[loop_index], player_and_card) != 0)
    {
      found_window = card_windows[loop_index];
    }
  }
  if (found_window != (HWND)0)
  {
    return count_hidden_battlefield_descendants(hwnd, found_window);
  }
  else
  {
    return 0;
  }
}

// FUNCTION: MAGIC 0x004e9b26
// FUNCTION: SHANDALAR 0x00507933
int count_hidden_battlefield_descendants(HWND hwnd, HWND hidden_parent)
{
  HWND *card_windows;
  int card_count;
  int descendant_count;
  int loop_index;

  card_windows = (HWND *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
  card_count = GetWindowLongA(hwnd, g_duel_window_userdata_card_offset);
  descendant_count = 0;
  for (loop_index = 0; loop_index < card_count; loop_index++)
  {
    if ((HWND)get_card_window_hidden_flag(card_windows[loop_index]) == hidden_parent)
    {
      descendant_count += 1 + count_hidden_battlefield_descendants(hwnd, card_windows[loop_index]);
    }
  }

  return descendant_count;
}

// FUNCTION: MAGIC 0x004e6a30
// FUNCTION: SHANDALAR 0x00504850
int register_MAGICGAME_TerritoryClass(LPCSTR class_name)
{
  int atom;
  WNDCLASSA wndclass;
  atom = 1;
  SET_DUEL_WNDCLASS(wndclass, 0xb, wndproc_MAGICGAME_TerritoryClass, g_magicgame_territory_window_extra_bytes,
                    (HICON)0, (HBRUSH)0, class_name);
  if (RegisterClassA(&wndclass) == 0)
    atom = 0;

  g_territory_popup_menu = CreatePopupMenu();
  load_text_with_tab_escapes(global_ui_strings_filename, "MENU_TERRITORY");
  strcpy(g_territory_menu_action_1_text, g_text_lines[0]);
  strcpy(g_territory_menu_action_2_to_5_text, g_text_lines[1]);
  strcpy(g_territory_menu_action_10_text, g_text_lines[2]);
  strcpy(g_territory_menu_action_20_text, g_text_lines[3]);
  strcpy(g_territory_menu_action_21_text, g_text_lines[4]);
  strcpy(g_territory_menu_action_22_text, g_text_lines[5]);
  strcpy(g_territory_menu_action_23_text, g_text_lines[6]);
  strcpy(g_territory_menu_action_24_text, g_text_lines[7]);
  strcpy(g_territory_menu_action_19_1a_text, g_text_lines[8]);
  strcpy(g_territory_menu_action_1b_text, g_text_lines[9]);
  strcpy(g_territory_menu_action_30_text, g_text_lines[10]);
  strcpy(g_territory_menu_action_31_text, g_text_lines[0xb]);
  strcpy(g_territory_menu_action_20_25_text, g_text_lines[0xc]);
  strcpy(g_territory_menu_default_action_text, g_text_lines[0xd]);
  strcpy(g_territory_menu_your_battlefield_text, g_text_lines[0xe]);
  strcpy(g_territory_menu_opponent_battlefield_text, g_text_lines[0xf]);
  strcpy(g_territory_menu_action_18_text, g_text_lines[0x10]);
  strcpy(g_territory_menu_action_69_text, g_text_lines[0x11]);
  strcpy(g_territory_menu_show_options_text, g_text_lines[0x12]);
  strcpy(g_territory_menu_end_turn_text, g_text_lines[0x13]);
  strcpy(g_territory_menu_action_68_text, g_text_lines[0x14]);
  strcpy(g_territory_menu_action_6f_text, g_text_lines[0x15]);
  strcpy(g_territory_menu_main_window_text, g_text_lines[0x16]);
  strcpy(g_territory_menu_submenu_text, g_text_lines[0x17]);
  strcpy(g_territory_menu_action_25_text, g_text_lines[0x18]);
  g_territory_submenu = CreatePopupMenu();
  AppendMenuA(g_territory_submenu, MF_STRING, 0x6c, g_territory_menu_action_25_text);
  load_text_with_tab_escapes("MP_UISTRINGS.TXT", "SKIPFULLCARD");
  strcpy(g_territory_menu_skip_full_card_text[0], g_text_lines[0]);
  strcpy(g_territory_menu_skip_full_card_text[1], g_text_lines[1]);
  load_text_with_tab_escapes("MP_UISTRINGS.TXT", "DRAWRESPONSE");
  strcpy(g_territory_menu_draw_response_text[0], g_text_lines[0]);
  strcpy(g_territory_menu_draw_response_text[1], g_text_lines[1]);
  return atom;
}

// FUNCTION: MAGIC 0x004e6dc8
// FUNCTION: SHANDALAR 0x00504be8
void destroy_MAGICGAME_TerritoryClass(LPCSTR class_name)
{
  if (g_territory_popup_menu != (HMENU)0)
  {
    DestroyMenu(g_territory_popup_menu);
  }
  if (g_territory_submenu != (HMENU)0)
  {
    DestroyMenu(g_territory_submenu);
  }
  g_territory_popup_menu = (HMENU)0;
  g_territory_submenu = (HMENU)0;
}

// FUNCTION: MAGIC 0x004e6e19
// FUNCTION: SHANDALAR 0x00504c39
LRESULT CALLBACK wndproc_MAGICGAME_TerritoryClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  int menu_item_count;
  char menu_text[100];
  int action_code;
  int territory_player;
  POINT popup_point;
  RECT popup_rect;
  HDC erase_dc;
  RECT erase_rect;
  unsigned int create_data[2];
  char help_path[264];
  DWORD help_context;
  int descendant_count;
  RECT resize_rect;
  int scan_player_and_card[2];
  int card_windows_copy[200];
  int unique_flagged_count;
  int unique_inner_index;
  int unique_outer_index;
  int unique_flag_id;
  int menu_exile_cards[500];
  int show_index;
  int hidden_index;
  HWND hidden_parent;
  unsigned int hidden_lparam;
  int attachment_y_step;
  int attachment_x_step;
  int attachment_descendant_count;
  int attachment_index;
  int attachment_y;
  int attachment_x;
  HWND attachment_parent;
  RECT attachment_rect;
  HWND previous_attachment_window;
  int attached_player_and_card[2];
  int displayed_player_and_card[2];
  card_id_t displayed_card_id;
  HWND other_battlefield_window_400;
  int loop_index_400;
  HWND attached_window_400;
  int *raise_player_and_card;
  int raise_index;
  int raise_found;
  int query_index;
  int *query_player_and_card;
  int query_value;
  int query_found;
  int destroy_index;
  int *remove_player_and_card;
  int remove_compact_index;
  int remove_loop_index;
  int remove_found_40b;
  int y;
  int x;
  HWND found_window;
  int *player_and_card;
  HWND attached_window;
  HWND other_battlefield_window;
  int loop_index;
  int remove_found;
  int remove_index;
  int hidden_flag;
  HWND child_window;
  int card_id_index;
  int card_count;
  void *card_windows;
  HWND background_bitmap;
  HWND player_card_window;

  switch (msg)
  {
  case 0x437:
    return 0;

  case 0x433:
  case 0x434:
    card_windows = (void *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
    card_count = GetWindowLongA(hwnd, g_duel_window_userdata_card_offset);
    child_window = (HWND)wparam;
    for (card_id_index = 0; card_count > card_id_index; card_id_index++)
    {
      if (card_window_matches_card_id(((HWND *)card_windows)[card_id_index], (int)child_window) != 0)
      {
        InvalidateRect(((HWND *)card_windows)[card_id_index], NULL, FALSE);
      }
    }
    return 0;

  case 0x432:
    card_windows = (void *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
    card_count = GetWindowLongA(hwnd, g_duel_window_userdata_card_offset);
    for (hidden_flag = 0; card_count > hidden_flag; hidden_flag++)
    {
      if (IsWindowVisible(((HWND *)card_windows)[hidden_flag]) != 0)
      {
        SendMessageA(((HWND *)card_windows)[hidden_flag], 0x432, 0, 0);
      }
    }
    return 0;

  case 0x435:
    card_windows = (void *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
    card_count = GetWindowLongA(hwnd, g_duel_window_userdata_card_offset);
    for (remove_index = 0; card_count > remove_index; remove_index++)
    {
      InvalidateRect(((HWND *)card_windows)[remove_index], NULL, FALSE);
    }
    return 0;

  case 0x436:
    card_windows = (void *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
    card_count = GetWindowLongA(hwnd, g_duel_window_userdata_card_offset);
    other_battlefield_window = (HWND)wparam;
    attached_window = (HWND)lparam;
    remove_found = 0;
    for (loop_index = 0;
         card_count > loop_index && remove_found == 0;
         loop_index++)
    {
      if (card_window_matches_player_and_card(((HWND *)card_windows)[loop_index],
                                              (int *)other_battlefield_window) != 0)
      {
        remove_found = 1;
        if (attached_window != (HWND)0)
        {
          SendMessageA(((HWND *)card_windows)[loop_index], 0x432, 0, 0);
        }
        else
        {
          InvalidateRect(((HWND *)card_windows)[loop_index], NULL, FALSE);
        }
      }
    }
    return 0;

  case 0x439:
    background_bitmap = (HWND)GetWindowLongA(hwnd, g_duel_window_userdata_8_offset);
    if (background_bitmap != (HWND)0)
    {
      DeleteObject(background_bitmap);
    }
    background_bitmap = (HWND)wparam;
    SetWindowLongA(hwnd, g_duel_window_userdata_8_offset, (LONG)background_bitmap);
    InvalidateRect(hwnd, NULL, TRUE);
    return 0;

  case 0x438:
    background_bitmap = (HWND)GetWindowLongA(hwnd, g_duel_window_userdata_8_offset);
    return (LRESULT)background_bitmap;

  case 0x40a:
    card_windows = (void *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
    card_count = GetWindowLongA(hwnd, g_duel_window_userdata_card_offset);
    if (card_count >= 200)
    {
      return 0;
    }
    player_and_card = (int *)wparam;
    if (player_and_card == NULL ||
        global_available_slots < get_displayed_card_id(player_and_card[0], player_and_card[1]))
    {
      return 0;
    }
    if (find_battlefield_card_window(hwnd, player_and_card, NULL, &found_window) == 0)
    {
      found_window =
          CreateWindowExA(0, "MAGICGAME_CardClass", "In-play Card", 0x44000000,
                          0, 0, g_showlist_smallcard_width, g_showlist_smallcard_height,
                          hwnd, (HMENU)1, g_app_instance, player_and_card);
      if (found_window == (HWND)0)
      {
        return 0;
      }
      get_next_battlefield_card_position(hwnd, player_and_card,
                                         get_battlefield_card_stagger_offset(found_window),
                                         &x, &y, 1);
      SetWindowPos(found_window, (HWND)0, x, y, 0, 0, 5);
      ((HWND *)card_windows)[card_count] = found_window;
      card_count++;
      SetWindowLongA(hwnd, g_duel_window_userdata_card_offset, card_count);
      BringWindowToTop(found_window);
      SendMessageA(hwnd, 0x400, 0, 0);
      ShowWindow(found_window, SW_SHOW);
    }
    return card_count;

  case 0x40b:
    card_windows = (void *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
    card_count = GetWindowLongA(hwnd, g_duel_window_userdata_card_offset);
    remove_player_and_card = (int *)wparam;
    remove_found_40b = 0;
    for (remove_loop_index = 0;
         card_count > remove_loop_index && remove_found_40b == 0;
         remove_loop_index++)
    {
      if (card_window_matches_player_and_card(((HWND *)card_windows)[remove_loop_index],
                                              remove_player_and_card) != 0)
      {
        remove_found_40b = 1;
        DestroyWindow(((HWND *)card_windows)[remove_loop_index]);
        card_count--;
        for (remove_compact_index = remove_loop_index;
             card_count > remove_compact_index;
             remove_compact_index++)
        {
          ((HWND *)card_windows)[remove_compact_index] =
              ((HWND *)card_windows)[remove_compact_index + 1];
        }
        SetWindowLongA(hwnd, g_duel_window_userdata_card_offset, card_count);
        for (remove_compact_index = 0; card_count > remove_compact_index; remove_compact_index++)
        {
          if ((HWND)get_card_window_hidden_flag(((HWND *)card_windows)[remove_compact_index]) ==
              ((HWND *)card_windows)[remove_loop_index])
          {
            SendMessageA(((HWND *)card_windows)[remove_compact_index], 0x402, 0, 0);
          }
        }
      }
    }
    return remove_found_40b;

  case 0x40c:
    card_windows = (void *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
    card_count = GetWindowLongA(hwnd, g_duel_window_userdata_card_offset);
    for (destroy_index = 0; card_count > destroy_index; destroy_index++)
    {
      DestroyWindow(((HWND *)card_windows)[destroy_index]);
    }
    card_count = 0;
    SetWindowLongA(hwnd, g_duel_window_userdata_card_offset, card_count);
    reset_battlefield_layout_positions(hwnd);
    return 0;

  case 0x40e:
  case 0x40f:
    card_windows = (void *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
    card_count = GetWindowLongA(hwnd, g_duel_window_userdata_card_offset);
    query_player_and_card = (int *)wparam;
    query_found = 0;
    for (query_index = 0; card_count > query_index && query_found == 0; query_index++)
    {
      if (card_window_matches_player_and_card(((HWND *)card_windows)[query_index],
                                              query_player_and_card) != 0)
      {
        query_found = 1;
        if (msg == 0x40e)
        {
          query_value = get_card_window_displayed_card_id(((HWND *)card_windows)[query_index]);
        }
        else
        {
          query_value = (int)((HWND *)card_windows)[query_index];
        }
      }
    }

    if (query_found != 0)
    {
      return query_value;
    }
    else
    {
      return (msg == 0x40e) ? -1 : 0;
    }

  case 0x40d:
    card_windows = (void *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
    card_count = GetWindowLongA(hwnd, g_duel_window_userdata_card_offset);
    raise_player_and_card = (int *)wparam;
    raise_found = 0;
    for (raise_index = 0; card_count > raise_index && raise_found == 0; raise_index++)
    {
      if (card_window_matches_player_and_card(((HWND *)card_windows)[raise_index],
                                              raise_player_and_card) != 0)
      {
        raise_found = 1;
        BringWindowToTop(((HWND *)card_windows)[raise_index]);
        SendMessageA(hwnd, 0x400, 0, 0);
      }
    }
    return raise_found;

  case 0x400:
    card_windows = (void *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
    card_count = GetWindowLongA(hwnd, g_duel_window_userdata_card_offset);
    player_card_window = (HWND)GetWindowLongA(hwnd, g_duel_window_userdata_snapshot_offset);
    for (loop_index_400 = 0; card_count > loop_index_400; loop_index_400++)
    {
      SendMessageA(((HWND *)card_windows)[loop_index_400], 0x401, (WPARAM)displayed_player_and_card, 0);
      displayed_card_id = get_displayed_card_id(displayed_player_and_card[0], displayed_player_and_card[1]);
      get_displayed_card_attachment(attached_player_and_card, displayed_player_and_card[0], displayed_player_and_card[1]);
      if ((get_displayed_card_ui_flags(displayed_player_and_card[0], displayed_player_and_card[1]) & 0x10) != 0 &&
          (displayed_card_id >= g_damage_display_internal_card_id ||
           (global_raw_cards_storage[displayed_card_id].card_type == 2 &&
            global_raw_cards_storage[displayed_card_id].subtype != 0xd3)))
      {
        if (find_battlefield_card_window(hwnd, attached_player_and_card, NULL, &attached_window_400) != 0)
        {
          SendMessageA(((HWND *)card_windows)[loop_index_400], 0x402, (WPARAM)attached_window_400, 0);
        }
        else
        {
          if (g_duel_player_battlefield_window_hwnd == hwnd)
          {
            other_battlefield_window_400 = g_duel_help_owner_hwnd;
          }
          else
          {
            other_battlefield_window_400 = g_duel_player_battlefield_window_hwnd;
          }
          if (find_battlefield_card_window(other_battlefield_window_400, attached_player_and_card,
                                           NULL, &attached_window_400) != 0)
          {
            SendMessageA(hwnd, 0x40b, (WPARAM)displayed_player_and_card, 0);
            SendMessageA(other_battlefield_window_400, 0x40a, (WPARAM)displayed_player_and_card, 0);
            PostMessageA(other_battlefield_window_400, 0x400, 0, 0);
          }
        }
      }
      else if (displayed_card_id == g_damage_display_internal_card_id &&
               attached_player_and_card[0] != -1 &&
               attached_player_and_card[1] == -1)
      {
        SendMessageA(((HWND *)card_windows)[loop_index_400], 0x402, (WPARAM)player_card_window, 0);
      }
      else
      {
        SendMessageA(((HWND *)card_windows)[loop_index_400], 0x402, 0, 0);
      }
    }
    card_windows = (void *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
    card_count = GetWindowLongA(hwnd, g_duel_window_userdata_card_offset);
    for (loop_index_400 = 0; card_count > loop_index_400; loop_index_400++)
    {
      SendMessageA(hwnd, 0x410, (WPARAM)((HWND *)card_windows)[loop_index_400], 0);
    }
    SendMessageA(hwnd, 0x410, (WPARAM)player_card_window, 0);
    return 0;

  case 0x410:
    card_windows = (void *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
    card_count = GetWindowLongA(hwnd, g_duel_window_userdata_card_offset);
    attachment_parent = (HWND)wparam;
    if (attachment_parent == (HWND)0)
    {
      return 0;
    }
    attachment_x_step = 5;
    attachment_y_step = g_duel_selected_card_window;
    GetWindowRect(attachment_parent, &attachment_rect);
    MapWindowPoints((HWND)0, hwnd, (LPPOINT)&attachment_rect, 2);
    attachment_x = attachment_rect.left + attachment_x_step;
    attachment_y = attachment_rect.top - attachment_y_step;
    previous_attachment_window = attachment_parent;
    for (attachment_index = 0; card_count > attachment_index; attachment_index++)
    {
      if ((HWND)get_card_window_hidden_flag(((HWND *)card_windows)[attachment_index]) ==
          attachment_parent)
      {
        SetWindowPos(((HWND *)card_windows)[attachment_index], previous_attachment_window,
                     attachment_x, attachment_y, 0, 0, 1);
        attachment_y -= attachment_y_step;
        attachment_descendant_count =
            count_hidden_battlefield_descendants(hwnd, ((HWND *)card_windows)[attachment_index]);
        if (0 < attachment_descendant_count)
        {
          attachment_y -= attachment_descendant_count * attachment_y_step;
        }
        previous_attachment_window = ((HWND *)card_windows)[attachment_index];
      }
    }
    return 0;

  case 0x402:
    card_windows = (void *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
    card_count = GetWindowLongA(hwnd, g_duel_window_userdata_card_offset);
    hidden_parent = (HWND)wparam;
    hidden_lparam = lparam;
    if (hidden_parent == (HWND)0)
    {
      return 0;
    }
    ShowWindow(hidden_parent, (hidden_lparam < 1) ? 0 : SW_SHOW);
    for (hidden_index = 0; card_count > hidden_index; hidden_index++)
    {
      if ((HWND)get_card_window_hidden_flag(((HWND *)card_windows)[hidden_index]) == hidden_parent)
      {
        SendMessageA(hwnd, 0x402, (WPARAM)((HWND *)card_windows)[hidden_index], hidden_lparam);
      }
    }
    return 0;

  case 0x401:
    card_windows = (void *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
    card_count = GetWindowLongA(hwnd, g_duel_window_userdata_card_offset);
    for (show_index = 0; card_count > show_index; show_index++)
    {
      if (IsWindowVisible(((HWND *)card_windows)[show_index]) == 0)
      {
        ShowWindow(((HWND *)card_windows)[show_index], SW_SHOW);
      }
    }
    return 0;

  case 0x411:
    card_windows = (void *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
    card_count = GetWindowLongA(hwnd, g_duel_window_userdata_card_offset);
    for (unique_outer_index = 0; card_count > unique_outer_index; unique_outer_index++)
    {
      card_windows_copy[unique_outer_index] = ((int *)card_windows)[unique_outer_index];
    }
    unique_flagged_count = 0;
    for (unique_outer_index = 0; card_count > unique_outer_index; unique_outer_index++)
    {
      if (((int *)card_windows)[unique_outer_index] != 0)
      {
        SendMessageA(((HWND *)card_windows)[unique_outer_index], 0x401, (WPARAM)scan_player_and_card, 0);
        if ((get_displayed_card_ui_flags(scan_player_and_card[0], scan_player_and_card[1]) & 4) != 0)
        {
          unique_flagged_count++;
          unique_flag_id = get_displayed_card_blocking(scan_player_and_card[0], scan_player_and_card[1]);
          if (unique_flag_id != -1)
          {
            for (unique_inner_index = unique_outer_index + 1;
                 card_count > unique_inner_index;
                 unique_inner_index++)
            {
              SendMessageA(((HWND *)card_windows)[unique_inner_index], 0x401, (WPARAM)scan_player_and_card, 0);
              if (get_displayed_card_blocking(scan_player_and_card[0], scan_player_and_card[1]) == unique_flag_id)
              {
                ((int *)card_windows)[unique_inner_index] = 0;
              }
            }
          }
        }
      }
    }
    for (unique_outer_index = 0; card_count > unique_outer_index; unique_outer_index++)
    {
      ((int *)card_windows)[unique_outer_index] = card_windows_copy[unique_outer_index];
    }
    return unique_flagged_count;

  case 0x412:
    card_windows = (void *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
    card_count = GetWindowLongA(hwnd, g_duel_window_userdata_card_offset);
    player_card_window = (HWND)GetWindowLongA(hwnd, g_duel_window_userdata_snapshot_offset);
    GetClientRect(hwnd, &resize_rect);
    descendant_count = count_hidden_battlefield_descendants(hwnd, player_card_window);
    if (descendant_count > 0)
    {
      SetWindowPos(player_card_window, (HWND)0,
                   resize_rect.right - g_showlist_smallcard_width * 2,
                   resize_rect.bottom - g_showlist_smallcard_height - 5, 0, 0, 5);
      ShowWindow(player_card_window, SW_SHOW);
    }
    else
    {
      ShowWindow(player_card_window, SW_HIDE);
    }
    return 0;

  case WM_COMMAND:
    switch ((int)wparam & 0xffff)
    {
    case 0x66:
      g_stop_phase = -1;
      g_stop_phase_player = -1;
      g_recorded_action_player = 0;
      g_territory_command_packet[0] = -2;
      g_territory_command_packet[1] = -1;
      g_territory_command_packet[2] = -2;
      PostMessageA(g_duel_window_hwnd, 0x464, 0, (LPARAM)g_territory_command_packet);
      break;
    case 0x65:
      g_stop_phase = -1;
      g_stop_phase_player = -1;
      g_recorded_action_player = 0;
      g_territory_command_packet[0] = -2;
      g_territory_command_packet[1] = -1;
      g_territory_command_packet[2] = -1;
      PostMessageA(g_duel_window_hwnd, 0x464, 0, (LPARAM)g_territory_command_packet);
      break;
    case 0x64:
      get_next_phase_stop(&g_stop_phase_player, &g_stop_phase, NULL);
      g_recorded_action_player = 0;
      g_territory_command_packet[0] = -2;
      g_territory_command_packet[1] = -1;
      g_territory_command_packet[2] = -1;
      PostMessageA(g_duel_window_hwnd, 0x464, 0, (LPARAM)g_territory_command_packet);
      break;
    case 0x68:
      show_territory_options_dialog(g_duel_window_hwnd);
      break;
    case 0x67:
      refresh_duel_window(hwnd);
      break;
    case 0x6c:
      SendMessageA(g_duel_window_hwnd, WM_CLOSE, 0, 0);
      break;
    case 0x6d:
      SendMessageA(g_duel_window_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
      break;
    case 0x70:
      g_battlefield_ui_flags ^= 1;
      break;
    case 0x71:
      g_response_prompt_enabled ^= 1;
      break;
    case 0x72:
      SendMessageA(hwnd == g_duel_player_battlefield_window_hwnd
                       ? g_duel_player_graveyard_window_hwnd
                       : g_phasebar_your_untap_window_hwnd,
                   WM_COMMAND, 0x65, 0);
      break;
    case 0x73:
      SendMessageA(g_duel_player_graveyard_window_hwnd, WM_COMMAND, 0x66, 0);
      break;
    case 0x6e:
      help_context = 0x7e2;
      strcpy(help_path, global_base_directory);
      strcat(help_path, "\\duel.hlp");
      WinHelpA(g_duel_window_hwnd, help_path, HELP_CONTEXT, help_context);
      break;
    case 0x6f:
      if ((g_duel_network_flags & 2) == 0)
      {
        SendMessageA(g_duel_window_hwnd, msg, 0x27b, 0);
      }
      break;
    case 0x69:
      SendMessageA(g_duel_window_hwnd, msg, 0x279, 0);
      break;
    case 0x6a:
      SendMessageA(g_duel_window_hwnd, msg, 0x27a, 0);
      break;
    case 0x6b:
      SendMessageA(g_duel_window_hwnd, msg, 0x27c, 0);
      break;
    case 0x258:
    case 0x259:
    case 0x25c:
    case 0x25e:
    case 0x261:
    case 0x265:
    case 0x269:
    case 0x26d:
    case 0x271:
    case 0x273:
    default:
      SendMessageA(g_duel_window_hwnd, msg, wparam, lparam);
      break;
    }
    return 0;

  case WM_CREATE:
    shandalar_win32_imgui_register_territory_menu(hwnd);
    card_count = 0;
    SetWindowLongA(hwnd, g_duel_window_userdata_card_offset, card_count);
    card_windows = malloc(800);
    SetWindowLongA(hwnd, g_duel_window_userdata_player_offset, (LONG)card_windows);
    background_bitmap = (HWND)0;
    SetWindowLongA(hwnd, g_duel_window_userdata_8_offset, (LONG)background_bitmap);
    if (GetDlgCtrlID(hwnd) == 0x79)
    {
      create_data[0] = 0;
    }
    else
    {
      create_data[0] = 1;
    }
    create_data[1] = 0xffffffff;
    player_card_window =
        CreateWindowExA(0, "MAGICGAME_CardClass", "Player Card", 0x44000000, 0, 0, 0, 0,
                        hwnd, (HMENU)0, g_app_instance, create_data);
    SetWindowLongA(hwnd, g_duel_window_userdata_snapshot_offset, (LONG)player_card_window);
    if (card_windows == NULL || player_card_window == (HWND)0)
    {
      return -1;
    }
    return 0;

  case WM_DESTROY:
    shandalar_win32_imgui_unregister_territory_menu(hwnd);
    card_windows = (void *)GetWindowLongA(hwnd, g_duel_window_userdata_player_offset);
    free(card_windows);
    background_bitmap = (HWND)GetWindowLongA(hwnd, g_duel_window_userdata_8_offset);
    if (background_bitmap != (HWND)0)
    {
      delete_and_close_object(background_bitmap);
    }
    return 0;

  case WM_ERASEBKGND:
    background_bitmap = (HWND)GetWindowLongA(hwnd, g_duel_window_userdata_8_offset);
    erase_dc = (HDC)wparam;
    ApplyCardArtPaletteToDc(erase_dc);
    GetClientRect(hwnd, &erase_rect);
    if (g_duel_palette_refresh_pending != 0)
    {
      FillRect(erase_dc, &erase_rect, GetStockObject(WHITE_BRUSH));
      Sleep(300);
    }
    if (background_bitmap != (HWND)0)
    {
      DrawBitmapToRect(erase_dc, &erase_rect, (HBITMAP)background_bitmap);
    }
    else
    {
      FillRect(erase_dc, &erase_rect, GetStockObject(BLACK_BRUSH));
    }
    return 1;

  case WM_LBUTTONDOWN:
    return 0;

  case WM_LBUTTONDBLCLK:
    SendMessageA(hwnd, WM_COMMAND, 0x67, 0);
    return 0;

  case WM_RBUTTONDOWN:
    popup_point.x = (unsigned int)lparam & 0xffff;
    popup_point.y = (unsigned short)(((unsigned int)lparam >> 16) & 0xffff);
    ClientToScreen(hwnd, &popup_point);
    SetRect(&popup_rect, popup_point.x, popup_point.y, popup_point.x + 1, popup_point.y + 1);
    TrackPopupMenu(g_territory_popup_menu, TPM_RIGHTBUTTON, popup_point.x, popup_point.y, 0, hwnd,
                   &popup_rect);
    return 0;

  case WM_INITMENU:
    if (g_duel_modal_action_active != 0)
    {
      if ((g_duel_action_request_copy.allow_cancel & 2) != 0)
      {
        // "Done"
        // AppendMenuA(g_territory_popup_menu, MF_STRING, 0x66, g_custom_duel_action_text);
      }
      if ((g_duel_action_request_copy.allow_cancel & 1) != 0)
      {
        AppendMenuA(g_territory_popup_menu, MF_STRING, 0x65, g_duel_status_text);
      }
      get_current_duel_selection(NULL, &action_code);
      if (action_code == 1)
      {
        strcpy(menu_text, g_territory_menu_action_1_text);
      }
      else if (action_code == 2 || action_code == 3 || action_code == 4 || action_code == 5)
      {
        strcpy(menu_text, g_territory_menu_action_2_to_5_text);
      }
      else if (action_code == 10)
      {
        strcpy(menu_text, g_territory_menu_action_10_text);
      }
      else if (action_code == 0x14)
      {
        strcpy(menu_text, g_territory_menu_action_20_text);
      }
      else if (action_code == 0x15)
      {
        strcpy(menu_text, g_territory_menu_action_21_text);
      }
      else if (action_code == 0x16)
      {
        strcpy(menu_text, g_territory_menu_action_22_text);
      }
      else if (action_code == 0x17)
      {
        strcpy(menu_text, g_territory_menu_action_23_text);
      }
      else if (action_code == 0x18)
      {
        strcpy(menu_text, g_territory_menu_action_24_text);
      }
      else if (action_code == 0x19 || action_code == 0x1a)
      {
        strcpy(menu_text, g_territory_menu_action_19_1a_text);
      }
      else if (action_code == 0x1b)
      {
        strcpy(menu_text, g_territory_menu_action_1b_text);
      }
      else if (action_code == 0x1e)
      {
        strcpy(menu_text, g_territory_menu_action_30_text);
      }
      else if (action_code == 0x1f)
      {
        strcpy(menu_text, g_territory_menu_action_31_text);
      }
      else if (action_code == 0x20 || action_code == 0x21 ||
               action_code == 0x22 || action_code == 0x25)
      {
        strcpy(menu_text, g_territory_menu_action_20_25_text);
      }
      else
      {
        strcpy(menu_text, g_territory_menu_default_action_text);
      }
      if ((g_duel_network_flags & 2) == 0)
      {
        AppendMenuA(g_territory_popup_menu, MF_STRING, 100, menu_text);
      }
    }
    if (GetMenuItemCount(g_territory_popup_menu) > 0)
    {
      AppendMenuA(g_territory_popup_menu, MF_SEPARATOR, 0, NULL);
    }
    if (g_duel_player_battlefield_window_hwnd == hwnd)
    {
      AppendMenuA(g_territory_popup_menu, MF_STRING, 0x67, g_territory_menu_your_battlefield_text);
    }
    else
    {
      AppendMenuA(g_territory_popup_menu, MF_STRING, 0x67, g_territory_menu_opponent_battlefield_text);
    }
    AppendMenuA(g_territory_popup_menu, MF_STRING, 0x68, g_territory_menu_action_18_text);
    AppendMenuA(g_territory_popup_menu, MF_STRING, 0x69, g_territory_menu_action_69_text);
    if (g_duel_interface_options.show_id_tags_on_cards != 0)
    {
      CheckMenuItem(g_territory_popup_menu, 0x69, MF_CHECKED);
    }
    AppendMenuA(g_territory_popup_menu, MF_STRING, 0x6a, g_territory_menu_show_options_text);
    if (g_duel_interface_options.show_invisible_effect_cards != 0)
    {
      CheckMenuItem(g_territory_popup_menu, 0x6a, MF_CHECKED);
    }
    AppendMenuA(g_territory_popup_menu, MF_STRING, 0x6b, g_territory_menu_end_turn_text);
    if (g_duel_interface_options.show_all_cards_summon_sickness != 0)
    {
      CheckMenuItem(g_territory_popup_menu, 0x6b, MF_CHECKED);
    }
    AppendMenuA(g_territory_popup_menu, MF_SEPARATOR, 0, NULL);
    if ((g_duel_mode_flags & 1) == 0)
    {
      if ((g_duel_network_flags & 2) != 0)
      {
        if (g_battlefield_ui_flags != 0)
        {
          AppendMenuA(g_territory_popup_menu, MF_STRING, 0x70,
                      g_territory_menu_skip_full_card_text[0]);
        }
        else
        {
          AppendMenuA(g_territory_popup_menu, MF_STRING, 0x70,
                      g_territory_menu_skip_full_card_text[1]);
        }
        if (g_response_prompt_enabled != 0)
        {
          AppendMenuA(g_territory_popup_menu, MF_STRING, 0x71,
                      g_territory_menu_draw_response_text[0]);
        }
        else
        {
          AppendMenuA(g_territory_popup_menu, MF_STRING, 0x71,
                      g_territory_menu_draw_response_text[1]);
        }
      }
      AppendMenuA(g_territory_popup_menu, MF_STRING, 0x6d, g_territory_menu_action_68_text);
      if ((g_duel_network_flags & 2) == 0)
      {
        AppendMenuA(g_territory_popup_menu, MF_STRING, 0x6f, g_territory_menu_action_6f_text);
      }
    }
    // "Help..."
    // AppendMenuA(g_territory_popup_menu, MF_STRING, 0x6e, g_territory_menu_main_window_text);

    // These are from graveyard class but copied here to be accessible on mobile
    territory_player = g_duel_player_battlefield_window_hwnd == hwnd ? 0 : 1;
    AppendMenuA(g_territory_popup_menu, MF_STRING, 0x72,
                g_graveyard_menu_view_exile_text);
    if (copy_cached_exile_cards_and_get_count(menu_exile_cards, territory_player) == 0)
    {
      EnableMenuItem(g_territory_popup_menu, 0x72, MF_GRAYED);
    }
    AppendMenuA(g_territory_popup_menu, MF_STRING, 0x73,
                g_graveyard_menu_view_antes_text);

    if (g_duel_modal_action_active != 0)
    {
      // "Concede"
      // AppendMenuA(g_territory_popup_menu, MF_SEPARATOR, 0, NULL);
      AppendMenuA(g_territory_popup_menu, MF_POPUP, (UINT)g_territory_submenu,
                  g_territory_menu_submenu_text);
    }
    if (g_duel_cheats_state != 0)
    {
      AppendMenuA(g_territory_popup_menu, MF_SEPARATOR, 0, NULL);
      g_territory_debug_menu_b = g_territory_popup_menu;
      AppendMenuA(g_territory_debug_menu_b, MF_STRING,
                  territory_player == 0 ? 0x26b : 0x26c,
                  territory_player == 0 ? "Put a specific card IN PLAY for you"
                                        : "Put a specific card IN PLAY for opponent");
      AppendMenuA(g_territory_debug_menu_b, MF_STRING,
                  territory_player == 0 ? 0x26d : 0x26e,
                  territory_player == 0 ? "Put a specific card into your HAND"
                                        : "Put a specific card into opponent's HAND");
      AppendMenuA(g_territory_debug_menu_b, MF_STRING,
                  territory_player == 0 ? 0x269 : 0x26a,
                  territory_player == 0 ? "DRAW a card into your hand"
                                        : "DRAW a card into opponent's hand");
      AppendMenuA(g_territory_debug_menu_b, MF_STRING,
                  territory_player == 0 ? 0x26f : 0x270,
                  territory_player == 0 ? "SET your LIFE total"
                                        : "SET opponent's LIFE total");
      AppendMenuA(g_territory_debug_menu_b, MF_STRING, 0x267, "Win\tW");
      AppendMenuA(g_territory_debug_menu_b, MF_STRING, 0x263, "Tie (draw)\tT");
      AppendMenuA(g_territory_debug_menu_b, MF_STRING, 0x25c, "Show opponent's hand\tF8");
      if (g_duel_surface_reset_state != 0)
      {
        CheckMenuItem(g_territory_debug_menu_b, 0x25c, MF_CHECKED);
      }
      AppendMenuA(g_territory_debug_menu_b, MF_STRING,
                  territory_player == 0 ? 0x25e : 0x25d,
                  territory_player == 0 ? "Show your library"
                                        : "Show opponent's library");
    }
    return 0;

  case WM_MENUSELECT:
    if ((unsigned short)(((UINT)wparam >> 16) & 0xffff) == 0xffff && lparam == 0)
    {
      menu_item_count = GetMenuItemCount(g_territory_popup_menu);
      while (menu_item_count-- != 0)
      {
        RemoveMenu(g_territory_popup_menu, 0, MF_BYPOSITION);
      }
    }
    return 0;

  case WM_SETCURSOR:
    return handle_duel_inactive_cursor(hwnd, msg, wparam, lparam);

  case WM_QUERYNEWPALETTE:
  case WM_PALETTECHANGED:
  case WM_PALETTEISCHANGING:
    return handle_button_palette_message((int)hwnd, msg, (int)wparam, lparam);

  default:
    return DefWindowProcA(hwnd, msg, wparam, lparam);
  }

  return 0;
}
