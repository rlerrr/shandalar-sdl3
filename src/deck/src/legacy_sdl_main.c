#include <SDL3/SDL.h>

#include "deckdll/src/deckdll.h"
#include "cardartlib/CardArtLib.h"
#include "drawcardlib/Drawcardlib.h"
#include "platform/gdi_compat.h"
#include "platform/winuser_compat.h"
#include "platform/winuser_imgui_bridge.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static SDL_Window *g_window;
static SDL_Renderer *g_renderer;
static int g_quit_requested;
static int g_smoke_frames_remaining;
static const char *g_snapshot_path;

static UINT virtual_key(SDL_Keycode key)
{
  switch (key) {
  case SDLK_LEFT: return VK_LEFT;
  case SDLK_RIGHT: return VK_RIGHT;
  case SDLK_UP: return VK_UP;
  case SDLK_DOWN: return VK_DOWN;
  case SDLK_HOME: return VK_HOME;
  case SDLK_END: return VK_END;
  case SDLK_PAGEUP: return VK_PRIOR;
  case SDLK_PAGEDOWN: return VK_NEXT;
  case SDLK_RETURN: return 0x0d;
  case SDLK_ESCAPE: return 0x1b;
  case SDLK_BACKSPACE: return 0x08;
  case SDLK_DELETE: return 0x2e;
  default:
    return key >= 0 && key <= 0xff ? (UINT)key : 0;
  }
}

static void forward_keyboard_event(const SDL_Event *event)
{
  HWND focus = GetFocus();
  char class_name[32] = {0};
  if (!focus)
    return;
  GetClassNameA(focus, class_name, sizeof(class_name));
  if (SDL_strcasecmp(class_name, "EDIT") == 0 ||
      SDL_strcasecmp(class_name, "LISTBOX") == 0 ||
      SDL_strcasecmp(class_name, "COMBOBOX") == 0)
    return;
  if (event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP) {
    UINT key = virtual_key(event->key.key);
    if (key)
      PostMessageA(focus, event->type == SDL_EVENT_KEY_DOWN ? WM_KEYDOWN : WM_KEYUP,
                   key, 0);
  } else if (event->type == SDL_EVENT_TEXT_INPUT && event->text.text) {
    const unsigned char *text = (const unsigned char *)event->text.text;
    while (*text)
      PostMessageA(focus, WM_CHAR, *text++, 0);
  }
}

static BOOL pump_legacy_ui(BOOL wait)
{
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    shandalar_win32_imgui_process_event(g_renderer, &event);
    if (event.type == SDL_EVENT_QUIT) {
      g_quit_requested = 1;
      PostQuitMessage(0);
    } else {
      forward_keyboard_event(&event);
    }
  }

  SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
  SDL_RenderClear(g_renderer);
  shandalar_win32_imgui_begin_frame();
  shandalar_win32_imgui_present(g_renderer);
  shandalar_win32_imgui_render(g_renderer);
  SDL_RenderPresent(g_renderer);

  if (g_smoke_frames_remaining > 0 && --g_smoke_frames_remaining == 0) {
    if (g_snapshot_path) {
      SDL_Surface *surface = SDL_RenderReadPixels(g_renderer, NULL);
      if (surface) {
        SDL_SaveBMP(surface, g_snapshot_path);
        SDL_DestroySurface(surface);
      }
    }
    g_quit_requested = 1;
    PostQuitMessage(0);
  }

#ifdef __EMSCRIPTEN__
  emscripten_sleep(wait ? 8 : 0);
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
  int i;
  WPARAM result;
#ifdef __EMSCRIPTEN__
  asset_root = "/assets";
#endif
  for (i = 1; i < argc; ++i) {
    if (SDL_strcmp(argv[i], "--smoke") == 0)
      g_smoke_frames_remaining = 30;
    else if (SDL_strncmp(argv[i], "--snapshot=", 11) == 0)
      g_snapshot_path = argv[i] + 11;
    else
      asset_root = argv[i];
  }
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    return 1;
  if (!SDL_CreateWindowAndRenderer("Magic: Deck Builder (legacy)", 1024, 768,
                                   SDL_WINDOW_RESIZABLE,
                                   &g_window, &g_renderer))
    return 1;
  SDL_SetRenderLogicalPresentation(g_renderer, 1024, 768,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
  SDL_StartTextInput(g_window);
  if (!shandalar_win32_imgui_init(g_window, g_renderer))
    return 1;

  SDL_snprintf(module_path, sizeof(module_path), "%s/deck_legacy.exe", asset_root);
  shandalar_win32_set_module_path(module_path);
  shandalar_set_default_font_root(asset_root);
  shandalar_win32_set_pump_proc(pump_legacy_ui);
  deck_register_dialog_resources();

  if (!CardArtLib_InitializeFromRoot(asset_root) ||
      !DrawCardLib_InitializeFromRoot(asset_root))
    return 1;

  if (!DllMain((HINSTANCE)1, DLL_PROCESS_ATTACH, NULL))
    return 1;
  result = DeckBuilderMain(NULL, DBFLAGS_STANDALONE, DBFLAGS_STANDALONE);
  DllMain((HINSTANCE)1, DLL_PROCESS_DETACH, NULL);
  DrawCardLib_Shutdown();
  CardArtLib_Shutdown();

  shandalar_win32_imgui_shutdown();
  shandalar_win32_reset();
  SDL_StopTextInput(g_window);
  SDL_DestroyRenderer(g_renderer);
  SDL_DestroyWindow(g_window);
  SDL_Quit();
  return (int)result;
}
