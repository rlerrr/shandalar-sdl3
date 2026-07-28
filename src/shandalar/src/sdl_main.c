#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "drawcardlib/src/pic.h"
#include "facemaker/src/facemaker_types.h"
#include "magic/src/global_state.h"
#include "platform/asset_manager.h"
#include "platform/gdi_compat.h"
#include "platform/winuser_compat.h"
#include "platform/winuser_imgui_bridge.h"
#include "statwin/statwin.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

extern HDC global_main_hdc;
extern int global_screen_width;
extern int global_screen_height;
extern DIBSurface *g_graphics_pages[10];
extern int g_mouse_x;
extern int g_mouse_y;
extern int g_mouse_button_down_mask;
extern int g_mouse_button_released_mask;
extern int g_ui_tick_count;
extern int g_worker_should_exit;
extern int g_worker_exit_code;
extern int g_ui_ready;
extern int g_mouse_button_down;

int shandalar_runtime_init(const char *asset_root);
void shandalar_runtime_shutdown(void);
void ShutdownGraphicsSystem(void);
void PushQueuedKeyInput(int key_code);
extern int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nShowCmd);
extern int WINAPI FacemakerWinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int nShowCmd);

static BOOL pump_shandalar_win32_ui(BOOL wait);
static BOOL pump_deckbuilder_ui(BOOL wait);

static SDL_Window *g_window;
static SDL_Renderer *g_renderer;
static SDL_Texture *g_texture;
static uint32_t *g_rgba;
static int g_texture_width;
static int g_texture_height;
static int g_quit_requested;
static int g_facemaker_host_active;
static int g_deckbuilder_host_active;

static int legacy_key(SDL_Keycode key)
{
  switch (key)
  {
  case SDLK_RETURN:
    return 0x1c0d;
  case SDLK_KP_ENTER:
    return 0x1c0d;
  case SDLK_ESCAPE:
    return 0x011b;
  case SDLK_BACKSPACE:
    return 0x0e08;
  case SDLK_DELETE:
    return 0x5300;
  case SDLK_INSERT:
    return 0x5200;
  case SDLK_HOME:
    return 0x4700;
  case SDLK_END:
    return 0x4f00;
  case SDLK_LEFT:
    return 0x4b00;
  case SDLK_RIGHT:
    return 0x4d00;
  case SDLK_UP:
    return 0x4800;
  case SDLK_DOWN:
    return 0x5000;
  default:
    return 0;
  }
}

static int shandalar_mouse_button_mask(Uint8 button)
{
  switch (button)
  {
  case SDL_BUTTON_LEFT:
    return 1;
  case SDL_BUTTON_RIGHT:
    return 2;
  default:
    return 0;
  }
}

static UINT facemaker_virtual_key(SDL_Keycode key)
{
  switch (key)
  {
  case SDLK_LEFT:
    return VK_LEFT;
  case SDLK_RIGHT:
    return VK_RIGHT;
  case SDLK_UP:
    return VK_UP;
  case SDLK_DOWN:
    return VK_DOWN;
  case SDLK_RETURN:
    return 0x0d;
  case SDLK_ESCAPE:
    return 0x1b;
  case SDLK_BACKSPACE:
    return 0x08;
  default:
    return key >= 0 && key <= 0xff ? (UINT)key : 0;
  }
}

static UINT deckbuilder_virtual_key(SDL_Keycode key)
{
  switch (key)
  {
  case SDLK_LEFT:
    return VK_LEFT;
  case SDLK_RIGHT:
    return VK_RIGHT;
  case SDLK_UP:
    return VK_UP;
  case SDLK_DOWN:
    return VK_DOWN;
  case SDLK_HOME:
    return VK_HOME;
  case SDLK_END:
    return VK_END;
  case SDLK_PAGEUP:
    return VK_PRIOR;
  case SDLK_PAGEDOWN:
    return VK_NEXT;
  case SDLK_RETURN:
    return 0x0d;
  case SDLK_ESCAPE:
    return 0x1b;
  case SDLK_BACKSPACE:
    return 0x08;
  case SDLK_DELETE:
    return 0x2e;
  default:
    return key >= 0 && key <= 0xff ? (UINT)key : 0;
  }
}

static void forward_facemaker_keyboard_event(const SDL_Event *event)
{
  HWND focus = GetFocus();
  UINT key;
  if (!focus)
    return;
  if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP)
  {
    key = facemaker_virtual_key(event->key.key);
    if (key)
      PostMessageA(focus, event->type == SDL_EVENT_KEY_DOWN ? WM_KEYDOWN : WM_KEYUP,
                   key, 0);
  }
  else if (event->type == SDL_EVENT_TEXT_INPUT && event->text.text)
  {
    const unsigned char *text = (const unsigned char *)event->text.text;
    while (*text)
      PostMessageA(focus, WM_CHAR, *text++, 0);
  }
}

static void forward_deckbuilder_keyboard_event(const SDL_Event *event)
{
  HWND focus = GetFocus();
  char class_name[32] = {0};
  UINT key;
  if (!focus)
    return;
  GetClassNameA(focus, class_name, sizeof(class_name));
  if (SDL_strcasecmp(class_name, "EDIT") == 0 ||
      SDL_strcasecmp(class_name, "LISTBOX") == 0 ||
      SDL_strcasecmp(class_name, "COMBOBOX") == 0)
    return;
  if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP)
  {
    key = deckbuilder_virtual_key(event->key.key);
    if (key)
      PostMessageA(focus, event->type == SDL_EVENT_KEY_DOWN ? WM_KEYDOWN : WM_KEYUP,
                   key, 0);
  }
  else if (event->type == SDL_EVENT_TEXT_INPUT && event->text.text)
  {
    const unsigned char *text = (const unsigned char *)event->text.text;
    while (*text)
      PostMessageA(focus, WM_CHAR, *text++, 0);
  }
}

EM_ASYNC_JS(void, wait_next_frame, (), {
  await new Promise(requestAnimationFrame);
});

static BOOL pump_facemaker_ui(BOOL wait)
{
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    shandalar_win32_imgui_process_event(g_renderer, &event);
    if (event.type == SDL_EVENT_QUIT)
    {
      g_quit_requested = 1;
      PostQuitMessage(0);
    }
    else
    {
      forward_facemaker_keyboard_event(&event);
    }
  }

  SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
  SDL_RenderClear(g_renderer);
  shandalar_win32_imgui_begin_frame();
  shandalar_win32_imgui_present(g_renderer);
  shandalar_win32_imgui_render(g_renderer);
  SDL_RenderPresent(g_renderer);

#ifdef __EMSCRIPTEN__
  if (wait)
    wait_next_frame();
  else
    emscripten_sleep(1);
#else
  if (wait)
    SDL_Delay(8);
#endif
  return !g_quit_requested && g_facemaker_host_active;
}

static BOOL pump_deckbuilder_ui(BOOL wait)
{
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    shandalar_win32_imgui_process_event(g_renderer, &event);
    if (event.type == SDL_EVENT_QUIT)
    {
      g_quit_requested = 1;
      PostQuitMessage(0);
    }
    else
    {
      forward_deckbuilder_keyboard_event(&event);
    }
  }

  shandalar_win32_runtime_tick();
  shandalar_win32_mm_timers_tick();

  SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
  SDL_RenderClear(g_renderer);
  shandalar_win32_imgui_begin_frame();
  shandalar_win32_imgui_present(g_renderer);
  shandalar_win32_imgui_render(g_renderer);
  SDL_RenderPresent(g_renderer);

#ifdef __EMSCRIPTEN__
  emscripten_sleep(wait ? 8 : 0);
#else
  if (wait)
    SDL_Delay(8);
#endif
  return !g_quit_requested && g_deckbuilder_host_active;
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
  if (!g_texture)
  {
    g_rgba = NULL;
    g_texture_width = 0;
    g_texture_height = 0;
    return 0;
  }
  SDL_SetTextureScaleMode(g_texture, SDL_SCALEMODE_NEAREST);
  g_rgba = (uint32_t *)SDL_malloc((size_t)width * (size_t)height * sizeof(uint32_t));
  if (!g_rgba)
  {
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

static void present_frame(void)
{
  if (!ensure_framebuffer(global_screen_width, global_screen_height) ||
      !shandalar_dc_copy_rgba(global_main_hdc, g_rgba,
                              global_screen_width * (int)sizeof(uint32_t)))
    return;

  SDL_UpdateTexture(g_texture, NULL, g_rgba,
                    global_screen_width * (int)sizeof(uint32_t));
  SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
  SDL_RenderClear(g_renderer);
  SDL_RenderTexture(g_renderer, g_texture, NULL, NULL);
  SDL_RenderPresent(g_renderer);
}

int shandalar_host_run_facemaker(const char *asset_root, const char *writable_root)
{
  HDC shandalar_hdc = global_main_hdc;
  int shandalar_width = global_screen_width;
  int shandalar_height = global_screen_height;
  char module_path[1024];
  char cmd_line[16] = "/S";
  int result;

  (void)writable_root;
  SDL_SetWindowTitle(g_window, "Magic: Facemaker");
  SDL_SetRenderLogicalPresentation(g_renderer, 1024, 768,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
  SDL_StartTextInput(g_window);
  if (!shandalar_win32_imgui_init(g_window, g_renderer))
    return 0;

  shandalar_assets_set_root(asset_root);

  SDL_snprintf(module_path, sizeof(module_path), "%s/Facemaker.exe", asset_root);
  shandalar_win32_set_module_path(module_path);
  shandalar_set_default_font_root(asset_root);
  shandalar_win32_set_pump_proc(pump_facemaker_ui);
  shandalar_win32_clear_quit_message();
  g_worker_should_exit = 0;
  g_worker_exit_code = 0;
  g_ui_ready = 0;
  g_mouse_button_down = 0;
  g_facemaker_host_active = 1;

  if (g_graphics_pages[0] || g_graphics_pages[1])
    ShutdownGraphicsSystem();

  result = FacemakerWinMain((HINSTANCE)1, (HINSTANCE)0, cmd_line, SW_SHOW);

  g_facemaker_host_active = 0;
  shandalar_win32_set_pump_proc(NULL);
  shandalar_win32_imgui_shutdown();
  ShutdownGraphicsSystem();
  global_main_hdc = shandalar_hdc;
  global_screen_width = shandalar_width;
  global_screen_height = shandalar_height;
  SDL_StopTextInput(g_window);
  SDL_StartTextInput(g_window);
  SDL_SetWindowTitle(g_window, "Magic: Shandalar");
  SDL_SetRenderLogicalPresentation(g_renderer, global_screen_width, global_screen_height,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
  return result == 0 && !g_quit_requested;
}

void shandalar_host_begin_deckbuilder(void)
{
  SDL_SetWindowTitle(g_window, "Magic: Deck Builder");
  SDL_SetRenderLogicalPresentation(g_renderer, 1024, 768,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
  SDL_StartTextInput(g_window);
  if (!shandalar_win32_imgui_init(g_window, g_renderer))
  {
    printf("shandalar: deckbuilder imgui init failed\n");
    return;
  }
  shandalar_win32_clear_quit_message();
  g_deckbuilder_host_active = 1;
  shandalar_win32_set_pump_proc(pump_deckbuilder_ui);
}

void shandalar_host_end_deckbuilder(void)
{
  g_deckbuilder_host_active = 0;
  shandalar_win32_set_pump_proc(pump_shandalar_win32_ui);
  shandalar_win32_clear_quit_message();
  shandalar_win32_imgui_shutdown();
  SDL_StopTextInput(g_window);
  SDL_StartTextInput(g_window);
  SDL_SetWindowTitle(g_window, "Magic: Shandalar");
  SDL_SetRenderLogicalPresentation(g_renderer, global_screen_width, global_screen_height,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
}

void shandalar_host_pump(void)
{
  SDL_Event event;
  float x;
  float y;
  while (SDL_PollEvent(&event))
  {
    switch (event.type)
    {
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
      g_mouse_button_down_mask |= shandalar_mouse_button_mask(event.button.button);
      break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
      x = event.button.x;
      y = event.button.y;
      SDL_RenderCoordinatesFromWindow(g_renderer, x, y, &x, &y);
      g_mouse_x = (int)x;
      g_mouse_y = (int)y;
      g_mouse_button_released_mask |= shandalar_mouse_button_mask(event.button.button);
      g_mouse_button_down_mask &= ~shandalar_mouse_button_mask(event.button.button);
      break;
    case SDL_EVENT_KEY_DOWN:
    {
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
  shandalar_win32_runtime_tick();
  shandalar_win32_mm_timers_tick();
  present_frame();
#ifdef __EMSCRIPTEN__
  wait_next_frame();
#else
  SDL_Delay(1);
#endif
}

static BOOL pump_shandalar_win32_ui(BOOL wait)
{
  (void)wait;
  shandalar_host_pump();
  return !g_quit_requested;
}

int main(int argc, char **argv)
{
  char cmd_line[1] = "";
  const char *asset_root = argc > 1 ? argv[1] : "C:/Magic/Program";
  int result;
#ifdef __EMSCRIPTEN__
  asset_root = argc > 1 ? argv[1] : "/assets";
#endif
  if (!SDL_Init(SDL_INIT_VIDEO))
    return 1;
  if (!shandalar_runtime_init(asset_root))
    return 1;
  global_screen_width = 0x400;
  global_screen_height = 0x300;
  if (!SDL_CreateWindowAndRenderer("Magic: Shandalar", global_screen_width,
                                   global_screen_height, SDL_WINDOW_RESIZABLE,
                                   &g_window, &g_renderer))
    return 1;
  SDL_SetRenderLogicalPresentation(g_renderer, global_screen_width, global_screen_height,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
  if (!ensure_framebuffer(global_screen_width, global_screen_height))
    return 1;
  statwin_set_renderer(g_renderer);
  SDL_StartTextInput(g_window);
  shandalar_win32_set_pump_proc(pump_shandalar_win32_ui);
  result = WinMain((HINSTANCE)1, (HINSTANCE)0, cmd_line, SW_SHOW);
  shandalar_win32_set_pump_proc(NULL);
  ShutdownGraphicsSystem();
  shandalar_win32_reset();
  shandalar_win32_mm_timers_reset();
  global_main_hdc = NULL;
  SDL_free(g_rgba);
  SDL_DestroyTexture(g_texture);
  statwin_set_renderer(NULL);
  SDL_DestroyRenderer(g_renderer);
  SDL_DestroyWindow(g_window);
  shandalar_runtime_shutdown();
  SDL_Quit();
  return result;
}
