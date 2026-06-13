#include "platform/legacy_io.h"

#ifndef _WIN32
#include <fnmatch.h>
#include <stdlib.h>
#include <string.h>

typedef struct FindState {
  DIR *directory;
  char directory_path[512];
  char pattern[260];
} FindState;

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
#endif
