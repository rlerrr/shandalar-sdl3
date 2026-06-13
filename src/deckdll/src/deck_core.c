#include "deck_core.h"

#include "card_db.h"
#include "cardartlib/CardArtLib.h"
#include "deck_file.h"
#include "drm.h"
#include "drawcardlib/Drawcardlib.h"
#include "drawcardlib/src/pic.h"
#include "cardartlib/src/palette.h"
#include "platform/asset_manager.h"
#include "platform/gdi_compat.h"

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#define DECK_BASE_WIDTH 1024
#define DECK_BASE_HEIGHT 768
#define DECK_TITLE_X 8
#define DECK_TITLE_Y 10
#define DECK_TITLE_W 288
#define DECK_TITLE_H 72
#define DECK_FULLCARD_X 8
#define DECK_FULLCARD_Y 92
#define DECK_FULLCARD_W 288
#define DECK_FULLCARD_H 432
#define DECK_SURFACE_X 304
#define DECK_SURFACE_Y 0
#define DECK_SURFACE_W 720
#define DECK_SURFACE_H 524
#define DECK_FILTER_X 8
#define DECK_FILTER_Y 566
#define DECK_FILTER_W 1008
#define DECK_FILTER_H 44
#define DECK_HORZLIST_X 8
#define DECK_HORZLIST_Y 610
#define DECK_HORZLIST_W 1008
#define DECK_HORZLIST_H 148
#define DECK_BUTTON_Y 527
#define DECK_BUTTON_H 36
#define DECK_CARD_W 99
#define DECK_CARD_H 99
#define DECK_BROWSER_PAGE_SIZE 7
#define DECK_SURFACE_CARD_CACHE_SIZE 96

typedef enum DeckFilterCommand {
  DECK_FILTER_ALL,
  DECK_FILTER_BLACK,
  DECK_FILTER_BLUE,
  DECK_FILTER_GREEN,
  DECK_FILTER_RED,
  DECK_FILTER_WHITE,
  DECK_FILTER_LAND,
  DECK_FILTER_CREATURE,
  DECK_FILTER_ENCHANTMENT,
  DECK_FILTER_INSTANT,
  DECK_FILTER_INTERRUPT,
  DECK_FILTER_SORCERY,
  DECK_FILTER_ARTIFACT,
  DECK_FILTER_FOURTH,
  DECK_FILTER_ASTRAL,
  DECK_FILTER_ARABIAN,
  DECK_FILTER_ANTIQUITIES,
  DECK_FILTER_LEGENDS,
  DECK_FILTER_DARK,
  DECK_FILTER_SAVE,
  DECK_FILTER_LOAD,
  DECK_FILTER_CLEAR
} DeckFilterCommand;

typedef enum DeckMainCommand {
  DECK_MAIN_STATS,
  DECK_MAIN_INFO,
  DECK_MAIN_EXIT
} DeckMainCommand;

typedef struct DeckFilterButton {
  const char *label;
  int width;
  DeckFilterCommand command;
  const char *asset_name;
} DeckFilterButton;

typedef struct DeckMainButton {
  const char *label;
  int left;
  int right;
  DeckMainCommand command;
} DeckMainButton;

static const DeckFilterButton k_filter_buttons[] = {
  { "ALL", 34, DECK_FILTER_ALL, "DBArt/all-butn.pic" },
  { "BLACK", 34, DECK_FILTER_BLACK, "DBArt/GREY.PIC" },
  { "BLUE", 34, DECK_FILTER_BLUE, "DBArt/BLUE.PIC" },
  { "GREEN", 34, DECK_FILTER_GREEN, "DBArt/GREEN.PIC" },
  { "RED", 34, DECK_FILTER_RED, "DBArt/RED.PIC" },
  { "WHITE", 34, DECK_FILTER_WHITE, "DBArt/YELLOW.PIC" },
  { "LAND", 34, DECK_FILTER_LAND, "DBArt/LAND.PIC" },
  { "ARTIFACT", 34, DECK_FILTER_ARTIFACT, "DBArt/ARTIFACT.PIC" },
  { "CREATURE", 34, DECK_FILTER_CREATURE, "DBArt/CREATURE.PIC" },
  { "ENCHANT", 34, DECK_FILTER_ENCHANTMENT, "DBArt/ENCHANT.PIC" },
  { "INSTANT", 34, DECK_FILTER_INSTANT, "DBArt/INSTANT.PIC" },
  { "INTERRUPT", 34, DECK_FILTER_INTERRUPT, "DBArt/INTERRUPT.pic" },
  { "SORCERY", 34, DECK_FILTER_SORCERY, "DBArt/SORCERY.PIC" },
  { "FOURTH", 34, DECK_FILTER_FOURTH, "DBArt/FOURTH.PIC" },
  { "ASTRAL", 34, DECK_FILTER_ASTRAL, "DBArt/ASTRAL.PIC" },
  { "ARABIAN", 34, DECK_FILTER_ARABIAN, "DBArt/arabnite.pic" },
  { "ANTIQUITIES", 34, DECK_FILTER_ANTIQUITIES, "DBArt/antiquit.pic" },
  { "LEGENDS", 34, DECK_FILTER_LEGENDS, "DBArt/legends.pic" },
  { "DARK", 34, DECK_FILTER_DARK, "DBArt/dark.pic" },
  { "SAVE", 58, DECK_FILTER_SAVE, NULL },
  { "LOAD", 58, DECK_FILTER_LOAD, NULL },
  { "CLEAR", 58, DECK_FILTER_CLEAR, NULL }
};

#define DECK_ICON_FILTER_COUNT 19
#define DECK_SET_FOURTH (1u << 0)
#define DECK_SET_ASTRAL (1u << 1)
#define DECK_SET_ARABIAN (1u << 2)
#define DECK_SET_ANTIQUITIES (1u << 3)
#define DECK_SET_LEGENDS (1u << 4)
#define DECK_SET_DARK (1u << 5)
#define DECK_SET_ALL ((1u << 6) - 1)

static const DeckMainButton k_main_buttons[] = {
  { "STATS", DECK_SURFACE_X, DECK_SURFACE_X + 240, DECK_MAIN_STATS },
  { "DECK INFO", DECK_SURFACE_X + 240, DECK_SURFACE_X + 480, DECK_MAIN_INFO },
  { "EXIT", DECK_SURFACE_X + 480, DECK_SURFACE_X + 720, DECK_MAIN_EXIT }
};

typedef struct DeckCore {
  char asset_root[512];
  char writable_root[512];
  char deck_path[1024];
  char status_text[512];
  int width;
  int height;
  uint32_t *rgba;
  HBITMAP menubak;
  HBITMAP dektile4;
  HBITMAP dekbar1;
  HBITMAP dektit1;
  HBITMAP bldr[5];
  HBITMAP filter_art[DECK_ICON_FILTER_COUNT];
  uint64_t generation;
  DeckStatus status;
  int status_hold_frames;
  int ui_art_loaded;
  int database_loaded;
  int drawcard_initialized;
  int cardart_initialized;
  int expanded_text;
  int sample_csvid;
  float pointer_x;
  float pointer_y;
  int available_csvids[2000];
  int available_count;
  int available_selected;
  int preview_csvid;
  unsigned int color_filters;
  unsigned int type_filters;
  unsigned int set_filters;
  DeckFileEntry main_deck[300];
  int main_deck_total;
  int main_deck_cards;
  OrigRarities orig_rarities[2000];
} DeckCore;

typedef struct DeckSurfaceCardCacheEntry {
  int csvid;
  uint64_t last_used;
  uint32_t pixels[DECK_CARD_W * DECK_CARD_H];
} DeckSurfaceCardCacheEntry;

static DeckCore g_deck;
static DeckSurfaceCardCacheEntry g_surface_card_cache[DECK_SURFACE_CARD_CACHE_SIZE];
static uint64_t g_surface_card_cache_clock;

extern int global_available_slots;
extern card_ptr_t global_raw_cards_storage[2000];

static int capped_dimension(int requested, int maximum)
{
  if (requested <= 0)
    return maximum;
  return requested < maximum ? requested : maximum;
}

static void make_asset_path(char *out, size_t size, const char *relative_path)
{
  SDL_snprintf(out, size, "%s/%s", g_deck.asset_root, relative_path);
}

static void make_writable_path(char *out, size_t size, const char *relative_path)
{
  SDL_snprintf(out, size, "%s/%s", g_deck.writable_root, relative_path);
}

static void make_parent_dirs(const char *path)
{
  char tmp[1024];
  char *p;
  SDL_strlcpy(tmp, path ? path : "", sizeof(tmp));
  for (p = tmp + 1; *p; ++p) {
    if (*p == '/' || *p == '\\') {
      char saved = *p;
      *p = '\0';
      if (tmp[0]) {
#ifdef _WIN32
        _mkdir(tmp);
#else
        mkdir(tmp, 0777);
#endif
      }
      *p = saved;
    }
  }
}

static void set_status_text(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  SDL_vsnprintf(g_deck.status_text, sizeof(g_deck.status_text), fmt, args);
  va_end(args);
}

static void fill_rect_rgba(int left, int top, int right, int bottom, uint32_t color)
{
  int x;
  int y;
  if (left < 0) left = 0;
  if (top < 0) top = 0;
  if (right > g_deck.width) right = g_deck.width;
  if (bottom > g_deck.height) bottom = g_deck.height;
  for (y = top; y < bottom; ++y) {
    uint32_t *row = g_deck.rgba + y * g_deck.width;
    for (x = left; x < right; ++x)
      row[x] = color;
  }
}

static const unsigned char *glyph_bits(char ch)
{
  static const unsigned char unknown[7] = {0x1e,0x11,0x01,0x06,0x04,0x00,0x04};
  static const unsigned char space[7] = {0,0,0,0,0,0,0};
  static const unsigned char glyphs[][7] = {
    {0x0e,0x11,0x11,0x1f,0x11,0x11,0x11}, /* A */
    {0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e}, /* B */
    {0x0e,0x11,0x10,0x10,0x10,0x11,0x0e}, /* C */
    {0x1e,0x11,0x11,0x11,0x11,0x11,0x1e}, /* D */
    {0x1f,0x10,0x10,0x1e,0x10,0x10,0x1f}, /* E */
    {0x1f,0x10,0x10,0x1e,0x10,0x10,0x10}, /* F */
    {0x0e,0x11,0x10,0x17,0x11,0x11,0x0f}, /* G */
    {0x11,0x11,0x11,0x1f,0x11,0x11,0x11}, /* H */
    {0x0e,0x04,0x04,0x04,0x04,0x04,0x0e}, /* I */
    {0x07,0x02,0x02,0x02,0x12,0x12,0x0c}, /* J */
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, /* K */
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1f}, /* L */
    {0x11,0x1b,0x15,0x15,0x11,0x11,0x11}, /* M */
    {0x11,0x19,0x15,0x13,0x11,0x11,0x11}, /* N */
    {0x0e,0x11,0x11,0x11,0x11,0x11,0x0e}, /* O */
    {0x1e,0x11,0x11,0x1e,0x10,0x10,0x10}, /* P */
    {0x0e,0x11,0x11,0x11,0x15,0x12,0x0d}, /* Q */
    {0x1e,0x11,0x11,0x1e,0x14,0x12,0x11}, /* R */
    {0x0f,0x10,0x10,0x0e,0x01,0x01,0x1e}, /* S */
    {0x1f,0x04,0x04,0x04,0x04,0x04,0x04}, /* T */
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0e}, /* U */
    {0x11,0x11,0x11,0x11,0x11,0x0a,0x04}, /* V */
    {0x11,0x11,0x11,0x15,0x15,0x15,0x0a}, /* W */
    {0x11,0x11,0x0a,0x04,0x0a,0x11,0x11}, /* X */
    {0x11,0x11,0x0a,0x04,0x04,0x04,0x04}, /* Y */
    {0x1f,0x01,0x02,0x04,0x08,0x10,0x1f}, /* Z */
    {0x0e,0x11,0x13,0x15,0x19,0x11,0x0e}, /* 0 */
    {0x04,0x0c,0x04,0x04,0x04,0x04,0x0e}, /* 1 */
    {0x0e,0x11,0x01,0x02,0x04,0x08,0x1f}, /* 2 */
    {0x1f,0x02,0x04,0x02,0x01,0x11,0x0e}, /* 3 */
    {0x02,0x06,0x0a,0x12,0x1f,0x02,0x02}, /* 4 */
    {0x1f,0x10,0x1e,0x01,0x01,0x11,0x0e}, /* 5 */
    {0x06,0x08,0x10,0x1e,0x11,0x11,0x0e}, /* 6 */
    {0x1f,0x01,0x02,0x04,0x08,0x08,0x08}, /* 7 */
    {0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e}, /* 8 */
    {0x0e,0x11,0x11,0x0f,0x01,0x02,0x0c}  /* 9 */
  };
  if (ch >= 'a' && ch <= 'z')
    ch = (char)(ch - 'a' + 'A');
  if (ch >= 'A' && ch <= 'Z')
    return glyphs[ch - 'A'];
  if (ch >= '0' && ch <= '9')
    return glyphs[26 + ch - '0'];
  if (ch == ' ')
    return space;
  return unknown;
}

static void draw_text_rgba(int x, int y, const char *text, uint32_t color, int scale, int max_chars)
{
  int i;
  if (!text)
    return;
  if (scale < 1)
    scale = 1;
  for (i = 0; text[i] && (!max_chars || i < max_chars); ++i) {
    const unsigned char *glyph = glyph_bits(text[i]);
    int row;
    for (row = 0; row < 7; ++row) {
      int col;
      for (col = 0; col < 5; ++col) {
        if (glyph[row] & (1 << (4 - col)))
          fill_rect_rgba(x + i * 6 * scale + col * scale,
                         y + row * scale,
                         x + i * 6 * scale + (col + 1) * scale,
                         y + (row + 1) * scale,
                         color);
      }
    }
  }
}

static void draw_placeholder_text_bar(const char *message)
{
  int len = message ? (int)strlen(message) : 0;
  int bar_width = 320 + len * 4;
  int left = (g_deck.width - bar_width) / 2;
  int top = g_deck.height - 116;
  int i;
  fill_rect_rgba(left, top, left + bar_width, top + 54, 0xff201010u);
  fill_rect_rgba(left + 2, top + 2, left + bar_width - 2, top + 52, 0xff5b2d16u);
  fill_rect_rgba(left + 8, top + 8, left + bar_width - 8, top + 46, 0xff1b1111u);
  for (i = 0; i < len && i < 80; ++i) {
    uint32_t color = (message[i] == ' ') ? 0xff1b1111u : 0xffd4c080u;
    fill_rect_rgba(left + 18 + i * 7, top + 22, left + 22 + i * 7, top + 30, color);
  }
}

static uint32_t menubak_color(unsigned char index)
{
  uint8_t r = (uint8_t)SDL_min(255, 24 + index);
  uint8_t g = (uint8_t)SDL_min(255, 8 + (index * 2) / 3);
  uint8_t b = (uint8_t)SDL_min(255, 8 + index / 3);
  return 0xff000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

static int draw_menubak(void);

static uint32_t pic_color(unsigned char index)
{
  const RGBQUAD *color = &g_cardArtPalette[index];
  return 0xff000000u | ((uint32_t)color->rgbRed << 16) |
         ((uint32_t)color->rgbGreen << 8) | color->rgbBlue;
}

static int blit_pic_scaled(HBITMAP bitmap, int dst_left, int dst_top, int dst_w, int dst_h)
{
  BITMAP info;
  int x;
  int y;
  if (!bitmap || dst_w <= 0 || dst_h <= 0 ||
      !GetObjectA(bitmap, sizeof(info), &info) || !info.bmBits ||
      info.bmWidth <= 0 || info.bmHeight <= 0)
    return 0;
  for (y = 0; y < dst_h; ++y) {
    int dst_y = dst_top + y;
    int src_y = y * info.bmHeight / dst_h;
    const unsigned char *src = (const unsigned char *)info.bmBits + src_y * info.bmWidthBytes;
    if (dst_y < 0 || dst_y >= g_deck.height)
      continue;
    for (x = 0; x < dst_w; ++x) {
      int dst_x = dst_left + x;
      int src_x = x * info.bmWidth / dst_w;
      if (dst_x >= 0 && dst_x < g_deck.width)
        g_deck.rgba[dst_y * g_deck.width + dst_x] = pic_color(src[src_x]);
    }
  }
  return 1;
}

static int draw_deck_tile_background(void)
{
  BITMAP info;
  int tile_x;
  int tile_y;
  if (!g_deck.dektile4 || !GetObjectA(g_deck.dektile4, sizeof(info), &info) || !info.bmBits)
    return draw_menubak();
  fill_rect_rgba(0, 0, g_deck.width, g_deck.height, 0xff100808u);
  for (tile_x = 0; tile_x < g_deck.width; tile_x += 32) {
    for (tile_y = g_deck.height; tile_y > -29; tile_y -= 29)
      blit_pic_scaled(g_deck.dektile4, tile_x, tile_y, info.bmWidth, info.bmHeight);
  }
  return 1;
}

static int draw_builder_mosaic(int left, int top, int right, int bottom)
{
  static const unsigned char mosaic_order[30] = {
    0, 3, 1, 4, 2, 0,
    2, 1, 4, 0, 3, 1,
    4, 0, 2, 3, 1, 4,
    1, 4, 3, 2, 0, 3,
    3, 2, 0, 1, 4, 2
  };
  BITMAP info;
  int column;
  int row;
  int cell_w;
  int cell_h;
  if (!g_deck.bldr[0] || !GetObjectA(g_deck.bldr[0], sizeof(info), &info) || !info.bmBits)
    return 0;
  cell_w = (right - left) / 6;
  cell_h = (bottom - top) / 5;
  for (row = 0; row < 5; ++row) {
    for (column = 0; column < 6; ++column) {
      int index = mosaic_order[row * 6 + column];
      int x = left + column * cell_w;
      int y = top + row * cell_h;
      int width = column == 5 ? right - x : cell_w;
      int height = row == 4 ? bottom - y : cell_h;
      blit_pic_scaled(g_deck.bldr[index], x, y, width, height);
    }
  }
  return 1;
}

static int draw_menubak(void)
{
  BITMAP bitmap;
  int x;
  int y;
  if (!g_deck.menubak || !GetObjectA(g_deck.menubak, sizeof(bitmap), &bitmap) || !bitmap.bmBits)
    return 0;
  for (y = 0; y < g_deck.height; ++y) {
    int src_y = y * bitmap.bmHeight / g_deck.height;
    const unsigned char *src = (const unsigned char *)bitmap.bmBits + src_y * bitmap.bmWidthBytes;
    uint32_t *dst = g_deck.rgba + y * g_deck.width;
    for (x = 0; x < g_deck.width; ++x) {
      int src_x = x * bitmap.bmWidth / g_deck.width;
      dst[x] = menubak_color(src[src_x]);
    }
  }
  return 1;
}

static void draw_loading_screen(void)
{
  ShandalarAssetState small_state = shandalar_assets_state("CardArt/SmallArt.cat");
  ShandalarAssetState med_state = shandalar_assets_state("CardArt/MedArt.cat");
  const char *message = g_deck.status_text[0] ? g_deck.status_text : "Loading deck builder";
  int x;
  int y;

  if (!draw_menubak()) {
    for (y = 0; y < g_deck.height; ++y) {
      uint32_t *row = g_deck.rgba + y * g_deck.width;
      for (x = 0; x < g_deck.width; ++x) {
        uint8_t red = (uint8_t)(32 + ((x ^ y) & 0x1f));
        uint8_t shade = (uint8_t)(8 + ((x + y) & 0x0f));
        row[x] = 0xff000000u | ((uint32_t)red << 16) | ((uint32_t)shade << 8) | shade;
      }
    }
  }

  if (small_state == SHANDALAR_ASSET_ERROR || med_state == SHANDALAR_ASSET_ERROR)
    message = shandalar_assets_last_error();

  draw_placeholder_text_bar(message);
}

static int required_asset_ready(const char *relative_path)
{
  ShandalarAssetState state = shandalar_assets_state(relative_path);
  return state == SHANDALAR_ASSET_READY;
}

static int required_assets_ready(void)
{
  return required_asset_ready("CARDS.DAT") &&
         required_asset_ready("DeckBuilder.csv") &&
         required_asset_ready("DUELPALall.TR") &&
         required_asset_ready("DUEL.plogpal") &&
         required_asset_ready("TT0085M_.TTF") &&
         required_asset_ready("TT0127M_.TTF") &&
         required_asset_ready("TT0298M_.TTF") &&
         required_asset_ready("TT0299M_.TTF") &&
         required_asset_ready("TT0300M_.TTF") &&
         required_asset_ready("TT0530M_.TTF") &&
         required_asset_ready("CardArt/SmallArt.cat") &&
         required_asset_ready("CardArt/MedArt.cat") &&
         required_asset_ready("CardArt/ManaSymbols.pic") &&
         required_asset_ready("CardArt/CardSets.pic") &&
         required_asset_ready("CardArt/CardBack.pic") &&
         required_asset_ready("DBArt/DEKTILE4.PIC") &&
         required_asset_ready("DBArt/DEKBAR1.PIC") &&
         required_asset_ready("DBArt/DEKTIT1.PIC") &&
         required_asset_ready("DBArt/BLDR01C.PIC") &&
         required_asset_ready("DBArt/BLDR02C.PIC") &&
         required_asset_ready("DBArt/BLDR03C.PIC") &&
         required_asset_ready("DBArt/BLDR04C.PIC") &&
         required_asset_ready("DBArt/BLDR05C.PIC");
}

static int required_assets_failed(void)
{
  const char *required[] = {
    "CARDS.DAT",
    "DeckBuilder.csv",
    "DUELPALall.TR",
    "DUEL.plogpal",
    "TT0085M_.TTF",
    "TT0127M_.TTF",
    "TT0298M_.TTF",
    "TT0299M_.TTF",
    "TT0300M_.TTF",
    "TT0530M_.TTF",
    "CardArt/SmallArt.cat",
    "CardArt/MedArt.cat",
    "CardArt/ManaSymbols.pic",
    "CardArt/CardSets.pic",
    "CardArt/CardBack.pic",
    "DBArt/DEKTILE4.PIC",
    "DBArt/DEKBAR1.PIC",
    "DBArt/DEKTIT1.PIC",
    "DBArt/BLDR01C.PIC",
    "DBArt/BLDR02C.PIC",
    "DBArt/BLDR03C.PIC",
    "DBArt/BLDR04C.PIC",
    "DBArt/BLDR05C.PIC"
  };
  int i;
  for (i = 0; i < (int)(sizeof(required) / sizeof(required[0])); ++i) {
    if (shandalar_assets_state(required[i]) == SHANDALAR_ASSET_ERROR)
      return 1;
  }
  return 0;
}

static int choose_sample_card(void)
{
  int i;
  for (i = 0; i < global_available_slots; ++i) {
    if (global_raw_cards_storage[i].name &&
        strcmp(global_raw_cards_storage[i].name, "Blank") != 0 &&
        global_raw_cards_storage[i].id >= 0)
      return i;
  }
  return 0;
}

static const char *active_filter_name(void);

static void update_ready_status(void)
{
  const char *name = "(none)";
  if (g_deck.status_hold_frames > 0)
    return;
  if (g_deck.available_selected >= 0 && g_deck.available_selected < g_deck.available_count) {
    int csvid = g_deck.available_csvids[g_deck.available_selected];
    name = global_raw_cards_storage[csvid].full_name ? global_raw_cards_storage[csvid].full_name : "(unnamed)";
  }
  set_status_text("%d deck | %d %s | %s",
                  g_deck.main_deck_cards, g_deck.available_count, active_filter_name(), name);
}

static void hold_status_for_frames(int frames)
{
  g_deck.status_hold_frames = frames;
}

static int selected_csvid(void);
static void select_available_index(int index);

static const char *active_filter_name(void)
{
  if (g_deck.color_filters || g_deck.type_filters ||
      g_deck.set_filters != DECK_SET_ALL)
    return "filtered";
  return "all";
}

static unsigned int color_filter_bit(uint32_t color)
{
  switch (color) {
  case CP_COLOR_BLACK: return 1u << 0;
  case CP_COLOR_BLUE: return 1u << 1;
  case CP_COLOR_GREEN: return 1u << 2;
  case CP_COLOR_RED: return 1u << 3;
  case CP_COLOR_WHITE: return 1u << 4;
  default: return 0;
  }
}

static unsigned int type_filter_bit(uint32_t type)
{
  switch (type) {
  case CP_TYPE_LAND: return 1u << 0;
  case CP_TYPE_ARTIFACT: return 1u << 1;
  case CP_TYPE_CREATURE: return 1u << 2;
  case CP_TYPE_ENCHANTMENT: return 1u << 3;
  case CP_TYPE_INSTANT: return 1u << 4;
  case CP_TYPE_INTERRUPT: return 1u << 5;
  case CP_TYPE_SORCERY: return 1u << 6;
  default: return 0;
  }
}

static int card_matches_set_filter(int csvid)
{
  const OrigRarities *rarity = &g_deck.orig_rarities[csvid];
  if ((g_deck.set_filters & DECK_SET_FOURTH) &&
      (rarity->set == SET_UNLIMITED ||
       rarity->exp_rarities[0] != '-' ||
       rarity->exp_rarities[1] != '-'))
    return 1;
  if ((g_deck.set_filters & DECK_SET_ASTRAL) &&
      (rarity->set == SET_ASTRAL || rarity->set == SET_PROMO))
    return 1;
  if ((g_deck.set_filters & DECK_SET_ARABIAN) &&
      rarity->set == SET_ARABIAN_NIGHTS)
    return 1;
  if ((g_deck.set_filters & DECK_SET_ANTIQUITIES) &&
      rarity->set == SET_ANTIQUITIES)
    return 1;
  if ((g_deck.set_filters & DECK_SET_LEGENDS) &&
      rarity->set == SET_LEGENDS)
    return 1;
  if ((g_deck.set_filters & DECK_SET_DARK) &&
      rarity->set == SET_THE_DARK)
    return 1;
  return 0;
}

static int card_matches_filter(int csvid)
{
  const card_ptr_t *card;
  unsigned int color_bit;
  unsigned int type_bit;
  if (csvid < 0 || csvid >= global_available_slots)
    return 0;
  card = &global_raw_cards_storage[csvid];
  if (!card->name || strcmp(card->name, "Blank") == 0 || card->id < 0)
    return 0;
  if ((!IsCardAvailable(csvid, 0) &&
       !IsCardAvailable(csvid, 1) &&
       !IsCardAvailable(csvid, 2)) ||
      !card_matches_set_filter(csvid))
    return 0;
  color_bit = color_filter_bit(card->color);
  type_bit = type_filter_bit(card->card_type);
  if (g_deck.color_filters && !(g_deck.color_filters & color_bit))
    return 0;
  if (g_deck.type_filters && !(g_deck.type_filters & type_bit))
    return 0;
  return 1;
}

static int compare_available_cards(const void *left, const void *right)
{
  int left_csvid = *(const int *)left;
  int right_csvid = *(const int *)right;
  const char *left_name = global_raw_cards_storage[left_csvid].full_name;
  const char *right_name = global_raw_cards_storage[right_csvid].full_name;
  int comparison = SDL_strcasecmp(left_name ? left_name : "",
                                  right_name ? right_name : "");
  return comparison ? comparison : left_csvid - right_csvid;
}

static void build_available_cards(void)
{
  int i;
  int previous_csvid = selected_csvid();
  g_deck.available_count = 0;
  for (i = 0; i < global_available_slots && g_deck.available_count < 2000; ++i) {
    if (card_matches_filter(i))
      g_deck.available_csvids[g_deck.available_count++] = i;
  }
  SDL_qsort(g_deck.available_csvids, (size_t)g_deck.available_count,
            sizeof(g_deck.available_csvids[0]), compare_available_cards);
  g_deck.available_selected = g_deck.available_count ? 0 : -1;
  for (i = 0; i < g_deck.available_count; ++i) {
    if (g_deck.available_csvids[i] == previous_csvid) {
      g_deck.available_selected = i;
      break;
    }
  }
  set_status_text("Showing %d %s cards", g_deck.available_count, active_filter_name());
  hold_status_for_frames(90);
}

static int selected_csvid(void)
{
  if (g_deck.available_selected < 0 || g_deck.available_selected >= g_deck.available_count)
    return -1;
  return g_deck.available_csvids[g_deck.available_selected];
}

static int find_deck_entry(int csvid)
{
  int i;
  for (i = 0; i < g_deck.main_deck_total; ++i) {
    if (g_deck.main_deck[i].csvid == csvid)
      return i;
  }
  return -1;
}

static void add_card_to_main_deck(int csvid, int amount)
{
  int idx;
  if (csvid < 0 || amount <= 0)
    return;
  idx = find_deck_entry(csvid);
  if (idx >= 0) {
    g_deck.main_deck[idx].amount += amount;
  } else if (g_deck.main_deck_total < (int)(sizeof(g_deck.main_deck) / sizeof(g_deck.main_deck[0]))) {
    idx = g_deck.main_deck_total++;
    g_deck.main_deck[idx].csvid = csvid;
    g_deck.main_deck[idx].amount = amount;
    g_deck.main_deck[idx].name = global_raw_cards_storage[csvid].full_name;
  }
  g_deck.main_deck_cards += amount;
  update_ready_status();
}

static void remove_card_from_main_deck(int csvid, int amount)
{
  int idx;
  int i;
  if (csvid < 0 || amount <= 0)
    return;
  idx = find_deck_entry(csvid);
  if (idx < 0)
    return;
  if (amount >= g_deck.main_deck[idx].amount) {
    g_deck.main_deck_cards -= g_deck.main_deck[idx].amount;
    for (i = idx; i + 1 < g_deck.main_deck_total; ++i)
      g_deck.main_deck[i] = g_deck.main_deck[i + 1];
    --g_deck.main_deck_total;
  } else {
    g_deck.main_deck[idx].amount -= amount;
    g_deck.main_deck_cards -= amount;
  }
  update_ready_status();
}

static void clear_main_deck(void)
{
  memset(g_deck.main_deck, 0, sizeof(g_deck.main_deck));
  g_deck.main_deck_total = 0;
  g_deck.main_deck_cards = 0;
  update_ready_status();
}

static int save_deck_file(const char *path)
{
  if (!path || !path[0])
    return 0;
  make_parent_dirs(path);
  if (!deck_file_save(path, g_deck.main_deck, g_deck.main_deck_total))
    return 0;
  set_status_text("Saved %d cards to %s", g_deck.main_deck_cards, path);
  hold_status_for_frames(180);
  return 1;
}

static int add_loaded_card(int csvid, int amount, void *userdata)
{
  if (csvid < 0 || csvid >= global_available_slots || amount <= 0)
    return 0;
  (void)userdata;
  add_card_to_main_deck(csvid, amount);
  return 1;
}

static int load_deck_file(const char *path)
{
  int loaded_cards = 0;

  if (!path || !path[0])
    return 0;

  clear_main_deck();
  if (!deck_file_load(path, add_loaded_card, NULL, &loaded_cards)) {
    clear_main_deck();
    return 0;
  }
  set_status_text("Loaded %d cards from %s", loaded_cards, path);
  hold_status_for_frames(180);
  return loaded_cards > 0;
}

static int load_database(void)
{
  char path[1024];
  if (g_deck.database_loaded)
    return 1;

  make_asset_path(path, sizeof(path), "CARDS.DAT");
  if (!read_db_guts(path)) {
    set_status_text("Missing or invalid CARDS.DAT");
    return 0;
  }

  make_asset_path(path, sizeof(path), "DeckBuilder.csv");
  if (!ReadDeckbuilderCsv(path, g_deck.orig_rarities)) {
    set_status_text("Missing or invalid DeckBuilder.csv");
    return 0;
  }
  FreeRaritiesCsvRaw();

  g_deck.sample_csvid = choose_sample_card();
  build_available_cards();
  g_deck.database_loaded = 1;
  set_status_text("Loaded %d cards", global_available_slots);
  return 1;
}

static int load_ui_art(void)
{
  char path[1024];
  int i;
  if (g_deck.ui_art_loaded)
    return 1;

  if (!g_deck.menubak) {
    make_asset_path(path, sizeof(path), "MENUBAK.PIC");
    g_deck.menubak = load_pic(path);
  }
  if (!g_deck.dektile4) {
    make_asset_path(path, sizeof(path), "DBArt/DEKTILE4.PIC");
    g_deck.dektile4 = load_pic(path);
  }
  if (!g_deck.dekbar1) {
    make_asset_path(path, sizeof(path), "DBArt/DEKBAR1.PIC");
    g_deck.dekbar1 = load_pic(path);
  }
  if (!g_deck.dektit1) {
    make_asset_path(path, sizeof(path), "DBArt/DEKTIT1.PIC");
    g_deck.dektit1 = load_pic(path);
  }
  if (!g_deck.bldr[0]) {
    make_asset_path(path, sizeof(path), "DBArt/BLDR01C.PIC");
    g_deck.bldr[0] = load_pic(path);
  }
  if (!g_deck.bldr[1]) {
    make_asset_path(path, sizeof(path), "DBArt/BLDR02C.PIC");
    g_deck.bldr[1] = load_pic(path);
  }
  if (!g_deck.bldr[2]) {
    make_asset_path(path, sizeof(path), "DBArt/BLDR03C.PIC");
    g_deck.bldr[2] = load_pic(path);
  }
  if (!g_deck.bldr[3]) {
    make_asset_path(path, sizeof(path), "DBArt/BLDR04C.PIC");
    g_deck.bldr[3] = load_pic(path);
  }
  if (!g_deck.bldr[4]) {
    make_asset_path(path, sizeof(path), "DBArt/BLDR05C.PIC");
    g_deck.bldr[4] = load_pic(path);
  }
  for (i = 0; i < DECK_ICON_FILTER_COUNT; ++i) {
    if (!g_deck.filter_art[i]) {
      make_asset_path(path, sizeof(path), k_filter_buttons[i].asset_name);
      g_deck.filter_art[i] = load_pic(path);
    }
  }

  g_deck.ui_art_loaded = g_deck.dektile4 && g_deck.dekbar1 && g_deck.dektit1 &&
                         g_deck.bldr[0] && g_deck.bldr[1] && g_deck.bldr[2] &&
                         g_deck.bldr[3] && g_deck.bldr[4];
  for (i = 0; i < DECK_ICON_FILTER_COUNT; ++i)
    g_deck.ui_art_loaded = g_deck.ui_art_loaded && g_deck.filter_art[i];
  if (!g_deck.ui_art_loaded) {
    set_status_text("Could not load deck builder UI art");
    return 0;
  }
  return 1;
}

static int initialize_drawcard(void)
{
  if (g_deck.drawcard_initialized)
    return 1;
  if (!g_deck.cardart_initialized) {
    if (!CardArtLib_InitializeFromRoot(g_deck.asset_root)) {
      set_status_text("Could not initialize card art catalogs");
      return 0;
    }
    g_deck.cardart_initialized = 1;
  }
  if (!DrawCardLib_InitializeFromRoot(g_deck.asset_root)) {
    set_status_text("Could not initialize drawcard resources");
    return 0;
  }
  g_deck.drawcard_initialized = 1;
  return 1;
}

static uint32_t palette_index_to_rgba(uint8_t index)
{
  const RGBQUAD *color = &g_cardArtPalette[index];
  return 0xff000000u | ((uint32_t)color->rgbRed << 16) |
         ((uint32_t)color->rgbGreen << 8) | color->rgbBlue;
}

static void draw_gdi_card_to_frame(int csvid, int dst_left, int dst_top, int card_w, int card_h)
{
  uint32_t *pixels;
  int x;
  int y;
  pixels = (uint32_t *)SDL_malloc((size_t)card_w * (size_t)card_h * sizeof(*pixels));
  if (!pixels)
    return;
  if (!deck_ui_render_small_card(csvid, pixels, card_w, card_h)) {
    SDL_free(pixels);
    return;
  }

  fill_rect_rgba(dst_left - 4, dst_top - 4, dst_left + card_w + 4, dst_top + card_h + 4, 0xff120b0bu);
  fill_rect_rgba(dst_left - 2, dst_top - 2, dst_left + card_w + 2, dst_top + card_h + 2, 0xff6b4a2cu);
  for (y = 0; y < card_h; ++y) {
    for (x = 0; x < card_w; ++x) {
      int dst_x = dst_left + x;
      int dst_y = dst_top + y;
      if (dst_x >= 0 && dst_x < g_deck.width && dst_y >= 0 && dst_y < g_deck.height)
        g_deck.rgba[dst_y * g_deck.width + dst_x] = pixels[y * card_w + x];
    }
  }
  SDL_free(pixels);
}

static const uint32_t *cached_surface_card_pixels(int csvid)
{
  DeckSurfaceCardCacheEntry *slot = NULL;
  int i;
  if (csvid < 0)
    return NULL;
  ++g_surface_card_cache_clock;
  for (i = 0; i < DECK_SURFACE_CARD_CACHE_SIZE; ++i) {
    DeckSurfaceCardCacheEntry *entry = &g_surface_card_cache[i];
    if (entry->csvid == csvid) {
      entry->last_used = g_surface_card_cache_clock;
      return entry->pixels;
    }
    if (!slot || entry->csvid < 0 || entry->last_used < slot->last_used)
      slot = entry;
  }
  if (!slot || !deck_ui_render_small_card(csvid, slot->pixels, DECK_CARD_W, DECK_CARD_H))
    return NULL;
  slot->csvid = csvid;
  slot->last_used = g_surface_card_cache_clock;
  return slot->pixels;
}

static void draw_cached_surface_card_to_frame(int csvid, int dst_left, int dst_top)
{
  const uint32_t *pixels = cached_surface_card_pixels(csvid);
  int x;
  int y;
  if (!pixels) {
    draw_gdi_card_to_frame(csvid, dst_left, dst_top, DECK_CARD_W, DECK_CARD_H);
    return;
  }
  fill_rect_rgba(dst_left - 4, dst_top - 4, dst_left + DECK_CARD_W + 4,
                 dst_top + DECK_CARD_H + 4, 0xff120b0bu);
  fill_rect_rgba(dst_left - 2, dst_top - 2, dst_left + DECK_CARD_W + 2,
                 dst_top + DECK_CARD_H + 2, 0xff6b4a2cu);
  for (y = 0; y < DECK_CARD_H; ++y) {
    for (x = 0; x < DECK_CARD_W; ++x) {
      int dst_x = dst_left + x;
      int dst_y = dst_top + y;
      if (dst_x >= 0 && dst_x < g_deck.width && dst_y >= 0 && dst_y < g_deck.height)
        g_deck.rgba[dst_y * g_deck.width + dst_x] = pixels[y * DECK_CARD_W + x];
    }
  }
}

int deck_ui_render_small_card(int csvid, uint32_t *pixels, int width, int height)
{
  HDC dc;
  HBITMAP bitmap;
  BITMAPINFO info;
  uint8_t *bits;
  RECT rect;
  HBRUSH brush;
  int x;
  int y;
  if (!pixels || csvid < 0 || csvid >= global_available_slots || width <= 0 || height <= 0)
    return 0;

  memset(&info, 0, sizeof(info));
  info.bmiHeader.biWidth = 200;
  info.bmiHeader.biHeight = -300;
  info.bmiHeader.biPlanes = 1;
  info.bmiHeader.biBitCount = 8;
  dc = CreateCompatibleDC(NULL);
  bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, (void **)&bits, NULL, 0);
  if (!dc || !bitmap) {
    if (bitmap)
      DeleteObject(bitmap);
    if (dc)
      DeleteDC(dc);
    return 0;
  }
  SelectObject(dc, bitmap);
  ApplyCardArtPaletteToDc(dc);
  SetRect(&rect, 0, 0, 200, 300);
  brush = CreateSolidBrush(PALETTEINDEX(0));
  FillRect(dc, &rect, brush);
  DeleteObject(brush);
  DrawSmallCard(dc, &rect, &global_raw_cards_storage[csvid], 1, 1);

  for (y = 0; y < height; ++y)
    for (x = 0; x < width; ++x)
      pixels[y * width + x] =
        palette_index_to_rgba(bits[(y * 300 / height) * 200 + x * 200 / width]);
  DeleteObject(bitmap);
  DeleteDC(dc);
  return 1;
}

static void draw_gdi_full_card_to_frame(int csvid, int dst_left, int dst_top, int card_w, int card_h)
{
  HDC dc;
  HBITMAP bitmap;
  BITMAPINFO info;
  uint8_t *bits;
  RECT rect;
  HBRUSH brush;
  int x;
  int y;
  if (csvid < 0 || csvid >= global_available_slots || card_w <= 0 || card_h <= 0)
    return;

  memset(&info, 0, sizeof(info));
  info.bmiHeader.biWidth = card_w;
  info.bmiHeader.biHeight = -card_h;
  info.bmiHeader.biPlanes = 1;
  info.bmiHeader.biBitCount = 8;
  dc = CreateCompatibleDC(NULL);
  bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, (void **)&bits, NULL, 0);
  if (!dc || !bitmap) {
    if (bitmap)
      DeleteObject(bitmap);
    if (dc)
      DeleteDC(dc);
    return;
  }
  SelectObject(dc, bitmap);
  ApplyCardArtPaletteToDc(dc);
  SetRect(&rect, 0, 0, card_w, card_h);
  brush = CreateSolidBrush(PALETTEINDEX(0));
  FillRect(dc, &rect, brush);
  DeleteObject(brush);
  DrawFullCard(dc, &rect, &global_raw_cards_storage[csvid], 0, 1,
               g_deck.expanded_text, "%s");

  for (y = 0; y < card_h; ++y) {
    for (x = 0; x < card_w; ++x) {
      int dst_x = dst_left + x;
      int dst_y = dst_top + y;
      if (dst_x >= 0 && dst_x < g_deck.width && dst_y >= 0 && dst_y < g_deck.height)
        g_deck.rgba[dst_y * g_deck.width + dst_x] =
            palette_index_to_rgba(bits[y * ((card_w + 3) & ~3) + x]);
    }
  }
  DeleteObject(bitmap);
  DeleteDC(dc);
}

static void draw_panel(int left, int top, int right, int bottom, const char *title)
{
  fill_rect_rgba(left, top, right, bottom, 0xff100b0bu);
  fill_rect_rgba(left + 2, top + 2, right - 2, bottom - 2, 0xff2b1a14u);
  fill_rect_rgba(left + 6, top + 22, right - 6, bottom - 6, 0xff181414u);
  draw_text_rgba(left + 10, top + 7, title, 0xffe4d28cu, 1, 40);
}

static int filter_command_active(DeckFilterCommand command)
{
  switch (command) {
  case DECK_FILTER_ALL:
    return !g_deck.color_filters && !g_deck.type_filters;
  case DECK_FILTER_BLACK:
    return !!(g_deck.color_filters & (1u << 0));
  case DECK_FILTER_BLUE:
    return !!(g_deck.color_filters & (1u << 1));
  case DECK_FILTER_GREEN:
    return !!(g_deck.color_filters & (1u << 2));
  case DECK_FILTER_RED:
    return !!(g_deck.color_filters & (1u << 3));
  case DECK_FILTER_WHITE:
    return !!(g_deck.color_filters & (1u << 4));
  case DECK_FILTER_LAND:
    return !!(g_deck.type_filters & (1u << 0));
  case DECK_FILTER_CREATURE:
    return !!(g_deck.type_filters & (1u << 2));
  case DECK_FILTER_ENCHANTMENT:
    return !!(g_deck.type_filters & (1u << 3));
  case DECK_FILTER_INSTANT:
    return !!(g_deck.type_filters & (1u << 4));
  case DECK_FILTER_INTERRUPT:
    return !!(g_deck.type_filters & (1u << 5));
  case DECK_FILTER_SORCERY:
    return !!(g_deck.type_filters & (1u << 6));
  case DECK_FILTER_ARTIFACT:
    return !!(g_deck.type_filters & (1u << 1));
  case DECK_FILTER_FOURTH:
    return !!(g_deck.set_filters & DECK_SET_FOURTH);
  case DECK_FILTER_ASTRAL:
    return !!(g_deck.set_filters & DECK_SET_ASTRAL);
  case DECK_FILTER_ARABIAN:
    return !!(g_deck.set_filters & DECK_SET_ARABIAN);
  case DECK_FILTER_ANTIQUITIES:
    return !!(g_deck.set_filters & DECK_SET_ANTIQUITIES);
  case DECK_FILTER_LEGENDS:
    return !!(g_deck.set_filters & DECK_SET_LEGENDS);
  case DECK_FILTER_DARK:
    return !!(g_deck.set_filters & DECK_SET_DARK);
  default:
    return 0;
  }
}

static void draw_button_rect(int left, int top, int right, int bottom, const char *text, int active)
{
  fill_rect_rgba(left, top, right, bottom, active ? 0xff6c4418u : 0xff262018u);
  fill_rect_rgba(left + 1, top + 1, right - 1, bottom - 1, active ? 0xffe0b050u : 0xff847048u);
  fill_rect_rgba(left + 3, top + 3, right - 3, bottom - 3, active ? 0xff4a3014u : 0xff2c251du);
  draw_text_rgba(left + 12, top + (bottom - top - 7) / 2, text, 0xfff0d890u, 1, 24);
}

static void draw_title_panel(void)
{
  if (!blit_pic_scaled(g_deck.dektit1, DECK_TITLE_X, DECK_TITLE_Y, DECK_TITLE_W, DECK_TITLE_H))
    draw_panel(DECK_TITLE_X, DECK_TITLE_Y, DECK_TITLE_X + DECK_TITLE_W, DECK_TITLE_Y + DECK_TITLE_H, "");
  draw_text_rgba(DECK_TITLE_X + 28, DECK_TITLE_Y + 19, "New Deck", 0xffffe8a0u, 2, 16);
  draw_text_rgba(DECK_TITLE_X + 32, DECK_TITLE_Y + 46, g_deck.status_text, 0xffe4d28cu, 1, 34);
}

static void draw_fullcard_preview(void)
{
  int csvid = g_deck.preview_csvid >= 0 ? g_deck.preview_csvid : selected_csvid();
  int card_h = DECK_FULLCARD_H - 8;
  int card_w = card_h * 2 / 3;
  int card_x = DECK_FULLCARD_X + (DECK_FULLCARD_W - card_w) / 2;
  fill_rect_rgba(DECK_FULLCARD_X - 2, DECK_FULLCARD_Y - 2,
                 DECK_FULLCARD_X + DECK_FULLCARD_W + 2, DECK_FULLCARD_Y + DECK_FULLCARD_H + 2,
                 0xff070404u);
  fill_rect_rgba(DECK_FULLCARD_X, DECK_FULLCARD_Y,
                 DECK_FULLCARD_X + DECK_FULLCARD_W, DECK_FULLCARD_Y + DECK_FULLCARD_H,
                 0xff161111u);
  if (csvid >= 0)
    draw_gdi_full_card_to_frame(csvid, card_x, DECK_FULLCARD_Y + 4, card_w, card_h);
}

static void draw_filter_strip(void)
{
  int i;
  int x = DECK_FILTER_X + 8;
  if (!blit_pic_scaled(g_deck.dekbar1, DECK_FILTER_X, DECK_FILTER_Y, DECK_FILTER_W, DECK_FILTER_H))
    draw_panel(DECK_FILTER_X, DECK_FILTER_Y, DECK_FILTER_X + DECK_FILTER_W, DECK_FILTER_Y + DECK_FILTER_H, "");
  for (i = 0; i < (int)(sizeof(k_filter_buttons) / sizeof(k_filter_buttons[0])); ++i) {
    const DeckFilterButton *button = &k_filter_buttons[i];
    if (!button->asset_name)
      continue;
    {
      int active = filter_command_active(button->command);
      fill_rect_rgba(x, DECK_FILTER_Y + 5, x + button->width, DECK_FILTER_Y + 39,
                     active ? 0xffffffffu : 0xff303030u);
      fill_rect_rgba(x + 2, DECK_FILTER_Y + 7, x + button->width - 2, DECK_FILTER_Y + 37,
                     active ? 0xff606060u : 0xffb0b0b0u);
      blit_pic_scaled(g_deck.filter_art[i], x + 4, DECK_FILTER_Y + 9,
                      button->width - 8, 26);
    }
    x += button->width + 6;
  }
  x = DECK_FILTER_X + DECK_FILTER_W - 204;
  for (i = 0; i < (int)(sizeof(k_filter_buttons) / sizeof(k_filter_buttons[0])); ++i) {
    const DeckFilterButton *button = &k_filter_buttons[i];
    if (button->asset_name)
      continue;
    draw_button_rect(x, DECK_FILTER_Y + 8, x + button->width, DECK_FILTER_Y + 36,
                     button->label, 0);
    x += button->width + 8;
  }
}

static void draw_browser_backdrop(void)
{
  if (!blit_pic_scaled(g_deck.dekbar1, DECK_HORZLIST_X, DECK_HORZLIST_Y, DECK_HORZLIST_W, DECK_HORZLIST_H))
    draw_panel(DECK_HORZLIST_X, DECK_HORZLIST_Y, DECK_HORZLIST_X + DECK_HORZLIST_W, DECK_HORZLIST_Y + DECK_HORZLIST_H, "");
}

static void draw_main_buttons(void)
{
  int i;
  for (i = 0; i < (int)(sizeof(k_main_buttons) / sizeof(k_main_buttons[0])); ++i) {
    const DeckMainButton *button = &k_main_buttons[i];
    int left = button->left;
    int right = button->right;
    int top = DECK_BUTTON_Y;
    int bottom = DECK_BUTTON_Y + DECK_BUTTON_H;
    fill_rect_rgba(left, top, right, bottom, 0xff404040u);
    fill_rect_rgba(left + 1, top + 1, right - 1, bottom - 1, 0xffe0e0e0u);
    fill_rect_rgba(left + 3, top + 3, right - 3, bottom - 3, 0xffa8a8a8u);
    fill_rect_rgba(right - 3, top + 3, right - 1, bottom - 1, 0xff505050u);
    fill_rect_rgba(left + 3, bottom - 3, right - 1, bottom - 1, 0xff505050u);
    draw_text_rgba(left + (right - left - (int)strlen(button->label) * 12) / 2,
                   top + 11, button->label, 0xff202020u, 2, 24);
  }
}

static void draw_deck_surface(void)
{
  int x = DECK_SURFACE_X + 10;
  int y = DECK_SURFACE_Y + 10;
  int y_step = (DECK_CARD_H * 18) / 100;
  int i;
  if (!draw_builder_mosaic(DECK_SURFACE_X, DECK_SURFACE_Y,
                           DECK_SURFACE_X + DECK_SURFACE_W, DECK_SURFACE_Y + DECK_SURFACE_H))
    fill_rect_rgba(DECK_SURFACE_X, DECK_SURFACE_Y,
                   DECK_SURFACE_X + DECK_SURFACE_W, DECK_SURFACE_Y + DECK_SURFACE_H, 0xff181414u);
  fill_rect_rgba(DECK_SURFACE_X, DECK_SURFACE_Y, DECK_SURFACE_X + DECK_SURFACE_W, DECK_SURFACE_Y + 2, 0xff080404u);
  fill_rect_rgba(DECK_SURFACE_X, DECK_SURFACE_Y, DECK_SURFACE_X + 2, DECK_SURFACE_Y + DECK_SURFACE_H, 0xff080404u);
  fill_rect_rgba(DECK_SURFACE_X, DECK_SURFACE_Y + DECK_SURFACE_H - 2,
                 DECK_SURFACE_X + DECK_SURFACE_W, DECK_SURFACE_Y + DECK_SURFACE_H, 0xff403020u);
  fill_rect_rgba(DECK_SURFACE_X + DECK_SURFACE_W - 2, DECK_SURFACE_Y,
                 DECK_SURFACE_X + DECK_SURFACE_W, DECK_SURFACE_Y + DECK_SURFACE_H, 0xff403020u);
  for (i = 0; i < g_deck.main_deck_total; ++i) {
    char amount_text[16];
    if (y + DECK_CARD_H > DECK_SURFACE_Y + DECK_SURFACE_H - 10) {
      x += DECK_CARD_W + 8;
      y = DECK_SURFACE_Y + 10;
    }
    if (x + DECK_CARD_W > DECK_SURFACE_X + DECK_SURFACE_W - 8)
      break;
    draw_cached_surface_card_to_frame(g_deck.main_deck[i].csvid, x, y);
    SDL_snprintf(amount_text, sizeof(amount_text), "X%d", g_deck.main_deck[i].amount);
    fill_rect_rgba(x + DECK_CARD_W - 34, y + 4, x + DECK_CARD_W - 4, y + 18, 0xff201000u);
    draw_text_rgba(x + DECK_CARD_W - 30, y + 7, amount_text, 0xffffe080u, 1, 4);
    y += y_step;
  }
  if (!g_deck.main_deck_total)
    draw_text_rgba(DECK_SURFACE_X + 36, DECK_SURFACE_Y + 60,
                   "CLICK A CARD BELOW OR PRESS ENTER TO ADD TO DECK", 0xffc0b080u, 1, 55);
}

static void draw_ready_screen(void)
{
  if (!draw_deck_tile_background())
    fill_rect_rgba(0, 0, g_deck.width, g_deck.height, 0xff201010u);
  draw_title_panel();
  draw_fullcard_preview();
  draw_deck_surface();
  draw_main_buttons();
  draw_filter_strip();
  draw_browser_backdrop();
}

static void render(void)
{
  if (g_deck.database_loaded && g_deck.drawcard_initialized)
    draw_ready_screen();
  else
    draw_loading_screen();
  ++g_deck.generation;
}

int deck_create(const DeckConfig *config)
{
  if (!config || !config->asset_root)
    return 0;
  if (g_deck.rgba)
    deck_destroy();
  memset(&g_deck, 0, sizeof(g_deck));
  for (int i = 0; i < DECK_SURFACE_CARD_CACHE_SIZE; ++i)
    g_surface_card_cache[i].csvid = -1;
  g_surface_card_cache_clock = 0;
  g_deck.set_filters = DECK_SET_ALL;
  SDL_strlcpy(g_deck.asset_root, config->asset_root, sizeof(g_deck.asset_root));
  SDL_strlcpy(g_deck.writable_root, config->writable_root ? config->writable_root : ".",
              sizeof(g_deck.writable_root));
  g_deck.preview_csvid = -1;
  make_writable_path(g_deck.deck_path, sizeof(g_deck.deck_path), "PlayDeck/new.dck");
  g_deck.width = capped_dimension(config->width, DECK_BASE_WIDTH);
  g_deck.height = capped_dimension(config->height, DECK_BASE_HEIGHT);
  g_deck.rgba = (uint32_t *)SDL_calloc((size_t)g_deck.width * (size_t)g_deck.height, sizeof(uint32_t));
  if (!g_deck.rgba)
    goto fail;
  shandalar_assets_set_root(g_deck.asset_root);
  shandalar_set_default_font_root(g_deck.asset_root);
  set_status_text("Loading deck builder assets");
  if (config->prefetch_cardart)
    deck_prefetch_cardart();
  g_deck.status = DECK_RUNNING;
  render();
  return 1;

fail:
  deck_destroy();
  return 0;
}

void deck_destroy(void)
{
  int i;
  if (g_deck.drawcard_initialized)
    DrawCardLib_Shutdown();
  if (g_deck.cardart_initialized)
    CardArtLib_Shutdown();
  if (g_deck.menubak)
    DeleteObject(g_deck.menubak);
  if (g_deck.dektile4)
    DeleteObject(g_deck.dektile4);
  if (g_deck.dekbar1)
    DeleteObject(g_deck.dekbar1);
  if (g_deck.dektit1)
    DeleteObject(g_deck.dektit1);
  for (i = 0; i < 5; ++i) {
    if (g_deck.bldr[i])
      DeleteObject(g_deck.bldr[i]);
  }
  for (i = 0; i < DECK_ICON_FILTER_COUNT; ++i) {
    if (g_deck.filter_art[i])
      DeleteObject(g_deck.filter_art[i]);
  }
  SDL_free(g_deck.rgba);
  memset(&g_deck, 0, sizeof(g_deck));
}

DeckStatus deck_step(uint64_t now_ns)
{
  (void)now_ns;
  shandalar_assets_update();
  if (!required_assets_ready()) {
    if (required_assets_failed())
      set_status_text("%s", shandalar_assets_last_error());
    else
      set_status_text("Loading deck builder assets");
  } else if (!load_ui_art()) {
    /* Keep rendering the error text. */
  } else if (!load_database()) {
    /* Keep rendering the error text. */
  } else if (!initialize_drawcard()) {
    /* Keep rendering the error text. */
  } else {
    if (g_deck.status_hold_frames > 0)
      --g_deck.status_hold_frames;
    else
      update_ready_status();
  }
  render();
  return g_deck.status;
}

int deck_get_frame(DeckFrame *frame)
{
  if (!frame || !g_deck.rgba)
    return 0;
  frame->pixels = g_deck.rgba;
  frame->width = g_deck.width;
  frame->height = g_deck.height;
  frame->pitch = g_deck.width * (int)sizeof(uint32_t);
  frame->generation = g_deck.generation;
  return 1;
}

const char *deck_status_text(void)
{
  return g_deck.status_text;
}

static int core_is_interactive(void)
{
  return g_deck.database_loaded && g_deck.drawcard_initialized;
}

int deck_ui_is_interactive(void)
{
  return core_is_interactive();
}

int deck_ui_available_count(void)
{
  return core_is_interactive() ? g_deck.available_count : 0;
}

int deck_ui_available_csvid(int index)
{
  if (!core_is_interactive() || index < 0 || index >= g_deck.available_count)
    return -1;
  return g_deck.available_csvids[index];
}

int deck_ui_selected_index(void)
{
  return core_is_interactive() ? g_deck.available_selected : -1;
}

void deck_ui_select_available_index(int index)
{
  if (core_is_interactive())
    select_available_index(index);
}

void deck_ui_preview_card(int csvid)
{
  if (!core_is_interactive())
    return;
  g_deck.preview_csvid =
      csvid >= 0 && csvid < global_available_slots ? csvid : -1;
}

int deck_ui_deck_count(void)
{
  return core_is_interactive() ? g_deck.main_deck_total : 0;
}

int deck_ui_deck_entry(int index, int *csvid, int *amount,
                       int *x, int *y, int *width, int *height)
{
  int y_step = (DECK_CARD_H * 18) / 100;
  int cards_per_column =
      (DECK_SURFACE_H - 20 - DECK_CARD_H) / y_step + 1;
  int column;
  int row;

  if (!core_is_interactive() || index < 0 || index >= g_deck.main_deck_total)
    return 0;
  column = index / cards_per_column;
  row = index % cards_per_column;
  if (DECK_SURFACE_X + 10 + column * (DECK_CARD_W + 8) + DECK_CARD_W >
      DECK_SURFACE_X + DECK_SURFACE_W - 8)
    return 0;
  if (csvid)
    *csvid = g_deck.main_deck[index].csvid;
  if (amount)
    *amount = g_deck.main_deck[index].amount;
  if (x)
    *x = DECK_SURFACE_X + 10 + column * (DECK_CARD_W + 8);
  if (y)
    *y = DECK_SURFACE_Y + 10 + row * y_step;
  if (width)
    *width = DECK_CARD_W;
  if (height)
    *height = DECK_CARD_H;
  return 1;
}

static const card_ptr_t *ui_card(int csvid)
{
  if (!core_is_interactive() || csvid < 0 || csvid >= global_available_slots)
    return NULL;
  return &global_raw_cards_storage[csvid];
}

const char *deck_ui_card_name(int csvid)
{
  const card_ptr_t *card = ui_card(csvid);
  return card && card->full_name ? card->full_name : "";
}

const char *deck_ui_card_type(int csvid)
{
  const card_ptr_t *card = ui_card(csvid);
  return card && card->type_text ? card->type_text : "";
}

const char *deck_ui_card_rules(int csvid)
{
  const card_ptr_t *card = ui_card(csvid);
  return card && card->rules_text ? card->rules_text : "";
}

void deck_ui_add_card(int csvid)
{
  if (core_is_interactive())
    add_card_to_main_deck(csvid, 1);
}

void deck_ui_remove_card(int csvid)
{
  if (core_is_interactive())
    remove_card_from_main_deck(csvid, 1);
}

int deck_ui_expanded_text(void)
{
  return g_deck.expanded_text;
}

void deck_ui_set_expanded_text(int expanded)
{
  g_deck.expanded_text = expanded != 0;
}

static void show_deck_stats(void)
{
  int i;
  int black = 0;
  int blue = 0;
  int green = 0;
  int red = 0;
  int white = 0;
  int land = 0;
  int creature = 0;
  int spell = 0;
  int artifact = 0;
  for (i = 0; i < g_deck.main_deck_total; ++i) {
    int amount = g_deck.main_deck[i].amount;
    const card_ptr_t *card = &global_raw_cards_storage[g_deck.main_deck[i].csvid];
    switch (card->color) {
    case CP_COLOR_BLACK: black += amount; break;
    case CP_COLOR_BLUE: blue += amount; break;
    case CP_COLOR_GREEN: green += amount; break;
    case CP_COLOR_RED: red += amount; break;
    case CP_COLOR_WHITE: white += amount; break;
    }
    switch (card->card_type) {
    case CP_TYPE_LAND: land += amount; break;
    case CP_TYPE_CREATURE: creature += amount; break;
    case CP_TYPE_ARTIFACT: artifact += amount; break;
    case CP_TYPE_INSTANT:
    case CP_TYPE_INTERRUPT:
    case CP_TYPE_SORCERY:
      spell += amount;
      break;
    }
  }
  set_status_text("%d cards | B%d U%d G%d R%d W%d | Land%d Creature%d Spell%d Artifact%d",
                  g_deck.main_deck_cards, black, blue, green, red, white,
                  land, creature, spell, artifact);
  hold_status_for_frames(300);
}

static void show_deck_info(void)
{
  set_status_text("%s | %d unique | %d cards",
                  g_deck.deck_path, g_deck.main_deck_total, g_deck.main_deck_cards);
  hold_status_for_frames(300);
}

static void apply_main_command(DeckMainCommand command)
{
  switch (command) {
  case DECK_MAIN_STATS:
    show_deck_stats();
    break;
  case DECK_MAIN_INFO:
    show_deck_info();
    break;
  case DECK_MAIN_EXIT:
    g_deck.status = DECK_COMPLETED;
    break;
  }
}

static void apply_filter_command(DeckFilterCommand command)
{
  unsigned int bit = 0;
  switch (command) {
  case DECK_FILTER_ALL:
    g_deck.color_filters = 0;
    g_deck.type_filters = 0;
    g_deck.set_filters = DECK_SET_ALL;
    break;
  case DECK_FILTER_BLACK:
    bit = 1u << 0;
    g_deck.color_filters ^= bit;
    break;
  case DECK_FILTER_BLUE:
    bit = 1u << 1;
    g_deck.color_filters ^= bit;
    break;
  case DECK_FILTER_GREEN:
    bit = 1u << 2;
    g_deck.color_filters ^= bit;
    break;
  case DECK_FILTER_RED:
    bit = 1u << 3;
    g_deck.color_filters ^= bit;
    break;
  case DECK_FILTER_WHITE:
    bit = 1u << 4;
    g_deck.color_filters ^= bit;
    break;
  case DECK_FILTER_LAND:
    g_deck.type_filters ^= 1u << 0;
    break;
  case DECK_FILTER_CREATURE:
    g_deck.type_filters ^= 1u << 2;
    break;
  case DECK_FILTER_ENCHANTMENT:
    g_deck.type_filters ^= 1u << 3;
    break;
  case DECK_FILTER_INSTANT:
    g_deck.type_filters ^= 1u << 4;
    break;
  case DECK_FILTER_INTERRUPT:
    g_deck.type_filters ^= 1u << 5;
    break;
  case DECK_FILTER_SORCERY:
    g_deck.type_filters ^= 1u << 6;
    break;
  case DECK_FILTER_ARTIFACT:
    g_deck.type_filters ^= 1u << 1;
    break;
  case DECK_FILTER_FOURTH:
    g_deck.set_filters ^= DECK_SET_FOURTH;
    break;
  case DECK_FILTER_ASTRAL:
    g_deck.set_filters ^= DECK_SET_ASTRAL;
    break;
  case DECK_FILTER_ARABIAN:
    g_deck.set_filters ^= DECK_SET_ARABIAN;
    break;
  case DECK_FILTER_ANTIQUITIES:
    g_deck.set_filters ^= DECK_SET_ANTIQUITIES;
    break;
  case DECK_FILTER_LEGENDS:
    g_deck.set_filters ^= DECK_SET_LEGENDS;
    break;
  case DECK_FILTER_DARK:
    g_deck.set_filters ^= DECK_SET_DARK;
    break;
  case DECK_FILTER_SAVE:
    if (!save_deck_file(g_deck.deck_path)) {
      set_status_text("Could not save %s", g_deck.deck_path);
      hold_status_for_frames(180);
    }
    return;
  case DECK_FILTER_LOAD:
    if (!load_deck_file(g_deck.deck_path)) {
      set_status_text("Could not load %s", g_deck.deck_path);
      hold_status_for_frames(180);
    }
    return;
  case DECK_FILTER_CLEAR:
    clear_main_deck();
    set_status_text("Cleared deck");
    hold_status_for_frames(120);
    return;
  }
  build_available_cards();
}

static int hit_filter_button(int x, int y, DeckFilterCommand *command)
{
  int i;
  int button_x = DECK_FILTER_X + 8;
  if (y < DECK_FILTER_Y + 8 || y >= DECK_FILTER_Y + 36)
    return 0;
  for (i = 0; i < (int)(sizeof(k_filter_buttons) / sizeof(k_filter_buttons[0])); ++i) {
    const DeckFilterButton *button = &k_filter_buttons[i];
    if (!button->asset_name)
      continue;
    if (x >= button_x && x < button_x + button->width) {
      if (command)
        *command = button->command;
      return 1;
    }
    button_x += button->width + 6;
  }
  button_x = DECK_FILTER_X + DECK_FILTER_W - 204;
  for (i = 0; i < (int)(sizeof(k_filter_buttons) / sizeof(k_filter_buttons[0])); ++i) {
    const DeckFilterButton *button = &k_filter_buttons[i];
    if (button->asset_name)
      continue;
    if (x >= button_x && x < button_x + button->width) {
      if (command)
        *command = button->command;
      return 1;
    }
    button_x += button->width + 8;
  }
  return 0;
}

static int hit_main_button(int x, int y, DeckMainCommand *command)
{
  int i;
  if (y < DECK_BUTTON_Y || y >= DECK_BUTTON_Y + DECK_BUTTON_H)
    return 0;
  for (i = 0; i < (int)(sizeof(k_main_buttons) / sizeof(k_main_buttons[0])); ++i) {
    const DeckMainButton *button = &k_main_buttons[i];
    if (x >= button->left && x < button->right) {
      if (command)
        *command = button->command;
      return 1;
    }
  }
  return 0;
}

static void select_available_index(int index)
{
  if (index < 0)
    index = 0;
  if (index >= g_deck.available_count)
    index = g_deck.available_count - 1;
  g_deck.available_selected = index;
  update_ready_status();
}

void deck_pointer_move(float x, float y)
{
  g_deck.pointer_x = x;
  g_deck.pointer_y = y;
}

void deck_pointer_button(int button, int pressed, int clicks)
{
  int x = (int)g_deck.pointer_x;
  int y = (int)g_deck.pointer_y;
  DeckFilterCommand command;
  DeckMainCommand main_command;
  if (!core_is_interactive())
    return;
  if (!pressed)
    return;

  if (hit_main_button(x, y, &main_command)) {
    apply_main_command(main_command);
    return;
  }

  if (hit_filter_button(x, y, &command)) {
    apply_filter_command(command);
    return;
  }

  if (x >= DECK_FULLCARD_X && x < DECK_FULLCARD_X + DECK_FULLCARD_W &&
      y >= DECK_FULLCARD_Y && y < DECK_FULLCARD_Y + DECK_FULLCARD_H) {
    if (button == 1 && clicks >= 2)
      add_card_to_main_deck(selected_csvid(), 1);
    return;
  }

  if (x >= DECK_SURFACE_X && x < DECK_SURFACE_X + DECK_SURFACE_W &&
      y >= DECK_SURFACE_Y && y < DECK_SURFACE_Y + DECK_SURFACE_H) {
    int card_x[300];
    int card_y[300];
    int layout_count = 0;
    int lx = DECK_SURFACE_X + 10;
    int ly = DECK_SURFACE_Y + 10;
    int y_step = (DECK_CARD_H * 18) / 100;
    int i;
    for (i = 0; i < g_deck.main_deck_total; ++i) {
      if (ly + DECK_CARD_H > DECK_SURFACE_Y + DECK_SURFACE_H - 10) {
        lx += DECK_CARD_W + 8;
        ly = DECK_SURFACE_Y + 10;
      }
      if (lx + DECK_CARD_W > DECK_SURFACE_X + DECK_SURFACE_W - 8)
        break;
      card_x[layout_count] = lx;
      card_y[layout_count] = ly;
      ++layout_count;
      ly += y_step;
    }
    for (i = layout_count - 1; i >= 0; --i) {
      if (x >= card_x[i] && x < card_x[i] + DECK_CARD_W &&
          y >= card_y[i] && y < card_y[i] + DECK_CARD_H) {
        if (button == 1 && clicks >= 2)
          remove_card_from_main_deck(g_deck.main_deck[i].csvid, 1);
        return;
      }
    }
  }

}

void deck_key(int key, int pressed, int repeat)
{
  (void)repeat;
  if (!pressed || !core_is_interactive())
    return;
  switch (key) {
  case SDLK_RIGHT:
  case SDLK_DOWN:
    select_available_index(g_deck.available_selected + 1);
    break;
  case SDLK_LEFT:
  case SDLK_UP:
    select_available_index(g_deck.available_selected - 1);
    break;
  case SDLK_PAGEDOWN:
    select_available_index(g_deck.available_selected + DECK_BROWSER_PAGE_SIZE);
    break;
  case SDLK_PAGEUP:
    select_available_index(g_deck.available_selected - DECK_BROWSER_PAGE_SIZE);
    break;
  case SDLK_RETURN:
  case SDLK_KP_ENTER:
  case SDLK_SPACE:
    add_card_to_main_deck(selected_csvid(), 1);
    break;
  case SDLK_BACKSPACE:
  case SDLK_DELETE:
    remove_card_from_main_deck(selected_csvid(), 1);
    break;
  case SDLK_S:
    if (!save_deck_file(g_deck.deck_path)) {
      set_status_text("Could not save %s", g_deck.deck_path);
      hold_status_for_frames(180);
    }
    break;
  case SDLK_L:
    if (!load_deck_file(g_deck.deck_path)) {
      set_status_text("Could not load %s", g_deck.deck_path);
      hold_status_for_frames(180);
    }
    break;
  case SDLK_C:
    clear_main_deck();
    set_status_text("Cleared deck");
    hold_status_for_frames(120);
    break;
  case SDLK_ESCAPE:
    apply_main_command(DECK_MAIN_EXIT);
    break;
  case SDLK_0:
  case SDLK_A:
    apply_filter_command(DECK_FILTER_ALL);
    break;
  case SDLK_1:
  case SDLK_B:
    apply_filter_command(DECK_FILTER_BLACK);
    break;
  case SDLK_2:
    apply_filter_command(DECK_FILTER_BLUE);
    break;
  case SDLK_3:
  case SDLK_G:
    apply_filter_command(DECK_FILTER_GREEN);
    break;
  case SDLK_4:
  case SDLK_R:
    apply_filter_command(DECK_FILTER_RED);
    break;
  case SDLK_5:
  case SDLK_W:
    apply_filter_command(DECK_FILTER_WHITE);
    break;
  case SDLK_6:
    apply_filter_command(DECK_FILTER_LAND);
    break;
  case SDLK_7:
    apply_filter_command(DECK_FILTER_CREATURE);
    break;
  case SDLK_8:
    apply_filter_command(DECK_FILTER_ENCHANTMENT);
    break;
  case SDLK_9:
    apply_filter_command(DECK_FILTER_SORCERY);
    break;
  }
}

void deck_prefetch_cardart(void)
{
  int i;
  shandalar_assets_prefetch("MENUBAK.PIC");
  shandalar_assets_prefetch("CARDS.DAT");
  shandalar_assets_prefetch("DeckBuilder.csv");
  shandalar_assets_prefetch("DUELPALall.TR");
  shandalar_assets_prefetch("DUEL.plogpal");
  shandalar_assets_prefetch("TT0085M_.TTF");
  shandalar_assets_prefetch("TT0127M_.TTF");
  shandalar_assets_prefetch("TT0298M_.TTF");
  shandalar_assets_prefetch("TT0299M_.TTF");
  shandalar_assets_prefetch("TT0300M_.TTF");
  shandalar_assets_prefetch("TT0530M_.TTF");
  shandalar_assets_prefetch("CardArt/ManaSymbols.pic");
  shandalar_assets_prefetch("CardArt/CardSets.pic");
  shandalar_assets_prefetch("CardArt/CardBack.pic");
  shandalar_assets_prefetch("DBArt/DEKTILE4.PIC");
  shandalar_assets_prefetch("DBArt/DEKBAR1.PIC");
  shandalar_assets_prefetch("DBArt/DEKTIT1.PIC");
  shandalar_assets_prefetch("DBArt/BLDR01C.PIC");
  shandalar_assets_prefetch("DBArt/BLDR02C.PIC");
  shandalar_assets_prefetch("DBArt/BLDR03C.PIC");
  shandalar_assets_prefetch("DBArt/BLDR04C.PIC");
  shandalar_assets_prefetch("DBArt/BLDR05C.PIC");
  shandalar_assets_prefetch_small_art_cat();
  shandalar_assets_prefetch_med_art_cat();
  for (i = 0; i < DECK_ICON_FILTER_COUNT; ++i)
    shandalar_assets_prefetch(k_filter_buttons[i].asset_name);
}
