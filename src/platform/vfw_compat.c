#include "platform/winuser_compat.h"

#include "platform/asset_manager.h"
#include "platform/file_io.h"

#include <SDL3_image/SDL_image.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MCIWNDM_SETPALETTE (WM_USER + 127)
#define MCIWNDM_PLAY 0x806
#define MCIWNDM_STOP 0x808
#define MCI_FRAME_TIMER 0x4d43
#define STATWIN_ASSET_DIR "statwin_sdl"

typedef struct MciWindowState {
  struct MciWindowState *next;
  HWND window;
  IMG_AnimationDecoder *decoder;
  SDL_Surface *frame;
  SDL_AudioStream *audio_stream;
  SDL_AudioSpec audio_spec;
  Uint8 *audio_buffer;
  Uint32 audio_length;
  uint64_t deadline;
  int playing;
  int finished;
} MciWindowState;

static MciWindowState *g_mci_windows;

static MciWindowState *find_mci_window(HWND window)
{
  MciWindowState *state;
  for (state = g_mci_windows; state; state = state->next) {
    if (state->window == window)
      return state;
  }
  return NULL;
}

static void unlink_mci_window(MciWindowState *state)
{
  MciWindowState **cursor = &g_mci_windows;
  while (*cursor) {
    if (*cursor == state) {
      *cursor = state->next;
      return;
    }
    cursor = &(*cursor)->next;
  }
}

static void paint_mci_frame(HWND window, MciWindowState *state)
{
  SDL_Surface *converted;
  HDC dc;
  if (!state || !state->frame)
    return;
  converted = SDL_ConvertSurface(state->frame, SDL_PIXELFORMAT_ARGB8888);
  if (!converted)
    return;
  dc = GetDC(window);
  shandalar_dc_blit_argb8888(dc, (const uint32_t *)converted->pixels,
                             converted->pitch, converted->w, converted->h);
  ReleaseDC(window, dc);
  SDL_DestroySurface(converted);
}

static void stop_mci_audio(MciWindowState *state)
{
  if (state && state->audio_stream) {
    SDL_DestroyAudioStream(state->audio_stream);
    state->audio_stream = NULL;
  }
}

static void play_mci_audio(MciWindowState *state)
{
  if (!state || !state->audio_buffer || state->audio_length == 0)
    return;
  stop_mci_audio(state);
  state->audio_stream =
      SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                &state->audio_spec, NULL, NULL);
  if (!state->audio_stream)
    return;
  if (!SDL_PutAudioStreamData(state->audio_stream, state->audio_buffer,
                              (int)state->audio_length) ||
      !SDL_FlushAudioStream(state->audio_stream) ||
      !SDL_ResumeAudioStreamDevice(state->audio_stream))
    stop_mci_audio(state);
}

static int advance_mci_frame(HWND window, MciWindowState *state, uint64_t now)
{
  SDL_Surface *frame = NULL;
  uint64_t duration = 0;
  if (!state || state->finished)
    return 0;
  if (!IMG_GetAnimationDecoderFrame(state->decoder, &frame, &duration)) {
    state->finished = 1;
    state->playing = 0;
    KillTimer(window, MCI_FRAME_TIMER);
    return 0;
  }
  SDL_DestroySurface(state->frame);
  state->frame = frame;
  state->deadline = now + (duration ? duration : 67);
  paint_mci_frame(window, state);
  return 1;
}

static LRESULT CALLBACK mci_window_proc(HWND window, UINT message,
                                        WPARAM wparam, LPARAM lparam)
{
  MciWindowState *state = find_mci_window(window);
  switch (message) {
  case MCIWNDM_SETPALETTE:
    if (state) {
      HDC dc = GetDC(window);
      SelectPalette(dc, (HPALETTE)wparam, FALSE);
      ReleaseDC(window, dc);
      paint_mci_frame(window, state);
    }
    return 0;
  case MCIWNDM_PLAY:
    if (state && !state->finished) {
      state->playing = 1;
      play_mci_audio(state);
      SetTimer(window, MCI_FRAME_TIMER, 10, NULL);
    }
    return 0;
  case MCIWNDM_STOP:
    if (state) {
      state->playing = 0;
      stop_mci_audio(state);
      KillTimer(window, MCI_FRAME_TIMER);
    }
    return 0;
  case WM_TIMER:
    if (state && wparam == MCI_FRAME_TIMER && state->playing &&
        SDL_GetTicks() >= state->deadline)
      advance_mci_frame(window, state, SDL_GetTicks());
    return 0;
  case WM_PAINT:
    {
      PAINTSTRUCT paint;
      BeginPaint(window, &paint);
      paint_mci_frame(window, state);
      EndPaint(window, &paint);
    }
    return 0;
  case WM_LBUTTONDOWN:
  case WM_RBUTTONDOWN:
    SendMessageA(GetParent(window), WM_PARENTNOTIFY, message, lparam);
    return 0;
  case WM_DESTROY:
    if (state) {
      KillTimer(window, MCI_FRAME_TIMER);
      stop_mci_audio(state);
      SDL_free(state->audio_buffer);
      state->audio_buffer = NULL;
      unlink_mci_window(state);
      SDL_DestroySurface(state->frame);
      IMG_CloseAnimationDecoder(state->decoder);
      free(state);
    }
    return 0;
  default:
    return DefWindowProcA(window, message, wparam, lparam);
  }
}

HWND MCIWndCreateA(HWND parent, HINSTANCE instance, DWORD style,
                   LPCSTR filename)
{
  static int registered;
  char asset_name[260];
  char asset_path[1024];
  char audio_name[260];
  char audio_path[1024];
  char *base;
  char *extension;
  SDL_IOStream *io;
  MciWindowState *state;
  uint64_t duration = 0;
  HWND window;
  WNDCLASSA window_class;

  if (!registered) {
    memset(&window_class, 0, sizeof(window_class));
    window_class.lpfnWndProc = mci_window_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = "ShandalarMCIWindow";
    if (!RegisterClassA(&window_class))
      return NULL;
    registered = 1;
  }
  if (!filename)
    return NULL;

  base = strrchr(filename, '\\');
  if (!base)
    base = strrchr(filename, '/');
  base = base ? base + 1 : (char *)filename;
  SDL_snprintf(asset_name, sizeof(asset_name), "%s/%s",
               STATWIN_ASSET_DIR, base);
  extension = strrchr(asset_name, '.');
  if (extension)
    SDL_strlcpy(extension, ".webp",
                sizeof(asset_name) - (size_t)(extension - asset_name));
  else
    SDL_strlcat(asset_name, ".webp", sizeof(asset_name));
  for (char *p = asset_name; *p; ++p)
    *p = (char)tolower((unsigned char)*p);

  if (shandalar_assets_fetch_blocking(asset_name) != SHANDALAR_ASSET_READY ||
      !shandalar_assets_join_path(asset_path, sizeof(asset_path), asset_name))
    return NULL;
  io = shandalar_open_file_io(asset_path, "rb");
  if (!io)
    return NULL;
  state = (MciWindowState *)calloc(1, sizeof(*state));
  if (!state) {
    SDL_CloseIO(io);
    return NULL;
  }
  state->decoder = IMG_CreateAnimationDecoder_IO(io, true, "WEBP");
  if (!state->decoder ||
      !IMG_GetAnimationDecoderFrame(state->decoder, &state->frame, &duration)) {
    if (state->decoder)
      IMG_CloseAnimationDecoder(state->decoder);
    free(state);
    return NULL;
  }
  state->deadline = SDL_GetTicks() + (duration ? duration : 67);
  SDL_strlcpy(audio_name, asset_name, sizeof(audio_name));
  extension = strrchr(audio_name, '.');
  if (extension)
    SDL_strlcpy(extension, ".wav",
                sizeof(audio_name) - (size_t)(extension - audio_name));
  else
    SDL_strlcat(audio_name, ".wav", sizeof(audio_name));
  if (shandalar_assets_fetch_blocking(audio_name) == SHANDALAR_ASSET_READY &&
      shandalar_assets_join_path(audio_path, sizeof(audio_path), audio_name)) {
    io = shandalar_open_file_io(audio_path, "rb");
    if (io)
      SDL_LoadWAV_IO(io, true, &state->audio_spec, &state->audio_buffer,
                     &state->audio_length);
  }

  window = CreateWindowExA(0, "ShandalarMCIWindow", "", style,
                           0, 0, state->frame->w, state->frame->h,
                           parent, NULL, instance, NULL);
  if (!window) {
    SDL_DestroySurface(state->frame);
    SDL_free(state->audio_buffer);
    IMG_CloseAnimationDecoder(state->decoder);
    free(state);
    return NULL;
  }
  state->window = window;
  state->next = g_mci_windows;
  g_mci_windows = state;
  paint_mci_frame(window, state);
  return window;
}
