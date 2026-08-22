#include "platform/winuser_compat.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expression) do { if (!(expression)) { \
  fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expression); return __LINE__; \
} } while (0)

static LRESULT CALLBACK caption_test_wndproc(HWND window, UINT message,
                                             WPARAM wparam, LPARAM lparam)
{
  return DefWindowProcA(window, message, wparam, lparam);
}

int main(void)
{
  WNDCLASSA window_class;
  HWND themed_window;
  HWND dialog;
  RECT rect;

  memset(&window_class, 0, sizeof(window_class));
  window_class.lpfnWndProc = caption_test_wndproc;
  window_class.lpszClassName = "CaptionTest";
  CHECK(RegisterClassA(&window_class));

  themed_window = CreateWindowExA(0, "CaptionTest", "themed",
                                   WS_VISIBLE | WS_CAPTION,
                                   0, 0, 200, 100,
                                   NULL, NULL, NULL, NULL);
  CHECK(themed_window);
  CHECK(!shandalar_win32_should_render_caption(themed_window));
  CHECK(GetClientRect(themed_window, &rect));
  CHECK(rect.right == 188);
  CHECK(rect.bottom == 100 - GetSystemMetrics(SM_CYCAPTION) - 12);

  dialog = shandalar_win32_create_dialog(NULL, NULL, "dialog",
                                         0, 0, 200, 100);
  CHECK(dialog);
  CHECK(shandalar_win32_should_render_caption(dialog));

  CHECK(DestroyWindow(dialog));
  CHECK(DestroyWindow(themed_window));
  shandalar_win32_reset();
  return 0;
}
