#include <stdlib.h>
#include <STDDEF.H>
#include <string.h>
#include "platform/gdi_compat.h"
#include "haar.h"
#include "catalog.h"
#include "assert.h"
#include "huffman.h"

// Forward declarations for globals referenced elsewhere.
int Palette_FindNearestEntryIndex(int red, int green, int blue, byte *palette);
uint Rgb888_QuantizeToF8(uint rgb);
void RotateDwordsLeft1(undefined4 *dwords, uint count);
undefined4 QuantizeBgr24ToNearestPaletteColorInPlace(uint *bgr24, int height, int width, int row_padding);
int DitherBgr24ToPaletteColors(int dither_kernel_id, int serpentine, uint *bgr24, int height, int width,
                               int row_padding);
int InitErrorDiffusionDeltaTables(int dither_kernel_id, int *delta_table_ptrs_base);
int Octree_FindNearestColor(uint rgb);
void MemZeroDwords(undefined8 *dst, uint dword_count);

void CopyBgr24RectIntoStridedBuffer(byte *dst_bgr24, byte *src_bgr24, int dst_x, int dst_y,
                                    int rect_width, int rect_height, int dst_stride_pixels, int unused);
void CopyBytes(void *dst, const void *src, size_t size);
void SetBytes(void *dst, int value, size_t num);
void Haar2D_ReconstructInPlace(int *coeffs, int full_size, int base_size);
void Haar_CombineSumDiff(int *src_a, int *src_b, int *dst, int width, int rows, undefined4 src_stride_unused,
                         int dst_stride);
void Haar_CombineSumDiffHalf(int *src_a, int *src_b, int *dst, int width, int rows,
                             undefined4 src_stride_unused, int dst_stride);
undefined1 *YuvPlanesToBgr24(undefined1 *out_bgr24, int *luma, int width, int height, int *chroma_u, int *chroma_v,
                             int chroma_stride, undefined4 unused_chroma_height, int chroma_is_420);
BOOL Wvl_UnpackPieces(byte *dst, WvlEntry *wvl);

// GLOBAL: CARDARTLIB 0x100ea098
// GLOBAL: DRAWCARDLIB 0x100f1f68
undefined4 g_activeWvlCatalogHandle;

// GLOBAL: CARDARTLIB 0x1001d258
// GLOBAL: DRAWCARDLIB 0x10021048
undefined4 global_dither_kernel_id = 0x00000000;

// GLOBAL: CARDARTLIB 0x1001d25c
// GLOBAL: DRAWCARDLIB 0x1002104c
// GLOBAL: SHANDALAR 0x00581ebc
undefined4 global_serpentine = 0x00000000;

// GLOBAL: CARDARTLIB 0x1001d260
// GLOBAL: DRAWCARDLIB 0x10021050
// GLOBAL: SHANDALAR 0x00581ec0
int g_ditherKernelTapCounts[0xa] = {0, 0, 2, 4, 12, 7, 10, 12, 12, 0};

// GLOBAL: CARDARTLIB 0x1001d288
// GLOBAL: DRAWCARDLIB 0x10021078
// GLOBAL: SHANDALAR 0x00581ee8
int g_ditherKernelErrorRowCounts[8] = {0, 0, 2, 2, 3, 2, 3, 3};

// GLOBAL: CARDARTLIB 0x1001d2b0
// GLOBAL: DRAWCARDLIB 0x100210a0
// GLOBAL: SHANDALAR 0x00581f10
int g_ditherKernelDivisors[0xa] = {0, 0, 16, 16, 42, 32, 32, 48, 236, 0};

// GLOBAL: CARDARTLIB 0x1001d2d8
// GLOBAL: DRAWCARDLIB 0x100210c8
// GLOBAL: SHANDALAR 0x00581f38
undefined1 g_ditherKernelTable[0xd80] = {
#include "g_ditherKernelTable.inc"
};

// GLOBAL: CARDARTLIB 0x10031aa0
// GLOBAL: DRAWCARDLIB 0x10028af0
// GLOBAL: SHANDALAR 0x005a82f0
undefined1 g_u8ClampTableStorage[0x400];

// GLOBAL: CARDARTLIB 0x1001e058
// GLOBAL: DRAWCARDLIB 0x10021e48
// GLOBAL: SHANDALAR 0x00582cb8
undefined1 *g_u8ClampTable = g_u8ClampTableStorage + 0x200;

// GLOBAL: CARDARTLIB 0x1001e05c
// GLOBAL: DRAWCARDLIB 0x10021e4c
// GLOBAL: SHANDALAR 0x00582cbc
undefined4 g_paletteDitherKernelCachedId = 0xFFFFFFFF;

// GLOBAL: CARDARTLIB 0x1001e060
// GLOBAL: DRAWCARDLIB 0x10021e50
// GLOBAL: SHANDALAR 0x00582cc0
undefined4 g_rgbQuantKernelCachedId = 0xFFFFFFFF;

// GLOBAL: CARDARTLIB 0x1001e0e0
// GLOBAL: DRAWCARDLIB 0x100223c0
// GLOBAL: SHANDALAR 0x00593cd0
int g_rowAlignBytes = 0x00000004;

// GLOBAL: CARDARTLIB 0x100322d8
// GLOBAL: DRAWCARDLIB 0x1003a1a8
unsigned char g_waveletScaleTableStorage[0x801];

// GLOBAL: CARDARTLIB 0x10032adc
// GLOBAL: DRAWCARDLIB 0x1003a9ac
undefined4 g_wvlCatalogsInitialized;

// GLOBAL: CARDARTLIB 0x1001e11c
// GLOBAL: DRAWCARDLIB 0x100223fc
undefined4 g_smallArtCatalogHandle = 0x00000001;

// GLOBAL: CARDARTLIB 0x1001e120
// GLOBAL: DRAWCARDLIB 0x10022400
undefined4 g_medArtCatalogHandle = 0x00000002;

// GLOBAL: CARDARTLIB 0x10032ae8
// GLOBAL: DRAWCARDLIB 0x1003a9b8
WvlEntry g_wvlEntryScratch;

// GLOBAL: CARDARTLIB 0x1001e118
// GLOBAL: DRAWCARDLIB 0x100223f8
// GLOBAL: SHANDALAR 0x00593d08
unsigned char *g_waveletScaleToByteTable = g_waveletScaleTableStorage + 0x400;

// GLOBAL: CARDARTLIB 0x1001e124
// GLOBAL: DRAWCARDLIB 0x10022404
// GLOBAL: SHANDALAR 0x00593d14
undefined4 g_haarScratchInit = 0x00000000;

// GLOBAL: CARDARTLIB 0x100eaB00
// GLOBAL: DRAWCARDLIB 0x10124550
undefined1 g_yuvClampTableStorage[0x2000];

// GLOBAL: CARDARTLIB 0x1001e128
// GLOBAL: DRAWCARDLIB 0x10022408
// GLOBAL: SHANDALAR 0x00593d18
undefined1 *g_yuvClampTable = g_yuvClampTableStorage + 0x400;

// GLOBAL: CARDARTLIB 0x10032ae0
// GLOBAL: DRAWCARDLIB 0x1003a9b0
// GLOBAL: SHANDALAR 0x0067abc8
undefined4 g_waveletScaleToByteTableInit;

// GLOBAL: CARDARTLIB 0x100ea09c
// GLOBAL: DRAWCARDLIB 0x100f1f6c
// GLOBAL: SHANDALAR 0x00732184
undefined4 g_yuvClampTableInit;

#ifdef MODERN_FIXES
// Scratch space needed is dependant on screen size, allocate some bigass buffers to support 4k
undefined1 g_haarDecodeScratch[0x7a800 * 10];
unsigned char g_catalogReadScratch[0x3CC00 * 10];
#else
// GLOBAL: CARDARTLIB 0x10032c98
// GLOBAL: DRAWCARDLIB 0x1003ab68
// GLOBAL: SHANDALAR 0x0067ad80
undefined1 g_haarDecodeScratch[0x7a800];

// GLOBAL: CARDARTLIB 0x100ad498
// GLOBAL: DRAWCARDLIB 0x100b5368
// GLOBAL: SHANDALAR 0x006f5580
unsigned char g_catalogReadScratch[0x3CC00];
#endif

// GLOBAL: CARDARTLIB 0x100edb10
// GLOBAL: DRAWCARDLIB 0x10128e40
// GLOBAL: SHANDALAR 0x00952bf0
undefined1 g_errorDiffusionRowStorage[5 * 0x8060];

// GLOBAL: CARDARTLIB 0x100ecb04
// GLOBAL: DRAWCARDLIB 0x10126554
// GLOBAL: SHANDALAR 0x0073e7f4
BOOL g_unpackResultUnused;

// GLOBAL: CARDARTLIB 0x100322cc
// GLOBAL: DRAWCARDLIB 0x1002931c
// GLOBAL: SHANDALAR 0x005a8b1c
undefined4 g_rgbQuantClampInit;

// GLOBAL: CARDARTLIB 0x100322d0
// GLOBAL: DRAWCARDLIB 0x10029320
// GLOBAL: SHANDALAR 0x005a8b20
undefined4 g_paletteClampInit;

// GLOBAL: CARDARTLIB 0x100ea31c
// GLOBAL: DRAWCARDLIB 0x10124540
// GLOBAL: SHANDALAR 0x0073c7e8
int *g_haarScratchB;

// GLOBAL: CARDARTLIB 0x100ea320
// GLOBAL: DRAWCARDLIB 0x10124544
// GLOBAL: SHANDALAR 0x0073c7ec
int *g_haarScratchA;

// GLOBAL: CARDARTLIB 0x10117100
// GLOBAL: DRAWCARDLIB 0x10152430
// GLOBAL: SHANDALAR 0x0097c1e0
void *g_errorDiffusionDeltaTablePtrs[0x41];

// GLOBAL: CARDARTLIB 0x101221e8
// GLOBAL: DRAWCARDLIB 0x10124538
undefined4 global_color_depth;

// FUNCTION: CARDARTLIB 0x1000570f
// FUNCTION: DRAWCARDLIB 0x10001cbf
// FUNCTION: SHANDALAR 0x0042ec23
int DitherBgr24ToPaletteColors(int dither_kernel_id, int serpentine, uint *bgr24, int height, int width, int row_padding)
{
  short *err_cell;       /* [ebp-0x94] (also reused as temp) */
  int *delta_table;      /* [ebp-0x90] */
  short *err_px;         /* [ebp-0x8c] */
  int err_r;             /* [ebp-0x88] */
  int x_byte_step;       /* [ebp-0x84] */
  int nearest_rgb;       /* [ebp-0x80] */
  int x_start;           /* [ebp-0x7c] */
  undefined *kernel_ptr; /* [ebp-0x78] */
  int kernel_count;      /* [ebp-0x74] */
  int r_clamped;         /* [ebp-0x70] */
  int *err_row_ptrs[6];  /* [ebp-0x6c] */
  int y;                 /* [ebp-0x54] */
  undefined2 pad_50;     /* [ebp-0x50] */
  undefined2 pad_4e;     /* [ebp-0x4e] */
  undefined2 pad_4c_w;   /* [ebp-0x4c] */
  undefined2 pad_4a;     /* [ebp-0x4a] */
  int x;                 /* [ebp-0x48] */
  int k_row;             /* [ebp-0x44] */
  int k_x_off;           /* [ebp-0x40] */
  int err_b;             /* [ebp-0x3c] */
  int x_step;            /* [ebp-0x38] */
  uint g_clamped;        /* [ebp-0x34] */
  int err_g;             /* [ebp-0x30] */
  int *k_it;             /* [ebp-0x2c] */
  int rgb_clamped;       /* [ebp-0x28] */
  uint src_rgb;          /* [ebp-0x24] */
  undefined2 pad_20;     /* [ebp-0x20] */
  undefined2 pad_1e;     /* [ebp-0x1e] */
  undefined2 pad_1c;     /* [ebp-0x1c] */
  undefined2 pad_1a;     /* [ebp-0x1a] */
  int x_end;             /* [ebp-0x18] */
  uint clear_dwords;     /* [ebp-0x14] */
  undefined2 pad_10;     /* [ebp-0x10] */
  undefined2 pad_e;      /* [ebp-0xe] */
  undefined2 pad_c_w;    /* [ebp-0xc] */
  undefined2 pad_a;      /* [ebp-0xa] */
  int x_byte_off;        /* [ebp-0x8] */
  uint b_clamped;        /* [ebp-0x4] */

  pad_10 = 0;
  pad_e = 0;
  pad_c_w = 0;
  pad_a = 0;
  pad_50 = 0;
  pad_4e = 0;
  pad_4c_w = 0;
  pad_4a = 0;
  pad_20 = 0xffff;
  pad_1e = 0xffff;
  pad_1c = 0xffff;
  pad_1a = 0;
  nearest_rgb = -1;
  x_step = 1;
  rgb_clamped = 0;
  kernel_ptr = &g_ditherKernelTable[(dither_kernel_id << 6) * 3];
  clear_dwords = (uint)((width + 10) * 8) >> 2;

  if (dither_kernel_id == 0)
  {
    g_paletteDitherKernelCachedId = dither_kernel_id;
    return 0;
  }
  else if (dither_kernel_id == 1)
  {
    g_paletteDitherKernelCachedId = dither_kernel_id;
    return QuantizeBgr24ToNearestPaletteColorInPlace(bgr24, height, width, row_padding);
  }

  if (g_paletteClampInit == 0)
  {
    for (x = -0x200; x < 0x200; x = x + 1)
    {
      if ((0 <= x) && (x <= 0xff))
      {
        g_u8ClampTable[x] = (undefined1)x;
      }
      else
      {
        if (x < 0)
        {
          g_u8ClampTable[x] = 0;
        }
        else
        {
          g_u8ClampTable[x] = 0xff;
        }
      }
    }
    g_paletteClampInit = 1;
  }

  if ((int)g_paletteDitherKernelCachedId != dither_kernel_id)
  {
    for (x = 0; (uint)x < 0x41; x = x + 1)
    {
      if (g_errorDiffusionDeltaTablePtrs[x] != (void *)0x0)
      {
        free(g_errorDiffusionDeltaTablePtrs[x]);
        g_errorDiffusionDeltaTablePtrs[x] = (void *)0x0;
      }
    }
    InitErrorDiffusionDeltaTables(dither_kernel_id, (int *)g_errorDiffusionDeltaTablePtrs);
    g_paletteDitherKernelCachedId = dither_kernel_id;
  }

  for (x = 0; (uint)x < 5; x = x + 1)
  {
    SetBytes((void *)(g_errorDiffusionRowStorage + x * 0x8060), 0, 0x8060);
    err_row_ptrs[x + 1] = (int *)((int)(g_errorDiffusionRowStorage + x * 0x8060) + 0x28);
  }

  kernel_count = g_ditherKernelTapCounts[dither_kernel_id];
  for (y = 0; y < height; y = y + 1, *(int *)&bgr24 += width * 3 + row_padding)
  {
    if (0 < x_step)
    {
      x_start = 0;
      x_end = width;
      x_byte_step = 3;
    }
    else
    {
      x_start = width + -1;
      x_end = -1;
      x_byte_step = -3;
    }

    x = x_start;
    x_byte_off = x_start + x_start + x_start;
    for (; x != x_end; x = x + x_step, x_byte_off = x_byte_off + x_byte_step)
    {
      src_rgb = *(uint *)(x_byte_off + (ptrdiff_t)bgr24) & 0xffffff;
      *(uint *)&((byte *)bgr24)[x_byte_off] = *(uint *)(x_byte_off + (ptrdiff_t)bgr24) & 0xff000000;

      err_px = (short *)err_row_ptrs[1] + (x << 2);

      if (src_rgb == 0)
      {
        nearest_rgb = 0;
        r_clamped = nearest_rgb;
        g_clamped = r_clamped;
        b_clamped = g_clamped;
      }
      else if (src_rgb == 0xffffff)
      {
        r_clamped = 0xff;
        g_clamped = r_clamped;
        b_clamped = g_clamped;
        nearest_rgb = 0xffffff;
      }
      else
      {

        b_clamped = src_rgb & 0xff;
        b_clamped += ((int)*err_px >> 8);
        b_clamped = (uint)(byte)g_u8ClampTable[b_clamped];

        g_clamped = ((byte *)&src_rgb)[1];
        g_clamped += ((int)err_px[1] >> 8);
        g_clamped = (uint)(byte)g_u8ClampTable[g_clamped];

        r_clamped = (src_rgb & 0xff0000) >> 16;
        r_clamped += ((int)err_px[2] >> 8);
        r_clamped = (uint)(byte)g_u8ClampTable[r_clamped];

        src_rgb = g_clamped << 8 | r_clamped << 0x10 | b_clamped;

        nearest_rgb = Octree_FindNearestColor(src_rgb);
      }
      *(uint *)&((byte *)bgr24)[x_byte_off] = *(uint *)(x_byte_off + (ptrdiff_t)bgr24) | nearest_rgb;

      err_b = b_clamped - (nearest_rgb & 0xff);
      err_g = g_clamped - (nearest_rgb >> 8 & 0xff);
      err_r = r_clamped - ((uint)nearest_rgb >> 16);

      k_it = (int *)kernel_ptr;
      err_row_ptrs[0] = &k_it[kernel_count * 4];
      for (; err_row_ptrs[0] > k_it; k_it += 0x4)
      {
        k_x_off = k_it[1];
        k_row = k_it[2];
        delta_table = (int *)k_it[3];
        err_cell = ((k_x_off + x) << 2) + (short *)(err_row_ptrs[k_row + 1]);

        err_cell[0] = delta_table[err_b] + err_cell[0];
        err_cell[1] = delta_table[err_g] + err_cell[1];
        err_cell[2] = delta_table[err_r] + err_cell[2];
      }
    }

    RotateDwordsLeft1((undefined4 *)(err_row_ptrs + 1), g_ditherKernelErrorRowCounts[dither_kernel_id]);
    memset((void *)(err_row_ptrs[g_ditherKernelErrorRowCounts[dither_kernel_id]] + -10), 0, clear_dwords << 2);
    if (serpentine != 0)
    {
      x_step = -x_step;
      kernel_ptr = (undefined *)((int)g_ditherKernelTable + (uint)(x_step == -1) * 0x6c0 + dither_kernel_id * 0xc0);
    }
  }
}

// FUNCTION: CARDARTLIB 0x10005cb4
// FUNCTION: DRAWCARDLIB 0x10002264
// FUNCTION: MAGIC 0x004bae9a
// FUNCTION: SHANDALAR 0x0042f1c7
void RotateDwordsLeft1(undefined4 *dwords, uint count)
{
  undefined4 uVar1;
  uVar1 = *dwords;

  memcpy(dwords, dwords + 1, (count - 1) * 4);
  dwords[count - 1] = uVar1;
}

// FUNCTION: CARDARTLIB 0x10005cf5
// FUNCTION: DRAWCARDLIB 0x100022a5
// FUNCTION: SHANDALAR 0x0042f208
int InitErrorDiffusionDeltaTables(int dither_kernel_id, int *delta_table_ptrs_base)
{
  int delta;
  int tap_index;    // EBP - 0xc
  int tap_weight;   // EBP - 8
  int half_divisor; // EBP - 4

  half_divisor = g_ditherKernelDivisors[dither_kernel_id] >> 1;
  for (tap_index = 0; tap_index < g_ditherKernelTapCounts[dither_kernel_id]; tap_index = tap_index + 1)
  {
    tap_weight = *(int *)&g_ditherKernelTable[(tap_index << 4) + ((dither_kernel_id << 6) * 3)];

    if (delta_table_ptrs_base[tap_weight] != 0)
    {
      *(int *)&g_ditherKernelTable[0xc + ((dither_kernel_id << 6) * 3) + (tap_index << 4)] = delta_table_ptrs_base[tap_weight] + 0x400;
      continue;
    }

    delta_table_ptrs_base[tap_weight] = (int)malloc(0x800);
    assert((uint)(delta_table_ptrs_base[tap_weight] != 0), "D:\\Newmagic\\sources\\NedCard\\Palette.c",
           0x4fd, "Not enough memory for delta array\r\n");
    for (delta = -0x100; delta < 0x100; delta = delta + 1)
    {
      *(int *)(delta_table_ptrs_base[tap_weight] + 0x400 + delta * 4) =
          ((tap_weight * delta + half_divisor) * 0x100) / g_ditherKernelDivisors[dither_kernel_id];
    }
    *(int *)&g_ditherKernelTable[0xc + ((dither_kernel_id << 6) * 3) + (tap_index << 4)] = delta_table_ptrs_base[tap_weight] + 0x3fc;
  }

  for (tap_index = 0; tap_index < g_ditherKernelTapCounts[dither_kernel_id]; tap_index = tap_index + 1)
  {
    tap_weight = *(int *)&g_ditherKernelTable[0xc + 0x6b4 + ((dither_kernel_id << 6) * 3) + (tap_index << 4)];

    *(int *)&g_ditherKernelTable[0xc + 0x6b4 + 0xc + ((dither_kernel_id << 6) * 3) + (tap_index << 4)] = delta_table_ptrs_base[tap_weight] + 0x3fc;
  }
  return 0;
}

// FUNCTION: CARDARTLIB 0x100062c7
// FUNCTION: DRAWCARDLIB 0x10002877
// FUNCTION: SHANDALAR 0x0042f7dc
undefined4 DitherBgr24ToRgbQuantizedF8(int dither_kernel_id, int serpentine, uint *bgr24, int height, int width, int row_padding)
{
  short *err_cell;       /* [ebp-0x94] (also reused as temp) */
  int *delta_table;      /* [ebp-0x90] */
  short *err_px;         /* [ebp-0x8c] */
  int err_r;             /* [ebp-0x88] */
  int x_byte_step;       /* [ebp-0x84] */
  int quant_rgb;         /* [ebp-0x80] */
  int x_start;           /* [ebp-0x7c] */
  undefined *kernel_ptr; /* [ebp-0x78] */
  int kernel_count;      /* [ebp-0x74] */
  int r_clamped;         /* [ebp-0x70] */
  int err_row_ptrs[6];   /* [ebp-0x6c] */
  int y;                 /* [ebp-0x54] */
  undefined2 pad_50;     /* [ebp-0x50] */
  undefined2 pad_4e;     /* [ebp-0x4e] */
  undefined2 pad_4c_w;   /* [ebp-0x4c] */
  undefined2 pad_4a;     /* [ebp-0x4a] */
  int x;                 /* [ebp-0x48] */
  int k_row;             /* [ebp-0x44] */
  int k_x_off;           /* [ebp-0x40] */
  int err_b;             /* [ebp-0x3c] */
  int x_step;            /* [ebp-0x38] */
  uint g_clamped;        /* [ebp-0x34] */
  int err_g;             /* [ebp-0x30] */
  int *k_it;             /* [ebp-0x2c] */
  int rgb_clamped;       /* [ebp-0x28] */
  uint src_rgb;          /* [ebp-0x24] */
  undefined2 pad_20;     /* [ebp-0x20] */
  undefined2 pad_1e;     /* [ebp-0x1e] */
  undefined2 pad_1c;     /* [ebp-0x1c] */
  undefined2 pad_1a;     /* [ebp-0x1a] */
  int x_end;             /* [ebp-0x18] */
  uint clear_dwords;     /* [ebp-0x14] */
  undefined2 pad_10;     /* [ebp-0x10] */
  undefined2 pad_e;      /* [ebp-0xe] */
  undefined2 pad_c_w;    /* [ebp-0xc] */
  undefined2 pad_a;      /* [ebp-0xa] */
  int x_byte_off;        /* [ebp-0x8] */
  uint b_clamped;        /* [ebp-0x4] */

  pad_10 = 0;
  pad_e = 0;
  pad_c_w = 0;
  pad_a = 0;
  pad_50 = 0;
  pad_4e = 0;
  pad_4c_w = 0;
  pad_4a = 0;
  pad_20 = 0xffff;
  pad_1e = 0xffff;
  pad_1c = 0xffff;
  pad_1a = 0;
  quant_rgb = -1;
  x_step = 1;
  rgb_clamped = 0;
  kernel_ptr = &g_ditherKernelTable[(dither_kernel_id << 6) * 3];
  clear_dwords = (uint)((width + 10) * 8) >> 2;

  if (g_rgbQuantClampInit == 0)
  {
    for (x = -0x200; x < 0x200; x = x + 1)
    {
      if ((0 <= x) && (x <= 0xff))
      {
        g_u8ClampTable[x] = (undefined1)x;
      }
      else
      {
        if (x < 0)
        {
          g_u8ClampTable[x] = 0;
        }
        else
        {
          g_u8ClampTable[x] = 0xff;
        }
      }
    }
    g_rgbQuantClampInit = 1;
  }

  if ((int)g_rgbQuantKernelCachedId != dither_kernel_id)
  {
    for (x = 0; (uint)x < 0x41; x = x + 1)
    {
      if (g_errorDiffusionDeltaTablePtrs[x] != (void *)0x0)
      {
        free(g_errorDiffusionDeltaTablePtrs[x]);
        g_errorDiffusionDeltaTablePtrs[x] = (void *)0x0;
      }
    }
    InitErrorDiffusionDeltaTables(dither_kernel_id, (int *)g_errorDiffusionDeltaTablePtrs);
    g_rgbQuantKernelCachedId = dither_kernel_id;
  }

  for (x = 0; (uint)x < 5; x = x + 1)
  {
    memset((void *)(g_errorDiffusionRowStorage + x * 0x8060), 0, 0x8060);
    err_row_ptrs[x + 1] = (int)(g_errorDiffusionRowStorage + x * 0x8060) + 0x28;
  }

  kernel_count = g_ditherKernelTapCounts[dither_kernel_id];
  for (y = 0; y < height; y = y + 1, *(int *)&bgr24 += width * 3 + row_padding)
  {
    if (0 < x_step)
    {
      x_start = 0;
      x_end = width;
      x_byte_step = 3;
    }
    else
    {
      x_start = width + -1;
      x_end = -1;
      x_byte_step = -3;
    }

    x = x_start;
    x_byte_off = x_start + x_start + x_start;
    for (; x != x_end; x = x + x_step, x_byte_off = x_byte_off + x_byte_step)
    {
      src_rgb = *(uint *)(x_byte_off + (ptrdiff_t)bgr24) & 0xffffff;
      *(uint *)&((byte *)bgr24)[x_byte_off] = *(uint *)(x_byte_off + (ptrdiff_t)bgr24) & 0xff000000;

      err_px = (short *)err_row_ptrs[1] + (x << 2);

      b_clamped = src_rgb & 0xff;
      b_clamped += ((int)*err_px >> 8);
      b_clamped = (uint)(byte)g_u8ClampTable[b_clamped];

      g_clamped = ((byte *)&src_rgb)[1];
      g_clamped += ((int)err_px[1] >> 8);
      g_clamped = (uint)(byte)g_u8ClampTable[g_clamped];

      r_clamped = (src_rgb & 0xff0000) >> 16;
      r_clamped += ((int)err_px[2] >> 8);
      r_clamped = (uint)(byte)g_u8ClampTable[r_clamped];

      src_rgb = g_clamped << 8 | r_clamped << 0x10 | b_clamped;
      if (src_rgb == 0)
      {
        quant_rgb = 0;
      }
      else if (src_rgb == 0xffffff)
      {
        quant_rgb = 0xffffff;
      }
      else
      {
        quant_rgb = Rgb888_QuantizeToF8(src_rgb);
      }
      *(uint *)&((byte *)bgr24)[x_byte_off] = *(uint *)(x_byte_off + (ptrdiff_t)bgr24) | quant_rgb;

      err_b = b_clamped - (quant_rgb & 0xff);
      err_g = g_clamped - (quant_rgb >> 8 & 0xff);
      err_r = r_clamped - (quant_rgb >> 16 & 0xff);
      err_row_ptrs[0] = 0;
      k_it = (int *)kernel_ptr;
      for (; err_row_ptrs[0] < kernel_count; err_row_ptrs[0]++, k_it += 0x4)
      {
        k_x_off = k_it[1];
        k_row = k_it[2];
        delta_table = (int *)k_it[3];
        err_cell = (short *)(err_row_ptrs[k_row + 1] +
                             (k_x_off + x) * 8);

        err_cell[0] = delta_table[err_b] + err_cell[0];
        err_cell[1] = delta_table[err_g] + err_cell[1];
        err_cell[2] = delta_table[err_r] + err_cell[2];
      }
    }

    RotateDwordsLeft1((undefined4 *)(err_row_ptrs + 1), g_ditherKernelErrorRowCounts[dither_kernel_id]);
    memset((void *)(err_row_ptrs[g_ditherKernelErrorRowCounts[dither_kernel_id]] + -0x28), 0, clear_dwords << 2);
    if (serpentine != 0)
    {
      x_step = -x_step;
      kernel_ptr = &g_ditherKernelTable[(uint)(x_step == -1) * 0x6c0 + dither_kernel_id * 0xc0];
    }
  }
  return 1;
}

// MATCHING
// FUNCTION: CARDARTLIB 0x100068d0
// FUNCTION: DRAWCARDLIB 0x10002e80
// FUNCTION: MAGIC 0x004bbac0
// FUNCTION: SHANDALAR 0x0042fde0
uint Rgb888_QuantizeToF8(uint rgb)
{
  return rgb & 0xf8f8f8;
}

// FUNCTION: CARDARTLIB 0x100068f0
// FUNCTION: DRAWCARDLIB 0x10008370
// FUNCTION: MAGIC 0x0041f670
WvlEntry *Catalog_LoadWvlEntry(int catalog_id, char *wvl_path, int decode_haar)
{
  char fullpath[0x108]; /* [ebp-0x418] */
  char dir[0x104];      /* [ebp-0x310] */
  char fname[0x100];    /* [ebp-0x20c] */
  char ext[0x100];      /* [ebp-0x10c] */
  size_t entry_size;    /* [ebp-0x0c] */
  WvlEntry *entry;      /* [ebp-0x08] */
  char *dir_end;        /* [ebp-0x04] */

  entry = &g_wvlEntryScratch;
  EnterCriticalSection(&global_critical_section_for_catalog);
  _splitpath(wvl_path, (char *)0x0, dir, fname, ext);
  if (g_wvlCatalogsInitialized == 0)
  {
    strcpy(fullpath, dir);
    strcat(fullpath, "SmallArt.cat");
    g_smallArtCatalogHandle = Catalog_Open(fullpath);
    strcpy(fullpath, dir);
    strcat(fullpath, "MedArt.cat");
    g_medArtCatalogHandle = Catalog_Open(fullpath);
    g_wvlCatalogsInitialized = 1;
  }
  dir_end = dir + strlen(dir);

  if (catalog_id == 0)
  {
    g_activeWvlCatalogHandle = g_smallArtCatalogHandle;
  }
  else if (catalog_id == 1)
  {
    g_activeWvlCatalogHandle = g_medArtCatalogHandle;
  }
  else
  {
    /* NOTE: Original does not leave the critical section on this path. */
    return (WvlEntry *)0x0;
  }

  strcpy(dir, fname);
  strcat(dir, ext);
  _strlwr(dir);

  if (entry != (WvlEntry *)0x0)
  {
    memset(entry, 0, 0x1b0);
    strcpy(entry->wvl_path, wvl_path);
    entry->data_ptr = (byte *)&g_catalogReadScratch;
    entry_size = Catalog_ReadEntry(g_activeWvlCatalogHandle, dir, (void **)&entry->data_ptr);
    if (entry_size == (size_t)-1)
    {
      strcat(wvl_path, "\n");
      OutputDebugStringA((LPCSTR)wvl_path);
      LeaveCriticalSection(&global_critical_section_for_catalog);
      return (WvlEntry *)0x0;
    }

    entry->data_size = entry_size - 0x9c;
    memcpy(entry, entry->data_ptr, 0x9c);
    entry->data_ptr = entry->data_ptr + 0x9c;
    if (entry->pieces == 4)
    {
      entry->width = entry->width << 1;
      entry->height = entry->height << 1;
    }

    if (decode_haar != 0)
    {
      entry->decoded_ptr = Wvl_DecodeHaar(entry, (byte *)0x0);
      if (entry->decoded_ptr != 0)
      {
        entry->decoded_flag = 1;
        LeaveCriticalSection(&global_critical_section_for_catalog);
      }
      else
      {
        Catalog_Unlock(entry);
        LeaveCriticalSection(&global_critical_section_for_catalog);
        return (WvlEntry *)0x0;
      }
    }
  }

  /* NOTE: Original does not leave the critical section on this path. */
  return entry;
}

// FUNCTION: CARDARTLIB 0x10006be3
// FUNCTION: DRAWCARDLIB 0x10008663
// FUNCTION: MAGIC 0x0041f965
// FUNCTION: SHANDALAR 0x00566693
byte *Wvl_DecodeHaar(WvlEntry *wvl, byte *dst)
{
  byte *bgr_tmp;      /* [ebp-0x4c] */
  int scale_i;        /* [ebp-0x48] */
  int piece_idx;      /* [ebp-0x44] */
  int *y_plane;       /* [ebp-0x40] */
  int have_dst;       /* [ebp-0x3c] */
  int image_size;     /* [ebp-0x38] */
  int *unused2;       /* [ebp-0x34] */
  int chroma_base;    /* [ebp-0x30] */
  int height_px;      /* [ebp-0x2c] */
  int chroma_w;       /* [ebp-0x28] */
  int *unused1;       /* [ebp-0x24] */
  int base_size;      /* [ebp-0x20] */
  int alloc_bytes;    /* [ebp-0x1c] */
  int *v_plane;       /* [ebp-0x18] */
  int block_width;    /* [ebp-0x14] */
  int *u_plane;       /* [ebp-0x10] */
  int block_height;   /* [ebp-0xc] */
  int *unused3;       /* [ebp-0x8] */
  int pieces_per_row; /* [ebp-0x4] */

  have_dst = 0;
  if (g_waveletScaleToByteTableInit == 0)
  {
    for (scale_i = -0x400; scale_i <= 0x400; scale_i = scale_i + 1)
    {
      if (scale_i >= 0 && scale_i <= 0xf8)
      {
        g_waveletScaleToByteTable[scale_i] = (char)((scale_i * 255) / 0xf8);
      }
      else if (scale_i < 0xa)
      {
        g_waveletScaleToByteTable[scale_i] = 0;
      }
      else
      {
        g_waveletScaleToByteTable[scale_i] = 0xff;
      }
    }
    g_waveletScaleToByteTableInit = 1;
  }

  if (dst == NULL)
  {
    dst = malloc(wvl->decode_bytes + 2000);
    MemZeroDwords((undefined8 *)dst, (wvl->decode_bytes + 2000) / 4);
  }
  else
  {
    MemZeroDwords((undefined8 *)dst, (wvl->decode_bytes + 2000) / 4);
    have_dst = 1;
  }

  g_unpackResultUnused = Wvl_UnpackPieces(dst, wvl);

  if (wvl->pieces == 1)
  {
    pieces_per_row = 1;
  }
  else if (wvl->pieces == 4)
  {
    pieces_per_row = 2;
  }
  else if (wvl->pieces == 0x10)
  {
    pieces_per_row = 4;
  }
  else
  {
    assert(0, "D:\\Newmagic\\sources\\NedCard\\haar.c", 0x15e,
           "wavelet pieces has illegal value: %d", wvl->pieces);
  }

  image_size = wvl->width;
  height_px = image_size;
  block_width = wvl->width / pieces_per_row;
  block_height = block_width;
  alloc_bytes = wvl->base_size;
  base_size = wvl->height / pieces_per_row;
  unused1 = &wvl->unk_2c;
  unused2 = &wvl->unk_3c;
  unused3 = &wvl->unk_4c;

  for (piece_idx = 0; piece_idx < wvl->pieces; piece_idx = piece_idx + 1)
  {
    if (wvl->chroma_is_420 != 0)
    {
      chroma_w = (block_height / pieces_per_row) / (((wvl->pieces == 1) ? 2 : 1));
      chroma_base = (base_size / pieces_per_row) / (((wvl->pieces == 1) ? 2 : 1));
    }
    else
    {
      chroma_w = block_height;
      chroma_base = base_size;
    }

    y_plane = (int *)&dst[(block_height * block_height + chroma_w * chroma_w * 2 + 0x40) * piece_idx * 4];
    u_plane = &y_plane[block_height * block_height] + 0x20;
    v_plane = &u_plane[chroma_w * chroma_w] + 0x20;

    Haar2D_ReconstructInPlace(y_plane, block_height, alloc_bytes);
    Haar2D_ReconstructInPlace(u_plane, chroma_w, alloc_bytes);
    Haar2D_ReconstructInPlace(v_plane, chroma_w, alloc_bytes);

    if (piece_idx < wvl->pieces / 2)
    {
      bgr_tmp = YuvPlanesToBgr24(g_haarDecodeScratch, y_plane, block_height, block_height,
                                 u_plane, v_plane, chroma_w, chroma_w, wvl->chroma_is_420);
    }
    else if (wvl->pieces > 1)
    {
      bgr_tmp = YuvPlanesToBgr24(g_haarDecodeScratch, y_plane, block_height, wvl->height - block_width,
                                 u_plane, v_plane, chroma_w, chroma_base, wvl->chroma_is_420);
    }
    else
    {
      bgr_tmp = YuvPlanesToBgr24(g_haarDecodeScratch, y_plane, block_height, base_size,
                                 u_plane, v_plane, chroma_w, chroma_base, wvl->chroma_is_420);
    }

    if (wvl->pieces > 1)
    {

      CopyBgr24RectIntoStridedBuffer(dst, bgr_tmp,
                                     (height_px / pieces_per_row) * (piece_idx % pieces_per_row),
                                     (height_px / pieces_per_row) * (piece_idx / pieces_per_row), block_height,
                                     block_height, height_px, height_px);
      free(bgr_tmp);
    }
    else
    {
      if (have_dst == 0)
      {
        free(dst);
      }
      dst = bgr_tmp;
    }
  }
  return dst;
}

// FUNCTION: CARDARTLIB 0x10007035
// FUNCTION: DRAWCARDLIB 0x10008ab5
// FUNCTION: SHANDALAR 0x00566ae7
void CopyBgr24RectIntoStridedBuffer(byte *dst_bgr24, byte *src_bgr24, int dst_x, int dst_y, int rect_width,
                                    int rect_height, int dst_stride_pixels, int unused)
{
  int row;

  dst_bgr24 += (dst_stride_pixels * dst_y + dst_x) * 3;

  for (row = 0; row < rect_height; row = row + 1, dst_bgr24 += dst_stride_pixels * 3, src_bgr24 += rect_width * 3)
  {
    CopyBytes(dst_bgr24, src_bgr24, rect_width * 3);
  }
}

// FUNCTION: CARDARTLIB 0x10007238
// FUNCTION: DRAWCARDLIB 0x10008cb8
void CopyBytes(void *dst, const void *src, size_t num)
{
#ifdef MODERN_FIXES
  memcpy(dst, src, num);
#else
  // TODO: this looks like real inline asm but who knows
  __asm {
    mov edi, dst
    mov esi, src
    mov ecx, num
    push ecx
    shr ecx, 3
    je copy_bytes_tail
  copy_bytes_qword_loop:
    fld qword ptr [esi]
    fstp qword ptr [edi]
    add esi, 8
    add edi, 8
    dec ecx
    jne copy_bytes_qword_loop
  copy_bytes_tail:
    pop ecx
    and ecx, 7
    je copy_bytes_done
    rep movsb
  copy_bytes_done:
  }
#endif
}

// FUNCTION: CARDARTLIB 0x10007273
// FUNCTION: DRAWCARDLIB 0x10008cf3
// FUNCTION: SHANDALAR 0x00566d25
void SetBytes(void *dst, int value, size_t num)
{
#ifdef MODERN_FIXES
  memset(dst, value, num);
#else
  // This is weird because it doesn't do anything?
  __int64 uVar1;

  uVar1 = (__int64)(value << 8 | value);
  uVar1 |= uVar1 << 16;
  uVar1 |= uVar1 << 32;

  // TODO: this looks like real inline asm but who knows
  __asm {
    fld qword ptr [ebp - 8]
    mov edi, dst
    mov ecx, num
    push ecx
    shr ecx, 3
    dec ecx
  set_bytes_qword_loop:
    fst qword ptr [edi]
    add edi, 8
    dec ecx
    jne set_bytes_qword_loop
    fstp qword ptr [edi]
    pop ecx
    and ecx, 7
    mov eax, value
    rep stosb
  }
#endif
}

// FUNCTION: CARDARTLIB 0x100072dc
// FUNCTION: DRAWCARDLIB 0x10008d5c
// FUNCTION: SHANDALAR 0x00566d8e
void Haar2D_ReconstructInPlace(int *coeffs, int full_size, int base_size)
{
  int *idk;
  int *piVar1;       // ebp - 0x18
  int *haarScratchA; // ebp - 0x14
  int *haarScratchB; // ebp - 0x10
  int i;             // ebp - 0xc
  int *pad2;         // ebp - 8
  int *pad3;         // ebp - 4

  if (g_haarScratchInit == 0)
  {
    g_haarScratchA = malloc(0x32000);
    haarScratchA = g_haarScratchA;
    g_haarScratchB = malloc(0x32000);

    haarScratchB = g_haarScratchB;
    g_haarScratchInit = 1;
  }
  else
  {
    haarScratchA = g_haarScratchA;
    haarScratchB = g_haarScratchB;
  }

  for (i = base_size; i < full_size; i = i << 1)
  {
    pad2 = coeffs;
    idk = &coeffs[i * i];

    Haar_CombineSumDiff(pad2, idk, haarScratchA, i, i, i * 2, i);

    pad3 = &coeffs[i * i * 2];
    piVar1 = &coeffs[i * i * 3];

    Haar_CombineSumDiff(pad3, piVar1, haarScratchB, i,
                        i, i * 2, i);

    Haar_CombineSumDiffHalf(haarScratchA, haarScratchB, coeffs, i, i * 2, i * 2, i * 2);
  }
}

// FUNCTION: CARDARTLIB 0x10007419
// FUNCTION: DRAWCARDLIB 0x10008e99
// FUNCTION: SHANDALAR 0x00566ecc
void Haar_CombineSumDiff(int *src_a, int *src_b, int *dst, int width, int rows, undefined4 src_stride_unused, int dst_stride)
{
  int *src_a_end;  // ebp - 0x10
  int *src_a_row0; // ebp - 0xc
  int *dst_row;    // ebp - 8
  int row;         // ebp - 4

  row = dst_stride;

  for (row = 0; row < rows; row++, dst++, src_b++)
  {
    dst_row = dst;
    src_a_row0 = src_a;
    src_a_end = (width - 1) + src_a + 1;
    dst_row = dst_row + dst_stride * 2;
    src_a++;

    for (; src_a_end > src_a; src_a++, src_b++, dst_row += dst_stride * 2)
    {
      *dst_row = *src_b + *src_a;

      *(int *)((char *)dst_row + dst_stride * 4) = *src_a - *src_b;
    }

    *dst = *src_b + *src_a_row0;
    dst[dst_stride] = *src_a_row0 - *src_b;
  }
}

// FUNCTION: CARDARTLIB 0x100074ee
// FUNCTION: DRAWCARDLIB 0x10008f6e
// FUNCTION: SHANDALAR 0x00566fa1
void Haar_CombineSumDiffHalf(int *src_a, int *src_b, int *dst, int width, int rows, undefined4 src_stride_unused, int dst_stride)
{
  int *src_a_end;  // ebp - 0x10
  int *src_a_row0; // ebp - 0xc
  int *dst_row;    // ebp - 8
  int row;         // ebp - 4

  for (row = 0; row < rows; row++, dst++, src_b++)
  {
    dst_row = dst;
    src_a_row0 = src_a;
    src_a_end = (width - 1) + src_a + 1;
    dst_row = dst_row + dst_stride * 2;
    src_a++;

    for (; src_a_end > src_a; src_a++, src_b++, dst_row += dst_stride * 2)
    {
      *dst_row = (*src_b + *src_a) >> 1;
      *(int *)((char *)dst_row + dst_stride * 4) = (*src_a - *src_b) >> 1;
    }

    *dst = (*src_b + *src_a_row0) >> 1;
    dst[dst_stride] = (*src_a_row0 - *src_b) >> 1;
  }
}

// FUNCTION: CARDARTLIB 0x100075ef
// FUNCTION: DRAWCARDLIB 0x1000906f
// FUNCTION: SHANDALAR 0x0056709e
undefined1 *YuvPlanesToBgr24(undefined1 *out_bgr24, int *luma, int width, int height, int *chroma_u, int *chroma_v,
                             int chroma_stride, undefined4 unused_chroma_height, int chroma_is_420)

{
  int y;                /* ebp - 0x2c */
  int u;                /* ebp - 0x28 */
  int v;                /* ebp - 0x24 */
  undefined1 *out_base; /* ebp - 0x20 */
  int row;              /* ebp - 0x1c */
  int col;              /* ebp - 0x18 */
  int green;            /* ebp - 0x14 */
  int *chroma_u_ptr;    /* ebp - 0x10 */
  int *chroma_v_ptr;    /* ebp - 0xc */
  int red;              /* ebp - 8 */
  int blue;             /* ebp - 4 */

  if (g_yuvClampTableInit == 0)
  {
    for (col = -0x400; col < 0x1c00; col = col + 1)
    {
      if (0 < col)
      {
        g_yuvClampTable[col] = (char)(((col >> 2) <= 0xff) ? (col >> 2) : 0xff);
      }
      else
      {
        g_yuvClampTable[col] = 0;
      }
    }
    g_yuvClampTableInit = 1;
  }
  if (out_bgr24 == (undefined1 *)0x0)
  {
    out_bgr24 = malloc(width * width * 3 + 0x10);
    out_base = out_bgr24;
  }
  else
  {
    out_base = out_bgr24;
  }

  for (row = 0; row < height; row++)
  {
    if (chroma_is_420 != 0)
    {
      chroma_u_ptr = &chroma_u[row / 2 * chroma_stride];
      chroma_v_ptr = &chroma_v[row / 2 * chroma_stride];
    }
    else
    {
      chroma_u_ptr = &chroma_u[row * chroma_stride];
      chroma_v_ptr = &chroma_v[row * chroma_stride];
    }

    for (col = 0; col < width; col++, luma++, out_bgr24 += 3)
    {
      y = *luma;

      if (chroma_is_420 != 0)
      {
        if ((col & 1) == 0)
        {
          u = *chroma_u_ptr;
          v = *chroma_v_ptr;
        }
        else
        {
          u = (*chroma_u_ptr + chroma_u_ptr[((uint)(width - 1) - (uint)col) != 0]) / 2;
          v = (*chroma_v_ptr + chroma_v_ptr[((uint)(width - 1) - (uint)col) != 0]) / 2;
        }

        red = ((v >> 3) + (v >> 1) + v) - 0x333 + y;
        blue = (u * 2) - 0x400 + y;
      }
      else
      {
        u = *chroma_u_ptr;
        v = *chroma_v_ptr;

        red = ((v >> 3) + (v >> 1) + v) - 0x333 + y;
        blue = (u * 2) - 0x400 + y;
      }

      green = (((y * 2) - (y >> 2)) - (red >> 1)) - ((blue >> 2) - (blue >> 4));
      out_bgr24[0] = g_yuvClampTable[blue];
      out_bgr24[1] = g_yuvClampTable[green];
      out_bgr24[2] = g_yuvClampTable[red];

      if (chroma_is_420 != 0)
      {
        if ((col & 1) != 0)
        {
          chroma_u_ptr = chroma_u_ptr + 1;
          chroma_v_ptr = chroma_v_ptr + 1;
        }
      }
      else
      {
        chroma_u_ptr = chroma_u_ptr + 1;
        chroma_v_ptr = chroma_v_ptr + 1;
      }
    }
  }
  return out_base;
}

// FUNCTION: CARDARTLIB 0x100078b8
// FUNCTION: DRAWCARDLIB 0x10009338
// FUNCTION: SHANDALAR 0x0056735f
BOOL Wvl_UnpackPieces(byte *dst, WvlEntry *wvl)
{
  byte *dst_y;        /* ebp - 0x3c */
  WvlEntry *target;   /* ebp - 0x38 */
  int node_count;     /* ebp - 0x34 */
  int chroma_w;       /* ebp - 0x30 */
  int base_size;      /* ebp - 0x2c */
  int chroma_h;       /* ebp - 0x28 */
  int layer;          /* ebp - 0x24 */
  uint *symbol_table; /* ebp - 0x20 */
  int full_w;         /* ebp - 0x1c */
  byte *dst_v;        /* ebp - 0x18 */
  byte *bitstream;    /* ebp - 0x14 */
  byte *huff_data;    /* ebp - 0x10 */
  int full_h;         /* ebp - 0x0c */
  int tmp;            /* ebp - 0x08 */
  byte *dst_u;        /* ebp - 0x04 */

  target = wvl;

  full_w = target->width / ((target->pieces == 1) ? 1 : 2);
  full_h = full_w;
  chroma_w = full_h / ((target->chroma_is_420 == 0) ? 1 : 2);
  chroma_h = chroma_w;

  bitstream = wvl->data_ptr;
  huff_data = bitstream;
  base_size = target->base_size;

  tmp = (full_w * full_h) + chroma_w * chroma_h * 2;

  node_count = *(int *)bitstream;
  bitstream = bitstream + 4;
  symbol_table = (uint *)bitstream;
  *symbol_table = 0x80000000;
  bitstream = bitstream + (node_count << 2);
  bitstream =
      bitstream +
      Huffman13_Init((undefined4)bitstream, (undefined4)symbol_table, (undefined4)node_count);

  for (layer = 0; layer < target->pieces; layer = layer + 1)
  {
    dst_y = &dst[(((tmp + 0x40) * layer) << 2)];

    dst_u = &dst_y[((full_w * full_h) << 2)] + 0x80;

    dst_v = &dst_u[((chroma_w * chroma_h) << 2)] + 0x80;

    memcpy(dst_y, bitstream, (base_size * base_size) << 2);

    dst_y += (base_size * base_size) << 2;

    bitstream += (base_size * base_size) << 2;

    Huffman13_DecodeDwordsWithZeroRuns((undefined8 *)dst_y, (uint *)bitstream, target->huff_bytes_y[layer]);
    bitstream = bitstream + target->huff_bytes_y[layer];

    memcpy(dst_u, bitstream, (base_size * base_size) << 2);

    dst_u += (base_size * base_size) << 2;

    bitstream += (base_size * base_size) << 2;

    Huffman13_DecodeDwordsWithZeroRuns((undefined8 *)dst_u, (uint *)bitstream, target->huff_bytes_u[layer]);
    bitstream = bitstream + target->huff_bytes_u[layer];

    memcpy(dst_v, bitstream, (base_size * base_size) << 2);

    dst_v += (base_size * base_size) << 2;

    bitstream += (base_size * base_size) << 2;

    Huffman13_DecodeDwordsWithZeroRuns((undefined8 *)dst_v, (uint *)bitstream, target->huff_bytes_v[layer]);
    bitstream = bitstream + target->huff_bytes_v[layer];
  }
  return 0;
}

// FUNCTION: CARDARTLIB 0x1000807f
// FUNCTION: DRAWCARDLIB 0x10009aff
// FUNCTION: SHANDALAR 0x005678fa
uint *Wvl_DecodeToBgr24(byte *out_bgr24, WvlEntry *wvl_entry, int width, int height)
{
  int output_row_align;
  int row_padding;
  int y_step;
  int x_step;
  byte *out_base;
  uint x_map[4096];
  byte *decoded;    // ebp - 0x104c
  int y;            // ebp - 0x1048
  int y_acc;        // ebp - 0x1044
  int x;            // ebp - 0x1040
  int out_nonnull;  // ebp - 0x103c
  int src_width;    // ebp - 0x1038
  int *y_map_ptr;   // ebp - 0x1034
  int x_acc;        // ebp - 0x1030
  int *x_map_ptr;   // ebp - 0x102c
  byte *out_start;  // ebp - 0x1028
  byte *out_ptr;    // ebp - 0x1024
  int y_scale;      // ebp - 0x1020
  int row_pad;      // ebp - 0x101c
  int row_bytes;    // ebp - 0x1018
  int x_scale;      // ebp - 0x1014
  int zero_1010;    // ebp - 0x1010
  byte *src_ptr;    // ebp - 0x100c
  uint y_map[1024]; // ebp - 0x1008
  int tmp_8;        // ebp - 0x8
  byte *tmp_4;      // ebp - 0x4

  x_acc = 0;
  y_acc = 0;
  out_base = out_bgr24;
  out_start = (byte *)0;
  x_map_ptr = x_map;
  y_map_ptr = y_map;
  if (out_bgr24 != (byte *)0)
  {
    out_nonnull = 1;
  }
  else
  {
    out_nonnull = 0;
  }

  if (wvl_entry == (WvlEntry *)0)
  {
    return (uint *)0;
  }

  x_scale = wvl_entry->width;
  y_scale = wvl_entry->height;
  x_scale <<= 0x10;
  y_scale <<= 0x10;
  x_step = x_scale / width;
  y_step = y_scale / height;

  if (wvl_entry->decoded_flag != 0)
  {
    decoded = wvl_entry->decoded_ptr;
  }
  else
  {
    decoded = Wvl_DecodeHaar(wvl_entry, g_haarDecodeScratch);
  }

  src_width = wvl_entry->width;
  tmp_8 = 3;
  zero_1010 = 0;
  row_pad = (g_rowAlignBytes - (width * 3) % g_rowAlignBytes) % g_rowAlignBytes;

  if (out_bgr24 == (byte *)0)
  {
    out_bgr24 = g_catalogReadScratch;
    out_base = out_bgr24;
  }
  else
  {
    out_base = out_bgr24;
  }

  for (x = 0, x_acc = 0; x < width; x += 1, x_acc += x_step, x_map_ptr += 1)
  {
    if (tmp_8 == 3)
    {
      *x_map_ptr = (uint)(x_acc >> 8);
    }
    else
    {
      assert(0, "D:\\Newmagic\\sources\\NedCard\\haar.c", 0x6f7,
             "Only Works on 24 bit images\n");
    }
  }

  if (wvl_entry->height < height)
  {
    out_bgr24 += (height - wvl_entry->height) * (row_pad + width * 3);
    out_ptr = out_bgr24;
  }
  else
    out_ptr = (byte *)out_bgr24;

  for (y = 0; y < wvl_entry->height; y += 1, out_bgr24 += row_pad)
  {
    tmp_4 = decoded + (src_width * tmp_8 + zero_1010) * y;
    x_map_ptr = x_map;

    for (x = 0; x < width; x += 1, x_map_ptr += 1)
    {
      src_ptr = tmp_4 + (((int)*x_map_ptr >> 8) * 3);

      out_bgr24[0] = ((int)((uint)src_ptr[3] - (uint)src_ptr[0]) * (*x_map_ptr & 0xff) >> 8) + (int)(uint)src_ptr[0];
      out_bgr24[1] = ((int)((uint)src_ptr[4] - (uint)src_ptr[1]) * (*x_map_ptr & 0xff) >> 8) + (int)(uint)src_ptr[1];
      out_bgr24[2] = ((int)((uint)src_ptr[5] - (uint)src_ptr[2]) * (*x_map_ptr & 0xff) >> 8) + (int)(uint)src_ptr[2];

      out_bgr24 += 3;
    }
  }

  for (x = 0, y_acc = 0; x < height; x++, y_acc += y_step, y_map_ptr++)
  {
    if (tmp_8 == 3)
    {
      *y_map_ptr = (uint)(y_acc >> 8);
    }
    else
    {
      assert(0, "D:\\Newmagic\\sources\\NedCard\\haar.c", 0x730,
             "Only Works on 24 bit images\n");
    }
  }

  row_bytes = width * 3 + row_pad;
  if (height < wvl_entry->height)
  {
    memcpy(x_map, &out_base[(wvl_entry->height - 1) * row_bytes], row_bytes);
  }

  for (x = 0; x < width; x++)
  {
    out_bgr24 = &out_base[x * 3];
    tmp_4 = out_ptr + x * 3;
    y_map_ptr = y_map;

    for (y = 0; y < height - 1; y++, y_map_ptr++, out_bgr24 += row_bytes)
    {
      src_ptr = (byte *)(MIN((int)*y_map_ptr >> 8, wvl_entry->height - 2) * row_bytes + (int)tmp_4);

      out_bgr24[0] = ((int)((uint)src_ptr[row_bytes] - (uint)src_ptr[0]) * (*y_map_ptr & 0xff) >> 8) + (int)(uint)src_ptr[0];
      out_bgr24[1] = ((int)((uint)src_ptr[row_bytes + 1] - (uint)src_ptr[1]) * (*y_map_ptr & 0xff) >> 8) + (int)(uint)src_ptr[1];
      out_bgr24[2] = ((int)((uint)src_ptr[row_bytes + 2] - (uint)src_ptr[2]) * (*y_map_ptr & 0xff) >> 8) + (int)(uint)src_ptr[2];
    }
  }

  if (height < wvl_entry->height)
  {
    memcpy(&out_base[(height - 1) * row_bytes], x_map, row_bytes);
  }
  else
  {
    memset(&out_base[(height - 1) * row_bytes], 0, row_bytes);
  }

  output_row_align = 4;
  row_padding = (output_row_align - ((width * 3) % output_row_align)) % output_row_align;
  if (global_dither_kernel_id == 0)
  {
    QuantizeBgr24ToNearestPaletteColorInPlace((uint *)out_base, height, width, row_padding);
  }
  else if (global_color_depth == 16)
  {
    DitherBgr24ToRgbQuantizedF8(global_dither_kernel_id, global_serpentine, (uint *)out_base, height, width, row_padding);
  }
  else if (global_color_depth == 8)
  {
    DitherBgr24ToPaletteColors(global_dither_kernel_id, global_serpentine, (uint *)out_base, height, width, row_padding);
  }

  return (uint *)out_base;
}
