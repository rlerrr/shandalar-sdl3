#ifdef MODERN_FIXES
#include "platform/winuser_compat.h"
#else
#include <windows.h>
#endif
#include <string.h>
#include <stddef.h>

#include "defs.h"
#include "shandalar.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "magic/src/game_support.h"
#include "magic/src/network.h"
#include "shandalar_global_strings.h"
#include "facemaker/src/facemaker_types.h"
#include "drawcardlib/src/pic.h"

extern int global_screen_width;
extern int global_screen_height;
extern int g_mouse_x_snapshot;
extern int g_mouse_y_snapshot;
extern int g_mouse_button_mask_snapshot;
extern char g_ui_message_buffer[0x1000];
extern HDC global_main_hdc;
extern HPALETTE g_realized_palette_handle;
extern RpBitsPalettePacket g_palette_data_words;

extern FacemakerWindowBounds *g_page0_window_bounds;
extern FacemakerWindowBounds *g_page1_window_bounds;

int internal_rand(int max_exclusive);
int ScaleUiCoordinateFrom320(int value);

struct PaletteLog *ReadPalette(char *palette_text_path, char *palette_binary_path);
int ClearGraphicsPageWithPaletteColor(int page_number, int palette_color);
void LoadPcxResource(int page_number, int x, int y, char *path, void *opaque);
void LoadPcxIntoPageNoPalette(char *path);
void StretchBlitGraphicsRect(FacemakerWindowBounds *dst, int dst_x, int dst_y, int src_w, int src_h,
                             FacemakerWindowBounds *src, int src_x, int src_y, int copy_w, int copy_h);
void BlitGraphicsRect(FacemakerWindowBounds *dst, int dst_x, int dst_y, int width, int height,
                      FacemakerWindowBounds *src, int src_x, int src_y);

int ReadSpriteEntryPointers(EncodedImage **out_sprite_entries, char *sprite_path);
void DrawEncodedImageResampled(FacemakerWindowBounds *dst, int x, int y, int width, int height, EncodedImage *encoded_image);
void FreeSpriteBlob(void *sprite_blob);

void DrawAdventureCard(int card_index, int x, int y, int full_card, char *banner_label);
void DrawAdventureCardSized(int card_index, int x, int y, int width, int height, int full_card, char *banner_label);
void DestroyCachedCardArt(void);

void DrawCenteredTextLineWithShadow(char *text, int center_x, int y, int color_index);
void DrawTextAt(FacemakerWindowBounds *window, int color, int x, int y, char *text, ...);
void DrawScaledTextNoShadow(char *text, int x, int y, int color_index);

void UpdateMouseSnapshot(void);
int HasQueuedKeyInput(void);
int PopNormalizedQueuedKeyInput(void);
void ClearInputAndWaitForMouseRelease(void);

typedef ptrdiff_t INT_PTR;

// FUNCTION: SHANDALAR 0x0055837e
INT_PTR __cdecl ShowAdventureListCardList(int *card_ids, int card_count, void *title, int require_card_click, char *out_selection)
{
  if (g_duel_ai_mode_state == 1)
  {
    return 1;
  }

  return show_cardlist(card_ids, 0, 0, card_count, title, (unsigned int)require_card_click, out_selection);
}

// FUNCTION: SHANDALAR 0x0056a515
int SelectAdventureListCardIndex(int player, int *card_ids, int card_count, char *title, int require_card_click, int *out_selection)
{
  char *saved_accept_keys;
  int accept_key;
  int key_code;
  char *accept_keys;
  int sprite_copy_index;
  EncodedImage *sprite_load_buffer[13];
  EncodedImage *unused_sprite_row_3[3];
  EncodedImage *unused_sprite_row_2[3];
  EncodedImage *unused_sprite_2;
  EncodedImage *unused_sprite_row_1[3];
  EncodedImage *buy_button_sprite;
  int hover_index;
  EncodedImage *unused_sprite_1;
  int has_input;
  int y_step;
  int selected_index;
  int row_count;
  int card_draw_y[500];
  int card_draw_x[500];
  int y;
  int x;
  int visible_count;
  int card_indices[500];
  int i;
  int result;

  if ((player == g_other_player) &&
      ((g_duel_network_flags & 2) != 0) &&
      (g_duel_active != 0))
  {
    TENTATIVE_wait_for_network_result(player, 0x19);
    return g_network_result_value;
  }

  if (((player == g_other_player) &&
       ((g_duel_network_flags & 2) == 0)) ||
      (g_duel_ai_mode_state == 1))
  {
    visible_count = 0;
    for (i = 0; card_count > i; i = i + 1)
    {
      if (card_ids[i] != -1)
      {
        card_indices[visible_count] = i;
        visible_count = visible_count + 1;
      }
    }

    g_ai_recorded_choice = internal_rand(visible_count);
    if (player != g_active_player)
    {
      if (g_duel_ai_mode_state == 1)
      {
        record_ai_action_selection();
      }
      else
      {
        replay_ai_action_selection();
      }
    }

    return card_indices[g_ai_recorded_choice];
  }

  if (g_duel_active == 0)
  {
    ReadPalette("todpal.tr", (char *)0);
    ClearGraphicsPageWithPaletteColor(0, 0);
    SelectPalette(global_main_hdc, g_realized_palette_handle, FALSE);

    LoadPcxIntoPageNoPalette("advfac64.pic");
    LoadPcxResource(1, 0, 0, "seedeck.pic", &g_palette_data_words);
    StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x280, 0x1e0, g_page0_window_bounds, 0, 0, global_screen_width, global_screen_height);

    x = 0;
    row_count = 0;
    for (i = 0; card_count > i; i = i + 1)
    {
      if ((card_ids[i] != -1) &&
          ((i == 0) || (card_ids[i - 1] != card_ids[i])))
      {
        row_count = row_count + 1;
      }
    }

    visible_count = (row_count - 1) / 5;
    if (visible_count == 0)
    {
      visible_count = 1;
    }
    y_step = 0x54 / visible_count;

    x = 0x60;
    visible_count = 0;
    y = 0x10;
    for (i = 0; card_count > i; i = i + 1)
    {
      if ((card_ids[i] != -1) &&
          ((i == 0) || (card_ids[i - 1] != card_ids[i])))
      {
        card_draw_x[visible_count] = x + 4;
        card_draw_y[visible_count] = y;
        card_indices[visible_count] = i;
        visible_count = visible_count + 1;

        x = x + 0x38;
        if (x >= 0x140)
        {
          x = 0x60;
          y = y + y_step;
        }
      }
    }

    DrawCenteredTextLineWithShadow(title, 0xa0, 2, 0xff);
    hover_index = -1;
    has_input = 0;

    ReadSpriteEntryPointers(&sprite_load_buffer[0], "BuyButtons.spr");
    buy_button_sprite = sprite_load_buffer[0];
    unused_sprite_1 = sprite_load_buffer[1];
    unused_sprite_2 = sprite_load_buffer[2];
    for (sprite_copy_index = 0; sprite_copy_index < 3; sprite_copy_index = sprite_copy_index + 1)
    {
      unused_sprite_row_1[sprite_copy_index] = sprite_load_buffer[sprite_copy_index + 3];
    }
    for (sprite_copy_index = 0; sprite_copy_index < 3; sprite_copy_index = sprite_copy_index + 1)
    {
      unused_sprite_row_2[sprite_copy_index] = sprite_load_buffer[sprite_copy_index + 6];
    }
    for (sprite_copy_index = 0; sprite_copy_index < 3; sprite_copy_index = sprite_copy_index + 1)
    {
      unused_sprite_row_3[sprite_copy_index] = sprite_load_buffer[sprite_copy_index + 9];
    }
    for (i = 0; visible_count > i; i = i + 1)
    {
      DrawAdventureCard(card_ids[card_indices[i]] & 0xfff, card_draw_x[i], card_draw_y[i], 0, "");
    }

  get_input:
    do
    {
      UpdateMouseSnapshot();
      selected_index = -1;
      g_mouse_x_snapshot = (g_mouse_x_snapshot * 0x140) / global_screen_width;
      g_mouse_y_snapshot = (g_mouse_y_snapshot * 0xf0) / global_screen_height;

      for (i = 0; visible_count > i; i = i + 1)
      {
        if (card_draw_x[i] > g_mouse_x_snapshot || g_mouse_x_snapshot >= card_draw_x[i] + 0x30)
        {
          continue;
        }

        if (card_draw_y[i] > g_mouse_y_snapshot || g_mouse_y_snapshot >= card_draw_y[i] + 0x30)
        {
          continue;
        }

        selected_index = card_indices[i];
      }

      if ((selected_index != -1) &&
          (card_ids[selected_index] != card_ids[hover_index]))
      {
        DrawAdventureCard(card_ids[selected_index] & 0xfff, 8, 0x40, 1, "");
        hover_index = selected_index;
      }

      if (require_card_click != 0)
      {
        if (((g_mouse_button_mask_snapshot != 0) ||
             (HasQueuedKeyInput() != 0)) &&
            (selected_index != -1))
        {
          has_input = 1;
        }
        else
        {
          has_input = 0;
        }
      }
      else if ((g_mouse_button_mask_snapshot != 0) ||
               (HasQueuedKeyInput() != 0))
      {
        has_input = 1;
      }
      else
      {
        has_input = 0;
      }
    } while (has_input == 0);

    if (require_card_click != 0)
    {

      BlitGraphicsRect(g_page0_window_bounds,
                       ScaleUiCoordinateFrom320(0xdc) / 2,
                       ScaleUiCoordinateFrom320(0x34) / 2,
                       ScaleUiCoordinateFrom320(0xc5) / 2,
                       ScaleUiCoordinateFrom320(0x111) / 2,
                       g_page1_window_bounds,
                       ScaleUiCoordinateFrom320(0xdc) / 2,
                       ScaleUiCoordinateFrom320(0x34) / 2);

      strcpy(g_ui_message_buffer, gs_showcard_text_0077e110.title);
      DrawEncodedImageResampled(g_page0_window_bounds,
                                ScaleUiCoordinateFrom320(0xdc) / 2,
                                ScaleUiCoordinateFrom320(0x34) / 2,
                                ScaleUiCoordinateFrom320(0xc5) / 2,
                                ScaleUiCoordinateFrom320(0x10f) / 2,
                                buy_button_sprite);
      DrawAdventureCardSized(card_ids[selected_index] & 0xfff, 0x7a, 0x29, 0x4b, 0x70, 1, "");

      g_page0_window_bounds->font_slot = 1;
      DrawScaledTextNoShadow(g_ui_message_buffer, 0x76, 0x20, 0x1b);

      key_code = PopNormalizedQueuedKeyInput();
      accept_keys = gs_showcard_text_0077e110.accept_keys;
      accept_key = 0;
      while (*accept_keys != '\0')
      {
        saved_accept_keys = accept_keys;
        accept_keys = accept_keys + 1;
        if (*saved_accept_keys == key_code)
        {
          accept_key = 1;
        }
      }

      if (accept_key == 0)
      {
        BlitGraphicsRect(g_page1_window_bounds,
                         ScaleUiCoordinateFrom320(0xdc) / 2,
                         ScaleUiCoordinateFrom320(0x34) / 2,
                         ScaleUiCoordinateFrom320(0xc5) / 2,
                         ScaleUiCoordinateFrom320(0x111) / 2,
                         g_page0_window_bounds,
                         ScaleUiCoordinateFrom320(0xdc) / 2,
                         ScaleUiCoordinateFrom320(0x34) / 2);
        ClearInputAndWaitForMouseRelease();
        goto get_input;
      }
    }

    FreeSpriteBlob(buy_button_sprite);
    DestroyCachedCardArt();
    return selected_index;
  }
  else
  {
    result = (int)ShowAdventureListCardList(card_ids, card_count, title, require_card_click, (char *)out_selection);
  }

  if ((player == g_active_player) &&
      ((g_duel_network_flags & 2) != 0))
  {
    g_network_result_value = result;
    g_network_result_packet_type = 0x19;
    TENTATIVE_send_network_result(player, 0x19);
  }

  return result;
}
