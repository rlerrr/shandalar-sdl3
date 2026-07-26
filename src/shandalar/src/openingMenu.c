#include "platform/gdi_compat.h"
#include <stdio.h>
#include <string.h>

#include "defs.h"
#include "shandalar.h"
#include "magic/src/global_state.h"
#include "facemaker/src/facemaker_types.h"
#include "drawcardlib/src/pic.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef MODERN_FIXES
void shandalar_host_pump(void);
void UpdateMouseSnapshot(void);
#endif

extern int global_screen_width;
extern int global_screen_height;
extern int g_graphics_bpp;
extern RpBitsPalettePacket g_palette_data_words;
extern int g_default_palette_fade_steps;
extern FILE *g_advbuttons_ini_file;
extern char g_ui_message_buffer[0x1000];
extern char g_ini_string_scratch[0x28];
extern int g_mouse_x_snapshot;
extern int g_mouse_y_snapshot;
extern int g_mouse_button_down_mask;
extern int g_mouse_button_mask_snapshot;
extern int g_mouse_y;
extern int g_mouse_x;
extern int g_menu_render_guard;
extern char *gs_loadsave_0077d1b0[3];
extern HWND g_main_window_hwnd;
extern EncodedImage *g_face_preview_sprite_group[6];
extern EncodedImage *g_face_preview_sprite_selected;
extern DIBSurface *g_graphics_pages[10];
extern char g_text_lines[249][300];

extern FacemakerWindowBounds *g_page0_window_bounds;
extern FacemakerWindowBounds *g_page1_window_bounds;
extern FacemakerWindowBounds *g_page2_window_bounds;
static EncodedImage *g_name_entry_done_button_sprites[3];
static EncodedImage *g_loadsave_cancel_button_sprites[3];

int *LoadIniEscapedStringTable(FILE *ini_file, char *section_name, char *scratch);
int MeasureMultilineTextWidth(FacemakerWindowBounds *window, char *text);
int SetFontStyleSize(int font_id, unsigned int style);
int GetFontStyleSize(int font_id);
int DrawTextFormatted(FacemakerWindowBounds *dst, int text_id, int draw_shadow, int scale_to_screen, int center_x, int center_y, int x,
                      int y, int *format_and_args);
int LoadTextSectionLines(const char *filename, const char *section);
int PopQueuedKeyInput(void);
void LoadPcxResource(int page_number, int x, int y, char *path, void *opaque);
void LoadPcxIntoPage(int page_number, char *path);
void ShandalarLoadPcxIntoPageNoPalette(char *path);
int ClearGraphicsPageWithPaletteColor(int page_number, int color_index);
void CopyGraphicsRect(FacemakerWindowBounds *src, int src_x, int src_y, unsigned int width, int height,
                      FacemakerWindowBounds *dst, int dst_x, int dst_y);
AdvMenuRect *PushGraphicsClipRect(AdvMenuRect *out_rect, FacemakerWindowBounds *page, int x, int y, int width, int height);
void BeginSpriteEncodeSession(void);
EncodedImage *EncodeSpriteFromPage(int page_number, int x, int y, int width, int height);
void FinalizeSpriteEncodeSession(void);
void PlaySoundEffectOnChannel(char *sound_path, int channel, int volume, int pitch_percent, int pan_percent);
void BlitGraphicsRect(FacemakerWindowBounds *dst, unsigned int dst_x, int dst_y, unsigned int width, DWORD height,
                      FacemakerWindowBounds *src, int src_x, int src_y);
void DrawEncodedImageResampled(FacemakerWindowBounds *dst, int x, int y, int width, int height, EncodedImage *encoded_image);
void StretchBlitGraphicsRect(FacemakerWindowBounds *dst, int dst_x, int dst_y, int src_w, int src_h,
                             FacemakerWindowBounds *src, int src_x, int src_y, int copy_w, int copy_h);
int FileExists(const char *filename);
int ReadSpriteEntryPointers(EncodedImage **out_sprite_entries, char *sprite_table_name);
void FreeSpriteBlob(void *memory);
int AnimatePaletteToColor(int color_index, int palette_id);
void set_global_base_directory(char *path);
int BeginMenuContext(void);
int ResetMenuContext(int context_index);
int AddMenuControlsToContext(AdvMenuControl *controls, int control_count, int context_index);
int EndMenuContext(void);
int FadeInPaletteFromGray(int gray, int steps);
void DrawFormattedTextShadowedCenterY(FacemakerWindowBounds *window, int color, int x, int y, char *format, ...);
void DrawFormattedTextShadowedCentered(FacemakerWindowBounds *window, int color, int x, int y, char *format, ...);
void __cdecl DrawFormattedTextNoShadowCentered(FacemakerWindowBounds *dst, int text_color, int x, int y, char *format, ...);
void DrawFormattedTextShadowed(FacemakerWindowBounds *window, int color, int x, int y, char *format, ...);
void LoadPcxIntoPageOpaque(int page_number, char *path);
unsigned int WaitForInputEventUnlessBlocked(void);
void ClearInputAndWaitForMouseRelease(void);
int UpdateMenuControlSelection(int mouse_x, int mouse_y, int allow_activate_on_click);
int ScaleUiCoordinate(int value);
int Palette_FindNearestEntryIndex(int target_r, int target_g, int target_b, unsigned char *palette_bytes);
void *CreateGraphicsPage(int page_number, int width, int height, int bits_per_pixel);
void SetGraphicsPage(int page_number, void *page);
void ReadGraphicsScanline(unsigned int *out_scanline, int page_number, int src_x, int src_y, unsigned int byte_count);
void WriteGraphicsScanline(unsigned int *scanline_data, int page_number, int dst_x, int dst_y, unsigned int byte_count);
void FillGraphicsRect(FacemakerWindowBounds *window_bounds, int x, int y, int width, int height, unsigned int color_index);
void DrawGraphicsLine(FacemakerWindowBounds *window_bounds, int x1, int y1, int x2, int y2, int color_index);
void DelayUiTicks(int ticks);
int GetUiTickCount(void);
int GetFontCharWidth(int font_slot, char ch);
int GetFontLineHeight(int font_slot);
void DrawLoadSaveButtonText(FacemakerWindowBounds *window, int color, int x, int y, ...);
int RenderAdvMenuControlDisabled(AdvMenuControl *control);
int RenderAdvMenuControlNormally(AdvMenuControl *control);
int int_to_hex_digit(int value);
int ShowHallBackgroundScreen(void);
int RunNameEntryDialog(char *name_buffer);
void BuildFacemakerPortraitSprites(FacemakerWindowBounds *page);
int BlitTransparentRuns(FacemakerWindowBounds *src, int src_x, int src_y, unsigned int width, int height, FacemakerWindowBounds *dst, int dst_x, int dst_y);
int ApplyNameEntryKey(char *name_buffer, int key_code, int max_len);
int DrawBlinkingNameCaret(FacemakerWindowBounds *window, int color_index, int caret_x, int caret_y, char *name_buffer, int cursor_pos);
int ApplyPortraitPaletteMap(FacemakerWindowBounds *page, int src_x, int src_y, unsigned int width, int height, char *palette_source_path, char *portrait_path);
int ApplyPortraitTintMap(FacemakerWindowBounds *page, int src_x, int src_y, unsigned int width, int height, unsigned int tint_mask, int tint_mode);
int HandleLoadSaveEscControlEvent(AdvMenuControl *control, int event_type);
int ActivateLoadSaveEscControl(AdvMenuControl *control);
int RenderLoadSaveCancelButton(AdvMenuControl *control, int mode);
int DrawLoadSaveButton(FacemakerWindowBounds *window, EncodedImage **button_sprites, int button_width, int button_height, char *button_text,
                       int font_slot, int visual_state);
int HandleLoadSaveSlotControlEvent(AdvMenuControl *control, int event_type);
int ActivateLoadSaveSlotControl(AdvMenuControl *control);

int HandleOpeningMenuControlEvent(AdvMenuControl *control, int event_type);
int HandleOpeningMenuControlActivate(AdvMenuControl *control);

int SetSelectedMenuValue(AdvMenuControl *control, int event_type);
int ActivateSelectedMenuValue(AdvMenuControl *control);
int HandleEscControlEvent(AdvMenuControl *control, int event_type);
int HandleDifficultyMenuControlEvent(AdvMenuControl *control, int event_type);
int DrawDifficultyMenuEntry(int difficulty_index, int visual_state);
int HandleDifficultyMenuControlActivate(AdvMenuControl *control);
int DrawColorMenuEntry(int color_index, int visual_state);
int HandleColorMenuControlEvent(AdvMenuControl *control, int event_type);
int HandleColorMenuControlActivate(AdvMenuControl *control);
int shandalar_run_facemaker_flow(void);

static int FindNearestVisiblePaletteEntry(int target_r, int target_g, int target_b, unsigned char *palette_bytes)
{
  int palette_index;
  int best_index;
  int best_distance;
  int dr;
  int dg;
  int db;
  int distance;

  best_index = 1;
  best_distance = 0x7fffffff;
  for (palette_index = 1; palette_index < 0x100; palette_index = palette_index + 1)
  {
    dr = target_r - (int)palette_bytes[palette_index * 3];
    dg = target_g - (int)palette_bytes[palette_index * 3 + 1];
    db = target_b - (int)palette_bytes[palette_index * 3 + 2];
    distance = dr * dr + dg * dg + db * db;
    if (distance < best_distance)
    {
      best_distance = distance;
      best_index = palette_index;
    }
  }
  return best_index;
}

static int ApplyVisibleSpritePaletteMap(FacemakerWindowBounds *page, int src_x, int src_y, unsigned int width, int height, char *palette_source_path,
                                        char *target_palette_path)
{
  unsigned int scanline_buffer[0x200];
  RpBitsPalettePacket source_palette;
  RpBitsPalettePacket target_palette;
  unsigned char remap_table[0x100];
  unsigned char *palette_cursor;
  int y;
  int x;
  int red;
  int green;
  int blue;

  LoadPcxResource(-1, 0, 0, palette_source_path, &source_palette);
  LoadPcxResource(-1, 0, 0, target_palette_path, &target_palette);

  remap_table[0] = 0;
  palette_cursor = source_palette.entry_data + 3;
  for (x = 1; x < 0x100; x = x + 1)
  {
    red = (int)palette_cursor[0];
    green = (int)palette_cursor[1];
    blue = (int)palette_cursor[2];
    remap_table[x] = (unsigned char)FindNearestVisiblePaletteEntry(red, green, blue, target_palette.entry_data);
    palette_cursor = palette_cursor + 3;
  }

  for (y = src_y; y < src_y + height; y = y + 1)
  {
    ReadGraphicsScanline(scanline_buffer, page->page_number, src_x, y, width);
    for (x = 0; x < (int)width; x = x + 1)
    {
      ((unsigned char *)scanline_buffer)[x] = remap_table[((unsigned char *)scanline_buffer)[x]];
    }
    WriteGraphicsScanline(scanline_buffer, page->page_number, src_x, y, width);
  }

  return 0;
}

static void ResetNameEntryDoneButtonSprites(void)
{
  if (g_name_entry_done_button_sprites[0] != 0)
  {
    FreeSpriteBlob((void *)g_name_entry_done_button_sprites[0]);
  }
  g_name_entry_done_button_sprites[0] = 0;
  g_name_entry_done_button_sprites[1] = 0;
  g_name_entry_done_button_sprites[2] = 0;
}

static void BuildLoadSaveCancelButtonSprites(void)
{
  EncodedImage *source_sprites[13];
  EncodedImage *source_sprite;
  int source_sprite_indices[3] = {3, 4, 4};
  int i;

  if (g_loadsave_cancel_button_sprites[0] != 0)
  {
    return;
  }

  ReadSpriteEntryPointers(&source_sprites[0], "BuyButtons.spr");
  BeginSpriteEncodeSession();
  for (i = 0; i < 3; i = i + 1)
  {
    source_sprite = source_sprites[source_sprite_indices[i]];
    FillGraphicsRect(g_page1_window_bounds, 0, 0, (int)source_sprite->width, (int)source_sprite->height, 0);
    DrawEncodedImageResampled(g_page1_window_bounds, 0, 0, (int)source_sprite->width, (int)source_sprite->height, source_sprite);
    ApplyVisibleSpritePaletteMap(g_page1_window_bounds, 0, 0, (unsigned int)source_sprite->width, (int)source_sprite->height, "buycards.pic", "menopt.pic");
    g_loadsave_cancel_button_sprites[i] = EncodeSpriteFromPage(1, 0, 0, (int)source_sprite->width, (int)source_sprite->height);
  }
  FinalizeSpriteEncodeSession();
  FreeSpriteBlob((void *)source_sprites[0]);
}

static void BuildNameEntryDoneButtonSprites(void)
{
  // Uses page 2 as scratch, which is safe: only called from RunNameEntryDialog via
  // shandalar_run_facemaker_flow (character creation), before the world map exists.

  int i;
  int source_sprite_indices[3] = {3, 4, 4};
  EncodedImage *source_sprites[13];
  EncodedImage *source_sprite;

  if (g_name_entry_done_button_sprites[0] != 0)
  {
    return;
  }

  ReadSpriteEntryPointers(&source_sprites[0], "BuyButtons.spr");
  BeginSpriteEncodeSession();
  for (i = 0; i < 3; i = i + 1)
  {
    source_sprite = source_sprites[source_sprite_indices[i]];
    FillGraphicsRect(g_page2_window_bounds, 0, 0, (int)source_sprite->width, (int)source_sprite->height, 0);
    DrawEncodedImageResampled(g_page2_window_bounds, 0, 0, (int)source_sprite->width, (int)source_sprite->height, source_sprite);
    ApplyVisibleSpritePaletteMap(g_page2_window_bounds, 0, 0, (unsigned int)source_sprite->width, (int)source_sprite->height, "buycards.pic",
                                 "namepick.pic");
    g_name_entry_done_button_sprites[i] = EncodeSpriteFromPage(2, 0, 0, (int)source_sprite->width, (int)source_sprite->height);
  }
  FinalizeSpriteEncodeSession();
  FreeSpriteBlob((void *)source_sprites[0]);
}

static void GetNameEntryDoneButtonRect(int *out_x, int *out_y, int *out_w, int *out_h)
{
  EncodedImage *sprite;

  BuildNameEntryDoneButtonSprites();
  sprite = g_name_entry_done_button_sprites[0];
  *out_w = sprite ? sprite->width : 0x59;
  *out_h = sprite ? sprite->height : 0x23;
  *out_x = (global_screen_width - *out_w) / 2;
  *out_y = ScaleUiCoordinate(0x12c) + 0x6a + 6;
}

static int GetNameEntryDoneButtonState(void)
{
  int x;
  int y;
  int w;
  int h;

  GetNameEntryDoneButtonRect(&x, &y, &w, &h);
  if (x <= g_mouse_x && g_mouse_x <= x + w && y <= g_mouse_y && g_mouse_y <= y + h)
  {
    return (g_mouse_button_down_mask & 1) ? 2 : 1;
  }
  return 0;
}

static void DrawNameEntryDoneButton(void)
{
  int x;
  int y;
  int w;
  int h;
  int state;

  GetNameEntryDoneButtonRect(&x, &y, &w, &h);
  state = GetNameEntryDoneButtonState();
  DrawEncodedImageResampled(g_page0_window_bounds, x, y, w, h, g_name_entry_done_button_sprites[state]);
}

static int IsNameEntryDoneButtonPressed(void)
{
  int x;
  int y;
  int w;
  int h;

  GetNameEntryDoneButtonRect(&x, &y, &w, &h);
  return (g_mouse_button_mask_snapshot & 1) != 0 &&
         (g_mouse_button_down_mask & 1) == 0 &&
         x <= g_mouse_x_snapshot && g_mouse_x_snapshot <= x + w &&
         y <= g_mouse_y_snapshot && g_mouse_y_snapshot <= y + h;
}

#ifdef MODERN_FIXES
void PumpOpeningMenuWait(void)
{
  UpdateMouseSnapshot();
}
#else
void PumpOpeningMenuWait(void)
{
#ifdef __EMSCRIPTEN__
  emscripten_sleep(0);
#else
  SDL_Delay(1);
#endif
}
#endif

// GLOBAL: SHANDALAR 0x005aa0c8
EncodedImage *g_opening_menu_unused_sprite_group_d[2];
// GLOBAL: SHANDALAR 0x005aa3f8
EncodedImage *g_opening_menu_unused_sprite_group_a[3];
// GLOBAL: SHANDALAR 0x005aa408
EncodedImage *g_opening_menu_unused_sprite_group_b[2];
// GLOBAL: SHANDALAR 0x005aa410
EncodedImage *g_opening_menu_sprite_blob_handle;
// GLOBAL: SHANDALAR 0x005aa424
int g_opening_menu_strings_loaded;
// GLOBAL: SHANDALAR 0x005aa430
EncodedImage *g_opening_menu_icon_state_sprites[4];
// GLOBAL: SHANDALAR 0x005aa440
EncodedImage *g_opening_menu_unused_sprite_group_c[3];
// GLOBAL: SHANDALAR 0x005a9d8c
int *g_opening_menu_text_table;
// GLOBAL: SHANDALAR 0x005a9d90
EncodedImage *g_opening_menu_unused_sprite_group_e[2];
// GLOBAL: SHANDALAR 0x00587378
AdvMenuRect g_opening_menu_entry_rects[4] = {{0xb8, 0x121, 0x109, 0x29}, {0xb8, 0x14a, 0x109, 0x29}, {0xb8, 0x173, 0x109, 0x29}, {0xb8, 0x19c, 0x109, 0x29}};
// GLOBAL: SHANDALAR 0x005873c8
int g_opening_menu_icon_size = 0x18;
// GLOBAL: SHANDALAR 0x005873cc
int g_opening_menu_entry_height = 0x29;
// GLOBAL: SHANDALAR 0x005873d0
int g_opening_menu_icon_label_spacing = 0x8c;
// GLOBAL: SHANDALAR 0x005873d8
int g_opening_menu_entry_enabled[4] = {1, 1, 1, 1};
// GLOBAL: SHANDALAR 0x00587d30
char g_opening_menu_esc_hotkeys[] = "Ee\x1b";
// GLOBAL: SHANDALAR 0x00587d34
char g_opening_menu_esc_activate_hotkeys[4] = "\x1b";
// GLOBAL: SHANDALAR 0x00587d38
char g_difficulty_menu_esc_navigate_hotkeys[4] = "\x1b";
// GLOBAL: SHANDALAR 0x00587d3c
char g_difficulty_menu_esc_activate_hotkeys[4] = "\x1b";
// GLOBAL: SHANDALAR 0x00587d40
char g_color_menu_esc_navigate_hotkeys[4] = "\x1b";
// GLOBAL: SHANDALAR 0x00587d44
char g_color_menu_esc_activate_hotkeys[4] = "\x1b";
// GLOBAL: SHANDALAR 0x00781778
int g_name_entry_cursor;
// GLOBAL: SHANDALAR 0x00781774
int g_name_entry_insert_mode;
// GLOBAL: SHANDALAR 0x00789924
DIBSurface *g_facemaker_page4_dib;
// GLOBAL: SHANDALAR 0x00789928
HBITMAP g_facemaker_page4_bitmap;
extern char s_portrait_palette_map_empty_palette_path_0058d3bc[];
extern char s_portrait_palette_map_empty_portrait_path_0058d3c0[];
// GLOBAL: SHANDALAR 0x0058d3b4
char *g_portrait_palette_map_cached_palette_path = s_portrait_palette_map_empty_palette_path_0058d3bc;
// GLOBAL: SHANDALAR 0x0058d3b8
char *g_portrait_palette_map_cached_portrait_path = s_portrait_palette_map_empty_portrait_path_0058d3c0;
// GLOBAL: SHANDALAR 0x0058d3bc
char s_portrait_palette_map_empty_palette_path_0058d3bc[4] = "";
// GLOBAL: SHANDALAR 0x0058d3c0
char s_portrait_palette_map_empty_portrait_path_0058d3c0[4] = "";
// GLOBAL: SHANDALAR 0x00603ef8
RpBitsPalettePacket g_portrait_palette_source_palette;
// GLOBAL: SHANDALAR 0x00603bd0
RpBitsPalettePacket g_portrait_palette_target_palette;
// GLOBAL: SHANDALAR 0x00604218
unsigned char *g_portrait_palette_scan_cursor;
// GLOBAL: SHANDALAR 0x00604220
unsigned char g_portrait_palette_remap_table[0x100];
// GLOBAL: SHANDALAR 0x00604320
unsigned char g_portrait_tint_remap_table[0x100];
// GLOBAL: SHANDALAR 0x00587220
AdvMenuControl g_opening_menu_controls[4] = {
    {0xb8, 0x121, 0x115, 0x28, 0xb8, 0x121, 0x115, 0x28, 1, HandleOpeningMenuControlEvent, HandleOpeningMenuControlActivate, 1, 0, "Ss", (char *)0, 0, 0, {0, 0, 0, 0}},
    {0xb8, 0x14a, 0x115, 0x28, 0xb8, 0x14a, 0x115, 0x28, 1, HandleOpeningMenuControlEvent, HandleOpeningMenuControlActivate, 2, 0, "Ll", (char *)0, 0, 0, {0, 0, 0, 0}},
    {0xb8, 0x173, 0x115, 0x28, 0xb8, 0x173, 0x115, 0x28, 1, HandleOpeningMenuControlEvent, HandleOpeningMenuControlActivate, 3, 0, "rR", (char *)0, 0, 0, {0, 0, 0, 0}},
    {0xb8, 0x19c, 0x115, 0x28, 0xb8, 0x19c, 0x115, 0x28, 1, HandleOpeningMenuControlEvent, HandleOpeningMenuControlActivate, 5, 1, g_opening_menu_esc_hotkeys, g_opening_menu_esc_activate_hotkeys, 0, 0, {0, 0, 0, 0}}};

// GLOBAL: SHANDALAR 0x005873f0
AdvMenuControl g_difficulty_menu_controls[5] = {
    {0x118, 0x50, 0x10e, 0x54, 0x118, 0x50, 0x10e, 0x54, 1, HandleDifficultyMenuControlEvent, HandleDifficultyMenuControlActivate, 1, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x118, 0xb4, 0x10e, 0x54, 0x118, 0xb4, 0x10e, 0x54, 1, HandleDifficultyMenuControlEvent, HandleDifficultyMenuControlActivate, 2, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x118, 0x118, 0x10e, 0x54, 0x118, 0x118, 0x10e, 0x54, 1, HandleDifficultyMenuControlEvent, HandleDifficultyMenuControlActivate, 3, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x118, 0x17c, 0x10e, 0x54, 0x118, 0x17c, 0x10e, 0x54, 1, HandleDifficultyMenuControlEvent, HandleDifficultyMenuControlActivate, 4, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {-1, -1, -1, -1, -1, -1, -1, -1, 1, HandleEscControlEvent, ActivateSelectedMenuValue, 0, 0, g_difficulty_menu_esc_navigate_hotkeys, g_difficulty_menu_esc_activate_hotkeys, 0, 0, {0, 0, 0, 0}}};

// GLOBAL: SHANDALAR 0x005875d8
AdvMenuControl g_color_menu_controls[6] = {
    {0x33, 0x43, 0x10e, 0x4b, 0x33, 0x43, 0x10e, 0x4b, 1, HandleColorMenuControlEvent, HandleColorMenuControlActivate, 1, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x33, 0x8e, 0x10e, 0x4b, 0x33, 0x8e, 0x10e, 0x4b, 1, HandleColorMenuControlEvent, HandleColorMenuControlActivate, 2, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x33, 0xd9, 0x10e, 0x4b, 0x33, 0xd9, 0x10e, 0x4b, 1, HandleColorMenuControlEvent, HandleColorMenuControlActivate, 3, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x33, 0x124, 0x10e, 0x4b, 0x33, 0x124, 0x10e, 0x4b, 1, HandleColorMenuControlEvent, HandleColorMenuControlActivate, 4, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x33, 0x16f, 0x10e, 0x4b, 0x33, 0x16f, 0x10e, 0x4b, 1, HandleColorMenuControlEvent, HandleColorMenuControlActivate, 5, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {-1, -1, -1, -1, -1, -1, -1, -1, 1, HandleEscControlEvent, ActivateSelectedMenuValue, 0, 0, g_color_menu_esc_navigate_hotkeys, g_color_menu_esc_activate_hotkeys, 0, 0, {0, 0, 0, 0}}};

// GLOBAL: SHANDALAR 0x00587598
AdvMenuRect g_difficulty_menu_preview_rects[4] = {{0x1c7, 0x4b, 0x7a, 0x5c}, {0x1c7, 0xaf, 0x7a, 0x5c}, {0x1c7, 0x113, 0x7a, 0x5c}, {0x1c7, 0x177, 0x7a, 0x5c}};

// GLOBAL: SHANDALAR 0x00781710
EncodedImage *g_color_menu_normal_swatch_sprites[5];
// GLOBAL: SHANDALAR 0x007817d0
EncodedImage *g_color_menu_pressed_swatch_sprites[5];
// GLOBAL: SHANDALAR 0x005aa0c4
int g_difficulty_menu_strings_loaded;
// GLOBAL: SHANDALAR 0x005aa3f0
int g_color_menu_strings_loaded;
// GLOBAL: SHANDALAR 0x005aa420
int *g_difficulty_caption_table;
// GLOBAL: SHANDALAR 0x005aa428
int *g_color_menu_flavor_table;
// GLOBAL: SHANDALAR 0x005aa42c
int *g_color_menu_color_name_table;
// GLOBAL: SHANDALAR 0x005a9d88
int *g_difficulty_option_table;
// GLOBAL: SHANDALAR 0x005a9d98
int *g_color_caption_table;
// GLOBAL: SHANDALAR 0x00587370
int g_menu_selection_value = -1;

// GLOBAL: SHANDALAR 0x005877d0
AdvMenuRect g_color_menu_swatch_rects[5] = {{0x33, 0x43, 0x46, 0x4b}, {0x33, 0x8e, 0x46, 0x4b}, {0x33, 0xd9, 0x46, 0x4b}, {0x33, 0x124, 0x46, 0x4b}, {0x33, 0x16f, 0x46, 0x4b}};
// GLOBAL: SHANDALAR 0x00587820
int g_color_menu_selection_to_deck_color[5] = {4, 5, 1, 3, 2};

// GLOBAL: SHANDALAR 0x00588a18
int g_loadsave_insert_mode = 1;
// GLOBAL: SHANDALAR 0x005aa470
int g_loadsave_menu_selection;
// GLOBAL: SHANDALAR 0x005aa478
EncodedImage *g_loadsave_frame_sprites[9];
// GLOBAL: SHANDALAR 0x005aa4a0
int g_loadsave_slot_has_data[10];
// GLOBAL: SHANDALAR 0x005aa4c8
EncodedImage *g_loadsave_slot_button_state_sprites[2][3];
// GLOBAL: SHANDALAR 0x005aa4e0
int g_loadsave_cursor_index;
// GLOBAL: SHANDALAR 0x005aa4e4
int g_loadsave_edit_active;
// GLOBAL: SHANDALAR 0x0074ad20
#ifdef SHANDALAR
extern int g_loadsave_skip_esc;
#else
int g_loadsave_skip_esc;
#endif
// GLOBAL: SHANDALAR 0x0074ad30
char g_loadsave_slot_descriptions[10][0x40];
// GLOBAL: SHANDALAR 0x00588dbc
char g_loadsave_esc_key[] = "\x1b";
// GLOBAL: SHANDALAR 0x00588dc0
char g_loadsave_esc_key_2[] = "\x1b";

// GLOBAL: SHANDALAR 0x00588a20
AdvMenuControl g_loadsave_menu_controls[11] = {
    {0x2c, 0x55, 0x168, 0x1b, 0x2c, 0x55, 0x168, 0x1b, 1, HandleLoadSaveSlotControlEvent, ActivateLoadSaveSlotControl, 4, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x2c, 0x73, 0x168, 0x1b, 0x2c, 0x73, 0x168, 0x1b, 1, HandleLoadSaveSlotControlEvent, ActivateLoadSaveSlotControl, 5, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x2c, 0x91, 0x168, 0x1b, 0x2c, 0x91, 0x168, 0x1b, 1, HandleLoadSaveSlotControlEvent, ActivateLoadSaveSlotControl, 6, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x2c, 0xaf, 0x168, 0x1b, 0x2c, 0xaf, 0x168, 0x1b, 1, HandleLoadSaveSlotControlEvent, ActivateLoadSaveSlotControl, 7, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x2c, 0xcd, 0x168, 0x1b, 0x2c, 0xcd, 0x168, 0x1b, 1, HandleLoadSaveSlotControlEvent, ActivateLoadSaveSlotControl, 8, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x2c, 0xeb, 0x168, 0x1b, 0x2c, 0xeb, 0x168, 0x1b, 1, HandleLoadSaveSlotControlEvent, ActivateLoadSaveSlotControl, 9, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x2c, 0x109, 0x168, 0x1b, 0x2c, 0x109, 0x168, 0x1b, 1, HandleLoadSaveSlotControlEvent, ActivateLoadSaveSlotControl, 0xa, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x2c, 0x127, 0x168, 0x1b, 0x2c, 0x127, 0x168, 0x1b, 1, HandleLoadSaveSlotControlEvent, ActivateLoadSaveSlotControl, 0xb, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x2c, 0x145, 0x168, 0x1b, 0x2c, 0x145, 0x168, 0x1b, 1, HandleLoadSaveSlotControlEvent, ActivateLoadSaveSlotControl, 0xc, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0x2c, 0x163, 0x168, 0x1b, 0x2c, 0x163, 0x168, 0x1b, 1, HandleLoadSaveSlotControlEvent, ActivateLoadSaveSlotControl, 0xd, 0, (char *)0, (char *)0, 0, 0, {0, 0, 0, 0}},
    {0xb4, 0x1a0, 0x59, 0x23, 0xb4, 0x1a0, 0x59, 0x23, 1, RenderLoadSaveCancelButton, ActivateLoadSaveEscControl, 0xe, 0, g_loadsave_esc_key, g_loadsave_esc_key_2, 0, 0, {0, 0, 0, 0}}};

// FUNCTION: SHANDALAR 0x004a04a0
int SetSelectedMenuValue(AdvMenuControl *control, int event_type)
{
  // This param is real but unused
  (void)event_type;

  g_menu_selection_value = control->selection_value;
  return 0;
}

int ActivateSelectedMenuValue(AdvMenuControl *control)
{
  return SetSelectedMenuValue(control, 2);
}

// FUNCTION: SHANDALAR 0x004a04bd
int HandleEscControlEvent(AdvMenuControl *control, int event_type)
{
  if (event_type == 2)
  {
    SetSelectedMenuValue(control, event_type);
  }

  return 0;
}

// FUNCTION: SHANDALAR 0x004a04e9
int DrawOpeningMenuEntry(int entry_index, int visual_state)
{
  int pressed_offset_y;
  int pressed_offset_x;
  int unused_label_offset_index;
  int icon_sprite_index;
  int icon_y;
  int icon_x;
  int label_x_0;
  int label_x_1;
  int label_x_2;
  int label_x_3;
  EncodedImage **unused_button_sprite_group;
  int text_table_index;

  label_x_0 = 0x9c;
  label_x_1 = 0xbb;
  label_x_2 = 0xbb;
  label_x_3 = 0x40;

  g_page0_window_bounds->page_number = 1;
  BlitGraphicsRect(g_page2_window_bounds, g_opening_menu_entry_rects[entry_index].x, g_opening_menu_entry_rects[entry_index].y,
                   global_screen_width - g_opening_menu_entry_rects[entry_index].x, ScaleUiCoordinate(0x2d), g_page1_window_bounds,
                   g_opening_menu_entry_rects[entry_index].x, g_opening_menu_entry_rects[entry_index].y);

  switch (entry_index)
  {
  case 0:
    unused_button_sprite_group = &g_opening_menu_sprite_blob_handle;
    break;
  case 1:
    unused_button_sprite_group = g_opening_menu_unused_sprite_group_a;
    break;
  case 2:
    unused_button_sprite_group = g_opening_menu_unused_sprite_group_c;
    break;
  case 3:
    unused_button_sprite_group = g_opening_menu_unused_sprite_group_d;
    break;
  }

  if (3 <= entry_index)
  {
    text_table_index = entry_index + 1;
  }
  else
  {
    text_table_index = entry_index;
  }

  switch (visual_state)
  {
  case 0:
    unused_label_offset_index = 0;
    icon_sprite_index = 0;
    break;
  case 1:
    unused_label_offset_index = 1;
    icon_sprite_index = 1;
    break;
  case 2:
    unused_label_offset_index = 1;
    icon_sprite_index = 2;
    break;
  case 3:
    unused_label_offset_index = 2;
    icon_sprite_index = 3;
    break;
  }

  icon_x = g_opening_menu_entry_rects[entry_index].x + (g_opening_menu_entry_height - g_opening_menu_icon_size) / 2;
  icon_y = g_opening_menu_entry_rects[entry_index].y + (g_opening_menu_entry_height - g_opening_menu_icon_size) / 2;
  pressed_offset_x = ScaleUiCoordinate(2) / 2;
  pressed_offset_y = ScaleUiCoordinate(2) / 2;
  if (visual_state == 2)
  {
    DrawEncodedImageResampled(g_page0_window_bounds, (int)(*(volatile int *)&pressed_offset_x) + icon_x,
                              icon_y + pressed_offset_y, g_opening_menu_icon_size, g_opening_menu_icon_size,
                              g_opening_menu_icon_state_sprites[icon_sprite_index]);
  }
  else
  {
    DrawEncodedImageResampled(g_page0_window_bounds, icon_x, icon_y, g_opening_menu_icon_size, g_opening_menu_icon_size,
                              g_opening_menu_icon_state_sprites[icon_sprite_index]);
  }

  g_page0_window_bounds->font_slot = 6;
  DrawFormattedTextShadowed(g_page0_window_bounds, (&label_x_0)[icon_sprite_index], icon_x + ScaleUiCoordinate(0x24),
                            (icon_y + g_opening_menu_icon_size / 2) - (GetFontStyleSize(6) / 2),
                            "%s",
                            (char *)g_opening_menu_text_table[text_table_index]);

  g_page0_window_bounds->page_number = 0;
  BlitGraphicsRect(g_page1_window_bounds, g_opening_menu_entry_rects[entry_index].x, g_opening_menu_entry_rects[entry_index].y,
                   global_screen_width - g_opening_menu_entry_rects[entry_index].x, ScaleUiCoordinate(0x2d), g_page0_window_bounds,
                   g_opening_menu_entry_rects[entry_index].x, g_opening_menu_entry_rects[entry_index].y);
  (void)unused_button_sprite_group;
  return 0;
}

// FUNCTION: SHANDALAR 0x004a0834
int RunOpeningMenu(void)
{
  int sprite_entry_index;
  int sprite_group_index;
  int copied_entry_count;
  EncodedImage *sprite_entry_pointers[100];
  int menu_context;
#ifdef MODERN_FIXES
  char map_name[100];
#else
  char *map_name;
#endif
  int loop_index;

#ifdef MODERN_FIXES
  strcpy(map_name, "magic3.map");
#else
  map_name = "magic3.map";
#endif
  if (g_opening_menu_strings_loaded == 0)
  {
    g_opening_menu_text_table = LoadIniEscapedStringTable(g_advbuttons_ini_file, "openingMenu", g_ini_string_scratch);
    g_opening_menu_strings_loaded = 1;
  }

  SetFontStyleSize(6, ScaleUiCoordinate(0x14));

loop:
  strcpy(map_name, "magic3.map");
  g_menu_selection_value = -1;

  AnimatePaletteToColor(0, g_default_palette_fade_steps);
  LoadPcxResource(1, 0, 0, "menubak.pic",
                  (g_graphics_bpp == 8) ? &g_palette_data_words : (RpBitsPalettePacket *)1);

  StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x280, 0x1e0, g_page0_window_bounds, 0, 0, global_screen_width,
                          global_screen_height);
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x280, 0x1e0, g_page2_window_bounds, 0, 0, global_screen_width,
                          global_screen_height);

  FadeInPaletteFromGray(0, g_default_palette_fade_steps);
  g_page0_window_bounds->font_slot = 5;
  g_page0_window_bounds->font_slot = 1;

  g_opening_menu_entry_enabled[2] = FileExists(map_name);
  for (loop_index = 4; loop_index < 0xe; loop_index = loop_index + 1)
  {
    if (loop_index >= 10)
    {
      map_name[5] = (char)(loop_index + 'W');
    }
    else
    {
      map_name[5] = (char)(loop_index + '0');
    }

    if (FileExists(map_name) != 0)
    {
      break;
    }
  }

  if (loop_index == 0xe)
  {
    g_opening_menu_entry_enabled[1] = 0;
  }

  sprite_entry_index = 0;
  ReadSpriteEntryPointers(sprite_entry_pointers, "begin.spr");

  copied_entry_count = 0;
  for (sprite_group_index = 0; sprite_group_index < 2;
       copied_entry_count = copied_entry_count + 1, sprite_group_index = sprite_group_index + 1)
  {
    (&g_opening_menu_sprite_blob_handle)[sprite_group_index] = sprite_entry_pointers[sprite_entry_index];
    sprite_entry_index = sprite_entry_index + 1;
  }
  for (sprite_group_index = 0; sprite_group_index < 3;
       copied_entry_count = copied_entry_count + 1, sprite_group_index = sprite_group_index + 1)
  {
    g_opening_menu_unused_sprite_group_a[sprite_group_index] = sprite_entry_pointers[sprite_entry_index];
    sprite_entry_index = sprite_entry_index + 1;
  }
  for (sprite_group_index = 0; sprite_group_index < 3;
       copied_entry_count = copied_entry_count + 1, sprite_group_index = sprite_group_index + 1)
  {
    g_opening_menu_unused_sprite_group_c[sprite_group_index] = sprite_entry_pointers[sprite_entry_index];
    sprite_entry_index = sprite_entry_index + 1;
  }
  for (sprite_group_index = 0; sprite_group_index < 2;
       copied_entry_count = copied_entry_count + 1, sprite_group_index = sprite_group_index + 1)
  {
    g_opening_menu_unused_sprite_group_b[sprite_group_index] = sprite_entry_pointers[sprite_entry_index];
    sprite_entry_index = sprite_entry_index + 1;
  }
  for (sprite_group_index = 0; sprite_group_index < 2;
       copied_entry_count = copied_entry_count + 1, sprite_group_index = sprite_group_index + 1)
  {
    g_opening_menu_unused_sprite_group_d[sprite_group_index] = sprite_entry_pointers[sprite_entry_index];
    sprite_entry_index = sprite_entry_index + 1;
  }
  for (sprite_group_index = 0; sprite_group_index < 2;
       copied_entry_count = copied_entry_count + 1, sprite_group_index = sprite_group_index + 1)
  {
    g_opening_menu_unused_sprite_group_e[sprite_group_index] = sprite_entry_pointers[sprite_entry_index];
    sprite_entry_index = sprite_entry_index + 1;
  }
  for (sprite_group_index = 0; sprite_group_index < 4;
       copied_entry_count = copied_entry_count + 1, sprite_group_index = sprite_group_index + 1)
  {
    g_opening_menu_icon_state_sprites[sprite_group_index] = sprite_entry_pointers[sprite_entry_index];
    sprite_entry_index = sprite_entry_index + 1;
  }

  if (g_opening_menu_controls[0].x == g_opening_menu_controls[0].base_x)
  {
    for (loop_index = 0; loop_index < 4; loop_index = loop_index + 1)
    {
      g_opening_menu_entry_rects[loop_index].x = ScaleUiCoordinate(g_opening_menu_entry_rects[loop_index].x);
      g_opening_menu_entry_rects[loop_index].y = ScaleUiCoordinate(g_opening_menu_entry_rects[loop_index].y);
      g_opening_menu_entry_rects[loop_index].width = ScaleUiCoordinate(g_opening_menu_entry_rects[loop_index].width);
      g_opening_menu_entry_rects[loop_index].height = ScaleUiCoordinate(g_opening_menu_entry_rects[loop_index].height);
      g_opening_menu_controls[loop_index].x = ScaleUiCoordinate(g_opening_menu_controls[loop_index].x);
      g_opening_menu_controls[loop_index].y = ScaleUiCoordinate(g_opening_menu_controls[loop_index].y);
      g_opening_menu_controls[loop_index].width = ScaleUiCoordinate(g_opening_menu_controls[loop_index].width);
      g_opening_menu_controls[loop_index].height = ScaleUiCoordinate(g_opening_menu_controls[loop_index].height);
    }

    g_opening_menu_icon_size = ScaleUiCoordinate(g_opening_menu_icon_size);
    g_opening_menu_entry_height = ScaleUiCoordinate(g_opening_menu_entry_height);
    g_opening_menu_icon_label_spacing = ScaleUiCoordinate(g_opening_menu_icon_label_spacing);
  }

  menu_context = BeginMenuContext();
  ResetMenuContext(menu_context);
  AddMenuControlsToContext(g_opening_menu_controls, 4, menu_context);

  for (loop_index = 0; loop_index < 4; loop_index = loop_index + 1)
  {
    DrawOpeningMenuEntry(loop_index, ((g_opening_menu_entry_enabled[loop_index] == 0) ? 3 : 0));
    if (g_opening_menu_entry_enabled[loop_index] == 0)
    {
      RenderAdvMenuControlDisabled(&g_opening_menu_controls[loop_index]);
    }
  }

  while (g_menu_selection_value == -1)
  {
    PumpOpeningMenuWait();
    UpdateMenuControlSelection(g_mouse_x_snapshot, g_mouse_y_snapshot, g_mouse_button_down_mask);
  }

  EndMenuContext();
  ResetMenuContext(menu_context);
  FreeSpriteBlob((void *)g_opening_menu_sprite_blob_handle);
  g_page0_window_bounds->font_slot = 1;

  if (g_menu_selection_value == 4)
  {
    ShowHallBackgroundScreen();

    // TODO: can you accomplish this w/o goto?
    goto loop;
  }
  else
  {
    return g_menu_selection_value - 1;
  }
}

// FUNCTION: SHANDALAR 0x004a0eef
int HandleOpeningMenuControlEvent(AdvMenuControl *control, int event_type)
{
  int is_inside_control_bounds;

  if (g_menu_render_guard == 0)
  {
    if ((g_mouse_x < control->x) || (control->width + control->x < g_mouse_x))
    {
      is_inside_control_bounds = 0;
    }
    else if ((g_mouse_y < control->y) || (control->y + control->height < g_mouse_y))
    {
      is_inside_control_bounds = 0;
    }
    else
    {
      is_inside_control_bounds = 1;
    }

    if (is_inside_control_bounds == 0)
    {
      return 0;
    }
  }

  if (control->state == 3)
  {
    return 0;
  }

  DrawOpeningMenuEntry((int)(control - g_opening_menu_controls), event_type);
  if ((event_type == 2) && (control->on_activate != (AdvMenuActivateCallback)0))
  {
    control->on_activate(control);
  }
  return 1;
}

// FUNCTION: SHANDALAR 0x004a0fed
int HandleOpeningMenuControlActivate(AdvMenuControl *control)
{
  PlaySoundEffectOnChannel("x:sound\\button2.wav", 0xf, 100, 100, 0);
  g_menu_selection_value = control->selection_value;
  return 0;
}

// FUNCTION: SHANDALAR 0x004a101f
int DrawDifficultyMenuEntry(int difficulty_index, int visual_state)
{
  FacemakerWindowBounds *source_page;
  int preview_y;
  int preview_x;
  int preview_height;
  unsigned int preview_width;

  switch (visual_state)
  {
  case 0:
    source_page = g_page1_window_bounds;
    break;
  case 1:
    source_page = g_page2_window_bounds;
    break;
  case 2:
    source_page = g_page2_window_bounds;
  default:
    break;
  }
  preview_x = g_difficulty_menu_preview_rects[difficulty_index].x;
  preview_y = g_difficulty_menu_preview_rects[difficulty_index].y;
  preview_width = (unsigned int)g_difficulty_menu_preview_rects[difficulty_index].width;
  preview_height = g_difficulty_menu_preview_rects[difficulty_index].height;

  if (visual_state == 2)
  {
    BlitGraphicsRect(g_page1_window_bounds, 200, preview_y - ScaleUiCoordinate(0x43), preview_width, preview_height, g_page1_window_bounds, 0, 0);
    StretchBlitGraphicsRect(source_page, preview_x, preview_y, preview_width, preview_height, g_page1_window_bounds, 4, 4, preview_width - 4,
                            preview_height - 4);
    CopyGraphicsRect(g_page1_window_bounds, 0, 0, preview_width, preview_height, g_page0_window_bounds, preview_x, preview_y);
  }
  else
  {
    CopyGraphicsRect(source_page, preview_x, preview_y, preview_width, preview_height, g_page0_window_bounds, preview_x, preview_y);
  }
  return 0;
}

// FUNCTION: SHANDALAR 0x004a1194
int RunDifficultyMenu(void)
{
  int menu_context;
  int difficulty_index;

  if (g_difficulty_menu_strings_loaded == 0)
  {
    g_difficulty_caption_table = LoadIniEscapedStringTable(g_advbuttons_ini_file, "diffCaption", g_ini_string_scratch);
    g_difficulty_option_table = LoadIniEscapedStringTable(g_advbuttons_ini_file, "diffs", g_ini_string_scratch);
    g_difficulty_menu_strings_loaded = 1;
  }

  AnimatePaletteToColor(0, g_default_palette_fade_steps);
  LoadPcxResource(1, 0, 0, "menu2.pic",
                  (g_graphics_bpp == 8) ? &g_palette_data_words : (RpBitsPalettePacket *)1);
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x280, 0x1e0, g_page0_window_bounds, 0, 0, global_screen_width,
                          global_screen_height);

  g_page0_window_bounds->font_slot = 7;
  SetFontStyleSize(7, ScaleUiCoordinate(0x1b));
  DrawFormattedTextShadowedCentered(g_page0_window_bounds, 0x76, ScaleUiCoordinate(0x140), ScaleUiCoordinate(0x2d), "%s",
                                    (char *)g_difficulty_caption_table[0]);

  SetFontStyleSize(7, ScaleUiCoordinate(0x16));
  {
    int text_width;

    for (difficulty_index = 0; difficulty_index < 4; difficulty_index = difficulty_index + 1)
    {
      text_width = MeasureMultilineTextWidth(g_page0_window_bounds, (char *)g_difficulty_option_table[difficulty_index]);
      DrawFormattedTextShadowedCenterY(g_page0_window_bounds, 0x7b, ScaleUiCoordinate(0x1b0) - text_width,
                                       ScaleUiCoordinate(difficulty_index * 100 + 0x75),
                                       "%s", (char *)g_difficulty_option_table[difficulty_index]);
    }
  }

  StretchBlitGraphicsRect(g_page1_window_bounds, 0x1b1, 0x43, 0xa4, 0x19d, g_page1_window_bounds, 200, 0, ScaleUiCoordinate(0xa4),
                          ScaleUiCoordinate(0x19d));

  FadeInPaletteFromGray(0, g_default_palette_fade_steps);

  LoadPcxIntoPage(1, "menu2-norm.pic");
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, 0xa4, 0x19d, g_page1_window_bounds, ScaleUiCoordinate(0x1b1),
                          ScaleUiCoordinate(0x43), ScaleUiCoordinate(0xa4), ScaleUiCoordinate(0x19d));

  LoadPcxIntoPage(2, "menu2-hi.pic");
  StretchBlitGraphicsRect(g_page2_window_bounds, 0, 0, 0xa4, 0x19d, g_page2_window_bounds, ScaleUiCoordinate(0x1b1),
                          ScaleUiCoordinate(0x43), ScaleUiCoordinate(0xa4), ScaleUiCoordinate(0x19d));

  if (g_difficulty_menu_controls[0].x == g_difficulty_menu_controls[0].base_x)
  {
    for (difficulty_index = 0; difficulty_index < 4; difficulty_index = difficulty_index + 1)
    {
      g_difficulty_menu_preview_rects[difficulty_index].x = ScaleUiCoordinate(g_difficulty_menu_preview_rects[difficulty_index].x);
      g_difficulty_menu_preview_rects[difficulty_index].y = ScaleUiCoordinate(g_difficulty_menu_preview_rects[difficulty_index].y);
      g_difficulty_menu_preview_rects[difficulty_index].width = ScaleUiCoordinate(g_difficulty_menu_preview_rects[difficulty_index].width);
      g_difficulty_menu_preview_rects[difficulty_index].height = ScaleUiCoordinate(g_difficulty_menu_preview_rects[difficulty_index].height);
      g_difficulty_menu_controls[difficulty_index].x = ScaleUiCoordinate(g_difficulty_menu_controls[difficulty_index].x);
      g_difficulty_menu_controls[difficulty_index].y = ScaleUiCoordinate(g_difficulty_menu_controls[difficulty_index].y);
      g_difficulty_menu_controls[difficulty_index].width = ScaleUiCoordinate(g_difficulty_menu_controls[difficulty_index].width);
      g_difficulty_menu_controls[difficulty_index].height = ScaleUiCoordinate(g_difficulty_menu_controls[difficulty_index].height);
    }
  }

  menu_context = BeginMenuContext();
  ResetMenuContext(menu_context);
  AddMenuControlsToContext(g_difficulty_menu_controls, 5, menu_context);

  for (difficulty_index = 0; difficulty_index < 4; difficulty_index = difficulty_index + 1)
  {
    DrawDifficultyMenuEntry(difficulty_index, 0);
  }

  g_menu_selection_value = -1;
  while (g_menu_selection_value == -1)
  {
    PumpOpeningMenuWait();
    UpdateMenuControlSelection(g_mouse_x_snapshot, g_mouse_y_snapshot, g_mouse_button_down_mask);
  }

  EndMenuContext();
  ResetMenuContext(menu_context);
  return g_menu_selection_value - 1;
}

// FUNCTION: SHANDALAR 0x004a168a
int HandleDifficultyMenuControlEvent(AdvMenuControl *control, int event_type)
{
  int is_inside_control_bounds;

  if (g_menu_render_guard == 0)
  {
    if ((g_mouse_x < *(int *)((int)control + 0x10)) || (*(int *)((int)control + 0x18) + *(int *)((int)control + 0x10) < g_mouse_x))
    {
      is_inside_control_bounds = 0;
    }
    else if ((g_mouse_y < control->y) || (control->y + control->height < g_mouse_y))
    {
      is_inside_control_bounds = 0;
    }
    else
    {
      is_inside_control_bounds = 1;
    }

    if (is_inside_control_bounds == 0)
    {
      return 0;
    }
  }

  if (control->state == 3)
  {
    return 0;
  }

  DrawDifficultyMenuEntry(control->selection_value - 1, event_type);
  if ((event_type == 2) && (control->on_activate != (AdvMenuActivateCallback)0))
  {
    control->on_activate(control);
  }
  return 1;
}

// FUNCTION: SHANDALAR 0x004a177f
int HandleDifficultyMenuControlActivate(AdvMenuControl *control)
{
  PlaySoundEffectOnChannel("x:sound\\button2.wav", 0xf, 100, 100, 0);
  g_menu_selection_value = control->selection_value;
  return 0;
}

// FUNCTION: SHANDALAR 0x004a17b1
int DrawColorMenuEntry(int color_index, int visual_state)
{
  int swatch_y;
  unsigned int swatch_x;
  unsigned int swatch_height;
  unsigned int swatch_width;
  EncodedImage **swatch_sprite_table;

  switch (visual_state)
  {
  case 0:
    swatch_sprite_table = g_color_menu_normal_swatch_sprites;
    break;
  case 1:
    swatch_sprite_table = g_color_menu_pressed_swatch_sprites;
    break;
  case 2:
    swatch_sprite_table = g_color_menu_pressed_swatch_sprites;
    break;
  }

  swatch_x = (unsigned int)g_color_menu_swatch_rects[color_index].x;
  swatch_y = g_color_menu_swatch_rects[color_index].y;
  swatch_width = (unsigned int)g_color_menu_swatch_rects[color_index].width;
  swatch_height = (unsigned int)g_color_menu_swatch_rects[color_index].height;

  if (visual_state == 2)
  {
    DrawEncodedImageResampled(g_page2_window_bounds, swatch_x + 2, swatch_y + 2, swatch_width - 4, swatch_height - 4,
                              *(swatch_sprite_table + color_index));
    BlitGraphicsRect(g_page2_window_bounds, swatch_x, swatch_y, swatch_width, swatch_height, g_page0_window_bounds, swatch_x, swatch_y);
  }
  else
  {
    DrawEncodedImageResampled(g_page0_window_bounds, swatch_x, swatch_y, swatch_width, swatch_height, *(swatch_sprite_table + color_index));
  }
  return 0;
}

// FUNCTION: SHANDALAR 0x004a18f0
int RunColorMenu(void)
{
  int restore_rect_buffer[4];
  int temp_rect_buffer[4];
  AdvMenuRect restore_rect;
  int menu_context;
  int color_index;

  if (g_color_menu_strings_loaded == 0)
  {
    g_color_caption_table = LoadIniEscapedStringTable(g_advbuttons_ini_file, "colorCaption", g_ini_string_scratch);
    g_color_menu_color_name_table = LoadIniEscapedStringTable(g_advbuttons_ini_file, "colorMenuColors", g_ini_string_scratch);
    g_color_menu_flavor_table = LoadIniEscapedStringTable(g_advbuttons_ini_file, "colorMenuFlavor", g_ini_string_scratch);
    g_color_menu_strings_loaded = 1;
  }

  AnimatePaletteToColor(0, g_default_palette_fade_steps);
  if (g_graphics_bpp == 8)
  {
    ClearGraphicsPageWithPaletteColor(0, 0);
  }

  LoadPcxResource(1, 0, 0, "menu3.pic", (g_graphics_bpp == 8) ? &g_palette_data_words : (RpBitsPalettePacket *)1);
  ShandalarLoadPcxIntoPageNoPalette("menu3.pic");
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x280, 0x1e0, g_page2_window_bounds, 0, 0, global_screen_width, global_screen_height);
  BlitGraphicsRect(g_page2_window_bounds, 0, 0, global_screen_width, global_screen_height, g_page1_window_bounds, 0, 0);

  g_page1_window_bounds->font_slot = 7;
  SetFontStyleSize(7, ScaleUiCoordinate(0x1b));
  DrawFormattedTextShadowedCentered(g_page1_window_bounds, 0x9b, ScaleUiCoordinate(0x140), ScaleUiCoordinate(0x22), "%s",
                                    (char *)g_color_caption_table[0]);

  SetFontStyleSize(7, ScaleUiCoordinate(0x12));
  for (color_index = 0; color_index < 5; color_index = color_index + 1)
  {
    DrawFormattedTextShadowed(g_page1_window_bounds, 0xa3, ScaleUiCoordinate(0x7d), ScaleUiCoordinate(color_index * 0x4c + 0x4c), "%s",
                              (char *)g_color_menu_color_name_table[color_index]);
  }

  SetFontStyleSize(7, ScaleUiCoordinate(10));
  for (color_index = 0; color_index < 5; color_index = color_index + 1)
  {
    DrawFormattedTextShadowed(g_page1_window_bounds, 0x71, ScaleUiCoordinate(0x85), ScaleUiCoordinate(color_index * 0x4c + 100), "%s",
                              (char *)g_color_menu_flavor_table[color_index]);
  }

  AnimatePaletteToColor(0, 4);
  BlitGraphicsRect(g_page1_window_bounds, 0, 0, global_screen_width, global_screen_height, g_page0_window_bounds, 0, 0);
  LoadPcxResource(-1, 0, 0, "menu3.pic", &g_palette_data_words);
  FadeInPaletteFromGray(0, g_default_palette_fade_steps);

  restore_rect = *PushGraphicsClipRect((AdvMenuRect *)temp_rect_buffer, g_page2_window_bounds, 0, 0, global_screen_width, global_screen_height);

  BeginSpriteEncodeSession();
  LoadPcxIntoPageOpaque(1, "menu3-but1.pic");
  for (color_index = 0; color_index < 5; color_index = color_index + 1)
  {
    g_color_menu_normal_swatch_sprites[color_index] = EncodeSpriteFromPage(1, 0, color_index * 0x4b, 0x46, 0x4b);
  }

  LoadPcxIntoPageOpaque(1, "menu3but.pic");
  for (color_index = 0; color_index < 5; color_index = color_index + 1)
  {
    g_color_menu_pressed_swatch_sprites[color_index] = EncodeSpriteFromPage(1, 0, color_index * 0x4b, 0x46, 0x4b);
  }
  FinalizeSpriteEncodeSession();

  if (g_color_menu_controls[0].x == g_color_menu_controls[0].base_x)
  {
    for (color_index = 0; color_index < 5; color_index = color_index + 1)
    {
      g_color_menu_swatch_rects[color_index].x = ScaleUiCoordinate(g_color_menu_swatch_rects[color_index].x);
      g_color_menu_swatch_rects[color_index].y = ScaleUiCoordinate(g_color_menu_swatch_rects[color_index].y);
      g_color_menu_swatch_rects[color_index].width = ScaleUiCoordinate(g_color_menu_swatch_rects[color_index].width);
      g_color_menu_swatch_rects[color_index].height = ScaleUiCoordinate(g_color_menu_swatch_rects[color_index].height);
      g_color_menu_controls[color_index].x = ScaleUiCoordinate(g_color_menu_controls[color_index].x);
      g_color_menu_controls[color_index].y = ScaleUiCoordinate(g_color_menu_controls[color_index].y);
      g_color_menu_controls[color_index].width = ScaleUiCoordinate(g_color_menu_controls[color_index].width);
      g_color_menu_controls[color_index].height = ScaleUiCoordinate(g_color_menu_controls[color_index].height);
    }
  }

  menu_context = BeginMenuContext();
  ResetMenuContext(menu_context);
  AddMenuControlsToContext(g_color_menu_controls, 6, menu_context);
  for (color_index = 0; color_index < 5; color_index = color_index + 1)
  {
    DrawColorMenuEntry(color_index, 0);
  }

  g_menu_selection_value = -1;
  while (g_menu_selection_value == -1)
  {
    PumpOpeningMenuWait();
    UpdateMenuControlSelection(g_mouse_x_snapshot, g_mouse_y_snapshot, g_mouse_button_down_mask);
  }

  EndMenuContext();
  ResetMenuContext(menu_context);
  FreeSpriteBlob(g_color_menu_normal_swatch_sprites[0]);
  PushGraphicsClipRect((AdvMenuRect *)restore_rect_buffer, g_page2_window_bounds, restore_rect.x, restore_rect.y, restore_rect.width, restore_rect.height);

  if (g_menu_selection_value == 0)
  {
    return -1;
  }
  else
  {
    return g_color_menu_selection_to_deck_color[g_menu_selection_value - 1];
  }
}

// FUNCTION: SHANDALAR 0x004a1f2a
int HandleColorMenuControlEvent(AdvMenuControl *control, int event_type)
{
  int is_inside_control_bounds;

  if (g_menu_render_guard == 0)
  {
    if ((g_mouse_x < control->x) || (control->x + control->width < g_mouse_x))
    {
      is_inside_control_bounds = 0;
    }
    else if ((g_mouse_y < control->y) || (control->y + control->height < g_mouse_y))
    {
      is_inside_control_bounds = 0;
    }
    else
    {
      is_inside_control_bounds = 1;
    }

    if (is_inside_control_bounds == 0)
    {
      return 0;
    }
  }

  if (control->state == 3)
  {
    return 0;
  }

  DrawColorMenuEntry(control->selection_value - 1, event_type);
  if ((event_type == 2) && (control->on_activate != (AdvMenuActivateCallback)0))
  {
    control->on_activate(control);
  }
  return 1;
}

// FUNCTION: SHANDALAR 0x004a201f
int HandleColorMenuControlActivate(AdvMenuControl *control)
{
  PlaySoundEffectOnChannel("x:sound\\button2.wav", 0xf, 100, 100, 0);
  g_menu_selection_value = control->selection_value;
  return 0;
}

// FUNCTION: SHANDALAR 0x004a21ad
int RunFacemakerFlow(void)
{
  return shandalar_run_facemaker_flow();
}

// FUNCTION: SHANDALAR 0x004a288d
int RunNameEntryDialog(char *name_buffer)
{
  int dialog_top;
  int key_code;
  int text_width;

  dialog_top = 0x12c;
  LoadPcxIntoPageOpaque(1, "namepick.pic");
  g_page1_window_bounds->font_slot = g_page0_window_bounds->font_slot;
  ResetNameEntryDoneButtonSprites();
  BuildNameEntryDoneButtonSprites();
  LoadTextSectionLines("ADVstrings.txt", "STARTUP");
  DrawFormattedTextNoShadowCentered(g_page1_window_bounds, 0xed, 0x8b, 0x19, &g_text_lines[2][0]);
  DrawFormattedTextNoShadowCentered(g_page1_window_bounds, 0xb4, 0x8a, 0x18, &g_text_lines[2][0]);
  BlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x114, 0x6a, g_page1_window_bounds, 0, 200);
  DrawFormattedTextNoShadowCentered(g_page1_window_bounds, 0xb4, 0x8a, 0x100, "%s", name_buffer);
  BlitTransparentRuns(g_page1_window_bounds, 0, 200, 0x114, 0x6a, g_page0_window_bounds, (global_screen_width - 0x114) / 2, ScaleUiCoordinate(dialog_top));
  DrawNameEntryDoneButton();
  ClearInputAndWaitForMouseRelease();
  do
  {
    DelayUiTicks(1);
    DrawNameEntryDoneButton();
    if (IsNameEntryDoneButtonPressed())
    {
      break;
    }
    key_code = PopQueuedKeyInput();
    if (key_code == 0x1c0d)
    {
      break;
    }
    text_width = MeasureMultilineTextWidth(g_page1_window_bounds, name_buffer);
    if (key_code != 0)
    {
      if (key_code == 0x11b)
      {
        return -1;
      }
      ApplyNameEntryKey(name_buffer, (unsigned int)key_code, 0x19);
      BlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x114, 0x6a, g_page1_window_bounds, 0, 200);
      DrawFormattedTextNoShadowCentered(g_page1_window_bounds, 0xb4, 0x8a, 0x100, "%s", name_buffer);
      BlitTransparentRuns(g_page1_window_bounds, 0, 200, 0x114, 0x6a, g_page0_window_bounds, (global_screen_width - 0x114) / 2, ScaleUiCoordinate(dialog_top));
      DrawNameEntryDoneButton();
    }
    DrawBlinkingNameCaret(g_page0_window_bounds, 0xb4, ScaleUiCoordinate(0x140) - text_width / 2, ScaleUiCoordinate(dialog_top) + 0x30, name_buffer,
                          g_name_entry_cursor);
  } while (1);

  return 0;
}

// FUNCTION: SHANDALAR 0x004a2c10
void BuildFacemakerPortraitSprites(FacemakerWindowBounds *page)
{
  if (g_face_preview_sprite_group[0] != 0)
  {
    FreeSpriteBlob((void *)g_face_preview_sprite_group[0]);
  }
  ApplyPortraitPaletteMap(page, 0, 0, 0x89, 0xa9, "pedstls.pic", "advfac64.pic");
  ShandalarLoadPcxIntoPageNoPalette("advfac64.pic");
  BeginSpriteEncodeSession();
  g_face_preview_sprite_group[0] = EncodeSpriteFromPage(page->page_number, 0, 0, 0x89, 0xa9);
  ApplyPortraitTintMap(page, 0, 0, 0x89, 0xa9, 0, 1);
  g_face_preview_sprite_selected = EncodeSpriteFromPage(page->page_number, 0, 0, 0x89, 0xa9);
  FinalizeSpriteEncodeSession();
}

// FUNCTION: SHANDALAR 0x00521ffa
int ApplyPortraitPaletteMap(FacemakerWindowBounds *page, int src_x, int src_y, unsigned int width, int height, char *palette_source_path, char *portrait_path)
{
  unsigned int scanline_buffer[256];
  int y;
  int x;
  unsigned int green;
  unsigned int red;
  unsigned int blue;

  if ((strcmp(g_portrait_palette_map_cached_palette_path, palette_source_path) != 0) || (strcmp(g_portrait_palette_map_cached_portrait_path, portrait_path) != 0))
  {
    LoadPcxResource(-1, 0, 0, palette_source_path, &g_portrait_palette_source_palette);
    LoadPcxResource(-1, 0, 0, portrait_path, &g_portrait_palette_target_palette);
    g_portrait_palette_scan_cursor = g_portrait_palette_source_palette.entry_data;
    g_portrait_palette_map_cached_palette_path = palette_source_path;
    g_portrait_palette_map_cached_portrait_path = portrait_path;
    for (x = 0; x < 0x100; x = x + 1)
    {
      red = (unsigned int)*g_portrait_palette_scan_cursor;
      g_portrait_palette_scan_cursor = g_portrait_palette_scan_cursor + 1;
      green = (unsigned int)*g_portrait_palette_scan_cursor;
      g_portrait_palette_scan_cursor = g_portrait_palette_scan_cursor + 1;
      blue = (unsigned int)*g_portrait_palette_scan_cursor;
      g_portrait_palette_scan_cursor = g_portrait_palette_scan_cursor + 1;
      g_portrait_palette_remap_table[x] =
          (unsigned char)Palette_FindNearestEntryIndex((int)red, (int)green, (int)blue, g_portrait_palette_target_palette.entry_data);
    }
  }

  for (y = src_y; y < src_y + height; y = y + 1)
  {
    ReadGraphicsScanline(scanline_buffer, page->page_number, src_x, y, width);
    for (x = 0; x < (int)width; x = x + 1)
    {
      ((unsigned char *)scanline_buffer)[x] = g_portrait_palette_remap_table[((unsigned char *)scanline_buffer)[x]];
    }
    WriteGraphicsScanline(scanline_buffer, page->page_number, src_x, y, width);
  }

  return 0;
}

// FUNCTION: SHANDALAR 0x00521e80
int ApplyPortraitTintMap(FacemakerWindowBounds *page, int src_x, int src_y, unsigned int width, int height, unsigned int tint_mask, int tint_mode)
{
  unsigned int scanline_buffer[256];
  unsigned int tint_green;
  unsigned int tint_red;
  unsigned int tint_blue;
  int y;
  int x;
  int mixed_green;
  unsigned char *palette_entry;
  int mixed_red;
  int mixed_blue;

  palette_entry = g_palette_data_words.entry_data;
  if (tint_mode != 0)
  {
    tint_red = tint_mask & 0xff;
    tint_green = *((unsigned char *)&tint_mask + 1);
    tint_blue = (tint_mask & 0xff0000) >> 0x10;
    for (x = 0; x < 0x100; x = x + 1)
    {
      mixed_red = ((int)*palette_entry + (int)tint_red) / 2;
      palette_entry = palette_entry + 1;
      mixed_green = ((int)*palette_entry + (int)tint_green) / 2;
      palette_entry = palette_entry + 1;
      mixed_blue = ((int)*palette_entry + (int)tint_blue) / 2;
      palette_entry = palette_entry + 1;
      g_portrait_tint_remap_table[x] =
          (unsigned char)Palette_FindNearestEntryIndex(mixed_red, mixed_green, mixed_blue, g_palette_data_words.entry_data);
    }
  }

  for (y = src_y; y < src_y + height; y = y + 1)
  {
    ReadGraphicsScanline(scanline_buffer, page->page_number, src_x, y, width);
    for (x = 0; x < (int)width; x = x + 1)
    {
      ((unsigned char *)scanline_buffer)[x] = g_portrait_tint_remap_table[((unsigned char *)scanline_buffer)[x]];
    }
    WriteGraphicsScanline(scanline_buffer, page->page_number, src_x, y, width);
  }

  return 0;
}

// FUNCTION: SHANDALAR 0x004a2713
int BlitTransparentRuns(FacemakerWindowBounds *src, int src_x, int src_y, unsigned int width, int height, FacemakerWindowBounds *dst, int dst_x, int dst_y)
{
  int row;
  int x;
  unsigned int scanline_buffer[250];
  unsigned int *run_start;
  unsigned int run_length;

  for (row = 0; row < height; row = row + 1)
  {
    ReadGraphicsScanline(scanline_buffer, src->page_number, src_x, src_y + row, width);
    if ((unsigned char)scanline_buffer[0] != '\0')
    {
      run_start = scanline_buffer;
    }
    else
    {
      run_start = (unsigned int *)0;
    }

    run_length = 0;
    for (x = 0; x < (int)width; x = x + 1)
    {
      if (*((unsigned char *)scanline_buffer + x) == '\0')
      {
        if (run_length != 0)
        {
          WriteGraphicsScanline(run_start, dst->page_number,
                                (int)((char *)run_start - (char *)scanline_buffer) + dst_x,
                                dst_y + row, run_length);
          run_length = 0;
          run_start = (unsigned int *)((char *)scanline_buffer + x);
        }
        else
        {
          run_start = (unsigned int *)((char *)scanline_buffer + x);
        }
      }
      else
      {
        run_length = run_length + 1;
      }
    }

    if (run_length != 0)
    {
      WriteGraphicsScanline(run_start, dst->page_number,
                            (int)((char *)run_start - (char *)scanline_buffer) + dst_x,
                            dst_y + row, run_length);
    }
  }

  return 0;
}

// FUNCTION: SHANDALAR 0x004a2e33
int ApplyNameEntryKey(char *name_buffer, int key_code, int max_len)
{
  switch (key_code)
  {

  case 0x4b00:
    if (g_name_entry_cursor >= 0)
    {
      g_name_entry_cursor--;
    }
    break;

  case 0x4d00:
    if (max_len > g_name_entry_cursor)
    {
      if (strlen(name_buffer) == (size_t)g_name_entry_cursor)
      {
        strcat(name_buffer, " ");
      }
      g_name_entry_cursor++;
    }
    break;

  case 0x1c0d:
    return 1;
    break;

  case 0xf09:
    g_name_entry_cursor += 8;
    g_name_entry_cursor = MIN(g_name_entry_cursor, max_len);
    break;

  case 0xf0f:
    g_name_entry_cursor -= 8;
    g_name_entry_cursor = MAX(g_name_entry_cursor, 0);
    break;

  case 0xe08:
    if (g_name_entry_cursor == 0)
    {
      break;
    }

    g_name_entry_cursor--;
    strcpy(name_buffer + g_name_entry_cursor, name_buffer + g_name_entry_cursor + 1);
    break;

  case 0x5300:
    if (g_name_entry_cursor < (int)strlen(name_buffer))
    {
      strcpy(name_buffer + g_name_entry_cursor, name_buffer + g_name_entry_cursor + 1);
    }
    break;

  case 0x5200:
    g_name_entry_insert_mode = g_name_entry_insert_mode ^ 1;
    break;

  case 0x4700:
    g_name_entry_cursor = 0;
    break;

  case 0x4f00:
    g_name_entry_cursor = (int)strlen(name_buffer);
    break;

  default:

    if ((int)strlen(name_buffer) < max_len)
    {
      key_code = key_code & 0xff;
      if (((key_code >= 'A' && key_code <= 'Z') ||
           (key_code >= 'a' && key_code <= 'z') ||
           (key_code >= '0' && key_code <= '9') ||
           (key_code == ' ')))
      {
        if (g_name_entry_insert_mode != 0)
        {
          memmove(name_buffer + g_name_entry_cursor + 1, name_buffer + g_name_entry_cursor, (max_len - g_name_entry_cursor) - 1);
        }
        name_buffer[g_name_entry_cursor++] = (char)key_code;
      }
    }
  }

  return 0;
}

// FUNCTION: SHANDALAR 0x004a8470
int HandleLoadSaveEscControlEvent(AdvMenuControl *control, int event_type)
{
  (void)control;
  if (event_type == 2)
  {
    PlaySoundEffectOnChannel("x:sound\\button2.wav", 0xf, 100, 100, 0);
    g_loadsave_menu_selection = 0xe;
  }
  return 0;
}

// FUNCTION: SHANDALAR 0x004a84ab
int ActivateLoadSaveEscControl(AdvMenuControl *control)
{
  (void)control;
  g_loadsave_menu_selection = 0xe;
  return 0;
}

int RenderLoadSaveCancelButton(AdvMenuControl *control, int mode)
{
  int mouse_inside;

  if (mode == 3)
  {
    return 0;
  }

  if (g_menu_render_guard == 0)
  {
    mouse_inside = (g_mouse_x >= control->x && g_mouse_x <= control->x + control->width &&
                    g_mouse_y >= control->y && g_mouse_y <= control->y + control->height);
    if (mouse_inside == 0)
    {
      return 0;
    }
  }

  if (control->state == 3)
  {
    return 0;
  }

  DrawEncodedImageResampled(g_page0_window_bounds, control->x, control->y, control->width, control->height,
                            g_loadsave_cancel_button_sprites[mode]);
  if ((mode == 2) && (control->on_activate != (AdvMenuActivateCallback)0))
  {
    control->on_activate(control);
  }
  return 1;
}

// FUNCTION: SHANDALAR 0x004a84c7
int DrawBlinkingNameCaret(FacemakerWindowBounds *window, int color_index, int caret_x, int caret_y, char *name_buffer, int cursor_pos)
{
  int i;
  int caret_width;
  int line_height;

  if ((GetUiTickCount() & 7) == 0)
  {
    return 0;
  }

  if (cursor_pos < 0)
    cursor_pos = 0;
  if (cursor_pos > 0x19)
    cursor_pos = 0x19;
  for (i = 0; i < cursor_pos; i = i + 1)
  {
    caret_x = caret_x + 8;
    if (caret_x > window->max_x + 0x80)
      break;
  }

  caret_width = 8;

  line_height = 12;
  caret_y = caret_y + line_height / 2;
  caret_y = caret_y - 1;
  if (caret_x > window->max_x || caret_x + caret_width < window->clip_left ||
      caret_y > window->max_y || caret_y + 1 < window->clip_top)
    return 0;
  if (caret_x < window->clip_left)
    caret_x = window->clip_left;
  if (caret_x + caret_width > window->max_x)
    caret_width = window->max_x - caret_x;
  DrawGraphicsLine(window, caret_x, caret_y, caret_x + caret_width, caret_y, color_index);
  DrawGraphicsLine(window, caret_x, caret_y + 1, caret_x + caret_width, caret_y + 1, color_index);
  return 0;
}

// FUNCTION: SHANDALAR 0x004a85fc
int DrawLoadSaveButton(FacemakerWindowBounds *window, EncodedImage **button_sprites, int button_width, int button_height, char *button_text, int font_slot,
                       int visual_state)
{
  size_t text_len;
  int text_width;
  int text_y;
  int x_cursor;
  int line_height;
  int text_color;
  EncodedImage *sprite;

  x_cursor = 0;
  button_width = ScaleUiCoordinate(button_width);
  button_height = ScaleUiCoordinate(button_height);

  sprite = button_sprites[0];
  DrawEncodedImageResampled(window, x_cursor, 0, ScaleUiCoordinate(sprite->width), ScaleUiCoordinate(sprite->height), sprite);
  x_cursor += ScaleUiCoordinate(sprite->width);

  sprite = button_sprites[1];
  while (x_cursor < button_width)
  {
    DrawEncodedImageResampled(window, x_cursor, 0, ScaleUiCoordinate(sprite->width), ScaleUiCoordinate(sprite->height), sprite);
    x_cursor += ScaleUiCoordinate(sprite->width);
  }

  sprite = button_sprites[2];
  x_cursor = button_width - ScaleUiCoordinate(sprite->width);
  DrawEncodedImageResampled(window, x_cursor, 0, ScaleUiCoordinate(sprite->width), ScaleUiCoordinate(sprite->height), sprite);

  window->font_slot = font_slot;
  line_height = GetFontLineHeight(font_slot);
  text_width = MeasureMultilineTextWidth(window, button_text);
  while (text_width > button_width)
  {
    text_len = strlen(button_text);
    button_text[text_len - 1] = '\0';
    text_width = MeasureMultilineTextWidth(window, button_text);
  }

  x_cursor = 10;
  text_y = button_height / 2;
  text_color = 0x25;
  if (visual_state == 1)
  {
    text_color = 0x67;
  }
  if (visual_state == 2)
  {
    text_color = 0x67;
    text_y += 2;
    x_cursor -= 2;
  }
  if (visual_state == 3)
  {
    text_color = 0xf0;
  }

  DrawLoadSaveButtonText(window, text_color, x_cursor, text_y, button_text);
  if (g_loadsave_edit_active != 0)
  {
    DrawBlinkingNameCaret(window, text_color, x_cursor, text_y, button_text, g_loadsave_cursor_index);
  }
  return 0;
}

// FUNCTION: SHANDALAR 0x004a884f
int DrawLoadSaveSlotEntry(int slot_index, int visual_state)
{
  int slot_y;
  int slot_x;
  int slot_h;
  int slot_w;
  EncodedImage **slot_state_sprites;

  switch (visual_state)
  {
  case 0:
    slot_state_sprites = &g_loadsave_slot_button_state_sprites[0][0];
    break;
  case 1:
    slot_state_sprites = &g_loadsave_slot_button_state_sprites[0][0];
    break;
  case 2:
    slot_state_sprites = &g_loadsave_slot_button_state_sprites[1][0];
    break;
  case 3:
    slot_state_sprites = &g_loadsave_slot_button_state_sprites[0][0];
    break;
  }

  slot_x = g_loadsave_menu_controls[slot_index].x;
  slot_y = g_loadsave_menu_controls[slot_index].y;
  slot_w = g_loadsave_menu_controls[slot_index].width;
  slot_h = g_loadsave_menu_controls[slot_index].height;

  DrawLoadSaveButton(g_page1_window_bounds, slot_state_sprites, 0x168, 0x1b, g_loadsave_slot_descriptions[slot_index], 4, visual_state);
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, ScaleUiCoordinate(0x168), ScaleUiCoordinate(0x1a), g_page0_window_bounds,
                          slot_x, slot_y, slot_w, slot_h);
  return 0;
}

// FUNCTION: SHANDALAR 0x004a898c
int DrawLoadSaveFrame(FacemakerWindowBounds *window, int x, int y, int width, int height)
{
  typedef struct
  {
    int total_size;
    short width;
    short height;
    short left_clip;
    short top_clip;
    short first_row;
    short row_count;
  } EncodedImageHeader;
  int cursor_y;
  int i;
  int right_span_x;
  EncodedImageHeader sprite_headers[9];
  int cursor_x;
  EncodedImage **sprite_table;

  sprite_table = g_loadsave_frame_sprites;
  for (i = 0; i < 9; i = i + 1)
  {
    sprite_headers[i] = *(EncodedImageHeader *)sprite_table[i];
    sprite_headers[i].width = (short)ScaleUiCoordinate((int)sprite_headers[i].width);
    sprite_headers[i].height = (short)ScaleUiCoordinate((int)sprite_headers[i].height);
  }

  x = ScaleUiCoordinate(x);
  y = ScaleUiCoordinate(y);
  width = ScaleUiCoordinate(width);
  height = ScaleUiCoordinate(height);

  DrawEncodedImageResampled(window, (x + width / 2) - (int)sprite_headers[0].width / 2,
                            y - ((int)sprite_headers[0].height - (int)sprite_headers[5].height),
                            (int)sprite_headers[0].width, (int)sprite_headers[0].height, sprite_table[0]);

  cursor_x = ((x + width / 2) - (int)sprite_headers[0].width / 2) - (int)sprite_headers[5].width;
  right_span_x = ((int)sprite_headers[0].width / 2) + (width / 2) + x;
  while (x < cursor_x)
  {
    DrawEncodedImageResampled(window, cursor_x, y, (int)sprite_headers[5].width, (int)sprite_headers[5].height, sprite_table[5]);
    DrawEncodedImageResampled(window, right_span_x, y, (int)sprite_headers[5].width, (int)sprite_headers[5].height, sprite_table[5]);
    cursor_x -= (int)sprite_headers[5].width;
    right_span_x += (int)sprite_headers[5].width;
  }

  DrawEncodedImageResampled(window, x, y, (int)sprite_headers[1].width, (int)sprite_headers[1].height, sprite_table[1]);
  DrawEncodedImageResampled(window, (width + x) - (int)sprite_headers[2].width, y, (int)sprite_headers[2].width,
                            (int)sprite_headers[2].height, sprite_table[2]);

  cursor_y = (int)sprite_headers[1].height + y;
  right_span_x = (width + x) - (int)sprite_headers[8].width;
  while (cursor_y < (height + y) - (int)sprite_headers[3].height)
  {
    DrawEncodedImageResampled(window, x, cursor_y, (int)sprite_headers[8].width, (int)sprite_headers[8].height, sprite_table[8]);
    DrawEncodedImageResampled(window, right_span_x, cursor_y,
                              (int)sprite_headers[6].width, (int)sprite_headers[6].height, sprite_table[6]);
    cursor_y += (int)sprite_headers[8].height;
  }

  DrawEncodedImageResampled(window, x, (height + y) - (int)sprite_headers[3].height, (int)sprite_headers[3].width,
                            (int)sprite_headers[3].height, sprite_table[3]);
  DrawEncodedImageResampled(window, (width + x) - (int)sprite_headers[4].width, (height + y) - (int)sprite_headers[4].height,
                            (int)sprite_headers[4].width, (int)sprite_headers[4].height, sprite_table[4]);

  cursor_x = (int)sprite_headers[3].width + x;
  right_span_x = ((width + x) - (int)sprite_headers[4].width) - (int)sprite_headers[7].width;
  y = y + (height - (int)sprite_headers[7].height);
  while (cursor_x < x + width / 2)
  {
    DrawEncodedImageResampled(window, cursor_x, y, (int)sprite_headers[7].width, (int)sprite_headers[7].height, sprite_table[7]);
    DrawEncodedImageResampled(window, right_span_x, y, (int)sprite_headers[7].width, (int)sprite_headers[7].height, sprite_table[7]);
    cursor_x += (int)sprite_headers[7].width;
    right_span_x -= (int)sprite_headers[7].width;
  }

  return 0;
}

// FUNCTION: SHANDALAR 0x004a8da8
int RunLoadSaveMenu(int save_mode)
{
  int menu_control_count;
  size_t text_index;
  int key_code;
  char saved_slot_name[0xff];
  char edit_buffer[0x101];
  int selected_slot;
  int frame_y;
  int frame_x;
  int frame_height;
  int frame_width;
  int menu_context;
  int row;
  char map_path[16];
  int slot_index;
  FILE *save_desc_file;

  strcpy(map_path, "magic4.map");
  AnimatePaletteToColor(0, g_default_palette_fade_steps);
  LoadPcxResource(1, 0, 0, "menopt.pic", (g_graphics_bpp == 8) ? &g_palette_data_words : (void *)1);
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x280, 0x1e0, g_page0_window_bounds, 0, 0, global_screen_width, global_screen_height);
  FadeInPaletteFromGray(0, g_default_palette_fade_steps);
  LoadPcxIntoPage(1, "optbox.pic");
  BeginSpriteEncodeSession();
  g_loadsave_frame_sprites[0] = EncodeSpriteFromPage(1, 1, 1, 0x40, 0x19);
  g_loadsave_frame_sprites[1] = EncodeSpriteFromPage(1, 0x42, 1, 0xe, 0xe);
  g_loadsave_frame_sprites[2] = EncodeSpriteFromPage(1, 0x83, 1, 0xe, 0xe);
  g_loadsave_frame_sprites[3] = EncodeSpriteFromPage(1, 1, 0x1d, 0xe, 0xe);
  g_loadsave_frame_sprites[4] = EncodeSpriteFromPage(1, 0x42, 0x1d, 0xe, 0xe);
  g_loadsave_frame_sprites[5] = EncodeSpriteFromPage(1, 0x83, 0x1d, 8, 0xb);
  g_loadsave_frame_sprites[6] = EncodeSpriteFromPage(1, 1, 0x39, 0xb, 8);
  g_loadsave_frame_sprites[7] = EncodeSpriteFromPage(1, 0x42, 0x39, 8, 0xb);
  g_loadsave_frame_sprites[8] = EncodeSpriteFromPage(1, 0x83, 0x39, 0xb, 8);
  for (row = 0; row < 2; row = row + 1)
  {
    for (slot_index = 0; slot_index < 3; slot_index = slot_index + 1)
    {
      g_loadsave_slot_button_state_sprites[row][slot_index] =
          EncodeSpriteFromPage(1, slot_index * 0x41 + 1, row * 0x1c + 0x55, 0x1e, 0x1b);
    }
  }
  FinalizeSpriteEncodeSession();

  if ((save_mode != 0) || (g_loadsave_skip_esc != 0))
  {
    BuildLoadSaveCancelButtonSprites();
  }

  frame_x = 0x1c;
  frame_y = 0x45;
  frame_width = 0x1a4 - frame_x;
  frame_height = 0x18e - frame_y;
  DrawLoadSaveFrame(g_page0_window_bounds, frame_x, frame_y, frame_width, frame_height);

  if (save_mode != 0)
  {
    strcpy(g_ui_message_buffer, gs_loadsave_0077d1b0[0]);
  }
  else
  {
    strcpy(g_ui_message_buffer, gs_loadsave_0077d1b0[1]);
  }

  map_path[5] = '4';
  save_desc_file = fopen("saveDescs", save_mode != 0 ? "r+t" : "rt");
  if (!save_desc_file && save_mode != 0)
  {
    save_desc_file = fopen("saveDescs", "w+t");
  }
  for (slot_index = 0; slot_index < 10; slot_index = slot_index + 1)
  {
    if (save_desc_file && fgets(g_loadsave_slot_descriptions[slot_index], 0x40, save_desc_file))
    {
      text_index = strlen(g_loadsave_slot_descriptions[slot_index]);
      if (text_index != 0 && (g_loadsave_slot_descriptions[slot_index][text_index - 1] == '\n' ||
                              g_loadsave_slot_descriptions[slot_index][text_index - 1] == '\r'))
      {
        g_loadsave_slot_descriptions[slot_index][text_index - 1] = '\0';
      }
    }
    else
    {
      g_loadsave_slot_descriptions[slot_index][0] = '\0';
    }
    map_path[5] = (char)int_to_hex_digit(slot_index + 4);
    g_loadsave_slot_has_data[slot_index] = FileExists(map_path);
    if (g_loadsave_slot_has_data[slot_index] == 0)
    {
      strcpy(g_loadsave_slot_descriptions[slot_index], gs_loadsave_0077d1b0[2]);
    }
    if (save_mode != 0)
    {
      g_loadsave_slot_has_data[slot_index] = 1;
    }
  }

  if (g_loadsave_menu_controls[0].base_x == g_loadsave_menu_controls[0].x)
  {
    for (slot_index = 0; slot_index < 11; slot_index = slot_index + 1)
    {
      g_loadsave_menu_controls[slot_index].x = ScaleUiCoordinate(g_loadsave_menu_controls[slot_index].x);
      g_loadsave_menu_controls[slot_index].y = ScaleUiCoordinate(g_loadsave_menu_controls[slot_index].y);
      g_loadsave_menu_controls[slot_index].width = ScaleUiCoordinate(g_loadsave_menu_controls[slot_index].width);
      g_loadsave_menu_controls[slot_index].height = ScaleUiCoordinate(g_loadsave_menu_controls[slot_index].height);
    }
  }

restart_menu_loop:
  menu_context = BeginMenuContext();
  ResetMenuContext(menu_context);
  if ((save_mode != 0) || (g_loadsave_skip_esc != 0))
  {
    menu_control_count = 0xb;
  }
  else
  {
    menu_control_count = 10;
  }
  AddMenuControlsToContext(g_loadsave_menu_controls, menu_control_count, menu_context);
  for (slot_index = 0; slot_index < 10; slot_index = slot_index + 1)
  {
    if (g_loadsave_slot_has_data[slot_index] == 0)
    {
      RenderAdvMenuControlDisabled(&g_loadsave_menu_controls[slot_index]);
      DrawLoadSaveSlotEntry(slot_index, 3);
    }
    else
    {
      RenderAdvMenuControlNormally(&g_loadsave_menu_controls[slot_index]);
      DrawLoadSaveSlotEntry(slot_index, 0);
    }
  }

  if ((save_mode != 0) || (g_loadsave_skip_esc != 0))
  {
    RenderAdvMenuControlNormally(&g_loadsave_menu_controls[10]);
  }

  g_loadsave_menu_selection = -1;
  while (g_loadsave_menu_selection == -1)
  {
    PumpOpeningMenuWait();
    UpdateMenuControlSelection(g_mouse_x_snapshot, g_mouse_y_snapshot, g_mouse_button_down_mask);
  }
  EndMenuContext();
  ResetMenuContext(menu_context);
  if (g_loadsave_menu_selection == 0xe)
  {
    AnimatePaletteToColor(0, g_default_palette_fade_steps);
    return -1;
  }
  if (save_mode != 0)
  {

    selected_slot = g_loadsave_menu_selection - 4;
    strcpy(saved_slot_name, g_loadsave_slot_descriptions[selected_slot]);
    g_loadsave_edit_active = 1;
    memset(edit_buffer + 1, 0, 0x100);
    strcpy(edit_buffer + 1, g_loadsave_slot_descriptions[selected_slot]);
    if (strcmp(edit_buffer + 1, gs_loadsave_0077d1b0[2]) == 0)
    {
      edit_buffer[1] = '\0';
    }
    g_loadsave_cursor_index = strlen(edit_buffer + 1);
    DrawLoadSaveSlotEntry(selected_slot, 2);

    while ((key_code = PopQueuedKeyInput()) != 0x1c0d)
    {
      if (key_code == 0)
      {
        PumpOpeningMenuWait();
        continue;
      }

      switch (key_code)
      {
      case 0x4b00:
        if (g_loadsave_cursor_index >= 0)
        {
          g_loadsave_cursor_index = g_loadsave_cursor_index - 1;
        }
        break;

      case 0x4d00:
        if (strlen(edit_buffer + 1) == g_loadsave_cursor_index)
        {
          strcat(edit_buffer + 1, " ");
        }
        g_loadsave_cursor_index++;
        break;

      case 0xf09:
        g_loadsave_cursor_index += 8;
        break;

      case 0xf0f:
        g_loadsave_cursor_index -= 8;
        g_loadsave_cursor_index = MAX(g_loadsave_cursor_index, 0);
        break;

      case 0xe08:
        if (g_loadsave_cursor_index == 0)
        {
          break;
        }

        g_loadsave_cursor_index--;
        strcpy(edit_buffer + g_loadsave_cursor_index + 1, edit_buffer + g_loadsave_cursor_index + 2);
        break;

      case 0x5300:
        if (g_loadsave_cursor_index < (int)strlen(edit_buffer + 1))
        {
          strcpy(edit_buffer + g_loadsave_cursor_index + 1, edit_buffer + g_loadsave_cursor_index + 2);
        }
        break;

      case 0x5200:
        g_loadsave_insert_mode = g_loadsave_insert_mode ^ 1;
        break;

      case 0x4700:
        g_loadsave_cursor_index = 0;
        break;

      case 0x4f00:
        g_loadsave_cursor_index = strlen(edit_buffer + 1);
        break;

      case 0x11b:
        g_loadsave_edit_active = 0;
        strcpy(g_loadsave_slot_descriptions[selected_slot], saved_slot_name);

      case 0x1c0d:
        goto restart_menu_loop;

      default:
        key_code = key_code & 0xff;
        if ((((key_code >= 0x41) && (key_code <= 0x5a)) || ((key_code >= 0x61) && (key_code <= 0x7a))) ||
            (((key_code >= 0x30) && (key_code <= 0x39)) || (key_code == 0x20)))
        {
          if (g_loadsave_insert_mode != 0)
          {
            memmove(edit_buffer + g_loadsave_cursor_index + 2, edit_buffer + g_loadsave_cursor_index + 1,
                    (0x100 - g_loadsave_cursor_index) - 1);
          }
          edit_buffer[g_loadsave_cursor_index + 1] = (char)key_code;
          g_loadsave_cursor_index = g_loadsave_cursor_index + 1;
        }
      }

      strcpy(g_loadsave_slot_descriptions[selected_slot], edit_buffer + 1);
      DrawLoadSaveSlotEntry(selected_slot, 2);
    }

    g_loadsave_edit_active = 0;
    if (save_desc_file)
    {
      fseek(save_desc_file, 0, 0);
      for (slot_index = 0; slot_index < 10; slot_index = slot_index + 1)
      {
        fprintf(save_desc_file, "%s\n", g_loadsave_slot_descriptions[slot_index]);
      }
    }
  }

  if (save_desc_file)
  {
    fclose(save_desc_file);
  }
  FreeSpriteBlob(g_loadsave_frame_sprites[0]);
  if (g_loadsave_cancel_button_sprites[0] != 0)
  {
    FreeSpriteBlob((void *)g_loadsave_cancel_button_sprites[0]);
    g_loadsave_cancel_button_sprites[0] = 0;
    g_loadsave_cancel_button_sprites[1] = 0;
    g_loadsave_cancel_button_sprites[2] = 0;
  }
  AnimatePaletteToColor(0, g_default_palette_fade_steps);
  return g_loadsave_menu_selection;
}

// FUNCTION: SHANDALAR 0x004a97e7
int HandleLoadSaveSlotControlEvent(AdvMenuControl *control, int event_type)
{
  int is_inside_control_bounds;

  if (g_menu_render_guard == 0)
  {
    if ((g_mouse_x < control->x) || (control->x + control->width < g_mouse_x))
    {
      is_inside_control_bounds = 0;
    }
    else if ((g_mouse_y < control->y) || (control->y + control->height < g_mouse_y))
    {
      is_inside_control_bounds = 0;
    }
    else
    {
      is_inside_control_bounds = 1;
    }

    if (is_inside_control_bounds == 0)
    {
      return 0;
    }
  }

  if (control->state == 3)
  {
    return 0;
  }

  DrawLoadSaveSlotEntry(control->selection_value - 4, event_type);
  if ((event_type == 2) && (control->on_activate != (AdvMenuActivateCallback)0))
  {
    control->on_activate(control);
  }
  return 1;
}

// FUNCTION: SHANDALAR 0x004a98de
int ActivateLoadSaveSlotControl(AdvMenuControl *control)
{
  PlaySoundEffectOnChannel("x:sound\\button2.wav", 0xf, 100, 100, 0);
  g_loadsave_menu_selection = control->selection_value;
  return 0;
}

// FUNCTION: SHANDALAR 0x0056c9b0
int ShowHallBackgroundScreen(void)
{
  LoadPcxIntoPageOpaque(1, "hallback.pic");
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x280, 0x1e0, g_page0_window_bounds, 0, 0, global_screen_width,
                          global_screen_height);
  WaitForInputEventUnlessBlocked();
  return 0;
}
