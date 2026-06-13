#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "deck_core.h"
#include "deck_imgui.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#endif

#define DECK_RENDER_WIDTH 1280
#define DECK_RENDER_HEIGHT 960

typedef struct DeckApp {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  uint64_t presented_generation;
  int render_width;
  int render_height;
  int exit_notified;
} DeckApp;

static void notify_host_exit(DeckApp *app, int status)
{
  if (app && app->exit_notified)
    return;
  if (app)
    app->exit_notified = 1;
#ifdef __EMSCRIPTEN__
  EM_ASM({
    if (typeof window !== 'undefined' && window.parent) {
      window.parent.postMessage({ type: 'shandalar:exit', status: $0 }, window.location.origin);
    }
  }, status);
#else
  (void)status;
#endif
}

#ifdef __EMSCRIPTEN__
static int query_canvas_available_width(void)
{
  return EM_ASM_INT({
    var canvas = Module['canvas'] || document.getElementById('canvas') || document.querySelector('canvas');
    var parent = canvas && canvas.parentElement;
    var width = parent && parent.clientWidth ? parent.clientWidth : window.innerWidth;
    return Math.max(1, Math.floor(width || 1));
  });
}

static int query_canvas_available_height(void)
{
  return EM_ASM_INT({
    var canvas = Module['canvas'] || document.getElementById('canvas') || document.querySelector('canvas');
    var rect = canvas ? canvas.getBoundingClientRect() : { top: 0 };
    var windowHeight = window.innerHeight || document.documentElement.clientHeight || 1;
    var height = windowHeight - rect.top;
    return Math.max(1, Math.floor(height || 1));
  });
}

static void choose_canvas_render_size(int *width, int *height)
{
  int available_width = query_canvas_available_width();
  int available_height = query_canvas_available_height();
  if (available_width * 3 > available_height * 4) {
    *height = available_height;
    *width = available_height * 4 / 3;
  } else {
    *width = available_width;
    *height = available_width * 3 / 4;
  }
  if (*width < 320 || *height < 240) {
    *width = DECK_RENDER_WIDTH;
    *height = DECK_RENDER_HEIGHT;
  }
  *width &= ~3;
  *height &= ~3;
}

static void request_canvas_size(int width, int height)
{
  double dpr = emscripten_get_device_pixel_ratio();
  int pixel_width;
  int pixel_height;
  pixel_width = (int)((double)width * dpr);
  pixel_height = (int)((double)height * dpr);
  if (pixel_width < width)
    pixel_width = width;
  if (pixel_height < height)
    pixel_height = height;
  EM_ASM({
    var canvas = Module['canvas'] || document.getElementById('canvas') || document.querySelector('canvas');
    if (canvas) {
      canvas.width = $2;
      canvas.height = $3;
      canvas.style.width = $0 + 'px';
      canvas.style.height = $1 + 'px';
    }
  }, width, height, pixel_width, pixel_height);
}
#endif

static int recreate_render_target(DeckApp *app, int width, int height)
{
  SDL_Texture *texture;
  if (!app || width <= 0 || height <= 0)
    return 0;
  if (width == app->render_width && height == app->render_height)
    return 1;
  texture = SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STREAMING, width, height);
  if (!texture)
    return 0;
  SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
  if (!deck_resize(width, height)) {
    SDL_DestroyTexture(texture);
    return 0;
  }
  SDL_DestroyTexture(app->texture);
  app->texture = texture;
  app->render_width = width;
  app->render_height = height;
  app->presented_generation = 0;
  SDL_SetRenderLogicalPresentation(app->renderer, width, height,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
  SDL_SetWindowSize(app->window, width, height);
  return 1;
}

#ifdef __EMSCRIPTEN__
static int sync_wasm_canvas_size(DeckApp *app)
{
  int render_width;
  int render_height;
  if (!app)
    return 0;
  choose_canvas_render_size(&render_width, &render_height);
  if (render_width == app->render_width && render_height == app->render_height)
    return 1;
  request_canvas_size(render_width, render_height);
  return recreate_render_target(app, render_width, render_height);
}
#endif

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
  DeckApp *app;
  DeckConfig config;
  const char *asset_root = argc > 1 ? argv[1] : "C:/Magic/Program";
  int render_width = DECK_RENDER_WIDTH;
  int render_height = DECK_RENDER_HEIGHT;
#ifdef __EMSCRIPTEN__
  asset_root = argc > 1 ? argv[1] : "/assets";
  choose_canvas_render_size(&render_width, &render_height);
  request_canvas_size(render_width, render_height);
#endif
  if (!SDL_Init(SDL_INIT_VIDEO))
    return SDL_APP_FAILURE;
  app = (DeckApp *)SDL_calloc(1, sizeof(*app));
  if (!app)
    return SDL_APP_FAILURE;
  *appstate = app;
  app->render_width = render_width;
  app->render_height = render_height;
  if (!SDL_CreateWindowAndRenderer("Magic: Deck Builder", render_width, render_height,
                                   SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY,
                                   &app->window, &app->renderer))
    return SDL_APP_FAILURE;
  SDL_SetRenderLogicalPresentation(app->renderer, render_width, render_height,
                                   SDL_LOGICAL_PRESENTATION_LETTERBOX);
  app->texture = SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_ARGB8888,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   render_width, render_height);
  if (!app->texture)
    return SDL_APP_FAILURE;
  SDL_SetTextureScaleMode(app->texture, SDL_SCALEMODE_NEAREST);
  config.asset_root = asset_root;
  config.writable_root = ".";
  config.width = render_width;
  config.height = render_height;
  config.prefetch_cardart = 1;
  if (!deck_create(&config))
    return SDL_APP_FAILURE;
  if (!deck_imgui_init(app->window, app->renderer))
    return SDL_APP_FAILURE;
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  DeckApp *app = (DeckApp *)appstate;
  float x;
  float y;
  int imgui_consumed;
#ifndef __EMSCRIPTEN__
  int render_width;
  int render_height;
#endif
  if (event->type != SDL_EVENT_QUIT) {
    imgui_consumed = deck_imgui_process_event(app->renderer, event);
    if (imgui_consumed)
      return SDL_APP_CONTINUE;
  }
  switch (event->type) {
  case SDL_EVENT_QUIT:
    notify_host_exit(app, 0);
    return SDL_APP_SUCCESS;
  case SDL_EVENT_WINDOW_RESIZED:
#ifndef __EMSCRIPTEN__
    render_width = event->window.data1;
    render_height = event->window.data2;
    if (render_width * 3 > render_height * 4) {
      render_width = render_height * 4 / 3;
    } else {
      render_height = render_width * 3 / 4;
    }
    render_width &= ~3;
    render_height &= ~3;
    if (!recreate_render_target(app, render_width, render_height))
      return SDL_APP_FAILURE;
#endif
    break;
  case SDL_EVENT_MOUSE_MOTION:
    x = event->motion.x;
    y = event->motion.y;
    SDL_RenderCoordinatesFromWindow(app->renderer, x, y, &x, &y);
    deck_pointer_move(x, y);
    break;
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
  case SDL_EVENT_MOUSE_BUTTON_UP:
    x = event->button.x;
    y = event->button.y;
    SDL_RenderCoordinatesFromWindow(app->renderer, x, y, &x, &y);
    deck_pointer_move(x, y);
    deck_pointer_button(event->button.button, event->type == SDL_EVENT_MOUSE_BUTTON_DOWN,
                        event->button.clicks);
    break;
  case SDL_EVENT_KEY_DOWN:
  case SDL_EVENT_KEY_UP:
    deck_key((int)event->key.key, event->type == SDL_EVENT_KEY_DOWN, event->key.repeat);
    break;
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
  DeckApp *app = (DeckApp *)appstate;
  DeckFrame frame;
#ifdef __EMSCRIPTEN__
  if (!sync_wasm_canvas_size(app))
    return SDL_APP_FAILURE;
#endif
  DeckStatus status = deck_step(SDL_GetTicksNS());
  const char *status_text = deck_status_text();
  if (status_text && status_text[0])
    SDL_SetWindowTitle(app->window, status_text);
  if (deck_get_frame(&frame) && frame.generation != app->presented_generation) {
    SDL_UpdateTexture(app->texture, NULL, frame.pixels, frame.pitch);
    app->presented_generation = frame.generation;
  }
  SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
  SDL_RenderClear(app->renderer);
  SDL_RenderTexture(app->renderer, app->texture, NULL, NULL);
  deck_imgui_begin_frame();
  deck_imgui_render(app->renderer);
  SDL_RenderPresent(app->renderer);
  if (status == DECK_FAILED)
    return SDL_APP_FAILURE;
  if (status == DECK_COMPLETED) {
    notify_host_exit(app, 0);
    return SDL_APP_SUCCESS;
  }
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
  DeckApp *app = (DeckApp *)appstate;
  (void)result;
  deck_imgui_shutdown();
  deck_destroy();
  if (app) {
    SDL_DestroyTexture(app->texture);
    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow(app->window);
    SDL_free(app);
  }
}
