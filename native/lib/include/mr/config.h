#ifndef __MR_CONFIG_H__
#define __MR_CONFIG_H__

/* 1 = using standard headers and routines */
#define MR_ARCH      std_x86_64

#if MR_ARCH == std_x86 || MR_ARCH == std_x86_64
/* Providing pointer-size integer types by stdint.h */
#include <stdint.h>
typedef uintptr_t mr_uintptr_t;
typedef intptr_t  mr_intptr_t;
typedef uint64_t  mr_uint64_t;
typedef int64_t   mr_int64_t;
typedef uint32_t  mr_uint32_t;
typedef int32_t   mr_int32_t;
#if MR_ARCH == std_x86
#define MR_PTR_SIZE 4
#define MR_PTR_BITS 32
#else
#define MR_PTR_SIZE 8
#define MR_PTR_BITS 64
#endif
/* And memory/string routines from stdlib.h/string.h */
#include <stdlib.h>
#include <string.h>
#define MR_MALLOC  malloc
#define MR_FREE    free
#define MR_REALLOC realloc
#define MR_MEMCPY  memcpy
#define MR_MEMSET  memset
#define MR_STRLEN  strlen
#define MR_MEMCMP  memcmp
#define MR_STRCMP  strcmp
#define MR_STRNCMP strncmp
#define MR_DEBUG_ERROR(v ...)

#include <stdio.h>
#define USING_FILE 1
#define MR_FILE_T  FILE *
#define MR_FPRINTF fprintf
#define MR_INTPTR_FD "%ld"

#else

#error ARCH not supported

#endif

#endif
