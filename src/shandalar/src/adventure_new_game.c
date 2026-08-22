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

// FUNCTION: SHANDALAR 0x004184d2
void LoadTownHintMetadata(void)
{
  int first_csvid;
  char difficulty_codes[8];
  int hint_index;
  int second_csvid;
  char line[256];
  FILE *hints_file;
  long body_offset;

  hints_file = fopen("hints.txt", "rt");
  hint_index = 0;
  while (hint_index < 0x100 && fgets(line, sizeof(line), hints_file) != NULL)
  {
    line[strcspn(line, "\n")] = '\0';
    if (line[0] == '.')
    {
      sscanf(line + 1, "%d %d %s", &first_csvid, &second_csvid, difficulty_codes);
      g_hint_card_pairs[hint_index].first = first_csvid;
      g_hint_card_pairs[hint_index].second = second_csvid;
      g_hint_difficulty_masks[hint_index] = 0;

      if (strchr(difficulty_codes, 'A') != (char *)0)
      {
        g_hint_difficulty_masks[hint_index] = g_hint_difficulty_masks[hint_index] | 1;
      }

      if (strchr(difficulty_codes, 'B') != (char *)0)
      {
        g_hint_difficulty_masks[hint_index] = g_hint_difficulty_masks[hint_index] | 2;
      }

      if (strchr(difficulty_codes, 'C') != (char *)0)
      {
        g_hint_difficulty_masks[hint_index] = g_hint_difficulty_masks[hint_index] | 4;
      }

      if (strchr(difficulty_codes, 'D') != (char *)0)
      {
        g_hint_difficulty_masks[hint_index] = g_hint_difficulty_masks[hint_index] | 8;
      }

      body_offset = ftell(hints_file);
      while (fgets(line, sizeof(line), hints_file) != NULL)
      {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] != '\0')
        {
          break;
        }
        body_offset = ftell(hints_file);
      }
      g_hint_text_offsets[hint_index] = body_offset;
      fseek(hints_file, body_offset, SEEK_SET);
      hint_index = hint_index + 1;
    }
  }

  do
  {
    g_hint_card_pairs[hint_index].second = -1;
    g_hint_card_pairs[hint_index].first = g_hint_card_pairs[hint_index].second;
    hint_index = hint_index + 1;
  } while (hint_index < 0x100);

  fclose(hints_file);
}
// FUNCTION: SHANDALAR 0x0041786e
void LoadCardRaritiesAndCsvOffsets(void)
{
  int icon_height_scaled;
  int icon_width_scaled;
  long selected_state_sprite;
  int location_block_start_index;
  int inner_index;
  FILE *entry_index;
  int scan_result;

  for (location_block_start_index = 0; location_block_start_index < 0x4e2; location_block_start_index = location_block_start_index + 1)
  {
    g_master_csv_offsets[location_block_start_index] = -1;
  }

  entry_index = fopen("concise.csv", "rt");
  inner_index = 0;

  for (location_block_start_index = 0; location_block_start_index < g_card_count; location_block_start_index = location_block_start_index + 1)
  {
    inner_index = global_cards_data[location_block_start_index].id;
    scan_result = fscanf(entry_index, "%d %d %ld\n", &icon_width_scaled, &icon_height_scaled, &selected_state_sprite);
    global_cards_data[location_block_start_index].rarity = (unsigned char)icon_height_scaled;
    g_master_csv_offsets[inner_index] = selected_state_sprite;
  }

  fclose(entry_index);
}
// FUNCTION: SHANDALAR 0x004bdd0a
void InitializeNewGameState(void)
{
  int icon_width_scaled;
  unsigned int uVar3;
  unsigned int uVar2;
  int location_block_start_index;
  int entry_index;
  int difficulty_copy;

  if (g_duel_ai_mode_state != 0)
  {
    return;
  }

  for (location_block_start_index = 0; location_block_start_index < 500; location_block_start_index = location_block_start_index + 1)
  {
    deck[location_block_start_index] = -1;
  }

  if (g_skip_new_game_state_initialization)
  {
    return;
  }

  for (location_block_start_index = 0; location_block_start_index < 7; location_block_start_index = location_block_start_index + 1)
  {
    g_opponent_deck_color_filter_by_color[location_block_start_index] = 0;
    g_duel_special_rules_by_color[location_block_start_index] = -1;
  }

  g_world_magic_bitmap = g_world_magic_bitmap | (1 << (g_selected_wizard_color * 2));
  Scards[g_selected_wizard_color * 2].worldmagic_city = 0;

  uVar2 = 1 << (BYTE)g_selected_wizard_color;
  entry_index = g_shandalar_difficulty + 1;
  difficulty_copy = entry_index;
  if (g_shandalar_difficulty == 3)
  {
    entry_index = 1;
    entry_index = 3;
  }

  switch (g_shandalar_difficulty)
  {
  case 0:
    if ((uVar2 < 1) & 0x20)
    {
      AddRandomStarterDeckCards(uVar2, 0xd, 7, 0xf, 1, 1);
    }
    else
    {
      AddRandomStarterDeckCards(uVar2, 0xd, 0xc, 10, 1, 1);
    }
    break;
  case 1:
    AddRandomStarterDeckCards(uVar2, 0xb, 4, 0xc, 1, 1);
    AddRandomStarterDeckCards(PickRandomColorBitExcludingMask(uVar2), 4, 3, 4, 0, 1);
    break;
  case 2:
    AddRandomStarterDeckCards(uVar2, 9, 3, 9, 1, 1);
    uVar3 = PickRandomColorBitExcludingMask(uVar2);
    AddRandomStarterDeckCards(uVar3, 5, 3, 4, 0, 1);
    AddRandomStarterDeckCards(PickRandomColorBitExcludingMask(uVar2 | uVar3), 4, 3, 3, 0, 1);
    break;
  case 3:
    AddRandomStarterDeckCards(uVar2, 6, 3, 5, 1, 1);
    AddRandomStarterDeckCards(1, 0xb, 5, 0xe, 0, 1);
    break;
  }

  g_journal_entry_count = 0;
  for (location_block_start_index = 0; location_block_start_index < 5; location_block_start_index = location_block_start_index + 1)
  {
    g_amulet_inventory[location_block_start_index] = 0;
  }

  g_amulet_inventory[g_selected_wizard_color - 1]++;
  for (location_block_start_index = 0; location_block_start_index < 3 - g_shandalar_difficulty; location_block_start_index = location_block_start_index + 1)
  {
    icon_width_scaled = internal_rand(5);
    g_amulet_inventory[icon_width_scaled]++;
  }

  for (location_block_start_index = 0; location_block_start_index < 0x80; location_block_start_index = location_block_start_index + 1)
  {
    g_town_slots[location_block_start_index].location_type = -1;
  }

  for (location_block_start_index = 0; location_block_start_index < 8; location_block_start_index = location_block_start_index + 1)
  {
    g_lair_or_monster_slots[location_block_start_index].entry_type = SHANDALAR_ENTRY_NONE;
  }

  for (location_block_start_index = 0; location_block_start_index < 1000; location_block_start_index = location_block_start_index + 1)
  {
    g_duel_victory_log[0] = 0;
  }

  for (location_block_start_index = 0; location_block_start_index < 4; location_block_start_index = location_block_start_index + 1)
  {
    g_ai_combat_value_weights[location_block_start_index] = 8;
  }

  for (location_block_start_index = 0; location_block_start_index < 0x96; location_block_start_index = location_block_start_index + 1)
  {
    deck[location_block_start_index] = deck[location_block_start_index] | 0x10000;
  }
  g_selected_wizard_color = -1;
}

// FUNCTION: SHANDALAR 0x004bdccc
unsigned int PickRandomColorBitExcludingMask(unsigned int excluded_mask)
{
  int entry_index;

  do
  {
    entry_index = internal_rand(5) + 1;
  } while ((excluded_mask & (1U << (entry_index & 0xff))) != 0);

  return 1U << (entry_index & 0xff);
}

// FUNCTION: SHANDALAR 0x004be0bf
int AddRandomStarterDeckCards(unsigned int color_mask, int land_count, int spell_count, int creature_count, int add_rare, int allow_artifact_spells)
{
  unsigned int required_color_mask;
  unsigned int candidate_color_filter;
  unsigned int use_artifact_pool;
  unsigned int selected_card_id;
  unsigned int added_count;
  int duplicate_attempt_count;
  int card_ok;

  for (added_count = 0; land_count > (int)added_count; added_count = added_count + 1)
  {
    selected_card_id = PickRandomCardMatchingTypeAndColor(1, color_mask);
    if ((IsCardColorCompatibleWithMask((int)(char)global_cards_data[selected_card_id].color, color_mask, 0) != 0) &&
        ((int)selected_card_id <= 4) &&
        ((global_cards_data[selected_card_id].expansion & 0xc1) != 0))
    {
      AddCardToDeckSorted(selected_card_id);
    }
    else
    {
      added_count = added_count - 1;
    }
  }

  for (added_count = 0; spell_count > (int)added_count; added_count = added_count + 1)
  {
    if (allow_artifact_spells != 0)
    {
      use_artifact_pool = rand() & 1;
    }
    else
    {
      use_artifact_pool = 0;
    }

    if (use_artifact_pool != 0)
    {
      candidate_color_filter = 1;
    }
    else
    {
      candidate_color_filter = color_mask;
    }

    selected_card_id = PickRandomCardMatchingTypeAndColor((-(unsigned int)(use_artifact_pool == 0) & 0xffffffc4) + 0x40, candidate_color_filter);
    if (((g_shandalar_difficulty == 0) && ((global_cards_data[selected_card_id].static_ability & 3) != 0)) ||
        ((global_cards_data[selected_card_id].extra_ability & 0x900) != 0))
    {
      added_count = added_count - 1;
    }
    else
    {
      if ((global_cards_data[selected_card_id].type & 4) != 0)
      {
        use_artifact_pool = 0;
      }

      if (use_artifact_pool != 0)
      {
        required_color_mask = 1;
      }
      else
      {
        required_color_mask = color_mask;
      }

      if ((IsCardColorCompatibleWithMask((int)(char)global_cards_data[selected_card_id].color, required_color_mask, 0) != 0) &&
          (GetCardRarity(selected_card_id) <= ((added_count & 1) == 0 ? 2 : 1)) &&
          ((global_cards_data[selected_card_id].expansion & 0xc1) != 0))
      {
        AddCardToDeckSorted(selected_card_id);
      }
      else
      {
        added_count = added_count - 1;
      }
    }
  }

  duplicate_attempt_count = 0;
  for (added_count = 0; creature_count > (int)added_count; added_count = added_count + 1, duplicate_attempt_count = duplicate_attempt_count + 1)
  {
    selected_card_id = PickRandomCardMatchingTypeAndColor(2, color_mask);
    if (((g_shandalar_difficulty <= 3) && ((global_cards_data[selected_card_id].static_ability & 3) != 0)) ||
        ((global_cards_data[selected_card_id].extra_ability & 0x900) != 0))
    {
      added_count = added_count - 1;
      duplicate_attempt_count = duplicate_attempt_count - 1;
    }
    else
    {
      if (duplicate_attempt_count < 1000)
      {
        card_ok = 1 <= GetRemainingAllowedCardCopies(selected_card_id);
      }
      else
      {
        card_ok = 1;
      }

      if ((IsCardColorCompatibleWithMask((int)(char)global_cards_data[selected_card_id].color, color_mask, 0) != 0) &&
          (GetCardRarity(selected_card_id) <= ((added_count & 1) == 0 ? 2 : 1)) &&
          (card_ok && ((global_cards_data[selected_card_id].expansion & 0xc1) != 0)))
      {
        AddCardToDeckSorted(selected_card_id);
      }
      else
      {
        added_count = added_count - 1;
      }
    }
  }

  if (add_rare != 0)
  {
    do
    {
      do
      {
        selected_card_id = PickRandomCardMatchingTypeAndColor(0xe, 1);
      } while (IsCardColorCompatibleWithMask((int)(char)global_cards_data[selected_card_id].color, color_mask, 1) == 0);
    } while ((GetCardRarity(selected_card_id) < 3) ||
             (GetRemainingAllowedCardCopies(selected_card_id) <= 0) ||
             ((g_shandalar_difficulty == 0 && ((global_cards_data[selected_card_id].static_ability & 3) != 0))) ||
             ((global_cards_data[selected_card_id].extra_ability & 0x900) != 0) ||
             ((global_cards_data[selected_card_id].expansion & 0xc1) == 0));
  }

  AddCardToDeckSorted(selected_card_id);
  return 0;
}

// FUNCTION: SHANDALAR 0x0056bcf7
int PickRandomCardMatchingTypeAndColor(unsigned int type_mask, unsigned int color_mask)
{
  int attempt_count;
  int card_index;
  int found;

  attempt_count = 0;
  do
  {
    found = 0;
    card_index = internal_rand(g_card_count - 0x39);
    if ((type_mask != 0) && ((type_mask & global_cards_data[card_index].type) == 0))
    {
    }
    else if ((color_mask != 1) && ((color_mask & (int)(char)global_cards_data[card_index].color) == 0))
    {
    }
    else
    {
      found = 1;
    }
  } while (found == 0 && ++attempt_count < 999);

  return card_index;
}

// FUNCTION: SHANDALAR 0x004bb1cf
int GetRemainingAllowedCardCopies(unsigned int card_id)
{
  int color_index;
  int copies_in_deck;
  int total_non_market_cards;
  int i;
  int max_color_count;
  int color_counts[7];

  if ((int)card_id <= 4)
  {
    return 99;
  }

  for (i = 0; i < 7; i = i + 1)
  {
    color_counts[i] = 0;
  }

  total_non_market_cards = 0;
  copies_in_deck = total_non_market_cards;

  for (i = 0; i < 500; i = i + 1)
  {
    if ((deck[i] != -1) && ((deck[i] & 0x4000) == 0))
    {
      total_non_market_cards = total_non_market_cards + 1;
      color_index = single_color_test_bit_to_color_t(global_cards_data[deck[i] & 0xfff].color);
      color_counts[color_index] = color_counts[color_index] + 1;
    }

    if (((unsigned int)deck[i] & 0xffff7fffU) == card_id)
    {
      copies_in_deck = copies_in_deck + 1;
      g_last_matching_deck_card_index = i;
    }
  }

  max_color_count = -1;
  for (i = 1; i < 7; i = i + 1)
  {
    if (max_color_count < color_counts[i])
    {
      max_color_count = color_counts[i];
    }
  }

  g_deck_color_bitmap = 0;
  for (i = 1; i < 7; i = i + 1)
  {
    if (((max_color_count * 2) / 3) <= color_counts[i])
    {
      g_deck_color_bitmap = g_deck_color_bitmap | (1 << (unsigned char)i);
    }
  }

  max_color_count = 1;
  if (total_non_market_cards >= 0x14)
  {
    max_color_count = 1;
  }
  if (total_non_market_cards >= 0x28)
  {
    max_color_count = 2;
  }
  if (total_non_market_cards >= 0x3c)
  {
    max_color_count = 3;
  }
  if ((g_world_magic_bitmap & 0x20) != 0)
  {
    max_color_count = max_color_count + 1;
  }

  if ((global_cards_data[card_id].extra_ability & 0x100) != 0)
  {
    max_color_count = (g_shandalar_difficulty <= max_color_count);
  }
  else if ((global_cards_data[card_id].expansion & 0xc1) == 0)
  {
    max_color_count = max_color_count / 2;
  }

  return max_color_count - copies_in_deck;
}

// FUNCTION: SHANDALAR 0x004bb458
int FindWorldMagicCardIndex(shandalar_worldmagic_index_t inx)
{
  int entry_index;

  if ((int)inx < 0)
  {
    return -1;
  }

  for (entry_index = 0; entry_index < g_card_count + 0x10; entry_index = entry_index + 1)
  {
    if (global_cards_data[entry_index].id == Scards[inx].worldmagic_csvid)
    {
      return entry_index;
    }
  }
  return -1;
}

// FUNCTION: SHANDALAR 0x004290e2
void AddJournalEntry(int entry_type, int entry_arg)
{
  if (g_journal_entry_count < 2000)
  {
    g_journal_entries[g_journal_entry_count].type = entry_type;
    g_journal_entries[g_journal_entry_count].arg = entry_arg;
    g_journal_entries[g_journal_entry_count].tile_x = g_world_player_x / 32;
    g_journal_entries[g_journal_entry_count].tile_y = g_world_player_y / 32;
    g_journal_entry_count = g_journal_entry_count + 1;
  }
}

// FUNCTION: SHANDALAR 0x0052280c
int UpdateStatWindowProgress(void *progress_values)
{
  return statwin_init((const StatWinData *)progress_values);
}

// FUNCTION: SHANDALAR 0x0056302b
int MapWizardColorToDisplayIndex(int wizard_color)
{
  switch (wizard_color)
  {
  case 1:
    return 4;
  case 2:
    return 2;
  case 3:
    return 3;
  case 4:
    return 1;
  case 5:
    return 0;
  case 6:
    return 5;
  default:
    return -1;
  }
}

// FUNCTION: SHANDALAR 0x0056bff1
void RebuildDeckEntriesByCardGroup(void)
{
  int deck_slot_index;
  int card_index;
  unsigned int saved_deck_entries[500];
  int saved_journal_entry_count;

  for (card_index = 0; card_index < 500; card_index = card_index + 1)
  {
    saved_deck_entries[card_index] = deck[card_index];
    deck[card_index] = -1;
  }
  saved_journal_entry_count = g_journal_entry_count;
  for (card_index = 0; card_index < 500; card_index = card_index + 1)
  {
    if (saved_deck_entries[card_index] != 0xffffffff)
    {
      deck_slot_index = AddCardToDeckSorted(saved_deck_entries[card_index] & 0xfff);
      deck[deck_slot_index] |= ((int)saved_deck_entries[card_index] & -4096);
    }
  }
  g_journal_entry_count = saved_journal_entry_count;
}

// FUNCTION: SHANDALAR 0x004ed0bd
int RecountDeckCardTotals(void)
{
  int deck_index;

  g_deck_active_card_count = g_deck_total_card_count = 0;
  for (deck_index = 0; deck_index < 500; deck_index = deck_index + 1)
  {
    if (deck[deck_index] != -1)
    {
      g_deck_total_card_count = g_deck_total_card_count + 1;
      if ((deck[deck_index] & 0x4000) == 0)
      {
        g_deck_active_card_count++;
      }
    }
  }
  return 0;
}

// FUNCTION: SHANDALAR 0x00559807
int RunRandomAiDuelDemo(void)
{
  int creature_type;
  int ante_index;
  int player;

  for (player = 0; player < 2; player++)
  {
    switch (internal_rand(5))
    {
    case 0:
      ClearAndLoadInitialLibraryFromDeckFile("decks\\0016.dck", player, 1, -1);
      creature_type = 2;
      break;
    case 1:
      ClearAndLoadInitialLibraryFromDeckFile("decks\\0283.dck", player, 1, -1);
      creature_type = 10;
      break;
    case 2:
      ClearAndLoadInitialLibraryFromDeckFile("decks\\0150.dck", player, 1, -1);
      creature_type = 0x10;
      break;
    case 3:
      ClearAndLoadInitialLibraryFromDeckFile("decks\\0076.dck", player, 1, -1);
      creature_type = 0x17;
      break;
    case 4:
      ClearAndLoadInitialLibraryFromDeckFile("decks\\0102.dck", player, 1, -1);
      creature_type = 0x20;
      break;
    }
  }

  g_selected_wizard_color = 0;
  g_opponent_initial_library_index = 1;
  for (ante_index = 0; ante_index < 0x10; ante_index++)
  {
    global_ante_cards[1][ante_index] = -1;
    global_ante_cards[0][ante_index] = global_ante_cards[1][ante_index];
  }

  g_duel_network_state = 1;
  RunDuelEngine(0, creature_type);
  g_duel_network_state = 0;
  RefreshAdventureInterfaceLayout();
  return 0;
}
