#include "platform/gdi_compat.h"
#include "cardartlib/src/palette.h"

// FUNCTION: DRAWCARDLIB 0x10007296
// FUNCTION: DECKDLL 0x10015fe0
// FUNCTION: MAGIC 0x0055d9ed
// FUNCTION: SHANDALAR 0x00573d07
int GetNextManaSymbol(char **textCursor)
{
  char result;
  char *chr;

  chr = (char *)*textCursor;
  result = '\0';
  if (*chr == '|')
  {
    if (chr[1] == 'X')
    {
      result = -0x10;
    }
    else if ((chr[1] == '1') && (chr[2] == '0'))
    {
      result = -0x11;
      chr = chr + 1;
    }
    else if (chr[1] == '0')
    {
      result = -0xf;
    }
    else if (chr[1] == '1')
    {
      result = -0xe;
    }
    else if (chr[1] == '2')
    {
      result = -0xd;
    }
    else if (chr[1] == '3')
    {
      result = -0xc;
    }
    else if (chr[1] == '4')
    {
      result = -0xb;
    }
    else if (chr[1] == '5')
    {
      result = -10;
    }
    else if (chr[1] == '6')
    {
      result = -9;
    }
    else if (chr[1] == '7')
    {
      result = -8;
    }
    else if (chr[1] == '8')
    {
      result = -7;
    }
    else if (chr[1] == '9')
    {
      result = -6;
    }
    else if (chr[1] == 'T')
    {
      result = -0x12;
    }
    else if (chr[1] == 'B')
    {
      result = -2;
    }
    else if (chr[1] == 'U')
    {
      result = -3;
    }
    else if (chr[1] == 'W')
    {
      result = -5;
    }
    else if (chr[1] == 'G')
    {
      result = -1;
    }
    else if (chr[1] == 'R')
    {
      result = -4;
    }
  }
  if (result != '\0')
  {
    chr++;
    chr++;
  }
  *textCursor = chr;
  return (int)result;
}

// FUNCTION: DRAWCARDLIB 0x1000aab9
// FUNCTION: DECKDLL 0x10023b0b
// FUNCTION: MAGIC 0x00494270
// FUNCTION: SHANDALAR 0x00464d0b
BOOL DrawMaskedBitmapToRect(HDC dc, RECT *rect, HANDLE bitmap, int wSrc, int hSrc, int srcX, int srcY, int maskX, int maskY)

{
  int hDest;
  BITMAP bm;
  int wDest;
  int nSavedDC;
  int yDest;
  int xDest;

  if (((dc == (HDC)0x0) || (rect == (RECT *)0x0)) || (bitmap == (HANDLE)0x0))
  {
    return 0;
  }
  EnterCriticalSection(&global_critical_section_for_drawing);
  nSavedDC = SaveDC(dc);
  SelectObject(global_screen_dc, bitmap);
  GetObjectA(bitmap, sizeof(bm), &bm);
  xDest = rect->left;
  yDest = rect->top;
  if (rect->right >= rect->left)
  {
    wDest = rect->right - rect->left;
  }
  else
  {
    wDest = wSrc;
  }
  if (rect->top <= rect->bottom)
  {
    hDest = rect->bottom - rect->top;
  }
  else
  {
    hDest = hSrc;
  }
  ApplyCardArtPaletteToDc(global_screen_dc);
  StretchBlt(dc, xDest, yDest, wDest, hDest, global_screen_dc, maskX, maskY, wSrc, hSrc, SRCAND);
  ApplyCardArtPaletteToDc(global_screen_dc);
  StretchBlt(dc, xDest, yDest, wDest, hDest, global_screen_dc, srcX, srcY, wSrc, hSrc, SRCPAINT);
  RestoreDC(dc, nSavedDC);
  LeaveCriticalSection(&global_critical_section_for_drawing);
  return 1;
}

// FUNCTION: DECKDLL 0x10023972
// FUNCTION: MAGIC 0x004940d6
// FUNCTION: SHANDALAR 0x00464b72
BOOL TileBitmapIntoRect(HDC hdc, RECT *r, HBITMAP bmp)
{
  BITMAP bm;
  RECT dst;
  int y;
  int saved;
  int x;

  if (!hdc || !r || !bmp)
    return FALSE;

  saved = SaveDC(hdc);
  IntersectClipRect(hdc, r->left, r->top, r->right, r->bottom);

  GetObjectA(bmp, sizeof(bm), &bm);

  for (x = r->left; x < r->right; x += bm.bmWidth)
  {
    for (y = r->top; y < r->bottom; y += bm.bmHeight)
    {
      SetRect(&dst, x, y, x - 1, y - 1);
      DrawBitmapSubrectToRect(hdc, &dst, bmp, 0, 0, bm.bmWidth, bm.bmHeight);
    }
  }

  RestoreDC(hdc, saved);
  return TRUE;
}

// FUNCTION: DECKDLL 0x10024ed5
// FUNCTION: MAGIC 0x0049563b
// FUNCTION: SHANDALAR 0x004660d6
void draw_item(DRAWITEMSTRUCT *item, HBRUSH brush, HANDLE hbmp_bkgrd, HPEN pen1, HPEN pen2, COLORREF col, int do_focus, UINT format)
{
  HDC hdc;       /* ebp - 0x64 */
  RECT focus;    /* ebp - 0x60 */
  HGDIOBJ hfont; /* ebp - 0x50 */
  RECT r;        /* ebp - 0x4c */
  char txt[52];  /* ebp - 0x3c */
  SIZE sz;       /* ebp - 0xc */

  hdc = item->hDC;
  CopyRect(&r, &item->rcItem);
  GetWindowText(item->hwndItem, txt, 0x32);
  ApplyCardArtPaletteToDc(hdc);

  OffsetRect(&r, -item->rcItem.left, -item->rcItem.top);

  if (item->itemState & ODS_SELECTED)
  {
    if (brush)
      FillRect(hdc, &r, brush);
    else if (hbmp_bkgrd)
      TileBitmapIntoRect(hdc, &r, hbmp_bkgrd);

    SelectObject(hdc, GetStockObject(BLACK_PEN));
    MoveToEx(hdc, 0, 0, NULL);
    LineTo(hdc, r.right, 0);
    MoveToEx(hdc, 0, 0, NULL);
    LineTo(hdc, 0, r.bottom);

    SelectObject(hdc, pen2);
    MoveToEx(hdc, 1, 1, NULL);
    LineTo(hdc, r.right - 1, 1);
    MoveToEx(hdc, 1, 1, NULL);
    LineTo(hdc, 1, r.bottom - 1);

    SelectObject(hdc, pen1);
    MoveToEx(hdc, r.right - 1, 1, NULL);
    LineTo(hdc, r.right - 1, r.bottom);
    MoveToEx(hdc, 1, r.bottom - 1, NULL);
    LineTo(hdc, r.right, r.bottom - 1);

    OffsetRect(&r, 2, 2);
  }
  else
  {
    if (brush)
      FillRect(hdc, &r, brush);
    else if (hbmp_bkgrd)
      TileBitmapIntoRect(hdc, &r, hbmp_bkgrd);

    SelectObject(hdc, pen1);
    MoveToEx(hdc, 0, 0, NULL);
    LineTo(hdc, r.right, 0);
    MoveToEx(hdc, 0, 0, NULL);
    LineTo(hdc, 0, r.bottom);
    MoveToEx(hdc, 1, 1, NULL);
    LineTo(hdc, r.right - 1, 1);
    MoveToEx(hdc, 1, 1, NULL);
    LineTo(hdc, 1, r.bottom - 1);

    SelectObject(hdc, pen2);
    MoveToEx(hdc, r.right - 1, 1, NULL);
    LineTo(hdc, r.right - 1, r.bottom);
    MoveToEx(hdc, 1, r.bottom - 1, NULL);
    LineTo(hdc, r.right, r.bottom - 1);
    MoveToEx(hdc, r.right - 2, 2, NULL);
    LineTo(hdc, r.right - 2, r.bottom - 1);
    MoveToEx(hdc, 2, r.bottom - 2, NULL);
    LineTo(hdc, r.right - 1, r.bottom - 2);
  }

  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, col);
  hfont = (HGDIOBJ)SendMessageA(item->hwndItem, WM_GETFONT, 0, 0);
  SelectObject(hdc, hfont);
  DrawTextA(hdc, txt, -1, &r, format);

  if (do_focus && (item->itemState & ODS_FOCUS))
  {
    GetTextExtentPoint32A(hdc, txt, strlen(txt), &sz);

    focus.left = (r.right - r.left) / 2 - sz.cx / 2 - 3;
    focus.right = sz.cx + focus.left + 6;
    focus.top = (r.bottom - r.top) / 2 - sz.cy / 2 - 3;
    focus.bottom = sz.cy + focus.top + 6;
    DrawFocusRect(hdc, &focus);
  }
}
