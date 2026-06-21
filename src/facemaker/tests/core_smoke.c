#include "facemaker_core.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned long long frame_checksum(const FacemakerFrame *frame)
{
  unsigned long long checksum = 1469598103934665603ull;
  int x;
  int y;
  for (y = 0; y < frame->height; ++y) {
    for (x = 0; x < frame->width; ++x) {
      checksum ^= frame->pixels[y * frame->pitch + x];
      checksum *= 1099511628211ull;
    }
  }
  return checksum;
}

static int file_is_nonempty(const char *path)
{
  FILE *file = fopen(path, "rb");
  long size;
  if (!file)
    return 0;
  fseek(file, 0, SEEK_END);
  size = ftell(file);
  fclose(file);
  return size > 0;
}

static int write_frame_bmp(const char *path, const FacemakerFrame *frame)
{
  unsigned char *rgb;
  SDL_Surface *surface;
  int result;
  int x;
  int y;
  rgb = (unsigned char *)malloc((size_t)frame->width * (size_t)frame->height * 3);
  if (!rgb)
    return 0;
  for (y = 0; y < frame->height; ++y) {
    const unsigned char *source =
        frame->pixels + (frame->height - 1 - y) * frame->pitch;
    for (x = 0; x < frame->width; ++x) {
      const unsigned char *color = frame->rgb_palette + source[x] * 3;
      unsigned char *target = rgb + (y * frame->width + x) * 3;
      target[0] = color[0];
      target[1] = color[1];
      target[2] = color[2];
    }
  }
  surface = SDL_CreateSurfaceFrom(frame->width, frame->height, SDL_PIXELFORMAT_RGB24,
                                  rgb, frame->width * 3);
  result = surface && SDL_SaveBMP(surface, path);
  SDL_DestroySurface(surface);
  free(rgb);
  return result;
}

int main(int argc, char **argv)
{
  FacemakerConfig config;
  FacemakerFrame frame;
  unsigned int pixel_or = 0;
  unsigned int palette_or = 0;
  unsigned long long initial_checksum;
  unsigned long long next_checksum;
  char output_path[1024];
  int x;
  int y;

  if (argc < 2 || !argv[1][0]) {
    fprintf(stderr, "asset directory argument is required\n");
    return 2;
  }

  config.asset_root = argv[1];
  config.writable_root = ".";
  config.width = 1024;
  config.height = 768;
  config.download_on_save = 0;
  if (!facemaker_create(&config)) {
    fprintf(stderr, "facemaker_create failed\n");
    return 1;
  }
  if (!facemaker_get_frame(&frame) || !frame.pixels || !frame.rgb_palette ||
      frame.width != 1024 || frame.height != 768 || frame.pitch < frame.width) {
    fprintf(stderr, "invalid initial frame\n");
    facemaker_destroy();
    return 1;
  }
  for (y = 0; y < frame.height; ++y)
    for (x = 0; x < frame.width; ++x)
      pixel_or |= frame.pixels[y * frame.pitch + x];
  for (x = 0; x < 256 * 3; ++x)
    palette_or |= frame.rgb_palette[x];
  if (!pixel_or || !palette_or) {
    fprintf(stderr, "decoded frame is empty\n");
    facemaker_destroy();
    return 1;
  }
  initial_checksum = frame_checksum(&frame);
  facemaker_key(SDLK_RIGHT, 1, 0);
  if (!facemaker_get_frame(&frame)) {
    fprintf(stderr, "frame unavailable after face-set change\n");
    facemaker_destroy();
    return 1;
  }
  next_checksum = frame_checksum(&frame);
  if (initial_checksum == next_checksum) {
    fprintf(stderr, "face-set change did not alter the frame\n");
    facemaker_destroy();
    return 1;
  }
  if (argc > 2 && !write_frame_bmp(argv[2], &frame)) {
    fprintf(stderr, "frame snapshot failed\n");
    facemaker_destroy();
    return 1;
  }
  snprintf(output_path, sizeof(output_path), "facemaker-core-smoke.pic");
  if (!facemaker_save(output_path) || !file_is_nonempty(output_path)) {
    fprintf(stderr, "face export failed\n");
    facemaker_destroy();
    return 1;
  }
  if (!facemaker_get_frame(&frame) || frame_checksum(&frame) == next_checksum) {
    fprintf(stderr, "save did not redraw the frame\n");
    facemaker_destroy();
    return 1;
  }
  remove(output_path);
  if (facemaker_step(0) != FACEMAKER_RUNNING) {
    fprintf(stderr, "initial state is not running\n");
    facemaker_destroy();
    return 1;
  }
  facemaker_pointer_move(350.0f, 150.0f);
  facemaker_pointer_button(1, 1);
  facemaker_pointer_button(1, 0);
  if (facemaker_step(0) != FACEMAKER_COMPLETED) {
    fprintf(stderr, "leave control did not complete the session\n");
    facemaker_destroy();
    return 1;
  }

  facemaker_destroy();

  config.width = 2048;
  config.height = 1536;
  if (!facemaker_create(&config) || !facemaker_get_frame(&frame) ||
      frame.width != 1024 || frame.height != 768) {
    fprintf(stderr, "recreate or resolution cap failed\n");
    facemaker_destroy();
    return 1;
  }
  facemaker_key(SDLK_ESCAPE, 1, 0);
  if (facemaker_step(0) != FACEMAKER_CANCELLED) {
    fprintf(stderr, "escape did not cancel the recreated session\n");
    facemaker_destroy();
    return 1;
  }
  facemaker_destroy();
  return 0;
}
