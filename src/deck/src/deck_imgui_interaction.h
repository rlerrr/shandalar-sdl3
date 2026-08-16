#ifndef SHANDALAR_DECK_IMGUI_INTERACTION_H
#define SHANDALAR_DECK_IMGUI_INTERACTION_H

#ifdef __cplusplus
extern "C" {
#endif

void shandalar_deck_imgui_interaction_present(void);
void shandalar_deck_imgui_interaction_shutdown(void);
void shandalar_deck_imgui_set_menu_sprite(const void *encoded_sprite,
                                           const unsigned char *palette_bgra);

#ifdef __cplusplus
}
#endif

#endif
