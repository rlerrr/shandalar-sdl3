#ifndef SHANDALAR_DECK_IMGUI_H
#define SHANDALAR_DECK_IMGUI_H

typedef union SDL_Event SDL_Event;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Window SDL_Window;

#ifdef __cplusplus
extern "C" {
#endif

int deck_imgui_init(SDL_Window *window, SDL_Renderer *renderer);
void deck_imgui_shutdown(void);
int deck_imgui_process_event(SDL_Renderer *renderer, const SDL_Event *event);
void deck_imgui_begin_frame(void);
void deck_imgui_render(SDL_Renderer *renderer);

#ifdef __cplusplus
}
#endif

#endif
