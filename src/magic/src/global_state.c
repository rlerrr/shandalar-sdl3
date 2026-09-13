#define GLOBAL_STATE_IMPL
#include "global_state.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/legacy_io.h"
#include "platform/winreg_compat.h"

#include "magic/src/game_support.h"
#include "magic/src/global_duel_ui_ids.h"
#include "magic/src/global_other.h"
#ifdef SHANDALAR
#include "shandalar/src/shandalar_global_strings.h"
#endif

extern card_data_t global_cards_data[];

int CardTypeFromID(int csvid);
int is_card_available_in_installed_expansion(unsigned int csvid);
int save_or_load_data(void *buf, unsigned int count);
#ifdef SHANDALAR
void HideMouseCursorNested(void);
void ShowMouseCursorNested(void);
int GetSaveDriveIndex(void);
int RunTextMenuAt(char *menu_text, int left_x, int top_y);
int LoadGameFromPath(char *save_file_path);
#endif

// GLOBAL: MAGIC 0x005710f8
// GLOBAL: SHANDALAR 0x00588114
static char *s_sealed_deck_registry_path_005710e0 = "Software\\MicroProse\\Magic: The Gathering\\SealedDeckOptions";
// GLOBAL: MAGIC 0x005710ec
// GLOBAL: SHANDALAR 0x00588108
static char *s_solo_options_registry_path_00571284 = "Software\\MicroProse\\Magic: The Gathering\\SoloOptions";
// GLOBAL: MAGIC 0x005710f4
// GLOBAL: SHANDALAR 0x00588110
static char *s_gauntlet_options_registry_path_00571338 = "Software\\MicroProse\\Magic: The Gathering\\GauntletEx1Options";
// GLOBAL: MAGIC 0x00588100
// GLOBAL: SHANDALAR 0x00588100
char *s_duel_options_registry_path_00588100 = "Software\\MicroProse\\Magic: The Gathering\\DuelOptions";

// FUNCTION: MAGIC 0x004a5a1b
// FUNCTION: SHANDALAR 0x00557b9b
void SetCardInDeck(int deck_index, int enabled)
{
  if (enabled == 1)
  {
    deck[deck_index] = deck[deck_index] | 0x4000;
  }
  else
  {
    deck[deck_index] = deck[deck_index] & 0x8fff;
  }
}

// FUNCTION: MAGIC 0x004ed13a
// FUNCTION: SHANDALAR 0x00501b31
int write_save_data(int file_descriptor, void *buffer, unsigned int count)
{
  if (write(file_descriptor, buffer, count) == -1)
  {
    g_save_errno = errno;
    return 0;
  }
  else
  {
    return 1;
  }
}

// FUNCTION: MAGIC 0x004ef073
// FUNCTION: SHANDALAR 0x00504063
unsigned int save_or_load_duel_mode_state(void)
{
  unsigned int result;

  result = 1;
  result &= save_or_load_data(&g_duel_state_008cee6c, 4);
  result &= save_or_load_data(&g_ante_card_count, 4);
  result &= save_or_load_data(&g_creature_count_total, 4);
  result &= save_or_load_data(&g_opponent_deck_color_filter_count, 4);
  result &= save_or_load_data(&g_duel_state_00789104, 4);

  return result;
}

// FUNCTION: MAGIC 0x0048b144
// FUNCTION: SHANDALAR 0x004a6dd4
void __stdcall LoadSoloDuelRegistryOptions(void)
{
  HKEY options_key;
  BYTE value_buffer[0x34];
  DWORD value_size;

  if (RegOpenKeyExA(HKEY_CURRENT_USER, s_solo_options_registry_path_00571284, 0, KEY_QUERY_VALUE,
                    &options_key) == ERROR_SUCCESS)
  {
    value_size = 10;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "Difficulty", NULL, NULL, value_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)value_buffer, "%d", &g_solo_duel_options.difficulty);
    else
      g_solo_duel_options.difficulty = 1;

    value_size = 0x32;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "PlayerDeck", NULL, NULL, value_buffer, &value_size) == ERROR_SUCCESS)
      sprintf(g_solo_duel_options.player_deck, "%s", value_buffer);
    else
      g_solo_duel_options.player_deck[0] = '\0';

    value_size = 10;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "PlayerRandom", NULL, NULL, value_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)value_buffer, "%d", &g_solo_duel_options.player_random);
    else
      g_solo_duel_options.player_random = 1;

    value_size = 0x32;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "OpponentDeck", NULL, NULL, value_buffer, &value_size) == ERROR_SUCCESS)
      sprintf(g_solo_duel_options.opponent_deck, "%s", value_buffer);
    else
      g_solo_duel_options.opponent_deck[0] = '\0';

    value_size = 10;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "OpponentRandom", NULL, NULL, value_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)value_buffer, "%d", &g_solo_duel_options.opponent_random);
    else
      g_solo_duel_options.opponent_random = 1;

    value_size = 10;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "BestOf", NULL, NULL, value_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)value_buffer, "%d", &g_solo_duel_options.best_of);
    else
      g_solo_duel_options.best_of = 1;

    value_size = 10;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "Ante", NULL, NULL, value_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)value_buffer, "%d", &g_solo_duel_options.ante);
    else
      g_solo_duel_options.ante = 1;

    value_size = 10;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "AllowSideboarding", NULL, NULL, value_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)value_buffer, "%d", &g_solo_duel_options.allow_sideboarding);
    else
      g_solo_duel_options.allow_sideboarding = 1;

    RegCloseKey(options_key);
  }
  else
  {
    g_solo_duel_options.difficulty = 1;
    g_solo_duel_options.player_deck[0] = '\0';
    g_solo_duel_options.player_random = 1;
    g_solo_duel_options.opponent_deck[0] = '\0';
    g_solo_duel_options.opponent_random = 1;
    g_solo_duel_options.best_of = 1;
    g_solo_duel_options.ante = 1;
    g_solo_duel_options.allow_sideboarding = 1;
  }
}

// FUNCTION: MAGIC 0x0048b69e
// FUNCTION: SHANDALAR 0x004a732e
void __stdcall LoadGauntletRegistryOptions(void)
{
  HKEY options_key;
  BYTE value_buffer[0x34];
  DWORD value_size;

  if (RegOpenKeyExA(HKEY_CURRENT_USER, s_gauntlet_options_registry_path_00571338, 0, KEY_QUERY_VALUE,
                    &options_key) == ERROR_SUCCESS)
  {
    value_size = 0x32;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "PlayerDeck", NULL, NULL, value_buffer, &value_size) == ERROR_SUCCESS)
      sprintf(g_gauntlet_options.player_deck, "%s", value_buffer);
    else
      g_gauntlet_options.player_deck[0] = '\0';

    value_size = 10;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "PlayerRandom", NULL, NULL, value_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)value_buffer, "%d", &g_gauntlet_options.player_random);
    else
      g_gauntlet_options.player_random = 1;

    value_size = 10;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "BestOf", NULL, NULL, value_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)value_buffer, "%d", &g_gauntlet_options.best_of);
    else
      g_gauntlet_options.best_of = 1;

    value_size = 10;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "Ante", NULL, NULL, value_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)value_buffer, "%d", &g_gauntlet_options.ante);
    else
      g_gauntlet_options.ante = 1;

    value_size = 10;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "AllowSideboarding", NULL, NULL, value_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)value_buffer, "%d", &g_gauntlet_options.allow_sideboarding);
    else
      g_gauntlet_options.allow_sideboarding = 1;

    value_size = 10;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "GauntletLength", NULL, NULL, value_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)value_buffer, "%d", &g_gauntlet_options.gauntlet_length);
    else
      g_gauntlet_options.gauntlet_length = 0x14;

    RegCloseKey(options_key);
  }
  else
  {
    g_gauntlet_options.player_deck[0] = '\0';
    g_gauntlet_options.player_random = 1;
    g_gauntlet_options.best_of = 1;
    g_gauntlet_options.ante = 1;
    g_gauntlet_options.allow_sideboarding = 1;
    g_gauntlet_options.gauntlet_length = 0x14;
  }
}

// FUNCTION: MAGIC 0x0048bace
// FUNCTION: SHANDALAR 0x004a775e
void __stdcall LoadSealedDeckRegistryOptions(void)
{
  int booster_pack_index;
  char *booster_value_scan;
  int starter_pack_index;
  char *starter_value_scan;
  HKEY options_key;
  char value_buffer[0x34];
  DWORD value_size;

  if (RegOpenKeyExA(HKEY_CURRENT_USER, s_sealed_deck_registry_path_005710e0, 0, KEY_QUERY_VALUE,
                    &options_key) == ERROR_SUCCESS)
  {
    value_size = 10;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "Rounds", NULL, NULL, (BYTE *)value_buffer, &value_size) == ERROR_SUCCESS)
      sscanf(value_buffer, "%d", &g_sealed_deck_options.rounds);
    else
      g_sealed_deck_options.rounds = 5;

    value_size = 0x32;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "Starters", NULL, NULL, (BYTE *)value_buffer, &value_size) == ERROR_SUCCESS)
    {
      sscanf(value_buffer, "%d", &g_sealed_deck_options.starter_count);
      starter_value_scan = value_buffer;
      starter_pack_index = 0;
      while (starter_value_scan != NULL && *starter_value_scan != '\0' &&
             !(g_sealed_deck_options.starter_count <= starter_pack_index))
      {
        while (*starter_value_scan >= '0' && *starter_value_scan <= '9')
          starter_value_scan++;
        while (*starter_value_scan == ' ' || *starter_value_scan == ',')
          starter_value_scan++;
        if (*starter_value_scan != '\0')
        {
          sscanf(starter_value_scan, "%d", &g_sealed_deck_options.starter_packs[starter_pack_index++]);
        }
      }
      for (; !(g_sealed_deck_options.starter_count <= starter_pack_index); starter_pack_index++)
        g_sealed_deck_options.starter_packs[starter_pack_index] = 2;
    }
    else
    {
      g_sealed_deck_options.starter_count = 2;
      g_sealed_deck_options.starter_packs[0] = 2;
      g_sealed_deck_options.starter_packs[1] = 2;
    }

    value_size = 0x32;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "Boosters", NULL, NULL, (BYTE *)value_buffer, &value_size) == ERROR_SUCCESS)
    {
      sscanf(value_buffer, "%d", &g_sealed_deck_options.booster_count);
      booster_value_scan = value_buffer;
      booster_pack_index = 0;
      while (booster_value_scan != NULL && *booster_value_scan != '\0' &&
             !(g_sealed_deck_options.booster_count <= booster_pack_index))
      {
        while (*booster_value_scan >= '0' && *booster_value_scan <= '9')
          booster_value_scan++;
        while (*booster_value_scan == ' ' || *booster_value_scan == ',')
          booster_value_scan++;
        if (*booster_value_scan != '\0')
        {
          sscanf(booster_value_scan, "%d", &g_sealed_deck_options.booster_packs[booster_pack_index++]);
        }
      }
      for (; !(g_sealed_deck_options.booster_count <= booster_pack_index); booster_pack_index++)
        g_sealed_deck_options.booster_packs[booster_pack_index] = 2;
    }
    else
    {
      g_sealed_deck_options.booster_count = 2;
      g_sealed_deck_options.booster_packs[0] = 2;
      g_sealed_deck_options.booster_packs[1] = 2;
    }

    value_size = 10;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "FreeLands", NULL, NULL, (BYTE *)value_buffer, &value_size) == ERROR_SUCCESS)
      sscanf(value_buffer, "%d", &g_sealed_deck_options.free_lands);
    else
      g_sealed_deck_options.free_lands = 5;

    value_size = 10;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "BestOf", NULL, NULL, (BYTE *)value_buffer, &value_size) == ERROR_SUCCESS)
      sscanf(value_buffer, "%d", &g_sealed_deck_options.best_of);
    else
      g_sealed_deck_options.best_of = 3;

    value_size = 10;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "Ante", NULL, NULL, (BYTE *)value_buffer, &value_size) == ERROR_SUCCESS)
      sscanf(value_buffer, "%d", &g_sealed_deck_options.ante);
    else
      g_sealed_deck_options.ante = 0;

    value_size = 10;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "MinDeckSize", NULL, NULL, (BYTE *)value_buffer, &value_size) == ERROR_SUCCESS)
      sscanf(value_buffer, "%d", &g_sealed_deck_options.minimum_deck_size);
    else
      g_sealed_deck_options.minimum_deck_size = 0x28;

    RegCloseKey(options_key);
  }
  else
  {
    g_sealed_deck_options.rounds = 5;
    g_sealed_deck_options.starter_count = 1;
    g_sealed_deck_options.starter_packs[0] = 2;
    g_sealed_deck_options.booster_count = 2;
    g_sealed_deck_options.booster_packs[0] = 2;
    g_sealed_deck_options.booster_packs[1] = 2;
    g_sealed_deck_options.free_lands = 5;
    g_sealed_deck_options.best_of = 3;
    g_sealed_deck_options.ante = 0;
    g_sealed_deck_options.minimum_deck_size = 0x28;
  }
}

// FUNCTION: MAGIC 0x0048a8ee
// FUNCTION: SHANDALAR 0x004a657e
void save_duel_interface_options_to_registry(void)
{
  BYTE *phase_stop_text;
  BYTE phase_stoppers[100];
  int step;
  int player;
  HKEY options_key;
  BYTE value_buffer[12];
  DWORD disposition;

  if (RegCreateKeyExA(HKEY_CURRENT_USER, s_duel_options_registry_path_00588100, 0, NULL, 0, 0xf003f,
                      NULL, &options_key, &disposition) == ERROR_SUCCESS)
  {
    wsprintfA((char *)value_buffer, "%d", g_duel_interface_options.layout);
    RegSetValueExA(options_key, "Layout", 0, REG_SZ, value_buffer, strlen((char *)value_buffer) + 1);

    wsprintfA((char *)value_buffer, "%d", g_duel_interface_options.directive_tracks_mouse);
    RegSetValueExA(options_key, "DirectiveTracksMouse", 0, REG_SZ, value_buffer, strlen((char *)value_buffer) + 1);

    wsprintfA((char *)value_buffer, "%d", g_duel_interface_options.show_coin_flips);
    RegSetValueExA(options_key, "ShowCoinFlips", 0, REG_SZ, value_buffer, strlen((char *)value_buffer) + 1);

    wsprintfA((char *)value_buffer, "%d", g_duel_interface_options.show_cue_cards);
    RegSetValueExA(options_key, "ShowCueCards", 0, REG_SZ, value_buffer, strlen((char *)value_buffer) + 1);

    wsprintfA((char *)value_buffer, "%d", g_duel_interface_options.show_power_toughness_on_cards);
    RegSetValueExA(options_key, "ShowPowerToughnessOnCards", 0, REG_SZ, value_buffer,
                   strlen((char *)value_buffer) + 1);

    wsprintfA((char *)value_buffer, "%d", g_duel_interface_options.show_abilities_on_cards);
    RegSetValueExA(options_key, "ShowAbilitiesOnCards", 0, REG_SZ, value_buffer, strlen((char *)value_buffer) + 1);

    wsprintfA((char *)value_buffer, "%d", g_duel_interface_options.show_id_tags_on_cards);
    RegSetValueExA(options_key, "ShowIDTagsOnCards", 0, REG_SZ, value_buffer, strlen((char *)value_buffer) + 1);

    wsprintfA((char *)value_buffer, "%d", g_duel_interface_options.show_invisible_effect_cards);
    RegSetValueExA(options_key, "ShowInvisibleEffectCards", 0, REG_SZ, value_buffer,
                   strlen((char *)value_buffer) + 1);

    wsprintfA((char *)value_buffer, "%d", g_duel_interface_options.show_all_cards_summon_sickness);
    RegSetValueExA(options_key, "ShowAllCardsSummonSickness", 0, REG_SZ, value_buffer,
                   strlen((char *)value_buffer) + 1);

    wsprintfA((char *)value_buffer, "%d", g_duel_interface_options.expand_text_box_on_big_card);
    RegSetValueExA(options_key, "ExpandTextBoxOnBigCard", 0, REG_SZ, value_buffer,
                   strlen((char *)value_buffer) + 1);

    wsprintfA((char *)value_buffer, "%d", g_duel_interface_options.see_next_draws_at_end_of_duel);
    RegSetValueExA(options_key, "SeeNextDrawsAtEndOfDuel", 0, REG_SZ, value_buffer,
                   strlen((char *)value_buffer) + 1);

    phase_stop_text = phase_stoppers;
    for (player = 0; player < 2; player++)
    {
      for (step = 0; step < 0x25; step++)
      {
        if (((int)(char)g_duel_phase_stop_settings[player].phase_flags[step] & PHASE_STOP_ENABLED) != 0)
        {
          *phase_stop_text = 'S';
          phase_stop_text++;
        }
        else
        {
          *phase_stop_text = '-';
          phase_stop_text++;
        }
      }
    }
    *phase_stop_text = '\0';
    RegSetValueExA(options_key, "PhaseStoppers", 0, REG_SZ, phase_stoppers,
                   strlen((char *)phase_stoppers) + 1);

    if ((g_duel_mode_flags & 1) == 0)
    {
      wsprintfA((char *)value_buffer, "%d", g_duel_interface_options.player_territory_color);
      RegSetValueExA(options_key, "PlayerTerritoryColor", 0, REG_SZ, value_buffer,
                     strlen((char *)value_buffer) + 1);
    }

    if ((g_duel_mode_flags & 1) == 0)
    {
      wsprintfA((char *)value_buffer, "%d", g_duel_interface_options.player_territory_type);
      RegSetValueExA(options_key, "PlayerTerritoryType", 0, REG_SZ, value_buffer,
                     strlen((char *)value_buffer) + 1);
    }

    RegFlushKey(options_key);
    RegCloseKey(options_key);
  }
}

// FUNCTION: MAGIC 0x004ee0dc
// FUNCTION: SHANDALAR 0x005030bd
int save_or_load_data(void *buf, unsigned int count)
{
  int ok;

  ok = 1;
  if (global_saveload_loading)
  {
    if ((unsigned int)_read(g_save_file_fd, buf, count) != count)
    {
      ok = 0;
    }
  }
  else
  {
    ok = write_save_data(g_save_file_fd, buf, count);
  }

  if (ok == 0)
  {
    OutputDebugStringA("SHIT\n");
  }

  return ok;
}

// FUNCTION: MAGIC 0x004ed32e
// FUNCTION: SHANDALAR 0x005022fd
unsigned int save_or_load_ver1(void)
{
  unsigned int result;
  int saved_world_player_x;
  int saved_world_player_y;

  result = 1;
  result &= save_or_load_data((char *)global_cards_data + g_card_count * 0x48, 0x480);
  result &= save_or_load_data(g_initial_library, 0xc80);
  result &= save_or_load_data(&g_target_player_choice, 4);
  result &= save_or_load_data(&g_recorded_action_controller, 4);
  result &= save_or_load_data(&g_recorded_action_phase, 4);
  result &= save_or_load_data(&g_stop_phase, 4);
  result &= save_or_load_data(&g_stop_phase_player, 4);
  result &= save_or_load_data(&g_recorded_action_player, 4);
  result &= save_or_load_data(&g_duel_state_007161cc, 4);
  result &= save_or_load_data(&g_phase_stop_suppressed, 4);
  result &= save_or_load_data(&g_ai_action_replay_available, 4);
  result &= save_or_load_data(&g_duel_state_0093f4b0, 4);
  result &= save_or_load_data(&g_response_card_type_mask, 4);
  result &= save_or_load_data(&g_response_action_mask, 4);
  result &= save_or_load_data(&g_response_selection_in_progress, 4);
  result &= save_or_load_data(&g_attack_action_available, 4);
  result &= save_or_load_data(&g_duel_state_007161c8, 4);
  result &= save_or_load_data(&g_response_action_selected, 4);
  result &= save_or_load_data(&g_response_window_serial, 4);
  result &= save_or_load_data(&g_event_player, 4);
  result &= save_or_load_data(&g_response_window_depth, 4);
  result &= save_or_load_data(g_special_mana_pool, 0x40);
  result &= save_or_load_data(g_raw_mana_available_hex, 0x198);
  result &= save_or_load_data(g_mana_color_conversions, 0x58);
  result &= save_or_load_data(g_mana_pool_colorless_components, 8);
  result &= save_or_load_data(g_raw_mana_available, 0x40);
  result &= save_or_load_data(g_basiclandtypes_controlled, 0x40);
  result &= save_or_load_data(&g_next_duel_life_delta, 4);
  result &= save_or_load_data(&g_player_starting_life, 4);
  result &= save_or_load_data(&g_one_deck_mode, 4);
  result &= save_or_load_data(&g_duel_active, 4);
  result &= save_or_load_data(&g_hand_count_adjustment, 4);
  result &= save_or_load_data(&g_encounter_opening_hand_size_modifier, 4);
  result &= save_or_load_data(&g_duel_state_00742fc0, 4);
  result &= save_or_load_data(&deck, 2000);
  result &= save_or_load_data(global_card_instances, 0x161e8);
  result &= save_or_load_data(g_damage_accumulators, 0x25c0);
  result &= save_or_load_data(global_graveyard_slots, 4000);
  result &= save_or_load_data(global_exile, 4000);
  result &= save_or_load_data(global_library, 4000);
  result &= save_or_load_data(global_ante_cards, 0x80);
  result &= save_or_load_data(&g_duel_state_0093a848, 4);
  result &= save_or_load_data(&g_duel_extra_turn_player, 4);
  result &= save_or_load_data(&g_ai_recorded_choice, 4);
  result &= save_or_load_data(&g_ai_recorded_action, 4);
  result &= save_or_load_data(&g_duel_summary, 0x60);
  result &= save_or_load_data(&g_player_duel_state_00789498, 8);
  result &= save_or_load_data(g_mana_charge, 0x20);
  result &= save_or_load_data(&g_duel_state_0091b2c4, 4);
  result &= save_or_load_data(&g_ai_combat_value_weights, 0x10);
  result &= save_or_load_data(&g_spell_fizzled, 4);
  result &= save_or_load_data(g_active_cards_count, 8);
  result &= save_or_load_data(&g_ai_search_flags, 4);
  result &= save_or_load_data(&g_duel_message_text, 100);
  result &= save_or_load_data(g_life, 8);
  result &= save_or_load_data(g_poison_counters, 8);
  result &= save_or_load_data(&g_player_duel_state_00938e38, 8);
  result &= save_or_load_data(g_global_stats_bonus, 0x10);
  result &= save_or_load_data(&g_current_player, 4);
  result &= save_or_load_data(&g_current_encounter_color, 4);
  result &= save_or_load_data(&g_ai_modifier, 4);
  result &= save_or_load_data(&g_current_phase, 4);
  result &= save_or_load_data(&g_land_can_be_played, 4);
  result &= save_or_load_data(&g_attacking_creature_count, 4);
  result &= save_or_load_data(&g_response_processing_flags, 4);
  result &= save_or_load_data(&g_encounter_color_match_mode, 4);
  result &= save_or_load_data(&g_current_encounter_strength, 4);
  result &= save_or_load_data(&g_x_value, 4);
  result &= save_or_load_data(&g_max_x_value, 4);
  result &= save_or_load_data(&g_active_player, 4);
  result &= save_or_load_data(&g_other_player, 4);
  result &= save_or_load_data(&g_affected_card_controller, 4);
  result &= save_or_load_data(&g_affected_card, 4);
  result &= save_or_load_data(&g_affected_internal_card_id, 4);
  result &= save_or_load_data(&g_affected_card_color, 4);
  result &= save_or_load_data(&g_attacking_card_controller, 4);
  result &= save_or_load_data(&g_attacking_card, 4);
  result &= save_or_load_data(&g_event_result, 4);
  result &= save_or_load_data(&g_ai_search_force_pass, 4);
  result &= save_or_load_data(&g_ai_search_target_depth, 4);
  result &= save_or_load_data(&g_current_casting_player, 4);
  result &= save_or_load_data(&g_current_casting_internal_card_id, 4);
  result &= save_or_load_data(&g_duel_state_00925d30, 4);
  result &= save_or_load_data(&g_produced_mana_card, 4);
  result &= save_or_load_data(&g_produced_mana_color_valid, 4);
  result &= save_or_load_data(&g_produced_mana_color, 4);
  result &= save_or_load_data(&g_current_spell_player, 4);
  result &= save_or_load_data(&g_current_spell_card, 4);
  result &= save_or_load_data(&g_duel_state_008cfd20, 4);
  result &= save_or_load_data(&g_trigger_condition, 4);
  result &= save_or_load_data(&g_trigger_cause_controller, 4);
  result &= save_or_load_data(&g_trigger_cause, 4);
  result &= save_or_load_data(&g_card_on_stack_controller, 4);
  result &= save_or_load_data(&g_card_on_stack, 4);
  result &= save_or_load_data(&g_current_turn, 4);
  result &= save_or_load_data(&g_current_action_event_code, 4);
  result &= save_or_load_data(&g_opponent_starting_card_id_2, 4);
  result &= save_or_load_data(&g_opponent_starting_card_id_1, 4);
  result &= save_or_load_data(&g_duel_state_00925ac4, 4);
  result &= save_or_load_data(&g_ai_action_score_table, 0x40);
  result &= save_or_load_data(&g_duel_state_008a8de4, 4);
  result &= save_or_load_data(g_ai_side_scores, 8);
  result &= save_or_load_data(&g_nested_trigger_stack, 0x40);
  result &= save_or_load_data(&g_ai_search_best_score, 4);
  result &= save_or_load_data(&g_current_spell_state, 4);
  result &= save_or_load_data(&g_duel_state_0077784c, 4);
  result &= save_or_load_data(&g_recorded_action_cursor, 4);
  result &= save_or_load_data(&g_activation_dispatch_flags, 4);
  result &= save_or_load_data(&g_duel_state_current_card, 4);
  result &= save_or_load_data(&g_ai_search_result, 4);
  result &= save_or_load_data(&g_adventure_world_state, 4);
  result &= save_or_load_data(g_creature_power_by_color, 0x40);
  result &= save_or_load_data(g_creature_toughness_by_color, 0x40);
  result &= save_or_load_data(g_ai_mana_demand_by_color, 0x40);
  result &= save_or_load_data(&g_card_types_in_play, 8);
  result &= save_or_load_data(&g_creature_count_summary, 8);
  result &= save_or_load_data(&g_duel_state_008b2880, 4);
  result &= save_or_load_data(&g_ai_search_time_limit, 4);
  result &= save_or_load_data(&g_ai_decision_code, 4);
  result &= save_or_load_data(g_graveyard_card_types, 8);
  result &= save_or_load_data(&g_opponent_deck_color_filter_by_color, 0x30);
  result &= save_or_load_data(&g_duel_initialized, 4);
  result &= save_or_load_data(&g_required_mana_color_mask, 4);
  result &= save_or_load_data(&g_first_turn_draw_pending, 4);
  result &= save_or_load_data(&g_starting_player_was_random, 4);
  result &= save_or_load_data(g_player_special_effect_flags, 8);
  result &= save_or_load_data(g_stack_data, 0x80);
  result &= save_or_load_data(global_stack_cards, 0x100);
  result &= save_or_load_data(global_stack_damage_targets, 0x100);
  result &= save_or_load_data(g_stack_trigger_condition, 0x80);
  result &= save_or_load_data(g_stack_prompt_flags, 0x80);
  result &= save_or_load_data(&g_stack_size, 4);
  result &= save_or_load_data(&g_activation_event_flags, 4);
  result &= save_or_load_data(&g_ai_search_retry_state, 4);
  result &= save_or_load_data(&g_opponent_starting_card_id_3, 4);
  result &= save_or_load_data(&g_last_single_card_event_result, 4);
  result &= save_or_load_data(&g_ai_decision_parameter, 4);
  result &= save_or_load_data(&g_upkeep_payment_completed, 4);
  result &= save_or_load_data(&g_ai_search_try_count, 4);
  result &= save_or_load_data(TENTATIVE_timestamp_player, 2000);
  result &= save_or_load_data(TENTATIVE_timestamp_card, 2000);
  result &= save_or_load_data(&g_ai_speculation_depth, 4);
  result &= save_or_load_data(&g_duel_state_0091c4fc, 4);
  result &= save_or_load_data(&g_combat_assignment_cancelled, 4);
  result &= save_or_load_data(&g_trigger_dispatch_depth, 4);
  result &= save_or_load_data(&g_max_trigger_dispatch_depth, 4);
  result &= save_or_load_data(&g_nested_trigger_depth, 4);
  result &= save_or_load_data(&g_response_action_taken, 4);
  result &= save_or_load_data(&g_opponent_initial_library_index, 4);
  result &= save_or_load_data(&g_selected_wizard_color, 4);
  result &= save_or_load_data(&g_deck_color_bitmap, 4);
  result &= save_or_load_data(&g_shandalar_difficulty, 4);
  result &= save_or_load_data(&g_skip_new_game_state_initialization, 4);
  result &= save_or_load_data(&g_lair_or_monster_slots, 0xc0);
  // The on-disk format stores integral world coordinates. Keep that layout so
  // old saves remain readable while runtime movement uses subpixel positions.
  saved_world_player_x = (int)(g_world_player_x + 0.5);
  saved_world_player_y = (int)(g_world_player_y + 0.5);
  result &= save_or_load_data(&saved_world_player_x, 4);
  result &= save_or_load_data(&saved_world_player_y, 4);
  if (global_saveload_loading)
  {
    g_world_player_x = saved_world_player_x;
    g_world_player_y = saved_world_player_y;
  }
  result &= save_or_load_data(&g_monster_timer, 4);
  result &= save_or_load_data(&g_town_slots, 0x3200);
  result &= save_or_load_data(&g_current_quest_type, 4);
  result &= save_or_load_data(&g_current_quest_destination, 4);
  result &= save_or_load_data(&g_current_quest_color, 4);
  result &= save_or_load_data(&g_current_quest_data, 4);
  result &= save_or_load_data(&g_current_quest_deadline, 4);
  result &= save_or_load_data(&g_amulet_inventory, 0x14);
  result &= save_or_load_data(&Gold, 4);
  result &= save_or_load_data(&g_food, 4);
  result &= save_or_load_data(&g_duel_victory_log, 1000);
  result &= save_or_load_data(&g_world_magic_bitmap, 4);
  result &= save_or_load_data(&Scards, 0xc0);
  result &= save_or_load_data(&g_defeated_wizards_bitmap, 4);
  result &= save_or_load_data(&g_castle_dungeon_slots, 0x2d0);
  result &= save_or_load_data(&g_siege_timer, 4);
  result &= save_or_load_data(&g_lairs_explored, 4);
  result &= save_or_load_data(&g_quest_restock_timer, 4);
  result &= save_or_load_data(&g_player_is_male, 4);
  result &= save_or_load_data(&g_starting_color, 4);
  result &= save_or_load_data(&g_next_duel_life_delta, 4);
  result &= save_or_load_data(&g_next_duel_card_id, 4);
  result &= save_or_load_data(&_currentDeck, 4);
  result &= save_or_load_data(&g_journal_entries, 32000);
  result &= save_or_load_data(&g_journal_entry_count, 4);
  result &= save_or_load_data(&g_duel_interface_options.player_territory_color, 4);
  result &= save_or_load_data(&g_duel_interface_options.player_territory_type, 4);
  result &= save_or_load_data(&g_last_duel_player_primary_color, 4);
  result &= save_or_load_data(&g_last_duel_enemy_primary_color, 4);
  result &= save_or_load_data(&g_last_duel_result_state, 4);
  result &= save_or_load_data(&g_name_entry_buffer, 0x40);
#ifdef SHANDALAR
  strcpy(g_player_name, g_name_entry_buffer);
#endif
  result &= save_or_load_data(&g_siege_indicator, 4);

  return result;
}

// FUNCTION: MAGIC 0x004ee3cb
// FUNCTION: SHANDALAR 0x005033bb
unsigned int save_or_load_ver2(void)
{
  unsigned int result;
  int card_index;
  int card_type;

  result = 1;
  result &= save_or_load_data((char *)global_cards_data + g_card_count * 0x48, 0x480);
  result &= save_or_load_data(g_initial_library, 0xc80);
  result &= save_or_load_data(&g_target_player_choice, 4);
  result &= save_or_load_data(&g_recorded_action_controller, 4);
  result &= save_or_load_data(&g_recorded_action_phase, 4);
  result &= save_or_load_data(&g_stop_phase, 4);
  result &= save_or_load_data(&g_stop_phase_player, 4);
  result &= save_or_load_data(&g_recorded_action_player, 4);
  result &= save_or_load_data(&g_duel_state_007161cc, 4);
  result &= save_or_load_data(&g_phase_stop_suppressed, 4);
  result &= save_or_load_data(&g_ai_action_replay_available, 4);
  result &= save_or_load_data(&g_duel_state_0093f4b0, 4);
  result &= save_or_load_data(&g_response_card_type_mask, 4);
  result &= save_or_load_data(&g_response_action_mask, 4);
  result &= save_or_load_data(&g_response_selection_in_progress, 4);
  result &= save_or_load_data(&g_attack_action_available, 4);
  result &= save_or_load_data(&g_duel_state_007161c8, 4);
  result &= save_or_load_data(&g_response_action_selected, 4);
  result &= save_or_load_data(&g_response_window_serial, 4);
  result &= save_or_load_data(&g_event_player, 4);
  result &= save_or_load_data(&g_response_window_depth, 4);
  result &= save_or_load_data(g_special_mana_pool, 0x40);
  result &= save_or_load_data(g_raw_mana_available_hex, 0x198);
  result &= save_or_load_data(g_mana_color_conversions, 0x58);
  result &= save_or_load_data(g_mana_pool_colorless_components, 8);
  result &= save_or_load_data(g_raw_mana_available, 0x40);
  result &= save_or_load_data(g_basiclandtypes_controlled, 0x40);
  result &= save_or_load_data(&g_next_duel_life_delta, 4);
  result &= save_or_load_data(&g_player_starting_life, 4);
  result &= save_or_load_data(&g_one_deck_mode, 4);
  result &= save_or_load_data(&g_duel_active, 4);
  result &= save_or_load_data(&g_hand_count_adjustment, 4);
  result &= save_or_load_data(&g_encounter_opening_hand_size_modifier, 4);
  result &= save_or_load_data(&g_duel_state_00742fc0, 4);
  result &= save_or_load_data(&deck, 2000);
  result &= save_or_load_data(global_card_instances, 0x161e8);
  result &= save_or_load_data(g_damage_accumulators, 0x25c0);
  result &= save_or_load_data(global_graveyard_slots, 4000);
  result &= save_or_load_data(global_exile, 4000);
  result &= save_or_load_data(global_library, 4000);
  result &= save_or_load_data(global_ante_cards, 0x80);
  result &= save_or_load_data(&g_duel_state_0093a848, 4);
  result &= save_or_load_data(&g_duel_extra_turn_player, 4);
  result &= save_or_load_data(&g_ai_recorded_choice, 4);
  result &= save_or_load_data(&g_ai_recorded_action, 4);
  result &= save_or_load_data(&g_duel_summary, 0x60);
  result &= save_or_load_data(&g_player_duel_state_00789498, 8);
  result &= save_or_load_data(g_mana_charge, 0x20);
  result &= save_or_load_data(&g_duel_state_0091b2c4, 4);
  result &= save_or_load_data(&g_ai_combat_value_weights, 0x10);
  result &= save_or_load_data(&g_spell_fizzled, 4);
  result &= save_or_load_data(g_active_cards_count, 8);
  result &= save_or_load_data(&g_ai_search_flags, 4);
  result &= save_or_load_data(&g_duel_message_text, 100);
  result &= save_or_load_data(g_life, 8);
  result &= save_or_load_data(g_poison_counters, 8);
  result &= save_or_load_data(g_lich_active, 8);
  result &= save_or_load_data(&g_player_duel_state_00938e38, 8);
  result &= save_or_load_data(g_global_stats_bonus, 0x10);
  result &= save_or_load_data(&g_current_player, 4);
  result &= save_or_load_data(&g_current_encounter_color, 4);
  result &= save_or_load_data(&g_ai_modifier, 4);
  result &= save_or_load_data(&g_current_phase, 4);
  result &= save_or_load_data(&g_land_can_be_played, 4);
  result &= save_or_load_data(&g_attacking_creature_count, 4);
  result &= save_or_load_data(&g_response_processing_flags, 4);
  result &= save_or_load_data(&g_encounter_color_match_mode, 4);
  result &= save_or_load_data(&g_current_encounter_strength, 4);
  result &= save_or_load_data(&g_x_value, 4);
  result &= save_or_load_data(&g_max_x_value, 4);
  result &= save_or_load_data(&g_active_player, 4);
  result &= save_or_load_data(&g_other_player, 4);
  result &= save_or_load_data(&g_affected_card_controller, 4);
  result &= save_or_load_data(&g_affected_card, 4);
  result &= save_or_load_data(&g_affected_internal_card_id, 4);
  result &= save_or_load_data(&g_affected_card_color, 4);
  result &= save_or_load_data(&g_attacking_card_controller, 4);
  result &= save_or_load_data(&g_attacking_card, 4);
  result &= save_or_load_data(&g_event_result, 4);
  result &= save_or_load_data(&g_ai_search_force_pass, 4);
  result &= save_or_load_data(&g_ai_search_target_depth, 4);
  result &= save_or_load_data(&g_current_casting_player, 4);
  result &= save_or_load_data(&g_current_casting_internal_card_id, 4);
  result &= save_or_load_data(&g_duel_state_00925d30, 4);
  result &= save_or_load_data(&g_produced_mana_card, 4);
  result &= save_or_load_data(&g_produced_mana_color_valid, 4);
  result &= save_or_load_data(&g_produced_mana_color, 4);
  result &= save_or_load_data(&g_current_spell_player, 4);
  result &= save_or_load_data(&g_current_spell_card, 4);
  result &= save_or_load_data(&g_duel_state_008cfd20, 4);
  result &= save_or_load_data(&g_trigger_condition, 4);
  result &= save_or_load_data(&g_trigger_cause_controller, 4);
  result &= save_or_load_data(&g_trigger_cause, 4);
  result &= save_or_load_data(&g_card_on_stack_controller, 4);
  result &= save_or_load_data(&g_card_on_stack, 4);
  result &= save_or_load_data(&g_current_turn, 4);
  result &= save_or_load_data(&g_current_action_event_code, 4);
  result &= save_or_load_data(&g_opponent_starting_card_id_2, 4);
  result &= save_or_load_data(&g_opponent_starting_card_id_1, 4);
  result &= save_or_load_data(&g_duel_state_00925ac4, 4);
  result &= save_or_load_data(&g_ai_action_score_table, 0x40);
  result &= save_or_load_data(&g_duel_state_008a8de4, 4);
  result &= save_or_load_data(g_ai_side_scores, 8);
  result &= save_or_load_data(&g_nested_trigger_stack, 0x40);
  result &= save_or_load_data(&g_ai_search_best_score, 4);
  result &= save_or_load_data(&g_current_spell_state, 4);
  result &= save_or_load_data(&g_duel_state_0077784c, 4);
  result &= save_or_load_data(&g_recorded_action_cursor, 4);
  result &= save_or_load_data(&g_activation_dispatch_flags, 4);
  result &= save_or_load_data(&g_duel_state_current_card, 4);
  result &= save_or_load_data(&g_ai_search_result, 4);
  result &= save_or_load_data(&g_adventure_world_state, 4);
  result &= save_or_load_data(g_creature_power_by_color, 0x40);
  result &= save_or_load_data(g_creature_toughness_by_color, 0x40);
  result &= save_or_load_data(g_ai_mana_demand_by_color, 0x40);
  result &= save_or_load_data(&g_card_types_in_play, 8);
  result &= save_or_load_data(&g_creature_count_summary, 8);
  result &= save_or_load_data(&g_duel_state_008b2880, 4);
  result &= save_or_load_data(&g_ai_search_time_limit, 4);
  result &= save_or_load_data(&g_ai_decision_code, 4);
  result &= save_or_load_data(g_graveyard_card_types, 8);
  result &= save_or_load_data(&g_opponent_deck_color_filter_by_color, 0x30);
  result &= save_or_load_data(&g_duel_initialized, 4);
  result &= save_or_load_data(&g_required_mana_color_mask, 4);
  result &= save_or_load_data(&g_first_turn_draw_pending, 4);
  result &= save_or_load_data(&g_starting_player_was_random, 4);
  result &= save_or_load_data(g_player_special_effect_flags, 8);
  result &= save_or_load_data(g_stack_data, 0x80);
  result &= save_or_load_data(global_stack_cards, 0x100);
  result &= save_or_load_data(global_stack_damage_targets, 0x100);
  result &= save_or_load_data(g_stack_trigger_condition, 0x80);
  result &= save_or_load_data(g_stack_prompt_flags, 0x80);
  result &= save_or_load_data(&g_stack_size, 4);
  result &= save_or_load_data(&g_activation_event_flags, 4);
  result &= save_or_load_data(&g_ai_search_retry_state, 4);
  result &= save_or_load_data(&g_opponent_starting_card_id_3, 4);
  result &= save_or_load_data(&g_last_single_card_event_result, 4);
  result &= save_or_load_data(&g_ai_decision_parameter, 4);
  result &= save_or_load_data(&g_upkeep_payment_completed, 4);
  result &= save_or_load_data(&g_ai_search_try_count, 4);
  result &= save_or_load_data(TENTATIVE_timestamp_player, 2000);
  result &= save_or_load_data(TENTATIVE_timestamp_card, 2000);
  result &= save_or_load_data(&g_ai_speculation_depth, 4);
  result &= save_or_load_data(&g_duel_state_0091c4fc, 4);
  result &= save_or_load_data(&g_combat_assignment_cancelled, 4);
  result &= save_or_load_data(&g_trigger_dispatch_depth, 4);
  result &= save_or_load_data(&g_max_trigger_dispatch_depth, 4);
  result &= save_or_load_data(&g_nested_trigger_depth, 4);
  result &= save_or_load_data(&g_response_action_taken, 4);
  result &= save_or_load_data(&g_opponent_initial_library_index, 4);
  result &= save_or_load_data(&g_selected_wizard_color, 4);
  result &= save_or_load_data(&g_deck_color_bitmap, 4);
  result &= save_or_load_data(&g_shandalar_difficulty, 4);
  result &= save_or_load_data(&g_skip_new_game_state_initialization, 4);
  result &= save_or_load_data(&g_duel_interface_options.player_territory_color, 4);
  result &= save_or_load_data(&g_duel_interface_options.player_territory_type, 4);
  result &= save_or_load_data(&g_last_duel_player_primary_color, 4);
  result &= save_or_load_data(&g_last_duel_enemy_primary_color, 4);
  result &= save_or_load_data(&g_last_duel_result_state, 4);
  result &= save_or_load_data(&_PlayerFace, 4);
  result &= save_or_load_data(&_OpponFace, 4);
  result &= save_or_load_data(&g_duel_state_block_008ce570, 0x104);
  result &= save_or_load_data(&g_duel_state_block_008b3fc0, 0x104);
  result &= save_or_load_data(g_saved_player_name, 0x32);
  result &= save_or_load_data(&g_player_name, 0x32);
  result &= save_or_load_data(&g_opponent_deck_cards, 0x640);
  result &= save_or_load_data(&g_player_deck_cards, 0x640);

  if (global_saveload_loading == 1)
  {
    for (card_index = g_card_count; g_card_count + 0x10 > card_index; ++card_index)
    {
      if (global_cards_data[card_index].id != -1)
      {
        card_type = CardTypeFromID(global_cards_data[card_index].id);
        global_cards_data[card_index].code_pointer = global_cards_data[card_type].code_pointer;
      }
    }
  }

  return result;
}

// FUNCTION: MAGIC 0x004ee1d1
// FUNCTION: SHANDALAR 0x005031c2
void save_gametype0(char *path)
{
  g_save_file_fd = _open(path, 0x8301, 0x80);
  if (g_save_file_fd != -1)
  {
    global_saveload_loading = 0;
    save_or_load_data(&g_save_format_version, 4);
    save_or_load_ver1();
    save_or_load_data(&_PlayerFace, 4);
    save_or_load_data(&_OpponFace, 4);
    save_or_load_data(g_saved_player_name, 0x32);
    save_or_load_data(&g_player_name, 0x32);
    _close(g_save_file_fd);
  }
}

// FUNCTION: MAGIC 0x004c09b0
// FUNCTION: SHANDALAR 0x004ece40
int int_to_hex_digit(int value)
{
  if (value >= 0 && value <= 9)
    return value + 0x30;
  else if (value >= 10 && value <= 0xf)
    return value + 0x57;
  else
    return 0;
}

// FUNCTION: MAGIC 0x004ed186
int get_duel_save_slot_source(void)
{
  return g_save_path_needs_init;
}

// FUNCTION: MAGIC 0x004ed1ad
unsigned int load_duel_save_file_ver1(char *path)
{
  int player;
  int card;

  strcpy(path + 9, "SVE");
  g_save_file_fd = _open(path, 0x8000);
  if (g_save_file_fd == -1)
  {
    return 0;
  }

  global_saveload_loading = 1;
  save_or_load_ver1();
  _close(g_save_file_fd);
  for (player = 0; player < 2; player++)
  {
    for (card = 0; card < 0x96; card++)
    {
      if (global_card_instances[player][card].internal_card_id != -1)
        g_active_cards_count[player] = card;
    }
  }
  return 1;
}

// FUNCTION: MAGIC 0x004ecf12
// FUNCTION: SHANDALAR 0x005018e8
unsigned int load_or_probe_duel_save_slot(char *path, int probe_only)
{
  strcpy(path + 9, "SVE");
  if (probe_only != 0)
  {
    g_save_file_fd = _open(path, 0x8000);
    if (g_save_file_fd != -1)
    {
      strcat(g_ui_message_buffer, "OK\n");
    }
    else
    {
#ifdef SHANDALAR
      sprintf(g_ui_message_buffer, "%s\n", gs_loadsave_0077d1b0[2]);
#else
      sprintf(g_ui_message_buffer, "%s\n", g_load_error_text);
#endif
    }
    _close(g_save_file_fd);
    return (g_save_file_fd != -1);
  }

#ifdef SHANDALAR
  return (unsigned int)LoadGameFromPath(path);
#else
  return load_duel_save_file_ver1(path);
#endif

  return 1;
}

// FUNCTION: MAGIC 0x004ecdc0
// FUNCTION: SHANDALAR 0x00501760
int load_selected_duel_save_slot(int player)
{
#ifdef SHANDALAR
  int save_drive_index;
  unsigned int slot_index;

  global_saveload_loading = 1;
  HideMouseCursorNested();
  save_drive_index = GetSaveDriveIndex();
  if (save_drive_index != -1)
  {
    if (player == -1)
    {
      strcpy(g_ui_message_buffer, "\x8cSelect Load File...\n");
      g_load_menu_valid_slot_mask = 0;

      for (slot_index = 0; slot_index < 10; slot_index = slot_index + 1)
      {
        g_save_file_path[7] = (char)int_to_hex_digit(slot_index);
        if (load_or_probe_duel_save_slot(g_save_file_path, 1) != 0)
        {
          g_load_menu_valid_slot_mask |= (1 << (unsigned char)slot_index);
        }
      }

      ShowMouseCursorNested();
      g_selected_save_slot_index = RunTextMenuAt(g_ui_message_buffer, 0x30, 0x40);
      HideMouseCursorNested();
      if ((g_load_menu_valid_slot_mask & (1 << (unsigned char)g_selected_save_slot_index)) == 0)
      {
        g_selected_save_slot_index = -1;
      }
    }
    else
    {
      g_selected_save_slot_index = player;
    }

    if (g_selected_save_slot_index != -1)
    {
      g_save_file_path[7] = (char)int_to_hex_digit(g_selected_save_slot_index);
      if (load_or_probe_duel_save_slot(g_save_file_path, 0) == 0)
      {
        g_selected_save_slot_index = -1;
      }
    }

    if (g_selected_save_slot_index == -1)
    {
      RunTextMenuAt("Error Loading Save Game File:EXITING\n ", 100, 0x50);
      exit(1);
    }

    ShowMouseCursorNested();
    return g_selected_save_slot_index;
  }
  else
  {
    ShowMouseCursorNested();
    return -1;
  }
#else
  int save_slot_source;
  unsigned int slot;

  global_saveload_loading = 1;
  save_slot_source = get_duel_save_slot_source();
  if (save_slot_source != -1)
  {
    if (player == -1)
    {
      strcpy(g_ui_message_buffer, "\x8cSelect Load File...\n");
      g_load_menu_valid_slot_mask = 0;
      for (slot = 0; slot < 10; slot++)
      {
        g_save_file_path[7] = (char)int_to_hex_digit(slot);
        if (load_or_probe_duel_save_slot(g_save_file_path, 1) != 0)
          g_load_menu_valid_slot_mask |= 1 << (char)slot;
      }
      g_selected_save_slot_index = 0;
      if ((g_load_menu_valid_slot_mask & (1 << (char)g_selected_save_slot_index)) == 0)
        g_selected_save_slot_index = -1;
    }
    else
    {
      g_selected_save_slot_index = player;
    }

    if (g_selected_save_slot_index != -1)
    {
      g_save_file_path[7] = (char)int_to_hex_digit(g_selected_save_slot_index);
      if (load_or_probe_duel_save_slot(g_save_file_path, 0) == 0)
        g_selected_save_slot_index = -1;
    }

    if (g_selected_save_slot_index == -1)
      exit(1);
    return g_selected_save_slot_index;
  }
  else
  {
    return -1;
  }
#endif
}

// FUNCTION: MAGIC 0x004ee26e
// FUNCTION: SHANDALAR 0x0050325f
unsigned int load_gametype0(char *path)
{
  int card_index;
  unsigned int result;
  int save_version;
  int card_type;

  result = 1;
  g_save_file_fd = _open(path, 0x8000);
  if (g_save_file_fd != -1)
  {
    global_saveload_loading = 1;
    result &= save_or_load_data(&save_version, 4);
    if (g_save_format_version == save_version)
    {
      result &= save_or_load_ver1();
      result &= save_or_load_data(&_PlayerFace, 4);
      result &= save_or_load_data(&_OpponFace, 4);
      result &= save_or_load_data(g_saved_player_name, 0x32);
      g_saved_player_name_terminator = 0;
      result &= save_or_load_data(g_player_name, 0x32);
      g_player_name_terminator = 0;
      for (card_index = g_card_count; card_index < g_card_count + 0x10; card_index++)
      {
        if (global_cards_data[card_index].id != -1)
        {
          card_type = CardTypeFromID(global_cards_data[card_index].id);
          global_cards_data[card_index].code_pointer = global_cards_data[card_type].code_pointer;
        }
      }
    }
    else
    {
      result = 0;
    }
    _close(g_save_file_fd);
  }
  else
  {
    result = 0;
  }

  return result;
}

// FUNCTION: MAGIC 0x004ef0ea
// FUNCTION: SHANDALAR 0x005040da
void save_soloduel(char *path)
{
  g_save_file_fd = _open(path, 0x8301, 0x80);
  if (g_save_file_fd != -1)
  {
    global_saveload_loading = 0;
    save_or_load_data(&g_savegame_version, 4);
    save_or_load_ver2();
    save_or_load_duel_mode_state();
    LoadSoloDuelRegistryOptions();
    save_or_load_data(&g_solo_duel_options.best_of, 4);
    save_or_load_data(&g_solo_duel_options.allow_sideboarding, 4);
    _close(g_save_file_fd);
  }
}

// FUNCTION: MAGIC 0x004ef26b
// FUNCTION: SHANDALAR 0x0050425c
void save_gauntlet(char *path)
{
  g_save_file_fd = _open(path, 0x8301, 0x80);
  if (g_save_file_fd != -1)
  {
    global_saveload_loading = 0;
    save_or_load_data(&g_savegame_version, 4);
    save_or_load_ver2();
    save_or_load_duel_mode_state();
    LoadGauntletRegistryOptions();
    save_or_load_data(&g_gauntlet_options.best_of, 4);
    save_or_load_data(&g_gauntlet_options.allow_sideboarding, 4);
    save_or_load_data(&g_gauntlet_options.gauntlet_length, 4);
    save_or_load_data(&g_duel_options_state, 4);
    save_or_load_data(&g_current_spell_internal_card_id, 4);
    save_or_load_data(&g_duel_state_00925bf8, 4);
    save_or_load_data(&g_duel_state_00777c10, 4);
    save_or_load_data(&g_duel_state_00777c08, 4);
    _close(g_save_file_fd);
  }
}

// FUNCTION: MAGIC 0x004ef4ba
// FUNCTION: SHANDALAR 0x005044ac
void save_sealeddeck(char *path)
{
  g_save_file_fd = _open(path, 0x8301, 0x80);
  if (g_save_file_fd != -1)
  {
    global_saveload_loading = 0;
    save_or_load_data(&g_savegame_version, 4);
    save_or_load_ver2();
    save_or_load_duel_mode_state();
    LoadSealedDeckRegistryOptions();
    save_or_load_data(&g_sealed_deck_options.best_of, 4);
    save_or_load_data((void *)g_savegame_data_pointer, 0x4ae64);
    _close(g_save_file_fd);
  }
}

// FUNCTION: MAGIC 0x0048b482
// FUNCTION: SHANDALAR 0x004a7112
void save_solo_duel_options_to_registry(void)
{
  HKEY options_key;
  BYTE value_buffer[12];
  DWORD disposition;

  if (RegCreateKeyExA(HKEY_CURRENT_USER, s_solo_options_registry_path_00571284, 0, NULL, 0, 0xf003f,
                      NULL, &options_key, &disposition) == ERROR_SUCCESS)
  {
    wsprintfA((char *)value_buffer, "%d", g_solo_duel_options.difficulty);
    RegSetValueExA(options_key, "Difficulty", 0, REG_SZ, value_buffer, strlen((char *)value_buffer) + 1);

    RegSetValueExA(options_key, "PlayerDeck", 0, REG_SZ, (BYTE *)g_solo_duel_options.player_deck,
                   strlen(g_solo_duel_options.player_deck) + 1);

    wsprintfA((char *)value_buffer, "%d", g_solo_duel_options.player_random);
    RegSetValueExA(options_key, "PlayerRandom", 0, REG_SZ, value_buffer, strlen((char *)value_buffer) + 1);

    RegSetValueExA(options_key, "OpponentDeck", 0, REG_SZ, (BYTE *)g_solo_duel_options.opponent_deck,
                   strlen(g_solo_duel_options.opponent_deck) + 1);

    wsprintfA((char *)value_buffer, "%d", g_solo_duel_options.opponent_random);
    RegSetValueExA(options_key, "OpponentRandom", 0, REG_SZ, value_buffer, strlen((char *)value_buffer) + 1);

    wsprintfA((char *)value_buffer, "%d", g_solo_duel_options.best_of);
    RegSetValueExA(options_key, "BestOf", 0, REG_SZ, value_buffer, strlen((char *)value_buffer) + 1);

    wsprintfA((char *)value_buffer, "%d", g_solo_duel_options.ante);
    RegSetValueExA(options_key, "Ante", 0, REG_SZ, value_buffer, strlen((char *)value_buffer) + 1);

    wsprintfA((char *)value_buffer, "%d", g_solo_duel_options.allow_sideboarding);
    RegSetValueExA(options_key, "AllowSideboarding", 0, REG_SZ, value_buffer, strlen((char *)value_buffer) + 1);

    RegFlushKey(options_key);
    RegCloseKey(options_key);
  }
}

// FUNCTION: MAGIC 0x0048b91e
// FUNCTION: SHANDALAR 0x004a75af
void save_gauntlet_options_to_registry(void)
{
  HKEY options_key;
  BYTE value_buffer[12];
  DWORD disposition;

  if (RegCreateKeyExA(HKEY_CURRENT_USER, s_gauntlet_options_registry_path_00571338, 0, NULL, 0,
                      0xf003f, NULL, &options_key, &disposition) == ERROR_SUCCESS)
  {
    RegSetValueExA(options_key, "PlayerDeck", 0, REG_SZ, (BYTE *)g_gauntlet_options.player_deck,
                   strlen(g_gauntlet_options.player_deck) + 1);

    wsprintfA((char *)value_buffer, "%d", g_gauntlet_options.player_random);
    RegSetValueExA(options_key, "PlayerRandom", 0, REG_SZ, value_buffer, strlen((char *)value_buffer) + 1);

    wsprintfA((char *)value_buffer, "%d", g_gauntlet_options.best_of);
    RegSetValueExA(options_key, "BestOf", 0, REG_SZ, value_buffer, strlen((char *)value_buffer) + 1);

    wsprintfA((char *)value_buffer, "%d", g_gauntlet_options.ante);
    RegSetValueExA(options_key, "Ante", 0, REG_SZ, value_buffer, strlen((char *)value_buffer) + 1);

    wsprintfA((char *)value_buffer, "%d", g_gauntlet_options.allow_sideboarding);
    RegSetValueExA(options_key, "AllowSideboarding", 0, REG_SZ, value_buffer, strlen((char *)value_buffer) + 1);

    wsprintfA((char *)value_buffer, "%d", g_gauntlet_options.gauntlet_length);
    RegSetValueExA(options_key, "GauntletLength", 0, REG_SZ, value_buffer, strlen((char *)value_buffer) + 1);

    RegFlushKey(options_key);
    RegCloseKey(options_key);
  }
}

// FUNCTION: MAGIC 0x004ef622
// FUNCTION: SHANDALAR 0x00504614
int validate_loaded_duel_card_availability(void)
{
  unsigned int card_id;
  int result;
  int card;
  int player;
  int internal_card_id;

  result = 1;
  for (player = 0; player <= 1; player++)
  {
    for (card = 0; card < 0x97 && card <= g_active_cards_count[player]; card++)
    {
      internal_card_id = global_card_instances[player][card].original_internal_card_id;
      if (internal_card_id < g_damage_card_internal_card_id || g_damage_card_internal_card_id + 0x2d <= internal_card_id)
      {
        if (internal_card_id > -1)
          card_id = global_cards_data[internal_card_id].id;
        else
          card_id = 0xffffffff;
        if (card_id != 0xffffffff && card_id != 0x38d && is_card_available_in_installed_expansion(card_id) == 0)
          result = 0;
      }
    }

    for (card = 0; card < 500 && global_graveyard_slots[player][card] != -1; card++)
    {
      card_id = CardIDFromType(global_graveyard_slots[player][card]);
      if (card_id != 0xffffffff && is_card_available_in_installed_expansion(card_id) == 0)
        result = 0;
    }

    for (card = 0; card < 500 && global_library[player][card] != -1; card++)
    {
      card_id = CardIDFromType(global_library[player][card]);
      if (card_id != 0xffffffff && is_card_available_in_installed_expansion(card_id) == 0)
        result = 0;
    }
  }

  return result;
}

// FUNCTION: MAGIC 0x004ef17b
// FUNCTION: SHANDALAR 0x0050416b
unsigned int load_duel_run_mode_1_save(char *path)
{
  unsigned int result;
  int save_version;

  result = 1;
  g_save_file_fd = _open(path, 0x8000);
  if (g_save_file_fd != -1)
  {
    global_saveload_loading = 1;
    result &= save_or_load_data(&save_version, 4);
    if (g_savegame_version == save_version)
    {
      result &= save_or_load_ver2();
      result &= save_or_load_duel_mode_state();
      result &= save_or_load_data(&g_solo_duel_options.best_of, 4);
      result &= save_or_load_data(&g_solo_duel_options.allow_sideboarding, 4);
      save_solo_duel_options_to_registry();
    }
    else
    {
      result = 0;
    }
    _close(g_save_file_fd);
  }
  else
  {
    result = 0;
  }

  if (result != 0 && validate_loaded_duel_card_availability() == 0)
    result = 0;

  return result;
}

// FUNCTION: MAGIC 0x004ef35a
// FUNCTION: SHANDALAR 0x0050434b
unsigned int load_duel_run_mode_2_save(char *path)
{
  unsigned int result;
  int save_version;

  result = 1;
  g_save_file_fd = _open(path, 0x8000);
  if (g_save_file_fd != -1)
  {
    global_saveload_loading = 1;
    result &= save_or_load_data(&save_version, 4);
    if (save_version == g_savegame_version)
    {
      result &= save_or_load_ver2();
      result &= save_or_load_duel_mode_state();
      result &= save_or_load_data(&g_gauntlet_options.best_of, 4);
      result &= save_or_load_data(&g_gauntlet_options.allow_sideboarding, 4);
      result &= save_or_load_data(&g_gauntlet_options.gauntlet_length, 4);
      save_gauntlet_options_to_registry();
      result &= save_or_load_data(&g_duel_options_state, 4);
      result &= save_or_load_data(&g_current_spell_internal_card_id, 4);
      result &= save_or_load_data(&g_duel_state_00925bf8, 4);
      result &= save_or_load_data(&g_duel_state_00777c10, 4);
      result &= save_or_load_data(&g_duel_state_00777c08, 4);
    }
    else
    {
      result = 0;
    }
    _close(g_save_file_fd);
  }
  else
  {
    result = 0;
  }

  if (result != 0 && validate_loaded_duel_card_availability() == 0)
    result = 0;

  return result;
}

// FUNCTION: MAGIC 0x004ef54b
// FUNCTION: SHANDALAR 0x0050453d
unsigned int load_duel_run_mode_3_save(char *path)
{
  unsigned int result;
  int save_version;

  result = 1;
  g_save_file_fd = _open(path, 0x8000);
  if (g_save_file_fd != -1)
  {
    global_saveload_loading = 1;
    result &= save_or_load_data(&save_version, 4);
    if (save_version == g_savegame_version)
    {
      result &= save_or_load_ver2();
      result &= save_or_load_duel_mode_state();
      result &= save_or_load_data(&g_sealed_deck_options.best_of, 4);
      result &= save_or_load_data(g_savegame_data_buffer, 0x4ae64);
      g_savegame_data_pointer = g_savegame_data_buffer;
    }
    else
    {
      result = 0;
    }
    _close(g_save_file_fd);
  }
  else
  {
    result = 0;
  }

  return result;
}
