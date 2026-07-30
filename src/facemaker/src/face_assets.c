#include "face_assets.h"

#include "platform/legacy_io.h"

#include <SDL3/SDL.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int facemaker_face_definitions_load(const char *path, FacemakerFaceDefinition *definitions,
                                    int capacity)
{
  char line[256];
  FILE *file;
  int count = 0;
  if (!path || !definitions || capacity <= 0)
    return 0;
  file = fopen(path, "rt");
  if (!file)
    return 0;
  while (count < capacity && fgets(line, sizeof(line), file)) {
    char *first_comma = strchr(line, ',');
    char *second_comma;
    char *gender;
    if (!first_comma)
      continue;
    second_comma = strchr(first_comma + 1, ',');
    if (!second_comma)
      continue;
    *first_comma = '\0';
    *second_comma = '\0';
    SDL_strlcpy(definitions[count].display_name, line,
                sizeof(definitions[count].display_name));
    do {
      ++first_comma;
    } while (*first_comma == ' ' || *first_comma == '\t');
    SDL_strlcpy(definitions[count].file_name, first_comma,
                sizeof(definitions[count].file_name));
    gender = second_comma + 1;
    while (*gender == ' ' || *gender == '\t')
      ++gender;
    definitions[count].gender = *gender;
    ++count;
  }
  fclose(file);
  return count;
}

int facemaker_sprite_blob_load_entries(const char *path, unsigned char **blob,
                                       size_t *blob_size, EncodedImage **entries,
                                       int entry_capacity)
{
  FILE *file;
  long file_size;
  unsigned char *cursor;
  unsigned char *end;
  int entry_count = 0;

  if (!path || !blob || !blob_size || !entries || entry_capacity <= 0)
    return 0;
  *blob = NULL;
  *blob_size = 0;
  file = fopen(path, "rb");
  if (!file)
    return 0;
  fseek(file, 0, SEEK_END);
  file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  if (file_size <= 0) {
    fclose(file);
    return 0;
  }
  *blob = (unsigned char *)malloc((size_t)file_size);
  if (!*blob) {
    fclose(file);
    return 0;
  }
  *blob_size = (size_t)file_size;
  if (fread(*blob, 1, *blob_size, file) != *blob_size) {
    fclose(file);
    free(*blob);
    *blob = NULL;
    *blob_size = 0;
    return 0;
  }
  fclose(file);

  cursor = *blob;
  end = cursor + *blob_size;
  while (cursor + sizeof(int) <= end && entry_count < entry_capacity) {
    int total_size;
    memcpy(&total_size, cursor, sizeof(total_size));
    if (total_size == -1)
      break;
    if (total_size < (int)offsetof(EncodedImage, spans) || cursor + total_size > end) {
      free(*blob);
      *blob = NULL;
      *blob_size = 0;
      return 0;
    }
    entries[entry_count++] = (EncodedImage *)cursor;
    cursor += total_size;
  }
  return entry_count;
}

int facemaker_face_sprite_set_load(const char *path, FacemakerFaceSpriteSet *set)
{
  EncodedImage *entries[1 + FACEMAKER_MAX_GROUPS * FACEMAKER_MAX_FRAMES];
  int entry_count;
  int entry_index;
  int group = 0;
  int frame = 0;

  if (!path || !set)
    return 0;
  facemaker_face_sprite_set_destroy(set);
  entry_count = facemaker_sprite_blob_load_entries(
      path, &set->blob, &set->blob_size, entries,
      (int)(sizeof(entries) / sizeof(entries[0])));
  if (entry_count < 1) {
    facemaker_face_sprite_set_destroy(set);
    return 0;
  }

  set->base = entries[0];
  for (entry_index = 1; entry_index < entry_count && group < FACEMAKER_MAX_GROUPS;
       ++entry_index) {
    EncodedImage *entry = entries[entry_index];
    if (frame < FACEMAKER_MAX_FRAMES)
      set->groups[group][frame++] = entry;
    if (entry->left_clip == 0) {
      set->frame_count[group++] = frame;
      frame = 0;
      if (entry->top_clip == 0)
        break;
    }
  }
  if (frame > 0 && group < FACEMAKER_MAX_GROUPS)
    set->frame_count[group++] = frame;
  set->group_count = group;
  return group > 0;
}

void facemaker_face_sprite_set_destroy(FacemakerFaceSpriteSet *set)
{
  if (!set)
    return;
  free(set->blob);
  memset(set, 0, sizeof(*set));
}

int facemaker_face_sprite_is_valid(const FacemakerFaceSpriteSet *set,
                                   const EncodedImage *sprite)
{
  const unsigned char *address = (const unsigned char *)sprite;
  return set && set->blob && address >= set->blob &&
         address + sizeof(EncodedImage) <= set->blob + set->blob_size;
}
