#ifndef SHANDALAR_DECK_RULES_H
#define SHANDALAR_DECK_RULES_H

/* Keep deck-builder warnings and duel deck padding on the same difficulty rules. */
static inline int shandalar_minimum_deck_size(int difficulty)
{
  static const int minimums[] = {30, 35, 40, 40};
  return minimums[difficulty >= 0 && difficulty < 4 ? difficulty : 3];
}

#endif
