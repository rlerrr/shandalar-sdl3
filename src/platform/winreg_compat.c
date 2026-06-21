#include "platform/winuser_compat.h"

#include <stdlib.h>
#include <string.h>

typedef struct RegistryValue {
  struct RegistryValue *next;
  char *name;
  DWORD type;
  BYTE *data;
  DWORD size;
} RegistryValue;

struct ShandalarRegistryKey {
  struct ShandalarRegistryKey *next;
  char *path;
  RegistryValue *values;
};

static struct ShandalarRegistryKey *g_registry_keys;

static char *registry_strdup(const char *text)
{
  size_t size = strlen(text ? text : "") + 1;
  char *copy = (char *)malloc(size);
  if (copy)
    memcpy(copy, text ? text : "", size);
  return copy;
}

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

static HKEY find_key(const char *path)
{
  HKEY key;
  for (key = g_registry_keys; key; key = key->next)
    if (strcmp(key->path, path) == 0)
      return key;
  return NULL;
}

static RegistryValue *find_value(HKEY key, const char *name)
{
  RegistryValue *value;
  if (!key || key == HKEY_CURRENT_USER || key == HKEY_LOCAL_MACHINE)
    return NULL;
  for (value = key->values; value; value = value->next)
    if (strcmp(value->name, name ? name : "") == 0)
      return value;
  return NULL;
}

LSTATUS RegOpenKeyExA(HKEY key, LPCSTR subkey, DWORD options, DWORD desired, HKEY *result)
{
  char *path;
  HKEY found;
  (void)options;
  (void)desired;
  if (!result || !(path = make_path(key, subkey)))
    return ERROR_INVALID_PARAMETER;
  found = find_key(path);
  free(path);
  if (!found)
    return ERROR_FILE_NOT_FOUND;
  *result = found;
  return ERROR_SUCCESS;
}

LSTATUS RegCreateKeyExA(HKEY key, LPCSTR subkey, DWORD reserved, LPSTR class_name,
                        DWORD options, DWORD desired, LPSECURITY_ATTRIBUTES security,
                        HKEY *result, LPDWORD disposition)
{
  char *path;
  HKEY found;
  (void)reserved;
  (void)class_name;
  (void)options;
  (void)desired;
  (void)security;
  if (!result || !(path = make_path(key, subkey)))
    return ERROR_INVALID_PARAMETER;
  found = find_key(path);
  if (!found) {
    found = (HKEY)calloc(1, sizeof(*found));
    if (!found) {
      free(path);
      return ERROR_INVALID_PARAMETER;
    }
    found->path = path;
    found->next = g_registry_keys;
    g_registry_keys = found;
    if (disposition)
      *disposition = 1;
  } else {
    free(path);
    if (disposition)
      *disposition = 2;
  }
  *result = found;
  return ERROR_SUCCESS;
}

LSTATUS RegQueryValueExA(HKEY key, LPCSTR value_name, LPDWORD reserved, LPDWORD type,
                         LPBYTE data, LPDWORD data_size)
{
  RegistryValue *value = find_value(key, value_name);
  (void)reserved;
  if (!value || !data_size)
    return value ? ERROR_INVALID_PARAMETER : ERROR_FILE_NOT_FOUND;
  if (type)
    *type = value->type;
  if (!data) {
    *data_size = value->size;
    return ERROR_SUCCESS;
  }
  if (*data_size < value->size) {
    *data_size = value->size;
    return ERROR_MORE_DATA;
  }
  memcpy(data, value->data, value->size);
  *data_size = value->size;
  return ERROR_SUCCESS;
}

LSTATUS RegSetValueExA(HKEY key, LPCSTR value_name, DWORD reserved, DWORD type,
                       const BYTE *data, DWORD data_size)
{
  RegistryValue *value;
  BYTE *copy;
  (void)reserved;
  if (!key || key == HKEY_CURRENT_USER || key == HKEY_LOCAL_MACHINE || (!data && data_size))
    return ERROR_INVALID_PARAMETER;
  copy = data_size ? (BYTE *)malloc(data_size) : NULL;
  if (data_size && !copy)
    return ERROR_INVALID_PARAMETER;
  if (data_size)
    memcpy(copy, data, data_size);
  value = find_value(key, value_name);
  if (!value) {
    value = (RegistryValue *)calloc(1, sizeof(*value));
    if (!value) {
      free(copy);
      return ERROR_INVALID_PARAMETER;
    }
    value->name = registry_strdup(value_name ? value_name : "");
    value->next = key->values;
    key->values = value;
  }
  free(value->data);
  value->data = copy;
  value->size = data_size;
  value->type = type;
  return ERROR_SUCCESS;
}

LSTATUS RegCloseKey(HKEY key) { return key ? ERROR_SUCCESS : ERROR_INVALID_PARAMETER; }
LSTATUS RegFlushKey(HKEY key) { return key ? ERROR_SUCCESS : ERROR_INVALID_PARAMETER; }
