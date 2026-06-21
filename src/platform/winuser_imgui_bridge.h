#ifndef SHANDALAR_WINUSER_IMGUI_BRIDGE_H
#define SHANDALAR_WINUSER_IMGUI_BRIDGE_H

typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Window SDL_Window;
typedef union SDL_Event SDL_Event;

#ifdef __cplusplus
extern "C" {
#endif

/* Called after ImGui::NewFrame() and before ImGui::Render(). */
int shandalar_win32_imgui_init(SDL_Window *window, SDL_Renderer *renderer);
int shandalar_win32_imgui_process_event(SDL_Renderer *renderer, const SDL_Event *event);
void shandalar_win32_imgui_begin_frame(void);
void shandalar_win32_imgui_present(SDL_Renderer *renderer);
void shandalar_win32_imgui_render(SDL_Renderer *renderer);
void shandalar_win32_imgui_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
