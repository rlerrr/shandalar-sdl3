#include "platform/winuser_compat.h"

#include <stdlib.h>

typedef struct ScrollState {
  struct ScrollState *next;
  HWND window;
  int minimum[2];
  int maximum[2];
  int position[2];
} ScrollState;

static ScrollState *g_scroll_states;

static ScrollState *get_scroll(HWND window, int bar, BOOL create)
{
  ScrollState *state;
  if (!IsWindow(window) || (bar != SB_HORZ && bar != SB_VERT && bar != SB_CTL))
    return NULL;
  for (state = g_scroll_states; state; state = state->next)
    if (state->window == window)
      return state;
  if (!create)
    return NULL;
  state = (ScrollState *)calloc(1, sizeof(*state));
  if (!state)
    return NULL;
  state->window = window;
  state->next = g_scroll_states;
  g_scroll_states = state;
  return state;
}

static int bar_index(int bar) { return bar == SB_VERT ? 1 : 0; }

BOOL SetScrollRange(HWND window, int bar, int minimum, int maximum, BOOL redraw)
{
  ScrollState *state = get_scroll(window, bar, TRUE);
  int index = bar_index(bar);
  (void)redraw;
  if (!state) return FALSE;
  state->minimum[index] = minimum;
  state->maximum[index] = maximum;
  if (state->position[index] < minimum) state->position[index] = minimum;
  if (state->position[index] > maximum) state->position[index] = maximum;
  return TRUE;
}

BOOL GetScrollRange(HWND window, int bar, int *minimum, int *maximum)
{
  ScrollState *state = get_scroll(window, bar, FALSE);
  int index = bar_index(bar);
  if (!state) return FALSE;
  if (minimum) *minimum = state->minimum[index];
  if (maximum) *maximum = state->maximum[index];
  return TRUE;
}

int SetScrollPos(HWND window, int bar, int position, BOOL redraw)
{
  ScrollState *state = get_scroll(window, bar, TRUE);
  int index = bar_index(bar);
  int old;
  (void)redraw;
  if (!state) return 0;
  old = state->position[index];
  if (position < state->minimum[index]) position = state->minimum[index];
  if (position > state->maximum[index]) position = state->maximum[index];
  state->position[index] = position;
  return old;
}

int GetScrollPos(HWND window, int bar)
{
  ScrollState *state = get_scroll(window, bar, FALSE);
  return state ? state->position[bar_index(bar)] : 0;
}
