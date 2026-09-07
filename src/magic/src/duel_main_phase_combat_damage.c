#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "defs.h"
#include "magic/src/cards.h"
#include "magic/src/duel_engine.h"
#include "magic/src/game_support.h"
#include "magic/src/global_duel_ui_ids.h"
#include "magic/src/global_other.h"
#include "magic/src/global_state.h"
#include "magic/src/global_strings.h"

extern int ai_blocker_count;
extern int ai_blocker_cards[16];

void update_phase_display(int player, phase_t phase);
int prompt_stop_phase_anyway(phase_t phase);
char *get_displayed_card_name(int player, int card);
int process_killed_card(int player, int card);
int is_combat_damage_ability_in_step(int damage_step, int abilities);
int adjust_two_headed_giant_damage(int player, int card, int amount);
void clear_combat_damage_assignment_matrix(void);
void assign_attacker_combat_damage(int player, int index, int has_trample, int *damage_ids, int damage_step, int mode, int *best_low, int *best_high);
void assign_blocker_combat_damage(int player, int index, int multiple_attackers, int *damage_ids, int damage_step, int mode, int *best_low, int *best_high);
void highlight_combat_damage_attacker(int player, int card, int highlight);
void highlight_combat_damage_blocker(int player, int card, int highlight);
int has_damage_card_targeting(int player, int card);

#define COMBAT_DAMAGE_FLAG_COMBAT 0x40000
#define COMBAT_DAMAGE_FLAG_TRAMPLE 0x80000
#define COMBAT_DAMAGE_FLAG_FIRST_STRIKE 0x100000
#define COMBAT_DAMAGE_FLAG_CANCELABLE 0x8000000

// GLOBAL: MAGIC 0x00707c18
// GLOBAL: SHANDALAR 0x005a7610
int combat_damage_attacker_abilities[16];

// GLOBAL: MAGIC 0x00707c58
// GLOBAL: SHANDALAR 0x005a7650
int combat_damage_blocker_abilities[16];

// GLOBAL: MAGIC 0x00707cd8
// GLOBAL: SHANDALAR 0x005a76d0
int combat_damage_attacker_cards[16];

// GLOBAL: MAGIC 0x00707e28
// GLOBAL: SHANDALAR 0x005a7820
int combat_damage_attacker_toughness[16];

// GLOBAL: MAGIC 0x00707e68
// GLOBAL: SHANDALAR 0x005a7860
int combat_damage_blocker_toughness[16];

// GLOBAL: MAGIC 0x00707f78
// GLOBAL: SHANDALAR 0x005a7970
int combat_damage_best_assignment_codes[16];

// GLOBAL: MAGIC 0x00708040
// GLOBAL: SHANDALAR 0x005a7a38
int combat_damage_current_assignment_codes[16];

// GLOBAL: MAGIC 0x00708124
// GLOBAL: SHANDALAR 0x005a7b1c
int combat_damage_attacker_count;

// GLOBAL: MAGIC 0x00708210
// GLOBAL: SHANDALAR 0x005a7c08
int combat_damage_assignment_matrix[16][16];

// GLOBAL: MAGIC 0x00708618
// GLOBAL: SHANDALAR 0x005a8010
int combat_damage_attacker_damage[16];

// GLOBAL: MAGIC 0x007086a0
// GLOBAL: SHANDALAR 0x005a8098
int combat_damage_blocker_damage[16];

// FUNCTION: MAGIC 0x00515bba
// FUNCTION: SHANDALAR 0x0042476a
int adjust_two_headed_giant_damage(int player, int card, int amount)
{
  card_instance_t *instance;
  int index;
  card_function_pointer code_pointer;
  int found_trampler;
  int damage_source_card;
  int damage_source_player;
  int damage_player;

  damage_player = player;
  found_trampler = 0;
  code_pointer = global_cards_data[PLAYER_CARD_INSTANCE(player, card).internal_card_id].code_pointer;
  damage_source_player = (int)(char)PLAYER_CARD_INSTANCE(player, card).damage_source_player;
  damage_source_card = PLAYER_CARD_INSTANCE(player, card).damage_source_card;

  if (code_pointer == card_two_headed_giant_of_foriys || code_pointer == card_two_headed_giant_of_foriys_legacy)
  {
    found_trampler = 1;
  }

  for (index = 0; index < g_active_cards_count[damage_player] && found_trampler == 0; index = index + 1)
  {
    if (PLAYER_CARD_INSTANCE(damage_player, index).internal_card_id == g_duel_generated_internal_card_id_29 &&
        PLAYER_CARD_INSTANCE(damage_player, index).damage_target_player == player &&
        PLAYER_CARD_INSTANCE(damage_player, index).damage_target_card == card)
    {
      found_trampler = 1;
    }
  }

  if (found_trampler != 0)
  {
    if (code_pointer != card_two_headed_giant_of_foriys_legacy)
    {
      for (index = 0; index < g_active_cards_count[damage_player]; index = index + 1)
      {
        instance = &PLAYER_CARD_INSTANCE(damage_player, index);
        if (global_cards_data[instance->internal_card_id].code_pointer == card_two_headed_giant_of_foriys_legacy &&
            instance->damage_source_player == player &&
            instance->damage_source_card == card)
        {
          instance->power = amount;
          instance->info_slot -= 1;
        }
      }
    }
    else
    {
      for (index = 0; index < g_active_cards_count[damage_player]; index = index + 1)
      {
        instance = &PLAYER_CARD_INSTANCE(damage_player, index);
        if (index != card &&
            global_cards_data[instance->internal_card_id].code_pointer == card_two_headed_giant_of_foriys_legacy &&
            instance->damage_source_player == damage_source_player &&
            instance->damage_source_card == damage_source_card)
        {
          instance->power = amount;
          instance->info_slot -= 1;
        }
      }
    }
  }

  return 0;
}

// FUNCTION: MAGIC 0x00515ee6
// FUNCTION: SHANDALAR 0x00424a96
int is_combat_damage_ability_in_step(int damage_step, int abilities)
{
  if (damage_step == 0 && (abilities & 0x100) != 0)
  {
    return 1;
  }

  if (damage_step != 0 && (abilities & 0x100) == 0)
  {
    return 1;
  }

  return 0;
}

// FUNCTION: MAGIC 0x00516063
// FUNCTION: SHANDALAR 0x00424c13
void clear_combat_damage_assignment_matrix(void)
{
  int blocker_index;
  int attacker_index;

  for (blocker_index = 0; blocker_index < 16; blocker_index = blocker_index + 1)
  {
    for (attacker_index = 0; attacker_index < 16; attacker_index = attacker_index + 1)
    {
      combat_damage_assignment_matrix[attacker_index][blocker_index] = 0;
    }
  }
}

// FUNCTION: MAGIC 0x005160c1
// FUNCTION: SHANDALAR 0x00424c71
void assign_attacker_combat_damage(int player, int attacker_index, int has_trample, int *damage_ids, int damage_step, int mode, int *best_low, int *best_high)
{
  union
  {
    int terminal_total_assigned;
    int assignment_base;
  } shared_value;         // ebp - 0x3c
  int combination_count;  // ebp - 0x38
  int damage_remaining;   // ebp - 0x34
  int defending_player;   // ebp - 0x30
  int assignment_index;   // ebp - 0x2c
  int assignment_code;    // ebp - 0x28
  int blocker_index;      // ebp - 0x24
  int damage_to_assign;   // ebp - 0x20
  int use_shortcut;       // ebp - 0x1c
  int trample_assigned;   // ebp - 0x18
  int attacker_card;      // ebp - 0x14
  int damage_card;        // ebp - 0x10
  int life_after_trample; // ebp - 0xc
  int score;              // ebp - 0x8
  int trample_damage;     // ebp - 0x4

  defending_player = 1 - player;
  if (combat_damage_attacker_count <= attacker_index)
  {
    score = 0;
    life_after_trample = g_life[defending_player];
    for (blocker_index = 0; blocker_index < ai_blocker_count; blocker_index = blocker_index + 1)
    {
      shared_value.terminal_total_assigned = trample_assigned = 0;
      for (assignment_index = 0; assignment_index < combat_damage_attacker_count; assignment_index = assignment_index + 1)
      {
        shared_value.terminal_total_assigned += combat_damage_assignment_matrix[assignment_index][blocker_index];
        if ((combat_damage_attacker_abilities[assignment_index] & 0x80) != 0)
        {
          trample_assigned += combat_damage_assignment_matrix[assignment_index][blocker_index];
          attacker_card = combat_damage_attacker_cards[assignment_index];
        }
      }
      if (combat_damage_blocker_toughness[blocker_index] <= shared_value.terminal_total_assigned)
      {
        score += PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[blocker_index]).attack_rating;
        score += shared_value.terminal_total_assigned - combat_damage_blocker_toughness[blocker_index];
        trample_damage = ClampIntToRange(shared_value.terminal_total_assigned - combat_damage_blocker_toughness[blocker_index], 0, trample_assigned);
        life_after_trample -= trample_damage;
      }
      else
      {
        score += shared_value.terminal_total_assigned * 2;
      }
    }

    if (life_after_trample <= 0)
    {
      score += 999 - life_after_trample * 0x18;
    }
    else
    {
      score += ((g_life[defending_player] - life_after_trample) * 0x30) / life_after_trample;
    }

    if (has_trample == 0)
    {
      if (*best_high < score)
      {
        *best_high = score;
        for (blocker_index = 0; blocker_index < 16; blocker_index = blocker_index + 1)
        {
          combat_damage_best_assignment_codes[blocker_index] = combat_damage_current_assignment_codes[blocker_index];
        }
      }
    }
    else if (score < *best_low)
    {
      *best_low = score;
      for (blocker_index = 0; blocker_index < 16; blocker_index = blocker_index + 1)
      {
        combat_damage_best_assignment_codes[blocker_index] = combat_damage_current_assignment_codes[blocker_index];
      }
    }
  }
  else
  {
    shared_value.assignment_base = combat_damage_attacker_damage[attacker_index] + 1;
    combination_count = 1;
    for (blocker_index = 0; blocker_index < ai_blocker_count; blocker_index = blocker_index + 1)
    {
      combination_count *= shared_value.assignment_base;
    }
    use_shortcut = (g_shandalar_difficulty + 1) * 0x100 < combination_count;

    if (mode != 0)
    {
      damage_remaining = shared_value.assignment_base - 1;
      assignment_code = combat_damage_best_assignment_codes[attacker_index];
      for (assignment_index = 0; assignment_index < ai_blocker_count; assignment_index = assignment_index + 1)
      {
        damage_to_assign = assignment_code % shared_value.assignment_base;
        if (has_damage_card_targeting(defending_player, ai_blocker_cards[assignment_index]) != 0)
        {
          damage_to_assign = 0;
        }
        if (damage_to_assign != 0)
        {
          damage_card = damage_creature(defending_player, ai_blocker_cards[assignment_index], damage_to_assign,
                                        player, combat_damage_attacker_cards[attacker_index]);
          damage_ids[assignment_index] = damage_card;
          if (damage_card != -1)
          {
            PLAYER_CARD_INSTANCE(player, damage_card).token_status |= COMBAT_DAMAGE_FLAG_COMBAT;
            if ((combat_damage_attacker_abilities[attacker_index] & 0x80) != 0)
            {
              PLAYER_CARD_INSTANCE(player, damage_card).token_status |= COMBAT_DAMAGE_FLAG_TRAMPLE;
            }
            if (damage_step == 0)
            {
              PLAYER_CARD_INSTANCE(player, damage_card).token_status |= COMBAT_DAMAGE_FLAG_FIRST_STRIKE;
            }
          }
        }
        combat_damage_assignment_matrix[attacker_index][assignment_index] = damage_to_assign;
        assignment_code /= shared_value.assignment_base;
        damage_remaining -= damage_to_assign;
      }
      assign_attacker_combat_damage(player, attacker_index + 1, has_trample, damage_ids, damage_step, mode, best_low, best_high);
    }
    else if (use_shortcut != 0)
    {
      damage_remaining = shared_value.assignment_base - 1;
      assignment_code = 0;
      for (blocker_index = ai_blocker_count - 1; 0 < blocker_index; blocker_index = blocker_index - 1)
      {
        if (combat_damage_blocker_toughness[blocker_index] < damage_remaining &&
            (combat_damage_blocker_abilities[blocker_index] & 0x200) == 0)
        {
          assignment_code += combat_damage_blocker_toughness[blocker_index];
          combat_damage_assignment_matrix[attacker_index][blocker_index] =
              combat_damage_blocker_toughness[blocker_index];
          damage_remaining -= combat_damage_blocker_toughness[blocker_index];
        }
        assignment_code *= shared_value.assignment_base;
      }
      if (damage_remaining != 0)
      {
        assignment_code += damage_remaining;
      }
      combat_damage_current_assignment_codes[attacker_index] = assignment_code;
      assign_attacker_combat_damage(player, attacker_index + 1, has_trample, damage_ids, damage_step, mode, best_low, best_high);
    }
    else
    {
      combination_count = MIN(combination_count, 5000000);
      for (blocker_index = 0; blocker_index < combination_count; blocker_index = blocker_index + 1)
      {
        damage_remaining = shared_value.assignment_base - 1;
        assignment_code = blocker_index;
        memset(&combat_damage_assignment_matrix[attacker_index][0], 0, 0x40);

        for (assignment_index = 0; assignment_index < ai_blocker_count && 0 < damage_remaining; assignment_index = assignment_index + 1)
        {
          damage_to_assign = assignment_code % shared_value.assignment_base;
          if (use_shortcut != 0 && damage_to_assign > 1 && damage_to_assign < damage_remaining)
          {
            break;
          }
          combat_damage_assignment_matrix[attacker_index][assignment_index] = damage_to_assign;
          assignment_code /= shared_value.assignment_base;
          damage_remaining -= damage_to_assign;
        }
        if (damage_remaining != 0)
        {
          continue;
        }

        combat_damage_current_assignment_codes[attacker_index] = blocker_index;
        assign_attacker_combat_damage(player, attacker_index + 1, has_trample, damage_ids, damage_step, mode, best_low, best_high);
      }
    }
  }
}

// FUNCTION: MAGIC 0x00516736
// FUNCTION: SHANDALAR 0x004252e8
void assign_blocker_combat_damage(int player, int blocker_index, int multiple_attackers, int *damage_ids, int damage_step, int mode, int *best_low, int *best_high)
{
  union
  {
    int terminal_total_assigned;
    int assignment_base;
  } shared_value;
  int combination_count;
  int damage_remaining;
  int defending_player;
  int attacker_index;
  int assignment_code;
  int assignment_index;
  int damage_to_assign;
  int restricted_assignments;
  int damage_card;
  int score;

  defending_player = 1 - player;
  if (!(ai_blocker_count > blocker_index))
  {
    score = 0;
    for (assignment_index = 0; assignment_index < combat_damage_attacker_count; assignment_index = assignment_index + 1)
    {
      shared_value.terminal_total_assigned = 0;
      for (attacker_index = 0; attacker_index < ai_blocker_count; attacker_index = attacker_index + 1)
      {
        shared_value.terminal_total_assigned += combat_damage_assignment_matrix[attacker_index][assignment_index];
      }
      if (combat_damage_attacker_toughness[assignment_index] <= shared_value.terminal_total_assigned)
      {
        score += PLAYER_CARD_INSTANCE(player, combat_damage_attacker_cards[assignment_index]).attack_rating;
        score += shared_value.terminal_total_assigned - combat_damage_attacker_toughness[assignment_index];
      }
      else
      {
        score += shared_value.terminal_total_assigned * 2;
      }
    }

    if (multiple_attackers == 0)
    {
      if (*best_high < score)
      {
        *best_high = score;
        for (assignment_index = 0; assignment_index < 16; assignment_index = assignment_index + 1)
        {
          combat_damage_best_assignment_codes[assignment_index] = combat_damage_current_assignment_codes[assignment_index];
        }
      }
    }
    else if (score < *best_low)
    {
      *best_low = score;
      for (assignment_index = 0; assignment_index < 16; assignment_index = assignment_index + 1)
      {
        combat_damage_best_assignment_codes[assignment_index] = combat_damage_current_assignment_codes[assignment_index];
      }
    }
  }
  else
  {
    if ((PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[blocker_index]).state & STATE_TAPPED) != 0)
    {
      assign_blocker_combat_damage(player, blocker_index + 1, multiple_attackers, damage_ids, damage_step, mode, best_low, best_high);
    }
    else
    {
      shared_value.assignment_base = combat_damage_blocker_damage[blocker_index] + 1;
      combination_count = 1;
      for (assignment_index = 0; assignment_index < combat_damage_attacker_count; assignment_index = assignment_index + 1)
      {
        combination_count *= shared_value.assignment_base;
      }
      restricted_assignments = combination_count > 0x100;

      if (mode != 0)
      {
        damage_remaining = shared_value.assignment_base - 1;
        assignment_code = combat_damage_best_assignment_codes[blocker_index];
        for (attacker_index = 0; attacker_index < combat_damage_attacker_count; attacker_index = attacker_index + 1)
        {
          damage_to_assign = assignment_code % shared_value.assignment_base;
          if (has_damage_card_targeting(defending_player, ai_blocker_cards[attacker_index]) != 0)
          {
            damage_to_assign = 0;
          }
          if (damage_to_assign != 0)
          {
            damage_card = damage_creature(player, combat_damage_attacker_cards[attacker_index], damage_to_assign,
                                          defending_player, ai_blocker_cards[blocker_index]);
            damage_ids[attacker_index] = damage_card;
            if (damage_card != -1)
            {
              PLAYER_CARD_INSTANCE(defending_player, damage_card).token_status |= COMBAT_DAMAGE_FLAG_COMBAT;
              if (damage_step == 0)
              {
                PLAYER_CARD_INSTANCE(defending_player, damage_card).token_status |= COMBAT_DAMAGE_FLAG_FIRST_STRIKE;
              }
            }
          }
          assignment_code /= shared_value.assignment_base;
          damage_remaining -= damage_to_assign;
        }
        assign_blocker_combat_damage(player, blocker_index + 1, multiple_attackers, damage_ids, damage_step, mode, best_low, best_high);
      }
      else
      {
        for (assignment_index = 0; assignment_index < combination_count; assignment_index = assignment_index + 1)
        {
          damage_remaining = shared_value.assignment_base - 1;
          assignment_code = assignment_index;
          for (attacker_index = 0; attacker_index < combat_damage_attacker_count; attacker_index = attacker_index + 1)
          {
            damage_to_assign = assignment_code % shared_value.assignment_base;
            if (restricted_assignments != 0 && damage_to_assign > 1 && damage_to_assign < damage_remaining)
            {
              continue;
            }

            combat_damage_assignment_matrix[blocker_index][attacker_index] = damage_to_assign;
            assignment_code /= shared_value.assignment_base;
            damage_remaining -= damage_to_assign;
          }
          if (damage_remaining != 0)
          {
            continue;
          }

          combat_damage_current_assignment_codes[blocker_index] = assignment_index;
          assign_blocker_combat_damage(player, blocker_index + 1, multiple_attackers, damage_ids, damage_step, mode, best_low, best_high);
        }
      }
    }
  }
}

// FUNCTION: MAGIC 0x00516db1
// FUNCTION: SHANDALAR 0x00425964
void highlight_combat_damage_attacker(int player, int card, int highlight)
{
  if (highlight != 0)
  {
    PLAYER_CARD_INSTANCE(player, card).token_status |= 0x20000;
  }
  else
  {
    PLAYER_CARD_INSTANCE(player, card).token_status &= ~0x20000;
  }
}

// FUNCTION: MAGIC 0x00516e2f
// FUNCTION: SHANDALAR 0x004259e2
void highlight_combat_damage_blocker(int player, int card, int highlight)
{
  if (highlight != 0)
  {
    PLAYER_CARD_INSTANCE(player, card).token_status |= 0x20000;
  }
  else
  {
    PLAYER_CARD_INSTANCE(player, card).token_status &= ~0x20000;
  }
}

// FUNCTION: MAGIC 0x0051724e
// FUNCTION: SHANDALAR 0x00425df8
int has_damage_card_targeting(int player, int card)
{
  int loop_player;
  int loop_card;

  for (loop_player = 0; loop_player < 2; loop_player = loop_player + 1)
  {
    for (loop_card = 0; loop_card < g_active_cards_count[loop_player]; loop_card = loop_card + 1)
    {
      if (global_card_instances[loop_player][loop_card].damage_target_player == player &&
          global_card_instances[loop_player][loop_card].damage_target_card == card &&
          global_cards_data[global_card_instances[loop_player][loop_card].internal_card_id].id == 0x285)
      {
        return 1;
      }
    }
  }

  return 0;
}

// FUNCTION: MAGIC 0x005136ed
// FUNCTION: SHANDALAR 0x00422294
void resolve_combat_damage(int player)
{
  int stop_prompt_result;
  int blocker_score[4];
  int trample_damage;
  int attacker_score_bounds[2]; // ebp - 0xdc
  int blocker_damage_remaining;
  int damage_to_assign;
  int multiple_attackers;
  int scan_card;
  target_t selected_target;
  int played_damage_sound;
  int damage_index;
  int attacker_damage_remaining;
  int blocker_has_trample;
  int blocker_damage_ids[16];
  int defending_player;
  int loop_index; // ebp - 0x68
  int damage_step;
  int assigning_player;
  int card;
  int total_blocker_damage;
  int firststrike_attacker_damage;
  int attacker_damage_ids[16];
  int total_attacker_damage;
  int canceled_assignment;
  int damage_effect_card;
  int target_is_valid;

  defending_player = 1 - player;
  played_damage_sound = 0;

  for (damage_step = 0; damage_step < 2; damage_step++)
  {
    if (damage_step != 0)
    {
      g_current_phase = PHASE_NORMAL_COMBAT_DAMAGE;
    }
    else
    {
      g_current_phase = PHASE_FIRST_STRIKE_DAMAGE;
    }
    update_phase_display(player, g_current_phase);

    for (card = 0; card < g_active_cards_count[player]; card = card + 1)
    {
      if (PLAYER_CARD_INSTANCE(player, card).blocking != -1 &&
          PLAYER_CARD_INSTANCE(player, card).blocking != card)
      {
        continue;
      }

      if (played_damage_sound == 0 && g_duel_ai_mode_state != 1)
      {
        play_sound_effect(0x14);
        played_damage_sound = 1;
      }

      combat_damage_attacker_count = total_attacker_damage = firststrike_attacker_damage = 0;
      multiple_attackers = 0;

      for (scan_card = 0; scan_card < g_active_cards_count[player]; scan_card = scan_card + 1)
      {
        if (((scan_card == card ||
              PLAYER_CARD_INSTANCE(player, scan_card).blocking == card) &&
             PLAYER_CARD_INSTANCE(player, scan_card).internal_card_id != -1) &&
            ((PLAYER_CARD_INSTANCE(player, scan_card).state & (STATE_OUBLIETTED | STATE_ATTACKING | STATE_IN_PLAY)) == (STATE_ATTACKING | STATE_IN_PLAY)))
        {
          combat_damage_attacker_cards[combat_damage_attacker_count] = scan_card;
          combat_damage_attacker_toughness[combat_damage_attacker_count] =
              C_get_abilities(player, scan_card, 0x33, -1);
          combat_damage_attacker_abilities[combat_damage_attacker_count] =
              C_get_abilities(player, scan_card, 0x34, -1);
          combat_damage_attacker_damage[combat_damage_attacker_count] = attacker_damage_remaining = 0;

          if (is_combat_damage_ability_in_step(damage_step, combat_damage_attacker_abilities[combat_damage_attacker_count]) != 0)
          {
            attacker_damage_remaining = C_get_abilities(player, scan_card, 0x32, -1);
            if (attacker_damage_remaining < 0)
            {
              attacker_damage_remaining = 0;
            }
            combat_damage_attacker_damage[combat_damage_attacker_count] = attacker_damage_remaining;
            total_attacker_damage += attacker_damage_remaining;
            if ((combat_damage_attacker_abilities[combat_damage_attacker_count] & 0x80) != 0)
            {
              firststrike_attacker_damage += attacker_damage_remaining;
            }
          }

          combat_damage_attacker_count = combat_damage_attacker_count + 1;
          if (combat_damage_attacker_count == 16)
          {
            break;
          }
        }
      }

      if (1 < combat_damage_attacker_count)
      {
        multiple_attackers = 1;
      }

      ai_blocker_count = total_blocker_damage = 0;
      blocker_has_trample = 0;

      for (scan_card = 0; scan_card < g_active_cards_count[defending_player]; scan_card = scan_card + 1)
      {
        if (is_in_play(defending_player, scan_card) != 0 &&
            PLAYER_CARD_INSTANCE(defending_player, scan_card).blocking == card)
        {
          ai_blocker_cards[ai_blocker_count] = scan_card;
          combat_damage_blocker_toughness[ai_blocker_count] =
              C_get_abilities(defending_player, scan_card, 0x33, card) -
              PLAYER_CARD_INSTANCE(defending_player, scan_card).damage_on_card;
          combat_damage_blocker_abilities[ai_blocker_count] =
              C_get_abilities(defending_player, scan_card, 0x34, -1);

          combat_damage_blocker_damage[ai_blocker_count] = blocker_damage_remaining = 0;

          if ((PLAYER_CARD_INSTANCE(defending_player, scan_card).state & STATE_TAPPED) == 0 &&
              is_combat_damage_ability_in_step(damage_step, combat_damage_blocker_abilities[ai_blocker_count]) != 0)
          {
            blocker_damage_remaining = C_get_abilities(defending_player, scan_card, 0x32, card);
            if (blocker_damage_remaining < 0)
            {
              blocker_damage_remaining = 0;
            }
            combat_damage_blocker_damage[ai_blocker_count] = blocker_damage_remaining;
            total_blocker_damage += blocker_damage_remaining;
          }

          if ((combat_damage_blocker_abilities[ai_blocker_count] & 0x40) != 0)
          {
            blocker_has_trample = 1;
          }

          ai_blocker_count = ai_blocker_count + 1;
          if (ai_blocker_count == 16)
          {
            break;
          }
        }
      }

      if (ai_blocker_count != 0)
      {
        for (damage_index = 0; damage_index < combat_damage_attacker_count; damage_index = damage_index + 1)
        {
          PLAYER_CARD_INSTANCE(player, combat_damage_attacker_cards[damage_index]).state |= 0x200;
        }
      }

      if (combat_damage_attacker_count == 0 && ai_blocker_count == 0)
      {
        continue;
      }

      if (ai_blocker_count > 1)
      {
        if ((blocker_has_trample == 0 && player == g_other_player) ||
            (blocker_has_trample != 0 && player == g_active_player))
        {
          assigning_player = g_other_player;
        }
        else
        {
          assigning_player = g_active_player;
        }

        if ((g_duel_network_flags & 2) == 0 &&
            (g_duel_ai_mode_state == 1 || assigning_player == g_other_player))
        {
          attacker_score_bounds[0] = 0x7fffffff;
          attacker_score_bounds[1] = 0xffff8001;
          clear_combat_damage_assignment_matrix();
          for (loop_index = 0; loop_index < 16; loop_index = loop_index + 1)
          {
            attacker_damage_ids[loop_index] = -1;
          }
          assign_attacker_combat_damage(player, 0, blocker_has_trample, attacker_damage_ids, damage_step, 0,
                                        attacker_score_bounds, attacker_score_bounds + 1);
          assign_attacker_combat_damage(player, 0, blocker_has_trample, attacker_damage_ids, damage_step, 1,
                                        attacker_score_bounds, attacker_score_bounds + 1);
        }
        else
        {
          for (damage_index = 0; damage_index < combat_damage_attacker_count; damage_index = damage_index + 1)
          {
            for (loop_index = 0; loop_index < 16; loop_index = loop_index + 1)
            {
              attacker_damage_ids[loop_index] = -1;
            }
            attacker_damage_remaining = combat_damage_attacker_damage[damage_index];

            while (attacker_damage_remaining != 0)
            {
              load_text(global_ui_strings_filename, "PROMPT_RESOLVECOMBAT");
              if (g_duel_interface_options.layout != 2)
              {
                if (combat_damage_attacker_abilities[damage_index] & 0x80)
                {
                  sprintf(g_ui_message_buffer, g_text_lines[1],
                          get_displayed_card_name(player, combat_damage_attacker_cards[damage_index]),
                          attacker_damage_remaining);
                }
                else
                {
                  sprintf(g_ui_message_buffer, g_text_lines[0],
                          get_displayed_card_name(player, combat_damage_attacker_cards[damage_index]),
                          attacker_damage_remaining);
                }
              }
              else if (combat_damage_attacker_abilities[damage_index] & 0x80)
              {
                sprintf(g_ui_message_buffer, g_text_lines[3], attacker_damage_remaining);
              }
              else
              {
                sprintf(g_ui_message_buffer, g_text_lines[2], attacker_damage_remaining);
              }

              highlight_combat_damage_attacker(player, combat_damage_attacker_cards[damage_index], 1);
              target_is_valid = 0;
              while (target_is_valid == 0)
              {
                C_real_select_target(assigning_player, defending_player, defending_player, 0x200, 2, 0, 0, 0, 0, 0,
                                     -1, -1, -1, -1, 0, 0x10, 0, g_ui_message_buffer, 0, &selected_target);
                for (loop_index = 0; loop_index < ai_blocker_count; loop_index = loop_index + 1)
                {
                  if (ai_blocker_cards[loop_index] == selected_target.card)
                  {
                    target_is_valid = 1;
                  }
                }
                if (target_is_valid == 0 && g_duel_ai_mode_state != 1)
                {
                  set_duel_prompt_text(g_text_lines[4]);
                  Sleep(1500);
                  set_duel_prompt_text("");
                }
                if (target_is_valid == 1 && has_damage_card_targeting(selected_target.player, selected_target.card) != 0)
                {
                  target_is_valid = 0;
                  if (g_duel_ai_mode_state != 1)
                  {
                    set_duel_prompt_text(g_text_lines[5]);
                    Sleep(1500);
                    set_duel_prompt_text("");
                  }
                }
              }

              strcpy(g_ui_message_buffer, "");
              highlight_combat_damage_attacker(player, combat_damage_attacker_cards[damage_index], 0);
              if (selected_target.player != -1 && selected_target.card != -1 && selected_target.card != -2)
              {
                for (loop_index = 0; loop_index < ai_blocker_count; loop_index = loop_index + 1)
                {
                  if (ai_blocker_cards[loop_index] == selected_target.card)
                  {
                    if (g_recorded_action_player != 0)
                    {
                      damage_to_assign = attacker_damage_remaining;
                    }
                    else
                    {
                      damage_to_assign = 1;
                    }

                    if (attacker_damage_ids[loop_index] == -1)
                    {
                      damage_effect_card = damage_creature(selected_target.player, selected_target.card,
                                                           damage_to_assign,
                                                           player, combat_damage_attacker_cards[damage_index]);
                      attacker_damage_ids[loop_index] = damage_effect_card;
                      if (damage_effect_card != -1)
                      {
                        PLAYER_CARD_INSTANCE(player, damage_effect_card).token_status |= COMBAT_DAMAGE_FLAG_COMBAT;
                        if ((combat_damage_attacker_abilities[damage_index] & 0x80) != 0)
                        {
                          PLAYER_CARD_INSTANCE(player, damage_effect_card).token_status |= COMBAT_DAMAGE_FLAG_TRAMPLE;
                        }
                        if (damage_step == 0)
                        {
                          PLAYER_CARD_INSTANCE(player, damage_effect_card).token_status |= COMBAT_DAMAGE_FLAG_FIRST_STRIKE;
                        }
                      }
                    }
                    else
                    {
                      PLAYER_CARD_INSTANCE(player, attacker_damage_ids[loop_index]).info_slot += damage_to_assign;
                    }
                    attacker_damage_remaining -= damage_to_assign;
                  }
                }
              }
            }
          }
        }
      }
      else if (ai_blocker_count == 1)
      {
        for (damage_index = 0; damage_index < combat_damage_attacker_count; damage_index = damage_index + 1)
        {
          if (is_combat_damage_ability_in_step(damage_step, combat_damage_attacker_abilities[damage_index]) != 0)
          {
            damage_effect_card = damage_creature(defending_player, ai_blocker_cards[0],
                                                 combat_damage_attacker_damage[damage_index],
                                                 player, combat_damage_attacker_cards[damage_index]);
            attacker_damage_ids[0] = damage_effect_card;
            if (damage_effect_card != -1)
            {
              PLAYER_CARD_INSTANCE(player, damage_effect_card).token_status |= COMBAT_DAMAGE_FLAG_COMBAT;
              if ((combat_damage_attacker_abilities[damage_index] & 0x80) != 0)
              {
                PLAYER_CARD_INSTANCE(player, damage_effect_card).token_status |= COMBAT_DAMAGE_FLAG_TRAMPLE;
              }
              if (damage_step == 0)
              {
                PLAYER_CARD_INSTANCE(player, damage_effect_card).token_status |= COMBAT_DAMAGE_FLAG_FIRST_STRIKE;
              }
            }
          }
        }
      }
      else
      {
        for (damage_index = 0; damage_index < combat_damage_attacker_count; damage_index = damage_index + 1)
        {
          if (is_combat_damage_ability_in_step(damage_step, combat_damage_attacker_abilities[damage_index]) != 0 &&
              ((PLAYER_CARD_INSTANCE(player, combat_damage_attacker_cards[damage_index]).state & STATE_ISBLOCKED) == 0 ||
               (combat_damage_attacker_abilities[damage_index] & 0x80) != 0))
          {
            damage_player(defending_player, combat_damage_attacker_damage[damage_index],
                          player, combat_damage_attacker_cards[damage_index]);
          }
        }
      }

      if ((g_battlefield_extra_ability_flags & 4) != 0 &&
          defending_player == g_other_player &&
          (g_duel_network_flags & 2) == 0)
      {
        trample_damage = 0;
        blocker_score[2] = 0;
        if (combat_damage_attacker_count > 1)
        {
          for (damage_index = 0; damage_index < ai_blocker_count; damage_index = damage_index + 1)
          {
            if ((PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[damage_index]).token_status & COMBAT_DAMAGE_FLAG_CANCELABLE) != 0 &&
                (global_cards_data[PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[damage_index]).internal_card_id].code_pointer != card_two_headed_giant_of_foriys_legacy ||
                 PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[damage_index]).info_slot >= 2))
            {
              adjust_two_headed_giant_damage(defending_player, ai_blocker_cards[damage_index], combat_damage_blocker_damage[damage_index]);
              combat_damage_blocker_damage[damage_index] = 0;
            }
          }
        }
        else
        {
          for (damage_index = 0; damage_index < ai_blocker_count; damage_index = damage_index + 1)
          {
            if (PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[damage_index]).token_status & COMBAT_DAMAGE_FLAG_CANCELABLE)
            {
              trample_damage += combat_damage_blocker_damage[damage_index];
            }
            else
            {
              blocker_score[2] += combat_damage_blocker_damage[damage_index];
            }
          }

          for (damage_index = 0; damage_index < ai_blocker_count; damage_index = damage_index + 1)
          {
            if ((PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[damage_index]).token_status & COMBAT_DAMAGE_FLAG_CANCELABLE) != 0)
            {
              blocker_score[3] = combat_damage_blocker_damage[damage_index];
              if (PLAYER_CARD_INSTANCE(player, combat_damage_attacker_cards[0]).toughness <= blocker_score[2] + trample_damage)
              {
                while (blocker_score[3] != 0 &&
                       blocker_score[2] < PLAYER_CARD_INSTANCE(player, combat_damage_attacker_cards[0]).toughness)
                {
                  blocker_score[2] += 1;
                  trample_damage -= 1;
                  blocker_score[3] -= 1;
                }
              }
              if (blocker_score[3] != 0 &&
                  (global_cards_data[PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[damage_index]).internal_card_id].code_pointer != card_two_headed_giant_of_foriys_legacy ||
                   PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[damage_index]).info_slot >= 2))
              {
                adjust_two_headed_giant_damage(defending_player, ai_blocker_cards[damage_index], blocker_score[3]);
                combat_damage_blocker_damage[damage_index] -= blocker_score[3];
              }
            }
          }
        }
      }

      if (combat_damage_attacker_count > 1 ||
          ((g_battlefield_extra_ability_flags & 4) &&
           (defending_player == g_active_player || (g_duel_network_flags & 2))))
      {
        if ((multiple_attackers == 0 && player == g_active_player) ||
            (multiple_attackers != 0 && player == g_other_player))
        {
          assigning_player = g_other_player;
        }
        else
        {
          assigning_player = g_active_player;
        }

        if ((g_duel_network_flags & 2) == 0 &&
            (g_duel_ai_mode_state == 1 || assigning_player == g_other_player))
        {
          blocker_score[0] = 0x7fffffff;
          blocker_score[1] = -1;
          clear_combat_damage_assignment_matrix();
          for (loop_index = 0; loop_index < 16; loop_index = loop_index + 1)
          {
            blocker_damage_ids[loop_index] = -1;
          }
          assign_blocker_combat_damage(player, 0, multiple_attackers, blocker_damage_ids, damage_step, 0, blocker_score, blocker_score + 1);
          assign_blocker_combat_damage(player, 0, multiple_attackers, blocker_damage_ids, damage_step, 1, blocker_score, blocker_score + 1);
        }
        else
        {
          for (damage_index = 0; damage_index < ai_blocker_count; damage_index = damage_index + 1)
          {
            for (loop_index = 0; loop_index < 16; loop_index = loop_index + 1)
            {
              blocker_damage_ids[loop_index] = -1;
            }
            blocker_damage_remaining = combat_damage_blocker_damage[damage_index];
            canceled_assignment = 0;

            while (blocker_damage_remaining != 0 && canceled_assignment == 0)
            {
              load_text(global_ui_strings_filename, "PROMPT_RESOLVECOMBAT");
              if (g_duel_interface_options.layout != 2)
              {
                sprintf(g_ui_message_buffer, g_text_lines[6],
                        get_displayed_card_name(defending_player, ai_blocker_cards[damage_index]), blocker_damage_remaining);
              }
              else
              {
                sprintf(g_ui_message_buffer, g_text_lines[7], blocker_damage_remaining);
              }

              highlight_combat_damage_blocker(defending_player, ai_blocker_cards[damage_index], 1);
              target_is_valid = 0;
              if (combat_damage_attacker_count == 1 &&
                  ((PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[damage_index]).token_status & COMBAT_DAMAGE_FLAG_CANCELABLE) == 0 ||
                   (global_cards_data[PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[damage_index]).internal_card_id].code_pointer == card_two_headed_giant_of_foriys_legacy &&
                    PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[damage_index]).info_slot < 2)))
              {
                target_is_valid = 1;
                g_recorded_action_player = 0;
                selected_target.player = player;
                selected_target.card = combat_damage_attacker_cards[0];
              }

              while (target_is_valid == 0)
              {
                C_real_select_target(assigning_player, player, player, 0x200, 2, 0, 0, 0, 0, 0,
                                     -1, -1, -1, -1, 0, 2, 0, g_ui_message_buffer,
                                     (PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[damage_index]).token_status &
                                      COMBAT_DAMAGE_FLAG_CANCELABLE) >>
                                         26,
                                     &selected_target);
                if (selected_target.card == -2)
                {
                  target_is_valid = 1;
                  canceled_assignment = 1;
                  adjust_two_headed_giant_damage(defending_player, ai_blocker_cards[damage_index],
                                                 blocker_damage_remaining);
                }
                else
                {
                  for (loop_index = 0; loop_index < combat_damage_attacker_count; loop_index = loop_index + 1)
                  {
                    if (combat_damage_attacker_cards[loop_index] == selected_target.card)
                    {
                      target_is_valid = 1;
                    }
                  }
                  if (target_is_valid == 0 && g_duel_ai_mode_state != 1)
                  {
                    set_duel_prompt_text(g_text_lines[8]);
                    Sleep(1500);
                    set_duel_prompt_text("");
                  }
                  if (target_is_valid == 1 && has_damage_card_targeting(selected_target.player, selected_target.card) != 0)
                  {
                    target_is_valid = 0;
                    if (g_duel_ai_mode_state != 1)
                    {
                      set_duel_prompt_text(g_text_lines[9]);
                      Sleep(1500);
                      set_duel_prompt_text("");
                    }
                  }
                }
              }

              strcpy(g_ui_message_buffer, "");
              highlight_combat_damage_blocker(defending_player, ai_blocker_cards[damage_index], 0);
              if (selected_target.player != -1 && selected_target.card != -1 && selected_target.card != -2)
              {
                for (loop_index = 0; loop_index < combat_damage_attacker_count; loop_index = loop_index + 1)
                {
                  if (combat_damage_attacker_cards[loop_index] == selected_target.card)
                  {
                    if (g_recorded_action_player != 0 ||
                        (global_cards_data[PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[damage_index]).internal_card_id].code_pointer == card_two_headed_giant_of_foriys_legacy &&
                         PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[damage_index]).info_slot < 2))
                    {
                      damage_to_assign = blocker_damage_remaining;
                    }
                    else
                    {
                      damage_to_assign = 1;
                    }
                    if (blocker_damage_ids[loop_index] == -1)
                    {
                      damage_effect_card = damage_creature(player, selected_target.card,
                                                           damage_to_assign,
                                                           defending_player, ai_blocker_cards[damage_index]);
                      blocker_damage_ids[loop_index] = damage_effect_card;
                      if (damage_effect_card != -1)
                      {
                        PLAYER_CARD_INSTANCE(defending_player, damage_effect_card).token_status |= COMBAT_DAMAGE_FLAG_COMBAT;
                        if (damage_step == 0)
                        {
                          PLAYER_CARD_INSTANCE(defending_player, damage_effect_card).token_status |= COMBAT_DAMAGE_FLAG_FIRST_STRIKE;
                        }
                      }
                    }
                    else
                    {
                      PLAYER_CARD_INSTANCE(defending_player, blocker_damage_ids[loop_index]).info_slot += damage_to_assign;
                    }
                    blocker_damage_remaining -= damage_to_assign;
                  }
                }
              }
            }
          }

          if ((PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[damage_index - 1]).token_status & COMBAT_DAMAGE_FLAG_CANCELABLE) != 0 &&
              selected_target.card != -2 &&
              (global_cards_data[PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[damage_index - 1]).internal_card_id].code_pointer != card_two_headed_giant_of_foriys_legacy ||
               PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[damage_index - 1]).info_slot >= 2) &&
              ((damage_step == 0 && (PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[damage_index - 1]).regen_status & 0x100) != 0) ||
               (damage_step == 1 && (PLAYER_CARD_INSTANCE(defending_player, ai_blocker_cards[damage_index - 1]).regen_status & 0x100) == 0)))
          {
            adjust_two_headed_giant_damage(defending_player, ai_blocker_cards[damage_index - 1],
                                           blocker_damage_remaining);
          }
        }
      }
      else if (combat_damage_attacker_count == 1)
      {
        for (damage_index = 0; damage_index < ai_blocker_count; damage_index = damage_index + 1)
        {
          if (is_combat_damage_ability_in_step(damage_step, combat_damage_blocker_abilities[damage_index]) != 0)
          {
            damage_effect_card = damage_creature(player, combat_damage_attacker_cards[0],
                                                 combat_damage_blocker_damage[damage_index],
                                                 defending_player, ai_blocker_cards[damage_index]);
            blocker_damage_ids[0] = damage_effect_card;
            if (damage_effect_card != -1)
            {
              PLAYER_CARD_INSTANCE(defending_player, damage_effect_card).token_status |= COMBAT_DAMAGE_FLAG_COMBAT;
              if (damage_step == 0)
              {
                PLAYER_CARD_INSTANCE(defending_player, damage_effect_card).token_status |= COMBAT_DAMAGE_FLAG_FIRST_STRIKE;
              }
            }
          }
        }
      }
    }

    ai_blocker_count = 0;
    for (scan_card = 0; scan_card < g_active_cards_count[defending_player]; scan_card = scan_card + 1)
    {
      if (PLAYER_CARD_INSTANCE(defending_player, scan_card).internal_card_id != -1 &&
          PLAYER_CARD_INSTANCE(defending_player, scan_card).blocking != -1)
      {
        ai_blocker_cards[ai_blocker_count] = scan_card;
        combat_damage_blocker_toughness[ai_blocker_count] =
            C_get_abilities(defending_player, scan_card, 0x33, card) -
            PLAYER_CARD_INSTANCE(defending_player, scan_card).damage_on_card;
        if (has_damage_card_targeting(defending_player, scan_card) != 0)
        {
          combat_damage_blocker_toughness[ai_blocker_count] = 0;
        }
        ai_blocker_count = ai_blocker_count + 1;
        if (ai_blocker_count == 16)
        {
          break;
        }
      }
    }

    if (g_duel_ai_mode_state != 1)
    {
      g_phase_was_skipped = 0;
    }
    g_phase_response_window_open = 0;
    if (g_duel_ai_mode_state != 1)
    {
      g_phase_stop_suppressed = 0;
    }

    process_damage_prevention(player);
    if (g_phase_was_skipped == 0 && (stop_prompt_result = prompt_stop_phase_anyway(g_current_phase)) != 0 && g_duel_ai_mode_state != 1)
    {
      g_phase_was_skipped = 1;
    }

    for (loop_index = 0; loop_index < ai_blocker_count; loop_index = loop_index + 1)
    {
      scan_card = ai_blocker_cards[loop_index];
      blocker_damage_remaining = combat_damage_blocker_toughness[loop_index];

      for (card = 0; card < g_active_cards_count[player]; card = card + 1)
      {
        if (PLAYER_CARD_INSTANCE(player, card).original_internal_card_id == g_damage_card_internal_card_id &&
            PLAYER_CARD_INSTANCE(player, card).damage_target_player == defending_player &&
            PLAYER_CARD_INSTANCE(player, card).damage_target_card == scan_card &&
            ((damage_step == 0 && (PLAYER_CARD_INSTANCE(player, card).token_status & COMBAT_DAMAGE_FLAG_FIRST_STRIKE) != 0) ||
             (damage_step == 1 && (PLAYER_CARD_INSTANCE(player, card).token_status & COMBAT_DAMAGE_FLAG_FIRST_STRIKE) == 0)) &&
            (PLAYER_CARD_INSTANCE(player, card).token_status & COMBAT_DAMAGE_FLAG_COMBAT) != 0 &&
            (PLAYER_CARD_INSTANCE(player, card).token_status & COMBAT_DAMAGE_FLAG_TRAMPLE) == 0)
        {
          blocker_damage_remaining -= PLAYER_CARD_INSTANCE(player, card).info_slot;
        }
      }

      for (card = 0; card < g_active_cards_count[player]; card = card + 1)
      {
        if (PLAYER_CARD_INSTANCE(player, card).original_internal_card_id == g_damage_card_internal_card_id &&
            PLAYER_CARD_INSTANCE(player, card).damage_target_player == defending_player &&
            PLAYER_CARD_INSTANCE(player, card).damage_target_card == scan_card &&
            ((damage_step == 0 && (PLAYER_CARD_INSTANCE(player, card).token_status & COMBAT_DAMAGE_FLAG_FIRST_STRIKE) != 0) ||
             (damage_step == 1 && (PLAYER_CARD_INSTANCE(player, card).token_status & COMBAT_DAMAGE_FLAG_FIRST_STRIKE) == 0)) &&
            (PLAYER_CARD_INSTANCE(player, card).token_status & COMBAT_DAMAGE_FLAG_TRAMPLE) != 0 &&
            blocker_damage_remaining - (PLAYER_CARD_INSTANCE(player, card).unknown0x37 +
                                        PLAYER_CARD_INSTANCE(player, card).info_slot) <
                0)
        {
          damage_player(defending_player,
                        (-(blocker_damage_remaining - (PLAYER_CARD_INSTANCE(player, card).unknown0x37 +
                                                       PLAYER_CARD_INSTANCE(player, card).info_slot)) <
                         (PLAYER_CARD_INSTANCE(player, card).unknown0x37 + PLAYER_CARD_INSTANCE(player, card).info_slot))
                            ? -(blocker_damage_remaining - (PLAYER_CARD_INSTANCE(player, card).unknown0x37 +
                                                            PLAYER_CARD_INSTANCE(player, card).info_slot))
                            : (PLAYER_CARD_INSTANCE(player, card).unknown0x37 + PLAYER_CARD_INSTANCE(player, card).info_slot),
                        (int)(char)PLAYER_CARD_INSTANCE(player, card).damage_source_player, PLAYER_CARD_INSTANCE(player, card).damage_source_card);
          blocker_damage_remaining -= PLAYER_CARD_INSTANCE(player, card).unknown0x37 +
                                      PLAYER_CARD_INSTANCE(player, card).info_slot;
        }
      }
    }

    regenerate_or_graveyard_triggers();
    process_damage_prevention(player);
    if ((g_battlefield_extra_ability_flags & 1) != 0)
    {
      g_battlefield_extra_ability_flags &= ~1;
      pending_killed_card_handler = process_killed_card;
      regenerate_or_graveyard_triggers();
      process_damage_prevention(player);
    }

    g_phase_response_window_open = 0;
    if (g_duel_ai_mode_state != 1)
    {
      g_phase_stop_suppressed = 0;
    }
    if (g_phase_was_skipped == 0)
    {
      prompt_stop_phase_anyway(g_current_phase);
    }
  }
}
