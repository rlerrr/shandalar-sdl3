// DeckDll: standalone and integrated deckbuilder.
// deckdll.c: primary interface to Magic.exe/Shandalar.exe

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include "platform/legacy_io.h"

#include "deckdll.h"
#include "full_deck.h"
#include "mystdbool.h"
#include "drawcardlib/src/pic.h"
#include "cardartlib/src/assert.h"
#include "cardartlib/src/palette.h"
#include "magsnd.h"
#include "drawcardlib/Drawcardlib.h"
#include "shared_resources.h"

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif

#ifndef WM_APP
#define WM_APP 0x8000
#endif

#define WM_DECKBUILDER_DRAG_SELL_HOVER (WM_APP + 0x442)

#define MAKELONG2(a, b) ((LONG)(((a) & 0xffff) | ((DWORD)((WORD)(b))) << 16))
#define MAKELONG3(a, b) ((LONG)(((a) & 0xffff) | ((DWORD)((b) & 0xffff)) << 16))

typedef ptrdiff_t INT_PTR;

typedef int (*Int_fn_int)(int);
typedef void (*Void_fn_int_int)(int, int);
void change_buttonclass_wndproc(HWND hwnd);
static int read_deck_line(FILE *file, char *dest);
static int is_deck_card_line(char *line);
static int is_deck_version_line(char *line);
void draw_item(DRAWITEMSTRUCT *item, HBRUSH brush, HANDLE background,
               HPEN pen1, HPEN pen2, COLORREF color, int focus, UINT format);
extern int global_num_artists;

// GLOBAL: DECKDLL 0x10030030
int HORZLIST_LASTPIC_INDEX = 0;
// GLOBAL: DECKDLL 0x10030034
int HORZLIST_ADDR_INDEX = 2;
// GLOBAL: DECKDLL 0x10030038
int HORZLIST_CURRSEL_INDEX = 6;
// GLOBAL: DECKDLL 0x1003003c
int HORZLIST_LEFTPIC_INDEX = 8;
// GLOBAL: DECKDLL 0x10030040
int HORZLIST_WIDTHPLUSSPACE_INDEX = 10;
// GLOBAL: DECKDLL 0x10030044
int HORZLIST_PICSINLINE_INDEX = 12;

// GLOBAL: DECKDLL 0x10030000
static int global_cardclass_csvid_offset = 0;
// GLOBAL: DECKDLL 0x10030004
static int global_cardclass_amount_offset = 4;

#define CARD_CSVID_INDEX 0
#define CARD_AMOUNT_INDEX 4

#define CUECARD_FONT_INDEX 0

// GLOBAL: DECKDLL 0x101a88f4
HINSTANCE global_hinstance = NULL;

// GLOBAL: DECKDLL 0x101a9710
static CRITICAL_SECTION global_critical_section_for_unknown;

// GLOBAL: DECKDLL 0x10146bb0
static char global_previous_directory[MAX_PATH + 1];
// GLOBAL: DECKDLL 0x101bc0d0
static char global_manalink_ini_path[MAX_PATH + 15];
// GLOBAL: DECKDLL 0x101a9240
static char global_playdeck_path[MAX_PATH + 15];
// GLOBAL: DECKDLL 0x10144f50
char global_duelart_path[MAX_PATH + 15];
// GLOBAL: DECKDLL 0x10143d80
static char global_dbart_path[264];

// GLOBAL: DECKDLL 0x10143fbc
DBFlags global_db_flags_1;
// GLOBAL: DECKDLL 0x1019f00c
DBFlags global_db_flags_2 = DBFLAGS_0;
// GLOBAL: DECKDLL 0x101bc0c4
bool global_cfg_consolidate = false;

// GLOBAL: DECKDLL 0x101a8a70
static char global_deck_filename[(MAX_PATH + 15) + 32 + 5];

// GLOBAL: DECKDLL 0x101a8c64
HWND global_listbox_hwnd;

// GLOBAL: DECKDLL 0x101a8c70
struct GlobalConfig_t global_cfg;

static bool global_cfg_no_frame = false;
static bool global_cfg_read_by_name = false;
bool global_cfg_view_all = false;

// GLOBAL: DECKDLL 0x1012e898
static int global_current_deck;

// GLOBAL: DECKDLL 0x10144370
static int *global_external_current_deck;

// GLOBAL: DECKDLL 0x101f1e80
int *deck = NULL;

// GLOBAL: DECKDLL 0x1012ed80
GlobalDeckEntry global_deck[1000];
// GLOBAL: DECKDLL 0x101a9694
int global_deck_num_entries = 0;
// GLOBAL: DECKDLL 0x1012e88c
static int global_deck_num_entries_copy;

// STATIC_ASSERT(sizeof(FullDeck) == 0x1464, full_deck_wrong_size);

// GLOBAL: DECKDLL 0x1019fed0
FullDeck global_edited_deck;

#define global_deck_num_cards (global_edited_deck.cards)
#define global_edited_deck_num_entries (global_edited_deck.total)

// GLOBAL: DECKDLL 0x10144710
static DeckEntry global_excessive_cards[84];

// GLOBAL: DECKDLL 0x1012ea0c
bool global_deck_was_edited;
// GLOBAL: DECKDLL 0x1014436c
static bool global_deckname_set;
// GLOBAL: DECKDLL 0x1019f010
static int global_deckbuilder_unknown_1019f010 = 0;

// GLOBAL: DECKDLL 0x1012ea10
static char global_ui_strings_filename[MAX_PATH + 1];

// GLOBAL: DECKDLL 0x101255a0
static char global_artistline[128];

// GLOBAL: DECKDLL 0x1012df44
static int global_unknown_1012df44;

// GLOBAL: DECKDLL 0x1012df34
static int global_supports_palette;

// GLOBAL: DECKDLL 0x101bbefc
static int global_color_depth = 0;

// GLOBAL: DECKDLL 0x1012dca0
GlobalDeckInfoBlob global_deckinfo;

// GLOBAL: DECKDLL 0x10142814
static char *global_external_deckname;

// GLOBAL: DECKDLL 0x101bc604
int global_dlg_parameter = 0;
// GLOBAL: DECKDLL 0x101423d0
int global_dlg_result;
// GLOBAL: DECKDLL 0x10143ae0
static char global_ask_x_dlg_title[52];
// GLOBAL: DECKDLL 0x101127e0
static char global_loaddeck_dlg_filename[MAX_PATH + 15 + 10];

static HBRUSH global_brush_gold1 = NULL;
static HBRUSH global_brush_gold2 = NULL;
// GLOBAL: DECKDLL 0x10104c94
static HBRUSH global_brush_mediumgrey;
// GLOBAL: DECKDLL 0x101a9654
static HBRUSH global_create_brush_0 = NULL;
// GLOBAL: DECKDLL 0x10144c40
static HBRUSH global_create_brush_1;
// GLOBAL: DECKDLL 0x1012d68c
static HBRUSH global_create_brush_2;
// GLOBAL: DECKDLL 0x10139f14
static HBRUSH global_create_brush_3;
// GLOBAL: DECKDLL 0x10142f00
static HBRUSH global_create_brush_4;
// GLOBAL: DECKDLL 0x1012e894
HBRUSH global_create_brush_5;
// GLOBAL: DECKDLL 0x10132c3c
HBRUSH global_create_brush_6;
// GLOBAL: DECKDLL 0x1012e890
static HANDLE global_delete_brush_handle_0;
// GLOBAL: DECKDLL 0x10142efc
static HANDLE global_delete_brush_handle_1;
// GLOBAL: DECKDLL 0x101bbf00
static HANDLE global_delete_brush_handle_2 = NULL;
// GLOBAL: DECKDLL 0x10139e2c
static HANDLE global_delete_brush_handle_3;
// GLOBAL: DECKDLL 0x1014470c
static HANDLE global_delete_brush_handle_4;
// GLOBAL: DECKDLL 0x1012e62c
static HANDLE global_delete_brush_handle_5;
// GLOBAL: DECKDLL 0x101a9708
static HANDLE global_delete_brush_handle_6 = NULL;

// GLOBAL: DECKDLL 0x10104d74
COLORREF global_colorref_darkgrey;
// GLOBAL: DECKDLL 0x10104c90
COLORREF global_colorref_flesh;
COLORREF global_colorref_lavender = 0;
// GLOBAL: DECKDLL 0x10104d70
COLORREF global_colorref_lightgrey;
// GLOBAL: DECKDLL 0x10104ca0
COLORREF global_colorref_white;

// GLOBAL: DECKDLL 0x10113f6c
HANDLE global_pic_ability;
// GLOBAL: DECKDLL 0x1014422c
HANDLE global_pic_antiquit;
// GLOBAL: DECKDLL 0x10143b14
HANDLE global_pic_arabnite;
// GLOBAL: DECKDLL 0x10125ba4
HANDLE global_pic_artifact;
// GLOBAL: DECKDLL 0x10146df4
HANDLE global_pic_astral;
// GLOBAL: DECKDLL 0x1012d7d4
HANDLE global_pic_grey;
// GLOBAL: DECKDLL 0x1012d7bc
HANDLE global_pic_bldr01c;
// GLOBAL: DECKDLL 0x1012d7c0
HANDLE global_pic_bldr02c;
// GLOBAL: DECKDLL 0x1012d7c8
HANDLE global_pic_bldr03c;
// GLOBAL: DECKDLL 0x1012d7cc
HANDLE global_pic_bldr04c;
// GLOBAL: DECKDLL 0x1012d7c4
HANDLE global_pic_bldr05c;
// GLOBAL: DECKDLL 0x1019f74c
HANDLE global_pic_blue = NULL;
// GLOBAL: DECKDLL 0x10144ef0
HANDLE global_pic_castcost;
// GLOBAL: DECKDLL 0x1012e2d4
HANDLE global_pic_creature;
// GLOBAL: DECKDLL 0x10141b84
HANDLE global_pic_dark;
// GLOBAL: DECKDLL 0x1012e89c
HANDLE global_pic_dekbar1;
// GLOBAL: DECKDLL 0x101bc43c
HANDLE global_pic_dektile4 = NULL;
// GLOBAL: DECKDLL 0x101bc484
HANDLE global_pic_dektit1 = NULL;
// GLOBAL: DECKDLL 0x10144230
HANDLE global_pic_enchant;
// GLOBAL: DECKDLL 0x100f2854
HANDLE global_groupmove_pic;
// GLOBAL: DECKDLL 0x1019e9d4
HANDLE global_pic_fourth = NULL;
// GLOBAL: DECKDLL 0x10132c34
HANDLE global_pic_yellow;
// GLOBAL: DECKDLL 0x10125668
HANDLE global_pic_green;
// GLOBAL: DECKDLL 0x100f284c
HANDLE global_deckinfo_pic;
// GLOBAL: DECKDLL 0x1012dc54
HANDLE global_pic_instant;
// GLOBAL: DECKDLL 0x1012df38
HANDLE global_pic_interrupt;
// GLOBAL: DECKDLL 0x101a970c
HANDLE global_pic_land = NULL;
// GLOBAL: DECKDLL 0x1014294c
HANDLE global_pic_legends;
// GLOBAL: DECKDLL 0x1019eedc
HANDLE global_pic_power = NULL;
// GLOBAL: DECKDLL 0x10113e38
HANDLE global_pic_rarity;
// GLOBAL: DECKDLL 0x10139cd0
HANDLE global_pic_red;
// GLOBAL: DECKDLL 0x101140b4
HANDLE global_pic_sorcery;
// GLOBAL: DECKDLL 0x1019f828
HANDLE global_pic_statbak1 = NULL;
// GLOBAL: DECKDLL 0x10144c3c
HANDLE global_pic_tough;
// GLOBAL: DECKDLL 0x10146b60
HANDLE global_pic_artist;
// GLOBAL: DECKDLL 0x1019e9cc
HANDLE global_pic_all_butn = NULL;

// GLOBAL: DECKDLL 0x101a9704
HBITMAP global_hbmp = NULL;

// GLOBAL: DECKDLL 0x101bc5fc
static HGDIOBJ global_old_bmp_obj = NULL;

// GLOBAL: DECKDLL 0x10125e0c
static HFONT global_card_amount_font;

// GLOBAL: DECKDLL 0x10113cec
int global_smallcard_normal_width;
// GLOBAL: DECKDLL 0x1012e4f4
int global_smallcard_normal_height;

// GLOBAL: DECKDLL 0x101bc448
int global_smallcard_smaller_width = 0;
// GLOBAL: DECKDLL 0x10113cf0
int global_smallcard_smaller_height;

// GLOBAL: DECKDLL 0x10143fc0
int global_smallcard_smallest_width;
// GLOBAL: DECKDLL 0x1014532c
int global_smallcard_smallest_height;

// GLOBAL: DECKDLL 0x10132c40
int global_smallcard_piclist_height;
// GLOBAL: DECKDLL 0x10143e88
int global_smallcard_piclist_width;

HFONT global_font_42 = NULL;
HFONT global_font_42unused = NULL;
HFONT global_font_40percent = NULL;
HFONT global_font_28percent = NULL;
HFONT global_font_32 = NULL;

// GLOBAL: DECKDLL 0x101bbc28
static HWND global_button_stats_hwnd;
// GLOBAL: DECKDLL 0x101436b4
static HWND global_cuecard_hwnd;

extern HWND global_decksurface_hwnd;
extern HPALETTE global_cart_art_hpalette;

/* In the SDL monolith, Shandalar and the embedded deck builder share the
 * card-art globals that were separate in the original EXE/DLL processes. */
static int global_deckbuilder_owns_card_art_palette;

// GLOBAL: DECKDLL 0x100f2848
static HANDLE global_gaun_options_pic;

// GLOBAL: DECKDLL 0x100f2850
static HANDLE global_gaun_results_pic;

// GLOBAL: DECKDLL 0x10104b4c
static HANDLE global_loaddeck_pic;

// GLOBAL: DECKDLL 0x10132c38
static HWND global_fullcard_hwnd;
// GLOBAL: DECKDLL 0x10104520
static HMENU global_fullcard_popup;

// GLOBAL: DECKDLL 0x101bc444
HWND global_horzlist_hwnd;
// GLOBAL: DECKDLL 0x101052c0
static POINT global_horzlist_last_mousemove_pt;
// GLOBAL: DECKDLL 0x101052c8
static HMENU global_horzlist_popup;

// GLOBAL: DECKDLL 0x1012e2d8
HWND cardlistfilter_hwnd;

// GLOBAL: DECKDLL 0x10144ef4
HWND global_main_hwnd;
static int global_main_window_width = 0;
static int global_main_window_height = 0;

// GLOBAL: DECKDLL 0x10144eec
int global_smallcard_height;
// GLOBAL: DECKDLL 0x1013a17c
int global_smallcard_width;

// GLOBAL: DECKDLL 0x10141b4c
static HMENU global_smallcard_popup;

// GLOBAL: DECKDLL 0x10146df0
HWND global_title_hwnd;

// GLOBAL: DECKDLL 0x10125a3c
HWND global_unknown_10125a3c;

// GLOBAL: DECKDLL 0x10144b04
HWND global_unknown_10144b04;

// GLOBAL: DECKDLL 0x1012566c
HDC global_hdc;

// GLOBAL: DECKDLL 0x10105738
static POINT global_horzlist_mouse_pt;

// GLOBAL: DECKDLL 0x10105748
static POINT global_main_mouse_pt;

static char global_search_string[264];

// GLOBAL: DECKDLL 0x101915a0
Packs global_packs[PACK1_MAX + 1][PACK2_MAX + 1];
// GLOBAL: DECKDLL 0x101e74f0
Packs global_packs_copy[PACK1_MAX + 1][PACK2_MAX + 1];

// GLOBAL: DECKDLL 0x10132c50
char text_lines[225][128];

extern int global_available_slots;

extern card_ptr_t global_raw_cards_storage[2000];

// GLOBAL: DECKDLL 0x10145330
OrigRarities global_origrarities[1000];
// GLOBAL: DECKDLL 0x101bc440
int *Gold;
// GLOBAL: DECKDLL 0x10146aa0
shandalar_worldmagic_t *Scards;
// Why are these exported from here?  And with illegal names, no less?
int _PlayerFace;
int _OpponFace;

// GLOBAL: DECKDLL 0x1019f824
static Int_fn_int CardIDFromType;
// GLOBAL: DECKDLL 0x10142818
static Int_fn_int CardTypeFromID;
// GLOBAL: DECKDLL 0x101a8934
static Int_fn_int CardInDeck;
// GLOBAL: DECKDLL 0x10143350
static Void_fn_int_int SetCardInDeck;
// GLOBAL: DECKDLL 0x10125eb4
static Int_fn_int SellPrice;
static void (*g_host_begin_deckbuilder)(void);
static void (*g_host_end_deckbuilder)(void);

bool check_filters(csvid_t csvid);
int InitLicenseSecretsFromRegistry(void);

/* Defined in filtermenus.c (original binary uses these in wndproc_MainClass). */
extern int check_filters_debug_log_enabled;
extern FILE *check_filters_debug_log_file;

LRESULT CALLBACK wndproc_CardClass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK wndproc_CardListFilterClass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK wndproc_CueCardClass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK wndproc_DeckSurfaceClass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK wndproc_FullCardClass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK wndproc_HorzListClass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK wndproc_MainClass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK wndproc_TitleClass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK wndproc_SideboardSurfaceClass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK wndproc_TradeSurfaceClass(HWND, UINT, WPARAM, LPARAM);

INT_PTR CALLBACK dlgproc_DeckStats(HWND, UINT, WPARAM, LPARAM);
static int deckbuilder_selling_available(void);
static int deckbuilder_drag_sell_price(HWND source, POINT source_point);

// FUNCTION: DECKDLL 0x1000d9a3
static int fatal_err(const char *text, HWND hwnd)
{
  printf("deck: fatal: %s\n", text ? text : "");
  MessageBox(hwnd, text, 0, MB_SYSTEMMODAL | MB_ICONERROR);

  if (hwnd)
    SendMessage(hwnd, WM_CLOSE, 0, 0);
  else
    PostQuitMessage(1);
  return 0;
}

void fatal(const char *fmt, ...)
{
  char buf[8000];
  va_list args;
  va_start(args, fmt);
  _vsnprintf(buf, 8000, fmt, args);
  va_end(args);

  fatal_err(buf, 0);
  exit(1);
}

#ifdef SHANDALAR_SDL_WIN32_COMPAT
void deck_set_host_pump_hooks(void (*begin)(void), void (*end)(void))
{
  g_host_begin_deckbuilder = begin;
  g_host_end_deckbuilder = end;
}
#endif

// FUNCTION: DECKDLL 0x10028350
int load_text(const char *file_name, const char *section_name)
{
  FILE *f;
  int i;
  char line[80];
  char path[128];
  char section_line[80]; // ebp - 0x54
  unsigned int num_text;

  strcpy(section_line, "@");
  strcat(section_line, section_name);
  strcat(section_line, "\n");

  strcpy(path, global_base_directory);
  strcat(path, "\\");
  strcat(path, file_name);
  if (!strchr(file_name, '.'))
    strcat(path, ".txt");

  f = fopen(path, "rt");
  if (f == NULL)
    return -1;

  while (strcmp(section_line, line))
  {
    if (!fgets(line, 80, f))
    {
      fclose(f);
      return -1;
    }
  }

  fscanf(f, "%d", &num_text);
  fgets(line, 80, f);
  if (num_text > 225)
  {
    fclose(f);
    return -1;
  }

  for (i = 0; i < (int)num_text; i++)
  {
    if (!fgets(text_lines[i], 80, f))
    {
      fclose(f);
      return -1;
    }
    text_lines[i][strlen(text_lines[i]) - 1] = 0;
  }

  fclose(f);
  return num_text;
}

// GLOBAL: DECKDLL 0x1003181c
static const char s_PRICE_1003181c[8] = "PRICE";
// GLOBAL: DECKDLL 0x10031824
static const char s_menus_10031824[8] = "menus";
// GLOBAL: DECKDLL 0x1003182c
static const char format_decimal_int[4] = "%d";
// GLOBAL: DECKDLL 0x10031830
static const char s_SELLCARD_10031830[12] = "SELLCARD";
// GLOBAL: DECKDLL 0x1003183c
static const char s_menus_1003183c[8] = "menus";

// GLOBAL: DECKDLL 0x1019c020
char read_db_artist_names[100][100];

// FUNCTION: DECKDLL 0x1000d9f1
static void
read_manalink_ini(void)
{
  HKEY key;      /* ebp - 0x6c */
  BYTE buf[100]; /* ebp - 0x68 */
  DWORD buflen;  /* ebp - 0x4 */

  if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\MicroProse\\Magic: The Gathering\\DeckBuilderOptions", 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
  {
    buflen = 10;
    if (RegQueryValueEx(key, "Consolidate", NULL, NULL, buf, &buflen) == ERROR_SUCCESS)
      sscanf((char *)buf, "%d", (int *)&global_cfg.consolidate_from_registry);
    else
      global_cfg.consolidate_from_registry = 1;

    if (RegQueryValueEx(key, "Music", NULL, NULL, buf, &buflen) == ERROR_SUCCESS)
      sscanf((char *)buf, "%d", (int *)((char *)&global_cfg.consolidate_from_registry + 1));
    else
      global_cfg.music = 1;

    if (RegQueryValueEx(key, "Effects", NULL, NULL, buf, &buflen) == ERROR_SUCCESS)
      sscanf((char *)buf, "%d", (int *)((char *)&global_cfg.consolidate_from_registry + 2));
    else
      global_cfg.effects = 1;

    RegCloseKey(key);
  }
  else
  {
    global_cfg.consolidate_from_registry = 1;
    global_cfg.music = 1;
    global_cfg.effects = 1;
  }

  if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\MicroProse\\Magic: The Gathering", 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
  {
    buflen = 100;
    if (RegQueryValueEx(key, "Name", NULL, NULL, buf, &buflen) == ERROR_SUCCESS)
      strcpy(global_cfg.player_name, (char *)buf);
    else
      strcpy(global_cfg.player_name, "User");

    buflen = 100;
    if (RegQueryValueEx(key, "Email", NULL, NULL, buf, &buflen) == ERROR_SUCCESS)
      strcpy(global_cfg.email, (char *)buf);
    else
      strcpy(global_cfg.email, "User E-Mail");

    RegCloseKey(key);
  }
  else
    strcpy(global_cfg.email, "User E-Mail");

  if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\MicroProse\\Magic: The Gathering\\DuelOptions", 0, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
  {
    buflen = 100;
    if (RegQueryValueEx(key, "ExpandTextOnBigCard", NULL, NULL, buf, &buflen) == ERROR_SUCCESS)
      sscanf((char *)buf, "%d", &global_cfg.expand_text);
    else
      global_cfg.expand_text = 0;
    RegCloseKey(key);
  }
  else
    global_cfg.expand_text = 0;
}

static void
cfg_write_str(const char *str, const char *keyname)
{
  WritePrivateProfileString("DeckBuilder", keyname, str, global_manalink_ini_path);
}

static void
cfg_write_int(int val, const char *keyname)
{
  char buf[256];
  sprintf(buf, "%d", val);
  cfg_write_str(buf, keyname);
}

// FUNCTION: DECKDLL 0x1000dca4
static void
write_manalink_ini(void)
{
  HKEY key;
  BYTE buf[12];
  DWORD disposition;

  if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\MicroProse\\Magic: The Gathering\\DeckBuilderOptions",
                     0, NULL, 0, 0xF003F, NULL, &key, &disposition) == ERROR_SUCCESS)
  {
    wsprintf((char *)buf, "%d", (int)(signed char)global_cfg.consolidate_from_registry);
    RegSetValueEx(key, "Consolidate", 0, 1, buf, strlen((char *)buf) + 1);

    wsprintf((char *)buf, "%d", (int)(signed char)global_cfg.music);
    RegSetValueEx(key, "Music", 0, 1, buf, strlen((char *)buf) + 1);

    wsprintf((char *)buf, "%d", (int)(signed char)global_cfg.effects);
    RegSetValueEx(key, "Effects", 0, 1, buf, strlen((char *)buf) + 1);

    RegFlushKey(key);
    RegCloseKey(key);
  }

  if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\MicroProse\\Magic: The Gathering\\DuelOptions",
                     0, NULL, 0, 0xF003F, NULL, &key, &disposition) == ERROR_SUCCESS)
  {
    wsprintf((char *)buf, "%d", (int)(signed char)global_cfg.expand_text);
    RegSetValueEx(key, "ExpandTextOnBigCard", 0, 1, buf, strlen((char *)buf) + 1);

    RegFlushKey(key);
    RegCloseKey(key);
  }
}

void delete_and_close_object(HANDLE obj);

// FUNCTION: DECKDLL 0x1000d741
static void
delete_brushes(void)
{
  delete_and_close_object(global_delete_brush_handle_0);
  delete_and_close_object(global_delete_brush_handle_1);
  delete_and_close_object(global_delete_brush_handle_2);
  delete_and_close_object(global_delete_brush_handle_3);
  delete_and_close_object(global_delete_brush_handle_4);
  delete_and_close_object(global_delete_brush_handle_5);
  delete_and_close_object(global_delete_brush_handle_6);
}

// FUNCTION: DECKDLL 0x1000d7ae
static bool
create_brushes(void)
{
  global_create_brush_0 = CreateSolidBrush(0x24a3230);
  global_create_brush_1 = CreateSolidBrush(0x2508080);
  global_create_brush_2 = CreateSolidBrush(0x2508080);
  global_create_brush_3 = CreateSolidBrush(0x2508080);
  global_create_brush_4 = CreateSolidBrush(0x2508080);
  global_create_brush_5 = CreateSolidBrush(0x2508080);
  global_create_brush_6 = CreateSolidBrush(0x2508080);

  if (!global_create_brush_0 || !global_create_brush_1 || !global_create_brush_2 ||
      !global_create_brush_3 || !global_create_brush_4 || !global_create_brush_5 ||
      !global_create_brush_6)
  {
    if (global_create_brush_0)
      DeleteObject(global_create_brush_0);
    if (global_create_brush_1)
      DeleteObject(global_create_brush_1);
    if (global_create_brush_2)
      DeleteObject(global_create_brush_2);
    if (global_create_brush_3)
      DeleteObject(global_create_brush_3);
    if (global_create_brush_4)
      DeleteObject(global_create_brush_4);
    if (global_create_brush_5)
      DeleteObject(global_create_brush_5);
    if (global_create_brush_6)
      DeleteObject(global_create_brush_6);
    return false;
  }

  return true;
}

// FUNCTION: DECKDLL 0x1000d944
static void
delete_fonts(void)
{
  DeleteObject(global_create_brush_0);
  DeleteObject(global_create_brush_1);
  DeleteObject(global_create_brush_2);
  DeleteObject(global_create_brush_3);
  DeleteObject(global_create_brush_4);
  DeleteObject(global_create_brush_5);
  DeleteObject(global_create_brush_6);
}

#define DELETE_PIC_NOZERO(pic)      \
  if ((pic) != (HANDLE)0)           \
  {                                 \
    delete_and_close_object((pic)); \
  }
#define DELETE_PIC(pic)             \
  if ((pic) != (HANDLE)0)           \
  {                                 \
    delete_and_close_object((pic)); \
    (pic) = (HANDLE)0;              \
  }

// FUNCTION: DECKDLL 0x1000d0ae
static void
delete_pics(void)
{
  DELETE_PIC(global_pic_artist);
  DELETE_PIC(global_pic_rarity);
  DELETE_PIC(global_pic_ability);
  DELETE_PIC(global_pic_all_butn);
  DELETE_PIC(global_pic_astral);
  DELETE_PIC(global_pic_fourth);
  DELETE_PIC(global_pic_green);
  DELETE_PIC(global_pic_yellow);
  DELETE_PIC(global_pic_blue);
  DELETE_PIC(global_pic_grey);
  DELETE_PIC(global_pic_red);
  DELETE_PIC(global_pic_artifact);
  DELETE_PIC(global_pic_castcost);
  DELETE_PIC(global_pic_creature);
  DELETE_PIC(global_pic_enchant);
  DELETE_PIC(global_pic_instant);
  DELETE_PIC(global_pic_interrupt);
  DELETE_PIC(global_pic_land);
  DELETE_PIC(global_pic_power);
  DELETE_PIC(global_pic_sorcery);
  DELETE_PIC(global_pic_tough);
  DELETE_PIC(global_pic_arabnite);
  DELETE_PIC(global_pic_antiquit);
  DELETE_PIC(global_pic_legends);
  DELETE_PIC(global_pic_dark);
  DELETE_PIC_NOZERO(global_pic_dektile4);
  DELETE_PIC_NOZERO(global_pic_dekbar1);
  DELETE_PIC_NOZERO(global_pic_dektit1);
  DELETE_PIC_NOZERO(global_pic_statbak1);
  DELETE_PIC_NOZERO(global_pic_bldr01c);
  DELETE_PIC_NOZERO(global_pic_bldr02c);
  DELETE_PIC_NOZERO(global_pic_bldr03c);
  DELETE_PIC_NOZERO(global_pic_bldr04c);
  DELETE_PIC_NOZERO(global_pic_bldr05c);
}

// FUNCTION: DECKDLL 0x1000c51f
static bool
load_pics(void)
{
  char path[MAX_PATH + 4];

  global_fonts_init_state = 3;

  sprintf(path, "%s\\Artifact.pic", global_dbart_path);
  global_pic_artifact = load_pic(path);
  sprintf(path, "%s\\CastCost.pic", global_dbart_path);
  global_pic_castcost = load_pic(path);
  sprintf(path, "%s\\Creature.pic", global_dbart_path);
  global_pic_creature = load_pic(path);
  sprintf(path, "%s\\Enchant.pic", global_dbart_path);
  global_pic_enchant = load_pic(path);
  sprintf(path, "%s\\Instant.pic", global_dbart_path);
  global_pic_instant = load_pic(path);
  sprintf(path, "%s\\Interrupt.pic", global_dbart_path);
  global_pic_interrupt = load_pic(path);
  sprintf(path, "%s\\Land.pic", global_dbart_path);
  global_pic_land = load_pic(path);
  sprintf(path, "%s\\Power.pic", global_dbart_path);
  global_pic_power = load_pic(path);
  sprintf(path, "%s\\Sorcery.pic", global_dbart_path);
  global_pic_sorcery = load_pic(path);
  sprintf(path, "%s\\Tough.pic", global_dbart_path);
  global_pic_tough = load_pic(path);
  sprintf(path, "%s\\Artist.pic", global_dbart_path);
  global_pic_artist = load_pic(path);
  sprintf(path, "%s\\Rarity.pic", global_dbart_path);
  global_pic_rarity = load_pic(path);
  sprintf(path, "%s\\Ability.pic", global_dbart_path);
  global_pic_ability = load_pic(path);
  sprintf(path, "%s\\All-Butn.pic", global_dbart_path);
  global_pic_all_butn = load_pic(path);
  sprintf(path, "%s\\Astral.pic", global_dbart_path);
  global_pic_astral = load_pic(path);
  sprintf(path, "%s\\Arabnite.pic", global_dbart_path);
  global_pic_arabnite = load_pic(path);
  sprintf(path, "%s\\Antiquit.pic", global_dbart_path);
  global_pic_antiquit = load_pic(path);
  sprintf(path, "%s\\Legends.pic", global_dbart_path);
  global_pic_legends = load_pic(path);
  sprintf(path, "%s\\Dark.pic", global_dbart_path);
  global_pic_dark = load_pic(path);
  sprintf(path, "%s\\Green.pic", global_dbart_path);
  global_pic_green = load_pic(path);
  sprintf(path, "%s\\Yellow.pic", global_dbart_path);
  global_pic_yellow = load_pic(path);
  sprintf(path, "%s\\Blue.pic", global_dbart_path);
  global_pic_blue = load_pic(path);
  sprintf(path, "%s\\Grey.pic", global_dbart_path);
  global_pic_grey = load_pic(path);
  sprintf(path, "%s\\Red.pic", global_dbart_path);
  global_pic_red = load_pic(path);
  sprintf(path, "%s\\dektile4.pic", global_dbart_path);
  global_pic_dektile4 = load_pic(path);
  sprintf(path, "%s\\dekbar1.pic", global_dbart_path);
  global_pic_dekbar1 = load_pic(path);
  sprintf(path, "%s\\dektit1.pic", global_dbart_path);
  global_pic_dektit1 = load_pic(path);
  sprintf(path, "%s\\statbak1.pic", global_dbart_path);
  global_pic_statbak1 = load_pic(path);
  sprintf(path, "%s\\bldr01c.pic", global_dbart_path);
  global_pic_bldr01c = load_pic(path);
  sprintf(path, "%s\\bldr02c.pic", global_dbart_path);
  global_pic_bldr02c = load_pic(path);
  sprintf(path, "%s\\bldr03c.pic", global_dbart_path);
  global_pic_bldr03c = load_pic(path);
  sprintf(path, "%s\\bldr04c.pic", global_dbart_path);
  global_pic_bldr04c = load_pic(path);
  sprintf(path, "%s\\bldr05c.pic", global_dbart_path);
  global_pic_bldr05c = load_pic(path);

  if (!global_pic_dektile4 || !global_pic_dekbar1 || !global_pic_dektit1 || !global_pic_statbak1 ||
      !global_pic_bldr01c || !global_pic_bldr02c || !global_pic_bldr03c || !global_pic_bldr04c ||
      !global_pic_bldr05c || !global_pic_artist || !global_pic_rarity || !global_pic_ability ||
      !global_pic_artifact || !global_pic_castcost || !global_pic_creature || !global_pic_enchant ||
      !global_pic_instant || !global_pic_interrupt || !global_pic_land || !global_pic_power ||
      !global_pic_sorcery || !global_pic_tough || !global_pic_all_butn || !global_pic_astral ||
      !global_pic_green || !global_pic_yellow || !global_pic_blue || !global_pic_grey ||
      !global_pic_red)
  {

    DELETE_PIC_NOZERO(global_pic_dektile4);
    DELETE_PIC_NOZERO(global_pic_dekbar1);
    DELETE_PIC_NOZERO(global_pic_dektit1);
    DELETE_PIC_NOZERO(global_pic_statbak1);
    DELETE_PIC_NOZERO(global_pic_bldr01c);
    DELETE_PIC_NOZERO(global_pic_bldr02c);
    DELETE_PIC_NOZERO(global_pic_bldr03c);
    DELETE_PIC_NOZERO(global_pic_bldr04c);
    DELETE_PIC_NOZERO(global_pic_bldr05c);

    DELETE_PIC(global_pic_artist);
    DELETE_PIC(global_pic_rarity);
    DELETE_PIC(global_pic_ability);
    DELETE_PIC(global_pic_all_butn);
    DELETE_PIC(global_pic_astral);
    DELETE_PIC(global_pic_fourth);
    DELETE_PIC(global_pic_green);
    DELETE_PIC(global_pic_yellow);
    DELETE_PIC(global_pic_blue);
    DELETE_PIC(global_pic_grey);
    DELETE_PIC(global_pic_red);
    DELETE_PIC(global_pic_artifact);
    DELETE_PIC(global_pic_castcost);
    DELETE_PIC(global_pic_creature);
    DELETE_PIC(global_pic_enchant);
    DELETE_PIC(global_pic_instant);
    DELETE_PIC(global_pic_interrupt);
    DELETE_PIC(global_pic_land);
    DELETE_PIC(global_pic_power);
    DELETE_PIC(global_pic_sorcery);
    DELETE_PIC(global_pic_tough);

    // Conspicuously missing
    /*
    DELETE_PIC(global_pic_arabnite);
    DELETE_PIC(global_pic_antiquit);
    DELETE_PIC(global_pic_legends);
    DELETE_PIC(global_pic_dark);
    */

    return false;
  }

  return true;
}

#undef DELETE_PIC_NOZERO
#undef DELETE_PIC

// FUNCTION: DECKDLL 0x1000bbec
static int
delete_resources(void)
{
  unsigned int v1;
  int i;

  write_manalink_ini();

  if (global_db_flags_1 & (DBFLAGS_EDITDECK | DBFLAGS_SHANDALAR))
  {
    for (i = 0; i < global_deck_num_entries; ++i)
    {
      v1 = 0;
      if (global_deck[i].GDE_Available)
        v1 |= 0x4000;
      v1 |= global_deck[i].GDE_DecksBits << 16;
      deck[i] = global_deck[i].GDE_iid | v1;
    }

    for (i = global_deck_num_entries; i < 500; ++i)
      deck[i] = -1;

    *global_external_current_deck = global_current_deck;
  }

  delete_pics();

  delete_brushes();
  if (global_deckbuilder_owns_card_art_palette)
  {
    DestroyCardArtPalette();
    global_deckbuilder_owns_card_art_palette = 0;
  }
  delete_fonts();
  DeleteDC(global_hdc);
  DeleteObject(global_hbmp);
  chdir(global_previous_directory);
  return 0;
}

// FUNCTION: DECKDLL 0x1000c337
bool process_cue_cards(MSG *msg)
{
  POINT screen_point; /* ebp - 0x18 */
  LRESULT msgnum;     /* ebp - 0x10 */
  HWND hwnd;          /* ebp - 0xc */
  POINT client_point; /* ebp - 0x8 */

  // GLOBAL: DECKDLL 0x100327d0
  static int last_cursor_x = 0;
  // GLOBAL: DECKDLL 0x100327d4
  static int last_cursor_y = 0;
  // GLOBAL: DECKDLL 0x100f30a0
  static HWND last_hwnd;

  if (msg->message == WM_TIMER && msg->hwnd == global_main_hwnd && msg->wParam == 0)
  {
    GetCursorPos(&screen_point);
    KillTimer(global_main_hwnd, 0);

    hwnd = WindowFromPoint(screen_point);
    client_point.x = screen_point.x;
    client_point.y = screen_point.y;
    ScreenToClient(hwnd, &client_point);
    msgnum = SendMessage(hwnd,
                         0x465,
                         0,
                         MAKELONG2(client_point.x, client_point.y));
    if (msgnum != 0)
      SendMessage(global_cuecard_hwnd,
                  0x400,
                  MAKELONG3(screen_point.x + 5, screen_point.y + 0xf),
                  (LPARAM)text_lines[msgnum - 1]);
    return true;
  }

  if (msg->message == WM_MOUSEMOVE)
  {
    GetCursorPos(&screen_point);
    if (abs(last_cursor_y - screen_point.y) + abs(last_cursor_x - screen_point.x) < 2)
      return false;

    last_cursor_x = screen_point.x;
    last_cursor_y = screen_point.y;
    SetTimer(global_main_hwnd, 0, 500, NULL);
    if (last_hwnd != msg->hwnd)
    {
      last_hwnd = msg->hwnd;
      ShowWindow(global_cuecard_hwnd, SW_HIDE);
    }
    return false;
  }

  if ((unsigned int)msg->message >= WM_MOUSEMOVE && (unsigned int)msg->message <= WM_MBUTTONDBLCLK)
  {
    KillTimer(global_main_hwnd, 0);
    ShowWindow(global_cuecard_hwnd, SW_HIDE);
    return false;
  }

  return false;
}

/* stdcall version exported without stdcall suffix; used by assembly and external callers. */
// FUNCTION: DECKDLL 0x1000bcfd
WPARAM WINAPI
DeckBuilderMain(HWND parent_hwnd, int db_flags_1, int db_flags_2)
{
  int screen_width;
  int screen_height;
  MSG msg;
  int unused_zero;
  HDC desktop_hdc;
  HACCEL accel;
  int host_pump_active;

  unused_zero = 0;
  host_pump_active = 0;

  global_db_flags_1 = db_flags_1;
  global_db_flags_2 = db_flags_2;

  InitializeCriticalSection(&global_critical_section_for_unknown);

  _getcwd(global_previous_directory, 0x105);
  _chdir(global_base_directory);

  CardIDFromType = (Int_fn_int)GetProcAddress(GetModuleHandleA(NULL), "CardIDFromType");
  CardTypeFromID = (Int_fn_int)GetProcAddress(GetModuleHandleA(NULL), "CardTypeFromID");
  CardInDeck = (Int_fn_int)GetProcAddress(GetModuleHandleA(NULL), "CardInDeck");
  SetCardInDeck = (Void_fn_int_int)GetProcAddress(GetModuleHandleA(NULL), "SetCardInDeck");
  SellPrice = (Int_fn_int)GetProcAddress(GetModuleHandleA(NULL), "SellPrice");
  deck = (int *)GetProcAddress(GetModuleHandleA(NULL), "deck");
  Gold = (int *)GetProcAddress(GetModuleHandleA(NULL), "Gold");
  Scards = (shandalar_worldmagic_t *)GetProcAddress(GetModuleHandleA(NULL), "Scards");
  global_external_deckname = (char *)GetProcAddress(GetModuleHandleA(NULL), "szDeckName");
  global_external_current_deck = (int *)GetProcAddress(GetModuleHandleA(NULL), "_currentDeck");

  if (global_db_flags_1 & (DBFLAGS_EDITDECK | DBFLAGS_SHANDALAR))
  {
    if (!CardIDFromType || !CardInDeck || !SetCardInDeck || !SellPrice || !deck || !Gold || !Scards || !global_external_current_deck)
    {
      char missing[512] = "Deck Builder WinMain: Couldn't find External Functions:";
      if (!CardIDFromType) strcat(missing, " CardIDFromType");
      if (!CardInDeck) strcat(missing, " CardInDeck");
      if (!SetCardInDeck) strcat(missing, " SetCardInDeck");
      if (!SellPrice) strcat(missing, " SellPrice");
      if (!deck) strcat(missing, " deck");
      if (!Gold) strcat(missing, " Gold");
      if (!Scards) strcat(missing, " Scards");
      if (!global_external_current_deck) strcat(missing, " _currentDeck");
      fatal_err(missing, 0);
      return 0;
    }
  }
  else
  {
    if (global_db_flags_1 & DBFLAGS_NOCARDCOUNTCHECK || global_db_flags_1 & DBFLAGS_GAUNTLET)
    {
      if (!CardIDFromType || !CardTypeFromID || !CardInDeck || !SetCardInDeck || !Gold || !global_external_deckname)
      {
        char missing[512] = "Deck Builder WinMain: Couldn't find External Functions:";
        if (!CardIDFromType) strcat(missing, " CardIDFromType");
        if (!CardTypeFromID) strcat(missing, " CardTypeFromID");
        if (!CardInDeck) strcat(missing, " CardInDeck");
        if (!SetCardInDeck) strcat(missing, " SetCardInDeck");
        if (!Gold) strcat(missing, " Gold");
        if (!global_external_deckname) strcat(missing, " szDeckName");
        fatal_err(missing, 0);
        return 0;
      }
    }
  }

  global_deckbuilder_owns_card_art_palette = 0;
#ifdef SHANDALAR_SDL_WIN32_COMPAT
  if (!((global_db_flags_1 & DBFLAGS_SHANDALAR) && global_cart_art_hpalette))
#endif
  {
    if (!SetupDuelPalette())
    {
      fatal_err("WinMain: Couldn't create the palette", 0);
      return 0;
    }
    global_deckbuilder_owns_card_art_palette = 1;
  }

  if (global_db_flags_1 & (DBFLAGS_EDITDECK | DBFLAGS_SHANDALAR))
    global_current_deck = *global_external_current_deck;

  desktop_hdc = GetDC(GetDesktopWindow());

  global_hdc = CreateCompatibleDC(desktop_hdc);
  global_hbmp = CreateCompatibleBitmap(desktop_hdc, 800, 800);
  if (!global_hdc || !global_hbmp)
    fatal_err("WinMain: Couldn't create the app-wide memory DC or bitmap", 0);

  ApplyCardArtPaletteToDc(global_hdc);

  global_old_bmp_obj = SelectObject(global_hdc, global_hbmp);

  ReleaseDC(GetDesktopWindow(), desktop_hdc);

  if (!InitCardArtGdiResources())
  {
    fatal_err("WinMain: Couldn't load deck builder bitmaps", 0);
    return 0;
  }

  if (!load_pics())
  {
    fatal_err("WinMain: Couldn't load deck builder bitmaps", 0);
    return 0;
  }

  if (!create_fonts())
  {
    fatal_err("WinMain: Couldn't load the card background bitmaps", 0);
    return 0;
  }

  if (!create_brushes())
  {
    fatal_err("WinMain: Couldn't create the window background bitmaps", 0);
    return 0;
  }

  ReadDeckbuilderCsv("DeckBuilder.csv", global_origrarities);

#ifdef SHANDALAR_SDL_WIN32_COMPAT
  if ((global_db_flags_1 & DBFLAGS_SHANDALAR) && g_host_begin_deckbuilder) {
    g_host_begin_deckbuilder();
    host_pump_active = 1;
  }
#endif

  screen_width = GetSystemMetrics(0);
  screen_height = GetSystemMetrics(1);

  global_main_hwnd = CreateWindowEx(0, "MAGICDECK_MainClass", "Deck Maker",
                                    0x90040000,
                                    0, 0, screen_width, screen_height,
                                    parent_hwnd,
                                    0,
                                    global_hinstance, 0);
  if (!global_main_hwnd)
  {
    fatal_err("WinMain: Couldn't create the main window", 0);
#ifdef SHANDALAR_SDL_WIN32_COMPAT
    if (host_pump_active && g_host_end_deckbuilder)
      g_host_end_deckbuilder();
#endif
    return 0;
  }

  global_cuecard_hwnd = CreateWindowEx(0, "CueCardClass", "",
                                       WS_POPUP,
                                       0, 0, 0, 0,
                                       global_main_hwnd,
                                       0,
                                       global_hinstance, 0);
  if (!global_cuecard_hwnd)
  {
    fatal_err("WinMain: Couldn't create the cue card window", global_main_hwnd);
#ifdef SHANDALAR_SDL_WIN32_COMPAT
    if (host_pump_active && g_host_end_deckbuilder)
      g_host_end_deckbuilder();
#endif
    return 0;
  }

  accel = LoadAccelerators(global_hinstance, MAKEINTRESOURCE(RES_ACCEL));
  if (!accel)
  {
    fatal_err("WinMain: Couldn't load the accelerator table", global_main_hwnd);
#ifdef SHANDALAR_SDL_WIN32_COMPAT
    if (host_pump_active && g_host_end_deckbuilder)
      g_host_end_deckbuilder();
#endif
    return 0;
  }

  ShowWindow(global_main_hwnd, SW_SHOWNORMAL);
  SetTimer(global_main_hwnd, 0, 500, NULL);
  SetForegroundWindow(global_main_hwnd);

  while (GetMessage(&msg, NULL, 0, 0))
    if (!TranslateAccelerator(global_main_hwnd, accel, &msg) && !process_cue_cards(&msg))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

  delete_resources();
  if (!(global_db_flags_1 & DBFLAGS_SHANDALAR))
    ShutdownCardArtGdiResources();
  destroy_create_fonts_resources();
  DeleteCriticalSection(&global_critical_section_for_unknown);
#ifdef SHANDALAR_SDL_WIN32_COMPAT
  if (host_pump_active && g_host_end_deckbuilder)
    g_host_end_deckbuilder();
#endif

  return msg.wParam;
}

WPARAM
deckbuilder_main(HWND parent_hwnd, int db_flags_1, int db_flags_2)
{
  return DeckBuilderMain(parent_hwnd, db_flags_1, db_flags_2);
}

// TODO: call to RegisterClass seems to think it returns an int in orig?
#define REGISTER_AND_RETURN_CLASS(classname, wndproc, style, extra_size, icon, cursor)                    \
  WNDCLASS hwnd = {style, wndproc, 0, extra_size, global_hinstance, icon, cursor, NULL, NULL, classname}; \
  if (!(RegisterClass(&hwnd) & 0x0000ffff))                                                               \
    return 0;                                                                                             \
  return 1;

// FUNCTION: DECKDLL 0x10001140
bool register_DeckSurfaceClass(void)
{
  WNDCLASS hwnd;

  hwnd.style = 0;
  hwnd.lpfnWndProc = wndproc_DeckSurfaceClass;
  hwnd.cbClsExtra = 0;
  hwnd.cbWndExtra = 0;
  hwnd.hInstance = global_hinstance;
  hwnd.hIcon = LoadIcon(0, IDI_APPLICATION);
  hwnd.hCursor = LoadCursor(0, IDC_ARROW);
  hwnd.hbrBackground = (HBRUSH)0x0;
  hwnd.lpszMenuName = (LPCSTR)0x0;
  hwnd.lpszClassName = "MAGICDECK_DeckSurfaceClass";

  if (!(RegisterClassA(&hwnd) & 0x0000ffff))
    return 0;

  hwnd.style = 0;
  hwnd.lpfnWndProc = wndproc_SideboardSurfaceClass;
  hwnd.cbClsExtra = 0;
  hwnd.cbWndExtra = 0;
  hwnd.hInstance = global_hinstance;
  hwnd.hIcon = LoadIcon(0, IDI_APPLICATION);
  hwnd.hCursor = LoadCursor(0, IDC_ARROW);
  hwnd.hbrBackground = (HBRUSH)0x0;
  hwnd.lpszMenuName = (LPCSTR)0x0;
  hwnd.lpszClassName = "MAGICDECK_SideboardSurfaceClass";

  if (!(RegisterClassA(&hwnd) & 0x0000ffff))
    return 0;

  hwnd.style = 0;
  hwnd.lpfnWndProc = wndproc_TradeSurfaceClass;
  hwnd.cbClsExtra = 0;
  hwnd.cbWndExtra = 0;
  hwnd.hInstance = global_hinstance;
  hwnd.hIcon = LoadIconA((HINSTANCE)0x0, (LPCSTR)0x7f00);
  hwnd.hCursor = LoadCursorA((HINSTANCE)0x0, (LPCSTR)0x7f00);
  hwnd.hbrBackground = (HBRUSH)0x0;
  hwnd.lpszMenuName = (LPCSTR)0x0;
  hwnd.lpszClassName = "MAGICDECK_TradeSurfaceClass";

  if (!(RegisterClass(&hwnd) & 0x0000ffff))
    return 0;

  return 1;
}

// FUNCTION: DECKDLL 0x10008080
bool register_CardClass(void)
{
  // GLOBAL: DECKDLL 0x10030008
  static int cardclass_cb_wnd_extra = 6;
  REGISTER_AND_RETURN_CLASS("MAGICDECK_CardClass",
                            wndproc_CardClass,
                            CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
                            cardclass_cb_wnd_extra,
                            LoadIcon(0, IDI_APPLICATION),
                            LoadCursor(0, IDC_ARROW));
}

// FUNCTION: DECKDLL 0x10010EF0
bool register_FullCardClass(void)
{
  // GLOBAL: DECKDLL 0x1003001c
  static int fullcardclass_cb_wnd_extra = 0xe;
  REGISTER_AND_RETURN_CLASS("MagicFullCardClass",
                            wndproc_FullCardClass,
                            CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
                            fullcardclass_cb_wnd_extra,
                            0,
                            LoadCursor(0, IDC_ARROW));
}

// FUNCTION: DECKDLL 0x10016DC0
bool register_MainClass(void)
{
  REGISTER_AND_RETURN_CLASS("MAGICDECK_MainClass",
                            wndproc_MainClass,
                            CS_OWNDC,
                            0,
                            LoadIcon(global_hinstance, MAKEINTRESOURCE(RES_ICON)),
                            LoadCursor(0, IDC_ARROW));
}

// FUNCTION: DECKDLL 0x1001BC50
bool register_CardListFilterClass(void)
{
  // GLOBAL: DECKDLL 0x10030024
  static int cardlistfilterclass_cb_wnd_extra = 4;
  REGISTER_AND_RETURN_CLASS("MAGICDECK_CardListFiltersClass",
                            wndproc_CardListFilterClass,
                            CS_HREDRAW | CS_VREDRAW,
                            cardlistfilterclass_cb_wnd_extra,
                            0,
                            LoadCursor(0, IDC_ARROW));
}

// FUNCTION: DECKDLL 0x10021360
bool register_CueCardClass(void)
{
  // GLOBAL: DECKDLL 0x1003002c
  static int cuecardclass_cb_wnd_extra = 4;
  REGISTER_AND_RETURN_CLASS("CueCardClass",
                            wndproc_CueCardClass,
                            CS_SAVEBITS,
                            cuecardclass_cb_wnd_extra,
                            LoadIcon(0, IDI_APPLICATION),
                            LoadCursor(0, IDC_ARROW));
}

// FUNCTION: DECKDLL 0x100275C0
bool register_TitleClass(void)
{
  REGISTER_AND_RETURN_CLASS("MAGICDECK_TitleClass",
                            wndproc_TitleClass,
                            0,
                            0,
                            LoadIcon(0, IDI_APPLICATION),
                            LoadCursor(0, IDC_ARROW));
}

// FUNCTION: DECKDLL 0x100287F9
bool register_HorzListClass(void)
{
  // GLOBAL: DECKDLL 0x10030048
  static int horzlistclass_cb_wnd_extra = 0xe;
  REGISTER_AND_RETURN_CLASS("MAGICDECK_HorzListClass",
                            wndproc_HorzListClass,
                            CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
                            horzlistclass_cb_wnd_extra,
                            0,
                            LoadCursor(0, IDC_ARROW));
}

// FUNCTION: DECKDLL 0x1000c27a
static bool
register_classes(void)
{
  bool result = 1;
  if (!register_MainClass())
    result = 0;
  if (!register_DeckSurfaceClass())
    result = 0;
  if (!register_CardClass())
    result = 0;
  if (!register_FullCardClass())
    result = 0;
  if (!register_HorzListClass())
    result = 0;
  if (!register_CardListFilterClass())
    result = 0;
  if (!register_CueCardClass())
    result = 0;
  if (!register_TitleClass())
    result = 0;
  return result;
}

// GLOBAL: DECKDLL 0x10104d78
static int global_unknown_10104d78;

// FUNCTION: DECKDLL 0x1000f190
static int __stdcall
check_deckbuilder_dat(void)
{
  return 1;
}

// FUNCTION: DECKDLL 0x1001b5e7
static int
load_newdata_csv(const char *newdata_csv_path, int available_slots)
{
  (void)newdata_csv_path;
  (void)available_slots;
  global_unknown_10104d78 = 0;
  return 1;
}

// FUNCTION: DECKDLL 0x1000b8df
static int
init_deckbuilder(HINSTANCE dll, int unused)
{
  char *pcVar3;         /* [ebp-0x110] */
  HDC hdc;              /* [ebp-0x10c] */
  char path_buf[0x108]; /* [ebp-0x108] */

  global_hinstance = dll;
  if (InitLicenseSecretsFromRegistry())
    return 0;

  read_manalink_ini();
  global_cfg_consolidate = (int)(signed char)global_cfg.consolidate_from_registry;
  global_deck_was_edited = 0;
  global_deckname_set = 0;
  global_deckbuilder_unknown_1019f010 = 0;
  global_current_deck = 0;

#ifdef _DEBUG
  // Allow debugging directly from output directory
  if (getcwd(global_base_directory, sizeof(global_base_directory)) == NULL)
  {
    return 0;
  }
#else
  GetModuleFileNameA(0, global_base_directory, 0x105);
  pcVar3 = strrchr(global_base_directory, '\\');
  *pcVar3 = 0;
#endif

  strcpy(global_manalink_ini_path, global_base_directory);
  strcpy(global_playdeck_path, global_base_directory);
  strcat(global_playdeck_path, "\\PlayDeck");
  strcpy(global_cardart_path, global_base_directory);
  strcat(global_cardart_path, "\\CARDART");
  strcpy(global_duelart_path, global_base_directory);
  strcat(global_duelart_path, "\\DUELART");
  strcpy(global_dbart_path, global_base_directory);
  strcat(global_dbart_path, "\\DBART");
  strcpy(global_duel_dat_path, global_duelart_path);
  strcat(global_duel_dat_path, "\\Duel.dat");

  strcpy(global_ui_strings_filename, "UIStrings.txt");
  load_text(global_ui_strings_filename, "ARTISTLINE");
  strcpy(global_artistline, text_lines[0]);
  global_unknown_1012df44 = 1;

  strcpy(path_buf, global_base_directory);
  strcat(path_buf, "\\CARDS.DAT");
  global_available_slots = read_db_guts(path_buf);
  if (global_available_slots == 0)
    fatal_err("WinMain: Couldn't find CARDS.DAT", (HWND)0);

  strcpy(path_buf, global_base_directory);
  strcat(path_buf, "\\NEWDATA.CSV");
  if (!load_newdata_csv(path_buf, global_available_slots))
    fatal_err("WinMain: Couldn't find CARDS.DAT", (HWND)0);

  hdc = GetDC(GetDesktopWindow());
  global_supports_palette = GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE;
  global_color_depth = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
  ReleaseDC(GetDesktopWindow(), hdc);

  global_fonts_init_state = 3;
  InitCommonControls();

  if (!check_deckbuilder_dat())
  {
    MessageBoxA((HWND)0, "Deckbuilder.dat is either missing or currupt!", "Magic: The Gathering", 0x10);
    return 0;
  }

  if (!register_classes())
  {
    fatal_err("WinMain: Couldn't register the classes", (HWND)0);
  }

  return 0;
}

// FUNCTION: DECKDLL 0x1000b860
BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, LPVOID reserved)
{
  int result;

  switch (reason)
  {
  case DLL_PROCESS_ATTACH:
    result = 1;
    init_deckbuilder(dll, 0);
    return result;
  case DLL_THREAD_ATTACH:
    return 1;
  case DLL_THREAD_DETACH:
    return 1;
  case DLL_PROCESS_DETACH:
    return 1;
  }
}

BOOL WINAPI DllEntryPoint(HINSTANCE dll, DWORD reason, LPVOID reserved) // Stupidly enough, there's a direct reference to this alternate name.
{
  return DllMain(dll, reason, reserved);
}

// FUNCTION: DECKDLL 0x1001a8ca
static void
sound_init(const char *path, int num)
{
  Sound snd;

  if (global_db_flags_1 & DBFLAGS_SHANDALAR)
    return;

  memset(&snd, 0, sizeof(Sound));
  snd.flags = (int)snd.flags | 4;
  snd.volume = 400;
  snd.sampleRate = 0;
  snd.pan = 0;

  sound_load(path, num, &snd);

  set_sound_loop(num, 1);
}

// FUNCTION: DECKDLL 0x1001a62a
static void
init_sounds_and_music(void)
{
  char path[0x50];
  int i;

  if (global_db_flags_1 & DBFLAGS_SHANDALAR)
    return;

  // Very expensive srand()
  for (i = 0; (clock() % 1024) > i; i++)
  {
    rand();
  }

  i = (rand() % 0x13) + 1;
  sprintf(path, "sound\\locmus%d.wav", i);
  sound_init(path, 1);
  sprintf(path, "DuelSounds\\discard.wav");
  sound_load(path, 3, 0);
  sprintf(path, "DuelSounds\\draw.wav");
  sound_load(path, 2, 0);
  sprintf(path, "DuelSounds\\button.wav");
  sound_load(path, 4, 0);
  sprintf(path, "DuelSounds\\cancel.wav");
  sound_load(path, 5, 0);
}

// FUNCTION: DECKDLL 0x1001a741
static void
free_sounds_and_music(void)
{
  if (global_db_flags_1 & 1)
    return;

  sound_unload(1);
  sound_unload(2);
  sound_unload(3);
  sound_unload(4);
  sound_unload(5);
  sound_close();
}

// FUNCTION: DECKDLL 0x1001a795
void play_sound(int a1, int a2, int a3, int a4)
{
  Sound snd;

  if (global_db_flags_1 & DBFLAGS_SHANDALAR)
    return;

  memset(&snd, 0, sizeof(Sound));
  snd.volume = 4 * a2;
  snd.sampleRate = 22050 * a3 / 100;
  snd.pan = 4 * a4;
  snd.flags &= ~1;
  snd.flags &= ~0x10;

  sound_play(a1, &snd);
}

// FUNCTION: DECKDLL 0x1001a80f
static void
play_music(int a1, int a2, int a3)
{
  Sound snd;

  if (global_db_flags_1 & DBFLAGS_SHANDALAR)
    return;

  memset(&snd, 0, sizeof(Sound));
  snd.volume = 4 * a2;
  snd.sampleRate = 22050;
  snd.pan = 4 * a3;
  snd.flags = (int)snd.flags | 1;

  sound_play(a1, &snd);
}

// FUNCTION: DECKDLL 0x10006e7f
INT_PTR CALLBACK
dlgproc_DeckInfo(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
  /* Stack layout is extremely sensitive; keep locals grouped. */
  HDC hdc2;             /* [ebp-0x134] */
  RECT r;               /* [ebp-0x130] */
  LPARAM lparam_copy;   /* [ebp-0x120] */
  HDC hdc;              /* [ebp-0x11c] */
  HGDIOBJ hollow_brush; /* [ebp-0x118] */
  int text_length;      /* [ebp-0x114] */
  char buf[264];        /* [ebp-0x110] */
  int deckface_count;   /* [ebp-0x08] */
  char *p;              /* [ebp-0x04] */

  switch (msg)
  {
  case WM_INITDIALOG:
    load_text("menus", "TITLEDIALOG");
    SetWindowTextA(hdlg, text_lines[0]);
    SetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_DECKTITLE), text_lines[1]);
    SetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_DESCRIPTION), text_lines[2]);
    SetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_NAME), text_lines[3]);
    SetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_EMAIL), text_lines[4]);
    SetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_DATE), text_lines[5]);
    SetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_DECKFACES), text_lines[6]);
    SetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_COMMENT), text_lines[7]);
    SetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_VERSION), text_lines[8]);

    load_text("menus", "OKCANCEL");
    SetWindowTextA(GetDlgItem(hdlg, RES_BUTTON_OK), text_lines[0]);
    SetWindowTextA(GetDlgItem(hdlg, RES_BUTTON_CANCEL), text_lines[1]);

    SetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_DECKTITLE_EDITTEXT), global_deckinfo.deckname);
    SendMessageA(GetDlgItem(hdlg, RES_DECKINFO_DECKTITLE_EDITTEXT), EM_LIMITTEXT, 0x1c, 0);

    SetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_DESCRIPTION_EDITTEXT), global_deckinfo.description);
    SendMessageA(GetDlgItem(hdlg, RES_DECKINFO_DESCRIPTION_EDITTEXT), EM_LIMITTEXT, 0x12, 0);

    SetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_NAME_EDITTEXT), global_deckinfo.author);
    SendMessageA(GetDlgItem(hdlg, RES_DECKINFO_NAME_EDITTEXT), EM_LIMITTEXT, 0x4e, 0);

    SetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_EMAIL_EDITTEXT), global_deckinfo.email);
    SendMessageA(GetDlgItem(hdlg, RES_DECKINFO_EMAIL_EDITTEXT), EM_LIMITTEXT, 0x4e, 0);

    SetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_DATE_EDITTEXT), global_deckinfo.creation_date);
    SendMessageA(GetDlgItem(hdlg, RES_DECKINFO_DATE_EDITTEXT), EM_LIMITTEXT, 0x13, 0);

    SetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_VERSION_EDITTEXT), "4th Edition");
    SendMessageA(GetDlgItem(hdlg, RES_DECKINFO_VERSION_EDITTEXT), EM_LIMITTEXT, 0xd, 0);

    SetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_COMMENT_EDITTEXT), global_deckinfo.comments);
    SendMessageA(GetDlgItem(hdlg, RES_DECKINFO_COMMENT_EDITTEXT), EM_LIMITTEXT, 0x18e, 0);

    deckface_count = load_text("menus", "DECKFACES");
    for (text_length = 0; text_length < deckface_count; text_length = text_length + 1)
      SendMessageA(GetDlgItem(hdlg, RES_DECKINFO_DECKFACES_COMBOBOX), LB_ADDSTRING, 0, (LPARAM)(text_lines + text_length));
    SendMessageA(GetDlgItem(hdlg, RES_DECKINFO_DECKFACES_COMBOBOX), LB_SETCURSEL, global_deckinfo.revision - 1, 0);

    sprintf(buf, "%s\\GAUN_Results.pic", global_duelart_path);
    global_deckinfo_pic = load_pic(buf);
    return 0;

  case WM_COMMAND:
  {
    if ((((unsigned int)wparam) & 0xffff) == RES_BUTTON_OK)
    {
      text_length = GetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_DECKTITLE_EDITTEXT), buf, 0x105);
      p = buf;
      while (*p != '\0')
      {
        if (*p == '.')
          strcpy(p, p + 1);
        else
          p = p + 1;
      }
      buf[text_length] = '\0';
      if (strcmp(buf, global_deckinfo.deckname) != 0)
      {
        global_deck_was_edited = 1;
        global_deckname_set = 1;
      }
      strcpy(global_deckinfo.deckname, buf);
      text_length = 0;

      text_length = GetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_DESCRIPTION_EDITTEXT), global_deckinfo.description, 0x15);
      global_deckinfo.description[text_length] = '\0';
      text_length = 0;

      text_length = GetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_NAME_EDITTEXT), global_deckinfo.author, 0x51);
      global_deckinfo.author[text_length] = '\0';
      text_length = 0;

      text_length = GetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_EMAIL_EDITTEXT), global_deckinfo.email, 0x51);
      global_deckinfo.email[text_length] = '\0';
      text_length = 0;

      text_length = GetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_DATE_EDITTEXT), global_deckinfo.creation_date, 0x16);
      global_deckinfo.creation_date[text_length] = '\0';

      global_deckinfo.revision = SendMessageA(GetDlgItem(hdlg, RES_DECKINFO_DECKFACES_COMBOBOX), LB_GETCURSEL, 0, 0) + 1;

      GetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_VERSION_EDITTEXT), global_deckinfo.edition, 0x10);
      text_length = 0;

      text_length = GetWindowTextA(GetDlgItem(hdlg, RES_DECKINFO_COMMENT_EDITTEXT), global_deckinfo.comments, 0x191);
      global_deckinfo.comments[text_length] = '\0';

      strcpy(global_deck_filename, global_playdeck_path);
      strcat(global_deck_filename, "\\");
      strcat(global_deck_filename, global_deckinfo.deckname);
      strcat(global_deck_filename, ".dck");
      EndDialog(hdlg, 1);
      if (global_deckinfo_pic != (HANDLE)0)
        delete_and_close_object(global_deckinfo_pic);
    }
    else if ((((unsigned int)wparam) & 0xffff) == RES_BUTTON_CANCEL)
    {
      if (global_deckinfo_pic != (HANDLE)0)
        delete_and_close_object(global_deckinfo_pic);
      EndDialog(hdlg, 0);
    }
    return 1;
  }

  case WM_CTLCOLORBTN:
  case WM_CTLCOLORSTATIC:
  {
    hdc = (HDC)wparam;
    ApplyCardArtPaletteToDc(hdc);
    lparam_copy = lparam;
    SetBkMode(hdc, TRANSPARENT);
    hollow_brush = GetStockObject(HOLLOW_BRUSH);
    return (INT_PTR)hollow_brush;
  }

  case WM_ERASEBKGND:
  {
    hdc2 = (HDC)wparam;
    ApplyCardArtPaletteToDc(hdc2);
    GetClientRect(hdlg, &r);
    if (global_deckinfo_pic != (HANDLE)0)
    {
      DrawBitmapToRect(hdc2, &r, global_deckinfo_pic);
    }
    else
    {
      FillRect(hdc2, &r, (HBRUSH)GetStockObject(GRAY_BRUSH));
    }
    return 1;
  }

  default:
    return 0;
  }
}

// FUNCTION: DECKDLL 0x10006df3
bool show_dialog_deckinfo(void)
{
  int result;
  INT_PTR dlg_result;

  dlg_result = DialogBoxParam(global_hinstance, MAKEINTRESOURCE(RES_DIALOG_DECKINFO), global_main_hwnd, dlgproc_DeckInfo, 0);
  if (dlg_result == -1)
  {
    MessageBox(global_main_hwnd, "Couldn't bring up the title dialog box", "", 0);
    result = 0;
  }
  else if (dlg_result == 0)
  {
    result = 0;
  }
  else if (dlg_result == 1)
  {
    result = 1;
  }
  return result;
}

// FUNCTION: DECKDLL 0x10007bad
INT_PTR CALLBACK
dlgproc_GroupMove(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
  /* Stack layout is extremely sensitive; keep locals grouped. */
  HDC hdc_erase;      /* [ebp-0x128] */
  RECT r;             /* [ebp-0x124] */
  LPARAM lparam_copy; /* [ebp-0x114] */
  HDC hdc;            /* [ebp-0x110] */
  HGDIOBJ pv;         /* [ebp-0x10c] */
  char path[264];     /* [ebp-0x108] */

  switch (msg)
  {
  case WM_INITDIALOG:
  {
    load_text("menus", "GROUPMOVE");
    SetWindowTextA(hdlg, text_lines[0]);
    SetWindowTextA(GetDlgItem(hdlg, RES_GROUPMOVE_BUTTON_BLACK), text_lines[1]);
    SetWindowTextA(GetDlgItem(hdlg, RES_GROUPMOVE_BUTTON_BLUE), text_lines[2]);
    SetWindowTextA(GetDlgItem(hdlg, RES_GROUPMOVE_BUTTON_GREEN), text_lines[3]);
    SetWindowTextA(GetDlgItem(hdlg, RES_GROUPMOVE_BUTTON_RED), text_lines[4]);
    SetWindowTextA(GetDlgItem(hdlg, RES_GROUPMOVE_BUTTON_WHITE), text_lines[5]);
    SetWindowTextA(GetDlgItem(hdlg, RES_GROUPMOVE_BUTTON_ARTIFACT), text_lines[6]);

    load_text("menus", "OKCANCEL");
    SetWindowTextA(GetDlgItem(hdlg, RES_BUTTON_OK), text_lines[0]);
    SetWindowTextA(GetDlgItem(hdlg, RES_BUTTON_CANCEL), text_lines[1]);

    EnableWindow(GetDlgItem(hdlg, RES_GROUPMOVE_BUTTON_BLACK), global_dlg_parameter & 1);
    EnableWindow(GetDlgItem(hdlg, RES_GROUPMOVE_BUTTON_BLUE), (global_dlg_parameter & 2U) >> 1);
    EnableWindow(GetDlgItem(hdlg, RES_GROUPMOVE_BUTTON_GREEN), (global_dlg_parameter & 4U) >> 2);
    EnableWindow(GetDlgItem(hdlg, RES_GROUPMOVE_BUTTON_RED), (global_dlg_parameter & 8U) >> 3);
    EnableWindow(GetDlgItem(hdlg, RES_GROUPMOVE_BUTTON_WHITE), (global_dlg_parameter & 0x10U) >> 4);
    EnableWindow(GetDlgItem(hdlg, RES_GROUPMOVE_BUTTON_ARTIFACT), (global_dlg_parameter & 0x20U) >> 5);

    sprintf(path, "%s\\GAUN_Options.pic", global_duelart_path);
    global_groupmove_pic = load_pic(path);
    return 0;
  }

  case WM_COMMAND:
  {
    if ((((unsigned int)wparam) & 0xffff) == RES_BUTTON_OK)
    {
      global_dlg_result = 0;
      if (IsDlgButtonChecked(hdlg, RES_GROUPMOVE_BUTTON_BLACK))
        global_dlg_result |= 1;
      if (IsDlgButtonChecked(hdlg, RES_GROUPMOVE_BUTTON_BLUE))
        global_dlg_result |= 2;
      if (IsDlgButtonChecked(hdlg, RES_GROUPMOVE_BUTTON_GREEN))
        global_dlg_result |= 4;
      if (IsDlgButtonChecked(hdlg, RES_GROUPMOVE_BUTTON_RED))
        global_dlg_result |= 8;
      if (IsDlgButtonChecked(hdlg, RES_GROUPMOVE_BUTTON_WHITE))
        global_dlg_result |= 0x10;
      if (IsDlgButtonChecked(hdlg, RES_GROUPMOVE_BUTTON_ARTIFACT))
        global_dlg_result |= 0x20;
      if (global_groupmove_pic != (HANDLE)0)
        delete_and_close_object(global_groupmove_pic);
      EndDialog(hdlg, 1);
    }
    else if ((((unsigned int)wparam) & 0xffff) == RES_BUTTON_CANCEL)
    {
      if (global_groupmove_pic != (HANDLE)0)
        delete_and_close_object(global_groupmove_pic);
      global_dlg_result = 0;
      EndDialog(hdlg, 0);
    }
    return 1;
  }

  case WM_CTLCOLORBTN:
  case WM_CTLCOLORSTATIC:
  {
    hdc = (HDC)wparam;
    ApplyCardArtPaletteToDc(hdc);
    lparam_copy = lparam;
    SetBkMode(hdc, TRANSPARENT);
    pv = GetStockObject(HOLLOW_BRUSH);
    return (INT_PTR)pv;
  }

  case WM_ERASEBKGND:
  {
    hdc_erase = (HDC)wparam;
    ApplyCardArtPaletteToDc(hdc_erase);

    GetClientRect(hdlg, &r);
    if (global_groupmove_pic != (HANDLE)0)
    {
      DrawBitmapToRect(hdc_erase, &r, global_groupmove_pic);
    }
    else
    {
      FillRect(hdc_erase, &r, (HBRUSH)GetStockObject(GRAY_BRUSH));
    }
    return 1;
  }

  default:
    return 0;
  }
}

// FUNCTION: DECKDLL 0x10007b21
bool show_dialog_groupmove(void)
{
  int result;
  INT_PTR dlg_result;

  dlg_result = DialogBoxParam(global_hinstance, MAKEINTRESOURCE(RES_DIALOG_GROUPMOVE), global_main_hwnd, dlgproc_GroupMove, 0);
  if (dlg_result == -1)
  {
    MessageBox(global_main_hwnd, "Couldn't bring up the filter dialog box", "", 0);
    result = 0;
  }
  else if (dlg_result == 0)
  {
    result = 0;
  }
  else if (dlg_result == 1)
  {
    result = 1;
  }
  return result;
}

// FUNCTION: DECKDLL 0x10006aba
INT_PTR CALLBACK dlgproc_AskX(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
  HDC hdc_erase;      /* [ebp-0x158] */
  RECT client_rect;   /* [ebp-0x154] */
  LPARAM lparam_copy; /* [ebp-0x144] */
  HDC hdc_copy;       /* [ebp-0x140] */
  HGDIOBJ pv;         /* [ebp-0x13c] */
  int text_len;       /* [ebp-0x138] */
  char path[264];     /* [ebp-0x134] */
  char edit_txt[12];  /* [ebp-0x2c] */
  RECT rect_main;     /* [ebp-0x20] */
  RECT rect_dlg;      /* [ebp-0x10] */

  switch (msg)
  {
  case WM_INITDIALOG:
  {
    sprintf(edit_txt, "%d", global_dlg_parameter);
    SetWindowTextA(hdlg, global_ask_x_dlg_title);
    SetWindowTextA(GetDlgItem(hdlg, RES_ASKVALUE_EDITTEXT), edit_txt);

    load_text("menus", "OKCANCEL");
    SetWindowTextA(GetDlgItem(hdlg, RES_BUTTON_OK), text_lines[0]);
    SetWindowTextA(GetDlgItem(hdlg, RES_BUTTON_CANCEL), text_lines[1]);

    GetWindowRect(global_main_hwnd, &rect_main);
    GetWindowRect(hdlg, &rect_dlg);
    SetWindowPos(hdlg, (HWND)0,
                 rect_main.left +
                     ((rect_main.right - rect_main.left) - (rect_dlg.right - rect_dlg.left)) / 2,
                 rect_main.top +
                     ((rect_main.bottom - rect_main.top) - (rect_dlg.bottom - rect_dlg.top)) / 2,
                 0, 0, (SWP_NOSIZE | SWP_NOZORDER));

    sprintf(path, "%s\\GAUN_Options.pic", global_duelart_path);
    global_gaun_options_pic = load_pic(path);
    return 0;
  }

  case WM_COMMAND:
  {
    if ((((unsigned int)wparam) & 0xffff) == RES_BUTTON_OK)
    {
      text_len = GetWindowTextA(GetDlgItem(hdlg, RES_ASKVALUE_EDITTEXT), edit_txt, 10);
      edit_txt[text_len] = '\0';
      global_dlg_result = atoi(edit_txt);
      EndDialog(hdlg, 1);
      if (global_gaun_options_pic != (HANDLE)0)
      {
        delete_and_close_object(global_gaun_options_pic);
      }
    }
    else if ((((unsigned int)wparam) & 0xffff) == RES_BUTTON_CANCEL)
    {
      if (global_gaun_options_pic != (HANDLE)0)
      {
        delete_and_close_object(global_gaun_options_pic);
      }
      global_dlg_result = 0;
      EndDialog(hdlg, 0);
    }
    return 1;
  }

  case WM_CTLCOLORBTN:
  case WM_CTLCOLORSTATIC:
    hdc_copy = (HDC)wparam;
    ApplyCardArtPaletteToDc(hdc_copy);
    lparam_copy = lparam;
    SetBkMode(hdc_copy, TRANSPARENT);
    pv = GetStockObject(HOLLOW_BRUSH);
    return (INT_PTR)pv;

  case WM_ERASEBKGND:
    hdc_erase = (HDC)wparam;
    ApplyCardArtPaletteToDc(hdc_erase);
    GetClientRect(hdlg, &client_rect);
    if (global_gaun_options_pic != (HANDLE)0)
    {
      DrawBitmapToRect(hdc_erase, &client_rect, global_gaun_options_pic);
    }
    else
    {
      FillRect(hdc_erase, &client_rect, (HBRUSH)GetStockObject(GRAY_BRUSH));
    }
    return 1;

  default:
    return 0;
  }
}

// FUNCTION: DECKDLL 0x10006960
static bool show_dialog_sellxcards(void)
{
  int result;
  INT_PTR dlg_result;

  load_text("menus", "NUMBERTOSELL");
  strcpy(global_ask_x_dlg_title, text_lines[0]);
  dlg_result = DialogBoxParam(global_hinstance, MAKEINTRESOURCE(RES_DIALOG_ASK_VALUE), global_main_hwnd, dlgproc_AskX, 0);
  if (dlg_result == -1)
  {
    MessageBox(global_main_hwnd, "Couldn't bring up the filter dialog box", "", MB_OK);
    result = 0;
  }
  else if (dlg_result == 0)
  {
    result = 0;
  }
  else if (dlg_result == 1)
  {
    result = 1;
  }
  return result;
}

// FUNCTION: DECKDLL 0x10006a0d
static bool show_dialog_movexcards(void)
{
  int result;
  INT_PTR dlg_result;

  load_text("menus", "NUMBERTOMOVE");
  strcpy(global_ask_x_dlg_title, text_lines[0]);
  dlg_result = DialogBoxParam(global_hinstance, MAKEINTRESOURCE(RES_DIALOG_ASK_VALUE), global_main_hwnd, dlgproc_AskX, 0);
  if (dlg_result == -1)
  {
    MessageBox(global_main_hwnd, "Couldn't bring up the filter dialog box", "", MB_OK);
    result = 0;
  }
  else if (dlg_result == 0)
  {
    result = 0;
  }
  else if (dlg_result == 1)
  {
    result = 1;
  }
  return result;
}

// FUNCTION: DECKDLL 0x1000774f
INT_PTR CALLBACK
dlgproc_InfoBox(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
  /* Stack layout is extremely sensitive; keep locals grouped. */
  int tmp;                   /* [ebp-0x138] */
  HDC hdc_erase;             /* [ebp-0x134] */
  RECT r;                    /* [ebp-0x130] */
  LPARAM lparam_copy;        /* [ebp-0x120] */
  HDC hdc_ctl;               /* [ebp-0x11c] */
  HGDIOBJ hobj;              /* [ebp-0x118] */
  char background_path[264]; /* [ebp-0x114] */
  int j;                     /* [ebp-0x0c] */
  int idx;                   /* [ebp-0x08] */
  int done;                  /* [ebp-0x04] */

  switch (msg)
  {
  case WM_INITDIALOG:
    load_text("menus", "EXTRACARDSDIALOG");
    SetWindowTextA(hdlg, text_lines[0]);
    SetWindowTextA(GetDlgItem(hdlg, RES_INFOBOX_DESCRIPTION), text_lines[1]);
    SetWindowTextA(GetDlgItem(hdlg, RES_BUTTON_OK), text_lines[2]);
    SetWindowTextA(GetDlgItem(hdlg, RES_BUTTON_CANCEL), text_lines[3]);

    done = 0;
    idx = done;
    while (done == 0)
    {
      if (((int *)global_excessive_cards)[idx * 3] != -1)
      {
        tmp = idx;
        idx++;
        SendDlgItemMessageA(hdlg, RES_INFOBOX_LIST, LB_ADDSTRING, 0, ((LPARAM *)global_excessive_cards)[tmp * 3 + 2]);
      }
      else
      {
        done = 1;
      }
    }

    sprintf(background_path, "%s\\GAUN_Results.pic", global_duelart_path);
    global_gaun_results_pic = load_pic(background_path);
    return 0;

  case WM_COMMAND:
    if ((((unsigned int)wparam) & 0xffff) == RES_BUTTON_OK)
    {
      done = 0;
      idx = done;
      while (done == 0)
      {
        if (global_excessive_cards[idx].DeckEntry_csvid != -1)
        {
          for (j = 0; j < global_deck_num_entries; ++j)
          {
            if ((global_deck[j].GDE_csvid == global_excessive_cards[idx].DeckEntry_csvid) &&
                (global_deck[j].GDE_Available == 0) &&
                (0 < global_excessive_cards[idx].DeckEntry_Amount))
            {
              global_deck[j].GDE_Available = 1;
              global_deck[j].GDE_DecksBits &= ~(1 << ((unsigned char)global_current_deck));
              global_excessive_cards[idx].DeckEntry_Amount--;
            }
          }
          ++idx;
        }
        else
        {
          done = 1;
        }
      }

      EndDialog(hdlg, 1);
      if (global_gaun_results_pic != (HANDLE)0)
        delete_and_close_object(global_gaun_results_pic);
    }
    else if ((((unsigned int)wparam) & 0xffff) == RES_BUTTON_CANCEL)
    {
      if (global_gaun_results_pic != (HANDLE)0)
        delete_and_close_object(global_gaun_results_pic);
      EndDialog(hdlg, 0);
    }
    return 1;

  case WM_CTLCOLORBTN:
  case WM_CTLCOLORSTATIC:
    hdc_ctl = (HDC)wparam;
    ApplyCardArtPaletteToDc(hdc_ctl);
    lparam_copy = lparam;
    SetBkMode(hdc_ctl, TRANSPARENT);
    hobj = GetStockObject(HOLLOW_BRUSH);
    return (INT_PTR)hobj;

  case WM_ERASEBKGND:
    hdc_erase = (HDC)wparam;
    ApplyCardArtPaletteToDc(hdc_erase);
    GetClientRect(hdlg, &r);

    if (global_gaun_results_pic != (HANDLE)0)
    {
      DrawBitmapToRect(hdc_erase, &r, global_gaun_results_pic);
    }
    else
    {
      FillRect(hdc_erase, &r, (HBRUSH)GetStockObject(GRAY_BRUSH));
    }
    return 1;

  default:
    return 0;
  }
}

// FUNCTION: DECKDLL 0x100076c3
static bool show_dialog_infobox(void)
{
  int result;
  INT_PTR dlg_result;

  dlg_result = DialogBoxParam(global_hinstance, MAKEINTRESOURCE(RES_DIALOG_INFOBOX), global_main_hwnd, dlgproc_InfoBox, 0);
  if (dlg_result == -1)
  {
    MessageBox(global_main_hwnd, "Couldn't bring up the title dialog box", "", 0);
    result = 0;
  }
  else if (dlg_result == 0)
  {
    result = 0;
  }
  else if (dlg_result == 1)
  {
    result = 1;
  }
  return result;
}

// FUNCTION: DECKDLL 0x10019fe9
INT_PTR CALLBACK
dlgproc_LoadDeck(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
  /* Stack layout is extremely sensitive; keep locals grouped. */
  HDC pad;
  RECT client_rect;        /* [ebp-0x53c] */
  LPARAM lparam_copy;      /* [ebp-0x530] */
  HDC hdc_copy;            /* [ebp-0x52c] */
  int pad_500;             /* [ebp-0x524] */
  WPARAM sel;              /* [ebp-0x520] */
  char filename[32];       /* [ebp-0x51c] */
  char path[0x108];        /* [ebp-0x4fc] */
  char listbox_buf[0x108]; /* [ebp-0x3f4] */
  HWND hwnd_list;          /* [ebp-0x2ec] */
  int count;               /* [ebp-0x2e8] */
  char deckname[200];      /* [ebp-0x2e4] */
  char prev_dir[0x108];    /* [ebp-0x21c] */
  WPARAM i;                /* [ebp-0x114] */
  FILE *fp;                /* [ebp-0x110] */
  char *p;                 /* [ebp-0x10c] */
  char gaun_path[0x108];   /* [ebp-0x108] */

  {
    switch (msg)
    {
    case WM_INITDIALOG:
      load_text("menus", "LOADDECKDIALOG");
      SetWindowTextA(hdlg, text_lines[0]);
      SetWindowTextA(GetDlgItem(hdlg, RES_LOADDECK_PLAYERDECK), text_lines[1]);

      load_text("menus", "OKCANCEL");
      SetWindowTextA(GetDlgItem(hdlg, RES_BUTTON_OK), text_lines[0]);
      SetWindowTextA(GetDlgItem(hdlg, RES_BUTTON_CANCEL), text_lines[1]);

      hwnd_list = CreateWindowExA(0, "LISTBOX", "",
                                  (WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_SORT),
                                  0, 0, 0, 0,
                                  hdlg, (HMENU)0, global_hinstance, (LPVOID)0);
      if (hwnd_list == (HWND)0)
      {
        EndDialog(hdlg, -1);
        return 1;
      }

      GetCurrentDirectoryA(0x104, prev_dir);
      _chdir(global_base_directory);

      strcpy(listbox_buf, "playdeck\\*.DCK");
      SendMessageA(hwnd_list, LB_DIR, 0, (LPARAM)listbox_buf);
      count = SendMessageA(hwnd_list, LB_GETCOUNT, 0, 0);

      for (i = 0; (int)i < count; i = i + 1)
      {
        SendMessageA(hwnd_list, LB_GETTEXT, i, (LPARAM)listbox_buf);
        strcpy(path, "playdeck\\");
        strcat(path, listbox_buf);

        fp = fopen(path, "r");
        if (fp != (FILE *)0)
        {
          if (fgetc(fp) != ';')
          {
            fclose(fp);
            continue;
          }
          deckname[0] = '\0';
          p = (char *)&deckname + strlen(deckname);
          while ((*p = (char)fgetc(fp)) != '\n')
          {
            p = p + 1;
          }
          *p = '\0';
          fclose(fp);

          strcpy(path, "playdeck\\");
          strcat(path, deckname);
          strcat(path, ".dck");
          fp = fopen(path, "r");
          if (fp != (FILE *)0)
          {
            SendDlgItemMessageA(hdlg, RES_LOADDECK_DECKLIST, CB_ADDSTRING, 0, (LPARAM)deckname);
            fclose(fp);
          }
        }
      }

      SendDlgItemMessageA(hdlg, RES_LOADDECK_DECKLIST, CB_SETCURSEL, 0, 0);
      _chdir(prev_dir);

      sprintf(gaun_path, "%s\\GAUN_Results.pic", global_duelart_path);
      global_loaddeck_pic = load_pic(gaun_path);
      return 0;

    case WM_COMMAND:
      if ((wparam & 0xFFFF) == RES_BUTTON_OK)
      {
        sel = SendDlgItemMessageA(hdlg, RES_LOADDECK_DECKLIST, CB_GETCURSEL, 0, 0);
        SendDlgItemMessageA(hdlg, RES_LOADDECK_DECKLIST, CB_GETLBTEXT, sel, (LPARAM)filename);
        sprintf(global_loaddeck_dlg_filename, "%s\\%s.dck", global_playdeck_path, filename);
        if (global_loaddeck_pic != (HANDLE)0)
        {
          delete_and_close_object(global_loaddeck_pic);
        }
        EndDialog(hdlg, 1);
      }
      else if ((wparam & 0xFFFF) == RES_BUTTON_CANCEL)
      {
        if (global_loaddeck_pic != (HANDLE)0)
        {
          delete_and_close_object(global_loaddeck_pic);
        }
        EndDialog(hdlg, 0);
      }
      return 1;

    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSTATIC:
      hdc_copy = (HDC)wparam;
      ApplyCardArtPaletteToDc(hdc_copy);
      lparam_copy = lparam;
      SetBkMode(hdc_copy, TRANSPARENT);
      return (INT_PTR)(pad_500 = (int)GetStockObject(HOLLOW_BRUSH));

    case WM_ERASEBKGND:
      pad = (HDC)wparam;
      ApplyCardArtPaletteToDc(pad);
      GetClientRect(hdlg, &client_rect);
      if (global_loaddeck_pic != (HANDLE)0)
      {
        DrawBitmapToRect(pad, &client_rect, global_loaddeck_pic);
      }
      else
      {
        FillRect(pad, &client_rect, (HBRUSH)GetStockObject(2));
      }
      return 1;

    default:
      return 0;
    }
  }
}

// FUNCTION: DECKDLL 0x10019f4c
static bool show_dialog_loaddeck(char *dest)
{
  int ret;
  INT_PTR dlg_result;

  dlg_result = DialogBoxParam(global_hinstance, MAKEINTRESOURCE(RES_DIALOG_LOADDECK), global_main_hwnd,
                              dlgproc_LoadDeck, 0);
  if (dlg_result == -1)
  {
    MessageBox(global_main_hwnd, "Couldn't bring up the Load Deck dialog", "", MB_OK);
    ret = 0;
  }
  else if (dlg_result == 0)
  {
    ret = 0;
  }
  else if (dlg_result == 1)
  {
    strcpy(dest, global_loaddeck_dlg_filename);
    ret = 1;
  }

  return ret;
}

// FUNCTION: DECKDLL 0x1000679f
void clear_packs(void)
{
  int i;

  for (i = 0; i < 6; ++i)
  {
    global_packs[i][0].num = 0;
    global_packs[i][1].num = 0;
    global_packs[i][2].num = 0;
    global_packs[i][3].num = 0;
    global_packs[i][4].num = 0;
    global_packs[i][5].num = 0;
  }
}

// FUNCTION: DECKDLL 0x1000687f
void clear_packs_copy(void)
{
  int i;

  for (i = 0; i < 6; ++i)
  {
    global_packs_copy[i][0].num = 0;
    global_packs_copy[i][1].num = 0;
    global_packs_copy[i][2].num = 0;
    global_packs_copy[i][3].num = 0;
    global_packs_copy[i][4].num = 0;
    global_packs_copy[i][5].num = 0;
  }
}

// FUNCTION: DECKDLL 0x10005a9e
void count_packs(void)
{
  int amt;
  int l;
  unsigned int csvid;
#define STORE_PACK(p1, p2)                                                                                              \
  do                                                                                                                    \
  {                                                                                                                     \
    *(unsigned int *)&global_packs[(p1)][(p2)].table[global_packs[(p1)][(p2)].num] = ((unsigned int)amt << 16) | csvid; \
    ++global_packs[(p1)][(p2)].num;                                                                                     \
  } while (0)

  for (l = 0; l < 6; ++l)
  {
    global_packs[l][0].num = 0;
    global_packs[l][1].num = 0;
    global_packs[l][2].num = 0;
    global_packs[l][3].num = 0;
    global_packs[l][4].num = 0;
    global_packs[l][5].num = 0;
  }

  for (l = 0; l < global_edited_deck_num_entries; ++l)
  {
    csvid = global_edited_deck.entries[l].DeckEntry_csvid;
    amt = global_edited_deck.entries[l].DeckEntry_Amount;

    if (global_raw_cards_storage[csvid].card_type == CP_TYPE_LAND)
    {
      if (strcmp(global_raw_cards_storage[csvid].full_name, "Mountain") == 0)
        STORE_PACK(PACK1_RED, PACK2_LAND);
      else if (strcmp(global_raw_cards_storage[csvid].full_name, "Plains") == 0)
        STORE_PACK(PACK1_WHITE, PACK2_LAND);
      else if (strcmp(global_raw_cards_storage[csvid].full_name, "Forest") == 0)
        STORE_PACK(PACK1_GREEN, PACK2_LAND);
      else if (strcmp(global_raw_cards_storage[csvid].full_name, "Swamp") == 0)
        STORE_PACK(PACK1_BLACK, PACK2_LAND);
      else if (strcmp(global_raw_cards_storage[csvid].full_name, "Island") == 0)
        STORE_PACK(PACK1_BLUE, PACK2_LAND);
      else
        STORE_PACK(PACK1_OTHER, PACK2_LAND);
    }
    else if (global_raw_cards_storage[csvid].card_type == CP_TYPE_CREATURE)
    {
      if (global_raw_cards_storage[csvid].color == CP_COLOR_RED)
        STORE_PACK(PACK1_RED, PACK2_CREATURE);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_BLACK)
        STORE_PACK(PACK1_BLACK, PACK2_CREATURE);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_BLUE)
        STORE_PACK(PACK1_BLUE, PACK2_CREATURE);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_GREEN)
        STORE_PACK(PACK1_GREEN, PACK2_CREATURE);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_WHITE)
        STORE_PACK(PACK1_WHITE, PACK2_CREATURE);
      else
        STORE_PACK(PACK1_OTHER, PACK2_CREATURE);
    }
    else if (global_raw_cards_storage[csvid].card_type == CP_TYPE_ENCHANTMENT)
    {
      if (global_raw_cards_storage[csvid].color == CP_COLOR_RED)
        STORE_PACK(PACK1_RED, PACK2_ENCHANTMENT);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_BLACK)
        STORE_PACK(PACK1_BLACK, PACK2_ENCHANTMENT);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_BLUE)
        STORE_PACK(PACK1_BLUE, PACK2_ENCHANTMENT);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_GREEN)
        STORE_PACK(PACK1_GREEN, PACK2_ENCHANTMENT);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_WHITE)
        STORE_PACK(PACK1_WHITE, PACK2_ENCHANTMENT);
      else
        STORE_PACK(PACK1_OTHER, PACK2_ENCHANTMENT);
    }
    else if (global_raw_cards_storage[csvid].card_type == CP_TYPE_SORCERY)
    {
      if (global_raw_cards_storage[csvid].color == CP_COLOR_RED)
        STORE_PACK(PACK1_RED, PACK2_SORCERY);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_BLACK)
        STORE_PACK(PACK1_BLACK, PACK2_SORCERY);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_BLUE)
        STORE_PACK(PACK1_BLUE, PACK2_SORCERY);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_GREEN)
        STORE_PACK(PACK1_GREEN, PACK2_SORCERY);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_WHITE)
        STORE_PACK(PACK1_WHITE, PACK2_SORCERY);
      else
        STORE_PACK(PACK1_OTHER, PACK2_SORCERY);
    }
    else if (global_raw_cards_storage[csvid].card_type == CP_TYPE_INTERRUPT)
    {
      if (global_raw_cards_storage[csvid].color == CP_COLOR_RED)
        STORE_PACK(PACK1_RED, PACK2_INTERRUPT);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_BLACK)
        STORE_PACK(PACK1_BLACK, PACK2_INTERRUPT);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_BLUE)
        STORE_PACK(PACK1_BLUE, PACK2_INTERRUPT);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_GREEN)
        STORE_PACK(PACK1_GREEN, PACK2_INTERRUPT);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_WHITE)
        STORE_PACK(PACK1_WHITE, PACK2_INTERRUPT);
      else
        STORE_PACK(PACK1_OTHER, PACK2_INTERRUPT);
    }
    else if (global_raw_cards_storage[csvid].card_type == CP_TYPE_INSTANT)
    {
      if (global_raw_cards_storage[csvid].color == CP_COLOR_RED)
        STORE_PACK(PACK1_RED, PACK2_INSTANT);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_BLACK)
        STORE_PACK(PACK1_BLACK, PACK2_INSTANT);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_BLUE)
        STORE_PACK(PACK1_BLUE, PACK2_INSTANT);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_GREEN)
        STORE_PACK(PACK1_GREEN, PACK2_INSTANT);
      else if (global_raw_cards_storage[csvid].color == CP_COLOR_WHITE)
        STORE_PACK(PACK1_WHITE, PACK2_INSTANT);
      else
        STORE_PACK(PACK1_OTHER, PACK2_INSTANT);
    }
    else
      STORE_PACK(PACK1_OTHER, PACK2_INSTANT);
  }
#undef STORE_PACK
}

// FUNCTION: DECKDLL 0x100213ee
LRESULT CALLBACK
wndproc_CueCardClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  // GLOBAL: DECKDLL 0x10030028
  static int cuecard_font_offset = 0;

  char txt[100]; /* matches [ebp-0x104] */
  HDC paintdc;
  PAINTSTRUCT paint;
  HBRUSH brush_inner;
  RECT rect_inner;
  HBRUSH brush_outer;
  RECT rect_client;
  int height;
  HDC hdc_local;
  int width;
  UINT y;
  UINT x;
  LPCSTR str;
  SIZE sz;
  HFONT font;
  RECT rect_parent;

  switch (msg)
  {
  case 0x400:
    font = (HFONT)GetWindowLongA(hwnd, cuecard_font_offset);

    x = (UINT)wparam & 0xFFFF;
    y = ((UINT)wparam >> 16) & 0xFFFF;
    str = (LPCSTR)lparam;

    hdc_local = GetDC(hwnd);
    if (hdc_local != 0)
    {
      ApplyCardArtPaletteToDc(hdc_local);
      SelectObject(hdc_local, font);

      GetTextExtentPointA(hdc_local, str, lstrlenA(str), &sz);

      width = sz.cx + 10;
      height = sz.cy + 6;

      ReleaseDC(hwnd, hdc_local);

      GetClientRect(GetParent(hwnd), &rect_parent);
      if (rect_parent.right < width + (int)x)
        x = rect_parent.right - width;

      MoveWindow(hwnd, x, y, width, height, TRUE);
      SetWindowTextA(hwnd, str);
      ShowWindow(hwnd, SW_SHOW);
      InvalidateRect(hwnd, NULL, TRUE);
    }
    return 0;

  case WM_CREATE:
    font = CreateFontA(12, 0, 0, 0, 400, 0, 0, 0, 0, 4, 0, 0, 0, "Arial");
    if (font == 0)
      return -1;
    SetWindowLongA(hwnd, cuecard_font_offset, (LONG)font);
    return 0;

  case WM_DESTROY:

#ifdef MODERN_FIXES
    font = (HFONT)GetWindowLong(hwnd, cuecard_font_offset);
#else
    font = (HFONT)GetWindowWord(hwnd, cuecard_font_offset);
#endif

    if (font != 0)
      DeleteObject(font);
    return 0;

  case WM_KEYDOWN:
    SetFocus(global_main_hwnd);
    PostMessageA(GetFocus(), msg, wparam, lparam);
    return 0;

  case WM_PAINT:

#ifdef MODERN_FIXES
    font = (HFONT)GetWindowLong(hwnd, cuecard_font_offset);
#else
    font = (HFONT)GetWindowWord(hwnd, cuecard_font_offset);
#endif

    paintdc = BeginPaint(hwnd, &paint);
    if (paintdc != 0)
    {

      ApplyCardArtPaletteToDc(paintdc);

      GetClientRect(hwnd, &rect_client);

      brush_inner = CreateSolidBrush(PALETTERGB(210, 190, 150));
      brush_outer = CreateSolidBrush(PALETTERGB(127, 127, 127));

      SetRect(&rect_inner,
              rect_client.left,
              rect_client.top,
              rect_client.right - 2,
              rect_client.bottom - 2);
      FillRect(paintdc, &rect_client, brush_outer);
      FillRect(paintdc, &rect_inner, brush_inner);

      SetTextColor(paintdc, PALETTERGB(0, 0, 0));
      SetBkMode(paintdc, TRANSPARENT);
      GetWindowTextA(hwnd, txt, 100);
      SelectObject(paintdc, font);
      DrawTextA(paintdc,
                txt,
                -1,
                &rect_inner,
                DT_SINGLELINE | DT_VCENTER | DT_CENTER);

      EndPaint(hwnd, &paint);

      DeleteObject(brush_inner);
      DeleteObject(brush_outer);
    }
    return 0;

  default:
    return DefWindowProcA(hwnd, msg, wparam, lparam);
  }

  return 0;
}

// FUNCTION: DECKDLL 0x1002764d
LRESULT CALLBACK
wndproc_TitleClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  HDC chdc; // ebp - 0x80
  HDC hdc;  // ebp - 0x7c
  BITMAP bmp;
  PAINTSTRUCT paint;
  RECT rect2; // ebp - 0x20
  RECT hint_rect;
  RECT rect;  // ebp - 0x10
  int saved_dc;
  POINT drag_source_point;
  char hint_text[64];
  int new_drag_sell_price;

  // TitleClass fonts.
  // GLOBAL: DECKDLL 0x10105280
  static LOGFONT title_font_logfont;
  // GLOBAL: DECKDLL 0x10105278
  static HFONT title_font_large;
  // GLOBAL: DECKDLL 0x101052bc
  static HFONT title_font_small;
  static HFONT title_font_hint;
  static int title_drag_sell_price = -1;
  // GLOBAL: DECKDLL 0x100362d8
  static LOGFONT titleClass_fontTemplate = {
      0,        // lfHeight
      0,        // lfWidth
      0,        // lfEscapement
      0,        // lfOrientation
      FW_NORMAL // lfWeight
  };

  switch (msg)
  {
  case WM_CREATE:
    memcpy(&title_font_logfont, &titleClass_fontTemplate, 0x3c);
    strcpy(title_font_logfont.lfFaceName, "Cheltenham ITC Bold BT");
    title_font_logfont.lfHeight = 0x32;
    title_font_large = CreateFontIndirect(&title_font_logfont);
    title_font_logfont.lfHeight = 0x1e;
    title_font_small = CreateFontIndirect(&title_font_logfont);
    title_font_logfont.lfHeight = 0x18;
    title_font_hint = CreateFontIndirect(&title_font_logfont);
    return 0;

  case WM_DESTROY:
    DeleteObject(title_font_large);
    DeleteObject(title_font_small);
    DeleteObject(title_font_hint);
    return 0;

  case WM_ERASEBKGND:
  {
    hdc = (HDC)wparam;
    ApplyCardArtPaletteToDc(hdc);

    chdc = CreateCompatibleDC(hdc);
    ApplyCardArtPaletteToDc(chdc);

    SelectObject(chdc, global_pic_dektit1);

    GetClientRect(hwnd, &rect);

    GetObject(global_pic_dektit1, sizeof(BITMAP), &bmp);

    StretchBlt(hdc, 0, 0, rect.right, rect.bottom, chdc, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);

    DeleteDC(chdc);
    return 1;
  }

  case WM_PAINT:
    hdc = BeginPaint(hwnd, &paint);
    ApplyCardArtPaletteToDc(hdc);
    GetClientRect(hwnd, &rect);
    InflateRect(&rect, -5, -5);
    CopyRect(&rect2, &rect);

    if (strlen(global_deckinfo.deckname) < 12)
      SelectObject(hdc, title_font_large);
    else
      SelectObject(hdc, title_font_small);

    SetBkMode(hdc, TRANSPARENT);
    saved_dc = SaveDC(hdc);
    SetMapMode(hdc, MM_ANISOTROPIC);

    SetWindowExtEx(hdc, rect2.right - rect2.left, 0x46, NULL);
    SetViewportExtEx(hdc, rect2.right - rect2.left, rect2.bottom - rect2.top, NULL);

    rect2.left = rect2.left + 3;
    rect2.top = rect2.top + 3;
    SetTextColor(hdc, PALETTERGB(20, 46, 77));
    DrawTextA(hdc, global_deckinfo.deckname, strlen(global_deckinfo.deckname), &rect2, DT_SINGLELINE | DT_VCENTER | DT_CENTER);

    rect2.left = rect2.left + -3;
    rect2.top = rect2.top + -3;
    SetTextColor(hdc, PALETTERGB(243, 209, 175));
    DrawTextA(hdc, global_deckinfo.deckname, strlen(global_deckinfo.deckname), &rect2, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
    RestoreDC(hdc, saved_dc);

    if (deckbuilder_selling_available())
    {
      CopyRect(&hint_rect, &rect);
      hint_rect.top = hint_rect.bottom - 0x18;
      hint_rect.bottom += 2;
      if (title_drag_sell_price >= 0)
        sprintf(hint_text, "Sell for %d gold", title_drag_sell_price);
      else
        strcpy(hint_text, "Drop card here to sell");

      SelectObject(hdc, title_font_hint);
      SetBkMode(hdc, TRANSPARENT);
      SetTextColor(hdc, PALETTERGB(20, 46, 77));
      OffsetRect(&hint_rect, 2, 2);
      DrawTextA(hdc, hint_text, strlen(hint_text), &hint_rect, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
      OffsetRect(&hint_rect, -2, -2);
      SetTextColor(hdc, PALETTERGB(243, 209, 175));
      DrawTextA(hdc, hint_text, strlen(hint_text), &hint_rect, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
    }

    EndPaint(hwnd, &paint);
    return 0;

  case WM_DECKBUILDER_DRAG_SELL_HOVER:
    new_drag_sell_price = -1;
    if (wparam != 0)
    {
      drag_source_point.x = (short)(lparam & 0xffff);
      drag_source_point.y = (short)((lparam >> 16) & 0xffff);
      new_drag_sell_price = deckbuilder_drag_sell_price((HWND)wparam, drag_source_point);
    }
    if (new_drag_sell_price != title_drag_sell_price)
    {
      title_drag_sell_price = new_drag_sell_price;
      InvalidateRect(hwnd, NULL, TRUE);
    }
    return 0;

  case WM_LBUTTONDOWN:
  case WM_RBUTTONDOWN:
    if (global_db_flags_1 & (DBFLAGS_EDITDECK | DBFLAGS_SHANDALAR))
      return 0;

    show_dialog_deckinfo();
    InvalidateRect(hwnd, NULL, TRUE);
    return 0;

  default:
    return DefWindowProcA(hwnd, msg, wparam, lparam);
  }

  return 0;
}

// FUNCTION: DECKDLL 0x10010f75
LRESULT CALLBACK wndproc_FullCardClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  // GLOBAL: DECKDLL 0x10030010
  static int FULLCARD_CSVID_INDEX = 4;
  // GLOBAL: DECKDLL 0x10030014
  static int FULLCARD_EXPANDTEXT_INDEX = 8;

  int card_to_draw;
  uint expanded_text;
  int wanted_card;
  switch (msg)
  {

  case 0x466:
    wanted_card = wparam;
    card_to_draw = GetWindowLongA(hwnd, FULLCARD_CSVID_INDEX);
    if (wanted_card == card_to_draw)
      InvalidateRect(hwnd, NULL, 0);
    return 0;

  case 0x400:
    card_to_draw = wparam;
    if (GetWindowLongA(hwnd, FULLCARD_CSVID_INDEX) != card_to_draw)
    {
      if (card_to_draw < 0 && global_edited_deck.entries[0].DeckEntry_Amount > 0)
        card_to_draw = global_edited_deck.entries[0].DeckEntry_csvid;
      SetWindowLongA(hwnd, FULLCARD_CSVID_INDEX, card_to_draw);
      InvalidateRect(hwnd, NULL, 0);
    }
    return 0;

  case 0x402:
    expanded_text = (WORD)GetWindowWord(hwnd, FULLCARD_EXPANDTEXT_INDEX);
    return expanded_text;

  case 0x401:
    expanded_text = wparam;
    SetWindowWord(hwnd, FULLCARD_EXPANDTEXT_INDEX, (WORD)expanded_text);
    InvalidateRect(hwnd, NULL, 0);
    CheckMenuItem(global_fullcard_popup, RES_FULLCARDMENU_EXPAND,
                  (expanded_text >= 1) ? MF_CHECKED : 0);
    return expanded_text;

  case WM_COMMAND:
    if ((wparam & 0xffff) == RES_FULLCARDMENU_EXPAND)
    {
      if (!SendMessageA(hwnd, 0x402, 0, 0))
        SendMessageA(hwnd, 0x401, 1, 0);
      else
        SendMessageA(hwnd, 0x401, 0, 0);
    }
    return 0;

  case WM_CREATE:
    global_fullcard_popup = CreatePopupMenu();
    load_text("menus", "FULLCARD");
    AppendMenuA(global_fullcard_popup, 0, RES_FULLCARDMENU_EXPAND, text_lines[0]);
    card_to_draw = -1;
    SetWindowLongA(hwnd, FULLCARD_CSVID_INDEX, card_to_draw);
    expanded_text = global_cfg.expand_text;
    SetWindowWord(hwnd, FULLCARD_EXPANDTEXT_INDEX, (WORD)expanded_text);
    CheckMenuItem(global_fullcard_popup, RES_FULLCARDMENU_EXPAND,
                  (expanded_text >= 1) ? MF_CHECKED : 0);
    return 0;

  case WM_DESTROY:
    global_cfg.expand_text = (char)GetWindowWord(hwnd, FULLCARD_EXPANDTEXT_INDEX);
    DestroyMenu(global_fullcard_popup);
    return 0;

  case WM_PAINT:
  {
    HDC hdc;           // ebp - 0x60
    PAINTSTRUCT paint; // ebp - 0x5c
    RECT rect;         // ebp - 0x1c

    card_to_draw = GetWindowLong(hwnd, FULLCARD_CSVID_INDEX);
    expanded_text = GetWindowWord(hwnd, FULLCARD_EXPANDTEXT_INDEX);

    hdc = BeginPaint(hwnd, &paint);

    GetClientRect(hwnd, &rect);

    ApplyCardArtPaletteToDc(hdc);

    if (card_to_draw < 0)
    {
      DrawCardBack(hdc, &rect);
    }
    else
    {
      uint cardid;
      FillRect(global_hdc, &rect, global_create_brush_0);
      cardid = DrawFullCard(global_hdc, &rect, &global_raw_cards_storage[card_to_draw], 0, 1, expanded_text, global_artistline);
      BitBlt(hdc, 0, 0, rect.right, rect.bottom, global_hdc, 0, 0, SRCCOPY);
    }

    EndPaint(hwnd, &paint);
    return 0;
  }

  case WM_RBUTTONDOWN:
  {
    POINT pt;
    pt.x = (int)(short)LOWORD(lparam);
    pt.y = (int)(short)HIWORD(lparam);
    ClientToScreen(hwnd, &pt);
    TrackPopupMenu(global_fullcard_popup, 2, pt.x, pt.y, 0, hwnd, NULL);
    return 0;
  }

  case WM_LBUTTONDOWN:
  {
    static DWORD last_click = 0;
    DWORD now = GetTickCount();
    if (now - last_click <= GetDoubleClickTime()) {
      SendMessageA(hwnd, WM_COMMAND, RES_FULLCARDMENU_EXPAND, 0);
      last_click = 0;
      return 0;
    }
    last_click = now;
    return DefWindowProcA(hwnd, msg, wparam, lparam);
  }

  default:
    return DefWindowProcA(hwnd, msg, wparam, lparam);
  }

  return 0;
}

// FUNCTION: DECKDLL 0x1002ab15
void filter_cards_in_lists(HWND hwnd_listbox, HWND hwnd_horzlist)
{
  int newsel;           // ebp - 0x1c
  int idx;              // ebp - 0x18
  int pad;              // ebp - 0x14
  int data;             // ebp - 0x10
  int count;            // ebp - 0xc
  const card_ptr_t *cp; // ebp - 0x8
  csvid_t csvid;        // ebp - 0x4

  pad = 0;
  idx = SendMessage(hwnd_listbox, LB_GETCURSEL, 0, 0);
  data = SendMessage(hwnd_listbox, LB_GETITEMDATA, idx, 0);

  SendMessage(hwnd_listbox, LB_RESETCONTENT, 0, 0);
  SendMessage(hwnd_horzlist, LB_RESETCONTENT, 0, 0);

  UpdateWindow(hwnd_listbox);
  UpdateWindow(hwnd_horzlist);

  newsel = -1;
  csvid = 0;
  cp = &global_raw_cards_storage[0];
  for (; csvid < global_available_slots; ++csvid, ++cp)
    if (check_filters(csvid))
    {
      idx = SendMessage(hwnd_listbox, LB_ADDSTRING, 0, (LPARAM)cp->full_name);
      SendMessage(hwnd_listbox, LB_SETITEMDATA, idx, csvid);
      if (csvid == data)
        newsel = idx;
    }

  if (newsel == -1)
    newsel = 0;

  SendMessage(hwnd_listbox, LB_SETCURSEL, newsel, 0);

  SendMessage(GetParent(hwnd_listbox), WM_COMMAND, MAKELONG2(GetDlgCtrlID(hwnd_listbox), 1), (LPARAM)hwnd_listbox);
  UpdateWindow(hwnd_listbox);

  count = SendMessage(hwnd_listbox, LB_GETCOUNT, 0, 0);
  for (idx = 0; idx < count; ++idx)
  {
    csvid = SendMessage(hwnd_listbox, LB_GETITEMDATA, idx, 0);
    SendMessage(hwnd_horzlist, LB_ADDSTRING, 0, (LPARAM)csvid);
  }

  SendMessage(hwnd_horzlist, LB_SETCURSEL, 0, 0);

  SendMessage(GetParent(hwnd_horzlist), WM_COMMAND, MAKELONG2(GetDlgCtrlID(hwnd_horzlist), 1), (LPARAM)hwnd_horzlist);
}

// FUNCTION: DECKDLL 0x1002b8f0
void insert_cards_into_deck(csvid_t csvid, int num, FullDeck *tgt_deck)
{
  int i;     /* ebp - 0x8 */
  int found; /* ebp - 0x4 */

  i = 0;
  found = 0;
  for (; i < tgt_deck->total && found == 0; i++)
  {
    if (tgt_deck->entries[i].DeckEntry_csvid == csvid)
    {
      tgt_deck->entries[i].DeckEntry_Amount += num;
      tgt_deck->cards += num;
      found = 1;
    }
  }

  if (found == 0)
  {
    tgt_deck->entries[tgt_deck->total].DeckEntry_csvid = csvid;
    tgt_deck->entries[tgt_deck->total].DeckEntry_Amount = num;
    tgt_deck->entries[tgt_deck->total].DeckEntry_FullName = global_raw_cards_storage[csvid].full_name;
    tgt_deck->total += 1;
    tgt_deck->cards += num;
  }
}

// FUNCTION: DECKDLL 0x1002b9dc
void add_or_increment_sideboard_bucket_entry(int which, csvid_t csvid, int num, FullDeck *tgt_deck)
{
  DeckBucket *bucket;
  int i;
  int *count_ptr;
  int found;

  if (which == 0)
  {
    bucket = &tgt_deck->sideboard[0];
    count_ptr = &tgt_deck->sideboard[0].total;
  }
  else if (which == 1)
  {
    bucket = &tgt_deck->sideboard[1];
    count_ptr = &tgt_deck->sideboard[1].total;
  }
  else if (which == 2)
  {
    bucket = &tgt_deck->sideboard[2];
    count_ptr = &tgt_deck->sideboard[2].total;
  }
  else if (which == 5)
  {
    bucket = &tgt_deck->sideboard[5];
    count_ptr = &tgt_deck->sideboard[5].total;
  }
  else if (which == 4)
  {
    bucket = &tgt_deck->sideboard[4];
    count_ptr = &tgt_deck->sideboard[4].total;
  }
  else if (which == 3)
  {
    bucket = &tgt_deck->sideboard[3];
    count_ptr = &tgt_deck->sideboard[3].total;
  }
  else
  {

    return;
  }

  i = 0;
  found = 0;
  for (; (i < *count_ptr) && (!found); i++)
    if (bucket->entries[i].DeckEntry_csvid == csvid)
    {
      bucket->entries[i].DeckEntry_Amount += num;
      found = 1;
    }

  if (!found)
  {
    bucket->entries[*count_ptr].DeckEntry_csvid = csvid;
    bucket->entries[*count_ptr].DeckEntry_Amount = num;
    bucket->entries[*count_ptr].DeckEntry_FullName = global_raw_cards_storage[csvid].full_name;
    *count_ptr = *count_ptr + 1;
  }
}

// FUNCTION: DECKDLL 0x1002bb7f
void append_trade_bucket_entry(int which, int csvid, int num, FullDeck *deck_base)
{
  DeckBucket *bucket;
  int *count_ptr;

  if (which == 0)
  {
    bucket = &deck_base->trade[1];
    count_ptr = &deck_base->trade[1].total;
  }
  else if (which == 1)
  {
    bucket = &deck_base->trade[2];
    count_ptr = &deck_base->trade[2].total;
  }
  else if (which == 2)
  {
    bucket = &deck_base->trade[3];
    count_ptr = &deck_base->trade[3].total;
  }
  else if (which == 5)
  {
    bucket = &deck_base->trade[6];
    count_ptr = &deck_base->trade[6].total;
  }
  else if (which == 4)
  {
    bucket = &deck_base->trade[5];
    count_ptr = &deck_base->trade[5].total;
  }
  else if (which == 3)
  {
    bucket = &deck_base->trade[4];
    count_ptr = &deck_base->trade[4].total;
  }

  else if (which == 6)
  {
    bucket = &deck_base->trade[0];
    count_ptr = &deck_base->trade[0].total;
  }
  else
  {
    return;
  }

  bucket->entries[*count_ptr].DeckEntry_csvid = csvid;
  bucket->entries[*count_ptr].DeckEntry_Amount = num;
  bucket->entries[*count_ptr].DeckEntry_FullName = global_raw_cards_storage[csvid].full_name;
  *count_ptr = *count_ptr + 1;
}

// FUNCTION: DECKDLL 0x1002bcde
static void remove_cards_from_deck(csvid_t csvid, int num, FullDeck *tgt_deck)
{
  int j;     /* ebp - 0xc */
  int i;     /* ebp - 0x8 */
  int found; /* ebp - 0x4 */

  i = 0;
  found = 0;
  for (; i < tgt_deck->total && found == 0; i++)
  {
    if (tgt_deck->entries[i].DeckEntry_csvid == csvid)
    {
      found = 1;
      if (tgt_deck->entries[i].DeckEntry_Amount == num)
      {
        tgt_deck->cards -= num;
        for (j = i; j < tgt_deck->total - 1; j++)
        {
          tgt_deck->entries[j].DeckEntry_csvid = tgt_deck->entries[j + 1].DeckEntry_csvid;
          tgt_deck->entries[j].DeckEntry_Amount = tgt_deck->entries[j + 1].DeckEntry_Amount;
          tgt_deck->entries[j].DeckEntry_FullName = tgt_deck->entries[j + 1].DeckEntry_FullName;
        }
        --tgt_deck->total;
      }
      else if (num < tgt_deck->entries[i].DeckEntry_Amount)
      {
        tgt_deck->entries[i].DeckEntry_Amount -= num;
        tgt_deck->cards -= num;
      }
      else
      {
        tgt_deck->cards -= num;
        num -= tgt_deck->entries[i].DeckEntry_Amount;
        found = 0;
        for (j = i; j < tgt_deck->total - 1; j++)
        {
          tgt_deck->entries[j].DeckEntry_csvid = tgt_deck->entries[j + 1].DeckEntry_csvid;
          tgt_deck->entries[j].DeckEntry_Amount = tgt_deck->entries[j + 1].DeckEntry_Amount;
          tgt_deck->entries[j].DeckEntry_FullName = tgt_deck->entries[j + 1].DeckEntry_FullName;
        }
        --tgt_deck->total;
      }
    }
  }
}

// FUNCTION: DECKDLL 0x1002bed0
static void remove_or_decrement_sideboard_bucket_entry(int which, int csvid, int num, FullDeck *deck_base)
{
  DeckBucket *bucket; /* ebp - 0x14 */
  int idx;            /* ebp - 0x10 */
  int i;              /* ebp - 0xc */
  int *count_ptr;     /* ebp - 0x8 */
  int found;          /* ebp - 0x4 */

  if (which == 0)
  {
    bucket = &deck_base->sideboard[0];
    count_ptr = &deck_base->sideboard[0].total;
  }
  else if (which == 1)
  {
    bucket = &deck_base->sideboard[1];
    count_ptr = &deck_base->sideboard[1].total;
  }
  else if (which == 2)
  {
    bucket = &deck_base->sideboard[2];
    count_ptr = &deck_base->sideboard[2].total;
  }
  else if (which == 5)
  {
    bucket = &deck_base->sideboard[5];
    count_ptr = &deck_base->sideboard[5].total;
  }
  else if (which == 4)
  {
    bucket = &deck_base->sideboard[4];
    count_ptr = &deck_base->sideboard[4].total;
  }
  else if (which == 3)
  {
    bucket = &deck_base->sideboard[3];
    count_ptr = &deck_base->sideboard[3].total;
  }
  else
  {
    return;
  }

  i = 0;
  found = 0;
  for (; i < *count_ptr && !found; i++)
  {
    if (bucket->entries[i].DeckEntry_csvid == csvid)
    {
      found = 1;

      if (bucket->entries[i].DeckEntry_Amount == num)
      {
        for (idx = i; idx < *count_ptr - 1; idx++)
        {
          bucket->entries[idx].DeckEntry_csvid = bucket->entries[idx + 1].DeckEntry_csvid;
          bucket->entries[idx].DeckEntry_Amount = bucket->entries[idx + 1].DeckEntry_Amount;
          bucket->entries[idx].DeckEntry_FullName = bucket->entries[idx + 1].DeckEntry_FullName;
        }
        *count_ptr = *count_ptr - 1;
      }
      else if (num < bucket->entries[i].DeckEntry_Amount)
      {
        bucket->entries[i].DeckEntry_Amount -= num;
      }
      else
      {
        num -= bucket->entries[i].DeckEntry_Amount;
        found = 0;

        for (idx = i; idx < *count_ptr - 1; idx++)
        {
          bucket->entries[idx].DeckEntry_csvid = bucket->entries[idx + 1].DeckEntry_csvid;
          bucket->entries[idx].DeckEntry_Amount = bucket->entries[idx + 1].DeckEntry_Amount;
          bucket->entries[idx].DeckEntry_FullName = bucket->entries[idx + 1].DeckEntry_FullName;
        }
        *count_ptr = *count_ptr - 1;
      }
    }
  }
}

// FUNCTION: DECKDLL 0x1002c161
static void remove_trade_bucket_entries(int which, int csvid, int num, FullDeck *deck_base)
{
  DeckBucket *bucket; /* ebp - 0x14 */
  int idx;            /* ebp - 0x10 */
  int i;              /* ebp - 0xc */
  int *count_ptr;     /* ebp - 0x8 */
  int done;           /* ebp - 0x4 */

  if (which == 0)
  {
    bucket = &deck_base->trade[1];
    count_ptr = &deck_base->trade[1].total;
  }
  else if (which == 1)
  {
    bucket = &deck_base->trade[2];
    count_ptr = &deck_base->trade[2].total;
  }
  else if (which == 2)
  {
    bucket = &deck_base->trade[3];
    count_ptr = &deck_base->trade[3].total;
  }
  else if (which == 5)
  {
    bucket = &deck_base->trade[6];
    count_ptr = &deck_base->trade[6].total;
  }
  else if (which == 4)
  {
    bucket = &deck_base->trade[5];
    count_ptr = &deck_base->trade[5].total;
  }
  else if (which == 3)
  {
    bucket = &deck_base->trade[4];
    count_ptr = &deck_base->trade[4].total;
  }
  else if (which == 6)
  {
    bucket = &deck_base->trade[0];
    count_ptr = &deck_base->trade[0].total;
  }
  else
  {
    return;
  }

  i = 0;
  done = 0;
  for (; i < *count_ptr && !done; i++)
  {
    if (bucket->entries[i].DeckEntry_csvid == csvid)
    {
      for (idx = i; idx < *count_ptr - 1; idx++)
      {
        bucket->entries[idx].DeckEntry_csvid = bucket->entries[idx + 1].DeckEntry_csvid;
        bucket->entries[idx].DeckEntry_Amount = bucket->entries[idx + 1].DeckEntry_Amount;
        bucket->entries[idx].DeckEntry_FullName = bucket->entries[idx + 1].DeckEntry_FullName;
      }
      *count_ptr = *count_ptr - 1;

      num = num + -1;
      if (num <= 0)
        done = 1;
    }
  }
}

// FUNCTION: DECKDLL 0x100092c5
static int delete_card_from_global_deck(csvid_t csvid, int num)
{
  int idx;   // ebp - 0xc
  int i;     // ebp - 0x8
  int found; // ebp - 0x4

  i = 0;
  found = 0;
  for (; i < global_deck_num_entries && found == 0; i++)
    if (global_deck[i].GDE_csvid == csvid && global_deck[i].GDE_Available == num)
    {
      idx = i;
      found = 1;
      if (num == 0)
        remove_cards_from_deck(csvid, 1, &global_edited_deck);
    }

  if (!found)
    return 0;

  for (i = idx + 1; i < global_deck_num_entries; i++)
  {
    global_deck[i - 1] = global_deck[i];
  }
  global_deck_num_entries = global_deck_num_entries + -1;
  return 1;
}

// FUNCTION: DECKDLL 0x1002c6b5
void move_colors_fromto_global_deck(int move_into, unsigned int dlgbits)
{
  int i;
  if (move_into == 1)
  {
    for (i = 0; i < global_deck_num_entries; ++i)
    {
      if (global_deck[i].GDE_Available == 1)
      {
        if ((((dlgbits & 1) != 0 && global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_BLACK) ||
             ((dlgbits & 2) != 0 && global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_BLUE) ||
             ((dlgbits & 4) != 0 && global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_GREEN) ||
             ((dlgbits & 8) != 0 && global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_RED) ||
             ((dlgbits & 0x10) != 0 && global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_WHITE) ||
             ((dlgbits & 0x20) != 0 && global_raw_cards_storage[global_deck[i].GDE_csvid].card_type == CP_TYPE_ARTIFACT)))
        {
          global_deck[i].GDE_Available = 0;
          global_deck[i].GDE_DecksBits |= 1 << global_current_deck;
          insert_cards_into_deck(global_deck[i].GDE_csvid, 1, &global_edited_deck);
        }
        else if ((((dlgbits & 1) != 0 && global_deck[i].GDE_iid == 0) ||   // swamp
                  ((dlgbits & 2) != 0 && global_deck[i].GDE_iid == 1) ||   // island
                  ((dlgbits & 4) != 0 && global_deck[i].GDE_iid == 2) ||   // forest
                  ((dlgbits & 8) != 0 && global_deck[i].GDE_iid == 3) ||   // mountain
                  ((dlgbits & 0x10) != 0 && global_deck[i].GDE_iid == 4))) // plains
        {
          global_deck[i].GDE_Available = 0;
          global_deck[i].GDE_DecksBits |= 1 << global_current_deck;
          insert_cards_into_deck(global_deck[i].GDE_csvid, 1, &global_edited_deck);
        }
      }
    }
  }
  else
  {
    for (i = 0; i < global_deck_num_entries; ++i)
    {
      if (global_deck[i].GDE_Available == 0)
      {
        if ((((dlgbits & 1) != 0 && global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_BLACK) ||
             ((dlgbits & 2) != 0 && global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_BLUE) ||
             ((dlgbits & 4) != 0 && global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_GREEN) ||
             ((dlgbits & 8) != 0 && global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_RED) ||
             ((dlgbits & 0x10) != 0 && global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_WHITE) ||
             ((dlgbits & 0x20) != 0 && global_raw_cards_storage[global_deck[i].GDE_csvid].card_type == CP_TYPE_ARTIFACT)))
        {
          global_deck[i].GDE_Available = 1;
          global_deck[i].GDE_DecksBits &= ~(1 << global_current_deck);
          remove_cards_from_deck(global_deck[i].GDE_csvid, 1, &global_edited_deck);
        }
        else
        {
          if ((((dlgbits & 1) != 0 && global_deck[i].GDE_iid == 0) ||   // swamp
               ((dlgbits & 2) != 0 && global_deck[i].GDE_iid == 1) ||   // island
               ((dlgbits & 4) != 0 && global_deck[i].GDE_iid == 2) ||   // forest
               ((dlgbits & 8) != 0 && global_deck[i].GDE_iid == 3) ||   // mountain
               ((dlgbits & 0x10) != 0 && global_deck[i].GDE_iid == 4))) // plains
          {
            global_deck[i].GDE_Available = 1;
            global_deck[i].GDE_DecksBits &= ~(1 << global_current_deck);
            remove_cards_from_deck(global_deck[i].GDE_csvid, 1, &global_edited_deck);
          }
        }
      }
    }
  }
}

// FUNCTION: DECKDLL 0x1002cb92
void set_smallcard_dimensions(void)
{
  if (global_cfg_consolidate)
  {
    if (global_edited_deck_num_entries < 75)
    {
      global_smallcard_width = global_smallcard_normal_width;
      global_smallcard_height = global_smallcard_normal_height;
    }
    else if (global_edited_deck_num_entries < 132)
    {
      global_smallcard_width = global_smallcard_smaller_width;
      global_smallcard_height = global_smallcard_smaller_height;
    }
    else
    {
      global_smallcard_width = global_smallcard_smallest_width;
      global_smallcard_height = global_smallcard_smallest_height;
    }
  }
  else
  {
    if (global_deck_num_cards < 75)
    {
      global_smallcard_width = global_smallcard_normal_width;
      global_smallcard_height = global_smallcard_normal_height;
    }
    else if (global_deck_num_cards < 132)
    {
      global_smallcard_width = global_smallcard_smaller_width;
      global_smallcard_height = global_smallcard_smaller_height;
    }
    else
    {
      global_smallcard_width = global_smallcard_smallest_width;
      global_smallcard_height = global_smallcard_smallest_height;
    }
  }
}

// FUNCTION: DECKDLL 0x1002c349
int check_colors_inout_edited_deck(bool currently_in)
{
  int i;
  int dlgbits;

  dlgbits = 0;
  if (currently_in == 1)
  {
    for (i = 0; i < global_deck_num_entries; ++i)
    {
      if (global_deck[i].GDE_Available == 0)
      {
        if (global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_BLACK)
          dlgbits |= 1;
        if (global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_BLUE)
          dlgbits |= 2;
        if (global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_GREEN)
          dlgbits |= 4;
        if (global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_RED)
          dlgbits |= 8;
        if (global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_WHITE)
          dlgbits |= 0x10;
        if (global_raw_cards_storage[global_deck[i].GDE_csvid].card_type == CP_TYPE_ARTIFACT)
          dlgbits |= 0x20;
        if (global_deck[i].GDE_iid == 0)
          dlgbits |= 1;
        else if (global_deck[i].GDE_iid == 1)
          dlgbits |= 2;
        else if (global_deck[i].GDE_iid == 2)
          dlgbits |= 4;
        else if (global_deck[i].GDE_iid == 3)
          dlgbits |= 8;
        else if (global_deck[i].GDE_iid == 4)
          dlgbits |= 0x10;
      }
    }
  }
  else
  {
    for (i = 0; i < global_deck_num_entries; ++i)
    {
      if (global_deck[i].GDE_Available == 1)
      {
        if (global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_BLACK)
          dlgbits |= 1;
        if (global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_BLUE)
          dlgbits |= 2;
        if (global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_GREEN)
          dlgbits |= 4;
        if (global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_RED)
          dlgbits |= 8;
        if (global_raw_cards_storage[global_deck[i].GDE_csvid].color == CP_COLOR_WHITE)
          dlgbits |= 0x10;
        if (global_raw_cards_storage[global_deck[i].GDE_csvid].card_type == CP_TYPE_ARTIFACT)
          dlgbits |= 0x20;
        if (global_deck[i].GDE_iid == 0)
          dlgbits |= 1;
        else if (global_deck[i].GDE_iid == 1)
          dlgbits |= 2;
        else if (global_deck[i].GDE_iid == 2)
          dlgbits |= 4;
        else if (global_deck[i].GDE_iid == 3)
          dlgbits |= 8;
        else if (global_deck[i].GDE_iid == 4)
          dlgbits |= 0x10;
      }
    }
  }

  return dlgbits;
}

// FUNCTION: DECKDLL 0x1002a9b5
static int
count_card_amount_outside_edited_deck(csvid_t csvid)
{
  int i;
  int count = 0;

  for (i = 0; i < global_deck_num_entries; ++i)
    if (global_deck[i].GDE_csvid == csvid && !(global_deck[i].GDE_DecksBits & (1 << (unsigned char)global_current_deck)))
      ++count;

  return count;
}

// FUNCTION: DECKDLL 0x100093cd
static iid_t
get_iid_from_global_deck_card(csvid_t csvid)
{
  int i;
  for (i = 0; i < global_deck_num_entries; ++i)
    if (global_deck[i].GDE_csvid == csvid)
      return global_deck[i].GDE_iid;
  return -1;
}

// FUNCTION: DECKDLL 0x10009187
static bool
change_global_deck_card_availability(csvid_t csvid, int mode)
{
  int old_avail; /* ebp - 0xc */
  int new_avail; /* ebp - 0x8 */
  int i;         /* ebp - 0x4 */
  if (mode == 0)
  {
    old_avail = 0;
    new_avail = 2;
  }
  else if (mode == 1)
  {
    old_avail = 1;
    new_avail = 2;
  }
  else if (mode == 2)
  {
    old_avail = 2;
    new_avail = 0;
  }
  else if (mode == 3)
  {
    old_avail = 2;
    new_avail = 1;
  }
  else
    return false;

  for (i = 0; global_deck_num_entries > i; ++i)
  {
    if (global_deck[i].GDE_csvid == csvid && global_deck[i].GDE_Available == old_avail)
    {
      global_deck[i].GDE_Available = new_avail;

      if (new_avail == 0)
        global_deck[i].GDE_DecksBits |= 1 << global_current_deck;
      if (new_avail == 1)
        global_deck[i].GDE_DecksBits &= ~(1 << global_current_deck);

      return true;
    }
  }

  return false;
}

// FUNCTION: DECKDLL 0x1002ad25
static void
TENTATIVE_remove_selected_from_horzlist(HWND hwnd_listbox, HWND hwnd_horzlist)
{
  int cursel;
  int n;
  int count;
  int idk;

  cursel = SendMessage(hwnd_listbox, LB_GETCURSEL, 0, 0);
  count = SendMessage(hwnd_listbox, LB_GETCOUNT, 0, 0);

  for (n = 0; n < count;)
    if (check_filters(idk = SendMessage(hwnd_listbox, LB_GETITEMDATA, n, 0)))
      ++n;
    else
    {
      SendMessage(hwnd_listbox, LB_DELETESTRING, n, 0);
      SendMessage(hwnd_horzlist, LB_DELETESTRING, n, 0);
      --count;

      if (n < cursel)
        --cursel;
      else if (n == cursel)
        cursel = 0;
    }

  if (count = SendMessage(hwnd_listbox, LB_GETCOUNT, 0, 0))
  {
    SendMessage(hwnd_listbox, LB_SETCURSEL, cursel, 0);
    SendMessage(hwnd_horzlist, LB_SETCURSEL, cursel, 0);
    SendMessage(GetParent(hwnd_listbox), WM_COMMAND, MAKELONG2(GetDlgCtrlID(hwnd_listbox), 1), (LPARAM)hwnd_listbox);
  }
}

// FUNCTION: DECKDLL 0x1002aa2b
int horzlist_prep_rectangle(HWND hwnd, int idx, RECT *rect)
{
  RECT r;
  WORD w;
  WORD lft;
  WORD pics;

  w = GetWindowWord(hwnd, HORZLIST_WIDTHPLUSSPACE_INDEX);
  lft = GetWindowWord(hwnd, HORZLIST_LEFTPIC_INDEX);
  pics = GetWindowWord(hwnd, HORZLIST_PICSINLINE_INDEX);
  GetClientRect(hwnd, &r);

  SetRect(rect, (idx - lft) * w + r.left, r.top,
          ((idx - lft) + 1) * w + r.left, r.bottom);

  if (idx >= lft && (lft + pics) >= idx)
    return 1;
  else
    return 0;
}

// FUNCTION: DECKDLL 0x1002ae78
static void
TENTATIVE_scroll(HWND hwnd_listbox, HWND hwnd_horzlist)
{
  LRESULT tmp;   /* ebp - 0x1f58 */
  WPARAM sel;    /* ebp - 0x1f54 */
  LRESULT count; /* ebp - 0x1f50 */
  LRESULT data;  /* ebp - 0x1f4c */
  WPARAM i;      /* ebp - 0x1f48 */
  int done;      /* ebp - 0x1f44 */
  int present[2000];

  sel = SendMessageA(hwnd_listbox, 0x188, 0, 0);
  data = SendMessageA(hwnd_listbox, 0x199, sel, 0);
  count = SendMessageA(hwnd_listbox, 0x18b, 0, 0);

  memset(present, 0, 8000);
  for (i = 0; (int)i < count; i = i + 1)
  {
    tmp = SendMessageA(hwnd_listbox, 0x199, i, 0);
    present[tmp] = 1;
  }

  for (i = 0; (int)i < global_available_slots; i = i + 1)
  {
    tmp = global_raw_cards_storage[i].id;
    if (present[tmp] == 0)
    {
      if (check_filters(tmp))
      {
        sel = SendMessageA(hwnd_listbox, 0x180, 0, (LPARAM)global_raw_cards_storage[i].full_name);
        SendMessageA(hwnd_listbox, 0x19a, sel, tmp);
        SendMessageA(hwnd_horzlist, 0x181, sel, tmp);
      }
    }
  }

  count = SendMessageA(hwnd_listbox, 0x18b, 0, 0);
  sel = 0;
  i = 0;
  done = 0;
  for (; (int)i < count && done == 0; i++)
  {
    if (SendMessageA(hwnd_listbox, 0x199, i, 0) == (LRESULT)data)
    {
      sel = i;
      done = 1;
    }
  }

  SendMessageA(hwnd_listbox, 0x186, sel, 0);
  SendMessageA(hwnd_horzlist, 0x186, sel, 0);
  SendMessageA(GetParent(hwnd_listbox), 0x111, (GetDlgCtrlID(hwnd_listbox) & 0xffff) | 0x10000,
               (LPARAM)hwnd_listbox);
}

// FUNCTION: DECKDLL 0x1002b69d
static int
ask_movexcards(csvid_t csvid, bool shifted)
{
  int move_count; /* ebp - 0x8 */
  int i;          /* ebp - 0x4 */

  move_count = 0;

  if (shifted == 1)
  {
    if ((global_db_flags_1 & 0x61) != 0)
    {
      move_count = count_card_amount_outside_edited_deck(csvid);
      global_dlg_parameter = move_count;
      if (show_dialog_movexcards() == 0)
        global_dlg_result = 0;
      if (move_count > global_dlg_result)
        move_count = global_dlg_result;

      for (i = 0; move_count > i; i++)
      {
        change_global_deck_card_availability(csvid, 1);
      }

      TENTATIVE_remove_selected_from_horzlist(global_listbox_hwnd, global_horzlist_hwnd);
    }
    else
    {
      if (show_dialog_movexcards() == 0)
        move_count = 0;
      move_count = global_dlg_result;
    }
  }
  else
  {
    move_count = 1;
    if ((global_db_flags_1 & 0x61) != 0)
      if (change_global_deck_card_availability(csvid, 1) != 0)
        TENTATIVE_remove_selected_from_horzlist(global_listbox_hwnd, global_horzlist_hwnd);
  }

  return move_count;
}

// FUNCTION: DECKDLL 0x10019c42
void refresh_numofcards_text(void)
{
  char buf[80];

  load_text("menus", "STATS");
  sprintf(buf, text_lines[0], global_deck_num_cards);
  SetWindowText(global_button_stats_hwnd, buf);
  InvalidateRect(global_button_stats_hwnd, NULL, FALSE);
}

static int deckbuilder_selling_available(void)
{
  return ((global_db_flags_1 & DBFLAGS_SHANDALAR) != 0) &&
         ((global_db_flags_2 & DBFLAGS_STANDALONE) != 0) &&
         SellPrice != 0 && Gold != 0;
}

static int deckbuilder_sell_price_for_csvid(csvid_t csvid)
{
  iid_t iid;

  if (!deckbuilder_selling_available())
    return -1;
  iid = get_iid_from_global_deck_card(csvid);
  if (iid == -1)
    return -1;
  return SellPrice(iid);
}

static int deckbuilder_drag_sell_price(HWND source, POINT source_point)
{
  char class_name[64];
  csvid_t csvid;
  int item_index;
  int item_count;
  int left_index;
  int width_plus_space;
  RECT source_rect;

  if (!source || !deckbuilder_selling_available())
    return -1;

  class_name[0] = '\0';
  GetClassNameA(source, class_name, sizeof(class_name));
  if (strcasecmp(class_name, "MAGICDECK_CardClass") == 0)
  {
    if (GetParent(source) != global_decksurface_hwnd)
      return -1;
    csvid = (csvid_t)GetWindowLongA(source, global_cardclass_csvid_offset);
    return deckbuilder_sell_price_for_csvid(csvid);
  }

  if (strcasecmp(class_name, "MAGICDECK_HorzListClass") == 0)
  {
    if (source != global_horzlist_hwnd)
      return -1;
    GetClientRect(source, &source_rect);
    if (!PtInRect(&source_rect, source_point))
      return -1;
    width_plus_space = (int)GetWindowWord(source, HORZLIST_WIDTHPLUSSPACE_INDEX);
    if (width_plus_space <= 0)
      return -1;
    left_index = (int)GetWindowWord(source, HORZLIST_LEFTPIC_INDEX);
    item_index = left_index + source_point.x / width_plus_space;
    item_count = (int)GetWindowWord(source, HORZLIST_LASTPIC_INDEX);
    if (item_index < 0 || item_index >= item_count)
      return -1;
    csvid = (csvid_t)SendMessageA(source, LB_GETITEMDATA, item_index, 0);
    return deckbuilder_sell_price_for_csvid(csvid);
  }

  return -1;
}

static void refresh_gold_title_text(void)
{
  char buf[32];

  load_text("menus", "PRICE");
  sprintf(buf, text_lines[0], *Gold);
  strncpy(global_deckinfo.deckname, buf, 12);
  InvalidateRect(global_title_hwnd, NULL, TRUE);
}

static int sell_card_from_available_pool(csvid_t csvid)
{
  iid_t iid;
  int sell_price;

  if (!deckbuilder_selling_available())
    return 0;
  iid = get_iid_from_global_deck_card(csvid);
  if (iid == -1)
    return 0;
  sell_price = SellPrice(iid);
  if (!delete_card_from_global_deck(csvid, 1))
    return 0;

  *Gold += sell_price;
  refresh_gold_title_text();
  TENTATIVE_remove_selected_from_horzlist(global_listbox_hwnd, global_horzlist_hwnd);
  InvalidateRect(global_horzlist_hwnd, 0, 0);
  count_packs();
  if (global_cfg.effects != 0)
    play_sound(3, 400, 0, 0);
  return 1;
}

static int sell_temp_removed_deck_cards(csvid_t csvid, int amount)
{
  iid_t iid;
  int sell_price;
  int sold;
  int i;

  if (!deckbuilder_selling_available() || amount <= 0)
    return 0;
  iid = get_iid_from_global_deck_card(csvid);
  if (iid == -1)
    return 0;
  sell_price = SellPrice(iid);
  sold = 0;
  for (i = 0; i < amount; i++)
  {
    if (delete_card_from_global_deck(csvid, 2))
      sold++;
  }
  if (sold == 0)
    return 0;

  *Gold += sell_price * sold;
  global_deck_was_edited = 1;
  refresh_gold_title_text();
  count_packs();
  refresh_numofcards_text();
  if (global_cfg.effects != 0)
    play_sound(3, 400, 0, 0);
  return 1;
}

// FUNCTION: DECKDLL 0x1002b126
static bool handle_card_list_click_or_drag(HWND hwnd, int singleclick, int shifted)
{
  WPARAM wParam;    /* ebp - 0x30 */
  int amt;          /* ebp - 0x2c */
  int i;            /* ebp - 0x28 */
  int xoff;         /* ebp - 0x24 */
  RECT r;           /* ebp - 0x20 */
  HANDLE itemdata;  /* ebp - 0x10 */
  HWND card_hwnd;   /* ebp - 0xc */
  HWND target_hwnd; /* ebp - 0x8 */
  int yoff;         /* ebp - 0x4 */

  xoff = 0;
  yoff = 0;

  wParam = SendMessageA(hwnd, 0x188, 0, 0);
  itemdata = (HANDLE)SendMessageA(hwnd, 0x199, wParam, 0);

  if ((((itemdata == (HANDLE)0x38e) || (itemdata == (HANDLE)0x37a)) || (itemdata == (HANDLE)0x375)) ||
      (itemdata == (HANDLE)0x384) || (itemdata == (HANDLE)0x37b))
  {
    return 0;
  }

  horzlist_prep_rectangle(global_horzlist_hwnd, wParam, &r);
  if (PtInRect(&r, global_horzlist_mouse_pt) == 0)
    return 0;

  MapWindowPoints(global_horzlist_hwnd, global_main_hwnd, (LPPOINT)&r, 2);

  if (singleclick == 1)
  {
    card_hwnd = CreateWindowExA(0, "MAGICDECK_CardClass", "Card", 0x54000000,
                                r.left, r.top, global_smallcard_width, global_smallcard_height,
                                global_main_hwnd, (HMENU)1, global_hinstance, (LPVOID)itemdata);
    if (card_hwnd == (HWND)0)
      return 0;

    BringWindowToTop(card_hwnd);
    UpdateWindow(card_hwnd);

    xoff = global_main_mouse_pt.x - r.left;
    yoff = global_main_mouse_pt.y - r.top;

    SendMessageA(card_hwnd, 0x112, 0xf012, 0);
    DestroyWindow(card_hwnd);

    GetCursorPos(&global_main_mouse_pt);

    target_hwnd = WindowFromPoint(global_main_mouse_pt);
    while (target_hwnd != (HWND)0 &&
           target_hwnd != global_main_hwnd &&
           target_hwnd != global_decksurface_hwnd &&
           target_hwnd != global_title_hwnd &&
           target_hwnd != global_unknown_10144b04 &&
           target_hwnd != global_unknown_10125a3c &&
           target_hwnd != global_horzlist_hwnd)
    {
      target_hwnd = GetParent(target_hwnd);
    }
  }
  else
  {
    target_hwnd = global_decksurface_hwnd;
  }

  if (target_hwnd == global_title_hwnd && !deckbuilder_selling_available())
    target_hwnd = global_main_hwnd;

  if (target_hwnd == global_decksurface_hwnd)
  {
    amt = ask_movexcards((int)itemdata, shifted);
    set_smallcard_dimensions();
    if (global_cfg.effects != 0)
      play_sound(2, 400, 0, 0);

    global_deck_was_edited = 1;
    ScreenToClient(target_hwnd, &global_main_mouse_pt);
    global_main_mouse_pt.x = global_main_mouse_pt.x - xoff;
    global_main_mouse_pt.y = global_main_mouse_pt.y - yoff;

    for (i = 0; i < amt; i = i + 1)
    {
      if (SendMessageA(target_hwnd, 0x4c8, (WPARAM)itemdata,
                       MAKELONG(global_main_mouse_pt.x, global_main_mouse_pt.y)) == 0)
        MessageBeep(0);
      change_global_deck_card_availability((int)itemdata, 2);
    }

    count_packs();
    if ((global_db_flags_1 & 0x20) != 0)
    {
      SendMessageA(global_horzlist_hwnd, 0x186, 0, 0);
      SendMessageA(global_listbox_hwnd, 0x186, 0, 0);
    }
    else
    {
      SendMessageA(global_fullcard_hwnd, 0x400, (WPARAM)itemdata, 0);
      SendMessageA(global_horzlist_hwnd, 0x186, wParam, 0);
      SendMessageA(global_listbox_hwnd, 0x186, wParam, 0);
    }

    if ((global_db_flags_1 & 0x41) != 0)
      SendMessageA(global_horzlist_hwnd, 0x466, (WPARAM)itemdata, 0);
  }
  else if (target_hwnd == global_title_hwnd)
  {
    if (!sell_card_from_available_pool((csvid_t)itemdata))
      MessageBeep(0);
  }
  else if ((target_hwnd == global_unknown_10125a3c) || (target_hwnd == global_unknown_10144b04))
  {
    amt = ask_movexcards((int)itemdata, shifted);
    if (global_cfg.effects != 0)
      play_sound(2, 400, 0, 0);

    global_deck_was_edited = 1;
    ScreenToClient(target_hwnd, &global_main_mouse_pt);
    for (i = 0; i < amt; i = i + 1)
    {
      if (SendMessageA(target_hwnd, 0x4c8, (WPARAM)itemdata,
                       MAKELONG(global_main_mouse_pt.x, global_main_mouse_pt.y)) == 0)
        MessageBeep(0);
    }

    if ((global_db_flags_1 & 0x20) != 0)
    {
      SendMessageA(global_horzlist_hwnd, 0x186, 0, 0);
      SendMessageA(global_listbox_hwnd, 0x186, 0, 0);
    }
    else
    {
      SendMessageA(global_fullcard_hwnd, 0x400, (WPARAM)itemdata, 0);
      SendMessageA(global_horzlist_hwnd, 0x186, wParam, 0);
      SendMessageA(global_listbox_hwnd, 0x186, wParam, 0);
    }
  }
  else
  {
    TENTATIVE_scroll(global_listbox_hwnd, global_horzlist_hwnd);
    SendMessageA(global_fullcard_hwnd, 0x400, (WPARAM)itemdata, 0);
    SendMessageA(global_horzlist_hwnd, 0x186, wParam, 0);
    SendMessageA(global_listbox_hwnd, 0x186, wParam, 0);
    if ((global_db_flags_1 & 0x41) != 0)
      SendMessageA(global_horzlist_hwnd, 0x466, (WPARAM)itemdata, 0);
  }

  refresh_numofcards_text();
  return 1;
}

// FUNCTION: DECKDLL 0x1002887e
LRESULT CALLBACK wndproc_HorzListClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  // GLOBAL: DECKDLL 0x10105740
  static HDC horzlist_background_hdc;
  // GLOBAL: DECKDLL 0x101052d0
  static LOGFONT horzlist_font_logfont;
  // GLOBAL: DECKDLL 0x1010530c
  static HFONT horzlist_font;

  // GLOBAL: DECKDLL 0x1003a7cc
  static int sellprice = 0;
  // GLOBAL: DECKDLL 0x10105310
  static csvid_t curr_csvid;
  // GLOBAL: DECKDLL 0x1003a790
  static LOGFONT horzListClass_fontTemplate = {
      0,        // lfHeight
      0,        // lfWidth
      0,        // lfEscapement
      0,        // lfOrientation
      FW_NORMAL // lfWeight
  };

  HRGN hrgn;         /* ebp - 0x22c */
  HGDIOBJ h;         /* ebp - 0x228 */
  RECT r_small;      /* ebp - 0x224 */
  int paint_amt;     /* ebp - 0x214 */
  HDC hdc;           /* ebp - 0x210 */
  BITMAP bmp;        /* ebp - 0x20c */
  PAINTSTRUCT paint; /* ebp - 0x1f4 */

  int paint_idx; /* ebp - 0x1b4 */
  int save;      /* ebp - 0x1b0 */

  RECT paint_r;    /* ebp - 0x1ac */
  int paint_csvid; /* ebp - 0x19c */

  POINT mm_pt;    /* ebp - 0x198 */
  int mm_i;       /* ebp - 0x190 */
  int mm_hit_idx; /* ebp - 0x18c */
  RECT mm_r;      /* ebp - 0x188 */
  int mm_hit;     /* ebp - 0x178 */

  int scrollpos;    /* ebp - 0x174 */
  int minpos;       /* ebp - 0x170 */
  int page_amt;     /* ebp - 0x16c */
  WORD scroll_code; /* ebp - 0x168 */
  int maxpos;       /* ebp - 0x164 */
  uint wLast_int;   /* ebp - 0x160 */
  int tgt;          /* ebp - 0x15c */

  HDC hdc_local; /* ebp - 0x158 */
  RECT create_r; /* ebp - 0x154 */
  int pad_144;   /* ebp - 0x144 */

  DELETEITEMSTRUCT del2; /* ebp - 0x140 */
  int last_pic_index;    /* ebp - 0x12c */
  int left_pic_index;    /* ebp - 0x128 */
  int pad_124;           /* ebp - 0x124 */

  RECT r6;              /* ebp - 0x120 */
  int scrollMinPos;     /* ebp - 0x110 */
  int scrollMaxPos;     /* ebp - 0x10c */
  DELETEITEMSTRUCT del; /* ebp - 0x108 */
  int oldidx;           /* ebp - 0xf4 */
  int getInx;           /* ebp - 0xf0 */
  int insert_index;     /* ebp - 0xec */
  int insert_loop_idx;  /* ebp - 0xe8 */
  RECT r5;              /* ebp - 0xe4 */
  int item_idx;         /* ebp - 0xd4 */
  RECT r4;              /* ebp - 0xd0 */
  UINT lparam_width;    /* ebp - 0xc0 */
  RECT r7;              /* ebp - 0xb0 */

  WORD wOldSel; /* ebp - 0xac */
  RECT r2;      /* ebp - 0xa8 */

  unsigned int wanted_card; /* ebp - 0x98 */
  int idx;                  /* ebp - 0x94 */
  RECT r3;                  /* ebp - 0x90 */
  WORD wPics;               /* ebp - 0x80 */
  int *horz_list_addr;      /* ebp - 0x7c */

  MSG m;        /* ebp - 0x78 */
  int amt;      /* ebp - 0x5c */
  int currsel;  /* ebp - 0x58 */
  POINT p;      /* ebp - 0x54 */
  int j;        /* ebp - 0x4c */
  WORD wWidth;  /* ebp - 0x48 */
  iid_t iid;    /* ebp - 0x44 */
  char buf[32]; /* ebp - 0x40 */

  int i;           /* ebp - 0x20 */
  WORD wNewSel[2]; /* ebp - 0x1c */
  WORD wLeft;      /* ebp - 0x18 */
  RECT r;          /* ebp - 0x14 */
  WORD wLast;      /* ebp - 4 */

  switch (msg)
  {
  case 0x464:
    SendMessageA(global_main_hwnd, 0x466, (WPARAM)wparam, (LPARAM)hwnd);
    return 0;

  case 0x466:
  {

    wanted_card = (unsigned int)wparam;
    wLast = GetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX);
    horz_list_addr = (int *)GetWindowLongA(hwnd, HORZLIST_ADDR_INDEX);
    wLeft = GetWindowWord(hwnd, HORZLIST_LEFTPIC_INDEX);
    wPics = GetWindowWord(hwnd, HORZLIST_PICSINLINE_INDEX);

    for (idx = wLeft; MIN(wLast, wPics + wLeft) > idx; idx++)
    {
      if ((unsigned int)horz_list_addr[idx] == wanted_card)
      {
        horzlist_prep_rectangle(hwnd, (int)idx, &r3);
        InvalidateRect(hwnd, &r3, 0);
      }
    }
    return 0;
  }

  case LB_GETCURSEL:
    return wNewSel[0] = GetWindowWord(hwnd, HORZLIST_CURRSEL_INDEX);

  case LB_SETCURSEL:
  {
    wLast = GetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX);
    horz_list_addr = (int *)GetWindowLongA(hwnd, HORZLIST_ADDR_INDEX);
    wPics = GetWindowWord(hwnd, HORZLIST_PICSINLINE_INDEX);
    wOldSel = GetWindowWord(hwnd, HORZLIST_CURRSEL_INDEX);
    wLeft = GetWindowWord(hwnd, HORZLIST_LEFTPIC_INDEX);

    wNewSel[0] = (WORD)wparam;
    if (wNewSel[0] >= 0 && wNewSel[0] < wLast)
    {
      SetWindowWord(hwnd, HORZLIST_CURRSEL_INDEX, wNewSel[0]);

      if (wOldSel >= 0 && wOldSel < wLast)
      {
        horzlist_prep_rectangle(hwnd, wOldSel, &r2);
        // InvalidateRect(hwnd, &r2, 0);
      }
      horzlist_prep_rectangle(hwnd, wNewSel[0], &r2);
      // InvalidateRect(hwnd, &r2, 0);
      UpdateWindow(hwnd);

      if (wNewSel[0] < wLeft)
      {
        SendMessageA(hwnd, WM_HSCROLL, MAKELONG(SB_THUMBPOSITION, wNewSel[0]), 0);
      }

      if ((wPics + wLeft - 1) < wNewSel[0])
      {
        SendMessageA(hwnd, WM_HSCROLL, MAKELONG3(SB_THUMBPOSITION, wNewSel[0] - wPics + 1), 0);
      }
      return 0;
    }
    else
      return -1;
  }

  case LB_SETITEMHEIGHT:
  {
    lparam_width = (UINT)lparam & 0xffff;

    wLast = GetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX);

    wWidth = (WORD)(lparam_width + 8);

    SetWindowWord(hwnd, HORZLIST_WIDTHPLUSSPACE_INDEX, wWidth);

    GetClientRect(hwnd, &r7);

    wPics = (r7.right - 4) / wWidth;
    if (((r7.right - 4) % wWidth) >= (wWidth >> 2))
      ++wPics;

    SetWindowWord(hwnd, HORZLIST_PICSINLINE_INDEX, wPics);

    SetScrollRange(hwnd, SB_HORZ, 0, wLast - wPics + 1, 0);
    InvalidateRect(hwnd, 0, 1);
    return 0;
  }

  case LB_ADDSTRING:
  {
    wLast = GetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX);
    horz_list_addr = (int *)GetWindowLongA(hwnd, HORZLIST_ADDR_INDEX);
    wLast = GetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX);
    wPics = GetWindowWord(hwnd, HORZLIST_PICSINLINE_INDEX);

    item_idx = wLast;
    horz_list_addr = (int *)realloc(horz_list_addr, (wLast + 1) * 4);
    if (horz_list_addr)
    {

      horz_list_addr[wLast] = (int)lparam;
      SetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX, ++wLast);
      SetWindowLongA(hwnd, HORZLIST_ADDR_INDEX, (LONG)horz_list_addr);

      SetScrollRange(hwnd, SB_HORZ, 0, wLast - wPics + 1, 0);
      horzlist_prep_rectangle(hwnd, item_idx, &r4);
      InvalidateRect(hwnd, &r4, 1);

      return item_idx;
    }
    else
      return -2;
  }

  case LB_INSERTSTRING:
  {
    wLast = GetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX);
    horz_list_addr = (int *)GetWindowLongA(hwnd, HORZLIST_ADDR_INDEX);
    wPics = GetWindowWord(hwnd, HORZLIST_PICSINLINE_INDEX);

    insert_index = (int)wparam;
    if (insert_index == -1)
    {
      return SendMessageA(hwnd, LB_ADDSTRING, 0, lparam);
    }

    if (insert_index >= (int)(unsigned int)wLast)
    {
      return SendMessageA(hwnd, LB_ADDSTRING, 0, lparam);
    }

    if (insert_index < -1 || insert_index >= wLast)
    {
      return -1;
    }

    horz_list_addr = (int *)realloc(horz_list_addr, (wLast + 1) * 4);
    if (horz_list_addr)
    {

      for (insert_loop_idx = wLast; insert_index < insert_loop_idx; insert_loop_idx--)
      {
        horz_list_addr[insert_loop_idx] = horz_list_addr[insert_loop_idx - 1];
      }
      horz_list_addr[insert_index] = (int)lparam;

      SetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX, ++wLast);
      SetWindowLongA(hwnd, HORZLIST_ADDR_INDEX, (LONG)horz_list_addr);
      SetScrollRange(hwnd, SB_HORZ, 0, wLast - wPics + 1, 0);

      horzlist_prep_rectangle(hwnd, insert_index, &r5);
      r5.right = 2000;
      InvalidateRect(hwnd, &r5, 1);

      return insert_index;
    }
    else
      return -2;
  }

  case LB_GETITEMDATA:
  {
    wLast = GetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX);
    horz_list_addr = (int *)GetWindowLongA(hwnd, HORZLIST_ADDR_INDEX);
    getInx = wparam;
    if (getInx >= 0 && getInx < wLast)
      return horz_list_addr[getInx];
    else
      return -1;
  }

  case LB_RESETCONTENT:
  {
    wLast = GetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX);
    horz_list_addr = (int *)GetWindowLongA(hwnd, HORZLIST_ADDR_INDEX);

    oldidx = 0;
    for (; wLast > oldidx; oldidx++)
    {
      del.CtlType = ODT_LISTBOX;
      del.CtlID = GetDlgCtrlID(hwnd);
      del.itemID = oldidx;
      del.hwndItem = hwnd;
      del.itemData = (unsigned int)horz_list_addr[oldidx];

      SendMessageA(GetParent(hwnd), WM_DELETEITEM, (WPARAM)GetDlgCtrlID(hwnd), (LPARAM)&del);
    }

    horz_list_addr = (int *)realloc(horz_list_addr, 4);

    wLast = 0;
    wNewSel[0] = (WORD)-1;
    wLeft = 0;

    SetWindowLongA(hwnd, HORZLIST_ADDR_INDEX, (LONG)horz_list_addr);
    SetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX, wLast);
    SetWindowWord(hwnd, HORZLIST_CURRSEL_INDEX, wNewSel[0]);
    SetWindowWord(hwnd, HORZLIST_LEFTPIC_INDEX, wLeft);

    SetScrollRange(hwnd, SB_HORZ, 0, -1, 0);
    SetScrollPos(hwnd, SB_HORZ, wNewSel[0], 1);
    InvalidateRect(hwnd, 0, 1);

    return 0;
  }

  case LB_GETTOPINDEX:
    wLeft = GetWindowWord(hwnd, HORZLIST_LEFTPIC_INDEX);
    return (LRESULT)(wLeft);

  case LB_SETTOPINDEX:
    wLeft = (WORD)wparam;
    wLast = GetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX);

    if (wLeft >= 0 && wLast > wLeft)
    {
      SetWindowWord(hwnd, HORZLIST_LEFTPIC_INDEX, wLeft);
      SendMessageA(hwnd, 0x114, (WPARAM)MAKELONG(SB_THUMBPOSITION, wLeft), 0);
      return 0;
    }
    else
      return -1;

  case LB_DELETESTRING:
  {
    wLast = GetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX);
    // if ((int)(unsigned int)wparam >= last_pic_index)
    //   return -1;

    horz_list_addr = (int *)GetWindowLongA(hwnd, HORZLIST_ADDR_INDEX);
    wNewSel[0] = (int)(short)GetWindowWord(hwnd, HORZLIST_CURRSEL_INDEX);
    wLeft = (int)(short)GetWindowWord(hwnd, HORZLIST_LEFTPIC_INDEX);
    last_pic_index = (int)wparam;
    if (last_pic_index >= 0 && last_pic_index < wLast)
    {
      del2.CtlType = ODT_LISTBOX;
      del2.CtlID = GetDlgCtrlID(hwnd);
      del2.itemID = last_pic_index;
      del2.hwndItem = hwnd;
      del2.itemData = (unsigned int)horz_list_addr[last_pic_index];

      SendMessageA(GetParent(hwnd), WM_DELETEITEM, (WPARAM)GetDlgCtrlID(hwnd), (LPARAM)&del2);

      for (left_pic_index = last_pic_index; left_pic_index < wLast - 1; ++left_pic_index)
        horz_list_addr[left_pic_index] = horz_list_addr[left_pic_index + 1];

      SetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX, --wLast);

      if (wLast - 1 < wNewSel[0])
      {
        SetWindowWord(hwnd, HORZLIST_CURRSEL_INDEX, --wNewSel[0]);
      }

      if (wLeft > wLast - 1)
      {
        SetWindowWord(hwnd, HORZLIST_LEFTPIC_INDEX, --wLeft);
      }

      // wPics = GetWindowWord(hwnd, HORZLIST_PICSINLINE_INDEX);
      // wLast = GetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX);
      GetScrollRange(hwnd, 0, &scrollMinPos, &scrollMaxPos);
      SetScrollRange(hwnd, 0, scrollMinPos, scrollMaxPos - 1, 1);

      horzlist_prep_rectangle(hwnd, last_pic_index, &r6);
      r6.right = GetWindowWord(hwnd, HORZLIST_WIDTHPLUSSPACE_INDEX) * (wLast + 1);
      r6.left = MAX(r6.left, 0);
      InvalidateRect(hwnd, &r6, 1);

      if (!wLast)
      {
        wNewSel[0] = -1;
        wLeft = 0;
        SetWindowWord(hwnd, HORZLIST_CURRSEL_INDEX, wNewSel[0]);
        SetWindowWord(hwnd, HORZLIST_LEFTPIC_INDEX, wLeft);
      }

      return wLast;
    }
    else
      return -1;
  }

  case WM_CHAR:
    if (wparam == 27)
    {
      SendMessageA(global_main_hwnd, 0x10, 0, 0);
      return 0;
    }

    SendMessageA(global_listbox_hwnd, msg, wparam, lparam);
    pad_144 = (int)SendMessageA(global_listbox_hwnd, LB_GETCURSEL, 0, 0);
    SendMessageA(hwnd, WM_HSCROLL, MAKELONG(SB_THUMBPOSITION, pad_144), 0);
    SendMessageA(hwnd, LB_SETCURSEL, pad_144, 0);
    SendMessageA(GetParent(hwnd), WM_COMMAND, (GetDlgCtrlID(hwnd) & 0xffff) | 0x10000, (LPARAM)hwnd);
    return 0;

  case WM_COMMAND:
  {
    switch ((unsigned int)wparam & 0xffff)
    {
    case 1:
      *(WORD *)&amt = (WORD)count_card_amount_outside_edited_deck(curr_csvid);

      global_dlg_parameter = 1;
      global_dlg_result = 1;

      if ((amt & 0xffff) > 1)
        if (show_dialog_sellxcards() == 0)
          return 0;

      if (!global_dlg_result)
        return 0;
      else if ((amt & 0xffff) < global_dlg_result)
        global_dlg_result = amt & 0xffff;

      for (i = 0, j = 0; j < global_dlg_result && !i; j++)
        if (!delete_card_from_global_deck(curr_csvid, 1))
          i = 1;

      *Gold += global_dlg_result * sellprice;

      load_text("menus", "PRICE");
      sprintf(buf, text_lines[0], *Gold);
      strncpy(global_deckinfo.deckname, buf, 12);

      InvalidateRect(global_title_hwnd, NULL, 1);
      TENTATIVE_remove_selected_from_horzlist(global_listbox_hwnd, global_horzlist_hwnd);
      InvalidateRect(global_horzlist_hwnd, 0, 0);
      break;
    case 2:
      global_dlg_parameter = check_colors_inout_edited_deck(1);
      show_dialog_groupmove();
      move_colors_fromto_global_deck(1, global_dlg_result);
      TENTATIVE_remove_selected_from_horzlist(global_listbox_hwnd, global_horzlist_hwnd);
      InvalidateRect(global_horzlist_hwnd, 0, 1);

      SendMessage(global_decksurface_hwnd, 0x401, 0, 0);
      InvalidateRect(global_decksurface_hwnd, 0, 1);
      break;
    case 3:
      global_dlg_parameter = check_colors_inout_edited_deck(0);
      show_dialog_groupmove();
      move_colors_fromto_global_deck(0, global_dlg_result);
      TENTATIVE_remove_selected_from_horzlist(global_listbox_hwnd, global_horzlist_hwnd);
      InvalidateRect(global_horzlist_hwnd, 0, 1);

      SendMessage(global_decksurface_hwnd, 0x401, 0, 0);
      InvalidateRect(global_decksurface_hwnd, 0, 1);
      break;
    }
    return 0;
  }

  case WM_CREATE:
  {
    GetClientRect(hwnd, &create_r);

    horz_list_addr = (int *)malloc(4);

    wLast = 0;
    wNewSel[0] = (WORD)-1;
    wLeft = 0;

    wWidth = (WORD)global_smallcard_piclist_width;
    if ((int)(unsigned int)wWidth > 0)
      wPics = (WORD)((create_r.right - 4) / wWidth);
    else
      wPics = 0;

    SetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX, wLast);
    SetWindowLongA(hwnd, HORZLIST_ADDR_INDEX, (LONG)horz_list_addr);
    SetWindowWord(hwnd, HORZLIST_CURRSEL_INDEX, wNewSel[0]);
    SetWindowWord(hwnd, HORZLIST_LEFTPIC_INDEX, wLeft);

    SetWindowWord(hwnd, HORZLIST_WIDTHPLUSSPACE_INDEX, wWidth);
    SetWindowWord(hwnd, HORZLIST_PICSINLINE_INDEX, wPics);

    SetScrollRange(hwnd, SB_HORZ, 0, wLast - wPics + 1, 0);
    SetScrollPos(hwnd, SB_HORZ, wNewSel[0], 1);

    global_horzlist_popup = CreatePopupMenu();

    hdc_local = GetDC(hwnd);
    ApplyCardArtPaletteToDc(hdc_local);
    horzlist_background_hdc = CreateCompatibleDC(hdc_local);
    ApplyCardArtPaletteToDc(horzlist_background_hdc);
    ReleaseDC(hwnd, hdc_local);

    if (!horzlist_font)
    {
      memcpy(&horzlist_font_logfont, &horzListClass_fontTemplate, sizeof(LOGFONT));
      horzlist_font_logfont.lfHeight = 0x2a;
      horzlist_font_logfont.lfWeight = 0x1f4;
      strcpy(horzlist_font_logfont.lfFaceName, "Kudos Condensed SSi");
      horzlist_font = CreateFontIndirectA(&horzlist_font_logfont);
    }

    return 0;
  }

  case WM_DESTROY:
  {
    horz_list_addr = (int *)GetWindowLongA(hwnd, HORZLIST_ADDR_INDEX);
    free(horz_list_addr);

    DestroyMenu(global_horzlist_popup);
    DeleteDC(horzlist_background_hdc);
    if (horzlist_font)
      DeleteObject(horzlist_font);
    horzlist_font = 0;
    return 0;
  }

  case WM_HSCROLL:
  {
    wWidth = GetWindowWord(hwnd, HORZLIST_WIDTHPLUSSPACE_INDEX);
    wPics = GetWindowWord(hwnd, HORZLIST_PICSINLINE_INDEX);

    scrollpos = GetScrollPos(hwnd, SB_HORZ);
    GetScrollRange(hwnd, SB_HORZ, &minpos, &maxpos);

    page_amt = wPics;
    wLast_int = GetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX);
    scroll_code = (WORD)wparam;

    if (scroll_code == SB_TOP)
      tgt = minpos;
    else if (scroll_code == SB_LINEUP)
      tgt = scrollpos - 1;
    else if (scroll_code == SB_PAGEUP)
      tgt = scrollpos - page_amt;
    else if (scroll_code == SB_THUMBPOSITION)
      tgt = GET_Y_LPARAM(wparam);
    else if (scroll_code == SB_PAGEDOWN)
      tgt = page_amt + scrollpos;
    else if (scroll_code == SB_LINEDOWN)
      tgt = scrollpos + 1;
    else if (scroll_code == SB_BOTTOM)
      tgt = maxpos;
    else
      tgt = scrollpos;

    if (tgt < minpos)
      tgt = minpos;
    if (tgt > maxpos)
      tgt = maxpos;

    if (scrollpos != tgt)
    {
      wLeft = (WORD)tgt;
      SetWindowWord(hwnd, HORZLIST_LEFTPIC_INDEX, wLeft);
      UpdateWindow(hwnd);

      ScrollWindow(hwnd, (scrollpos - tgt) * wWidth, 0, 0, 0);
      SetScrollPos(hwnd, SB_HORZ, tgt, 1);
      InvalidateRect(hwnd, NULL, 0);
    }
    return 0;
  }

  case WM_KEYDOWN:
    if (wparam == VK_UP || wparam == VK_LEFT)
    {
      SendMessageA(hwnd, WM_HSCROLL, SB_LINELEFT, 0);
    }
    else if (wparam == VK_DOWN || wparam == VK_RIGHT)
    {
      SendMessageA(hwnd, WM_HSCROLL, SB_LINERIGHT, 0);
    }
    else if (wparam == VK_PRIOR)
    {
      SendMessageA(hwnd, WM_HSCROLL, SB_PAGELEFT, 0);
    }
    else if (wparam == VK_NEXT)
    {
      SendMessageA(hwnd, WM_HSCROLL, SB_PAGERIGHT, 0);
    }
    else if (wparam == VK_HOME)
    {
      SendMessageA(hwnd, WM_HSCROLL, SB_LEFT, 0);
    }
    else if (wparam == VK_END)
    {
      SendMessageA(hwnd, WM_HSCROLL, SB_RIGHT, 0);
    }
    return 0;

  case WM_LBUTTONDBLCLK:
  {
    if ((global_db_flags_2 & 1) == 0)
      return 0;

    GetCursorPos(&p);
    GetCursorPos((LPPOINT)&global_horzlist_mouse_pt);
    ScreenToClient(global_main_hwnd, &p);
    ScreenToClient(global_horzlist_hwnd, (LPPOINT)&global_horzlist_mouse_pt);

    SetFocus(hwnd);
    wLast = GetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX);
    if (wLast > 0)
    {
      SendMessageA(hwnd, WM_MOUSEMOVE, wparam & ~MK_LBUTTON, lparam);

      if ((wparam & MK_SHIFT))
      {
        if (!handle_card_list_click_or_drag(hwnd, 0, 1))
        {
          MessageBeep(0);
          return 0;
        }
      }
      else if (!handle_card_list_click_or_drag(hwnd, 0, 0))
      {
        MessageBeep(0);
        return 0;
      }
    }

    return 0;
  }

  case WM_LBUTTONDOWN:
  {
    if ((global_db_flags_2 & 1) == 0)
      return 0;

    GetCursorPos(&p);
    GetCursorPos((LPPOINT)&global_horzlist_mouse_pt);
    ScreenToClient(global_main_hwnd, &p);
    ScreenToClient(global_horzlist_hwnd, (LPPOINT)&global_horzlist_mouse_pt);

#ifndef SHANDALAR_SDL_WIN32_COMPAT
    j = GetDoubleClickTime();
    Sleep(j);
    if (PeekMessageA(&m, hwnd, WM_LBUTTONDBLCLK, WM_LBUTTONDBLCLK, 0))
      return 0;
#endif

    SetFocus(hwnd);
    wLast = GetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX);
    if (wLast > 0)
    {
      SendMessageA(hwnd, WM_MOUSEMOVE, wparam & ~MK_LBUTTON, lparam);

      if ((wparam & MK_SHIFT))
      {
        if (!handle_card_list_click_or_drag(hwnd, 1, 1))
        {
          MessageBeep(0);
          return 0;
        }
      }
      else if (!handle_card_list_click_or_drag(hwnd, 1, 0))
      {
        MessageBeep(0);
        return 0;
      }
    }
    return 0;
  }

  case WM_MOUSEMOVE:
  {
    if (abs(global_horzlist_last_mousemove_pt.y - GET_Y_LPARAM(lparam)) +
            abs(global_horzlist_last_mousemove_pt.x - GET_X_LPARAM(lparam)) <
        2)
      return 0;

    global_horzlist_last_mousemove_pt.x = GET_X_LPARAM(lparam);
    global_horzlist_last_mousemove_pt.y = GET_Y_LPARAM(lparam);

    wLast = GetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX);
    wLeft = GetWindowWord(hwnd, HORZLIST_LEFTPIC_INDEX);
    wWidth = GetWindowWord(hwnd, HORZLIST_WIDTHPLUSSPACE_INDEX);
    wPics = GetWindowWord(hwnd, HORZLIST_PICSINLINE_INDEX);

    if (wLast > 0)
    {
      mm_pt.x = GET_X_LPARAM(lparam);
      mm_pt.y = GET_Y_LPARAM(lparam);

      mm_i = wLeft;
      mm_hit = 0;
      for (; wPics + wLeft >= mm_i; mm_i++)
      {
        if (mm_hit == 0)
        {
          horzlist_prep_rectangle(hwnd, mm_i, &mm_r);
          if (PtInRect(&mm_r, mm_pt))
          {
            mm_hit = 1;
            mm_hit_idx = mm_i;
          }
        }
      }

      if (mm_hit != 0)
      {
        if (SendMessageA(hwnd, LB_GETCURSEL, 0, 0) != mm_hit_idx)
        {
          SendMessageA(hwnd, LB_SETCURSEL, (WPARAM)mm_hit_idx, 0);
          SendMessageA(GetParent(hwnd), WM_COMMAND, (GetDlgCtrlID(hwnd) & 0xffff) | 0x10000, (LPARAM)hwnd);
        }
      }
    }

    return 0;
  }

  case WM_PAINT:
  {
    wLast = GetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX);
    horz_list_addr = (int *)GetWindowLongA(hwnd, HORZLIST_ADDR_INDEX);
    wNewSel[0] = GetWindowWord(hwnd, HORZLIST_CURRSEL_INDEX);
    wLeft = GetWindowWord(hwnd, HORZLIST_LEFTPIC_INDEX);
    wWidth = GetWindowWord(hwnd, HORZLIST_WIDTHPLUSSPACE_INDEX);
    wPics = GetWindowWord(hwnd, HORZLIST_PICSINLINE_INDEX);

    hdc = BeginPaint(hwnd, &paint);
    ApplyCardArtPaletteToDc(hdc);

    hrgn = CreateRectRgn(0, 0, global_smallcard_piclist_width, global_smallcard_piclist_height);

    GetClientRect(hwnd, &paint_r);
    GetObjectA(global_pic_dekbar1, sizeof(BITMAP), &bmp);

    h = SelectObject(horzlist_background_hdc, global_pic_dekbar1);
    StretchBlt(hdc, 0, 0, paint_r.right, paint_r.bottom, horzlist_background_hdc, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
    SelectObject(horzlist_background_hdc, h);

    for (paint_idx = wLeft;
         MIN(wLast, wPics + wLeft) > paint_idx;
         paint_idx++)
    {
      horzlist_prep_rectangle(hwnd, (int)paint_idx, &paint_r);
      paint_csvid = horz_list_addr[paint_idx];

      SetRect(&r_small, 0, 0, global_smallcard_piclist_width, global_smallcard_piclist_height);

      SetRectRgn(hrgn, r_small.left, r_small.top, r_small.right, r_small.bottom);
      SelectClipRgn(global_hdc, hrgn);

      paint_amt = count_card_amount_outside_edited_deck((csvid_t)paint_csvid);
      DrawSmallCard(global_hdc, &r_small, &global_raw_cards_storage[paint_csvid], 1, 1);

      if ((global_db_flags_1 & 0x41) != 0 && (1 < paint_amt))
      {
        sprintf(buf, "%d", paint_amt);

        save = SaveDC(global_hdc);
        SetMapMode(global_hdc, 8);
        SetWindowExtEx(global_hdc, 200, 0x118, 0);
        SetViewportExtEx(global_hdc, r_small.right - r_small.left, r_small.bottom - r_small.top, 0);
        SetViewportOrgEx(global_hdc, r_small.left, r_small.top, 0);

        SetBkMode(global_hdc, 1);
        SelectObject(global_hdc, horzlist_font);
        SetTextAlign(global_hdc, 0);

        SetTextColor(global_hdc, 0x010000C9);

        TextOutA(global_hdc, 0xc8 - (int)(strlen(buf) * 0x10) + 3, 3, buf, (int)strlen(buf));

        SetTextColor(global_hdc, 0x02FEFEFE);
        SetBkMode(global_hdc, 1);

        TextOutA(global_hdc, 0xc8 - (int)(strlen(buf) * 0x10), 0, buf, (int)strlen(buf));

        RestoreDC(global_hdc, save);
      }

      SelectClipRgn(global_hdc, 0);

      paint_r.left += (paint_r.right - paint_r.left - global_smallcard_piclist_width) / 2;
      paint_r.top += (paint_r.bottom - paint_r.top - global_smallcard_piclist_height) / 2;

      BitBlt(hdc, paint_r.left, paint_r.top, global_smallcard_piclist_width, global_smallcard_piclist_height,
             global_hdc, 0, 0, SRCCOPY);
    }

    EndPaint(hwnd, &paint);
    DeleteObject(hrgn);
    return 0;
  }

  case WM_RBUTTONDOWN:
  {
    if ((global_db_flags_2 & 2) == 0)
      return 0;

    SetFocus(hwnd);
    wLast = GetWindowWord(hwnd, HORZLIST_LASTPIC_INDEX);
    if (wLast > 0)
    {
      SendMessageA(hwnd, WM_MOUSEMOVE, wparam & ~MK_RBUTTON, lparam);

      p.x = GET_X_LPARAM(lparam);
      p.y = GET_Y_LPARAM(lparam);

      currsel = (int)SendMessageA(hwnd, LB_GETCURSEL, 0, 0);
      horzlist_prep_rectangle(global_horzlist_hwnd, currsel, &r);
      if (!PtInRect(&r, p))
        return 0;

      ScreenToClient(global_main_hwnd, &p);

      curr_csvid = (csvid_t)SendMessageA(hwnd, LB_GETITEMDATA, currsel, 0);
      iid = get_iid_from_global_deck_card(curr_csvid);
      if (iid == -1)
        return 0;

      sellprice = SellPrice(iid);

      load_text("menus", "SELLCARD");
      sprintf(buf, text_lines[0], sellprice);

      AppendMenuA(global_horzlist_popup, 0, 1, buf);

      ClientToScreen(hwnd, &p);
      TrackPopupMenu(global_horzlist_popup, 2, p.x + 10, p.y + 10, 0, hwnd, 0);
      DeleteMenu(global_horzlist_popup, 1, 0);
    }
    return 0;
  }

  default:
    return DefWindowProcA(hwnd, msg, wparam, lparam);
  }

  return 0;
}

// FUNCTION: DECKDLL 0x100057bc
void reset_dim_and_pos_all_cards(HWND hwnd, RECT *rect)
{
  HWND last;                  /* ebp - 0x18 */
  int offset_between_cards_y; /* ebp - 0x14 */
  HWND w;                     /* ebp - 0x10 */
  int offset_from_sides;      /* ebp - 0xc */
  int y;                      /* ebp - 8 */
  int x;                      /* ebp - 4 */

  offset_between_cards_y = (global_smallcard_height * 18) / 100;

  offset_from_sides = global_smallcard_height / 10;

  x = rect->left + offset_from_sides;
  y = rect->top + offset_from_sides;

  last = 0;
  for (w = GetTopWindow(hwnd); w != 0; w = GetWindow(w, GW_HWNDNEXT))
    last = w;

  for (w = last; w != 0; w = GetWindow(w, GW_HWNDPREV))
  {
    if (rect->bottom < y + global_smallcard_height)
    {
      x += global_smallcard_width + 8;
      y = rect->top + offset_from_sides;
    }

    SetWindowPos(w, 0, x, y, global_smallcard_width, global_smallcard_height, 4);
    y += offset_between_cards_y;
  }
}

// FUNCTION: DECKDLL 0x1001a58e
int ask_about_saving_deck(void)
{
  int rval;
  char txt[128];

  load_text("menus", "SAVE");
  sprintf(txt, text_lines[0], global_deck_filename);
  load_text("menus", "DECKBUILDER");
  rval = MessageBox(global_main_hwnd, txt, text_lines[0], MB_ICONQUESTION | MB_YESNOCANCEL);
  if (rval == IDYES)
    rval = SendMessage(global_main_hwnd, WM_COMMAND, RES_MAINMENU_SAVEDECK, 0);
  return rval;
}

// FUNCTION: DECKDLL 0x10008abf
static void
TENTATIVE_move_from_owned_cards(HWND hwnd, POINT point, int shift2_unshift1, int singleclick)
{
  WORD amt32;    /* ebp - 0x38 */
  POINT pt;      /* ebp - 0x34 */
  int ctrl_id;   /* ebp - 0x2c */
  int i;         /* ebp - 0x28 */
  HWND new_hwnd; /* ebp - 0x24 */
  int num;       /* ebp - 0x20 (low word) */
  HWND parent;   /* ebp - 0x1c */
  RECT r;        /* ebp - 0x18 */
  csvid_t csvid; /* ebp - 0x8 */
  HWND target;   /* ebp - 0x4 */

  new_hwnd = NULL;

  csvid = (csvid_t)GetWindowLongA(hwnd, global_cardclass_csvid_offset);
  amt32 = GetWindowWord(hwnd, global_cardclass_amount_offset);
  parent = GetParent(hwnd);
  ctrl_id = GetDlgCtrlID(hwnd);
  GetWindowRect(hwnd, &r);

  if (amt32 >= 2)
  {
    pt.x = r.left;
    pt.y = r.top;
    SendMessageA(hwnd, 0x401, 1, 0);
    MapWindowPoints(NULL, parent, &pt, 1);

    new_hwnd = CreateWindowExA(0, "MAGICDECK_CardClass", "", 0x54000000, pt.x, pt.y, global_smallcard_width,
                               global_smallcard_height, parent, (HMENU)1, global_hinstance, (LPVOID)csvid);
    SendMessageA(new_hwnd, 0x401, amt32 - 1, 0);
    BringWindowToTop(new_hwnd);
  }

  if (singleclick == 1)
  {
    LockWindowUpdate(global_main_hwnd);
    MapWindowPoints(NULL, global_main_hwnd, (LPPOINT)&r, 2);
    MoveWindow(hwnd, r.left, r.top, 0, 0, 1);
    SetParent(hwnd, global_main_hwnd);
    MoveWindow(hwnd, r.left, r.top, r.right - r.left, r.bottom - r.top, 1);
    BringWindowToTop(hwnd);
    LockWindowUpdate(NULL);
    SendMessageA(hwnd, 0x112, 0xF012, 0);
    DestroyWindow(hwnd);

    GetCursorPos(&pt);
    target = WindowFromPoint(pt);
    while (
        target != NULL &&
        target != global_main_hwnd &&
        target != global_decksurface_hwnd &&
        target != global_title_hwnd &&
        target != global_unknown_10144b04 &&
        target != global_unknown_10125a3c &&
        target != global_horzlist_hwnd)
    {
      target = GetParent(target);
    }
  }
  else
  {
    target = global_horzlist_hwnd;
    DestroyWindow(hwnd);
    SetFocus(global_horzlist_hwnd);
  }

  if (target == global_title_hwnd && !deckbuilder_selling_available())
    target = global_main_hwnd;

  *(WORD *)&num = 1;
  if (global_decksurface_hwnd == parent)
  {
    if (shift2_unshift1 > 1)
    {
      global_dlg_parameter = amt32;
      if (!show_dialog_movexcards())
        global_dlg_result = 0;

      if (global_dlg_result <= 0)
      {
        if (new_hwnd)
          SendMessageA(new_hwnd, 0x401, amt32, 0);
        *(WORD *)&num = 0;
      }
      else if (global_dlg_result >= amt32)
      {
        if (new_hwnd)
          DestroyWindow(new_hwnd);
        *(WORD *)&num = amt32;
      }
      else
      {
        if (new_hwnd)
          SendMessageA(new_hwnd, 0x401, amt32 - global_dlg_result, 0);
        *(WORD *)&num = (WORD)global_dlg_result;
      }
    }
    else
    {
      if (new_hwnd)
        SendMessageA(new_hwnd, 0x401, amt32 - 1, 0);
      *(WORD *)&num = 1;
    }

    for (i = 0; i < (int)(num & 0xFFFF); i++)
      change_global_deck_card_availability(csvid, 0);

    remove_cards_from_deck(csvid, num & 0xFFFF, &global_edited_deck);
    count_packs();
    refresh_numofcards_text();
  }
  else if (global_unknown_10144b04 == parent)
  {
    remove_or_decrement_sideboard_bucket_entry(ctrl_id, (int)csvid, num & 0xffff, &global_edited_deck);
  }
  else
  {
    remove_trade_bucket_entries(ctrl_id, (int)csvid, num & 0xffff, &global_edited_deck);
  }

  if (target == global_decksurface_hwnd)
  {
    if (target != parent)
      global_deck_was_edited = 1;

    ScreenToClient(target, &pt);
    pt.x = pt.x - point.x;
    pt.y = pt.y - point.y;

    for (i = 0; i < (int)(num & 0xFFFF); i++)
    {
      if (!SendMessageA(target, 0x4C8, (WPARAM)csvid, MAKELONG(pt.x, pt.y)))
        MessageBeep(0);

      change_global_deck_card_availability(csvid, 2);
    }
    count_packs();
  }
  else if (target == global_title_hwnd)
  {
    if (parent != global_decksurface_hwnd || !sell_temp_removed_deck_cards(csvid, num & 0xffff))
      MessageBeep(0);
  }
  else if (target == global_unknown_10125a3c || target == global_unknown_10144b04)
  {
    if (target != parent)
      global_deck_was_edited = 1;

    ScreenToClient(target, &pt);
    if (target == global_unknown_10125a3c)
      *(short *)&num = 0;

    for (i = 0; i < (int)(num & 0xFFFF); i++)
      if (!SendMessageA(target, 0x4C8, (WPARAM)csvid, MAKELONG(pt.x, pt.y)))
        MessageBeep(0);
  }
  else if (target == global_horzlist_hwnd)
  {
    set_smallcard_dimensions();
    if (target != parent)
    {
      global_deck_was_edited = 1;
      if (global_cfg.effects != 0)
        play_sound(3, 400, 0, 0);
    }

    for (i = 0; i < (int)(num & 0xFFFF); i++)
      if (change_global_deck_card_availability(csvid, 3))
        TENTATIVE_scroll(global_listbox_hwnd, global_horzlist_hwnd);

    if ((global_db_flags_1 & 0x41) != 0)
      SendMessageA(global_horzlist_hwnd, 0x466, (WPARAM)csvid, 0);
  }
  else
  {
    ScreenToClient(target, &pt);
    pt.x = pt.x - point.x;
    pt.y = pt.y - point.y;

    change_global_deck_card_availability(csvid, 2);
    for (i = 0; i < (int)(num & 0xFFFF); i++)
      if (!SendMessageA(global_decksurface_hwnd, 0x4C8, (WPARAM)csvid, MAKELONG(pt.x, pt.y)))
        MessageBeep(0);

    count_packs();
  }

  if (global_edited_deck_num_entries == 0)
    global_deck_was_edited = 0;
}

// FUNCTION: DECKDLL 0x1000810e
LRESULT CALLBACK
wndproc_CardClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  // GLOBAL: DECKDLL 0x100f2858
  static POINT card_mouse_pt;
  // GLOBAL: DECKDLL 0x100f2860
  static int card_sell_price;

  RECT window_rect;      /* ebp - 0xd4 */
  HDC paint_hdc;         /* ebp - 0xc4 */
  PAINTSTRUCT paint;     /* ebp - 0xc0 */
  int saved_dc;          /* ebp - 0x84 */
  RECT client_rect;      /* ebp - 0x80 */
  MSG dblclick_msg;      /* ebp - 0x70 */
  WORD amount;           /* ebp - 0x54 */
  POINT popup_pt;        /* ebp - 0x50 */
  UINT i_or_delay_ms;    /* ebp - 0x48 */
  char text_buf[32];     /* ebp - 0x44 */
  int iid;               /* ebp - 0x24 */
  int delete_failed;     /* ebp - 0x20 */
  WPARAM csvid;          /* ebp - 0x18 */
  RECT decksurface_rect; /* ebp - 0x14 */
  LONG sell_csvid;       /* ebp - 0x4 */

  switch (msg)
  {
  case WM_USER:
    return csvid = GetWindowLongA(hwnd, global_cardclass_csvid_offset);

  case WM_USER + 1:
    amount = (WORD)wparam;
    SetWindowWord(hwnd, global_cardclass_amount_offset, amount);
    InvalidateRect(hwnd, NULL, TRUE);
    return 0;

  case WM_USER + 2:
    amount = GetWindowWord(hwnd, global_cardclass_amount_offset);
    return amount & 0xffff;

  case WM_COMMAND:
    switch (wparam & 0xffff)
    {
    case RES_SELLMENU_SELLCARDFORX:
      sell_csvid = GetWindowLongA(hwnd, global_cardclass_csvid_offset);
      amount = GetWindowWord(hwnd, global_cardclass_amount_offset);
      global_dlg_result = 1;
      global_dlg_parameter = 1;

      if ((1 < (WORD)amount) && (show_dialog_sellxcards() == 0))
        return 0;
      if (global_dlg_result == 0)
        return 0;

      if (GetWindowWord(hwnd, global_cardclass_amount_offset) < global_dlg_result)
      {
        global_dlg_result = GetWindowWord(hwnd, global_cardclass_amount_offset);
      }

      delete_failed = 0;
      i_or_delay_ms = 0;
      for (; ((int)i_or_delay_ms < global_dlg_result) && (delete_failed == 0); i_or_delay_ms++)
      {
        if (delete_card_from_global_deck(sell_csvid, 0) == 0)
          delete_failed = 1;
      }
      *Gold = *Gold + card_sell_price * global_dlg_result;

      count_packs();

      load_text(s_menus_10031824, s_PRICE_1003181c);
      sprintf(text_buf, text_lines[0], *Gold);
      strncpy(global_deckinfo.deckname, text_buf, 0xC);
      InvalidateRect(global_title_hwnd, NULL, TRUE);

      if (global_dlg_result < GetWindowWord(hwnd, global_cardclass_amount_offset))
      {
        SetWindowWord(hwnd, global_cardclass_amount_offset, amount - global_dlg_result);
        InvalidateRect(hwnd, NULL, FALSE);
      }
      else
        DestroyWindow(hwnd);

      GetClientRect(global_decksurface_hwnd, &decksurface_rect);
      LockWindowUpdate(global_main_hwnd);
      reset_dim_and_pos_all_cards(global_decksurface_hwnd, &decksurface_rect);
      LockWindowUpdate(NULL);
      refresh_numofcards_text();
    }

    return 0;
  case WM_CREATE:
    csvid = *(LONG *)lparam;
    SetWindowLongA(hwnd, global_cardclass_csvid_offset, csvid);
    amount = 1;
    SetWindowWord(hwnd, global_cardclass_amount_offset, amount);
    if ((int)csvid < 0 || ((int)csvid > global_available_slots))
      return -1;
    return 0;

  case WM_DESTROY:
    return 0;

  case WM_MOUSEMOVE:
    csvid = GetWindowLongA(hwnd, global_cardclass_csvid_offset);
    SendMessageA(global_fullcard_hwnd, WM_USER, csvid, 0);
    return 0;

  case WM_LBUTTONDBLCLK:
  {
    if ((global_db_flags_2 & 1) == 0)
      return 0;

    card_mouse_pt.x = GET_X_LPARAM(lparam);
    card_mouse_pt.y = GET_Y_LPARAM(lparam);
    if ((wparam & MK_SHIFT) != 0)
      TENTATIVE_move_from_owned_cards(hwnd, card_mouse_pt, 2, 0);
    else
      TENTATIVE_move_from_owned_cards(hwnd, card_mouse_pt, 1, 0);
    SendMessageA(global_decksurface_hwnd, WM_COMMAND, RES_MENU_REFRESH, 0);
    return 0;
  }

  case WM_LBUTTONDOWN:
  {
    if ((global_db_flags_2 & 1) == 0)
      return 0;

#ifndef SHANDALAR_SDL_WIN32_COMPAT
    i_or_delay_ms = GetDoubleClickTime();
    Sleep(i_or_delay_ms);
    if (PeekMessageA(&dblclick_msg, hwnd, WM_LBUTTONDBLCLK, WM_LBUTTONDBLCLK, 0) != 0)
      return 0;
#endif

    card_mouse_pt.x = GET_X_LPARAM(lparam);
    card_mouse_pt.y = GET_Y_LPARAM(lparam);
    if ((wparam & MK_SHIFT) != 0)
      TENTATIVE_move_from_owned_cards(hwnd, card_mouse_pt, 2, 1);
    else
      TENTATIVE_move_from_owned_cards(hwnd, card_mouse_pt, 1, 1);
    SendMessageA(global_decksurface_hwnd, WM_COMMAND, RES_MENU_REFRESH, 0);
    return 0;
  }

  case WM_PAINT:
  {
    amount = GetWindowWord(hwnd, global_cardclass_amount_offset);
    csvid = GetWindowLongA(hwnd, global_cardclass_csvid_offset);
    paint_hdc = BeginPaint(hwnd, &paint);
    GetClientRect(hwnd, &client_rect);
    ApplyCardArtPaletteToDc(paint_hdc);
    FillRect(global_hdc, &client_rect, global_create_brush_0);
    DrawSmallCard(global_hdc, &client_rect, global_raw_cards_storage + (int)csvid, 1, 1);
    if (1 < (int)(amount & 0xFFFF))
    {
      sprintf(text_buf, format_decimal_int, (unsigned int)(amount & 0xFFFF));
      saved_dc = SaveDC(global_hdc);
      SetMapMode(global_hdc, MM_ANISOTROPIC);
      SetWindowExtEx(global_hdc, 200, 0x118, NULL);
      SetViewportExtEx(global_hdc,
                       client_rect.right - client_rect.left,
                       client_rect.bottom - client_rect.top,
                       NULL);
      SetViewportOrgEx(global_hdc, client_rect.left, client_rect.top, NULL);
      SetBkMode(global_hdc, TRANSPARENT);
      SelectObject(global_hdc, global_card_amount_font);
      SetTextAlign(global_hdc, 0);
      SetTextColor(global_hdc, PALETTERGB(0x43, 0x43, 0x43));

      TextOutA(global_hdc, 200 - (int)strlen(text_buf) * 0x10 + 3, 3, text_buf, (int)strlen(text_buf));
      SetTextColor(global_hdc, PALETTERGB(0xF1, 0xD9, 0x27));
      SetBkMode(global_hdc, TRANSPARENT);

      TextOutA(global_hdc, 200 - (int)strlen(text_buf) * 0x10, 0, text_buf, (int)strlen(text_buf));
      RestoreDC(global_hdc, saved_dc);
    }
    BitBlt(paint_hdc, 0, 0, client_rect.right, client_rect.bottom, global_hdc, 0, 0, SRCCOPY);
    EndPaint(hwnd, &paint);
    return 0;
  }

  case WM_RBUTTONDOWN:
  {
    popup_pt.x = GET_X_LPARAM(lparam);
    popup_pt.y = GET_Y_LPARAM(lparam);
    if ((global_db_flags_1 & DBFLAGS_STANDALONE) != 0)
    {
      ClientToScreen(hwnd, &popup_pt);
      ScreenToClient(GetParent(hwnd), &popup_pt);
      SendMessageA(GetParent(hwnd), msg, wparam,
                   (LPARAM)((unsigned short)popup_pt.y << 16) | (unsigned short)popup_pt.x);
      return 0;
    }
    else if ((global_db_flags_2 & DBFLAGS_STANDALONE) == 0)
      return 0;

    sell_csvid = GetWindowLongA(hwnd, global_cardclass_csvid_offset);
    iid = get_iid_from_global_deck_card(sell_csvid);
    if (iid == -1)
      return 0;
    card_sell_price = SellPrice(iid);
    load_text(s_menus_1003183c, s_SELLCARD_10031830);
    sprintf(text_buf, text_lines[0], card_sell_price);
    AppendMenuA(global_smallcard_popup, 0, RES_SELLMENU_SELLCARDFORX, text_buf);
    ClientToScreen(hwnd, &popup_pt);
    TrackPopupMenu(global_smallcard_popup, TPM_RIGHTBUTTON, popup_pt.x + 10, popup_pt.y + 10, 0, hwnd, NULL);
    DeleteMenu(global_smallcard_popup, 0, MF_BYPOSITION);

    return 0;
  }

  case WM_SIZE:
    GetWindowRect(hwnd, &window_rect);
    global_smallcard_width = window_rect.right - window_rect.left;
    global_smallcard_height = window_rect.bottom - window_rect.top;
    return 0;

  default:
    return DefWindowProcA(hwnd, msg, wparam, lparam);
  }

  return 0;
}

// FUNCTION: DECKDLL 0x10019d9f
static void copy_deck_to_edit(void)
{
  int i;         /* ebp - 0x8 */
  csvid_t csvid; /* ebp - 0x4 */
  global_deck_num_entries = 0;
  global_deck_num_entries_copy = global_deck_num_entries;
  i = global_deck_num_entries_copy;
  global_deck_num_cards = 0;
  global_edited_deck_num_entries = 0;
  while (i < 0x1f4)
  {
    csvid = CardIDFromType(deck[i]);
    if (csvid == -1)
    {
      count_packs();
      return;
    }

    global_deck[i].GDE_csvid = csvid;
    global_deck[i].GDE_iid = deck[global_deck_num_entries] & 0xFFF;
    global_deck[i].GDE_Available = 1;
    global_deck[i].GDE_DecksBits = (deck[global_deck_num_entries] & 0x70000) >> 16;

    if ((deck[global_deck_num_entries] & (1 << (global_current_deck + 0x10))) != 0)
    {
      insert_cards_into_deck(global_deck[i].GDE_csvid, 1, &global_edited_deck);
      global_deck[i].GDE_Available = 0;
    }

    ++global_deck_num_entries;
    ++i;
  }
}

// FUNCTION: DECKDLL 0x1000e770
static bool
save_deck(const char *filename)
{
  FILE *f;    /* ebp - 0xc */
  int i;      /* ebp - 0x8 */
  int result; /* ebp - 0x4 */

  f = fopen(filename, "wt");
  if (f != 0)
  {
    fprintf(f, ";%s\n", global_deckinfo.deckname);
    fprintf(f, ";%s\n", global_deckinfo.description);
    fprintf(f, ";%s\n", global_deckinfo.author);
    fprintf(f, ";%s\n", global_deckinfo.email);
    fprintf(f, ";%s\n", global_deckinfo.creation_date);
    fprintf(f, ";%d\n", global_deckinfo.revision);
    fprintf(f, ";%s\n", global_deckinfo.edition);
    fprintf(f, ";%s\n", global_deckinfo.comments);
    fprintf(f, "\n");

    for (i = 0; i < global_edited_deck_num_entries; ++i)
      fprintf(f, ".%d\t%d\t%s\n",
              global_edited_deck.entries[i].DeckEntry_csvid,
              global_edited_deck.entries[i].DeckEntry_Amount,
              global_edited_deck.entries[i].DeckEntry_FullName);

    fclose(f);
    result = 1;
  }
  else
    result = 0;

  return result;
}

// FUNCTION: DECKDLL 0x1000e90f
static int
load_deck(char *filename)
{
  size_t len_comments;  /* ebp - 0x224 */
  size_t len_edition;   /* ebp - 0x220 */
  size_t len_revision;  /* ebp - 0x21c */
  size_t len_date;      /* ebp - 0x218 */
  size_t len_email;     /* ebp - 0x214 */
  size_t len_author;    /* ebp - 0x210 */
  size_t len_deckname2; /* ebp - 0x20c */
  size_t len_deckname1; /* ebp - 0x208 */
  FILE *f;              /* ebp - 0x204 */
  char txt[500];        /* ebp - 0x200 */
  int ok;               /* ebp - 0xc */
  int num;              /* ebp - 0x8 */
  csvid_t csvid;        /* ebp - 0x4 */

  f = fopen(filename, "rt");
  if (f != 0)
  {
    memset(&global_edited_deck, 0, 0x1464);

    if (global_db_flags_1 & DBFLAGS_GAUNTLET)
    {
      global_deck_num_entries_copy = 0;
      global_deck_num_entries = global_deck_num_entries_copy;
    }

    fgets(txt, 0x1f, f);
    if (txt[0] != ';')
      return 0;
    len_deckname1 = strlen(txt);
    txt[len_deckname1 - 1] = 0;
    strncpy(global_deckinfo.deckname, &txt[1], 0x1f);

    fgets(txt, 0x15, f);
    if (txt[0] != ';')
      return 0;
    len_deckname2 = strlen(txt);
    txt[len_deckname2 - 1] = 0;
    strncpy(global_deckinfo.description, &txt[1], 0x15);

    fgets(txt, 0x51, f);
    if (txt[0] != ';')
      return 0;
    len_author = strlen(txt);
    txt[len_author - 1] = 0;
    strncpy(global_deckinfo.author, &txt[1], 0x51);

    fgets(txt, 0x51, f);
    if (txt[0] != ';')
      return 0;
    len_email = strlen(txt);
    txt[len_email - 1] = 0;
    strncpy(global_deckinfo.email, &txt[1], 0x51);

    fgets(txt, 0x16, f);
    if (txt[0] != ';')
      return 0;
    len_date = strlen(txt);
    txt[len_date - 1] = 0;
    strncpy(global_deckinfo.creation_date, &txt[1], 0x16);

    fgets(txt, 0x10, f);
    len_revision = strlen(txt);
    txt[len_revision - 1] = 0;
    if (txt[0] != ';')
      return 0;
    global_deckinfo.revision = atoi(&txt[1]);

    fgets(txt, 0x10, f);
    if (txt[0] != ';')
      return 0;
    len_edition = strlen(txt);
    txt[len_edition - 1] = 0;
    strncpy(global_deckinfo.edition, &txt[1], 0x10);

    fgets(txt, 0x191, f);
    if (txt[0] != ';')
      return 0;
    len_comments = strlen(txt);
    txt[len_comments - 1] = 0;
    strncpy(global_deckinfo.comments, &txt[1], 0x191);

    while (1 && (int)read_deck_line(f, txt) != -1 && is_deck_version_line(txt) == 0)
    {
      if (is_deck_card_line(txt))
      {
        sscanf(txt, ".%d %d", &csvid, &num);
        if (num != 0)
          insert_cards_into_deck(csvid, num, &global_edited_deck);
        else
          append_trade_bucket_entry(6, csvid, num, &global_edited_deck);
      }
    }

    fclose(f);
    ok = 1;
  }
  else
  {
    ok = 0;
  }

  if (ok != 0)
  {
    count_packs();
    ok = SendMessage(global_decksurface_hwnd, 0x401, 0, 0);
  }
  return ok;
}

// FUNCTION: DECKDLL 0x1000edea
static int read_deck_line(FILE *file, char *dest)
{
  int len;
  int ch;

  for (len = 0; (ch = fgetc(file)) != '\n' && ch != -1;)
  {
    *dest = (char)ch;
    dest++;
    len++;
  }

  *dest = 0;
  if (ch == -1)
    return -1;
  else
    return len;
}

// FUNCTION: DECKDLL 0x1000ee5d
static int is_deck_card_line(char *line)
{
  if (*line == '.' && line[1] != 'v')
    return 1;
  return 0;
}

// FUNCTION: DECKDLL 0x1000ee98
static int is_deck_version_line(char *line)
{
  if (*line == 'v')
    return 1;
  if (*line == '.' && line[1] == 'v')
    return 1;
  return 0;
}

// FUNCTION: DECKDLL 0x1002b7bb
static int
check_card_count(int csvid)
{
  int i;
  unsigned int limit;

  limit = Scards[5].worldmagic_city == 0;

  if (global_deck_num_cards < 59)
  {
    for (i = 0; i < global_edited_deck_num_entries; ++i)
    {
      if (global_edited_deck.entries[i].DeckEntry_csvid == csvid && CardTypeFromID(csvid) > 4)
        return global_edited_deck.entries[i].DeckEntry_Amount - (limit + 3);
    }
  }
  else
  {
    for (i = 0; i < global_edited_deck_num_entries; ++i)
    {
      if (global_edited_deck.entries[i].DeckEntry_csvid == csvid && CardTypeFromID(csvid) > 4)
        return global_edited_deck.entries[i].DeckEntry_Amount - (limit * 99 + 4);
    }
  }

  return 0;
}

// FUNCTION: DECKDLL 0x10024e48
void draw_ownerdraw_button_item(DRAWITEMSTRUCT *item, HBRUSH brush, HPEN pen1, HPEN pen2, COLORREF col, int do_focus)
{
  draw_item(item, brush, (HANDLE)0x0, pen1, pen2, col, do_focus, 0x25);
}

// FUNCTION: DECKDLL 0x1000942e
static void NoopProcessHorzListCard(HWND hwnd, int csvid)
{
  (void)hwnd;
  (void)csvid;
}

// FUNCTION: DECKDLL 0x10019ed5
static void
append_unique_csvid(int csvid, int *list, int *count)
{
  int countVal;
  int i;
  bool found;

  countVal = *count;
  found = false;

  for (i = 0; i < countVal; i++)
  {
    if (list[i] == csvid)
    {
      found = true;
    }
  }
  if (!found)
  {
    list[countVal] = csvid;
    countVal++;
  }
  *count = countVal;
}

// FUNCTION: DECKDLL 0x10016e4e
LRESULT CALLBACK
wndproc_MainClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  int border_adjust;     /* ebp - 0xb40 */
  int margin8;           /* ebp - 0xb3c */
  int fullcard_height;   /* ebp - 0xb38 */
  int horzlist_hgt6;     /* ebp - 0xb34 */
  int decksurface_x;     /* ebp - 0xb30 */
  LONG horzlist_style;   /* ebp - 0xb2c */
  int title_height;      /* ebp - 0xb28 */
  RECT rect_load_button; /* ebp - 0xb24 */
  int decksurface_top;   /* ebp - 0xb14 */

  int main_h; /* ebp - 0xb10 */
  int ten;    /* ebp - 0xb0c */

  RECT rect_cardlistfilter; /* ebp - 0xb08 */
  HDC pad_b0c;              /* ebp - 0xb04 */
  int main_w;               /* ebp - 0xaf4 */
  int cardlistfilter_hgt;   /* ebp - 0xaf0 */

  RECT rect_title;    /* ebp - 0xaec */
  RECT rect_pad_adc;  /* ebp - 0xadc */
  RECT rect_fullcard; /* ebp - 0xacc */
  int third_w;        /* ebp - 0xabc */

  SIZE sz;               /* ebp - 0xab8 */
  RECT rect_horzlist;    /* ebp - 0xab0 */
  RECT rect_decksurface; /* ebp - 0xaa0 */
  POINT pad_a90;         /* ebp - 0xa90 */

  HWND measure_hwnd; /* ebp - 0xa8c */
  MEASUREITEMSTRUCT *measure_item;
  RECT measure_client_rect;
  HDC erase_dc; /* ebp - 0xa74 */

  BITMAP tile_bitmap; /* ebp - 0xa6c */
  int tile_y;         /* ebp - 0xa54 */
  int tile_x;         /* ebp - 0xa50 */

  RECT r;                       /* ebp - 0xa4c */
  COLORREF drawitem_fill_color; /* ebp - 0xa3c */

  DRAWITEMSTRUCT *draw_item; /* ebp - 0xa38 */
  HWND color_target;         /* ebp - 0xa38 */
  int color_ctrl_id;         /* ebp - 0xa34 */
  HDC color_dc;

  HBRUSH ctlcolor_brush; /* ebp - 0xa28 */
  HWND invalidate_hwnd;  /* ebp - 0xa24 */

  HWND clicked_button;
  HDC display_dc;
  HGDIOBJ title_font_obj;
  HWND listbox_hwnd_param;
  HDC listbox_dc;

  int csvidraw;     /* ebp - 0xa0c */
  int csvid;        /* ebp - 0xa08 */
  int unique_count; /* ebp - 0xa04 */

  int legal_scan_index;      /* ebp - 0xa00 */
  int legal_unique_ids[100]; /* ebp - 0x9fc */
  char legal_message[2000];  /* ebp - 0x86c */

  HWND cmd_hwnd;        /* ebp - 0x9c */
  HWND cmd_tgt;         /* ebp - 0x98 */
  int cmd_cursel;       /* ebp - 0x94 */
  LRESULT cmd_itemdata; /* ebp - 0x90 */

  HDC hdc_local;    /* ebp - 0x8c */
  HDC hdc;          /* ebp - 0x88 */
  WPARAM wparam_84; /* ebp - 0x84 */

  LPARAM lparam_copy; /* ebp - 0x80 */
  WPARAM wparam_copy; /* ebp - 0x7c */

  WORD wPicsInline;      /* ebp - 0x78 */
  FILE *f;               /* ebp - 0x74 */
  int *horz_list_addr;   /* ebp - 0x70 */
  int i;                 /* ebp - 0x6c */
  unsigned int artist_i; /* ebp - 0x68 */

  HDC tile_dc;      /* ebp - 0x64 */
  char message[80]; /* ebp - 0x60 */

  WORD wLeft;       /* ebp - 0x10 */
  int excess;       /* ebp - 0xc */
  int check_excess; /* ebp - 8 */
  WORD wLast;       /* ebp - 4 */
  // GLOBAL: DECKDLL 0x10104c98
  static HPEN pen_ltgrey;
  // GLOBAL: DECKDLL 0x10104c9c
  static HPEN pen_medgrey;
  // GLOBAL: DECKDLL 0x10104ca4
  static HGDIOBJ mainclass_destroy_obj;
  // wndproc_MainClass statics
  // GLOBAL: DECKDLL 0x101a8c24
  static HWND button_deckinfo;
  // GLOBAL: DECKDLL 0x1019e9d0
  static HWND button_exit;
  // GLOBAL: DECKDLL 0x1014263c
  static HWND button_deck1;
  // GLOBAL: DECKDLL 0x1014250c
  static HWND button_deck2;
  // GLOBAL: DECKDLL 0x101423cc
  static HWND button_deck3;
  // GLOBAL: DECKDLL 0x10104b8c
  static HMENU popup;
  // GLOBAL: DECKDLL 0x10035208
  static const char wndname_title[] = "";
  // GLOBAL: DECKDLL 0x10035224
  static const char wndname_fullcard[] = "";
  // GLOBAL: DECKDLL 0x1003523c
  static const char wndname_decksurface[] = "";
  // GLOBAL: DECKDLL 0x1003525c
  static const char wndname_button_stats[] = "";
  // GLOBAL: DECKDLL 0x10035268
  static const char wndname_button_deckinfo[] = "";
  // GLOBAL: DECKDLL 0x10035274
  static const char wndname_button_exit[] = "";
  // GLOBAL: DECKDLL 0x10035280
  static const char wndname_button_deck1[] = "";
  // GLOBAL: DECKDLL 0x1003528c
  static const char wndname_button_deck2[] = "";
  // GLOBAL: DECKDLL 0x10035298
  static const char wndname_button_deck3[] = "";
  // GLOBAL: DECKDLL 0x100352a4
  static const char wndname_listbox[] = "";
  // GLOBAL: DECKDLL 0x100352b0
  static const char wndname_horzlist[] = "";
  // GLOBAL: DECKDLL 0x100352cc
  static const char wndname_cardlistfilter[] = "";

  // GLOBAL: DECKDLL 0x10034f08
  static LOGFONT mainClass_fontTemplate = {
      0,         // lfHeight
      0,         // lfWidth
      0,         // lfEscapement
      0,         // lfOrientation
      FW_NORMAL, // lfWeight
  };
  // GLOBAL: DECKDLL 0x10104b50
  static LOGFONT main_font_logfont;

  switch (msg)
  {
  case 0x466:
    wparam_copy = wparam;
    lparam_copy = lparam;
    if ((HWND)lparam_copy != global_fullcard_hwnd)
      SendMessage(global_fullcard_hwnd, 0x466, wparam_copy, 0);
    return 0;

  case WM_PALETTECHANGED:
    wparam_84 = wparam;
    if (hwnd != (HWND)wparam_84)
    {
      if (global_main_hwnd != (HWND)wparam_84)
      {
        hdc = GetDC(hwnd);
        SelectPalette(hdc, global_cart_art_hpalette, TRUE);
        if (RealizePalette(hdc))
          InvalidateRect(hwnd, NULL, FALSE);
        ReleaseDC(hwnd, hdc);
      }
    }
    return 0;

  case WM_QUERYNEWPALETTE:
    hdc_local = GetDC(hwnd);
    SelectPalette(hdc_local, global_cart_art_hpalette, FALSE);
    if (RealizePalette(hdc_local))
      InvalidateRect(hwnd, NULL, FALSE);
    ReleaseDC(hwnd, hdc_local);
    return 1;

  case WM_CLOSE:
    if ((global_db_flags_1 & 0x0a) || (global_db_flags_1 & 4))
    {
      if (!global_deck_was_edited || ask_about_saving_deck() != 2)
        DestroyWindow(global_main_hwnd);
    }
    else
    {
      artist_i = 0;
      i = 0;
      excess = 0;
      for (; global_edited_deck_num_entries > (int)artist_i; ++artist_i)
      {
        check_excess = check_card_count(global_edited_deck.entries[artist_i].DeckEntry_csvid);
        if (check_excess > 0)
        {
          global_excessive_cards[i].DeckEntry_csvid = global_edited_deck.entries[artist_i].DeckEntry_csvid;
          global_excessive_cards[i].DeckEntry_Amount = check_excess;
          global_excessive_cards[i].DeckEntry_FullName =
              global_raw_cards_storage[global_edited_deck.entries[artist_i].DeckEntry_csvid].full_name;
          ++i;
          excess = 1;
        }
      }
      global_excessive_cards[i].DeckEntry_csvid = -1;

      if (excess)
      {
        if (show_dialog_infobox())
          DestroyWindow(global_main_hwnd);
      }
      else
        DestroyWindow(global_main_hwnd);
    }
    return 0;

  case WM_COMMAND:
  {
    switch ((int)wparam & 0xffff)
    {
    case RES_UNK:
      if (global_deckbuilder_unknown_1019f010 != 0)
        global_deckbuilder_unknown_1019f010 = 0;
      else
        global_deckbuilder_unknown_1019f010 = 1;
      break;

    case 4:
    case 5:
    {
      cmd_hwnd = (HWND)lparam;
      if (((unsigned int)wparam & 0xffff) == 4)
        cmd_tgt = global_horzlist_hwnd;
      else
        cmd_tgt = global_listbox_hwnd;

      if ((((unsigned int)wparam >> 16) & 0xffff) == 1)
      {
        cmd_cursel = SendMessage(cmd_hwnd, LB_GETCURSEL, 0, 0);
        cmd_itemdata = SendMessage(cmd_hwnd, LB_GETITEMDATA, cmd_cursel, 0);
        SendMessage(global_fullcard_hwnd, 0x400, cmd_itemdata, 0);

        cmd_cursel = SendMessage(cmd_hwnd, LB_GETCURSEL, 0, 0);
        SendMessage(cmd_tgt, LB_SETCURSEL, cmd_cursel, 0);
      }
      break;
    }

    case 10:
    {
      unique_count = 0;

      for (legal_scan_index = 0; global_edited_deck_num_entries > legal_scan_index; ++legal_scan_index)
        append_unique_csvid(global_edited_deck.entries[legal_scan_index].DeckEntry_csvid, legal_unique_ids, &unique_count);

      for (legal_scan_index = 0; global_edited_deck.trade[0].total > legal_scan_index; ++legal_scan_index)
        append_unique_csvid(global_edited_deck.trade[0].entries[legal_scan_index].DeckEntry_csvid, legal_unique_ids, &unique_count);
      for (legal_scan_index = 0; global_edited_deck.sideboard[0].total > legal_scan_index; ++legal_scan_index)
        append_unique_csvid(global_edited_deck.sideboard[0].entries[legal_scan_index].DeckEntry_csvid, legal_unique_ids, &unique_count);

      for (legal_scan_index = 0; global_edited_deck.trade[1].total > legal_scan_index; ++legal_scan_index)
        append_unique_csvid(global_edited_deck.trade[1].entries[legal_scan_index].DeckEntry_csvid, legal_unique_ids, &unique_count);
      for (legal_scan_index = 0; global_edited_deck.sideboard[1].total > legal_scan_index; ++legal_scan_index)
        append_unique_csvid(global_edited_deck.sideboard[1].entries[legal_scan_index].DeckEntry_csvid, legal_unique_ids, &unique_count);

      for (legal_scan_index = 0; global_edited_deck.trade[2].total > legal_scan_index; ++legal_scan_index)
        append_unique_csvid(global_edited_deck.trade[2].entries[legal_scan_index].DeckEntry_csvid, legal_unique_ids, &unique_count);
      for (legal_scan_index = 0; global_edited_deck.sideboard[2].total > legal_scan_index; ++legal_scan_index)
        append_unique_csvid(global_edited_deck.sideboard[2].entries[legal_scan_index].DeckEntry_csvid, legal_unique_ids, &unique_count);

      for (legal_scan_index = 0; global_edited_deck.trade[3].total > legal_scan_index; ++legal_scan_index)
        append_unique_csvid(global_edited_deck.trade[3].entries[legal_scan_index].DeckEntry_csvid, legal_unique_ids, &unique_count);
      for (legal_scan_index = 0; global_edited_deck.sideboard[3].total > legal_scan_index; ++legal_scan_index)
        append_unique_csvid(global_edited_deck.sideboard[3].entries[legal_scan_index].DeckEntry_csvid, legal_unique_ids, &unique_count);

      for (legal_scan_index = 0; global_edited_deck.trade[4].total > legal_scan_index; ++legal_scan_index)
        append_unique_csvid(global_edited_deck.trade[4].entries[legal_scan_index].DeckEntry_csvid, legal_unique_ids, &unique_count);
      for (legal_scan_index = 0; global_edited_deck.sideboard[5].total > legal_scan_index; ++legal_scan_index)
        append_unique_csvid(global_edited_deck.sideboard[5].entries[legal_scan_index].DeckEntry_csvid, legal_unique_ids, &unique_count);

      for (legal_scan_index = 0; global_edited_deck.trade[5].total > legal_scan_index; ++legal_scan_index)
        append_unique_csvid(global_edited_deck.trade[5].entries[legal_scan_index].DeckEntry_csvid, legal_unique_ids, &unique_count);
      for (legal_scan_index = 0; global_edited_deck.sideboard[4].total > legal_scan_index; ++legal_scan_index)
        append_unique_csvid(global_edited_deck.sideboard[4].entries[legal_scan_index].DeckEntry_csvid, legal_unique_ids, &unique_count);

      for (legal_scan_index = 0; global_edited_deck.trade[6].total > legal_scan_index; ++legal_scan_index)
        append_unique_csvid(global_edited_deck.trade[6].entries[legal_scan_index].DeckEntry_csvid, legal_unique_ids, &unique_count);

      csvid = 0;
      strcpy(legal_message, "The following cards are not in 4th edition:\n");

      for (legal_scan_index = 0; legal_scan_index < unique_count; ++legal_scan_index)
      {
        if (!check_filters(legal_unique_ids[legal_scan_index]))
        {
          strcat(legal_message, global_raw_cards_storage[legal_unique_ids[legal_scan_index]].full_name);
          strcat(legal_message, "\n");
          ++csvid;
        }
      }

      if (csvid)
        MessageBoxA(hwnd, legal_message, "Checking deck for legal cards", 0);
      else
        MessageBoxA(hwnd, "All cards are in 4th edition", "Checking deck for legal cards", 0);
      break;
    }

    case RES_MAINMENU_NEWDECK:
      SendMessage(global_decksurface_hwnd, RES_MENU_NEWDECK, 0, 0);
      break;

    case RES_MAINMENU_LOADDECK:
      if ((!global_deck_was_edited || ask_about_saving_deck() != 2) && show_dialog_loaddeck(global_deck_filename))
      {
        if (load_deck(global_deck_filename))
        {
          InvalidateRect(global_title_hwnd, NULL, TRUE);
          global_deck_was_edited = false;
        }
        else
        {
          load_text("menus", "DECKLOADERROR");
          sprintf(message, text_lines[0], global_deck_filename);

          load_text("menus", "DECKBUILDER");
          MessageBoxA(hwnd, message, text_lines[0], MB_ICONHAND);

          load_text("menus", "NEWDECK");
          sprintf(global_deckinfo.deckname, text_lines[0]);

          strcpy(global_deck_filename, global_playdeck_path);
          strcat(global_deck_filename, "\\new.dck");
        }
      }
      break;

    case RES_MAINMENU_SAVEDECK:
      load_text("menus", "NEWDECK");
      if (!strcmp(global_deckinfo.deckname, text_lines[0]) || !global_deckinfo.deckname[0])
      {
        load_text("menus", "NAMEYOURDECK");
        strcpy(message, text_lines[0]);
        load_text("menus", "DECKBUILDER");
        MessageBoxA(hwnd, message, text_lines[0], MB_ICONINFORMATION);
        show_dialog_deckinfo();
        return 2;
      }

      if (global_deck_num_cards < 40)
      {
        load_text("menus", "TOOFEWCARDS");
        strcpy(message, text_lines[0]);
        load_text("menus", "DECKBUILDER");
        MessageBoxA(hwnd, message, text_lines[0], MB_ICONINFORMATION);
        return 2;
      }

      if (global_deck_num_cards > 500 || global_edited_deck_num_entries > 200)
      {
        load_text("menus", "TOOMANYCARDS");
        strcpy(message, text_lines[0]);
        load_text("menus", "DECKBUILDER");
        MessageBoxA(hwnd, message, text_lines[0], MB_ICONINFORMATION);
      }

      strcpy(global_deck_filename, global_playdeck_path);
      strcat(global_deck_filename, "\\");
      strcat(global_deck_filename, global_deckinfo.deckname);
      strcat(global_deck_filename, ".dck");

      if (global_deckname_set)
      {
        f = fopen(global_deck_filename, "rt");
        if (f != NULL)
        {
          fclose(f);

          load_text("menus", "DECKEXISTS");
          sprintf(message, text_lines[0], global_deckinfo.deckname);
          load_text("menus", "DECKBUILDER");
          artist_i = MessageBoxA(hwnd, message, text_lines[0], MB_ICONQUESTION | MB_YESNOCANCEL);
          if (artist_i == IDCANCEL || artist_i == IDNO)
            return 2;
        }
      }

      global_deckname_set = false;
      global_deck_was_edited = false;

      if (save_deck(global_deck_filename))
      {
        load_text("menus", "SAVED");
        sprintf(message, text_lines[0], global_deckinfo.deckname);
        load_text("menus", "DECKBUILDER");
        MessageBoxA(hwnd, message, text_lines[0], MB_ICONINFORMATION);
      }
      else
      {
        load_text("menus", "DECKSAVEERROR");
        sprintf(message, text_lines[0], global_deckinfo.deckname);
        load_text("menus", "DECKBUILDER");
        MessageBoxA(hwnd, message, text_lines[0], MB_ICONINFORMATION);
      }

      if (global_db_flags_1 & (DBFLAGS_EDITDECK | DBFLAGS_GAUNTLET | DBFLAGS_NOCARDCOUNTCHECK))
        strcpy(global_external_deckname, global_deckinfo.deckname);

      return 6;

    case RES_FULLCARDMENU_EXPAND:
      SendMessage(global_fullcard_hwnd, msg, wparam, lparam);
      break;

    case RES_MENU_CONSOLIDATE:
      SendMessage(global_decksurface_hwnd, msg, wparam, lparam);
      break;

    case RES_MAINMENU_EFFECTS:
      global_cfg.effects ^= 1;
      break;

    case RES_MAINMENU_MUSIC:
      global_cfg.music ^= 1;
      if (global_cfg.music)
        play_music(1, 400, 0);
      else
        sound_stop(1);
      break;

    case RES_MAINMENU_MINIMIZE:
      SendMessage(global_main_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
      break;

    case RES_MAINMENU_EXIT:
    case RES_MAINMENU_BUTTON_EXIT:
      SendMessage(global_main_hwnd, WM_CLOSE, 0, 0);
      break;

    case RES_MAINMENU_BUTTON_STATS:
      DialogBoxParam(global_hinstance, MAKEINTRESOURCE(RES_DIALOG_DECKSTATS), global_main_hwnd, dlgproc_DeckStats, 0);
      break;

    case RES_MAINMENU_BUTTON_DECKINFO:
      show_dialog_deckinfo();
      InvalidateRect(global_title_hwnd, 0, TRUE);
      break;

    case RES_MAINMENU_BUTTON_DECK_1:
    case RES_MAINMENU_BUTTON_DECK_2:
    case RES_MAINMENU_BUTTON_DECK_3:
      if (!(global_db_flags_2 & DBFLAGS_SHANDALAR))
        break;

      load_text("menus", "DECKNUMBERS");
      SetWindowText(button_deck1, text_lines[0]);
      SetWindowText(button_deck2, text_lines[1]);
      SetWindowText(button_deck3, text_lines[2]);

      global_current_deck = (((unsigned int)wparam & 0xffff) - RES_MAINMENU_BUTTON_DECK_1);
      global_deck_num_cards = 0;
      global_edited_deck_num_entries = 0;

      for (artist_i = 0; global_deck_num_entries > (int)artist_i; ++artist_i)
        if (global_deck[artist_i].GDE_DecksBits & (1 << global_current_deck))
        {
          global_deck[artist_i].GDE_Available = 0;
          insert_cards_into_deck(global_deck[artist_i].GDE_csvid, 1, &global_edited_deck);
        }
        else
          global_deck[artist_i].GDE_Available = 1;

      count_packs();

      SendMessage(global_decksurface_hwnd, 0x401, 0, 0);

      filter_cards_in_lists(global_listbox_hwnd, global_horzlist_hwnd);

      if (global_current_deck == 0)
        SetWindowText(button_deck1, text_lines[3]);
      else if (global_current_deck == 1)
        SetWindowText(button_deck2, text_lines[4]);
      else if (global_current_deck == 2)
        SetWindowText(button_deck3, text_lines[5]);

      SetFocus(global_horzlist_hwnd);

      break;
    default:
      break;
    }
    return 0;
  }

  case WM_CREATE:
    if (check_filters_debug_log_enabled != 0)
    {
      check_filters_debug_log_file = fopen("CardIDs.TXT", "wt");
      if (check_filters_debug_log_file == NULL)
      {
        check_filters_debug_log_enabled = 0;
        MessageBoxA(hwnd, "Could not create CARDID.TXT", "Deck Builder Error", MB_ICONHAND);
      }
    }

    global_colorref_lightgrey = PALETTEINDEX(7);
    global_colorref_darkgrey = PALETTEINDEX(1);
    global_brush_mediumgrey = CreateSolidBrush(PALETTEINDEX(0x86));
    pen_ltgrey = CreatePen(0, 0, PALETTEINDEX(0xC1));
    pen_medgrey = CreatePen(0, 0, PALETTEINDEX(0x16));
    global_colorref_flesh = PALETTEINDEX(1);
    global_colorref_white = PALETTEINDEX(0xBF);

    memcpy(&main_font_logfont, &mainClass_fontTemplate, sizeof(LOGFONT));
    main_font_logfont.lfHeight = 0x2a;
    main_font_logfont.lfWeight = 500;
    strcpy(main_font_logfont.lfFaceName, "Kudos Condensed SSi");
    global_card_amount_font = CreateFontIndirectA(&main_font_logfont);

    if (!(global_db_flags_1 & DBFLAGS_SHANDALAR))
      init_sound_dll(hwnd, 0, 0);

    init_sounds_and_music();

    strcpy(global_deck_filename, global_playdeck_path);
    strcat(global_deck_filename, "\\new.dck");

    global_num_artists = load_text("menus", "ARTISTNAMES");
    for (artist_i = 0; global_num_artists > (int)artist_i; artist_i = artist_i + 1)
      strcpy(read_db_artist_names[artist_i], text_lines[artist_i]);

    if (global_db_flags_1 & DBFLAGS_SHANDALAR)
    {
      load_text("menus", "GOLDTITLE");
      sprintf(message, text_lines[0], *Gold);
      strncpy(global_deckinfo.deckname, message, 12);
    }
    else if (global_db_flags_1 & DBFLAGS_EDITDECK)
    {
      global_deckinfo.deckname[0] = ' ';
      global_deckinfo.deckname[1] = '\0';
    }
    else
    {
      load_text("menus", "NEWDECK");
      sprintf(global_deckinfo.deckname, text_lines[0]);
    }

    global_deckinfo.description[0] = global_deckinfo.comments[0] = 0;
    global_deckinfo.revision = 1;
    strcpy(global_deckinfo.author, global_cfg.player_name);
    strcpy(global_deckinfo.email, global_cfg.email);

    GetDateFormatA(LOCALE_SYSTEM_DEFAULT, 0, NULL, "MMMM dd',' yyyy", global_deckinfo.creation_date, 22);

    if (global_db_flags_1 & (DBFLAGS_EDITDECK | DBFLAGS_GAUNTLET | DBFLAGS_SHANDALAR))
      copy_deck_to_edit();

    global_title_hwnd = CreateWindowEx(0, "MAGICDECK_TitleClass", wndname_title,
                                       WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                       0, 0, 0, 0,
                                       hwnd,
                                       (HMENU)1,
                                       global_hinstance, 0);

    global_fullcard_hwnd = CreateWindowEx(0, "MagicFullCardClass", wndname_fullcard,
                                          WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                          0, 0, 0, 0,
                                          hwnd,
                                          (HMENU)2,
                                          global_hinstance, 0);

    global_decksurface_hwnd = CreateWindowEx(0, "MAGICDECK_DeckSurfaceClass", wndname_decksurface,
                                             WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER,
                                             0, 0, 0, 0,
                                             hwnd,
                                             (HMENU)7,
                                             global_hinstance, 0);

    global_button_stats_hwnd = CreateWindowEx(0, "BUTTON", wndname_button_stats,
                                              WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_OWNERDRAW,
                                              0, 0, 0, 0,
                                              hwnd,
                                              (HMENU)RES_MAINMENU_BUTTON_STATS,
                                              global_hinstance, 0);

    button_deckinfo = CreateWindowEx(0, "BUTTON", wndname_button_deckinfo,
                                     WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_OWNERDRAW,
                                     0, 0, 0, 0,
                                     hwnd,
                                     (HMENU)RES_MAINMENU_BUTTON_DECKINFO,
                                     global_hinstance, 0);

    button_exit = CreateWindowEx(0, "BUTTON", wndname_button_exit,
                                 WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_OWNERDRAW,
                                 0, 0, 0, 0,
                                 hwnd,
                                 (HMENU)RES_MAINMENU_BUTTON_EXIT,
                                 global_hinstance, 0);

    button_deck1 = CreateWindowEx(0, "BUTTON", wndname_button_deck1,
                                  WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_OWNERDRAW,
                                  0, 0, 0, 0,
                                  hwnd,
                                  (HMENU)RES_MAINMENU_BUTTON_DECK_1,
                                  global_hinstance, 0);

    button_deck2 = CreateWindowEx(0, "BUTTON", wndname_button_deck2,
                                  WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_OWNERDRAW,
                                  0, 0, 0, 0,
                                  hwnd,
                                  (HMENU)RES_MAINMENU_BUTTON_DECK_2,
                                  global_hinstance, 0);

    button_deck3 = CreateWindowEx(0, "BUTTON", wndname_button_deck3,
                                  WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_OWNERDRAW,
                                  0, 0, 0, 0,
                                  hwnd,
                                  (HMENU)RES_MAINMENU_BUTTON_DECK_3,
                                  global_hinstance, 0);

    global_listbox_hwnd = CreateWindowEx(0, "LISTBOX", wndname_listbox,
                                         (WS_CHILDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL | WS_THICKFRAME | LBS_NOINTEGRALHEIGHT | LBS_HASSTRINGS | LBS_OWNERDRAWFIXED | LBS_SORT),
                                         0, 0, 0, 0,
                                         hwnd,
                                         (HMENU)4,
                                         global_hinstance, 0);

    global_horzlist_hwnd = CreateWindowEx(0, "MAGICDECK_HorzListClass", wndname_horzlist,
                                          WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL,
                                          0, 0, 0, 0,
                                          hwnd,
                                          (HMENU)5,
                                          global_hinstance, 0);

    cardlistfilter_hwnd = CreateWindowEx(0, "MAGICDECK_CardListFiltersClass", wndname_cardlistfilter,
                                         WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                         0, 0, 0, 0,
                                         hwnd,
                                         (HMENU)3,
                                         global_hinstance, 0);

    popup = CreatePopupMenu();
    global_smallcard_popup = CreatePopupMenu();

    if (!global_title_hwnd || !global_fullcard_hwnd || !global_listbox_hwnd || !global_horzlist_hwnd || !cardlistfilter_hwnd || !global_decksurface_hwnd || !popup)
    {
      if (popup)
        DestroyMenu(popup);
      if (global_smallcard_popup)
        DestroyMenu(global_smallcard_popup);
      return -1;
    }

    SendMessage(global_listbox_hwnd, LB_SETTOPINDEX, 0, 0);
    SendMessage(global_listbox_hwnd, LB_SETCURSEL, 0, 0);
    SendMessage(global_horzlist_hwnd, LB_SETCURSEL, 0, 0);

    {
      csvidraw = SendMessage(global_horzlist_hwnd, LB_GETITEMDATA, 0, 0);
      if (csvidraw == -1 && global_deck_num_entries)
        csvidraw = global_deck[0].GDE_csvid;
      SendMessage(global_fullcard_hwnd, 0x400, csvidraw, 0);
    }

    if (global_db_flags_1 & DBFLAGS_STANDALONE)
    {
      load_text("menus", "MAINMENU_STANDALONE");
      AppendMenu(popup, MF_ENABLED, RES_MAINMENU_NEWDECK, text_lines[0]);
      AppendMenu(popup, MF_ENABLED, RES_MAINMENU_LOADDECK, text_lines[1]);
      AppendMenu(popup, MF_ENABLED, RES_MAINMENU_SAVEDECK, text_lines[2]);
      AppendMenu(popup, MF_SEPARATOR, 0, 0);
      AppendMenu(popup, MF_ENABLED, RES_FULLCARDMENU_EXPAND, text_lines[3]);
      AppendMenu(popup, MF_ENABLED, RES_MENU_CONSOLIDATE, text_lines[4]);
      AppendMenu(popup, MF_SEPARATOR, 0, 0);
      AppendMenu(popup, MF_ENABLED, RES_MAINMENU_MINIMIZE, text_lines[5]);
      AppendMenu(popup, MF_ENABLED, RES_MAINMENU_MUSIC, text_lines[6]);
      AppendMenu(popup, MF_ENABLED, RES_MAINMENU_EFFECTS, text_lines[7]);
      AppendMenu(popup, MF_ENABLED, RES_MAINMENU_EXIT, text_lines[8]);
    }
    else if (global_db_flags_1 & DBFLAGS_GAUNTLET)
    {
      load_text("menus", "MAINMENU_GAUNTLET");
      AppendMenu(popup, MF_ENABLED, RES_MAINMENU_NEWDECK, text_lines[0]);
      AppendMenu(popup, MF_ENABLED, RES_MAINMENU_LOADDECK, text_lines[1]);
      AppendMenu(popup, MF_ENABLED, RES_MAINMENU_SAVEDECK, text_lines[2]);
      AppendMenu(popup, MF_SEPARATOR, 0, 0);
      AppendMenu(popup, MF_ENABLED, RES_FULLCARDMENU_EXPAND, text_lines[3]);
      AppendMenu(popup, MF_ENABLED, RES_MENU_CONSOLIDATE, text_lines[4]);
      AppendMenu(popup, MF_SEPARATOR, 0, 0);
      AppendMenu(popup, MF_ENABLED, RES_MAINMENU_MUSIC, text_lines[5]);
      AppendMenu(popup, MF_ENABLED, RES_MAINMENU_EFFECTS, text_lines[6]);
      AppendMenu(popup, MF_ENABLED, RES_MAINMENU_EXIT, text_lines[7]);
    }
    else
    {
      load_text("menus", "MAINMENU_ADVENTURE");
      AppendMenu(popup, MF_ENABLED, RES_FULLCARDMENU_EXPAND, text_lines[0]);
      AppendMenu(popup, MF_ENABLED, RES_MENU_CONSOLIDATE, text_lines[1]);
      AppendMenu(popup, MF_SEPARATOR, 0, 0);
      AppendMenu(popup, MF_ENABLED, RES_MAINMENU_EXIT, text_lines[2]);
    }

    SetFocus(global_horzlist_hwnd);
    load_text("menus", "DECKINFO");
    SetWindowText(button_deckinfo, text_lines[0]);
    change_buttonclass_wndproc(hwnd);
    refresh_numofcards_text();
    if ((global_db_flags_1 & DBFLAGS_SHANDALAR))
    {
      load_text("menus", "EXIT");
      SetWindowText(button_exit, text_lines[0]);

      load_text("menus", "DECKNUMBERS");
      SetWindowText(button_deck1, text_lines[0]);
      SetWindowText(button_deck2, text_lines[1]);
      SetWindowText(button_deck3, text_lines[2]);
      if (global_current_deck == 0)
      {
        SetWindowText(button_deck1, text_lines[3]);
      }
      else if (global_current_deck == 1)
      {
        SetWindowText(button_deck2, text_lines[4]);
      }
      else if (global_current_deck == 2)
      {
        SetWindowText(button_deck3, text_lines[5]);
      }
    }
    else
    {
      load_text("menus", "EXIT");
      SetWindowText(button_exit, text_lines[1]);
    }
    return 0;

  case WM_CTLCOLORLISTBOX:
  {
    listbox_dc = (HDC)wparam;
    ApplyCardArtPaletteToDc(listbox_dc);
    listbox_hwnd_param = (HWND)lparam;
    SetTextColor(listbox_dc, RGB(0, 0, 0));
    if (global_listbox_hwnd == listbox_hwnd_param)
      return (LRESULT)global_create_brush_3;
    else
      return (LRESULT)global_create_brush_4;
  }

  case WM_DESTROY:
    if (global_brush_mediumgrey)
      DeleteObject(global_brush_mediumgrey);
    if (pen_ltgrey)
      DeleteObject(pen_ltgrey);
    if (pen_medgrey)
      DeleteObject(pen_medgrey);

    title_font_obj = (HGDIOBJ)SendMessageA(global_title_hwnd, 0x31, 0, 0);
    SendMessageA(global_title_hwnd, 0x30, 0, 0);
    if (title_font_obj)
      DeleteObject(title_font_obj);

    if (global_card_amount_font)
    {
      DeleteObject(global_card_amount_font);
    }
    global_card_amount_font = 0;

    if (popup)
      DestroyMenu(popup);
    if (global_smallcard_popup)
      DestroyMenu(global_smallcard_popup);
    if (mainclass_destroy_obj)
      DeleteObject(mainclass_destroy_obj);
    if (!(global_db_flags_1 & DBFLAGS_SHANDALAR))
      free_sounds_and_music();
    if (check_filters_debug_log_file != NULL)
    {
      fclose(check_filters_debug_log_file);
      check_filters_debug_log_file = NULL;
    }
    PostQuitMessage(0);
    return 0;

  case WM_DISPLAYCHANGE:
    global_color_depth = (int)wparam;
    SelectObject(global_hdc, global_old_bmp_obj);
    DeleteObject(global_hbmp);

    display_dc = GetDC(hwnd);
    ApplyCardArtPaletteToDc(display_dc);

    global_hbmp = CreateCompatibleBitmap(display_dc, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));

    ReleaseDC(hwnd, display_dc);

    SelectObject(global_hdc, global_hbmp);

    if (!global_hbmp)
    {
      MessageBox(hwnd,
                 ("Not enough system memory to run at this screen resolution.\n"
                  "\n"
                  "Change it back to a smaller size before continuing (the game will hide itself until then)."),
                 "Magic: The Gathering",
                 MB_ICONWARNING);
      ShowWindow(hwnd, SW_HIDE);
    }
    else
      ShowWindow(hwnd, SW_SHOW);

    MoveWindow(hwnd, 0, 0, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), TRUE);
    return 0;

  case 0x4c8:
  {
    clicked_button = (HWND)wparam;
    invalidate_hwnd = (HWND)lparam;
    if (GetDlgItem(hwnd, RES_MAINMENU_BUTTON_STATS) == clicked_button || GetDlgItem(hwnd, RES_MAINMENU_BUTTON_DECKINFO) == clicked_button || GetDlgItem(hwnd, RES_MAINMENU_BUTTON_EXIT) == clicked_button || GetDlgItem(hwnd, RES_MAINMENU_BUTTON_DECK_1) == clicked_button || GetDlgItem(hwnd, RES_MAINMENU_BUTTON_DECK_2) == clicked_button || GetDlgItem(hwnd, RES_MAINMENU_BUTTON_DECK_3) == clicked_button)
      SendMessage(hwnd, 0x401, GetDlgCtrlID(clicked_button), 0);
    else
      SendMessage(hwnd, 0x401, 1, 0);

    if (clicked_button)
      InvalidateRect(clicked_button, NULL, TRUE);

    if (invalidate_hwnd)
      InvalidateRect(invalidate_hwnd, NULL, TRUE);

    return 0;
  }

  case WM_CTLCOLORBTN:
  case WM_CTLCOLORSTATIC:
  {
    color_dc = (HDC)wparam;
    ApplyCardArtPaletteToDc(color_dc);
    color_target = (HWND)lparam;
    color_ctrl_id = GetDlgCtrlID(color_target);
    if (GetFocus() == color_target)
      SetTextColor(color_dc, global_colorref_white);
    else
      SetTextColor(color_dc, global_colorref_darkgrey);
    SetBkMode(color_dc, TRANSPARENT);
    ctlcolor_brush = (HBRUSH)GetStockObject(HOLLOW_BRUSH);
    return (LRESULT)ctlcolor_brush;
  }

  case WM_DRAWITEM:
  {
    draw_item = (DRAWITEMSTRUCT *)lparam;

    if (GetFocus() == draw_item->hwndItem)
      drawitem_fill_color = global_colorref_white;
    else
    {
      if (draw_item->itemState & 2)
        drawitem_fill_color = 0x10000c6;
      else
        drawitem_fill_color = global_colorref_flesh;
    }

    draw_ownerdraw_button_item(draw_item, global_brush_mediumgrey, pen_ltgrey, pen_medgrey, drawitem_fill_color, 0);
    return 1;
  }

  case WM_ERASEBKGND:
    erase_dc = (HDC)wparam;
    ApplyCardArtPaletteToDc(erase_dc);
    tile_dc = CreateCompatibleDC(erase_dc);
    ApplyCardArtPaletteToDc(tile_dc);
    SelectObject(tile_dc, global_pic_dektile4);

    GetClientRect(hwnd, &r);
    GetObjectA(global_pic_dektile4, sizeof(BITMAP), &tile_bitmap);

    for (tile_x = 0; r.right > tile_x; tile_x += 0x20)
      for (tile_y = r.bottom; tile_y > -0x1d; tile_y -= 0x1d)
        BitBlt(erase_dc, tile_x, tile_y, tile_bitmap.bmWidth, tile_bitmap.bmHeight, tile_dc, 0, 0, SRCCOPY);

    DeleteDC(tile_dc);
    return 1;

  case WM_INITMENU:
    CHECKMENU_IF(popup, RES_FULLCARDMENU_EXPAND, SendMessage(global_fullcard_hwnd, 0x402, 0, 0));
    CHECKMENU_IF(popup, RES_MAINMENU_EFFECTS, global_cfg.effects);
    CHECKMENU_IF(popup, RES_MAINMENU_MUSIC, global_cfg.music);
    CHECKMENU_IF(popup, RES_MENU_CONSOLIDATE, global_cfg_consolidate);
    return 0;

  case WM_MEASUREITEM:
  {
    measure_item = (MEASUREITEMSTRUCT *)lparam;
    measure_hwnd = GetDlgItem(hwnd, wparam);
    GetClientRect(measure_hwnd, &measure_client_rect);

    if (measure_item->CtlID == 4)
    {
      measure_item->itemWidth = measure_client_rect.right;
      measure_item->itemHeight = measure_client_rect.bottom / 8;
    }
    return 0;
  }

  case WM_RBUTTONDOWN:
  {
    pad_a90.x = GET_X_LPARAM(lparam);
    pad_a90.y = GET_Y_LPARAM(lparam);
    ClientToScreen(hwnd, &pad_a90);
    TrackPopupMenu(popup, TPM_RIGHTBUTTON, pad_a90.x, pad_a90.y, 0, hwnd, NULL);
    return 0;
  }

  case WM_SETFOCUS:
    SetFocus(global_horzlist_hwnd);
    return 0;

  case WM_SIZE:
  {
    main_w = (int)((unsigned int)lparam & 0xffff);
    main_h = (int)(((unsigned int)lparam >> 16) & 0xffff);
    // global_main_window_width = main_w;
    // global_main_window_height = main_h;

    LockWindowUpdate(hwnd);

    margin8 = 8;
    ten = 0xa;

    third_w = main_w / 3 - 0x35;
    fullcard_height = ((main_h - ten * 4) * 64) / 100 - 0xd;
    horzlist_hgt6 = (main_h - ten * 4) * 11 * 2 / 100;
    title_height = 10 * (main_h - ten * 4) / 100;
    cardlistfilter_hgt = 6 * (main_h - ten * 4) / 100;

    pad_b0c = GetDC(hwnd);
    ApplyCardArtPaletteToDc(pad_b0c);
    GetTextExtentPointA(pad_b0c, "Load new deck", 0xd, &sz);
    sz.cx += 0xa;
    sz.cy += sz.cy / 2;
    ReleaseDC(hwnd, pad_b0c);

    SetRect(&rect_title,
            margin8, ten,
            third_w + margin8, title_height + ten);

    SetRect(&rect_fullcard,
            margin8, rect_title.bottom + ten,
            third_w + margin8,
            (rect_title.bottom - ten) + fullcard_height);

    SetRect(&rect_horzlist,
            margin8, (main_h - ten) - horzlist_hgt6,
            main_w - margin8, main_h - ten);

    SetRect(&rect_cardlistfilter,
            rect_horzlist.left, rect_horzlist.top - cardlistfilter_hgt,
            rect_horzlist.right, rect_horzlist.top);

    SetRect(&rect_load_button,
            main_w - margin8 - sz.cx,
            ten,
            main_w - margin8,
            sz.cy * 2 + ten / 2 + ten);

    SetRect(&rect_decksurface,
            rect_fullcard.right + margin8, 0,
            main_w, rect_fullcard.bottom);

    MoveWindow(global_title_hwnd,
               rect_title.left, rect_title.top,
               rect_title.right - rect_title.left,
               rect_title.bottom - rect_title.top,
               TRUE);

    MoveWindow(global_fullcard_hwnd,
               rect_fullcard.left, rect_fullcard.top,
               rect_fullcard.right - rect_fullcard.left,
               rect_fullcard.bottom - rect_fullcard.top,
               TRUE);

    decksurface_top = rect_decksurface.top;
    decksurface_x = rect_decksurface.left;

    MoveWindow(global_decksurface_hwnd,
               rect_decksurface.left, rect_decksurface.top,
               rect_decksurface.right - rect_decksurface.left,
               rect_decksurface.bottom - rect_decksurface.top,
               TRUE);

    horzlist_style = GetWindowLongA(global_horzlist_hwnd, GWL_STYLE);
    if (horzlist_style & WS_BORDER)
      border_adjust = GetSystemMetrics(SM_CXBORDER) * 2;
    else
      border_adjust = 0;

    rect_horzlist.top = rect_horzlist.bottom - global_smallcard_piclist_height - 8 -
                        GetSystemMetrics(SM_CYHSCROLL) - border_adjust;

    rect_cardlistfilter.bottom = rect_horzlist.top;
    rect_cardlistfilter.top = rect_cardlistfilter.bottom - cardlistfilter_hgt;

    MoveWindow(cardlistfilter_hwnd,
               rect_cardlistfilter.left, rect_cardlistfilter.top,
               rect_cardlistfilter.right - rect_cardlistfilter.left,
               rect_cardlistfilter.bottom - rect_cardlistfilter.top,
               TRUE);

    MoveWindow(global_horzlist_hwnd,
               rect_horzlist.left, rect_horzlist.top,
               rect_horzlist.right - rect_horzlist.left,
               rect_horzlist.bottom - rect_horzlist.top,
               TRUE);

    // TODO : huh?
    if (false)
    {
      rect_fullcard.bottom = rect_cardlistfilter.top + 4;
      rect_decksurface.bottom = rect_cardlistfilter.top + 4;

      MoveWindow(global_fullcard_hwnd,
                 rect_fullcard.left, rect_fullcard.top,
                 rect_fullcard.right - rect_fullcard.left,
                 rect_fullcard.bottom - rect_fullcard.top,
                 TRUE);

      decksurface_top = rect_decksurface.top + GetSystemMetrics(SM_CYCAPTION) * 2;
      decksurface_x = rect_decksurface.left;

      MoveWindow(global_decksurface_hwnd,
                 decksurface_x, decksurface_top,
                 rect_decksurface.right - decksurface_x,
                 rect_decksurface.bottom - decksurface_top,
                 TRUE);

      decksurface_x += 0x2a;
      decksurface_top -= GetSystemMetrics(SM_CYCAPTION);
      MoveWindow(global_unknown_10125a3c,
                 decksurface_x, decksurface_top,
                 rect_decksurface.right - decksurface_x,
                 rect_decksurface.bottom - decksurface_top,
                 TRUE);

      decksurface_x += 0x2a;
      decksurface_top -= GetSystemMetrics(SM_CYCAPTION);
      MoveWindow(global_unknown_10144b04,
                 decksurface_x, decksurface_top,
                 rect_decksurface.right - decksurface_x,
                 rect_decksurface.bottom - decksurface_top,
                 TRUE);
    }
    SetRect(&rect_pad_adc,
            rect_decksurface.left, rect_decksurface.bottom,
            (rect_decksurface.right - rect_decksurface.left) / 3,
            rect_cardlistfilter.top - rect_decksurface.bottom - 3);

    MoveWindow(global_button_stats_hwnd,
               rect_pad_adc.left, rect_pad_adc.top,
               rect_pad_adc.right, rect_pad_adc.bottom,
               TRUE);

    MoveWindow(button_exit,
               rect_pad_adc.left + rect_pad_adc.right * 2, rect_pad_adc.top,
               rect_pad_adc.right, rect_pad_adc.bottom,
               TRUE);

    if (global_db_flags_1 & DBFLAGS_SHANDALAR)
    {
      MoveWindow(button_deck1,
                 rect_pad_adc.left + rect_pad_adc.right, rect_pad_adc.top,
                 rect_pad_adc.right / 3, rect_pad_adc.bottom,
                 TRUE);
      MoveWindow(button_deck2,
                 rect_pad_adc.left + rect_pad_adc.right + rect_pad_adc.right / 3, rect_pad_adc.top,
                 rect_pad_adc.right / 3, rect_pad_adc.bottom,
                 TRUE);
      MoveWindow(button_deck3,
                 rect_pad_adc.left + rect_pad_adc.right + 2 * rect_pad_adc.right / 3, rect_pad_adc.top,
                 rect_pad_adc.right / 3, rect_pad_adc.bottom,
                 TRUE);
    }
    else
      MoveWindow(button_deckinfo,
                 rect_pad_adc.left + rect_pad_adc.right, rect_pad_adc.top,
                 rect_pad_adc.right, rect_pad_adc.bottom,
                 TRUE);

    SendMessageA(global_horzlist_hwnd, 0x1a0, 0, (LPARAM)(unsigned short)global_smallcard_piclist_width);
    SendMessage(cardlistfilter_hwnd, 0x402,
                MAKELONG(rect_cardlistfilter.left, rect_cardlistfilter.right),
                MAKELONG(rect_cardlistfilter.top, rect_cardlistfilter.bottom));

    wLast = GetWindowWord(global_horzlist_hwnd, HORZLIST_LASTPIC_INDEX);
    horz_list_addr = (int *)GetWindowLongA(global_horzlist_hwnd, HORZLIST_ADDR_INDEX);
    wLeft = GetWindowWord(global_horzlist_hwnd, HORZLIST_LEFTPIC_INDEX);
    wPicsInline = GetWindowWord(global_horzlist_hwnd, HORZLIST_PICSINLINE_INDEX);

    for (artist_i = wLeft;
         MIN(wPicsInline + wLeft, wLast) > (int)artist_i;
         ++artist_i)
      NoopProcessHorzListCard(global_horzlist_hwnd, horz_list_addr[artist_i]);

    BringWindowToTop(global_title_hwnd);
    BringWindowToTop(cardlistfilter_hwnd);
    BringWindowToTop(global_horzlist_hwnd);
    BringWindowToTop(global_button_stats_hwnd);
    BringWindowToTop(button_deckinfo);
    BringWindowToTop(button_exit);
    LockWindowUpdate(0);

    if (!(global_db_flags_1 & DBFLAGS_SHANDALAR) && global_cfg.music)
      play_music(1, 400, 0);

    return 0;
  }

  case WM_QUERYENDSESSION:
    if (!global_deck_was_edited || ask_about_saving_deck() != 2)
      return 1;

    return 0;

  default:
    return DefWindowProcA(hwnd, msg, wparam, lparam);
  }

  return 0;
}
