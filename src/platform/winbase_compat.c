#include "platform/winuser_compat.h"
#ifndef _WIN32
#include "platform/legacy_io.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

typedef struct ProcEntry {
  struct ProcEntry *next;
  char *name;
  FARPROC proc;
} ProcEntry;

static ProcEntry *g_procs;
static int g_main_module;
static int g_process_sentinel;

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

BOOL SetCurrentDirectoryA(LPCSTR path)
{
  return path &&
#ifdef _WIN32
         _chdir(path) == 0;
#else
         chdir(path) == 0;
#endif
}

void Sleep(DWORD milliseconds) { SDL_Delay(milliseconds); }

DWORD GetTickCount(void)
{
  return (DWORD)SDL_GetTicks();
}

HANDLE GetCurrentProcess(void)
{
  return (HANDLE)&g_process_sentinel;
}

HANDLE GetCurrentThread(void)
{
  return (HANDLE)(uintptr_t)SDL_GetCurrentThreadID();
}

BOOL DuplicateHandle(HANDLE source_process, HANDLE source_handle,
                     HANDLE target_process, HANDLE *target_handle,
                     DWORD access, BOOL inherit, DWORD options)
{
  (void)source_process;
  (void)target_process;
  (void)access;
  (void)inherit;
  (void)options;
  if (!target_handle)
    return FALSE;
  *target_handle = source_handle;
  return TRUE;
}

void ExitThread(DWORD exit_code)
{
  (void)exit_code;
}

LONG ChangeDisplaySettingsA(DEVMODEA *dev_mode, DWORD flags)
{
  (void)dev_mode;
  (void)flags;
  return 0;
}

HANDLE CreateFileA(LPCSTR filename, DWORD desired_access, DWORD share_mode,
                   LPSECURITY_ATTRIBUTES security_attributes,
                   DWORD creation_disposition, DWORD flags_and_attributes,
                   HANDLE template_file)
{
  FILE *file;
  (void)desired_access;
  (void)share_mode;
  (void)security_attributes;
  (void)creation_disposition;
  (void)flags_and_attributes;
  (void)template_file;
#ifdef _WIN32
  file = fopen(filename, "rb");
#else
  file = shandalar_fopen(filename, "rb");
#endif
  return file ? (HANDLE)file : INVALID_HANDLE_VALUE;
}

DWORD GetFileSize(HANDLE file, DWORD *high_size)
{
  FILE *stream = (FILE *)file;
  long current;
  long end;
  if (high_size)
    *high_size = 0;
  if (!stream || stream == (FILE *)INVALID_HANDLE_VALUE)
    return 0xffffffffu;
  current = ftell(stream);
  if (fseek(stream, 0, SEEK_END) != 0)
    return 0xffffffffu;
  end = ftell(stream);
  fseek(stream, current, SEEK_SET);
  return end < 0 ? 0xffffffffu : (DWORD)end;
}

BOOL ReadFile(HANDLE file, LPVOID buffer, DWORD bytes_to_read,
              DWORD *bytes_read, LPOVERLAPPED overlapped)
{
  FILE *stream = (FILE *)file;
  size_t count;
  (void)overlapped;
  if (bytes_read)
    *bytes_read = 0;
  if (!stream || stream == (FILE *)INVALID_HANDLE_VALUE || !buffer)
    return FALSE;
  count = fread(buffer, 1, bytes_to_read, stream);
  if (bytes_read)
    *bytes_read = (DWORD)count;
  return count == bytes_to_read || feof(stream);
}

BOOL DeviceIoControl(HANDLE device, DWORD io_control_code, LPVOID in_buffer,
                     DWORD in_buffer_size, LPVOID out_buffer,
                     DWORD out_buffer_size, DWORD *bytes_returned,
                     LPOVERLAPPED overlapped)
{
  (void)device;
  (void)io_control_code;
  (void)in_buffer;
  (void)in_buffer_size;
  (void)out_buffer;
  (void)out_buffer_size;
  (void)overlapped;
  if (bytes_returned)
    *bytes_returned = 0;
  return FALSE;
}

UINT GetDriveTypeA(LPCSTR root_path_name)
{
  (void)root_path_name;
  return 3;
}

UINT GetWindowsDirectoryA(LPSTR buffer, UINT size)
{
  if (!buffer || size == 0)
    return 0;
  SDL_strlcpy(buffer, "C:\\Windows", size);
  return (UINT)strlen(buffer);
}

DWORD FormatMessageA(DWORD flags, LPCVOID source, DWORD message_id, DWORD language_id,
                     LPSTR buffer, DWORD size, va_list *arguments)
{
  const char *format = (const char *)source;
  const char *scan;
  uintptr_t inserts[16];
  DWORD out = 0;
  int max_insert = 0;
  int i;
  (void)flags;
  (void)message_id;
  (void)language_id;
  if (!format || !buffer || size == 0)
    return 0;

  for (i = 0; i < 16; ++i)
    inserts[i] = 0;
  for (scan = format; *scan; ++scan) {
    if (*scan == '%' && isdigit((unsigned char)scan[1])) {
      int index = 0;
      ++scan;
      while (isdigit((unsigned char)*scan)) {
        index = index * 10 + (*scan - '0');
        ++scan;
      }
      if (index > max_insert && index <= 16)
        max_insert = index;
      if (*scan == '\0')
        break;
    }
  }
  if (arguments) {
    va_list args_copy;
    va_copy(args_copy, *arguments);
    for (i = 0; i < max_insert; ++i)
      inserts[i] = va_arg(args_copy, uintptr_t);
    va_end(args_copy);
  }

  while (*format && out + 1 < size) {
    if (*format != '%' || format[1] == '\0') {
      buffer[out++] = *format++;
      continue;
    }
    ++format;
    if (*format == '%') {
      buffer[out++] = *format++;
      continue;
    }
    if (isdigit((unsigned char)*format)) {
      int index = 0;
      char specifier = 's';
      while (isdigit((unsigned char)*format)) {
        index = index * 10 + (*format - '0');
        ++format;
      }
      if (*format == '!') {
        ++format;
        if (*format)
          specifier = (char)tolower((unsigned char)*format);
        while (*format && *format != '!')
          ++format;
        if (*format == '!')
          ++format;
      }
      if (index > 0 && index <= 16) {
        char scratch[64];
        const char *text = (const char *)inserts[index - 1];
        if (specifier == 'd' || specifier == 'i') {
          SDL_snprintf(scratch, sizeof(scratch), "%d", (int)inserts[index - 1]);
          text = scratch;
        } else if (specifier == 'u') {
          SDL_snprintf(scratch, sizeof(scratch), "%u", (unsigned int)inserts[index - 1]);
          text = scratch;
        }
        if (text) {
          while (*text && out + 1 < size)
            buffer[out++] = *text++;
        }
      }
      continue;
    }
    buffer[out++] = '%';
  }
  buffer[out] = '\0';
  return out;
}

UINT SetSystemPaletteUse(HDC dc, UINT use)
{
  (void)dc;
  return use;
}

UINT timeBeginPeriod(UINT period) { (void)period; return 0; }
UINT timeEndPeriod(UINT period) { (void)period; return 0; }

#define MAX_COMPAT_MM_TIMERS 16

typedef struct CompatMmTimer {
  UINT id;
  UINT interval;
  LPTIMECALLBACK callback;
  DWORD user;
  UINT event;
  uint64_t next_tick;
} CompatMmTimer;

static CompatMmTimer g_mm_timers[MAX_COMPAT_MM_TIMERS];
static UINT g_next_mm_timer_id = 1;

UINT timeSetEvent(UINT delay, UINT resolution, LPTIMECALLBACK callback,
                  DWORD user, UINT event)
{
  size_t i;
  (void)resolution;
  if (!callback)
    return 0;
  if (!delay)
    delay = 1;
  for (i = 0; i < MAX_COMPAT_MM_TIMERS; ++i) {
    CompatMmTimer *timer = &g_mm_timers[i];
    if (!timer->id) {
      timer->id = g_next_mm_timer_id++;
      if (!g_next_mm_timer_id)
        g_next_mm_timer_id = 1;
      timer->interval = delay;
      timer->callback = callback;
      timer->user = user;
      timer->event = event;
      timer->next_tick = SDL_GetTicks() + delay;
      return timer->id;
    }
  }
  return 0;
}

UINT timeKillEvent(UINT timer_id)
{
  size_t i;
  for (i = 0; i < MAX_COMPAT_MM_TIMERS; ++i) {
    if (g_mm_timers[i].id == timer_id) {
      memset(&g_mm_timers[i], 0, sizeof(g_mm_timers[i]));
      return 0;
    }
  }
  return 1;
}

void shandalar_win32_mm_timers_tick(void)
{
  uint64_t now = SDL_GetTicks();
  size_t i;
  for (i = 0; i < MAX_COMPAT_MM_TIMERS; ++i) {
    CompatMmTimer *timer = &g_mm_timers[i];
    if (!timer->id || !timer->callback)
      continue;
    if (now >= timer->next_tick) {
      UINT id = timer->id;
      timer->next_tick = now + timer->interval;
      timer->callback(id, 0, timer->user, 0, 0);
    }
  }
}

void shandalar_win32_mm_timers_reset(void)
{
  memset(g_mm_timers, 0, sizeof(g_mm_timers));
  g_next_mm_timer_id = 1;
}

BOOL GetSaveFileNameA(OPENFILENAMEA *open_file)
{
  if (!open_file || !open_file->lpstrFile || open_file->nMaxFile == 0)
    return FALSE;
  if (open_file->lpstrFile[0] == '\0' || open_file->lpstrFile[0] == '*')
    SDL_strlcpy(open_file->lpstrFile, "face.pic", open_file->nMaxFile);
  return TRUE;
}

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
