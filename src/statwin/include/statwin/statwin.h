#ifndef SHANDALAR_STATWIN_H
#define SHANDALAR_STATWIN_H

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StatWinData {
  int progress_by_color[5];
  int duel_wins_by_color[5];
  unsigned char town_count_by_color[5];
  unsigned char highlighted_wizard_color;
  unsigned char highlighted_creature_type;
  unsigned char highlighted_lair_color;
  int reserved;
} StatWinData;
SDL_COMPILE_TIME_ASSERT(statwin_data_size, sizeof(StatWinData) == 52);

void statwin_set_renderer(SDL_Renderer *renderer);
int statwin_init(const StatWinData *data);
void statwin_show(const StatWinData *data, int mode);
void statwin_prefetch_wizard_defeat_media(int wizard_color);
void statwin_prefetch_arzakon_defeat_media(void);
int statwin_play_video(const char *path, int x, int y, int flags);
void statwin_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
