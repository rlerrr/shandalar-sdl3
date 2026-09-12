#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cardartlib/src/palette.h"
#include "drawcardlib/src/pic.h"
#include "magic/src/duel_engine.h"
#include "magic/src/game_support.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "magic/src/shared_startup.h"

#define ATTACK_PHASE_DISPLAY_COMMAND_HELP 0x64
#define ATTACK_PHASE_DISPLAY_COMMAND_STOP_BASE 0x96
#define ATTACK_PHASE_DISPLAY_COMMAND_TOGGLE_BASE 0xc8
#define ATTACK_PHASE_DISPLAY_COMMAND_HELP_BASE 0xfa
#define ATTACK_PHASE_DISPLAY_BITMAP_WIDTH 0x2f8

#define ATTACK_PHASE_DISPLAY_PIC g_magicgame_attack_phase_display_pic
#define ATTACK_WINDOW_TITLE_HEIGHT g_attack_ui_card_spacing

#define ATTACK_MAX_CARDS_PER_GROUP 50

typedef struct attack_window_group
{
  int group_card;
  HWND attackers[ATTACK_MAX_CARDS_PER_GROUP];
  int attacker_count;
  HWND blockers[ATTACK_MAX_CARDS_PER_GROUP];
  int blocker_count;
} attack_window_group_t;

typedef struct msvc_bitmap
{
  LONG bmType;
  LONG bmWidth;
  LONG bmHeight;
  LONG bmWidthBytes;
  WORD bmPlanes;
  WORD bmBitsPixel;
  LPVOID bmBits;
} msvc_bitmap_t;

// GLOBAL: MAGIC 0x00571d20
// GLOBAL: SHANDALAR 0x00585928
int g_attack_phase_display_selection = -1;

// GLOBAL: MAGIC 0x0069c618
// GLOBAL: SHANDALAR 0x005a8fe0
int g_attack_phase_display_state;

// GLOBAL: MAGIC 0x0069c69c
// GLOBAL: SHANDALAR 0x005a9064
int g_attack_phase_display_width;

// GLOBAL: MAGIC 0x0069c6ac
// GLOBAL: SHANDALAR 0x005a9074
int g_attack_phase_display_height;

// GLOBAL: MAGIC 0x0069c6b0
// GLOBAL: SHANDALAR 0x005a9078
int g_attack_phase_display_flags;

// GLOBAL: MAGIC 0x00708828
// GLOBAL: SHANDALAR 0x00669660
target_selection_result_t g_attack_phase_display_click_packet;

// GLOBAL: MAGIC 0x00708838
// GLOBAL: SHANDALAR 0x00669670
target_selection_result_t g_attack_phase_display_menu_packet;

extern HWND g_attack_ui_tooltip_window;
extern char g_attack_ui_label_text[0x1c];
extern HBITMAP g_attack_ui_button_bitmap;
extern int g_attack_ui_card_spacing;
extern int g_attack_ui_layout_flags;
extern HWND g_duel_player_battlefield_window;
extern HWND g_duel_opponent_battlefield_window;
extern HWND g_attack_phase_window_hwnd;
extern int g_duel_selected_card_window;
extern char global_base_directory[];
extern char global_duelart_path[];
extern int g_duel_modal_action_active;
extern int g_battlefield_draw_placeholder_x[2];
extern int g_battlefield_draw_placeholder_y[2];
extern int g_showlist_smallcard_width;
extern int g_showlist_smallcard_height;
extern int g_attackclass_data_window_long_offset;
extern int g_attackclass_count_window_long_offset;

extern char g_phase_display_menu_phase_help_text[0x38];
extern int g_phase_display_selected_card_window_long_offset;
extern char g_phase_display_menu_toggle_text[0x38];
extern char g_phase_display_menu_help_text[0x1c];
extern HMENU g_phase_display_menu;
extern char g_phase_display_menu_stop_text[0x38];
extern int g_phase_display_window_extra_bytes;

void copy_phase_stop_flags(int *phase_flags, int player);
void get_current_duel_selection(int *selected_player, int *selected_card);
void get_phase_display_action_selection(int *player, int *phase);
void noop_phase_display_user_message(HWND hwnd, WPARAM wparam, LPARAM lparam);
void delete_and_close_object(HANDLE obj);
unsigned int refresh_duel_display_cache(void);
LRESULT handle_duel_inactive_cursor(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
HWND get_card_window_hidden_flag(HWND hwnd);
int card_window_matches_player_and_card(HWND hwnd, int *player_and_card);
card_id_t get_card_window_displayed_card_id(HWND hwnd);
int is_attack_phase_window_enabled(void);
void copy_opponent_name_prefix(char *name);
void restack_duel_child_windows(void);

static void hit_test_attack_phase_display(POINT *point, RECT *client_rect, int *phase);
static void get_attack_phase_display_phase_rect(RECT *rect, int phase, int width, int height);
static void draw_attack_phase_display_status_markers(HDC dc, RECT *client_rect);
static int count_attack_phase_hidden_descendants(HWND hwnd, HWND parent_card_hwnd);
static void set_attack_phase_window_title(HWND hwnd);

// FUNCTION: MAGIC 0x004954ef
// FUNCTION: SHANDALAR 0x00465f8a
int debug_draw_attack_phase_message(char *text, COLORREF color, HBRUSH brush)
{
  HDC dc;
  RECT rect;

  SetRect(&rect, 0, 0x23f, 0x8c, 0x26c);
  if (g_attack_phase_display_selection != -1)
  {

    dc = CreateDCA("DISPLAY", (LPCSTR)0, (LPCSTR)0, (DEVMODEA *)0);
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);
    FillRect(dc, &rect, brush);
    TextOutA(dc, rect.left + 5, rect.top + 5, text, strlen(text));
    DeleteDC(dc);
    Sleep(g_attack_phase_display_selection);
  }
  return 1;
}

// FUNCTION: MAGIC 0x00538854
// FUNCTION: SHANDALAR 0x0055d078
static void hit_test_attack_phase_display(POINT *point, RECT *client_rect, int *phase)
{
  POINT point_copy;
  RECT client_copy;
  RECT phase_rect;
  int found_phase;

  CopyRect(&client_copy, client_rect);
  point_copy.x = point->x;
  point_copy.y = point->y;
  found_phase = -1;

  get_attack_phase_display_phase_rect(&phase_rect, 0x15, client_copy.right, client_copy.bottom);
  if (PtInRect(&phase_rect, point_copy) != 0)
    found_phase = 0x15;
  get_attack_phase_display_phase_rect(&phase_rect, 0x16, client_copy.right, client_copy.bottom);
  if (PtInRect(&phase_rect, point_copy) != 0)
    found_phase = 0x16;
  get_attack_phase_display_phase_rect(&phase_rect, 0x17, client_copy.right, client_copy.bottom);
  if (PtInRect(&phase_rect, point_copy) != 0)
    found_phase = 0x17;
  get_attack_phase_display_phase_rect(&phase_rect, 0x18, client_copy.right, client_copy.bottom);
  if (PtInRect(&phase_rect, point_copy) != 0)
    found_phase = 0x18;
  get_attack_phase_display_phase_rect(&phase_rect, 0x19, client_copy.right, client_copy.bottom);
  if (PtInRect(&phase_rect, point_copy) != 0)
    found_phase = 0x19;
  get_attack_phase_display_phase_rect(&phase_rect, 0x1b, client_copy.right, client_copy.bottom);
  if (PtInRect(&phase_rect, point_copy) != 0)
    found_phase = 0x1b;
  get_attack_phase_display_phase_rect(&phase_rect, 0x1e, client_copy.right, client_copy.bottom);
  if (PtInRect(&phase_rect, point_copy) != 0)
    found_phase = 0x1e;

  *phase = found_phase;
}

// FUNCTION: MAGIC 0x00538a11
// FUNCTION: SHANDALAR 0x0055d235
static void get_attack_phase_display_phase_rect(RECT *rect, int phase, int width, int height)
{
  int phase_height;
  int top;

  if (phase == -1)
  {
    SetRect(rect, 0, 0, 0, 0);
    return;
  }

  phase_height = (height * 0x28) / ATTACK_PHASE_DISPLAY_BITMAP_WIDTH;

  if (phase == 0x15)
    top = (height * 2) / ATTACK_PHASE_DISPLAY_BITMAP_WIDTH;
  else if (phase == 0x16)
    top = (height * 0x2b) / ATTACK_PHASE_DISPLAY_BITMAP_WIDTH;
  else if (phase == 0x17)
    top = (height * 0x54) / ATTACK_PHASE_DISPLAY_BITMAP_WIDTH;
  else if (phase == 0x18)
    top = (height * 0x7d) / ATTACK_PHASE_DISPLAY_BITMAP_WIDTH;
  else if (phase == 0x19)
    top = (height * 0xa6) / ATTACK_PHASE_DISPLAY_BITMAP_WIDTH;
  else if (phase == 0x1a)
    top = (height * 0xcf) / ATTACK_PHASE_DISPLAY_BITMAP_WIDTH;
  else if (phase == 0x1b)
    top = (height * 0xcf) / ATTACK_PHASE_DISPLAY_BITMAP_WIDTH;
  else if (phase == 0x1e)
    top = (height * 0x121) / ATTACK_PHASE_DISPLAY_BITMAP_WIDTH;
  else
    top = -1;

  if (top == -1)
    SetRect(rect, 0, 0, 0, 0);
  else
    SetRect(rect, 0, top, width, phase_height + top);
}

// FUNCTION: MAGIC 0x00538bd2
// FUNCTION: SHANDALAR 0x0055d3f6
static void draw_attack_phase_display_status_markers(HDC dc, RECT *client_rect)
{
  int phase;
  int phase_flags[38];
  HGDIOBJ old_brush;
  RECT phase_rect;
  int selected_phase;
  RECT marker_rect;
  HBRUSH brush;
  int current_player;
  int selected_player;

  get_phase_display_action_selection(&selected_player, &selected_phase);
  if (selected_player == -1 || selected_phase == -1)
  {
    brush = CreateSolidBrush(0xff);
    old_brush = SelectObject(dc, brush);
    get_current_duel_selection(&current_player, (int *)0);
    copy_phase_stop_flags(phase_flags, current_player);
    for (phase = 0x15; phase <= 0x1e; phase++)
    {
      if (phase_flags[phase] != 0)
      {
        get_attack_phase_display_phase_rect(&phase_rect, phase, client_rect->right, client_rect->bottom);
        if (IsRectEmpty(&phase_rect) == 0)
        {
          CopyRect(&marker_rect, &phase_rect);
          marker_rect.left = marker_rect.right - (phase_rect.right - phase_rect.left) / 3;
          marker_rect.top = marker_rect.bottom -
                            ((phase_rect.bottom - phase_rect.top) *
                             (marker_rect.right - marker_rect.left)) /
                                (phase_rect.right - phase_rect.left);
          Ellipse(dc, marker_rect.left, marker_rect.top, marker_rect.right, marker_rect.bottom);
        }
      }
    }
    SelectObject(dc, old_brush);
    DeleteObject(brush);
  }

  if (selected_player != -1 && selected_phase != -1)
  {
    brush = CreateSolidBrush(0xff00);
    old_brush = SelectObject(dc, brush);
    for (phase = 0x15; phase <= 0x1e; phase++)
    {
      if (selected_phase == phase)
      {
        get_attack_phase_display_phase_rect(&phase_rect, phase, client_rect->right, client_rect->bottom);
        if (IsRectEmpty(&phase_rect) == 0)
        {
          CopyRect(&marker_rect, &phase_rect);
          marker_rect.right = marker_rect.left + (phase_rect.right - phase_rect.left) / 3;
          marker_rect.top = marker_rect.bottom -
                            ((phase_rect.bottom - phase_rect.top) *
                             (marker_rect.right - marker_rect.left)) /
                                (phase_rect.right - phase_rect.left);
          Ellipse(dc, marker_rect.left, marker_rect.top, marker_rect.right, marker_rect.bottom);
        }
      }
    }
    SelectObject(dc, old_brush);
    DeleteObject(brush);
  }
}

// FUNCTION: MAGIC 0x004d8b29
// FUNCTION: SHANDALAR 0x0046277f
static int count_attack_phase_hidden_descendants(HWND hwnd, HWND parent_card_hwnd)
{
  attack_window_group_t *groups;
  int group_count;
  int result;
  int card_index;
  int group_index;

  groups = (attack_window_group_t *)GetWindowLongPtrA(hwnd, g_attackclass_data_window_long_offset);
  group_count = GetWindowLongA(hwnd, g_attackclass_count_window_long_offset);
  result = 0;
  for (group_index = 0; group_count > group_index; group_index++)
  {
    for (card_index = 0; card_index < (groups + group_index)->attacker_count; card_index++)
    {
      if (get_card_window_hidden_flag((groups + group_index)->attackers[card_index]) == parent_card_hwnd)
      {
        result++;
        result += count_attack_phase_hidden_descendants(hwnd, (groups + group_index)->attackers[card_index]);
      }
    }
    for (card_index = 0; card_index < (groups + group_index)->blocker_count; card_index++)
    {
      if (get_card_window_hidden_flag((groups + group_index)->blockers[card_index]) == parent_card_hwnd)
      {
        result++;
        result += count_attack_phase_hidden_descendants(hwnd, (groups + group_index)->blockers[card_index]);
      }
    }
  }
  return result;
}

// FUNCTION: MAGIC 0x004d8468
// FUNCTION: SHANDALAR 0x004620c4
static void set_attack_phase_window_title(HWND hwnd)
{
  char title[100];
  char opponent_name[100];
  int player;

  get_current_duel_selection(&player, (int *)0);
  if (player == 0)
  {
    strcpy(title, gs_window_title_your_attack_00926010);
  }
  else
  {
    copy_opponent_name_prefix(opponent_name);
    sprintf(title, gs_window_title_opponent_attack_009267a0, opponent_name);
  }
  SetWindowTextA(hwnd, title);
}

// FUNCTION: MAGIC 0x004d8879
// FUNCTION: SHANDALAR 0x004624cf
int find_attack_phase_card_window(HWND hwnd, int *player_and_card, int *unused1, HWND *child_hwnd, int *unused2)
{
  attack_window_group_t *groups;
  card_id_t found_card_id;
  int group_count;
  int card_index;
  int group_index;
  HWND found_hwnd;
  int found_is_attacker;
  int found;

  if (hwnd == (HWND)0 || player_and_card == (int *)0)
  {
    found = 0;
  }
  else
  {
    groups = (attack_window_group_t *)GetWindowLongPtrA(hwnd, g_attackclass_data_window_long_offset);
    group_count = GetWindowLongA(hwnd, g_attackclass_count_window_long_offset);
    found = 0;
    for (group_index = 0; group_index < group_count; group_index++)
    {
      for (card_index = 0;
           card_index < groups[group_index].attacker_count && found == 0;
           card_index++)
      {
        if (card_window_matches_player_and_card(groups[group_index].attackers[card_index], player_and_card) != 0)
        {
          found = 1;
          found_card_id = get_card_window_displayed_card_id(groups[group_index].attackers[card_index]);
          found_hwnd = groups[group_index].attackers[card_index];
          found_is_attacker = 1;
        }
      }
      for (card_index = 0;
           card_index < groups[group_index].blocker_count && found == 0;
           card_index++)
      {
        if (card_window_matches_player_and_card(groups[group_index].blockers[card_index], player_and_card) != 0)
        {
          found = 1;
          found_card_id = get_card_window_displayed_card_id(groups[group_index].blockers[card_index]);
          found_hwnd = groups[group_index].blockers[card_index];
          found_is_attacker = 0;
        }
      }
    }
  }

  if (unused1 != (int *)0)
  {
    if (found != 0)
      *unused1 = found_card_id;
    else
      *unused1 = -1;
  }
  if (child_hwnd != (HWND *)0)
  {
    if (found != 0)
      *child_hwnd = found_hwnd;
    else
      *child_hwnd = (HWND)0;
  }
  if (unused2 != (int *)0)
  {
    if (found != 0)
      *unused2 = found_is_attacker;
    else
      *unused2 = 0;
  }
  return found;
}

// FUNCTION: MAGIC 0x004d6c6d
// FUNCTION: SHANDALAR 0x004608d2
void layout_attack_phase_window(HWND hwnd)
{
  DWORD style;
  int width;
  BOOL visible;
  int value;
  int scratch_padding1;
  int scratch_padding2;
  int scratch_padding3;
  int scratch_padding4;
  RECT adjusted_rect;
  RECT battlefield_rect;
  int visible_card_count;
  int layout_card_index;
  int attacker_y;
  int layout_group_index;
  int card_x;
  int blocker_y;
  int attacker_x;
  int blocker_x;
  int visible_attacker_descendants;
  int hidden_count;
  int count_card_index;
  int count_group_index;
  int visible_blocker_descendants;
  msvc_bitmap_t bitmap;
  attack_window_group_t *groups;
  int scrollbar_height;
  LONG group_count;
  int current_phase;
  int minimum_rows;
  int visible_attacker_rows;
  RECT content_rect;
  int current_player;
  HWND scrollbar_hwnd;
  int visible_blocker_rows;
  int blocker_spacing;
  int attacker_spacing;
  int scroll_pos;

  groups = (attack_window_group_t *)GetWindowLongPtrA(hwnd, g_attackclass_data_window_long_offset);
  group_count = GetWindowLongA(hwnd, g_attackclass_count_window_long_offset);
  scrollbar_hwnd = GetDlgItem(hwnd, 0);
  scroll_pos = SendMessageA(scrollbar_hwnd, SBM_GETPOS, 0, 0);
  get_current_duel_selection(&current_player, &current_phase);
  if (current_phase <= 0x14 || current_phase >= 0x1e ||
      is_attack_phase_window_enabled() == 0 ||
      (current_player == 1 && group_count == 0 && (g_duel_network_flags & 2) == 0))
  {
    ShowWindow(g_duel_player_battlefield_window, SW_HIDE);
    ShowWindow(g_duel_opponent_battlefield_window, SW_SHOW);
    ShowWindow(hwnd, SW_HIDE);
    ShowWindow(g_attack_ui_tooltip_window, SW_HIDE);
    UpdateWindow(g_duel_help_owner_hwnd);
    SendMessageA(scrollbar_hwnd, 0x468, 0, 0);
    SendMessageA(g_duel_attack_phase_window_hwnd, 0x40c, 0, 0);
    SendMessageA(g_duel_player_battlefield_window_hwnd, 0x401, 0, 0);
    SendMessageA(g_duel_help_owner_hwnd, 0x401, 0, 0);
    g_battlefield_draw_placeholder_x[1] = 5;
    g_battlefield_draw_placeholder_x[0] = g_battlefield_draw_placeholder_x[1];
    g_battlefield_draw_placeholder_y[1] = g_showlist_smallcard_height / 2;
    g_battlefield_draw_placeholder_y[0] = g_battlefield_draw_placeholder_y[1];
    return;
  }

  debug_draw_attack_phase_message("LayoutAttackCards", 0xff00ff, GetStockObject(LTGRAY_BRUSH));
  g_attack_ui_card_spacing = 10;
  g_attack_ui_layout_flags = 10;
  g_attack_phase_display_width = (g_showlist_smallcard_width * 0xf) / 100;
  g_attack_phase_display_state = 5;
  minimum_rows = 2;
  if (g_attack_ui_button_bitmap != (HBITMAP)0)
  {
    GetObjectA(g_attack_ui_button_bitmap, sizeof(bitmap), &bitmap);
    if (GetSystemMetrics(SM_CYHSCROLL) * 2 < bitmap.bmHeight / 2)
      scrollbar_height = bitmap.bmHeight / 2;
    else
      scrollbar_height = GetSystemMetrics(SM_CYHSCROLL) * 2;
  }
  else
  {
    scrollbar_height = GetSystemMetrics(SM_CYHSCROLL) * 2;
  }

  visible_attacker_descendants = 0;
  visible_blocker_descendants = 0;
  for (count_group_index = 0; count_group_index < group_count; count_group_index++)
  {
    for (count_card_index = 0;
         count_card_index < groups[count_group_index].attacker_count;
         count_card_index++)
    {
      if (get_card_window_hidden_flag(groups[count_group_index].attackers[count_card_index]) == 0)
      {
        hidden_count = count_attack_phase_hidden_descendants(hwnd,
                                                             groups[count_group_index].attackers[count_card_index]);
        if (hidden_count > visible_attacker_descendants)
          visible_attacker_descendants = hidden_count;
      }
    }
    for (count_card_index = 0;
         count_card_index < groups[count_group_index].blocker_count;
         count_card_index++)
    {
      if (get_card_window_hidden_flag(groups[count_group_index].blockers[count_card_index]) == 0)
      {
        hidden_count = count_attack_phase_hidden_descendants(hwnd,
                                                             groups[count_group_index].blockers[count_card_index]);
        if (hidden_count > visible_blocker_descendants)
          visible_blocker_descendants = hidden_count;
      }
    }
  }

  if (current_player == 0)
  {
    visible_attacker_rows = visible_attacker_descendants;
    visible_blocker_rows = visible_blocker_descendants;
  }
  else
  {
    visible_blocker_rows = visible_attacker_descendants;
    visible_attacker_rows = visible_blocker_descendants;
  }

  g_attack_phase_display_height = (visible_blocker_rows <= minimum_rows ? minimum_rows : visible_blocker_rows) *
                                      g_duel_selected_card_window +
                                  g_attack_ui_layout_flags;

  g_attack_phase_display_flags = (visible_attacker_rows <= minimum_rows ? minimum_rows : visible_attacker_rows) *
                                     g_duel_selected_card_window +
                                 g_showlist_smallcard_height +
                                 g_attack_phase_display_height + scrollbar_height + g_attack_phase_display_state + g_attack_phase_display_state;

  attacker_spacing = g_showlist_smallcard_width / 3;
  blocker_spacing = g_showlist_smallcard_width / 3;
  SetWindowPos(scrollbar_hwnd, (HWND)0, 0, g_showlist_smallcard_height + g_attack_phase_display_height + g_attack_phase_display_state,
               0, 0, SWP_NOSIZE | SWP_NOZORDER);

  card_x = ATTACK_WINDOW_TITLE_HEIGHT - scroll_pos;
  content_rect.left = card_x;
  content_rect.right = card_x;
  content_rect.top = g_attack_phase_display_height -
                     (visible_blocker_rows <= minimum_rows ? minimum_rows : visible_blocker_rows) *
                         g_duel_selected_card_window;
  content_rect.bottom = g_showlist_smallcard_height + g_attack_phase_display_flags;
  if (current_player == 0)
  {
    attacker_y = g_attack_phase_display_flags;
    blocker_y = g_attack_phase_display_height;
  }
  else
  {
    attacker_y = g_attack_phase_display_height;
    blocker_y = g_attack_phase_display_flags;
  }

  for (layout_group_index = 0; layout_group_index < group_count; layout_group_index++)
  {
    blocker_x = card_x;
    attacker_x = blocker_x;
    visible_card_count = 0;
    for (layout_card_index = 0;
         layout_card_index < groups[layout_group_index].attacker_count;
         layout_card_index++)
    {
      if (get_card_window_hidden_flag(groups[layout_group_index].attackers[layout_card_index]) == 0)
      {
        attacker_x = attacker_spacing * visible_card_count + card_x;
        MoveWindow(groups[layout_group_index].attackers[layout_card_index], attacker_x, attacker_y,
                   g_showlist_smallcard_width, g_showlist_smallcard_height, 1);
        visible_card_count++;
        if (content_rect.right < attacker_x + g_showlist_smallcard_width)
          content_rect.right = attacker_x + g_showlist_smallcard_width;
      }
    }
    visible_card_count = 0;
    for (layout_card_index = 0;
         layout_card_index < groups[layout_group_index].blocker_count;
         layout_card_index++)
    {
      if (get_card_window_hidden_flag(groups[layout_group_index].blockers[layout_card_index]) == 0)
      {
        blocker_x = blocker_spacing * visible_card_count + card_x;
        MoveWindow(groups[layout_group_index].blockers[layout_card_index], blocker_x, blocker_y,
                   g_showlist_smallcard_width, g_showlist_smallcard_height, 1);
        visible_card_count++;
        if (content_rect.right < blocker_x + g_showlist_smallcard_width)
          content_rect.right = blocker_x + g_showlist_smallcard_width;
      }
    }
    card_x = (blocker_x <= attacker_x ? attacker_x : blocker_x) +
             g_showlist_smallcard_width + g_attack_phase_display_width;
  }

  scratch_padding3 = g_showlist_smallcard_width / 3;
  content_rect.left -= ATTACK_WINDOW_TITLE_HEIGHT;
  content_rect.right += ATTACK_WINDOW_TITLE_HEIGHT;
  content_rect.top -= g_attack_ui_layout_flags;
  content_rect.bottom += g_attack_ui_layout_flags;
  width = g_showlist_smallcard_width + ATTACK_WINDOW_TITLE_HEIGHT * 2;
  if (content_rect.right - content_rect.left < width)
    content_rect.right = content_rect.left + width;

  style = GetWindowLongA(hwnd, GWL_STYLE);
  CopyRect(&adjusted_rect, &content_rect);
  AdjustWindowRect(&adjusted_rect, style, 0);
  GetWindowRect(g_duel_player_battlefield_window_hwnd, &battlefield_rect);
  width = battlefield_rect.top - (adjusted_rect.bottom - adjusted_rect.top);
  if (width < 6)
    width = 5;
  hidden_count = (battlefield_rect.right - battlefield_rect.left) - scratch_padding3;
  if (adjusted_rect.right - adjusted_rect.left <= hidden_count)
    hidden_count = adjusted_rect.right - adjusted_rect.left;

  MoveWindow(g_attack_phase_window_hwnd, battlefield_rect.left, width, scratch_padding3,
             adjusted_rect.bottom - adjusted_rect.top, 1);
  MoveWindow(hwnd, scratch_padding3 + battlefield_rect.left, width, hidden_count,
             adjusted_rect.bottom - adjusted_rect.top, 1);
  GetClientRect(hwnd, &adjusted_rect);
  SetWindowPos(scrollbar_hwnd, (HWND)0, 0, 0, adjusted_rect.right, scrollbar_height,
               SWP_NOMOVE | SWP_NOZORDER);
  UpdateWindow(g_duel_help_owner_hwnd);
  UpdateWindow(g_duel_player_battlefield_window_hwnd);
  GetClientRect(hwnd, &adjusted_rect);
  if (adjusted_rect.right < content_rect.right - content_rect.left)
  {
    value = (content_rect.right - content_rect.left) - adjusted_rect.right;
    SendMessageA(scrollbar_hwnd, SBM_SETRANGE, 0, value);
    SendMessageA(scrollbar_hwnd, 0x468, 1, 0);
    if (scroll_pos < 0)
      scroll_pos = 0;
    if (value < scroll_pos)
      scroll_pos = value;
    SendMessageA(hwnd, WM_HSCROLL, MAKELONG(SB_THUMBPOSITION, scroll_pos), (LPARAM)scrollbar_hwnd);
  }
  else
  {
    SendMessageA(scrollbar_hwnd, 0x468, 0, 0);
    scroll_pos = 0;
    SendMessageA(hwnd, WM_HSCROLL, MAKELONG(SB_THUMBPOSITION, scroll_pos), (LPARAM)scrollbar_hwnd);
  }

  if (IsWindowVisible(hwnd) == 0 && IsWindowVisible(g_attack_ui_tooltip_window) == 0)
  {
    set_attack_phase_window_title(hwnd);
    ShowWindow(g_duel_player_battlefield_window, SW_SHOW);
    ShowWindow(g_duel_opponent_battlefield_window, SW_HIDE);
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(g_attack_phase_window_hwnd);
    UpdateWindow(hwnd);
    UpdateWindow(g_duel_player_battlefield_window);
    restack_duel_child_windows();
  }
  UpdateWindow(hwnd);
  UpdateWindow(g_duel_help_owner_hwnd);
}

// FUNCTION: MAGIC 0x0053582a
// FUNCTION: SHANDALAR 0x0055a06a
int register_MAGICGAME_AttackPhaseDisplayClass(LPCSTR class_name)
{
  char pic_path[264];
  int registered;
  WNDCLASSA wndclass;

  registered = 1;
  SET_DUEL_WNDCLASS(wndclass, 0xb, wndproc_MAGICGAME_AttackPhaseDisplayClass, g_phase_display_window_extra_bytes,
                    LoadIconA((HINSTANCE)0, (LPCSTR)0x7f00), (HBRUSH)0x6, class_name);
  if (RegisterClassA(&wndclass) == 0)
    registered = 0;
  strcpy(pic_path, global_duelart_path);
  strcat(pic_path, "\\WINBK_PhaseCombat.pic");
  ATTACK_PHASE_DISPLAY_PIC = load_pic(pic_path);
  return registered;
}

// FUNCTION: MAGIC 0x005358fb
// FUNCTION: SHANDALAR 0x0055a13b
void destroy_MAGICGAME_AttackPhaseDisplayClass(LPCSTR class_name)
{
  (void)class_name;
  if (ATTACK_PHASE_DISPLAY_PIC != (HBITMAP)0)
    delete_and_close_object(ATTACK_PHASE_DISPLAY_PIC);

  ATTACK_PHASE_DISPLAY_PIC = (HANDLE)0;
}

// FUNCTION: MAGIC 0x00537760
// FUNCTION: SHANDALAR 0x0055bf91
LRESULT CALLBACK wndproc_MAGICGAME_AttackPhaseDisplayClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  int paint_selected_phase_index;
  int phase_index;
  int menu_count;
  int selected_phase;
  int phase_flags[38];
  POINT menu_point;
  unsigned int phase_help_command;
  int menu_phase_index;
  unsigned int stop_command;
  unsigned int toggle_command;
  RECT menu_client_rect;
  int menu_player;
  POINT popup_point;
  RECT popup_rect;
  char pic_path[264];
  HRGN paint_clip_region;
  HDC paint_dc;
  int paint_selected_phase;
  msvc_bitmap_t paint_bitmap;
  PAINTSTRUCT paint;
  int paint_saved_dc;
  RECT paint_phase_rect;
  int paint_player;
  RECT paint_client_rect;
  int click_player;
  MSG peek_message;
  int peek_result;
  POINT click_point;
  RECT click_client_rect;
  int click_phase;
  char help_path[264];
  unsigned int help_context;
  int toggle_phase;
  int toggle_player;
  RECT toggle_client_rect;
  RECT toggle_phase_rect;
  int stop_phase_code;
  int command_phase;
  int command_base;
  int command_index;
  int stop_player;
  char general_help_path[264];
  unsigned int general_help_context;
  int refresh_phase;
  char cue_text[100];
  int cue_phase;
  POINT cue_point;
  int cue_valid;
  RECT cue_client_rect;
  int cached_phase;

  switch (msg)
  {
  case 0x437:
    cue_point.x = lparam & 0xffff;
    cue_point.y = HIWORD(lparam);
    GetClientRect(hwnd, &cue_client_rect);
    hit_test_attack_phase_display(&cue_point, &cue_client_rect, &cue_phase);
    cue_valid = 1;
    if (cue_phase == 0x15)
      strcpy(cue_text, gs_phasebar_choose_attackers_008966e0);
    else if (cue_phase == 0x16)
      strcpy(cue_text, gs_phasebar_attacker_fast_effects_0093a7c0);
    else if (cue_phase == 0x17)
      strcpy(cue_text, gs_phasebar_assign_defenders_00925a90);
    else if (cue_phase == 0x18)
      strcpy(cue_text, gs_phasebar_blocker_fast_effects_00925bc0);
    else if (cue_phase == 0x19)
      strcpy(cue_text, gs_phasebar_resolve_first_strike_damage_0091d220);
    else if (cue_phase == 0x1a || cue_phase == 0x1b)
      strcpy(cue_text, gs_phasebar_resolve_normal_damage_008cec80);
    else if (cue_phase == 0x1e)
      strcpy(cue_text, gs_phasebar_main_phase_postcombat_00789870);
    else
      cue_valid = 0;
    if (cue_valid != 0)
      strcpy((char *)wparam, cue_text);
    return cue_valid;

  case 0x432:
    cached_phase = GetWindowLongA(hwnd, g_phase_display_selected_card_window_long_offset);
    get_current_duel_selection((int *)0, &refresh_phase);
    if (cached_phase != refresh_phase)
      InvalidateRect(hwnd, (RECT *)0, 0);
    return 0;

  case 0x400:
    noop_phase_display_user_message(hwnd, wparam, lparam);
    return 0;

  case WM_COMMAND:
    switch ((int)((unsigned int)wparam & 0xffff))
    {
    case ATTACK_PHASE_DISPLAY_COMMAND_HELP:
      general_help_context = 0x7e5;
      strcpy(general_help_path, global_base_directory);
      strcat(general_help_path, "\\duel.hlp");
      WinHelpA(g_duel_window_hwnd, general_help_path, HELP_CONTEXT, general_help_context);
      break;

    default:
      command_phase = (int)((unsigned int)wparam & 0xffff);
      if (command_phase >= ATTACK_PHASE_DISPLAY_COMMAND_HELP_BASE)
        command_base = ATTACK_PHASE_DISPLAY_COMMAND_HELP_BASE;
      else if (command_phase >= ATTACK_PHASE_DISPLAY_COMMAND_TOGGLE_BASE)
        command_base = ATTACK_PHASE_DISPLAY_COMMAND_TOGGLE_BASE;
      else
        command_base = ATTACK_PHASE_DISPLAY_COMMAND_STOP_BASE;
      command_phase -= command_base;
      command_index = command_phase;

      switch (command_base)
      {
      case ATTACK_PHASE_DISPLAY_COMMAND_STOP_BASE:
        if ((g_duel_network_flags & 2) != 0)
          break;

        if (command_index == 0)
          stop_phase_code = 0x15;
        else if (command_index == 1)
          stop_phase_code = 0x16;
        else if (command_index == 2)
          stop_phase_code = 0x17;
        else if (command_index == 3)
          stop_phase_code = 0x18;
        else if (command_index == 4)
          stop_phase_code = 0x19;
        else if (command_index == 5)
          stop_phase_code = 0x1b;
        else if (command_index == 6)
          stop_phase_code = 0x1e;
        get_current_duel_selection(&stop_player, (int *)0);
        g_stop_phase_player = stop_player;
        g_stop_phase = stop_phase_code;
        g_recorded_action_player = 0;
        g_attack_phase_display_menu_packet.selection_code = -2;
        g_attack_phase_display_menu_packet.target_player = -1;
        g_attack_phase_display_menu_packet.target_card = -1;
        g_attack_phase_display_menu_packet.unused = 0;
        PostMessageA(g_duel_window_hwnd, 0x464, 0, (LPARAM)&g_attack_phase_display_menu_packet);
        break;

      case ATTACK_PHASE_DISPLAY_COMMAND_TOGGLE_BASE:
        if (command_index == 0)
          toggle_phase = 0x15;
        else if (command_index == 1)
          toggle_phase = 0x16;
        else if (command_index == 2)
          toggle_phase = 0x17;
        else if (command_index == 3)
          toggle_phase = 0x18;
        else if (command_index == 4)
          toggle_phase = 0x19;
        else if (command_index == 5)
          toggle_phase = 0x1b;
        else if (command_index == 6)
          toggle_phase = 0x1e;
        get_current_duel_selection(&toggle_player, (int *)0);
        if (((int)(char)g_duel_phase_stop_settings[toggle_player].phase_flags[toggle_phase] & PHASE_STOP_ENABLED) != 0)
          g_duel_phase_stop_settings[toggle_player].phase_flags[toggle_phase] =
              (unsigned char)(((int)(char)g_duel_phase_stop_settings[toggle_player].phase_flags[toggle_phase]) & ~PHASE_STOP_ENABLED);
        else
          g_duel_phase_stop_settings[toggle_player].phase_flags[toggle_phase] =
              (unsigned char)(((int)(char)g_duel_phase_stop_settings[toggle_player].phase_flags[toggle_phase]) | PHASE_STOP_ENABLED);
        refresh_duel_display_cache();
        GetClientRect(hwnd, &toggle_client_rect);
        get_attack_phase_display_phase_rect(&toggle_phase_rect, toggle_phase,
                                            toggle_client_rect.right, toggle_client_rect.bottom);
        InvalidateRect(hwnd, &toggle_phase_rect, 0);
        break;

      case ATTACK_PHASE_DISPLAY_COMMAND_HELP_BASE:
        if (command_index == 0)
          help_context = 0x7dd;
        else if (command_index == 1)
          help_context = 0x7dd;
        else if (command_index == 2)
          help_context = 0x7dd;
        else if (command_index == 3)
          help_context = 0x7dd;
        else if (command_index == 4)
          help_context = 0x7dd;
        else if (command_index == 5)
          help_context = 0x7dd;
        else if (command_index == 6)
          help_context = 0x7dd;
        strcpy(help_path, global_base_directory);
        strcat(help_path, "\\duel.hlp");
        WinHelpA(g_duel_window_hwnd, help_path, HELP_CONTEXT, help_context);
        break;
      }
      break;
    }
    return 0;

  case WM_CREATE:
    cached_phase = 0;
    SetWindowLongA(hwnd, g_phase_display_selected_card_window_long_offset, cached_phase);
    return 0;

  case WM_ERASEBKGND:
    return 1;

  case WM_LBUTTONDOWN:
    Sleep(GetDoubleClickTime());
    peek_result = PeekMessageA(&peek_message, hwnd, WM_LBUTTONDBLCLK, WM_LBUTTONDBLCLK, 0);
    click_point.x = lparam & 0xffff;
    click_point.y = HIWORD(lparam);
    GetClientRect(hwnd, &click_client_rect);
    hit_test_attack_phase_display(&click_point, &click_client_rect, &click_phase);
    if ((g_duel_network_flags & 2) == 0 && click_phase != -1 && g_duel_modal_action_active != 0)
    {
      get_current_duel_selection(&click_player, (int *)0);
      g_stop_phase_player = click_player;
      g_stop_phase = click_phase;
      g_recorded_action_player = peek_result;
      g_attack_phase_display_click_packet.selection_code = -2;
      g_attack_phase_display_click_packet.target_player = -1;
      g_attack_phase_display_click_packet.target_card = -1;
      g_attack_phase_display_click_packet.unused = 0;
      PostMessageA(g_duel_window_hwnd, 0x464, 0, (LPARAM)&g_attack_phase_display_click_packet);
    }
    return 0;

  case WM_PAINT:
    cached_phase = GetWindowLongA(hwnd, g_phase_display_selected_card_window_long_offset);
    get_current_duel_selection(&paint_player, &paint_selected_phase);
    if (ATTACK_PHASE_DISPLAY_PIC == (HBITMAP)0)
    {
      strcpy(pic_path, global_duelart_path);
      strcat(pic_path, "\\WINBK_PhaseCombat.pic");
      ATTACK_PHASE_DISPLAY_PIC = load_pic(pic_path);
    }
    if (paint_selected_phase != cached_phase)
      InvalidateRect(hwnd, (RECT *)0, 0);

    EnterCriticalSection(&g_card_render_lock);
    paint_dc = g_shared_offscreen_dc;
    paint_saved_dc = SaveDC(g_shared_offscreen_dc);
    GetClientRect(hwnd, &paint_client_rect);
    get_current_duel_selection(&paint_player, (int *)0);
    if (ATTACK_PHASE_DISPLAY_PIC != (HBITMAP)0)
    {
      GetObjectA(ATTACK_PHASE_DISPLAY_PIC, sizeof(paint_bitmap), &paint_bitmap);
      if (paint_player == 1)
        phase_index = 0;
      else
        phase_index = paint_bitmap.bmWidth / 2;
      DrawBitmapSubrectToRect(paint_dc, &paint_client_rect, ATTACK_PHASE_DISPLAY_PIC,
                              phase_index, 0,
                              paint_bitmap.bmWidth / 4,
                              paint_bitmap.bmHeight);
    }
    else
    {
      FillRect(paint_dc, &paint_client_rect, GetStockObject(BLACK_BRUSH));
    }
    if (paint_selected_phase != -1)
    {
      GetClientRect(hwnd, &paint_client_rect);
      get_attack_phase_display_phase_rect(&paint_phase_rect, paint_selected_phase,
                                          paint_client_rect.right, paint_client_rect.bottom);
      paint_clip_region = CreateRectRgnIndirect(&paint_phase_rect);
      SelectClipRgn(paint_dc, paint_clip_region);
      if (ATTACK_PHASE_DISPLAY_PIC != (HBITMAP)0)
      {
        GetObjectA(ATTACK_PHASE_DISPLAY_PIC, sizeof(paint_bitmap), &paint_bitmap);
        if (paint_player == 1)
          paint_selected_phase_index = paint_bitmap.bmWidth / 4;
        else
          paint_selected_phase_index = (paint_bitmap.bmWidth * 3) / 4;
        DrawBitmapSubrectToRect(paint_dc, &paint_client_rect, ATTACK_PHASE_DISPLAY_PIC,
                                paint_selected_phase_index, 0,
                                paint_bitmap.bmWidth / 4,
                                paint_bitmap.bmHeight);
      }
      else
      {
        FillRect(paint_dc, &paint_client_rect, GetStockObject(GRAY_BRUSH));
      }
      SelectClipRgn(paint_dc, (HRGN)0);
      DeleteObject(paint_clip_region);
    }

    RestoreDC(g_shared_offscreen_dc, paint_saved_dc);
    draw_attack_phase_display_status_markers(paint_dc, &paint_client_rect);
    paint_dc = BeginPaint(hwnd, &paint);
    if (paint_dc != (HDC)0)
    {
      ApplyCardArtPaletteToDc(paint_dc);
      GetClientRect(hwnd, &paint_client_rect);
      BitBlt(paint_dc, 0, 0, paint_client_rect.right, paint_client_rect.bottom,
             g_shared_offscreen_dc, 0, 0, SRCCOPY);
      EndPaint(hwnd, &paint);
      cached_phase = paint_selected_phase;
      SetWindowLongA(hwnd, g_phase_display_selected_card_window_long_offset, cached_phase);
    }
    LeaveCriticalSection(&g_card_render_lock);
    return 0;

  case WM_RBUTTONDOWN:
    popup_point.x = lparam & 0xffff;
    popup_point.y = HIWORD(lparam);
    if (g_duel_modal_action_active != 0)
    {
      ClientToScreen(hwnd, &popup_point);
      SetRect(&popup_rect, popup_point.x, popup_point.y,
              popup_point.x + 1, popup_point.y + 1);
      TrackPopupMenu(g_phase_display_menu, TPM_RIGHTBUTTON, popup_point.x, popup_point.y,
                     0, hwnd, &popup_rect);
    }
    return 0;

  case WM_INITMENU:
    GetCursorPos(&menu_point);
    ScreenToClient(hwnd, &menu_point);
    GetClientRect(hwnd, &menu_client_rect);
    hit_test_attack_phase_display(&menu_point, &menu_client_rect, &selected_phase);
    get_current_duel_selection(&menu_player, (int *)0);
    if (selected_phase == 0x15)
      menu_phase_index = 0;
    else if (selected_phase == 0x16)
      menu_phase_index = 1;
    else if (selected_phase == 0x17)
      menu_phase_index = 2;
    else if (selected_phase == 0x18)
      menu_phase_index = 3;
    else if (selected_phase == 0x19)
      menu_phase_index = 4;
    else if (selected_phase == 0x1b)
      menu_phase_index = 5;
    else if (selected_phase == 0x1e)
      menu_phase_index = 6;
    else
      menu_phase_index = -1;
    if (selected_phase != -1)
    {
      if (g_duel_modal_action_active != 0)
      {
        stop_command = menu_phase_index + ATTACK_PHASE_DISPLAY_COMMAND_STOP_BASE;
        if ((g_duel_network_flags & 2) == 0)
          AppendMenuA(g_phase_display_menu, 0, stop_command, g_phase_display_menu_stop_text);
      }
      if (GetMenuItemCount(g_phase_display_menu) != 0)
        AppendMenuA(g_phase_display_menu, MF_SEPARATOR, 0, (LPCSTR)0);
      toggle_command = menu_phase_index + ATTACK_PHASE_DISPLAY_COMMAND_TOGGLE_BASE;
      AppendMenuA(g_phase_display_menu, 0, toggle_command, g_phase_display_menu_toggle_text);
      copy_phase_stop_flags(phase_flags, menu_player);
      if (phase_flags[selected_phase] != 0)
        CheckMenuItem(g_phase_display_menu, toggle_command, MF_CHECKED);
      phase_help_command = menu_phase_index + ATTACK_PHASE_DISPLAY_COMMAND_HELP_BASE;
      AppendMenuA(g_phase_display_menu, 0, phase_help_command, g_phase_display_menu_phase_help_text);
    }
    AppendMenuA(g_phase_display_menu, 0, ATTACK_PHASE_DISPLAY_COMMAND_HELP, g_phase_display_menu_help_text);
    return 0;

  case WM_MENUSELECT:
    if (HIWORD(wparam) == 0xffff && lparam == 0)
    {
      menu_count = GetMenuItemCount(g_phase_display_menu);
      while (menu_count-- != 0)
      {
        DeleteMenu(g_phase_display_menu, 0, MF_BYPOSITION);
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
