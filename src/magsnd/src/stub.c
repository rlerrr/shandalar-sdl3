#include "magsnd.h"

static HWND g_snd_hwnd;

undefined4 __cdecl InitSnd(HWND hwnd, undefined4 unused, uint flags)
{
  (void)unused;
  (void)flags;
  g_snd_hwnd = hwnd;
  return 0;
}

void __cdecl ReleaseSnd(void) { g_snd_hwnd = (HWND)0; }

int __cdecl LoadSnd(LPSTR path, int num, Sound *snd)
{
  (void)path;
  (void)num;
  (void)snd;
  return 0;
}

undefined4 __cdecl UnloadSnd(int num) { (void)num; return 0; }
undefined4 __cdecl UnloadAllSnds(void) { return 0; }
undefined4 __cdecl PlaySnd(int num, Sound *snd) { (void)num; (void)snd; return 0; }
int __cdecl PlaySndFile(LPSTR path, int num, Sound *snd) { (void)path; (void)num; (void)snd; return 0; }
undefined4 __cdecl StopSnd(int num) { (void)num; return 0; }
void __cdecl StopAllSnds(void) {}
undefined4 __cdecl PlayMidiFile(void) { return 0; }
undefined4 __cdecl SetPitch(int num, undefined4 pitch) { (void)num; (void)pitch; return 0; }
undefined4 __cdecl GetPitch(void) { return 0; }
undefined4 __cdecl SetVol(int num, uint vol) { (void)num; (void)vol; return 0; }
undefined4 __cdecl GetVol(void) { return 0; }
undefined4 __cdecl SetPan(int num, int pan) { (void)num; (void)pan; return 0; }
undefined4 __cdecl GetPan(void) { return 0; }
undefined4 __cdecl UpdateSnd(void) { return 0; }
int __cdecl SetSndMarker(int num, uint marker) { (void)num; (void)marker; return 0; }
int __cdecl PlaySndMarker(int num, uint marker) { (void)num; (void)marker; return 0; }

undefined4 __cdecl GetSndTime(int num, uint *out_time)
{
  (void)num;
  if (out_time)
    *out_time = 0;
  return 0;
}

undefined4 __cdecl ResetSnd(void) { return 0; }

undefined4 __cdecl GetSndState(int num, undefined4 *out_state)
{
  (void)num;
  if (out_state)
    *out_state = 0;
  return 0;
}

void *__cdecl GetAVISndBuff(int slot, uint blockNumber)
{
  (void)slot;
  (void)blockNumber;
  return 0;
}

undefined4 __cdecl ReleaseAVISndBuff(int slot) { (void)slot; return 0; }
HWND __cdecl GetSndHWND(void) { return g_snd_hwnd; }

int __cdecl IsSndLoaded(int loadId, int *out_slot)
{
  (void)loadId;
  if (out_slot)
    *out_slot = -1;
  return 0;
}

undefined4 __cdecl GetLRUSnd(int *out_num, int start, int end)
{
  (void)end;
  if (out_num)
    *out_num = start;
  return 0;
}
