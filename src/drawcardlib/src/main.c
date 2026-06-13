#include "platform/gdi_compat.h"
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "platform/legacy_io.h"
#include "mystdbool.h"
#include "defs.h"
#include "pcxw.h"
#include "pic.h"
#include "cardartlib/CardArtLib.h"
#include "cardartlib/src/assert.h"
#include "cardartlib/src/palette.h"
#include "cardartlib/src/catalog.h"
#include "cardartlib/src/haar.h"

/* Function prototypes (auto-generated). */
#include "autoproto.h"

#pragma intrinsic(memset)

typedef unsigned short ushort;
typedef BOOL(WINAPI code)(HINSTANCE, DWORD, LPVOID);

#define CONCAT12(x, y) ((undefined4)(((undefined1)(x)) | ((undefined4)(undefined2)(y) << 8)))

// GLOBAL: DRAWCARDLIB 0x10021fe0
LOGFONTA global_template_font = {0, 0, 0, 0, 400};

// GLOBAL: DRAWCARDLIB 0x1003a048
LOGFONTA global_temp_font;

// GLOBAL: DRAWCARDLIB 0x1003a090
char global_dual_dat_filename[260];

// GLOBAL: DRAWCARDLIB 0x100f36c0
char global_cardart_directory[0x105];

// GLOBAL: DRAWCARDLIB 0x100223ac
undefined4 g_emptyStringStorage = 0;

// GLOBAL: DRAWCARDLIB 0x10022504
undefined4 global_dwPlatformId = 0xFFFFFFFF;

// GLOBAL: DRAWCARDLIB 0x10039fc8
HBITMAP global_CARDBK_LegendsLand;

// GLOBAL: DRAWCARDLIB 0x10039fcc
HBITMAP global_CARDBK_Green;

// GLOBAL: DRAWCARDLIB 0x10039fd0
HBITMAP global_CARDBK_FallenEmpiresLand;

// GLOBAL: DRAWCARDLIB 0x10039fd4
HBITMAP global_CARDBK_White;

// GLOBAL: DRAWCARDLIB 0x10039fd8
HBITMAP global_CARDBK_GreenLand;

// GLOBAL: DRAWCARDLIB 0x10039fdc
HFONT g_bigCardSubtitleFont;

// GLOBAL: DRAWCARDLIB 0x10039fe0
COLORREF g_smallCardTitleColor;

// GLOBAL: DRAWCARDLIB 0x10039fe4
HBITMAP global_CardSets;

// GLOBAL: DRAWCARDLIB 0x10039fe8
HBITMAP global_CARDBK_BlackLand;

// GLOBAL: DRAWCARDLIB 0x10039fec
COLORREF g_cardTextShadowColor;

// GLOBAL: DRAWCARDLIB 0x10039ff0
HBITMAP global_CARDBK_WhiteLand;

// GLOBAL: DRAWCARDLIB 0x10039ff4
HBITMAP global_CARDBK_Gold;

// GLOBAL: DRAWCARDLIB 0x10039ff8
HFONT g_bigCardPowerToughnessFont;

// GLOBAL: DRAWCARDLIB 0x10039ffc
COLORREF g_defaultCardFrameColor;

// GLOBAL: DRAWCARDLIB 0x1003a000
HFONT g_bigCardTextFont;

// GLOBAL: DRAWCARDLIB 0x1003a004
HBITMAP global_CARDBK_BlueLand;

// GLOBAL: DRAWCARDLIB 0x1003a008
HFONT g_bigCardTitleFont;

// GLOBAL: DRAWCARDLIB 0x1003a00c
HBITMAP global_CARDBK_RedLand;

// GLOBAL: DRAWCARDLIB 0x1003a010
HBITMAP global_CardBack;

// GLOBAL: DRAWCARDLIB 0x1003a014
HBITMAP global_CARDBK_DarklandsLand;

// GLOBAL: DRAWCARDLIB 0x1003a018
HFONT g_smallCardTitleFont;

// GLOBAL: DRAWCARDLIB 0x1003a01c
undefined4 g_cardBorderPen;

// GLOBAL: DRAWCARDLIB 0x1003a020
HBITMAP global_CARDBK_Special;

// GLOBAL: DRAWCARDLIB 0x1003a024
HBITMAP global_CARDBK_Artifact;

// GLOBAL: DRAWCARDLIB 0x1003a028
HBITMAP global_CARDBK_ArabianNightsLand;

// GLOBAL: DRAWCARDLIB 0x1003a02c
COLORREF g_alternateCardFrameColor;

// GLOBAL: DRAWCARDLIB 0x1003a030
undefined4 g_defaultCardFrameBrush;

// GLOBAL: DRAWCARDLIB 0x1003a034
HBITMAP global_CARDBK_Blue;

// GLOBAL: DRAWCARDLIB 0x1003a038
undefined4 g_alternateCardFrameBrush;

// GLOBAL: DRAWCARDLIB 0x1003a03c
HBITMAP global_CARDBK_Red;

// GLOBAL: DRAWCARDLIB 0x1003a040
HFONT g_bigCardTextItalicFont;

// GLOBAL: DRAWCARDLIB 0x1003a084
HBITMAP global_CARDBK_AntiquitiesLand;

// GLOBAL: DRAWCARDLIB 0x1003a088
HBITMAP global_CARDBK_Black;

// GLOBAL: DRAWCARDLIB 0x1003a08c
COLORREF g_smallCardTitleAltColor;

// GLOBAL: DRAWCARDLIB 0x1003a198
HBITMAP global_ManaSymbols;

// GLOBAL: DRAWCARDLIB 0x1003a19c
COLORREF g_smallCardTitleFallbackColor;

// GLOBAL: DRAWCARDLIB 0x1003a1a0
COLORREF g_rulesTextColor;

// GLOBAL: DRAWCARDLIB 0x1003a1a4
COLORREF g_cardBorderColor;

// GLOBAL: DRAWCARDLIB 0x100f37d0
char g_cardNames[100 * 1024];

// GLOBAL: DRAWCARDLIB 0x100f3833
char g_cardNameSlotTerminators[100 * 1024];

// GLOBAL: DRAWCARDLIB 0x10124510
HINSTANCE g_drawCardLibInstance;

// GLOBAL: DRAWCARDLIB 0x1012655c
COLORREF g_smallCardTitleBackgroundColor;

// FUNCTION: DRAWCARDLIB 0x10004970
undefined4 prepare_fonts_and_imgs(void)
{
  CHAR resourcePath[264];

  strcpy(global_dual_dat_filename, global_base_directory);
  strcat(global_dual_dat_filename, "\\DuelArt");
  strcat(global_dual_dat_filename, "\\Duel.dat");
  strcpy(resourcePath, global_base_directory);
  strcat(resourcePath, "\\Tt0530m_.TTF");
  AddFontResourceA(resourcePath);
  strcpy(resourcePath, global_base_directory);
  strcat(resourcePath, "\\Magis___.TTF");
  AddFontResourceA(resourcePath);
  strcpy(resourcePath, global_base_directory);
  strcat(resourcePath, "\\Tt0127m_.TTF");
  AddFontResourceA(resourcePath);
  strcpy(resourcePath, global_base_directory);
  strcat(resourcePath, "\\Tt0085m_.TTF");
  AddFontResourceA(resourcePath);
  strcpy(resourcePath, global_base_directory);
  strcat(resourcePath, "\\Tt0298m_.TTF");
  AddFontResourceA(resourcePath);
  strcpy(resourcePath, global_base_directory);
  strcat(resourcePath, "\\Tt0299m_.TTF");
  AddFontResourceA(resourcePath);
  strcpy(resourcePath, global_base_directory);
  strcat(resourcePath, "\\Tt0300m_.TTF");
  AddFontResourceA(resourcePath);
  sprintf(resourcePath, "%s\\ManaSymbols.pic", &global_cardart_directory);
  global_ManaSymbols = load_pic(resourcePath);
  sprintf(resourcePath, "%s\\CardSets.pic", &global_cardart_directory);
  global_CardSets = load_pic(resourcePath);
  sprintf(resourcePath, "%s\\CardBack.pic", &global_cardart_directory);
  global_CardBack = load_pic(resourcePath);

  g_bigCardTitleFont = CreateFontIndirectA(LoadFontFromIni("BigCardTitle", 0));
  g_bigCardSubtitleFont = CreateFontIndirectA(LoadFontFromIni("BigCardSubtitle", 0));
  g_bigCardPowerToughnessFont = CreateFontIndirectA(LoadFontFromIni("BigCardPT", 0));
  g_bigCardTextFont = CreateFontIndirectA(LoadFontFromIni("BigCardText", 0));
  g_bigCardTextItalicFont = CreateFontIndirectA(LoadFontFromIni("BigCardText", 1));
  g_smallCardTitleFont = CreateFontIndirectA(LoadFontFromIni("SmallCardTitle", 0));
  g_defaultCardFrameColor = GetPaletteColor(0xbe);
  g_alternateCardFrameColor = GetPaletteColor(0xca);
  g_cardTextShadowColor = GetPaletteColor(0xc9);
  g_smallCardTitleColor = GetPaletteColor(0xbf);
  g_smallCardTitleAltColor = GetPaletteColor(0xd8);
  g_smallCardTitleFallbackColor = GetPaletteColor(0x31);
  g_rulesTextColor = GetPaletteColor(0xc9);
  g_smallCardTitleBackgroundColor = GetPaletteColor(0xc4);
  g_cardBorderColor = GetPaletteColor(0xc4);
  g_cardBorderPen = (undefined4)CreatePen(0, 0, g_cardBorderColor);
  g_defaultCardFrameBrush = (undefined4)CreateSolidBrush(g_defaultCardFrameColor);
  g_alternateCardFrameBrush = (undefined4)CreateSolidBrush(g_alternateCardFrameColor);
  global_CARDBK_Artifact = 0;
  global_CARDBK_Gold = global_CARDBK_Artifact;
  global_CARDBK_Red = global_CARDBK_Gold;
  global_CARDBK_Black = global_CARDBK_Red;
  global_CARDBK_Blue = global_CARDBK_Black;
  global_CARDBK_White = global_CARDBK_Blue;
  global_CARDBK_Green = global_CARDBK_White;
  global_CARDBK_ArabianNightsLand = 0;
  global_CARDBK_LegendsLand = global_CARDBK_ArabianNightsLand;
  global_CARDBK_AntiquitiesLand = global_CARDBK_LegendsLand;
  global_CARDBK_FallenEmpiresLand = global_CARDBK_AntiquitiesLand;
  global_CARDBK_DarklandsLand = global_CARDBK_FallenEmpiresLand;
  global_CARDBK_RedLand = global_CARDBK_DarklandsLand;
  global_CARDBK_BlackLand = global_CARDBK_RedLand;
  global_CARDBK_BlueLand = global_CARDBK_BlackLand;
  global_CARDBK_WhiteLand = global_CARDBK_BlueLand;
  global_CARDBK_GreenLand = global_CARDBK_WhiteLand;
  global_CARDBK_Special = 0;
  if ((((((global_ManaSymbols == (HBITMAP)0x0) || (global_CardSets == (HBITMAP)0x0)) || (global_CardBack == (HBITMAP)0x0)) ||
        ((g_bigCardTitleFont == (HFONT)0x0 || (g_bigCardSubtitleFont == (HFONT)0x0)))) ||
       ((g_bigCardTextFont == (HFONT)0x0 || ((g_bigCardTextItalicFont == (HFONT)0x0 || (g_smallCardTitleFont == (HFONT)0x0)))))) ||
      (((HPEN)g_cardBorderPen == (HPEN)0x0 ||
        (((HBRUSH)g_defaultCardFrameBrush == (HBRUSH)0x0 || ((HBRUSH)g_alternateCardFrameBrush == (HBRUSH)0x0))))))
  {
    DestroyAllResources();
    return 0;
  }

  return 1;
}

// FUNCTION: DRAWCARDLIB 0x10004e82
LOGFONTA *LoadFontFromIni(char *fontKeySuffix, int italic)
{
  CHAR key[100];
  UINT boldSetting;

  memcpy(&global_temp_font, &global_template_font, sizeof(LOGFONTA));
  strcpy(key, "size");
  strcat(key, fontKeySuffix);
  global_temp_font.lfHeight = GetPrivateProfileIntA("Fonts", key, 0x14, global_dual_dat_filename);

  strcpy(key, "bold");
  strcat(key, fontKeySuffix);
  boldSetting = GetPrivateProfileIntA("Fonts", key, 0, global_dual_dat_filename);
  if (boldSetting != 0)
  {
    global_temp_font.lfWeight = FW_BOLD;
  }
  if (italic != 0)
  {
    global_temp_font.lfItalic = TRUE;
  }

  strcpy(key, "font");
  strcat(key, fontKeySuffix);
  GetPrivateProfileStringA("Fonts", key, "MS Sans Serif", global_temp_font.lfFaceName, 0x20, global_dual_dat_filename);
  return &global_temp_font;
}

static void DestroyAllCardBackgrounds_impl(void)
{
  if (global_CARDBK_Green != 0)
  {
    DeleteAndCloseObject(global_CARDBK_Green);
    global_CARDBK_Green = 0;
  }
  if (global_CARDBK_White != 0)
  {
    DeleteAndCloseObject(global_CARDBK_White);
    global_CARDBK_White = 0;
  }
  if (global_CARDBK_Blue != 0)
  {
    DeleteAndCloseObject(global_CARDBK_Blue);
    global_CARDBK_Blue = 0;
  }
  if (global_CARDBK_Black != 0)
  {
    DeleteAndCloseObject(global_CARDBK_Black);
    global_CARDBK_Black = 0;
  }
  if (global_CARDBK_Red != 0)
  {
    DeleteAndCloseObject(global_CARDBK_Red);
    global_CARDBK_Red = 0;
  }
  if (global_CARDBK_Gold != 0)
  {
    DeleteAndCloseObject(global_CARDBK_Gold);
    global_CARDBK_Gold = 0;
  }
  if (global_CARDBK_Artifact != 0)
  {
    DeleteAndCloseObject(global_CARDBK_Artifact);
    global_CARDBK_Artifact = 0;
  }
  if (global_CARDBK_GreenLand != 0)
  {
    DeleteAndCloseObject(global_CARDBK_GreenLand);
    global_CARDBK_GreenLand = 0;
  }
  if (global_CARDBK_WhiteLand != 0)
  {
    DeleteAndCloseObject(global_CARDBK_WhiteLand);
    global_CARDBK_WhiteLand = 0;
  }
  if (global_CARDBK_BlueLand != 0)
  {
    DeleteAndCloseObject(global_CARDBK_BlueLand);
    global_CARDBK_BlueLand = 0;
  }
  if (global_CARDBK_BlackLand != 0)
  {
    DeleteAndCloseObject(global_CARDBK_BlackLand);
    global_CARDBK_BlackLand = 0;
  }
  if (global_CARDBK_RedLand != 0)
  {
    DeleteAndCloseObject(global_CARDBK_RedLand);
    global_CARDBK_RedLand = 0;
  }
  if (global_CARDBK_DarklandsLand != 0)
  {
    DeleteAndCloseObject(global_CARDBK_DarklandsLand);
    global_CARDBK_DarklandsLand = 0;
  }
  if (global_CARDBK_FallenEmpiresLand != 0)
  {
    DeleteAndCloseObject(global_CARDBK_FallenEmpiresLand);
    global_CARDBK_FallenEmpiresLand = 0;
  }
  if (global_CARDBK_AntiquitiesLand != 0)
  {
    DeleteAndCloseObject(global_CARDBK_AntiquitiesLand);
    global_CARDBK_AntiquitiesLand = 0;
  }
  if (global_CARDBK_LegendsLand != 0)
  {
    DeleteAndCloseObject(global_CARDBK_LegendsLand);
    global_CARDBK_LegendsLand = 0;
  }
  if (global_CARDBK_ArabianNightsLand != 0)
  {
    DeleteAndCloseObject(global_CARDBK_ArabianNightsLand);
    global_CARDBK_ArabianNightsLand = 0;
  }
  if (global_CARDBK_Special != 0)
  {
    DeleteAndCloseObject(global_CARDBK_Special);
    global_CARDBK_Special = 0;
  }
}

// FUNCTION: DRAWCARDLIB 0x10004f8e
void DestroyAllResources(void)
{
  CHAR resourcePath[264];

  strcpy(resourcePath, global_base_directory);
  strcat(resourcePath, "\\Magim___.TTF");
  RemoveFontResourceA(resourcePath);
  strcpy(resourcePath, global_base_directory);
  strcat(resourcePath, "\\Tt0530m_.TTF");
  RemoveFontResourceA(resourcePath);
  strcpy(resourcePath, global_base_directory);
  strcat(resourcePath, "\\Magis___.TTF");
  RemoveFontResourceA(resourcePath);
  strcpy(resourcePath, global_base_directory);
  strcat(resourcePath, "\\Tt0127m_.TTF");
  RemoveFontResourceA(resourcePath);
  strcpy(resourcePath, global_base_directory);
  strcat(resourcePath, "\\Tt0085m_.TTF");
  RemoveFontResourceA(resourcePath);
  strcpy(resourcePath, global_base_directory);
  strcat(resourcePath, "\\Tt0298m_.TTF");
  RemoveFontResourceA(resourcePath);
  strcpy(resourcePath, global_base_directory);
  strcat(resourcePath, "\\Tt0299m_.TTF");
  RemoveFontResourceA(resourcePath);
  strcpy(resourcePath, global_base_directory);
  strcat(resourcePath, "\\Tt0300m_.TTF");
  RemoveFontResourceA(resourcePath);
  if (global_ManaSymbols != 0)
  {
    DeleteAndCloseObject(global_ManaSymbols);
    global_ManaSymbols = 0;
  }
  if (global_CardSets != 0)
  {
    DeleteAndCloseObject(global_CardSets);
    global_CardSets = 0;
  }
  if (global_CardBack != 0)
  {
    DeleteAndCloseObject(global_CardBack);
    global_CardBack = 0;
  }

  DestroyAllCardBackgrounds_impl();

  if (g_bigCardTitleFont != (HGDIOBJ)0x0)
  {
    DeleteObject(g_bigCardTitleFont);
  }
  g_bigCardTitleFont = (HGDIOBJ)0x0;
  if (g_bigCardSubtitleFont != (HGDIOBJ)0x0)
  {
    DeleteObject(g_bigCardSubtitleFont);
  }
  g_bigCardSubtitleFont = (HGDIOBJ)0x0;
  if (g_bigCardPowerToughnessFont != (HGDIOBJ)0x0)
  {
    DeleteObject(g_bigCardPowerToughnessFont);
  }
  g_bigCardPowerToughnessFont = (HGDIOBJ)0x0;
  if (g_bigCardTextFont != (HGDIOBJ)0x0)
  {
    DeleteObject(g_bigCardTextFont);
  }
  g_bigCardTextFont = (HGDIOBJ)0x0;
  if (g_bigCardTextItalicFont != (HGDIOBJ)0x0)
  {
    DeleteObject(g_bigCardTextItalicFont);
  }
  g_bigCardTextItalicFont = (HGDIOBJ)0x0;
  if (g_smallCardTitleFont != (HGDIOBJ)0x0)
  {
    DeleteObject(g_smallCardTitleFont);
  }
  g_smallCardTitleFont = (HGDIOBJ)0x0;
  if ((HGDIOBJ)g_cardBorderPen != (HGDIOBJ)0x0)
  {
    DeleteObject((HGDIOBJ)g_cardBorderPen);
  }
  g_cardBorderPen = (undefined4)(HGDIOBJ)0x0;
  if ((HGDIOBJ)g_defaultCardFrameBrush != (HGDIOBJ)0x0)
  {
    DeleteObject((HGDIOBJ)g_defaultCardFrameBrush);
  }
  g_defaultCardFrameBrush = (undefined4)(HGDIOBJ)0x0;
  if ((HGDIOBJ)g_alternateCardFrameBrush != (HGDIOBJ)0x0)
  {
    DeleteObject((HGDIOBJ)g_alternateCardFrameBrush);
  }
  g_alternateCardFrameBrush = (undefined4)(HGDIOBJ)0x0;
}

// FUNCTION: DRAWCARDLIB 0x1000558b
void DestroyAllCardBackgrounds(void)
{
  DestroyAllCardBackgrounds_impl();
}

// FUNCTION: DRAWCARDLIB 0x10005830
void DrawCardBack(HDC dc, RECT *rect)
{
  int xInset;
  int yInset;
  RECT innerRect;

  if (dc == NULL || rect == NULL)
  {
    return;
  }

  if (global_CardBack != 0)
  {
    xInset = max(((rect->right - rect->left) * 3) / 100, 1);
    yInset = max(((rect->bottom - rect->top) * 2) / 100, 1);

    FillRect(dc, rect, GetStockObject(4));
    SetRect(&innerRect, rect->left + xInset, rect->top + yInset, rect->right - xInset,
            rect->bottom - yInset);
    DrawBitmapToRect(dc, &innerRect, global_CardBack);
  }
  else
  {
    FillRect(dc, rect, GetStockObject(4));
  }
}

// FUNCTION: DRAWCARDLIB 0x1000592c
undefined4 DrawFullCard(HDC dc, RECT *rect, card_ptr_t *card, undefined4 version, uint drawFlags, bool expanded_text, LPCSTR artistFormat)
{
  int backgroundCropLeftPermille;
  int backgroundCropTopPermille;
  BITMAP backgroundBitmapInfo;
  int rulesBackgroundTopInset;
  int backgroundCropHeightPermille;
  int backgroundCropWidthPermille;
  TEXTMETRICA textMetrics;
  RECT flavorRect;
  int textBlockHeight;
  uint manaTextHeight;
  RECT artDeviceRect;
  LPCSTR typeText;
  int defaultPowerToughness;
  HBITMAP *backgroundPointer;
  RECT cardBackgroundRect;
  HBRUSH frameBrush;
  RECT artRect;
  RECT powerToughnessRect;
  int shadowOffset;
  COLORREF textColor;
  char rules_text[1000];
  int cardWidth;
  RECT artistRect;
  char tempText[52];
  RECT rulesBackgroundRect;
  undefined4 result;
  HANDLE backgroundBitmap;
  RECT typeLineRect;
  RECT rulesTextRect;
  int cornerWidth;
  RECT nameRect;
  int nSavedDC;
  int cardHeight;
  int cornerHeight;
  RECT castingCostRect;
  RECT setIconRect;

  if (((dc == (HDC)0x0) || (rect == (RECT *)0x0)) || (card == (card_ptr_t *)0x0))
  {
    return 0;
  }

  nSavedDC = SaveDC(dc);
  cardWidth = 200;
  cardHeight = 300;
  SetMapMode(dc, 7);
  SetWindowExtEx(dc, cardWidth, cardHeight, (LPSIZE)0x0);
  SetViewportExtEx(dc, rect->right - rect->left, rect->bottom - rect->top, (LPSIZE)0x0);
  SetWindowOrgEx(dc, cardWidth / 2, cardHeight / 2, (LPPOINT)0x0);
  SetViewportOrgEx(dc, rect->left + (rect->right - rect->left) / 2,
                   rect->top + (rect->bottom - rect->top) / 2, (LPPOINT)0x0);
  cornerWidth = 6;
  cornerHeight = 6;
  SetRect(&nameRect, 10, 5, 0xbc, 0x15);
  SetRect(&castingCostRect, 0xc, 8, 0xbe, 0x14);
  SetRect(&artRect, 0x15, 0x19, 0xb5, 0xa4);
  SetRect(&typeLineRect, 0xc, 0xa8, 0xba, 0xb3);
  SetRect(&setIconRect, 0xc, 0xa9, 0xba, 0xb4);
  SetRect(&rulesTextRect, 0x1c, 0xb9, 0xae, 0x10c);
  SetRect(&rulesBackgroundRect, 0x14, 0xb4, 0xb6, 0x10f);
  SetRect(&artistRect, 0xc, 0x114, 0xbc, 0x124);
  SetRect(&powerToughnessRect, 0xc, 0x114, 0xbc, 0x124);
  if (card->expansion == -1 || (card->expansion & 0x10) != 0 || (card->expansion & 0x80) != 0)
  {
    frameBrush = CreateSolidBrush(g_defaultCardFrameColor);
    SelectObject(dc, frameBrush);
  }
  else
  {
    frameBrush = CreateSolidBrush(g_alternateCardFrameColor);
    SelectObject(dc, frameBrush);
  }
  SelectObject(dc, GetStockObject(8));
  RoundRect(dc, 0, 0, cardWidth, cardHeight, cornerWidth / 2, cornerHeight / 2);
  SelectObject(dc, GetStockObject(4));
  if (frameBrush != (HBRUSH)0x0)
  {
    DeleteObject(frameBrush);
  }
  backgroundPointer = GetCardBackgroundPointer(card);
  LoadCardBackground(backgroundPointer);
  SetRect(&cardBackgroundRect, cornerWidth, cornerHeight,
          // Well these are weird
          cornerWidth + cardWidth - (cornerWidth << 1),
          cornerHeight + cardHeight - (cornerHeight << 1));
  if (*backgroundPointer != 0)
  {
    DrawBitmapToRect(dc, &cardBackgroundRect, *backgroundPointer);
  }
  else
  {
    FillRect(dc, &cardBackgroundRect, GetStockObject(0));
  }
  backgroundBitmap = *backgroundPointer;
  shadowOffset = 1;
  textColor = GetPaletteColor(0xbf);
  SelectObject(dc, g_bigCardTitleFont);
  SetBkMode(dc, 1);
  SetTextColor(dc, g_cardTextShadowColor);
  OffsetRect(&nameRect, shadowOffset, shadowOffset);
  DrawTextA(dc, card->full_name, -1, &nameRect, 0x824);
  OffsetRect(&nameRect, -shadowOffset, -shadowOffset);
  SetTextColor(dc, textColor);
  DrawTextA(dc, card->full_name, -1, &nameRect, 0x824);
  defaultPowerToughness = 0x64;
  SelectObject(dc, g_bigCardPowerToughnessFont);
  if ((card->power != 0) || (card->toughness != 0))
  {
    tempText[0] = '\0';
    if (card->power == defaultPowerToughness)
    {
      strcat(tempText, "*");
    }
    else if (defaultPowerToughness < card->power)
    {
      wsprintfA((char *)((ptrdiff_t)&tempText + strlen(tempText)), "%d+*", card->power - defaultPowerToughness);
    }
    else
    {
      wsprintfA((char *)((ptrdiff_t)&tempText + strlen(tempText)), "%d", card->power);
    }
    strcat(tempText, "/");
    if (card->toughness == defaultPowerToughness)
    {
      strcat(tempText, "*");
    }
    else if (defaultPowerToughness < card->toughness)
    {
      wsprintfA((char *)((ptrdiff_t)&tempText + strlen(tempText)), "%d+*", card->toughness - defaultPowerToughness);
    }
    else
    {
      wsprintfA((char *)((ptrdiff_t)&tempText + strlen(tempText)), "%d", card->toughness);
    }
    SetTextColor(dc, g_cardTextShadowColor);
    OffsetRect(&powerToughnessRect, shadowOffset, shadowOffset);
    DrawTextA(dc, tempText, -1, &powerToughnessRect, 0x26);
    OffsetRect(&powerToughnessRect, -shadowOffset, -shadowOffset);
    SetTextColor(dc, textColor);
    DrawTextA(dc, tempText, -1, &powerToughnessRect, 0x26);
  }
  SelectObject(dc, g_bigCardSubtitleFont);
  SetBkMode(dc, 1);
  if ((card->card_type != -1) && (card->card_type != 0))
  {
    typeText = card->type_text;
    SetTextColor(dc, g_cardTextShadowColor);
    OffsetRect(&typeLineRect, shadowOffset, shadowOffset);
    DrawTextA(dc, typeText, -1, &typeLineRect, 0x24);
    OffsetRect(&typeLineRect, -shadowOffset, -shadowOffset);
    SetTextColor(dc, textColor);
    DrawTextA(dc, typeText, -1, &typeLineRect, 0x24);
  }
  if (card->artist != 0)
  {
    wsprintfA(tempText, artistFormat, card->artist);
    SetTextColor(dc, g_cardTextShadowColor);
    OffsetRect(&artistRect, shadowOffset, shadowOffset);
    DrawTextA(dc, tempText, -1, &artistRect, 0x824);
    OffsetRect(&artistRect, -shadowOffset, -shadowOffset);
    SetTextColor(dc, textColor);
    DrawTextA(dc, tempText, -1, &artistRect, 0x824);
  }
  if (((card->card_type != 5) && (card->card_type != 8)) && (card->card_type != 0))
  {
    DrawCastingCost(dc, &castingCostRect, &card->req);
  }
  if (card->expansion != -1)
  {
    DrawCardSet(dc, &setIconRect, card->expansion);
  }
  CopyRect(&artDeviceRect, &artRect);
  LPtoDP(dc, (LPPOINT)&artDeviceRect, 2);

  // HMMM
  switch (drawFlags & 0xf)
  {
  case 0:
    if (IsBigArtIn(card->id, version) != 0)
    {
      DrawBigArt(dc, &artRect, card->id, version);
    }
    else
    {
      DrawSmallArt(dc, &artRect, card->id, version);
    }
    break;

  case 1:
    if (IsBigArtIn(card->id, version) == 0)
    {
      if (((drawFlags & 0x10) != 0) && (IsSmallArtIn(card->id, version) != 0))
      {
        DrawSmallArt(dc, &artRect, card->id, version);
      }
      LoadBigArt(card->id, version, artDeviceRect.right - artDeviceRect.left,
                 artDeviceRect.bottom - artDeviceRect.top);
    }
    DrawBigArt(dc, &artRect, card->id, version);
    break;
  default:
    if (((IsBigArtIn(card->id, version) == 0) && ((drawFlags & 0x10) != 0)) &&
        (IsSmallArtIn(card->id, version) != 0))
    {
      DrawSmallArt(dc, &artRect, card->id, version);
    }
    if (LoadBigArt(card->id, version, artDeviceRect.right - artDeviceRect.left,
                   artDeviceRect.bottom - artDeviceRect.top) != 0)
    {
      DrawBigArt(dc, &artRect, card->id, version);
    }
    else
    {
      DrawSmallArt(dc, &artRect, card->id, version);
    }
    break;
  }
  result = IsBigArtRightSize(card->id, version, artDeviceRect.right - artDeviceRect.left,
                             artDeviceRect.bottom - artDeviceRect.top);
  strcpy(rules_text, card->rules_text);
  ReplaceSubstring(rules_text, "|H1", 1, "");
  ReplaceSubstring(rules_text, "|H2", 1, "");
  ReplaceSubstring(rules_text, "|H3", 1, "");
  ReplaceSubstring(rules_text, "|H4", 1, "");
  ReplaceSubstring(rules_text, "|H", 1, "");
  ReplaceSubstring(rules_text, "|S1", 1, "");
  ReplaceSubstring(rules_text, "|S2", 1, "");
  ReplaceSubstring(rules_text, "|S3", 1, "");
  ReplaceSubstring(rules_text, "|S4", 1, "");
  ReplaceSubstring(rules_text, "|S", 1, "");
  SetTextColor(dc, g_rulesTextColor);
  SetBkMode(dc, 1);
  if (expanded_text != 0)
  {
    SelectObject(dc, g_bigCardTextFont);
    manaTextHeight = (unsigned short)((CalcDrawManaText(dc, &rulesTextRect, rules_text) >> 0x10) & 0xffff);
    GetTextMetricsA(dc, &textMetrics);
    CopyRect(&flavorRect, &rulesTextRect);
    flavorRect.top += manaTextHeight;
    flavorRect.top += textMetrics.tmHeight / 2;
    SelectObject(dc, g_bigCardTextItalicFont);
    DrawTextA(dc, card->flavor_text, -1, &flavorRect, 0x410);
    textBlockHeight = flavorRect.bottom - rulesTextRect.top;
    if (rulesTextRect.bottom - rulesTextRect.top < textBlockHeight)
    {
      rulesBackgroundTopInset = rulesTextRect.top - rulesBackgroundRect.top;
      rulesTextRect.top = rulesTextRect.bottom - textBlockHeight;
      rulesBackgroundRect.top = rulesTextRect.top - rulesBackgroundTopInset;
      if (backgroundBitmap != 0)
      {
        GetObjectA(backgroundBitmap, 0x18, &backgroundBitmapInfo);
        backgroundCropLeftPermille = 0x49;
        backgroundCropTopPermille = 0x25d;
        backgroundCropWidthPermille = 0x359;
        backgroundCropHeightPermille = 0x140;
        DrawBitmapSubrectToRect(dc, &rulesBackgroundRect, backgroundBitmap,
                                (backgroundBitmapInfo.bmWidth * backgroundCropLeftPermille) / 1000,
                                (backgroundBitmapInfo.bmHeight * backgroundCropTopPermille) / 1000,
                                (backgroundBitmapInfo.bmWidth * backgroundCropWidthPermille) / 1000,
                                (backgroundBitmapInfo.bmHeight * backgroundCropHeightPermille) / 1000);
      }
      else
      {
        FillRect(dc, &rulesBackgroundRect, GetStockObject(0));
      }
    }
  }
  SelectObject(dc, g_bigCardTextFont);
  manaTextHeight = (unsigned short)((DrawManaText(dc, &rulesTextRect, rules_text, 1) >> 0x10) & 0xffff);
  GetTextMetricsA(dc, &textMetrics);
  CopyRect(&flavorRect, &rulesTextRect);
  flavorRect.top += manaTextHeight;
  flavorRect.top += textMetrics.tmHeight / 3;
  SelectObject(dc, g_bigCardTextItalicFont);
  DrawTextA(dc, card->flavor_text, -1, &flavorRect, 0x10);
  RestoreDC(dc, nSavedDC);
  return result;
}

// FUNCTION: DRAWCARDLIB 0x100067c7
HBITMAP *GetCardBackgroundPointer(card_ptr_t *card)
{
  HBITMAP *result;

  if (card->color == 1)
  {
    result = &global_CARDBK_Black;
  }
  else if (card->color == 8)
  {
    result = &global_CARDBK_White;
  }
  else if (card->color == 7)
  {
    result = &global_CARDBK_Red;
  }
  else if (card->color == 5)
  {
    result = &global_CARDBK_Green;
  }
  else if (card->color == 2)
  {
    result = &global_CARDBK_Blue;
  }
  else if (card->color == 4)
  {
    result = &global_CARDBK_Gold;
  }
  else if (card->color == 0)
  {
    result = &global_CARDBK_Artifact;
  }
  else if (card->color == 3)
  {
    result = &global_CARDBK_Artifact;
  }
  else if (card->color == 6)
  {
    if ((card->expansion & 2) != 0)
    {
      result = &global_CARDBK_AntiquitiesLand;
    }
    else
    {
      if ((card->expansion & 4) != 0)
      {
        result = &global_CARDBK_ArabianNightsLand;
      }
      else
      {
        if ((card->expansion & 0x20) != 0)
        {
          result = &global_CARDBK_DarklandsLand;
        }
        else
        {
          if ((card->expansion & 0x100) != 0)
          {
            result = &global_CARDBK_LegendsLand;
          }
          else
          {
            if ((card->expansion & 8) != 0)
            {
              result = &global_CARDBK_AntiquitiesLand;
            }
            else
            {
              if (strcmp(card->full_name, g_cardNames + 0x5d5c) == 0)
              {
                result = &global_CARDBK_BlackLand;
              }
              else if (strcmp(card->full_name, g_cardNames + 0x4970) == 0)
              {
                result = &global_CARDBK_WhiteLand;
              }
              else if (strcmp(card->full_name, g_cardNames + 0x4010) == 0)
              {
                result = &global_CARDBK_RedLand;
              }
              else if (strcmp(card->full_name, g_cardNames + 0x238c) == 0)
              {
                result = &global_CARDBK_GreenLand;
              }
              else if (strcmp(card->full_name, g_cardNames + 0x3138) == 0)
              {
                result = &global_CARDBK_BlueLand;
              }
              else
              {
                result = &global_CARDBK_AntiquitiesLand;
              }
            }
          }
        }
      }
    }
  }
  else if (card->color == -1)
  {
    result = &global_CARDBK_Special;
  }
  else
  {
    result = &global_CARDBK_Artifact;
  }
  return result;
}

// FUNCTION: DRAWCARDLIB 0x10006a41
BOOL LoadCardBackground(HBITMAP *outBitmap)
{
  char filepath[264];
  char filename[100];

  if (outBitmap == (HBITMAP *)0x0)
  {
    return 0;
  }
  if (*outBitmap != 0)
  {
    return 1;
  }
  if (outBitmap == &global_CARDBK_Green)
  {
    strcpy(filename, "CARDBK_Green");
  }
  else if (outBitmap == &global_CARDBK_White)
  {
    strcpy(filename, "CARDBK_White");
  }
  else if (outBitmap == &global_CARDBK_Blue)
  {
    strcpy(filename, "CARDBK_Blue");
  }
  else if (outBitmap == &global_CARDBK_Black)
  {
    strcpy(filename, "CARDBK_Black");
  }
  else if (outBitmap == &global_CARDBK_Red)
  {
    strcpy(filename, "CARDBK_Red");
  }
  else if (outBitmap == &global_CARDBK_Gold)
  {
    strcpy(filename, "CARDBK_Gold");
  }
  else if (outBitmap == &global_CARDBK_Artifact)
  {
    strcpy(filename, "CARDBK_Artifact");
  }
  else if (outBitmap == &global_CARDBK_GreenLand)
  {
    strcpy(filename, "CARDBK_GreenLand");
  }
  else if (outBitmap == &global_CARDBK_WhiteLand)
  {
    strcpy(filename, "CARDBK_WhiteLand");
  }
  else if (outBitmap == &global_CARDBK_BlueLand)
  {
    strcpy(filename, "CARDBK_BlueLand");
  }
  else if (outBitmap == &global_CARDBK_BlackLand)
  {
    strcpy(filename, "CARDBK_BlackLand");
  }
  else if (outBitmap == &global_CARDBK_RedLand)
  {
    strcpy(filename, "CARDBK_RedLand");
  }
  else if (outBitmap == &global_CARDBK_DarklandsLand)
  {
    strcpy(filename, "CARDBK_DarklandsLand");
  }
  else if (outBitmap == &global_CARDBK_FallenEmpiresLand)
  {
    strcpy(filename, "CARDBK_FallenEmpiresLand");
  }
  else if (outBitmap == &global_CARDBK_AntiquitiesLand)
  {
    strcpy(filename, "CARDBK_AntiquitiesLand");
  }
  else if (outBitmap == &global_CARDBK_LegendsLand)
  {
    strcpy(filename, "CARDBK_LegendsLand");
  }
  else if (outBitmap == &global_CARDBK_ArabianNightsLand)
  {
    strcpy(filename, "CARDBK_ArabianNightsLand");
  }
  else if (outBitmap == &global_CARDBK_Special)
  {
    strcpy(filename, "CARDBK_Special");
  }
  else
  {
    strcpy(filename, (char *)&g_emptyStringStorage);
  }
  if (filename[0] != '\0')
  {
    sprintf(filepath, "%s\\%s\x2Epic", &global_cardart_directory, filename);
    *outBitmap = load_pic(filepath);
  }
  if (*outBitmap != 0)
  {
    return 1;
  }
  else
    return 0;
}

// FUNCTION: DRAWCARDLIB 0x10006d5f
void DrawCardSet(HDC dc, RECT *rect, uint expansion)
{
  BITMAP bm; // ebp - 0x40
  int cellH; // ebp - 0x24

  int dstW;     // ebp - 0x24
  int cellW;    // ebp - 0x20
  int srcX;     // ebp - 0x1c
  RECT dstRect; // ebp - 0x18
  int srcX2;    // ebp - 8
  int dstH;

  if (dc == 0 || rect == (RECT *)0x0)
  {
    return;
  }
  if ((expansion & 0x800) != 0)
  {
    return;
  }
  if (expansion == 0xffffffff || (expansion & 0x10) != 0 || (expansion & 0x80) != 0)
  {
    return;
  }
  if (global_CardSets != NULL)
  {
    if (((expansion & 0x2e) != 0) || ((expansion & 0x100) != 0))
    {
      GetObjectA(global_CardSets, 0x18, &bm);
      cellW = bm.bmWidth / 10;
      cellH = bm.bmHeight;
      if ((expansion & 0x20) != 0)
      {
        srcX = 0;
      }
      else if ((expansion & 0x100) != 0)
      {
        srcX = cellW * 2;
      }
      else if ((expansion & 4) != 0)
      {
        srcX = cellW << 2;
      }
      else if ((expansion & 2) != 0)
      {
        srcX = cellW * 6;
      }
      else if ((expansion & 8) != 0)
      {
        srcX = cellW << 3;
      }

      srcX2 = cellW + srcX;
      dstH = rect->bottom - rect->top;
      dstW = (dstH * cellW) / cellH;
      SetRect(&dstRect, rect->right - dstW, rect->top, rect->right, rect->top + dstH);
      DrawMaskedBitmapToRect(dc, &dstRect, global_CardSets, cellW, cellH, srcX, 0, srcX2, 0);
    }
  }
}

// FUNCTION: DRAWCARDLIB 0x10006eed
void DrawCastingCost(HDC dc, RECT *rect, casting_cost_t *castingCost)
{
  int width;
  char castingCostString[20];
  int top;
  char symbol;
  int left;
  char *stringPtr;
  int height;
  int costReqs[7];

  if (dc == 0 || rect == 0 || castingCost == NULL)
  {
  }
  else
  {
    costReqs[0] = (int)(char)castingCost->req_colorless;
    costReqs[1] = (int)(char)castingCost->req_black;
    costReqs[5] = (int)(char)castingCost->req_white;
    costReqs[3] = (int)(char)castingCost->req_green;
    costReqs[4] = (int)(char)castingCost->req_red;
    costReqs[2] = (int)(char)castingCost->req_blue;
    BuildCostString(costReqs, castingCostString);
    height = rect->bottom - rect->top;
    width = height;
    left = rect->right - (strlen(castingCostString) >> 1) * width;
    top = rect->top;
    stringPtr = castingCostString;
    while (*stringPtr != '\0')
    {
      symbol = GetNextManaSymbol(&stringPtr);
      DrawManaSymbol(dc, symbol, left, top, width, height);
      left += width;
    }
  }
}

// FUNCTION: DRAWCARDLIB 0x10006ff3
int BuildCostString(int *costReqs, char *costStringOut)
{
  int req_black;
  int req_blue;
  int length;
  char result[52];
  int req_colorless;
  int req_red;
  int req_white;
  int done;
  int req_green;

  if ((costReqs == (int *)0x0) || (costStringOut == 0))
  {
    return 0;
  }

  req_white = costReqs[5];
  req_green = costReqs[3];
  req_red = costReqs[4];
  req_black = costReqs[1];
  req_blue = costReqs[2];
  req_colorless = costReqs[0];
  length = 0;
  if (req_colorless != 0)
  {
    if (req_colorless == -1)
    {
      result[length] = '|';
      length++;
      result[length] = 'X';
      length++;
    }
    else if (req_colorless == 0x48)
    {
      result[length] = '|';
      length++;
      result[length] = 'X';
      length++;
    }
    else if (req_colorless == 0x28)
    {
      result[length] = '|';
      length++;
      result[length] = 'X';
      length++;
    }
    else if ((req_colorless >= 1) && (9 >= req_colorless))
    {
      result[length] = '|';
      length++;
      result[length] = (char)(req_colorless + '0');
      length++;
    }
    else if (req_colorless == 10)
    {
      result[length] = '|';
      length++;
      result[length] = '1';
      length++;
      result[length] = '0';
      length++;
    }
  }
  done = 0;
  while (done == 0)
  {
    if (req_white != 0)
    {
      result[length] = '|';
      length++;
      result[length] = 'W';
      length++;
      req_white--;
    }
    else if (req_green != 0)
    {
      result[length] = '|';
      length++;
      result[length] = 'G';
      length++;
      req_green--;
    }
    else if (req_red != 0)
    {
      result[length] = '|';
      length++;
      result[length] = 'R';
      length++;
      req_red--;
    }
    else if (req_black != 0)
    {
      result[length] = '|';
      length++;
      result[length] = 'B';
      length++;
      --req_black;
    }
    else if (req_blue != 0)
    {
      result[length] = '|';
      length++;
      result[length] = 'U';
      length++;
      --req_blue;
    }
    else
    {
      result[length] = '\0';
      done = 1;
    }
  }
  if ((((req_colorless == 0) && (length == 0)) && (req_white == 0)) &&
      (((req_green == 0 && (req_red == 0)) && ((req_black == 0 && (req_blue == 0))))))
  {
    // 0 cost
    strcpy(result, "|0");
  }
  if (costStringOut != 0)
  {
    strcpy(costStringOut, result);
  }
  return length;
}

// FUNCTION: DRAWCARDLIB 0x100074b2
void DrawManaSymbol(HDC dc, char symbol, int left, int top, int width, int height)
{
  BITMAP bm;
  int hSrc;
  int wSrc;
  int symbolX;
  RECT rect;
  int maskX;

  if (dc == (HDC)0x0)
  {
    return;
  }
  if (symbol < -0x12 || -1 < symbol)
  {
    return;
  }
  GetObjectA(global_ManaSymbols, 0x18, &bm);
  wSrc = bm.bmHeight;
  hSrc = wSrc;
  maskX = bm.bmWidth - wSrc;
  if (symbol == -0x10)
  {
    symbolX = 0;
  }
  else if (symbol == -0xf)
  {
    symbolX = wSrc;
  }
  else if (symbol == -0xe)
  {
    symbolX = wSrc * 2;
  }
  else if (symbol == -0xd)
  {
    symbolX = wSrc * 3;
  }
  else if (symbol == -0xc)
  {
    symbolX = wSrc << 2;
  }
  else if (symbol == -0xb)
  {
    symbolX = wSrc * 5;
  }
  else if (symbol == -10)
  {
    symbolX = (wSrc * 3) * 2;
  }
  else if (symbol == -9)
  {
    symbolX = wSrc * 7;
  }
  else if (symbol == -8)
  {
    symbolX = wSrc << 3;
  }
  else if (symbol == -7)
  {
    symbolX = wSrc * 9;
  }
  else if (symbol == -6)
  {
    symbolX = wSrc * 10;
  }
  else if (symbol == -0x11)
  {
    symbolX = wSrc * 0xb;
  }
  else if (symbol == -5)
  {
    symbolX = wSrc * 0xc;
  }
  else if (symbol == -4)
  {
    symbolX = wSrc * 0xd;
  }
  else if (symbol == -3)
  {
    symbolX = wSrc * 0xe;
  }
  else if (symbol == -2)
  {
    symbolX = wSrc * 0xf;
  }
  else if (symbol == -1)
  {
    symbolX = wSrc << 4;
  }
  else if (symbol == -0x12)
  {
    symbolX = wSrc * 17;
  }
  SetRect(&rect, left, top, width + left, height + top);
  DrawMaskedBitmapToRect(dc, &rect, global_ManaSymbols, wSrc, hSrc, symbolX, 0, maskX, 0);
}

// FUNCTION: DRAWCARDLIB 0x1000775c
undefined4 CalcDrawManaText(HDC dc, RECT *rect, char *text)
{
  int nSavedDC;
  undefined4 result;

  /* 0x775c  1  CalcDrawManaText */
  if (((dc == (HDC)0x0) || (rect == (RECT *)0x0)) || (text == (char *)0x0))
  {
    return 0;
  }
  nSavedDC = SaveDC(dc);
  IntersectClipRect(dc, 0, 0, 1, 1);
  result = DrawManaText(dc, rect, text, 0);
  RestoreDC(dc, nSavedDC);
  return result;
}

// FUNCTION: DRAWCARDLIB 0x100077dd
uint DrawManaText(HDC dc, RECT *rect, char *text, int drawSymbols)
{
  int line_height;
  int symbol_step;
  char buf[52];
  int symbol_width;
  int font_height;
  int temp;
  int nSavedDC;
  int top;
  int idx;
  char symbol;
  int max_width;            // ebp - 0x60
  int left;                 // ebp - 0x5c
  TEXTMETRICA text_metrics; // ebp - 0x58
  int symbol_height;        // ebp - 0x20
  RECT rc;                  // ebp - 0x1c
  SIZE size;                // ebp - 0xc
  int height;               // ebp - 0x4

  if (((dc == (HDC)0x0) || (rect == (RECT *)0x0)) || (text == (char *)0x0))
  {
    return 0;
  }
  if (*text == '\0')
  {
    return 0;
  }
  nSavedDC = SaveDC(dc);
  max_width = 0;
  height = 0;
  GetTextMetricsA(dc, &text_metrics);
  line_height = text_metrics.tmExternalLeading + text_metrics.tmHeight;
  SetRect(&rc, 0, 0, 0, text_metrics.tmHeight);
  LPtoDP(dc, (LPPOINT)&rc, 2);
  SetRect(&rc, 0, 0, rc.bottom - rc.top, 0);
  DPtoLP(dc, (LPPOINT)&rc, 2);
  symbol_width = ((rc.right - rc.left) * 0x4b) / 100;
  symbol_step = ((rc.right - rc.left) * 0x55) / 100;
  symbol_height = (text_metrics.tmHeight * 0x4b) / 100;
  font_height = text_metrics.tmHeight;
  IntersectClipRect(dc, rect->left, rect->top, rect->right + 1, rect->bottom);
  SelectObject(dc, GetStockObject(4));
  SelectObject(dc, GetStockObject(8));
  left = rect->left;
  top = rect->top;
  while (*text != '\0')
  {
    if (*text == ' ')
    {
      idx = 0;
      buf[idx] = *text;
      text = text + 1;
      idx = idx + 1;
      while (*text != '\0' && *text == ' ')
      {
        buf[idx] = *text;
        text = text + 1;
        idx = idx + 1;
      }
      buf[idx] = '\0';
      GetTextExtentPoint32A(dc, buf, idx, &size);
      left = left + size.cx;
    }
    else if (*text != '\0' && *text == '\n')
    {
      text = text + 1;
      top = top + line_height + line_height / 3;
      left = rect->left;
    }
    else
    {
      symbol = GetNextManaSymbol(&text);
      if (symbol != '\0')
      {
        idx = 0;
        buf[idx] = symbol;
        idx = idx + 1;
        while (*text != '\0' && (symbol = GetNextManaSymbol(&text)) != '\0')
        {
          buf[idx++] = symbol;
        }
        buf[idx] = '\0';
        size.cx = idx * symbol_width;
        if (rect->right < size.cx + left)
        {
          max_width = max(max_width, left - rect->left);
          top = top + line_height;
          left = rect->left;
        }
        temp = strlen(buf);
        for (idx = 0; temp > idx; idx = idx + 1)
        {
          if (drawSymbols != 0)
          {
            DrawManaSymbol(dc, buf[idx], left + (symbol_step - symbol_width) / 2,
                           top + (font_height - symbol_height) / 2, symbol_width, symbol_height);
          }
          else
          {
            Ellipse(dc, left + (symbol_step - symbol_width) / 2,
                    top + (font_height - symbol_height) / 2,
                    symbol_width + (symbol_step - symbol_width) / 2 + left,
                    symbol_height + (font_height - symbol_height) / 2 + top);
          }
          left = left + symbol_step;
        }
        if ((*text == ':') || (*text == ','))
        {
          idx = 0;
          buf[idx] = *text;
          text = text + 1;
          idx = idx + 1;
          buf[idx] = '\0';
          GetTextExtentPoint32A(dc, buf, idx, &size);
          TextOutA(dc, left, top, buf, strlen(buf));
          left = left + size.cx;
        }
      }
      else
      {
        idx = 0;
        buf[idx] = *text;
        text = text + 1;
        idx = idx + 1;
        while (*text != '\0' && *text != ' ' && *text != '\n' && *text != '|')
        {
          buf[idx] = *text;
          text = text + 1;
          idx = idx + 1;
        }
        buf[idx] = '\0';
        GetTextExtentPoint32A(dc, buf, idx, &size);
        if (rect->right < size.cx + left)
        {
          max_width = max(max_width, left - rect->left);
          top = top + line_height;
          left = rect->left;
        }
        TextOutA(dc, left, top, buf, strlen(buf));
        left = left + size.cx;
      }
    }
  }
  max_width = max(max_width, left - rect->left);
  height = (line_height + top) - rect->top;
  RestoreDC(dc, nSavedDC);
  return ((unsigned short)height << 0x10) | (unsigned short)max_width;
}

// FUNCTION: DRAWCARDLIB 0x10007dd7
void DrawSmallCard(HDC dc, RECT *rect, card_ptr_t *card, undefined4 version, int reloadIfWrongSize)
{
  RECT artRect;   // ebp - 0x30
  int artTop;     // ebp - 0x20
  int artLeft;    // ebp - 0x1c
  int cardHeight; // ebp - 0x18
  int cardWidth;  // ebp - 0x14
  int artHeight;  // ebp - 0x10
  int status;     // ebp - 0xc
  int artWidth;   // ebp - 0x8
  int nSavedDC;   // ebp - 0x4

  if (dc == NULL || rect == NULL || card == NULL)
  {
    return;
  }

  nSavedDC = SaveDC(dc);
  SetMapMode(dc, 8);
  SetWindowExtEx(dc, 200, 0x118, (LPSIZE)0x0);
  SetViewportExtEx(dc, rect->right - rect->left, rect->bottom - rect->top, (LPSIZE)0x0);
  SetWindowOrgEx(dc, 0, 0, (LPPOINT)0x0);
  SetViewportOrgEx(dc, rect->left, rect->top, (LPPOINT)0x0);
  DrawCardBackground(dc, rect, card);
  DrawSmallCardTitle(dc, rect, card->name, 0, 1);
  SelectObject(dc, GetStockObject(5));
  SelectObject(dc, (HGDIOBJ)g_cardBorderPen);
  Rectangle(dc, 0, 0, 200, 0x118);
  SetMapMode(dc, 1);
  cardWidth = rect->right - rect->left;
  cardHeight = rect->bottom - rect->top;
  artLeft = (cardWidth * 0x12) / 0xe4;
  artTop = (cardHeight * 8) / 100 + (cardHeight * 0xb) / 100 + -2;
  artWidth = ((cardWidth * 0xd3) / 0xe4 - artLeft) + 1;
  artHeight = (cardHeight * 0xb2) / 0xbf - artTop;
  SetRect(&artRect, artLeft, artTop, artWidth + artLeft, artHeight + artTop);
  if (IsSmallArtIn(card->id, version) == 0)
  {
    LoadSmallArt(card->id, version, artWidth, artHeight);
  }
  else if (reloadIfWrongSize != 0)
  {
    ReloadSmallArtIfWrongSize(card->id, version, artWidth, artHeight);
  }
  status = DrawSmallArt(dc, &artRect, card->id, version);
  if (status == 0)
  {
    DrawBigArt(dc, &artRect, card->id, version);
  }
  RestoreDC(dc, nSavedDC);
}

// FUNCTION: DRAWCARDLIB 0x10008065
void DrawCardBackground(HDC dc, RECT *rect, card_ptr_t *card)
{
  int scaledHeight;
  BITMAP bm;
  HBITMAP *backgroundPointer;
  RECT dstRect;
  int sectionTop;
  int sectionHeight;

  if (dc == NULL || rect == NULL || card == NULL)
  {
    return;
  }

  backgroundPointer = GetCardBackgroundPointer(card);
  LoadCardBackground(backgroundPointer);
  if (*backgroundPointer != 0)
  {
    GetObjectA(*backgroundPointer, 0x18, &bm);
    SetRect(&dstRect, 0, 0, 200, 0x118);
    DrawBitmapSubrectToRect(dc, &dstRect, *backgroundPointer, 0, 0, bm.bmWidth,
                            (bm.bmHeight * 0x3b) / 100);
    scaledHeight = (((bm.bmHeight * 0x3b) / 100) * 0xb) / 100;
    sectionHeight = 0x1e;
    sectionTop = 0x16;
    SetRect(&dstRect, 0, sectionTop, 200, sectionHeight + sectionTop);
    DrawBitmapSubrectToRect(dc, &dstRect, *backgroundPointer, 0, 2, bm.bmWidth,
                            scaledHeight + 2);
  }
  else
  {
    FillRect(dc, rect, GetStockObject(0));
  }
}

// FUNCTION: DRAWCARDLIB 0x100081b2
void DrawSmallCardTitle(HDC dc, RECT *rect, char *name, int colorMode, int transparentBackground)
{
  COLORREF titleColor;
  char titleText[100];
  int nSavedDC;

  /* 0x81b2  7  DrawSmallCardTitle */
  if (((dc == (HDC)0x0) || (rect == (RECT *)0x0)) || (name == 0))
  {
    return;
  }
  strcpy(titleText, name);
  nSavedDC = SaveDC(dc);
  SetMapMode(dc, 8);
  SetWindowExtEx(dc, 200, 0x118, (LPSIZE)0x0);
  SetViewportExtEx(dc, rect->right - rect->left, rect->bottom - rect->top, (LPSIZE)0x0);
  SetWindowOrgEx(dc, 0, 0, (LPPOINT)0x0);
  SetViewportOrgEx(dc, rect->left, rect->top, (LPPOINT)0x0);
  if (colorMode == 0)
  {
    titleColor = g_smallCardTitleColor;
  }
  else if (colorMode == 2)
  {
    titleColor = g_smallCardTitleAltColor;
  }
  else
  {
    titleColor = g_smallCardTitleFallbackColor;
  }
  SelectObject(dc, g_smallCardTitleFont);
  SetTextAlign(dc, 0);
  if (transparentBackground == 0)
  {
    SetBkMode(dc, 2);
    SetBkColor(dc, g_smallCardTitleBackgroundColor);
  }
  else
  {
    SetBkMode(dc, 1);
  }
  SetTextColor(dc, g_cardTextShadowColor);
  TextOutA(dc, 5, 1, titleText, strlen(titleText));
  SetTextColor(dc, titleColor);
  SetBkMode(dc, 1);
  TextOutA(dc, 2, -2, titleText, strlen(titleText));
  RestoreDC(dc, nSavedDC);
}

// FUNCTION: DRAWCARDLIB 0x1000ac34
void DeleteAndCloseObject(HANDLE handle)
{
  OSVERSIONINFOA ver;
  HANDLE sectionHandle;
  DIBSECTION dibSection;
  char *bitsPointer;

  if (global_dwPlatformId == -1)
  {
    ver.dwOSVersionInfoSize = 0x94;
    memset(&ver.dwMajorVersion, 0, 0x90);
    GetVersionExA(&ver);
    if (ver.dwPlatformId == 1)
    {
      global_dwPlatformId = 1;
    }
    else
    {
      global_dwPlatformId = 0;
    }
  }
  if (handle != (HANDLE)0x0)
  {
    GetObjectA(handle, 0x54, &dibSection);
    sectionHandle = dibSection.dshSection;
    bitsPointer = (char *)dibSection.dsBm.bmBits;
    bitsPointer += dibSection.dsOffset;
    DeleteObject(handle);
    if (sectionHandle != (HANDLE)0x0)
    {
      if (global_dwPlatformId != 0)
      {
        CloseHandle(sectionHandle);
      }
    }
  }
}

// FUNCTION: DRAWCARDLIB 0x1000b06a
void ReplaceSubstring(char *inOutStr, char *needle, int caseSensitive, char *replacement)
{
  int outLen;
  char outStr[1000];
  size_t needleLen;
  int matched;
  char *cur;

  if (((inOutStr == (char *)0x0) || (needle == (char *)0x0)) || (replacement == (char *)0x0))
  {
    return;
  }

  if (strlen(inOutStr) == 0 || strlen(needle) == 0)
  {
    return;
  }

  needleLen = strlen(needle);
  cur = inOutStr;
  outStr[0] = '\0';
  outLen = 0;
  while (*cur != '\0')
  {
    matched = 0;
    if ((caseSensitive != 0 && strncmp(cur, needle, needleLen) == 0) || (caseSensitive == 0 && _strnicmp(cur, needle, needleLen) == 0))
    {
      matched = 1;
    }
    if (matched != 0)
    {
      strcat(outStr, replacement);
      outLen += strlen(replacement);
      cur += needleLen;
    }
    else
    {
      outStr[outLen] = *cur;
      cur++;
      outLen++;
      outStr[outLen] = '\0';
    }
  }
  strcpy(inOutStr, outStr);
}

// FUNCTION: DRAWCARDLIB 0x1000bd90
#ifndef SHANDALAR_STATIC_LIB
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID reserved)
{
  undefined4 result;

  (void)reserved;
  switch (reason)
  {
  case 1:
    result = 1;
    result &= read_cfg(hinst);
    result &= prepare_fonts_and_imgs();
    return (BOOL)result;
  case 2:
    return (BOOL)1;
  case 3:
    return (BOOL)1;
  case 0:
    DestroyAllResources();
    DrawCardLib_Shutdown();
    return (BOOL)1;
  }
}
#endif

// FUNCTION: DRAWCARDLIB 0x1000be22
int read_cfg(HINSTANCE hinst)
{
  char *lastSlash;
  void *recordBuf;
  char cards_dat_filename[264];
  size_t global_available_slots;
  size_t record_size;
  int slotIndex;
  FILE *cards_dat;
  int record_table[76000];
  int result;

  result = 1;
  g_drawCardLibInstance = hinst;
  InitializeCriticalSection(&global_critical_section_for_catalog);

#ifdef _DEBUG
  // Allow debugging directly from output directory
  if (getcwd(global_base_directory, sizeof(global_base_directory)) == NULL)
  {
    return 0;
  }
#else
  GetModuleFileNameA((HMODULE)0x0, global_base_directory, 0x105);
  lastSlash = strrchr(global_base_directory, '\\');
  *lastSlash = '\0';
#endif

  strcpy(global_cardart_directory, global_base_directory);
  strcat(global_cardart_directory, "\\CARDART");
  global_dither_kernel_id = 3;
  global_color_depth = 0x10;
  InitCardArtGdiResources();
  strcpy(cards_dat_filename, global_base_directory);
  strcat(cards_dat_filename, "\\CARDS.DAT");
  cards_dat = fopen(cards_dat_filename, "rb");
  if (cards_dat != (FILE *)0x0)
  {
    fread(&global_available_slots, 4, 1, cards_dat);
    fread(&record_size, 4, 1, cards_dat);
    fread(record_table, 0x98, global_available_slots, cards_dat);
    recordBuf = malloc(record_size);
    if (recordBuf != (void *)0x0)
    {
      fread(recordBuf, 1, record_size, cards_dat);
      for (slotIndex = 0; (int)global_available_slots > slotIndex;
           slotIndex = slotIndex + 1)
      {
        record_table[slotIndex * 0x26 + 1] = record_table[slotIndex * 0x26 + 1] + (int)recordBuf;
        strncpy(g_cardNames + slotIndex * 100, (char *)record_table[slotIndex * 0x26 + 1], 100);
        g_cardNameSlotTerminators[slotIndex * 100] = 0;
      }
      free(recordBuf);
    }
    else
    {
      result = 0;
    }
    fclose(cards_dat);
  }
  else
  {
    result = 0;
  }
  if (result == 0)
  {
    for (slotIndex = 0; (int)global_available_slots > slotIndex;
         slotIndex = slotIndex + 1)
    {
      strcpy(g_cardNames + slotIndex * 100, "");
    }
  }
  return result;
}

int DrawCardLib_InitializeFromRoot(const char *asset_root)
{
  if (!asset_root || !asset_root[0])
    return 0;
  strncpy(global_base_directory, asset_root, sizeof(global_base_directory) - 1);
  global_base_directory[sizeof(global_base_directory) - 1] = '\0';
  strcpy(global_cardart_directory, global_base_directory);
  strcat(global_cardart_directory, "\\CardArt");
  global_dither_kernel_id = 3;
  global_color_depth = 0x10;
  InitializeCriticalSection(&global_critical_section_for_catalog);
  if (!InitCardArtGdiResources())
    return 0;
  if (!prepare_fonts_and_imgs()) {
    DrawCardLib_Shutdown();
    return 0;
  }
  return 1;
}

// FUNCTION: DRAWCARDLIB 0x1000c0ca
void DrawCardLib_Shutdown(void)
{
  ShutdownCardArtGdiResources();
  DeleteCriticalSection(&global_critical_section_for_catalog);
}
