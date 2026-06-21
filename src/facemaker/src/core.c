#include "facemaker_core.h"
#include "editor.h"

#include "platform/gdi_compat.h"
#include "drawcardlib/src/pic.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

extern DIBSurface *InitializeGraphicsSystem(int width, int height, int bits_per_pixel);
extern DIBSurface *CreateGraphicsPage(int page_number, int width, int height, int bits_per_pixel);
extern void ShutdownGraphicsSystem(void);
extern void LoadPcxResource(int page_number, int x, int y, char *path, void *palette);
extern void PresentGraphicsPage(int page_number);
extern int ExportEncodedImage(int page_number, int x, int y, int width, int height,
                              int write_palette, char *path);
extern PALETTEENTRY g_palette_entries[256];
extern DIBSurface *g_graphics_pages[10];

HDC global_main_hdc;
int global_screen_width = 1024;
int global_screen_height = 768;

static FacemakerStatus g_status = FACEMAKER_FAILED;
static uint64_t g_generation;
static char g_asset_root[512];
static char g_writable_root[512];

static int capped_dimension(int requested, int maximum)
{
  if (requested <= 0)
    return maximum;
  return requested < maximum ? requested : maximum;
}

static void make_asset_path(char *out, size_t size, const char *relative)
{
  SDL_snprintf(out, size, "%s/%s", g_asset_root, relative);
}

int facemaker_create(const FacemakerConfig *config)
{
  char background_path[1024];
  if (!config || !config->asset_root)
    return 0;
  if (global_main_hdc)
    facemaker_destroy();
  global_screen_width = capped_dimension(config->width, 1024);
  global_screen_height = capped_dimension(config->height, 768);
  SDL_strlcpy(g_asset_root, config->asset_root, sizeof(g_asset_root));
  SDL_strlcpy(g_writable_root, config->writable_root ? config->writable_root : ".",
              sizeof(g_writable_root));
  shandalar_set_default_font_root(g_asset_root);
  global_main_hdc = shandalar_create_screen_dc(global_screen_width, global_screen_height);
  if (!global_main_hdc || !InitializeGraphicsSystem(global_screen_width, global_screen_height, 8))
    goto fail;
  if (!CreateGraphicsPage(1, 640, 480, 8) ||
      !CreateGraphicsPage(2, global_screen_width, global_screen_height, 8))
    goto fail;
  make_asset_path(background_path, sizeof(background_path), "FaceArt/Facebk0New.pcx");
  LoadPcxResource(1, 0, 0, background_path, (void *)1);
  if (!facemaker_editor_create(g_asset_root, g_writable_root,
                               global_screen_width, global_screen_height,
                               config->download_on_save))
    goto fail;
  facemaker_editor_render();
  PresentGraphicsPage(2);
  g_generation = 1;
  g_status = FACEMAKER_RUNNING;
  return 1;

fail:
  facemaker_destroy();
  return 0;
}

void facemaker_destroy(void)
{
  facemaker_editor_destroy();
  ShutdownGraphicsSystem();
  shandalar_destroy_screen_dc(global_main_hdc);
  global_main_hdc = NULL;
  g_generation = 0;
  g_status = FACEMAKER_FAILED;
}

static void apply_editor_action(FacemakerEditorAction action)
{
  if (action == FACEMAKER_EDITOR_REDRAW) {
    facemaker_editor_render();
    PresentGraphicsPage(2);
    ++g_generation;
  } else if (action == FACEMAKER_EDITOR_COMPLETE) {
    g_status = FACEMAKER_COMPLETED;
  } else if (action == FACEMAKER_EDITOR_CANCEL) {
    g_status = FACEMAKER_CANCELLED;
  }
}

void facemaker_pointer_move(float x, float y)
{
  apply_editor_action(facemaker_editor_pointer_move(x, y));
}

void facemaker_pointer_button(int button, int pressed)
{
  apply_editor_action(facemaker_editor_pointer_button(button, pressed));
}

void facemaker_key(int key, int pressed, int repeat)
{
  apply_editor_action(facemaker_editor_key(key, pressed, repeat));
}

FacemakerStatus facemaker_step(uint64_t now_ns)
{
  (void)now_ns;
  return g_status;
}

int facemaker_get_frame(FacemakerFrame *frame)
{
  int pitch;
  int i;
  static unsigned char rgb_palette[256 * 3];
  const unsigned char *pixels = (const unsigned char *)shandalar_dc_pixels(global_main_hdc, &pitch);
  if (!frame || !pixels)
    return 0;
  for (i = 0; i < 256; ++i) {
    rgb_palette[i * 3 + 0] = g_palette_entries[i].peRed;
    rgb_palette[i * 3 + 1] = g_palette_entries[i].peGreen;
    rgb_palette[i * 3 + 2] = g_palette_entries[i].peBlue;
  }
  frame->pixels = pixels;
  frame->width = global_screen_width;
  frame->height = global_screen_height;
  frame->pitch = pitch;
  frame->rgb_palette = rgb_palette;
  frame->generation = g_generation;
  return 1;
}

int facemaker_save(const char *path)
{
  int result = facemaker_editor_save(path);
  if (result) {
    PresentGraphicsPage(2);
    ++g_generation;
  }
  return result;
}
