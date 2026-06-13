#ifndef SHANDALAR_FACEMAKER_CORE_H
#define SHANDALAR_FACEMAKER_CORE_H

#include <stdint.h>

typedef enum FacemakerStatus {
  FACEMAKER_RUNNING,
  FACEMAKER_COMPLETED,
  FACEMAKER_CANCELLED,
  FACEMAKER_FAILED
} FacemakerStatus;

typedef struct FacemakerConfig {
  const char *asset_root;
  const char *writable_root;
  int width;
  int height;
} FacemakerConfig;

typedef struct FacemakerFrame {
  const unsigned char *pixels;
  int width;
  int height;
  int pitch;
  const unsigned char *rgb_palette;
  uint64_t generation;
} FacemakerFrame;

int facemaker_create(const FacemakerConfig *config);
void facemaker_destroy(void);
void facemaker_pointer_move(float x, float y);
void facemaker_pointer_button(int button, int pressed);
void facemaker_key(int key, int pressed, int repeat);
FacemakerStatus facemaker_step(uint64_t now_ns);
int facemaker_get_frame(FacemakerFrame *frame);
int facemaker_save(const char *path);

#endif
