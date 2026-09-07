#include <windows.h>
#include <string.h>
#include <stdio.h>

#include "cardartlib/src/palette.h"
#include "drawcardlib/src/pic.h"
#include "magic/src/duel_engine.h"
#include "magic/src/game_support.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "magic/src/shared_startup.h"

#define PHASE_DISPLAY_SELECTED_CARD_OFFSET 0
#define PHASE_DISPLAY_SELECTED_PLAYER_OFFSET 4

#define PHASE_DISPLAY_COMMAND_HELP 0x64
#define PHASE_DISPLAY_COMMAND_STOP_BASE 0x96
#define PHASE_DISPLAY_COMMAND_TOGGLE_BASE 0xc8
#define PHASE_DISPLAY_COMMAND_HELP_BASE 0xfa

#define PHASE_DISPLAY_PLAYER_COMMAND_OFFSET 10
#define PHASE_DISPLAY_BITMAP_WIDTH 0x2f8

#ifndef SHANDALAR
extern HWND global_main_hwnd;
#define DUEL_MAIN_WINDOW_HWND global_main_hwnd
#else
#define DUEL_MAIN_WINDOW_HWND g_main_window_hwnd
#endif

// GLOBAL: MAGIC 0x0055e1b4
// GLOBAL: SHANDALAR 0x0057f16c
int g_phase_display_selected_card_window_long_offset = 0;

// GLOBAL: MAGIC 0x0055e1b8
// GLOBAL: SHANDALAR 0x0057f170
int g_phase_display_selected_player_window_long_offset = 4;

// GLOBAL: MAGIC 0x0055e1bc
// GLOBAL: SHANDALAR 0x0057f174
int g_phase_display_window_extra_bytes = 8;

// GLOBAL: MAGIC 0x00708768
// GLOBAL: SHANDALAR 0x006695a0
HANDLE g_magicgame_phase_display_pic;

// GLOBAL: MAGIC 0x00708770
// GLOBAL: SHANDALAR 0x006695a8
char g_phase_display_menu_phase_help_text[0x38];

// GLOBAL: MAGIC 0x007087a8
// GLOBAL: SHANDALAR 0x006695e0
char g_phase_display_menu_toggle_text[0x38];

// GLOBAL: MAGIC 0x007087e0
// GLOBAL: SHANDALAR 0x00669618
target_selection_result_t g_phase_display_click_packet;

// GLOBAL: MAGIC 0x007087f0
// GLOBAL: SHANDALAR 0x00669628
char g_phase_display_menu_help_text[0x1c];

// GLOBAL: MAGIC 0x0070880c
// GLOBAL: SHANDALAR 0x00669644
HBRUSH g_phase_display_hatch_brush;

// GLOBAL: MAGIC 0x00708810
// GLOBAL: SHANDALAR 0x00669648
HMENU g_phase_display_menu;

// GLOBAL: MAGIC 0x00708818
// GLOBAL: SHANDALAR 0x00669650
target_selection_result_t g_phase_display_menu_packet;

// GLOBAL: MAGIC 0x00708848
// GLOBAL: SHANDALAR 0x00669680
int g_phase_display_bitmap_divisor;

// GLOBAL: MAGIC 0x00708850
// GLOBAL: SHANDALAR 0x00669688
char g_phase_display_menu_stop_text[0x38];

extern char global_base_directory[];
extern char global_duelart_path[];
extern int g_duel_modal_action_active;
extern int g_phase_display_refresh_requested;
extern int g_saved_stop_phase_player;
extern int g_duel_cached_phase_stops_player_1[0x26];
extern int g_duel_cached_phase_stops_player_0[0x26];

void get_current_duel_selection(int *selected_player, int *selected_card);
void save_duel_interface_options_to_registry(void);
unsigned int refresh_duel_display_cache(void);
LRESULT handle_duel_inactive_cursor(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
void delete_and_close_object(HANDLE obj);
int load_text_with_tab_escapes(char *filename, char *section_name);
int is_invalid_duel_player(int player);

static void get_phase_display_phase_rect(RECT *rect, int player, int phase, int width, int height);
static void hit_test_phase_display(POINT *point, RECT *client_rect, int *player, int *phase);
static int get_phase_display_command_index(int phase);
void copy_phase_stop_flags(int *phase_flags, int player);
void get_phase_display_selected_half(int *selected_half);
void get_phase_display_action_selection(int *player, int *phase);
void draw_phase_display_status_markers(HDC dc, RECT *client_rect);
void noop_phase_display_user_message(HWND hwnd, WPARAM wparam, LPARAM lparam);

// FUNCTION: MAGIC 0x00536e7e
// FUNCTION: SHANDALAR 0x0055b6af
static void hit_test_phase_display(POINT *point, RECT *client_rect, int *player, int *phase)
{
  int found_player;
  POINT test_point;
  RECT client_copy;
  RECT phase_rect;
  int found_phase;

  CopyRect(&client_copy, client_rect);
  test_point = *point;
  found_phase = -1;
  found_player = 1;

  get_phase_display_phase_rect(&phase_rect, 1, 1, client_copy.right, client_copy.bottom);
  if (PtInRect(&phase_rect, test_point) != 0)
    found_phase = 1;
  get_phase_display_phase_rect(&phase_rect, 1, 4, client_copy.right, client_copy.bottom);
  if (PtInRect(&phase_rect, test_point) != 0)
    found_phase = 4;
  get_phase_display_phase_rect(&phase_rect, 1, 0xa, client_copy.right, client_copy.bottom);
  if (PtInRect(&phase_rect, test_point) != 0)
    found_phase = 0xa;
  get_phase_display_phase_rect(&phase_rect, 1, 0x14, client_copy.right, client_copy.bottom);
  if (PtInRect(&phase_rect, test_point) != 0)
    found_phase = 0x14;
  get_phase_display_phase_rect(&phase_rect, 1, 0x16, client_copy.right, client_copy.bottom);
  if (PtInRect(&phase_rect, test_point) != 0)
    found_phase = 0x16;
  get_phase_display_phase_rect(&phase_rect, 1, 0x1e, client_copy.right, client_copy.bottom);
  if (PtInRect(&phase_rect, test_point) != 0)
    found_phase = 0x1e;
  get_phase_display_phase_rect(&phase_rect, 1, 0x1f, client_copy.right, client_copy.bottom);
  if (PtInRect(&phase_rect, test_point) != 0)
    found_phase = 0x1f;
  get_phase_display_phase_rect(&phase_rect, 1, 0x20, client_copy.right, client_copy.bottom);
  if (PtInRect(&phase_rect, test_point) != 0)
    found_phase = 0x20;

  if (found_phase == -1)
  {
    found_player = 0;
    get_phase_display_phase_rect(&phase_rect, 0, 1, client_copy.right, client_copy.bottom);
    if (PtInRect(&phase_rect, test_point) != 0)
      found_phase = 1;
    get_phase_display_phase_rect(&phase_rect, 0, 4, client_copy.right, client_copy.bottom);
    if (PtInRect(&phase_rect, test_point) != 0)
      found_phase = 4;
    get_phase_display_phase_rect(&phase_rect, 0, 0xa, client_copy.right, client_copy.bottom);
    if (PtInRect(&phase_rect, test_point) != 0)
      found_phase = 0xa;
    get_phase_display_phase_rect(&phase_rect, 0, 0x14, client_copy.right, client_copy.bottom);
    if (PtInRect(&phase_rect, test_point) != 0)
      found_phase = 0x14;
    get_phase_display_phase_rect(&phase_rect, 0, 0x15, client_copy.right, client_copy.bottom);
    if (PtInRect(&phase_rect, test_point) != 0)
      found_phase = 0x15;
    get_phase_display_phase_rect(&phase_rect, 0, 0x1e, client_copy.right, client_copy.bottom);
    if (PtInRect(&phase_rect, test_point) != 0)
      found_phase = 0x1e;
    get_phase_display_phase_rect(&phase_rect, 0, 0x1f, client_copy.right, client_copy.bottom);
    if (PtInRect(&phase_rect, test_point) != 0)
      found_phase = 0x1f;
    get_phase_display_phase_rect(&phase_rect, 0, 0x20, client_copy.right, client_copy.bottom);
    if (PtInRect(&phase_rect, test_point) != 0)
      found_phase = 0x20;
  }

  *player = found_player;
  *phase = found_phase;
}

static int get_phase_display_command_index(int phase)
{
  if (phase == 1)
    return 0;
  if (phase >= 2 && phase <= 5)
    return 1;
  if (phase == 0xa)
    return 2;
  if (phase == 0x14)
    return 3;
  if (phase >= 0x15 && phase <= 0x1b)
    return 4;
  if (phase == 0x1e)
    return 5;
  if (phase == 0x1f)
    return 6;
  if (phase >= 0x20 && phase <= 0x25)
    return 7;
  return -1;
}

// FUNCTION: MAGIC 0x0053726a
// FUNCTION: SHANDALAR 0x0055ba9b
static void get_phase_display_phase_rect(RECT *rect, int player, int phase, int width, int height)
{
  int top;
  int left;

  if (player == -1 || phase == -1)
  {
    SetRect(rect, 0, 0, 0, 0);
    return;
  }

  top = (height * 40) / PHASE_DISPLAY_BITMAP_WIDTH;
  if (phase == 1)
    left = (height * 2) / PHASE_DISPLAY_BITMAP_WIDTH;
  else if (phase == 2 || phase == 3 || phase == 4 || phase == 5)
    left = (height * 43) / PHASE_DISPLAY_BITMAP_WIDTH;
  else if (phase == 0xa)
    left = (height * 84) / PHASE_DISPLAY_BITMAP_WIDTH;
  else if (phase == 0x14)
    left = (height * 125) / PHASE_DISPLAY_BITMAP_WIDTH;
  else if ((player == 1 && phase == 0x16) || (player == 0 && phase == 0x15))
    left = (height * 166) / PHASE_DISPLAY_BITMAP_WIDTH;
  else if (phase == 0x1e)
    left = (height * 207) / PHASE_DISPLAY_BITMAP_WIDTH;
  else if (phase == 0x1f)
    left = (height * 248) / PHASE_DISPLAY_BITMAP_WIDTH;
  else if (phase == 0x20 || phase == 0x21 || phase == 0x22 || phase == 0x25)
    left = (height * 289) / PHASE_DISPLAY_BITMAP_WIDTH;
  else
    left = -1;

  if (left == -1)
  {
    SetRect(rect, 0, 0, 0, 0);
    return;
  }

  if (player == 0)
    left += (height * 430) / PHASE_DISPLAY_BITMAP_WIDTH;

  SetRect(rect, 0, left, width, left + top);
}

// FUNCTION: MAGIC 0x0044a213
// FUNCTION: SHANDALAR 0x00453d96
void copy_phase_stop_flags(int *phase_flags, int player)
{
  if (phase_flags == (int *)0)
    return;

  if (is_invalid_duel_player(player) != 0)
    return;

  EnterCriticalSection(&g_duel_render_lock);

  memcpy(phase_flags, player == 0 ? g_duel_cached_phase_stops_player_0 : g_duel_cached_phase_stops_player_1, 0x98);

  LeaveCriticalSection(&g_duel_render_lock);
}

// FUNCTION: MAGIC 0x0044a288
// FUNCTION: SHANDALAR 0x00453e0b
void get_phase_display_selected_half(int *selected_half)
{
  EnterCriticalSection(&g_duel_render_lock);
  if (selected_half != (int *)0)
    *selected_half = g_phase_display_refresh_requested;
  LeaveCriticalSection(&g_duel_render_lock);
}

// FUNCTION: MAGIC 0x0044a376
// FUNCTION: SHANDALAR 0x00453ef9
void get_phase_display_action_selection(int *player, int *phase)
{
  EnterCriticalSection(&g_duel_render_lock);
  if (player != (int *)0)
    *player = g_saved_stop_phase_player;
  if (phase != (int *)0)
    *phase = g_saved_stop_phase;
  LeaveCriticalSection(&g_duel_render_lock);
}

// FUNCTION: MAGIC 0x005374b3
// FUNCTION: SHANDALAR 0x0055bce4
void draw_phase_display_status_markers(HDC dc, RECT *client_rect)
{
  int phase_index;
  int phase_flags[0x26];
  HGDIOBJ old_brush;
  RECT phase_rect;
  int phase;
  RECT ellipse_rect;
  int player_index;
  HBRUSH brush;
  int player;

  get_phase_display_action_selection(&player, &phase);
  if (player == -1 || phase == -1)
  {
    brush = CreateSolidBrush(0xff);
    old_brush = SelectObject(dc, brush);
    for (player_index = 0; player_index < 2; ++player_index)
    {
      copy_phase_stop_flags(phase_flags, player_index);
      for (phase_index = 0; phase_index <= 0x25; ++phase_index)
      {
        if (phase_flags[phase_index] != 0)
        {
          get_phase_display_phase_rect(&phase_rect, player_index, phase_index,
                                       client_rect->right, client_rect->bottom);
          if (IsRectEmpty(&phase_rect) == 0)
          {
            CopyRect(&ellipse_rect, &phase_rect);
            ellipse_rect.left = ellipse_rect.right -
                                (phase_rect.right - phase_rect.left) / 3;
            ellipse_rect.top = ellipse_rect.bottom -
                               ((ellipse_rect.right - ellipse_rect.left) *
                                (phase_rect.bottom - phase_rect.top)) /
                                   (phase_rect.right - phase_rect.left);
            Ellipse(dc, ellipse_rect.left, ellipse_rect.top,
                    ellipse_rect.right, ellipse_rect.bottom);
          }
        }
      }
    }
    SelectObject(dc, old_brush);
    DeleteObject(brush);
  }

  if (player != -1 && phase != -1)
  {
    brush = CreateSolidBrush(0xff00);
    old_brush = SelectObject(dc, brush);
    for (player_index = 0; player_index < 2; ++player_index)
    {
      for (phase_index = 0; phase_index <= 0x25; ++phase_index)
      {
        if (player_index == player)
        {
          if (phase_index == phase)
          {
            get_phase_display_phase_rect(&phase_rect, player_index, phase_index,
                                         client_rect->right, client_rect->bottom);
            if (IsRectEmpty(&phase_rect) == 0)
            {
              CopyRect(&ellipse_rect, &phase_rect);
              ellipse_rect.right = ellipse_rect.left +
                                   (phase_rect.right - phase_rect.left) / 3;
              ellipse_rect.top = ellipse_rect.bottom -
                                 ((ellipse_rect.right - ellipse_rect.left) *
                                  (phase_rect.bottom - phase_rect.top)) /
                                     (phase_rect.right - phase_rect.left);
              Ellipse(dc, ellipse_rect.left, ellipse_rect.top,
                      ellipse_rect.right, ellipse_rect.bottom);
            }
          }
        }
      }
    }
    SelectObject(dc, old_brush);
    DeleteObject(brush);
  }
}

// FUNCTION: MAGIC 0x005390d6
// FUNCTION: SHANDALAR 0x0055d8fa
void noop_phase_display_user_message(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
}

// FUNCTION: MAGIC 0x00535650
// FUNCTION: SHANDALAR 0x00559e90
int register_MAGICGAME_PhaseDisplayClass(LPCSTR class_name)
{
  char pic_path[264];
  int result;
  WNDCLASSA wndclass;

  result = 1;
  wndclass.style = 0xb;
  wndclass.lpfnWndProc = wndproc_MAGICGAME_PhaseDisplayClass;
  wndclass.cbClsExtra = 0;
  wndclass.cbWndExtra = g_phase_display_window_extra_bytes;
  wndclass.hInstance = g_app_instance;
  wndclass.hIcon = LoadIconA((HINSTANCE)0, (LPCSTR)0x7f00);
  wndclass.hCursor = LoadCursorA((HINSTANCE)0, (LPCSTR)0x7f00);
  wndclass.hbrBackground = (HBRUSH)0x6;
  wndclass.lpszMenuName = (LPCSTR)0;
  wndclass.lpszClassName = class_name;
  if (RegisterClassA(&wndclass) == 0)
    result = 0;

  g_phase_display_menu = CreatePopupMenu();
  load_text_with_tab_escapes(global_ui_strings_filename, "MENU_PHASEBAR");
  strcpy(g_phase_display_menu_stop_text, g_text_lines[0]);
  strcpy(g_phase_display_menu_toggle_text, g_text_lines[1]);
  strcpy(g_phase_display_menu_phase_help_text, g_text_lines[2]);
  strcpy(g_phase_display_menu_help_text, g_text_lines[3]);
  strcpy(pic_path, global_duelart_path);
  strcat(pic_path, "\\WINBK_Phase.pic");
  g_magicgame_phase_display_pic = load_pic(pic_path);
  g_phase_display_bitmap_divisor = 2;
  g_phase_display_hatch_brush = CreateHatchBrush(3, 0x808080);
  return result;
}

// FUNCTION: MAGIC 0x005357b4
// FUNCTION: SHANDALAR 0x00559ff4
void destroy_MAGICGAME_PhaseDisplayClass(LPCSTR class_name)
{
  if (g_phase_display_menu != (HMENU)0)
  {
    DestroyMenu(g_phase_display_menu);
  }
  g_phase_display_menu = (HMENU)0;
  if (g_magicgame_phase_display_pic != (HANDLE)0)
  {
    delete_and_close_object(g_magicgame_phase_display_pic);
  }
  if (g_phase_display_hatch_brush != (HBRUSH)0)
  {
    DeleteObject(g_phase_display_hatch_brush);
  }
  g_magicgame_phase_display_pic = (HANDLE)0;
  g_phase_display_hatch_brush = (HBRUSH)0;
}

// FUNCTION: MAGIC 0x0053592b
// FUNCTION: SHANDALAR 0x0055a16b
LRESULT CALLBACK wndproc_MAGICGAME_PhaseDisplayClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  int menu_remaining;
  int menu_count;
  int menu_hit_phase;
  int menu_values[0x26];
  POINT menu_cursor_point;
  int menu_help_command;
  int menu_player_offset;
  int menu_phase_index;
  int menu_stop_command;
  int menu_hit_player;
  int menu_toggle_command;
  RECT menu_client_rect;
  POINT popup_point;
  RECT popup_rect;
  int selected_half;
  char pic_path[264];
  HRGN paint_clip_region;
  HDC paint_dc;
  int paint_hit_phase;
  BITMAP paint_bitmap;
  PAINTSTRUCT paint;
  int paint_saved_dc;
  RECT paint_phase_rect;
  int paint_hit_player;
  RECT paint_client_rect;
  int click_hit_player;
  POINT click_point;
  RECT click_client_rect;
  int click_hit_phase;
  char phase_help_path[264];
  int phase_help_context;
  int toggle_phase_code;
  int toggle_player;
  RECT toggle_client_rect;
  RECT toggle_phase_rect;
  int stop_phase_code;
  int stop_player;
  int command_phase;
  int command_player;
  int command_base;
  int command_phase_index;
  char general_help_path[264];
  int general_help_context;
  int refresh_current_card;
  int refresh_current_player;
  char cue_text[100];
  int cue_hit_phase;
  POINT cue_point;
  int cue_valid;
  char opponent_name[100];
  int cue_hit_player;
  RECT cue_client_rect;
  int cached_card;
  int cached_player;

  switch (msg)
  {
  case 0x437:
    cue_point.x = lparam & 0xffff;
    cue_point.y = HIWORD(lparam);
    GetClientRect(hwnd, &cue_client_rect);
    hit_test_phase_display(&cue_point, &cue_client_rect, &cue_hit_player, &cue_hit_phase);
    cue_valid = 1;
    copy_opponent_name_prefix(opponent_name);
    switch (cue_hit_phase)
    {
    case 1:
      if (cue_hit_player == 1)
        sprintf(cue_text, gs_phasebar_opponent_untap_008a8d80, opponent_name);
      else
        strcpy(cue_text, gs_phasebar_your_untap_008b3280);
      break;
    case 2:
    case 3:
    case 4:
    case 5:
      if (cue_hit_player == 1)
        sprintf(cue_text, gs_phasebar_opponent_upkeep_008a9b30, opponent_name);
      else
        strcpy(cue_text, gs_phasebar_your_upkeep_007a7c20);
      break;
    case 0xa:
      if (cue_hit_player == 1)
        sprintf(cue_text, gs_phasebar_opponent_draw_0091c490, opponent_name);
      else
        strcpy(cue_text, gs_phasebar_your_draw_0091b110);
      break;
    case 0x14:
      if (cue_hit_player == 1)
        sprintf(cue_text, gs_phasebar_opponent_main_precombat_00924790, opponent_name);
      else
        strcpy(cue_text, gs_phasebar_your_main_precombat_008b2840);
      break;
    case 0x15:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1a:
    case 0x1b:
      if (cue_hit_player == 1)
        sprintf(cue_text, gs_phasebar_opponent_main_combat_00926380, opponent_name);
      else
        strcpy(cue_text, gs_phasebar_your_main_declare_combat_007a7cd0);
      break;
    case 0x1e:
      if (cue_hit_player == 1)
        sprintf(cue_text, gs_phasebar_opponent_main_postcombat_00925fa0, opponent_name);
      else
        strcpy(cue_text, gs_phasebar_your_main_postcombat_00926670);
      break;
    case 0x1f:
      if (cue_hit_player == 1)
        sprintf(cue_text, gs_phasebar_opponent_discard_00938e40, opponent_name);
      else
        strcpy(cue_text, gs_phasebar_your_discard_00939350);
      break;
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x25:
      if (cue_hit_player == 1)
        sprintf(cue_text, gs_phasebar_opponent_cleanup_007aae40, opponent_name);
      else
        strcpy(cue_text, gs_phasebar_your_cleanup_00789b90);
      break;
    default:
      cue_valid = 0;
      break;
    }
    if (cue_valid != 0)
      strcpy((char *)wparam, cue_text);
    return cue_valid;

  case 0x432:
    cached_card = GetWindowLongA(hwnd, g_phase_display_selected_card_window_long_offset);
    cached_player = GetWindowLongA(hwnd, g_phase_display_selected_player_window_long_offset);
    get_current_duel_selection(&refresh_current_player, &refresh_current_card);
    if (cached_card != refresh_current_card || cached_player != refresh_current_player)
      InvalidateRect(hwnd, (RECT *)0, 0);
    return 0;

  case 0x400:
    noop_phase_display_user_message(hwnd, wparam, lparam);
    return 0;

  case WM_COMMAND:
    switch ((unsigned int)wparam & 0xffff)
    {
    case PHASE_DISPLAY_COMMAND_HELP:
      general_help_context = 0x7e5;
      strcpy(general_help_path, global_base_directory);
      strcat(general_help_path, "\\duel.hlp");
      WinHelpA(g_duel_window_hwnd, general_help_path, HELP_CONTEXT, general_help_context);
      break;

    default:
      command_phase = (unsigned int)wparam & 0xffff;
      if (command_phase >= PHASE_DISPLAY_COMMAND_HELP_BASE)
        command_base = PHASE_DISPLAY_COMMAND_HELP_BASE;
      else if (command_phase >= PHASE_DISPLAY_COMMAND_TOGGLE_BASE)
        command_base = PHASE_DISPLAY_COMMAND_TOGGLE_BASE;
      else
        command_base = PHASE_DISPLAY_COMMAND_STOP_BASE;

      command_phase -= command_base;
      if (command_phase >= PHASE_DISPLAY_PLAYER_COMMAND_OFFSET)
      {
        command_player = 1;
        command_phase -= PHASE_DISPLAY_PLAYER_COMMAND_OFFSET;
      }
      else
      {
        command_player = 0;
      }

      command_phase_index = command_phase;
      switch (command_base)
      {
      case PHASE_DISPLAY_COMMAND_STOP_BASE:
        if ((g_duel_network_flags & 2) != 0)
          break;
        stop_player = command_player;
        if (command_phase_index == 0)
          stop_phase_code = 1;
        else if (command_phase_index == 1)
          stop_phase_code = 4;
        else if (command_phase_index == 2)
          stop_phase_code = 0xa;
        else if (command_phase_index == 3)
          stop_phase_code = 0x14;
        else if (command_phase_index == 4)
        {
          if (stop_player == 0)
            stop_phase_code = 0x15;
          else
            stop_phase_code = 0x16;
        }
        else if (command_phase_index == 5)
          stop_phase_code = 0x1e;
        else if (command_phase_index == 6)
          stop_phase_code = 0x1f;
        else if (command_phase_index == 7)
          stop_phase_code = 0x20;
        g_stop_phase_player = stop_player;
        g_stop_phase = stop_phase_code;
        g_recorded_action_player = 0;
        g_phase_display_menu_packet.selection_code = -2;
        g_phase_display_menu_packet.target_player = -1;
        g_phase_display_menu_packet.target_card = -1;
        g_phase_display_menu_packet.unused = 0;
        PostMessageA(g_duel_window_hwnd, 0x464, 0, (LPARAM)&g_phase_display_menu_packet);
        break;

      case PHASE_DISPLAY_COMMAND_TOGGLE_BASE:
        toggle_player = command_player;
        if (command_phase_index == 3 && g_active_player == toggle_player)
          break;
        if (command_phase_index == 0)
          toggle_phase_code = 1;
        else if (command_phase_index == 1)
          toggle_phase_code = 4;
        else if (command_phase_index == 2)
          toggle_phase_code = 0xa;
        else if (command_phase_index == 3)
          toggle_phase_code = 0x14;
        else if (command_phase_index == 4)
        {
          if (toggle_player == 0)
            toggle_phase_code = 0x15;
          else
            toggle_phase_code = 0x16;
        }
        else if (command_phase_index == 5)
          toggle_phase_code = 0x1e;
        else if (command_phase_index == 6)
          toggle_phase_code = 0x1f;
        else if (command_phase_index == 7)
          toggle_phase_code = 0x22;
        if (((int)(char)g_duel_phase_stop_settings[toggle_player].phase_flags[toggle_phase_code] & PHASE_STOP_ENABLED) != 0)
          g_duel_phase_stop_settings[toggle_player].phase_flags[toggle_phase_code] =
              (unsigned char)(((int)(char)g_duel_phase_stop_settings[toggle_player].phase_flags[toggle_phase_code]) & ~PHASE_STOP_ENABLED);
        else
          g_duel_phase_stop_settings[toggle_player].phase_flags[toggle_phase_code] =
              (unsigned char)(((int)(char)g_duel_phase_stop_settings[toggle_player].phase_flags[toggle_phase_code]) | PHASE_STOP_ENABLED);
        save_duel_interface_options_to_registry();
        refresh_duel_display_cache();
        GetClientRect(hwnd, &toggle_client_rect);
        get_phase_display_phase_rect(&toggle_phase_rect, toggle_player, toggle_phase_code,
                                     toggle_client_rect.right, toggle_client_rect.bottom);
        InvalidateRect(hwnd, &toggle_phase_rect, 0);
        break;

      case PHASE_DISPLAY_COMMAND_HELP_BASE:
        if (command_phase_index == 0)
          phase_help_context = 0x7da;
        else if (command_phase_index == 1)
          phase_help_context = 0x7db;
        else if (command_phase_index == 2)
          phase_help_context = 0x7dc;
        else if (command_phase_index == 3)
          phase_help_context = 0x7dd;
        else if (command_phase_index == 4)
          phase_help_context = 0x7dd;
        else if (command_phase_index == 5)
          phase_help_context = 0x7dd;
        else if (command_phase_index == 6)
          phase_help_context = 0x7de;
        else if (command_phase_index == 7)
          phase_help_context = 0x7df;
        strcpy(phase_help_path, global_base_directory);
        strcat(phase_help_path, "\\duel.hlp");
        WinHelpA(g_duel_window_hwnd, phase_help_path, HELP_CONTEXT, phase_help_context);
        break;
      }
      break;
    }
    return 0;

  case WM_CREATE:
    cached_card = 0;
    cached_player = 0;
    SetWindowLongA(hwnd, g_phase_display_selected_card_window_long_offset, cached_card);
    SetWindowLongA(hwnd, g_phase_display_selected_player_window_long_offset, cached_player);
    return 0;

  case WM_ERASEBKGND:
    return 1;

  case WM_LBUTTONDOWN:
    click_point.x = lparam & 0xffff;
    click_point.y = HIWORD(lparam);
    GetClientRect(hwnd, &click_client_rect);
    hit_test_phase_display(&click_point, &click_client_rect, &click_hit_player, &click_hit_phase);
    command_phase_index = get_phase_display_command_index(click_hit_phase);
    if (command_phase_index != -1)
    {
      SendMessageA(hwnd, WM_COMMAND,
                   PHASE_DISPLAY_COMMAND_TOGGLE_BASE + command_phase_index +
                       (click_hit_player == 1 ? PHASE_DISPLAY_PLAYER_COMMAND_OFFSET : 0),
                   0);
    }
    return 0;

  case WM_PAINT:
    cached_card = GetWindowLongA(hwnd, g_phase_display_selected_card_window_long_offset);
    cached_player = GetWindowLongA(hwnd, g_phase_display_selected_player_window_long_offset);
    get_current_duel_selection(&paint_hit_player, &paint_hit_phase);
    if (paint_hit_phase != cached_card || paint_hit_player != cached_player)
      InvalidateRect(hwnd, (RECT *)0, 0);

    if (g_magicgame_phase_display_pic == (HANDLE)0)
    {
      strcpy(pic_path, global_duelart_path);
      strcat(pic_path, "\\WINBK_Phase.pic");
      g_magicgame_phase_display_pic = load_pic(pic_path);
    }

    EnterCriticalSection(&g_card_render_lock);
    paint_dc = g_shared_offscreen_dc;
    paint_saved_dc = SaveDC(g_shared_offscreen_dc);
    GetClientRect(hwnd, &paint_client_rect);
    if (g_magicgame_phase_display_pic != (HANDLE)0)
    {
      GetObjectA(g_magicgame_phase_display_pic, sizeof(BITMAP), &paint_bitmap);
      DrawBitmapSubrectToRect(paint_dc, &paint_client_rect, g_magicgame_phase_display_pic, 0, 0,
                              paint_bitmap.bmWidth / g_phase_display_bitmap_divisor,
                              paint_bitmap.bmHeight);
    }
    else
    {
      FillRect(paint_dc, &paint_client_rect, GetStockObject(4));
    }

    if (paint_hit_player != -1 && paint_hit_phase != -1)
    {
      get_phase_display_phase_rect(&paint_phase_rect, paint_hit_player, paint_hit_phase,
                                   paint_client_rect.right, paint_client_rect.bottom);
      paint_clip_region = CreateRectRgnIndirect(&paint_phase_rect);
      SelectClipRgn(paint_dc, paint_clip_region);
      if (g_magicgame_phase_display_pic != (HANDLE)0)
      {
        GetObjectA(g_magicgame_phase_display_pic, sizeof(BITMAP), &paint_bitmap);
        DrawBitmapSubrectToRect(paint_dc, &paint_client_rect, g_magicgame_phase_display_pic,
                                paint_bitmap.bmWidth -
                                    (paint_bitmap.bmWidth / g_phase_display_bitmap_divisor),
                                0, paint_bitmap.bmWidth / g_phase_display_bitmap_divisor,
                                paint_bitmap.bmHeight);
      }
      else
      {
        FillRect(paint_dc, &paint_client_rect, GetStockObject(2));
      }
      SelectClipRgn(paint_dc, (HRGN)0);
      DeleteObject(paint_clip_region);
    }

    RestoreDC(g_shared_offscreen_dc, paint_saved_dc);
    draw_phase_display_status_markers(paint_dc, &paint_client_rect);
    paint_dc = BeginPaint(hwnd, &paint);
    if (paint_dc != (HDC)0)
    {
      ApplyCardArtPaletteToDc(paint_dc);
      GetClientRect(hwnd, &paint_client_rect);
      if (g_duel_palette_refresh_pending != 0)
      {
        FillRect(paint_dc, &paint_client_rect, GetStockObject(WHITE_BRUSH));
        Sleep(200);
      }
      BitBlt(paint_dc, 0, 0, paint_client_rect.right, paint_client_rect.bottom,
             g_shared_offscreen_dc, 0, 0, SRCCOPY);
      get_phase_display_selected_half(&selected_half);
      if (selected_half != -1)
      {
        if (selected_half == 0)
          paint_client_rect.bottom -=
              (paint_client_rect.bottom - paint_client_rect.top) / 2;
        else
          paint_client_rect.top +=
              (paint_client_rect.bottom - paint_client_rect.top) / 2;
        SelectObject(paint_dc, g_phase_display_hatch_brush);
        SetBkMode(paint_dc, TRANSPARENT);
        Rectangle(paint_dc, paint_client_rect.left, paint_client_rect.top,
                  paint_client_rect.right, paint_client_rect.bottom);
      }
      EndPaint(hwnd, &paint);
      cached_card = paint_hit_phase;
      cached_player = paint_hit_player;
      SetWindowLongA(hwnd, g_phase_display_selected_card_window_long_offset, cached_card);
      SetWindowLongA(hwnd, g_phase_display_selected_player_window_long_offset, cached_player);
    }
    LeaveCriticalSection(&g_card_render_lock);
    return 0;

  case WM_RBUTTONDOWN:
    popup_point.x = lparam & 0xffff;
    popup_point.y = HIWORD(lparam);
    ClientToScreen(hwnd, &popup_point);
    SetRect(&popup_rect, popup_point.x, popup_point.y,
            popup_point.x + 1, popup_point.y + 1);
    TrackPopupMenu(g_phase_display_menu, TPM_RIGHTBUTTON, popup_point.x, popup_point.y,
                   0, hwnd, &popup_rect);
    return 0;

  case WM_INITMENU:
    GetCursorPos(&menu_cursor_point);
    ScreenToClient(hwnd, &menu_cursor_point);
    GetClientRect(hwnd, &menu_client_rect);
    hit_test_phase_display(&menu_cursor_point, &menu_client_rect,
                           &menu_hit_player, &menu_hit_phase);
    menu_player_offset = menu_hit_player == 1 ? PHASE_DISPLAY_PLAYER_COMMAND_OFFSET : 0;
    if (menu_hit_phase == 1)
      menu_phase_index = 0;
    else if (menu_hit_phase == 2 || menu_hit_phase == 3 ||
             menu_hit_phase == 4 || menu_hit_phase == 5)
      menu_phase_index = 1;
    else if (menu_hit_phase == 0xa)
      menu_phase_index = 2;
    else if (menu_hit_phase == 0x14)
      menu_phase_index = 3;
    else if (menu_hit_phase == 0x15 || menu_hit_phase == 0x16 ||
             menu_hit_phase == 0x17 || menu_hit_phase == 0x18 ||
             menu_hit_phase == 0x19 || menu_hit_phase == 0x1a ||
             menu_hit_phase == 0x1b)
      menu_phase_index = 4;
    else if (menu_hit_phase == 0x1e)
      menu_phase_index = 5;
    else if (menu_hit_phase == 0x1f)
      menu_phase_index = 6;
    else if (menu_hit_phase == 0x20 || menu_hit_phase == 0x21 ||
             menu_hit_phase == 0x22 || menu_hit_phase == 0x25)
      menu_phase_index = 7;
    else
      menu_phase_index = -1;
    if (menu_hit_phase != -1)
    {
      if (g_duel_modal_action_active != 0)
      {
        menu_stop_command =
            menu_phase_index + menu_player_offset + PHASE_DISPLAY_COMMAND_STOP_BASE;
        if ((g_duel_network_flags & 2) == 0)
          AppendMenuA(g_phase_display_menu, 0, menu_stop_command, g_phase_display_menu_stop_text);
      }
      if (GetMenuItemCount(g_phase_display_menu) != 0)
        AppendMenuA(g_phase_display_menu, MF_SEPARATOR, 0, (LPCSTR)0);
      menu_toggle_command =
          menu_phase_index + menu_player_offset + PHASE_DISPLAY_COMMAND_TOGGLE_BASE;
      AppendMenuA(g_phase_display_menu, 0, menu_toggle_command, g_phase_display_menu_toggle_text);
      copy_phase_stop_flags(menu_values, menu_hit_player);
      if (menu_values[menu_hit_phase] != 0)
        CheckMenuItem(g_phase_display_menu, menu_toggle_command, MF_CHECKED);
      menu_help_command =
          menu_phase_index + menu_player_offset + PHASE_DISPLAY_COMMAND_HELP_BASE;
      AppendMenuA(g_phase_display_menu, 0, menu_help_command, g_phase_display_menu_phase_help_text);
    }
    AppendMenuA(g_phase_display_menu, 0, PHASE_DISPLAY_COMMAND_HELP, g_phase_display_menu_help_text);
    return 0;

  case WM_MENUSELECT:
    if (HIWORD(wparam) == 0xffff && lparam == 0)
    {
      menu_count = GetMenuItemCount(g_phase_display_menu);
      while (menu_remaining = menu_count--)
      {
        DeleteMenu(g_phase_display_menu, 0, MF_BYPOSITION);
      }
    }
    return 0;

  case WM_SETCURSOR:
    return handle_duel_inactive_cursor(hwnd, msg, wparam, lparam);

  case 0x30f:
  case 0x310:
  case 0x311:
    return handle_button_palette_message(hwnd, msg, (HWND)wparam, lparam);

  default:
    return DefWindowProcA(hwnd, msg, wparam, lparam);
  }

  return 0;
}
