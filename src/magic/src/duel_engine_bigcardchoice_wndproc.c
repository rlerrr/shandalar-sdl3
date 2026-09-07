#include <windows.h>
#include <stdlib.h>
#include <string.h>

#include "cardartlib/src/palette.h"
#include "drawcardlib/Drawcardlib.h"
#include "magic/src/duel_engine.h"
#include "magic/src/global_other.h"
#include "magic/src/shared_startup.h"

HBITMAP load_pic(char *filename);
int handle_button_palette_message(HWND hwnd, UINT msg, HWND wparam, LPARAM lparam);

extern int g_big_card_choice_timeout_ms;

#define BIGCARD_CHOICE_FONT_WINDOW_LONG_OFFSET g_big_card_choice_font_window_long_offset
#define BIGCARD_CHOICE_ITEMS_WINDOW_LONG_OFFSET g_big_card_choice_items_window_long_offset
#define BIGCARD_CHOICE_COUNT_WINDOW_LONG_OFFSET g_big_card_choice_count_window_long_offset
#define BIGCARD_CHOICE_SELECTED_WINDOW_LONG_OFFSET g_big_card_choice_selected_window_long_offset
#define BIGCARD_CHOICE_MODE_WINDOW_LONG_OFFSET g_big_card_choice_mode_window_long_offset

// GLOBAL: MAGIC 0x0055e19c
// GLOBAL: SHANDALAR 0x0057f0b8
int g_big_card_choice_font_window_long_offset = 0;

// GLOBAL: MAGIC 0x0055e1a0
// GLOBAL: SHANDALAR 0x0057f0bc
int g_big_card_choice_items_window_long_offset = 4;

// GLOBAL: MAGIC 0x0055e1a4
// GLOBAL: SHANDALAR 0x0057f0c0
int g_big_card_choice_count_window_long_offset = 8;

// GLOBAL: MAGIC 0x0055e1a8
// GLOBAL: SHANDALAR 0x0057f0c4
int g_big_card_choice_selected_window_long_offset = 0xc;

// GLOBAL: MAGIC 0x0055e1ac
// GLOBAL: SHANDALAR 0x0057f0c8
int g_big_card_choice_mode_window_long_offset = 0x10;

// GLOBAL: MAGIC 0x0055e1b0
// GLOBAL: SHANDALAR 0x0057f0cc
int g_big_card_choice_window_extra_bytes = 0x14;

extern char g_big_card_choice_window_text_buffer[0xc8];

int big_card_choice_item_is_enabled(HWND hwnd, int item);

// FUNCTION: MAGIC 0x00507e37
// FUNCTION: SHANDALAR 0x004c6b35
int register_MAGICGAME_BigCardChoiceClass(LPCSTR class_name)
{
  char path[264];
  int registered;
  WNDCLASSA wndclass;

  registered = 1;
  SET_DUEL_WNDCLASS(wndclass, 0, wndproc_MAGICGAME_BigCardChoiceClass,
                    g_big_card_choice_window_extra_bytes,
                    LoadIconA((HINSTANCE)0, (LPCSTR)0x7f00), (HBRUSH)0x6, class_name);
  if (RegisterClassA(&wndclass) == 0)
  {
    registered = 0;
  }

  strcpy(path, global_duelart_path);
  strcat(path, "\\WINBK_BigCard.pic");
  g_magicgame_big_card_choice_pic = load_pic(path);
  g_big_card_choice_timeout_ms = 9000;
  return registered;
}

// FUNCTION: MAGIC 0x00507f42
// FUNCTION: SHANDALAR 0x004c6c40
LRESULT CALLBACK wndproc_MAGICGAME_BigCardChoiceClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  int strlen_result;
  int any_enabled;
  int found_choices;
  int text_index;
  LRESULT text_result;
  char *source;
  RECT selection_rect;
  SIZE text_size;
  COLORREF color;
  HDC paint_dc;
  int line_height;
  PAINTSTRUCT paint;
  int item_y;
  int index;
  COLORREF title_color;
  COLORREF normal_color;
  COLORREF selected_text_color;
  COLORREF selected_color;
  char text[500];
  TEXTMETRICA metrics;
  COLORREF disabled_color;
  RECT rect; // ebp - 0xd0
  COLORREF shadow_color;
  char *item_text; // ebp - 0xbc
  int title_height;
  RECT before_drag_rect;
  int mouse_line_height; // ebp - 0xa4
  RECT mouse_selection_rect;
  int mouse_x;
  int mouse_y;
  HDC mouse_dc;
  int new_selected; // ebp - 0x84
  int mouse_index;  // ebp - 0x80
  int mouse_item_y; // ebp - 0x7c
  int was_dragged;
  TEXTMETRICA mouse_metrics;
  RECT after_drag_rect;
  RECT child_rect;
  RECT parent_rect;
  HDC parent_dc;
  RECT mouse_rect;
  int command_selected;
  union
  {
    char *create_name;
    int enabled;
  } u18;
  int count;    // ebp - 0x14
  int selected; // ebp - 0x10
  int mode;     // ebp - 0xc
  HFONT font;   // ebp - 0x8
  char *items;  // ebp - 0x4

  switch (msg)
  {
  case 0x400:
    count = GetWindowLongA(hwnd, BIGCARD_CHOICE_COUNT_WINDOW_LONG_OFFSET);
    return count;

  case 0x401:
    mode = (int)wparam;
    SetWindowLongA(hwnd, BIGCARD_CHOICE_MODE_WINDOW_LONG_OFFSET, mode);
    return 0;

  case WM_CHAR:
    SendMessageA(GetParent(hwnd), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwnd), 0), (LPARAM)hwnd);
    return 0;

  case WM_CREATE:
    font = 0;
    SetWindowLongPtrA(hwnd, BIGCARD_CHOICE_FONT_WINDOW_LONG_OFFSET, (LONG_PTR)font);
    items = (char *)malloc(0x3e8);
    count = 0;
    selected = -1;
    SetWindowLongA(hwnd, BIGCARD_CHOICE_COUNT_WINDOW_LONG_OFFSET, count);
    SetWindowLongPtrA(hwnd, BIGCARD_CHOICE_ITEMS_WINDOW_LONG_OFFSET, (LONG_PTR)items);
    SetWindowLongA(hwnd, BIGCARD_CHOICE_SELECTED_WINDOW_LONG_OFFSET, selected);
    mode = (int)(intptr_t)((CREATESTRUCTA *)lparam)->lpCreateParams;
    SetWindowLongA(hwnd, BIGCARD_CHOICE_MODE_WINDOW_LONG_OFFSET, mode);
    u18.create_name = (char *)((CREATESTRUCTA *)lparam)->lpszName;
    if (u18.create_name != NULL)
    {
      PostMessageA(hwnd, WM_SETTEXT, 0, (LPARAM)u18.create_name);
    }
    return 0;

  case WM_DESTROY:
    items = (char *)GetWindowLongPtrA(hwnd, BIGCARD_CHOICE_ITEMS_WINDOW_LONG_OFFSET);
    if (items != NULL)
    {
      free(items);
    }
    return 0;

  case WM_ERASEBKGND:
#ifdef SHANDALAR
    ApplyCardArtPaletteToDc((HDC)wparam);
    /*
     * This control is intentionally transparent in the original UI.  Win32
     * paints it directly over the dialog, while the SDL compatibility layer
     * gives every child its own backing surface.  Seed that surface with the
     * corresponding dialog pixels before drawing the prompt and choices.
     */
    parent_dc = GetDC(GetParent(hwnd));
    if (parent_dc != (HDC)0 &&
        GetWindowRect(hwnd, &child_rect) &&
        GetWindowRect(GetParent(hwnd), &parent_rect))
    {
      BitBlt((HDC)wparam, 0, 0,
             child_rect.right - child_rect.left,
             child_rect.bottom - child_rect.top,
             parent_dc,
             child_rect.left - parent_rect.left,
             child_rect.top - parent_rect.top -
                 (((GetWindowLongA(GetParent(hwnd), GWL_STYLE) & WS_CAPTION) ==
                   WS_CAPTION)
                      ? GetSystemMetrics(SM_CYCAPTION)
                      : 0),
             SRCCOPY);
    }
    if (parent_dc != (HDC)0)
    {
      ReleaseDC(GetParent(hwnd), parent_dc);
    }
#endif
    return 1;

  case WM_GETDLGCODE:
    return 4;

  case WM_GETFONT:
    font = (HFONT)GetWindowLongPtrA(hwnd, BIGCARD_CHOICE_FONT_WINDOW_LONG_OFFSET);
    return (LRESULT)font;

  case WM_KEYDOWN:
    count = GetWindowLongA(hwnd, BIGCARD_CHOICE_COUNT_WINDOW_LONG_OFFSET);
    selected = GetWindowLongA(hwnd, BIGCARD_CHOICE_SELECTED_WINDOW_LONG_OFFSET);
    mode = GetWindowLongA(hwnd, BIGCARD_CHOICE_MODE_WINDOW_LONG_OFFSET);
    if (mode != 0)
    {
      if (wparam == VK_RETURN || wparam == VK_SPACE)
      {
        command_selected = selected + 1;
        SendMessageA(GetParent(hwnd),
                     WM_COMMAND,
                     MAKELONG(GetDlgCtrlID(hwnd), command_selected),
                     (LPARAM)hwnd);
      }
      else
      {
        if (wparam == VK_TAB)
        {
          if ((GetKeyState(VK_SHIFT) & 0x8000) != 0)
          {
            wparam = VK_UP;
          }
          else
          {
            wparam = VK_DOWN;
          }
        }

        switch (wparam)
        {
        case VK_HOME:
          selected = 0;
          while (!big_card_choice_item_is_enabled(hwnd, selected))
          {
            selected++;
          }
          break;

        case VK_END:
          selected = count - 1;
          while (!big_card_choice_item_is_enabled(hwnd, selected))
          {
            selected--;
          }
          break;

        case VK_UP:
          if (--selected < 0)
          {
            selected = count - 1;
          }

          while (!big_card_choice_item_is_enabled(hwnd, selected))
          {
            if (--selected < 0)
            {
              selected = count - 1;
            }
          }

          break;

        case VK_DOWN:
          if (count - 1 < ++selected)
          {
            selected = 0;
          }
          while (!big_card_choice_item_is_enabled(hwnd, selected))
          {
            if (count - 1 < ++selected)
            {
              selected = 0;
            }
          }
          break;
        }

        SetWindowLongA(hwnd, BIGCARD_CHOICE_SELECTED_WINDOW_LONG_OFFSET, selected);
        InvalidateRect(hwnd, (RECT *)0, 0);
      }
    }
    return 0;

  case WM_LBUTTONDOWN:
  case WM_MOUSEMOVE:
  case WM_LBUTTONUP:
    count = GetWindowLongA(hwnd, BIGCARD_CHOICE_COUNT_WINDOW_LONG_OFFSET);
    font = (HFONT)GetWindowLongPtrA(hwnd, BIGCARD_CHOICE_FONT_WINDOW_LONG_OFFSET);
    selected = GetWindowLongA(hwnd, BIGCARD_CHOICE_SELECTED_WINDOW_LONG_OFFSET);
    mode = GetWindowLongA(hwnd, BIGCARD_CHOICE_MODE_WINDOW_LONG_OFFSET);
    if (msg == WM_LBUTTONDOWN)
    {
      GetWindowRect(GetParent(hwnd), &before_drag_rect);
      SendMessageA(GetParent(hwnd), WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
      msg = WM_LBUTTONUP;
      GetWindowRect(GetParent(hwnd), &after_drag_rect);
      if (abs(before_drag_rect.top - after_drag_rect.top) +
              abs(before_drag_rect.left - after_drag_rect.left) <
          5)
      {
        was_dragged = 0;
      }
      else
      {
        was_dragged = 1;
      }
    }
    else
    {
      was_dragged = 0;
    }

    if (was_dragged == 0)
    {
      new_selected = selected + 1;
      if (mode == 0)
      {
        if (msg == WM_LBUTTONUP)
        {
          SendMessageA(GetParent(hwnd),
                       WM_COMMAND,
                       MAKELONG(GetDlgCtrlID(hwnd), new_selected),
                       (LPARAM)hwnd);
        }
      }
      else if (count < 1)
      {
        if (msg == WM_LBUTTONUP)
        {
          SendMessageA(GetParent(hwnd), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwnd), 0), (LPARAM)hwnd);
        }
      }
      else
      {
        mouse_x = lparam & 0xffff;
        mouse_y = HIWORD(lparam);
        mouse_dc = GetDC(hwnd);
        ApplyCardArtPaletteToDc(mouse_dc);
        SelectObject(mouse_dc, font);
        GetTextMetricsA(mouse_dc, &mouse_metrics);
        mouse_line_height = mouse_metrics.tmHeight + mouse_metrics.tmExternalLeading;
        ReleaseDC(hwnd, mouse_dc);
        new_selected = 0;
        GetClientRect(hwnd, &mouse_rect);
        mouse_item_y = mouse_rect.bottom - mouse_line_height;
        for (mouse_index = count; mouse_index > 0 && new_selected == 0; mouse_item_y -= mouse_line_height, mouse_index--)
        {
          if (mouse_item_y < mouse_y)
          {
            new_selected = mouse_index;
            SetRect(&mouse_selection_rect, 0, mouse_item_y,
                    mouse_rect.right, mouse_item_y + mouse_line_height);
          }
        }
        if (big_card_choice_item_is_enabled(hwnd, new_selected - 1))
        {
          selected = new_selected - 1;
          SetWindowLongA(hwnd, BIGCARD_CHOICE_SELECTED_WINDOW_LONG_OFFSET, selected);
          InvalidateRect(hwnd, (RECT *)0, 0);
          UpdateWindow(hwnd);
          if (msg == WM_LBUTTONUP)
          {
            Sleep(750);
            SendMessageA(GetParent(hwnd),
                         WM_COMMAND,
                         MAKELONG(GetDlgCtrlID(hwnd), new_selected),
                         (LPARAM)hwnd);
          }
        }
      }
    }
    return 0;

  case WM_PAINT:
    font = (HFONT)GetWindowLongPtrA(hwnd, BIGCARD_CHOICE_FONT_WINDOW_LONG_OFFSET);
    count = GetWindowLongA(hwnd, BIGCARD_CHOICE_COUNT_WINDOW_LONG_OFFSET);
    items = (char *)GetWindowLongPtrA(hwnd, BIGCARD_CHOICE_ITEMS_WINDOW_LONG_OFFSET);
    selected = GetWindowLongA(hwnd, BIGCARD_CHOICE_SELECTED_WINDOW_LONG_OFFSET);
    mode = GetWindowLongA(hwnd, BIGCARD_CHOICE_MODE_WINDOW_LONG_OFFSET);
    GetWindowTextA(hwnd, text, sizeof(text));

    item_text = items;
    title_color = 0x01000097;
    normal_color = 0x01000097;
    selected_text_color = 0x01000097;
    disabled_color = 0x010000c3;
    selected_color = 0x010000bf;
    shadow_color = 0x010000c9;
    paint_dc = BeginPaint(hwnd, &paint);
    if (paint_dc != (HDC)0)
    {
      ApplyCardArtPaletteToDc(paint_dc);
      GetClientRect(hwnd, &rect);
      SetBkMode(paint_dc, TRANSPARENT);
      SelectObject(paint_dc, font);
      SetTextColor(paint_dc, shadow_color);
      OffsetRect(&rect, 1, 1);
      DrawTextA(paint_dc, text, -1, &rect, DT_WORDBREAK);
      OffsetRect(&rect, -1, -1);
      SetTextColor(paint_dc, title_color);
      DrawTextA(paint_dc, text, -1, &rect, DT_WORDBREAK);
      title_height = DrawTextA(paint_dc, text, -1, &rect, DT_CALCRECT | DT_WORDBREAK);
      if (count != 0)
      {
        item_y = rect.top + title_height + 20;
        GetTextMetricsA(paint_dc, &metrics);
        line_height = metrics.tmHeight + metrics.tmExternalLeading +
                      (metrics.tmHeight * 10) / 100;
        SetBkMode(paint_dc, TRANSPARENT);
        for (index = 0; index < count; index++)
        {
          if (big_card_choice_item_is_enabled(hwnd, index) == 0)
          {
            item_text++;
            if (*item_text == ' ')
            {
              item_text++;
            }
          }

          if (mode != 0)
          {
            if (big_card_choice_item_is_enabled(hwnd, index) == 0)
            {
              color = disabled_color;
            }
            else if (index == selected)
            {
              color = selected_color;
            }
            else
            {
              color = normal_color;
            }
          }
          else
          {
            if (index == selected)
            {
              GetTextExtentPointA(paint_dc, item_text, strlen(item_text), &text_size);
              SetRect(&selection_rect,
                      rect.left - 5,
                      item_y,
                      rect.left + text_size.cx + 5,
                      item_y + line_height);
              FillRect(paint_dc, &selection_rect, GetStockObject(0));
            }

            if (big_card_choice_item_is_enabled(hwnd, index) == 0)
            {
              color = disabled_color;
            }
            else
            {
              color = selected_text_color;
            }
          }

          SetTextColor(paint_dc, shadow_color);
          TextOutA(paint_dc, rect.left + 1, item_y + 1, item_text, strlen(item_text));
          SetTextColor(paint_dc, color);
          TextOutA(paint_dc, rect.left, item_y, item_text, strlen(item_text));
          item_y += line_height;
          item_text += strlen(item_text) + 1;
        }
      }
      EndPaint(hwnd, &paint);
      if (count != 0)
      {
        GetClientRect(hwnd, &rect);
        rect.bottom = item_y;
        SetWindowPos(hwnd, (HWND)0, 0, 0,
                     rect.right - rect.left,
                     rect.bottom - rect.top,
                     SWP_NOZORDER | SWP_NOMOVE);
      }
    }
    return 0;

  case WM_SETFONT:
    font = (HFONT)GetWindowLongPtrA(hwnd, BIGCARD_CHOICE_FONT_WINDOW_LONG_OFFSET);
    if ((HFONT)wparam != font)
    {
      font = (HFONT)wparam;
      SetWindowLongPtrA(hwnd, BIGCARD_CHOICE_FONT_WINDOW_LONG_OFFSET, (LONG_PTR)font);
      InvalidateRect(hwnd, (RECT *)0, 1);
    }
    return 0;

  case WM_SETTEXT:
    source = (char *)lparam;
    text_index = 0;
    found_choices = 0;
    while (*source != '\0' && found_choices == 0)
    {
      if (*source == ' ' || *source == '>')
      {
        g_big_card_choice_window_text_buffer[text_index] = '\0';
        found_choices = 1;
      }
      else
      {
        while (*source != '\0' && *source != '\n')
        {
          g_big_card_choice_window_text_buffer[text_index] = *source;
          source++;
          text_index++;
        }
        if (*source != '\0')
        {
          g_big_card_choice_window_text_buffer[text_index] = *source;
          source++;
          text_index++;
        }
      }
    }

    g_big_card_choice_window_text_buffer[text_index] = '\0';
    while (strlen(g_big_card_choice_window_text_buffer) >= 2 &&
           g_big_card_choice_window_text_buffer[strlen(g_big_card_choice_window_text_buffer) - 2] == '\n')
    {
      strlen_result = strlen(g_big_card_choice_window_text_buffer);
      g_big_card_choice_window_text_buffer[strlen_result - 2] = '\0';
    }

    text_result = DefWindowProcA(hwnd, msg, wparam, (LPARAM)g_big_card_choice_window_text_buffer);
    items = (char *)GetWindowLongPtrA(hwnd, BIGCARD_CHOICE_ITEMS_WINDOW_LONG_OFFSET);
    count = 0;
    selected = -1;
    if (*source != '\0')
    {
      text_index = 0;
      while (*source != '\0')
      {
        while (*source == ' ')
        {
          source++;
        }

        if (*source == '>')
        {
          selected = count;
          source++;
        }

        while (*source == '\n')
        {
          source++;
        }

        while (*source != '\0' && *source != '\n')
        {
          items[text_index] = *source;
          source++;
          text_index++;
        }
        items[text_index] = '\0';
        text_index++;
        if (*source != '\0')
        {
          source++;
        }
        count++;
      }

      items[text_index] = '\0';
    }

    if (count != 0 && selected == -1)
    {
      selected = 0;
    }

    any_enabled = 0;
    for (text_index = 0; text_index < count; text_index++)
    {
      any_enabled |= big_card_choice_item_is_enabled(hwnd, text_index);
    }
    if (any_enabled == 0)
    {
      mode = 0;
      SetWindowLongA(hwnd, BIGCARD_CHOICE_MODE_WINDOW_LONG_OFFSET, mode);
    }
    SetWindowLongA(hwnd, BIGCARD_CHOICE_COUNT_WINDOW_LONG_OFFSET, count);
    SetWindowLongPtrA(hwnd, BIGCARD_CHOICE_ITEMS_WINDOW_LONG_OFFSET, (LONG_PTR)items);
    SetWindowLongA(hwnd, BIGCARD_CHOICE_SELECTED_WINDOW_LONG_OFFSET, selected);
    return text_result;

  case WM_QUERYNEWPALETTE:
  case WM_PALETTEISCHANGING:
  case WM_PALETTECHANGED:
    return handle_button_palette_message(hwnd, msg, (HWND)wparam, lparam);

  default:
    return DefWindowProcA(hwnd, msg, wparam, lparam);
  }
}

// FUNCTION: MAGIC 0x00509154
// FUNCTION: SHANDALAR 0x004c7e59
int big_card_choice_item_is_enabled(HWND hwnd, int item)
{
  int result;
  int count;
  int index;
  char *items;
  char *item_text;

  if (hwnd == (HWND)0)
  {
    return 0;
  }
  items = (char *)GetWindowLongPtrA(hwnd, BIGCARD_CHOICE_ITEMS_WINDOW_LONG_OFFSET);
  count = GetWindowLongA(hwnd, BIGCARD_CHOICE_COUNT_WINDOW_LONG_OFFSET);
  if (count - 1 < item)
  {
    return 0;
  }

  item_text = items;

  for (index = 0; index < item; index++)
  {
    item_text += strlen(item_text) + 1;
  }

  if (*item_text == '_')
  {
    result = 0;
  }
  else
  {
    result = 1;
  }
  return result;
}
