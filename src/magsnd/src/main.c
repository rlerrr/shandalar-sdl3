#include "magsnd.h"
#include "defs.h"
#include "mystdbool.h"
#include <DSOUND.H>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mmsystem.h>
#include <vfw.h>

#pragma intrinsic(memcpy)

typedef unsigned char byte;
typedef int(__stdcall code)();

typedef struct PaddedWaveFormat_t
{
  WAVEFORMATEX fmt;
  byte pad[2];
} PaddedWaveFormat;

typedef struct SndInstance SndInstance;
typedef struct SndDuplicateBufferEntry SndDuplicateBufferEntry;

// Why on earth is this a thing?
#define MAGSND_DELETE(toDelete) \
  do                            \
  {                             \
    void *tmp1 = toDelete;      \
    void *tmp2 = tmp1;          \
    operator_delete(tmp2);      \
  } while (false);

struct SndDuplicateBufferEntry
{
  LPDIRECTSOUNDBUFFER buffer; /* 0x000 */
  int reserved1;              /* 0x004 */
  int reserved2;              /* 0x008 */
};

struct SndInstance
{
  void *streamOrMmio;         /* 0x000: PAVISTREAM for AVI path, otherwise used as opaque */
  uint flags;                 /* 0x004 */
  uint flags2;                /* 0x008 */
  uint lruCounter;            /* 0x00c */
  int slotIndex;              /* 0x010 */
  int loadId;                 /* 0x014 */
  int activeMarkerIndex;      /* 0x018 */
  MMCKINFO mmck;              /* 0x01c */
  uint markerCount;           /* 0x030 */
  SndInstance *markers[0x11]; /* 0x034 */
  WAVEFORMATEX waveFmt;       /* 0x078 (18 bytes) */
  unsigned short pad_08a;     /* 0x08a */
  uint blockBytes;            /* 0x08c */
  int blockScale;             /* 0x090 */
  int blockCount;             /* 0x094 */
  byte pad_098[0xa0 - 0x98];
  uint timeEventCount;                            /* 0x0a0 */
  void *dsLockedPtr;                              /* 0x0a4: pointer from IDirectSoundBuffer::Lock */
  DSBUFFERDESC dsDesc;                            /* 0x0a8 (DSBUFFERDESC, size 0x14) */
  LPDIRECTSOUNDBUFFER dsBuffer;                   /* 0x0bc: LPDIRECTSOUNDBUFFER */
  SndDuplicateBufferEntry duplicateBuffers[0x10]; /* 0x0c0 - 0x180 */
  MMIOINFO mmioInfo;                              /* 0x180 */
  HMMIO hMmio;                                    /* 0x1c8 */
  int dataBytes;                                  /* 0x1cc */
  uint writeCursorBytes;                          /* 0x1d0: bytes written/consumed from stream */
  uint readCursorBytes;                           /* 0x1d4 */
  uint ringCursorBytes;                           /* 0x1d8 */
  uint timeBytes;                                 /* 0x1dc */
  int mmioBaseOffset;                             /* 0x1e0 */
  uint ringCursorSavedBytes;                      /* 0x1e4 */
  int pan;                                        /* 0x1e8 */
  int sampleRate;                                 /* 0x1ec */
  int volume;                                     /* 0x1f0 */
  SndInstance *updatePrev;                        /* 0x1f4: used by AddToUpdateList/RemoveFromUpdateList */
  SndInstance *updateNext;                        /* 0x1f8 */
  SndInstance *activePrev;                        /* 0x1fc: used by AddToActiveList/RemoveFromActiveList */
  SndInstance *activeNext;                        /* 0x200 */
};
#if !(defined(MODERN_FIXES) && defined(__clang__))
STATIC_ASSERT(sizeof(SndInstance) == 0x204, SndInstance_wrong_size);
#endif

void __cdecl ReleaseSnd(void);
int __cdecl LoadSnd(LPSTR path, int slot, Sound *sound);
undefined4 __cdecl UnloadSnd(int slot);
undefined4 __cdecl UnloadAllSnds(void);
undefined4 __cdecl PlaySnd(int slot, Sound *sound);
int __cdecl PlaySndFile(LPSTR filename, int slot, Sound *sound);
undefined4 __cdecl StopSnd(int slot);
void __cdecl StopAllSnds(void);
undefined4 __cdecl PlayMidiFile(void);
undefined4 __cdecl SetPitch(int slot, undefined4 pitchHz);
undefined4 __cdecl GetPitch(void);
undefined4 __cdecl SetVol(int slot, uint volume);
undefined4 __cdecl GetVol(void);
undefined4 __cdecl SetPan(int slot, int pan);
undefined4 __cdecl GetPan(void);
undefined4 __cdecl UpdateSnd(void);
int __cdecl SetSndMarker(int slot, uint markerIndex);
int __cdecl PlaySndMarker(int slot, uint markerIndex);
int __cdecl LoadMarkerInstance(SndInstance *parent, int markerIndex);
int __cdecl PrimeMarkerInstance(SndInstance *parent, int markerIndex);
undefined4 __cdecl GetSndTime(int slot, uint *timeMs);
undefined4 __cdecl ResetSnd(void);
undefined4 __cdecl GetSndState(int slot, undefined4 *outState);
void *__cdecl GetAVISndBuff(int slot, uint bytesRequested);
undefined4 __cdecl ReleaseAVISndBuff(int slot);
int __cdecl IsSndLoaded(int loadId, int *outSlot);
undefined4 __cdecl GetLRUSnd(int *outSlot, int startSlot, int endSlot);

void __cdecl UpdateMmioSnd(SndInstance *snd);
int __cdecl UpdateAviSnd(SndInstance *snd);
void __cdecl AddToUpdateList(SndInstance *snd);
void __cdecl RemoveFromUpdateList(SndInstance *snd);
void __cdecl AddToActiveList(SndInstance *snd);
void __cdecl RemoveFromActiveList(SndInstance *snd);
undefined4 __cdecl AcquireFreeDsBuffer(SndInstance *snd, LPDIRECTSOUNDBUFFER *outBuffer);
int __cdecl PlaySndInstance(SndInstance *snd, Sound *sound);
undefined4 __cdecl StopSndNow(int slot);
int __cdecl ResetAndPrimeSndInstance(SndInstance *snd);
undefined4 __cdecl StartUpdateTimer(undefined4 resolutionMs);
void __cdecl StopUpdateTimer(void);
void CALLBACK UpdateTimerProc(UINT u1, UINT u2, DWORD dw1, DWORD dw2, DWORD dw3);
void __cdecl CloseMmioAndFreeSource(SndInstance *snd);
undefined4 __cdecl LoadFromFile(char *filename, SndInstance **outSnd);
undefined4 __cdecl LoadWaveMmio(LPSTR filename, SndInstance **outSnd, undefined4 scratch);
undefined4 __cdecl PrimeDsBufferFromSource(SndInstance *snd, int startSample);
undefined4 __cdecl PrimeAviAudio(SndInstance *snd, int unused);
undefined4 __cdecl LoadAviAudioStream(void *stream, SndInstance **outSnd, undefined4 scratch);
void __cdecl RefillAviMmioBuffer(SndInstance *sndObj, long streamStartSample);
int __cdecl MmioTellFromInfo(MMIOINFO *info);
undefined4 __cdecl LoadWaveFromFileHandle(FILE *file, SndInstance **outSnd);
undefined4 __cdecl FindRiffChunk(FILE *file, int chunkId, long searchStart, uint searchEnd);
SndInstance *__cdecl CreateSndInstance(IDirectSound *dSound, DWORD dwBufferBytes, WAVEFORMATEX *wfxFormat, DWORD dwFlags);
undefined4 __cdecl DestroySndInstance(SndInstance *snd);
undefined4 __cdecl CleanupAviStream(SndInstance *snd);
void *__cdecl operator_new(unsigned int size);
void __cdecl operator_delete(void *p);

typedef unsigned short ushort;

typedef struct SndTimeSample
{
  uint prevTime;
  uint currTime;
  uint unused08;
  uint unused0c;
} SndTimeSample;

// GLOBAL: MAGSND 0x10006000
double g_sndTimeDiv_88200 = 88.2;

// GLOBAL: MAGSND 0x10006008
double g_sndTimeDiv_44100 = 44.1;

// GLOBAL: MAGSND 0x10006010
double g_sndTimeDiv_22050 = 22.05;

// GLOBAL: MAGSND 0x10006018
double g_sndTimeDiv_11025 = 11.025;

// GLOBAL: MAGSND 0x10007040
DSBUFFERDESC g_primaryDsDesc = {20, 1, 0, 0, NULL};

// GLOBAL: MAGSND 0x10007058
WAVEFORMATEX g_primaryWaveFormat = {1, 2, 22050, 88200, 4, 16, 0};

// GLOBAL: MAGSND 0x10007030
undefined4 g_sndLoadSerial = 0x00000000;

// GLOBAL: MAGSND 0x100070A8
SndInstance *g_sndSlots[0x110];
// GLOBAL: MAGSND 0x1000707C
undefined4 g_oversized_sound_buffer_count = 0x00000000;

// GLOBAL: MAGSND 0x10007088
undefined4 g_mmioInfoMismatchCount = 0x00000000;

// GLOBAL: MAGSND 0x100074E8
SndTimeSample g_sndTimeSamples[0x100];

// GLOBAL: MAGSND 0x10008500
CRITICAL_SECTION g_sndCs;

// GLOBAL: MAGSND 0x10007010
SndInstance *g_activeSndListHead = (SndInstance *)0x0;

// GLOBAL: MAGSND 0x10007014
SndInstance *g_activeSndTail = (SndInstance *)0x0;

// GLOBAL: MAGSND 0x10007018
SndInstance *g_updateSndHead = (SndInstance *)0x0;

// GLOBAL: MAGSND 0x1000701C
SndInstance *g_updateSndTail = (SndInstance *)0x0;

// GLOBAL: MAGSND 0x10007020
undefined4 g_updateTimerEnabled = 0x00000001;

// GLOBAL: MAGSND 0x10007024
undefined4 g_updateTimerActive = 0x00000000;

// GLOBAL: MAGSND 0x10007028
int g_updateTimerUsers = 0x00000000;

// GLOBAL: MAGSND 0x1000702C
undefined4 g_updateResolutionMs = 0x00000019;

// GLOBAL: MAGSND 0x10007034
undefined4 g_sndRefCount = 0x00000000;

// GLOBAL: MAGSND 0x10007038
uint g_lastSndTime = 0x00000000;

// GLOBAL: MAGSND 0x1000706C
undefined4 g_updateSndDisabled = 0x00000000;

// GLOBAL: MAGSND 0x10007074
uint g_sndTimeSampleIndex = 0x00000000;

// GLOBAL: MAGSND 0x10007080
undefined4 g_updatePeriodMs = 0x00000021;

// GLOBAL: MAGSND 0x100070A0
LPDIRECTSOUNDBUFFER g_primaryDsBuffer;

// GLOBAL: MAGSND 0x100084E8
HWND g_sndHwnd = 0x00000000;

// GLOBAL: MAGSND 0x100084F0
IDirectSound *g_directSound = NULL;

// GLOBAL: MAGSND 0x100084F4
undefined4 g_updateTimerId = 0x00000000;

// GLOBAL: MAGSND 0x100084F8
TIMECAPS g_timeCaps = {0};

// FUNCTION: MAGSND 0x10001000
undefined4 __cdecl InitSnd(HWND hwnd, undefined4 unused, uint flags)
{
  HRESULT hr;
  DWORD format;

  if (flags & (byte)0x2)
  {
    if (g_sndRefCount == 0)
    {
      return 4;
    }
  }
  if (flags & 2)
  {
    if (g_sndRefCount != 0)
    {
      return 0;
    }
  }
  if (g_sndRefCount == 0)
  {
    if (hwnd != 0)
    {
      hr = DirectSoundCreate(0, &g_directSound, 0);
      if (hr != 0)
      {
        return 4;
      }
      hr = g_directSound->lpVtbl->SetCooperativeLevel(g_directSound, hwnd, 3);
      if (hr != 0)
      {
        ReleaseSnd();
        return 4;
      }
      hr = g_directSound->lpVtbl->CreateSoundBuffer(g_directSound, &g_primaryDsDesc, &g_primaryDsBuffer, 0);
      if (hr != 0)
      {
        ReleaseSnd();
        return 4;
      }
      hr = g_primaryDsBuffer->lpVtbl->SetFormat(g_primaryDsBuffer, &g_primaryWaveFormat);
      if (hr != 0)
      {
        g_primaryDsBuffer->lpVtbl->GetFormat(g_primaryDsBuffer, &g_primaryWaveFormat, 0x12, &format);
      }
      g_sndHwnd = hwnd;
      g_sndRefCount = g_sndRefCount + 1;
      InitializeCriticalSection(&g_sndCs);
    }
  }
  if ((flags & 1) != 0)
  {
    if (g_updateTimerActive != 0)
    {
      StopUpdateTimer();
    }
    g_updateTimerEnabled = 0;
  }
  return 0;
}

// FUNCTION: MAGSND 0x10001176
void ReleaseSnd(void)
{
  if (g_sndRefCount == 0)
    return;

  UnloadAllSnds();
  if (g_updateTimerActive != 0)
  {
    StopUpdateTimer();
  }
  g_directSound->lpVtbl->Release(g_directSound);
  g_directSound = NULL;
  g_sndHwnd = 0;
  g_sndRefCount = 0;
  DeleteCriticalSection(&g_sndCs);
  return;
}

// FUNCTION: MAGSND 0x100011E8
HWND GetSndHWND(void)
{
  return g_sndHwnd;
}

// FUNCTION: MAGSND 0x100011FD
int __cdecl LoadSnd(LPSTR path, int slot, Sound *sound)
{
  int result;

  if ((sound != (Sound *)0) && ((sound->flags >> 4 & 1U) != 0) && (slot != 0))
  {
    if (0x10f < slot || slot < 0x100)
    {
      return 5;
    }
    if (g_sndSlots[slot] != (SndInstance *)0x0)
    {
      return 0;
    }
  }
  else
  {
    if (0x100 < slot || slot < 0)
    {
      return 5;
    }
    if (g_sndSlots[slot] != (SndInstance *)0x0)
    {
      return 0;
    }
  }
  if ((sound != (Sound *)0) && ((sound->flags >> 2 & 1U) != 0))
  {
    if ((sound->flags >> 4 & 1U) != 0)
    {
      result = LoadAviAudioStream((void *)path, (&g_sndSlots[slot]), 0);
      if (result != 0)
      {
        g_sndSlots[slot] = (SndInstance *)0;
        return result;
      }
      if ((sound->flags >> 5 & 1U) != 0)
      {
        g_sndSlots[slot]->flags2 = (int)g_sndSlots[slot]->flags2 | 0x40;
      }
      else
      {
        PrimeAviAudio(g_sndSlots[slot], 0);
        g_sndSlots[slot]->flags2 &= ~0x40u;
      }
      g_sndLoadSerial += 1;
    }
    else
    {
      result = LoadWaveMmio(path, (&g_sndSlots[slot]), 0);
      if (result != 0)
      {
        g_sndSlots[slot] = (SndInstance *)0;
        return result;
      }
      PrimeDsBufferFromSource(g_sndSlots[slot], 0);
    }
    g_sndSlots[slot]->flags = (int)g_sndSlots[slot]->flags | 0x20;
    AddToUpdateList(g_sndSlots[slot]);
    g_sndSlots[slot]->flags2 = (int)g_sndSlots[slot]->flags2 | 2;
  }
  else
  {
    result = LoadFromFile(path, (&g_sndSlots[slot]));
    if (result != 0)
    {
      g_sndSlots[slot] = (SndInstance *)0;
      return result;
    }
  }

  EnterCriticalSection(&g_sndCs);
  AddToActiveList(g_sndSlots[slot]);
  g_sndSlots[slot]->slotIndex = slot;
  if ((sound != (Sound *)0) && (sound->loadId != 0))
  {
    g_sndSlots[slot]->loadId = sound->loadId;
  }
  LeaveCriticalSection(&g_sndCs);
  return 0;
}

// FUNCTION: MAGSND 0x100014BA
undefined4 __cdecl UnloadSnd(int slot)
{
  uint i;

  if ((slot > 0x10f) || (0 > slot))
  {
    return 5;
  }
  EnterCriticalSection(&g_sndCs);
  if (g_sndSlots[slot] == (SndInstance *)0x0)
  {
    LeaveCriticalSection(&g_sndCs);
    return 1;
  }
  StopSndNow(slot);
  RemoveFromActiveList(g_sndSlots[slot]);
  if (((g_sndSlots[slot]->flags2 >> 1) & 1) != 0)
  {
    RemoveFromUpdateList(g_sndSlots[slot]);
    if (((g_sndSlots[slot]->flags2 >> 5) & 1) == 0)
    {
      CloseMmioAndFreeSource(g_sndSlots[slot]);
      for (i = 0; i < g_sndSlots[slot]->markerCount; i = i + 1)
      {
        if (g_sndSlots[slot]->markers[i + 1] != (void *)0x0)
        {
          CloseMmioAndFreeSource(g_sndSlots[slot]->markers[i + 1]);
        }
      }
    }
  }
  DestroySndInstance(g_sndSlots[slot]);
  g_sndSlots[slot] = (SndInstance *)0;
  LeaveCriticalSection(&g_sndCs);
  return 0;
}

// FUNCTION: MAGSND 0x1000161D
undefined4 UnloadAllSnds(void)
{
  EnterCriticalSection(&g_sndCs);
  while (g_activeSndListHead != (SndInstance *)0x0)
  {
    UnloadSnd(g_activeSndListHead->slotIndex);
  }
  LeaveCriticalSection(&g_sndCs);
  return 0;
}

// FUNCTION: MAGSND 0x10001668
undefined4 __cdecl PlaySnd(int slot, Sound *sound)
{
  int unused;
  if (0x10f < slot || slot < 0)
  {
    return 5;
  }
  EnterCriticalSection(&g_sndCs);
  if (g_sndSlots[slot] == (SndInstance *)0x0)
  {
    LeaveCriticalSection(&g_sndCs);
    return 1;
  }
  unused = PlaySndInstance(g_sndSlots[slot], sound);
  g_sndSlots[slot]->lruCounter++;
  LeaveCriticalSection(&g_sndCs);
  return 0;
}

#define ONE 1

// FUNCTION: MAGSND 0x10001701
int __cdecl PlaySndInstance(SndInstance *snd, Sound *sound)

{
  int result2;                  // ebp - 0x1c
  int freqValue;                // ebp - 0x18
  LPDIRECTSOUNDBUFFER dsBuffer; // ebp - 0x14
  uint playFlags;               // ebp - 0x10
  int panValue;                 // ebp - 0xc
  int sampleRate;               // ebp - 0x8
  int result;                   // ebp - 0x4

  playFlags = 0;
  freqValue = 0;
  sampleRate = 0;
  if (((snd->flags2 >> ONE) & 1) != 0)
  {
    if ((snd->flags & 1) != 0)
    {
      StopSndNow(snd->slotIndex);
    }
    if ((g_updateTimerEnabled == 1) && (g_updateTimerActive == 0))
    {
      result = StartUpdateTimer(g_updateResolutionMs);
      if (result != 0)
      {
        UnloadSnd(snd->slotIndex);
        return result;
      }
    }
    if (g_updateTimerEnabled != 0)
    {
      g_updateTimerUsers = g_updateTimerUsers + 1;
    }
    playFlags |= 1;
    dsBuffer = snd->dsBuffer;
    if (((snd->flags >> 5) & 1) == 0)
    {
      result = ResetAndPrimeSndInstance(snd);
      if (result != 0)
      {
        UnloadSnd(snd->slotIndex);
        return result;
      }
    }
  }
  else
  {
    if ((sound == NULL) || ((sound->flags >> 1 & 1) == 0))
    {
      result = AcquireFreeDsBuffer(snd, &dsBuffer);
      if (result != 0)
      {
        return result;
      }
    }
    else
    {
      dsBuffer = snd->dsBuffer;
    }
  }
  if (sound != NULL)
  {
    freqValue = sound->volume;
    if (freqValue > 400)
    {
      freqValue = 400;
    }
    snd->volume = freqValue;
    freqValue = (freqValue - 400) * 10;
    if (sound->sampleRate != 0)
    {
      sampleRate = sound->sampleRate;
    }
    else
    {
      sampleRate = snd->waveFmt.nSamplesPerSec;
    }
    snd->sampleRate = sampleRate;
    if (sound->pan != 0)
    {
      panValue = sound->pan;
    }
    else
    {
      panValue = 0;
    }
    snd->pan = panValue;
    panValue = panValue * 10;
    if ((sound->flags & 1) != 0)
    {
      playFlags |= 1;
      snd->flags2 = (int)snd->flags2 | 1;
    }
    if ((sound->flags >> 3 & 1) != 0)
    {
      snd->flags2 = (int)snd->flags2 | 4;
    }
  }
  else
  {
    freqValue = 0;
    snd->volume = 400;
    sampleRate = snd->waveFmt.nSamplesPerSec;
    snd->sampleRate = sampleRate;
    panValue = 0;
    snd->pan = 0;
  }
  dsBuffer->lpVtbl->SetVolume(dsBuffer, freqValue);
  dsBuffer->lpVtbl->SetFrequency(dsBuffer, sampleRate);
  dsBuffer->lpVtbl->SetPan(dsBuffer, panValue);
  dsBuffer->lpVtbl->SetCurrentPosition(dsBuffer, 0);
  if (dsBuffer->lpVtbl->Play(dsBuffer, 0, 0, playFlags))
  {
    result2 = 1;
  }
  else
  {
    result2 = 0;
  }
  if (result2 != 0)
  {
    return 9;
  }

  snd->flags = (int)snd->flags | 1;
  snd->flags &= 0xffffffdf;
  return 0;
}

// FUNCTION: MAGSND 0x10001A1F
int __cdecl PlaySndFile(LPSTR filename, int slot, Sound *sound)

{
  DWORD writeCursorBytes;
  int volume;
  int pan;
  int nSamplesPerSec;
  int result;
  undefined4 scratch;

  scratch = 0;
  volume = 0;
  nSamplesPerSec = 0;
  if ((0xff < slot) || (slot < 0))
  {
    return 5;
  }
  EnterCriticalSection(&g_sndCs);
  if ((int)g_sndSlots[slot] != 0)
  {
    LeaveCriticalSection(&g_sndCs);
    return 2;
  }
  result = LoadWaveMmio(filename, (&g_sndSlots[slot]), scratch);
  if (result != 0)
  {
    LeaveCriticalSection(&g_sndCs);
    return result;
  }
  AddToActiveList(g_sndSlots[slot]);
  AddToUpdateList(g_sndSlots[slot]);
  if ((g_updateTimerEnabled == 1) && (g_updateTimerActive == 0))
  {
    result = StartUpdateTimer(g_updateResolutionMs);
    if (result != 0)
    {
      UnloadSnd(slot);
      LeaveCriticalSection(&g_sndCs);
      return result;
    }
  }
  g_sndSlots[slot]->slotIndex = slot;
  if (sound != (Sound *)0)
  {
    volume = sound->volume;
    g_sndSlots[slot]->volume = volume;
    if (400 < volume)
    {
      volume = 400;
    }
    volume = (volume - 400) * 10;
    if (sound->sampleRate != 0)
    {
      nSamplesPerSec = sound->sampleRate;
    }
    else
    {
      nSamplesPerSec = (int)g_sndSlots[slot]->waveFmt.nSamplesPerSec;
    }
    g_sndSlots[slot]->sampleRate = nSamplesPerSec;

    if (sound->pan != 0)
    {
      pan = sound->pan;
    }
    else
    {
      pan = 0;
    }
    g_sndSlots[slot]->pan = pan;
    pan = pan * 10;
    if (sound->flags & 1)
    {
      g_sndSlots[slot]->flags2 = (int)g_sndSlots[slot]->flags2 | 1;
    }
    if ((sound->flags >> 3 & 1) != 0)
    {
      g_sndSlots[slot]->flags2 = (int)g_sndSlots[slot]->flags2 | 4;
    }
  }
  else
  {
    volume = 0;
    g_sndSlots[slot]->volume = 400;
    nSamplesPerSec = (int)g_sndSlots[slot]->waveFmt.nSamplesPerSec;
    g_sndSlots[slot]->sampleRate = nSamplesPerSec;
    pan = 0;
    g_sndSlots[slot]->pan = pan;
  }
  g_sndSlots[slot]->flags2 = (int)g_sndSlots[slot]->flags2 | 2;

  g_sndSlots[slot]->dsBuffer->lpVtbl->SetVolume(g_sndSlots[slot]->dsBuffer, volume);
  g_sndSlots[slot]->volume = volume;

  g_sndSlots[slot]->dsBuffer->lpVtbl->SetFrequency(g_sndSlots[slot]->dsBuffer, nSamplesPerSec);
  g_sndSlots[slot]->sampleRate = nSamplesPerSec;

  g_sndSlots[slot]->dsBuffer->lpVtbl->SetPan(g_sndSlots[slot]->dsBuffer, pan);
  g_sndSlots[slot]->pan = pan;

  PrimeDsBufferFromSource(g_sndSlots[slot], 0);

  g_sndSlots[slot]->dsBuffer->lpVtbl->Play(g_sndSlots[slot]->dsBuffer, 0, 0, 1);
  g_sndSlots[slot]->dsBuffer->lpVtbl->GetCurrentPosition(g_sndSlots[slot]->dsBuffer, (LPDWORD)&g_sndSlots[slot]->ringCursorBytes, &writeCursorBytes);
  g_sndSlots[slot]->flags = (int)g_sndSlots[slot]->flags | 1;
  LeaveCriticalSection(&g_sndCs);
  return 0;
}

// FUNCTION: MAGSND 0x10001E52
int __cdecl LoadMarkerInstance(SndInstance *parent, int markerIndex)
{
  SndInstance **markerSlot;
  int markerMetaIndex;
  SndInstance **markerMeta;
  int result;
  int markerStartSample;

  result = 0;
  markerStartSample = 0;
  markerMetaIndex = 0;

  markerMeta = &parent->markers[markerIndex * 3 - 3];
  markerSlot = &parent->markers[markerIndex - 1] + 1;
  result = LoadWaveMmio((LPSTR)parent->streamOrMmio, markerSlot, 0x10);
  if (result != 0)
  {
    return result;
  }

  markerStartSample = (*markerMeta)->loadId;
  result = PrimeDsBufferFromSource(*markerSlot, markerStartSample);
  if (result != 0)
  {
    return result;
  }

  (*markerSlot)->flags = (int)(*markerSlot)->flags | 0x20;
  AddToUpdateList(*markerSlot);
  (*markerSlot)->flags2 = (int)(*markerSlot)->flags2 | 2;
  (*markerSlot)->flags2 = (int)(*markerSlot)->flags2 | 0x10;
  (*markerSlot)->flags2 = (parent->flags2 & 1) | ((*markerSlot)->flags2 & 0xfffffffe);
  (*markerSlot)->slotIndex = parent->slotIndex;
  return result;
}

// FUNCTION: MAGSND 0x10001F6F
int __cdecl PrimeMarkerInstance(SndInstance *parent, int markerIndex)
{
  int markerMetaIndex;
  SndInstance *markerInstance;
  SndInstance *markerMeta;
  int result;
  int markerStartSample;

  result = 0;
  markerStartSample = 0;
  markerMetaIndex = 0;

  if (((parent->flags >> 6) & 1) != 0)
  {
    if (parent->activeMarkerIndex == markerIndex - 1)
    {
      return 0xd;
    }
  }

  markerMeta = parent->markers[(markerIndex - 1) * 6];
  markerInstance = parent->markers[markerIndex];
  markerStartSample = markerMeta->loadId;
  result = PrimeDsBufferFromSource(markerInstance, markerStartSample);
  if (result != 0)
  {
    return result;
  }

  markerInstance->flags = (int)markerInstance->flags | 0x20;
  markerInstance->flags &= 0xfffffffb;
  markerInstance->flags &= 0xffffffef;
  markerInstance->ringCursorBytes = 0;
  markerInstance->flags2 = (parent->flags2 & 1) | (markerInstance->flags2 & 0xfffffffe);
  markerInstance->volume = parent->volume;
  markerInstance->sampleRate = parent->sampleRate;
  markerInstance->pan = parent->pan;
  return 0;
}

// FUNCTION: MAGSND 0x1000208C
void __cdecl SwapActiveInstances(int *activeInstanceSlot, int *newInstanceSlot)
{
  int i;
  int old;

  old = *activeInstanceSlot;
  *activeInstanceSlot = *newInstanceSlot;
  i = 0;
  for (; i < 0x10; i = i + 1)
  {
    if (*(int *)(old + 0x38 + i * 4) == *activeInstanceSlot)
    {
      *(int *)(*activeInstanceSlot + 0x38 + i * 4) = old;
    }
    else
    {
      *(int *)(*activeInstanceSlot + 0x38 + i * 4) = *(int *)(old + 0x38 + i * 4);
    }
    *(undefined4 *)(old + 0x38 + i * 4) = 0;
  }
  *(int *)(*activeInstanceSlot + 0x10) = *(int *)(old + 0x10);
  *(uint *)(old + 8) = *(int *)(old + 8) | 0x10;
  *(uint *)(*activeInstanceSlot + 8) = *(uint *)(*activeInstanceSlot + 8) & 0xffffffef;
  RemoveFromActiveList((SndInstance *)old);
  AddToActiveList((SndInstance *)*activeInstanceSlot);
  return;
}

// FUNCTION: MAGSND 0x1000215C
int __cdecl SetSndMarker(int slot, uint markerIndex)
{
  SndInstance *snd;
  int result;

  result = 0;
  if ((slot > 0xff) || (slot < 0))
  {
    return 5;
  }

  EnterCriticalSection(&g_sndCs);
  snd = g_sndSlots[slot];
  if (snd == (SndInstance *)0x0)
  {
    LeaveCriticalSection(&g_sndCs);
    return 1;
  }
  if ((snd->markerCount == 0) || (snd->markerCount < markerIndex))
  {
    LeaveCriticalSection(&g_sndCs);
    return 5;
  }
  if (snd->markers[markerIndex] != (void *)0x0)
  {
    result = PrimeMarkerInstance(snd, markerIndex);
  }
  else
  {
    result = LoadMarkerInstance(snd, markerIndex);
  }
  LeaveCriticalSection(&g_sndCs);
  return result;
}

// FUNCTION: MAGSND 0x10002249
int __cdecl PlaySndMarker(int slot, uint markerIndex)
{
  SndInstance *marker; // ebp - 0x2c
  SndInstance *snd;    // ebp - 0x28
  int result;          // ebp - 0x24
  Sound sound;         // ebp - 0x20

  result = 0;

  if ((slot > 0xff) || (slot < 0))
  {
    return 5;
  }
  if (((int)markerIndex > 0x10) || ((int)markerIndex < 0))
  {
    return 5;
  }
  EnterCriticalSection(&g_sndCs);
  snd = g_sndSlots[slot];
  if (snd == (SndInstance *)0x0)
  {
    LeaveCriticalSection(&g_sndCs);
    return 1;
  }
  if ((snd->markerCount == 0) || (snd->markerCount < markerIndex))
  {
    LeaveCriticalSection(&g_sndCs);
    return 5;
  }
  marker = snd->markers[markerIndex];
  if (marker == (SndInstance *)0x0)
  {
    LeaveCriticalSection(&g_sndCs);
    return 1;
  }
  if (((marker->flags >> 5) & 1) == 0)
  {
    LeaveCriticalSection(&g_sndCs);
    return 1;
  }

  if ((snd->flags & 1) != 0)
  {
    StopSndNow(slot);
  }

  marker->volume = g_sndSlots[slot]->volume;
  sound.volume = marker->volume;
  marker->sampleRate = g_sndSlots[slot]->sampleRate;
  sound.sampleRate = marker->sampleRate;
  marker->pan = g_sndSlots[slot]->pan;
  sound.pan = marker->pan;
  sound.flags = (marker->flags2 & 1) | (sound.flags & 0xfffffffe);

  result = PlaySndInstance(marker, &sound);
  if (result == 0)
  {
    g_sndSlots[slot]->flags = (int)g_sndSlots[slot]->flags | 0x40;
    g_sndSlots[slot]->activeMarkerIndex = markerIndex - 1;
    g_sndSlots[slot]->flags = (int)g_sndSlots[slot]->flags | 1;
  }
  LeaveCriticalSection(&g_sndCs);
  return result;
}

// FUNCTION: MAGSND 0x1000246E
undefined4 __cdecl StopSnd(int slot)
{
  int status;
  SndDuplicateBufferEntry *duplicateBuffer;
  int i;
  SndInstance *marker;
  int markerIndex;

  if ((0x10f < slot) || (slot < 0))
  {
    return 5;
  }

  EnterCriticalSection(&g_sndCs);
  if (g_sndSlots[slot] == (SndInstance *)0x0)
  {
    LeaveCriticalSection(&g_sndCs);
    return 1;
  }
  if (((g_sndSlots[slot]->flags >> 6) & 1) != 0)
  {
    markerIndex = g_sndSlots[slot]->activeMarkerIndex;
    marker = g_sndSlots[slot]->markers[markerIndex + 1];
    if (marker == (SndInstance *)0x0)
    {
      LeaveCriticalSection(&g_sndCs);
      return 1;
    }
    marker->flags = (int)marker->flags | 2;
  }
  else
  {
    if (((g_sndSlots[slot]->flags2 >> 1) & 1) != 0)
    {
      g_sndSlots[slot]->flags = (int)g_sndSlots[slot]->flags | 2;
    }
    else
    {
      status = g_sndSlots[slot]->dsBuffer->lpVtbl->Stop(g_sndSlots[slot]->dsBuffer);
      if (status != 0)
      {
        LeaveCriticalSection(&g_sndCs);
        return 9;
      }
    }
  }
  if (((g_sndSlots[slot]->flags2 >> 1) & 1) == 0)
  {
    for (i = 0; i < 0x10; i++)
    {
      duplicateBuffer = &g_sndSlots[slot]->duplicateBuffers[i];
      if (duplicateBuffer->buffer != 0)
      {
        status = duplicateBuffer->buffer->lpVtbl->Stop(duplicateBuffer->buffer);
        if (status != 0)
        {
          LeaveCriticalSection(&g_sndCs);
          return 9;
        }
      }
      else
        break;
    }
    g_sndSlots[slot]->flags &= ~1;
  }

  LeaveCriticalSection(&g_sndCs);
  return 0;
}

// FUNCTION: MAGSND 0x10002676
void StopAllSnds(void)
{
  SndInstance *node;
  EnterCriticalSection(&g_sndCs);
  node = g_activeSndListHead;
  while (node != (SndInstance *)0x0)
  {
    StopSnd(node->slotIndex);
    node = node->activeNext;
  }
  LeaveCriticalSection(&g_sndCs);
  return;
}

// FUNCTION: MAGSND 0x100026D1
undefined4 __cdecl StopSndNow(int slot)
{
  int status;
  LPLPDIRECTSOUNDBUFFER subPtr;
  int i;
  SndInstance *markerInstance;
  int markerIndex;

  if (slot > 0x10f || slot < 0)
  {
    return 5;
  }
  if (g_sndSlots[slot] == (SndInstance *)0x0)
  {
    return 1;
  }
  if (((g_sndSlots[slot]->flags >> 6) & 1) != 0)
  {
    markerIndex = g_sndSlots[slot]->activeMarkerIndex;
    markerInstance = g_sndSlots[slot]->markers[markerIndex + 1];
    if (markerInstance == 0)
    {
      return 1;
    }

    status = markerInstance->dsBuffer->lpVtbl->Stop(markerInstance->dsBuffer);
    if (status != 0)
    {
      return 9;
    }
    g_sndSlots[slot]->flags &= 0xffffffbf;
    markerInstance->flags &= 0xfffffffe;
    markerInstance->flags &= 0xfffffffd;
    markerInstance->flags = (int)markerInstance->flags | 4;
    markerInstance->flags &= 0xffffffdf;
  }
  else
  {
    status = g_sndSlots[slot]->dsBuffer->lpVtbl->Stop(g_sndSlots[slot]->dsBuffer);
    if (status != 0)
    {
      return 9;
    }
  }
  if (((g_sndSlots[slot]->flags2 >> 1) & 1) == 0)
  {
    for (i = 0; i < 0x10; i = i + 1)
    {
      subPtr = &g_sndSlots[slot]->duplicateBuffers[i].buffer;
      if ((*subPtr) != 0x0)
      {
        status = (*subPtr)->lpVtbl->Stop(*subPtr);
        if (status != 0)
        {
          return 9;
        }
      }
      else
        break;
    }
  }
  else if (g_updateTimerUsers > 0)
  {
    g_updateTimerUsers = g_updateTimerUsers - 1;
    if (g_updateTimerUsers == 0)
    {
      StopUpdateTimer();
    }
  }
  g_sndSlots[slot]->flags &= 0xfffffffe;
  g_sndSlots[slot]->flags &= 0xfffffffd;
  g_sndSlots[slot]->flags = (int)g_sndSlots[slot]->flags | 4;
  g_sndSlots[slot]->flags &= 0xffffffdf;
  return 0;
}

// FUNCTION: MAGSND 0x1000290C
undefined4 PlayMidiFile(void)

{
  /* 0x290c  10  PlayMidiFile */
  return 0;
}

// FUNCTION: MAGSND 0x1000291E
undefined4 __cdecl SetPitch(int slot, undefined4 pitchHz)
{
  LPLPDIRECTSOUNDBUFFER duplicatePtr;
  int index;

  duplicatePtr = 0;
  if ((0x10f < slot) || (slot < 0))
  {
    return 5;
  }

  EnterCriticalSection(&g_sndCs);
  if (g_sndSlots[slot] == (SndInstance *)0x0)
  {
    LeaveCriticalSection(&g_sndCs);
    return 1;
  }

  g_sndSlots[slot]->dsBuffer->lpVtbl->SetFrequency(g_sndSlots[slot]->dsBuffer, pitchHz);
  for (index = 0; index < 0x10; index = index + 1)
  {
    duplicatePtr = &g_sndSlots[slot]->duplicateBuffers[index].buffer;
    if (*duplicatePtr != (LPDIRECTSOUNDBUFFER)0x0)
    {
      (*duplicatePtr)->lpVtbl->SetFrequency(*duplicatePtr, pitchHz);
    }
    else
      break;
  }
  LeaveCriticalSection(&g_sndCs);
  return 0;
}

// FUNCTION: MAGSND 0x10002A24
undefined4 GetPitch(void)
{
  return 0;
}

// FUNCTION: MAGSND 0x10002A36
undefined4 __cdecl SetVol(int slot, uint volume)
{
  SndDuplicateBufferEntry *duplicateBuffer;
  int i;
  SndInstance *markerInst;
  int markerIndex;

  duplicateBuffer = 0;

  if (slot > 0x10f || slot < 0)
  {
    return 5;
  }
  EnterCriticalSection(&g_sndCs);
  if ((int)g_sndSlots[slot] == 0)
  {
    LeaveCriticalSection(&g_sndCs);
    return 1;
  }
  if (400 < volume)
  {
    volume = 400;
  }
  if (volume < 0)
  {
    volume = 0;
  }

  if ((*(uint *)((int)g_sndSlots[slot] + 4) >> 6 & 1) != 0)
  {
    markerIndex = g_sndSlots[slot]->activeMarkerIndex;
    markerInst = g_sndSlots[slot]->markers[markerIndex + 1];
    if (markerInst == 0)
    {
      LeaveCriticalSection(&g_sndCs);
      return 1;
    }
    else
    {
      markerInst->volume = volume;
      volume = (volume - 400) * 10;
      markerInst->dsBuffer->lpVtbl->SetVolume(markerInst->dsBuffer, volume);
    }
  }
  else
  {
    g_sndSlots[slot]->volume = volume;
    g_sndSlots[slot]->volume = volume;
    volume = (volume - 400) * 10;
    g_sndSlots[slot]->dsBuffer->lpVtbl->SetVolume(g_sndSlots[slot]->dsBuffer, volume);
  }
  for (i = 0; i < 0x10; i = i + 1)
  {
    duplicateBuffer = &g_sndSlots[slot]->duplicateBuffers[i];
    if (duplicateBuffer->buffer != 0)
    {
      duplicateBuffer->buffer->lpVtbl->SetVolume(duplicateBuffer->buffer, volume);
    }
    else
      break;
  }
  LeaveCriticalSection(&g_sndCs);
  return 0;
}

// FUNCTION: MAGSND 0x10002C32
undefined4 GetVol(void)

{
  /* 0x2c32  14  GetVol */
  return 0;
}

// FUNCTION: MAGSND 0x10002C44
undefined4 __cdecl SetPan(int slot, int pan)

{
  LPLPDIRECTSOUNDBUFFER duplicateBuffer;
  int index;

  duplicateBuffer = 0;
  if ((0x10f < slot) || (slot < 0))
  {
    return 5;
  }
  EnterCriticalSection(&g_sndCs);
  if (g_sndSlots[slot] == (SndInstance *)0x0)
  {
    LeaveCriticalSection(&g_sndCs);
    return 1;
  }

  pan = (pan * 5) * 2;
  // pan = pan + pan;
  g_sndSlots[slot]->dsBuffer->lpVtbl->SetPan(g_sndSlots[slot]->dsBuffer, pan);
  for (index = 0; index < 0x10; index++)
  {
    duplicateBuffer = &g_sndSlots[slot]->duplicateBuffers[index].buffer;
    if (*duplicateBuffer != NULL)
    {
      (*duplicateBuffer)->lpVtbl->SetPan(*duplicateBuffer, pan);
    }
    else
      break;
  }
  LeaveCriticalSection(&g_sndCs);
  return 0;
}

// FUNCTION: MAGSND 0x10002D55
undefined4 GetPan(void)
{
  return 0;
}

// FUNCTION: MAGSND 0x10002D67
undefined4 UpdateSnd(void)
{
  SndInstance *node;
  SndInstance *next;

  if (g_updateSndDisabled != 0)
  {
    return 0xd;
  }

  EnterCriticalSection(&g_sndCs);
  node = g_updateSndHead;
  while (node != (SndInstance *)0x0)
  {
    if ((node->flags & 1) != 0)
    {
      if (((node->flags >> 1) & 1) != 0)
      {
        if ((uint)node->volume > 5)
        {
          node->volume = node->volume - 5;
        }
        else
        {
          node->volume = 0;
        }
        SetVol(node->slotIndex, node->volume);
      }

      if (((node->flags2 >> 5) & 1) != 0)
      {
        if (((node->flags2 >> 6) & 1) == 0)
        {
          UpdateAviSnd(node);
        }
      }
      else
      {
        UpdateMmioSnd(node);
      }
    }

    if (((node->flags >> 2) & 1) != 0 && ((node->flags2 >> 2) & 1) != 0)
    {
      next = node->activeNext;
      UnloadSnd(node->slotIndex);
      node = next;
    }
    else
      node = node->updateNext;
  }
  LeaveCriticalSection(&g_sndCs);
  return 0;
}

#pragma pack(push, 4)
// FUNCTION: MAGSND 0x10002EB5
undefined4 __cdecl GetSndTime(int slot, uint *outSndTime)
{
  unsigned long playCursorBytes;
  SndInstance *snd;
  uint prevCursorBytes;
  double timeBytes;
  uint blockIndex;
  unsigned long writeCursorBytes;

  if (0x10f < slot || slot < 0)
  {
    return 5;
  }
  EnterCriticalSection(&g_sndCs);
  if (g_sndSlots[slot] == (SndInstance *)0x0)
  {
    LeaveCriticalSection(&g_sndCs);
    return 1;
  }
  snd = g_sndSlots[slot];
  snd->dsBuffer->lpVtbl->GetCurrentPosition(snd->dsBuffer, &playCursorBytes, &writeCursorBytes);
  prevCursorBytes = snd->timeBytes % snd->dsDesc.dwBufferBytes;
  if (prevCursorBytes < playCursorBytes)
  {
    snd->timeBytes += (playCursorBytes - prevCursorBytes);
  }
  else
  {
    snd->timeBytes += (snd->dsDesc.dwBufferBytes - prevCursorBytes);
    snd->timeBytes += playCursorBytes;
  }
  if (((snd->flags2 >> 5) & 1) != 0)
  {
    blockIndex = snd->timeBytes / snd->blockBytes;
    while (snd->timeEventCount < blockIndex)
    {
      PostMessageA(g_sndHwnd, 0x3bd, 0, blockIndex);
      snd->timeEventCount++;
    }
  }
  timeBytes = (double)snd->timeBytes;
  if (snd->waveFmt.nAvgBytesPerSec == 0x15888)
  {
    *outSndTime = (uint)(long)(timeBytes / g_sndTimeDiv_88200);
  }
  else if (snd->waveFmt.nAvgBytesPerSec == 0xac44)
  {
    *outSndTime = (uint)(long)(timeBytes / g_sndTimeDiv_44100);
  }
  else if (snd->waveFmt.nAvgBytesPerSec == 0x5622)
  {
    *outSndTime = (uint)(long)(timeBytes / g_sndTimeDiv_22050);
  }
  else if (snd->waveFmt.nAvgBytesPerSec == 0x2b11)
  {
    *outSndTime = (uint)(long)(timeBytes / g_sndTimeDiv_11025);
  }
  else
  {

    LeaveCriticalSection(&g_sndCs);
    return 8;
  }
  g_sndTimeSamples[g_sndTimeSampleIndex].prevTime = g_lastSndTime;
  g_sndTimeSamples[g_sndTimeSampleIndex].currTime = *outSndTime;
  g_sndTimeSampleIndex = g_sndTimeSampleIndex + 1;
  g_sndTimeSampleIndex = g_sndTimeSampleIndex & 0xff;
  if (*outSndTime < g_lastSndTime)
  {
    LeaveCriticalSection(&g_sndCs);
    return 5;
  }
  g_lastSndTime = *outSndTime;
  LeaveCriticalSection(&g_sndCs);
  return 0;
}
#pragma pack(pop)

// FUNCTION: MAGSND 0x1000314D
undefined4 __cdecl GetSndState(int slot, undefined4 *outSndState)
{
  if ((0x10f < slot) || (slot < 0))
  {
    return 5;
  }
  EnterCriticalSection(&g_sndCs);
  if (g_sndSlots[slot] == (SndInstance *)0x0)
  {
    LeaveCriticalSection(&g_sndCs);
    return 1;
  }
  if ((g_sndSlots[slot]->flags & 1) != 0)
  {
    *outSndState = 1;
  }
  else if (((g_sndSlots[slot]->flags >> 3) & 1) != 0)
  {
    *outSndState = 2;
  }
  else
  {
    *outSndState = 0;
  }
  LeaveCriticalSection(&g_sndCs);
  return 0;
}

// FUNCTION: MAGSND 0x1000320D
undefined4 ResetSnd(void)

{
  /* 0x320d  21  ResetSnd */
  return 0;
}

// FUNCTION: MAGSND 0x1000321F
int __cdecl IsSndLoaded(int loadId, int *outSlot)

{
  SndInstance *activeNode;

  /* 0x321f  26  IsSndLoaded */
  if (loadId <= 0)
  {
    return 0;
  }
  EnterCriticalSection(&g_sndCs);
  activeNode = g_activeSndListHead;
  while (activeNode != NULL)
  {
    if (activeNode->loadId == loadId)
    {
      *outSlot = activeNode->slotIndex;
      LeaveCriticalSection(&g_sndCs);
      return 1;
    }
    activeNode = activeNode->activeNext;
  }
  LeaveCriticalSection(&g_sndCs);
  return 0;
}

// FUNCTION: MAGSND 0x100032AD
undefined4 __cdecl GetLRUSnd(int *outSlot, int startSlot, int endSlot)
{
  int selectedSlot;
  int slot;
  uint lowestLruCounter;
  int foundEmptySlot;

  foundEmptySlot = 0;
  lowestLruCounter = -1;
  if (endSlot != 0 && endSlot > startSlot && endSlot < 0x100)
  {
    slot = startSlot;
  }
  else
  {
    slot = 0;
    endSlot = 0xff;
  }

  EnterCriticalSection(&g_sndCs);
  for (; slot < endSlot; slot = slot + 1)
  {
    if (g_sndSlots[slot] != 0)
    {
      if (g_sndSlots[slot]->lruCounter < lowestLruCounter)
      {
        lowestLruCounter = g_sndSlots[slot]->lruCounter;
        selectedSlot = g_sndSlots[slot]->slotIndex;
      }
    }
    else
    {
      selectedSlot = slot;
      foundEmptySlot = 1;
      break;
    }
  }
  *outSlot = selectedSlot;
  LeaveCriticalSection(&g_sndCs);
  return foundEmptySlot;
}

// FUNCTION: MAGSND 0x100033A2
void *__cdecl GetAVISndBuff(int slot, uint blockNumber)
{
  void *lockPtr1;
  SndInstance *snd;
  undefined4 unusedZero;
  uint blockIndex;
  int hr;
  void *lockPtr2;
  DWORD lockBytes2;
  DWORD lockBytes1;
  int offset;

  lockPtr1 = 0;
  lockPtr2 = 0;
  lockBytes1 = 0;
  lockBytes2 = 0;
  unusedZero = 0;

  if (slot > 0x10f || slot < 0x100)
  {
    return 0;
  }
  EnterCriticalSection(&g_sndCs);
  snd = g_sndSlots[slot];
  if (snd == 0)
  {
    LeaveCriticalSection(&g_sndCs);
    return 0;
  }
  if (((snd->flags >> 7) & 1) != 0)
  {
    LeaveCriticalSection(&g_sndCs);
    return 0;
  }

  blockIndex = blockNumber % snd->blockCount;
  offset = snd->blockBytes * blockIndex;
  hr = snd->dsBuffer->lpVtbl->Lock(
      snd->dsBuffer, offset, snd->blockBytes,
      &lockPtr1, &lockBytes1, &lockPtr2, &lockBytes2, 0);
  if (hr != 0)
  {
    return 0;
  }
  if (lockPtr2 != 0)
  {
    snd->dsBuffer->lpVtbl->Unlock(snd->dsBuffer, lockPtr1, lockBytes1, lockPtr2, lockBytes2);
    LeaveCriticalSection(&g_sndCs);
    return 0;
  }
  snd->dsLockedPtr = lockPtr1;
  snd->flags = (int)snd->flags | 0x80;
  LeaveCriticalSection(&g_sndCs);
  return lockPtr1;
}

// FUNCTION: MAGSND 0x1000352C
undefined4 __cdecl ReleaseAVISndBuff(int slot)
{
  int unused_18;
  SndInstance *snd;
  int unused_10;
  int unused_c;
  int unused_8;
  int unused_4;

  unused_18 = 0;
  unused_c = 0;
  unused_4 = 0;
  unused_8 = 0;
  unused_10 = 0;

  if ((slot > 0x10f) || (slot < 0x100))
  {
    return 0;
  }

  EnterCriticalSection(&g_sndCs);
  snd = g_sndSlots[slot];
  if (snd == 0)
  {
    LeaveCriticalSection(&g_sndCs);
    return 0;
  }
  if (((snd->flags >> 7) & 1) == 0)
  {
    LeaveCriticalSection(&g_sndCs);
    return 0xe;
  }
  if (snd->dsLockedPtr == 0)
  {
    LeaveCriticalSection(&g_sndCs);
    return 0xf;
  }
  snd->dsBuffer->lpVtbl->Unlock(snd->dsBuffer, snd->dsLockedPtr, snd->blockBytes, 0, 0);
  snd->flags &= 0xffffff7f;
  snd->dsLockedPtr = 0;
  LeaveCriticalSection(&g_sndCs);
  return 0;
}

// FUNCTION: MAGSND 0x10003656
int __cdecl ResetAndPrimeSndInstance(SndInstance *snd)
{
  MMIOINFO *mmioInfo;
  int result;

  snd->flags &= 0xfffffffe;
  snd->flags &= 0xfffffffd;
  snd->flags &= 0xffffffef;
  snd->flags &= 0xfffffffb;
  snd->writeCursorBytes = 0;
  snd->readCursorBytes = 0;
  snd->ringCursorBytes = 0;
  snd->timeBytes = 0;
  g_lastSndTime = 0;
  if ((snd->flags2 >> 5 & 1) != 0)
  {
    PrimeAviAudio(snd, 0);
  }
  else
  {
    mmioInfo = &snd->mmioInfo;
    mmioInfo->pchNext = mmioInfo->pchEndRead;

    mmioSetInfo(snd->hMmio, mmioInfo, 0);

    result = PrimeDsBufferFromSource(snd, 0);
    if (result != 0)
    {
      return result;
    }
  }
  snd->flags = (int)snd->flags | 0x20;
  return 0;
}

// FUNCTION: MAGSND 0x10003748
void __cdecl UpdateMmioSnd(SndInstance *snd)
{
  int silenceByte;
  DWORD playCursorBytes;
  void *lockPtr1;
  uint bufferedSourceBytes;
  uint silenceBytes;
  uint bytesUntilDataEnd;
  DWORD writeCursorBytes;
  uint pendingSourceBytes;
  uint sourceBytes;
  int hr;
  void *lockPtr2;
  DWORD lockBytes2;
  DWORD lockBytes1;

  writeCursorBytes = 0;
  playCursorBytes = 0;
  sourceBytes = 0;
  bufferedSourceBytes = 0;
  bytesUntilDataEnd = 0;
  pendingSourceBytes = 0;
  lockPtr1 = (void *)0x0;
  lockPtr2 = (void *)0x0;
  lockBytes1 = 0;
  lockBytes2 = 0;
  silenceBytes = 0;

  EnterCriticalSection(&g_sndCs);
  snd->dsBuffer->lpVtbl->GetCurrentPosition(snd->dsBuffer, &playCursorBytes, &writeCursorBytes);
  snd->timeBytes += (playCursorBytes - snd->timeBytes & 0xffff);
  if (snd->dataBytes < snd->timeBytes)
  {
    snd->timeBytes -= snd->dataBytes;
  }
  if ((((snd->flags >> 4) & 1) != 0 && (0x10000 <= snd->readCursorBytes)) ||
      (((snd->flags >> 1) & 1) != 0 && (snd->volume == 0)))
  {
    snd->dsBuffer->lpVtbl->Stop(snd->dsBuffer);
    if (g_updateTimerUsers > 0 && --g_updateTimerUsers == 0)
    {
      StopUpdateTimer();
    }
    snd->flags &= 0xfffffffe;
    snd->flags &= 0xfffffffd;
    snd->flags = (int)snd->flags | 4;
    snd->flags &= 0xffffffdf;
    if (((snd->flags2 >> 4) & 1) != 0)
    {
      g_sndSlots[snd->slotIndex]->flags &= 0xffffffbf;
      g_sndSlots[snd->slotIndex]->flags &= 0xfffffffe;
    }
    LeaveCriticalSection(&g_sndCs);
  }
  else
  {
    sourceBytes = (int)(playCursorBytes - snd->ringCursorBytes & 0xffff);
    bytesUntilDataEnd = snd->dataBytes - snd->writeCursorBytes;
    if (((snd->flags >> 4) & 1) == 0)
    {
      bufferedSourceBytes = snd->writeCursorBytes - snd->readCursorBytes;
      if (0x10000 < bufferedSourceBytes)
      {
        g_oversized_sound_buffer_count = g_oversized_sound_buffer_count + 1;
      }
    }
    else
    {
      bufferedSourceBytes = 0;
    }
    pendingSourceBytes = bytesUntilDataEnd + bufferedSourceBytes;
    if (pendingSourceBytes == 0)
    {
      silenceBytes = sourceBytes;
      sourceBytes = 0U;
    }
    if ((sourceBytes == 0) && (silenceBytes == 0))
    {
      LeaveCriticalSection(&g_sndCs);
    }
    else
    {
      if (bufferedSourceBytes < sourceBytes)
      {
        sourceBytes = bufferedSourceBytes;
      }
      if (silenceBytes + sourceBytes != 0)
      {
        hr = snd->dsBuffer->lpVtbl->Lock(snd->dsBuffer, snd->ringCursorBytes,
                                         silenceBytes + sourceBytes, &lockPtr1, &lockBytes1,
                                         &lockPtr2, &lockBytes2, 0);
        if (hr != 0)
        {
          LeaveCriticalSection(&g_sndCs);
          return;
        }
        snd->flags = (int)snd->flags | 0x80;
        if (silenceBytes == 0)
        {
          mmioGetInfo(snd->hMmio, &snd->mmioInfo, 0);
          memmove(lockPtr1, snd->mmioInfo.pchNext, lockBytes1);
          snd->mmioInfo.pchNext += lockBytes1;
          if (lockPtr2 != (void *)0x0)
          {
            memmove(lockPtr2, snd->mmioInfo.pchNext, lockBytes2);
            snd->mmioInfo.pchNext += lockBytes2;
          }
          mmioSetInfo(snd->hMmio, &snd->mmioInfo, 0);
          bufferedSourceBytes -= sourceBytes;
          pendingSourceBytes -= sourceBytes;
          snd->readCursorBytes += sourceBytes;
        }
        else
        {
          if (snd->waveFmt.wBitsPerSample == 8)
          {
            silenceByte = 0x80;
          }
          else
          {
            silenceByte = 0;
          }
          memset(lockPtr1, silenceByte, lockBytes1);
          if (lockPtr2 != (void *)0x0)
          {
            memset(lockPtr2, silenceByte, lockBytes2);
          }
          snd->readCursorBytes += silenceBytes;
        }
        snd->ringCursorBytes = (snd->ringCursorBytes + silenceBytes + sourceBytes) & 0xffff;
        snd->dsBuffer->lpVtbl->Unlock(snd->dsBuffer, lockPtr1, lockBytes1, lockPtr2, lockBytes2);
        snd->flags &= 0xffffff7f;
      }
      if (((pendingSourceBytes != 0) && (bytesUntilDataEnd > 0)) &&
          ((bufferedSourceBytes == 0 || (bufferedSourceBytes < sourceBytes * 2))))
      {
        mmioAdvance(snd->hMmio, &snd->mmioInfo, 0);
        if (bytesUntilDataEnd < snd->mmioInfo.cchBuffer - bufferedSourceBytes)
        {
          snd->writeCursorBytes += bytesUntilDataEnd;
        }
        else
        {
          snd->writeCursorBytes += snd->mmioInfo.cchBuffer - bufferedSourceBytes;
        }
      }
      if (pendingSourceBytes == 0)
      {
        if ((snd->flags2 & 1) != 0)
        {
          snd->mmioInfo.pchNext = snd->mmioInfo.pchEndRead;
          mmioSetInfo(snd->hMmio, &snd->mmioInfo, 0);
          mmioSeek(snd->hMmio, snd->mmioBaseOffset, 0);
          mmioGetInfo(snd->hMmio, &snd->mmioInfo, 0);
          mmioAdvance(snd->hMmio, &snd->mmioInfo, 0);
          snd->writeCursorBytes = (uint)(snd->mmioInfo.pchNext - snd->mmioInfo.pchBuffer);
          if (snd->dataBytes < snd->writeCursorBytes)
          {
            snd->writeCursorBytes = snd->dataBytes;
          }
          snd->readCursorBytes = 0;
        }
        else if (((snd->flags >> 4) & 1) == 0)
        {
          snd->ringCursorSavedBytes = snd->ringCursorBytes;
          snd->readCursorBytes = 0;
          snd->flags = (int)snd->flags | 0x10;
        }
      }
      LeaveCriticalSection(&g_sndCs);
      return;
    }
  }
}

// FUNCTION: MAGSND 0x10003D60
int __cdecl UpdateAviSnd(SndInstance *snd)
{
  uint aviStartSample;
  DWORD playCursorBytes;
  void *lockPtr1;
  uint unused_3c;
  uint unused_38;
  uint unused_34;
  uint playBlockIndex;
  uint ringBlockIndex;
  long aviBytesRead;
  DWORD writeCursorBytes;
  uint unused_20;
  uint previousRingBlockIndex;
  long aviSamplesRead;
  uint blockBytes;
  int hr;
  void *lockPtr2;
  DWORD lockBytes2;
  DWORD lockBytes1;

  writeCursorBytes = 0;
  playCursorBytes = 0;
  unused_3c = 0;
  unused_34 = 0;
  unused_20 = 0;
  lockPtr1 = (void *)0x0;
  lockPtr2 = (void *)0x0;
  lockBytes1 = 0;
  lockBytes2 = 0;
  unused_38 = 0;

  snd->dsBuffer->lpVtbl->GetCurrentPosition(snd->dsBuffer, &playCursorBytes, &writeCursorBytes);

  playBlockIndex = snd->timeBytes % snd->dsDesc.dwBufferBytes;
  if (playCursorBytes > playBlockIndex)
  {
    snd->timeBytes = snd->timeBytes + (playCursorBytes - playBlockIndex);
  }
  else
  {
    snd->timeBytes = snd->timeBytes + (snd->dsDesc.dwBufferBytes - playBlockIndex);
    snd->timeBytes = snd->timeBytes + playCursorBytes;
  }

  ringBlockIndex = playCursorBytes / snd->blockBytes;
  previousRingBlockIndex = snd->ringCursorBytes / snd->blockBytes;
  if (previousRingBlockIndex == ringBlockIndex)
  {
    return 0;
  }

  if (((((uint)snd->flags >> 4 & 1) != 0) && (snd->dsDesc.dwBufferBytes <= snd->readCursorBytes)) ||
      ((((uint)snd->flags >> 1 & 1) != 0) && (snd->volume == 0)))
  {
    snd->dsBuffer->lpVtbl->Stop(snd->dsBuffer);
    if (g_updateTimerUsers > 0 && --g_updateTimerUsers == 0)
    {
      StopUpdateTimer();
    }
    snd->flags = snd->flags & 0xfffffffe;
    snd->flags = snd->flags & 0xfffffffd;
    snd->flags = (int)snd->flags | 4;
    snd->flags = snd->flags & 0xffffffdf;
    snd->timeBytes = 0;
    return 0;
  }

  blockBytes = snd->blockBytes;
  if (((uint)snd->flags >> 4 & 1) == 0)
  {
    aviStartSample = snd->writeCursorBytes / snd->blockScale;
    hr = snd->dsBuffer->lpVtbl->Lock(snd->dsBuffer, snd->ringCursorBytes, blockBytes, &lockPtr1,
                                     &lockBytes1, &lockPtr2, &lockBytes2, 0);
    if (hr != 0)
    {
      return 10;
    }
    AVIStreamRead((PAVISTREAM)snd->streamOrMmio, (LONG)aviStartSample,
                  (LONG)(lockBytes1 / snd->blockScale), lockPtr1, (LONG)lockBytes1,
                  &aviBytesRead, &aviSamplesRead);
    if (lockPtr2 != (void *)0x0)
    {
      AVIStreamRead((PAVISTREAM)snd->streamOrMmio, (LONG)(aviSamplesRead + aviStartSample),
                    (LONG)(lockBytes2 / snd->blockScale), lockPtr2, (LONG)lockBytes2,
                    &aviBytesRead, &aviSamplesRead);
    }
    snd->ringCursorBytes = snd->ringCursorBytes + blockBytes;
    snd->ringCursorBytes = snd->ringCursorBytes % snd->dsDesc.dwBufferBytes;
    snd->writeCursorBytes = snd->writeCursorBytes + blockBytes;
    snd->readCursorBytes = snd->readCursorBytes + blockBytes;
    if ((uint)snd->dataBytes <= snd->writeCursorBytes)
    {
      snd->flags = (int)snd->flags | 0x10;
      snd->readCursorBytes = 0;
    }
    snd->dsBuffer->lpVtbl->Unlock(snd->dsBuffer, lockPtr1, lockBytes1, lockPtr2, lockBytes2);
    snd->flags = snd->flags & 0xffffff7f;
  }
  else
  {
    snd->readCursorBytes = snd->readCursorBytes + blockBytes;
  }

  return 0;
}

// FUNCTION: MAGSND 0x100040C9
undefined4 __cdecl AcquireFreeDsBuffer(SndInstance *snd, LPDIRECTSOUNDBUFFER *outBuffer)
{
  DWORD status;
  LPDIRECTSOUNDBUFFER dupOut;
  int hr;
  LPDIRECTSOUNDBUFFER buffer;
  int index;
  LPDIRECTSOUNDBUFFER candidate;

  buffer = snd->dsBuffer;
  candidate = buffer;
  hr = buffer->lpVtbl->GetStatus(buffer, &status);
  if (hr != 0)
  {
    return 9;
  }
  if ((status & 2) != 0)
  {
    return 9;
  }
  if ((status & 1) != 0)
  {
    for (index = 0; index < 0x10; index = index + 1)
    {
      buffer = snd->duplicateBuffers[index].buffer;
      if (buffer != NULL)
      {
        hr = buffer->lpVtbl->GetStatus(buffer, &status);
        if (hr != 0)
        {
          return 9;
        }
        if ((status & 1) != 0)
        {
          continue;
        }
        else
        {
          *outBuffer = buffer;
          return 0;
        }
      }
      else
      {
        hr = g_directSound->lpVtbl->DuplicateSoundBuffer(g_directSound, candidate, &dupOut);
        if (hr != 0)
        {
          return 9;
        }
        else
        {
          snd->duplicateBuffers[index].buffer = dupOut;
          *outBuffer = snd->duplicateBuffers[index].buffer;
          return 0;
        }
      }
    }
  }
  else
  {
    *outBuffer = candidate;
    return 0;
  }
  return 9;
}

// FUNCTION: MAGSND 0x10004239
void __cdecl AddToUpdateList(SndInstance *snd)

{
  if (g_updateSndHead == (SndInstance *)0x0)
  {
    g_updateSndHead = snd;
    g_updateSndTail = g_updateSndHead;
  }
  else
  {
    g_updateSndTail->updateNext = snd;
    snd->updatePrev = g_updateSndTail;
    g_updateSndTail = snd;
  }
  return;
}

// FUNCTION: MAGSND 0x10004292
void __cdecl RemoveFromUpdateList(SndInstance *snd)

{
  SndInstance *prev;
  SndInstance *next;

  prev = (SndInstance *)0x0;
  next = (SndInstance *)0x0;
  prev = snd->updatePrev;
  next = snd->updateNext;
  if (prev != (SndInstance *)0x0)
  {
    prev->updateNext = next;
  }
  else
  {
    g_updateSndHead = next;
  }
  if (next != (SndInstance *)0x0)
  {
    next->updatePrev = prev;
  }
  else
  {
    g_updateSndTail = prev;
  }
  return;
}

// FUNCTION: MAGSND 0x10004311
void __cdecl AddToActiveList(SndInstance *snd)

{
  if (g_activeSndListHead == (SndInstance *)0x0)
  {
    g_activeSndListHead = snd;
    g_activeSndTail = g_activeSndListHead;
  }
  else
  {
    g_activeSndTail->activeNext = snd;
    snd->activePrev = g_activeSndTail;
    g_activeSndTail = snd;
  }
  return;
}

// FUNCTION: MAGSND 0x1000436A
void __cdecl RemoveFromActiveList(SndInstance *snd)

{
  SndInstance *prev;
  SndInstance *next;

  prev = (SndInstance *)0x0;
  next = (SndInstance *)0x0;
  prev = snd->activePrev;
  next = snd->activeNext;
  if (prev != (SndInstance *)0x0)
  {
    prev->activeNext = next;
  }
  else
  {
    g_activeSndListHead = next;
  }
  if (next != (SndInstance *)0x0)
  {
    next->activePrev = prev;
  }
  else
  {
    g_activeSndTail = prev;
  }
  return;
}

// FUNCTION: MAGSND 0x100043E9
void CALLBACK UpdateTimerProc(UINT u1, UINT u2, DWORD dw1, DWORD dw2, DWORD dw3)

{
  (void)u1;
  (void)u2;
  (void)dw1;
  (void)dw2;
  (void)dw3;
  UpdateSnd();
  return;
}

// FUNCTION: MAGSND 0x10004400
undefined4 __cdecl StartUpdateTimer(undefined4 resolutionMs)
{
  (void)resolutionMs;
  if (timeGetDevCaps(&g_timeCaps, 8) != 0)
  {
    return 0xc;
  }
  timeBeginPeriod(g_updatePeriodMs);
  g_updateTimerId = timeSetEvent(g_updatePeriodMs, g_timeCaps.wPeriodMin, UpdateTimerProc, (DWORD)&g_sndHwnd, 1);
  if (g_updateTimerId == 0)
  {
    timeEndPeriod(g_updatePeriodMs);
    return 0xc;
  }
  g_updateTimerActive = 1;
  return 0;
}

// FUNCTION: MAGSND 0x1000448D
void StopUpdateTimer(void)

{
  if (g_updateTimerActive != 0)
  {
    EnterCriticalSection(&g_sndCs);
    timeKillEvent(g_updateTimerId);
    g_updateTimerId = 0;
    timeEndPeriod(g_updatePeriodMs);
    g_updateTimerActive = 0;
    LeaveCriticalSection(&g_sndCs);
  }
  return;
}

// FUNCTION: MAGSND 0x100044F0
undefined4 __cdecl LoadFromFile(char *filename, SndInstance **outSnd)

{
  FILE *file;
  undefined4 status;

  file = fopen(filename, "rb");
  if (file == (FILE *)0x0)
  {
    return 7;
  }
  status = LoadWaveFromFileHandle(file, outSnd);
  fclose(file);
  return status;
}

// FUNCTION: MAGSND 0x1000454F
undefined4 __cdecl LoadWaveMmio(LPSTR filename, SndInstance **outSnd, undefined4 dwFlags)
{
  typedef struct SndInstanceSlice_t
  {
    MMCKINFO mmck;              /* 0x01c */
    uint markerCount;           /* 0x030 */
    SndInstance *markers[0x11]; /* 0x034 */
  } SndInstanceSlice;

  SndInstanceSlice *sndInstanceHacked;
  MMCKINFO riffChunk;
  PaddedWaveFormat riffHeader; // ebp - 0x80
  MMIOINFO pmmioinfo;
  undefined4 unused;   // ebp - 0x24
  DWORD riffChunkSize; // ebp - 0x20
  LONG mmioBaseOffset; // ebp - 0x1c
  MMCKINFO riff;       // ebp - 0x18
  HMMIO fileHandle;    // ebp - 0x4

  unused = 0;
  memset(&pmmioinfo, 0, sizeof(MMIOINFO));
  pmmioinfo.cchBuffer = 0x10000;
  fileHandle = mmioOpenA(filename, &pmmioinfo, 0x10000);
  if (fileHandle == (HMMIO)0x0)
  {
    return 7;
  }
  riff.fccType = 0x45564157;
  if (mmioDescend(fileHandle, &riff, (MMCKINFO *)0x0, MMIO_FINDRIFF) != 0)
  {
    mmioClose(fileHandle, 0);
    return 8;
  }
  riffChunk.ckid = 0x20746d66;
  if (mmioDescend(fileHandle, &riffChunk, &riff, MMIO_FINDCHUNK) != 0)
  {
    mmioClose(fileHandle, 0);
    return 8;
  }
  riffChunkSize = riffChunk.cksize;
  if (mmioRead(fileHandle, (HPSTR)&riffHeader, riffChunkSize) != riffChunkSize)
  {
    mmioClose(fileHandle, 0);
    return 8;
  }

  riffHeader.fmt.cbSize = 0;
  mmioAscend(fileHandle, &riffChunk, 0);
  riffChunk.ckid = 0x61746164;
  if (mmioDescend(fileHandle, &riffChunk, &riff, 0x10) != 0)
  {
    mmioClose(fileHandle, 0);
    return 8;
  }
  mmioGetInfo(fileHandle, &pmmioinfo, 0);
  mmioAdvance(fileHandle, &pmmioinfo, 0);
  mmioBaseOffset = pmmioinfo.lBufOffset;
  dwFlags = 0xe8;
  *outSnd = CreateSndInstance(g_directSound, 0x10000, &riffHeader.fmt, dwFlags);
  if (*outSnd != (SndInstance *)0x0)
  {
    (*outSnd)->mmioBaseOffset = mmioBaseOffset;
    (*outSnd)->hMmio = fileHandle;
    (*outSnd)->mmioInfo = pmmioinfo;
    (*outSnd)->dataBytes = riffChunk.cksize;
    (*outSnd)->streamOrMmio = operator_new(strlen(filename) + 1);
    if ((*outSnd)->streamOrMmio != (void *)0x0)
    {
      strcpy((char *)(*outSnd)->streamOrMmio, filename);
    }
    else
    {
      mmioClose(fileHandle, 0);
      return 3;
    }
    (*outSnd)->flags2 = (*outSnd)->flags2 & 0xffffffdf;
    (*outSnd)->flags2 = (int)(*outSnd)->flags2 | 2;
    (*outSnd)->blockBytes = 0;
    (*outSnd)->blockScale = 0;
    (*outSnd)->blockCount = 0;
  }
  else
  {
    mmioClose(fileHandle, 0);
    return 9;
  }
  mmioAscend(fileHandle, &riffChunk, 0);
  riffChunk.ckid = 0x20657563;

  if (mmioDescend(fileHandle, &riffChunk, &riff, 0x10) == 0)
  {
    // TODO: this is fuckin weird no?
    sndInstanceHacked = (SndInstanceSlice *)&(*outSnd)->mmck;
    memcpy(sndInstanceHacked, &riffChunk, sizeof(MMCKINFO));
    if (mmioRead(fileHandle, (HPSTR)&sndInstanceHacked->markerCount, 4) != 4)
    {
      DestroySndInstance(*outSnd);
      mmioClose(fileHandle, 0);
      return 8;
    }
    sndInstanceHacked->markers[0] = operator_new(sndInstanceHacked->markerCount * 0x18);
    if (sndInstanceHacked->markers[0] == (void *)0x0)
    {
      DestroySndInstance(*outSnd);
      mmioClose(fileHandle, 0);
      return 3;
    }
    riffChunkSize = sndInstanceHacked->markerCount * 0x18;
    if (mmioRead(fileHandle, (HPSTR)sndInstanceHacked->markers[0], riffChunkSize) != (LONG)riffChunkSize)
    {
      DestroySndInstance(*outSnd);
      mmioClose(fileHandle, 0);
      MAGSND_DELETE(sndInstanceHacked->markers[0]);
      return 8;
    }
  }
  return 0;
}

// FUNCTION: MAGSND 0x10004986
void __cdecl CloseMmioAndFreeSource(SndInstance *snd)
{
  void *tmp4;
  void *tmp3;
  void *tmp2;
  void *tmp1;

  mmioClose(snd->hMmio, 0);
  if (snd->markers[0] != 0)
  {
    tmp1 = snd->markers[0];
    tmp3 = tmp1;
    operator_delete(tmp3);
  }
  if (((snd->flags2 >> 5) & 1) == 0)
  {
    tmp2 = snd->streamOrMmio;
    tmp4 = tmp2;
    operator_delete(tmp4);
  }
  return;
}

// FUNCTION: MAGSND 0x100049FE
undefined4 __cdecl LoadWaveFromFileHandle(FILE *file, SndInstance **outSnd)
{
  undefined4 waveFmt[4];
  undefined2 cbSize;
  void *ptr1; // ebp - 0x38
  int seek0;
  int riffEnd;
  int riffTag;
  int riffSize;
  int waveTag;
  void *ptr2;   // ebp - 0x20
  DWORD bytes2; // ebp - 0x1c
  int chunkHdr[2];
  HRESULT lockHr;
  DWORD bytes1;    // ebp - 0xc
  int bytesRead;   // ebp - 0x8
  int createFlags; // ebp - 0x4

  ptr1 = (void *)0x0;
  ptr2 = (void *)0x0;
  bytes1 = 0;
  bytes2 = 0;
  bytesRead = 0;
  createFlags = 0;
  riffEnd = 0;
  seek0 = 0;

  fseek(file, seek0, 0);

  // This reads riffTag, riffSize, and waveTag
  fread(&riffTag, 1, 0xc, file);
  riffEnd = riffSize + seek0 + 8;
  if ((riffTag != 0x46464952) || (waveTag != 0x45564157))
  {
    return 8;
  }

  if (FindRiffChunk(file, 0x20746d66, seek0, (uint)riffEnd) == 0)
  {
    return 8;
  }
  fread(chunkHdr, 1, 8, file);
  fread(waveFmt, 1, 0x10, file);
  cbSize = 0;
  if (FindRiffChunk(file, 0x61746164, seek0, (uint)riffEnd) == 0)
  {
    return 8;
  }
  fread(chunkHdr, 1, 8, file);
  createFlags = 0xea;

  *outSnd = CreateSndInstance(g_directSound, chunkHdr[1], (WAVEFORMATEX *)waveFmt, createFlags);
  //*outSnd = bytes1;
  if (*outSnd == 0)
  {
    return 9;
  }

  lockHr = (*outSnd)->dsBuffer->lpVtbl->Lock((*outSnd)->dsBuffer, 0, chunkHdr[1], &ptr1, &bytes1, &ptr2, &bytes2, 0);
  if (lockHr != 0)
  {
    DestroySndInstance(*outSnd);
    return 9;
  }
  bytesRead = fread(ptr1, 1, bytes1, file);
  if (bytesRead != bytes1)
  {
    DestroySndInstance(*outSnd);
    return 9;
  }
  lockHr = (*outSnd)->dsBuffer->lpVtbl->Unlock((*outSnd)->dsBuffer, ptr1, bytes1, ptr2, bytes2);
  if (lockHr != 0)
  {
    (*outSnd)->dsBuffer->lpVtbl->Release((*outSnd)->dsBuffer);
    MAGSND_DELETE(*outSnd);
    return 9;
  }
  return 0;
}

// FUNCTION: MAGSND 0x10004C8A
undefined4 __cdecl SeekMmioToSample(SndInstance *sndObj, int sampleIndex)
{
  int seekDelta;
  int currentOffset;
  int targetOffset;

  mmioGetInfo(sndObj->hMmio, (LPMMIOINFO)&sndObj->mmioInfo, 0);
  targetOffset = sndObj->waveFmt.nBlockAlign * sampleIndex + sndObj->mmioBaseOffset;
  currentOffset = MmioTellFromInfo(&sndObj->mmioInfo);
  seekDelta = targetOffset - currentOffset;
  mmioSeek(sndObj->hMmio, seekDelta, 1);
  mmioGetInfo(sndObj->hMmio, (LPMMIOINFO)&sndObj->mmioInfo, 0);
  mmioAdvance(sndObj->hMmio, (LPMMIOINFO)&sndObj->mmioInfo, 0);
  return 0;
}

// FUNCTION: MAGSND 0x10004D3F
undefined4 __cdecl FindRiffChunk(FILE *file, int chunkId, long searchStart, uint searchEnd)
{
  int chunk[2];
  int pos;
  int hdr[3];

  fseek(file, searchStart, 0);
  fread(hdr, 1, 0xc, file);
  if (hdr[0] == chunkId)
  {
    fseek(file, searchStart, 0);
    return 1;
  }
  while (1)
  {
    pos = ftell(file);
    fread(chunk, 1, 8, file);
    if (searchEnd <= (uint)pos)
    {
      fseek(file, searchStart, 0);
      return 0;
    }
    if (chunk[0] == chunkId)
    {
      break;
    }

    pos = ftell(file);
    if ((chunk[1] & 1) != 0)
    {
      chunk[1] = chunk[1] + 1;
    }
    fseek(file, chunk[1], 1);
  }

  fseek(file, pos, 0);
  return 1;
}

// FUNCTION: MAGSND 0x10004E4C
undefined4 __cdecl PrimeDsBufferFromSource(SndInstance *snd, int startSample)
{
  int bytesRemainingInData; // ebp - 0x48
  int samplesReadAdvance;   // ebp - 0x44
  LONG blockAlignCopy;      // ebp - 0x40
  HPSTR lockPtr1;           // ebp - 0x3c
  int startOffsetBytes;     // ebp - 0x38
  int done;                 // ebp - 0x34
  int streamSampleCount;    // ebp - 0x30
  int bytesRemainingToFill; // ebp - 0x2c
  HPSTR writePtr;           // ebp - 0x28
  int sourceTotalBytes;     // ebp - 0x24
  int wrapWindowBytes;      // ebp - 0x20
  int sourceOffsetBytes;    // ebp - 0x1c
  HPSTR lockPtr2;           // ebp - 0x18
  DWORD lockSize2;          // ebp - 0x14
  int lockHr;               // ebp - 0x10
  LONG bytesRead;           // ebp - 0xc
  int streamByteOffset;     // ebp - 0x8
  DWORD lockSize1;          // ebp - 0x4

  lockPtr1 = (HPSTR)0x0;
  lockPtr2 = (HPSTR)0x0;
  writePtr = (HPSTR)0x0;
  lockSize1 = 0;
  lockSize2 = 0;
  streamSampleCount = 0;
  done = 0;
  if (((snd->flags2 >> 5) & 1) == 0)
  {
    SeekMmioToSample(snd, startSample);
  }

  startOffsetBytes = (uint)snd->waveFmt.nBlockAlign * startSample;
  sourceOffsetBytes = startOffsetBytes;
  sourceTotalBytes = snd->dataBytes;
  bytesRemainingInData = sourceTotalBytes - sourceOffsetBytes;
  bytesRemainingToFill = 0x10000;
  samplesReadAdvance = 0;
  streamByteOffset = startOffsetBytes;

  if ((startOffsetBytes < 0) || (sourceTotalBytes < startOffsetBytes))
  {
    return 5;
  }

  lockHr = snd->dsBuffer->lpVtbl->Lock(snd->dsBuffer, 0, 0x10000, (LPVOID *)&lockPtr1, &lockSize1, (LPVOID *)&lockPtr2, &lockSize2, 0);
  if (lockHr != 0)
  {
    if (((snd->flags2 >> 5) & 1) != 0)
    {
      CleanupAviStream(snd);
    }
    else
    {
      CloseMmioAndFreeSource(snd);
    }
    DestroySndInstance(snd);
    return 9;
  }

  writePtr = lockPtr1;
  while (done == 0)
  {
    if (bytesRemainingInData >= bytesRemainingToFill)
    {
      if (((snd->flags2 >> 5) & 1) != 0)
      {
        AVIStreamRead((PAVISTREAM)snd->streamOrMmio, startSample, 0x10000 / snd->waveFmt.nBlockAlign, writePtr, 0x10000, &bytesRead,
                      &blockAlignCopy);
      }
      else
      {
        bytesRead = mmioRead(snd->hMmio, writePtr, bytesRemainingToFill);
      }
      bytesRemainingToFill = bytesRemainingToFill - bytesRead;
      samplesReadAdvance += bytesRead;
      streamByteOffset += bytesRead;
      bytesRemainingInData = bytesRemainingInData - bytesRead;
      sourceOffsetBytes += bytesRead;
      startSample = startSample + blockAlignCopy;
      done = 1;
    }
    else
    {
      if (((snd->flags2 >> 5) & 1) != 0)
      {
        AVIStreamRead((PAVISTREAM)snd->streamOrMmio, startSample, 0x10000 / snd->waveFmt.nBlockAlign, writePtr, bytesRemainingInData, &bytesRead, 0);
      }
      else
      {
        bytesRead = mmioRead(snd->hMmio, writePtr, bytesRemainingInData);
      }
      writePtr = writePtr + bytesRead;
      bytesRemainingToFill = bytesRemainingToFill - bytesRead;
      bytesRemainingInData = 0;
      sourceOffsetBytes = snd->dataBytes;
      if ((snd->flags2 & 1) == 0)
      {
        memset(writePtr, 0, (size_t)bytesRemainingToFill);
        done = 1;
      }
      else
      {
        if (((snd->flags2 >> 5) & 1) == 0)
        {
          SeekMmioToSample(snd, 0);
        }
        sourceOffsetBytes = 0;
        bytesRemainingInData = snd->dataBytes;
        streamByteOffset = 0;
        samplesReadAdvance = 0;
      }
    }
  }

  mmioGetInfo(snd->hMmio, &snd->mmioInfo, 0);
  if (((snd->flags2 >> 5) & 1) != 0)
  {
    RefillAviMmioBuffer(snd, startSample);
  }
  else
  {
    mmioAdvance(snd->hMmio, &snd->mmioInfo, 0);
  }

  wrapWindowBytes = min(0x10000, bytesRemainingInData);
  snd->writeCursorBytes = snd->dataBytes - (bytesRemainingInData - wrapWindowBytes);
  snd->readCursorBytes = snd->dataBytes - bytesRemainingInData;
  snd->ringCursorBytes = 0;

  lockHr = snd->dsBuffer->lpVtbl->Unlock(snd->dsBuffer, lockPtr1, lockSize1, lockPtr2, lockSize2);
  if (lockHr != 0)
  {
    CloseMmioAndFreeSource(snd);
    DestroySndInstance(snd);
    return 9;
  }
  return 0;
}

// FUNCTION: MAGSND 0x1000524C
undefined4 __cdecl PrimeAviAudio(SndInstance *snd, int unused)
{
  HPSTR lockPtr1;
  uint blockIndex;
  LONG aviBytesRead;
  uint samplesPerBlock;
  int lockBytesTotal;
  LONG aviSamplesRead;
  HPSTR writePtr;
  HPSTR lockPtr2;
  DWORD lockBytes2;
  int streamSample;
  DWORD lockBytes1;
  int hr;

  lockBytesTotal = snd->blockCount * snd->blockBytes;
  hr = snd->dsBuffer->lpVtbl->Lock(snd->dsBuffer, 0, lockBytesTotal, (LPVOID *)&lockPtr1, &lockBytes1, (LPVOID *)&lockPtr2, &lockBytes2, 0);
  if (hr != 0)
  {
    CleanupAviStream(snd);
    DestroySndInstance(snd);
    return 9;
  }
  if (lockPtr2 != 0)
  {
    snd->dsBuffer->lpVtbl->Unlock(snd->dsBuffer, lockPtr1, lockBytes1, lockPtr2, lockBytes2);
    CleanupAviStream(snd);
    DestroySndInstance(snd);
    return 9;
  }
  snd->ringCursorBytes = 0;
  snd->writeCursorBytes = 0;
  snd->readCursorBytes = 0;
  writePtr = lockPtr1;
  streamSample = 0;
  samplesPerBlock = snd->blockBytes / snd->blockScale;
  // hr = 0;
  for (blockIndex = 0; blockIndex < snd->blockCount; blockIndex = blockIndex + 1)
  {
    AVIStreamRead((PAVISTREAM)snd->streamOrMmio, streamSample, samplesPerBlock, (void *)writePtr, snd->blockBytes, &aviBytesRead,
                  &aviSamplesRead);
    snd->ringCursorBytes = snd->ringCursorBytes + snd->blockBytes;
    snd->ringCursorBytes = snd->ringCursorBytes % snd->dsDesc.dwBufferBytes;
    snd->writeCursorBytes = snd->writeCursorBytes + snd->blockBytes;
    snd->readCursorBytes = snd->readCursorBytes + snd->blockBytes;
    writePtr += (int)snd->blockBytes;
    streamSample = streamSample + samplesPerBlock;
  }
  hr = snd->dsBuffer->lpVtbl->Unlock(snd->dsBuffer, lockPtr1, lockBytes1, lockPtr2, lockBytes2);
  if (hr != 0)
  {
    CloseMmioAndFreeSource((SndInstance *)snd);
    DestroySndInstance((SndInstance *)snd);
    return 9;
  }
  return 0;
}

// FUNCTION: MAGSND 0x10005480
SndInstance *__cdecl CreateSndInstance(IDirectSound *dSound, DWORD dwBufferBytes, WAVEFORMATEX *wfxFormat, DWORD dwFlags)
{
  void *deletePtr2;
  void *deletePtr1;
  int hr;
  SndInstance *result;

  result = (SndInstance *)operator_new(sizeof(SndInstance));
  memset(result, 0, sizeof(SndInstance));
  result->dsDesc.dwSize = 0x14;
  result->dsDesc.dwFlags = dwFlags;
  result->dsDesc.dwBufferBytes = dwBufferBytes;
  result->dsDesc.lpwfxFormat = wfxFormat;
  memcpy(&result->waveFmt, wfxFormat, sizeof(WAVEFORMATEX));

  hr = dSound->lpVtbl->CreateSoundBuffer(dSound, &result->dsDesc, &result->dsBuffer, 0);
  if (hr != 0)
  {
    deletePtr1 = result;
    deletePtr2 = deletePtr1;
    operator_delete(deletePtr2);
    return (SndInstance *)0x0;
  }
  else
    return result;
}

// FUNCTION: MAGSND 0x10005562
undefined4 __cdecl DestroySndInstance(SndInstance *snd)
{
  if (snd == (SndInstance *)0x0)
  {
    return 5;
  }

  snd->dsBuffer->lpVtbl->Release(snd->dsBuffer);
  MAGSND_DELETE(snd);
  return 0;
}

// FUNCTION: MAGSND 0x100055BB
undefined4 __cdecl LoadAviAudioStream(void *stream, SndInstance **outSnd, undefined4 scratch)

{
  PaddedWaveFormat waveFmt;
  MMIOINFO mmioInfo;
  LONG firstReadBytes;
  int bufferBytes;
  int dataBytes;
  int formatSize;
  AVISTREAMINFOA streamInfo;

  formatSize = 0x12;
  AVIStreamInfoA((PAVISTREAM)stream, &streamInfo, 0x8c);
  if (streamInfo.fccType != 0x73647561)
  {
    return 8;
  }
  AVIStreamRead((PAVISTREAM)stream, 0, 1, 0, 0, &firstReadBytes, 0);
  dataBytes = (int)streamInfo.dwSampleSize * (int)streamInfo.dwLength;
  AVIStreamReadFormat((PAVISTREAM)stream, 0, &waveFmt.fmt, (LONG *)&formatSize);
  waveFmt.fmt.cbSize = 0;
  scratch = 0xe8;
  bufferBytes = (int)streamInfo.dwSuggestedBufferSize * 0xb;
  *outSnd = CreateSndInstance(g_directSound, bufferBytes, &waveFmt.fmt, scratch);
  if (*outSnd != (SndInstance *)0x0)
  {
    (*outSnd)->mmioBaseOffset = 0;
    (*outSnd)->hMmio = (HMMIO)0;
    (*outSnd)->mmioInfo = mmioInfo;
    (*outSnd)->dataBytes = dataBytes;
    (*outSnd)->streamOrMmio = stream;
    (*outSnd)->flags2 = (int)(*outSnd)->flags2 | 0x20;
    (*outSnd)->flags2 = (int)(*outSnd)->flags2 | 2;
    (*outSnd)->blockBytes = streamInfo.dwSuggestedBufferSize;
    (*outSnd)->blockScale = streamInfo.dwSampleSize;
    (*outSnd)->blockCount = 0xb;
    (*outSnd)->timeBytes = 0;
  }
  else
    return 9;
  return 0;
}

// FUNCTION: MAGSND 0x10005770
void __cdecl RefillAviMmioBuffer(SndInstance *sndObj, long streamStartSample)
{
  int unused_1c;
  int bytesToRead;
  int unused_14;
  int bytesRemainingInData;
  int unused_c;
  long aviBytesRead;
  size_t bufferedBytes;

  aviBytesRead = 0;
  unused_c = 0;
  unused_14 = 0;
  unused_1c = 0;
  bufferedBytes = sndObj->writeCursorBytes - sndObj->readCursorBytes;
  if ((sndObj->mmioInfo.pchEndRead - sndObj->mmioInfo.pchNext) != bufferedBytes)
  {
    g_mmioInfoMismatchCount = g_mmioInfoMismatchCount + 1;
  }
  bytesRemainingInData = sndObj->dataBytes - sndObj->writeCursorBytes;
  bytesToRead = sndObj->mmioInfo.cchBuffer - bufferedBytes;
  if (bytesToRead > bytesRemainingInData)
  {
    bytesToRead = bytesRemainingInData;
  }
  if (bufferedBytes != 0)
  {
    memmove((void *)sndObj->mmioInfo.pchBuffer, (void *)sndObj->mmioInfo.pchNext, bufferedBytes);
  }
  sndObj->mmioInfo.pchNext = &sndObj->mmioInfo.pchBuffer[bufferedBytes];
  AVIStreamRead((PAVISTREAM)sndObj->streamOrMmio, streamStartSample,
                bytesToRead / (int)(uint)sndObj->waveFmt.nBlockAlign, sndObj->mmioInfo.pchNext, bytesToRead,
                &aviBytesRead, 0);
  sndObj->mmioInfo.pchEndRead = sndObj->mmioInfo.pchBuffer + sndObj->mmioInfo.cchBuffer;
  sndObj->mmioInfo.pchNext = sndObj->mmioInfo.pchBuffer;
  mmioSetInfo(sndObj->hMmio, &sndObj->mmioInfo, 0);
  return;
}

// FUNCTION: MAGSND 0x100058D0
int __cdecl MmioTellFromInfo(MMIOINFO *info) { return (int)info->lDiskOffset - ((int)info->pchEndRead - (int)info->pchNext); }

// FUNCTION: MAGSND 0x1000575E
undefined4 __cdecl CleanupAviStream(SndInstance *snd)
{
  (void)snd;
  return 0;
}

long __cdecl ftol(void)
{
  return 0;
}

void *__cdecl operator_new(unsigned int size)
{
  return malloc(size);
}

void __cdecl operator_delete(void *p)
{
  if (p)
    free(p);
}
