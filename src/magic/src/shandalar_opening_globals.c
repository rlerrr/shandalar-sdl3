#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"
#include "magic/src/global_other.h"

char g_text_lines[249][300];
char g_ui_message_buffer[0x1000];
char g_name_entry_buffer[0x40] = "Ned Way the Ratiocinator";
char g_player_name[0x32];
int g_player_is_male;
HWND g_main_window_hwnd;
int g_shandalar_difficulty;
int g_selected_wizard_color = -1;
int g_starting_color;
int g_deck_color_bitmap = 1;
int Gold = 100;
int g_skip_new_game_state_initialization;
int g_world_location_entry_enabled = 1;
int g_world_scene_reveal_effect_pending;
