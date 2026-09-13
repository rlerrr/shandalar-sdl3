#ifdef MODERN_FIXES
#include "platform/winuser_compat.h"
#else
#include <windows.h>
#endif
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "defs.h"
#include "shandalar.h"
#include "shandalar_internal.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "shandalar_global_strings.h"
#include "facemaker/src/facemaker_types.h"

// External engine state (defined in shandalar.c / other modules)


// External functions


char *GetTownCardDescription(int town_index);

// From city-info helpers
DWORD __cdecl FormatMessageFromStringStripCarriageReturns(char *dst, DWORD dst_len, LPCVOID format, ...);
char *BuildTownDisplayName(int town_index);
int __cdecl DrawCityInfoTownRow(FacemakerWindowBounds *dst, int town_index, int x, int y);
void __cdecl DrawFormattedTextNoShadowCentered(FacemakerWindowBounds *dst, int text_color, int x, int y, char *format, ...);

// GLOBAL: SHANDALAR 0x00603a34
int g_adv_menu_selected_value;
// GLOBAL: SHANDALAR 0x00603a38
int g_city_info_strings_loaded;
// GLOBAL: SHANDALAR 0x00603a48
char **g_city_info_heading_strings;

// GLOBAL: SHANDALAR 0x00746e60
EncodedImage *g_city_info_done_button_sprites[3];
// GLOBAL: SHANDALAR 0x00746e70
EncodedImage *g_city_info_scroll_button_sprites[0x10];


// GLOBAL: SHANDALAR 0x00581918
int g_wizard_text_colors[0x10] = {
    0x000000d2,
    0x00000000,
    0x000000f4,
    0x000000ce,
    0x000000bc,
    0x00000003,
    0x00000000,
    0x0000001b,
    0x00000002,
    0x0000000d,
    0x00000003,
    0x00000018,
    0x00000004,
    0x00000001,
    0x00000005,
    0x0000001d,
};

// Forward decls
int __cdecl RenderCityInfoScrollButton(AdvMenuControl *control, int mode);
int __cdecl ActivateCityInfoScrollButton(AdvMenuControl *control);
int __cdecl RenderCityInfoDoneButton(AdvMenuControl *control, int mode);
int __cdecl ActivateCityInfoDoneButton(AdvMenuControl *control);
extern char g_city_info_done_navigate_hotkeys[];
extern char g_city_info_done_activate_hotkeys[];

// GLOBAL: SHANDALAR 0x0058c8f8
AdvMenuControl g_city_info_menu_controls[5] = {
    // PageUp
    {27, 171, 17, 58, 27, 171, 17, 58, 1, (AdvMenuRenderCallback)RenderCityInfoScrollButton, (AdvMenuActivateCallback)ActivateCityInfoScrollButton, -2, 0, (char *)0, (char *)0, 0x4900, 0, {0, 0, 0, 0}},
    // Up
    {27, 230, 17, 58, 27, 230, 17, 58, 1, (AdvMenuRenderCallback)RenderCityInfoScrollButton, (AdvMenuActivateCallback)ActivateCityInfoScrollButton, -1, 1, (char *)0, (char *)0, 0x4800, 0, {0, 0, 0, 0}},
    // Down
    {27, 289, 17, 58, 27, 289, 17, 58, 1, (AdvMenuRenderCallback)RenderCityInfoScrollButton, (AdvMenuActivateCallback)ActivateCityInfoScrollButton, 1, 2, (char *)0, (char *)0, 0x5000, 0, {0, 0, 0, 0}},
    // PageDown
    {27, 348, 17, 58, 27, 348, 17, 58, 1, (AdvMenuRenderCallback)RenderCityInfoScrollButton, (AdvMenuActivateCallback)ActivateCityInfoScrollButton, 2, 3, (char *)0, (char *)0, 0x5100, 0, {0, 0, 0, 0}},
    // Done
    {544, 28, 58, 26, 544, 28, 58, 26, 1, (AdvMenuRenderCallback)RenderCityInfoDoneButton, (AdvMenuActivateCallback)ActivateCityInfoDoneButton, 0, 0, g_city_info_done_navigate_hotkeys, g_city_info_done_activate_hotkeys, 0, 0, {0, 0, 0, 0}},
};

// GLOBAL: SHANDALAR 0x0058cad4
char g_city_info_done_navigate_hotkeys[8] = " \rDd\x1b";

// GLOBAL: SHANDALAR 0x0058cadc
char g_city_info_done_activate_hotkeys[8] = " \rDd\x1b";

// FUNCTION: SHANDALAR 0x004ffd0a
int RenderAdvMenuControlNormally(AdvMenuControl *control)
{
  int old_state;

  old_state = control->state;
  control->state = 0;
  g_menu_render_guard = 1;
  control->on_render(control, 0);
  g_menu_render_guard = 0;
  return old_state;
}

// FUNCTION: SHANDALAR 0x00500129
int __cdecl SetCurrentMenuContextArrowNavigation(int allow_arrow_nav)
{
  g_menu_allow_arrow_nav_by_context[g_menu_context_index] = allow_arrow_nav;
  return 1;
}

// FUNCTION: SHANDALAR 0x005001e3
int RenderCurrentMenuContextControls(void)
{
  int i;

  g_menu_render_guard = 1;
  for (i = 0; i < g_menu_control_count_by_context[g_menu_context_index]; i++)
  {
    g_menu_controls_by_context[g_menu_context_index][i]->on_render(g_menu_controls_by_context[g_menu_context_index][i],
                                                                   (g_menu_prev_control_index == i));
  }
  g_menu_render_guard = 0;
  return 1;
}

// FUNCTION: SHANDALAR 0x0050a709
int __cdecl RenderCityInfoScrollButton(AdvMenuControl *control, int mode)
{
  int in_bounds;

  if (g_menu_render_guard == 0)
  {
    if ((g_mouse_x < control->x) || ((control->x + control->width) < g_mouse_x))
    {
      in_bounds = 0;
    }
    else if ((g_mouse_y < control->y) || ((control->y + control->height) < g_mouse_y))
    {
      in_bounds = 0;
    }
    else
    {
      in_bounds = 1;
    }

    if (in_bounds == 0)
    {
      return 0;
    }
  }

  if (control->state == 3)
  {
    return 0;
  }

  DrawEncodedImageResampled(g_page0_window_bounds, control->x, control->y, control->width, control->height,
                            g_city_info_scroll_button_sprites[control->data_value * 4 + mode]);

  if ((mode == 2) && (control->on_activate != (AdvMenuActivateCallback)0))
  {
    control->on_activate(control);
  }

  return 1;
}

// FUNCTION: SHANDALAR 0x0050a828
int __cdecl ActivateCityInfoScrollButton(AdvMenuControl *control)
{
  PlaySoundEffectOnChannel("x:sound\\button2.wav", 0xf, 100, 100, 0);
  g_adv_menu_selected_value = control->selection_value;
  return 0;
}

// FUNCTION: SHANDALAR 0x0050a85a
int __cdecl RenderCityInfoDoneButton(AdvMenuControl *control, int mode)
{
  int in_bounds;

  if (g_menu_render_guard == 0)
  {
    if ((g_mouse_x < control->x) || ((control->x + control->width) < g_mouse_x))
    {
      in_bounds = 0;
    }
    else if ((g_mouse_y < control->y) || ((control->y + control->height) < g_mouse_y))
    {
      in_bounds = 0;
    }
    else
    {
      in_bounds = 1;
    }

    if (in_bounds == 0)
    {
      return 0;
    }
  }

  if (control->state == 3)
  {
    return 0;
  }

  DrawEncodedImageResampled(g_page0_window_bounds, control->x, control->y, control->width, control->height, g_city_info_done_button_sprites[mode]);

  if ((mode == 2) && (control->on_activate != (AdvMenuActivateCallback)0))
  {
    control->on_activate(control);
  }

  return 1;
}

// FUNCTION: SHANDALAR 0x0050a970
int __cdecl ActivateCityInfoDoneButton(AdvMenuControl *control)
{
  PlaySoundEffectOnChannel("x:sound\\button2.wav", 0xf, 100, 100, 0);
  g_adv_menu_selected_value = control->selection_value;
  return 0;
}

static void CityInfo_DrawVisibleRows(int *town_indices_1based, int town_count, int first_index, int list_top_y, int row_height_scaled)
{
  int i;
  int y0;

  y0 = list_top_y + ScaleUiCoordinate(0x15);
  for (i = 0; i < 9; i++)
  {
    int list_index = first_index + i;
    if (list_index < town_count)
    {
      DrawCityInfoTownRow(g_page1_window_bounds, town_indices_1based[list_index + 1], 0x30, y0 + i * row_height_scaled);
    }
  }
}

// FUNCTION: SHANDALAR 0x0050a9a2
void ShowCityInfoScreen(int unused)
{
  int list_buffer_y;       // ebp - 0x444
  int x_table[3];          // ebp - 0x440
  int list_panel_x;        // ebp - 0x434
  int row_height_scaled;   // ebp - 0x430
  int menu_context;        // ebp - 0x42c
  int list_width_scaled;   // ebp - 0x428
  int scroll_top_index;    // ebp - 0x424
  int j;                   // ebp - 0x420
  int scroll_input;        // ebp - 0x41c
  int row_y;               // ebp - 0x418
  int i;                   // ebp - 0x414
  int visible_town_index;  // ebp - 0x410
  int town_indices[0x100]; // ebp - 0x40c
  int town_count;          // ebp - 0xc
  int town_scan_index;     // ebp - 0x8
  int list_top_y;          // ebp - 0x4

  (void)unused;

  town_count = 0;
  scroll_top_index = 0;

  if (g_city_info_strings_loaded == 0)
  {
    g_city_info_heading_strings = LoadIniEscapedStringTable(g_advbuttons_ini_file, "cityInfo", g_ini_string_scratch);
    g_city_info_strings_loaded = 1;
  }

  LoadPcxIntoPage(1, "infobar.pic");

  BeginSpriteEncodeSession();

  // Scale/encode the 4 scroll buttons
  for (j = 0; j < 4; j++)
  {
    if (g_city_info_menu_controls[j].x == g_city_info_menu_controls[j].base_x)
    {
      g_city_info_menu_controls[j].x = ScaleUiCoordinate(g_city_info_menu_controls[j].x);
      g_city_info_menu_controls[j].y = ScaleUiCoordinate(g_city_info_menu_controls[j].y);
      g_city_info_menu_controls[j].width = ScaleUiCoordinate(g_city_info_menu_controls[j].width);
      g_city_info_menu_controls[j].height = ScaleUiCoordinate(g_city_info_menu_controls[j].height);
    }

    for (i = 0; i < 4; i++)
    {
      g_city_info_scroll_button_sprites[j * 4 + i] =
          EncodeSpriteFromPage(1, i * 0x12 + 0x2b, j * 0x31 + 0x1c, 0x11, 0x2f);
    }
  }

  // Scale the Done button
  if (g_city_info_menu_controls[4].x == g_city_info_menu_controls[4].base_x)
  {
    g_city_info_menu_controls[4].x = ScaleUiCoordinate(g_city_info_menu_controls[4].x);
    g_city_info_menu_controls[4].y = ScaleUiCoordinate(g_city_info_menu_controls[4].y);
    g_city_info_menu_controls[4].width = ScaleUiCoordinate(g_city_info_menu_controls[4].width);
    g_city_info_menu_controls[4].height = ScaleUiCoordinate(g_city_info_menu_controls[4].height);
  }

  // Build 3-state Done button sprites into g_city_info_done_button_sprites
  g_page1_window_bounds->font_slot = 7;
  SetFontStyleSize(7, (unsigned int)ScaleUiCoordinate(10));
  for (i = 0; i < 3; i++)
  {
    x_table[0] = 0x48;
    x_table[1] = 0x3f;
    x_table[2] = 0x40;

    SetFontStyleSize(7, (unsigned int)(10 - i / 2));
    DrawFormattedTextNoShadowCentered(g_page1_window_bounds, x_table[i], i * 0x3c + 0x49, 0xe, g_done_text_table_entry);
  }

  for (i = 0; i < 3; i++)
  {
    g_city_info_done_button_sprites[i] = EncodeSpriteFromPage(1, i * 0x3c + 0x2b, 1, 0x3a, 0x18);
  }

  FinalizeSpriteEncodeSession();

  menu_context = BeginMenuContext();
  ResetMenuContext(menu_context);
  AddMenuControlsToContext(g_city_info_menu_controls, 5, menu_context);
  SetCurrentMenuContextArrowNavigation(0);

  // Prepare background
  g_world_scroll_cache_ready = 1;
  LoadPcxResource(1, 0, global_screen_height - 0x1e0, "cityinfo.pic", (void *)0);
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, global_screen_height - 0x1e0, 0x280, 0x1e0, g_page1_window_bounds, 0, 0, global_screen_width,
                          global_screen_height);

  // Headings, loaded from advButtons [cityInfo] (format strings in .rdata are \"%s\")
  g_page1_window_bounds->font_slot = 7;
  SetFontStyleSize(7, (unsigned int)ScaleUiCoordinate(10));
  DrawTextAt(g_page1_window_bounds, 200, 0x69, 0x28, "%s", g_city_info_heading_strings[0]);
  DrawTextAt(g_page1_window_bounds, 200, 0x54, 0x45, "%s", g_city_info_heading_strings[1]);
  DrawTextAt(g_page1_window_bounds, 200, 0xb3, 0x45, "%s", g_city_info_heading_strings[2]);
  DrawTextAt(g_page1_window_bounds, 200, 0x130, 0x45, "%s", g_city_info_heading_strings[3]);
  DrawTextAt(g_page1_window_bounds, 200, 0x1cc, 0x45, "%s", g_city_info_heading_strings[4]);
  DrawTextAt(g_page1_window_bounds, 200, 0x23f, 0x45, "%s", g_city_info_heading_strings[5]);

  EnsureAdvfac64Loaded(1);

  // Reset font slots
  g_page0_window_bounds->font_slot = 1;
  g_page1_window_bounds->font_slot = 1;
  g_page2_window_bounds->font_slot = 1;

  LoadPcxResource(2, 0, g_page2_window_bounds->max_y - 0x46, "cinfopce.pic", (void *)0);

  // List panel geometry
  list_panel_x = (global_screen_width * 0x30 + global_screen_width / 2) / 0x280;
  list_top_y = ScaleUiCoordinate(0x52);
  list_width_scaled = ScaleUiCoordinate(0x231);
  row_height_scaled = ScaleUiCoordinate(0x2a);

  // Build repeating row background on the offscreen page
  StretchBlitGraphicsRect(g_page2_window_bounds, 0, g_page2_window_bounds->max_y - 0x2c, 0x231, 0x2a, g_page2_window_bounds, 0, 0x80, list_width_scaled,
                          row_height_scaled);
  for (i = 0; i < 9; i++)
  {
    CopyGraphicsRect(g_page2_window_bounds, 0, 0x80, list_width_scaled, row_height_scaled, g_page1_window_bounds, list_panel_x,
                     i * row_height_scaled + list_top_y);
  }

  CopyGraphicsRect(g_page1_window_bounds, 0, 0, global_screen_width, global_screen_height, g_page0_window_bounds, 0, 0);

  // Build town list (1-based indexing like the original)
  row_y = 2;
  for (town_scan_index = 0; town_scan_index < 0x80; town_scan_index++)
  {
    if ((((g_town_slots[town_scan_index].status_and_ruling_wizard & 2) == 0) && (g_reveal_all_world_info == 0)) ||
        (g_town_slots[town_scan_index].location_type == 1))
    {
      continue;
    }

    town_indices[town_count++] = town_scan_index;
  }

  for (i = 0; i < 5; i++)
  {
    DrawTextAt(g_page0_window_bounds, 0xfe, i * 0x2a + 0xcc, 0x2a, "%d", g_amulet_inventory[i]);
  }

main_loop:
  // Rebuild row backgrounds
  for (i = 0; i < 9; i++)
  {
    CopyGraphicsRect(g_page2_window_bounds, 0, 0x80, list_width_scaled, row_height_scaled, g_page1_window_bounds, list_panel_x,
                     i * row_height_scaled + list_top_y);
  }

  visible_town_index = scroll_top_index;
  for (j = 0; ((town_count - scroll_top_index < 9) ? town_count - scroll_top_index : 9) > j; j++, visible_town_index++)
  {
    town_scan_index = town_indices[visible_town_index];
    row_y = ScaleUiCoordinate(0x68) + j * row_height_scaled;
    DrawCityInfoTownRow(g_page1_window_bounds, town_scan_index, 0x30, row_y);
  }

  // Copy list area to the visible page
  CopyGraphicsRect(g_page1_window_bounds, list_panel_x, list_top_y, list_width_scaled, row_height_scaled * 9, g_page0_window_bounds, list_panel_x,
                   list_top_y);

  if (town_count > 9)
  {
    if (scroll_top_index == 0)
    {
      list_buffer_y = 0;
    }
    else
    {
      list_buffer_y = row_height_scaled;
    }

    CopyGraphicsRect(g_page1_window_bounds, list_panel_x, list_top_y, list_width_scaled, row_height_scaled * 9, g_page1_window_bounds, list_panel_x,
                     list_buffer_y);

    if (scroll_top_index == 0)
    {
      CopyGraphicsRect(g_page2_window_bounds, 0, 0x80, list_width_scaled, row_height_scaled, g_page1_window_bounds, list_panel_x,
                       row_height_scaled * 9);
      DrawCityInfoTownRow(g_page1_window_bounds, town_indices[scroll_top_index + 9], 0x30, ScaleUiCoordinate(0x15) + row_height_scaled * 9);

      CopyGraphicsRect(g_page2_window_bounds, 0, 0x80, list_width_scaled, row_height_scaled, g_page1_window_bounds, list_panel_x,
                       row_height_scaled * 10);

      if (scroll_top_index + 10 < town_count)
      {
        DrawCityInfoTownRow(g_page1_window_bounds, town_indices[scroll_top_index + 10], 0x30, ScaleUiCoordinate(0x15) + row_height_scaled * 10);
      }
    }
    else
    {
      CopyGraphicsRect(g_page2_window_bounds, 0, 0x80, list_width_scaled, row_height_scaled, g_page1_window_bounds, list_panel_x, 0);
      DrawCityInfoTownRow(g_page1_window_bounds, town_indices[scroll_top_index - 1], 0x30, ScaleUiCoordinate(0x15));

      if (scroll_top_index + 10 < town_count)
      {
        CopyGraphicsRect(g_page2_window_bounds, 0, 0x80, list_width_scaled, row_height_scaled, g_page1_window_bounds, list_panel_x,
                         row_height_scaled * 10);
      }

      if (scroll_top_index + 10 < town_count)
      {
        DrawCityInfoTownRow(g_page1_window_bounds, town_indices[scroll_top_index + 9], 0x30, ScaleUiCoordinate(0x15) + row_height_scaled * 10);
      }
    }
  }
  if (scroll_top_index == 0)
  {
    row_y = 0;
  }
  else
  {
    row_y = row_height_scaled;
  }

page_loop:
  // Enable/disable scroll buttons
  if (town_count > 9)
  {
    if (scroll_top_index == 0)
    {
      RenderAdvMenuControlDisabled(&g_city_info_menu_controls[0]);
      RenderAdvMenuControlDisabled(&g_city_info_menu_controls[1]);
    }
    else
    {
      RenderAdvMenuControlNormally(&g_city_info_menu_controls[0]);
      RenderAdvMenuControlNormally(&g_city_info_menu_controls[1]);
    }

    if (town_count - 9 == scroll_top_index)
    {
      RenderAdvMenuControlDisabled(&g_city_info_menu_controls[2]);
      RenderAdvMenuControlDisabled(&g_city_info_menu_controls[3]);
    }
    else
    {
      RenderAdvMenuControlNormally(&g_city_info_menu_controls[2]);
      RenderAdvMenuControlNormally(&g_city_info_menu_controls[3]);
    }
  }
  else
  {
    RenderAdvMenuControlDisabled(&g_city_info_menu_controls[0]);
    RenderAdvMenuControlDisabled(&g_city_info_menu_controls[1]);
    RenderAdvMenuControlDisabled(&g_city_info_menu_controls[2]);
    RenderAdvMenuControlDisabled(&g_city_info_menu_controls[3]);
  }

  RenderCurrentMenuContextControls();

  g_adv_menu_selected_value = -5;
  do
  {
    UpdateMouseSnapshot();
    UpdateMenuControlSelection(g_mouse_x_snapshot, g_mouse_y_snapshot, g_mouse_button_down_mask);
  } while ((g_adv_menu_selected_value == -5) && (HasQueuedKeyInput() == 0));

  if (g_adv_menu_selected_value == 0)
  {
    EndMenuContext();
    FreeSpriteBlob(g_city_info_scroll_button_sprites[0]);
    return;
  }

  switch (g_adv_menu_selected_value)
  {
  case -2: // PageUp
    scroll_input = 0x4900;
    break;
  case -1: // Up
    scroll_input = 0x4800;
    break;
  case 1: // Down
    scroll_input = 0x5000;
    break;
  case 2: // PageDown
    scroll_input = 0x5100;
    break;
  default:
    goto page_loop;
  }

  if (town_count > 9)
  {
    if (scroll_input == 0x4800)
    {
      scroll_top_index--;
      if (scroll_top_index < 0)
      {
        scroll_top_index = 0;
      }
      else
      {

        for (i = 0; ScaleUiCoordinate(0x2b) > i; i += 3)
        {
          CopyGraphicsRect(g_page1_window_bounds, list_panel_x, row_y - i + ScaleUiCoordinate(3), list_width_scaled,
                           row_height_scaled * 9 - ScaleUiCoordinate(5), g_page0_window_bounds, list_panel_x, list_top_y + ScaleUiCoordinate(3));
        }

        CopyGraphicsRect(g_page1_window_bounds, list_panel_x, 0, list_width_scaled, row_height_scaled * 9 - ScaleUiCoordinate(5), g_page0_window_bounds,
                         list_panel_x, list_top_y);

        if (scroll_top_index >= 1)
        {
          CopyGraphicsRect(g_page1_window_bounds, list_panel_x, 0, list_width_scaled, row_height_scaled * 10, g_page1_window_bounds, list_panel_x,
                           row_height_scaled);
          row_y = row_height_scaled;
          CopyGraphicsRect(g_page2_window_bounds, 0, 0x80, list_width_scaled, row_height_scaled, g_page1_window_bounds, list_panel_x, 0);
          DrawCityInfoTownRow(g_page1_window_bounds, town_indices[scroll_top_index - 1], 0x30, ScaleUiCoordinate(0x15));
        }
        else
        {
          row_y = 0;
        }
      }
      goto page_loop;
    }
    else if (scroll_input == 0x5000)
    {
      if (scroll_top_index != town_count - 9)
      {
        for (i = 0; ScaleUiCoordinate(0x2b) > i; i += 3)
        {
          CopyGraphicsRect(g_page1_window_bounds, list_panel_x, row_y + i + ScaleUiCoordinate(3), list_width_scaled,
                           row_height_scaled * 9 - ScaleUiCoordinate(5), g_page0_window_bounds, list_panel_x, list_top_y + ScaleUiCoordinate(3));
        }

        CopyGraphicsRect(g_page1_window_bounds, list_panel_x, row_y + row_height_scaled, list_width_scaled, row_height_scaled * 9 - ScaleUiCoordinate(5),
                         g_page0_window_bounds, list_panel_x, list_top_y);

        row_y = row_height_scaled;
        if ((scroll_top_index >= 1) && (scroll_top_index < town_count - 9))
        {
          CopyGraphicsRect(g_page1_window_bounds, list_panel_x, row_height_scaled, list_width_scaled, row_height_scaled * 10, g_page1_window_bounds,
                           list_panel_x, 0);
          if (scroll_top_index + 10 < town_count)
          {
            CopyGraphicsRect(g_page2_window_bounds, 0, 0x80, list_width_scaled, row_height_scaled, g_page1_window_bounds, list_panel_x,
                             row_height_scaled * 10);
          }

          if (scroll_top_index + 10 < town_count)
          {
            DrawCityInfoTownRow(g_page1_window_bounds, town_indices[scroll_top_index + 10], 0x30,
                                ScaleUiCoordinate(0x15) + row_height_scaled * 10);
          }
        }
      }

      scroll_top_index++;

      if (town_count - 9 < scroll_top_index)
      {
        scroll_top_index = town_count - 9;
      }

      goto page_loop;
    }
    else if (scroll_input == 0x4900)
    {
      scroll_top_index -= 8;
      scroll_top_index = MAX(scroll_top_index, 0);
      goto main_loop;
    }
    else if (scroll_input == 0x5100)
    {
      scroll_top_index += 8;
      scroll_top_index = MIN(scroll_top_index, town_count - 9);
      goto main_loop;
    }
    else
    {
      goto page_loop;
    }
  }

  EndMenuContext();
  FreeSpriteBlob(g_city_info_scroll_button_sprites[0]);
}

// FUNCTION: SHANDALAR 0x0050bb6d
int __cdecl DrawCityInfoTownRow(FacemakerWindowBounds *dst, int town_index, int x, int y)
{
  int avatar_indices[5];
  char *space_fwd;
  char *space_back;
  unsigned int tile_class;
  int world_magic_slot_result;
  int wizard_sprite_x;
  int i;
  char *mid;
  int sprite_height;
  int sprite_width;
  int text_color;
  int wizard_count;

  (void)dst;

  if (g_town_slots[town_index].location_type != 4)
  {
    strcpy(g_ui_message_buffer, BuildTownDisplayName(town_index));
  }
  else
  {
    strcpy(g_ui_message_buffer, gs_cityname_castle_00765dc0);
  }

  mid = g_ui_message_buffer + (strlen(g_ui_message_buffer) >> 1);

  for (space_fwd = mid; (*space_fwd != '\0') && (*space_fwd != ' '); space_fwd++)
  {
  }
  if (*space_fwd != ' ')
  {
    space_fwd = (char *)0;
  }

  for (space_back = mid; (*space_back != ' ') && (space_back >= g_ui_message_buffer); space_back--)
  {
  }
  if (*space_back != ' ')
  {
    space_back = (char *)0;
  }

  if ((space_fwd != (char *)0) && ((space_back == (char *)0) || ((space_back - mid) <= (mid - space_fwd))))
  {
    *space_fwd = '\n';
  }
  else if ((space_back != (char *)0) && ((space_fwd == (char *)0) || ((space_back - mid) >= (mid - space_fwd))))
  {
    *space_back = '\n';
  }

  strcat(g_ui_message_buffer, "\n");

  tile_class = GetWorldTileMagicMask(GetWorldTileType(g_town_slots[town_index].world_x, g_town_slots[town_index].world_y));

  if ((g_town_slots[town_index].status_and_ruling_wizard & 1) != 0)
  {
    text_color = 0xff;
  }
  else
  {
    text_color = 0xe3;
  }

  if (*(char *)((char *)&g_town_slots[town_index].status_and_ruling_wizard + 1) != '\0')
  {
    text_color = *(int *)((char *)g_wizard_text_colors + (((int)(g_town_slots[town_index].status_and_ruling_wizard & 0xffffff00)) >> 6));
  }

  DrawFormattedTextShadowedCentered(g_page1_window_bounds, text_color, ScaleUiCoordinate(x + 0x2a), y, g_ui_message_buffer);
  if ((g_town_slots[town_index].status_and_ruling_wizard & 1) != 0)
  {
    DrawFormattedTextShadowedCentered(g_page1_window_bounds, text_color, ScaleUiCoordinate(x + 0x20c), y, "X");
  }

  tile_class = GetWorldTileMagicMask(GetWorldTileType(g_town_slots[town_index].world_x, g_town_slots[town_index].world_y));

  wizard_count = 0;
  for (i = 1; i < 6; i++)
  {
    if ((tile_class & (1U << (unsigned char)i)) != 0)
    {
      wizard_count++;
    }
  }

  sprite_width = (int)g_world_magic_avatar_sprites[0]->width;
  sprite_height = (int)g_world_magic_avatar_sprites[0]->height;
  wizard_sprite_x = ScaleUiCoordinate(((x + 0x80) - (sprite_width * wizard_count) / 2) - ((wizard_count - 1) * 5));

  for (i = 1; i < 6; i++)
  {
    avatar_indices[0] = 2;
    avatar_indices[1] = 1;
    avatar_indices[2] = 4;
    avatar_indices[3] = 3;
    avatar_indices[4] = 0;

    if ((tile_class & (1U << (unsigned char)i)) != 0)
    {
      DrawEncodedImageResampled(g_page1_window_bounds, wizard_sprite_x, y - ScaleUiCoordinate(sprite_height / 2), ScaleUiCoordinate(sprite_width),
                                ScaleUiCoordinate(sprite_height), g_world_magic_avatar_sprites[avatar_indices[i - 1]]);
      wizard_sprite_x += ScaleUiCoordinate(sprite_width + 5);
    }
  }

  strcpy(g_ui_message_buffer, GetTownCardDescription(town_index));
  DrawFormattedTextShadowedCentered(g_page1_window_bounds, text_color, ScaleUiCoordinate(x + 0xff), y, g_ui_message_buffer);

  strcpy(g_ui_message_buffer, "");
  for (i = 0; i < 0xc; i++)
  {
    if ((town_index != 0) && (Scards[i].worldmagic_city == town_index))
    {
      world_magic_slot_result = FindWorldMagicCardIndex(i);
      strcat(g_ui_message_buffer, gs_worldmagic_names_00780660[i]);
    }
  }
  DrawFormattedTextShadowedCentered(g_page1_window_bounds, text_color, ScaleUiCoordinate(x + 0x19c), y, g_ui_message_buffer);

  return 0;
}
