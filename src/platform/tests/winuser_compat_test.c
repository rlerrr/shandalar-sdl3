#include "platform/winuser_compat.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expression) do { if (!(expression)) { \
  fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expression); return __LINE__; \
} } while (0)

static int creates;
static int destroys;
static int custom_messages;
static int paints;

static LRESULT CALLBACK test_wndproc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  switch (message) {
  case WM_CREATE:
    ++creates;
    return 0;
  case WM_DESTROY:
    ++destroys;
    return 0;
  case WM_PAINT:
    {
      PAINTSTRUCT paint;
      CHECK(BeginPaint(window, &paint));
      CHECK(EndPaint(window, &paint));
      ++paints;
      return 0;
    }
  case WM_USER + 1:
    ++custom_messages;
    return (LRESULT)(wparam + lparam);
  default:
    return DefWindowProcA(window, message, wparam, lparam);
  }
}

int main(void)
{
  WNDCLASSA window_class;
  HWND parent;
  HWND child;
  HWND listbox;
  HWND button;
  HKEY registry_key;
  RECT rect;
  POINT point;
  MSG message;
  char text[32];

  memset(&window_class, 0, sizeof(window_class));
  window_class.lpfnWndProc = test_wndproc;
  window_class.cbWndExtra = 12;
  window_class.lpszClassName = "CompatTest";
  CHECK(RegisterClassA(&window_class));
  parent = CreateWindowExA(0, "CompatTest", "parent", WS_VISIBLE, 10, 20, 320, 200,
                           NULL, NULL, NULL, NULL);
  CHECK(parent);
  child = CreateWindowExA(0, "CompatTest", "child", WS_CHILD | WS_VISIBLE,
                          5, 7, 100, 50, parent, (HMENU)42, NULL, NULL);
  CHECK(child);
  CHECK(creates == 2);
  CHECK(GetParent(child) == parent);
  CHECK(GetDlgItem(parent, 42) == child);
  CHECK(GetDlgCtrlID(child) == 42);
  CHECK(GetWindowTextA(child, text, sizeof(text)) == 5);
  CHECK(strcmp(text, "child") == 0);
  CHECK(GetWindowRect(child, &rect));
  CHECK(rect.left == 15 && rect.top == 27 && rect.right == 115 && rect.bottom == 77);
  point.x = 3;
  point.y = 4;
  CHECK(ClientToScreen(child, &point));
  CHECK(point.x == 18 && point.y == 31);
  CHECK(SetWindowLongA(child, 4, 0x12345678) == 0);
  CHECK(GetWindowLongA(child, 4) == 0x12345678);
  CHECK(SendMessageA(child, WM_USER + 1, 7, 9) == 16);
  CHECK(custom_messages == 1);
  CHECK(PostMessageA(child, WM_USER + 1, 10, 20));
  CHECK(PeekMessageA(&message, NULL, 0, 0, PM_REMOVE));
  CHECK(DispatchMessageA(&message) == 30);
  CHECK(custom_messages == 2);
  CHECK(PeekMessageA(&message, NULL, 0, 0, PM_REMOVE));
  CHECK(message.message == WM_PAINT);
  DispatchMessageA(&message);
  CHECK(paints == 1);
  listbox = CreateWindowExA(0, "LISTBOX", "", WS_CHILD | WS_VISIBLE,
                            0, 0, 80, 80, parent, (HMENU)43, NULL, NULL);
  CHECK(listbox);
  CHECK(SendMessageA(listbox, LB_ADDSTRING, 0, (LPARAM)"alpha") == 0);
  CHECK(SendMessageA(listbox, LB_ADDSTRING, 0, (LPARAM)"beta") == 1);
  CHECK(SendMessageA(listbox, LB_GETCOUNT, 0, 0) == 2);
  CHECK(SendMessageA(listbox, LB_SETCURSEL, 1, 0) == 1);
  CHECK(SendMessageA(listbox, LB_GETCURSEL, 0, 0) == 1);
  button = CreateWindowExA(0, "BUTTON", "", WS_CHILD | WS_VISIBLE,
                           0, 0, 20, 20, parent, (HMENU)44, NULL, NULL);
  CHECK(button);
  CHECK(CheckDlgButton(parent, 44, BST_CHECKED));
  CHECK(IsDlgButtonChecked(parent, 44) == BST_CHECKED);
  CHECK(SetTimer(parent, 0, 100, NULL) != 0);
  CHECK(KillTimer(parent, 0));
  CHECK(RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\CompatTest", 0, NULL, 0,
                        KEY_ALL_ACCESS, NULL, &registry_key, NULL) == ERROR_SUCCESS);
  CHECK(RegSetValueExA(registry_key, "Value", 0, REG_SZ,
                       (const BYTE *)"saved", 6) == ERROR_SUCCESS);
  {
    DWORD size = sizeof(text);
    CHECK(RegQueryValueExA(registry_key, "Value", NULL, NULL,
                           (BYTE *)text, &size) == ERROR_SUCCESS);
    CHECK(strcmp(text, "saved") == 0);
  }
  CHECK(DestroyWindow(parent));
  CHECK(destroys == 2); /* Built-in controls use their own window procedure. */
  CHECK(!IsWindow(parent) && !IsWindow(child));
  shandalar_win32_reset();
  return 0;
}
