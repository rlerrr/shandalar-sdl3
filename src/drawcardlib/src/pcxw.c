#include "platform/gdi_compat.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "pcxw.h"
#include "cardartlib/src/assert.h"

typedef void *pointer;
#ifndef MODERN_FIXES
#endif
typedef unsigned short ushort;

typedef struct
{
  char manufacturer;   /* 0x00 */
  char version;        /* 0x01 */
  char encoding;       /* 0x02 */
  char bitsPerPixel;   /* 0x03 */
  ushort xmin;         /* 0x04 */
  ushort ymin;         /* 0x06 */
  ushort xmax;         /* 0x08 */
  ushort ymax;         /* 0x0A */
  ushort hDpi;         /* 0x0C */
  ushort vDpi;         /* 0x0E */
  byte colormap[0x30]; /* 0x10 */
  byte reserved;       /* 0x40 */
  char nPlanes;        /* 0x41 */
  ushort bytesPerLine; /* 0x42 */
  ushort paletteInfo;  /* 0x44 */
  ushort hScreenSize;  /* 0x46 */
  ushort vScreenSize;  /* 0x48 */
  byte filler[0x36];   /* 0x4A */
} PcxHeader_t;

STATIC_ASSERT(sizeof(PcxHeader_t) == 0x80, PCXHEADER_wrong_size);

// GLOBAL: DRAWCARDLIB 0x10021ed4
// GLOBAL: DECKDLL 0x10033918
int gPcxBufferWidth = 0x00000000;
// GLOBAL: DRAWCARDLIB 0x10021ed8
// GLOBAL: DECKDLL 0x1003391c
int gPcxBufferHeight = 0x00000000;

#if defined(FACEMAKER)
// GLOBAL: FACEMAKER 0x00411790
char *PTR_s_D__Newmagic_sources_sidlib_Pcxw__10021ed0 = "D:\\NewMagic\\sources\\sidlib\\Pcxw.c";
#else
// GLOBAL: DRAWCARDLIB 0x10021ed0
// GLOBAL: DECKDLL 0x10033914
// GLOBAL: SHANDALAR 0x005a5d10
char *PTR_s_D__Newmagic_sources_sidlib_Pcxw__10021ed0 = "D:\\Newmagic\\sources\\sidlib\\Pcxw.c";
#endif

// GLOBAL: DRAWCARDLIB 0x10026548
// GLOBAL: DECKDLL 0x1003a344
// GLOBAL: SHANDALAR 0x005a58c0
// GLOBAL: FACEMAKER 0x00411340
int global_pcxw_image_width = 0x00000000;

// GLOBAL: DRAWCARDLIB 0x1002654c
// GLOBAL: DECKDLL 0x1003a348
// GLOBAL: SHANDALAR 0x005a58c4
// GLOBAL: FACEMAKER 0x00411344
int global_pcxw_image_height = 0x00000000;

// GLOBAL: DRAWCARDLIB 0x10127d30
// GLOBAL: DECKDLL 0x10113960
// GLOBAL: FACEMAKER 0x0041f55c
// GLOBAL: SHANDALAR 0x0097f994
FILE *gPcxInFile;

// GLOBAL: DRAWCARDLIB 0x10127d34
// GLOBAL: DECKDLL 0x10113964
// GLOBAL: MAGIC 0x00777524
FILE *gPcxOutFile;

// GLOBAL: DRAWCARDLIB 0x10127d38
// GLOBAL: DECKDLL 0x10113968
// GLOBAL: FACEMAKER 0x0041f560
// GLOBAL: SHANDALAR 0x0097f998
char *gPcxPath;

// GLOBAL: DRAWCARDLIB 0x10126d30
// GLOBAL: DECKDLL 0x10112960
byte gPcxScanlineBuffer[0x1000];

// GLOBAL: SHANDALAR 0x0097F9A0
// GLOBAL: DRAWCARDLIB 0x10127d40
// GLOBAL: DECKDLL 0x10113970
// GLOBAL: FACEMAKER 0x0041f570
PcxHeader_t gPcxHeader;

// FUNCTION: DRAWCARDLIB 0x1000b80f
// FUNCTION: DECKDLL 0x1002d2b3
void PcxBuildSaveScanline(byte *dstScanline, undefined *srcPixels, undefined4 arg3, undefined4 arg4, int width)
{
}

// FUNCTION: DRAWCARDLIB 0x10004050
// FUNCTION: DECKDLL 0x10016280
undefined1 *PcxLoad8bppImage(char *path, undefined1 *imagePixels, void *paletteOut)
{
  int isPalettized256;
  int col;
  int padBytes;
  undefined1 *basePixels;
  int row;

  gPcxInFile = fopen(path, "rb");
  assert(gPcxInFile != (FILE *)0x0, PTR_s_D__Newmagic_sources_sidlib_Pcxw__10021ed0, 0x69,
         "Error Opening File %s\r\n", path);
  gPcxPath = path;
  PcxReadHeaderAndPalette(paletteOut);
  if ((gPcxHeader.bitsPerPixel == 8) && (gPcxHeader.nPlanes == 1))
  {
    isPalettized256 = 1;
  }
  else
  {
    isPalettized256 = 0;
  }
  assert(isPalettized256, PTR_s_D__Newmagic_sources_sidlib_Pcxw__10021ed0, 0x6f,
         "%s Not a 256 color palettized pcx file\r\n", gPcxPath);
  padBytes = abs(4 - (global_pcxw_image_width & 3)) & 3;
  if ((global_pcxw_image_width == gPcxBufferWidth) && (global_pcxw_image_height == gPcxBufferHeight))
  {
    memset(imagePixels, 0, (gPcxBufferWidth + padBytes) * gPcxBufferHeight);
    basePixels = imagePixels;
  }
  else
  {
    GlobalUnlock(GlobalHandle(imagePixels));
    GlobalUnlock(GlobalHandle(imagePixels));
    GlobalFree(GlobalHandle(imagePixels));
    imagePixels = GlobalLock(GlobalAlloc(0x40, (global_pcxw_image_width + padBytes) * global_pcxw_image_height));
    basePixels = imagePixels;
    GlobalLock(GlobalHandle(imagePixels));
    gPcxBufferWidth = global_pcxw_image_width;
    gPcxBufferHeight = global_pcxw_image_height;
  }

  for (row = 0; global_pcxw_image_height > row; row = row + 1)
  {
    PcxReadScanlineRle((char *)gPcxScanlineBuffer);
    for (col = 0; col < global_pcxw_image_width; col = col + 1)
    {
      *imagePixels = gPcxScanlineBuffer[col];
      imagePixels = imagePixels + 1;
    }
    imagePixels = imagePixels + padBytes;
  }
  fclose(gPcxInFile);
  return basePixels;
}

// FUNCTION: DRAWCARDLIB 0x10004267
// FUNCTION: DECKDLL 0x1001649a
bool PcxReadHeaderAndPaletteFromPath(char *path, void *paletteOut)
{
  gPcxInFile = fopen(path, "rb");
  assert(gPcxInFile != (FILE *)0x0, PTR_s_D__Newmagic_sources_sidlib_Pcxw__10021ed0, 0x9c,
         "Error Opening File %s\r\n", path);
  gPcxPath = path;

  if (PcxReadHeaderAndPalette(paletteOut) == 0)
    return 0;

  fclose(gPcxInFile);
  return 1;
}

#if defined(FACEMAKER) || defined(SHANDALAR)
// For some reason these are actually optimized in facemaker?
#endif

// FUNCTION: DRAWCARDLIB 0x100042ea
// FUNCTION: DECKDLL 0x1001651f
// FUNCTION: SHANDALAR 0x0057d850
// FUNCTION: FACEMAKER 0x0040aa50
undefined4 PcxReadHeaderAndPalette(void *paletteOut)
{
  int paletteIndex;

  fread(&gPcxHeader, 0x80, 1, gPcxInFile);
  assert(gPcxHeader.manufacturer == 0x0a, PTR_s_D__Newmagic_sources_sidlib_Pcxw__10021ed0, 0xad,
         "%s Not a pcx file\r\n", gPcxPath);
  assert(gPcxHeader.version == 5, PTR_s_D__Newmagic_sources_sidlib_Pcxw__10021ed0, 0xae,
         "%s Not a version 5 pcx file\r\n", gPcxPath);
  global_pcxw_image_width = ((uint)gPcxHeader.xmax - (uint)gPcxHeader.xmin) + 1;
  global_pcxw_image_height = ((uint)gPcxHeader.ymax - (uint)gPcxHeader.ymin) + 1;
  if (paletteOut == (void *)0x0)
  {
    return 1;
  }
  if ((gPcxHeader.nPlanes == 1) && (gPcxHeader.bitsPerPixel == 8))
  {
    fseek(gPcxInFile, -0x300, 2);
    fread(paletteOut, 1, 0x300, gPcxInFile);
    fseek(gPcxInFile, 0x80, 0);
  }
  else if ((gPcxHeader.nPlanes == 4) && (gPcxHeader.bitsPerPixel == 1))
  {
    fseek(gPcxInFile, 0x10, 2);
    for (paletteIndex = 0; paletteIndex < 0x10; paletteIndex = paletteIndex + 1)
    {
      fread((byte *)paletteOut + paletteIndex * 4, 1, 3, gPcxInFile);
    }
    fseek(gPcxInFile, 0x80, 0);
  }
  else
  {
    assert(0, PTR_s_D__Newmagic_sources_sidlib_Pcxw__10021ed0, 0xd4,
           "%s is not in a recognizable format\r\n", gPcxPath);
  }
  return 1;
}

// FUNCTION: FACEMAKER 0x0040ac00
// FUNCTION: SHANDALAR 0x0057da00
// FUNCTION: DRAWCARDLIB 0x100044d0
// FUNCTION: DECKDLL 0x10016708
undefined4 PcxReadScanlineRle(char *dstScanline)
{
  int bytesRemaining;
  int runCount;
  uint repeatIndex;
  int dummy;
  byte b;

  runCount = 0;
  dummy = 0;
  bytesRemaining = (int)(short)gPcxHeader.bytesPerLine;
  while (0 < bytesRemaining)
  {
    b = (byte)fgetc(gPcxInFile);
    if ((b & 0xc0) == 0xc0)
    {
      runCount = (int)(b & 0x3f);
      b = (byte)fgetc(gPcxInFile);
      if (1 < (uint)runCount)
      {
        for (repeatIndex = 0; repeatIndex < (uint)runCount; repeatIndex = repeatIndex + 1)
        {
          *dstScanline = (char)b;
          dstScanline = dstScanline + 1;
        }

        bytesRemaining -= (uint)runCount;
      }
      else
      {
        *dstScanline = (char)b;
        dstScanline = dstScanline + 1;
        bytesRemaining--;
      }
    }
    else
    {
      *dstScanline = (char)b;
      dstScanline = dstScanline + 1;
      bytesRemaining--;
    }
  }
  return 1;
}


// FUNCTION: DRAWCARDLIB 0x100045ac
// FUNCTION: DECKDLL 0x100167e6
undefined4
PcxSave8bppImage(undefined *srcPixels, char *path, void *palette, undefined4 unused_param_4,
                 undefined4 unused_param_5, int width, int height)

{
  int row;
  undefined1 scanline[0x1000];
  byte paletteMarker;

  paletteMarker = '\f';
  gPcxOutFile = fopen(path, "w+b");
  assert((uint)(gPcxOutFile != (FILE *)0x0),
         PTR_s_D__Newmagic_sources_sidlib_Pcxw__10021ed0, 0x146,
         "Error Opening File %s\r\n", path);
  gPcxPath = path;
  PcxWriteHeader(width, height);
  for (row = 0; row < height; row = row + 1)
  {
    PcxBuildSaveScanline((byte *)scanline, srcPixels, unused_param_4, (undefined4)(row + (int)unused_param_5), width);
    PcxWriteScanlineRle((char *)scanline, width);
  }
  PcxWritePalette256(palette);
  fclose(gPcxOutFile);
  return 0;
}

// FUNCTION: DRAWCARDLIB 0x1000469c
// FUNCTION: DECKDLL 0x100168d8
undefined4 PcxWriteHeader(int width, int height)
{
  gPcxHeader.manufacturer = 0x0a;
  gPcxHeader.version = 5;
  gPcxHeader.encoding = 1;
  gPcxHeader.bitsPerPixel = 8;
  gPcxHeader.xmin = 0;
  gPcxHeader.xmax = width - 1;
  gPcxHeader.ymin = 0;
  gPcxHeader.ymax = height + -1;
  gPcxHeader.hDpi = 0;
  gPcxHeader.vDpi = 0;
  gPcxHeader.reserved = 0;
  gPcxHeader.nPlanes = 1;
  // Can't get this funky rounding to match
  gPcxHeader.bytesPerLine = width + (abs(width) & 1);
  gPcxHeader.paletteInfo = 1;
  gPcxHeader.hScreenSize = 0;
  gPcxHeader.vScreenSize = 0;
  fwrite(&gPcxHeader, 0x80, 1, gPcxOutFile);
  return 0;
}

// FUNCTION: DRAWCARDLIB 0x10004760
// FUNCTION: DECKDLL 0x1001699d
undefined4 PcxWriteScanlineRle(char *srcScanline, int widthBytes)
{
  int pos;
  int maxCount;
  int runCount;
  uint value;

  *(unsigned char *)&value = *(unsigned char *)srcScanline;
  maxCount = 0;
  pos = 0;
  while (pos < widthBytes)
  {
    *(unsigned char *)&value = *(unsigned char *)srcScanline;
    if ((widthBytes - pos == 1) || ((unsigned char)srcScanline[1] != (unsigned char)value))
    {
      PcxWriteRleByte((unsigned char)value);
      pos = pos + 1;
      srcScanline = srcScanline + 1;
    }
    else
    {
      runCount = CountRepeats((unsigned char)value, (unsigned char *)srcScanline,
                              (widthBytes - pos < 0x3f) ? (widthBytes - pos) : 0x3f);
      pos = pos + runCount;
      srcScanline = srcScanline + runCount;
      runCount = runCount | 0xc0;
      fwrite(&runCount, 1, 1, gPcxOutFile);
      fwrite(&value, 1, 1, gPcxOutFile);
    }
  }
  if (pos < (int)(short)gPcxHeader.bytesPerLine)
  {
    *(unsigned char *)&value = 0;
    fwrite(&value, 1, 1, gPcxOutFile);
  }
  return 1;
}

// FUNCTION: DRAWCARDLIB 0x10004873
// FUNCTION: DECKDLL 0x10016ab3
// FUNCTION: MAGIC 0x004995d7
void PcxWriteRleByte(unsigned char value)
{
  byte buff[4];

  buff[0] = 0xc1;
  if ((value & 0xc0) == 0xc0)
  {
    fwrite(buff, 1, 1, gPcxOutFile);
  }
  fwrite(&value, 1, 1, gPcxOutFile);
}

// FUNCTION: DRAWCARDLIB 0x100048c0
// FUNCTION: DECKDLL 0x10016b02
// FUNCTION: MAGIC 0x00499626
int CountRepeats(unsigned char value, unsigned char *buffer, int maxCount)
{
  unsigned char *current;
  int remaining;
  int count;

  count = 0;
  while ((remaining = maxCount--) != 0)
  {
    current = buffer;
    buffer++;
    if (*current == value)
    {
      count++;
    }
    else
    {
      break;
    }
  }
  return count;
}

// MATCHING
// FUNCTION: DRAWCARDLIB 0x1000491f
// FUNCTION: DECKDLL 0x10016b61
undefined4 PcxWritePalette256(void *palette)
{
  undefined1 paletteIndex[4];

  paletteIndex[0] = 0xc;
  fwrite(paletteIndex, 1, 1, gPcxOutFile);
  fwrite(palette, 3, 0x100, gPcxOutFile);
  return 0;
}
