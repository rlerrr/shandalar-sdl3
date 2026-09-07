#include <windows.h>
#include <stdlib.h>
#include <string.h>

#include "cardartlib/src/palette.h"
#include "defs.h"
#include "magic/src/duel_engine.h"
#include "magic/src/game_support.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "magic/src/shared_startup.h"

typedef struct
{
  int player;
  int card;
  target_t targets[20];
  int number_of_targets;
} spell_chain_display_entry_t;
STATIC_ASSERT(sizeof(spell_chain_display_entry_t) == 0xac, spell_chain_display_entry_t_wrong_size);

typedef struct
{
  HWND spell_window;
  HWND target_windows[20];
  int target_count;
} spell_chain_window_entry_t;
extern char global_base_directory[];
extern HINSTANCE g_app_instance;
extern HWND g_duel_opponent_battlefield_window;
extern int g_showlist_smallcard_width;
extern int g_showlist_smallcard_height;
extern spell_chain_display_entry_t g_duel_cached_stack_entries[32];

int load_text_with_tab_escapes(char *filename, char *section_name);
HBITMAP load_pic(char *filename);
void ApplyCardArtPaletteToDc(HDC dc);
BOOL TileBitmapIntoRect(HDC dc, RECT *rect, HBITMAP bitmap);
int handle_button_palette_message(HWND hwnd, UINT msg, HWND wparam, LPARAM lparam);
int card_window_matches_card_id(HWND hwnd, card_id_t card_id);
int card_window_matches_player_and_card(HWND hwnd, int *player_and_card);
void layout_phase_display_window(HWND hwnd, LPRECT rect);
int handle_duel_inactive_cursor(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
void restack_duel_child_windows(void);
void delete_and_close_object(HANDLE obj);

// GLOBAL: MAGIC 0x0055e03c
// GLOBAL: SHANDALAR 0x0057f0d0
int g_spell_chain_windows_long_offset = 0;

// GLOBAL: MAGIC 0x0055e040
// GLOBAL: SHANDALAR 0x0057f0d4
int g_spell_chain_count_long_offset = 4;

// GLOBAL: MAGIC 0x0055e044
// GLOBAL: SHANDALAR 0x0057f0d8
int g_spell_chain_window_extra_bytes = 8;

// GLOBAL: MAGIC 0x00570f64
// GLOBAL: SHANDALAR 0x00589be0
int g_spell_chain_scrollbar_direction = 3;

// GLOBAL: MAGIC 0x00637e58
// GLOBAL: SHANDALAR 0x005b7ba8
HWND g_spell_minimized_hwnd;

// GLOBAL: MAGIC 0x00637e60
// GLOBAL: SHANDALAR 0x005b7bb0
char g_spell_chain_menu_minimize_text[0x1c];

// GLOBAL: MAGIC 0x00637e7c
// GLOBAL: SHANDALAR 0x005b7bcc
COLORREF g_spell_chain_title_text_color;

// GLOBAL: MAGIC 0x00637e80
// GLOBAL: SHANDALAR 0x005b7bd0
HPEN g_spell_chain_shadow_pen;

// GLOBAL: MAGIC 0x00637e88
// GLOBAL: SHANDALAR 0x005b7bd8
char g_spell_chain_menu_help_text[0x20];

// GLOBAL: MAGIC 0x00637ea8
// GLOBAL: SHANDALAR 0x005b7bf8
char g_spell_minimized_menu_help_text[0x1c];

// GLOBAL: MAGIC 0x00637ec4
// GLOBAL: SHANDALAR 0x005b7c14
HMENU g_spell_minimized_popup_menu;

// GLOBAL: MAGIC 0x00637ec8
// GLOBAL: SHANDALAR 0x005b7c18
HPEN g_spell_chain_dark_pen;

// GLOBAL: MAGIC 0x00637ed0
// GLOBAL: SHANDALAR 0x005b7c20
RECT g_spell_chain_layout_rect;

// GLOBAL: MAGIC 0x00637ee0
// GLOBAL: SHANDALAR 0x005b7c30
HBITMAP g_spell_chain_scrollbar_track_bitmap;

// GLOBAL: MAGIC 0x00637ee4
// GLOBAL: SHANDALAR 0x005b7c34
HBITMAP g_spell_chain_scrollbar_thumb_bitmap;

// GLOBAL: MAGIC 0x00637ee8
// GLOBAL: SHANDALAR 0x005b7c38
HBITMAP g_spell_minimized_background_bitmap;

// GLOBAL: MAGIC 0x00637ef0
// GLOBAL: SHANDALAR 0x005b7c40
char g_spell_minimized_menu_restore_text[0x1c];

// GLOBAL: MAGIC 0x00637f0c
// GLOBAL: SHANDALAR 0x005b7c5c
HBITMAP g_spell_chain_background_bitmap;

// GLOBAL: MAGIC 0x00637f10
// GLOBAL: SHANDALAR 0x005b7c60
HBRUSH g_spell_chain_title_brush;

// GLOBAL: MAGIC 0x00637f14
// GLOBAL: SHANDALAR 0x005b7c64
HPEN g_spell_chain_highlight_pen;

// FUNCTION: MAGIC 0x0044a08e
// FUNCTION: SHANDALAR 0x00453c0f
int copy_spell_chain_display_entries(spell_chain_display_entry_t *entries)
{
  int count;

  if (entries == NULL)
  {
    return 0;
  }

  EnterCriticalSection(&g_duel_render_lock);
  count = g_multiblock_creature_count;
  memcpy(entries, g_duel_cached_stack_entries, sizeof(g_duel_cached_stack_entries));
  LeaveCriticalSection(&g_duel_render_lock);

  return count;
}

// FUNCTION: MAGIC 0x00486050
// FUNCTION: SHANDALAR 0x004c8c30
int register_MAGICGAME_SpellChainClass(LPCSTR class_name)
{
  char path[264];
  int registered;
  WNDCLASSA wndclass;

  registered = 1;
  SET_DUEL_WNDCLASS(wndclass, 0x800, wndproc_MAGICGAME_SpellChainClass,
                    g_spell_chain_window_extra_bytes, (HICON)0, (HBRUSH)0x6, class_name);
  if (RegisterClassA(&wndclass) == 0)
  {
    registered = 0;
  }

  SET_DUEL_WNDCLASS(wndclass, 3, wndproc_SpellMinimized, 0, (HICON)0, (HBRUSH)0x6, "SpellMinimized");
  if (RegisterClassA(&wndclass) == 0)
  {
    registered = 0;
  }

  g_spell_minimized_popup_menu = CreatePopupMenu();
  load_text_with_tab_escapes(global_ui_strings_filename, "MENU_SPELLCHAIN");
  strcpy(g_spell_chain_menu_minimize_text, g_text_lines[0]);
  strcpy(g_spell_chain_menu_help_text, g_text_lines[1]);
  load_text_with_tab_escapes(global_ui_strings_filename, "MENU_MINIMIZEDSPELLCHAIN");
  strcpy(g_spell_minimized_menu_restore_text, g_text_lines[0]);
  strcpy(g_spell_minimized_menu_help_text, g_text_lines[1]);

  strcpy(path, global_duelart_path);
  strcat(path, "\\WINBK_SpellChain.pic");
  g_spell_chain_background_bitmap = load_pic(path);
  strcpy(path, global_duelart_path);
  strcat(path, "\\WINBK_SpellMushrooms.pic");
  g_spell_chain_scrollbar_thumb_bitmap = load_pic(path);
  strcpy(path, global_duelart_path);
  strcat(path, "\\WINBK_SpellSnails.pic");
  g_spell_chain_scrollbar_track_bitmap = load_pic(path);
  g_spell_chain_scrollbar_direction = 3;
  strcpy(path, global_duelart_path);
  strcat(path, "\\WINBK_SpellMin.pic");
  g_spell_minimized_background_bitmap = load_pic(path);

  SetRect(&g_spell_chain_layout_rect, 0, 0, 0, 0);
  g_spell_chain_shadow_pen = CreatePen(0, 0, 0x1000040);
  g_spell_chain_highlight_pen = CreatePen(0, 0, 0x1000037);
  g_spell_chain_dark_pen = CreatePen(0, 0, 0x1000016);
  g_spell_chain_title_brush = CreateSolidBrush(0x1000037);
  g_spell_chain_title_text_color = 0x1000048;
  if (g_spell_chain_shadow_pen == (HPEN)0 || g_spell_chain_highlight_pen == (HPEN)0 ||
      g_spell_chain_dark_pen == (HPEN)0 || g_spell_chain_title_brush == (HBRUSH)0)
  {
    registered = 0;
  }

  return registered;
}

// FUNCTION: MAGIC 0x00486369
// FUNCTION: SHANDALAR 0x004c8f49
void destroy_MAGICGAME_SpellChainClass(LPCSTR class_name)
{
  if (g_spell_minimized_popup_menu != (HMENU)0)
  {
    DestroyMenu(g_spell_minimized_popup_menu);
  }
  g_spell_minimized_popup_menu = (HMENU)0;
  if (g_spell_chain_background_bitmap != (HBITMAP)0)
  {
    delete_and_close_object(g_spell_chain_background_bitmap);
  }
  if (g_spell_chain_scrollbar_thumb_bitmap != (HBITMAP)0)
  {
    delete_and_close_object(g_spell_chain_scrollbar_thumb_bitmap);
  }
  if (g_spell_chain_scrollbar_track_bitmap != (HBITMAP)0)
  {
    delete_and_close_object(g_spell_chain_scrollbar_track_bitmap);
  }
  if (g_spell_minimized_background_bitmap != (HBITMAP)0)
  {
    delete_and_close_object(g_spell_minimized_background_bitmap);
  }
  if (g_spell_chain_shadow_pen != (HPEN)0)
  {
    DeleteObject(g_spell_chain_shadow_pen);
  }
  if (g_spell_chain_highlight_pen != (HPEN)0)
  {
    DeleteObject(g_spell_chain_highlight_pen);
  }
  if (g_spell_chain_dark_pen != (HPEN)0)
  {
    DeleteObject(g_spell_chain_dark_pen);
  }
  if (g_spell_chain_title_brush != (HBRUSH)0)
  {
    DeleteObject(g_spell_chain_title_brush);
  }
  g_spell_chain_background_bitmap = (HBITMAP)0;
  g_spell_chain_scrollbar_thumb_bitmap = (HBITMAP)0;
  g_spell_chain_scrollbar_track_bitmap = (HBITMAP)0;
  g_spell_minimized_background_bitmap = (HBITMAP)0;
  g_spell_chain_shadow_pen = (HPEN)0;
  g_spell_chain_highlight_pen = (HPEN)0;
  g_spell_chain_dark_pen = (HPEN)0;
  g_spell_chain_title_brush = (HBRUSH)0;
}

// FUNCTION: MAGIC 0x00488229
// FUNCTION: SHANDALAR 0x004cadfe
int find_spell_chain_entry(HWND hwnd, spell_chain_display_entry_t entry, int start_index)
{
  spell_chain_window_entry_t *windows;
  int result;
  int i;
  int window_count;

  if (hwnd == (HWND)0)
  {
    return -1;
  }

  windows = (spell_chain_window_entry_t *)GetWindowLongPtrA(hwnd, g_spell_chain_windows_long_offset);
  window_count = GetWindowLongA(hwnd, g_spell_chain_count_long_offset);
  result = -1;
  for (i = start_index; i < window_count && result == -1; ++i)
  {
    if (card_window_matches_player_and_card(windows[i].spell_window, &entry.player) != 0)
    {
      result = i;
    }
  }
  return result;
}

// FUNCTION: MAGIC 0x004882d8
// FUNCTION: SHANDALAR 0x004caead
void remove_spell_chain_entry(HWND hwnd, int entry_index)
{
  spell_chain_window_entry_t *windows;
  int target_index;
  int i;
  int window_count;

  if (hwnd == (HWND)0)
  {
    return;
  }
  if (entry_index < 0 || entry_index > 0x64)
  {
    return;
  }

  windows = (spell_chain_window_entry_t *)GetWindowLongPtrA(hwnd, g_spell_chain_windows_long_offset);
  window_count = GetWindowLongA(hwnd, g_spell_chain_count_long_offset);
  DestroyWindow(windows[entry_index].spell_window);
  for (target_index = 0; target_index < windows[entry_index].target_count; ++target_index)
  {
    if (windows[entry_index].target_windows[target_index] != (HWND)0)
    {
      DestroyWindow(windows[entry_index].target_windows[target_index]);
    }
  }
  for (i = entry_index; i < window_count - 1; ++i)
  {
    memcpy(&windows[i], &windows[i + 1], sizeof(windows[i]));
  }
  --window_count;
  SetWindowLongA(hwnd, g_spell_chain_count_long_offset, window_count);
}

// FUNCTION: MAGIC 0x00488425
// FUNCTION: SHANDALAR 0x004caffa
int spell_chain_entry_matches(spell_chain_window_entry_t window_entry, spell_chain_display_entry_t display_entry)
{
  int matches;
  int i;
  matches = 1;

  if (window_entry.target_count != display_entry.number_of_targets)
  {
    matches = 0;
  }

  for (i = 0; !(window_entry.target_count <= i); ++i)
  {
    if (card_window_matches_player_and_card(window_entry.target_windows[i], &display_entry.targets[i].player) == 0)
    {
      matches = 0;
    }
  }
  return matches;
}

// FUNCTION: MAGIC 0x004884a2
// FUNCTION: SHANDALAR 0x004cb077
int add_spell_chain_entry(HWND hwnd, spell_chain_display_entry_t display_entry, int insert_index)
{
  target_t create_target;
  int result;
  int target_index;
  int shift_index;
  int window_count;
  spell_chain_window_entry_t new_entry;
  spell_chain_window_entry_t *windows;

  if (hwnd == (HWND)0)
  {
    return 0;
  }

  windows = (spell_chain_window_entry_t *)GetWindowLongPtrA(hwnd, g_spell_chain_windows_long_offset);
  window_count = GetWindowLongA(hwnd, g_spell_chain_count_long_offset);
  if (window_count == 100)
  {
    return 0;
  }

  result = 1;
  create_target.player = display_entry.player;
  create_target.card = display_entry.card;
  new_entry.spell_window = CreateWindowExA(0, "MAGICGAME_CardClass", "Spell Card", 0x50000000,
                                           0, 0, 0, 0, hwnd, (HMENU)1, g_app_instance, &create_target);
  if (new_entry.spell_window == (HWND)0)
  {
    result = 0;
  }

  new_entry.target_count = 0;
  for (target_index = 0; target_index < display_entry.number_of_targets && result != 0; ++target_index)
  {
    SET_TARGET(create_target, display_entry.targets[target_index]);
    new_entry.target_windows[target_index] =
        CreateWindowExA(0, "MAGICGAME_CardClass", "Spell Target Card", 0x50000000,
                        0, 0, 0, 0, hwnd, (HMENU)1, g_app_instance, &create_target);
    if (new_entry.target_windows[target_index] == (HWND)0)
    {
      result = 0;
    }
    else
    {
      ++new_entry.target_count;
    }
  }

  if (result != 0)
  {
    for (shift_index = insert_index; shift_index < window_count; ++shift_index)
    {
      memcpy(&windows[shift_index + 1], &windows[shift_index], sizeof(windows[shift_index]));
    }
    memcpy(&windows[insert_index], &new_entry, sizeof(windows[insert_index]));
    ++window_count;
    SetWindowLongA(hwnd, g_spell_chain_count_long_offset, window_count);
  }
  else
  {
    if (new_entry.spell_window != (HWND)0)
    {
      DestroyWindow(new_entry.spell_window);
    }
    for (target_index = 0; target_index < new_entry.target_count; ++target_index)
    {
      if (new_entry.target_windows[target_index] != (HWND)0)
      {
        DestroyWindow(new_entry.target_windows[target_index]);
      }
    }
  }

  return result;
}

// FUNCTION: MAGIC 0x004886db
// FUNCTION: SHANDALAR 0x004cb2b0
void destroy_spell_chain_target_windows(HWND hwnd, int entry_index)
{
  spell_chain_window_entry_t *windows;
  int target_index;
  int window_count;

  if (hwnd == (HWND)0)
  {
    return;
  }
  if (entry_index < 0 || entry_index > 0x64)
  {
    return;
  }

  windows = (spell_chain_window_entry_t *)GetWindowLongPtrA(hwnd, g_spell_chain_windows_long_offset);
  window_count = GetWindowLongA(hwnd, g_spell_chain_count_long_offset);
  for (target_index = 0; target_index < windows[entry_index].target_count; ++target_index)
  {
    if (windows[entry_index].target_windows[target_index] != (HWND)0)
    {
      DestroyWindow(windows[entry_index].target_windows[target_index]);
    }
  }
  windows[entry_index].target_count = 0;
}

// FUNCTION: MAGIC 0x004887c0
// FUNCTION: SHANDALAR 0x004cb395
int update_spell_chain_entry_targets(HWND hwnd, spell_chain_display_entry_t display_entry, int entry_index)
{
  target_t create_target;
  int result;
  int target_index;
  int window_count;
  spell_chain_window_entry_t new_entry;
  spell_chain_window_entry_t *windows;

  if (hwnd == (HWND)0)
  {
    return 0;
  }

  windows = (spell_chain_window_entry_t *)GetWindowLongPtrA(hwnd, g_spell_chain_windows_long_offset);
  window_count = GetWindowLongA(hwnd, g_spell_chain_count_long_offset);
  result = 1;
  new_entry.spell_window = windows[entry_index].spell_window;
  new_entry.target_count = 0;
  for (target_index = 0; target_index < display_entry.number_of_targets && result != 0; ++target_index)
  {
    SET_TARGET(create_target, display_entry.targets[target_index]);
    new_entry.target_windows[target_index] =
        CreateWindowExA(0, "MAGICGAME_CardClass", "Spell Target Card", 0x50000000,
                        0, 0, 0, 0, hwnd, (HMENU)1, g_app_instance, &create_target);
    if (new_entry.target_windows[target_index] == (HWND)0)
    {
      result = 0;
    }
    else
    {
      ++new_entry.target_count;
    }
  }

  if (result != 0)
  {
    memcpy(&windows[entry_index], &new_entry, sizeof(windows[entry_index]));
  }
  else
  {
    for (target_index = 0; target_index < new_entry.target_count; ++target_index)
    {
      if (new_entry.target_windows[target_index] != (HWND)0)
      {
        DestroyWindow(new_entry.target_windows[target_index]);
      }
    }
    windows[entry_index].target_count = 0;
  }

  return result;
}

static __inline void draw_spell_chain_frame(HWND hwnd, HDC dc)
{
  RECT window_rect;
  RECT client_rect;
  RECT title_rect;
  RECT button_rect;
  char title[100];
  BITMAP button_bitmap;
  HBITMAP close_button;
  int client_border_width;
  int bottom_border_height;
  int x;
  int y;
  int i;

  GetWindowRect(hwnd, &window_rect);
  GetClientRect(hwnd, &client_rect);
  MapWindowPoints(hwnd, (HWND)0, (LPPOINT)&client_rect, 2);
  OffsetRect(&client_rect, -window_rect.left, -window_rect.top);
  OffsetRect(&window_rect, -window_rect.left, -window_rect.top);
  GetWindowTextA(hwnd, title, 100);
  client_border_width = window_rect.right - client_rect.right;
  bottom_border_height = window_rect.bottom - client_rect.bottom;

  SelectObject(dc, g_spell_chain_highlight_pen);
  MoveToEx(dc, 0, 0, NULL);
  LineTo(dc, window_rect.right - 1, 0);
  SelectObject(dc, g_spell_chain_shadow_pen);
  y = 1;
  for (i = 1; i <= bottom_border_height - 2; ++i)
  {
    MoveToEx(dc, 1, y, NULL);
    LineTo(dc, window_rect.right - client_border_width + 1, y);
    ++y;
  }
  SelectObject(dc, g_spell_chain_highlight_pen);
  y = bottom_border_height - 1;
  MoveToEx(dc, client_border_width - 1, y, NULL);
  LineTo(dc, client_rect.right + 1, y);

  SelectObject(dc, g_spell_chain_highlight_pen);
  x = 0;
  MoveToEx(dc, 0, 0, NULL);
  LineTo(dc, x, window_rect.bottom - 1);
  SelectObject(dc, g_spell_chain_shadow_pen);
  x = 1;
  for (i = 1; i <= client_border_width - 2; ++i)
  {
    MoveToEx(dc, x, 1, NULL);
    LineTo(dc, x, window_rect.bottom - 1);
    ++x;
  }
  SelectObject(dc, g_spell_chain_highlight_pen);
  x = client_rect.left - 1;
  MoveToEx(dc, x, bottom_border_height - 1, NULL);
  LineTo(dc, x, client_rect.bottom + 1);

  SelectObject(dc, GetStockObject(BLACK_PEN));
  x = window_rect.right - 1;
  MoveToEx(dc, x, 0, NULL);
  LineTo(dc, x, window_rect.bottom);
  SelectObject(dc, g_spell_chain_dark_pen);
  x = window_rect.right - 2;
  for (i = 1; i <= client_border_width - 2; ++i)
  {
    MoveToEx(dc, x, 1, NULL);
    LineTo(dc, x, window_rect.bottom - 1);
    --x;
  }
  SelectObject(dc, g_spell_chain_highlight_pen);
  x = client_rect.right;
  MoveToEx(dc, client_rect.right, bottom_border_height - 1, NULL);
  LineTo(dc, x, client_rect.bottom + 1);

  SelectObject(dc, GetStockObject(BLACK_PEN));
  y = window_rect.bottom - 1;
  MoveToEx(dc, 0, y, NULL);
  LineTo(dc, window_rect.right, y);
  SelectObject(dc, g_spell_chain_dark_pen);
  y = window_rect.bottom - 2;
  for (i = 1; i <= bottom_border_height - 2; ++i)
  {
    MoveToEx(dc, 1, y, NULL);
    LineTo(dc, window_rect.right - 1, y);
    --y;
  }
  SelectObject(dc, g_spell_chain_highlight_pen);
  y = window_rect.bottom - bottom_border_height;
  MoveToEx(dc, client_border_width - 1, y, NULL);
  LineTo(dc, window_rect.right - 2, y);
  SelectObject(dc, g_spell_chain_highlight_pen);
  y = client_rect.top - 1;
  MoveToEx(dc, client_rect.left, y, NULL);
  LineTo(dc, client_rect.right + 1, y);

  SetRect(&title_rect, client_rect.left, bottom_border_height, client_rect.right, client_rect.top - 1);
  FillRect(dc, &title_rect, g_spell_chain_title_brush);
  SetTextColor(dc, g_spell_chain_title_text_color);
  SetBkMode(dc, TRANSPARENT);
  title_rect.left += 5;
  DrawTextA(dc, title, -1, &title_rect, DT_SINGLELINE | DT_VCENTER);

  close_button = LoadBitmapA((HINSTANCE)0, (LPCSTR)0x7fed);
  GetObjectA(close_button, sizeof(button_bitmap), &button_bitmap);
  SetRect(&button_rect, client_rect.right - button_bitmap.bmWidth, client_rect.top - button_bitmap.bmHeight,
          client_rect.right, client_rect.top);
  DrawBitmapToRect(dc, &button_rect, g_spell_minimized_background_bitmap);
  DeleteObject(close_button);
}

// FUNCTION: MAGIC 0x004864b7
// FUNCTION: SHANDALAR 0x004c9097
LRESULT CALLBACK wndproc_MAGICGAME_SpellChainClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  char stack_padding[0x70];
  int menu_item_count;
  POINT popup_point;
  RECT popup_rect;
  UINT scrollbar_new_pos;
  char background_path[264];
  HDC dc;
  BITMAP bitmap;
  int scrollbar_height;
  int scrollbar_y;
  RECT client_rect;
  RECT tile_rect;
  LRESULT scroll_pos;
  HWND scrollbar_hwnd;
  RECT scroll_rect;
  RECT window_rect;
  int hit_border_width;
  LRESULT hit_test;
  int paint_result;
  HWND create_scrollbar_hwnd;
  char help_path[264];
  DWORD help_topic;
  int minimized_height;
  BITMAP minimized_bitmap;
  int minimized_width;
  int minimized_y;
  int minimized_x;
  RECT face_rect;
  int clear_target_index;
  int clear_entry_index;
  int sync_existing_index;
  int sync_target_index;
  int sync_j;
  int sync_changed;
  spell_chain_display_entry_t display_entries[32];
  int sync_count;
  BOOL changed;
  int *existing_player_and_card;
  int j;
  LPARAM message_lparam;
  HWND child_hwnd;
  int min_scroll_pos;
  int max_scroll_pos;
  int current_scroll_pos;
  int scroll_step;
  WPARAM card_id;
  int target_index;
  int i;
  spell_chain_window_entry_t *windows;
  LONG window_count;

  switch (msg)
  {
  case 0x437:
    return 0;

  case 0x433:
  case 0x434:
    windows = (spell_chain_window_entry_t *)GetWindowLongPtrA(hwnd, g_spell_chain_windows_long_offset);
    window_count = GetWindowLongA(hwnd, g_spell_chain_count_long_offset);
    card_id = wparam;
    for (i = 0; i < window_count; ++i)
    {
      if (card_window_matches_card_id(windows[i].spell_window, card_id) != 0)
      {
        InvalidateRect(windows[i].spell_window, NULL, FALSE);
      }
      for (target_index = 0; target_index < windows[i].target_count; ++target_index)
      {
        if (card_window_matches_card_id((&windows[i])->target_windows[target_index], card_id) != 0)
        {
          InvalidateRect((&windows[i])->target_windows[target_index], NULL, FALSE);
        }
      }
    }
    return 0;

  case 0x432:
    windows = (spell_chain_window_entry_t *)GetWindowLongPtrA(hwnd, g_spell_chain_windows_long_offset);
    window_count = GetWindowLongA(hwnd, g_spell_chain_count_long_offset);
    for (scroll_step = 0; scroll_step < window_count; ++scroll_step)
    {
      SendMessageA(windows[scroll_step].spell_window, 0x432, 0, 0);
      for (current_scroll_pos = 0; current_scroll_pos < windows[scroll_step].target_count; ++current_scroll_pos)
      {
        SendMessageA((&windows[scroll_step])->target_windows[current_scroll_pos], 0x432, 0, 0);
      }
    }
    return 0;

  case 0x435:
    windows = (spell_chain_window_entry_t *)GetWindowLongPtrA(hwnd, g_spell_chain_windows_long_offset);
    window_count = GetWindowLongA(hwnd, g_spell_chain_count_long_offset);
    for (max_scroll_pos = 0; !(window_count <= max_scroll_pos); ++max_scroll_pos)
    {
      InvalidateRect(windows[max_scroll_pos].spell_window, NULL, FALSE);
      for (min_scroll_pos = 0; min_scroll_pos < windows[max_scroll_pos].target_count; ++min_scroll_pos)
      {
        InvalidateRect((&windows[max_scroll_pos])->target_windows[min_scroll_pos], NULL, FALSE);
      }
    }
    return 0;

  case 0x436:
    windows = (spell_chain_window_entry_t *)GetWindowLongPtrA(hwnd, g_spell_chain_windows_long_offset);
    window_count = GetWindowLongA(hwnd, g_spell_chain_count_long_offset);
    existing_player_and_card = (int *)wparam;
    changed = (BOOL)lparam;
    if (existing_player_and_card == NULL)
    {
      return 0;
    }
    child_hwnd = 0;
    for (message_lparam = 0; message_lparam < window_count && child_hwnd == 0; ++message_lparam)
    {
      if (card_window_matches_player_and_card(windows[message_lparam].spell_window, existing_player_and_card) != 0)
      {
        child_hwnd = (HWND)1;
        if (changed != 0)
        {
          SendMessageA(windows[message_lparam].spell_window, 0x432, 0, 0);
        }
        else
        {
          InvalidateRect(windows[message_lparam].spell_window, NULL, FALSE);
        }
      }
      for (j = 0; j < windows[message_lparam].target_count; ++j)
      {
        if (card_window_matches_player_and_card((&windows[message_lparam])->target_windows[j], existing_player_and_card) != 0)
        {
          child_hwnd = (HWND)1;
          if (changed != 0)
          {
            SendMessageA((&windows[message_lparam])->target_windows[j], 0x432, 0, 0);
          }
          else
          {
            InvalidateRect((&windows[message_lparam])->target_windows[j], NULL, FALSE);
          }
        }
      }
    }
    return 0;

  case 0x412:
    windows = (spell_chain_window_entry_t *)GetWindowLongPtrA(hwnd, g_spell_chain_windows_long_offset);
    window_count = GetWindowLongA(hwnd, g_spell_chain_count_long_offset);
    sync_count = copy_spell_chain_display_entries(display_entries);
    sync_changed = 0;
    for (sync_j = 0; !(sync_count <= sync_j); ++sync_j)
    {
      sync_existing_index = find_spell_chain_entry(hwnd, display_entries[sync_j], sync_j);
      if (sync_existing_index == sync_j)
      {
        if (spell_chain_entry_matches(windows[sync_existing_index], display_entries[sync_existing_index]) == 0)
        {
          destroy_spell_chain_target_windows(hwnd, sync_existing_index);
          update_spell_chain_entry_targets(hwnd, display_entries[sync_existing_index], sync_existing_index);
          sync_changed = 1;
        }
      }
      else if (sync_existing_index == -1)
      {
        add_spell_chain_entry(hwnd, display_entries[sync_j], sync_j);
        sync_changed = 1;
      }
      else if (!(sync_existing_index <= sync_j))
      {
        for (sync_target_index = sync_j; !(sync_existing_index <= sync_target_index); ++sync_target_index)
        {
          remove_spell_chain_entry(hwnd, sync_target_index);
        }
        sync_changed = 1;
      }
    }
    window_count = GetWindowLongA(hwnd, g_spell_chain_count_long_offset);
    for (sync_target_index = sync_j; !(window_count <= sync_target_index); ++sync_target_index)
    {
      remove_spell_chain_entry(hwnd, sync_target_index);
      sync_changed = 1;
    }
    if (sync_changed != 0)
    {
      layout_phase_display_window(hwnd, &g_spell_chain_layout_rect);
    }
    return 0;

  case 0x40c:
    windows = (spell_chain_window_entry_t *)GetWindowLongPtrA(hwnd, g_spell_chain_windows_long_offset);
    window_count = GetWindowLongA(hwnd, g_spell_chain_count_long_offset);
    ShowWindow(hwnd, SW_HIDE);
    ShowWindow(g_spell_minimized_hwnd, SW_HIDE);
    for (clear_entry_index = 0; !(window_count <= clear_entry_index); ++clear_entry_index)
    {
      DestroyWindow(windows[clear_entry_index].spell_window);
      for (clear_target_index = 0; !(windows[clear_entry_index].target_count <= clear_target_index); ++clear_target_index)
      {
        DestroyWindow(windows[clear_entry_index].target_windows[clear_target_index]);
      }
      windows[clear_entry_index].target_count = 0;
    }
    window_count = 0;
    SetWindowLongA(hwnd, g_spell_chain_count_long_offset, window_count);
    SendMessageA(GetDlgItem(hwnd, 0), 0xe0, 0, TRUE);
    layout_phase_display_window(hwnd, &g_spell_chain_layout_rect);
    return 0;

  case WM_CLOSE:
    ShowWindow(hwnd, SW_HIDE);
    ShowWindow(g_spell_minimized_hwnd, SW_HIDE);
    return 0;

  case WM_COMMAND:
    switch (wparam & 0xffff)
    {
    case 0x65:
      ShowWindow(hwnd, SW_HIDE);
      UpdateWindow(g_duel_help_owner_hwnd);
      GetWindowRect(g_duel_opponent_battlefield_window, &face_rect);
      minimized_x = face_rect.left;
      minimized_width = face_rect.right - face_rect.left;
      if (g_spell_minimized_background_bitmap != (HBITMAP)0)
      {
        GetObjectA(g_spell_minimized_background_bitmap, sizeof(minimized_bitmap), &minimized_bitmap);
        minimized_height = (minimized_bitmap.bmHeight * minimized_width) / minimized_bitmap.bmWidth;
      }
      else
      {
        minimized_height = minimized_width * 2;
      }
      minimized_y = (face_rect.bottom - face_rect.top) / 2 - minimized_height / 2;
      MoveWindow(g_spell_minimized_hwnd, minimized_x, minimized_y, minimized_width, minimized_height, TRUE);
      ShowWindow(g_spell_minimized_hwnd, SW_SHOW);
      BringWindowToTop(g_spell_minimized_hwnd);
      SendMessageA(g_duel_prompt_context_hwnd, 0x403, 0, 0);
      break;

    case 0x66:
      ShowWindow(g_spell_minimized_hwnd, SW_HIDE);
      ShowWindow(hwnd, SW_SHOW);
      SendMessageA(g_duel_prompt_context_hwnd, 0x403, 0, 0);
      restack_duel_child_windows();
      break;

    case 100:
      help_topic = 0xbd2;
      strcpy(help_path, global_base_directory);
      strcat(help_path, "\\duel.hlp");
      WinHelpA(g_duel_window_hwnd, help_path, HELP_CONTEXT, help_topic);
      break;
    }
    return 0;

  case WM_CREATE:
    window_count = 0;
    SetWindowLongA(hwnd, g_spell_chain_count_long_offset, window_count);
    windows = (spell_chain_window_entry_t *)malloc(100 * sizeof(*windows));
    SetWindowLongPtrA(hwnd, g_spell_chain_windows_long_offset, (LONG_PTR)windows);
    create_scrollbar_hwnd = CreateWindowExA(0, "MAGICGAME_ScrollbarClass", "", WS_CHILD | WS_VISIBLE,
                                            0, 0, 0, 0, hwnd, (HMENU)0, g_app_instance, NULL);
    if (create_scrollbar_hwnd != (HWND)0)
    {
      SendMessageA(create_scrollbar_hwnd, 0x464, (WPARAM)g_spell_chain_scrollbar_thumb_bitmap, 0);
      SendMessageA(create_scrollbar_hwnd, 0x466, (WPARAM)g_spell_chain_scrollbar_track_bitmap, g_spell_chain_scrollbar_direction);
    }
    g_spell_minimized_hwnd = CreateWindowExA(0, "SpellMinimized", "", WS_POPUP,
                                                  0, 0, 0, 0, hwnd, (HMENU)0, g_app_instance, NULL);
    if (windows == NULL || create_scrollbar_hwnd == (HWND)0 || g_spell_minimized_hwnd == (HWND)0)
    {
      if (windows != NULL)
      {
        free(windows);
      }
      return -1;
    }
    return 0;

  case WM_DESTROY:
    windows = (spell_chain_window_entry_t *)GetWindowLongPtrA(hwnd, g_spell_chain_windows_long_offset);
    free(windows);
    return 0;

  case WM_ERASEBKGND:
    dc = (HDC)wparam;
    ApplyCardArtPaletteToDc(dc);
    GetClientRect(hwnd, &client_rect);
    scroll_pos = SendDlgItemMessageA(hwnd, 0, 0xe1, 0, 0);
    if (g_spell_chain_background_bitmap == (HBITMAP)0)
    {
      strcpy(background_path, global_duelart_path);
      strcat(background_path, "\\WINBK_SpellChain.pic");
      g_spell_chain_background_bitmap = load_pic(background_path);
    }
    if (g_spell_chain_background_bitmap != (HBITMAP)0)
    {
      CopyRect(&tile_rect, &client_rect);
      GetObjectA(g_spell_chain_background_bitmap, sizeof(bitmap), &bitmap);
      tile_rect.left = -(scroll_pos % bitmap.bmWidth);
      TileBitmapIntoRect(dc, &tile_rect, g_spell_chain_background_bitmap);
    }
    else
    {
      FillRect(dc, &client_rect, GetStockObject(GRAY_BRUSH));
    }
    return 1;

  case WM_HSCROLL:
    scrollbar_hwnd = GetDlgItem(hwnd, 0);
    SendMessageA(scrollbar_hwnd, 0xe3, (WPARAM)&min_scroll_pos, (LPARAM)&max_scroll_pos);
    current_scroll_pos = SendMessageA(scrollbar_hwnd, 0xe1, 0, 0);
    scroll_step = g_showlist_smallcard_width;
    GetClientRect(hwnd, &client_rect);
    switch (wparam & 0xffff)
    {
    case SB_LINELEFT:
      scrollbar_new_pos = current_scroll_pos - scroll_step;
      break;
    case SB_LINERIGHT:
      scrollbar_new_pos = current_scroll_pos + scroll_step;
      break;
    case SB_PAGELEFT:
      scrollbar_new_pos = current_scroll_pos - client_rect.right;
      break;
    case SB_PAGERIGHT:
      scrollbar_new_pos = current_scroll_pos + client_rect.right;
      break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
      scrollbar_new_pos = HIWORD(wparam);
      break;
    case SB_LEFT:
      scrollbar_new_pos = min_scroll_pos;
      break;
    case SB_RIGHT:
      scrollbar_new_pos = max_scroll_pos;
      break;
    default:
      scrollbar_new_pos = current_scroll_pos;
      break;
    }
    if ((int)scrollbar_new_pos < min_scroll_pos)
    {
      scrollbar_new_pos = min_scroll_pos;
    }
    if (max_scroll_pos < (int)scrollbar_new_pos)
    {
      scrollbar_new_pos = max_scroll_pos;
    }
    if (scrollbar_new_pos != (UINT)current_scroll_pos)
    {
      SendMessageA(scrollbar_hwnd, 0xe0, scrollbar_new_pos, TRUE);
      GetWindowRect(scrollbar_hwnd, &scroll_rect);
      MapWindowPoints((HWND)0, hwnd, (LPPOINT)&scroll_rect, 2);
      ScrollWindow(hwnd, current_scroll_pos - scrollbar_new_pos, 0, NULL, NULL);
      MoveWindow(scrollbar_hwnd, scroll_rect.left, scroll_rect.top,
                 scroll_rect.right - scroll_rect.left, scroll_rect.bottom - scroll_rect.top, FALSE);
      UpdateWindow(hwnd);
    }
    return 0;

  case WM_LBUTTONDOWN:
    return 0;

  case WM_NCHITTEST:
    hit_test = DefWindowProcA(hwnd, msg, wparam, lparam);
    if (hit_test != HTCAPTION)
    {
      return hit_test;
    }
    hit_border_width = GetSystemMetrics(SM_CXSIZE);
    GetClientRect(hwnd, &client_rect);
    MapWindowPoints(hwnd, (HWND)0, (LPPOINT)&client_rect, 2);
    return HTMINBUTTON;

  case WM_NCLBUTTONDOWN:
    if (wparam == HTMINBUTTON)
    {
      SendMessageA(hwnd, WM_COMMAND, 0x65, 0);
      return 0;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);

  case WM_NCPAINT:
  case WM_NCACTIVATE:
    GetWindowRect(hwnd, &window_rect);
    OffsetRect(&window_rect, -window_rect.left, -window_rect.top);
    if (window_rect.right == window_rect.left || window_rect.bottom == window_rect.top)
    {
      return DefWindowProcA(hwnd, msg, wparam, lparam);
    }

    paint_result = msg != WM_NCPAINT;
    dc = GetWindowDC(hwnd);
    if (dc == (HDC)0)
    {
      return paint_result;
    }
    ApplyCardArtPaletteToDc(dc);
    draw_spell_chain_frame(hwnd, dc);
    ReleaseDC(hwnd, dc);
    return paint_result;

  case WM_SHOWWINDOW:
    return handle_duel_inactive_cursor(hwnd, msg, wparam, lparam);

  case WM_SETCURSOR:
    PostMessageA(g_duel_prompt_context_hwnd, 0x403, 0, 0);
    return DefWindowProcA(hwnd, msg, wparam, lparam);

  case WM_SIZE:
    GetClientRect(hwnd, &client_rect);
    if (g_spell_chain_scrollbar_thumb_bitmap == (HBITMAP)0)
    {
      scrollbar_height = GetSystemMetrics(SM_CYHSCROLL) * 2;
    }
    else
    {
      GetObjectA(g_spell_chain_scrollbar_thumb_bitmap, sizeof(bitmap), &bitmap);
      if (GetSystemMetrics(SM_CYHSCROLL) * 2 < bitmap.bmHeight)
      {
        scrollbar_height = bitmap.bmHeight;
      }
      else
      {
        scrollbar_height = GetSystemMetrics(SM_CYHSCROLL) * 2;
      }
    }
    scrollbar_y = client_rect.bottom - scrollbar_height;
    MoveWindow(GetDlgItem(hwnd, 0), 0, scrollbar_y, client_rect.right, scrollbar_height, TRUE);
    return 0;

  case WM_SYSCOMMAND:
    if ((wparam & 0xfff0) == SC_MOVE)
    {
      return 0;
    }
    return DefWindowProcA(hwnd, msg, wparam, lparam);

  case WM_NCRBUTTONDOWN:
  case WM_RBUTTONDOWN:
    popup_point.x = LOWORD(lparam);
    popup_point.y = HIWORD(lparam);
    if (msg == WM_RBUTTONDOWN)
    {
      ClientToScreen(hwnd, &popup_point);
    }
    SetRect(&popup_rect, popup_point.x, popup_point.y, popup_point.x + 1, popup_point.y + 1);
    TrackPopupMenu(g_spell_minimized_popup_menu, TPM_RIGHTBUTTON, popup_point.x, popup_point.y, 0, hwnd, &popup_rect);
    return 0;

  case WM_INITMENU:
    AppendMenuA(g_spell_minimized_popup_menu, 0, 0x65, g_spell_chain_menu_minimize_text);
    AppendMenuA(g_spell_minimized_popup_menu, 0, 100, g_spell_chain_menu_help_text);
    return 0;

  case WM_MENUSELECT:
    if (HIWORD(wparam) == 0xffff && lparam == 0)
    {
      menu_item_count = GetMenuItemCount(g_spell_minimized_popup_menu);
      while (menu_item_count != 0)
      {
        DeleteMenu(g_spell_minimized_popup_menu, 0, MF_BYPOSITION);
        --menu_item_count;
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

// FUNCTION: MAGIC 0x0048894d
// FUNCTION: SHANDALAR 0x004cb522
void layout_phase_display_window(HWND hwnd, LPRECT rect)
{
  spell_chain_window_entry_t *windows;
  int spell_gap_x;
  int scrollbar_height;
  int max_scroll_pos;
  int card_player_and_card[2];
  int top_spell_y;
  int target_x_step;
  int target_x;
  int bottom_spell_y;
  int target_gap_y;
  int window_height;
  int target_index;
  int window_width;
  int y;
  int entry_index;
  int x;
  int left_margin;
  int top_margin;
  LONG window_count;
  RECT content_rect;
  int min_scroll_pos;
  int target_y;
  int has_targets;
  RECT window_rect;
  RECT battlefield_rect;
  HWND scrollbar_hwnd;
  LRESULT scroll_pos;

  windows = (spell_chain_window_entry_t *)GetWindowLongPtrA(hwnd, g_spell_chain_windows_long_offset);
  window_count = GetWindowLongA(hwnd, g_spell_chain_count_long_offset);
  scrollbar_hwnd = GetDlgItem(hwnd, 0);
  scroll_pos = SendMessageA(scrollbar_hwnd, 0xe1, 0, 0);
  if (window_count == 0)
  {
    if (IsWindowVisible(hwnd) != 0 || IsWindowVisible(g_spell_minimized_hwnd) != 0)
    {
      ShowWindow(hwnd, SW_HIDE);
      ShowWindow(g_spell_minimized_hwnd, SW_HIDE);
      UpdateWindow(g_duel_help_owner_hwnd);
    }
    if (rect != (LPRECT)0)
    {
      SetRect(rect, 0, 0, 0, 0);
    }
  }
  else
  {
    GetWindowRect(scrollbar_hwnd, &window_rect);
    scrollbar_height = window_rect.bottom - window_rect.top;
    left_margin = 5;
    top_margin = 5;
    spell_gap_x = 5;
    target_gap_y = 5;
    target_x_step = g_showlist_smallcard_width / 4;
    has_targets = 0;
    for (entry_index = 0; entry_index < window_count; entry_index++)
    {
      if (windows[entry_index].target_count != 0)
      {
        has_targets = 1;
      }
    }
    SetRect(&content_rect, 500, 500, 0, 0);
    x = left_margin;
    top_spell_y = top_margin;
    bottom_spell_y = top_spell_y + g_showlist_smallcard_height / 2 + g_showlist_smallcard_height;
    for (entry_index = 0; entry_index < window_count; entry_index++)
    {
      SendMessageA(windows[entry_index].spell_window, 0x401, (WPARAM)card_player_and_card, 0);
      if (card_player_and_card[0] == 0)
      {
        y = bottom_spell_y;
        target_y = (y - g_showlist_smallcard_height) - target_gap_y;
      }
      else
      {
        y = top_spell_y;
        target_y = g_showlist_smallcard_height + y + target_gap_y;
      }
      MoveWindow(windows[entry_index].spell_window, x, y,
                 g_showlist_smallcard_width, g_showlist_smallcard_height, TRUE);
      if (content_rect.left > x)
      {
        content_rect.left = x;
      }
      if (content_rect.right < g_showlist_smallcard_width + x)
      {
        content_rect.right = g_showlist_smallcard_width + x;
      }
      if (content_rect.top > y)
      {
        content_rect.top = y;
      }
      if (content_rect.bottom < g_showlist_smallcard_height + y)
      {
        content_rect.bottom = g_showlist_smallcard_height + y;
      }
      target_x = x;
      for (target_index = 0; target_index < windows[entry_index].target_count; target_index++)
      {
        MoveWindow(windows[entry_index].target_windows[target_index], target_x, target_y,
                   g_showlist_smallcard_width, g_showlist_smallcard_height, TRUE);
        if (content_rect.left > target_x)
        {
          content_rect.left = target_x;
        }
        if (content_rect.right < target_x + g_showlist_smallcard_width)
        {
          content_rect.right = target_x + g_showlist_smallcard_width;
        }
        if (content_rect.top > target_y)
        {
          content_rect.top = target_y;
        }
        if (content_rect.bottom < target_y + g_showlist_smallcard_height)
        {
          content_rect.bottom = target_y + g_showlist_smallcard_height;
        }
        target_x = target_x + target_x_step;
      }
      x = x + g_showlist_smallcard_width + spell_gap_x;
      if (windows[entry_index].target_count != 0)
      {
        x = x + (windows[entry_index].target_count - 1) * target_x_step;
      }
    }
    x -= -(-spell_gap_x);
    x += left_margin;
    content_rect.right = content_rect.right + content_rect.left;
    content_rect.left = 0;
    content_rect.top = 0;
    content_rect.bottom = content_rect.bottom + top_margin;
    window_rect.left = 0;
    window_rect.right = x - spell_gap_x + left_margin;
    window_rect.top = 0;
    window_rect.bottom = top_margin + top_margin + g_showlist_smallcard_height + bottom_spell_y + scrollbar_height;
    AdjustWindowRect(&window_rect, GetWindowLongA(hwnd, GWL_STYLE), FALSE);
    GetWindowRect(g_duel_player_battlefield_window_hwnd, &battlefield_rect);
    x = battlefield_rect.left;
    y = max(0, battlefield_rect.top - (window_rect.bottom - window_rect.top));
    window_width = min(window_rect.right - window_rect.left, battlefield_rect.right - battlefield_rect.left);
    window_height = window_rect.bottom - window_rect.top;
    MoveWindow(hwnd, x, y, window_width, window_height, TRUE);
    scrollbar_hwnd = GetDlgItem(hwnd, 0);
    if (window_width < window_rect.right - window_rect.left)
    {
      min_scroll_pos = 0;
      max_scroll_pos = (window_rect.right - window_rect.left) - window_width;
      SendMessageA(scrollbar_hwnd, 0xe2, min_scroll_pos, max_scroll_pos);
      SendMessageA(scrollbar_hwnd, 0x468, 1, 0);
    }
    else
    {
      SendMessageA(scrollbar_hwnd, 0x468, 0, 0);
    }
    SendMessageA(scrollbar_hwnd, 0xe3, (WPARAM)&min_scroll_pos, (LPARAM)&max_scroll_pos);
    if (min_scroll_pos > scroll_pos)
    {
      scroll_pos = min_scroll_pos;
    }
    if (scroll_pos > max_scroll_pos)
    {
      scroll_pos = max_scroll_pos;
    }
    SendMessageA(scrollbar_hwnd, 0xe0, 0, 0);
    SendMessageA(scrollbar_hwnd, WM_HSCROLL, MAKELONG(SB_THUMBPOSITION, scroll_pos), (LPARAM)scrollbar_hwnd);
    if (IsWindowVisible(hwnd) == 0 && IsWindowVisible(g_spell_minimized_hwnd) == 0)
    {
      ShowWindow(hwnd, SW_SHOW);
      restack_duel_child_windows();
    }
    UpdateWindow(hwnd);
    if (rect != (LPRECT)0)
    {
      CopyRect(rect, &content_rect);
    }
  }
}
