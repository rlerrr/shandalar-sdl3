#ifndef __CARDARTLIB_H__
#define __CARDARTLIB_H__

#include "platform/gdi_compat.h"

#if defined(_WIN32) && defined(SHANDALAR_BUILD_DLL)
#define CARDART_API __declspec(dllexport)
#elif defined(_WIN32) && defined(SHANDALAR_USE_DLL)
#define CARDART_API __declspec(dllimport)
#else
#define CARDART_API
#endif

#ifdef __cplusplus
extern "C" {
#endif
	CARDART_API int LoadBigArt(card_id_t id, int version, int width, int height);
	CARDART_API int LoadSmallArt(card_id_t id, int version, int width, int height);
	CARDART_API void DestroyBigArt(card_id_t id, int version);
	CARDART_API void DestroySmallArt(card_id_t id, int version);
	CARDART_API int IsBigArtRightSize(card_id_t id, int version, int width, int height);
	CARDART_API int IsSmallArtRightSize(card_id_t id, int version, int width, int height);
	CARDART_API void DestroyAllBigArts(void);
	CARDART_API void DestroyAllSmallArts(void);
	CARDART_API void *IsBigArtIn(card_id_t id, int version);
	CARDART_API int IsSmallArtIn(card_id_t id, int version);
	CARDART_API int ReloadBigArtIfWrongSize(card_id_t id, int version, int width, int height);
	CARDART_API int ReloadSmallArtIfWrongSize(card_id_t id, int version, int width, int height);
	CARDART_API int DrawBigArt(HDC hdc, RECT* rect, card_id_t id, int version);
	CARDART_API int DrawSmallArt(HDC hdc, RECT* rect, card_id_t id, int version);
	CARDART_API int CardArtLib_InitializeFromRoot(const char *asset_root);
	CARDART_API void CardArtLib_Shutdown(void);
	int WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID reserved);
#ifdef __cplusplus
}
#endif

#undef CARDART_API

#endif
