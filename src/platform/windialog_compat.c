#include "platform/winuser_compat.h"

#include <stdio.h>
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

typedef struct DialogFactoryRegistration {
  struct DialogFactoryRegistration *next;
  UINT resource_id;
  ShandalarDialogFactory factory;
} DialogFactoryRegistration;

static DialogFactoryRegistration *g_factories;

static BOOL dialog_focusable(HWND window)
{
  char class_name[32];
  if (!window || !IsWindowVisible(window) || !IsWindowEnabled(window))
    return FALSE;
  if (!GetClassNameA(window, class_name, sizeof(class_name)))
    return FALSE;
  return SDL_strcasecmp(class_name, "BUTTON") == 0 ||
         SDL_strcasecmp(class_name, "EDIT") == 0 ||
         SDL_strcasecmp(class_name, "LISTBOX") == 0 ||
         SDL_strcasecmp(class_name, "COMBOBOX") == 0 ||
         SDL_strcasecmp(class_name, "msctls_updown32") == 0;
}

static BOOL handle_dialog_key(HWND dialog, const MSG *message)
{
  HWND focus;
  HWND next;
  if (message->message != WM_KEYDOWN)
    return FALSE;
  focus = GetFocus();
  if (message->wParam == VK_RETURN) {
    int id = focus ? GetDlgCtrlID(focus) : IDOK;
    if (!GetDlgItem(dialog, id))
      id = IDOK;
    SendMessageA(dialog, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), (LPARAM)focus);
    return TRUE;
  }
  if (message->wParam == VK_ESCAPE) {
    SendMessageA(dialog, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), 0);
    return TRUE;
  }
  if (message->wParam != VK_TAB)
    return FALSE;
  next = focus;
  do {
    next = next ? GetWindow(next, (GetKeyState(VK_SHIFT) & 0x8000) ?
                                  GW_HWNDPREV : GW_HWNDNEXT) : NULL;
    if (!next)
      next = GetWindow(dialog, GW_CHILD);
    if (dialog_focusable(next)) {
      SetFocus(next);
      return TRUE;
    }
  } while (next && next != focus);
  return TRUE;
}

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
  if (message == WM_ERASEBKGND) {
    RECT client;
    if (GetClientRect(window, &client))
      FillRect((HDC)wparam, &client, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
    return TRUE;
  }
  return DefWindowProcA(window, message, wparam, lparam);
}

HWND shandalar_win32_create_dialog(HINSTANCE instance, HWND parent, LPCSTR title,
                                   int x, int y, int width, int height)
{
  WNDCLASSA window_class;
  memset(&window_class, 0, sizeof(window_class));
  window_class.lpfnWndProc = dialog_window_proc;
  window_class.cbWndExtra = DLGWINDOWEXTRA;
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
  DialogFactoryRegistration *registration;
  UINT resource_id = (UINT)(uintptr_t)template_name & 0xffffu;
  HWND window = NULL;
  for (registration = g_factories; registration; registration = registration->next) {
    if (registration->resource_id == resource_id) {
      window = registration->factory(instance, template_name, parent);
      break;
    }
  }
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
    if (handle_dialog_key(window, &message))
      continue;
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

BOOL SetDlgItemTextA(HWND dialog, int id, LPCSTR text)
{
  HWND item = GetDlgItem(dialog, id);
  return item ? SetWindowTextA(item, text) : FALSE;
}

UINT GetDlgItemTextA(HWND dialog, int id, LPSTR text, int max_count)
{
  HWND item = GetDlgItem(dialog, id);
  return item ? (UINT)GetWindowTextA(item, text, max_count) : 0;
}

BOOL SetDlgItemInt(HWND dialog, int id, UINT value, BOOL is_signed)
{
  char text[32];
  if (is_signed)
    snprintf(text, sizeof(text), "%d", (int)value);
  else
    snprintf(text, sizeof(text), "%u", value);
  return SetDlgItemTextA(dialog, id, text);
}

UINT GetDlgItemInt(HWND dialog, int id, BOOL *translated, BOOL is_signed)
{
  char text[32];
  char *end;
  unsigned long value;
  if (!GetDlgItemTextA(dialog, id, text, sizeof(text))) {
    if (translated)
      *translated = FALSE;
    return 0;
  }
  value = is_signed ? (unsigned long)strtol(text, &end, 10)
                    : strtoul(text, &end, 10);
  if (translated)
    *translated = end != text && *end == '\0';
  return (UINT)value;
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

BOOL CheckRadioButton(HWND dialog, int first_id, int last_id, int check_id)
{
  int id;
  BOOL found = FALSE;
  for (id = first_id; id <= last_id; ++id) {
    HWND button = GetDlgItem(dialog, id);
    if (button) {
      SendMessageA(button, BM_SETCHECK,
                   id == check_id ? BST_CHECKED : BST_UNCHECKED, 0);
      if (id == check_id)
        found = TRUE;
    }
  }
  return found;
}

void shandalar_win32_set_dialog_factory(ShandalarDialogFactory factory)
{
  (void)factory;
}

BOOL shandalar_win32_register_dialog_factory(UINT resource_id,
                                             ShandalarDialogFactory factory)
{
  DialogFactoryRegistration *registration;
  if (!factory)
    return FALSE;
  for (registration = g_factories; registration; registration = registration->next) {
    if (registration->resource_id == resource_id) {
      registration->factory = factory;
      return TRUE;
    }
  }
  registration = (DialogFactoryRegistration *)malloc(sizeof(*registration));
  if (!registration)
    return FALSE;
  registration->resource_id = resource_id;
  registration->factory = factory;
  registration->next = g_factories;
  g_factories = registration;
  return TRUE;
}
