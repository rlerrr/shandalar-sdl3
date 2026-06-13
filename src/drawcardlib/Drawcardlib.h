#ifndef __DRAWCARDLIB_H__
#define __DRAWCARDLIB_H__

#include "platform/gdi_compat.h"
#include "defs.h"
#include "mystdbool.h"

#if defined(_WIN32) && defined(SHANDALAR_BUILD_DLL)
#define DRAWCARD_API __declspec(dllexport)
#elif defined(_WIN32) && defined(SHANDALAR_USE_DLL)
#define DRAWCARD_API __declspec(dllimport)
#else
#define DRAWCARD_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

    DRAWCARD_API void DrawSmallCardTitle(HDC dc, RECT *rect, char* name, int colorMode, int transparentBackground);
    DRAWCARD_API void DrawSmallCard(HDC dc,RECT *rect, card_ptr_t *card, uint version, int reloadIfWrongSize);
    DRAWCARD_API uint CalcDrawManaText(HDC dc,RECT *rect,char* text);
    DRAWCARD_API uint DrawManaText(HDC dc,RECT *rect,char *text,int drawSymbols);
    DRAWCARD_API void DrawCardBack(HDC dc,RECT *rect);
    DRAWCARD_API void DestroyAllCardBackgrounds(void);
    DRAWCARD_API uint DrawFullCard(HDC dc, RECT *rect, card_ptr_t *card, uint version, uint drawFlags, bool expanded_text, LPCSTR artistFormat);
    DRAWCARD_API int DrawCardLib_InitializeFromRoot(const char *asset_root);
    DRAWCARD_API void DrawCardLib_Shutdown(void);

#ifdef __cplusplus
}
#endif

#undef DRAWCARD_API

#endif
