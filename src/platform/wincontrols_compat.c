#include "platform/winuser_compat.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct ControlItem {
  char *text;
  LPARAM data;
  BOOL selected;
} ControlItem;

typedef struct ControlState {
  struct ControlState *next;
  HWND window;
  ControlItem *items;
  size_t count;
  size_t capacity;
  int current;
  int top;
  UINT check;
  UINT text_limit;
  int updown_min;
  int updown_max;
  int updown_position;
} ControlState;

static ControlState *g_controls;

static char *copy_text(const char *text)
{
  size_t size = strlen(text ? text : "") + 1;
  char *copy = (char *)malloc(size);
  if (copy)
    memcpy(copy, text ? text : "", size);
  return copy;
}

static ControlState *get_state(HWND window, BOOL create)
{
  ControlState *state;
  for (state = g_controls; state; state = state->next)
    if (state->window == window)
      return state;
  if (!create)
    return NULL;
  state = (ControlState *)calloc(1, sizeof(*state));
  if (!state)
    return NULL;
  state->window = window;
  state->current = -1;
  state->next = g_controls;
  g_controls = state;
  return state;
}

static BOOL reserve_items(ControlState *state, size_t needed)
{
  ControlItem *items;
  size_t capacity;
  if (needed <= state->capacity)
    return TRUE;
  capacity = state->capacity ? state->capacity * 2 : 16;
  while (capacity < needed)
    capacity *= 2;
  items = (ControlItem *)realloc(state->items, capacity * sizeof(*items));
  if (!items)
    return FALSE;
  memset(items + state->capacity, 0, (capacity - state->capacity) * sizeof(*items));
  state->items = items;
  state->capacity = capacity;
  return TRUE;
}

static LRESULT insert_item(ControlState *state, int index, LPCSTR text)
{
  if (index < 0 || (size_t)index > state->count)
    index = (int)state->count;
  if (!reserve_items(state, state->count + 1))
    return -1;
  memmove(&state->items[index + 1], &state->items[index],
          (state->count - (size_t)index) * sizeof(*state->items));
  memset(&state->items[index], 0, sizeof(state->items[index]));
  state->items[index].text = copy_text(text);
  if (!state->items[index].text)
    return -1;
  ++state->count;
  return index;
}

static void clear_items(ControlState *state)
{
  size_t i;
  for (i = 0; i < state->count; ++i)
    free(state->items[i].text);
  state->count = 0;
  state->current = -1;
  state->top = 0;
}

static BOOL valid_index(ControlState *state, WPARAM index)
{
  return state && (size_t)index < state->count;
}

static int sorted_insert_index(ControlState *state, LPCSTR text)
{
  size_t index;
  const char *candidate = text ? text : "";
  for (index = 0; index < state->count; ++index)
    if (SDL_strcasecmp(candidate, state->items[index].text) < 0)
      return (int)index;
  return (int)state->count;
}

static int set_updown_position(ControlState *state, HWND window, int position)
{
  HWND buddy;
  char text[32];
  if (position < state->updown_min)
    position = state->updown_min;
  if (position > state->updown_max)
    position = state->updown_max;
  state->updown_position = position;
  buddy = GetWindow(window, GW_HWNDNEXT);
  if (buddy) {
    SDL_snprintf(text, sizeof(text), "%d", position);
    SetWindowTextA(buddy, text);
  }
  return position;
}

static BOOL read_updown_buddy(ControlState *state, HWND window)
{
  HWND buddy = GetWindow(window, GW_HWNDNEXT);
  char text[32];
  char *end;
  long position;
  if (!buddy)
    return TRUE;
  GetWindowTextA(buddy, text, sizeof(text));
  errno = 0;
  position = strtol(text, &end, 10);
  if (text == end || *end != '\0' || errno == ERANGE ||
      position < state->updown_min || position > state->updown_max ||
      position < INT_MIN || position > INT_MAX)
    return FALSE;
  state->updown_position = (int)position;
  return TRUE;
}

LRESULT shandalar_control_message(HWND window, UINT message, WPARAM wparam,
                                 LPARAM lparam, BOOL *handled)
{
  char class_name[32];
  ControlState *state;
  BOOL is_list;
  BOOL is_combo;
  *handled = FALSE;
  if (!GetClassNameA(window, class_name, sizeof(class_name)))
    return 0;
  is_list = SDL_strcasecmp(class_name, "LISTBOX") == 0;
  is_combo = SDL_strcasecmp(class_name, "COMBOBOX") == 0;
  if (SDL_strcasecmp(class_name, "msctls_updown32") == 0) {
    state = get_state(window, TRUE);
    switch (message) {
    case WM_USER + 101:
      state->updown_max = (short)LOWORD(lparam);
      state->updown_min = (short)HIWORD(lparam);
      *handled = TRUE;
      return 0;
    case WM_USER + 102:
      *handled = TRUE;
      return MAKELONG(state->updown_max, state->updown_min);
    case WM_USER + 103:
      *handled = TRUE;
      return set_updown_position(state, window, (short)LOWORD(lparam));
    case WM_USER + 104:
      {
        BOOL valid = read_updown_buddy(state, window);
        *handled = TRUE;
        return MAKELONG(state->updown_position, valid ? 0 : 1);
      }
    case UDM_SETRANGE32:
      state->updown_min = (int)wparam;
      state->updown_max = (int)lparam;
      *handled = TRUE;
      return 0;
    case UDM_GETRANGE32:
      if (wparam) *(int *)wparam = state->updown_min;
      if (lparam) *(int *)lparam = state->updown_max;
      *handled = TRUE;
      return 0;
    case UDM_SETPOS32:
      *handled = TRUE;
      return set_updown_position(state, window, (int)lparam);
    case UDM_GETPOS32:
      {
        BOOL valid = read_updown_buddy(state, window);
        if (lparam) *(BOOL *)lparam = !valid;
        *handled = TRUE;
        return state->updown_position;
      }
    default:
      return 0;
    }
  }
  if (SDL_strcasecmp(class_name, "BUTTON") == 0) {
    state = get_state(window, TRUE);
    if (message == BM_SETCHECK) {
      *handled = TRUE;
      state->check = (UINT)wparam;
      return 0;
    }
    if (message == BM_GETCHECK) {
      *handled = TRUE;
      return state->check;
    }
    return 0;
  }
  if (SDL_strcasecmp(class_name, "EDIT") == 0 && message == EM_LIMITTEXT) {
    *handled = TRUE;
    get_state(window, TRUE)->text_limit = (UINT)wparam;
    return 0;
  }
  if (!is_list && !is_combo)
    return 0;

  state = get_state(window, TRUE);
  switch (message) {
  case LB_ADDSTRING:
  case CB_ADDSTRING:
    *handled = TRUE;
    if ((is_list && (GetWindowLongA(window, GWL_STYLE) & LBS_SORT)) ||
        (is_combo && (GetWindowLongA(window, GWL_STYLE) & CBS_SORT)))
      return insert_item(state, sorted_insert_index(state, (LPCSTR)lparam),
                         (LPCSTR)lparam);
    return insert_item(state, (int)state->count, (LPCSTR)lparam);
  case LB_INSERTSTRING:
  case CB_INSERTSTRING:
    *handled = TRUE;
    return insert_item(state, (int)wparam, (LPCSTR)lparam);
  case LB_DELETESTRING:
  case CB_DELETESTRING:
    *handled = TRUE;
    if (!valid_index(state, wparam)) return -1;
    free(state->items[wparam].text);
    memmove(&state->items[wparam], &state->items[wparam + 1],
            (state->count - (size_t)wparam - 1) * sizeof(*state->items));
    --state->count;
    if (state->current >= (int)state->count) state->current = (int)state->count - 1;
    return (LRESULT)state->count;
  case LB_RESETCONTENT:
  case CB_RESETCONTENT:
    *handled = TRUE;
    clear_items(state);
    return 0;
  case LB_GETCOUNT:
  case CB_GETCOUNT:
    *handled = TRUE;
    return (LRESULT)state->count;
  case LB_SETCURSEL:
  case CB_SETCURSEL:
    *handled = TRUE;
    state->current = valid_index(state, wparam) ? (int)wparam : -1;
    return state->current;
  case LB_GETCURSEL:
  case CB_GETCURSEL:
    *handled = TRUE;
    return state->current;
  case LB_GETSEL:
    *handled = TRUE;
    return valid_index(state, wparam) ? state->items[wparam].selected : -1;
  case LB_GETTEXT:
  case CB_GETLBTEXT:
    *handled = TRUE;
    if (!valid_index(state, wparam) || !lparam) return -1;
    strcpy((char *)lparam, state->items[wparam].text);
    return (LRESULT)strlen(state->items[wparam].text);
  case LB_GETITEMDATA:
    *handled = TRUE;
    return valid_index(state, wparam) ? state->items[wparam].data : -1;
  case LB_SETITEMDATA:
    *handled = TRUE;
    if (!valid_index(state, wparam)) return -1;
    state->items[wparam].data = lparam;
    return 0;
  case LB_SETSEL:
    *handled = TRUE;
    if (!valid_index(state, lparam)) return -1;
    state->items[lparam].selected = wparam != 0;
    return 0;
  case LB_GETSELITEMS: {
    int *indices = (int *)lparam;
    int copied = 0;
    size_t i;
    *handled = TRUE;
    for (i = 0; i < state->count && copied < (int)wparam; ++i)
      if (state->items[i].selected)
        indices[copied++] = (int)i;
    return copied;
  }
  case LB_SETTOPINDEX:
    *handled = TRUE;
    state->top = valid_index(state, wparam) ? (int)wparam : 0;
    return 0;
  case LB_GETTOPINDEX:
    *handled = TRUE;
    return state->top;
  case LB_SETCARETINDEX:
    *handled = TRUE;
    state->current = valid_index(state, wparam) ? (int)wparam : -1;
    return 0;
  case LB_SETITEMHEIGHT:
  case LB_DIR:
    *handled = TRUE;
    return 0;
  default:
    return 0;
  }
}

void shandalar_controls_reset(void)
{
  while (g_controls) {
    ControlState *next = g_controls->next;
    clear_items(g_controls);
    free(g_controls->items);
    free(g_controls);
    g_controls = next;
  }
}
