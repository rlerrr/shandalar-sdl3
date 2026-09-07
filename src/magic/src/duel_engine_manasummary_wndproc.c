#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cardartlib/src/palette.h"
#include "drawcardlib/src/pic.h"
#include "magic/src/duel_engine.h"
#include "magic/src/game_support.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "magic/src/shared_startup.h"

#define CLASS_MAGICGAME_MANA_SUMMARY "MAGICGAME_ManaSummaryClass"

#ifndef SHANDALAR
extern HWND global_main_hwnd;
#define DUEL_MAIN_WINDOW_HWND global_main_hwnd
#else
#define DUEL_MAIN_WINDOW_HWND g_duel_window_hwnd
#endif

extern char global_base_directory[];
extern int g_duel_modal_action_active;
extern target_selection_request_t g_duel_action_request_copy;

typedef struct mana_summary_values_t
{
  int colorless;
  int black;
  int blue;
  int green;
  int red;
  int white;
  int artifact;
} mana_summary_values_t;

BOOL DrawBitmapToRect(HDC dst_dc, const RECT *dst_rect, HBITMAP bitmap);
void delete_and_close_object(HANDLE obj);
LRESULT handle_duel_inactive_cursor(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
int is_invalid_duel_player(int player);
int load_text_with_tab_escapes(char *filename, char *section_name);
void copy_opponent_name_prefix(char *name);
LOGFONTA *LoadFontFromIni(char *section, int must_load);
LRESULT CALLBACK wndproc_MAGICGAME_ManaSummaryClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
int get_displayed_mana_pool(mana_summary_values_t *mana_pool, int player);
void get_mana_pool_text_rect(LPRECT rect, HWND hwnd, int mana_color);

// GLOBAL: MAGIC 0x0055e1c4
// GLOBAL: SHANDALAR 0x0057f13c
int g_magicgame_mana_summary_window_extra_bytes = 4;

// GLOBAL: MAGIC 0x00708888
// GLOBAL: SHANDALAR 0x00652fc0
target_selection_result_t g_mana_summary_click_packet;

// GLOBAL: MAGIC 0x00708898
// GLOBAL: SHANDALAR 0x00652fd0
char g_mana_summary_menu_blue_text[0x48];

// GLOBAL: MAGIC 0x007088d0
// GLOBAL: SHANDALAR 0x00653008
target_selection_result_t g_mana_summary_menu_packet;

// GLOBAL: MAGIC 0x007088e0
// GLOBAL: SHANDALAR 0x00653018
char g_mana_summary_menu_colorless_text[0x38];

// GLOBAL: MAGIC 0x00708918
// GLOBAL: SHANDALAR 0x00653050
char g_mana_summary_menu_white_text[0x38];

// GLOBAL: MAGIC 0x00708950
// GLOBAL: SHANDALAR 0x00653088
char g_mana_summary_menu_red_text[0x38];

// GLOBAL: MAGIC 0x00708988
// GLOBAL: SHANDALAR 0x006530c0
char g_mana_summary_menu_help_text[0x1c];

// GLOBAL: MAGIC 0x007089a4
// GLOBAL: SHANDALAR 0x006530dc
HMENU g_mana_summary_popup_menu;

// GLOBAL: MAGIC 0x007089a8
// GLOBAL: SHANDALAR 0x006530e0
char g_mana_summary_menu_black_text[0x34];

// GLOBAL: MAGIC 0x007089dc
// GLOBAL: SHANDALAR 0x00653114
HFONT g_mana_summary_font;

// GLOBAL: MAGIC 0x007089e0
// GLOBAL: SHANDALAR 0x00653118
char g_mana_summary_menu_green_text[0x34];

// GLOBAL: MAGIC 0x00708a14
// GLOBAL: SHANDALAR 0x0065314c
HBITMAP g_mana_summary_background_bitmap;

// GLOBAL: MAGIC 0x00708a18
// GLOBAL: SHANDALAR 0x00653150
char g_mana_summary_menu_artifact_text[0x4c];

// FUNCTION: MAGIC 0x00539320
// FUNCTION: SHANDALAR 0x00550c00
int register_MAGICGAME_ManaSummaryClass(LPCSTR class_name)
{
  char path[264];
  int result;
  WNDCLASSA wndclass;

  result = 1;
  wndclass.style = 0xb;
  wndclass.lpfnWndProc = wndproc_MAGICGAME_ManaSummaryClass;
  wndclass.cbClsExtra = 0;
  wndclass.cbWndExtra = g_magicgame_mana_summary_window_extra_bytes;
  wndclass.hInstance = g_app_instance;
  wndclass.hIcon = (HICON)0;
  wndclass.hCursor = LoadCursorA((HINSTANCE)0, (LPCSTR)0x7f00);
  wndclass.hbrBackground = (HBRUSH)0x6;
  wndclass.lpszMenuName = (LPCSTR)0;
  wndclass.lpszClassName = class_name;
  if (RegisterClassA(&wndclass) == 0)
  {
    result = 0;
  }

  g_mana_summary_popup_menu = CreatePopupMenu();
  load_text_with_tab_escapes(global_ui_strings_filename, "MENU_MANAPOOL");
  strcpy(g_mana_summary_menu_black_text, g_text_lines[0]);
  strcpy(g_mana_summary_menu_blue_text, g_text_lines[1]);
  strcpy(g_mana_summary_menu_green_text, g_text_lines[2]);
  strcpy(g_mana_summary_menu_red_text, g_text_lines[3]);
  strcpy(g_mana_summary_menu_white_text, g_text_lines[4]);
  strcpy(g_mana_summary_menu_colorless_text, g_text_lines[5]);
  strcpy(g_mana_summary_menu_artifact_text, g_text_lines[6]);
  strcpy(g_mana_summary_menu_help_text, g_text_lines[7]);

  strcpy(path, global_duelart_path);
  strcat(path, "\\WINBK_ManaPool.pic");
  g_mana_summary_background_bitmap = load_pic(path);
  g_mana_summary_font = CreateFontIndirectA(LoadFontFromIni("ManaPool", 0));
  return result;
}

// FUNCTION: MAGIC 0x005394da
// FUNCTION: SHANDALAR 0x00550dba
void destroy_MAGICGAME_ManaSummaryClass(LPCSTR class_name)
{
  if (g_mana_summary_popup_menu != (HMENU)0)
  {
    DestroyMenu(g_mana_summary_popup_menu);
  }
  g_mana_summary_popup_menu = (HMENU)0;

  if (g_mana_summary_background_bitmap != (HBITMAP)0)
  {
    delete_and_close_object(g_mana_summary_background_bitmap);
  }
  g_mana_summary_background_bitmap = (HBITMAP)0;

  if (g_mana_summary_font != (HFONT)0)
  {
    DeleteObject(g_mana_summary_font);
  }
  g_mana_summary_font = (HFONT)0;
}

// FUNCTION: MAGIC 0x00539550
// FUNCTION: SHANDALAR 0x00550e30
LRESULT CALLBACK wndproc_MAGICGAME_ManaSummaryClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  int menu_item_count;
  POINT cursor_point;
  char menu_text[100];
  RECT menu_hit_rect;
  int menu_color;
  int menu_index;
  POINT menu_point;
  RECT popup_rect;
  mana_summary_values_t paint_mana_pool;
  char background_path[264];
  char amount_text[8];
  int text_x_offset; // ebp - 0x31c
  COLORREF mana_text_colors[7];
  HDC dc;
  PAINTSTRUCT paint;
  int saved_dc;
  int color_index;
  RECT text_rect;
  RECT client_rect;
  COLORREF shadow_color;
  int clicked_mana_pool[7];
  POINT click_point;
  MSG peek_msg;
  RECT click_rect;
  int peek_result;
  int unused_row_height; // ebp - 0x238
  int click_color;       // ebp - 0x234
  int unused_row_bottom; // ebp - 0x230
  RECT click_client_rect;
  unsigned int command_id;
  char help_path[264];
  DWORD help_context;
  mana_summary_values_t updated_mana_pool;
  char cuecard_text[100];
  int hit_color;
  POINT cuecard_point;
  int has_cuecard;
  unsigned int cuecard_player;
  RECT cuecard_rect;
  char opponent_name[100];
  mana_summary_values_t *cached_mana_pool;

  switch (msg)
  {
  case 0x437:
    cuecard_point.x = (short)((unsigned int)lparam & 0xffff);
    cuecard_point.y = (short)HIWORD(lparam);
    hit_color = -2;
    get_mana_pool_text_rect(&cuecard_rect, hwnd, 1);
    if (PtInRect(&cuecard_rect, cuecard_point) != 0)
    {
      hit_color = 1;
    }
    get_mana_pool_text_rect(&cuecard_rect, hwnd, 5);
    if (PtInRect(&cuecard_rect, cuecard_point) != 0)
    {
      hit_color = 5;
    }
    get_mana_pool_text_rect(&cuecard_rect, hwnd, 2);
    if (PtInRect(&cuecard_rect, cuecard_point) != 0)
    {
      hit_color = 2;
    }
    get_mana_pool_text_rect(&cuecard_rect, hwnd, 3);
    if (PtInRect(&cuecard_rect, cuecard_point) != 0)
    {
      hit_color = 3;
    }
    get_mana_pool_text_rect(&cuecard_rect, hwnd, 4);
    if (PtInRect(&cuecard_rect, cuecard_point) != 0)
    {
      hit_color = 4;
    }
    get_mana_pool_text_rect(&cuecard_rect, hwnd, 0);
    if (PtInRect(&cuecard_rect, cuecard_point) != 0)
    {
      hit_color = 0;
    }
    get_mana_pool_text_rect(&cuecard_rect, hwnd, 6);
    if (PtInRect(&cuecard_rect, cuecard_point) != 0)
    {
      hit_color = 6;
    }
    cuecard_player = g_duel_active_popup_window == hwnd ? 0 : 1;
    if (hit_color == 0 || hit_color == 1 || hit_color == 5 ||
        hit_color == 3 || hit_color == 4 || hit_color == 2 || hit_color == 6)
    {
      if (cuecard_player == 0)
      {
        strcpy(cuecard_text, gs_cuecard_your_mana_pool_008cc2e0[hit_color]);
      }
      else
      {
        copy_opponent_name_prefix(opponent_name);
        sprintf(cuecard_text, gs_cuecard_opponent_mana_pool_008cff20[hit_color], opponent_name);
      }
      has_cuecard = 1;
    }
    else
    {
      has_cuecard = 0;
    }
    if (has_cuecard != 0)
    {
      strcpy((char *)wparam, cuecard_text);
      return has_cuecard;
    }
    break;

  case 0x432:
    cached_mana_pool = (mana_summary_values_t *)GetWindowLongPtrA(hwnd, 0);
    get_displayed_mana_pool(&updated_mana_pool, (unsigned int)(g_duel_active_popup_window != hwnd));
    if (cached_mana_pool->colorless != updated_mana_pool.colorless ||
        cached_mana_pool->black != updated_mana_pool.black ||
        cached_mana_pool->blue != updated_mana_pool.blue ||
        cached_mana_pool->red != updated_mana_pool.red ||
        cached_mana_pool->green != updated_mana_pool.green ||
        cached_mana_pool->white != updated_mana_pool.white ||
        cached_mana_pool->artifact != updated_mana_pool.artifact)
    {
      InvalidateRect(hwnd, (RECT *)0, 0);
    }
    return 0;

  case WM_COMMAND:
    switch ((unsigned int)wparam & 0xffff)
    {
    case 100:
      help_context = 0x7ea;
      strcpy(help_path, global_base_directory);
      strcat(help_path, "\\duel.hlp");
      WinHelpA(g_duel_window_hwnd, help_path, HELP_CONTEXT, help_context);
      break;

    default:
      command_id = (unsigned int)wparam & 0xffff;
      if (100 < command_id)
      {
        g_recorded_action_controller = (int)(g_duel_active_popup_window != hwnd);
        g_recorded_action_phase = command_id - 0x65;
        g_recorded_action_player = 0;
        g_mana_summary_menu_packet.selection_code = -3;
        g_mana_summary_menu_packet.target_player = -1;
        g_mana_summary_menu_packet.target_card = -1;
        g_mana_summary_menu_packet.unused = 0;
        PostMessageA(DUEL_MAIN_WINDOW_HWND, 0x464, 0, (LPARAM)&g_mana_summary_menu_packet);
      }
      break;
    }
    return 0;

  case WM_CREATE:
    cached_mana_pool = malloc(sizeof(mana_summary_values_t));
    cached_mana_pool->artifact = 0;
    cached_mana_pool->white = cached_mana_pool->artifact;
    cached_mana_pool->green = cached_mana_pool->white;
    cached_mana_pool->red = cached_mana_pool->green;
    cached_mana_pool->blue = cached_mana_pool->red;
    cached_mana_pool->black = cached_mana_pool->blue;
    cached_mana_pool->colorless = cached_mana_pool->black;
    SetWindowLongPtrA(hwnd, 0, (LONG_PTR)cached_mana_pool);
    if (cached_mana_pool == (mana_summary_values_t *)0)
    {
      return -1;
    }
    return 0;

  case WM_DESTROY:
    cached_mana_pool = (mana_summary_values_t *)GetWindowLongPtrA(hwnd, 0);
    free(cached_mana_pool);
    return 0;

  case WM_ERASEBKGND:
    return 1;

  case WM_LBUTTONDOWN:
    if (g_duel_modal_action_active != 0)
    {
      Sleep(GetDoubleClickTime());
      peek_result = PeekMessageA(&peek_msg, hwnd, WM_LBUTTONDBLCLK, WM_LBUTTONDBLCLK, 0);
      get_displayed_mana_pool((mana_summary_values_t *)clicked_mana_pool, (unsigned int)(g_duel_active_popup_window != hwnd));
      GetClientRect(hwnd, &click_client_rect);
      click_point.x = (short)((unsigned int)lparam & 0xffff);
      click_point.y = (short)HIWORD(lparam);
      unused_row_height = unused_row_bottom = click_client_rect.bottom / 6;
      g_recorded_action_controller = g_duel_active_popup_window == hwnd ? 0 : 1;
      g_recorded_action_phase = -1;
      for (click_color = 0; click_color < 7; click_color++)
      {
        get_mana_pool_text_rect(&click_rect, hwnd, click_color);
        if (PtInRect(&click_rect, click_point) != 0 &&
            0 < clicked_mana_pool[click_color])
        {
          g_recorded_action_phase = click_color;
        }
      }
      if (g_recorded_action_phase != -1 && g_duel_modal_action_active != 0 &&
          (g_duel_action_request_copy.player == -1 || g_duel_action_request_copy.player == g_recorded_action_controller) &&
          (g_duel_action_request_copy.zone_flags == -1 || (g_duel_action_request_copy.zone_flags & 1) != 0))
      {
        g_recorded_action_player = peek_result;
        g_mana_summary_click_packet.selection_code = -3;
        g_mana_summary_click_packet.target_player = -1;
        g_mana_summary_click_packet.target_card = -1;
        g_mana_summary_click_packet.unused = 0;
        PostMessageA(g_duel_window_hwnd, 0x464, 0, (LPARAM)&g_mana_summary_click_packet);
      }
    }
    return 0;

  case WM_PAINT:
    cached_mana_pool = (mana_summary_values_t *)GetWindowLongPtrA(hwnd, 0);
    get_displayed_mana_pool(&paint_mana_pool, (unsigned int)(g_duel_active_popup_window != hwnd));
    if (cached_mana_pool->colorless != paint_mana_pool.colorless ||
        cached_mana_pool->black != paint_mana_pool.black ||
        cached_mana_pool->blue != paint_mana_pool.blue ||
        cached_mana_pool->red != paint_mana_pool.red ||
        cached_mana_pool->green != paint_mana_pool.green ||
        cached_mana_pool->white != paint_mana_pool.white ||
        cached_mana_pool->artifact != paint_mana_pool.artifact)
    {
      InvalidateRect(hwnd, (RECT *)0, 0);
    }

    EnterCriticalSection(&g_card_render_lock);
    GetClientRect(hwnd, &client_rect);
    dc = g_shared_offscreen_dc;
    saved_dc = SaveDC(g_shared_offscreen_dc);
    if (g_mana_summary_background_bitmap == (HBITMAP)0)
    {
      strcpy(background_path, global_duelart_path);
      strcat(background_path, "\\WINBK_ManaPool.pic");
      g_mana_summary_background_bitmap = load_pic(background_path);
    }
    if (g_mana_summary_background_bitmap != (HBITMAP)0)
    {
      DrawBitmapToRect(dc, &client_rect, g_mana_summary_background_bitmap);
    }
    else
    {
      FillRect(dc, &client_rect, GetStockObject(LTGRAY_BRUSH));
    }
    SelectObject(dc, g_mana_summary_font);
    SetBkMode(dc, TRANSPARENT);
    SetTextAlign(dc, TA_CENTER);
    text_x_offset = (client_rect.right * 0x28) / 100;
    SetMapMode(dc, MM_ANISOTROPIC);
    get_mana_pool_text_rect(&text_rect, hwnd, 1);
    SetWindowExtEx(dc, text_rect.right - text_rect.left, 0x28, (LPSIZE)0);
    SetViewportExtEx(dc, text_rect.right - text_rect.left,
                     text_rect.bottom - text_rect.top, (LPSIZE)0);
    shadow_color = 0x10000c9;
    mana_text_colors[1] = 0x10000c8;
    mana_text_colors[2] = 0x100005d;
    mana_text_colors[3] = 0x1000026;
    mana_text_colors[4] = 0x100001e;
    mana_text_colors[5] = 0x10000bf;
    mana_text_colors[0] = 0x10000c5;
    mana_text_colors[6] = 0x10000d0;

    for (color_index = 0; color_index < 7; color_index++)
    {
      wsprintfA(amount_text, "%d", ((int *)&paint_mana_pool)[color_index]);
      get_mana_pool_text_rect(&text_rect, hwnd, color_index);
      DPtoLP(dc, (LPPOINT)&text_rect, 2);
      if (color_index != 6)
      {
        text_rect.left += text_x_offset;
      }
      if (color_index == 6 && IsRectEmpty(&text_rect))
      {
      }
      else
      {
        SetTextColor(dc, shadow_color);
        TextOutA(dc, text_rect.left + 1, text_rect.top + 1, amount_text, lstrlenA(amount_text));
        SetTextColor(dc, mana_text_colors[color_index]);
        TextOutA(dc, text_rect.left, text_rect.top, amount_text, lstrlenA(amount_text));
      }
    }
    RestoreDC(g_shared_offscreen_dc, saved_dc);
    dc = BeginPaint(hwnd, &paint);
    if (dc != (HDC)0)
    {
      ApplyCardArtPaletteToDc(dc);
      GetClientRect(hwnd, &client_rect);
      if (g_duel_palette_refresh_pending != 0)
      {
        FillRect(dc, &client_rect, GetStockObject(WHITE_BRUSH));
        Sleep(200);
      }
      BitBlt(dc, 0, 0, client_rect.right, client_rect.bottom,
             g_shared_offscreen_dc, 0, 0, SRCCOPY);
      EndPaint(hwnd, &paint);
      cached_mana_pool->colorless = paint_mana_pool.colorless;
      cached_mana_pool->black = paint_mana_pool.black;
      cached_mana_pool->blue = paint_mana_pool.blue;
      cached_mana_pool->red = paint_mana_pool.red;
      cached_mana_pool->green = paint_mana_pool.green;
      cached_mana_pool->white = paint_mana_pool.white;
      cached_mana_pool->artifact = paint_mana_pool.artifact;
    }
    LeaveCriticalSection(&g_card_render_lock);
    return 0;

  case WM_RBUTTONDOWN:
    menu_point.x = lparam & 0xffff;
    menu_point.y = HIWORD(lparam);
    ClientToScreen(hwnd, &menu_point);
    SetRect(&popup_rect, menu_point.x, menu_point.y, menu_point.x + 1, menu_point.y + 1);
    TrackPopupMenu(g_mana_summary_popup_menu, TPM_RIGHTBUTTON, menu_point.x, menu_point.y, 0, hwnd, &popup_rect);
    return 0;

  case WM_SETCURSOR:
    return handle_duel_inactive_cursor(hwnd, WM_SETCURSOR, wparam, lparam);

  case WM_INITMENU:
    has_cuecard = 0;
    for (menu_index = 0; menu_index < 7; menu_index++)
    {
      if (g_mana_charge[menu_index] != 0)
      {
        has_cuecard = 1;
      }
    }
    if (g_duel_modal_action_active != 0 && has_cuecard &&
        (g_duel_action_request_copy.player == -1 || g_duel_action_request_copy.player == g_recorded_action_controller) &&
        (g_duel_action_request_copy.type_flags == -1 ||
         g_duel_action_request_copy.type_flags == 0 ||
         g_duel_action_request_copy.type_flags == 1) &&
        (g_duel_action_request_copy.color_flags == -1 ||
         g_duel_action_request_copy.color_flags == 0 ||
         g_duel_action_request_copy.color_flags == g_recorded_action_phase) &&
        (g_duel_action_request_copy.owner == -1 || g_duel_action_request_copy.owner == g_recorded_action_controller) &&
        (g_duel_action_request_copy.zone_flags == -1 || (g_duel_action_request_copy.zone_flags & 1) != 0))
    {
      GetCursorPos(&cursor_point);
      MapWindowPoints((HWND)0, hwnd, &cursor_point, 1);
      menu_color = -1;
      get_mana_pool_text_rect(&menu_hit_rect, hwnd, 1);
      if (PtInRect(&menu_hit_rect, cursor_point) != 0)
      {
        menu_color = 1;
        strcpy(menu_text, g_mana_summary_menu_black_text);
      }
      get_mana_pool_text_rect(&menu_hit_rect, hwnd, 5);
      if (PtInRect(&menu_hit_rect, cursor_point) != 0)
      {
        menu_color = 5;
        strcpy(menu_text, g_mana_summary_menu_white_text);
      }
      get_mana_pool_text_rect(&menu_hit_rect, hwnd, 2);
      if (PtInRect(&menu_hit_rect, cursor_point) != 0)
      {
        menu_color = 2;
        strcpy(menu_text, g_mana_summary_menu_blue_text);
      }
      get_mana_pool_text_rect(&menu_hit_rect, hwnd, 3);
      if (PtInRect(&menu_hit_rect, cursor_point) != 0)
      {
        menu_color = 3;
        strcpy(menu_text, g_mana_summary_menu_green_text);
      }
      get_mana_pool_text_rect(&menu_hit_rect, hwnd, 4);
      if (PtInRect(&menu_hit_rect, cursor_point) != 0)
      {
        menu_color = 4;
        strcpy(menu_text, g_mana_summary_menu_red_text);
      }
      get_mana_pool_text_rect(&menu_hit_rect, hwnd, 0);
      if (PtInRect(&menu_hit_rect, cursor_point) != 0)
      {
        menu_color = 0;
        strcpy(menu_text, g_mana_summary_menu_colorless_text);
      }
      get_mana_pool_text_rect(&menu_hit_rect, hwnd, 6);
      if (PtInRect(&menu_hit_rect, cursor_point) != 0)
      {
        menu_color = 0;
        strcpy(menu_text, g_mana_summary_menu_artifact_text);
      }
      if (menu_color != -1)
      {
        AppendMenuA(g_mana_summary_popup_menu, 0, menu_color + 0x65, menu_text);
      }
    }
    if (0 < GetMenuItemCount(g_mana_summary_popup_menu))
    {
      AppendMenuA(g_mana_summary_popup_menu, MF_SEPARATOR, 0, (LPCSTR)0);
    }
    AppendMenuA(g_mana_summary_popup_menu, 0, 100, g_mana_summary_menu_help_text);
    return 0;

  case WM_MENUSELECT:
    if (((unsigned int)wparam >> 0x10) == 0xffff && lparam == 0)
    {
      menu_item_count = GetMenuItemCount(g_mana_summary_popup_menu);
      while (menu_item_count-- != 0)
      {
        DeleteMenu(g_mana_summary_popup_menu, 0, MF_BYPOSITION);
      }
    }
    return 0;

  case 0x30f:
  case 0x310:
  case 0x311:
    return handle_button_palette_message(hwnd, msg, (HWND)wparam, lparam);

  default:
    return DefWindowProcA(hwnd, msg, wparam, lparam);
  }

  return 0;
}

// FUNCTION: MAGIC 0x00449d7e
// FUNCTION: SHANDALAR 0x004538ff
int get_displayed_mana_pool(mana_summary_values_t *mana_pool, int player)
{
  if (mana_pool == (mana_summary_values_t *)0)
  {
    return 0;
  }

  if (is_invalid_duel_player(player))
  {
    return 0;
  }

  EnterCriticalSection(&g_duel_render_lock);
  if (player == 0)
  {
    memcpy(mana_pool, g_duel_cached_raw_mana_player_0, sizeof(mana_summary_values_t));
  }
  else
  {
    memcpy(mana_pool, g_duel_cached_raw_mana_player_1, sizeof(mana_summary_values_t));
  }
  LeaveCriticalSection(&g_duel_render_lock);
  return 1;
}

// FUNCTION: MAGIC 0x0053a876
// FUNCTION: SHANDALAR 0x00552144
void get_mana_pool_text_rect(LPRECT rect, HWND hwnd, int mana_color)
{
  mana_summary_values_t mana_pool;
  int row_index;
  int row_height;
  int row_spacing;
  int top_margin;
  int bottom;
  RECT client_rect;
  int top;

  get_displayed_mana_pool(&mana_pool, (unsigned int)(hwnd != g_duel_active_popup_window));
  GetClientRect(hwnd, &client_rect);
  top_margin = (client_rect.bottom * 5) / 100;
  row_height = (client_rect.bottom * 0x91) / 1000;
  row_spacing = (client_rect.bottom * 0x14) / 1000;
  if (mana_color == 1)
  {
    row_index = 0;
  }
  else if (mana_color == 2)
  {
    row_index = 1;
  }
  else if (mana_color == 3)
  {
    row_index = 2;
  }
  else if (mana_color == 4)
  {
    row_index = 3;
  }
  else if (mana_color == 5)
  {
    row_index = 4;
  }
  else if (mana_color == 0)
  {
    row_index = 5;
  }
  else if (mana_color == 6)
  {
    row_index = 5;
  }
  else
  {
    row_index = -1;
  }
  if (row_index == -1)
  {
    SetRect(rect, 0, 0, 0, 0);
  }
  else
  {
    top = (row_spacing + row_height) * row_index + top_margin;
    bottom = top + row_height;
    SetRect(rect, client_rect.left, top, client_rect.right, bottom);
    if (mana_pool.artifact != 0)
    {
      if (mana_color == 0)
      {
        rect->right -= (client_rect.right - client_rect.left) / 2;
      }
      else if (mana_color == 6)
      {
        rect->left += ((client_rect.right - client_rect.left) * 2) / 3;
      }
    }
    else if (mana_color == 6)
    {
      SetRect(rect, 0, 0, 0, 0);
    }
  }
}
