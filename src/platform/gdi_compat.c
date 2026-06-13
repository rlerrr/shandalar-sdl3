#include "platform/gdi_compat.h"

#include <SDL3_ttf/SDL_ttf.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

enum ObjectKind {
  OBJECT_BITMAP = 0x424d,
  OBJECT_PALETTE = 0x504c,
  OBJECT_FONT = 0x464e,
  OBJECT_BRUSH = 0x4252,
  OBJECT_PEN = 0x504e
};

typedef struct ObjectHeader { int kind; } ObjectHeader;

struct ShandalarBitmap {
  ObjectHeader header;
  int width;
  int height;
  int pitch;
  int bits_per_pixel;
  unsigned char *pixels;
  int owns_pixels;
};

struct ShandalarPalette {
  ObjectHeader header;
  PALETTEENTRY entries[256];
};

struct ShandalarFont {
  ObjectHeader header;
  TTF_Font *font;
  int size;
};

typedef struct PaintObject {
  ObjectHeader header;
  COLORREF color;
} PaintObject;

struct ShandalarDC {
  HBITMAP bitmap;
  HPALETTE palette;
  HFONT font;
  COLORREF text_color;
  COLORREF background_color;
  int background_mode;
  int cursor_x;
  int cursor_y;
  HPEN pen;
  HBRUSH brush;
};

static char g_last_font_path[512];
static char g_font_root[512];

static unsigned char color_to_index(HDC dc, COLORREF color)
{
  unsigned int i;
  unsigned int best = 0;
  unsigned int best_distance = UINT32_MAX;
  int r;
  int g;
  int b;

  if ((color & 0xff000000u) == 0x01000000u)
    return (unsigned char)color;

  r = color & 0xff;
  g = (color >> 8) & 0xff;
  b = (color >> 16) & 0xff;
  if (!dc || !dc->palette)
    return (unsigned char)((r + g + b) / 3);

  for (i = 0; i < 256; ++i) {
    int dr = r - dc->palette->entries[i].peRed;
    int dg = g - dc->palette->entries[i].peGreen;
    int db = b - dc->palette->entries[i].peBlue;
    unsigned int distance = (unsigned int)(dr * dr + dg * dg + db * db);
    if (distance < best_distance) {
      best_distance = distance;
      best = i;
    }
  }
  return (unsigned char)best;
}

static unsigned char *pixel_at(HBITMAP bitmap, int x, int y)
{
  int row;
  if (!bitmap || x < 0 || y < 0 || x >= bitmap->width || y >= bitmap->height)
    return NULL;
  row = bitmap->height - 1 - y;
  return bitmap->pixels + row * bitmap->pitch + x * (bitmap->bits_per_pixel / 8);
}

HDC CreateCompatibleDC(HDC unused)
{
  HDC dc = (HDC)calloc(1, sizeof(*dc));
  if (dc) {
    dc->text_color = PALETTEINDEX(255);
    dc->background_color = PALETTEINDEX(0);
    dc->background_mode = OPAQUE;
  }
  return dc;
}

HDC GetDC(HWND window)
{
  (void)window;
  return CreateCompatibleDC(NULL);
}

int ReleaseDC(HWND window, HDC dc)
{
  (void)window;
  return DeleteDC(dc);
}

BOOL DeleteDC(HDC dc)
{
  free(dc);
  return TRUE;
}

HBITMAP CreateDIBSection(HDC dc, const BITMAPINFO *info, UINT usage, void **bits, HANDLE section, DWORD offset)
{
  HBITMAP bitmap;
  int bytes_per_pixel;
  (void)dc; (void)usage; (void)section; (void)offset;
  if (!info || !bits)
    return NULL;
  bitmap = (HBITMAP)calloc(1, sizeof(*bitmap));
  if (!bitmap)
    return NULL;
  bitmap->header.kind = OBJECT_BITMAP;
  bitmap->width = info->bmiHeader.biWidth;
  bitmap->height = abs(info->bmiHeader.biHeight);
  bitmap->bits_per_pixel = info->bmiHeader.biBitCount;
  bytes_per_pixel = bitmap->bits_per_pixel / 8;
  if (bytes_per_pixel < 1)
    bytes_per_pixel = 1;
  bitmap->pitch = (bitmap->width * bytes_per_pixel + 3) & ~3;
  bitmap->pixels = (unsigned char *)calloc((size_t)bitmap->pitch, (size_t)bitmap->height);
  bitmap->owns_pixels = 1;
  if (!bitmap->pixels) {
    free(bitmap);
    return NULL;
  }
  *bits = bitmap->pixels;
  return bitmap;
}

HBITMAP CreateBitmap(int width, int height, UINT planes, UINT bits_per_pixel, const void *bits)
{
  BITMAPINFO info;
  HBITMAP bitmap;
  void *pixels;
  size_t size;
  memset(&info, 0, sizeof(info));
  info.bmiHeader.biWidth = width;
  info.bmiHeader.biHeight = height;
  info.bmiHeader.biBitCount = (WORD)(planes * bits_per_pixel);
  bitmap = CreateDIBSection(NULL, &info, 0, &pixels, NULL, 0);
  if (bitmap && bits) {
    size = (size_t)bitmap->pitch * (size_t)bitmap->height;
    memcpy(pixels, bits, size);
  }
  return bitmap;
}

HGDIOBJ SelectObject(HDC dc, HGDIOBJ object)
{
  ObjectHeader *header = (ObjectHeader *)object;
  HGDIOBJ previous = NULL;
  if (!dc || !header)
    return NULL;
  switch (header->kind) {
  case OBJECT_BITMAP: previous = dc->bitmap; dc->bitmap = (HBITMAP)object; break;
  case OBJECT_FONT: previous = dc->font; dc->font = (HFONT)object; break;
  case OBJECT_PEN: previous = dc->pen; dc->pen = (HPEN)object; break;
  case OBJECT_BRUSH: previous = dc->brush; dc->brush = (HBRUSH)object; break;
  default: break;
  }
  return previous;
}

BOOL DeleteObject(HGDIOBJ object)
{
  ObjectHeader *header = (ObjectHeader *)object;
  if (!header)
    return FALSE;
  if (header->kind == OBJECT_BITMAP) {
    HBITMAP bitmap = (HBITMAP)object;
    if (bitmap->owns_pixels)
      free(bitmap->pixels);
  } else if (header->kind == OBJECT_FONT) {
    HFONT font = (HFONT)object;
    TTF_CloseFont(font->font);
  }
  free(object);
  return TRUE;
}

int GetObjectA(HGDIOBJ object, int size, void *out)
{
  ObjectHeader *header = (ObjectHeader *)object;
  if (!header || !out)
    return 0;
  if (header->kind == OBJECT_BITMAP && size >= (int)sizeof(BITMAP)) {
    HBITMAP bitmap = (HBITMAP)object;
    BITMAP *result = (BITMAP *)out;
    memset(result, 0, sizeof(*result));
    result->bmWidth = bitmap->width;
    result->bmHeight = bitmap->height;
    result->bmWidthBytes = bitmap->pitch;
    result->bmPlanes = 1;
    result->bmBitsPixel = (WORD)bitmap->bits_per_pixel;
    result->bmBits = bitmap->pixels;
    return sizeof(*result);
  }
  return 0;
}

static void apply_rop(unsigned char *dst, unsigned char src, DWORD rop)
{
  if (rop == SRCAND)
    *dst &= src;
  else if (rop == SRCPAINT)
    *dst |= src;
  else
    *dst = src;
}

BOOL BitBlt(HDC dst, int dx, int dy, int width, int height, HDC src, int sx, int sy, DWORD rop)
{
  int x, y;
  unsigned char *temp;
  if (!dst || !src || !dst->bitmap || !src->bitmap || width <= 0 || height <= 0)
    return FALSE;
  temp = (unsigned char *)malloc((size_t)width * (size_t)height);
  if (!temp)
    return FALSE;
  for (y = 0; y < height; ++y)
    for (x = 0; x < width; ++x) {
      unsigned char *pixel = pixel_at(src->bitmap, sx + x, sy + y);
      temp[y * width + x] = pixel ? *pixel : 0;
    }
  for (y = 0; y < height; ++y)
    for (x = 0; x < width; ++x) {
      unsigned char *pixel = pixel_at(dst->bitmap, dx + x, dy + y);
      if (pixel)
        apply_rop(pixel, temp[y * width + x], rop);
    }
  free(temp);
  return TRUE;
}

BOOL StretchBlt(HDC dst, int dx, int dy, int dw, int dh, HDC src, int sx, int sy, int sw, int sh, DWORD rop)
{
  int x, y;
  if (!dst || !src || !dst->bitmap || !src->bitmap || dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0)
    return FALSE;
  for (y = 0; y < dh; ++y)
    for (x = 0; x < dw; ++x) {
      unsigned char *source = pixel_at(src->bitmap, sx + x * sw / dw, sy + y * sh / dh);
      unsigned char *target = pixel_at(dst->bitmap, dx + x, dy + y);
      if (source && target)
        apply_rop(target, *source, rop);
    }
  return TRUE;
}

COLORREF GetPixel(HDC dc, int x, int y)
{
  unsigned char *pixel = dc ? pixel_at(dc->bitmap, x, y) : NULL;
  if (!pixel)
    return UINT32_MAX;
  if (dc->palette) {
    PALETTEENTRY *entry = &dc->palette->entries[*pixel];
    return RGB(entry->peRed, entry->peGreen, entry->peBlue);
  }
  return RGB(*pixel, *pixel, *pixel);
}

BOOL SetPixelV(HDC dc, int x, int y, COLORREF color)
{
  unsigned char *pixel = dc ? pixel_at(dc->bitmap, x, y) : NULL;
  if (!pixel)
    return FALSE;
  *pixel = color_to_index(dc, color);
  return TRUE;
}

static PaintObject *create_paint(int kind, COLORREF color)
{
  PaintObject *object = (PaintObject *)calloc(1, sizeof(*object));
  if (object) {
    object->header.kind = kind;
    object->color = color;
  }
  return object;
}

HBRUSH CreateSolidBrush(COLORREF color) { return create_paint(OBJECT_BRUSH, color); }
HBRUSH CreateBrushIndirect(const LOGBRUSH *brush) { return CreateSolidBrush(brush ? brush->lbColor : 0); }
HPEN CreatePen(int style, int width, COLORREF color) { (void)style; (void)width; return create_paint(OBJECT_PEN, color); }
HPEN CreatePenIndirect(const LOGPEN *pen) { return CreatePen(0, 1, pen ? pen->lopnColor : 0); }

int FillRect(HDC dc, const RECT *rect, HBRUSH brush)
{
  int x, y;
  PaintObject *paint = (PaintObject *)brush;
  unsigned char index = color_to_index(dc, paint ? paint->color : 0);
  if (!dc || !dc->bitmap || !rect)
    return 0;
  for (y = rect->top; y < rect->bottom; ++y)
    for (x = rect->left; x < rect->right; ++x) {
      unsigned char *pixel = pixel_at(dc->bitmap, x, y);
      if (pixel) *pixel = index;
    }
  return 1;
}

BOOL MoveToEx(HDC dc, int x, int y, POINT *old)
{
  if (!dc) return FALSE;
  if (old) { old->x = dc->cursor_x; old->y = dc->cursor_y; }
  dc->cursor_x = x; dc->cursor_y = y;
  return TRUE;
}

BOOL LineTo(HDC dc, int x1, int y1)
{
  int x0, y0, dx, sx, dy, sy, err;
  COLORREF color = dc && dc->pen ? ((PaintObject *)dc->pen)->color : PALETTEINDEX(255);
  if (!dc) return FALSE;
  x0 = dc->cursor_x; y0 = dc->cursor_y;
  dx = abs(x1 - x0); sx = x0 < x1 ? 1 : -1;
  dy = -abs(y1 - y0); sy = y0 < y1 ? 1 : -1;
  err = dx + dy;
  for (;;) {
    SetPixelV(dc, x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    if (2 * err >= dy) { err += dy; x0 += sx; }
    if (2 * err <= dx) { err += dx; y0 += sy; }
  }
  dc->cursor_x = x1; dc->cursor_y = y1;
  return TRUE;
}

BOOL Rectangle(HDC dc, int left, int top, int right, int bottom)
{
  MoveToEx(dc, left, top, NULL); LineTo(dc, right - 1, top);
  LineTo(dc, right - 1, bottom - 1); LineTo(dc, left, bottom - 1); LineTo(dc, left, top);
  return TRUE;
}

BOOL Ellipse(HDC dc, int left, int top, int right, int bottom)
{
  int i;
  double rx = (right - left) / 2.0, ry = (bottom - top) / 2.0;
  double cx = left + rx, cy = top + ry;
  for (i = 0; i < 360; ++i)
    SetPixelV(dc, (int)(cx + cos(i * 3.141592653589793 / 180.0) * rx),
              (int)(cy + sin(i * 3.141592653589793 / 180.0) * ry),
              dc && dc->pen ? ((PaintObject *)dc->pen)->color : PALETTEINDEX(255));
  return TRUE;
}

HGDIOBJ GetStockObject(int object)
{
  COLORREF color = object == WHITE_BRUSH ? RGB(255,255,255) : RGB(0,0,0);
  if (object == DEFAULT_GUI_FONT || object == SYSTEM_FONT)
    return NULL;
  if (object == NULL_PEN)
    return CreatePen(0, 0, color);
  return CreateSolidBrush(color);
}

HPALETTE CreatePalette(const LOGPALETTE *palette)
{
  HPALETTE result = (HPALETTE)calloc(1, sizeof(*result));
  unsigned int count;
  if (!result) return NULL;
  result->header.kind = OBJECT_PALETTE;
  count = palette ? palette->palNumEntries : 0;
  if (count > 256) count = 256;
  if (palette) memcpy(result->entries, palette->palPalEntry, count * sizeof(PALETTEENTRY));
  return result;
}

HPALETTE SelectPalette(HDC dc, HPALETTE palette, BOOL background)
{
  HPALETTE previous;
  (void)background;
  if (!dc) return NULL;
  previous = dc->palette; dc->palette = palette;
  return previous;
}
UINT RealizePalette(HDC dc) { return dc && dc->palette ? 256 : 0; }

UINT GetPaletteEntries(HPALETTE palette, UINT start, UINT count, PALETTEENTRY *entries)
{
  if (!palette || start >= 256) return 0;
  if (count > 256 - start) count = 256 - start;
  if (entries) memcpy(entries, palette->entries + start, count * sizeof(*entries));
  return count;
}

UINT SetPaletteEntries(HPALETTE palette, UINT start, UINT count, const PALETTEENTRY *entries)
{
  if (!palette || !entries || start >= 256) return 0;
  if (count > 256 - start) count = 256 - start;
  memcpy(palette->entries + start, entries, count * sizeof(*entries));
  return count;
}

UINT SetDIBColorTable(HDC dc, UINT start, UINT count, const RGBQUAD *colors)
{
  UINT i;
  if (!dc || !dc->palette || !colors) return 0;
  if (count > 256 - start) count = 256 - start;
  for (i = 0; i < count; ++i) {
    dc->palette->entries[start + i].peRed = colors[i].rgbRed;
    dc->palette->entries[start + i].peGreen = colors[i].rgbGreen;
    dc->palette->entries[start + i].peBlue = colors[i].rgbBlue;
  }
  return count;
}
int SetStretchBltMode(HDC dc, int mode) { (void)dc; return mode; }

COLORREF SetTextColor(HDC dc, COLORREF color) { COLORREF old = dc ? dc->text_color : 0; if (dc) dc->text_color = color; return old; }
COLORREF SetBkColor(HDC dc, COLORREF color) { COLORREF old = dc ? dc->background_color : 0; if (dc) dc->background_color = color; return old; }
int SetBkMode(HDC dc, int mode) { int old = dc ? dc->background_mode : 0; if (dc) dc->background_mode = mode; return old; }

BOOL GetTextExtentPoint32A(HDC dc, LPCSTR text, int length, SIZE *size)
{
  if (!size) return FALSE;
  if (length < 0) length = (int)strlen(text);
  if (dc && dc->font && dc->font->font)
    return TTF_GetStringSize(dc->font->font, text, (size_t)length, (int *)&size->cx, (int *)&size->cy);
  size->cx = length * 8; size->cy = 12;
  return TRUE;
}

BOOL GetTextMetricsA(HDC dc, TEXTMETRICA *metrics)
{
  if (!metrics) return FALSE;
  memset(metrics, 0, sizeof(*metrics));
  if (dc && dc->font && dc->font->font) {
    metrics->tmHeight = TTF_GetFontHeight(dc->font->font);
    metrics->tmAscent = TTF_GetFontAscent(dc->font->font);
    metrics->tmDescent = -TTF_GetFontDescent(dc->font->font);
    metrics->tmInternalLeading = metrics->tmHeight - metrics->tmAscent - metrics->tmDescent;
  } else {
    metrics->tmHeight = 12; metrics->tmAscent = 10; metrics->tmDescent = 2;
  }
  return TRUE;
}

BOOL GetCharABCWidthsA(HDC dc, UINT first, UINT last, ABC *widths)
{
  UINT ch;
  if (!widths) return FALSE;
  for (ch = first; ch <= last; ++ch) {
    char text[2] = {(char)ch, 0};
    SIZE size;
    GetTextExtentPoint32A(dc, text, 1, &size);
    widths[ch - first].abcA = 0; widths[ch - first].abcB = (UINT)size.cx; widths[ch - first].abcC = 0;
  }
  return TRUE;
}

BOOL TextOutA(HDC dc, int x, int y, LPCSTR text, int length)
{
  SDL_Surface *surface;
  SDL_Color white = {255,255,255,255};
  int px, py;
  unsigned char index;
  if (!dc || !dc->bitmap || !dc->font || !dc->font->font || !text) return FALSE;
  if (length < 0) length = (int)strlen(text);
  surface = TTF_RenderText_Blended(dc->font->font, text, (size_t)length, white);
  if (!surface) return FALSE;
  index = color_to_index(dc, dc->text_color);
  for (py = 0; py < surface->h; ++py)
    for (px = 0; px < surface->w; ++px) {
      uint32_t pixel = ((uint32_t *)((unsigned char *)surface->pixels + py * surface->pitch))[px];
      if ((pixel >> 24) > 32) {
        unsigned char *target = pixel_at(dc->bitmap, x + px, y + py);
        if (target) *target = index;
      }
    }
  SDL_DestroySurface(surface);
  return TRUE;
}

int DrawTextA(HDC dc, LPCSTR text, int length, RECT *rect, UINT format)
{
  (void)format;
  if (!rect) return 0;
  return TextOutA(dc, rect->left, rect->top, text, length);
}

int AddFontResourceA(LPCSTR path)
{
  if (!path) return 0;
  SDL_strlcpy(g_last_font_path, path, sizeof(g_last_font_path));
  return 1;
}

static HFONT open_font(int size)
{
  HFONT result = (HFONT)calloc(1, sizeof(*result));
  char path[1024];
  if (!result) return NULL;
  if (!TTF_Init()) {
    free(result);
    return NULL;
  }
  result->header.kind = OBJECT_FONT;
  result->size = size > 0 ? size : 12;
  if (g_last_font_path[0] && g_font_root[0] && !strchr(g_last_font_path, '/') && !strchr(g_last_font_path, '\\')) {
    SDL_snprintf(path, sizeof(path), "%s/%s", g_font_root, g_last_font_path);
    result->font = TTF_OpenFont(path, (float)result->size);
  }
  if (!result->font && g_last_font_path[0])
    result->font = TTF_OpenFont(g_last_font_path, (float)result->size);
  return result;
}

HFONT CreateFontA(int height, int width, int escapement, int orientation, int weight,
                  DWORD italic, DWORD underline, DWORD strikeout, DWORD charset,
                  DWORD out_precision, DWORD clip_precision, DWORD quality,
                  DWORD pitch_family, LPCSTR face)
{
  (void)width; (void)escapement; (void)orientation; (void)weight; (void)italic;
  (void)underline; (void)strikeout; (void)charset; (void)out_precision;
  (void)clip_precision; (void)quality; (void)pitch_family; (void)face;
  return open_font(abs(height));
}

HFONT CreateFontIndirectA(const LOGFONTA *font) { return open_font(font ? abs(font->lfHeight) : 12); }

int SetDIBitsToDevice(HDC dc, int x, int y, DWORD width, DWORD height,
                      int src_x, int src_y, UINT start_scan, UINT scan_lines,
                      const void *bits, const BITMAPINFO *info, UINT usage)
{
  int row;
  int src_pitch;
  (void)src_x; (void)src_y; (void)start_scan; (void)scan_lines; (void)usage;
  if (!dc || !dc->bitmap || !bits || !info) return 0;
  src_pitch = ((int)width + 3) & ~3;
  for (row = 0; row < (int)height; ++row) {
    int col;
    for (col = 0; col < (int)width; ++col) {
      unsigned char *target = pixel_at(dc->bitmap, x + col, y + row);
      if (target) *target = ((const unsigned char *)bits)[row * src_pitch + col];
    }
  }
  return (int)height;
}

int MessageBoxA(HWND window, LPCSTR message, LPCSTR title, UINT flags)
{
  (void)window;
  (void)flags;
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title ? title : "Facemaker",
                           message ? message : "", NULL);
  return 1;
}

BOOL CloseHandle(HANDLE handle) { (void)handle; return TRUE; }
void *GlobalAlloc(UINT flags, size_t size) { (void)flags; return malloc(size); }
void *GlobalLock(void *memory) { return memory; }
BOOL GlobalUnlock(void *memory) { (void)memory; return TRUE; }
void *GlobalHandle(void *memory) { return memory; }
void *GlobalFree(void *memory) { free(memory); return NULL; }
BOOL GdiFlush(void) { return TRUE; }
int GetDeviceCaps(HDC dc, int index)
{
  if (index == BITSPIXEL)
    return 8;
  if (!dc || !dc->bitmap)
    return 0;
  if (index == HORZRES)
    return dc->bitmap->width;
  if (index == VERTRES)
    return dc->bitmap->height;
  return 0;
}

HDC shandalar_create_screen_dc(int width, int height)
{
  BITMAPINFO info;
  HDC dc = CreateCompatibleDC(NULL);
  HBITMAP bitmap;
  void *bits;
  memset(&info, 0, sizeof(info));
  info.bmiHeader.biWidth = width;
  info.bmiHeader.biHeight = height;
  info.bmiHeader.biBitCount = 8;
  bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, NULL, 0);
  SelectObject(dc, bitmap);
  return dc;
}

void shandalar_destroy_screen_dc(HDC dc)
{
  if (!dc)
    return;
  DeleteObject(dc->bitmap);
  DeleteDC(dc);
}

void shandalar_set_screen_dc(HDC dc) { (void)dc; }
const void *shandalar_dc_pixels(HDC dc, int *pitch)
{
  if (!dc || !dc->bitmap) return NULL;
  if (pitch) *pitch = dc->bitmap->pitch;
  return dc->bitmap->pixels;
}
void shandalar_set_default_font_root(const char *root)
{
  SDL_strlcpy(g_font_root, root ? root : "", sizeof(g_font_root));
}
