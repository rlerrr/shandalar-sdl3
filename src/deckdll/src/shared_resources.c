#ifdef SHANDALAR_SDL_WIN32_COMPAT
#include "platform/winuser_compat.h"
#else
#include <windows.h>
#endif
#include <stdio.h>
#include "mystdbool.h"
#include "cardartlib/src/assert.h"
#include "cardartlib/src/palette.h"
#include "drawcardlib/src/pic.h"

#define SHARED_RESOURCES_IMPL
#include "shared_resources.h"

// GLOBAL: DECKDLL 0x10035b48
// GLOBAL: MAGIC 0x00571d28
// GLOBAL: SHANDALAR 0x00585930
static LOGFONT global_font_template_ini = {
    0,                           // lfHeight
    0,                           // lfWidth
    0,                           // lfEscapement
    0,                           // lfOrientation
    FW_NORMAL,                   // lfWeight
    FALSE,                       // lfItalic
    FALSE,                       // lfUnderline
    FALSE,                       // lfStrikeOut
    ANSI_CHARSET,                // lfCharSet
    OUT_DEFAULT_PRECIS,          // lfOutPrecision
    CLIP_DEFAULT_PRECIS,         // lfClipPrecision
    DEFAULT_QUALITY,             // lfQuality
    DEFAULT_PITCH | FF_DONTCARE, // lfPitchAndFamily
    {0}                          // lfFaceName[LF_FACESIZE]
};

// FUNCTION: DECKDLL 0x10025727
// FUNCTION: MAGIC 0x00495e95
// FUNCTION: SHANDALAR 0x00466928
LOGFONT *LoadFontFromIni(char *name, int italic)
{
  char key[100]; /* ebp - 0x68 */
  UINT bold;     /* ebp - 0x4 */

  memcpy(&global_font_from_ini, &global_font_template_ini, sizeof(LOGFONT));

  strcpy(key, "size");
  strcat(key, name);
  global_font_from_ini.lfHeight = GetPrivateProfileIntA("Fonts", key, 0x14, global_duel_dat_path);

  strcpy(key, "bold");
  strcat(key, name);
  bold = GetPrivateProfileIntA("Fonts", key, 0, global_duel_dat_path);
  if (bold != 0)
    global_font_from_ini.lfWeight = FW_BOLD;

  if (italic != 0)
    global_font_from_ini.lfItalic = TRUE;

  strcpy(key, "font");
  strcat(key, name);
  GetPrivateProfileStringA("Fonts", key, "MS Sans Serif",
                           global_font_from_ini.lfFaceName, 0x20, global_duel_dat_path);
  return &global_font_from_ini;
}

// FUNCTION: DECKDLL 0x10011680
// FUNCTION: MAGIC 0x00558fb0
// FUNCTION: SHANDALAR 0x0056f300
bool create_fonts(void)
{
  char path[MAX_PATH + 4]; /* big buffer first */
  int debug_pen_index;     /* ebp - 0x8 */
  int debug_pens_ok;       /* ebp - 0x4 */

  strcpy(path, global_base_directory);
  strcat(path, "\\Tt0530m_.TTF");
  AddFontResourceA(path);
  strcpy(path, global_base_directory);
  strcat(path, "\\Magis___.TTF");
  AddFontResourceA(path);
  strcpy(path, global_base_directory);
  strcat(path, "\\Tt0127m_.TTF");
  AddFontResourceA(path);
  strcpy(path, global_base_directory);
  strcat(path, "\\Tt0085m_.TTF");
  AddFontResourceA(path);
  strcpy(path, global_base_directory);
  strcat(path, "\\Tt0298m_.TTF");
  AddFontResourceA(path);
  strcpy(path, global_base_directory);
  strcat(path, "\\Tt0299m_.TTF");
  AddFontResourceA(path);
  strcpy(path, global_base_directory);
  strcat(path, "\\Tt0300m_.TTF");
  AddFontResourceA(path);

  global_fonts_init_state = 3;

  sprintf(path, "%s\\Damage.pic", global_cardart_path);
  global_damage_pic = load_pic(path);
  sprintf(path, "%s\\CardCounters.pic", global_cardart_path);
  global_card_counters_pic = load_pic(path);
  sprintf(path, "%s\\ManaSymbols.pic", global_cardart_path);
  global_mana_symbols_pic = load_pic(path);
  sprintf(path, "%s\\Abilities.pic", global_cardart_path);
  global_abilities_pic = load_pic(path);
  sprintf(path, "%s\\ManaStripes.pic", global_cardart_path);
  global_mana_stripes_pic = load_pic(path);
  sprintf(path, "%s\\Summon.pic", global_cardart_path);
  global_summon_pic = load_pic(path);
  sprintf(path, "%s\\Dying.pic", global_cardart_path);
  global_dying_pic = load_pic(path);
  sprintf(path, "%s\\Target.pic", global_cardart_path);
  global_target_pic = load_pic(path);
  sprintf(path, "%s\\CantTarget.pic", global_cardart_path);
  global_cant_target_pic = load_pic(path);
  sprintf(path, "%s\\WillUntap.pic", global_cardart_path);
  global_will_untap_pic = load_pic(path);

  global_smallcard_pt_font = CreateFontIndirectA(LoadFontFromIni("SmallCardPT", 0));
  global_damage_font = CreateFontIndirectA(LoadFontFromIni("Damage", 0));
  global_idtag_font = CreateFontIndirectA(LoadFontFromIni("IDTag", 0));

  global_palette_col_c9 = GetPaletteColor(0xc9);
  global_palette_col_9e_a = GetPaletteColor(0x9e);
  global_palette_col_9e_b = GetPaletteColor(0x9e);
  global_palette_col_7c = GetPaletteColor(0x7c);
  global_palette_col_2f = GetPaletteColor(0x2f);
  global_palette_col_bf = GetPaletteColor(0xbf);
  global_palette_col_5d = GetPaletteColor(0x5d);
  global_palette_col_1f = GetPaletteColor(0x1f);
  global_pen_palette_5d = CreatePen(6, 2, global_palette_col_5d);
  global_pen_palette_1f = CreatePen(6, 2, global_palette_col_1f);

  for (debug_pen_index = 0, debug_pens_ok = 1; debug_pen_index < 10; ++debug_pen_index)
  {
    global_debug_pens[debug_pen_index] =
        CreatePen(6, 3,
                  ((unsigned int)(BYTE)(debug_pen_index * 75) << 16) |
                      ((unsigned int)(BYTE)(debug_pen_index * 20) << 8) |
                      (unsigned int)(BYTE)(debug_pen_index * 10));
    if (global_debug_pens[debug_pen_index] == (HPEN)0)
      debug_pens_ok = 0;
  }

  if (!global_damage_pic || !global_card_counters_pic || !global_mana_symbols_pic ||
      !global_abilities_pic || !global_mana_stripes_pic || !global_summon_pic ||
      !global_dying_pic || !global_target_pic || !global_cant_target_pic ||
      !global_will_untap_pic || !global_smallcard_pt_font || !global_damage_font ||
      !global_idtag_font || !global_pen_palette_5d || !global_pen_palette_1f || !debug_pens_ok)
  {
    destroy_create_fonts_resources();
    return false;
  }

  return true;
}

// FUNCTION: DECKDLL 0x10011c41
// FUNCTION: MAGIC 0x00559571
// FUNCTION: SHANDALAR 0x0056f8c1
void destroy_create_fonts_resources(void)
{
  char path[MAX_PATH + 4];
  int i;

  strcpy(path, global_base_directory);
  strcat(path, "\\Magim___.TTF");
  RemoveFontResourceA(path);
  strcpy(path, global_base_directory);
  strcat(path, "\\Tt0530m_.TTF");
  RemoveFontResourceA(path);
  strcpy(path, global_base_directory);
  strcat(path, "\\Magis___.TTF");
  RemoveFontResourceA(path);
  strcpy(path, global_base_directory);
  strcat(path, "\\Tt0127m_.TTF");
  RemoveFontResourceA(path);
  strcpy(path, global_base_directory);
  strcat(path, "\\Tt0085m_.TTF");
  RemoveFontResourceA(path);
  strcpy(path, global_base_directory);
  strcat(path, "\\Tt0298m_.TTF");
  RemoveFontResourceA(path);
  strcpy(path, global_base_directory);
  strcat(path, "\\Tt0299m_.TTF");
  RemoveFontResourceA(path);
  strcpy(path, global_base_directory);
  strcat(path, "\\Tt0300m_.TTF");
  RemoveFontResourceA(path);

  if (global_damage_pic)
  {
    delete_and_close_object(global_damage_pic);
    global_damage_pic = NULL;
  }
  if (global_card_counters_pic)
  {
    delete_and_close_object(global_card_counters_pic);
    global_card_counters_pic = NULL;
  }
  if (global_mana_symbols_pic)
  {
    delete_and_close_object(global_mana_symbols_pic);
    global_mana_symbols_pic = NULL;
  }
  if (global_abilities_pic)
  {
    delete_and_close_object(global_abilities_pic);
    global_abilities_pic = NULL;
  }
  if (global_mana_stripes_pic)
  {
    delete_and_close_object(global_mana_stripes_pic);
    global_mana_stripes_pic = NULL;
  }
  if (global_summon_pic)
  {
    delete_and_close_object(global_summon_pic);
    global_summon_pic = NULL;
  }
  if (global_dying_pic)
  {
    delete_and_close_object(global_dying_pic);
    global_dying_pic = NULL;
  }
  if (global_target_pic)
  {
    delete_and_close_object(global_target_pic);
    global_target_pic = NULL;
  }
  if (global_cant_target_pic)
  {
    delete_and_close_object(global_cant_target_pic);
    global_cant_target_pic = NULL;
  }
  if (global_will_untap_pic)
  {
    delete_and_close_object(global_will_untap_pic);
    global_will_untap_pic = NULL;
  }

  if (global_smallcard_pt_font)
    DeleteObject(global_smallcard_pt_font);
  global_smallcard_pt_font = NULL;
  if (global_damage_font)
    DeleteObject(global_damage_font);
  global_damage_font = NULL;
  if (global_idtag_font)
    DeleteObject(global_idtag_font);
  global_idtag_font = NULL;
  if (global_pen_palette_5d)
    DeleteObject(global_pen_palette_5d);
  global_pen_palette_5d = NULL;
  if (global_pen_palette_1f)
    DeleteObject(global_pen_palette_1f);
  global_pen_palette_1f = NULL;

  for (i = 0; i < 10; i++)
    if (global_debug_pens[i])
    {
      DeleteObject(global_debug_pens[i]);
      global_debug_pens[i] = NULL;
    }
}

// GLOBAL: DECKDLL 0x10035b3c
// GLOBAL: MAGIC 0x00571d1c
// GLOBAL: SHANDALAR 0x00585924
static int global_deleteobj_platform_id = -1;

#pragma intrinsic(memset)

// FUNCTION: DECKDLL 0x1002417b
// FUNCTION: MAGIC 0x004948e1
// FUNCTION: SHANDALAR 0x0046537b
void delete_and_close_object(HANDLE obj)
{
  char dbg_buf[500];
  OSVERSIONINFOA ver;
  HANDLE section_handle;
  DIBSECTION dib_section;
  char *bits_pointer;

  if (global_deleteobj_platform_id == -1)
  {
    ver.dwOSVersionInfoSize = 0x94;
    memset(&ver.dwMajorVersion, 0, 0x90);
    GetVersionExA(&ver);
    if (ver.dwPlatformId == 1)
      global_deleteobj_platform_id = 1;
    else
      global_deleteobj_platform_id = 0;
  }

  if (obj != (HANDLE)0)
  {
    GetObjectA(obj, sizeof(dib_section), &dib_section);
    section_handle = dib_section.dshSection;
    bits_pointer = (char *)dib_section.dsBm.bmBits;
    bits_pointer += dib_section.dsOffset;
    DeleteObject(obj);
    if (section_handle != (HANDLE)0 && global_deleteobj_platform_id != 0)
      CloseHandle(section_handle);
  }

  if (global_DIB_debug != 0)
  {
    sprintf(dbg_buf, "  %08x DestroyDIBSection (file mapping: %08x)\n", obj, section_handle);
    OutputDebugStringA(dbg_buf);
  }
}

#pragma function(memset)
