#ifndef SHANDALAR_DECK_CORE_H
#define SHANDALAR_DECK_CORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum DeckStatus {
  DECK_RUNNING,
  DECK_COMPLETED,
  DECK_FAILED
} DeckStatus;

typedef struct DeckConfig {
  const char *asset_root;
  const char *writable_root;
  int width;
  int height;
  int prefetch_cardart;
} DeckConfig;

typedef struct DeckFrame {
  const uint32_t *pixels;
  int width;
  int height;
  int pitch;
  uint64_t generation;
} DeckFrame;

int deck_create(const DeckConfig *config);
int deck_resize(int width, int height);
void deck_destroy(void);
DeckStatus deck_step(uint64_t now_ns);
int deck_get_frame(DeckFrame *frame);
const char *deck_status_text(void);
void deck_pointer_move(float x, float y);
void deck_pointer_button(int button, int pressed, int clicks);
void deck_key(int key, int pressed, int repeat);
void deck_prefetch_cardart(void);

int deck_ui_is_interactive(void);
int deck_ui_width(void);
int deck_ui_height(void);
int deck_ui_scale_x(int logical_x);
int deck_ui_scale_y(int logical_y);
int deck_ui_scale_width(int logical_left, int logical_right);
int deck_ui_scale_height(int logical_top, int logical_bottom);
int deck_ui_available_count(void);
int deck_ui_available_csvid(int index);
int deck_ui_selected_index(void);
void deck_ui_select_available_index(int index);
void deck_ui_preview_card(int csvid);
int deck_ui_deck_count(void);
int deck_ui_deck_entry(int index, int *csvid, int *amount,
                       int *x, int *y, int *width, int *height);
const char *deck_ui_card_name(int csvid);
const char *deck_ui_card_type(int csvid);
const char *deck_ui_card_rules(int csvid);
void deck_ui_add_card(int csvid);
void deck_ui_remove_card(int csvid);
int deck_ui_render_small_card(int csvid, uint32_t *pixels, int width, int height);
int deck_ui_expanded_text(void);
void deck_ui_set_expanded_text(int expanded);

#ifdef __cplusplus
}
#endif

#endif
