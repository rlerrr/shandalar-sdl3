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
  uint64_t content_revision;
  uint32_t *pixels;
} WindowTexture;

static SDL_Renderer *g_renderer;
static WindowTexture *g_textures;
static WPARAM g_mouse_buttons;

typedef struct CardListDragState {
  HWND initial_target;
  HWND scroll_target;
  POINT start;
  POINT last;
  int dragging;
} CardListDragState;

static CardListDragState g_card_list_drag;

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

static int is_parent_drawn_control(HWND window)
{
  char name[64] = {0};
  GetClassNameA(window, name, sizeof(name));
  return SDL_strcasecmp(name, "MAGICTHEME_IconButtonClass") == 0;
}

static SDL_Texture *update_window_texture(HWND window, int *out_width,
                                          int *out_height,
                                          int include_builtin_control)
{
  RECT rect;
  HDC dc;
  WindowTexture *entry;
  int width;
  int height;
  uint64_t content_revision;

  if (!window || !IsWindowVisible(window) || is_parent_drawn_control(window) ||
      (!include_builtin_control && shandalar_win32_is_builtin_control(window)))
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
  content_revision = shandalar_window_content_revision(window);
  entry = find_texture(window);
  if (!entry)
    return NULL;
  if (entry->texture && entry->width == width && entry->height == height &&
      entry->content_revision == content_revision) {
    if (out_width)
      *out_width = width;
    if (out_height)
      *out_height = height;
    return entry->texture;
  }
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
  entry->content_revision = content_revision;
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
  texture = update_window_texture(window, &width, &height, 0);
  if (texture && GetWindowRect(window, &rect))
    visitor(window, texture, &rect, width, height, userdata);

  child = GetWindow(window, GW_CHILD);
  if (child)
    child = GetWindow(child, GW_HWNDLAST);
  for (; child; child = GetWindow(child, GW_HWNDPREV))
    visit_window_tree(child, visitor, userdata);
}

static void visit_popups_of_hidden_owners(
    HWND owner, ShandalarWin32WindowTextureVisitor visitor, void *userdata)
{
  HWND child = GetWindow(owner, GW_CHILD);
  if (child)
    child = GetWindow(child, GW_HWNDLAST);
  for (; child; child = GetWindow(child, GW_HWNDPREV)) {
    if (IsWindowVisible(child) && !IsWindowVisible(owner) &&
        (GetWindowLongA(child, GWL_STYLE) & WS_POPUP) == WS_POPUP) {
      /* Draw independently visible owned popups after the normal tree.  In
       * particular, minimized combat/spell buttons must sit above the phase
       * bar even though the full window which owns them is hidden. */
      visit_window_tree(child, visitor, userdata);
    } else {
      visit_popups_of_hidden_owners(child, visitor, userdata);
    }
  }
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
  visit_popups_of_hidden_owners(desktop, update_only, NULL);
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
  visit_popups_of_hidden_owners(desktop, visitor, userdata);
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
    return update_window_texture(window, width, height, 0);
  if (width)
    *width = entry->width;
  if (height)
    *height = entry->height;
  return entry->texture;
}

SDL_Texture *shandalar_win32_sdl_compositor_control_texture(HWND window,
                                                             int *width,
                                                             int *height)
{
  return update_window_texture(window, width, height, 1);
}

void shandalar_win32_sdl_compositor_forget_window(HWND window)
{
  WindowTexture **link = &g_textures;

  while (*link) {
    WindowTexture *entry = *link;
    if (entry->window == window) {
      *link = entry->next;
      SDL_DestroyTexture(entry->texture);
      free(entry->pixels);
      free(entry);
      return;
    }
    link = &entry->next;
  }
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

/* Scrollbars are non-client controls rendered by the ImGui bridge.  Do not
 * also deliver their mouse events to the legacy client window underneath. */
static int point_in_rendered_scrollbar(HWND window, POINT point)
{
  while (window && window != GetDesktopWindow()) {
    RECT client;
    POINT origin = {0, 0};
    LONG style = GetWindowLongA(window, GWL_STYLE);
    int minimum;
    int maximum;
    if (GetClientRect(window, &client) && ClientToScreen(window, &origin)) {
      if ((style & WS_HSCROLL) &&
          GetScrollRange(window, SB_HORZ, &minimum, &maximum) &&
          maximum > minimum &&
          point.x >= origin.x && point.x < origin.x + client.right &&
          point.y >= origin.y + client.bottom &&
          point.y < origin.y + client.bottom + GetSystemMetrics(SM_CYHSCROLL))
        return 1;
      if ((style & WS_VSCROLL) &&
          GetScrollRange(window, SB_VERT, &minimum, &maximum) &&
          maximum > minimum &&
          point.x >= origin.x + client.right &&
          point.x < origin.x + client.right + GetSystemMetrics(SM_CXVSCROLL) &&
          point.y >= origin.y && point.y < origin.y + client.bottom)
        return 1;
    }
    window = GetParent(window);
  }
  return 0;
}

/* Captions are rendered and interacted with by the ImGui bridge. */
static int point_in_rendered_caption(HWND window, POINT point)
{
  while (window && window != GetDesktopWindow()) {
    RECT rect;
    if (shandalar_win32_should_render_caption(window) &&
        GetWindowRect(window, &rect) &&
        point.x >= rect.left && point.x < rect.right &&
        point.y >= rect.top &&
        point.y < rect.top + GetSystemMetrics(SM_CYCAPTION))
      return 1;
    window = GetParent(window);
  }
  return 0;
}

static int window_class_is(HWND window, const char *wanted)
{
  char name[32] = {0};
  return window && GetClassNameA(window, name, sizeof(name)) &&
         SDL_strcasecmp(name, wanted) == 0;
}

static int has_show_list_card_child(HWND window)
{
  HWND child = GetWindow(window, GW_CHILD);
  for (; child; child = GetWindow(child, GW_HWNDNEXT))
    if (window_class_is(child, "ShowListCard"))
      return 1;
  return 0;
}

static HWND find_scrollable_card_list_dialog(HWND window)
{
  int saw_card = 0;
  while (window && window != GetDesktopWindow()) {
    int minimum;
    int maximum;
    if (window_class_is(window, "ShowListCard"))
      saw_card = 1;
    if (window_class_is(window, "#32770") &&
        (GetWindowLongA(window, GWL_STYLE) & WS_VSCROLL) &&
        GetScrollRange(window, SB_VERT, &minimum, &maximum) &&
        maximum > minimum && (saw_card || has_show_list_card_child(window)))
      return window;
    window = GetParent(window);
  }
  return NULL;
}

static int continue_card_list_drag(const SDL_Event *event, POINT screen)
{
  const int drag_threshold = 6;
  int minimum;
  int maximum;
  int position;
  int next_position;
  int delta_y;

  if (!g_card_list_drag.initial_target)
    return 0;
  if (!IsWindow(g_card_list_drag.initial_target) ||
      !IsWindow(g_card_list_drag.scroll_target)) {
    SDL_zero(g_card_list_drag);
    return 0;
  }
  if (event->type == SDL_EVENT_MOUSE_MOTION) {
    if (!g_card_list_drag.dragging &&
        (abs(screen.x - g_card_list_drag.start.x) > drag_threshold ||
         abs(screen.y - g_card_list_drag.start.y) > drag_threshold))
      g_card_list_drag.dragging = 1;
    if (g_card_list_drag.dragging) {
      delta_y = screen.y - g_card_list_drag.last.y;
      if (delta_y != 0 &&
          GetScrollRange(g_card_list_drag.scroll_target, SB_VERT,
                         &minimum, &maximum)) {
        position = GetScrollPos(g_card_list_drag.scroll_target, SB_VERT);
        next_position = position - delta_y;
        if (next_position < minimum)
          next_position = minimum;
        if (next_position > maximum)
          next_position = maximum;
        SendMessageA(g_card_list_drag.scroll_target, WM_VSCROLL,
                     MAKEWPARAM(SB_THUMBPOSITION, next_position), 0);
      }
      g_card_list_drag.last = screen;
    }
    return 1;
  }
  if (event->type == SDL_EVENT_MOUSE_BUTTON_UP &&
      event->button.button == SDL_BUTTON_LEFT) {
    g_mouse_buttons &= ~MK_LBUTTON;
    if (!g_card_list_drag.dragging) {
      POINT down = g_card_list_drag.start;
      POINT up = screen;
      ScreenToClient(g_card_list_drag.initial_target, &down);
      ScreenToClient(g_card_list_drag.initial_target, &up);
      PostMessageA(g_card_list_drag.initial_target, WM_LBUTTONDOWN,
                   MK_LBUTTON, MAKELPARAM(down.x, down.y));
      PostMessageA(g_card_list_drag.initial_target, WM_LBUTTONUP, 0,
                   MAKELPARAM(up.x, up.y));
    }
    SDL_zero(g_card_list_drag);
    return 1;
  }
  return 1;
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
  int force_captured_release = 0;
  int target_has_capture;
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
  if (continue_card_list_drag(event, screen))
    return 1;
  /*
   * ImGui owns mouse input while it is displaying a Win32 popup menu.  Keep
   * our button-state bookkeeping current, but do not route the same event to
   * the legacy window underneath the menu.
   */
  if (shandalar_win32_active_popup(NULL, NULL, NULL)) {
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
        event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
      WPARAM button = event->button.button == SDL_BUTTON_LEFT ? MK_LBUTTON :
                      event->button.button == SDL_BUTTON_RIGHT ? MK_RBUTTON : 0;
      if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        g_mouse_buttons |= button;
      else
        g_mouse_buttons &= ~button;
    }
    return 1;
  }
  target = GetCapture();
  target_has_capture = target != NULL;
  if (!target)
    target = WindowFromPoint(screen);
  if (!target || target == GetDesktopWindow())
    return 0;
  modal = shandalar_win32_active_dialog();
  if (modal && !is_descendant_or_self(target, modal)) {
    /* A modal dialog may appear after this target captured a button-down.
     * Deliver the matching button-up so its nested drag loop can terminate. */
    force_captured_release =
        target_has_capture && event->type == SDL_EVENT_MOUSE_BUTTON_UP;
    if (!force_captured_release) {
      if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
          event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
        WPARAM button = event->button.button == SDL_BUTTON_LEFT ? MK_LBUTTON :
                        event->button.button == SDL_BUTTON_RIGHT ? MK_RBUTTON : 0;
        if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
          g_mouse_buttons |= button;
        else
          g_mouse_buttons &= ~button;
      }
      BringWindowToTop(modal);
      return 1;
    }
  }
  if (!force_captured_release &&
      (point_in_rendered_scrollbar(target, screen) ||
       point_in_rendered_caption(target, screen))) {
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
        event->type == SDL_EVENT_MOUSE_BUTTON_UP) {
      WPARAM button = event->button.button == SDL_BUTTON_LEFT ? MK_LBUTTON :
                      event->button.button == SDL_BUTTON_RIGHT ? MK_RBUTTON : 0;
      if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        g_mouse_buttons |= button;
      else
        g_mouse_buttons &= ~button;
    }
    return 0;
  }
  if (!force_captured_release &&
      event->type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
      event->button.button == SDL_BUTTON_LEFT) {
    HWND scroll_target = find_scrollable_card_list_dialog(target);
    if (scroll_target) {
      g_card_list_drag.initial_target = target;
      g_card_list_drag.scroll_target = scroll_target;
      g_card_list_drag.start = screen;
      g_card_list_drag.last = screen;
      g_card_list_drag.dragging = 0;
      g_mouse_buttons |= MK_LBUTTON;
      return 1;
    }
  }
  if (!force_captured_release && shandalar_win32_is_builtin_control(target))
    return 0;

  if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
    LRESULT hit = SendMessageA(target, WM_NCHITTEST, 0,
                              MAKELPARAM(screen.x, screen.y));
    if (hit != HTCLIENT && hit != HTNOWHERE) {
      if (event->button.button == SDL_BUTTON_LEFT) {
        g_mouse_buttons |= MK_LBUTTON;
        PostMessageA(target, WM_NCLBUTTONDOWN, (WPARAM)hit,
                     MAKELPARAM(screen.x, screen.y));
        return 1;
      }
      if (event->button.button == SDL_BUTTON_RIGHT) {
        g_mouse_buttons |= MK_RBUTTON;
        PostMessageA(target, WM_NCRBUTTONDOWN, (WPARAM)hit,
                     MAKELPARAM(screen.x, screen.y));
        return 1;
      }
    }
  }

  client = screen;
  ScreenToClient(target, &client);
  local = MAKELPARAM(client.x, client.y);
  if (event->type == SDL_EVENT_MOUSE_MOTION) {
    PostMessageA(target, WM_MOUSEMOVE, g_mouse_buttons, local);
    return 1;
  }
  if (event->type == SDL_EVENT_MOUSE_WHEEL) {
    HWND scroll_target = target;
    WPARAM wheel = MAKEWPARAM(g_mouse_buttons,
                              (int)(event->wheel.y * 120.0f));
    while (scroll_target && scroll_target != GetDesktopWindow() &&
           !(GetWindowLongA(scroll_target, GWL_STYLE) & WS_VSCROLL))
      scroll_target = GetParent(scroll_target);
    if (scroll_target && scroll_target != GetDesktopWindow()) {
      int steps = event->wheel.y > 0.0f ? (int)event->wheel.y : -(int)event->wheel.y;
      UINT command = event->wheel.y > 0.0f ? SB_LINEUP : SB_LINEDOWN;
      if (steps < 1)
        steps = 1;
      while (steps-- > 0)
        SendMessageA(scroll_target, WM_VSCROLL, command, 0);
      return 1;
    }
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
  PostMessageA(target, message, g_mouse_buttons, local);
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
  SDL_zero(g_card_list_drag);
}
