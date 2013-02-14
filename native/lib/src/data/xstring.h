#ifndef __MR_XSTRING_H__
#define __MR_XSTRING_H__

#include "../internal.h"

/* Simple string encapsulation for string */

typedef struct xstring_s *xstring_t;
typedef struct xstring_s
{
    mr_uintptr_t ref;
    mr_uintptr_t len;
    char        *cstr;
} xstring_s;

#define XSTRING_LEN_UNDEFINED ((mr_uintptr_t)(-1))

xstring_t xstring_from_cstr(const char *cstr, mr_uintptr_t len);
int       xstring_equal_cstr(xstring_t string, const char *cstr, mr_uintptr_t len);
int       xstring_equal(xstring_t a, xstring_t b);
char     *xstring_cstr(xstring_t string);
mr_uintptr_t xstring_len(xstring_t string);
void      xstring_get(xstring_t string);
void      xstring_put(xstring_t string);

#endif
