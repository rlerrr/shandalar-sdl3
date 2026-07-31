#ifndef SHANDALAR_FILE_IO_H
#define SHANDALAR_FILE_IO_H

#include <SDL3/SDL_iostream.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Opens a file through the platform's legacy I/O layer and transfers ownership
 * of it to the returned SDL_IOStream.
 */
SDL_IOStream *shandalar_open_file_io(const char *path, const char *mode);

#ifdef __cplusplus
}
#endif

#endif
