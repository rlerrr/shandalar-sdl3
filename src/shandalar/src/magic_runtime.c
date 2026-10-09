#include "defs.h"
#include "magic/src/duel_engine.h"
#include "magic/src/global_duel_ui_ids.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "magic/src/shandalar_deck_rules.h"
#include "magic/src/network.h"
#include "cardartlib/CardArtLib.h"
#include "deckdll/src/deckdll.h"
#include "drawcardlib/Drawcardlib.h"
#include "drawcardlib/src/pic.h"
#include "facemaker/src/facemaker_types.h"
#include "platform/asset_manager.h"
#include "platform/gdi_compat.h"
#include "platform/legacy_io.h"
#include "platform/winuser_compat.h"
#include "shandalar_internal.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char global_cardart_path[0x110];
char global_duel_dat_path[0x110];
char szDeckName[0x104];

extern int DrawTextFormattedVa(FacemakerWindowBounds *dst, int text_color, int draw_shadow,
                               int scale_to_screen, int center_x, int center_y,
                               int x, int y, char *format, va_list args);
extern DIBSurface *InitializeGraphicsSystem(int width, int height, int bits_per_pixel);
extern int RunNameEntryDialog(char *name_buffer);
extern int shandalar_host_run_facemaker(const char *asset_root, const char *writable_root);
extern void shandalar_host_begin_deckbuilder(void);
extern void shandalar_host_end_deckbuilder(void);
extern void deck_register_dialog_resources(void);
extern void duel_register_dialog_resources(void);
extern int register_MAGICGAME_FullCardClass(LPCSTR class_name);
extern int pick_internal_card_from_list_dialog_for_owner(
    HWND owner, char *prompt, int initial_card_id, int filter);
extern char global_base_directory[0x105];
extern int g_selected_wizard_color;
extern int g_name_entry_cursor;
static EncodedImage *g_name_entry_portrait_sprite;
static int g_deckbuilder_attached;
static int g_card_libraries_initialized;

static int shandalar_deck_minimum_size(void)
{
  return shandalar_minimum_deck_size(g_shandalar_difficulty);
}

int CardIDFromType(unsigned int type);
int CardTypeFromID(int csvid);
int CardInDeck(int deck_entry);
void SetCardInDeck(int deck_index, int enabled);
int SellPrice(int card_index);
int is_card_available_in_expansion(unsigned int card_id, int expansion);
unsigned int HasExpansion(unsigned char expansion_mask);
int deckdll_create_fonts(void);
void deckdll_destroy_create_fonts_resources(void);
color_t single_color_test_bit_to_color_t(color_test_t color_mask);

static int shandalar_pick_debug_deck_card(HWND owner)
{
  register_MAGICGAME_FullCardClass(CLASS_MAGICGAME_FULL_CARD);
  return pick_internal_card_from_list_dialog_for_owner(
      owner, "Add a card to your deck", -1, -1);
}

static int shandalar_deck_card_sort_key(int internal_card_id)
{
  if (internal_card_id < 0 || internal_card_id >= g_card_count)
    return -1;

  return single_color_test_bit_to_color_t(
             (color_test_t)global_cards_data[internal_card_id].color) * 0x20 +
         (unsigned int)global_cards_data[internal_card_id].type * 0x100 +
         (unsigned char)global_cards_data[internal_card_id].name[0];
}

#ifdef __EMSCRIPTEN__
void __cdecl _assert(void *expression, void *filename, unsigned line)
{
  fprintf(stderr, "Assertion failed: %s, file %s, line %u\n",
          expression ? (const char *)expression : "",
          filename ? (const char *)filename : "", line);
  abort();
}
#endif

static int shandalar_register_deckbuilder_exports(void)
{
  int ok = 1;
  ok &= shandalar_win32_register_proc("CardIDFromType", (FARPROC)CardIDFromType);
  ok &= shandalar_win32_register_proc("CardTypeFromID", (FARPROC)CardTypeFromID);
  ok &= shandalar_win32_register_proc("CardInDeck", (FARPROC)CardInDeck);
  ok &= shandalar_win32_register_proc("SetCardInDeck", (FARPROC)SetCardInDeck);
  ok &= shandalar_win32_register_proc("SellPrice", (FARPROC)SellPrice);
  ok &= shandalar_win32_register_proc("deck", (FARPROC)deck);
  ok &= shandalar_win32_register_proc("Gold", (FARPROC)&Gold);
  ok &= shandalar_win32_register_proc("Scards", (FARPROC)Scards);
  ok &= shandalar_win32_register_proc("szDeckName", (FARPROC)szDeckName);
  ok &= shandalar_win32_register_proc("_currentDeck", (FARPROC)&_currentDeck);
  return ok;
}

void DrawTextAt(FacemakerWindowBounds *window, int color, int x, int y, char *format, ...)
{
  va_list args;
  va_start(args, format);
  DrawTextFormattedVa(window, color, 1, 1, 1, 1, x, y, format, args);
  va_end(args);
}

int is_card_available_in_installed_expansion(unsigned int card_id)
{
  if (HasExpansion(1) != 0 && is_card_available_in_expansion(card_id, 0) != 0)
    return 1;
  if (HasExpansion(2) != 0 && is_card_available_in_expansion(card_id, 1) != 0)
    return 1;
  if (HasExpansion(4) != 0 && is_card_available_in_expansion(card_id, 2) != 0)
    return 1;
  return 0;
}

int create_fonts(void)
{
  return deckdll_create_fonts();
}

void destroy_create_fonts_resources(void)
{
  deckdll_destroy_create_fonts_resources();
}

void ShandalarLoadPcxIntoPageNoPalette(char *path)
{
  LoadPcxResource(-1, 0, 0, path, (void *)1);
}

static void free_detached_facemaker_page4(void)
{
  if (!g_facemaker_page4_dib)
    return;
  if (g_graphics_pages[4] == g_facemaker_page4_dib)
    g_graphics_pages[4] = NULL;
  SelectObject(g_facemaker_page4_dib->hTempDC, g_facemaker_page4_dib->hPreviousBitmap);
  DeleteObject(g_facemaker_page4_dib->hBitmap);
  DeleteDC(g_facemaker_page4_dib->hTempDC);
  free(g_facemaker_page4_dib->pBitmapInfo);
  free(g_facemaker_page4_dib);
  g_facemaker_page4_dib = NULL;
  g_facemaker_page4_bitmap = NULL;
}

static int restore_shandalar_opening_graphics(void)
{
  void *page;
  int page_number;

  global_screen_width = 0x400;
  global_screen_height = 0x300;

  if (!InitializeGraphicsSystem(global_screen_width, global_screen_height, -1))
    return 0;

  for (page_number = 1; page_number < 3; ++page_number) {
    if (page_number == 1 && g_graphics_pages[0]->width <= 0x400)
      page = CreateGraphicsPage(page_number, 0x400, 800, 8);
    else
      page = CreateGraphicsPage(page_number, global_screen_width, global_screen_height + 1, 8);
    SetGraphicsPage(page_number, page);
  }

  SetGraphicsPage(5, CreateGraphicsPage(5, ScaleUiCoordinate(0x40), ScaleUiCoordinate(0x148), 8));
  SetGraphicsPage(3, CreateGraphicsPage(3, ScaleUiCoordinate(0x280),
                                        (ScaleUiCoordinate(0x1e0) - ScaleUiCoordinate(0x148)) + 3, 8));

  g_page3_window_bounds->max_x = ScaleUiCoordinate(0x280);
  g_page2_window_bounds->max_x = g_page3_window_bounds->max_x;
  g_page1_window_bounds->max_x = g_page2_window_bounds->max_x;
  g_page0_window_bounds->max_x = g_page1_window_bounds->max_x;
  g_page2_window_bounds->max_x = ScaleUiCoordinate(0x1e0);
  g_page1_window_bounds->max_y = g_page2_window_bounds->max_x;
  g_page0_window_bounds->max_y = g_page1_window_bounds->max_y;
  g_page3_window_bounds->max_y = ScaleUiCoordinate(0x1e0) - ScaleUiCoordinate(0x148);
  g_page5_window_bounds->max_y = ScaleUiCoordinate(0x148);
  g_page5_window_bounds->max_x = ScaleUiCoordinate(0x40);

  LoadSystemFont(1, ScaleUiCoordinate(9), "tt0530m_.ttf", "Benguiat BkCn BT", 100, 0);
  LoadSystemFont(2, 10, "tt0298m_.ttf", "MPZurich Cn BT", 400, 0);
  LoadSystemFont(4, ScaleUiCoordinate(0xb), "tt0530m_.ttf", "Benguiat BkCn BT", 100, 0);
  LoadSystemFont(5, ScaleUiCoordinate(0x14), "tt0127m_.ttf", "Benguiat Bk BT", 700, 1);
  LoadSystemFont(6, ScaleUiCoordinate(0x1c), "tt0530m_.ttf", "Benguiat BkCn BT", 100, 0);
  LoadSystemFont(7, ScaleUiCoordinate(0xb), "tt0530m_.ttf", "Benguiat BkCn BT", 100, 0);
  g_page0_window_bounds->font_slot = 1;
  return 1;
}

static int load_facemaker_portrait_output(void)
{
  FacemakerWindowBounds page4_bounds = {4, 0, 0, 275, 169, 1, 0x0f, 4, 0};

  if (!FileExists("face.pic"))
    return 0;

  free_detached_facemaker_page4();
  if (!CreateGraphicsPage(4, 276, 170, 8))
    return 0;

  FillGraphicsRect(&page4_bounds, 0, 0, 276, 170, 0);
  LoadPcxIntoPage(4, "face.pic");
  BuildFacemakerPortraitSprites(&page4_bounds);
  LoadPcxIntoPage(4, "face.pic");
  if (g_name_entry_portrait_sprite)
    FreeSpriteBlob(g_name_entry_portrait_sprite);
  BeginSpriteEncodeSession();
  g_name_entry_portrait_sprite = EncodeSpriteFromPage(4, 0, 0, 0x89, 0xa9);
  FinalizeSpriteEncodeSession();

  g_facemaker_page4_bitmap = g_graphics_pages[4]->hBitmap;
  g_facemaker_page4_dib = g_graphics_pages[4];
  SelectObject(g_graphics_pages[4]->hTempDC, g_graphics_pages[4]->hPreviousBitmap);
  g_graphics_pages[4] = NULL;
  return 1;
}

static void prepare_name_entry_background(void)
{
  static char *pedestal_images[6] = {
      NULL, "prdblk.pic", "prdblu.pic", "prdgrn.pic", "prdrd.pic", "prdwt.pic"};
  int selected_color;
  char *pedestal_path;

  selected_color = g_selected_wizard_color;
  if ((selected_color < 1 || selected_color > 5) &&
      g_menu_selection_value >= 1 && g_menu_selection_value <= 5)
    selected_color = g_color_menu_selection_to_deck_color[g_menu_selection_value - 1];
  if (selected_color < 1 || selected_color > 5)
    selected_color = 1;

  pedestal_path = pedestal_images[selected_color];
  ShandalarLoadPcxIntoPageNoPalette("pedstls.pic");
  LoadPcxResource(1, global_screen_width - 0x280, global_screen_height - 0x1e0,
                  pedestal_path, (g_graphics_bpp == 8) ? &g_palette_data_words : (void *)1);
  StretchBlitGraphicsRect(g_page1_window_bounds, global_screen_width - 0x280,
                          global_screen_height - 0x1e0, 0x280, 0x1e0,
                          g_page1_window_bounds, 0, 0, global_screen_width,
                          global_screen_height);
  if (g_name_entry_portrait_sprite)
    DrawEncodedImageResampled(g_page1_window_bounds,
                              global_screen_width / 2 - ScaleUiCoordinate(0x89),
                              ScaleUiCoordinate(0x14),
                              ScaleUiCoordinate(0x112),
                              ScaleUiCoordinate(0x152),
                              g_name_entry_portrait_sprite);
  BlitGraphicsRect(g_page1_window_bounds, 0, 0, global_screen_width,
                   global_screen_height, g_page0_window_bounds, 0, 0);
}

int shandalar_run_facemaker_flow(void)
{
  char asset_root[sizeof(global_base_directory)];
  int completed;

  snprintf(asset_root, sizeof(asset_root), "%s", global_base_directory);
  completed = shandalar_host_run_facemaker(asset_root, asset_root);
  if (!restore_shandalar_opening_graphics())
    return -1;
  if (!completed)
    return -1;
  load_facemaker_portrait_output();
  g_name_entry_cursor = (int)strlen(g_name_entry_buffer);
  prepare_name_entry_background();
  g_page0_window_bounds->font_slot = 1;
  if (RunNameEntryDialog(g_name_entry_buffer) == -1)
    return -1;
  strcpy(g_player_name, g_name_entry_buffer);
  g_player_is_male = 1;
  return g_player_is_male;
}

int shandalar_runtime_init(const char *asset_root)
{
  char module_path[512];
  const char *root = (asset_root && asset_root[0]) ? asset_root : ".";
  shandalar_assets_set_root(root);
  snprintf(global_base_directory, 0x105, "%s", root);
  _chdir(global_base_directory);
  snprintf(module_path, sizeof(module_path), "%s/shandalar.exe", root);
  shandalar_win32_set_module_path(module_path);
  shandalar_set_default_font_root(root);
  if (!g_card_libraries_initialized) {
    if (!CardArtLib_InitializeFromRoot(root))
      return 0;
    if (!DrawCardLib_InitializeFromRoot(root)) {
      CardArtLib_Shutdown();
      return 0;
    }
    g_card_libraries_initialized = 1;
  }
  if (!shandalar_register_deckbuilder_exports())
    return 0;
  deck_register_dialog_resources();
  duel_register_dialog_resources();
  deck_set_host_pump_hooks(shandalar_host_begin_deckbuilder,
                          shandalar_host_end_deckbuilder);
  deck_set_minimum_deck_size_hook(shandalar_deck_minimum_size);
  deck_set_debug_card_picker(shandalar_pick_debug_deck_card,
                             shandalar_deck_card_sort_key);
  if (!g_deckbuilder_attached) {
    if (!DllMain((HINSTANCE)1, DLL_PROCESS_ATTACH, NULL))
      return 0;
    g_deckbuilder_attached = 1;
  }
  return 1;
}

void shandalar_runtime_shutdown(void)
{
  if (g_deckbuilder_attached) {
    DllMain((HINSTANCE)1, DLL_PROCESS_DETACH, NULL);
    g_deckbuilder_attached = 0;
  }
  if (g_card_libraries_initialized) {
    DrawCardLib_Shutdown();
    CardArtLib_Shutdown();
    g_card_libraries_initialized = 0;
  }
}
