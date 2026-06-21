#ifndef SHANDALAR_WINREG_COMPAT_H
#define SHANDALAR_WINREG_COMPAT_H

typedef struct ShandalarRegistryKey *HKEY;
typedef LONG LSTATUS;
typedef DWORD *LPDWORD;
typedef BYTE *LPBYTE;

#define ERROR_SUCCESS 0L
#define ERROR_FILE_NOT_FOUND 2L
#define ERROR_INVALID_PARAMETER 87L
#define ERROR_MORE_DATA 234L
#define HKEY_CURRENT_USER ((HKEY)(uintptr_t)1)
#define HKEY_LOCAL_MACHINE ((HKEY)(uintptr_t)2)
#define KEY_QUERY_VALUE 0x0001
#define KEY_SET_VALUE 0x0002
#define KEY_READ 0x20019
#define KEY_WRITE 0x20006
#define KEY_ALL_ACCESS 0xf003f
#define REG_SZ 1
#define REG_BINARY 3
#define REG_DWORD 4
#define REG_OPTION_NON_VOLATILE 0

LSTATUS RegOpenKeyExA(HKEY key, LPCSTR subkey, DWORD options, DWORD desired, HKEY *result);
LSTATUS RegCreateKeyExA(HKEY key, LPCSTR subkey, DWORD reserved, LPSTR class_name,
                        DWORD options, DWORD desired, LPSECURITY_ATTRIBUTES security,
                        HKEY *result, LPDWORD disposition);
LSTATUS RegQueryValueExA(HKEY key, LPCSTR value_name, LPDWORD reserved, LPDWORD type,
                         LPBYTE data, LPDWORD data_size);
LSTATUS RegSetValueExA(HKEY key, LPCSTR value_name, DWORD reserved, DWORD type,
                       const BYTE *data, DWORD data_size);
LSTATUS RegCloseKey(HKEY key);
LSTATUS RegFlushKey(HKEY key);

#define RegOpenKeyEx RegOpenKeyExA
#define RegCreateKeyEx RegCreateKeyExA
#define RegQueryValueEx RegQueryValueExA
#define RegSetValueEx RegSetValueExA

#endif
