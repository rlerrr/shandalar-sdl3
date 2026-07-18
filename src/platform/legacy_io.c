#define SHANDALAR_LEGACY_IO_IMPLEMENTATION
#include "platform/legacy_io.h"
#include "platform/asset_manager.h"

#ifndef _WIN32
#include <fnmatch.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

typedef struct FindState {
  DIR *directory;
  char directory_path[512];
  char pattern[260];
} FindState;

typedef struct LegacyFileMode {
  FILE *file;
  int text_mode;
  int sync_on_close;
  struct LegacyFileMode *next;
} LegacyFileMode;

static LegacyFileMode *legacy_file_modes;

typedef struct LegacyFdMode {
  int fd;
  int sync_on_close;
  struct LegacyFdMode *next;
} LegacyFdMode;

static LegacyFdMode *legacy_fd_modes;

static void normalize_path_copy(char *out, size_t out_size, const char *path);

#ifdef __EMSCRIPTEN__
EM_JS(int, shandalar_wasm_user_data_mount_poll, (), {
  if (typeof FS === 'undefined' || typeof IDBFS === 'undefined')
    return -1;
  if (Module.__shandalarUserDataReady === 1)
    return 1;
  if (Module.__shandalarUserDataReady === -1)
    return -1;
  if (Module.__shandalarUserDataReady === undefined) {
    Module.__shandalarUserDataReady = 0;
    try {
      try { FS.mkdir('/userdata'); } catch (e) {}
      try { FS.mount(IDBFS, {}, '/userdata'); } catch (e) {
        if (!String(e).includes('already mounted'))
          throw e;
      }
      FS.syncfs(true, function(err) {
        if (err)
          console.error('shandalar: could not load /userdata from IndexedDB', err);
        Module.__shandalarUserDataReady = err ? -1 : 1;
      });
    } catch (e) {
      console.error('shandalar: could not mount /userdata', e);
      Module.__shandalarUserDataReady = -1;
    }
  }
  return 0;
});

EM_JS(int, shandalar_wasm_user_data_sync_poll, (), {
  if (Module.__shandalarUserDataReady !== 1)
    return -1;
  if (Module.__shandalarUserDataSyncState === 1)
    return 0;
  if (Module.__shandalarUserDataSyncState === 2) {
    Module.__shandalarUserDataSyncState = 0;
    return 1;
  }
  if (Module.__shandalarUserDataSyncState === -1) {
    Module.__shandalarUserDataSyncState = 0;
    return -1;
  }
  Module.__shandalarUserDataSyncState = 1;
  FS.syncfs(false, function(err) {
    if (err)
      console.error('shandalar: could not save /userdata to IndexedDB', err);
    Module.__shandalarUserDataSyncState = err ? -1 : 2;
  });
  return 0;
});

static int ensure_user_data_ready(void)
{
  int state;
  do {
    state = shandalar_wasm_user_data_mount_poll();
    if (state == 0)
      emscripten_sleep(1);
  } while (state == 0);
  return state == 1;
}
#else
static int ensure_user_data_ready(void)
{
  return 1;
}
#endif

const char *shandalar_user_data_root(void)
{
#ifdef __EMSCRIPTEN__
  ensure_user_data_ready();
  return "/userdata";
#else
  return ".";
#endif
}

int shandalar_user_data_sync(void)
{
#ifdef __EMSCRIPTEN__
  int state;
  if (!ensure_user_data_ready())
    return 0;
  do {
    state = shandalar_wasm_user_data_sync_poll();
    if (state == 0)
      emscripten_sleep(1);
  } while (state == 0);
  return state == 1;
#else
  return 1;
#endif
}

static int is_text_mode(const char *mode)
{
  return !mode || !strchr(mode, 'b');
}

static int is_write_mode(const char *mode)
{
  return mode && (strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+'));
}

static void register_file_mode(FILE *file, const char *mode, int sync_on_close)
{
  LegacyFileMode *entry;

  if (!file)
    return;

  entry = (LegacyFileMode *)malloc(sizeof(*entry));
  if (!entry)
    return;

  entry->file = file;
  entry->text_mode = is_text_mode(mode);
  entry->sync_on_close = sync_on_close;
  entry->next = legacy_file_modes;
  legacy_file_modes = entry;
}

static int unregister_file_mode(FILE *file)
{
  LegacyFileMode **entry = &legacy_file_modes;

  while (*entry) {
    if ((*entry)->file == file) {
      LegacyFileMode *removed = *entry;
      int sync_on_close = removed->sync_on_close;
      *entry = removed->next;
      free(removed);
      return sync_on_close;
    }
    entry = &(*entry)->next;
  }

  return 0;
}

static int file_is_text_mode(FILE *file)
{
  LegacyFileMode *entry;

  for (entry = legacy_file_modes; entry; entry = entry->next) {
    if (entry->file == file)
      return entry->text_mode;
  }

  return 0;
}

static void register_fd_mode(int fd, int sync_on_close)
{
  LegacyFdMode *entry;

  if (fd < 0)
    return;

  entry = (LegacyFdMode *)malloc(sizeof(*entry));
  if (!entry)
    return;

  entry->fd = fd;
  entry->sync_on_close = sync_on_close;
  entry->next = legacy_fd_modes;
  legacy_fd_modes = entry;
}

static int unregister_fd_mode(int fd)
{
  LegacyFdMode **entry = &legacy_fd_modes;

  while (*entry) {
    if ((*entry)->fd == fd) {
      LegacyFdMode *removed = *entry;
      int sync_on_close = removed->sync_on_close;
      *entry = removed->next;
      free(removed);
      return sync_on_close;
    }
    entry = &(*entry)->next;
  }

  return 0;
}

#ifdef __EMSCRIPTEN__
static int ascii_equal_ignore_case(const char *a, const char *b)
{
  while (*a && *b) {
    if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
      return 0;
    ++a;
    ++b;
  }
  return *a == *b;
}

static int is_hex_save_slot(char c)
{
  c = (char)tolower((unsigned char)c);
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

static int map_user_data_path(const char *path, char *out, size_t out_size)
{
  char normalized[1024];
  const char *base;
  const char *drive;
  char name[260];
  size_t i;

  normalize_path_copy(normalized, sizeof(normalized), path);
  if (strncmp(normalized, "/userdata/", 10) == 0) {
    snprintf(out, out_size, "%s", normalized);
    return 1;
  }

  base = strrchr(normalized, '/');
  base = base ? base + 1 : normalized;
  drive = strchr(base, ':');
  if (drive && drive[1])
    base = drive + 1;

  if (ascii_equal_ignore_case(base, "face.pic")) {
    snprintf(out, out_size, "/userdata/face.pic");
  } else if (ascii_equal_ignore_case(base, "FaceMostRecent.txt")) {
    snprintf(out, out_size, "/userdata/FaceMostRecent.txt");
  } else if (ascii_equal_ignore_case(base, "saveDescs")) {
    snprintf(out, out_size, "/userdata/saveDescs");
  } else if (ascii_equal_ignore_case(base, "registry.ini")) {
    snprintf(out, out_size, "/userdata/registry.ini");
  } else if (strlen(base) == 10 &&
             tolower((unsigned char)base[0]) == 'm' &&
             tolower((unsigned char)base[1]) == 'a' &&
             tolower((unsigned char)base[2]) == 'g' &&
             tolower((unsigned char)base[3]) == 'i' &&
             tolower((unsigned char)base[4]) == 'c' &&
             is_hex_save_slot(base[5]) &&
             base[6] == '.') {
    snprintf(name, sizeof(name), "magic%c.%c%c%c",
             (char)tolower((unsigned char)base[5]),
             (char)tolower((unsigned char)base[7]),
             (char)tolower((unsigned char)base[8]),
             (char)tolower((unsigned char)base[9]));
    if (!(ascii_equal_ignore_case(name + 7, "sve") ||
          ascii_equal_ignore_case(name + 7, "fce") ||
          ascii_equal_ignore_case(name + 7, "map")))
      return 0;
    snprintf(out, out_size, "/userdata/%s", name);
  } else {
    return 0;
  }

  for (i = 0; out[i]; ++i) {
    if (out[i] == '\\')
      out[i] = '/';
  }
  ensure_user_data_ready();
  return 1;
}
#else
static int map_user_data_path(const char *path, char *out, size_t out_size)
{
  (void)path;
  (void)out;
  (void)out_size;
  return 0;
}
#endif

static void normalize_path_copy(char *out, size_t out_size, const char *path)
{
  size_t i;
  if (!out || !out_size)
    return;
  snprintf(out, out_size, "%s", path ? path : "");
  for (i = 0; out[i]; ++i) {
    if (out[i] == '\\')
      out[i] = '/';
  }
}

static int make_asset_relative_path(const char *path, char *out, size_t out_size)
{
  char normalized[1024];
  char root[1024];
  size_t root_len;

  normalize_path_copy(normalized, sizeof(normalized), path);
  normalize_path_copy(root, sizeof(root), shandalar_assets_root());
  root_len = strlen(root);

  while (root_len > 1 && root[root_len - 1] == '/')
    root[--root_len] = '\0';

  if (normalized[0] == '/') {
    if (strncmp(normalized, root, root_len) != 0)
      return 0;
    if (normalized[root_len] != '/' && normalized[root_len] != '\0')
      return 0;
    snprintf(out, out_size, "%s", normalized[root_len] == '/' ? normalized + root_len + 1 : "");
    return out[0] != '\0';
  }

  snprintf(out, out_size, "%s", normalized);
  return out[0] != '\0';
}

static int fetch_asset_for_path(const char *path)
{
  char relative[1024];
  if (!make_asset_relative_path(path, relative, sizeof(relative)))
    return 0;
  return shandalar_assets_fetch_blocking(relative) == SHANDALAR_ASSET_READY;
}

static void log_open_failure(const char *kind, const char *path, const char *mode_or_flags)
{
#ifdef __EMSCRIPTEN__
  char escaped_path[1024];
  char escaped_extra[128];
  char script[1400];
  size_t i;
  size_t out = 0;

  for (i = 0; path && path[i] && out + 2 < sizeof(escaped_path); ++i) {
    if (path[i] == '\\' || path[i] == '\'')
      escaped_path[out++] = '\\';
    escaped_path[out++] = path[i];
  }
  escaped_path[out] = '\0';
  out = 0;
  for (i = 0; mode_or_flags && mode_or_flags[i] && out + 2 < sizeof(escaped_extra); ++i) {
    if (mode_or_flags[i] == '\\' || mode_or_flags[i] == '\'')
      escaped_extra[out++] = '\\';
    escaped_extra[out++] = mode_or_flags[i];
  }
  escaped_extra[out] = '\0';
  snprintf(script, sizeof(script), "console.warn('deck: %s failed', '%s', '%s');",
           kind ? kind : "open", escaped_path, escaped_extra);
  emscripten_run_script(script);
#else
  (void)kind;
  (void)path;
  (void)mode_or_flags;
#endif
}

static int resolve_case_insensitive_path(const char *path, char *out, size_t out_size)
{
  char normalized[1024];
  char current[1024];
  const char *cursor;

  normalize_path_copy(normalized, sizeof(normalized), path);
  if (access(normalized, F_OK) == 0) {
    snprintf(out, out_size, "%s", normalized);
    return 1;
  }

  cursor = normalized;
  if (*cursor == '/') {
    snprintf(current, sizeof(current), "/");
    while (*cursor == '/')
      ++cursor;
  } else {
    snprintf(current, sizeof(current), ".");
  }

  while (*cursor) {
    char component[260];
    char directory[1024];
    const char *slash;
    struct dirent *entry;
    DIR *dir;
    size_t component_len;
    int found = 0;

    slash = strchr(cursor, '/');
    component_len = slash ? (size_t)(slash - cursor) : strlen(cursor);
    if (component_len >= sizeof(component))
      return 0;
    memcpy(component, cursor, component_len);
    component[component_len] = '\0';

    if (component[0] == '\0' || strcmp(component, ".") == 0) {
      cursor += component_len;
      while (*cursor == '/')
        ++cursor;
      continue;
    }

    snprintf(directory, sizeof(directory), "%s", current);
    dir = opendir(directory);
    if (!dir)
      return 0;

    while ((entry = readdir(dir)) != NULL) {
      if (strcasecmp(entry->d_name, component) == 0) {
        size_t len = strlen(current);
        if (strcmp(current, "/") == 0)
          snprintf(current, sizeof(current), "/%s", entry->d_name);
        else
          snprintf(current + len, sizeof(current) - len, "/%s", entry->d_name);
        found = 1;
        break;
      }
    }
    closedir(dir);

    if (!found)
      return 0;

    cursor += component_len;
    while (*cursor == '/')
      ++cursor;
  }

  snprintf(out, out_size, "%s", current);
  return 1;
}

int shandalar_resolve_existing_path(const char *path, char *out, size_t out_size)
{
  char normalized[1024];
  normalize_path_copy(normalized, sizeof(normalized), path);
  if (resolve_case_insensitive_path(normalized, out, out_size))
    return 1;
  snprintf(out, out_size, "%s", normalized);
  return 0;
}

static int translate_legacy_open_flags(int flags)
{
  int translated;

  switch (flags & 0x0003) {
  case 0x0001:
    translated = O_WRONLY;
    break;
  case 0x0002:
    translated = O_RDWR;
    break;
  default:
    translated = O_RDONLY;
    break;
  }

  if ((flags & 0x0100) != 0 || (flags & O_CREAT) != 0)
    translated |= O_CREAT;
  if ((flags & 0x0200) != 0 || (flags & O_TRUNC) != 0)
    translated |= O_TRUNC;
#ifdef O_EXCL
  if ((flags & 0x0400) != 0 || (flags & O_EXCL) != 0)
    translated |= O_EXCL;
#endif
#ifdef O_APPEND
  if ((flags & 0x0008) != 0 || (flags & O_APPEND) != 0)
    translated |= O_APPEND;
#endif

  return translated;
}

static mode_t translate_legacy_open_mode(int flags, mode_t mode)
{
  mode_t translated = 0;

  if ((mode & 0x0100) != 0)
    translated |= S_IRUSR;
  if ((mode & 0x0080) != 0)
    translated |= S_IWUSR;
  if ((mode & 0x0020) != 0)
    translated |= S_IXUSR;

  if ((flags & 0x0003) != O_RDONLY)
    translated |= S_IRUSR | S_IWUSR;
  if (translated == 0)
    translated = S_IRUSR | S_IWUSR;
  return translated;
}

FILE *shandalar_fopen(const char *path, const char *mode)
{
  char normalized[1024];
  FILE *file;
  int user_data_file = map_user_data_path(path, normalized, sizeof(normalized));
  if (!user_data_file)
    normalize_path_copy(normalized, sizeof(normalized), path);
  if (!user_data_file && mode && mode[0] == 'r') {
    if (!shandalar_resolve_existing_path(normalized, normalized, sizeof(normalized))) {
      fetch_asset_for_path(normalized);
      shandalar_resolve_existing_path(normalized, normalized, sizeof(normalized));
    }
  }
  file = fopen(normalized, mode);
  if (!file && mode && mode[0] == 'r')
    log_open_failure("fopen", normalized, mode);
  register_file_mode(file, mode, user_data_file && is_write_mode(mode));
  return file;
}

int shandalar_fclose(FILE *stream)
{
  int sync_on_close = unregister_file_mode(stream);
  int result = fclose(stream);
  if (sync_on_close)
    shandalar_user_data_sync();
  return result;
}

char *shandalar_fgets(char *str, int count, FILE *stream)
{
  char *result = fgets(str, count, stream);
  char *newline;

  if (!result || !file_is_text_mode(stream))
    return result;

  newline = strchr(str, '\n');
  if (newline && newline > str && newline[-1] == '\r')
    memmove(newline - 1, newline, strlen(newline) + 1);

  return result;
}

int shandalar_open(const char *path, int flags, ...)
{
  char normalized[1024];
  int translated_flags;
  mode_t mode = 0;
  int user_data_file;
  translated_flags = translate_legacy_open_flags(flags);
  if (translated_flags & O_CREAT) {
    va_list args;
    va_start(args, flags);
    mode = translate_legacy_open_mode(flags, (mode_t)va_arg(args, int));
    va_end(args);
  }
  user_data_file = map_user_data_path(path, normalized, sizeof(normalized));
  if (!user_data_file)
    normalize_path_copy(normalized, sizeof(normalized), path);
  if (!user_data_file && (translated_flags & O_CREAT) == 0) {
    if (!shandalar_resolve_existing_path(normalized, normalized, sizeof(normalized))) {
      fetch_asset_for_path(normalized);
      shandalar_resolve_existing_path(normalized, normalized, sizeof(normalized));
    }
  }
  {
    int fd = open(normalized, translated_flags, mode);
    if (fd < 0 && (translated_flags & O_CREAT) == 0)
      log_open_failure("open", normalized, "");
    if (fd >= 0)
      register_fd_mode(fd, user_data_file && (translated_flags & (O_WRONLY | O_RDWR)));
    return fd;
  }
}

int shandalar_close(int fd)
{
  int sync_on_close = unregister_fd_mode(fd);
  int result = close(fd);
  if (sync_on_close)
    shandalar_user_data_sync();
  return result;
}

int shandalar_chdir(const char *path)
{
  char normalized[1024];
  shandalar_resolve_existing_path(path, normalized, sizeof(normalized));
  return chdir(normalized);
}

long shandalar_filelength(int fd)
{
  struct stat st;
  if (fstat(fd, &st) != 0)
    return -1;
  return (long)st.st_size;
}

static int find_next_match(FindState *state, struct _finddata_t *data)
{
  struct dirent *entry;
  while ((entry = readdir(state->directory)) != NULL) {
    if (fnmatch(state->pattern, entry->d_name, FNM_CASEFOLD) == 0) {
      snprintf(data->name, sizeof(data->name), "%s", entry->d_name);
      return 0;
    }
  }
  return -1;
}

intptr_find_t shandalar_findfirst(const char *pattern, struct _finddata_t *data)
{
  FindState *state;
  const char *slash;
  size_t directory_length;
  char normalized_pattern[1024];
  char relative[1024];
  normalize_path_copy(normalized_pattern, sizeof(normalized_pattern), pattern);
  if (!strchr(normalized_pattern, '*') && !strchr(normalized_pattern, '?') &&
      !shandalar_resolve_existing_path(normalized_pattern, normalized_pattern, sizeof(normalized_pattern)) &&
      make_asset_relative_path(normalized_pattern, relative, sizeof(relative))) {
    shandalar_assets_fetch_blocking(relative);
  }
  state = (FindState *)calloc(1, sizeof(*state));
  if (!state)
    return -1;
  slash = strrchr(pattern, '/');
  if (!slash)
    slash = strrchr(pattern, '\\');
  if (slash) {
    directory_length = (size_t)(slash - pattern);
    if (directory_length >= sizeof(state->directory_path))
      directory_length = sizeof(state->directory_path) - 1;
    memcpy(state->directory_path, pattern, directory_length);
    state->directory_path[directory_length] = '\0';
    shandalar_resolve_existing_path(state->directory_path, state->directory_path,
                                    sizeof(state->directory_path));
    snprintf(state->pattern, sizeof(state->pattern), "%s", slash + 1);
  } else {
    snprintf(state->directory_path, sizeof(state->directory_path), ".");
    snprintf(state->pattern, sizeof(state->pattern), "%s", pattern);
  }
  state->directory = opendir(state->directory_path);
  if (!state->directory || find_next_match(state, data) != 0) {
    if (state->directory) closedir(state->directory);
    free(state);
    return -1;
  }
  return (intptr_find_t)state;
}

int shandalar_findnext(intptr_find_t handle, struct _finddata_t *data)
{
  return find_next_match((FindState *)handle, data);
}

int shandalar_findclose(intptr_find_t handle)
{
  FindState *state = (FindState *)handle;
  if (!state)
    return -1;
  closedir(state->directory);
  free(state);
  return 0;
}

char *shandalar_strlwr(char *str)
{
  char *p = str;
  if (!str)
    return str;
  while (*p) {
    *p = (char)tolower((unsigned char)*p);
    ++p;
  }
  return str;
}

void shandalar_splitpath(const char *path, char *drive, char *dir, char *fname, char *ext)
{
  const char *slash;
  const char *backslash;
  const char *name_start;
  const char *dot;
  size_t dir_len;
  size_t name_len;

  if (drive)
    drive[0] = '\0';
  if (dir)
    dir[0] = '\0';
  if (fname)
    fname[0] = '\0';
  if (ext)
    ext[0] = '\0';
  if (!path)
    return;

  slash = strrchr(path, '/');
  backslash = strrchr(path, '\\');
  if (!slash || (backslash && backslash > slash))
    slash = backslash;
  name_start = slash ? slash + 1 : path;
  dot = strrchr(name_start, '.');

  if (dir && slash) {
    dir_len = (size_t)(slash - path) + 1;
    memcpy(dir, path, dir_len);
    dir[dir_len] = '\0';
  }

  if (dot) {
    if (ext)
      strcpy(ext, dot);
    name_len = (size_t)(dot - name_start);
  } else {
    name_len = strlen(name_start);
  }

  if (fname) {
    memcpy(fname, name_start, name_len);
    fname[name_len] = '\0';
  }
}
#endif
