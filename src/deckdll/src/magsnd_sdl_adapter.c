#include "magsnd.h"

/*
 * The original deck DLL loaded magsnd.dll by ordinal.  The SDL port links the
 * same ABI as an ordinary static library, so keep that distinction out of the
 * retained deck code and adapt its small wrapper API here.
 */
extern int __cdecl InitSnd(HWND hwnd, int unused, unsigned int flags);
extern void __cdecl ReleaseSnd(void);
extern int __cdecl LoadSnd(char *path, int num, Sound *snd);
extern int __cdecl UnloadSnd(int num);
extern int __cdecl UnloadAllSnds(void);
extern int __cdecl PlaySnd(int num, Sound *snd);
extern int __cdecl StopSnd(int num);
extern int __cdecl SetSndMarker(int num, unsigned int marker);
extern int __cdecl UpdateSnd(void);
extern int __cdecl IsSndLoaded(int load_id, int *out_slot);
extern int __cdecl GetLRUSnd(int *out_num, int start, int end);

int init_sound_dll(HWND hwnd, int unused, int flags)
{
  return InitSnd(hwnd, unused, (unsigned int)flags);
}

void clear_sound_imports_table(void) {}

void sound_close(void)
{
  ReleaseSnd();
}

int sound_load(const char *path, int num, Sound *snd)
{
  return LoadSnd((char *)path, num, snd);
}

int sound_unload(int num)
{
  return UnloadSnd(num);
}

int sound_unload_all(void)
{
  return UnloadAllSnds();
}

int sound_play(int num, Sound *snd)
{
  return PlaySnd(num, snd);
}

int sound_stop(int num)
{
  return StopSnd(num);
}

int set_sound_loop(int num, int marker)
{
  return SetSndMarker(num, (unsigned int)marker);
}

int sound_is_loaded(int load_id, int *out_slot)
{
  return IsSndLoaded(load_id, out_slot);
}

int sound_get_lru(int *out_num, int start, int end)
{
  return GetLRUSnd(out_num, start, end);
}

int update_snd(void)
{
  return UpdateSnd();
}
