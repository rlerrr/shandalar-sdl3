#include "defs.h"
#include "drm.h"
#include "platform/gdi_compat.h"
#include "mystdbool.h"

#ifndef MODERN_FIXES
#pragma intrinsic(_rotl)
#pragma intrinsic(_rotr)
#endif

// GLOBAL: DECKDLL 0x10035b88
// GLOBAL: MAGIC 0x00571d68
// GLOBAL: SHANDALAR 0x585970
uint8_t card_coded[3][128] = {
    {255, 229, 239, 119, 127, 181, 191, 206, 127, 59, 254, 255, 126, 247, 63, 250, 235, 217, 239, 255, 255, 235, 255, 223, 219, 55, 235, 246, 251, 255, 253, 175, 121, 250, 255, 255, 237, 144, 7, 68, 66, 0, 155, 12, 0, 49, 8, 193, 16, 158, 1, 178, 74, 74, 194, 48, 65, 84, 132, 200, 251, 213, 208, 120, 133, 32, 241, 13, 202, 0, 9, 128, 208, 33, 0, 81, 0, 85, 20, 0, 36, 2, 96, 0, 128, 0, 0, 12, 64, 128, 0, 0, 65, 34, 17, 40, 1, 96, 0, 12, 8, 36, 2, 132, 40, 2, 216, 248, 255, 0, 32, 12, 16, 64, 0, 0, 140, 202, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 26, 16, 0, 128, 74, 64, 49, 128, 132, 1, 0, 129, 8, 192, 4, 20, 38, 16, 0, 0, 20, 0, 32, 36, 200, 20, 9, 4, 0, 2, 80, 134, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 128, 97, 250, 77, 181, 181, 61, 205, 190, 137, 123, 55, 4, 42, 39, 135, 90, 223, 14, 242, 53, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 208, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 75, 16, 26, 25, 148, 4, 66, 10, 0, 196, 16, 42, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 18, 32, 9, 66, 16, 168, 146, 160, 128, 8, 129, 4, 20, 40, 37, 72, 16, 225, 1, 10, 192, 5, 128, 0, 40, 128, 76, 0, 5, 82, 32, 0, 1, 8, 128, 60, 5, 0, 0, 0, 128, 0, 0, 0, 0, 0, 0, 49, 153, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

#ifndef MODERN_FIXES

// GLOBAL: DECKDLL 0x10033008
// GLOBAL: SHANDALAR 0x00583848
char encoded_windows_current_version_key[] = {0x12, 0x0e, 0x07, 0x15, 0x16, 0x00, 0x13, 0x04, 0x1d, 0x0c, 0x28, 0x22, 0x33, 0x2e, 0x32, 0x2e, 0x27, 0x35, 0x1d, 0x16, 0x28, 0x2f, 0x25, 0x2e, 0x36, 0x32, 0x1d, 0x02, 0x34, 0x33, 0x33, 0x24, 0x2f, 0x35, 0x17, 0x24, 0x33, 0x32, 0x28, 0x2e, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// GLOBAL: DECKDLL 0x10033038
// GLOBAL: SHANDALAR 0x00583878
char encoded_magic_registry_key[40] = {0x12, 0x0e, 0x07, 0x15, 0x16, 0x00, 0x13, 0x04, 0x1d, 0x0c, 0x28, 0x22, 0x33, 0x2e, 0x11, 0x33, 0x2e, 0x32, 0x24, 0x1d, 0x0c, 0x20, 0x26, 0x28, 0x22, 0x7b, 0x61, 0x15, 0x29, 0x24, 0x61, 0x06, 0x20, 0x35, 0x29, 0x24, 0x33, 0x28, 0x2f, 0x26};

// GLOBAL: DECKDLL 0x10033060
// GLOBAL: SHANDALAR 0x005838a0
char encoded_first_install_date_time[] = {0x07, 0x28, 0x33, 0x32, 0x35, 0x08, 0x2f, 0x32, 0x35, 0x20, 0x2d, 0x2d, 0x05, 0x20, 0x35, 0x24, 0x15, 0x28, 0x2c, 0x24, 0x00, 0x00, 0x00, 0x00};

// GLOBAL: DECKDLL 0x10033078
// GLOBAL: SHANDALAR 0x005838b8
char encoded_product_id_value[] = {0x11, 0x33, 0x2e, 0x25, 0x34, 0x22, 0x35, 0x08, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// GLOBAL: DECKDLL 0x10033088
char s__10033088[] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00};

// GLOBAL: DECKDLL 0x100330b8
char s__100330b8[] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00};

// GLOBAL: DECKDLL 0x100330c8
char s__100330c8[] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00};

// GLOBAL: DECKDLL 0x100330f8
char s__100330f8[] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00};

// GLOBAL: DECKDLL 0x101bc86c
// GLOBAL: SHANDALAR 0x0078e5bc
uint32_t global_serial_part_a;

// GLOBAL: DECKDLL 0x101bc870
// GLOBAL: SHANDALAR 0x0078e5c0
int global_serial_part_b;

// GLOBAL: DECKDLL 0x101bc874
// GLOBAL: SHANDALAR 0x0078e5c4
int global_Windows_FirstInstallDateTime;

// GLOBAL: DECKDLL 0x101bc878
// GLOBAL: SHANDALAR 0x0078e5c8
int global_product_id_seed_a;

// GLOBAL: DECKDLL 0x101bc87c
// GLOBAL: SHANDALAR 0x0078e5cc
int global_product_id_seed_b;

// FUNCTION: DECKDLL 0x1000de30
// FUNCTION: MAGIC 0x0043d210
// FUNCTION: SHANDALAR 0x00441b70
static void ScrubString(char *decoded_string)
{
  memset(decoded_string, 0x20, strlen(decoded_string));
}

// FUNCTION: DECKDLL 0x1000de56
// FUNCTION: SHANDALAR 0x00441b96
static void ReverseBits32(uint *value)
{
  int i;             // ebp - 0x10
  uint result;       // ebp - 0x0c
  uint dst_bit_mask; // ebp - 0x08
  uint src_bit_mask; // ebp - 0x04

  result = 0;
  src_bit_mask = 1;
  dst_bit_mask = 0x80000000;
  for (i = 0; i < 0x20; i++)
  {
    if (src_bit_mask & *value)
      result |= dst_bit_mask;
    src_bit_mask <<= 1;
    dst_bit_mask >>= 1;
  }
  *value = result;
}

// FUNCTION: DECKDLL 0x1000deb9
// FUNCTION: MAGIC 0x0043d29b
// FUNCTION: SHANDALAR 0x00441bf9
int InitLicenseSecretsFromRegistry(void)
{
#ifndef MODERN_FIXES
  int sProductIdLength;
  size_t sFirstInstallDateTimeLength;
  int sWindowsCurrentVersionKeyLength;
  int repeat_idx;
  DWORD lpcbData;
  size_t winProductIdLength;
  uint i;
  HKEY hKey;
  BYTE winProductId[1024];
  char *prodKeyPtr;
  DWORD lpType;

  prodKeyPtr = NULL;
  // This generates "SOFTWARE\Microsoft\Windows\CurrentVersion"
  for (i = 0; strlen(s__100330c8) > i; i++)
  {
    s__100330c8[i] = encoded_windows_current_version_key[i] ^ 0x41;
  }
  sWindowsCurrentVersionKeyLength = strlen(s__100330c8);
  s__100330c8[sWindowsCurrentVersionKeyLength] = '\0';

  if (RegOpenKeyA((HKEY)0x80000002, s__100330c8, &hKey) != 0)
  {
    ScrubString(s__100330c8);
    return 1;
  }

  ScrubString(s__100330c8);
  lpType = 4;
  lpcbData = 4;

  // This generates "FirstInstallDateTime" which is a Windows 95/98/ME thing
  for (i = 0; strlen(s__100330f8) > i; i++)
  {
    s__100330f8[i] = encoded_first_install_date_time[i] ^ 0x41;
  }

  sFirstInstallDateTimeLength = strlen(s__100330f8);
  s__100330f8[sFirstInstallDateTimeLength] = '\0';

  if (RegQueryValueExA(hKey, s__100330f8, (LPDWORD)0x0, &lpType, (LPBYTE)&global_Windows_FirstInstallDateTime, &lpcbData) != 0)
  {
    ScrubString(s__100330f8);
    RegCloseKey(hKey);
    return 1;
  }

  ScrubString(s__100330f8);
  lpType = 1;
  lpcbData = 0x400;

  // This generates "ProductId"
  for (i = 0; strlen(s__100330b8) > i; i++)
  {
    s__100330b8[i] = encoded_product_id_value[i] ^ 0x41;
  }

  sProductIdLength = strlen(s__100330b8);
  s__100330b8[sProductIdLength] = '\0';

  if (RegQueryValueExA(hKey, s__100330b8, (LPDWORD)0x0, &lpType, winProductId, &lpcbData) != 0)
  {
    ScrubString(s__100330b8);
    RegCloseKey(hKey);
    return 1;
  }

  ScrubString(s__100330b8);
  winProductIdLength = strlen((char *)winProductId);

  // Repeats product key into itself up to 16 chars
  for (repeat_idx = 0; strlen((char *)winProductId) < 0x10;)
  {
    winProductId[winProductIdLength] = winProductId[repeat_idx];
    repeat_idx = repeat_idx + 1;
    winProductIdLength++;
    winProductId[winProductIdLength] = '\0';
  }

  // Using product key as entropy in a weird way (just the last 8 chars)
  prodKeyPtr = &(winProductId - 8)[strlen((char *)winProductId)];
  memcpy(&global_product_id_seed_a, prodKeyPtr, 4);
  prodKeyPtr += 4;
  memcpy(&global_product_id_seed_b, prodKeyPtr, 4);
  prodKeyPtr = NULL;
  RegCloseKey(hKey);
#endif
  return 0;
}

// DRM
// Return codes:
// 0 = success (valid serial)
// 1 = registry / allocation failure
// 2 = invalid serial
// FUNCTION: DECKDLL 0x1000e20a
// FUNCTION: MAGIC 0x0043d5ec
// FUNCTION: SHANDALAR 0x00441f4a
static int ValidateRegistrySerial(uint32_t *out_value)
{
  int regPathLength; // ebp - 0x40
  uint8_t *buf;      // ebp - 0x3c
  uint32_t i;        // ebp - 0x38
  int sumpt8;        // ebp - 0x34
  int sumpt7;        // ebp - 0x30
  char *pad6;        // ebp - 0x2c
  uint32_t checksum; // ebp - 0x28
  int sumpt6;        // ebp - 0x24
  DWORD size;        // ebp - 0x20
  int sumpt5;        // ebp - 0x1c
  HKEY hKey;
  int sumpt4;      // ebp - 0x14
  int sumpt3;      // ebp - 0x10
  int sumpt2;      // ebp - 0xc
  DWORD type;      // ebp - 0x8
  uint32_t sumpt1; // ebp - 0x4

  *out_value = 0;

  for (i = 0; i < strlen(s__10033088); i++)
  {
    s__10033088[i] = encoded_magic_registry_key[i] ^ 0x41;
  }
  regPathLength = strlen(s__10033088);
  s__10033088[regPathLength] = '\0';

  // ------------------------------------------------------------
  // Open HKLM\<decoded key>
  // ------------------------------------------------------------
  if (RegOpenKeyA(HKEY_LOCAL_MACHINE, s__10033088, &hKey) != ERROR_SUCCESS)
  {
    ScrubString(s__10033088); // wipe decoded string
    return 1;
  }

  ScrubString(s__10033088); // wipe decoded string again

  // ------------------------------------------------------------
  // Allocate buffer for serial (12 bytes)
  // ------------------------------------------------------------
  size = 12;
  type = REG_BINARY;
  buf = malloc(size);
  if (!buf)
  {
    RegCloseKey(hKey);
    return 1;
  }

  // ------------------------------------------------------------
  // Read "SerialNumber" value
  // ------------------------------------------------------------
  if (RegQueryValueExA(
          hKey,
          "SerialNumber",
          NULL,
          &type,
          buf,
          &size) != ERROR_SUCCESS)
  {
    RegCloseKey(hKey);
    return 1;
  }

  RegCloseKey(hKey);

  // ------------------------------------------------------------
  // Split serial fields
  // ------------------------------------------------------------
  pad6 = buf;
  memcpy(&global_serial_part_a, pad6, 4); // part A
  pad6 += 4;
  memcpy(&global_serial_part_b, pad6, 4); // part B
  pad6 += 4;
  memcpy(&checksum, pad6, 4); // checksum
  pad6 = NULL;
  checksum ^= global_Windows_FirstInstallDateTime;

  // ------------------------------------------------------------
  // Nibble checksum verification
  // ------------------------------------------------------------
  *(byte *)&sumpt1 = ((global_serial_part_a >> 24) & 0xF);
  *(byte *)&sumpt2 = ((global_serial_part_a >> 16) & 0xF);
  *(byte *)&sumpt3 = ((global_serial_part_a >> 8) & 0xF);
  *(byte *)&sumpt4 = ((global_serial_part_a >> 0) & 0xF);
  *(byte *)&sumpt5 = ((global_serial_part_b >> 24) & 0xF);
  *(byte *)&sumpt6 = ((global_serial_part_b >> 16) & 0xF);
  *(byte *)&sumpt7 = ((global_serial_part_b >> 8) & 0xF);
  *(byte *)&sumpt8 = ((global_serial_part_b >> 0) & 0xF);

  //*(byte*)&sum =

  if ((
          *(byte *)&sumpt1 +
          *(byte *)&sumpt2 +
          *(byte *)&sumpt3 +
          *(byte *)&sumpt4 +
          *(byte *)&sumpt5 +
          *(byte *)&sumpt6 +
          *(byte *)&sumpt7 +
          *(byte *)&sumpt8) != checksum)
    return 2;

  // ------------------------------------------------------------
  // Serial decryption / mixing
  // ------------------------------------------------------------

  global_serial_part_a ^= global_product_id_seed_a;
  global_serial_part_b ^= global_product_id_seed_b;

  // TODO: this is almost possible using _rotl/_rotr
  // the push/pop make no sense even for an inlined function
  __asm {
    push eax
    rol dword ptr [global_Windows_FirstInstallDateTime], 6
    mov eax, dword ptr [global_serial_part_a]
    xor eax, dword ptr [global_Windows_FirstInstallDateTime]
    mov dword ptr [global_serial_part_a], eax
    ror dword ptr [global_Windows_FirstInstallDateTime], 0xc
    mov eax, dword ptr [global_serial_part_b]
    xor eax, dword ptr [global_Windows_FirstInstallDateTime]
    mov dword ptr [global_serial_part_b], eax
    rol dword ptr [global_Windows_FirstInstallDateTime], 6
    pop eax
  }

  ReverseBits32(&global_serial_part_b);

  if (global_serial_part_a != global_serial_part_b)
    return 2;

  *out_value = global_serial_part_a;
  return 0;
}
#else
int InitLicenseSecretsFromRegistry(void)
{
  return 0;
}
#endif

unsigned int HasExpansion(unsigned char expansion_mask);

// FUNCTION: DECKDLL 0x1002709f
// FUNCTION: MAGIC 0x00497811
// FUNCTION: SHANDALAR 0x004682a0
int IsCardAvailable(csvid_t csvid, int expansion)
{
  int result;

  result = 1;
  if (expansion == 1)
  {
    if (HasExpansion(2) == 0)
      result = 0;
  }
  if (expansion == 2)
  {
    if (HasExpansion(4) == 0)
      result = 0;
  }
  if ((((csvid < 0) || (0x3FF < csvid)) || (expansion < 0)) || (2 < expansion))
    result = 0;

  if ((*(int *)&card_coded[expansion][(csvid >> 5) << 2] & (1 << (csvid & 0x1f))) == 0)
    result = 0;

  return result;
}

// FUNCTION: DECKDLL 0x10027164
// FUNCTION: MAGIC 0x004978d6
// FUNCTION: SHANDALAR 0x00468365
int is_card_available_in_expansion(unsigned int csvid, int expansion)
{
  int result;

  result = 1;
  if ((((int)csvid < 0) || (0x3FF < (int)csvid)) || (expansion < 0) || (2 < expansion))
    result = 0;

  if ((*(int *)&card_coded[expansion][((int)(csvid & 0xffffffe0)) >> 3] & (1 << (csvid & 0x1f))) == 0)
    result = 0;

  return result;
}

#ifndef DECKDLL
// FUNCTION: MAGIC 0x00497955
// FUNCTION: SHANDALAR 0x004683e4
int is_card_available_in_installed_expansion(unsigned int csvid)
{
  int result;

  result = 0;
  if (HasExpansion(1) != 0 && is_card_available_in_expansion(csvid, 0) != 0)
    result = 1;
  else if (HasExpansion(2) != 0 && is_card_available_in_expansion(csvid, 1) != 0)
    result = 1;
  else if (HasExpansion(4) != 0 && is_card_available_in_expansion(csvid, 2) != 0)
    result = 1;

  return result;
}
#endif

// FUNCTION: DECKDLL 0x100271e3
// FUNCTION: MAGIC 0x00497a09
// FUNCTION: SHANDALAR 0x00468498
unsigned int HasExpansion(unsigned char expansion_mask)
{
#ifndef MODERN_FIXES
  unsigned int serial;
  unsigned int result;

  result = 0;
  if ((expansion_mask & 1) == 1)
    return 1;

  serial = 0;
  if (ValidateRegistrySerial(&serial) != 0)
    return 0;

  if ((expansion_mask & 1) == 1 && (serial & 0x400000))
    result = 1;
  else if ((expansion_mask & 2) == 2 && (serial & 0x100))
    result = 1;
  else if ((expansion_mask & 4) == 4 && (serial & 0x800))
    result = 1;
  else if ((expansion_mask & 8) == 8 && (serial & 0x20000))
    result = 1;
  else if ((expansion_mask & 0x10) == 0x10 && (serial & 0x10000000))
    result = 1;
#ifndef DECKDLL
  else if ((expansion_mask & 0x20) == 0x20 && (serial & 0x2000))
    result = 1;
#endif
  else
    result = 0;

  return result;
#else
  return (expansion_mask & 0x0f) != 0;
#endif
}
