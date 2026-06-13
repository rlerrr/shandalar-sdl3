#ifndef SHANDALAR_GDI_COMPAT_H
#define SHANDALAR_GDI_COMPAT_H

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WINAPI
#define CALLBACK
#define APIENTRY
#define __stdcall
#define __cdecl
#define MAX_PATH 260

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef uint32_t UINT;
typedef int BOOL;
typedef uintptr_t WPARAM;
typedef intptr_t LPARAM;
typedef intptr_t LRESULT;
typedef uint32_t COLORREF;
typedef char CHAR;
typedef const char *LPCSTR;
typedef char *LPSTR;
typedef void *LPVOID;
typedef void *LPSECURITY_ATTRIBUTES;
typedef void *HANDLE;
typedef void *HINSTANCE;
typedef void *HMODULE;
typedef void *HWND;
typedef void *HMENU;
typedef void *HCURSOR;
typedef void *HICON;
typedef uint16_t ATOM;
typedef void *HBRUSH;
typedef void *HPEN;
typedef void *HGDIOBJ;
typedef struct ShandalarBitmap *HBITMAP;
typedef struct ShandalarDC *HDC;
typedef struct ShandalarPalette *HPALETTE;
typedef struct ShandalarFont *HFONT;

typedef struct RECT { LONG left, top, right, bottom; } RECT;
typedef struct POINT { LONG x, y; } POINT;
typedef struct SIZE { LONG cx, cy; } SIZE;
typedef struct ABC { int abcA; UINT abcB; int abcC; } ABC;
typedef struct RGBQUAD { BYTE rgbBlue, rgbGreen, rgbRed, rgbReserved; } RGBQUAD;
typedef struct PALETTEENTRY { BYTE peRed, peGreen, peBlue, peFlags; } PALETTEENTRY;
typedef struct LOGPALETTE { WORD palVersion, palNumEntries; PALETTEENTRY palPalEntry[1]; } LOGPALETTE;
typedef struct BITMAPINFOHEADER {
  DWORD biSize;
  LONG biWidth;
  LONG biHeight;
  WORD biPlanes;
  WORD biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG biXPelsPerMeter;
  LONG biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER;
typedef struct BITMAPINFO { BITMAPINFOHEADER bmiHeader; RGBQUAD bmiColors[256]; } BITMAPINFO;
typedef struct BITMAP {
  LONG bmType;
  LONG bmWidth;
  LONG bmHeight;
  LONG bmWidthBytes;
  WORD bmPlanes;
  WORD bmBitsPixel;
  LPVOID bmBits;
} BITMAP;
typedef struct LOGBRUSH { UINT lbStyle; COLORREF lbColor; uintptr_t lbHatch; } LOGBRUSH;
typedef struct LOGPEN { UINT lopnStyle; POINT lopnWidth; COLORREF lopnColor; } LOGPEN;
typedef struct LOGFONTA {
  LONG lfHeight, lfWidth, lfEscapement, lfOrientation, lfWeight;
  BYTE lfItalic, lfUnderline, lfStrikeOut, lfCharSet;
  BYTE lfOutPrecision, lfClipPrecision, lfQuality, lfPitchAndFamily;
  CHAR lfFaceName[32];
} LOGFONTA;
typedef struct TEXTMETRICA {
  LONG tmHeight, tmAscent, tmDescent, tmInternalLeading, tmExternalLeading;
  LONG tmAveCharWidth, tmMaxCharWidth, tmWeight, tmOverhang;
} TEXTMETRICA;
typedef struct CRITICAL_SECTION { int unused; } CRITICAL_SECTION;

#define TRUE 1
#define FALSE 0
#define BI_RGB 0
#define DIB_RGB_COLORS 0
#define DIB_PAL_COLORS 1
#define SRCCOPY 0x00cc0020
#define SRCPAINT 0x00ee0086
#define SRCAND 0x008800c6
#define COLORONCOLOR 3
#define TRANSPARENT 1
#define OPAQUE 2
#define DEFAULT_GUI_FONT 17
#define SYSTEM_FONT 13
#define WHITE_BRUSH 0
#define BLACK_BRUSH 4
#define NULL_BRUSH 5
#define NULL_PEN 8
#define HORZRES 8
#define VERTRES 10
#define BITSPIXEL 12
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)

#define RGB(r,g,b) ((COLORREF)((BYTE)(r) | ((DWORD)(BYTE)(g) << 8) | ((DWORD)(BYTE)(b) << 16)))
#define PALETTEINDEX(i) ((COLORREF)(0x01000000u | ((DWORD)(i) & 0xffffu)))
#define PALETTERGB(r,g,b) (RGB((r),(g),(b)) | 0x02000000u)

HDC CreateCompatibleDC(HDC dc);
HDC GetDC(HWND window);
int ReleaseDC(HWND window, HDC dc);
BOOL DeleteDC(HDC dc);
HBITMAP CreateDIBSection(HDC dc, const BITMAPINFO *info, UINT usage, void **bits, HANDLE section, DWORD offset);
HBITMAP CreateBitmap(int width, int height, UINT planes, UINT bits_per_pixel, const void *bits);
HGDIOBJ SelectObject(HDC dc, HGDIOBJ object);
BOOL DeleteObject(HGDIOBJ object);
int GetObjectA(HGDIOBJ object, int size, void *out);
BOOL BitBlt(HDC dst, int dx, int dy, int width, int height, HDC src, int sx, int sy, DWORD rop);
BOOL StretchBlt(HDC dst, int dx, int dy, int dw, int dh, HDC src, int sx, int sy, int sw, int sh, DWORD rop);
COLORREF GetPixel(HDC dc, int x, int y);
BOOL SetPixelV(HDC dc, int x, int y, COLORREF color);
int FillRect(HDC dc, const RECT *rect, HBRUSH brush);
HBRUSH CreateSolidBrush(COLORREF color);
HBRUSH CreateBrushIndirect(const LOGBRUSH *brush);
HPEN CreatePen(int style, int width, COLORREF color);
HPEN CreatePenIndirect(const LOGPEN *pen);
BOOL MoveToEx(HDC dc, int x, int y, POINT *old);
BOOL LineTo(HDC dc, int x, int y);
BOOL Rectangle(HDC dc, int left, int top, int right, int bottom);
BOOL Ellipse(HDC dc, int left, int top, int right, int bottom);
HGDIOBJ GetStockObject(int object);

HPALETTE CreatePalette(const LOGPALETTE *palette);
HPALETTE SelectPalette(HDC dc, HPALETTE palette, BOOL background);
UINT RealizePalette(HDC dc);
UINT GetPaletteEntries(HPALETTE palette, UINT start, UINT count, PALETTEENTRY *entries);
UINT SetPaletteEntries(HPALETTE palette, UINT start, UINT count, const PALETTEENTRY *entries);
UINT SetDIBColorTable(HDC dc, UINT start, UINT count, const RGBQUAD *colors);
int SetStretchBltMode(HDC dc, int mode);

COLORREF SetTextColor(HDC dc, COLORREF color);
COLORREF SetBkColor(HDC dc, COLORREF color);
int SetBkMode(HDC dc, int mode);
BOOL TextOutA(HDC dc, int x, int y, LPCSTR text, int length);
int DrawTextA(HDC dc, LPCSTR text, int length, RECT *rect, UINT format);
BOOL GetTextExtentPoint32A(HDC dc, LPCSTR text, int length, SIZE *size);
BOOL GetTextMetricsA(HDC dc, TEXTMETRICA *metrics);
BOOL GetCharABCWidthsA(HDC dc, UINT first, UINT last, ABC *widths);
int AddFontResourceA(LPCSTR path);
HFONT CreateFontA(int height, int width, int escapement, int orientation, int weight,
                  DWORD italic, DWORD underline, DWORD strikeout, DWORD charset,
                  DWORD out_precision, DWORD clip_precision, DWORD quality,
                  DWORD pitch_family, LPCSTR face);
HFONT CreateFontIndirectA(const LOGFONTA *font);

int SetDIBitsToDevice(HDC dc, int x, int y, DWORD width, DWORD height,
                      int src_x, int src_y, UINT start_scan, UINT scan_lines,
                      const void *bits, const BITMAPINFO *info, UINT usage);

int MessageBoxA(HWND window, LPCSTR message, LPCSTR title, UINT flags);
BOOL CloseHandle(HANDLE handle);
void *GlobalAlloc(UINT flags, size_t size);
void *GlobalLock(void *memory);
BOOL GlobalUnlock(void *memory);
void *GlobalHandle(void *memory);
void *GlobalFree(void *memory);
BOOL GdiFlush(void);
int GetDeviceCaps(HDC dc, int index);

void shandalar_set_screen_dc(HDC dc);
HDC shandalar_create_screen_dc(int width, int height);
void shandalar_destroy_screen_dc(HDC dc);
const void *shandalar_dc_pixels(HDC dc, int *pitch);
void shandalar_set_default_font_root(const char *root);

#ifdef __cplusplus
}
#endif
#endif
