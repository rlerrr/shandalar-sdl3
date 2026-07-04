#ifndef SHANDALAR_LEGACY_IO_H
#define SHANDALAR_LEGACY_IO_H

#include <fcntl.h>
#include <stdio.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#else
#include <dirent.h>
#include <strings.h>
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
#define _read read
#define _write write
#define _close shandalar_close
#define _lseek lseek
#define _tell(fd) lseek((fd), 0, SEEK_CUR)
#define _fileno fileno
#define _filelength shandalar_filelength
#define _chdir shandalar_chdir
#define _getcwd getcwd
#define _stricmp strcasecmp
#define _strcmpi strcasecmp
#define _strnicmp strncasecmp
#define _vsnprintf vsnprintf
#define _strlwr shandalar_strlwr
#define _splitpath shandalar_splitpath
#define _itoa(value, buffer, radix) (snprintf((buffer), 34, ((radix) == 16 ? "%x" : "%d"), (value)), (buffer))

typedef intptr_t intptr_find_t;
struct _finddata_t {
  char name[260];
};

intptr_find_t shandalar_findfirst(const char *pattern, struct _finddata_t *data);
int shandalar_findnext(intptr_find_t handle, struct _finddata_t *data);
int shandalar_findclose(intptr_find_t handle);
char *shandalar_strlwr(char *str);
void shandalar_splitpath(const char *path, char *drive, char *dir, char *fname, char *ext);
FILE *shandalar_fopen(const char *path, const char *mode);
int shandalar_fclose(FILE *stream);
char *shandalar_fgets(char *str, int count, FILE *stream);
int shandalar_open(const char *path, int flags, ...);
int shandalar_close(int fd);
int shandalar_chdir(const char *path);
long shandalar_filelength(int fd);
int shandalar_resolve_existing_path(const char *path, char *out, size_t out_size);
const char *shandalar_user_data_root(void);
int shandalar_user_data_sync(void);

#define _findfirst shandalar_findfirst
#define _findnext shandalar_findnext
#define _findclose shandalar_findclose
#define _open shandalar_open
#ifndef SHANDALAR_LEGACY_IO_IMPLEMENTATION
#define fopen shandalar_fopen
#define fclose shandalar_fclose
#define fgets shandalar_fgets
#endif
#endif

#endif
