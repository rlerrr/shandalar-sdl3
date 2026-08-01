#include <SDL3/SDL.h>

#include "platform/asset_manager.h"
#include "platform/gdi_compat.h"
#include "platform/winuser_compat.h"
#include "platform/winuser_sdl_compositor.h"

#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

extern int WINAPI FacemakerWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nShowCmd);

HDC global_main_hdc;
int global_screen_width;
int global_screen_height;

static SDL_Window *g_window;
static SDL_Renderer *g_renderer;
static int g_quit_requested;
static int g_smoke_frames_remaining;
static const char *g_snapshot_path;
static int g_snapshot_saved;

static void save_snapshot(void)
{
  SDL_Surface *surface;
  if (!g_snapshot_path || g_snapshot_saved)
    return;
  surface = SDL_RenderReadPixels(g_renderer, NULL);
  if (surface) {
    SDL_SaveBMP(surface, g_snapshot_path);
    SDL_DestroySurface(surface);
    g_snapshot_saved = 1;
  }
}

static UINT virtual_key(SDL_Keycode key)
{
  switch (key) {
  case SDLK_LEFT: return VK_LEFT;
  case SDLK_RIGHT: return VK_RIGHT;
  case SDLK_UP: return VK_UP;
  case SDLK_DOWN: return VK_DOWN;
  case SDLK_RETURN: return 0x0d;
  case SDLK_ESCAPE: return 0x1b;
  case SDLK_BACKSPACE: return 0x08;
  default:
    return key >= 0 && key <= 0xff ? (UINT)key : 0;
  }
}

static void forward_keyboard_event(const SDL_Event *event)
{
  HWND focus = GetFocus();
  UINT key;
  if (!focus)
    return;
  if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {
    key = virtual_key(event->key.key);
    if (key)
      PostMessageA(focus, event->type == SDL_EVENT_KEY_DOWN ? WM_KEYDOWN : WM_KEYUP,
                   key, 0);
  } else if (event->type == SDL_EVENT_TEXT_INPUT && event->text.text) {
    const unsigned char *text = (const unsigned char *)event->text.text;
    while (*text)
      PostMessageA(focus, WM_CHAR, *text++, 0);
  }
}

#ifdef __EMSCRIPTEN__
EM_ASYNC_JS(void, wait_next_frame, (), {
  await new Promise(requestAnimationFrame);
});
#endif

static BOOL pump_facemaker_ui(BOOL wait)
{
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    shandalar_win32_sdl_compositor_process_event(g_renderer, &event);
    if (event.type == SDL_EVENT_QUIT) {
      g_quit_requested = 1;
      PostQuitMessage(0);
    } else {
      forward_keyboard_event(&event);
    }
  }

  SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
  SDL_RenderClear(g_renderer);
  shandalar_win32_sdl_compositor_present(g_renderer);
  SDL_RenderPresent(g_renderer);

  if (g_smoke_frames_remaining > 0 && --g_smoke_frames_remaining == 0) {
    save_snapshot();
    g_quit_requested = 1;
    PostQuitMessage(0);
  }

#ifdef __EMSCRIPTEN__
  wait_next_frame();
#else
  if (wait)
    SDL_Delay(8);
#endif
  return !g_quit_requested;
}

int main(int argc, char **argv)
{
  const char *asset_root = "C:/Magic/Program";
  char module_path[1024];
  char cmd_line[16] = "/1";
  int i;
  int result;
#ifdef __EMSCRIPTEN__
  asset_root = "/assets";
#endif
  for (i = 1; i < argc; ++i) {
    if (SDL_strcmp(argv[i], "--smoke") == 0)
      g_smoke_frames_remaining = 30;
    else if (SDL_strncmp(argv[i], "--snapshot=", 11) == 0)
      g_snapshot_path = argv[i] + 11;
    else if (argv[i][0] == '/' || argv[i][0] == '-')
      SDL_snprintf(cmd_line, sizeof(cmd_line), "/%c", argv[i][1] ? argv[i][1] : '1');
    else
      asset_root = argv[i];
  }

  printf("facemaker: starting with asset root %s\n", asset_root);
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    printf("facemaker: SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }
  if (!SDL_CreateWindowAndRenderer("Magic: Facemaker (legacy)", 1024, 768,
                                   SDL_WINDOW_RESIZABLE,
                                   &g_window, &g_renderer)) {
    printf("facemaker: SDL_CreateWindowAndRenderer failed: %s\n", SDL_GetError());
    return 1;
  }
  SDL_SetRenderLogicalPresentation(g_renderer, 1024, 768,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
  SDL_StartTextInput(g_window);
  if (!shandalar_win32_sdl_compositor_init(g_renderer)) {
    printf("facemaker: compositor init failed\n");
    return 1;
  }

  shandalar_assets_set_root(asset_root);
  SDL_snprintf(module_path, sizeof(module_path), "%s/Facemaker.exe", asset_root);
  shandalar_win32_set_module_path(module_path);
  shandalar_set_default_font_root(asset_root);
  shandalar_win32_set_pump_proc(pump_facemaker_ui);

  result = FacemakerWinMain((HINSTANCE)1, (HINSTANCE)0, cmd_line, SW_SHOW);

  shandalar_win32_sdl_compositor_shutdown();
  shandalar_win32_reset();
  SDL_StopTextInput(g_window);
  SDL_DestroyRenderer(g_renderer);
  SDL_DestroyWindow(g_window);
  SDL_Quit();
  return result;
}
