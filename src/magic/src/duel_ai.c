
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defs.h"
#include "magic/src/cards.h"
#include "magic/src/duel_engine.h"
#include "magic/src/game_support.h"
#include "magic/src/global_duel_ui_ids.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"

extern int g_life[2];
extern char g_duel_winner;
extern int g_duel_action_pending;
extern char g_shell_art_directory[300];

void TENTATIVE_reassess_all_cards(int view_player, int present_after_draw);
void append_to_trace_txt(char *text);
int dispatch_trigger(int player, trigger_t trig, const char *prompt, int TENTATIVE_allow_response);
int dispatch_trigger_twice_once_with_each_player_as_reason(int reason_for_trig, trigger_t trig, const char *prompt, int a4);
void resolve_mana_burn(void);
int check_duel_finished(void);
void show_opponent_taunt(char *text);
void FUN_00464bb0(void);
int get_duel_thread_time_ms(void);
int put_card_on_stack(int player, int card, int pay_costs);
int resolve_card_on_stack(int player, int card);
int dispatch_event_to_single_card(int player, int card, event_t event, int a4, int a5);
int activate(int player, int a2, int card);
void request_duel_display_refresh_if_human(int player, int card, int a3, int a4);
int can_attack(int player, int card);
int get_recorded_action_count(void);
void push_affected_card_stack(void);
void pop_affected_card_stack(void);
int C_get_abilities(int player, int card, event_t event, int a4);
#ifdef SHANDALAR
int ConsumeUiTickCount(void);
void HideMouseCursorNested(void);
void ShowMouseCursorNested(void);
void NoopSetSpecialSprite(int unused_a, int unused_b, void *unused_c);
extern void *g_ttsprite_special_sprite_b;
#endif
void set_duel_prompt_text(char *text);
void process_damage_prevention(int player);
void mark_blocked_attackers(int player);
void resolve_combat_damage(int player);
void reassess_all_cards_and_mana(void);
int GetCardRarity(int card_id);
int reset_duel_tick_timer_indirect(void);
void update_duel_thread_time_marker(void);
void StopWorldLocationMusic(void);
void __cdecl _assert(void *expr, void *filename, unsigned line);
void append_displayed_card_name(int player, int card);
int is_in_play(int player, int card);
int has_mana(int player, color_t color, int amount);
int player_has_available_blocker(int player);
int assign_blocker_to_attacker(int blocker_player, int blocker_card, int attacker_player, int attacker_card);
void get_landwalk_evasion_masks(unsigned int *out_landwalk_mask, unsigned int *out_basic_land_mask);
int can_block_attacker_with_abilities(int blocker_player, int blocker_card, int attacker_player, int attacker_card, unsigned int attacker_abilities,
                                      unsigned int evasion_mask);
int play_sound_effect(wav_t sound_id);
void record_ai_action_selection(void);
void replay_ai_action_selection(void);
void save_ai_search_state(void);
#ifdef SHANDALAR
void refresh_ai_random_table(void);
#endif
void restore_ai_search_state(void);
int ai_opinion_of_gamestate_continued(int player, int score);
void setup_ai_combat_abilities(int player);
int score_ai_attacker_candidate(int player, int card);
void setup_combat_damage_simulation(int player);
void save_combat_simulation_state(void);
void restore_combat_simulation_state(void);
int score_gamestate_after_simulated_card_death(int player, int card);
void dispatch_ai_combat_pump_checks(int player);
int score_ai_blocker_candidate(int player, int card);
unsigned int get_destroy_if_blocked_combat_flags(int attacker_player, int attacker_card, int blocker_player, int blocker_card);
void choose_blockers_ai(int player);
void enumerate_ai_block_assignments(int player, int blocker_index);
int score_current_ai_block_assignment(void);

extern int combat_damage_attacker_abilities[16];
extern int combat_damage_attacker_cards[16];
extern int combat_damage_attacker_count;
extern int combat_damage_attacker_damage[16];
extern int combat_damage_attacker_toughness[16];
extern int combat_damage_blocker_abilities[16];
extern int combat_damage_blocker_damage[16];
extern int combat_damage_blocker_toughness[16];
extern int g_reveal_all_world_info;
extern int g_ai_response_state;

typedef struct
{
  card_instance_t card_instances[2][151];
  card_data_t extra_card_data[16];
  int library[2][500];
  int graveyard[2][500];
  int exile[2][500];
  int special_mana[2][8];
  int raw_mana[2][8];
  int basic_lands[2][8];
  int raw_mana_hex[2][51];
  int life_totals[2];
  int poison_counters[2];
  unsigned int card_types[2];
  int graveyard_type_flags[2];
  int duel_summary_copy[24];
  int hand_counts[2];
  int creature_counts[2];
  int artifact_counts[2];
  int enchantment_counts[2];
  int cards_drawn_value;
  int creatures_died_this_turn_value;
  int land_entries_this_turn_value;
  int ante_cards[2][16];
  int timestamp_player[500];
  int timestamp_card[500];
  int land_play_flags;
  phase_t phase;
  int current_casting_internal_card_id_value;
  int combat_count;
  int spell_fizzled_value;
  int stack_events[32];
  target_t stack_cards[32];
  target_t stack_damage_targets[32];
  int active_counts[2];
  int stack_count;
  int(__cdecl *pending_killed_card_handler_value)(int, int);
  int duel_state_0093a848_value;
  damage_accumulator_t damage_matrix[2][151][2];
  int cost_mod[8];
  int mana_charge[8];
  int x_value_copy;
  int max_x_value_copy;
  int extra_turn_player;
  phase_t stop_phase_copy;
  int stop_phase_player_copy;
  int battlefield_flags;
  int lich_active_copy[2];
  int player_special_effect_flags_copy[2];
} AiSearchBackup;

// GLOBAL: SHANDALAR 0x0074f1f0
static AiSearchBackup g_ai_search_backup;

typedef struct ai_combat_eval_entry_t
{
  int power;
  int toughness;
  unsigned int abilities;
  int cached_score;
} ai_combat_eval_entry_t;

#define AI_CARD(player_, card_) global_card_instances[(player_)][(card_)]
#define AI_CARD_STATE(player_, card_) AI_CARD(player_, card_).state
#define AI_CARD_INTERNAL_ID(player_, card_) AI_CARD(player_, card_).internal_card_id
#define AI_CARD_BLOCKING(player_, card_) AI_CARD(player_, card_).blocking
#define AI_EVAL(player_, card_) (*(ai_combat_eval_table[(player_)] + (card_)))

// GLOBAL: MAGIC 0x007081ac
// GLOBAL: SHANDALAR 0x005a7ba4
int ai_blocker_count;

// GLOBAL: MAGIC 0x007086e0
// GLOBAL: SHANDALAR 0x005a80d8
int ai_blocker_cards[16];

// GLOBAL: MAGIC 0x00707d58
// GLOBAL: SHANDALAR 0x005a7750
int ai_block_damage_orders[16];

// GLOBAL: MAGIC 0x00707de0
// GLOBAL: SHANDALAR 0x005a77d8
int ai_block_assignments[16];

// GLOBAL: MAGIC 0x00708080
// GLOBAL: SHANDALAR 0x005a7a78
int ai_blocker_player;

// GLOBAL: MAGIC 0x00708698
// GLOBAL: SHANDALAR 0x005a8090
int g_ai_attacker_count;

// GLOBAL: MAGIC 0x00708610
// GLOBAL: SHANDALAR 0x005a8008
int g_ai_block_assignment_count;

// GLOBAL: MAGIC 0x00708658
// GLOBAL: SHANDALAR 0x005a8050
int g_ai_attacker_cards[16];
// GLOBAL: MAGIC 0x00707ef0
// GLOBAL: SHANDALAR 0x005a78e8
int g_ai_attacker_power[16];
// GLOBAL: MAGIC 0x00708128
// GLOBAL: SHANDALAR 0x005a7b20
int g_ai_attacker_toughness[16];
// GLOBAL: MAGIC 0x00707f38
// GLOBAL: SHANDALAR 0x005a7930
int g_ai_attacker_damage[16];
// GLOBAL: MAGIC 0x00707d98
// GLOBAL: SHANDALAR 0x005a7790
int g_ai_attacker_scores[16];
// GLOBAL: MAGIC 0x007080c8
// GLOBAL: SHANDALAR 0x005a7ac0
int g_ai_blocker_power[16];
// GLOBAL: MAGIC 0x00708088
// GLOBAL: SHANDALAR 0x005a7a80
int g_ai_blocker_toughness[16];
// GLOBAL: MAGIC 0x00707ea8
// GLOBAL: SHANDALAR 0x005a78a0
int g_ai_blocker_damage[16];
// GLOBAL: MAGIC 0x007081a8
// GLOBAL: SHANDALAR 0x005a7ba0
int g_ai_current_attacker;
// GLOBAL: MAGIC 0x00708108
// GLOBAL: SHANDALAR 0x005a7b00
unsigned int g_ai_attacker_ability_counts[7];
// GLOBAL: MAGIC 0x00708168
// GLOBAL: SHANDALAR 0x005a7b60
int g_ai_blocker_scores[16];
// GLOBAL: MAGIC 0x007081b0
// GLOBAL: SHANDALAR 0x005a7ba8
unsigned int g_ai_blocker_ability_counts[7];
// GLOBAL: MAGIC 0x00707ff8
// GLOBAL: SHANDALAR 0x005a79f0
int g_ai_current_blocker;
// GLOBAL: MAGIC 0x00707ffc
// GLOBAL: SHANDALAR 0x005a79f4
unsigned int g_ai_combat_flags;
// GLOBAL: MAGIC 0x00707e20
// GLOBAL: SHANDALAR 0x005a7818
int g_ai_destroy_if_blocked_attacker_mask;
// GLOBAL: MAGIC 0x00707ee8
// GLOBAL: SHANDALAR 0x005a78e0
int g_ai_combat_best_score;
// GLOBAL: MAGIC 0x00707fb8
// GLOBAL: SHANDALAR 0x005a79b0
int ai_candidate_block_assignments[16];
// GLOBAL: MAGIC 0x007081d0
// GLOBAL: SHANDALAR 0x005a7bc8
int ai_attacker_blocker_counts[16];
// GLOBAL: MAGIC 0x00708720
// GLOBAL: SHANDALAR 0x005a8118
int g_ai_combat_candidate_count;
// GLOBAL: MAGIC 0x00708724
// GLOBAL: SHANDALAR 0x005a811c
unsigned int g_ai_combat_candidate_mask;
// GLOBAL: MAGIC 0x00708728
// GLOBAL: SHANDALAR 0x005a8120
int g_ai_combat_candidate_cards[16];
// GLOBAL: MAGIC 0x00707c98
// GLOBAL: SHANDALAR 0x005a7690
int g_ai_combat_candidate_scores[16];

// GLOBAL: MAGIC 0x00707bf8
// GLOBAL: SHANDALAR 0x005a75f0
unsigned int g_ai_combat_search_flags;

// GLOBAL: MAGIC 0x00707dd8
// GLOBAL: SHANDALAR 0x005a77d0
int g_ai_combat_search_depth;

// GLOBAL: MAGIC 0x0070a830
// GLOBAL: SHANDALAR 0x0097db20
int untapped_royal_assassin_count;

// GLOBAL: MAGIC 0x0070a834
// GLOBAL: SHANDALAR 0x0097db24
int defending_cop_color_mask;

// GLOBAL: MAGIC 0x0070a838
// GLOBAL: SHANDALAR 0x0097db2c
int defending_cop_available_mana;

// GLOBAL: MAGIC 0x0072c8f0
// GLOBAL: SHANDALAR 0x00765e30
static card_instance_t combat_sim_saved_card_instances[2][151];
// GLOBAL: MAGIC 0x00742ae0
// GLOBAL: SHANDALAR 0x0077c070
static card_data_t combat_sim_saved_dynamic_card_data[16];
// GLOBAL: MAGIC 0x0069fbc8
// GLOBAL: SHANDALAR 0x005abeb0
static int combat_sim_saved_library[2][500];
// GLOBAL: MAGIC 0x006a4ef0
// GLOBAL: SHANDALAR 0x005b11d8
static int combat_sim_saved_graveyard_slots[2][500];
// GLOBAL: MAGIC 0x006aa3c8
// GLOBAL: SHANDALAR 0x005b66b0
static int combat_sim_saved_exile[2][500];
// GLOBAL: MAGIC 0x006aa380
// GLOBAL: SHANDALAR 0x005b6668
static int combat_sim_saved_special_mana_pool[2][8];
// GLOBAL: MAGIC 0x006a4eb0
// GLOBAL: SHANDALAR 0x005b1198
static int combat_sim_saved_raw_mana_available[2][8];
// GLOBAL: MAGIC 0x006a4e20
// GLOBAL: SHANDALAR 0x005b1108
static int combat_sim_saved_basiclandtypes_controlled[2][8];
// GLOBAL: MAGIC 0x006a0b70
// GLOBAL: SHANDALAR 0x005ace58
static int combat_sim_saved_raw_mana_available_hex[2][51];
// GLOBAL: MAGIC 0x0069e3b0
// GLOBAL: SHANDALAR 0x005aa698
static int combat_sim_saved_life[2];
// GLOBAL: MAGIC 0x006a4e60
// GLOBAL: SHANDALAR 0x005b1148
static int combat_sim_saved_poison_counters[2];
// GLOBAL: MAGIC 0x006a4e88
// GLOBAL: SHANDALAR 0x005b1170
static unsigned int combat_sim_saved_card_types_in_play[2];
// GLOBAL: MAGIC 0x006a4e90
// GLOBAL: SHANDALAR 0x005b1178
static int combat_sim_saved_graveyard_card_types[2];
// GLOBAL: MAGIC 0x006a72f0
// GLOBAL: SHANDALAR 0x005b35d8
static int combat_sim_saved_duel_summary[24];
// GLOBAL: MAGIC 0x006a5e90
// GLOBAL: SHANDALAR 0x005b2178
static int combat_sim_saved_ante_cards[2][16];
// GLOBAL: MAGIC 0x006a9b38
// GLOBAL: SHANDALAR 0x005b5e20
static int combat_sim_saved_timestamp_player[500];
// GLOBAL: MAGIC 0x0069e3b8
// GLOBAL: SHANDALAR 0x005aa6a0
static int combat_sim_saved_timestamp_card[500];
// GLOBAL: MAGIC 0x006a4d18
// GLOBAL: SHANDALAR 0x005b1000
static int combat_sim_saved_land_can_be_played;
// GLOBAL: MAGIC 0x006a8a64
// GLOBAL: SHANDALAR 0x005b4d4c
static phase_t combat_sim_saved_current_phase;
// GLOBAL: MAGIC 0x006a4e9c
// GLOBAL: SHANDALAR 0x005b1184
static int combat_sim_saved_current_casting_internal_card_id;
// GLOBAL: MAGIC 0x006a4ea8
// GLOBAL: SHANDALAR 0x005b1190
static int combat_sim_saved_attacking_creature_count;
// GLOBAL: MAGIC 0x006a7350
// GLOBAL: SHANDALAR 0x005b3638
static int combat_sim_saved_spell_fizzled;
// GLOBAL: MAGIC 0x0069fb40
// GLOBAL: SHANDALAR 0x005abe28
static int combat_sim_saved_stack_data[32];
// GLOBAL: MAGIC 0x006a7bb8
// GLOBAL: SHANDALAR 0x005b3ea0
static target_t combat_sim_saved_stack_cards[32];
// GLOBAL: MAGIC 0x006a4c18
// GLOBAL: SHANDALAR 0x005b0f00
static target_t combat_sim_saved_stack_damage_targets[32];
// GLOBAL: MAGIC 0x006a4ea0
// GLOBAL: SHANDALAR 0x005b1188
static int combat_sim_saved_active_cards_count[2];
// GLOBAL: MAGIC 0x006a7ba8
// GLOBAL: SHANDALAR 0x005b3e90
static int combat_sim_saved_stack_size;
// GLOBAL: MAGIC 0x006a0d50
// GLOBAL: SHANDALAR 0x005ad038
static int(__cdecl *combat_sim_saved_pending_killed_card_handler)(int, int);
// GLOBAL: MAGIC 0x006a0d08
// GLOBAL: SHANDALAR 0x005acff0
static int combat_sim_saved_ai_modifier;
// GLOBAL: MAGIC 0x006aa310
// GLOBAL: SHANDALAR 0x005b65f8
static int combat_sim_saved_mana_charge[8];
// GLOBAL: MAGIC 0x006a7bac
// GLOBAL: SHANDALAR 0x005b3e94
static int combat_sim_saved_x_value;
// GLOBAL: MAGIC 0x006a9b28
// GLOBAL: SHANDALAR 0x005b5e10
static int combat_sim_saved_max_x_value;
// GLOBAL: MAGIC 0x006a7bb4
// GLOBAL: SHANDALAR 0x005b3e9c
static int combat_sim_saved_extra_turn_player;
// GLOBAL: MAGIC 0x006a8658
// GLOBAL: SHANDALAR 0x005b4940
static int combat_sim_saved_DAT_0093a848;
// GLOBAL: MAGIC 0x006a8a60
// GLOBAL: SHANDALAR 0x005b4d48
static phase_t combat_sim_saved_stop_phase;
// GLOBAL: MAGIC 0x006aa330
// GLOBAL: SHANDALAR 0x005b6618
static int combat_sim_saved_stop_phase_player;
// GLOBAL: MAGIC 0x006a25b8
// GLOBAL: SHANDALAR 0x005ae8a0
static int combat_sim_saved_battlefield_extra_ability_flags;
// GLOBAL: MAGIC 0x006aa308
// GLOBAL: SHANDALAR 0x005b65f0
static int combat_sim_saved_unk_008b44d0[2];
// GLOBAL: MAGIC 0x006a4c10
// GLOBAL: SHANDALAR 0x005b0ef8
static int combat_sim_saved_unk_007a79b0[2];

// GLOBAL: MAGIC 0x00709570
// GLOBAL: SHANDALAR 0x0097c860
ai_combat_eval_entry_t ai_combat_eval_table[2][150];

// GLOBAL: MAGIC 0x008cefac
// GLOBAL: SHANDALAR 0x0097db28
int combat_eval_power;

// GLOBAL: MAGIC 0x0093b278
// GLOBAL: SHANDALAR 0x0097db38
int combat_eval_toughness;

// GLOBAL: MAGIC 0x0092608c
// GLOBAL: SHANDALAR 0x0097db34
int combat_eval_abilities;

// GLOBAL: MAGIC 0x0069fbc0
// GLOBAL: SHANDALAR 0x005abea8
int ai_castable_phase_mask;

// GLOBAL: MAGIC 0x006a72e8
// GLOBAL: SHANDALAR 0x005b35d0
int ai_activatable_phase_mask;

// GLOBAL: MAGIC 0x0057aaec
// GLOBAL: SHANDALAR 0x00589a3c
int ai_search_candidate_offset = 0;

// GLOBAL: MAGIC 0x0057dd20
// GLOBAL: SHANDALAR 0x00581808
int g_ai_combat_search_width_by_depth[16] = {4, 4, 4, 4, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

// FUNCTION: MAGIC 0x004e3380
// FUNCTION: SHANDALAR 0x004c25f0
int choose_ai_main_phase_action(int player)
{
  unsigned int land_colors_needed;
  unsigned int candidates[20];
  int color;
  unsigned int result;
  unsigned int selected_color;
  int fewest_controlled;
  int nonbasic_land_count;
  unsigned int playable_types;
  int opponent;
  int card;
  unsigned int possible_land_colors;
  unsigned int preferred_land_color;
  int internal_card_id;
  int candidate_count;
  unsigned int exact_land_colors_needed;
  int ai_depth;
  int colorless_land;
  int playable_card_count;
  unsigned int missing_land_colors;

  if ((g_duel_ai_mode_state != 1) && (g_ai_action_replay_available == 0))
  {
    return -1;
  }

  opponent = 1 - player;
  playable_card_count = 0;
  nonbasic_land_count = playable_card_count;
  missing_land_colors = nonbasic_land_count;
  possible_land_colors = missing_land_colors;
  land_colors_needed = possible_land_colors;
  playable_types = 2;
  if ((g_land_can_be_played & LCBP_LAND_HAS_BEEN_PLAYED) || (g_duel_summary.hand_counts[player] + g_hand_count_adjustment <= 0))
  {
  }
  else
  {
    for (card = 0; (int)card < g_active_cards_count[player]; card++)
    {
      if (global_card_instances[player][card].internal_card_id != -1)
      {
        if (((global_card_instances[player][card].state & (STATE_OUBLIETTED | STATE_IN_PLAY)) == STATE_IN_PLAY) &&
            ((global_cards_data[global_card_instances[player][card].internal_card_id].type & 2) != 0))
        {
          playable_types |= 0x7c;
        }
        if ((global_card_instances[player][card].state & (STATE_IN_PLAY | STATE_TAPPED)) == 0)
        {
          if ((global_cards_data[global_card_instances[player][card].internal_card_id].type & 1) != 0)
          {
            missing_land_colors = 1;
            if (global_card_instances[player][card].mana_color != 0)
            {
              nonbasic_land_count++;
            }
          }
          if (global_cards_data[global_card_instances[player][card].internal_card_id].cc[1] == -1)
          {
            nonbasic_land_count += 99;
          }
          playable_card_count++;
        }
      }
    }
    if (missing_land_colors == 0)
    {
    }
    else
    {
      missing_land_colors = colorless_land = 0;
      for (card = 0; (int)card < g_active_cards_count[player]; card++)
      {
        internal_card_id = global_card_instances[player][card].internal_card_id;
        if (internal_card_id == -1)
        {
          continue;
        }

        if ((global_card_instances[player][card].state & (STATE_IN_PLAY | STATE_TAPPED)) == 0)
        {
          selected_color = (unsigned int)global_cards_data[internal_card_id].color;
          if ((global_cards_data[internal_card_id].type & 1) != 0)
          {
            land_colors_needed |= selected_color;
            if (selected_color == 0)
            {
              colorless_land = 1;
            }
          }
          if ((playable_types & global_cards_data[internal_card_id].type) != 0)
          {
            for (color = 1; color < 6; color++)
            {
              if ((selected_color & (1 << (unsigned char)color)) == 0)
              {
                continue;
              }

              if (g_basiclandtypes_controlled[player][color] + 1 == (int)global_cards_data[internal_card_id].cc[0])
              {
                possible_land_colors |= 1 << (unsigned char)color;
              }
              if (g_basiclandtypes_controlled[player][color] + 1 < (int)global_cards_data[internal_card_id].cc[0])
              {
                missing_land_colors |= 1 << (unsigned char)color;
              }
              if (g_basiclandtypes_controlled[player][7] < abs((int)global_cards_data[internal_card_id].cc[1]) + global_cards_data[internal_card_id].cc[0])
              {
                missing_land_colors |= 0xff;
              }
            }
          }
        }
      }
      fewest_controlled = 999;
      preferred_land_color = land_colors_needed;
      for (card = 0; (int)card < 6; card++)
      {
        if (0 < g_ai_mana_demand_by_color[player][card])
        {
          missing_land_colors |= 1 << (unsigned char)card;
        }
        if (((land_colors_needed & (1 << (unsigned char)card)) != 0) &&
            (g_basiclandtypes_controlled[player][card] - g_ai_mana_demand_by_color[player][card] < fewest_controlled))
        {
          fewest_controlled = g_basiclandtypes_controlled[player][card] - g_ai_mana_demand_by_color[player][card];
          preferred_land_color = 1 << (unsigned char)card;
        }
      }
      if (land_colors_needed != 0)
      {
        exact_land_colors_needed = preferred_land_color;
        playable_types = missing_land_colors & land_colors_needed;
        if (playable_types != 0)
        {
          exact_land_colors_needed = playable_types;
        }
        playable_types = possible_land_colors & land_colors_needed;
        if (playable_types != 0)
        {
          exact_land_colors_needed = playable_types;
        }
        if ((playable_types == 0) && (colorless_land != 0))
        {
          selected_color = 0xffffffff;
        }
        else
        {
          do
          {
            selected_color = internal_rand(7);
          } while ((exact_land_colors_needed & (1 << (unsigned char)selected_color)) == 0);
        }
        if (g_duel_ai_mode_state == 1)
        {
          g_ai_recorded_choice = selected_color;
          g_ai_recorded_action = -1;
          if ((internal_rand(4) == 0) && (g_ai_search_force_pass == 0))
          {
            g_ai_recorded_choice = -2;
          }
          if ((((possible_land_colors == 0) && (missing_land_colors == 0)) && (colorless_land == 0)) &&
              ((playable_card_count < 7) && (4 <= g_basiclandtypes_controlled[player][7] - nonbasic_land_count)))
          {
            g_ai_recorded_choice = -2;
          }
        }
        else
        {
          g_ai_recorded_action_type = 1;
          replay_ai_action_selection();
          selected_color = g_ai_recorded_choice;
        }
      }
      else
      {
        if ((g_duel_ai_mode_state != 1) && (colorless_land != 0))
        {
          g_ai_recorded_action_type = 1;
          replay_ai_action_selection();
        }
        selected_color = 99;
      }
      result = 0xffffffff;
      for (card = 0; (int)card < g_active_cards_count[player]; card++)
      {
        internal_card_id = global_card_instances[player][card].internal_card_id;
        if (((((internal_card_id != -1) && ((global_card_instances[player][card].state & (STATE_IN_PLAY | STATE_TAPPED)) == 0)) &&
              ((global_cards_data[internal_card_id].type & 1) != 0)) &&
             ((((selected_color == 0xffffffff && (global_card_instances[player][card].mana_color == 0)) ||
                (((1 << (unsigned char)selected_color) & (int)(char)global_card_instances[player][card].mana_color) != 0)) ||
               (land_colors_needed == 0)))) &&
            ((result == 0xffffffff) || (4 < internal_card_id)))
        {
          result = card;
        }
      }
      if ((g_duel_ai_mode_state == 1) && ((land_colors_needed != 0) || (colorless_land != 0)))
      {
        if (g_ai_recorded_choice == -2)
        {
          g_ai_recorded_action = -1;
        }
        else
        {
          g_ai_recorded_action = ((player << 8) | result) | 0x1000;
        }
        g_ai_recorded_action_type = 1;
        record_ai_action_selection();
        if (g_ai_recorded_choice != -2)
        {
          if (g_duel_state_008a8de4 >= 0x10)
          {
            g_duel_state_008a8de4--;
          }
          g_nested_trigger_stack[g_duel_state_008a8de4] = global_card_instances[player][result].internal_card_id;
          g_ai_action_score_table[g_duel_state_008a8de4] = result;
          g_duel_state_008a8de4++;
        }
      }
      if ((g_ai_recorded_choice == -2) && ((land_colors_needed != 0) || (colorless_land != 0)))
      {
        g_land_can_be_played |= LCBP_LAND_HAS_BEEN_PLAYED;
      }
      else
      {
        return result;
      }
    }
  }
  candidate_count = 0;
  ai_castable_phase_mask = 2;
  ai_activatable_phase_mask = 0x20;
  if (PHASE_DECLARE_BLOCKERS <= g_current_phase)
  {
    ai_castable_phase_mask = 4;
    ai_activatable_phase_mask = 0x40;
  }
  if (g_current_phase <= PHASE_MAIN1)
  {
    ai_castable_phase_mask = 1;
    ai_activatable_phase_mask = 0x10;
  }
  if (PHASE_MAIN2 <= g_current_phase)
  {
    ai_castable_phase_mask = 8;
    ai_activatable_phase_mask = 0xffffff80;
  }
  if (g_current_phase == PHASE_DISCARD)
  {
    ai_castable_phase_mask = 0xf;
    ai_activatable_phase_mask = 0xfffffff0;
  }
  for (card = 0; (int)card < g_active_cards_count[player]; card++)
  {
    internal_card_id = global_card_instances[player][card].internal_card_id;
    if (internal_card_id == -1)
    {
      continue;
    }

    if ((global_card_instances[player][card].state & STATE_IN_PLAY) == 0)
    {
      if (((ai_castable_phase_mask & (int)(char)global_cards_data[internal_card_id].act_phases) != 0) &&
          ((global_cards_data[internal_card_id].type & 0x7e) != 0))
      {
        selected_color = single_color_test_bit_to_color_t((int)global_cards_data[internal_card_id].color);
        if ((can_pay_card_mana_cost(player, player, card) != 0) &&
            (((global_cards_data[internal_card_id].type & 0x3c) == 0) ||
             ((*global_cards_data[internal_card_id].code_pointer)(player, card, 0x74) != 0)))
        {
          candidates[candidate_count] = card;
          candidate_count++;
        }
      }
    }
    else if (((((global_card_instances[player][card].state & (STATE_ATTACKING | STATE_TAPPED | STATE_INVISIBLE)) == 0) &&
               ((global_cards_data[internal_card_id].extra_ability & 0x1003U) != 0x1000)) &&
              ((ai_activatable_phase_mask & (int)(char)global_cards_data[internal_card_id].act_phases) != 0)) &&
             (dispatch_event_to_single_card(player, card, 0x73, 1 - player, -1) != 0))
    {
      candidates[candidate_count] = card;
      candidate_count++;
    }
  }
  if (candidate_count == 0)
  {
    g_ai_search_target_depth = -1;
    ai_search_candidate_offset = 0;
    if (g_ai_search_force_pass == 1)
    {
      g_ai_search_time_limit = -1;
    }
    return -1;
  }

  if (g_duel_ai_mode_state == 1)
  {
    if (g_ai_search_best_score == -9999)
    {
      g_ai_search_flags = 0;
    }
    if ((g_ai_search_try_count == 0x19) && (candidate_count < 3))
    {
      g_ai_search_flags |= 1;
    }
    g_ai_recorded_choice = g_ai_search_flags;
    record_ai_action_selection();
  }
  else
  {
    replay_ai_action_selection();
    g_ai_search_flags = g_ai_recorded_choice;
  }
  candidates[candidate_count] = 0xffffffff;
  candidate_count++;
  if (g_duel_ai_mode_state == 1)
  {
    if ((g_ai_search_target_depth == -1) || (g_ai_search_force_pass != 0))
    {
      g_ai_recorded_choice = internal_rand(candidate_count);
      if (g_ai_search_force_pass != 0)
      {
        g_ai_recorded_choice = candidate_count - 1;
        if (g_ai_search_force_pass == 1)
        {
          g_ai_search_time_limit = ClampIntToRange(candidate_count * candidate_count, 10, 0x14) * (g_shandalar_difficulty + 1) * 5;
        }
        g_ai_search_force_pass = -1;
      }
      g_ai_recorded_action = (((global_card_instances[player][candidates[g_ai_recorded_choice]].state & STATE_IN_PLAY) == 0) ? 0x1000 : 0x2000) |
                             candidates[g_ai_recorded_choice] | (player != 0 ? 0x100 : 0);
      g_ai_recorded_action_type = 2;
      record_ai_action_selection();
    }
    else
    {
      ai_depth = get_recorded_action_count();
      if ((ai_search_candidate_offset == 0) && (g_ai_search_target_depth < ai_depth))
      {
        g_ai_search_target_depth = ai_depth;
      }
      if (ai_depth == g_ai_search_target_depth)
      {
        g_ai_recorded_choice = ai_search_candidate_offset;
        ai_search_candidate_offset++;
        if (candidate_count - 1 <= g_ai_recorded_choice)
        {
          g_ai_search_target_depth++;
          ai_search_candidate_offset = 0;
        }
      }
      if (g_ai_search_target_depth < ai_depth)
      {
        g_ai_recorded_choice = candidate_count - 1;
      }
      if (ai_depth < g_ai_search_target_depth)
      {
        g_ai_recorded_action_type = 2;
        replay_ai_action_selection();
        rewind_recorded_action();
        if (g_ai_recorded_choice >= candidate_count)
        {
          g_ai_recorded_choice = candidate_count - 1;
        }
      }
      g_ai_recorded_action = (((global_card_instances[player][candidates[g_ai_recorded_choice]].state & STATE_IN_PLAY) == 0) ? 0x1000 : 0x2000) |
                             candidates[g_ai_recorded_choice] | (player != 0 ? 0x100 : 0);
      g_ai_recorded_action_type = 2;
      record_ai_action_selection();
      if ((candidate_count - 1 == g_ai_recorded_choice) && (ai_depth < g_ai_search_target_depth))
      {
        g_ai_search_target_depth = -1;
        ai_search_candidate_offset = 0;
      }
    }
  }
  else
  {
    g_ai_recorded_action_type = 2;
    replay_ai_action_selection();
    if (g_ai_recorded_choice >= candidate_count)
    {
      g_ai_recorded_choice = candidate_count - 1;
    }
  }
  if (candidates[g_ai_recorded_choice] != 0xffffffff)
  {
    if (g_duel_state_008a8de4 >= 0x10)
    {
      g_duel_state_008a8de4--;
    }
    g_nested_trigger_stack[g_duel_state_008a8de4] = global_card_instances[player][candidates[g_ai_recorded_choice]].internal_card_id;
    g_ai_action_score_table[g_duel_state_008a8de4] = candidates[g_ai_recorded_choice];
    g_duel_state_008a8de4++;
  }

  return candidates[g_ai_recorded_choice];
  return -1;
}

// FUNCTION: MAGIC 0x004e4f11
// FUNCTION: SHANDALAR 0x004c4181
void record_ai_action_selection(void)
{
  if (g_recorded_action_count < 0x100)
  {
    g_trial_recorded_action_targets[g_recorded_action_count] = g_ai_recorded_action;
    g_trial_recorded_action_internal_card_ids[g_recorded_action_count] = PLAYER_CARD_INSTANCE((g_ai_recorded_action & 0x100) >> 8, g_ai_recorded_action & 0xff).internal_card_id;
    g_trial_recorded_action_types[g_recorded_action_count] = g_ai_recorded_action_type;
    g_trial_recorded_action_codes[g_recorded_action_count] = g_ai_recorded_choice;
    ++g_recorded_action_count;
    if (g_trial_recorded_action_codes[0] == 99 || g_recorded_action_codes[0] == 99)
    {
      g_ai_recorded_action = -1;
    }
  }
  else
  {
    g_spell_fizzled = 1;
  }

  g_ai_recorded_action_type = 0;
}

// FUNCTION: MAGIC 0x004e5089
// FUNCTION: SHANDALAR 0x004c42f8
void replay_ai_action_selection(void)
{
  if (g_recorded_action_types[g_recorded_action_count] != g_ai_recorded_action_type)
  {
    g_ai_recorded_action_type |= 0x100;
  }

  g_ai_recorded_action = g_recorded_action_targets[g_recorded_action_count];
  g_ai_recorded_choice = g_recorded_action_codes[g_recorded_action_count];
  if (g_ai_recorded_choice != 99)
  {
    ++g_recorded_action_count;
  }

  g_ai_recorded_action_type = 0;
}

// FUNCTION: MAGIC 0x004e4e9a
// FUNCTION: SHANDALAR 0x004c410a
void __stdcall reset_ai_search_trial_state(void)
{
  int i;

  g_recorded_action_cursor = 0;
  g_recorded_action_count = g_recorded_action_cursor;
  g_response_action_mask = -1;
  for (i = 0; i < 0x100; ++i)
  {
    g_trial_recorded_action_codes[i] = 0x63;
  }
  restore_ai_search_state();
  if (g_duel_ai_mode_state != 1)
  {
    g_ai_decision_code = -1;
  }
}

// FUNCTION: MAGIC 0x004e50f5
// FUNCTION: SHANDALAR 0x004c4364
void __stdcall save_recorded_ai_actions(void)
{
  int action_index;

  for (action_index = 0; action_index < g_recorded_action_count; action_index = action_index + 1)
  {
    g_recorded_action_codes[action_index] = g_trial_recorded_action_codes[action_index];
    g_recorded_action_targets[action_index] = g_trial_recorded_action_targets[action_index];
    g_saved_recorded_action_internal_card_ids[action_index] = g_trial_recorded_action_internal_card_ids[action_index];
    g_recorded_action_types[action_index] = g_trial_recorded_action_types[action_index];
  }

  g_recorded_action_codes[g_recorded_action_count] = 99;
  if (g_recorded_action_codes[0] == 99)
  {
    g_saved_recorded_action_count = g_recorded_action_count;
  }
  g_ai_action_replay_available = 1;
}

// FUNCTION: MAGIC 0x004e51e8
// FUNCTION: SHANDALAR 0x004c4457
int ai_opinion_of_gamestate(int player)
{
  int block_result;
  int power_score;
  int toughness;
  unsigned int abilities;
  unsigned int landwalk_masks[2];
  char attached_to_card[300];
  int card_value;
  int opponent;
  int loop2;
  int life_score;
  int card;
  int damage_power;
  int internal_card_id;
  int side_score;
  int score;
  int current_player;

  g_adventure_duel_internal_card_id = 1;
  score = 0;
  opponent = 1 - player;
  get_landwalk_evasion_masks(&landwalk_masks[0], &landwalk_masks[1]);
  memset(attached_to_card, 0, 0x12c);

  life_score = 0;
  score = life_score;
  for (card = 1; card <= g_life[player]; card = card + 1)
  {
    life_score += 0x18 / card + 0xc;
  }
  score += (g_ai_combat_value_weights[player] * life_score) / 8;

  life_score = 0;
  for (card = 1; card <= g_life[opponent]; card = card + 1)
  {
    life_score += 0x18 / card + 0xc;
  }
  score -= (g_ai_combat_value_weights[opponent] * life_score) / 8;

  if (g_life[player] <= 0)
  {
    score += (g_life[player] * 4 - 8) * 0x4b;
  }
  if (g_life[opponent] <= 0)
  {
    score -= (g_life[opponent] - 2) * 0x100;
  }

  if (g_reveal_all_world_info != 0)
  {
    strcpy(g_ui_message_buffer, "");
  }

  for (current_player = 0; current_player < 2; current_player = current_player + 1)
  {
    opponent = 1 - current_player;
    side_score = -(((g_other_player == current_player ? 0x30 : 0) + 0x18) *
                   g_raw_mana_available[current_player][7]);

    for (card = 1; card < 6; card = card + 1)
    {
      for (loop2 = 1; loop2 <= g_basiclandtypes_controlled[current_player][card]; loop2 = loop2 + 1)
      {
        side_score += 0x30 / loop2;
      }
    }

    for (card = 0; card < g_active_cards_count[current_player]; card = card + 1)
    {
      if (global_card_instances[current_player][card].internal_card_id == -1)
      {
        continue;
      }
      if ((global_card_instances[current_player][card].state & STATE_OUBLIETTED) != 0)
      {
        continue;
      }

      internal_card_id = global_card_instances[current_player][card].internal_card_id;
      if ((global_cards_data[internal_card_id].type & TYPE_EFFECT) != 0)
      {
        continue;
      }

      card_value = 1;
      if ((global_cards_data[internal_card_id].type & TYPE_CREATURE) != 0)
      {
        abilities = C_get_abilities(current_player, card, EVENT_ABILITIES, -1);
        power_score = C_get_abilities(current_player, card, EVENT_POWER, -1) & ~0x4000;
        power_score *= 2;
        damage_power = power_score;
        if (global_cards_data[internal_card_id].subtype == 0)
        {
          damage_power = 0;
          power_score = damage_power;
        }
        toughness = C_get_abilities(current_player, card, EVENT_TOUGHNESS, -1) & ~0x4000;
        card_value = ((power_score + 3) * (toughness + 4)) / 2;

        if ((global_card_instances[current_player][card].state & STATE_TAPPED) != 0 &&
            g_current_player == current_player)
        {
          card_value--;
        }
        if ((abilities & 0x80) != 0)
        {
          card_value = (card_value * 3) / 2;
        }
        if ((abilities & 0x100) != 0)
        {
          card_value = (card_value * 3) / 2;
        }
        if ((global_cards_data[internal_card_id].extra_ability & 3) != 0)
        {
          card_value = (card_value * 3) / 2;
        }
        if ((abilities & 0x40) != 0)
        {
          card_value = ((toughness + 1) * card_value) / 2;
        }
        if ((abilities & 0x200) != 0)
        {
          card_value = (card_value * 3) / 2;
        }

        if (g_duel_state_0093f4b0 == 0 &&
            player != current_player &&
            player == g_current_player &&
            (global_card_instances[current_player][card].state & STATE_IN_PLAY) != 0)
        {
          block_result = 0;
          abilities = C_get_abilities(current_player, card, EVENT_ABILITIES, -1);
          for (loop2 = 0; loop2 < g_active_cards_count[opponent]; loop2 = loop2 + 1)
          {
            if (can_block_attacker_with_abilities(opponent,
                                                  loop2,
                                                  current_player,
                                                  card,
                                                  abilities,
                                                  landwalk_masks[current_player]) != 0)
            {
              block_result |= 1;
              if (C_get_abilities(opponent, loop2, EVENT_TOUGHNESS, card) > power_score ||
                  C_get_abilities(opponent, loop2, EVENT_POWER, card) >= toughness)
              {
                block_result |= 2;
                break;
              }
            }
          }
          if ((block_result & 2) == 0)
          {
            side_score += (g_ai_combat_value_weights[opponent] * damage_power * 0x18) / 0x10;
            if (block_result == 0 && g_life[opponent] <= power_score)
            {
              side_score += 0x100;
            }
          }
        }

        if ((global_card_instances[current_player][card].state & STATE_IN_PLAY) != 0)
        {
          card_value *= 3;
        }
        else if (global_cards_data[internal_card_id].code_pointer == card_ball_lightning)
        {
          card_value = 1;
        }

        card_value = (g_ai_combat_value_weights[current_player + 2] * card_value) / 8;
      }

      if ((global_cards_data[internal_card_id].type & TYPE_LAND) != 0)
      {
        if ((global_card_instances[current_player][card].state & STATE_IN_PLAY) == 0)
        {
          card_value = 2;
        }
        else if ((global_card_instances[current_player][card].state & STATE_TAPPED) == 0)
        {
          card_value = 1;
        }
        else
        {
          card_value = 0;
        }
      }

      if (global_cards_data[internal_card_id].type == TYPE_ARTIFACT &&
          (global_card_instances[current_player][card].state & STATE_IN_PLAY) != 0)
      {
        card_value = ((global_cards_data[internal_card_id].cc[1] * 3 + 3) * 4) / 2;
      }

      if (global_cards_data[internal_card_id].type == TYPE_ENCHANTMENT &&
          (global_card_instances[current_player][card].state & STATE_IN_PLAY) != 0 &&
          (char)global_card_instances[current_player][card].damage_target_player != -1 &&
          global_card_instances[current_player][card].damage_target_card != -1)
      {
        attached_to_card[(int)(char)global_card_instances[current_player][card].damage_target_player * 150 +
                         global_card_instances[current_player][card].damage_target_card] |=
            (char)(1 << (unsigned char)current_player);
      }

      if ((global_cards_data[internal_card_id].type & TYPE_SPELL) != 0 &&
          (global_card_instances[current_player][card].state & STATE_IN_PLAY) == 0)
      {
        card_value = GetCardRarity(internal_card_id) * 0xc;
      }

      if ((global_cards_data[internal_card_id].type & TYPE_ENCHANTMENT) != 0 &&
          (global_card_instances[current_player][card].state & STATE_IN_PLAY) == 0)
      {
        card_value = 3;
      }

      side_score += card_value;
      if ((g_reveal_all_world_info & 2) != 0 && current_player + 2 == g_reveal_all_world_info)
      {
        append_displayed_card_name(current_player, card);
        strcat(g_ui_message_buffer, " ");
        strcat(g_ui_message_buffer, _itoa(card_value, g_itoa_buffer, 10));
        strcat(g_ui_message_buffer, "\n");
      }
    }

    if (current_player == 0)
    {
      g_ai_side_score_player_0 = side_score;
    }
    else
    {
      g_ai_side_score_player_1 = side_score;
    }

    if (player == current_player)
    {
      score += side_score;
    }
    else
    {
      score -= side_score;
    }
  }

  for (current_player = 0; current_player < 2; current_player = current_player + 1)
  {
    for (card = 0; card < g_active_cards_count[current_player]; card = card + 1)
    {
      if (((int)(char)attached_to_card[card + current_player * 150] & (1 << (unsigned char)player)) != 0)
      {
        score += 2;
      }
      if (((int)(char)attached_to_card[card + current_player * 150] & (1 << (unsigned char)(1 - player))) != 0)
      {
        score -= 2;
      }
    }
  }

  if (g_duel_state_0093f4b0 == 0 && player == g_current_player)
  {
    score = ai_opinion_of_gamestate_continued(player, score);
  }
  g_adventure_duel_internal_card_id = 0;
  return score;
}

// FUNCTION: MAGIC 0x004e5db4
// FUNCTION: SHANDALAR 0x004c501b
int ai_opinion_of_gamestate_continued(int player, int score)
{
  int damage_to_player_score;
  int blocker_trade_score;
  int attacker_power;
  int attacker_toughness;
  unsigned int block_result;
  int best_blocker;
  int test_card;
  int opponent_player;
  unsigned int landwalk_mask;
  int expected_damage;
  int card;
  int internal_card_id;
  unsigned int attacker_abilities;
  int blocker_toughness[16];
  int ranked_attackers[24];
  int creature_index;
  int blocker_power[16];
  int test_toughness;
  char creature_index_by_card[152];
  int smallest_blocker_score;
  int blocker_scores[16];
  int test_power;
  int ranked_index;
  int creature_count;
  char saved_special_mana[8];

  opponent_player = 1 - player;
  get_landwalk_evasion_masks(&landwalk_mask, NULL);

  for (card = 0; card <= 7; card = card + 1)
  {
    saved_special_mana[card] = (char)g_special_mana_pool[opponent_player][card];
    g_special_mana_pool[opponent_player][card] = g_basiclandtypes_controlled[opponent_player][card];
  }

  for (ranked_index = 0; ranked_index < 8; ranked_index = ranked_index + 1)
  {
    ranked_attackers[ranked_index * 3 + 1] = -1;
  }

  creature_count = 0;
  for (test_card = 0; test_card < g_active_cards_count[player]; test_card = test_card + 1)
  {
    if (global_card_instances[player][test_card].internal_card_id == -1 ||
        (global_card_instances[player][test_card].state & (STATE_IN_PLAY | STATE_PLAYED_FROM_HAND)) == 0 ||
        (global_card_instances[player][test_card].state & STATE_OUBLIETTED) != 0 ||
        (global_cards_data[global_card_instances[player][test_card].internal_card_id].type & TYPE_CREATURE) == 0)
    {
      continue;
    }

    creature_index_by_card[test_card] = (char)creature_count;
    blocker_power[creature_count] = C_get_abilities(player, test_card, EVENT_POWER, -1);
    blocker_toughness[creature_count] = C_get_abilities(player, test_card, EVENT_TOUGHNESS, -1);
    blocker_scores[creature_count] = (int)global_card_instances[player][test_card].attack_rating;
    creature_count++;
  }

  for (card = 0; card < g_active_cards_count[opponent_player]; card = card + 1)
  {
    internal_card_id = global_card_instances[opponent_player][card].internal_card_id;
    if (is_in_play(opponent_player, card) == 0 ||
        (global_cards_data[internal_card_id].type & TYPE_CREATURE) == 0)
    {
      continue;
    }

    if ((global_cards_data[internal_card_id].subtype == 0) &&
        (global_card_instances[opponent_player][card].token_status & STATUS_WALL_CAN_ATTACK) == 0)
    {
      continue;
    }

    attacker_power = C_get_abilities(opponent_player, card, EVENT_POWER, -1);
    attacker_toughness = C_get_abilities(opponent_player, card, EVENT_TOUGHNESS, -1);
    if (g_active_player == opponent_player)
    {
      if ((global_cards_data[internal_card_id].extra_ability & EA_INF_POWER) != 0)
      {
        attacker_power += (*global_cards_data[internal_card_id].code_pointer)(opponent_player, card, 0x39);
      }
      if ((global_cards_data[internal_card_id].extra_ability & EA_INF_TOUGHNESS) != 0)
      {
        attacker_toughness += (*global_cards_data[internal_card_id].code_pointer)(opponent_player, card, 0x3a);
      }
    }

    for (test_card = 0; test_card < 8; test_card++)
    {
      if (attacker_power > ranked_attackers[test_card * 3 + 1])
      {
        for (ranked_index = 7; test_card < ranked_index; ranked_index = ranked_index - 1)
        {
          ranked_attackers[ranked_index * 3] = ranked_attackers[ranked_index * 3 - 3];
          ranked_attackers[ranked_index * 3 + 1] = ranked_attackers[ranked_index * 3 - 2];
          ranked_attackers[ranked_index * 3 + 2] = ranked_attackers[ranked_index * 3 - 1];
        }
        ranked_attackers[test_card * 3] = card;
        ranked_attackers[test_card * 3 + 1] = attacker_power;
        ranked_attackers[test_card * 3 + 2] = attacker_toughness;
        break;
      }
    }
  }

  expected_damage = 0;
  for (ranked_index = 0; ranked_index < 8; ranked_index++)
  {
    if (ranked_attackers[ranked_index * 3 + 1] == -1)
    {
      break;
    }

    card = ranked_attackers[ranked_index * 3];
    attacker_abilities = C_get_abilities(opponent_player, card, EVENT_ABILITIES, -1);
    attacker_power = ranked_attackers[ranked_index * 3 + 1];
    attacker_toughness = ranked_attackers[ranked_index * 3 + 2];
    best_blocker = 0;
    block_result = best_blocker;
    creature_index = 0;
    smallest_blocker_score = 0x7fff;

    for (test_card = 0; test_card < g_active_cards_count[player]; test_card = test_card + 1)
    {
      if (is_in_play(player, test_card) == 0 || (global_cards_data[global_card_instances[player][test_card].internal_card_id].type & TYPE_CREATURE) == 0)
      {
        continue;
      }
      creature_index = (int)creature_index_by_card[test_card];
      test_power = blocker_power[creature_index];
      test_toughness = blocker_toughness[creature_index];
      if ((global_card_instances[player][test_card].state &
           ((has_vigilance(player, test_card) == 0 ? STATE_ATTACKING : 0) + STATE_BLOCKING)) != 0)
      {
        continue;
      }
      if (can_block_attacker_with_abilities(player,
                                            test_card,
                                            opponent_player,
                                            card,
                                            attacker_abilities,
                                            landwalk_mask) == 0)
      {
        continue;
      }
      block_result |= 1;
      if (attacker_power < test_toughness || attacker_toughness <= test_power)
      {
        block_result |= 2;
        global_card_instances[player][test_card].state |= STATE_BLOCKING;
        break;
      }
      if (blocker_scores[creature_index] < smallest_blocker_score)
      {
        smallest_blocker_score = blocker_scores[creature_index];
        best_blocker = test_card;
      }
    }

    if ((block_result & 2) == 0)
    {
      damage_to_player_score = ((g_ai_combat_value_weights[player] * attacker_power * 0x18) / 4) / ClampIntToRange(g_life[player] + 1, 1, 99);
      blocker_trade_score = (g_ai_combat_value_weights[player + 2] * smallest_blocker_score) / 0x10;
      if (block_result == 0 || (damage_to_player_score < blocker_trade_score && g_life[player] > expected_damage + attacker_power))
      {
        expected_damage += attacker_power;
        score -= damage_to_player_score;
      }
      else
      {
        score -= blocker_trade_score;
        global_card_instances[player][best_blocker].state |= STATE_BLOCKING;
      }
    }
  }

  if (g_life[player] <= expected_damage && g_life[opponent_player] > 0)
  {
    score -= 0x100;
  }

  for (card = 0; card <= 7; card = card + 1)
  {
    g_special_mana_pool[opponent_player][card] = (int)saved_special_mana[card];
  }

  return score;
}

// FUNCTION: MAGIC 0x004e6754
// FUNCTION: SHANDALAR 0x004c59bb
int show_ai_action_log_dialog(int use_saved_actions, int score)
{
  int internal_card_id;
  int action_count;
  unsigned int action_flags;
  int action_index;

  strcpy(g_ui_message_buffer, "AI:");
  strcat(g_ui_message_buffer, _itoa(score, g_itoa_buffer, 10));
  strcat(g_ui_message_buffer, " L:");
  strcat(g_ui_message_buffer, _itoa(g_life[0], g_itoa_buffer, 10));
  strcat(g_ui_message_buffer, "/");
  strcat(g_ui_message_buffer, _itoa(g_life[1], g_itoa_buffer, 10));
  strcat(g_ui_message_buffer, " ...\n");

  for (action_index = 0; (action_count = use_saved_actions != 0 ? g_saved_recorded_action_count : g_recorded_action_count) > action_index; action_index++)
  {
    if (use_saved_actions != 0)
    {
      action_flags = g_recorded_action_targets[action_index];
    }
    else
    {
      action_flags = g_trial_recorded_action_targets[action_index];
    }

    if (action_flags != (unsigned int)-1)
    {
      if ((action_flags & 0x1000) != 0)
      {
        strcat(g_ui_message_buffer, "Cast ");
      }
      if ((action_flags & 0x2000) != 0)
      {
        strcat(g_ui_message_buffer, "Tap ");
      }
      if ((action_flags & 0x4000) != 0)
      {
        strcat(g_ui_message_buffer, "...target ");
      }
      if ((action_flags & 0x100) == 0)
      {
        strcat(g_ui_message_buffer, "e");
      }
      if ((action_flags & 0xff) == 0xff)
      {
        strcat(g_ui_message_buffer, "Player");
      }
      else
      {
        if (use_saved_actions != 0)
        {
          internal_card_id = g_saved_recorded_action_internal_card_ids[action_index];
        }
        else
        {
          internal_card_id = g_trial_recorded_action_internal_card_ids[action_index];
        }
        strcat(g_ui_message_buffer, global_cards_data[internal_card_id].name);
      }
      strcat(g_ui_message_buffer, "\n");
    }
  }

  do_dialog(0, 0, 0, -1, -1, g_ui_message_buffer, 0);
  return 0;
}

// FUNCTION: MAGIC 0x00441cf2
// FUNCTION: SHANDALAR 0x0040d572
void start_ai_decision_search(int decision_code, int time_scale)
{
  g_ai_search_best_score = -9999;
  g_duel_ai_mode_state = 1;
  g_ai_decision_code = decision_code;
  g_ai_search_time_limit = ((g_shandalar_difficulty + 1) * time_scale) / 4;

  g_ai_land_score_modifier = 1 << (unsigned char)(internal_rand(5) + 1);

  save_ai_search_state();
#ifdef SHANDALAR
  ConsumeUiTickCount();
#else
  reset_duel_tick_timer_indirect();
#endif
  update_duel_thread_time_marker();

  g_ai_search_try_count = 0;
  g_ai_search_force_pass = 1;
#ifdef SHANDALAR
  HideMouseCursorNested();
  NoopSetSpecialSprite(1, 1, g_ttsprite_special_sprite_b);
  ShowMouseCursorNested();
#endif
  g_adventure_duel_internal_card_id = 0;
}

// FUNCTION: MAGIC 0x004e430e
// FUNCTION: SHANDALAR 0x004c357e
void save_ai_search_state(void)
{
  g_saved_recorded_action_count = 0;
  memcpy(g_ai_search_backup.card_instances, global_card_instances, sizeof(g_ai_search_backup.card_instances));
  memcpy(g_ai_search_backup.extra_card_data, global_cards_data + g_card_count, sizeof(g_ai_search_backup.extra_card_data));
  memcpy(g_ai_search_backup.library, global_library, sizeof(g_ai_search_backup.library));
  memcpy(g_ai_search_backup.graveyard, global_graveyard_slots, sizeof(g_ai_search_backup.graveyard));
  memcpy(g_ai_search_backup.exile, global_exile, sizeof(g_ai_search_backup.exile));
  memcpy(g_ai_search_backup.special_mana, g_special_mana_pool, sizeof(g_ai_search_backup.special_mana));
  memcpy(g_ai_search_backup.raw_mana, g_raw_mana_available, sizeof(g_ai_search_backup.raw_mana));
  memcpy(g_ai_search_backup.basic_lands, g_basiclandtypes_controlled, sizeof(g_ai_search_backup.basic_lands));
  memcpy(g_ai_search_backup.raw_mana_hex, g_raw_mana_available_hex, sizeof(g_ai_search_backup.raw_mana_hex));
  memcpy(g_ai_search_backup.life_totals, g_life, sizeof(g_ai_search_backup.life_totals));
  memcpy(g_ai_search_backup.poison_counters, g_poison_counters, sizeof(g_ai_search_backup.poison_counters));
  memcpy(g_ai_search_backup.card_types, g_card_types_in_play, sizeof(g_ai_search_backup.card_types));
  memcpy(g_ai_search_backup.graveyard_type_flags, g_graveyard_card_types, sizeof(g_ai_search_backup.graveyard_type_flags));
  memcpy(g_ai_search_backup.duel_summary_copy, &g_duel_summary, sizeof(g_ai_search_backup.duel_summary_copy));
  memcpy(g_ai_search_backup.hand_counts, g_duel_summary.hand_counts, sizeof(g_ai_search_backup.hand_counts));
  memcpy(g_ai_search_backup.creature_counts, g_duel_summary.creature_counts, sizeof(g_ai_search_backup.creature_counts));
  memcpy(g_ai_search_backup.artifact_counts, g_duel_summary.artifact_counts, sizeof(g_ai_search_backup.artifact_counts));
  memcpy(g_ai_search_backup.enchantment_counts, g_duel_summary.enchantment_counts, sizeof(g_ai_search_backup.enchantment_counts));
  g_ai_search_backup.cards_drawn_value = g_duel_summary.cards_drawn;
  g_ai_search_backup.creatures_died_this_turn_value = g_duel_summary.creatures_died;
  g_ai_search_backup.land_entries_this_turn_value = g_duel_summary.land_entries;
  memcpy(g_ai_search_backup.ante_cards, global_ante_cards, sizeof(g_ai_search_backup.ante_cards));
  memcpy(g_ai_search_backup.timestamp_player, TENTATIVE_timestamp_player, sizeof(g_ai_search_backup.timestamp_player));
  memcpy(g_ai_search_backup.timestamp_card, TENTATIVE_timestamp_card, sizeof(g_ai_search_backup.timestamp_card));
  g_ai_search_backup.land_play_flags = g_land_can_be_played;
  g_ai_search_backup.phase = g_current_phase;
  g_ai_response_state = g_ai_search_backup.phase;
  g_ai_search_backup.current_casting_internal_card_id_value = g_current_casting_internal_card_id;
  g_ai_search_backup.combat_count = g_attacking_creature_count;
  g_ai_search_backup.spell_fizzled_value = g_spell_fizzled;
  memcpy(g_ai_search_backup.stack_events, g_stack_data, sizeof(g_ai_search_backup.stack_events));
  memcpy(g_ai_search_backup.stack_cards, global_stack_cards, sizeof(g_ai_search_backup.stack_cards));
  memcpy(g_ai_search_backup.stack_damage_targets, global_stack_damage_targets, sizeof(g_ai_search_backup.stack_damage_targets));
  memcpy(g_ai_search_backup.active_counts, g_active_cards_count, sizeof(g_ai_search_backup.active_counts));
  if (g_stack_size < 0)
  {
    _assert("ScWilly>=0", "D:\\Newmagic\\multiplayer\\sid\\Ai.c", 0x1cb);
  }
  g_ai_search_backup.stack_count = g_stack_size;
  g_ai_search_backup.pending_killed_card_handler_value = pending_killed_card_handler;
  g_ai_search_backup.duel_state_0093a848_value = g_duel_state_0093a848;
  memcpy(g_ai_search_backup.damage_matrix, g_damage_accumulators, sizeof(g_ai_search_backup.damage_matrix));
  memcpy(g_ai_search_backup.cost_mod, g_mana_cost_modifiers_by_color, sizeof(g_ai_search_backup.cost_mod));
  memcpy(g_ai_search_backup.mana_charge, g_mana_charge, sizeof(g_ai_search_backup.mana_charge));
  g_ai_search_backup.x_value_copy = g_x_value;
  g_ai_search_backup.max_x_value_copy = g_max_x_value;
  g_ai_search_backup.extra_turn_player = g_duel_extra_turn_player;
  g_ai_search_backup.duel_state_0093a848_value = g_duel_state_0093a848;
  g_ai_search_backup.stop_phase_copy = g_stop_phase;
  g_ai_search_backup.stop_phase_player_copy = g_stop_phase_player;
  g_ai_search_backup.battlefield_flags = g_battlefield_extra_ability_flags;
  memcpy(g_ai_search_backup.lich_active_copy, g_lich_active, sizeof(g_ai_search_backup.lich_active_copy));
  memcpy(g_ai_search_backup.player_special_effect_flags_copy, g_player_special_effect_flags, sizeof(g_ai_search_backup.player_special_effect_flags_copy));
#ifndef SHANDALAR
  StopWorldLocationMusic();
#else
  refresh_ai_random_table();
#endif
}

// FUNCTION: MAGIC 0x004e4632
// FUNCTION: SHANDALAR 0x004c38a2
void restore_ai_search_state(void)
{
  memcpy(global_card_instances, g_ai_search_backup.card_instances, sizeof(g_ai_search_backup.card_instances));
  memcpy(global_cards_data + g_card_count, g_ai_search_backup.extra_card_data, sizeof(g_ai_search_backup.extra_card_data));
  memcpy(global_library, g_ai_search_backup.library, sizeof(g_ai_search_backup.library));
  memcpy(global_graveyard_slots, g_ai_search_backup.graveyard, sizeof(g_ai_search_backup.graveyard));
  memcpy(global_exile, g_ai_search_backup.exile, sizeof(g_ai_search_backup.exile));
  memcpy(g_special_mana_pool, g_ai_search_backup.special_mana, sizeof(g_ai_search_backup.special_mana));
  memcpy(g_raw_mana_available, g_ai_search_backup.raw_mana, sizeof(g_ai_search_backup.raw_mana));
  memcpy(g_basiclandtypes_controlled, g_ai_search_backup.basic_lands, sizeof(g_ai_search_backup.basic_lands));
  memcpy(g_raw_mana_available_hex, g_ai_search_backup.raw_mana_hex, sizeof(g_ai_search_backup.raw_mana_hex));
  memcpy(g_life, g_ai_search_backup.life_totals, sizeof(g_ai_search_backup.life_totals));
  memcpy(g_poison_counters, g_ai_search_backup.poison_counters, sizeof(g_ai_search_backup.poison_counters));
  memcpy(g_card_types_in_play, g_ai_search_backup.card_types, sizeof(g_ai_search_backup.card_types));
  memcpy(g_graveyard_card_types, g_ai_search_backup.graveyard_type_flags, sizeof(g_ai_search_backup.graveyard_type_flags));
  memcpy(&g_duel_summary, g_ai_search_backup.duel_summary_copy, sizeof(g_ai_search_backup.duel_summary_copy));
  memcpy(g_duel_summary.hand_counts, g_ai_search_backup.hand_counts, sizeof(g_ai_search_backup.hand_counts));
  memcpy(g_duel_summary.creature_counts, g_ai_search_backup.creature_counts, sizeof(g_ai_search_backup.creature_counts));
  memcpy(g_duel_summary.artifact_counts, g_ai_search_backup.artifact_counts, sizeof(g_ai_search_backup.artifact_counts));
  memcpy(g_duel_summary.enchantment_counts, g_ai_search_backup.enchantment_counts, sizeof(g_ai_search_backup.enchantment_counts));
  g_duel_summary.cards_drawn = g_ai_search_backup.cards_drawn_value;
  g_duel_summary.creatures_died = g_ai_search_backup.creatures_died_this_turn_value;
  g_duel_summary.land_entries = g_ai_search_backup.land_entries_this_turn_value;
  memcpy(global_ante_cards, g_ai_search_backup.ante_cards, sizeof(g_ai_search_backup.ante_cards));
  memcpy(TENTATIVE_timestamp_player, g_ai_search_backup.timestamp_player, sizeof(g_ai_search_backup.timestamp_player));
  memcpy(TENTATIVE_timestamp_card, g_ai_search_backup.timestamp_card, sizeof(g_ai_search_backup.timestamp_card));
  g_land_can_be_played = g_ai_search_backup.land_play_flags;
  g_current_phase = g_ai_search_backup.phase;
  g_current_casting_internal_card_id = g_ai_search_backup.current_casting_internal_card_id_value;
  g_attacking_creature_count = g_ai_search_backup.combat_count;
  g_spell_fizzled = g_ai_search_backup.spell_fizzled_value;
  memcpy(g_stack_data, g_ai_search_backup.stack_events, sizeof(g_ai_search_backup.stack_events));
  memcpy(global_stack_cards, g_ai_search_backup.stack_cards, sizeof(g_ai_search_backup.stack_cards));
  memcpy(global_stack_damage_targets, g_ai_search_backup.stack_damage_targets, sizeof(g_ai_search_backup.stack_damage_targets));
  memcpy(g_active_cards_count, g_ai_search_backup.active_counts, sizeof(g_ai_search_backup.active_counts));
  g_stack_size = g_ai_search_backup.stack_count;
  pending_killed_card_handler = g_ai_search_backup.pending_killed_card_handler_value;
  g_duel_state_0093a848 = g_ai_search_backup.duel_state_0093a848_value;
  memcpy(g_damage_accumulators, g_ai_search_backup.damage_matrix, sizeof(g_ai_search_backup.damage_matrix));
  memcpy(g_mana_cost_modifiers_by_color, g_ai_search_backup.cost_mod, sizeof(g_ai_search_backup.cost_mod));
  memcpy(g_mana_charge, g_ai_search_backup.mana_charge, sizeof(g_ai_search_backup.mana_charge));
  g_x_value = g_ai_search_backup.x_value_copy;
  g_max_x_value = g_ai_search_backup.max_x_value_copy;
  g_duel_extra_turn_player = g_ai_search_backup.extra_turn_player;
  g_duel_state_0093a848 = g_ai_search_backup.duel_state_0093a848_value;
  g_stop_phase = g_ai_search_backup.stop_phase_copy;
  g_stop_phase_player = g_ai_search_backup.stop_phase_player_copy;
  g_battlefield_extra_ability_flags = g_ai_search_backup.battlefield_flags;
  memcpy(g_lich_active, g_ai_search_backup.lich_active_copy, sizeof(g_ai_search_backup.lich_active_copy));
  memcpy(g_player_special_effect_flags, g_ai_search_backup.player_special_effect_flags_copy, sizeof(g_ai_search_backup.player_special_effect_flags_copy));
  FUN_00464bb0();
}

// FUNCTION: MAGIC 0x004a7ccf
// FUNCTION: SHANDALAR 0x00559e52
int get_ai_search_elapsed_time(void)
{
  uint32_t current_time;
  uint32_t start_time;
  uint32_t elapsed_time;

  current_time = (uint32_t)get_duel_thread_time_ms();
  start_time = (uint32_t)g_ai_trace_enabled;
  elapsed_time = current_time - start_time;
  return (int)(((uint64_t)elapsed_time * 100u) / 0x151d);
}

// FUNCTION: MAGIC 0x0050e79b
// FUNCTION: SHANDALAR 0x0041d33b
void setup_ai_combat_abilities(int player)
{
  int attached_player;
  unsigned int saved_state;
  int saved_ai_mode;
  int defending_player;
  int saved_combat_eval_flag;
  int card;
  int mana_available;
  int attached_card;
  int internal_card_id;

  defending_player = 1 - player;
  saved_ai_mode = g_duel_ai_mode_state;
  saved_combat_eval_flag = g_duel_state_0093f4b0;
  g_duel_ai_mode_state = 1;
  g_duel_state_0093f4b0 = 1;
  untapped_royal_assassin_count = 0;
  memset(ai_combat_eval_table, 0, sizeof(ai_combat_eval_table));
  for (card = 0; card < g_active_cards_count[player]; card++)
  {
    if (is_in_play(player, card) == 0)
    {
      continue;
    }
    internal_card_id = global_card_instances[player][card].internal_card_id;
    attached_player = (int)(char)global_card_instances[player][card].damage_target_player;
    attached_card = global_card_instances[player][card].damage_target_card;
    if ((global_cards_data[internal_card_id].code_pointer == card_regeneration) &&
        has_mana(attached_player, COLOR_GREEN, 1) != 0)
    {
      ai_combat_eval_table[attached_player][attached_card].abilities |= 0x200;
      global_card_instances[attached_player][attached_card].regen_status |= 0x200;
    }
    else if ((global_cards_data[internal_card_id].code_pointer == card_the_brute) &&
             has_mana(attached_player, COLOR_RED, 3) != 0)
    {
      ai_combat_eval_table[attached_player][attached_card].abilities |= 0x200;
      global_card_instances[attached_player][attached_card].regen_status |= 0x200;
    }
    if (global_cards_data[internal_card_id].code_pointer == card_holy_armor)
    {
      ai_combat_eval_table[attached_player][attached_card].toughness += has_mana(attached_player, COLOR_WHITE, 1);
    }
    else if (global_cards_data[internal_card_id].code_pointer == card_firebreathing)
    {
      ai_combat_eval_table[attached_player][attached_card].power += has_mana(attached_player, COLOR_RED, 1);
    }
    else if (global_cards_data[internal_card_id].code_pointer == card_blessing)
    {
      ai_combat_eval_table[attached_player][attached_card].power += has_mana(attached_player, COLOR_WHITE, 1);
      ai_combat_eval_table[attached_player][attached_card].toughness += has_mana(attached_player, COLOR_WHITE, 1);
    }
    else if (global_cards_data[internal_card_id].code_pointer == card_uncle_istvan)
    {
      ai_combat_eval_table[player][card].abilities |= 0x1ff800;
    }
    else if (global_cards_data[internal_card_id].code_pointer == card_murk_dwellers)
    {
      ai_combat_eval_table[player][card].abilities |= 0x80000000;
    }
    else if (global_cards_data[internal_card_id].code_pointer == card_gaseous_form)
    {
      ai_combat_eval_table[attached_player][attached_card].abilities |= 0x1ff800;
      ai_combat_eval_table[attached_player][attached_card].power = 0xffffff9d;
    }
    global_card_instances[player][card].regen_status |= 0xe000000;
    saved_state = global_card_instances[player][card].state;
    if (g_other_player == player)
    {
      if (global_card_instances[player][card].state & STATE_VIGILANCE)
      {
        global_card_instances[player][card].state |= STATE_ATTACKING;
      }
      else
      {
        global_card_instances[player][card].state |= STATE_TAPPED | STATE_ATTACKING;
      }
    }
    combat_eval_power = C_get_abilities(player, card, EVENT_POWER, -1);
    combat_eval_toughness = C_get_abilities(player, card, EVENT_TOUGHNESS, -1);
    combat_eval_abilities = C_get_abilities(player, card, EVENT_ABILITIES, -1);
    if (((combat_eval_abilities & 0x200) != 0) &&
        has_mana(player, single_color_test_bit_to_color_t((int)global_cards_data[internal_card_id].color), 1) == 0)
    {
      combat_eval_abilities &= 0xfffffdff;
    }
    if (g_active_player == player)
    {
      dispatch_event(player, card, EVENT_CHECK_ABILITIES);
    }
    ai_combat_eval_table[player][card].power += combat_eval_power;
    ai_combat_eval_table[player][card].toughness += combat_eval_toughness;
    ai_combat_eval_table[player][card].abilities |= combat_eval_abilities;
    global_card_instances[player][card].state = saved_state;
  }
  defending_cop_color_mask = 0;
  for (card = 0; card < g_active_cards_count[defending_player]; card++)
  {
    if (is_in_play(defending_player, card) == 0)
    {
      continue;
    }
    internal_card_id = global_card_instances[defending_player][card].internal_card_id;
    attached_player = (int)(char)global_card_instances[defending_player][card].damage_target_player;
    attached_card = global_card_instances[defending_player][card].damage_target_card;
    if ((global_cards_data[internal_card_id].code_pointer == card_regeneration) &&
        has_mana(attached_player, COLOR_GREEN, 1) != 0)
    {
      ai_combat_eval_table[attached_player][attached_card].abilities |= 0x200;
      global_card_instances[attached_player][attached_card].regen_status |= 0x200;
    }
    else if ((global_cards_data[internal_card_id].code_pointer == card_the_brute) &&
             has_mana(attached_player, COLOR_RED, 3) != 0)
    {
      ai_combat_eval_table[attached_player][attached_card].abilities |= 0x200;
      global_card_instances[attached_player][attached_card].regen_status |= 0x200;
    }
    if (global_cards_data[internal_card_id].code_pointer == card_holy_armor)
    {
      ai_combat_eval_table[attached_player][attached_card].toughness += has_mana(attached_player, COLOR_WHITE, 1);
    }
    else if (global_cards_data[internal_card_id].code_pointer == card_firebreathing)
    {
      ai_combat_eval_table[attached_player][attached_card].power += has_mana(attached_player, COLOR_RED, 1);
    }
    else if (global_cards_data[internal_card_id].code_pointer == card_blessing)
    {
      ai_combat_eval_table[attached_player][attached_card].power += has_mana(attached_player, COLOR_WHITE, 1);
      ai_combat_eval_table[attached_player][attached_card].toughness += has_mana(attached_player, COLOR_WHITE, 1);
    }
    else if ((global_cards_data[internal_card_id].code_pointer == card_royal_assassin) &&
             ((global_card_instances[defending_player][card].state & STATE_SUMMONSICK_BOTH) == 0))
    {
      untapped_royal_assassin_count++;
    }
    else if (global_cards_data[internal_card_id].code_pointer == card_uncle_istvan)
    {
      ai_combat_eval_table[defending_player][card].abilities |= 0x1ff800;
    }
    else if (global_cards_data[internal_card_id].code_pointer == card_murk_dwellers)
    {
      ai_combat_eval_table[defending_player][card].abilities |= 0x80000000;
    }
    else if (global_cards_data[internal_card_id].code_pointer == card_gaseous_form)
    {
      ai_combat_eval_table[attached_player][attached_card].abilities |= 0x1ff800;
      ai_combat_eval_table[attached_player][attached_card].power = 0xffffff9d;
    }
    global_card_instances[defending_player][card].regen_status |= 0xe000000;
    saved_state = global_card_instances[defending_player][card].state;
    global_card_instances[defending_player][card].state |= STATE_BLOCKING;
    combat_eval_power = C_get_abilities(defending_player, card, EVENT_POWER, -1);
    combat_eval_toughness = C_get_abilities(defending_player, card, EVENT_TOUGHNESS, -1);
    combat_eval_abilities = C_get_abilities(defending_player, card, EVENT_ABILITIES, -1);
    if (((combat_eval_abilities & 0x200) != 0) &&
        has_mana(defending_player, single_color_test_bit_to_color_t((int)global_cards_data[internal_card_id].color), 1) == 0)
    {
      combat_eval_abilities &= 0xfffffdff;
    }
    if (g_active_player == defending_player)
    {
      dispatch_event(defending_player, card, EVENT_CHECK_ABILITIES);
    }
    ai_combat_eval_table[defending_player][card].power += combat_eval_power;
    ai_combat_eval_table[defending_player][card].toughness += combat_eval_toughness;
    ai_combat_eval_table[defending_player][card].abilities |= combat_eval_abilities;
    global_card_instances[defending_player][card].state = saved_state;
    internal_card_id = global_card_instances[defending_player][card].internal_card_id;
    if (global_cards_data[internal_card_id].code_pointer == card_co_p_black)
    {
      defending_cop_color_mask |= 2;
    }
    if (global_cards_data[internal_card_id].code_pointer == card_co_p_blue)
    {
      defending_cop_color_mask |= 4;
    }
    if (global_cards_data[internal_card_id].code_pointer == card_co_p_green)
    {
      defending_cop_color_mask |= 8;
    }
    if (global_cards_data[internal_card_id].code_pointer == card_co_p_red)
    {
      defending_cop_color_mask |= 0x10;
    }
    if (global_cards_data[internal_card_id].code_pointer == card_co_p_white)
    {
      defending_cop_color_mask |= 0x20;
    }
  }
  if (defending_cop_color_mask != 0)
  {
    defending_cop_available_mana = has_mana(defending_player, COLOR_ANY, 0);
  }
  else
  {
    defending_cop_available_mana = 0;
  }
  reassess_all_cards_and_mana();
  g_duel_ai_mode_state = saved_ai_mode;
  g_duel_state_0093f4b0 = saved_combat_eval_flag;
}

// FUNCTION: MAGIC 0x00516ecf
// FUNCTION: SHANDALAR 0x00425a82
int score_ai_attacker_candidate(int player, int card)
{
  int adjusted_toughness_plus_3;
  int adjusted_power_plus_5;
  int adjusted_power_plus_2;
  int adjusted_toughness;
  int internal_card_id;
  int adjusted_power;
  int adjusted_toughness_plus_2;
  int score;

  internal_card_id = global_card_instances[player][card].internal_card_id;
  adjusted_power = C_get_abilities(player, card, EVENT_POWER, -1);
  adjusted_toughness = C_get_abilities(player, card, EVENT_TOUGHNESS, -1);
  if (adjusted_power != 0)
  {
    adjusted_power_plus_5 = adjusted_power + 5;
  }
  else
  {
    adjusted_power_plus_5 = adjusted_power;
  }
  adjusted_power_plus_2 = adjusted_power_plus_5 + 2;
  if (adjusted_toughness != 0)
  {
    adjusted_toughness_plus_3 = adjusted_toughness + 3;
  }
  else
  {
    adjusted_toughness_plus_3 = adjusted_toughness;
  }
  adjusted_toughness_plus_2 = adjusted_toughness_plus_3 + 2;
  score = (global_cards_data[internal_card_id].ai_base_value + adjusted_power_plus_2) * adjusted_toughness_plus_2 * 5;
  if ((global_cards_data[internal_card_id].static_ability & 0x1f) != 0)
  {
    score = (score * 3) / 2;
  }
  if ((global_cards_data[internal_card_id].static_ability & KEYWORD_REGENERATION) != 0)
  {
    score = (score * 3) / 2;
  }
  if ((global_cards_data[internal_card_id].extra_ability & 3) != 0)
  {
    score /= 2;
  }
  if (global_cards_data[internal_card_id].code_pointer != card_dummy)
  {
    score = (score * 3) / 2;
  }
  if ((global_cards_data[internal_card_id].static_ability & 0x1c0) != 0)
  {
    score = (score * 3) / 2;
  }
  if ((global_cards_data[internal_card_id].extra_ability & 8) != 0)
  {
    score *= 3;
  }
  if ((global_cards_data[internal_card_id].extra_ability & 0x10) != 0)
  {
    score *= 3;
  }
  if ((global_card_instances[player][card].state & STATE_UNKNOWN8000) != 0)
  {
    score <<= 1;
  }
  return score;
}

// FUNCTION: MAGIC 0x00510a3f
// FUNCTION: SHANDALAR 0x0041f5e3
unsigned int choose_attackers_ai(int player)
{
  int pad_518;
  unsigned int source_mask;
  unsigned int temp_mask;
  int sort_temp;
  int highest_power;
  int highest_power_index;
  int remaining_power;
  int untapped_blocker_count;
  int candidate_cards[16];
  unsigned int block_result_mask;
  int blocker_power_value;
  int blocker_toughness_value;
  int band_anchor_card;
  int blocker_power[32];
  int blocker_toughness[16];
  char blocker_index_by_card[152];
  unsigned int saved_attacker_abilities[16];
  int best_blocker_card;
  unsigned int band_regeneration_ability;
  unsigned int subset_mask;
  int subset_legal;
  int inner;
  unsigned int landwalk_mask;
  int card_id;
  int expected_damage;
  int card;
  unsigned int selected_mask;
  int saved_event_state;
  int band_candidate_power;
  int saved_attacker_scores[16];
  int best_score;
  int candidate_count;
  int pressure_score;
  int saved_ai_mode;
  int band_candidate_count;
  int band_candidate_cards[16];
  int internal_card_id;
  unsigned int attacker_abilities;
  int own_toughness[14];
  int ranked_blockers[18];
  int own_creature_index;
  int candidate_powers[16];
  int test_toughness;
  char own_index_by_card[152];
  int highest_blocker_toughness;
  int candidate_scores[16];
  int test_power;
  int smallest_blocker_score;
  int assassin_count;
  int ranked_index;
  int cost_to_chump;
  int blocker_creature_index;
  unsigned int forced_attack_mask;
  int saved_attacker_damage[16];
  int saved_attacker_toughness[16];
  unsigned int band_common_abilities;

  setup_ai_combat_abilities(player);
  saved_event_state = g_adventure_duel_internal_card_id;
  g_adventure_duel_internal_card_id = 2;
  saved_ai_mode = g_duel_ai_mode_state;
  if (g_duel_ai_mode_state == 1)
  {
    g_ai_search_flags ^= 2;
  }

  for (card = 0; card < g_active_cards_count[player]; card++)
  {
    AI_CARD_STATE(player, card) &= ~STATE_ATTACKING;
  }
  C_dispatch_event_raw(0x15);

  band_regeneration_ability = 0;
  band_candidate_power = band_regeneration_ability;
  band_candidate_count = band_candidate_power;
  forced_attack_mask = band_candidate_count;
  candidate_count = forced_attack_mask;
  band_common_abilities = 0xffffffff;
  memset(candidate_cards, 0, sizeof(candidate_cards));
  for (card = 0; card < g_active_cards_count[player]; card++)
  {
    internal_card_id = AI_CARD_INTERNAL_ID(player, card);
    if ((internal_card_id != -1) &&
        (((global_cards_data[internal_card_id].type & TYPE_CREATURE) != 0 ||
          ((AI_CARD_STATE(player, card) & STATE_NONCREATURE_CAN_ATTACK) != 0)) &&
         ((AI_CARD_STATE(player, card) & 0x820012) == STATE_IN_PLAY)) &&
        (can_attack(player, card) != 0))
    {
      if ((AI_CARD_STATE(player, card) & STATE_UNKNOWN8000) != 0)
      {
        forced_attack_mask |= 1 << (unsigned char)candidate_count;
      }
      if (((g_ai_search_flags & 2) != 0) &&
          ((AI_CARD_STATE(player, card) & STATE_UNKNOWN8000) == 0) &&
          ((AI_EVAL(player, card).abilities & KEYWORD_BANDING) != 0))
      {
        band_candidate_power += AI_EVAL(player, card).power;
        band_common_abilities &= AI_EVAL(player, card).abilities;
        band_regeneration_ability |= AI_EVAL(player, card).abilities & KEYWORD_REGENERATION;
        band_candidate_cards[band_candidate_count] = card;
        band_candidate_count++;
      }
      else
      {
        candidate_scores[candidate_count] = score_ai_attacker_candidate(player, card);
        candidate_cards[candidate_count] = card;
        candidate_count++;
        AI_CARD_STATE(player, card) |= STATE_ATTACKING;
      }
    }
  }
  band_common_abilities |= band_regeneration_ability;

  if (candidate_count > 7)
  {
    untapped_blocker_count = 0;
    for (card = 0; card < g_active_cards_count[ai_blocker_player]; card++)
    {
      internal_card_id = AI_CARD_INTERNAL_ID(ai_blocker_player, card);
      if ((is_in_play(ai_blocker_player, card) == 0) ||
          ((global_cards_data[internal_card_id].type & TYPE_CREATURE) == 0) ||
          ((AI_CARD_STATE(ai_blocker_player, card) & STATE_TAPPED) != 0))
      {
        continue;
      }
      untapped_blocker_count++;
    }
    for (card = 0; card < 16; card++)
    {
      candidate_powers[card] = 0;
    }
    for (card = 0; card < candidate_count; card++)
    {
      candidate_powers[card] = AI_EVAL(player, candidate_cards[card]).power;
    }
    while (untapped_blocker_count-- != 0)
    {
      highest_power = 0;
      highest_power_index = 0;
      for (card = 0; card < candidate_count; card++)
      {
        if (highest_power < candidate_powers[card])
        {
          highest_power = candidate_powers[card];
          highest_power_index = card;
        }
      }
      candidate_powers[highest_power_index] = 0;
    }
    remaining_power = 0;
    for (card = 0; card < candidate_count; card++)
    {
      remaining_power += candidate_powers[card];
    }
    if (g_life[1 - player] <= remaining_power)
    {
      for (card = 0; card < candidate_count; card++)
      {
        AI_CARD_STATE(player, candidate_cards[card]) |= STATE_ATTACKING;
      }
      g_attacking_creature_count = (1 << (unsigned char)candidate_count) - 1;
      return g_attacking_creature_count;
    }
  }

  if (candidate_count > 7)
  {
    for (card = 0; card < candidate_count; card++)
    {
      for (inner = card; inner < candidate_count; inner++)
      {
        if (candidate_scores[inner] > candidate_scores[card])
        {
          sort_temp = candidate_scores[card];
          candidate_scores[card] = candidate_scores[inner];
          candidate_scores[inner] = sort_temp;
          sort_temp = candidate_cards[card];
          candidate_cards[card] = candidate_cards[inner];
          candidate_cards[inner] = sort_temp;
          temp_mask = (1 << (unsigned char)card) & forced_attack_mask;
          forced_attack_mask &= ~(1 << (unsigned char)card);
          source_mask = (1 << (unsigned char)inner) & forced_attack_mask;
          forced_attack_mask &= ~(1 << (unsigned char)inner);
          if (temp_mask != 0)
          {
            forced_attack_mask |= 1 << (unsigned char)inner;
          }
          if (source_mask != 0)
          {
            forced_attack_mask |= 1 << (unsigned char)card;
          }
        }
      }
    }
    candidate_count = candidate_count < 7 ? candidate_count : 7;
    for (card = 7; card < 16; card++)
    {
      AI_CARD_STATE(player, candidate_cards[card]) &= ~STATE_ATTACKING;
    }
    memset(&candidate_cards[7], 0, 36);
    forced_attack_mask = 0;
    candidate_count = forced_attack_mask;
    for (card = 0; card < g_active_cards_count[player]; card++)
    {
      if ((AI_CARD_STATE(player, card) & STATE_ATTACKING) != 0)
      {
        if ((AI_CARD_STATE(player, card) & STATE_UNKNOWN8000) != 0)
        {
          forced_attack_mask |= 1 << (unsigned char)candidate_count;
        }
        candidate_cards[candidate_count] = card;
        candidate_count++;
      }
    }
  }

  setup_combat_damage_simulation(player);
  memcpy(saved_attacker_damage, combat_damage_attacker_damage, sizeof(saved_attacker_damage));
  memcpy(saved_attacker_toughness, combat_damage_attacker_toughness, sizeof(saved_attacker_toughness));
  memcpy(saved_attacker_abilities, combat_damage_attacker_abilities, sizeof(saved_attacker_abilities));
  memcpy(saved_attacker_scores, g_ai_attacker_cards, sizeof(saved_attacker_scores));
  memcpy(blocker_toughness, g_ai_attacker_power, sizeof(blocker_toughness));
  memcpy(g_ai_attacker_damage, g_ai_attacker_toughness, sizeof(g_ai_attacker_damage));
  memcpy(g_ai_attacker_scores, g_ai_blocker_power, sizeof(g_ai_attacker_scores));

  save_combat_simulation_state();
  g_duel_ai_mode_state = 1;
  C_dispatch_event_raw(199);
  g_duel_ai_mode_state = saved_ai_mode;

  for (card = 0; card <= 7; card++)
  {
    g_special_mana_pool[ai_blocker_player][card] = g_basiclandtypes_controlled[ai_blocker_player][card];
  }

  highest_blocker_toughness = 0;
  blocker_creature_index = highest_blocker_toughness;
  for (card = 0; card < g_active_cards_count[ai_blocker_player]; card++)
  {
    internal_card_id = AI_CARD_INTERNAL_ID(ai_blocker_player, card);
    if ((is_in_play(ai_blocker_player, card) == 0) ||
        ((global_cards_data[internal_card_id].type & TYPE_CREATURE) == 0))
    {
    }
    else
    {
      blocker_power[blocker_creature_index] = AI_EVAL(ai_blocker_player, card).power;
      blocker_power[blocker_creature_index + 16] = AI_EVAL(ai_blocker_player, card).toughness;
      if ((global_cards_data[internal_card_id].extra_ability & 8) != 0)
      {
        blocker_power[blocker_creature_index] +=
            (*global_cards_data[internal_card_id].code_pointer)(ai_blocker_player, card, EVENT_POW_BOOST);
      }
      if ((global_cards_data[internal_card_id].extra_ability & 0x10) != 0)
      {
        blocker_power[blocker_creature_index + 16] +=
            (*global_cards_data[internal_card_id].code_pointer)(ai_blocker_player, card, EVENT_TOU_BOOST);
      }
      if (highest_blocker_toughness < blocker_power[blocker_creature_index + 16])
      {
        highest_blocker_toughness = blocker_power[blocker_creature_index + 16];
      }
      blocker_index_by_card[card] = (char)blocker_creature_index;
      blocker_creature_index++;
    }
  }

  band_anchor_card = -1;
  for (card = 0; candidate_count > card; card++)
  {
    if ((band_anchor_card == -1) &&
        (saved_attacker_damage[card] < highest_blocker_toughness) &&
        (highest_blocker_toughness <= saved_attacker_damage[card] + band_candidate_power) &&
        ((saved_attacker_abilities[card] & band_common_abilities) == saved_attacker_abilities[card]))
    {
      saved_attacker_damage[card] += band_candidate_power;
      band_anchor_card = candidate_cards[card];
    }
  }

  own_creature_index = 0;
  for (inner = 0; inner < g_active_cards_count[player]; inner++)
  {
    if ((is_in_play(player, inner) == 0) ||
        ((global_cards_data[AI_CARD_INTERNAL_ID(player, inner)].type & TYPE_CREATURE) == 0))
    {
    }
    else
    {
      candidate_powers[own_creature_index] = AI_EVAL(player, inner).power;
      own_toughness[own_creature_index] = AI_EVAL(player, inner).toughness;
      candidate_scores[own_creature_index] =
          (candidate_powers[own_creature_index] + 1) * (own_toughness[own_creature_index] + 1);
      for (subset_mask = 0; (int)subset_mask < combat_damage_attacker_count; subset_mask++)
      {
        if (combat_damage_attacker_cards[subset_mask] == inner)
        {
          candidate_scores[own_creature_index] = g_ai_attacker_cards[subset_mask];
        }
      }
      own_index_by_card[inner] = (char)own_creature_index;
      own_creature_index++;
    }
  }

  restore_combat_simulation_state();
  best_score = 9999;
  get_landwalk_evasion_masks(&landwalk_mask, NULL);

  for (subset_mask = 0; (1 << (unsigned char)candidate_count) > (int)subset_mask; subset_mask++)
  {
    subset_legal = 1;
    combat_damage_attacker_count = 0;
    g_ai_destroy_if_blocked_attacker_mask = 0;
    for (card = 0; card < candidate_count; card++)
    {
      AI_CARD_STATE(player, candidate_cards[card]) &= ~STATE_ATTACKING;
      card_id = global_cards_data[AI_CARD_INTERNAL_ID(player, candidate_cards[card])].id;
      if ((subset_mask & (1 << (unsigned char)card)) != 0)
      {
        AI_CARD_STATE(player, candidate_cards[card]) |= STATE_ATTACKING;
        combat_damage_attacker_cards[combat_damage_attacker_count] = candidate_cards[card];
        combat_damage_attacker_damage[combat_damage_attacker_count] = saved_attacker_damage[card];
        combat_damage_attacker_toughness[combat_damage_attacker_count] = saved_attacker_toughness[card];
        combat_damage_attacker_abilities[combat_damage_attacker_count] = saved_attacker_abilities[card];
        g_ai_attacker_cards[combat_damage_attacker_count] = saved_attacker_scores[card];
        g_ai_attacker_power[combat_damage_attacker_count] = blocker_toughness[card];
        g_ai_attacker_toughness[combat_damage_attacker_count] = g_ai_attacker_damage[card];
        g_ai_blocker_power[combat_damage_attacker_count] = g_ai_attacker_scores[card];
        if ((card_id == 0x28) || (card_id == 0x98))
        {
          g_ai_destroy_if_blocked_attacker_mask |= 1 << (unsigned char)combat_damage_attacker_count;
        }
        combat_damage_attacker_count++;
      }
      else
      {
        if ((card_id == 0x19f) || (card_id == 0x84))
        {
          subset_legal = 0;
        }
        if ((forced_attack_mask & (1 << (unsigned char)card)) != 0)
        {
          subset_legal = 0;
        }
      }
    }

    if (subset_legal == 0)
    {
      continue;
    }

    if (untapped_royal_assassin_count != 0)
    {
      assassin_count = 0;
      for (card = 0; card < combat_damage_attacker_count; card++, assassin_count++)
      {
        if ((AI_CARD_STATE(player, combat_damage_attacker_cards[card]) & STATE_VIGILANCE) == 0)
        {
          g_ai_blocker_toughness[card] = card;
        }
        else
        {
          g_ai_blocker_toughness[card] = 99;
          assassin_count--;
        }
      }
      for (card = 0; card < combat_damage_attacker_count; card++)
      {
        for (inner = card; inner < combat_damage_attacker_count; inner++)
        {
          if ((g_ai_blocker_toughness[card] != 99) &&
              (combat_damage_attacker_damage[g_ai_blocker_toughness[card]] <
               combat_damage_attacker_damage[g_ai_blocker_toughness[inner]]))
          {
            pad_518 = g_ai_blocker_toughness[inner];
            g_ai_blocker_toughness[inner] = g_ai_blocker_toughness[card];
            g_ai_blocker_toughness[card] = pad_518;
          }
        }
      }
      for (card = 0; card < combat_damage_attacker_count; card++)
      {
        if (g_ai_blocker_toughness[card] == 99)
        {
          g_ai_blocker_damage[card] = 99;
        }
        else
        {
          g_ai_blocker_damage[g_ai_blocker_toughness[card]] = card;
        }
      }
    }

    g_ai_current_attacker = 1;
    choose_blockers_ai(player);
    save_combat_simulation_state();
    g_duel_ai_mode_state = 1;
    C_dispatch_event_raw(199);
    g_duel_ai_mode_state = saved_ai_mode;

    expected_damage = 0;
    for (ranked_index = 0; ranked_index < 8; ranked_index++)
    {
      ranked_blockers[ranked_index * 2 + 3] = -1;
    }

    for (card = 0; card < g_active_cards_count[ai_blocker_player]; card++)
    {
      internal_card_id = AI_CARD_INTERNAL_ID(ai_blocker_player, card);
      if ((is_in_play(ai_blocker_player, card) == 0) ||
          ((global_cards_data[internal_card_id].type & TYPE_CREATURE) == 0) ||
          ((AI_CARD_STATE(ai_blocker_player, card) & STATE_PLAYED_FROM_HAND) == 0))
      {
      }
      else
      {
        blocker_creature_index = (int)blocker_index_by_card[card];
        blocker_power_value = blocker_power[blocker_creature_index];
        if ((AI_EVAL(ai_blocker_player, card).abilities & 0x80000000) != 0)
        {
          blocker_power_value += 2;
        }
        for (inner = 0; inner < 8; inner++)
        {
          if (blocker_power_value > ranked_blockers[inner * 2 + 3])
          {
            for (ranked_index = 7; inner < ranked_index; ranked_index--)
            {
              ranked_blockers[ranked_index * 2 + 2] = ranked_blockers[ranked_index * 2];
              ranked_blockers[ranked_index * 2 + 3] = ranked_blockers[ranked_index * 2 + 1];
            }
            ranked_blockers[inner * 2 + 2] = card;
            ranked_blockers[inner * 2 + 3] = blocker_power_value;
            break;
          }
        }
      }
    }

    for (ranked_index = 0; ranked_index < 8; ranked_index++)
    {
      if (ranked_blockers[ranked_index * 2 + 3] == -1)
      {
        break;
      }
      card = ranked_blockers[ranked_index * 2 + 2];
      attacker_abilities = C_get_abilities(ai_blocker_player, card, EVENT_ABILITIES, -1);
      blocker_creature_index = (int)blocker_index_by_card[card];
      blocker_power_value = blocker_power[blocker_creature_index];
      if ((AI_EVAL(ai_blocker_player, card).abilities & 0x80000000) != 0)
      {
        blocker_power_value += 2;
      }
      blocker_toughness_value = blocker_power[blocker_creature_index + 16];
      best_blocker_card = 0;
      block_result_mask = best_blocker_card;
      own_creature_index = 0;
      smallest_blocker_score = 0x7fff;

      for (inner = 0; inner < g_active_cards_count[player]; inner++)
      {
        if ((is_in_play(player, inner) == 0) ||
            ((AI_CARD_STATE(player, inner) & STATE_PLAYED_FROM_HAND) == 0) ||
            ((global_cards_data[AI_CARD_INTERNAL_ID(player, inner)].type & TYPE_CREATURE) == 0))
        {
        }
        else
        {
          own_creature_index = (int)own_index_by_card[inner];
          test_power = candidate_powers[own_creature_index];
          test_toughness = own_toughness[own_creature_index];
          if (((AI_CARD_STATE(player, inner) &
                ((has_vigilance(player, inner) == 0 ? STATE_ATTACKING : 0) + STATE_BLOCKING)) != 0) ||
              (can_block_attacker_with_abilities(player, inner, ai_blocker_player, card,
                                                 attacker_abilities, landwalk_mask) == 0))
          {
          }
          else
          {
            block_result_mask |= 1;
            if ((blocker_power_value < test_toughness) || (blocker_toughness_value <= test_power))
            {
              block_result_mask |= 2;
              AI_CARD_STATE(player, inner) |= STATE_BLOCKING;
              break;
            }
            if (candidate_scores[own_creature_index] < smallest_blocker_score)
            {
              smallest_blocker_score = candidate_scores[own_creature_index];
              best_blocker_card = inner;
            }
          }
        }
      }

      if ((block_result_mask & 2) == 0)
      {
        pressure_score = (g_ai_combat_value_weights[player] * blocker_power_value * 0x18) / 4;
        test_toughness = g_life[player] - blocker_power_value;
        if (test_toughness < 2)
        {
          test_toughness = 1;
        }
        test_toughness = ClampIntToRange(test_toughness, 1, 99);
        pressure_score /= test_toughness;
        if (block_result_mask != 0)
        {
          cost_to_chump = (g_ai_combat_value_weights[player + 2] * smallest_blocker_score) / 8;
          if (cost_to_chump <= pressure_score)
          {
            g_ai_attacker_count += cost_to_chump;
            AI_CARD_STATE(player, best_blocker_card) |= STATE_BLOCKING;
            continue;
          }
        }
        expected_damage += blocker_power_value;
        g_ai_attacker_count += pressure_score;
      }
    }

    restore_combat_simulation_state();
    if ((g_life[player] <= expected_damage) && (0 < g_life[player]) && (0 < g_life[1 - player]))
    {
      g_ai_attacker_count += ((expected_damage - g_life[player]) + 2) * 0x80;
      g_ai_attacker_count += g_ai_current_blocker;
    }
    if (best_score > g_ai_attacker_count)
    {
      best_score = g_ai_attacker_count;
      selected_mask = subset_mask;
    }
  }

  if ((selected_mask != 0) && ((g_battlefield_extra_ability_flags & 0x2000000) != 0))
  {
    C_dispatch_event_raw(0x92);
  }

  for (card = 0; candidate_count > card; card++)
  {
    AI_CARD_STATE(player, candidate_cards[card]) &= ~STATE_ATTACKING;
    if ((selected_mask & (1 << (unsigned char)card)) != 0)
    {
      AI_CARD_STATE(player, candidate_cards[card]) |= STATE_ATTACKING;
      if ((g_battlefield_extra_ability_flags & 0x80000) != 0)
      {
        g_trigger_cause_controller = player;
        g_trigger_cause = candidate_cards[card];
        dispatch_trigger_twice_once_with_each_player_as_reason(g_current_player, TRIGGER_ATTACKER_CHOSEN,
                                                               gs_attacker_selected_008b32d0, 0);
      }
      if ((band_candidate_count != 0) &&
          (band_anchor_card == -1) &&
          ((saved_attacker_abilities[card] & band_common_abilities) == saved_attacker_abilities[card]))
      {
        band_anchor_card = candidate_cards[card];
      }
    }
  }

  if ((band_candidate_count != 0) && (band_anchor_card != -1) &&
      ((AI_CARD_STATE(player, band_anchor_card) & STATE_ATTACKING) != 0))
  {
    for (card = 0; band_candidate_count > card; card++)
    {
      AI_CARD_BLOCKING(player, band_candidate_cards[card]) = (unsigned char)band_anchor_card;
      AI_CARD_STATE(player, band_candidate_cards[card]) |= STATE_ATTACKING;
      if ((g_battlefield_extra_ability_flags & 0x80000) != 0)
      {
        g_trigger_cause_controller = player;
        g_trigger_cause = band_candidate_cards[card];
        dispatch_trigger_twice_once_with_each_player_as_reason(g_current_player, TRIGGER_ATTACKER_CHOSEN,
                                                               gs_attacker_selected_008b32d0, 0);
      }
    }
    AI_CARD_BLOCKING(player, band_anchor_card) = (unsigned char)band_anchor_card;
  }
  else if ((band_candidate_count != 0) && (highest_blocker_toughness == 0))
  {
    for (card = 0; band_candidate_count > card; card++)
    {
      AI_CARD_STATE(player, band_candidate_cards[card]) |= STATE_ATTACKING;
      if ((g_battlefield_extra_ability_flags & 0x80000) != 0)
      {
        g_trigger_cause_controller = player;
        g_trigger_cause = band_candidate_cards[card];
        dispatch_trigger_twice_once_with_each_player_as_reason(g_current_player, TRIGGER_ATTACKER_CHOSEN,
                                                               gs_attacker_selected_008b32d0, 0);
      }
    }
    selected_mask = 1;
  }

  g_attacking_creature_count = selected_mask;
  g_adventure_duel_internal_card_id = saved_event_state;
  return selected_mask;
}

// FUNCTION: MAGIC 0x0050e6f0
// FUNCTION: SHANDALAR 0x0041d290
int score_gamestate_after_simulated_card_death(int player, int card)
{
  int saved_ai_modifier;
  int score;

  save_combat_simulation_state();
  saved_ai_modifier = g_ai_modifier;
  g_ai_modifier = 0;
  AI_CARD(player, card).token_status |= 8;
  kill_card(player, card, KILL_DESTROY);
  C_dispatch_event_raw(199);
  process_damage_prevention(player);
  score = g_ai_modifier + ai_opinion_of_gamestate(player);
  restore_combat_simulation_state();
  g_ai_modifier = saved_ai_modifier;
  return score;
}

// FUNCTION: MAGIC 0x00516bf1
// FUNCTION: SHANDALAR 0x004257a4
void dispatch_ai_combat_pump_checks(int player)
{
  int card;

  for (card = 0; card < 4; card++)
  {
    g_global_stats_bonus[card] = 0;
  }

  for (card = 0; MAX(g_active_cards_count[g_active_player], g_active_cards_count[g_other_player]) > card; card++)
  {
    if (is_in_play(g_active_player, card) != 0)
    {
      (*global_cards_data[AI_CARD_INTERNAL_ID(g_active_player, card)].code_pointer)(g_active_player, card, EVENT_CHECK_PUMP);
    }

    if ((is_in_play(g_other_player, card) != 0) ||
        ((AI_CARD_INTERNAL_ID(g_other_player, card) != -1) &&
         ((global_cards_data[AI_CARD_INTERNAL_ID(g_other_player, card)].type & 0x10) != 0)))
    {
      (*global_cards_data[AI_CARD_INTERNAL_ID(g_other_player, card)].code_pointer)(g_other_player, card, EVENT_CHECK_PUMP);
    }
  }
}

// FUNCTION: MAGIC 0x005170d1
// FUNCTION: SHANDALAR 0x00425c7e
int score_ai_blocker_candidate(int player, int card)
{
  int adjusted_toughness;
  int adjusted_power;
  int power_plus_two;
  int toughness;
  int internal_card_id;
  int power;
  int toughness_plus_two;
  int score;

  internal_card_id = global_card_instances[player][card].internal_card_id;
  power = C_get_abilities(player, card, EVENT_POWER, -1);
  toughness = C_get_abilities(player, card, EVENT_TOUGHNESS, -1);
  if (power != 0)
  {
    adjusted_power = power + 5;
  }
  else
  {
    adjusted_power = power;
  }
  power_plus_two = adjusted_power + 2;
  if (toughness != 0)
  {
    adjusted_toughness = toughness + 5;
  }
  else
  {
    adjusted_toughness = toughness;
  }
  toughness_plus_two = adjusted_toughness + 2;
  score = ((global_cards_data[internal_card_id].ai_base_value + power_plus_two) * toughness_plus_two) * 5;
  if ((global_cards_data[internal_card_id].static_ability & 0x200) != 0)
  {
    score = (score * 3) / 2;
  }
  if ((global_cards_data[internal_card_id].extra_ability & (EA_ACT_ABILITY | EA_ACT_INTERRUPT)) != 0)
  {
    score /= 2;
  }
  if ((global_cards_data[internal_card_id].static_ability & 0x20) != 0)
  {
    score = (score * 3) / 2;
  }
  if ((global_cards_data[internal_card_id].extra_ability & EA_INF_POWER) != 0)
  {
    score *= 3;
  }
  if ((global_cards_data[internal_card_id].extra_ability & EA_INF_TOUGHNESS) != 0)
  {
    score *= 3;
  }
  return score;
}

// FUNCTION: MAGIC 0x00446281
// FUNCTION: SHANDALAR 0x00411b19
unsigned int get_destroy_if_blocked_combat_flags(int attacker_player, int attacker_card, int blocker_player, int blocker_card)
{
  unsigned int flags;

  flags = 0;
  if ((AI_CARD(attacker_player, attacker_card).destroys_if_blocked == 0) &&
      (AI_CARD(blocker_player, blocker_card).destroys_if_blocked == 0))
  {
    return 0;
  }

  if ((AI_CARD(attacker_player, attacker_card).destroys_if_blocked &
       AI_CARD(blocker_player, blocker_card).color & 0x3f) != 0)
  {
    flags |= 1;
    if ((global_cards_data[AI_CARD_INTERNAL_ID(blocker_player, blocker_card)].subtype == 0) &&
        ((AI_CARD(attacker_player, attacker_card).destroys_if_blocked & 0x80) == 0))
    {
      flags &= ~1;
    }
  }

  if ((AI_CARD(attacker_player, attacker_card).color &
       AI_CARD(blocker_player, blocker_card).destroys_if_blocked & 0x3f) != 0)
  {
    flags |= 2;
    if ((global_cards_data[AI_CARD_INTERNAL_ID(attacker_player, attacker_card)].subtype == 0) &&
        ((AI_CARD(blocker_player, blocker_card).destroys_if_blocked & 0x80) == 0))
    {
      flags &= ~2;
    }
  }
  return flags;
}

// FUNCTION: MAGIC 0x0050f53a
// FUNCTION: SHANDALAR 0x0041e0d9
void setup_combat_damage_simulation(int player)
{
  int unused_padding_1;
  int unused_padding_2;
  unsigned int block_interaction_flags;
  int blocker_scores[16];
  unsigned int regen_ability_bits;
  int baseline_score;
  int blocker_candidate_index;
  int card_color;
  int band_scan_index;
  int power;
  int attacker_index;
  int card;
  int saved_ai_mode;
  int internal_card_id;
  int saved_ai_modifier;
  int blocker_candidate_count;
  int band_anchor_index;
  int blocker_candidates[16];
  unsigned int player_0_landwalk_mask;
  int combat_score;
  int cached_score;
  unsigned int player_1_landwalk_mask;

  ai_blocker_player = 1 - player;
  if (g_ai_current_attacker == 0)
  {
    get_landwalk_evasion_masks(&player_1_landwalk_mask, &player_0_landwalk_mask);
    if (ai_blocker_player == 1)
    {
      g_ai_combat_candidate_mask = player_1_landwalk_mask;
    }
    else
    {
      g_ai_combat_candidate_mask = player_0_landwalk_mask;
    }

    g_ai_combat_flags = 0;
    g_ai_destroy_if_blocked_attacker_mask = g_ai_combat_flags;
    g_ai_combat_candidate_count = 0;
    combat_damage_attacker_count = g_ai_combat_candidate_count;
    saved_ai_mode = g_duel_ai_mode_state;
    g_duel_ai_mode_state = 1;
    g_duel_state_0093f4b0 = 1;
    save_combat_simulation_state();
    saved_ai_modifier = g_ai_modifier;
    g_ai_modifier = 0;
    C_dispatch_event_raw(199);
    process_damage_prevention(player);
    baseline_score = ai_opinion_of_gamestate(player);
    baseline_score += g_ai_modifier;
    for (card = 0; card < g_active_cards_count[player]; card++)
    {
      AI_CARD_STATE(player, card) &= ~STATE_ATTACKING;
    }
    dispatch_ai_combat_pump_checks(player);
    restore_combat_simulation_state();
    g_ai_modifier = saved_ai_modifier;

    for (card = 0; card < g_active_cards_count[player]; card++)
    {
      internal_card_id = AI_CARD_INTERNAL_ID(player, card);
      if (((internal_card_id != -1) &&
           ((AI_CARD_STATE(player, card) & STATE_ATTACKING) != 0)) &&
          ((AI_CARD_BLOCKING(player, card) == -1) || (AI_CARD_BLOCKING(player, card) == card)))
      {
        card_color = single_color_test_bit_to_color_t((int)global_cards_data[internal_card_id].color);
        power = AI_EVAL(player, card).power;
        combat_damage_attacker_cards[combat_damage_attacker_count] = card;
        combat_damage_attacker_damage[combat_damage_attacker_count] = g_global_power_bonus[player] + power;
        combat_damage_attacker_toughness[combat_damage_attacker_count] = AI_EVAL(player, card).toughness + g_global_toughness_bonus[player];
        combat_damage_attacker_abilities[combat_damage_attacker_count] = AI_EVAL(player, card).abilities;
        if (has_mana(player, card_color, 1) == 0)
        {
          combat_damage_attacker_abilities[combat_damage_attacker_count] &= 0xfffffdff;
        }
        g_ai_score = 0;
        dispatch_event(player, card, 0x8a);
        g_ai_attacker_toughness[combat_damage_attacker_count] = g_ai_score;
        if ((global_cards_data[internal_card_id].extra_ability & EA_INF_POWER) != 0)
        {
          combat_damage_attacker_damage[combat_damage_attacker_count] +=
              (*global_cards_data[internal_card_id].code_pointer)(player, card, EVENT_POW_BOOST);
        }
        if ((global_cards_data[internal_card_id].extra_ability & EA_INF_TOUGHNESS) != 0)
        {
          combat_damage_attacker_toughness[combat_damage_attacker_count] +=
              (*global_cards_data[internal_card_id].code_pointer)(player, card, EVENT_TOU_BOOST);
        }
        if (AI_EVAL(player, card).cached_score == 0)
        {
          AI_EVAL(player, card).cached_score = score_gamestate_after_simulated_card_death(player, card);
        }
        cached_score = AI_EVAL(player, card).cached_score;
        combat_score = ((abs((int)global_cards_data[internal_card_id].cc[1]) +
                         global_cards_data[internal_card_id].cc[0]) -
                        cached_score) +
                       baseline_score;
        if ((combat_damage_attacker_abilities[combat_damage_attacker_count] & 0x200) != 0)
        {
          if (has_mana(player, card_color, 1) == 0)
          {
            combat_score <<= 1;
          }
          else
          {
            combat_score /= 3;
          }
        }
        AI_CARD(player, card).attack_rating = combat_score;
        g_ai_attacker_cards[combat_damage_attacker_count] = combat_score;
        combat_damage_attacker_toughness[combat_damage_attacker_count] -= AI_CARD(player, card).damage_on_card;
        g_ai_attacker_power[combat_damage_attacker_count] = 0;
        if (AI_CARD(player, card).destroys_if_blocked != 0)
        {
          g_ai_destroy_if_blocked_attacker_mask |= 1 << (unsigned char)combat_damage_attacker_count;
        }
        combat_damage_attacker_count++;
        if ((g_duel_ai_mode_state == 1) && (combat_damage_attacker_count > 6))
        {
          break;
        }
      }
    }

    for (card = 0; card < g_active_cards_count[player]; card++)
    {
      internal_card_id = AI_CARD_INTERNAL_ID(player, card);
      if ((((internal_card_id != -1) &&
            ((AI_CARD_STATE(player, card) & STATE_ATTACKING) != 0)) &&
           (AI_CARD_BLOCKING(player, card) != -1)) &&
          (AI_CARD_BLOCKING(player, card) != card))
      {
        band_anchor_index = -1;
        for (attacker_index = 0; attacker_index < combat_damage_attacker_count; attacker_index++)
        {
          if (AI_CARD_BLOCKING(player, card) == combat_damage_attacker_cards[attacker_index])
          {
            band_anchor_index = attacker_index;
            break;
          }
        }

        if (band_anchor_index != -1)
        {
          card_color = single_color_test_bit_to_color_t((int)global_cards_data[internal_card_id].color);
          g_ai_blocker_scores[g_ai_combat_candidate_count] = card;
          power = AI_EVAL(player, card).power;
          combat_damage_attacker_damage[band_anchor_index] += power;
          combat_damage_attacker_toughness[band_anchor_index] += AI_EVAL(player, card).toughness;
          regen_ability_bits =
              (AI_EVAL(player, card).abilities & 0x200) | (combat_damage_attacker_abilities[band_anchor_index] & 0x200);
          combat_damage_attacker_abilities[band_anchor_index] &= AI_EVAL(player, card).abilities;
          combat_damage_attacker_abilities[band_anchor_index] |= regen_ability_bits;
          if ((global_cards_data[internal_card_id].extra_ability & EA_INF_POWER) != 0)
          {
            combat_damage_attacker_damage[band_anchor_index] +=
                (*global_cards_data[internal_card_id].code_pointer)(player, card, EVENT_POW_BOOST);
          }
          if ((global_cards_data[internal_card_id].extra_ability & EA_INF_TOUGHNESS) != 0)
          {
            combat_damage_attacker_toughness[band_anchor_index] +=
                (*global_cards_data[internal_card_id].code_pointer)(player, card, EVENT_TOU_BOOST);
          }
          if (AI_EVAL(player, card).cached_score == 0)
          {
            AI_EVAL(player, card).cached_score = score_gamestate_after_simulated_card_death(player, card);
          }
          cached_score = AI_EVAL(player, card).cached_score;
          combat_score = ((abs((int)global_cards_data[internal_card_id].cc[1]) +
                           global_cards_data[internal_card_id].cc[0]) -
                          cached_score) +
                         baseline_score;
          if ((combat_damage_attacker_abilities[band_anchor_index] & 0x200) != 0)
          {
            if (has_mana(player, card_color, 1) == 0)
            {
              combat_score <<= 1;
            }
            else
            {
              combat_score /= 3;
            }
          }
          AI_CARD(player, card).attack_rating = combat_score;
          if (combat_score < g_ai_attacker_cards[band_anchor_index])
          {
            g_ai_attacker_cards[band_anchor_index] = combat_score;
          }
          combat_damage_attacker_toughness[band_anchor_index] -= AI_CARD(player, card).damage_on_card;
          g_ai_attacker_power[band_anchor_index] = 0;
          if (AI_CARD(player, card).destroys_if_blocked != 0)
          {
            g_ai_destroy_if_blocked_attacker_mask |= 1 << (unsigned char)band_anchor_index;
          }
          g_ai_combat_candidate_count++;
        }
      }
    }

    blocker_candidate_count = 0;
    for (card = 0; card < g_active_cards_count[ai_blocker_player]; card++)
    {
      internal_card_id = AI_CARD_INTERNAL_ID(ai_blocker_player, card);
      if ((((internal_card_id != -1) &&
            (((global_cards_data[internal_card_id].type & TYPE_CREATURE) != 0) ||
             ((AI_CARD_STATE(ai_blocker_player, card) & STATE_NONCREATURE_CAN_BLOCK) != 0))) &&
           ((AI_CARD_STATE(ai_blocker_player, card) & (STATE_OUBLIETTED | STATE_IN_PLAY | STATE_TAPPED)) == STATE_IN_PLAY)) &&
          (AI_CARD_BLOCKING(ai_blocker_player, card) == -1))
      {
        blocker_candidates[blocker_candidate_count] = card;
        blocker_candidate_count++;
      }
      if ((g_duel_ai_mode_state == 1) && (blocker_candidate_count > 15))
      {
        break;
      }
    }

    if ((g_duel_ai_mode_state == 1) && (blocker_candidate_count > 6))
    {
      for (card = 0; card < blocker_candidate_count; card++)
      {
        blocker_scores[card] = score_ai_blocker_candidate(ai_blocker_player, blocker_candidates[card]);
      }
      for (card = 0; card < blocker_candidate_count; card++)
      {
        for (attacker_index = card; attacker_index < blocker_candidate_count; attacker_index++)
        {
          if (blocker_scores[card] < blocker_scores[attacker_index])
          {
            power = blocker_scores[card];
            blocker_scores[card] = blocker_scores[attacker_index];
            blocker_scores[attacker_index] = power;
            power = blocker_candidates[card];
            blocker_candidates[card] = blocker_candidates[attacker_index];
            blocker_candidates[attacker_index] = power;
          }
        }
      }
      if (blocker_candidate_count > 6)
      {
        blocker_candidate_count = 7;
      }
      for (card = 0; card < blocker_candidate_count; card++)
      {
        for (attacker_index = card; attacker_index < blocker_candidate_count; attacker_index++)
        {
          if (blocker_candidates[attacker_index] < blocker_candidates[card])
          {
            power = blocker_candidates[card];
            blocker_candidates[card] = blocker_candidates[attacker_index];
            blocker_candidates[attacker_index] = power;
          }
        }
      }
    }

    ai_blocker_count = 0;
    for (blocker_candidate_index = 0; blocker_candidate_index < blocker_candidate_count; blocker_candidate_index++)
    {
      card = blocker_candidates[blocker_candidate_index];
      internal_card_id = AI_CARD_INTERNAL_ID(ai_blocker_player, card);
      card_color = single_color_test_bit_to_color_t((int)global_cards_data[internal_card_id].color);
      AI_CARD_STATE(ai_blocker_player, card) |= STATE_BLOCKING;
      power = AI_EVAL(ai_blocker_player, card).power;
      ai_blocker_cards[ai_blocker_count] = card;
      combat_damage_blocker_damage[ai_blocker_count] = g_global_power_bonus[ai_blocker_player] + power;
      combat_damage_blocker_toughness[ai_blocker_count] = AI_EVAL(ai_blocker_player, card).toughness + g_global_toughness_bonus[ai_blocker_player];
      combat_damage_blocker_abilities[ai_blocker_count] = AI_EVAL(ai_blocker_player, card).abilities;
      if (has_mana(ai_blocker_player, card_color, 1) == 0)
      {
        combat_damage_blocker_abilities[ai_blocker_count] &= 0xfffffdff;
      }
      g_ai_score = 0;
      dispatch_event(ai_blocker_player, card, 0x8b);
      g_ai_blocker_power[combat_damage_attacker_count] = g_ai_score;
      if (ai_blocker_player == g_active_player)
      {
        if ((global_cards_data[internal_card_id].extra_ability & EA_INF_POWER) != 0)
        {
          combat_damage_blocker_damage[ai_blocker_count] +=
              (*global_cards_data[internal_card_id].code_pointer)(ai_blocker_player, card, EVENT_POW_BOOST);
        }
        if ((global_cards_data[internal_card_id].extra_ability & EA_INF_TOUGHNESS) != 0)
        {
          combat_damage_blocker_toughness[ai_blocker_count] +=
              (*global_cards_data[internal_card_id].code_pointer)(ai_blocker_player, card, EVENT_TOU_BOOST);
        }
      }
      if (AI_EVAL(ai_blocker_player, card).cached_score == 0)
      {
        save_combat_simulation_state();
        saved_ai_modifier = g_ai_modifier;
        g_ai_modifier = 0;
        AI_CARD(ai_blocker_player, card).token_status |= 8;
        kill_card(ai_blocker_player, card, KILL_DESTROY);
        C_dispatch_event_raw(199);
        process_damage_prevention(player);
        cached_score = ai_opinion_of_gamestate(player);
        cached_score += g_ai_modifier;
        restore_combat_simulation_state();
        AI_EVAL(ai_blocker_player, card).cached_score = cached_score;
      }
      cached_score = AI_EVAL(ai_blocker_player, card).cached_score;
      combat_score = cached_score - baseline_score;
      if ((combat_damage_blocker_abilities[ai_blocker_count] & 0x200) != 0)
      {
        if (has_mana(ai_blocker_player, card_color, 1) == 0)
        {
          combat_score <<= 1;
        }
        else
        {
          combat_score /= 5;
        }
      }
      g_ai_combat_candidate_cards[ai_blocker_count] = combat_score;
      AI_CARD(ai_blocker_player, card).attack_rating = combat_score;
      combat_damage_blocker_toughness[ai_blocker_count] -= AI_CARD(ai_blocker_player, card).damage_on_card;
      AI_CARD_STATE(ai_blocker_player, card) &= ~STATE_BLOCKING;
      for (attacker_index = 0; attacker_index < combat_damage_attacker_count; attacker_index++)
      {
        if (can_block_attacker_with_abilities(ai_blocker_player, card, player, combat_damage_attacker_cards[attacker_index],
                                              combat_damage_attacker_abilities[attacker_index], g_ai_combat_candidate_mask) == 0)
        {
          if (g_ai_combat_candidate_count != 0)
          {
            for (band_scan_index = 0; band_scan_index < g_ai_combat_candidate_count; band_scan_index++)
            {
              if ((AI_CARD_BLOCKING(player, g_ai_blocker_scores[band_scan_index]) == combat_damage_attacker_cards[attacker_index]) &&
                  (can_block_attacker_with_abilities(ai_blocker_player, card, player, g_ai_blocker_scores[band_scan_index],
                                                     combat_damage_attacker_abilities[attacker_index], g_ai_combat_candidate_mask) != 0))
              {
                g_ai_attacker_power[attacker_index] |= 1 << (unsigned char)ai_blocker_count;
              }
            }
          }
        }
        else
        {
          g_ai_attacker_power[attacker_index] |= 1 << (unsigned char)ai_blocker_count;
        }
      }
      if (AI_CARD(ai_blocker_player, card).destroys_if_blocked != 0)
      {
        g_ai_combat_flags |= 1 << (unsigned char)ai_blocker_count;
      }
      AI_CARD_STATE(ai_blocker_player, card) &= ~STATE_BLOCKING;
      g_ai_combat_candidate_scores[ai_blocker_count] =
          (g_ai_combat_value_weights[ai_blocker_player + 2] * g_ai_combat_candidate_cards[ai_blocker_count]) /
          ClampIntToRange(combat_damage_blocker_toughness[ai_blocker_count] + 1, 1, 99);
      ai_blocker_count++;
      if ((g_duel_ai_mode_state == 1) && (7 <= ai_blocker_count))
      {
        break;
      }
    }

    memset(g_ai_attacker_ability_counts, 0, sizeof(g_ai_attacker_ability_counts));
    memset(g_ai_blocker_ability_counts, 0, sizeof(g_ai_blocker_ability_counts));
    for (card = 0; card < combat_damage_attacker_count; card++)
    {
      for (attacker_index = 0; attacker_index < ai_blocker_count; attacker_index++)
      {
        block_interaction_flags = get_destroy_if_blocked_combat_flags(player, combat_damage_attacker_cards[card], ai_blocker_player,
                                                                      ai_blocker_cards[attacker_index]);
        if ((block_interaction_flags & 1) != 0)
        {
          g_ai_attacker_ability_counts[card] |= 1 << (unsigned char)attacker_index;
        }
        if ((block_interaction_flags & 2) != 0)
        {
          g_ai_blocker_ability_counts[attacker_index] |= 1 << (unsigned char)card;
        }
      }
    }
    g_duel_ai_mode_state = saved_ai_mode;
  }
  g_ai_current_attacker = 0;
  g_duel_state_0093f4b0 = 0;
  g_ai_combat_best_score = g_ai_combat_search_width_by_depth[combat_damage_attacker_count];
}

// FUNCTION: MAGIC 0x004e4913
// FUNCTION: SHANDALAR 0x004c3b83
void save_combat_simulation_state(void)
{
  memcpy(combat_sim_saved_card_instances, global_card_instances, sizeof(combat_sim_saved_card_instances));
  memcpy(combat_sim_saved_dynamic_card_data, global_cards_data + g_card_count, sizeof(combat_sim_saved_dynamic_card_data));
  memcpy(combat_sim_saved_library, global_library, sizeof(combat_sim_saved_library));
  memcpy(combat_sim_saved_graveyard_slots, global_graveyard_slots, sizeof(combat_sim_saved_graveyard_slots));
  memcpy(combat_sim_saved_exile, global_exile, sizeof(combat_sim_saved_exile));
  memcpy(combat_sim_saved_special_mana_pool, g_special_mana_pool, sizeof(combat_sim_saved_special_mana_pool));
  memcpy(combat_sim_saved_raw_mana_available, g_raw_mana_available, sizeof(combat_sim_saved_raw_mana_available));
  memcpy(combat_sim_saved_basiclandtypes_controlled, g_basiclandtypes_controlled, sizeof(combat_sim_saved_basiclandtypes_controlled));
  memcpy(combat_sim_saved_raw_mana_available_hex, g_raw_mana_available_hex, sizeof(combat_sim_saved_raw_mana_available_hex));
  memcpy(combat_sim_saved_life, g_life, sizeof(combat_sim_saved_life));
  memcpy(combat_sim_saved_poison_counters, g_poison_counters, sizeof(combat_sim_saved_poison_counters));
  memcpy(combat_sim_saved_card_types_in_play, g_card_types_in_play, sizeof(combat_sim_saved_card_types_in_play));
  memcpy(combat_sim_saved_graveyard_card_types, g_graveyard_card_types, sizeof(combat_sim_saved_graveyard_card_types));
  memcpy(combat_sim_saved_duel_summary, &g_duel_summary, sizeof(combat_sim_saved_duel_summary));
  memcpy(combat_sim_saved_ante_cards, global_ante_cards, sizeof(combat_sim_saved_ante_cards));
  memcpy(combat_sim_saved_timestamp_player, TENTATIVE_timestamp_player, sizeof(combat_sim_saved_timestamp_player));
  memcpy(combat_sim_saved_timestamp_card, TENTATIVE_timestamp_card, sizeof(combat_sim_saved_timestamp_card));
  combat_sim_saved_land_can_be_played = g_land_can_be_played;
  combat_sim_saved_current_phase = g_current_phase;
  combat_sim_saved_current_casting_internal_card_id = g_current_casting_internal_card_id;
  combat_sim_saved_attacking_creature_count = g_attacking_creature_count;
  combat_sim_saved_spell_fizzled = g_spell_fizzled;
  memcpy(combat_sim_saved_stack_data, g_stack_data, sizeof(combat_sim_saved_stack_data));
  memcpy(combat_sim_saved_stack_cards, global_stack_cards, sizeof(combat_sim_saved_stack_cards));
  memcpy(combat_sim_saved_stack_damage_targets, global_stack_damage_targets, sizeof(combat_sim_saved_stack_damage_targets));
  memcpy(combat_sim_saved_active_cards_count, g_active_cards_count, sizeof(combat_sim_saved_active_cards_count));
  combat_sim_saved_stack_size = g_stack_size;
  combat_sim_saved_pending_killed_card_handler = pending_killed_card_handler;
  combat_sim_saved_ai_modifier = g_ai_modifier;
  memcpy(combat_sim_saved_mana_charge, g_mana_charge, sizeof(combat_sim_saved_mana_charge));
  combat_sim_saved_x_value = g_x_value;
  combat_sim_saved_max_x_value = g_max_x_value;
  combat_sim_saved_extra_turn_player = g_duel_extra_turn_player;
  combat_sim_saved_DAT_0093a848 = g_duel_state_0093a848;
  combat_sim_saved_stop_phase = g_stop_phase;
  combat_sim_saved_stop_phase_player = g_stop_phase_player;
  combat_sim_saved_battlefield_extra_ability_flags = g_battlefield_extra_ability_flags;
  memcpy(combat_sim_saved_unk_008b44d0, g_lich_active, sizeof(combat_sim_saved_unk_008b44d0));
  memcpy(combat_sim_saved_unk_007a79b0, g_player_special_effect_flags, sizeof(combat_sim_saved_unk_007a79b0));
}

// FUNCTION: MAGIC 0x004e4bc4
// FUNCTION: SHANDALAR 0x004c3e34
void restore_combat_simulation_state(void)
{
  memcpy(global_card_instances, combat_sim_saved_card_instances, sizeof(combat_sim_saved_card_instances));
  memcpy(global_cards_data + g_card_count, combat_sim_saved_dynamic_card_data, sizeof(combat_sim_saved_dynamic_card_data));
  memcpy(global_library, combat_sim_saved_library, sizeof(combat_sim_saved_library));
  memcpy(global_graveyard_slots, combat_sim_saved_graveyard_slots, sizeof(combat_sim_saved_graveyard_slots));
  memcpy(global_exile, combat_sim_saved_exile, sizeof(combat_sim_saved_exile));
  memcpy(g_special_mana_pool, combat_sim_saved_special_mana_pool, sizeof(combat_sim_saved_special_mana_pool));
  memcpy(g_raw_mana_available, combat_sim_saved_raw_mana_available, sizeof(combat_sim_saved_raw_mana_available));
  memcpy(g_basiclandtypes_controlled, combat_sim_saved_basiclandtypes_controlled, sizeof(combat_sim_saved_basiclandtypes_controlled));
  memcpy(g_raw_mana_available_hex, combat_sim_saved_raw_mana_available_hex, sizeof(combat_sim_saved_raw_mana_available_hex));
  memcpy(g_life, combat_sim_saved_life, sizeof(combat_sim_saved_life));
  memcpy(g_poison_counters, combat_sim_saved_poison_counters, sizeof(combat_sim_saved_poison_counters));
  memcpy(g_card_types_in_play, combat_sim_saved_card_types_in_play, sizeof(combat_sim_saved_card_types_in_play));
  memcpy(g_graveyard_card_types, combat_sim_saved_graveyard_card_types, sizeof(combat_sim_saved_graveyard_card_types));
  memcpy(&g_duel_summary, combat_sim_saved_duel_summary, sizeof(combat_sim_saved_duel_summary));
  memcpy(global_ante_cards, combat_sim_saved_ante_cards, sizeof(combat_sim_saved_ante_cards));
  memcpy(TENTATIVE_timestamp_player, combat_sim_saved_timestamp_player, sizeof(combat_sim_saved_timestamp_player));
  memcpy(TENTATIVE_timestamp_card, combat_sim_saved_timestamp_card, sizeof(combat_sim_saved_timestamp_card));
  g_land_can_be_played = combat_sim_saved_land_can_be_played;
  g_current_phase = combat_sim_saved_current_phase;
  g_current_casting_internal_card_id = combat_sim_saved_current_casting_internal_card_id;
  g_attacking_creature_count = combat_sim_saved_attacking_creature_count;
  g_spell_fizzled = combat_sim_saved_spell_fizzled;
  memcpy(g_stack_data, combat_sim_saved_stack_data, sizeof(combat_sim_saved_stack_data));
  memcpy(global_stack_cards, combat_sim_saved_stack_cards, sizeof(combat_sim_saved_stack_cards));
  memcpy(global_stack_damage_targets, combat_sim_saved_stack_damage_targets, sizeof(combat_sim_saved_stack_damage_targets));
  memcpy(g_active_cards_count, combat_sim_saved_active_cards_count, sizeof(combat_sim_saved_active_cards_count));
  g_stack_size = combat_sim_saved_stack_size;
  pending_killed_card_handler = combat_sim_saved_pending_killed_card_handler;
  g_ai_modifier = combat_sim_saved_ai_modifier;
  memcpy(g_mana_charge, combat_sim_saved_mana_charge, sizeof(combat_sim_saved_mana_charge));
  g_x_value = combat_sim_saved_x_value;
  g_max_x_value = combat_sim_saved_max_x_value;
  g_duel_extra_turn_player = combat_sim_saved_extra_turn_player;
  g_duel_state_0093a848 = combat_sim_saved_DAT_0093a848;
  g_stop_phase = combat_sim_saved_stop_phase;
  g_stop_phase_player = combat_sim_saved_stop_phase_player;
  g_battlefield_extra_ability_flags = combat_sim_saved_battlefield_extra_ability_flags;
  memcpy(g_lich_active, combat_sim_saved_unk_008b44d0, sizeof(combat_sim_saved_unk_008b44d0));
  memcpy(g_player_special_effect_flags, combat_sim_saved_unk_007a79b0, sizeof(combat_sim_saved_unk_007a79b0));
}

// FUNCTION: MAGIC 0x005129d1
// FUNCTION: SHANDALAR 0x00421578
void choose_blockers_ai(int player)
{
  int saved_event_state;
  int blocker_index;

  saved_event_state = g_adventure_duel_internal_card_id;
  g_adventure_duel_internal_card_id = 2;
  setup_combat_damage_simulation(player);
  g_ai_attacker_count = 0xffffd8f1;
  for (blocker_index = 0; blocker_index < 0x10; blocker_index++)
  {
    ai_block_damage_orders[blocker_index] = 0;
  }
  enumerate_ai_block_assignments(player, 0);
  if (g_duel_ai_mode_state == 1 || g_active_player == g_current_player)
  {
    for (blocker_index = 0; blocker_index < ai_blocker_count; blocker_index++)
    {
      if ((g_battlefield_extra_ability_flags & 0x800000) != 0)
      {
        push_affected_card_stack();
        g_trigger_cause_controller = ai_blocker_player;
        g_trigger_cause = ai_blocker_cards[blocker_index];
        dispatch_trigger(1 - g_current_player, TRIGGER_PAY_TO_BLOCK, gs_pay_for_blocker_009263f0, 1);
        pop_affected_card_stack();
      }
      if ((ai_block_assignments[blocker_index] != -1) && ((g_battlefield_extra_ability_flags & 0x100000) != 0))
      {
        g_trigger_cause_controller = ai_blocker_player;
        g_trigger_cause = ai_blocker_cards[blocker_index];
        dispatch_trigger_twice_once_with_each_player_as_reason(g_current_player, TRIGGER_BLOCKER_CHOSEN, gs_blocker_selected_00926210, 0);
      }
      global_card_instances[ai_blocker_player][ai_blocker_cards[blocker_index]].blocking = (unsigned char)ai_block_assignments[blocker_index];
      if (g_duel_ai_mode_state != 1)
      {
        request_duel_display_refresh_if_human(ai_blocker_player, ai_blocker_cards[blocker_index], 5, 2);
        if (ai_block_assignments[blocker_index] != -1)
        {
          play_sound_effect(0x15);
        }
      }
    }
  }
  g_adventure_duel_internal_card_id = saved_event_state;
}

// FUNCTION: MAGIC 0x00512bb2
// FUNCTION: SHANDALAR 0x00421758
void enumerate_ai_block_assignments(int player, int blocker_index)
{
  card_instance_t *blocker;
  int attacker_index;
  int score;

  if (ai_blocker_count == blocker_index)
  {
    score = score_current_ai_block_assignment();
    if (g_ai_attacker_count < score)
    {
      g_ai_attacker_count = score;
      memcpy(ai_block_assignments, ai_candidate_block_assignments, 0x1c);
      g_ai_current_blocker = g_ai_block_assignment_count;
    }
  }
  else
  {
    blocker = &global_card_instances[ai_blocker_player][ai_blocker_cards[blocker_index]];
    if (((blocker->state & STATE_UNKNOWN8000) == 0) || (blocker->blocking == -1))
    {
      ai_candidate_block_assignments[blocker_index] = -1;
      enumerate_ai_block_assignments(player, blocker_index + 1);
    }
    for (attacker_index = 0; attacker_index < combat_damage_attacker_count; attacker_index++)
    {
      if (ai_attacker_blocker_counts[attacker_index] >= g_ai_combat_best_score)
      {
        continue;
      }
      if ((g_ai_attacker_power[attacker_index] & (1 << (unsigned char)blocker_index)) == 0)
      {
        continue;
      }
      if ((blocker->state & STATE_UNKNOWN8000) != 0)
      {
        if (combat_damage_attacker_cards[attacker_index] != blocker->blocking)
        {
          continue;
        }
      }
      {
        ai_attacker_blocker_counts[attacker_index]++;
        ai_candidate_block_assignments[blocker_index] = combat_damage_attacker_cards[attacker_index];
        enumerate_ai_block_assignments(player, blocker_index + 1);
        ai_attacker_blocker_counts[attacker_index]--;
      }
    }
  }
}

// FUNCTION: MAGIC 0x00512d46
// FUNCTION: SHANDALAR 0x004218ec
int score_current_ai_block_assignment(void)
{
  int best_prevented_damage;
  int best_prevention_index;
  int blocker_damage;
  int attacker_damage_remaining;
  int blocker_index;
  int blocked_attacker_index;
  int blockers_on_attacker;
  int lethal_blocker_count;
  int best_damage_assignment_score;
  unsigned int damage_assignment_mask;
  int total_blocker_toughness;
  int best_blocker_score;
  int attacker_damage;
  int blocker_scan_index;
  int has_banding;
  int blocker_score_candidate;
  int attacker_index;
  int current_damage_assignment_score;
  int cop_mana;
  int saved_life;
  int prevention_count;
  int remaining_life;
  int prevented_damage[16];
  unsigned int destroyed_by_attacker_mask;
  int vulnerable_blockers[16];
  int assignment_score;
  int best_blocker;

  cop_mana = 0;
  g_ai_score = 0;
  assignment_score = g_ai_score;
  remaining_life = g_life[ai_blocker_player];
  prevention_count = 0;
  g_ai_block_assignment_count = 0;
  if (defending_cop_color_mask != 0)
  {
    cop_mana = defending_cop_available_mana;
  }

  for (attacker_index = 0; attacker_index < combat_damage_attacker_count; attacker_index++)
  {
    attacker_damage = combat_damage_attacker_damage[attacker_index];
    blocker_damage = 0;
    total_blocker_toughness = 0;
    blocked_attacker_index = -1;
    best_blocker_score = -1;
    lethal_blocker_count = 0;
    has_banding = 0;
    destroyed_by_attacker_mask = 0;
    blockers_on_attacker = 0;
    g_ai_score += g_ai_attacker_toughness[attacker_index];

    for (blocker_index = 0; blocker_index < ai_blocker_count; blocker_index++)
    {
      if (combat_damage_attacker_cards[attacker_index] != ai_candidate_block_assignments[blocker_index])
      {
        continue;
      }
      blockers_on_attacker++;
      g_ai_score += g_ai_blocker_power[blocker_index];
      blocker_damage += combat_damage_blocker_damage[blocker_index];
      total_blocker_toughness += combat_damage_blocker_toughness[blocker_index];
      blocked_attacker_index = blocker_index;
      if ((combat_damage_blocker_abilities[blocker_index] & 0x40) != 0)
      {
        has_banding = 1;
      }
      if ((g_ai_blocker_ability_counts[blocker_index] & (1 << (unsigned char)attacker_index)) != 0)
      {
        blocker_damage += 99;
      }
      if ((g_ai_attacker_ability_counts[attacker_index] & (1 << (unsigned char)blocker_index)) != 0)
      {
        destroyed_by_attacker_mask |= 1 << (unsigned char)blocker_index;
      }
      if (((combat_damage_blocker_toughness[blocker_index] <= attacker_damage) ||
           (destroyed_by_attacker_mask != 0)) &&
          ((combat_damage_blocker_abilities[blocker_index] & 0x200) == 0))
      {
        vulnerable_blockers[lethal_blocker_count] = blocker_index;
        lethal_blocker_count++;
      }
      blocker_score_candidate = g_ai_combat_candidate_scores[blocker_index];
      if (best_blocker_score < blocker_score_candidate)
      {
        best_blocker_score = blocker_score_candidate;
        best_blocker = blocker_index;
      }
    }

    saved_life = remaining_life;
    if (blocked_attacker_index == -1)
    {
      if ((untapped_royal_assassin_count == 0) || (untapped_royal_assassin_count <= g_ai_blocker_damage[attacker_index]))
      {
        if ((combat_damage_attacker_abilities[attacker_index] & 0x80000000) != 0)
        {
          remaining_life -= attacker_damage + 2;
        }
        else
        {
          remaining_life -= attacker_damage;
        }
      }
    }
    else if (((combat_damage_attacker_abilities[attacker_index] & 0x80) != 0) &&
             (total_blocker_toughness < attacker_damage))
    {
      remaining_life -= attacker_damage - total_blocker_toughness;
    }

    if (((defending_cop_color_mask != 0) && (remaining_life < saved_life)) &&
        ((defending_cop_color_mask &
          ((int)(char)global_card_instances[1 - ai_blocker_player][combat_damage_attacker_cards[attacker_index]].color)) != 0))
    {
      prevented_damage[prevention_count] = saved_life - remaining_life;
      prevention_count++;
    }

    if ((blocked_attacker_index != -1) &&
        (((combat_damage_attacker_toughness[attacker_index] <= blocker_damage) &&
          (((combat_damage_attacker_abilities[attacker_index] & 0x300) == 0) ||
           (lethal_blocker_count == 0))) ||
         ((untapped_royal_assassin_count != 0) &&
          (g_ai_blocker_damage[attacker_index] < untapped_royal_assassin_count))))
    {
      assignment_score += (g_ai_combat_value_weights[3 - ai_blocker_player] * g_ai_attacker_cards[attacker_index]) / 8;
    }

    best_damage_assignment_score = -1;
    for (damage_assignment_mask = 0;
         (int)damage_assignment_mask < (1 << (unsigned char)lethal_blocker_count);
         damage_assignment_mask++)
    {
      attacker_damage_remaining = attacker_damage;
      if (destroyed_by_attacker_mask != 0)
      {
        attacker_damage_remaining = 99;
      }
      else if (has_banding)
      {
        if (blocker_damage - blockers_on_attacker >= attacker_damage)
        {
          current_damage_assignment_score = 0;
          goto finish_damage_assignment_score;
        }
        if (damage_assignment_mask != 0)
        {
          current_damage_assignment_score = 9999;
        }
        else
        {
          current_damage_assignment_score = 0;
        }
        for (blocker_scan_index = 0; lethal_blocker_count > blocker_scan_index; blocker_scan_index++)
        {
          if ((damage_assignment_mask & (1 << (unsigned char)blocker_scan_index)) != 0)
          {
            blocker_index = vulnerable_blockers[blocker_scan_index];
            if (g_ai_combat_candidate_cards[blocker_index] < current_damage_assignment_score)
            {
              current_damage_assignment_score = g_ai_combat_candidate_cards[blocker_index];
            }
            if (attacker_damage < combat_damage_blocker_toughness[blocker_index])
            {
              current_damage_assignment_score = 0;
            }
            else if (((combat_damage_blocker_abilities[blocker_index] & 0x1ff800) != 0) &&
                     ((((int)(char)global_card_instances[1 - ai_blocker_player][combat_damage_attacker_cards[attacker_index]].color << 10) &
                       combat_damage_blocker_abilities[blocker_index] & 0x1ff800) != 0))
            {
              current_damage_assignment_score = 0;
            }
          }
        }
        goto finish_damage_assignment_score;
      }

      current_damage_assignment_score = 0;
      for (blocker_scan_index = 0; lethal_blocker_count > blocker_scan_index; blocker_scan_index++)
      {
        if ((damage_assignment_mask & (1 << (unsigned char)blocker_scan_index)) != 0)
        {
          blocker_index = vulnerable_blockers[blocker_scan_index];
          if ((((combat_damage_blocker_abilities[blocker_index] & 0x100) == 0) ||
               (combat_damage_blocker_damage[blocker_index] < combat_damage_attacker_toughness[attacker_index])) &&
              ((((int)(char)global_card_instances[1 - ai_blocker_player][combat_damage_attacker_cards[attacker_index]].color << 10) &
                combat_damage_blocker_abilities[blocker_index] & 0x1ff800) == 0))
          {
            current_damage_assignment_score += g_ai_combat_candidate_cards[blocker_index];
          }
          attacker_damage_remaining -= combat_damage_blocker_toughness[blocker_index];
          if (attacker_damage_remaining < 0)
          {
            break;
          }
        }
      }

    finish_damage_assignment_score:
      if ((attacker_damage_remaining >= 0) && (best_damage_assignment_score < current_damage_assignment_score))
      {
        best_damage_assignment_score = current_damage_assignment_score;
        g_ai_combat_search_flags = damage_assignment_mask;
      }
    }

    if (lethal_blocker_count != 0)
    {
      ai_block_damage_orders[attacker_index] = g_ai_combat_search_flags;
    }
    else
    {
      ai_block_damage_orders[attacker_index] = -best_blocker;
    }
    assignment_score -= best_damage_assignment_score;
  }

  assignment_score += g_ai_score;
  if (prevention_count != 0)
  {
    while (cop_mana != 0)
    {
      best_prevented_damage = 0;
      for (attacker_index = 0; prevention_count > attacker_index; attacker_index++)
      {
        if (best_prevented_damage < prevented_damage[attacker_index])
        {
          best_prevented_damage = prevented_damage[attacker_index];
          best_prevention_index = attacker_index;
        }
      }
      if (best_prevented_damage != 0)
      {
        remaining_life += prevented_damage[best_prevention_index];
        prevented_damage[best_prevention_index] = 0;
        cop_mana--;
      }
      else
      {
        cop_mana = 0;
      }
    }
  }
  if (remaining_life <= 0)
  {
    assignment_score -= 999 - (remaining_life * 0x60);
  }
  else
  {
    assignment_score -= (((g_life[ai_blocker_player] - remaining_life) * g_ai_combat_value_weights[ai_blocker_player] * 0x18) / 4) / remaining_life;
    if (g_ai_combat_search_depth != 0)
    {
      g_ai_block_assignment_count = ((g_life[ai_blocker_player] - remaining_life) * 700) / g_life[ai_blocker_player];
      assignment_score -= g_ai_block_assignment_count;
    }
  }
  return assignment_score;
}

// FUNCTION: MAGIC 0x00515f34
// FUNCTION: SHANDALAR 0x00424ae4
int cleanup_combat_state(int player)
{
  int card;

  for (card = 0; card < ai_blocker_count; card++)
  {
    global_card_instances[1 - player][ai_blocker_cards[card]].state &= ~STATE_BLOCKING;
  }
  for (card = 0; card < g_active_cards_count[player]; card++)
  {
    if ((global_card_instances[player][card].state & STATE_ATTACKING) != 0)
    {
      global_card_instances[player][card].state &= ~STATE_ATTACKING;
      global_card_instances[player][card].state |= STATE_ATTACKED;
    }
  }
  return 1;
}
