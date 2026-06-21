#include "editor.h"

#include "drawcardlib/src/pic.h"
#include "face_assets.h"
#include "face_controls.h"
#include "facemaker_types.h"
#include "platform/gdi_compat.h"

#include <SDL3/SDL.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#define BASE_WIDTH 640
#define BASE_HEIGHT 480
#define CONTROL_COUNT 21
#define BUTTON_ATLAS_PAGE 3
#define SLOT_ATLAS_PAGE 4
#define BUTTON_ATLAS_WIDTH 578
#define BUTTON_ATLAS_HEIGHT 439
#define SLOT_ATLAS_WIDTH 138
#define SLOT_ATLAS_HEIGHT 172
#define FACEMAKER_UI_FONT_FACE "Benguiat BkCn Bt"

typedef struct FaceControl {
  int x;
  int y;
  int width;
  int height;
} FaceControl;

typedef struct FacemakerEditor {
  char asset_root[512];
  char writable_root[512];
  int width;
  int height;
  FacemakerFaceDefinition definitions[FACEMAKER_MAX_FACE_SETS];
  int definition_count;
  int selected_set;
  int selected_frame[FACEMAKER_MAX_GROUPS];
  int scroll_offset;
  int hover_control;
  int pressed_control;
  char status_message[128];
  char button_labels[4][32];
  int button_text_sizes[4];
  int button_font_slot;
  int status_font_slot;
  int download_on_save;
  FacemakerWindowBounds canvas_bounds;
  FacemakerFaceSpriteSet sprites;
} FacemakerEditor;

extern DIBSurface *g_graphics_pages[10];
extern PALETTEENTRY g_palette_entries[256];
extern DIBSurface *CreateGraphicsPage(int page_number, int width, int height, int bits_per_pixel);
extern void LoadPcxResource(int page_number, int x, int y, char *path, void *palette);
extern int ExportEncodedImage(int page_number, int x, int y, int width, int height,
                              int write_palette, char *path);
extern void DrawEncodedImageUnscaled(FacemakerWindowBounds *dst, int x, int y,
                                     EncodedImage *encoded_image);
extern void DrawEncodedImageResampled(FacemakerWindowBounds *dst, int x, int y,
                                      int width, int height,
                                      EncodedImage *encoded_image);
extern int LoadSystemFont(int font_id, unsigned int point_size, char *font_file,
                          char *font_name, int weight, DWORD italic);
extern int SetFontStyleSize(int font_id, unsigned int style);
extern int DrawTextLine(FacemakerWindowBounds *window_bounds, int x, int y,
                        char *text);
extern FontSlot g_font_slots[0x10];

static FacemakerEditor g_editor;

#define FACE_CONTROL_GEOMETRY(x, y, width, height, callback, value_index, value_delta) \
  {x, y, width, height},
static const FaceControl g_controls[CONTROL_COUNT] = {
    FACEMAKER_FACE_CONTROL_LIST(FACE_CONTROL_GEOMETRY)
};
#undef FACE_CONTROL_GEOMETRY

#ifdef __EMSCRIPTEN__
EM_JS(int, facemaker_browser_download_bytes,
      (const char *path, const unsigned char *bytes, int byte_count), {
  var filePath = UTF8ToString(path);
  try {
    if (byte_count <= 0)
      throw new Error('Saved file is empty');
    var data = HEAPU8.slice(bytes, bytes + byte_count);
    var blob = new Blob([data], {type: 'application/octet-stream'});
    var url = URL.createObjectURL(blob);
    var link = document.createElement('a');
    var slash = filePath.lastIndexOf('/');
    link.href = url;
    link.download = slash >= 0 ? filePath.substring(slash + 1) : filePath;
    if (!link.download)
      link.download = 'face.pic';
    link.style.display = 'none';
    document.body.appendChild(link);
    link.click();
    setTimeout(function() {
      URL.revokeObjectURL(url);
      link.remove();
    }, 60000);
    return 1;
  } catch (error) {
    console.error('Facemaker could not download ' + filePath, error);
    return 0;
  }
});
#else
static int facemaker_browser_download_bytes(const char *path, const unsigned char *bytes,
                                            int byte_count)
{
  (void)path;
  (void)bytes;
  (void)byte_count;
  return 1;
}
#endif

static int facemaker_download_saved_file(const char *path)
{
  FILE *file;
  unsigned char *bytes;
  long byte_count;
  int result;

  file = fopen(path, "rb");
  if (!file)
    return 0;
  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return 0;
  }
  byte_count = ftell(file);
  if (byte_count <= 0 || fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return 0;
  }
  bytes = (unsigned char *)malloc((size_t)byte_count);
  if (!bytes) {
    fclose(file);
    return 0;
  }
  result = fread(bytes, 1, (size_t)byte_count, file) == (size_t)byte_count &&
           facemaker_browser_download_bytes(path, bytes, (int)byte_count);
  free(bytes);
  fclose(file);
  return result;
}

static int scale_x(int value)
{
  return value * g_editor.width / BASE_WIDTH;
}

static int scale_y(int value)
{
  return value * g_editor.height / BASE_HEIGHT;
}

static void update_canvas_bounds(void)
{
  g_editor.canvas_bounds.page_number = 2;
  g_editor.canvas_bounds.clip_left = 0;
  g_editor.canvas_bounds.clip_top = 0;
  g_editor.canvas_bounds.max_x = g_editor.width;
  g_editor.canvas_bounds.max_y = g_editor.height;
  g_editor.canvas_bounds.draw_shadow_enabled = 1;
  g_editor.canvas_bounds.text_color = 0xf;
  g_editor.canvas_bounds.unk_1c = 4;
  g_editor.canvas_bounds.font_slot = g_editor.status_font_slot;
}

static unsigned char nearest_palette_index(int red, int green, int blue)
{
  unsigned int best_distance = UINT32_MAX;
  int best_index = 0;
  int index;
  for (index = 0; index < 256; ++index) {
    int dr = red - g_palette_entries[index].peRed;
    int dg = green - g_palette_entries[index].peGreen;
    int db = blue - g_palette_entries[index].peBlue;
    unsigned int distance = (unsigned int)(dr * dr + dg * dg + db * db);
    if (distance < best_distance) {
      best_distance = distance;
      best_index = index;
    }
  }
  return (unsigned char)best_index;
}

static unsigned char *visual_pixel(DIBSurface *page, int x, int y)
{
  int pitch;
  if (!page || x < 0 || y < 0 || x >= page->width || y >= page->height)
    return NULL;
  pitch = page->width + page->rowPadding;
  return (unsigned char *)page->pBits + y * pitch + x;
}

static unsigned char *raw_pixel(DIBSurface *page, int x, int y)
{
  int pitch;
  if (!page || x < 0 || y < 0 || x >= page->width || y >= page->height)
    return NULL;
  pitch = page->width + page->rowPadding;
  return (unsigned char *)page->pBits + y * pitch + x;
}

static void set_status_message(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vsnprintf(g_editor.status_message, sizeof(g_editor.status_message), format, args);
  va_end(args);
  g_editor.status_message[sizeof(g_editor.status_message) - 1] = '\0';
}

static void draw_atlas_region(int page_number, int src_x, int src_y, int src_width,
                              int src_height, int dst_x, int dst_y,
                              int transparent_zero)
{
  DIBSurface *source = g_graphics_pages[page_number];
  DIBSurface *target = g_graphics_pages[2];
  int dst_width = scale_x(src_width);
  int dst_height = scale_y(src_height);
  int y;
  int x;
  if (!source || !target)
    return;
  for (y = 0; y < dst_height; ++y) {
    int source_y = src_y + y * src_height / dst_height;
    for (x = 0; x < dst_width; ++x) {
      int source_x = src_x + x * src_width / dst_width;
      unsigned char *src_pixel = raw_pixel(source, source_x, source_y);
      unsigned char *dst_pixel = visual_pixel(target, scale_x(dst_x) + x, scale_y(dst_y) + y);
      if (src_pixel && dst_pixel && (!transparent_zero || *src_pixel))
        *dst_pixel = *src_pixel;
    }
  }
}

static void draw_sprite_scaled(const EncodedImage *sprite, int x, int y)
{
  int draw_width;
  int draw_height;

  if (!sprite || !facemaker_face_sprite_is_valid(&g_editor.sprites, sprite))
    return;
  if (sprite->width <= 0 || sprite->height <= 0)
    return;
  draw_width = scale_x(sprite->width);
  draw_height = scale_y(sprite->height);
  if (draw_width == sprite->width && draw_height == sprite->height)
    DrawEncodedImageUnscaled(&g_editor.canvas_bounds, x, y, (EncodedImage *)sprite);
  else
    DrawEncodedImageResampled(&g_editor.canvas_bounds, scale_x(x), scale_y(y),
                              draw_width, draw_height, (EncodedImage *)sprite);
}

static void destroy_font_slot(int font_slot)
{
  if (font_slot <= 0 || font_slot >= 0x10)
    return;
  if (g_font_slots[font_slot].hfont)
    DeleteObject(g_font_slots[font_slot].hfont);
  g_font_slots[font_slot].hfont = NULL;
  g_font_slots[font_slot].font_loaded = 0;
}

static void draw_sprite_unscaled_legacy(const EncodedImage *sprite, int x, int y)
{
  if (!sprite || !facemaker_face_sprite_is_valid(&g_editor.sprites, sprite))
    return;
  DrawEncodedImageUnscaled(&g_editor.canvas_bounds, x, y, (EncodedImage *)sprite);
}

static void compose_preview(int x, int y)
{
  int group;
  draw_sprite_scaled(g_editor.sprites.base, x, y);
  for (group = 0; group < g_editor.sprites.group_count; ++group) {
    int frame = g_editor.selected_frame[group];
    if (frame >= 0 && frame < g_editor.sprites.frame_count[group])
      draw_sprite_scaled(g_editor.sprites.groups[group][frame], x, y);
  }
}

static void draw_text(int x, int y, const char *text, unsigned char color)
{
  FacemakerWindowBounds bounds = g_editor.canvas_bounds;
  char buffer[256];
  int draw_y;
  if (!text)
    return;
  SDL_strlcpy(buffer, text, sizeof(buffer));
  SetFontStyleSize(g_editor.status_font_slot, (unsigned int)scale_y(14));
  bounds.text_color = color;
  bounds.font_slot = g_editor.status_font_slot;
  draw_y = scale_y(y) + g_font_slots[g_editor.status_font_slot].tm_leading;
  DrawTextLine(&bounds, scale_x(x), draw_y, buffer);
}

static void draw_text_with_font(int x, int y, const char *text, unsigned char color,
                                int font_slot, int point_size)
{
  FacemakerWindowBounds bounds = g_editor.canvas_bounds;
  char buffer[256];
  int draw_y;
  if (!text)
    return;
  SDL_strlcpy(buffer, text, sizeof(buffer));
  SetFontStyleSize(font_slot, (unsigned int)scale_y(point_size));
  bounds.text_color = color;
  bounds.font_slot = font_slot;
  draw_y = scale_y(y) + g_font_slots[font_slot].tm_leading;
  DrawTextLine(&bounds, scale_x(x), draw_y, buffer);
}

static void draw_text_centered_at(int center_x, int center_y, const char *text,
                                  unsigned char color, int font_slot,
                                  int point_size)
{
  HDC dc = g_graphics_pages[2]->hTempDC;
  FacemakerWindowBounds bounds = g_editor.canvas_bounds;
  SIZE size;
  int draw_x;
  int draw_y;
  char buffer[256];
  if (!text)
    return;
  SDL_strlcpy(buffer, text, sizeof(buffer));
  SetFontStyleSize(font_slot, (unsigned int)scale_y(point_size));
  SelectObject(dc, g_font_slots[font_slot].hfont);
  if (!GetTextExtentPoint32A(dc, text, -1, &size)) {
    draw_text_with_font(center_x, center_y, text, color, font_slot, point_size);
    return;
  }
  draw_x = scale_x(center_x) - size.cx / 2;
  draw_y = scale_y(center_y) - size.cy / 2 + g_font_slots[font_slot].tm_leading;
  bounds.text_color = color;
  bounds.font_slot = font_slot;
  DrawTextLine(&bounds, draw_x, draw_y, buffer);
}

static int control_enabled(int control_index)
{
  int visible_group;
  int frame;
  if (control_index == 0 || control_index == 3 || control_index == 4)
    return 0;
  if (control_index == 7)
    return g_editor.scroll_offset > 0;
  if (control_index == 8)
    return g_editor.scroll_offset + 6 < g_editor.sprites.group_count;
  if (control_index >= 9) {
    visible_group = g_editor.scroll_offset + (control_index - 9) / 2;
    if (visible_group >= g_editor.sprites.group_count)
      return 0;
    frame = g_editor.selected_frame[visible_group];
    if ((control_index & 1) != 0)
      return frame >= 0;
    return frame + 1 < g_editor.sprites.frame_count[visible_group];
  }
  return 1;
}

static int control_mode(int index)
{
  if (index == 3 || index == 4)
    return 0;
  if (!control_enabled(index))
    return 3;
  if (index == g_editor.pressed_control)
    return 2;
  if (index == g_editor.hover_control)
    return 1;
  return 0;
}

static int button_atlas_slot_for_control(int index, int mode)
{
  if (index == 1)
    return mode;
  if (index == 2)
    return 4 + mode;
  if (index >= 5)
    return ((index & 1) ? 8 : 12) + mode;
  if (index == 3)
    return mode < 2 ? 19 + mode : 20;
  if (index == 4)
    return mode < 2 ? 21 + mode : 22;
  return -1;
}

static void draw_control(int index)
{
  const FaceControl *control = &g_controls[index];
  int mode = control_mode(index);
  int atlas_slot = button_atlas_slot_for_control(index, mode);
  if (atlas_slot < 0)
    return;
  draw_atlas_region(BUTTON_ATLAS_PAGE, (atlas_slot % 5) * 0x70,
                    (atlas_slot / 5) * 0x60, control->width, control->height,
                    control->x, control->y, 1);
  if (index >= 1 && index <= 4) {
    int label_center_x = control->x + (index <= 2 ? 0x2b : 0x20);
    int label_center_y = control->y + 0x0e;
    unsigned char color = mode == 0 ? 0xce : 0x3b;
    draw_text_centered_at(label_center_x, label_center_y,
                          g_editor.button_labels[index - 1],
                          color, g_editor.button_font_slot,
                          g_editor.button_text_sizes[mode]);
  }
}

static int load_face_definitions(void)
{
  char path[1024];
  SDL_snprintf(path, sizeof(path), "%s/FaceData.txt", g_editor.asset_root);
  g_editor.definition_count =
      facemaker_face_definitions_load(path, g_editor.definitions,
                                      FACEMAKER_MAX_FACE_SETS);
  return g_editor.definition_count > 0;
}

static int load_sprite_set(int definition_index)
{
  char path[1024];
  SDL_snprintf(path, sizeof(path), "%s/FaceArt/%s", g_editor.asset_root,
               g_editor.definitions[definition_index].file_name);
  if (!facemaker_face_sprite_set_load(path, &g_editor.sprites))
    return 0;
  memset(g_editor.selected_frame, 0, sizeof(g_editor.selected_frame));
  g_editor.scroll_offset = 0;
  return 1;
}

static int load_ui_atlases(void)
{
  char button_path[1024];
  char slot_path[1024];
  if (!CreateGraphicsPage(BUTTON_ATLAS_PAGE, BUTTON_ATLAS_WIDTH, BUTTON_ATLAS_HEIGHT, 8) ||
      !CreateGraphicsPage(SLOT_ATLAS_PAGE, SLOT_ATLAS_WIDTH, SLOT_ATLAS_HEIGHT, 8))
    return 0;
  SDL_snprintf(button_path, sizeof(button_path), "%s/FaceArt/facebut1.pcx",
               g_editor.asset_root);
  SDL_snprintf(slot_path, sizeof(slot_path), "%s/FaceArt/Facfrsh2New.pcx",
               g_editor.asset_root);
  LoadPcxResource(-1, 0, 0, button_path, (void *)1);
  LoadPcxResource(BUTTON_ATLAS_PAGE, 0, 0, button_path, (void *)0);
  LoadPcxResource(-1, 0, 0, slot_path, (void *)1);
  LoadPcxResource(SLOT_ATLAS_PAGE, 0, 0, slot_path, (void *)0);
  return 1;
}

static void load_button_labels(void)
{
  char path[1024];
  char ignored[128];
  char leave_label[32];
  char save_label[32];
  int font_slot;
  FILE *file;
  g_editor.button_text_sizes[0] = 10;
  g_editor.button_text_sizes[1] = 10;
  g_editor.button_text_sizes[2] = 8;
  g_editor.button_text_sizes[3] = 10;
  g_editor.button_font_slot = 4;
  SDL_strlcpy(g_editor.button_labels[0], "Save", sizeof(g_editor.button_labels[0]));
  SDL_strlcpy(g_editor.button_labels[1], "Leave", sizeof(g_editor.button_labels[1]));
  SDL_strlcpy(g_editor.button_labels[2], "Wizards", sizeof(g_editor.button_labels[2]));
  SDL_strlcpy(g_editor.button_labels[3], "Variations", sizeof(g_editor.button_labels[3]));
  SDL_snprintf(path, sizeof(path), "%s/FaceButtons.txt", g_editor.asset_root);
  file = fopen(path, "rt");
  if (!file)
    return;
  if (!fgets(ignored, sizeof(ignored), file) ||
      !fgets(ignored, sizeof(ignored), file) ||
      !fgets(leave_label, sizeof(leave_label), file) ||
      !fgets(save_label, sizeof(save_label), file) ||
      !fgets(g_editor.button_labels[2], sizeof(g_editor.button_labels[2]), file) ||
      !fgets(g_editor.button_labels[3], sizeof(g_editor.button_labels[3]), file)) {
    fclose(file);
    return;
  }
  if (fscanf(file, "%d %d,%d,%d,%d", &font_slot, &g_editor.button_text_sizes[0],
             &g_editor.button_text_sizes[1], &g_editor.button_text_sizes[2],
             &g_editor.button_text_sizes[3]) == 5 &&
      font_slot > 0 && font_slot < 0x10)
    g_editor.button_font_slot = font_slot;
  fclose(file);
  leave_label[strcspn(leave_label, "\r\n")] = '\0';
  save_label[strcspn(save_label, "\r\n")] = '\0';
  SDL_strlcpy(g_editor.button_labels[0], save_label, sizeof(g_editor.button_labels[0]));
  SDL_strlcpy(g_editor.button_labels[1], leave_label, sizeof(g_editor.button_labels[1]));
  g_editor.button_labels[0][strcspn(g_editor.button_labels[0], "\r\n")] = '\0';
  g_editor.button_labels[1][strcspn(g_editor.button_labels[1], "\r\n")] = '\0';
  g_editor.button_labels[2][strcspn(g_editor.button_labels[2], "\r\n")] = '\0';
  g_editor.button_labels[3][strcspn(g_editor.button_labels[3], "\r\n")] = '\0';
}

static void draw_slot(int visible_index)
{
  int group = g_editor.scroll_offset + visible_index;
  int x = ((visible_index + 2) % 4) * 159 + 13;
  int y = ((visible_index + 2) / 4) * 223 + 36;
  draw_atlas_region(SLOT_ATLAS_PAGE, 0, 0, 138, 172, x, y, 1);
  if (group < g_editor.sprites.group_count) {
    int frame = g_editor.selected_frame[group];
    if (frame >= 0 && frame < g_editor.sprites.frame_count[group])
      draw_sprite_scaled(g_editor.sprites.groups[group][frame], x, y);
  }
}

void facemaker_editor_render(void)
{
  DIBSurface *background = g_graphics_pages[1];
  DIBSurface *canvas = g_graphics_pages[2];
  int slot;
  if (!background || !canvas)
    return;
  draw_atlas_region(1, 0, 0, BASE_WIDTH, BASE_HEIGHT, 0, 0, 0);
  compose_preview(13, 36);
  for (slot = 0; slot < 6; ++slot)
    draw_slot(slot);

  draw_control(1);
  draw_control(2);
  draw_control(3);
  draw_control(4);
  draw_control(5);
  draw_control(6);
  draw_control(7);
  draw_control(8);
  for (slot = 9; slot < CONTROL_COUNT; ++slot)
    draw_control(slot);
  if (g_editor.status_message[0])
    draw_text(328, 12, g_editor.status_message, nearest_palette_index(230, 220, 170));
}

static int point_in_control(int control_index, float x, float y)
{
  const FaceControl *control = &g_controls[control_index];
  return control_enabled(control_index) &&
         x >= scale_x(control->x) && x < scale_x(control->x + control->width) &&
         y >= scale_y(control->y) && y < scale_y(control->y + control->height);
}

static int hit_test(float x, float y)
{
  int index;
  for (index = 1; index < CONTROL_COUNT; ++index) {
    if (point_in_control(index, x, y))
      return index;
  }
  return -1;
}

static FacemakerEditorAction activate_control(int control_index)
{
  int group;
  int delta;
  char path[1024];
  if (!control_enabled(control_index))
    return FACEMAKER_EDITOR_NONE;
  switch (control_index) {
  case 1:
    SDL_snprintf(path, sizeof(path), "%s/face.pic", g_editor.writable_root);
    facemaker_editor_save(path);
    return FACEMAKER_EDITOR_REDRAW;
  case 2:
    return FACEMAKER_EDITOR_COMPLETE;
  case 5:
  case 6:
    delta = control_index == 5 ? -1 : 1;
    g_editor.selected_set =
        (g_editor.selected_set + delta + g_editor.definition_count) % g_editor.definition_count;
    return load_sprite_set(g_editor.selected_set) ? FACEMAKER_EDITOR_REDRAW : FACEMAKER_EDITOR_NONE;
  case 7:
    --g_editor.scroll_offset;
    return FACEMAKER_EDITOR_REDRAW;
  case 8:
    ++g_editor.scroll_offset;
    return FACEMAKER_EDITOR_REDRAW;
  default:
    group = g_editor.scroll_offset + (control_index - 9) / 2;
    delta = (control_index & 1) ? -1 : 1;
    g_editor.selected_frame[group] += delta;
    return FACEMAKER_EDITOR_REDRAW;
  }
}

int facemaker_editor_create(const char *asset_root, const char *writable_root,
                            int width, int height, int download_on_save)
{
  char recent_path[1024];
  FILE *recent;
  int recent_index = 0;
  int font_index;
  memset(&g_editor, 0, sizeof(g_editor));
  SDL_strlcpy(g_editor.asset_root, asset_root, sizeof(g_editor.asset_root));
  SDL_strlcpy(g_editor.writable_root, writable_root ? writable_root : ".",
              sizeof(g_editor.writable_root));
  g_editor.width = width;
  g_editor.height = height;
  g_editor.download_on_save = download_on_save;
  g_editor.status_font_slot = 4;
  g_editor.button_font_slot = 4;
  update_canvas_bounds();
  g_editor.hover_control = -1;
  g_editor.pressed_control = -1;
  load_button_labels();
  if (!load_face_definitions())
    return 0;
  if (!load_ui_atlases())
    return 0;
  SDL_snprintf(recent_path, sizeof(recent_path), "%s/FaceMostRecent.txt", asset_root);
  recent = fopen(recent_path, "rt");
  if (recent) {
    if (fscanf(recent, "%d", &recent_index) == 1 &&
        recent_index >= 0 && recent_index < g_editor.definition_count)
      g_editor.selected_set = recent_index;
    fclose(recent);
  }
  if (!load_sprite_set(g_editor.selected_set))
    return 0;
  if (!LoadSystemFont(g_editor.status_font_slot, (unsigned int)scale_y(14),
                      "tt0530m_.ttf", FACEMAKER_UI_FONT_FACE, 100, 0))
    return 0;
  if (g_editor.button_font_slot != g_editor.status_font_slot &&
      !LoadSystemFont(g_editor.button_font_slot,
                      (unsigned int)scale_y(g_editor.button_text_sizes[0]),
                      "tt0530m_.ttf", FACEMAKER_UI_FONT_FACE, 100, 0))
    return 0;
  for (font_index = 0; font_index < 4; ++font_index)
    SetFontStyleSize(g_editor.button_font_slot,
                     (unsigned int)scale_y(g_editor.button_text_sizes[font_index]));
  SetFontStyleSize(g_editor.status_font_slot, (unsigned int)scale_y(14));
  return 1;
}

void facemaker_editor_destroy(void)
{
  facemaker_face_sprite_set_destroy(&g_editor.sprites);
  destroy_font_slot(g_editor.status_font_slot);
  if (g_editor.button_font_slot != g_editor.status_font_slot)
    destroy_font_slot(g_editor.button_font_slot);
  memset(&g_editor, 0, sizeof(g_editor));
}

FacemakerEditorAction facemaker_editor_pointer_move(float x, float y)
{
  int hover = hit_test(x, y);
  if (hover == g_editor.hover_control)
    return FACEMAKER_EDITOR_NONE;
  g_editor.hover_control = hover;
  return FACEMAKER_EDITOR_REDRAW;
}

FacemakerEditorAction facemaker_editor_pointer_button(int button, int pressed)
{
  int control;
  if (button != SDL_BUTTON_LEFT)
    return FACEMAKER_EDITOR_NONE;
  if (pressed) {
    g_editor.pressed_control = g_editor.hover_control;
    return g_editor.pressed_control >= 0 ? FACEMAKER_EDITOR_REDRAW : FACEMAKER_EDITOR_NONE;
  }
  control = g_editor.pressed_control;
  g_editor.pressed_control = -1;
  if (control >= 0 && control == g_editor.hover_control)
    return activate_control(control);
  return control >= 0 ? FACEMAKER_EDITOR_REDRAW : FACEMAKER_EDITOR_NONE;
}

FacemakerEditorAction facemaker_editor_key(int key, int pressed, int repeat)
{
  int group;
  (void)repeat;
  if (!pressed)
    return FACEMAKER_EDITOR_NONE;
  switch (key) {
  case SDLK_ESCAPE:
    return FACEMAKER_EDITOR_CANCEL;
  case SDLK_RETURN:
  case SDLK_KP_ENTER:
    return activate_control(1);
  case SDLK_LEFT:
    return activate_control(5);
  case SDLK_RIGHT:
    return activate_control(6);
  case SDLK_UP:
    return activate_control(7);
  case SDLK_DOWN:
    return activate_control(8);
  case SDLK_TAB:
    if (g_editor.sprites.group_count > 6)
      g_editor.scroll_offset = (g_editor.scroll_offset + 1) %
                               (g_editor.sprites.group_count - 5);
    return FACEMAKER_EDITOR_REDRAW;
  case SDLK_LEFTBRACKET:
  case SDLK_RIGHTBRACKET:
    group = g_editor.scroll_offset;
    if (key == SDLK_LEFTBRACKET && g_editor.selected_frame[group] >= 0)
      --g_editor.selected_frame[group];
    if (key == SDLK_RIGHTBRACKET &&
        g_editor.selected_frame[group] + 1 < g_editor.sprites.frame_count[group])
      ++g_editor.selected_frame[group];
    return FACEMAKER_EDITOR_REDRAW;
  default:
    return FACEMAKER_EDITOR_NONE;
  }
}

int facemaker_editor_save(const char *path)
{
  DIBSurface *page = g_graphics_pages[2];
  int pitch;
  int x;
  int y;
  int group;
  int result;
  if (!path || !page) {
    set_status_message("Save failed");
    facemaker_editor_render();
    return 0;
  }
  pitch = page->width + page->rowPadding;
  for (y = 0; y < 170; ++y)
    memset((unsigned char *)page->pBits + y * pitch, 0, 276);
  draw_sprite_unscaled_legacy(g_editor.sprites.base, 0, 0);
  for (group = 0; group < g_editor.sprites.group_count; ++group) {
    int frame = g_editor.selected_frame[group];
    if (frame >= 0 && frame < g_editor.sprites.frame_count[group])
      draw_sprite_unscaled_legacy(g_editor.sprites.groups[group][frame], 0, 0);
  }
  for (y = 0; y < 170; ++y) {
    unsigned char *row = (unsigned char *)page->pBits + y * pitch;
    for (x = 0; x < 138; ++x)
      row[138 + x] = row[x] ? 0 : 255;
  }
  result = ExportEncodedImage(2, 0, 0, 276, 170, 0, (char *)path) == 0;
  if (result) {
    if (g_editor.download_on_save) {
      set_status_message(facemaker_download_saved_file(path)
                             ? "Saved face.pic"
                             : "Saved face.pic; download failed");
    } else {
      set_status_message("Saved face.pic");
    }
  } else {
    set_status_message("Save failed");
  }
  facemaker_editor_render();
  return result;
}
