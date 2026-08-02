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
static int menu_initializations;
static int menu_closures;
static int text_changes;

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
  case WM_SETTEXT:
    ++text_changes;
    return DefWindowProcA(window, message, wparam, lparam);
  case WM_INITMENU:
    ++menu_initializations;
    CHECK(AppendMenuA((HMENU)wparam, MF_STRING, 101, "test item"));
    return 0;
  case WM_MENUSELECT:
    if (HIWORD(wparam) == 0xffff && lparam == 0) {
      ++menu_closures;
      while (GetMenuItemCount((HMENU)GetWindowLongA(window, 8)) > 0)
        CHECK(DeleteMenu((HMENU)GetWindowLongA(window, 8), 0, MF_BYPOSITION));
    }
    return 0;
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
  HWND double_click_window;
  HWND caption_window;
  HWND caption_child;
  HWND dialog;
  HWND listbox;
  HWND button;
  HWND popup;
  HMENU popup_menu;
  UINT_PTR thread_timer;
  HDC parent_dc;
  HDC child_dc;
  HDC bitmap_dc;
  HPALETTE palette;
  HBITMAP bitmap;
  HBRUSH brush;
  HPEN pen;
  BITMAPINFO bitmap_info;
  void *bitmap_bits;
  HKEY registry_key;
  RECT rect;
  POINT point;
  MSG message;
  char text[32];
  uint32_t rgba_pixels[100 * 50];
  struct {
    WORD version;
    WORD count;
    PALETTEENTRY entries[256];
  } palette_data;

  memset(&window_class, 0, sizeof(window_class));
  window_class.lpfnWndProc = test_wndproc;
  window_class.cbWndExtra = 12;
  window_class.lpszClassName = "CompatTest";
  CHECK(RegisterClassA(&window_class));
  parent = CreateWindowExA(0, "CompatTest", "parent", WS_VISIBLE, 10, 20, 320, 200,
                           NULL, NULL, NULL, NULL);
  CHECK(parent);
  CHECK(!IsWindow(HWND_TOPMOST));
  CHECK(!IsWindow(HWND_NOTOPMOST));
  CHECK(!shandalar_win32_accepts_double_click(parent));
  window_class.style = CS_DBLCLKS;
  window_class.lpszClassName = "DoubleClickTest";
  CHECK(RegisterClassA(&window_class));
  double_click_window = CreateWindowExA(0, "DoubleClickTest", "double", WS_VISIBLE,
                                        0, 0, 20, 20, NULL, NULL, NULL, NULL);
  CHECK(double_click_window);
  CHECK(shandalar_win32_accepts_double_click(double_click_window));
  CHECK(SetWindowPos(parent, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE));
  CHECK((GetWindowLongA(parent, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0);
  CHECK(GetWindow(GetDesktopWindow(), GW_CHILD) == parent);
  CHECK(SetWindowPos(parent, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE));
  CHECK((GetWindowLongA(parent, GWL_EXSTYLE) & WS_EX_TOPMOST) == 0);
  CHECK(SetWindowPos(parent, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE));
  CHECK(GetWindow(GetWindow(GetDesktopWindow(), GW_CHILD), GW_HWNDLAST) == parent);
  child = CreateWindowExA(0, "CompatTest", "child", WS_CHILD | WS_VISIBLE,
                          5, 7, 100, 50, parent, (HMENU)42, NULL, NULL);
  CHECK(child);
  CHECK(creates == 3);
  caption_window = CreateWindowExA(0, "CompatTest", "caption",
                                   WS_VISIBLE | WS_CAPTION,
                                   100, 120, 200, 100,
                                   NULL, NULL, NULL, NULL);
  CHECK(caption_window);
  caption_child = CreateWindowExA(0, "CompatTest", "caption child",
                                  WS_CHILD | WS_VISIBLE,
                                  4, 6, 20, 10,
                                  caption_window, (HMENU)45, NULL, NULL);
  CHECK(caption_child);
  CHECK(GetWindowRect(caption_child, &rect));
  CHECK(rect.left == 104);
  CHECK(rect.top == 126 + GetSystemMetrics(SM_CYCAPTION));
  point.x = rect.left;
  point.y = rect.top;
  CHECK(ScreenToClient(caption_window, &point));
  CHECK(point.x == 4 && point.y == 6);
  popup = CreateWindowExA(0, "CompatTest", "popup", WS_POPUP,
                          0, 0, 20, 20, parent, NULL, NULL, NULL);
  CHECK(popup);
  dialog = shandalar_win32_create_dialog(NULL, parent, "dialog", 0, 0, 100, 50);
  CHECK(dialog);
  CHECK(SetWindowLongA(dialog, DWL_USER, 0x12345678) == 0);
  CHECK(GetWindowLongA(dialog, DWL_USER) == 0x12345678);
  CHECK(SetWindowPos(popup, HWND_BOTTOM, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE));
  CHECK(GetWindow(parent, GW_CHILD) == child);
  CHECK(!ShowWindow(popup, SW_SHOW));
  CHECK(GetWindow(parent, GW_CHILD) == popup);
  popup_menu = CreatePopupMenu();
  CHECK(popup_menu);
  CHECK(SetWindowLongA(parent, 8, (LONG)popup_menu) == 0);
  CHECK(TrackPopupMenu(popup_menu, TPM_RIGHTBUTTON, 1, 2, 0, parent, NULL));
  CHECK(menu_initializations == 1);
  CHECK(shandalar_win32_menu_item_count(
            shandalar_win32_active_popup(NULL, NULL, NULL)) == 1);
  CHECK(TrackPopupMenu(popup_menu, TPM_RIGHTBUTTON, 3, 4, 0, parent, NULL));
  CHECK(menu_closures == 1);
  CHECK(menu_initializations == 2);
  CHECK(shandalar_win32_menu_item_count(
            shandalar_win32_active_popup(NULL, NULL, NULL)) == 1);
  shandalar_win32_dismiss_popup();
  CHECK(menu_closures == 2);
  CHECK(GetMenuItemCount(popup_menu) == 0);
  CHECK(DestroyMenu(popup_menu));
  memset(&palette_data, 0, sizeof(palette_data));
  palette_data.version = 0x300;
  palette_data.count = 256;
  palette_data.entries[42].peRed = 255;
  palette = CreatePalette((const LOGPALETTE *)&palette_data);
  CHECK(palette);
  parent_dc = GetDC(parent);
  CHECK(parent_dc);
  CHECK(SelectPalette(parent_dc, palette, FALSE) == NULL);
  CHECK(RealizePalette(parent_dc) == 256);
  CHECK(ReleaseDC(parent, parent_dc));
  child_dc = GetDC(child);
  CHECK(child_dc);
  CHECK(SetTextAlign(child_dc, TA_RIGHT | TA_BOTTOM) == (TA_LEFT | TA_TOP));
  CHECK(SaveDC(child_dc));
  CHECK(SetTextAlign(child_dc, TA_CENTER | TA_BASELINE) ==
        (TA_RIGHT | TA_BOTTOM));
  CHECK(RestoreDC(child_dc, -1));
  CHECK(SetTextAlign(child_dc, TA_LEFT | TA_TOP) == (TA_RIGHT | TA_BOTTOM));
  CHECK(SetPixelV(child_dc, 0, 0, PALETTEINDEX(42)));
  CHECK(shandalar_dc_copy_rgba(child_dc, rgba_pixels, 100 * sizeof(rgba_pixels[0])));
  CHECK(rgba_pixels[0] == 0xffff0000u);
  CHECK(SetPixelV(child_dc, 1, 0, RGB(255, 0, 0)));
  CHECK(GetPixel(child_dc, 1, 0) == RGB(255, 0, 0));
  palette_data.entries[42].peRed = 0;
  palette_data.entries[43].peRed = 255;
  CHECK(SetPaletteEntries(palette, 42, 2, &palette_data.entries[42]) == 2);
  CHECK(SetPixelV(child_dc, 2, 0, RGB(255, 0, 0)));
  CHECK(GetPixel(child_dc, 2, 0) == RGB(255, 0, 0));
  {
    RECT card_rect = {10, 10, 30, 30};
    CHECK(FillRect(child_dc, &card_rect, GetStockObject(BLACK_BRUSH)));
    brush = CreateSolidBrush(RGB(255, 0, 0));
    pen = GetStockObject(NULL_PEN);
    CHECK(brush && pen);
    SelectObject(child_dc, brush);
    SelectObject(child_dc, pen);
    CHECK(RoundRect(child_dc, 10, 10, 30, 30, 6, 6));
    CHECK(GetPixel(child_dc, 10, 10) == RGB(0, 0, 0));
    CHECK(GetPixel(child_dc, 20, 20) == RGB(255, 0, 0));
    SelectObject(child_dc, GetStockObject(BLACK_BRUSH));
    SelectObject(child_dc, GetStockObject(BLACK_PEN));
    CHECK(DeleteObject(brush));
    CHECK(DeleteObject(pen));
  }
  palette_data.entries[42].peGreen = 255;
  CHECK(SetPaletteEntries(palette, 42, 1, &palette_data.entries[42]) == 1);
  CHECK(shandalar_dc_copy_rgba(child_dc, rgba_pixels, 100 * sizeof(rgba_pixels[0])));
  CHECK(rgba_pixels[0] == 0xff00ff00u);
  memset(&bitmap_info, 0, sizeof(bitmap_info));
  bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
  bitmap_info.bmiHeader.biWidth = 2;
  bitmap_info.bmiHeader.biHeight = -2;
  bitmap_info.bmiHeader.biPlanes = 1;
  bitmap_info.bmiHeader.biBitCount = 24;
  bitmap_info.bmiHeader.biCompression = BI_RGB;
  bitmap = CreateDIBSection(NULL, &bitmap_info, DIB_RGB_COLORS,
                            &bitmap_bits, NULL, 0);
  bitmap_dc = CreateCompatibleDC(NULL);
  CHECK(bitmap && bitmap_dc && bitmap_bits);
  SelectObject(bitmap_dc, bitmap);
  memset(bitmap_bits, 0x7f, 16);
  SetRect(&rect, 0, 0, 2, 2);
  CHECK(FillRect(bitmap_dc, &rect, GetStockObject(BLACK_BRUSH)));
  CHECK(shandalar_dc_copy_rgba(bitmap_dc, rgba_pixels,
                               2 * sizeof(rgba_pixels[0])));
  CHECK(rgba_pixels[0] == 0xff000000u);
  CHECK(rgba_pixels[1] == 0xff000000u);
  CHECK(rgba_pixels[2] == 0xff000000u);
  CHECK(rgba_pixels[3] == 0xff000000u);
  CHECK(DeleteDC(bitmap_dc));
  CHECK(DeleteObject(bitmap));
  CHECK(ReleaseDC(child, child_dc));
  CHECK(GetParent(child) == parent);
  CHECK(GetDlgItem(parent, 42) == child);
  CHECK(GetDlgCtrlID(child) == 42);
  CHECK(GetWindowTextA(child, text, sizeof(text)) == 5);
  CHECK(strcmp(text, "child") == 0);
  CHECK(SetWindowTextA(child, "updated"));
  CHECK(text_changes == 1);
  CHECK(GetWindowTextA(child, text, sizeof(text)) == 7);
  CHECK(strcmp(text, "updated") == 0);
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
  UpdateWindow(child);
  CHECK(SetWindowPos(child, NULL, 0, 0, 90, 45,
                     SWP_NOMOVE | SWP_NOZORDER));
  CHECK(UpdateWindow(child));
  thread_timer = SetTimer(NULL, 0, 1, NULL);
  CHECK(thread_timer != 0);
  Sleep(2);
  shandalar_win32_runtime_tick();
  CHECK(PeekMessageA(&message, NULL, WM_TIMER, WM_TIMER, PM_REMOVE));
  CHECK(message.hwnd == NULL && message.wParam == thread_timer);
  CHECK(KillTimer(NULL, thread_timer));
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
  CHECK(DestroyWindow(dialog));
  CHECK(DestroyWindow(parent));
  CHECK(DestroyWindow(caption_window));
  CHECK(DestroyWindow(double_click_window));
  CHECK(DeleteObject(palette));
  CHECK(destroys == 6); /* Built-in controls use their own window procedure. */
  CHECK(!IsWindow(parent) && !IsWindow(child));
  shandalar_win32_reset();
  return 0;
}
