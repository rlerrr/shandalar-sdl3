#ifdef MODERN_FIXES
#include "platform/winuser_compat.h"
#else
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>

#include "defs.h"
#include "shandalar.h"
#include "shandalar_internal.h"
#include "shandalar_global_strings.h"
#include "facemaker/src/facemaker_types.h"
#include "drawcardlib/src/pic.h"
#include "platform/legacy_io.h"


int DeckContainsCsvid(int csvid);
DWORD FormatMessageFromStringStripCarriageReturns(char *dst, DWORD max_length, LPCVOID format, ...);
char *AppendString(char *dst, char *src);
int MeasureTextSpanWidth(FacemakerWindowBounds *window, char *text, int length);

// FUNCTION: SHANDALAR 0x0041871f
void AppendTownHintBodyText(int hint_index)
{
  char hint_text[256];
  FILE *hints_file;

  hints_file = fopen("hints.txt", "rt");
  fseek(hints_file, g_hint_text_offsets[hint_index], 0);
  while (fgets(hint_text, sizeof(hint_text), hints_file) != NULL)
  {
    hint_text[strcspn(hint_text, "\r\n")] = '\0';
    if (hint_text[0] == '\0')
    {
      continue;
    }
    strcat(g_ui_message_buffer, hint_text);
    break;
  }
  fclose(hints_file);
}

// FUNCTION: SHANDALAR 0x00418a8f
char *FindTextWordBreak(char *text)
{
  while ((*text != '\0') && (*text == ' ') && (*text != '\n'))
  {
    text++;
  }

  while ((*text != '\0') && (*text != ' ') && (*text != '\n'))
  {
    text++;
  }

  if (*text != '\0')
  {
    return text;
  }
  else
  {
    return 0;
  }
}

#pragma intrinsic(memset)

// FUNCTION: SHANDALAR 0x00418b24
char *WrapTextToWidthForDropCap(char *src, char *dst, int max_width)
{
  char *word_end;
  int line_width;
  size_t word_length;
  char *scan;
  char current_line[256];

  line_width = 0;
  scan = src;
  current_line[0] = '\0';
  memset(&current_line[1], 0, 254);
  dst[0] = '\0';

  while ((word_end = FindTextWordBreak(scan)) != (char *)0)
  {
    word_length = (int)(word_end - scan);
    if (max_width < line_width + MeasureTextSpanWidth(g_page0_window_bounds, scan, word_length))
    {
      strcat(current_line, "\n");
      strcat(dst, current_line);
      current_line[0] = '\0';
      while ((scan != (char *)0) && (*scan == ' '))
      {
        scan++;
        word_length--;
      }
      strncat(current_line, scan, word_length);
      while ((*word_end != '\0') && (*word_end == '\n'))
      {
        strcat(dst, "\n");
        word_end++;
      }
      scan = word_end;
      line_width = MeasureTextSpanWidth(g_page0_window_bounds, scan, word_length);
    }
    else
    {
      line_width += MeasureTextSpanWidth(g_page0_window_bounds, scan, word_length);
      strncat(current_line, scan, word_length);
      if (*word_end == '\n')
      {
        strcat(dst, current_line);
      }
      while ((*word_end != '\0') && (*word_end == '\n'))
      {
        strcat(dst, "\n");
        word_end++;
        line_width = 0;
        current_line[0] = '\0';
      }
      scan = word_end;
    }
  }

  if (max_width < line_width + MeasureTextSpanWidth(g_page0_window_bounds, scan, word_length))
  {
    strcat(current_line, "\n");
    strcat(dst, current_line);
    strcat(dst, scan);
  }
  else
  {
    strcat(dst, current_line);
    strcat(dst, scan);
  }

  return dst;
}

// FUNCTION: SHANDALAR 0x00418e15
char *WrapTextToWidthForPopup(char *src, char *dst, int max_width)
{
  char *word_end;
  int line_width;
  size_t word_length;
  int done;
  char *scan;
  int word_width;
  char current_line[256];

  if (dst == (char *)0)
  {
    return 0;
  }
  if (src == (char *)0)
  {
    dst[0] = '\0';
    return dst;
  }
  dst[0] = '\0';
  scan = src;
  current_line[0] = '\0';
  line_width = 0;
  done = 0;

  while (done == 0)
  {
    if (*scan == '\0')
    {
      strcat(dst, current_line);
      done = 1;
    }
    else if (*scan == '\n')
    {
      strcat(current_line, "\n");
      strcat(dst, current_line);
      current_line[0] = '\0';
      line_width = 0;
      scan++;
    }
    else
    {
      word_end = scan;
      while (*word_end == ' ')
      {
        word_end++;
      }
      while ((*word_end != ' ') && (*word_end != '\n') && (*word_end != '\0'))
      {
        word_end++;
      }
      word_length = (int)(word_end - scan);
      word_width = MeasureTextSpanWidth(g_page0_window_bounds, scan, word_length);
      if (max_width < word_width + line_width)
      {
        strcat(current_line, "\n");
        strcat(dst, current_line);
        current_line[0] = '\0';
        line_width = 0;
        while (*scan == ' ')
        {
          scan++;
          word_length--;
        }
      }
      strncat(current_line, scan, word_length);
      line_width += word_width;
      scan = word_end;
    }
  }

  return dst;
}

// FUNCTION: SHANDALAR 0x0041905e
void ShowTownHintTextPopup(int hint_index)
{
  int i;
  EncodedImage *loaded_button_sprites[12];
  int card_name_index;
  int player_has_first_card;
  EncodedImage *button_sprites[10];
  char wrapped_hint_text[1000];
  EncodedImage *button_sprite_10;
  EncodedImage *button_sprite_11;

  if (g_hint_card_pairs[hint_index].second != -1)
  {
    player_has_first_card = (DeckContainsCsvid(g_hint_card_pairs[hint_index].first) != 0);
    FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer, 0x1000, gs_hinttext_0077e580[0],
                                                global_cards_data[FindCardIndexByCsvid((&g_hint_card_pairs[hint_index].first)[player_has_first_card])].name,
                                                global_cards_data[FindCardIndexByCsvid((&g_hint_card_pairs[hint_index].first)[player_has_first_card ^ 1])].name);
  }
  else
  {
    sprintf(g_ui_message_buffer, gs_hinttext_0077e580[1],
            global_cards_data[FindCardIndexByCsvid(g_hint_card_pairs[hint_index].first)].name);
  }

  AppendTownHintBodyText(hint_index);
  AppendString(g_ui_message_buffer, "\x94");
  ReadSpriteEntryPointers(loaded_button_sprites, "BuyButtons.spr");

  button_sprite_10 = loaded_button_sprites[0];
  button_sprite_11 = loaded_button_sprites[1];
  button_sprites[6] = loaded_button_sprites[2];
  for (i = 0; i < 3; i++)
  {
    button_sprites[7 + i] = loaded_button_sprites[3 + i];
  }
  for (i = 0; i < 3; i++)
  {
    button_sprites[3 + i] = loaded_button_sprites[6 + i];
  }
  for (i = 0; i < 3; i++)
  {
    button_sprites[i] = loaded_button_sprites[9 + i];
  }

  DrawEncodedImageResampled(g_page0_window_bounds,
                            ScaleUiCoordinate(0xdc),
                            ScaleUiCoordinate(0x34),
                            ScaleUiCoordinate(0xc5),
                            ScaleUiCoordinate(0x10f),
                            button_sprite_10);

  WrapTextToWidthForPopup(g_ui_message_buffer, wrapped_hint_text, ScaleUiCoordinate(0x85));
  DrawTextAt(g_page0_window_bounds, 0xfe, 0x140, 0xbc, "%s", wrapped_hint_text);
  WaitForInputEventUnlessBlocked();
  FreeSpriteBlob(button_sprite_10);
}
