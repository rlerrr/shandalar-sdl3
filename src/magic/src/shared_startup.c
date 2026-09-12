#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform/gdi_compat.h"
#include "platform/cheat_options.h"
#include "platform/legacy_io.h"

#include "defs.h"
#include "cardartlib/src/assert.h"
#include "cardartlib/src/palette.h"
#include "deckdll/src/card_db.h"
#include "deckdll/src/drm.h"
#include "deckdll/src/shared_resources.h"
#include "magic/src/global_strings.h"
#include "magic/src/global_state.h"
#include "shared_startup.h"

extern int global_available_slots;
extern card_ptr_t global_raw_cards_storage[2000];
extern card_data_t global_cards_data[];
extern BITMAPINFO g_duel_backbuffer_bmi;
extern char *s_duel_options_registry_path_00588100;

typedef struct
{
  int color_mask;
  int land_count;
  int spell_count;
  int allow_artifact_spells;
} SealedDeckStarterTemplate;

typedef struct
{
  char name[0x34];
  int starter_count;
  int booster_count;
  int free_lands;
  int rounds;
  int color_count;
} SealedDeckPackDefinition;

// GLOBAL: SHANDALAR 0x0094cfe0
// GLOBAL: MAGIC 0x00938eb0
int g_rarity_csv_sealed_template_counts[3][3];

// GLOBAL: SHANDALAR 0x0094d004
// GLOBAL: MAGIC 0x00938ed4
int g_rarity_csv_sealed_card_weights[3][20];

// GLOBAL: SHANDALAR 0x0094d0f4
// GLOBAL: MAGIC 0x00938fc4
int g_rarity_csv_sealed_starter_counts[3];

// GLOBAL: SHANDALAR 0x0094d100
// GLOBAL: MAGIC 0x00938fd0
int g_rarity_csv_sealed_booster_counts[3];

// GLOBAL: SHANDALAR 0x0094d10c
// GLOBAL: MAGIC 0x00938fdc
int g_rarity_csv_sealed_color_weights[3][3];

// GLOBAL: SHANDALAR 0x0094d130
// GLOBAL: MAGIC 0x00939000
int g_rarity_csv_sealed_land_weights[3][6];

// GLOBAL: SHANDALAR 0x0079ff90
// GLOBAL: MAGIC 0x00789200
char g_exp1_art_path[0x110];

// GLOBAL: SHANDALAR 0x00746dc0
// GLOBAL: MAGIC 0x0074b5f0
SealedDeckStarterTemplate g_sealed_starter_templates[3];

// GLOBAL: SHANDALAR 0x00746b20
// GLOBAL: MAGIC 0x0074b350
SealedDeckPackDefinition g_sealed_pack_definitions[9];

int LoadRarityCsvCardData(const char *filename);
char *CsvParseNextField(char **txt);
char *SkipCsvField(char *field);
int ParseRarityCsvExpansionMask(char *field);
int ParseRarityCsvDeckBuilderRarity(char *field);
int ParseRarityCsvExpansionRarity(char *field);
int ParseRarityCsvBooleanField(char **field);
int ParseRarityCsvIntFieldClamped(char **field, int maximum);
unsigned short PackRarityCsvDigitPattern(char *field);
int PackRarityCsvThreeBitValue(int value, int slot);

// GLOBAL: SHANDALAR 0x0093a230
// GLOBAL: MAGIC 0x00926100
char global_playdeck_path[0x110];

// GLOBAL: SHANDALAR 0x008c8660
// GLOBAL: MAGIC 0x008b44e0
char global_faces_path[0x110];

// GLOBAL: SHANDALAR 0x008e33e0
// GLOBAL: MAGIC 0x008cf290
char global_duelart_path[0x110];

// GLOBAL: SHANDALAR 0x008a92c0
// GLOBAL: MAGIC 0x008950c0
char global_duelsounds_path[0x110];

// GLOBAL: SHANDALAR 0x008bdaf0
// GLOBAL: MAGIC 0x008a98f0
char global_savegame_path[0x108];

// GLOBAL: SHANDALAR 0x007a00a0
// GLOBAL: MAGIC 0x00789310
HDC g_shared_offscreen_dc;

// GLOBAL: SHANDALAR 0x0093a938
// GLOBAL: MAGIC 0x00926808
HBITMAP g_shared_offscreen_bitmap;

// GLOBAL: SHANDALAR 0x0094eaa0
// GLOBAL: MAGIC 0x0093a980
HGDIOBJ g_shared_offscreen_prev_object;

// GLOBAL: SHANDALAR 0x008e30fc
// GLOBAL: MAGIC 0x008cefb0
void *g_shared_offscreen_bits;

// GLOBAL: SHANDALAR 0x0094d460
// GLOBAL: MAGIC 0x00939334
int g_display_color_depth;

// GLOBAL: SHANDALAR 0x006531a0
// GLOBAL: MAGIC 0x00638ca8
int g_is_win9x;

// GLOBAL: SHANDALAR 0x0073ea90
// GLOBAL: MAGIC 0x00776518
HANDLE g_mpstime_vxd_handle;

// GLOBAL: SHANDALAR 0x0078df78
// GLOBAL: MAGIC 0x00715fa0
int g_shared_startup_lock_initialized;

// GLOBAL: SHANDALAR 0x0093aa40
// GLOBAL: MAGIC 0x00926910
CRITICAL_SECTION g_shared_startup_lock;

// GLOBAL: SHANDALAR 0x007beb08
// GLOBAL: MAGIC 0x007a7d7c
int g_shared_startup_completed;

// GLOBAL: SHANDALAR 0x0093a7e0
// GLOBAL: MAGIC 0x009266b0
CRITICAL_SECTION g_card_render_lock;

// GLOBAL: SHANDALAR 0x005a8b30
// GLOBAL: MAGIC 0x00637a94
void *g_shared_startup_allocation;

static __inline void append_startup_error(char *message_buffer, const char *path, int line_index)
{
  sprintf(message_buffer + strlen(message_buffer), g_text_lines[line_index], path);
  strcat(message_buffer, "\n");
}

// FUNCTION: MAGIC 0x004a59ad
// FUNCTION: SHANDALAR 0x00557b2d
int CardIDFromType(unsigned int type)
{
  if (type == -1)
  {
    return -1;
  }
  else
  {
    type &= 0xfff;
    return global_cards_data[type].id;
  }
}

// FUNCTION: MAGIC 0x004a5929
// FUNCTION: SHANDALAR 0x00557aa9
int CardTypeFromID(int csvid)
{
  int result;
  int internal_card_id;

  if (csvid == -1)
  {
    return -1;
  }
  else
  {
    result = -1;
    for (internal_card_id = 0; global_cards_data[internal_card_id].id != -1 && result == -1; ++internal_card_id)
    {
      if (global_cards_data[internal_card_id].id == csvid)
      {
        result = internal_card_id;
      }
    }
    return result;
  }
}

// FUNCTION: MAGIC 0x004a59ea
// FUNCTION: SHANDALAR 0x00557b6a
int CardInDeck(int deck_entry)
{
  if (deck_entry == -1)
  {
    return -1;
  }
  else
  {
    return deck_entry & 0x4000;
  }
}

#ifndef SHANDALAR
// FUNCTION: MAGIC 0x004e1de1
int SellPrice(int card_index)
{
  (void)card_index;
  return 10;
}
#endif

// FUNCTION: SHANDALAR 0x00464663
// FUNCTION: MAGIC 0x00493bc3
void set_global_base_directory(char *path)
{
#ifdef _DEBUG
  // Allow debugging directly from output directory
  getcwd(path, 0x105);

#else
  char *last_slash;

  GetModuleFileNameA(NULL, path, 0x105);
  last_slash = strrchr(path, '\\');
  *last_slash = '\0';
#endif
}
// FUNCTION: SHANDALAR 0x00522880
// FUNCTION: MAGIC 0x004226c0
int setup_paths_and_load_text_etc(char *message_buffer)
{
  char path[264];
  int ok;
  HDC desktop_dc;

  ok = 1;

  set_global_base_directory(global_base_directory);
  _chdir(global_base_directory);

  strcpy(global_playdeck_path, global_base_directory);
  strcat(global_playdeck_path, "\\PlayDeck");
  strcpy(global_faces_path, global_base_directory);
  strcat(global_faces_path, "\\Faces");
  strcpy(global_cardart_path, global_base_directory);
  strcat(global_cardart_path, "\\CardArt");
  strcpy(global_duelart_path, global_base_directory);
  strcat(global_duelart_path, "\\DuelArt");
  strcpy(global_duelsounds_path, global_base_directory);
  strcat(global_duelsounds_path, "\\DuelSounds");
  strcpy(global_duel_dat_path, global_duelart_path);
  strcat(global_duel_dat_path, "\\Duel.dat");
  strcpy(global_ui_strings_filename, "UIStrings.txt");
  strcpy(global_savegame_path, global_base_directory);
  strcat(global_savegame_path, "\\SaveGame");
  _mkdir(global_savegame_path);

  load_global_ui_strings(global_ui_strings_filename);

  strcpy(path, global_base_directory);
  strcat(path, "\\CARDS.DAT");
  global_available_slots = read_db_guts(path);
  if (global_available_slots == 0)
  {
    ok = 0;
    load_text(global_ui_strings_filename, "PROMPT_STARTUPERROR");
    append_startup_error(message_buffer, path, 1);
  }

  strcpy(path, global_base_directory);
  strcat(path, "\\LEGACY.CSV");
  if (!ReadLegacyCsv(path))
  {
    ok = 0;
    load_text(global_ui_strings_filename, "PROMPT_STARTUPERROR");
    append_startup_error(message_buffer, path, 2);
  }

  strcpy(path, global_base_directory);
  strcat(path, "\\RARITY.CSV");
  if (!LoadRarityCsv(path))
  {
    ok = 0;
    load_text(global_ui_strings_filename, "PROMPT_STARTUPERROR");
    append_startup_error(message_buffer, path, 1);
  }

  CopyRawCardNamesAndRarities();

  desktop_dc = GetDC(NULL);
  if (desktop_dc != NULL)
  {
    g_display_color_depth = GetDeviceCaps(desktop_dc, BITSPIXEL) * GetDeviceCaps(desktop_dc, PLANES);
    ReleaseDC(NULL, desktop_dc);
  }
  else
  {
    ok = 0;
    load_text(global_ui_strings_filename, "PROMPT_STARTUPERROR");
    strcat(message_buffer, g_text_lines[3]);
    strcat(message_buffer, "\n");
  }

  if (!SetupDuelPalette())
  {
    ok = 0;
    load_text(global_ui_strings_filename, "PROMPT_STARTUPERROR");
    strcat(message_buffer, g_text_lines[4]);
    strcat(message_buffer, "\n");
  }

  if (!CreateOffscreen32bppDibSection(GetSystemMetrics(SM_CXSCREEN),
                                      GetSystemMetrics(SM_CYSCREEN),
                                      &g_shared_offscreen_dc,
                                      &g_duel_backbuffer_bmi,
                                      &g_shared_offscreen_bitmap,
                                      &g_shared_offscreen_prev_object,
                                      &g_shared_offscreen_bits))
  {
    ok = 0;
    load_text(global_ui_strings_filename, "PROMPT_STARTUPERROR");
    strcat(message_buffer, g_text_lines[5]);
    strcat(message_buffer, "\n");
  }

  if (!create_fonts())
  {
    ok = 0;
    load_text(global_ui_strings_filename, "PROMPT_STARTUPERROR");
    strcat(message_buffer, g_text_lines[6]);
    strcat(message_buffer, "\n");
  }

  if (!InitCardArtGdiResources())
  {
    ok = 0;
    load_text(global_ui_strings_filename, "PROMPT_STARTUPERROR");
    strcat(message_buffer, g_text_lines[7]);
    strcat(message_buffer, "\n");
  }

  InitMpstimeVxdTimer();
  return ok;
}

// FUNCTION: SHANDALAR 0x00468a60
// FUNCTION: MAGIC 0x00509210
int CopyRawCardNamesAndRarities(void)
{
  int card_index;
  int card_type;

  for (card_index = 0; card_index < global_available_slots; ++card_index)
  {
    card_type = CardTypeFromID(card_index);
    if (card_type == -1)
    {
      continue;
    }

    if (global_raw_cards_storage[card_index].rarity == 1)
    {
      global_cards_data[card_type].rarity = 1;
    }
    else if (global_raw_cards_storage[card_index].rarity == 2)
    {
      global_cards_data[card_type].rarity = 3;
    }
    else if (global_raw_cards_storage[card_index].rarity == 3)
    {
      global_cards_data[card_type].rarity = 4;
    }
    else if (global_raw_cards_storage[card_index].rarity == 4)
    {
      global_cards_data[card_type].rarity = 2;
    }
    else
    {
      global_cards_data[card_type].rarity = 1;
    }

    strncpy(global_cards_data[card_type].name, global_raw_cards_storage[card_index].full_name, 0x23);
    global_cards_data[card_type].name[0x22] = '\0';
  }

  return 1;
}

// FUNCTION: MAGIC 0x004a7b3d
// FUNCTION: SHANDALAR 0x00559cc3
int InitMpstimeVxdTimer(void)
{
  OSVERSIONINFOA version = {sizeof(OSVERSIONINFOA)};
  GetVersionExA(&version);
  g_is_win9x = version.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS;

  if (g_mpstime_vxd_handle == NULL && g_is_win9x != 0)
  {
    int ioctl_result;
    g_mpstime_vxd_handle = CreateFileA("\\\\.\\MPStime.VXD", 0, 0, NULL, 0, FILE_FLAG_DELETE_ON_CLOSE, NULL);
    assert((unsigned int)(g_mpstime_vxd_handle != INVALID_HANDLE_VALUE), "D:\\Newmagic\\multiplayer\\sid\\glue.c", 0x360,
           "Could Not Load Dave's Extra Cool Timer\n");
    DeviceIoControl(g_mpstime_vxd_handle, 1, NULL, 0, &ioctl_result, 4, NULL, NULL);
    assert((unsigned int)(ioctl_result == 0x100), "D:\\Newmagic\\multiplayer\\sid\\glue.c", 0x367,
           "Could Not Initialize Dave's Extra Cool Timer\n");
  }

  return 1;
}

// FUNCTION: SHANDALAR 0x00442f6a
// FUNCTION: MAGIC 0x004537a7
void FreeBaseTextBuffer(void)
{
  if (global_base_txt != (char *)0)
  {
    free(global_base_txt);
  }
  global_base_txt = (char *)0;
}

// FUNCTION: SHANDALAR 0x004432ff
// FUNCTION: MAGIC 0x00453b3c
void free_duel_interface_resource_buffer(void)
{
  if (g_shared_startup_allocation != NULL)
  {
    free(g_shared_startup_allocation);
  }
}

// FUNCTION: SHANDALAR 0x004a5d72
// FUNCTION: MAGIC 0x0048a0e2
void LoadDuelInterfaceRegistryOptions(void)
{
  unsigned int has_expansion;
  int unused_008c;
  int unused_0088;
  BYTE *phase_value;
  BYTE value_buffer[100];
  int step;
  int phase;
  HKEY options_key;
  BYTE integer_buffer[12];
  DWORD value_size;

  if (RegOpenKeyExA(HKEY_CURRENT_USER, s_duel_options_registry_path_00588100, 0, KEY_QUERY_VALUE, &options_key) == ERROR_SUCCESS)
  {
    value_size = 10;
    integer_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "Layout", NULL, NULL, integer_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)integer_buffer, "%d", &g_duel_interface_options.layout);
    else
      g_duel_interface_options.layout = 1;

    value_size = 10;
    integer_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "DirectiveTracksMouse", NULL, NULL, integer_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)integer_buffer, "%d", &g_duel_interface_options.directive_tracks_mouse);
    else
      g_duel_interface_options.directive_tracks_mouse = 0;

    value_size = 10;
    integer_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "ShowCoinFlips", NULL, NULL, integer_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)integer_buffer, "%d", &g_duel_interface_options.show_coin_flips);
    else
      g_duel_interface_options.show_coin_flips = 1;

    value_size = 10;
    integer_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "ShowCueCards", NULL, NULL, integer_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)integer_buffer, "%d", &g_duel_interface_options.show_cue_cards);
    else
      g_duel_interface_options.show_cue_cards = 1;

    value_size = 10;
    integer_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "ShowPowerToughnessOnCards", NULL, NULL, integer_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)integer_buffer, "%d", &g_duel_interface_options.show_power_toughness_on_cards);
    else
      g_duel_interface_options.show_power_toughness_on_cards = 1;

    value_size = 10;
    integer_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "ShowIDTagsOnCards", NULL, NULL, integer_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)integer_buffer, "%d", &g_duel_interface_options.show_id_tags_on_cards);
    else
      g_duel_interface_options.show_id_tags_on_cards = 0;

    value_size = 10;
    integer_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "ShowInvisibleEffectCards", NULL, NULL, integer_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)integer_buffer, "%d", &g_duel_interface_options.show_invisible_effect_cards);
    else
      g_duel_interface_options.show_invisible_effect_cards = 0;

    value_size = 10;
    integer_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "ShowAllCardsSummonSickness", NULL, NULL, integer_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)integer_buffer, "%d", &g_duel_interface_options.show_all_cards_summon_sickness);
    else
      g_duel_interface_options.show_all_cards_summon_sickness = 0;

    value_size = 10;
    integer_buffer[0] = '\0';
    RegQueryValueExA(options_key, "ShowAbilitiesOnCards", NULL, NULL, integer_buffer, &value_size);
    if (integer_buffer[0] == '\0')
      g_duel_interface_options.show_abilities_on_cards = 1;
    else
      sscanf((char *)integer_buffer, "%d", &g_duel_interface_options.show_abilities_on_cards);

    value_size = 10;
    integer_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "ExpandTextBoxOnBigCard", NULL, NULL, integer_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)integer_buffer, "%d", &g_duel_interface_options.expand_text_box_on_big_card);
    else
      g_duel_interface_options.expand_text_box_on_big_card = 0;

    value_size = 10;
    integer_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "SeeNextDrawsAtEndOfDuel", NULL, NULL, integer_buffer, &value_size) == ERROR_SUCCESS)
      sscanf((char *)integer_buffer, "%d", &g_duel_interface_options.see_next_draws_at_end_of_duel);
    else
      g_duel_interface_options.see_next_draws_at_end_of_duel = 0;

    value_size = 100;
    value_buffer[0] = '\0';
    if (RegQueryValueExA(options_key, "PhaseStoppers", NULL, NULL, value_buffer, &value_size) == ERROR_SUCCESS)
    {
      phase_value = value_buffer;
      phase = 0;
      while (phase < 2 && *phase_value != '\0')
      {
        step = 0;
        while (step < 0x25 && *phase_value != '\0')
        {
          if (g_duel_active == 0)
          {
            if (*phase_value == 'S')
              g_duel_phase_stop_settings[phase].phase_flags[step] = 1;
            else
              g_duel_phase_stop_settings[phase].phase_flags[step] = 0;
          }
          else if (*phase_value == 'S')
          {
            g_duel_phase_stop_settings[phase].phase_flags[step] |= 1;
          }
          else
          {
            g_duel_phase_stop_settings[phase].phase_flags[step] &= 0xfe;
          }
          phase_value++;
          step++;
        }
        phase++;
      }
      g_duel_phase_stop_settings[0].phase_flags[0x14] |= 1;
    }
    else
    {
      for (phase = 0; phase < 2; phase++)
      {
        for (step = 0; step < 0x25; step++)
          g_duel_phase_stop_settings[phase].phase_flags[step] = 0;
      }
      g_duel_phase_stop_settings[0].phase_flags[0x14] = 1;
      g_duel_phase_stop_settings[0].phase_flags[0x1e] = 1;
      g_duel_phase_stop_settings[1].phase_flags[0x1f] = 1;
    }

    if ((g_duel_mode_flags & 1) == 0)
    {
      value_size = 10;
      integer_buffer[0] = '\0';
      if (RegQueryValueExA(options_key, "PlayerTerritoryColor", NULL, NULL, integer_buffer, &value_size) == ERROR_SUCCESS)
        sscanf((char *)integer_buffer, "%d", &g_duel_interface_options.player_territory_color);
      else
        g_duel_interface_options.player_territory_color = -1;
    }

    if ((g_duel_mode_flags & 1) == 0)
    {
      value_size = 10;
      integer_buffer[0] = '\0';
      if (RegQueryValueExA(options_key, "PlayerTerritoryType", NULL, NULL, integer_buffer, &value_size) == ERROR_SUCCESS)
        sscanf((char *)integer_buffer, "%d", &g_duel_interface_options.player_territory_type);
      else
        g_duel_interface_options.player_territory_type = 2;
    }

  }
  else
  {
    g_duel_interface_options.layout = 1;
    g_duel_interface_options.directive_tracks_mouse = 0;
    g_duel_interface_options.show_coin_flips = 1;
    g_duel_interface_options.show_cue_cards = 1;
    g_duel_interface_options.show_power_toughness_on_cards = 1;
    g_duel_interface_options.show_id_tags_on_cards = 0;
    g_duel_interface_options.show_invisible_effect_cards = 0;
    g_duel_interface_options.show_all_cards_summon_sickness = 0;
    g_duel_interface_options.show_abilities_on_cards = 1;
    g_duel_interface_options.expand_text_box_on_big_card = 0;
    g_duel_interface_options.see_next_draws_at_end_of_duel = 0;
    for (phase = 0; phase < 2; phase++)
    {
      for (step = 0; step < 0x25; step++)
        g_duel_phase_stop_settings[phase].phase_flags[step] = 0;
    }
    g_duel_phase_stop_settings[0].phase_flags[0x14] = 1;
    g_duel_phase_stop_settings[0].phase_flags[0x1e] = 1;
    g_duel_phase_stop_settings[1].phase_flags[0x1f] = 1;
    if ((g_duel_mode_flags & 1) == 0)
    {
      g_duel_interface_options.player_territory_color = -1;
      g_duel_interface_options.player_territory_type = 2;
    }
  }

  has_expansion = HasExpansion(0x10);
  g_has_expansion_10 = ((has_expansion != 0) ||
                        shandalar_cheat_enabled(SHANDALAR_CHEAT_ENABLE_DEBUG_MENU));
}

// FUNCTION: SHANDALAR 0x00522e00
// FUNCTION: MAGIC 0x004c0c20
int LoadRarityCsv(const char *filename)
{
  SealedDeckPackDefinition *pack;
  int result;
  SealedDeckStarterTemplate *starter_template;

  result = 1;
  strcpy(g_exp1_art_path, global_base_directory);
  strcat(g_exp1_art_path, "\\Exp1Art");

  starter_template = &g_sealed_starter_templates[0];
  starter_template->color_mask = 0;
  starter_template->land_count = 0xb;
  starter_template->spell_count = 3;
  starter_template->allow_artifact_spells = 1;

  starter_template = &g_sealed_starter_templates[1];
  starter_template->color_mask = 0;
  starter_template->land_count = 9;
  starter_template->spell_count = 3;
  starter_template->allow_artifact_spells = 0;

  starter_template = &g_sealed_starter_templates[2];
  starter_template->color_mask = 0;
  starter_template->land_count = 6;
  starter_template->spell_count = 2;
  starter_template->allow_artifact_spells = 0;

  load_text(global_ui_strings_filename, "SEALEDDECK_PACKNAMES");

  pack = &g_sealed_pack_definitions[0];
  strcpy(pack->name, g_text_lines[0]);
  pack->starter_count = 2;
  pack->booster_count = 1;
  pack->free_lands = 0;
  pack->rounds = 1;
  pack->color_count = 1;

  pack = &g_sealed_pack_definitions[1];
  strcpy(pack->name, g_text_lines[1]);
  pack->starter_count = 2;
  pack->booster_count = 1;
  pack->free_lands = 0;
  pack->rounds = 1;
  pack->color_count = 1;

  pack = &g_sealed_pack_definitions[2];
  strcpy(pack->name, g_text_lines[2]);
  pack->starter_count = 7;
  pack->booster_count = 1;
  pack->free_lands = 0;
  pack->rounds = 1;
  pack->color_count = 5;

  pack = &g_sealed_pack_definitions[3];
  strcpy(pack->name, g_text_lines[3]);
  pack->starter_count = 2;
  pack->booster_count = 0;
  pack->free_lands = 2;
  pack->rounds = 1;
  pack->color_count = 1;

  pack = &g_sealed_pack_definitions[4];
  strcpy(pack->name, g_text_lines[4]);
  pack->starter_count = 2;
  pack->booster_count = 0;
  pack->free_lands = 2;
  pack->rounds = 1;
  pack->color_count = 1;

  pack = &g_sealed_pack_definitions[5];
  strcpy(pack->name, g_text_lines[5]);
  pack->starter_count = 0;
  pack->booster_count = 0;
  pack->free_lands = 0;
  pack->rounds = 1;
  pack->color_count = 1;

  pack = &g_sealed_pack_definitions[6];
  strcpy(pack->name, g_text_lines[6]);
  pack->starter_count = 0;
  pack->booster_count = 0;
  pack->free_lands = 2;
  pack->rounds = 1;
  pack->color_count = 1;

  pack = &g_sealed_pack_definitions[7];
  strcpy(pack->name, g_text_lines[7]);
  pack->starter_count = 0;
  pack->booster_count = 0;
  pack->free_lands = 1;
  pack->rounds = 1;
  pack->color_count = 1;

  pack = &g_sealed_pack_definitions[8];
  strcpy(pack->name, g_text_lines[8]);
  pack->starter_count = 0;
  pack->booster_count = 1;
  pack->free_lands = 0;
  pack->rounds = 1;
  pack->color_count = 1;

  result &= LoadRarityCsvCardData(filename);
  return result;
}

// FUNCTION: SHANDALAR 0x00523ef4
// FUNCTION: MAGIC 0x004c1d0e
char *SkipCsvField(char *field)
{
  while (*field != '\0' && *field != ',' && *field != '\n' && *field != '\r')
    ++field;

  if (*field == ',')
    ++field;

  return field;
}

// FUNCTION: SHANDALAR 0x00523f5c
// FUNCTION: MAGIC 0x004c1d76
int ParseRarityCsvExpansionMask(char *field)
{
  int result;
  int expansion_index;

  if (_strnicmp(field, "Un", 2) == 0)
    expansion_index = 0xb;
  else if (_strnicmp(field, "Da", 2) == 0)
    expansion_index = 5;
  else if (_strnicmp(field, "Ar", 2) == 0)
    expansion_index = 2;
  else if (_strnicmp(field, "An", 2) == 0)
    expansion_index = 1;
  else if (_strnicmp(field, "Le", 2) == 0)
    expansion_index = 8;
  else if (_strnicmp(field, "As", 2) == 0)
    expansion_index = 3;
  else if (_strnicmp(field, "Ut", 2) == 0)
    expansion_index = 0xc;
  else if (_strnicmp(field, "To", 2) == 0)
    expansion_index = 0xc;
  else if (_strnicmp(field, "Pr", 2) == 0)
    expansion_index = 9;
  else
    expansion_index = -1;

  if (expansion_index == -1)
    result = -1;
  else
    result = 1 << (unsigned char)expansion_index;

  return result;
}

// FUNCTION: SHANDALAR 0x00524104
// FUNCTION: MAGIC 0x004c1f1e
int ParseRarityCsvDeckBuilderRarity(char *field)
{
  int result;

  if (*field == 'C')
    result = 1;
  else if (*field == 'U')
    result = 4;
  else if (*field == 'R')
    result = 2;
  else
    result = -1;

  return result;
}

// FUNCTION: SHANDALAR 0x00524172
// FUNCTION: MAGIC 0x004c1f8c
int ParseRarityCsvExpansionRarity(char *field)
{
  int result;

  result = 0;
  if (*field == 'U')
    result = 1;
  else if (*field == 'C')
    result = 5;
  else if (*field == 'R')
    result = 9;

  if (result != 0)
  {
    ++field;
    if (*field < '1' || *field > '4')
    {
      if (*field > '4' && *field < ':')
        result += 3;
    }
    else
    {
      result += *field - '1';
    }
  }

  return result;
}

// FUNCTION: SHANDALAR 0x00524239
// FUNCTION: MAGIC 0x004c2053
int ParseRarityCsvBooleanField(char **field)
{
  int result;

  if (field == NULL || *field == NULL || **field == '-' || **field == '0')
    result = 0;
  else
    result = 1;

  *field = SkipCsvField(*field);
  return result;
}

// FUNCTION: SHANDALAR 0x005242ad
// FUNCTION: MAGIC 0x004c20c7
int ParseRarityCsvIntFieldClamped(char **field, int maximum)
{
  int result;
  int is_negative;

  if (field == NULL || *field == NULL)
    return 0;

  result = 0;
  if (**field == '-')
  {
    is_negative = 1;
    ++*field;
  }
  else
  {
    is_negative = 0;
  }

  while ('0' <= **field && **field <= '9')
  {
    result = **field - '0' + result * 10;
    ++*field;
  }

  if (is_negative)
    result = -result;

  *field = SkipCsvField(*field);
  if (maximum <= result)
    result = maximum;

  return result;
}

// FUNCTION: SHANDALAR 0x0052f9e0
// FUNCTION: MAGIC 0x004cd840
unsigned short PackRarityCsvDigitPattern(char *field)
{
  unsigned short result;

  if (field == NULL || *field == '-' || *field == '\0')
  {
    result = 0;
  }
  else
  {
    result = (unsigned short)(((((((field[3] - '0' < 3) ? field[3] - '0' : 3) << 8) |
                                 (((field[4] - '0' < 3) ? field[4] - '0' : 3) << 6)) |
                                (((field[2] - '0' < 3) ? field[2] - '0' : 3) << 2)) |
                               (((field[1] - '0' < 3) ? field[1] - '0' : 3) << 4)) |
                              (((field[0] - '0' < 3) ? field[0] - '0' : 3) << 10));
  }

  return result;
}

// FUNCTION: SHANDALAR 0x0052fac0
// FUNCTION: MAGIC 0x004cd920
int PackRarityCsvThreeBitValue(int value, int slot)
{
  return value << (slot * 3);
}

// FUNCTION: SHANDALAR 0x005231ec
// FUNCTION: MAGIC 0x004c100c
int LoadRarityCsvCardData(const char *filename)
{
  int parsed_db_rarity;
  int temp_value2;
  char *next_field;
  unsigned short ai_dependencies;
  int inflatable_value;
  int parsed_expansion;
  int ai_modifiers2;
  int temp_value;
  int ai_modifiers1;
  char *file_buffer;
  card_ptr_t *raw_card;
  HANDLE file;
  DWORD start_tick;
  int result;
  DWORD bytes_read;
  DWORD file_size;
  unsigned char mana_source_colors;
  int field_index;
  unsigned int expansion_rarity;
  char *cursor;
  int row_index;

  start_tick = GetTickCount();
  file = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
  if (file != INVALID_HANDLE_VALUE)
  {
    file_size = GetFileSize(file, NULL);
    file_buffer = malloc(file_size + 1);
    if (file_buffer != NULL)
    {
      ReadFile(file, file_buffer, file_size, &bytes_read, NULL);
      cursor = file_buffer;
      cursor = strchr(cursor, '\n') + 1;
      cursor = strchr(cursor, '\n') + 1;

      for (row_index = 0; row_index < 3; ++row_index)
      {
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);

        for (field_index = 0; field_index < 3; ++field_index)
          g_rarity_csv_sealed_template_counts[row_index][field_index] =
              ParseRarityCsvIntFieldClamped(&cursor, 99);

        for (field_index = 0; field_index < 0x14; ++field_index)
          g_rarity_csv_sealed_card_weights[row_index][field_index] =
              ParseRarityCsvIntFieldClamped(&cursor, 99);

        cursor = SkipCsvField(cursor);
        g_rarity_csv_sealed_starter_counts[row_index] = ParseRarityCsvIntFieldClamped(&cursor, 99);
        g_rarity_csv_sealed_booster_counts[row_index] = ParseRarityCsvIntFieldClamped(&cursor, 99);

        for (field_index = 0; field_index < 3; ++field_index)
          g_rarity_csv_sealed_color_weights[row_index][field_index] =
              ParseRarityCsvIntFieldClamped(&cursor, 99);

        for (field_index = 0; field_index < 6; ++field_index)
          g_rarity_csv_sealed_land_weights[row_index][field_index] =
              ParseRarityCsvIntFieldClamped(&cursor, 99);

        if (*cursor != '\n')
          cursor = strchr(cursor, '\n') + 1;
      }

      for (field_index = 0; field_index < global_available_slots; ++field_index)
      {
        raw_card = &global_raw_cards_storage[field_index];

        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        cursor = SkipCsvField(cursor);
        next_field = CsvParseNextField(&cursor);
        cursor = next_field;
        cursor = SkipCsvField(cursor);

        next_field = CsvParseNextField(&cursor);
        parsed_expansion = ParseRarityCsvExpansionMask(cursor);
        if (parsed_expansion != -1)
          raw_card->expansion = parsed_expansion;

        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        parsed_db_rarity = ParseRarityCsvDeckBuilderRarity(cursor);
        if (parsed_db_rarity != -1)
          raw_card->rarity = parsed_db_rarity;

        expansion_rarity = 0;
        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        expansion_rarity |= ParseRarityCsvExpansionRarity(cursor);
        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        expansion_rarity |= ParseRarityCsvExpansionRarity(cursor) << 4;
        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        expansion_rarity |= ParseRarityCsvExpansionRarity(cursor) << 8;
        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        expansion_rarity |= ParseRarityCsvExpansionRarity(cursor) << 0xc;
        raw_card->expansion_rarity = expansion_rarity;

        mana_source_colors = 0;
        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        if (*cursor == '-')
        {
        }
        else
        {
          mana_source_colors = (unsigned char)(((*cursor - '0') << 5) | mana_source_colors);
        }
        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        if (*cursor == '-')
        {
        }
        else
        {
          mana_source_colors = (unsigned char)(((*cursor - '0') * 4) | mana_source_colors);
        }
        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        if (*cursor == '-')
        {
        }
        else
        {
          mana_source_colors = (unsigned char)(((*cursor - '0') * 2) | mana_source_colors);
        }
        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        if (*cursor == '-')
        {
        }
        else
        {
          mana_source_colors = (unsigned char)(((*cursor - '0') << 4) | mana_source_colors);
        }
        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        if (*cursor == '-')
        {
        }
        else
        {
          mana_source_colors = (unsigned char)(((*cursor - '0') * 8) | mana_source_colors);
        }
        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        if (*cursor == '-')
        {
        }
        else
        {
          mana_source_colors = (unsigned char)((*cursor - '0') | mana_source_colors);
        }
        raw_card->mana_source_colors = (int8_t)mana_source_colors;

        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        raw_card->ai_against_color = PackRarityCsvDigitPattern(cursor);
        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        raw_card->ai_for_color = PackRarityCsvDigitPattern(cursor);
        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        raw_card->ai_counts_as_color = PackRarityCsvDigitPattern(cursor);
        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        raw_card->ai_against_land = PackRarityCsvDigitPattern(cursor);
        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        raw_card->ai_for_land = PackRarityCsvDigitPattern(cursor);
        cursor = next_field;
        next_field = CsvParseNextField(&cursor);
        raw_card->ai_counts_as_land = PackRarityCsvDigitPattern(cursor);

        cursor = next_field;
        ai_dependencies = 0;
        ai_dependencies |= PackRarityCsvThreeBitValue((unsigned short)ParseRarityCsvBooleanField(&cursor), 0);
        ai_dependencies |= PackRarityCsvThreeBitValue((unsigned short)ParseRarityCsvBooleanField(&cursor), 1);
        ai_dependencies |= PackRarityCsvThreeBitValue((unsigned short)ParseRarityCsvBooleanField(&cursor), 2);
        raw_card->ai_dependencies = ai_dependencies;

        ai_modifiers1 = 0;
        ai_modifiers1 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 0);
        ai_modifiers1 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 1);
        ai_modifiers1 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 2);
        ai_modifiers1 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 3);
        ai_modifiers1 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 4);
        ai_modifiers1 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 5);
        ai_modifiers1 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 6);
        ai_modifiers1 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 7);
        ai_modifiers1 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 8);
        ai_modifiers1 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 9);

        ai_modifiers2 = 0;
        ai_modifiers2 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 0);
        ai_modifiers2 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 1);
        ai_modifiers2 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 2);
        ai_modifiers2 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 3);
        ai_modifiers2 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 4);
        ai_modifiers2 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 5);
        ai_modifiers2 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 6);
        ai_modifiers2 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 7);
        ai_modifiers2 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 8);
        ai_modifiers2 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 9);
        raw_card->ai_modifiers1 = ai_modifiers1;
        raw_card->ai_modifiers2 = ai_modifiers2;

        raw_card->ai_base_value = (short)ParseRarityCsvIntFieldClamped(&cursor, 999);
        temp_value = ParseRarityCsvIntFieldClamped(&cursor, 999);
        temp_value2 = ParseRarityCsvIntFieldClamped(&cursor, 999);
        raw_card->ai_power_toughness = MAKEWORD(temp_value, temp_value2);
        temp_value = ParseRarityCsvIntFieldClamped(&cursor, 0xff);
        temp_value2 = ParseRarityCsvIntFieldClamped(&cursor, 0xff);
        raw_card->ai_inc_power_toughness = MAKEWORD(temp_value, temp_value2);

        inflatable_value = ParseRarityCsvIntFieldClamped(&cursor, 1);
        if (inflatable_value != 0)
          raw_card->inflatable = 1;
        else
          raw_card->inflatable = 0;

        ai_modifiers1 = 0;
        ai_modifiers1 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 0);
        ai_modifiers1 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 1);
        ai_modifiers1 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 2);
        ai_modifiers1 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 3);
        ai_modifiers1 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 4);
        ai_modifiers1 |= PackRarityCsvThreeBitValue(ParseRarityCsvIntFieldClamped(&cursor, 7), 5);
        raw_card->ai_abilities = ai_modifiers1;

        if (*cursor != '\n')
          cursor = strchr(cursor, '\n') + 1;
      }

      global_raw_cards_storage[239].rarity = 0;
      global_raw_cards_storage[188].rarity = 0;
      global_raw_cards_storage[91].rarity = 0;
      global_raw_cards_storage[126].rarity = 0;
      global_raw_cards_storage[164].rarity = 0;

      result = 1;
      free(file_buffer);
    }
    else
    {
      result = 0;
    }

    CloseHandle(file);
  }
  else
  {
    result = 0;
  }

  start_tick = GetTickCount() - start_tick;
  return result;
}

// FUNCTION: SHANDALAR 0x0046901b
// FUNCTION: MAGIC 0x005097cb
unsigned int setup_shared_startup(void)
{
  unsigned int result;
  char startup_message[2000];

  result = 1;
  if (g_shared_startup_lock_initialized == 0)
  {
    InitializeCriticalSection(&g_shared_startup_lock);
    g_shared_startup_lock_initialized = 1;
  }
  startup_message[0] = '\0';
  result |= setup_paths_and_load_text_etc(startup_message);
  LoadDuelInterfaceRegistryOptions();
  g_shared_startup_completed = 1;
  InitializeCriticalSection(&g_card_render_lock);
  return result;
}

// FUNCTION: MAGIC 0x00422bea
// FUNCTION: SHANDALAR 0x00522daa
void ShutdownSharedStartupResources(void)
{
  FreeBaseTextBuffer();
  free_duel_interface_resource_buffer();
  FreeRaritiesCsvRaw();
  DestroyCardArtPalette();
  checked_DeleteDC_DeleteObject(g_shared_offscreen_dc, g_shared_offscreen_bitmap);
  g_shared_offscreen_bitmap = (HBITMAP)0;
  g_shared_offscreen_dc = (HDC)0;
  ShutdownCardArtGdiResources();
  destroy_create_fonts_resources();
}

// FUNCTION: MAGIC 0x00509849
// FUNCTION: SHANDALAR 0x00469099
int ShutdownSharedStartup(void)
{
  ShutdownSharedStartupResources();
  if (g_shared_startup_lock_initialized != 0)
  {
    DeleteCriticalSection(&g_shared_startup_lock);
    g_shared_startup_lock_initialized = 0;
  }
  DeleteCriticalSection(&g_card_render_lock);
}
