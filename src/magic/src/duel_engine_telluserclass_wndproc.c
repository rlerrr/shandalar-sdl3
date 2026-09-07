#include <windows.h>
#include <string.h>

#include "cardartlib/src/palette.h"
#include "deckdll/src/shared_resources.h"
#include "drawcardlib/Drawcardlib.h"
#include "drawcardlib/src/pic.h"
#include "magic/src/duel_engine.h"
#include "magic/src/game_support.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "magic/src/shared_startup.h"

BOOL TileBitmapIntoRect(HDC hdc, RECT *rect, HBITMAP bitmap);
void draw_item(DRAWITEMSTRUCT *item, HBRUSH brush, HANDLE background_bitmap, HPEN pen1,
               HPEN pen2, COLORREF text_color, int draw_focus, UINT format);
void position_duel_prompt_context_window(HWND hwnd);
void setup_duel_prompt_context_text_dc(HWND hwnd, HDC dc, int *rect);
void draw_owner_draw_button_centered(DRAWITEMSTRUCT *draw_item, HBRUSH brush, HPEN pen1, HPEN pen2,
                                     COLORREF color, int draw_focus);

extern int g_duel_modal_action_active;
extern char g_custom_duel_action_text[0x100];

// GLOBAL: MAGIC 0x0057b024
// GLOBAL: SHANDALAR 0x00586190
char g_telluser_short_text_1[4] = "";

// GLOBAL: MAGIC 0x0057b030
// GLOBAL: SHANDALAR 0x0058619c
char g_telluser_short_text_2[4] = "";

// GLOBAL: MAGIC 0x0057b03c
// GLOBAL: SHANDALAR 0x005861a8
char g_telluser_short_text_3[4] = "";

// GLOBAL: MAGIC 0x0055e180
// GLOBAL: SHANDALAR 0x0057f064
int g_tell_user_window_extra_bytes = 4;

// GLOBAL: MAGIC 0x006abde8
// GLOBAL: SHANDALAR 0x005a9540
HFONT g_tell_user_button_font;

// GLOBAL: MAGIC 0x006abdec
// GLOBAL: SHANDALAR 0x005a9544
COLORREF g_tell_user_text_color;

// GLOBAL: MAGIC 0x006abdf0
// GLOBAL: SHANDALAR 0x005a9548
COLORREF g_tell_user_shadow_color;

// GLOBAL: MAGIC 0x006abdf4
// GLOBAL: SHANDALAR 0x005a954c
HFONT g_tell_user_font;

// GLOBAL: MAGIC 0x006abdf8
// GLOBAL: SHANDALAR 0x005a9550
HPEN g_tell_user_border_pen_1;

// GLOBAL: MAGIC 0x006abdfc
// GLOBAL: SHANDALAR 0x005a9554
HPEN g_tell_user_button_pen_2;

// GLOBAL: MAGIC 0x006abe00
// GLOBAL: SHANDALAR 0x005a9558
HPEN g_tell_user_border_pen_3;

// GLOBAL: MAGIC 0x006abe04
// GLOBAL: SHANDALAR 0x005a955c
HPEN g_tell_user_button_pen_1;

// GLOBAL: MAGIC 0x006abe08
// GLOBAL: SHANDALAR 0x005a9560
COLORREF g_tell_user_button_text_color;

// GLOBAL: MAGIC 0x006abe0c
// GLOBAL: SHANDALAR 0x005a9564
HBRUSH g_tell_user_button_brush;

// GLOBAL: MAGIC 0x006abe10
// GLOBAL: SHANDALAR 0x005a9568
target_selection_result_t g_tell_user_command_packet;

// GLOBAL: MAGIC 0x006abe20
// GLOBAL: SHANDALAR 0x005a9578
HBITMAP g_tell_user_background_bitmap;

// GLOBAL: MAGIC 0x006abe24
// GLOBAL: SHANDALAR 0x005a957c
HPEN g_tell_user_border_pen_2;

// FUNCTION: MAGIC 0x004eb3e0
// FUNCTION: SHANDALAR 0x004690e0
int register_MAGIC_TellUserClass(LPCSTR class_name)
{
  char path[264];
  int result;
  WNDCLASSA wndclass;

  result = 1;
  g_tell_user_default_button_state = 2;
  g_tell_user_button_state = 1;
  SET_DUEL_WNDCLASS(wndclass, 1, wndproc_MAGIC_TellUserClass, g_tell_user_window_extra_bytes,
                    LoadIconA((HINSTANCE)0, (LPCSTR)0x7f00), (HBRUSH)GetStockObject(2), class_name);
  if (RegisterClassA(&wndclass) == 0)
  {
    result = 0;
  }

  strcpy(path, global_duelart_path);
  strcat(path, "\\WINBK_TellUser.pic");
  g_tell_user_background_bitmap = load_pic(path);
  load_text(global_ui_strings_filename, "BUTTONLABELS");
  strcpy(g_duel_status_text, g_text_lines[0]);
  strcpy(g_custom_duel_action_text, g_text_lines[1]);
  g_tell_user_font = CreateFontIndirectA(LoadFontFromIni("TellUser", 0));
  g_tell_user_button_font = CreateFontIndirectA(LoadFontFromIni("TellUser", 0));
  g_tell_user_border_pen_1 = CreatePen(0, 0, 0x10000cb);
  g_tell_user_border_pen_2 = CreatePen(0, 0, 0x10000cd);
  g_tell_user_border_pen_3 = CreatePen(0, 0, 0x10000cf);
  g_tell_user_text_color = 0x10000b6;
  g_tell_user_shadow_color = 0x10000c9;
  g_tell_user_button_brush = CreateSolidBrush(0x10000cd);
  g_tell_user_button_pen_1 = CreatePen(0, 0, 0x10000cb);
  g_tell_user_button_pen_2 = CreatePen(0, 0, 0x10000cf);
  g_tell_user_button_text_color = g_tell_user_text_color;
  if (g_tell_user_font == (HFONT)0 ||
      g_tell_user_button_font == (HFONT)0 ||
      g_tell_user_border_pen_1 == (HPEN)0 ||
      g_tell_user_border_pen_2 == (HPEN)0 ||
      g_tell_user_border_pen_3 == (HPEN)0 ||
      g_tell_user_button_brush == (HBRUSH)0 ||
      g_tell_user_button_pen_1 == (HPEN)0 ||
      g_tell_user_button_pen_2 == (HPEN)0)
  {
    result = 0;
  }

  return result;
}

// FUNCTION: MAGIC 0x004eb63c
// FUNCTION: SHANDALAR 0x0046933c
void destroy_MAGIC_TellUserClass(LPCSTR class_name)
{
  if (g_tell_user_background_bitmap != (HBITMAP)0)
  {
    delete_and_close_object(g_tell_user_background_bitmap);
  }
  if (g_tell_user_font != (HFONT)0)
  {
    DeleteObject(g_tell_user_font);
  }
  if (g_tell_user_button_font != (HFONT)0)
  {
    DeleteObject(g_tell_user_button_font);
  }
  if (g_tell_user_border_pen_1 != (HPEN)0)
  {
    DeleteObject(g_tell_user_border_pen_1);
  }
  if (g_tell_user_border_pen_2 != (HPEN)0)
  {
    DeleteObject(g_tell_user_border_pen_2);
  }
  if (g_tell_user_border_pen_3 != (HPEN)0)
  {
    DeleteObject(g_tell_user_border_pen_3);
  }
  if (g_tell_user_button_brush != (HBRUSH)0)
  {
    DeleteObject(g_tell_user_button_brush);
  }
  if (g_tell_user_button_pen_1 != (HPEN)0)
  {
    DeleteObject(g_tell_user_button_pen_1);
  }
  if (g_tell_user_button_pen_2 != (HPEN)0)
  {
    DeleteObject(g_tell_user_button_pen_2);
  }

  g_tell_user_background_bitmap = (HBITMAP)0;
  g_tell_user_font = (HFONT)0;
  g_tell_user_border_pen_1 = (HPEN)0;
  g_tell_user_border_pen_2 = (HPEN)0;
  g_tell_user_border_pen_3 = (HPEN)0;
  g_tell_user_button_brush = (HBRUSH)0;
  g_tell_user_button_pen_1 = (HPEN)0;
  g_tell_user_button_pen_2 = (HPEN)0;
}

// FUNCTION: MAGIC 0x004eb77a
// FUNCTION: SHANDALAR 0x0046947a
LRESULT CALLBACK wndproc_MAGIC_TellUserClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  char path[264];
  HDC paint_dc;
  PAINTSTRUCT paint;
  char window_text[200];
  RECT rect;
  char *button_text;
  DRAWITEMSTRUCT *draw_item;
  int unused_1c;
  int unused_18;
  int unused_14;
  int unused_10;
  unsigned int button_flags;
  int action_value;
  HFONT font;

  switch (msg)
  {
  case 0x401:
    if (g_duel_modal_action_active != 0 &&
        (IsWindowVisible(g_duel_client_window) != 0 || IsWindowVisible(g_duel_main_window) != 0))
    {
      if (wparam == 0)
      {
        if (IsWindowVisible(g_duel_main_window) != 0)
        {
          action_value = -2;
        }
        else
        {
          action_value = -1;
        }
      }
      else if (wparam == 2)
      {
        action_value = -2;
      }
      else
      {
        action_value = -1;
      }
      g_stop_phase_player = -1;
      g_stop_phase = -1;
      g_recorded_action_player = 0;
      g_tell_user_command_packet.selection_code = -2;
      g_tell_user_command_packet.target_player = -1;
      g_tell_user_command_packet.target_card = action_value;
      g_tell_user_command_packet.unused = 0;
      PostMessageA(g_duel_window_hwnd, 0x464, 0, (LPARAM)&g_tell_user_command_packet);
    }
    return 0;

  case 0x402:
    if (wparam != 0)
    {
      GetWindowTextA(hwnd, (LPSTR)wparam, 200);
    }
    button_flags = 0;
    if (IsWindowVisible(g_duel_client_window) != 0)
    {
      button_flags |= 1;
    }
    if (IsWindowVisible(g_duel_main_window) != 0)
    {
      button_flags |= 2;
    }
    return button_flags;

  case 0x403:
    position_duel_prompt_context_window(hwnd);
    return 0;

  case WM_COMMAND:
    switch (wparam & 0xffff)
    {
    case 1:
      SendMessageA(hwnd, 0x401, 1, 0);
      break;

    case 2:
      SendMessageA(hwnd, 0x401, 2, 0);

    default:
      break;
    }
    return 0;

  case WM_CREATE:
    font = g_tell_user_font;
    SetWindowLongA(hwnd, g_duel_ui_initialized, (LONG)font);
    g_duel_client_window = CreateWindowExA(0, "BUTTON", g_telluser_short_text_1, 0x4080000b,
                                           0, 0, 0, 0, hwnd, (HMENU)1, g_app_instance, (LPVOID)0);
    g_duel_main_window = CreateWindowExA(0, "BUTTON", g_telluser_short_text_2, 0x4080000b,
                                         0, 0, 0, 0, hwnd, (HMENU)2, g_app_instance, (LPVOID)0);
    if (g_duel_client_window == (HWND)0 || g_duel_main_window == (HWND)0)
    {
      return -1;
    }
    return 0;

  case WM_DRAWITEM:
    draw_item = (DRAWITEMSTRUCT *)lparam;
    draw_item->itemState &= ~ODS_FOCUS;
    draw_owner_draw_button_centered(draw_item, g_tell_user_button_brush, g_tell_user_border_pen_1,
                                    g_tell_user_border_pen_3, g_tell_user_button_text_color, 0);
    if (draw_item->CtlID == 1)
    {
      button_text = g_duel_status_text;
    }
    else if (draw_item->CtlID == 2)
    {
      button_text = g_custom_duel_action_text;
    }
    else
    {
      button_text = g_telluser_short_text_3;
    }
    SetMapMode(draw_item->hDC, MM_ANISOTROPIC);
    SetWindowExtEx(draw_item->hDC, draw_item->rcItem.right - draw_item->rcItem.left,
                   0x18, (LPSIZE)0);
    SetViewportExtEx(draw_item->hDC, draw_item->rcItem.right - draw_item->rcItem.left,
                     draw_item->rcItem.bottom - draw_item->rcItem.top, (LPSIZE)0);
    SelectObject(draw_item->hDC, g_tell_user_button_font);
    SetTextColor(draw_item->hDC, g_tell_user_button_text_color);
    if ((draw_item->itemState & ODS_SELECTED) != 0)
    {
      OffsetRect(&draw_item->rcItem, 2, 2);
    }
    DPtoLP(draw_item->hDC, (LPPOINT)&draw_item->rcItem, 2);
    DrawTextA(draw_item->hDC, button_text, -1, &draw_item->rcItem,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    return 1;

  case WM_ERASEBKGND:
    return 1;

  case WM_GETFONT:
    font = (HFONT)GetWindowLongA(hwnd, g_duel_ui_initialized);
    return (LRESULT)font;

  case WM_LBUTTONDOWN:
    SendMessageA(hwnd, WM_SYSCOMMAND, 0xf012, 0);
    return 0;

  case WM_PAINT:
    font = (HFONT)GetWindowLongA(hwnd, g_duel_ui_initialized);
    paint_dc = BeginPaint(hwnd, &paint);
    if (paint_dc != (HDC)0)
    {
      ApplyCardArtPaletteToDc(paint_dc);
      GetClientRect(hwnd, &rect);
      if (g_tell_user_background_bitmap == (HBITMAP)0)
      {
        strcpy(path, global_duelart_path);
        strcat(path, "\\WINBK_TellUser.pic");
        g_tell_user_background_bitmap = load_pic(path);
      }
      if (g_tell_user_background_bitmap != (HBITMAP)0)
      {
        TileBitmapIntoRect(paint_dc, &rect, g_tell_user_background_bitmap);
      }
      else
      {
        FillRect(paint_dc, &rect, GetStockObject(1));
      }
      SelectObject(paint_dc, g_tell_user_border_pen_1);
      MoveToEx(paint_dc, 0, 0, (LPPOINT)0);
      LineTo(paint_dc, rect.right - 1, 0);
      MoveToEx(paint_dc, 0, 0, (LPPOINT)0);
      LineTo(paint_dc, 0, rect.bottom - 1);
      SelectObject(paint_dc, g_tell_user_border_pen_2);
      MoveToEx(paint_dc, 1, 1, (LPPOINT)0);
      LineTo(paint_dc, rect.right - 2, 1);
      MoveToEx(paint_dc, 1, 1, (LPPOINT)0);
      LineTo(paint_dc, 1, rect.bottom - 2);
      SelectObject(paint_dc, g_tell_user_border_pen_3);
      MoveToEx(paint_dc, 2, 2, (LPPOINT)0);
      LineTo(paint_dc, rect.right - 4, 2);
      MoveToEx(paint_dc, 2, 2, (LPPOINT)0);
      LineTo(paint_dc, 2, rect.bottom - 4);
      SelectObject(paint_dc, GetStockObject(7));
      MoveToEx(paint_dc, 3, 3, (LPPOINT)0);
      LineTo(paint_dc, rect.right - 4, 3);
      MoveToEx(paint_dc, 3, 3, (LPPOINT)0);
      LineTo(paint_dc, 3, rect.bottom - 4);
      SelectObject(paint_dc, GetStockObject(7));
      MoveToEx(paint_dc, 0, rect.bottom - 1, (LPPOINT)0);
      LineTo(paint_dc, rect.right, rect.bottom - 1);
      MoveToEx(paint_dc, rect.right - 1, 0, (LPPOINT)0);
      LineTo(paint_dc, rect.right - 1, rect.bottom);
      SelectObject(paint_dc, g_tell_user_border_pen_3);
      MoveToEx(paint_dc, 1, rect.bottom - 2, (LPPOINT)0);
      LineTo(paint_dc, rect.right - 1, rect.bottom - 2);
      MoveToEx(paint_dc, rect.right - 2, 1, (LPPOINT)0);
      LineTo(paint_dc, rect.right - 2, rect.bottom - 1);
      SelectObject(paint_dc, g_tell_user_border_pen_2);
      MoveToEx(paint_dc, 2, rect.bottom - 3, (LPPOINT)0);
      LineTo(paint_dc, rect.right - 2, rect.bottom - 3);
      MoveToEx(paint_dc, rect.right - 3, 2, (LPPOINT)0);
      LineTo(paint_dc, rect.right - 3, rect.bottom - 2);
      SelectObject(paint_dc, g_tell_user_border_pen_1);
      MoveToEx(paint_dc, 2, rect.bottom - 4, (LPPOINT)0);
      LineTo(paint_dc, rect.right - 3, rect.bottom - 4);
      MoveToEx(paint_dc, rect.right - 4, 2, (LPPOINT)0);
      LineTo(paint_dc, rect.right - 4, rect.bottom - 3);
      GetWindowTextA(hwnd, window_text, 200);
      SetBkMode(paint_dc, TRANSPARENT);
      setup_duel_prompt_context_text_dc(hwnd, paint_dc, (int *)&rect.left);
      DPtoLP(paint_dc, (LPPOINT)&rect, 2);
      OffsetRect(&rect, 2, 2);
      SetTextColor(paint_dc, g_tell_user_shadow_color);
      DrawManaText(paint_dc, &rect, window_text, 0);
      OffsetRect(&rect, -2, -2);
      SetTextColor(paint_dc, g_tell_user_text_color);
      DrawManaText(paint_dc, &rect, window_text, 1);
      EndPaint(hwnd, &paint);
    }
    return 0;

  case WM_SETFONT:
    font = (HFONT)wparam;
    if (font == (HFONT)0)
    {
      font = g_tell_user_font;
    }
    SetWindowLongA(hwnd, g_duel_ui_initialized, (LONG)font);
    InvalidateRect(hwnd, (RECT *)0, 1);
    return 0;

  case WM_QUERYNEWPALETTE:
  case WM_PALETTEISCHANGING:
  case WM_PALETTECHANGED:
    return handle_button_palette_message((int)hwnd, msg, (int)wparam, lparam);

  default:
    return DefWindowProcA(hwnd, msg, wparam, lparam);
  }

  return 0;
}

// FUNCTION: MAGIC 0x004955ae
// FUNCTION: SHANDALAR 0x00466049
void draw_owner_draw_button_centered(DRAWITEMSTRUCT *draw_item_struct, HBRUSH brush, HPEN pen1, HPEN pen2,
                                     COLORREF color, int draw_focus)
{
  draw_item(draw_item_struct, brush, (HANDLE)0, pen1, pen2, color, draw_focus,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}
