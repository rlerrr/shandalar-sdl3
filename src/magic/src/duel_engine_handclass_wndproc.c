#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cardartlib/src/palette.h"
#include "magic/src/duel_engine.h"
#include "magic/src/game_support.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "magic/src/shared_startup.h"

extern int g_showlist_smallcard_width;
extern int g_showlist_smallcard_height;
extern int g_duel_selected_card_window;
extern char global_base_directory[];

LOGFONTA *LoadFontFromIni(char *section, int must_load);
int load_text_with_tab_escapes(char *filename, char *section_name);
void delete_and_close_object(HANDLE obj);
card_id_t get_displayed_card_id(int player, int card);
int card_window_matches_player_and_card(HWND hwnd, int *player_and_card);
int card_window_matches_card_id(HWND hwnd, card_id_t card_id);
card_id_t get_card_window_displayed_card_id(HWND hwnd);

// GLOBAL: MAGIC 0x0055e0d8
// GLOBAL: SHANDALAR 0x0057f068
int g_hand_card_windows_long_offset = 0;

// GLOBAL: MAGIC 0x0055e0dc
// GLOBAL: SHANDALAR 0x0057f06c
int g_hand_card_count_long_offset = 4;

// GLOBAL: MAGIC 0x0055e0e0
// GLOBAL: SHANDALAR 0x0057f070
int g_hand_art_width_long_offset = 8;

// GLOBAL: MAGIC 0x0055e0e4
// GLOBAL: SHANDALAR 0x0057f074
int g_hand_art_title_height_long_offset = 0xc;

// GLOBAL: MAGIC 0x0055e0e8
// GLOBAL: SHANDALAR 0x0057f078
int g_hand_art_side_width_long_offset = 0x10;

// GLOBAL: MAGIC 0x0055e0ec
// GLOBAL: SHANDALAR 0x0057f07c
int g_hand_art_bottom_height_long_offset = 0x14;

// GLOBAL: MAGIC 0x0055e0f0
// GLOBAL: SHANDALAR 0x0057f080
int g_hand_background_bitmap_long_offset = 0x18;

// GLOBAL: MAGIC 0x0055e0f4
// GLOBAL: SHANDALAR 0x0057f084
int g_hand_selected_index_long_offset = 0x1c;

// GLOBAL: MAGIC 0x0055e0f8
// GLOBAL: SHANDALAR 0x0057f088
int g_hand_window_extra_bytes = 0x20;

// GLOBAL: MAGIC 0x0064f8f8
// GLOBAL: SHANDALAR 0x005a9d58
COLORREF g_hand_title_text_color;

// GLOBAL: MAGIC 0x0064f8fc
// GLOBAL: SHANDALAR 0x005a9d5c
COLORREF g_hand_title_shadow_color;

// GLOBAL: MAGIC 0x0064f900
// GLOBAL: SHANDALAR 0x005a9d60
char g_hand_menu_help_text[0x1c];

// GLOBAL: MAGIC 0x0064f91c
// GLOBAL: SHANDALAR 0x005a9d7c
HMENU g_hand_popup_menu;

// GLOBAL: MAGIC 0x0064f920
// GLOBAL: SHANDALAR 0x005a9d80
HFONT g_hand_title_font;

// FUNCTION: MAGIC 0x004bbae0
// FUNCTION: SHANDALAR 0x0046ee20
int register_MAGICGAME_HandClass(LPCSTR class_name)
{
  int registered;
  WNDCLASSA wndclass;

  registered = 1;
  SET_DUEL_WNDCLASS(wndclass, 3, wndproc_MAGICGAME_HandClass, g_hand_window_extra_bytes,
                    (HICON)0, (HBRUSH)0x6, class_name);
  if (RegisterClassA(&wndclass) == 0)
  {
    registered = 0;
  }
  g_hand_popup_menu = CreatePopupMenu();
  load_text_with_tab_escapes(global_ui_strings_filename, "MENU_HAND");
  strcpy(g_hand_menu_help_text, g_text_lines[0]);
  g_hand_title_font = CreateFontIndirectA(LoadFontFromIni("Hand", 0));
  g_hand_title_text_color = 0x10000bf;
  g_hand_title_shadow_color = 0x10000c9;
  return registered;
}

// FUNCTION: MAGIC 0x004bbbc7
// FUNCTION: SHANDALAR 0x0046ef07
void destroy_MAGICGAME_HandClass(LPCSTR class_name)
{
  if (g_hand_popup_menu != (HMENU)0)
  {
    DestroyMenu(g_hand_popup_menu);
  }
  g_hand_popup_menu = (HMENU)0;
  if (g_hand_title_font != (HFONT)0)
  {
    DeleteObject(g_hand_title_font);
  }
  g_hand_title_font = (HFONT)0;
}

// FUNCTION: MAGIC 0x004bd6dd
// FUNCTION: SHANDALAR 0x00470a12
void calculate_hand_art_offsets(int card_width, int card_height, int art_width,
                                int title_art_height, int side_art_width,
                                int bottom_art_height, int *title_height,
                                int *bottom_margin, int *side_margin,
                                int *edge_width)
{
  (void)card_width;
  if (art_width != 0)
  {
    *title_height = (card_height * 0x1f) / 100;
    *bottom_margin = (*title_height * title_art_height) / art_width;
    *side_margin = (*title_height * side_art_width) / art_width;
    *edge_width = (*title_height * bottom_art_height) / art_width;
  }
  else
  {
    *title_height = 0;
    *bottom_margin = 0;
    *side_margin = 0;
    *edge_width = 0;
  }
}

// FUNCTION: MAGIC 0x004bd468
// FUNCTION: SHANDALAR 0x0047079d
void draw_hand_window_frame(HDC dc, int *outer_rect, int *inner_rect,
                            int edge_width, int art_width, int title_art_height,
                            int side_art_width, int bottom_art_height,
                            HBITMAP bitmap)
{
  BITMAP bitmap_info;
  int y;
  RECT rect;
  int y_step;

  if (dc == (HDC)0 || outer_rect == NULL || inner_rect == NULL || bitmap == (HBITMAP)0)
  {
    return;
  }

  GetObjectA(bitmap, sizeof(BITMAP), &bitmap_info);
  y_step = ((inner_rect[1] - outer_rect[1]) * bitmap_info.bmHeight) / art_width;
  for (y = inner_rect[1]; y < inner_rect[3]; y += y_step)
  {
    SetRect(&rect, inner_rect[0], y, inner_rect[0] + edge_width, y + y_step);
    DrawBitmapSubrectToRect(dc, &rect, bitmap, bitmap_info.bmWidth - bottom_art_height, 0,
                            bottom_art_height, bitmap_info.bmHeight);
    SetRect(&rect, inner_rect[2] - edge_width, y, inner_rect[2], y + y_step);
    DrawBitmapSubrectToRect(dc, &rect, bitmap, bitmap_info.bmWidth - bottom_art_height, 0,
                            bottom_art_height, bitmap_info.bmHeight);
  }

  SetRect(&rect, inner_rect[0], outer_rect[1], inner_rect[2], inner_rect[1]);
  DrawBitmapSubrectToRect(dc, &rect, bitmap, 0, 0,
                          (bitmap_info.bmWidth - side_art_width) - bottom_art_height,
                          art_width);
  SetRect(&rect, inner_rect[0], inner_rect[3], inner_rect[2], outer_rect[3]);
  DrawBitmapSubrectToRect(dc, &rect, bitmap, 0, bitmap_info.bmHeight - title_art_height,
                          (bitmap_info.bmWidth - side_art_width) - bottom_art_height,
                          title_art_height);

  for (y = outer_rect[1]; y < outer_rect[3]; y += y_step)
  {
    SetRect(&rect, outer_rect[0], y, inner_rect[0], y + y_step);
    DrawBitmapSubrectToRect(dc, &rect, bitmap,
                            (bitmap_info.bmWidth - side_art_width) - bottom_art_height,
                            0, side_art_width, bitmap_info.bmHeight);
    SetRect(&rect, inner_rect[2], y, outer_rect[2], y + y_step);
    DrawBitmapSubrectToRect(dc, &rect, bitmap,
                            (bitmap_info.bmWidth - side_art_width) - bottom_art_height,
                            0, side_art_width, bitmap_info.bmHeight);
  }
}

// FUNCTION: MAGIC 0x004bd768
// FUNCTION: SHANDALAR 0x00470a9d
void update_hand_window_title(char *title, HWND hwnd, int card_count)
{
  sprintf(title, "%s (%d)",
          hwnd == g_duel_full_card_window_hwnd ? gs_window_title_your_hand_00777bf0 : gs_window_title_opponent_0091c820,
          card_count);
  SetWindowTextA(hwnd, title);
  InvalidateRect(hwnd, NULL, TRUE);
}

static void resize_hand_window_preserving_right_edge(HWND hwnd, int width, int height)
{
  RECT rect;

  if (GetWindowRect(hwnd, &rect) && rect.right > rect.left)
  {
    SetWindowPos(hwnd, (HWND)0, rect.right - width, rect.top, width, height,
                 SWP_NOZORDER);
  }
  else
  {
    SetWindowPos(hwnd, (HWND)0, 0, 0, width, height,
                 SWP_NOMOVE | SWP_NOZORDER);
  }
}

// FUNCTION: MAGIC 0x004bd131
// FUNCTION: SHANDALAR 0x00470468
void resize_duel_hand_window(HWND hwnd)
{
  int side_margin;
  int edge_width;
  int loop_index;
  int art_bottom_height;
  int card_count;
  int art_width;
  int art_title_height;
  int art_side_width;
  int window_width;
  int card_index;
  int card_y;
  int card_x;
  HWND previous_card;
  HWND *card_windows;
  int visible_count;
  int max_visible_cards;
  int selected_index;
  int title_height;
  HBITMAP background_bitmap;
  int window_height;
  int bottom_margin;
  int card_spacing;
  int card_column_step;
  int card_slot;
  int visible_row_count;

  max_visible_cards = 8;
  card_count = GetWindowLongA(hwnd, g_hand_card_count_long_offset);
  card_windows = (HWND *)GetWindowLongPtrA(hwnd, g_hand_card_windows_long_offset);
  art_width = GetWindowLongA(hwnd, g_hand_art_width_long_offset);
  art_title_height = GetWindowLongA(hwnd, g_hand_art_title_height_long_offset);
  art_side_width = GetWindowLongA(hwnd, g_hand_art_side_width_long_offset);
  art_bottom_height = GetWindowLongA(hwnd, g_hand_art_bottom_height_long_offset);
  background_bitmap = (HBITMAP)GetWindowLongPtrA(hwnd, g_hand_background_bitmap_long_offset);
  selected_index = GetWindowLongA(hwnd, g_hand_selected_index_long_offset);
  calculate_hand_art_offsets(g_showlist_smallcard_width, g_showlist_smallcard_height,
                             art_width, art_title_height, art_side_width, art_bottom_height,
                             &title_height, &bottom_margin, &side_margin, &edge_width);
  card_spacing = g_duel_selected_card_window * 3;
  card_column_step = g_showlist_smallcard_width;

  if (g_duel_full_card_window_hwnd == hwnd ||
      (g_duel_life_window_hwnd == hwnd && g_duel_surface_reset_state != 0))
  {
    visible_count = max_visible_cards < card_count ? max_visible_cards : card_count;
    visible_row_count = (visible_count + 1) / 2;
    window_width = edge_width + edge_width + side_margin + side_margin +
                   g_showlist_smallcard_width +
                   (visible_count > 1 ? card_column_step : 0);
    if (card_count == 0)
    {
      window_height = bottom_margin + title_height;
    }
    else
    {
      window_height = (visible_row_count - 1) * card_spacing + bottom_margin +
                      title_height + g_showlist_smallcard_height;
    }

    if (card_count > 0)
    {
      card_index = selected_index;
      previous_card = (HWND)0;
      for (loop_index = 0; loop_index < visible_count; loop_index++)
      {
        card_slot = loop_index;
        if ((visible_count & 1) != 0 && loop_index > 0)
        {
          card_slot++;
        }
        card_x = edge_width + side_margin +
                 (card_slot % 2) * card_column_step;
        card_y = title_height +
                 (visible_row_count - 1 - card_slot / 2) * card_spacing;
        MoveWindow(card_windows[card_index], card_x, card_y,
                   g_showlist_smallcard_width, g_showlist_smallcard_height, TRUE);
        if (previous_card == (HWND)0)
        {
          BringWindowToTop(card_windows[card_index]);
        }
        else
        {
          SetWindowPos(card_windows[card_index], previous_card, 0, 0, 0, 0,
                       SWP_NOMOVE | SWP_NOSIZE);
        }
        previous_card = card_windows[card_index];
        if (card_index == 0)
        {
          card_index = card_count - 1;
        }
        else
        {
          card_index--;
        }
      }
      for (loop_index = visible_count; card_count > loop_index; loop_index++)
      {
        MoveWindow(card_windows[card_index], -1, -1, 0, 0, TRUE);
        if (card_index == 0)
        {
          card_index = card_count - 1;
        }
        else
        {
          card_index--;
        }
      }
    }

    UpdateWindow(hwnd);
    resize_hand_window_preserving_right_edge(hwnd, window_width, window_height);
  }
  else
  {
    window_width = edge_width + edge_width + side_margin + side_margin +
                   g_showlist_smallcard_width;
    window_height = bottom_margin + title_height;
    for (card_index = 0; card_count > card_index; card_index++)
    {
      MoveWindow(card_windows[card_index], -1, -1, 0, 0, TRUE);
    }
    resize_hand_window_preserving_right_edge(hwnd, window_width, window_height);
  }
}

// FUNCTION: MAGIC 0x004bbc18
// FUNCTION: SHANDALAR 0x0046ef58
LRESULT CALLBACK wndproc_MAGICGAME_HandClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  int menu_item_count;
  POINT menu_point;
  RECT menu_rect;
  int side_margin;
  int edge_width;
  char window_title[100];
  RECT client_rect;
  HDC paint_dc;
  PAINTSTRUCT paint_struct;
  int saved_dc;
  RECT title_rect;
  int title_height;
  RECT blit_rect;
  int bottom_margin;
  RECT inner_rect;
  int hit_side_margin;   // ebp - 0x278
  int hit_edge_width;    // ebp - 0x274
  unsigned int click_x;  // ebp - 0x270
  unsigned int click_y;  // ebp - 0x26c
  int hit_title_height;  // ebp - 0x268
  int hit_bottom_margin; // ebp - 0x264
  int target_index;
  RECT before_move_rect;
  RECT after_move_rect;
  char help_path[264];
  DWORD help_context;
  int find_index;
  int *find_player_card;
  LRESULT find_result;
  int find_found;
  int *bring_player_card;
  int bring_shift_index;
  int bring_index;
  HWND bring_hwnd;
  int bring_found;
  char clear_title[52];
  int clear_index;
  char remove_title[52];
  int *remove_player_card;
  int remove_shift_index;
  int remove_index;
  int remove_found;
  char add_title[52];
  int add_shift_index;
  HWND existing_hwnd;
  int *add_player_card;
  int add_index;      // ebp - 0x50
  HWND selected_hwnd; // ebp - 0x4c
  HBITMAP new_bitmap; // ebp - 0x48
  RECT *bitmap_rect;
  RECT *invalidate_rect;
  int *invalidate_player_card; // ebp - 0x3c
  int invalidate_index;        // ebp - 0x38
  int invalidate_found;        // ebp - 0x34
  int invalidate_all_index;    // ebp - 0x30
  int refresh_index;           // ebp - 0x2c
  card_id_t card_id;           // ebp - 0x28
  int card_id_index;           // ebp - 0x24
  LONG art_bottom_height;      // ebp - 0x20
  int card_count;              // ebp - 0x1c
  LONG art_width;              // ebp - 0x18
  LONG art_title_height;       // ebp - 0x14
  LONG art_side_width;         // ebp - 0x10
  HWND *card_windows;          // ebp - 0xc
  int selected_index;          // ebp - 0x8
  HBITMAP background_bitmap;   // ebp - 0x4

  switch (msg)
  {
  case 0x433:
  case 0x434:
    card_windows = (HWND *)GetWindowLongPtrA(hwnd, g_hand_card_windows_long_offset);
    card_count = GetWindowLongA(hwnd, g_hand_card_count_long_offset);
    card_id = (int)wparam;
    for (card_id_index = 0; card_id_index < card_count; card_id_index++)
    {
      if (card_window_matches_card_id(card_windows[card_id_index], card_id) != 0)
      {
        InvalidateRect(card_windows[card_id_index], NULL, FALSE);
      }
    }
    return 0;

  case 0x432:
    card_windows = (HWND *)GetWindowLongPtrA(hwnd, g_hand_card_windows_long_offset);
    card_count = GetWindowLongA(hwnd, g_hand_card_count_long_offset);
    for (refresh_index = 0; refresh_index < card_count; refresh_index++)
    {
      SendMessageA(card_windows[refresh_index], 0x432, 0, 0);
    }
    return 0;

  case 0x435:
    card_windows = (HWND *)GetWindowLongPtrA(hwnd, g_hand_card_windows_long_offset);
    card_count = GetWindowLongA(hwnd, g_hand_card_count_long_offset);
    for (invalidate_all_index = 0; invalidate_all_index < card_count; invalidate_all_index++)
    {
      InvalidateRect(card_windows[invalidate_all_index], NULL, FALSE);
    }
    return 0;

  case 0x436:
    card_windows = (HWND *)GetWindowLongPtrA(hwnd, g_hand_card_windows_long_offset);
    card_count = GetWindowLongA(hwnd, g_hand_card_count_long_offset);
    invalidate_player_card = (int *)wparam;
    invalidate_rect = (RECT *)lparam;
    if (invalidate_player_card == 0)
    {
      return 0;
    }
    invalidate_found = 0;
    invalidate_index = 0;
    for (; invalidate_index < card_count && invalidate_found == 0;
         invalidate_index++)
    {
      if (card_window_matches_player_and_card(card_windows[invalidate_index], invalidate_player_card) != 0)
      {
        invalidate_found = 1;
        if (invalidate_rect != NULL)
        {
          SendMessageA(card_windows[invalidate_index], 0x432, 0, 0);
        }
        else
        {
          InvalidateRect(card_windows[invalidate_index], NULL, FALSE);
        }
      }
    }
    return 0;

  case 0x439:
    new_bitmap = (HBITMAP)wparam;
    bitmap_rect = (RECT *)lparam;
    background_bitmap = (HBITMAP)GetWindowLongPtrA(hwnd, g_hand_background_bitmap_long_offset);
    if (background_bitmap != (HBITMAP)0)
    {
      DeleteObject(background_bitmap);
    }
    background_bitmap = new_bitmap;
    art_width = bitmap_rect->left;
    art_title_height = bitmap_rect->top;
    art_side_width = bitmap_rect->right;
    art_bottom_height = bitmap_rect->bottom;
    SetWindowLongA(hwnd, g_hand_art_width_long_offset, art_width);
    SetWindowLongA(hwnd, g_hand_art_title_height_long_offset, art_title_height);
    SetWindowLongA(hwnd, g_hand_art_side_width_long_offset, art_side_width);
    SetWindowLongA(hwnd, g_hand_art_bottom_height_long_offset, art_bottom_height);
    SetWindowLongPtrA(hwnd, g_hand_background_bitmap_long_offset, (LONG_PTR)background_bitmap);
    resize_duel_hand_window(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
    return 0;

  case 0x438:
    return (LRESULT)(background_bitmap = (HBITMAP)GetWindowLongPtrA(hwnd, g_hand_background_bitmap_long_offset));

  case 0x400:
    card_windows = (HWND *)GetWindowLongPtrA(hwnd, g_hand_card_windows_long_offset);
    card_count = GetWindowLongA(hwnd, g_hand_card_count_long_offset);
    selected_hwnd = (HWND)wparam;
    if (selected_hwnd == 0)
    {
      return 0;
    }
    for (add_index = 0; add_index < card_count; add_index++)
    {
      if (card_windows[add_index] == selected_hwnd)
      {
        selected_index = add_index;
        SetWindowLongA(hwnd, g_hand_selected_index_long_offset, selected_index);
        resize_duel_hand_window(hwnd);
      }
    }
    return 0;

  case 0x40a:
    card_windows = (HWND *)GetWindowLongPtrA(hwnd, g_hand_card_windows_long_offset);
    card_count = GetWindowLongA(hwnd, g_hand_card_count_long_offset);
    selected_index = GetWindowLongA(hwnd, g_hand_selected_index_long_offset);
    if (card_count >= 0x32)
    {
      return 0;
    }
    add_player_card = (int *)wparam;
    existing_hwnd = (HWND)SendMessageA(hwnd, 0x40f, (WPARAM)add_player_card, 0);
    if (existing_hwnd == (HWND)0)
    {
      existing_hwnd = CreateWindowExA(0, "MAGICGAME_CardClass", "Hand Card",
                                      WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, 0, 0, 0, 0, hwnd, (HMENU)1,
                                      g_app_instance, (LPVOID)add_player_card);
      if (existing_hwnd == (HWND)0)
      {
        return 0;
      }
      card_windows[card_count] = existing_hwnd;
      card_count++;
      SetWindowLongA(hwnd, g_hand_card_count_long_offset, card_count);
      if (card_count - 1 == selected_index + 1)
      {
        selected_index = card_count - 1;
        SetWindowLongA(hwnd, g_hand_selected_index_long_offset, selected_index);
      }
      else
      {
        for (add_shift_index = card_count - 1; selected_index + 1 < add_shift_index; add_shift_index--)
        {
          card_windows[add_shift_index] = card_windows[add_shift_index - 1];
        }
        card_windows[selected_index + 1] = existing_hwnd;
        selected_index++;
        SetWindowLongA(hwnd, g_hand_selected_index_long_offset, selected_index);
      }
      resize_duel_hand_window(hwnd);
      update_hand_window_title(add_title, hwnd, card_count);
    }
    else if (card_window_matches_card_id(existing_hwnd,
                                         get_displayed_card_id(add_player_card[0], add_player_card[1])) == 0)
    {
      SendMessageA(hwnd, 0x40b, (WPARAM)add_player_card, 0);
      SendMessageA(hwnd, 0x40a, (WPARAM)add_player_card, 0);
    }
    return card_count;

  case 0x40b:
    card_windows = (HWND *)GetWindowLongPtrA(hwnd, g_hand_card_windows_long_offset);
    card_count = GetWindowLongA(hwnd, g_hand_card_count_long_offset);
    selected_index = GetWindowLongA(hwnd, g_hand_selected_index_long_offset);
    remove_player_card = (int *)wparam;
    remove_found = 0;
    remove_index = 0;
    for (; remove_index < card_count && remove_found == 0;
         remove_index++)
    {
      if (card_window_matches_player_and_card(card_windows[remove_index], remove_player_card) != 0)
      {
        remove_found = 1;
        DestroyWindow(card_windows[remove_index]);
        card_count--;
        SetWindowLongA(hwnd, g_hand_card_count_long_offset, card_count);
        for (remove_shift_index = remove_index; remove_shift_index < card_count; remove_shift_index++)
        {
          card_windows[remove_shift_index] = card_windows[remove_shift_index + 1];
        }
        if (selected_index == remove_index)
        {
          if (selected_index == 0)
          {
            selected_index = card_count - 1;
          }
          else
          {
            selected_index--;
          }
          SetWindowLongA(hwnd, g_hand_selected_index_long_offset, selected_index);
        }
        else
        {
          if (selected_index == 0)
          {
            selected_index = card_count - 1;
          }
          else
          {
            selected_index--;
          }
          SetWindowLongA(hwnd, g_hand_selected_index_long_offset, selected_index);
        }
        update_hand_window_title(remove_title, hwnd, card_count);
        UpdateWindow(hwnd);
      }
    }
    if (remove_found != 0)
    {
      resize_duel_hand_window(hwnd);
    }
    return remove_found;

  case 0x40c:
    card_windows = (HWND *)GetWindowLongPtrA(hwnd, g_hand_card_windows_long_offset);
    card_count = GetWindowLongA(hwnd, g_hand_card_count_long_offset);
    for (clear_index = 0; clear_index < card_count; clear_index++)
    {
      DestroyWindow(card_windows[clear_index]);
    }
    card_count = 0;
    SetWindowLongA(hwnd, g_hand_card_count_long_offset, card_count);
    selected_index = -1;
    SetWindowLongA(hwnd, g_hand_selected_index_long_offset, selected_index);
    resize_duel_hand_window(hwnd);
    update_hand_window_title(clear_title, hwnd, card_count);
    return 0;

  case 0x40d:
    card_windows = (HWND *)GetWindowLongPtrA(hwnd, g_hand_card_windows_long_offset);
    card_count = GetWindowLongA(hwnd, g_hand_card_count_long_offset);
    bring_player_card = (int *)wparam;
    bring_found = 0;
    bring_index = 0;
    for (; card_count > bring_index && bring_found == 0; bring_index++)
    {
      if (card_window_matches_player_and_card(card_windows[bring_index], bring_player_card) != 0)
      {
        bring_found = 1;
        bring_hwnd = card_windows[bring_index];
        BringWindowToTop(bring_hwnd);
        for (bring_shift_index = bring_index; bring_shift_index < card_count - 1; bring_shift_index++)
        {
          card_windows[bring_shift_index] = card_windows[bring_shift_index + 1];
        }
        card_windows[card_count - 1] = bring_hwnd;
      }
    }
    return bring_found;

  case 0x40e:
  case 0x40f:
    card_windows = (HWND *)GetWindowLongPtrA(hwnd, g_hand_card_windows_long_offset);
    card_count = GetWindowLongA(hwnd, g_hand_card_count_long_offset);
    find_player_card = (int *)wparam;
    find_found = 0;
    find_index = 0;
    for (; find_index < card_count && find_found == 0;
         find_index++)
    {
      if (card_window_matches_player_and_card(card_windows[find_index], find_player_card) != 0)
      {
        find_found = 1;
        if (msg == 0x40e)
        {
          find_result = get_card_window_displayed_card_id(card_windows[find_index]);
        }
        else
        {
          find_result = (LRESULT)card_windows[find_index];
        }
      }
    }
    if (find_found != 0)
    {
      return find_result;
    }
    else
    {
      return (msg == 0x40e) ? -1 : 0;
    }

  case WM_CLOSE:
    ShowWindow(hwnd, SW_HIDE);
    return 0;

  case WM_COMMAND:
    switch ((unsigned int)wparam & 0xffff)
    {
    case 100:
      help_context = 0x7e3;
      strcpy(help_path, global_base_directory);
      strcat(help_path, "\\duel.hlp");
      WinHelpA(g_duel_window_hwnd, help_path, HELP_CONTEXT, help_context);
      break;
    default:
      break;
    }
    return 0;

  case WM_CREATE:
    card_count = 0;
    SetWindowLongA(hwnd, g_hand_card_count_long_offset, card_count);
    card_windows = malloc(50 * sizeof(*card_windows));
    SetWindowLongPtrA(hwnd, g_hand_card_windows_long_offset, (LONG_PTR)card_windows);
    art_width = 0;
    art_title_height = 0;
    art_side_width = 0;
    art_bottom_height = 0;
    SetWindowLongA(hwnd, g_hand_art_width_long_offset, art_width);
    SetWindowLongA(hwnd, g_hand_art_title_height_long_offset, art_title_height);
    SetWindowLongA(hwnd, g_hand_art_side_width_long_offset, art_side_width);
    SetWindowLongA(hwnd, g_hand_art_bottom_height_long_offset, art_bottom_height);
    background_bitmap = (HBITMAP)0;
    SetWindowLongPtrA(hwnd, g_hand_background_bitmap_long_offset, (LONG_PTR)background_bitmap);
    selected_index = -1;
    SetWindowLongA(hwnd, g_hand_selected_index_long_offset, selected_index);
    if (card_windows == NULL)
    {
      return -1;
    }
    resize_duel_hand_window(hwnd);
    return 0;

  case WM_DESTROY:
    card_windows = (HWND *)GetWindowLongPtrA(hwnd, g_hand_card_windows_long_offset);
    free(card_windows);
    background_bitmap = (HBITMAP)GetWindowLongPtrA(hwnd, g_hand_background_bitmap_long_offset);
    if (background_bitmap != (HBITMAP)0)
    {
      delete_and_close_object(background_bitmap);
    }
    return 0;

  case WM_ERASEBKGND:
    return 1;

  case WM_LBUTTONDOWN:
    GetWindowRect(hwnd, &before_move_rect);
    SendMessageA(hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
    GetWindowRect(hwnd, &after_move_rect);
    if (abs(after_move_rect.top - before_move_rect.top) +
            abs(after_move_rect.left - before_move_rect.left) <
        5)
    {
      click_x = (unsigned int)lparam & 0xffff;
      click_y = HIWORD(lparam);
    card_windows = (HWND *)GetWindowLongPtrA(hwnd, g_hand_card_windows_long_offset);
      card_count = GetWindowLongA(hwnd, g_hand_card_count_long_offset);
      selected_index = GetWindowLongA(hwnd, g_hand_selected_index_long_offset);
      art_width = GetWindowLongA(hwnd, g_hand_art_width_long_offset);
      art_title_height = GetWindowLongA(hwnd, g_hand_art_title_height_long_offset);
      art_side_width = GetWindowLongA(hwnd, g_hand_art_side_width_long_offset);
      art_bottom_height = GetWindowLongA(hwnd, g_hand_art_bottom_height_long_offset);
      if (card_count > 1)
      {
        calculate_hand_art_offsets(g_showlist_smallcard_width, g_showlist_smallcard_height,
                                   art_width, art_title_height, art_side_width,
                                   art_bottom_height, &hit_title_height,
                                   &hit_bottom_margin, &hit_side_margin, &hit_edge_width);
        if ((int)click_y < hit_title_height)
        {
          GetClientRect(hwnd, &after_move_rect);
          if ((int)click_x < (after_move_rect.right * 0x14) / 100)
          {
            if (selected_index > 0)
            {
              target_index = selected_index - 1;
            }
            else
            {
              target_index = card_count - 1;
            }
            SendMessageA(hwnd, 0x400, (WPARAM)card_windows[target_index], 0);
          }
          else if ((after_move_rect.right * 0x50) / 100 < (int)click_x)
          {
            if (selected_index >= card_count - 1)
            {
              target_index = 0;
            }
            else
            {
              target_index = selected_index + 1;
            }
            SendMessageA(hwnd, 0x400, (WPARAM)card_windows[target_index], 0);
          }
        }
      }
    }
    return 0;

  case WM_PAINT:
    art_width = GetWindowLongA(hwnd, g_hand_art_width_long_offset);
    art_title_height = GetWindowLongA(hwnd, g_hand_art_title_height_long_offset);
    art_side_width = GetWindowLongA(hwnd, g_hand_art_side_width_long_offset);
    art_bottom_height = GetWindowLongA(hwnd, g_hand_art_bottom_height_long_offset);
    background_bitmap = (HBITMAP)GetWindowLongPtrA(hwnd, g_hand_background_bitmap_long_offset);
    calculate_hand_art_offsets(g_showlist_smallcard_width, g_showlist_smallcard_height,
                               art_width, art_title_height, art_side_width,
                               art_bottom_height, &title_height, &bottom_margin,
                               &side_margin, &edge_width);
    GetClientRect(hwnd, &client_rect);
    inner_rect.top = client_rect.top + title_height;
    inner_rect.bottom = client_rect.bottom - bottom_margin;
    inner_rect.left = client_rect.left + side_margin;
    inner_rect.right = client_rect.right - side_margin;
    EnterCriticalSection(&g_card_render_lock);
    saved_dc = SaveDC(g_shared_offscreen_dc);
    FillRect(g_shared_offscreen_dc, &client_rect, GetStockObject(4));
    draw_hand_window_frame(g_shared_offscreen_dc, (int *)&client_rect, (int *)&inner_rect,
                           edge_width, art_width, art_title_height,
                           art_side_width, art_bottom_height, background_bitmap);
    SetMapMode(g_shared_offscreen_dc, MM_ANISOTROPIC);
    SetWindowExtEx(g_shared_offscreen_dc, client_rect.right - client_rect.left, 0x14, NULL);
    SetViewportExtEx(g_shared_offscreen_dc, client_rect.right - client_rect.left,
                     (title_height * 0x30) / 100, NULL);
    SelectObject(g_shared_offscreen_dc, g_hand_title_font);
    GetWindowTextA(hwnd, window_title, 100);
    SetBkMode(g_shared_offscreen_dc, TRANSPARENT);
    SetRect(&title_rect, client_rect.left, client_rect.top,
            client_rect.right, inner_rect.top);
    DPtoLP(g_shared_offscreen_dc, (LPPOINT)&title_rect, 2);
    OffsetRect(&title_rect, 2, 2);
    SetTextColor(g_shared_offscreen_dc, g_hand_title_shadow_color);
    DrawTextA(g_shared_offscreen_dc, window_title, -1, &title_rect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    OffsetRect(&title_rect, -2, -2);
    SetTextColor(g_shared_offscreen_dc, g_hand_title_text_color);
    DrawTextA(g_shared_offscreen_dc, window_title, -1, &title_rect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    RestoreDC(g_shared_offscreen_dc, saved_dc);
    paint_dc = BeginPaint(hwnd, &paint_struct);
    if (paint_dc != (HDC)0)
    {
      ApplyCardArtPaletteToDc(paint_dc);
      GetClientRect(hwnd, &blit_rect);
      BitBlt(paint_dc, 0, 0, blit_rect.right, blit_rect.bottom,
             g_shared_offscreen_dc, 0, 0, SRCCOPY);
      EndPaint(hwnd, &paint_struct);
    }
    LeaveCriticalSection(&g_card_render_lock);
    return 0;

  case WM_RBUTTONDOWN:
    menu_point.x = (unsigned int)lparam & 0xffff;
    menu_point.y = HIWORD(lparam);
    ClientToScreen(hwnd, &menu_point);
    SetRect(&menu_rect, menu_point.x, menu_point.y, menu_point.x + 1, menu_point.y + 1);
    TrackPopupMenu(g_hand_popup_menu, TPM_RIGHTBUTTON, menu_point.x, menu_point.y, 0, hwnd, &menu_rect);
    return 0;

  case WM_INITMENU:
    AppendMenuA(g_hand_popup_menu, 0, 100, g_hand_menu_help_text);
    return 0;

  case WM_MENUSELECT:
    if (HIWORD(wparam) == 0xffff && lparam == 0)
    {
      menu_item_count = GetMenuItemCount(g_hand_popup_menu);
      while (menu_item_count-- != 0)
      {
        DeleteMenu(g_hand_popup_menu, 0, MF_BYPOSITION);
      }
    }
    return 0;

  case WM_QUERYNEWPALETTE:
  case WM_PALETTEISCHANGING:
  case WM_PALETTECHANGED:
    return handle_button_palette_message(hwnd, msg, (HWND)wparam, lparam);

  default:
    return DefWindowProcA(hwnd, msg, wparam, lparam);
  }

  return 0;
}
