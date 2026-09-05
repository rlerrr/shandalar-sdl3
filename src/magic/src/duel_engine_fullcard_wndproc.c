#include <windows.h>
#include <stdlib.h>
#include <string.h>

#include "cardartlib/src/palette.h"
#include "drawcardlib/Drawcardlib.h"
#include "magic/src/duel_engine.h"
#include "magic/src/game_support.h"
#include "magic/src/global_duel_ui_ids.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "magic/src/shared_startup.h"

#define DISPLAYED_PLAYER_CARD_INSTANCE(player_, card_) global_displayed_card_instances[(player_)][(card_)]

#define DUEL_MAIN_WINDOW_HWND g_duel_window_hwnd

#ifdef SHANDALAR_SDL_WIN32_COMPAT
#define FULL_CARD_LONG_PRESS_TIMER_ID 2
#define FULL_CARD_LONG_PRESS_DEAD_ZONE 8
static HWND g_full_card_press_window;
static POINT g_full_card_press_origin;
static int g_full_card_press_timer_active;
static int g_full_card_long_press_triggered;
#endif

extern int global_available_slots;
extern card_ptr_t global_raw_cards_storage[2000];
extern char global_base_directory[];
extern CRITICAL_SECTION g_card_render_lock;

int load_text_with_tab_escapes(char *filename, char *section_name);
LRESULT handle_duel_inactive_cursor(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
unsigned int get_displayed_card_special_counters(int player, int card);
card_id_t get_displayed_card_id(int player, int card);
int get_displayed_card_internal_id(int player, int card);
int displayed_card_indices_invalid(int player, int card);
int get_displayed_card_color_flags(int player, int card);
void get_displayed_card_counter_power_toughness(int player, int card, int *power, int *toughness);
unsigned int draw_displayed_full_card(HDC dc, RECT *rect, card_ptr_t *raw_card, int player, int card, int draw_mode, int expand_text_box);
void draw_displayed_card_overlaid_full_card(HDC dc, RECT *rect, int player, int card);
void draw_special_effect_full_card(HDC dc, RECT *rect, card_id_t card_id, int player, int card);
void save_duel_interface_options_to_registry(void);

// GLOBAL: MAGIC 0x0055e1c8
// GLOBAL: SHANDALAR 0x0057f184
int g_full_card_card_id_window_long_offset = 0;

// GLOBAL: MAGIC 0x0055e1cc
// GLOBAL: SHANDALAR 0x0057f188
int g_full_card_player_window_long_offset = 4;

// GLOBAL: MAGIC 0x0055e1d0
// GLOBAL: SHANDALAR 0x0057f18c
int g_full_card_card_window_long_offset = 8;

// GLOBAL: MAGIC 0x0055e1d4
// GLOBAL: SHANDALAR 0x0057f190
int g_full_card_counters_window_long_offset = 0xc;

// GLOBAL: MAGIC 0x0055e1d8
// GLOBAL: SHANDALAR 0x0057f194
int g_full_card_color_window_long_offset = 0x10;

// GLOBAL: MAGIC 0x0055e1dc
// GLOBAL: SHANDALAR 0x0057f198
int g_full_card_window_extra_bytes = 0x14;

// GLOBAL: MAGIC 0x0057f440
// GLOBAL: SHANDALAR 0x005a0d94
int g_full_card_preview_repositioned = 0;

// GLOBAL: MAGIC 0x00708b10
// GLOBAL: SHANDALAR 0x00732188
char g_full_card_menu_help_text[0x38];

// GLOBAL: MAGIC 0x00708b48
// GLOBAL: SHANDALAR 0x007321c0
char g_full_card_menu_expand_text[0x38];

// GLOBAL: MAGIC 0x00708b80
// GLOBAL: SHANDALAR 0x007321f8
char g_full_card_menu_more_help_text[0x1c];

// GLOBAL: MAGIC 0x00708ba0
// GLOBAL: SHANDALAR 0x00732218
RECT g_full_card_saved_window_rect;

// FUNCTION: MAGIC 0x00557850
// FUNCTION: SHANDALAR 0x0056dbb0
int register_MAGICGAME_FullCardClass(LPCSTR class_name)
{
  int result;
  WNDCLASSA wndclass;

  result = 1;
  wndclass.style = 0x803;
  wndclass.lpfnWndProc = wndproc_MAGICGAME_FullCardClass;
  wndclass.cbClsExtra = 0;
  wndclass.cbWndExtra = g_full_card_window_extra_bytes;
  wndclass.hInstance = g_app_instance;
  wndclass.hIcon = (HICON)0;
  wndclass.hCursor = LoadCursorA((HINSTANCE)0, (LPCSTR)0x7f00);
  wndclass.hbrBackground = GetStockObject(4);
  wndclass.lpszMenuName = (LPCSTR)0;
  wndclass.lpszClassName = class_name;

  if (RegisterClassA(&wndclass) == 0)
  {
    result = 0;
  }
  if (g_magicgame_full_card_menu == (HMENU)0)
    g_magicgame_full_card_menu = CreatePopupMenu();
  load_text_with_tab_escapes(global_ui_strings_filename, "MENU_FULLCARD");
  strcpy(g_full_card_menu_expand_text, g_text_lines[0]);
  strcpy(g_full_card_menu_help_text, g_text_lines[1]);
  strcpy(g_full_card_menu_more_help_text, g_text_lines[2]);
  return result;
}

// FUNCTION: MAGIC 0x0055793c
// FUNCTION: SHANDALAR 0x0056dc9c
void destroy_MAGICGAME_FullCardClass(LPCSTR class_name)
{
  if (g_magicgame_full_card_menu != (HMENU)0)
  {
    DestroyMenu(g_magicgame_full_card_menu);
  }

  g_magicgame_full_card_menu = (HMENU)0;
}

// FUNCTION: MAGIC 0x004493ec
// FUNCTION: SHANDALAR 0x00452f69
int displayed_card_has_changed_color_words(int player, int card)
{
  int result;
  int internal_card_id;

  if (displayed_card_indices_invalid(player, card) != 0)
  {
    return 0;
  }

  internal_card_id = get_displayed_card_internal_id(player, card);
  if (internal_card_id == -1)
  {
    return 0;
  }

  EnterCriticalSection(&g_duel_render_lock);
  if ((DISPLAYED_PLAYER_CARD_INSTANCE(player, card).token_status & (STATUS_HACKED | STATUS_SLEIGHTED)) != 0)
  {
    result = 1;
  }
  else
  {
    result = 0;
  }
  LeaveCriticalSection(&g_duel_render_lock);
  return result;
}

// FUNCTION: MAGIC 0x0055796a
// FUNCTION: SHANDALAR 0x0056dcca
LRESULT CALLBACK wndproc_MAGICGAME_FullCardClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  POINT point;
  int windowpos_scaled_height;
  int windowpos_cx;
  int windowpos_scaled_width;
  WINDOWPOS *window_pos;
  POINT timer_cursor;
  int menu_item_count;
  POINT popup_point;
  RECT popup_rect;
  unsigned int draw_result;
  int paint_toughness;
  unsigned int paint_power;
  HDC paint_dc;
  PAINTSTRUCT paint;
  int paint_color;
  RECT client_rect;
  unsigned int paint_counters;
  DWORD paint_start_tick;
  unsigned int mouse_x;
  unsigned int mouse_y;
  RECT mouse_rect;
  int create_height;
  int adjusted_height;
  int create_width;
  int adjusted_width;
  CREATESTRUCTA *create_struct;
  char more_help_path[264];
  DWORD more_help_context;
  char help_path[264];
  DWORD help_context;
  HWND remove_packet;
  int changed;
  int *update_card;
  int displayed_toughness;
  unsigned int displayed_power;
  int color;
  unsigned int counters;
  HWND previous_window_card_id;
  int card;
  unsigned int cached_counters;
  int player;
  LONG cached_color;
  HWND window_card_id;

  switch (msg)
  {
  case 0x437:
    return 0;

  case 0x433:
  case 0x434:
    previous_window_card_id = (HWND)wparam;
    previous_window_card_id = (HWND)GetWindowLongA(hwnd, g_full_card_card_id_window_long_offset);
    if (previous_window_card_id == previous_window_card_id)
    {
      InvalidateRect(hwnd, (RECT *)0, 0);
    }
    return 0;

  case 0x432:
    window_card_id = (HWND)GetWindowLongA(hwnd, g_full_card_card_id_window_long_offset);
    player = GetWindowLongA(hwnd, g_full_card_player_window_long_offset);
    card = GetWindowLongA(hwnd, g_full_card_card_window_long_offset);
    cached_counters = GetWindowLongA(hwnd, g_full_card_counters_window_long_offset);
    if (window_card_id == (HWND)g_damage_display_internal_card_id)
    {
      counters = get_displayed_card_special_counters(player, card);
    }
    else if (window_card_id == (HWND)g_effect_display_internal_card_id)
    {
      get_displayed_card_counter_power_toughness(player, card, (int *)&displayed_power, &displayed_toughness);
      counters = MAKELONG(displayed_power, displayed_toughness);
    }
    else
    {
      counters = 0;
    }

    cached_color = GetWindowLongA(hwnd, g_full_card_color_window_long_offset);
    if (player != -1 && card != -1)
    {
      color = single_color_test_bit_to_color_t(get_displayed_card_color_flags(player, card));
    }
    else
    {
      color = 0;
    }

    if (cached_counters != counters)
    {
      InvalidateRect(hwnd, (RECT *)0, 0);
    }
    if (cached_color != color)
    {
      InvalidateRect(hwnd, (RECT *)0, 0);
    }
    if (player != -1 && card != -1)
    {
      if (displayed_card_has_changed_color_words(player, card) != 0)
      {
        InvalidateRect(hwnd, (RECT *)0, 0);
      }
      if ((HWND)get_displayed_card_id(player, card) != window_card_id)
      {
        InvalidateRect(hwnd, (RECT *)0, 0);
      }
    }
    return 0;

  case 0x401:
    window_card_id = (HWND)wparam;
    update_card = (int *)lparam;
    if ((window_card_id == (HWND)g_damage_display_internal_card_id ||
         (HWND)g_hunting_display_internal_card_id == window_card_id ||
         (HWND)g_multiblock_display_internal_card_id == window_card_id ||
         window_card_id == (HWND)g_effect_display_internal_card_id ||
         window_card_id == (HWND)g_legacy_display_internal_card_id) &&
        (update_card == (int *)0 || update_card[0] == -1 || update_card[1] == -1))
    {
      return 0;
    }
    if (update_card != (int *)0 && update_card[0] != -1 && update_card[1] == -1)
    {
      return 0;
    }

    if (update_card != (int *)0)
    {
      player = update_card[0];
      card = update_card[1];
    }
    else
    {
      player = -1;
      card = -1;
    }

    changed = 0;
    if ((HWND)GetWindowLongA(hwnd, g_full_card_card_id_window_long_offset) != window_card_id)
    {
      changed = 1;
    }
    if (GetWindowLongA(hwnd, g_full_card_player_window_long_offset) != player ||
        GetWindowLongA(hwnd, g_full_card_card_window_long_offset) != card)
    {
      changed = 1;
    }

    if (changed != 0)
    {
      SetWindowLongA(hwnd, g_full_card_card_id_window_long_offset, (LONG)window_card_id);
      SetWindowLongA(hwnd, g_full_card_player_window_long_offset, player);
      SetWindowLongA(hwnd, g_full_card_card_window_long_offset, card);
      InvalidateRect(hwnd, (RECT *)0, 0);
    }
    else
    {
      SendMessageA(hwnd, 0x432, 0, 0);
    }

    if (g_duel_interface_options.layout == 2)
    {
      ShowWindow(hwnd, SW_SHOW);
      if (g_duel_card_preview_window_hwnd == hwnd)
      {
        SetTimer(hwnd, 1, 12000, (TIMERPROC)0);
      }
      BringWindowToTop(hwnd);
    }
    return 0;

  case 0x40b:
    remove_packet = (HWND)wparam;
    player = GetWindowLongA(hwnd, g_full_card_player_window_long_offset);
    card = GetWindowLongA(hwnd, g_full_card_card_window_long_offset);
    if (((int *)remove_packet)[0] == player && ((int *)remove_packet)[1] == card)
    {
      SendMessageA(hwnd, 0x401, 0xffffffff, 0);
      return 1;
    }
    else
    {
      return 0;
    }

  case WM_COMMAND:
    switch ((UINT)wparam & 0xffff)
    {
    case 1:
      g_duel_interface_options.expand_text_box_on_big_card = (g_duel_interface_options.expand_text_box_on_big_card == 0);
      save_duel_interface_options_to_registry();
      InvalidateRect(hwnd, (RECT *)0, 0);
      break;

    case 100:
      help_context = GetWindowLongA(hwnd, g_full_card_card_id_window_long_offset);
      if (help_context == (DWORD)g_card_back_display_internal_card_id)
      {
        help_context = 0xc1b;
      }
      if (help_context != 0xffffffff)
      {
        strcpy(help_path, global_base_directory);
        strcat(help_path, "\\duel.hlp");
        WinHelpA(DUEL_MAIN_WINDOW_HWND, help_path, HELP_CONTEXT, help_context);
      }
      break;

    case 0x65:
      more_help_context = 0x7e7;
      strcpy(more_help_path, global_base_directory);
      strcat(more_help_path, "\\duel.hlp");
      WinHelpA(DUEL_MAIN_WINDOW_HWND, more_help_path, HELP_CONTEXT, more_help_context);
      break;

    default:
      break;
    }
    return 0;

  case WM_CREATE:
    window_card_id = (HWND)-1;
    SetWindowLongA(hwnd, g_full_card_card_id_window_long_offset, (LONG)window_card_id);
    player = -1;
    card = -1;
    SetWindowLongA(hwnd, g_full_card_player_window_long_offset, player);
    SetWindowLongA(hwnd, g_full_card_card_window_long_offset, card);
    cached_counters = 0;
    SetWindowLongA(hwnd, g_full_card_counters_window_long_offset, cached_counters);
    cached_color = 0;
    SetWindowLongA(hwnd, g_full_card_color_window_long_offset, cached_color);
    create_struct = (CREATESTRUCTA *)lparam;
    create_width = create_struct->cx;
    create_height = create_struct->cy;
    adjusted_height = (create_height * 200) / 300;
    adjusted_width = (create_width * 300) / 200;
    if (abs(adjusted_height - create_width) + abs(adjusted_width - create_height) > 4)
    {
      if (adjusted_height < create_width)
      {
        SetWindowPos(hwnd, (HWND)0, 0, 0, adjusted_height, create_height, 6);
      }
      else
      {
        SetWindowPos(hwnd, (HWND)0, 0, 0, create_width, adjusted_width, 6);
      }
    }
    return 0;

  case WM_ERASEBKGND:
    return 1;

  case WM_LBUTTONDOWN:
#ifdef SHANDALAR_SDL_WIN32_COMPAT
    if (g_full_card_press_window != (HWND)0)
      KillTimer(g_full_card_press_window, FULL_CARD_LONG_PRESS_TIMER_ID);
    g_full_card_press_window = hwnd;
    g_full_card_press_origin.x = (short)LOWORD(lparam);
    g_full_card_press_origin.y = (short)HIWORD(lparam);
    g_full_card_press_timer_active = 1;
    g_full_card_long_press_triggered = 0;
    SetCapture(hwnd);
    SetTimer(hwnd, FULL_CARD_LONG_PRESS_TIMER_ID, GetDoubleClickTime(), (TIMERPROC)0);
#else
    if (g_duel_interface_options.layout == 2)
    {
      ShowWindow(hwnd, SW_HIDE);
      KillTimer(hwnd, 1);
    }
#endif
    return 0;

#ifdef SHANDALAR_SDL_WIN32_COMPAT
  case WM_LBUTTONUP:
    if (g_full_card_press_window == hwnd)
    {
      KillTimer(hwnd, FULL_CARD_LONG_PRESS_TIMER_ID);
      if (GetCapture() == hwnd)
        ReleaseCapture();
      if (g_full_card_long_press_triggered == 0 &&
          g_duel_interface_options.layout == 2)
      {
        ShowWindow(hwnd, SW_HIDE);
        KillTimer(hwnd, 1);
      }
      g_full_card_press_window = (HWND)0;
      g_full_card_press_timer_active = 0;
      g_full_card_long_press_triggered = 0;
    }
    return 0;
#endif

  case WM_MOUSEMOVE:
#ifdef SHANDALAR_SDL_WIN32_COMPAT
    if (g_full_card_press_window == hwnd &&
        g_full_card_press_timer_active != 0 &&
        (wparam & MK_LBUTTON) != 0)
    {
      point.x = (short)LOWORD(lparam);
      point.y = (short)HIWORD(lparam);
      if (abs(point.x - g_full_card_press_origin.x) > FULL_CARD_LONG_PRESS_DEAD_ZONE ||
          abs(point.y - g_full_card_press_origin.y) > FULL_CARD_LONG_PRESS_DEAD_ZONE)
      {
        KillTimer(hwnd, FULL_CARD_LONG_PRESS_TIMER_ID);
        g_full_card_press_timer_active = 0;
      }
    }
#endif
    if (GetParent(hwnd) != DUEL_MAIN_WINDOW_HWND)
    {
      return 0;
    }

    return 0;

    if (g_duel_cheats_state == 0)
    {
      return 0;
    }

    mouse_x = (unsigned int)lparam & 0xffff;
    mouse_y = HIWORD(lparam);
    GetClientRect(hwnd, &mouse_rect);
    if ((mouse_rect.right * 90) / 100 > (int)mouse_x &&
        (mouse_rect.bottom * 10) / 100 < (int)mouse_y &&
        (mouse_rect.bottom * 90) / 100 > (int)mouse_y)
    {
      if (g_full_card_preview_repositioned == 0)
      {
        GetWindowRect(hwnd, &g_full_card_saved_window_rect);
        if ((GetWindowLongA(hwnd, GWL_STYLE) & WS_CHILD) != 0)
        {
          MapWindowPoints((HWND)0, GetParent(hwnd), (LPPOINT)&g_full_card_saved_window_rect, 2);
        }
        g_full_card_preview_repositioned = 1;
        CopyRect(&mouse_rect, &g_full_card_saved_window_rect);
        if (g_duel_interface_options.layout == 2)
        {
          mouse_rect.bottom += ((mouse_rect.bottom - mouse_rect.top) * 30) / 100;
        }
        else
        {
          mouse_rect.top -= ((mouse_rect.bottom - mouse_rect.top) * 15) / 100;
          mouse_rect.bottom += ((mouse_rect.bottom - mouse_rect.top) * 15) / 100;
        }
        mouse_rect.right += ((mouse_rect.right - mouse_rect.left) * 20) / 100;

        if ((GetWindowLongA(hwnd, GWL_STYLE) & WS_CHILD) != 0)
        {
          MapWindowPoints((HWND)0, GetParent(hwnd), (LPPOINT)&mouse_rect, 2);
        }
        BringWindowToTop(hwnd);
        MoveWindow(hwnd,
                   mouse_rect.left,
                   mouse_rect.top,
                   mouse_rect.right - mouse_rect.left,
                   mouse_rect.bottom - mouse_rect.top,
                   1);
      }
    }
    else if (g_full_card_preview_repositioned != 0)
    {
      g_full_card_preview_repositioned = 0;
      SetWindowPos(hwnd, (HWND)1, 0, 0, 0, 0, 3);
      MoveWindow(hwnd,
                 g_full_card_saved_window_rect.left,
                 g_full_card_saved_window_rect.top,
                 g_full_card_saved_window_rect.right - g_full_card_saved_window_rect.left,
                 g_full_card_saved_window_rect.bottom - g_full_card_saved_window_rect.top,
                 1);
    }
    return 0;

  case WM_PAINT:
    window_card_id = (HWND)GetWindowLongA(hwnd, g_full_card_card_id_window_long_offset);
    player = GetWindowLongA(hwnd, g_full_card_player_window_long_offset);
    card = GetWindowLongA(hwnd, g_full_card_card_window_long_offset);
    cached_counters = GetWindowLongA(hwnd, g_full_card_counters_window_long_offset);
    cached_color = GetWindowLongA(hwnd, g_full_card_color_window_long_offset);
    if ((window_card_id == (HWND)-1 ||
         window_card_id == (HWND)g_card_back_display_internal_card_id ||
         global_available_slots - 1 < (int)window_card_id) &&
        g_duel_interface_options.layout == 2 &&
        g_duel_card_preview_window_hwnd == hwnd)
    {
      SendMessageA(hwnd, WM_TIMER, 1, 0);
      return DefWindowProcA(hwnd, msg, wparam, lparam);
    }

    if (window_card_id == (HWND)g_damage_display_internal_card_id)
    {
      paint_counters = get_displayed_card_special_counters(player, card);
    }
    else if (window_card_id == (HWND)g_effect_display_internal_card_id)
    {
      get_displayed_card_counter_power_toughness(player, card, (int *)&paint_power, &paint_toughness);
      paint_counters = MAKELONG(paint_power, paint_toughness);
    }
    else
    {
      paint_counters = 0;
    }

    if (player != -1 && card != -1)
    {
      paint_color = single_color_test_bit_to_color_t(get_displayed_card_color_flags(player, card));
    }
    else
    {
      paint_color = 0;
    }
    if (cached_counters != paint_counters)
    {
      InvalidateRect(hwnd, (RECT *)0, 0);
    }
    if (cached_color != paint_color)
    {
      InvalidateRect(hwnd, (RECT *)0, 0);
    }

    EnterCriticalSection(&g_card_render_lock);
    paint_start_tick = GetTickCount();
    GetClientRect(hwnd, &client_rect);
    paint_dc = BeginPaint(hwnd, &paint);
    if (paint_dc != (HDC)0)
    {
      ApplyCardArtPaletteToDc(paint_dc);
      if (g_duel_palette_refresh_pending != 0)
      {
        FillRect(paint_dc, &client_rect, GetStockObject(0));
        Sleep(200);
      }

      if (global_available_slots - 1 < (int)window_card_id)
      {
        FillRect(g_shared_offscreen_dc, &client_rect, GetStockObject(4));
        DrawCardBack(g_shared_offscreen_dc, &client_rect);
        BitBlt(paint_dc, 0, 0, client_rect.right, client_rect.bottom,
               g_shared_offscreen_dc, 0, 0, SRCCOPY);
      }
      else if ((int)window_card_id < 0 || window_card_id == (HWND)g_card_back_display_internal_card_id)
      {
        FillRect(g_shared_offscreen_dc, &client_rect, GetStockObject(4));
        DrawCardBack(g_shared_offscreen_dc, &client_rect);
        BitBlt(paint_dc, 0, 0, client_rect.right, client_rect.bottom,
               g_shared_offscreen_dc, 0, 0, SRCCOPY);
      }
      else if (window_card_id == (HWND)g_damage_display_internal_card_id ||
               (HWND)g_hunting_display_internal_card_id == window_card_id ||
               (HWND)g_multiblock_display_internal_card_id == window_card_id ||
               window_card_id == (HWND)g_effect_display_internal_card_id ||
               window_card_id == (HWND)g_legacy_display_internal_card_id)
      {
        FillRect(g_shared_offscreen_dc, &client_rect, GetStockObject(4));
        if (get_displayed_card_id(player, card) != -1)
        {
          draw_special_effect_full_card(g_shared_offscreen_dc, &client_rect, (card_id_t)window_card_id, player, card);
        }
        else
        {
          DrawCardBack(g_shared_offscreen_dc, &client_rect);
        }
        BitBlt(paint_dc, 0, 0, client_rect.right, client_rect.bottom,
               g_shared_offscreen_dc, 0, 0, SRCCOPY);
      }
      else if (window_card_id == (HWND)g_activation_display_internal_card_id)
      {
        FillRect(g_shared_offscreen_dc, &client_rect, GetStockObject(4));
        if (get_displayed_card_id(player, card) != -1)
        {
          draw_displayed_card_overlaid_full_card(g_shared_offscreen_dc, &client_rect, player, card);
        }
        else
        {
          DrawCardBack(g_shared_offscreen_dc, &client_rect);
        }
        BitBlt(paint_dc, 0, 0, client_rect.right, client_rect.bottom,
               g_shared_offscreen_dc, 0, 0, SRCCOPY);
      }
      else
      {
        FillRect(g_shared_offscreen_dc, &client_rect, GetStockObject(4));
        if (player != -1 && card != -1 && get_displayed_card_id(player, card) != -1)
        {
          draw_result = draw_displayed_full_card(g_shared_offscreen_dc,
                                                 &client_rect,
                                                 global_raw_cards_storage + (int)window_card_id,
                                                 player,
                                                 card,
                                                 0,
                                                 g_duel_interface_options.expand_text_box_on_big_card);
        }
        else if (window_card_id != (HWND)-1)
        {
          draw_result = DrawFullCard(g_shared_offscreen_dc,
                                     &client_rect,
                                     global_raw_cards_storage + (int)window_card_id,
                                     0,
                                     0,
                                     g_duel_interface_options.expand_text_box_on_big_card,
                                     gs_illus_00789130);
        }
        else
        {
          DrawCardBack(g_shared_offscreen_dc, &client_rect);
          draw_result = 1;
        }
        BitBlt(paint_dc, 0, 0, client_rect.right, client_rect.bottom,
               g_shared_offscreen_dc, 0, 0, SRCCOPY);
        if (draw_result == 0)
        {
          if (player != -1 && card != -1 && get_displayed_card_id(player, card) != -1)
          {
            draw_displayed_full_card(g_shared_offscreen_dc,
                                     &client_rect,
                                     global_raw_cards_storage + (int)window_card_id,
                                     player,
                                     card,
                                     2,
                                     g_duel_interface_options.expand_text_box_on_big_card);
          }
          else if (window_card_id != (HWND)-1)
          {
            DrawFullCard(g_shared_offscreen_dc,
                         &client_rect,
                         global_raw_cards_storage + (int)window_card_id,
                         0,
                         2,
                         g_duel_interface_options.expand_text_box_on_big_card,
                         gs_illus_00789130);
          }
          BitBlt(paint_dc, 0, 0, client_rect.right, client_rect.bottom,
                 g_shared_offscreen_dc, 0, 0, SRCCOPY);
        }
      }
      EndPaint(hwnd, &paint);
      cached_counters = paint_counters;
      SetWindowLongA(hwnd, g_full_card_counters_window_long_offset, cached_counters);
      cached_color = paint_color;
      SetWindowLongA(hwnd, g_full_card_color_window_long_offset, cached_color);
    }
    g_duel_tick_adjustment += (int)(GetTickCount() - paint_start_tick);
    LeaveCriticalSection(&g_card_render_lock);
    return 0;

  case WM_RBUTTONDOWN:
    popup_point.x = (unsigned int)lparam & 0xffff;
    popup_point.y = HIWORD(lparam);
    ClientToScreen(hwnd, &popup_point);
    SetRect(&popup_rect,
            popup_point.x,
            popup_point.y,
            popup_point.x + 1,
            popup_point.y + 1);
    TrackPopupMenu(g_magicgame_full_card_menu, 2, popup_point.x, popup_point.y, 0, hwnd, &popup_rect);
    return 0;

  case WM_INITMENU:
    AppendMenuA(g_magicgame_full_card_menu, 0, 1, g_full_card_menu_expand_text);
    if (g_duel_interface_options.expand_text_box_on_big_card != 0)
    {
      CheckMenuItem(g_magicgame_full_card_menu, 1, MF_CHECKED);
    }
    AppendMenuA(g_magicgame_full_card_menu, 0, 100, g_full_card_menu_help_text);
    AppendMenuA(g_magicgame_full_card_menu, 0, 0x65, g_full_card_menu_more_help_text);
    return 0;

  case WM_MENUSELECT:
    if (HIWORD(wparam) == 0xffff && lparam == 0)
    {
      menu_item_count = GetMenuItemCount(g_magicgame_full_card_menu);
      while ((point.x = menu_item_count--) != 0)
      {
        DeleteMenu(g_magicgame_full_card_menu, 0, MF_BYPOSITION);
      }
    }
    return 0;

  case WM_TIMER:
#ifdef SHANDALAR_SDL_WIN32_COMPAT
    if (wparam == FULL_CARD_LONG_PRESS_TIMER_ID &&
        g_full_card_press_window == hwnd &&
        g_full_card_press_timer_active != 0)
    {
      KillTimer(hwnd, FULL_CARD_LONG_PRESS_TIMER_ID);
      g_full_card_press_timer_active = 0;
      g_full_card_long_press_triggered = 1;
      SendMessageA(hwnd, WM_COMMAND, 1, 0);
      return 0;
    }
#endif
    if ((HWND)wparam == (HWND)1 && g_duel_interface_options.layout == 2)
    {
      GetCursorPos(&timer_cursor);
      if (WindowFromPoint(timer_cursor) != hwnd)
      {
        SendMessageA(hwnd, WM_LBUTTONDOWN, 1, 0);
      }
    }
    return 0;

  case WM_SETCURSOR:
    return handle_duel_inactive_cursor(hwnd, msg, wparam, lparam);

  case WM_WINDOWPOSCHANGING:
    DefWindowProcA(hwnd, msg, wparam, lparam);
    window_pos = (WINDOWPOS *)lparam;
    windowpos_cx = window_pos->cx;
    point.y = window_pos->cy;
    windowpos_scaled_height = (point.y * 200) / 300;
    windowpos_scaled_width = (windowpos_cx * 300) / 200;
    if (abs(windowpos_scaled_width - point.y) +
            abs(windowpos_scaled_height - windowpos_cx) <=
        4)
    {
      return 0;
    }
    if (windowpos_cx > windowpos_scaled_height)
    {
      window_pos->cx = windowpos_scaled_height;
    }
    else
    {
      window_pos->cy = windowpos_scaled_width;
    }
    return 0;

  case 0x30f:
  case 0x310:
  case 0x311:
    return handle_button_palette_message((int)hwnd, msg, (int)wparam, lparam);

  default:
    return DefWindowProcA(hwnd, msg, wparam, lparam);
  }
  return 0;
}
