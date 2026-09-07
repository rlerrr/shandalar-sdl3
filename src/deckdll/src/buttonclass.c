#ifdef SHANDALAR_SDL_WIN32_COMPAT
#include "platform/winuser_compat.h"
#include "platform/legacy_io.h"
#else
#include "WINDOWS.H"
#endif

extern HPALETTE global_cart_art_hpalette;

// GLOBAL: DECKDLL 0x1011279c
// GLOBAL: MAGIC 0x007775b0
static WNDPROC global_wndproc_std_ButtonClass;

typedef struct PaletteMessageArgs
{
  HWND hwnd;
  UINT msg;
  HWND wparam;
  LPARAM lparam;
} PaletteMessageArgs;

// FUNCTION: DECKDLL 0x10025d1b
// FUNCTION: MAGIC 0x00496489
static BOOL CALLBACK enum_child_palette_message_proc(HWND child_hwnd, LPARAM lparam)
{
  PaletteMessageArgs *args;

  args = (PaletteMessageArgs *)lparam;
  if (GetParent(child_hwnd) == args->hwnd)
    SendMessageA(child_hwnd, args->msg, (WPARAM)args->wparam, args->lparam);
  return TRUE;
}

// FUNCTION: DECKDLL 0x10025b5e
// FUNCTION: MAGIC 0x004962cc
// FUNCTION: SHANDALAR 0x00466d5f
int handle_button_palette_message(HWND hwnd, UINT msg, HWND wparam_hwnd, LPARAM lparam)
{
  HDC hdc2; // ebp - 0x38
  PaletteMessageArgs args;

  DWORD tid_dst;     // ebp - 0x18
  HDC hdc;           // ebp - 0x14
  DWORD tid_src;     // ebp - 0x10
  DWORD procid_dst;  // ebp - 0xc
  HWND changed_hwnd; // ebp - 0x8
  DWORD procid_src;  // ebp - 0x4

  switch (msg)
  {
  case WM_PALETTEISCHANGING:
  case WM_PALETTECHANGED:
    changed_hwnd = wparam_hwnd;
    if (changed_hwnd != hwnd)
    {
      tid_src = GetWindowThreadProcessId(changed_hwnd, &procid_src);
      tid_dst = GetWindowThreadProcessId(hwnd, &procid_dst);

      if (procid_src != procid_dst)
      {
        InvalidateRect(hwnd, NULL, TRUE);
      }
      else if (GetParent(hwnd) == NULL)
      {
        hdc = GetDC(hwnd);
        SelectPalette(hdc, global_cart_art_hpalette, TRUE);
        InvalidateRect(hwnd, NULL, TRUE);
        ReleaseDC(hwnd, hdc);
      }
      else if (!(GetWindowLongA(hwnd, GWL_STYLE) & WS_CHILD))
      {
        InvalidateRect(hwnd, NULL, TRUE);
      }
    }
    if (msg == WM_PALETTECHANGED)
    {
      args.hwnd = hwnd;
      args.msg = msg;
      args.wparam = wparam_hwnd;
      args.lparam = lparam;
      EnumChildWindows(hwnd, enum_child_palette_message_proc, (LPARAM)&args);
    }
    return 0;

  case WM_QUERYNEWPALETTE:
    UnrealizeObject(global_cart_art_hpalette);
    hdc2 = GetDC(hwnd);
    SelectPalette(hdc2, global_cart_art_hpalette, FALSE);

    if (RealizePalette(hdc2))
      InvalidateRect(hwnd, NULL, TRUE);

    ReleaseDC(hwnd, hdc2);
    return 1;
  default:
    return 0;
  }
}

// FUNCTION: DECKDLL 0x100256c8
// FUNCTION: MAGIC 0x00495e36
static int is_buttonclass(HWND hwnd)
{
  char classname[0x64];

  if (hwnd == (HWND)0)
    return 0;

  GetClassNameA(hwnd, classname, 0x64);
  if (!_strcmpi(classname, "Button"))
    return 1;
  else
    return 0;
}

// FUNCTION: DECKDLL 0x10025593
// FUNCTION: MAGIC 0x00495d01
LRESULT CALLBACK wndproc_ButtonClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  HWND lost_focus_button;
  HWND focused_button;
  LRESULT result;

  if ((msg == WM_SETFOCUS) || (msg == WM_KILLFOCUS))
  {
    if (msg == WM_SETFOCUS)
    {
      focused_button = hwnd;
      lost_focus_button = (HWND)wparam;
    }
    else
    {
      lost_focus_button = hwnd;
      focused_button = (HWND)wparam;
    }
    if (is_buttonclass(focused_button) == 0)
      focused_button = (HWND)0;
    if (is_buttonclass(lost_focus_button) == 0)
      lost_focus_button = (HWND)0;
    SendMessageA(GetParent(hwnd), WM_USER + 0xC8, (WPARAM)focused_button, (LPARAM)lost_focus_button);
    result = 0;
  }
  else if ((msg == WM_PALETTECHANGED) || (msg == WM_PALETTEISCHANGING) || (msg == WM_QUERYNEWPALETTE))
  {
    result = CallWindowProcA(global_wndproc_std_ButtonClass, hwnd, msg, wparam, lparam);
    handle_button_palette_message(hwnd, msg, (HWND)wparam, lparam);
  }
  else
    result = CallWindowProcA(global_wndproc_std_ButtonClass, hwnd, msg, wparam, lparam);

  return result;
}

// FUNCTION: DECKDLL 0x10025552
// FUNCTION: MAGIC 0x00495cc0
BOOL CALLBACK enumfunc_change_buttonclass_wndproc(HWND hwnd, LPARAM lparam)
{
  if (is_buttonclass(hwnd))
    global_wndproc_std_ButtonClass =
        (WNDPROC)SetWindowLongPtrA(hwnd, GWL_WNDPROC, (LONG_PTR)wndproc_ButtonClass);
  return TRUE;
}

// FUNCTION: DECKDLL 0x10025536
// FUNCTION: MAGIC 0x00495ca4
// FUNCTION: SHANDALAR 0x00466737
void change_buttonclass_wndproc(HWND hwnd)
{
  EnumChildWindows(hwnd, enumfunc_change_buttonclass_wndproc, 0);
}
