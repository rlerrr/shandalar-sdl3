#include "platform/cheat_options.h"

#include "platform/winuser_compat.h"

#include <stdlib.h>
#include <string.h>

#define SHANDALAR_CHEAT_OPTIONS_REGISTRY_PATH \
  "Software\\MicroProse\\Magic: The Gathering\\CheatOptions"

int shandalar_cheat_enabled(const char *option_name)
{
  BYTE value[32];
  DWORD type;
  DWORD value_size;
  HKEY options_key;
  int enabled;

  if (!option_name ||
      RegOpenKeyExA(HKEY_CURRENT_USER, SHANDALAR_CHEAT_OPTIONS_REGISTRY_PATH,
                    0, KEY_QUERY_VALUE, &options_key) != ERROR_SUCCESS)
  {
    return 0;
  }

  type = 0;
  value_size = sizeof(value);
  enabled = 0;
  if (RegQueryValueExA(options_key, option_name, NULL, &type, value,
                       &value_size) == ERROR_SUCCESS)
  {
    if (type == REG_DWORD && value_size >= sizeof(DWORD))
    {
      DWORD dword_value;
      memcpy(&dword_value, value, sizeof(dword_value));
      enabled = dword_value != 0;
    }
    else if (type == REG_SZ)
    {
      value[sizeof(value) - 1] = '\0';
      enabled = strtol((const char *)value, NULL, 10) != 0;
    }
  }

  RegCloseKey(options_key);
  return enabled;
}
