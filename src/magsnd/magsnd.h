#ifndef SHANDALAR_MAGSND_H
#define SHANDALAR_MAGSND_H

#include "platform/gdi_compat.h"
#include "inttypes.h"

#if defined(_WIN32) && defined(SHANDALAR_BUILD_DLL)
#define MAGSND_API __declspec(dllexport)
#elif defined(_WIN32) && defined(SHANDALAR_USE_DLL)
#define MAGSND_API __declspec(dllimport)
#else
#define MAGSND_API
#endif

typedef struct Sound_t
{
  int volume; // 0x00
  int sampleRate; // 0x04
  int pan; // 0x08
  int field_C; // 0x0c
  int field_10; // 0x10
  int field_14; // 0x14
  int loadId; // 0x18
  int flags; // 0x1c
} Sound;

MAGSND_API undefined4 __cdecl InitSnd(HWND hwnd, undefined4 unused, uint flags);
MAGSND_API void __cdecl ReleaseSnd(void);
MAGSND_API int __cdecl LoadSnd(LPSTR path, int num, Sound *snd);
MAGSND_API undefined4 __cdecl UnloadSnd(int num);
MAGSND_API undefined4 __cdecl UnloadAllSnds(void);
MAGSND_API undefined4 __cdecl PlaySnd(int num, Sound *snd);
MAGSND_API int __cdecl PlaySndFile(LPSTR path, int num, Sound *snd);
MAGSND_API undefined4 __cdecl StopSnd(int num);
MAGSND_API void __cdecl StopAllSnds(void);
MAGSND_API undefined4 __cdecl PlayMidiFile(void);
MAGSND_API undefined4 __cdecl SetPitch(int num, undefined4 pitch);
MAGSND_API undefined4 __cdecl GetPitch(void);
MAGSND_API undefined4 __cdecl SetVol(int num, uint vol);
MAGSND_API undefined4 __cdecl GetVol(void);
MAGSND_API undefined4 __cdecl SetPan(int num, int pan);
MAGSND_API undefined4 __cdecl GetPan(void);
MAGSND_API undefined4 __cdecl UpdateSnd(void);
MAGSND_API int __cdecl SetSndMarker(int num, uint marker);
MAGSND_API int __cdecl PlaySndMarker(int num, uint marker);
MAGSND_API undefined4 __cdecl GetSndTime(int num, uint *out_time);
MAGSND_API undefined4 __cdecl ResetSnd(void);
MAGSND_API undefined4 __cdecl GetSndState(int num, undefined4 *out_state);
MAGSND_API void * __cdecl GetAVISndBuff(int slot, uint blockNumber);
MAGSND_API undefined4 __cdecl ReleaseAVISndBuff(int slot);
MAGSND_API HWND __cdecl GetSndHWND(void);
MAGSND_API int __cdecl IsSndLoaded(int loadId, int *out_slot);
MAGSND_API undefined4 __cdecl GetLRUSnd(int *out_num, int start, int end);

#undef MAGSND_API

#endif
