#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "facemaker_core.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

typedef struct FacemakerApp {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  uint32_t *rgba;
  uint64_t presented_generation;
  int exit_notified;
} FacemakerApp;

static void notify_host_exit(FacemakerApp *app, int status)
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

static void convert_frame(FacemakerApp *app, const FacemakerFrame *frame)
{
  int x;
  int y;
  for (y = 0; y < frame->height; ++y) {
    const unsigned char *source = frame->pixels + y * frame->pitch;
    uint32_t *target = app->rgba + y * frame->width;
    for (x = 0; x < frame->width; ++x) {
      const unsigned char *rgb = frame->rgb_palette + source[x] * 3;
      target[x] = 0xff000000u | ((uint32_t)rgb[0] << 16) | ((uint32_t)rgb[1] << 8) | rgb[2];
    }
  }
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
  FacemakerApp *app;
  FacemakerConfig config;
  const char *asset_root = argc > 1 ? argv[1] : "C:/Magic/Program";
#ifdef __EMSCRIPTEN__
  asset_root = argc > 1 ? argv[1] : "/assets";
#endif
  if (!SDL_Init(SDL_INIT_VIDEO))
    return SDL_APP_FAILURE;
  app = (FacemakerApp *)SDL_calloc(1, sizeof(*app));
  if (!app)
    return SDL_APP_FAILURE;
  *appstate = app;
  if (!SDL_CreateWindowAndRenderer("Magic: Facemaker", 1024, 768, SDL_WINDOW_RESIZABLE,
                                    &app->window, &app->renderer))
    return SDL_APP_FAILURE;
  SDL_SetRenderLogicalPresentation(app->renderer, 1024, 768, SDL_LOGICAL_PRESENTATION_LETTERBOX);
  app->texture = SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_ARGB8888,
                                   SDL_TEXTUREACCESS_STREAMING, 1024, 768);
  SDL_SetTextureScaleMode(app->texture, SDL_SCALEMODE_NEAREST);
  app->rgba = (uint32_t *)SDL_malloc(1024u * 768u * sizeof(uint32_t));
  config.asset_root = asset_root;
  config.writable_root = ".";
  config.width = 1024;
  config.height = 768;
  config.download_on_save = 1;
  if (!app->texture || !app->rgba || !facemaker_create(&config))
    return SDL_APP_FAILURE;
  *appstate = app;
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  FacemakerApp *app = (FacemakerApp *)appstate;
  float x;
  float y;
  switch (event->type) {
  case SDL_EVENT_QUIT:
    notify_host_exit(app, 0);
    return SDL_APP_SUCCESS;
  case SDL_EVENT_MOUSE_MOTION:
    x = event->motion.x;
    y = event->motion.y;
    SDL_RenderCoordinatesFromWindow(app->renderer, x, y, &x, &y);
    facemaker_pointer_move(x, y);
    break;
  case SDL_EVENT_MOUSE_BUTTON_DOWN:
  case SDL_EVENT_MOUSE_BUTTON_UP:
    x = event->button.x;
    y = event->button.y;
    SDL_RenderCoordinatesFromWindow(app->renderer, x, y, &x, &y);
    facemaker_pointer_move(x, y);
    facemaker_pointer_button(event->button.button, event->type == SDL_EVENT_MOUSE_BUTTON_DOWN);
    break;
  case SDL_EVENT_KEY_DOWN:
  case SDL_EVENT_KEY_UP:
    facemaker_key((int)event->key.key, event->type == SDL_EVENT_KEY_DOWN, event->key.repeat);
    break;
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
  FacemakerApp *app = (FacemakerApp *)appstate;
  FacemakerFrame frame;
  FacemakerStatus status = facemaker_step(SDL_GetTicksNS());
  if (facemaker_get_frame(&frame) && frame.generation != app->presented_generation) {
    convert_frame(app, &frame);
    SDL_UpdateTexture(app->texture, NULL, app->rgba, frame.width * (int)sizeof(uint32_t));
    app->presented_generation = frame.generation;
  }
  SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 255);
  SDL_RenderClear(app->renderer);
  SDL_RenderTexture(app->renderer, app->texture, NULL, NULL);
  SDL_RenderPresent(app->renderer);
  if (status == FACEMAKER_FAILED)
    return SDL_APP_FAILURE;
  if (status == FACEMAKER_COMPLETED || status == FACEMAKER_CANCELLED) {
    notify_host_exit(app, status == FACEMAKER_COMPLETED ? 0 : 1);
    return SDL_APP_SUCCESS;
  }
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
  FacemakerApp *app = (FacemakerApp *)appstate;
  (void)result;
  facemaker_destroy();
  if (app) {
    SDL_DestroyTexture(app->texture);
    SDL_DestroyRenderer(app->renderer);
    SDL_DestroyWindow(app->window);
    SDL_free(app->rgba);
    SDL_free(app);
  }
}
