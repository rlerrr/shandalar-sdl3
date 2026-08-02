#ifndef SHANDALAR_MAGIC_VFW_COMPAT_H
#define SHANDALAR_MAGIC_VFW_COMPAT_H

#include "windows.h"

#define MCIWNDM_SETPALETTE (WM_USER + 127)

HWND MCIWndCreateA(HWND parent, HINSTANCE instance, DWORD style,
                   LPCSTR filename);

#endif
