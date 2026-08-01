#include "statwin/statwin.h"

#include "magsnd/magsnd.h"
#include "platform/asset_manager.h"
#include "platform/file_io.h"

#include <SDL3_image/SDL_image.h>
#include <string.h>

#define STATWIN_ASSET_DIR "Statwin_SDL"
#define STATWIN_SOUND_SLOT 0xfe
#define STATWIN_STATUS_SOUND_SLOT 0xff

typedef struct RectI {
  int x;
  int y;
  int w;
  int h;
} RectI;

typedef struct StatsAssets {
  SDL_Texture *background;
  SDL_Texture *sprites[5];
  SDL_Texture *yellow[5];
  SDL_Texture *red[5];
  SDL_Texture *mana[5];
  SDL_Texture *skulls[7];
} StatsAssets;

typedef struct Scene {
  SDL_Texture *background;
  StatsAssets *stats;
  const StatWinData *data;
  SDL_Texture *animation;
  int animation_x;
  int animation_y;
  int animation_relative_to_background;
} Scene;

static SDL_Renderer *g_renderer;
static StatWinData g_cached_data;
static int g_has_cached_data;

static const char *const g_creature_animations[56] = {
    "CBALL00.WEBP", "B_FWZ.WEBP", "B_KHT.WEBP", "B_MWZ.WEBP", "B_LRD.WEBP", "B_WG.WEBP", "B_AMG.WEBP",
    "CBAL00.WEBP", "W_MWZ.WEBP", "W_FWZ.WEBP", "W_KHT.WEBP", "W_LRD.WEBP", "W_WG.WEBP", "W_AMG.WEBP",
    "CBALL00.WEBP", "U_FWZ.WEBP", "U_LRD.WEBP", "U_MWZ.WEBP", "U_WRM.WEBP", "U_SFT.WEBP", "U_AMG.WEBP",
    "CBALL00.WEBP", "G_MWZ.WEBP", "G_KHT.WEBP", "G_FWZ.WEBP", "G_WRM.WEBP", "G_LRD.WEBP", "G_AMG.WEBP",
    "CBALL00.WEBP", "R_FWZ.WEBP", "R_MWZ.WEBP", "R_TRL.WEBP", "R_LRD.WEBP", "R_WRM.WEBP", "R_AMG.WEBP",
    "R_AMG.WEBP", "M_TSK.WEBP", "M_TRL.WEBP", "M_APE.WEBP", "M_CEN2.WEBP", "M_WG.WEBP", "M_FNG.WEBP",
    "M_CEN.WEBP", "M_LRD.WEBP", "M_KHT.WEBP", "M_FWZ.WEBP", "B_DJN.WEBP", "G_DJN.WEBP", "R_DJN.WEBP",
    "U_DJN.WEBP", "DG_BRU.WEBP", "DG_UWB.WEBP", "DG_GWR.WEBP", "DG_RGB.WEBP", "DG_WUG.WEBP", "CBALL00.WEBP"};

static const char *const g_hit_animations[5] = {
    "WHIT-HIT.WEBP", "RED-HIT.WEBP", "BLUE-HIT.WEBP", "GRN-HIT.WEBP", "BLAK-HIT.WEBP"};

static const char *const g_missing_animations[5] = {
    "BUBBLES.WEBP", "FIRE.WEBP", "WATER.WEBP", "VINE.WEBP", "SMOKE.WEBP"};

static const unsigned char g_defeated_wizard_colors[5] = {4, 2, 3, 1, 0};

static const char *const g_missing_backgrounds[5] = {
    "ROOM-WHT.PNG", "ROOM-RED.PNG", "ROOM-BLU.PNG", "ROOM-GRN.PNG", "ROOM-BLK.PNG"};

static const RectI g_missing_animation_rects[5] = {
    {140, 0, 360, 440}, {160, 0, 300, 440}, {212, 48, 240, 412},
    {136, 12, 368, 432}, {200, 0, 260, 460}};

static const RectI g_hit_rects[5] = {
    {248, 60, 144, 300}, {340, 48, 200, 300}, {0, 48, 260, 320},
    {140, 60, 160, 280}, {380, 48, 260, 300}};

static const char *const g_sprite_names[5] = {
    "WHT-SPRT.PNG", "RED-SPRT.PNG", "BLU-SPRT.PNG", "GRN-SPRT.PNG", "BLK-SPRT.PNG"};
static const char *const g_yellow_names[5] = {
    "S-WWIZY.PNG", "S-RWIZY.PNG", "S-BUWIZY.PNG", "S-GWIZY.PNG", "S-BKWIZY.PNG"};
static const char *const g_red_names[5] = {
    "S-WWIZR.PNG", "S-RWIZR.PNG", "S-BUWIZR.PNG", "S-GWIZR.PNG", "S-BKWIZR.PNG"};
static const char *const g_mana_names[5] = {
    "W-MANA.PNG", "R-MANA.PNG", "U-MANA.PNG", "G-MANA.PNG", "B-MANA.PNG"};
static const char *const g_skull_names[7] = {
    "1SKULLS.PNG", "2SKULLS.PNG", "3SKULLS.PNG", "4SKULLS.PNG",
    "5SKULLS.PNG", "8SKULLS.PNG", "12SKULLS.PNG"};

static const RectI g_sprite_rects[5] = {
    {272, 60, 104, 300}, {376, 48, 136, 300}, {0, 48, 160, 300},
    {160, 56, 104, 300}, {516, 48, 124, 300}};
static const RectI g_progress_rects[5] = {
    {330, 86, 60, 252}, {468, 80, 60, 260}, {0, 38, 80, 300},
    {140, 38, 60, 300}, {580, 12, 60, 324}};
static const RectI g_mana_rects[5] = {
    {294, 30, 48, 48}, {428, 24, 48, 48}, {58, 12, 48, 48},
    {184, 24, 48, 48}, {544, 12, 48, 48}};
static const RectI g_skull_rects[5] = {
    {260, 240, 120, 240}, {380, 240, 140, 240}, {0, 240, 140, 240},
    {140, 240, 120, 240}, {520, 240, 120, 240}};

static const RectI g_mana_sources[5][5] = {
    {{31, 13, 16, 15}, {3, 13, 18, 15}, {18, 4, 15, 16}, {9, 28, 17, 15}, {26, 27, 18, 17}},
    {{32, 10, 13, 14}, {4, 13, 14, 14}, {17, 13, 14, 14}, {8, 27, 15, 15}, {24, 25, 17, 17}},
    {{30, 18, 18, 15}, {0, 8, 19, 18}, {19, 1, 15, 18}, {6, 28, 19, 17}, {27, 31, 17, 16}},
    {{32, 15, 14, 16}, {4, 12, 14, 16}, {8, 5, 14, 13}, {8, 28, 16, 14}, {24, 28, 16, 15}},
    {{29, 8, 19, 19}, {0, 14, 17, 20}, {12, 0, 19, 21}, {0, 33, 26, 14}, {23, 29, 20, 16}}};

static SDL_IOStream *open_asset_io(const char *name)
{
  char relative[512];
  SDL_IOStream *io;
  SDL_snprintf(relative, sizeof(relative), "%s/%s", STATWIN_ASSET_DIR, name);
  io = shandalar_open_file_io(relative, "rb");
  if (!io)
    SDL_Log("statwin: could not open %s", relative);
  return io;
}

static SDL_Texture *load_texture(const char *name)
{
  SDL_IOStream *io;
  SDL_Texture *texture;
  if (!g_renderer)
    return NULL;
  io = open_asset_io(name);
  if (!io)
    return NULL;
  texture = IMG_LoadTexture_IO(g_renderer, io, true);
  if (!texture)
    SDL_Log("statwin: could not load %s: %s", name, SDL_GetError());
  return texture;
}

static void destroy_stats_assets(StatsAssets *assets)
{
  int i;
  if (!assets)
    return;
  SDL_DestroyTexture(assets->background);
  for (i = 0; i < 5; ++i) {
    SDL_DestroyTexture(assets->sprites[i]);
    SDL_DestroyTexture(assets->yellow[i]);
    SDL_DestroyTexture(assets->red[i]);
    SDL_DestroyTexture(assets->mana[i]);
  }
  for (i = 0; i < 7; ++i)
    SDL_DestroyTexture(assets->skulls[i]);
  SDL_zero(*assets);
}

static int load_stats_assets(StatsAssets *assets)
{
  int i;
  SDL_zero(*assets);
  assets->background = load_texture("WIZBG.PNG");
  if (!assets->background)
    return 0;
  for (i = 0; i < 5; ++i) {
    assets->sprites[i] = load_texture(g_sprite_names[i]);
    assets->yellow[i] = load_texture(g_yellow_names[i]);
    assets->red[i] = load_texture(g_red_names[i]);
    assets->mana[i] = load_texture(g_mana_names[i]);
  }
  for (i = 0; i < 7; ++i)
    assets->skulls[i] = load_texture(g_skull_names[i]);
  return 1;
}

static void render_texture_rect(SDL_Texture *texture, int origin_x, int origin_y,
                                const RectI *rect)
{
  SDL_FRect destination;
  if (!texture)
    return;
  destination.x = (float)(origin_x + rect->x);
  destination.y = (float)(origin_y + rect->y);
  destination.w = (float)rect->w;
  destination.h = (float)rect->h;
  SDL_RenderTexture(g_renderer, texture, NULL, &destination);
}

static int skull_index(int wins)
{
  if (wins <= 0) return -1;
  if (wins <= 4) return wins - 1;
  if (wins <= 7) return 4;
  if (wins <= 11) return 5;
  return 6;
}

static void render_stats_layers(StatsAssets *assets, const StatWinData *data,
                                int origin_x, int origin_y)
{
  int color;
  for (color = 0; color < 5; ++color) {
    int count;
    int pip;
    int hidden;
    int skull;
    SDL_FRect source;
    SDL_FRect destination;
    const RectI *rect;

    if (data->progress_by_color[color] == 0)
      continue;

    count = data->town_count_by_color[color] & 0x1f;
    for (pip = 0; pip < 5 && pip < count; ++pip) {
      rect = &g_mana_sources[color][pip];
      source.x = (float)rect->x;
      source.y = (float)rect->y;
      source.w = (float)rect->w;
      source.h = (float)rect->h;
      destination.x = (float)(origin_x + g_mana_rects[color].x + rect->x);
      destination.y = (float)(origin_y + g_mana_rects[color].y + rect->y);
      destination.w = source.w;
      destination.h = source.h;
      SDL_RenderTexture(g_renderer, assets->mana[color], &source, &destination);
    }

    render_texture_rect(assets->sprites[color], origin_x, origin_y, &g_sprite_rects[color]);
    render_texture_rect(assets->yellow[color], origin_x, origin_y, &g_progress_rects[color]);

    hidden = g_progress_rects[color].h -
             (data->progress_by_color[color] * g_progress_rects[color].h) / 30;
    if (hidden < 0) hidden = 0;
    if (hidden > g_progress_rects[color].h) hidden = g_progress_rects[color].h;
    source.x = 0.0f;
    source.y = (float)hidden;
    source.w = (float)g_progress_rects[color].w;
    source.h = (float)(g_progress_rects[color].h - hidden);
    destination.x = (float)(origin_x + g_progress_rects[color].x);
    destination.y = (float)(origin_y + g_progress_rects[color].y + hidden);
    destination.w = source.w;
    destination.h = source.h;
    if (source.h > 0)
      SDL_RenderTexture(g_renderer, assets->red[color], &source, &destination);

    skull = skull_index(data->duel_wins_by_color[color]);
    if (skull >= 0) {
      source.x = (float)g_skull_rects[color].x;
      source.y = 0.0f;
      source.w = (float)g_skull_rects[color].w;
      source.h = (float)g_skull_rects[color].h;
      destination.x = (float)(origin_x + g_skull_rects[color].x);
      destination.y = (float)(origin_y + g_skull_rects[color].y);
      destination.w = source.w;
      destination.h = source.h;
      SDL_RenderTexture(g_renderer, assets->skulls[skull], &source, &destination);
    }
  }
}

static void scene_origin(const Scene *scene, int *x, int *y)
{
  int logical_w = 0;
  int logical_h = 0;
  SDL_RendererLogicalPresentation mode;
  float width = 640.0f;
  float height = 480.0f;
  if (!SDL_GetRenderLogicalPresentation(g_renderer, &logical_w, &logical_h, &mode) ||
      logical_w <= 0 || logical_h <= 0)
    SDL_GetCurrentRenderOutputSize(g_renderer, &logical_w, &logical_h);
  if (scene->background)
    SDL_GetTextureSize(scene->background, &width, &height);
  *x = (logical_w - (int)width) / 2;
  *y = (logical_h - (int)height) / 2;
}

static void render_scene(const Scene *scene)
{
  int origin_x;
  int origin_y;
  SDL_FRect destination;
  float width;
  float height;
  scene_origin(scene, &origin_x, &origin_y);
  SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
  SDL_RenderClear(g_renderer);
  if (scene->background) {
    SDL_GetTextureSize(scene->background, &width, &height);
    destination.x = (float)origin_x;
    destination.y = (float)origin_y;
    destination.w = width;
    destination.h = height;
    SDL_RenderTexture(g_renderer, scene->background, NULL, &destination);
  }
  if (scene->stats && scene->data)
    render_stats_layers(scene->stats, scene->data, origin_x, origin_y);
  if (scene->animation) {
    SDL_GetTextureSize(scene->animation, &width, &height);
    destination.x = (float)scene->animation_x;
    destination.y = (float)scene->animation_y;
    if (scene->animation_relative_to_background) {
      destination.x += (float)origin_x;
      destination.y += (float)origin_y;
    }
    destination.w = width;
    destination.h = height;
    SDL_RenderTexture(g_renderer, scene->animation, NULL, &destination);
  }
  SDL_RenderPresent(g_renderer);
}

static int poll_dismiss_event(void)
{
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
      SDL_PushEvent(&event);
      return 1;
    }
    if (event.type == SDL_EVENT_KEY_DOWN ||
        event.type == SDL_EVENT_MOUSE_BUTTON_UP)
      return 1;
  }
  return 0;
}

static int play_sidecar_audio(const char *animation_name)
{
  char name[260];
  char path[1024];
  char *extension;
  Sound sound;
  SDL_snprintf(name, sizeof(name), "%s", animation_name);
  extension = strrchr(name, '.');
  if (!extension)
    return 0;
  SDL_strlcpy(extension, ".WAV", sizeof(name) - (size_t)(extension - name));
  SDL_snprintf(path, sizeof(path), "%s/%s", STATWIN_ASSET_DIR, name);
  SDL_zero(sound);
  sound.volume = 400;
  sound.flags = 4;
  return PlaySndFile(path, STATWIN_SOUND_SLOT, &sound) == 0;
}

static void stop_sidecar_audio(int playing)
{
  if (playing) {
    StopSnd(STATWIN_SOUND_SLOT);
    UnloadSnd(STATWIN_SOUND_SLOT);
  }
}

static int play_status_audio(void)
{
  Sound sound;
  SDL_zero(sound);
  sound.volume = 400;
  sound.flags = 5;
  return PlaySndFile("STATSCRN.WAV", STATWIN_STATUS_SOUND_SLOT, &sound) == 0;
}

static void stop_status_audio(int playing)
{
  if (playing) {
    StopSnd(STATWIN_STATUS_SOUND_SLOT);
    UnloadSnd(STATWIN_STATUS_SOUND_SLOT);
  }
}

static int run_animation(const char *name, Scene *scene)
{
  SDL_IOStream *io;
  IMG_AnimationDecoder *decoder;
  SDL_Surface *frame = NULL;
  SDL_Texture *frame_texture = NULL;
  Uint64 duration = 0;
  Uint64 deadline = 0;
  int dismissed = 0;
  int finished = 0;
  int audio_playing;

  if (!g_renderer)
    return 0;
  io = open_asset_io(name);
  if (!io)
    return 0;
  decoder = IMG_CreateAnimationDecoder_IO(io, true, "WEBP");
  if (!decoder) {
    SDL_Log("statwin: could not decode %s: %s", name, SDL_GetError());
    return 0;
  }
  audio_playing = play_sidecar_audio(name);
  while (!dismissed) {
    Uint64 now = SDL_GetTicks();
    if (!finished && (!frame_texture || now >= deadline)) {
      SDL_Surface *next_frame = NULL;
      Uint64 next_duration = 0;
      if (!IMG_GetAnimationDecoderFrame(decoder, &next_frame, &next_duration)) {
        finished = 1;
      } else {
        SDL_DestroyTexture(frame_texture);
        SDL_DestroySurface(frame);
        frame = next_frame;
        duration = next_duration;
        frame_texture = SDL_CreateTextureFromSurface(g_renderer, frame);
        if (!frame_texture)
          break;
        SDL_SetTextureBlendMode(frame_texture, SDL_BLENDMODE_BLEND);
        scene->animation = frame_texture;
        deadline = now + (duration ? duration : 67);
      }
    }
    render_scene(scene);
    dismissed = poll_dismiss_event();
    SDL_Delay(1);
  }
  scene->animation = NULL;
  stop_sidecar_audio(audio_playing);
  SDL_DestroyTexture(frame_texture);
  SDL_DestroySurface(frame);
  IMG_CloseAnimationDecoder(decoder);
  return dismissed ? 2 : 1;
}

static void wait_on_scene(Scene *scene)
{
  while (!poll_dismiss_event()) {
    render_scene(scene);
    SDL_Delay(1);
  }
}

void statwin_set_renderer(SDL_Renderer *renderer)
{
  g_renderer = renderer;
}

int statwin_init(const StatWinData *data)
{
  if (!data)
    return 1;
  g_cached_data = *data;
  g_has_cached_data = 1;
  return 0;
}

static void prefetch_animation_and_audio(const char *name)
{
  char path[512];
  char *extension;
  if (!name || !name[0])
    return;
  SDL_snprintf(path, sizeof(path), "%s/%s", STATWIN_ASSET_DIR, name);
  shandalar_assets_prefetch(path);
  extension = SDL_strrchr(path, '.');
  if (!extension)
    return;
  SDL_strlcpy(extension, ".WAV", sizeof(path) - (size_t)(extension - path));
  shandalar_assets_prefetch(path);
}

void statwin_prefetch_wizard_defeat_media(int wizard_color)
{
  unsigned int wizard_index;
  if (wizard_color < 1 || wizard_color > 5)
    return;
  wizard_index = (unsigned int)(wizard_color - 1);
  prefetch_animation_and_audio(
      g_missing_animations[g_defeated_wizard_colors[wizard_index]]);
}

void statwin_prefetch_arzakon_defeat_media(void)
{
  prefetch_animation_and_audio("Mtgend.WEBP");
}

void statwin_show(const StatWinData *data, int mode)
{
  StatsAssets assets;
  Scene scene;
  unsigned int color;
  int status_audio_playing = 0;
  if (!g_renderer || (!data && !g_has_cached_data))
    return;
  if (data) {
    g_cached_data = *data;
    g_has_cached_data = 1;
  }
  if (!load_stats_assets(&assets))
    return;
  SDL_zero(scene);
  scene.background = assets.background;
  scene.stats = &assets;
  scene.data = &g_cached_data;

  color = g_cached_data.highlighted_wizard_color;
  if (color > 4)
    color = 0;

  if (!(mode == 1 && g_cached_data.progress_by_color[color] == 0))
    status_audio_playing = play_status_audio();

  if (mode == 1 && g_cached_data.progress_by_color[color] == 0) {
    SDL_Texture *room = load_texture(g_missing_backgrounds[color]);
    if (room) {
      scene.background = room;
      scene.stats = NULL;
      scene.animation_x = g_missing_animation_rects[color].x;
      scene.animation_y = g_missing_animation_rects[color].y;
      scene.animation_relative_to_background = 1;
      if (run_animation(g_missing_animations[color], &scene) == 1)
        wait_on_scene(&scene);
      SDL_DestroyTexture(room);
    }
  }
  else if (mode == 2 && g_cached_data.highlighted_creature_type < 56) {
    scene.animation_x = 224;
    scene.animation_y = 280;
    scene.animation_relative_to_background = 1;
    if (run_animation(g_creature_animations[g_cached_data.highlighted_creature_type], &scene) == 1 &&
        g_cached_data.progress_by_color[color] != 0) {
      scene.animation_x = g_hit_rects[color].x;
      scene.animation_y = g_hit_rects[color].y;
      run_animation(g_hit_animations[color], &scene);
    }
    wait_on_scene(&scene);
  }
  else {
    wait_on_scene(&scene);
  }
  stop_status_audio(status_audio_playing);
  destroy_stats_assets(&assets);
}

int statwin_play_video(const char *path, int x, int y, int flags)
{
  char name[260];
  char *base;
  char *extension;
  Scene scene;
  int result;
  (void)flags;
  if (!path || !g_renderer)
    return 1;
  base = strrchr(path, '\\');
  if (!base)
    base = strrchr(path, '/');
  base = base ? base + 1 : (char *)path;
  SDL_snprintf(name, sizeof(name), "%s", base);
  extension = strrchr(name, '.');
  if (extension)
    SDL_strlcpy(extension, ".WEBP", sizeof(name) - (size_t)(extension - name));
  else
    SDL_strlcat(name, ".WEBP", sizeof(name));
  SDL_zero(scene);
  scene.animation_x = x;
  scene.animation_y = y;
  result = run_animation(name, &scene);
  return result ? 0 : 4;
}

void statwin_shutdown(void)
{
  g_has_cached_data = 0;
  SDL_zero(g_cached_data);
}
