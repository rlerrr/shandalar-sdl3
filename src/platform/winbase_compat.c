#include "platform/winuser_compat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct ProcEntry {
  struct ProcEntry *next;
  char *name;
  FARPROC proc;
} ProcEntry;

static ProcEntry *g_procs;
static int g_main_module;

static char *duplicate_string(const char *value)
{
  size_t size = strlen(value) + 1;
  char *copy = (char *)malloc(size);
  if (copy)
    memcpy(copy, value, size);
  return copy;
}

HMODULE GetModuleHandleA(LPCSTR module_name)
{
  /* The compatibility runtime is statically linked, so there is one module. */
  (void)module_name;
  return (HMODULE)&g_main_module;
}

FARPROC GetProcAddress(HMODULE module, LPCSTR proc_name)
{
  ProcEntry *entry;
  (void)module;
  if ((uintptr_t)proc_name <= 0xffffu)
    return NULL;
  for (entry = g_procs; entry; entry = entry->next)
    if (strcmp(entry->name, proc_name) == 0)
      return entry->proc;
  return NULL;
}

BOOL shandalar_win32_register_proc(LPCSTR proc_name, FARPROC proc)
{
  ProcEntry *entry;
  if (!proc_name || !proc)
    return FALSE;
  for (entry = g_procs; entry; entry = entry->next) {
    if (strcmp(entry->name, proc_name) == 0) {
      entry->proc = proc;
      return TRUE;
    }
  }
  entry = (ProcEntry *)calloc(1, sizeof(*entry));
  if (!entry)
    return FALSE;
  entry->name = duplicate_string(proc_name);
  if (!entry->name) {
    free(entry);
    return FALSE;
  }
  entry->proc = proc;
  entry->next = g_procs;
  g_procs = entry;
  return TRUE;
}

DWORD GetCurrentDirectoryA(DWORD size, LPSTR buffer)
{
  char *directory = SDL_GetCurrentDirectory();
  DWORD needed;
  if (!directory)
    return 0;
  needed = (DWORD)strlen(directory);
  if (!buffer || size <= needed) {
    SDL_free(directory);
    return needed + 1;
  }
  SDL_strlcpy(buffer, directory, size);
  SDL_free(directory);
  return needed;
}

void Sleep(DWORD milliseconds) { SDL_Delay(milliseconds); }

int GetDateFormatA(DWORD locale, DWORD flags, const void *date,
                   LPCSTR format, LPSTR output, int output_size)
{
  time_t now;
  struct tm local_time;
  size_t written;
  (void)locale; (void)flags; (void)date; (void)format;
  if (!output || output_size <= 0)
    return 0;
  time(&now);
#ifdef _WIN32
  if (localtime_s(&local_time, &now) != 0)
    return 0;
#else
  if (!localtime_r(&now, &local_time))
    return 0;
#endif
  written = strftime(output, (size_t)output_size, "%B %d, %Y", &local_time);
  return written ? (int)written + 1 : 0;
}

static void normalize_path(char *out, size_t out_size, const char *path)
{
  size_t i;
  SDL_strlcpy(out, path ? path : "", out_size);
  for (i = 0; out[i]; ++i)
    if (out[i] == '\\')
      out[i] = '/';
}

BOOL WritePrivateProfileStringA(LPCSTR section, LPCSTR key, LPCSTR value, LPCSTR filename)
{
  FILE *input;
  FILE *output;
  char path[1024];
  char temp_path[1050];
  char line[1024];
  int in_section = 0;
  int found_section = 0;
  int wrote_key = 0;
  BOOL result = FALSE;

  if (!section || !key || !filename)
    return FALSE;
  normalize_path(path, sizeof(path), filename);
  SDL_snprintf(temp_path, sizeof(temp_path), "%s.tmp", path);
  input = fopen(path, "rb");
  output = fopen(temp_path, "wb");
  if (!output) {
    if (input)
      fclose(input);
    return FALSE;
  }

  while (input && fgets(line, sizeof(line), input)) {
    char scratch[1024];
    char *cursor;
    SDL_strlcpy(scratch, line, sizeof(scratch));
    cursor = scratch;
    while (*cursor == ' ' || *cursor == '\t')
      ++cursor;
    if (*cursor == '[') {
      char *close = strchr(cursor + 1, ']');
      if (in_section && !wrote_key && value) {
        fprintf(output, "%s=%s\n", key, value);
        wrote_key = 1;
      }
      in_section = 0;
      if (close) {
        *close = '\0';
        in_section = SDL_strcasecmp(cursor + 1, section) == 0;
        if (in_section)
          found_section = 1;
      }
      fputs(line, output);
      continue;
    }
    if (in_section) {
      char *equals = strchr(cursor, '=');
      if (equals) {
        char *end = equals;
        while (end > cursor && (end[-1] == ' ' || end[-1] == '\t'))
          --end;
        *end = '\0';
        if (SDL_strcasecmp(cursor, key) == 0) {
          if (!wrote_key && value)
            fprintf(output, "%s=%s\n", key, value);
          wrote_key = 1;
          continue;
        }
      }
    }
    fputs(line, output);
  }

  if (input)
    fclose(input);
  if (!found_section) {
    if (value)
      fprintf(output, "\n[%s]\n%s=%s\n", section, key, value);
  } else if (!wrote_key && value) {
    fprintf(output, "%s=%s\n", key, value);
  }
  if (fclose(output) == 0) {
    remove(path);
    result = rename(temp_path, path) == 0;
  }
  if (!result)
    remove(temp_path);
  return result;
}
