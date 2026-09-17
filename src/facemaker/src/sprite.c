#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef _MSC_VER
#include <malloc.h>
#endif
#include "platform/legacy_io.h"
#include <string.h>
#include "cardartlib/src/assert.h"
#include "drawcardlib/src/pic.h"
#include "face_assets.h"
#include "facemaker_types.h"

#ifdef _fileno
#undef _fileno
#endif

int ReadSpriteEntryPointers(EncodedImage **out_entry_ptrs, char *sprite_path);
size_t WriteSpriteBlob(void *sprite_blob, char *output_path);
EncodedImage *EncodeSpriteFromPage(int page_number, int x, int y, unsigned int width, int height);

extern int ReadGraphicsPixel(int page_number, int x, int y);
extern void WriteGraphicsScanline(unsigned int *scanline_data, int page_number, int dst_x, int dst_y,
                                 unsigned int byte_count);
extern void ReadGraphicsScanline(unsigned int *out_scanline, int page_number, int src_x, int src_y,
                                 unsigned int byte_count);
extern void PutGraphicsPixel(FacemakerWindowBounds *window_bounds, int x, int y, unsigned int color_index);
extern void DrawEncodedImageUnscaled(FacemakerWindowBounds *dst, int x, int y, EncodedImage *encoded_image);
extern void LoadPcxIntoPageNoPalette(int page_number, char *path);
extern void BeginSpriteEncodeSession(void);
extern void FinalizeSpriteEncodeSession(void);
extern int *g_sprite_blob_base;
extern int *g_sprite_blob_cursor;
extern DIBSurface *g_graphics_pages[10];
extern FacemakerWindowBounds *g_face_fullscreen_bounds;
extern char g_file_read_mode[];

static size_t g_sprite_blob_byte_count;

static void shrink_sprite_blob(size_t byte_count)
{
#ifdef _MSC_VER
  size_t cursor_offset;
  void *resized;
#endif
  if (!g_sprite_blob_base)
    return;
#ifdef _MSC_VER
  cursor_offset = (size_t)((char *)g_sprite_blob_cursor - (char *)g_sprite_blob_base);
  resized = _expand((void *)g_sprite_blob_base, byte_count);
  if (resized) {
    g_sprite_blob_base = (int *)resized;
    g_sprite_blob_cursor = (int *)((char *)g_sprite_blob_base + cursor_offset);
  }
#endif
  /*
   * EncodeSpriteFromPage returns pointers directly into this allocation.
   * _expand is safe because it never moves the allocation, but realloc may
   * move it and leave every previously returned sprite pointer dangling.
   * There is no portable in-place realloc, so retain the original allocation
   * on non-MSVC platforms and only record the blob's logical size.
   */
  g_sprite_blob_byte_count = byte_count;
}

static size_t sprite_blob_size(void *sprite_blob)
{
  if (sprite_blob == (void *)g_sprite_blob_base && g_sprite_blob_byte_count != 0)
    return g_sprite_blob_byte_count;
#ifdef _MSC_VER
  return _msize(sprite_blob);
#else
  return 0;
#endif
}
#pragma optimize("gy", on)

// FUNCTION: SHANDALAR 0x0057b650
// FUNCTION: FACEMAKER 0x00408900
void BeginSpriteEncodeSession(void)
{
  g_sprite_blob_base = malloc(0x200000);
  g_sprite_blob_cursor = g_sprite_blob_base;
  g_sprite_blob_byte_count = 0x200000;
}

// FUNCTION: SHANDALAR 0x0057b670
// FUNCTION: FACEMAKER 0x00408920
void FinalizeSpriteEncodeSession(void)
{
  size_t byte_count;
  *g_sprite_blob_cursor = -1;
  byte_count = (size_t)((char *)g_sprite_blob_cursor - (char *)g_sprite_blob_base) + 0x10;
  shrink_sprite_blob(byte_count);
}

// FUNCTION: SHANDALAR 0x0057b6a0
// FUNCTION: FACEMAKER 0x00408950
void FreeSpriteBlob(void *memory)
{
  free(memory);
}

// FUNCTION: SHANDALAR 0x0057b6c0
// FUNCTION: FACEMAKER 0x00408970
size_t WriteSpriteBlob(void *sprite_blob, char *output_path)
{
  size_t byte_count;
  FILE *file;

  byte_count = sprite_blob_size(sprite_blob);
  if (byte_count == 0)
    return 0;
  file = fopen(output_path, "wb");
  fwrite(sprite_blob, 1, byte_count, file);
  fclose(file);
  return byte_count;
}

// FUNCTION: SHANDALAR 0x0057b710
// FUNCTION: FACEMAKER 0x004089c0
int ReadSpriteEntryPointers(EncodedImage **out_entry_ptrs, char *sprite_path)
{
  unsigned char *blob;
  size_t blob_size;
  int entry_count = facemaker_sprite_blob_load_entries(
      sprite_path, &blob, &blob_size, out_entry_ptrs, 1000);
  assert(entry_count > 0, "D:\\NewMagic\\sources\\sidlib\\sprite.c", 0xa3,
         "Could not open Sprite File %s\n", sprite_path);
  (void)blob;
  (void)blob_size;
  return entry_count;
}

// FUNCTION: SHANDALAR 0x0057b840
// FUNCTION: FACEMAKER 0x00408a50
EncodedImage *EncodeSpriteFromPage(int page_number, int x, int y, unsigned int width, int height)
{
  EncodedImage *sprite;
  unsigned char *write_cursor;
  unsigned char *last_nonempty_row_end;
  int border_left;
  int border_top;
  int line_index;
  int last_nonempty_line;
  int probe_x;
  int probe_y;
  int i;
  unsigned int run_start;
  int run_end;
  unsigned int sampled;
  unsigned int sampled_next;
  unsigned int run_len;
  unsigned char line_buffer[0x400];

  line_index = 0;
  probe_y = y + 1 + height;
  probe_x = x - 1;
  if (probe_x < 0 || probe_y < 0)
  {
    border_left = -1;
  }
  else
  {
    border_left = (int)(width != 0xfffffffe);
    sampled = ReadGraphicsPixel(page_number, probe_x, probe_y);
    run_end = width + 2;
    if (width == 0xfffffffe)
    {
      run_end = 0;
    }
    run_start = 0;
    i = probe_x;
    if (0 < run_end)
    {
      do
      {
        sampled_next = ReadGraphicsPixel(page_number, i, probe_y);
        if (sampled_next != sampled)
        {
          break;
        }
        run_start = run_start + 1;
        i = i + border_left;
      } while (run_start < run_end);
    }
    border_left = -1;
    if (run_end != run_start)
    {
      border_left = run_start;
    }
  }

  probe_y = y - 1;
  if (probe_x < 0 || probe_y < 0)
  {
    border_top = -1;
  }
  else
  {
    i = 0;
    sampled = ReadGraphicsPixel(page_number, probe_x, probe_y);
    run_end = height + 2;
    if (0 < run_end)
    {
      do
      {
        sampled_next = ReadGraphicsPixel(page_number, probe_x, probe_y);
        if (sampled_next != sampled)
        {
          break;
        }
        i = i + 1;
        probe_y = probe_y + (unsigned int)(height != -2);
      } while (i < run_end);
    }
    border_top = -1;
    if (run_end != i)
    {
      border_top = i;
    }
  }

  sprite = (EncodedImage *)g_sprite_blob_cursor;
  write_cursor = sprite->spans;
  last_nonempty_row_end = write_cursor;
  last_nonempty_line = -1;

  while (line_index < height)
  {
    ReadGraphicsScanline((unsigned int *)line_buffer, page_number, x, y + line_index, width);
    for (run_start = 0; run_start < width && line_buffer[run_start] == 0; run_start++)
    {
    }
    if (run_start != width)
    {
      break;
    }
    line_index = line_index + 1;
  }

  sprite->width = (unsigned short)width;
  sprite->height = (unsigned short)height;
  sprite->left_clip = (short)border_left;
  sprite->top_clip = (short)border_top;
  sprite->first_row = (short)line_index;
  sprite->row_count = (short)height - (short)line_index;
  for (; line_index < height; line_index = line_index + 1)
  {
    ReadGraphicsScanline((unsigned int *)line_buffer, page_number, x, y + line_index, width);

    for (run_start = 0; run_start < width && line_buffer[run_start] == 0; run_start++)
    {
    }
    if (run_start == width)
    {
      *write_cursor++ = 0xff;
    }
    else
    {
      run_end = (int)width;
      while (run_end > (int)run_start && line_buffer[run_end - 1] == 0)
      {
        run_end = run_end - 1;
      }

      run_len = (unsigned int)run_end - run_start;
      *write_cursor++ = (unsigned char)run_start;
      if (memchr(line_buffer + run_start, 0, run_len) == NULL)
      {
        *write_cursor++ = 0xfe;
      }
      *write_cursor++ = (unsigned char)run_len;
      memcpy(write_cursor, line_buffer + run_start, run_len);
      write_cursor += run_len;
      last_nonempty_row_end = write_cursor;
      last_nonempty_line = line_index;
    }
  }

  sprite->row_count =
      (last_nonempty_line < sprite->first_row) ? 0 : (short)(last_nonempty_line - sprite->first_row + 1);
  write_cursor = last_nonempty_row_end;
  g_sprite_blob_cursor = (int *)(((uintptr_t)write_cursor + 3) & ~(uintptr_t)3);
  *g_sprite_blob_cursor = -1;
  sprite->total_size = (int)((char *)g_sprite_blob_cursor - (char *)sprite);
  return sprite;
}

#pragma optimize("", on)
