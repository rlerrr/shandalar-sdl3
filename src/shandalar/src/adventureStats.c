#ifdef MODERN_FIXES
#include "platform/winuser_compat.h"
#else
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "shandalar.h"
#include "magic/src/global_state.h"
#include "shandalar_global_strings.h"
#include "deckdll/src/card_db.h"
#include "facemaker/src/facemaker_types.h"
#include "cardartlib/src/palette.h"
#include "drawcardlib/src/pic.h"

typedef unsigned char byte;

typedef struct
{
  int progress_by_color[5];
  int duel_wins_by_color[5];
  unsigned char town_count_by_color[5];
  unsigned char highlighted_wizard_color;
  unsigned char highlighted_creature_type;
  unsigned char highlighted_lair_color;
} StatWinData;

typedef struct
{
  int wins;
  int losses;
} StatsCreatureJournalCount;

typedef union
{
  struct
  {
    EncodedImage *normal[12];
    EncodedImage *highlight[12];
    EncodedImage *pressed[12];
    EncodedImage *icon[12];
  } named;
  EncodedImage *by_group[4][12];
} WorldMagicChoiceButtonSpriteBank;

typedef struct
{
  EncodedImage *normal[4];
  EncodedImage *highlight[4];
  EncodedImage *pressed[4];
} StatsMenuButtonSpriteSet;

int RenderStatsTabButtonControl(AdvMenuControl *control, int mode);
int RenderStatsWorldMagicControl(AdvMenuControl *control, int mode);
static int ActivateStatsMenuControl(AdvMenuControl *control);
static int RenderStatsScrollbarHotspot(AdvMenuControl *control, int mode);
int RenderStatsCreatureScrollbar(AdvMenuControl *control);
static int DragStatsCreatureScrollbar(AdvMenuControl *control);
static int RenderStatsColorHotspot(AdvMenuControl *control, int mode);
static int RenderJournalExitButtonControl(AdvMenuControl *control, int mode);
static int ActivateJournalExitButtonControl(AdvMenuControl *control);
static int RenderJournalStepButtonControl(AdvMenuControl *control, int mode);
static int ActivateJournalStepButtonControl(AdvMenuControl *control);
static int RenderJournalScrollbar(AdvMenuControl *control);
static int SelectStatsJournalEntry(int journal_index);
static void BuildStatsJournalEntryMessage(int entry_type, unsigned int entry_arg, int tile_x, int tile_y);
static void AnimateStatsJournalMarkerToTile(unsigned int tile_x, unsigned int tile_y);
char *BuildTownDisplayName(int town_index);
char *BuildCreatureNameWithArticle(int creature_type);
char *__cdecl GetDungeonName(int dungeon_index);
DWORD FormatMessageFromStringStripCarriageReturns(char *dst, DWORD max_length, LPCVOID format, ...);
char *__cdecl GetLairName(int lair_type);
int FindNearestTownIndex(int world_x, int world_y);

extern int global_screen_width;
extern int global_screen_height;
extern int g_default_palette_fade_steps;
extern int g_statwin_exports_by_ordinal[3];
extern int g_world_magic_town_flags[5];
extern char g_ui_message_buffer[0x1000];
extern HWND g_main_window_hwnd;
extern HANDLE g_main_thread_handle;
extern HANDLE g_timer_thread_handle;
extern HPALETTE g_palette_handle;
extern DIBSurface *g_graphics_pages[10];
extern FacemakerWindowBounds *g_page0_window_bounds;
extern FacemakerWindowBounds *g_page1_window_bounds;
extern FacemakerWindowBounds *g_page2_window_bounds;
extern int g_analyzed_deck_special_rules;
extern card_data_t global_cards_data[];
extern char g_itoa_buffer[0x20];
extern int g_menu_render_guard;
extern int g_mouse_x;
extern int g_mouse_y;
extern int g_reveal_all_world_info;
extern WorldMagicChoiceButtonSpriteBank g_world_magic_choice_button_sprite_bank;
extern int g_graphics_bpp;
extern RpBitsPalettePacket g_palette_data_words;
extern DIBSurface *g_facemaker_page4_dib;
extern HBITMAP g_facemaker_page4_bitmap;
extern int g_mouse_x_snapshot;
extern int g_mouse_y_snapshot;
extern int g_mouse_button_down_mask;
extern int Gold;
extern FacemakerWindowBounds *g_menu_control_draw_target_page;
extern EncodedImage *g_adv_scrollbar_track_sprite;
extern EncodedImage *g_adv_scrollbar_thumb_sprite;
extern EncodedImage *g_map_button_sprites[5][3];
extern int g_deck_total_card_count;
extern int g_deck_active_card_count;

// GLOBAL: SHANDALAR 0x00590618
static char s_statText_00590618[] = "statText";
// GLOBAL: SHANDALAR 0x005905f8
static char s_stats_card_tab_navigate_hotkeys_005905f8[4] = {'D', 'd', '\x1b', ' '};
// GLOBAL: SHANDALAR 0x00590600
static char s_stats_card_tab_activate_hotkeys_00590600[4] = {'\x1b', ' ', '\0', '\0'};

// GLOBAL: SHANDALAR 0x005905c8
static int g_stats_portrait_frame_colors[5][2] = {
    {4, 7},
    {245, 243},
    {207, 205},
    {190, 188},
    {127, 2}};

// GLOBAL: SHANDALAR 0x00650264
int g_deck_analysis_card_count;
// GLOBAL: SHANDALAR 0x0073ea30
static StatWinData g_statwin_data;
// GLOBAL: SHANDALAR 0x00650630
static EncodedImage *g_stats_menu_color_sprites[5];
// GLOBAL: SHANDALAR 0x00650698
static StatsMenuButtonSpriteSet g_stats_menu_button_sprites;
// GLOBAL: SHANDALAR 0x006506d0
static EncodedImage *g_stats_menu_scrollbar_sprite;
// GLOBAL: SHANDALAR 0x006506d4
static EncodedImage *g_stats_menu_scroll_thumb_sprite;
// GLOBAL: SHANDALAR 0x00650650
static int g_stats_world_magic_unlocked[12];
// GLOBAL: SHANDALAR 0x00650458
static int g_stats_journal_world_magic_counts[6];
// GLOBAL: SHANDALAR 0x00650470
static StatsCreatureJournalCount g_stats_creature_journal_counts[0x38];
// GLOBAL: SHANDALAR 0x00650684
static int g_stats_journal_completed_quests;
// GLOBAL: SHANDALAR 0x00650644
static int g_stats_journal_failed_quests;
// GLOBAL: SHANDALAR 0x00650680
static int g_stats_journal_cities_saved;
// GLOBAL: SHANDALAR 0x00650690
static int g_stats_journal_lairs_explored;
// GLOBAL: SHANDALAR 0x006506cc
static int g_stats_journal_dungeons_explored;
// GLOBAL: SHANDALAR 0x00650688
static int g_stats_journal_spells_found;
// GLOBAL: SHANDALAR 0x0065068c
static int g_stats_journal_artifacts_found;
// GLOBAL: SHANDALAR 0x0058fe70
static int g_stats_creature_sheet_resolution = 1;
// GLOBAL: SHANDALAR 0x006506d8
static int g_stats_creature_page_index;
// GLOBAL: SHANDALAR 0x00650648
static int g_stats_menu_selection;
// GLOBAL: SHANDALAR 0x0065064c
static int *g_stats_text_table;
// GLOBAL: SHANDALAR 0x006506c8
static int g_stats_text_loaded;

// GLOBAL: SHANDALAR 0x0058fe78
static int g_stats_creature_sheet_cell_dims[3][2] = {
    {80, 64},
    {100, 80},
    {128, 102},
};

// GLOBAL: SHANDALAR 0x0058fe90
static AdvMenuControl g_stats_menu_controls[0x16] = {
    {0x193, 7, 0x54, 0x17, 0x193, 7, 0x54, 0x17, 1, (AdvMenuRenderCallback)RenderStatsTabButtonControl, (AdvMenuActivateCallback)ActivateStatsMenuControl, 1, 0, "Ww", (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x1ec, 7, 0x48, 0x17, 0x1ec, 7, 0x48, 0x17, 1, (AdvMenuRenderCallback)RenderStatsTabButtonControl, (AdvMenuActivateCallback)ActivateStatsMenuControl, 2, 0, "Jj", (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x23c, 7, 0x31, 0x17, 0x23c, 7, 0x31, 0x17, 1, (AdvMenuRenderCallback)RenderStatsTabButtonControl, (AdvMenuActivateCallback)ActivateStatsMenuControl, 3, 1, s_stats_card_tab_navigate_hotkeys_005905f8, s_stats_card_tab_activate_hotkeys_00590600, 0, 0, {0, 0, 0, 0}},
    {0xe3, 0x2b, 0xe, 0x96, 0xe3, 0x2b, 0xe, 0x96, 1, (AdvMenuRenderCallback)RenderStatsScrollbarHotspot, (AdvMenuActivateCallback)DragStatsCreatureScrollbar, 4, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0xe3, 0x2b, 0xe, 0x12, 0xe3, 0x2b, 0xe, 0x12, 1, (AdvMenuRenderCallback)RenderStatsScrollbarHotspot, (AdvMenuActivateCallback)DragStatsCreatureScrollbar, 5, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x14d, 0x154, 0x30, 0x30, 0x14d, 0x154, 0x30, 0x30, 1, (AdvMenuRenderCallback)RenderStatsColorHotspot, (AdvMenuActivateCallback)0, 10, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x186, 0x154, 0x30, 0x30, 0x186, 0x154, 0x30, 0x30, 1, (AdvMenuRenderCallback)RenderStatsColorHotspot, (AdvMenuActivateCallback)0, 11, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x1bf, 0x154, 0x30, 0x30, 0x1bf, 0x154, 0x30, 0x30, 1, (AdvMenuRenderCallback)RenderStatsColorHotspot, (AdvMenuActivateCallback)0, 12, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x1f8, 0x154, 0x30, 0x30, 0x1f8, 0x154, 0x30, 0x30, 1, (AdvMenuRenderCallback)RenderStatsColorHotspot, (AdvMenuActivateCallback)0, 13, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x231, 0x154, 0x30, 0x30, 0x231, 0x154, 0x30, 0x30, 1, (AdvMenuRenderCallback)RenderStatsColorHotspot, (AdvMenuActivateCallback)0, 14, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x14d, 0x154, 0x30, 0x30, 0x14d, 0x154, 0x30, 0x30, 1, (AdvMenuRenderCallback)RenderStatsWorldMagicControl, (AdvMenuActivateCallback)ActivateStatsMenuControl, 15, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x186, 0x154, 0x30, 0x30, 0x186, 0x154, 0x30, 0x30, 1, (AdvMenuRenderCallback)RenderStatsWorldMagicControl, (AdvMenuActivateCallback)ActivateStatsMenuControl, 16, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x1bf, 0x154, 0x30, 0x30, 0x1bf, 0x154, 0x30, 0x30, 1, (AdvMenuRenderCallback)RenderStatsWorldMagicControl, (AdvMenuActivateCallback)ActivateStatsMenuControl, 17, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x1f8, 0x154, 0x30, 0x30, 0x1f8, 0x154, 0x30, 0x30, 1, (AdvMenuRenderCallback)RenderStatsWorldMagicControl, (AdvMenuActivateCallback)ActivateStatsMenuControl, 18, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x231, 0x154, 0x30, 0x30, 0x231, 0x154, 0x30, 0x30, 1, (AdvMenuRenderCallback)RenderStatsWorldMagicControl, (AdvMenuActivateCallback)ActivateStatsMenuControl, 19, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x14d, 0x154, 0x30, 0x30, 0x14d, 0x154, 0x30, 0x30, 1, (AdvMenuRenderCallback)RenderStatsWorldMagicControl, (AdvMenuActivateCallback)ActivateStatsMenuControl, 20, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x186, 0x154, 0x30, 0x30, 0x186, 0x154, 0x30, 0x30, 1, (AdvMenuRenderCallback)RenderStatsWorldMagicControl, (AdvMenuActivateCallback)ActivateStatsMenuControl, 21, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x1bf, 0x154, 0x30, 0x30, 0x1bf, 0x154, 0x30, 0x30, 1, (AdvMenuRenderCallback)RenderStatsWorldMagicControl, (AdvMenuActivateCallback)ActivateStatsMenuControl, 22, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x1f8, 0x154, 0x30, 0x30, 0x1f8, 0x154, 0x30, 0x30, 1, (AdvMenuRenderCallback)RenderStatsWorldMagicControl, (AdvMenuActivateCallback)ActivateStatsMenuControl, 23, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x231, 0x154, 0x30, 0x30, 0x231, 0x154, 0x30, 0x30, 1, (AdvMenuRenderCallback)RenderStatsWorldMagicControl, (AdvMenuActivateCallback)ActivateStatsMenuControl, 24, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x14d, 0x154, 0x30, 0x30, 0x14d, 0x154, 0x30, 0x30, 1, (AdvMenuRenderCallback)RenderStatsWorldMagicControl, (AdvMenuActivateCallback)ActivateStatsMenuControl, 25, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x186, 0x154, 0x30, 0x30, 0x186, 0x154, 0x30, 0x30, 1, (AdvMenuRenderCallback)RenderStatsWorldMagicControl, (AdvMenuActivateCallback)ActivateStatsMenuControl, 26, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
};

// GLOBAL: SHANDALAR 0x00581b00
static AdvMenuControl g_stats_journal_menu_controls[3] = {
    {0x1f3, 5, 0x5e, 0x1c, 0x1f3, 5, 0x5e, 0x1c, 1, (AdvMenuRenderCallback)RenderJournalExitButtonControl, (AdvMenuActivateCallback)ActivateJournalExitButtonControl, 4, 0, " \rDd", " \rDd", 0, 0, {0, 0, 0, 0}},
    {1, 0x6e, 0x13, 0x117, 1, 0x6e, 0x13, 0x117, 1, (AdvMenuRenderCallback)RenderJournalStepButtonControl, (AdvMenuActivateCallback)ActivateJournalStepButtonControl, 1, 0, " \rDd", " \rDd", 0, 0, {0, 0, 0, 0}},
    {2, 0x6f, 0xf, 0x46, 2, 0x6f, 0xf, 0x46, 1, (AdvMenuRenderCallback)RenderJournalStepButtonControl, (AdvMenuActivateCallback)ActivateJournalStepButtonControl, 2, 0, " \rDd", " \rDd", 0, 0, {0, 0, 0, 0}},
};

// GLOBAL: SHANDALAR 0x005a82d0
static int g_stats_journal_marker_x;
// GLOBAL: SHANDALAR 0x005a82d4
static int g_stats_journal_marker_y;
// GLOBAL: SHANDALAR 0x005a82d8
static int g_stats_journal_menu_selection;
// GLOBAL: SHANDALAR 0x005a82dc
static int g_stats_journal_current_entry;
// GLOBAL: SHANDALAR 0x005a82e0
static int g_stats_journal_entry_count;
// GLOBAL: SHANDALAR 0x005a82e4
static int g_stats_journal_previous_marker_y;
// GLOBAL: SHANDALAR 0x005a82e8
static int g_stats_journal_previous_marker_x;

int AnimatePaletteToColor(int color_index, int steps);
int ClearGraphicsPageWithPaletteColor(int page_number, int color_index);
void LoadPcxIntoPage(int page_number, char *path);
void LoadPcxIntoPageNoPalette(char *path);
int *LoadIniEscapedStringTable(FILE *ini_file, char *section_name, char *scratch);
int MapWizardColorToDisplayIndex(int wizard_color);
PaletteLog *ReadPalette(char *palette_text_path, char *palette_binary_path);
void DrawGraphicsLine(FacemakerWindowBounds *window_bounds, int x1, int y1, int x2, int y2, int color_index);
unsigned int WaitForInputEventUnlessBlocked(void);
int single_color_test_bit_to_color_t(int mask);
void DrawTextLineNoShadow(char *text, int x, int y, int color_index);
void DrawCenteredTextLineClamped(char *text, int center_x, int y, int color_index);
void DrawScaledCenteredTextNoShadow(char *text, int center_x, int y, int color_index);
int ScaleUiCoordinateFrom320(int value);
void BlitGraphicsRect(FacemakerWindowBounds *dst, unsigned int dst_x, int dst_y, unsigned int width, DWORD height,
                      FacemakerWindowBounds *src, int src_x, int src_y);
void StretchBlitGraphicsRect(FacemakerWindowBounds *dst, int dst_x, int dst_y, int src_w, int src_h,
                             FacemakerWindowBounds *src, int src_x, int src_y, int copy_w, int copy_h);
void DrawEncodedImageResampled(FacemakerWindowBounds *dst, int x, int y, int width, int height, EncodedImage *encoded_image);
void FillGraphicsRect(FacemakerWindowBounds *window_bounds, int x, int y, int width, int height, unsigned int color_index);
void DrawFormattedTextNoShadowCentered(FacemakerWindowBounds *dst, int text_color, int x, int y, char *format, ...);
void DrawFormattedTextShadowedCentered(FacemakerWindowBounds *window, int color_index, int x, int y, char *format, ...);
void DrawWorldUiFormattedText(FacemakerWindowBounds *window, int color_index, int x, int y, char *format, ...);
void DrawUiScaledTextWithShadowCenterY(FacemakerWindowBounds *window, int color_index, int x, int y, char *format, ...);
unsigned int FindWorldMagicCardIndex(int world_magic_slot_index);
int ScaleUiCoordinate(int value);
void DrawTextAt(FacemakerWindowBounds *dst, int text_color, int x, int y, char *text, ...);
void DrawUiScaledCenteredText(char *text, int center_x, int y, int color_index);
void ClearInputAndWaitForMouseRelease(void);
void BeginSpriteEncodeSession(void);
EncodedImage *EncodeSpriteFromPage(int page_number, int x, int y, int width, int height);
void FinalizeSpriteEncodeSession(void);
int SetFontStyleSize(int font_id, unsigned int style);
extern FILE *g_advbuttons_ini_file;
extern char g_ini_string_scratch[0x28];
void LoadPcxResource(int page_number, int x, int y, char *path, void *opaque);
int ApplyPortraitPaletteMap(FacemakerWindowBounds *page, int src_x, int src_y, unsigned int width, int height, char *palette_source_path,
                            char *portrait_path);
int CountDuelPoolEligibleTowns(void);
void RpBits_ApplyPalette(RpBitsPalettePacket *palette_data);
int FadeInPaletteFromGray(int gray, int steps);
void LoadPcxIntoPageOpaque(int page_number, char *path);
int BeginMenuContext(void);
int ResetMenuContext(int context_index);
int AddMenuControlsToContext(AdvMenuControl *controls, int control_count, int context_index);
int EndMenuContext(void);
void UpdateMouseSnapshot(void);
int UpdateMenuControlSelection(int mouse_x, int mouse_y, int allow_activate_on_click);
void FreeSpriteBlob(void *memory);
void PlaySoundEffectOnChannel(char *sound_path, int channel, int volume, int pitch_percent, int pan_percent);
extern char *g_world_magic_sound_paths[5];
int ShowStatsWindow(int mode, int highlight);
void ShowWorldMapScreen(int mode);
int ConsumeUiTickCount(void);
int GetUiTickCount(void);
int IsKeyInputQueueEmpty(void);
int PopQueuedKeyInput(void);
void ConvertWorldTileToMapScreenCoords(int tile_x, int tile_y, int *out_x, int *out_y);
void DelayUiTicks(int ticks);
int SignNonZero(int value);
int ClampIntToRange(int value, int min_value, int max_value);
int MeasureMultilineTextWidth(FacemakerWindowBounds *dst, char *text);
int GetFontLineHeight(int font_slot);
int ApplyPortraitTintMap(FacemakerWindowBounds *dst, int x, int y, int w, int h, unsigned int tint, int maybe_shadow);

int RenderStatsCreatureGridPage(int page_index);

// FUNCTION: SHANDALAR 0x00500e30
void ScaleMenuControlsForScreen(AdvMenuControl *control, int control_count)
{
  int i;

  if ((global_screen_width != 0x280) && (control->x == control->base_x))
  {
    for (i = 0; i < control_count; i = i + 1, control = control + 1)
    {
      control->x = ScaleUiCoordinate(control->x);
      control->y = ScaleUiCoordinate(control->y);
      control->width = ScaleUiCoordinate(control->width);
      control->height = ScaleUiCoordinate(control->height);
    }
  }
}

// FUNCTION: SHANDALAR 0x004ff340
int RenderBasicImageMenuControl(AdvMenuControl *control, int mode)
{
  int mouse_inside;

  if (g_menu_render_guard == 0)
  {
    if ((g_mouse_x < control->x) || (control->width + control->x < g_mouse_x))
    {
      mouse_inside = 0;
    }
    else if ((g_mouse_y < control->y) || (control->y + control->height < g_mouse_y))
    {
      mouse_inside = 0;
    }
    else
    {
      mouse_inside = 1;
    }
    if (!mouse_inside)
    {
      return 0;
    }
  }
  if (control->state == 3)
  {
    return 0;
  }
  DrawEncodedImageResampled(g_menu_control_draw_target_page, control->x, control->y, control->width, control->height,
                            (EncodedImage *)control->mode_data[mode]);
  if ((mode == 2) && (control->on_activate != 0))
  {
    control->on_activate(control);
  }
  return 1;
}

// FUNCTION: SHANDALAR 0x00548eb1
void DrawStatsRectFrame(FacemakerWindowBounds *window, int x, int y, int width, int height, int color_index)
{
  int x2;
  int y2;

  x2 = width + x;
  y2 = height + y;
  DrawGraphicsLine(window, x, y, x2, y, color_index);
  DrawGraphicsLine(window, x2, y, x2, y2, color_index);
  DrawGraphicsLine(window, x2, y2, x, y2, color_index);
  DrawGraphicsLine(window, x, y2, x, y, color_index);
}

// FUNCTION: SHANDALAR 0x005487c0
int RenderStatsTabButtonControl(AdvMenuControl *control, int mode)
{
  int mouse_inside;
  int y;
  int x;
  int height;
  int width;
  EncodedImage *highlight_image;
  EncodedImage *encoded_image;

  if (g_menu_render_guard == 0)
  {
    if ((g_mouse_x < control->x) || (control->width + control->x < g_mouse_x))
    {
      mouse_inside = 0;
    }
    else if ((g_mouse_y < control->y) || (control->y + control->height < g_mouse_y))
    {
      mouse_inside = 0;
    }
    else
    {
      mouse_inside = 1;
    }
    if (!mouse_inside)
    {
      return 0;
    }
  }
  if (control->state == 3)
  {
    return 0;
  }
  encoded_image = *(EncodedImage **)((char *)&g_stats_menu_button_sprites + ((control->selection_value - 1) << 4) + mode * 4);
  highlight_image = *(EncodedImage **)((char *)&g_stats_menu_button_sprites + ((control->selection_value - 1) << 4) + 4);
  x = control->x;
  y = control->y;
  width = control->width;
  height = control->height;
  if (mode == 2)
  {
    DrawEncodedImageResampled(g_page2_window_bounds, 0, y + 0x80, width, height, highlight_image);
    DrawEncodedImageResampled(g_page2_window_bounds, 2, y + 0x82, width - 4, height - 4, encoded_image);
    BlitGraphicsRect(g_page2_window_bounds, 0, y + 0x80, width, height, g_page0_window_bounds, x, y);

    if (mode == 2)
    {
      if (control->on_activate != 0)
      {
        control->on_activate(control);
      }
    }
  }
  else
  {
    DrawEncodedImageResampled(g_page0_window_bounds, x, y, width, height, encoded_image);
  }
  return 1;
}

// FUNCTION: SHANDALAR 0x00548b25
static int ActivateStatsMenuControl(AdvMenuControl *control)
{
  unsigned int world_magic_index = control->selection_value - 0xf;
  if (((control->selection_value >= 0xf) && ((int)world_magic_index >= 2)) && ((world_magic_index & 1) == 0))
  {
    PlaySoundEffectOnChannel(g_world_magic_sound_paths[(control->selection_value - 0x11) / 2], 0xf, 100, 100, 0);
  }
  else
  {
    PlaySoundEffectOnChannel("x:sound\\button2.wav", 0xf, 100, 100, 0);
  }
  g_stats_menu_selection = control->selection_value;
  return g_stats_menu_selection;
}

// FUNCTION: SHANDALAR 0x0054899f
int RenderStatsWorldMagicControl(AdvMenuControl *control, int mode)
{
  int mouse_inside;
  int y;
  int x;
  int height;
  int width;
  EncodedImage *highlight_sprite;
  EncodedImage *sprite;

  if (g_menu_render_guard == 0)
  {
    if ((g_mouse_x < control->x) || (control->width + control->x < g_mouse_x))
    {
      mouse_inside = 0;
    }
    else if ((g_mouse_y < control->y) || (control->y + control->height < g_mouse_y))
    {
      mouse_inside = 0;
    }
    else
    {
      mouse_inside = 1;
    }
    if (mouse_inside == 0)
    {
      return 0;
    }
  }
  if (control->state == 3)
  {
    return 0;
  }

  sprite = (EncodedImage *)control->mode_data[mode];
  highlight_sprite = (EncodedImage *)control->mode_data[1];
  x = control->x;
  y = control->y;
  width = control->width;
  height = control->height;

  if (mode == 2)
  {
    DrawEncodedImageResampled(g_page0_window_bounds, control->x + 2, control->y + 2, control->width - 4, control->height - 4,
                              highlight_sprite);

    if (mode == 2)
    {
      if (control->on_activate != 0)
      {
        control->on_activate(control);
      }
    }
  }
  else
  {
    DrawEncodedImageResampled(g_page0_window_bounds, x, y, width, height, sprite);
  }
  return 1;
}

// FUNCTION: SHANDALAR 0x00548bb5
static int RenderStatsScrollbarHotspot(AdvMenuControl *control, int mode)
{
  int mouse_inside;
  int y;
  int x;
  int height;
  int width;

  if (g_menu_render_guard == 0)
  {
    if ((g_mouse_x < control->x) || (control->width + control->x < g_mouse_x))
    {
      mouse_inside = 0;
    }
    else if ((g_mouse_y < control->y) || (control->y + control->height < g_mouse_y))
    {
      mouse_inside = 0;
    }
    else
    {
      mouse_inside = 1;
    }
    if (!mouse_inside)
    {
      return 0;
    }
  }
  if (control->state == 3)
  {
    return 0;
  }
  x = control->x;
  y = control->y;
  width = control->width;
  height = control->height;
  if ((mode == 2) && (control->on_activate != 0))
  {
    control->on_activate(control);
  }
  return 1;
}

// FUNCTION: SHANDALAR 0x00548f51
static int RenderStatsColorHotspot(AdvMenuControl *control, int mode)
{
  int mouse_inside;

  if ((g_mouse_x < control->x) || (control->width + control->x < g_mouse_x))
  {
    mouse_inside = 0;
  }
  else if ((g_mouse_y < control->y) || (control->y + control->height < g_mouse_y))
  {
    mouse_inside = 0;
  }
  else
  {
    mouse_inside = 1;
  }
  if (mouse_inside == 0)
  {
    return 0;
  }

  if (mode == 2)
  {
    g_stats_menu_selection = control->selection_value;
  }
  return 1;
}

static __inline void InitializeStatsFixedMenuControls(void)
{
  int i;

  memset(g_stats_menu_controls, 0, sizeof(g_stats_menu_controls));
  g_stats_menu_controls[0].base_x = 0x193;
  g_stats_menu_controls[0].base_y = 7;
  g_stats_menu_controls[0].base_width = 0x54;
  g_stats_menu_controls[0].base_height = 0x17;
  g_stats_menu_controls[0].x = 0x193;
  g_stats_menu_controls[0].y = 7;
  g_stats_menu_controls[0].width = 0x54;
  g_stats_menu_controls[0].height = 0x17;
  g_stats_menu_controls[0].enabled = 1;
  g_stats_menu_controls[0].on_render = (AdvMenuRenderCallback)RenderStatsTabButtonControl;
  g_stats_menu_controls[0].on_activate = (AdvMenuActivateCallback)ActivateStatsMenuControl;
  g_stats_menu_controls[0].selection_value = 1;

  g_stats_menu_controls[1] = g_stats_menu_controls[0];
  g_stats_menu_controls[1].base_x = 0x1ec;
  g_stats_menu_controls[1].x = 0x1ec;
  g_stats_menu_controls[1].base_width = 0x48;
  g_stats_menu_controls[1].width = 0x48;
  g_stats_menu_controls[1].selection_value = 2;

  g_stats_menu_controls[2] = g_stats_menu_controls[0];
  g_stats_menu_controls[2].base_x = 0x23c;
  g_stats_menu_controls[2].x = 0x23c;
  g_stats_menu_controls[2].base_width = 0x31;
  g_stats_menu_controls[2].width = 0x31;
  g_stats_menu_controls[2].selection_value = 3;

  g_stats_menu_controls[3].base_x = 0xe3;
  g_stats_menu_controls[3].base_y = 0x2b;
  g_stats_menu_controls[3].base_width = 0xe;
  g_stats_menu_controls[3].base_height = 0x96;
  g_stats_menu_controls[3].x = 0xe3;
  g_stats_menu_controls[3].y = 0x2b;
  g_stats_menu_controls[3].width = 0xe;
  g_stats_menu_controls[3].height = 0x96;
  g_stats_menu_controls[3].enabled = 1;
  g_stats_menu_controls[3].on_render = (AdvMenuRenderCallback)RenderStatsScrollbarHotspot;
  g_stats_menu_controls[3].on_activate = (AdvMenuActivateCallback)DragStatsCreatureScrollbar;
  g_stats_menu_controls[3].selection_value = 4;
  g_stats_menu_controls[3].mode_data[3] = 0x12;

  g_stats_menu_controls[4] = g_stats_menu_controls[3];
  g_stats_menu_controls[4].base_height = 0x12;
  g_stats_menu_controls[4].height = 0x12;
  g_stats_menu_controls[4].on_render = (AdvMenuRenderCallback)RenderStatsCreatureScrollbar;
  g_stats_menu_controls[4].selection_value = 5;

  for (i = 0; i < 5; i = i + 1)
  {
    g_stats_menu_controls[i + 5].base_x = i * 0x39 + 0x14d;
    g_stats_menu_controls[i + 5].base_y = 0x154;
    g_stats_menu_controls[i + 5].base_width = 0x30;
    g_stats_menu_controls[i + 5].base_height = 0x30;
    g_stats_menu_controls[i + 5].x = i * 0x39 + 0x14d;
    g_stats_menu_controls[i + 5].y = 0x154;
    g_stats_menu_controls[i + 5].width = 0x30;
    g_stats_menu_controls[i + 5].height = 0x30;
    g_stats_menu_controls[i + 5].enabled = 1;
    g_stats_menu_controls[i + 5].on_render = (AdvMenuRenderCallback)RenderStatsColorHotspot;
    g_stats_menu_controls[i + 5].selection_value = i + 10;
    g_stats_menu_controls[i + 5].state = g_world_magic_town_flags[i] ? 0 : 3;
  }
}

static __inline void InitializeStatsWorldMagicControls(void)
{
  int i;
  int x;
  int y;

  for (i = 0; i < 0xc; i = i + 1)
  {
    g_stats_menu_controls[i + 10].state = 3;
    g_stats_world_magic_unlocked[i] = 0;
    if ((i >= 2) && ((i & 1) == 0))
    {
      y = 0xd2;
      x = ((i - 2) / 2) * 0x35 + 0x16b;
      g_stats_menu_controls[i + 10].x = x;
      g_stats_menu_controls[i + 10].base_x = g_stats_menu_controls[i + 10].x;
      g_stats_menu_controls[i + 10].y = y;
      g_stats_menu_controls[i + 10].base_y = g_stats_menu_controls[i + 10].y;
      if ((g_world_magic_bitmap & (1 << (byte)i)) != 0)
      {
        g_stats_menu_controls[i + 10].state = 0;
        g_stats_world_magic_unlocked[i] = 1;
      }
    }
    if (((i < 2) || ((i & 1) != 0)) || (g_stats_world_magic_unlocked[i] == 0))
    {
      y = 0x10e;
      if (i < 2)
      {
        x = i * 0x35;
      }
      else
      {
        x = ((i - 2) / 2) * 0x35 + 0x6a;
      }
      x = x + 0xee;
      g_stats_menu_controls[i + 10].x = x;
      g_stats_menu_controls[i + 10].base_x = g_stats_menu_controls[i + 10].x;
      g_stats_menu_controls[i + 10].y = y;
      g_stats_menu_controls[i + 10].base_y = g_stats_menu_controls[i + 10].y;
      if ((g_world_magic_bitmap & (1 << (byte)i)) != 0)
      {
        g_stats_menu_controls[i + 10].state = 0;
        g_stats_world_magic_unlocked[i] = 1;
      }
    }
    g_stats_menu_controls[i + 10].height = 0x35;
    g_stats_menu_controls[i + 10].base_height = g_stats_menu_controls[i + 10].height;
    g_stats_menu_controls[i + 10].width = g_stats_menu_controls[i + 10].base_height;
    g_stats_menu_controls[i + 10].base_width = g_stats_menu_controls[i + 10].width;
    g_stats_menu_controls[i + 10].mode_data[0] = (int)g_world_magic_choice_button_sprite_bank.named.normal[i];
    g_stats_menu_controls[i + 10].mode_data[2] = (int)g_world_magic_choice_button_sprite_bank.named.highlight[i];
    g_stats_menu_controls[i + 10].mode_data[1] = g_stats_menu_controls[i + 10].mode_data[2];
  }
}

// FUNCTION: SHANDALAR 0x00548dfb
int RenderStatsCreatureScrollbar(AdvMenuControl *control)
{
  int page_count;
  int y;
  EncodedImage *encoded_image;
  AdvMenuControl *thumb_control;

  page_count = 0x11;
  thumb_control = control + 1;
  encoded_image = g_stats_menu_scroll_thumb_sprite;
  y = control->y + thumb_control->height / 2;
  FillGraphicsRect(g_page0_window_bounds, control->x, control->y, control->width, control->height, 0);
  DrawEncodedImageResampled(g_page0_window_bounds, control->x, y - thumb_control->height / 2, control->width,
                            ((int)encoded_image->height * control->width) / control->base_width, encoded_image);
  return 0;
}

// FUNCTION: SHANDALAR 0x00548cba
static int DragStatsCreatureScrollbar(AdvMenuControl *control)
{
  int page_count;
  int y;
  EncodedImage *encoded_image;
  int page_index;
  AdvMenuControl *thumb_control;

  page_count = 0x11;
  thumb_control = control + 1;
  encoded_image = g_stats_menu_scroll_thumb_sprite;
  UpdateMouseSnapshot();
  y = g_mouse_y_snapshot;
  y = MAX(control->y + thumb_control->height / 2, y);
  y = MIN(control->y + control->height - thumb_control->height / 2, y);
  page_index = ((y - control->y - thumb_control->height / 2) * page_count) / (control->height - thumb_control->height);
  FillGraphicsRect(g_page0_window_bounds, control->x, control->y, control->width, control->height, 0);
  DrawEncodedImageResampled(g_page0_window_bounds, control->x, y - thumb_control->height / 2, control->width,
                            ((int)encoded_image->height * control->width) / control->base_width, encoded_image);
  RenderStatsCreatureGridPage(page_index);
  g_stats_menu_selection = control->selection_value;
  return g_stats_menu_selection;
}

// FUNCTION: SHANDALAR 0x0054a4aa
int RebuildStatsJournalCounts(void)
{
  unsigned int entry_arg;
  int entry_type;
  int journal_index;
  int skipped_entry_count;

  skipped_entry_count = 0;
  memset(g_stats_creature_journal_counts, 0, sizeof(g_stats_creature_journal_counts));
  g_stats_journal_failed_quests =
      g_stats_journal_completed_quests =
          g_stats_journal_dungeons_explored =
              g_stats_journal_lairs_explored =
                  g_stats_journal_cities_saved =
                      g_stats_journal_spells_found =
                          g_stats_journal_artifacts_found = 0;

  for (journal_index = 0; journal_index < 10000; journal_index = journal_index + 1)
  {
    entry_type = g_journal_entries[journal_index].type;
    entry_arg = g_journal_entries[journal_index].arg;
    if (g_journal_entries[journal_index].type == JOURNAL_ENTRY_NONE)
    {
      break;
    }
    switch (entry_type)
    {
    case 2:
      if ((entry_arg & 0x80) != 0)
      {
        g_stats_journal_failed_quests = g_stats_journal_failed_quests + 1;
        g_stats_creature_journal_counts[entry_arg & 0x7f].wins = g_stats_creature_journal_counts[entry_arg & 0x7f].wins + 1;
      }
      else
      {
        g_stats_journal_completed_quests = g_stats_journal_completed_quests + 1;
        g_stats_creature_journal_counts[entry_arg & 0x7f].losses = g_stats_creature_journal_counts[entry_arg & 0x7f].losses + 1;
      }
      break;
    case 4:
      g_stats_journal_world_magic_counts[entry_arg & 0x7f] = g_stats_journal_world_magic_counts[entry_arg & 0x7f] + 1;
      break;
    case 7:
      g_stats_journal_cities_saved = g_stats_journal_cities_saved + 1;
      break;
    case 0xd:
      g_stats_journal_lairs_explored = g_stats_journal_lairs_explored + 1;
      break;
    case 0xf:
      skipped_entry_count = skipped_entry_count + 1;
      break;
    case 0x10:
      g_stats_journal_spells_found = g_stats_journal_spells_found + 1;
      break;
    case 0x11:
      g_stats_journal_artifacts_found = g_stats_journal_artifacts_found + 1;
      break;
    case 0x13:
      g_stats_journal_dungeons_explored = g_stats_journal_dungeons_explored + 1;
      break;
    }
  }
  return 0;
}

// FUNCTION: SHANDALAR 0x0054a099
int RenderStatsCreatureGridPage(int page_index)
{
  int row_count;              // ebp - 0x34
  unsigned int creature_type; // ebp - 0x30
  int win_percent;            // ebp - 0x2c
  int text_colors[5];         // ebp - 0x28
  int losses;                 // ebp - 0x14
  int row;                    // ebp - 0x10
  int column;                 // ebp - 0xc
  int wins;                   // ebp - 0x8
  int color_index;            // ebp - 0x4

  if (page_index == 0x12)
  {
    row_count = 1;
  }
  else
  {
    row_count = 2;
  }
  text_colors[0] = 0xbf;
  text_colors[1] = 0xbc;
  text_colors[2] = 0xb7;
  text_colors[3] = 0xf4;
  text_colors[4] = 0xf6;
  creature_type = page_index * 3;
  BlitGraphicsRect(g_page2_window_bounds, 0, 0x154, ScaleUiCoordinate(0x173), ScaleUiCoordinate(0x7f), g_page2_window_bounds, 0, 0x80);
  for (row = 0; row < row_count; row++)
  {
    column = 0;
    for (; (column < 3 && (int)creature_type < 0x37); column++, creature_type++)
    {
      if ((g_stats_creature_journal_counts[creature_type + 1].wins != 0) ||
          ((g_stats_creature_journal_counts[creature_type + 1].losses != 0 || (g_reveal_all_world_info != 0))))
      {
        wins = g_stats_creature_journal_counts[creature_type + 1].wins;
        losses = g_stats_creature_journal_counts[creature_type + 1].losses;

        win_percent = wins * 100 / MAX(wins + losses, 1);

        if (wins < losses)
        {
          if ((losses - wins > 5) && (win_percent <= 25))
          {
            color_index = 0;
          }
          else if ((wins + losses < 5) || (win_percent > 40))
          {
            color_index = 2;
          }
          else
          {
            color_index = 1;
          }
        }
        else if ((wins - losses > 5) && (win_percent >= 75))
        {
          color_index = 4;
        }
        else if ((wins + losses < 5) || (win_percent <= 60))
        {
          color_index = 2;
        }
        else
        {
          color_index = 3;
        }
        BlitGraphicsRect(g_page1_window_bounds,
                         g_stats_creature_sheet_cell_dims[g_stats_creature_sheet_resolution][0] * ((int)creature_type % 8) + 1,
                         g_stats_creature_sheet_cell_dims[g_stats_creature_sheet_resolution][1] * ((int)creature_type / 8) +
                             ScaleUiCoordinate(3) + 1,
                         g_stats_creature_sheet_cell_dims[g_stats_creature_sheet_resolution][0] - 2,
                         (g_stats_creature_sheet_cell_dims[g_stats_creature_sheet_resolution][1] - 2) - ScaleUiCoordinate(3),
                         g_page2_window_bounds, ScaleUiCoordinate(0x7c) * column, ScaleUiCoordinate(row << 6) + 0x80);
        g_page2_window_bounds->font_slot = 4;
        DrawFormattedTextShadowedCentered(g_page2_window_bounds, text_colors[color_index],
                                          ScaleUiCoordinate(0x7c) * column + ScaleUiCoordinate(0x5e),
                                          ScaleUiCoordinate(row * 0x40 + -3) + ScaleUiCoordinate(0x20) + 0x80, "%d/%d",
                                          g_stats_creature_journal_counts[creature_type + 1].wins, g_stats_creature_journal_counts[creature_type + 1].losses);
        g_page2_window_bounds->font_slot = 1;
        DrawFormattedTextShadowedCentered(g_page2_window_bounds, 0xb7, ScaleUiCoordinate(0x7c) * column + ScaleUiCoordinate(0x3e),
                                          ScaleUiCoordinate(row << 6) + ScaleUiCoordinate(0x36) + 0x80, "%s",
                                          g_shandalar_monster_definitions[creature_type + 1].name);
      }
    }
  }
  BlitGraphicsRect(g_page2_window_bounds, 0, 0x80, ScaleUiCoordinate(0x7c) * 3 - 2, ScaleUiCoordinate(0x40) * 2 - 2, g_page0_window_bounds,
                   ScaleUiCoordinate(0xf6), ScaleUiCoordinate(0x43));
  g_stats_creature_page_index = page_index;
  return page_index;
}

// FUNCTION: SHANDALAR 0x0054a668
int ShowWorldMagicStatsDetail(int world_magic_slot_index)
{
  int icon_top;
  int icon_bottom;
  int detail_result;
  int y;
  int icon_right;
  int x;
  int icon_left;
  EncodedImage *encoded_image;

  icon_left = 0x14f;
  icon_right = 0x19e;
  icon_top = 0x4c;
  icon_bottom = 0x9a;
  LoadPcxIntoPage(1, "worlbak1.pic");
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x280, 0x1e0, g_page0_window_bounds, 0, 0, global_screen_width, global_screen_height);
  detail_result = FindWorldMagicCardIndex(world_magic_slot_index);
  y = 0x28;
  y += 0x10;
  encoded_image = g_world_magic_choice_button_sprite_bank.named.normal[world_magic_slot_index];
  y = ((icon_bottom - icon_top) - encoded_image->height) / 2 + icon_top;
  x = ((icon_right - icon_left) - encoded_image->width) / 2 + icon_left;
  DrawEncodedImageResampled(g_page0_window_bounds, ScaleUiCoordinate(x), ScaleUiCoordinate(y),
                            ScaleUiCoordinate((int)encoded_image->width), ScaleUiCoordinate((int)encoded_image->height),
                            g_world_magic_choice_button_sprite_bank.named.normal[world_magic_slot_index]);
  y += (int)encoded_image->height + 0x10;
  y /= 2;
  x += (int)encoded_image->width / 2;
  x /= 2;
  g_page0_window_bounds->font_slot = 4;
  DrawTextAt(g_page0_window_bounds, 0x7b, 0x176, 0x4b, gs_worldmagic_title_0077e1d0);
  y -= 5;
  strcpy(g_ui_message_buffer, gs_worldmagic_names_00780660[world_magic_slot_index]);
  DrawUiScaledCenteredText(g_ui_message_buffer, x, y, 0x40);
  y += 8;
  strcpy(g_ui_message_buffer, gs_worldmagic_explains_0074b8f0[world_magic_slot_index]);
  DrawWorldUiFormattedText(g_page0_window_bounds, 0x7b, ScaleUiCoordinateFrom320(x), ScaleUiCoordinateFrom320(y), g_ui_message_buffer);
  y += 0x10;
  ClearInputAndWaitForMouseRelease();
  WaitForInputEventUnlessBlocked();
  return 0;
}

// Helper factored from RunAdventureStatsMenu (SHANDALAR 0x00549002); not a standalone original function.
static __inline void RenderAdventureStatsOverview(FacemakerWindowBounds *portrait_work_page, int *colors)
{
  int world_magic_slot_index;

  g_page1_window_bounds->font_slot = 4;
  g_page2_window_bounds->font_slot = 4;
  g_graphics_pages[4] = g_facemaker_page4_dib;
  SelectObject(g_graphics_pages[4]->hTempDC, g_facemaker_page4_bitmap);
  if (g_graphics_bpp == 8)
  {
    LoadPcxResource(1, 0, global_screen_height - 0x1e0, "statbak.pic", (void *)1);
  }
  else
  {
    LoadPcxResource(1, 0, global_screen_height - 0x1e0, "statbak.pic", (void *)1);
  }
  DrawStatsRectFrame(g_page1_window_bounds, 0x26, global_screen_height - 0x1d1, 0x8c, 0xac,
                     g_stats_portrait_frame_colors[g_starting_color - 1][0]);
  DrawStatsRectFrame(g_page1_window_bounds, 0x27, global_screen_height - 0x1d0, 0x8a, 0xaa,
                     g_stats_portrait_frame_colors[g_starting_color - 1][1]);
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, global_screen_height - 0x1e0, 0x280, 0x1e0, g_page1_window_bounds, 0, 0,
                          global_screen_width, global_screen_height);
  StretchBlitGraphicsRect(portrait_work_page, 0, 0, 0x89, 0xa9, g_page1_window_bounds, ScaleUiCoordinate(0x28),
                          ScaleUiCoordinate(0x11), ScaleUiCoordinate(0x89), ScaleUiCoordinate(0xa9));
  BlitGraphicsRect(g_page1_window_bounds, ScaleUiCoordinate(0xf7), ScaleUiCoordinate(0x43), ScaleUiCoordinate(0x173),
                   ScaleUiCoordinate(0x7f), g_page2_window_bounds, 0, 0x154);
  ApplyPortraitPaletteMap(g_page1_window_bounds, ScaleUiCoordinate(0x28), ScaleUiCoordinate(0x11), ScaleUiCoordinate(0x89),
                          ScaleUiCoordinate(0xa9), "prdblk.pic", "advfac64.pic");
  SelectObject(g_graphics_pages[4]->hTempDC, g_graphics_pages[4]->hPreviousBitmap);
  g_graphics_pages[4] = (DIBSurface *)0;

  DrawTextAt(g_page1_window_bounds, colors[3], 0x4f, 0x102, "%d", Gold);
  DrawTextAt(g_page1_window_bounds, colors[3], 0xb7, 0x102, "%d", g_food);
  DrawTextAt(g_page1_window_bounds, colors[3], 0x4f, 0x138, "%d", CountDuelPoolEligibleTowns());
  DrawTextAt(g_page1_window_bounds, colors[3], 0xb7, 0x138, "%d/%d", g_deck_active_card_count,
             g_deck_total_card_count);
  DrawTextAt(g_page1_window_bounds, colors[3], 0x29, 0x161, "%d", g_amulet_inventory[0]);
  DrawTextAt(g_page1_window_bounds, colors[3], 0x4e, 0x161, "%d", g_amulet_inventory[1]);
  DrawTextAt(g_page1_window_bounds, colors[3], 0x73, 0x161, "%d", g_amulet_inventory[2]);
  DrawTextAt(g_page1_window_bounds, colors[3], 0x98, 0x161, "%d", g_amulet_inventory[3]);
  DrawTextAt(g_page1_window_bounds, colors[3], 0xbd, 0x161, "%d", g_amulet_inventory[4]);
  DrawTextAt(g_page1_window_bounds, colors[4], 0x69, 0xc6, "%s",
             gs_difficultylevels_0077d130[g_shandalar_difficulty]);
  DrawTextAt(g_page1_window_bounds, colors[4], 0x69, 0xd6, "%s", g_name_entry_buffer);
  DrawTextAt(g_page1_window_bounds, colors[4], 0x10e, 0x16c, gs_stats_0077cfa0[0]);

  for (world_magic_slot_index = 0; (int)world_magic_slot_index < 5; world_magic_slot_index = world_magic_slot_index + 1)
  {
    if (g_world_magic_town_flags[world_magic_slot_index] != 0)
    {
      DrawEncodedImageResampled(g_page1_window_bounds, ScaleUiCoordinate(world_magic_slot_index * 0x39 + 0x14d), ScaleUiCoordinate(0x154),
                                ScaleUiCoordinate(0x30), ScaleUiCoordinate(0x30), g_stats_menu_color_sprites[world_magic_slot_index]);
    }
  }

  DrawTextAt(g_page1_window_bounds, colors[4], 0x118, 0xeb, gs_stats_0077cfa0[1]);
  InitializeStatsWorldMagicControls();
  ScaleMenuControlsForScreen(&g_stats_menu_controls[10], 0xc);
  RebuildStatsJournalCounts();
  DrawUiScaledTextWithShadowCenterY(g_page1_window_bounds, colors[4], 0x14, 0x184, gs_stats_0077cfa0[2]);
  DrawTextAt(g_page1_window_bounds, colors[3], 0xb2, 0x184, "%d", g_stats_journal_lairs_explored);
  DrawUiScaledTextWithShadowCenterY(g_page1_window_bounds, colors[4], 0x14, 0x196, gs_stats_0077cfa0[3]);
  DrawTextAt(g_page1_window_bounds, colors[3], 0xb2, 0x196, "%d", g_stats_journal_cities_saved);
  DrawUiScaledTextWithShadowCenterY(g_page1_window_bounds, colors[4], 0x14, 0x1a8, gs_stats_0077cfa0[4]);
  DrawTextAt(g_page1_window_bounds, colors[3], 0xb2, 0x1a8, "%d/%d",
             g_stats_journal_spells_found, g_stats_journal_artifacts_found);
  DrawUiScaledTextWithShadowCenterY(g_page1_window_bounds, colors[4], 0x14, 0x1ba, gs_stats_0077cfa0[5]);
  DrawTextAt(g_page1_window_bounds, colors[3], 0xb2, 0x1ba, "%d/%d",
             g_stats_journal_failed_quests, g_stats_journal_completed_quests);
  DrawUiScaledTextWithShadowCenterY(g_page1_window_bounds, colors[4], 0xe9, 0x19a, gs_stats_0077cfa0[6]);
  for (world_magic_slot_index = 0; (int)world_magic_slot_index < 5; world_magic_slot_index = world_magic_slot_index + 1)
  {
    DrawUiScaledTextWithShadowCenterY(g_page1_window_bounds, colors[3], world_magic_slot_index * 0x39 + 0x165, 0x19a, "%d",
                                      g_castle_dungeon_slots[world_magic_slot_index].times_entered + 1);
  }
  DrawUiScaledTextWithShadowCenterY(g_page1_window_bounds, colors[4], 0xe9, 0x1c0, gs_stats_0077cfa0[7]);
  DrawTextAt(g_page1_window_bounds, colors[3], 0x1a1, 0x1c0, "%d", g_stats_journal_dungeons_explored);
  DrawTextAt(g_page1_window_bounds, colors[4], 0x1b0, 0x36, gs_stats_0077cfa0[8]);

  if (g_graphics_bpp == 8)
  {
    memset(g_palette_data_words.entry_data, 0, 0x300);
    RpBits_ApplyPalette(&g_palette_data_words);
  }
  if (g_graphics_bpp == 8)
  {
    LoadPcxResource(-1, 0, 0, "advfac64.pic", &g_palette_data_words);
  }
  else
  {
    LoadPcxIntoPageNoPalette("advfac64.pic");
  }
  BlitGraphicsRect(g_page1_window_bounds, 0, 0, global_screen_width, global_screen_height, g_page0_window_bounds, 0, 0);
  FadeInPaletteFromGray(0, g_default_palette_fade_steps);
}

// FUNCTION: SHANDALAR 0x00428b90
static int RenderJournalExitButtonControl(AdvMenuControl *control, int mode)
{
  int mouse_inside;    // ebp - 0x20
  int draw_state;      // ebp - 0x1c
  int draw_y;          // ebp - 0x18
  int draw_x;          // ebp - 0x14
  int draw_h;          // ebp - 0x10
  int draw_w;          // ebp - 0xc
  int old_page_number; // ebp - 0x8
  int selection_value; // ebp - 0x4

  if (g_menu_render_guard == 0)
  {
    if ((g_mouse_x < control->x) || (control->width + control->x < g_mouse_x))
    {
      mouse_inside = 0;
    }
    else if ((g_mouse_y < control->y) || (control->y + control->height < g_mouse_y))
    {
      mouse_inside = 0;
    }
    else
    {
      mouse_inside = 1;
    }
    if (!mouse_inside)
    {
      return 0;
    }
  }
  if (control->state == 3)
  {
    return 0;
  }
  old_page_number = g_page0_window_bounds->page_number;
  selection_value = g_stats_journal_menu_controls[0].selection_value;
  switch (mode)
  {
  case 0:
    draw_state = 0;
    break;
  case 1:
    draw_state = 1;
    break;
  case 2:
    draw_state = 2;
    break;
  }

  draw_x = g_stats_journal_menu_controls[0].x;
  draw_y = g_stats_journal_menu_controls[0].y;
  draw_w = g_stats_journal_menu_controls[0].width;
  draw_h = g_stats_journal_menu_controls[0].height;
  DrawEncodedImageResampled(g_page0_window_bounds, draw_x, draw_y, draw_w, draw_h,
                            g_map_button_sprites[selection_value][draw_state]);
  if ((mode == 2) && (control->on_activate != 0))
  {
    control->on_activate(control);
  }
  return 1;
}

// FUNCTION: SHANDALAR 0x00428d2c
static int ActivateJournalExitButtonControl(AdvMenuControl *control)
{
  PlaySoundEffectOnChannel("x:sound\\button2.wav", 0xf, 100, 100, 0);
  g_stats_journal_menu_selection = control->selection_value;
  return 0;
}

// FUNCTION: SHANDALAR 0x00428d5e
static int RenderJournalStepButtonControl(AdvMenuControl *control, int mode)
{
  int mouse_inside;
  int y;
  int x;
  int height;
  int width;

  if (g_menu_render_guard == 0)
  {
    if ((g_mouse_x < control->x) || (control->width + control->x < g_mouse_x))
    {
      mouse_inside = 0;
    }
    else if ((g_mouse_y < control->y) || (control->y + control->height < g_mouse_y))
    {
      mouse_inside = 0;
    }
    else
    {
      mouse_inside = 1;
    }
    if (!mouse_inside)
    {
      return 0;
    }
  }
  if (control->state == 3)
  {
    return 0;
  }
  x = control->x;
  y = control->y;
  width = control->width;
  height = control->height;
  if ((mode == 2) && (control->on_activate != 0))
  {
    control->on_activate(control);
  }
  return 1;
}

// FUNCTION: SHANDALAR 0x00428e63
static int ActivateJournalStepButtonControl(AdvMenuControl *control)
{
  int entry_count;
  int y;
  EncodedImage *encoded_image;
  int journal_index;
  AdvMenuControl *thumb_control;

  entry_count = control->data_value;
  thumb_control = control + 1;
  encoded_image = g_adv_scrollbar_thumb_sprite;
  UpdateMouseSnapshot();
  y = g_mouse_y_snapshot;
  y = MAX(control->y + thumb_control->height / 2, y);
  y = MIN(control->y + control->height - thumb_control->height / 2, y);
  journal_index = ((y - control->y - thumb_control->height / 2) * entry_count) / (control->height - thumb_control->height);
  DrawEncodedImageResampled(g_page1_window_bounds, control->x, control->y, control->width, control->height, g_adv_scrollbar_track_sprite);
  DrawEncodedImageResampled(g_page1_window_bounds, control->x, y - thumb_control->height / 2, control->width,
                            ((int)encoded_image->height * control->width) / control->base_width, encoded_image);
  BlitGraphicsRect(g_page1_window_bounds, control->x, control->y, control->width, control->height, g_page0_window_bounds, control->x,
                   control->y);
  SelectStatsJournalEntry(journal_index);
  g_stats_journal_menu_selection = control->selection_value;
  return g_stats_journal_menu_selection;
}

// FUNCTION: SHANDALAR 0x00428fe8
static int RenderJournalScrollbar(AdvMenuControl *control)
{
  int entry_count;
  int y;
  EncodedImage *encoded_image;
  AdvMenuControl *thumb_control;

  entry_count = control->data_value;
  thumb_control = control + 1;
  encoded_image = g_adv_scrollbar_thumb_sprite;
  y = control->y + thumb_control->height / 2;
  DrawEncodedImageResampled(g_page1_window_bounds, control->x, control->y, control->width, control->height, g_adv_scrollbar_track_sprite);
  DrawEncodedImageResampled(g_page1_window_bounds, control->x, y - thumb_control->height / 2,
                            control->width, ((int)encoded_image->height * control->width) / control->base_width, encoded_image);
  BlitGraphicsRect(g_page1_window_bounds, control->x, control->y, control->width, control->height, g_page0_window_bounds, control->x,
                   control->y);
  return 0;
}

// FUNCTION: SHANDALAR 0x00429473
static int SelectStatsJournalEntry(int journal_index)
{
  int requested_journal_index;
  unsigned int tile_y;
  unsigned int tile_x;
  int entry_type;
  int message_found;
  unsigned int entry_arg;

  requested_journal_index = journal_index;
  message_found = 0;
  if (journal_index < 0)
  {
    journal_index = 0;
  }
  if (g_stats_journal_entry_count < journal_index)
  {
    journal_index = g_stats_journal_entry_count;
  }
  if (g_stats_journal_current_entry == journal_index)
  {
    return 0;
  }
  strcpy(g_ui_message_buffer, "");
  while (g_ui_message_buffer[0] == 0)
  {
    entry_type = g_journal_entries[journal_index].type;
    entry_arg = g_journal_entries[journal_index].arg;
    tile_x = g_journal_entries[journal_index].tile_x;
    tile_y = g_journal_entries[journal_index].tile_y;
    if (g_journal_entries[journal_index].type == JOURNAL_ENTRY_NONE)
    {
      return 0;
    }
    BlitGraphicsRect(g_page1_window_bounds, 0, 0, global_screen_width, global_screen_height, g_page0_window_bounds, 0, 0);
    strcpy(g_ui_message_buffer, "");
    BuildStatsJournalEntryMessage(entry_type, entry_arg, tile_x, tile_y);
    if (g_ui_message_buffer[0] != '\0')
    {
      message_found = 1;
    }
    journal_index = journal_index + 1;
  }
  g_stats_journal_previous_marker_x = -1;
  g_stats_journal_previous_marker_y = g_stats_journal_previous_marker_x;
  if (journal_index - 1 == g_stats_journal_current_entry)
  {
    return 0;
  }

  AnimateStatsJournalMarkerToTile(tile_x, tile_y);
  g_stats_journal_current_entry = journal_index - 1;
}

// FUNCTION: SHANDALAR 0x004295f7
static void BuildStatsJournalEntryMessage(int entry_type, unsigned int entry_arg, int tile_x, int tile_y)
{
  char nearest_town_name[300];
  int card_id;
  char formatted_message[500];

  switch (entry_type)
  {
  case 1:
    if ((entry_arg & 0x80) != 0)
    {
      sprintf(formatted_message, gs_logstrings_0077c9a0[0], BuildTownDisplayName(entry_arg & 0x7f));
    }
    else
    {
      sprintf(formatted_message, gs_logstrings_0077c9a0[1], BuildTownDisplayName(entry_arg & 0x7f));
    }
    break;
  case 7:
    sprintf(formatted_message, gs_logstrings_0077c9a0[2], BuildTownDisplayName(entry_arg));
    break;
  case 13:
    strcpy(g_ui_message_buffer, "");
    FindNearestTownIndex(tile_x, tile_y);
    strcpy(nearest_town_name, g_ui_message_buffer);
    sprintf(formatted_message, gs_logstrings_0077c9a0[3], nearest_town_name);
    break;
  case 2:
    if ((entry_arg & 0x80) != 0)
    {
      sprintf(formatted_message, gs_logstrings_0077c9a0[4], BuildCreatureNameWithArticle(entry_arg & 0x7f));
    }
    else
    {
      sprintf(formatted_message, gs_logstrings_0077c9a0[5], BuildCreatureNameWithArticle(entry_arg & 0x7f));
    }
    break;
  case 3:
    sprintf(formatted_message, gs_logstrings_0077c9a0[6], GetDungeonName(entry_arg & 0x7f));
    break;
  case 4:
    if ((entry_arg & 0x80) != 0)
    {
      FormatMessageFromStringStripCarriageReturns(formatted_message, 0x1000, gs_logstrings_0077c9a0[7], GetDungeonName(entry_arg & 0x7f),
                                                  gs_cityname_manacastle_0077de00[(entry_arg & 0x7f) + 1]);
    }
    else
    {
      FormatMessageFromStringStripCarriageReturns(formatted_message, 0x1000, gs_logstrings_0077c9a0[8], GetDungeonName(entry_arg & 0x7f),
                                                  gs_cityname_manacastle_0077de00[(entry_arg & 0x7f) + 1]);
    }
    break;
  case 5:
    if ((int)entry_arg < 5)
    {
      strcpy(formatted_message, "");
    }
    else
    {
      sprintf(formatted_message, gs_logstrings_0077c9a0[9], GetLairName(entry_arg));
    }
    break;
  case 6:
    sprintf(formatted_message, gs_logstrings_0077c9a0[10], gs_worldmagic_names_00780660[entry_arg]);
    break;
  case 8:
  case 9:
  case 10:
  case 11:
  case 12:
    card_id = (entry_type - 8) * 0x100 + entry_arg;
    sprintf(formatted_message, gs_logstrings_0077c9a0[11], global_cards_data[card_id].name);
    break;
  case 18:
    switch (entry_arg & 0xffff)
    {
    case 1:
    case 2:
      strcpy(formatted_message, gs_logstrings_0077c9a0[12]);
      break;
    case 3:
      strcpy(formatted_message, gs_logstrings_0077c9a0[13]);
      break;
    case 4:
      sprintf(formatted_message, gs_logstrings_0077c9a0[14], BuildCreatureNameWithArticle((int)entry_arg >> 16));
      break;
    case 5:
      sprintf(formatted_message, gs_logstrings_0077c9a0[15], BuildTownDisplayName(FindNearestTownIndex(tile_x, tile_y)));
      break;
    default:
      strcpy(formatted_message, "");
      break;
    }
    break;
  default:
    strcpy(formatted_message, "");
    break;
  }
  strcpy(g_ui_message_buffer, formatted_message);
}

// FUNCTION: SHANDALAR 0x00429a62
static void AnimateStatsJournalMarkerToTile(unsigned int tile_x, unsigned int tile_y)
{
  int line_height;
  int text_width;
  int delta_y;
  int delta_x;
  int target_y;
  int previous_y;
  int target_x;
  int previous_x;

  if (g_stats_journal_previous_marker_y == -1)
  {
    g_stats_journal_previous_marker_y = tile_x;
    g_stats_journal_previous_marker_x = tile_y;
    ConvertWorldTileToMapScreenCoords(tile_x, tile_y, &target_x, &target_y);
    target_x = (global_screen_width * target_x) / 0x280;
    target_y = (global_screen_height * target_y) / 0x1e0;
    target_y += ScaleUiCoordinate(0x40);
  }
  else
  {
    ConvertWorldTileToMapScreenCoords(g_stats_journal_previous_marker_y, g_stats_journal_previous_marker_x, &previous_x, &previous_y);
    previous_x = (previous_x * global_screen_width) / 0x280;
    previous_y = (global_screen_height * previous_y) / 0x1e0;
    previous_y += ScaleUiCoordinate(0x40);
    ConvertWorldTileToMapScreenCoords(tile_x, tile_y, &target_x, &target_y);
    target_x = (global_screen_width * target_x) / 0x280;
    target_y = (global_screen_height * target_y) / 0x1e0;
    target_y += ScaleUiCoordinate(0x40);
    g_stats_journal_previous_marker_y = tile_x;
    g_stats_journal_previous_marker_x = tile_y;
    delta_x = target_x - previous_x;
    delta_y = target_y - previous_y;
    tile_x = previous_x;
    tile_y = previous_y;

    if (abs(delta_y) < abs(delta_x))
    {
      while (tile_x != target_x)
      {
        FillGraphicsRect(g_page0_window_bounds, tile_x, tile_y, 2, 2, 0xff);
        DelayUiTicks(5);
        if ((tile_x & 1) != 0)
        {
          FillGraphicsRect(g_page0_window_bounds, tile_x, tile_y, 2, 2, 0);
        }
        else
        {
          BlitGraphicsRect(g_page1_window_bounds, tile_x, tile_y, 2, 2, g_page0_window_bounds, tile_x, tile_y);
        }
        tile_x += SignNonZero(delta_x);
        tile_y = previous_y + (abs(tile_x - previous_x) * delta_y) / abs(delta_x);
      }
    }
    else
    {
      while (tile_y != target_y)
      {
        FillGraphicsRect(g_page0_window_bounds, tile_x, tile_y, 2, 2, 0xff);
        DelayUiTicks(5);
        if ((tile_y & 1) != 0)
        {
          FillGraphicsRect(g_page0_window_bounds, tile_x, tile_y, 2, 2, 0);
        }
        else
        {
          BlitGraphicsRect(g_page1_window_bounds, tile_x, tile_y, 2, 2, g_page0_window_bounds, tile_x, tile_y);
        }
        tile_y += SignNonZero(delta_y);
        tile_x = previous_x + (abs(tile_y - previous_y) * delta_x) / abs(delta_y);
      }
    }
  }

  g_stats_journal_marker_x = target_x;
  g_stats_journal_marker_y = target_y;
  if (g_ui_message_buffer[0] == '\0')
  {
    return;
  }

  tile_x = ClampIntToRange(target_x - 0x50, 0, 0xa0);
  tile_y = ClampIntToRange(target_y - 10, 0, 0xbf);
  text_width = MeasureMultilineTextWidth(g_page0_window_bounds, g_ui_message_buffer);
  line_height = GetFontLineHeight(g_page0_window_bounds->font_slot);
  g_page2_window_bounds->font_slot = g_page0_window_bounds->font_slot;
  BlitGraphicsRect(g_page1_window_bounds, (target_x - 2) - text_width / 2, target_y - 2, text_width + 2, line_height + 2,
                   g_page2_window_bounds, 0, 0xa0);
  ApplyPortraitTintMap(g_page2_window_bounds, 0, 0xa0, text_width + 2, line_height + 2, 0x3f3f3f, 1);
  DrawFormattedTextShadowedCentered(g_page2_window_bounds, 0xff, text_width / 2 + 1, line_height / 2 + 0xa1, g_ui_message_buffer);
  BlitGraphicsRect(g_page2_window_bounds, 0, 0xa0, text_width + 2, line_height + 2, g_page0_window_bounds,
                   (target_x - 2) - text_width / 2, target_y - 2);
}

// FUNCTION: SHANDALAR 0x00429161
void RunStatsWorldMapJournalMenu(void)
{
  int key_code;
  int menu_context;
  int journal_index;

  ShowWorldMapScreen(4);
  BlitGraphicsRect(g_page0_window_bounds, 0, 0, global_screen_width, global_screen_height, g_page1_window_bounds, 0, 0);
  g_stats_journal_previous_marker_x = -1;
  g_stats_journal_previous_marker_y = g_stats_journal_previous_marker_x;
  for (journal_index = 0; journal_index < 10000; journal_index = journal_index + 1)
  {
    if (g_journal_entries[journal_index].type == JOURNAL_ENTRY_NONE)
    {
      break;
    }
  }
  g_stats_journal_entry_count = journal_index;
  g_stats_journal_menu_controls[1].data_value = g_stats_journal_entry_count;
  g_stats_journal_current_entry = -1;
  ScaleMenuControlsForScreen(g_stats_journal_menu_controls, 3);
  menu_context = BeginMenuContext();
  ResetMenuContext(menu_context);
  g_stats_journal_menu_controls[2].state = 3;
  AddMenuControlsToContext(g_stats_journal_menu_controls, 3, menu_context);
  g_menu_render_guard = 1;
  g_page0_window_bounds->page_number = 1;
  RenderJournalExitButtonControl(&g_stats_journal_menu_controls[0], 0);
  RenderJournalScrollbar(&g_stats_journal_menu_controls[1]);
  g_menu_render_guard = 0;
  g_page0_window_bounds->page_number = 0;
  SelectStatsJournalEntry(0);

retry:
  ConsumeUiTickCount();
  g_stats_journal_menu_selection = -1;
  while (g_stats_journal_menu_selection == -1)
  {
    if ((((GetUiTickCount() % 0x14) == 0) & ((GetUiTickCount() % 10) == 0)) != 0)
    {
      FillGraphicsRect(g_page0_window_bounds, g_stats_journal_marker_x - 2, g_stats_journal_marker_y - 2, 5, 5, 0xff);
    }
    if ((((GetUiTickCount() % 10) == 0) & (GetUiTickCount() % 0x14)) != 0)
    {
      BlitGraphicsRect(g_page1_window_bounds, g_stats_journal_marker_x - 2, g_stats_journal_marker_y - 2, 5, 5, g_page0_window_bounds,
                       g_stats_journal_marker_x, g_stats_journal_marker_y);
    }
    UpdateMouseSnapshot();
    UpdateMenuControlSelection(g_mouse_x_snapshot, g_mouse_y_snapshot, g_mouse_button_down_mask);
    if ((g_stats_journal_menu_selection == -1) && ((g_mouse_button_down_mask != 0) || (IsKeyInputQueueEmpty() == 0)))
    {
      key_code = PopQueuedKeyInput();
      if (((g_mouse_button_down_mask & 1) != 0) || (key_code == 0x5000))
      {
        SelectStatsJournalEntry(g_stats_journal_current_entry + 1);
      }
      else
      {
        if (((g_mouse_button_down_mask & 2) != 0) || (key_code == 0x4800))
        {
          SelectStatsJournalEntry(g_stats_journal_current_entry - 1);
        }
      }
      ClearInputAndWaitForMouseRelease();
    }
  }

  switch (g_stats_journal_menu_selection)
  {
  case 4:
    ClearInputAndWaitForMouseRelease();
    EndMenuContext();
    FreeSpriteBlob(g_map_button_sprites[0][0]);
    break;
  case 1:
    goto retry;
    break;
  }
}

// FUNCTION: SHANDALAR 0x005227e3
void __cdecl ShowStatWinDialog(void *statwin_data, int mode)
{
  if (g_statwin_exports_by_ordinal[0] != 0)
  {
    ((void(__cdecl *)(void *, int))g_statwin_exports_by_ordinal[0])(statwin_data, mode);
  }
}

// FUNCTION: SHANDALAR 0x005344b6
void DrawDeckAnalysisPercentCentered(int value, int center_x, int y, int color_index)
{
  strcpy(g_ui_message_buffer, "");
  strcat(g_ui_message_buffer, _itoa((value * 100) / g_deck_analysis_card_count, g_itoa_buffer, 10));
  strcat(g_ui_message_buffer, "%");
  DrawScaledCenteredTextNoShadow(g_ui_message_buffer, center_x, y, color_index);
}

// FUNCTION: SHANDALAR 0x00431002
void DrawScaledTextNoShadow(char *text, int x, int y, int color_index)
{
  x = (x * global_screen_width) / 0x140;
  y = (y * global_screen_height) / 0xf0;
  DrawTextLineNoShadow(text, x, y, color_index);
}

// FUNCTION: SHANDALAR 0x00549002
int RunAdventureStatsMenu(void)
{
  int world_magic_slot_index;
  FacemakerWindowBounds portrait_work_page;
  FacemakerWindowBounds *portrait_work_page_ptr;
  int menu_context;
  int colors[5];
  int button_index;

  portrait_work_page.page_number = 4;
  portrait_work_page.clip_left = 0;
  portrait_work_page.clip_top = 0;
  portrait_work_page.max_x = 800;
  portrait_work_page.max_y = 600;
  portrait_work_page.draw_shadow_enabled = 1;
  portrait_work_page.text_color = 0xf;
  portrait_work_page.unk_1c = 4;
  portrait_work_page.font_slot = 0;
  portrait_work_page_ptr = &portrait_work_page;
  colors[3] = 0xb7;
  colors[4] = 0x1c;
  colors[0] = 10;
  colors[1] = 8;
  colors[2] = 8;

  if (g_stats_text_loaded == 0)
  {
    g_stats_text_table = LoadIniEscapedStringTable(g_advbuttons_ini_file, s_statText_00590618, g_ini_string_scratch);
    g_stats_text_loaded = 1;
  }

  AnimatePaletteToColor(0, g_default_palette_fade_steps);
  FillGraphicsRect(g_page0_window_bounds, 0, 0, global_screen_width, global_screen_height, 0);
  LoadPcxIntoPageNoPalette("advfac64.pic");
  LoadPcxIntoPage(1, "statbut1.pic");
  BeginSpriteEncodeSession();
  for (world_magic_slot_index = 0; world_magic_slot_index < 5; world_magic_slot_index = world_magic_slot_index + 1)
  {
    g_stats_menu_color_sprites[world_magic_slot_index] =
        EncodeSpriteFromPage(1, world_magic_slot_index * 0x31 + 1, 0x4c, 0x30, 0x30);
  }

  g_page1_window_bounds->font_slot = 6;
  for (world_magic_slot_index = 0; world_magic_slot_index < 3; world_magic_slot_index = world_magic_slot_index + 1)
  {
    SetFontStyleSize(6, 9 + (((unsigned int)(world_magic_slot_index - 2) < 1) ? -1 : 0));
    DrawFormattedTextNoShadowCentered(g_page1_window_bounds, colors[world_magic_slot_index], world_magic_slot_index * 0x55 + 0x2b, 0xc,
                                      "%s", (char *)g_stats_text_table[0]);
    g_stats_menu_button_sprites.normal[world_magic_slot_index] =
        EncodeSpriteFromPage(1, world_magic_slot_index * 0x55 + 1, 1, 0x54, 0x18);
    DrawFormattedTextNoShadowCentered(g_page1_window_bounds, colors[world_magic_slot_index], world_magic_slot_index * 0x4a + 0x26, 0x25,
                                      "%s", (char *)g_stats_text_table[1]);
    g_stats_menu_button_sprites.highlight[world_magic_slot_index] =
        EncodeSpriteFromPage(1, world_magic_slot_index * 0x4a + 1, 0x1a, 0x49, 0x18);
    DrawFormattedTextNoShadowCentered(g_page1_window_bounds, colors[world_magic_slot_index], world_magic_slot_index * 0x32 + 0x1a, 0x3e,
                                      "%s", (char *)g_stats_text_table[2]);
    g_stats_menu_button_sprites.pressed[world_magic_slot_index] =
        EncodeSpriteFromPage(1, world_magic_slot_index * 0x32 + 1, 0x33, 0x31, 0x18);
  }
  g_stats_menu_scrollbar_sprite = EncodeSpriteFromPage(1, 1, 0x7d, 0x19, 0xa1);
  g_stats_menu_scroll_thumb_sprite = EncodeSpriteFromPage(1, 0x1b, 0x7d, 0xf, 0x12);
  FinalizeSpriteEncodeSession();
  ScaleMenuControlsForScreen(g_stats_menu_controls, 10);
  g_stats_menu_controls[4].state = 3;
  g_stats_creature_page_index = 0;

  while (1)
  {
    RenderAdventureStatsOverview(portrait_work_page_ptr, colors);

    switch (global_screen_width)
    {
    case 0x280:
      LoadPcxIntoPageOpaque(1, "creatures640.pic");
      g_stats_creature_sheet_resolution = 0;
      break;
    case 800:
      LoadPcxIntoPageOpaque(1, "creatures800.pic");
      g_stats_creature_sheet_resolution = 1;
      break;
    case 0x400:
      LoadPcxIntoPageOpaque(1, "creatures1024.pic");
      g_stats_creature_sheet_resolution = 2;
      break;
    }
    g_stats_creature_page_index = 0;
    RenderStatsCreatureGridPage(g_stats_creature_page_index);
    RenderStatsCreatureScrollbar(&g_stats_menu_controls[4]);

  retry:
    menu_context = BeginMenuContext();
    ResetMenuContext(menu_context);
    AddMenuControlsToContext(g_stats_menu_controls, 0x16, menu_context);
    g_menu_render_guard = 1;
    for (button_index = 0; button_index < 3; button_index = button_index + 1)
    {
      RenderStatsTabButtonControl(&g_stats_menu_controls[button_index], 0);
    }
    for (button_index = 10; button_index < 0x16; button_index = button_index + 1)
    {
      RenderBasicImageMenuControl(&g_stats_menu_controls[button_index], 0);
    }
    g_menu_render_guard = 0;
    g_stats_menu_selection = -1;
    while (g_stats_menu_selection == -1)
    {
      UpdateMouseSnapshot();
      UpdateMenuControlSelection(g_mouse_x_snapshot, g_mouse_y_snapshot, g_mouse_button_down_mask);
    }
    EndMenuContext();

    switch (g_stats_menu_selection)
    {
    case 1:
      ClearInputAndWaitForMouseRelease();
      ShowStatsWindow(0, -1);
      break;
    case 2:
      RunStatsWorldMapJournalMenu();
      break;
    case 3:
      FreeSpriteBlob(g_stats_menu_color_sprites[0]);
      AnimatePaletteToColor(0, g_default_palette_fade_steps);
      LoadPcxIntoPageNoPalette("advfac64.pic");
      return 0;
    case 4:
    case 5:
      goto retry;
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
      if (g_world_magic_town_flags[g_stats_menu_selection - 10] != 0)
      {
        AnimatePaletteToColor(0, g_default_palette_fade_steps);
        ShowStatsWindow(0x101, g_stats_menu_selection - 9);
      }
      ClearInputAndWaitForMouseRelease();
      if (g_world_magic_town_flags[g_stats_menu_selection - 10] == 0)
      {
        goto retry;
      }
      break;
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
      ShowWorldMagicStatsDetail(g_stats_menu_selection - 0xf);
      break;
    }
  }
  
  return 0;
}

// FUNCTION: SHANDALAR 0x0056335f
int ShowStatsWindow(int mode, int highlight)
{
  unsigned int color_order[7];
  int mapped_color;              // ebp - 0x20
  int duel_win_count;            // ebp - 0x1c
  int scan_index;                // ebp - 0x18
  int wizard_color_index;        // ebp - 0x14
  int old_timer_thread_priority; // ebp - 0x10
  int required_duel_wins;        // ebp - 0xc
  int ruled_town_count;          // ebp - 0x8
  int old_main_thread_priority;  // ebp - 0x4

  AnimatePaletteToColor(0, g_default_palette_fade_steps);
  ClearGraphicsPageWithPaletteColor(0, 0);
  LoadPcxIntoPageNoPalette("advfac64.pic");
  old_timer_thread_priority = 0;
  old_main_thread_priority = 0;

  for (wizard_color_index = 0; wizard_color_index < 5; wizard_color_index = wizard_color_index + 1)
  {
    duel_win_count = 0;
    ruled_town_count = 0;
    mapped_color = MapWizardColorToDisplayIndex(wizard_color_index + 1);
    for (scan_index = 0; scan_index < 0x80; scan_index = scan_index + 1)
    {
      if ((g_town_slots[scan_index].status_and_ruling_wizard & 0xff00) &&
          (((int)g_town_slots[scan_index].status_and_ruling_wizard >> 8) - 1 == wizard_color_index))
      {
        ruled_town_count = ruled_town_count + 1;
      }
    }
    g_statwin_data.town_count_by_color[mapped_color] = (unsigned char)ruled_town_count;
    if (g_world_magic_town_flags[wizard_color_index] != 0)
    {
      g_statwin_data.progress_by_color[mapped_color] = 0;
    }
    else
    {
      required_duel_wins = color_order[6] = g_shandalar_difficulty * ruled_town_count + g_shandalar_difficulty * 5 + 0x1e;
      for (scan_index = 0; scan_index < 1000; scan_index = scan_index + 1)
      {
        if (g_duel_victory_log[scan_index] == '\0')
          break;

        if (((int)(char)g_duel_victory_log[scan_index] >> 4) == wizard_color_index + 1)
        {
          duel_win_count = duel_win_count + 1;
        }
      }
      g_statwin_data.duel_wins_by_color[mapped_color] = duel_win_count;
      color_order[6] = MAX(g_shandalar_difficulty * 5 + 0x14, required_duel_wins - duel_win_count);

      g_statwin_data.progress_by_color[mapped_color] = 0x1e - (required_duel_wins - color_order[6]);
    }
  }

  g_statwin_data.highlighted_wizard_color = 0;
  g_statwin_data.highlighted_creature_type = 0;
  g_statwin_data.highlighted_lair_color = 0;
  if ((mode & 0xff) == 1)
  {
    if (((unsigned int)mode & 0x100U) == 0)
    {
      g_world_magic_town_flags[highlight - 1] = 1;
    }
    g_statwin_data.highlighted_wizard_color = (unsigned char)MapWizardColorToDisplayIndex(highlight);
    g_statwin_data.progress_by_color[g_statwin_data.highlighted_wizard_color] = 0;
    mode &= 0xff;
  }
  if (mode == 2)
  {
    color_order[0] = (unsigned int)highlight >> 0x10;
    color_order[1] = 4;
    color_order[2] = 2;
    color_order[3] = 3;
    color_order[4] = 1;
    color_order[5] = 0;
    highlight &= 0xffff;
    if (g_shandalar_monster_definitions[highlight].color_mask == -1)
    {
      mode = 0;
    }
    else
    {
      g_statwin_data.highlighted_creature_type = (unsigned char)highlight;
      g_statwin_data.highlighted_wizard_color = (unsigned char)color_order[color_order[0]];
    }
  }
  if (mode == 3)
  {
    g_statwin_data.highlighted_lair_color = (unsigned char)highlight;
  }
  ShowStatWinDialog(&g_statwin_data, mode);
  (void)old_timer_thread_priority;
  (void)old_main_thread_priority;
  SetForegroundWindow(g_main_window_hwnd);
  BringWindowToTop(g_main_window_hwnd);
  SetFocus(g_main_window_hwnd);
  LoadPcxIntoPageNoPalette("advfac64.pic");
  ReadPalette("todpal.tr", (char *)0);
  SelectPalette(g_graphics_pages[0]->hTempDC, g_palette_handle, 0);
  RealizePalette(g_graphics_pages[0]->hTempDC);
  return 0;
}

// FUNCTION: SHANDALAR 0x00533ccd
void AnalyzeDeckAndMaybeShowReport(int show_ui)
{
  int land_color_count[8];
  int card_type_count_by_color[7][8];
  int color_index;
  int column_index;
  int deck_index;
  int y;
  int x;
  unsigned int card_id_masked;
  int max_creature_count;
  int rarity_count[3];

  g_deck_analysis_card_count = 0;
  for (color_index = 0; color_index <= 7; color_index = color_index + 1)
  {
    land_color_count[color_index] = 0;
    for (column_index = 0; column_index < 7; column_index = column_index + 1)
    {
      card_type_count_by_color[column_index][color_index] = 0;
    }
  }
  for (color_index = 0; color_index < 3; color_index = color_index + 1)
  {
    rarity_count[color_index] = 0;
  }
  for (deck_index = 0; deck_index < 500; deck_index = deck_index + 1)
  {
    if ((deck[deck_index] & 0x4000) == 0)
    {
      g_deck_analysis_card_count = g_deck_analysis_card_count + 1;
      card_id_masked = deck[deck_index] & 0xfff;
      color_index = (int)global_cards_data[card_id_masked].color;
      if (global_cards_data[card_id_masked].rarity == '\x03')
      {
        rarity_count[2] = rarity_count[2] + 1;
      }
      else if (global_cards_data[card_id_masked].rarity == '\x02')
      {
        rarity_count[1] = rarity_count[1] + 1;
      }
      else
      {
        rarity_count[0] = rarity_count[0] + 1;
      }
      if ((global_cards_data[card_id_masked].type & 1) != 0)
      {
        for (column_index = 1; column_index < 7; column_index = column_index + 1)
        {
          if (((1 << (byte)column_index) & (int)global_cards_data[card_id_masked].color) != 0)
          {
            land_color_count[column_index] = land_color_count[column_index] + 1;
            land_color_count[7] = land_color_count[7] + 1;
          }
        }
      }
      color_index = single_color_test_bit_to_color_t(color_index);
      switch (global_cards_data[card_id_masked].type)
      {
      case '\x01':
        card_type_count_by_color[0][color_index] = card_type_count_by_color[0][color_index] + 1;
        card_type_count_by_color[0][0] = card_type_count_by_color[0][0] + 1;
        break;
      case '\x02':
        if (4 < (int)global_cards_data[card_id_masked].power + (int)global_cards_data[card_id_masked].toughness)
        {
          card_type_count_by_color[2][color_index] = card_type_count_by_color[2][color_index] + 1;
          card_type_count_by_color[2][0] = card_type_count_by_color[2][0] + 1;
        }
        else
        {
          card_type_count_by_color[1][color_index] = card_type_count_by_color[1][color_index] + 1;
          card_type_count_by_color[1][0] = card_type_count_by_color[1][0] + 1;
        }
        break;
      case '\x04':
        card_type_count_by_color[3][color_index] = card_type_count_by_color[3][color_index] + 1;
        card_type_count_by_color[3][0] = card_type_count_by_color[3][0] + 1;
        break;
      case '\x08':
        card_type_count_by_color[4][color_index] = card_type_count_by_color[4][color_index] + 1;
        card_type_count_by_color[4][0] = card_type_count_by_color[4][0] + 1;
        break;
      case '\x10':
      case ' ':
        card_type_count_by_color[5][color_index] = card_type_count_by_color[5][color_index] + 1;
        card_type_count_by_color[5][0] = card_type_count_by_color[5][0] + 1;
        break;
      case '@':
      case 'B':
        card_type_count_by_color[6][color_index] = card_type_count_by_color[6][color_index] + 1;
      }
    }
  }
  if (show_ui != 0)
  {
    ClearGraphicsPageWithPaletteColor(0, 0);
    g_page0_window_bounds->font_slot = 4;
    x = 0x10;
    y = x;
    sprintf(g_ui_message_buffer, gs_analyze_0074b870[0], g_deck_analysis_card_count);
    DrawScaledTextNoShadow(g_ui_message_buffer, x, y, 0xf6);
    DrawScaledCenteredTextNoShadow(gs_analyze_0074b870[1], 0x80, y, 0xff);
    DrawScaledCenteredTextNoShadow(gs_analyze_0074b870[2], 0xb0, y, 0xff);
    DrawScaledCenteredTextNoShadow(gs_analyze_0074b870[3], 0xd0, y, 0xff);
    DrawScaledCenteredTextNoShadow(gs_analyze_0074b870[4], 0xf0, y, 0xff);
    DrawScaledCenteredTextNoShadow(gs_analyze_0074b870[5], 0x110, y, 0xff);
    DrawScaledCenteredTextNoShadow(gs_analyze_0074b870[6], 0x130, y, 0xff);
    DrawGraphicsLine(g_page0_window_bounds, 0, y * 2 + 0x18, global_screen_width - 1, y * 2 + 0x18, 0xf4);
    DrawGraphicsLine(g_page0_window_bounds, 0x130, 0, 0x130, global_screen_height - 1, 0xf4);
    y = 0x20;
    DrawScaledTextNoShadow(gs_analyze_0074b870[7], x, y, 0xff);
    DrawDeckAnalysisPercentCentered(card_type_count_by_color[0][0], 0x80, y, 0xff);
    for (deck_index = 1; deck_index <= 5; deck_index = deck_index + 1)
    {
      DrawDeckAnalysisPercentCentered(land_color_count[deck_index], deck_index * 0x20 + 0x90, y, 0xff);
    }
    y = y + 0xc;
    for (column_index = 1; column_index < 6; column_index = column_index + 1)
    {
      switch (column_index)
      {
      case 1:
        strcpy(g_ui_message_buffer, gs_analyze_0074b870[8]);
        break;
      case 2:
        strcpy(g_ui_message_buffer, gs_analyze_0074b870[9]);
        break;
      case 3:
        strcpy(g_ui_message_buffer, gs_analyze_0074b870[10]);
        break;
      case 4:
        strcpy(g_ui_message_buffer, gs_analyze_0074b870[11]);
        break;
      case 5:
        strcpy(g_ui_message_buffer, gs_analyze_0074b870[12]);
        break;
      }
      DrawScaledTextNoShadow(g_ui_message_buffer, x, y, 0xff);
      DrawDeckAnalysisPercentCentered(card_type_count_by_color[column_index][0], 0x80, y, 0xff);
      for (deck_index = 1; deck_index <= 5; deck_index = deck_index + 1)
      {
        DrawDeckAnalysisPercentCentered(card_type_count_by_color[column_index][deck_index], deck_index * 0x20 + 0x90, y, 0xff);
      }
      y = y + 0xc;
    }
    DrawScaledTextNoShadow(gs_analyze_0074b870[0xd], x, y, 0xff);
    DrawDeckAnalysisPercentCentered(card_type_count_by_color[6][0], 0x80, y, 0xff);
    y = y + 0x18;
    x = 0x80;
    DrawScaledTextNoShadow(gs_analyze_0074b870[0xe], 0x80, y, 0xf6);
    strcat(g_ui_message_buffer, " ");
    DrawDeckAnalysisPercentCentered(rarity_count[0], x + 0x30, y, 0xf6);
    y = y + 8;
    DrawScaledTextNoShadow(gs_analyze_0074b870[0xf], x, y, 0xf6);
    strcat(g_ui_message_buffer, " ");
    DrawDeckAnalysisPercentCentered(rarity_count[1], x + 0x30, y, 0xf6);
    y = y + 8;
    DrawScaledTextNoShadow(gs_analyze_0074b870[0x10], x, y, 0xf6);
    strcat(g_ui_message_buffer, " ");
    DrawDeckAnalysisPercentCentered(rarity_count[2], x + 0x30, y, 0xf6);
    y = y + 8;
    WaitForInputEventUnlessBlocked();
  }
  g_analyzed_deck_special_rules = 0;
  max_creature_count = card_type_count_by_color[2][0];
  if (max_creature_count < card_type_count_by_color[3][0])
  {
    g_analyzed_deck_special_rules = 1;
    max_creature_count = card_type_count_by_color[3][0];
  }
  if (max_creature_count < card_type_count_by_color[5][0] + card_type_count_by_color[6][0])
  {
    g_analyzed_deck_special_rules = 2;
  }
}
