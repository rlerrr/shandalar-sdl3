#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdio.h>

static int test_png(const char *directory)
{
  char path[1024];
  SDL_Surface *surface;
  SDL_snprintf(path, sizeof(path), "%s/WIZBG.png", directory);
  surface = IMG_Load(path);
  if (!surface) {
    fprintf(stderr, "Could not decode %s: %s\n", path, SDL_GetError());
    return 0;
  }
  if (surface->w != 640 || surface->h != 480) {
    fprintf(stderr, "Unexpected WIZBG dimensions: %dx%d\n", surface->w, surface->h);
    SDL_DestroySurface(surface);
    return 0;
  }
  SDL_DestroySurface(surface);
  return 1;
}

static int test_animation(const char *directory, const char *filename,
                          int expected_width, int expected_height,
                          int expect_transparent_corner)
{
  char path[1024];
  IMG_AnimationDecoder *decoder;
  SDL_Surface *frame = NULL;
  Uint64 duration;
  int frame_count = 0;
  SDL_snprintf(path, sizeof(path), "%s/%s", directory, filename);
  decoder = IMG_CreateAnimationDecoder(path);
  if (!decoder) {
    fprintf(stderr, "Could not open %s: %s\n", path, SDL_GetError());
    return 0;
  }
  while (IMG_GetAnimationDecoderFrame(decoder, &frame, &duration)) {
    Uint8 alpha = 255;
    if (frame->w != expected_width || frame->h != expected_height || duration == 0) {
      fprintf(stderr, "Invalid frame %d in %s: %dx%d, %llu ms\n",
              frame_count, path, frame->w, frame->h,
              (unsigned long long)duration);
      SDL_DestroySurface(frame);
      IMG_CloseAnimationDecoder(decoder);
      return 0;
    }
    if (frame_count == 0 &&
        (!SDL_ReadSurfacePixel(frame, 0, 0, NULL, NULL, NULL, &alpha) ||
         (!!(alpha < 255) != !!expect_transparent_corner))) {
      fprintf(stderr, "Unexpected alpha at the top-left of %s: %u\n", path, alpha);
      SDL_DestroySurface(frame);
      IMG_CloseAnimationDecoder(decoder);
      return 0;
    }
    SDL_DestroySurface(frame);
    frame = NULL;
    ++frame_count;
  }
  if (IMG_GetAnimationDecoderStatus(decoder) != IMG_DECODER_STATUS_COMPLETE) {
    fprintf(stderr, "Decode failed for %s: %s\n", path, SDL_GetError());
    IMG_CloseAnimationDecoder(decoder);
    return 0;
  }
  IMG_CloseAnimationDecoder(decoder);
  if (frame_count < 2) {
    fprintf(stderr, "Expected an animation in %s, got %d frame(s)\n", path, frame_count);
    return 0;
  }
  printf("%s: %d frames\n", filename, frame_count);
  return 1;
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: statwin_asset_test <Statwin_SDL directory>\n");
    return 2;
  }
  if (!test_png(argv[1]) ||
      !test_animation(argv[1], "B_FWZ.webp", 224, 168, 1) ||
      !test_animation(argv[1], "WATER.webp", 240, 412, 0))
    return 1;
  return 0;
}
