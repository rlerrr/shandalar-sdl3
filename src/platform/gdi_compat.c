#include "platform/gdi_compat.h"

#include <SDL3_ttf/SDL_ttf.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>

enum ObjectKind {
  OBJECT_BITMAP = 0x424d,
  OBJECT_PALETTE = 0x504c,
  OBJECT_FONT = 0x464e,
  OBJECT_BRUSH = 0x4252,
  OBJECT_PEN = 0x504e,
  OBJECT_REGION = 0x5247
};

typedef struct ObjectHeader { int kind; } ObjectHeader;

struct ShandalarBitmap {
  ObjectHeader header;
  int width;
  int height;
  int pitch;
  int bits_per_pixel;
  int top_down;
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
  double font_size_to_cell_height;
  LOGFONTA description;
};

typedef struct DCState {
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
  int map_mode;
  int window_ext_x;
  int window_ext_y;
  int viewport_ext_x;
  int viewport_ext_y;
  int window_org_x;
  int window_org_y;
  int viewport_org_x;
  int viewport_org_y;
  int has_clip;
  RECT clip;
} DCState;

typedef struct PaintObject {
  ObjectHeader header;
  COLORREF color;
  int is_null;
} PaintObject;

typedef struct CompatRegion {
  ObjectHeader header;
  RECT rect;
} CompatRegion;

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
  int map_mode;
  int window_ext_x;
  int window_ext_y;
  int viewport_ext_x;
  int viewport_ext_y;
  int window_org_x;
  int window_org_y;
  int viewport_org_x;
  int viewport_org_y;
  int has_clip;
  RECT clip;
  DCState saved[16];
  int saved_count;
};

static char g_font_root[512];
static char g_module_path[1024] = ".\\shandalar.exe";
static double g_font_device_scale = 1.0;

static unsigned int cp1252_codepoint(unsigned char ch)
{
  static const unsigned short map[32] = {
    0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
    0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008d, 0x017d, 0x008f,
    0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x017e, 0x0178
  };
  if (ch >= 0x80 && ch <= 0x9f)
    return map[ch - 0x80];
  return ch;
}

static int utf8_length_for_codepoint(unsigned int codepoint)
{
  if (codepoint < 0x80)
    return 1;
  if (codepoint < 0x800)
    return 2;
  if (codepoint < 0x10000)
    return 3;
  return 4;
}

static char *utf8_from_windows_text(LPCSTR text, int length, int *out_length)
{
  int i;
  int needed = 0;
  int pos = 0;
  char *utf8;
  if (!text)
    return NULL;
  if (length < 0)
    length = (int)strlen(text);
  for (i = 0; i < length; ++i)
    needed += utf8_length_for_codepoint(cp1252_codepoint((unsigned char)text[i]));
  utf8 = (char *)malloc((size_t)needed + 1);
  if (!utf8)
    return NULL;
  for (i = 0; i < length; ++i) {
    unsigned int cp = cp1252_codepoint((unsigned char)text[i]);
    if (cp < 0x80) {
      utf8[pos++] = (char)cp;
    } else if (cp < 0x800) {
      utf8[pos++] = (char)(0xc0 | (cp >> 6));
      utf8[pos++] = (char)(0x80 | (cp & 0x3f));
    } else if (cp < 0x10000) {
      utf8[pos++] = (char)(0xe0 | (cp >> 12));
      utf8[pos++] = (char)(0x80 | ((cp >> 6) & 0x3f));
      utf8[pos++] = (char)(0x80 | (cp & 0x3f));
    } else {
      utf8[pos++] = (char)(0xf0 | (cp >> 18));
      utf8[pos++] = (char)(0x80 | ((cp >> 12) & 0x3f));
      utf8[pos++] = (char)(0x80 | ((cp >> 6) & 0x3f));
      utf8[pos++] = (char)(0x80 | (cp & 0x3f));
    }
  }
  utf8[pos] = '\0';
  if (out_length)
    *out_length = pos;
  return utf8;
}

static unsigned char color_to_index(HDC dc, COLORREF color)
{
  unsigned int i;
  unsigned int best = 0;
  unsigned int best_distance = UINT32_MAX;
  int r;
  int g;
  int b;

  if ((color & 0xff000000u) == 0x01000000u)
    return (unsigned char)(color & 0xffu);
  color &= 0x00ffffffu;

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

static void apply_rop(unsigned char *dst, unsigned char src, DWORD rop);

static unsigned char *pixel_at(HBITMAP bitmap, int x, int y)
{
  int row;
  if (!bitmap || x < 0 || y < 0 || x >= bitmap->width || y >= bitmap->height)
    return NULL;
  row = bitmap->top_down ? y : bitmap->height - 1 - y;
  return bitmap->pixels + row * bitmap->pitch + x * (bitmap->bits_per_pixel / 8);
}

static COLORREF read_pixel_color(HDC dc, int x, int y)
{
  unsigned char *pixel = dc && dc->bitmap ? pixel_at(dc->bitmap, x, y) : NULL;
  if (!pixel)
    return UINT32_MAX;
  if (dc->bitmap->bits_per_pixel == 24)
    return RGB(pixel[2], pixel[1], pixel[0]);
  if (dc->palette) {
    PALETTEENTRY *entry = &dc->palette->entries[*pixel];
    return RGB(entry->peRed, entry->peGreen, entry->peBlue);
  }
  return RGB(*pixel, *pixel, *pixel);
}

static COLORREF resolve_color(HDC dc, COLORREF color)
{
  if ((color & 0xff000000u) == 0x01000000u && dc && dc->palette) {
    PALETTEENTRY *entry = &dc->palette->entries[color & 0xff];
    return RGB(entry->peRed, entry->peGreen, entry->peBlue);
  }
  return color & 0x00ffffffu;
}

static void write_pixel_color(HDC dc, int x, int y, COLORREF color, DWORD rop)
{
  unsigned char *pixel = dc && dc->bitmap ? pixel_at(dc->bitmap, x, y) : NULL;
  if (dc && dc->has_clip &&
      (x < dc->clip.left || x >= dc->clip.right || y < dc->clip.top || y >= dc->clip.bottom))
    return;
  if (!pixel)
    return;
  if (dc->bitmap->bits_per_pixel == 24) {
    unsigned char bgr[3] = {
      (unsigned char)((color >> 16) & 0xff),
      (unsigned char)((color >> 8) & 0xff),
      (unsigned char)(color & 0xff)
    };
    int i;
    for (i = 0; i < 3; ++i)
      apply_rop(&pixel[i], bgr[i], rop);
  } else {
    apply_rop(pixel, color_to_index(dc, color), rop);
  }
}

HDC CreateCompatibleDC(HDC unused)
{
  HDC dc = (HDC)calloc(1, sizeof(*dc));
  if (dc) {
    dc->text_color = PALETTEINDEX(255);
    dc->background_color = PALETTEINDEX(0);
    dc->background_mode = OPAQUE;
    dc->window_ext_x = dc->viewport_ext_x = 1;
    dc->window_ext_y = dc->viewport_ext_y = 1;
  }
  return dc;
}

static int map_x(HDC dc, int x)
{
  if (!dc)
    return x;
  if (dc->map_mode == MM_TEXT)
    return dc->viewport_org_x + x - dc->window_org_x;
  return dc->viewport_org_x +
         (x - dc->window_org_x) * dc->viewport_ext_x / dc->window_ext_x;
}

static int map_y(HDC dc, int y)
{
  if (!dc)
    return y;
  if (dc->map_mode == MM_TEXT)
    return dc->viewport_org_y + y - dc->window_org_y;
  return dc->viewport_org_y +
         (y - dc->window_org_y) * dc->viewport_ext_y / dc->window_ext_y;
}

static double device_scale_x(HDC dc)
{
  if (!dc || dc->map_mode == MM_TEXT || !dc->window_ext_x)
    return 1.0;
  return fabs((double)dc->viewport_ext_x / (double)dc->window_ext_x);
}

static double device_scale_y(HDC dc)
{
  if (!dc || dc->map_mode == MM_TEXT || !dc->window_ext_y)
    return 1.0;
  return fabs((double)dc->viewport_ext_y / (double)dc->window_ext_y);
}

static int logical_from_device(int value, double scale)
{
  if (scale <= 0.0)
    return value;
  return (int)lround((double)value / scale);
}

static float prepare_font_for_dc(HDC dc)
{
  float old_size;
  float device_size;
  if (!dc || !dc->font || !dc->font->font)
    return 0.0f;
  old_size = TTF_GetFontSize(dc->font->font);
  device_size = (float)((double)dc->font->size * dc->font->font_size_to_cell_height *
                        device_scale_y(dc) * g_font_device_scale);
  if (device_size < 1.0f)
    device_size = 1.0f;
  if (fabsf(old_size - device_size) > 0.01f)
    TTF_SetFontSize(dc->font->font, device_size);
  return old_size;
}

static void restore_font_size(HDC dc, float old_size)
{
  if (dc && dc->font && dc->font->font && old_size > 0.0f &&
      fabsf(TTF_GetFontSize(dc->font->font) - old_size) > 0.01f)
    TTF_SetFontSize(dc->font->font, old_size);
}

HDC WINAPI shandalar_GetDC(HWND window)
{
  extern HDC shandalar_window_backing_dc(HWND window);
  HDC window_dc = shandalar_window_backing_dc(window);
  if (window_dc)
    return window_dc;
  return CreateCompatibleDC(NULL);
}

int ReleaseDC(HWND window, HDC dc)
{
  extern HDC shandalar_window_backing_dc(HWND window);
  if (window && dc == shandalar_window_backing_dc(window))
    return 1;
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
  bitmap->top_down = info->bmiHeader.biHeight < 0;
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

HBITMAP CreateCompatibleBitmap(HDC dc, int width, int height)
{
  UINT bits_per_pixel = 8;
  if (dc && dc->bitmap && dc->bitmap->bits_per_pixel > 0)
    bits_per_pixel = (UINT)dc->bitmap->bits_per_pixel;
  return CreateBitmap(width, height, 1, bits_per_pixel, NULL);
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

int WINAPI shandalar_GetObjectA(HGDIOBJ object, int size, void *out)
{
  ObjectHeader *header = (ObjectHeader *)object;
  if (!header || !out)
    return 0;
  if (header->kind == OBJECT_BITMAP && size >= (int)sizeof(BITMAP)) {
    HBITMAP bitmap = (HBITMAP)object;
    if (size >= (int)sizeof(DIBSECTION)) {
      DIBSECTION *result = (DIBSECTION *)out;
      memset(result, 0, sizeof(*result));
      result->dsBm.bmWidth = bitmap->width;
      result->dsBm.bmHeight = bitmap->height;
      result->dsBm.bmWidthBytes = bitmap->pitch;
      result->dsBm.bmPlanes = 1;
      result->dsBm.bmBitsPixel = (WORD)bitmap->bits_per_pixel;
      result->dsBm.bmBits = bitmap->pixels;
      result->dsBmih.biSize = sizeof(result->dsBmih);
      result->dsBmih.biWidth = bitmap->width;
      result->dsBmih.biHeight = bitmap->height;
      result->dsBmih.biBitCount = (WORD)bitmap->bits_per_pixel;
      return sizeof(*result);
    } else {
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
  }
  if (header->kind == OBJECT_FONT && size > 0) {
    HFONT font = (HFONT)object;
    size_t copy_size = (size_t)size;
    if (copy_size > sizeof(font->description))
      copy_size = sizeof(font->description);
    memset(out, 0, (size_t)size);
    memcpy(out, &font->description, copy_size);
    return (int)copy_size;
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

static int can_copy_indexed_pixels_directly(HDC dst, HDC src)
{
  if (!dst || !src)
    return 0;
  return dst->palette == src->palette;
}

BOOL BitBlt(HDC dst, int dx, int dy, int width, int height, HDC src, int sx, int sy, DWORD rop)
{
  int x, y;
  COLORREF *temp;
  if (!dst || !src || !dst->bitmap || !src->bitmap || width <= 0 || height <= 0)
    return FALSE;
  if (dx < 0) { sx -= dx; width += dx; dx = 0; }
  if (dy < 0) { sy -= dy; height += dy; dy = 0; }
  if (sx < 0) { dx -= sx; width += sx; sx = 0; }
  if (sy < 0) { dy -= sy; height += sy; sy = 0; }
  if (width > dst->bitmap->width - dx) width = dst->bitmap->width - dx;
  if (width > src->bitmap->width - sx) width = src->bitmap->width - sx;
  if (height > dst->bitmap->height - dy) height = dst->bitmap->height - dy;
  if (height > src->bitmap->height - sy) height = src->bitmap->height - sy;
  if (width <= 0 || height <= 0)
    return TRUE;
  if (dst->bitmap->bits_per_pixel == 8 && src->bitmap->bits_per_pixel == 8 &&
      can_copy_indexed_pixels_directly(dst, src) &&
      (rop == SRCCOPY || rop == SRCAND || rop == SRCPAINT)) {
    for (y = 0; y < height; ++y) {
      unsigned char *target = pixel_at(dst->bitmap, dx, dy + y);
      unsigned char *source = pixel_at(src->bitmap, sx, sy + y);
      if (!target || !source)
        continue;
      if (rop == SRCCOPY) {
        memmove(target, source, (size_t)width);
      } else {
        for (x = 0; x < width; ++x)
          apply_rop(&target[x], source[x], rop);
      }
    }
    return TRUE;
  }
  temp = (COLORREF *)malloc((size_t)width * (size_t)height * sizeof(*temp));
  if (!temp)
    return FALSE;
  for (y = 0; y < height; ++y)
    for (x = 0; x < width; ++x) {
      temp[y * width + x] = read_pixel_color(src, sx + x, sy + y);
    }
  for (y = 0; y < height; ++y)
    for (x = 0; x < width; ++x) {
      write_pixel_color(dst, dx + x, dy + y, temp[y * width + x], rop);
    }
  free(temp);
  return TRUE;
}

BOOL StretchBlt(HDC dst, int dx, int dy, int dw, int dh, HDC src, int sx, int sy, int sw, int sh, DWORD rop)
{
  int x, y;
  int right;
  int bottom;
  int logical_dx = dx;
  int logical_dy = dy;
  right = map_x(dst, logical_dx + dw);
  bottom = map_y(dst, logical_dy + dh);
  dx = map_x(dst, logical_dx);
  dy = map_y(dst, logical_dy);
  dw = right - dx;
  dh = bottom - dy;
  if (!dst || !src || !dst->bitmap || !src->bitmap || dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0)
    return FALSE;
  if (dst->bitmap->bits_per_pixel == 8 && src->bitmap->bits_per_pixel == 8 &&
      can_copy_indexed_pixels_directly(dst, src) &&
      (rop == SRCCOPY || rop == SRCAND || rop == SRCPAINT)) {
    for (y = 0; y < dh; ++y) {
      int source_y = sy + y * sh / dh;
      for (x = 0; x < dw; ++x) {
        unsigned char *source = pixel_at(src->bitmap, sx + x * sw / dw, source_y);
        unsigned char *target = pixel_at(dst->bitmap, dx + x, dy + y);
        if (source && target)
          apply_rop(target, *source, rop);
      }
    }
    return TRUE;
  }
  for (y = 0; y < dh; ++y)
    for (x = 0; x < dw; ++x) {
      COLORREF color = read_pixel_color(src, sx + x * sw / dw, sy + y * sh / dh);
      if (color != UINT32_MAX)
        write_pixel_color(dst, dx + x, dy + y, color, rop);
    }
  return TRUE;
}

COLORREF GetPixel(HDC dc, int x, int y)
{
  return read_pixel_color(dc, x, y);
}

COLORREF SetPixel(HDC dc, int x, int y, COLORREF color)
{
  return SetPixelV(dc, x, y, color) ? color : (COLORREF)-1;
}

BOOL SetPixelV(HDC dc, int x, int y, COLORREF color)
{
  unsigned char *pixel = dc ? pixel_at(dc->bitmap, x, y) : NULL;
  if (!pixel)
    return FALSE;
  if (dc->bitmap->bits_per_pixel == 24) {
    pixel[0] = (unsigned char)((color >> 16) & 0xff);
    pixel[1] = (unsigned char)((color >> 8) & 0xff);
    pixel[2] = (unsigned char)(color & 0xff);
  } else {
    *pixel = color_to_index(dc, color);
  }
  return TRUE;
}

static PaintObject *create_paint(int kind, COLORREF color, int is_null)
{
  PaintObject *object = (PaintObject *)calloc(1, sizeof(*object));
  if (object) {
    object->header.kind = kind;
    object->color = color;
    object->is_null = is_null;
  }
  return object;
}

HBRUSH CreateSolidBrush(COLORREF color) { return create_paint(OBJECT_BRUSH, color, 0); }
HBRUSH CreateBrushIndirect(const LOGBRUSH *brush) { return CreateSolidBrush(brush ? brush->lbColor : 0); }
HPEN CreatePen(int style, int width, COLORREF color) { (void)style; (void)width; return create_paint(OBJECT_PEN, color, 0); }
HPEN CreatePenIndirect(const LOGPEN *pen) { return CreatePen(0, 1, pen ? pen->lopnColor : 0); }

int FillRect(HDC dc, const RECT *rect, HBRUSH brush)
{
  int x, y;
  int left;
  int top;
  int right;
  int bottom;
  PaintObject *paint = (PaintObject *)brush;
  if (!dc || !dc->bitmap || !rect)
    return 0;
  if (!paint || paint->is_null)
    return 1;
  left = map_x(dc, rect->left);
  top = map_y(dc, rect->top);
  right = map_x(dc, rect->right);
  bottom = map_y(dc, rect->bottom);
  {
    unsigned char index = color_to_index(dc, paint->color);
    for (y = top; y < bottom; ++y)
      for (x = left; x < right; ++x) {
        unsigned char *pixel = pixel_at(dc->bitmap, x, y);
        if (pixel) *pixel = index;
      }
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
  PaintObject *pen = dc ? (PaintObject *)dc->pen : NULL;
  COLORREF color = pen ? pen->color : PALETTEINDEX(255);
  if (!dc) return FALSE;
  if (pen && pen->is_null) {
    dc->cursor_x = x1;
    dc->cursor_y = y1;
    return TRUE;
  }
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
  RECT fill_rect;
  if (!dc)
    return FALSE;
  SetRect(&fill_rect, left, top, right, bottom);
  FillRect(dc, &fill_rect, dc->brush);
  left = map_x(dc, left);
  top = map_y(dc, top);
  right = map_x(dc, right);
  bottom = map_y(dc, bottom);
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

BOOL RoundRect(HDC dc, int left, int top, int right, int bottom, int ellipse_width, int ellipse_height)
{
  (void)ellipse_width;
  (void)ellipse_height;
  return Rectangle(dc, left, top, right, bottom);
}

static void copy_dc_to_state(DCState *state, const HDC dc)
{
  state->bitmap = dc->bitmap;
  state->palette = dc->palette;
  state->font = dc->font;
  state->text_color = dc->text_color;
  state->background_color = dc->background_color;
  state->background_mode = dc->background_mode;
  state->cursor_x = dc->cursor_x;
  state->cursor_y = dc->cursor_y;
  state->pen = dc->pen;
  state->brush = dc->brush;
  state->map_mode = dc->map_mode;
  state->window_ext_x = dc->window_ext_x;
  state->window_ext_y = dc->window_ext_y;
  state->viewport_ext_x = dc->viewport_ext_x;
  state->viewport_ext_y = dc->viewport_ext_y;
  state->window_org_x = dc->window_org_x;
  state->window_org_y = dc->window_org_y;
  state->viewport_org_x = dc->viewport_org_x;
  state->viewport_org_y = dc->viewport_org_y;
  state->has_clip = dc->has_clip;
  state->clip = dc->clip;
}

static void copy_state_to_dc(HDC dc, const DCState *state)
{
  dc->bitmap = state->bitmap;
  dc->palette = state->palette;
  dc->font = state->font;
  dc->text_color = state->text_color;
  dc->background_color = state->background_color;
  dc->background_mode = state->background_mode;
  dc->cursor_x = state->cursor_x;
  dc->cursor_y = state->cursor_y;
  dc->pen = state->pen;
  dc->brush = state->brush;
  dc->map_mode = state->map_mode;
  dc->window_ext_x = state->window_ext_x;
  dc->window_ext_y = state->window_ext_y;
  dc->viewport_ext_x = state->viewport_ext_x;
  dc->viewport_ext_y = state->viewport_ext_y;
  dc->window_org_x = state->window_org_x;
  dc->window_org_y = state->window_org_y;
  dc->viewport_org_x = state->viewport_org_x;
  dc->viewport_org_y = state->viewport_org_y;
  dc->has_clip = state->has_clip;
  dc->clip = state->clip;
}

int SaveDC(HDC dc)
{
  if (!dc || dc->saved_count >= (int)(sizeof(dc->saved) / sizeof(dc->saved[0])))
    return 0;
  copy_dc_to_state(&dc->saved[dc->saved_count], dc);
  ++dc->saved_count;
  return dc->saved_count;
}

BOOL RestoreDC(HDC dc, int saved)
{
  int index;
  if (!dc || !dc->saved_count || saved == 0)
    return FALSE;
  index = saved < 0 ? dc->saved_count + saved : saved - 1;
  if (index < 0 || index >= dc->saved_count)
    return FALSE;
  copy_state_to_dc(dc, &dc->saved[index]);
  dc->saved_count = index;
  return TRUE;
}

int IntersectClipRect(HDC dc, int left, int top, int right, int bottom)
{
  RECT incoming;
  if (!dc)
    return 0;
  incoming.left = map_x(dc, left);
  incoming.top = map_y(dc, top);
  incoming.right = map_x(dc, right);
  incoming.bottom = map_y(dc, bottom);
  if (!dc->has_clip) {
    dc->clip = incoming;
    dc->has_clip = 1;
  } else {
    dc->clip.left = max(dc->clip.left, incoming.left);
    dc->clip.top = max(dc->clip.top, incoming.top);
    dc->clip.right = min(dc->clip.right, incoming.right);
    dc->clip.bottom = min(dc->clip.bottom, incoming.bottom);
  }
  return 1;
}

void *CreateRectRgn(int left, int top, int right, int bottom)
{
  CompatRegion *region = (CompatRegion *)calloc(1, sizeof(*region));
  if (!region) return NULL;
  region->header.kind = OBJECT_REGION;
  SetRect(&region->rect, left, top, right, bottom);
  return region;
}

BOOL SetRectRgn(void *handle, int left, int top, int right, int bottom)
{
  CompatRegion *region = (CompatRegion *)handle;
  if (!region || region->header.kind != OBJECT_REGION) return FALSE;
  return SetRect(&region->rect, left, top, right, bottom);
}

int SelectClipRgn(HDC dc, void *handle)
{
  CompatRegion *region = (CompatRegion *)handle;
  if (!dc) return 0;
  if (!region) {
    dc->has_clip = 0;
    return 1;
  }
  if (region->header.kind != OBJECT_REGION) return 0;
  dc->clip = region->rect;
  dc->has_clip = 1;
  return 1;
}

BOOL SetRect(RECT *rect, int left, int top, int right, int bottom)
{
  if (!rect) return FALSE;
  rect->left = left;
  rect->top = top;
  rect->right = right;
  rect->bottom = bottom;
  return TRUE;
}

BOOL CopyRect(RECT *dst, const RECT *src)
{
  if (!dst || !src) return FALSE;
  *dst = *src;
  return TRUE;
}

BOOL OffsetRect(RECT *rect, int dx, int dy)
{
  if (!rect) return FALSE;
  rect->left += dx;
  rect->right += dx;
  rect->top += dy;
  rect->bottom += dy;
  return TRUE;
}

BOOL DrawFocusRect(HDC dc, const RECT *rect)
{
  if (!dc || !rect) return FALSE;
  return Rectangle(dc, rect->left, rect->top, rect->right, rect->bottom);
}

int SetMapMode(HDC dc, int mode) { int old = dc ? dc->map_mode : 0; if (dc) dc->map_mode = mode; return old; }
BOOL SetWindowExtEx(HDC dc, int x, int y, LPSIZE old) { if (!dc || !x || !y) return FALSE; if (old) { old->cx = dc->window_ext_x; old->cy = dc->window_ext_y; } dc->window_ext_x = x; dc->window_ext_y = y; return TRUE; }
BOOL SetViewportExtEx(HDC dc, int x, int y, LPSIZE old) { if (!dc) return FALSE; if (old) { old->cx = dc->viewport_ext_x; old->cy = dc->viewport_ext_y; } dc->viewport_ext_x = x; dc->viewport_ext_y = y; return TRUE; }
BOOL SetWindowOrgEx(HDC dc, int x, int y, LPPOINT old) { if (!dc) return FALSE; if (old) { old->x = dc->window_org_x; old->y = dc->window_org_y; } dc->window_org_x = x; dc->window_org_y = y; return TRUE; }
BOOL SetViewportOrgEx(HDC dc, int x, int y, LPPOINT old) { if (!dc) return FALSE; if (old) { old->x = dc->viewport_org_x; old->y = dc->viewport_org_y; } dc->viewport_org_x = x; dc->viewport_org_y = y; return TRUE; }
BOOL LPtoDP(HDC dc, LPPOINT points, int count) { int i; if (!dc || !points) return FALSE; for (i = 0; i < count; ++i) { points[i].x = map_x(dc, points[i].x); points[i].y = map_y(dc, points[i].y); } return TRUE; }
BOOL DPtoLP(HDC dc, LPPOINT points, int count)
{
  int i;
  if (!dc || !points)
    return FALSE;
  if (dc->map_mode == MM_TEXT) {
    for (i = 0; i < count; ++i) {
      points[i].x = dc->window_org_x + points[i].x - dc->viewport_org_x;
      points[i].y = dc->window_org_y + points[i].y - dc->viewport_org_y;
    }
    return TRUE;
  }
  if (!dc->viewport_ext_x || !dc->viewport_ext_y)
    return FALSE;
  for (i = 0; i < count; ++i) {
    points[i].x = dc->window_org_x +
                  (points[i].x - dc->viewport_org_x) * dc->window_ext_x /
                    dc->viewport_ext_x;
    points[i].y = dc->window_org_y +
                  (points[i].y - dc->viewport_org_y) * dc->window_ext_y /
                    dc->viewport_ext_y;
  }
  return TRUE;
}
UINT SetTextAlign(HDC dc, UINT align) { (void)dc; return align; }

HGDIOBJ GetStockObject(int object)
{
  COLORREF color = object == WHITE_BRUSH ? RGB(255,255,255) :
                   object == LTGRAY_BRUSH ? RGB(192,192,192) :
                   object == GRAY_BRUSH ? RGB(128,128,128) :
                   object == DKGRAY_BRUSH ? RGB(64,64,64) : RGB(0,0,0);
  if (object == DEFAULT_GUI_FONT || object == SYSTEM_FONT)
    return NULL;
  if (object == NULL_BRUSH)
    return create_paint(OBJECT_BRUSH, color, 1);
  if (object == BLACK_PEN)
    return CreatePen(0, 1, color);
  if (object == NULL_PEN)
    return create_paint(OBJECT_PEN, color, 1);
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
BOOL UnrealizeObject(HGDIOBJ object) { return object != NULL; }

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
  if (!dc || !colors) return 0;
  if (!dc->palette) {
    LOGPALETTE *log_palette = (LOGPALETTE *)calloc(1, sizeof(LOGPALETTE) + 255 * sizeof(PALETTEENTRY));
    if (!log_palette)
      return 0;
    log_palette->palVersion = 0x300;
    log_palette->palNumEntries = 256;
    dc->palette = CreatePalette(log_palette);
    free(log_palette);
    if (!dc->palette)
      return 0;
  }
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
  float old_size;
  int width;
  int height;
  char *utf8;
  int utf8_length;
  if (!size) return FALSE;
  if (length < 0) length = (int)strlen(text);
  if (dc && dc->font && dc->font->font) {
    utf8 = utf8_from_windows_text(text, length, &utf8_length);
    if (!utf8)
      return FALSE;
    old_size = prepare_font_for_dc(dc);
    if (!TTF_GetStringSize(dc->font->font, utf8, (size_t)utf8_length, &width, &height)) {
      free(utf8);
      restore_font_size(dc, old_size);
      return FALSE;
    }
    free(utf8);
    size->cx = logical_from_device(width, device_scale_x(dc));
    size->cy = logical_from_device(height, device_scale_y(dc));
    restore_font_size(dc, old_size);
    return TRUE;
  }
  size->cx = length * 8; size->cy = 12;
  return TRUE;
}

BOOL GetTextMetricsA(HDC dc, TEXTMETRICA *metrics)
{
  float old_size;
  double scale;
  if (!metrics) return FALSE;
  memset(metrics, 0, sizeof(*metrics));
  if (dc && dc->font && dc->font->font) {
    old_size = prepare_font_for_dc(dc);
    scale = device_scale_y(dc);
    metrics->tmHeight = logical_from_device(TTF_GetFontHeight(dc->font->font), scale);
    metrics->tmAscent = logical_from_device(TTF_GetFontAscent(dc->font->font), scale);
    metrics->tmDescent = logical_from_device(-TTF_GetFontDescent(dc->font->font), scale);
    metrics->tmInternalLeading = metrics->tmHeight - metrics->tmAscent - metrics->tmDescent;
    restore_font_size(dc, old_size);
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

static void blend_text_pixel(HDC dc, int x, int y, unsigned char alpha)
{
  COLORREF foreground;
  COLORREF background;
  int fr, fg, fb;
  int br, bg, bb;
  if (!alpha)
    return;
  foreground = resolve_color(dc, dc->text_color);
  background = read_pixel_color(dc, x, y);
  if (background == UINT32_MAX)
    return;
  fr = foreground & 0xff;
  fg = (foreground >> 8) & 0xff;
  fb = (foreground >> 16) & 0xff;
  br = background & 0xff;
  bg = (background >> 8) & 0xff;
  bb = (background >> 16) & 0xff;
  write_pixel_color(dc, x, y,
                    RGB((fr * alpha + br * (255 - alpha)) / 255,
                        (fg * alpha + bg * (255 - alpha)) / 255,
                        (fb * alpha + bb * (255 - alpha)) / 255),
                    SRCCOPY);
}

static void draw_text_surface(HDC dc, SDL_Surface *surface, int draw_x, int draw_y,
                              int clip_right, int clip_bottom)
{
  int x;
  int y;
  if (!surface)
    return;
  for (y = 0; y < surface->h && draw_y + y < clip_bottom; ++y) {
    for (x = 0; x < surface->w && draw_x + x < clip_right; ++x) {
      Uint8 r;
      Uint8 g;
      Uint8 b;
      Uint8 a;
      if (SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a))
        blend_text_pixel(dc, draw_x + x, draw_y + y, a);
    }
  }
}

BOOL TextOutA(HDC dc, int x, int y, LPCSTR text, int length)
{
  SDL_Surface *surface;
  SDL_Color white = {255,255,255,255};
  float old_size;
  char *utf8;
  int utf8_length;
  if (!dc || !dc->bitmap || !dc->font || !dc->font->font || !text) return FALSE;
  x = map_x(dc, x);
  y = map_y(dc, y);
  if (length < 0) length = (int)strlen(text);
  utf8 = utf8_from_windows_text(text, length, &utf8_length);
  if (!utf8)
    return FALSE;
  old_size = prepare_font_for_dc(dc);
  surface = TTF_RenderText_Blended(dc->font->font, utf8, (size_t)utf8_length, white);
  if (!surface) {
    SDL_Log("gdi: TTF_RenderText_Blended failed for \"%.*s\": %s",
            length, text, SDL_GetError());
    free(utf8);
    restore_font_size(dc, old_size);
    return FALSE;
  }
  free(utf8);
  draw_text_surface(dc, surface, x, y, dc->bitmap->width, dc->bitmap->height);
  SDL_DestroySurface(surface);
  restore_font_size(dc, old_size);
  return TRUE;
}

int DrawTextA(HDC dc, LPCSTR text, int length, RECT *rect, UINT format)
{
  SDL_Surface *surface;
  SDL_Color white = {255,255,255,255};
  int max_width;
  int draw_x;
  int draw_y;
  int rect_right;
  int rect_bottom;
  int single_line;
  float old_size;
  double scale_x;
  double scale_y;
  char *utf8;
  int utf8_length;
  if (!dc || !dc->bitmap || !dc->font || !dc->font->font || !text || !rect)
    return 0;
  if (length < 0)
    length = (int)strlen(text);
  draw_x = map_x(dc, rect->left);
  draw_y = map_y(dc, rect->top);
  rect_right = map_x(dc, rect->right);
  rect_bottom = map_y(dc, rect->bottom);
  max_width = rect_right - draw_x;
  single_line = (format & DT_SINGLELINE) != 0;
  if (max_width <= 0 || (!(format & DT_CALCRECT) && rect_bottom <= draw_y))
    return 0;
  utf8 = utf8_from_windows_text(text, length, &utf8_length);
  if (!utf8)
    return 0;
  old_size = prepare_font_for_dc(dc);
  surface = single_line
              ? TTF_RenderText_Blended(dc->font->font, utf8, (size_t)utf8_length, white)
              : TTF_RenderText_Blended_Wrapped(dc->font->font, utf8, (size_t)utf8_length,
                                               white, max_width);
  if (!surface) {
    free(utf8);
    restore_font_size(dc, old_size);
    return 0;
  }
  free(utf8);
  if (format & DT_CALCRECT) {
    scale_x = device_scale_x(dc);
    scale_y = device_scale_y(dc);
    rect->right = rect->left + logical_from_device(surface->w, scale_x);
    rect->bottom = rect->top + logical_from_device(surface->h, scale_y);
    SDL_DestroySurface(surface);
    restore_font_size(dc, old_size);
    return rect->bottom - rect->top;
  }
  if (format & DT_RIGHT)
    draw_x = rect_right - surface->w;
  else if (format & DT_CENTER)
    draw_x += (max_width - surface->w) / 2;
  if (single_line && (format & DT_VCENTER))
    draw_y += (rect_bottom - draw_y - surface->h) / 2;
  else if (single_line && (format & DT_BOTTOM))
    draw_y = rect_bottom - surface->h;
  draw_text_surface(dc, surface, draw_x, draw_y, rect_right, rect_bottom);
  max_width = surface->h;
  SDL_DestroySurface(surface);
  restore_font_size(dc, old_size);
  return logical_from_device(max_width, device_scale_y(dc));
}

int AddFontResourceA(LPCSTR path)
{
  return path && path[0];
}

BOOL RemoveFontResourceA(LPCSTR path)
{
  (void)path;
  return TRUE;
}

static const char *font_filename_for_face(const char *face, int bold, int italic)
{
  if (!face)
    return "TT0298M_.TTF";
  if (SDL_strcasecmp(face, "Benguiat BkCn Bt") == 0)
    return "TT0530M_.TTF";
  if (SDL_strcasecmp(face, "Benguiat Bk BT") == 0)
    return "TT0127M_.TTF";
  if (SDL_strcasecmp(face, "CentSchbook BT") == 0)
    return "TT0085M_.TTF";
  if (SDL_strcasecmp(face, "MPZurich Cn BT") == 0 ||
      SDL_strcasecmp(face, "Zurich Cn BT") == 0) {
    if (bold)
      return "TT0300M_.TTF";
    if (italic)
      return "TT0299M_.TTF";
    return "TT0298M_.TTF";
  }
  return "TT0298M_.TTF";
}

static HFONT open_font(const LOGFONTA *description)
{
  HFONT result = (HFONT)calloc(1, sizeof(*result));
  char path[1024];
  int style = TTF_STYLE_NORMAL;
  int size = description ? abs(description->lfHeight) : 12;
  const char *filename = font_filename_for_face(description ? description->lfFaceName : NULL,
                                                description && description->lfWeight >= FW_BOLD,
                                                description && description->lfItalic);
  if (!result) return NULL;
  if (!TTF_Init()) {
    free(result);
    return NULL;
  }
  result->header.kind = OBJECT_FONT;
  if (description)
    result->description = *description;
  result->size = size > 0 ? size : 12;
  result->description.lfHeight = result->description.lfHeight < 0
                                   ? -result->size
                                   : result->size;
  SDL_snprintf(path, sizeof(path), "%s/%s", g_font_root, filename);
  result->font = TTF_OpenFont(path, (float)result->size);
  if (result->font) {
    if (description && description->lfWeight >= FW_BOLD &&
        SDL_strcasecmp(filename, "TT0300M_.TTF") != 0)
      style |= TTF_STYLE_BOLD;
    if (description && description->lfItalic &&
        SDL_strcasecmp(filename, "TT0299M_.TTF") != 0)
      style |= TTF_STYLE_ITALIC;
    TTF_SetFontStyle(result->font, style);
    {
      int cell_height = TTF_GetFontHeight(result->font);
      result->font_size_to_cell_height = cell_height > 0 ? (double)result->size / (double)cell_height : 1.0;
    }
  }
  if (!result->font) {
    SDL_Log("gdi: TTF_OpenFont failed: %s: %s", path, SDL_GetError());
    free(result);
    return NULL;
  }
  return result;
}

HFONT CreateFontA(int height, int width, int escapement, int orientation, int weight,
                  DWORD italic, DWORD underline, DWORD strikeout, DWORD charset,
                  DWORD out_precision, DWORD clip_precision, DWORD quality,
                  DWORD pitch_family, LPCSTR face)
{
  LOGFONTA description;
  (void)width; (void)escapement; (void)orientation; (void)charset;
  (void)out_precision; (void)clip_precision; (void)quality; (void)pitch_family;
  memset(&description, 0, sizeof(description));
  description.lfHeight = height;
  description.lfWeight = weight;
  description.lfItalic = (BYTE)italic;
  description.lfUnderline = (BYTE)underline;
  description.lfStrikeOut = (BYTE)strikeout;
  SDL_strlcpy(description.lfFaceName, face ? face : "", sizeof(description.lfFaceName));
  return open_font(&description);
}

HFONT CreateFontIndirectA(const LOGFONTA *font) { return open_font(font); }

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

UINT GetModuleFileNameA(HMODULE module, LPSTR buffer, UINT size)
{
  (void)module;
  if (!buffer || !size)
    return 0;
  SDL_strlcpy(buffer, g_module_path, size);
  return (UINT)strlen(buffer);
}

void shandalar_win32_set_module_path(LPCSTR path)
{
  size_t i;
  SDL_strlcpy(g_module_path, path ? path : ".\\shandalar.exe", sizeof(g_module_path));
  for (i = 0; g_module_path[i]; ++i)
    if (g_module_path[i] == '/') g_module_path[i] = '\\';
}

int GetPrivateProfileIntA(LPCSTR section, LPCSTR key, int default_value, LPCSTR filename)
{
  char value[64];
  char *end;
  long parsed;
  if (!GetPrivateProfileStringA(section, key, "", value, sizeof(value), filename) || !value[0])
    return default_value;
  parsed = strtol(value, &end, 10);
  return end == value ? default_value : (int)parsed;
}

DWORD GetPrivateProfileStringA(LPCSTR section, LPCSTR key, LPCSTR default_value,
                               LPSTR returned_string, DWORD size, LPCSTR filename)
{
  FILE *file;
  char path[1024];
  char line[512];
  int in_section = 0;
  if (!returned_string || !size)
    return 0;
  SDL_strlcpy(path, filename ? filename : "", sizeof(path));
  for (char *cursor = path; *cursor; ++cursor)
    if (*cursor == '\\')
      *cursor = '/';
  file = fopen(path, "rb");
  if (file) {
    while (fgets(line, sizeof(line), file)) {
      char *cursor = line;
      char *equals;
      char *value;
      char *end;
      while (*cursor == ' ' || *cursor == '\t')
        ++cursor;
      if (*cursor == '[') {
        char *close = strchr(cursor + 1, ']');
        if (close) {
          *close = '\0';
          in_section = SDL_strcasecmp(cursor + 1, section ? section : "") == 0;
        }
        continue;
      }
      if (!in_section || *cursor == ';' || *cursor == '#' || !*cursor)
        continue;
      equals = strchr(cursor, '=');
      if (!equals)
        continue;
      *equals = '\0';
      end = equals - 1;
      while (end >= cursor && (*end == ' ' || *end == '\t'))
        *end-- = '\0';
      if (SDL_strcasecmp(cursor, key ? key : "") != 0)
        continue;
      value = equals + 1;
      while (*value == ' ' || *value == '\t')
        ++value;
      end = value + strlen(value);
      while (end > value && (end[-1] == '\r' || end[-1] == '\n' ||
                             end[-1] == ' ' || end[-1] == '\t'))
        *--end = '\0';
      if (end - value >= 2 && value[0] == '"' && end[-1] == '"') {
        ++value;
        end[-1] = '\0';
      }
      SDL_strlcpy(returned_string, value, size);
      fclose(file);
      return (DWORD)strlen(returned_string);
    }
    fclose(file);
  }
  SDL_strlcpy(returned_string, default_value ? default_value : "", size);
  return (DWORD)strlen(returned_string);
}

BOOL GetVersionExA(OSVERSIONINFOA *version_info)
{
  if (!version_info)
    return FALSE;
  version_info->dwMajorVersion = 10;
  version_info->dwMinorVersion = 0;
  version_info->dwBuildNumber = 0;
  version_info->dwPlatformId = 2;
  return TRUE;
}

void OutputDebugStringA(LPCSTR message)
{
  if (message)
    SDL_Log("%s", message);
}

int wsprintfA(LPSTR buffer, LPCSTR format, ...)
{
  int result;
  va_list args;
  va_start(args, format);
  result = vsprintf(buffer, format, args);
  va_end(args);
  return result;
}

int lstrlenA(LPCSTR text) { return text ? (int)strlen(text) : 0; }

BOOL CloseHandle(HANDLE handle) { (void)handle; return TRUE; }
void *GlobalAlloc(UINT flags, size_t size) { (void)flags; return malloc(size); }
void *GlobalLock(void *memory) { return memory; }
BOOL GlobalUnlock(void *memory) { (void)memory; return TRUE; }
void *GlobalHandle(void *memory) { return memory; }
void *GlobalFree(void *memory) { free(memory); return NULL; }
BOOL GdiFlush(void) { return TRUE; }
DWORD GdiGetBatchLimit(void) { return 0; }
DWORD GdiSetBatchLimit(DWORD limit) { return limit; }
int GetDeviceCaps(HDC dc, int index)
{
  if (index == RASTERCAPS)
    return RC_PALETTE;
  if (index == BITSPIXEL)
    return 8;
  if (index == PLANES)
    return 1;
  if (!dc || !dc->bitmap)
    return 0;
  if (index == HORZRES)
    return dc->bitmap->width;
  if (index == VERTRES)
    return dc->bitmap->height;
  return 0;
}

void InitializeCriticalSection(CRITICAL_SECTION *section) { if (section) section->unused = 0; }
void DeleteCriticalSection(CRITICAL_SECTION *section) { (void)section; }
void EnterCriticalSection(CRITICAL_SECTION *section) { (void)section; }
void LeaveCriticalSection(CRITICAL_SECTION *section) { (void)section; }

int ShowCursor(BOOL show)
{
  static int cursor_display_count;
  cursor_display_count += show ? 1 : -1;
  return cursor_display_count;
}

HDC shandalar_create_screen_dc(int width, int height)
{
  BITMAPINFO info;
  HDC dc = CreateCompatibleDC(NULL);
  HBITMAP bitmap;
  void *bits;
  memset(&info, 0, sizeof(info));
  info.bmiHeader.biWidth = width;
  info.bmiHeader.biHeight = -height;
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
  char *cursor;
  SDL_strlcpy(g_font_root, root ? root : "", sizeof(g_font_root));
  for (cursor = g_font_root; *cursor; ++cursor) {
    if (*cursor == '\\')
      *cursor = '/';
  }
}

BOOL shandalar_dc_copy_rgba(HDC dc, uint32_t *pixels, int pitch_bytes)
{
  int x;
  int y;
  if (!dc || !dc->bitmap || !pixels || pitch_bytes < dc->bitmap->width * 4)
    return FALSE;
  for (y = 0; y < dc->bitmap->height; ++y) {
    uint32_t *dst = (uint32_t *)((unsigned char *)pixels + y * pitch_bytes);
    for (x = 0; x < dc->bitmap->width; ++x) {
      COLORREF color = read_pixel_color(dc, x, y);
      dst[x] = 0xff000000u | ((color & 0xffu) << 16) | (color & 0xff00u) |
               ((color >> 16) & 0xffu);
    }
  }
  return TRUE;
}

BOOL InflateRect(RECT *rect, int dx, int dy)
{
  if (!rect) return FALSE;
  rect->left -= dx;
  rect->right += dx;
  rect->top -= dy;
  rect->bottom += dy;
  return TRUE;
}

BOOL PtInRect(const RECT *rect, POINT point)
{
  return rect && point.x >= rect->left && point.x < rect->right &&
         point.y >= rect->top && point.y < rect->bottom;
}

double shandalar_set_font_device_scale(double scale)
{
  double old = g_font_device_scale;
  g_font_device_scale = scale > 0.0 ? scale : 1.0;
  return old;
}
