#define SHANDALAR_LEGACY_IO_IMPLEMENTATION
#include "platform/legacy_io.h"

#ifndef _WIN32
#include <fnmatch.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

typedef struct FindState {
  DIR *directory;
  char directory_path[512];
  char pattern[260];
} FindState;

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

FILE *shandalar_fopen(const char *path, const char *mode)
{
  char normalized[1024];
  normalize_path_copy(normalized, sizeof(normalized), path);
  return fopen(normalized, mode);
}

int shandalar_open(const char *path, int flags, ...)
{
  char normalized[1024];
  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_list args;
    va_start(args, flags);
    mode = (mode_t)va_arg(args, int);
    va_end(args);
  }
  normalize_path_copy(normalized, sizeof(normalized), path);
  return open(normalized, flags, mode);
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
