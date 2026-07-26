#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef MODERN_FIXES
#include "platform/winuser_compat.h"
#endif

#include "defs.h"
#include "shandalar.h"
#include "magic/src/global_state.h"
#include "shandalar_global_strings.h"
#include "facemaker/src/facemaker_types.h"
#include "drawcardlib/src/pic.h"

// GLOBAL: SHANDALAR 0x0059076c
int g_world_ui_top_offset = 128;
// GLOBAL: SHANDALAR 0x00590760
int g_world_ui_stats_color_index = 0xca;
// GLOBAL: SHANDALAR 0x00590770
unsigned char g_location_marker_sprite_lookup_by_town[0x24] = {
    0x01,
    0x07,
    0x05,
    0x0b,
    0x1c,
    0x20,
    0x19,
    0x1d,
    0x00,
    0x06,
    0x23,
    0x24,
    0x0d,
    0x13,
    0x21,
    0x22,
    0x02,
    0x08,
    0x0c,
    0x12,
    0x0e,
    0x14,
    0x1a,
    0x1e,
    0x1b,
    0x1f,
    0x26,
    0x2c,
    0x27,
    0x2d,
    0x34,
    0x3a,
    0x03,
    0x09,
    0x04,
    0x0a,
};
// GLOBAL: SHANDALAR 0x005907a8
int g_dungeon_marker_sprite_lookup[24] = {
    0x2a,
    0x30,
    0x3f,
    0x43,
    0x36,
    0x3c,
    0x35,
    0x3b,
    0x32,
    0x38,
    0x33,
    0x39,
    0x4e,
    0x54,
    0x50,
    0x56,
    0x4f,
    0x55,
    0x37,
    0x3d,
    0x4d,
    0x53,
    0x4c,
    0x52,
};
// GLOBAL: SHANDALAR 0x005907a0
int g_world_ui_cached_next_duel_life_delta = 0;
// GLOBAL: SHANDALAR 0x005907a4
int g_world_ui_cached_next_duel_card_id = -1;
// GLOBAL: SHANDALAR 0x0078cee0
int g_current_quest_giver_town_index;
// GLOBAL: SHANDALAR 0x00590808
unsigned char g_castle_sprite_lookup_by_tile_class[0x40] = {
    0x08,
    0x09,
    0x0a,
    0x0b,
    0x02,
    0x06,
    0x03,
    0x07,
    0x0c,
    0x10,
    0x0d,
    0x11,
    0x0e,
    0x12,
    0x0f,
    0x13,
    0x00,
    0x04,
    0x01,
    0x05,
    0x00,
    0x00,
    0x00,
    0x00,
    0x08,
    0x09,
    0x0a,
    0x0b,
    0x02,
    0x06,
    0x03,
    0x07,
    0x0c,
    0x10,
    0x0d,
    0x11,
    0x0e,
    0x12,
    0x0f,
    0x13,
    0x00,
    0x04,
    0x01,
    0x05,
    0x00,
    0x00,
    0x00,
    0x00,
    0xd2,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0x00,
    0xf4,
    0x00,
    0x00,
    0x00,
    0xce,
    0x00,
    0x00,
    0x00,
};
// GLOBAL: SHANDALAR 0x00652740
char g_world_ui_text_scratch_buffer[0x1000];
// GLOBAL: SHANDALAR 0x006696c0
char g_creature_name_with_article_buffer[0x80];
// GLOBAL: SHANDALAR 0x00590798
signed char g_lair_sprite_index_by_color[8] = {0, 4, 0, 3, 2, 1, 0, 0};

// GLOBAL: SHANDALAR 0x006506e4
int g_world_camera_initialized;
// GLOBAL: SHANDALAR 0x006506ec
int g_world_camera_viewport_top;
// GLOBAL: SHANDALAR 0x006506f0
int g_world_draw_entry_count;
// GLOBAL: SHANDALAR 0x006506f4
int g_world_draw_head_index;
// GLOBAL: SHANDALAR 0x006506f8
int g_world_camera_viewport_left;
// GLOBAL: SHANDALAR 0x006506fc
int g_world_camera_anchor_world_x;
// GLOBAL: SHANDALAR 0x00650700
int g_world_draw_depth_sort_keys[0x200];
// GLOBAL: SHANDALAR 0x00650f00
int g_world_camera_scroll_limit_left;
// GLOBAL: SHANDALAR 0x00650f04
ShandalarEntryType g_world_ui_cached_siege_entry_type;
// GLOBAL: SHANDALAR 0x00650f08
int g_world_camera_viewport_width_pixels;
// GLOBAL: SHANDALAR 0x00650f0c
int g_world_camera_minimap_panel_x;
// GLOBAL: SHANDALAR 0x00650f10
int g_world_camera_origin_screen_x;
// GLOBAL: SHANDALAR 0x00650f14
int g_world_player_tile_x;
// GLOBAL: SHANDALAR 0x00650f18
int g_world_player_tile_y;
// GLOBAL: SHANDALAR 0x00650f24
int g_world_camera_viewport_height_scaled;
// GLOBAL: SHANDALAR 0x00650f2c
int g_world_camera_scroll_limit_bottom;
// GLOBAL: SHANDALAR 0x00650f30
int g_world_camera_viewport_height_pixels;
// GLOBAL: SHANDALAR 0x00650f34
int g_world_camera_minimap_panel_y;
// GLOBAL: SHANDALAR 0x00650f38
int g_world_camera_scroll_limit_top;
// GLOBAL: SHANDALAR 0x00650f40
EncodedImage *g_world_draw_sprites[0x200];
// GLOBAL: SHANDALAR 0x00651740
int g_world_draw_screen_x[0x200];
// GLOBAL: SHANDALAR 0x00651f40
int g_world_draw_screen_y[0x200];
// GLOBAL: SHANDALAR 0x006527a8
int g_world_camera_viewport_width_scaled;
// GLOBAL: SHANDALAR 0x006527ac
int g_world_camera_anchor_world_y;
// GLOBAL: SHANDALAR 0x006527b4
int g_world_camera_origin_screen_y;
// GLOBAL: SHANDALAR 0x006527b8
int g_world_camera_scroll_limit_right;
// GLOBAL: SHANDALAR 0x006527c0
int g_world_draw_next_index[0x200];
// GLOBAL: SHANDALAR 0x0073ea98
int g_world_camera_last_anchor_x;
// GLOBAL: SHANDALAR 0x0073ea9c
int g_world_camera_last_anchor_y;
// GLOBAL: SHANDALAR 0x0073e9d0
int g_world_player_tile_x;
// GLOBAL: SHANDALAR 0x0073e9d4
int g_world_player_tile_y;

// GLOBAL: SHANDALAR 0x00590a48
AdvMenuRect g_adventure_interface_world_magic_icon_rects[0xc] = {
    {22, 427, 48, 48},
    {113, 427, 48, 48},
    {28, 378, 49, 49},
    {205, 427, 48, 48},
    {112, 378, 49, 49},
    {297, 427, 48, 48},
    {195, 378, 49, 49},
    {385, 427, 48, 48},
    {274, 378, 49, 49},
    {474, 427, 48, 48},
    {354, 378, 49, 49},
    {567, 427, 48, 48},
};

// TODO: why does g_world_tile_screen_x_cache need to be oversize? it isn't resolution dependant
#ifdef MODERN_FIXES
int g_world_tile_screen_x_cache[0x10000];
int g_world_tile_screen_y_cache[0x10000];
#else
// GLOBAL: SHANDALAR 0x0073eaf0
int g_world_tile_screen_x_cache[0x1000];
// GLOBAL: SHANDALAR 0x00742af0
int g_world_tile_screen_y_cache[0x1000];
#endif

// GLOBAL: SHANDALAR 0x00746af0
int g_world_scroll_x;
// GLOBAL: SHANDALAR 0x00746af4
int g_world_scroll_y;
// GLOBAL: SHANDALAR 0x00746af8
int g_world_player_world_x_cached;
// GLOBAL: SHANDALAR 0x00746afc
int g_world_player_world_y_cached;

typedef union
{
  struct
  {
    EncodedImage *normal[12];
    EncodedImage *highlight[12];
    EncodedImage *pressed[12];
    EncodedImage *icon[12];
  } named;
  EncodedImage *by_group[4][12];
} WorldMagicChoiceButtonSpriteBank;

extern int global_screen_width;
extern int global_screen_height;
extern FacemakerWindowBounds *g_page0_window_bounds;
extern FacemakerWindowBounds *g_page1_window_bounds;
extern FacemakerWindowBounds *g_page3_window_bounds;
extern FacemakerWindowBounds *g_page2_window_bounds;
extern int g_adventure_ui_layout_dirty;
extern int g_world_player_animation_direction;
extern int g_world_player_animation_frame;
extern int g_world_scene_reveal_effect_pending;
extern int g_world_scroll_cache_ready;
extern int g_monster_timer;
extern int g_skip_world_sfx_preload;
extern int g_mouse_x;
extern int g_mouse_y;
extern int g_mouse_button_down_mask;
extern char g_ui_message_buffer[0x1000];
extern int g_ego_sprite_width;
extern int g_ego_sprite_draw_height;
extern int g_sego_sprite_width;
extern int g_sego_sprite_draw_height;
extern int g_neighbor_dx[9];
extern int g_neighbor_dy[9];
extern EncodedImage *g_location07_sprite_entries[0xc];
extern EncodedImage *g_road_sprite_entries[0xc];
extern EncodedImage *g_land_sprite_entries[22][5];
extern EncodedImage *g_sland_sprite_entries[22][5];
extern EncodedImage *g_land_tile_sprite_entries[0x10];
extern EncodedImage *g_cstline1_sprite_entries[12][7];
extern EncodedImage *g_location_marker_sprite_entries[0x9e];
extern int g_world_lair_monster_sprite_top_clips[0x10];
extern EncodedImage *g_questnew_sprite_entries[4];
extern EncodedImage *g_clocknew_sprite_entries[9];
extern EncodedImage *g_sunmoon_sprite_entries[0x14];
extern EncodedImage *g_daysnew_sprite_entries[0xc];
extern EncodedImage *g_tips_frame_sprite;
extern EncodedImage *g_tips_icon_sprite;
extern int g_world_lair_monster_sprite_widths[0x10];
extern DIBSurface *g_graphics_pages[10];
extern OpeningMenuSpriteWorkEntry g_opening_menu_sprite_work_buffer[0x20];
extern EncodedImage *g_world_magic_avatar_sprites[5];
extern EncodedImage *g_worlds_extra_sprite_entries[4];
extern EncodedImage *g_main_menu_button_sprites_normal[4];
extern EncodedImage *g_main_menu_button_sprites_highlight[4];
extern EncodedImage *g_castles_sprite_entries[20];
extern EncodedImage *g_wizard_controlled_town_sprite_entries[5];
extern EncodedImage *g_current_location_marker_sprite;
extern WorldMagicChoiceButtonSpriteBank g_world_magic_choice_button_sprite_bank;

extern int g_next_duel_life_delta;
extern int g_next_duel_card_id;
extern int Gold;
extern int g_food;

extern int g_deck_total_card_count;
extern int g_deck_active_card_count;
extern int g_hide_world_map_overlays;
extern card_data_t global_cards_data[];
extern char g_ini_string_scratch[0x28];
extern char g_itoa_buffer[0x20];

void ShowMouseCursorNested(void);
void HideMouseCursorNested(void);
int ScaleUiCoordinateFrom320(int value);
AdvMenuRect *PushGraphicsClipRect(AdvMenuRect *saved_clip_rect, FacemakerWindowBounds *page, int x, int y, int width, int height);
void BlitGraphicsRect(FacemakerWindowBounds *dst, unsigned int dst_x, int dst_y, unsigned int width, DWORD height,
                      FacemakerWindowBounds *src, int src_x, int src_y);
void DrawEncodedImageResampled(FacemakerWindowBounds *dst, int x, int y, int width, int height, EncodedImage *encoded_image);
void DrawEncodedImageUnscaled(FacemakerWindowBounds *dst, int x, int y, EncodedImage *encoded_image);
void DrawEncodedImageUnscaledClipped(FacemakerWindowBounds *dst, int x, int y, EncodedImage *encoded_image);
void DrawEncodedImageUiScaled(FacemakerWindowBounds *dst, int x_320, int y_200, EncodedImage *sprite, int width_320, int height_200);
void FillGraphicsRect(FacemakerWindowBounds *window_bounds, int x, int y, int width, int height, unsigned int color_index);
int DrawTextFormatted(FacemakerWindowBounds *dst, int text_color, int parse_format, int centered, int draw_shadow, int multiline, int x, int y, int *arg_ptr);
void DrawTextAt(FacemakerWindowBounds *window, int color, int x, int y, char *text, ...);
void DrawWorldUiFormattedText(FacemakerWindowBounds *window, int color_index, int x, int y, char *format, ...);
unsigned int BlitRectByRandomTileOrder(HDC dst_hdc, int dst_x, int dst_y, int width, int height, int block_w, int block_h,
                                       HDC src_hdc, int src_x, int src_y);
void DrawFormattedTextShadowedCentered(FacemakerWindowBounds *window, int color_index, int x, int y, char *format, ...);
void DelayUiTicks(int ticks);
int ApproximateDistance(int x, int y);
int FindWorldMagicCardIndex(int world_magic_slot_index);
int single_color_test_bit_to_color_t(int mask);
unsigned int GetWorldTileType(int x, int y);
unsigned int GetWorldMapPixelFlags(int x, int y);
void SetWorldMapPixelFlags(unsigned int mask, int x, int y);
unsigned int GetWorldTileMagicMask(unsigned int tile_mask);
unsigned int GetGraphicsPixelColorRef(FacemakerWindowBounds *window, int x, int y);
void QueueWorldSpriteForDraw(FacemakerWindowBounds *window, int draw_x, int draw_y, int depth_y, EncodedImage *sprite);
void DrawWorldTileRange(unsigned int world_x, unsigned int world_y, int tile_x_start, int tile_x_end, int tile_y_start, int tile_y_end,
                        int edge_mode, int draw_mode);
void ScreenToWorldPoint(int screen_x, int screen_y, int *out_world_x, int *out_world_y);
int IsWorldMapCoordinateInBounds(int x, int y);
int FindCastleDungeonAtWorldCoordinates(int world_x, int world_y);
int FindTownAtWorldCoordinates(int world_x, int world_y);
char *BuildQuestLocationName(int town_index, int quest_destination, int mana_castle_index);
void WorldPointToScreen(int world_x, int world_y, int *out_screen_x, int *out_screen_y);
void RefreshAdventureInterfaceLayout(void);
int RunTextMenuAtScaled(char *menu_text, int x_320_scale, unsigned int y_200_scale);
int CountDuelPoolEligibleTowns(void);
void AddJournalEntry(int entry_type, int entry_arg);
int GetRelativeWorldQuadrant(int world_x, int world_y);
int FindDeckSlotForQuestColorAndType(unsigned char quest_color, int quest_bitmap_mask);
char *GetTownCardDescription(int town_index);
char *GetQuestCardClassName(int quest_bitmap_mask);
int FindNearestTownIndex(int world_x, int world_y);
char *BuildCreatureNameWithArticle(int creature_type);
char *BuildTownDisplayName(int town_index);
DWORD FormatMessageFromStringStripCarriageReturns(char *dst, DWORD max_length, LPCVOID format, ...);
int IsAdventureRightClickMenuButtonHit(int x, int y);
void DrawAdventureRightClickMenuButton(void);

// FUNCTION: SHANDALAR 0x005501dc
int ScaleUiCoordinateFrom320(int value)
{
  return (value * global_screen_width) / 0x140;
}

// FUNCTION: SHANDALAR 0x004bb0ca
int FindNearestTownIndex(int world_x, int world_y)
{
  int nearest_town_distance;
  int nearest_town_index;
  int town_index;
  int town_distance;

  nearest_town_index = -1;
  nearest_town_distance = 0x7fff;
  for (town_index = 0; town_index < 0x80; town_index = town_index + 1)
  {
    if (g_town_slots[town_index].location_type == -1)
    {
      continue;
    }
    town_distance = ApproximateDistance(g_town_slots[town_index].world_x - world_x, g_town_slots[town_index].world_y - world_y);
    if (town_distance < nearest_town_distance)
    {
      nearest_town_distance = town_distance;
      nearest_town_index = town_index;
    }
  }
  return nearest_town_index;
}

// FUNCTION: SHANDALAR 0x00550220
char *BuildTownDisplayName(int town_index)
{
  int second_half_index;
  int first_half_index;

  second_half_index = town_index / 8 % 0x10;
  if (second_half_index < 0)
  {
    second_half_index = 0;
  }
  if (second_half_index > 0xf)
  {
    second_half_index = 0xf;
  }

  first_half_index = town_index % 0x10;
  if (first_half_index < 0)
  {
    first_half_index = 0;
  }
  if (first_half_index > 0xf)
  {
    first_half_index = 0xf;
  }

  if (g_town_slots[town_index].location_type != 1)
  {
    FormatMessageFromStringStripCarriageReturns(g_world_ui_text_scratch_buffer, 0x1000, gs_cityname_format_left_0074c950, gs_citynames_firsthalf_0077e060[second_half_index],
                                                gs_citynames_secondhalf_007653e0[first_half_index]);
  }
  else
  {
    FormatMessageFromStringStripCarriageReturns(g_world_ui_text_scratch_buffer, 0x1000, gs_cityname_format_right_0077f190, gs_citynames_firsthalf_0077e060[second_half_index]);
  }
  return g_world_ui_text_scratch_buffer;
}

// FUNCTION: SHANDALAR 0x004f2e17
char *GetTownCardDescription(int town_index)
{
  int card_color_index;
  int city_card_class;
  char *city_card_text;

  card_color_index = single_color_test_bit_to_color_t(g_town_slots[town_index].trade_color_and_type & 0xff);
  city_card_class = g_town_slots[town_index].trade_color_and_type >> 8;
  if (city_card_class == 7)
  {
    city_card_class = 6;
  }
  if ((g_town_slots[town_index].trade_color_and_type & 0xff))
  {
    city_card_text = gs_citycardtext_0074ccf0[card_color_index * 7 + city_card_class];
  }
  else
  {
    city_card_text = gs_citycardtext_0074ccf0[city_card_class + 0x2a];
  }
  return city_card_text;
}

// FUNCTION: SHANDALAR 0x004f21bb
int GetRelativeWorldQuadrant(int world_x, int world_y)
{
  int delta_y;
  int delta_x;
  int quadrant_index;

  delta_x = world_x - g_world_player_tile_x;
  delta_y = world_y - g_world_player_tile_y;
  if (delta_y > 0)
  {
    if (0 < delta_x)
    {
      quadrant_index = 1;
    }
    else
    {
      quadrant_index = 2;
    }
  }
  else
  {
    if (0 < delta_x)
    {
      quadrant_index = 0;
    }
    else
    {
      quadrant_index = 3;
    }
  }
  return quadrant_index;
}

// FUNCTION: SHANDALAR 0x004f2322
int FindDeckSlotForQuestColorAndType(unsigned char quest_color, int quest_bitmap_mask)
{
  int deck_slot_index;
  int card_id;

  for (deck_slot_index = 0; deck_slot_index < 500; ++deck_slot_index)
  {
    if (deck[deck_slot_index] != -1)
    {
      card_id = deck[deck_slot_index] & 0xfff;
      if (((1 << quest_color) & (int)(char)global_cards_data[card_id].color) != 0)
      {
        if ((quest_bitmap_mask & (unsigned char)global_cards_data[card_id].type) != 0)
        {
          return deck_slot_index + 1;
        }
      }
    }
  }
  return 0;
}

// FUNCTION: SHANDALAR 0x00428615
char *GetQuestCardClassName(int quest_bitmap_mask)
{
  char *result;

  switch (quest_bitmap_mask)
  {
  case 1:
    result = gs_cardclassnames_0077cf70[0];
    break;
  case 2:
    result = gs_cardclassnames_0077cf70[1];
    break;
  case 4:
    result = gs_cardclassnames_0077cf70[2];
    break;
  case 8:
    result = gs_cardclassnames_0077cf70[3];
    break;
  case 0x10:
    result = gs_cardclassnames_0077cf70[4];
    break;
  case 0x20:
    result = gs_cardclassnames_0077cf70[5];
    break;
  case 0x40:
    result = gs_cardclassnames_0077cf70[6];
    break;
  case 0x42:
    result = gs_cardclassnames_0077cf70[7];
    break;
  case 0x80:
    result = gs_cardclassnames_0077cf70[8];
    break;
  default:
    result = "";
    break;
  }
  return result;
}

// FUNCTION: SHANDALAR 0x00561465
char *BuildCreatureNameWithArticle(int creature_type)
{
  strcpy(g_creature_name_with_article_buffer, g_shandalar_monster_definitions[creature_type].article);
  strcat(g_creature_name_with_article_buffer, g_shandalar_monster_definitions[creature_type].name);
  return g_creature_name_with_article_buffer;
}

// FUNCTION: SHANDALAR 0x00564e70
DWORD FormatMessageFromStringStripCarriageReturns(char *dst, DWORD max_length, LPCVOID format, ...)
{
  DWORD result;
  va_list args;
  char *carriage_return;

  va_start(args, format);
  result = FormatMessageA(0x400, format, 0, 0, dst, max_length, &args);
  va_end(args);
  while ((carriage_return = strchr(dst, '\r')) != (char *)0)
  {
    strcpy(carriage_return, carriage_return + 1);
  }
  return result;
}

// FUNCTION: SHANDALAR 0x00550416
char *BuildQuestLocationName(int town_index, int quest_destination, int mana_castle_index)
{
  int city_name_index = (town_index / 8) % 16;
  int sign_adjust;
  int location_type;
  if (city_name_index < 0)
  {
    city_name_index = 0;
  }
  if (city_name_index > 15)
  {
    city_name_index = 15;
  }
  sign_adjust = town_index % 16;
  if (sign_adjust < 0)
  {
    sign_adjust = 0;
  }
  if (sign_adjust > 15)
  {
    sign_adjust = 15;
  }

  location_type = g_town_slots[town_index].location_type;
  if ((location_type > 1) || (town_index == quest_destination))
  {
    if (location_type == 4)
    {
      sprintf(g_world_ui_text_scratch_buffer, (char *)gs_cityname_manacastle_0077de00 + mana_castle_index * 100);
    }
    else if (location_type > 1)
    {
      FormatMessageFromStringStripCarriageReturns(g_world_ui_text_scratch_buffer, 0x1000, gs_cityname_format_left_0074c950, gs_citynames_firsthalf_0077e060[city_name_index], gs_citynames_secondhalf_007653e0[sign_adjust]);
    }
    else if (location_type == 1)
    {
      FormatMessageFromStringStripCarriageReturns(g_world_ui_text_scratch_buffer, 0x1000, gs_cityname_format_right_0077f190, gs_citynames_firsthalf_0077e060[city_name_index]);
    }
    else
    {
      strcpy(g_world_ui_text_scratch_buffer, gs_citynames_firsthalf_0077e060[city_name_index]);
    }
  }
  else
  {
    strcpy(g_world_ui_text_scratch_buffer, gs_cityname_village_0077cfd0);
  }
  return g_world_ui_text_scratch_buffer;
}

// FUNCTION: SHANDALAR 0x0054cee2
void UpdateWorldViewportBuffer(int world_x, int world_y)
{
  int clip_dst_y;
  int clip_dst_x;
  int clip_mode_x;
  int clip_height;
  int clip_width;
  int clip_src_y;
  int clip_src_x;
  int clip_mode_y;
  int any_edge_needed;
  int need_top_edge;
  int need_left_edge;
  int need_right_edge;
  int need_bottom_edge;
  int tile_x;
  int scroll_delta_y;
  int origin_screen_x;
  int scroll_delta_x;
  int redraw_x_end;
  int snapped_screen_y;
  int redraw_y_end;
  int snapped_world_y;
  int snapped_screen_x;
  int redraw_y_start;
  int redraw_x_start;
  int snapped_world_x;
  int origin_screen_y;
  int saved_page_number;
  int world_screen_y;
  int world_screen_x;
  int tile_y;

  saved_page_number = g_page0_window_bounds->page_number;
  g_page0_window_bounds->page_number = 2;

  g_world_player_world_x_cached = world_x;
  g_world_player_world_y_cached = world_y;

  snapped_world_x = world_x - (world_x & 0x1f);
  snapped_world_y = world_y - (world_y & 0x1f);
  WorldPointToScreen(snapped_world_x, snapped_world_y, &snapped_screen_x, &snapped_screen_y);
  WorldPointToScreen(world_x, world_y, &world_screen_x, &world_screen_y);
  WorldPointToScreen(0, 0, &origin_screen_x, &origin_screen_y);

  if ((g_world_camera_initialized != 0) && (g_world_scroll_cache_ready != 0))
  {
    need_left_edge = 0;
    need_right_edge = 0;
    need_top_edge = 0;
    need_bottom_edge = 0;

    tile_x = world_x / 32;
    tile_y = world_y / 32;
    scroll_delta_x = g_world_tile_screen_x_cache[(tile_y << 8) + tile_x] - snapped_screen_x;
    g_world_scroll_x = scroll_delta_x;
    scroll_delta_y = g_world_tile_screen_y_cache[(tile_y << 8) + tile_x] - snapped_screen_y;
    g_world_scroll_y = scroll_delta_y;

    if ((scroll_delta_x != 0) || (scroll_delta_y != 0))
    {
      if (scroll_delta_x < 0 && g_world_camera_scroll_limit_left >= scroll_delta_x)
      {
        need_left_edge = 1;
      }
      if (scroll_delta_x > 0 && g_world_camera_scroll_limit_right <= scroll_delta_x)
      {
        need_right_edge = 1;
      }
      if ((0 < scroll_delta_y) && (g_world_camera_scroll_limit_bottom <= scroll_delta_y))
      {
        need_bottom_edge = 1;
      }
      if ((scroll_delta_y < 0) && (scroll_delta_y <= g_world_camera_scroll_limit_top))
      {
        need_top_edge = 1;
      }
    }

    any_edge_needed = need_bottom_edge | need_top_edge | need_right_edge | need_left_edge;
    if (any_edge_needed == 0)
    {
      BlitGraphicsRect(g_page0_window_bounds, g_world_camera_viewport_left + scroll_delta_x, g_world_camera_viewport_top + scroll_delta_y,
                       g_world_camera_viewport_width_scaled, g_world_camera_viewport_height_scaled,
                       g_page1_window_bounds, 0x40, g_world_camera_minimap_panel_y * 2 + g_world_ui_top_offset + 0x10);

      g_page0_window_bounds->page_number = saved_page_number;
      DrawWorldTileRange(g_world_camera_anchor_world_x, g_world_camera_anchor_world_y, -5, 4, -7, 9, 0, 1);
      g_world_player_world_x_cached = world_x;
      g_world_player_world_y_cached = world_y;
      return;
    }
    else
    {
      clip_width = g_page0_window_bounds->max_x;
      clip_height = g_page0_window_bounds->max_y;
      clip_mode_x = 0;
      clip_mode_y = 0;

      DrawWorldTileRange(world_x, world_y, -5, 4, -7, 9, 0, 1);

      if (scroll_delta_x > 0)
      {
        clip_dst_x = scroll_delta_x;
        clip_src_x = 0;
        clip_width = g_page0_window_bounds->max_x - scroll_delta_x;
        clip_mode_x = 2;
      }
      else if (scroll_delta_x < 0)
      {
        clip_dst_x = 0;
        clip_src_x = -scroll_delta_x;
        clip_width = g_page0_window_bounds->max_x - -scroll_delta_x;
        clip_mode_x = 1;
      }
      else
      {
        clip_dst_x = 0;
        clip_src_x = 0;
        clip_width = g_page0_window_bounds->max_x;
        clip_mode_x = -1;
      }

      if (scroll_delta_y > 0)
      {
        clip_dst_y = scroll_delta_y + 0x80;
        clip_src_y = 0x80;
        clip_height = (g_page0_window_bounds->max_y - scroll_delta_y) - 0x80;
        clip_mode_y = 2;
      }
      else if (scroll_delta_y < 0)
      {
        clip_dst_y = 0x80;
        clip_src_y = 0x80 - scroll_delta_y;
        clip_height = (g_page0_window_bounds->max_y - -scroll_delta_y) - 0x80;
        clip_mode_y = 1;
      }
      else
      {
        clip_dst_y = 0x80;
        clip_src_y = 0x80;
        clip_height = g_page0_window_bounds->max_y - 0x80;
        clip_mode_y = -1;
      }

      BlitGraphicsRect(g_page0_window_bounds, clip_dst_x, clip_dst_y, clip_width, (DWORD)clip_height,
                       g_page0_window_bounds, clip_src_x, clip_src_y);

      if (clip_mode_x == 1)
      {
        DrawWorldTileRange(world_x, world_y, -3, -1, -7, 7, 1, 0);
        if (clip_mode_y == 1)
        {
          DrawWorldTileRange(world_x, world_y, -1, 4, -7, -3, 1, 0);
        }
        else if (clip_mode_y == 2)
        {
          DrawWorldTileRange(world_x, world_y, -1, 4, 3, 7, 1, 0);
        }
      }
      else if (clip_mode_x == 2)
      {
        DrawWorldTileRange(world_x, world_y, 2, 5, -7, 7, 1, 0);
        if (clip_mode_y == 1)
        {
          DrawWorldTileRange(world_x, world_y, -3, 2, -7, -3, 1, 0);
        }
        else if (clip_mode_y == 2)
        {
          DrawWorldTileRange(world_x, world_y, -3, 2, 3, 7, 1, 0);
        }
      }
      else if (clip_mode_y == 1)
      {
        DrawWorldTileRange(world_x, world_y, -3, 5, -7, -3, 1, 0);
      }
      else if (clip_mode_y == 2)
      {
        DrawWorldTileRange(world_x, world_y, -3, 5, 3, 7, 1, 0);
      }

      DrawWorldTileRange(world_x, world_y, -4, 6, -9, 0xb, 2, 0);

      g_world_camera_origin_screen_x = origin_screen_x;
      g_world_camera_origin_screen_y = origin_screen_y;
      g_world_camera_anchor_world_x = world_x;
      g_world_camera_anchor_world_y = world_y;
      g_world_scroll_x = 0;
      g_world_scroll_y = 0;
      g_world_camera_last_anchor_x = world_x;
      g_world_camera_last_anchor_y = world_y;
    }
  }
  else
  {
    redraw_y_start = -9;
    redraw_x_end = 9;
    redraw_x_start = -4;
    redraw_y_end = 6;

    g_world_camera_origin_screen_x = origin_screen_x;
    g_world_camera_origin_screen_y = origin_screen_y;
    g_world_camera_anchor_world_x = world_x;
    g_world_camera_anchor_world_y = world_y;
    g_world_scroll_x = 0;
    g_world_scroll_y = 0;
    g_world_camera_last_anchor_x = world_x;
    g_world_camera_last_anchor_y = world_y;
    g_world_camera_viewport_left = 0x40;
    g_world_camera_viewport_top = g_world_camera_minimap_panel_y * 2 + g_world_ui_top_offset + 0x10;
    g_world_camera_viewport_width_scaled = ScaleUiCoordinateFrom320(0x100);
    g_world_camera_viewport_height_scaled = ScaleUiCoordinateFrom320(0x8c);
    g_world_camera_scroll_limit_left = -0x40;
    g_world_camera_scroll_limit_right = 0x40;
    g_world_camera_scroll_limit_top = 0x80 - g_world_camera_viewport_top;
    g_world_camera_scroll_limit_bottom = g_page0_window_bounds->max_y - (g_world_camera_viewport_top + g_world_camera_viewport_height_scaled);
    g_world_camera_initialized = 1;
    g_world_scroll_cache_ready = 1;

    DrawWorldTileRange(world_x, world_y, redraw_x_start, redraw_y_end, redraw_y_start, redraw_x_end, 1, 0);
    DrawWorldTileRange(world_x, world_y, redraw_x_start - 2, redraw_y_end, redraw_y_start, redraw_x_end + 2, 2, 1);
  }

  g_world_player_world_x_cached = world_x;
  g_world_player_world_y_cached = world_y;
  BlitGraphicsRect(g_page0_window_bounds, g_world_camera_viewport_left, g_world_camera_viewport_top,
                   g_world_camera_viewport_width_scaled, g_world_camera_viewport_height_scaled,
                   g_page1_window_bounds, 0x40, g_world_camera_minimap_panel_y * 2 + g_world_ui_top_offset + 0x10);
  g_page0_window_bounds->page_number = saved_page_number;
}

// FUNCTION: SHANDALAR 0x0054d619
void DrawWorldTileRange(unsigned int world_x, unsigned int world_y, int tile_x_start, int tile_x_end, int tile_y_start, int tile_y_end,
                        int edge_mode, int draw_mode)
{
  int terrain_edge_x_adjust;           /* ebp-0xb4 */
  int terrain_edge_y_adjust;           /* ebp-0xb0 */
  int terrain_draw_x_adjust;           /* ebp-0xac */
  int terrain_draw_y_adjust;           /* ebp-0xa8 */
  int terrain_draw_base_y_adjust;      /* ebp-0xa4 */
  int coastline_bits_before_dec;       /* ebp-0xa0 */
  int road_bits_before_dec;            /* ebp-0x9c */
  EncodedImage *marker_sprite;         /* ebp-0x98 */
  EncodedImage *town_marker_sprite;    /* ebp-0x94 */
  int town_marker_height;              /* ebp-0x90 */
  int wizard_index;                    /* ebp-0x8c */
  EncodedImage *dungeon_marker_sprite; /* ebp-0x88 */
  EncodedImage *second_terrain_sprite; /* ebp-0x84 */
  EncodedImage *first_terrain_sprite;  /* ebp-0x80 */
  int tile_x;                          /* ebp-0x7c */
  int terrain_offset_y;                /* ebp-0x78 */
  int tile_screen_x;                   /* ebp-0x74 */
  int tile_base_screen_y;              /* ebp-0x70 */
  int terrain_flip_side;               /* ebp-0x6c */
  int terrain_pattern;                 /* ebp-0x68 */
  unsigned int neighbor_tile_class;    /* ebp-0x64 */
  int terrain_offset_x;                /* ebp-0x60 */
  unsigned int neighbor_tile_type;     /* ebp-0x5c */
  int terrain_column;                  /* ebp-0x58 */
  unsigned int tile_mask;              /* ebp-0x54 */
  int snapped_screen_y;                /* ebp-0x50 */
  unsigned int tile_class;             /* ebp-0x4c */
  int snapped_world_y;                 /* ebp-0x48 */
  int snapped_screen_x;                /* ebp-0x44 */
  unsigned int road_mask;              /* ebp-0x40 */
  int scratch_index;                   /* ebp-0x3c */
  int i;                               /* ebp-0x38 */
  int snapped_world_x;                 /* ebp-0x34 */
  int coastline_bits;                  /* ebp-0x30 */
  int terrain_row;                     /* ebp-0x2c */
  unsigned int tile_type;              /* ebp-0x28 */
  unsigned int road_bits;              /* ebp-0x24 */
  int tile_screen_y;                   /* ebp-0x20 */
  int saved_page_number;               /* ebp-0x1c */
  int world_tile_y;                    /* ebp-0x18 */
  int town_index;                      /* ebp-0x14 */
  int world_tile_x;                    /* ebp-0x10 */
  int road_connection_bits;            /* ebp-0x0c */
  int terrain_height;                  /* ebp-0x08 */
  int tile_y;                          /* ebp-0x04 */

  saved_page_number = g_page0_window_bounds->page_number;
  g_page0_window_bounds->page_number = 2;

  g_world_player_world_x_cached = world_x;
  g_world_player_world_y_cached = world_y;
  snapped_world_x = (int)world_x - ((int)world_x & 0x1f);
  snapped_world_y = (int)world_y - ((int)world_y & 0x1f);
  WorldPointToScreen(snapped_world_x, snapped_world_y, &snapped_screen_x, &snapped_screen_y);
  for (tile_y = tile_y_start; tile_y < tile_y_end; tile_y++)
  {
    for (tile_x = tile_x_start; tile_x < tile_x_end; tile_x++)
    {
      tile_screen_x = tile_x * g_world_camera_minimap_panel_x + snapped_screen_x;
      tile_screen_y = tile_y * g_world_camera_minimap_panel_y + snapped_screen_y;
      if ((tile_y & 1) != 0)
      {
        tile_screen_x += g_world_camera_minimap_panel_x / 2;
      }

      ScreenToWorldPoint(tile_screen_x + 0x10, tile_screen_y, &world_tile_x, &world_tile_y);
      world_tile_x = world_tile_x / 32;
      world_tile_y = world_tile_y / 32;

      if ((abs(tile_y) <= 7) && (abs(tile_x) <= 3))
      {
        SetWorldMapPixelFlags(0x80, world_tile_x, world_tile_y);
      }

      /* Tile flags/type for rendering decisions. */
      tile_mask = GetWorldMapPixelFlags(world_tile_x, world_tile_y);
      tile_type = tile_mask & 0xf;

      if ((draw_mode != 0) &&
          (world_tile_y >= 0) && (world_tile_x >= 0) &&
          (world_tile_y <= 0x3f) && (world_tile_x <= 0x3f))
      {
        g_world_tile_screen_x_cache[world_tile_y * 0x100 + world_tile_x] = tile_screen_x;
        g_world_tile_screen_y_cache[world_tile_y * 0x100 + world_tile_x] = tile_screen_y;
      }

      tile_screen_y -= g_world_camera_minimap_panel_y;
      tile_class = GetWorldTileMagicMask((unsigned int)tile_type);

      if ((edge_mode == 2) || ((draw_mode != 0) && (edge_mode == 0)))
      {
      }
      else
      {
        if (((tile_type == 0) || (tile_type == 8)) && (edge_mode != 0))
        {
          DrawEncodedImageUnscaledClipped(g_page0_window_bounds, tile_screen_x, tile_screen_y, g_land_tile_sprite_entries[1]);
        }
        else if (edge_mode != 0)
        {
          if (tile_type == 1)
          {
            DrawEncodedImageUnscaledClipped(g_page0_window_bounds, tile_screen_x, tile_screen_y, g_land_tile_sprite_entries[2]);
          }
          else
          {
            if (tile_class == 2)
            {
              marker_sprite = g_land_tile_sprite_entries[4];
            }
            else
            {
              marker_sprite = g_land_tile_sprite_entries[0];
            }
            DrawEncodedImageUnscaledClipped(g_page0_window_bounds, tile_screen_x, tile_screen_y, marker_sprite);
          }

          coastline_bits = road_connection_bits = 0;
          for (i = 1; i <= 8; i++)
          {
            coastline_bits >>= 1;
            road_connection_bits >>= 1;
            neighbor_tile_type = GetWorldTileType(g_neighbor_dx[i] + world_tile_x, g_neighbor_dy[i] + world_tile_y);
            neighbor_tile_class = GetWorldTileMagicMask(neighbor_tile_type);

            if ((neighbor_tile_type == 0) || (neighbor_tile_type == 8) ||
                ((neighbor_tile_type != (unsigned int)tile_type) && (((tile_class | neighbor_tile_class) & 4) != 0)))
            {
              coastline_bits |= 0x80;
            }

            if ((tile_class != 2) && (neighbor_tile_class == 2))
            {
              road_connection_bits |= 0x80;
            }
          }

          scratch_index = road_connection_bits;
          road_connection_bits |= road_connection_bits << 8;
          if ((road_connection_bits != 0) && (g_hide_world_map_overlays == 0))
          {
            for (i = 0; i < 4; i++)
            {
              switch (i)
              {
              case 0:
                road_bits = road_connection_bits & 7;
                break;
              case 3:
                road_bits = (road_connection_bits >> 2) & 7;
                break;
              case 1:
                road_bits = (road_connection_bits >> 4) & 7;
                break;
              case 2:
                road_bits = (road_connection_bits >> 6) & 7;
                break;
              }

              road_bits_before_dec = road_bits;
              road_bits--;
              if (road_bits_before_dec != 0)
              {
                if (i < 2)
                {
                  DrawEncodedImageUnscaledClipped(g_page0_window_bounds,
                                                  tile_screen_x + g_world_camera_minimap_panel_y,
                                                  (i & 1) * g_world_camera_minimap_panel_y + tile_screen_y,
                                                  g_cstline1_sprite_entries[i + 4][road_bits]);
                }
                else
                {
                  DrawEncodedImageUnscaledClipped(g_page0_window_bounds,
                                                  (i & 1) * g_world_camera_minimap_panel_y * 2 + tile_screen_x,
                                                  tile_screen_y + g_world_camera_minimap_panel_y / 2,
                                                  g_cstline1_sprite_entries[i + 4][road_bits]);
                }
              }
            }
          }

          scratch_index = coastline_bits;
          coastline_bits |= coastline_bits << 8;
          if ((coastline_bits != 0) && (g_hide_world_map_overlays == 0))
          {
            for (i = 0; i < 4; i++)
            {
              switch (i)
              {
              case 0:
                road_bits = coastline_bits & 7;
                break;
              case 3:
                road_bits = (coastline_bits >> 2) & 7;
                break;
              case 1:
                road_bits = (coastline_bits >> 4) & 7;
                break;
              case 2:
                road_bits = (coastline_bits >> 6) & 7;
                break;
              }

              coastline_bits_before_dec = road_bits;
              road_bits--;
              if (coastline_bits_before_dec != 0)
              {
                if (i < 2)
                {
                  DrawEncodedImageUnscaledClipped(g_page0_window_bounds,
                                                  tile_screen_x + g_world_camera_minimap_panel_y,
                                                  (i & 1) * g_world_camera_minimap_panel_y + tile_screen_y,
                                                  g_cstline1_sprite_entries[i][road_bits]);
                }
                else
                {
                  DrawEncodedImageUnscaledClipped(g_page0_window_bounds,
                                                  (i & 1) * g_world_camera_minimap_panel_y * 2 + tile_screen_x,
                                                  tile_screen_y + g_world_camera_minimap_panel_y / 2,
                                                  g_cstline1_sprite_entries[i][road_bits]);
                }
              }
            }
          }
        }

        if (IsWorldMapCoordinateInBounds(world_tile_x, world_tile_y) != 0)
        {
          road_mask = GetGraphicsPixelColorRef(g_page2_window_bounds, world_tile_x, world_tile_y + 0x40);
        }
        else
        {
          road_mask = 0;
        }

        if (road_mask != 0)
        {
          for (i = 0; i < 8; i++)
          {
            if (((road_mask & (1 << (unsigned char)i)) != 0) && (edge_mode != 0))
            {
              DrawEncodedImageUnscaledClipped(g_page0_window_bounds, tile_screen_x, tile_screen_y, g_road_sprite_entries[(i - 2 & 7) + 1]);
            }
          }
        }
      }

      if (edge_mode == 1)
      {
        continue;
      }

      tile_base_screen_y = tile_screen_y + g_world_camera_minimap_panel_y;
      if (tile_mask & 0x10)
      {
      }
      else
      {
        terrain_flip_side = ((-(world_tile_y * 2 - world_tile_y) - (world_tile_x * 2 - world_tile_x) & 2) != 0);
        terrain_pattern = abs(world_tile_y) * 3 + abs(world_tile_x) * 7;
        switch (tile_type)
        {
        case 3:
          terrain_column = 0;
          break;
        case 1:
          terrain_column = 1;
          break;
        case 2:
          terrain_column = 2;
          break;
        case 5:
          terrain_column = 3;
          break;
        case 6:
          terrain_column = 4;
          break;
        case 8:
          terrain_column = 5;
          break;
        case 0xd:
          terrain_column = 6;
          break;
        case 10:
          terrain_column = 7;
          break;
        case 0xf:
          terrain_column = 8;
          break;
        case 0:
          terrain_column = -1;
          break;
        default:
          terrain_column = -1;
          break;
        }

        scratch_index = 0;
        terrain_offset_x = g_world_camera_minimap_panel_x / ((terrain_pattern % 5) * 2 + 4);
        terrain_offset_y = ScaleUiCoordinate((terrain_pattern % 4) * 2 + 5);
        if (tile_type == 1)
        {
          terrain_offset_y = 0;
          terrain_offset_x = terrain_offset_y;
        }

        if (terrain_column >= 0)
        {
          terrain_row = terrain_pattern % 0xb;
          if (5 <= terrain_column)
          {
            terrain_column %= 5;
            terrain_row += 0xb;
          }
          first_terrain_sprite = g_land_sprite_entries[terrain_row][terrain_column];
          if (first_terrain_sprite->top_clip != 0xffff)
          {
            terrain_height = first_terrain_sprite->top_clip;
          }
          else
          {
            terrain_height = 0;
          }

          if (draw_mode != 0)
          {
            if (scratch_index != 0)
            {
              terrain_draw_base_y_adjust = terrain_offset_y;
            }
            else
            {
              terrain_draw_base_y_adjust = -terrain_offset_y;
            }

            if (scratch_index != 0)
            {
              terrain_draw_y_adjust = terrain_offset_y;
            }
            else
            {
              terrain_draw_y_adjust = -terrain_offset_y;
            }

            if (scratch_index != 0)
            {
              terrain_draw_x_adjust = -terrain_offset_x;
            }
            else
            {
              terrain_draw_x_adjust = terrain_offset_x;
            }

            QueueWorldSpriteForDraw(g_page0_window_bounds,
                                    tile_screen_x + terrain_draw_x_adjust,
                                    (tile_base_screen_y - terrain_height) + terrain_draw_y_adjust,
                                    terrain_draw_base_y_adjust + tile_base_screen_y,
                                    g_land_sprite_entries[terrain_row][terrain_column]);
          }
          if (edge_mode != 0)
          {
            if (scratch_index != 0)
            {
              terrain_edge_y_adjust = terrain_offset_y;
            }
            else
            {
              terrain_edge_y_adjust = -terrain_offset_y;
            }

            if (scratch_index != 0)
            {
              terrain_edge_x_adjust = -terrain_offset_x;
            }
            else
            {
              terrain_edge_x_adjust = terrain_offset_x;
            }
            DrawEncodedImageUnscaledClipped(g_page0_window_bounds,
                                            tile_screen_x + terrain_edge_x_adjust,
                                            (tile_base_screen_y - terrain_height) + terrain_edge_y_adjust,
                                            g_sland_sprite_entries[terrain_row][terrain_column]);
          }

          scratch_index = (scratch_index == 0);
          terrain_row = (terrain_pattern + world_tile_x) % 0xb + (((terrain_row < 0xb) - 1) & 0xb);
          second_terrain_sprite = g_land_sprite_entries[terrain_row][terrain_column];
          if (second_terrain_sprite->top_clip != 0xffff)
          {
            terrain_height = second_terrain_sprite->top_clip;
          }
          else
          {
            terrain_height = 0;
          }

          if (draw_mode != 0)
          {

            QueueWorldSpriteForDraw(g_page0_window_bounds,
                                    tile_screen_x - terrain_offset_x,
                                    (tile_base_screen_y - terrain_height) + terrain_offset_y,
                                    terrain_offset_y + tile_base_screen_y,
                                    g_land_sprite_entries[terrain_row][terrain_column]);
          }
          if (edge_mode != 0)
          {
            DrawEncodedImageUnscaledClipped(g_page0_window_bounds,
                                            tile_screen_x - terrain_offset_x,
                                            (tile_base_screen_y - terrain_height) + terrain_offset_y,
                                            g_sland_sprite_entries[terrain_row][terrain_column]);
          }
        }
      }

      if (tile_mask & 0x40)
      {
        i = FindCastleDungeonAtWorldCoordinates(world_tile_x, world_tile_y);

        if ((i >= 0) &&
            (g_castle_dungeon_slots[i].clues_bitmap != 0) &&
            (g_castle_dungeon_slots[i].card_slot_1 != -1))
        {
          scratch_index = (i - 5) * 2;
          dungeon_marker_sprite = g_location_marker_sprite_entries[g_dungeon_marker_sprite_lookup[scratch_index]];

          if (draw_mode != 0)
          {
            QueueWorldSpriteForDraw(g_page0_window_bounds,
                                    tile_screen_x,
                                    (tile_base_screen_y - (int)dungeon_marker_sprite->height) + g_world_camera_minimap_panel_y,
                                    tile_base_screen_y,
                                    g_location_marker_sprite_entries[g_dungeon_marker_sprite_lookup[scratch_index]]);
          }
          if (edge_mode != 0)
          {
            DrawEncodedImageUnscaledClipped(g_page0_window_bounds,
                                            tile_screen_x,
                                            (tile_base_screen_y - (int)dungeon_marker_sprite->height) + g_world_camera_minimap_panel_y,
                                            g_location_marker_sprite_entries[g_dungeon_marker_sprite_lookup[scratch_index + 1]]);
          }
        }
      }

      if ((tile_mask & 0x10) != 0)
      {
        town_index = FindTownAtWorldCoordinates(world_tile_x, world_tile_y);
        if (g_town_slots[town_index].location_type == 4)
        {
          tile_class = single_color_test_bit_to_color_t((int)tile_class) - 1;
          if (draw_mode != 0)
          {
            QueueWorldSpriteForDraw(g_page0_window_bounds,
                                    tile_screen_x,
                                    tile_base_screen_y - ScaleUiCoordinate(0xa0),
                                    tile_base_screen_y,
                                    g_castles_sprite_entries[(char)g_castle_sprite_lookup_by_tile_class[tile_class * 4]]);
          }
          if (edge_mode != 0)
          {
            DrawEncodedImageUnscaledClipped(g_page0_window_bounds,
                                            tile_screen_x,
                                            tile_base_screen_y - ScaleUiCoordinate(0xa0),
                                            g_castles_sprite_entries[(char)g_castle_sprite_lookup_by_tile_class[tile_class * 4 + 1]]);
          }
        }
        else if (g_town_slots[town_index].location_type == 5)
        {
          tile_class = single_color_test_bit_to_color_t((int)tile_class) - 1;
          if (draw_mode != 0)
          {
            QueueWorldSpriteForDraw(g_page0_window_bounds,
                                    tile_screen_x,
                                    tile_base_screen_y - ScaleUiCoordinate(0xa0),
                                    tile_base_screen_y,
                                    g_castles_sprite_entries[(char)g_castle_sprite_lookup_by_tile_class[tile_class * 4 + 0x1a]]);
          }
          if (edge_mode != 0)
          {
            DrawEncodedImageUnscaledClipped(g_page0_window_bounds,
                                            tile_screen_x,
                                            tile_base_screen_y - ScaleUiCoordinate(0xa0),
                                            g_castles_sprite_entries[(char)g_castle_sprite_lookup_by_tile_class[tile_class * 4 + 0x1b]]);
          }
        }
        else if (g_town_slots[town_index].status_and_ruling_wizard & 0xff00)
        {
          wizard_index = (g_town_slots[town_index].status_and_ruling_wizard >> 8) - 1;
          if (draw_mode != 0)
          {
            QueueWorldSpriteForDraw(g_page0_window_bounds,
                                    tile_screen_x,
                                    tile_base_screen_y - ScaleUiCoordinate(0xa6),
                                    tile_base_screen_y,
                                    g_wizard_controlled_town_sprite_entries[wizard_index]);
          }
          if (edge_mode != 0)
          {
            DrawEncodedImageUnscaledClipped(g_page0_window_bounds,
                                            tile_screen_x,
                                            tile_base_screen_y - ScaleUiCoordinate(0xa6),
                                            g_current_location_marker_sprite);
          }
        }
        else
        {
          if (g_town_slots[town_index].location_type == 1)
          {
            scratch_index = ((world_tile_x & 1) + 0x10) * 2;
          }
          else
          {
            scratch_index = (town_index % 0x10) * 2;
          }

          town_marker_sprite = g_location_marker_sprite_entries[(char)g_location_marker_sprite_lookup_by_town[scratch_index]];
          town_marker_height = town_marker_sprite->height;
          if (draw_mode != 0)
          {
            QueueWorldSpriteForDraw(g_page0_window_bounds,
                                    tile_screen_x,
                                    ScaleUiCoordinate(0x18) + (tile_base_screen_y - town_marker_height),
                                    tile_base_screen_y,
                                    g_location_marker_sprite_entries[(char)g_location_marker_sprite_lookup_by_town[scratch_index]]);
          }
          if (edge_mode != 0)
          {
            DrawEncodedImageUnscaledClipped(g_page0_window_bounds,
                                            tile_screen_x,
                                            ScaleUiCoordinate(0x18) + (tile_base_screen_y - town_marker_height),
                                            g_location_marker_sprite_entries[(char)g_location_marker_sprite_lookup_by_town[scratch_index + 1]]);
          }
        }

        if ((town_index == g_current_quest_destination) && (edge_mode != 0))
        {
          g_page0_window_bounds->font_slot = 1;
          if (g_town_slots[town_index].location_type > 1 || town_index == g_current_quest_destination)
          {
            if (g_town_slots[town_index].location_type == 4)
            {
              scratch_index = single_color_test_bit_to_color_t((int)tile_class);
              strcpy(g_ui_message_buffer, BuildQuestLocationName(town_index, g_current_quest_destination, scratch_index));
              g_castle_dungeon_slots[scratch_index - 1].clues_bitmap |= 1;
            }
            else
            {
              strcpy(g_ui_message_buffer, BuildQuestLocationName(town_index, g_current_quest_destination, 0));
            }

            DrawWorldUiFormattedText(g_page0_window_bounds, 0xff, tile_screen_x + g_world_camera_minimap_panel_x / 2,
                                     tile_base_screen_y + g_world_camera_minimap_panel_y / 2, g_ui_message_buffer);
          }
          else
          {
            strcpy(g_ui_message_buffer, BuildQuestLocationName(town_index, g_current_quest_destination, 0));
            DrawWorldUiFormattedText(g_page0_window_bounds, 0xff, tile_screen_x + g_world_camera_minimap_panel_x / 2,
                                     tile_base_screen_y + g_world_camera_minimap_panel_y / 2, g_ui_message_buffer);
          }
        }
      }
    }
  }

  WorldPointToScreen(g_world_player_world_x_cached, g_world_player_world_y_cached, &tile_screen_x, &tile_screen_y);

  g_page0_window_bounds->page_number = saved_page_number;
}

void DrawAdventureRightClickMenuButton(void)
{
  int pad;
  int size;
  int left;
  int top;
  int fill_color;
  int border_light;
  int border_dark;
  EncodedImage *sprite;

  pad = ScaleUiCoordinate(4);
  size = ScaleUiCoordinate(0x20);
  left = global_screen_width - size - pad;
  top = pad;

  fill_color = 0x1a;
  border_light = 0xfe;
  border_dark = 0x1b;
  sprite = g_main_menu_button_sprites_normal[0];
  if (IsAdventureRightClickMenuButtonHit(g_mouse_x, g_mouse_y) != 0)
  {
    fill_color = (g_mouse_button_down_mask & 1) ? 0x18 : 0x1b;
    border_light = 0xff;
    border_dark = 0x1a;
    sprite = g_main_menu_button_sprites_highlight[0] ? g_main_menu_button_sprites_highlight[0] : sprite;
  }

  FillGraphicsRect(g_page0_window_bounds, left, top, size, size, fill_color);
  FillGraphicsRect(g_page0_window_bounds, left, top, size, ScaleUiCoordinate(1), border_light);
  FillGraphicsRect(g_page0_window_bounds, left, top, ScaleUiCoordinate(1), size, border_light);
  FillGraphicsRect(g_page0_window_bounds, left, top + size - ScaleUiCoordinate(1), size, ScaleUiCoordinate(1), border_dark);
  FillGraphicsRect(g_page0_window_bounds, left + size - ScaleUiCoordinate(1), top, ScaleUiCoordinate(1), size, border_dark);
  if (sprite != 0)
  {
    DrawEncodedImageResampled(g_page0_window_bounds, left + ScaleUiCoordinate(3), top + ScaleUiCoordinate(3),
                              size - ScaleUiCoordinate(6), size - ScaleUiCoordinate(6), sprite);
  }
}

// FUNCTION: SHANDALAR 0x0054baaa
int DrawAdventureQuestStatusPanel(int force_redraw)
{
  int siege_slot_index;
  int avatar_draw_y;
  EncodedImage *avatar_sprite_entry;
  int scaled_strip_x;
  int scaled_strip_height;
  int text_x;
  int day_sprite_index;
  int text_tail_length;
  int saved_page_number;
  EncodedImage *clocknew_sprite_entry;
  EncodedImage *questnew_sprite_entry;

  questnew_sprite_entry = g_questnew_sprite_entries[1];
  clocknew_sprite_entry = g_clocknew_sprite_entries[0];
  saved_page_number = g_page1_window_bounds->page_number;
  g_page1_window_bounds->page_number = 1;
  day_sprite_index = (g_current_quest_deadline - g_quest_restock_timer) / 0x10 + 1;

  if (force_redraw != 0 &&
      ((g_current_quest_destination != -1) || (g_lair_or_monster_slots[7].entry_type != SHANDALAR_ENTRY_NONE) ||
       (g_next_duel_life_delta != 0) || (g_next_duel_card_id != -1)))
  {
    g_world_ui_cached_next_duel_card_id = g_next_duel_card_id;
    g_world_ui_cached_next_duel_life_delta = g_next_duel_life_delta;
    g_world_ui_cached_siege_entry_type = g_lair_or_monster_slots[7].entry_type;
    scaled_strip_height = ScaleUiCoordinate((int)questnew_sprite_entry->height) - ScaleUiCoordinate(0x13);

    if (g_skip_world_sfx_preload == 0)
    {
      scaled_strip_x = ScaleUiCoordinate(0x17c);
      BlitGraphicsRect(g_page3_window_bounds, (unsigned int)scaled_strip_x, 0,
                       (unsigned int)(ScaleUiCoordinate(0x280) - scaled_strip_x),
                       (DWORD)g_page3_window_bounds->max_y,
                       g_page1_window_bounds, scaled_strip_x, ScaleUiCoordinate(0x148) - ScaleUiCoordinate(0x135));

      scaled_strip_x = ScaleUiCoordinate(0x181);
      DrawEncodedImageResampled(g_page1_window_bounds,
                                scaled_strip_x,
                                0,
                                ScaleUiCoordinate((int)questnew_sprite_entry->width),
                                ScaleUiCoordinate((int)questnew_sprite_entry->height),
                                g_questnew_sprite_entries[1]);

      if (g_current_quest_destination != -1)
      {
        DrawEncodedImageResampled(g_page1_window_bounds,
                                  ScaleUiCoordinate(0x23a),
                                  ScaleUiCoordinate(0x15),
                                  ScaleUiCoordinate((int)clocknew_sprite_entry->width),
                                  ScaleUiCoordinate((int)clocknew_sprite_entry->height),
                                  g_clocknew_sprite_entries[g_monster_timer & 7]);

        DrawEncodedImageResampled(g_page1_window_bounds,
                                  ScaleUiCoordinate(0x23a),
                                  ScaleUiCoordinate(0x15),
                                  ScaleUiCoordinate((int)clocknew_sprite_entry->width),
                                  ScaleUiCoordinate((int)clocknew_sprite_entry->height),
                                  g_sunmoon_sprite_entries[(g_current_quest_deadline - g_quest_restock_timer) % 0xe]);

        DrawEncodedImageResampled(g_page1_window_bounds,
                                  ScaleUiCoordinate(0x23a),
                                  ScaleUiCoordinate(0x15),
                                  ScaleUiCoordinate((int)clocknew_sprite_entry->width),
                                  ScaleUiCoordinate((int)clocknew_sprite_entry->height),
                                  g_daysnew_sprite_entries[day_sprite_index]);
      }
    }

    avatar_draw_y = 0x5b;
    avatar_sprite_entry = g_world_magic_avatar_sprites[0];
    g_page1_window_bounds->font_slot = 4;

    avatar_draw_y -= (int)avatar_sprite_entry->height / 2;
    avatar_sprite_entry = g_world_magic_avatar_sprites[0];
    DrawEncodedImageUiScaled(g_page1_window_bounds,
                             0x193,
                             avatar_draw_y,
                             g_world_magic_avatar_sprites[0],
                             (int)avatar_sprite_entry->width,
                             (int)avatar_sprite_entry->height);
    DrawTextAt(g_page1_window_bounds, g_world_ui_stats_color_index, 0x1a9, avatar_draw_y + (int)avatar_sprite_entry->height / 2, "%d",
               g_amulet_inventory[4]);

    scaled_strip_x = ScaleUiCoordinate(0x17c);
    text_x = 0x200;
    text_tail_length = 0x33;
    g_page1_window_bounds->font_slot = 2;
    strcpy(g_ui_message_buffer, "");
    if (g_lair_or_monster_slots[7].entry_type != SHANDALAR_ENTRY_NONE)
    {
      siege_slot_index = 7;
      FormatMessageFromStringStripCarriageReturns(
          g_ui_message_buffer,
          0x1000,
          gs_queststatus_0077e0a0[0],
          GetCreatureName(g_lair_or_monster_slots[siege_slot_index].entry_type),
          BuildTownDisplayName(FindNearestTownIndex(
              g_lair_or_monster_slots[siege_slot_index].world_x / 0x20,
              g_lair_or_monster_slots[siege_slot_index].world_y / 0x20)));
    }
    else
    {
      g_siege_indicator = 0;
    }

    if ((g_next_duel_life_delta != 0) || (g_next_duel_card_id != -1))
    {
      strcat(g_ui_message_buffer, gs_queststatus_0077e0a0[1]);
      if (g_next_duel_life_delta != 0)
      {
        if (g_next_duel_life_delta >= 0)
        {
          sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_queststatus_0077e0a0[2], g_next_duel_life_delta);
        }
        else
        {
          sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_queststatus_0077e0a0[3], abs(g_next_duel_life_delta));
        }
        DrawEncodedImageResampled(g_page0_window_bounds,
                                  ScaleUiCoordinate(0x254),
                                  ScaleUiCoordinate(200),
                                  ScaleUiCoordinate(0x1c),
                                  ScaleUiCoordinate(0x25),
                                  g_worlds_extra_sprite_entries[3]);
      }

      if (g_next_duel_card_id == 0)
      {
        strcat(g_ui_message_buffer, gs_queststatus_0077e0a0[4]);
        DrawEncodedImageResampled(g_page0_window_bounds,
                                  ScaleUiCoordinate(0x254),
                                  ScaleUiCoordinate(200),
                                  ScaleUiCoordinate(0x1c),
                                  ScaleUiCoordinate(0x25),
                                  g_worlds_extra_sprite_entries[1]);
      }
      else if (g_next_duel_card_id > 0 && g_next_duel_card_id <= 5)
      {
        sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_queststatus_0077e0a0[2], g_next_duel_card_id);
        DrawEncodedImageResampled(g_page0_window_bounds,
                                  ScaleUiCoordinate(0x254),
                                  ScaleUiCoordinate(200),
                                  ScaleUiCoordinate(0x1c),
                                  ScaleUiCoordinate(0x25),
                                  g_worlds_extra_sprite_entries[2]);
      }
      else if (g_next_duel_card_id > 5)
      {
        strcat(g_ui_message_buffer, global_cards_data[g_next_duel_card_id].name);
        strcat(g_ui_message_buffer, " ");
        DrawEncodedImageResampled(g_page0_window_bounds,
                                  ScaleUiCoordinate(0x254),
                                  ScaleUiCoordinate(0x90),
                                  ScaleUiCoordinate(0x1c),
                                  ScaleUiCoordinate(0x25),
                                  g_worlds_extra_sprite_entries[0]);
      }
      strcat(g_ui_message_buffer, "\n\n");
    }

    if (g_current_quest_destination != -1)
    {
      if (g_current_quest_type != 0)
      {
        if (g_town_slots[g_current_quest_destination].location_type == 1)
        {
          strcat(g_ui_message_buffer, gs_queststatus_0077e0a0[g_current_quest_color + 4]);
        }
        else
        {
          strcat(g_ui_message_buffer, GetTownCardDescription(g_current_quest_destination));
        }
        strcat(g_ui_message_buffer, "\n");
      }
      else
      {
        strcat(g_ui_message_buffer, gs_queststatus_0077e0a0[10]);
      }

      if ((((g_current_quest_type == 0) || (g_current_quest_type == 2)) ||
           ((g_current_quest_type == 1) &&
            (FindDeckSlotForQuestColorAndType(g_current_quest_color, 1 << (g_current_quest_destination & 3)) != 0))) ||
          (g_current_quest_type < -100))
      {
        day_sprite_index = GetRelativeWorldQuadrant(g_town_slots[g_current_quest_destination].world_x,
                                                    g_town_slots[g_current_quest_destination].world_y);
        if (g_current_quest_data != day_sprite_index)
        {
          g_current_quest_data = -1;
        }

        switch (g_current_quest_data)
        {
        case -1:
          if (g_current_quest_type < 0)
          {
            sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_queststatus_0077e0a0[0xb], BuildTownDisplayName(g_current_quest_destination));
          }
          else if ((g_current_quest_type == 0) || (g_current_quest_type == 2))
          {
            sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_queststatus_0077e0a0[0xc], BuildTownDisplayName(g_current_quest_destination));
          }
          else
          {
            sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_queststatus_0077e0a0[0xd], BuildTownDisplayName(g_current_quest_destination));
          }
          break;
        case 0:
          sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_queststatus_0077e0a0[0xe], BuildTownDisplayName(g_current_quest_destination));
          break;
        case 1:
          sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_queststatus_0077e0a0[0xf], BuildTownDisplayName(g_current_quest_destination));
          break;
        case 2:
          sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_queststatus_0077e0a0[0x10], BuildTownDisplayName(g_current_quest_destination));
          break;
        case 3:
          sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_queststatus_0077e0a0[0x11], BuildTownDisplayName(g_current_quest_destination));
          break;
        }

        DrawTextAt(g_page1_window_bounds, 0xff, text_x, 0x41, "%s", g_ui_message_buffer);

        BlitGraphicsRect(g_page1_window_bounds, (unsigned int)scaled_strip_x,
                         ScaleUiCoordinate(0x148) - ScaleUiCoordinate(0x135),
                         (unsigned int)(ScaleUiCoordinate(0x280) - scaled_strip_x),
                         (DWORD)scaled_strip_height,
                         g_page0_window_bounds, scaled_strip_x, ScaleUiCoordinate(0x148));

        goto check_quest_deadline;
      }
      if (g_current_quest_type == 1)
      {
        FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer, 0x1000, gs_queststatus_0077e0a0[0x12],
                                                    gs_queststatus_0077e0a0[g_current_quest_color + 0x12],
                                                    GetQuestCardClassName(1 << (g_current_quest_destination & 3)));
      }
      if (g_current_quest_type < 0)
      {
        sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_queststatus_0077e0a0[0x18], BuildCreatureNameWithArticle(-g_current_quest_type));
      }
    }

    DrawTextAt(g_page1_window_bounds, 0xff, text_x, 0x41, "%s", g_ui_message_buffer);

    BlitGraphicsRect(g_page1_window_bounds, (unsigned int)scaled_strip_x,
                     ScaleUiCoordinate(0x148) - ScaleUiCoordinate(0x135),
                     (unsigned int)(ScaleUiCoordinate(0x280) - scaled_strip_x),
                     (DWORD)scaled_strip_height,
                     g_page0_window_bounds, scaled_strip_x, ScaleUiCoordinate(0x148));

  check_quest_deadline:
    if ((g_current_quest_destination != -1) && (g_current_quest_deadline <= g_quest_restock_timer))
    {
      AddJournalEntry(JOURNAL_ENTRY_QUEST_FAILED, g_current_quest_type);
      sprintf(g_ui_message_buffer, gs_queststatus_0077e0a0[0x19], BuildTownDisplayName(g_current_quest_giver_town_index));
      g_page0_window_bounds->font_slot = 4;
      RunTextMenuAtScaled(g_ui_message_buffer, 0x5a, 0x50);
      g_town_slots[g_current_quest_giver_town_index].status_and_ruling_wizard |= 4;
      g_current_quest_destination = -1;
      RefreshAdventureInterfaceLayout();
      DrawAdventureQuestStatusPanel(1);
    }
  }
  else if (force_redraw != 0)
  {
    DrawEncodedImageUiScaled(g_page0_window_bounds, 0x181, 0x135, g_questnew_sprite_entries[0], 0xed, 0x74);
  }
  g_page1_window_bounds->page_number = saved_page_number;
  return 0;
}

// FUNCTION: SHANDALAR 0x0054b80d
int DrawQuestDeadlineClock(void)
{
  int scaled_strip_height;             // ebp - 0x10
  EncodedImage *clocknew_sprite_entry; // ebp - 0xc
  EncodedImage *questnew_sprite_entry; // ebp - 0x8
  int day_sprite_index;                // ebp - 0x4

  if (g_current_quest_destination != -1)
  {
    questnew_sprite_entry = g_questnew_sprite_entries[1];
    clocknew_sprite_entry = g_clocknew_sprite_entries[0];
    day_sprite_index = ((g_current_quest_deadline - g_quest_restock_timer) / 16) + 1;

    if (g_skip_world_sfx_preload == 0)
    {
      scaled_strip_height = ScaleUiCoordinate(0x23a);
      BlitGraphicsRect(g_page3_window_bounds, (unsigned int)scaled_strip_height, 0,
                       (unsigned int)(ScaleUiCoordinate(0x280) - scaled_strip_height),
                       (DWORD)g_page3_window_bounds->max_y,
                       g_page1_window_bounds, scaled_strip_height, ScaleUiCoordinate(0x148) - ScaleUiCoordinate(0x135));

      DrawEncodedImageResampled(g_page1_window_bounds,
                                ScaleUiCoordinate(0x181),
                                ScaleUiCoordinate(0),
                                ScaleUiCoordinate(questnew_sprite_entry->width),
                                ScaleUiCoordinate(questnew_sprite_entry->height),
                                g_questnew_sprite_entries[1]);

      DrawEncodedImageResampled(g_page1_window_bounds,
                                ScaleUiCoordinate(0x23a),
                                ScaleUiCoordinate(0x15),
                                ScaleUiCoordinate(clocknew_sprite_entry->width),
                                ScaleUiCoordinate(clocknew_sprite_entry->height),
                                g_clocknew_sprite_entries[g_monster_timer & 7]);

      DrawEncodedImageResampled(g_page1_window_bounds,
                                ScaleUiCoordinate(0x23a),
                                ScaleUiCoordinate(0x15),
                                ScaleUiCoordinate(clocknew_sprite_entry->width),
                                ScaleUiCoordinate(clocknew_sprite_entry->height),
                                g_sunmoon_sprite_entries[(g_current_quest_deadline - g_quest_restock_timer) % 0xe]);

      DrawEncodedImageResampled(g_page1_window_bounds,
                                ScaleUiCoordinate(0x23a),
                                ScaleUiCoordinate(0x15),
                                ScaleUiCoordinate(clocknew_sprite_entry->width),
                                ScaleUiCoordinate(clocknew_sprite_entry->height),
                                g_daysnew_sprite_entries[day_sprite_index]);

      BlitGraphicsRect(g_page1_window_bounds, (unsigned int)ScaleUiCoordinate(0x23a),
                       ScaleUiCoordinate(0x148) - ScaleUiCoordinate(0x135),
                       (unsigned int)ScaleUiCoordinate(clocknew_sprite_entry->width),
                       ScaleUiCoordinate(clocknew_sprite_entry->height),
                       g_page0_window_bounds, ScaleUiCoordinate(0x23a), ScaleUiCoordinate(0x148));
    }
  }
  return 0;
}

// FUNCTION: SHANDALAR 0x0055060c
void *DrawAdventureInterfaceLayout(int force_redraw)
{
  int avatar_draw_y;
  EncodedImage *avatar_sprite_entry;
  int world_magic_score;
  int world_magic_icon_y;
  int world_magic_slot_index;
  int avatar_x;

  g_page0_window_bounds->font_slot = 4;
  g_page1_window_bounds->font_slot = 4;

  BlitGraphicsRect(g_page3_window_bounds, ScaleUiCoordinate(0x58), ScaleUiCoordinate(0x15b) - ScaleUiCoordinate(0x148),
                   (unsigned int)ScaleUiCoordinate(0x126), ScaleUiCoordinate(0x15b) - ScaleUiCoordinate(0x148),
                   g_page1_window_bounds, 0, 0);

  DrawFormattedTextShadowedCentered(g_page1_window_bounds, g_world_ui_stats_color_index, ScaleUiCoordinate(0x12), ScaleUiCoordinate(10), "%d", Gold);
  DrawFormattedTextShadowedCentered(g_page1_window_bounds, g_world_ui_stats_color_index, ScaleUiCoordinate(0x60), ScaleUiCoordinate(10), "%d", g_food);
  DrawFormattedTextShadowedCentered(g_page1_window_bounds, g_world_ui_stats_color_index, ScaleUiCoordinate(0xab), ScaleUiCoordinate(10), "%d", CountDuelPoolEligibleTowns());
  DrawFormattedTextShadowedCentered(g_page1_window_bounds, g_world_ui_stats_color_index, ScaleUiCoordinate(0x10c), ScaleUiCoordinate(10), "%d/%d", g_deck_active_card_count, g_deck_total_card_count);

  BlitGraphicsRect(g_page1_window_bounds, 0, 0, (unsigned int)ScaleUiCoordinate(0x126), ScaleUiCoordinate(0x15b) - ScaleUiCoordinate(0x148),
                   g_page0_window_bounds, ScaleUiCoordinate(0x58), ScaleUiCoordinate(0x15b));

  g_page0_window_bounds->font_slot = 2;
  DrawAdventureQuestStatusPanel(1);
  DrawAdventureRightClickMenuButton();

  if ((g_adventure_ui_layout_dirty == 0) && (force_redraw == 0))
  {
    ShowMouseCursorNested();
    return (void *)0;
  }

  avatar_x = 0x48;
  avatar_draw_y = 400;
  avatar_sprite_entry = g_world_magic_avatar_sprites[0];
  g_page0_window_bounds->font_slot = 4;
  avatar_draw_y -= (int)avatar_sprite_entry->height / 2;

  DrawEncodedImageUiScaled(g_page0_window_bounds, 0x4e, avatar_draw_y, g_world_magic_avatar_sprites[2],
                           (int)avatar_sprite_entry->width,
                           (int)avatar_sprite_entry->height);
  DrawTextAt(g_page0_window_bounds, g_world_ui_stats_color_index, 100, avatar_draw_y + (int)avatar_sprite_entry->height / 2, "%d",
             g_amulet_inventory[0]);

  DrawEncodedImageUiScaled(g_page0_window_bounds, 0xa1, avatar_draw_y, g_world_magic_avatar_sprites[1],
                           (int)avatar_sprite_entry->width,
                           (int)avatar_sprite_entry->height);
  DrawTextAt(g_page0_window_bounds, g_world_ui_stats_color_index, 0xb7, avatar_draw_y + (int)avatar_sprite_entry->height / 2, "%d",
             g_amulet_inventory[1]);

  DrawEncodedImageUiScaled(g_page0_window_bounds, 0xf0, avatar_draw_y, g_world_magic_avatar_sprites[4],
                           (int)avatar_sprite_entry->width,
                           (int)avatar_sprite_entry->height);
  DrawTextAt(g_page0_window_bounds, g_world_ui_stats_color_index, 0x106, avatar_draw_y + (int)avatar_sprite_entry->height / 2, "%d",
             g_amulet_inventory[2]);

  DrawEncodedImageUiScaled(g_page0_window_bounds, 0x140, avatar_draw_y, g_world_magic_avatar_sprites[3],
                           (int)avatar_sprite_entry->width,
                           (int)avatar_sprite_entry->height);
  DrawTextAt(g_page0_window_bounds, g_world_ui_stats_color_index, 0x156, avatar_draw_y + (int)avatar_sprite_entry->height / 2, "%d",
             g_amulet_inventory[3]);

  DrawEncodedImageUiScaled(g_page0_window_bounds, 0x193, avatar_draw_y, g_world_magic_avatar_sprites[0],
                           (int)avatar_sprite_entry->width,
                           (int)avatar_sprite_entry->height);
  DrawTextAt(g_page0_window_bounds, g_world_ui_stats_color_index, 0x1a9, avatar_draw_y + (int)avatar_sprite_entry->height / 2, "%d",
             g_amulet_inventory[4]);

  avatar_x = 0x10;
  world_magic_icon_y = 0x96;
  for (world_magic_slot_index = 0; world_magic_slot_index < 0xc; world_magic_slot_index++)
  {
    if ((g_world_magic_bitmap & (1 << (unsigned char)world_magic_slot_index)) != 0)
    {
      world_magic_score = FindWorldMagicCardIndex(world_magic_slot_index);
      if ((world_magic_slot_index >= 2) && ((world_magic_slot_index & 1) == 0))
      {
        if (g_amulet_inventory[world_magic_slot_index / 2 - 1] != 0)
        {
          DrawEncodedImageUiScaled(g_page0_window_bounds,
                                   g_adventure_interface_world_magic_icon_rects[world_magic_slot_index].x,
                                   g_adventure_interface_world_magic_icon_rects[world_magic_slot_index].y,
                                   g_world_magic_choice_button_sprite_bank.named.normal[world_magic_slot_index],
                                   g_adventure_interface_world_magic_icon_rects[world_magic_slot_index].width,
                                   g_adventure_interface_world_magic_icon_rects[world_magic_slot_index].height);
          DrawEncodedImageUiScaled(g_page0_window_bounds,
                                   g_adventure_interface_world_magic_icon_rects[world_magic_slot_index].x,
                                   g_adventure_interface_world_magic_icon_rects[world_magic_slot_index].y,
                                   g_world_magic_choice_button_sprite_bank.named.icon[world_magic_slot_index],
                                   g_adventure_interface_world_magic_icon_rects[world_magic_slot_index].width,
                                   g_adventure_interface_world_magic_icon_rects[world_magic_slot_index].height);
        }
        else
        {
          DrawEncodedImageUiScaled(g_page0_window_bounds,
                                   g_adventure_interface_world_magic_icon_rects[world_magic_slot_index].x,
                                   g_adventure_interface_world_magic_icon_rects[world_magic_slot_index].y,
                                   g_world_magic_choice_button_sprite_bank.named.pressed[world_magic_slot_index],
                                   g_adventure_interface_world_magic_icon_rects[world_magic_slot_index].width,
                                   g_adventure_interface_world_magic_icon_rects[world_magic_slot_index].height);
        }
      }
      else
      {
        DrawEncodedImageUiScaled(g_page0_window_bounds,
                                 g_adventure_interface_world_magic_icon_rects[world_magic_slot_index].x,
                                 g_adventure_interface_world_magic_icon_rects[world_magic_slot_index].y,
                                 g_world_magic_choice_button_sprite_bank.named.normal[world_magic_slot_index],
                                 g_adventure_interface_world_magic_icon_rects[world_magic_slot_index].width,
                                 g_adventure_interface_world_magic_icon_rects[world_magic_slot_index].height);
      }
    }
  }

  g_page0_window_bounds->font_slot = 1;
}

// FUNCTION: SHANDALAR 0x0054a880
void ResetWorldDrawQueue(void)
{
  g_world_draw_entry_count = 0;
  g_world_draw_head_index = 0;
}

// FUNCTION: SHANDALAR 0x0054a89f
void QueueWorldSpriteForDraw(FacemakerWindowBounds *window, int draw_x, int draw_y, int depth_y, EncodedImage *sprite)
{
  int current_index;
  int previous_index;

  (void)window;

  g_world_draw_screen_x[g_world_draw_entry_count] = draw_x;
  g_world_draw_screen_y[g_world_draw_entry_count] = draw_y;
  g_world_draw_depth_sort_keys[g_world_draw_entry_count] = depth_y;
  g_world_draw_sprites[g_world_draw_entry_count] = sprite;
  if (g_world_draw_entry_count == 0)
  {
    g_world_draw_next_index[g_world_draw_entry_count] = -1;
    g_world_draw_entry_count++;
  }
  else
  {
    current_index = g_world_draw_head_index;
    previous_index = -1;
    while ((g_world_draw_depth_sort_keys[current_index] < depth_y) && (current_index != -1))
    {
      previous_index = current_index;
      current_index = g_world_draw_next_index[current_index];
    }
    g_world_draw_next_index[g_world_draw_entry_count] = current_index;
    if (previous_index != -1)
    {
      g_world_draw_next_index[previous_index] = g_world_draw_entry_count;
    }
    else
    {
      g_world_draw_head_index = g_world_draw_entry_count;
    }
    g_world_draw_entry_count++;
  }
}

// FUNCTION: SHANDALAR 0x0054a997
void DrawQueuedWorldSprites(void)
{
  int draw_index;

  draw_index = g_world_draw_head_index;
  while (draw_index != -1)
  {
    DrawEncodedImageUnscaledClipped(g_page0_window_bounds,
                                    g_world_draw_screen_x[draw_index] - g_world_scroll_x,
                                    g_world_draw_screen_y[draw_index] - g_world_scroll_y,
                                    g_world_draw_sprites[draw_index]);
    draw_index = g_world_draw_next_index[draw_index];
  }
}

// FUNCTION: SHANDALAR 0x0054aa04
void WorldSubtileOffsetToScreen(int world_subcell_x, int world_subcell_y, int *out_screen_x, int *out_screen_y)
{
  int world_subcell_y_copy;
  int screen_x;
  int world_subcell_x_copy;
  int screen_y;

  world_subcell_x_copy = world_subcell_x;
  world_subcell_y_copy = world_subcell_y;
  screen_x = ((world_subcell_y_copy + world_subcell_x_copy) * global_screen_width * 2) / 0x280;
  screen_y = ((world_subcell_y_copy - world_subcell_x_copy) * global_screen_height) / 0x1e0;
  *out_screen_x = screen_x;
  *out_screen_y = screen_y;
}

// FUNCTION: SHANDALAR 0x0054aa60
void WorldTileToScreenFromCameraAnchor(int world_tile_x, int world_tile_y, int *out_screen_x, int *out_screen_y)
{
  int delta_y;
  int screen_x;
  int delta_x;
  int screen_y;

  delta_x = world_tile_x - g_world_camera_last_anchor_x;
  delta_y = world_tile_y - g_world_camera_last_anchor_y;
  screen_x = ((delta_y + delta_x) * global_screen_width * 2) / 0x280 + g_world_camera_viewport_width_pixels / 2;
  screen_y = ((delta_y - delta_x) * global_screen_height) / 0x1e0 + g_world_camera_viewport_height_pixels / 2;
  *out_screen_x = screen_x;
  *out_screen_y = g_world_ui_top_offset + screen_y;
}

// FUNCTION: SHANDALAR 0x0054aae9
void WorldPointToScreen(int world_x, int world_y, int *out_screen_x, int *out_screen_y)
{
  int delta_y;
  int screen_x;
  int delta_x;
  int screen_y;

  delta_x = world_x - g_world_player_world_x_cached;
  delta_y = world_y - g_world_player_world_y_cached;
  screen_x = ((delta_y + delta_x) * global_screen_width * 2) / 0x280 + g_world_camera_viewport_width_pixels / 2;
  screen_y = ((delta_y - delta_x) * global_screen_height) / 0x1e0 + g_world_camera_viewport_height_pixels / 2;
  *out_screen_x = screen_x;
  *out_screen_y = screen_y + g_world_ui_top_offset;
}

// FUNCTION: SHANDALAR 0x0054ab73
void ScreenToWorldPoint(int screen_x, int screen_y, int *out_world_x, int *out_world_y)
{
  int world_delta_y;
  int world_delta_x;

  world_delta_x = screen_x - g_world_camera_viewport_width_pixels / 2;
  world_delta_y = (screen_y - g_world_ui_top_offset) - g_world_camera_viewport_height_pixels / 2;
  world_delta_x = (world_delta_x * 0x280) / global_screen_width;
  world_delta_y = (world_delta_y * 0x1e0) / global_screen_height;
  *out_world_x = g_world_player_world_x_cached + ((world_delta_x >> 1) - world_delta_y) / 2;
  *out_world_y = g_world_player_world_y_cached + ((world_delta_x >> 1) + world_delta_y) / 2;
}

// FUNCTION: SHANDALAR 0x004318c7
int IsWorldMapCoordinateInBounds(int x, int y)
{
  if ((x < 0) || (x >= 0x40))
  {
    return 0;
  }
  if ((y < 0) || (y >= 0x40))
  {
    return 0;
  }
  return 1;
}

// FUNCTION: SHANDALAR 0x00508b89
int FindCastleDungeonAtWorldCoordinates(int world_x, int world_y)
{
  int entry_index;

  for (entry_index = 0; entry_index < 0xf; entry_index++)
  {
    if ((g_castle_dungeon_slots[entry_index].world_x == world_x) &&
        (g_castle_dungeon_slots[entry_index].world_y == world_y))
    {
      return entry_index;
    }
  }

  return -1;
}

// FUNCTION: SHANDALAR 0x004bb040
int FindTownAtWorldCoordinates(int world_x, int world_y)
{
  int town_index;

  for (town_index = 0; town_index < 0x80; town_index++)
  {
    if ((g_town_slots[town_index].location_type != -1) &&
        (g_town_slots[town_index].world_x == world_x) &&
        (g_town_slots[town_index].world_y == world_y))
    {
      return town_index;
    }
  }

  return -1;
}

static __inline void ConfigureAdventureWorldViewport(void)
{
  switch (global_screen_width)
  {
  case 640:
    g_world_camera_viewport_width_pixels = 0x280;
    g_world_camera_viewport_height_pixels = 0x1e0;
    g_world_camera_minimap_panel_x = 0x80;
    g_world_camera_minimap_panel_y = 0x20;
    break;
  case 800:
    g_world_camera_viewport_width_pixels = 800;
    g_world_camera_viewport_height_pixels = 600;
    g_world_camera_minimap_panel_x = 0xa0;
    g_world_camera_minimap_panel_y = 0x28;
    break;
  case 1024:
    g_world_camera_viewport_width_pixels = 0x400;
    g_world_camera_viewport_height_pixels = 0x300;
    g_world_camera_minimap_panel_x = 0xcc;
    g_world_camera_minimap_panel_y = 0x33;
    break;
  default:
    break;
  }
}

static __inline void DrawWorldStatusTipSprites(void)
{
  int status_layout_x[6];
  int status_layout_y[3];
  int status_frame_y[3];
  int resolution_index;
  int panel_top_y;
  int left_icon_x;
  int right_icon_x;
  int frame_y;
  EncodedImage *status_sprite;

  if (g_questnew_sprite_entries[0] == (EncodedImage *)0)
  {
    return;
  }

  status_layout_x[0] = 0xcb;
  status_layout_x[1] = 0xfd;
  status_layout_x[2] = 0x143;
  status_layout_x[3] = 0x1af;
  status_layout_x[4] = 0x21a;
  status_layout_x[5] = 0x2b0;

  status_layout_y[0] = 0x40;
  status_layout_y[1] = 0x50;
  status_layout_y[2] = 0x66;

  status_frame_y[0] = 0x48;
  status_frame_y[1] = 0x5a;
  status_frame_y[2] = 0x72;

  if (global_screen_width == 0x280)
  {
    resolution_index = 0;
  }
  else if (global_screen_width == 800)
  {
    resolution_index = 1;
  }
  else
  {
    resolution_index = 2;
  }

  status_sprite = g_questnew_sprite_entries[0];
  panel_top_y = ScaleUiCoordinate(0x105) + g_world_camera_minimap_panel_y * 2 + g_world_ui_top_offset + 0x10;
  DrawEncodedImageResampled(g_page1_window_bounds,
                            ScaleUiCoordinate(0x141) + 0x40,
                            panel_top_y,
                            ScaleUiCoordinate((int)status_sprite->width),
                            ScaleUiCoordinate((int)status_sprite->height),
                            status_sprite);

  if (g_tips_frame_sprite != (EncodedImage *)0)
  {
    frame_y = status_frame_y[resolution_index] + g_world_camera_minimap_panel_y * 2 + g_world_ui_top_offset + 0x10;
    DrawEncodedImageUnscaled(g_page1_window_bounds, 0x40, frame_y - ScaleUiCoordinate(0x30), g_tips_frame_sprite);
  }

  if (g_tips_icon_sprite != (EncodedImage *)0)
  {
    panel_top_y = status_layout_y[resolution_index] + g_world_camera_minimap_panel_y * 2 + g_world_ui_top_offset + 0x10;
    left_icon_x = status_layout_x[resolution_index];
    right_icon_x = status_layout_x[resolution_index + 3];
    DrawEncodedImageUnscaled(g_page1_window_bounds, (left_icon_x + 0x40) - ScaleUiCoordinate(0x40), panel_top_y, g_tips_icon_sprite);
    DrawEncodedImageUnscaled(g_page1_window_bounds, (right_icon_x + 0x40) - ScaleUiCoordinate(0x40), panel_top_y, g_tips_icon_sprite);
  }
}

// FUNCTION: SHANDALAR 0x0054ac08
void RenderAdventureWorldScene(int world_x, int world_y, int world_state)
{
  int scratch_padding[4];
  AdvMenuRect restore_clip_rect_scratch;
  AdvMenuRect minimap_clip_rect_scratch;
  AdvMenuRect ui_clip_rect_scratch;
  AdvMenuRect world_clip_rect_scratch;
  DIBSurface * scaled_0x20;
  DIBSurface * scaled_0x8c;
  int status_layout_x[6];
  int status_layout_y[3];
  int resolution_index;
  int status_layout_panel_y[3];
  int status_frame_y[3];
  int scaled_0x100;
  int monster_tile_screen_x;
  int monster_subcell_screen_y;
  int monster_subcell_screen_x;
  int monster_tile_screen_y;
  int player_screen_y;
  int player_screen_x;
  int heading_sprite_index;
  int slot_index;
  int tile_y;
  ShandalarEntryType current_entry_type;
  int tile_x_text;
  int previous_page_number;
  AdvMenuRect saved_clip_rect_world;
  AdvMenuRect saved_clip_rect_ui;

#ifdef MODERN_FIXES
  tile_x_text = 0;
  tile_y = 0;
#endif

  previous_page_number = g_page1_window_bounds->page_number;
  HideMouseCursorNested();

  ConfigureAdventureWorldViewport();

  g_world_player_world_x_cached = world_x;
  g_world_player_world_y_cached = world_y;

  g_world_player_tile_x = world_x / 32;
  g_world_player_tile_y = world_y / 32;

  g_page0_window_bounds->page_number = g_page1_window_bounds->page_number;
  ResetWorldDrawQueue();
  g_world_ui_top_offset = 0x50;
  saved_clip_rect_world =
      *PushGraphicsClipRect(&world_clip_rect_scratch, g_page1_window_bounds, 0, 0x80, g_page0_window_bounds->max_x, g_page0_window_bounds->max_y - 0x80);
  saved_clip_rect_ui =
      *PushGraphicsClipRect(&ui_clip_rect_scratch, g_page0_window_bounds, 0, 0x80, g_page0_window_bounds->max_x, g_page0_window_bounds->max_y - 0x80);
  UpdateWorldViewportBuffer(world_x, world_y);

  WorldPointToScreen(world_x, world_y, &player_screen_x, &player_screen_y);
  PushGraphicsClipRect(&minimap_clip_rect_scratch, g_page0_window_bounds, 0x40,
                       g_world_camera_minimap_panel_y * 2 + g_world_ui_top_offset + 0x10,
                       ScaleUiCoordinateFrom320(0x100), ScaleUiCoordinateFrom320(0x8c));

  QueueWorldSpriteForDraw(g_page0_window_bounds,
                          (player_screen_x - g_ego_sprite_width / 2) + g_world_scroll_x,
                          (player_screen_y - g_ego_sprite_draw_height) + g_world_scroll_y,
                          player_screen_y + g_world_scroll_y,
                          (&g_opening_menu_sprite_work_buffer[0x10].sprites[((g_world_player_animation_direction + 2) & 7) * 5])
                              [g_world_player_animation_frame]);

  DrawEncodedImageUnscaledClipped(g_page0_window_bounds,
                                  player_screen_x - g_sego_sprite_width / 2,
                                  player_screen_y - g_sego_sprite_draw_height,
                                  (&g_opening_menu_sprite_work_buffer[0x11].sprites[((g_world_player_animation_direction + 2) & 7) * 5])
                                      [g_world_player_animation_frame]);
  strcpy(g_ui_message_buffer, "");
  strcat(g_ui_message_buffer, _itoa(tile_x_text, g_itoa_buffer, 10));
  strcat(g_ui_message_buffer, " ");
  strcat(g_ui_message_buffer, _itoa(tile_y, g_itoa_buffer, 10));

  for (slot_index = 0; slot_index < 8; slot_index = slot_index + 1)
  {
    WorldTileToScreenFromCameraAnchor(g_lair_or_monster_slots[slot_index].world_x & 0xffffffe0, g_lair_or_monster_slots[slot_index].world_y & 0xffffffe0,
                                      &monster_tile_screen_x, &monster_tile_screen_y);
    WorldSubtileOffsetToScreen(g_lair_or_monster_slots[slot_index].world_x & 0x1f, g_lair_or_monster_slots[slot_index].world_y & 0x1f,
                               &monster_subcell_screen_x, &monster_subcell_screen_y);
    player_screen_x = monster_subcell_screen_x + monster_tile_screen_x;
    player_screen_y = monster_tile_screen_y + monster_subcell_screen_y;
    monster_tile_screen_x = g_lair_or_monster_slots[slot_index].world_x / 32;
    monster_tile_screen_y = g_lair_or_monster_slots[slot_index].world_y / 32;
    if ((g_lair_or_monster_slots[slot_index].entry_type != SHANDALAR_ENTRY_NONE))
    {
      if ((abs(monster_tile_screen_x - g_world_player_tile_x) > 1) ||
          (abs(monster_tile_screen_y - g_world_player_tile_y) > 1))
      {
        continue;
      }

      current_entry_type = g_lair_or_monster_slots[slot_index].entry_type;
      if ((g_shandalar_monster_definitions[current_entry_type].flags_0a & 2U) != 0)
      {
        current_entry_type -= (ShandalarEntryType)(((unsigned int)g_monster_timer >> 5) & 3);
      }
      if (((g_shandalar_monster_definitions[current_entry_type].flags_0a & 0x100U) != 0) &&
          (ApproximateDistance(player_screen_x - 0x140, player_screen_y - 0xf0) > ScaleUiCoordinateFrom320(0x80)))
      {
        continue;
      }

      if (g_lair_or_monster_slots[slot_index].entry_type == SHANDALAR_ENTRY_LAIR)
      {
        QueueWorldSpriteForDraw(g_page0_window_bounds,
                                player_screen_x - ScaleUiCoordinateFrom320(0x20),
                                player_screen_y - ScaleUiCoordinateFrom320(0x30),
                                player_screen_y,
                                g_location07_sprite_entries[g_lair_sprite_index_by_color[g_lair_or_monster_slots[slot_index].color]]);

        DrawEncodedImageUnscaledClipped(g_page0_window_bounds,
                                        (player_screen_x - ScaleUiCoordinateFrom320(0x20)) - g_world_scroll_x,
                                        (player_screen_y - ScaleUiCoordinateFrom320(0x30)) - g_world_scroll_y,
                                        g_location07_sprite_entries[g_lair_sprite_index_by_color[g_lair_or_monster_slots[slot_index].color] + 6]);
      }
      else
      {
        heading_sprite_index = (signed char)g_shandalar_monster_definitions[current_entry_type].encounter_type;
        if (g_skip_world_sfx_preload != 0)
        {
          heading_sprite_index = 1;
        }
        heading_sprite_index = slot_index;
        QueueWorldSpriteForDraw(g_page0_window_bounds,
                                player_screen_x - g_world_lair_monster_sprite_widths[heading_sprite_index] / 2,
                                player_screen_y - g_world_lair_monster_sprite_top_clips[heading_sprite_index],
                                player_screen_y,
                                (&g_opening_menu_sprite_work_buffer[heading_sprite_index]
                                      .sprites[((g_lair_or_monster_slots[slot_index].movement_heading + 2) & 7) * 5])
                                    [g_lair_or_monster_slots[slot_index].movement_anim_frame]);

        heading_sprite_index = slot_index + 8;
        DrawEncodedImageUnscaledClipped(g_page0_window_bounds,
                                        (player_screen_x - g_world_lair_monster_sprite_widths[heading_sprite_index] / 2) - g_world_scroll_x,
                                        (player_screen_y - g_world_lair_monster_sprite_top_clips[heading_sprite_index]) - g_world_scroll_y,
                                        (&g_opening_menu_sprite_work_buffer[heading_sprite_index]
                                              .sprites[((g_lair_or_monster_slots[slot_index].movement_heading + 2) & 7) * 5])
                                            [g_lair_or_monster_slots[slot_index].movement_anim_frame]);
      }

      if ((((g_monster_timer >> 3) & 3) == (slot_index & 3)))
      {
        g_page0_window_bounds->font_slot = 3;
        strcpy(g_ui_message_buffer, GetCreatureName(current_entry_type));
      }
    }
  }

  DrawQueuedWorldSprites();
  g_page0_window_bounds->page_number = 0;
  scaled_0x100 = (int)g_questnew_sprite_entries[0];
  status_layout_y[0] = 0x40;
  status_layout_y[1] = 0x50;
  status_layout_y[2] = 0x66;
  status_frame_y[0] = 0x48;
  status_frame_y[1] = 0x5a;
  status_frame_y[2] = 0x72;
  status_layout_x[0] = 0xcb;
  status_layout_x[1] = 0xfd;
  status_layout_x[2] = 0x143;
  status_layout_x[3] = 0x1af;
  status_layout_x[4] = 0x21a;
  status_layout_x[5] = 0x2b0;
  status_layout_panel_y[0] = 0x2f;
  status_layout_panel_y[1] = 0x3c;
  status_layout_panel_y[2] = 0x4d;
  DrawEncodedImageResampled(g_page1_window_bounds,
                            ScaleUiCoordinate(0x141) + 0x40,
                            ScaleUiCoordinate(0x105) + g_world_camera_minimap_panel_y * 2 + g_world_ui_top_offset + 0x10,
                            ScaleUiCoordinate((int)((EncodedImage *)scaled_0x100)->width),
                            ScaleUiCoordinate((int)((EncodedImage *)scaled_0x100)->height),
                            g_questnew_sprite_entries[0]);
  switch (global_screen_width)
  {
  case 0x280:
    resolution_index = 0;
    break;

  case 0x320:
    resolution_index = 1;
    break;

  case 0x400:
    resolution_index = 2;
    break;
  }
  DrawEncodedImageUnscaled(g_page1_window_bounds, 0x40,
                           (status_frame_y[resolution_index] + g_world_camera_minimap_panel_y * 2 + g_world_ui_top_offset + 0x10) -
                               ScaleUiCoordinate(0x30),
                           g_tips_frame_sprite);
  DrawEncodedImageUnscaled(g_page1_window_bounds,
                           (status_layout_x[resolution_index] + 0x40) - ScaleUiCoordinate(0x40),
                           g_world_camera_minimap_panel_y * 2 + g_world_ui_top_offset + 0x10,
                           g_tips_icon_sprite);
  DrawEncodedImageUnscaled(g_page1_window_bounds,
                           (status_layout_x[resolution_index + 3] + 0x40) - ScaleUiCoordinate(0x40),
                           g_world_camera_minimap_panel_y * 2 + g_world_ui_top_offset + 0x10,
                           g_tips_icon_sprite);
  if (g_world_scene_reveal_effect_pending == 0)
  {
    if (global_screen_width != 0x400)
    {
      BlitGraphicsRect(g_page1_window_bounds, 0x40, g_world_camera_minimap_panel_y * 2 + g_world_ui_top_offset + 0x10,
                       ScaleUiCoordinateFrom320(0x100), ScaleUiCoordinateFrom320(0x8c), g_page0_window_bounds,
                       ScaleUiCoordinateFrom320(0x20) & 0xfffffffc, ScaleUiCoordinateFrom320(0x18));
    }
    else
    {
      BlitGraphicsRect(g_page1_window_bounds, 0x40, g_world_camera_minimap_panel_y * 2 + g_world_ui_top_offset + 0x10,
                       ScaleUiCoordinateFrom320(0x100) - 2, ScaleUiCoordinateFrom320(0x8c), g_page0_window_bounds,
                       (ScaleUiCoordinateFrom320(0x20) + 4U) & 0xfffffffc, ScaleUiCoordinateFrom320(0x18));
    }
  }
  else
  {
    scaled_0x8c = g_graphics_pages[g_page1_window_bounds->page_number];
    scaled_0x20 = g_graphics_pages[g_page0_window_bounds->page_number];
    if (global_screen_width != 0x400)
    {
      BlitRectByRandomTileOrder(scaled_0x20->hTempDC,
                                ScaleUiCoordinateFrom320(0x20) & 0xfffffffc, ScaleUiCoordinateFrom320(0x18),
                                ScaleUiCoordinateFrom320(0x100), ScaleUiCoordinateFrom320(0x8c), 6, 6,
                                scaled_0x8c->hTempDC,
                                0x40, g_world_camera_minimap_panel_y * 2 + g_world_ui_top_offset + 0x10);
    }
    else
    {
      BlitRectByRandomTileOrder(scaled_0x20->hTempDC,
                                (ScaleUiCoordinateFrom320(0x20) + 4U) & 0xfffffffc, ScaleUiCoordinateFrom320(0x18),
                                ScaleUiCoordinateFrom320(0x100) - 2, ScaleUiCoordinateFrom320(0x8c), 8, 8,
                                scaled_0x8c->hTempDC,
                                0x40, g_world_camera_minimap_panel_y * 2 + g_world_ui_top_offset + 0x10);
    }
    g_world_scene_reveal_effect_pending = 0;
  }

  PushGraphicsClipRect(&restore_clip_rect_scratch, g_page1_window_bounds, saved_clip_rect_world.x, saved_clip_rect_world.y,
                       saved_clip_rect_world.width, saved_clip_rect_world.height);
  PushGraphicsClipRect((AdvMenuRect *)scratch_padding, g_page0_window_bounds, saved_clip_rect_ui.x, saved_clip_rect_ui.y, saved_clip_rect_ui.width,
                       saved_clip_rect_ui.height);
  g_page1_window_bounds->page_number = previous_page_number;

  if ((world_state == 0) && (g_adventure_ui_layout_dirty == 0))
  {
    g_page0_window_bounds->font_slot = 1;
    ShowMouseCursorNested();
    DrawAdventureQuestStatusPanel(0);
    DrawQuestDeadlineClock();
    DrawAdventureRightClickMenuButton();
    return;
  }
  else
  {
    DrawAdventureInterfaceLayout(0);
    g_adventure_ui_layout_dirty = 0;
    ShowMouseCursorNested();
  }
}
