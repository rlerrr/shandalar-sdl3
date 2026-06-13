#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "deck_core.h"
#include "deck_imgui.h"

typedef struct DeckApp {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  uint64_t presented_generation;
} DeckApp;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
  DeckApp *app;
  DeckConfig config;
  const char *asset_root = argc > 1 ? argv[1] : "C:/Magic/Program";
#ifdef __EMSCRIPTEN__
  asset_root = argc > 1 ? argv[1] : "/assets";
#endif
  if (!SDL_Init(SDL_INIT_VIDEO))
    return SDL_APP_FAILURE;
  app = (DeckApp *)SDL_calloc(1, sizeof(*app));
  if (!app)
    return SDL_APP_FAILURE;
  *appstate = app;
  if (!SDL_CreateWindowAndRenderer("Magic: Deck Builder", 1024, 768, SDL_WINDOW_RESIZABLE,
                                   &app->window, &app->renderer))
    return SDL_APP_FAILURE;
  SDL_SetRenderLogicalPresentation(app->renderer, 1024, 768, SDL_LOGICAL_PRESENTATION_LETTERBOX);
  app->texture = SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_ARGB8888,
                                   SDL_TEXTUREACCESS_STREAMING, 1024, 768);
  if (!app->texture)
    return SDL_APP_FAILURE;
  SDL_SetTextureScaleMode(app->texture, SDL_SCALEMODE_NEAREST);
  config.asset_root = asset_root;
  config.writable_root = ".";
  config.width = 1024;
  config.height = 768;
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
  if (event->type != SDL_EVENT_QUIT) {
    imgui_consumed = deck_imgui_process_event(app->renderer, event);
    if (imgui_consumed)
      return SDL_APP_CONTINUE;
  }
  switch (event->type) {
  case SDL_EVENT_QUIT:
    return SDL_APP_SUCCESS;
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
  if (status == DECK_COMPLETED)
    return SDL_APP_SUCCESS;
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
