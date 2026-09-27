#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vfw.h>

#include "cardartlib/src/palette.h"
#include "drawcardlib/Drawcardlib.h"
#include "drawcardlib/src/pic.h"
#include "magic/src/duel_engine.h"
#include "magic/src/game_support.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "magic/src/network.h"
#include "magic/src/shared_startup.h"

#ifndef MCIWNDM_PLAY
#define MCIWNDM_PLAY 0x806
#endif
#ifndef MCIWNDM_STOP
#define MCIWNDM_STOP 0x808
#endif

extern HINSTANCE g_app_instance;
extern int g_manalink_is_host;
extern HWND g_show_card_list_last_preview_hwnd;
extern HPALETTE global_cart_art_hpalette;
extern card_ptr_t global_raw_cards_storage[2000];
extern HANDLE global_mutex_GameInit;

void delete_and_close_object(HANDLE obj);
void resize_duel_hand_window(HWND hwnd);
void set_player_directive_value(int player, int value);
void refresh_duel_window(HWND hwnd);
void change_buttonclass_wndproc(HWND hwnd);
void draw_duel_dialog_bitmap_button(DRAWITEMSTRUCT *draw_item,
                                    HBITMAP normal_bitmap,
                                    HBITMAP pressed_bitmap,
                                    HBITMAP disabled_bitmap,
                                    COLORREF text_color,
                                    int draw_focus);
INT_PTR CALLBACK dlgproc_duel_coin_flip_animation(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
void load_coin_flip_animation_dialog_assets(HBRUSH *brush, COLORREF *text_color);
void cleanup_coin_flip_animation_dialog_assets(HGDIOBJ brush);
static BOOL CALLBACK restore_coin_flip_palette(HWND hwnd, LPARAM palette);
void load_redraw_ante_dialog_assets(HBITMAP *background,
                                    int *text_color,
                                    HBITMAP *normal_button_bitmap,
                                    HBITMAP *depressed_button_bitmap,
                                    HBITMAP *disabled_button_bitmap,
                                    COLORREF *normal_color,
                                    COLORREF *focus_color);
void cleanup_redraw_ante_dialog_assets(HANDLE background,
                                       HANDLE normal_button_bitmap,
                                       HANDLE depressed_button_bitmap,
                                       HANDLE disabled_button_bitmap);
void __cdecl redraw_ante_wait_for_opponent_done_thread(void *hwnd);
void __cdecl redraw_ante_wait_for_opponent_mulligan_thread(void *hwnd);

typedef struct DuelCoinFlipAnimationDialogContext
{
  char title[100];
  int coin_result;
  int show_coin_flips;
} DuelCoinFlipAnimationDialogContext;

// GLOBAL: MAGIC 0x00638c74
// GLOBAL: SHANDALAR 0x006503bc
HBITMAP g_coin_flip_dialog_background;

// GLOBAL: MAGIC 0x00638c68
// GLOBAL: SHANDALAR 0x006503b0
int g_coin_flip_dialog_text_color;

// GLOBAL: MAGIC 0x00638bac
// GLOBAL: SHANDALAR 0x006502f4
HBITMAP g_coin_flip_dialog_button_normal_bitmap;

// GLOBAL: MAGIC 0x00638c58
// GLOBAL: SHANDALAR 0x006503a0
HBITMAP g_coin_flip_dialog_button_depressed_bitmap;

// GLOBAL: MAGIC 0x00638be0
// GLOBAL: SHANDALAR 0x00650328
COLORREF g_coin_flip_dialog_button_normal_color;

// GLOBAL: MAGIC 0x00638ba0
// GLOBAL: SHANDALAR 0x006502e8
COLORREF g_coin_flip_dialog_button_focus_color;

// GLOBAL: MAGIC 0x005726a0
// GLOBAL: SHANDALAR 0x0058f1a8
HWND g_coin_flip_animation_mci_hwnd = (HWND)0;

// GLOBAL: MAGIC 0x00638b7c
// GLOBAL: SHANDALAR 0x006502c4
HBRUSH g_coin_flip_animation_dialog_brush;

// GLOBAL: MAGIC 0x00638c60
// GLOBAL: SHANDALAR 0x006503a8
COLORREF g_coin_flip_animation_dialog_text_color;

// GLOBAL: MAGIC 0x00638c64
// GLOBAL: SHANDALAR 0x006503ac
DuelCoinFlipAnimationDialogContext *g_coin_flip_animation_dialog_context;

typedef struct
{
  int coin_winner;
  int was_random_starting_player;
  int play_draw_choice;
  int coin_flip_result;
} DuelCoinFlipDialogContext;

typedef struct
{
  int starting_player;
  int opponent_ante_card;
  int player_ante_card;
  int redraw_result;
  int ante_result;
  int ante_info;
  int mulligan_accepted;
  int opponent_mulligan_accepted;
} DuelRedrawAnteDialogContext;

// GLOBAL: MAGIC 0x00638c10
// GLOBAL: SHANDALAR 0x00650358
HBITMAP g_redraw_ante_dialog_background;

// GLOBAL: MAGIC 0x00638c88
// GLOBAL: SHANDALAR 0x006503d0
int g_redraw_ante_dialog_text_color;

// GLOBAL: MAGIC 0x00638c3c
// GLOBAL: SHANDALAR 0x00650384
HBITMAP g_redraw_ante_dialog_button_normal_bitmap;

// GLOBAL: MAGIC 0x00638b4c
// GLOBAL: SHANDALAR 0x00650294
HBITMAP g_redraw_ante_dialog_button_depressed_bitmap;

// GLOBAL: MAGIC 0x00638b40
// GLOBAL: SHANDALAR 0x00650288
HBITMAP g_redraw_ante_dialog_button_disabled_bitmap;

// GLOBAL: MAGIC 0x00638c04
// GLOBAL: SHANDALAR 0x0065034c
COLORREF g_redraw_ante_dialog_button_normal_color;

// GLOBAL: MAGIC 0x00638bf0
// GLOBAL: SHANDALAR 0x00650338
COLORREF g_redraw_ante_dialog_button_focus_color;

// GLOBAL: MAGIC 0x00638b74
// GLOBAL: SHANDALAR 0x006502bc
int g_redraw_ante_waiting_for_both_network_choices;

// GLOBAL: MAGIC 0x00638c0c
// GLOBAL: SHANDALAR 0x00650354
char g_redraw_ante_opponent_choice_received;

// FUNCTION: MAGIC 0x004a3b82
// FUNCTION: SHANDALAR 0x0053f510
int coin_flip(int player, char *dialog_title, int show_dialog_if_animation_is_off)
{
  int palette_index;      // ebp - 0x884
  PaletteLog log_palette; // ebp - 0x880
  PALETTEENTRY palette_entries[256];
  HPALETTE coin_flip_palette;
  HPALETTE old_palette;
  DuelCoinFlipAnimationDialogContext animation_context;
  int dialog_result;
  int show_dialog;

  KillTimer(g_duel_window_hwnd, g_duel_timer_id);
  if ((g_duel_network_flags & 2) != 0)
  {
    if (g_manalink_is_host != 0)
    {
      g_network_result_packet.packet_type = 1;
      animation_context.coin_result = rand() % 2;
      g_network_result_packet.result = animation_context.coin_result;
      TENTATIVE_send_network_result(player, 1);
    }
    else
    {
      TENTATIVE_wait_for_network_result(player, 1);
      animation_context.coin_result = g_network_result_packet.result;
    }
  }
  else
  {
    animation_context.coin_result = rand() % 2;
  }
  animation_context.show_coin_flips = g_duel_interface_options.show_coin_flips;
  if (g_duel_interface_options.show_coin_flips != 0)
  {
    show_dialog = 1;
  }
  else if (show_dialog_if_animation_is_off != 0)
  {
    show_dialog = 1;
  }
  else
  {
    show_dialog = 0;
  }

  if (g_duel_ai_mode_state == 1)
  {
    show_dialog = 0;
  }

  if (show_dialog != 0)
  {
    strcpy(animation_context.title, dialog_title);
    GetPaletteEntries(global_cart_art_hpalette, 0, 0x100, palette_entries);
    for (palette_index = 0; palette_index < 0x100; ++palette_index)
    {
      palette_entries[palette_index].peFlags = 0;
    }
    log_palette.palVersion = 0x300;
    log_palette.palNumEntries = 0x100;
    memcpy(log_palette.palPalEntry, palette_entries, 0x400);
    coin_flip_palette = CreatePalette((LOGPALETTE *)&log_palette);
    old_palette = global_cart_art_hpalette;
    global_cart_art_hpalette = coin_flip_palette;
    dialog_result = DialogBoxParamA(g_app_instance,
                                    (LPCSTR)0xf0,
                                    g_duel_window_hwnd,
                                    dlgproc_duel_coin_flip_animation,
                                    (LPARAM)&animation_context);
    InvalidateRect(g_duel_player_battlefield_window_hwnd, (RECT *)0, 1);
    InvalidateRect(g_duel_help_owner_hwnd, (RECT *)0, 1);
    global_cart_art_hpalette = old_palette;
    ApplyCardArtPaletteToDc(g_shared_offscreen_dc);
    restore_coin_flip_palette(g_duel_window_hwnd, (LPARAM)old_palette);
    EnumChildWindows(g_duel_window_hwnd, restore_coin_flip_palette,
                     (LPARAM)old_palette);
    DeleteObject(coin_flip_palette);
  }
  return animation_context.coin_result;
}

static BOOL CALLBACK restore_coin_flip_palette(HWND hwnd, LPARAM palette)
{
  HDC dc = GetDC(hwnd);
  if (dc != NULL)
  {
    SelectPalette(dc, (HPALETTE)palette, FALSE);
    ReleaseDC(hwnd, dc);
  }
  /*
   * The coin palette is cloned from the card-art palette, so the existing
   * indexed pixels remain valid after this swap. Repainting every window here
   * exposes partially covered backing surfaces during dialog teardown; the
   * two battlefield windows that need refreshing are invalidated explicitly
   * by coin_flip().
   */
  return TRUE;
}

// FUNCTION: MAGIC 0x004a44b7
// FUNCTION: SHANDALAR 0x0053fe44
void load_coin_flip_animation_dialog_assets(HBRUSH *brush, COLORREF *text_color)
{
  *brush = CreateSolidBrush(0x100000d);
  *text_color = 0x10000bf;
}

// FUNCTION: MAGIC 0x004a44db
// FUNCTION: SHANDALAR 0x0053fe68
void cleanup_coin_flip_animation_dialog_assets(HGDIOBJ brush)
{
  if (brush != (HGDIOBJ)0)
  {
    DeleteObject(brush);
  }
}

// FUNCTION: MAGIC 0x004a3d9c
// FUNCTION: SHANDALAR 0x0053f72a
INT_PTR CALLBACK dlgproc_duel_coin_flip_animation(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  HDC erase_dc;
  RECT erase_rect;
  HWND ctl_hwnd;
  int ctl_id;
  HDC ctl_dc;
  char avi_path[264];
  HDC measure_dc;
  HGDIOBJ prompt_font;
  RECT rect;
  SIZE prompt_size;

  switch (msg)
  {
  case WM_INITDIALOG:
    g_coin_flip_animation_dialog_context = (DuelCoinFlipAnimationDialogContext *)lparam;
    load_coin_flip_animation_dialog_assets(&g_coin_flip_animation_dialog_brush,
                                           &g_coin_flip_animation_dialog_text_color);
    load_text("UIStrings.txt", "DIALOG_COINFLIP");
    if (g_coin_flip_animation_dialog_context->coin_result != 0)
    {
      SetDlgItemTextA(hwnd, 0x41c, g_text_lines[1]);
    }
    else
    {
      SetDlgItemTextA(hwnd, 0x41c, g_text_lines[0]);
    }
    ShowWindow(GetDlgItem(hwnd, 0x41c), 0);
    SetDlgItemTextA(hwnd, 0x41b, g_coin_flip_animation_dialog_context->title);
    prompt_font = (HGDIOBJ)SendDlgItemMessageA(hwnd, 0x41b, WM_GETFONT, 0, 0);
    measure_dc = GetDC(hwnd);
    SelectObject(measure_dc, prompt_font);
    GetTextExtentPoint32A(measure_dc,
                          g_coin_flip_animation_dialog_context->title,
                          strlen(g_coin_flip_animation_dialog_context->title),
                          &prompt_size);
    ReleaseDC(hwnd, measure_dc);
    GetClientRect(hwnd, &rect);
    prompt_size.cx += prompt_size.cy;
    prompt_size.cy += 5;
    MoveWindow(GetDlgItem(hwnd, 0x41b),
               (rect.right - prompt_size.cx) / 2,
               0x14,
               prompt_size.cx,
               prompt_size.cy,
               1);
    if (g_coin_flip_animation_dialog_context->show_coin_flips != 0)
    {
      if (g_coin_flip_animation_dialog_context->coin_result != 0)
      {
        sprintf(avi_path, "%s\\COINTOSS_Tails.AVI", global_duelart_path);
      }
      else
      {
        sprintf(avi_path, "%s\\COINTOSS_Heads.AVI", global_duelart_path);
      }
      g_coin_flip_animation_mci_hwnd = MCIWndCreateA(hwnd, g_app_instance, 0x50000102, avi_path);
      SendMessageA(g_coin_flip_animation_mci_hwnd, MCIWNDM_SETPALETTE, (WPARAM)global_cart_art_hpalette, 0);
      GetWindowRect(g_coin_flip_animation_mci_hwnd, &rect);
      AdjustWindowRect(&rect, GetWindowLongA(hwnd, GWL_STYLE), 0);
      MoveWindow(hwnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 1);
    }
    else
    {
      if (g_coin_flip_animation_dialog_context->coin_result != 0)
      {
        sprintf(avi_path, "%s\\COINTOSS_Heads.AVI", global_duelart_path);
      }
      else
      {
        sprintf(avi_path, "%s\\COINTOSS_Tails.AVI", global_duelart_path);
      }
      g_coin_flip_animation_mci_hwnd = MCIWndCreateA(hwnd, g_app_instance, 0x50000102, avi_path);
      SendMessageA(g_coin_flip_animation_mci_hwnd, MCIWNDM_SETPALETTE, (WPARAM)global_cart_art_hpalette, 0);
    }
    if (g_coin_flip_animation_dialog_context->show_coin_flips != 0)
    {
      if (SetTimer(hwnd, 1, 10, (TIMERPROC)0) == 0)
      {
        PostMessageA(hwnd, WM_TIMER, 1, 0);
      }
    }
    else
    {
      if (SetTimer(hwnd, 1, 1000, (TIMERPROC)0) == 0)
      {
        PostMessageA(hwnd, WM_TIMER, 1, 0);
      }
    }
    SetTimer(hwnd, 2, 15000, (TIMERPROC)0);
    SetFocus(hwnd);
    return 0;

  case WM_DESTROY:
    cleanup_coin_flip_animation_dialog_assets(g_coin_flip_animation_dialog_brush);
    return 0;

  case WM_CTLCOLORSTATIC:
    ctl_dc = (HDC)wparam;
    ApplyCardArtPaletteToDc(ctl_dc);
    ctl_hwnd = (HWND)lparam;
    ctl_id = GetDlgCtrlID(ctl_hwnd);
    SetBkMode(ctl_dc, TRANSPARENT);
    SetTextColor(ctl_dc, g_coin_flip_animation_dialog_text_color);
    return (INT_PTR)g_coin_flip_animation_dialog_brush;

  case 0x30f:
  case 0x310:
  case 0x311:
    return handle_button_palette_message(hwnd, msg, (HWND)wparam, lparam);

  case WM_ERASEBKGND:
    erase_dc = (HDC)wparam;
    ApplyCardArtPaletteToDc(erase_dc);
    GetClientRect(hwnd, &erase_rect);
    FillRect(erase_dc, &erase_rect, g_coin_flip_animation_dialog_brush);
    return 1;

  case WM_CLOSE:
  case WM_CHAR:
    KillTimer(hwnd, 2);
    if (g_coin_flip_animation_dialog_context->show_coin_flips != 0)
    {
      SendMessageA(g_coin_flip_animation_mci_hwnd, MCIWNDM_STOP, 0, 0);
    }
    SendMessageA(g_coin_flip_animation_mci_hwnd, WM_CLOSE, 0, 0);
    EndDialog(hwnd, 0);
    return 1;

  case WM_PARENTNOTIFY:
    if (((unsigned int)wparam & 0xffff) == WM_LBUTTONDOWN || ((unsigned int)wparam & 0xffff) == WM_RBUTTONDOWN)
    {
      KillTimer(hwnd, 2);
      if (g_coin_flip_animation_dialog_context->show_coin_flips != 0)
      {
        SendMessageA(g_coin_flip_animation_mci_hwnd, MCIWNDM_STOP, 0, 0);
      }
      SendMessageA(g_coin_flip_animation_mci_hwnd, WM_CLOSE, 0, 0);
      EndDialog(hwnd, 0);
    }
    return 1;

  case WM_KEYDOWN:
    if (lparam == 0x20d)
    {
      KillTimer(hwnd, 2);
      if (g_coin_flip_animation_dialog_context->show_coin_flips != 0)
      {
        SendMessageA(g_coin_flip_animation_mci_hwnd, MCIWNDM_STOP, 0, 0);
      }
      SendMessageA(g_coin_flip_animation_mci_hwnd, WM_CLOSE, 0, 0);
      EndDialog(hwnd, 0);
    }
    return 1;

  case WM_TIMER:
    KillTimer(hwnd, (UINT)wparam);
    if (wparam == 1)
    {
      if (g_coin_flip_animation_dialog_context->show_coin_flips != 0)
      {
        SendMessageA(g_coin_flip_animation_mci_hwnd, MCIWNDM_PLAY, 0, 0);
        play_sound_effect(0x3b);
      }
      else
      {
        ShowWindow(GetDlgItem(hwnd, 0x41c), 1);
      }
      SetFocus(hwnd);
    }
    else
    {
      if (g_coin_flip_animation_dialog_context->show_coin_flips != 0)
      {
        SendMessageA(g_coin_flip_animation_mci_hwnd, MCIWNDM_STOP, 0, 0);
      }
      SendMessageA(g_coin_flip_animation_mci_hwnd, WM_CLOSE, 0, 0);
      EndDialog(hwnd, 0);
    }
    return 1;

  default:
    return 0;
  }
}

// FUNCTION: MAGIC 0x0049a6c8
// FUNCTION: SHANDALAR 0x00536086
INT_PTR CALLBACK dlgproc_duel_redraw_ante(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  HDC paint_dc;
  PAINTSTRUCT paint;
  RECT card_rect;
  POINT mouse_point;
  RECT player_card_rect;
  RECT opponent_card_rect;
  HDC erase_dc;
  RECT erase_rect;
  int draw_focus;
  COLORREF draw_color;
  DRAWITEMSTRUCT *draw_item;
  HWND ctl_hwnd;
  int ctl_id;
  HDC ctl_dc;
  HWND invalidate_b;
  HWND invalidate_a;
  RECT dialog_rect;
  HDC measure_dc;
  int button_height;
  int button_width;
  HGDIOBJ button_font;
  RECT button_rect;
  SIZE text_size;
  int command_id;
  char text[500];
  int ante_info;
  char player_name[100];
  DuelRedrawAnteDialogContext *context;

  switch (msg)
  {
  case WM_INITDIALOG:
    g_redraw_ante_waiting_for_both_network_choices = 0;
    BringWindowToTop(g_duel_full_card_window_hwnd);
    context = (DuelRedrawAnteDialogContext *)lparam;
    SetWindowLongPtrA(hwnd, DWL_USER, (LONG_PTR)context);
    ShowWindow(GetDlgItem(hwnd, 0x448), SW_HIDE);
    ShowWindow(GetDlgItem(hwnd, 0x449), SW_HIDE);
    g_show_card_list_last_preview_hwnd = 0;
    load_redraw_ante_dialog_assets(&g_redraw_ante_dialog_background,
                                   &g_redraw_ante_dialog_text_color,
                                   &g_redraw_ante_dialog_button_normal_bitmap,
                                   &g_redraw_ante_dialog_button_depressed_bitmap,
                                   &g_redraw_ante_dialog_button_disabled_bitmap,
                                   &g_redraw_ante_dialog_button_normal_color,
                                   &g_redraw_ante_dialog_button_focus_color);
    copy_opponent_name_prefix(player_name);
    load_text(global_ui_strings_filename, "DIALOG_MULLIGAN");
    if (context->starting_player == 1)
    {
      sprintf(text, g_text_lines[0], player_name);
      SetDlgItemTextA(hwnd, 0x442, text);
    }
    else
    {
      SetDlgItemTextA(hwnd, 0x442, g_text_lines[1]);
    }
    sprintf(text, g_text_lines[2], player_name);
    SetDlgItemTextA(hwnd, 0x446, text);
    SetDlgItemTextA(hwnd, 0x447, g_text_lines[3]);
    if ((g_duel_network_flags & 2) == 0)
    {
      if (context->ante_result != 0)
      {
        if (context->ante_result == 1)
        {
          sprintf(text, g_text_lines[4], player_name);
        }
        else if (context->ante_result == 2)
        {
          sprintf(text, g_text_lines[5], player_name);
        }
        else
        {
          sprintf(text, g_text_lines[6], player_name);
        }
        SetDlgItemTextA(hwnd, 0x443, text);
      }
      else
      {
        sprintf(text, g_text_lines[7], player_name);
        SetDlgItemTextA(hwnd, 0x443, text);
      }
      if (context->ante_result != 0)
      {
        SetDlgItemTextA(hwnd, 0x444, g_text_lines[10]);
        ShowWindow(GetDlgItem(hwnd, 0x444), SW_SHOW);
        SetFocus(GetDlgItem(hwnd, 0x444));
        SendMessageA(hwnd, 0x401, 0x444, 0);
      }
      else
      {
        if (context->redraw_result != 0)
        {
          if (context->ante_result == 0)
          {
            ShowWindow(GetDlgItem(hwnd, 0x443), SW_HIDE);
          }
          SetDlgItemTextA(hwnd, 0x444, g_text_lines[10]);
          ShowWindow(GetDlgItem(hwnd, 0x444), SW_SHOW);
          SetFocus(GetDlgItem(hwnd, 0x444));
          SendMessageA(hwnd, 0x401, 0x444, 0);
        }
        else
        {
          ShowWindow(GetDlgItem(hwnd, 0x444), SW_HIDE);
          SetFocus(GetDlgItem(hwnd, IDOK));
          SendMessageA(hwnd, 0x401, IDOK, 0);
        }
      }
    }
    else
    {
      g_redraw_ante_opponent_choice_received = 0;
      if (context->redraw_result != 0 && context->ante_result != 0)
      {
        g_redraw_ante_waiting_for_both_network_choices = 1;
      }
      ShowWindow(GetDlgItem(hwnd, IDOK), SW_HIDE);
      if (context->redraw_result != 0 && (context->ante_result == 0 || g_manalink_is_host != 0))
      {
        SetDlgItemTextA(hwnd, 0x444, g_text_lines[10]);
        ShowWindow(GetDlgItem(hwnd, 0x444), SW_SHOW);
        SetFocus(GetDlgItem(hwnd, 0x444));
        SendMessageA(hwnd, 0x401, 0x444, 0);
        ShowWindow(GetDlgItem(hwnd, IDOK), SW_SHOW);
        ShowWindow(GetDlgItem(hwnd, 0x443), SW_HIDE);
      }
      else
      {
        if (context->ante_result != 0)
        {
          ShowWindow(GetDlgItem(hwnd, 0x444), SW_HIDE);
          redraw_ante_wait_for_opponent_done_thread(hwnd);
        }
        else
        {
          ShowWindow(GetDlgItem(hwnd, 0x443), SW_HIDE);
          ShowWindow(GetDlgItem(hwnd, 0x444), SW_HIDE);
          ShowWindow(GetDlgItem(hwnd, IDOK), SW_SHOW);
          SetFocus(GetDlgItem(hwnd, IDOK));
          SendMessageA(hwnd, 0x401, IDOK, 0);
        }
      }
    }
    context->opponent_mulligan_accepted = 0;
    if (context->ante_info != 0)
    {
      sprintf(text, g_text_lines[8], player_name);
    }
    else
    {
      sprintf(text, g_text_lines[9], player_name);
    }
    SetDlgItemTextA(hwnd, 0x445, text);
    ShowWindow(GetDlgItem(hwnd, 0x445), SW_HIDE);
    context->mulligan_accepted = 0;
    SetDlgItemTextA(hwnd, IDOK, g_text_lines[11]);

    measure_dc = GetDC(hwnd);
    ApplyCardArtPaletteToDc(measure_dc);
    button_font = (HGDIOBJ)SendDlgItemMessageA(hwnd, 0x444, WM_GETFONT, 0, 0);
    SelectObject(measure_dc, button_font);
    GetWindowRect(GetDlgItem(hwnd, 0x444), &button_rect);
    MapWindowPoints((HWND)0, hwnd, (LPPOINT)&button_rect, 2);
    GetDlgItemTextA(hwnd, 0x444, text, 200);
    GetTextExtentPoint32A(measure_dc, text, strlen(text), &text_size);
    button_width = text_size.cx + text_size.cy * 2;
    button_height = (text_size.cy * 5) / 2;
    button_rect.left = button_rect.left + ((button_rect.right - button_rect.left) / 2 - button_width / 2);
    MoveWindow(GetDlgItem(hwnd, 0x444),
               button_rect.left,
               button_rect.top,
               button_width,
               button_height,
               1);

    GetWindowRect(GetDlgItem(hwnd, IDOK), &button_rect);
    MapWindowPoints((HWND)0, hwnd, (LPPOINT)&button_rect, 2);
    GetDlgItemTextA(hwnd, IDOK, text, 200);
    GetTextExtentPoint32A(measure_dc, text, strlen(text), &text_size);
    button_width = text_size.cx + text_size.cy * 2;
    button_height = (text_size.cy * 5) / 2;
    button_rect.left = button_rect.left + ((button_rect.right - button_rect.left) / 2 - button_width / 2);
    MoveWindow(GetDlgItem(hwnd, IDOK),
               button_rect.left,
               button_rect.top,
               button_width,
               button_height,
               1);

    if (context->opponent_ante_card == -1)
    {
      ShowWindow(GetDlgItem(hwnd, 0x448), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, 0x446), SW_HIDE);
    }
    if (context->player_ante_card == -1)
    {
      ShowWindow(GetDlgItem(hwnd, 0x449), SW_HIDE);
      ShowWindow(GetDlgItem(hwnd, 0x447), SW_HIDE);
    }
    ReleaseDC(hwnd, measure_dc);
    change_buttonclass_wndproc(hwnd);
    if (((g_duel_network_flags & 2) == 0 && context->ante_result != 0) || context->redraw_result != 0)
    {
      GetWindowRect(hwnd, &dialog_rect);
      SetWindowPos(hwnd, (HWND)0, 5, dialog_rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
    return 0;

  case WM_COMMAND:
    command_id = (unsigned int)wparam & 0xffff;
    switch (command_id)
    {
    case IDOK:
    case IDCANCEL:
      context = (DuelRedrawAnteDialogContext *)GetWindowLongPtrA(hwnd, DWL_USER);
      if ((g_duel_network_flags & 2) != 0)
      {
        if (context->redraw_result != 0 || context->ante_result != 0)
        {
          g_network_result_packet.packet_type = 6;
          g_network_result_packet.result = 0;
          TENTATIVE_send_network_result(0, 6);
        }
        EnableWindow(GetDlgItem(hwnd, IDOK), 0);
        ShowWindow(GetDlgItem(hwnd, 0x444), SW_HIDE);
        if (context->redraw_result != 0 && g_redraw_ante_opponent_choice_received == 0)
        {
          redraw_ante_wait_for_opponent_mulligan_thread(hwnd);
        }
        else
        {
          SetTimer(hwnd, 1, 2000, (TIMERPROC)0);
        }
      }
      if ((g_duel_network_flags & 2) == 0)
      {
        cleanup_redraw_ante_dialog_assets(g_redraw_ante_dialog_background,
                                          g_redraw_ante_dialog_button_normal_bitmap,
                                          g_redraw_ante_dialog_button_depressed_bitmap,
                                          g_redraw_ante_dialog_button_disabled_bitmap);
        EndDialog(hwnd, context->mulligan_accepted);
      }
      break;

    case 0x444:
      context = (DuelRedrawAnteDialogContext *)GetWindowLongPtrA(hwnd, DWL_USER);
      context->mulligan_accepted = 1;
      EnableWindow(GetDlgItem(hwnd, 0x444), 0);
      ShowWindow(GetDlgItem(hwnd, IDOK), SW_HIDE);
      if ((g_duel_network_flags & 2) != 0)
      {
        g_network_result_packet.packet_type = 6;
        g_network_result_packet.result = 1;
        TENTATIVE_send_network_result(0, 6);
        if (context->redraw_result != 0 && g_redraw_ante_opponent_choice_received == 0)
        {
          redraw_ante_wait_for_opponent_mulligan_thread(hwnd);
        }
        else
        {
          SetTimer(hwnd, 1, 2000, (TIMERPROC)0);
        }
      }
      if ((g_duel_network_flags & 2) == 0)
      {
        if (context->ante_result == 0)
        {
          Sleep(500);
          ShowWindow(GetDlgItem(hwnd, 0x445), SW_SHOW);
          if (context->ante_info != 0)
          {
            SendMessageA(g_duel_life_window_hwnd, 0x40c, 0, 0);
          }
        }
        SetTimer(hwnd, 1, 2000, (TIMERPROC)0);
      }
      break;
    case 0x402:
      context = (DuelRedrawAnteDialogContext *)GetWindowLongPtrA(hwnd, DWL_USER);
      ante_info = context->ante_result;
      context->opponent_mulligan_accepted = lparam;
      g_redraw_ante_opponent_choice_received = 1;
      if ((context->redraw_result == 0 || g_redraw_ante_waiting_for_both_network_choices != 0) &&
          (g_redraw_ante_waiting_for_both_network_choices != 0 ||
           (lparam != 0 && context->mulligan_accepted == 0)))
      {
        g_redraw_ante_waiting_for_both_network_choices = 0;
        load_text(global_ui_strings_filename, "DIALOG_MULLIGAN");
        copy_opponent_name_prefix(player_name);
        if (context->opponent_mulligan_accepted != 0)
        {
          if (ante_info == 1)
          {
            sprintf(text, g_text_lines[4], player_name);
          }
          else if (ante_info == 2)
          {
            sprintf(text, g_text_lines[5], player_name);
          }
          else
          {
            sprintf(text, g_text_lines[6], player_name);
          }
          SetDlgItemTextA(hwnd, 0x443, text);
          ShowWindow(GetDlgItem(hwnd, 0x443), SW_SHOW);
        }
        SetDlgItemTextA(hwnd, 0x444, g_text_lines[10]);
        ShowWindow(GetDlgItem(hwnd, 0x444), SW_SHOW);
        SetFocus(GetDlgItem(hwnd, 0x444));
        SendMessageA(hwnd, 0x401, 0x444, 0);
        if (context->opponent_mulligan_accepted != 0)
        {
          g_duel_surface_reset_state = 1;
          resize_duel_hand_window(g_duel_life_window_hwnd);
        }
      }
      ShowWindow(GetDlgItem(hwnd, IDOK), SW_SHOW);
      break;

    case 0x401:
      context = (DuelRedrawAnteDialogContext *)GetWindowLongPtrA(hwnd, DWL_USER);
      context->opponent_mulligan_accepted = lparam;
      if (context->opponent_mulligan_accepted != 0)
      {
        load_text(global_ui_strings_filename, "DIALOG_MULLIGAN");
        copy_opponent_name_prefix(player_name);
        if (context->opponent_mulligan_accepted != 0)
        {
          sprintf(text, g_text_lines[8], player_name);
        }
        else
        {
          sprintf(text, g_text_lines[9], player_name);
        }
        SetDlgItemTextA(hwnd, 0x445, text);
        ShowWindow(GetDlgItem(hwnd, 0x445), SW_SHOW);
      }
      SetTimer(hwnd, 1, 2000, (TIMERPROC)0);
      break;
    }
    return 1;

  case 0x4c8:
    invalidate_a = (HWND)wparam;
    invalidate_b = (HWND)lparam;
    if (invalidate_a != (HWND)0)
    {
      SendMessageA(hwnd, 0x401, GetDlgCtrlID(invalidate_a), 0);
    }
    if (invalidate_a != (HWND)0)
    {
      InvalidateRect(invalidate_a, (RECT *)0, 1);
    }
    if (invalidate_b != (HWND)0)
    {
      InvalidateRect(invalidate_b, (RECT *)0, 1);
    }
    return 0;

  case WM_CTLCOLORSTATIC:
    ctl_dc = (HDC)wparam;
    ApplyCardArtPaletteToDc(ctl_dc);
    ctl_hwnd = (HWND)lparam;
    ctl_id = GetDlgCtrlID(ctl_hwnd);
    SetBkMode(ctl_dc, TRANSPARENT);
    SetTextColor(ctl_dc, g_redraw_ante_dialog_text_color);
    return (INT_PTR)GetStockObject(HOLLOW_BRUSH);

  case WM_DRAWITEM:
    draw_item = (DRAWITEMSTRUCT *)lparam;
    if (GetFocus() == draw_item->hwndItem)
    {
      draw_color = g_redraw_ante_dialog_button_focus_color;
    }
    else
    {
      draw_color = g_redraw_ante_dialog_button_normal_color;
    }
    draw_focus = 0;
    if (IsWindowVisible(GetDlgItem(hwnd, 0x444)) == 0)
    {
      draw_color = g_redraw_ante_dialog_button_normal_color;
      draw_focus = 1;
    }
    draw_duel_dialog_bitmap_button(draw_item,
                                   g_redraw_ante_dialog_button_normal_bitmap,
                                   g_redraw_ante_dialog_button_depressed_bitmap,
                                   g_redraw_ante_dialog_button_disabled_bitmap,
                                   draw_color,
                                   draw_focus);
    return 1;

  case 0x30f:
  case 0x310:
  case 0x311:
    return handle_button_palette_message(hwnd, msg, (HWND)wparam, lparam);

  case WM_ERASEBKGND:
    erase_dc = (HDC)wparam;
    ApplyCardArtPaletteToDc(erase_dc);
    GetClientRect(hwnd, &erase_rect);
    if (g_redraw_ante_dialog_background != (HBITMAP)0)
    {
      DrawBitmapToRect(erase_dc, &erase_rect, g_redraw_ante_dialog_background);
    }
    else
    {
      FillRect(erase_dc, &erase_rect, GetStockObject(2));
    }
    return 1;

  case WM_LBUTTONDOWN:
    SendMessageA(hwnd, WM_SYSCOMMAND, 0xf012, 0);
    return 1;

  case WM_MOUSEMOVE:
  case WM_RBUTTONDOWN:
    mouse_point.x = (unsigned int)lparam & 0xffff;
    mouse_point.y = (unsigned short)(((unsigned int)lparam >> 0x10) & 0xffff);
    context = (DuelRedrawAnteDialogContext *)GetWindowLongPtrA(hwnd, DWL_USER);
    if ((msg == WM_MOUSEMOVE && g_duel_interface_options.layout != 2) ||
        (msg == WM_RBUTTONDOWN && g_duel_interface_options.layout == 2))
    {
      GetWindowRect(GetDlgItem(hwnd, 0x448), &opponent_card_rect);
      MapWindowPoints((HWND)0, hwnd, (LPPOINT)&opponent_card_rect, 2);
      GetWindowRect(GetDlgItem(hwnd, 0x449), &player_card_rect);
      MapWindowPoints((HWND)0, hwnd, (LPPOINT)&player_card_rect, 2);
      if (context->opponent_ante_card != -1 &&
          PtInRect(&opponent_card_rect, mouse_point) != 0)
      {
        SendMessageA(g_duel_card_preview_window_hwnd, 0x401, context->opponent_ante_card, 0);
      }
      else if (context->player_ante_card != -1 &&
               PtInRect(&player_card_rect, mouse_point) != 0)
      {
        SendMessageA(g_duel_card_preview_window_hwnd, 0x401, context->player_ante_card, 0);
      }
    }
    return 0;

  case WM_PAINT:
    UpdateWindow(GetDlgItem(hwnd, 0x442));
    UpdateWindow(GetDlgItem(hwnd, 0x447));
    UpdateWindow(GetDlgItem(hwnd, 0x446));
    context = (DuelRedrawAnteDialogContext *)GetWindowLongPtrA(hwnd, DWL_USER);
    paint_dc = BeginPaint(hwnd, &paint);
    if (paint_dc != (HDC)0)
    {
      ApplyCardArtPaletteToDc(paint_dc);
      if (context->player_ante_card != -1)
      {
        GetWindowRect(GetDlgItem(hwnd, 0x449), &card_rect);
        MapWindowPoints((HWND)0, hwnd, (LPPOINT)&card_rect, 2);
        DrawFullCard(paint_dc, &card_rect, global_raw_cards_storage + context->player_ante_card, 0, 2, 0, gs_illus_00789130);
      }
      if (context->opponent_ante_card != -1)
      {
        GetWindowRect(GetDlgItem(hwnd, 0x448), &card_rect);
        MapWindowPoints((HWND)0, hwnd, (LPPOINT)&card_rect, 2);
        DrawFullCard(paint_dc, &card_rect, global_raw_cards_storage + context->opponent_ante_card, 0, 2, 0, gs_illus_00789130);
      }
      EndPaint(hwnd, &paint);
    }
    return 1;

  case WM_TIMER:
    context = (DuelRedrawAnteDialogContext *)GetWindowLongPtrA(hwnd, DWL_USER);
    command_id = wparam;
    if (command_id == 1)
    {
      cleanup_redraw_ante_dialog_assets(g_redraw_ante_dialog_background,
                                        g_redraw_ante_dialog_button_normal_bitmap,
                                        g_redraw_ante_dialog_button_depressed_bitmap,
                                        g_redraw_ante_dialog_button_disabled_bitmap);
      EndDialog(hwnd, context->mulligan_accepted);
    }
    else if (command_id == 2)
    {
      ShowWindow(GetDlgItem(hwnd, IDOK), SW_SHOW);
      SetFocus(GetDlgItem(hwnd, IDOK));
      SendMessageA(hwnd, 0x401, IDOK, 0);
    }
    return 1;

  default:
    return 0;
  }
}

// FUNCTION: MAGIC 0x0049bc58
// FUNCTION: SHANDALAR 0x0053760d
void redraw_ante_wait_for_opponent_mulligan_thread(void *hwnd)
{
  int network_choice;
  HWND thread_hwnd;

  thread_hwnd = (HWND)hwnd;
#ifndef SHANDALAR
  while (WaitForSingleObject(global_mutex_GameInit, 0xffffffff) != 0)
  {
  }
#endif
  TENTATIVE_wait_for_network_result(1, 6);
  network_choice = g_network_result_packet.result;
  if (thread_hwnd != (HWND)0)
  {
    SendMessageA(thread_hwnd, WM_COMMAND, 0x401, network_choice);
  }
#ifndef SHANDALAR
  ReleaseMutex(global_mutex_GameInit);
#endif
  return;
}

// FUNCTION: MAGIC 0x0049bccf
// FUNCTION: SHANDALAR 0x00537684
void redraw_ante_wait_for_opponent_done_thread(void *hwnd)
{
  int network_choice;
  HWND thread_hwnd;

  thread_hwnd = (HWND)hwnd;
#ifndef SHANDALAR
  while (WaitForSingleObject(global_mutex_GameInit, 0xffffffff) != 0)
  {
  }
#endif
  TENTATIVE_wait_for_network_result(1, 6);
  network_choice = g_network_result_packet.result;
  if (thread_hwnd != (HWND)0)
  {
    SendMessageA(thread_hwnd, WM_COMMAND, 0x402, network_choice);
  }
#ifndef SHANDALAR
  ReleaseMutex(global_mutex_GameInit);
#endif
  return;
}

// FUNCTION: MAGIC 0x0049bd46
// FUNCTION: SHANDALAR 0x005376fb
void load_redraw_ante_dialog_assets(HBITMAP *background,
                                    int *text_color,
                                    HBITMAP *normal_button_bitmap,
                                    HBITMAP *depressed_button_bitmap,
                                    HBITMAP *disabled_button_bitmap,
                                    COLORREF *normal_color,
                                    COLORREF *focus_color)
{
  char filename[264];

  sprintf(filename, "%s\\WINBK_StartDuel.pic", global_duelart_path);
  *background = load_pic(filename);
  *text_color = 0;
  sprintf(filename, "%s\\WINBK_StartDuelButtonNormal.pic", global_duelart_path);
  *normal_button_bitmap = load_pic(filename);
  sprintf(filename, "%s\\WINBK_StartDuelButtonDepressed.pic", global_duelart_path);
  *depressed_button_bitmap = load_pic(filename);
  sprintf(filename, "%s\\WINBK_StartDuelButtonDisabled.pic", global_duelart_path);
  *disabled_button_bitmap = load_pic(filename);
  *normal_color = 0x1000001;
  *focus_color = 0x10000bf;
}

// FUNCTION: MAGIC 0x0049be2a
// FUNCTION: SHANDALAR 0x005377df
void cleanup_redraw_ante_dialog_assets(HANDLE background,
                                       HANDLE normal_button_bitmap,
                                       HANDLE depressed_button_bitmap,
                                       HANDLE disabled_button_bitmap)
{
  if (background != 0)
  {
    delete_and_close_object(background);
  }
  if (normal_button_bitmap != 0)
  {
    delete_and_close_object(normal_button_bitmap);
  }
  if (depressed_button_bitmap != 0)
  {
    delete_and_close_object(depressed_button_bitmap);
  }
  if (disabled_button_bitmap != 0)
  {
    delete_and_close_object(disabled_button_bitmap);
  }
}

// FUNCTION: MAGIC 0x0049a5c5
// FUNCTION: SHANDALAR 0x00535f83
void load_coin_flip_dialog_assets(HBITMAP *background,
                                  int *text_color,
                                  HBITMAP *normal_button_bitmap,
                                  HBITMAP *depressed_button_bitmap,
                                  COLORREF *normal_color,
                                  COLORREF *focus_color)
{
  char filename[264];

  sprintf(filename, "%s\\WINBK_StartDuel2.pic", global_duelart_path);
  *background = load_pic(filename);
  *text_color = 0;
  sprintf(filename, "%s\\WINBK_StartDuelButtonNormal.pic", global_duelart_path);
  *normal_button_bitmap = load_pic(filename);
  sprintf(filename, "%s\\WINBK_StartDuelButtonDepressed.pic", global_duelart_path);
  *depressed_button_bitmap = load_pic(filename);
  *normal_color = 0x1000001;
  *focus_color = 0x10000bf;
}

// FUNCTION: MAGIC 0x0049a67b
// FUNCTION: SHANDALAR 0x00536039
void cleanup_coin_flip_dialog_assets(HANDLE background, HANDLE normal_button_bitmap, HANDLE depressed_button_bitmap)
{
  if (background != 0)
  {
    delete_and_close_object(background);
  }
  if (normal_button_bitmap != 0)
  {
    delete_and_close_object(normal_button_bitmap);
  }
  if (depressed_button_bitmap != 0)
  {
    delete_and_close_object(depressed_button_bitmap);
  }
}

// FUNCTION: MAGIC 0x00495a69
// FUNCTION: SHANDALAR 0x00466500
void draw_duel_dialog_bitmap_button(DRAWITEMSTRUCT *draw_item,
                                    HBITMAP normal_bitmap,
                                    HBITMAP pressed_bitmap,
                                    HBITMAP disabled_bitmap,
                                    COLORREF text_color,
                                    int draw_focus)
{
  HDC hdc;
  RECT focus_rect;
  HGDIOBJ font;
  RECT draw_rect;
  char text[200];
  SIZE text_size;

  hdc = draw_item->hDC;
  CopyRect(&draw_rect, &draw_item->rcItem);
  GetWindowTextA(draw_item->hwndItem, text, 200);
  ApplyCardArtPaletteToDc(hdc);
  OffsetRect(&draw_rect, -draw_item->rcItem.left, -draw_item->rcItem.top);
  if ((draw_item->itemState & ODS_SELECTED) != 0)
  {
    DrawBitmapToRect(hdc, &draw_rect, pressed_bitmap);
    OffsetRect(&draw_rect, 2, 2);
  }
  else
  {
    if ((draw_item->itemState & ODS_DISABLED) != 0 || (draw_item->itemState & ODS_GRAYED) != 0)
    {
      DrawBitmapToRect(hdc, &draw_rect, disabled_bitmap);
    }
    else
    {
      DrawBitmapToRect(hdc, &draw_rect, normal_bitmap);
    }
  }
  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, text_color);
  font = (HGDIOBJ)SendMessageA(draw_item->hwndItem, WM_GETFONT, 0, 0);
  SelectObject(hdc, font);
  DrawTextA(hdc, text, -1, &draw_rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  if (draw_focus != 0 && (draw_item->itemState & ODS_FOCUS) != 0)
  {
    GetTextExtentPoint32A(hdc, text, strlen(text), &text_size);
    focus_rect.left = (draw_rect.right - draw_rect.left) / 2 - text_size.cx / 2 - 3;
    focus_rect.right = focus_rect.left + text_size.cx + 6;
    focus_rect.top = (draw_rect.bottom - draw_rect.top) / 2 - text_size.cy / 2 - 3;
    focus_rect.bottom = focus_rect.top + text_size.cy + 6;
    DrawFocusRect(hdc, &focus_rect);
  }
}

// FUNCTION: MAGIC 0x0049a558
// FUNCTION: SHANDALAR 0x00535f16
void coin_flip_wait_for_network_choice_thread(void *hwnd)
{
  int network_choice;
  HWND thread_hwnd;

  thread_hwnd = (HWND)hwnd;
#ifndef SHANDALAR
  while (WaitForSingleObject(global_mutex_GameInit, 0xffffffff) != 0)
  {
  }
#endif
  TENTATIVE_wait_for_network_result(1, 5);
  network_choice = g_network_result_packet.result;
  SendMessageA(thread_hwnd, WM_COMMAND, 0x401, network_choice);
#ifndef SHANDALAR
  ReleaseMutex(global_mutex_GameInit);
#endif
  return;
}

// FUNCTION: MAGIC 0x00499984
// FUNCTION: SHANDALAR 0x00535344
INT_PTR CALLBACK dlgproc_duel_coin_flip(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  UINT timer_id;             // ebp - 0x1ac
  HDC hdc;                   // ebp - 0x1a8
  RECT erase_rect;           // ebp - 0x1a4
  COLORREF draw_color;       // ebp - 0x1a0
  DRAWITEMSTRUCT *draw_item; // ebp - 0x190
  HWND ctl_hwnd;             // ebp - 0x18c
  int ctl_id;                // ebp - 0x188
  HDC hdc_a;                 // ebp - 0x184
  HWND invalidate_b;         // ebp - 0x180
  HWND custom_hwnd;          // ebp - 0x17c
  int command_id;            // ebp - 0x178
  HDC dialog_dc;             // ebp - 0x174
  int button_height;         // ebp - 0x170
  int button_width;          // ebp - 0x16c
  int button_y;              // ebp - 0x168
  int button_x;              // ebp - 0x164
  HGDIOBJ button_font;       // ebp - 0x160
  RECT client_rect;
  SIZE text_size;
  RECT button_rect;
  int random_choice;
  char text[200];
  char player_name[100];
  DuelCoinFlipDialogContext *context;

  switch (msg)
  {
  case WM_INITDIALOG:
    context = (DuelCoinFlipDialogContext *)lparam;
    SetWindowLongPtrA(hwnd, DWL_USER, (LONG_PTR)context);
    load_coin_flip_dialog_assets(&g_coin_flip_dialog_background,
                                 &g_coin_flip_dialog_text_color,
                                 &g_coin_flip_dialog_button_normal_bitmap,
                                 &g_coin_flip_dialog_button_depressed_bitmap,
                                 &g_coin_flip_dialog_button_normal_color,
                                 &g_coin_flip_dialog_button_focus_color);
    copy_opponent_name_prefix(player_name);
    if ((g_duel_network_flags & 2) != 0)
    {
      context->play_draw_choice = -1;
    }
    load_text(global_ui_strings_filename, "DIALOG_PLAYORDRAW");
    if (context->coin_winner == 1)
    {
      if ((g_duel_network_flags & 2) != 0)
      {
        sprintf(text, g_text_lines[0], player_name);
        SetDlgItemTextA(hwnd, 0x4c4, text);
        ShowWindow(GetDlgItem(hwnd, 0x4c5), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, 0x4c6), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, 0x4c7), SW_HIDE);
        SetFocus(hwnd);
        coin_flip_wait_for_network_choice_thread(hwnd);
      }
      else
      {
        sprintf(text, g_text_lines[0], player_name);
        SetDlgItemTextA(hwnd, 0x4c4, text);
        random_choice = internal_rand(2);
        context->play_draw_choice = random_choice;
        if (context->was_random_starting_player)
        {
          sprintf(text, g_text_lines[0], player_name);
          SetDlgItemTextA(hwnd, 0x4c4, text);
          if (random_choice)
          {
            SetDlgItemTextA(hwnd, 0x4c7, g_text_lines[1]);
          }
          else
          {
            SetDlgItemTextA(hwnd, 0x4c7, g_text_lines[2]);
          }
          ShowWindow(GetDlgItem(hwnd, 0x4c7), SW_HIDE);
          SetTimer(hwnd, 1, 1000, (TIMERPROC)0);
        }
        else
        {
          ShowWindow(GetDlgItem(hwnd, 0x4c4), SW_HIDE);
          if (random_choice)
          {
            sprintf(text, g_text_lines[7], player_name);
          }
          else
          {
            sprintf(text, g_text_lines[8], player_name);
          }
          SetDlgItemTextA(hwnd, 0x4c7, text);
        }
        ShowWindow(GetDlgItem(hwnd, 0x4c5), SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, 0x4c6), SW_HIDE);
        SetFocus(hwnd);
        SetTimer(hwnd, 1, 1000, (TIMERPROC)0);
      }
    }
    else
    {
      if (context->was_random_starting_player)
      {
        SetDlgItemTextA(hwnd, 0x4c4, g_text_lines[3]);
      }
      else
      {
        ShowWindow(GetDlgItem(hwnd, 0x4c4), SW_HIDE);
      }
      SetDlgItemTextA(hwnd, 0x4c7, g_text_lines[4]);
      SetDlgItemTextA(hwnd, 0x4c5, g_text_lines[5]);
      SetDlgItemTextA(hwnd, 0x4c6, g_text_lines[6]);
      SetFocus(GetDlgItem(hwnd, 0x4c5));
      SendMessageA(hwnd, 0x401, 0x4c5, 0);
    }

    dialog_dc = GetDC(hwnd);
    ApplyCardArtPaletteToDc(dialog_dc);
    button_font = (HGDIOBJ)SendDlgItemMessageA(hwnd, 0x4c5, WM_GETFONT, 0, 0);
    SelectObject(dialog_dc, button_font);
    GetDlgItemTextA(hwnd, 0x4c5, text, 200);
    GetTextExtentPoint32A(dialog_dc, text, strlen(text), &text_size);
    button_width = text_size.cx;
    button_height = (text_size.cy * 5) / 2;
    GetDlgItemTextA(hwnd, 0x4c6, text, 200);
    GetTextExtentPoint32A(dialog_dc, text, strlen(text), &text_size);
    button_width = MAX(button_width, text_size.cx);
    button_width += text_size.cy * 2;
    GetWindowRect(GetDlgItem(hwnd, 0x4c5), &button_rect);
    MapWindowPoints((HWND)0, hwnd, (LPPOINT)&button_rect, 2);
    button_y = button_rect.top;
    GetClientRect(hwnd, &client_rect);
    button_x = client_rect.left + ((client_rect.right - client_rect.left) - button_width) / 2;
    SetWindowPos(GetDlgItem(hwnd, 0x4c5), (HWND)0, button_x, button_y, button_width, button_height, SWP_NOZORDER);
    button_y = button_y + button_height + button_height / 5;
    SetWindowPos(GetDlgItem(hwnd, 0x4c6), (HWND)0, button_x, button_y, button_width, button_height, SWP_NOZORDER);
    ReleaseDC(hwnd, dialog_dc);
    change_buttonclass_wndproc(hwnd);
    return 0;

  case WM_DESTROY:
    cleanup_coin_flip_dialog_assets(g_coin_flip_dialog_background,
                                    g_coin_flip_dialog_button_normal_bitmap,
                                    g_coin_flip_dialog_button_depressed_bitmap);
    return 0;

  case WM_COMMAND:
    switch (command_id = (unsigned int)wparam & 0xffff)
    {
    case 0x4c5:
    case 0x4c6:
      context = (DuelCoinFlipDialogContext *)GetWindowLongPtrA(hwnd, DWL_USER);
      if (command_id == 0x4c5)
      {
        context->play_draw_choice = 1;
      }
      else
      {
        context->play_draw_choice = 0;
      }
      EndDialog(hwnd, 1);
      break;
    case 0x401:
      context = (DuelCoinFlipDialogContext *)GetWindowLongPtrA(hwnd, DWL_USER);
      context->play_draw_choice = lparam;
      load_text(global_ui_strings_filename, "DIALOG_PLAYORDRAW");
      if (context->play_draw_choice != 0)
      {
        SetDlgItemTextA(hwnd, 0x4c7, g_text_lines[1]);
      }
      else
      {
        SetDlgItemTextA(hwnd, 0x4c7, g_text_lines[2]);
      }
      ShowWindow(GetDlgItem(hwnd, 0x4c7), SW_SHOW);
      SetTimer(hwnd, 2, 3000, (TIMERPROC)0);
      break;
    }
    return 1;

  case 0x4c8:
    custom_hwnd = (HWND)wparam;
    invalidate_b = (HWND)lparam;
    if (custom_hwnd != (HWND)0)
    {
      SendMessageA(hwnd, 0x401, GetDlgCtrlID(custom_hwnd), 0);
    }
    if (custom_hwnd != (HWND)0)
    {
      InvalidateRect(custom_hwnd, (RECT *)0, 1);
    }
    if (invalidate_b != (HWND)0)
    {
      InvalidateRect(invalidate_b, (RECT *)0, 1);
    }
    return 0;

  case WM_CTLCOLORSTATIC:
    hdc_a = (HDC)wparam;
    ApplyCardArtPaletteToDc(hdc_a);
    ctl_hwnd = (HWND)lparam;
    ctl_id = GetDlgCtrlID(ctl_hwnd);
    SetBkMode(hdc_a, TRANSPARENT);
    SetTextColor(hdc_a, g_coin_flip_dialog_text_color);
    return (INT_PTR)GetStockObject(HOLLOW_BRUSH);

  case WM_DRAWITEM:
    draw_item = (DRAWITEMSTRUCT *)lparam;
    if (GetFocus() == draw_item->hwndItem)
    {
      draw_color = g_coin_flip_dialog_button_focus_color;
    }
    else
    {
      draw_color = g_coin_flip_dialog_button_normal_color;
    }
    draw_duel_dialog_bitmap_button(draw_item,
                                   g_coin_flip_dialog_button_normal_bitmap,
                                   g_coin_flip_dialog_button_depressed_bitmap,
                                   g_coin_flip_dialog_button_normal_bitmap,
                                   draw_color,
                                   0);
    return 1;

  case 0x30f:
  case 0x310:
  case 0x311:
    return handle_button_palette_message(hwnd, msg, (HWND)wparam, lparam);

  case WM_ERASEBKGND:
    hdc = (HDC)wparam;
    ApplyCardArtPaletteToDc(hdc);
    GetClientRect(hwnd, &erase_rect);
    if (g_coin_flip_dialog_background != (HBITMAP)0)
    {
      DrawBitmapToRect(hdc, &erase_rect, g_coin_flip_dialog_background);
    }
    else
    {
      FillRect(hdc, &erase_rect, GetStockObject(2));
    }
    return 1;

  case WM_TIMER:
    timer_id = wparam;
    if (timer_id == 1)
    {
      KillTimer(hwnd, 1);
      ShowWindow(GetDlgItem(hwnd, 0x4c7), SW_SHOW);
      SetTimer(hwnd, 2, 3000, (TIMERPROC)0);
    }
    else if (timer_id == 2)
    {
      KillTimer(hwnd, 2);
      EndDialog(hwnd, 1);
    }
    return 0;

  case WM_CLOSE:
    if (IsWindowVisible(GetDlgItem(hwnd, 0x4c5)) == 0)
    {
      KillTimer(hwnd, 2);
      EndDialog(hwnd, 1);
      return 1;
    }
    else
    {
      return 1;
    }

  case WM_CHAR:
  case WM_LBUTTONDOWN:
  case WM_RBUTTONDOWN:
    if (g_duel_network_flags & 2)
    {
      context = (DuelCoinFlipDialogContext *)GetWindowLongPtrA(hwnd, DWL_USER);
      if (context->play_draw_choice == -1)
      {
        return 0;
      }
    }
    if (IsWindowVisible(GetDlgItem(hwnd, 0x4c5)) == 0)
    {
      KillTimer(hwnd, 2);
      EndDialog(hwnd, 1);
      return 1;
    }
    else
    {
      return 0;
    }
  default:
    return 0;
  }
}

// FUNCTION: MAGIC 0x004996d0
// FUNCTION: SHANDALAR 0x00535090
int run_duel_coin_flip_dialogs(unsigned int *starting_player,
                               unsigned int *mulligan_accepted,
                               unsigned int starting_player_value,
                               int random_starting_player,
                               int opponent_ante_card,
                               int player_ante_card,
                               int redraw_result,
                               int ante_result,
                               int ante_info,
                               int *out_flag)
{
  unsigned int mulligan_result;
  DuelRedrawAnteDialogContext redraw_ante_context;
  DuelCoinFlipDialogContext coin_flip_context;

  if (starting_player == (unsigned int *)0 || mulligan_accepted == (unsigned int *)0)
  {
    return 0;
  }

  load_text(global_ui_strings_filename, "DIALOG_STARTCOINFLIP");
  if (random_starting_player != 0)
  {
    if ((g_duel_network_flags & 2) != 0)
    {
      coin_flip_context.coin_flip_result = coin_flip(starting_player_value, g_text_lines[0], 0);
      if (g_manalink_is_host != 0)
      {
        coin_flip_context.coin_winner = 1 - coin_flip_context.coin_flip_result;
      }
      else
      {
        coin_flip_context.coin_winner = coin_flip_context.coin_flip_result;
      }
    }
    else
    {
      if (coin_flip(starting_player_value, g_text_lines[0], 0) == 0)
      {
        coin_flip_context.coin_winner = 1;
      }
      else
      {
        coin_flip_context.coin_winner = 0;
      }
    }
    coin_flip_context.was_random_starting_player = 1;
  }
  else
  {
    coin_flip_context.coin_winner = starting_player_value;
    coin_flip_context.was_random_starting_player = 0;
  }

  if (g_starting_player_was_random != 0)
  {
    coin_flip_context.coin_flip_result =
        DialogBoxParamA(g_app_instance, (LPCSTR)0xf4, g_duel_window_hwnd, dlgproc_duel_coin_flip, (LPARAM)&coin_flip_context);
    if ((g_duel_network_flags & 2) != 0 && coin_flip_context.coin_winner == 0)
    {
      g_network_result_packet.packet_type = 5;
      g_network_result_packet.result = coin_flip_context.play_draw_choice;
      TENTATIVE_send_network_result(0, 5);
    }
    set_player_directive_value(1, 0);
  }
  else
  {
    coin_flip_context.play_draw_choice = 1;
  }

  shuffle_duel_library(0, 0);
  shuffle_duel_library(1, 1);
  TENTATIVE_reassess_all_cards(0, 0x30);
  if ((coin_flip_context.coin_winner == 1 && coin_flip_context.play_draw_choice != 0) ||
      (coin_flip_context.coin_winner == 0 && coin_flip_context.play_draw_choice == 0))
  {
    redraw_ante_context.starting_player = 1;
  }
  else
  {
    redraw_ante_context.starting_player = 0;
  }

  redraw_ante_context.opponent_ante_card = CardIDFromType(opponent_ante_card);
  redraw_ante_context.player_ante_card = CardIDFromType(player_ante_card);
  redraw_ante_context.redraw_result = redraw_result;
  redraw_ante_context.ante_result = ante_result;
  redraw_ante_context.ante_info = ante_info;
  if (ante_result != 0 && (g_duel_network_flags & 2) == 0)
  {
    g_duel_surface_reset_state = 1;
    resize_duel_hand_window(g_duel_life_window_hwnd);
  }

  coin_flip_context.coin_flip_result =
      DialogBoxParamA(g_app_instance, (LPCSTR)0xe3, g_duel_window_hwnd, dlgproc_duel_redraw_ante, (LPARAM)&redraw_ante_context);
  mulligan_result = (coin_flip_context.coin_flip_result != 0);
  *starting_player = redraw_ante_context.starting_player;
  *mulligan_accepted = mulligan_result;
  *out_flag = redraw_ante_context.opponent_mulligan_accepted;
  g_duel_surface_reset_state = 0;
  resize_duel_hand_window(g_duel_life_window_hwnd);
  if (mulligan_result != 0)
  {
    PostMessageA(g_duel_full_card_window_hwnd, 0x40c, 0, 0);
  }
  UpdateWindow(g_duel_window_hwnd);
  return 1;
}
