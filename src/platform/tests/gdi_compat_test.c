#include "platform/gdi_compat.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expression) do { if (!(expression)) { \
  fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expression); \
  return __LINE__; \
} } while (0)

int main(void)
{
  struct {
    WORD version;
    WORD count;
    PALETTEENTRY entries[256];
  } palette_data;
  PALETTEENTRY changed_entries[3];
  BITMAPINFO info;
  HPALETTE palette;
  HDC destination;
  HDC source;
  HBITMAP source_bitmap;
  HBITMAP temporary_bitmap;
  void *source_bits;
  unsigned char *bgr;
  uint32_t rgba[16];

  memset(&palette_data, 0, sizeof(palette_data));
  palette_data.version = 0x300;
  palette_data.count = 256;
  palette_data.entries[42].peRed = 255;
  palette_data.entries[43].peGreen = 255;
  palette = CreatePalette((const LOGPALETTE *)&palette_data);
  destination = shandalar_create_screen_dc(4, 4);
  CHECK(palette && destination);
  CHECK(SelectPalette(destination, palette, FALSE) == NULL);

  memset(&info, 0, sizeof(info));
  info.bmiHeader.biSize = sizeof(info.bmiHeader);
  info.bmiHeader.biWidth = 2;
  info.bmiHeader.biHeight = -2;
  info.bmiHeader.biPlanes = 1;
  info.bmiHeader.biBitCount = 24;
  info.bmiHeader.biCompression = BI_RGB;
  source_bitmap = CreateDIBSection(NULL, &info, DIB_RGB_COLORS,
                                   &source_bits, NULL, 0);
  source = CreateCompatibleDC(NULL);
  CHECK(source_bitmap && source && source_bits);
  SelectObject(source, source_bitmap);

  CHECK(SaveDC(source) == 1);
  temporary_bitmap = CreateCompatibleBitmap(source, 1, 1);
  CHECK(temporary_bitmap);
  CHECK(SelectObject(source, temporary_bitmap) == source_bitmap);
  CHECK(DeleteObject(source_bitmap));
  CHECK(RestoreDC(source, -1));
  CHECK(DeleteObject(temporary_bitmap));

  bgr = (unsigned char *)source_bits;
  bgr[0] = 0; bgr[1] = 0; bgr[2] = 255;
  bgr[3] = 0; bgr[4] = 255; bgr[5] = 0;
  bgr[8] = 0; bgr[9] = 255; bgr[10] = 0;
  bgr[11] = 0; bgr[12] = 0; bgr[13] = 255;

  CHECK(IntersectClipRect(destination, 1, 0, 3, 4));
  CHECK(StretchBlt(destination, 0, 0, 4, 4, source, 0, 0, 2, 2,
                   SRCCOPY));
  CHECK(GetPixel(destination, 0, 0) == RGB(0, 0, 0));
  CHECK(GetPixel(destination, 1, 0) == RGB(255, 0, 0));
  CHECK(GetPixel(destination, 2, 0) == RGB(0, 255, 0));
  CHECK(GetPixel(destination, 3, 0) == RGB(0, 0, 0));
  CHECK(GetPixel(destination, 1, 3) == RGB(0, 255, 0));
  CHECK(GetPixel(destination, 2, 3) == RGB(255, 0, 0));

  changed_entries[0] = palette_data.entries[42];
  changed_entries[0].peRed = 0;
  changed_entries[1] = palette_data.entries[43];
  changed_entries[2] = palette_data.entries[44];
  changed_entries[2].peRed = 255;
  CHECK(SetPaletteEntries(palette, 42, 3, changed_entries) == 3);
  CHECK(StretchBlt(destination, 1, 0, 1, 1, source, 0, 0, 1, 1,
                   SRCCOPY));
  changed_entries[0].peBlue = 255;
  CHECK(SetPaletteEntries(palette, 42, 1, changed_entries) == 1);
  CHECK(GetPixel(destination, 1, 0) == RGB(255, 0, 0));
  CHECK(SetPixelV(destination, 0, 0, PALETTEINDEX(42)));
  CHECK(shandalar_dc_copy_rgba(destination, rgba, 4 * sizeof(rgba[0])));
  CHECK(rgba[0] == 0xff0000ffu);

  CHECK(DeleteDC(source));
  shandalar_destroy_screen_dc(destination);
  CHECK(DeleteObject(palette));
  return 0;
}
