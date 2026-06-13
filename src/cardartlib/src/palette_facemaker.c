#include "palette.h"

#include <string.h>

CRITICAL_SECTION global_critical_section_for_drawing;
HDC global_screen_dc;
RGBQUAD g_cardArtPalette[256];

BOOL InitCardArtGdiResources(void)
{
  memset(g_cardArtPalette, 0, sizeof(g_cardArtPalette));
  return TRUE;
}

void ShutdownCardArtGdiResources(void)
{
}

void ApplyCardArtPaletteToDc(HDC hdc)
{
  SetDIBColorTable(hdc, 0, 256, g_cardArtPalette);
}

COLORREF GetPaletteColor(int index)
{
  RGBQUAD *entry;
  if (index < 0 || index >= 256)
    return 0;
  entry = &g_cardArtPalette[index];
  return RGB(entry->rgbRed, entry->rgbGreen, entry->rgbBlue);
}

BOOL SetupDuelPalette(void)
{
  return TRUE;
}
