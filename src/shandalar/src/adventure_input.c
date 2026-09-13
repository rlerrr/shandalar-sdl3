#include "platform/winuser_compat.h"
#include "platform/cheat_options.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
#include <errno.h>
#include <math.h>

#include "platform/legacy_io.h"
#include "defs.h"
#include "shandalar.h"
#include "shandalar_internal.h"
#include "magic/src/global_duel_ui_ids.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "magic/src/global_other.h"
#include "magic/src/game_support.h"
#include "magic/src/duel_engine.h"
#include "magic/src/shared_startup.h"
#include "shandalar_global_strings.h"
#include "cardartlib/src/assert.h"
#include "cardartlib/src/palette.h"
#include "deckdll/src/card_db.h"
#include "deckdll/src/magsnd.h"
#include "deckdll/src/shared_resources.h"
#include "facemaker/src/facemaker_types.h"
#include "statwin/statwin.h"
#include <drawcardlib/src/pic.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// FUNCTION: SHANDALAR 0x0055e1b2
int RunRightClickMenuAndQueueInput(void)
{
  int menu_selection;
  int previous_font_slot;
  char *menu_text;

  previous_font_slot = g_page0_window_bounds->font_slot;
  LoadTextSectionLines("ADVstrings.txt", "STARTUP");
  menu_text = g_text_lines[1];
  g_page0_window_bounds->font_slot = 4;

  if (shandalar_cheat_enabled(SHANDALAR_CHEAT_ENABLE_DEBUG_MENU))
  {
    strcpy(g_ui_message_buffer, menu_text);
    strcat(g_ui_message_buffer, " DEBUG\n");
    menu_text = g_ui_message_buffer;
  }

  menu_selection = RunTextMenuAtWithCancel(menu_text, ScaleUiCoordinate(0x50), ScaleUiCoordinate(0x40));
  switch (menu_selection + 1)
  {
  case 1:
    PushQueuedKeyInput(0x53);
    break;
  case 2:
    PushQueuedKeyInput(0x4c);
    break;
  case 3:
    PushQueuedKeyInput(0x51);
    break;
  case 4:
    PushQueuedKeyInput(0x3b00);
    break;
  case 5:
    PushQueuedKeyInput(0x3c00);
    break;
  case 6:
    PushQueuedKeyInput(0x3d00);
    break;
  case 7:
    PushQueuedKeyInput(0x3e00);
    break;
  case 8:
    PushQueuedKeyInput(0x3f00);
    break;
  case 9:
    PushQueuedKeyInput(0x4000);
    break;
  case 10:
    RunDebugRightClickMenu();
    RefreshAdventureInterfaceLayout();
    break;
  default:
    RefreshAdventureInterfaceLayout();
    break;
  }

  g_page0_window_bounds->font_slot = previous_font_slot;
  return menu_selection + 1;
}

// FUNCTION: SHANDALAR 0x0055e651
int QueuePendingMenuActionInput(void)
{
  int unused_mapped_key;

  unused_mapped_key = -1;
  switch (g_pending_ui_action_code)
  {
  case 1:
    PushQueuedKeyInput(0x3b00);
    break;
  case 2:
    PushQueuedKeyInput(0x3c00);
    break;
  case 3:
    PushQueuedKeyInput(0x3d00);
    break;
  case 4:
    PushQueuedKeyInput(0x3e00);
    break;
  case 5:
    PushQueuedKeyInput(0x3f00);
    break;
  case 6:
    PushQueuedKeyInput(0x4000);
    break;
  case 0x31:
    PushQueuedKeyInput(0x31);
    break;
  case 0x32:
    PushQueuedKeyInput(0x32);
    break;
  case 0x33:
    PushQueuedKeyInput(0x33);
    break;
  case 0x34:
    PushQueuedKeyInput(0x34);
    break;
  case 0x35:
    PushQueuedKeyInput(0x35);
    break;
  default:
    UpdateMouseSnapshotImpl();
    if ((g_mouse_button_mask_snapshot & 2U) != 0)
    {
      RunRightClickMenuAndQueueInput();
      g_pending_ui_action_code = 0;
      return 1;
    }
    if ((g_mouse_button_mask_snapshot & 1U) != 0)
    {
      if (IsAdventureRightClickMenuButtonHit(g_mouse_x_snapshot, g_mouse_y_snapshot) != 0)
      {
        RunRightClickMenuAndQueueInput();
        g_pending_ui_action_code = 0;
        return 1;
      }
      unused_mapped_key = MapWorldClickToMovementKey(g_mouse_x_snapshot, g_mouse_y_snapshot);
    }
    if (unused_mapped_key != -1)
    {
      PushQueuedKeyInput(unused_mapped_key);
    }
    g_pending_ui_action_code = 0;
    return 1;
  }

  g_pending_ui_action_code = 0;
  return 0;
}

// FUNCTION: SHANDALAR 0x0055e31f
int MapWorldClickToMovementKey(int x, int y)
{
  int center_y;
  int center_x;
  int abs_y;
  int abs_x;
  unsigned int direction;
  int keycode_map[10];

  keycode_map[0] = 0x4800;
  keycode_map[1] = 0x4900;
  keycode_map[2] = 0x4d00;
  keycode_map[3] = 0x5100;
  keycode_map[4] = 0x5000;
  keycode_map[5] = 0x4f00;
  keycode_map[6] = 0x4b00;
  keycode_map[7] = 0x4700;
  keycode_map[8] = 0x4800;

  if ((x < ScaleUiCoordinate(0x40) || ScaleUiCoordinate(0x240) < x) || (y < ScaleUiCoordinate(0x30) || ScaleUiCoordinate(0x148) < y))
  {
    return -1;
  }

  if ((ScaleUiCoordinate(0x130) < x && x < ScaleUiCoordinate(0x158)) &&
      (ScaleUiCoordinate(0xa8) < y && y < ScaleUiCoordinate(0xdd)))
  {
    return 0x20;
  }

  center_x = ScaleUiCoordinate(0x140);
  center_y = ScaleUiCoordinate(0xbc);
  x = x - center_x;
  y = center_y - y;
  abs_x = abs(x);
  abs_y = abs(y);

  if (x >= 0 && y >= 0)
  {
    direction = 0;
  }
  else if (x >= 0 && y < 0)
  {
    direction = 2;
  }
  else if (x < 0 && y < 0)
  {
    direction = 4;
  }
  else if (x < 0 && y >= 0)
  {
    direction = 6;
  }

  if ((direction & 2) == 0 && abs_y < abs_x)
  {
    direction++;
  }
  else if ((direction & 2) != 0 && abs_x < abs_y)
  {
    direction++;
  }

  switch (direction)
  {
  case 0:
  case 3:
  case 4:
  case 7:
    keycode_map[9] = (abs_x * 0x9a85) >> 0xe;
    if (y < 0)
    {
      keycode_map[9] = -keycode_map[9];
    }
    break;
  case 1:
  case 2:
  case 5:
  case 6:
    keycode_map[9] = (abs_x * 0x1a82) >> 0xe;
    if (y < 0)
    {
      keycode_map[9] = -keycode_map[9];
    }
    break;
  }

  switch (direction)
  {
  case 0:
  case 1:
  case 2:
  case 3:
    if (y < keycode_map[9])
    {
      direction = direction + 1;
    }
    break;
  case 4:
  case 5:
  case 6:
  case 7:
    if (keycode_map[9] < y)
    {
      direction = direction + 1;
    }
    break;
  }

  return keycode_map[direction];
}

int QuantizeAdventureWorldCoordinate(double coordinate)
{
  return (int)floor(coordinate + 0.5);
}

int AdventureWorldTileFromCoordinate(double coordinate)
{
  return (int)floor(coordinate / 32.0);
}

static int GetAdventureMovementTickInterval(int tile_type)
{
  int movement_tick_interval;

  movement_tick_interval = 1;
  if ((tile_type == 2) || ((tile_type == 3 && ((g_world_magic_bitmap & 8U) == 0)) ||
                           (tile_type == 4 && ((g_world_magic_bitmap & 0x200U) == 0))) ||
      ((tile_type == 5) && ((g_world_magic_bitmap & 0x200U) == 0)))
  {
    movement_tick_interval = 3;
  }
  if ((WorldRoadTileHasDirection(g_world_player_tile_x, g_world_player_tile_y, g_world_move_dir_index) != 0) ||
      (WorldRoadTileHasDirection(g_world_player_tile_x, g_world_player_tile_y,
                                 (g_world_move_dir_index + 3U & 7) + 1) != 0))
  {
    movement_tick_interval = 1;
  }
  if (g_food == 0)
  {
    movement_tick_interval = ClampIntToRange(movement_tick_interval + 2, 0, 4);
  }
  return movement_tick_interval;
}

void AdvanceAdventureWorldPlayer(double elapsed_seconds)
{
  double movement_step;
  double movement_speed;
  double previous_world_x;
  double previous_world_y;
  int previous_tile_x;
  int previous_tile_y;
  int tile_type;
  int movement_tick_interval;
  int road_acceleration;

  if ((elapsed_seconds <= 0.0) || (g_world_move_dir_index == 0))
  {
    return;
  }
  if (elapsed_seconds > 0.1)
  {
    elapsed_seconds = 0.1;
  }

  previous_world_x = g_world_player_x;
  previous_world_y = g_world_player_y;
  previous_tile_x = AdventureWorldTileFromCoordinate(previous_world_x);
  previous_tile_y = AdventureWorldTileFromCoordinate(previous_world_y);
  g_world_player_tile_x = previous_tile_x;
  g_world_player_tile_y = previous_tile_y;

  tile_type = GetWorldTileType(AdventureWorldTileFromCoordinate(
                                  g_world_player_x + g_neighbor_dx[g_world_move_dir_index]),
                              AdventureWorldTileFromCoordinate(
                                  g_world_player_y + g_neighbor_dy[g_world_move_dir_index]));
  movement_tick_interval = GetAdventureMovementTickInterval(tile_type);
  movement_step = movement_tick_interval == 3 ? 2.0 : 1.0;
  road_acceleration = WorldRoadTileHasDirection(g_world_player_tile_x, g_world_player_tile_y,
                                                (g_world_move_dir_index + 3U & 7) + 1);
  if (Scards[WORLDMAGIC_QUICKENING].worldmagic_duration != 0)
  {
    movement_step += 1.0;
  }
  else if (road_acceleration != 0)
  {
    // The original added one unit every other accepted road step.
    movement_step += 0.5;
  }
  movement_speed = movement_step / ((double)movement_tick_interval * 0.060);

  g_world_player_x += g_neighbor_dx[g_world_move_dir_index] * movement_speed * elapsed_seconds;
  g_world_player_y += g_neighbor_dy[g_world_move_dir_index] * movement_speed * elapsed_seconds;

  tile_type = GetWorldTileType(AdventureWorldTileFromCoordinate(g_world_player_x),
                               AdventureWorldTileFromCoordinate(g_world_player_y));
  if ((tile_type == 0) &&
      ((fabs(g_world_player_x - (floor(g_world_player_x / 32.0) * 32.0 + 16.0)) < 12.0) ||
       (fabs(g_world_player_y - (floor(g_world_player_y / 32.0) * 32.0 + 16.0)) < 12.0)))
  {
    g_world_move_dir_index = 0;
    g_world_player_x = previous_world_x;
    g_world_player_y = previous_world_y;
  }

  g_world_player_tile_x = AdventureWorldTileFromCoordinate(g_world_player_x);
  g_world_player_tile_y = AdventureWorldTileFromCoordinate(g_world_player_y);
  if ((previous_tile_x != g_world_player_tile_x) || (previous_tile_y != g_world_player_tile_y))
  {
    g_world_location_entry_latched = 0;
  }
}

// FUNCTION: SHANDALAR 0x0055e808
void UpdateAdventureWorldInputAndLogic(void)
{
  int move_offset_y;             // ebp - 0x70
  int move_offset_x;             // ebp - 0x6c
  int nearest_town_distance;     // ebp - 0x68
  int nearest_town_index;        // ebp - 0x64
  int town_index;                // ebp - 0x60
  int castle_index;              // ebp - 0x5c
  int ambient_track_id;          // ebp - 0x58
  int previous_world_x_adjusted; // ebp - 0x54
  int previous_world_y_adjusted; // ebp - 0x50
  int delta_y;                   // ebp - 0x4c
  int delta_x;                   // ebp - 0x48
  int move_step_divisor;         // ebp - 0x44
  unsigned int tile_magic_mask;  // ebp - 0x40
  int abs_delta_y;               // ebp - 0x3c
  int key_code;                  // ebp - 0x30
  int key_magic_index;           // ebp - 0x2c
  int slot_index;                // ebp - 0x28
  int abs_delta_x;               // ebp - 0x24
  int random_world_y;            // ebp - 0x20
  int random_world_x;            // ebp - 0x1c
  int random_value;              // ebp - 0x18
  unsigned int tile_type;        // ebp - 0x14
  int nearest_slot_distance;     // ebp - 0x10
  int nearest_slot_index;        // ebp - 0xc
  int movement_tick_interval;    // ebp - 0x8
  int dungeon_index;             // ebp - 0x4
  int player_world_x;
  int player_world_y;

  if (IsKeyInputQueueEmpty() == 0)
  {
    key_code = PopNormalizedQueuedKeyInput();
    g_adventure_demo_idle_ticks = 0;

    switch (key_code)
    {
    case 0x1b:
    case 0x51:
    case 0x71:
      g_page0_window_bounds->font_slot = 4;
      LoadTextSectionLines("ADVstrings.txt", "SHUTDOWN");
      strcpy(g_ui_message_buffer, g_text_lines[0]);
      if (RunTextMenuAtScaled(g_ui_message_buffer, 100, 0x50) == 1)
      {
        g_adventure_world_exit_requested = 1;
      }
      else
      {
        RefreshAdventureInterfaceLayout();
      }
      SaveGameToSlot(3);
      break;

    case 0x4800:
      g_world_move_dir_index = 2;
      break;
    case 0x4900:
      g_world_move_dir_index = 3;
      break;
    case 0x4d00:
      g_world_move_dir_index = 4;
      break;
    case 0x5100:
      g_world_move_dir_index = 5;
      break;
    case 0x5000:
      g_world_move_dir_index = 6;
      break;
    case 0x4f00:
      g_world_move_dir_index = 7;
      break;
    case 0x4b00:
      g_world_move_dir_index = 8;
      break;
    case 0x4700:
      g_world_move_dir_index = 1;
      break;
    case 0x20:
      g_world_move_dir_index = 0;
      break;

    case 0x3b00:
      if ((g_world_magic_bitmap & (1U << WORLDMAGIC_CONJURERS_WILL)) != 0)
      {
        key_code = 0x31;
        goto handle_world_magic_hotkey;
      }

      ClearInputAndWaitForMouseRelease();
      AnimatePaletteToColor(0, g_default_palette_fade_steps);
      DeckBuilderMain(g_main_window_hwnd, 1, 0);
      RestoreAdventureUiPaletteAndFocus();
      RefreshAdventureInterfaceLayout();

      break;
    case 0x3c00:
      ClearInputAndWaitForMouseRelease();
      ShowWorldMapScreen(0);
      RefreshAdventureInterfaceLayout();
      break;
    case 0x3d00:
      ClearInputAndWaitForMouseRelease();
      ShowCityInfoScreen(1);
      RefreshAdventureInterfaceLayout();
      break;
    case 0x3e00:
      ClearInputAndWaitForMouseRelease();
      ShowDungeonCluesScreen(1);
      RefreshAdventureInterfaceLayout();
      break;
    case 0x3f00:
      RunAdventureStatsMenu();
      RefreshAdventureInterfaceLayout();
      break;
    case 0x4000:
      ClearInputAndWaitForMouseRelease();
      ShowStatsWindow(0, -1);
      RefreshAdventureInterfaceLayout();
      break;
    case 0x4c:
    case 0x6c:
      g_loadsave_skip_esc = 1;
      previous_world_y_adjusted = RunLoadGameMenu();
      if (previous_world_y_adjusted != -1)
      {
        load_selected_duel_save_slot(previous_world_y_adjusted);
      }
      LoadPcxIntoPageNoPalette("advfac64.pic");
      g_world_move_dir_index = 0;
      RefreshAdventureInterfaceLayout();
      DrawAdventureInterfaceLayout(1);
      g_loadsave_skip_esc = 0;
      break;
    case 0x53:
    case 0x73:
      g_world_move_dir_index = 0;
      previous_world_x_adjusted = RunSaveMenuAndSelectSlot();
      if (previous_world_x_adjusted != -1)
      {
        SaveGameToSlot(previous_world_x_adjusted);
      }
      LoadPcxIntoPageNoPalette("advfac64.pic");
      RefreshAdventureInterfaceLayout();
      break;
    case 0x55:
      g_frontbuffer_direct_blit_enabled = g_frontbuffer_direct_blit_enabled ^ 1;
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:

    handle_world_magic_hotkey:
      key_magic_index = key_code - 0x30;
      if ((g_amulet_inventory[key_magic_index - 1] != 0) && ((g_world_magic_bitmap & (1 << (key_magic_index * 2))) != 0))
      {
        if (internal_rand(4 - g_shandalar_difficulty) == 0)
        {
          g_amulet_inventory[key_magic_index - 1] = g_amulet_inventory[key_magic_index - 1] - 1;
        }
        RefreshAdventureInterfaceLayout();

        switch (key_magic_index)
        {
        case 1:
          AnimatePaletteToColor(0, g_default_palette_fade_steps);
          DeckBuilderMain(g_main_window_hwnd, 1, 1);
          RestoreAdventureUiPaletteAndFocus();
          ClearGraphicsPageWithPaletteColor(0, 7);
          RefreshAdventureInterfaceLayout();
          break;
        case 2:
          do
          {
            random_world_y = internal_rand(0x40);
            abs_delta_x = internal_rand(0x40);
          } while (GetWorldTileType(random_world_y, abs_delta_x) == 0);
          AddJournalEntry(JOURNAL_ENTRY_WORLD_MAGIC_EVENT, JOURNAL_WORLD_MAGIC_EVENT_TELEPORT_RANDOM);
          g_world_player_x = random_world_y * 0x20 + 0x10;
          g_world_player_y = abs_delta_x * 0x20 + 0x10;
          RefreshAdventureInterfaceLayout();
          g_world_scene_reveal_effect_pending = 1;
          break;
        case 3:
          Scards[key_magic_index * 2].worldmagic_duration = 0x96;
          AddJournalEntry(JOURNAL_ENTRY_WORLD_MAGIC_EVENT, JOURNAL_WORLD_MAGIC_EVENT_DELAY_MONSTERS);
          break;
        case 4:
          tile_magic_mask = 0x7fff;
          nearest_slot_index = -1;
          for (slot_index = 0; slot_index < 6; slot_index = slot_index + 1)
          {
            if (SHANDALAR_ENTRY_LAIR < g_lair_or_monster_slots[slot_index].entry_type)
            {
              nearest_slot_distance = ApproximateDistance(
                  QuantizeAdventureWorldCoordinate(g_world_player_x) - g_lair_or_monster_slots[slot_index].world_x,
                  QuantizeAdventureWorldCoordinate(g_world_player_y) - g_lair_or_monster_slots[slot_index].world_y);
              if ((int)nearest_slot_distance < (int)tile_magic_mask)
              {
                tile_magic_mask = nearest_slot_distance;
                nearest_slot_index = slot_index;
              }
            }
          }
          if (nearest_slot_index != -1)
          {
            FreeOpeningMenuSpriteWorkEntries(nearest_slot_index, nearest_slot_index + 8);
            AddJournalEntry(JOURNAL_ENTRY_WORLD_MAGIC_EVENT,
                            (g_lair_or_monster_slots[nearest_slot_index].entry_type << 0x10) |
                                JOURNAL_WORLD_MAGIC_EVENT_BANISH_MONSTER);
            g_lair_or_monster_slots[nearest_slot_index].entry_type = SHANDALAR_ENTRY_NONE;
          }
          break;
        case 5:
          if (g_lair_or_monster_slots[7].entry_type != SHANDALAR_ENTRY_NONE)
          {
            g_world_player_x = (g_lair_or_monster_slots[7].world_x & 0xffe0U) + 0x10;
            g_world_player_y = (g_lair_or_monster_slots[7].world_y & 0xffe0U) + 0x1f;
            AddJournalEntry(JOURNAL_ENTRY_WORLD_MAGIC_EVENT, JOURNAL_WORLD_MAGIC_EVENT_TELEPORT_TO_CASTLE);
          }
          g_world_scene_reveal_effect_pending = 1;
          break;
        }
      }

    default:
      break;
    }

    EnsureAdvfac64Loaded(0);
  }

  if ((g_adventure_demo_enabled != 0) && (++g_adventure_demo_idle_ticks > 500))
  {
    // Some sort of demo? Looks unreachable since g_adventure_demo_enabled is never set
    RunRandomAiDuelDemo();
    g_adventure_demo_idle_ticks = 300;
  }

  player_world_x = QuantizeAdventureWorldCoordinate(g_world_player_x);
  player_world_y = QuantizeAdventureWorldCoordinate(g_world_player_y);

  random_value = QuantizeAdventureWorldCoordinate(g_neighbor_dx[g_world_move_dir_index] + g_world_player_x);
  random_world_x = QuantizeAdventureWorldCoordinate(g_neighbor_dy[g_world_move_dir_index] + g_world_player_y);
  tile_type = GetWorldTileType(random_value / 32, random_world_x / 32);
  key_magic_index = GetWorldTileMagicMask(tile_type);
  if (key_magic_index != 0)
  {
    do
    {
      abs_delta_y = internal_rand(5) + 1;
    } while ((key_magic_index & (1 << (unsigned char)abs_delta_y)) == 0);
  }
  else
  {
    abs_delta_y = 0;
  }

  g_world_player_tile_x = AdventureWorldTileFromCoordinate(g_world_player_x);
  g_world_player_tile_y = AdventureWorldTileFromCoordinate(g_world_player_y);
  movement_tick_interval = GetAdventureMovementTickInterval(tile_type);

  random_value = g_world_player_tile_x;
  random_world_x = g_world_player_tile_y;
  if (g_monster_timer % movement_tick_interval == 0)
  {
    g_world_player_animation_frame = g_world_player_animation_frame + 1;
    if (4 < g_world_player_animation_frame)
    {
      g_world_player_animation_frame = 1;
    }

    if (g_world_move_dir_index != 0)
    {
      PlaySoundWithPitchAndPan(((g_world_player_animation_frame & 1U) - 2) + abs_delta_y * 2, internal_rand(0x19) + 0x4b, internal_rand(0x28) + 0x50, 0);
    }

    if (g_world_move_dir_index == 0)
    {
      g_world_player_animation_frame = 0;
    }
    else
    {
      g_world_player_animation_direction = g_world_move_dir_index;
    }

    tile_type = GetWorldTileType(AdventureWorldTileFromCoordinate(g_world_player_x),
                                 AdventureWorldTileFromCoordinate(g_world_player_y));

    if ((internal_rand(0x28) == 0) && (g_skip_world_sfx_preload == 0))
    {
      UpdateAmbientWizardColorSound(abs_delta_y);
    }

    if (((g_monster_timer & 0x1fU) == 0) && (g_world_move_dir_index != 0))
    {
      if (g_food != 0 && !shandalar_cheat_enabled(SHANDALAR_CHEAT_UNLIMITED_FOOD))
      {
        g_food = g_food - 1;
      }
      if ((Scards[WORLDMAGIC_FRUIT_OF_SUSTENANCE].worldmagic_city == 0) && (tile_type == 2))
      {
        g_food = g_food + 2;
      }
      g_siege_timer = g_siege_timer + 1;
      g_quest_restock_timer = g_quest_restock_timer + 1;

      if ((g_siege_timer & 0x3fU) == 0)
      {
        StartWizardTownSiege();
        g_siege_timer = g_siege_timer + ClampIntToRange(g_shandalar_difficulty + g_siege_timer / 0x100, 0, 0x10);
      }
      if ((g_siege_timer & 0x3f) == 0x18)
      {
        ResolveWizardTownSiege();
        g_siege_timer = g_siege_timer + ClampIntToRange(g_shandalar_difficulty + g_siege_timer / 0x40 + g_shandalar_difficulty, 0, 0x20);
      }

      g_world_scene_force_redraw = 1;
      if (g_siege_timer >= 8)
      {
        g_world_location_entry_enabled = 1;
      }
    }

    g_world_player_tile_x = AdventureWorldTileFromCoordinate(g_world_player_x);
    g_world_player_tile_y = AdventureWorldTileFromCoordinate(g_world_player_y);

    if ((g_monster_timer & 1U) == 0)
    {
      nearest_town_distance = 0x7fff;
      for (nearest_town_index = 0; nearest_town_index < 0x80; nearest_town_index = nearest_town_index + 1)
      {
        if (g_town_slots[nearest_town_index].location_type != -1)
        {
          castle_index = ApproximateDistance(g_town_slots[nearest_town_index].world_x * 0x20 + 0x10 - player_world_x,
                                             g_town_slots[nearest_town_index].world_y * 0x20 + 0x10 - player_world_y);
          if (castle_index < nearest_town_distance)
          {
            nearest_town_distance = castle_index;
            ambient_track_id = nearest_town_index;
          }
        }
      }

      town_index = ClampIntToRange(0x80 - nearest_town_distance, 0, 100);
      if ((town_index > 0xa) && (g_town_slots[ambient_track_id].location_type >= 1))
      {
        if (ambient_track_id != g_world_location_music_town_index)
        {
          g_world_location_music_town_index = ambient_track_id;
          if (g_town_slots[ambient_track_id].location_type == 4)
          {
            for (key_magic_index = 0; key_magic_index < 5; key_magic_index = key_magic_index + 1)
            {
              if (g_town_slots[ambient_track_id].world_x == g_castle_dungeon_slots[key_magic_index].world_x)
              {
                if (g_castle_dungeon_slots[key_magic_index].world_y == g_town_slots[ambient_track_id].world_y)
                {
                  break;
                }
              }
            }

            if (key_magic_index + 0x15 != g_world_location_music_track_id)
            {
              if ((g_world_location_music_track_id != -1) && (key_magic_index + 0x15 != g_world_location_music_track_id))
              {
                sound_unload(0x10);
              }

              switch (key_magic_index + 1)
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
              }
              key_magic_index += +0x15;
              g_world_location_music_track_id = key_magic_index;
            }
          }
          else if (g_town_slots[ambient_track_id].location_type == 1)
          {
            if ((g_world_location_music_track_id != -1) && (g_world_location_music_track_id != 0x32))
            {
              sound_unload(0x10);
            }
            if (g_world_location_music_track_id != 0x32)
            {
              LoadLoopingSound("x:sound\\locmus0.wav", 0x10);
            }
            g_world_location_music_track_id = key_magic_index = 0x32;
          }
          else
          {
            key_magic_index = ambient_track_id % 0x14;
            if (key_magic_index != g_world_location_music_track_id)
            {
              if (g_world_location_music_track_id != -1)
              {
                sound_unload(0x10);
              }
              switch (key_magic_index)
              {
              case 0:
                LoadLoopingSound("x:sound\\locmus1.wav", 0x10);
                break;
              case 1:
                LoadLoopingSound("x:sound\\locmus2.wav", 0x10);
                break;
              case 2:
                LoadLoopingSound("x:sound\\locmus3.wav", 0x10);
                break;
              case 3:
                LoadLoopingSound("x:sound\\locmus4.wav", 0x10);
                break;
              case 4:
                LoadLoopingSound("x:sound\\locmus5.wav", 0x10);
                break;
              case 5:
                LoadLoopingSound("x:sound\\locmus6.wav", 0x10);
                break;
              case 6:
                LoadLoopingSound("x:sound\\locmus7.wav", 0x10);
                break;
              case 7:
                LoadLoopingSound("x:sound\\locmus8.wav", 0x10);
                break;
              case 8:
                LoadLoopingSound("x:sound\\locmus9.wav", 0x10);
                break;
              case 9:
                LoadLoopingSound("x:sound\\locmus10.wav", 0x10);
                break;
              case 10:
                LoadLoopingSound("x:sound\\locmus11.wav", 0x10);
                break;
              case 0xb:
                LoadLoopingSound("x:sound\\locmus12.wav", 0x10);
                break;
              case 0xc:
                LoadLoopingSound("x:sound\\locmus13.wav", 0x10);
                break;
              case 0xd:
                LoadLoopingSound("x:sound\\locmus14.wav", 0x10);
                break;
              case 0xe:
                LoadLoopingSound("x:sound\\locmus15.wav", 0x10);
                break;
              case 0xf:
                LoadLoopingSound("x:sound\\locmus16.wav", 0x10);
                break;
              case 0x10:
                LoadLoopingSound("x:sound\\locmus17.wav", 0x10);
                break;
              case 0x11:
                LoadLoopingSound("x:sound\\locmus18.wav", 0x10);
                break;
              case 0x12:
                LoadLoopingSound("x:sound\\locmus19.wav", 0x10);
                break;
              case 0x13:
                LoadLoopingSound("x:sound\\tmplmus1.wav", 0x10);
                break;
              default:
                LoadLoopingSound("x:sound\\locmus0.wav", 0x10);
                break;
              }
            }
            g_world_location_music_track_id = key_magic_index;
          }
          PlayLoopingSoundWithPan(0x10, town_index, 0);
          set_sound_loop(0x10, 1);
        }
        else
        {
          sound_set_vol(0x10, town_index << 2);
        }
        g_world_location_music_active = 1;
      }
      else
      {
        if (g_world_location_music_active != 0)
        {
          sound_stop(0x10);
        }
        g_world_location_music_active = 0;
        g_world_location_music_town_index = -1;
      }
    }

    if ((g_world_location_entry_latched == 0) &&
        (abs((QuantizeAdventureWorldCoordinate(g_world_player_x) & 0x1f) - 0x10) < 0xc) &&
        (abs((QuantizeAdventureWorldCoordinate(g_world_player_y) & 0x1f) - 0x10) < 0xc) &&
        ((GetWorldMapPixelFlags(g_world_player_tile_x, g_world_player_tile_y) & 0x10) != 0))
    {
      move_step_divisor = FindTownAtWorldCoordinates(g_world_player_tile_x, g_world_player_tile_y);
      if (move_step_divisor != -1)
      {
        sound_set_vol(0x10, 400);
        play_snd_marker(0x10, 1);
        VisitTownSlot(move_step_divisor);
        g_world_location_entry_enabled = 1;
        g_world_location_entry_latched = g_world_location_entry_enabled;
        g_world_move_dir_index = 0;
        for (slot_index = 0; slot_index < 6; slot_index = slot_index + 1)
        {
          if (SHANDALAR_ENTRY_LAIR < g_lair_or_monster_slots[slot_index].entry_type)
          {
            if (ApproximateDistance(player_world_x - g_lair_or_monster_slots[slot_index].world_x,
                                    player_world_y - g_lair_or_monster_slots[slot_index].world_y) < 0x60)
            {
              delta_x = player_world_x - g_lair_or_monster_slots[slot_index].world_x;
              delta_y = player_world_y - g_lair_or_monster_slots[slot_index].world_y;
              if (abs(delta_y) / 2 < abs(delta_x))
              {
                move_offset_x = SignNonZero(delta_x) * 0x30;
              }
              else
              {
                move_offset_x = 0;
              }
              g_lair_or_monster_slots[slot_index].world_x = player_world_x - move_offset_x;

              if (abs(delta_x) / 2 < abs(delta_y))
              {
                move_offset_y = SignNonZero(delta_y) * 0x30;
              }
              else
              {
                move_offset_y = 0;
              }
              g_lair_or_monster_slots[slot_index].world_y = player_world_y - move_offset_y;
            }
          }
        }
        AnalyzeDeckAndMaybeShowReport(0);
        SaveGameToSlot(3);
        EnsureAdvfac64Loaded(0);
        RefreshAdventureInterfaceLayout();
        g_monster_timer = g_monster_timer | 0x1f;
      }
      else
      {
        ClearWorldMapPixelFlags(0x10, g_world_player_tile_x, g_world_player_tile_y);
      }
    }

    if ((g_world_location_entry_latched == 0) &&
        (((QuantizeAdventureWorldCoordinate(g_world_player_x) - 8U) & 0x10) == 0) &&
        (((QuantizeAdventureWorldCoordinate(g_world_player_y) - 8U) & 0x10) == 0) &&
        ((GetWorldMapPixelFlags(g_world_player_tile_x, g_world_player_tile_y) & 0x40) != 0))
    {
      dungeon_index = FindCastleDungeonAtWorldCoordinates(g_world_player_tile_x, g_world_player_tile_y);
      g_world_location_entry_latched = 1;
      if ((dungeon_index != -1) && (g_castle_dungeon_slots[dungeon_index].card_slot_1 != -1) && (g_castle_dungeon_slots[dungeon_index].clues_bitmap != 0))
      {
        EnterCastleDungeon(dungeon_index);
      }
    }

    g_deck_total_card_count = 0;
    g_deck_active_card_count = g_deck_total_card_count;
    for (slot_index = 0; slot_index < 500; slot_index = slot_index + 1)
    {
      if (deck[slot_index] != -1)
      {
        g_deck_total_card_count = g_deck_total_card_count + 1;
        if ((deck[slot_index] & 0x4000) == 0)
        {
          g_deck_active_card_count = g_deck_active_card_count + 1;
        }
      }
    }

    ClearQueuedKeyInput();
  }
}

// FUNCTION: SHANDALAR 0x0041d1ef
int IsKeyInputQueueEmpty(void)
{
  if (g_key_input_queue_count == 0)
  {
    return -1;
  }
  else
  {
    return 0;
  }
}

// FUNCTION: SHANDALAR 0x004ecefd
int SignNonZero(int value)
{
  if (0 < value)
  {
    return 1;
  }
  if (value < 0)
  {
    return -1;
  }
}

// FUNCTION: SHANDALAR 0x00561647
void TickWorldMagicSlotTimers(void)
{
  int slot_index;

  for (slot_index = 0; slot_index < 0xc; slot_index = slot_index + 1)
  {
    if (0 < Scards[slot_index].worldmagic_duration)
    {
      Scards[slot_index].worldmagic_duration = Scards[slot_index].worldmagic_duration - 1;
      if (Scards[slot_index].worldmagic_duration == 0)
      {
        RefreshAdventureInterfaceLayout();
      }
    }
  }
}

// FUNCTION: SHANDALAR 0x0046eca4
void UpdateMouseSnapshot(void)
{
#ifdef MODERN_FIXES
  shandalar_host_pump();
#endif
  UpdateMouseSnapshotImpl();
}

void UpdateMouseSnapshotImpl(void)
{
  if (g_legacy_mouse_active != 0)
  {
    g_mouse_button_mask_snapshot = ConsumeMouseButtonReleaseMask() | g_mouse_button_down_mask;
    g_mouse_x_snapshot = g_mouse_x;
    g_mouse_y_snapshot = g_mouse_y;
  }
  else
  {
    g_mouse_button_mask_snapshot = g_mouse_x_snapshot = g_mouse_y_snapshot = 0;
  }
}

// FUNCTION: SHANDALAR 0x0041d21d
unsigned int PeekQueuedKeyInput(void)
{
  if (g_key_input_queue_count == 0)
  {
    return 0;
  }
  else
  {
    return g_key_input_queue[0];
  }
}

// FUNCTION: SHANDALAR 0x0041d24b
void PushQueuedKeyInput(int key_code)
{
  memmove(g_key_input_queue + 1, g_key_input_queue, (size_t)g_key_input_queue_count * sizeof(g_key_input_queue[0]));
  g_key_input_queue_count = g_key_input_queue_count + 1;
  g_key_input_queue[0] = (unsigned int)key_code;
}

// FUNCTION: SHANDALAR 0x005003b7
int UpdateMenuControlSelection(int mouse_x, int mouse_y, int allow_activate_on_click)
{
  int direction;
  int control_index;
  int next_search_index;
  int queued_key;
  int i;
  int full_key;

  (void)mouse_x;
  (void)mouse_y;

  g_menu_input_unhandled = 1;
  if (HasQueuedKeyInput() != 0)
  {
    queued_key = PeekQueuedKeyInput();
    if ((queued_key == 0xf09) || (queued_key == 0xf0f) ||
        ((g_menu_allow_arrow_nav_by_context[g_menu_context_index] != 0) && ((queued_key == 0x4800) || (queued_key == 0x5000))))
    {
      PopQueuedKeyInput();
      switch (queued_key)
      {
      case 0xf09:
        direction = 1;
        break;
      case 0xf0f:
        direction = -1;
        break;
      case 0x4800:
        direction = -1;
        break;
      case 0x5000:
        direction = 1;
        break;
      }

      if (g_menu_current_control_index == -1)
      {
        if (direction > 0)
        {
          g_menu_current_control_index = 0;
        }
        else
        {
          g_menu_current_control_index = g_menu_control_count_by_context[g_menu_context_index] - 1;
        }
      }
      else
      {
        g_menu_current_control_index = (g_menu_control_count_by_context[g_menu_context_index] + g_menu_current_control_index + direction) % g_menu_control_count_by_context[g_menu_context_index];
      }

      while (g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index]->state == 3)
      {
        g_menu_current_control_index = (g_menu_control_count_by_context[g_menu_context_index] + g_menu_current_control_index + direction) % g_menu_control_count_by_context[g_menu_context_index];
      }

      if (g_menu_prev_control_index != -1)
      {
        g_menu_controls_by_context[g_menu_context_index][g_menu_prev_control_index]->on_render(g_menu_controls_by_context[g_menu_context_index][g_menu_prev_control_index], 0);
      }

      if (g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index]->x != -1)
      {
        SetCursorPos(g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index]->x + g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index]->width / 2,
                     g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index]->y + g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index]->height / 2);
      }
      g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index]->on_render(g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index], 1);
    }

    full_key = (int)queued_key;
    queued_key &= 0xff;
    if ((queued_key == 0) && (g_menu_allow_arrow_nav_by_context[g_menu_context_index] != 0))
    {
    }
    else
    {
      i = 0;
      if (g_menu_current_control_index == -1)
      {
        next_search_index = 0;
      }
      else
      {
        next_search_index = g_menu_current_control_index + 1;
      }

      for (; i < g_menu_control_count_by_context[g_menu_context_index]; i = i + 1)
      {
        control_index = (next_search_index + i) % g_menu_control_count_by_context[g_menu_context_index];
        if (g_menu_controls_by_context[g_menu_context_index][control_index]->state == 3)
        {
          continue;
        }
        if ((g_menu_controls_by_context[g_menu_context_index][control_index]->direct_hotkey == full_key) ||
            ((queued_key != 0) && (g_menu_controls_by_context[g_menu_context_index][control_index]->navigate_hotkeys != (char *)0) &&
             (strchr(g_menu_controls_by_context[g_menu_context_index][control_index]->navigate_hotkeys, queued_key) != (char *)0)))
        {
          g_menu_current_control_index = control_index;
          if (g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index]->x != -1)
          {
            SetCursorPos(
                g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index]->x + g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index]->width / 2,
                g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index]->y + g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index]->height / 2);
          }

          if (g_menu_prev_control_index != -1)
          {
            g_menu_controls_by_context[g_menu_context_index][g_menu_prev_control_index]->on_render(g_menu_controls_by_context[g_menu_context_index][g_menu_prev_control_index], 0);
          }

          g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index]->on_render(g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index], 1);
          if ((g_menu_controls_by_context[g_menu_context_index][control_index]->direct_hotkey == full_key) ||
              ((queued_key != 0) && (g_menu_controls_by_context[g_menu_context_index][control_index]->activate_hotkeys != (char *)0) &&
               (strchr(g_menu_controls_by_context[g_menu_context_index][control_index]->activate_hotkeys, queued_key) != (char *)0)))
          {
            g_menu_render_guard = 1;
            g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index]->on_render(g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index], 2);
            g_menu_render_guard = 0;
            g_menu_prev_control_index = g_menu_current_control_index;
            PopQueuedKeyInput();
            g_menu_input_unhandled = 0;
            return g_menu_current_control_index;
          }
          PopQueuedKeyInput();
          break;
        }
      }

      if ((queued_key == 0xd) && (g_menu_current_control_index != -1))
      {
        if (g_menu_prev_control_index != -1)
        {
          g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index]->on_render(g_menu_controls_by_context[g_menu_context_index][g_menu_prev_control_index], 0);
        }
        g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index]->on_render(g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index], 2);
        g_menu_prev_control_index = g_menu_current_control_index;
        PopQueuedKeyInput();
        g_menu_input_unhandled = 0;
        return g_menu_current_control_index;
      }

      g_menu_prev_control_index = g_menu_current_control_index;
    }
  }

  PopQueuedKeyInput();
  if (g_menu_prev_control_index >= 0)
  {
    int in_bounds;

    if ((g_mouse_x < g_menu_controls_by_context[g_menu_context_index][g_menu_prev_control_index]->x) ||
        (g_menu_controls_by_context[g_menu_context_index][g_menu_prev_control_index]->x + g_menu_controls_by_context[g_menu_context_index][g_menu_prev_control_index]->width < g_mouse_x))
    {
      in_bounds = 0;
    }
    else if ((g_mouse_y < g_menu_controls_by_context[g_menu_context_index][g_menu_prev_control_index]->y) ||
             (g_menu_controls_by_context[g_menu_context_index][g_menu_prev_control_index]->height + g_menu_controls_by_context[g_menu_context_index][g_menu_prev_control_index]->y < g_mouse_y))
    {
      in_bounds = 0;
    }
    else
    {
      in_bounds = 1;
    }

    if (in_bounds == 0)
    {
      g_menu_render_guard = 1;
      g_menu_controls_by_context[g_menu_context_index][g_menu_prev_control_index]->on_render(g_menu_controls_by_context[g_menu_context_index][g_menu_prev_control_index], 0);
      g_menu_current_control_index = g_menu_prev_control_index = -1;
      g_menu_render_guard = 0;
    }
  }

  for (i = 0; i < g_menu_control_count_by_context[g_menu_context_index]; i = i + 1)
  {
    if (i != g_menu_prev_control_index)
    {
      if (g_menu_controls_by_context[g_menu_context_index][i]->on_render(g_menu_controls_by_context[g_menu_context_index][i], 0) != 0)
      {
        g_menu_current_control_index = i;
      }
    }
  }

  if (g_menu_current_control_index != g_menu_prev_control_index)
  {
    if (g_menu_prev_control_index >= 0)
    {
      g_menu_render_guard = 1;
      g_menu_controls_by_context[g_menu_context_index][g_menu_prev_control_index]->on_render(g_menu_controls_by_context[g_menu_context_index][g_menu_prev_control_index], 0);
      g_menu_render_guard = 0;
    }

    g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index]->on_render(g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index], 1);
  }

  if ((g_mouse_button_mask_snapshot & 1) && !(g_mouse_button_down_mask & 1) && (g_menu_current_control_index >= 0))
  {
    g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index]->on_render(g_menu_controls_by_context[g_menu_context_index][g_menu_current_control_index], 2);
  }

  g_menu_prev_control_index = g_menu_current_control_index;
  g_menu_input_unhandled = 0;
  return g_menu_current_control_index;
}

// FUNCTION: SHANDALAR 0x004ed005
unsigned int WaitForInputEvent(void)
{
  int has_queued_key;

  if (g_duel_ai_mode_state != 0)
    return 0;

  do
  {
#ifdef MODERN_FIXES
    PumpOpeningMenuWait();
#else
    UpdateMouseSnapshot();
#endif
  } while (g_mouse_button_mask_snapshot == 0 && !HasQueuedKeyInput());

  has_queued_key = g_mouse_button_mask_snapshot;
  if (has_queued_key == 0)
  {
    has_queued_key = PopNormalizedQueuedKeyInput();
  }

  ClearInputAndWaitForMouseRelease();
  return has_queued_key;
}

// FUNCTION: SHANDALAR 0x005597ca
unsigned int WaitForInputEventUnlessBlocked(void)
{
  if (g_duel_ai_mode_state == 1)
  {
    return 0;
  }

  return (g_duel_active != 0) ? 0 : WaitForInputEvent();
}

// FUNCTION: SHANDALAR 0x004ecf30
int ApproximateDistance(int delta_x, int delta_y)
{
  int weighted_distance;

  if (delta_x < 0)
  {
    delta_x = -delta_x;
  }
  if (delta_y < 0)
  {
    delta_y = -delta_y;
  }
  if (delta_y < delta_x)
  {
    weighted_distance = delta_x * 2 + delta_y;
  }
  else
  {
    weighted_distance = delta_y * 2 + delta_x;
  }
  if (weighted_distance < 0)
  {
    weighted_distance = 0x7ffe;
  }
  return weighted_distance;
}

// FUNCTION: SHANDALAR 0x004ecfa2
void ClearInputAndWaitForMouseRelease(void)
{
  if (g_legacy_mouse_active != 0)
  {
    do
    {
#ifdef MODERN_FIXES
      PumpOpeningMenuWait();
#else
      UpdateMouseSnapshot();
#endif
    } while (g_mouse_button_mask_snapshot != 0);
  }

  while (HasQueuedKeyInput() != 0)
  {
    PopNormalizedQueuedKeyInput();
  }
}

// FUNCTION: SHANDALAR 0x004ecfe3
void ClearQueuedKeyInput(void)
{
  while (HasQueuedKeyInput() != 0)
  {
    PopNormalizedQueuedKeyInput();
  }
}

// FUNCTION: SHANDALAR 0x00414127
int PopNormalizedQueuedKeyInput(void)
{
  int queued_key;

loop:
  queued_key = PopQueuedKeyInput();
  if (queued_key == 0)
  {
#ifdef MODERN_FIXES
    PumpOpeningMenuWait();
#endif
    goto loop;
  }

  switch (queued_key)
  {
  case 0x487e:
  case 0x4800:
    queued_key = 0x4800;
    break;
  case 0x4900:
    queued_key = 0x4900;
    break;
  case 0xf400:
  case 0x4d00:
    queued_key = 0x4d00;
    break;
  case 0x5100:
    queued_key = 0x5100;
    break;
  case 0x5060:
  case 0x5000:
    queued_key = 0x5000;
    break;
  case 0x4f00:
    queued_key = 0x4f00;
    break;
  case 0x4b7c:
  case 0x4b00:
    queued_key = 0x4b00;
    break;
  case 0x475c:
  case 0x4700:
    queued_key = 0x4700;
    break;
  case 0x4838:
    queued_key = 0x4838;
    break;
  case 0x4939:
    queued_key = 0x4939;
    break;
  case 0x4d46:
  case 0x4d36:
    queued_key = 0x4d36;
    break;
  case 0x5133:
    queued_key = 0x5133;
    break;
  case 0x5032:
    queued_key = 0x5032;
    break;
  case 0x4f31:
    queued_key = 0x4f31;
    break;
  case 0x4b43:
  case 0x4b34:
    queued_key = 0x4b34;
    break;
  case 0x4737:
    queued_key = 0x4737;
    break;
  default:
    if (queued_key & 0xff)
    {
      queued_key = queued_key & 0xff;
    }
    break;
  }

  return (int)queued_key;
}
