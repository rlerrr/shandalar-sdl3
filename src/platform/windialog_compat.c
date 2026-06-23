#include "platform/winuser_compat.h"

#include <stdlib.h>
#include <string.h>

typedef struct DialogState {
  struct DialogState *next;
  HWND window;
  DLGPROC proc;
  INT_PTR result;
  BOOL ended;
} DialogState;

static DialogState *g_dialogs;
static ShandalarDialogFactory g_factory;

static LRESULT CALLBACK dialog_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  DialogState *state;
  for (state = g_dialogs; state; state = state->next) {
    if (state->window == window) {
      LRESULT result = state->proc ? state->proc(window, message, wparam, lparam) : 0;
      if (message == WM_CLOSE && !state->ended) {
        if (state->proc)
          state->proc(window, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), 0);
        if (!state->ended) {
          state->result = IDCANCEL;
          state->ended = TRUE;
        }
        return 0;
      }
      if (result)
        return result;
      break;
    }
  }
  return DefWindowProcA(window, message, wparam, lparam);
}

HWND shandalar_win32_create_dialog(HINSTANCE instance, HWND parent, LPCSTR title,
                                   int x, int y, int width, int height)
{
  WNDCLASSA window_class;
  memset(&window_class, 0, sizeof(window_class));
  window_class.lpfnWndProc = dialog_window_proc;
  window_class.lpszClassName = "#32770";
  window_class.hInstance = instance;
  RegisterClassA(&window_class);
  return CreateWindowExA(0, "#32770", title ? title : "", WS_POPUP | WS_CAPTION,
                         x, y, width, height, parent, NULL, instance, NULL);
}

INT_PTR DialogBoxParamA(HINSTANCE instance, LPCSTR template_name, HWND parent,
                        DLGPROC dialog_proc, LPARAM init_param)
{
  DialogState state;
  MSG message;
  HWND window = g_factory ? g_factory(instance, template_name, parent) : NULL;
  if (!window)
    window = shandalar_win32_create_dialog(instance, parent, "", 100, 100, 320, 200);
  if (!window)
    return -1;
  memset(&state, 0, sizeof(state));
  state.window = window;
  state.proc = dialog_proc;
  state.next = g_dialogs;
  g_dialogs = &state;
  if (parent)
    EnableWindow(parent, FALSE);
  dialog_proc(window, WM_INITDIALOG, 0, init_param);
  ShowWindow(window, SW_SHOW);
  while (!state.ended && GetMessageA(&message, NULL, 0, 0)) {
    TranslateMessage(&message);
    DispatchMessageA(&message);
  }
  if (IsWindow(window))
    DestroyWindow(window);
  if (parent) {
    EnableWindow(parent, TRUE);
    SetFocus(parent);
  }
  g_dialogs = state.next;
  return state.result;
}

BOOL EndDialog(HWND dialog, INT_PTR result)
{
  DialogState *state;
  for (state = g_dialogs; state; state = state->next) {
    if (state->window == dialog) {
      state->result = result;
      state->ended = TRUE;
      return TRUE;
    }
  }
  return FALSE;
}

HWND shandalar_win32_active_dialog(void)
{
  return g_dialogs && IsWindow(g_dialogs->window) ? g_dialogs->window : NULL;
}

LRESULT SendDlgItemMessageA(HWND dialog, int id, UINT message, WPARAM wparam, LPARAM lparam)
{
  return SendMessageA(GetDlgItem(dialog, id), message, wparam, lparam);
}

UINT IsDlgButtonChecked(HWND dialog, int id)
{
  return (UINT)SendDlgItemMessageA(dialog, id, BM_GETCHECK, 0, 0);
}

BOOL CheckDlgButton(HWND dialog, int id, UINT check)
{
  HWND button = GetDlgItem(dialog, id);
  if (!button)
    return FALSE;
  SendMessageA(button, BM_SETCHECK, check, 0);
  return TRUE;
}

void shandalar_win32_set_dialog_factory(ShandalarDialogFactory factory)
{
  g_factory = factory;
}
