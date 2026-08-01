#include "platform/winuser_sdl_compositor.h"

#include "platform/gdi_compat.h"

#include <SDL3/SDL.h>

#include <stdint.h>
#include <stdlib.h>

typedef struct WindowTexture {
  struct WindowTexture *next;
  HWND window;
  SDL_Texture *texture;
  int width;
  int height;
  uint32_t *pixels;
} WindowTexture;

static SDL_Renderer *g_renderer;
static WindowTexture *g_textures;
static WPARAM g_mouse_buttons;

static WindowTexture *find_texture(HWND window)
{
  WindowTexture *entry;
  for (entry = g_textures; entry; entry = entry->next) {
    if (entry->window == window)
      return entry;
  }
  entry = (WindowTexture *)calloc(1, sizeof(*entry));
  if (!entry)
    return NULL;
  entry->window = window;
  entry->next = g_textures;
  g_textures = entry;
  return entry;
}

int shandalar_win32_is_builtin_control(HWND window)
{
  char name[32] = {0};
  GetClassNameA(window, name, sizeof(name));
  return SDL_strcasecmp(name, "BUTTON") == 0 ||
         SDL_strcasecmp(name, "EDIT") == 0 ||
         SDL_strcasecmp(name, "STATIC") == 0 ||
         SDL_strcasecmp(name, "LISTBOX") == 0 ||
         SDL_strcasecmp(name, "COMBOBOX") == 0 ||
         SDL_strcasecmp(name, "SCROLLBAR") == 0;
}

static int is_descendant_or_self(HWND window, HWND ancestor)
{
  while (window) {
    if (window == ancestor)
      return 1;
    window = GetParent(window);
  }
  return 0;
}

static SDL_Texture *update_window_texture(HWND window, int *out_width,
                                          int *out_height)
{
  RECT rect;
  HDC dc;
  WindowTexture *entry;
  int width;
  int height;

  if (!window || !IsWindowVisible(window) ||
      shandalar_win32_is_builtin_control(window))
    return NULL;
  if (!GetWindowRect(window, &rect))
    return NULL;
  width = rect.right - rect.left;
  height = rect.bottom - rect.top;
  if (width <= 0 || height <= 0)
    return NULL;
  dc = shandalar_window_backing_dc(window);
  if (!dc)
    return NULL;

  UpdateWindow(window);
  entry = find_texture(window);
  if (!entry)
    return NULL;
  if (!entry->texture || entry->width != width || entry->height != height) {
    SDL_DestroyTexture(entry->texture);
    free(entry->pixels);
    entry->texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_ARGB8888,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       width, height);
    entry->pixels = (uint32_t *)malloc((size_t)width * (size_t)height *
                                       sizeof(*entry->pixels));
    entry->width = width;
    entry->height = height;
  }
  if (!entry->texture || !entry->pixels)
    return NULL;
  if (!shandalar_dc_copy_rgba(dc, entry->pixels,
                              width * (int)sizeof(*entry->pixels)))
    return NULL;
  SDL_UpdateTexture(entry->texture, NULL, entry->pixels,
                    width * (int)sizeof(*entry->pixels));
  if (out_width)
    *out_width = width;
  if (out_height)
    *out_height = height;
  return entry->texture;
}

static void visit_window_tree(HWND window,
                              ShandalarWin32WindowTextureVisitor visitor,
                              void *userdata)
{
  RECT rect;
  SDL_Texture *texture;
  HWND child;
  int width;
  int height;

  if (!window || !IsWindowVisible(window))
    return;
  texture = update_window_texture(window, &width, &height);
  if (texture && GetWindowRect(window, &rect))
    visitor(window, texture, &rect, width, height, userdata);

  child = GetWindow(window, GW_CHILD);
  if (child)
    child = GetWindow(child, GW_HWNDLAST);
  for (; child; child = GetWindow(child, GW_HWNDPREV))
    visit_window_tree(child, visitor, userdata);
}

int shandalar_win32_sdl_compositor_init(SDL_Renderer *renderer)
{
  if (!renderer)
    return 0;
  g_renderer = renderer;
  g_mouse_buttons = 0;
  return 1;
}

static void update_only(HWND window, SDL_Texture *texture,
                        const RECT *rect, int width, int height,
                        void *userdata)
{
  (void)window;
  (void)texture;
  (void)rect;
  (void)width;
  (void)height;
  (void)userdata;
}

void shandalar_win32_sdl_compositor_update(void)
{
  HWND window;
  HWND desktop;
  if (!g_renderer)
    return;
  desktop = GetDesktopWindow();
  window = GetWindow(desktop, GW_CHILD);
  if (window)
    window = GetWindow(window, GW_HWNDLAST);
  for (; window; window = GetWindow(window, GW_HWNDPREV))
    visit_window_tree(window, update_only, NULL);
}

static void render_texture(HWND window, SDL_Texture *texture,
                           const RECT *rect, int width, int height,
                           void *userdata)
{
  SDL_Renderer *renderer = (SDL_Renderer *)userdata;
  SDL_FRect destination;
  (void)window;
  (void)width;
  (void)height;
  destination.x = (float)rect->left;
  destination.y = (float)rect->top;
  destination.w = (float)(rect->right - rect->left);
  destination.h = (float)(rect->bottom - rect->top);
  SDL_RenderTexture(renderer, texture, NULL, &destination);
}

void shandalar_win32_sdl_compositor_present(SDL_Renderer *renderer)
{
  shandalar_win32_sdl_compositor_visit_all(render_texture, renderer);
}

void shandalar_win32_sdl_compositor_visit_all(
    ShandalarWin32WindowTextureVisitor visitor, void *userdata)
{
  HWND window;
  HWND desktop;
  if (!g_renderer || !visitor)
    return;
  desktop = GetDesktopWindow();
  window = GetWindow(desktop, GW_CHILD);
  if (window)
    window = GetWindow(window, GW_HWNDLAST);
  for (; window; window = GetWindow(window, GW_HWNDPREV))
    visit_window_tree(window, visitor, userdata);
}

void shandalar_win32_sdl_compositor_visit_subtree(
    HWND root, ShandalarWin32WindowTextureVisitor visitor, void *userdata)
{
  if (!g_renderer || !visitor)
    return;
  visit_window_tree(root, visitor, userdata);
}

SDL_Texture *shandalar_win32_sdl_compositor_texture(HWND window,
                                                     int *width,
                                                     int *height)
{
  WindowTexture *entry;
  if (!window)
    return NULL;
  entry = find_texture(window);
  if (!entry)
    return NULL;
  if (!entry->texture)
    return update_window_texture(window, width, height);
  if (width)
    *width = entry->width;
  if (height)
    *height = entry->height;
  return entry->texture;
}

static int event_mouse_position(SDL_Renderer *renderer, const SDL_Event *event,
                                float *x, float *y)
{
  float window_x;
  float window_y;
  if (event->type == SDL_EVENT_MOUSE_MOTION) {
    window_x = event->motion.x;
    window_y = event->motion.y;
  } else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
             event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
    window_x = event->button.x;
    window_y = event->button.y;
  } else {
    SDL_GetMouseState(&window_x, &window_y);
  }
  return SDL_RenderCoordinatesFromWindow(renderer, window_x, window_y, x, y);
}

int shandalar_win32_sdl_compositor_process_event(SDL_Renderer *renderer,
                                                  const SDL_Event *event)
{
  HWND target;
  HWND modal;
  POINT screen;
  POINT client;
  LPARAM local;
  float x;
  float y;
  UINT message;

  if (!renderer || !event)
    return 0;
  if (event->type != SDL_EVENT_MOUSE_MOTION &&
      event->type != SDL_EVENT_MOUSE_BUTTON_DOWN &&
      event->type != SDL_EVENT_MOUSE_BUTTON_UP &&
      event->type != SDL_EVENT_MOUSE_WHEEL)
    return 0;
  if (!event_mouse_position(renderer, event, &x, &y))
    return 0;

  screen.x = (LONG)x;
  screen.y = (LONG)y;
  shandalar_win32_set_cursor_position(screen.x, screen.y);
  target = WindowFromPoint(screen);
  if (!target || target == GetDesktopWindow())
    return 0;
  modal = shandalar_win32_active_dialog();
  if (modal && !is_descendant_or_self(target, modal)) {
    BringWindowToTop(modal);
    return 1;
  }
  if (shandalar_win32_is_builtin_control(target))
    return 0;

  client = screen;
  ScreenToClient(target, &client);
  local = MAKELPARAM(client.x, client.y);
  if (event->type == SDL_EVENT_MOUSE_MOTION) {
    PostMessageA(target, WM_MOUSEMOVE, g_mouse_buttons, local);
    return 1;
  }
  if (event->type == SDL_EVENT_MOUSE_WHEEL) {
    WPARAM wheel = MAKEWPARAM(g_mouse_buttons,
                              (int)(event->wheel.y * 120.0f));
    SendMessageA(target, WM_MOUSEWHEEL, wheel,
                 MAKELPARAM(screen.x, screen.y));
    return 1;
  }

  if (event->button.button == SDL_BUTTON_LEFT) {
    message = event->type == SDL_EVENT_MOUSE_BUTTON_DOWN
                  ? (event->button.clicks > 1 &&
                             shandalar_win32_accepts_double_click(target)
                         ? WM_LBUTTONDBLCLK
                         : WM_LBUTTONDOWN)
                  : WM_LBUTTONUP;
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
      g_mouse_buttons |= MK_LBUTTON;
    else
      g_mouse_buttons &= ~MK_LBUTTON;
  } else if (event->button.button == SDL_BUTTON_RIGHT) {
    message = event->type == SDL_EVENT_MOUSE_BUTTON_DOWN
                  ? (event->button.clicks > 1 &&
                             shandalar_win32_accepts_double_click(target)
                         ? WM_RBUTTONDBLCLK
                         : WM_RBUTTONDOWN)
                  : WM_RBUTTONUP;
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
      g_mouse_buttons |= MK_RBUTTON;
    else
      g_mouse_buttons &= ~MK_RBUTTON;
  } else {
    return 0;
  }
  SendMessageA(target, message, g_mouse_buttons, local);
  return 1;
}

void shandalar_win32_sdl_compositor_shutdown(void)
{
  while (g_textures) {
    WindowTexture *next = g_textures->next;
    SDL_DestroyTexture(g_textures->texture);
    free(g_textures->pixels);
    free(g_textures);
    g_textures = next;
  }
  g_renderer = NULL;
  g_mouse_buttons = 0;
}
