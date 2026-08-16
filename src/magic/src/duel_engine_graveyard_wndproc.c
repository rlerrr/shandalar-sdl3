#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "cardartlib/src/palette.h"
#include "drawcardlib/Drawcardlib.h"
#include "drawcardlib/src/pic.h"
#include "magic/src/duel_engine.h"
#include "magic/src/game_support.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "magic/src/shared_startup.h"

#ifndef SHANDALAR
extern HWND global_main_hwnd;
#define DUEL_SHELL_WINDOW_HWND global_main_hwnd
#else
#define DUEL_SHELL_WINDOW_HWND g_main_window_hwnd
#endif

#define CLASS_EXPANDED_GRAVEYARD "ExpandedGraveyard"
#define CLASS_GRAVEYARD_CARDS "GraveyardCards"

extern card_ptr_t global_raw_cards_storage[2000];
extern char global_base_directory[];
extern int g_duel_cached_graveyard_player_0[500];
extern int g_duel_cached_graveyard_player_1[500];
extern int g_duel_cached_exile_player_0[500];
extern int g_duel_cached_exile_player_1[500];
extern int g_duel_cached_ante_player_1[16];
extern int g_duel_cached_ante_player_0[16];
extern int g_duel_cached_graveyard_count_player_0;
extern int g_duel_cached_graveyard_count_player_1;
extern int g_duel_cached_exile_count_player_0;
extern int g_duel_cached_exile_count_player_1;
extern int g_duel_cached_ante_count_player_1;
extern int g_duel_cached_ante_count_player_0;
extern int is_invalid_duel_player(int player);
extern LRESULT handle_duel_inactive_cursor(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
void delete_and_close_object(HANDLE obj);
int load_text_with_tab_escapes(char *filename, char *section_name);
LRESULT CALLBACK wndproc_ExpandedGraveyard(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK wndproc_GraveyardCards(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
LRESULT CALLBACK wndproc_MAGICGAME_GraveyardClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
INT_PTR CALLBACK dlgproc_ViewAntes(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

// GLOBAL: MAGIC 0x0055e00c
int g_graveyard_window_long_offset = 0;

// GLOBAL: MAGIC 0x0055e010
// GLOBAL: SHANDALAR 0x0057f10c
int g_graveyard_card_window_long_offset = 0;

// GLOBAL: MAGIC 0x0055e014
// GLOBAL: SHANDALAR 0x0057f110
int g_graveyard_expanded_window_long_offset = 4;

// GLOBAL: MAGIC 0x0055e018
// GLOBAL: SHANDALAR 0x0057f114
int g_graveyard_bitmap_window_long_offset = 8;

// GLOBAL: MAGIC 0x0055e01c
int g_graveyard_window_extra_bytes = 0xc;

// GLOBAL: MAGIC 0x0055e020
int g_expanded_graveyard_window_extra_bytes = 0;

// GLOBAL: MAGIC 0x0055e024
// GLOBAL: SHANDALAR 0x0057f120
int g_graveyard_card_id_window_long_offset = 0;

// GLOBAL: MAGIC 0x0055e028
int g_graveyard_cards_window_extra_bytes = 4;

// GLOBAL: MAGIC 0x00637918
HBITMAP g_view_antes_background_bitmap;

// GLOBAL: MAGIC 0x0063791c
COLORREF g_view_antes_text_color;

// GLOBAL: MAGIC 0x00637958
// GLOBAL: SHANDALAR 0x00603a90
char g_graveyard_menu_view_text[0x34];

// GLOBAL: MAGIC 0x0063798c
HBITMAP g_view_antes_label_bitmap;

// GLOBAL: MAGIC 0x00637990
// GLOBAL: SHANDALAR 0x00603ac8
char g_graveyard_menu_help_text[0x1c];

// GLOBAL: MAGIC 0x006379ac
// GLOBAL: SHANDALAR 0x00603ae4
HMENU g_graveyard_popup_menu;

// GLOBAL: MAGIC 0x006379c0
// GLOBAL: SHANDALAR 0x00603af8
char g_graveyard_view_antes_opponent_text[0x68];

// GLOBAL: MAGIC 0x00637920
// GLOBAL: SHANDALAR 0x00603a58
char g_graveyard_view_antes_player_text[0x38];

// GLOBAL: MAGIC 0x00637a28
// GLOBAL: SHANDALAR 0x00603b60
char g_graveyard_menu_view_antes_text[0x38];

// GLOBAL: MAGIC 0x00637a60
// GLOBAL: SHANDALAR 0x00603b98
char g_graveyard_menu_view_exile_text[0x34];

extern int g_graveyard_card_window_long_offset;
extern int g_graveyard_expanded_window_long_offset;
extern int g_graveyard_bitmap_window_long_offset;
extern int g_graveyard_card_id_window_long_offset;

// FUNCTION: MAGIC 0x00449ea5
// FUNCTION: SHANDALAR 0x00453a26
int copy_cached_graveyard_cards_and_get_count(void *cards, int player)
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
    result = g_duel_cached_graveyard_count_player_0;
  }
  else
  {
    result = g_duel_cached_graveyard_count_player_1;
  }
  memcpy(cards, (player == 0) ? g_duel_cached_graveyard_player_0 : g_duel_cached_graveyard_player_1, 2000);
  LeaveCriticalSection(&g_duel_render_lock);

  return result;
}

// FUNCTION: MAGIC 0x00449f48
// FUNCTION: SHANDALAR 0x00453ac9
int copy_cached_exile_cards_and_get_count(void *cards, int player)
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
    result = g_duel_cached_exile_count_player_0;
  }
  else
  {
    result = g_duel_cached_exile_count_player_1;
  }
  memcpy(cards, (player == 0) ? g_duel_cached_exile_player_0 : g_duel_cached_exile_player_1, 2000);
  LeaveCriticalSection(&g_duel_render_lock);

  return result;
}

// FUNCTION: MAGIC 0x00450a70
// FUNCTION: SHANDALAR 0x0050d5d0
int register_MAGICGAME_GraveyardClass(LPCSTR class_name)
{
  ATOM atom1;
  ATOM atom2;
  ATOM atom3;
  WNDCLASSA wndclass;

  SET_DUEL_WNDCLASS(wndclass, 0xb, wndproc_MAGICGAME_GraveyardClass, g_graveyard_window_extra_bytes,
                    LoadIconA((HINSTANCE)0, (LPCSTR)0x7f00), (HBRUSH)GetStockObject(4), class_name);
  atom1 = RegisterClassA(&wndclass);
  SET_DUEL_WNDCLASS(wndclass, 8, wndproc_ExpandedGraveyard, g_expanded_graveyard_window_extra_bytes,
                    LoadIconA((HINSTANCE)0, (LPCSTR)0x7f00), (HBRUSH)GetStockObject(4), CLASS_EXPANDED_GRAVEYARD);
  atom2 = RegisterClassA(&wndclass);
  SET_DUEL_WNDCLASS(wndclass, 0xb, wndproc_GraveyardCards, g_graveyard_cards_window_extra_bytes,
                    LoadIconA((HINSTANCE)0, (LPCSTR)0x7f00), (HBRUSH)0x6, CLASS_GRAVEYARD_CARDS);
  atom3 = RegisterClassA(&wndclass);
  g_graveyard_popup_menu = CreatePopupMenu();
  load_text_with_tab_escapes(global_ui_strings_filename, "MENU_GRAVEYARD");
  strcpy(g_graveyard_menu_view_text, g_text_lines[0]);
  strcpy(g_graveyard_menu_view_exile_text, g_text_lines[1]);
  strcpy(g_graveyard_menu_view_antes_text, g_text_lines[2]);
  strcpy(g_graveyard_menu_help_text, g_text_lines[3]);
  load_text_with_tab_escapes(global_ui_strings_filename, "DIALOG_VIEWANTES");
  strcpy(g_graveyard_view_antes_opponent_text, g_text_lines[0]);
  strcpy(g_graveyard_view_antes_player_text, g_text_lines[1]);
  return atom1 != 0 && atom2 != 0 && atom3 != 0;
}

// FUNCTION: MAGIC 0x00451fd8
// FUNCTION: SHANDALAR 0x0050eb28
int get_cached_top_graveyard_card_id(int player)
{
  int cards[500];
  int count;

  count = copy_cached_graveyard_cards_and_get_count(cards, player);
  if (count == 0)
  {
    return -1;
  }

  return cards[count - 1];
}

// FUNCTION: MAGIC 0x00451fc3
// FUNCTION: SHANDALAR 0x0050eb13
void destroy_expanded_graveyard_window(HWND hwnd)
{
  DestroyWindow(hwnd);
}

// FUNCTION: MAGIC 0x00451bf9
// FUNCTION: SHANDALAR 0x0050e749
HWND create_expanded_graveyard_window(HWND hwnd, int show_graveyard)
{
  int row_count;
  int card_x;
  int created_card_count;
  int card_width;
  int x_step;
  int card_count;
  int all_cards_created;
  int card_index;
  WPARAM cards[500];
  int x_pad;
  int y_pad;
  int cards_per_row;
  int card_y;
  HWND card_window;
  HWND expanded_window;
  int card_height;
  unsigned int player;
  RECT client_rect;
  RECT expanded_rect;

  GetWindowRect(hwnd, &client_rect);
  expanded_rect.left = client_rect.left;
  expanded_rect.top = client_rect.top;
  GetClientRect(g_duel_window_hwnd, &client_rect);
  expanded_rect.right = (client_rect.right * 0x4b) / 100;
  expanded_rect.bottom = client_rect.bottom;
  GetClientRect(hwnd, &client_rect);
  card_width = client_rect.right;
  card_height = client_rect.bottom;
  x_step = (card_width * 0x3c) / 100;
  x_pad = 5;
  y_pad = 5;
  cards_per_row = (((expanded_rect.right - expanded_rect.left) - x_pad * 2) - card_width) / x_step + 1;
  player = hwnd != g_duel_player_graveyard_window_hwnd;
  expanded_window = CreateWindowExA(0, CLASS_EXPANDED_GRAVEYARD,
                                    show_graveyard ? "Graveyard list" : "Out-of-play list",
                                    WS_POPUP, 0, 0, 0, 0, g_duel_window_hwnd,
                                    (HMENU)0, g_app_instance, (LPVOID)0);
  if (expanded_window == (HWND)0)
  {
    return (HWND)0;
  }

  if (show_graveyard != 0)
  {
    SetClassLongA(expanded_window, GCL_HBRBACKGROUND, (LONG)GetStockObject(BLACK_BRUSH));
  }
  else
  {
    SetClassLongA(expanded_window, GCL_HBRBACKGROUND, (LONG)GetStockObject(WHITE_BRUSH));
  }

  all_cards_created = 1;
  card_x = x_pad;
  card_y = y_pad;
  if (show_graveyard != 0)
  {
    card_count = copy_cached_graveyard_cards_and_get_count(cards, player);
  }
  else
  {
    card_count = copy_cached_exile_cards_and_get_count(cards, player);
  }
  card_index = card_count - 1;
  created_card_count = 0;
  for (; card_index >= 0; --card_index)
  {
    card_window = CreateWindowExA(0, CLASS_GRAVEYARD_CARDS,
                                  show_graveyard ? "Graveyard card" : "Out-of-play card",
                                  WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                  card_x, card_y, card_width, card_height,
                                  expanded_window, (HMENU)1, g_app_instance, (LPVOID)0);
    if (card_window != (HWND)0)
    {
      created_card_count++;
      SendMessageA(card_window, 0x401, cards[card_index], 0);
      card_x += x_step;
      if (created_card_count % cards_per_row == 0)
      {
        card_x = x_pad;
        card_y += card_height + y_pad;
      }
    }
    else
    {
      all_cards_created = 0;
    }
  }

  if (card_count != 0)
  {
    BringWindowToTop(expanded_window);
    expanded_rect.right = (card_count - 1 < cards_per_row - 1 ? card_count - 1 : cards_per_row - 1) *
                              x_step +
                          x_pad * 2 + expanded_rect.left + card_width;
    row_count = card_count / cards_per_row;
    if (card_count % cards_per_row != 0)
    {
      row_count++;
    }
    expanded_rect.bottom =
        (card_height + y_pad) * (row_count - 1) + expanded_rect.top + y_pad * 2 + card_height;
    GetClientRect(g_duel_window_hwnd, &client_rect);
    if (client_rect.bottom < expanded_rect.bottom)
    {
      OffsetRect(&expanded_rect, 0, -(expanded_rect.bottom - client_rect.bottom));
    }
    MoveWindow(expanded_window, expanded_rect.left, expanded_rect.top,
               expanded_rect.right - expanded_rect.left,
               expanded_rect.bottom - expanded_rect.top, 1);
    ShowWindow(expanded_window, SW_SHOW);
  }
  else
  {
    DestroyWindow(expanded_window);
    expanded_window = (HWND)0;
  }
  return expanded_window;
}

// FUNCTION: MAGIC 0x0044a0e9
void copy_cached_ante_cards(int *opponent_antes, int *opponent_count, int *player_antes, int *player_count)
{
  int index;

  if (opponent_antes == NULL || opponent_count == NULL || player_antes == NULL || player_count == NULL)
    return;

  EnterCriticalSection(&g_duel_render_lock);
  *opponent_count = g_duel_cached_ante_count_player_1;
  for (index = 0; index < g_duel_cached_ante_count_player_1; index++)
  {
    opponent_antes[index] = CardIDFromType(g_duel_cached_ante_player_1[index]);
  }
  *player_count = g_duel_cached_ante_count_player_0;
  for (index = 0; index < g_duel_cached_ante_count_player_0; index++)
  {
    player_antes[index] = CardIDFromType(g_duel_cached_ante_player_0[index]);
  }
  LeaveCriticalSection(&g_duel_render_lock);
}

// FUNCTION: MAGIC 0x00452a75
void get_view_antes_card_rect(RECT *rect, HWND hwnd, int opponent, int index)
{
  int opponent_antes[16];
  int player_antes[16];
  int card_width;
  int x_step;
  RECT opponent_rect;
  RECT player_rect;
  int player_count;
  int opponent_count;
  RECT client_rect;

  copy_cached_ante_cards(opponent_antes, &opponent_count, player_antes, &player_count);
  GetWindowRect(GetDlgItem(hwnd, 0x4b3), &opponent_rect);
  MapWindowPoints(NULL, hwnd, (LPPOINT)&opponent_rect, 2);
  GetWindowRect(GetDlgItem(hwnd, 0x4b0), &player_rect);
  MapWindowPoints(NULL, hwnd, (LPPOINT)&player_rect, 2);
  card_width = opponent_rect.right - opponent_rect.left;
  GetClientRect(hwnd, &client_rect);
  client_rect.left = opponent_rect.left;

  if (opponent == 0)
  {
    if (player_count < 1 || player_count <= index)
    {
      SetRect(rect, 0, 0, 0, 0);
    }
    else
    {
      x_step = (card_width * 60) / 100;
      while (client_rect.right - client_rect.left < (player_count - 1) * x_step + card_width &&
             (card_width * 10) / 100 < x_step)
      {
        --x_step;
      }
      CopyRect(rect, &player_rect);
      OffsetRect(rect, x_step * index, 0);
    }
  }
  else if (opponent_count < 1 || opponent_count <= index)
  {
    SetRect(rect, 0, 0, 0, 0);
  }
  else
  {
    x_step = (card_width * 60) / 100;
    while (client_rect.right - client_rect.left < (opponent_count - 1) * x_step + card_width &&
           (card_width * 10) / 100 < x_step)
    {
      --x_step;
    }
    CopyRect(rect, &opponent_rect);
    OffsetRect(rect, x_step * index, 0);
  }
}

// FUNCTION: MAGIC 0x00452059
INT_PTR CALLBACK dlgproc_ViewAntes(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  int opponent_antes[16];
  int player_antes[16];
  HDC paint_dc;
  PAINTSTRUCT paint;
  int player_count;
  int index;
  RECT card_rect;
  int opponent_count;
  int card_id;
  HDC erase_dc;
  HGDIOBJ font;
  RECT text_rect;
  RECT client_rect;
  char text[100];
  HWND ctl_hwnd;
  int ctl_id;
  HDC ctl_dc;
  WPARAM hover_opponent_antes[16];
  WPARAM hover_player_antes[16];
  POINT mouse_point;
  int hover_player_count;
  int hover_index;
  RECT hover_rect;
  int hover_opponent_count;
  WPARAM hover_card_id;
  HDC measure_dc;
  int label_height;
  int label_width;
  HGDIOBJ label_font;
  SIZE label_size;
  char path[264];
  char label_text[200];
  char opponent_name[100];
  RECT window_rect;

  switch (msg)
  {
  case WM_INITDIALOG:
    ShowWindow(GetDlgItem(hwnd, 0x4b3), 0);
    ShowWindow(GetDlgItem(hwnd, 0x4b0), 0);
    ShowWindow(GetDlgItem(hwnd, 0x4b1), 0);
    ShowWindow(GetDlgItem(hwnd, 0x4b2), 0);
    sprintf(path, "%s\\WINBK_Ante.pic", global_duelart_path);
    g_view_antes_background_bitmap = load_pic(path);
    sprintf(path, "%s\\WINBK_AnteLabel.pic", global_duelart_path);
    g_view_antes_label_bitmap = load_pic(path);
    g_view_antes_text_color = 0;
    copy_opponent_name_prefix(opponent_name);
    sprintf(label_text, g_graveyard_view_antes_opponent_text, opponent_name);
    SetDlgItemTextA(hwnd, 0x4b1, label_text);
    strcpy(label_text, g_graveyard_view_antes_player_text);
    SetDlgItemTextA(hwnd, 0x4b2, label_text);
    measure_dc = GetDC(hwnd);
    ApplyCardArtPaletteToDc(measure_dc);
    label_font = (HGDIOBJ)SendDlgItemMessageA(hwnd, 0x4b1, WM_GETFONT, 0, 0);
    SelectObject(measure_dc, label_font);
    GetDlgItemTextA(hwnd, 0x4b1, label_text, 200);
    GetTextExtentPoint32A(measure_dc, label_text, strlen(label_text), &label_size);
    label_width = label_size.cx + label_size.cy;
    label_height = (label_size.cy * 3) / 2;
    SetWindowPos(GetDlgItem(hwnd, 0x4b1), NULL, 0, 0, label_width, label_height, SWP_NOZORDER | SWP_NOMOVE);
    SetWindowPos(GetDlgItem(hwnd, 0x4b2), NULL, 0, 0, label_width, label_height, SWP_NOZORDER | SWP_NOMOVE);
    ReleaseDC(hwnd, measure_dc);
    GetWindowRect(hwnd, &window_rect);
    SetWindowPos(hwnd, NULL, (GetSystemMetrics(SM_CXSCREEN) * 20) / 100, window_rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    SetFocus(hwnd);
    return 0;

  case WM_KEYDOWN:
  case WM_COMMAND:
    delete_and_close_object(g_view_antes_background_bitmap);
    delete_and_close_object(g_view_antes_label_bitmap);
    EndDialog(hwnd, 0);
    return 1;

  case WM_MOUSEMOVE:
  case WM_RBUTTONDOWN:
    copy_cached_ante_cards((int *)hover_opponent_antes, &hover_opponent_count, (int *)hover_player_antes, &hover_player_count);
    mouse_point.x = (unsigned int)lparam & 0xffff;
    mouse_point.y = (unsigned short)(((unsigned int)lparam >> 16) & 0xffff);
    if ((msg == WM_MOUSEMOVE && g_duel_interface_options.layout != 2) || (msg == WM_RBUTTONDOWN && g_duel_interface_options.layout == 2))
    {
      hover_card_id = 0xffffffff;
      if (hover_player_count != 0)
      {
        for (hover_index = hover_player_count - 1; hover_index >= 0 && hover_card_id == 0xffffffff; --hover_index)
        {
          get_view_antes_card_rect(&hover_rect, hwnd, 0, hover_index);
          if (PtInRect(&hover_rect, mouse_point) != 0)
          {
            hover_card_id = hover_player_antes[hover_index];
          }
        }
      }
      if (hover_opponent_count != 0)
      {
        for (hover_index = hover_opponent_count - 1; hover_index >= 0 && hover_card_id == 0xffffffff; --hover_index)
        {
          get_view_antes_card_rect(&hover_rect, hwnd, 1, hover_index);
          if (PtInRect(&hover_rect, mouse_point) != 0)
          {
            hover_card_id = hover_opponent_antes[hover_index];
          }
        }
      }
      if (hover_card_id != 0xffffffff)
      {
        SendMessageA(g_duel_card_preview_window_hwnd, 0x401, hover_card_id, 0);
      }
    }
    return 0;

  case WM_LBUTTONDOWN:
    delete_and_close_object(g_view_antes_background_bitmap);
    delete_and_close_object(g_view_antes_label_bitmap);
    EndDialog(hwnd, 0);
    return 1;

  case WM_CTLCOLORSTATIC:
    ctl_dc = (HDC)wparam;
    ApplyCardArtPaletteToDc(ctl_dc);
    ctl_hwnd = (HWND)lparam;
    ctl_id = GetDlgCtrlID(ctl_hwnd);
    SetBkMode(ctl_dc, TRANSPARENT);
    SetTextColor(ctl_dc, g_view_antes_text_color);
    return (INT_PTR)GetStockObject(NULL_BRUSH);

  case WM_QUERYNEWPALETTE:
  case WM_PALETTECHANGED:
  case WM_PALETTEISCHANGING:
    return handle_button_palette_message((int)hwnd, msg, (int)wparam, lparam);

  case WM_ERASEBKGND:
    erase_dc = (HDC)wparam;
    ApplyCardArtPaletteToDc(erase_dc);
    GetClientRect(hwnd, &client_rect);
    font = (HGDIOBJ)SendDlgItemMessageA(hwnd, 0x4b1, WM_GETFONT, 0, 0);
    SelectObject(erase_dc, font);
    SetTextColor(erase_dc, 0);
    SetBkMode(erase_dc, TRANSPARENT);
    if (g_view_antes_background_bitmap != NULL)
    {
      DrawBitmapToRect(erase_dc, &client_rect, g_view_antes_background_bitmap);
    }
    else
    {
      FillRect(erase_dc, &client_rect, GetStockObject(2));
    }
    GetWindowRect(GetDlgItem(hwnd, 0x4b1), &text_rect);
    MapWindowPoints(NULL, hwnd, (LPPOINT)&text_rect, 2);
    DrawBitmapToRect(erase_dc, &text_rect, g_view_antes_label_bitmap);
    GetDlgItemTextA(hwnd, 0x4b1, text, 100);
    text_rect.left += (text_rect.bottom - text_rect.top) / 4;
    DrawTextA(erase_dc, text, -1, &text_rect, DT_SINGLELINE | DT_VCENTER);
    GetWindowRect(GetDlgItem(hwnd, 0x4b2), &text_rect);
    MapWindowPoints(NULL, hwnd, (LPPOINT)&text_rect, 2);
    DrawBitmapToRect(erase_dc, &text_rect, g_view_antes_label_bitmap);
    GetDlgItemTextA(hwnd, 0x4b2, text, 100);
    text_rect.left += (text_rect.bottom - text_rect.top) / 4;
    DrawTextA(erase_dc, text, -1, &text_rect, DT_SINGLELINE | DT_VCENTER);
    return 1;

  case WM_PAINT:
    paint_dc = BeginPaint(hwnd, &paint);
    if (paint_dc != NULL)
    {
      ApplyCardArtPaletteToDc(paint_dc);
      copy_cached_ante_cards(opponent_antes, &opponent_count, player_antes, &player_count);
      if (opponent_count != 0)
      {
        for (index = 0; index < opponent_count; ++index)
        {
          get_view_antes_card_rect(&card_rect, hwnd, 1, index);
          card_id = opponent_antes[index];
          DrawFullCard(paint_dc, &card_rect, global_raw_cards_storage + card_id, 0, 0x12, 0,
                       gs_illus_00789130);
        }
      }
      if (player_count != 0)
      {
        for (index = 0; player_count > index; ++index)
        {
          get_view_antes_card_rect(&card_rect, hwnd, 0, index);
          card_id = player_antes[index];
          DrawFullCard(paint_dc, &card_rect, global_raw_cards_storage + card_id, 0, 0x12, 0,
                       gs_illus_00789130);
        }
      }
      EndPaint(hwnd, &paint);
    }
    return 1;

  default:
    return 0;
  }
}

// FUNCTION: MAGIC 0x00452030
// FUNCTION: SHANDALAR 0x0050eb80
void show_view_antes_dialog(void)
{
  DialogBoxParamA(g_app_instance, (LPCSTR)0xeb, g_duel_window_hwnd, dlgproc_ViewAntes, 0);
}

// FUNCTION: MAGIC 0x00451926
// FUNCTION: SHANDALAR 0x0050e476
LRESULT CALLBACK wndproc_GraveyardCards(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  HDC dc;
  PAINTSTRUCT paint;
  RECT client_rect;
  int card_id;

  switch (msg)
  {

  case 0x437:
    card_id = GetWindowLongA(hwnd, g_graveyard_card_id_window_long_offset);
    if (g_duel_interface_options.layout != 2 || IsWindowVisible(g_duel_card_preview_window_hwnd) != 0)
    {
      SendMessageA(g_duel_card_preview_window_hwnd, 0x401, card_id, 0);
    }
    return 0;

  case 0x400:
    return card_id = GetWindowLongA(hwnd, g_graveyard_card_id_window_long_offset);

  case 0x401:
    card_id = GetWindowLongA(hwnd, g_graveyard_card_id_window_long_offset);
    if (card_id != (int)wparam)
    {
      card_id = wparam;
      SetWindowLongA(hwnd, g_graveyard_card_id_window_long_offset, card_id);
      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;

  case WM_CREATE:
    card_id = -1;
    SetWindowLongA(hwnd, g_graveyard_card_id_window_long_offset, card_id);
    return 0;

  case WM_PAINT:
    card_id = GetWindowLongA(hwnd, g_graveyard_card_id_window_long_offset);
    GetClientRect(hwnd, &client_rect);
    EnterCriticalSection(&g_card_render_lock);
    FillRect(g_shared_offscreen_dc, &client_rect, GetStockObject(4));
    DrawFullCard(g_shared_offscreen_dc, &client_rect, global_raw_cards_storage + card_id, 0, 0x11, 0,
                 gs_illus_00789130);
    dc = BeginPaint(hwnd, &paint);
    if (dc != (HDC)0)
    {
      ApplyCardArtPaletteToDc(dc);
      BitBlt(dc, 0, 0, client_rect.right, client_rect.bottom,
             g_shared_offscreen_dc, 0, 0, SRCCOPY);
      EndPaint(hwnd, &paint);
    }
    LeaveCriticalSection(&g_card_render_lock);
    return 0;

  case WM_RBUTTONDBLCLK:
    card_id = GetWindowLongA(hwnd, g_graveyard_card_id_window_long_offset);
    if (g_duel_interface_options.layout == 2)
    {
      SendMessageA(g_duel_card_preview_window_hwnd, 0x401, card_id, 0);
    }
    return 0;

  case WM_QUERYNEWPALETTE:
  case WM_PALETTECHANGED:
  case WM_PALETTEISCHANGING:
    return handle_button_palette_message((int)hwnd, msg, (int)wparam, lparam);

  default:
    return DefWindowProcA(hwnd, msg, wparam, lparam);
  }

  return 0;
}

// FUNCTION: MAGIC 0x00450cd7
// FUNCTION: SHANDALAR 0x0050d837
LRESULT CALLBACK wndproc_MAGICGAME_GraveyardClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  int unused_padding_a70;
  int unused_padding_a6c;
  int unused_padding_a68;
  int menu_item_count;
  int cards[500];
  unsigned int player;
  MSG peek_msg;
  int show_popup;
  POINT popup_point;
  RECT popup_rect;
  HDC paint_dc;
  PAINTSTRUCT paint;
  RECT client_rect;
  int card_id;
  char help_path[264];
  DWORD help_context;
  unsigned int expand_mode;
  HWND expanded_window;
  int new_card_id;
  WPARAM hover_card_id;
  char cuecard_text[100];
  char opponent_name[100];
  int previous_card_id;
  HBITMAP background_bitmap;
  HWND captured_window;
  int unused_padding_4;
  int unused_padding_0;

  switch (msg)
  {
  case 0x437:
    if (hwnd == g_duel_player_graveyard_window_hwnd)
    {
      strcpy(cuecard_text, gs_cuecard_your_graveyard_0091c9b0);
    }
    else
    {
      copy_opponent_name_prefix(opponent_name);
      sprintf(cuecard_text, gs_cuecard_opponent_graveyard_0091c7b0, opponent_name);
    }
    strcpy((char *)wparam, cuecard_text);
    hover_card_id = get_cached_top_graveyard_card_id(hwnd != g_duel_player_graveyard_window_hwnd);
    if (hover_card_id != 0xffffffff &&
        (g_duel_interface_options.layout != 2 || IsWindowVisible(g_duel_card_preview_window_hwnd) != 0))
    {
      SendMessageA(g_duel_card_preview_window_hwnd, 0x401, hover_card_id, 0);
    }
    return 1;

  case 0x433:
  case 0x434:
    new_card_id = get_cached_top_graveyard_card_id(hwnd != g_duel_player_graveyard_window_hwnd);
    if ((int)wparam == new_card_id)
    {
      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;

  case 0x432:
    previous_card_id = GetWindowLongA(hwnd, g_graveyard_card_window_long_offset);
    new_card_id = get_cached_top_graveyard_card_id(hwnd != g_duel_player_graveyard_window_hwnd);
    SendMessageA(hwnd, 0x400, 0, 0);
    if (new_card_id != previous_card_id)
    {
      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;

  case 0x439:
    background_bitmap = (HBITMAP)GetWindowLongA(hwnd, g_graveyard_bitmap_window_long_offset);
    if (background_bitmap != (HBITMAP)0)
    {
      DeleteObject(background_bitmap);
    }
    background_bitmap = (HBITMAP)wparam;
    SetWindowLongA(hwnd, g_graveyard_bitmap_window_long_offset, (LONG)wparam);
    InvalidateRect(hwnd, NULL, TRUE);
    return 0;

  case 0x438:
    return GetWindowLongA(hwnd, g_graveyard_bitmap_window_long_offset);

  case 0x400:
    captured_window = (HWND)GetWindowLongA(hwnd, g_graveyard_expanded_window_long_offset);
    expanded_window = (HWND)wparam;
    expand_mode = lparam;
    if (wparam != 0)
    {
      if (captured_window == (HWND)0)
      {
        captured_window = create_expanded_graveyard_window(hwnd, lparam);
      }
      SetWindowLongA(hwnd, g_graveyard_expanded_window_long_offset, (LONG)captured_window);
      if (captured_window != (HWND)0)
      {
        SetCapture(captured_window);
      }
    }
    else if (captured_window != (HWND)0)
    {
      ReleaseCapture();
      destroy_expanded_graveyard_window(captured_window);
      captured_window = (HWND)0;
      SetWindowLongA(hwnd, g_graveyard_expanded_window_long_offset, 0);
    }
    return 0;

  case WM_COMMAND:
    switch ((UINT)wparam & 0xffff)
    {
    case 100:
      SendMessageA(hwnd, 0x400, 1, 1);
      break;
    case 0x65:
      SendMessageA(hwnd, 0x400, 1, 0);
      break;
    case 0x66:
      show_view_antes_dialog();
      break;
    case 0x67:
      help_context = 0x7e6;
      strcpy(help_path, global_base_directory);
      strcat(help_path, "\\duel.hlp");
      WinHelpA(g_duel_window_hwnd, help_path, HELP_CONTEXT, help_context);
      break;
    }
    return 0;

  case WM_CREATE:
    previous_card_id = -1;
    SetWindowLongA(hwnd, g_graveyard_card_window_long_offset, -1);
    captured_window = (HWND)0;
    SetWindowLongA(hwnd, g_graveyard_expanded_window_long_offset, 0);
    background_bitmap = (HBITMAP)0;
    SetWindowLongA(hwnd, g_graveyard_bitmap_window_long_offset, 0);
    return 0;

  case WM_DESTROY:
    background_bitmap = (HBITMAP)GetWindowLongA(hwnd, g_graveyard_bitmap_window_long_offset);
    if (background_bitmap != (HBITMAP)0)
    {
      delete_and_close_object(background_bitmap);
    }
    return 0;

  case WM_ERASEBKGND:
    return 1;

  case WM_LBUTTONDOWN:
    SendMessageA(hwnd, 0x400, 1, 1);
    return 0;

  case WM_PAINT:
    previous_card_id = GetWindowLongA(hwnd, g_graveyard_card_window_long_offset);
    background_bitmap = (HBITMAP)GetWindowLongA(hwnd, g_graveyard_bitmap_window_long_offset);
    card_id = get_cached_top_graveyard_card_id(hwnd != g_duel_player_graveyard_window_hwnd);
    if (card_id != previous_card_id)
    {
      InvalidateRect(hwnd, NULL, FALSE);
    }
    EnterCriticalSection(&g_card_render_lock);
    GetClientRect(hwnd, &client_rect);
    FillRect(g_shared_offscreen_dc, &client_rect, GetStockObject(4));
    if (card_id == -1)
    {
      if (background_bitmap != (HBITMAP)0)
      {
        DrawBitmapToRect(g_shared_offscreen_dc, &client_rect, background_bitmap);
      }
    }
    else
    {
      DrawFullCard(g_shared_offscreen_dc, &client_rect, global_raw_cards_storage + card_id, 0, 0x11, 0,
                   gs_illus_00789130);
    }
    paint_dc = BeginPaint(hwnd, &paint);
    if (paint_dc != (HDC)0)
    {
      ApplyCardArtPaletteToDc(paint_dc);
      if (g_duel_palette_refresh_pending != 0)
      {
        FillRect(paint_dc, &client_rect, GetStockObject(WHITE_BRUSH));
        Sleep(200);
      }
      BitBlt(paint_dc, 0, 0, client_rect.right, client_rect.bottom,
             g_shared_offscreen_dc, 0, 0, SRCCOPY);
      EndPaint(hwnd, &paint);
      previous_card_id = card_id;
      SetWindowLongA(hwnd, g_graveyard_card_window_long_offset, card_id);
    }
    LeaveCriticalSection(&g_card_render_lock);
    return 0;

  case WM_RBUTTONDOWN:
    show_popup = 1;
    if (g_duel_interface_options.layout == 2)
    {
      Sleep(GetDoubleClickTime());
      if (PeekMessageA(&peek_msg, hwnd, WM_RBUTTONDBLCLK, WM_RBUTTONDBLCLK, 0) != 0)
      {
        show_popup = 0;
      }
    }
    if (show_popup != 0)
    {
      popup_point.x = (unsigned int)lparam & 0xffff;
      popup_point.y = HIWORD(lparam);
      ClientToScreen(hwnd, &popup_point);
      SetRect(&popup_rect, popup_point.x, popup_point.y, popup_point.x + 1, popup_point.y + 1);
      TrackPopupMenu(g_graveyard_popup_menu, TPM_RIGHTBUTTON, popup_point.x, popup_point.y, 0, hwnd,
                     &popup_rect);
    }
    return 0;

  case WM_INITMENU:
    player = hwnd != g_duel_player_graveyard_window_hwnd;
    AppendMenuA(g_graveyard_popup_menu, MF_STRING, 100, g_graveyard_menu_view_text);
    if (copy_cached_graveyard_cards_and_get_count(cards, player) == 0)
    {
      EnableMenuItem(g_graveyard_popup_menu, 100, MF_GRAYED);
    }
    AppendMenuA(g_graveyard_popup_menu, MF_STRING, 0x65, g_graveyard_menu_view_exile_text);
    if (copy_cached_exile_cards_and_get_count(cards, player) == 0)
    {
      EnableMenuItem(g_graveyard_popup_menu, 0x65, MF_GRAYED);
    }
    AppendMenuA(g_graveyard_popup_menu, MF_STRING, 0x66, g_graveyard_menu_view_antes_text);
    AppendMenuA(g_graveyard_popup_menu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(g_graveyard_popup_menu, MF_STRING, 0x67, g_graveyard_menu_help_text);
    return 0;

  case WM_MENUSELECT:
    if (((UINT)wparam >> 16) == 0xffff && lparam == 0)
    {
      menu_item_count = GetMenuItemCount(g_graveyard_popup_menu);
      while (menu_item_count != 0)
      {
        DeleteMenu(g_graveyard_popup_menu, 0, MF_BYPOSITION);
        menu_item_count--;
      }
    }
    return 0;

  case WM_RBUTTONDBLCLK:
    hover_card_id = get_cached_top_graveyard_card_id(hwnd != g_duel_player_graveyard_window_hwnd);
    if (hover_card_id != 0xffffffff && g_duel_interface_options.layout == 2)
    {
      SendMessageA(g_duel_card_preview_window_hwnd, 0x401, hover_card_id, 0);
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
