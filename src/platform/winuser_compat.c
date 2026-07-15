#include "platform/winuser_compat.h"

#include <stdlib.h>
#include <string.h>

#define WINDOW_MAGIC 0x57494e44u
#define MESSAGE_QUEUE_CAPACITY 1024

typedef struct RegisteredClass {
  struct RegisteredClass *next;
  char *name;
  WNDCLASSA value;
  ATOM atom;
} RegisteredClass;

struct ShandalarWindow {
  uint32_t magic;
  int alive;
  int destroying;
  struct ShandalarWindow *allocation_next;
  struct ShandalarWindow *parent;
  struct ShandalarWindow *first_child;
  struct ShandalarWindow *last_child;
  struct ShandalarWindow *previous_sibling;
  struct ShandalarWindow *next_sibling;
  RegisteredClass *window_class;
  WNDPROC wndproc;
  HINSTANCE instance;
  HMENU menu;
  DWORD style;
  DWORD ex_style;
  LONG user_data;
  int control_id;
  int x;
  int y;
  int width;
  int height;
  int visible;
  int enabled;
  int dirty;
  int erase;
  char *text;
  unsigned char *extra;
  size_t extra_size;
  HDC backing_dc;
};

static RegisteredClass *g_classes;
static struct ShandalarWindow *g_allocations;
static struct ShandalarWindow g_desktop;
static HWND g_focus;
static MSG g_queue[MESSAGE_QUEUE_CAPACITY];
static size_t g_queue_head;
static size_t g_queue_count;
static int g_quit_posted;
static int g_quit_code;
static ShandalarWin32PumpProc g_pump_proc;
static POINT g_system_drag_endpoint;
static int g_has_system_drag_endpoint;

static char *copy_string(const char *text)
{
  size_t size;
  char *copy;
  if (!text)
    text = "";
  size = strlen(text) + 1;
  copy = (char *)malloc(size);
  if (copy)
    memcpy(copy, text, size);
  return copy;
}

static int valid_window(HWND window)
{
  return window && window->magic == WINDOW_MAGIC && window->alive;
}

static int effectively_visible(HWND window)
{
  while (valid_window(window) && window != &g_desktop) {
    if (!window->visible)
      return 0;
    window = window->parent;
  }
  return 1;
}

static RegisteredClass *find_class(LPCSTR name)
{
  RegisteredClass *item;
  if (!name)
    return NULL;
  for (item = g_classes; item; item = item->next)
    if (strcmp(item->name, name) == 0)
      return item;
  return NULL;
}

static LRESULT CALLBACK builtin_wndproc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  BOOL handled = FALSE;
  LRESULT result = shandalar_control_message(window, message, wparam, lparam, &handled);
  if (handled)
    return result;
  return DefWindowProcA(window, message, wparam, lparam);
}

static RegisteredClass *find_or_create_builtin(LPCSTR name)
{
  static const char *builtins[] = {"BUTTON", "LISTBOX", "COMBOBOX", "EDIT", "STATIC", "SCROLLBAR"};
  size_t i;
  RegisteredClass *registered = find_class(name);
  if (registered)
    return registered;
  for (i = 0; i < sizeof(builtins) / sizeof(builtins[0]); ++i) {
    if (strcmp(name, builtins[i]) == 0) {
      WNDCLASSA value;
      memset(&value, 0, sizeof(value));
      value.lpszClassName = name;
      value.lpfnWndProc = builtin_wndproc;
      if (!RegisterClassA(&value))
        return NULL;
      return find_class(name);
    }
  }
  return NULL;
}

static void link_child(HWND parent, HWND child)
{
  if (!parent)
    parent = GetDesktopWindow();
  child->parent = parent;
  child->next_sibling = parent->first_child;
  if (parent->first_child)
    parent->first_child->previous_sibling = child;
  else
    parent->last_child = child;
  parent->first_child = child;
}

static void unlink_child(HWND child)
{
  HWND parent = child->parent;
  if (!parent)
    return;
  if (child->previous_sibling)
    child->previous_sibling->next_sibling = child->next_sibling;
  else
    parent->first_child = child->next_sibling;
  if (child->next_sibling)
    child->next_sibling->previous_sibling = child->previous_sibling;
  else
    parent->last_child = child->previous_sibling;
  child->parent = NULL;
  child->previous_sibling = NULL;
  child->next_sibling = NULL;
}

static void absolute_origin(HWND window, int *x, int *y)
{
  int ax = 0;
  int ay = 0;
  while (valid_window(window) && window != &g_desktop) {
    ax += window->x;
    ay += window->y;
    window = window->parent;
  }
  if (x)
    *x = ax;
  if (y)
    *y = ay;
}

ATOM RegisterClassA(const WNDCLASSA *window_class)
{
  static ATOM next_atom = 1;
  RegisteredClass *item;
  if (!window_class || !window_class->lpszClassName || !window_class->lpfnWndProc)
    return 0;
  item = find_class(window_class->lpszClassName);
  if (item)
    return item->atom;
  item = (RegisteredClass *)calloc(1, sizeof(*item));
  if (!item)
    return 0;
  item->name = copy_string(window_class->lpszClassName);
  if (!item->name) {
    free(item);
    return 0;
  }
  item->value = *window_class;
  item->value.lpszClassName = item->name;
  item->atom = next_atom++;
  item->next = g_classes;
  g_classes = item;
  return item->atom;
}

BOOL UnregisterClassA(LPCSTR class_name, HINSTANCE instance)
{
  RegisteredClass **cursor = &g_classes;
  (void)instance;
  while (*cursor) {
    RegisteredClass *item = *cursor;
    if (strcmp(item->name, class_name ? class_name : "") == 0) {
      *cursor = item->next;
      free(item->name);
      free(item);
      return TRUE;
    }
    cursor = &item->next;
  }
  return FALSE;
}

HWND CreateWindowExA(DWORD ex_style, LPCSTR class_name, LPCSTR window_name,
                     DWORD style, int x, int y, int width, int height,
                     HWND parent, HMENU menu, HINSTANCE instance, LPVOID param)
{
  CREATESTRUCTA create;
  RegisteredClass *registered = find_class(class_name);
  HWND window;
  if (!registered)
    registered = find_or_create_builtin(class_name);
  if (!registered)
    return NULL;
  window = (HWND)calloc(1, sizeof(*window));
  if (!window)
    return NULL;
  window->magic = WINDOW_MAGIC;
  window->alive = 1;
  window->enabled = (style & WS_DISABLED) == 0;
  window->visible = (style & WS_VISIBLE) != 0;
  window->window_class = registered;
  window->wndproc = registered->value.lpfnWndProc;
  window->instance = instance;
  window->menu = menu;
  window->control_id = (style & WS_CHILD) ? (int)(intptr_t)menu : 0;
  window->style = style;
  window->ex_style = ex_style;
  window->x = x;
  window->y = y;
  window->width = width > 0 ? width : 0;
  window->height = height > 0 ? height : 0;
  window->text = copy_string(window_name);
  window->extra_size = registered->value.cbWndExtra > 0 ? (size_t)registered->value.cbWndExtra : 0;
  if (window->extra_size)
    window->extra = (unsigned char *)calloc(1, window->extra_size);
  window->backing_dc = shandalar_create_screen_dc(window->width > 0 ? window->width : 1,
                                                   window->height > 0 ? window->height : 1);
  if (!window->text || (window->extra_size && !window->extra) || !window->backing_dc) {
    shandalar_destroy_screen_dc(window->backing_dc);
    free(window->extra);
    free(window->text);
    free(window);
    return NULL;
  }
  window->allocation_next = g_allocations;
  g_allocations = window;
  link_child(parent, window);
  memset(&create, 0, sizeof(create));
  create.lpCreateParams = param;
  create.hInstance = instance;
  create.hMenu = menu;
  create.hwndParent = parent;
  create.cy = height;
  create.cx = width;
  create.y = y;
  create.x = x;
  create.style = (LONG)style;
  create.lpszName = window_name;
  create.lpszClass = class_name;
  create.dwExStyle = ex_style;
  if (!SendMessageA(window, WM_NCCREATE, 0, (LPARAM)&create) ||
      SendMessageA(window, WM_CREATE, 0, (LPARAM)&create) == -1) {
    DestroyWindow(window);
    return NULL;
  }
  if (window->width > 0 && window->height > 0)
    SendMessageA(window, WM_SIZE, 0, MAKELPARAM(window->width, window->height));
  if (window->visible)
    SendMessageA(window, WM_SHOWWINDOW, TRUE, 0);
  if (window->visible)
    InvalidateRect(window, NULL, TRUE);
  return window;
}

BOOL DestroyWindow(HWND window)
{
  if (!valid_window(window) || window == &g_desktop || window->destroying)
    return FALSE;
  window->destroying = 1;
  while (window->last_child)
    DestroyWindow(window->last_child);
  if (g_focus == window)
    SetFocus(NULL);
  SendMessageA(window, WM_DESTROY, 0, 0);
  SendMessageA(window, WM_NCDESTROY, 0, 0);
  unlink_child(window);
  /*
   * Keep the window allocation as a tombstone so stale HWND values can be
   * rejected safely, but release everything owned by the live window.  Deck
   * builder windows are created and destroyed repeatedly inside Shandalar;
   * retaining their full-size backing bitmaps here leaked several megabytes
   * per visit and left selected fonts/brushes referenced after their owners
   * had deleted them.
   */
  shandalar_destroy_screen_dc(window->backing_dc);
  window->backing_dc = NULL;
  free(window->extra);
  window->extra = NULL;
  window->extra_size = 0;
  free(window->text);
  window->text = NULL;
  window->alive = 0;
  return TRUE;
}

LRESULT DefWindowProcA(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  switch (message) {
  case WM_NCCREATE:
    return TRUE;
  case WM_CLOSE:
    return DestroyWindow(window);
  case WM_SETTEXT:
    return SetWindowTextA(window, (LPCSTR)lparam);
  case WM_GETTEXT:
    return GetWindowTextA(window, (LPSTR)lparam, (int)wparam);
  case WM_GETTEXTLENGTH:
    return GetWindowTextLengthA(window);
  case WM_SYSCOMMAND:
    if ((wparam & 0xfff0) == SC_MOVE && g_has_system_drag_endpoint) {
      shandalar_win32_set_cursor_position(g_system_drag_endpoint.x,
                                         g_system_drag_endpoint.y);
      g_has_system_drag_endpoint = 0;
    }
    return 0;
  case WM_PAINT:
    /* DefWindowProc validates the update region for controls which do not
     * implement their own paint handler.  ImGui renders those controls. */
    if (window->erase)
      SendMessageA(window, WM_ERASEBKGND, (WPARAM)window->backing_dc, 0);
    window->dirty = 0;
    window->erase = 0;
    return 0;
  default:
    return 0;
  }
}

LRESULT CallWindowProcA(WNDPROC proc, HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  return proc ? proc(window, message, wparam, lparam) : 0;
}

LRESULT SendMessageA(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  if (!valid_window(window) || !window->wndproc)
    return 0;
  return window->wndproc(window, message, wparam, lparam);
}

BOOL PostMessageA(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  size_t index;
  if (g_queue_count == MESSAGE_QUEUE_CAPACITY)
    return FALSE;
  index = (g_queue_head + g_queue_count) % MESSAGE_QUEUE_CAPACITY;
  memset(&g_queue[index], 0, sizeof(g_queue[index]));
  g_queue[index].hwnd = window;
  g_queue[index].message = message;
  g_queue[index].wParam = wparam;
  g_queue[index].lParam = lparam;
  ++g_queue_count;
  return TRUE;
}

void PostQuitMessage(int exit_code)
{
  g_quit_posted = 1;
  g_quit_code = exit_code;
}

static int message_matches(const MSG *message, HWND window, UINT min_message, UINT max_message)
{
  if (window && message->hwnd != window)
    return 0;
  if (min_message || max_message)
    return message->message >= min_message && message->message <= max_message;
  return 1;
}

BOOL PeekMessageA(MSG *message, HWND window, UINT min_message, UINT max_message, UINT remove)
{
  size_t i;
  if (!message)
    return FALSE;
  shandalar_win32_runtime_tick();
  shandalar_win32_mm_timers_tick();
  for (i = 0; i < g_queue_count; ++i) {
    size_t index = (g_queue_head + i) % MESSAGE_QUEUE_CAPACITY;
    if (message_matches(&g_queue[index], window, min_message, max_message)) {
      size_t j;
      *message = g_queue[index];
      if (remove & PM_REMOVE) {
        for (j = i; j + 1 < g_queue_count; ++j) {
          size_t dst = (g_queue_head + j) % MESSAGE_QUEUE_CAPACITY;
          size_t src = (g_queue_head + j + 1) % MESSAGE_QUEUE_CAPACITY;
          g_queue[dst] = g_queue[src];
        }
        --g_queue_count;
      }
      return TRUE;
    }
  }
  if (g_quit_posted && !window) {
    memset(message, 0, sizeof(*message));
    message->message = WM_QUIT;
    message->wParam = (WPARAM)g_quit_code;
    if (remove & PM_REMOVE)
      g_quit_posted = 0;
    return TRUE;
  }
  {
    HWND dirty;
    for (dirty = g_allocations; dirty; dirty = dirty->allocation_next) {
      MSG paint_message;
      if (!valid_window(dirty) || !dirty->dirty || dirty->width <= 0 || dirty->height <= 0 ||
          !effectively_visible(dirty))
        continue;
      memset(&paint_message, 0, sizeof(paint_message));
      paint_message.hwnd = dirty;
      paint_message.message = WM_PAINT;
      if (!message_matches(&paint_message, window, min_message, max_message))
        continue;
      *message = paint_message;
      return TRUE;
    }
  }
  return FALSE;
}

BOOL GetMessageA(MSG *message, HWND window, UINT min_message, UINT max_message)
{
  while (!PeekMessageA(message, window, min_message, max_message, PM_REMOVE)) {
    if (!g_pump_proc || !g_pump_proc(TRUE)) {
      /* A platform pump commonly reports shutdown in the same call that posts
       * WM_QUIT.  Consume that message before returning so the caller receives
       * a defined wParam, as it would from Win32 GetMessage(). */
      if (!PeekMessageA(message, window, min_message, max_message, PM_REMOVE)) {
        memset(message, 0, sizeof(*message));
        message->message = WM_QUIT;
      }
      break;
    }
  }
  return message->message != WM_QUIT;
}

LRESULT DispatchMessageA(const MSG *message)
{
  if (message && message->message == WM_TIMER && message->lParam) {
    TIMERPROC proc = (TIMERPROC)message->lParam;
    proc(message->hwnd, WM_TIMER, (UINT_PTR)message->wParam, message->time);
    return 0;
  }
  return message ? SendMessageA(message->hwnd, message->message, message->wParam, message->lParam) : 0;
}

BOOL TranslateMessage(const MSG *message)
{
  (void)message;
  return FALSE;
}

static LONG extra_get(HWND window, int index, size_t size)
{
  LONG value = 0;
  if (!valid_window(window) || index < 0 || (size_t)index + size > window->extra_size)
    return 0;
  memcpy(&value, window->extra + index, size);
  return value;
}

static LONG extra_set(HWND window, int index, LONG value, size_t size)
{
  LONG old = extra_get(window, index, size);
  if (valid_window(window) && index >= 0 && (size_t)index + size <= window->extra_size)
    memcpy(window->extra + index, &value, size);
  return old;
}

LONG GetWindowLongA(HWND window, int index)
{
  if (!valid_window(window))
    return 0;
  switch (index) {
  case GWL_WNDPROC: return (LONG)(intptr_t)window->wndproc;
  case GWL_HINSTANCE: return (LONG)(intptr_t)window->instance;
  case GWL_HWNDPARENT: return (LONG)(intptr_t)GetParent(window);
  case GWL_ID: return window->control_id;
  case GWL_STYLE: return (LONG)window->style;
  case GWL_EXSTYLE: return (LONG)window->ex_style;
  case GWL_USERDATA: return window->user_data;
  default: return extra_get(window, index, sizeof(LONG));
  }
}

LONG SetWindowLongA(HWND window, int index, LONG value)
{
  LONG old = GetWindowLongA(window, index);
  if (!valid_window(window))
    return 0;
  switch (index) {
  case GWL_WNDPROC: window->wndproc = (WNDPROC)(intptr_t)value; break;
  case GWL_HINSTANCE: window->instance = (HINSTANCE)(intptr_t)value; break;
  case GWL_HWNDPARENT: SetParent(window, (HWND)(intptr_t)value); break;
  case GWL_ID: window->control_id = value; break;
  case GWL_STYLE: window->style = (DWORD)value; break;
  case GWL_EXSTYLE: window->ex_style = (DWORD)value; break;
  case GWL_USERDATA: window->user_data = value; break;
  default: return extra_set(window, index, value, sizeof(LONG));
  }
  return old;
}

WORD GetWindowWord(HWND window, int index) { return (WORD)extra_get(window, index, sizeof(WORD)); }
WORD SetWindowWord(HWND window, int index, WORD value) { return (WORD)extra_set(window, index, value, sizeof(WORD)); }
HWND GetParent(HWND window) { return valid_window(window) && window->parent != &g_desktop ? window->parent : NULL; }

HWND SetParent(HWND child, HWND parent)
{
  HWND old;
  if (!valid_window(child) || (parent && !valid_window(parent)))
    return NULL;
  old = GetParent(child);
  unlink_child(child);
  link_child(parent, child);
  return old;
}

HWND GetWindow(HWND window, UINT command)
{
  HWND parent;
  if (!valid_window(window))
    return NULL;
  parent = window->parent;
  switch (command) {
  case GW_HWNDFIRST: return parent ? parent->first_child : NULL;
  case GW_HWNDLAST: return parent ? parent->last_child : NULL;
  case GW_HWNDNEXT: return window->next_sibling;
  case GW_HWNDPREV: return window->previous_sibling;
  case GW_CHILD: return window->first_child;
  case GW_OWNER: return GetParent(window);
  default: return NULL;
  }
}

HWND GetTopWindow(HWND parent)
{
  if (!parent)
    parent = &g_desktop;
  return valid_window(parent) ? parent->first_child : NULL;
}

static BOOL enum_descendants(HWND parent, WNDENUMPROC proc, LPARAM param)
{
  HWND child;
  for (child = parent->first_child; child; child = child->next_sibling) {
    if (!proc(child, param) || !enum_descendants(child, proc, param))
      return FALSE;
  }
  return TRUE;
}

BOOL EnumChildWindows(HWND parent, WNDENUMPROC proc, LPARAM param)
{
  if (!parent)
    parent = &g_desktop;
  return valid_window(parent) && proc ? enum_descendants(parent, proc, param) : FALSE;
}

HWND GetDlgItem(HWND parent, int id)
{
  HWND child;
  if (!valid_window(parent))
    return NULL;
  for (child = parent->first_child; child; child = child->next_sibling)
    if (child->control_id == id)
      return child;
  return NULL;
}

int GetDlgCtrlID(HWND window) { return valid_window(window) ? window->control_id : 0; }

int GetClassNameA(HWND window, LPSTR class_name, int max_count)
{
  size_t length;
  if (!valid_window(window) || !class_name || max_count <= 0)
    return 0;
  length = strlen(window->window_class->name);
  if (length >= (size_t)max_count)
    length = (size_t)max_count - 1;
  memcpy(class_name, window->window_class->name, length);
  class_name[length] = '\0';
  return (int)length;
}

DWORD GetWindowThreadProcessId(HWND window, DWORD *process_id)
{
  if (!valid_window(window))
    return 0;
  if (process_id)
    *process_id = 1;
  return 1;
}

BOOL GetClientRect(HWND window, RECT *rect)
{
  int height;
  if (!valid_window(window) || !rect)
    return FALSE;
  height = window->height;
  if ((window->style & WS_CAPTION) == WS_CAPTION)
    height -= GetSystemMetrics(SM_CYCAPTION);
  SetRect(rect, 0, 0, window->width, height > 0 ? height : 0);
  return TRUE;
}

BOOL GetWindowRect(HWND window, RECT *rect)
{
  int x;
  int y;
  if (!valid_window(window) || !rect)
    return FALSE;
  absolute_origin(window, &x, &y);
  SetRect(rect, x, y, x + window->width, y + window->height);
  return TRUE;
}

BOOL SetWindowPos(HWND window, HWND insert_after, int x, int y, int width, int height, UINT flags)
{
  int old_width;
  int old_height;
  if (!valid_window(window))
    return FALSE;
  old_width = window->width;
  old_height = window->height;
  if (!(flags & SWP_NOMOVE)) {
    window->x = x;
    window->y = y;
    SendMessageA(window, WM_MOVE, 0, MAKELPARAM(x, y));
  }
  if (!(flags & SWP_NOSIZE)) {
    window->width = width > 0 ? width : 0;
    window->height = height > 0 ? height : 0;
    if (window->width != old_width || window->height != old_height) {
      shandalar_destroy_screen_dc(window->backing_dc);
      window->backing_dc = shandalar_create_screen_dc(window->width > 0 ? window->width : 1,
                                                       window->height > 0 ? window->height : 1);
      SendMessageA(window, WM_SIZE, 0, MAKELPARAM(window->width, window->height));
    }
  }
  if (!(flags & SWP_NOZORDER) && insert_after != window && window->parent) {
    HWND parent = window->parent;
    unlink_child(window);
    if (!insert_after || insert_after == (HWND)1) {
      link_child(parent == &g_desktop ? NULL : parent, window);
    } else if (valid_window(insert_after) && insert_after->parent == parent) {
      window->parent = parent;
      window->previous_sibling = insert_after;
      window->next_sibling = insert_after->next_sibling;
      if (insert_after->next_sibling)
        insert_after->next_sibling->previous_sibling = window;
      else
        parent->last_child = window;
      insert_after->next_sibling = window;
    } else {
      link_child(parent == &g_desktop ? NULL : parent, window);
    }
  }
  if (flags & SWP_SHOWWINDOW)
    ShowWindow(window, SW_SHOW);
  if (flags & SWP_HIDEWINDOW)
    ShowWindow(window, SW_HIDE);
  return TRUE;
}

BOOL MoveWindow(HWND window, int x, int y, int width, int height, BOOL repaint)
{
  BOOL result = SetWindowPos(window, NULL, x, y, width, height, SWP_NOZORDER);
  if (result && repaint)
    InvalidateRect(window, NULL, TRUE);
  return result;
}

BOOL ShowWindow(HWND window, int command)
{
  int old;
  if (!valid_window(window))
    return FALSE;
  old = window->visible;
  window->visible = command != SW_HIDE;
  if (old != window->visible) {
    SendMessageA(window, WM_SHOWWINDOW, window->visible, 0);
    if (window->visible)
      InvalidateRect(window, NULL, TRUE);
  }
  return old;
}

BOOL IsWindowVisible(HWND window) { return valid_window(window) && window->visible; }
BOOL IsWindow(HWND window) { return valid_window(window); }
BOOL IsWindowEnabled(HWND window) { return valid_window(window) && window->enabled; }

BOOL EnableWindow(HWND window, BOOL enable)
{
  int old;
  if (!valid_window(window))
    return FALSE;
  old = window->enabled;
  window->enabled = !!enable;
  if (old != window->enabled)
    SendMessageA(window, WM_ENABLE, window->enabled, 0);
  return !old;
}

BOOL ClientToScreen(HWND window, POINT *point)
{
  int x;
  int y;
  if (!valid_window(window) || !point)
    return FALSE;
  absolute_origin(window, &x, &y);
  point->x += x;
  point->y += y;
  return TRUE;
}

BOOL ScreenToClient(HWND window, POINT *point)
{
  int x;
  int y;
  if (!valid_window(window) || !point)
    return FALSE;
  absolute_origin(window, &x, &y);
  point->x -= x;
  point->y -= y;
  return TRUE;
}

int MapWindowPoints(HWND from, HWND to, POINT *points, UINT count)
{
  UINT i;
  int from_x = 0;
  int from_y = 0;
  int to_x = 0;
  int to_y = 0;
  if (!points)
    return 0;
  if (from)
    absolute_origin(from, &from_x, &from_y);
  if (to)
    absolute_origin(to, &to_x, &to_y);
  for (i = 0; i < count; ++i) {
    points[i].x += from_x - to_x;
    points[i].y += from_y - to_y;
  }
  return MAKELONG(from_x - to_x, from_y - to_y);
}

BOOL SetWindowTextA(HWND window, LPCSTR text)
{
  char *copy;
  if (!valid_window(window))
    return FALSE;
  copy = copy_string(text);
  if (!copy)
    return FALSE;
  free(window->text);
  window->text = copy;
  return TRUE;
}

int GetWindowTextA(HWND window, LPSTR text, int max_count)
{
  size_t length;
  if (!valid_window(window) || !text || max_count <= 0)
    return 0;
  length = strlen(window->text);
  if (length >= (size_t)max_count)
    length = (size_t)max_count - 1;
  memcpy(text, window->text, length);
  text[length] = '\0';
  return (int)length;
}

int GetWindowTextLengthA(HWND window) { return valid_window(window) ? (int)strlen(window->text) : 0; }

HWND SetFocus(HWND window)
{
  HWND old = g_focus;
  if (window && !valid_window(window))
    return old;
  if (old == window)
    return old;
  g_focus = window;
  if (old)
    SendMessageA(old, WM_KILLFOCUS, (WPARAM)window, 0);
  if (window)
    SendMessageA(window, WM_SETFOCUS, (WPARAM)old, 0);
  return old;
}

HWND GetFocus(void) { return g_focus; }

BOOL BringWindowToTop(HWND window)
{
  return SetWindowPos(window, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

BOOL SetForegroundWindow(HWND window)
{
  if (!IsWindow(window))
    return FALSE;
  BringWindowToTop(window);
  SetFocus(window);
  return TRUE;
}

BOOL InvalidateRect(HWND window, const RECT *rect, BOOL erase)
{
  (void)rect;
  if (!valid_window(window))
    return FALSE;
  window->dirty = 1;
  window->erase |= !!erase;
  return TRUE;
}

BOOL UpdateWindow(HWND window)
{
  if (!valid_window(window) || !window->dirty)
    return FALSE;
  SendMessageA(window, WM_PAINT, 0, 0);
  window->dirty = 0;
  window->erase = 0;
  return TRUE;
}

HDC BeginPaint(HWND window, PAINTSTRUCT *paint)
{
  int erase;
  if (!valid_window(window) || !paint)
    return NULL;
  erase = window->erase;
  if (erase)
    SendMessageA(window, WM_ERASEBKGND, (WPARAM)window->backing_dc, 0);
  memset(paint, 0, sizeof(*paint));
  paint->hdc = window->backing_dc;
  paint->fErase = erase;
  SetRect(&paint->rcPaint, 0, 0, window->width, window->height);
  window->dirty = 0;
  window->erase = 0;
  return paint->hdc;
}

BOOL EndPaint(HWND window, const PAINTSTRUCT *paint)
{
  return valid_window(window) && paint != NULL;
}

HWND GetDesktopWindow(void)
{
  if (!g_desktop.magic) {
    memset(&g_desktop, 0, sizeof(g_desktop));
    g_desktop.magic = WINDOW_MAGIC;
    g_desktop.alive = 1;
    g_desktop.enabled = 1;
    g_desktop.visible = 1;
    g_desktop.width = 1024;
    g_desktop.height = 768;
    g_desktop.text = copy_string("Desktop");
  }
  return &g_desktop;
}

static int window_matches(HWND window, LPCSTR class_name, LPCSTR window_name)
{
  if (!valid_window(window))
    return 0;
  if (class_name && strcmp(window->window_class->name, class_name) != 0)
    return 0;
  if (window_name && strcmp(window->text ? window->text : "", window_name) != 0)
    return 0;
  return 1;
}

HWND FindWindowExA(HWND parent, HWND child_after, LPCSTR class_name, LPCSTR window_name)
{
  HWND child;
  if (!parent)
    parent = GetDesktopWindow();
  if (!valid_window(parent))
    return NULL;
  child = child_after ? child_after->next_sibling : parent->first_child;
  for (; child; child = child->next_sibling)
    if (window_matches(child, class_name, window_name))
      return child;
  return NULL;
}

HWND FindWindowA(LPCSTR class_name, LPCSTR window_name)
{
  return FindWindowExA(NULL, NULL, class_name, window_name);
}

HDC shandalar_window_backing_dc(HWND window)
{
  return valid_window(window) ? window->backing_dc : NULL;
}

void shandalar_win32_set_pump_proc(ShandalarWin32PumpProc proc) { g_pump_proc = proc; }

BOOL shandalar_win32_pump(BOOL wait)
{
  return !g_pump_proc || g_pump_proc(wait);
}

void shandalar_win32_clear_quit_message(void)
{
  g_quit_posted = 0;
  g_quit_code = 0;
}

void shandalar_win32_set_system_drag_endpoint(int x, int y)
{
  g_system_drag_endpoint.x = x;
  g_system_drag_endpoint.y = y;
  g_has_system_drag_endpoint = 1;
}

void shandalar_win32_clear_system_drag_endpoint(void)
{
  g_has_system_drag_endpoint = 0;
}

void shandalar_win32_reset(void)
{
  RegisteredClass *registered;
  HWND window;
  shandalar_win32_dismiss_popup();
  while (g_desktop.magic && g_desktop.last_child)
    DestroyWindow(g_desktop.last_child);
  window = g_allocations;
  while (window) {
    HWND next = window->allocation_next;
    shandalar_destroy_screen_dc(window->backing_dc);
    free(window->extra);
    free(window->text);
    window->magic = 0;
    free(window);
    window = next;
  }
  g_allocations = NULL;
  while ((registered = g_classes) != NULL) {
    g_classes = registered->next;
    free(registered->name);
    free(registered);
  }
  free(g_desktop.text);
  memset(&g_desktop, 0, sizeof(g_desktop));
  g_focus = NULL;
  g_queue_head = 0;
  g_queue_count = 0;
  g_quit_posted = 0;
  g_quit_code = 0;
  g_pump_proc = NULL;
  g_has_system_drag_endpoint = 0;
  shandalar_controls_reset();
  shandalar_win32_runtime_reset();
  shandalar_win32_mm_timers_reset();
}
