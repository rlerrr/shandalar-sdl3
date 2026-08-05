#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "cardartlib/src/palette.h"
#include "deckdll/src/shared_resources.h"
#include "drawcardlib/Drawcardlib.h"
#include "magic/src/duel_engine.h"
#include "magic/src/game_support.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "magic/src/shared_startup.h"

#define DUEL_MAIN_WINDOW_HWND g_duel_window_hwnd

#define CLASS_SHUFFLE_CARD "ShuffleCard"

extern int g_duel_cached_library_player_0[500];
extern int g_duel_cached_library_player_1[500];
extern int g_duel_cached_library_count_player_0;
extern int g_duel_cached_library_count_player_1;
extern char global_base_directory[];
extern int is_invalid_duel_player(int player);
extern LRESULT handle_duel_inactive_cursor(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
int load_text_with_tab_escapes(char *filename, char *section_name);

// GLOBAL: MAGIC 0x0055e0b8
// GLOBAL: SHANDALAR 0x0057f098
int g_library_window_long_offset = 0;

// GLOBAL: MAGIC 0x0055e0bc
// GLOBAL: SHANDALAR 0x0057f09c
int g_library_window_extra_bytes = 4;

// GLOBAL: MAGIC 0x00638ac8
// GLOBAL: SHANDALAR 0x005aa630
char g_library_menu_help_text[0x1c];

// GLOBAL: MAGIC 0x00638ae4
// GLOBAL: SHANDALAR 0x005aa64c
HMENU g_library_popup_menu;

// GLOBAL: MAGIC 0x00638ae8
// GLOBAL: SHANDALAR 0x005aa650
HMENU g_library_submenu;

// GLOBAL: MAGIC 0x00638af0
// GLOBAL: SHANDALAR 0x005aa658
char g_library_menu_count_text[0x20];

// FUNCTION: MAGIC 0x00449feb
// FUNCTION: SHANDALAR 0x00453b6c
int copy_cached_library_cards_and_get_count(void *cards, int player)
{
  int result;

  if (cards == NULL)
  {
    return 0;
  }

  if (is_invalid_duel_player(player) != 0)
  {
    return 0;
  }

  EnterCriticalSection(&g_duel_render_lock);
  if (player == 0)
  {
    result = g_duel_cached_library_count_player_0;
  }
  else
  {
    result = g_duel_cached_library_count_player_1;
  }
  memcpy(cards, (player == 0) ? g_duel_cached_library_player_0 : g_duel_cached_library_player_1, 2000);
  LeaveCriticalSection(&g_duel_render_lock);

  return result;
}

// FUNCTION: MAGIC 0x00498ca9
// FUNCTION: SHANDALAR 0x004c2501
int get_cached_library_count(int player)
{
  char cards[2000];

  return copy_cached_library_cards_and_get_count(cards, player);
}

// FUNCTION: MAGIC 0x00497fd0
// FUNCTION: SHANDALAR 0x004c1830
int register_MAGICGAME_LibraryClass(LPCSTR class_name)
{
  int result;
  WNDCLASSA wndclass;

  result = 1;
  wndclass.style = 3;
  wndclass.lpfnWndProc = wndproc_MAGICGAME_LibraryClass;
  wndclass.cbClsExtra = 0;
  wndclass.cbWndExtra = g_library_window_extra_bytes;
  wndclass.hInstance = g_app_instance;
  wndclass.hIcon = LoadIconA((HINSTANCE)0, (LPCSTR)0x7f00);
  wndclass.hCursor = LoadCursorA((HINSTANCE)0, (LPCSTR)0x7f00);
  wndclass.hbrBackground = (HBRUSH)GetStockObject(4);
  wndclass.lpszMenuName = (LPCSTR)0;
  wndclass.lpszClassName = class_name;
  if (RegisterClassA(&wndclass) == 0)
    result = 0;

  wndclass.style = 0;
  wndclass.lpfnWndProc = wndproc_ShuffleCard;
  wndclass.cbClsExtra = 0;
  wndclass.cbWndExtra = 0;
  wndclass.hInstance = g_app_instance;
  wndclass.hIcon = LoadIconA((HINSTANCE)0, (LPCSTR)0x7f00);
  wndclass.hCursor = LoadCursorA((HINSTANCE)0, (LPCSTR)0x7f00);
  wndclass.hbrBackground = (HBRUSH)0;
  wndclass.lpszMenuName = (LPCSTR)0;
  wndclass.lpszClassName = CLASS_SHUFFLE_CARD;
  if (RegisterClassA(&wndclass) == 0)
    result = 0;

  g_library_popup_menu = CreatePopupMenu();
  g_library_submenu = CreatePopupMenu();
  AppendMenuA(g_library_submenu, 0, 0x65, "");
  load_text_with_tab_escapes(global_ui_strings_filename, "MENU_LIBRARY");
  strcpy(g_library_menu_count_text, g_text_lines[0]);
  strcpy(g_library_menu_help_text, g_text_lines[1]);
  return result;
}

// FUNCTION: MAGIC 0x00498193
// FUNCTION: SHANDALAR 0x004c19f3
LRESULT CALLBACK wndproc_MAGICGAME_LibraryClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  int unused;
  int menu_item_count;
  int saved_dc;
  int count_text_length;
  char menu_count_text[12];
  POINT menu_point;
  RECT menu_rect;
  char count_overlay_text[12];
  RECT stack_card_rect;
  int card_y_step;       // ebp - 0x41c
  int card_x_step;       // ebp - 0x418
  int card_width_delta;  // ebp - 0x414
  int stack_card_bottom; // ebp - 0x410
  RECT top_card_rect;
  int stack_card_right;
  int card_height_delta;
  int library_count;
  HDC paint_dc;
  PAINTSTRUCT paint_struct;
  RECT client_rect;
  char help_path[264];
  DWORD help_context;
  int shuffle_card_width_delta;
  int shuffle_card_height_delta;
  int shuffle_count;
  int shuffle_card_height;
  int shuffle_card_width;
  int shuffle_index;
  int shuffle_y;
  int shuffle_x;
  RECT shuffle_rect;
  HWND shuffle_windows[100];
  int cached_count;
  char cuecard_text[100];
  char opponent_name[100];
  int previous_count;

  switch (msg)
  {
  case 0x437:
    if (g_duel_main_window_hwnd == hwnd)
    {
      strcpy(cuecard_text, gs_cuecard_your_library_0091ca20);
    }
    else
    {
      copy_opponent_name_prefix(opponent_name);
      sprintf(cuecard_text, gs_cuecard_opponent_library_00789bd0, opponent_name);
    }
    strcpy((char *)wparam, cuecard_text);
    return 1;

  case 0x432:
    previous_count = GetWindowLongA(hwnd, g_library_window_long_offset);
    cached_count = get_cached_library_count(1 <= (unsigned int)((int)hwnd - (int)g_duel_main_window_hwnd));
    if (previous_count != cached_count)
    {
      InvalidateRect(hwnd, NULL, TRUE);
    }
    return 0;

  case 0x400:
    shuffle_count = get_cached_library_count(1 <= (unsigned int)((int)hwnd - (int)g_duel_main_window_hwnd));
    GetClientRect(hwnd, &shuffle_rect);
    shuffle_card_width_delta = (((shuffle_rect.right * 0x28) / 100) *
                                (shuffle_count < 0x4b ? shuffle_count : 0x4b)) /
                               0x4b;
    shuffle_card_height_delta = (shuffle_card_width_delta * shuffle_rect.bottom) / shuffle_rect.right;
    shuffle_card_width = shuffle_rect.right - shuffle_card_width_delta;
    shuffle_card_height = shuffle_rect.bottom - shuffle_card_height_delta;
    GetWindowRect(hwnd, &shuffle_rect);
    shuffle_x = shuffle_rect.left;
    shuffle_y = shuffle_rect.top;
    if (shuffle_count > 100)
    {
      shuffle_count = 100;
    }
    if (GetSystemMetrics(SM_SLOWMACHINE) != 0)
    {
      if (shuffle_count > 4)
      {
        shuffle_count = MAX(shuffle_count / 5, 4);
      }
    }
    else if (shuffle_count > 5)
    {
      shuffle_count = MAX(shuffle_count / 3, 5);
    }

    for (shuffle_index = 0; shuffle_count > shuffle_index; shuffle_index++)
    {
      shuffle_windows[shuffle_index] =
          CreateWindowExA(0, CLASS_SHUFFLE_CARD, "", 0x90000000, shuffle_x, shuffle_y,
                          shuffle_card_width, shuffle_card_height, DUEL_MAIN_WINDOW_HWND,
                          (HMENU)0, g_app_instance, (LPVOID)0);
      if (shuffle_windows[shuffle_index] != (HWND)0)
      {
        UpdateWindow(shuffle_windows[shuffle_index]);
#ifdef SHANDALAR_SDL_WIN32_COMPAT
        shandalar_win32_pump(TRUE);
#endif
        shuffle_x += (shuffle_card_width * 0xf) / 100;
      }
    }
    for (shuffle_index = shuffle_count - 1; shuffle_index >= 0; shuffle_index--)
    {
      if (shuffle_windows[shuffle_index] != (HWND)0)
      {
        DestroyWindow(shuffle_windows[shuffle_index]);
      }
      UpdateWindow(DUEL_MAIN_WINDOW_HWND);
#ifdef SHANDALAR_SDL_WIN32_COMPAT
      shandalar_win32_pump(TRUE);
#endif
    }
    return 0;

  case WM_COMMAND:
    switch (wparam & 0xffff)
    {
    case 100:
      help_context = 0x7e9;
      strcpy(help_path, global_base_directory);
      strcat(help_path, "\\duel.hlp");
      WinHelpA(DUEL_MAIN_WINDOW_HWND, help_path, HELP_CONTEXT, help_context);
      break;

    default:
      break;
    }
    return 0;

  case WM_CREATE:
    previous_count = 0;
    SetWindowLongA(hwnd, g_library_window_long_offset, previous_count);
    return 0;

  case WM_ERASEBKGND:
    return 1;

  case WM_LBUTTONDOWN:
    return 0;

  case WM_PAINT:
    previous_count = GetWindowLongA(hwnd, g_library_window_long_offset);
    library_count = get_cached_library_count(1 <= (unsigned int)((int)hwnd - (int)g_duel_main_window_hwnd));
    if (previous_count != library_count)
    {
      InvalidateRect(hwnd, NULL, FALSE);
    }
    EnterCriticalSection(&g_card_render_lock);
    paint_dc = BeginPaint(hwnd, &paint_struct);
    if (paint_dc != (HDC)0)
    {
      ApplyCardArtPaletteToDc(paint_dc);
      GetClientRect(hwnd, &client_rect);
      if (g_duel_palette_refresh_pending != 0)
      {
        FillRect(paint_dc, &client_rect, GetStockObject(WHITE_BRUSH));
        Sleep(200);
      }
      if (library_count == 0)
      {
        FillRect(paint_dc, &client_rect, GetStockObject(BLACK_BRUSH));
      }
      else if (library_count == 1)
      {
        DrawCardBack(paint_dc, &client_rect);
      }
      else
      {
        FillRect(g_shared_offscreen_dc, &client_rect, GetStockObject(BLACK_BRUSH));
        card_width_delta = (((client_rect.right * 0x28) / 100) *
                            (library_count < 0x4b ? library_count : 0x4b)) /
                           0x4b;
        card_height_delta = (card_width_delta * client_rect.bottom) / client_rect.right;
        SetRect(&top_card_rect, client_rect.left, client_rect.top,
                client_rect.right - card_width_delta, client_rect.bottom - card_height_delta);
        SelectObject(paint_dc, GetStockObject(GRAY_BRUSH));
        SelectObject(paint_dc, GetStockObject(BLACK_PEN));

        card_x_step = MAX(card_width_delta / MAX(library_count / 5, 1), 2);

        card_y_step = MAX(card_height_delta / MAX(library_count / 5, 1), 3);

        stack_card_right = client_rect.right;
        stack_card_bottom = client_rect.bottom;
        for (; stack_card_right > top_card_rect.right;
             stack_card_right -= card_x_step, stack_card_bottom -= card_y_step)
        {
          SetRect(&stack_card_rect, stack_card_right - (top_card_rect.right - top_card_rect.left),
                  stack_card_bottom - (top_card_rect.bottom - top_card_rect.top),
                  stack_card_right, stack_card_bottom);
          DrawCardBack(g_shared_offscreen_dc, &stack_card_rect);
        }
        DrawCardBack(g_shared_offscreen_dc, &top_card_rect);
        BitBlt(paint_dc, 0, 0, client_rect.right, client_rect.bottom,
               g_shared_offscreen_dc, 0, 0, SRCCOPY);
      }
      saved_dc = SaveDC(paint_dc);
      SelectObject(paint_dc, global_idtag_font);
      sprintf(count_overlay_text, "%d", library_count);
      count_text_length = strlen(count_overlay_text);
      SetBkMode(paint_dc, TRANSPARENT);
      SetTextColor(paint_dc, 0x000000);
      TextOutA(paint_dc, 6, 1, count_overlay_text, count_text_length);
      SetTextColor(paint_dc, 0xffffff);
      TextOutA(paint_dc, 5, 0, count_overlay_text, count_text_length);
      RestoreDC(paint_dc, saved_dc);
      EndPaint(hwnd, &paint_struct);
      previous_count = library_count;
      SetWindowLongA(hwnd, g_library_window_long_offset, previous_count);
    }
    LeaveCriticalSection(&g_card_render_lock);
    return 0;

  case WM_RBUTTONDOWN:
    menu_point.x = lparam & 0xffff;
    menu_point.y = HIWORD(lparam);
    ClientToScreen(hwnd, &menu_point);
    SetRect(&menu_rect, menu_point.x, menu_point.y, menu_point.x + 1, menu_point.y + 1);
    TrackPopupMenu(g_library_popup_menu, TPM_RIGHTBUTTON, menu_point.x, menu_point.y, 0, hwnd, &menu_rect);
    return 0;

  case WM_INITMENU:
    previous_count = GetWindowLongA(hwnd, g_library_window_long_offset);
    sprintf(menu_count_text, "%d", previous_count);
    AppendMenuA(g_library_popup_menu, MF_POPUP, (UINT)g_library_submenu, g_library_menu_count_text);
    ModifyMenuA(g_library_submenu, 0x65, MF_BYCOMMAND, 0x65, menu_count_text);
    AppendMenuA(g_library_popup_menu, MF_STRING, 100, g_library_menu_help_text);
    return 0;

  case WM_MENUSELECT:
    if (HIWORD(wparam) == 0xffff && lparam == 0)
    {
      menu_item_count = GetMenuItemCount(g_library_popup_menu);
      while (unused = menu_item_count--)
      {
        RemoveMenu(g_library_popup_menu, 0, MF_BYPOSITION);
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
