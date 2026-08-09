#ifndef SHANDALAR_WINUSER_COMPAT_H
#define SHANDALAR_WINUSER_COMPAT_H

#include "platform/gdi_compat.h"

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef LRESULT(CALLBACK *WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef BOOL(CALLBACK *WNDENUMPROC)(HWND, LPARAM);
typedef DWORD(CALLBACK *LPTHREAD_START_ROUTINE)(LPVOID);
typedef intptr_t INT_PTR;
typedef uintptr_t UINT_PTR;
typedef INT_PTR(CALLBACK *DLGPROC)(HWND, UINT, WPARAM, LPARAM);
typedef void(CALLBACK *TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);
typedef void(CALLBACK *LPTIMECALLBACK)(UINT, UINT, DWORD, DWORD, DWORD);

typedef struct tagWNDCLASSA {
  UINT style;
  WNDPROC lpfnWndProc;
  int cbClsExtra;
  int cbWndExtra;
  HINSTANCE hInstance;
  HICON hIcon;
  HCURSOR hCursor;
  HBRUSH hbrBackground;
  LPCSTR lpszMenuName;
  LPCSTR lpszClassName;
} WNDCLASSA, WNDCLASS;

typedef struct tagCREATESTRUCTA {
  LPVOID lpCreateParams;
  HINSTANCE hInstance;
  HMENU hMenu;
  HWND hwndParent;
  int cy;
  int cx;
  int y;
  int x;
  LONG style;
  LPCSTR lpszName;
  LPCSTR lpszClass;
  DWORD dwExStyle;
} CREATESTRUCTA, CREATESTRUCT;

typedef struct tagMSG {
  HWND hwnd;
  UINT message;
  WPARAM wParam;
  LPARAM lParam;
  DWORD time;
  POINT pt;
} MSG;

typedef struct tagDEVMODEA {
  BYTE dmDeviceName[32];
  WORD dmSpecVersion;
  WORD dmDriverVersion;
  WORD dmSize;
  WORD dmDriverExtra;
  DWORD dmFields;
  LONG dmPositionX;
  LONG dmPositionY;
  DWORD dmDisplayOrientation;
  DWORD dmDisplayFixedOutput;
  short dmColor;
  short dmDuplex;
  short dmYResolution;
  short dmTTOption;
  short dmCollate;
  BYTE dmFormName[32];
  WORD dmLogPixels;
  DWORD dmBitsPerPel;
  DWORD dmPelsWidth;
  DWORD dmPelsHeight;
} DEVMODEA;

typedef struct tagOFNA {
  DWORD lStructSize;
  HWND hwndOwner;
  HINSTANCE hInstance;
  LPCSTR lpstrFilter;
  LPSTR lpstrCustomFilter;
  DWORD nMaxCustFilter;
  DWORD nFilterIndex;
  LPSTR lpstrFile;
  DWORD nMaxFile;
  LPSTR lpstrFileTitle;
  DWORD nMaxFileTitle;
  LPCSTR lpstrInitialDir;
  LPCSTR lpstrTitle;
  DWORD Flags;
  WORD nFileOffset;
  WORD nFileExtension;
  LPCSTR lpstrDefExt;
  LPARAM lCustData;
  void *lpfnHook;
  LPCSTR lpTemplateName;
} OPENFILENAMEA;
typedef void *LPOFNHOOKPROC;

typedef struct tagPAINTSTRUCT {
  HDC hdc;
  BOOL fErase;
  RECT rcPaint;
  BOOL fRestore;
  BOOL fIncUpdate;
  BYTE rgbReserved[32];
} PAINTSTRUCT;

typedef struct tagWINDOWPOS {
  HWND hwnd;
  HWND hwndInsertAfter;
  int x;
  int y;
  int cx;
  int cy;
  UINT flags;
} WINDOWPOS;

#define CS_VREDRAW 0x0001
#define CS_HREDRAW 0x0002
#define CS_DBLCLKS 0x0008
#define CS_OWNDC 0x0020
#define CS_SAVEBITS 0x0800

#define WS_OVERLAPPED 0x00000000L
#define WS_POPUP 0x80000000L
#define WS_CHILD 0x40000000L
#define WS_MINIMIZE 0x20000000L
#define WS_VISIBLE 0x10000000L
#define WS_DISABLED 0x08000000L
#define WS_CLIPSIBLINGS 0x04000000L
#define WS_CLIPCHILDREN 0x02000000L
#define WS_MAXIMIZE 0x01000000L
#define WS_CAPTION 0x00c00000L
#define WS_BORDER 0x00800000L
#define WS_DLGFRAME 0x00400000L
#define WS_VSCROLL 0x00200000L
#define WS_HSCROLL 0x00100000L
#define WS_SYSMENU 0x00080000L
#define WS_THICKFRAME 0x00040000L
#define WS_TABSTOP 0x00010000L
#define WS_GROUP 0x00020000L
#define WS_CHILDWINDOW WS_CHILD
#define WS_EX_TOPMOST 0x00000008L

#define SW_HIDE 0
#define SW_SHOWNORMAL 1
#define SW_SHOWMINIMIZED 2
#define SW_SHOWMAXIMIZED 3
#define SW_SHOWNOACTIVATE 4
#define SW_SHOW 5
#define SW_MINIMIZE 6
#define SW_RESTORE 9

#define MB_OK 0x00000000L
#define MB_YESNOCANCEL 0x00000003L
#define MB_ICONHAND 0x00000010L
#define MB_ICONQUESTION 0x00000020L
#define MB_ICONEXCLAMATION 0x00000030L
#define MB_SYSTEMMODAL 0x00001000L
#define MB_ICONWARNING MB_ICONEXCLAMATION
#define MB_ICONERROR MB_ICONHAND
#define MB_ICONSTOP MB_ICONHAND
#define MB_ICONINFORMATION 0x00000040L
#define IDOK 1
#define IDCANCEL 2
#define IDYES 6
#define IDNO 7

#define DLL_PROCESS_DETACH 0
#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH 2
#define DLL_THREAD_DETACH 3

#define SWP_NOSIZE 0x0001
#define SWP_NOMOVE 0x0002
#define SWP_NOZORDER 0x0004
#define SWP_NOACTIVATE 0x0010
#define SWP_SHOWWINDOW 0x0040
#define SWP_HIDEWINDOW 0x0080
#define HWND_TOP ((HWND)0)
#define HWND_BOTTOM ((HWND)(intptr_t)1)
#define HWND_TOPMOST ((HWND)(intptr_t)-1)
#define HWND_NOTOPMOST ((HWND)(intptr_t)-2)

#define GWL_WNDPROC (-4)
#define GWL_HINSTANCE (-6)
#define GWL_HWNDPARENT (-8)
#define GWL_ID (-12)
#define GWL_STYLE (-16)
#define GWL_EXSTYLE (-20)
#define GWL_USERDATA (-21)
#define GCL_HBRBACKGROUND (-10)
#define DWL_MSGRESULT 0
#define DWL_DLGPROC 4
#define DWL_USER 8
#define DLGWINDOWEXTRA 12

#define GW_HWNDFIRST 0
#define GW_HWNDLAST 1
#define GW_HWNDNEXT 2
#define GW_HWNDPREV 3
#define GW_OWNER 4
#define GW_CHILD 5

#define WM_NULL 0x0000
#define WM_CREATE 0x0001
#define WM_DESTROY 0x0002
#define WM_MOVE 0x0003
#define WM_SIZE 0x0005
#define WM_ACTIVATE 0x0006
#define WM_SETFOCUS 0x0007
#define WM_KILLFOCUS 0x0008
#define WM_ENABLE 0x000a
#define WM_SETTEXT 0x000c
#define WM_GETTEXT 0x000d
#define WM_GETTEXTLENGTH 0x000e
#define WM_PAINT 0x000f
#define WM_CLOSE 0x0010
#define WM_QUERYENDSESSION 0x0011
#define WM_QUERYOPEN 0x0013
#define WM_QUIT 0x0012
#define WM_ERASEBKGND 0x0014
#define WM_SHOWWINDOW 0x0018
#define WM_SETCURSOR 0x0020
#define WM_NCHITTEST 0x0084
#define WM_NCPAINT 0x0085
#define WM_NCACTIVATE 0x0086
#define WM_GETDLGCODE 0x0087
#define WM_NCCALCSIZE 0x0083
#define WM_GETMINMAXINFO 0x0024
#define WM_NCMOUSEMOVE 0x00a0
#define WM_NCLBUTTONDOWN 0x00a1
#define WM_NCRBUTTONDOWN 0x00a4
#define WM_DRAWITEM 0x002b
#define WM_MEASUREITEM 0x002c
#define WM_DELETEITEM 0x002d
#define WM_SETFONT 0x0030
#define WM_GETFONT 0x0031
#define WM_SETREDRAW 0x000b
#define WM_CTLCOLORBTN 0x0135
#define WM_CTLCOLORLISTBOX 0x0134
#define WM_CTLCOLORSTATIC 0x0138
#define WM_NCCREATE 0x0081
#define WM_NCDESTROY 0x0082
#define WM_COMMAND 0x0111
#define WM_INITDIALOG 0x0110
#define WM_SYSCOMMAND 0x0112
#define WM_TIMER 0x0113
#define WM_HSCROLL 0x0114
#define WM_VSCROLL 0x0115
#define WM_INITMENU 0x0116
#define WM_MENUSELECT 0x011f
#define WM_PARENTNOTIFY 0x0210
#define WM_NOTIFY 0x004e
#define WM_WINDOWPOSCHANGING 0x0046
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101
#define WM_CHAR 0x0102
#define WM_SYSKEYDOWN 0x0104
#define WM_MOUSEMOVE 0x0200
#define WM_LBUTTONDOWN 0x0201
#define WM_LBUTTONUP 0x0202
#define WM_LBUTTONDBLCLK 0x0203
#define WM_RBUTTONDOWN 0x0204
#define WM_RBUTTONUP 0x0205
#define WM_RBUTTONDBLCLK 0x0206
#define WM_MBUTTONDOWN 0x0207
#define WM_MBUTTONUP 0x0208
#define WM_MBUTTONDBLCLK 0x0209
#define WM_MOUSEWHEEL 0x020a
#define WM_DISPLAYCHANGE 0x007e
#define WM_QUERYNEWPALETTE 0x030f
#define WM_PALETTEISCHANGING 0x0310
#define WM_PALETTECHANGED 0x0311
#define WM_USER 0x0400

#define HTCAPTION 2
#define HTMINBUTTON 8
#define DLGC_BUTTON 0x2000
#define WA_ACTIVE 1
#define WA_CLICKACTIVE 2
#define HELP_CONTEXT 1
#define HELP_QUIT 2

#define PM_NOREMOVE 0x0000
#define PM_REMOVE 0x0001

#define BM_SETCHECK 0x00f1
#define BM_GETCHECK 0x00f0
#define BST_UNCHECKED 0
#define BST_CHECKED 1
#define BN_CLICKED 0
#define LBN_SELCHANGE 1
#define LBN_DBLCLK 2
#define CBN_SELCHANGE 1
#define EN_CHANGE 0x0300

#define EM_LIMITTEXT 0x00c5
#define EM_SETSEL 0x00b1

#define LB_ADDSTRING 0x0180
#define LB_ERRSPACE (-2)
#define LB_ERR (-1)
#define LB_INSERTSTRING 0x0181
#define LB_DELETESTRING 0x0182
#define LB_RESETCONTENT 0x0184
#define LB_SETSEL 0x0185
#define LB_SETCURSEL 0x0186
#define LB_GETSEL 0x0187
#define LB_GETCURSEL 0x0188
#define LB_GETTEXT 0x0189
#define LB_GETTEXTLEN 0x018a
#define LB_GETCOUNT 0x018b
#define LB_SELECTSTRING 0x018c
#define LB_DIR 0x018d
#define LB_GETTOPINDEX 0x018e
#define LB_FINDSTRING 0x018f
#define LB_GETSELCOUNT 0x0190
#define LB_GETSELITEMS 0x0191
#define LB_SETTABSTOPS 0x0192
#define LB_GETHORIZONTALEXTENT 0x0193
#define LB_SETHORIZONTALEXTENT 0x0194
#define LB_SETCOLUMNWIDTH 0x0195
#define LB_ADDFILE 0x0196
#define LB_SETTOPINDEX 0x0197
#define LB_GETITEMRECT 0x0198
#define LB_GETITEMDATA 0x0199
#define LB_SETITEMDATA 0x019a
#define LB_SELITEMRANGE 0x019b
#define LB_SETANCHORINDEX 0x019c
#define LB_GETANCHORINDEX 0x019d
#define LB_SETCARETINDEX 0x019e
#define LB_GETCARETINDEX 0x019f
#define LB_SETITEMHEIGHT 0x01a0

#define LBS_NOTIFY 0x0001L
#define LBS_SORT 0x0002L
#define LBS_MULTIPLESEL 0x0008L
#define LBS_NOINTEGRALHEIGHT 0x0100L
#define LBS_OWNERDRAWFIXED 0x0010L
#define LBS_HASSTRINGS 0x0040L
#define LBS_STANDARD (LBS_NOTIFY | LBS_SORT | WS_VSCROLL | WS_BORDER)

#define CB_GETEDITSEL 0x0140
#define CB_LIMITTEXT 0x0141
#define CB_SETEDITSEL 0x0142
#define CB_ADDSTRING 0x0143
#define CB_DELETESTRING 0x0144
#define CB_DIR 0x0145
#define CB_GETCOUNT 0x0146
#define CB_GETCURSEL 0x0147
#define CB_GETLBTEXT 0x0148
#define CB_GETLBTEXTLEN 0x0149
#define CB_INSERTSTRING 0x014a
#define CB_RESETCONTENT 0x014b
#define CB_FINDSTRING 0x014c
#define CB_SELECTSTRING 0x014d
#define CB_SETCURSEL 0x014e

#define MF_BYCOMMAND 0x0000
#define MF_BYPOSITION 0x0400
#define MF_SEPARATOR 0x0800
#define MF_ENABLED 0x0000
#define MF_CHECKED 0x0008
#define MF_UNCHECKED 0x0000
#define MF_GRAYED 0x0001
#define MF_DISABLED 0x0002
#define MF_STRING 0x0000
#define MF_POPUP 0x0010
#define MF_REMOVE 0x1000
#define TPM_RIGHTBUTTON 0x0002

#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_ESCAPE 0x1b
#define VK_RETURN 0x0d
#define VK_SPACE 0x20
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_TAB 0x09
#define VK_BACK 0x08
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F12 0x7b

#define SB_HORZ 0
#define SB_VERT 1
#define SB_CTL 2
#define SB_THUMBPOSITION 4
#define SB_THUMBTRACK 5
#define SB_LINEUP 0
#define SB_LINELEFT 0
#define SB_LINEDOWN 1
#define SB_LINERIGHT 1
#define SB_PAGEUP 2
#define SB_PAGELEFT 2
#define SB_PAGEDOWN 3
#define SB_PAGERIGHT 3
#define SB_TOP 6
#define SB_LEFT 6
#define SB_BOTTOM 7
#define SB_RIGHT 7
#define SB_ENDSCROLL 8
#define SBM_GETPOS 0x00e1
#define SBM_SETRANGE 0x00e2
#define SBM_SETPOS 0x00e0
#define SBM_GETRANGE 0x00e3

#define MK_LBUTTON 0x0001
#define MK_SHIFT 0x0004
#define MK_RBUTTON 0x0002

#define BS_OWNERDRAW 0x0000000bL
#define BS_CHECKBOX 0x00000002L
#define BS_AUTOCHECKBOX 0x00000003L
#define BS_GROUPBOX 0x00000007L
#define BS_AUTORADIOBUTTON 0x00000009L
#define BS_DEFPUSHBUTTON 0x00000001L
#define BS_TYPEMASK 0x0000000fL
#define SS_LEFT 0x00000000L
#define SS_CENTER 0x00000001L
#define SS_RIGHT 0x00000002L
#define SS_SIMPLE 0x0000000bL
#define SS_LEFTNOWORDWRAP 0x0000000cL
#define SS_TYPEMASK 0x0000001fL
#define SS_NOPREFIX 0x00000080L
#define SS_CENTERIMAGE 0x00000200L
#define CBS_DROPDOWNLIST 0x0003L
#define CBS_SORT 0x0100L
#define ES_MULTILINE 0x0004L
#define ES_READONLY 0x0800L
#define SC_MINIMIZE 0xf020
#define SC_MOVE 0xf010
#define LOCALE_SYSTEM_DEFAULT 0x0800
#define GENERIC_READ 0x80000000u
#define FILE_SHARE_READ 0x00000001u
#define OPEN_EXISTING 3
#define FILE_ATTRIBUTE_NORMAL 0x00000080u
#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000u
#define FILE_FLAG_DELETE_ON_CLOSE 0x04000000u
#define VER_PLATFORM_WIN32_WINDOWS 1
#define INFINITE 0xffffffffu
#define WAIT_OBJECT_0 0
#define WAIT_TIMEOUT 258
#define STILL_ACTIVE 259
#define THREAD_PRIORITY_NORMAL 0

#define SM_CXSCREEN 0
#define SM_CYSCREEN 1
#define SM_CXVSCROLL 2
#define SM_CXBORDER 5
#define SM_CYCAPTION 4
#define SM_CYHSCROLL 3
#define SM_CXSIZE 30
#define SM_CXCURSOR 13
#define SM_SLOWMACHINE 73

#define SIZE_RESTORED 0
#define SIZE_MINIMIZED 1
#define MB_YESNO 0x00000004u
#define ODS_GRAYED 0x0002
#define ODS_DISABLED 0x0004

#define MAKEINTRESOURCEA(id) ((LPSTR)(uintptr_t)(WORD)(id))
#define MAKEINTRESOURCE MAKEINTRESOURCEA
#define IDC_ARROW MAKEINTRESOURCEA(32512)
#define IDI_APPLICATION MAKEINTRESOURCEA(32512)

#ifndef LOWORD
#define LOWORD(value) ((WORD)((uintptr_t)(value) & 0xffffu))
#endif
#ifndef HIWORD
#define HIWORD(value) ((WORD)(((uintptr_t)(value) >> 16) & 0xffffu))
#endif
#define MAKELONG(low, high) ((LONG)(((WORD)(low)) | ((DWORD)((WORD)(high))) << 16))
#define MAKEWPARAM(low, high) ((WPARAM)(uintptr_t)MAKELONG((low), (high)))
#define MAKELPARAM(low, high) ((LPARAM)(intptr_t)MAKELONG((low), (high)))

ATOM RegisterClassA(const WNDCLASSA *window_class);
BOOL UnregisterClassA(LPCSTR class_name, HINSTANCE instance);
HWND CreateWindowExA(DWORD ex_style, LPCSTR class_name, LPCSTR window_name,
                     DWORD style, int x, int y, int width, int height,
                     HWND parent, HMENU menu, HINSTANCE instance, LPVOID param);
BOOL DestroyWindow(HWND window);
LRESULT DefWindowProcA(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
LRESULT CallWindowProcA(WNDPROC proc, HWND window, UINT message, WPARAM wparam, LPARAM lparam);
LRESULT SendMessageA(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
BOOL PostMessageA(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
void PostQuitMessage(int exit_code);
BOOL PeekMessageA(MSG *message, HWND window, UINT min_message, UINT max_message, UINT remove);
BOOL GetMessageA(MSG *message, HWND window, UINT min_message, UINT max_message);
LRESULT DispatchMessageA(const MSG *message);
BOOL TranslateMessage(const MSG *message);
int TranslateAcceleratorA(HWND window, HACCEL accelerator, MSG *message);

LONG GetWindowLongA(HWND window, int index);
LONG SetWindowLongA(HWND window, int index, LONG value);
DWORD GetClassLongA(HWND window, int index);
DWORD SetClassLongA(HWND window, int index, LONG value);
WORD GetWindowWord(HWND window, int index);
WORD SetWindowWord(HWND window, int index, WORD value);
HWND GetParent(HWND window);
HWND SetParent(HWND child, HWND parent);
HWND GetWindow(HWND window, UINT command);
HWND GetTopWindow(HWND parent);
BOOL EnumChildWindows(HWND parent, WNDENUMPROC proc, LPARAM param);
HWND GetDlgItem(HWND parent, int id);
int GetDlgCtrlID(HWND window);
int GetClassNameA(HWND window, LPSTR class_name, int max_count);
DWORD GetWindowThreadProcessId(HWND window, DWORD *process_id);

BOOL GetClientRect(HWND window, RECT *rect);
BOOL GetWindowRect(HWND window, RECT *rect);
BOOL AdjustWindowRect(RECT *rect, DWORD style, BOOL menu);
BOOL MoveWindow(HWND window, int x, int y, int width, int height, BOOL repaint);
BOOL SetWindowPos(HWND window, HWND insert_after, int x, int y, int width, int height, UINT flags);
BOOL ShowWindow(HWND window, int command);
BOOL IsWindowVisible(HWND window);
BOOL EnableWindow(HWND window, BOOL enable);
BOOL IsWindowEnabled(HWND window);
BOOL IsWindow(HWND window);
void shandalar_win32_set_render_caption(HWND window, BOOL render);
BOOL shandalar_win32_should_render_caption(HWND window);
BOOL shandalar_win32_accepts_double_click(HWND window);
BOOL ClientToScreen(HWND window, POINT *point);
BOOL ScreenToClient(HWND window, POINT *point);
int MapWindowPoints(HWND from, HWND to, POINT *points, UINT count);

BOOL SetWindowTextA(HWND window, LPCSTR text);
int GetWindowTextA(HWND window, LPSTR text, int max_count);
int GetWindowTextLengthA(HWND window);
HWND SetFocus(HWND window);
HWND GetFocus(void);
HWND SetActiveWindow(HWND window);
HWND SetCapture(HWND window);
HWND GetCapture(void);
BOOL ReleaseCapture(void);
BOOL BringWindowToTop(HWND window);
BOOL SetForegroundWindow(HWND window);

BOOL InvalidateRect(HWND window, const RECT *rect, BOOL erase);
BOOL UpdateWindow(HWND window);
HDC BeginPaint(HWND window, PAINTSTRUCT *paint);
BOOL EndPaint(HWND window, const PAINTSTRUCT *paint);
HDC GetWindowDC(HWND window);
HWND GetDesktopWindow(void);
HWND FindWindowA(LPCSTR class_name, LPCSTR window_name);
HWND FindWindowExA(HWND parent, HWND child_after, LPCSTR class_name, LPCSTR window_name);
BOOL GetCursorPos(POINT *point);
BOOL SetCursorPos(int x, int y);
HWND WindowFromPoint(POINT point);
UINT_PTR SetTimer(HWND window, UINT_PTR id, UINT interval_ms, TIMERPROC proc);
BOOL KillTimer(HWND window, UINT_PTR id);
int GetSystemMetrics(int index);
HACCEL LoadAcceleratorsA(HINSTANCE instance, LPCSTR resource_name);
HICON LoadIconA(HINSTANCE instance, LPCSTR resource_name);
BOOL DestroyIcon(HICON icon);
HBITMAP LoadBitmapA(HINSTANCE instance, LPCSTR resource_name);
HCURSOR LoadCursorA(HINSTANCE instance, LPCSTR resource_name);
HCURSOR SetCursor(HCURSOR cursor);
SHORT GetKeyState(int virtual_key);
LONG GetMessageTime(void);
BOOL IsIconic(HWND window);
void InitCommonControls(void);
BOOL MessageBeep(UINT type);
BOOL WinHelpA(HWND window, LPCSTR help_file, UINT command, ULONG_PTR data);

HMENU CreatePopupMenu(void);
BOOL AppendMenuA(HMENU menu, UINT flags, UINT_PTR item, LPCSTR text);
BOOL InsertMenuA(HMENU menu, UINT position, UINT flags, UINT_PTR item, LPCSTR text);
BOOL ModifyMenuA(HMENU menu, UINT position, UINT flags, UINT_PTR item, LPCSTR text);
UINT CheckMenuItem(HMENU menu, UINT item, UINT flags);
UINT EnableMenuItem(HMENU menu, UINT item, UINT flags);
BOOL DeleteMenu(HMENU menu, UINT item, UINT flags);
BOOL RemoveMenu(HMENU menu, UINT item, UINT flags);
BOOL DestroyMenu(HMENU menu);
int GetMenuItemCount(HMENU menu);
BOOL TrackPopupMenu(HMENU menu, UINT flags, int x, int y, int reserved,
                    HWND window, const RECT *exclude);
HMENU shandalar_win32_active_popup(HWND *owner, int *x, int *y);
int shandalar_win32_menu_item_count(HMENU menu);
BOOL shandalar_win32_menu_item(HMENU menu, int index, UINT *flags,
                               UINT_PTR *id, LPCSTR *text);
void shandalar_win32_choose_menu_item(UINT_PTR id);
void shandalar_win32_dismiss_popup(void);
BOOL SetScrollRange(HWND window, int bar, int minimum, int maximum, BOOL redraw);
BOOL GetScrollRange(HWND window, int bar, int *minimum, int *maximum);
int SetScrollPos(HWND window, int bar, int position, BOOL redraw);
int GetScrollPos(HWND window, int bar);
BOOL ScrollWindow(HWND window, int dx, int dy, const RECT *scroll, const RECT *clip);
UINT GetDoubleClickTime(void);
BOOL LockWindowUpdate(HWND window);
int GetDateFormatA(DWORD locale, DWORD flags, const void *date,
                   LPCSTR format, LPSTR output, int output_size);
COLORREF GetSysColor(int index);

INT_PTR DialogBoxParamA(HINSTANCE instance, LPCSTR template_name, HWND parent,
                        DLGPROC dialog_proc, LPARAM init_param);
BOOL EndDialog(HWND dialog, INT_PTR result);
HWND shandalar_win32_active_dialog(void);
LRESULT SendDlgItemMessageA(HWND dialog, int id, UINT message, WPARAM wparam, LPARAM lparam);
BOOL SetDlgItemTextA(HWND dialog, int id, LPCSTR text);
UINT GetDlgItemTextA(HWND dialog, int id, LPSTR text, int max_count);
BOOL SetDlgItemInt(HWND dialog, int id, UINT value, BOOL is_signed);
UINT GetDlgItemInt(HWND dialog, int id, BOOL *translated, BOOL is_signed);
UINT IsDlgButtonChecked(HWND dialog, int id);
BOOL CheckDlgButton(HWND dialog, int id, UINT check);
BOOL CheckRadioButton(HWND dialog, int first_id, int last_id, int check_id);

typedef HWND (*ShandalarDialogFactory)(HINSTANCE instance, LPCSTR template_name, HWND parent);
void shandalar_win32_set_dialog_factory(ShandalarDialogFactory factory);
BOOL shandalar_win32_register_dialog_factory(UINT resource_id,
                                             ShandalarDialogFactory factory);
HWND shandalar_win32_create_dialog(HINSTANCE instance, HWND parent, LPCSTR title,
                                   int x, int y, int width, int height);

HMODULE GetModuleHandleA(LPCSTR module_name);
FARPROC GetProcAddress(HMODULE module, LPCSTR proc_name);
BOOL shandalar_win32_register_proc(LPCSTR proc_name, FARPROC proc);
DWORD GetCurrentDirectoryA(DWORD size, LPSTR buffer);
BOOL SetCurrentDirectoryA(LPCSTR path);
BOOL CopyFileA(LPCSTR existing_filename, LPCSTR new_filename, BOOL fail_if_exists);
void Sleep(DWORD milliseconds);
DWORD GetTickCount(void);
HANDLE GetCurrentProcess(void);
HANDLE GetCurrentThread(void);
BOOL GetThreadTimes(HANDLE thread, FILETIME *creation, FILETIME *exit,
                    FILETIME *kernel, FILETIME *user);
BOOL DuplicateHandle(HANDLE source_process, HANDLE source_handle,
                     HANDLE target_process, HANDLE *target_handle,
                     DWORD access, BOOL inherit, DWORD options);
void ExitThread(DWORD exit_code);
LONG ChangeDisplaySettingsA(DEVMODEA *dev_mode, DWORD flags);
HANDLE CreateFileA(LPCSTR filename, DWORD desired_access, DWORD share_mode,
                   LPSECURITY_ATTRIBUTES security_attributes,
                   DWORD creation_disposition, DWORD flags_and_attributes,
                   HANDLE template_file);
DWORD GetFileSize(HANDLE file, DWORD *high_size);
BOOL ReadFile(HANDLE file, LPVOID buffer, DWORD bytes_to_read,
              DWORD *bytes_read, LPOVERLAPPED overlapped);
BOOL DeviceIoControl(HANDLE device, DWORD io_control_code, LPVOID in_buffer,
                     DWORD in_buffer_size, LPVOID out_buffer,
                     DWORD out_buffer_size, DWORD *bytes_returned,
                     LPOVERLAPPED overlapped);
UINT GetDriveTypeA(LPCSTR root_path_name);
UINT GetWindowsDirectoryA(LPSTR buffer, UINT size);
UINT SetSystemPaletteUse(HDC dc, UINT use);
BOOL GetSaveFileNameA(OPENFILENAMEA *open_file);
DWORD FormatMessageA(DWORD flags, LPCVOID source, DWORD message_id, DWORD language_id,
                     LPSTR buffer, DWORD size, va_list *arguments);
UINT timeBeginPeriod(UINT period);
UINT timeEndPeriod(UINT period);
UINT timeSetEvent(UINT delay, UINT resolution, LPTIMECALLBACK callback,
                  DWORD user, UINT event);
UINT timeKillEvent(UINT timer_id);

typedef BOOL (*ShandalarWin32PumpProc)(BOOL wait);
void shandalar_win32_set_pump_proc(ShandalarWin32PumpProc proc);
BOOL shandalar_win32_pump(BOOL wait);
void shandalar_win32_clear_quit_message(void);
void shandalar_win32_set_system_drag_endpoint(int x, int y);
void shandalar_win32_clear_system_drag_endpoint(void);
void shandalar_win32_set_cursor_position(int x, int y);
void shandalar_win32_runtime_tick(void);
void shandalar_win32_runtime_reset(void);
void shandalar_win32_mm_timers_tick(void);
void shandalar_win32_mm_timers_reset(void);
LRESULT shandalar_control_message(HWND window, UINT message, WPARAM wparam,
                                 LPARAM lparam, BOOL *handled);
void shandalar_controls_reset(void);
void shandalar_win32_reset(void);
HDC shandalar_window_backing_dc(HWND window);
uint64_t shandalar_window_content_revision(HWND window);
void shandalar_window_content_changed(HWND window);

#include "platform/winreg_compat.h"

#define RegisterClass RegisterClassA
#define UnregisterClass UnregisterClassA
#define CreateWindowEx CreateWindowExA
#define DefWindowProc DefWindowProcA
#define FindWindow FindWindowA
#define FindWindowEx FindWindowExA
#define SendMessage SendMessageA
#define PostMessage PostMessageA
#define PeekMessage PeekMessageA
#define GetMessage GetMessageA
#define DispatchMessage DispatchMessageA
#define TranslateAccelerator TranslateAcceleratorA
#define LoadAccelerators LoadAcceleratorsA
#define LoadIcon LoadIconA
#define LoadCursor LoadCursorA
#define DialogBoxParam DialogBoxParamA
#define AppendMenu AppendMenuA
#define InsertMenu InsertMenuA
#define GetDateFormat GetDateFormatA
#define SetCurrentDirectory SetCurrentDirectoryA
#define GetWindowLong GetWindowLongA
#define SetWindowLong SetWindowLongA
#define SetWindowText SetWindowTextA
#define GetWindowText GetWindowTextA
#define GetWindowTextLength GetWindowTextLengthA

#ifdef __cplusplus
}
#endif
#endif
