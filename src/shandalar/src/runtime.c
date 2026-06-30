#include "platform/gdi_compat.h"
#include "platform/legacy_io.h"
#include "drawcardlib/src/pic.h"
#include "facemaker/src/facemaker_types.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "shandalar.h"

#include <SDL3/SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

extern HDC global_main_hdc;
extern int global_screen_width;
extern int global_screen_height;
extern int g_graphics_bpp;
extern DIBSurface *g_graphics_pages[10];
extern PALETTEENTRY g_palette_entries[256];
extern RpBitsPalettePacket g_palette_data_words;
extern FontSlot g_font_slots[0x10];
extern int g_key_input_queue_count;
extern unsigned int g_key_input_queue[50];

DIBSurface *InitializeGraphicsSystem(int width, int height, int bits_per_pixel);
DIBSurface *CreateGraphicsPage(int page_number, int width, int height, int bits_per_pixel);
void SetGraphicsPage(int page_number, void *page);
void ShutdownGraphicsSystem(void);
void PresentGraphicsPage(int page_number);
void BlitGraphicsRect(FacemakerWindowBounds *dst, unsigned int dst_x, int dst_y,
                      unsigned int width, DWORD height, FacemakerWindowBounds *src,
                      int src_x, int src_y);
void StretchBlitGraphicsRect(FacemakerWindowBounds *dst, int dst_x, int dst_y,
                             int src_w, int src_h, FacemakerWindowBounds *src,
                             int src_x, int src_y, int copy_w, int copy_h);
void LoadPcxIntoPage(int page_number, char *path);
void LoadPcxIntoPageOpaque(int page_number, char *path);
void LoadPcxResource(int page_number, int x, int y, char *path, void *opaque);
void RpBits_ApplyPalette(RpBitsPalettePacket *palette_data);
void FillGraphicsRect(FacemakerWindowBounds *window_bounds, int x, int y,
                      int width, int height, unsigned int color_index);
void DrawEncodedImageResampled(FacemakerWindowBounds *dst, int x, int y,
                               int width, int height, EncodedImage *encoded_image);
int LoadSystemFont(int font_id, unsigned int point_size, char *font_file,
                   char *font_name, int weight, int italic);
int DrawTextFormatted(FacemakerWindowBounds *dst, int text_color, int draw_shadow,
                      int scale_to_screen, int center_x, int center_y, int x, int y,
                      int *format_and_args);
int DrawTextFormattedVa(FacemakerWindowBounds *dst, int text_color, int draw_shadow,
                        int scale_to_screen, int center_x, int center_y,
                        int x, int y, char *format, va_list args);
int DrawTextLine(FacemakerWindowBounds *window_bounds, int x, int y, char *text);
int MeasureMultilineTextWidth(FacemakerWindowBounds *window_bounds, char *text);
int SetFontStyleSize(int font_id, unsigned int style);
void DrawFormattedTextShadowedCenterY(FacemakerWindowBounds *window, int color, int x,
                                      int y, char *format, ...);
int HasQueuedKeyInput(void);
int PopQueuedKeyInput(void);
void PutGraphicsPixel(FacemakerWindowBounds *window_bounds, int x, int y, int color_index);
int RunNameEntryDialog(char *name_buffer);
void BeginSpriteEncodeSession(void);
EncodedImage *EncodeSpriteFromPage(int page_number, int x, int y, int width, int height);
void FinalizeSpriteEncodeSession(void);
void FreeSpriteBlob(void *memory);
void BuildFacemakerPortraitSprites(FacemakerWindowBounds *page);
int ResetMenuContext(int context_index);
void ShandalarLoadPcxIntoPageNoPalette(char *path);

void shandalar_host_pump(void);
int shandalar_host_run_facemaker(const char *asset_root, const char *writable_root);
int shandalar_runtime_init(const char *asset_root);
void shandalar_runtime_shutdown(void);

int g_default_palette_fade_steps = 8;
FILE *g_advbuttons_ini_file;
char g_ini_string_scratch[0x28];
int g_mouse_x_snapshot;
int g_mouse_y_snapshot;
int g_mouse_button_down_mask;
int g_mouse_button_mask_snapshot;
int g_mouse_button_released_mask;
int g_mouse_y;
int g_mouse_x;
int g_menu_render_guard;
int g_ui_tick_count;
extern int g_name_entry_cursor;
extern int g_menu_selection_value;
extern int g_color_menu_selection_to_deck_color[5];
EncodedImage *g_face_preview_sprite_group[6];
EncodedImage *g_face_preview_sprite_selected;
extern DIBSurface *g_facemaker_page4_dib;
extern HBITMAP g_facemaker_page4_bitmap;
static EncodedImage *g_name_entry_portrait_sprite;

extern char global_base_directory[0x105];

static void free_detached_facemaker_page4(void)
{
  if (!g_facemaker_page4_dib)
    return;
  if (g_graphics_pages[4] == g_facemaker_page4_dib)
    g_graphics_pages[4] = NULL;
  SelectObject(g_facemaker_page4_dib->hTempDC, g_facemaker_page4_dib->hPreviousBitmap);
  DeleteObject(g_facemaker_page4_dib->hBitmap);
  DeleteDC(g_facemaker_page4_dib->hTempDC);
  free(g_facemaker_page4_dib->pBitmapInfo);
  free(g_facemaker_page4_dib);
  g_facemaker_page4_dib = NULL;
  g_facemaker_page4_bitmap = NULL;
}

FacemakerWindowBounds g_page0_window_bounds_storage = {0, 0, 0, 0x27f, 0x1df, 1, 0x0f, 4, 0};
FacemakerWindowBounds *g_page0_window_bounds = &g_page0_window_bounds_storage;
FacemakerWindowBounds g_page1_window_bounds_storage = {1, 0, 0, 0x320, 0x258, 1, 0x0f, 4, 0};
FacemakerWindowBounds *g_page1_window_bounds = &g_page1_window_bounds_storage;
FacemakerWindowBounds g_page2_window_bounds_storage = {2, 0, 0, 0x27f, 0x1df, 1, 0x0f, 4, 0};
FacemakerWindowBounds *g_page2_window_bounds = &g_page2_window_bounds_storage;

int g_menu_context_index;
int g_menu_current_control_index = -1;
int g_menu_prev_control_index = -1;
int g_menu_input_unhandled;
int g_menu_control_count_by_context[50];
int g_menu_allow_arrow_nav_by_context[50];
AdvMenuControl *g_menu_controls_by_context[50][50];

char gs_loadsave_empty[] = "";
char gs_loadsave_load[] = "Load";
char gs_loadsave_save[] = "Save";
char gs_loadsave_none[] = "Empty";
char *gs_loadsave_0077d1b0[3] = {gs_loadsave_save, gs_loadsave_load, gs_loadsave_none};

static int g_runtime_initialized;

static int menu_control_contains_point(const AdvMenuControl *control, int x, int y)
{
  return control && control->state != 3 && control->x >= 0 &&
         x >= control->x && x <= control->x + control->width &&
         y >= control->y && y <= control->y + control->height;
}

static void render_menu_control(int control_index, int state)
{
  AdvMenuControl *control;
  if (control_index < 0 ||
      control_index >= g_menu_control_count_by_context[g_menu_context_index])
    return;

  control = g_menu_controls_by_context[g_menu_context_index][control_index];
  if (!control || !control->on_render)
    return;

  g_menu_render_guard = 1;
  control->on_render(control, state);
  g_menu_render_guard = 0;
}

static void select_menu_control(int control_index)
{
  if (control_index == g_menu_current_control_index)
    return;
  render_menu_control(g_menu_current_control_index, 0);
  g_menu_current_control_index = control_index;
  render_menu_control(g_menu_current_control_index, 1);
}

void set_global_base_directory(char *path)
{
  SDL_strlcpy(path, global_base_directory, MAX_PATH);
}

int FileExists(const char *filename)
{
  FILE *file = fopen(filename, "rb");
  if (!file)
    return 0;
  fclose(file);
  return 1;
}

int ScaleUiCoordinate(int value)
{
  return (value * global_screen_width) / 0x280;
}

int GetUiTickCount(void)
{
  return g_ui_tick_count;
}

void DelayUiTicks(int ticks)
{
  int end = g_ui_tick_count + ticks;
  while (g_ui_tick_count < end)
    shandalar_host_pump();
}

unsigned int PeekQueuedKeyInput(void)
{
  return g_key_input_queue_count > 0 ? g_key_input_queue[0] : 0;
}

void PushQueuedKeyInput(int key_code)
{
  if (g_key_input_queue_count >= 50)
    return;
  memmove(g_key_input_queue + 1, g_key_input_queue,
          (size_t)g_key_input_queue_count * sizeof(g_key_input_queue[0]));
  g_key_input_queue[0] = (unsigned int)key_code;
  ++g_key_input_queue_count;
}

int PopNormalizedQueuedKeyInput(void)
{
  int key = PopQueuedKeyInput();
  if ((key & 0xff) != 0)
    return key & 0xff;
  return key;
}

void UpdateMouseSnapshot(void)
{
  g_mouse_button_mask_snapshot = g_mouse_button_released_mask | g_mouse_button_down_mask;
  g_mouse_button_released_mask = 0;
  g_mouse_x_snapshot = g_mouse_x;
  g_mouse_y_snapshot = g_mouse_y;
}

int BeginMenuContext(void)
{
  ++g_menu_context_index;
  g_menu_current_control_index = -1;
  g_menu_prev_control_index = -1;
  ResetMenuContext(g_menu_context_index);
  return g_menu_context_index;
}

int ResetMenuContext(int context_index)
{
  if (context_index >= 0 && context_index < 50)
  {
    g_menu_control_count_by_context[context_index] = 0;
    g_menu_allow_arrow_nav_by_context[context_index] = 1;
  }
  return 0;
}

int AddMenuControlsToContext(AdvMenuControl *controls, int control_count, int context_index)
{
  int i;
  int base = g_menu_control_count_by_context[context_index];
  g_menu_render_guard = 1;
  for (i = 0; i < control_count && base + i < 50; ++i)
    g_menu_controls_by_context[context_index][base + i] = &controls[i];
  g_menu_control_count_by_context[context_index] = base + i;
  g_menu_render_guard = 0;
  return g_menu_control_count_by_context[context_index];
}

int EndMenuContext(void)
{
  ResetMenuContext(g_menu_context_index);
  if (g_menu_context_index > 0)
    --g_menu_context_index;
  g_menu_current_control_index = -1;
  g_menu_prev_control_index = -1;
  return g_menu_context_index;
}

int RenderAdvMenuControlDisabled(AdvMenuControl *control)
{
  if (!control)
    return 0;
  control->state = 3;
  return control->on_render ? control->on_render(control, 3) : 0;
}

int RenderAdvMenuControlNormally(AdvMenuControl *control)
{
  if (!control)
    return 0;
  control->state = 0;
  return control->on_render ? control->on_render(control, 0) : 0;
}

int UpdateMenuControlSelection(int mouse_x, int mouse_y, int allow_activate_on_click)
{
  int i;
  int key;
  int key_low;
  int count;
  shandalar_host_pump();
  UpdateMouseSnapshot();
  mouse_x = g_mouse_x_snapshot;
  mouse_y = g_mouse_y_snapshot;
  count = g_menu_control_count_by_context[g_menu_context_index];
  g_menu_input_unhandled = 1;

  if (HasQueuedKeyInput())
  {
    key = PeekQueuedKeyInput();
    key_low = key & 0xff;

    if (key == 0x0f09 || key == 0x0f0f ||
        (g_menu_allow_arrow_nav_by_context[g_menu_context_index] &&
         (key == 0x4800 || key == 0x5000)))
    {
      int direction = (key == 0x0f0f || key == 0x4800) ? -1 : 1;
      int next = g_menu_current_control_index;
      PopQueuedKeyInput();
      if (count > 0)
      {
        if (next < 0)
          next = direction > 0 ? 0 : count - 1;
        else
          next = (count + next + direction) % count;

        for (i = 0; i < count; ++i)
        {
          AdvMenuControl *control = g_menu_controls_by_context[g_menu_context_index][next];
          if (control && control->state != 3)
            break;
          next = (count + next + direction) % count;
        }
        select_menu_control(next);
        g_menu_prev_control_index = g_menu_current_control_index;
        g_menu_input_unhandled = 0;
        return g_menu_current_control_index;
      }
    }

    for (i = 0; i < count; ++i)
    {
      int control_index = (g_menu_current_control_index + 1 + i + count) % count;
      AdvMenuControl *control = g_menu_controls_by_context[g_menu_context_index][control_index];
      if (!control || control->state == 3)
        continue;
      if ((control->direct_hotkey && control->direct_hotkey == key) ||
          (key_low && control->navigate_hotkeys &&
           strchr(control->navigate_hotkeys, key_low)))
      {
        select_menu_control(control_index);
        if ((control->direct_hotkey && control->direct_hotkey == key) ||
            (key_low && control->activate_hotkeys &&
             strchr(control->activate_hotkeys, key_low)))
        {
          render_menu_control(g_menu_current_control_index, 2);
          PopQueuedKeyInput();
          g_menu_prev_control_index = g_menu_current_control_index;
          g_menu_input_unhandled = 0;
          return g_menu_current_control_index;
        }
        PopQueuedKeyInput();
        g_menu_prev_control_index = g_menu_current_control_index;
        g_menu_input_unhandled = 0;
        return g_menu_current_control_index;
      }
    }

    if ((key_low == 0x0d || key == 0x1c0d) && g_menu_current_control_index >= 0)
    {
      render_menu_control(g_menu_current_control_index, 2);
      PopQueuedKeyInput();
      g_menu_prev_control_index = g_menu_current_control_index;
      g_menu_input_unhandled = 0;
      return g_menu_current_control_index;
    }

    PopQueuedKeyInput();
  }

  for (i = 0; i < count; ++i)
  {
    AdvMenuControl *control = g_menu_controls_by_context[g_menu_context_index][i];
    if (!menu_control_contains_point(control, mouse_x, mouse_y))
      continue;
    select_menu_control(i);
    if (allow_activate_on_click)
    {
      render_menu_control(i, 2);
      g_mouse_button_down_mask = 0;
      g_mouse_button_released_mask = 0;
    }
    g_menu_prev_control_index = g_menu_current_control_index;
    g_menu_input_unhandled = 0;
    return g_menu_current_control_index;
  }

  select_menu_control(-1);
  g_menu_prev_control_index = g_menu_current_control_index;
  return -1;
}

unsigned int WaitForInputEventUnlessBlocked(void)
{
  while (!HasQueuedKeyInput() && g_mouse_button_down_mask == 0 &&
         g_mouse_button_released_mask == 0)
    shandalar_host_pump();
  if (HasQueuedKeyInput())
    return (unsigned int)PopNormalizedQueuedKeyInput();
  g_mouse_button_down_mask = 0;
  g_mouse_button_released_mask = 0;
  return 1;
}

void CopyGraphicsRect(FacemakerWindowBounds *src, int src_x, int src_y,
                      unsigned int width, int height, FacemakerWindowBounds *dst,
                      int dst_x, int dst_y)
{
  BlitGraphicsRect(src, src_x, src_y, width, (DWORD)height, dst, dst_x, dst_y);
}

void ShandalarLoadPcxIntoPageNoPalette(char *path)
{
  LoadPcxResource(-1, 0, 0, path, (void *)1);
}

int Palette_FindNearestEntryIndex(int target_r, int target_g, int target_b,
                                  unsigned char *palette_bytes)
{
  int best = 0;
  int best_distance = 0x7fffffff;
  int i;
  for (i = 0; i < 256; ++i)
  {
    int dr = target_r - palette_bytes[i * 3 + 0];
    int dg = target_g - palette_bytes[i * 3 + 1];
    int db = target_b - palette_bytes[i * 3 + 2];
    int distance = dr * dr + dg * dg + db * db;
    if (distance < best_distance)
    {
      best_distance = distance;
      best = i;
    }
  }
  return best;
}

void PlaySoundEffectOnChannel(char *sound_path, int channel, int volume,
                              int pitch_percent, int pan_percent)
{
  (void)sound_path;
  (void)channel;
  (void)volume;
  (void)pitch_percent;
  (void)pan_percent;
}

int FadeInPaletteFromGray(int gray, int steps)
{
  (void)gray;
  (void)steps;
  if (g_palette_data_words.signature == *(unsigned short *)"M0" ||
      g_palette_data_words.signature == *(unsigned short *)"M1")
    RpBits_ApplyPalette(&g_palette_data_words);
  shandalar_host_pump();
  return 0;
}

int GetFontStyleSize(int font_id)
{
  return g_font_slots[font_id].point_size;
}

void DrawFormattedTextNoShadowCentered(FacemakerWindowBounds *window, int color, int x,
                                       int y, char *format, ...)
{
  va_list args;
  va_start(args, format);
  DrawTextFormattedVa(window, color, 0, 0, 1, 1, x, y, format, args);
  va_end(args);
}

void DrawLoadSaveButtonText(FacemakerWindowBounds *window, int color, int x, int y, ...)
{
  char *text;
  va_list args;
  va_start(args, y);
  text = va_arg(args, char *);
  va_end(args);
  DrawFormattedTextShadowedCenterY(window, color, x, y, "%s", text ? text : "");
}

int int_to_hex_digit(int value)
{
  return value < 10 ? value + '0' : value + 'W';
}

static void decode_text_line(char *line)
{
  char *src = line;
  char *dst = line;
  while (*src)
  {
    if (*src == '\\')
    {
      ++src;
      switch (*src)
      {
      case 'n':
        *dst++ = '\n';
        ++src;
        break;
      case 'r':
        *dst++ = '\r';
        ++src;
        break;
      case '\\':
        *dst++ = '\\';
        ++src;
        break;
      default:
        if (*src)
          *dst++ = *src++;
        break;
      }
    }
    else
    {
      *dst++ = *src++;
    }
  }
  *dst = '\0';
}

static void strip_text_newline(char *line)
{
  size_t len = strlen(line);
  while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
    line[--len] = '\0';
}

int LoadTextSectionLines(const char *filename, const char *section)
{
  FILE *file;
  char marker[128];
  char line[512];
  int expected_count;
  int count;

  file = fopen(filename, "rt");
  if (!file)
    return 0;

  SDL_snprintf(marker, sizeof(marker), "@%s", section);
  while (fgets(line, sizeof(line), file))
  {
    strip_text_newline(line);
    if (SDL_strcasecmp(line, marker) == 0)
      break;
  }

  if (feof(file))
  {
    fclose(file);
    return 0;
  }

  if (!fgets(line, sizeof(line), file))
  {
    fclose(file);
    return 0;
  }
  expected_count = atoi(line);

  for (count = 0; count < expected_count && count < 249; ++count)
  {
    if (!fgets(g_text_lines[count], sizeof(g_text_lines[count]), file))
      break;
    strip_text_newline(g_text_lines[count]);
    decode_text_line(g_text_lines[count]);
  }

  fclose(file);
  return count;
}

static int load_facemaker_portrait_output(void)
{
  FacemakerWindowBounds page4_bounds = {4, 0, 0, 275, 169, 1, 0x0f, 4, 0};

  if (!FileExists("face.pic"))
    return 0;

  free_detached_facemaker_page4();
  if (!CreateGraphicsPage(4, 276, 170, 8))
    return 0;

  FillGraphicsRect(&page4_bounds, 0, 0, 276, 170, 0);
  LoadPcxIntoPage(4, "face.pic");
  BuildFacemakerPortraitSprites(&page4_bounds);
  LoadPcxIntoPage(4, "face.pic");
  if (g_name_entry_portrait_sprite)
    FreeSpriteBlob(g_name_entry_portrait_sprite);
  BeginSpriteEncodeSession();
  g_name_entry_portrait_sprite = EncodeSpriteFromPage(4, 0, 0, 0x89, 0xa9);
  FinalizeSpriteEncodeSession();

  g_facemaker_page4_bitmap = g_graphics_pages[4]->hBitmap;
  g_facemaker_page4_dib = g_graphics_pages[4];
  SelectObject(g_graphics_pages[4]->hTempDC, g_graphics_pages[4]->hPreviousBitmap);
  g_graphics_pages[4] = NULL;
  return 1;
}

static void prepare_name_entry_background(void)
{
  static char *pedestal_images[6] = {
      NULL, "prdblk.pic", "prdblu.pic", "prdgrn.pic", "prdrd.pic", "prdwt.pic"};
  int selected_color;
  char *pedestal_path;

  selected_color = g_selected_wizard_color;
  if ((selected_color < 1 || selected_color > 5) &&
      g_menu_selection_value >= 1 && g_menu_selection_value <= 5)
    selected_color = g_color_menu_selection_to_deck_color[g_menu_selection_value - 1];
  if (selected_color < 1 || selected_color > 5)
    selected_color = 1;

  pedestal_path = pedestal_images[selected_color];
  ShandalarLoadPcxIntoPageNoPalette("pedstls.pic");
  LoadPcxResource(1, global_screen_width - 0x280, global_screen_height - 0x1e0,
                  pedestal_path, (g_graphics_bpp == 8) ? &g_palette_data_words : (void *)1);
  StretchBlitGraphicsRect(g_page1_window_bounds, global_screen_width - 0x280,
                          global_screen_height - 0x1e0, 0x280, 0x1e0,
                          g_page1_window_bounds, 0, 0, global_screen_width,
                          global_screen_height);
  if (g_name_entry_portrait_sprite)
    DrawEncodedImageResampled(g_page1_window_bounds,
                              global_screen_width / 2 - ScaleUiCoordinate(0x89),
                              ScaleUiCoordinate(0x14),
                              ScaleUiCoordinate(0x112),
                              ScaleUiCoordinate(0x152),
                              g_name_entry_portrait_sprite);
  BlitGraphicsRect(g_page1_window_bounds, 0, 0, global_screen_width,
                   global_screen_height, g_page0_window_bounds, 0, 0);
}

int shandalar_run_facemaker_flow(void)
{
  char asset_root[sizeof(global_base_directory)];
  int completed;
  SDL_strlcpy(asset_root, global_base_directory, sizeof(asset_root));
  shandalar_runtime_shutdown();
  completed = shandalar_host_run_facemaker(asset_root, asset_root);
  if (g_graphics_pages[0] || g_graphics_pages[1])
  {
    ShutdownGraphicsSystem();
  }
  if (!shandalar_runtime_init(asset_root))
    return -1;
  if (!completed)
    return -1;
  load_facemaker_portrait_output();
  g_name_entry_cursor = (int)strlen(g_name_entry_buffer);
  prepare_name_entry_background();
  g_page0_window_bounds->font_slot = 1;
  if (RunNameEntryDialog(g_name_entry_buffer) == -1)
    return -1;
  strcpy(g_player_name, g_name_entry_buffer);
  g_player_is_male = 1;
  return g_player_is_male;
}

int shandalar_runtime_init(const char *asset_root)
{
  if (g_runtime_initialized)
    return 1;
  SDL_strlcpy(global_base_directory, asset_root ? asset_root : ".", sizeof(global_base_directory));
  chdir(global_base_directory);
  shandalar_set_default_font_root(global_base_directory);
  global_screen_width = 1024;
  global_screen_height = 768;
  global_main_hdc = shandalar_create_screen_dc(global_screen_width, global_screen_height);
  if (!global_main_hdc)
    return 0;
  if (!InitializeGraphicsSystem(global_screen_width, global_screen_height, 8))
    return 0;
  CreateGraphicsPage(1, 1024, 800, 8);
  CreateGraphicsPage(2, global_screen_width, global_screen_height + 1, 8);
  g_page0_window_bounds->max_x = global_screen_width - 1;
  g_page0_window_bounds->max_y = global_screen_height - 1;
  g_page1_window_bounds->max_x = 1023;
  g_page1_window_bounds->max_y = 799;
  g_page2_window_bounds->max_x = global_screen_width - 1;
  g_page2_window_bounds->max_y = global_screen_height;
  LoadSystemFont(1, 11, "tt0300m_.ttf", "MPZurich Cn BT", 400, 0);
  LoadSystemFont(2, 9, "tt0298m_.ttf", "MPZurich Cn BT", 400, 0);
  LoadSystemFont(4, ScaleUiCoordinate(10), "tt0530m_.ttf", "Benguiat BkCn BT", 100, 0);
  LoadSystemFont(5, ScaleUiCoordinate(0x14), "tt0127m_.ttf", "Benguiat Bk BT", 700, 1);
  LoadSystemFont(6, ScaleUiCoordinate(0x1c), "tt0530m_.ttf", "Benguiat BkCn BT", 100, 0);
  LoadSystemFont(7, ScaleUiCoordinate(0xb), "tt0530m_.ttf", "Benguiat BkCn BT", 100, 0);
  g_page0_window_bounds->font_slot = 1;
  g_advbuttons_ini_file = fopen("advButtons.txt", "rt");
  strcpy(g_text_lines[2], "Enter your name");
  g_runtime_initialized = 1;
  return g_advbuttons_ini_file != NULL;
}

void shandalar_runtime_shutdown(void)
{
  if (g_advbuttons_ini_file)
  {
    fclose(g_advbuttons_ini_file);
    g_advbuttons_ini_file = NULL;
  }
  ShutdownGraphicsSystem();
  shandalar_destroy_screen_dc(global_main_hdc);
  global_main_hdc = NULL;
  g_runtime_initialized = 0;
}
