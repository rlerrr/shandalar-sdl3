#include "platform/winuser_compat.h"

#include <string.h>

#define MAX_COMPAT_TIMERS 64

typedef struct CompatTimer {
  HWND window;
  UINT_PTR id;
  UINT interval;
  TIMERPROC proc;
  uint64_t next_tick;
} CompatTimer;

static CompatTimer g_timers[MAX_COMPAT_TIMERS];
static POINT g_cursor;
static int g_accelerator_sentinel;
static int g_icon_sentinel;
static int g_cursor_sentinel;

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

HWND WindowFromPoint(POINT point)
{
  HWND hit = hit_test_children(GetDesktopWindow(), point);
  return hit ? hit : GetDesktopWindow();
}

UINT_PTR SetTimer(HWND window, UINT_PTR id, UINT interval_ms, TIMERPROC proc)
{
  size_t i;
  if (!window || !IsWindow(window))
    return 0;
  if (!interval_ms)
    interval_ms = 1;
  for (i = 0; i < MAX_COMPAT_TIMERS; ++i) {
    if ((g_timers[i].window == window && g_timers[i].id == id) || !g_timers[i].window) {
      g_timers[i].window = window;
      g_timers[i].id = id;
      g_timers[i].interval = interval_ms;
      g_timers[i].proc = proc;
      g_timers[i].next_tick = SDL_GetTicks() + interval_ms;
      return id ? id : 1;
    }
  }
  return 0;
}

BOOL KillTimer(HWND window, UINT_PTR id)
{
  size_t i;
  for (i = 0; i < MAX_COMPAT_TIMERS; ++i) {
    if (g_timers[i].window == window && g_timers[i].id == id) {
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
    if (!timer->window)
      continue;
    if (!IsWindow(timer->window)) {
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
  memset(&g_cursor, 0, sizeof(g_cursor));
}

int GetSystemMetrics(int index)
{
  RECT desktop;
  GetClientRect(GetDesktopWindow(), &desktop);
  switch (index) {
  case SM_CXSCREEN: return desktop.right;
  case SM_CYSCREEN: return desktop.bottom;
  case SM_CXBORDER: return 1;
  case SM_CYCAPTION: return 19;
  case SM_CYHSCROLL: return 17;
  default: return 0;
  }
}

HACCEL LoadAcceleratorsA(HINSTANCE instance, LPCSTR resource_name)
{
  (void)instance;
  (void)resource_name;
  return (HACCEL)&g_accelerator_sentinel;
}

int TranslateAcceleratorA(HWND window, HACCEL accelerator, MSG *message)
{
  (void)window;
  (void)accelerator;
  (void)message;
  return 0;
}

HICON LoadIconA(HINSTANCE instance, LPCSTR resource_name)
{
  (void)instance;
  (void)resource_name;
  return (HICON)&g_icon_sentinel;
}

HCURSOR LoadCursorA(HINSTANCE instance, LPCSTR resource_name)
{
  (void)instance;
  (void)resource_name;
  return (HCURSOR)&g_cursor_sentinel;
}

void InitCommonControls(void) {}
BOOL MessageBeep(UINT type) { (void)type; return TRUE; }
UINT GetDoubleClickTime(void) { return 500; }

BOOL ScrollWindow(HWND window, int dx, int dy, const RECT *scroll, const RECT *clip)
{
  (void)dx; (void)dy; (void)scroll; (void)clip;
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
