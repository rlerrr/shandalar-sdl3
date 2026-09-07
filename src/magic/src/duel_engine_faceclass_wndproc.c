#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cardartlib/src/assert.h"
#include "cardartlib/src/palette.h"
#include "deckdll/src/shared_resources.h"
#include "drawcardlib/src/pic.h"
#include "magic/src/duel_engine.h"
#include "magic/src/game_support.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "magic/src/shared_startup.h"

BOOL DrawMaskedBitmapToRect(HDC dc, RECT *rect, HANDLE bitmap, int width, int height, int src_x, int src_y, int mask_x, int mask_y);
extern int g_duel_modal_action_active;
void draw_duel_face_window(HDC dc, RECT *rect, int player);
void set_player_directive_value(int player, int value);
void post_face_directive_action(int player);
int is_face_directive_action_enabled(int player);
int draw_masked_bitmap_left_half_to_rect(HDC dc, RECT *rect, HANDLE bitmap);
LRESULT handle_duel_inactive_cursor(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
int load_text_with_tab_escapes(char *filename, char *section_name);

// GLOBAL: MAGIC 0x0055e02c
// GLOBAL: SHANDALAR 0x0057f004
int g_face_picture_window_long_offset = 0;

// GLOBAL: MAGIC 0x0055e030
// GLOBAL: SHANDALAR 0x0057f008
int g_face_picture_ownership_window_long_offset = 4;

// GLOBAL: MAGIC 0x0055e034
// GLOBAL: SHANDALAR 0x0057f00c
int g_magicgame_face_window_extra_bytes = 8;

// GLOBAL: MAGIC 0x00637b48
// GLOBAL: SHANDALAR 0x005a8160
COLORREF g_face_name_text_color;

// GLOBAL: MAGIC 0x00637b4c
int g_face_ui_player_life;

// GLOBAL: MAGIC 0x00637b50
// GLOBAL: SHANDALAR 0x005a8168
char g_face_menu_end_directive_text[0x34];

// GLOBAL: MAGIC 0x00637b84
// GLOBAL: SHANDALAR 0x005a819c
COLORREF g_face_name_shadow_color;

// GLOBAL: MAGIC 0x00637b88
// GLOBAL: SHANDALAR 0x005a81a0
target_selection_result_t g_face_directive_packet;

// GLOBAL: MAGIC 0x00637b94
int g_face_ui_opponent_life;

// GLOBAL: MAGIC 0x00637b98
// GLOBAL: SHANDALAR 0x005a81b0
char g_face_menu_help_text[0x1c];

// GLOBAL: MAGIC 0x00637bb4
// GLOBAL: SHANDALAR 0x005a81cc
HMENU g_face_popup_menu;

// GLOBAL: MAGIC 0x00637bb8
// GLOBAL: SHANDALAR 0x005a81d0
char g_face_menu_directive_format[100];

// GLOBAL: MAGIC 0x00637c1c
// GLOBAL: SHANDALAR 0x005a8234
HFONT g_face_name_font;

// GLOBAL: MAGIC 0x00637c20
// GLOBAL: SHANDALAR 0x005a8238
char g_face_menu_directive_self_text[0x38];

// GLOBAL: MAGIC 0x00637c58
// GLOBAL: SHANDALAR 0x005a8270
HBITMAP g_face_background_pics[6];

// GLOBAL: MAGIC 0x00637c70
// GLOBAL: SHANDALAR 0x00604420
int g_face_ui_refresh_flags;

// GLOBAL: MAGIC 0x00925bb0
int g_opponent_face_directive_action_enabled;

// GLOBAL: MAGIC 0x00925bb4
int g_player_face_directive_action_enabled;

// FUNCTION: MAGIC 0x004637d0
// FUNCTION: SHANDALAR 0x00425f10
int register_MAGICGAME_FaceClass(LPCSTR class_name)
{
  char path[264];
  int result;
  WNDCLASSA wndclass;

  result = 1;
  wndclass.style = 0xb;
  wndclass.lpfnWndProc = wndproc_MAGICGAME_FaceClass;
  wndclass.cbClsExtra = 0;
  wndclass.cbWndExtra = g_magicgame_face_window_extra_bytes;
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

  g_face_popup_menu = CreatePopupMenu();
  load_text_with_tab_escapes(global_ui_strings_filename, "MENU_FACE");
  strcpy(g_face_menu_directive_format, g_text_lines[0]);
  strcpy(g_face_menu_directive_self_text, g_text_lines[1]);
  strcpy(g_face_menu_end_directive_text, g_text_lines[2]);
  strcpy(g_face_menu_help_text, g_text_lines[3]);

  strcpy(path, global_duelart_path);
  strcat(path, "\\FACE_MULTI.pic");
  g_face_background_pics[0] = load_pic(path);

  strcpy(path, global_duelart_path);
  strcat(path, "\\FACE_BLACK.pic");
  g_face_background_pics[1] = load_pic(path);

  strcpy(path, global_duelart_path);
  strcat(path, "\\FACE_BLUE.pic");
  g_face_background_pics[2] = load_pic(path);

  strcpy(path, global_duelart_path);
  strcat(path, "\\FACE_GREEN.pic");
  g_face_background_pics[3] = load_pic(path);

  strcpy(path, global_duelart_path);
  strcat(path, "\\FACE_RED.pic");
  g_face_background_pics[4] = load_pic(path);

  strcpy(path, global_duelart_path);
  strcat(path, "\\FACE_WHITE.pic");
  g_face_background_pics[5] = load_pic(path);

  g_face_name_font = CreateFontIndirectA(LoadFontFromIni("Face", 0));
  g_face_name_text_color = 0x2f6f7f7;
  g_face_name_shadow_color = 0x2565656;
  return result;
}

// FUNCTION: MAGIC 0x00463a6e
// FUNCTION: SHANDALAR 0x004261ae
void destroy_MAGICGAME_FaceClass(LPCSTR class_name)
{
  int i;

  if (g_face_popup_menu != (HMENU)0)
  {
    DestroyMenu(g_face_popup_menu);
  }
  g_face_popup_menu = (HMENU)0;

  for (i = 0; i < 6; i++)
  {
    if (g_face_background_pics[i] != (HBITMAP)0)
    {
      delete_and_close_object(g_face_background_pics[i]);
      g_face_background_pics[i] = (HBITMAP)0;
    }
  }

  if (g_face_name_font != (HFONT)0)
  {
    DeleteObject(g_face_name_font);
  }
  g_face_name_font = (HFONT)0;
}

// FUNCTION: MAGIC 0x00463b12
// FUNCTION: SHANDALAR 0x00426252
LRESULT CALLBACK wndproc_MAGICGAME_FaceClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  int menu_item_count;
  char directive_text[100];
  int menu_player;
  char opponent_name[100];
  POINT menu_point;
  RECT popup_rect;
  HDC paint_dc;
  PAINTSTRUCT paint;
  RECT client_rect;
  MSG peek_msg;
  int click_player;
  char help_path[264];
  DWORD help_context;
  int command_player;
  HBITMAP face_bitmap;
  int owns_face_bitmap;

  switch (msg)
  {

  case 0x439:
    face_bitmap = (HBITMAP)GetWindowLongPtrA(hwnd, g_face_picture_window_long_offset);
    owns_face_bitmap = GetWindowLongA(hwnd, g_face_picture_ownership_window_long_offset);
    if (face_bitmap != (HBITMAP)0 && owns_face_bitmap == 0)
    {
      delete_and_close_object(face_bitmap);
    }

    face_bitmap = (HBITMAP)wparam;
    owns_face_bitmap = lparam;
    SetWindowLongPtrA(hwnd, g_face_picture_window_long_offset, (LONG_PTR)face_bitmap);
    SetWindowLongA(hwnd, g_face_picture_ownership_window_long_offset, owns_face_bitmap);
    InvalidateRect(hwnd, (RECT *)0, 0);
    return 0;

  case 0x438:
    face_bitmap = (HBITMAP)GetWindowLongPtrA(hwnd, g_face_picture_window_long_offset);
    return (LRESULT)face_bitmap;

  case WM_COMMAND:
    switch ((UINT)wparam & 0xffff)
    {
    case 102:
      command_player = g_duel_player_face_window_hwnd == hwnd ? 0 : 1;
      g_recorded_action_player = 0;
      post_face_directive_action(command_player);
      break;
    case 100:
      set_player_directive_value(g_duel_player_face_window_hwnd != hwnd, 0);
      break;
    case 101:
      help_context = 0xbdf;
      strcpy(help_path, global_base_directory);
      strcat(help_path, "\\duel.hlp");
      WinHelpA(g_duel_window_hwnd, help_path, HELP_CONTEXT, help_context);
      break;
    default:
      break;
    }
    return 0;

  case WM_CREATE:
    face_bitmap = (HBITMAP)0;
    SetWindowLongPtrA(hwnd, g_face_picture_window_long_offset, (LONG_PTR)face_bitmap);
    owns_face_bitmap = 0;
    SetWindowLongA(hwnd, g_face_picture_ownership_window_long_offset, owns_face_bitmap);
    return 0;

  case WM_DESTROY:
    face_bitmap = (HBITMAP)GetWindowLongPtrA(hwnd, g_face_picture_window_long_offset);
    owns_face_bitmap = GetWindowLongA(hwnd, g_face_picture_ownership_window_long_offset);
    if (face_bitmap != (HBITMAP)0 && owns_face_bitmap == 0)
    {
      delete_and_close_object(face_bitmap);
    }
    return 0;

  case WM_LBUTTONDOWN:
    click_player = g_duel_player_face_window_hwnd == hwnd ? 0 : 1;
    if (g_duel_modal_action_active != 0)
    {
      Sleep(GetDoubleClickTime());
      g_recorded_action_player = PeekMessageA(&peek_msg, hwnd, WM_LBUTTONDBLCLK, WM_LBUTTONDBLCLK, 0);
      post_face_directive_action(click_player);
    }
    return 0;

  case WM_PAINT:
    face_bitmap = (HBITMAP)GetWindowLongPtrA(hwnd, g_face_picture_window_long_offset);
    paint_dc = BeginPaint(hwnd, &paint);
    if (paint_dc != (HDC)0)
    {
      ApplyCardArtPaletteToDc(paint_dc);
      GetClientRect(hwnd, &client_rect);
      if (face_bitmap != (HBITMAP)0)
      {
        draw_duel_face_window(paint_dc, &client_rect, g_duel_player_face_window_hwnd != hwnd);
      }
      else
      {
        FillRect(paint_dc, &client_rect, GetStockObject(4));
      }
      EndPaint(hwnd, &paint);
    }
    return 0;

  case WM_RBUTTONDOWN:
    menu_point.x = lparam & 0xffff;
    menu_point.y = HIWORD(lparam);
    ClientToScreen(hwnd, &menu_point);
    SetRect(&popup_rect, menu_point.x, menu_point.y, menu_point.x + 1, menu_point.y + 1);
    TrackPopupMenu(g_face_popup_menu, TPM_RIGHTBUTTON, menu_point.x, menu_point.y, 0, hwnd, &popup_rect);
    return 0;

  case WM_INITMENU:
    menu_player = g_duel_life_status_window_1_hwnd == hwnd ? 0 : 1;
    if (g_duel_modal_action_active != 0 && is_face_directive_action_enabled(menu_player) != 0)
    {
      if (menu_player == 1)
      {
        copy_opponent_name_prefix(opponent_name);
        sprintf(directive_text, g_face_menu_directive_format, opponent_name);
      }
      else
      {
        strcpy(directive_text, g_face_menu_directive_self_text);
      }
      AppendMenuA(g_face_popup_menu, MF_ENABLED, 0x66, directive_text);
    }

    if (0 < GetMenuItemCount(g_face_popup_menu))
    {
      AppendMenuA(g_face_popup_menu, MF_SEPARATOR, 0, (LPCSTR)0);
    }
    AppendMenuA(g_face_popup_menu, MF_ENABLED, 100, g_face_menu_end_directive_text);
    AppendMenuA(g_face_popup_menu, MF_ENABLED, 0x65, g_face_menu_help_text);
    return 0;

  case WM_MENUSELECT:
    if (HIWORD(wparam) == 0xffff && lparam == 0)
    {
      menu_item_count = GetMenuItemCount(g_face_popup_menu);
      while (menu_item_count-- != 0)
      {
        DeleteMenu(g_face_popup_menu, 0, MF_BYPOSITION);
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

// FUNCTION: MAGIC 0x00464209
// FUNCTION: SHANDALAR 0x0042693e
void draw_duel_face_window(HDC dc, RECT *rect, int player)
{
  BITMAP bitmap;
  int saved_dc;
  RECT bitmap_rect;
  HBITMAP background_bitmap;
  POINT text_origin;
  int text_saved_dc;
  HBITMAP face_bitmap;
  char player_name[52];
  HWND face_window;

  if (player == 0)
  {
    face_window = g_duel_player_face_window_hwnd;
  }
  else
  {
    face_window = g_duel_opponent_face_window_hwnd;
  }

  face_bitmap = (HBITMAP)GetWindowLongPtrA(face_window, g_face_picture_window_long_offset);
  if (player == 0)
  {
    background_bitmap = g_face_background_pics[g_last_duel_player_primary_color];
  }
  else
  {
    background_bitmap = g_face_background_pics[g_last_duel_enemy_primary_color];
  }

  if (background_bitmap != (HBITMAP)0)
  {
    DrawBitmapToRect(dc, rect, background_bitmap);
  }
  else
  {
    FillRect(dc, rect, GetStockObject(4));
  }

  if (face_bitmap != (HBITMAP)0)
  {
    GetObjectA(face_bitmap, 0x18, &bitmap);
    bitmap.bmWidth = bitmap.bmWidth / 2;
    saved_dc = SaveDC(dc);
    SetMapMode(dc, MM_ISOTROPIC);
    SetWindowOrgEx(dc, bitmap.bmWidth / 2, bitmap.bmHeight / 2, (LPPOINT)0);
    SetViewportOrgEx(dc, rect->left + (rect->right - rect->left) / 2, rect->top + (rect->bottom - rect->top) / 2, (LPPOINT)0);
    SetWindowExtEx(dc, bitmap.bmWidth, bitmap.bmHeight, (LPSIZE)0);
    SetViewportExtEx(dc, rect->right - rect->left, rect->bottom - rect->top, (LPSIZE)0);
    SetRect(&bitmap_rect, 0, 0, bitmap.bmWidth, bitmap.bmHeight);
    draw_masked_bitmap_left_half_to_rect(dc, &bitmap_rect, face_bitmap);
    RestoreDC(dc, saved_dc);
  }

  if (player == 1)
  {
    copy_opponent_name_prefix(player_name);
  }
  else
  {
    strcpy(player_name, g_player_name);
  }

  if (strlen(player_name) != 0)
  {
    text_saved_dc = SaveDC(dc);
    SetMapMode(dc, MM_ANISOTROPIC);
    SetWindowExtEx(dc, rect->right - rect->left, 100, (LPSIZE)0);
    SetViewportExtEx(dc, rect->right - rect->left, rect->bottom - rect->top, (LPSIZE)0);
    SelectObject(dc, g_face_name_font);
    SetBkMode(dc, TRANSPARENT);
    SetTextAlign(dc, 0xe);
    text_origin.x = rect->left + (rect->right - rect->left) / 2;
    text_origin.y = rect->bottom;
    DPtoLP(dc, &text_origin, 1);
    SetTextColor(dc, g_face_name_shadow_color);
    TextOutA(dc, text_origin.x + 1, text_origin.y + 1, player_name, strlen(player_name));
    SetTextColor(dc, g_face_name_text_color);
    TextOutA(dc, text_origin.x, text_origin.y, player_name, strlen(player_name));
    RestoreDC(dc, text_saved_dc);
  }
}

// FUNCTION: MAGIC 0x0046452a
// FUNCTION: SHANDALAR 0x00426c59
void set_player_directive_value(int player, int value)
{
  HWND life_window;
  HWND face_window;
  HWND graveyard_window;
  HWND mana_summary_window;
  HWND library_window;

  if (player == 0)
  {
    face_window = g_duel_player_face_window_hwnd;
  }
  else
  {
    face_window = g_duel_opponent_face_window_hwnd;
  }

  if (player == 0)
  {
    life_window = g_duel_life_status_window_1_hwnd;
  }
  else
  {
    life_window = g_duel_life_status_window_2_hwnd;
  }

  if (player == 0)
  {
    library_window = g_duel_main_window_hwnd;
  }
  else
  {
    library_window = g_duel_status_window_hwnd;
  }

  if (player == 0)
  {
    graveyard_window = g_duel_player_graveyard_window_hwnd;
  }
  else
  {
    graveyard_window = g_phasebar_your_untap_window_hwnd;
  }

  if (player == 0)
  {
    mana_summary_window = g_duel_active_popup_window;
  }
  else
  {
    mana_summary_window = g_duel_last_active_window;
  }

  if (value != 0)
  {
    ShowWindow(face_window, SW_SHOW);
    BringWindowToTop(face_window);
    ShowWindow(life_window, SW_HIDE);
    if (g_duel_interface_options.layout != 2)
    {
      ShowWindow(library_window, SW_HIDE);
      ShowWindow(graveyard_window, SW_HIDE);
      ShowWindow(mana_summary_window, SW_HIDE);
    }
  }
  else
  {
    ShowWindow(life_window, SW_SHOW);
    ShowWindow(library_window, SW_SHOW);
    ShowWindow(graveyard_window, SW_SHOW);
    ShowWindow(mana_summary_window, SW_SHOW);
    ShowWindow(face_window, SW_HIDE);
  }
}

// FUNCTION: MAGIC 0x00464671
// FUNCTION: SHANDALAR 0x00426da0
void post_face_directive_action(int player)
{
  g_face_directive_packet.selection_code = 0;
  g_face_directive_packet.target_player = player;
  g_face_directive_packet.target_card = -1;
  g_face_directive_packet.unused = 0;
  PostMessageA(g_duel_window_hwnd, 0x464, 0, (LPARAM)&g_face_directive_packet);
}

// FUNCTION: MAGIC 0x004646b0
// FUNCTION: SHANDALAR 0x00426ddf
int is_face_directive_action_enabled(int player)
{
  if ((player == 1 && g_opponent_face_directive_action_enabled != 0) ||
      (player == 0 && g_player_face_directive_action_enabled != 0))
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// FUNCTION: MAGIC 0x004941ee
// FUNCTION: SHANDALAR 0x00464c8a
int draw_masked_bitmap_left_half_to_rect(HDC dc, RECT *rect, HANDLE bitmap)
{
  int result;
  BITMAP bitmap_info;
  int height;
  int half_width;
  int unused_14;
  int unused_10;
  int unused_c;
  int width;

  GetObjectA(bitmap, 0x18, &bitmap_info);
  half_width = bitmap_info.bmWidth / 2;
  height = bitmap_info.bmHeight;
  unused_10 = 0;
  unused_c = 0;
  width = half_width;
  unused_14 = 0;
  result = DrawMaskedBitmapToRect(dc, rect, bitmap, half_width, height, unused_10, unused_c, width, unused_14);
  return result;
}

// FUNCTION: MAGIC 0x0049545f
// FUNCTION: SHANDALAR 0x00465efa
LRESULT handle_duel_inactive_cursor(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg)
  {
  case WM_SETCURSOR:
    if (g_duel_modal_action_active == 0)
    {
      SetCursor(LoadCursorA((HINSTANCE)0, (LPCSTR)0x7f8a));
      return 0;
    }
    else
    {
      return DefWindowProcA(hwnd, msg, wparam, lparam);
    }

  default:
    return DefWindowProcA(hwnd, msg, wparam, lparam);
  }
}

// FUNCTION: MAGIC 0x004ecbd4
// FUNCTION: SHANDALAR 0x0056cdb4
int load_text_with_tab_escapes(char *filename, char *section_name)
{
  int loaded_count;
  int line_index;
  int line_count;
  size_t line_length;
  int out_index;
  int char_index;

  if (g_duel_ai_mode_state == 1)
  {
    loaded_count = 0;
  }
  else
  {
    loaded_count = load_text(filename, section_name);
    for (line_index = 0; line_count = abs(loaded_count), line_index < line_count; line_index++)
    {
      line_length = strlen(g_text_lines[line_index]);
      out_index = 0;
      for (char_index = 0; char_index < (int)line_length; char_index++)
      {
        if (g_text_lines[line_index][char_index] == '\\' && g_text_lines[line_index][char_index + 1] == 't')
        {
          g_text_lines[line_index][out_index] = '\t';
          char_index++;
        }
        else
        {
          g_text_lines[line_index][out_index] = g_text_lines[line_index][char_index];
        }
        out_index++;
      }
      g_text_lines[line_index][out_index] = '\0';
    }
  }
  return loaded_count;
}
