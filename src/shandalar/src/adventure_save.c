#include "platform/winuser_compat.h"
#include "platform/cheat_options.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
#include <errno.h>

#include "platform/legacy_io.h"
#include "defs.h"
#include "shandalar.h"
#include "shandalar_internal.h"
#include "magic/src/global_duel_ui_ids.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "magic/src/global_other.h"
#include "magic/src/game_support.h"
#include "magic/src/duel_engine.h"
#include "magic/src/shared_startup.h"
#include "shandalar_global_strings.h"
#include "cardartlib/src/assert.h"
#include "cardartlib/src/palette.h"
#include "deckdll/src/card_db.h"
#include "deckdll/src/magsnd.h"
#include "deckdll/src/shared_resources.h"
#include "facemaker/src/facemaker_types.h"
#include "statwin/statwin.h"
#include <drawcardlib/src/pic.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// FUNCTION: SHANDALAR 0x005019ad
void SaveGameToSlot(int save_slot_index)
{
  int save_drive_index;
  g_save_errno = 0;
  global_saveload_loading = g_save_errno;
  HideMouseCursorNested();
  save_drive_index = GetSaveDriveIndex();
  if (save_drive_index != -1)
  {
    if (save_slot_index == -1)
    {
      ShowMouseCursorNested();
      save_slot_index = RunTextMenuAt(g_ui_message_buffer, 0x30, 0x20);
      HideMouseCursorNested();
    }

    if (save_slot_index != -1)
    {
      g_selected_save_slot_index = save_slot_index;
      g_save_file_path[7] = (char)int_to_hex_digit(save_slot_index);
      if (SaveGameWithMessage(g_save_file_path) != 0)
      {
        if (g_save_errno == 0)
        {
          strcpy(g_ui_message_buffer, " Game has been saved.\n");
        }
        else
        {
          strcpy(g_ui_message_buffer, " Game NOT saved.\n");
          FillGraphicsRect(g_page0_window_bounds, 0x40, 0x7f, 0xc0, 0x22, 0xc);
        }

        if (g_save_errno == 0xd)
        {
          strcat(g_ui_message_buffer, " Write access denied.\n");
        }

        if (g_save_errno == 0x1c)
        {
          strcat(g_ui_message_buffer, " Disk Full.\n");
        }
        strcat(g_ui_message_buffer, " Press key to continue.\n");
      }
    }
  }
  ShowMouseCursorNested();
}

// FUNCTION: SHANDALAR 0x005031a8
int RunLoadGameMenu(void)
{
  return RunLoadSaveMenu(0);
}

// FUNCTION: SHANDALAR 0x00501b7d
int GetSaveDriveIndex(void)
{
  char current_directory_buffer[0x104];

  if (g_save_path_needs_init == -1)
  {
    GetCurrentDirectoryA(0x100, current_directory_buffer);
    g_save_file_path[0] = current_directory_buffer[0];
  }

  return tolower(g_save_file_path[0]) - 0x61;
}

// FUNCTION: SHANDALAR 0x00501e44
int LoadGameFromPath(char *save_file_path)
{
  int saved_clip_rect[4];
  FacemakerWindowBounds page4_bounds;
  FacemakerWindowBounds *page4_bounds_ptr;
  DIBSurface *page4_dib;
  int image_height;
  int image_width;
  int slot_index;
  int player_index;

  strcpy(save_file_path + 9, "SVE");
  g_save_file_fd = _open(save_file_path, 0x8000);
  if (g_save_file_fd == -1)
  {
    strcpy(g_ui_message_buffer, "File Error: ");
    strcat(g_ui_message_buffer, save_file_path);
    strcat(g_ui_message_buffer, "\n");
    RunTextMenuAt(g_ui_message_buffer, 100, 0x50);
    return 0;
  }

  global_saveload_loading = 1;
  save_or_load_ver1();
  _close(g_save_file_fd);

  for (player_index = 0; player_index < 2; player_index = player_index + 1)
  {
    for (slot_index = 0; slot_index < 0x96; slot_index = slot_index + 1)
    {
      if (global_card_instances[player_index][slot_index].internal_card_id != -1)
      {
        g_active_cards_count[player_index] = slot_index;
      }
    }
  }

  if (g_duel_active == 0)
  {
    HideMouseCursorNested();
    strcpy(save_file_path + 9, "map");
    LoadPcxIntoPage(2, save_file_path);
    ShowMouseCursorNested();
  }

  page4_bounds.page_number = 4;
  page4_bounds.clip_left = 0;
  page4_bounds.clip_top = 0;
  page4_bounds.max_x = 800;
  page4_bounds.max_y = 600;
  page4_bounds.draw_shadow_enabled = 1;
  page4_bounds.text_color = 0xf;
  page4_bounds.unk_1c = 4;
  page4_bounds.font_slot = 0;
  page4_bounds_ptr = &page4_bounds;

  image_width = 0x8a;
  image_height = 0xaa;
  if (g_facemaker_page4_dib == 0)
  {
    page4_dib = (DIBSurface *)CreateGraphicsPage(4, image_width * 2, image_height, 8);
    SetGraphicsPage(4, page4_dib);
  }
  else
  {
    g_graphics_pages[4] = g_facemaker_page4_dib;
  }

  PushGraphicsClipRect((AdvMenuRect *)saved_clip_rect, page4_bounds_ptr, 0, 0, image_width * 2, image_height);
  FillGraphicsRect(page4_bounds_ptr, 0, 0, image_width * 2, image_height, 0);
  strcpy(save_file_path + 9, "fce");
  LoadPcxIntoPage(4, save_file_path);
  BuildFacemakerPortraitSprites(&page4_bounds);
  LoadPcxIntoPage(4, save_file_path);
  g_facemaker_page4_bitmap = g_graphics_pages[4]->hBitmap;
  g_facemaker_page4_dib = g_graphics_pages[4];
  SelectObject(g_graphics_pages[4]->hTempDC, g_graphics_pages[4]->hPreviousBitmap);
  g_graphics_pages[4] = (DIBSurface *)0;
  return 1;
}

// FUNCTION: SHANDALAR 0x005020fe
int SaveGameToPath(char *save_file_path)
{
  FacemakerWindowBounds page4_bounds;
  FacemakerWindowBounds *page4_bounds_ptr;

  if (g_duel_active == 0)
  {
    strcpy(save_file_path + 9, "map");
    if (ExportGraphicsPage(2, save_file_path) != 0)
    {
      strcpy(g_ui_message_buffer, "Error writing map file.\n");
      RunTextMenuAt(g_ui_message_buffer, 4, 0x40);
      g_save_errno = 1;
      return 0;
    }
  }

  strcpy(save_file_path + 9, "SVE");
  g_save_file_fd = _open(save_file_path, 0x8301, 0x80);
  if (g_save_file_fd == -1)
  {
    strcpy(g_ui_message_buffer, "File Error: ");
    strcat(g_ui_message_buffer, save_file_path);
    strcat(g_ui_message_buffer, "\n");
    RunTextMenuAt(g_ui_message_buffer, 100, 0x50);
    return 0;
  }

  global_saveload_loading = 0;
  save_or_load_ver1();
  _close(g_save_file_fd);

  page4_bounds.page_number = 4;
  page4_bounds.clip_left = 0;
  page4_bounds.clip_top = 0;
  page4_bounds.max_x = 800;
  page4_bounds.max_y = 600;
  page4_bounds.draw_shadow_enabled = 1;
  page4_bounds.text_color = 0xf;
  page4_bounds.unk_1c = 4;
  page4_bounds.font_slot = 0;
  page4_bounds_ptr = &page4_bounds;

  g_graphics_pages[4] = g_facemaker_page4_dib;
  SelectObject(g_graphics_pages[4]->hTempDC, g_facemaker_page4_bitmap);

  strcpy(save_file_path + 9, "fce");
  ExportEncodedImage(4, 0, 0, g_facemaker_page4_dib->width, g_facemaker_page4_dib->height, 0, save_file_path);

  SelectObject(g_graphics_pages[4]->hTempDC, g_graphics_pages[4]->hPreviousBitmap);
  g_graphics_pages[4] = (DIBSurface *)0;

  if (g_save_errno != 0)
  {
    return 0;
  }

  return 1;
}

// FUNCTION: SHANDALAR 0x00501aec
int SaveGameWithMessage(char *save_file_path)
{
  strcpy(g_ui_message_buffer, " ");
  strcat(g_ui_message_buffer, "\n ... save in progress.\n");
  SaveGameToPath(save_file_path);
  return 1;
}

// FUNCTION: SHANDALAR 0x005226e0
int LoadStatWinDllExports(void)
{
  return 1;
}

// FUNCTION: SHANDALAR 0x004ed135
int RestoreAdventureUiPaletteAndFocus(void)
{
  SetFocus(g_main_window_hwnd);
  LoadPcxIntoPageNoPalette("advfac64.pic");
  ReadPalette("todpal.tr", (char *)0);
  SelectPalette(g_graphics_pages[0]->hTempDC, g_palette_handle, FALSE);
  RealizePalette(g_graphics_pages[0]->hTempDC);
  RecountDeckCardTotals();
  ClearInputAndWaitForMouseRelease();
  return 0;
}

// FUNCTION: SHANDALAR 0x0050318e
int RunSaveMenuAndSelectSlot(void)
{
  return RunLoadSaveMenu(1);
}

// FUNCTION: SHANDALAR 0x00522786
void UnloadStatWinDllExports(void)
{
  statwin_shutdown();
}
