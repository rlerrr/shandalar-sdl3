#include "deckdll.h"

extern HINSTANCE global_hinstance;
#include "resources.h"

#ifdef SHANDALAR_SDL_WIN32_COMPAT

#define SX(value) ((value) * 3 / 2)
#define SY(value) ((value) * 5 / 3)
#define NOSYS_DLG_STYLE (WS_POPUP | WS_CAPTION)
#define DFLT_DLG_STYLE (NOSYS_DLG_STYLE | WS_SYSMENU)

static HWND add_control(HWND dialog, const char *class_name, int id,
                        int x, int y, int width, int height, DWORD style)
{
  return CreateWindowExA(0, class_name, "", WS_CHILD | WS_VISIBLE | style,
                         SX(x), SY(y),
                         SX(width), SY(height), dialog,
                         (HMENU)(intptr_t)id, global_hinstance, NULL);
}

static HWND make_dialog(HINSTANCE instance, HWND parent, int width, int height,
                        DWORD style)
{
  RECT desktop;
  HWND dialog;
  int pixel_width = SX(width);
  int pixel_height = SY(height) +
                     (((style & WS_CAPTION) == WS_CAPTION) ?
                          GetSystemMetrics(SM_CYCAPTION) : 0);
  GetClientRect(GetDesktopWindow(), &desktop);
  dialog = shandalar_win32_create_dialog(instance, parent, "",
                                         (desktop.right - pixel_width) / 2,
                                         (desktop.bottom - pixel_height) / 2,
                                         pixel_width, pixel_height);
  if (dialog)
    SetWindowLongA(dialog, GWL_STYLE, style);
  return dialog;
}

static HWND create_deck_dialog(HINSTANCE instance, LPCSTR template_name, HWND parent)
{
  unsigned int id = (unsigned int)(uintptr_t)template_name & 0xffffu;
  HWND dialog;
  switch (id) {
  case RES_DIALOG_ASK_VALUE:
    dialog = make_dialog(instance, parent, 114, 61, DFLT_DLG_STYLE);
    add_control(dialog, "EDIT", RES_ASKVALUE_EDITTEXT, 42, 10, 30, 12, WS_BORDER);
    add_control(dialog, "BUTTON", RES_BUTTON_OK, 10, 35, 35, 14, BS_DEFPUSHBUTTON);
    add_control(dialog, "BUTTON", RES_BUTTON_CANCEL, 60, 35, 45, 14, 0);
    return dialog;

  case RES_DIALOG_FILTER_LIST:
    dialog = make_dialog(instance, parent, 164, 216, DFLT_DLG_STYLE);
    add_control(dialog, "LISTBOX", RES_FILTERLIST_LISTBOX, 5, 5, 155, 135,
                WS_BORDER | WS_VSCROLL | WS_HSCROLL | LBS_SORT | LBS_MULTIPLESEL);
    add_control(dialog, "BUTTON", RES_BUTTON_OK, 10, 190, 65, 14, BS_DEFPUSHBUTTON);
    add_control(dialog, "BUTTON", RES_BUTTON_CANCEL, 90, 190, 65, 14, 0);
    add_control(dialog, "BUTTON", RES_FILTERLIST_ENABLEFILTER, 5, 145, 106, 10,
                BS_AUTOCHECKBOX);
    add_control(dialog, "BUTTON", RES_FILTERLIST_SELECTALL, 10, 165, 65, 15, 0);
    add_control(dialog, "BUTTON", RES_FILTERLIST_CLEARALL, 90, 165, 65, 15, 0);
    return dialog;

  case RES_DIALOG_DECKINFO:
    dialog = make_dialog(instance, parent, 279, 180, DFLT_DLG_STYLE);
    add_control(dialog, "EDIT", RES_DECKINFO_DECKTITLE_EDITTEXT, 65, 15, 185, 12, WS_BORDER);
    add_control(dialog, "EDIT", RES_DECKINFO_DESCRIPTION_EDITTEXT, 65, 30, 185, 12, WS_BORDER);
    add_control(dialog, "EDIT", RES_DECKINFO_NAME_EDITTEXT, 65, 45, 185, 12, WS_BORDER);
    add_control(dialog, "EDIT", RES_DECKINFO_EMAIL_EDITTEXT, 65, 60, 185, 12, WS_BORDER);
    add_control(dialog, "EDIT", RES_DECKINFO_DATE_EDITTEXT, 65, 75, 86, 12, WS_BORDER);
    add_control(dialog, "EDIT", RES_DECKINFO_VERSION_EDITTEXT, 190, 75, 60, 12,
                WS_BORDER | ES_READONLY);
    add_control(dialog, "EDIT", RES_DECKINFO_COMMENT_EDITTEXT, 65, 105, 185, 50,
                WS_BORDER | ES_MULTILINE);
    add_control(dialog, "COMBOBOX", RES_DECKINFO_DECKFACES_COMBOBOX, 65, 90, 185, 14,
                CBS_DROPDOWNLIST);
    add_control(dialog, "STATIC", RES_DECKINFO_DECKTITLE, 5, 15, 55, 10, 0);
    add_control(dialog, "STATIC", RES_DECKINFO_DESCRIPTION, 5, 30, 55, 10, 0);
    add_control(dialog, "STATIC", RES_DECKINFO_NAME, 5, 45, 55, 10, 0);
    add_control(dialog, "STATIC", RES_DECKINFO_EMAIL, 5, 60, 55, 10, 0);
    add_control(dialog, "STATIC", RES_DECKINFO_DATE, 5, 75, 55, 10, 0);
    add_control(dialog, "STATIC", RES_DECKINFO_VERSION, 160, 75, 25, 10, 0);
    add_control(dialog, "STATIC", RES_DECKINFO_DECKFACES, 5, 90, 55, 10, 0);
    add_control(dialog, "STATIC", RES_DECKINFO_COMMENT, 5, 105, 55, 10, 0);
    add_control(dialog, "BUTTON", RES_BUTTON_OK, 60, 160, 50, 14, BS_DEFPUSHBUTTON);
    add_control(dialog, "BUTTON", RES_BUTTON_CANCEL, 175, 160, 50, 14, 0);
    return dialog;

  case RES_DIALOG_DECKSTATS:
    return make_dialog(instance, parent, 270, 179, DFLT_DLG_STYLE);

  case RES_DIALOG_INFOBOX:
    dialog = make_dialog(instance, parent, 219, 151, NOSYS_DLG_STYLE);
    add_control(dialog, "BUTTON", RES_BUTTON_OK, 25, 130, 85, 14, BS_DEFPUSHBUTTON);
    add_control(dialog, "BUTTON", RES_BUTTON_CANCEL, 135, 130, 60, 14, 0);
    add_control(dialog, "STATIC", RES_INFOBOX_DESCRIPTION, 20, 15, 190, 10, 0);
    add_control(dialog, "LISTBOX", RES_INFOBOX_LIST, 25, 30, 170, 95,
                WS_BORDER | WS_VSCROLL | LBS_STANDARD | LBS_NOINTEGRALHEIGHT);
    return dialog;

  case RES_DIALOG_GROUPMOVE: {
    static const int button_ids[] = {
      RES_GROUPMOVE_BUTTON_BLACK, RES_GROUPMOVE_BUTTON_BLUE,
      RES_GROUPMOVE_BUTTON_GREEN, RES_GROUPMOVE_BUTTON_RED,
      RES_GROUPMOVE_BUTTON_WHITE, RES_GROUPMOVE_BUTTON_ARTIFACT
    };
    int i;
    dialog = make_dialog(instance, parent, 164, 136, NOSYS_DLG_STYLE);
    for (i = 0; i < 5; ++i)
      add_control(dialog, "BUTTON", button_ids[i], 45, 15 + i * 15, 95, 10,
                  BS_AUTOCHECKBOX);
    add_control(dialog, "BUTTON", button_ids[5], 45, 90, 90, 10, BS_AUTOCHECKBOX);
    add_control(dialog, "BUTTON", RES_BUTTON_OK, 10, 110, 60, 14, BS_DEFPUSHBUTTON);
    add_control(dialog, "BUTTON", RES_BUTTON_CANCEL, 95, 110, 60, 14, 0);
    return dialog;
  }

  case RES_DIALOG_LOADDECK:
    dialog = make_dialog(instance, parent, 342, 94, DFLT_DLG_STYLE);
    add_control(dialog, "STATIC", RES_LOADDECK_PLAYERDECK, 9, 6, 47, 19, 0);
    add_control(dialog, "COMBOBOX", RES_LOADDECK_DECKLIST, 61, 6, 259, 14,
                WS_VSCROLL | CBS_DROPDOWNLIST | CBS_SORT);
    add_control(dialog, "BUTTON", RES_BUTTON_OK, 61, 68, 50, 14, BS_DEFPUSHBUTTON);
    add_control(dialog, "BUTTON", RES_BUTTON_CANCEL, 131, 68, 50, 14, 0);
    return dialog;
  default:
    return NULL;
  }
}

void deck_register_dialog_resources(void)
{
  static const UINT resource_ids[] = {
      RES_DIALOG_ASK_VALUE, RES_DIALOG_FILTER_LIST, RES_DIALOG_DECKINFO,
      RES_DIALOG_DECKSTATS, RES_DIALOG_INFOBOX, RES_DIALOG_GROUPMOVE,
      RES_DIALOG_LOADDECK};
  size_t index;
  for (index = 0; index < sizeof(resource_ids) / sizeof(resource_ids[0]); ++index)
    shandalar_win32_register_dialog_factory(resource_ids[index], create_deck_dialog);
}

#endif
