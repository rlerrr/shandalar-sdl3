#include "platform/winuser_compat.h"
#include "platform/cheat_options.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
#include <errno.h>

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

// FUNCTION: SHANDALAR 0x004f6d90
void GenerateAdventureWorldMap(void)
{
  int icon_y_scaled;
  int neighbor_is_open;           // ebp - 0x20
  int uVar1;                      // ebp - 0x1c
  int icon_width_scaled;          // ebp - 0x18
  int i;                          // ebp - 0x14
  int location_block_start_index; // ebp - 0x10
  unsigned int entry_index;       // ebp - 0xc
  int inner_index;                // ebp - 0x8
  int selected_state_sprite;      // ebp - 0x4

start:

  InitializeAnimatedNoiseGrid();
  FillGraphicsRect(g_page2_window_bounds, 0, 0, 0x140, 200, 0);
  icon_y_scaled = 0;
  for (location_block_start_index = 0; location_block_start_index < 0x40; location_block_start_index++)
  {
    for (icon_width_scaled = 0; icon_width_scaled < 0x40; icon_width_scaled++)
    {
      if (((location_block_start_index < 2) || (icon_width_scaled < 2)) || ((0x3e <= location_block_start_index) || (0x3e <= icon_width_scaled)))
      {
        PutGraphicsPixel(g_page2_window_bounds, location_block_start_index, icon_width_scaled, 0);
        continue;
      }

      selected_state_sprite = (location_block_start_index + icon_width_scaled) * 3 - 0x20;
      inner_index = (icon_width_scaled - location_block_start_index) * 3 + 100;
      if ((selected_state_sprite < 4) || (inner_index < 4) || ((0x13c <= selected_state_sprite) || (0xc4 <= inner_index)))
      {
        PutGraphicsPixel(g_page2_window_bounds, location_block_start_index, icon_width_scaled, 0);
        continue;
      }

      uVar1 = CalculateWorldTerrainValue(location_block_start_index, icon_width_scaled);
      switch (uVar1 / 8)
      {
      case 0:
      case 1:
        entry_index = 0;
        break;
      case 2:
        entry_index = 1;
        if (0x16 <= uVar1)
        {
          entry_index = 8;
        }
        break;
      case 3:
        entry_index = 3;
        break;
      case 4:
        entry_index = 6;
        if (uVar1 < 0x22)
        {
          entry_index = 0xd;
        }
        break;
      case 5:
        if (0x2a >= uVar1)
        {
          entry_index = 10;
          break;
        }
      case 6:
        entry_index = 2;
        break;
      case 7:
        if (uVar1 >= 0x3c)
        {
          entry_index = 4;
        }
        else
        {
          entry_index = 0xc;
        }
        if (icon_width_scaled < 0x10 || icon_width_scaled > 0x30)
        {
          entry_index = 0xf;
        }

        if (uVar1 >= 0x3c)
        {
          entry_index = 5;
        }
        else
        {
          entry_index = 0xf;
        }

        break;

      case 8:
      case 9:
      case 10:
      case 11:
        entry_index = 5;
        break;
      }

      PutGraphicsPixel(g_page2_window_bounds, location_block_start_index, icon_width_scaled, (int)entry_index);
      if (entry_index != 0)
      {
        icon_y_scaled++;
      }
    }
  }

  if (icon_y_scaled < 0x6d6)
  {
    // TODO : Why didn't they just write this as do/while?
    goto start;
  }

  for (location_block_start_index = 1; location_block_start_index < 0x3f; location_block_start_index++)
  {
    for (icon_width_scaled = 1; icon_width_scaled < 0x3f; icon_width_scaled++)
    {
      entry_index = GetWorldTileType(location_block_start_index, icon_width_scaled);
      if (entry_index == 0)
      {
        neighbor_is_open = 0;
        for (i = 1; i <= 8; i += 2)
        {
          if (GetWorldTileType(location_block_start_index + g_neighbor_dx[i], icon_width_scaled + g_neighbor_dy[i]) == 0)
          {
            neighbor_is_open = 1;
            break;
          }
        }
        if (neighbor_is_open == 0)
        {
          PutGraphicsPixel(g_page2_window_bounds, location_block_start_index, icon_width_scaled, 6);
        }
      }
    }
  }

  PropagatePathConnectivity();
  if (GenerateWorldTownSlots() == 0)
  {
    goto start;
  }

  GenerateTownConnections();
  for (i = 0; i < 7; i++)
  {
    g_lair_or_monster_slots[i].respawn_timestamp = (4 - g_shandalar_difficulty) * i * -100;
  }
}

// FUNCTION: SHANDALAR 0x004f717a
int GenerateWorldTownSlots(void)
{
  int random_timer;                       // ebp - 0x4c
  int world_magic_town_count;             // ebp - 0x48
  int generation_pass;                    // ebp - 0x44
  int placement_attempt_count;            // ebp - 0x40
  int nearest_town_distance;              // ebp - 0x3c
  unsigned int world_magic_mask;          // ebp - 0x38
  int valid_world;                        // ebp - 0x34
  int scan_index;                         // ebp - 0x30
  int magic_slot_or_trade_color_index;    // ebp - 0x2c
  int town_index;                         // ebp - 0x28
  int candidate_y;                        // ebp - 0x24
  int candidate_x;                        // ebp - 0x20
  int special_location_world_magic_index; // ebp - 0x1c
  unsigned int tile_type;                 // ebp - 0x18
  int nearest_city_distance;              // ebp - 0x14
  int distance;                           // ebp - 0x10
  int special_location_mask;              // ebp - 0xc
  int placement_slot_index;               // ebp - 0x8
  int major_town_count;                   // ebp - 0x4

  generation_pass = 0;
generate_pass:
  if (4 < ++generation_pass)
  {
    return 0;
  }

  world_magic_town_count = 0;
  major_town_count = world_magic_town_count;
  special_location_mask = major_town_count;

  for (town_index = 0; town_index < 0xc; town_index = town_index + 1)
  {
    Scards[town_index].worldmagic_city = 0;
  }

  placement_slot_index = (int)clock() % 0x80;

  memset(g_town_slots, 0xff, 0x3200);

  for (town_index = 0; town_index < 0x80; town_index = town_index + 1)
  {
    placement_attempt_count = 0;
    do
    {
      valid_world = 0;
      candidate_x = internal_rand(0x40);
      candidate_y = internal_rand(0x40);
      tile_type = GetWorldTileType(candidate_x, candidate_y);
      if (tile_type == 0)
      {
      }
      else
      {
        nearest_town_distance = 0x7fff;
        nearest_city_distance = 0x7fff;
        for (scan_index = 0; scan_index < 0x80; scan_index = scan_index + 1)
        {
          if (g_town_slots[scan_index].world_x == -1)
          {
            continue;
          }

          distance = ApproximateDistance(candidate_x - g_town_slots[scan_index].world_x, candidate_y - g_town_slots[scan_index].world_y);
          if (distance < nearest_town_distance)
          {
            nearest_town_distance = distance;
          }
          if ((distance < nearest_city_distance) && (g_town_slots[scan_index].location_type == 3))
          {
            nearest_city_distance = distance;
          }
        }

        placement_attempt_count = placement_attempt_count + 1;
        if (7 - placement_attempt_count / 100 > nearest_town_distance)
        {
        }
        else
        {
          valid_world = 1;

          g_town_slots[placement_slot_index].world_x = candidate_x;
          g_town_slots[placement_slot_index].world_y = candidate_y;

          if (0x20 < nearest_city_distance)
          {
            g_town_slots[placement_slot_index].location_type = 3;
          }
          else if (0xa < nearest_town_distance)
          {
            g_town_slots[placement_slot_index].location_type = 2;
          }
          else
          {
            g_town_slots[placement_slot_index].location_type = 1;
          }

          g_town_slots[placement_slot_index].trade_color_and_type = 0;
          g_town_slots[placement_slot_index].status_and_ruling_wizard = g_town_slots[placement_slot_index].trade_color_and_type;

          for (scan_index = 0; scan_index < 8; scan_index = scan_index + 1)
          {
            g_town_slots[placement_slot_index].card_restock_timers[scan_index] = 0xfffffc18;
          }
          g_town_slots[placement_slot_index].quest_restock_timer = 0xfffffc18;
          g_town_slots[placement_slot_index].special_card_restock_timer = 0xfffffc18;

          special_location_world_magic_index = 0;
          if (tile_type == 3)
          {
            special_location_world_magic_index = 1;
          }
          if (tile_type == 1)
          {
            special_location_world_magic_index = 2;
          }
          if (tile_type == 2)
          {
            special_location_world_magic_index = 3;
          }
          if (tile_type == 5)
          {
            special_location_world_magic_index = 4;
          }
          if (tile_type == 6)
          {
            special_location_world_magic_index = 5;
          }

          if ((0x10 < nearest_city_distance) && (special_location_world_magic_index != 0) && ((special_location_mask & 1 << special_location_world_magic_index) == 0))
          {
            g_town_slots[placement_slot_index].location_type = 4;
            special_location_mask = special_location_mask | 1 << special_location_world_magic_index;
          }

          world_magic_mask = GetWorldTileMagicMask(tile_type);
          if ((placement_slot_index != 0) && ((g_town_slots[placement_slot_index].location_type == 3) || (g_town_slots[placement_slot_index].location_type == 2)))
          {
            for (scan_index = 0; scan_index < 99; scan_index = scan_index + 1)
            {
              magic_slot_or_trade_color_index = internal_rand(10) + 2;
              if (Scards[magic_slot_or_trade_color_index].worldmagic_city != 0)
              {
                continue;
              }

              if ((world_magic_mask & 1 << (char)(magic_slot_or_trade_color_index / 2)) == 0)
              {
                continue;
              }

              Scards[magic_slot_or_trade_color_index].worldmagic_city = placement_slot_index;
              break;
            }

            if (0x63 <= scan_index)
            {
              random_timer = internal_rand(2);
              Scards[random_timer].worldmagic_city = placement_slot_index;
            }

            if (world_magic_town_count < 10)
            {
              g_town_slots[placement_slot_index].status_and_ruling_wizard = g_town_slots[placement_slot_index].status_and_ruling_wizard | 1;

              world_magic_town_count++;
            }
          }

          if (valid_world != 0)
          {
            SetWorldMapPixelFlags(0x10, candidate_x, candidate_y);
          }
          placement_slot_index = (placement_slot_index + placement_slot_index * 4 + 1) % 0x80;
        }
      }
    } while (valid_world == 0);

    if (1 < g_town_slots[town_index].location_type)
    {
      major_town_count = major_town_count + 1;
    }
  }

  valid_world = 1;
  if ((special_location_mask != 0x3e) || (major_town_count < 0x1e))
  {
    valid_world = 0;
  }

  for (magic_slot_or_trade_color_index = 0; magic_slot_or_trade_color_index < 6; magic_slot_or_trade_color_index = magic_slot_or_trade_color_index + 1)
  {
    do
    {
      do
      {
        town_index = internal_rand(0x80);
      } while (g_town_slots[town_index].location_type <= 1);
    } while ((g_town_slots[town_index].location_type == 4) || (g_town_slots[town_index].trade_color_and_type != 0));
    g_town_slots[town_index].trade_color_and_type = 1 << (char)magic_slot_or_trade_color_index;
  }

  scan_index = internal_rand(0xc);
  for (town_index = 0; town_index < 0x80; town_index = town_index + 1)
  {
    if ((1 < g_town_slots[town_index].location_type) && (g_town_slots[town_index].location_type < 4))
    {
      if ((scan_index & 1) != 0)
      {
        g_town_slots[town_index].trade_color_and_type = 1 << (char)((scan_index % 0xc) / 2);
      }
      else
      {
        g_town_slots[town_index].trade_color_and_type = (((scan_index % 10) / 2) + 1) * 0x100;
      }
      scan_index = scan_index + 1;
    }
  }

  for (town_index = 0; town_index < 0xc; town_index = town_index + 1)
  {
    if (Scards[town_index].worldmagic_city == 0)
    {
      valid_world = 0;
    }
    if ((g_world_magic_bitmap & 1 << (char)town_index) != 0)
    {
      Scards[town_index].worldmagic_city = 0;
    }
  }

  if (valid_world == 0)
  {
    for (town_index = 0; town_index < 0x80; town_index = town_index + 1)
    {
      ClearWorldMapPixelFlags(0x10, g_town_slots[town_index].world_x, g_town_slots[town_index].world_y);
    }

    for (town_index = 0; town_index < 0xc; town_index = town_index + 1)
    {
      Scards[town_index].worldmagic_city = 0;
    }
    goto generate_pass;
  }

  return 1;
}

// FUNCTION: SHANDALAR 0x004f78d3
void GenerateTownConnections(void)
{
  int connection_index; /* -0x24 */
  int best_dist;        /* -0x20 */
  int candidate_index;  /* -0x1c */
  int town_index;       /* -0x18 */
  int sample_index;     /* -0x14 */
  int distance;         /* -0x10 */
  int chosen_index;     /* -0x0c */
  int best_index;       /* -0x08 */
  int attempts;         /* -0x04 */

  for (town_index = 0; town_index < 0x80; town_index = town_index + 1)
  {
    for (connection_index = 0; connection_index < g_town_slots[town_index].location_type; connection_index = connection_index + 1)
    {
      attempts = 0;
    retry:
      best_dist = 0x7fff;
      sample_index = 0;
      for (; sample_index < 0x2a; sample_index = sample_index + 1)
      {
        candidate_index = internal_rand(0x80);
        distance =
            ApproximateDistance(g_town_slots[town_index].world_x - g_town_slots[candidate_index].world_x,
                                g_town_slots[town_index].world_y - g_town_slots[candidate_index].world_y);
        if (distance < best_dist)
        {
          best_dist = distance;
          chosen_index = best_index;
          best_index = candidate_index;
        }
      }

      if (CreateTownConnectionPath(g_town_slots[town_index].world_x, g_town_slots[town_index].world_y,
                                   g_town_slots[chosen_index].world_x, g_town_slots[chosen_index].world_y) == 0)
      {
        if (++attempts < 3)
        {
          goto retry;
        }
      }
    }
  }
}

// FUNCTION: SHANDALAR 0x004f7a3c
int CreateTownConnectionPath(int start_x, int start_y, int target_x, int target_y)
{
  int step_count;      /* -0x30 */
  int best_direction;  /* -0x2c */
  int best_score;      /* -0x28 */
  int flags_masked;    /* -0x24 */
  int direction_index; /* -0x20 */
  int cur_y;           /* -0x1c */
  int cur_x;           /* -0x18 */
  int target_dist;     /* -0x14 */
  int next_y;          /* -0x10 */
  int next_x;          /* -0x0c */
  int tile_type;       /* -0x08 */
  int score;           /* -0x04 */

  cur_x = start_x;
  cur_y = start_y;
  step_count = 0;

  BlitGraphicsRect(g_page2_window_bounds, 0, 0, 0x40, 0x80, g_page2_window_bounds, 0x40, 0);
  SetWorldMapPixelFlags(0x20, cur_x, cur_y);

  do
  {
    target_dist = ApproximateDistance(target_x - cur_x, target_y - cur_y);
    best_direction = -1;
    best_score = 0x7fff;

    for (direction_index = 1; direction_index <= 8; direction_index = direction_index + 1)
    {
      next_x = g_neighbor_dx[direction_index] + cur_x;
      next_y = g_neighbor_dy[direction_index] + cur_y;

      tile_type = GetWorldTileType(next_x, next_y);
      if (tile_type == 0)
      {
        continue;
      }

      score = ApproximateDistance(target_x - next_x, target_y - next_y);
      if (target_dist <= score)
      {
        continue;
      }

      if ((tile_type == 2) || (tile_type == 0xb))
      {
        score = score + 1;
      }
      if ((tile_type == 4) || (tile_type == 5))
      {
        score = score + 4;
      }

      if ((GetWorldMapPixelFlags(next_x, next_y) & 0x20) != 0)
      {
        if (step_count >= 1)
        {
          score = score - 4;
        }
      }

      if (best_score > score)
      {
        best_score = score;
        best_direction = direction_index;
      }
    }

    if (best_direction == -1)
    {
      BlitGraphicsRect(g_page2_window_bounds, 0x40, 0, 0x40, 0x80, g_page2_window_bounds, 0, 0);
      return 0;
    }

    next_x = g_neighbor_dx[best_direction] + cur_x;
    next_y = g_neighbor_dy[best_direction] + cur_y;
    flags_masked = GetWorldMapPixelFlags(next_x, next_y) & 0x20;

    MarkPathConnection(cur_x, cur_y, best_direction);

    if ((flags_masked != 0) && (step_count >= 1))
    {
      return 1;
    }

    cur_x = next_x;
    cur_y = next_y;
    step_count = step_count + 1;
  } while ((cur_x != target_x) || (cur_y != target_y));

  return 1;
}

// FUNCTION: SHANDALAR 0x004f7c7d
void PropagatePathConnectivity(void)
{
  int scan_x;       /* -0x14 */
  int y;            /* -0x10 */
  int x;            /* -0x0c */
  int found_change; /* -0x08 */
  int scan_y;       /* -0x04 */

  PutGraphicsPixel(g_page2_window_bounds, 0xa8, 0x58, 0xff);
  do
  {
    found_change = 0;
    for (x = 4; x < 0x40; x = x + 4)
    {
      for (y = 4; y < 0x40; y = y + 4)
      {
        if (ReadGraphicsPixel(g_page2_window_bounds->page_number, x + 0x80, y + 0x40) != 0)
        {
          FillGraphicsRect(g_page2_window_bounds, 0x40, 0, 0x40, 0x40, 0);
          FloodFillPathConnectivity(x, y, 8);
          PutGraphicsPixel(g_page2_window_bounds, x + 0x80, y + 0x40, 0);

          for (scan_x = 0; scan_x < 0x40; scan_x = scan_x + 1)
          {
            for (scan_y = 0; scan_y < 0x40; scan_y = scan_y + 1)
            {
              if (ReadGraphicsPixel(g_page2_window_bounds->page_number, scan_x + 0x40, scan_y) != 0)
              {
                if (ReadGraphicsPixel(g_page2_window_bounds->page_number, scan_x + 0x80, scan_y) == 0)
                {
                  found_change = 1;
                  PutGraphicsPixel(g_page2_window_bounds, scan_x + 0x80, scan_y, 0xff);
                  PutGraphicsPixel(g_page2_window_bounds, scan_x + 0x80, scan_y + 0x40, 0xff);
                }
              }
            }
          }
        }
      }
    }
  } while (found_change != 0);

  for (x = 0; x < 0x40; x = x + 1)
  {
    for (y = 0; y < 0x40; y = y + 1)
    {
      if (ReadGraphicsPixel(g_page2_window_bounds->page_number, x + 0x80, y) == 0)
      {
        PutGraphicsPixel(g_page2_window_bounds, x, y, 0);
      }
    }
  }
}

// FUNCTION: SHANDALAR 0x004f7eb2
void FloodFillPathConnectivity(int x, int y, unsigned int depth)
{
  char tile_value; /* -0x10 */
  char pad_0d[3];
  char direction_index; /* -0x0c */
  char pad_09[3];
  char next_y; /* -0x08 */
  char pad_05[3];
  char next_x; /* -0x04 */
  char pad_01[3];

  PutGraphicsPixel(g_page2_window_bounds, x + 0x40, y, depth);
  if ((int)depth <= 1)
  {
  }
  else
  {
    for (direction_index = 1; (int)direction_index <= 8; direction_index = direction_index + 2)
    {
      next_x = (char)(g_neighbor_dx[(int)direction_index] + x);
      next_y = (char)(g_neighbor_dy[(int)direction_index] + y);
      tile_value = (char)ReadGraphicsPixel(g_page2_window_bounds->page_number, next_x + 0x40, next_y);
      if ((int)tile_value < (int)depth)
      {
        if (ReadGraphicsPixel(g_page2_window_bounds->page_number, next_x, next_y) != 0)
        {
          FloodFillPathConnectivity(next_x, next_y, depth - 1);
        }
      }
    }
  }
}

// FUNCTION: SHANDALAR 0x004f7fb9
int CalculateWorldTerrainValue(int x, int y)
{
  int edge_penalty;
  int terrain_value;

  edge_penalty = 0x10 / (x + 2);
  edge_penalty = edge_penalty + 0x10 / (y + 2);
  edge_penalty = edge_penalty + 0x10 / (0x41 - x);
  edge_penalty = edge_penalty + 0x10 / (0x41 - y);

  edge_penalty = abs(x - 0x20) + abs(y - 0x20);
  edge_penalty = (edge_penalty * edge_penalty) / 512;
  edge_penalty += abs(x - y) / 16;

  x = x << 5;
  y = y << 5;
  terrain_value = SampleAnimatedNoiseGridBilinear(x, y) << 2;

  terrain_value = terrain_value + SampleAnimatedNoiseGridBilinear(x << 3, y << 3) * 2;
  terrain_value = terrain_value + SampleAnimatedNoiseGridBilinear(x << 4, y << 4);
  terrain_value = terrain_value - ClampIntToRange(edge_penalty, 0, 0xc) * 0x200;

  return ClampIntToRange(((7 * terrain_value) / 64) / 4, 0, 100);
}

// FUNCTION: SHANDALAR 0x004f8101
void InitializeAnimatedNoiseGrid(void)
{
  int weighted_neighbor_sum;
  int x;
  int direction_index;
  int y;

  for (y = 0; y < 0x12; y = y + 1)
  {
    for (x = 0; x < 0x12; x = x + 1)
    {
      g_animated_noise_grid[0].samples[y][x] = (char)internal_rand(0x10);
    }
    g_animated_noise_grid[0].samples[y][0x12] = g_animated_noise_grid[0].samples[y][0];
  }
  for (x = 0; x < 0x12; x = x + 1)
  {
    g_animated_noise_grid[0].samples[0x12][x] = g_animated_noise_grid[0].samples[0][x];
  }
  for (y = 0; y <= 0x10; y = y + 1)
  {
    for (x = 0; x <= 0x10; x = x + 1)
    {
      weighted_neighbor_sum = (int)g_animated_noise_grid[0].samples[y][x] << 2;
      for (direction_index = 1; direction_index <= 8; direction_index = direction_index + 1)
      {
        weighted_neighbor_sum +=
            (int)g_animated_noise_grid[0].samples[y + g_neighbor_dx[direction_index] + 1][x + g_neighbor_dy[direction_index] + 1];
      }
      g_animated_noise_grid[1].samples[y][x] = g_animated_noise_grid[0].samples[y][x];
    }
  }
  for (y = 0; y <= 0x10; y = y + 1)
  {
    g_animated_noise_grid[1].samples[y][0x10] = g_animated_noise_grid[1].samples[y][0];
  }
  for (x = 0; x <= 0x10; x = x + 1)
  {
    g_animated_noise_grid[1].samples[0x10][x] = g_animated_noise_grid[1].samples[0][x];
  }
}

// FUNCTION: SHANDALAR 0x004f82f2
int SampleAnimatedNoiseGridBilinear(int x_fixed, int y_fixed)
{
  int weighted_sample;
  unsigned int entry_index;
  unsigned int inner_index;
  unsigned int location_block_start_index;
  unsigned int selected_state_sprite;

  x_fixed = x_fixed - 0x80;
  y_fixed = y_fixed - 0x80;
  entry_index = (x_fixed >> 8) & 0xf;
  location_block_start_index = (x_fixed & 0xff) / 8;
  inner_index = (y_fixed >> 8) & 0xf;
  selected_state_sprite = (y_fixed & 0xff) / 8;

  weighted_sample = (int)g_animated_noise_grid[1].samples[entry_index][inner_index] * (0x20 - selected_state_sprite) * (0x20 - location_block_start_index);
  weighted_sample = weighted_sample + (int)g_animated_noise_grid[1].samples[entry_index + 1][inner_index] * (0x20 - selected_state_sprite) * location_block_start_index;
  weighted_sample = weighted_sample + (int)g_animated_noise_grid[1].samples[entry_index][inner_index + 1] * (0x20 - location_block_start_index) * selected_state_sprite;
  weighted_sample = weighted_sample + (int)g_animated_noise_grid[1].samples[entry_index + 1][inner_index + 1] * selected_state_sprite * location_block_start_index;
  return weighted_sample / 32;
}

// FUNCTION: SHANDALAR 0x005081fa
void InitializeCastleDungeonSlots(void)
{
  int card_index;
  int nearest_distance;
  int nearest_town_index;
  int scan_index;
  int candidate_y;
  int candidate_x;
  int distance;
  int mana_castle_town_index;
  int inner_index;
  int entry_index;

  for (entry_index = 0; entry_index < 0xf; entry_index = entry_index + 1)
  {
    g_castle_dungeon_slots[entry_index].card_slot_3 = -1;
    g_castle_dungeon_slots[entry_index].card_slot_2 = g_castle_dungeon_slots[entry_index].card_slot_3;
    g_castle_dungeon_slots[entry_index].card_slot_1 = g_castle_dungeon_slots[entry_index].card_slot_2;
    g_castle_dungeon_slots[entry_index].times_entered = -1;
    g_castle_dungeon_slots[entry_index].reserved_2c = g_castle_dungeon_slots[entry_index].times_entered;
  }

  entry_index = 0;
  for (card_index = 0; card_index < g_card_count - 0x39; card_index = card_index + 1)
  {
    if ((global_cards_data[card_index].extra_ability & 0x100) == 0)
    {
      continue;
    }

    do
    {
      entry_index = internal_rand(10) + 5;
    } while (g_castle_dungeon_slots[entry_index].card_slot_3 != -1);

    if (g_castle_dungeon_slots[entry_index].card_slot_1 == -1)
    {
      g_castle_dungeon_slots[entry_index].card_slot_1 = card_index;
    }
    else if (g_castle_dungeon_slots[entry_index].card_slot_2 == -1)
    {
      g_castle_dungeon_slots[entry_index].card_slot_2 = card_index;
    }
    else
    {
      g_castle_dungeon_slots[entry_index].card_slot_3 = card_index;
    }
  }

  for (entry_index = 0; entry_index < 0xf; entry_index = entry_index + 1)
  {
    do
    {
      do
      {
        do
        {
          candidate_x = internal_rand(0x40);
          candidate_y = internal_rand(0x40);
        } while (GetWorldTileType(candidate_x, candidate_y) == 0);
      } while ((GetWorldMapPixelFlags(candidate_x, candidate_y) & 0x30) != 0);

      nearest_distance = 0xff;
      for (scan_index = 0; scan_index < 0x80; scan_index = scan_index + 1)
      {
        if ((entry_index < 5) && (g_town_slots[scan_index].location_type == 4))
        {
          if (GetWorldTileMagicMask(GetWorldTileType(g_town_slots[scan_index].world_x, g_town_slots[scan_index].world_y)) == 1 << (entry_index + 1))
          {
            mana_castle_town_index = scan_index;
          }
        }

        if ((g_town_slots[scan_index].location_type <= 1) || (g_town_slots[scan_index].location_type == 4))
        {
          continue;
        }

        distance = ApproximateDistance(g_town_slots[scan_index].world_x - candidate_x,
                                       g_town_slots[scan_index].world_y - candidate_y);
        if (nearest_distance > distance)
        {
          nearest_distance = distance;
          nearest_town_index = scan_index;
        }
      }

      for (scan_index = 0; scan_index < entry_index; scan_index = scan_index + 1)
      {
        distance = ApproximateDistance(g_castle_dungeon_slots[scan_index].world_x - candidate_x, g_castle_dungeon_slots[scan_index].world_y - candidate_y);
        if (nearest_distance > distance)
        {
          nearest_distance = distance;
        }
      }
    } while (nearest_distance < 4);

    SetWorldMapPixelFlags(0x40, candidate_x, candidate_y);
    g_castle_dungeon_slots[entry_index].world_x = candidate_x;
    g_castle_dungeon_slots[entry_index].world_y = candidate_y;
    g_castle_dungeon_slots[entry_index].north_of_town_index = nearest_town_index;
    g_castle_dungeon_slots[entry_index].color = (unsigned char)(internal_rand(5) + 1);
    g_castle_dungeon_slots[entry_index].monster_flags = 2;

    if (entry_index < 5)
    {
      g_castle_dungeon_slots[entry_index].color = entry_index + 1;
      g_castle_dungeon_slots[entry_index].monster_flags = 0x81;
      g_castle_dungeon_slots[entry_index].world_x = g_town_slots[mana_castle_town_index].world_x;
      g_castle_dungeon_slots[entry_index].world_y = g_town_slots[mana_castle_town_index].world_y;
    }

    inner_index = 0;
    if ((int)(unsigned int)(unsigned char)g_castle_dungeon_slots[entry_index].monster_flags <= internal_rand(2) + 1)
    {
      if (1 < (int)(unsigned int)(unsigned char)g_castle_dungeon_slots[entry_index].monster_flags)
      {
        inner_index = 0x1c;
      }
      else
      {
        inner_index = 0x10;
      }
      g_castle_dungeon_slots[entry_index].monster_flags = g_castle_dungeon_slots[entry_index].monster_flags | 0x80;
    }
    else
    {
      inner_index = 0;
      for (scan_index = 0; scan_index < (int)(unsigned int)(unsigned char)g_castle_dungeon_slots[entry_index].monster_flags; scan_index = scan_index + 1)
      {
        inner_index = inner_index + scan_index * 2 + 4;
      }
    }

    if (g_castle_dungeon_slots[entry_index].card_slot_2 == -1)
    {
      inner_index = (inner_index * 3) / 2;
    }
    if (g_castle_dungeon_slots[entry_index].card_slot_3 != -1)
    {
      inner_index = (inner_index * 2) / 3;
    }

    g_castle_dungeon_slots[entry_index].card_in_effect =
        g_dungeon_card_effect_by_monster_flags[((g_castle_dungeon_slots[entry_index].monster_flags & 0xc0) != 0 ? 4 : 0) +
                                               (g_castle_dungeon_slots[entry_index].monster_flags & 0x7f) + internal_rand(2)];
    g_castle_dungeon_slots[entry_index].rules_bitmap = 1;

    switch (inner_index / 4)
    {
    case 0:
    case 1:
    case 2:
      g_castle_dungeon_slots[entry_index].monster_flags++;
    case 3:
      g_castle_dungeon_slots[entry_index].card_in_effect = g_dungeon_card_effect_by_color[(char)g_castle_dungeon_slots[entry_index].color];
      g_castle_dungeon_slots[entry_index].rules_bitmap = g_castle_dungeon_slots[entry_index].rules_bitmap | 1 << (char)(internal_rand(5) + 4);
      break;
    case 4:
      g_castle_dungeon_slots[entry_index].card_in_effect = g_dungeon_card_effect_by_color[(char)g_castle_dungeon_slots[entry_index].color];
      break;
    case 5:
      g_castle_dungeon_slots[entry_index].rules_bitmap = g_castle_dungeon_slots[entry_index].rules_bitmap | 1 << (char)(internal_rand(5) + 4);
      g_castle_dungeon_slots[entry_index].card_in_effect = -1;
      break;
    case 6:
      g_castle_dungeon_slots[entry_index].card_in_effect = -1;
      break;
    }

    if (((g_castle_dungeon_slots[entry_index].rules_bitmap & 0x100) != 0) ||
        (((g_castle_dungeon_slots[entry_index].monster_flags & 0x3f) < 2) && ((g_castle_dungeon_slots[entry_index].rules_bitmap & 0x20) != 0)))
    {
      g_castle_dungeon_slots[entry_index].card_in_effect = g_dungeon_card_effect_by_color[(char)g_castle_dungeon_slots[entry_index].color];
      g_castle_dungeon_slots[entry_index].rules_bitmap = g_castle_dungeon_slots[entry_index].rules_bitmap & 0xfffffedf;
    }

    if ((g_castle_dungeon_slots[entry_index].monster_flags & 0x7f) == 1)
    {
      g_castle_dungeon_slots[entry_index].rules_bitmap = g_castle_dungeon_slots[entry_index].rules_bitmap & 0xfffffffe;
    }

    if (entry_index < 5)
    {
      g_castle_dungeon_slots[entry_index].rules_bitmap = g_castle_dungeon_slots[entry_index].rules_bitmap | 1;
      g_castle_dungeon_slots[entry_index].rules_bitmap = g_castle_dungeon_slots[entry_index].rules_bitmap | 2;
      g_castle_dungeon_slots[entry_index].card_in_effect = g_mana_castle_card_effect_by_difficulty[entry_index][g_shandalar_difficulty];
    }
  }
}

// FUNCTION: SHANDALAR 0x0043146b
unsigned int GetWorldTileType(int x, int y)
{
  if ((0x40 <= x) || (x < 0))
  {
    x = 0;
  }
  if ((0x40 <= y) || (y < 0))
  {
    y = 0;
  }
  return GetGraphicsPixelColorRef(g_page2_window_bounds, x, y) & 0xf;
}

// FUNCTION: SHANDALAR 0x005611c8
unsigned int GetWorldTileMagicMask(unsigned int tile_type)
{
  unsigned int entry_index;

  switch (tile_type)
  {
  case 3:
    entry_index = 2;
    break;
  case 0xb:
    entry_index = 10;
    break;
  case 1:
    entry_index = 4;
    break;
  case 2:
    entry_index = 8;
    break;
  case 4:
    entry_index = 0x30;
    break;
  case 5:
    entry_index = 0x10;
    break;
  case 6:
    entry_index = 0x20;
    break;
  case 10:
    entry_index = 0x28;
    break;
  case 0xf:
    entry_index = 0x18;
    break;
  case 7:
    entry_index = 0x24;
    break;
  case 0xc:
    entry_index = 0x12;
    break;
  case 0xd:
    entry_index = 0x22;
    break;
  case 0xe:
    entry_index = 0xc;
    break;
  case 9:
    entry_index = 0x14;
    break;
  case 8:
    entry_index = 6;
    break;
  default:
    entry_index = 0;
    break;
  }

  return entry_index;
}

// FUNCTION: SHANDALAR 0x004314ca
unsigned int GetWorldMapPixelFlags(int x, int y)
{
  if (x >= 0x40 || x < 0)
    return 0;

  if (y >= 0x40 || y < 0)
    return 0;

  return GetGraphicsPixelColorRef(g_page2_window_bounds, x, y);
}

// FUNCTION: SHANDALAR 0x00431526
void SetWorldMapPixelFlags(unsigned int mask, int x, int y)
{
  if (x >= 0x40 || x < 0)
    return;

  if (y >= 0x40 || y < 0)
    return;

  PutGraphicsPixel(g_page2_window_bounds, x, y, GetGraphicsPixelColorRef(g_page2_window_bounds, x, y) | mask);
}

// FUNCTION: SHANDALAR 0x00431593
void ClearWorldMapPixelFlags(unsigned int mask, int x, int y)
{
  if (x >= 0x40 || x < 0)
    return;

  if (y >= 0x40 || y < 0)
    return;

  PutGraphicsPixel(g_page2_window_bounds, x, y, GetGraphicsPixelColorRef(g_page2_window_bounds, x, y) & ~mask);
}

// FUNCTION: SHANDALAR 0x0043174d
void MarkPathConnection(int x, int y, int direction_index)
{
  if (x >= 0x40 || x < 0)
    return;

  if (y >= 0x40 || y < 0)
    return;

  PutGraphicsPixel(g_page2_window_bounds, x, y + 0x40,
                   GetGraphicsPixelColorRef(g_page2_window_bounds, x, y + 0x40) | (1 << (direction_index - 1)));
  SetWorldMapPixelFlags(0x20, x, y);

  x += g_neighbor_dx[direction_index];
  y += g_neighbor_dy[direction_index];
  direction_index = ((direction_index + 3) & 7) + 1;

  PutGraphicsPixel(g_page2_window_bounds, x, y + 0x40,
                   GetGraphicsPixelColorRef(g_page2_window_bounds, x, y + 0x40) | (1 << (direction_index - 1)));
  SetWorldMapPixelFlags(0x20, x, y);
}

// FUNCTION: SHANDALAR 0x00431859
unsigned int WorldRoadTileHasDirection(int tile_x, int tile_y, int direction_index)
{
  if ((tile_x >= 0x40) || (tile_x < 0))
  {
    return 0;
  }

  if ((tile_y >= 0x40) || (tile_y < 0))
  {
    return 0;
  }

  return GetGraphicsPixelColorRef(g_page2_window_bounds, tile_x, tile_y + 0x40) & (1U << ((unsigned char)(direction_index - 1U)));
}
