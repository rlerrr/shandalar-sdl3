#ifndef SHANDALAR_WINUSER_SDL_COMPOSITOR_H
#define SHANDALAR_WINUSER_SDL_COMPOSITOR_H

#include "platform/winuser_compat.h"

typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef union SDL_Event SDL_Event;

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ShandalarWin32WindowTextureVisitor)(HWND window,
                                                   SDL_Texture *texture,
                                                   const RECT *window_rect,
                                                   int texture_width,
                                                   int texture_height,
                                                   void *userdata);

int shandalar_win32_sdl_compositor_init(SDL_Renderer *renderer);
int shandalar_win32_sdl_compositor_process_event(SDL_Renderer *renderer,
                                                  const SDL_Event *event);
void shandalar_win32_sdl_compositor_update(void);
void shandalar_win32_sdl_compositor_present(SDL_Renderer *renderer);
void shandalar_win32_sdl_compositor_visit_all(
    ShandalarWin32WindowTextureVisitor visitor, void *userdata);
void shandalar_win32_sdl_compositor_visit_subtree(
    HWND root, ShandalarWin32WindowTextureVisitor visitor, void *userdata);
SDL_Texture *shandalar_win32_sdl_compositor_texture(HWND window,
                                                     int *width,
                                                     int *height);
int shandalar_win32_is_builtin_control(HWND window);
void shandalar_win32_sdl_compositor_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
