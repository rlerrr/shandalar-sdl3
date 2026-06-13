#ifndef SHANDALAR_INTTYPES_H
#define SHANDALAR_INTTYPES_H

#include <stdint.h>

typedef unsigned char _BYTE;
typedef uint32_t _DWORD;
typedef unsigned int uint;

typedef unsigned char undefined;
typedef unsigned char byte;
typedef unsigned char undefined1;
typedef unsigned short undefined2;
typedef unsigned int undefined4;
typedef uint64_t undefined8;
typedef int64_t longlong;
typedef uint64_t ulonglong;

#define CONCAT31(x,y) ((undefined4)(((undefined4)(x) << 8) | ((undefined1)(y))))
#define CONCAT11(x,y) ((undefined2)(((undefined1)(x)) | ((undefined2)((undefined1)(y)) << 8)))
#define CONCAT44(x,y) ((undefined8)(((ulonglong)(x) << 32) | (undefined4)(y)))

#endif
