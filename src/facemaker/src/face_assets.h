#ifndef SHANDALAR_FACEMAKER_FACE_ASSETS_H
#define SHANDALAR_FACEMAKER_FACE_ASSETS_H

#include "facemaker_types.h"

#include <stddef.h>

#define FACEMAKER_MAX_FACE_SETS 64
#define FACEMAKER_MAX_GROUPS 10
#define FACEMAKER_MAX_FRAMES 20

typedef struct FacemakerFaceDefinition {
  char display_name[64];
  char file_name[32];
  char gender;
} FacemakerFaceDefinition;

typedef struct FacemakerFaceSpriteSet {
  unsigned char *blob;
  size_t blob_size;
  EncodedImage *base;
  EncodedImage *groups[FACEMAKER_MAX_GROUPS][FACEMAKER_MAX_FRAMES];
  int frame_count[FACEMAKER_MAX_GROUPS];
  int group_count;
} FacemakerFaceSpriteSet;

int facemaker_face_definitions_load(const char *path, FacemakerFaceDefinition *definitions,
                                    int capacity);
int facemaker_sprite_blob_load_entries(const char *path, unsigned char **blob,
                                       size_t *blob_size, EncodedImage **entries,
                                       int entry_capacity);
int facemaker_face_sprite_set_load(const char *path, FacemakerFaceSpriteSet *set);
void facemaker_face_sprite_set_destroy(FacemakerFaceSpriteSet *set);
int facemaker_face_sprite_is_valid(const FacemakerFaceSpriteSet *set,
                                   const EncodedImage *sprite);

#endif
