#ifndef __CARDARTLIB_H__
#define __CARDARTLIB_H__

#include "platform/gdi_compat.h"

#ifdef __cplusplus
extern "C" {
#endif
	int LoadBigArt(card_id_t id, int version, int width, int height);
	int LoadSmallArt(card_id_t id, int version, int width, int height);
	void DestroyBigArt(card_id_t id, int version);
	void DestroySmallArt(card_id_t id, int version);
	int IsBigArtRightSize(card_id_t id, int version, int width, int height);
	int IsSmallArtRightSize(card_id_t id, int version, int width, int height);
	void DestroyAllBigArts(void);
	void DestroyAllSmallArts(void);
	void *IsBigArtIn(card_id_t id, int version);
	int IsSmallArtIn(card_id_t id, int version);
	int ReloadBigArtIfWrongSize(card_id_t id, int version, int width, int height);
	int ReloadSmallArtIfWrongSize(card_id_t id, int version, int width, int height);
	int DrawBigArt(HDC hdc, RECT* rect, card_id_t id, int version);
	int DrawSmallArt(HDC hdc, RECT* rect, card_id_t id, int version);
	int CardArtLib_InitializeFromRoot(const char *asset_root);
	void CardArtLib_Shutdown(void);
#ifdef __cplusplus
}
#endif

#endif
