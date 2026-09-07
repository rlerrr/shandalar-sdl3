#include <windows.h>
#include <string.h>

#include "cardartlib/src/palette.h"
#include "magic/src/duel_engine.h"
#include "magic/src/global_other.h"
#include "magic/src/global_strings.h"

BOOL DrawMaskedBitmapToRect(HDC dc, RECT *rect, HANDLE bitmap, int width, int height, int src_x, int src_y, int mask_x, int mask_y);
int draw_masked_bitmap_left_half_to_rect(HDC dc, RECT *rect, HANDLE bitmap);
int handle_button_palette_message(HWND hwnd, UINT msg, HWND wparam, LPARAM lparam);
void checked_DeleteDC_DeleteObject(HDC dc, HGDIOBJ obj);

#define SCROLLBAR_OFFSCREEN_DC_PTR (&g_scrollbar_offscreen_dc)
#define SCROLLBAR_OFFSCREEN_BITMAP_PTR (&g_scrollbar_offscreen_bitmap)
#define SCROLLBAR_OFFSCREEN_DC g_scrollbar_offscreen_dc
#define SCROLLBAR_LAST_MOUSE_X g_scrollbar_last_mouse_x
#define SCROLLBAR_LAST_MOUSE_Y g_scrollbar_last_mouse_y
#define SCROLLBAR_OFFSCREEN_BITMAP g_scrollbar_offscreen_bitmap

// GLOBAL: MAGIC 0x00638628
// GLOBAL: SHANDALAR 0x00653188
HDC g_scrollbar_offscreen_dc;

// GLOBAL: MAGIC 0x00638630
// GLOBAL: SHANDALAR 0x00653190
int g_scrollbar_last_mouse_x;

// GLOBAL: MAGIC 0x00638634
// GLOBAL: SHANDALAR 0x00653194
int g_scrollbar_last_mouse_y;

// GLOBAL: MAGIC 0x00638638
// GLOBAL: SHANDALAR 0x00653198
HBITMAP g_scrollbar_offscreen_bitmap;

// GLOBAL: MAGIC 0x0055e048
// GLOBAL: SHANDALAR 0x0057f140
int g_scrollbar_position_window_long_offset = 0;

// GLOBAL: MAGIC 0x0055e04c
// GLOBAL: SHANDALAR 0x0057f144
int g_scrollbar_min_position_window_long_offset = 4;

// GLOBAL: MAGIC 0x0055e050
// GLOBAL: SHANDALAR 0x0057f148
int g_scrollbar_max_position_window_long_offset = 8;

// GLOBAL: MAGIC 0x0055e054
// GLOBAL: SHANDALAR 0x0057f14c
int g_scrollbar_direction_window_long_offset = 0xc;

// GLOBAL: MAGIC 0x0055e058
// GLOBAL: SHANDALAR 0x0057f150
int g_scrollbar_track_bitmap_window_long_offset = 0x10;

// GLOBAL: MAGIC 0x0055e05c
// GLOBAL: SHANDALAR 0x0057f154
int g_scrollbar_track_bitmap_layout_window_long_offset = 0x14;

// GLOBAL: MAGIC 0x0055e060
// GLOBAL: SHANDALAR 0x0057f158
int g_scrollbar_thumb_bitmap_window_long_offset = 0x18;

// GLOBAL: MAGIC 0x0055e064
// GLOBAL: SHANDALAR 0x0057f15c
int g_scrollbar_thumb_bitmap_frame_count_window_long_offset = 0x1c;

// GLOBAL: MAGIC 0x0055e068
// GLOBAL: SHANDALAR 0x0057f160
int g_scrollbar_thumb_position_window_long_offset = 0x20;

// GLOBAL: MAGIC 0x0055e06c
// GLOBAL: SHANDALAR 0x0057f164
int g_scrollbar_thumb_visible_window_long_offset = 0x24;

// GLOBAL: MAGIC 0x0055e070
// GLOBAL: SHANDALAR 0x0057f168
int g_scrollbar_window_extra_bytes = 0x28;

// FUNCTION: MAGIC 0x00490900
// FUNCTION: SHANDALAR 0x005562d0
int register_MAGICGAME_ScrollbarClass(LPCSTR class_name)
{
  int registered;
  WNDCLASSA wndclass;

  registered = 1;
  SET_DUEL_WNDCLASS(wndclass, 1, wndproc_MAGICGAME_ScrollbarClass,
                    g_scrollbar_window_extra_bytes,
                    (HICON)0, (HBRUSH)0x6, class_name);
  if (RegisterClassA(&wndclass) == 0)
  {
    registered = 0;
  }

  registered &= CreateOffscreen32bppDibSection(1000,
                                               GetSystemMetrics(SM_CYHSCROLL) * 5,
                                               SCROLLBAR_OFFSCREEN_DC_PTR,
                                               (BITMAPINFO *)0,
                                               SCROLLBAR_OFFSCREEN_BITMAP_PTR,
                                               (HGDIOBJ *)0,
                                               (void **)0);
  return registered;
}

// FUNCTION: MAGIC 0x004909b5
// FUNCTION: SHANDALAR 0x00556385
void destroy_MAGICGAME_ScrollbarClass(LPCSTR class_name)
{
  checked_DeleteDC_DeleteObject(SCROLLBAR_OFFSCREEN_DC, SCROLLBAR_OFFSCREEN_BITMAP);
  SCROLLBAR_OFFSCREEN_DC = (HDC)0;
  SCROLLBAR_OFFSCREEN_BITMAP = (HBITMAP)0;
}

// FUNCTION: MAGIC 0x00491be4
// FUNCTION: SHANDALAR 0x005575b3
int scrollbar_client_x_to_position(HWND hwnd, int client_x)
{
  int result;
  LONG min_position;
  RECT client_rect;
  int max_position;

  if (hwnd == (HWND)0)
  {
    return 0;
  }

  min_position = GetWindowLongA(hwnd, g_scrollbar_min_position_window_long_offset);
  max_position = GetWindowLongA(hwnd, g_scrollbar_max_position_window_long_offset);
  GetClientRect(hwnd, &client_rect);
  if (client_x < client_rect.left)
  {
    client_x = client_rect.left;
  }
  if (client_rect.right < client_x)
  {
    client_x = client_rect.right;
  }
  result = min_position + (((max_position - min_position) + 1) * client_x) / client_rect.right;
  return result;
}

// FUNCTION: MAGIC 0x00491c7a
// FUNCTION: SHANDALAR 0x00557649
int scrollbar_position_to_client_x(HWND hwnd, int position)
{
  LONG min_position;
  int result;
  RECT client_rect;
  int max_position;

  if (hwnd == (HWND)0)
  {
    return 0;
  }

  min_position = GetWindowLongA(hwnd, g_scrollbar_min_position_window_long_offset);
  max_position = GetWindowLongA(hwnd, g_scrollbar_max_position_window_long_offset);
  GetClientRect(hwnd, &client_rect);
  if (position < min_position)
  {
    position = min_position;
  }
  if (max_position < position)
  {
    position = max_position;
  }
  result = (client_rect.right * position) / ((max_position - min_position) + 1);
  return result;
}

// FUNCTION: MAGIC 0x00491d0d
// FUNCTION: SHANDALAR 0x005576dc
void get_scrollbar_thumb_rect(HWND hwnd, LPRECT rect)
{
  LONG frame_count;
  BITMAP bitmap;
  int height;
  int width;
  LONG position;
  HANDLE thumb_bitmap;
  RECT client_rect;

  if (hwnd != (HWND)0 && rect != (LPRECT)0)
  {
    position = GetWindowLongA(hwnd, g_scrollbar_thumb_position_window_long_offset);
    thumb_bitmap = (HANDLE)GetWindowLongPtrA(hwnd, g_scrollbar_thumb_bitmap_window_long_offset);
    frame_count = GetWindowLongA(hwnd, g_scrollbar_thumb_bitmap_frame_count_window_long_offset);
    GetClientRect(hwnd, &client_rect);
    if (thumb_bitmap != (HANDLE)0 && frame_count != 0)
    {
      GetObjectA(thumb_bitmap, 0x18, &bitmap);
      height = client_rect.bottom;
      width = ((bitmap.bmWidth / 2) * client_rect.bottom) / (bitmap.bmHeight / frame_count);
      SetRect(rect, position - width / 2, 0,
              (position - width / 2) + width, client_rect.bottom);
    }
    else
    {
      SetRect(rect, position - client_rect.right / 20, 0,
              position + client_rect.right / 20, client_rect.bottom);
    }
    if (rect->left < client_rect.left)
    {
      OffsetRect(rect, client_rect.left - rect->left, 0);
    }
    else if (client_rect.right < rect->right)
    {
      OffsetRect(rect, client_rect.right - rect->right, 0);
    }
  }
}

// FUNCTION: MAGIC 0x004909e8
// FUNCTION: SHANDALAR 0x005563b8
LRESULT CALLBACK wndproc_MAGICGAME_ScrollbarClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  int flipped_rect_edge;
  BITMAP thumb_bitmap_info;
  int thumb_source_height;
  int thumb_source_width;
  int thumb_mask_y;
  int thumb_frame;
  int thumb_source_x;
  int thumb_source_y;
  int thumb_mask_x;
  int track_source_height;
  int track_source_width;
  RECT bitmap_rect;
  HDC paint_dc;
  BITMAP bitmap;
  int client_height;
  PAINTSTRUCT paint;
  int scaled_width;
  RECT tile_rect;
  int tile_y;
  int saved_dc;
  int tile_x;
  RECT thumb_rect;
  RECT window_rect;
  RECT client_rect;
  int button_up_mouse_x;
  int button_up_mouse_y;
  RECT button_up_client_rect;
  int button_down_mouse_x;
  int button_down_mouse_y;
  RECT button_down_hit_rect;
  int clamped_x;
  int move_mouse_x;
  int move_mouse_y;
  RECT move_client_rect;
  int cue_mouse_x;
  int cue_mouse_y;
  RECT cue_hit_rect;
  int scroll_position;
  LONG thumb_frame_count;
  unsigned int min_position;
  unsigned int thumb_visible;
  HANDLE track_bitmap;
  int direction;
  LONG thumb_position;
  HANDLE thumb_bitmap;
  LONG max_position;
  LONG track_bitmap_layout;

  switch (msg)
  {
  case 0x437:
    thumb_position = GetWindowLongA(hwnd, g_scrollbar_thumb_position_window_long_offset);
    thumb_visible = GetWindowLongA(hwnd, g_scrollbar_thumb_visible_window_long_offset);
    cue_mouse_x = (int)(short)lparam;
    cue_mouse_y = (int)(short)((unsigned int)lparam >> 16);
    get_scrollbar_thumb_rect(hwnd, &cue_hit_rect);
    if (thumb_visible != 0 && cue_hit_rect.left <= cue_mouse_x && cue_mouse_x <= cue_hit_rect.right)
    {
      strcpy((char *)wparam, gs_cuecard_scroll_thumb_0093a810);
    }
    else
    {
      strcpy((char *)wparam, gs_cuecard_scrollbar_008b2900);
    }
    return 1;

  case 0x432:
    InvalidateRect(hwnd, (RECT *)0, 1);
    return 0;

  case SBM_SETRANGE:
    min_position = GetWindowLongA(hwnd, g_scrollbar_min_position_window_long_offset);
    max_position = GetWindowLongA(hwnd, g_scrollbar_max_position_window_long_offset);
    if (min_position != wparam || max_position != lparam)
    {
      min_position = wparam;
      max_position = lparam;
      SetWindowLongA(hwnd, g_scrollbar_min_position_window_long_offset, min_position);
      SetWindowLongA(hwnd, g_scrollbar_max_position_window_long_offset, max_position);
      InvalidateRect(hwnd, (RECT *)0, 1);
    }
    return 0;

  case SBM_GETRANGE:
    min_position = GetWindowLongA(hwnd, g_scrollbar_min_position_window_long_offset);
    max_position = GetWindowLongA(hwnd, g_scrollbar_max_position_window_long_offset);
    if (wparam != 0)
    {
      *(unsigned int *)wparam = min_position;
    }
    if (lparam != 0)
    {
      *(LONG *)lparam = max_position;
    }
    return (max_position << 16) | (min_position & 0xffff);

  case SBM_SETPOS:
    scroll_position = GetWindowLongA(hwnd, g_scrollbar_position_window_long_offset);
    if (scroll_position != wparam)
    {
      scroll_position = wparam;
      SetWindowLongA(hwnd, g_scrollbar_position_window_long_offset, scroll_position);
      InvalidateRect(hwnd, (RECT *)0, 1);
    }
    return 0;

  case SBM_GETPOS:
    scroll_position = GetWindowLongA(hwnd, g_scrollbar_position_window_long_offset);
    return scroll_position;

  case 0x464:
    track_bitmap = (HANDLE)GetWindowLongPtrA(hwnd, g_scrollbar_track_bitmap_window_long_offset);
    if (track_bitmap != (HANDLE)wparam)
    {
      track_bitmap = (HANDLE)wparam;
      track_bitmap_layout = lparam;
      SetWindowLongPtrA(hwnd, g_scrollbar_track_bitmap_window_long_offset, (LONG_PTR)track_bitmap);
      SetWindowLongA(hwnd, g_scrollbar_track_bitmap_layout_window_long_offset, track_bitmap_layout);
      InvalidateRect(hwnd, (RECT *)0, 1);
    }
    return 0;

  case 0x465:
    track_bitmap = (HANDLE)GetWindowLongPtrA(hwnd, g_scrollbar_track_bitmap_window_long_offset);
    if (wparam != 0)
    {
      *(HANDLE *)wparam = track_bitmap;
    }
    return (LRESULT)track_bitmap;

  case 0x466:
    thumb_bitmap = (HANDLE)GetWindowLongPtrA(hwnd, g_scrollbar_thumb_bitmap_window_long_offset);
    thumb_frame_count = GetWindowLongA(hwnd, g_scrollbar_thumb_bitmap_frame_count_window_long_offset);
    if ((HANDLE)wparam != thumb_bitmap)
    {
      thumb_bitmap = (HANDLE)wparam;
      thumb_frame_count = lparam;
      SetWindowLongPtrA(hwnd, g_scrollbar_thumb_bitmap_window_long_offset, (LONG_PTR)thumb_bitmap);
      SetWindowLongA(hwnd, g_scrollbar_thumb_bitmap_frame_count_window_long_offset, thumb_frame_count);
      InvalidateRect(hwnd, (RECT *)0, 1);
    }
    return 0;

  case 0x467:
    thumb_bitmap = (HANDLE)GetWindowLongPtrA(hwnd, g_scrollbar_thumb_bitmap_window_long_offset);
    thumb_frame_count = GetWindowLongA(hwnd, g_scrollbar_thumb_bitmap_frame_count_window_long_offset);
    if (wparam != 0)
    {
      *(HANDLE *)wparam = thumb_bitmap;
    }
    if (lparam != 0)
    {
      *(LONG *)lparam = thumb_frame_count;
    }
    return (LRESULT)thumb_bitmap;

  case 0x468:
    thumb_visible = GetWindowLongA(hwnd, g_scrollbar_thumb_visible_window_long_offset);
    if (wparam != thumb_visible)
    {
      thumb_visible = wparam;
      SetWindowLongA(hwnd, g_scrollbar_thumb_visible_window_long_offset, thumb_visible);
      InvalidateRect(hwnd, (RECT *)0, 1);
    }
    return 0;

  case WM_CREATE:
    scroll_position = 0;
    min_position = 0;
    max_position = 0;
    SetWindowLongA(hwnd, g_scrollbar_position_window_long_offset, scroll_position);
    SetWindowLongA(hwnd, g_scrollbar_min_position_window_long_offset, min_position);
    SetWindowLongA(hwnd, g_scrollbar_max_position_window_long_offset, max_position);
    direction = 1;
    SetWindowLongA(hwnd, g_scrollbar_direction_window_long_offset, direction);
    track_bitmap = (HANDLE)0;
    track_bitmap_layout = 0;
    SetWindowLongPtrA(hwnd, g_scrollbar_track_bitmap_window_long_offset, (LONG_PTR)track_bitmap);
    SetWindowLongA(hwnd, g_scrollbar_track_bitmap_layout_window_long_offset, track_bitmap_layout);
    thumb_bitmap = (HANDLE)0;
    thumb_frame_count = 0;
    SetWindowLongPtrA(hwnd, g_scrollbar_thumb_bitmap_window_long_offset, (LONG_PTR)thumb_bitmap);
    SetWindowLongA(hwnd, g_scrollbar_thumb_bitmap_frame_count_window_long_offset, thumb_frame_count);
    thumb_position = 0;
    SetWindowLongA(hwnd, g_scrollbar_thumb_position_window_long_offset, thumb_position);
    thumb_visible = 0;
    SetWindowLongA(hwnd, g_scrollbar_thumb_visible_window_long_offset, thumb_visible);
    return 0;

  case WM_ERASEBKGND:
    return 1;

  case WM_MOUSEMOVE:
    if (GetCapture() == hwnd)
    {
      direction = GetWindowLongA(hwnd, g_scrollbar_direction_window_long_offset);
      scroll_position = GetWindowLongA(hwnd, g_scrollbar_position_window_long_offset);
      min_position = GetWindowLongA(hwnd, g_scrollbar_min_position_window_long_offset);
      max_position = GetWindowLongA(hwnd, g_scrollbar_max_position_window_long_offset);
      thumb_position = GetWindowLongA(hwnd, g_scrollbar_position_window_long_offset);
      move_mouse_x = (int)(short)lparam;
      move_mouse_y = (int)(short)((unsigned int)lparam >> 16);
      GetClientRect(hwnd, &move_client_rect);
      if (move_mouse_x < 0)
      {
        move_mouse_x = 0;
      }
      if (move_client_rect.right < move_mouse_x)
      {
        move_mouse_x = move_client_rect.right;
      }
      if (move_mouse_y < 0)
      {
        move_mouse_y = 0;
      }
      if (move_client_rect.bottom < move_mouse_y)
      {
        move_mouse_y = move_client_rect.bottom;
      }
      if (SCROLLBAR_LAST_MOUSE_X < move_mouse_x)
      {
        if (direction == 0)
        {
          direction = 1;
          SetWindowLongA(hwnd, g_scrollbar_direction_window_long_offset, direction);
          InvalidateRect(hwnd, (RECT *)0, 1);
        }
      }
      else if (move_mouse_x < SCROLLBAR_LAST_MOUSE_X && direction != 0)
      {
        direction = 0;
        SetWindowLongA(hwnd, g_scrollbar_direction_window_long_offset, direction);
        InvalidateRect(hwnd, (RECT *)0, 1);
      }
      clamped_x = move_mouse_x;
      if (clamped_x < (int)min_position)
      {
        clamped_x = min_position;
      }
      else if (max_position < clamped_x)
      {
        clamped_x = max_position;
      }
      if (clamped_x != thumb_position)
      {
        thumb_position = clamped_x;
        SetWindowLongA(hwnd, g_scrollbar_thumb_position_window_long_offset, thumb_position);
        InvalidateRect(hwnd, (RECT *)0, 1);
      }
      if (scrollbar_client_x_to_position(hwnd, move_mouse_x) != scroll_position)
      {
        SendMessageA(GetParent(hwnd), WM_HSCROLL,
                     MAKELONG(5, scrollbar_client_x_to_position(hwnd, move_mouse_x)), (LPARAM)hwnd);
      }
      UpdateWindow(hwnd);
      SCROLLBAR_LAST_MOUSE_X = move_mouse_x;
      SCROLLBAR_LAST_MOUSE_Y = move_mouse_y;
    }
    return 0;

  case WM_LBUTTONDOWN:
    UpdateWindow(hwnd);
    thumb_position = GetWindowLongA(hwnd, g_scrollbar_thumb_position_window_long_offset);
    direction = GetWindowLongA(hwnd, g_scrollbar_direction_window_long_offset);
    thumb_visible = GetWindowLongA(hwnd, g_scrollbar_thumb_visible_window_long_offset);
    button_down_mouse_x = (int)(short)lparam;
    button_down_mouse_y = (int)(short)((unsigned int)lparam >> 16);
    if (thumb_visible == 0)
    {
      return 0;
    }
    get_scrollbar_thumb_rect(hwnd, &button_down_hit_rect);
    if (button_down_hit_rect.left <= button_down_mouse_x &&
        button_down_mouse_x <= button_down_hit_rect.right)
    {
      SetCapture(hwnd);
      SCROLLBAR_LAST_MOUSE_X = button_down_mouse_x;
      SCROLLBAR_LAST_MOUSE_Y = button_down_mouse_y;
    }
    else if (button_down_hit_rect.right < button_down_mouse_x)
    {
      if (direction == 0)
      {
        direction = 1;
        SetWindowLongA(hwnd, g_scrollbar_direction_window_long_offset, direction);
        InvalidateRect(hwnd, (RECT *)0, 1);
      }
      SendMessageA(GetParent(hwnd), WM_HSCROLL, 1, (LPARAM)hwnd);
    }
    else if (button_down_mouse_x < button_down_hit_rect.left)
    {
      if (direction != 0)
      {
        direction = 0;
        SetWindowLongA(hwnd, g_scrollbar_direction_window_long_offset, direction);
        InvalidateRect(hwnd, (RECT *)0, 1);
      }
      SendMessageA(GetParent(hwnd), WM_HSCROLL, 0, (LPARAM)hwnd);
    }
    SCROLLBAR_LAST_MOUSE_X = button_down_mouse_x;
    SCROLLBAR_LAST_MOUSE_Y = button_down_mouse_y;
    return 0;

  case WM_LBUTTONUP:
    if (GetCapture() == hwnd)
    {
      ReleaseCapture();
      thumb_position = GetWindowLongA(hwnd, g_scrollbar_thumb_position_window_long_offset);
      button_up_mouse_x = (int)(short)lparam;
      button_up_mouse_y = (int)(short)((unsigned int)lparam >> 16);
      GetClientRect(hwnd, &button_up_client_rect);
      if (button_up_mouse_x < 0)
      {
        button_up_mouse_x = 0;
      }
      if (button_up_client_rect.right < button_up_mouse_x)
      {
        button_up_mouse_x = button_up_client_rect.right;
      }
      if (button_up_mouse_y < 0)
      {
        button_up_mouse_y = 0;
      }
      if (button_up_client_rect.bottom < button_up_mouse_y)
      {
        button_up_mouse_y = button_up_client_rect.bottom;
      }
      thumb_position = button_up_mouse_x;
      SetWindowLongA(hwnd, g_scrollbar_thumb_position_window_long_offset, thumb_position);
      InvalidateRect(hwnd, (RECT *)0, 1);
      SendMessageA(GetParent(hwnd), WM_HSCROLL,
                   MAKELONG(4, scrollbar_client_x_to_position(hwnd, button_up_mouse_x)), (LPARAM)hwnd);
    }
    return 0;

  case WM_PAINT:
    scroll_position = GetWindowLongA(hwnd, g_scrollbar_position_window_long_offset);
    thumb_position = GetWindowLongA(hwnd, g_scrollbar_thumb_position_window_long_offset);
    direction = GetWindowLongA(hwnd, g_scrollbar_direction_window_long_offset);
    track_bitmap = (HANDLE)GetWindowLongPtrA(hwnd, g_scrollbar_track_bitmap_window_long_offset);
    track_bitmap_layout = GetWindowLongA(hwnd, g_scrollbar_track_bitmap_layout_window_long_offset);
    thumb_bitmap = (HANDLE)GetWindowLongPtrA(hwnd, g_scrollbar_thumb_bitmap_window_long_offset);
    thumb_frame_count = GetWindowLongA(hwnd, g_scrollbar_thumb_bitmap_frame_count_window_long_offset);
    thumb_visible = GetWindowLongA(hwnd, g_scrollbar_thumb_visible_window_long_offset);
    min_position = GetWindowLongA(hwnd, g_scrollbar_min_position_window_long_offset);
    max_position = GetWindowLongA(hwnd, g_scrollbar_max_position_window_long_offset);
    paint_dc = BeginPaint(hwnd, &paint);
    if (paint_dc != (HDC)0)
    {
      ApplyCardArtPaletteToDc(paint_dc);
      GetClientRect(hwnd, &client_rect);
      saved_dc = SaveDC(SCROLLBAR_OFFSCREEN_DC);
      IntersectClipRect(SCROLLBAR_OFFSCREEN_DC, 0, 0, client_rect.right, client_rect.bottom);
      GetWindowRect(hwnd, &window_rect);
      MapWindowPoints((HWND)0, GetParent(hwnd), (LPPOINT)&window_rect, 2);
      OffsetViewportOrgEx(SCROLLBAR_OFFSCREEN_DC, -window_rect.left, -window_rect.top, (LPPOINT)0);
      SendMessageA(GetParent(hwnd), WM_ERASEBKGND, (WPARAM)SCROLLBAR_OFFSCREEN_DC, 0);
      OffsetViewportOrgEx(SCROLLBAR_OFFSCREEN_DC, window_rect.left, window_rect.top, (LPPOINT)0);
      if (track_bitmap != (HANDLE)0)
      {
        CopyRect(&bitmap_rect, &client_rect);
        GetObjectA(track_bitmap, 0x18, &bitmap);
        bitmap_rect.left = 0 - (scroll_position % bitmap.bmWidth);
        client_height = client_rect.bottom;
        if (track_bitmap_layout != 0)
        {
          scaled_width = (bitmap.bmWidth * client_height) / (bitmap.bmHeight / 2);
          track_source_width = bitmap.bmWidth;
          track_source_height = bitmap.bmHeight / 2;
        }
        else
        {
          scaled_width = ((bitmap.bmWidth / 2) * client_height) / bitmap.bmHeight;
          track_source_width = bitmap.bmWidth / 2;
          track_source_height = bitmap.bmHeight;
        }
        for (tile_x = bitmap_rect.left;
             tile_x < bitmap_rect.right;
             tile_x += track_source_width)
        {
          for (tile_y = bitmap_rect.top;
               tile_y < bitmap_rect.bottom;
               tile_y += track_source_height)
          {
            SetRect(&tile_rect, tile_x, tile_y,
                    scaled_width + tile_x, tile_y + client_height);
            if (track_bitmap_layout != 0)
            {
              DrawMaskedBitmapToRect(SCROLLBAR_OFFSCREEN_DC, &tile_rect, track_bitmap,
                                     bitmap.bmWidth, bitmap.bmHeight / 2,
                                     0, 0, 0, bitmap.bmHeight / 2);
            }
            else
            {
              draw_masked_bitmap_left_half_to_rect(SCROLLBAR_OFFSCREEN_DC, &tile_rect, track_bitmap);
            }
          }
        }
      }
      else
      {
        FillRect(SCROLLBAR_OFFSCREEN_DC, &client_rect, GetStockObject(GRAY_BRUSH));
      }
      if (thumb_visible != 0)
      {
        thumb_frame = thumb_position % thumb_frame_count;
        if (scrollbar_client_x_to_position(hwnd, thumb_position) != scroll_position)
        {
          thumb_position = scrollbar_position_to_client_x(hwnd, scroll_position);
          SetWindowLongA(hwnd, g_scrollbar_thumb_position_window_long_offset, thumb_position);
        }
        if (thumb_bitmap != (HANDLE)0 && thumb_frame_count != 0)
        {
          GetObjectA(thumb_bitmap, 0x18, &thumb_bitmap_info);
          thumb_source_width = thumb_bitmap_info.bmWidth / 2;
          thumb_source_height = thumb_bitmap_info.bmHeight / thumb_frame_count;
          thumb_source_x = 0;
          thumb_source_y = thumb_source_height * thumb_frame;
          thumb_mask_x = thumb_source_width;
          thumb_mask_y = thumb_source_y;
          get_scrollbar_thumb_rect(hwnd, &thumb_rect);
          if (direction == 0)
          {
            SetMapMode(SCROLLBAR_OFFSCREEN_DC, MM_ANISOTROPIC);
            SetWindowExtEx(SCROLLBAR_OFFSCREEN_DC, 1, 1, (LPSIZE)0);
            SetViewportExtEx(SCROLLBAR_OFFSCREEN_DC, -1, 1, (LPSIZE)0);
            SetWindowOrgEx(SCROLLBAR_OFFSCREEN_DC, 0, 0, (LPPOINT)0);
            SetViewportOrgEx(SCROLLBAR_OFFSCREEN_DC, client_rect.right, 0, (LPPOINT)0);
            thumb_rect.left = client_rect.right - thumb_rect.left;
            thumb_rect.right = client_rect.right - thumb_rect.right;
            flipped_rect_edge = thumb_rect.left;
            thumb_rect.left = thumb_rect.right;
            thumb_rect.right = flipped_rect_edge;
          }
          DrawMaskedBitmapToRect(SCROLLBAR_OFFSCREEN_DC, &thumb_rect, thumb_bitmap,
                                 thumb_source_width, thumb_source_height,
                                 thumb_source_x, thumb_source_y, thumb_mask_x, thumb_mask_y);
          if (direction == 0)
          {
            SetMapMode(SCROLLBAR_OFFSCREEN_DC, MM_TEXT);
            SetWindowOrgEx(SCROLLBAR_OFFSCREEN_DC, 0, 0, (LPPOINT)0);
            SetViewportOrgEx(SCROLLBAR_OFFSCREEN_DC, 0, 0, (LPPOINT)0);
          }
        }
        else
        {
          SetRect(&thumb_rect,
                  thumb_position - client_rect.right / 20,
                  0,
                  thumb_position + client_rect.right / 20,
                  client_rect.bottom);
          FillRect(SCROLLBAR_OFFSCREEN_DC, &thumb_rect, GetStockObject(BLACK_BRUSH));
        }
      }
      RestoreDC(SCROLLBAR_OFFSCREEN_DC, saved_dc);
      BitBlt(paint_dc, 0, 0, client_rect.right, client_rect.bottom,
             SCROLLBAR_OFFSCREEN_DC, 0, 0, SRCCOPY);
      EndPaint(hwnd, &paint);
    }
    return 0;

  case WM_QUERYNEWPALETTE:
  case WM_PALETTEISCHANGING:
  case WM_PALETTECHANGED:
    return handle_button_palette_message(hwnd, msg, (HWND)wparam, lparam);

  default:
    return DefWindowProcA(hwnd, msg, wparam, lparam);
  }
}
