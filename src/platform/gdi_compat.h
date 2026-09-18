#ifndef SHANDALAR_GDI_COMPAT_H
#define SHANDALAR_GDI_COMPAT_H

#define BeginPaint ShandalarBeginPaint
#define BitBlt ShandalarBitBlt
#define ClientToScreen ShandalarClientToScreen
#define CheckMenuItem ShandalarCheckMenuItem
#define CloseHandle ShandalarCloseHandle
#define CreateBitmap ShandalarCreateBitmap
#define CreateCompatibleBitmap ShandalarCreateCompatibleBitmap
#define CreateCompatibleDC ShandalarCreateCompatibleDC
#define CreateDIBSection ShandalarCreateDIBSection
#define CreateFileA ShandalarCreateFileA
#define CreatePopupMenu ShandalarCreatePopupMenu
#define CreateRectRgn ShandalarCreateRectRgn
#define CreateSolidBrush ShandalarCreateSolidBrush
#define CreateWindowExA ShandalarCreateWindowExA
#define DeleteCriticalSection ShandalarDeleteCriticalSection
#define DeleteDC ShandalarDeleteDC
#define DeleteMenu ShandalarDeleteMenu
#define DeleteObject ShandalarDeleteObject
#define DestroyIcon ShandalarDestroyIcon
#define DestroyMenu ShandalarDestroyMenu
#define DestroyWindow ShandalarDestroyWindow
#define DeviceIoControl ShandalarDeviceIoControl
#define DuplicateHandle ShandalarDuplicateHandle
#define EnableMenuItem ShandalarEnableMenuItem
#define EnableWindow ShandalarEnableWindow
#define EndDialog ShandalarEndDialog
#define EndPaint ShandalarEndPaint
#define EnterCriticalSection ShandalarEnterCriticalSection
#define FillRect ShandalarFillRect
#define GetClientRect ShandalarGetClientRect
#define GetCurrentProcess ShandalarGetCurrentProcess
#define GetCurrentThread ShandalarGetCurrentThread
#define GetCursorPos ShandalarGetCursorPos
#define GetDeviceCaps ShandalarGetDeviceCaps
#define GetDesktopWindow ShandalarGetDesktopWindow
#define GetDlgItem ShandalarGetDlgItem
#define GetDoubleClickTime ShandalarGetDoubleClickTime
#define GetFocus ShandalarGetFocus
#define GetKeyState ShandalarGetKeyState
#define GetMessageA ShandalarGetMessageA
#define GetMessageTime ShandalarGetMessageTime
#define GetProcAddress ShandalarGetProcAddress
#define GetSystemMetrics ShandalarGetSystemMetrics
#define GetTextExtentPoint32A ShandalarGetTextExtentPoint32A
#define GetTickCount ShandalarGetTickCount
#define GetWindow ShandalarGetWindow
#define GetWindowRect ShandalarGetWindowRect
#define GetWindowLongPtrA ShandalarGetWindowLongPtrA
#define GetWindowThreadProcessId ShandalarGetWindowThreadProcessId
#define GlobalAlloc ShandalarGlobalAlloc
#define GlobalFree ShandalarGlobalFree
#define GlobalLock ShandalarGlobalLock
#define GlobalUnlock ShandalarGlobalUnlock
#define IsIconic ShandalarIsIconic
#define KillTimer ShandalarKillTimer
#define LeaveCriticalSection ShandalarLeaveCriticalSection
#define MessageBoxA ShandalarMessageBoxA
#define PtInRect ShandalarPtInRect
#define ReadFile ShandalarReadFile
#define RegCloseKey ShandalarRegCloseKey
#define ReleaseCapture ShandalarReleaseCapture
#define ReleaseDC ShandalarReleaseDC
#define SelectObject ShandalarSelectObject
#define ScreenToClient ShandalarScreenToClient
#define SetActiveWindow ShandalarSetActiveWindow
#define SetCapture ShandalarSetCapture
#define SetCursor ShandalarSetCursor
#define SetCursorPos ShandalarSetCursorPos
#define SetFocus ShandalarSetFocus
#define SetForegroundWindow ShandalarSetForegroundWindow
#define SetParent ShandalarSetParent
#define SetPixel ShandalarSetPixel
#define SetTimer ShandalarSetTimer
#define SetWindowPos ShandalarSetWindowPos
#define SetWindowLongPtrA ShandalarSetWindowLongPtrA
#define ShowWindow ShandalarShowWindow
#define Sleep ShandalarSleep
#define TrackPopupMenu ShandalarTrackPopupMenu
#define TranslateMessage ShandalarTranslateMessage
#define UnregisterClassA ShandalarUnregisterClassA
#define timeBeginPeriod ShandalarTimeBeginPeriod
#define timeEndPeriod ShandalarTimeEndPeriod

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_WIN32)
#define __stdcall
#define __cdecl
#define __int64 long long
#endif
#define WINAPI __stdcall
#define CALLBACK __stdcall
#define APIENTRY WINAPI
#define MAX_PATH 260

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef int16_t SHORT;
typedef uint32_t DWORD;
typedef int32_t LONG;
typedef intptr_t LONG_PTR;
typedef uint32_t UINT;
typedef int BOOL;
typedef uintptr_t WPARAM;
typedef uintptr_t ULONG_PTR;
typedef intptr_t LPARAM;
typedef intptr_t LRESULT;
typedef uint32_t COLORREF;
typedef char CHAR;
typedef const char *LPCSTR;
typedef const void *LPCVOID;
typedef char *LPSTR;
typedef char *PSTR;
typedef void *LPVOID;
typedef void *LPOVERLAPPED;
typedef uintptr_t FARPROC;
typedef void *LPSECURITY_ATTRIBUTES;
typedef void *HANDLE;
typedef void *HINSTANCE;
typedef void *HMODULE;
typedef struct ShandalarWindow *HWND;
typedef void *HMENU;
typedef void *HCURSOR;
typedef void *HICON;
typedef void *HACCEL;
typedef uint16_t ATOM;
typedef void *HBRUSH;
typedef void *HPEN;
typedef void *HGDIOBJ;
typedef struct ShandalarBitmap *HBITMAP;
typedef struct ShandalarDC *HDC;
typedef struct ShandalarPalette *HPALETTE;
typedef struct ShandalarFont *HFONT;
typedef void *HRGN;

typedef struct RECT { LONG left, top, right, bottom; } RECT;
typedef RECT *LPRECT;
typedef struct POINT { LONG x, y; } POINT;
typedef struct SIZE { LONG cx, cy; } SIZE;
typedef struct FILETIME { DWORD dwLowDateTime, dwHighDateTime; } FILETIME;
typedef SIZE *LPSIZE;
typedef POINT *LPPOINT;
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
typedef struct DIBSECTION {
  BITMAP dsBm;
  BITMAPINFOHEADER dsBmih;
  DWORD dsBitfields[3];
  HANDLE dshSection;
  DWORD dsOffset;
} DIBSECTION;
typedef struct OSVERSIONINFOA {
  DWORD dwOSVersionInfoSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformId;
  CHAR szCSDVersion[128];
} OSVERSIONINFOA;
typedef struct LOGBRUSH { UINT lbStyle; COLORREF lbColor; uintptr_t lbHatch; } LOGBRUSH;
typedef struct LOGPEN { UINT lopnStyle; POINT lopnWidth; COLORREF lopnColor; } LOGPEN;
typedef struct LOGFONTA {
  LONG lfHeight, lfWidth, lfEscapement, lfOrientation, lfWeight;
  BYTE lfItalic, lfUnderline, lfStrikeOut, lfCharSet;
  BYTE lfOutPrecision, lfClipPrecision, lfQuality, lfPitchAndFamily;
  CHAR lfFaceName[32];
} LOGFONTA;
typedef LOGFONTA LOGFONT;
typedef struct TEXTMETRICA {
  LONG tmHeight, tmAscent, tmDescent, tmInternalLeading, tmExternalLeading;
  LONG tmAveCharWidth, tmMaxCharWidth, tmWeight, tmOverhang;
} TEXTMETRICA;
typedef struct CRITICAL_SECTION { int unused; } CRITICAL_SECTION;
typedef struct DRAWITEMSTRUCT {
  UINT CtlType;
  UINT CtlID;
  UINT itemID;
  UINT itemAction;
  UINT itemState;
  HWND hwndItem;
  HDC hDC;
  RECT rcItem;
  uintptr_t itemData;
} DRAWITEMSTRUCT;
typedef struct DELETEITEMSTRUCT {
  UINT CtlType;
  UINT CtlID;
  UINT itemID;
  HWND hwndItem;
  uintptr_t itemData;
} DELETEITEMSTRUCT;
typedef struct MEASUREITEMSTRUCT {
  UINT CtlType;
  UINT CtlID;
  UINT itemID;
  UINT itemWidth;
  UINT itemHeight;
  uintptr_t itemData;
} MEASUREITEMSTRUCT;

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
#define MM_TEXT 1
#define MM_ISOTROPIC 7
#define MM_ANISOTROPIC 8
#define DEFAULT_GUI_FONT 17
#define SYSTEM_FONT 13
#define WHITE_BRUSH 0
#define LTGRAY_BRUSH 1
#define GRAY_BRUSH 2
#define DKGRAY_BRUSH 3
#define BLACK_BRUSH 4
#define NULL_BRUSH 5
#define HOLLOW_BRUSH NULL_BRUSH
#define WHITE_PEN 6
#define NULL_PEN 8
#define PS_SOLID 0
#define PS_INSIDEFRAME 6
#define HS_DIAGCROSS 5
#define R2_COPYPEN 13
#define TA_LEFT 0
#define TA_RIGHT 2
#define TA_CENTER 6
#define TA_TOP 0
#define TA_BOTTOM 8
#define TA_BASELINE 24
#define GDI_ERROR 0xffffffffL
#define BLACK_PEN 7
#define HORZRES 8
#define VERTRES 10
#define BITSPIXEL 12
#define PLANES 14
#define RASTERCAPS 38
#define RC_PALETTE 0x0100
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define FW_BOLD 700
#define FW_NORMAL 400
#define ANSI_CHARSET 0
#define OUT_DEFAULT_PRECIS 0
#define CLIP_DEFAULT_PRECIS 0
#define DEFAULT_QUALITY 0
#define DEFAULT_PITCH 0
#define FF_DONTCARE 0
#define DT_TOP 0x0000
#define DT_LEFT 0x0000
#define DT_CENTER 0x0001
#define DT_RIGHT 0x0002
#define DT_VCENTER 0x0004
#define DT_BOTTOM 0x0008
#define DT_WORDBREAK 0x0010
#define DT_SINGLELINE 0x0020
#define DT_CALCRECT 0x0400
#define DT_NOPREFIX 0x0800
#define WM_GETFONT 0x0031
#define ODS_SELECTED 0x0001
#define ODS_FOCUS 0x0010
#define ODT_LISTBOX 2
#define ODT_BUTTON 4

#define MAKEWORD(low, high) ((WORD)(((BYTE)(low)) | ((WORD)((BYTE)(high)) << 8)))
#define RGB(r,g,b) ((COLORREF)((BYTE)(r) | ((DWORD)(BYTE)(g) << 8) | ((DWORD)(BYTE)(b) << 16)))
#define PALETTEINDEX(i) ((COLORREF)(0x01000000u | ((DWORD)(i) & 0xffffu)))
#define PALETTERGB(r,g,b) (RGB((r),(g),(b)) | 0x02000000u)
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

HDC CreateCompatibleDC(HDC dc);
HDC WINAPI shandalar_GetDC(HWND window);
#define GetDC shandalar_GetDC
int ReleaseDC(HWND window, HDC dc);
BOOL DeleteDC(HDC dc);
HBITMAP CreateDIBSection(HDC dc, const BITMAPINFO *info, UINT usage, void **bits, HANDLE section, DWORD offset);
HBITMAP CreateBitmap(int width, int height, UINT planes, UINT bits_per_pixel, const void *bits);
HBITMAP CreateCompatibleBitmap(HDC dc, int width, int height);
HGDIOBJ SelectObject(HDC dc, HGDIOBJ object);
BOOL DeleteObject(HGDIOBJ object);
int WINAPI shandalar_GetObjectA(HGDIOBJ object, int size, void *out);
#define GetObjectA shandalar_GetObjectA
BOOL BitBlt(HDC dst, int dx, int dy, int width, int height, HDC src, int sx, int sy, DWORD rop);
BOOL StretchBlt(HDC dst, int dx, int dy, int dw, int dh, HDC src, int sx, int sy, int sw, int sh, DWORD rop);
COLORREF GetPixel(HDC dc, int x, int y);
COLORREF SetPixel(HDC dc, int x, int y, COLORREF color);
BOOL SetPixelV(HDC dc, int x, int y, COLORREF color);
int FillRect(HDC dc, const RECT *rect, HBRUSH brush);
int FrameRect(HDC dc, const RECT *rect, HBRUSH brush);
HBRUSH CreateSolidBrush(COLORREF color);
HBRUSH CreateBrushIndirect(const LOGBRUSH *brush);
HBRUSH CreateHatchBrush(int hatch, COLORREF color);
HPEN CreatePen(int style, int width, COLORREF color);
HPEN CreatePenIndirect(const LOGPEN *pen);
BOOL MoveToEx(HDC dc, int x, int y, POINT *old);
BOOL LineTo(HDC dc, int x, int y);
BOOL Rectangle(HDC dc, int left, int top, int right, int bottom);
BOOL Ellipse(HDC dc, int left, int top, int right, int bottom);
BOOL RoundRect(HDC dc, int left, int top, int right, int bottom, int ellipse_width, int ellipse_height);
HGDIOBJ GetStockObject(int object);
int SaveDC(HDC dc);
BOOL RestoreDC(HDC dc, int saved);
int IntersectClipRect(HDC dc, int left, int top, int right, int bottom);
void *CreateRectRgn(int left, int top, int right, int bottom);
HRGN CreateRectRgnIndirect(const RECT *rect);
BOOL SetRectRgn(void *region, int left, int top, int right, int bottom);
int SelectClipRgn(HDC dc, void *region);
BOOL SetRect(RECT *rect, int left, int top, int right, int bottom);
BOOL CopyRect(RECT *dst, const RECT *src);
BOOL OffsetRect(RECT *rect, int dx, int dy);
BOOL InflateRect(RECT *rect, int dx, int dy);
BOOL PtInRect(const RECT *rect, POINT point);
BOOL IsRectEmpty(const RECT *rect);
BOOL UnionRect(RECT *destination, const RECT *first, const RECT *second);
BOOL DrawFocusRect(HDC dc, const RECT *rect);
int SetMapMode(HDC dc, int mode);
BOOL SetWindowExtEx(HDC dc, int x, int y, LPSIZE old);
BOOL SetViewportExtEx(HDC dc, int x, int y, LPSIZE old);
BOOL SetWindowOrgEx(HDC dc, int x, int y, LPPOINT old);
BOOL SetViewportOrgEx(HDC dc, int x, int y, LPPOINT old);
BOOL OffsetViewportOrgEx(HDC dc, int x, int y, LPPOINT old);
BOOL LPtoDP(HDC dc, LPPOINT points, int count);
BOOL DPtoLP(HDC dc, LPPOINT points, int count);
UINT SetTextAlign(HDC dc, UINT align);

HPALETTE CreatePalette(const LOGPALETTE *palette);
HPALETTE SelectPalette(HDC dc, HPALETTE palette, BOOL background);
UINT RealizePalette(HDC dc);
BOOL UnrealizeObject(HGDIOBJ object);
UINT GetPaletteEntries(HPALETTE palette, UINT start, UINT count, PALETTEENTRY *entries);
UINT SetPaletteEntries(HPALETTE palette, UINT start, UINT count, const PALETTEENTRY *entries);
UINT SetDIBColorTable(HDC dc, UINT start, UINT count, const RGBQUAD *colors);
int SetStretchBltMode(HDC dc, int mode);

COLORREF SetTextColor(HDC dc, COLORREF color);
COLORREF GetTextColor(HDC dc);
COLORREF SetBkColor(HDC dc, COLORREF color);
int SetBkMode(HDC dc, int mode);
int SetROP2(HDC dc, int mode);
BOOL TextOutA(HDC dc, int x, int y, LPCSTR text, int length);
int DrawTextA(HDC dc, LPCSTR text, int length, RECT *rect, UINT format);
BOOL GetTextExtentPoint32A(HDC dc, LPCSTR text, int length, SIZE *size);
BOOL GetCharWidthA(HDC dc, UINT first, UINT last, int *widths);
#define TextOut TextOutA
#define GetTextExtentPoint32 GetTextExtentPoint32A
BOOL GetTextMetricsA(HDC dc, TEXTMETRICA *metrics);
BOOL GetCharABCWidthsA(HDC dc, UINT first, UINT last, ABC *widths);
int AddFontResourceA(LPCSTR path);
BOOL RemoveFontResourceA(LPCSTR path);
HFONT CreateFontA(int height, int width, int escapement, int orientation, int weight,
                  DWORD italic, DWORD underline, DWORD strikeout, DWORD charset,
                  DWORD out_precision, DWORD clip_precision, DWORD quality,
                  DWORD pitch_family, LPCSTR face);
HFONT CreateFontIndirectA(const LOGFONTA *font);
#define CreateFontIndirect CreateFontIndirectA
#define GetObject GetObjectA
#define GetTextExtentPointA GetTextExtentPoint32A

int SetDIBitsToDevice(HDC dc, int x, int y, DWORD width, DWORD height,
                      int src_x, int src_y, UINT start_scan, UINT scan_lines,
                      const void *bits, const BITMAPINFO *info, UINT usage);

int MessageBoxA(HWND window, LPCSTR message, LPCSTR title, UINT flags);
#define MessageBox MessageBoxA
HWND GetDesktopWindow(void);
UINT GetModuleFileNameA(HMODULE module, LPSTR buffer, UINT size);
void shandalar_win32_set_module_path(LPCSTR path);
int GetPrivateProfileIntA(LPCSTR section, LPCSTR key, int default_value, LPCSTR filename);
DWORD GetPrivateProfileStringA(LPCSTR section, LPCSTR key, LPCSTR default_value,
                               LPSTR returned_string, DWORD size, LPCSTR filename);
BOOL WritePrivateProfileStringA(LPCSTR section, LPCSTR key, LPCSTR value, LPCSTR filename);
BOOL GetVersionExA(OSVERSIONINFOA *version_info);
void OutputDebugStringA(LPCSTR message);
#define OutputDebugString OutputDebugStringA
int wsprintfA(LPSTR buffer, LPCSTR format, ...);
int lstrlenA(LPCSTR text);
#define wsprintf wsprintfA
#define WritePrivateProfileString WritePrivateProfileStringA
int GetWindowTextA(HWND window, LPSTR text, int max_count);
#define GetWindowText GetWindowTextA
LRESULT SendMessageA(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
int ShowCursor(BOOL show);
BOOL CloseHandle(HANDLE handle);
void *GlobalAlloc(UINT flags, size_t size);
void *GlobalLock(void *memory);
BOOL GlobalUnlock(void *memory);
void *GlobalHandle(void *memory);
void *GlobalFree(void *memory);
BOOL GdiFlush(void);
DWORD GdiGetBatchLimit(void);
DWORD GdiSetBatchLimit(DWORD limit);
void checked_DeleteDC_DeleteObject(HDC dc, HGDIOBJ obj);
int GetDeviceCaps(HDC dc, int index);
HDC CreateDCA(LPCSTR driver, LPCSTR device, LPCSTR output, const void *init_data);
void InitializeCriticalSection(CRITICAL_SECTION *section);
void DeleteCriticalSection(CRITICAL_SECTION *section);
void EnterCriticalSection(CRITICAL_SECTION *section);
void LeaveCriticalSection(CRITICAL_SECTION *section);

void shandalar_set_screen_dc(HDC dc);
HDC shandalar_create_screen_dc(int width, int height);
void shandalar_destroy_screen_dc(HDC dc);
void shandalar_dc_set_device_origin(HDC dc, int x, int y);
const void *shandalar_dc_pixels(HDC dc, int *pitch);
BOOL shandalar_dc_copy_rgba(HDC dc, uint32_t *pixels, int pitch_bytes);
COLORREF shandalar_dc_resolve_color(HDC dc, COLORREF color);
BOOL shandalar_dc_blit_argb8888(HDC dc, const uint32_t *pixels,
                                int pitch_bytes, int width, int height);
void shandalar_set_default_font_root(const char *root);
const char *shandalar_default_font_root(void);
double shandalar_set_font_device_scale(double scale);

#ifdef __cplusplus
}
#endif
#endif
