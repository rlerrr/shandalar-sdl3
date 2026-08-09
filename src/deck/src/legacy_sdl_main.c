#include <SDL3/SDL.h>

#include "deckdll/src/deckdll.h"
#include "deck/src/deck_imgui_interaction.h"
#include "cardartlib/CardArtLib.h"
#include "drawcardlib/Drawcardlib.h"
#include "platform/gdi_compat.h"
#include "platform/asset_manager.h"
#include "platform/winuser_compat.h"
#include "platform/winuser_imgui_bridge.h"

#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static SDL_Window *g_window;
static SDL_Renderer *g_renderer;
static int g_quit_requested;
static int g_smoke_frames_remaining;
static const char *g_snapshot_path;
static int g_smoke_add_card;
static int g_smoke_drag_card;
static int g_smoke_resize;
static int g_smoke_reverse;
static int g_smoke_double_card;
static int g_smoke_menu;
static int g_smoke_tooltip;
static int g_smoke_dialog_command;
static int g_smoke_dialog_open;
static int g_snapshot_saved;

extern HWND global_horzlist_hwnd;
extern HWND global_decksurface_hwnd;
extern HWND cardlistfilter_hwnd;
extern HWND deckdll_global_main_hwnd;

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

static void push_smoke_mouse_button(float logical_x, float logical_y, int down)
{
  SDL_Event event;
  float window_x;
  float window_y;
  if (!SDL_RenderCoordinatesToWindow(g_renderer, logical_x, logical_y,
                                     &window_x, &window_y))
    return;
  SDL_WarpMouseInWindow(g_window, window_x, window_y);
  SDL_zero(event);
  event.type = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
  event.button.windowID = SDL_GetWindowID(g_window);
  event.button.button = SDL_BUTTON_LEFT;
  event.button.down = down;
  event.button.x = window_x;
  event.button.y = window_y;
  SDL_PushEvent(&event);
}

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

#ifdef __EMSCRIPTEN__
EM_ASYNC_JS(void, wait_next_frame, (), {
  await new Promise(requestAnimationFrame);
});
#endif

static BOOL pump_legacy_ui(BOOL wait)
{
  SDL_Event event;
  static int first_frame = 1;
  if (g_smoke_dialog_open && g_smoke_frames_remaining == 3) {
    HWND dialog;
    for (dialog = GetWindow(deckdll_global_main_hwnd, GW_CHILD); dialog;
         dialog = GetWindow(dialog, GW_HWNDNEXT)) {
      char class_name[16] = {0};
      GetClassNameA(dialog, class_name, sizeof(class_name));
      if (SDL_strcmp(class_name, "#32770") == 0) {
        PostMessageA(dialog, WM_CLOSE, 0, 0);
        g_smoke_dialog_open = 0;
        break;
      }
    }
  }
  if (g_smoke_resize && g_smoke_frames_remaining == 28) {
    SDL_SetWindowSize(g_window, 1400, 900);
    g_smoke_resize = 0;
  }
  if (g_smoke_tooltip && g_smoke_frames_remaining == 90 && cardlistfilter_hwnd) {
    RECT rect;
    float window_x;
    float window_y;
    GetWindowRect(cardlistfilter_hwnd, &rect);
    if (SDL_RenderCoordinatesToWindow(g_renderer, (float)rect.left + 15.0f,
                                      (float)rect.top + 15.0f, &window_x, &window_y))
      SDL_WarpMouseInWindow(g_window, window_x, window_y);
    g_smoke_tooltip = 0;
  }
  if (g_smoke_reverse && g_smoke_frames_remaining == 18 && global_decksurface_hwnd) {
    HWND card = GetWindow(global_decksurface_hwnd, GW_CHILD);
    if (card) {
      RECT rect;
      GetWindowRect(card, &rect);
      if (g_smoke_reverse == 1) {
        shandalar_win32_set_cursor_position(rect.left + 20, rect.top + 20);
        SendMessageA(card, WM_LBUTTONDBLCLK, 0, MAKELPARAM(20, 20));
        g_smoke_reverse = 0;
      } else {
        push_smoke_mouse_button((float)rect.left + 20.0f, (float)rect.top + 20.0f, 1);
      }
    }
  } else if (g_smoke_reverse == 2 && g_smoke_frames_remaining == 17) {
    RECT rect;
    GetWindowRect(global_horzlist_hwnd, &rect);
    push_smoke_mouse_button((float)rect.left + 30.0f, (float)rect.top + 30.0f, 0);
    g_smoke_reverse = 0;
  }
  if (g_smoke_drag_card && g_smoke_frames_remaining == 25 && global_horzlist_hwnd) {
    RECT rect;
    GetWindowRect(global_horzlist_hwnd, &rect);
    push_smoke_mouse_button((float)rect.left + 30.0f, (float)rect.top + 30.0f, 1);
  } else if (g_smoke_drag_card && g_smoke_frames_remaining == 24) {
    push_smoke_mouse_button(500.0f, 250.0f, 0);
    g_smoke_drag_card = 0;
  }
  if (g_smoke_double_card && g_smoke_frames_remaining <= 25 &&
      g_smoke_frames_remaining >= 22 && global_horzlist_hwnd) {
    RECT rect;
    int down = (g_smoke_frames_remaining == 25 || g_smoke_frames_remaining == 23);
    GetWindowRect(global_horzlist_hwnd, &rect);
    push_smoke_mouse_button((float)rect.left + 30.0f, (float)rect.top + 30.0f, down);
    if (g_smoke_frames_remaining == 22)
      g_smoke_double_card = 0;
  }
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
  shandalar_deck_imgui_interaction_present();
  shandalar_win32_imgui_render(g_renderer);
  SDL_RenderPresent(g_renderer);
  if (first_frame) {
#ifdef __EMSCRIPTEN__
    emscripten_run_script("if (typeof Module !== 'undefined' && Module.setStatus) Module.setStatus('');");
#endif
    first_frame = 0;
  }

  if (g_smoke_dialog_open && g_smoke_frames_remaining == 10)
    save_snapshot();

  if (g_smoke_menu && g_smoke_frames_remaining == 20 && global_decksurface_hwnd) {
    RECT rect;
    GetWindowRect(global_decksurface_hwnd, &rect);
    shandalar_win32_set_cursor_position(rect.left + 100, rect.top + 100);
    SendMessageA(global_decksurface_hwnd, WM_RBUTTONDOWN, 0, MAKELPARAM(100, 100));
    g_smoke_menu = 0;
  }

  if (g_smoke_dialog_command && g_smoke_frames_remaining == 20 && deckdll_global_main_hwnd) {
    PostMessageA(deckdll_global_main_hwnd, WM_COMMAND, g_smoke_dialog_command, 0);
    g_smoke_dialog_command = 0;
    g_smoke_dialog_open = 1;
  }

  if (g_smoke_add_card && g_smoke_frames_remaining == 20 && global_horzlist_hwnd) {
    RECT rect;
    GetWindowRect(global_horzlist_hwnd, &rect);
    shandalar_win32_set_cursor_position(rect.left + 30, rect.top + 30);
    SendMessageA(global_horzlist_hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(30, 30));
    SendMessageA(global_horzlist_hwnd, WM_LBUTTONDBLCLK, 0, MAKELPARAM(30, 30));
    g_smoke_add_card = 0;
  }

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
    else if (SDL_strcmp(argv[i], "--smoke-add-card") == 0)
      g_smoke_add_card = 1;
    else if (SDL_strcmp(argv[i], "--smoke-drag-card") == 0)
      g_smoke_drag_card = 1;
    else if (SDL_strcmp(argv[i], "--smoke-resize") == 0)
      g_smoke_resize = 1;
    else if (SDL_strcmp(argv[i], "--smoke-double-card") == 0)
      g_smoke_double_card = 1;
    else if (SDL_strcmp(argv[i], "--smoke-menu") == 0)
      g_smoke_menu = 1;
    else if (SDL_strcmp(argv[i], "--smoke-stats") == 0)
      g_smoke_dialog_command = RES_MAINMENU_BUTTON_STATS;
    else if (SDL_strcmp(argv[i], "--smoke-deck-info") == 0)
      g_smoke_dialog_command = RES_MAINMENU_BUTTON_DECKINFO;
    else if (SDL_strcmp(argv[i], "--smoke-tooltip") == 0) {
      g_smoke_frames_remaining = 100;
      g_smoke_tooltip = 1;
    }
    else if (SDL_strcmp(argv[i], "--smoke-double-back") == 0) {
      g_smoke_add_card = 1;
      g_smoke_reverse = 1;
    } else if (SDL_strcmp(argv[i], "--smoke-drag-back") == 0) {
      g_smoke_add_card = 1;
      g_smoke_reverse = 2;
    }
    else
      asset_root = argv[i];
  }
  printf("deck: starting with asset root %s\n", asset_root);
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    printf("deck: SDL_Init failed: %s\n", SDL_GetError());
    return 1;
  }
  if (!SDL_CreateWindowAndRenderer("Magic: Deck Builder (legacy)", 1024, 768,
                                   SDL_WINDOW_RESIZABLE,
                                   &g_window, &g_renderer)) {
    printf("deck: SDL_CreateWindowAndRenderer failed: %s\n", SDL_GetError());
    return 1;
  }
  SDL_SetRenderLogicalPresentation(g_renderer, 1024, 768,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
  SDL_StartTextInput(g_window);
  if (!shandalar_win32_imgui_init(g_window, g_renderer)) {
    printf("deck: imgui init failed\n");
    return 1;
  }

  shandalar_assets_set_root(asset_root);
  SDL_snprintf(module_path, sizeof(module_path), "%s/deck.exe", asset_root);
  shandalar_win32_set_module_path(module_path);
  shandalar_set_default_font_root(asset_root);
  shandalar_win32_set_pump_proc(pump_legacy_ui);
  deck_register_dialog_resources();

  printf("deck: initializing card art and drawcard\n");
  if (!CardArtLib_InitializeFromRoot(asset_root)) {
    printf("deck: card art init failed\n");
    return 1;
  }
  printf("deck: card art init ok\n");
  if (!DrawCardLib_InitializeFromRoot(asset_root)) {
    printf("deck: drawcard init failed\n");
    return 1;
  }
  printf("deck: drawcard init ok\n");

  printf("deck: attaching deck dll\n");
  if (!DllMain((HINSTANCE)1, DLL_PROCESS_ATTACH, NULL)) {
    printf("deck: DllMain attach failed\n");
    return 1;
  }
  /* The standalone executable passes 2 for mode and 1 for editable input. */
  printf("deck: entering DeckBuilderMain\n");
  result = DeckBuilderMain(NULL, DBFLAGS_STANDALONE, 1);
  printf("deck: DeckBuilderMain returned %d\n", (int)result);
  DllMain((HINSTANCE)1, DLL_PROCESS_DETACH, NULL);
  DrawCardLib_Shutdown();
  CardArtLib_Shutdown();

  shandalar_deck_imgui_interaction_shutdown();
  shandalar_win32_imgui_shutdown();
  shandalar_win32_reset();
  SDL_StopTextInput(g_window);
  SDL_DestroyRenderer(g_renderer);
  SDL_DestroyWindow(g_window);
  SDL_Quit();
  return (int)result;
}
