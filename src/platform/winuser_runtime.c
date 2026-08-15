#include "platform/winuser_compat.h"

#include <string.h>

#define MAX_COMPAT_TIMERS 64

typedef struct CompatTimer {
  int active;
  HWND window;
  UINT_PTR id;
  UINT interval;
  TIMERPROC proc;
  uint64_t next_tick;
} CompatTimer;

static CompatTimer g_timers[MAX_COMPAT_TIMERS];
static UINT_PTR g_next_thread_timer_id = 1;
static POINT g_cursor;
static int g_accelerator_sentinel;
static UINT g_accelerator_resource_id;
static int g_icon_sentinel;
static int g_cursor_sentinel;
static HCURSOR g_current_cursor;

BOOL GetCursorPos(POINT *point)
{
  if (!point)
    return FALSE;
  *point = g_cursor;
  return TRUE;
}

BOOL SetCursorPos(int x, int y)
{
  shandalar_win32_set_cursor_position(x, y);
  return TRUE;
}

void shandalar_win32_set_cursor_position(int x, int y)
{
  g_cursor.x = x;
  g_cursor.y = y;
}

static HWND hit_test_children(HWND parent, POINT point)
{
  HWND child = GetWindow(parent, GW_CHILD);
  while (child) {
    RECT rect;
    if (IsWindowVisible(child) && GetWindowRect(child, &rect) &&
        point.x >= rect.left && point.x < rect.right &&
        point.y >= rect.top && point.y < rect.bottom) {
      HWND descendant = hit_test_children(child, point);
      return descendant ? descendant : child;
    }
    child = GetWindow(child, GW_HWNDNEXT);
  }
  return NULL;
}

/* Owned popups live in the owner's bookkeeping tree in the compatibility
 * layer, but unlike WS_CHILD controls they can remain visible outside (or
 * while hiding) that owner. */
static HWND hit_test_owned_popups(HWND parent, POINT point)
{
  HWND child = GetWindow(parent, GW_CHILD);
  while (child) {
    HWND nested = hit_test_owned_popups(child, point);
    if (nested)
      return nested;
    if ((GetWindowLongA(child, GWL_STYLE) & WS_POPUP) == WS_POPUP &&
        IsWindowVisible(child)) {
      RECT rect;
      if (GetWindowRect(child, &rect) &&
          point.x >= rect.left && point.x < rect.right &&
          point.y >= rect.top && point.y < rect.bottom) {
        HWND descendant = hit_test_children(child, point);
        return descendant ? descendant : child;
      }
    }
    child = GetWindow(child, GW_HWNDNEXT);
  }
  return NULL;
}

HWND WindowFromPoint(POINT point)
{
  HWND hit = hit_test_owned_popups(GetDesktopWindow(), point);
  if (!hit)
    hit = hit_test_children(GetDesktopWindow(), point);
  return hit ? hit : GetDesktopWindow();
}

UINT_PTR SetTimer(HWND window, UINT_PTR id, UINT interval_ms, TIMERPROC proc)
{
  UINT_PTR assigned_id = id;
  size_t i;
  if (window && !IsWindow(window))
    return 0;
  if (!window && !assigned_id) {
    assigned_id = g_next_thread_timer_id++;
    if (!g_next_thread_timer_id)
      g_next_thread_timer_id = 1;
  }
  if (!interval_ms)
    interval_ms = 1;
  for (i = 0; i < MAX_COMPAT_TIMERS; ++i) {
    if ((g_timers[i].active && g_timers[i].window == window &&
         g_timers[i].id == assigned_id) || !g_timers[i].active) {
      g_timers[i].active = 1;
      g_timers[i].window = window;
      g_timers[i].id = assigned_id;
      g_timers[i].interval = interval_ms;
      g_timers[i].proc = proc;
      g_timers[i].next_tick = SDL_GetTicks() + interval_ms;
      return assigned_id ? assigned_id : 1;
    }
  }
  return 0;
}

BOOL KillTimer(HWND window, UINT_PTR id)
{
  size_t i;
  for (i = 0; i < MAX_COMPAT_TIMERS; ++i) {
    if (g_timers[i].active && g_timers[i].window == window &&
        g_timers[i].id == id) {
      memset(&g_timers[i], 0, sizeof(g_timers[i]));
      return TRUE;
    }
  }
  return FALSE;
}

void shandalar_win32_runtime_tick(void)
{
  uint64_t now = SDL_GetTicks();
  size_t i;
  for (i = 0; i < MAX_COMPAT_TIMERS; ++i) {
    CompatTimer *timer = &g_timers[i];
    if (!timer->active)
      continue;
    if (timer->window && !IsWindow(timer->window)) {
      memset(timer, 0, sizeof(*timer));
      continue;
    }
    if (now >= timer->next_tick) {
      PostMessageA(timer->window, WM_TIMER, timer->id, (LPARAM)timer->proc);
      timer->next_tick = now + timer->interval;
    }
  }
}

void shandalar_win32_runtime_reset(void)
{
  memset(g_timers, 0, sizeof(g_timers));
  g_next_thread_timer_id = 1;
  memset(&g_cursor, 0, sizeof(g_cursor));
}

int GetSystemMetrics(int index)
{
  RECT desktop;
  GetClientRect(GetDesktopWindow(), &desktop);
  switch (index) {
  case SM_CXSCREEN: return desktop.right;
  case SM_CYSCREEN: return desktop.bottom;
  case SM_CXVSCROLL: return 17;
  case SM_CXBORDER: return 1;
  case SM_CYCAPTION: return 19;
  case SM_CYHSCROLL: return 17;
  case SM_CXSIZE: return 18;
  default: return 0;
  }
}

HACCEL LoadAcceleratorsA(HINSTANCE instance, LPCSTR resource_name)
{
  (void)instance;
  g_accelerator_resource_id = (UINT)(uintptr_t)resource_name & 0xffffu;
  return (HACCEL)&g_accelerator_sentinel;
}

int TranslateAcceleratorA(HWND window, HACCEL accelerator, MSG *message)
{
  UINT key;
  UINT command = 0;
  BOOL control;
  BOOL alt;
  BOOL shift;
  if (accelerator != (HACCEL)&g_accelerator_sentinel || !message ||
      message->message != WM_KEYDOWN || g_accelerator_resource_id != 111)
    return 0;
  key = (UINT)message->wParam;
  control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
  alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
  shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
  if (!control && !alt) {
    switch (key) {
    case VK_F1: command = 600; break; case VK_F2: command = 620; break;
    case VK_F3: command = 622; break; case VK_F4: command = 601; break;
    case VK_F5: command = 602; break; case VK_F6: command = 603; break;
    case VK_F8: command = 604; break; case VK_F9: command = 605; break;
    case VK_F10: command = 606; break; case VK_F12: command = 599; break;
    }
  }
  if (key >= 'A' && key <= 'Z')
    key += 'a' - 'A';
  if (command) {
  } else if (control) {
    switch (key) {
    case 'd': command = 617; break; case 'g': command = 619; break;
    case 'h': command = 621; break; case 'i': command = 634; break;
    case 'l': command = 623; break; case 's': command = 635; break;
    case 't': command = 633; break; case 'u': command = 636; break;
    case 'w': command = 615; break;
    }
  } else if (alt) {
    switch (key) {
    case 'd': command = 618; break; case 'g': command = 620; break;
    case 'h': command = 622; break; case 'l': command = 624; break;
    case 'w': command = 616; break;
    }
  } else {
    switch (key) {
    case 'a': command = shift ? 628 : 627; break;
    case 'c': command = 609; break; case 'd': command = 617; break;
    case 'f': command = 630; break; case 'g': command = 619; break;
    case 'h': command = 621; break; case 'k': command = 631; break;
    case 'l': command = 623; break; case 'o': command = 613; break;
    case 'p': command = 625; break; case 's': command = 607; break;
    case 't': if (shift) command = 611; break;
    case 'w': command = 615; break; case 'z': command = 629; break;
    }
  }
  if (command) {
    SendMessageA(window, WM_COMMAND, command, 0);
    return 1;
  }
  return 0;
}

HICON LoadIconA(HINSTANCE instance, LPCSTR resource_name)
{
  (void)instance;
  (void)resource_name;
  return (HICON)&g_icon_sentinel;
}

BOOL DestroyIcon(HICON icon)
{
  (void)icon;
  return TRUE;
}

HBITMAP LoadBitmapA(HINSTANCE instance, LPCSTR resource_name)
{
  UINT_PTR resource_id = (UINT_PTR)resource_name;
  (void)instance;
  if (resource_id == 0x7fed)
    return CreateBitmap(GetSystemMetrics(SM_CXSIZE),
                        GetSystemMetrics(SM_CYCAPTION) - 1, 1, 8, NULL);
  return CreateBitmap(1, 1, 1, 8, NULL);
}

HCURSOR LoadCursorA(HINSTANCE instance, LPCSTR resource_name)
{
  (void)instance;
  (void)resource_name;
  return (HCURSOR)&g_cursor_sentinel;
}

HCURSOR SetCursor(HCURSOR cursor)
{
  HCURSOR previous = g_current_cursor;
  g_current_cursor = cursor;
  return previous;
}

SHORT GetKeyState(int virtual_key)
{
  const bool *keys = SDL_GetKeyboardState(NULL);
  if (virtual_key == VK_SHIFT &&
      (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]))
    return (SHORT)0x8000;
  if (virtual_key == VK_CONTROL &&
      (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]))
    return (SHORT)0x8000;
  if (virtual_key == VK_MENU &&
      (keys[SDL_SCANCODE_LALT] || keys[SDL_SCANCODE_RALT]))
    return (SHORT)0x8000;
  return 0;
}

LONG GetMessageTime(void) { return (LONG)SDL_GetTicks(); }
BOOL IsIconic(HWND window) { (void)window; return FALSE; }

void InitCommonControls(void) {}
BOOL MessageBeep(UINT type) { (void)type; return TRUE; }
BOOL WinHelpA(HWND window, LPCSTR help_file, UINT command, ULONG_PTR data)
{
  (void)window;
  (void)help_file;
  (void)command;
  (void)data;
  return TRUE;
}
UINT GetDoubleClickTime(void) { return 500; }

BOOL ScrollWindow(HWND window, int dx, int dy, const RECT *scroll, const RECT *clip)
{
  HWND child;
  RECT child_rect;
  POINT child_origin;
  POINT child_corner;

  (void)clip;
  if (!window)
    return FALSE;

  /* ScrollWindow moves child windows contained by the scrolled rectangle.
   * Card-list dialogs rely on this to make their grid act as one scrolling
   * client surface. */
  for (child = GetWindow(window, GW_CHILD); child;
       child = GetWindow(child, GW_HWNDNEXT)) {
    if (!GetWindowRect(child, &child_rect))
      continue;
    child_origin.x = child_rect.left;
    child_origin.y = child_rect.top;
    child_corner.x = child_rect.right;
    child_corner.y = child_rect.bottom;
    ScreenToClient(window, &child_origin);
    ScreenToClient(window, &child_corner);
    if (scroll && (child_corner.x <= scroll->left ||
                   child_origin.x >= scroll->right ||
                   child_corner.y <= scroll->top ||
                   child_origin.y >= scroll->bottom))
      continue;
    SetWindowPos(child, NULL, child_origin.x + dx, child_origin.y + dy,
                 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
  }
  return InvalidateRect(window, NULL, FALSE);
}

BOOL LockWindowUpdate(HWND window) { (void)window; return TRUE; }

COLORREF GetSysColor(int index)
{
  switch (index) {
  case 15: return RGB(192, 192, 192);
  case 16: return RGB(128, 128, 128);
  case 20: return RGB(255, 255, 255);
  default: return RGB(0, 0, 0);
  }
}
