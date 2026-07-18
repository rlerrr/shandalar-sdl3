#include "platform/winuser_compat.h"
#include "platform/winreg_compat.h"
#include "platform/legacy_io.h"
#include "platform/iniparser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ShandalarRegistryKey {
  char *path;
};

static const char *root_name(HKEY key)
{
  if (key == HKEY_CURRENT_USER)
    return "HKCU";
  if (key == HKEY_LOCAL_MACHINE)
    return "HKLM";
  return key ? key->path : NULL;
}

static char *make_path(HKEY key, const char *subkey)
{
  const char *root = root_name(key);
  size_t root_size;
  size_t subkey_size;
  char *path;
  if (!root)
    return NULL;
  root_size = strlen(root);
  subkey_size = strlen(subkey ? subkey : "");
  path = (char *)malloc(root_size + (subkey_size ? 1 : 0) + subkey_size + 1);
  if (!path)
    return NULL;
  memcpy(path, root, root_size);
  if (subkey_size) {
    path[root_size++] = '\\';
    memcpy(path + root_size, subkey, subkey_size);
    root_size += subkey_size;
  }
  path[root_size] = '\0';
  return path;
}

static dictionary *ini_load(void)
{
  FILE *fp;
  dictionary *d;
  fp = fopen("registry.ini", "r");
  if (!fp)
    return NULL;
  d = iniparser_load_file(fp, "registry.ini");
  fclose(fp);
  return d;
}

static int ini_save(dictionary *d)
{
  size_t i;
  FILE *fp;
  if (!d)
    return -1;
  fp = fopen("registry.ini", "w");
  if (!fp)
    return -1;
  for (i = 0; i < d->size; i++) {
    if (d->key[i] && d->val[i] == NULL)
      iniparser_dumpsection_ini(d, d->key[i], fp);
  }
  fprintf(fp, "\n");
  fclose(fp);
  return 0;
}

static const char *type_to_str(DWORD type)
{
  switch (type) {
    case REG_SZ:     return "REG_SZ";
    case REG_DWORD:  return "REG_DWORD";
    case REG_BINARY: return "REG_BINARY";
    case REG_QWORD:  return "REG_QWORD";
    default:         return "REG_BINARY";
  }
}

static DWORD str_to_type(const char *str)
{
  if (strcmp(str, "REG_SZ") == 0)     return REG_SZ;
  if (strcmp(str, "REG_DWORD") == 0)  return REG_DWORD;
  if (strcmp(str, "REG_BINARY") == 0) return REG_BINARY;
  if (strcmp(str, "REG_QWORD") == 0)  return REG_QWORD;
  return REG_BINARY;
}

static unsigned long long parse_qword(const char *str)
{
  unsigned long long val = 0;
  while (*str >= '0' && *str <= '9') {
    val = val * 10 + (unsigned long long)(*str - '0');
    str++;
  }
  return val;
}

static void bin_to_hex(const BYTE *data, DWORD size, char *out, DWORD out_size)
{
  static const char hex[] = "0123456789abcdef";
  DWORD i;
  for (i = 0; i < size && i * 2 + 1 < out_size; i++) {
    out[i * 2] = hex[(data[i] >> 4) & 0xf];
    out[i * 2 + 1] = hex[data[i] & 0xf];
  }
  if (out_size > 0)
    out[i * 2 < out_size ? i * 2 : out_size - 1] = '\0';
}

static int hex_to_bin(const char *hex_str, BYTE *out, DWORD out_size)
{
  DWORD len = (DWORD)strlen(hex_str) / 2;
  DWORD i;
  if (len > out_size) len = out_size;
  for (i = 0; i < len; i++) {
    BYTE b = 0;
    char hi = hex_str[i * 2];
    char lo = hex_str[i * 2 + 1];
    if (hi >= '0' && hi <= '9') b = (BYTE)((hi - '0') << 4);
    else if (hi >= 'a' && hi <= 'f') b = (BYTE)((hi - 'a' + 10) << 4);
    else if (hi >= 'A' && hi <= 'F') b = (BYTE)((hi - 'A' + 10) << 4);
    if (lo >= '0' && lo <= '9') b |= (BYTE)(lo - '0');
    else if (lo >= 'a' && lo <= 'f') b |= (BYTE)(lo - 'a' + 10);
    else if (lo >= 'A' && lo <= 'F') b |= (BYTE)(lo - 'A' + 10);
    out[i] = b;
  }
  return (int)len;
}

LSTATUS RegOpenKeyExA(HKEY key, LPCSTR subkey, DWORD options, DWORD desired, HKEY *result)
{
  char *path;
  dictionary *d;
  int found;
  (void)options;
  (void)desired;
  if (!result || !(path = make_path(key, subkey)))
    return ERROR_INVALID_PARAMETER;
  d = ini_load();
  found = d && iniparser_find_entry(d, path);
  iniparser_freedict(d);
  if (!found) {
    free(path);
    return ERROR_FILE_NOT_FOUND;
  }
  *result = (HKEY)malloc(sizeof(struct ShandalarRegistryKey));
  if (!*result) {
    free(path);
    return ERROR_INVALID_PARAMETER;
  }
  (*result)->path = path;
  return ERROR_SUCCESS;
}

LSTATUS RegCreateKeyExA(HKEY key, LPCSTR subkey, DWORD reserved, LPSTR class_name,
                        DWORD options, DWORD desired, LPSECURITY_ATTRIBUTES security,
                        HKEY *result, LPDWORD disposition)
{
  char *path;
  dictionary *d;
  int exists;
  (void)reserved;
  (void)class_name;
  (void)options;
  (void)desired;
  (void)security;
  if (!result || !(path = make_path(key, subkey)))
    return ERROR_INVALID_PARAMETER;
  d = ini_load();
  exists = d && iniparser_find_entry(d, path);
  iniparser_freedict(d);
  *result = (HKEY)malloc(sizeof(struct ShandalarRegistryKey));
  if (!*result) {
    free(path);
    return ERROR_INVALID_PARAMETER;
  }
  (*result)->path = path;
  if (disposition)
    *disposition = exists ? 2 : 1;
  return ERROR_SUCCESS;
}

LSTATUS RegQueryValueExA(HKEY key, LPCSTR value_name, LPDWORD reserved, LPDWORD type,
                         LPBYTE data, LPDWORD data_size)
{
  const char *section;
  char type_key[520], value_key[520];
  const char *type_str, *value_str;
  DWORD reg_type, needed_size;
  BYTE buffer[4096];
  dictionary *d;
  (void)reserved;
  if (!key || key == HKEY_CURRENT_USER || key == HKEY_LOCAL_MACHINE || !value_name || !data_size)
    return ERROR_INVALID_PARAMETER;
  section = key->path;
  d = ini_load();
  if (!d)
    return ERROR_FILE_NOT_FOUND;
  snprintf(type_key, sizeof(type_key), "%s:%s.type", section, value_name);
  snprintf(value_key, sizeof(value_key), "%s:%s.value", section, value_name);
  type_str = iniparser_getstring(d, type_key, "");
  value_str = iniparser_getstring(d, value_key, "");
  if (!type_str[0] || !value_str[0]) {
    iniparser_freedict(d);
    return ERROR_FILE_NOT_FOUND;
  }
  reg_type = str_to_type(type_str);
  if (type)
    *type = reg_type;
  switch (reg_type) {
    case REG_SZ: {
      DWORD slen = (DWORD)strlen(value_str) + 1;
      if (slen > sizeof(buffer)) slen = sizeof(buffer);
      memcpy(buffer, value_str, slen);
      needed_size = (DWORD)strlen(value_str) + 1;
      break;
    }
    case REG_DWORD: {
      DWORD val = (DWORD)iniparser_getint(d, value_key, 0);
      memcpy(buffer, &val, sizeof(val));
      needed_size = 4;
      break;
    }
    case REG_QWORD: {
      unsigned long long qval = parse_qword(value_str);
      memcpy(buffer, &qval, sizeof(qval));
      needed_size = 8;
      break;
    }
    default: {
      needed_size = (DWORD)(strlen(value_str) / 2);
      if (needed_size > sizeof(buffer))
        needed_size = (DWORD)sizeof(buffer);
      hex_to_bin(value_str, buffer, needed_size);
      break;
    }
  }
  iniparser_freedict(d);
  if (!data) {
    *data_size = needed_size;
    return ERROR_SUCCESS;
  }
  if (*data_size < needed_size) {
    *data_size = needed_size;
    return ERROR_MORE_DATA;
  }
  memcpy(data, buffer, needed_size);
  *data_size = needed_size;
  return ERROR_SUCCESS;
}

LSTATUS RegSetValueExA(HKEY key, LPCSTR value_name, DWORD reserved, DWORD type,
                       const BYTE *data, DWORD data_size)
{
  const char *section;
  char type_key[520], value_key[520];
  char type_str[32], value_str[4096];
  dictionary *d;
  (void)reserved;
  if (!key || key == HKEY_CURRENT_USER || key == HKEY_LOCAL_MACHINE || !value_name || (!data && data_size))
    return ERROR_INVALID_PARAMETER;
  if (type == REG_DWORD && data_size < 4)
    return ERROR_INVALID_PARAMETER;
  section = key->path;
  snprintf(type_key, sizeof(type_key), "%s:%s.type", section, value_name);
  snprintf(value_key, sizeof(value_key), "%s:%s.value", section, value_name);
  snprintf(type_str, sizeof(type_str), "%s", type_to_str(type));
  switch (type) {
    case REG_SZ: {
      DWORD slen = data ? (DWORD)strlen((const char *)data) : 0;
      if (slen >= sizeof(value_str)) slen = sizeof(value_str) - 1;
      if (data) memcpy(value_str, data, slen);
      value_str[slen] = '\0';
      break;
    }
    case REG_DWORD: {
      DWORD val = 0;
      if (data_size >= 4) memcpy(&val, data, 4);
      snprintf(value_str, sizeof(value_str), "%lu", (unsigned long)val);
      break;
    }
    case REG_QWORD: {
      unsigned long long qval = 0;
      if (data_size >= 8) memcpy(&qval, data, 8);
      snprintf(value_str, sizeof(value_str), "%llu", qval);
      break;
    }
    default: {
      DWORD hex_len = data_size * 2;
      if (hex_len >= sizeof(value_str)) hex_len = (DWORD)sizeof(value_str) - 1;
      bin_to_hex(data, data_size, value_str, sizeof(value_str));
      break;
    }
  }
  d = ini_load();
  if (!d)
    d = dictionary_new(0);
  iniparser_set(d, section, NULL);
  iniparser_set(d, type_key, type_str);
  iniparser_set(d, value_key, value_str);
  ini_save(d);
  iniparser_freedict(d);
  return ERROR_SUCCESS;
}

LSTATUS RegCloseKey(HKEY key)
{
  if (!key)
    return ERROR_INVALID_PARAMETER;
  if (key == HKEY_CURRENT_USER || key == HKEY_LOCAL_MACHINE)
    return ERROR_SUCCESS;
  free(key->path);
  free(key);
  return ERROR_SUCCESS;
}

LSTATUS RegFlushKey(HKEY key)
{
  (void)key;
  return ERROR_SUCCESS;
}
