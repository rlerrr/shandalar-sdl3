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
#include "shandalar_internal.h"
#include "magic/src/global_state.h"
#include "shandalar_global_strings.h"
#include "facemaker/src/facemaker_types.h"
#include "drawcardlib/src/pic.h"
#include "platform/cheat_options.h"


#define OPENING_MENU_WORK_ENTRY_SPRITES(entry_index) (g_opening_menu_sprite_work_buffer[(entry_index)].sprites)


extern int g_wizard_siege_count;

ShandalarEntryType PickRandomCreatureTypeForWizardTier(int wizard_color, int creature_tier);
int FindNearestTownIndex(int world_x, int world_y);
int internal_rand(int max_exclusive);
void AnimateVisitBackdropZoomIn(char *pcx_path);
char *BuildTownDisplayName(int town_index);
char *BuildCreatureNameWithArticle(int creature_type);
DWORD FormatMessageFromStringStripCarriageReturns(char *dst, DWORD max_length, LPCVOID format, ...);
int IsWizardColorFeminine(int color_index);
int ShutdownSharedStartup(void);
int sound_unload(int sound_id);
void PlayCreatureEncounterSound(int creature_type, int volume, int pitch_percent, int pan_percent);
int RunWorldLairMonsterEncounter(int slot_index, int monster_color);

// GLOBAL: SHANDALAR 0x00591270
int g_creature_encounter_sound_is_first = 1;

// FUNCTION: SHANDALAR 0x004bc6d3
void LoadCreatureEncounterSprites(int creature_type, int work_entry_index_a, int work_entry_index_b)
{
  EncodedImage *sprite_header;
  int sprite_count_a;
  int ok;
  int sprite_count_b;

  ok = 1;
  if (g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites[0] != (EncodedImage *)0)
  {
    return;
  }

  switch (creature_type)
  {
  case 1:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("BK_FWZ.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SFWZ.spr"));
    break;
  case 2:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("BK_KHT.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SB_KHT.spr"));
    break;
  case 3:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("BK_MWZ.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SMWZ.spr"));
    break;
  case 4:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("BK_LRD.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SB_LRD.spr"));
    break;
  case 5:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("BK_WG.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SBK_WG.spr"));
    break;
  case 6:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("BK_AMG.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SB_AMG.spr"));
    break;
  case 8:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("W_MWZ.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SW_MWZ.spr"));
    break;
  case 9:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("W_FWZ.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SFWZ.spr"));
    break;
  case 0xa:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("W_KHT.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SKHT.spr"));
    break;
  case 0xb:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("W_LRD.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SW_LRD.spr"));
    break;
  case 0xc:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("W_WG.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SW_WG.spr"));
    break;
  case 0xd:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("W_AMG.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SW_AMG.spr"));
    break;
  case 0xf:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("BU_FWZ.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SFWZ.spr"));
    break;
  case 0x10:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("BU_LRD.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SU_LRD.spr"));
    break;
  case 0x11:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("BU_MWZ.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SMWZ.spr"));
    break;
  case 0x12:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("BU_WRM.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SWRM.spr"));
    break;
  case 0x13:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("B_SFR.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SB_SFT.spr"));
    break;
  case 0x14:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("BU_AMG.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SU_AMG.spr"));
    break;
  case 0x16:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("G_MWZ.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SMWZ.spr"));
    break;
  case 0x17:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("G_KHT.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SKHT.spr"));
    break;
  case 0x18:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("G_FWZ.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SFWZ.spr"));
    break;
  case 0x19:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("G_WRM.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SWRM.spr"));
    break;
  case 0x1a:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("G_LRD.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SG_LRD.spr"));
    break;
  case 0x1b:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("G_AMG.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SG_AMG.spr"));
    break;
  case 0x1d:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("R_FWZ.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SR_FWZ.spr"));
    break;
  case 0x1e:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("R_MWZ.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SMWZ.spr"));
    break;
  case 0x1f:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("TROLL.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("STRL.spr"));
    break;
  case 0x20:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("R_LRD.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SR_LRD.spr"));
    break;
  case 0x21:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("R_WRM.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SR_WRM.spr"));
    break;
  case 0x22:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("R_AMG.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SR_AMG.spr"));
    break;
  case 0x23:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("R_AMG.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SR_AMG.spr"));
    break;
  case 0x24:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("M_TSK.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SM_TSK.spr"));
    break;
  case 0x25:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("M_TRL.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SM_TRL.spr"));
    break;
  case 0x26:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("M_APE.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SM_APE.spr"));
    break;
  case 0x27:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("M_CEN2.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SM_CEN.spr"));
    break;
  case 0x28:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("M_WG.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SM_WG.spr"));
    break;
  case 0x29:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("M_FNG.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SM_FNG.spr"));
    break;
  case 0x2a:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("M_CEN.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SM_CEN2.spr"));
    break;
  case 0x2b:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("M_LRD.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SM_LRD.spr"));
    break;
  case 0x2c:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("M_KHT.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SKHT.spr"));
    break;
  case 0x2d:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("M_FWZ.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SFWZ.spr"));
    break;
  case 0x2e:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("BK_DJN.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SDJN.spr"));
    break;
  case 0x2f:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("G_DJN.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SDJN.spr"));
    break;
  case 0x30:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("R_DJN.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SDJN.spr"));
    break;
  case 0x31:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("BU_DJN.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("SDJN.spr"));
    break;
  case 0x32:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("DG_BRU.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("S_DG.spr"));
    break;
  case 0x33:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("DG_UWB.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("S_DG.spr"));
    break;
  case 0x34:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("DG_GWR.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("S_DG.spr"));
    break;
  case 0x35:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("DG_RBG.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("S_DG.spr"));
    break;
  case 0x36:
    sprite_count_a = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites, BuildResolutionSpritePath("DG_WUG.spr"));
    sprite_count_b = ReadSpriteEntryPointers(g_opening_menu_sprite_work_buffer[work_entry_index_b].sprites, BuildResolutionSpritePath("S_DG.spr"));
    break;
  default:
    ok = 0;
  }

  if (ok)
  {
    sprite_header = g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites[0];
    g_world_lair_monster_sprite_widths[work_entry_index_a] = sprite_header->width;
    g_world_lair_monster_sprite_heights[work_entry_index_a] = sprite_header->height;
    g_world_lair_monster_sprite_top_clips[work_entry_index_a] = sprite_header->top_clip;
    if (g_world_lair_monster_sprite_heights[work_entry_index_a] < g_world_lair_monster_sprite_top_clips[work_entry_index_a])
    {
      g_world_lair_monster_sprite_top_clips[work_entry_index_a] = (g_world_lair_monster_sprite_heights[work_entry_index_a] * 2) / 3;
    }
    g_world_lair_monster_sprite_widths[work_entry_index_b] = g_world_lair_monster_sprite_widths[work_entry_index_a];
    g_world_lair_monster_sprite_heights[work_entry_index_b] = g_world_lair_monster_sprite_heights[work_entry_index_a];
    g_world_lair_monster_sprite_top_clips[work_entry_index_b] = g_world_lair_monster_sprite_top_clips[work_entry_index_a];
  }
  else
  {
    g_opening_menu_sprite_work_buffer[work_entry_index_a].sprites[0] = (EncodedImage *)0;
  }
}

// FUNCTION: SHANDALAR 0x005614c3
void PlayCreatureEncounterSound(int creature_type, int volume, int pitch_percent, int pan_percent)
{
  if (g_creature_encounter_sound_is_first == 0)
  {
    sound_unload(0xf);
  }
  g_creature_encounter_sound_is_first = 0;

  switch (g_shandalar_monster_definitions[creature_type].encounter_type)
  {
  case 1:
    LoadSoundWithDriveFallback("x:sound\\malewiz.wav", 0xf, 0);
    break;
  case 2:
    LoadSoundWithDriveFallback("x:sound\\fewiz.wav", 0xf, 0);
    break;
  case 3:
    LoadSoundWithDriveFallback("x:sound\\knight.wav", 0xf, 0);
    break;
  case 4:
    LoadSoundWithDriveFallback("x:sound\\lord.wav", 0xf, 0);
    break;
  case 5:
    LoadSoundWithDriveFallback("x:sound\\djinn.wav", 0xf, 0);
    break;
  case 6:
    LoadSoundWithDriveFallback("x:sound\\troll.wav", 0xf, 0);
    break;
  case 7:
    LoadSoundWithDriveFallback("x:sound\\wyrm.wav", 0xf, 0);
    break;
  case 8:
    LoadSoundWithDriveFallback("x:sound\\dragon.wav", 0xf, 0);
    break;
  case 9:
    LoadSoundWithDriveFallback("x:sound\\flyer.wav", 0xf, 0);
    break;
  case 10:
    LoadSoundWithDriveFallback("x:sound\\archmage.wav", 0xf, 0);
    break;
  }

  PlaySoundWithPitchAndPan(0xf, volume, pitch_percent, -pan_percent);
}

// FUNCTION: SHANDALAR 0x0055fd27
void UpdateWorldLairAndMonsterSlots(void)
{
  int slot_u;             // ebp - 0x6c
  int terrain_mask;       // ebp - 0x68
  int target_dy;          // ebp - 0x64
  int tile_dist_y;        // ebp - 0x60
  int tile_dist_x;        // ebp - 0x5c
  int can_run_movement;   // ebp - 0x58
  int color;              // ebp - 0x54
  int nearest_distance;   // ebp - 0x50
  int tile_mask;          // ebp - 0x4c
  int tile_x;             // ebp - 0x48
  int i;                  // ebp - 0x44
  int victory_count;      // ebp - 0x40
  int slot_index;         // ebp - 0x3c
  int spawn_y;            // ebp - 0x38
  int prev_y;             // ebp - 0x34
  int range;              // ebp - 0x30
  int move_dir;           // ebp - 0x2c
  int new_distance;       // ebp - 0x28
  int spawn_x;            // ebp - 0x24
  int creature_tier;      // ebp - 0x20
  int movement_step;      // ebp - 0x1c
  int tile_y;             // ebp - 0x18
  int distance_to_player; // ebp - 0x14
  int tmp;                // ebp - 0x10
  int nearest_slot;       // ebp - 0xc
  int speed;              // ebp - 0x8
  int prev_x;             // ebp - 0x4

  nearest_distance = 0x7fff;
#ifdef MODERN_FIXES
  nearest_slot = -1;
#endif

  for (slot_index = 0; slot_index < 6; slot_index = slot_index + 1)
  {
    distance_to_player = ApproximateDistance(g_world_player_x - g_lair_or_monster_slots[slot_index].world_x,
                                             g_world_player_y - g_lair_or_monster_slots[slot_index].world_y);
    if ((distance_to_player < nearest_distance) && (g_lair_or_monster_slots[slot_index].entry_type != SHANDALAR_ENTRY_LAIR))
    {
      nearest_distance = distance_to_player;
      nearest_slot = slot_index;
    }
  }

  if (g_lair_or_monster_slots[7].entry_type == SHANDALAR_ENTRY_NONE)
  {
    g_siege_indicator = 0;
  }

  for (slot_index = 0; slot_index < 8; slot_index = slot_index + 1)
  {
    distance_to_player = ApproximateDistance(g_world_player_x - g_lair_or_monster_slots[slot_index].world_x,
                                             g_world_player_y - g_lair_or_monster_slots[slot_index].world_y);

    tile_dist_x = abs(g_world_player_tile_x - g_lair_or_monster_slots[slot_index].world_x / 0x20);
    tile_dist_y = abs(g_world_player_tile_y - g_lair_or_monster_slots[slot_index].world_y / 0x20);

    if ((slot_index < 6) &&
        ((g_lair_or_monster_slots[slot_index].entry_type == SHANDALAR_ENTRY_NONE) || (tile_dist_x > 4) || (tile_dist_y > 4)))
    {
      color = 0;
      for (i = 0; (i < 0x100) && (color == 0); i = i + 1)
      {
        if (internal_rand(2) != 0)
        {
          spawn_x = (internal_rand(2) ? 4 : -4) + g_world_player_tile_x;
          spawn_y = g_world_player_tile_y + internal_rand(9) - 4;
        }
        else
        {
          spawn_y = (internal_rand(2) ? 4 : -4) + g_world_player_tile_y;
          spawn_x = g_world_player_tile_x + internal_rand(9) - 4;
        }

        tile_y = GetWorldTileType(spawn_x, spawn_y);
        color = GetWorldTileMagicMask(tile_y);
      }

      if (color == 0)
      {
        continue;
      }

      do
      {
        victory_count = internal_rand(6);
      } while ((color & (1U << (byte)victory_count)) == 0);
      color = victory_count;

      victory_count = g_lairs_explored / 8;
      for (i = 0; i < 1000; i = i + 1)
      {
        if (((int)(char)g_duel_victory_log[i] >> 4) == color)
        {
          victory_count = victory_count + 1;
        }
      }

      move_dir = ClampIntToRange(0x80 / (victory_count + 4), 6, 0x14);

      switch (internal_rand(move_dir) + 5 / (victory_count + 1))
      {
      case 0:
        creature_tier = 10;
        break;
      case 1:
        creature_tier = 10;
        break;
      case 2:
        creature_tier = 10;
        break;
      case 3:
        creature_tier = 0;
        break;
      case 4:
        creature_tier = 10;
        break;
      case 5:
        creature_tier = 8;
        break;
      case 6:
        creature_tier = 6;
        break;
      case 7:
        creature_tier = 4;
        break;
      case 8:
        creature_tier = 6;
        break;
      case 9:
        creature_tier = 0;
        break;
      case 0xa:
        creature_tier = 4;
        break;
      case 0xb:
        creature_tier = 6;
        break;
      case 0xc:
        creature_tier = 0;
        break;
      case 0xd:
        creature_tier = 4;
        break;
      case 0xe:
        creature_tier = 0;
        break;
      case 0xf:
        creature_tier = 4;
        break;
      default:
        creature_tier = 0;
        break;
      }

      if (creature_tier == 10)
      {
        creature_tier = internal_rand(200);
        if (creature_tier < 0x32)
        {
          creature_tier = 10;
        }
        else if (creature_tier < 0x55)
        {
          creature_tier = 0xb;
        }
        else if (creature_tier < 0x73)
        {
          creature_tier = 0xc;
        }
        else if (creature_tier < 0x8e)
        {
          creature_tier = 0xd;
        }
        else if (creature_tier < 0xa5)
        {
          creature_tier = 0xe;
        }
        else if (creature_tier < 0xb9)
        {
          creature_tier = 0x10;
        }
        else if (creature_tier < 200)
        {
          creature_tier = 0x12;
        }
      }

      if ((slot_index == 0) && (g_current_quest_type < 0) && (-100 < g_current_quest_type))
      {
        creature_tier = (int)g_shandalar_monster_definitions[-g_current_quest_type].tier;
      }

      if (creature_tier != 0)
      {
        if (((g_shandalar_difficulty == 0) && ((slot_index == 5) || (slot_index == 6))) ||
            ((g_shandalar_difficulty == 1) && (slot_index == 6)))
        {
          continue;
        }

#define WORLD_MONSTER_RESPAWN_TIMER_SCALE (4320 * 86400)
#define WORLD_MONSTER_RESPAWN_TIMER_BIAS 2309

        if (((4 - g_shandalar_difficulty) * WORLD_MONSTER_RESPAWN_TIMER_SCALE) /
                ((g_monster_timer / 0xf) * (g_monster_timer / 0xf) +
                 WORLD_MONSTER_RESPAWN_TIMER_BIAS * WORLD_MONSTER_RESPAWN_TIMER_BIAS) >
            g_monster_timer - g_lair_or_monster_slots[slot_index].respawn_timestamp)
        {
          continue;
        }
#undef WORLD_MONSTER_RESPAWN_TIMER_SCALE
#undef WORLD_MONSTER_RESPAWN_TIMER_BIAS
      }

      {
        if (creature_tier != 0)
        {
          creature_tier = PickRandomCreatureTypeForWizardTier(color, creature_tier);
          g_lair_or_monster_slots[slot_index].entry_type = (ShandalarEntryType)creature_tier;

          new_distance = 0;
          for (i = 0; (i < 1000) && ((char)g_duel_victory_log[i] != '\0'); i = i + 1)
          {
            if ((g_shandalar_monster_definitions[creature_tier].encounter_type == ((char)g_duel_victory_log[i] & 0xf)) &&
                (((int)(char)g_duel_victory_log[i] >> 4) == color))
            {
              new_distance = new_distance + 1;
            }
          }

          if (new_distance > 8)
          {
            g_lair_or_monster_slots[slot_index].entry_type = creature_tier = SHANDALAR_ENTRY_NONE;
          }
        }
        else
        {
          g_lair_or_monster_slots[slot_index].entry_type = SHANDALAR_ENTRY_LAIR;
        }

        g_lair_or_monster_slots[slot_index].world_x = spawn_x * 0x20 + 0x10;
        g_lair_or_monster_slots[slot_index].world_y = spawn_y * 0x20 + 0x10;
        g_lair_or_monster_slots[slot_index].color = color;
        g_lair_or_monster_slots[slot_index].movement_heading = 0;
        g_lair_or_monster_slots[slot_index].movement_anim_frame = 0;

        if (((1U << (byte)color) & (unsigned int)g_shandalar_monster_definitions[creature_tier].color_mask) &&
            ((g_defeated_wizards_bitmap & (1U << (byte)color)) != 0))
        {
          target_dy = (unsigned int)g_shandalar_monster_definitions[creature_tier].color_mask & ~(1U << (byte)color);
          if ((target_dy == 0) || ((g_defeated_wizards_bitmap & target_dy) != 0))
          {
            FreeOpeningMenuSpriteWorkEntries(slot_index, slot_index + 8);
            g_lair_or_monster_slots[slot_index].entry_type = SHANDALAR_ENTRY_NONE;
          }
          else if (target_dy != 0)
          {
            g_lair_or_monster_slots[slot_index].color = single_color_test_bit_to_color_t((int)target_dy);
          }
        }

        if ((GetWorldMapPixelFlags(spawn_x, spawn_y) & 0x10) != 0)
        {
          FreeOpeningMenuSpriteWorkEntries(slot_index, slot_index + 8);
          g_lair_or_monster_slots[slot_index].entry_type = SHANDALAR_ENTRY_NONE;
        }

        if ((slot_index == 0) && (g_current_quest_type < 0) &&
            (g_lair_or_monster_slots[slot_index].entry_type != -g_current_quest_type))
        {
          FreeOpeningMenuSpriteWorkEntries(slot_index, slot_index + 8);
          g_lair_or_monster_slots[slot_index].entry_type = SHANDALAR_ENTRY_NONE;
        }

        if (g_lair_or_monster_slots[slot_index].entry_type > SHANDALAR_ENTRY_LAIR)
        {
          FreeOpeningMenuSpriteWorkEntries(slot_index, slot_index + 8);
        }
        else if (g_lair_or_monster_slots[slot_index].entry_type != SHANDALAR_ENTRY_NONE)
        {
          for (terrain_mask = 0; terrain_mask < 8; terrain_mask = terrain_mask + 1)
          {
            if ((g_lair_or_monster_slots[terrain_mask].entry_type == SHANDALAR_ENTRY_NONE) ||
                (slot_index == terrain_mask))
            {
              continue;
            }
            if ((g_lair_or_monster_slots[slot_index].world_x == g_lair_or_monster_slots[terrain_mask].world_x) &&
                (g_lair_or_monster_slots[slot_index].world_y == g_lair_or_monster_slots[terrain_mask].world_y))
            {
              g_lair_or_monster_slots[slot_index].entry_type = SHANDALAR_ENTRY_NONE;
            }
          }
        }
      }
    }

    creature_tier = g_lair_or_monster_slots[slot_index].entry_type;
    if (creature_tier == SHANDALAR_ENTRY_NONE)
    {
      continue;
    }

    if (creature_tier > SHANDALAR_ENTRY_LAIR)
    {
      LoadCreatureEncounterSprites(creature_tier, slot_index, slot_index + 8);
    }

    movement_step = 1;
#ifdef MODERN_FIXES
    speed = 0;
    range = 0;
#endif
    switch (g_shandalar_monster_definitions[creature_tier].encounter_type)
    {
    case 0:
      range = -1;
      movement_step = 0;
      break;
    case 2:
      speed = 1;
      range = 0x20;
      movement_step = 1;
      break;
    case 3:
      speed = 1;
      range = 0x30;
      movement_step = 1;
      break;
    case 1:
      speed = 1;
      range = 0x20;
      movement_step = 1;
      break;
    case 4:
      speed = 1;
      range = 0x30;
      movement_step = 1;
      break;
    case 5:
      speed = 1;
      range = 0x40;
      movement_step = 1;
      break;
    case 6:
      speed = 1;
      range = 0x60;
      movement_step = 1;
      break;
    case 7:
      speed = 1;
      range = 0x60;
      movement_step = 1;
      break;
    case 8:
      speed = 1;
      range = 0x40;
      movement_step = 1;
      break;
    case 9:
      speed = 1;
      range = 0x20;
      movement_step = 2;
      break;
    case 0xa:
      speed = 1;
      range = 0x30;
      movement_step = 1;
      break;
    }

    range = (range * 3) / 2;
    if (g_shandalar_monster_definitions[creature_tier].flags_0a & 0x200)
    {
      range = range << 1;
    }

    if (slot_index != nearest_slot)
    {
      range = range / 2;
    }
    tmp = distance_to_player;
    while (abs(range) < tmp)
    {
      speed = speed << 1;
      tmp = tmp / 2;
    }

    prev_x = g_lair_or_monster_slots[slot_index].world_x;
    prev_y = g_lair_or_monster_slots[slot_index].world_y;

    if (g_world_player_animation_frame != 0)
    {
      tile_dist_x = (ClampIntToRange(distance_to_player / 3, 0, g_shandalar_difficulty << 4) *
                         g_neighbor_dx[g_world_player_animation_direction] +
                     g_world_player_x) -
                    prev_x;
      tile_dist_y = (ClampIntToRange(distance_to_player / 3, 0, g_shandalar_difficulty << 4) *
                         g_neighbor_dy[g_world_player_animation_direction] +
                     g_world_player_y) -
                    prev_y;
    }
    else
    {
      tile_dist_x = g_world_player_x - prev_x;
      tile_dist_y = g_world_player_y - prev_y;
    }

    if (g_shandalar_monster_definitions[creature_tier].flags_0a & 1)
    {
      movement_step = 2;
    }

    if ((distance_to_player < 0x40) && (slot_index < 7))
    {
      new_distance = 0;
      for (i = 0; (i < 1000) && ((char)g_duel_victory_log[i] != '\0'); i = i + 1)
      {
        if ((g_shandalar_monster_definitions[creature_tier].encounter_type == ((char)g_duel_victory_log[i] & 0xf)) &&
            (((int)(char)g_duel_victory_log[i] >> 4) == g_lair_or_monster_slots[slot_index].color))
        {
          new_distance = new_distance + 1;
        }
      }

      if (g_shandalar_difficulty + 3 <= new_distance)
      {
        tile_dist_x = -tile_dist_x;
        tile_dist_y = -tile_dist_y;
      }
    }

    if ((slot_index == 7) && (distance_to_player > 0x18))
    {
      tile_mask = FindNearestTownIndex(g_lair_or_monster_slots[slot_index].world_x / 0x20,
                                       g_lair_or_monster_slots[slot_index].world_y / 0x20);
      tile_dist_x = g_town_slots[tile_mask].world_x * 0x20 + 0x10 - prev_x;
      tile_dist_y = g_town_slots[tile_mask].world_y * 0x20 + 0x10 - prev_y;
      tile_dist_x += (g_monster_timer & 0x1f) - 0x10;
      tile_dist_y += ((g_monster_timer & 0x3e) >> 1) - 0x10;
    }

    tile_y = GetWorldTileType(
        g_lair_or_monster_slots[slot_index].world_x / 0x20,
        g_lair_or_monster_slots[slot_index].world_y / 0x20);
    color = GetWorldTileMagicMask(tile_y);

    if ((g_shandalar_monster_definitions[creature_tier].flags_0a & 0xf8) &&
        ((g_shandalar_monster_definitions[creature_tier].flags_0a & (color << 3)) != 0) &&
        (speed > 1))
    {
      speed = speed / 2;
    }

    can_run_movement = 0;
    tile_dist_x = tile_dist_x + g_neighbor_dx[g_lair_or_monster_slots[slot_index].movement_heading] * 8;
    tile_dist_y = tile_dist_y + g_neighbor_dy[g_lair_or_monster_slots[slot_index].movement_heading] * 8;

    if (abs(tile_dist_y) * 2 < abs(tile_dist_x))
    {
      if (tile_dist_x > 0)
      {
        can_run_movement = 3;
      }
      else
      {
        can_run_movement = 7;
      }
    }

    if (abs(tile_dist_x) * 2 < abs(tile_dist_y))
    {
      if (tile_dist_y > 0)
      {
        can_run_movement = 5;
      }
      else
      {
        can_run_movement = 1;
      }
    }

    if (can_run_movement == 0)
    {
      if (tile_dist_x > 0)
      {
        if (tile_dist_y > 0)
        {
          can_run_movement = 4;
        }
        else
        {
          can_run_movement = 2;
        }
      }
      else
      {
        if (tile_dist_y > 0)
        {
          can_run_movement = 6;
        }
        else
        {
          can_run_movement = 8;
        }
      }
    }

    if (((g_monster_timer + slot_index) & 3) == 0)
    {
      g_lair_or_monster_slots[slot_index].movement_heading = (signed char)can_run_movement;
    }
    else
    {
      can_run_movement = g_lair_or_monster_slots[slot_index].movement_heading;
    }

    g_lair_or_monster_slots[slot_index].world_x = g_lair_or_monster_slots[slot_index].world_x + g_neighbor_dx[can_run_movement] * 4;
    g_lair_or_monster_slots[slot_index].world_y = g_lair_or_monster_slots[slot_index].world_y + g_neighbor_dy[can_run_movement] * 4;

    tile_x = 1;
    for (i = 0; i < 6; i = i + 1)
    {
      if ((g_lair_or_monster_slots[i].entry_type == SHANDALAR_ENTRY_LAIR) ||
          (i == slot_index) ||
          (distance_to_player < 0x20))
      {
        continue;
      }

      victory_count = ApproximateDistance(g_lair_or_monster_slots[slot_index].world_x - g_lair_or_monster_slots[i].world_x,
                                          g_lair_or_monster_slots[slot_index].world_y - g_lair_or_monster_slots[i].world_y);
      if (victory_count < 0x20)
      {
        if (victory_count < ApproximateDistance(prev_x - g_lair_or_monster_slots[i].world_x,
                                                prev_y - g_lair_or_monster_slots[i].world_y))
        {
          tile_x = 0;
        }
      }
    }

    if (tile_x == 0)
    {
      g_lair_or_monster_slots[slot_index].world_x = prev_x;
      g_lair_or_monster_slots[slot_index].world_y = prev_y;
      g_lair_or_monster_slots[slot_index].movement_anim_frame = 0;
      continue;
    }

    if ((g_shandalar_monster_definitions[creature_tier].flags_0a & 4) && ((g_monster_timer & 0x3fU) == 0))
    {
      if (internal_rand(2) != 0)
      {
        if (internal_rand(2) != 0)
        {
          g_lair_or_monster_slots[slot_index].world_x = g_lair_or_monster_slots[slot_index].world_x + 0x20;
        }
        else
        {
          g_lair_or_monster_slots[slot_index].world_x = g_lair_or_monster_slots[slot_index].world_x - 0x20;
        }
      }
      else
      {
        if (internal_rand(2) != 0)
        {
          g_lair_or_monster_slots[slot_index].world_y = g_lair_or_monster_slots[slot_index].world_y + 0x20;
        }
        else
        {
          g_lair_or_monster_slots[slot_index].world_y = g_lair_or_monster_slots[slot_index].world_y - 0x20;
        }
      }
    }

    tile_y = GetWorldTileType(
        g_lair_or_monster_slots[slot_index].world_x / 0x20,
        g_lair_or_monster_slots[slot_index].world_y / 0x20);
    color = GetWorldTileMagicMask(tile_y);

    if (((color & (unsigned int)g_shandalar_monster_definitions[creature_tier].color_mask) == 0) &&
        (range >= 0) && (slot_index != 7))
    {
      g_lair_or_monster_slots[slot_index].world_x = prev_x;
      g_lair_or_monster_slots[slot_index].world_y = prev_y;
      tile_y = GetWorldTileType(prev_x / 0x20, prev_y / 0x20);
      color = GetWorldTileMagicMask(tile_y);
      g_lair_or_monster_slots[slot_index].movement_anim_frame = 0;

      if ((color & (unsigned int)g_shandalar_monster_definitions[creature_tier].color_mask) == 0)
      {
        FreeOpeningMenuSpriteWorkEntries(slot_index, slot_index + 8);
        g_lair_or_monster_slots[slot_index].entry_type = SHANDALAR_ENTRY_NONE;
      }
    }
    else
    {
      g_lair_or_monster_slots[slot_index].world_x = g_neighbor_dx[can_run_movement] * movement_step + prev_x;
      g_lair_or_monster_slots[slot_index].world_y = g_neighbor_dy[can_run_movement] * movement_step + prev_y;
      g_lair_or_monster_slots[slot_index].movement_heading = (signed char)can_run_movement;
      g_lair_or_monster_slots[slot_index].movement_anim_frame++;
      if (g_lair_or_monster_slots[slot_index].movement_anim_frame > 4)
      {
        g_lair_or_monster_slots[slot_index].movement_anim_frame = 1;
      }
    }

    if (ApproximateDistance(g_world_player_x - g_lair_or_monster_slots[slot_index].world_x,
                            g_world_player_y - g_lair_or_monster_slots[slot_index].world_y) <
        ((g_lair_or_monster_slots[slot_index].entry_type == SHANDALAR_ENTRY_LAIR) ? 0x1a : 0x10))
    {
      SaveGameToSlot(3);
      slot_u = RunWorldLairMonsterEncounter(slot_index, g_lair_or_monster_slots[slot_index].color);
      g_lair_or_monster_slots[slot_index].respawn_timestamp = g_monster_timer;
      RefreshAdventureInterfaceLayout();
      if ((slot_index == 7) && (slot_u <= 0))
      {
        ResolveWizardTownSiege();
      }
      else if (slot_index == 7)
      {
        AddJournalEntry(JOURNAL_ENTRY_LAIR_EXPLORED, g_lair_or_monster_slots[slot_index].color);
        g_siege_indicator = 0;
      }
      FreeOpeningMenuSpriteWorkEntries(slot_index, slot_index + 8);
      g_lair_or_monster_slots[slot_index].entry_type = SHANDALAR_ENTRY_NONE;
      can_run_movement = 0;
      EnsureAdvfac64Loaded(0);
      g_monster_timer = g_monster_timer | 0x1f;
      SaveGameToSlot(3);
    }
    else if ((creature_tier != 0) &&
             (g_lair_or_monster_slots[slot_index].movement_anim_frame != 0) &&
             (distance_to_player < 0x50) && (internal_rand(distance_to_player) < 4))
    {
      PlayCreatureEncounterSound(creature_tier, 0x68 - distance_to_player / 2, 100 - distance_to_player / 3,
                                 g_neighbor_dx[(g_lair_or_monster_slots[slot_index].movement_heading + 2U) & 7] *
                                     100);
    }
  }
}

// FUNCTION: SHANDALAR 0x005616f9
void StartWizardTownSiege(void)
{
  int unused_stack_slot;
  int ruled_by_color_count[7];
  int town_index;
  int color;
  int best_score;
  int nearest_distance;
  int best_color;
  int lair_world_y_by_color[7];
  int lair_world_x_by_color[7];
  int scan_index;
  int creature_tier;
  int dist;
  int best_town_index;
  int score;

  if (shandalar_cheat_enabled(SHANDALAR_CHEAT_DISABLE_SIEGES))
  {
    return;
  }

  if (g_siege_indicator != 0)
  {
    return;
  }

  ClearInputAndWaitForMouseRelease();

  for (scan_index = 0; scan_index < 7; scan_index++)
  {
    lair_world_x_by_color[scan_index] = -1;
    ruled_by_color_count[scan_index] = 0;
  }

  for (scan_index = 0; scan_index < 0x80; scan_index++)
  {
    if (g_town_slots[scan_index].location_type == 4)
    {
      color = single_color_test_bit_to_color_t((int)GetWorldTileMagicMask(GetWorldTileType(g_town_slots[scan_index].world_x, g_town_slots[scan_index].world_y)));
      lair_world_x_by_color[color] = g_town_slots[scan_index].world_x;
      lair_world_y_by_color[color] = g_town_slots[scan_index].world_y;
    }

    if (g_town_slots[scan_index].status_and_ruling_wizard & 0xff00)
    {
      ruled_by_color_count[g_town_slots[scan_index].status_and_ruling_wizard >> 8] += 1;
    }
  }

  unused_stack_slot = 0;
  best_score = 0x7fff;
  color = -1;

  for (town_index = 0; town_index < 0x80; town_index = town_index + 1)
  {
    if ((((g_town_slots[town_index].status_and_ruling_wizard & 0xff00) ||
          (g_town_slots[town_index].location_type == 4)) ||
         (g_town_slots[town_index].location_type == 1)) ||
        (g_town_slots[town_index].location_type == 5))
    {
      continue;
    }

    nearest_distance = 0x7fff;
    for (scan_index = 1; scan_index < 6; scan_index = scan_index + 1)
    {
      if (lair_world_x_by_color[scan_index] == -1)
      {
        continue;
      }

      dist = ApproximateDistance(g_town_slots[town_index].world_x - lair_world_x_by_color[scan_index],
                                 g_town_slots[town_index].world_y - lair_world_y_by_color[scan_index]);
      if (dist < nearest_distance)
      {
        nearest_distance = dist;
        color = scan_index;
      }
    }

    score = ruled_by_color_count[color] * 0x20 + internal_rand(0x80);
    if (score < best_score)
    {
      best_score = score;
      best_town_index = town_index;
      best_color = color;
    }
  }

  color = best_color;
  town_index = best_town_index;

  if (color == -1)
  {
  }
  else
  {
    scan_index = 7;

    switch (g_siege_timer / 0x80)
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
    default:
      if ((g_town_slots[town_index].world_y & 1) != 0)
      {
        creature_tier = 0xc;
      }
      else
      {
        creature_tier = 0x10;
      }
      break;
    }

    FreeOpeningMenuSpriteWorkEntries(scan_index, scan_index + 8);
    g_lair_or_monster_slots[scan_index].entry_type = PickRandomCreatureTypeForWizardTier(color, creature_tier);
    g_lair_or_monster_slots[scan_index].world_x = g_town_slots[town_index].world_x * 0x20 + 0x10;
    g_lair_or_monster_slots[scan_index].world_y = g_town_slots[town_index].world_y * 0x20 + 0x10;
    g_lair_or_monster_slots[scan_index].color = color;

    g_wizard_siege_count = g_wizard_siege_count + 1;
    SetWorldMapPixelFlags(0x80, g_town_slots[town_index].world_x, g_town_slots[town_index].world_y);

    PlaySoundEffectOnChannel("x:sound\\newsflash.wav", 0x97, 100, 100, 0);
    AnimateVisitBackdropZoomIn("newsback.pic");

    strcpy(g_ui_message_buffer, gs_newsflash_0077d140[0]);
    FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer + strlen(g_ui_message_buffer), 0x1000, gs_newsflash_0077d140[1],
                                                gs_wizardnames_0077ee70[color],
                                                BuildCreatureNameWithArticle(g_lair_or_monster_slots[scan_index].entry_type),
                                                BuildTownDisplayName(town_index));

    g_page0_window_bounds->font_slot = 5;
    DrawTextAt(g_page0_window_bounds, 0xbe, 0x140, 0xf7, g_ui_message_buffer);
    g_page0_window_bounds->font_slot = 1;
    ClearInputAndWaitForMouseRelease();
    WaitForInputEventUnlessBlocked();
    RefreshAdventureInterfaceLayout();
    g_siege_indicator = 1;
  }
}

// FUNCTION: SHANDALAR 0x00562169
void ResolveWizardTownSiege(void)
{
  int font_size;
  int ruled_count;
  int slot_index;
  int town_index;
  int wizard_color;
  int required_count;
  int i;

  slot_index = 7;
  g_siege_indicator = 0;

  if (g_lair_or_monster_slots[slot_index].entry_type == SHANDALAR_ENTRY_NONE)
  {
    return;
  }
  ClearInputAndWaitForMouseRelease();
  wizard_color = g_lair_or_monster_slots[slot_index].color;
  town_index = FindNearestTownIndex(g_lair_or_monster_slots[slot_index].world_x / 0x20,
                                    g_lair_or_monster_slots[slot_index].world_y / 0x20);

  g_town_slots[town_index].status_and_ruling_wizard &= -65281;
  g_town_slots[town_index].status_and_ruling_wizard |= wizard_color << 8;

  ruled_count = 0;
  for (i = 0; i < 0x80; i = i + 1)
  {
    if ((int)g_town_slots[i].status_and_ruling_wizard >> 8 == wizard_color)
    {
      ruled_count = ruled_count + 1;
    }
  }

  if (Scards[WORLDMAGIC_RING_OF_THE_GUARDIAN].worldmagic_city == 0)
  {
    required_count = 5;
  }
  else
  {
    required_count = 3;
  }

  PlaySoundEffectOnChannel("x:sound\\newsflash.wav", 0x97, 100, 100, 0);
  AnimateVisitBackdropZoomIn("newsback.pic");

  strcpy(g_ui_message_buffer, gs_newsflash_0077d140[0]);
  if (IsWizardColorFeminine(wizard_color) != 0)
  {
    FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer + strlen(g_ui_message_buffer), 0x1000, gs_newsflash_0077d140[4],
                                                gs_wizardnames_0077ee70[wizard_color], BuildTownDisplayName(town_index));
  }
  else
  {
    FormatMessageFromStringStripCarriageReturns(g_ui_message_buffer + strlen(g_ui_message_buffer), 0x1000, gs_newsflash_0077d140[5],
                                                gs_wizardnames_0077ee70[wizard_color], BuildTownDisplayName(town_index));
  }

  if (required_count - ruled_count != 0)
  {
    if (IsWizardColorFeminine(wizard_color) != 0)
    {
      sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_newsflash_0077d140[6], required_count - ruled_count);
    }
    else
    {
      sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_newsflash_0077d140[7], required_count - ruled_count);
    }
  }
  else
  {
    if (IsWizardColorFeminine(wizard_color) != 0)
    {
      strcat(g_ui_message_buffer, gs_newsflash_0077d140[8]);
    }
    else
    {
      strcat(g_ui_message_buffer, gs_newsflash_0077d140[9]);
    }
  }

  if ((g_town_slots[town_index].status_and_ruling_wizard & 1) != 0)
  {
    g_town_slots[town_index].status_and_ruling_wizard &= -2;
    strcat(g_ui_message_buffer, gs_newsflash_0077d140[10]);
  }

  g_wizard_siege_count = g_wizard_siege_count + 1;
  g_page0_window_bounds->font_slot = 5;
  DrawTextAt(g_page0_window_bounds, 0xbe, 0x140, 0xf7, g_ui_message_buffer);
  g_page0_window_bounds->font_slot = 1;
  ClearInputAndWaitForMouseRelease();
  WaitForInputEventUnlessBlocked();

  FreeOpeningMenuSpriteWorkEntries(slot_index, slot_index + 8);
  g_lair_or_monster_slots[slot_index].entry_type = SHANDALAR_ENTRY_NONE;
  ShowStatsWindow(3, wizard_color);
  RefreshAdventureInterfaceLayout();
  g_siege_indicator = 0;

  if (ruled_count >= required_count)
  {
    AnimatePaletteToColor(0, g_default_palette_fade_steps);
    LoadPcxIntoPageOpaque(1, "uth-arz.pic");
    if ((global_screen_width == 0x280) || (global_screen_width == 800))
    {
      font_size = 0x10;
    }
    else
    {
      font_size = 0xc;
    }
    g_page1_window_bounds->font_slot = 5;
    SetFontStyleSize(5, (unsigned int)ScaleUiCoordinate(font_size));

    strcpy(g_ui_message_buffer, gs_questfailed_0077c580[1]);
    if (IsWizardColorFeminine(wizard_color) != 0)
    {
      sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_questfailed_0077c580[2], gs_wizardnames_0077ee70[wizard_color]);
    }
    else
    {
      sprintf(g_ui_message_buffer + strlen(g_ui_message_buffer), gs_questfailed_0077c580[3], gs_wizardnames_0077ee70[wizard_color]);
    }
    strcat(g_ui_message_buffer, gs_questfailed_0077c580[5]);

    DrawFormattedTextShadowedCentered(g_page1_window_bounds, 0xea, 0x140, 0x1e0 - GetFontLineHeight(g_page1_window_bounds->font_slot) * 7, g_ui_message_buffer);
    StretchBlitGraphicsRect(g_page1_window_bounds, 0, 0, 0x280, 0x1e0, g_page0_window_bounds, 0, 0, global_screen_width, global_screen_height);
    ClearInputAndWaitForMouseRelease();
    WaitForInputEventUnlessBlocked();
    AnimatePaletteToColor(0, g_default_palette_fade_steps);
    g_adventure_world_exit_requested = 1;
    UnloadStatWinDllExports();
    ShutdownSharedStartup();
    exit(0);
  }
}
