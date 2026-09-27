#ifndef __DRAWCARDLIB_H__
#define __DRAWCARDLIB_H__

#include "platform/gdi_compat.h"
#include "defs.h"
#include "mystdbool.h"

#ifdef __cplusplus
extern "C" {
#endif

    void DrawSmallCardTitle(HDC dc, RECT *rect, char* name, int colorMode, int transparentBackground);
    void DrawSmallCard(HDC dc,RECT *rect, card_ptr_t *card, uint version, int reloadIfWrongSize);
    uint CalcDrawManaText(HDC dc,RECT *rect,char* text);
    uint DrawManaText(HDC dc,RECT *rect,char *text,int drawSymbols);
    void DrawCardBack(HDC dc,RECT *rect);
    void DestroyAllCardBackgrounds(void);
    uint DrawFullCard(HDC dc, RECT *rect, card_ptr_t *card, uint version, uint drawFlags, bool expanded_text, LPCSTR artistFormat);
    int DrawCardLib_InitializeFromRoot(const char *asset_root);
    void DrawCardLib_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
