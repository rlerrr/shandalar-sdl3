#include "platform/winuser_compat.h"

#include <string.h>

static LRESULT CALLBACK mci_window_proc(HWND window, UINT message,
                                        WPARAM wparam, LPARAM lparam)
{
  switch (message) {
  case WM_USER + 127: /* MCIWNDM_SETPALETTE */
  case 0x806:         /* MCIWNDM_PLAY */
  case 0x808:         /* MCIWNDM_STOP */
    return 0;
  default:
    return DefWindowProcA(window, message, wparam, lparam);
  }
}

HWND MCIWndCreateA(HWND parent, HINSTANCE instance, DWORD style,
                   LPCSTR filename)
{
  static int registered;
  WNDCLASSA window_class;
  (void)filename;
  if (!registered) {
    memset(&window_class, 0, sizeof(window_class));
    window_class.lpfnWndProc = mci_window_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = "ShandalarMCIWindow";
    if (!RegisterClassA(&window_class))
      return NULL;
    registered = 1;
  }
  return CreateWindowExA(0, "ShandalarMCIWindow", "", style,
                         0, 0, 320, 240, parent, NULL, instance, NULL);
}
