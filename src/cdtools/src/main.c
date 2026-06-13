#include "cdtools.h"
#include <stdio.h>
#include <winnt.h>
#include <winreg.h>
#include "mystdbool.h"

// GLOBAL: CDTOOLS 0x100123cc
HINSTANCE g_cdtoolsInstance;

// GLOBAL: CDTOOLS 0x100123d0
DWORD g_originalNoDriveTypeAutoRun;

// GLOBAL: CDTOOLS 0x100123d4
undefined4 g_queryCancelAutoplayMsg;

// FUNCTION: CDTOOLS 0x10001000
undefined4 __cdecl CheckOriginalCD(undefined4 unused, int *maxTries, undefined4 drivePath)
{
  HANDLE hFile;                 /* EBP - 0x330 */
  CHAR volumeName[0x104];       /* EBP - 0x32c */
  CHAR fileSystemName[0x104];   /* EBP - 0x228 */
  int result;                   /* EBP - 0x124 */
  int tries;                    /* EBP - 0x120 */
  int found;                    /* EBP - 0x11c */
  DWORD volumeSerial;           /* EBP - 0x118 */
  DWORD maximumComponentLength; /* EBP - 0x114 */
  DWORD fileSystemFlags;        /* EBP - 0x110 */
  DWORD fileSize;               /* EBP - 0x10c */
  CHAR rootPath[0x108];         /* EBP - 0x108 */

  for (found = 0, tries = 0; *maxTries > tries && found == 0; tries++)
  {
    result = IsCDDrive(drivePath);
    if (result != 0)
    {
      sprintf(rootPath, "%s\\", drivePath);
      result = GetVolumeInformationA(rootPath, volumeName, 0x104, &volumeSerial,
                                     &maximumComponentLength, &fileSystemFlags,
                                     fileSystemName, 0x104);
      volumeName[3] = 0;
      if (strcmp(volumeName, "MTG") == 0)
      {
        found = 1;
      }
      else
      {
        Sleep(1000);
      }
    }
    else
    {
      Sleep(1000);
    }
  }

  if (found == 0)
  {
    return 0;
  }

  sprintf(rootPath, "%s\\%s", drivePath, "DuelSounds\\Manaball.wav");
  hFile = CreateFileA(rootPath, 0x80000000, 1, (LPSECURITY_ATTRIBUTES)0x0, 3, 0x80, (HANDLE)0x0);
  if (hFile != (HANDLE)-1)
  {
    CloseHandle(hFile);
    return 0;
  }

  sprintf(rootPath, "%s\\%s", drivePath, "Sound\\Locmus15.wav");
  hFile = CreateFileA(rootPath, 0x80000000, 1, (LPSECURITY_ATTRIBUTES)0x0, 3, 0x80, (HANDLE)0x0);
  if (hFile == (HANDLE)-1)
  {
    return 0;
  }

  fileSize = GetFileSize(hFile, (LPDWORD)0x0);
  if (fileSize != 0x307864)
  {
    CloseHandle(hFile);
    return 0;
  }

  CloseHandle(hFile);
  return 1;
}

// FUNCTION: CDTOOLS 0x10001234
undefined4 __cdecl CheckDoPCD(undefined4 unused, int *maxTries, undefined4 drivePath)
{
  HANDLE hFile;                 /* EBP - 0x330 */
  CHAR volumeName[0x104];       /* EBP - 0x32c */
  CHAR fileSystemName[0x104];   /* EBP - 0x228 */
  int result;                   /* EBP - 0x124 */
  int tries;                    /* EBP - 0x120 */
  int found;                    /* EBP - 0x11c */
  DWORD volumeSerial;           /* EBP - 0x118 */
  DWORD maximumComponentLength; /* EBP - 0x114 */
  DWORD fileSystemFlags;        /* EBP - 0x110 */
  DWORD fileSize;               /* EBP - 0x10c */
  CHAR rootPath[0x108];         /* EBP - 0x108 */

  for (found = 0, tries = 0; *maxTries > tries && found == 0; tries++)
  {
    result = IsCDDrive(drivePath);
    if (result != 0)
    {
      sprintf(rootPath, "%s\\", drivePath);
      result = GetVolumeInformationA(rootPath, volumeName, 0x104, &volumeSerial,
                                     &maximumComponentLength, &fileSystemFlags,
                                     fileSystemName, 0x104);
      volumeName[3] = 0;
      if (strcmp(volumeName, "MTG") == 0)
      {
        found = 1;
      }
      else
      {
        Sleep(1000);
      }
    }
    else
    {
      Sleep(1000);
    }
  }

  if (found == 0)
  {
    return 0;
  }

  sprintf(rootPath, "%s\\%s", drivePath, "DuelSounds\\Deep.wav");
  hFile = CreateFileA(rootPath, 0x80000000, 1, (LPSECURITY_ATTRIBUTES)0x0, 3, 0x80, (HANDLE)0x0);
  if (hFile == (HANDLE)-1)
  {
    return 0;
  }

  CloseHandle(hFile);

  sprintf(rootPath, "%s\\%s", drivePath, "Sound\\Locmus15.wav");
  hFile = CreateFileA(rootPath, 0x80000000, 1, (LPSECURITY_ATTRIBUTES)0x0, 3, 0x80, (HANDLE)0x0);
  if (hFile == (HANDLE)-1)
  {
    return 0;
  }

  fileSize = GetFileSize(hFile, (LPDWORD)0x0);
  if (fileSize != 0x307864)
  {
    CloseHandle(hFile);
    return 0;
  }

  CloseHandle(hFile);
  return 1;
}

// FUNCTION: CDTOOLS 0x10001468
undefined4 __cdecl CheckSotaCD(undefined4 unused, int *maxTries, undefined4 drivePath)

{
  HANDLE hFile;                 /* EBP - 0x330 */
  CHAR volumeName[0x104];       /* EBP - 0x32c */
  CHAR fileSystemName[0x104];   /* EBP - 0x228 */
  int result;                   /* EBP - 0x124 */
  int tries;                    /* EBP - 0x120 */
  int found;                    /* EBP - 0x11c */
  DWORD volumeSerial;           /* EBP - 0x118 */
  DWORD maximumComponentLength; /* EBP - 0x114 */
  DWORD fileSystemFlags;        /* EBP - 0x110 */
  DWORD fileSize;               /* EBP - 0x10c */
  CHAR rootPath[0x108];         /* EBP - 0x108 */

  for (found = 0, tries = 0; *maxTries > tries && found == 0; tries++)
  {
    result = IsCDDrive(drivePath);
    if (result != 0)
    {
      sprintf(rootPath, "%s\\", drivePath);
      result = GetVolumeInformationA(rootPath, volumeName, 0x104, &volumeSerial,
                                     &maximumComponentLength, &fileSystemFlags,
                                     fileSystemName, 0x104);
      volumeName[3] = 0;
      if (strcmp(volumeName, "MTG") == 0)
      {
        found = 1;
      }
      else
      {
        Sleep(1000);
      }
    }
    else
    {
      Sleep(1000);
    }
  }

  if (found == 0)
  {
    return 0;
  }

  sprintf(rootPath, "%s\\%s", drivePath, "DuelSounds\\Manaball.wav");
  hFile = CreateFileA(rootPath, 0x80000000, 1, (LPSECURITY_ATTRIBUTES)0x0, 3, 0x80, (HANDLE)0x0);
  if (hFile == (HANDLE)-1)
  {
    return 0;
  }

  CloseHandle(hFile);
  sprintf(rootPath, "%s\\%s", drivePath, "Sound\\Locmus15.wav");
  hFile = CreateFileA(rootPath, 0x80000000, 1, (LPSECURITY_ATTRIBUTES)0x0, 3, 0x80, (HANDLE)0x0);
  if (hFile == (HANDLE)-1)
  {
    return 0;
  }

  fileSize = GetFileSize(hFile, (LPDWORD)0x0);
  if (fileSize != 0x307864)
  {
    CloseHandle(hFile);
    return 0;
  }

  CloseHandle(hFile);
  return 1;
}

// FUNCTION: CDTOOLS 0x1000169C
bool __cdecl IsCDDrive(undefined4 drivePath)
{
  DWORD sectorsPerCluster;     /* EBP - 0x224 */
  FILE *testDirTmp;            /* EBP - 0x220*/
  DWORD numberOfFreeClusters;  /* EBP - 0x21c */
  DWORD bytesPerSector;        /* EBP - 0x218 */
  DWORD totalNumberOfClusters; /* EBP - 0x214 */
  char idk[0x108];             /* EBP - 0x210*/
  char rootPathName[0x106];    /* EBP - 0x108*/

  sprintf(rootPathName, "%s\\", drivePath);

  // Rest of the code is unreachable?
  if (GetDriveTypeA(rootPathName) == 5)
    return 1;
  else
    return 0;

  if (GetDiskFreeSpaceA(rootPathName, &sectorsPerCluster,
                        &bytesPerSector, &numberOfFreeClusters,
                        &totalNumberOfClusters) == 0)
  {
    return 1;
  }

  if (sectorsPerCluster * bytesPerSector * numberOfFreeClusters != 0)
    return 0;

  sprintf(idk, "%s\\Testdir.tmp", drivePath);
  testDirTmp = fopen(idk, "wb");
  if (testDirTmp != 0)
  {
    fclose(testDirTmp);
    unlink(idk);
    return 0;
  }

  return 1;
}

// FUNCTION: CDTOOLS 0x100017C0
undefined4 Autoplay_ShutDown(void)
{
  HKEY hKey;
  DWORD autoRunToSet;
  DWORD lpcbData;
  DWORD disableAutoRun;
  DWORD noDriveTypeAutoRun;

  noDriveTypeAutoRun = 0x95;
  disableAutoRun = 0xff;
  autoRunToSet = disableAutoRun;

  lpcbData = 4;
  hKey = (HKEY)0x0;
  g_originalNoDriveTypeAutoRun = noDriveTypeAutoRun;

  if (RegOpenKeyExA((HKEY)0x80000001, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", 0, 0xf003f,
                    &hKey) == 0)
  {

    if (RegQueryValueExA(hKey, "NoDriveTypeAutoRun", (LPDWORD)0x0, (LPDWORD)0x0,
                         (LPBYTE)&g_originalNoDriveTypeAutoRun, &lpcbData) == 0)
    {
      RegSetValueExA(hKey, "NoDriveTypeAutoRun", 0, 3, (const BYTE *)&autoRunToSet, 4);
    }
    else
    {
      g_originalNoDriveTypeAutoRun = noDriveTypeAutoRun;
    }
    RegFlushKey(hKey);
    RegCloseKey(hKey);
  }
  g_queryCancelAutoplayMsg = RegisterWindowMessageA("QueryCancelAutoPlay");
  return 1;
}

// FUNCTION: CDTOOLS 0x10001893
BOOL Autoplay_Restore(void)
{
  HKEY hKey;
  BOOL result;

  result = 0;
  if (RegOpenKeyExA((HKEY)0x80000001, "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", 0, 0xf003f,
                    &hKey) == 0)
  {
    result = 1;
    RegSetValueExA(hKey, "NoDriveTypeAutoRun", 0, 3, (const BYTE *)&g_originalNoDriveTypeAutoRun, 4);
    RegFlushKey(hKey);
    RegCloseKey(hKey);
  }
  return result;
}

// FUNCTION: CDTOOLS 0x10001910
int WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID reserved)
{
  g_cdtoolsInstance = hDllHandle;
  switch (nReason)
  {
  case 1:
    break;
  case 0:
    break;
  case 2:
    break;
  case 3:
    break;
  }
  return 1;
}
