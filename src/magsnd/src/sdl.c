#include "magsnd.h"

#include "platform/asset_manager.h"
#include "platform/file_io.h"

#include <SDL3/SDL.h>
#include <stddef.h>
#include <stdint.h>

#define MAGSND_MAX_SLOTS 0x110
#define MAGSND_OK 0
#define MAGSND_NOT_LOADED 1
#define MAGSND_ALREADY_LOADED 2
#define MAGSND_UNINITIALIZED 4
#define MAGSND_BAD_ARGUMENT 5
#define MAGSND_PLAYBACK_ERROR 9

typedef struct SdlSndSlot {
  SDL_AudioSpec spec;
  Uint8 *buffer;
  Uint32 length;
  Uint32 cursor;
  SDL_AudioStream *stream;
  int loaded;
  int playing;
  int loop;
  int load_id;
  int volume;
  int sample_rate;
  int pan;
  Uint64 play_started_ms;
  uint lru_counter;
} SdlSndSlot;

static HWND g_snd_hwnd;
static int g_initialized;
static uint g_lru_clock;
static SdlSndSlot g_slots[MAGSND_MAX_SLOTS];

static int valid_slot(int num)
{
  return num >= 0 && num < MAGSND_MAX_SLOTS;
}

static int is_absolute_path(const char *path)
{
  if (!path || !path[0])
    return 0;
  if (path[0] == '/' || path[0] == '\\')
    return 1;
  return path[0] && path[1] == ':' && (path[2] == '/' || path[2] == '\\');
}

static const char *asset_relative_sound_path(const char *path)
{
  if (!path)
    return path;
  if (path[0] && path[1] == ':' && path[2] != '/' && path[2] != '\\')
    return path + 2;
  if ((path[0] == 'x' || path[0] == 'X') && path[1] == ':' &&
      (path[2] == '/' || path[2] == '\\'))
    return path + 3;
  return path;
}

static void normalize_slashes(char *path)
{
  while (path && *path) {
    if (*path == '\\')
      *path = '/';
    ++path;
  }
}

/* DuelSounds are tiny (tens of KB).  Cap what we'll hand to the WAV parser so a
 * corrupt/truncated file (e.g. an HTML error page cached as .wav, or a
 * truncated fetch) can't coax SDL into a gigabyte SDL_malloc.  On Emscripten
 * the default build aborts with "memory access out of bounds" inside malloc
 * instead of returning NULL, which is the crash in question. */
#define MAGSND_MAX_WAV_BYTES (4 * 1024 * 1024)

static SDL_IOStream *open_sound_io(const char *path)
{
  const char *asset_path;
  char resolved[1024];

  if (!path || !path[0])
    return NULL;
  asset_path = asset_relative_sound_path(path);
  if (is_absolute_path(asset_path)) {
    SDL_strlcpy(resolved, asset_path, sizeof(resolved));
  } else if (!shandalar_assets_join_path(resolved, sizeof(resolved),
                                         asset_path)) {
    return NULL;
  }
  normalize_slashes(resolved);
  return shandalar_open_file_io(resolved, "rb");
}

static Sint64 sound_io_size(SDL_IOStream *io)
{
  Sint64 size = SDL_GetIOSize(io);
  if (size >= 0)
    return size;
  /* No size callback (custom FILE* wrapper): seek to end and back. */
  {
    Sint64 pos = SDL_TellIO(io);
    Sint64 end;
    if (pos < 0)
      return -1;
    end = SDL_SeekIO(io, 0, SDL_IO_SEEK_END);
    if (end < 0)
      return -1;
    if (SDL_SeekIO(io, pos, SDL_IO_SEEK_SET) != pos)
      return -1;
    return end;
  }
}

/* Returns 1 if the stream looks like a plausible WAV of a sane size, with the
 * stream rewound to position 0.  Returns 0 otherwise. */
static int sound_header_is_plausible(SDL_IOStream *io, Sint64 file_size)
{
  Uint8 header[12];
  size_t got;
  Uint32 riff_len;

  if (file_size < 44 || file_size > MAGSND_MAX_WAV_BYTES)
    return 0;
  if (SDL_SeekIO(io, 0, SDL_IO_SEEK_SET) != 0)
    return 0;
  got = SDL_ReadIO(io, header, sizeof(header));
  if (got != sizeof(header))
    return 0;
  if (SDL_SeekIO(io, 0, SDL_IO_SEEK_SET) != 0)
    return 0;
  if (SDL_memcmp(header, "RIFF", 4) == 0) {
    if (SDL_memcmp(header + 8, "WAVE", 4) != 0)
      return 0;
    riff_len = (Uint32)header[4] | ((Uint32)header[5] << 8) |
               ((Uint32)header[6] << 16) | ((Uint32)header[7] << 24);
    /* 0 means "unknown length" to some writers; otherwise it must roughly
     * match the file size and stay under the cap. */
    if (riff_len != 0 &&
        (riff_len > (Uint32)(file_size + 8) || riff_len > MAGSND_MAX_WAV_BYTES))
      return 0;
    return 1;
  }
  /* SDL also accepts RIFF-less files starting with "WAVE". */
  if (SDL_memcmp(header, "WAVE", 4) == 0)
    return 1;
  return 0;
}

static void free_slot(SdlSndSlot *slot)
{
  if (!slot)
    return;
  if (slot->stream)
    SDL_DestroyAudioStream(slot->stream);
  if (slot->buffer)
    SDL_free(slot->buffer);
  SDL_zero(*slot);
}

static float volume_to_gain(uint volume)
{
  if (volume > 400)
    volume = 400;
  return (float)volume / 400.0f;
}

static float pitch_to_ratio(const SdlSndSlot *slot, int sample_rate)
{
  float ratio;
  if (!slot || sample_rate <= 0 || slot->spec.freq <= 0)
    return 1.0f;
  ratio = (float)sample_rate / (float)slot->spec.freq;
  if (ratio < 0.01f)
    ratio = 0.01f;
  if (ratio > 100.0f)
    ratio = 100.0f;
  return ratio;
}

static void apply_sound_params(SdlSndSlot *slot, const Sound *snd)
{
  int volume = 400;
  int sample_rate;
  int pan = 0;

  if (!slot)
    return;
  sample_rate = slot->spec.freq;
  if (snd) {
    volume = snd->volume;
    sample_rate = snd->sampleRate ? snd->sampleRate : slot->spec.freq;
    pan = snd->pan;
    slot->loop = (snd->flags & 1) != 0 || slot->loop;
  }
  slot->volume = volume;
  slot->sample_rate = sample_rate;
  slot->pan = pan;
  if (slot->stream) {
    SDL_SetAudioStreamGain(slot->stream, volume_to_gain((uint)volume));
    SDL_SetAudioStreamFrequencyRatio(slot->stream, pitch_to_ratio(slot, sample_rate));
  }
}

static void SDLCALL stream_get_callback(void *userdata, SDL_AudioStream *stream,
                                        int additional_amount, int total_amount)
{
  SdlSndSlot *slot = (SdlSndSlot *)userdata;
  (void)total_amount;
  if (!slot || !slot->buffer || slot->length == 0)
    return;

  while (additional_amount > 0 && slot->playing) {
    int remaining = (int)(slot->length - slot->cursor);
    int chunk;
    if (remaining <= 0) {
      if (!slot->loop) {
        slot->playing = 0;
        break;
      }
      slot->cursor = 0;
      remaining = (int)slot->length;
    }
    chunk = remaining < additional_amount ? remaining : additional_amount;
    if (!SDL_PutAudioStreamData(stream, slot->buffer + slot->cursor, chunk)) {
      slot->playing = 0;
      break;
    }
    slot->cursor += (Uint32)chunk;
    additional_amount -= chunk;
  }
}

static int start_slot_stream(SdlSndSlot *slot, const Sound *snd)
{
  if (!slot || !slot->loaded)
    return MAGSND_NOT_LOADED;
  if (slot->stream) {
    SDL_DestroyAudioStream(slot->stream);
    slot->stream = NULL;
  }
  slot->cursor = 0;
  slot->playing = 1;
  slot->stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                           &slot->spec, stream_get_callback, slot);
  if (!slot->stream) {
    slot->playing = 0;
    return MAGSND_PLAYBACK_ERROR;
  }
  apply_sound_params(slot, snd);
  stream_get_callback(slot, slot->stream, 4096, 0);
  if (!SDL_ResumeAudioStreamDevice(slot->stream)) {
    SDL_DestroyAudioStream(slot->stream);
    slot->stream = NULL;
    slot->playing = 0;
    return MAGSND_PLAYBACK_ERROR;
  }
  slot->play_started_ms = SDL_GetTicks();
  slot->lru_counter = ++g_lru_clock;
  return MAGSND_OK;
}

undefined4 __cdecl InitSnd(HWND hwnd, undefined4 unused, uint flags)
{
  (void)unused;
  (void)flags;
  g_snd_hwnd = hwnd;
  if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) && !SDL_InitSubSystem(SDL_INIT_AUDIO))
    return MAGSND_PLAYBACK_ERROR;
  g_initialized = 1;
  return MAGSND_OK;
}

void __cdecl ReleaseSnd(void)
{
  UnloadAllSnds();
  g_snd_hwnd = (HWND)0;
  g_initialized = 0;
}

int __cdecl LoadSnd(LPSTR path, int num, Sound *snd)
{
  SDL_IOStream *io;
  SdlSndSlot *slot;
  (void)snd;

  if (!g_initialized)
    return MAGSND_UNINITIALIZED;
  if (!valid_slot(num))
    return MAGSND_BAD_ARGUMENT;
  slot = &g_slots[num];
  if (slot->loaded)
    return MAGSND_OK;
  if (!path || !path[0])
    return MAGSND_BAD_ARGUMENT;
  io = open_sound_io(path);
  if (!io)
    return MAGSND_NOT_LOADED;
  {
    Sint64 file_size = sound_io_size(io);
    if (!sound_header_is_plausible(io, file_size)) {
      SDL_CloseIO(io);
      SDL_zero(*slot);
      return MAGSND_NOT_LOADED;
    }
  }
  if (!SDL_LoadWAV_IO(io, true, &slot->spec, &slot->buffer, &slot->length)) {
    SDL_zero(*slot);
    return MAGSND_NOT_LOADED;
  }
  if (slot->length == 0 || slot->length > MAGSND_MAX_WAV_BYTES ||
      !slot->buffer) {
    if (slot->buffer)
      SDL_free(slot->buffer);
    SDL_zero(*slot);
    return MAGSND_NOT_LOADED;
  }
  slot->loaded = 1;
  slot->volume = 400;
  slot->sample_rate = slot->spec.freq;
  slot->load_id = snd ? snd->loadId : 0;
  slot->lru_counter = ++g_lru_clock;
  return MAGSND_OK;
}

undefined4 __cdecl UnloadSnd(int num)
{
  if (!valid_slot(num))
    return MAGSND_BAD_ARGUMENT;
  if (!g_slots[num].loaded)
    return MAGSND_NOT_LOADED;
  free_slot(&g_slots[num]);
  return MAGSND_OK;
}

undefined4 __cdecl UnloadAllSnds(void)
{
  int i;
  for (i = 0; i < MAGSND_MAX_SLOTS; ++i) {
    if (g_slots[i].loaded)
      free_slot(&g_slots[i]);
  }
  return MAGSND_OK;
}

undefined4 __cdecl PlaySnd(int num, Sound *snd)
{
  if (!g_initialized)
    return MAGSND_UNINITIALIZED;
  if (!valid_slot(num))
    return MAGSND_BAD_ARGUMENT;
  return start_slot_stream(&g_slots[num], snd);
}

int __cdecl PlaySndFile(LPSTR path, int num, Sound *snd)
{
  int result;
  if (!valid_slot(num))
    return MAGSND_BAD_ARGUMENT;
  if (g_slots[num].loaded)
    return MAGSND_ALREADY_LOADED;
  result = LoadSnd(path, num, snd);
  if (result != MAGSND_OK)
    return result;
  return PlaySnd(num, snd);
}

undefined4 __cdecl StopSnd(int num)
{
  SdlSndSlot *slot;
  if (!valid_slot(num))
    return MAGSND_BAD_ARGUMENT;
  slot = &g_slots[num];
  if (!slot->loaded)
    return MAGSND_NOT_LOADED;
  slot->playing = 0;
  if (slot->stream) {
    SDL_DestroyAudioStream(slot->stream);
    slot->stream = NULL;
  }
  slot->cursor = 0;
  return MAGSND_OK;
}

void __cdecl StopAllSnds(void)
{
  int i;
  for (i = 0; i < MAGSND_MAX_SLOTS; ++i) {
    if (g_slots[i].loaded)
      StopSnd(i);
  }
}

undefined4 __cdecl PlayMidiFile(void) { return MAGSND_OK; }

undefined4 __cdecl SetPitch(int num, undefined4 pitch)
{
  SdlSndSlot *slot;
  if (!valid_slot(num))
    return MAGSND_BAD_ARGUMENT;
  slot = &g_slots[num];
  if (!slot->loaded)
    return MAGSND_NOT_LOADED;
  slot->sample_rate = (int)pitch;
  if (slot->stream)
    SDL_SetAudioStreamFrequencyRatio(slot->stream, pitch_to_ratio(slot, slot->sample_rate));
  return MAGSND_OK;
}

undefined4 __cdecl GetPitch(void) { return MAGSND_OK; }

undefined4 __cdecl SetVol(int num, uint vol)
{
  SdlSndSlot *slot;
  if (!valid_slot(num))
    return MAGSND_BAD_ARGUMENT;
  slot = &g_slots[num];
  if (!slot->loaded)
    return MAGSND_NOT_LOADED;
  slot->volume = (int)vol;
  if (slot->stream)
    SDL_SetAudioStreamGain(slot->stream, volume_to_gain(vol));
  return MAGSND_OK;
}

undefined4 __cdecl GetVol(void) { return MAGSND_OK; }

undefined4 __cdecl SetPan(int num, int pan)
{
  if (!valid_slot(num))
    return MAGSND_BAD_ARGUMENT;
  if (!g_slots[num].loaded)
    return MAGSND_NOT_LOADED;
  g_slots[num].pan = pan;
  return MAGSND_OK;
}

undefined4 __cdecl GetPan(void) { return MAGSND_OK; }

undefined4 __cdecl UpdateSnd(void)
{
  int i;
  for (i = 0; i < MAGSND_MAX_SLOTS; ++i) {
    SdlSndSlot *slot = &g_slots[i];
    if (slot->stream && !slot->playing &&
        SDL_GetAudioStreamQueued(slot->stream) <= 0 &&
        SDL_GetAudioStreamAvailable(slot->stream) <= 0) {
      SDL_DestroyAudioStream(slot->stream);
      slot->stream = NULL;
      slot->cursor = 0;
    }
  }
  return MAGSND_OK;
}

int __cdecl SetSndMarker(int num, uint marker)
{
  if (!valid_slot(num))
    return MAGSND_BAD_ARGUMENT;
  if (!g_slots[num].loaded)
    return MAGSND_NOT_LOADED;
  g_slots[num].loop = marker != 0;
  return MAGSND_OK;
}

int __cdecl PlaySndMarker(int num, uint marker)
{
  (void)marker;
  return PlaySnd(num, NULL);
}

undefined4 __cdecl GetSndTime(int num, uint *out_time)
{
  SdlSndSlot *slot;
  if (!valid_slot(num))
    return MAGSND_BAD_ARGUMENT;
  slot = &g_slots[num];
  if (!slot->loaded)
    return MAGSND_NOT_LOADED;
  if (out_time)
    *out_time = slot->play_started_ms ? (uint)(SDL_GetTicks() - slot->play_started_ms) : 0;
  return MAGSND_OK;
}

undefined4 __cdecl ResetSnd(void) { return MAGSND_OK; }

undefined4 __cdecl GetSndState(int num, undefined4 *out_state)
{
  SdlSndSlot *slot;
  if (!valid_slot(num))
    return MAGSND_BAD_ARGUMENT;
  slot = &g_slots[num];
  if (!slot->loaded)
    return MAGSND_NOT_LOADED;
  if (out_state)
    *out_state = (slot->stream && (slot->playing || SDL_GetAudioStreamQueued(slot->stream) > 0)) ? 1 : 0;
  return MAGSND_OK;
}

void *__cdecl GetAVISndBuff(int slot, uint blockNumber)
{
  (void)slot;
  (void)blockNumber;
  return 0;
}

undefined4 __cdecl ReleaseAVISndBuff(int slot)
{
  (void)slot;
  return MAGSND_OK;
}

HWND __cdecl GetSndHWND(void) { return g_snd_hwnd; }

int __cdecl IsSndLoaded(int loadId, int *out_slot)
{
  int i;
  for (i = 0; i < MAGSND_MAX_SLOTS; ++i) {
    if (g_slots[i].loaded && g_slots[i].load_id == loadId) {
      if (out_slot)
        *out_slot = i;
      return 1;
    }
  }
  /* Leave *out_slot untouched on a miss: play_sound_effect()'s direct-mapped
   * range (e.g. WAV_ASWANJAG..WAV_TOSS) passes sound_num == sound_id in and
   * expects it to still hold the slot number so it can LoadSnd into that
   * slot.  Clobbering it with -1 turned every miss into LoadSnd(-1). */
  return 0;
}

undefined4 __cdecl GetLRUSnd(int *out_num, int start, int end)
{
  int i;
  int best = -1;
  uint best_lru = UINT32_MAX;
  if (!out_num || start < 0 || end < start || end >= MAGSND_MAX_SLOTS)
    return MAGSND_BAD_ARGUMENT;
  for (i = start; i <= end; ++i) {
    if (!g_slots[i].loaded) {
      *out_num = i;
      return MAGSND_OK;
    }
    if (g_slots[i].lru_counter < best_lru) {
      best = i;
      best_lru = g_slots[i].lru_counter;
    }
  }
  *out_num = best >= 0 ? best : start;
  return MAGSND_OK;
}
