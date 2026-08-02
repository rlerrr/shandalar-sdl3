#include "platform/winuser_compat.h"

#include <stdlib.h>
#include <string.h>

typedef struct MenuItem {
  UINT flags;
  UINT_PTR id;
  char *text;
} MenuItem;

typedef struct CompatMenu {
  uint32_t magic;
  MenuItem *items;
  size_t count;
  size_t capacity;
} CompatMenu;

#define MENU_MAGIC 0x4d454e55u

static CompatMenu *g_active_popup;
static HWND g_popup_owner;
static int g_popup_x;
static int g_popup_y;

static CompatMenu *as_menu(HMENU handle)
{
  CompatMenu *menu = (CompatMenu *)handle;
  return menu && menu->magic == MENU_MAGIC ? menu : NULL;
}

static int find_item(CompatMenu *menu, UINT item, UINT flags);

static CompatMenu *clone_menu(CompatMenu *source)
{
  CompatMenu *copy;
  size_t i;
  if (!source)
    return NULL;
  copy = (CompatMenu *)CreatePopupMenu();
  if (!copy)
    return NULL;
  for (i = 0; i < source->count; ++i)
    if (!AppendMenuA((HMENU)copy, source->items[i].flags, source->items[i].id,
                     source->items[i].text)) {
      DestroyMenu((HMENU)copy);
      return NULL;
    }
  return copy;
}

HMENU CreatePopupMenu(void)
{
  CompatMenu *menu = (CompatMenu *)calloc(1, sizeof(*menu));
  if (menu)
    menu->magic = MENU_MAGIC;
  return (HMENU)menu;
}

BOOL AppendMenuA(HMENU handle, UINT flags, UINT_PTR item, LPCSTR text)
{
  CompatMenu *menu = as_menu(handle);
  MenuItem *items;
  if (!menu)
    return FALSE;
  if (menu->count == menu->capacity) {
    size_t capacity = menu->capacity ? menu->capacity * 2 : 8;
    items = (MenuItem *)realloc(menu->items, capacity * sizeof(*items));
    if (!items)
      return FALSE;
    menu->items = items;
    menu->capacity = capacity;
  }
  memset(&menu->items[menu->count], 0, sizeof(menu->items[menu->count]));
  menu->items[menu->count].flags = flags;
  menu->items[menu->count].id = item;
  if (text) {
    size_t size = strlen(text) + 1;
    menu->items[menu->count].text = (char *)malloc(size);
    if (menu->items[menu->count].text)
      memcpy(menu->items[menu->count].text, text, size);
  }
  ++menu->count;
  return TRUE;
}

BOOL InsertMenuA(HMENU handle, UINT position, UINT flags, UINT_PTR item, LPCSTR text)
{
  CompatMenu *menu = as_menu(handle);
  MenuItem inserted;
  int index;
  if (!menu)
    return FALSE;
  index = find_item(menu, position, flags);
  if (index < 0)
    return AppendMenuA(handle, flags, item, text);
  if (!AppendMenuA(handle, flags, item, text))
    return FALSE;
  inserted = menu->items[menu->count - 1];
  memmove(&menu->items[index + 1], &menu->items[index],
          (menu->count - (size_t)index - 1) * sizeof(*menu->items));
  menu->items[index] = inserted;
  return TRUE;
}

BOOL ModifyMenuA(HMENU handle, UINT position, UINT flags, UINT_PTR item, LPCSTR text)
{
  CompatMenu *menu = as_menu(handle);
  int index;
  char *copy = NULL;
  if (!menu || (index = find_item(menu, position, flags)) < 0)
    return FALSE;
  if (text) {
    size_t size = strlen(text) + 1;
    copy = (char *)malloc(size);
    if (!copy)
      return FALSE;
    memcpy(copy, text, size);
  }
  free(menu->items[index].text);
  menu->items[index].flags = flags;
  menu->items[index].id = item;
  menu->items[index].text = copy;
  return TRUE;
}

static int find_item(CompatMenu *menu, UINT item, UINT flags)
{
  size_t i;
  if (flags & MF_BYPOSITION)
    return item < menu->count ? (int)item : -1;
  for (i = 0; i < menu->count; ++i)
    if (menu->items[i].id == item)
      return (int)i;
  return -1;
}

UINT CheckMenuItem(HMENU handle, UINT item, UINT flags)
{
  CompatMenu *menu = as_menu(handle);
  int index;
  UINT old;
  if (!menu || (index = find_item(menu, item, flags)) < 0)
    return (UINT)-1;
  old = menu->items[index].flags;
  menu->items[index].flags &= ~MF_CHECKED;
  menu->items[index].flags |= flags & MF_CHECKED;
  return old;
}

UINT EnableMenuItem(HMENU handle, UINT item, UINT flags)
{
  CompatMenu *menu = as_menu(handle);
  int index;
  UINT old;
  if (!menu || (index = find_item(menu, item, flags)) < 0)
    return (UINT)-1;
  old = menu->items[index].flags;
  menu->items[index].flags &= ~(MF_DISABLED | MF_GRAYED);
  menu->items[index].flags |= flags & (MF_DISABLED | MF_GRAYED);
  return old;
}

static BOOL remove_item(HMENU handle, UINT item, UINT flags)
{
  CompatMenu *menu = as_menu(handle);
  int index;
  if (!menu || (index = find_item(menu, item, flags)) < 0)
    return FALSE;
  free(menu->items[index].text);
  memmove(&menu->items[index], &menu->items[index + 1],
          (menu->count - (size_t)index - 1) * sizeof(*menu->items));
  --menu->count;
  return TRUE;
}

BOOL DeleteMenu(HMENU menu, UINT item, UINT flags) { return remove_item(menu, item, flags); }
BOOL RemoveMenu(HMENU menu, UINT item, UINT flags) { return remove_item(menu, item, flags); }

BOOL DestroyMenu(HMENU handle)
{
  CompatMenu *menu = as_menu(handle);
  size_t i;
  if (!menu)
    return FALSE;
  for (i = 0; i < menu->count; ++i)
    free(menu->items[i].text);
  free(menu->items);
  menu->magic = 0;
  free(menu);
  return TRUE;
}

int GetMenuItemCount(HMENU handle)
{
  CompatMenu *menu = as_menu(handle);
  return menu ? (int)menu->count : -1;
}

BOOL TrackPopupMenu(HMENU handle, UINT flags, int x, int y, int reserved,
                    HWND window, const RECT *exclude)
{
  CompatMenu *menu = as_menu(handle);
  (void)flags; (void)reserved; (void)exclude;
  if (!menu || !IsWindow(window))
    return FALSE;
  shandalar_win32_dismiss_popup();
  SendMessageA(window, WM_INITMENU, (WPARAM)handle, 0);
  g_active_popup = clone_menu(menu);
  if (!g_active_popup)
    return FALSE;
  g_popup_owner = window;
  g_popup_x = x;
  g_popup_y = y;
  return TRUE;
}

HMENU shandalar_win32_active_popup(HWND *owner, int *x, int *y)
{
  if (owner) *owner = g_popup_owner;
  if (x) *x = g_popup_x;
  if (y) *y = g_popup_y;
  return (HMENU)g_active_popup;
}

int shandalar_win32_menu_item_count(HMENU handle)
{
  CompatMenu *menu = as_menu(handle);
  return menu ? (int)menu->count : 0;
}

BOOL shandalar_win32_menu_item(HMENU handle, int index, UINT *flags,
                               UINT_PTR *id, LPCSTR *text)
{
  CompatMenu *menu = as_menu(handle);
  if (!menu || index < 0 || (size_t)index >= menu->count)
    return FALSE;
  if (flags) *flags = menu->items[index].flags;
  if (id) *id = menu->items[index].id;
  if (text) *text = menu->items[index].text ? menu->items[index].text : "";
  return TRUE;
}

void shandalar_win32_dismiss_popup(void)
{
  CompatMenu *menu = g_active_popup;
  HWND owner = g_popup_owner;
  g_active_popup = NULL;
  g_popup_owner = NULL;
  /*
   * Native TrackPopupMenu() does not return until the menu closes.  The duel
   * window procedures use WM_MENUSELECT's closing notification to empty their
   * reusable popup menus after TrackPopupMenu() returns.  Our popup is
   * asynchronous, so synthesize that notification here instead.
   */
  if (owner)
    SendMessageA(owner, WM_MENUSELECT, MAKEWPARAM(0, 0xffff), 0);
  if (menu)
    DestroyMenu((HMENU)menu);
}

void shandalar_win32_choose_menu_item(UINT_PTR id)
{
  HWND owner = g_popup_owner;
  shandalar_win32_dismiss_popup();
  if (owner)
    PostMessageA(owner, WM_COMMAND, (WPARAM)id, 0);
}
