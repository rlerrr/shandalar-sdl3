#ifndef SHANDALAR_DECK_DRM_H
#define SHANDALAR_DECK_DRM_H

#include "defs.h"

int IsCardAvailable(csvid_t csvid, int expansion);
int is_card_available_in_expansion(unsigned int csvid, int expansion);
unsigned int HasExpansion(unsigned char expansion_mask);

#endif
