#include <stddef.h>
#include <windows.h>
#include "mystdbool.h"
#include "defs.h"
#include "deckdll.h"
#include "full_deck.h"
#include "resources.h"
#include "cardartlib/src/palette.h"
#include "drawcardlib/src/pic.h"

typedef ptrdiff_t INT_PTR;

extern char text_lines[500][128];

extern FullDeck global_edited_deck;
#define global_edited_deck_num_entries (global_edited_deck.total)

extern GlobalDeckInfoBlob global_deckinfo;
;
extern const card_ptr_t *cards_ptr;
extern card_ptr_t global_raw_cards_storage[2000];

extern COLORREF global_colorref_darkgrey;
extern COLORREF global_colorref_flesh;
extern COLORREF global_colorref_lavender;
extern COLORREF global_colorref_lightgrey;
extern COLORREF global_colorref_white;

extern HFONT global_font_32;
extern char global_base_directory[];
extern char global_duelart_path[];
extern HANDLE global_pic_statbak1;

bool check_restricted(csvid_t csvid);
bool check_banned(csvid_t csvid);
bool check_ante(csvid_t csvid);
bool check_basic(csvid_t csvid);
int load_text(const char *file_name, const char *section_name);

typedef enum
{
  DT_UNKNOWN = 0,
  DT_UNRESTRICTED = 0x1,
  DT_WILD = 0x2,
  DT_RESTRICTED_T1 = 0x4,
  DT_TOURNAMENT_T1_5 = 0x8,
  DT_HIGHLANDER = 0x10,
  DT_HAS_ANTE = 0x20,
} DeckType;

#define DASH "\227"

// FUNCTION: DECKDLL 0x1002f0ec
static int
check_deck_type(void)
{
  int all_unique_nonbasics;
  int i;
  int rval;
  int num;
  int csvid;

  rval = DT_UNKNOWN;
  all_unique_nonbasics = 1;

  for (i = 0; i < global_edited_deck_num_entries; ++i)
    if (!check_basic(global_edited_deck.entries[i].DeckEntry_csvid))
    {
      if (global_edited_deck.entries[i].DeckEntry_Amount > 1)
        all_unique_nonbasics = 0;
      if (check_ante(global_edited_deck.entries[i].DeckEntry_csvid))
        rval |= DT_HAS_ANTE;
    }

  if (all_unique_nonbasics)
    return rval | DT_HIGHLANDER;

  for (i = 0; i < global_edited_deck_num_entries; ++i)
  {
    csvid = global_edited_deck.entries[i].DeckEntry_csvid;
    num = global_edited_deck.entries[i].DeckEntry_Amount;
    if (!check_basic(csvid))
    {
      if (check_banned(csvid))
        rval |= DT_UNRESTRICTED;
      else if (check_restricted(csvid))
      {
        if (num > 4)
          rval |= DT_UNRESTRICTED;
        else if (num > 1)
          rval |= DT_WILD;
        else
          rval |= DT_RESTRICTED_T1;
      }
      else if (num > 4)
        rval |= DT_UNRESTRICTED;
      else
        rval |= DT_TOURNAMENT_T1_5;
    }
  }

  return rval;
}

// GLOBAL: DECKDLL 0x101053a8
static int global_deck_stats[9][7];

// GLOBAL: DECKDLL 0x10105368
static LOGFONT global_stats_font_desc;

// GLOBAL: DECKDLL 0x101054a4
static HFONT global_stats_font;

// GLOBAL: DECKDLL 0x101054a8
static HANDLE global_stats_pic;

// GLOBAL: DECKDLL 0x101054ac
static COLORREF global_colorref_stats_lightgrey;

// GLOBAL: DECKDLL 0x101054b0
static COLORREF global_colorref_stats_lavender;

// GLOBAL: DECKDLL 0x101054b4
static COLORREF global_colorref_stats_flesh;

// GLOBAL: DECKDLL 0x1003a878
static const LOGFONT stats_logfont_template = {
    0,         /* lfHeight */
    0,         /* lfWidth */
    0,         /* lfEscapement */
    0,         /* lfOrientation */
    FW_NORMAL, /* lfWeight (0x190) */
    0,         /* lfItalic */
    0,         /* lfUnderline */
    0,         /* lfStrikeOut */
    0,         /* lfCharSet */
    0,         /* lfOutPrecision */
    0,         /* lfClipPrecision */
    0,         /* lfQuality */
    0,         /* lfPitchAndFamily */
    {0}        /* lfFaceName */
};

// FUNCTION: DECKDLL 0x1002e9ee
static void
build_deck_stats_table(void)
{
  int j;
  int i;
  int col;
  int amt;
  int num_five_color_lands;
  int row;
  int csvid;

  num_five_color_lands = 0;
  for (j = 0; j < 9; ++j)
    for (i = 0; i < 7; ++i)
      global_deck_stats[j][i] = 0;

  for (i = 0; i < global_edited_deck_num_entries; ++i)
  {
    csvid = global_edited_deck.entries[i].DeckEntry_csvid;
    amt = global_edited_deck.entries[i].DeckEntry_Amount;

    if (global_raw_cards_storage[csvid].card_type == CP_TYPE_CREATURE)
      row = 1;
    else if (global_raw_cards_storage[csvid].card_type == CP_TYPE_ENCHANTMENT)
      row = 2;
    else if (global_raw_cards_storage[csvid].card_type == CP_TYPE_SORCERY)
      row = 3;
    else if (global_raw_cards_storage[csvid].card_type == CP_TYPE_INSTANT)
      row = 4;
    else if (global_raw_cards_storage[csvid].card_type == CP_TYPE_INTERRUPT)
      row = 5;
    else if (global_raw_cards_storage[csvid].card_type == CP_TYPE_ARTIFACT)
      row = 7;
    else if (global_raw_cards_storage[csvid].card_type == CP_TYPE_LAND)
      row = 6;
    else
      continue;

    if (global_raw_cards_storage[csvid].color == CP_COLOR_BLACK)
      col = 0;
    else if (global_raw_cards_storage[csvid].color == CP_COLOR_BLUE)
      col = 1;
    else if (global_raw_cards_storage[csvid].color == CP_COLOR_GREEN)
      col = 2;
    else if (global_raw_cards_storage[csvid].color == CP_COLOR_RED)
      col = 3;
    else if (global_raw_cards_storage[csvid].color == CP_COLOR_WHITE)
      col = 4;
    else
      col = -1;

    if (row == 6)
    {
      if (csvid == 0xa4)
        col = 3;
      else if (csvid == 0x5b)
        col = 2;
      else if (csvid == 0x7e)
        col = 1;
      else if (csvid == 0xbc)
        col = 4;
      else if (csvid == 0xef)
        col = 0;
      else
        col = 5;
    }

    if (row == 7)
    {
      col = 5;
      if (global_raw_cards_storage[csvid].subtype == 0x2c)
        row = 1;
    }

    if (col == -1)
    {
      MessageBox(NULL, "Color is -1", "Stats Error", 0);
      continue;
    }

    if (global_raw_cards_storage[csvid].db_card_type_2 == 10)
    {
      if (csvid == 0xf || csvid == 0x11 || csvid == 0x193 || csvid == 0x1b9)
      {
        num_five_color_lands += amt;
        global_deck_stats[0][0] += amt;
        global_deck_stats[0][1] += amt;
        global_deck_stats[0][2] += amt;
        global_deck_stats[0][3] += amt;
        global_deck_stats[0][4] += amt;
      }
      else if (csvid == 0xa5)
        global_deck_stats[0][2] += amt;
      else if (csvid == 0xa6)
        global_deck_stats[0][0] += amt;
      else if (csvid == 0xa7)
        global_deck_stats[0][4] += amt;
      else if (csvid == 0xa8)
        global_deck_stats[0][3] += amt;
      else if (csvid == 0xa9)
        global_deck_stats[0][1] += amt;
      else if (csvid == 9)
      {
        global_deck_stats[0][0] += amt;
        global_deck_stats[0][3] += amt;
      }
      else if (csvid == 0xc)
      {
        global_deck_stats[0][0] += amt;
        global_deck_stats[0][2] += amt;
      }
      else if (csvid == 0xbd)
      {
        global_deck_stats[0][3] += amt;
        global_deck_stats[0][4] += amt;
      }
      else if (csvid == 0xd4)
      {
        global_deck_stats[0][4] += amt;
        global_deck_stats[0][2] += amt;
      }
      else if (csvid == 0xd8)
      {
        global_deck_stats[0][4] += amt;
        global_deck_stats[0][0] += amt;
      }
      else if (csvid == 0xf1)
      {
        global_deck_stats[0][3] += amt;
        global_deck_stats[0][2] += amt;
      }
      else if (csvid == 0xfc)
      {
        global_deck_stats[0][1] += amt;
        global_deck_stats[0][2] += amt;
      }
      else if (csvid == 0xfe)
      {
        global_deck_stats[0][4] += amt;
        global_deck_stats[0][1] += amt;
      }
      else if (csvid == 0x102)
      {
        global_deck_stats[0][1] += amt;
        global_deck_stats[0][0] += amt;
      }
      else if (csvid == 0x10a)
      {
        global_deck_stats[0][1] += amt;
        global_deck_stats[0][3] += amt;
      }
      else
        global_deck_stats[0][col] += amt;
    }

    global_deck_stats[row][col] += amt;
  }

  for (j = 0; j < 8; ++j)
    for (i = 0; i < 6; ++i)
      global_deck_stats[j][6] += global_deck_stats[j][i];

  global_deck_stats[0][6] -= num_five_color_lands * 4;

  for (i = 0; i < 6; ++i)
    for (j = 0; j < 8; ++j)
      if (j != 0)
        global_deck_stats[8][i] += global_deck_stats[j][i];

  for (i = 0; i < 8; ++i)
    if (i != 0)
      global_deck_stats[8][6] += global_deck_stats[i][6];
}

// FUNCTION: DECKDLL 0x1002e3fd
static void
show_stats(HDC hdc, SIZE word_size, int *stepx, int *stepy, int *posx, int *starty)
{
  char buf[264];
  int y;
  int x;

  load_text("menus", "STATSSCREEN");
  SetTextColor(hdc, global_colorref_stats_lightgrey);

  x = 20;
  y = 20;
  wsprintf(buf, text_lines[0]);
  TextOut(hdc, x, y, buf, strlen(buf));

  x += word_size.cx;
  *stepx = (1000 - x) / 8;
  x += 0x50;
  *posx = x;

  wsprintf(buf, text_lines[1]);
  TextOut(hdc, x, y, buf, strlen(buf));
  x += *stepx;
  wsprintf(buf, text_lines[2]);
  TextOut(hdc, x, y, buf, strlen(buf));
  x += *stepx;
  wsprintf(buf, text_lines[3]);
  TextOut(hdc, x, y, buf, strlen(buf));
  x += *stepx;
  wsprintf(buf, text_lines[4]);
  TextOut(hdc, x, y, buf, strlen(buf));
  x += *stepx;
  wsprintf(buf, text_lines[5]);
  TextOut(hdc, x, y, buf, strlen(buf));
  x += *stepx;
  wsprintf(buf, text_lines[6]);
  TextOut(hdc, x, y, buf, strlen(buf));
  x += *stepx;
  wsprintf(buf, text_lines[7]);
  TextOut(hdc, x, y, buf, strlen(buf));

  *stepy = 65;
  *starty = *stepy + 20;
  SetTextColor(hdc, global_colorref_stats_lavender);
  x = 20;
  y = *starty;
  wsprintf(buf, text_lines[8]);
  TextOut(hdc, x, y, buf, strlen(buf));

  SetTextColor(hdc, global_colorref_stats_lightgrey);
  y += word_size.cy / 2;
  y += *stepy;
  wsprintf(buf, text_lines[9]);
  TextOut(hdc, x, y, buf, strlen(buf));
  y += *stepy;
  wsprintf(buf, text_lines[10]);
  TextOut(hdc, x, y, buf, strlen(buf));
  y += *stepy;
  wsprintf(buf, text_lines[11]);
  TextOut(hdc, x, y, buf, strlen(buf));
  y += *stepy;
  wsprintf(buf, text_lines[12]);
  TextOut(hdc, x, y, buf, strlen(buf));
  y += *stepy;
  wsprintf(buf, text_lines[13]);
  TextOut(hdc, x, y, buf, strlen(buf));
  y += *stepy;
  wsprintf(buf, text_lines[14]);
  TextOut(hdc, x, y, buf, strlen(buf));
  y += *stepy;
  wsprintf(buf, text_lines[15]);
  TextOut(hdc, x, y, buf, strlen(buf));
  //  y += *stepy;
  y += word_size.cy;
  wsprintf(buf, text_lines[16]);
  TextOut(hdc, x, y, buf, strlen(buf));
  y += *stepy;
  wsprintf(buf, text_lines[7]);
  TextOut(hdc, x, y, buf, strlen(buf));
}

// FUNCTION: DECKDLL 0x1002e096
static void
show_stats_values(HDC hdc, SIZE word_size, int stepx, int stepy, int posx, int starty)
{
  char buf[264]; // ebp - 0x118
  int l;         // ebp - 0x10
  int y;         // ebp - 0xc
  int c;         // ebp - 8
  int x;         // ebp - 4

  y = starty;
  SetTextColor(hdc, global_colorref_stats_flesh);
  for (l = 0; l < 9; ++l)
  {
    x = posx;
    if (l == 1)
      y += word_size.cy / 2;

    for (c = 0; c < 7; ++c)
    {
      if (l == 0)
        SetTextColor(hdc, global_colorref_stats_lavender);
      else
        SetTextColor(hdc, global_colorref_stats_flesh);

      if (global_deck_stats[l][c] != 0)
      {
        if (l == 8 || c == 6)
        {
          int percent;
          if (global_deck_stats[l][c] == 0 || global_deck_stats[8][6] == 0)
            percent = 0;
          else
            percent = (global_deck_stats[l][c] * 100 + (global_deck_stats[8][6] >> 1)) / global_deck_stats[8][6];

          wsprintf(buf, "(%d) %d%%", global_deck_stats[l][c], percent);
        }
        else
        {
          int percent;
          if (global_deck_stats[l][c] == 0 || global_deck_stats[l][6] == 0)
            percent = 0;
          else
            percent = (global_deck_stats[l][c] * 100 + (global_deck_stats[l][6] >> 1)) / global_deck_stats[l][6];

          wsprintf(buf, "(%d) %d%%", global_deck_stats[l][c], percent);
        }
      }
      else
      {
        int percent;
        if (global_deck_stats[l][c] == 0 || global_deck_stats[l][6] == 0)
          percent = 0;
        else
          percent = (global_deck_stats[l][c] * 100 + (global_deck_stats[l][6] >> 1)) / global_deck_stats[l][6];

        wsprintf(buf, "  -", global_deck_stats[l][c], percent);
      }

      TextOut(hdc, x, y, buf, strlen(buf));
      x += stepx;
    }

    if (l != 7)
      y += stepy;
    else
      y += word_size.cy + stepy;
  }
}

// FUNCTION: DECKDLL 0x1002dfd0
static void
fill_stats_window(HDC hdc, LONG left, LONG top, LONG right, LONG bottom, HFONT font)
{
  int posx;  // ebp - 0x18
  int stepy; // ebp - 0x14
  SIZE sz;
  int starty; // ebp - 0x8
  int stepx;  // ebp - 0x4

  build_deck_stats_table();
  SetMapMode(hdc, MM_ANISOTROPIC);
  SetWindowExtEx(hdc, 1000, 750, NULL);
  SetViewportExtEx(hdc, right - left, bottom - top, NULL);
  SelectObject(hdc, font);
  SetBkMode(hdc, TRANSPARENT);

  GetTextExtentPoint32(hdc, "Enchantments", 0xc, &sz);

  show_stats(hdc, sz, &stepx, &stepy, &posx, &starty);
  show_stats_values(hdc, sz, stepx, stepy, posx, starty);
}

// FUNCTION: DECKDLL 0x1002db30
INT_PTR CALLBACK
dlgproc_DeckStats(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
  /* Stack layout is extremely sensitive; keep locals grouped. */
  HDC hdc_erase;         /* [ebp-0x150] */
  RECT client;           /* [ebp-0x14c] */
  LPARAM lparam_copy;    /* [ebp-0x13c] */
  HDC hdc_ctl;           /* [ebp-0x138] */
  HGDIOBJ brush;         /* [ebp-0x134] */
  HDC chdc;              /* [ebp-0x130] */
  char txt[264];         /* [ebp-0x12c] */
  BITMAP bmp;            /* [ebp-0x24] */
  int y;                 /* [ebp-0x0c] */
  int x;                 /* [ebp-0x08] */
  unsigned int decktype; /* [ebp-0x04] */

  switch (msg)
  {
  case WM_INITDIALOG:
    load_text("menus", "STATSDIALOG");
    strcpy(txt, text_lines[0]);
    strcat(txt, ": ");
    strcat(txt, global_deckinfo.deckname);
    strcat(txt, "  - ");
    decktype = check_deck_type();
    load_text("MP_UIStrings.txt", "SHELLPAGE_MULTIDUEL");
    if (decktype & DT_UNRESTRICTED)
      strcat(txt, text_lines[1]);
    else if (decktype & DT_WILD)
      strcat(txt, text_lines[2]);
    else if (decktype & DT_RESTRICTED_T1)
      strcat(txt, text_lines[3]);
    else if (decktype & DT_TOURNAMENT_T1_5)
      strcat(txt, text_lines[4]);
    else if (decktype & DT_HIGHLANDER)
      strcat(txt, text_lines[5]);
    else
      strcat(txt, "Unknown");

    if (decktype & DT_HAS_ANTE)
    {
      strcat(txt, " / ");
      strcat(txt, text_lines[7] + 1);
    }

    SetWindowTextA(hdlg, txt);
    sprintf(txt, "%s\\GAUN_Options.pic", global_duelart_path);
    global_stats_pic = (HANDLE)load_pic(txt);
    memcpy(&global_stats_font_desc, &stats_logfont_template, sizeof(LOGFONT));
    global_stats_font_desc.lfHeight = 30;
    strcpy(global_stats_font_desc.lfFaceName, "Cheltenham ITC Bold BT");
    global_stats_font = CreateFontIndirectA(&global_stats_font_desc);
    global_colorref_stats_lavender = 0x02efb2ae;
    global_colorref_stats_lightgrey = 0x02f6f7f7;
    global_colorref_stats_flesh = 0x028fb0cd;
    return 0;

  case WM_COMMAND:
    if (((((unsigned int)wparam) & 0xffff) == RES_BUTTON_OK) || ((((unsigned int)wparam) & 0xffff) == RES_BUTTON_CANCEL))
    {
      DeleteObject(global_stats_font);
      if (global_stats_pic)
        delete_and_close_object(global_stats_pic);
      EndDialog(hdlg, 0);
    }
    return 1;

  case WM_CTLCOLORBTN:
  case WM_CTLCOLORSTATIC:
    hdc_ctl = (HDC)wparam;
    ApplyCardArtPaletteToDc(hdc_ctl);
    lparam_copy = lparam;
    SetBkMode(hdc_ctl, TRANSPARENT);
    brush = GetStockObject(HOLLOW_BRUSH);
    return (INT_PTR)brush;

  case WM_ERASEBKGND:
    hdc_erase = (HDC)wparam;
    ApplyCardArtPaletteToDc(hdc_erase);
    GetClientRect(hdlg, &client);
    chdc = CreateCompatibleDC(hdc_erase);
    ApplyCardArtPaletteToDc(chdc);
    SelectObject(chdc, (HGDIOBJ)global_pic_statbak1);
    GetObjectA((HGDIOBJ)global_pic_statbak1, sizeof(BITMAP), &bmp);
    for (x = 0; client.right > x; x = x + bmp.bmWidth)
      for (y = 0; client.bottom > y; y = y + bmp.bmHeight)
        BitBlt(hdc_erase, x, y, bmp.bmWidth, bmp.bmHeight, chdc, 0, 0, SRCCOPY);

    fill_stats_window(hdc_erase, client.left, client.top, client.right, client.bottom, global_stats_font);
    DeleteDC(chdc);
    return 1;

  default:
    return 0;
  }
}
