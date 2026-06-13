#ifndef SHANDALAR_LEGACY_IO_H
#define SHANDALAR_LEGACY_IO_H

#include <fcntl.h>
#include <stdio.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define _O_BINARY 0
#define _O_RDONLY O_RDONLY
#define _O_WRONLY O_WRONLY
#define _O_CREAT O_CREAT
#define _O_TRUNC O_TRUNC
#define _S_IREAD S_IRUSR
#define _S_IWRITE S_IWUSR
#define _open open
#define _read read
#define _write write
#define _close close
#define _lseek lseek
#define _tell(fd) lseek((fd), 0, SEEK_CUR)
#define _chdir chdir
#define _getcwd getcwd
#define _stricmp strcasecmp
#define _itoa(value, buffer, radix) (snprintf((buffer), 34, ((radix) == 16 ? "%x" : "%d"), (value)), (buffer))

typedef intptr_t intptr_find_t;
struct _finddata_t {
  char name[260];
};

intptr_find_t shandalar_findfirst(const char *pattern, struct _finddata_t *data);
int shandalar_findnext(intptr_find_t handle, struct _finddata_t *data);
int shandalar_findclose(intptr_find_t handle);

#define _findfirst shandalar_findfirst
#define _findnext shandalar_findnext
#define _findclose shandalar_findclose
#endif

#endif
