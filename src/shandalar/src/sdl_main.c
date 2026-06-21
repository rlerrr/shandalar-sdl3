#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "drawcardlib/src/pic.h"
#include "facemaker/src/facemaker_core.h"
#include "facemaker/src/facemaker_types.h"
#include "platform/gdi_compat.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

extern HDC global_main_hdc;
extern int global_screen_width;
extern int global_screen_height;
extern DIBSurface *g_graphics_pages[10];
extern PALETTEENTRY g_palette_entries[256];
extern int g_mouse_x;
extern int g_mouse_y;
extern int g_mouse_button_down_mask;
extern int g_mouse_button_released_mask;
extern int g_ui_tick_count;

int shandalar_runtime_init(const char *asset_root);
void shandalar_runtime_shutdown(void);
int RunOpeningMenu(void);
int RunDifficultyMenu(void);
int RunColorMenu(void);
int RunFacemakerFlow(void);
void PushQueuedKeyInput(int key_code);

static SDL_Window *g_window;
static SDL_Renderer *g_renderer;
static SDL_Texture *g_texture;
static uint32_t *g_rgba;
static int g_texture_width;
static int g_texture_height;
static int g_quit_requested;

static int legacy_key(SDL_Keycode key)
{
  switch (key) {
  case SDLK_RETURN: return 0x1c0d;
  case SDLK_KP_ENTER: return 0x1c0d;
  case SDLK_ESCAPE: return 0x011b;
  case SDLK_BACKSPACE: return 0x0e08;
  case SDLK_DELETE: return 0x5300;
  case SDLK_INSERT: return 0x5200;
  case SDLK_HOME: return 0x4700;
  case SDLK_END: return 0x4f00;
  case SDLK_LEFT: return 0x4b00;
  case SDLK_RIGHT: return 0x4d00;
  case SDLK_UP: return 0x4800;
  case SDLK_DOWN: return 0x5000;
  default: return 0;
  }
}

static int ensure_framebuffer(int width, int height)
{
  if (width <= 0 || height <= 0)
    return 0;
  if (g_texture && g_texture_width == width && g_texture_height == height)
    return 1;
  SDL_DestroyTexture(g_texture);
  SDL_free(g_rgba);
  g_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING, width, height);
  if (!g_texture) {
    g_rgba = NULL;
    g_texture_width = 0;
    g_texture_height = 0;
    return 0;
  }
  SDL_SetTextureScaleMode(g_texture, SDL_SCALEMODE_NEAREST);
  g_rgba = (uint32_t *)SDL_malloc((size_t)width * (size_t)height * sizeof(uint32_t));
  if (!g_rgba) {
    SDL_DestroyTexture(g_texture);
    g_texture = NULL;
    g_texture_width = 0;
    g_texture_height = 0;
    return 0;
  }
  g_texture_width = width;
  g_texture_height = height;
  return 1;
}

static void present_indexed_frame(const unsigned char *pixels, int width, int height,
                                  int pitch, const unsigned char *rgb_palette)
{
  int x;
  int y;
  if (!pixels || !rgb_palette || !ensure_framebuffer(width, height))
    return;
  for (y = 0; y < height; ++y) {
    const unsigned char *src = pixels + y * pitch;
    uint32_t *dst = g_rgba + y * width;
    for (x = 0; x < width; ++x) {
      const unsigned char *rgb = rgb_palette + src[x] * 3;
      dst[x] = 0xff000000u | ((uint32_t)rgb[0] << 16) |
               ((uint32_t)rgb[1] << 8) | rgb[2];
    }
  }
  SDL_UpdateTexture(g_texture, NULL, g_rgba, width * (int)sizeof(uint32_t));
  SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
  SDL_RenderClear(g_renderer);
  SDL_RenderTexture(g_renderer, g_texture, NULL, NULL);
  SDL_RenderPresent(g_renderer);
}

static void present_frame(void)
{
  int pitch;
  int i;
  unsigned char rgb_palette[256 * 3];
  const unsigned char *pixels = (const unsigned char *)shandalar_dc_pixels(global_main_hdc, &pitch);
  if (!pixels)
    return;
  for (i = 0; i < 256; ++i) {
    rgb_palette[i * 3 + 0] = g_palette_entries[i].peRed;
    rgb_palette[i * 3 + 1] = g_palette_entries[i].peGreen;
    rgb_palette[i * 3 + 2] = g_palette_entries[i].peBlue;
  }
  present_indexed_frame(pixels, global_screen_width, global_screen_height, pitch, rgb_palette);
}

int shandalar_host_run_facemaker(const char *asset_root, const char *writable_root)
{
  FacemakerConfig config;
  FacemakerFrame frame;
  FacemakerStatus status;
  int completed = 0;

  config.asset_root = asset_root;
  config.writable_root = writable_root ? writable_root : ".";
  config.width = global_screen_width;
  config.height = global_screen_height;
  SDL_SetWindowTitle(g_window, "Magic: Facemaker");
  SDL_SetRenderLogicalPresentation(g_renderer, config.width, config.height,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
  if (!facemaker_create(&config))
    goto done;

  while (!g_quit_requested) {
    SDL_Event event;
    float x;
    float y;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT:
        g_quit_requested = 1;
        break;
      case SDL_EVENT_MOUSE_MOTION:
        x = event.motion.x;
        y = event.motion.y;
        SDL_RenderCoordinatesFromWindow(g_renderer, x, y, &x, &y);
        facemaker_pointer_move(x, y);
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      case SDL_EVENT_MOUSE_BUTTON_UP:
        x = event.button.x;
        y = event.button.y;
        SDL_RenderCoordinatesFromWindow(g_renderer, x, y, &x, &y);
        facemaker_pointer_move(x, y);
        facemaker_pointer_button(event.button.button,
                                 event.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        break;
      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP:
        facemaker_key((int)event.key.key, event.type == SDL_EVENT_KEY_DOWN,
                      event.key.repeat);
        break;
      }
    }

    status = facemaker_step(SDL_GetTicksNS());
    if (facemaker_get_frame(&frame))
      present_indexed_frame(frame.pixels, frame.width, frame.height, frame.pitch,
                            frame.rgb_palette);
    if (status == FACEMAKER_COMPLETED) {
      facemaker_save("face.pic");
      completed = 1;
      break;
    }
    if (status == FACEMAKER_CANCELLED || status == FACEMAKER_FAILED)
      break;
#ifdef __EMSCRIPTEN__
    emscripten_sleep(0);
#else
    SDL_Delay(1);
#endif
  }

done:
  facemaker_destroy();
  SDL_SetWindowTitle(g_window, "Magic: Shandalar");
  SDL_SetRenderLogicalPresentation(g_renderer, global_screen_width, global_screen_height,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
  return completed;
}

void shandalar_host_pump(void)
{
  SDL_Event event;
  float x;
  float y;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_EVENT_QUIT:
      g_quit_requested = 1;
      break;
    case SDL_EVENT_MOUSE_MOTION:
      x = event.motion.x;
      y = event.motion.y;
      SDL_RenderCoordinatesFromWindow(g_renderer, x, y, &x, &y);
      g_mouse_x = (int)x;
      g_mouse_y = (int)y;
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      x = event.button.x;
      y = event.button.y;
      SDL_RenderCoordinatesFromWindow(g_renderer, x, y, &x, &y);
      g_mouse_x = (int)x;
      g_mouse_y = (int)y;
      if (event.button.button == SDL_BUTTON_LEFT)
        g_mouse_button_down_mask = 1;
      break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
      if (event.button.button == SDL_BUTTON_LEFT)
        g_mouse_button_released_mask |= 1;
      g_mouse_button_down_mask = 0;
      break;
    case SDL_EVENT_KEY_DOWN: {
      int key = 0;
      if (event.key.key == SDLK_TAB)
        key = (event.key.mod & SDL_KMOD_SHIFT) ? 0x0f0f : 0x0f09;
      else
        key = legacy_key(event.key.key);
      if (key)
        PushQueuedKeyInput(key);
      break;
    }
    case SDL_EVENT_TEXT_INPUT:
      if (event.text.text && event.text.text[0])
        PushQueuedKeyInput((unsigned char)event.text.text[0]);
      break;
    }
  }
  ++g_ui_tick_count;
  present_frame();
#ifdef __EMSCRIPTEN__
  emscripten_sleep(0);
#else
  SDL_Delay(1);
#endif
}

static int run_setup_flow(void)
{
  int mode;
  int difficulty;
  int color;
  int male;
  for (;;) {
    if (g_quit_requested)
      return 0;
    mode = RunOpeningMenu();
    if (mode == 4) {
      g_quit_requested = 1;
      return 0;
    }
    if (mode != 0)
      continue;
    for (;;) {
      difficulty = RunDifficultyMenu();
      if (difficulty < 0 || g_quit_requested)
        break;
      for (;;) {
        color = RunColorMenu();
        if (color < 0 || g_quit_requested)
          break;
        male = RunFacemakerFlow();
        if (male >= 0 || g_quit_requested)
          return 0;
      }
    }
  }
}

int main(int argc, char **argv)
{
  const char *asset_root = argc > 1 ? argv[1] : "C:/Magic/Program";
#ifdef __EMSCRIPTEN__
  asset_root = argc > 1 ? argv[1] : "/assets";
#endif
  if (!SDL_Init(SDL_INIT_VIDEO))
    return 1;
  if (!shandalar_runtime_init(asset_root))
    return 1;
  if (!SDL_CreateWindowAndRenderer("Magic: Shandalar", global_screen_width,
                                   global_screen_height, SDL_WINDOW_RESIZABLE,
                                   &g_window, &g_renderer))
    return 1;
  SDL_SetRenderLogicalPresentation(g_renderer, global_screen_width, global_screen_height,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
  if (!ensure_framebuffer(global_screen_width, global_screen_height))
    return 1;
  SDL_StartTextInput(g_window);
  run_setup_flow();
  while (!g_quit_requested)
    shandalar_host_pump();
  SDL_free(g_rgba);
  SDL_DestroyTexture(g_texture);
  SDL_DestroyRenderer(g_renderer);
  SDL_DestroyWindow(g_window);
  shandalar_runtime_shutdown();
  SDL_Quit();
  return 0;
}
