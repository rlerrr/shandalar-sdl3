#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "inttypes.h"
#include "mystdbool.h"
#include "defs.h"
#include "platform/legacy_io.h"
#include "catalog.h"
#include "haar.h"
#include "assert.h"
#include "palette.h"

// GLOBAL: CARDARTLIB 0x101177f4
int g_versionedBigArtCount;

// GLOBAL: CARDARTLIB 0x10117910
char global_cartart_directory[0x105];

// GLOBAL: CARDARTLIB 0x10121fe0
int g_versionedSmallArtCount;

// GLOBAL: CARDARTLIB 0x10121fe4
HINSTANCE global_hinstance;

// GLOBAL: CARDARTLIB 0x10117a20
CRITICAL_SECTION global_critical_section_for_small_art;

// GLOBAL: CARDARTLIB 0x101221f0
CRITICAL_SECTION global_critical_section_for_big_art;

// GLOBAL: CARDARTLIB 0x101200a0
int g_cardPicCounts[2000];

// SIZE 0x10
typedef struct ArtCacheEntry
{
  HBITMAP hbm;
  void *bits;
  int width;
  int height;
} ArtCacheEntry;

// GLOBAL: CARDARTLIB 0x10117a40
ArtCacheEntry g_SmallArtCache[2000];

// SIZE 0x18
typedef struct VersionedArtCacheEntry
{
  HBITMAP hbm;
  void *bits;
  int width;
  int height;
  card_id_t id;
  int version;
} VersionedArtCacheEntry;

// GLOBAL: CARDARTLIB 0x1011f740
VersionedArtCacheEntry g_versionedSmallArtCache[100];

// GLOBAL: CARDARTLIB 0x10121ff0
VersionedArtCacheEntry g_versionedBigArtCache[0x14];

void CardArtLib_Shutdown(void);
int LoadBigArt(card_id_t id, int version, int width, int height);
int LoadSmallArt(card_id_t id, int version, int width, int height);
void DestroyBigArt(card_id_t id, int version);
void DestroySmallArt(card_id_t id, int version);
int IsBigArtRightSize(card_id_t id, int version, int width, int height);
void DestroyAllBigArts(void);
void DestroyAllSmallArts(void);
void *IsBigArtIn(card_id_t id, int version);
int IsSmallArtIn(card_id_t id, int version);
int ReloadBigArtIfWrongSize(card_id_t id, int version, int width, int height);
bool ReloadSmallArtIfWrongSize(card_id_t id, int version, int width, int height);
int DrawBigArt(HDC hdc, RECT *rect, card_id_t id, int version);
int DrawSmallArt(HDC hdc, RECT *rect, card_id_t id, int version);
VersionedArtCacheEntry *FindVersionedSmallArtCacheEntry(card_id_t id, int version);
void DestroyVersionedSmallArt(card_id_t id, int version);
void DestroyAllVersionedSmallArts(void);
int LoadVersionedSmallArt(card_id_t id, int version, int width, int height);
int DrawVersionedSmallArt(HDC hdc, RECT *rect, card_id_t id, int version);
bool ReloadVersionedSmallArtIfWrongSize(card_id_t id, int version, int width, int height);

int CardArtLib_InitializeFromRoot(const char *asset_root)
{
  char cards_dat_path[0x208];
  card_ptr_t *records;
  int card_count = 0;
  int unused_header_value;
  int idx;
  int i;
  FILE *cards_file;

  if (!asset_root || !asset_root[0])
    return 0;

  strncpy(global_base_directory, asset_root, sizeof(global_base_directory) - 1);
  global_base_directory[sizeof(global_base_directory) - 1] = '\0';
  strncpy(global_cartart_directory, asset_root, sizeof(global_cartart_directory) - 1);
  global_cartart_directory[sizeof(global_cartart_directory) - 1] = '\0';
  strncat(global_cartart_directory, "/CardArt",
          sizeof(global_cartart_directory) - strlen(global_cartart_directory) - 1);

  InitializeCriticalSection(&global_critical_section_for_small_art);
  InitializeCriticalSection(&global_critical_section_for_big_art);
  InitializeCriticalSection(&global_critical_section_for_catalog);

  if (!InitCardArtGdiResources())
    return 0;

  records = (card_ptr_t *)calloc(2000, sizeof(*records));
  if (!records)
    return 0;

  snprintf(cards_dat_path, sizeof(cards_dat_path), "%s/CARDS.DAT", asset_root);
  cards_file = fopen(cards_dat_path, "rb");
  if (cards_file) {
    if (fread(&card_count, sizeof(card_count), 1, cards_file) != 1 ||
        fread(&unused_header_value, sizeof(unused_header_value), 1, cards_file) != 1) {
      fclose(cards_file);
      free(records);
      return 0;
    }
    if (card_count < 0)
      card_count = 0;
    if (card_count > 2000)
      card_count = 2000;
    if (fread(records, sizeof(records[0]), (size_t)card_count, cards_file) != (size_t)card_count) {
      fclose(cards_file);
      free(records);
      return 0;
    }
    fclose(cards_file);
  }

  for (idx = 0; idx < 2000; ++idx)
    g_cardPicCounts[idx] = idx < card_count && records[idx].num_pics > 0 ? records[idx].num_pics : 1;
  free(records);
  for (i = 0; i < 2000; ++i)
    g_SmallArtCache[i].hbm = 0;
  for (i = 0; i < 100; ++i)
    g_versionedSmallArtCache[i].hbm = 0;
  for (i = 0; i < 0x14; ++i)
    g_versionedBigArtCache[i].hbm = 0;
  g_versionedSmallArtCount = 0;
  g_versionedBigArtCount = 0;
  global_dither_kernel_id = 3;
  return 1;
}

// MATCHING
// FUNCTION: CARDARTLIB 0x1000343e
void CardArtLib_Shutdown(void)
{
  DestroyAllSmallArts();
  DestroyAllBigArts();
  ShutdownCardArtGdiResources();
  DeleteCriticalSection(&global_critical_section_for_small_art);
  DeleteCriticalSection(&global_critical_section_for_big_art);
  DeleteCriticalSection(&global_critical_section_for_catalog);
}

// FUNCTION: CARDARTLIB 0x10003480
int LoadBigArt(card_id_t id, int version, int width, int height)
{
  int align_bytes;      /* -0x158 */
  HDC desktop_hdc;      /* -0x154 */
  char wvl_path[0x108]; /* -0x150 */
  VersionedArtCacheEntry *existing; /* -0x48 */
  HBITMAP bitmap;       /* -0x44 */
  HDC mem_dc;           /* -0x40 */
  int ok;               /* -0x3c */
  WvlEntry *wvl_entry;  /* -0x38 */
  void *dib_bits;       /* -0x34 */
  uint *decoded;        /* -0x30 */
  BITMAPINFO bmi;       /* -0x2c */

  ok = 1;
  if (id == -1)
  {
    return 0;
  }

  EnterCriticalSection(&global_critical_section_for_big_art);

  existing = IsBigArtIn(id, version);
  if (existing != 0)
  {
    if ((existing->width == width) && (existing->height == height))
    {
      LeaveCriticalSection(&global_critical_section_for_big_art);
      return 1;
    }
    else
      DestroyBigArt(id, version);
  }

  if ((g_cardPicCounts[id] > 1) && (version != 0))
  {
    sprintf(wvl_path, "%s\\%04d%c.WVL", &global_cartart_directory, id, (int)(char)(version + '`'));
  }
  else
  {
    sprintf(wvl_path, "%s\\%04d.WVL", &global_cartart_directory, id);
  }

  desktop_hdc = GetDC(GetDesktopWindow());
  global_color_depth = GetDeviceCaps(desktop_hdc, BITSPIXEL) * GetDeviceCaps(desktop_hdc, PLANES);
  ReleaseDC(GetDesktopWindow(), desktop_hdc);

  wvl_entry = Catalog_LoadWvlEntry(1, wvl_path, 0);
  if (wvl_entry != (WvlEntry *)0x0)
  {
    mem_dc = GetDC((HWND)0x0);
    ApplyCardArtPaletteToDc(mem_dc);
    InitBitmapInfo24bppTopDown(&bmi, width, height);
    bitmap = CreateDIBSection(mem_dc, &bmi, 0, &dib_bits, (HANDLE)0x0, 0);
    if (bitmap != (HBITMAP)0x0)
    {
      decoded = Wvl_DecodeToBgr24((byte *)0x0, wvl_entry, width, height);
      if (decoded != (uint *)0x0)
      {
        if ((-(width + width - width) & 3U) != 0)
        {
          align_bytes = 4 - (-(width + width - width) & 3U);
        }
        else
          align_bytes = 0;

        memcpy(dib_bits, decoded, (width * 3 + align_bytes) * height);
      }
      else
      {
        ok = 0;
        DeleteObject(bitmap);
      }
    }
    else
    {
      ok = 0;
    }
    ReleaseDC((HWND)0x0, mem_dc);
    Catalog_Unlock(wvl_entry);
  }
  else
  {
    ok = 0;
  }

  if (ok != 0)
  {
    if ((int)g_versionedBigArtCount >= 0x14)
    {
      DestroyBigArt(g_versionedBigArtCache[0].id, g_versionedBigArtCache[0].version);
    }

    g_versionedBigArtCache[g_versionedBigArtCount].hbm = bitmap;
    g_versionedBigArtCache[g_versionedBigArtCount].bits = dib_bits;
    g_versionedBigArtCache[g_versionedBigArtCount].width = width;
    g_versionedBigArtCache[g_versionedBigArtCount].height = height;
    g_versionedBigArtCache[g_versionedBigArtCount].id = id;
    g_versionedBigArtCache[g_versionedBigArtCount].version = version;
    g_versionedBigArtCount = g_versionedBigArtCount + 1;
  }

  LeaveCriticalSection(&global_critical_section_for_big_art);
  return ok;
}

// FUNCTION: CARDARTLIB 0x100037ba
void *IsBigArtIn(card_id_t id, int version)
{
  VersionedArtCacheEntry *result;
  int i;

  result = NULL;
  if (id == -1)
    return 0;

  EnterCriticalSection(&global_critical_section_for_big_art);

  for (i = 0; i < g_versionedBigArtCount && result == 0; i++)
  {
    if (g_versionedBigArtCache[i].id == id && g_versionedBigArtCache[i].version == version)
    {
      result = &g_versionedBigArtCache[i];
    }
  }

  LeaveCriticalSection(&global_critical_section_for_big_art);

  return result;
}

// MATCHING
// FUNCTION: CARDARTLIB 0x10003867
int IsBigArtRightSize(card_id_t id, int version, int width, int height)
{
  VersionedArtCacheEntry *cache_entry = NULL;
  if (id == -1)
  {
    return 0;
  }

  EnterCriticalSection(&global_critical_section_for_big_art);
  cache_entry = IsBigArtIn(id, version);
  if ((cache_entry != NULL) &&
      ((cache_entry->width != width || cache_entry->height != height)))
  {
    cache_entry = NULL;
  }
  LeaveCriticalSection(&global_critical_section_for_big_art);

  return cache_entry != NULL;
}

// FUNCTION: CARDARTLIB 0x100038ed
int DrawBigArt(HDC hdc, RECT *rect, card_id_t id, int version)
{
  int result;
  int found_index;
  int i;
  int found;

  if (id == -1)
  {
    return 0;
  }

  EnterCriticalSection(&global_critical_section_for_big_art);
  i = 0;
  found = 0;
  for (; i < (int)g_versionedBigArtCount && (found == 0); i++)
  {
    if (g_versionedBigArtCache[i].id == id && g_versionedBigArtCache[i].version == version)
    {
      found = 1;
      found_index = i;
    }
  }

  if (found != 0)
  {
    result = DrawBitmapToRect(hdc, rect, g_versionedBigArtCache[found_index].hbm);
  }
  else
  {
    result = 0;
  }

  if (result == 0)
  {
    FillRect(hdc, rect, GetStockObject(2));
  }

  LeaveCriticalSection(&global_critical_section_for_big_art);
  return result;
}

// MATCHING
// FUNCTION: CARDARTLIB 0x100039ef
int ReloadBigArtIfWrongSize(card_id_t id, int version, int width, int height)
{
  if (id == -1)
  {
    return 0;
  }

  if (IsBigArtRightSize(id, version, width, height) != 0)
  {
    return 1;
  }
  else
  {
    DestroyBigArt(id, version);
  }

  if (LoadBigArt(id, version, width, height) != 0)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// FUNCTION: CARDARTLIB 0x10003a80
void DestroyBigArt(card_id_t id, int version)
{
  int j;
  int i;
  int found;

  if (id == -1)
  {
    return;
  }

  EnterCriticalSection(&global_critical_section_for_big_art);
  i = 0;
  found = 0;

  for (; i < g_versionedBigArtCount && (found == 0); i++)
  {
    if (g_versionedBigArtCache[i].id == id && g_versionedBigArtCache[i].version == version)
    {
      found = 1;

      if (g_versionedBigArtCache[i].hbm != 0)
      {
        DeleteObject(g_versionedBigArtCache[i].hbm);
      }

      g_versionedBigArtCount--;

      j = i;
      for (; j < g_versionedBigArtCount; j++)
      {
        g_versionedBigArtCache[j].hbm = g_versionedBigArtCache[j + 1].hbm;
        g_versionedBigArtCache[j].bits = g_versionedBigArtCache[j + 1].bits;
        g_versionedBigArtCache[j].width = g_versionedBigArtCache[j + 1].width;
        g_versionedBigArtCache[j].height = g_versionedBigArtCache[j + 1].height;
        g_versionedBigArtCache[j].id = g_versionedBigArtCache[j + 1].id;
        g_versionedBigArtCache[j].version = g_versionedBigArtCache[j + 1].version;
      }
    }
  }

  LeaveCriticalSection(&global_critical_section_for_big_art);
}

// FUNCTION: CARDARTLIB 0x10003c0a
void DestroyAllBigArts(void)
{
  int i;
  EnterCriticalSection(&global_critical_section_for_big_art);
  for (i = 0; i < g_versionedBigArtCount; i = i + 1)
  {
    DeleteObject(g_versionedBigArtCache[i].hbm);
  }
  g_versionedBigArtCount = 0;
  LeaveCriticalSection(&global_critical_section_for_big_art);
}

// FUNCTION: CARDARTLIB 0x10003cf0
int LoadSmallArt(card_id_t id, int version, int width, int height)
{
  int row_pad;
  HDC desktop_hdc;
  uint wvl_path[66];
  HBITMAP hbm;
  HDC hdc;
  int ok;
  WvlEntry *wvl_entry;
  void *dib_bits;
  uint *decoded_bgr;
  BITMAPINFO bmi;

  ok = 1;
  if (id == -1)
  {
    return 0;
  }

  if (g_cardPicCounts[id] > 1)
  {
    return LoadVersionedSmallArt(id, version, width, height);
  }

  EnterCriticalSection(&global_critical_section_for_small_art);

  if (g_SmallArtCache[id].hbm != (HBITMAP)0)
  {
    if (g_SmallArtCache[id].width == width && g_SmallArtCache[id].height == height)
    {
      LeaveCriticalSection(&global_critical_section_for_small_art);
      return 1;
    }
    else
      DestroySmallArt(id, 0);
  }

  desktop_hdc = GetDC(GetDesktopWindow());
  global_color_depth = GetDeviceCaps(desktop_hdc, BITSPIXEL) * GetDeviceCaps(desktop_hdc, PLANES);
  ReleaseDC(GetDesktopWindow(), desktop_hdc);

  sprintf((char *)wvl_path, "%s\\%04d.WVL", &global_cartart_directory, id);
  wvl_entry = Catalog_LoadWvlEntry(0, (char *)wvl_path, 0);
  if (wvl_entry != (WvlEntry *)0x0)
  {
    hdc = GetDC((HWND)0x0);
    ApplyCardArtPaletteToDc(hdc);
    InitBitmapInfo24bppTopDown(&bmi, width, height);
    hbm = CreateDIBSection(hdc, &bmi, 0, &dib_bits, (HANDLE)0x0, 0);
    if (hbm != (HBITMAP)0x0)
    {
      decoded_bgr = Wvl_DecodeToBgr24((byte *)0x0, wvl_entry, width, height);
      if (decoded_bgr != (uint *)0x0)
      {
        do
        {
          if ((-(width + width - width) & 3) != 0)
          {
            row_pad = 4 - (-(width + width - width) & 3);
            break;
          }
          row_pad = 0;
        } while (0);
        memcpy(dib_bits, decoded_bgr, (width * 3 + row_pad) * height);
      }
      else
      {
        ok = 0;
        DeleteObject(hbm);
      }
    }
    else
    {
      ok = 0;
    }
    ReleaseDC((HWND)0x0, hdc);
    Catalog_Unlock(wvl_entry);
  }
  else
  {
    ok = 0;
  }

  if (ok != 0)
  {
    g_SmallArtCache[id].hbm = hbm;
    g_SmallArtCache[id].bits = dib_bits;
    g_SmallArtCache[id].width = width;
    g_SmallArtCache[id].height = height;
  }

  LeaveCriticalSection(&global_critical_section_for_small_art);
  return ok;
}

// MATCHING
// FUNCTION: CARDARTLIB 0x10003fb4
int IsSmallArtIn(card_id_t id, int version)
{
  if (id == -1)
  {
    return 0;
  }

  if (g_cardPicCounts[id] > 1)
  {
    if (FindVersionedSmallArtCacheEntry(id, version) != (VersionedArtCacheEntry *)0x0)
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }
  else
  {
    if (g_SmallArtCache[id].hbm != 0)
    {
      return 1;
    }
    else
    {
      return 0;
    }
  }
}

// MATCHING
// FUNCTION: CARDARTLIB 0x1000403d
int DrawSmallArt(HDC hdc, RECT *rect, card_id_t id, int version)
{
  int result;
  if ((hdc == (HDC)0x0) || (rect == (RECT *)0x0))
  {
    return 0;
  }

  if (id == -1)
  {
    return 0;
  }

  if (g_cardPicCounts[id] > 1)
  {
    return DrawVersionedSmallArt(hdc, rect, id, version);
  }
  else
  {
    EnterCriticalSection(&global_critical_section_for_small_art);
    if (IsSmallArtIn(id, version) != 0)
    {
      result = DrawBitmapToRect(hdc, rect, g_SmallArtCache[id].hbm);
    }
    else
    {
      result = 0;
    }
    if (result == 0)
    {
      FillRect(hdc, rect, GetStockObject(2));
    }
    LeaveCriticalSection(&global_critical_section_for_small_art);

    return result;
  }
}

// MATCHING
// FUNCTION: CARDARTLIB 0x10004128
bool ReloadSmallArtIfWrongSize(card_id_t id, int version, int width, int height)
{
  int result;
  HGDIOBJ old;

  if (id == -1)
  {
    return 0;
  }

  if (g_cardPicCounts[id] > 1)
  {
    return ReloadVersionedSmallArtIfWrongSize(id, version, width, height);
  }

  EnterCriticalSection(&global_critical_section_for_small_art);

  if (g_SmallArtCache[id].hbm != (HGDIOBJ)0x0 && g_SmallArtCache[id].width == width && g_SmallArtCache[id].height == height)
  {
    result = 1;
  }
  else
  {
    old = g_SmallArtCache[id].hbm;
    g_SmallArtCache[id].hbm = (HGDIOBJ)0x0;
    if (LoadSmallArt(id, version, width, height) != 0)
    {
      if (old != (HGDIOBJ)0x0)
      {
        DeleteObject(old);
      }
      result = 1;
    }
    else
    {
      g_SmallArtCache[id].hbm = old;
      result = 0;
    }
  }

  LeaveCriticalSection(&global_critical_section_for_small_art);
  return result;
}

// FUNCTION: CARDARTLIB 0x10004251
void DestroySmallArt(card_id_t id, int version)
{
  if (id == -1)
    return;

  if ((g_cardPicCounts)[id] > 1)
  {
    DestroyVersionedSmallArt(id, version);
  }
  else
  {
    EnterCriticalSection(&global_critical_section_for_small_art);
    if (g_SmallArtCache[id].hbm != 0)
    {
      DeleteObject(g_SmallArtCache[id].hbm);
      g_SmallArtCache[id].hbm = 0;
    }
    LeaveCriticalSection(&global_critical_section_for_small_art);
  }
}

// FUNCTION: CARDARTLIB 0x100042dd
void DestroyAllSmallArts(void)
{
  int i;

  EnterCriticalSection(&global_critical_section_for_small_art);
  DestroyAllVersionedSmallArts();
  for (i = 0; i < 2000; i = i + 1)
  {
    if (g_cardPicCounts[i] <= 1)
    {
      DestroySmallArt(i, 0);
    }
  }
  LeaveCriticalSection(&global_critical_section_for_small_art);
}

// FUNCTION: CARDARTLIB 0x10004346
int LoadVersionedSmallArt(card_id_t id, int version, int width, int height)
{
  int row_pad;
  uint wvl_path[66];
  HBITMAP hbm;
  HDC hdc;
  int ok;
  WvlEntry *wvl_entry;
  void *dib_bits;
  uint *decoded_bgr;
  BITMAPINFO bmi;
  VersionedArtCacheEntry *cache_entry;

  ok = 1;
  if (id == -1)
  {
    return 0;
  }

  EnterCriticalSection(&global_critical_section_for_small_art);

  cache_entry = (VersionedArtCacheEntry *)FindVersionedSmallArtCacheEntry(id, version);
  if (cache_entry != (VersionedArtCacheEntry *)0x0)
  {
    if ((cache_entry->width == width) && (cache_entry->height == height))
    {
      LeaveCriticalSection(&global_critical_section_for_small_art);
      return 1;
    }
    else
      DestroyVersionedSmallArt(id, version);
  }

  if (version != 0)
  {
    sprintf((char *)wvl_path, "%s\\%04d%c.WVL", &global_cartart_directory, id, (int)(char)(version + '`'));
  }
  else
  {
    sprintf((char *)wvl_path, "%s\\%04d.WVL", &global_cartart_directory, id);
  }

  wvl_entry = Catalog_LoadWvlEntry(0, (char *)wvl_path, 0);
  if (wvl_entry != (WvlEntry *)0x0)
  {
    hdc = GetDC((HWND)0x0);
    ApplyCardArtPaletteToDc(hdc);
    InitBitmapInfo24bppTopDown(&bmi, width, height);
    hbm = CreateDIBSection(hdc, &bmi, 0, &dib_bits, (HANDLE)0x0, 0);
    if (hbm != (HBITMAP)0x0)
    {
      decoded_bgr = Wvl_DecodeToBgr24((byte *)0x0, wvl_entry, width, height);
      if (decoded_bgr != (uint *)0x0)
      {
        if ((-(width + width - width) & 3) != 0)
        {
          row_pad = 4 - (-(width + width - width) & 3);
        }
        else
        {
          row_pad = 0;
        }
        memcpy(dib_bits, decoded_bgr, (width * 3 + row_pad) * height);
      }
      else
      {
        ok = 0;
        DeleteObject(hbm);
      }
    }
    else
    {
      ok = 0;
    }
    ReleaseDC((HWND)0x0, hdc);
    Catalog_Unlock(wvl_entry);
  }
  else
  {
    ok = 0;
  }

  if (ok != 0)
  {
    g_versionedSmallArtCache[g_versionedSmallArtCount].hbm = hbm;
    g_versionedSmallArtCache[g_versionedSmallArtCount].bits = dib_bits;
    g_versionedSmallArtCache[g_versionedSmallArtCount].width = width;
    g_versionedSmallArtCache[g_versionedSmallArtCount].height = height;
    g_versionedSmallArtCache[g_versionedSmallArtCount].id = id;
    g_versionedSmallArtCache[g_versionedSmallArtCount].version = version;
    g_versionedSmallArtCount++;
  }

  LeaveCriticalSection(&global_critical_section_for_small_art);
  return ok;
}

// FUNCTION: CARDARTLIB 0x100045fe
VersionedArtCacheEntry *FindVersionedSmallArtCacheEntry(card_id_t id, int version)
{
  int i;
  VersionedArtCacheEntry *ptr;

  ptr = 0;
  if (id == -1)
    return 0;

  EnterCriticalSection(&global_critical_section_for_small_art);

  for (i = 0; i < (int)g_versionedSmallArtCount && ptr == 0; i++)
  {
    if (g_versionedSmallArtCache[i].id == id && g_versionedSmallArtCache[i].version == version)
    {
      ptr = &g_versionedSmallArtCache[i];
    }
  }

  LeaveCriticalSection(&global_critical_section_for_small_art);

  return ptr;
}

// FUNCTION: CARDARTLIB 0x100046aa
int DrawVersionedSmallArt(HDC hdc, RECT *rect, card_id_t id, int version)
{
  int result;
  int found_index;
  int i;
  bool bVar1;

  if (id == -1)
    return 0;

  EnterCriticalSection(&global_critical_section_for_small_art);

  for (i = 0, bVar1 = false; i < g_versionedSmallArtCount && (!bVar1); i++)
  {
    if (g_versionedSmallArtCache[i].id == id && g_versionedSmallArtCache[i].version == version)
    {
      bVar1 = true;
      found_index = i;
    }
  }

  if (bVar1)
  {
    result = DrawBitmapToRect(hdc, rect, g_versionedSmallArtCache[found_index].hbm);
  }
  else
  {
    result = 0;
  }
  if (result == 0)
  {
    FillRect(hdc, rect, GetStockObject(2));
  }
  LeaveCriticalSection(&global_critical_section_for_small_art);

  return result;
}

// MATCHING
// FUNCTION: CARDARTLIB 0x100047ab
bool ReloadVersionedSmallArtIfWrongSize(card_id_t id, int version, int width, int height)
{
  VersionedArtCacheEntry *cache_entry;

  if (id == -1)
  {
    return 0;
  }

  cache_entry = FindVersionedSmallArtCacheEntry(id, version);
  if (cache_entry != (VersionedArtCacheEntry *)0x0)
  {
    if ((cache_entry->width == width) && (cache_entry->height == height))
    {
      return 1;
    }
    else
    {
      DestroyVersionedSmallArt(id, version);
    }
  }

  if (LoadVersionedSmallArt(id, version, width, height) != 0)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// FUNCTION: CARDARTLIB 0x1000485a
void DestroyVersionedSmallArt(card_id_t id, int version)
{
  int j;
  int i;
  bool found;

  if (id == -1)
  {
    return;
  }

  EnterCriticalSection(&global_critical_section_for_small_art);

  for (i = 0, found = 0; i < g_versionedSmallArtCount && found == 0; i++)
  {
    if (g_versionedSmallArtCache[i].id == id && g_versionedSmallArtCache[i].version == version)
    {
      found = 1;

      if (g_versionedSmallArtCache[i].hbm != 0)
      {
        DeleteObject(g_versionedSmallArtCache[i].hbm);
      }

      g_versionedSmallArtCount = g_versionedSmallArtCount + -1;

      for (j = i; j < g_versionedSmallArtCount; j++)
      {
        g_versionedSmallArtCache[j].hbm = g_versionedSmallArtCache[j + 1].hbm;
        g_versionedSmallArtCache[j].bits = g_versionedSmallArtCache[j + 1].bits;
        g_versionedSmallArtCache[j].width = g_versionedSmallArtCache[j + 1].width;
        g_versionedSmallArtCache[j].height = g_versionedSmallArtCache[j + 1].height;
        g_versionedSmallArtCache[j].id = g_versionedSmallArtCache[j + 1].id;
        g_versionedSmallArtCache[j].version = g_versionedSmallArtCache[j + 1].version;
      }
    }
  }

  LeaveCriticalSection(&global_critical_section_for_small_art);
}

// FUNCTION: CARDARTLIB 0x100049e4
void DestroyAllVersionedSmallArts(void)
{
  int i;

  EnterCriticalSection(&global_critical_section_for_small_art);
  for (i = 0; i < g_versionedSmallArtCount; i = i + 1)
  {
    DeleteObject(g_versionedSmallArtCache[i].hbm);
  }
  g_versionedSmallArtCount = 0;
  LeaveCriticalSection(&global_critical_section_for_small_art);
}
