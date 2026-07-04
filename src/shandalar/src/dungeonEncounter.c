#ifdef MODERN_FIXES
#include "platform/winuser_compat.h"
#else
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "shandalar.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "magic/src/game_support.h"
#include "magic/src/global_strings.h"
#include "shandalar_global_strings.h"
#include "deckdll/src/magsnd.h"
#include "facemaker/src/facemaker_types.h"
#include "drawcardlib/src/pic.h"
#include "cardartlib/src/palette.h"

/* Dungeon / castle encounter flow rooted at SHANDALAR 0x0050a5c1. */

typedef unsigned char byte;
typedef unsigned char uchar;
typedef int undefined3;
#ifndef bool
typedef int bool;
#endif
#define DUNGEON_GRID_WIDTH 0xf
#define DUNGEON_GRID_HEIGHT 0xd
typedef int DungeonGrid[DUNGEON_GRID_WIDTH][DUNGEON_GRID_HEIGHT];
typedef char DungeonGrid_size_must_be_0x30c[(sizeof(DungeonGrid) == 0x30c) ? 1 : -1];
#define DUNGEON_CELL_DISTANCE(x, y) (*(char *)((char *)&g_dungeon_runtime_state + 0x364 + (y) + (x) * DUNGEON_GRID_HEIGHT))
typedef struct DungeonRuntimeState
{
  union
  {
    int monster_sound_ids[8];
    struct
    {
      int unused_sound_ids[3];
      int monster_creature_types[5];
    } selected;
  } encounter;                      // 0x000
  EncodedImage *button_sprite_blob; // 0x020
  int cell_tile_variants[171];      // 0x024
  char padding_2d0[0x60];           // 0x2d0
  int current_dungeon_index;        // 0x330
  int player_x;                     // 0x334
  int player_y;                     // 0x338
  char padding_33c[0xec];           // 0x33c
  int reveal_dirty;                 // 0x428
  EncodedImage *button_sprite_aux0; // 0x42c
  EncodedImage *button_sprite_aux1; // 0x430
  EncodedImage *button_sprite_aux2; // 0x434
} DungeonRuntimeState;
typedef char DungeonRuntimeState_size_must_be_0x438[(sizeof(DungeonRuntimeState) == 0x438) ? 1 : -1];

#ifndef CONCAT31
#define CONCAT31(a, b) ((int)(b))
#endif
#ifndef true
#define true 1
#endif

extern int global_screen_width;
extern int global_screen_height;
extern int g_default_palette_fade_steps;
extern int g_adventure_world_exit_requested;

extern int g_shandalar_difficulty;
extern int g_mouse_button_mask_snapshot;
extern int g_mouse_x_snapshot;
extern int g_mouse_y_snapshot;
extern int g_defeated_wizards_bitmap;
extern int g_next_duel_card_id;
extern int g_next_duel_life_delta;
extern int g_neighbor_dx[9];
extern int g_neighbor_dy[9];
extern int g_statwin_exports_by_ordinal[3];
extern int g_wizard_siege_count;
extern int g_ego_sprite_width;
extern int g_ego_sprite_height;
extern int g_ego_sprite_draw_height;
extern int g_sego_sprite_width;
extern int g_sego_sprite_draw_height;
extern int g_world_lair_monster_sprite_top_clips[0x10];
extern int g_world_lair_monster_sprite_widths[0x10];
extern int g_world_location_music_track_id;
extern int g_world_location_music_active;
extern int DAT_00742fc0;
extern int g_encounter_opening_hand_size_modifier;
extern int g_current_encounter_strength;
extern int g_ai_combat_value_weights[4];
extern int g_current_encounter_color;
extern int g_opponent_starting_card_id_1;
extern int g_opponent_starting_card_id_2;
extern int Gold;
extern int g_life[2];
extern int deck[500];
extern char g_ui_message_buffer[0x1000];
extern HPALETTE g_palette_handle;
extern HWND g_main_window_hwnd;
extern DIBSurface *g_graphics_pages[10];
extern FacemakerWindowBounds *g_page0_window_bounds;
extern FacemakerWindowBounds *g_page1_window_bounds;
extern FacemakerWindowBounds *g_page2_window_bounds;
extern OpeningMenuSpriteWorkEntry g_opening_menu_sprite_work_buffer[0x20];
char *BuildCreatureNameWithArticle(int creature_type);
char *BuildTownDisplayName(int town_index);
char *GetCreatureName(int creature_type);
DWORD FormatMessageFromStringStripCarriageReturns(char *dst, DWORD max_length, LPCVOID format, ...);
int BeginMenuContext(void);
int ClampIntToRange(int value, int min_value, int max_value);
int CountDuelPoolEligibleTowns(void);
int EndMenuContext(void);
int ExitIfNoUsableDeckCards(void);
int PopNormalizedQueuedKeyInput(void);
int PopulateDungeonCellEvents(void);
void SetDungeonPassageHighlight(int direction, int enable);
void DrawCastleDungeonBoard(int animation_step, int initial_draw, int dungeon_index);
void ConvertDungeonCellToScreen(int dungeon_x, int dungeon_y, int *out_screen_x, int *out_screen_y);
void ConvertMouseToDungeonCell(int mouse_x, int mouse_y, int *out_x, int *out_y);
int QueueDungeonMouseNavigationInput(int mouse_x, int mouse_y);
int ShutdownSharedStartup(void);
int FindTownAtWorldCoordinates(int world_x, int world_y);
int LoadCreatureEncounterSprites(int creature_type, int work_entry_index_a, int work_entry_index_b);
int SeedRandomFromTickCount(void);
char *GetDungeonName(int dungeon_index);
int CalculateDungeonEndgameScore(void);
int internal_rand(int max_value);
int RunCardRiddleChallenge(void);
int ScaleUiCoordinateFrom320(int value);
int ScaleUiCoordinate(int value);
int AddCardToDeckSorted(int card_id);
int FindCardIndexByCsvid(int csvid);
int sound_get_state(int sound_id, int *out_state);
int GetUiTickCount(void);
int IsKeyInputQueueEmpty(void);
int IsWizardColorFeminine(int color_index);
int LoadCreatureDuelDeck(int creature_type, unsigned int name_id, unsigned int color_filter, int speed_filter);
ShandalarEntryType PickRandomCreatureTypeForWizardTier(int wizard_color, int creature_tier);
int QuestCardChooserCallback(void);
int ReadSpriteEntryPointers(EncodedImage **out_sprite_entries, char *sprite_path);
int ReadSpriteEntryPointersWithLimit(EncodedImage **out_sprite_entries, char *sprite_path, int max_entries);
int RemoveCardFromDeckById(unsigned int card_id);
int ResetMenuContext(int context_index);
int RunCardBrowser(char *title, int color_mask, unsigned int type_mask, int reset_filters, int show_done_button);
int RunCastleDungeonBoard(int dungeon_index);
int RunDungeonMonsterDuel(int dungeon_index, int monster_slot, int final_battle);
undefined4 HandleDefeatedWizardCastle(int dungeon_index);
int RunDuelEngine(unsigned int card_id, int creature_type);
int RunTextMenuAt(char *menu_text, int x, int y);
int RunTextMenuAtScaled(char *menu_text, int x_320_scale, unsigned int y_200_scale);
int ScaleUiCoordinate(int value);
int MeasureTextLineWidth(char *text);
int sound_stop(int sound_id);
int sound_unload(int sound_id);
unsigned int GetWorldTileType(int x, int y);
unsigned int GetWorldMapPixelFlags(int x, int y);
unsigned int LoadSoundWithDriveFallback(char *filename, int channel, int flags);
unsigned int WaitForInputEventUnlessBlocked(void);
int AnimatePaletteToColor(int color_index, int palette_id);
int ClearGraphicsPageWithPaletteColor(int page_number, int color_index);
void ClearInputAndWaitForMouseRelease(void);
void ConsumeUiTickCount(void);
WPARAM WINAPI DeckBuilderMain(HWND parent_hwnd, int db_flags_1, int db_flags_2);
void DrawTextAt(FacemakerWindowBounds *window, int color, int x, int y, char *text, ...);
void DrawCenteredTextLineWithShadow(char *text, int center_x, int y, int color_index);
void DrawFormattedTextShadowedCentered(FacemakerWindowBounds *window, int color_index, int x, int y, char *format, ...);
void DrawUiScaledCenteredText(char *text, int center_x_320, int y_200, int color_index);
void DrawTiledDialogBoxFrame(int x, int y, int width, int height, int frame_style);
int FadeInPaletteFromGray(int color_index, int palette_id);
void FreeOpeningMenuSpriteWorkEntries(int work_entry_index_a, int work_entry_index_b);
void FreeSpriteBlob(EncodedImage *sprite_blob);
void AddJournalEntry(int entry_type, int entry_arg);
void SetWorldMapPixelFlags(unsigned int mask, int x, int y);
void ClearWorldMapPixelFlags(unsigned int mask, int x, int y);
void RevealDungeonCellAndNeighbors(int x, int y);
void AppendDungeonRewardDescription(void);
void InitDungeonScratchPage(void);
void FreeDungeonScratchPage(void);
void DelayUiTicks(int ticks);
void RestoreAdventureUiPaletteAndFocus(void);
undefined4 TransitionPcxToScreenRandomTiles8(char *path);
undefined4 TransitionPcxToScreenRandomTiles(char *path, int x_scale, int y_scale);
void ClearInputAndWaitForMouseRelease(void);
void DrawAdventureCard(unsigned int card_id, int x, int y, int full_card, char *banner_text);
void DrawAdventureCardSized(int card_id, int x, int y, int width, int height, int full_card, char *label);
int ResetMenuContext(int context_index);
void ForceReloadAdvfac64(int state);
undefined4 PlayStatWinMovie(char *path, int x, int y, int flags);
void PlaySoundWithPitchAndPan(int sound_id, int volume, int pitch_percent, int pan_percent);
void PlayLoopingSoundWithPan(int sound_id, int volume, int pan_percent);
void LoadLoopingSound(char *sound_path, int channel);
void StopWorldLocationMusic(void);
void PlayCastleMusic(int castle_music_index);
int FreeGraphicsPage(int page_number);
void DrawUiScaledCenteredTextWithShadow(FacemakerWindowBounds *window, int color, int x, int y, char *format, ...);
void BlitGraphicsRect(FacemakerWindowBounds *dst, unsigned int dst_x, int dst_y, unsigned int width, DWORD height,
                      FacemakerWindowBounds *src, int src_x, int src_y);
void CopyUiScaledGraphicsRect(FacemakerWindowBounds *src_page, int src_x_320, int src_y_240, int width_320,
                              int height_240, FacemakerWindowBounds *dst_page, int dst_x_320, int dst_y_240);
void CopyGraphicsRect(FacemakerWindowBounds *src_page, int src_x, int src_y, int width, int height,
                      FacemakerWindowBounds *dst_page, int dst_x, int dst_y);
void FillGraphicsRect(FacemakerWindowBounds *window_bounds, int x, int y, int width, int height, int color_index);
void FillUiScaledGraphicsRect(FacemakerWindowBounds *window_bounds, int x_320, int y_240, int width_320,
                              int height_240, int color_index);
void DrawEncodedImageResampled(FacemakerWindowBounds *dst, int x, int y, int width, int height, EncodedImage *encoded_image);
void DrawEncodedImageUiScaled(FacemakerWindowBounds *dst, int x_320, int y_200, EncodedImage *sprite, int width_320, int height_200);
void DrawEncodedImageUnscaled(FacemakerWindowBounds *dst, int x, int y, EncodedImage *encoded_image);
unsigned int BlitRectByRandomTileOrderInPlace(HDC dst, int dst_x, int dst_y, int width, int height, int tile_w,
                                              int tile_h, HDC src);
DIBSurface *CreateGraphicsPage(int page_number, int width, int height, int bits_per_pixel);
void LoadPcxIntoPage(int page_number, char *path);
void LoadPcxIntoPageNoPalette(char *path);
void LoadPcxIntoPageOpaque(int page_number, char *path);
void LoadPcxResource(int page_number, int x, int y, char *path, void *opaque);
void PlaySoundEffectOnChannel(char *sound_path, int channel, int volume, int pitch_percent, int pan_percent);
void PushQueuedKeyInput(int key_code);
void ReadPalette(char *path, char *out_palette);
void RefreshAdventureInterfaceLayout(void);
void RestoreAdventureUiPaletteAndFocus(void);
void RunAdventureStatsMenu(void);
void ShowStatsWindow(int mode, int highlight);
void StretchBlitGraphicsRect(FacemakerWindowBounds *dst, int dst_x, int dst_y, int src_w, int src_h,
                             FacemakerWindowBounds *src, int src_x, int src_y, int copy_w, int copy_h);
void SetGraphicsPage(int page_number, DIBSurface *page);
void UnloadStatWinDllExports(void);
void UpdateMouseSnapshot(void);

// GLOBAL: SHANDALAR 0x00583a28
int g_dungeon_move_direction = 4;
// GLOBAL: SHANDALAR 0x00583a50
FacemakerWindowBounds g_dungeon_scratch_page9_bounds = {9, 0, 0, 1, 0xf, 4, 0, 0, 0};
// GLOBAL: SHANDALAR 0x00583a74
FacemakerWindowBounds *g_dungeon_scratch_page9 = &g_dungeon_scratch_page9_bounds;
// GLOBAL: SHANDALAR 0x005a8b34
DungeonRuntimeState g_dungeon_runtime_state;

// GLOBAL: SHANDALAR 0x007495b0
EncodedImage *g_dungeon_sprite_entries[0x3c];
#define g_dungeon_sprite_blob g_dungeon_sprite_entries[0]
extern int g_dungeon_life_reward_delta;
// GLOBAL: SHANDALAR 0x0078df8c
int g_dungeon_exit_x;
// GLOBAL: SHANDALAR 0x0078df90
int g_dungeon_exit_y;
// GLOBAL: SHANDALAR 0x0078dfa0
DungeonGrid g_dungeon_grid_backup;
// GLOBAL: SHANDALAR 0x0078e2ac
int g_dungeon_force_final_encounter;
// GLOBAL: SHANDALAR 0x0078e2b0
DungeonGrid g_dungeon_grid;
#define DUNGEON_GRID_CELL(x, y) g_dungeon_grid[(x)][(y)]
// GLOBAL: SHANDALAR 0x0078e44c
int g_dungeon_grid_generation_state;
static __inline int GetDungeonGridCellOrZero(int x, int y)
{
  if (x < 0 || x >= DUNGEON_GRID_WIDTH || y < 0 || y >= DUNGEON_GRID_HEIGHT)
  {
    return 0;
  }
  return DUNGEON_GRID_CELL(x, y);
}

static __inline int GetDungeonCellDistanceOrZero(int x, int y)
{
  if (x < 0 || x >= DUNGEON_GRID_WIDTH || y < 0 || y >= DUNGEON_GRID_HEIGHT)
  {
    return 0;
  }
  return DUNGEON_CELL_DISTANCE(x, y);
}

// GLOBAL: SHANDALAR 0x00583a30
static char *g_dungeon_sprite_paths[] = {"dungeon1.spr", "dungeon2.spr", "dungeon3.spr", "dungeonb.spr", "dungeonu.spr", "dungeong.spr", "dungeonr.spr", "dungeonw.spr"};
#define g_dungeon_wizard_sprite_paths_start g_dungeon_sprite_paths[2]
// GLOBAL: SHANDALAR 0x0058c670
int g_dungeon_monster_duel_music_csvids[] = {0x343, 0x24d, 0x75, 0xa0, 0x35b, 0x1c8, 0xde, 0x95, 0x5e, 0xa3, 0x130, 0x140, 0x0d, 0xa1, 0x1cf};
// GLOBAL: SHANDALAR 0x0058caa0
static int g_castle_defeat_music_ids[] = {0, 2, 3, 4, 1};

// FUNCTION: SHANDALAR 0x00443940
int RunCastleDungeonBoard(int dungeon_index)
{
  int result;                                 // ebp-0x50c
  uint random_or_flags;                       // ebp-0x508
  int scratch_504;                            // ebp-0x504
  char mana_reward_text[52];                  // ebp-0x500
  int mana_color;                             // ebp-0x4cc
  int has_mana_reward;                        // ebp-0x4c8
  int mana_reward_delta;                      // ebp-0x4c4
  int gold_reward;                            // ebp-0x4c0
  char reward_description[100];               // ebp-0x4bc
  int candidate_card_id;                      // ebp-0x458
  int pre_move_y;                             // ebp-0x454
  int pre_move_x;                             // ebp-0x450
  EncodedImage *button_sprite_entries[0x100]; // ebp-0x44c
  int creature_tier;                          // ebp-0x4c
  int card_slot_attempts;                     // ebp-0x48
  int entry_x;                                // ebp-0x44
  int entry_y;                                // ebp-0x40
  int passage_direction;                      // ebp-0x3c
  int next_ambient_ticks;                     // ebp-0x38
  int board_state;                            // ebp-0x34
  uint event_code;                            // ebp-0x30
  int input_or_sound_code;                    // ebp-0x2c
  int tile_row;                               // ebp-0x28
  uint grid_y;                                // ebp-0x24
  int tile_col;                               // ebp-0x20
  uint grid_x;                                // ebp-0x1c
  int target_y;                               // ebp-0x18
  int ambient_repeat_count;                   // ebp-0x14
  int target_x;                               // ebp-0x10
  int monster_slot;                           // ebp-0xc
#ifdef MODERN_FIXES
  char ambient_sound_path[100]; // ebp-0x8
#else
  char *ambient_sound_path; // ebp-0x8
#endif
  int lost_required_duel; // ebp-0x4

#ifdef MODERN_FIXES
  strcpy(ambient_sound_path, "x:sound\\damb1.wav");
#else
  ambient_sound_path = "x:sound\\damb1.wav";
#endif
  ambient_repeat_count = 0;
  lost_required_duel = 0;
  if (dungeon_index >= 5)
  {
    AddJournalEntry(JOURNAL_ENTRY_DUNGEON_ENTERED, dungeon_index);
  }
  else
  {
    AddJournalEntry(JOURNAL_ENTRY_CASTLE_DUNGEON_ENTERED, dungeon_index);
  }
  g_dungeon_force_final_encounter = 0;
  SeedRandomFromTickCount();
  InitDungeonScratchPage();
  g_castle_dungeon_slots[dungeon_index].times_entered = g_castle_dungeon_slots[dungeon_index].times_entered + 1;
  g_dungeon_runtime_state.current_dungeon_index = dungeon_index;
  g_dungeon_life_reward_delta = 0;
  for (tile_row = 0; tile_row < DUNGEON_GRID_HEIGHT; tile_row = tile_row + 1)
  {
    for (tile_col = 0; tile_col < DUNGEON_GRID_WIDTH; tile_col = tile_col + 1)
    {
      *(uint *)((char *)g_dungeon_runtime_state.cell_tile_variants + tile_col * 4 + tile_row * 0x34) = rand() & 3;
    }
  }
  if (dungeon_index < 5)
  {
    ReadSpriteEntryPointersWithLimit((EncodedImage **)&g_dungeon_sprite_entries,
                                     (&g_dungeon_wizard_sprite_paths_start)[(char)g_castle_dungeon_slots[dungeon_index].color], 0x3c);
  }
  else
  {
    ReadSpriteEntryPointers((EncodedImage **)&g_dungeon_sprite_entries, g_dungeon_sprite_paths[internal_rand(3)]);
  }
  LoadLoopingSound("x:sound\\dambloop.wav", 100);
  PlayLoopingSoundWithPan(100, 0x50, 0);
  set_sound_loop(100, 1);
  LoadPcxIntoPage(2, "CaveBkgd.pic");
  ReadSpriteEntryPointers(button_sprite_entries, "dungbutt.spr");
  g_dungeon_runtime_state.button_sprite_blob = button_sprite_entries[0];
  g_dungeon_runtime_state.button_sprite_aux0 = button_sprite_entries[1];
  g_dungeon_runtime_state.button_sprite_aux1 = button_sprite_entries[2];
  g_dungeon_runtime_state.button_sprite_aux2 = button_sprite_entries[3];
  for (monster_slot = 0; monster_slot < 5; monster_slot = monster_slot + 1)
  {
    do
    {
      switch (((g_castle_dungeon_slots[dungeon_index].monster_flags & 0x80) >> 6) + monster_slot)
      {
      case 0:
        creature_tier = 4;
        break;
      case 1:
        creature_tier = 6;
        break;
      case 2:
        creature_tier = 8;
        break;
      case 3:
        creature_tier = 0xc;
        break;
      case 4:
        creature_tier = 0x10;
        break;
      case 5:
        creature_tier = 0x12;
        break;
      case 6:
        creature_tier = 0x12;
        break;
      }

      if (dungeon_index < 5)
      {
        switch (internal_rand(6))
        {
        case 0:
          creature_tier = 10;
          break;
        case 1:
          creature_tier = 0xb;
          break;
        case 2:
          creature_tier = 0xc;
          break;
        case 3:
          creature_tier = 0xd;
          break;
        case 4:
          creature_tier = 0xe;
          break;
        case 5:
          creature_tier = 0x10;
          break;
        }

        if (monster_slot == 4)
        {
          creature_tier = 0x12;
        }
      }
      g_dungeon_runtime_state.encounter.selected.monster_creature_types[monster_slot] =
          PickRandomCreatureTypeForWizardTier((int)(char)g_castle_dungeon_slots[dungeon_index].color, creature_tier);
    } while (g_dungeon_runtime_state.encounter.selected.monster_creature_types[monster_slot] == 0);
    FreeOpeningMenuSpriteWorkEntries(monster_slot, monster_slot + 8);
    LoadCreatureEncounterSprites(g_dungeon_runtime_state.encounter.selected.monster_creature_types[monster_slot], monster_slot, monster_slot + 8);
    if ((dungeon_index < 5) && (monster_slot == 4))
    {
      g_dungeon_runtime_state.encounter.selected.monster_creature_types[monster_slot] =
          PickRandomCreatureTypeForWizardTier((int)(char)g_castle_dungeon_slots[dungeon_index].color, 0x14);
    }
    g_lair_or_monster_slots[monster_slot].entry_type = SHANDALAR_ENTRY_NONE;
  }
  for (grid_x = 0; (int)grid_x < DUNGEON_GRID_WIDTH; grid_x = grid_x + 1)
  {
    for (grid_y = 0; (int)grid_y < DUNGEON_GRID_HEIGHT; grid_y = grid_y + 1)
    {
      DUNGEON_GRID_CELL(grid_x, grid_y) = 1;
      if ((((grid_x == 0) || (grid_y == 0)) || ((int)grid_x >= 0xe)) || ((int)grid_y >= 0xc))
      {
        DUNGEON_GRID_CELL(grid_x, grid_y) = 0;
      }
      if (((grid_x & 1) == 0) && ((grid_y & 1) == 0))
      {
        DUNGEON_GRID_CELL(grid_x, grid_y) = 0;
      }
      if (((int)grid_x < 3) && ((int)grid_y < 3))
      {
        DUNGEON_GRID_CELL(grid_x, grid_y) = 0;
      }
      if (((int)grid_x < 3) && ((int)grid_y >= 10))
      {
        DUNGEON_GRID_CELL(grid_x, grid_y) = 0;
      }
      if (((int)grid_x >= 12) && ((int)grid_y < 3))
      {
        DUNGEON_GRID_CELL(grid_x, grid_y) = 0;
      }
      if (((int)grid_x >= 12) && ((int)grid_y >= 10))
      {
        DUNGEON_GRID_CELL(grid_x, grid_y) = 0;
      }
    }
  }
  entry_x = 7;
  g_dungeon_runtime_state.player_x = entry_x;
  g_dungeon_exit_x = g_dungeon_runtime_state.player_x;
  entry_y = 0xc;
  g_dungeon_runtime_state.player_y = entry_y;
  g_dungeon_exit_y = g_dungeon_runtime_state.player_y;
  DUNGEON_GRID_CELL(g_dungeon_runtime_state.player_x, g_dungeon_runtime_state.player_y) = 1;
  do
  {
    grid_x = internal_rand(7) * 2;
    grid_y = internal_rand(6) * 2;
    if (internal_rand(2) != 0)
    {
      grid_x = grid_x + 1;
    }
    else
    {
      grid_y = grid_y + 1;
    }
    memcpy(g_dungeon_grid_backup, g_dungeon_grid, sizeof(g_dungeon_grid));
    DUNGEON_GRID_CELL(grid_x, grid_y) = 0;
    board_state = PopulateDungeonCellEvents();
    if (board_state == 0)
    {
      memcpy(g_dungeon_grid, g_dungeon_grid_backup, sizeof(g_dungeon_grid));
    }
    else
    {
    }
  } while (board_state < 2);
  g_dungeon_runtime_state.player_y = g_dungeon_runtime_state.player_y + -1;
  RevealDungeonCellAndNeighbors(g_dungeon_runtime_state.player_x, g_dungeon_runtime_state.player_y);
  DrawCastleDungeonBoard(0, 1, dungeon_index);
  WaitForInputEventUnlessBlocked();
  DrawCastleDungeonBoard(0, 0, dungeon_index);
  next_ambient_ticks = internal_rand(0x5a) + 0x5a;
  ConsumeUiTickCount();
  board_state = 1;
  do
  {
    if (next_ambient_ticks < GetUiTickCount())
    {
      if ((ambient_sound_path[0xc] == '5') && (ambient_repeat_count != 0))
      {
        ambient_repeat_count = ambient_repeat_count + -1;
        PlaySoundWithPitchAndPan(0x65, internal_rand(0x14) + 0x50, 100, internal_rand(100) + -0x32);
        next_ambient_ticks = internal_rand(0x3c) + 0x1e;
        ConsumeUiTickCount();
        continue;
      }
      else
      {
        do
        {
          input_or_sound_code = internal_rand(5) + 0x31;
        } while (ambient_sound_path[0xc] == input_or_sound_code);
        ambient_sound_path[0xc] = (char)input_or_sound_code;
      }
      sound_unload(0x65);
      LoadSoundWithDriveFallback(ambient_sound_path, 0x65, 0);
      PlaySoundWithPitchAndPan(0x65, internal_rand(0x14) + 0x50, 100, internal_rand(100) + -0x32);
      if ((ambient_sound_path[0xc] == '5') && (ambient_repeat_count == 0))
      {
        ambient_repeat_count = internal_rand(3) + 2;
        next_ambient_ticks = internal_rand(0x1e) + 0x3c;
      }
      else
      {
        next_ambient_ticks = internal_rand(0x5a) + 0x78;
      }
      ConsumeUiTickCount();
    }

    UpdateMouseSnapshot();
    if ((IsKeyInputQueueEmpty() != 0) && (g_mouse_button_mask_snapshot == 0))
    {
    }
    else
    {
      if (g_mouse_button_mask_snapshot != 0)
      {
        board_state = QueueDungeonMouseNavigationInput(g_mouse_x_snapshot, g_mouse_y_snapshot);
      }
      passage_direction = -1;
      input_or_sound_code = PopNormalizedQueuedKeyInput();
      switch (input_or_sound_code)
      {
      case 0x4700:
        pre_move_x = g_dungeon_runtime_state.player_x;
        pre_move_y = g_dungeon_runtime_state.player_y;
        target_x = g_dungeon_runtime_state.player_x;
        target_y = g_dungeon_runtime_state.player_y + -1;
        g_dungeon_move_direction = 3;
        passage_direction = 1;
        goto default_case;
      case 0x4900:
        target_y = g_dungeon_runtime_state.player_y;
        target_x = g_dungeon_runtime_state.player_x + 1;
        g_dungeon_move_direction = 5;
        passage_direction = 3;
        goto default_case;
      case 0x5100:
        target_x = g_dungeon_runtime_state.player_x;
        target_y = g_dungeon_runtime_state.player_y + 1;
        g_dungeon_move_direction = 7;
        passage_direction = 5;
        goto default_case;
      case 0x4f00:
        target_y = g_dungeon_runtime_state.player_y;
        target_x = g_dungeon_runtime_state.player_x + -1;
        g_dungeon_move_direction = 1;
        passage_direction = 7;
        goto default_case;
      default_case:
        if ((((target_x >= 0) && (target_x < DUNGEON_GRID_WIDTH)) && (target_y >= 0)) &&
            ((target_y < DUNGEON_GRID_HEIGHT && (DUNGEON_GRID_CELL(target_x, target_y) != 0))))
        {
          if ((DUNGEON_GRID_CELL(target_x, target_y) & 0xf0) != 0)
          {
            if (passage_direction != 0)
            {
              SetDungeonPassageHighlight(passage_direction, 1);
            }
            event_code = DUNGEON_GRID_CELL(target_x, target_y) / 16 & 0xf;
            if (g_dungeon_force_final_encounter != 0)
            {
              event_code = 0xf;
            }
            switch (event_code)
            {
            case 1:
              PlaySoundEffectOnChannel("x:sound\\dice.wav", 0xf, 100, 100, 0);
              if ((rand() & 1) != 0)
              {
                g_next_duel_card_id = internal_rand(3) + 1;
                g_dungeon_life_reward_delta = g_dungeon_life_reward_delta + g_next_duel_card_id;
                goto LAB_004443bd;
              }
            LAB_00444355:
              do
              {
                do
                {
                  event_code = deck[internal_rand(500)];
                  candidate_card_id = event_code & 0xfff;
                } while (candidate_card_id < 6);
              } while ((event_code & 0x4000) != 0);
              event_code &= 0xfff;
              if ((global_cards_data[event_code].type & 0x42) == 0)
                goto LAB_00444355;
              g_next_duel_card_id = event_code;
            LAB_004443bd:
              strcpy(g_ui_message_buffer, "");
              AppendDungeonRewardDescription();
              strcpy(reward_description, g_ui_message_buffer);
              sprintf(g_ui_message_buffer, gs_dungeon_0077f000[0], reward_description);
              if (g_next_duel_card_id == -1)
                goto LAB_00444355;
              RunTextMenuAtScaled(g_ui_message_buffer, 100, 0x50);
              DUNGEON_GRID_CELL(target_x, target_y) = 0x101;
              DrawCastleDungeonBoard(0, 0, dungeon_index);
              break;
            case 2:
              PlaySoundEffectOnChannel("x:sound\\scroll.wav", 0xf, 100, 100, 0);
              if (RunCardRiddleChallenge() == 0)
              {
                PlaySoundEffectOnChannel("x:sound\\dsummon.wav", 0xf, 100, 100, 0);
                DUNGEON_GRID_CELL(target_x, target_y) = 0x61;
                target_x = g_dungeon_runtime_state.player_x;
                target_y = g_dungeon_runtime_state.player_y;
                passage_direction = 0;
              }
              else
              {
                DUNGEON_GRID_CELL(target_x, target_y) = 0x101;
              }
              DrawCastleDungeonBoard(0, 0, dungeon_index);
              break;
            case 3:
            case 4:
            case 5:
            case 6:
              if (RunDungeonMonsterDuel(dungeon_index, event_code - 3, 0) == 0)
              {
                DUNGEON_GRID_CELL(target_x, target_y) = 0;
                target_x = g_dungeon_runtime_state.player_x;
                target_y = g_dungeon_runtime_state.player_y;
                passage_direction = 0;
                if ((g_castle_dungeon_slots[dungeon_index].rules_bitmap & 1) != 0)
                {
                  board_state = 0;
                  lost_required_duel = 1;
                  RefreshAdventureInterfaceLayout();
                }
                AddJournalEntry(JOURNAL_ENTRY_CREATURE_DUEL, g_dungeon_runtime_state.encounter.monster_sound_ids[event_code]);
              }
              else
              {
                DUNGEON_GRID_CELL(target_x, target_y) = 0x101;
                AddJournalEntry(JOURNAL_ENTRY_CREATURE_DUEL,
                                (uint)g_dungeon_runtime_state.encounter.monster_sound_ids[event_code] | 0x80);
              }
              DrawCastleDungeonBoard(0, 0, dungeon_index);
              break;
            case 7:
              card_slot_attempts = 0;
              do
              {
                event_code = internal_rand(3);
              } while ((&g_castle_dungeon_slots[dungeon_index].card_slot_1)[event_code] == -1 && ++card_slot_attempts < 100);

              if ((&g_castle_dungeon_slots[dungeon_index].card_slot_1)[event_code] != -1)
              {
                PlaySoundEffectOnChannel("x:sound\\findcard.wav", 0xf, 100, 100, 0);
                TransitionPcxToScreenRandomTiles8("staceybk.pic");
                AddJournalEntry(JOURNAL_ENTRY_DUNGEON_TREASURE,
                                (&g_castle_dungeon_slots[dungeon_index].card_slot_1)[event_code] | 0x10000);
                DrawAdventureCardSized((&g_castle_dungeon_slots[dungeon_index].card_slot_1)[event_code], 0x22, 0x53, 0x4b, 0x70, 1, "");
                DrawTextAt(g_page0_window_bounds, 0x1b, 0x90, 0x93, gs_dungeon_0077f000[3]);
                ClearInputAndWaitForMouseRelease();
                WaitForInputEventUnlessBlocked();
                DrawCastleDungeonBoard(0, 0, dungeon_index);
                result = AddCardToDeckSorted((&g_castle_dungeon_slots[dungeon_index].card_slot_1)[event_code]);
                deck[result] = deck[result] | 0x4000;
                (&g_castle_dungeon_slots[dungeon_index].card_slot_1)[event_code] = -1;
              }
              else
              {
                gold_reward = internal_rand(100) * (g_shandalar_difficulty + 2) + 100;
                has_mana_reward = 0;
                gold_reward = gold_reward - gold_reward % 10;
                PlaySoundEffectOnChannel("x:sound\\treasure.wav", 0xf, 100, 100, 0);
                Gold = Gold + gold_reward;
                mana_reward_text[0] = '\0';
                for (mana_color = 1; mana_color <= 5; mana_color = mana_color + 1)
                {
                  mana_reward_delta = internal_rand(2);
                  if (mana_reward_delta != 0)
                  {
                    sprintf((char *)((int)mana_reward_text + strlen(mana_reward_text)), gs_dungeon_0077f000[mana_color + 5], mana_reward_delta);
                    has_mana_reward = 1;
                    g_amulet_inventory[mana_color - 1] += mana_reward_delta;
                  }
                }
                if (has_mana_reward != 0)
                {
                  sprintf(g_ui_message_buffer, gs_dungeon_0077f000[5], gold_reward, mana_reward_text);
                }
                else
                {
                  sprintf(g_ui_message_buffer, gs_dungeon_0077f000[4], gold_reward);
                }
                AddJournalEntry(JOURNAL_ENTRY_DUNGEON_TREASURE, 100);
                RunTextMenuAtScaled(g_ui_message_buffer, 100, 0x50);
              }
              DUNGEON_GRID_CELL(target_x, target_y) = 0x101;
              DrawCastleDungeonBoard(0, 0, dungeon_index);
              break;
            case 0xf:
              if (dungeon_index < 5)
              {
                scratch_504 = RunDungeonMonsterDuel(dungeon_index, 4, 1);
                if (scratch_504 == 1)
                {
                  FreeSpriteBlob(g_dungeon_runtime_state.button_sprite_blob);
                  FreeSpriteBlob(g_dungeon_sprite_blob);
                  HandleDefeatedWizardCastle(dungeon_index);
                }
                if (scratch_504 != -1)
                {
                  RefreshAdventureInterfaceLayout();
                  FreeDungeonScratchPage();
                  g_next_duel_life_delta = 0;
                  g_next_duel_card_id = -1;
                  g_dungeon_life_reward_delta = 0;
                  return 0;
                }
                else
                {
                  passage_direction = 0;
                  target_x = g_dungeon_runtime_state.player_x;
                  target_y = g_dungeon_runtime_state.player_y;
                  DrawCastleDungeonBoard(0, 0, dungeon_index);
                }
              }
              break;
            }
            RevealDungeonCellAndNeighbors(target_x, target_y);
            if (g_dungeon_runtime_state.reveal_dirty != 0)
            {
              DrawCastleDungeonBoard(0, 0, dungeon_index);
            }
            SetFocus(g_main_window_hwnd);
          }
          if (passage_direction != 0)
          {
            SetDungeonPassageHighlight(passage_direction, 0);
          }
          ClearInputAndWaitForMouseRelease();
          g_dungeon_runtime_state.player_x = target_x;
          g_dungeon_runtime_state.player_y = target_y;
          RevealDungeonCellAndNeighbors(g_dungeon_runtime_state.player_x, g_dungeon_runtime_state.player_y);
          if (g_dungeon_runtime_state.reveal_dirty != 0)
          {
            DrawCastleDungeonBoard(0, 0, dungeon_index);
          }
          if ((entry_x == g_dungeon_runtime_state.player_x) && (entry_y == g_dungeon_runtime_state.player_y))
          {
            board_state = 0;
          }
        }

        break;
      case 0x1b:
        board_state = 0;
      default:
        break;
      }
    }

  } while (board_state != 0);

  if ((dungeon_index < 5) && (lost_required_duel != 0))
  {
    if (IsWizardColorFeminine(dungeon_index + 1) != 0)
    {
      sprintf(g_ui_message_buffer, gs_dungeon_0077f000[0xb],
              gs_wizardnames_0077ee70[dungeon_index + 1]);
    }
    else
    {
      sprintf(g_ui_message_buffer, gs_dungeon_0077f000[0xc],
              gs_wizardnames_0077ee70[dungeon_index + 1]);
    }
    PlaySoundEffectOnChannel("x:sound\\dsummon.wav", 0xf, 100, 100, 0);
    RunTextMenuAtScaled(g_ui_message_buffer, 0xa0, 0x78);
  }
  g_next_duel_life_delta = 0;
  g_next_duel_card_id = -1;
  g_dungeon_life_reward_delta = 0;
  FreeSpriteBlob(g_dungeon_runtime_state.button_sprite_blob);
  FreeSpriteBlob(g_dungeon_sprite_blob);
  RefreshAdventureInterfaceLayout();
  FreeDungeonScratchPage();
  sound_stop(100);
  sound_unload(100);
  if (dungeon_index >= 5)
  {
    ClearWorldMapPixelFlags(0x40, g_castle_dungeon_slots[dungeon_index].world_x,
                            g_castle_dungeon_slots[dungeon_index].world_y);
    g_castle_dungeon_slots[dungeon_index].clues_bitmap &= -2;
    g_castle_dungeon_slots[dungeon_index].clues_bitmap |= 6;
    do
    {
      do
      {
        grid_x = internal_rand(0x40);
        grid_y = internal_rand(0x40);
        random_or_flags = GetWorldMapPixelFlags(grid_x, grid_y) & 0xf;
      } while (GetWorldTileType(grid_x, grid_y) == 0);
    } while ((GetWorldMapPixelFlags(grid_x, grid_y) & 0x30) != 0);
    g_castle_dungeon_slots[dungeon_index].world_x = grid_x;
    g_castle_dungeon_slots[dungeon_index].world_y = grid_y;
    SetWorldMapPixelFlags(0x40, grid_x, grid_y);
  }
}

// FUNCTION: SHANDALAR 0x00446220
int RunDungeonMonsterDuel(int dungeon_index, int monster_slot, int final_battle)
{
  int sound_state;
  int work_index;
  int deck_index;
  uint card_id;
  int creature_type;
  int duel_result;

  for (deck_index = 0; deck_index < 500; deck_index = deck_index + 1)
  {
    card_id = deck[deck_index] & 0xfff;
    if ((g_castle_dungeon_slots[dungeon_index].rules_bitmap & 0x80) != 0)
    {
      DAT_00742fc0 = 0;
      if ((global_cards_data[card_id].type & 0x30) != 0)
      {
        deck[deck_index] |= 0x8000;
      }
    }
    if ((g_castle_dungeon_slots[dungeon_index].rules_bitmap & 0x40) != 0)
    {
      DAT_00742fc0 = 0;
      if ((global_cards_data[card_id].type & 0x40) != 0)
      {
        deck[deck_index] |= 0x8000;
      }
    }
    if ((g_castle_dungeon_slots[dungeon_index].rules_bitmap & 0x10) != 0)
    {
      DAT_00742fc0 = 0;
      if ((int)global_cards_data[card_id].color == 1 << (byte)g_castle_dungeon_slots[dungeon_index].color)
      {
        deck[deck_index] |= 0x8000;
      }
    }
  }
  g_current_encounter_color = (int)(char)g_castle_dungeon_slots[dungeon_index].color;
  if (g_castle_dungeon_slots[dungeon_index].card_in_effect != -1)
  {
    g_opponent_starting_card_id_2 = FindCardIndexByCsvid(g_castle_dungeon_slots[dungeon_index].card_in_effect);
  }
  creature_type = g_dungeon_runtime_state.encounter.selected.monster_creature_types[monster_slot];
  g_current_encounter_strength = (int)g_shandalar_monster_definitions[creature_type].base_strength;
  g_encounter_opening_hand_size_modifier = 0;
  if (monster_slot != 4)
  {
    g_ai_combat_value_weights[0] = 0x10;
  }
  if (dungeon_index < 5)
  {
    g_current_encounter_strength = 2;
    work_index = monster_slot + g_shandalar_difficulty + g_current_encounter_strength - 5;
    if (work_index >= 0)
    {
      g_opponent_starting_card_id_1 =
          FindCardIndexByCsvid(((int (*)[3])g_dungeon_monster_duel_music_csvids)
                                   [(char)g_castle_dungeon_slots[dungeon_index].color - 1][ClampIntToRange(work_index, 0, 2)]);
    }
  }
  LoadCreatureDuelDeck(creature_type, FindCardIndexByCsvid(g_shandalar_monster_definitions[creature_type].deck_number), 0,
                       -1);
  PlaySoundEffectOnChannel("x:sound\\dngnduel.wav", 0xf, 100, 100, 0);
  if (dungeon_index < 5)
  {
    sprintf(g_ui_message_buffer, gs_dungeon_0077f000[0xf], BuildCreatureNameWithArticle(creature_type));
  }
  else
  {
    sprintf(g_ui_message_buffer, gs_dungeon_0077f000[0x10], BuildCreatureNameWithArticle(creature_type));
  }
  if (g_opponent_starting_card_id_1 != -1)
  {
    work_index =
        ((int (*)[3])g_dungeon_monster_duel_music_csvids)[(char)g_castle_dungeon_slots[dungeon_index].color - 1]
                                                         [ClampIntToRange(work_index, 0, 2)];
    FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer + strlen(g_ui_message_buffer), 0x1000,
                                                gs_dungeon_0077f000[0x11], GetCreatureName(creature_type),
                                                global_cards_data[FindCardIndexByCsvid(work_index)].name);
  }
  if (g_opponent_starting_card_id_2 != -1)
  {
    sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_dungeon_0077f000[0x12],
            global_cards_data[FindCardIndexByCsvid(g_castle_dungeon_slots[dungeon_index].card_in_effect)].name);
  }
  RunTextMenuAtScaled(g_ui_message_buffer, 100, 0x50);
  for (work_index = 0; work_index < 0x10; work_index = work_index + 1)
  {
    global_ante_cards[1][work_index] = -1;
    global_ante_cards[0][work_index] = global_ante_cards[1][work_index];
  }
  ExitIfNoUsableDeckCards();
  do
  {
    do
    {
      work_index = internal_rand(500);
    } while (deck[work_index] == -1);
  } while (((deck[work_index] & 0x4000) != 0) || ((deck[work_index] & 0xfff) <= 4));
  global_ante_cards[0][0] = deck[work_index] & 0xfff;
  if ((0 < g_next_duel_card_id) && (g_next_duel_card_id <= 5))
  {
    g_next_duel_card_id = -1;
  }
  g_life[0] = CountDuelPoolEligibleTowns();
  g_life[0] = CountDuelPoolEligibleTowns();
  g_life[0] += g_next_duel_life_delta;
  g_life[0] = g_life[0] + g_dungeon_life_reward_delta;
  if ((0 < g_next_duel_card_id) && (g_next_duel_card_id <= 5))
  {
    g_life[0] = g_life[0] + g_next_duel_card_id;
  }
  g_player_starting_life = g_life[0];
  if (final_battle != 0)
  {
    sound_stop(100);
    do
    {
      if (sound_get_state(100, &sound_state) != 4)
      {
        if (sound_state == 1)
        {
          continue;
        }
      }
      break;
    } while (1);
    sound_unload(100);
  }
  else
  {
    sound_stop(100);
  }
  AnimatePaletteToColor(0, g_default_palette_fade_steps);
  duel_result = RunDuelEngine(0xffffffff, creature_type);
  LoadPcxIntoPageNoPalette("advfac64.pic");
  FadeInPaletteFromGray(0, g_default_palette_fade_steps);
  if ((duel_result != 1) && (duel_result == 0))
  {
    for (deck_index = 0; deck_index < 0x10; deck_index = deck_index + 1)
    {
      if (global_ante_cards[0][deck_index] == -1)
      {
        continue;
      }
      strcpy(g_ui_message_buffer, gs_dungeon_0077f000[0x13]);
      LoadPcxIntoPage(1, "losedul2.pic");
      StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x280, 0x1e0, g_page0_window_bounds, 0, 0, global_screen_width,
                              global_screen_height);
      DrawAdventureCard(global_ante_cards[0][0],
                        internal_rand(ScaleUiCoordinate(10)) + deck_index * 0x62 + 0x21,
                        internal_rand(ScaleUiCoordinate(10)) + 0x50, 1, g_ui_message_buffer);
      ClearInputAndWaitForMouseRelease();
      WaitForInputEventUnlessBlocked();
      RemoveCardFromDeckById(global_ante_cards[0][deck_index]);
    }
  }
  if ((g_castle_dungeon_slots[dungeon_index].rules_bitmap & 1) != 0)
  {
    g_next_duel_life_delta = g_life[0] - g_player_starting_life;
    g_dungeon_life_reward_delta += g_next_duel_life_delta;
    g_next_duel_life_delta = 0;
  }
  if ((g_castle_dungeon_slots[dungeon_index].rules_bitmap & 2) != 0)
  {
    g_next_duel_life_delta = 0;
    g_dungeon_life_reward_delta = 0;
  }
  g_next_duel_card_id = -1;
  if ((final_battle == 0) || (duel_result == -1))
  {
    LoadLoopingSound("x:sound\\dambloop.wav", 100);
    PlayLoopingSoundWithPan(100, 0x50, 0);
    set_sound_loop(100, 1);
  }
  return duel_result;
}

// FUNCTION: SHANDALAR 0x0050c0ff
undefined4 HandleDefeatedWizardCastle(int castle_index)
{
  int transition_tile_size;     // ebp - 0x130
  int menu_choice;              // ebp - 0x12c
  char card_browser_title[100]; // ebp - 0x128
  int card_browser_page;        // ebp - 0x28
  uint chosen_card;             // ebp - 0x24
  int wizard_color;             // ebp - 0x20
  int endgame_score;            // ebp - 0x1c
  int loop_index;               // ebp - 0x18
  int town_index;               // ebp - 0x14
  int final_creature;           // ebp - 0x10
  uint selected_cards[3];       // ebp - 0xc

  wizard_color = (int)(char)g_castle_dungeon_slots[castle_index].color;
  AddJournalEntry(JOURNAL_ENTRY_CREATURE_DUEL, (g_castle_defeat_music_ids[castle_index] + 1) * 7 | 0x80);
  ShowStatsWindow(1, (int)(char)g_castle_dungeon_slots[castle_index].color);
  LoadPcxIntoPageNoPalette("advfac64.pic");
  LoadPcxIntoPage(1, "tradscrn.pic");
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x280, 0x1e0, g_page0_window_bounds, 0, 0, global_screen_width,
                          global_screen_height);
  PlayCastleMusic(castle_index + 1);
  if (IsWizardColorFeminine(wizard_color) != 0)
  {
    sprintf(g_ui_message_buffer, gs_castlewin_0074b8c0[0], gs_wizardnames_0077ee70[wizard_color]);
  }
  else
  {
    sprintf(g_ui_message_buffer, gs_castlewin_0074b8c0[1], gs_wizardnames_0077ee70[wizard_color]);
  }
  for (loop_index = 0; loop_index < 0x80; loop_index = loop_index + 1)
  {
    if ((g_town_slots[loop_index].status_and_ruling_wizard & 0xff00) == wizard_color << 8)
    {
      g_town_slots[loop_index].status_and_ruling_wizard &= -65281;
      strcat(g_ui_message_buffer, BuildTownDisplayName(loop_index));
      strcat(g_ui_message_buffer, gs_castlewin_0074b8c0[2]);
    }
  }
  DrawUiScaledCenteredTextWithShadow(g_page0_window_bounds, 0xfe, 0x140, 0x72, g_ui_message_buffer);
  g_defeated_wizards_bitmap = g_defeated_wizards_bitmap | 1 << (byte)wizard_color;
  town_index = FindTownAtWorldCoordinates(g_castle_dungeon_slots[castle_index].world_x,
                                          g_castle_dungeon_slots[castle_index].world_y);
  g_town_slots[town_index].location_type = 5;
  for (loop_index = 0; loop_index < 8; loop_index = loop_index + 1)
  {
    if (g_lair_or_monster_slots[loop_index].color == wizard_color)
    {
      FreeOpeningMenuSpriteWorkEntries(loop_index, loop_index + 8);
      g_lair_or_monster_slots[loop_index].entry_type = SHANDALAR_ENTRY_NONE;
    }
  }
  card_browser_page = 1;
  sprintf(g_ui_message_buffer, gs_castlewin_0074b8c0[3], gs_colorcards_0077c5e0[wizard_color]);
  DrawUiScaledCenteredTextWithShadow(g_page0_window_bounds, 0xfe, 0x140, 200, g_ui_message_buffer);
  ClearInputAndWaitForMouseRelease();
  WaitForInputEventUnlessBlocked();
  g_town_dialog_callback = (TownDialogCallback)QuestCardChooserCallback;
  ResetMenuContext(BeginMenuContext());
  ForceReloadAdvfac64(1);
  g_defeated_wizards_bitmap = g_defeated_wizards_bitmap & ~(1 << (byte)wizard_color);

  for (loop_index = 0; loop_index < 3; loop_index = loop_index + 1)
  {
    sprintf(card_browser_title, gs_castlewin_0074b8c0[4], 3 - loop_index);
    chosen_card = RunCardBrowser(card_browser_title, 1 << (byte)wizard_color, 0xffffffff,
                                 card_browser_page, 0);
    if (chosen_card != 0xffffffff)
    {
      strcpy(g_ui_message_buffer, gs_castlewin_0074b8c0[5]);
      do
      {
        menu_choice = RunTextMenuAt(g_ui_message_buffer, ScaleUiCoordinate(0xf4) + 10,
                                    ScaleUiCoordinate(0x15c) + 10);
      } while (menu_choice < 0);
      if (menu_choice == 0)
      {
        town_index = AddCardToDeckSorted(chosen_card);
        if (town_index != -1)
        {
          deck[town_index] = deck[town_index] | 0x4000;
        }
        selected_cards[loop_index] = chosen_card;
        global_cards_data[chosen_card].extra_ability |= 0x100;
      }
      else
      {
        loop_index = loop_index + -1;
        continue;
      }
    }
    else
    {
      loop_index = loop_index + -1;
    }
    DelayUiTicks(0xf);
  }

  for (loop_index = 0; loop_index < 3; loop_index = loop_index + 1)
  {
    global_cards_data[selected_cards[loop_index]].extra_ability &= -257;
  }
  EndMenuContext();
  g_defeated_wizards_bitmap = g_defeated_wizards_bitmap | 1 << (byte)wizard_color;
  g_town_dialog_callback = (TownDialogCallback)DrawTiledDialogBoxFrame;
  if (g_defeated_wizards_bitmap != 0x3e)
  {
    sound_stop(0x10);
  }
  AnimatePaletteToColor(0, g_default_palette_fade_steps);
  if (g_defeated_wizards_bitmap == 0x3e)
  {
    DelayUiTicks(0x3c);
    LoadPcxIntoPageOpaque(1, "5thwiz.pic");
    StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x280, 0x1e0, g_page0_window_bounds, 0, 0, global_screen_width,
                            global_screen_height);
    strcpy(g_ui_message_buffer, gs_castlewin_0074b8c0[6]);
    *(undefined4 *)((int)g_page0_window_bounds + 0x20) = 5;
    DrawUiScaledCenteredTextWithShadow(g_page0_window_bounds, 99, 0x140, 0x100, g_ui_message_buffer);
    ClearInputAndWaitForMouseRelease();
    WaitForInputEventUnlessBlocked();
    endgame_score = CalculateDungeonEndgameScore();
    sprintf(g_ui_message_buffer, gs_castlewin_0074b8c0[7], MAX(endgame_score, 0));
    strcat(g_ui_message_buffer, gs_castlewin_0074b8c0[8]);
    DrawUiScaledCenteredTextWithShadow(g_page0_window_bounds, 99, 0x140, 0x15e, g_ui_message_buffer);
    ClearInputAndWaitForMouseRelease();
    WaitForInputEventUnlessBlocked();
    AnimatePaletteToColor(0, g_default_palette_fade_steps);
    DeckBuilderMain(g_main_window_hwnd, 1, 1);
    RestoreAdventureUiPaletteAndFocus();
    final_creature = PickRandomCreatureTypeForWizardTier(0, 100);
    g_shandalar_monster_definitions[final_creature].tier =
        g_shandalar_monster_definitions[final_creature].tier *
        (g_shandalar_difficulty + 1);
    LoadCreatureDuelDeck(final_creature, 0, 0, -1);
    g_current_encounter_color = wizard_color;
    g_encounter_opening_hand_size_modifier = 0;
    g_current_encounter_strength = 3;
    g_next_duel_life_delta = 0;
    sound_stop(0x10);
    g_opponent_starting_card_id_1 = FindCardIndexByCsvid(0x11);
    g_opponent_starting_card_id_2 = FindCardIndexByCsvid(0x1d);
    RunDuelEngine(0, final_creature);
    AddJournalEntry(JOURNAL_ENTRY_CREATURE_DUEL, 0xb7);
    AnimatePaletteToColor(0, g_default_palette_fade_steps);
    ClearGraphicsPageWithPaletteColor(0, 0);
    LoadPcxIntoPageNoPalette("advfac64.pic");
    PlayStatWinMovie("mtgend.avi", (global_screen_width + -0x230) / 2,
                     (global_screen_height + -0x1a4) / 2, 0);
    SetForegroundWindow(g_main_window_hwnd);
    BringWindowToTop(g_main_window_hwnd);
    SetFocus(g_main_window_hwnd);
    LoadPcxIntoPageNoPalette("advfac64.pic");
    ReadPalette("todpal.tr", (char *)0x0);
    SelectPalette(g_graphics_pages[0]->hTempDC, g_palette_handle, 0);
    RealizePalette(g_graphics_pages[0]->hTempDC);
    PlayCastleMusic(6);
    endgame_score = endgame_score + (100 - g_life[1]) * 10;
    LoadPcxIntoPageNoPalette("wingame.pic");
    if (global_screen_width == 0x280)
    {
      transition_tile_size = 3;
    }
    else if (global_screen_width == 800)
    {
      transition_tile_size = 4;
    }
    else
    {
      transition_tile_size = 6;
    }
    TransitionPcxToScreenRandomTiles("wingame.pic", transition_tile_size, transition_tile_size);
    sprintf(g_ui_message_buffer, gs_castlewin_0074b8c0[9], endgame_score);
    strcat(g_ui_message_buffer, gs_castlewin_0074b8c0[10]);
    DrawTextAt(g_page0_window_bounds, 0xd8, 0x140, 0x81, g_ui_message_buffer);
    ClearInputAndWaitForMouseRelease();
    WaitForInputEventUnlessBlocked();
    AnimatePaletteToColor(0, g_default_palette_fade_steps);
    LoadPcxIntoPageNoPalette("advfac64.pic");
    FreeDungeonScratchPage();
    RunAdventureStatsMenu();
    g_adventure_world_exit_requested = 1;
    UnloadStatWinDllExports();
    ShutdownSharedStartup();
    exit(0xff);
  }
  return 0;
}

// FUNCTION: SHANDALAR 0x0050a5c1
void EnterCastleDungeon(int dungeon_index)
{
  RunCastleDungeonBoard(dungeon_index);
  g_opponent_starting_card_id_1 = -1;
  g_opponent_starting_card_id_2 = -1;
  return;
}

// FUNCTION: SHANDALAR 0x00446a57
void FreeDungeonScratchPage(void)
{
  BlitGraphicsRect(g_dungeon_scratch_page9, 0, 0, global_screen_width, 0x80, g_page2_window_bounds, 0, 0);
  FreeGraphicsPage(9);
}

// FUNCTION: SHANDALAR 0x0043125f
void CopyUiScaledGraphicsRect(FacemakerWindowBounds *src_page, int src_x_320, int src_y_240, int width_320,
                              int height_240, FacemakerWindowBounds *dst_page, int dst_x_320, int dst_y_240)
{
  CopyGraphicsRect(src_page, (global_screen_width * src_x_320) / 0x140,
                   (global_screen_height * src_y_240) / 0xf0,
                   (width_320 * global_screen_width) / 0x140,
                   (height_240 * global_screen_height) / 0xf0,
                   dst_page, (dst_x_320 * global_screen_width) / 0x140,
                   (dst_y_240 * global_screen_height) / 0xf0);
}

// FUNCTION: SHANDALAR 0x004312ea
void FillUiScaledGraphicsRect(FacemakerWindowBounds *window_bounds, int x_320, int y_240, int width_320,
                              int height_240, int color_index)
{
  FillGraphicsRect(window_bounds, (x_320 * global_screen_width) / 0x140,
                   (y_240 * global_screen_height) / 0xf0,
                   (width_320 * global_screen_width) / 0x140,
                   (height_240 * global_screen_height) / 0xf0, color_index);
}

// FUNCTION: SHANDALAR 0x0044553d
void DrawCastleDungeonBoard(int animation_step, int initial_draw, int dungeon_index)
{
  int title_left;
  int title_piece_count;
  int title_middle_width;
  int title_width;
  int title_piece_width;
  int title_end_width;
  int title_tmp;
  int draw_direction;
  int screen_y;
  int screen_x;
  int work;
  int cell_y;
  int monster_direction;
  int cell_x;
  int player_screen_y;
  int player_screen_x;
  int direction;

  if (initial_draw == 0)
  {
    g_page0_window_bounds->page_number = 1;
  }
  StretchBlitGraphicsRect(g_page2_window_bounds, 0, 0, 0x280, 0x1e0, g_page0_window_bounds, 0, 0,
                          ScaleUiCoordinateFrom320(0x140), ScaleUiCoordinateFrom320(0xf0));
  g_page0_window_bounds->font_slot = 4;
  strcpy(g_ui_message_buffer, GetDungeonName(dungeon_index));
  title_width = MeasureTextLineWidth(g_ui_message_buffer);
  title_piece_count = 0;
  title_piece_width = g_dungeon_runtime_state.button_sprite_aux0->width;
  title_middle_width = g_dungeon_runtime_state.button_sprite_aux1->width;
  title_end_width = g_dungeon_runtime_state.button_sprite_aux2->width;
  title_tmp = title_end_width + title_piece_width + -0x32;
  while (title_tmp < title_width)
  {
    title_piece_count++;
    title_tmp += title_middle_width;
  }
  title_left = ScaleUiCoordinate(0x140) - (title_middle_width * title_piece_count) / 2 - title_piece_width;
  DrawEncodedImageUnscaled(g_page0_window_bounds, title_left, 1, g_dungeon_runtime_state.button_sprite_aux0);
  title_left += title_piece_width;
  while (title_piece_count-- != 0)
  {
    DrawEncodedImageUnscaled(g_page0_window_bounds, title_left, 1, g_dungeon_runtime_state.button_sprite_aux1);
    title_left += title_middle_width;
  }
  DrawEncodedImageUnscaled(g_page0_window_bounds, title_left, 1, g_dungeon_runtime_state.button_sprite_aux2);
  DrawCenteredTextLineWithShadow(g_ui_message_buffer, ScaleUiCoordinate(0x140), 0x12, 0xff);

  g_page0_window_bounds->font_slot = 1;
  if (g_next_duel_card_id != -1)
  {
    strcpy(g_ui_message_buffer, "");
    AppendDungeonRewardDescription();
    DrawFormattedTextShadowedCentered(g_page0_window_bounds, 0xd0, ScaleUiCoordinate(0x140), 0x30, g_ui_message_buffer);
  }

  for (cell_x = DUNGEON_GRID_WIDTH - 1; cell_x >= 0; cell_x--)
  {
    for (cell_y = 0; cell_y < DUNGEON_GRID_HEIGHT; cell_y++)
    {
      ConvertDungeonCellToScreen(cell_x, cell_y, &screen_x, &screen_y);
      if ((animation_step == 0) && ((DUNGEON_GRID_CELL(cell_x, cell_y) & 0x100) == 0))
      {
        if ((screen_x >= 0) && (screen_y >= 0) && (screen_x <= 0x240) && (screen_y <= 0x190))
        {
          if (DUNGEON_GRID_CELL(cell_x, cell_y) == 0)
          {
            work = 0;
            for (direction = 1; direction <= 8; direction += 2)
            {
              if ((DUNGEON_GRID_CELL(cell_x + g_neighbor_dx[direction],
                                     cell_y + g_neighbor_dy[direction]) &
                   0x100) != 0)
              {
                work++;
              }
            }
            if (work > 1)
            {
              DrawEncodedImageUiScaled(g_page0_window_bounds, screen_x - 0x20, screen_y - 0x30,
                                       g_dungeon_sprite_entries[0x14], 0x40, 0x40);
            }
          }
        }
        continue;
      }
      if (DUNGEON_GRID_CELL(cell_x, cell_y) != 0)
      {
        work = 0;
        if (DUNGEON_GRID_CELL(cell_x + 1, cell_y) == 0)
        {
          work |= 1;
        }
        if (DUNGEON_GRID_CELL(cell_x, cell_y - 1) == 0)
        {
          work |= 2;
        }
        switch (((int (*)[DUNGEON_GRID_HEIGHT])g_dungeon_runtime_state.cell_tile_variants)[cell_x][cell_y])
        {
        case 0:
          break;
        case 1:
          work += 4;
          break;
        case 2:
          work += 0x18;
          break;
        case 3:
          work += 0x1c;
          break;
        }
        DrawEncodedImageUiScaled(g_page0_window_bounds, screen_x - 0x20, screen_y - 0x30,
                                 g_dungeon_sprite_entries[work], 0x40, 0x40);

        work = 0;
        if (DUNGEON_GRID_CELL(cell_x, cell_y + 1) == 0)
        {
          work |= 1;
        }
        if (DUNGEON_GRID_CELL(cell_x - 1, cell_y) == 0)
        {
          work |= 2;
        }
        if (cell_y == 0xc)
        {
          work |= 1;
        }
        if ((cell_y & 1) != 0)
        {
          work += 4;
        }
        DrawEncodedImageUiScaled(g_page0_window_bounds, screen_x - 0x20, screen_y - 0x30,
                                 g_dungeon_sprite_entries[work + 0xc], 0x40, 0x40);
      }

      strcpy(g_ui_message_buffer, "");
      if (((DUNGEON_GRID_CELL(cell_x, cell_y) & 0xf0) != 0) || (cell_y == 0xc))
      {
        work = (DUNGEON_GRID_CELL(cell_x, cell_y) / 16) & 0xf;
        switch (work)
        {
        case 1:
          DrawEncodedImageUiScaled(g_page0_window_bounds, screen_x - 0x20, screen_y - 0x30,
                                   g_dungeon_sprite_entries[10], 0x40, 0x40);
          break;
        case 2:
          DrawEncodedImageUiScaled(g_page0_window_bounds, screen_x - 0x20, screen_y - 0x30,
                                   g_dungeon_sprite_entries[9], 0x40, 0x40);
          break;
        case 3:
          break;
          FillUiScaledGraphicsRect(g_page0_window_bounds, screen_x + 2, screen_y + 2, 0xc, 0xc, 0xd8);
          break;
        case 4:
          break;
          FillUiScaledGraphicsRect(g_page0_window_bounds, screen_x + 2, screen_y + 2, 0xc, 0xc, 0xdc);
          break;
        case 5:
          break;
          FillUiScaledGraphicsRect(g_page0_window_bounds, screen_x + 2, screen_y + 2, 0xc, 0xc, 0xbc);
          break;
        case 6:
          break;
          FillUiScaledGraphicsRect(g_page0_window_bounds, screen_x + 2, screen_y + 2, 0xc, 0xc, 0xbe);
          break;
        case 7:
          DrawEncodedImageUiScaled(g_page0_window_bounds, screen_x - 0x20, screen_y - 0x30,
                                   g_dungeon_sprite_entries[8], 0x40, 0x40);
          break;
        case 0xf:
          DrawEncodedImageUiScaled(g_page0_window_bounds, screen_x - 0x20, screen_y - 0x30,
                                   g_dungeon_sprite_entries[0x31], 0x40, 0x40);
          break;
        }

        if ((work >= 3) && (work < 7))
        {
          work -= 3;
          strcpy(g_ui_message_buffer, GetCreatureName(g_dungeon_runtime_state.encounter.selected.monster_creature_types[work]));
          draw_direction = 5;
          for (monster_direction = 1; monster_direction <= 8; monster_direction += 2)
          {
            direction = DUNGEON_CELL_DISTANCE(cell_x + g_neighbor_dx[monster_direction],
                                              cell_y + g_neighbor_dy[monster_direction]);
            if ((direction != 0) && (direction < DUNGEON_CELL_DISTANCE(cell_x, cell_y)))
            {
              draw_direction = monster_direction;
            }
          }
          DrawEncodedImageUnscaled(g_page0_window_bounds,
                                   ScaleUiCoordinate(screen_x + 4) - g_world_lair_monster_sprite_widths[work] / 2,
                                   ScaleUiCoordinate(screen_y + 4) - g_world_lair_monster_sprite_top_clips[work],
                                   g_opening_menu_sprite_work_buffer[work + 8].sprites[((draw_direction + 2) & 7) * 5]);
          DrawEncodedImageUnscaled(g_page0_window_bounds,
                                   ScaleUiCoordinate(screen_x + 4) - g_world_lair_monster_sprite_widths[work] / 2,
                                   ScaleUiCoordinate(screen_y + 4) - g_world_lair_monster_sprite_top_clips[work],
                                   g_opening_menu_sprite_work_buffer[work].sprites[((draw_direction + 2) & 7) * 5]);
        }
        else
        {
          if (cell_y == 0xc)
          {
            strcpy(g_ui_message_buffer, gs_dungeon_0077f000[0xd]);
          }
          DrawUiScaledCenteredText(g_ui_message_buffer, screen_x / 2, screen_y / 2 - 2, 0xff);
        }
      }

      if (g_dungeon_runtime_state.player_x == cell_x)
      {
        if (cell_y == g_dungeon_runtime_state.player_y)
        {
          player_screen_x = screen_x;
          player_screen_y = screen_y;
        }
      }
    }
  }

  if ((initial_draw != 0) && (g_castle_dungeon_slots[dungeon_index].card_in_effect != -1))
  {
    work = FindCardIndexByCsvid(g_castle_dungeon_slots[dungeon_index].card_in_effect);
    sprintf(g_ui_message_buffer, gs_dungeon_0077f000[0xe], global_cards_data[work].name);
    DrawEncodedImageResampled(g_page0_window_bounds, ScaleUiCoordinateFrom320(0xdc) / 2, ScaleUiCoordinateFrom320(0x34) / 2,
                              ScaleUiCoordinateFrom320(0xc5) / 2, ScaleUiCoordinateFrom320(0x10f) / 2,
                              g_dungeon_runtime_state.button_sprite_blob);
    DrawAdventureCardSized(work, 0x7a, 0x29, 0x4b, 0x70, 1, "");
    DrawScaledTextNoShadow(g_ui_message_buffer, 0x76, 0x20, 0x1b);
  }

  g_page0_window_bounds->page_number = 0;
  if (initial_draw == 0)
  {
    CopyGraphicsRect(g_page1_window_bounds, 0, 0, global_screen_width, global_screen_height, g_page0_window_bounds, 0, 0);
  }
  else
  {
    CopyGraphicsRect(g_page0_window_bounds, 0, 0, global_screen_width, global_screen_height, g_page1_window_bounds, 0, 0);
  }
  DrawEncodedImageUnscaled(g_page0_window_bounds,
                           ScaleUiCoordinate(player_screen_x) - g_sego_sprite_width / 2,
                           ScaleUiCoordinate(player_screen_y) - g_sego_sprite_draw_height,
                           g_opening_menu_sprite_work_buffer[0x11].sprites[g_dungeon_move_direction * 5]);
  DrawEncodedImageUnscaled(g_page0_window_bounds,
                           ScaleUiCoordinate(player_screen_x) - g_ego_sprite_width / 2,
                           ScaleUiCoordinate(player_screen_y) - g_ego_sprite_draw_height,
                           g_opening_menu_sprite_work_buffer[0x10].sprites[g_dungeon_move_direction * 5]);
}

// FUNCTION: SHANDALAR 0x00446049
void ConvertDungeonCellToScreen(int dungeon_x, int dungeon_y, int *out_screen_x, int *out_screen_y)
{
  *out_screen_x = dungeon_y * 0x20 + dungeon_x * 0x20 - 0x60;
  *out_screen_y = dungeon_x * -0x10 + dungeon_y * 0x10 + 0x110;
}

// FUNCTION: SHANDALAR 0x00446084
void ConvertMouseToDungeonCell(int mouse_x, int mouse_y, int *out_x, int *out_y)
{
  mouse_x = (mouse_x * 0x280) / global_screen_width;
  mouse_y = (mouse_y - 0x10) * 480 / global_screen_height;
  *out_x = ((mouse_x + 96) / 2 - (mouse_y - 272)) / 32;
  *out_y = (mouse_y + (mouse_x + 96) / 2 - 272) / 32 + 1;
}

// FUNCTION: SHANDALAR 0x00446101
void RevealDungeonCellAndNeighbors(int x, int y)
{
  int neighbor_cell;
  int neighbor_y;
  int neighbor_x;
  int direction_index;

  DUNGEON_GRID_CELL(x, y) = DUNGEON_GRID_CELL(x, y) | 0x100;
  g_dungeon_runtime_state.reveal_dirty = 0;
  for (direction_index = 1; direction_index <= 8; direction_index = direction_index + 2)
  {
    neighbor_x = x;
    neighbor_y = y;
    while (1)
    {
      neighbor_x = neighbor_x + g_neighbor_dx[direction_index];
      neighbor_y = neighbor_y + g_neighbor_dy[direction_index];
      if ((neighbor_x < 0) || (neighbor_x >= DUNGEON_GRID_WIDTH))
      {
        break;
      }
      if ((neighbor_y < 0) || (neighbor_y >= DUNGEON_GRID_HEIGHT))
      {
        break;
      }
      neighbor_cell = DUNGEON_GRID_CELL(neighbor_x, neighbor_y);
      if (neighbor_cell == 0)
      {
        break;
      }
      if ((DUNGEON_GRID_CELL(neighbor_x, neighbor_y) & 0x100) == 0)
      {
        g_dungeon_runtime_state.reveal_dirty = 1;
      }
      DUNGEON_GRID_CELL(neighbor_x, neighbor_y) = DUNGEON_GRID_CELL(neighbor_x, neighbor_y) | 0x100;
    }
  }
}

// FUNCTION: SHANDALAR 0x00445365
void SetDungeonPassageHighlight(int direction, int enable)
{
  int ego_sprite_frame;
  int sego_sprite_frame;
  int draw_x;
  int screen_y;
  int screen_x;
  int frame;
  int draw_y;

  ConvertDungeonCellToScreen(g_dungeon_runtime_state.player_x, g_dungeon_runtime_state.player_y, &screen_x, &screen_y);
  for (frame = 1; frame <= ((enable == 0) ? 8 : 4); frame++)
  {
    draw_x = screen_x - g_ego_sprite_width / 2;
    draw_y = screen_y - g_ego_sprite_draw_height;
    CopyUiScaledGraphicsRect(g_page1_window_bounds, draw_x / 2, draw_y / 2, g_ego_sprite_width / 2,
                             g_ego_sprite_height / 2, g_page0_window_bounds, draw_x / 2, draw_y / 2);
    draw_x = ScaleUiCoordinate(screen_x) - g_ego_sprite_width / 2;
    draw_y = ScaleUiCoordinate(screen_y) - g_ego_sprite_draw_height;
    if (frame == 8)
    {
      sego_sprite_frame = 0;
    }
    else
    {
      sego_sprite_frame = (frame & 3) + 1;
    }
    DrawEncodedImageUnscaled(g_page0_window_bounds, draw_x, draw_y,
                             g_opening_menu_sprite_work_buffer[0x11].sprites[g_dungeon_move_direction * 5 + sego_sprite_frame]);
    if (frame == 8)
    {
      ego_sprite_frame = 0;
    }
    else
    {
      ego_sprite_frame = (frame & 3) + 1;
    }
    DrawEncodedImageUnscaled(g_page0_window_bounds, draw_x, draw_y,
                             g_opening_menu_sprite_work_buffer[0x10].sprites[g_dungeon_move_direction * 5 + ego_sprite_frame]);
    screen_x += (g_neighbor_dx[((direction - 2) & 7) + 1] << 4) / 4;
    screen_y += (g_neighbor_dy[((direction - 2) & 7) + 1] << 4) / 8;
    DelayUiTicks(5);
    ClearInputAndWaitForMouseRelease();
  }
}

// FUNCTION: SHANDALAR 0x004ed8d3
undefined4 TransitionPcxToScreenRandomTiles8(char *path)
{
  DIBSurface *work_page;
  DIBSurface *screen_page;

  work_page = g_graphics_pages[g_page1_window_bounds->page_number];
  screen_page = g_graphics_pages[g_page0_window_bounds->page_number];
  LoadPcxResource(1, 0, global_screen_height + -0x1e0, path, (void *)0);
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, global_screen_height + -0x1e0, 0x280, 0x1e0, g_page1_window_bounds, 0,
                          0, global_screen_width, global_screen_height);
  BlitRectByRandomTileOrderInPlace(screen_page->hTempDC, 0, 0, global_screen_width, global_screen_height, 8, 8,
                                   work_page->hTempDC);
  return 0;
}

// FUNCTION: SHANDALAR 0x00446922
void AppendDungeonRewardDescription(void)
{
  int town_reward_count = CountDuelPoolEligibleTowns();
  if (g_next_duel_card_id == 5)
  {
    g_next_duel_card_id = -1;
  }

  if ((g_next_duel_card_id > 0) && (g_next_duel_card_id <= 5))
  {
    sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_dungeon_0077f000[1], g_next_duel_card_id, g_dungeon_life_reward_delta + g_next_duel_life_delta + town_reward_count);
  }
  else if (g_next_duel_card_id == 0)
  {
    strcat(g_ui_message_buffer, gs_dungeon_0077f000[2]);
  }
  else if (5 < g_next_duel_card_id)
  {
    strcat(g_ui_message_buffer, global_cards_data[g_next_duel_card_id].name);
  }
}

// FUNCTION: SHANDALAR 0x00446b89
int QueueDungeonMouseNavigationInput(int mouse_x, int mouse_y)
{
  int click_grid_x;
  int key_codes[8];
  int current_screen_y;
  int current_screen_x;
  int abs_dy;
  int abs_dx;
  int direction_base;
  int click_grid_y;

  key_codes[0] = 0x4800;
  key_codes[1] = 0x4900;
  key_codes[2] = 0x4d00;
  key_codes[3] = 0x5100;
  key_codes[4] = 0x5000;
  key_codes[5] = 0x4f00;
  key_codes[6] = 0x4b00;
  key_codes[7] = 0x4700;
  ConvertMouseToDungeonCell(mouse_x, mouse_y, &click_grid_x, &click_grid_y);
  if (click_grid_x == g_dungeon_exit_x && click_grid_y == g_dungeon_exit_y)
  {
    PushQueuedKeyInput(0x1b);
    return 0;
  }
  ConvertDungeonCellToScreen(g_dungeon_runtime_state.player_x, g_dungeon_runtime_state.player_y,
                             &current_screen_x, &current_screen_y);
  current_screen_x = ScaleUiCoordinate(current_screen_x);
  current_screen_y = ScaleUiCoordinate(current_screen_y);
  mouse_x = mouse_x - current_screen_x;
  mouse_y = current_screen_y - mouse_y;
  abs_dx = abs(mouse_x);
  abs_dy = abs(mouse_y);
  if ((mouse_x >= 0) && (mouse_y >= 0))
  {
    direction_base = 0;
  }
  else if ((mouse_x >= 0) && (mouse_y < 0))
  {
    direction_base = 2;
  }
  else if ((mouse_x < 0) && (mouse_y < 0))
  {
    direction_base = 4;
  }
  else if ((mouse_x < 0) && (mouse_y >= 0))
  {
    direction_base = 6;
  }
  PushQueuedKeyInput(key_codes[direction_base + 1]);
  return 1;
}

// FUNCTION: SHANDALAR 0x00445256
static void MarkDungeonDistanceFromCell(int x, int y, int distance)
{
  int direction;
  int neighbor_y;
  int neighbor_x;

  distance++;
  DUNGEON_CELL_DISTANCE(x, y) = (char)distance;
  for (direction = 1; direction <= 8; direction += 2)
  {
    neighbor_x = x + g_neighbor_dx[direction];
    neighbor_y = y + g_neighbor_dy[direction];
    if (DUNGEON_GRID_CELL(neighbor_x, neighbor_y) == 0)
    {
      continue;
    }
    if ((DUNGEON_CELL_DISTANCE(neighbor_x, neighbor_y) != 0) &&
        (DUNGEON_CELL_DISTANCE(neighbor_x, neighbor_y) <= distance))
    {
      continue;
    }
    if ((((neighbor_x < 0) || (neighbor_x >= DUNGEON_GRID_WIDTH)) || (neighbor_y < 0)) ||
        (neighbor_y >= DUNGEON_GRID_HEIGHT))
    {
      continue;
    }
    MarkDungeonDistanceFromCell(neighbor_x, neighbor_y, distance);
  }
}

// FUNCTION: SHANDALAR 0x00444cef
int PopulateDungeonCellEvents(void)
{
  int connected_count;
  int diagonal_count;
  int min_dead_end_depth;
  int previous_row;
  int generation_ok;
  int event_code;
  int y;
  int direction;
  int x;
  int max_reachable_depth;
  int branch_count;

  for (x = 0; x < DUNGEON_GRID_WIDTH; x++)
  {
    for (y = 0; y < DUNGEON_GRID_HEIGHT; y++)
    {
      DUNGEON_CELL_DISTANCE(x, y) = 0;
    }
  }
  MarkDungeonDistanceFromCell(g_dungeon_runtime_state.player_x, g_dungeon_runtime_state.player_y, 0);

  min_dead_end_depth = 0x63;
  max_reachable_depth = 0;
  generation_ok = 1;
  previous_row = 0;
  branch_count = previous_row;
  for (y = 0; y < DUNGEON_GRID_HEIGHT; y++)
  {
    for (x = 0; x < DUNGEON_GRID_WIDTH; x++)
    {
      if (DUNGEON_GRID_CELL(x, y) == 0 || ((g_dungeon_runtime_state.player_x == x) && (g_dungeon_runtime_state.player_y == y)))
      {
        continue;
      }
      if (DUNGEON_CELL_DISTANCE(x, y) == 0)
      {
        generation_ok = 0;
      }
      connected_count = 0;
      for (direction = 1; direction <= 8; direction += 2)
      {
        if (DUNGEON_GRID_CELL(x + g_neighbor_dx[direction], y + g_neighbor_dy[direction]) != 0)
        {
          connected_count++;
        }
      }
      if (connected_count == 1)
      {
        if ((DUNGEON_CELL_DISTANCE(x, y) < 0x10) || (previous_row == y))
        {
          generation_ok = 0;
        }
        else if (previous_row != y)
        {
          branch_count++;
          previous_row = y;
          if (DUNGEON_CELL_DISTANCE(x, y) < min_dead_end_depth)
          {
            min_dead_end_depth = DUNGEON_CELL_DISTANCE(x, y);
          }
          if (DUNGEON_CELL_DISTANCE(x, y) > max_reachable_depth)
          {
            max_reachable_depth = DUNGEON_CELL_DISTANCE(x, y);
          }
        }
      }
    }
  }
  if ((generation_ok == 0) || (branch_count < 3))
  {
    return generation_ok;
  }

  previous_row = 0;
  branch_count = previous_row;
  for (y = 0; y < DUNGEON_GRID_HEIGHT; y++)
  {
    for (x = 0; x < DUNGEON_GRID_WIDTH; x++)
    {
      if (DUNGEON_GRID_CELL(x, y) == 0 || ((g_dungeon_runtime_state.player_x == x) && (g_dungeon_runtime_state.player_y == y)))
      {
        continue;
      }
      diagonal_count = 0;
      connected_count = diagonal_count;
      for (direction = 1; direction <= 8; direction += 2)
      {
        if (DUNGEON_GRID_CELL(x + g_neighbor_dx[direction], y + g_neighbor_dy[direction]) != 0)
        {
          connected_count++;
        }
        if (DUNGEON_GRID_CELL(x + g_neighbor_dx[direction + 1], y + g_neighbor_dy[direction + 1]) != 0)
        {
          diagonal_count++;
        }
      }
      if (connected_count == 1)
      {
        if ((DUNGEON_CELL_DISTANCE(x, y) < 0x10) || (previous_row == y))
        {
          generation_ok = 0;
        }
        else if (previous_row != y)
        {
          branch_count++;
          previous_row = y;
          if ((g_dungeon_runtime_state.current_dungeon_index < 5) && (branch_count == 1))
          {
            DUNGEON_GRID_CELL(x, y) |= 0xf0;
          }
          else
          {
            DUNGEON_GRID_CELL(x, y) |= 0x70;
          }
        }
      }
      else if (((min_dead_end_depth - DUNGEON_CELL_DISTANCE(x, y) + 1) %
                    (5 - ((g_castle_dungeon_slots[g_dungeon_runtime_state.current_dungeon_index].times_entered + g_shandalar_difficulty < 3)
                              ? g_castle_dungeon_slots[g_dungeon_runtime_state.current_dungeon_index].times_entered + g_shandalar_difficulty
                              : 3)) ==
                0) &&
               (DUNGEON_CELL_DISTANCE(x, y) < max_reachable_depth) &&
               (DUNGEON_CELL_DISTANCE(x, y) >= 8))
      {
        event_code = ClampIntToRange(((DUNGEON_CELL_DISTANCE(x, y) - min_dead_end_depth) / 4 - 2) +
                                         internal_rand(6) + connected_count + diagonal_count,
                                     0, 6);
        if (internal_rand(g_shandalar_difficulty + 3) == 0)
        {
          event_code = 1;
        }
        DUNGEON_GRID_CELL(x, y) |= event_code << 4;
      }
    }
  }
  return 2;
}

// FUNCTION: SHANDALAR 0x004469f7
void InitDungeonScratchPage(void)
{
  SetGraphicsPage(9, CreateGraphicsPage(9, global_screen_width, 0x81, 8));
  g_dungeon_scratch_page9->font_slot = 1;
  BlitGraphicsRect(g_page2_window_bounds, 0, 0, global_screen_width, 0x80, g_dungeon_scratch_page9, 0, 0);
}

// FUNCTION: SHANDALAR 0x004ed98c
undefined4 TransitionPcxToScreenRandomTiles(char *path, int x_scale, int y_scale)
{
  DIBSurface *work_page;
  DIBSurface *screen_page;

  work_page = g_graphics_pages[g_page1_window_bounds->page_number];
  screen_page = g_graphics_pages[g_page0_window_bounds->page_number];
  LoadPcxResource(1, 0, global_screen_height + -0x1e0, path, (void *)0);
  StretchBlitGraphicsRect(g_page1_window_bounds, 0, global_screen_height + -0x1e0, 0x280, 0x1e0, g_page1_window_bounds, 0,
                          0, global_screen_width, global_screen_height);
  BlitRectByRandomTileOrderInPlace(screen_page->hTempDC, 0, 0, global_screen_width, global_screen_height, x_scale,
                                   y_scale, work_page->hTempDC);
  return 0;
}

// FUNCTION: SHANDALAR 0x005628bf
void PlayCastleMusic(int castle_music_index)
{
  StopWorldLocationMusic();
  if (g_world_location_music_track_id != -1)
  {
    sound_unload(0x10);
  }
  g_world_location_music_track_id = castle_music_index + 0x15;
  switch (castle_music_index)
  {
  case 1:
    LoadLoopingSound("x:sound\\bcastle.wav", 0x10);
    break;
  case 2:
    LoadLoopingSound("x:sound\\ucastle.wav", 0x10);
    break;
  case 3:
    LoadLoopingSound("x:sound\\gcastle.wav", 0x10);
    break;
  case 4:
    LoadLoopingSound("x:sound\\rcastle.wav", 0x10);
    break;
  case 5:
    LoadLoopingSound("x:sound\\wcastle.wav", 0x10);
    break;
  case 6:
    LoadLoopingSound("x:sound\\wingame.wav", 0x10);
    break;
  }
  PlayLoopingSoundWithPan(0x10, 100, 0);
  g_world_location_music_active = 1;
}

// FUNCTION: SHANDALAR 0x0052283d
undefined4 PlayStatWinMovie(char *path, int x, int y, int flags)
{
  if (g_statwin_exports_by_ordinal[2] != 0)
  {
    return ((int(__cdecl *)(char *, int, int, int))g_statwin_exports_by_ordinal[2])(path, x, y, flags);
  }

  return 0;
}

// FUNCTION: SHANDALAR 0x0050c03d
int CalculateDungeonEndgameScore(void)
{
  int score;
  int victory_value;
  int index;

  score = -g_wizard_siege_count;
  for (index = 0; index < 0x80; index = index + 1)
  {
    if (g_town_slots[index].status_and_ruling_wizard & 0xff00)
    {
      score = score + -2;
    }
  }
  score = score * 3;
  for (index = 0; index < 1000; index = index + 1)
  {
    if ((char)g_duel_victory_log[index] != '\0')
    {
      victory_value = (int)(char)g_duel_victory_log[index] & 0xf;
      score += victory_value + 2;
      if (victory_value == 0xc)
      {
        score += 0x19;
      }
    }
  }
  score <<= 1;
  return score;
}
