#include "platform/file_io.h"

#include "platform/legacy_io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ShandalarFileIO {
  FILE *file;
  int writable;
} ShandalarFileIO;

static Sint64 SDLCALL file_io_seek(void *userdata, Sint64 offset,
                                   SDL_IOWhence whence)
{
  ShandalarFileIO *file_io = (ShandalarFileIO *)userdata;
  int origin;
  switch (whence) {
  case SDL_IO_SEEK_SET: origin = SEEK_SET; break;
  case SDL_IO_SEEK_CUR: origin = SEEK_CUR; break;
  case SDL_IO_SEEK_END: origin = SEEK_END; break;
  default: return -1;
  }
  if (fseek(file_io->file, (long)offset, origin) != 0)
    return -1;
  return (Sint64)ftell(file_io->file);
}

static size_t SDLCALL file_io_read(void *userdata, void *ptr, size_t size,
                                   SDL_IOStatus *status)
{
  ShandalarFileIO *file_io = (ShandalarFileIO *)userdata;
  size_t bytes = fread(ptr, 1, size, file_io->file);
  if (bytes < size)
    *status = ferror(file_io->file) ? SDL_IO_STATUS_ERROR : SDL_IO_STATUS_EOF;
  return bytes;
}

static size_t SDLCALL file_io_write(void *userdata, const void *ptr,
                                    size_t size, SDL_IOStatus *status)
{
  ShandalarFileIO *file_io = (ShandalarFileIO *)userdata;
  size_t bytes;
  if (!file_io->writable) {
    *status = SDL_IO_STATUS_READONLY;
    return 0;
  }
  bytes = fwrite(ptr, 1, size, file_io->file);
  if (bytes < size)
    *status = SDL_IO_STATUS_ERROR;
  return bytes;
}

static bool SDLCALL file_io_flush(void *userdata, SDL_IOStatus *status)
{
  ShandalarFileIO *file_io = (ShandalarFileIO *)userdata;
  if (!file_io->writable || fflush(file_io->file) == 0)
    return true;
  *status = SDL_IO_STATUS_ERROR;
  return false;
}

static bool SDLCALL file_io_close(void *userdata)
{
  ShandalarFileIO *file_io = (ShandalarFileIO *)userdata;
  int result = fclose(file_io->file);
  free(file_io);
  return result == 0;
}

SDL_IOStream *shandalar_open_file_io(const char *path, const char *mode)
{
  ShandalarFileIO *file_io;
  SDL_IOStream *io;
  SDL_IOStreamInterface interface;

  if (!path || !path[0] || !mode || !mode[0])
    return NULL;

  file_io = (ShandalarFileIO *)malloc(sizeof(*file_io));
  if (!file_io)
    return NULL;
  file_io->file = fopen(path, mode);
  if (!file_io->file) {
    free(file_io);
    return NULL;
  }
  file_io->writable =
      strchr(mode, 'w') != NULL || strchr(mode, 'a') != NULL ||
      strchr(mode, '+') != NULL;

  SDL_INIT_INTERFACE(&interface);
  interface.seek = file_io_seek;
  interface.read = file_io_read;
  interface.write = file_io_write;
  interface.flush = file_io_flush;
  interface.close = file_io_close;
  io = SDL_OpenIO(&interface, file_io);
  if (!io) {
    fclose(file_io->file);
    free(file_io);
  }
  return io;
}
