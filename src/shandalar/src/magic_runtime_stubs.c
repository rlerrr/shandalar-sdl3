#define DUEL_ENGINE_IMPL

#include "defs.h"
#include "magic/src/duel_engine.h"
#include "magic/src/global_duel_ui_ids.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "cardartlib/CardArtLib.h"
#include "deckdll/src/deckdll.h"
#include "drawcardlib/Drawcardlib.h"
#include "drawcardlib/src/pic.h"
#include "facemaker/src/facemaker_types.h"
#include "platform/asset_manager.h"
#include "platform/gdi_compat.h"
#include "platform/legacy_io.h"
#include "platform/winuser_compat.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int g_network_result_value;
int g_network_result_packet_type;
int g_reveal_all_world_info;
int g_duel_selection_pending;
int g_showlibrary_menu_selection;
char global_cardart_path[0x110];
char global_duel_dat_path[0x110];
char szDeckName[0x104];
BITMAPINFO g_duel_backbuffer_bmi;

extern void LoadPcxResource(int page, int x, int y, char *path, void *palette);
extern void LoadPcxIntoPage(int page_number, char *path);
extern void FillGraphicsRect(FacemakerWindowBounds *window_bounds, int x, int y,
                             int width, int height, unsigned int color_index);
extern void BlitGraphicsRect(FacemakerWindowBounds *dst, unsigned int dst_x, int dst_y,
                             unsigned int width, DWORD height, FacemakerWindowBounds *src,
                             int src_x, int src_y);
extern void StretchBlitGraphicsRect(FacemakerWindowBounds *dst, int dst_x, int dst_y,
                                    int src_w, int src_h, FacemakerWindowBounds *src,
                                    int src_x, int src_y, int copy_w, int copy_h);
extern void DrawEncodedImageResampled(FacemakerWindowBounds *dst, int x, int y,
                                      int width, int height, EncodedImage *encoded_image);
extern int DrawTextFormattedVa(FacemakerWindowBounds *dst, int text_color, int draw_shadow,
                               int scale_to_screen, int center_x, int center_y,
                               int x, int y, char *format, va_list args);
extern DIBSurface *InitializeGraphicsSystem(int width, int height, int bits_per_pixel);
extern void *CreateGraphicsPage(int page_number, int width, int height, int bits_per_pixel);
extern void SetGraphicsPage(int page_number, void *page);
extern int LoadSystemFont(int font_id, unsigned int point_size, char *font_file,
                          char *font_name, int weight, int italic);
extern int ScaleUiCoordinate(int value);
extern int FileExists(const char *filename);
extern int RunNameEntryDialog(char *name_buffer);
extern void BeginSpriteEncodeSession(void);
extern EncodedImage *EncodeSpriteFromPage(int page_number, int x, int y, int width, int height);
extern void FinalizeSpriteEncodeSession(void);
extern void FreeSpriteBlob(void *memory);
extern void BuildFacemakerPortraitSprites(FacemakerWindowBounds *page);
extern int shandalar_host_run_facemaker(const char *asset_root, const char *writable_root);
extern void shandalar_host_begin_deckbuilder(void);
extern void shandalar_host_end_deckbuilder(void);
extern char global_base_directory[0x105];
extern int global_screen_width;
extern int global_screen_height;
extern int g_graphics_bpp;
extern DIBSurface *g_graphics_pages[10];
extern RpBitsPalettePacket g_palette_data_words;
extern FacemakerWindowBounds *g_page0_window_bounds;
extern FacemakerWindowBounds *g_page1_window_bounds;
extern FacemakerWindowBounds *g_page2_window_bounds;
extern FacemakerWindowBounds *g_page3_window_bounds;
extern FacemakerWindowBounds *g_page5_window_bounds;
extern int g_selected_wizard_color;
extern int g_name_entry_cursor;
extern int g_menu_selection_value;
extern int g_color_menu_selection_to_deck_color[5];
extern DIBSurface *g_facemaker_page4_dib;
extern HBITMAP g_facemaker_page4_bitmap;
extern card_data_t global_cards_data[];
static EncodedImage *g_name_entry_portrait_sprite;
static int g_deckbuilder_attached;
static int g_card_libraries_initialized;

int internal_rand(int maximum);
int CardIDFromType(unsigned int type);
int CardTypeFromID(int csvid);
int CardInDeck(int deck_entry);
void SetCardInDeck(int deck_index, int enabled);
int SellPrice(int card_index);
int is_card_available_in_expansion(unsigned int card_id, int expansion);
unsigned int HasExpansion(unsigned char expansion_mask);

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

void TENTATIVE_wait_for_network_result(int player, int packet_type)
{
  (void)player;
  g_network_result_packet_type = packet_type;
}

void TENTATIVE_send_network_result(int player, int packet_type)
{
  (void)player;
  g_network_result_packet_type = packet_type;
}

DWORD WINAPI RunDuelEngineThreadProc(LPVOID creature_type)
{
  (void)creature_type;
  return 1;
}

void StopWorldLocationMusic(void)
{
}

void DrawTextAt(FacemakerWindowBounds *window, int color, int x, int y, char *format, ...)
{
  va_list args;
  va_start(args, format);
  DrawTextFormattedVa(window, color, 1, 1, 1, 1, x, y, format, args);
  va_end(args);
}

int DrawRandomCardFromInitialLibrary(int library_index)
{
  struct
  {
    int rand_pick;    // ebp-0x14
    int chosen_csvid; // ebp-0x10
    int total_cards;  // ebp-0xc
    int running_pick; // ebp-0x8
    int i;            // ebp-0x4
  } s;

  if (library_index == -1)
  {
    return -1;
  }

  s.total_cards = 0;
  for (s.i = 0; s.i < 200; ++s.i)
  {
    s.total_cards += g_initial_library[library_index][s.i].numcards;
  }

  if (s.total_cards == 0)
  {
    return -1;
  }

  s.rand_pick = internal_rand(s.total_cards);
  s.running_pick = s.rand_pick;

  for (s.i = 0; s.i < 200; ++s.i)
  {
    s.running_pick -= g_initial_library[library_index][s.i].numcards;
    if (s.running_pick < 0)
    {
      s.chosen_csvid = g_initial_library[library_index][s.i].csvid;
      if (g_duel_ai_mode_state != 1)
      {
        --g_initial_library[library_index][s.i].numcards;
      }
      break;
    }
  }

  for (s.i = 0; s.i < g_card_count; ++s.i)
  {
    if ((int)global_cards_data[s.i].id == s.chosen_csvid)
    {
      return s.i;
    }
  }

  return -1;
}

int show_cardlist(int *graveyard, int *card_counts, int *available, int count,
                  void *window_title, unsigned int require_selection,
                  char *prompt)
{
  (void)card_counts;
  (void)available;
  (void)window_title;
  (void)require_selection;
  (void)prompt;
  return (graveyard && count > 0) ? 0 : -1;
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

void record_ai_action_selection(void)
{
}

void replay_ai_action_selection(void)
{
}

int create_fonts(void)
{
  return 1;
}

void destroy_create_fonts_resources(void)
{
}

int internal_rand(int maximum)
{
  return maximum > 0 ? rand() % maximum : 0;
}

int ClampIntToRange(int value, int min_value, int max_value)
{
  if (value < min_value)
    return min_value;
  if (value > max_value)
    return max_value;
  return value;
}

int SeedRandomFromTickCount(void)
{
  unsigned int seed = (unsigned int)time(NULL);
  srand(seed);
  return (int)seed;
}

int AddCardToDeckSorted(int card_id)
{
  int entry_key;
  int candidate_key;
  int card_index;
  int slot;
  int deck_full;

  entry_key = (int)global_cards_data[card_id].color * 0x20 +
              (int)global_cards_data[card_id].type * 0x100 +
              (int)(unsigned char)global_cards_data[card_id].name[0];
  deck_full = 1;
  for (slot = 0; slot < 500; slot = slot + 1)
  {
    if (deck[slot] == -1)
    {
      deck_full = 0;
    }
  }
  if (deck_full != 0)
  {
    return -1;
  }
  for (slot = 0x1f2; slot >= 0; slot = slot - 1)
  {
    if (deck[slot] == -1)
    {
      continue;
    }
    card_index = deck[slot] & 0xfff;
    candidate_key = (int)global_cards_data[card_index].color * 0x20 +
                    (int)global_cards_data[card_index].type * 0x100 +
                    (int)(unsigned char)global_cards_data[card_index].name[0];
    if (candidate_key >= entry_key)
    {
      deck[slot + 1] = card_id;
      return slot + 1;
    }
    deck[slot + 1] = (unsigned int)deck[slot];
  }
  deck[0] = card_id;
  return 0;
}

int GetCardRarity(int card_id)
{
  if (card_id < 0 || card_id >= 2000)
    return 0;
  return ClampIntToRange((int)(signed char)global_cards_data[card_id].rarity, 0, 4);
}

char *GetCreatureName(int creature_type)
{
  if (creature_type < 0 || creature_type >= 0x39)
    return "";
  return g_shandalar_monster_definitions[creature_type].name;
}

int CountDuelPoolEligibleTowns(void)
{
  int count = 0;
  int index;
  for (index = 0; index < 128; ++index)
    if (g_town_slots[index].location_type > 1 && g_town_slots[index].location_type != 4)
      ++count;
  return count;
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
  if (!InitializeGraphicsSystem(global_screen_width, global_screen_height, 8))
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

color_t single_color_test_bit_to_color_t(color_test_t color_test)
{
  int color;
  for (color = 0; color < 8; ++color)
    if (((int)color_test & (1 << color)) != 0)
      return (color_t)color;
  return COLOR_COLORLESS;
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
  deck_set_host_pump_hooks(shandalar_host_begin_deckbuilder,
                           shandalar_host_end_deckbuilder);
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
